#include "render/Texture.h"

#include "render/Buffer.h"
#include "core/VulkanContext.h"

#include <stb_image.h>

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
    throw std::runtime_error("Texture: no suitable memory type");
}

VkCommandBuffer begin_oneshot(VkDevice dev, VkCommandPool pool) {
    VkCommandBufferAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandPool        = pool;
    ai.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(dev, &ai, &cmd);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);
    return cmd;
}

void end_oneshot(VkDevice dev, VkCommandPool pool, VkQueue queue, VkCommandBuffer cmd) {
    vkEndCommandBuffer(cmd);
    VkSubmitInfo si{};
    si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cmd;
    vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(dev, pool, 1, &cmd);
}

void transition_layout(VkCommandBuffer cmd,
                       VkImage image,
                       VkImageLayout old_layout,
                       VkImageLayout new_layout,
                       VkPipelineStageFlags src_stage,
                       VkPipelineStageFlags dst_stage,
                       VkAccessFlags src_access,
                       VkAccessFlags dst_access) {
    VkImageMemoryBarrier barrier{};
    barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                   = old_layout;
    barrier.newLayout                   = new_layout;
    barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                       = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.srcAccessMask               = src_access;
    barrier.dstAccessMask               = dst_access;

    vkCmdPipelineBarrier(cmd, src_stage, dst_stage,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
}

}  // namespace

Texture::Texture(VulkanContext& vk, VkCommandPool pool, const std::string& path)
    : vk_(&vk) {

    // 1. Load pixels (forced to RGBA8).
    int w = 0, h = 0, channels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("Texture: stbi_load failed for " + path);
    }
    width_  = static_cast<uint32_t>(w);
    height_ = static_cast<uint32_t>(h);
    const VkDeviceSize image_size = static_cast<VkDeviceSize>(w) * h * 4;

    // 2. Stage pixels in a host-visible buffer.
    Buffer staging(vk,
                   image_size,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    staging.upload(pixels, image_size);
    stbi_image_free(pixels);

    // 3. Create the device-local image.
    VkImageCreateInfo image_info{};
    image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType     = VK_IMAGE_TYPE_2D;
    image_info.extent.width  = width_;
    image_info.extent.height = height_;
    image_info.extent.depth  = 1;
    image_info.mipLevels     = 1;
    image_info.arrayLayers   = 1;
    image_info.format        = VK_FORMAT_R8G8B8A8_SRGB;
    image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples       = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(vk.device(), &image_info, nullptr, &image_) != VK_SUCCESS) {
        throw std::runtime_error("Texture: vkCreateImage failed");
    }

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(vk.device(), image_, &req);

    VkMemoryAllocateInfo alloc{};
    alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize  = req.size;
    alloc.memoryTypeIndex = find_memory_type(vk.physical_device(),
                                             req.memoryTypeBits,
                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(vk.device(), &alloc, nullptr, &memory_) != VK_SUCCESS) {
        throw std::runtime_error("Texture: vkAllocateMemory failed");
    }
    vkBindImageMemory(vk.device(), image_, memory_, 0);

    // 4. One-shot command buffer: undef → transfer_dst → copy → shader_read.
    VkCommandBuffer cmd = begin_oneshot(vk.device(), pool);

    transition_layout(cmd, image_,
                      VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                      VK_PIPELINE_STAGE_TRANSFER_BIT,
                      0,
                      VK_ACCESS_TRANSFER_WRITE_BIT);

    VkBufferImageCopy region{};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width_, height_, 1};
    vkCmdCopyBufferToImage(cmd,
                           staging.handle(),
                           image_,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &region);

    transition_layout(cmd, image_,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_PIPELINE_STAGE_TRANSFER_BIT,
                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                      VK_ACCESS_TRANSFER_WRITE_BIT,
                      VK_ACCESS_SHADER_READ_BIT);

    end_oneshot(vk.device(), pool, vk.graphics_queue(), cmd);
    // staging Buffer destructs here (RAII), freeing its buffer + memory.

    // 5. Image view.
    VkImageViewCreateInfo view_info{};
    view_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image    = image_;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format   = VK_FORMAT_R8G8B8A8_SRGB;
    view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel   = 0;
    view_info.subresourceRange.levelCount     = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount     = 1;
    if (vkCreateImageView(vk.device(), &view_info, nullptr, &view_) != VK_SUCCESS) {
        throw std::runtime_error("Texture: vkCreateImageView failed");
    }

    // 6. Sampler — linear, repeat, no aniso, no mipmaps (single level).
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter        = VK_FILTER_LINEAR;
    sampler_info.minFilter        = VK_FILTER_LINEAR;
    sampler_info.addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable    = VK_FALSE;
    sampler_info.compareOp        = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias       = 0.0f;
    sampler_info.minLod           = 0.0f;
    sampler_info.maxLod           = 0.0f;
    if (vkCreateSampler(vk.device(), &sampler_info, nullptr, &sampler_) != VK_SUCCESS) {
        throw std::runtime_error("Texture: vkCreateSampler failed");
    }
}

Texture::~Texture() { destroy(); }

Texture::Texture(Texture&& other) noexcept
    : vk_(other.vk_), image_(other.image_), memory_(other.memory_),
      view_(other.view_), sampler_(other.sampler_),
      width_(other.width_), height_(other.height_) {
    other.vk_      = nullptr;
    other.image_   = VK_NULL_HANDLE;
    other.memory_  = VK_NULL_HANDLE;
    other.view_    = VK_NULL_HANDLE;
    other.sampler_ = VK_NULL_HANDLE;
    other.width_   = 0;
    other.height_  = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        destroy();
        vk_      = other.vk_;
        image_   = other.image_;
        memory_  = other.memory_;
        view_    = other.view_;
        sampler_ = other.sampler_;
        width_   = other.width_;
        height_  = other.height_;
        other.vk_      = nullptr;
        other.image_   = VK_NULL_HANDLE;
        other.memory_  = VK_NULL_HANDLE;
        other.view_    = VK_NULL_HANDLE;
        other.sampler_ = VK_NULL_HANDLE;
        other.width_   = 0;
        other.height_  = 0;
    }
    return *this;
}

void Texture::destroy() {
    if (!vk_) return;
    VkDevice dev = vk_->device();
    if (sampler_) { vkDestroySampler(dev, sampler_, nullptr); sampler_ = VK_NULL_HANDLE; }
    if (view_)    { vkDestroyImageView(dev, view_,   nullptr); view_   = VK_NULL_HANDLE; }
    if (image_)   { vkDestroyImage(dev, image_,      nullptr); image_  = VK_NULL_HANDLE; }
    if (memory_)  { vkFreeMemory(dev, memory_,       nullptr); memory_ = VK_NULL_HANDLE; }
    vk_ = nullptr;
}

}  // namespace vigil
