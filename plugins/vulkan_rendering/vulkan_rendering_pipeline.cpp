#include "vulkan_rendering_pipeline.hpp"

#include "vulkan_rendering_device.hpp"
#include <broken/spv_shader_compiler.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

namespace broken {

struct vulkan_rendering_pipeline_builder {
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfos;
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
    vk::PipelineRenderingCreateInfo renderingCreateInfo;
    vk::Format colorAttachmentFormat;

    void reset() {
        shaderStageCreateInfos.clear();
        inputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{};
        rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo{};
        colorBlendAttachmentState = vk::PipelineColorBlendAttachmentState{};
        multisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo{};
        depthStencilStateCreateInfo = vk::PipelineDepthStencilStateCreateInfo{};
        renderingCreateInfo = vk::PipelineRenderingCreateInfo{};
        colorAttachmentFormat = vk::Format::eUndefined;
    }

    vk::UniquePipeline make_graphics_pipeline(vk::Device logicalDevice, vk::PipelineLayout pipelineLayout) const {
        vk::PipelineViewportStateCreateInfo viewportStateCreateInfo;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.scissorCount = 1;

        vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
        colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        colorBlendStateCreateInfo.logicOp = vk::LogicOp::eCopy;
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

        // completely clear VertexInputStateCreateInfo, as we have no need for
        // it
        vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;

        vk::DynamicState states[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

        vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo;
        dynamicStateCreateInfo.setDynamicStates(states);

        vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.pNext = &renderingCreateInfo;
        pipelineCreateInfo.stageCount = (uint32_t)shaderStageCreateInfos.size();
        pipelineCreateInfo.pStages = shaderStageCreateInfos.data();
        pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        pipelineCreateInfo.layout = pipelineLayout;
        pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

        auto [result, pipeline] = logicalDevice.createGraphicsPipelineUnique(nullptr, pipelineCreateInfo);
        return std::move(pipeline);
    }

    vk::UniquePipeline make_compute_pipeline(vk::Device logicalDevice, vk::PipelineLayout pipelineLayout) const {
        vk::ComputePipelineCreateInfo computePipelineCreateInfo;
        computePipelineCreateInfo.layout = pipelineLayout;
        computePipelineCreateInfo.setStage(!shaderStageCreateInfos.empty() ? shaderStageCreateInfos[0]
                                                                           : vk::PipelineShaderStageCreateInfo{});

        auto [result, pipeline] = logicalDevice.createComputePipelineUnique(nullptr, computePipelineCreateInfo);
        return std::move(pipeline);
    }

    void add_shader(vk::ShaderModule shaderModule, vk::ShaderStageFlagBits shaderStage) {
        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{};
        shaderStageCreateInfo.stage = shaderStage;
        shaderStageCreateInfo.module = shaderModule;
        shaderStageCreateInfo.pName = "main";
        shaderStageCreateInfos.emplace_back(shaderStageCreateInfo);
    }

    void set_input_topology(vk::PrimitiveTopology topology) {
        inputAssemblyStateCreateInfo.topology = topology;
        inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
    }

    void set_polygon_mode(vk::PolygonMode mode) {
        rasterizationStateCreateInfo.polygonMode = mode;
        rasterizationStateCreateInfo.lineWidth = 1.f;
    }

    void set_cull_mode(vk::CullModeFlags cullMode, vk::FrontFace frontFace) {
        rasterizationStateCreateInfo.cullMode = cullMode;
        rasterizationStateCreateInfo.frontFace = frontFace;
    }

    void disable_multisampling() {
        multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        // multisampling defaulted to no multisampling (1 sample per pixel)
        multisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
        multisampleStateCreateInfo.minSampleShading = 1.0f;
        multisampleStateCreateInfo.pSampleMask = nullptr;
        // no alpha to coverage either
        multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
    }

    void disable_blending() {
        // default write mask
        colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                   vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        // no blending
        colorBlendAttachmentState.blendEnable = VK_FALSE;
    }

    void enable_blending_additive() {
        colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                   vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colorBlendAttachmentState.blendEnable = VK_TRUE;
        colorBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        colorBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOne;
        colorBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
        colorBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        colorBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        colorBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
    }

    void enable_blending_alpha_blend() {
        colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                   vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colorBlendAttachmentState.blendEnable = VK_TRUE;
        colorBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        colorBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        colorBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
        colorBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        colorBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        colorBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
    }

    void set_color_attachment_format(vk::Format format) {
        colorAttachmentFormat = format;
        // connect the format to the renderInfo  structure
        renderingCreateInfo.colorAttachmentCount = 1;
        renderingCreateInfo.pColorAttachmentFormats = &colorAttachmentFormat;
    }

    void set_depth_attachment_format(vk::Format format) { renderingCreateInfo.depthAttachmentFormat = format; }

    void disable_depth_test() {
        depthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
        depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eNever;
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.minDepthBounds = 0.f;
        depthStencilStateCreateInfo.maxDepthBounds = 1.f;
    }

    void enable_depth_test(bool depthWriteEnable, vk::CompareOp op) {
        depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
        depthStencilStateCreateInfo.depthCompareOp = op;
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.minDepthBounds = 0.f;
        depthStencilStateCreateInfo.maxDepthBounds = 1.f;
    }
};

vulkan_rendering_compute_pipeline::vulkan_rendering_compute_pipeline(vk::Device logicalDevice, const char* shaderPath) {
    vk::DescriptorPoolSize descriptorPoolSize;
    descriptorPoolSize.setType(vk::DescriptorType::eStorageImage);
    descriptorPoolSize.setDescriptorCount(1);

    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    descriptorPoolCreateInfo.setMaxSets(10);
    descriptorPoolCreateInfo.setPoolSizes({descriptorPoolSize});
    descriptorPool = logicalDevice.createDescriptorPoolUnique(descriptorPoolCreateInfo);

    vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding;
    descriptorSetLayoutBinding.setBinding(0);
    descriptorSetLayoutBinding.setDescriptorCount(1);
    descriptorSetLayoutBinding.setDescriptorType(vk::DescriptorType::eStorageImage);
    descriptorSetLayoutBinding.setStageFlags(vk::ShaderStageFlagBits::eCompute);

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.setBindings({descriptorSetLayoutBinding});
    descriptorSetLayout = logicalDevice.createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo);

    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.setDescriptorPool(descriptorPool.get());
    descriptorSetAllocateInfo.setDescriptorSetCount(1);
    descriptorSetAllocateInfo.setSetLayouts({descriptorSetLayout.get()});
    descriptorSet = std::move(logicalDevice.allocateDescriptorSetsUnique(descriptorSetAllocateInfo)[0]);

    vk::PushConstantRange pushConstantRange;
    pushConstantRange.setStageFlags(vk::ShaderStageFlagBits::eCompute);
    pushConstantRange.setSize(sizeof(float));
    pushConstantRange.setOffset(0);

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.setSetLayouts({descriptorSetLayout.get()});
    pipelineLayoutCreateInfo.setPushConstantRanges({pushConstantRange});

    pipelineLayout = logicalDevice.createPipelineLayoutUnique(pipelineLayoutCreateInfo);

    spv_shader_compiler compiler;
    auto shaderCode = compiler.glsl_to_spv_vulkan(shaderPath);
    vk::ShaderModuleCreateInfo shaderModuleCreateInfo;
    shaderModuleCreateInfo.setCode(shaderCode);
    vk::UniqueShaderModule shaderModule = logicalDevice.createShaderModuleUnique(shaderModuleCreateInfo);

    vulkan_rendering_pipeline_builder builder;
    builder.add_shader(shaderModule.get(), vk::ShaderStageFlagBits::eCompute);

    pipeline = builder.make_compute_pipeline(logicalDevice, pipelineLayout.get());
}

} // namespace broken
