#include "game/PlayerState.h"

namespace vigil {

void PlayerState::update(float dt, const PlayerInput& in) {
    state_changed_ = false;
    state_time_   += dt;

    switch (state_) {
        case PlayerStateId::Idle:
            if (in.riposte)      { transition_to(PlayerStateId::Riposte); break; }
            if (in.cast)         { transition_to(PlayerStateId::Cast);    break; }
            if (in.heavy_attack) { transition_to(PlayerStateId::Heavy);   break; }
            if (in.attack)       { transition_to(PlayerStateId::Attack1); break; }
            if (in.dodge)        { transition_to(PlayerStateId::Roll);    break; }
            if (in.block)        { transition_to(PlayerStateId::Block);   break; }
            if (in.move_forward) { transition_to(in.sprint ? PlayerStateId::Jog
                                                           : PlayerStateId::Walk); }
            break;

        case PlayerStateId::Walk:
            if (in.riposte)       { transition_to(PlayerStateId::Riposte); break; }
            if (in.cast)          { transition_to(PlayerStateId::Cast);    break; }
            if (in.heavy_attack)  { transition_to(PlayerStateId::Heavy);   break; }
            if (in.attack)        { transition_to(PlayerStateId::Attack1); break; }
            if (in.dodge)         { transition_to(PlayerStateId::Roll);    break; }
            if (in.block)         { transition_to(PlayerStateId::Block);   break; }
            if (!in.move_forward) { transition_to(PlayerStateId::Idle);    break; }
            if (in.sprint)        { transition_to(PlayerStateId::Jog);     break; }
            break;

        case PlayerStateId::Jog:
            if (in.riposte)       { transition_to(PlayerStateId::Riposte); break; }
            if (in.cast)          { transition_to(PlayerStateId::Cast);    break; }
            if (in.heavy_attack)  { transition_to(PlayerStateId::Heavy);   break; }
            if (in.attack)        { transition_to(PlayerStateId::Attack1); break; }
            if (in.dodge)         { transition_to(PlayerStateId::Roll);    break; }
            if (in.block)         { transition_to(PlayerStateId::Block);   break; }
            if (!in.move_forward) { transition_to(PlayerStateId::Idle);    break; }
            if (!in.sprint)       { transition_to(PlayerStateId::Walk);    break; }
            break;

        case PlayerStateId::Attack1:
        case PlayerStateId::Attack2:
        case PlayerStateId::Attack3:
        case PlayerStateId::Heavy:
        case PlayerStateId::Riposte:
        case PlayerStateId::Cast:
            update_attack(in);
            break;

        case PlayerStateId::Roll:
            // GDD §6: roll cannot be cancelled.
            if (state_time_ >= ROLL_DURATION) {
                transition_to(default_locomotion(in));
            }
            break;

        case PlayerStateId::Block:
            // Day 14: riposte takes priority. Player::try_parry already
            // consumed the parry window and set riposte_pending_; the SM
            // jumps straight to Riposte even if RMB is still held — block
            // is finished the moment the parry catches.
            if (in.riposte) { transition_to(PlayerStateId::Riposte); break; }
            if (!in.block)  { transition_to(default_locomotion(in));  break; }
            break;

        case PlayerStateId::Count:
            break;  // sentinel, not a real state
    }
}

void PlayerState::cancel_cast() {
    // Day 15: external interrupt from Player when incoming damage lands
    // during cast. Caller (Player::cancel_cast_with_refund) handles the
    // 50% Faith refund; we just drop out of the state.
    if (state_ == PlayerStateId::Cast) {
        transition_to(PlayerStateId::Idle);
    }
}

void PlayerState::update_attack(const PlayerInput& in) {
    const bool is_heavy   = (state_ == PlayerStateId::Heavy);
    const bool is_riposte = (state_ == PlayerStateId::Riposte);
    const bool is_cast    = (state_ == PlayerStateId::Cast);
    const bool is_light   = !is_heavy && !is_riposte && !is_cast;

    float startup, active, post_active;
    if (is_cast) {
        startup     = CAST_STARTUP;
        active      = CAST_ACTIVE;
        post_active = CAST_RECOVERY;
    } else if (is_riposte) {
        startup     = RIPOSTE_STARTUP;
        active      = RIPOSTE_ACTIVE;
        post_active = RIPOSTE_RECOVERY;
    } else if (is_heavy) {
        startup     = HEAVY_STARTUP;
        active      = HEAVY_ACTIVE;
        post_active = HEAVY_RECOVERY;
    } else {
        // Light: COMBO_WINDOW covers recovery + 0.10s grace for chain.
        startup     = ATK_STARTUP;
        active      = ATK_ACTIVE;
        post_active = COMBO_WINDOW;
    }

    const float t = state_time_;

    if (t < startup) {
        phase_ = AttackPhase::Startup;
        if (is_light && in.attack) attack_buffer_ = true;
        return;
    }
    if (t < startup + active) {
        phase_ = AttackPhase::Active;
        if (is_light && in.attack) attack_buffer_ = true;
        return;
    }

    if (t < startup + active + post_active) {
        phase_ = AttackPhase::Recovery;

        // Roll-cancel: light and heavy only. Riposte and Cast are terminal
        // (GDD §6). Cast is also INTERRUPTIBLE BY DAMAGE — not by input —
        // so the player can't bail out voluntarily; only incoming hits via
        // cancel_cast() can break it early.
        if (in.dodge && !is_riposte && !is_cast) { transition_to(PlayerStateId::Roll); return; }

        // Combo chain on light only.
        if (is_light) {
            const bool want_chain = (in.attack || attack_buffer_);
            if (want_chain && state_ != PlayerStateId::Attack3) {
                const PlayerStateId next = (state_ == PlayerStateId::Attack1)
                    ? PlayerStateId::Attack2
                    : PlayerStateId::Attack3;
                transition_to(next);
                return;
            }
        }
        return;
    }

    // Recovery expired — return to locomotion implied by input.
    transition_to(default_locomotion(in));
}

void PlayerState::transition_to(PlayerStateId next) {
    state_         = next;
    state_time_    = 0.0f;
    attack_buffer_ = false;
    state_changed_ = true;
    attack_landed_ = false;   // fresh hit budget per attack instance
    phase_ = (next == PlayerStateId::Attack1 ||
              next == PlayerStateId::Attack2 ||
              next == PlayerStateId::Attack3 ||
              next == PlayerStateId::Heavy   ||
              next == PlayerStateId::Riposte ||
              next == PlayerStateId::Cast)
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
        case PlayerStateId::Heavy:   return "Heavy";
        case PlayerStateId::Block:   return "Block";
        case PlayerStateId::Riposte: return "Riposte";
        case PlayerStateId::Cast:    return "Cast";
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
