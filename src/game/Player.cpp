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
    PlayerInput pin;
    pin.move_forward = input.w_held || input.a_held || input.s_held || input.d_held;
    pin.sprint       = input.shift_held;   // tracked; stamina drain lands Day 13
    pin.attack       = input.lmb_pressed;
    pin.dodge        = input.space_pressed;

    state_.update(dt, pin);

    // 2) Build world-space wish direction from camera basis. forward_xz and
    //    right_xz are already signed correctly — do NOT re-derive trig here.
    //    That is the Phase 1 yaw+pi bug, and it lives here too.
    const glm::vec3 fwd   = camera.forward_xz();
    const glm::vec3 right = camera.right_xz();
    glm::vec3 wish_dir(0.0f);
    if (input.w_held) wish_dir -= fwd;
    if (input.s_held) wish_dir += fwd;
    if (input.d_held) wish_dir -= right;
    if (input.a_held) wish_dir += right;
    const float wish_mag = glm::length(wish_dir);
    if (wish_mag > kEpsilon) wish_dir /= wish_mag;

    // 3) Latch Roll direction the frame we entered Roll. If no input direction,
    //    roll along current facing (-Z at yaw=0).
    if (state_.id() == PlayerStateId::Roll && state_.state_changed()) {
        if (wish_mag > kEpsilon) {
            roll_dir_ = wish_dir;
        } else {
            roll_dir_ = glm::vec3(-std::sin(yaw_facing_), 0.0f, -std::cos(yaw_facing_));
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
    //    Mixamo forward = -Z, so dir = (-sin(yaw), 0, -cos(yaw)) and yaw =
    //    atan2(-dir.x, -dir.z).
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
        const float target_yaw = std::atan2(-face_dir.x, -face_dir.z);
        const float alpha = std::clamp(dt / FACE_SMOOTH, 0.0f, 1.0f);
        yaw_facing_ = lerp_yaw_shortest(yaw_facing_, target_yaw, alpha);
    }
}

glm::mat4 Player::world_transform() const {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), position_);
    return glm::rotate(t, yaw_facing_, glm::vec3(0.0f, 1.0f, 0.0f));
}

}  // namespace vigil
