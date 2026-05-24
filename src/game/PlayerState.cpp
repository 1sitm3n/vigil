#include "game/PlayerState.h"

namespace vigil {

void PlayerState::update(float dt, const PlayerInput& in) {
    state_changed_ = false;
    state_time_   += dt;

    switch (state_) {
        case PlayerStateId::Idle:
            if (in.attack)       { transition_to(PlayerStateId::Attack1); break; }
            if (in.dodge)        { transition_to(PlayerStateId::Roll);    break; }
            if (in.move_forward) { transition_to(in.sprint ? PlayerStateId::Jog
                                                           : PlayerStateId::Walk); }
            break;

        case PlayerStateId::Walk:
            if (in.attack)        { transition_to(PlayerStateId::Attack1); break; }
            if (in.dodge)         { transition_to(PlayerStateId::Roll);    break; }
            if (!in.move_forward) { transition_to(PlayerStateId::Idle);    break; }
            if (in.sprint)        { transition_to(PlayerStateId::Jog);     break; }
            break;

        case PlayerStateId::Jog:
            if (in.attack)        { transition_to(PlayerStateId::Attack1); break; }
            if (in.dodge)         { transition_to(PlayerStateId::Roll);    break; }
            if (!in.move_forward) { transition_to(PlayerStateId::Idle);    break; }
            if (!in.sprint)       { transition_to(PlayerStateId::Walk);    break; }
            break;

        case PlayerStateId::Attack1:
        case PlayerStateId::Attack2:
        case PlayerStateId::Attack3:
            update_attack(in);
            break;

        case PlayerStateId::Roll:
            // GDD §6: roll cannot be cancelled.
            if (state_time_ >= ROLL_DURATION) {
                transition_to(default_locomotion(in));
            }
            break;

        case PlayerStateId::Count:
            break;  // sentinel, not a real state
    }
}

void PlayerState::update_attack(const PlayerInput& in) {
    const float t = state_time_;

    if (t < ATK_STARTUP) {
        phase_ = AttackPhase::Startup;
        if (in.attack) attack_buffer_ = true;
        return;
    }
    if (t < ATK_STARTUP + ATK_ACTIVE) {
        phase_ = AttackPhase::Active;
        if (in.attack) attack_buffer_ = true;
        return;
    }

    // Recovery + 0.10s grace window beyond recovery end.
    if (t < ATK_STARTUP + ATK_ACTIVE + COMBO_WINDOW) {
        phase_ = AttackPhase::Recovery;

        // Roll-cancel: defensive option mid-recovery.
        if (in.dodge) { transition_to(PlayerStateId::Roll); return; }

        // Combo chain on live or buffered LMB. Attack3 is terminal.
        const bool want_chain = (in.attack || attack_buffer_);
        if (want_chain && state_ != PlayerStateId::Attack3) {
            const PlayerStateId next = (state_ == PlayerStateId::Attack1)
                ? PlayerStateId::Attack2
                : PlayerStateId::Attack3;
            transition_to(next);
            return;
        }
        return;
    }

    // Combo window expired — return to locomotion state implied by input.
    transition_to(default_locomotion(in));
}

void PlayerState::transition_to(PlayerStateId next) {
    state_         = next;
    state_time_    = 0.0f;
    attack_buffer_ = false;
    state_changed_ = true;
    attack_landed_ = false;   // Day 12: fresh hit budget per attack instance
    phase_ = (next == PlayerStateId::Attack1 ||
              next == PlayerStateId::Attack2 ||
              next == PlayerStateId::Attack3)
              ? AttackPhase::Startup
              : AttackPhase::None;
}

PlayerStateId PlayerState::default_locomotion(const PlayerInput& in) const {
    if (!in.move_forward) return PlayerStateId::Idle;
    return in.sprint ? PlayerStateId::Jog : PlayerStateId::Walk;
}

bool PlayerState::iframes_active() const {
    return state_ == PlayerStateId::Roll
        && state_time_ >= ROLL_IF_START
        && state_time_ <= ROLL_IF_END;
}

}  // namespace vigil

namespace vigil {

const char* to_string(PlayerStateId id) {
    switch (id) {
        case PlayerStateId::Idle:    return "Idle";
        case PlayerStateId::Walk:    return "Walk";
        case PlayerStateId::Jog:     return "Jog";
        case PlayerStateId::Attack1: return "Attack1";
        case PlayerStateId::Attack2: return "Attack2";
        case PlayerStateId::Attack3: return "Attack3";
        case PlayerStateId::Roll:    return "Roll";
        case PlayerStateId::Count:   return "Count";
    }
    return "?";
}

const char* to_string(AttackPhase phase) {
    switch (phase) {
        case AttackPhase::None:     return "None";
        case AttackPhase::Startup:  return "Startup";
        case AttackPhase::Active:   return "Active";
        case AttackPhase::Recovery: return "Recovery";
    }
    return "?";
}

}  // namespace vigil
