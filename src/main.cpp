// =============================================================================
//  Vigil — Day 14
//  Phase 3 continues: parry + riposte. RMB edge press opens a 0.15s parry
//  window; held RMB enters Block. N fires a fake incoming attack 0.5s later.
//  When the fake hit lands: parry window open -> [PARRY] -> Riposte state;
//  in Block -> [BLOCKED] + 50% stamina drain; else -> [DAMAGED].
//
//  WASD = camera-relative move. Shift = sprint. LMB = light combo.
//  Shift+LMB = heavy. Space = roll. RMB = block / tap to parry. Q = (Day 15).
//  N = debug fake incoming attack. Esc quits.
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

namespace {
// Day 14 fake-hit tunables. Phase 4 enemies will replace this with the
// real damage event emitted by enemy AI. Damage = 20 is a midpoint between
// Cultist Slash (8) and Lunge (12) — small enough that the 50% block drain
// is a tap (10 stamina, ~12% of bar), big enough that ignoring the parry
// would matter once HP exists.
constexpr float kFakeHitDelay  = 0.5f;
constexpr float kFakeHitDamage = 20.0f;
}  // namespace

int main(int /*argc*/, char* /*argv*/[]) {
    try {
        std::printf("================ Vigil — Day 14 ================\n");

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

        std::printf("[Vigil] WASD = move, Shift = sprint, Space = roll.\n");
        std::printf("[Vigil] LMB = light combo, Shift+LMB = heavy.\n");
        std::printf("[Vigil] RMB hold = Block. RMB tap (within 0.15s of hit) = Parry -> Riposte.\n");
        std::printf("[Vigil] N = fake incoming attack in 0.5s (debug). Esc to quit.\n");

        auto last_time = std::chrono::high_resolution_clock::now();

        // Day 14: pending fake-hit timer. -1 = inactive. Set on N press to
        // kFakeHitDelay, ticks down each frame, resolves when <=0.
        float fake_hit_timer = -1.0f;

        while (!window.should_close()) {
            window.poll_events([](const SDL_Event& e) {
                ImGui_ImplSDL3_ProcessEvent(&e);
            });

            const auto now = std::chrono::high_resolution_clock::now();
            const float dt = std::chrono::duration<float>(now - last_time).count();
            last_time = now;

            const auto input = window.consume_input();

            // Camera follow + mouse-look.
            camera.set_target(player.position() + glm::vec3(0.0f, 0.5f, 0.0f));
            if (input.mouse_dx != 0.0f || input.mouse_dy != 0.0f) {
                camera.orbit(input.mouse_dx, input.mouse_dy);
            }
            if (input.scroll_y != 0.0f) {
                camera.zoom(input.scroll_y);
            }

            // ----- Day 14 parry pipeline -----
            // Order is load-bearing: tick window FIRST so a fresh RMB-press
            // this frame catches an N-pressed-last-frame fake hit firing
            // this frame. Then resolve incoming damage against the updated
            // window. Then player.update() runs the SM with riposte_pending_
            // already set from try_parry. No 1-frame latency anywhere.
            player.tick_parry_window(dt, input);

            // Schedule fake incoming hit. Late-overwrite is OK (latest N
            // wins) — matches "I changed my mind, swing again."
            if (input.n_pressed) {
                fake_hit_timer = kFakeHitDelay;
                std::printf("[FAKE-HIT] scheduled in %.2fs (dmg=%.0f)\n",
                            kFakeHitDelay, kFakeHitDamage);
            }

            // Tick + resolve. Fire when crossing zero this frame.
            if (fake_hit_timer > 0.0f) {
                fake_hit_timer -= dt;
                if (fake_hit_timer <= 0.0f) {
                    fake_hit_timer = -1.0f;
                    const bool parried = player.try_parry();
                    if (parried) {
                        std::printf("[PARRY] window=%.3fs dmg=%.0f -> Riposte\n",
                                    vigil::Player::PARRY_WINDOW, kFakeHitDamage);
                    } else if (player.state().id() == vigil::PlayerStateId::Block) {
                        const float drain = kFakeHitDamage * 0.5f;
                        player.absorb_block_hit(kFakeHitDamage);
                        std::printf("[BLOCKED] dmg=%.0f stam_drain=%.0f\n",
                                    kFakeHitDamage, drain);
                    } else if (player.state().iframes_active()) {
                        std::printf("[IFRAMES] absorbed dmg=%.0f (Roll active)\n",
                                    kFakeHitDamage);
                    } else {
                        std::printf("[DAMAGED] dmg=%.0f (HP system in Phase 4)\n",
                                    kFakeHitDamage);
                    }
                }
            }
            // ----- end Day 14 parry pipeline -----

            // SM + locomotion. Consumes riposte_pending_ into pin.riposte
            // and transitions to Riposte if applicable.
            player.update(dt, input, camera);

            // Day 12 — outgoing hit detection against the practice dummy.
            // Now fires for Light/Heavy/Riposte (all share AttackPhase::Active).
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
