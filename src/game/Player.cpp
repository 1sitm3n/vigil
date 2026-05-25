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

void Player::tick_parry_window(float dt, const InputFrame& input) {
    // Decay first so a fresh edge press this frame gets the full 0.15s,
    // not 0.15s - dt.
    if (parry_window_remaining_ > 0.0f) {
        parry_window_remaining_ -= dt;
        if (parry_window_remaining_ < 0.0f) parry_window_remaining_ = 0.0f;
    }
    // RMB edge press opens (or refreshes) the window. Held RMB does NOT
    // refresh — that's the point of edge vs held: defensive tap is parry,
    // held wall is block. A single RMB press fires BOTH this and Block
    // entry on the same frame (rmb_pressed + rmb_held both true), which
    // is exactly the Souls feel.
    if (input.rmb_pressed) {
        parry_window_remaining_ = PARRY_WINDOW;
    }
}

bool Player::try_parry() {
    if (parry_window_remaining_ <= 0.0f) return false;
    parry_window_remaining_ = 0.0f;   // consume so next try_parry fails
    riposte_pending_        = true;   // update() will feed to pin.riposte
    return true;
}

void Player::absorb_block_hit(float damage) {
    // GDD §6: blocking drains stamina = 50% of incoming damage.
    // Phase 4 will handle guard-broken when drain exceeds available.
    stamina_.drain(damage * 0.5f);
}

void Player::update(float dt, const InputFrame& input, const Camera& camera) {
    // 1) Feed the SM. "move_forward" reads as "any locomotion input held" —
    //    SM only branches on the bool, not direction.
    //
    //    Day 13: stamina gates each input before the SM sees it.
    //    Day 14: pin.riposte fed from riposte_pending_ (set earlier this
    //    frame by try_parry). No stamina gate — riposte is the parry reward.
    const bool wants_heavy = input.lmb_pressed &&  input.shift_held;
    const bool wants_light = input.lmb_pressed && !input.shift_held;

    PlayerInput pin;
    pin.move_forward = input.w_held || input.a_held || input.s_held || input.d_held;
    pin.sprint       = input.shift_held    && stamina_.available_for_sprint();
    pin.attack       = wants_light         && stamina_.available(Stamina::LIGHT_COST);
    pin.heavy_attack = wants_heavy         && stamina_.available(Stamina::HEAVY_COST);
    pin.dodge        = input.space_pressed && stamina_.available(Stamina::ROLL_COST);
    pin.block        = input.rmb_held;
    pin.riposte      = riposte_pending_;
    riposte_pending_ = false;   // single-shot — clear regardless of whether
                                // the SM actually transitions (e.g., parry
                                // during Roll silently drops; i-frames
                                // already absorb the hit)

    state_.update(dt, pin);

    // 1b) Charge stamina on entry into action states.
    if (state_.state_changed()) {
        switch (state_.id()) {
            case PlayerStateId::Attack1:
            case PlayerStateId::Attack2:
            case PlayerStateId::Attack3: stamina_.drain(Stamina::LIGHT_COST); break;
            case PlayerStateId::Heavy:   stamina_.drain(Stamina::HEAVY_COST); break;
            case PlayerStateId::Roll:    stamina_.drain(Stamina::ROLL_COST);  break;
            default: break;   // Riposte: no entry cost (reward state)
        }
    }

    // 1c) Tick stamina. GDD §7: regen 25/s except attacking/blocking/sprinting.
    //     Day 14: Riposte is an attack state, joins the spending list.
    const PlayerStateId id = state_.id();
    const bool sprint_active = (id == PlayerStateId::Jog) && pin.sprint;
    const bool spending = sprint_active
                       || id == PlayerStateId::Attack1
                       || id == PlayerStateId::Attack2
                       || id == PlayerStateId::Attack3
                       || id == PlayerStateId::Heavy
                       || id == PlayerStateId::Riposte
                       || id == PlayerStateId::Block;
    if (sprint_active) stamina_.drain_rate(Stamina::SPRINT_DRAIN, dt);
    if (!spending)     stamina_.regen_rate(Stamina::REGEN_RATE, dt);

    // 2) Build world-space wish direction from camera basis.
    const glm::vec3 fwd   = camera.forward_xz();
    const glm::vec3 right = camera.right_xz();
    glm::vec3 wish_dir(0.0f);
    if (input.w_held) wish_dir += fwd;
    if (input.s_held) wish_dir -= fwd;
    if (input.d_held) wish_dir += right;
    if (input.a_held) wish_dir -= right;
    const float wish_mag = glm::length(wish_dir);
    if (wish_mag > kEpsilon) wish_dir /= wish_mag;

    // 3) Latch Roll direction on Roll entry.
    if (state_.id() == PlayerStateId::Roll && state_.state_changed()) {
        if (wish_mag > kEpsilon) {
            roll_dir_ = wish_dir;
        } else {
            // +Z-facing-model: forward = (sin(yaw), 0, cos(yaw)).
            roll_dir_ = glm::vec3(std::sin(yaw_facing_), 0.0f, std::cos(yaw_facing_));
        }
    }

    // 4) Velocity per state. Heavy/Block/Riposte all hold position via default.
    switch (state_.id()) {
        case PlayerStateId::Walk: velocity_ = wish_dir * WALK_SPEED;   break;
        case PlayerStateId::Jog:  velocity_ = wish_dir * SPRINT_SPEED; break;
        case PlayerStateId::Roll: velocity_ = roll_dir_ * ROLL_SPEED;  break;
        default:                  velocity_ = glm::vec3(0.0f);         break;
    }
    position_ += velocity_ * dt;

    // 5) Face the movement direction (locomotion + roll). Attacks hold facing.
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
