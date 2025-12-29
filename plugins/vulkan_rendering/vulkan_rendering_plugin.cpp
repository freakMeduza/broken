#include "vulkan_rendering_plugin.hpp"

#include "vulkan_rendering_window.hpp"

namespace broken {

std::string VulkanRenderingPlugin::getName() const {
    return "vulkan_rendering";
}

const unsigned int VulkanRenderingPlugin::getVersion() const {
    return 0;
}

std::unique_ptr<RenderingWindow> VulkanRenderingPlugin::createRenderingWindow(
    int width, int height, const std::string& title, bool resizable, bool fullscreen) const {
    return std::make_unique<VulkanRenderingWindow>(width, height, title, resizable, fullscreen);
}

} // namespace broken

broken_plugin(broken::VulkanRenderingPlugin, VulkanRenderingPlugin) {
    return new broken::VulkanRenderingPlugin();
}
