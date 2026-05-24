#include "game/Player.h"
#include "core/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace vigil {

namespace {

constexpr float kEpsilon = 1.0e-5f;
constexpr float kPi      = 3.14159265358979f;
constexpr float kTwoPi   = 6.28318530717958f;

// Shortest-arc lerp on yaw in radians. Wraps the difference into (-pi, pi]
// so a slerp from +170° to -170° goes the short way (20°), not the long way.
float lerp_yaw_shortest(float current, float target, float t) {
    float diff = std::fmod(target - current + kPi, kTwoPi);
    if (diff < 0.0f) diff += kTwoPi;
    diff -= kPi;
    return current + diff * t;
}

}  // namespace

void Player::update(float dt, const InputFrame& input, const Camera& camera) {
    // 1) Feed the SM. "move_forward" reads as "any locomotion input held" —
    //    SM only branches on the bool, not direction. Idle <-> Walk/Jog
    //    transitions fire on any of W/A/S/D so strafe-only input still
    //    enters Walk.
    //
    //    Day 13: stamina gates each input before the SM sees it.
    //    - Shift+LMB triggers Heavy (24 cost). LMB-alone triggers Light (12).
    //      Splitting at the input layer means Shift+LMB never spills into
    //      heavy + light + sprint simultaneously.
    //    - Sprint cuts off the moment stamina hits 0. SM sees !pin.sprint
    //      next tick and drops Jog -> Walk naturally.
    //    - Block has no entry cost (drain on hit lands Phase 4 with enemies).
    const bool wants_heavy = input.lmb_pressed &&  input.shift_held;
    const bool wants_light = input.lmb_pressed && !input.shift_held;

    PlayerInput pin;
    pin.move_forward = input.w_held || input.a_held || input.s_held || input.d_held;
    pin.sprint       = input.shift_held   && stamina_.available_for_sprint();
    pin.attack       = wants_light        && stamina_.available(Stamina::LIGHT_COST);
    pin.heavy_attack = wants_heavy        && stamina_.available(Stamina::HEAVY_COST);
    pin.dodge        = input.space_pressed && stamina_.available(Stamina::ROLL_COST);
    pin.block        = input.rmb_held;

    state_.update(dt, pin);

    // 1b) Charge stamina on entry into action states. transition_to() sets
    //     state_changed_; this latches off the same tick. Light/heavy
    //     deduct full cost up-front; the gate above guarantees we won't
    //     go negative.
    if (state_.state_changed()) {
        switch (state_.id()) {
            case PlayerStateId::Attack1:
            case PlayerStateId::Attack2:
            case PlayerStateId::Attack3: stamina_.drain(Stamina::LIGHT_COST); break;
            case PlayerStateId::Heavy:   stamina_.drain(Stamina::HEAVY_COST); break;
            case PlayerStateId::Roll:    stamina_.drain(Stamina::ROLL_COST);  break;
            default: break;
        }
    }

    // 1c) Tick stamina. GDD §7: regen 25/s except attack/block/sprint.
    //     Roll DOES regen (the 25 paid at entry is the cost; regen during
    //     the 0.5s roll claws back ~12.5).
    const PlayerStateId id = state_.id();
    const bool sprint_active = (id == PlayerStateId::Jog) && pin.sprint;
    const bool spending = sprint_active
                       || id == PlayerStateId::Attack1
                       || id == PlayerStateId::Attack2
                       || id == PlayerStateId::Attack3
                       || id == PlayerStateId::Heavy
                       || id == PlayerStateId::Block;
    if (sprint_active) stamina_.drain_rate(Stamina::SPRINT_DRAIN, dt);
    if (!spending)     stamina_.regen_rate(Stamina::REGEN_RATE, dt);

    // 2) Build world-space wish direction from camera basis. forward_xz and
    //    right_xz are already signed correctly — do NOT re-derive trig here.
    //    That is the Phase 1 yaw+pi bug, and it lives here too.
    const glm::vec3 fwd   = camera.forward_xz();
    const glm::vec3 right = camera.right_xz();
    // Day 12: textbook camera-relative signs. Day 11 flipped these from
    // textbook on a first-run "looks reversed" call made without a fixed
    // reference object. The Day 12 dummy at (3,0,-2) revealed the flip
    // was wrong (pos.z went +ve on W presses = moving toward camera).
    glm::vec3 wish_dir(0.0f);
    if (input.w_held) wish_dir += fwd;
    if (input.s_held) wish_dir -= fwd;
    if (input.d_held) wish_dir += right;
    if (input.a_held) wish_dir -= right;
    const float wish_mag = glm::length(wish_dir);
    if (wish_mag > kEpsilon) wish_dir /= wish_mag;

    // 3) Latch Roll direction the frame we entered Roll. If no input direction,
    //    roll along current facing (-Z at yaw=0).
    if (state_.id() == PlayerStateId::Roll && state_.state_changed()) {
        if (wish_mag > kEpsilon) {
            roll_dir_ = wish_dir;
        } else {
            // +Z-facing-model: forward = (sin(yaw), 0, cos(yaw)).
            roll_dir_ = glm::vec3(std::sin(yaw_facing_), 0.0f, std::cos(yaw_facing_));
        }
    }

    // 4) Velocity per state. Roll is the latched 8 m/s; locomotion uses live
    //    wish_dir; everything else holds position. Roll's animation slot is
    //    strip_root=true so the bone palette carries no translation —
    //    integrating velocity here is the single source of position truth.
    switch (state_.id()) {
        case PlayerStateId::Walk: velocity_ = wish_dir * WALK_SPEED;   break;
        case PlayerStateId::Jog:  velocity_ = wish_dir * SPRINT_SPEED; break;
        case PlayerStateId::Roll: velocity_ = roll_dir_ * ROLL_SPEED;  break;
        default:                  velocity_ = glm::vec3(0.0f);         break;
    }
    position_ += velocity_ * dt;

    // 5) Face the movement direction. Smooth-lerp over FACE_SMOOTH so
    //    strafe<->forward transitions don't pop. Attacks hold facing.
    //    Day 12: model rest = +Z forward (NOT Mixamo standard). Facing
    //    at yaw is (sin(yaw), 0, cos(yaw)); solving for yaw given a
    //    target dir gives atan2(dir.x, dir.z). Re-test if Week-4 Blender
    //    re-upload corrects the source orientation.
    glm::vec3 face_dir(0.0f);
    if (state_.id() == PlayerStateId::Roll) {
        face_dir = roll_dir_;
    } else if (wish_mag > kEpsilon &&
               (state_.id() == PlayerStateId::Walk ||
                state_.id() == PlayerStateId::Jog ||
                state_.id() == PlayerStateId::Idle)) {
        face_dir = wish_dir;
    }
    if (glm::length(face_dir) > kEpsilon) {
        const float target_yaw = std::atan2(face_dir.x, face_dir.z);
        const float alpha = std::clamp(dt / FACE_SMOOTH, 0.0f, 1.0f);
        yaw_facing_ = lerp_yaw_shortest(yaw_facing_, target_yaw, alpha);
    }
}

glm::mat4 Player::world_transform() const {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), position_);
    return glm::rotate(t, yaw_facing_, glm::vec3(0.0f, 1.0f, 0.0f));
}

}  // namespace vigil
