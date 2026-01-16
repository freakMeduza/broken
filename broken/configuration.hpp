#pragma once

#include <nlohmann/json.hpp>

#include <iostream>

namespace broken {

class configuration {
public:
    bool load(const std::string& configurationPath);

    bool save(const std::string& configurationPath);

    template <typename T>
    [[nodiscard]] inline T get(const std::string& jsonKey) {
        if (m_json.contains(jsonKey)) {
            try {
                return m_json[jsonKey].get<T>();
            } catch (nlohmann::ordered_json::exception& ex) {
                std::cerr << ex.what() << std::endl;
            }
        }

        m_json[jsonKey] = T();
        return m_json[jsonKey];
    }

    template <typename T>
    inline void set(const std::string& jsonKey, T&& json) {
        m_json[jsonKey] = json;
    }

    friend std::ostream& operator<<(std::ostream& os, const configuration& c);

private:
    nlohmann::ordered_json m_json;
};

} // namespace broken