#include "command_line.hpp"

namespace broken {

void CommandLine::parse(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    for (size_t i = 0; i < args.size(); ++i) {
        std::string arg = args[i];
        if (arg.starts_with("--")) {
            std::string flag = arg.substr(2);
            if (m_options.contains(flag)) {
                std::string next = (i + 1 < args.size()) ? args[++i] : std::string{};
                if (next.empty() || next.starts_with("--")) {
                    m_options[flag]("true");
                } else {
                    m_options[flag](next);
                }
            }
        }
    }
}

} // namespace broken
