#include "vulkan_rendering_pipeline.hpp"

#include "vulkan_rendering_device.hpp"

#include <glslang/Include/glslang_c_interface.h>
// Required for use of glslang_default_resource
#include <glslang/Public/resource_limits_c.h>

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

// [[maybe_unused]] static glslang_stage_t to_glslang_stage(ShaderStage stage)
// {
//     switch (stage)
//     {
//     case ShaderStage::Compute:
//         return GLSLANG_STAGE_COMPUTE;
//     case ShaderStage::Vertex:
//         return GLSLANG_STAGE_VERTEX;
//     case ShaderStage::Fragment:
//         return GLSLANG_STAGE_FRAGMENT;
//     }

//     return GLSLANG_STAGE_COUNT;
// }

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

    vk::UniqueShaderModule shaderModule = createShaderModule(device->getLogicalDevice(), shaderFileName);
    PipelineBuilder builder;
    builder.setShader(shaderModule.get(), vk::ShaderStageFlagBits::eCompute);

    m_pipeline = builder.buildComputePipeline(device->getLogicalDevice(), m_pipelineLayout.get());
}

vk::UniqueShaderModule VulkanRenderingPipeline::createShaderModule(vk::Device logicalDevice,
                                                                   const std::string& shaderPath) const {
    std::fstream fs(shaderPath, std::ios::in);
    std::string src((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());

    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = GLSLANG_STAGE_COMPUTE,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_4,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_5,
        .code = src.c_str(),
        .default_version = 100,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = glslang_default_resource(),
    };

    glslang_shader_t* shader = glslang_shader_create(&input);

    std::vector<uint32_t> binary;
    if (!glslang_shader_preprocess(shader, &input)) {
        std::stringstream ss;
        ss << shaderPath << "\n\t" << glslang_shader_get_info_log(shader) << "\n\t"
           << glslang_shader_get_info_debug_log(shader);
        std::cerr << ss.str() << std::endl;

        glslang_shader_delete(shader);

        return {};
    }

    if (!glslang_shader_parse(shader, &input)) {
        std::stringstream ss;
        ss << shaderPath << "\n\t" << glslang_shader_get_info_log(shader) << "\n\t"
           << glslang_shader_get_info_debug_log(shader) << "\n\t" << glslang_shader_get_preprocessed_code(shader);
        std::cerr << ss.str() << std::endl;

        glslang_shader_delete(shader);

        return {};
    }

    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {

        std::stringstream ss;
        ss << shaderPath << "\n\t" << glslang_shader_get_info_log(shader) << "\n\t"
           << glslang_shader_get_info_debug_log(shader);
        std::cerr << ss.str() << std::endl;

        glslang_program_delete(program);
        glslang_shader_delete(shader);

        return {};
    }

    glslang_program_SPIRV_generate(program, GLSLANG_STAGE_COMPUTE); // to_glslang_stage(stage));

    binary.resize(glslang_program_SPIRV_get_size(program));
    glslang_program_SPIRV_get(program, binary.data());

    const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
    if (spirv_messages) {
        std::cout << shaderPath << "\n\t" << spirv_messages << std::endl;
    }

    glslang_program_delete(program);
    glslang_shader_delete(shader);

    vk::ShaderModuleCreateInfo shaderModuleCreateInfo;
    shaderModuleCreateInfo.setCode(binary);
    return logicalDevice.createShaderModuleUnique(shaderModuleCreateInfo);
}

} // namespace broken
