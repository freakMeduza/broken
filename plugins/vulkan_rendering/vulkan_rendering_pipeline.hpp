#pragma once

#include <broken/rendering_plugin.hpp>

#include <vulkan/vulkan.hpp>

namespace broken {

class VulkanRenderingDevice;
class VulkanRenderingPipeline : public RenderingPipeline {
public:
    explicit VulkanRenderingPipeline(const VulkanRenderingDevice* device, const std::string& shaderFileName);

    VulkanRenderingPipeline(const VulkanRenderingPipeline&) = delete;
    VulkanRenderingPipeline& operator=(const VulkanRenderingPipeline&) = delete;

    ~VulkanRenderingPipeline() noexcept override = default;

    inline vk::Pipeline getPipeline() const noexcept { return m_pipeline.get(); }

    inline vk::PipelineLayout getPipelineLayout() const noexcept { return m_pipelineLayout.get(); }

    inline vk::DescriptorSet getDescriptorSet() const noexcept { return m_descriptorSet.get(); }

private:
    vk::UniqueShaderModule createShaderModule(vk::Device logicalDevice, const std::string& shaderPath) const;

    vk::UniqueDescriptorPool m_descriptorPool;
    vk::UniqueDescriptorSetLayout m_descriptorSetLayout;
    vk::UniqueDescriptorSet m_descriptorSet;
    vk::UniquePipelineLayout m_pipelineLayout;
    vk::UniquePipeline m_pipeline;
};

} // namespace broken
