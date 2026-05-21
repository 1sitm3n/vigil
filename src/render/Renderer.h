#pragma once

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

class Renderer {
public:
    static constexpr uint32_t FRAMES_IN_FLIGHT = 2;

    Renderer(VulkanContext& vk,
             const Swapchain& swapchain,
             const GraphicsPipeline& pipeline);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void draw_frame(const Camera& camera);
    void wait_idle();

private:
    struct FrameData {
        VkCommandBuffer command_buffer  = VK_NULL_HANDLE;
        VkSemaphore     image_available = VK_NULL_HANDLE;
        VkFence         in_flight       = VK_NULL_HANDLE;
    };

    void create_command_pool();
    void create_command_buffers();
    void create_sync_objects();
    void create_mesh();
    void create_texture();
    void create_descriptor_pool();
    void create_descriptor_sets();
    void record_command_buffer(VkCommandBuffer cmd, uint32_t image_index,
                               const Camera& camera);

    VulkanContext&          vk_;
    const Swapchain&        swapchain_;
    const GraphicsPipeline& pipeline_;

    VkCommandPool                           command_pool_ = VK_NULL_HANDLE;
    std::array<FrameData, FRAMES_IN_FLIGHT> frames_{};
    std::vector<VkSemaphore>                render_finished_;
    uint32_t                                current_frame_ = 0;

    Mesh                                    mesh_;

    Texture                                       diffuse_texture_;
    VkDescriptorPool                              descriptor_pool_ = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> descriptor_sets_{};

    std::chrono::high_resolution_clock::time_point start_time_;
};

}  // namespace vigil
