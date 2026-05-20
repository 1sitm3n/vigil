// =============================================================================
//  Vigil — Day 2
//  SDL3 window opens, Vulkan instance + surface + device initialised,
//  main loop runs, clean shutdown on quit or Escape.
//  Still no rendering. That starts Day 3.
// =============================================================================

#include "core/Window.h"
#include "core/VulkanContext.h"

#include <cstdio>
#include <exception>

int main(int /*argc*/, char* /*argv*/[]) {
    try {
        std::printf("================ Vigil — Day 2 ================\n");

        vigil::Window window(1440, 900, "Vigil");
        vigil::VulkanContext vk(window);

        std::printf("[Vigil] Initialisation complete. Entering main loop.\n");
        std::printf("[Vigil] Close window or press Esc to quit.\n");

        while (!window.should_close()) {
            window.poll_events();
            // Day 3 will draw here.
        }

        std::printf("[Vigil] Shutting down cleanly.\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[Vigil FATAL] %s\n", e.what());
        return 1;
    }
    return 0;
}
