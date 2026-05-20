#pragma once

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

#include <cstdint>

namespace vigil {

class Window;

class VulkanContext {
public:
    explicit VulkanContext(const Window& window);
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;

    VkInstance       instance()         const { return instance_.instance; }
    VkPhysicalDevice physical_device()  const { return physical_device_.physical_device; }
    VkDevice         device()           const { return device_.device; }
    VkSurfaceKHR     surface()          const { return surface_; }
    VkQueue          graphics_queue()   const { return graphics_queue_; }
    VkQueue          present_queue()    const { return present_queue_; }
    uint32_t         graphics_family()  const { return graphics_family_; }
    uint32_t         present_family()   const { return present_family_; }

private:
    vkb::Instance       instance_;
    VkSurfaceKHR        surface_ = VK_NULL_HANDLE;
    vkb::PhysicalDevice physical_device_;
    vkb::Device         device_;
    VkQueue             graphics_queue_  = VK_NULL_HANDLE;
    VkQueue             present_queue_   = VK_NULL_HANDLE;
    uint32_t            graphics_family_ = 0;
    uint32_t            present_family_  = 0;
};

}  // namespace vigil
