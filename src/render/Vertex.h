#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace vigil {

// Single interleaved vertex layout for all meshes, skinned or static.
//
// Skinning attributes carry through the static path with no perf hit worth
// caring about at this scope: a non-skinned mesh sets joints={0,0,0,0} and
// weights={1,0,0,0}. With an identity bone palette the linear-blend math in
// the vertex shader collapses to (1.0 * I) * pos = pos, so the static render
// is bit-identical to a no-skin pipeline.
struct Vertex {
    glm::vec3  pos;
    glm::vec2  uv;
    glm::uvec4 joints  {0u};                              // node indices into Skeleton's flat joint array
    glm::vec4  weights {1.0f, 0.0f, 0.0f, 0.0f};          // sum = 1.0 per glTF spec

    static VkVertexInputBindingDescription binding_description() {
        VkVertexInputBindingDescription b{};
        b.binding   = 0;
        b.stride    = sizeof(Vertex);
        b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return b;
    }

    static std::array<VkVertexInputAttributeDescription, 4> attribute_descriptions() {
        std::array<VkVertexInputAttributeDescription, 4> a{};
        a[0].binding  = 0;
        a[0].location = 0;
        a[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        a[0].offset   = offsetof(Vertex, pos);

        a[1].binding  = 0;
        a[1].location = 1;
        a[1].format   = VK_FORMAT_R32G32_SFLOAT;
        a[1].offset   = offsetof(Vertex, uv);

        a[2].binding  = 0;
        a[2].location = 2;
        a[2].format   = VK_FORMAT_R32G32B32A32_UINT;
        a[2].offset   = offsetof(Vertex, joints);

        a[3].binding  = 0;
        a[3].location = 3;
        a[3].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        a[3].offset   = offsetof(Vertex, weights);
        return a;
    }
};

}  // namespace vigil
