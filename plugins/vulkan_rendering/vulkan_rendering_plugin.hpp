#pragma once

#include <broken/rendering_plugin.hpp>

namespace broken {

class VulkanRenderingPlugin : public RenderingPlugin {
private:
    virtual std::string getName() const override;

    virtual const unsigned int getVersion() const override;

    virtual std::unique_ptr<RenderingWindow> createRenderingWindow(
        int width, int height, const std::string& title, bool resizable, bool fullscreen) const override;
};

} // namespace broken
