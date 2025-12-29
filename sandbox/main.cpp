#include <sandbox.hpp>

#include <broken/configuration.hpp>
#include <broken/plugin_loader.hpp>
#include <broken/rendering_command_list.hpp>

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    broken::Configuration configuration(SANDBOX_CONFIGURATION_FILE);
    std::cout << configuration << std::endl;

    auto renderingPluginConfig = configuration.get<broken::RenderingPluginConfig>("rendering");

    broken::PluginLoader pluginLoader;
    auto* renderingPlugin =
        pluginLoader.load<broken::RenderingPlugin>(renderingPluginConfig.path, "createVulkanRenderingPlugin");
    if (!renderingPlugin) {
        std::cerr << "failed to load plugin " << renderingPluginConfig.path << std::endl;
        return EXIT_FAILURE;
    }

    auto renderingWindow = renderingPlugin->createRenderingWindow(renderingPluginConfig.window.width,
                                                                  renderingPluginConfig.window.height,
                                                                  renderingPluginConfig.window.title,
                                                                  renderingPluginConfig.window.resizable,
                                                                  renderingPluginConfig.window.fullscreen);
    if (!renderingWindow) {
        std::cerr << "failed to create rendering window" << std::endl;
        return EXIT_FAILURE;
    }

    auto renderingDevice = renderingWindow->createRenderingDevice(renderingPluginConfig.device.validation);
    if (!renderingDevice) {
        std::cerr << "failed to create rendering device" << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<std::unique_ptr<broken::RenderingPipeline>> pipelines;

    for (const auto& pipeline : renderingPluginConfig.pipelines) {
        if (!std::filesystem::exists(pipeline.path)) {
            std::cerr << pipeline.path << " is not exist" << std::endl;
            continue;
        }

        pipelines.push_back(renderingDevice->createRenderingPipeline(pipeline.path));
    }

    broken::RenderingCommandList renderingCommandList;
    renderingCommandList.add({broken::RenderingCommand::eBeginRendering});
    renderingCommandList.add({broken::RenderingCommand::eClear, broken::RenderingColor{0.1f, 0.1f, 0.1f, 1.f}});

    for (const auto& pipeline : pipelines) {
        renderingCommandList.add({broken::RenderingCommand::eBindPipeline, pipeline.get()});
        renderingCommandList.add({broken::RenderingCommand::eDispatchCompute});
    }

    renderingCommandList.add({broken::RenderingCommand::eEndRendering});

    while (renderingWindow->processEvents()) {
        renderingCommandList.execute(renderingDevice.get());
    }

    renderingDevice->waitIdle();

    configuration.set("rendering", renderingPluginConfig);

    return EXIT_SUCCESS;
}