// =============================================================================
//  Vigil — Day 9
//  Phase 2: Animation state machine + 0.2s crossfading.
//  W = walk, Shift+W = jog, LMB = attack combo (chains in window),
//  Space = roll. RMB drag orbits camera, scroll zooms.
//  WASD camera pan retired — WASD now feeds the player SM exclusively.
// =============================================================================

#include "core/Camera.h"
#include "core/Window.h"
#include "core/VulkanContext.h"
#include "game/PlayerState.h"
#include "render/Swapchain.h"
#include "render/GraphicsPipeline.h"
#include "render/Renderer.h"

#include <SDL3/SDL.h>

#include <glm/glm.hpp>

#include <chrono>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <string>

int main(int /*argc*/, char* /*argv*/[]) {
    try {
        std::printf("================ Vigil — Day 9 ================\n");

        vigil::Window window(1440, 900, "Vigil");
        vigil::VulkanContext vk(window);
        vigil::Swapchain swapchain(vk, window);

        const char* base = SDL_GetBasePath();
        const std::filesystem::path shader_dir =
            std::filesystem::path(base ? base : "./") / "shaders";

        const std::string vert_path = (shader_dir / "triangle.vert.spv").string();
        const std::string frag_path = (shader_dir / "triangle.frag.spv").string();

        std::printf("[Vigil] Shader dir: %s\n", shader_dir.string().c_str());

        vigil::GraphicsPipeline pipeline(vk, swapchain, vert_path, frag_path);
        vigil::Renderer    renderer(vk, swapchain, pipeline);
        vigil::Camera      camera;
        vigil::PlayerState player;

        std::printf("[Vigil] W = walk, Shift+W = jog, LMB = attack combo, Space = roll.\n");
        std::printf("[Vigil] Hold RMB + drag to orbit. Scroll to zoom. Esc to quit.\n");

        auto last_time = std::chrono::high_resolution_clock::now();

        while (!window.should_close()) {
            window.poll_events();

            const auto now = std::chrono::high_resolution_clock::now();
            const float dt = std::chrono::duration<float>(now - last_time).count();
            last_time = now;

            const auto input = window.consume_input();

            // Camera: RMB orbit + scroll zoom only. WASD pan retired.
            if (input.rmb_held && (input.mouse_dx != 0.0f || input.mouse_dy != 0.0f)) {
                camera.orbit(input.mouse_dx, input.mouse_dy);
            }
            if (input.scroll_y != 0.0f) {
                camera.zoom(input.scroll_y);
            }

            // Player state machine — frame data per GDD §6.
            vigil::PlayerInput pin;
            pin.move_forward = input.w_held;
            pin.sprint       = input.shift_held;
            pin.attack       = input.lmb_pressed;
            pin.dodge        = input.space_pressed;
            player.update(dt, pin);

            renderer.draw_frame(camera, player);
        }

        renderer.wait_idle();
        std::printf("[Vigil] Shutting down cleanly.\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[Vigil FATAL] %s\n", e.what());
        return 1;
    }
    return 0;
}
