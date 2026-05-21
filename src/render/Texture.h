#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>

namespace vigil {

class VulkanContext;

// RAII wrapper for a sampled 2D texture: VkImage + memory + view + sampler.
// Loads from a PNG via stb_image; uploads via a staging buffer and a one-shot
// command buffer allocated from the caller's command pool.
class Texture {
public:
    Texture() = default;
    Texture(VulkanContext& vk, VkCommandPool pool, const std::string& path);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    VkImageView view()    const { return view_; }
    VkSampler   sampler() const { return sampler_; }
    uint32_t    width()   const { return width_; }
    uint32_t    height()  const { return height_; }

private:
    void destroy();

    VulkanContext* vk_      = nullptr;
    VkImage        image_   = VK_NULL_HANDLE;
    VkDeviceMemory memory_  = VK_NULL_HANDLE;
    VkImageView    view_    = VK_NULL_HANDLE;
    VkSampler      sampler_ = VK_NULL_HANDLE;
    uint32_t       width_   = 0;
    uint32_t       height_  = 0;
};

}  // namespace vigil
