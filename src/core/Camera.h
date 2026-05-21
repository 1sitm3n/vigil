#pragma once

#include <glm/glm.hpp>

namespace vigil {

// Orbit camera around a target point. Spherical coords: yaw, pitch, distance.
// Vulkan NDC Y-flip is baked into projection() so render code stays dumb.
class Camera {
public:
    Camera() = default;

    // Apply input deltas this frame.
    void orbit(float dx, float dy);           // pixels of mouse motion
    void zoom(float wheel_delta);             // SDL wheel ticks (positive = scroll up)
    void pan_target(const glm::vec3& delta);

    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const; // Vulkan Y-flip baked in
    glm::vec3 position() const;

    const glm::vec3& target() const { return target_; }
    void set_target(const glm::vec3& t) { target_ = t; }

    float yaw()      const { return yaw_; }
    float pitch()    const { return pitch_; }
    float distance() const { return distance_; }

    // Camera-relative basis, projected onto XZ. For WASD ground-plane pan.
    glm::vec3 forward_xz() const;
    glm::vec3 right_xz()   const;

    // Tunables — public so main.cpp can poke them if you want to feel-tune.
    float orbit_sensitivity = 0.005f;  // rad per pixel
    float zoom_step         = 0.9f;    // multiplier per wheel tick
    float pan_speed         = 2.0f;    // m/s of target translation

private:
    glm::vec3 target_   { 0.0f, 0.15f, 0.0f };
    float     yaw_      = glm::radians(35.0f);
    float     pitch_    = glm::radians(20.0f);
    float     distance_ = 1.5f;

    float fov_y_      = glm::radians(45.0f);
    float near_plane_ = 0.1f;
    float far_plane_  = 100.0f;

    float min_pitch_    = glm::radians(-85.0f);
    float max_pitch_    = glm::radians( 85.0f);
    float min_distance_ = 0.3f;
    float max_distance_ = 10.0f;
};

}  // namespace vigil
