#pragma once

#include "anim/Animation.h"
#include "anim/Animator.h"
#include "game/Player.h"
#include "game/Projectile.h"
#include "render/Buffer.h"
#include "render/Mesh.h"
#include "render/Texture.h"

#include <vulkan/vulkan.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <vector>

namespace vigil {

class VulkanContext;
class Swapchain;
class GraphicsPipeline;
class Camera;
class Window;

class Renderer {
public:
    static constexpr uint32_t FRAMES_IN_FLIGHT = 2;
    static constexpr uint32_t MAX_BONES        = 128;

    // Day 12: single practice dummy world position. Used by Renderer for
    // the draw call, by main.cpp for cone-overlap hit detection, and (Day 15)
    // by main.cpp as Projectile::target for weak-homing.
    static constexpr glm::vec3 DUMMY_POSITION{3.0f, 0.0f, -2.0f};

    Renderer(VulkanContext& vk,
             const Swapchain& swapchain,
             const GraphicsPipeline& pipeline,
             Window& window);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    // Day 11: draw_debug_overlay + draw_frame take the Player wrapper.
    // Day 15: draw_frame also accepts the active projectile pool. Empty
    // default keeps callers happy until main.cpp wires the pool in Paste 6.
    // Each live projectile draws as a small cube via the slot-127 identity-
    // skin trick (same path the practice dummy uses).
    void draw_debug_overlay(const Player& player, float dt);
    void draw_frame(const Camera& camera, const Player& player,
                    const std::vector<Projectile>& projectiles = {});
    void wait_idle();

private:
    struct FrameData {
        VkCommandBuffer command_buffer  = VK_NULL_HANDLE;
        VkSemaphore     image_available = VK_NULL_HANDLE;
        VkFence         in_flight       = VK_NULL_HANDLE;
    };

    struct AnimSlot {
        Animation anim;
        float     speed      = 1.0f;
        bool      strip_root = true;
    };

    void create_command_pool();
    void create_command_buffers();
    void create_sync_objects();
    void create_mesh();
    void create_texture();
    void create_bone_palette_buffers();
    void load_animations();
    void create_descriptor_pool();
    void create_descriptor_sets();
    void init_imgui();
    void shutdown_imgui();
    void record_command_buffer(VkCommandBuffer cmd, uint32_t image_index,
                               const Camera& camera,
                               const glm::mat4& world_transform,
                               const std::vector<Projectile>& projectiles);
    void switch_to(PlayerStateId id, float fade);

    VulkanContext&          vk_;
    const Swapchain&        swapchain_;
    const GraphicsPipeline& pipeline_;
    Window&                 window_;

    VkCommandPool                           command_pool_ = VK_NULL_HANDLE;
    std::array<FrameData, FRAMES_IN_FLIGHT> frames_{};
    std::vector<VkSemaphore>                render_finished_;
    uint32_t                                current_frame_ = 0;

    Mesh                                          mesh_;
    Mesh                                          dummy_mesh_;  // Day 12 dummy cube; Day 15 reused for projectile draws
    Texture                                       diffuse_texture_;
    std::array<Buffer, FRAMES_IN_FLIGHT>          bone_palette_buffers_{};
    VkDescriptorPool                              descriptor_pool_ = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> descriptor_sets_{};

    VkDescriptorPool imgui_descriptor_pool_ = VK_NULL_HANDLE;
    bool             imgui_ready_           = false;

    std::array<AnimSlot, static_cast<size_t>(PlayerStateId::Count)> anim_slots_;
    Animator                                      animator_;
    std::vector<glm::mat4>                        palette_scratch_;

    static constexpr size_t FPS_WINDOW = 60;
    std::array<float, FPS_WINDOW> fps_window_{};
    size_t fps_cursor_ = 0;

    std::chrono::high_resolution_clock::time_point last_frame_time_;
};

}  // namespace vigil
