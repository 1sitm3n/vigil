// =============================================================================
//  Vigil — Day 11
//  Phase 3 opens: Player class owns position, facing, velocity, and the SM.
//  Third-person follow camera at offset (5.5m, pitch 25°) around the player's
//  head-height pivot. Mouse-look is unconditional (no RMB gate); cursor is
//  captured for the session via SDL_SetWindowRelativeMouseMode.
//
//  WASD = camera-relative locomotion (any direction). Shift = sprint (intent
//  tracked, drain lands Day 13). LMB = attack combo. Space = roll. Esc quits.
//  Scroll wheel zooms in/out for camera feel-tuning.
// =============================================================================

#include "core/Camera.h"
#include "core/Window.h"
#include "core/VulkanContext.h"
#include "game/Player.h"
#include "render/Swapchain.h"
#include "render/GraphicsPipeline.h"
#include "render/Renderer.h"

#include <SDL3/SDL.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <glm/glm.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <string>

int main(int /*argc*/, char* /*argv*/[]) {
    try {
        std::printf("================ Vigil — Day 11 ================\n");

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
        vigil::Renderer renderer(vk, swapchain, pipeline, window);
        vigil::Camera   camera;
        vigil::Player   player;

        std::printf("[Vigil] WASD = move (camera-relative), Shift = sprint.\n");
        std::printf("[Vigil] LMB = attack combo, Space = roll.\n");
        std::printf("[Vigil] Mouse-look is always on. Scroll wheel = zoom. Esc to quit.\n");

        auto last_time = std::chrono::high_resolution_clock::now();

        while (!window.should_close()) {
            // ImGui gets first dibs on every SDL event so its IO state stays
            // in sync. Window's own switch consumes events for game logic
            // afterwards.
            window.poll_events([](const SDL_Event& e) {
                ImGui_ImplSDL3_ProcessEvent(&e);
            });

            const auto now = std::chrono::high_resolution_clock::now();
            const float dt = std::chrono::duration<float>(now - last_time).count();
            last_time = now;

            const auto input = window.consume_input();

            // Camera follow + mouse-look. Order: (a) re-pin pivot to the
            // player's head BEFORE consuming mouse delta — keeps yaw/pitch
            // updates relative to the current pivot, not lagged-by-one-frame.
            // (b) Mouse-look unconditional now (no RMB gate). (c) Scroll
            // zoom kept as camera-distance feel-tuner.
            camera.set_target(player.position() + glm::vec3(0.0f, 0.5f, 0.0f));
            if (input.mouse_dx != 0.0f || input.mouse_dy != 0.0f) {
                camera.orbit(input.mouse_dx, input.mouse_dy);
            }
            if (input.scroll_y != 0.0f) {
                camera.zoom(input.scroll_y);
            }

            // Player owns the SM now. update() drives transitions, locomotion,
            // Roll motion, and facing-slerp using the camera's basis.
            player.update(dt, input, camera);

            // Day 12 — Phase 3 hit detection against the single practice dummy.
            // GDD §6 attack arc: 90° cone (45° half-angle), 2.0m reach, XZ only.
            // One-shot per attack instance via PlayerState::attack_landed_ (reset
            // on transition_to entry to Attack1/2/3). 0.10s Active * 60Hz = 6
            // frames in the hot path so the flag is load-bearing for correctness.
            //
            // Cone direction: +Z-forward (Player.cpp Day 12 — the Day 11
            // hypothesis is now confirmed, model rest pose faces +Z, not -Z).
            // facing = (sin(yaw), 0, cos(yaw)) matches Player.cpp's atan2 path.
            {
                const auto& st = player.state();
                const bool active = (st.attack_phase() == vigil::AttackPhase::Active);
                if (active && !st.attack_landed()) {
                    const glm::vec3 d_xz(
                        vigil::Renderer::DUMMY_POSITION.x - player.position().x,
                        0.0f,
                        vigil::Renderer::DUMMY_POSITION.z - player.position().z);
                    const float dist = glm::length(d_xz);
                    if (dist > 1.0e-4f && dist <= 2.0f) {
                        const float yaw = player.yaw();
                        const glm::vec3 facing(std::sin(yaw), 0.0f, std::cos(yaw));
                        const glm::vec3 dir = d_xz / dist;
                        const float cosang = glm::clamp(glm::dot(facing, dir), -1.0f, 1.0f);
                        const float ang    = std::acos(cosang);
                        if (ang <= glm::radians(45.0f)) {
                            std::printf("[HIT] %s  dist=%.2fm  angle=%.1fdeg\n",
                                        vigil::to_string(st.id()), dist,
                                        glm::degrees(ang));
                            player.register_hit();
                        }
                    }
                }
            }

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
