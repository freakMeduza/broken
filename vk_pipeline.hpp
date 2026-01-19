#pragma once
#include "vk_device.hpp"

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

struct pipeline_builder {
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfos;
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;

    vk::Format colorAttachmentFormat = vk::Format::eUndefined;
    vk::Format depthAttachmentFormat = vk::Format::eUndefined;

    void reset() {
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

    vk::UniquePipeline build_graphics_pipeline(vk::Device logicalDevice, vk::PipelineLayout pipelineLayout) const {
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

    vk::UniquePipeline build_compute_pipeline(vk::Device logicalDevice, vk::PipelineLayout pipelineLayout) const {
        vk::ComputePipelineCreateInfo computePipelineCreateInfo;
        computePipelineCreateInfo.layout = pipelineLayout;
        computePipelineCreateInfo.setStage(!shaderStageCreateInfos.empty() ? shaderStageCreateInfos[0]
                                                                           : vk::PipelineShaderStageCreateInfo{});

        return std::move(logicalDevice.createComputePipelineUnique(nullptr, computePipelineCreateInfo).value);
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

    void set_vertex_input(const vk::VertexInputBindingDescription& bind,
                          vk::ArrayProxy<const vk::VertexInputAttributeDescription> attr) {
        vertexInputStateCreateInfo.setVertexBindingDescriptions(bind);
        vertexInputStateCreateInfo.setVertexAttributeDescriptions(attr);
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
        multisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
        multisampleStateCreateInfo.minSampleShading = 1.0f;
        multisampleStateCreateInfo.pSampleMask = nullptr;
        multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
    }

    void disable_blending() {
        colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                   vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
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
    }

    void set_depth_attachment_format(vk::Format format) {
        depthAttachmentFormat = format;
    }

    void disable_depth_test() {
        depthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
        depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eNever;
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.minDepthBounds = 0.0f;
        depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
    }

    void enable_depth_test(bool writeEnable = true, vk::CompareOp compareOp = vk::CompareOp::eLess) {
        depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthWriteEnable = writeEnable ? VK_TRUE : VK_FALSE;
        depthStencilStateCreateInfo.depthCompareOp = compareOp;
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.minDepthBounds = 0.0f;
        depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
    }
};

} // namespace broken::vulkan
