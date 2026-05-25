#pragma once

#include "core/Window.h"        // InputFrame
#include "game/PlayerState.h"
#include "game/Stamina.h"

#include <glm/glm.hpp>

namespace vigil {

class Camera;

// Owns position, facing yaw, velocity, and the PlayerState SM. Day-11
// replacement for main.cpp's previous direct PlayerState ownership.
//
// Day 14: also owns parry window state. The window decays each tick;
// an RMB edge press refreshes it to PARRY_WINDOW. main.cpp orders the
// per-frame work explicitly:
//
//   1) player.tick_parry_window(dt, input)   // window decay + edge refresh
//   2) [if fake-hit fires] player.try_parry() or absorb_block_hit() or print
//   3) player.update(dt, input, camera)      // SM consumes riposte_pending_
//
// The split lets damage resolution see a current window value AND lets the
// SM consume riposte_pending_ in the same frame as the parry — no 1-frame
// latency. Phase 4 enemies will replace main.cpp's fake-hit timer with the
// real damage event but the contract here stays identical.
class Player {
public:
    Player() = default;

    // Day 14: tick the parry window. Call BEFORE damage resolution AND
    // BEFORE update() each frame.
    void tick_parry_window(float dt, const InputFrame& input);

    // Day 14: incoming-damage hooks. Both called by main.cpp when the
    // fake-hit timer fires (Phase 4 enemies will be the real callers).
    //
    // try_parry: returns true if parry window is open. Consumes the window
    //            and sets riposte_pending_ which next update() feeds to the
    //            SM as pin.riposte. No stamina cost.
    // absorb_block_hit: drain 50% of damage from stamina (GDD §6).
    bool try_parry();
    void absorb_block_hit(float damage);

    // Drives SM, camera-relative locomotion, Roll motion, facing-slerp.
    void update(float dt, const InputFrame& input, const Camera& camera);

    const glm::vec3&   position() const { return position_; }
    const glm::vec3&   velocity() const { return velocity_; }
    float              yaw()      const { return yaw_facing_; }   // radians
    const PlayerState& state()    const { return state_; }
    const Stamina&     stamina()  const { return stamina_; }
    float              parry_window_remaining() const { return parry_window_remaining_; }

    glm::mat4 world_transform() const;

    void register_hit() { state_.mark_attack_landed(); }

    // Tunables — GDD §6 canonical.
    static constexpr float WALK_SPEED   = 3.0f;   // m/s
    static constexpr float SPRINT_SPEED = 6.0f;
    static constexpr float ROLL_SPEED   = 8.0f;   // 4m / 0.5s
    static constexpr float FACE_SMOOTH  = 0.15f;  // seconds to align facing
    static constexpr float PARRY_WINDOW = 0.15f;  // Day 14: parry pre-hit window

private:
    PlayerState state_;
    Stamina     stamina_;
    glm::vec3   position_   { 0.0f, 0.0f, 0.0f };
    glm::vec3   velocity_   { 0.0f, 0.0f, 0.0f };
    // Day 12: model faces +Z at yaw=0 (NOT Mixamo's standard -Z — Tripo
    // silhouette or Blender pass left it pointed the opposite way).
    float       yaw_facing_ = 0.0f;   // radians; 0 = facing +Z

    glm::vec3 roll_dir_ { 0.0f, 0.0f, 1.0f };

    // Day 14 parry state.
    float parry_window_remaining_ = 0.0f;
    bool  riposte_pending_        = false;
};

}  // namespace vigil
