#pragma once
#include "rhi.hpp"
#include "scene.hpp"
#include "vk_device.hpp"

#include <unordered_map>

namespace broken::vulkan {

struct gpu_scene_data {
    alignas(16) glm::mat4 viewProj;
    alignas(16) glm::mat4 invViewProj;
    alignas(16) glm::mat4 model;
    alignas(16) glm::vec4 sunDirection;
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

    struct gpu_mesh_data {
        vk::UniqueBuffer vertexBuffer;
        vk::UniqueDeviceMemory vertexBufferMemory;
        vk::UniqueBuffer indexBuffer;
        vk::UniqueDeviceMemory indexBufferMemory;
        uint32_t indexCount;
    };

    mutable std::unordered_map<const broken::mesh*, gpu_mesh_data> meshCache;
    const gpu_mesh_data& get_or_create_gpu_mesh_data(const broken::mesh& cpuMesh) const noexcept;
};

} // namespace broken::vulkan

static_assert(broken::rhi::renderer_concept<broken::vulkan::renderer>,
              "Type 'broken::vulkan::renderer' does not match 'broken::rhi::renderer_concept'!");
