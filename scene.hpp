#pragma once
#include "broken.hpp"
#include "log.hpp"
#include "math.hpp"

#include <filesystem>
#include <optional>
#include <vector>

#include <GLFW/glfw3.h>

namespace broken {

struct vertex {
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
};

struct mesh {
    using index = uint16_t;
    std::vector<vertex> vertices;
    std::vector<index> indices;
};

struct scene_object {
    broken::mesh mesh;
    glm::mat4 transform;
};

namespace fs = std::filesystem;

struct scene {
    std::vector<broken::scene_object> objects;

    static std::optional<scene> load_from_file(const fs::path& path);

    glm::vec3 sunDirection;
    glm::mat4 projMatrix;
    glm::mat4 viewMatrix;
};

} // namespace broken
