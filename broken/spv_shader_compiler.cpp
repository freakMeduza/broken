#include "spv_shader_compiler.hpp"

#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace broken {

static std::vector<uint32_t> glslToSpv(glslang::TShader& shader, EShMessages messages) {
    std::vector<uint32_t> spirv;

    const TBuiltInResource* resources = GetDefaultResources();

    if (!shader.parse(resources, 450, false, messages)) {
        std::cerr << shader.getInfoLog() << std::endl;
        return spirv;
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        std::cerr << program.getInfoLog() << std::endl;
        return spirv;
    }

    glslang::SpvOptions spvOptions;
    spvOptions.validate = true;
    glslang::GlslangToSpv(*program.getIntermediate(shader.getStage()), spirv, &spvOptions);
    return spirv;
}

static std::vector<uint32_t> glslToVulkan(const std::string& source, EShLanguage stage) {
    const char* str = source.c_str();

    glslang::TShader shader(stage);
    shader.setStrings(&str, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 460);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_4);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

    // // OpenGl 4.5/4.6 example
    // // ARB_gl_spirv
    // shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientOpenGL, 450);
    // shader.setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
    // shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    return glslToSpv(shader, (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules));
}

SpvShaderCompiler::SpvShaderCompiler() {
    glslang::InitializeProcess();
}

SpvShaderCompiler::~SpvShaderCompiler() {
    glslang::FinalizeProcess();
}

std::vector<uint32_t> SpvShaderCompiler::glslToSpvVulkan(const std::string& path) const {
    std::fstream file(path, std::ios::binary | std::ios::in);
    if (!file.is_open()) {
        std::cerr << "failed to open shader file " << path << " for reading" << std::endl;
        return {};
    }

    EShLanguage stage = EShLangCount;
    const auto ext = std::filesystem::path(path).extension();
    if (ext == ".comp") {
        stage = EShLangCompute;
    } else if (ext == ".vert") {
        stage = EShLangVertex;
    } else if (ext == ".frag") {
        stage = EShLangFragment;
    }

    if (stage == EShLangCount) {
        std::cerr << "failed to get shader stage from name " << path << std::endl;
        return {};
    }

    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    return glslToVulkan(source, stage);
}

} // namespace broken
