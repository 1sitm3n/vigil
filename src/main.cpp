// =============================================================================
//  Vigil — Day 10
//  Phase 2 checkpoint: ImGui debug overlay (state/phase/state_time/anim/FPS/
//  i-frames dot) + attack-anim audit + 60-second retrospective clip.
//
//  W = walk, Shift+W = jog, LMB = attack combo (chains in window),
//  Space = roll. RMB drag orbits camera, scroll zooms. Esc quits.
// =============================================================================

#include "core/Camera.h"
#include "core/Window.h"
#include "core/VulkanContext.h"
#include "game/PlayerState.h"
#include "render/Swapchain.h"
#include "render/GraphicsPipeline.h"
#include "render/Renderer.h"

#include <SDL3/SDL.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <glm/glm.hpp>

#include <chrono>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <string>

int main(int /*argc*/, char* /*argv*/[]) {
    try {
        std::printf("================ Vigil — Day 10 ================\n");

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
        vigil::Renderer    renderer(vk, swapchain, pipeline, window);
        vigil::Camera      camera;
        vigil::PlayerState player;

        std::printf("[Vigil] W = walk, Shift+W = jog, LMB = attack combo, Space = roll.\n");
        std::printf("[Vigil] Hold RMB + drag to orbit. Scroll to zoom. Esc to quit.\n");
        std::printf("[Vigil] Debug overlay (ImGui) top-left.\n");

        auto last_time = std::chrono::high_resolution_clock::now();

        while (!window.should_close()) {
            // ImGui gets first dibs on every SDL event so its IO state stays
            // in sync (mouse, keyboard, text input). Window's own switch
            // consumes events for game logic afterwards.
            window.poll_events([](const SDL_Event& e) {
                ImGui_ImplSDL3_ProcessEvent(&e);
            });

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

            // ImGui frame: NewFrame -> build UI -> Render. RenderDrawData
            // happens inside renderer.draw_frame's command-buffer recording.
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
            renderer.draw_debug_overlay(player, dt);
            ImGui::Render();

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
