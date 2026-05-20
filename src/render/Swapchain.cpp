#include "render/Swapchain.h"
#include "core/VulkanContext.h"
#include "core/Window.h"

#include <SDL3/SDL.h>

#include <stdexcept>

namespace vigil {

Swapchain::Swapchain(VulkanContext& vk, Window& window) : vk_(vk) {
    // High-DPI: use pixel size, not logical size, for swapchain extent.
    int pixel_w = 0, pixel_h = 0;
    SDL_GetWindowSizeInPixels(window.native_handle(), &pixel_w, &pixel_h);

    vkb::SwapchainBuilder builder{vk_.physical_device(), vk_.device(), vk_.surface()};
    auto ret = builder
        .set_desired_format({VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(static_cast<uint32_t>(pixel_w),
                            static_cast<uint32_t>(pixel_h))
        .add_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        .build();
    if (!ret) {
        throw std::runtime_error("Failed to create swapchain: " + ret.error().message());
    }
    swapchain_   = ret.value();
    images_      = swapchain_.get_images().value();
    image_views_ = swapchain_.get_image_views().value();

    create_render_pass();
    create_framebuffers();
}

Swapchain::~Swapchain() {
    for (auto fb : framebuffers_) {
        vkDestroyFramebuffer(vk_.device(), fb, nullptr);
    }
    if (render_pass_) {
        vkDestroyRenderPass(vk_.device(), render_pass_, nullptr);
    }
    swapchain_.destroy_image_views(image_views_);
    vkb::destroy_swapchain(swapchain_);
}

void Swapchain::create_render_pass() {
    VkAttachmentDescription color_attachment{};
    color_attachment.format         = swapchain_.image_format;
    color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_ref{};
    color_ref.attachment = 0;
    color_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &color_ref;

    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info{};
    info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments    = &color_attachment;
    info.subpassCount    = 1;
    info.pSubpasses      = &subpass;
    info.dependencyCount = 1;
    info.pDependencies   = &dep;

    if (vkCreateRenderPass(vk_.device(), &info, nullptr, &render_pass_) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateRenderPass failed");
    }
}

void Swapchain::create_framebuffers() {
    framebuffers_.resize(image_views_.size());
    for (size_t i = 0; i < image_views_.size(); ++i) {
        VkFramebufferCreateInfo info{};
        info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass      = render_pass_;
        info.attachmentCount = 1;
        info.pAttachments    = &image_views_[i];
        info.width           = swapchain_.extent.width;
        info.height          = swapchain_.extent.height;
        info.layers          = 1;
        if (vkCreateFramebuffer(vk_.device(), &info, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateFramebuffer failed");
        }
    }
}

}  // namespace vigil
