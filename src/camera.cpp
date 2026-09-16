#include "exworld/camera.hpp"

#include <algorithm>
#include <cmath>

namespace exworld {

void GameCamera::reset(const exgine::Vec3& target) {
    // Pull back so a 1.75m character reads small against 3m floors / 14m buildings
    current_pos_ = {target.x - 7.f, target.y + 3.2f, target.z + 7.f};
    initialized_ = true;
    camera_.position = current_pos_;
    camera_.rotation = {pitch_, yaw_, 0.f};
    camera_.vertical_fov_degrees = 55.f;
    camera_.near_plane = 0.1f;
    camera_.far_plane = 5000.f;
}

void GameCamera::set_look(float yaw, float pitch) noexcept {
    yaw_ = yaw;
    pitch_ = std::clamp(pitch, -1.15f, 0.4f);
}

void GameCamera::update(float dt, const exgine::Vec3& target, bool in_vehicle,
                        exgine::Runtime& runtime) {
    if (!initialized_) reset(target);

    // Foot: ~10m back; vehicle: ~16m — world feels larger, player smaller on screen
    const float dist = in_vehicle ? 16.f : 10.f;
    const float height = in_vehicle ? 4.2f : 2.6f;
    const float cy = std::cos(yaw_);
    const float sy = std::sin(yaw_);
    const float cp = std::cos(pitch_);
    const float sp = std::sin(pitch_);

    exgine::Vec3 desired{
        target.x - sy * cp * dist,
        target.y - sp * dist + height,
        target.z - cy * cp * dist
    };

    const float t = 1.f - std::exp(-smooth_ * dt);
    current_pos_.x += (desired.x - current_pos_.x) * t;
    current_pos_.y += (desired.y - current_pos_.y) * t;
    current_pos_.z += (desired.z - current_pos_.z) * t;

    camera_.position = current_pos_;
    camera_.rotation = {pitch_, yaw_, 0.f};
    camera_.vertical_fov_degrees = in_vehicle ? 58.f : 55.f;

    (void)runtime.set_main_camera(camera_);
}

} // namespace exworld
