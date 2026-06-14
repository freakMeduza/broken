#pragma once
#include <vulkan/vulkan.hpp>

#include <functional>

struct GLFWwindow;

namespace broken::vulkan {

class device {
public:
    explicit device(GLFWwindow* window) noexcept;

    device(const device&) = delete;
    device& operator=(const device&) = delete;

    uint32_t find_memory_type(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const noexcept;

    void create_buffer(vk::DeviceSize size,
                       vk::BufferUsageFlags usage,
                       vk::MemoryPropertyFlags properties,
                       vk::UniqueBuffer& buffer,
                       vk::UniqueDeviceMemory& memory) const noexcept;

    void create_image(vk::Extent3D extent,
                      vk::Format format,
                      vk::ImageUsageFlags usage,
                      vk::MemoryPropertyFlags properties,
                      vk::UniqueImage& image,
                      vk::UniqueDeviceMemory& memory,
                      vk::ImageTiling tiling = vk::ImageTiling::eOptimal,
                      vk::ImageType imageType = vk::ImageType::e2D,
                      uint32_t mipLevels = 1,
                      uint32_t arrayLayers = 1,
                      vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1) const noexcept;

    void copy_image_to_image(vk::CommandBuffer cmd,
                             vk::Image src,
                             vk::Image dst,
                             vk::Extent2D srcSize,
                             vk::Extent2D dstSize) const noexcept;

    void immediate_submit(std::function<void(vk::CommandBuffer)>&& function);

    vk::UniqueInstance instance;
    vk::PhysicalDevice physicalDevice;
    vk::UniqueDevice logicalDevice;
    uint32_t graphicsQueueFamilyIndex = (uint32_t)-1;
    vk::Queue graphicsQueue;
    vk::UniqueSurfaceKHR surface;

    vk::UniqueSwapchainKHR swapchain;
    vk::Format swapchainFormat = vk::Format::eB8G8R8A8Unorm;
    vk::Extent2D swapchainExtent;
    std::vector<vk::Image> swapchainImages;
    std::vector<vk::UniqueImageView> swapchainImageViews;

    vk::UniqueImage depthImage;
    vk::UniqueDeviceMemory depthImageMemory;
    vk::UniqueImageView depthImageView;
    vk::Format depthFormat = vk::Format::eD32Sfloat;
    vk::Extent2D depthExtent;

    vk::UniqueImage colorImage;
    vk::UniqueDeviceMemory colorImageMemory;
    vk::UniqueImageView colorImageView;
    vk::Format colorFormat = vk::Format::eR16G16B16A16Sfloat;
    vk::Extent2D colorExtent;

    vk::UniqueCommandPool commandPool;
    vk::CommandBuffer commandBuffer;

    void recreate_swapchain(const vk::Extent2D& extent, vk::UniqueSwapchainKHR oldSwapchain);

private:
#if defined(_DEBUG)
    vk::UniqueDebugUtilsMessengerEXT debugMessenger;
#endif

    void create_color_buffer(const vk::Extent2D& extent);
    void create_depth_buffer(const vk::Extent2D& extent);

    vk::PipelineStageFlags2KHR stage_flags_for_layout(vk::ImageLayout layout) const noexcept;
    vk::AccessFlags2KHR access_flags_for_layout(vk::ImageLayout layout) const noexcept;

public:
    void transition_image_layout(vk::CommandBuffer cmd,
                                 vk::Image image,
                                 vk::Format format,
                                 vk::ImageLayout oldLayout,
                                 vk::ImageLayout newLayout,
                                 uint32_t mipLevels = 1,
                                 uint32_t arrayLayers = 1) const noexcept;
};

} // namespace broken::vulkan
