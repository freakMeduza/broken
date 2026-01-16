#pragma once

#include "plugin.hpp"
#include "resource.hpp"

namespace broken {

// Resource Tags
struct rendering_compute_pipeline {};

struct rendering_color {
    float r;
    float g;
    float b;
    float a;
};

class rendering_device {
public:
    virtual ~rendering_device() noexcept = default;

    virtual void begin() = 0;

    virtual void end() = 0;

    virtual void clear(const rendering_color& color) = 0;

    virtual void bind(resource_handle<rendering_compute_pipeline> handle) = 0;

    virtual void dispatch() = 0;

    [[nodiscard]] virtual resource_handle<rendering_compute_pipeline> make_compute_pipeline(const char* shaderPath) = 0;

    virtual void release_compute_pipeline(resource_handle<rendering_compute_pipeline> handle) = 0;
};

class rendering_window {
public:
    virtual ~rendering_window() noexcept = default;

    [[nodiscard]] virtual int width() const = 0;

    [[nodiscard]] virtual int height() const = 0;

    [[nodiscard]] virtual bool process_events() = 0;

    [[nodiscard]] virtual rendering_device* device() const = 0;
};

class rendering_plugin : public plugin {
public:
    [[nodiscard]] inline auto
    make_rendering_window_unique(int width, int height, const char* title, bool resizable, bool fullscreen) const {
        using deleter =
            plugin_resource_deleter<rendering_plugin, rendering_window, &rendering_plugin::release_rendering_window>;

        return std::unique_ptr<rendering_window, deleter>(
            make_rendering_window(width, height, title, resizable, fullscreen), deleter(this));
    }

protected:
    [[nodiscard]] virtual rendering_window*
    make_rendering_window(int width, int height, const char* title, bool resizable, bool fullscreen) const = 0;

    virtual void release_rendering_window(rendering_window* window) const = 0;
};

} // namespace broken
