#pragma once
#include <concepts>

struct GLFWwindow;

namespace broken {
struct scene;
}

namespace broken::rhi {

template <typename T>
concept renderer_concept = requires(T& renderer, GLFWwindow* window, const scene& scene) {
    T{window};
    { renderer.draw(scene) } -> std::same_as<void>;
};

} // namespace broken::rhi
