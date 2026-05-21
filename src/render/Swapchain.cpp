#include "render/Swapchain.h"
#include "core/VulkanContext.h"
#include "core/Window.h"

#include <SDL3/SDL.h>

#include <array>
#include <stdexcept>

namespace vigil {

namespace {

uint32_t find_memory_type(VkPhysicalDevice phys,
                          uint32_t type_filter,
                          VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mp{};
    vkGetPhysicalDeviceMemoryProperties(phys, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; ++i) {
        if ((type_filter & (1u << i)) &&
            (mp.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("Swapchain: no suitable memory type for depth image");
}

}  // namespace

Swapchain::Swapchain(VulkanContext& vk, Window& window) : vk_(vk) {
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
    create_depth_resources();
    create_framebuffers();
}

Swapchain::~Swapchain() {
    for (auto fb : framebuffers_) {
        vkDestroyFramebuffer(vk_.device(), fb, nullptr);
    }
    if (depth_view_)   vkDestroyImageView(vk_.device(), depth_view_, nullptr);
    if (depth_image_)  vkDestroyImage(vk_.device(), depth_image_, nullptr);
    if (depth_memory_) vkFreeMemory(vk_.device(), depth_memory_, nullptr);
    if (render_pass_)  vkDestroyRenderPass(vk_.device(), render_pass_, nullptr);
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

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format         = depth_format_;
    depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_ref{};
    color_ref.attachment = 0;
    color_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_ref{};
    depth_ref.attachment = 1;
    depth_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &color_ref;
    subpass.pDepthStencilAttachment = &depth_ref;

    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                      | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                      | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                      | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    const std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo info{};
    info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = static_cast<uint32_t>(attachments.size());
    info.pAttachments    = attachments.data();
    info.subpassCount    = 1;
    info.pSubpasses      = &subpass;
    info.dependencyCount = 1;
    info.pDependencies   = &dep;

    if (vkCreateRenderPass(vk_.device(), &info, nullptr, &render_pass_) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateRenderPass failed");
    }
}

void Swapchain::create_depth_resources() {
    VkImageCreateInfo image_info{};
    image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType     = VK_IMAGE_TYPE_2D;
    image_info.extent.width  = swapchain_.extent.width;
    image_info.extent.height = swapchain_.extent.height;
    image_info.extent.depth  = 1;
    image_info.mipLevels     = 1;
    image_info.arrayLayers   = 1;
    image_info.format        = depth_format_;
    image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples       = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(vk_.device(), &image_info, nullptr, &depth_image_) != VK_SUCCESS) {
        throw std::runtime_error("Swapchain: vkCreateImage (depth) failed");
    }

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(vk_.device(), depth_image_, &req);

    VkMemoryAllocateInfo alloc{};
    alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize  = req.size;
    alloc.memoryTypeIndex = find_memory_type(vk_.physical_device(),
                                             req.memoryTypeBits,
                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(vk_.device(), &alloc, nullptr, &depth_memory_) != VK_SUCCESS) {
        throw std::runtime_error("Swapchain: vkAllocateMemory (depth) failed");
    }
    vkBindImageMemory(vk_.device(), depth_image_, depth_memory_, 0);

    VkImageViewCreateInfo view_info{};
    view_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image    = depth_image_;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format   = depth_format_;
    view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel   = 0;
    view_info.subresourceRange.levelCount     = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount     = 1;
    if (vkCreateImageView(vk_.device(), &view_info, nullptr, &depth_view_) != VK_SUCCESS) {
        throw std::runtime_error("Swapchain: vkCreateImageView (depth) failed");
    }
}

void Swapchain::create_framebuffers() {
    framebuffers_.resize(image_views_.size());
    for (size_t i = 0; i < image_views_.size(); ++i) {
        const std::array<VkImageView, 2> attachments = { image_views_[i], depth_view_ };

        VkFramebufferCreateInfo info{};
        info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass      = render_pass_;
        info.attachmentCount = static_cast<uint32_t>(attachments.size());
        info.pAttachments    = attachments.data();
        info.width           = swapchain_.extent.width;
        info.height          = swapchain_.extent.height;
        info.layers          = 1;
        if (vkCreateFramebuffer(vk_.device(), &info, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateFramebuffer failed");
        }
    }
}

}  // namespace vigil
