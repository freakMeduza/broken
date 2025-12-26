#include "rendering_command_list.hpp"

namespace broken {

void RenderingCommandList::add(const RenderingCommandEntry& command) {
    m_commands.push_back(command);
}

void RenderingCommandList::add(RenderingCommand command) {
    add({command, {}});
}

void RenderingCommandList::execute(RenderingDevice* renderingDevice) {
    for (const auto [commandType, commandData] : m_commands) {
        switch (commandType) {
        case RenderingCommand::eBeginRendering: {
            renderingDevice->beginRendering();
            break;
        }
        case RenderingCommand::eEndRendering: {
            renderingDevice->endRendering();
            break;
        }
        case RenderingCommand::eClear: {
            if (commandData) {
                renderingDevice->clear(std::get<RenderingColor>(*commandData));
            }
            break;
        }
        case RenderingCommand::eBindPipeline: {
            if (commandData) {
                renderingDevice->bind(std::get<RenderingPipeline*>(*commandData));
            }
            break;
        }
        case RenderingCommand::eDispatchCompute: {
            renderingDevice->dispatch();
            break;
        }
        }
    }
}

} // namespace broken
