#include "vulkan_rendering_pipeline.hpp"

#include "vulkan_rendering_device.hpp"
#include <broken/spv_shader_compiler.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

namespace broken {

struct PipelineBuilder {
public:
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

    vk::UniquePipeline buildGraphicsPipeline(vk::Device logicalDevice, vk::PipelineLayout pipelineLayout) const {
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

    vk::UniquePipeline buildComputePipeline(vk::Device logicalDevice, vk::PipelineLayout pipelineLayout) const {
        vk::ComputePipelineCreateInfo computePipelineCreateInfo;
        computePipelineCreateInfo.layout = pipelineLayout;
        computePipelineCreateInfo.setStage(!shaderStageCreateInfos.empty() ? shaderStageCreateInfos[0]
                                                                           : vk::PipelineShaderStageCreateInfo{});

        auto [result, pipeline] = logicalDevice.createComputePipelineUnique(nullptr, computePipelineCreateInfo);
        return std::move(pipeline);
    }

    void setShader(vk::ShaderModule shaderModule, vk::ShaderStageFlagBits shaderStage) {
        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{};
        shaderStageCreateInfo.stage = shaderStage;
        shaderStageCreateInfo.module = shaderModule;
        shaderStageCreateInfo.pName = "main";
        shaderStageCreateInfos.emplace_back(shaderStageCreateInfo);
    }

    void setInputTopology(vk::PrimitiveTopology topology) {
        inputAssemblyStateCreateInfo.topology = topology;
        inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
    }

    void setPolygonMode(vk::PolygonMode mode) {
        rasterizationStateCreateInfo.polygonMode = mode;
        rasterizationStateCreateInfo.lineWidth = 1.f;
    }

    void setCullMode(vk::CullModeFlags cullMode, vk::FrontFace frontFace) {
        rasterizationStateCreateInfo.cullMode = cullMode;
        rasterizationStateCreateInfo.frontFace = frontFace;
    }

    void disableMultisampling() {
        multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        // multisampling defaulted to no multisampling (1 sample per pixel)
        multisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
        multisampleStateCreateInfo.minSampleShading = 1.0f;
        multisampleStateCreateInfo.pSampleMask = nullptr;
        // no alpha to coverage either
        multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
    }

    void disableBlending() {
        // default write mask
        colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                   vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        // no blending
        colorBlendAttachmentState.blendEnable = VK_FALSE;
    }

    void enableBlendingAdditive() {
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

    void enableBlendingAlphaBlend() {
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

    void setColorAttachmentFormat(vk::Format format) {
        colorAttachmentFormat = format;
        // connect the format to the renderInfo  structure
        renderingCreateInfo.colorAttachmentCount = 1;
        renderingCreateInfo.pColorAttachmentFormats = &colorAttachmentFormat;
    }

    void setDepthAttachmentFormat(vk::Format format) { renderingCreateInfo.depthAttachmentFormat = format; }

    void disableDepthTest() {
        depthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
        depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eNever;
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.minDepthBounds = 0.f;
        depthStencilStateCreateInfo.maxDepthBounds = 1.f;
    }

    void enableDepthTest(bool depthWriteEnable, vk::CompareOp op) {
        depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
        depthStencilStateCreateInfo.depthCompareOp = op;
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.minDepthBounds = 0.f;
        depthStencilStateCreateInfo.maxDepthBounds = 1.f;
    }
};

VulkanRenderingPipeline::VulkanRenderingPipeline(const VulkanRenderingDevice* device,
                                                 const std::string& shaderFileName) {
    vk::DescriptorPoolSize descriptorPoolSize;
    descriptorPoolSize.setType(vk::DescriptorType::eStorageImage);
    descriptorPoolSize.setDescriptorCount(1);

    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    descriptorPoolCreateInfo.setMaxSets(10);
    descriptorPoolCreateInfo.setPoolSizes({descriptorPoolSize});
    m_descriptorPool = device->getLogicalDevice().createDescriptorPoolUnique(descriptorPoolCreateInfo);

    vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding;
    descriptorSetLayoutBinding.setBinding(0);
    descriptorSetLayoutBinding.setDescriptorCount(1);
    descriptorSetLayoutBinding.setDescriptorType(vk::DescriptorType::eStorageImage);
    descriptorSetLayoutBinding.setStageFlags(vk::ShaderStageFlagBits::eCompute);

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.setBindings({descriptorSetLayoutBinding});
    m_descriptorSetLayout = device->getLogicalDevice().createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo);

    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.setDescriptorPool(m_descriptorPool.get());
    descriptorSetAllocateInfo.setDescriptorSetCount(1);
    descriptorSetAllocateInfo.setSetLayouts({m_descriptorSetLayout.get()});
    m_descriptorSet = std::move(device->getLogicalDevice().allocateDescriptorSetsUnique(descriptorSetAllocateInfo)[0]);

    vk::PushConstantRange pushConstantRange;
    pushConstantRange.setStageFlags(vk::ShaderStageFlagBits::eCompute);
    pushConstantRange.setSize(sizeof(float));
    pushConstantRange.setOffset(0);

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.setSetLayouts({m_descriptorSetLayout.get()});
    pipelineLayoutCreateInfo.setPushConstantRanges({pushConstantRange});

    m_pipelineLayout = device->getLogicalDevice().createPipelineLayoutUnique(pipelineLayoutCreateInfo);

    SpvShaderCompiler compiler;
    auto shaderCode = compiler.glslToSpvVulkan(shaderFileName);
    vk::ShaderModuleCreateInfo shaderModuleCreateInfo;
    shaderModuleCreateInfo.setCode(shaderCode);
    vk::UniqueShaderModule shaderModule = device->getLogicalDevice().createShaderModuleUnique(shaderModuleCreateInfo);

    PipelineBuilder builder;
    builder.setShader(shaderModule.get(), vk::ShaderStageFlagBits::eCompute);

    m_pipeline = builder.buildComputePipeline(device->getLogicalDevice(), m_pipelineLayout.get());
}

} // namespace broken
