#pragma once
#include "rhi.hpp"
#include "scene.hpp"

#include "vk_device.hpp"

#include <unordered_map>

namespace broken::vulkan {

struct alignas(16) gpu_scene_data {
    glm::mat4 viewProj;
    glm::mat4 invViewProj;
    glm::mat4 model;
    glm::vec4 sunDirection;
    vk::DeviceAddress ssboAddress;
    uint32_t ssboOffset;
};

struct renderer {
    explicit renderer(GLFWwindow* window) noexcept;

    renderer(const renderer&) = delete;
    renderer& operator=(const renderer&) = delete;

    void draw(const broken::scene& scene);

    broken::vulkan::device device;

    vk::UniqueCommandPool commandPool;
    vk::CommandBuffer commandBuffer;
    vk::UniqueSemaphore imageAvailableSemaphore;
    vk::UniqueSemaphore renderFinishedSemaphore;
    uint32_t imageIndex = (uint32_t)-1;

    vk::UniqueDescriptorPool descriptorPool;
    vk::UniqueDescriptorSetLayout descriptorSetLayout;
    vk::UniqueDescriptorSet descriptorSet;
    vk::UniquePipelineLayout computePipelineLayout;
    vk::UniquePipeline computePipeline;
    vk::UniquePipelineLayout graphicsPipelineLayout;
    vk::UniquePipeline graphicsPipeline;

    const vk::DeviceSize ssboMaxSizeBytes = 256 * 1024 * 1024; // 256 Mb

    struct {
        vk::UniqueBuffer buffer;
        vk::UniqueDeviceMemory memory;
        vk::DeviceAddress address;
        vk::DeviceSize currentAllocatedBytes = 0;
    } ssbo;

    struct gpu_mesh_data {
        vk::DeviceAddress ssboAddress;
        uint32_t ssboOffset;
        vk::UniqueBuffer indexBuffer;
        vk::UniqueDeviceMemory indexBufferMemory;
        uint32_t indexCount;
    };

    mutable std::unordered_map<const broken::mesh*, gpu_mesh_data> meshCache;
    const gpu_mesh_data& get_or_create_gpu_mesh_data(const broken::mesh& cpuMesh) noexcept;
};

} // namespace broken::vulkan

static_assert(broken::rhi::renderer_concept<broken::vulkan::renderer>,
              "Type 'broken::vulkan::renderer' does not match 'broken::rhi::renderer_concept'!");
