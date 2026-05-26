#pragma once

#include <glm/glm.hpp>

namespace vigil {

// Holy Bolt projectile (GDD §6: 50 dmg, 18 m/s travel, weak 30° cone homing,
// 0.8s cast cost 20 Faith). Spawned from Player::position + facing on Cast
// entering AttackPhase::Active; one bolt per cast (attack_landed_ gate).
//
// Design notes:
//  - target is a non-owning pointer. Day 15 it's always &Renderer::DUMMY_POSITION
//    (single dummy); Phase 4 enemy spawns will populate from a wider list and
//    main.cpp will pick the nearest in-cone target at spawn.
//  - "Weak homing": each frame, if target is within HOMING_CONE_DEG of the
//    current velocity vector, the velocity rotates toward target at
//    HOMING_RATE_DEG_S. Outside the cone, no homing — fires straight. Matches
//    "weakly toward nearest enemy in 30° cone" (GDD §6).
//  - Lifetime cap: 3s. Without this, missed bolts fly to infinity and stay
//    in the pool forever (small leak per missed cast, ugly over a 30-min run).
//  - AABB hit detection lives in main.cpp (Day 15) since it's tied to the
//    fake-target dummy; Phase 4 enemies will move it into a system.
struct Projectile {
    static constexpr float BOLT_SPEED         = 18.0f;          // m/s, GDD §6
    static constexpr float BOLT_DAMAGE        = 50.0f;          // GDD §6
    static constexpr float MAX_LIFETIME       = 3.0f;           // s
    static constexpr float HOMING_CONE_DEG    = 30.0f;          // GDD §6
    static constexpr float HOMING_RATE_DEG_S  = 60.0f;          // weak; ~1/6 rev per sec
    static constexpr float DRAW_HALF_EXTENT   = 0.15f;          // visual cube half-extent
    static constexpr float HIT_HALF_EXTENT    = 0.5f;           // AABB hit test on dummy

    glm::vec3        position           { 0.0f };
    glm::vec3        velocity           { 0.0f };
    float            lifetime_remaining { MAX_LIFETIME };
    bool             alive              { true };
    const glm::vec3* target             { nullptr };
};

// Integrate position + apply weak homing + decay lifetime. Caller is
// responsible for hit detection and culling dead projectiles.
void update_projectile(Projectile& p, float dt);

}  // namespace vigil
