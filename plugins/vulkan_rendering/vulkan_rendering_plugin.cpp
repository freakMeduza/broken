#include "vulkan_rendering_plugin.hpp"

#include "vulkan_rendering_window.hpp"

namespace broken {

const char* vulkan_rendering_plugin::name() const {
    return "vulkan_rendering";
}

const unsigned int vulkan_rendering_plugin::version() const {
    return VK_MAKE_VERSION(0, 0, 1);
}

rendering_window* vulkan_rendering_plugin::make_rendering_window(
    int width, int height, const char* title, bool resizable, bool fullscreen) const {
    return new vulkan_renderiing_window(width, height, title, resizable, fullscreen);
}

void vulkan_rendering_plugin::release_rendering_window(rendering_window* window) const {
    delete window;
}

} // namespace broken

broken_plugin(broken::vulkan_rendering_plugin, vulkan_rendering_plugin)
