#pragma once
#include "broken.hpp"
#include "log.hpp"
#include "math.hpp"

#include <filesystem>
#include <vector>

namespace broken {

struct mesh {
    struct vertex {
        glm::vec3 position;
        float uv_x;
        glm::vec3 normal;
        float uv_y;
        glm::vec4 color;
    };

    using index = uint16_t;

    std::vector<vertex> vertices;
    std::vector<index> indices;
};

struct scene_object {
    broken::mesh mesh;
    glm::mat4 transform;
};

struct scene {
    struct {
        glm::mat4 projMatrix;
        glm::mat4 viewMatrix;
    } camera;
    glm::vec3 sunDirection;
    std::vector<broken::scene_object> objects;

    void load_glTF(const std::filesystem::path& path);
};

} // namespace broken
