#pragma once

#ifdef _WIN64
#define BROKEN_PLUGIN_EXPORT __declspec(dllexport)
#else
#define BROKEN_PLUGIN_EXPORT
#endif

#define broken_plugin(PluginInterface, PluginName)                                                                     \
    extern "C" BROKEN_PLUGIN_EXPORT PluginInterface* make_##PluginName##() {                                           \
        return new PluginInterface();                                                                                  \
    }                                                                                                                  \
    extern "C" BROKEN_PLUGIN_EXPORT void release_##PluginName##(PluginInterface * plugin) {                            \
        delete plugin;                                                                                                 \
    }

#include <memory>
#include <string>
#include <unordered_map>

namespace broken {

class plugin {
public:
    virtual ~plugin() noexcept = default;

    virtual const char* name() const = 0;

    virtual const unsigned int version() const = 0;
};

using make_plugin_function = plugin* (*)();
using release_plugin_function = void (*)(plugin*);

class plugin_dynamic_library final {
public:
    explicit plugin_dynamic_library(const std::string& fileName);

    plugin_dynamic_library(const plugin_dynamic_library&) = delete;
    plugin_dynamic_library& operator=(const plugin_dynamic_library&) = delete;

    ~plugin_dynamic_library() noexcept;

    void* symbol(const std::string& symbolName) const;

    template <typename T>
    inline T symbol(const std::string& symbolName) const {
        return static_cast<T>(symbol(symbolName));
    }

    inline bool loaded() const noexcept { return m_handle != nullptr; }

private:
    void* m_handle = nullptr;
};

class plugin_loader final {
public:
    template <std::derived_from<plugin> T>
    inline T* get(const std::string& pluginName) const {
        return static_cast<T*>(get(pluginName));
    }

    template <std::derived_from<plugin> T>
    inline T* load(const std::string& pluginPath,
                   const std::string& pluginCreateFunctionName,
                   const std::string& pluginDestroyFunctionName) {
        return static_cast<T*>(load(pluginPath, pluginCreateFunctionName, pluginDestroyFunctionName));
    }

private:
    using unique_library = std::unique_ptr<plugin_dynamic_library>;
    using library_registry = std::unordered_map<std::string, unique_library>;
    library_registry m_libraries;

    using unique_plugin = std::unique_ptr<plugin, release_plugin_function>;
    using plugin_registry = std::unordered_map<std::string, unique_plugin>;
    plugin_registry m_plugins;

    plugin* get(const std::string& pluginName) const;

    plugin* load(const std::string& pluginPath,
                 const std::string& pluginCreateFunctionName,
                 const std::string& pluginDestroyFunctionName);
};

} // namespace broken
