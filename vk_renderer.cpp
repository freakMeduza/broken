#include "vk_renderer.hpp"
#include "vk_pipeline.hpp"
#include <GLFW/glfw3.h>

#include <fstream>
#include <string>

namespace broken::vulkan {

static std::vector<uint32_t> load_spv(const std::string& filename) {
    if (auto file = std::fstream(filename, std::ios::in | std::ios::binary); file.is_open()) {
        file.seekg(0, std::ios::end);
        auto size = (size_t)file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<uint32_t> buffer(size / sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(buffer.data()), size);
        return buffer;
    }
    return {};
}

renderer::renderer(GLFWwindow* window) noexcept : device(window) {
    // Create CommandBuffer
    commandPool = device.logicalDevice->createCommandPoolUnique(
        vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, 0));
    commandBuffer = device.logicalDevice->allocateCommandBuffers(
        vk::CommandBufferAllocateInfo(commandPool.get(), vk::CommandBufferLevel::ePrimary, 1))[0];

    // Create Synchronization
    imageAvailableSemaphore = device.logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());
    renderFinishedSemaphore = device.logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());

    // Create ColorAttachment image
    colorExtent = device.swapchainExtent;
    colorFormat = vk::Format::eR16G16B16A16Sfloat;
    device.create_image(vk::Extent3D(colorExtent.width, colorExtent.height, 1),
                        colorFormat,
                        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc |
                            vk::ImageUsageFlagBits::eStorage,
                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                        colorImage,
                        colorImageView,
                        colorImageMemory);

    // Create DepthAttachment image
    depthExtent = device.swapchainExtent;
    depthFormat = vk::Format::eD32Sfloat;
    device.create_image(vk::Extent3D(depthExtent.width, depthExtent.height, 1),
                        depthFormat,
                        vk::ImageUsageFlagBits::eDepthStencilAttachment,
                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                        depthImage,
                        depthImageView,
                        depthImageMemory);

    // Create static Vertex SSBO (Storage Shader Buffer Object)
    vertexSSBOSize = 0;
    vertexSSBOCapacity = 256 * 1024 * 1024; // 256 Mb
    device.create_buffer(vertexSSBOCapacity,
                         vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress |
                             vk::BufferUsageFlagBits::eTransferDst,
                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                         vertexSSBO,
                         vertexSSBOMemory);
    vertexSSBODeviceAddress = device.logicalDevice->getBufferAddress({*vertexSSBO});

    // Create static Index SSBO (Storage Shader Buffer Object)
    indexSSBOSize = 0;
    indexSSBOCapacity = 64 * 1024 * 1024; // 64 Mb (enough for ~16.7 million uint32_t indices)
    device.create_buffer(indexSSBOCapacity,
                         vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress |
                             vk::BufferUsageFlagBits::eTransferDst,
                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                         indexSSBO,
                         indexSSBOMemory);
    indexSSBODeviceAddress = device.logicalDevice->getBufferAddress({*indexSSBO});

    // Create Global Scene UBO (Uniform Buffer Object)
    sceneUBOSize = 0;
    sceneUBOCapacity = sizeof(scene_uniform_buffer);
    device.create_buffer(sceneUBOCapacity,
                         vk::BufferUsageFlagBits::eUniformBuffer,
                         vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
                         sceneUBO,
                         sceneUBOMemory);

    // Create DescriptorPool
    std::vector<vk::DescriptorPoolSize> descriptorPoolSizes;
    descriptorPoolSizes.emplace_back(vk::DescriptorType::eStorageImage, 1);
    descriptorPoolSizes.emplace_back(vk::DescriptorType::eUniformBuffer, 1);

    descriptorPool = device.logicalDevice->createDescriptorPoolUnique({
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        10,
        descriptorPoolSizes,
    });

    // Create DescriptorSet
    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetBindings;
    descriptorSetBindings.emplace_back(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute);
    descriptorSetBindings.emplace_back(1,
                                       vk::DescriptorType::eUniformBuffer,
                                       1,
                                       vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

    descriptorSetLayout = device.logicalDevice->createDescriptorSetLayoutUnique({{}, descriptorSetBindings});

    descriptorSet = std::move(device.logicalDevice->allocateDescriptorSetsUnique({
        descriptorPool.get(),
        {descriptorSetLayout.get()},
    })[0]);

    // Write Descriptor Sets
    vk::DescriptorImageInfo storageImageInfo{
        {},
        colorImageView.get(),
        vk::ImageLayout::eGeneral,
    };

    vk::WriteDescriptorSet storageImageWrite{
        descriptorSet.get(),
        0,
        0,
        vk::DescriptorType::eStorageImage,
        {storageImageInfo},
    };

    vk::DescriptorBufferInfo sceneBufferInfo{
        sceneUBO.get(),
        0,
        VK_WHOLE_SIZE,
    };

    vk::WriteDescriptorSet sceneBufferWrite{
        descriptorSet.get(),
        1,
        0,
        vk::DescriptorType::eUniformBuffer,
        {},
        {sceneBufferInfo},
    };

    device.logicalDevice->updateDescriptorSets({storageImageWrite, sceneBufferWrite}, {});

    pipeline_builder builder;
    // Create compute pipeline
    vk::PushConstantRange computePushRange{
        vk::ShaderStageFlagBits::eCompute,
        0,
        sizeof(compute_constants),
    };

    vk::PipelineLayoutCreateInfo computePipelineLayoutCreateInfo{
        {},
        {descriptorSetLayout.get()},
        {computePushRange},
    };

    computePipelineLayout = device.logicalDevice->createPipelineLayoutUnique(computePipelineLayoutCreateInfo);

    const auto compShaderCode = load_spv(BROKEN_SHADER_COMP);
    const auto compShaderModule = device.logicalDevice->createShaderModuleUnique(
        {{}, compShaderCode.size() * sizeof(compShaderCode[0]), compShaderCode.data()});

    builder.add_shader(compShaderModule.get(), vk::ShaderStageFlagBits::eCompute);

    computePipeline = builder.build_compute_pipeline(device.logicalDevice.get(), computePipelineLayout.get());

    builder.reset();
    // Create graphics pipeline
    vk::PushConstantRange graphicsPushRange(
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(scene_constants));

    vk::PipelineLayoutCreateInfo graphicsPipelineLayoutCreateInfo{
        {},
        {descriptorSetLayout.get()},
        {graphicsPushRange},
    };

    graphicsPipelineLayout = device.logicalDevice->createPipelineLayoutUnique(graphicsPipelineLayoutCreateInfo);

    auto vertShaderCode = load_spv(BROKEN_SHADER_VERT);
    auto fragShaderCode = load_spv(BROKEN_SHADER_FRAG);
    auto vertShaderModule = device.logicalDevice->createShaderModuleUnique(
        vk::ShaderModuleCreateInfo({}, vertShaderCode.size() * sizeof(vertShaderCode[0]), vertShaderCode.data()));
    auto fragShaderModule = device.logicalDevice->createShaderModuleUnique(
        vk::ShaderModuleCreateInfo({}, fragShaderCode.size() * sizeof(fragShaderCode[0]), fragShaderCode.data()));

    builder.add_shader(vertShaderModule.get(), vk::ShaderStageFlagBits::eVertex);
    builder.add_shader(fragShaderModule.get(), vk::ShaderStageFlagBits::eFragment);
    builder.set_input_topology(vk::PrimitiveTopology::eTriangleList);
    builder.set_polygon_mode(vk::PolygonMode::eFill);
    builder.set_cull_mode(vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise);
    builder.disable_multisampling();
    builder.disable_blending();
    builder.enable_depth_test(true, vk::CompareOp::eLess);
    builder.set_color_attachment_format(colorFormat);
    builder.set_depth_attachment_format(depthFormat);

    graphicsPipeline = builder.build_graphics_pipeline(device.logicalDevice.get(), graphicsPipelineLayout.get());
}

