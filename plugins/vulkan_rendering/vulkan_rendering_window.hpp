#pragma once

#include <broken/rendering_plugin.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace broken {

class VulkanRenderingWindow final : public RenderingWindow {
public:
    explicit VulkanRenderingWindow(int width, int height, const std::string& title, bool resizable, bool fullscreen);

    VulkanRenderingWindow(const VulkanRenderingWindow&) = delete;
    VulkanRenderingWindow& operator=(const VulkanRenderingWindow&) = delete;

    ~VulkanRenderingWindow() noexcept override;

    VkSurfaceKHR createSurface(VkInstance instance) const noexcept;

    std::vector<const char*> getRequiredInstanceExtensions() const noexcept;

    virtual int getWidth() const override;

    virtual int getHeight() const override;

    virtual bool processEvents() override;

private:
    GLFWwindow* m_window = nullptr;
};

} // namespace broken
