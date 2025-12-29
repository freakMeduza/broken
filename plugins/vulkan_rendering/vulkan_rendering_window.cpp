#include "vulkan_rendering_window.hpp"

#include "vulkan_rendering_device.hpp"

#include <iostream>

namespace broken {

VulkanRenderingWindow::VulkanRenderingWindow(
    int width, int height, const std::string& title, bool resizable, bool fullscreen) {
    glfwSetErrorCallback([](int error, const char* errorMessage) {
        std::cerr << "GLFW: " << error << ": " << errorMessage << std::endl;
    });

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWmonitor* monitor = nullptr;
    if (fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        if (const auto* videoMode = glfwGetVideoMode(monitor)) {
            width = videoMode->width;
            height = videoMode->height;
        }
    }

    glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);

    m_window = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
}

VulkanRenderingWindow::~VulkanRenderingWindow() noexcept {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

VkSurfaceKHR VulkanRenderingWindow::createSurface(VkInstance instance) const noexcept {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    glfwCreateWindowSurface(instance, m_window, nullptr, &surface);
    return surface;
}

std::vector<const char*> VulkanRenderingWindow::getRequiredInstanceExtensions() const noexcept {
    uint32_t instanceExtensionCount = 0;
    const auto** extensions = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
    std::vector<const char*> instanceExtensions(instanceExtensionCount);
    for (auto i = 0u; i < instanceExtensionCount; ++i) {
        instanceExtensions[i] = extensions[i];
    }

    return instanceExtensions;
}

int VulkanRenderingWindow::getWidth() const {
    int w = 0, h = 0;
    glfwGetWindowSize(m_window, &w, &h);
    return w;
}

int VulkanRenderingWindow::getHeight() const {
    int w = 0, h = 0;
    glfwGetWindowSize(m_window, &w, &h);
    return h;
}

bool VulkanRenderingWindow::processEvents() {
    if (glfwWindowShouldClose(m_window)) {
        return false;
    }

    glfwPollEvents();

    return true;
}

std::unique_ptr<RenderingDevice> VulkanRenderingWindow::createRenderingDevice(bool enableValidationLayers) const {
    return std::make_unique<VulkanRenderingDevice>(this, enableValidationLayers);
}

} // namespace broken