void renderer::draw(const broken::scene& scene) {
    const auto [imageIndexResult, imageIndexValue] = device.logicalDevice->acquireNextImageKHR(
        device.swapchain.get(), std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore.get(), nullptr);
    imageIndex = imageIndexValue;

    commandBuffer.reset();
    commandBuffer.begin(vk::CommandBufferBeginInfo());

    device.transition_image_layout(
        commandBuffer, colorImage.get(), colorFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

    glm::mat4 staticView = glm::mat4(glm::mat3(scene.camera.viewMatrix));
    glm::mat4 invViewProj = glm::inverse(scene.camera.projMatrix * staticView);

    compute_constants compPush;
    compPush.data1 = invViewProj;
    compPush.data2 = glm::vec4(scene.sunDirection, (float)glfwGetTime());
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline.get());
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, computePipelineLayout.get(), 0, 1, &descriptorSet.get(), 0, nullptr);
    commandBuffer.pushConstants<compute_constants>(
        computePipelineLayout.get(), vk::ShaderStageFlagBits::eCompute, 0, compPush);

    uint32_t groupCountX = (colorExtent.width + 15) / 16;
    uint32_t groupCountY = (colorExtent.height + 15) / 16;

    commandBuffer.dispatch(groupCountX, groupCountY, 1);

    device.transition_image_layout(commandBuffer,
                                   colorImage.get(),
                                   colorFormat,
                                   vk::ImageLayout::eGeneral,
                                   vk::ImageLayout::eColorAttachmentOptimal);

    device.transition_image_layout(commandBuffer,
                                   depthImage.get(),
                                   depthFormat,
                                   vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::RenderingAttachmentInfo colorAttachment;
    colorAttachment.setImageView(colorImageView.get());
    colorAttachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
    colorAttachment.setLoadOp(vk::AttachmentLoadOp::eLoad);
    colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);

    vk::RenderingAttachmentInfo depthAttachment;
    depthAttachment.setImageView(depthImageView.get());
    depthAttachment.setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    depthAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    depthAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    depthAttachment.setClearValue(vk::ClearDepthStencilValue(1.0f, 0));

    vk::RenderingInfo renderingInfo;
    renderingInfo.setRenderArea(vk::Rect2D({0, 0}, colorExtent));
    renderingInfo.setLayerCount(1);
    renderingInfo.setColorAttachments(colorAttachment);
    renderingInfo.setPDepthAttachment(&depthAttachment);

    commandBuffer.beginRendering(renderingInfo);

    vk::Viewport viewport = {
        0.0f,
        0.0f,
        (float)colorExtent.width,
        (float)colorExtent.height,
        0.0f,
        1.0f,
    };

    commandBuffer.setViewport(0, viewport);

    vk::Rect2D scissor = {
        {0, 0},
        colorExtent,
    };

    commandBuffer.setScissor(0, scissor);

    // Update Global Scene UBO
    scene_uniform_buffer sceneUBOData;
    sceneUBOData.projMatrix = scene.camera.projMatrix;
    sceneUBOData.viewMatrix = scene.camera.viewMatrix;
    sceneUBOData.sunDirection = glm::vec4(scene.sunDirection, 0.f);
    auto sceneUBOMemoryMapped = device.logicalDevice->mapMemory(sceneUBOMemory.get(), 0, sceneUBOCapacity);
    std::memcpy(sceneUBOMemoryMapped, &sceneUBOData, sceneUBOCapacity);
    device.logicalDevice->unmapMemory(sceneUBOMemory.get());

    // Bind Global Scene UBO
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, graphicsPipelineLayout.get(), 0, {descriptorSet.get()}, {});

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.get());

    for (const auto& [cpuMesh, transform] : scene.objects) {
        const auto& gpuMesh = get_or_create_gpu_mesh_data(cpuMesh);

        scene_constants scenePush;
        scenePush.modelMatrix = transform;
        scenePush.vertexSSBODeviceAddress = gpuMesh.vertexSSBODeviceAddress;
        scenePush.vertexSSBOOffset = gpuMesh.vertexSSBOOffset;
        scenePush.indexSSBODeviceAddress = gpuMesh.indexSSBODeviceAddress;
        scenePush.indexSSBOOffset = gpuMesh.indexSSBOOffset;
        commandBuffer.pushConstants<scene_constants>(
            graphicsPipelineLayout.get(),
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            0,
            scenePush);

        commandBuffer.draw(gpuMesh.indexCount, 1, 0, 0);
    }

    commandBuffer.endRendering();

    device.transition_image_layout(commandBuffer,
                                   colorImage.get(),
                                   colorFormat,
                                   vk::ImageLayout::eColorAttachmentOptimal,
                                   vk::ImageLayout::eTransferSrcOptimal);

    device.transition_image_layout(commandBuffer,
                                   device.swapchainImages[imageIndex],
                                   device.swapchainFormat,
                                   vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eTransferDstOptimal);

    device.copy_image_to_image(
        commandBuffer, colorImage.get(), device.swapchainImages[imageIndex], colorExtent, device.swapchainExtent);

    device.transition_image_layout(commandBuffer,
                                   device.swapchainImages[imageIndex],
                                   device.swapchainFormat,
                                   vk::ImageLayout::eTransferDstOptimal,
                                   vk::ImageLayout::ePresentSrcKHR);

    commandBuffer.end();

    // ======== Submit command buffer ========
    vk::SemaphoreSubmitInfo waitSemaphoreSubmitInfo;
    waitSemaphoreSubmitInfo.setSemaphore(imageAvailableSemaphore.get());
    waitSemaphoreSubmitInfo.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
    waitSemaphoreSubmitInfo.setDeviceIndex(0);
    waitSemaphoreSubmitInfo.setValue(1);

    vk::SemaphoreSubmitInfo signalSemaphoreSubmitInfo;
    signalSemaphoreSubmitInfo.setSemaphore(renderFinishedSemaphore.get());
    signalSemaphoreSubmitInfo.setStageMask(vk::PipelineStageFlagBits2::eAllGraphics);
    signalSemaphoreSubmitInfo.setDeviceIndex(0);
    signalSemaphoreSubmitInfo.setValue(1);

    vk::CommandBufferSubmitInfo commandBufferSubmitInfo;
    commandBufferSubmitInfo.setCommandBuffer(commandBuffer);
    commandBufferSubmitInfo.setDeviceMask(0);

    vk::SubmitInfo2 submitInfo;
    submitInfo.setCommandBufferInfos(commandBufferSubmitInfo);
    submitInfo.setWaitSemaphoreInfos(waitSemaphoreSubmitInfo);
    submitInfo.setSignalSemaphoreInfos(signalSemaphoreSubmitInfo);

    device.graphicsQueue.submit2(submitInfo);
    // =======================================

    // ============ Present image ============
    vk::PresentInfoKHR presentInfo;
    presentInfo.setSwapchains(device.swapchain.get());
    presentInfo.setImageIndices(imageIndex);
    presentInfo.setWaitSemaphores(signalSemaphoreSubmitInfo.semaphore);

    [[maybe_unused]] auto result = device.graphicsQueue.presentKHR(presentInfo);
    // =======================================

    device.graphicsQueue.waitIdle();
}

