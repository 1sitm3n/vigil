#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <vector>

namespace vigil {

class VulkanContext;
class Swapchain;
class GraphicsPipeline;

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

    void draw_frame();
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
    void record_command_buffer(VkCommandBuffer cmd, uint32_t image_index);

    VulkanContext&          vk_;
    const Swapchain&        swapchain_;
    const GraphicsPipeline& pipeline_;

    VkCommandPool                          command_pool_ = VK_NULL_HANDLE;
    std::array<FrameData, FRAMES_IN_FLIGHT> frames_{};
    std::vector<VkSemaphore>               render_finished_;  // one per swapchain image
    uint32_t                               current_frame_ = 0;
};

}  // namespace vigil
