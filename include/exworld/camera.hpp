#pragma once

#include "exgine/runtime.hpp"

namespace exworld {

// Third-person follow camera for free-roam / GTA-style control
class GameCamera {
public:
    void reset(const exgine::Vec3& target);

    // yaw/pitch in radians, distance from target
    void set_look(float yaw, float pitch) noexcept;
    void set_distance(float d) noexcept { distance_ = d; }

    void update(float dt, const exgine::Vec3& target, bool in_vehicle,
                exgine::Runtime& runtime);

    [[nodiscard]] const exgine::Camera& camera() const noexcept { return camera_; }
    [[nodiscard]] float yaw() const noexcept { return yaw_; }
    [[nodiscard]] float pitch() const noexcept { return pitch_; }

private:
    exgine::Camera camera_{};
    float yaw_ = 0.6f;
    float pitch_ = -0.25f;
    float distance_ = 5.5f;
    float vehicle_distance_ = 8.0f;
    float smooth_ = 10.0f;
    exgine::Vec3 current_pos_{};
    bool initialized_ = false;
};

} // namespace exworld
