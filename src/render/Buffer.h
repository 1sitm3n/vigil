#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace vigil {

class VulkanContext;

// RAII wrapper for VkBuffer + VkDeviceMemory.
// Host-visible buffers are persistently mapped; call upload() to copy data.
class Buffer {
public:
    Buffer() = default;
    Buffer(VulkanContext& vk,
           VkDeviceSize size,
           VkBufferUsageFlags usage,
           VkMemoryPropertyFlags properties);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    void upload(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

    VkBuffer     handle() const { return buffer_; }
    VkDeviceSize size()   const { return size_; }

private:
    void destroy();
    static uint32_t find_memory_type(VkPhysicalDevice phys,
                                     uint32_t type_filter,
                                     VkMemoryPropertyFlags properties);

    VulkanContext* vk_     = nullptr;
    VkBuffer       buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkDeviceSize   size_   = 0;
    void*          mapped_ = nullptr;
};

}  // namespace vigil
