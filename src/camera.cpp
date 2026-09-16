#include "exworld/camera.hpp"

#include <algorithm>
#include <cmath>

namespace exworld {

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

void GameCamera::reset(const exgine::Vec3& target) {
    const float cy = std::cos(yaw_);
    const float sy = std::sin(yaw_);
    const float cp = std::cos(pitch_);
    const float sp = std::sin(pitch_);
    current_pos_ = {
        target.x - sy * cp * distance_,
        target.y - sp * distance_ + 2.6f,
        target.z - cy * cp * distance_
    };
    initialized_ = true;
    camera_.position = current_pos_;
    // EXGINE uses conventional camera forward (-Z), while the gameplay
    // camera's yaw/pitch convention describes forward as +X/+Z. Convert
    // between the two conventions at the game-camera boundary.
    camera_.rotation = {-pitch_, kPi - yaw_, 0.f};
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

    const float dist = in_vehicle ? vehicle_distance_ : distance_;
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

    const float t = 1.f - std::exp(-smooth_ * std::max(dt, 0.f));
    current_pos_.x += (desired.x - current_pos_.x) * t;
    current_pos_.y += (desired.y - current_pos_.y) * t;
    current_pos_.z += (desired.z - current_pos_.z) * t;

    camera_.position = current_pos_;
    camera_.rotation = {-pitch_, kPi - yaw_, 0.f};
    camera_.vertical_fov_degrees = in_vehicle ? 58.f : 55.f;

    (void)runtime.set_main_camera(camera_);

    // Stream procedural terrain/water into the same generic render path used
    // by authored entities. The bridge contains no game-specific knowledge.
    (void)world_render_.sync(runtime, target);
}

} // namespace exworld
