#include "game/Projectile.h"

#include <algorithm>
#include <cmath>

namespace vigil {

namespace {
constexpr float kEpsilon = 1.0e-5f;
constexpr float kPi      = 3.14159265358979f;
}  // namespace

void update_projectile(Projectile& p, float dt) {
    if (!p.alive) return;

    // Weak homing. If target is within HOMING_CONE_DEG of current velocity,
    // rotate velocity toward target at HOMING_RATE_DEG_S (capped at the
    // current angle so we don't overshoot in a single tick). Linear lerp
    // between unit vectors + renormalize — cheaper than slerp and
    // indistinguishable at the small per-frame angles this produces.
    if (p.target) {
        const glm::vec3 to_target = *p.target - p.position;
        const float to_target_len = glm::length(to_target);
        const float vel_len       = glm::length(p.velocity);
        if (to_target_len > kEpsilon && vel_len > kEpsilon) {
            const glm::vec3 vel_dir = p.velocity / vel_len;
            const glm::vec3 to_dir  = to_target / to_target_len;
            const float cos_ang     = std::clamp(glm::dot(vel_dir, to_dir), -1.0f, 1.0f);
            const float ang_rad     = std::acos(cos_ang);
            const float cone_rad    = Projectile::HOMING_CONE_DEG * (kPi / 180.0f);
            if (ang_rad <= cone_rad) {
                const float rate_rad = Projectile::HOMING_RATE_DEG_S * (kPi / 180.0f);
                const float step     = std::min(rate_rad * dt, ang_rad);
                const float t        = (ang_rad > kEpsilon) ? (step / ang_rad) : 0.0f;
                glm::vec3 new_dir = vel_dir * (1.0f - t) + to_dir * t;
                const float new_dir_len = glm::length(new_dir);
                if (new_dir_len > kEpsilon) {
                    new_dir /= new_dir_len;
                    p.velocity = new_dir * vel_len;
                }
            }
        }
    }

    p.position += p.velocity * dt;

    p.lifetime_remaining -= dt;
    if (p.lifetime_remaining <= 0.0f) {
        p.alive = false;
    }
}

}  // namespace vigil
