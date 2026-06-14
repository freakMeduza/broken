#include "vk_renderer.hpp"
#include "vk_pipeline.hpp"
#include <GLFW/glfw3.h>

namespace broken::vulkan {

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
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    commandPool = device.logicalDevice->createCommandPoolUnique(
        vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, 0));
    commandBuffer = device.logicalDevice
                        ->allocateCommandBuffers(
                            vk::CommandBufferAllocateInfo(commandPool.get(), vk::CommandBufferLevel::ePrimary, 1))
                        .front();

    imageAvailableSemaphore = device.logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());
    renderFinishedSemaphore = device.logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());

    vk::DescriptorPoolSize descriptorPoolSize;
    descriptorPoolSize.setType(vk::DescriptorType::eStorageImage);
    descriptorPoolSize.setDescriptorCount(1);

    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    descriptorPoolCreateInfo.setMaxSets(10);
    descriptorPoolCreateInfo.setPoolSizes({descriptorPoolSize});
    descriptorPool = device.logicalDevice->createDescriptorPoolUnique(descriptorPoolCreateInfo);

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
    descriptorImageInfo.setImageView(device.colorImageView.get());
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
        builder.set_color_attachment_format(device.colorFormat);
        builder.set_depth_attachment_format(device.depthFormat);

        graphicsPipeline = builder.build_graphics_pipeline(device.logicalDevice.get(), graphicsPipelineLayout.get());
    }

    device.create_buffer(ssboMaxSizeBytes,
                         vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress |
                             vk::BufferUsageFlagBits::eTransferDst,
                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                         ssbo.buffer,
                         ssbo.memory);

    ssbo.address = device.logicalDevice->getBufferAddress({*ssbo.buffer});
}

void renderer::draw(const broken::scene& scene) {
    const auto [imageIndexResult, imageIndexValue] = device.logicalDevice->acquireNextImageKHR(
        device.swapchain.get(), std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore.get(), nullptr);
    imageIndex = imageIndexValue;

    commandBuffer.reset();
    commandBuffer.begin(vk::CommandBufferBeginInfo());

    device.transition_image_layout(commandBuffer,
                                   device.colorImage.get(),
                                   device.colorFormat,
                                   vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eGeneral);

    glm::mat4 staticView = glm::mat4(glm::mat3(scene.viewMatrix));
    glm::mat4 invViewProj = glm::inverse(scene.projMatrix * staticView);

    gpu_scene_data sceneData;
    sceneData.invViewProj = invViewProj;
    sceneData.sunDirection = glm::vec4(scene.sunDirection, 0.f);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline.get());
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, computePipelineLayout.get(), 0, 1, &descriptorSet.get(), 0, nullptr);
    commandBuffer.pushConstants<gpu_scene_data>(
        computePipelineLayout.get(), vk::ShaderStageFlagBits::eCompute, 0, sceneData);

    uint32_t groupCountX = (device.colorExtent.width + 15) / 16;
    uint32_t groupCountY = (device.colorExtent.height + 15) / 16;

    commandBuffer.dispatch(groupCountX, groupCountY, 1);

    device.transition_image_layout(commandBuffer,
                                   device.colorImage.get(),
                                   device.colorFormat,
                                   vk::ImageLayout::eGeneral,
                                   vk::ImageLayout::eColorAttachmentOptimal);

    device.transition_image_layout(commandBuffer,
                                   device.depthImage.get(),
                                   device.depthFormat,
                                   vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::RenderingAttachmentInfo colorAttachment;
    colorAttachment.setImageView(device.colorImageView.get());
    colorAttachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
    colorAttachment.setLoadOp(vk::AttachmentLoadOp::eLoad);
    colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);

    vk::RenderingAttachmentInfo depthAttachment;
    depthAttachment.setImageView(device.depthImageView.get());
    depthAttachment.setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    depthAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    depthAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    depthAttachment.setClearValue(vk::ClearDepthStencilValue(1.0f, 0));

    vk::RenderingInfo renderingInfo;
    renderingInfo.setRenderArea(vk::Rect2D({0, 0}, device.colorExtent));
    renderingInfo.setLayerCount(1);
    renderingInfo.setColorAttachments(colorAttachment);
    renderingInfo.setPDepthAttachment(&depthAttachment);

    commandBuffer.beginRendering(renderingInfo);

    vk::Viewport viewport = {
        0.0f,
        0.0f,
        (float)device.colorExtent.width,
        (float)device.colorExtent.height,
        0.0f,
        1.0f,
    };

    commandBuffer.setViewport(0, viewport);

    vk::Rect2D scissor = {
        {0, 0},
        device.colorExtent,
    };

    commandBuffer.setScissor(0, scissor);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.get());

    sceneData.viewProj = scene.projMatrix * scene.viewMatrix;
    for (const auto& [cpuMesh, transform] : scene.objects) {
        const auto& gpuMesh = get_or_create_gpu_mesh_data(cpuMesh);
        sceneData.ssboAddress = gpuMesh.ssboAddress;
        sceneData.ssboOffset = gpuMesh.ssboOffset;
        sceneData.model = transform;
        commandBuffer.pushConstants<gpu_scene_data>(
            graphicsPipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, sceneData);
        commandBuffer.bindIndexBuffer(gpuMesh.indexBuffer.get(), 0, vk::IndexType::eUint16);
        commandBuffer.drawIndexed(gpuMesh.indexCount, 1, 0, 0, 0);
    }

    commandBuffer.endRendering();

    device.transition_image_layout(commandBuffer,
                                   device.colorImage.get(),
                                   device.colorFormat,
                                   vk::ImageLayout::eColorAttachmentOptimal,
                                   vk::ImageLayout::eTransferSrcOptimal);

    device.transition_image_layout(commandBuffer,
                                   device.swapchainImages[imageIndex],
                                   device.swapchainFormat,
                                   vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eTransferDstOptimal);

    device.copy_image_to_image(commandBuffer,
                               device.colorImage.get(),
                               device.swapchainImages[imageIndex],
                               device.colorExtent,
                               device.swapchainExtent);

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

    const auto vertexBufferSize = cpuMesh.vertices.size() * sizeof(cpuMesh.vertices[0]);
    const auto indexBufferSize = cpuMesh.indices.size() * sizeof(cpuMesh.indices[0]);
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

    device.create_buffer(indexBufferSize,
                         vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                         gpuMesh.indexBuffer,
                         gpuMesh.indexBufferMemory);

    device.immediate_submit([&](vk::CommandBuffer cmd) {
        vk::BufferCopy vertexCopy{};
        vertexCopy.srcOffset = 0;
        vertexCopy.dstOffset = ssbo.currentAllocatedBytes;
        vertexCopy.size = vertexBufferSize;
        cmd.copyBuffer(stagingBuffer.get(), ssbo.buffer.get(), vertexCopy);

        vk::BufferCopy indexCopy{};
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.dstOffset = 0;
        indexCopy.size = indexBufferSize;
        cmd.copyBuffer(stagingBuffer.get(), gpuMesh.indexBuffer.get(), indexCopy);
    });

    gpuMesh.ssboAddress = ssbo.address;
    gpuMesh.ssboOffset = static_cast<uint32_t>(ssbo.currentAllocatedBytes / sizeof(vertex));
    gpuMesh.indexCount = static_cast<uint32_t>(cpuMesh.indices.size());

    ssbo.currentAllocatedBytes += vertexBufferSize;

    return meshCache.emplace(&cpuMesh, std::move(gpuMesh)).first->second;
}

} // namespace broken::vulkan
