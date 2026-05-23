#pragma once

#include <cstdint>

namespace vigil {

// One enum across locomotion + combat + dodge. PlayerStateId is the SM
// node AND the index into Renderer's animation slot array — 1:1 mapping
// locks the contract. New states require new animation slots and vice versa.
enum class PlayerStateId : uint8_t {
    Idle    = 0,
    Walk    = 1,
    Jog     = 2,
    Attack1 = 3,
    Attack2 = 4,
    Attack3 = 5,
    Roll    = 6,
    Count   = 7
};

// Sub-phase within attack states. None outside of Attack1/2/3.
enum class AttackPhase : uint8_t {
    None,
    Startup,
    Active,
    Recovery,
};

// Edge-triggered for tap inputs (attack, dodge); held for movement (forward, sprint).
// main.cpp fills this from Window's InputFrame each frame.
struct PlayerInput {
    bool move_forward = false;  // W held
    bool sprint       = false;  // Shift held
    bool attack       = false;  // LMB pressed THIS frame (edge)
    bool dodge        = false;  // Space pressed THIS frame (edge)
};

// Phase 2 player state machine — Idle/Walk/Jog/Attack1-3/Roll.
//
// Timing is GDD §6 canonical, not animation duration:
//   light attack: startup 0.30s | active 0.10s | recovery 0.40s
//   combo window: 0.50s from recovery start (extends 0.10s past recovery end)
//   roll:         0.50s total, i-frames 0.10s -> 0.35s
//
// Renderer scales animation playback speed per-slot to fit the SM budget
// (e.g. roll_forward.glb at 1.27s native -> speed 2.54 to hit 0.5s).
//
// Input buffering: LMB during startup/active is buffered, consumed at
// recovery start to chain. Dodge during attack recovery cancels into Roll.
// Roll itself cannot be cancelled (GDD §6).
class PlayerState {
public:
    PlayerState() = default;

    void update(float dt, const PlayerInput& input);

    PlayerStateId id()           const { return state_; }
    AttackPhase   attack_phase() const { return phase_; }
    float         state_time()   const { return state_time_; }

    // True for the one update() that transitioned. Renderer reads this each
    // frame to trigger Animator::play() with a 0.2s crossfade.
    bool state_changed() const { return state_changed_; }

    // Damage system contract (Phase 3+): true while in roll i-frame window.
    bool iframes_active() const;

private:
    void update_attack(const PlayerInput& in);
    void transition_to(PlayerStateId next);
    PlayerStateId default_locomotion(const PlayerInput& in) const;

    // GDD §6 frame data, seconds.
    static constexpr float ATK_STARTUP   = 0.30f;
    static constexpr float ATK_ACTIVE    = 0.10f;
    static constexpr float ATK_RECOVERY  = 0.40f;
    static constexpr float COMBO_WINDOW  = 0.50f; // from recovery start
    static constexpr float ROLL_DURATION = 0.50f;
    static constexpr float ROLL_IF_START = 0.10f;
    static constexpr float ROLL_IF_END   = 0.35f;

    PlayerStateId state_         = PlayerStateId::Idle;
    AttackPhase   phase_         = AttackPhase::None;
    float         state_time_    = 0.0f;
    bool          attack_buffer_ = false;
    bool          state_changed_ = false;
};

}  // namespace vigil
