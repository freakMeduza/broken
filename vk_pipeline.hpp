#pragma once
#include "vk_device.hpp"

namespace broken::vulkan {

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

    void reset();

    vk::UniquePipeline build_graphics_pipeline(vk::Device logicalDevice, vk::PipelineLayout pipelineLayout) const;
    vk::UniquePipeline build_compute_pipeline(vk::Device logicalDevice, vk::PipelineLayout pipelineLayout) const;

    void add_shader(vk::ShaderModule shaderModule, vk::ShaderStageFlagBits shaderStage);
    void set_input_topology(vk::PrimitiveTopology topology);
    void set_vertex_input(const vk::VertexInputBindingDescription& bind,
                          vk::ArrayProxy<const vk::VertexInputAttributeDescription> attr);
    void set_polygon_mode(vk::PolygonMode mode);
    void set_cull_mode(vk::CullModeFlags cullMode, vk::FrontFace frontFace);
    void disable_multisampling();
    void disable_blending();
    void enable_blending_additive();
    void enable_blending_alpha_blend();
    void set_color_attachment_format(vk::Format format);
    void set_depth_attachment_format(vk::Format format);
    void disable_depth_test();
    void enable_depth_test(bool writeEnable = true, vk::CompareOp compareOp = vk::CompareOp::eLess);
};

} // namespace broken::vulkan
