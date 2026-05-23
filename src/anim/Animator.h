#pragma once

#include "anim/Animation.h"

#include <glm/glm.hpp>

namespace vigil {

struct Skeleton;

// Runs Animations against a Skeleton with crossfading support.
//
// Crossfade: play(new_anim, fade) makes new_anim the "target" while the
// previous "current" continues. blend_t_ ramps 0 -> 1 over `fade` seconds,
// blending per-channel TRS (lerp t/s, slerp r). At completion the target
// becomes current and target is cleared.
//
// Per-animation knobs:
//   playback_speed     1.0 = native; >1.0 speeds up. Used to compress
//                      roll_forward.glb's ~1.27s native to the GDD 0.5s
//                      target via speed ~= 2.54.
//   strip_root_motion  zero the root joint's Translation channel during
//                      sampling. True for locomotion + combat (player owns
//                      position; animation owns posture). False for the roll
//                      where the 4m forward distance comes from the anim.
//
// FK is order-independent via memoised recursion (Day 8 fix preserved) —
// Mixamo's skin->joints is non-topological so the walk resolves parents
// before children unconditionally.
class Animator {
public:
    Animator() = default;

    void set_skeleton(const Skeleton& s) { skeleton_ = &s; }

    // Crossfade to a new animation over `fade_duration` seconds.
    // fade_duration <= 0 snaps without blending.
    void play(const Animation& anim,
              float fade_duration     = 0.2f,
              float playback_speed    = 1.0f,
              bool  strip_root_motion = true);

    // Backwards-compat single-anim setter — snaps with no blend.
    void set_animation(const Animation& a) { play(a, 0.0f, 1.0f, true); }

    void  update(float dt);
    float playback_time() const { return current_time_; }
    bool  is_blending()   const { return target_ != nullptr; }
    float blend_t()       const { return blend_t_; }  // 0..1 over active crossfade

    void compute_bone_palette(glm::mat4* out_palette) const;

private:
    const Skeleton* skeleton_ = nullptr;

    const Animation* current_     = nullptr;
    float current_time_           = 0.0f;
    float current_speed_          = 1.0f;
    bool  current_strip_root_     = true;

    const Animation* target_      = nullptr;
    float target_time_            = 0.0f;
    float target_speed_           = 1.0f;
    bool  target_strip_root_      = true;

    float blend_duration_         = 0.2f;
    float blend_t_                = 1.0f;  // 1.0 = no active blend
};

}  // namespace vigil
