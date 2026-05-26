// =============================================================================
//  Vigil — Day 15  (Phase 3 closer)
//
//  Holy Bolt spell + Faith resource. Q triggers a 0.8s cast (lock-in-place,
//  no roll-cancel, interruptible by damage). On Cast::Active, spawn one
//  projectile at 18 m/s toward the dummy with weak 30 degree cone homing.
//  Projectile pool lives here; renderer.draw_frame takes it as a param.
//  N during Cast = [CAST-CANCEL] with 50% Faith refund. Phase 4 enemies
//  will replace the fake-hit dispatcher with real damage events.
//
//  WASD = camera-relative move. Shift = sprint. LMB = light combo.
//  Shift+LMB = heavy. Space = roll. RMB = block / tap to parry.
//  Q = Holy Bolt cast. N = debug fake incoming attack. Esc quits.
// =============================================================================

#include "core/Camera.h"
#include "core/Window.h"
#include "core/VulkanContext.h"
#include "game/Faith.h"
#include "game/Player.h"
#include "game/Projectile.h"
#include "render/Swapchain.h"
#include "render/GraphicsPipeline.h"
#include "render/Renderer.h"

#include <SDL3/SDL.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <string>
#include <vector>

namespace {
// Day 14 fake-hit tunables. Phase 4 enemies will replace this with the
// real damage event emitted by enemy AI. Damage = 20 is a midpoint between
// Cultist Slash (8) and Lunge (12) — small enough that the 50% block drain
// is a tap (10 stamina, ~12% of bar), big enough that ignoring the parry
// would matter once HP exists.
constexpr float kFakeHitDelay  = 0.5f;
constexpr float kFakeHitDamage = 20.0f;

// Day 15: bolt spawn offset - chest height above player origin. Knight's
// 1m bind-pose runs y from -0.499 to +0.499 (see [Mesh] bounds at startup);
// 0.25 is mid-chest. Grows to ~0.5 once Week-4 Blender re-upload puts the
// model at proper 1.8m scale. Bolts travel horizontally so this single Y
// value defines the firing line.
constexpr float kBoltSpawnY = 0.25f;
}  // namespace

