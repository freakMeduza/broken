#include "app.hpp"
#if defined(BROKEN_USE_VULKAN)
#include "vk_renderer.hpp"
#endif

int main() {
    try {
#if defined(BROKEN_USE_VULKAN)
        return broken::run<broken::vulkan::renderer>();
    } catch (const vk::SystemError& ex) {
        broken_crit("vk::SystemError: {}", ex.what());
#endif
    } catch (const std::exception& ex) {
        broken_crit("std::exception: {}", ex.what());
    }
    return EXIT_FAILURE;
}
