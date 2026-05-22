#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace vigil {

// Per-mesh skeleton data extracted from a glTF `skin`.
//
// Joint ordering matches `skin->joints` in the source file — also the space
// the vertex JOINTS_0 attribute indexes into. Topologically sorted (parents
// before children) by Mixamo + FBX2glTF, which the FK walk in the Animator
// relies on.
//
// Rest pose stored as TRS rather than a composed mat4 so the Animator can
// override any single TRS component independently when an animation channel
// targets only that component. Missing channels fall back to the rest value
// for that component.
struct Skeleton {
    uint32_t                 joint_count = 0;
    std::vector<std::string> joint_names;            // for animation -> joint mapping
    std::vector<glm::mat4>   inverse_bind_matrices;
    std::vector<glm::vec3>   rest_translation;
    std::vector<glm::quat>   rest_rotation;
    std::vector<glm::vec3>   rest_scale;
    std::vector<int32_t>     parent_indices;         // -1 if outside this skin

    bool empty() const { return joint_count == 0; }
};

}  // namespace vigil
