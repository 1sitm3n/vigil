// =============================================================================
//  Vigil — Day 5
//  Orbit camera (RMB drag), zoom (scroll), WASD target pan.
//  Phase 1 checkpoint: textured cat tumbles, camera flies around it.
// =============================================================================

#include "core/Camera.h"
#include "core/Window.h"
#include "core/VulkanContext.h"
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
        std::printf("================ Vigil — Day 5 ================\n");

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
        vigil::Renderer renderer(vk, swapchain, pipeline);
        vigil::Camera   camera;

        std::printf("[Vigil] Hold RMB + drag to orbit. Scroll to zoom. WASD to pan.\n");
        std::printf("[Vigil] Close window or press Esc to quit.\n");

        auto last_time = std::chrono::high_resolution_clock::now();

        while (!window.should_close()) {
            window.poll_events();

            const auto now = std::chrono::high_resolution_clock::now();
            const float dt = std::chrono::duration<float>(now - last_time).count();
            last_time = now;

            const auto input = window.consume_input();

            if (input.rmb_held && (input.mouse_dx != 0.0f || input.mouse_dy != 0.0f)) {
                camera.orbit(input.mouse_dx, input.mouse_dy);
            }
            if (input.scroll_y != 0.0f) {
                camera.zoom(input.scroll_y);
            }

            glm::vec3 pan_dir(0.0f);
            if (input.w_held) pan_dir -= camera.forward_xz();
            if (input.s_held) pan_dir += camera.forward_xz();
            if (input.d_held) pan_dir -= camera.right_xz();
            if (input.a_held) pan_dir += camera.right_xz();

            if (glm::length(pan_dir) > 0.0001f) {
                pan_dir = glm::normalize(pan_dir);
                camera.pan_target(pan_dir * camera.pan_speed * dt);
            }

            renderer.draw_frame(camera);
        }

        renderer.wait_idle();
        std::printf("[Vigil] Shutting down cleanly.\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[Vigil FATAL] %s\n", e.what());
        return 1;
    }
    return 0;
}
