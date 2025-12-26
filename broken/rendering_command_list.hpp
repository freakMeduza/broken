#pragma once

#include "rendering_plugin.hpp"

#include <optional>
#include <variant>

namespace broken {

enum class RenderingCommand {
    eBeginRendering,
    eEndRendering,
    eClear,
    eBindPipeline,
    eDispatchCompute,
};

using RenderingCommandData = std::variant<RenderingColor, RenderingPipeline*>;
using RenderingCommandEntry = std::pair<RenderingCommand, std::optional<RenderingCommandData>>;

class RenderingCommandList final {
public:
    void add(const RenderingCommandEntry& command);

    void add(RenderingCommand command);

    void execute(RenderingDevice* renderingDevice);

private:
    std::vector<RenderingCommandEntry> m_commands;
};

} // namespace broken
