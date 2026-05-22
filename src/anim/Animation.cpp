#include "anim/Animation.h"
#include "render/Skeleton.h"

#include <cgltf.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace vigil {

namespace {

std::unordered_map<std::string, int32_t> build_joint_lookup(const Skeleton& s) {
    std::unordered_map<std::string, int32_t> m;
    m.reserve(s.joint_count);
    for (uint32_t i = 0; i < s.joint_count; ++i) {
        m[s.joint_names[i]] = static_cast<int32_t>(i);
    }
    return m;
}

// FBX2glTF sometimes keeps the "mixamorig:" prefix on node names, sometimes
// strips it. Try direct match first, then with/without the prefix.
int32_t resolve_joint(const std::unordered_map<std::string, int32_t>& lookup,
                      const char* name) {
    if (!name) return -1;
    const std::string s(name);
    auto it = lookup.find(s);
    if (it != lookup.end()) return it->second;

    static const std::string kPrefix = "mixamorig:";
    if (s.rfind(kPrefix, 0) == 0) {
        it = lookup.find(s.substr(kPrefix.size()));
    } else {
        it = lookup.find(kPrefix + s);
    }
    return it != lookup.end() ? it->second : -1;
}

}  // namespace

Animation load_animation(const std::string& path, const Skeleton& target_skeleton) {
    cgltf_options options{};
    cgltf_data*   data = nullptr;

    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success) {
        throw std::runtime_error("Animation: cgltf_parse_file failed for " + path);
    }
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {
        cgltf_free(data);
        throw std::runtime_error("Animation: cgltf_load_buffers failed for " + path);
    }
    if (data->animations_count == 0) {
        cgltf_free(data);
        throw std::runtime_error("Animation: no animations in " + path);
    }

    const cgltf_animation* src = &data->animations[0];
    Animation out;
    out.name = src->name ? src->name : path;
    out.channels.reserve(src->channels_count);

    const auto lookup = build_joint_lookup(target_skeleton);
    int unresolved = 0;

    for (cgltf_size ci = 0; ci < src->channels_count; ++ci) {
        const cgltf_animation_channel& src_chan = src->channels[ci];
        if (!src_chan.target_node || !src_chan.sampler) continue;

        const int32_t joint = resolve_joint(lookup, src_chan.target_node->name);
        if (joint < 0) {
            ++unresolved;
            continue;
        }

        AnimChannel ch;
        ch.target_joint = joint;
        switch (src_chan.target_path) {
            case cgltf_animation_path_type_translation: ch.path = AnimPath::Translation; break;
            case cgltf_animation_path_type_rotation:    ch.path = AnimPath::Rotation;    break;
            case cgltf_animation_path_type_scale:       ch.path = AnimPath::Scale;       break;
            default: continue;  // morph weights and friends not supported
        }

        const cgltf_animation_sampler* samp = src_chan.sampler;
        if (samp->interpolation != cgltf_interpolation_type_linear) {
            std::printf("[Animation] %s: non-linear interpolation on joint %d, treating as LINEAR\n",
                        out.name.c_str(), joint);
        }

        const cgltf_size n_keys = samp->input->count;
        ch.times.resize(n_keys);
        cgltf_accessor_unpack_floats(samp->input, ch.times.data(), n_keys);

        const cgltf_size n_values = samp->output->count;
        if (ch.path == AnimPath::Rotation) {
            std::vector<float> raw(n_values * 4);
            cgltf_accessor_unpack_floats(samp->output, raw.data(), n_values * 4);
            ch.values_quat.resize(n_values);
            for (cgltf_size k = 0; k < n_values; ++k) {
                ch.values_quat[k] = glm::quat(raw[k*4 + 3], raw[k*4 + 0], raw[k*4 + 1], raw[k*4 + 2]);
            }
        } else {
            std::vector<float> raw(n_values * 3);
            cgltf_accessor_unpack_floats(samp->output, raw.data(), n_values * 3);
            ch.values_vec3.resize(n_values);
            for (cgltf_size k = 0; k < n_values; ++k) {
                ch.values_vec3[k] = glm::vec3(raw[k*3 + 0], raw[k*3 + 1], raw[k*3 + 2]);
            }
        }

        if (!ch.times.empty()) {
            out.duration = std::max(out.duration, ch.times.back());
        }
        out.channels.push_back(std::move(ch));
    }

    std::printf("[Animation] %s: %zu channels kept, %d unresolvable target nodes, duration %.3fs\n",
                path.c_str(), out.channels.size(), unresolved, out.duration);

    cgltf_free(data);
    return out;
}

}  // namespace vigil
