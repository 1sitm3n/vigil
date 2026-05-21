#include "render/Renderer.h"
#include "render/Swapchain.h"
#include "render/GraphicsPipeline.h"
#include "render/Vertex.h"
#include "core/VulkanContext.h"
#include "core/Camera.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cstdint>
#include <stdexcept>

namespace vigil {

Renderer::Renderer(VulkanContext& vk,
                   const Swapchain& swapchain,
                   const GraphicsPipeline& pipeline)
    : vk_(vk), swapchain_(swapchain), pipeline_(pipeline) {
    start_time_ = std::chrono::high_resolution_clock::now();
    create_command_pool();
    create_command_buffers();
    create_sync_objects();
    create_mesh();
    create_texture();
    create_descriptor_pool();
    create_descriptor_sets();
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
    if (descriptor_pool_) vkDestroyDescriptorPool(vk_.device(), descriptor_pool_, nullptr);
    if (command_pool_)    vkDestroyCommandPool(vk_.device(), command_pool_, nullptr);
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
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

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

void Renderer::create_mesh() {
    mesh_ = Mesh(vk_, "assets/test/concrete_cat_statue_1k.gltf");
}

void Renderer::create_texture() {
    diffuse_texture_ = Texture(vk_, command_pool_, "assets/test/checker.png");
}

void Renderer::create_descriptor_pool() {
    VkDescriptorPoolSize pool_size{};
    pool_size.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo info{};
    info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = 1;
    info.pPoolSizes    = &pool_size;
    info.maxSets       = FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(vk_.device(), &info, nullptr, &descriptor_pool_) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateDescriptorPool failed");
    }
}

void Renderer::create_descriptor_sets() {
    std::array<VkDescriptorSetLayout, FRAMES_IN_FLIGHT> layouts;
    layouts.fill(pipeline_.descriptor_set_layout());

    VkDescriptorSetAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool     = descriptor_pool_;
    ai.descriptorSetCount = FRAMES_IN_FLIGHT;
    ai.pSetLayouts        = layouts.data();

    if (vkAllocateDescriptorSets(vk_.device(), &ai, descriptor_sets_.data()) != VK_SUCCESS) {
        throw std::runtime_error("vkAllocateDescriptorSets failed");
    }

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView   = diffuse_texture_.view();
        image_info.sampler     = diffuse_texture_.sampler();

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = descriptor_sets_[i];
        write.dstBinding      = 0;
        write.dstArrayElement = 0;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo      = &image_info;

        vkUpdateDescriptorSets(vk_.device(), 1, &write, 0, nullptr);
    }
}

void Renderer::draw_frame(const Camera& camera) {
    auto& frame = frames_[current_frame_];

    vkWaitForFences(vk_.device(), 1, &frame.in_flight, VK_TRUE, UINT64_MAX);

    uint32_t image_index = 0;
    VkResult acquire = vkAcquireNextImageKHR(
        vk_.device(), swapchain_.handle(), UINT64_MAX,
        frame.image_available, VK_NULL_HANDLE, &image_index);

    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        return;
    } else if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("vkAcquireNextImageKHR failed");
    }

    vkResetFences(vk_.device(), 1, &frame.in_flight);
    vkResetCommandBuffer(frame.command_buffer, 0);
    record_command_buffer(frame.command_buffer, image_index, camera);

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
        // Resize handled later.
    } else if (present_result != VK_SUCCESS) {
        throw std::runtime_error("vkQueuePresentKHR failed");
    }

    current_frame_ = (current_frame_ + 1) % FRAMES_IN_FLIGHT;
}

void Renderer::record_command_buffer(VkCommandBuffer cmd, uint32_t image_index,
                                     const Camera& camera) {
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(cmd, &begin) != VK_SUCCESS) {
        throw std::runtime_error("vkBeginCommandBuffer failed");
    }

    std::array<VkClearValue, 2> clears{};
    clears[0].color        = { { 0.031f, 0.027f, 0.039f, 1.0f } };
    clears[1].depthStencil = { 1.0f, 0 };

    const VkExtent2D ext = swapchain_.extent();

    VkRenderPassBeginInfo rp{};
    rp.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp.renderPass        = swapchain_.render_pass();
    rp.framebuffer       = swapchain_.framebuffer(image_index);
    rp.renderArea.offset = {0, 0};
    rp.renderArea.extent = ext;
    rp.clearValueCount   = static_cast<uint32_t>(clears.size());
    rp.pClearValues      = clears.data();

    vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.handle());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.layout(),
                            0, 1, &descriptor_sets_[current_frame_], 0, nullptr);

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

    const auto now = std::chrono::high_resolution_clock::now();
    const float t = std::chrono::duration<float>(now - start_time_).count();

    glm::mat4 model = glm::rotate(glm::mat4(1.0f), t * 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));

    const float aspect = static_cast<float>(ext.width) / static_cast<float>(ext.height);
    glm::mat4 mvp = camera.projection(aspect) * camera.view() * model;

    vkCmdPushConstants(cmd, pipeline_.layout(),
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(mvp), &mvp);

    VkBuffer     vbufs[]   = { mesh_.vertex_buffer_handle() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vbufs, offsets);
    vkCmdBindIndexBuffer(cmd, mesh_.index_buffer_handle(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, mesh_.index_count(), 1, 0, 0, 0);

    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("vkEndCommandBuffer failed");
    }
}

}  // namespace vigil
