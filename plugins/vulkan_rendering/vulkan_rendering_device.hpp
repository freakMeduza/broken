#pragma once

#include <broken/rendering_plugin.hpp>

#include <vulkan/vulkan.hpp>

#include "vulkan_rendering_pipeline.hpp"

namespace broken {

class vulkan_renderiing_window;
class vulkan_rendering_device final : public rendering_device {
public:
    explicit vulkan_rendering_device(const vulkan_renderiing_window* window, bool enableValidationLayers);

    vulkan_rendering_device(const vulkan_rendering_device&) = delete;
    vulkan_rendering_device& operator=(const vulkan_rendering_device&) = delete;

    virtual ~vulkan_rendering_device() noexcept override;

    virtual void begin() override;

    virtual void end() override;

    virtual void clear(const rendering_color& color) override;

    virtual void bind(resource_handle<rendering_compute_pipeline> handle) override;

    virtual void dispatch() override;

    virtual resource_handle<rendering_compute_pipeline> make_compute_pipeline(const char* shaderPath) override;

    virtual void release_compute_pipeline(resource_handle<rendering_compute_pipeline> handle) override;

private:
    vk::UniqueInstance m_instance;
    vk::UniqueDebugUtilsMessengerEXT m_debugMessenger;
    uint32_t m_graphicsQueueFamilyIndex = vk::QueueFamilyIgnored;
    vk::PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_logicalDevice;
    vk::Queue m_graphicsQueue;
    vk::UniqueSurfaceKHR m_surface;
    vk::PresentModeKHR m_swapchainPresentMode = vk::PresentModeKHR::eFifo;
    vk::Format m_swapchainImageFormat = vk::Format::eUndefined;
    vk::Extent2D m_swapchainImageExtent;
    vk::UniqueSwapchainKHR m_swapchain;
    std::vector<vk::Image> m_swapchainImages;
    std::vector<vk::UniqueImageView> m_swapchainImageViews;

    vk::UniqueCommandPool m_commandPool;
    vk::UniqueCommandBuffer m_commandBuffer;
    std::vector<vk::UniqueSemaphore> m_renderFinishedSemaphores;
    vk::UniqueSemaphore m_imageAvailableSemaphore;
    vk::UniqueFence m_inFlightFence;
    uint32_t m_imageIndex = (uint32_t)-1;

    [[nodiscard]] static vk::PipelineStageFlags2KHR stage_flags_for_layout(vk::ImageLayout layout);

    [[nodiscard]] static vk::AccessFlags2KHR access_flags_for_layout(vk::ImageLayout layout);

    static void transition_image_layout(vk::CommandBuffer commandBuffer,
                                        vk::Image image,
                                        vk::Format format,
                                        vk::ImageLayout oldLayout,
                                        vk::ImageLayout newLayout,
                                        uint32_t mipLevels = 1,
                                        uint32_t arrayLayers = 1);

    resource_registry<rendering_compute_pipeline, vulkan_rendering_compute_pipeline> m_computePipelines{16};
};

} // namespace broken
