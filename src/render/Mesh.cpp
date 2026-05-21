#include "render/Mesh.h"
#include "render/Vertex.h"
#include "core/VulkanContext.h"

#include <cgltf.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace vigil {

Mesh::Mesh(VulkanContext& vk, const std::string& path) {
    cgltf_options options{};
    cgltf_data*   data = nullptr;

    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
    if (result != cgltf_result_success) {
        throw std::runtime_error("Mesh: cgltf_parse_file failed for " + path);
    }

    result = cgltf_load_buffers(&options, data, path.c_str());
    if (result != cgltf_result_success) {
        cgltf_free(data);
        throw std::runtime_error("Mesh: cgltf_load_buffers failed for " + path);
    }

    if (data->meshes_count == 0 || data->meshes[0].primitives_count == 0) {
        cgltf_free(data);
        throw std::runtime_error("Mesh: glTF has no mesh primitives: " + path);
    }

    const cgltf_primitive* prim = &data->meshes[0].primitives[0];

    const cgltf_accessor* pos_accessor = nullptr;
    const cgltf_accessor* uv_accessor  = nullptr;
    for (cgltf_size a = 0; a < prim->attributes_count; ++a) {
        const cgltf_attribute& attr = prim->attributes[a];
        if (attr.type == cgltf_attribute_type_position) pos_accessor = attr.data;
        else if (attr.type == cgltf_attribute_type_texcoord) uv_accessor = attr.data;
    }

    if (!pos_accessor) {
        cgltf_free(data);
        throw std::runtime_error("Mesh: primitive has no POSITION attribute");
    }
    if (!prim->indices) {
        cgltf_free(data);
        throw std::runtime_error("Mesh: primitive is non-indexed (not supported yet)");
    }

    const cgltf_size vertex_count = pos_accessor->count;

    std::vector<float> positions(vertex_count * 3);
    cgltf_accessor_unpack_floats(pos_accessor, positions.data(), vertex_count * 3);

    std::vector<float> uvs(vertex_count * 2, 0.0f);
    const bool has_uvs = uv_accessor && uv_accessor->count == vertex_count;
    if (has_uvs) {
        cgltf_accessor_unpack_floats(uv_accessor, uvs.data(), vertex_count * 2);
    }

    // Interleave into Vertex array.
    std::vector<Vertex> vertices(vertex_count);
    for (cgltf_size i = 0; i < vertex_count; ++i) {
        vertices[i].pos = glm::vec3(positions[i*3 + 0], positions[i*3 + 1], positions[i*3 + 2]);
        vertices[i].uv  = glm::vec2(uvs[i*2 + 0], uvs[i*2 + 1]);
    }

    // Indices, normalised to uint32 regardless of source component type.
    const cgltf_size n = prim->indices->count;
    std::vector<uint32_t> indices(n);
    for (cgltf_size i = 0; i < n; ++i) {
        indices[i] = static_cast<uint32_t>(cgltf_accessor_read_index(prim->indices, i));
    }
    index_count_ = static_cast<uint32_t>(n);

    // Log basic stats so we can see what was loaded.
    std::printf("[Mesh] %s: %zu vertices, %zu indices, UVs %s\n",
                path.c_str(),
                static_cast<size_t>(vertex_count),
                static_cast<size_t>(n),
                has_uvs ? "present" : "missing (using zeros)");
    if (pos_accessor->has_min && pos_accessor->has_max) {
        std::printf("[Mesh] bounds: min (%.3f %.3f %.3f) max (%.3f %.3f %.3f)\n",
                    pos_accessor->min[0], pos_accessor->min[1], pos_accessor->min[2],
                    pos_accessor->max[0], pos_accessor->max[1], pos_accessor->max[2]);
    }

    cgltf_free(data);

    // Upload to GPU (host-visible for now; staging path comes later).
    const VkDeviceSize vb_size = sizeof(Vertex) * vertices.size();
    vertex_buffer_ = Buffer(
        vk, vb_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    vertex_buffer_.upload(vertices.data(), vb_size);

    const VkDeviceSize ib_size = sizeof(uint32_t) * indices.size();
    index_buffer_ = Buffer(
        vk, ib_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    index_buffer_.upload(indices.data(), ib_size);
}

}  // namespace vigil
