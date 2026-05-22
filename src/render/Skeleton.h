#pragma once

#include <glm/mat4x4.hpp>

#include <cstdint>
#include <vector>

namespace vigil {

// Per-mesh skeleton data extracted from a glTF `skin`.
//
// Joint ordering matches `skin->joints` in the source file — this is also the
// space the vertex JOINTS_0 attribute indexes into, so no remap is needed when
// uploading the bone palette UBO.
//
// For each joint i:
//   * inverse_bind_matrices[i] — transforms from model space into joint-local
//     space at bind pose. Pre-baked, never mutated.
//   * local_rest_transforms[i] — TRS at rest, in the joint's parent's space.
//     Day 8 starts from this and applies sampled animation deltas.
//   * parent_indices[i] — index of joint i's parent within this same array,
//     or -1 if the parent is outside the skin (typically an Armature/scene
//     root node). Day 8 FK ignores -1 parents and treats their effective
//     world transform as identity for now; once root motion matters we'll
//     thread the skin root's world transform through here.
struct Skeleton {
    uint32_t               joint_count = 0;
    std::vector<glm::mat4> inverse_bind_matrices;
    std::vector<glm::mat4> local_rest_transforms;
    std::vector<int32_t>   parent_indices;

    bool empty() const { return joint_count == 0; }
};

}  // namespace vigil
