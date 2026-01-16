#include "plugin.hpp"

#include <filesystem>

#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#error "Platfrom is not supported!"
#endif

namespace broken {

plugin_dynamic_library::plugin_dynamic_library(const std::string& fileName) {
    m_handle = LoadLibrary(fileName.c_str());
}

plugin_dynamic_library::~plugin_dynamic_library() noexcept {
    FreeLibrary((HMODULE)m_handle);
}

void* plugin_dynamic_library::symbol(const std::string& symbolName) const {
    return m_handle ? GetProcAddress((HMODULE)m_handle, symbolName.c_str()) : nullptr;
}

plugin* plugin_loader::get(const std::string& pluginName) const {
    if (auto it = m_plugins.find(pluginName); it != m_plugins.end()) {
        return it->second.get();
    }

    return nullptr;
}

plugin* plugin_loader::load(const std::string& pluginPath,
                            const std::string& pluginCreateFunctionName,
                            const std::string& pluginDestroyFunctionName) {
    // a.k.a. cmake target
    const auto pluginName = std::filesystem::path(pluginPath).stem().string();

    plugin_dynamic_library* library = nullptr;
    // Look up the library registry
    if (auto it = m_libraries.find(pluginName); it != m_libraries.end()) {
        library = it->second.get();
    } else {
        const auto& [iterator, inserted] =
            m_libraries.emplace(pluginName, unique_library(new plugin_dynamic_library(pluginPath)));
        library = iterator->second.get();
    }

    if (!library || !library->loaded()) {
        return nullptr;
    }

    plugin* plugin = nullptr;
    // Look up the plugin registry
    if (auto it = m_plugins.find(pluginName); it != m_plugins.end()) {
        plugin = it->second.get();
    } else {
        // Load the dynamic library and create the plugin interface
        auto lib = std::make_shared<plugin_dynamic_library>(pluginPath);
        // Load make_plugin_function and release_plugin_function symbols from dll
        auto pluginCreateFunction = lib->symbol<make_plugin_function>(pluginCreateFunctionName);
        auto pluginDestroyFunction = lib->symbol<release_plugin_function>(pluginDestroyFunctionName);

        if (pluginCreateFunction && pluginDestroyFunction) {
            const auto& [iterator, inserted] =
                m_plugins.emplace(pluginName, unique_plugin(pluginCreateFunction(), pluginDestroyFunction));
            plugin = iterator->second.get();
        }
    }

    return plugin;
}

} // namespace broken
