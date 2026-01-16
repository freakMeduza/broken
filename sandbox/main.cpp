#include <sandbox.hpp>

#include <broken/configuration.hpp>
#include <broken/rendering_plugin.hpp>

namespace broken {

struct sandbox_rendering_window_configuration {
    int width = 800;
    int height = 800;
    bool resizable = false;
    bool fullscreen = false;
    std::string title = "broken";
    // FIXME: enable support 'resizable' and 'fullscreen' features in configuration file
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(sandbox_rendering_window_configuration,
                                                width,
                                                height /*, resizable, fullscreen*/,
                                                title)
};

struct sandbox_rendering_device_configuration {
    bool validation = false;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(sandbox_rendering_device_configuration, validation)
};

struct sandbox_rendering_compute_pipeline_configuration {
    std::string path;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(sandbox_rendering_compute_pipeline_configuration, path)
};

struct sandbox_rendering_plugin_configuration {
    std::string name;
    std::string path;
    sandbox_rendering_window_configuration window;
    sandbox_rendering_device_configuration device;
    std::vector<sandbox_rendering_compute_pipeline_configuration> pipelines;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        sandbox_rendering_plugin_configuration, name, path, window, device, pipelines)
};

} // namespace broken

#include <bitset>

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    broken::configuration configuration;
    if (!configuration.load(SANDBOX_CONFIGURATION_FILE)) {
        std::cerr << "failed to load configuration " << SANDBOX_CONFIGURATION_FILE << std::endl;
    }

    std::cout << configuration << std::endl;

    auto renderingPluginConfig = configuration.get<broken::sandbox_rendering_plugin_configuration>("rendering");

    broken::plugin_loader pluginLoader;
    auto* renderingPlugin = pluginLoader.load<broken::rendering_plugin>(
        renderingPluginConfig.path, "make_vulkan_rendering_plugin", "release_vulkan_rendering_plugin");
    if (!renderingPlugin) {
        std::cerr << "failed to load plugin " << renderingPluginConfig.path << std::endl;
        return EXIT_FAILURE;
    }

    auto renderingWindow = renderingPlugin->make_rendering_window_unique(renderingPluginConfig.window.width,
                                                                         renderingPluginConfig.window.height,
                                                                         renderingPluginConfig.window.title.c_str(),
                                                                         renderingPluginConfig.window.resizable,
                                                                         renderingPluginConfig.window.fullscreen);
    if (!renderingWindow) {
        std::cerr << "failed to create rendering window" << std::endl;
        return EXIT_FAILURE;
    }

    auto* renderingDevice = renderingWindow->device();
    if (!renderingDevice) {
        std::cerr << "failed to get rendering device" << std::endl;
        return EXIT_FAILURE;
    }

    auto pipeline = renderingDevice->make_compute_pipeline(
        !renderingPluginConfig.pipelines.empty() ? renderingPluginConfig.pipelines[0].path.c_str() : "");

    while (renderingWindow->process_events()) {
        renderingDevice->begin();

        renderingDevice->clear({
            .r = 0.1f,
            .g = 0.1f,
            .b = 0.1f,
            .a = 1.f,
        });

        if (pipeline.valid()) {
            renderingDevice->bind(pipeline);
            // TODO: groupCountX, groupCountY, groupCountZ
            renderingDevice->dispatch();
        }

        renderingDevice->end();
    }

    renderingDevice->release_compute_pipeline(pipeline);

    configuration.set("rendering", renderingPluginConfig);

    return EXIT_SUCCESS;
}