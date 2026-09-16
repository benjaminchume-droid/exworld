#include "exworld/camera.hpp"

#include <algorithm>
#include <cmath>

namespace exworld {

void GameCamera::reset(const exgine::Vec3& target) {
    current_pos_ = {target.x - 4.f, target.y + 2.2f, target.z + 4.f};
    initialized_ = true;
    camera_.position = current_pos_;
    camera_.rotation = {pitch_, yaw_, 0.f};
    camera_.vertical_fov_degrees = 58.f;
    camera_.near_plane = 0.08f;
    camera_.far_plane = 4000.f;
}

void GameCamera::set_look(float yaw, float pitch) noexcept {
    yaw_ = yaw;
    pitch_ = std::clamp(pitch, -1.2f, 0.35f);
}

void GameCamera::update(float dt, const exgine::Vec3& target, bool in_vehicle,
                        exgine::Runtime& runtime) {
    if (!initialized_) reset(target);

    const float dist = in_vehicle ? vehicle_distance_ : distance_;
    const float cy = std::cos(yaw_);
    const float sy = std::sin(yaw_);
    const float cp = std::cos(pitch_);
    const float sp = std::sin(pitch_);

    exgine::Vec3 desired{
        target.x - sy * cp * dist,
        target.y - sp * dist + (in_vehicle ? 2.8f : 1.9f),
        target.z - cy * cp * dist
    };

    const float t = 1.f - std::exp(-smooth_ * dt);
    current_pos_.x += (desired.x - current_pos_.x) * t;
    current_pos_.y += (desired.y - current_pos_.y) * t;
    current_pos_.z += (desired.z - current_pos_.z) * t;

    camera_.position = current_pos_;
    camera_.rotation = {pitch_, yaw_, 0.f};
    camera_.vertical_fov_degrees = in_vehicle ? 62.f : 58.f;

    (void)runtime.set_main_camera(camera_);
}

} // namespace exworld
