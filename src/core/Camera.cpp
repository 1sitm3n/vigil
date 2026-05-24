#include "core/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace vigil {

void Camera::orbit(float dx, float dy) {
    // Day 12: dx sign flipped so mouse-RIGHT rotates the view RIGHT
    // (background scrolls LEFT). Standard 3rd-person convention
    // (Souls / Skyrim / Witcher). Original code's yaw_ += dx gave the
    // inverse — view turned left on mouse right, which read as "the
    // camera moves the opposite way" with the dummy as a reference.
    // dy kept positive: mouse-DOWN tilts view DOWN (non-inverted Y).
    // Flip dy's sign too if you prefer Y-invert.
    yaw_   -= dx * orbit_sensitivity;
    pitch_ += dy * orbit_sensitivity;
    pitch_  = std::clamp(pitch_, min_pitch_, max_pitch_);

    constexpr float two_pi = 6.28318530718f;
    if (yaw_ >  two_pi) yaw_ -= two_pi;
    if (yaw_ < -two_pi) yaw_ += two_pi;
}

void Camera::zoom(float wheel_delta) {
    distance_ *= std::pow(zoom_step, wheel_delta);
    distance_  = std::clamp(distance_, min_distance_, max_distance_);
}

void Camera::pan_target(const glm::vec3& delta) {
    target_ += delta;
}

glm::vec3 Camera::position() const {
    const float cp = std::cos(pitch_);
    const float sp = std::sin(pitch_);
    const float cy = std::cos(yaw_);
    const float sy = std::sin(yaw_);
    // yaw=0,pitch=0 places camera at target + (0,0,distance), looking down -Z.
    return target_ + distance_ * glm::vec3(cp * sy, sp, cp * cy);
}

glm::mat4 Camera::view() const {
    return glm::lookAt(position(), target_, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::projection(float aspect) const {
    glm::mat4 p = glm::perspective(fov_y_, aspect, near_plane_, far_plane_);
    p[1][1] *= -1.0f;  // Vulkan NDC Y-flip
    return p;
}

glm::vec3 Camera::forward_xz() const {
    const float sy = std::sin(yaw_);
    const float cy = std::cos(yaw_);
    // Direction from camera toward target, projected onto XZ.
    return glm::vec3(-sy, 0.0f, -cy);
}

glm::vec3 Camera::right_xz() const {
    const float sy = std::sin(yaw_);
    const float cy = std::cos(yaw_);
    // forward_xz × (0,1,0)
    return glm::vec3(cy, 0.0f, -sy);
}

}  // namespace vigil
