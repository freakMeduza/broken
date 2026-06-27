#pragma once
#include "scene.hpp"

#include "vk_device.hpp"

#include <unordered_map>

namespace broken::vulkan {

struct scene_uniform_buffer {
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    glm::vec4 sunDirection;
};

struct scene_constants {
    glm::mat4 modelMatrix;
    vk::DeviceAddress vertexSSBODeviceAddress;
    uint32_t vertexSSBOOffset;
    vk::DeviceAddress indexSSBODeviceAddress;
    uint32_t indexSSBOOffset;
};

static_assert(sizeof(scene_constants) <= 128,
              "scene_constants size exceeds the standard 128-byte Vulkan push constant limit");

struct compute_constants {
    glm::mat4 data1;
    glm::vec4 data2;
    glm::vec4 data3;
    glm::vec4 data4;
    glm::vec4 data5;
};

static_assert(sizeof(compute_constants) <= 128,
              "compute_constants size exceeds the standard 128-byte Vulkan push constant limit");

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

    vk::UniqueImage depthImage;
    vk::UniqueDeviceMemory depthImageMemory;
    vk::UniqueImageView depthImageView;
    vk::Format depthFormat;
    vk::Extent2D depthExtent;

    vk::UniqueImage colorImage;
    vk::UniqueDeviceMemory colorImageMemory;
    vk::UniqueImageView colorImageView;
    vk::Format colorFormat;
    vk::Extent2D colorExtent;

    vk::UniqueBuffer vertexSSBO;
    vk::UniqueDeviceMemory vertexSSBOMemory;
    vk::DeviceAddress vertexSSBODeviceAddress;
    vk::DeviceSize vertexSSBOSize;
    vk::DeviceSize vertexSSBOCapacity;

    vk::UniqueBuffer indexSSBO;
    vk::UniqueDeviceMemory indexSSBOMemory;
    vk::DeviceAddress indexSSBODeviceAddress;
    vk::DeviceSize indexSSBOSize;
    vk::DeviceSize indexSSBOCapacity;

    vk::UniqueBuffer sceneUBO;
    vk::UniqueDeviceMemory sceneUBOMemory;
    vk::DeviceAddress sceneUBODeviceAddress;
    vk::DeviceSize sceneUBOSize;
    vk::DeviceSize sceneUBOCapacity;

    vk::UniqueDescriptorPool descriptorPool;
    vk::UniqueDescriptorSetLayout descriptorSetLayout;
    vk::UniqueDescriptorSet descriptorSet;
    vk::UniquePipelineLayout computePipelineLayout;
    vk::UniquePipeline computePipeline;
    vk::UniquePipelineLayout graphicsPipelineLayout;
    vk::UniquePipeline graphicsPipeline;

    struct gpu_mesh_data {
        vk::DeviceAddress vertexSSBODeviceAddress;
        vk::DeviceAddress indexSSBODeviceAddress;
        uint32_t vertexSSBOOffset;
        uint32_t indexSSBOOffset;
        uint32_t indexCount;
    };

    mutable std::unordered_map<const broken::mesh*, gpu_mesh_data> meshCache;
    const gpu_mesh_data& get_or_create_gpu_mesh_data(const broken::mesh& cpuMesh) noexcept;
};

} // namespace broken::vulkan
