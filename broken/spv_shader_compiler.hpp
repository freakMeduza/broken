#pragma once

#include <string>
#include <vector>

namespace broken {

class spv_shader_compiler {
public:
    spv_shader_compiler();

    spv_shader_compiler(const spv_shader_compiler&) = delete;
    spv_shader_compiler& operator=(const spv_shader_compiler&) = delete;

    ~spv_shader_compiler();

    [[nodiscard]] std::vector<uint32_t> glsl_to_spv_vulkan(const std::string& path) const;
};

} // namespace broken
