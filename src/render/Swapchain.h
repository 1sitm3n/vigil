#pragma once

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace vigil {

class VulkanContext;
class Window;

class Swapchain {
public:
    Swapchain(VulkanContext& vk, Window& window);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain(Swapchain&&) = delete;
    Swapchain& operator=(Swapchain&&) = delete;

    VkSwapchainKHR handle()       const { return swapchain_.swapchain; }
    VkFormat       image_format() const { return swapchain_.image_format; }
    VkExtent2D     extent()       const { return swapchain_.extent; }
    uint32_t       image_count()  const { return static_cast<uint32_t>(image_views_.size()); }
    VkRenderPass   render_pass()  const { return render_pass_; }
    VkFramebuffer  framebuffer(uint32_t index) const { return framebuffers_[index]; }

private:
    void create_render_pass();
    void create_framebuffers();

    VulkanContext&             vk_;
    vkb::Swapchain             swapchain_{};
    std::vector<VkImage>       images_;
    std::vector<VkImageView>   image_views_;
    VkRenderPass               render_pass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers_;
};

}  // namespace vigil
