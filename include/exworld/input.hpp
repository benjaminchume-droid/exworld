#pragma once

#include "exworld/player.hpp"

#include <cstdint>

namespace exworld {

// Platform-agnostic input. Desktop maps keyboard/gamepad.
// Android maps touch virtual sticks + buttons into the same structure.
enum class InputDevice : std::uint8_t { Keyboard, Gamepad, Touch, Combined };

struct TouchFinger {
    bool down = false;
    float x = 0.f; // 0..1 normalized
    float y = 0.f;
    std::int32_t id = -1;
};

class InputSystem {
public:
    void reset() noexcept;

    // Desktop / test injection
    void set_key(int keycode, bool down) noexcept;
    void set_gamepad_axis(float left_x, float left_y, float right_x, float right_y) noexcept;
    void set_gamepad_button(int button, bool down) noexcept;

    // Android touch
    void set_touch(int finger_id, bool down, float norm_x, float norm_y) noexcept;

    // Build the PlayerInput consumed by the game each frame
    [[nodiscard]] PlayerInput poll() noexcept;

    [[nodiscard]] InputDevice last_device() const noexcept { return device_; }

private:
    bool key_w_ = false, key_a_ = false, key_s_ = false, key_d_ = false;
    bool key_shift_ = false, key_space_ = false, key_e_ = false, key_f_ = false;
    bool key_ctrl_ = false;

    float pad_lx_ = 0, pad_ly_ = 0, pad_rx_ = 0, pad_ry_ = 0;
    bool pad_a_ = false, pad_b_ = false, pad_x_ = false, pad_y_ = false;
    bool pad_lb_ = false, pad_rb_ = false;

    TouchFinger fingers_[4]{};
    InputDevice device_ = InputDevice::Keyboard;

    // Edge detection for one-shot actions
    bool prev_interact_ = false;
    bool prev_exit_ = false;
};

} // namespace exworld
