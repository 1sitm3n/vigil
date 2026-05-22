#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace vigil {

struct Skeleton;

enum class AnimPath : uint8_t {
    Translation,
    Rotation,
    Scale,
};

// One animation channel: keyframes targeting a specific joint's specific TRS
// component. `times` is sorted ascending. For Translation/Scale, values_vec3
// is populated; for Rotation, values_quat is populated. The other vector is
// empty and ignored.
struct AnimChannel {
    int32_t                target_joint = -1;   // skeleton-local index; -1 = unresolved
    AnimPath               path         = AnimPath::Translation;
    std::vector<float>     times;
    std::vector<glm::vec3> values_vec3;         // T or S
    std::vector<glm::quat> values_quat;         // R
};

struct Animation {
    std::string              name;
    float                    duration = 0.0f;   // seconds, max of all channel end times
    std::vector<AnimChannel> channels;

    bool empty() const { return channels.empty() || duration <= 0.0f; }
};

// Loads the first animation from a glTF .glb and maps each channel's target
// node onto the given target skeleton's joints, matched by name. Channels
// whose target can't be resolved are dropped (logged). STEP and CUBICSPLINE
// interpolation are treated as LINEAR — Mixamo only emits LINEAR anyway.
Animation load_animation(const std::string& path, const Skeleton& target_skeleton);

}  // namespace vigil
