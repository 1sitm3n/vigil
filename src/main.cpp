// =============================================================================
//  Vigil — Day 3
//  Window, Vulkan device, swapchain, render pass, pipeline, command buffers.
//  Draws a hardcoded triangle in Vigil's palette every frame at 60Hz.
//  Real geometry, textures, depth come Day 4+.
// =============================================================================

#include "core/Window.h"
#include "core/VulkanContext.h"
#include "render/Swapchain.h"
#include "render/GraphicsPipeline.h"
#include "render/Renderer.h"

#include <SDL3/SDL.h>

#include <cstdio>
#include <exception>
#include <filesystem>
#include <string>

int main(int /*argc*/, char* /*argv*/[]) {
    try {
        std::printf("================ Vigil — Day 3 ================\n");

        vigil::Window window(1440, 900, "Vigil");
        vigil::VulkanContext vk(window);
        vigil::Swapchain swapchain(vk, window);

        // Resolve shader paths next to the executable.
        // SDL_GetBasePath returns a stable path owned by SDL — do not free.
        const char* base = SDL_GetBasePath();
        const std::filesystem::path shader_dir =
            std::filesystem::path(base ? base : "./") / "shaders";

        const std::string vert_path = (shader_dir / "triangle.vert.spv").string();
        const std::string frag_path = (shader_dir / "triangle.frag.spv").string();

        std::printf("[Vigil] Shader dir: %s\n", shader_dir.string().c_str());

        vigil::GraphicsPipeline pipeline(vk, swapchain, vert_path, frag_path);
        vigil::Renderer renderer(vk, swapchain, pipeline);

        std::printf("[Vigil] Initialisation complete. Drawing first triangle.\n");
        std::printf("[Vigil] Close window or press Esc to quit.\n");

        while (!window.should_close()) {
            window.poll_events();
            renderer.draw_frame();
        }

        renderer.wait_idle();
        std::printf("[Vigil] Shutting down cleanly.\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[Vigil FATAL] %s\n", e.what());
        return 1;
    }
    return 0;
}
