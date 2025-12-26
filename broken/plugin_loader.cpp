#include "plugin_loader.hpp"

namespace broken {

Plugin* PluginLoader::load(const std::string& pluginName, const std::string& pluginCreateFunctionName) {
    Plugin* ptr = nullptr;
    // Look up the registry first
    if (auto it = m_registry.find(pluginName); it != m_registry.end()) {
        ptr = it->second.second.get();
    } else {
        // Load the dynamic library and create the plugin interface
        auto lib = std::make_unique<DynamicLibrary>(pluginName);
        if (auto pluginCreateFunction = lib->getSymbol<PluginCreateFunction>(pluginCreateFunctionName)) {
            if (auto plugin = std::unique_ptr<Plugin>(pluginCreateFunction())) {
                ptr = plugin.get();
                // Make a record to the registry
                m_registry[pluginName] = {std::move(lib), std::move(plugin)};
            }
        }
    }

    return ptr;
}

} // namespace broken
