#include "render/Buffer.h"

#include "core/VulkanContext.h"

#include <cstring>
#include <stdexcept>

namespace vigil {

Buffer::Buffer(VulkanContext& vk,
               VkDeviceSize size,
               VkBufferUsageFlags usage,
               VkMemoryPropertyFlags properties)
    : vk_(&vk), size_(size) {

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size        = size;
    buffer_info.usage       = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vk.device(), &buffer_info, nullptr, &buffer_) != VK_SUCCESS) {
        throw std::runtime_error("Buffer: vkCreateBuffer failed");
    }

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(vk.device(), buffer_, &req);

    VkMemoryAllocateInfo alloc{};
    alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize  = req.size;
    alloc.memoryTypeIndex = find_memory_type(vk.physical_device(),
                                             req.memoryTypeBits,
                                             properties);

    if (vkAllocateMemory(vk.device(), &alloc, nullptr, &memory_) != VK_SUCCESS) {
        vkDestroyBuffer(vk.device(), buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
        throw std::runtime_error("Buffer: vkAllocateMemory failed");
    }

    vkBindBufferMemory(vk.device(), buffer_, memory_, 0);

    // Persistently map if host-visible. HOST_COHERENT means no manual flush.
    if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        vkMapMemory(vk.device(), memory_, 0, size_, 0, &mapped_);
    }
}

Buffer::~Buffer() { destroy(); }

Buffer::Buffer(Buffer&& other) noexcept
    : vk_(other.vk_),
      buffer_(other.buffer_),
      memory_(other.memory_),
      size_(other.size_),
      mapped_(other.mapped_) {
    other.vk_     = nullptr;
    other.buffer_ = VK_NULL_HANDLE;
    other.memory_ = VK_NULL_HANDLE;
    other.size_   = 0;
    other.mapped_ = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        destroy();
        vk_     = other.vk_;
        buffer_ = other.buffer_;
        memory_ = other.memory_;
        size_   = other.size_;
        mapped_ = other.mapped_;
        other.vk_     = nullptr;
        other.buffer_ = VK_NULL_HANDLE;
        other.memory_ = VK_NULL_HANDLE;
        other.size_   = 0;
        other.mapped_ = nullptr;
    }
    return *this;
}

void Buffer::upload(const void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (!mapped_) {
        throw std::runtime_error("Buffer::upload on non-host-visible buffer");
    }
    std::memcpy(static_cast<char*>(mapped_) + offset, data, size);
}

void Buffer::destroy() {
    if (!vk_) return;
    VkDevice dev = vk_->device();
    if (mapped_) {
        vkUnmapMemory(dev, memory_);
        mapped_ = nullptr;
    }
    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(dev, buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
    }
    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(dev, memory_, nullptr);
        memory_ = VK_NULL_HANDLE;
    }
    vk_ = nullptr;
}

uint32_t Buffer::find_memory_type(VkPhysicalDevice phys,
                                  uint32_t type_filter,
                                  VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_props{};
    vkGetPhysicalDeviceMemoryProperties(phys, &mem_props);
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((type_filter & (1u << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("Buffer: no suitable memory type");
}

}  // namespace vigil
