#pragma once

#include <string>
#include <vector>

namespace broken {

class SpvShaderCompiler {
public:
    SpvShaderCompiler();

    SpvShaderCompiler(const SpvShaderCompiler&) = delete;
    SpvShaderCompiler& operator=(const SpvShaderCompiler&) = delete;

    ~SpvShaderCompiler();

    [[nodiscard]] std::vector<uint32_t> glslToSpvVulkan(const std::string& path) const;
};

} // namespace broken
