#include "render/Mesh.h"
#include "render/Vertex.h"
#include "core/VulkanContext.h"

#include <cgltf.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace vigil {

namespace {

void node_to_trs(const cgltf_node* node,
                 glm::vec3& out_t, glm::quat& out_r, glm::vec3& out_s) {
    out_t = glm::vec3(0.0f);
    out_r = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    out_s = glm::vec3(1.0f);

    if (node->has_matrix) {
        std::printf("[Skeleton] node uses matrix form, TRS extraction not implemented; falling back to identity\n");
        return;
    }
    if (node->has_translation) {
        out_t = glm::vec3(node->translation[0], node->translation[1], node->translation[2]);
    }
    if (node->has_rotation) {
        out_r = glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);
    }
    if (node->has_scale) {
        out_s = glm::vec3(node->scale[0], node->scale[1], node->scale[2]);
    }
}

const cgltf_skin* find_skin_for_mesh(const cgltf_data* data, const cgltf_mesh* target) {
    for (cgltf_size i = 0; i < data->nodes_count; ++i) {
        if (data->nodes[i].mesh == target && data->nodes[i].skin) {
            return data->nodes[i].skin;
        }
    }
    return nullptr;
}

}  // namespace

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

    const cgltf_mesh*      gltf_mesh = &data->meshes[0];
    const cgltf_primitive* prim      = &gltf_mesh->primitives[0];

    const cgltf_accessor* pos_accessor     = nullptr;
    const cgltf_accessor* uv_accessor      = nullptr;
    const cgltf_accessor* joints_accessor  = nullptr;
    const cgltf_accessor* weights_accessor = nullptr;
    for (cgltf_size a = 0; a < prim->attributes_count; ++a) {
        const cgltf_attribute& attr = prim->attributes[a];
        switch (attr.type) {
            case cgltf_attribute_type_position: pos_accessor     = attr.data; break;
            case cgltf_attribute_type_texcoord: if (attr.index == 0) uv_accessor      = attr.data; break;
            case cgltf_attribute_type_joints:   if (attr.index == 0) joints_accessor  = attr.data; break;
            case cgltf_attribute_type_weights:  if (attr.index == 0) weights_accessor = attr.data; break;
            default: break;
        }
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

    const bool has_skin_attrs = joints_accessor && weights_accessor
                             && joints_accessor->count  == vertex_count
                             && weights_accessor->count == vertex_count;
    const cgltf_skin* skin = has_skin_attrs ? find_skin_for_mesh(data, gltf_mesh) : nullptr;

    std::vector<glm::uvec4> v_joints (vertex_count, glm::uvec4(0u));
    std::vector<glm::vec4>  v_weights(vertex_count, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
    if (skin) {
        for (cgltf_size i = 0; i < vertex_count; ++i) {
            cgltf_uint  ju[4] = {0u, 0u, 0u, 0u};
            cgltf_float wf[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            cgltf_accessor_read_uint (joints_accessor,  i, ju, 4);
            cgltf_accessor_read_float(weights_accessor, i, wf, 4);
            v_joints [i] = glm::uvec4(ju[0], ju[1], ju[2], ju[3]);
            v_weights[i] = glm::vec4 (wf[0], wf[1], wf[2], wf[3]);
        }
    }

    std::vector<Vertex> vertices(vertex_count);
    for (cgltf_size i = 0; i < vertex_count; ++i) {
        vertices[i].pos     = glm::vec3(positions[i*3 + 0], positions[i*3 + 1], positions[i*3 + 2]);
        vertices[i].uv      = glm::vec2(uvs[i*2 + 0],       uvs[i*2 + 1]);
        vertices[i].joints  = v_joints[i];
        vertices[i].weights = v_weights[i];
    }

    const cgltf_size n = prim->indices->count;
    std::vector<uint32_t> indices(n);
    for (cgltf_size i = 0; i < n; ++i) {
        indices[i] = static_cast<uint32_t>(cgltf_accessor_read_index(prim->indices, i));
    }
    index_count_ = static_cast<uint32_t>(n);

    if (skin) {
        const cgltf_size joints_n = skin->joints_count;
        skeleton_.joint_count = static_cast<uint32_t>(joints_n);

        skeleton_.inverse_bind_matrices.assign(joints_n, glm::mat4(1.0f));
        if (skin->inverse_bind_matrices) {
            std::vector<float> ibm_floats(joints_n * 16);
            cgltf_accessor_unpack_floats(skin->inverse_bind_matrices,
                                         ibm_floats.data(), joints_n * 16);
            for (cgltf_size i = 0; i < joints_n; ++i) {
                std::memcpy(glm::value_ptr(skeleton_.inverse_bind_matrices[i]),
                            &ibm_floats[i * 16], sizeof(float) * 16);
            }
        }

        std::unordered_map<const cgltf_node*, int32_t> joint_lookup;
        joint_lookup.reserve(joints_n);
        for (cgltf_size i = 0; i < joints_n; ++i) {
            joint_lookup[skin->joints[i]] = static_cast<int32_t>(i);
        }

        skeleton_.joint_names      .assign(joints_n, std::string());
        skeleton_.rest_translation .assign(joints_n, glm::vec3(0.0f));
        skeleton_.rest_rotation    .assign(joints_n, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        skeleton_.rest_scale       .assign(joints_n, glm::vec3(1.0f));
        skeleton_.parent_indices   .assign(joints_n, -1);

        for (cgltf_size i = 0; i < joints_n; ++i) {
            const cgltf_node* node = skin->joints[i];
            skeleton_.joint_names[i] = node->name ? node->name : "";
            node_to_trs(node,
                        skeleton_.rest_translation[i],
                        skeleton_.rest_rotation[i],
                        skeleton_.rest_scale[i]);
            if (node->parent) {
                auto it = joint_lookup.find(node->parent);
                if (it != joint_lookup.end()) {
                    skeleton_.parent_indices[i] = it->second;
                }
            }
        }
    }

    std::printf("[Mesh] %s: %zu vertices, %zu indices, UVs %s, skin %s",
                path.c_str(),
                static_cast<size_t>(vertex_count),
                static_cast<size_t>(n),
                has_uvs ? "present" : "missing (using zeros)",
                skeleton_.empty() ? "absent" : "present");
    if (!skeleton_.empty()) {
        std::printf(" (%u joints)", skeleton_.joint_count);
    }
    std::printf("\n");
    if (pos_accessor->has_min && pos_accessor->has_max) {
        std::printf("[Mesh] bounds: min (%.3f %.3f %.3f) max (%.3f %.3f %.3f)\n",
                    pos_accessor->min[0], pos_accessor->min[1], pos_accessor->min[2],
                    pos_accessor->max[0], pos_accessor->max[1], pos_accessor->max[2]);
    }

    cgltf_free(data);

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
