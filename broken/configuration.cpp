#include "configuration.hpp"

#include <fstream>

namespace broken {

Configuration::Configuration(const std::string& configurationPath) : m_configurationPath(configurationPath) {
    if (!load(m_configurationPath)) {
        std::cerr << "failed to load configuration: " << m_configurationPath << std::endl;

        if (!save()) {
            std::cerr << "failed to save configuration: " << m_configurationPath << std::endl;
        }
    }
}

Configuration::~Configuration() noexcept {
    if (!save(m_configurationPath)) {
        std::cerr << "failed to save configuration: " << m_configurationPath << std::endl;
    }
}

bool Configuration::load(const std::string& configurationPath) {
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

bool Configuration::save(const std::string& configurationPath) {
    std::fstream file(configurationPath, std::ios::binary | std::ios::out);
    if (file.is_open()) {
        file << m_json.dump(4);
        return true;
    }

    return false;
}

std::ostream& operator<<(std::ostream& os, const Configuration& c) {
    os << c.m_configurationPath << ":\n" << c.m_json.dump(4);
    return os;
}

} // namespace broken
