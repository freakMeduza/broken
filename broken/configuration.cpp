#include "configuration.hpp"

#include <fstream>

namespace broken {

bool configuration::load(const std::string& configurationPath) {
    std::fstream file(configurationPath, std::ios::binary | std::ios::in);
    if (file.is_open()) {
        try {
            m_json = nlohmann::ordered_json::parse(file);
            return true;
        } catch (nlohmann::ordered_json::exception& ex) {
            std::cerr << ex.what() << std::endl;
        }
    }

    return false;
}

bool configuration::save(const std::string& configurationPath) {
    std::fstream file(configurationPath, std::ios::binary | std::ios::out);
    if (file.is_open()) {
        file << m_json.dump(4);
        return true;
    }

    return false;
}

std::ostream& operator<<(std::ostream& os, const configuration& c) {
    os << c.m_json.dump(4);
    return os;
}

} // namespace broken
