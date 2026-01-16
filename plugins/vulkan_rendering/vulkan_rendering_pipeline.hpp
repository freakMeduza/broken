#pragma once

#include <broken/rendering_plugin.hpp>

#include <vulkan/vulkan.hpp>

namespace broken {

struct vulkan_rendering_compute_pipeline {
    explicit vulkan_rendering_compute_pipeline(vk::Device logicalDevice, const char* shaderPath);

    vk::UniqueDescriptorPool descriptorPool;
    vk::UniqueDescriptorSetLayout descriptorSetLayout;
    vk::UniqueDescriptorSet descriptorSet;
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeline;
};

} // namespace broken
