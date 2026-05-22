#pragma once

#include "render/Buffer.h"
#include "render/Skeleton.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>

namespace vigil {

class VulkanContext;

// Loads a single static or skinned mesh from a .glb (first mesh, first primitive).
// Material/texture handling still lives outside the Mesh (Renderer owns the
// diffuse Texture); extracting the glTF's bound material is a Phase 2 cleanup.
class Mesh {
public:
    Mesh() = default;
    Mesh(VulkanContext& vk, const std::string& path);

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    VkBuffer        vertex_buffer_handle() const { return vertex_buffer_.handle(); }
    VkBuffer        index_buffer_handle()  const { return index_buffer_.handle(); }
    uint32_t        index_count()          const { return index_count_; }

    const Skeleton& skeleton()             const { return skeleton_; }
    bool            has_skin()             const { return !skeleton_.empty(); }

private:
    Buffer   vertex_buffer_;
    Buffer   index_buffer_;
    uint32_t index_count_ = 0;
    Skeleton skeleton_;
};

}  // namespace vigil
