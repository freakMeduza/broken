#include "vk_pipeline.hpp"

namespace broken::vulkan {

void pipeline_builder::reset() {
    shaderStageCreateInfos.clear();
    inputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{};
    vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{};
    rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo{};
    colorBlendAttachmentState = vk::PipelineColorBlendAttachmentState{};
    multisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo{};
    depthStencilStateCreateInfo = vk::PipelineDepthStencilStateCreateInfo{};
    colorAttachmentFormat = vk::Format::eUndefined;
    depthAttachmentFormat = vk::Format::eUndefined;
}

vk::UniquePipeline pipeline_builder::build_graphics_pipeline(vk::Device logicalDevice,
                                                             vk::PipelineLayout pipelineLayout) const {
    vk::PipelineRenderingCreateInfo renderingCreateInfo;
    if (colorAttachmentFormat != vk::Format::eUndefined) {
        renderingCreateInfo.setColorAttachmentFormats(colorAttachmentFormat);
    }
    if (depthAttachmentFormat != vk::Format::eUndefined) {
        renderingCreateInfo.setDepthAttachmentFormat(depthAttachmentFormat);
    }

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.scissorCount = 1;

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.logicOp = vk::LogicOp::eCopy;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

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

    return std::move(logicalDevice.createGraphicsPipelineUnique(nullptr, pipelineCreateInfo).value);
}

vk::UniquePipeline pipeline_builder::build_compute_pipeline(vk::Device logicalDevice,
                                                            vk::PipelineLayout pipelineLayout) const {
    vk::ComputePipelineCreateInfo computePipelineCreateInfo;
    computePipelineCreateInfo.layout = pipelineLayout;
    computePipelineCreateInfo.setStage(!shaderStageCreateInfos.empty() ? shaderStageCreateInfos[0]
                                                                       : vk::PipelineShaderStageCreateInfo{});

    return std::move(logicalDevice.createComputePipelineUnique(nullptr, computePipelineCreateInfo).value);
}

void pipeline_builder::add_shader(vk::ShaderModule shaderModule, vk::ShaderStageFlagBits shaderStage) {
    vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{};
    shaderStageCreateInfo.stage = shaderStage;
    shaderStageCreateInfo.module = shaderModule;
    shaderStageCreateInfo.pName = "main";
    shaderStageCreateInfos.emplace_back(shaderStageCreateInfo);
}

void pipeline_builder::set_input_topology(vk::PrimitiveTopology topology) {
    inputAssemblyStateCreateInfo.topology = topology;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
}

void pipeline_builder::set_vertex_input(const vk::VertexInputBindingDescription& bind,
                                        vk::ArrayProxy<const vk::VertexInputAttributeDescription> attr) {
    vertexInputStateCreateInfo.setVertexBindingDescriptions(bind);
    vertexInputStateCreateInfo.setVertexAttributeDescriptions(attr);
}

void pipeline_builder::set_polygon_mode(vk::PolygonMode mode) {
    rasterizationStateCreateInfo.polygonMode = mode;
    rasterizationStateCreateInfo.lineWidth = 1.f;
}

void pipeline_builder::set_cull_mode(vk::CullModeFlags cullMode, vk::FrontFace frontFace) {
    rasterizationStateCreateInfo.cullMode = cullMode;
    rasterizationStateCreateInfo.frontFace = frontFace;
}

void pipeline_builder::disable_multisampling() {
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampleStateCreateInfo.minSampleShading = 1.0f;
    multisampleStateCreateInfo.pSampleMask = nullptr;
    multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
}

void pipeline_builder::disable_blending() {
    colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                               vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachmentState.blendEnable = VK_FALSE;
}

void pipeline_builder::enable_blending_additive() {
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

void pipeline_builder::enable_blending_alpha_blend() {
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

void pipeline_builder::set_color_attachment_format(vk::Format format) {
    colorAttachmentFormat = format;
}

void pipeline_builder::set_depth_attachment_format(vk::Format format) {
    depthAttachmentFormat = format;
}

void pipeline_builder::disable_depth_test() {
    depthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eNever;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
}

void pipeline_builder::enable_depth_test(bool writeEnable, vk::CompareOp compareOp) {
    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable = writeEnable ? VK_TRUE : VK_FALSE;
    depthStencilStateCreateInfo.depthCompareOp = compareOp;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
}

} // namespace broken::vulkan
