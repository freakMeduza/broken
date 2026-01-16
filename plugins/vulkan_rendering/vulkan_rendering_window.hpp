#pragma once

#include <broken/rendering_plugin.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace broken {

class vulkan_renderiing_window final : public rendering_window {
public:
    explicit vulkan_renderiing_window(int width, int height, const char* title, bool resizable, bool fullscreen);

    vulkan_renderiing_window(const vulkan_renderiing_window&) = delete;
    vulkan_renderiing_window& operator=(const vulkan_renderiing_window&) = delete;

    ~vulkan_renderiing_window() noexcept override;

    virtual int width() const override;

    virtual int height() const override;

    virtual bool process_events() override;

    virtual rendering_device* device() const override;

    VkSurfaceKHR make_vulkan_window_surface(VkInstance instance) const noexcept;

    std::vector<const char*> required_vulkan_instance_extensions() const noexcept;

private:
    GLFWwindow* m_window = nullptr;

    std::unique_ptr<class vulkan_rendering_device> m_device;
};

} // namespace broken
