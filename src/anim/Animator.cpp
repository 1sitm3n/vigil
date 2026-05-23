#include "anim/Animator.h"
#include "render/Skeleton.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
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

// Fill TRS arrays with rest pose, then override channels at time `t`.
// strip_root suppresses the root joint's Translation channel.
void sample_pose(const Animation& a, float t, const Skeleton& sk, bool strip_root,
                 std::vector<glm::vec3>& tr,
                 std::vector<glm::quat>& rt,
                 std::vector<glm::vec3>& sc) {
    const uint32_t N = sk.joint_count;
    for (uint32_t i = 0; i < N; ++i) {
        tr[i] = sk.rest_translation[i];
        rt[i] = sk.rest_rotation[i];
        sc[i] = sk.rest_scale[i];
    }

    for (const auto& ch : a.channels) {
        if (ch.target_joint < 0 || static_cast<uint32_t>(ch.target_joint) >= N) continue;
        if (ch.times.empty()) continue;
        if (strip_root && ch.target_joint == 0 && ch.path == AnimPath::Translation) continue;

        const Segment seg = find_segment(ch.times, t);
        const uint32_t j  = static_cast<uint32_t>(ch.target_joint);

        switch (ch.path) {
            case AnimPath::Translation:
                if (ch.values_vec3.size() > seg.k + 1) {
                    tr[j] = glm::mix(ch.values_vec3[seg.k], ch.values_vec3[seg.k + 1], seg.alpha);
                } else if (!ch.values_vec3.empty()) {
                    tr[j] = ch.values_vec3.back();
                }
                break;
            case AnimPath::Rotation:
                if (ch.values_quat.size() > seg.k + 1) {
                    rt[j] = glm::slerp(ch.values_quat[seg.k], ch.values_quat[seg.k + 1], seg.alpha);
                } else if (!ch.values_quat.empty()) {
                    rt[j] = ch.values_quat.back();
                }
                break;
            case AnimPath::Scale:
                if (ch.values_vec3.size() > seg.k + 1) {
                    sc[j] = glm::mix(ch.values_vec3[seg.k], ch.values_vec3[seg.k + 1], seg.alpha);
                } else if (!ch.values_vec3.empty()) {
                    sc[j] = ch.values_vec3.back();
                }
                break;
        }
    }
}

}  // namespace

void Animator::play(const Animation& anim, float fade, float speed, bool strip_root) {
    // First animation ever — snap into current, no blend.
    if (!current_) {
        current_           = &anim;
        current_time_      = 0.0f;
        current_speed_     = speed;
        current_strip_root_ = strip_root;
        target_            = nullptr;
        blend_t_           = 1.0f;
        return;
    }

    // Same animation as what's already playing/queued — no-op.
    if (&anim == current_ && !target_) return;
    if (&anim == target_) return;

    // Begin a fresh blend toward the new target.
    // NOTE: if a blend was already in progress, this restarts from current_
    // alone, not the live blended pose. 0.2s windows + sane combat input
    // make this rare in practice; revisit if a real pop shows up.
    target_            = &anim;
    target_time_       = 0.0f;
    target_speed_      = speed;
    target_strip_root_ = strip_root;
    blend_duration_    = std::max(fade, 1e-3f);
    blend_t_           = (fade <= 0.0f) ? 1.0f : 0.0f;

    // fade <= 0 means immediate adoption.
    if (blend_t_ >= 1.0f) {
        current_           = target_;
        current_time_      = target_time_;
        current_speed_     = target_speed_;
        current_strip_root_ = target_strip_root_;
        target_            = nullptr;
    }
}

void Animator::update(float dt) {
    if (current_ && current_->duration > 0.0f) {
        current_time_ = std::fmod(current_time_ + dt * current_speed_, current_->duration);
        if (current_time_ < 0.0f) current_time_ += current_->duration;
    }
    if (!target_) return;

    if (target_->duration > 0.0f) {
        target_time_ = std::fmod(target_time_ + dt * target_speed_, target_->duration);
        if (target_time_ < 0.0f) target_time_ += target_->duration;
    }

    blend_t_ += dt / blend_duration_;
    if (blend_t_ >= 1.0f) {
        current_            = target_;
        current_time_       = target_time_;
        current_speed_      = target_speed_;
        current_strip_root_ = target_strip_root_;
        target_             = nullptr;
        blend_t_            = 1.0f;
    }
}

void Animator::compute_bone_palette(glm::mat4* out_palette) const {
    if (!skeleton_ || skeleton_->joint_count == 0) return;
    const uint32_t N = skeleton_->joint_count;

    // 1. Sample current pose. No animation? Fall back to rest.
    std::vector<glm::vec3> t(N), s(N);
    std::vector<glm::quat> r(N);
    if (current_) {
        sample_pose(*current_, current_time_, *skeleton_, current_strip_root_, t, r, s);
    } else {
        for (uint32_t i = 0; i < N; ++i) {
            t[i] = skeleton_->rest_translation[i];
            r[i] = skeleton_->rest_rotation[i];
            s[i] = skeleton_->rest_scale[i];
        }
    }

    // 2. If blending, sample target and blend per-channel TRS.
    //    Matrix lerp breaks under rotation — split-TRS Skeleton (Day 8)
    //    is what makes this safe.
    if (target_) {
        std::vector<glm::vec3> tb(N), sb(N);
        std::vector<glm::quat> rb(N);
        sample_pose(*target_, target_time_, *skeleton_, target_strip_root_, tb, rb, sb);
        const float a = std::clamp(blend_t_, 0.0f, 1.0f);
        for (uint32_t i = 0; i < N; ++i) {
            t[i] = glm::mix(t[i], tb[i], a);
            r[i] = glm::slerp(r[i], rb[i], a);
            s[i] = glm::mix(s[i], sb[i], a);
        }
    }

    // 3. FK — memoised recursion. Day 8 fix preserved: Mixamo's skin->joints
    //    is non-topological (Spine2 at index 4, parent Spine1 at index 27).
    //    A forward pass reads uninit world[parent]; recursion + computed[]
    //    guarantees parents resolve first. Max depth ~11, stack-safe.
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
