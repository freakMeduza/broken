#include "vulkan_rendering_window.hpp"

#include "vulkan_rendering_device.hpp"

#include <iostream>

namespace broken {

vulkan_renderiing_window::vulkan_renderiing_window(
    int width, int height, const char* title, bool resizable, bool fullscreen) {
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

    m_window = glfwCreateWindow(width, height, title, monitor, nullptr);

    m_device = std::make_unique<vulkan_rendering_device>(this, true);
}

vulkan_renderiing_window::~vulkan_renderiing_window() noexcept {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

int vulkan_renderiing_window::width() const {
    int w = 0, h = 0;
    glfwGetWindowSize(m_window, &w, &h);
    return w;
}

int vulkan_renderiing_window::height() const {
    int w = 0, h = 0;
    glfwGetWindowSize(m_window, &w, &h);
    return h;
}

bool vulkan_renderiing_window::process_events() {
    if (glfwWindowShouldClose(m_window)) {
        return false;
    }

    glfwPollEvents();

    return true;
}

rendering_device* vulkan_renderiing_window::device() const {
    return m_device.get();
}

VkSurfaceKHR vulkan_renderiing_window::make_vulkan_window_surface(VkInstance instance) const noexcept {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    glfwCreateWindowSurface(instance, m_window, nullptr, &surface);
    return surface;
}

std::vector<const char*> vulkan_renderiing_window::required_vulkan_instance_extensions() const noexcept {
    uint32_t instanceExtensionCount = 0;
    const auto** extensions = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
    std::vector<const char*> instanceExtensions(instanceExtensionCount);
    for (auto i = 0u; i < instanceExtensionCount; ++i) {
        instanceExtensions[i] = extensions[i];
    }

    return instanceExtensions;
}

} // namespace broken
