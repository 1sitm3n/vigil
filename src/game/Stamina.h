#pragma once

#include <algorithm>

namespace vigil {

// GDD §6/§7 stamina economy.
//
//   max:           100
//   regen:         25/s when not attacking / blocking / sprinting (§7)
//   light attack:  12 stamina  (cost on entry to Attack1/2/3)
//   heavy attack:  24 stamina  (cost on entry to Heavy)
//   roll:          25 stamina  (cost on entry to Roll)
//   sprint:        30 stamina/s drain rate during Jog + shift_held
//
// Gate semantics: action initiates only if available(cost). Once initiated,
// drain(cost) deducts up front. This deviates from the roadmap's literal
// "can't attack if < 10" (which would allow a 10/11/12 stamina attack to go
// negative); we gate at cost to keep the resource sane. Flip back via
// Player.cpp's input-build if you want the looser version.
//
// Sprint is special: available_for_sprint() returns true for any non-zero
// stamina. drain_rate() bleeds it over time; when it hits 0, Player.cpp
// overrides pin.sprint = false before passing input to the SM, forcing the
// Jog -> Walk drop next tick.
//
// Day 14 Faith will mirror this layout — copy-paste with renames + a slower
// regen rate (5/s, GDD §6).
class Stamina {
public:
    static constexpr float MAX_VALUE      = 100.0f;
    static constexpr float REGEN_RATE     = 25.0f;
    static constexpr float SPRINT_DRAIN   = 30.0f;
    static constexpr float LIGHT_COST     = 12.0f;
    static constexpr float HEAVY_COST     = 24.0f;
    static constexpr float ROLL_COST      = 25.0f;

    float current()  const { return current_; }
    float fraction() const { return current_ / MAX_VALUE; }

    bool available(float cost)  const { return current_ >= cost; }
    bool available_for_sprint() const { return current_ > 0.0f; }

    void drain(float amount)              { current_ = std::max(0.0f, current_ - amount); }
    void drain_rate(float rate, float dt) { drain(rate * dt); }
    void regen_rate(float rate, float dt) { current_ = std::min(MAX_VALUE, current_ + rate * dt); }

private:
    float current_ = MAX_VALUE;
};

}  // namespace vigil
