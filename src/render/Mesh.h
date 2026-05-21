#pragma once

#include "render/Buffer.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>

namespace vigil {

class VulkanContext;

// Loads a single static mesh from a .glb (first mesh, first primitive).
// Geometry only — material/texture handling lands in a later iteration.
class Mesh {
public:
    Mesh() = default;
    Mesh(VulkanContext& vk, const std::string& path);

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    VkBuffer vertex_buffer_handle() const { return vertex_buffer_.handle(); }
    VkBuffer index_buffer_handle()  const { return index_buffer_.handle(); }
    uint32_t index_count()          const { return index_count_; }

private:
    Buffer   vertex_buffer_;
    Buffer   index_buffer_;
    uint32_t index_count_ = 0;
};

}  // namespace vigil
