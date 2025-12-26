#pragma once

#ifdef _WIN64
#define BROKEN_PLUGIN_EXPORT __declspec(dllexport)
#else
#define BROKEN_PLUGIN_EXPORT
#endif

#define broken_plugin(PluginInterface, PluginName)                                                                     \
    extern "C" BROKEN_PLUGIN_EXPORT PluginInterface* create##PluginName##()

#include <string>

namespace broken {

class BROKEN_PLUGIN_EXPORT Plugin {
public:
    virtual ~Plugin() noexcept = default;

    virtual std::string getName() const = 0;

    virtual const unsigned int getVersion() const = 0;
};

using PluginCreateFunction = Plugin* (*)();

} // namespace broken
