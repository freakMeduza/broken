#pragma once

#include <broken/rendering_plugin.hpp>

#include <vulkan/vulkan.hpp>

namespace broken {

class VulkanRenderingWindow;
class VulkanRenderingDevice final : public RenderingDevice {
public:
    explicit VulkanRenderingDevice(const VulkanRenderingWindow* window, bool enableValidationLayers);

    VulkanRenderingDevice(const VulkanRenderingDevice&) = delete;
    VulkanRenderingDevice& operator=(const VulkanRenderingDevice&) = delete;

    ~VulkanRenderingDevice() noexcept override = default;

    inline vk::Device getLogicalDevice() const noexcept { return m_logicalDevice.get(); }

private:
    virtual void beginRendering() override;

    virtual void endRendering() override;

    virtual void clear(const RenderingColor& color) override;

    virtual void bind(RenderingPipeline* pipeline) override;

    virtual void dispatch() override;

    virtual void waitIdle() override;

    virtual std::unique_ptr<RenderingPipeline> createRenderingPipeline(const std::string& path) const override;

    vk::UniqueInstance m_instance;
    vk::UniqueDebugUtilsMessengerEXT m_debugMessenger;
    uint32_t m_graphicsQueueFamilyIndex = vk::QueueFamilyIgnored;
    vk::PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_logicalDevice;
    vk::Queue m_graphicsQueue;
    vk::UniqueSurfaceKHR m_surface;
    vk::PresentModeKHR m_presentMode = vk::PresentModeKHR::eFifo;
    vk::Format m_imageFormat = vk::Format::eUndefined;
    vk::Extent2D m_imageExtent;
    vk::UniqueSwapchainKHR m_swapchain;
    std::vector<vk::Image> m_images;
    std::vector<vk::UniqueImageView> m_imageViews;
    vk::UniqueCommandPool m_commandPool;
    vk::UniqueCommandBuffer m_commandBuffer;
    std::vector<vk::UniqueSemaphore> m_renderFinishedSemaphores;
    vk::UniqueSemaphore m_imageAvailableSemaphore;
    vk::UniqueFence m_inFlightFence;
    uint32_t m_imageIndex = (uint32_t)-1;

    [[nodiscard]] static vk::PipelineStageFlags2KHR getStageFlagsForLayout(vk::ImageLayout layout);

    [[nodiscard]] static vk::AccessFlags2KHR getAccessFlagsForLayout(vk::ImageLayout layout);

    static void transitionImageLayout(vk::CommandBuffer commandBuffer,
                                      vk::Image image,
                                      vk::Format format,
                                      vk::ImageLayout oldLayout,
                                      vk::ImageLayout newLayout,
                                      uint32_t mipLevels = 1,
                                      uint32_t arrayLayers = 1);
};

} // namespace broken