int main(int /*argc*/, char* /*argv*/[]) {
    try {
        std::printf("================ Vigil — Day 15 ================\n");

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

        // Day 15: projectile pool. Bolts live here; renderer reads it; main
        // ticks + culls. One bolt per ~0.8s cast, lifetime 3s = max ~4
        // concurrent; vector-of-POD + remove_if cull is fine until Phase 4
        // enemies start firing back en masse.
        std::vector<vigil::Projectile> projectiles;

        std::printf("[Vigil] WASD = move, Shift = sprint, Space = roll.\n");
        std::printf("[Vigil] LMB = light combo, Shift+LMB = heavy.\n");
        std::printf("[Vigil] RMB hold = Block. RMB tap (within 0.15s of hit) = Parry -> Riposte.\n");
        std::printf("[Vigil] Q = Holy Bolt cast (20 Faith). N = fake incoming attack. Esc to quit.\n");

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
                    const float caught = player.try_parry();
                    if (caught > 0.0f) {
                        std::printf("[PARRY] caught_at=%.3fs dmg=%.0f -> Riposte\n",
                                    caught, kFakeHitDamage);
                    } else if (player.state().id() == vigil::PlayerStateId::Block) {
                        const float drain = kFakeHitDamage * 0.5f;
                        player.absorb_block_hit(kFakeHitDamage);
                        std::printf("[BLOCKED] dmg=%.0f stam_drain=%.0f\n",
                                    kFakeHitDamage, drain);
                    } else if (player.state().iframes_active()) {
                        std::printf("[IFRAMES] absorbed dmg=%.0f (Roll active)\n",
                                    kFakeHitDamage);
                    } else if (player.state().id() == vigil::PlayerStateId::Cast) {
                        // Day 15: incoming damage during Cast = cancel + 50%
                        // Faith refund (GDD section 6). Phase 4 enemies will
                        // replace this branch with the real damage event but
                        // the contract stays identical.
                        const float refund = vigil::Faith::BOLT_COST * 0.5f;
                        player.cancel_cast_with_refund();
                        std::printf("[CAST-CANCEL] dmg=%.0f faith_refund=%.0f\n",
                                    kFakeHitDamage, refund);
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

            // Day 15: projectile spawn on Cast Active (single-shot per cast).
            // attack_landed_ already gates single-fire for melee combos;
            // reuse the gate here. register_hit() sets the flag so
            // subsequent Active ticks within this Cast are no-ops. The flag
            // resets on the next state transition.
            {
                const auto& st = player.state();
                const bool cast_active = (st.id() == vigil::PlayerStateId::Cast &&
                                          st.attack_phase() == vigil::AttackPhase::Active);
                const bool can_fire    = (st.attack_landed() == false);
                if (cast_active && can_fire) {
                    const float yaw = player.yaw();
                    const glm::vec3 facing(std::sin(yaw), 0.0f, std::cos(yaw));
                    const glm::vec3 spawn_pos =
                        player.position() + glm::vec3(0.0f, kBoltSpawnY, 0.0f);

                    vigil::Projectile bolt;
                    bolt.position           = spawn_pos;
                    bolt.velocity           = facing * vigil::Projectile::BOLT_SPEED;
                    bolt.lifetime_remaining = vigil::Projectile::MAX_LIFETIME;
                    bolt.alive              = true;
                    bolt.target             = &vigil::Renderer::DUMMY_POSITION;
                    projectiles.push_back(bolt);
                    player.register_hit();
                    std::printf("[BOLT] spawned pos=%.2f,%.2f,%.2f  vel=%.2f m/s  -> dummy\n",
                                spawn_pos.x, spawn_pos.y, spawn_pos.z,
                                vigil::Projectile::BOLT_SPEED);
                }
            }

            // Day 12 cone hit detection (Light/Heavy/Riposte against dummy).
            // Day 15: gated off Cast - Cast Active is the bolt-spawn event,
            // not a melee swing. Without this gate the player got spurious
            // [HIT] Cast lines any time they cast within 2m of the dummy.
            // Booleans named in positive sense (fresh, melee) to dodge zsh
            // history expansion on !identifier patterns during paste.
            {
                const auto& st = player.state();
                const bool active = (st.attack_phase() == vigil::AttackPhase::Active);
                const bool fresh  = (st.attack_landed() == false);
                const bool melee  = (st.id() != vigil::PlayerStateId::Cast);
                if (active && fresh && melee) {
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

            // Day 15: tick projectiles, AABB-test vs dummy, cull dead.
            // update_projectile() (Projectile.cpp) handles integration +
            // weak 30 degree cone homing + lifetime decay. main owns hit
            // detection because the target list is currently main's
            // responsibility; Phase 4 hands this to an enemy system.
            for (auto& proj : projectiles) {
                if (proj.alive == false) continue;
                vigil::update_projectile(proj, dt);
                if (proj.alive == false) continue;

                const glm::vec3& dp = vigil::Renderer::DUMMY_POSITION;
                const float r       = vigil::Projectile::HIT_HALF_EXTENT;
                if (std::abs(proj.position.x - dp.x) <= r &&
                    std::abs(proj.position.y - dp.y) <= r &&
                    std::abs(proj.position.z - dp.z) <= r) {
                    std::printf("[BOLT HIT] dmg=%.0f at %.2f,%.2f,%.2f\n",
                                vigil::Projectile::BOLT_DAMAGE,
                                proj.position.x, proj.position.y, proj.position.z);
                    proj.alive = false;
                }
            }
            projectiles.erase(
                std::remove_if(projectiles.begin(), projectiles.end(),
                               [](const vigil::Projectile& q) { return q.alive == false; }),
                projectiles.end()
            );

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
            renderer.draw_debug_overlay(player, dt);
            ImGui::Render();

            renderer.draw_frame(camera, player, projectiles);
        }

        renderer.wait_idle();
        std::printf("[Vigil] Shutting down cleanly.\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[Vigil FATAL] %s\n", e.what());
        return 1;
    }
    return 0;
}
