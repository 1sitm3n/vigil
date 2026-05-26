#pragma once

#include <algorithm>

namespace vigil {

// GDD §6/§7 Faith economy — clone of Stamina with different constants.
//
//   max:        60
//   regen:      5/s ALWAYS (no exclusion list — unlike Stamina, Faith
//               doesn't pause during attacks or blocks; only spending
//               states actually consume it, and that's accounted for
//               by drain() at entry)
//   Holy Bolt:  20 Faith at cast entry
//   refund:     50% of cost on cancellation (GDD §6 — interruptible by damage)
//
// Day 15 lays the contract on the Player side; Phase 4 enemies hook the
// damage-cancel path that triggers the 10-Faith refund.
class Faith {
public:
    static constexpr float MAX_VALUE  = 60.0f;
    static constexpr float REGEN_RATE = 5.0f;
    static constexpr float BOLT_COST  = 20.0f;

    float current()  const { return current_; }
    float fraction() const { return current_ / MAX_VALUE; }

    bool available(float cost) const { return current_ >= cost; }

    void drain(float amount)              { current_ = std::max(0.0f, current_ - amount); }
    void refund(float amount)             { current_ = std::min(MAX_VALUE, current_ + amount); }
    void drain_rate(float rate, float dt) { drain(rate * dt); }
    void regen_rate(float rate, float dt) { current_ = std::min(MAX_VALUE, current_ + rate * dt); }

private:
    float current_ = MAX_VALUE;
};

}  // namespace vigil
