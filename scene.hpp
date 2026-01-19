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
    glm::vec3 normal;
    glm::vec3 color;
};

struct mesh {
    using index = uint16_t;
    std::vector<vertex> vertices;
    std::vector<index> indices;

    static mesh cube() noexcept {
        return {
            // Vertices
            {// Z+
             {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.8f, 0.8f, 0.8f}},
             {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.8f, 0.8f, 0.8f}},
             {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.8f, 0.8f, 0.8f}},
             {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.8f, 0.8f, 0.8f}},
             // Z-
             {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.8f, 0.8f, 0.8f}},
             {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.8f, 0.8f, 0.8f}},
             {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.8f, 0.8f, 0.8f}},
             {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.8f, 0.8f, 0.8f}},
             // Y+
             {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             // Y-
             {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             // X+
             {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             // X-
             {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
             {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.8f, 0.8f, 0.8f}}},
            // Indices
            {0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,  8,  9,  10, 10, 11, 8,
             12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20},
        };
    }
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
