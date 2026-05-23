#pragma once

#include "anim/Animation.h"
#include "anim/Animator.h"
#include "game/PlayerState.h"
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

    Renderer(VulkanContext& vk,
             const Swapchain& swapchain,
             const GraphicsPipeline& pipeline,
             Window& window);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    // main.cpp calls this between ImGui::NewFrame and ImGui::Render so the
    // overlay's draw data is finalised by the time we record commands.
    void draw_debug_overlay(const PlayerState& player, float dt);
    void draw_frame(const Camera& camera, const PlayerState& player);
    void wait_idle();

private:
    struct FrameData {
        VkCommandBuffer command_buffer  = VK_NULL_HANDLE;
        VkSemaphore     image_available = VK_NULL_HANDLE;
        VkFence         in_flight       = VK_NULL_HANDLE;
    };

    // One Animation + its playback knobs per PlayerStateId. anim_slots_ is
    // indexed by static_cast<size_t>(PlayerStateId::X). PlayerStateId is the
    // contract between SM and Renderer — adding a state means adding a slot.
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
    void inspect_attack_candidates();
    void create_descriptor_pool();
    void create_descriptor_sets();
    void init_imgui();
    void shutdown_imgui();
    void record_command_buffer(VkCommandBuffer cmd, uint32_t image_index,
                               const Camera& camera);
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
    Texture                                       diffuse_texture_;
    std::array<Buffer, FRAMES_IN_FLIGHT>          bone_palette_buffers_{};
    VkDescriptorPool                              descriptor_pool_ = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> descriptor_sets_{};

    // Separate descriptor pool for ImGui — it allocates its own font texture
    // descriptor + any user-bound textures. Don't share with the main pool.
    VkDescriptorPool imgui_descriptor_pool_ = VK_NULL_HANDLE;
    bool             imgui_ready_           = false;

    std::array<AnimSlot, static_cast<size_t>(PlayerStateId::Count)> anim_slots_;
    Animator                                      animator_;
    std::vector<glm::mat4>                        palette_scratch_;

    // FPS rolling average for the overlay.
    static constexpr size_t FPS_WINDOW = 60;
    std::array<float, FPS_WINDOW> fps_window_{};
    size_t fps_cursor_ = 0;

    std::chrono::high_resolution_clock::time_point last_frame_time_;
};

}  // namespace vigil
