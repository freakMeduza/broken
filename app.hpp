#pragma once
#include "broken.hpp"
#include "log.hpp"
#include "rhi.hpp"
#include "scene.hpp"

#include <GLFW/glfw3.h>

#include <string>

namespace broken {

glm::vec3 calculate_sun_direction(float time01) {
    float angle = time01 * 2.0f * glm::pi<float>();
    float latitude = 0.4f;

    glm::vec3 dir;
    dir.x = glm::cos(angle);
    dir.z = glm::sin(angle);
    dir.y = glm::sin(angle) * glm::cos(latitude) + glm::sin(latitude) * 0.2f;

    return glm::normalize(dir);
}

template <rhi::renderer_concept T>
int run() {
#if defined(_DEBUG)
    spdlog::set_level(spdlog::level::trace);
#endif

    glfwSetErrorCallback([](int error, const char* msg) { broken_error("{}: {}", error, msg); });
    if (glfwInit() != GLFW_TRUE) {
        broken_error("Failed to initialize GLFW!");
        return EXIT_FAILURE;
    }

#if defined(BROKEN_USE_VULKAN)
    if (glfwVulkanSupported() != GLFW_TRUE) {
        broken_error("Platform doesn't support Vulkan API!");
        return EXIT_FAILURE;
    }
#endif

    int width = 800;
    int height = 800;
    std::string title = std::string(BROKEN_PROJECT) + " " + BROKEN_VERSION + " " + BROKEN_PROJECT_SCENE;
    bool resizable = false;
    bool fullscreen = false;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
    GLFWmonitor* monitor = nullptr;
    if (fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        auto videoMode = glfwGetVideoMode(monitor);
        width = videoMode->width;
        height = videoMode->height;
    }

    GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
    if (!window) {
        broken_error("Failed to create GLFW window!");
        return EXIT_FAILURE;
    }

    glfwSetKeyCallback(
        window, [](GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods) {
            if (key == GLFW_KEY_ESCAPE) {
                if (action == GLFW_PRESS || action == GLFW_REPEAT) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
            }
        });

    broken::scene scene;
    scene.camera = {
        glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 1000.0f),
        glm::lookAt(glm::vec3(0.0f, 130.f, 400.f), glm::vec3(0.0f, 130.f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
    };

#if defined(BROKEN_USE_VULKAN)
    scene.camera.projMatrix[1][1] *= -1.0f;
#endif

    scene.load_glTF(BROKEN_PROJECT_SCENE);

    {
        T renderer{window};

        float lastFrame = static_cast<float>(glfwGetTime());
        static float angle = 0.0f;

        const float rotationSpeed = 0.5f;

        const glm::mat4 preTransform = glm::rotate(glm::mat4(1.f), glm::radians(90.f), {1.f, 0.f, 0.f});

        while (glfwWindowShouldClose(window) != GLFW_TRUE) {
            glfwPollEvents();

            float currentFrame = static_cast<float>(glfwGetTime());
            float deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            if (deltaTime > 0.1f)
                deltaTime = 0.1f;

            static float dayTimer = 0.25f;

            const float dayCycleLengthInSeconds = 60.0f;
            const float timeSpeed = 1.0f / dayCycleLengthInSeconds;

            dayTimer += deltaTime * timeSpeed;

            if (dayTimer > 1.0f) {
                dayTimer -= 1.0f;
            }

            float cycleTime = glm::mod((float)glfwGetTime() / 60.0f, 1.0f);
            scene.sunDirection = calculate_sun_direction(cycleTime);

            angle += rotationSpeed * deltaTime;
            for (auto& [cpuMesh, transform] : scene.objects)
                transform = glm::rotate(glm::mat4(1.f), -angle, {0.f, 1.f, 0.f}) * preTransform;

            renderer.draw(scene);
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}

} // namespace broken
