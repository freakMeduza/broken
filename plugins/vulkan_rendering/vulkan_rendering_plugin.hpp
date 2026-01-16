#pragma once

#include <broken/rendering_plugin.hpp>

namespace broken {

class vulkan_rendering_plugin final : public rendering_plugin {
private:
    virtual const char* name() const override;

    virtual const unsigned int version() const override;

    virtual rendering_window*
    make_rendering_window(int width, int height, const char* title, bool resizable, bool fullscreen) const override;

    virtual void release_rendering_window(rendering_window* window) const override;
};

} // namespace broken
