#include "app.hpp"
#include "vk_renderer.hpp"

int main() {
    try {
        return broken::run();
    } catch (const vk::SystemError& ex) {
        broken_crit("vk::SystemError: {}", ex.what());
    } catch (const std::exception& ex) {
        broken_crit("std::exception: {}", ex.what());
    }
    return EXIT_FAILURE;
}
