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

glm::vec3 calculate_sun_position(float time01) {
    float angle = time01 * 2.0f * glm::pi<float>();
    float latitude = 0.4f;

    glm::vec3 dir;
    dir.x = glm::cos(angle);
    dir.z = glm::sin(angle);
    dir.y = glm::sin(angle) * glm::cos(latitude) + glm::sin(latitude) * 0.2f;

    return glm::normalize(dir);
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

    // Create DescriptorPool
    vk::DescriptorPoolSize descriptorPoolSize;
    descriptorPoolSize.setType(vk::DescriptorType::eStorageImage);
    descriptorPoolSize.setDescriptorCount(1);

    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    descriptorPoolCreateInfo.setMaxSets(10);
    descriptorPoolCreateInfo.setPoolSizes({descriptorPoolSize});
    descriptorPool = device.logicalDevice->createDescriptorPoolUnique(descriptorPoolCreateInfo);

    // Create DescriptorSet with ColorAttachment image for the compute pipeline
    vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding;
    descriptorSetLayoutBinding.setBinding(0);
    descriptorSetLayoutBinding.setDescriptorCount(1);
    descriptorSetLayoutBinding.setDescriptorType(vk::DescriptorType::eStorageImage);
    descriptorSetLayoutBinding.setStageFlags(vk::ShaderStageFlagBits::eCompute);

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.setBindings({descriptorSetLayoutBinding});
    descriptorSetLayout = device.logicalDevice->createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo);

    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.setDescriptorPool(descriptorPool.get());
    descriptorSetAllocateInfo.setDescriptorSetCount(1);
    descriptorSetAllocateInfo.setSetLayouts({descriptorSetLayout.get()});
    descriptorSet = std::move(device.logicalDevice->allocateDescriptorSetsUnique(descriptorSetAllocateInfo)[0]);

    vk::DescriptorImageInfo descriptorImageInfo;
    descriptorImageInfo.setImageView(colorImageView.get());
    descriptorImageInfo.setImageLayout(vk::ImageLayout::eGeneral);

    vk::WriteDescriptorSet colorImageWrite;
    colorImageWrite.setDstBinding(0);
    colorImageWrite.setDstSet(descriptorSet.get());
    colorImageWrite.setDescriptorCount(1);
    colorImageWrite.setDescriptorType(vk::DescriptorType::eStorageImage);
    colorImageWrite.setImageInfo(descriptorImageInfo);

    device.logicalDevice->updateDescriptorSets({colorImageWrite}, {});

    // Create compute pipeline
    {
        vk::PushConstantRange pushRange(vk::ShaderStageFlagBits::eCompute, 0, sizeof(gpu_scene_data));

        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
        pipelineLayoutCreateInfo.setSetLayouts({descriptorSetLayout.get()});
        pipelineLayoutCreateInfo.setPushConstantRanges({pushRange});
        computePipelineLayout = device.logicalDevice->createPipelineLayoutUnique(pipelineLayoutCreateInfo);

        auto cCode = load_spv(BROKEN_SHADER_COMP);
        auto cMod = device.logicalDevice->createShaderModuleUnique(
            vk::ShaderModuleCreateInfo({}, cCode.size() * 4, cCode.data()));
        pipeline_builder builder;
        builder.add_shader(cMod.get(), vk::ShaderStageFlagBits::eCompute);
        computePipeline = builder.build_compute_pipeline(device.logicalDevice.get(), computePipelineLayout.get());
    }

    // Create graphics pipeline
    {
        vk::PushConstantRange pushRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(gpu_scene_data));
        graphicsPipelineLayout =
            device.logicalDevice->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, {}, pushRange));

        auto vCode = load_spv(BROKEN_SHADER_VERT);
        auto fCode = load_spv(BROKEN_SHADER_FRAG);
        auto vMod = device.logicalDevice->createShaderModuleUnique(
            vk::ShaderModuleCreateInfo({}, vCode.size() * 4, vCode.data()));
        auto fMod = device.logicalDevice->createShaderModuleUnique(
            vk::ShaderModuleCreateInfo({}, fCode.size() * 4, fCode.data()));

        pipeline_builder builder;
        builder.add_shader(vMod.get(), vk::ShaderStageFlagBits::eVertex);
        builder.add_shader(fMod.get(), vk::ShaderStageFlagBits::eFragment);
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
    indexSSBOCapacity = 64 * 1024 * 1024; // 64 Mb (enough for ~33.5 million uint16_t indices)
    device.create_buffer(indexSSBOCapacity,
                         vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress |
                             vk::BufferUsageFlagBits::eTransferDst,
                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                         indexSSBO,
                         indexSSBOMemory);
    indexSSBODeviceAddress = device.logicalDevice->getBufferAddress({*indexSSBO});
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

    gpu_scene_data sceneData;
    sceneData.invViewProj = invViewProj;
    sceneData.sunDirection = glm::vec4(scene.sunDirection, 0.f);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline.get());
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, computePipelineLayout.get(), 0, 1, &descriptorSet.get(), 0, nullptr);
    commandBuffer.pushConstants<gpu_scene_data>(
        computePipelineLayout.get(), vk::ShaderStageFlagBits::eCompute, 0, sceneData);

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

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.get());

    sceneData.viewProj = scene.camera.projMatrix * scene.camera.viewMatrix;
    for (const auto& [cpuMesh, transform] : scene.objects) {
        const auto& gpuMesh = get_or_create_gpu_mesh_data(cpuMesh);
        sceneData.vertexSSBODeviceAddress = gpuMesh.vertexSSBODeviceAddress;
        sceneData.vertexSSBOOffset = gpuMesh.vertexSSBOOffset;
        sceneData.indexSSBODeviceAddress = gpuMesh.indexSSBODeviceAddress;
        sceneData.indexSSBOOffset = gpuMesh.indexSSBOOffset;
        sceneData.model = transform;
        commandBuffer.pushConstants<gpu_scene_data>(
            graphicsPipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, sceneData);
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
