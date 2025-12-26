#pragma once

#include "dynamic_library.hpp"
#include "plugin.hpp"

#include <memory>
#include <unordered_map>

namespace broken {

class PluginLoader final {
public:
    template <std::derived_from<Plugin> T>
    inline T* load(const std::string& pluginName, const std::string& pluginCreateFunctionName) {
        return reinterpret_cast<T*>(load(pluginName, pluginCreateFunctionName));
    }

private:
    using PluginRegistry =
        std::unordered_map<std::string, std::pair<std::unique_ptr<DynamicLibrary>, std::unique_ptr<Plugin>>>;
    PluginRegistry m_registry;

    Plugin* load(const std::string& pluginName, const std::string& pluginCreateFunctionName);
};

} // namespace broken
