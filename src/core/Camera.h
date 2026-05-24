#pragma once

#include <glm/glm.hpp>

namespace vigil {

// Third-person follow camera. Day 11 flip: was orbit-at-origin (Phase 1/2),
// now main.cpp pushes target = player.pos + (0, 0.5, 0) each frame and feeds
// mouse-look unconditionally. Spherical coords (yaw, pitch, distance) around
// that pivot. Vulkan NDC Y-flip is baked into projection() so render code
// stays dumb. forward_xz / right_xz unchanged — Player consumes them for
// camera-relative WASD.
class Camera {
public:
    Camera() = default;

    // Mouse-look. Day 11: called unconditionally on any mouse motion (no
    // RMB gate). Pixel-scale matches SDL3's relative-mouse xrel/yrel.
    void orbit(float dx, float dy);
    void zoom(float wheel_delta);
    void pan_target(const glm::vec3& delta);

    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;  // Vulkan Y-flip baked in
    glm::vec3 position() const;

    const glm::vec3& target() const { return target_; }
    void set_target(const glm::vec3& t) { target_ = t; }

    float yaw()      const { return yaw_; }
    float pitch()    const { return pitch_; }
    float distance() const { return distance_; }

    // Camera-relative basis projected onto XZ. Player consumes these to
    // rotate WASD into world space. Already signed correctly — do NOT
    // re-derive the trig in callers (Phase 1 yaw+pi bug).
    glm::vec3 forward_xz() const;
    glm::vec3 right_xz()   const;

    // Tunables — public so main.cpp can feel-tune at runtime without
    // touching this file. Defaults are Day 11's starting point; expect to
    // iterate after seeing the camera in motion.
    float orbit_sensitivity = 0.0025f;  // rad per pixel of relative-mouse motion
    float zoom_step         = 0.92f;   // multiplier per wheel tick
    float pan_speed         = 2.0f;    // m/s of target translation (unused Day 11+)

private:
    // target_ initial value doesn't matter — main.cpp overwrites it each
    // frame from player.position(). Defaults shown for safety.
    glm::vec3 target_   { 0.0f, 0.5f, 0.0f };

    // yaw=0 -> camera at +Z looking down -Z. Knight default faces -Z
    // (Mixamo forward), so yaw=0 sees the knight's back. pitch tilts the
    // camera down at the player.
    float     yaw_      = 0.0f;
    float     pitch_    = glm::radians(20.0f);
    float     distance_ = 4.0f;

    float fov_y_      = glm::radians(55.0f);  // slightly wider FoV for 3rd person
    float near_plane_ = 0.1f;
    float far_plane_  = 200.0f;

    float min_pitch_    = glm::radians( -5.0f);  // never invert below horizon
    float max_pitch_    = glm::radians( 80.0f);  // never go fully top-down
    float min_distance_ = 2.0f;
    float max_distance_ = 12.0f;
};

}  // namespace vigil
