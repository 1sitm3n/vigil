#pragma once

#include <cstdint>

namespace vigil {

// One enum across locomotion + combat + dodge. PlayerStateId is the SM
// node AND the index into Renderer's animation slot array — 1:1 mapping
// locks the contract. New states require new animation slots and vice versa.
//
// Day 13: Heavy (7) and Block (8) added. Heavy mirrors Attack1/2/3's phase
// machine with longer frame data (0.55/0.15/0.70, GDD §6). Block is a held
// state — no internal timer, exits on RMB release.
enum class PlayerStateId : uint8_t {
    Idle    = 0,
    Walk    = 1,
    Jog     = 2,
    Attack1 = 3,
    Attack2 = 4,
    Attack3 = 5,
    Roll    = 6,
    Heavy   = 7,
    Block   = 8,
    Count   = 9
};

// Sub-phase within attack states. None outside of Attack1/2/3/Heavy.
enum class AttackPhase : uint8_t {
    None,
    Startup,
    Active,
    Recovery,
};

// Edge-triggered for tap inputs (attack, heavy_attack, dodge); held for
// movement (forward, sprint, block). Player.cpp builds this from Window's
// InputFrame each frame, gating each field through stamina before passing
// to the SM.
//
// Day 13: heavy_attack split out from attack so Shift+LMB triggers heavy
// only, not heavy + light + sprint. block held while RMB down.
struct PlayerInput {
    bool move_forward = false;  // W/A/S/D held
    bool sprint       = false;  // Shift held AND stamina > 0
    bool attack       = false;  // LMB pressed, !shift_held, stamina >= 12
    bool heavy_attack = false;  // LMB pressed,  shift_held, stamina >= 24
    bool dodge        = false;  // Space pressed, stamina >= 25
    bool block        = false;  // RMB held
};

// String labels for debug overlay rendering.
const char* to_string(PlayerStateId id);
const char* to_string(AttackPhase   phase);

// Phase 2/3 player state machine — Idle/Walk/Jog/Attack1-3/Roll/Heavy/Block.
//
// Timing is GDD §6 canonical, not animation duration:
//   light attack: startup 0.30s | active 0.10s | recovery 0.40s
//                 combo window: 0.50s from recovery start (extends 0.10s past)
//   heavy attack: startup 0.55s | active 0.15s | recovery 0.70s
//                 no combo chain; can be roll-cancelled during recovery
//   roll:         0.50s total, i-frames 0.10s -> 0.35s
//   block:        no timer; held while in.block, exits to default_locomotion
//
// Input buffering: LMB during startup/active is buffered, consumed at
// recovery start to chain. LIGHT ONLY — heavy is single-hit per GDD §6.
// Dodge during attack recovery (either kind) cancels into Roll. Roll itself
// cannot be cancelled.
//
// Day 12: attack_landed_ gates the hit test to one-shot per attack instance.
// Day 13: applies equally to Heavy — both share AttackPhase::Active during
// the active window, and main.cpp's cone test reads attack_phase() not id().
class PlayerState {
public:
    PlayerState() = default;

    void update(float dt, const PlayerInput& input);

    PlayerStateId id()              const { return state_; }
    AttackPhase   attack_phase()    const { return phase_; }
    float         state_time()      const { return state_time_; }
    bool          attack_buffered() const { return attack_buffer_; }

    bool state_changed() const { return state_changed_; }
    bool iframes_active() const;

    bool attack_landed()      const { return attack_landed_; }
    void mark_attack_landed()       { attack_landed_ = true; }

private:
    void update_attack(const PlayerInput& in);
    void transition_to(PlayerStateId next);
    PlayerStateId default_locomotion(const PlayerInput& in) const;

    // Light attack frame data (GDD §6).
    static constexpr float ATK_STARTUP    = 0.30f;
    static constexpr float ATK_ACTIVE     = 0.10f;
    static constexpr float ATK_RECOVERY   = 0.40f;
    static constexpr float COMBO_WINDOW   = 0.50f; // covers recovery + 0.10s grace

    // Heavy attack frame data (GDD §6).
    static constexpr float HEAVY_STARTUP  = 0.55f;
    static constexpr float HEAVY_ACTIVE   = 0.15f;
    static constexpr float HEAVY_RECOVERY = 0.70f;

    // Roll (GDD §6).
    static constexpr float ROLL_DURATION  = 0.50f;
    static constexpr float ROLL_IF_START  = 0.10f;
    static constexpr float ROLL_IF_END    = 0.35f;

    PlayerStateId state_         = PlayerStateId::Idle;
    AttackPhase   phase_         = AttackPhase::None;
    float         state_time_    = 0.0f;
    bool          attack_buffer_ = false;
    bool          state_changed_ = false;
    bool          attack_landed_ = false;
};

}  // namespace vigil
