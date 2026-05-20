#include "render/Renderer.h"
#include "render/Swapchain.h"
#include "render/GraphicsPipeline.h"
#include "core/VulkanContext.h"

#include <cstdint>
#include <stdexcept>

namespace vigil {

Renderer::Renderer(VulkanContext& vk,
                   const Swapchain& swapchain,
                   const GraphicsPipeline& pipeline)
    : vk_(vk), swapchain_(swapchain), pipeline_(pipeline) {
    create_command_pool();
    create_command_buffers();
    create_sync_objects();
}

Renderer::~Renderer() {
    wait_idle();

    for (auto& frame : frames_) {
        if (frame.image_available) vkDestroySemaphore(vk_.device(), frame.image_available, nullptr);
        if (frame.in_flight)       vkDestroyFence(vk_.device(),     frame.in_flight,       nullptr);
    }
    for (auto sem : render_finished_) {
        if (sem) vkDestroySemaphore(vk_.device(), sem, nullptr);
    }
    if (command_pool_) vkDestroyCommandPool(vk_.device(), command_pool_, nullptr);
}

void Renderer::wait_idle() {
    vkDeviceWaitIdle(vk_.device());
}

void Renderer::create_command_pool() {
    VkCommandPoolCreateInfo info{};
    info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = vk_.graphics_family();
    if (vkCreateCommandPool(vk_.device(), &info, nullptr, &command_pool_) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateCommandPool failed");
    }
}

void Renderer::create_command_buffers() {
    VkCommandBuffer buffers[FRAMES_IN_FLIGHT]{};
    VkCommandBufferAllocateInfo info{};
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool        = command_pool_;
    info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = FRAMES_IN_FLIGHT;
    if (vkAllocateCommandBuffers(vk_.device(), &info, buffers) != VK_SUCCESS) {
        throw std::runtime_error("vkAllocateCommandBuffers failed");
    }
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        frames_[i].command_buffer = buffers[i];
    }
}

void Renderer::create_sync_objects() {
    VkSemaphoreCreateInfo sem_info{};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // first frame doesn't wait

    for (auto& frame : frames_) {
        if (vkCreateSemaphore(vk_.device(), &sem_info, nullptr, &frame.image_available) != VK_SUCCESS ||
            vkCreateFence(vk_.device(),     &fence_info, nullptr, &frame.in_flight)      != VK_SUCCESS) {
            throw std::runtime_error("Failed to create per-frame sync objects");
        }
    }

    render_finished_.resize(swapchain_.image_count(), VK_NULL_HANDLE);
    for (auto& sem : render_finished_) {
        if (vkCreateSemaphore(vk_.device(), &sem_info, nullptr, &sem) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create per-image render-finished semaphore");
        }
    }
}

void Renderer::draw_frame() {
    auto& frame = frames_[current_frame_];

    vkWaitForFences(vk_.device(), 1, &frame.in_flight, VK_TRUE, UINT64_MAX);

    uint32_t image_index = 0;
    VkResult acquire = vkAcquireNextImageKHR(
        vk_.device(), swapchain_.handle(), UINT64_MAX,
        frame.image_available, VK_NULL_HANDLE, &image_index);

    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        // Swapchain stale (resize). Skip this frame — recreation comes Day 4+.
        return;
    } else if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("vkAcquireNextImageKHR failed");
    }

    vkResetFences(vk_.device(), 1, &frame.in_flight);
    vkResetCommandBuffer(frame.command_buffer, 0);
    record_command_buffer(frame.command_buffer, image_index);

    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit{};
    submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount   = 1;
    submit.pWaitSemaphores      = &frame.image_available;
    submit.pWaitDstStageMask    = &wait_stage;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &frame.command_buffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = &render_finished_[image_index];

    if (vkQueueSubmit(vk_.graphics_queue(), 1, &submit, frame.in_flight) != VK_SUCCESS) {
        throw std::runtime_error("vkQueueSubmit failed");
    }

    VkSwapchainKHR swap = swapchain_.handle();
    VkPresentInfoKHR present{};
    present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores    = &render_finished_[image_index];
    present.swapchainCount     = 1;
    present.pSwapchains        = &swap;
    present.pImageIndices      = &image_index;

    VkResult present_result = vkQueuePresentKHR(vk_.present_queue(), &present);
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
        // Resize. Day 4+ handles recreation.
    } else if (present_result != VK_SUCCESS) {
        throw std::runtime_error("vkQueuePresentKHR failed");
    }

    current_frame_ = (current_frame_ + 1) % FRAMES_IN_FLIGHT;
}

void Renderer::record_command_buffer(VkCommandBuffer cmd, uint32_t image_index) {
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(cmd, &begin) != VK_SUCCESS) {
        throw std::runtime_error("vkBeginCommandBuffer failed");
    }

    // VOID_BG clear colour — Vigil's deepest black-violet
    VkClearValue clear{};
    clear.color = { { 0.031f, 0.027f, 0.039f, 1.0f } };

    const VkExtent2D ext = swapchain_.extent();

    VkRenderPassBeginInfo rp{};
    rp.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp.renderPass        = swapchain_.render_pass();
    rp.framebuffer       = swapchain_.framebuffer(image_index);
    rp.renderArea.offset = {0, 0};
    rp.renderArea.extent = ext;
    rp.clearValueCount   = 1;
    rp.pClearValues      = &clear;

    vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.handle());

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(ext.width);
    viewport.height   = static_cast<float>(ext.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = ext;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("vkEndCommandBuffer failed");
    }
}

}  // namespace vigil