const renderer::gpu_mesh_data& renderer::get_or_create_gpu_mesh_data(const broken::mesh& cpuMesh) noexcept {
    auto it = meshCache.find(&cpuMesh);
    if (it != meshCache.end()) {
        return it->second;
    }

    const auto vertexBufferSize = cpuMesh.vertices.size() * sizeof(broken::mesh::vertex);
    const auto indexBufferSize = cpuMesh.indices.size() * sizeof(broken::mesh::index);
    const auto stagingBufferSize = vertexBufferSize + indexBufferSize;

    vk::UniqueBuffer stagingBuffer;
    vk::UniqueDeviceMemory stagingBufferMemory;
    device.create_buffer(stagingBufferSize,
                         vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                         stagingBuffer,
                         stagingBufferMemory);

    void* data = device.logicalDevice->mapMemory(stagingBufferMemory.get(), 0, stagingBufferSize);

    // Copy VBO data
    std::memcpy(data, cpuMesh.vertices.data(), vertexBufferSize);
    // Copy IBO data
    std::memcpy((char*)data + vertexBufferSize, cpuMesh.indices.data(), indexBufferSize);

    device.logicalDevice->unmapMemory(stagingBufferMemory.get());

    gpu_mesh_data gpuMesh;

    device.immediate_submit([&](vk::CommandBuffer cmd) {
        vk::BufferCopy vertexCopy{};
        vertexCopy.srcOffset = 0;
        vertexCopy.dstOffset = vertexSSBOSize;
        vertexCopy.size = vertexBufferSize;
        cmd.copyBuffer(stagingBuffer.get(), vertexSSBO.get(), vertexCopy);

        vk::BufferCopy indexCopy{};
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.dstOffset = indexSSBOSize;
        indexCopy.size = indexBufferSize;
        cmd.copyBuffer(stagingBuffer.get(), indexSSBO.get(), indexCopy);
    });

    gpuMesh.vertexSSBODeviceAddress = vertexSSBODeviceAddress;
    gpuMesh.vertexSSBOOffset = static_cast<uint32_t>(vertexSSBOSize / sizeof(broken::mesh::vertex));
    gpuMesh.indexSSBODeviceAddress = indexSSBODeviceAddress;
    gpuMesh.indexSSBOOffset = static_cast<uint32_t>(indexSSBOSize / sizeof(broken::mesh::index));
    gpuMesh.indexCount = static_cast<uint32_t>(cpuMesh.indices.size());

    vertexSSBOSize += vertexBufferSize;
    indexSSBOSize += indexBufferSize;

    return meshCache.emplace(&cpuMesh, std::move(gpuMesh)).first->second;
}

} // namespace broken::vulkan
