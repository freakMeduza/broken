#pragma once

#include "plugin.hpp"

#include <nlohmann/json.hpp>

namespace broken {

class RenderingDevice;
class RenderingWindow {
public:
    virtual ~RenderingWindow() noexcept = default;

    [[nodiscard]] virtual int getWidth() const = 0;

    [[nodiscard]] virtual int getHeight() const = 0;

    [[nodiscard]] virtual bool processEvents() = 0;

    [[nodiscard]] virtual std::unique_ptr<RenderingDevice> createRenderingDevice(bool enableValidationLayers) const = 0;
};

class RenderingPipeline {
public:
    virtual ~RenderingPipeline() noexcept = default;
};

using RenderingColor = std::array<float, 4>;

class RenderingDevice {
public:
    virtual ~RenderingDevice() noexcept = default;

    virtual void beginRendering() = 0;

    virtual void endRendering() = 0;

    virtual void clear(const RenderingColor& color) = 0;

    virtual void bind(RenderingPipeline* pipeline) = 0;

    virtual void dispatch() = 0;

    virtual void waitIdle() = 0;

    [[nodiscard]] virtual std::unique_ptr<RenderingPipeline> createRenderingPipeline(const std::string& path) const = 0;
};

class RenderingPlugin : public Plugin {
public:
    [[nodiscard]] virtual std::unique_ptr<RenderingWindow>
    createRenderingWindow(int width, int height, const std::string& title, bool resizable, bool fullscreen) const = 0;
};

struct RenderingWindowConfig {
    int width = 800;
    int height = 800;
    bool resizable = false;
    bool fullscreen = false;
    std::string title = "broken";
    // FIXME: enable support 'resizable' and 'fullscreen' features in configuration file
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(RenderingWindowConfig, width, height /*, resizable, fullscreen*/, title)
};

struct RenderingDeviceConfig {
    bool validation = false;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(RenderingDeviceConfig, validation)
};

struct RenderingPipelineConfig {
    std::string path;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(RenderingPipelineConfig, path)
};

struct RenderingPluginConfig {
    std::string name;
    std::string path;
    RenderingWindowConfig window;
    RenderingDeviceConfig device;
    std::vector<RenderingPipelineConfig> pipelines;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(RenderingPluginConfig, name, path, window, device, pipelines)
};

} // namespace broken
