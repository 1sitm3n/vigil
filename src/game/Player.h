#pragma once

#include "core/Window.h"        // InputFrame
#include "game/PlayerState.h"
#include "game/Stamina.h"

#include <glm/glm.hpp>

namespace vigil {

class Camera;

// Owns position, facing yaw, velocity, and the PlayerState SM. Day-11
// replacement for main.cpp's previous direct PlayerState ownership. The SM
// stays in charge of *what* the player is doing; this class is *where* and
// *which way* he's pointing.
//
// Locomotion is camera-relative WASD: input vector built from Camera's
// forward_xz / right_xz basis, rotated implicitly by yaw. Phase 1's "yaw + pi"
// bug doesn't recur because the basis is consumed signed, not re-derived.
//
// Roll uses code-driven 4m forward at 8 m/s (GDD §6) in the direction the
// player was facing at Roll entry (GDD v1.3 §6 #11: forward-only). The Roll
// animation slot is strip_root=true so the visual translation comes from
// world_transform(), not the bone palette — same integration path as Walk/Jog.
// This is a deliberate departure from the Day-10 handoff's "read animator
// root each frame" plan; see devlog Day 11 for the rationale.
//
// Sprint intent is tracked on the InputFrame but stamina drain is Day 13.
class Player {
public:
    Player() = default;

    // Drives SM (via PlayerInput), camera-relative locomotion, Roll motion,
    // and facing-direction slerp. Camera supplies forward_xz/right_xz.
    void update(float dt, const InputFrame& input, const Camera& camera);

    const glm::vec3&   position() const { return position_; }
    const glm::vec3&   velocity() const { return velocity_; }
    float              yaw()      const { return yaw_facing_; }   // radians
    const PlayerState& state()    const { return state_; }
    const Stamina&     stamina()  const { return stamina_; }

    // T(position) * Ry(yaw). Renderer pushes this through MVP, replacing the
    // identity matrix that lived in record_command_buffer through Day 10.
    glm::mat4 world_transform() const;

    // Day 12: damage system hook. main.cpp's hit test calls this once per
    // attack instance when the cone+reach check lands on a target. The flag
    // it sets resets on the next transition_to(Attack*), so combo chains
    // each get a fresh hit. Cone state and target list live OUTSIDE Player
    // by design — Player owns "am I attacking" but not "what's in range."
    void register_hit() { state_.mark_attack_landed(); }

    // Tunables — GDD §6 canonical.
    static constexpr float WALK_SPEED   = 3.0f;   // m/s
    static constexpr float SPRINT_SPEED = 6.0f;
    static constexpr float ROLL_SPEED   = 8.0f;   // 4m / 0.5s
    static constexpr float FACE_SMOOTH  = 0.15f;  // seconds to align facing

private:
    PlayerState state_;
    Stamina     stamina_;
    glm::vec3   position_   { 0.0f, 0.0f, 0.0f };
    glm::vec3   velocity_   { 0.0f, 0.0f, 0.0f };
    // Day 12: this model faces +Z at yaw=0 (NOT Mixamo's standard -Z —
    // the Tripo silhouette or the Blender pass on Day 6 left it pointed
    // the opposite way). Hypothesis from Day 11 confirmed by the
    // dummy-as-reference test on Day 12. Re-test once the Week-4
    // Blender re-upload corrects the asset orientation in-source.
    float       yaw_facing_ = 0.0f;   // radians; 0 = facing +Z (model rest)

    // Latched on Roll entry. Forward = +Z at yaw_facing_ = 0.
    glm::vec3 roll_dir_ { 0.0f, 0.0f, 1.0f };
};

}  // namespace vigil
