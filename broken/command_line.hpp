#pragma once

#include <nlohmann/json.hpp>

#include <iostream>

namespace broken {

class CommandLine {
    using from_string = std::function<void(const std::string&)>;
    std::unordered_map<std::string, from_string> m_options;

public:
    template <typename T> inline void add(const std::string& flag, T& option) {
        m_options[flag] = [&option](const std::string& value) {
            if constexpr (std::is_same_v<T, bool>) {
                option = (value == "true");
            } else if constexpr (std::is_integral_v<T>) {
                option = std::stoi(value);
            } else if constexpr (std::is_floating_point_v<T>) {
                option = std::stof(value);
            } else {
                option = value;
            }
        };
    }

    void parse(int argc, char** argv);
};

} // namespace broken
