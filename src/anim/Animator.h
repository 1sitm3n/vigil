#pragma once

#include "anim/Animation.h"

#include <glm/glm.hpp>

namespace vigil {

struct Skeleton;

// Runs an Animation against a Skeleton: advances time, samples local TRS per
// joint, walks the parent chain (FK), and writes (world * inverseBind) into a
// caller-provided palette. Pure CPU; the Renderer owns the GPU-side buffer.
class Animator {
public:
    Animator() = default;

    void set_skeleton(const Skeleton& s)   { skeleton_  = &s; }
    void set_animation(const Animation& a) { animation_ = &a; playback_time_ = 0.0f; }

    void  update(float dt);                          // looping playback
    float playback_time() const { return playback_time_; }

    // Writes skeleton.joint_count mat4s into out_palette. Slots beyond
    // joint_count are left untouched (Renderer pre-fills with identity).
    void compute_bone_palette(glm::mat4* out_palette) const;

private:
    const Skeleton*  skeleton_      = nullptr;
    const Animation* animation_     = nullptr;
    float            playback_time_ = 0.0f;
};

}  // namespace vigil
