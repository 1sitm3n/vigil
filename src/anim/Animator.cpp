#include "anim/Animator.h"
#include "render/Skeleton.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <functional>
#include <vector>

namespace vigil {

namespace {

struct Segment { size_t k; float alpha; };

Segment find_segment(const std::vector<float>& times, float t) {
    if (times.size() <= 1)  return { 0, 0.0f };
    if (t <= times.front()) return { 0, 0.0f };
    if (t >= times.back())  return { times.size() - 2, 1.0f };

    for (size_t k = 0; k + 1 < times.size(); ++k) {
        if (t >= times[k] && t <= times[k + 1]) {
            const float dt = times[k + 1] - times[k];
            return { k, dt > 0.0f ? (t - times[k]) / dt : 0.0f };
        }
    }
    return { times.size() - 2, 1.0f };
}

}  // namespace

void Animator::update(float dt) {
    if (!animation_ || animation_->duration <= 0.0f) return;
    playback_time_ = std::fmod(playback_time_ + dt, animation_->duration);
    if (playback_time_ < 0.0f) playback_time_ += animation_->duration;
}

void Animator::compute_bone_palette(glm::mat4* out_palette) const {
    if (!skeleton_ || skeleton_->joint_count == 0) return;

    const uint32_t N = skeleton_->joint_count;

    // 1. Start from rest pose TRS for every joint.
    std::vector<glm::vec3> t(N);
    std::vector<glm::quat> r(N);
    std::vector<glm::vec3> s(N);
    for (uint32_t i = 0; i < N; ++i) {
        t[i] = skeleton_->rest_translation[i];
        r[i] = skeleton_->rest_rotation[i];
        s[i] = skeleton_->rest_scale[i];
    }

    // 2. Override sampled channels at the current playback time.
    if (animation_) {
        const float at = playback_time_;
        for (const auto& ch : animation_->channels) {
            if (ch.target_joint < 0 || static_cast<uint32_t>(ch.target_joint) >= N) continue;
            if (ch.times.empty()) continue;

            const Segment seg = find_segment(ch.times, at);
            const uint32_t j  = static_cast<uint32_t>(ch.target_joint);

            switch (ch.path) {
                case AnimPath::Translation:
                    if (ch.values_vec3.size() > seg.k + 1) {
                        t[j] = glm::mix(ch.values_vec3[seg.k], ch.values_vec3[seg.k + 1], seg.alpha);
                    } else if (!ch.values_vec3.empty()) {
                        t[j] = ch.values_vec3.back();
                    }
                    break;
                case AnimPath::Rotation:
                    if (ch.values_quat.size() > seg.k + 1) {
                        r[j] = glm::slerp(ch.values_quat[seg.k], ch.values_quat[seg.k + 1], seg.alpha);
                    } else if (!ch.values_quat.empty()) {
                        r[j] = ch.values_quat.back();
                    }
                    break;
                case AnimPath::Scale:
                    if (ch.values_vec3.size() > seg.k + 1) {
                        s[j] = glm::mix(ch.values_vec3[seg.k], ch.values_vec3[seg.k + 1], seg.alpha);
                    } else if (!ch.values_vec3.empty()) {
                        s[j] = ch.values_vec3.back();
                    }
                    break;
            }
        }
    }

    // 3. Forward kinematics — recursive so it doesn't depend on joint ordering
    //    in skin->joints. Topologically sorted skins (Mixamo's typical output)
    //    still hit each joint exactly once via the early-return on computed[].
    std::vector<glm::mat4> world(N);
    std::vector<uint8_t>   computed(N, 0);
    std::function<void(uint32_t)> walk = [&](uint32_t i) {
        if (computed[i]) return;
        const int32_t parent = skeleton_->parent_indices[i];
        if (parent >= 0) walk(static_cast<uint32_t>(parent));
        const glm::mat4 local = glm::translate(glm::mat4(1.0f), t[i])
                              * glm::mat4_cast(r[i])
                              * glm::scale(glm::mat4(1.0f), s[i]);
        world[i] = (parent >= 0) ? world[parent] * local : local;
        computed[i] = 1;
    };
    for (uint32_t i = 0; i < N; ++i) walk(i);

    // 4. Final palette: world * IBM (matches the shader's skinning formula).
    for (uint32_t i = 0; i < N; ++i) {
        out_palette[i] = world[i] * skeleton_->inverse_bind_matrices[i];
    }
}

}  // namespace vigil
