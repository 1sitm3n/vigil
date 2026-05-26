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
//
// Day 14: Riposte (9) added. Punish state opened by parry edge-press inside
// (or just before) Block. Mirrors Heavy's phase machine with riposte frame
// data (0.20/0.20/1.10 = 1.5s budget, GDD §6). Terminal — no combo chain,
// NOT roll-cancellable.
//
// Day 15: Cast (10) added. Holy Bolt cast state. 0.8s budget total
// (0.55 startup + 0.10 active + 0.15 recovery, GDD §6). Active frame spawns
// the projectile (main.cpp watches AttackPhase::Active here, same single-
// shot gate as attack_landed_). Terminal — no combo, NOT roll-cancellable.
// Cancellable by INCOMING DAMAGE only, via PlayerState::cancel_cast() —
// Player::cancel_cast_with_refund() owns the 50% Faith refund (GDD §6).
// Phase 4 enemies will hook the cancel; Day 15 uses the N-key fake-hit
// dispatcher to verify the contract.
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
    Riposte = 9,
    Cast    = 10,
    Count   = 11
};

// Sub-phase within attack states. None outside of Attack1/2/3/Heavy/Riposte/Cast.
enum class AttackPhase : uint8_t {
    None,
    Startup,
    Active,
    Recovery,
};

// Edge-triggered for tap inputs (attack, heavy_attack, dodge, riposte, cast);
// held for movement (forward, sprint, block). Player.cpp builds this from
// Window's InputFrame each frame.
//
// Day 14: riposte is set by Player when try_parry() consumes the window;
// it propagates through update() into the SM exactly once per parry.
// Day 15: cast is set by Player from q_pressed edge when faith.available(BOLT_COST).
struct PlayerInput {
    bool move_forward = false;  // W/A/S/D held
    bool sprint       = false;  // Shift held AND stamina > 0
    bool attack       = false;  // LMB pressed, !shift_held, stamina >= 12
    bool heavy_attack = false;  // LMB pressed,  shift_held, stamina >= 24
    bool dodge        = false;  // Space pressed, stamina >= 25
    bool block        = false;  // RMB held
    bool riposte      = false;  // Day 14: parry consumed; transition to Riposte
    bool cast         = false;  // Day 15: Q pressed, faith >= 20; transition to Cast
};

// String labels for debug overlay rendering.
const char* to_string(PlayerStateId id);
const char* to_string(AttackPhase   phase);

// Phase 2/3 player state machine — Idle/Walk/Jog/Attack1-3/Roll/Heavy/Block/Riposte/Cast.
//
// Timing is GDD §6 canonical, not animation duration:
//   light:   startup 0.30s | active 0.10s | recovery 0.40s
//            combo window: 0.50s from recovery start
//   heavy:   startup 0.55s | active 0.15s | recovery 0.70s
//            no combo; roll-cancellable in recovery
//   riposte: startup 0.20s | active 0.20s | recovery 1.10s = 1.5s budget
//            no combo; NOT roll-cancellable (terminal)
//   cast:    startup 0.55s | active 0.10s | recovery 0.15s = 0.8s budget
//            no combo; NOT roll-cancellable; INTERRUPTIBLE BY DAMAGE
//            (cancel_cast() called by Player on hit, refunds 50% Faith)
//   roll:    0.50s total, i-frames 0.10s -> 0.35s
//   block:   no timer; held while in.block, exits to default_locomotion
//
// Input buffering: LMB during startup/active is buffered, consumed at
// recovery start to chain. LIGHT ONLY — heavy, riposte, cast are single-hit.
class PlayerState {
public:
    PlayerState() = default;

    void update(float dt, const PlayerInput& input);

    // Day 15: external interrupt hook. Player::cancel_cast_with_refund()
    // calls this after refunding 50% Faith. No-op if not in Cast. Phase 4
    // enemies route their damage events through Player; Day 15 uses the
    // N-key fake-hit dispatcher.
    void cancel_cast();

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

    // Riposte frame data — 1.5s budget total. 0.20 startup gives a quick
    // punch off the parry; 0.20 active is generous (1.5x light) since it's
    // a reward move; 1.10 recovery sells the satisfied "after-pose."
    static constexpr float RIPOSTE_STARTUP  = 0.20f;
    static constexpr float RIPOSTE_ACTIVE   = 0.20f;
    static constexpr float RIPOSTE_RECOVERY = 1.10f;

    // Cast frame data — 0.8s budget total (GDD §6). 0.55s startup sells the
    // wind-up (hand raises, sigil forms); 0.10s active is the bolt release —
    // main.cpp's projectile-spawn watches AttackPhase::Active here, single-
    // shot via the attack_landed_ gate; 0.15s recovery is just the settle.
    static constexpr float CAST_STARTUP   = 0.55f;
    static constexpr float CAST_ACTIVE    = 0.10f;
    static constexpr float CAST_RECOVERY  = 0.15f;

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
