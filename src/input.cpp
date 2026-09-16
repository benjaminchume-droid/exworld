#include "exworld/input.hpp"

#include <algorithm>
#include <cmath>

namespace exworld {

void InputSystem::reset() noexcept {
    key_w_ = key_a_ = key_s_ = key_d_ = false;
    key_shift_ = key_space_ = key_e_ = key_f_ = key_ctrl_ = false;
    pad_lx_ = pad_ly_ = pad_rx_ = pad_ry_ = 0;
    pad_a_ = pad_b_ = pad_x_ = pad_y_ = pad_lb_ = pad_rb_ = false;
    for (auto& f : fingers_) f = {};
    prev_interact_ = prev_exit_ = false;
    device_ = InputDevice::Keyboard;
}

void InputSystem::set_key(int keycode, bool down) noexcept {
    device_ = InputDevice::Keyboard;
    // Simple mapping: W A S D, Shift, Space, E (interact), F (exit), Ctrl (crouch)
    switch (keycode) {
    case 'W': case 'w': key_w_ = down; break;
    case 'A': case 'a': key_a_ = down; break;
    case 'S': case 's': key_s_ = down; break;
    case 'D': case 'd': key_d_ = down; break;
    case 16:  key_shift_ = down; break; // Shift
    case 32:  key_space_ = down; break; // Space
    case 'E': case 'e': key_e_ = down; break;
    case 'F': case 'f': key_f_ = down; break;
    case 17:  key_ctrl_ = down; break;  // Ctrl
    default: break;
    }
}

void InputSystem::set_gamepad_axis(float left_x, float left_y, float right_x, float right_y) noexcept {
    device_ = InputDevice::Gamepad;
    pad_lx_ = std::clamp(left_x, -1.f, 1.f);
    pad_ly_ = std::clamp(left_y, -1.f, 1.f);
    pad_rx_ = std::clamp(right_x, -1.f, 1.f);
    pad_ry_ = std::clamp(right_y, -1.f, 1.f);
}

void InputSystem::set_gamepad_button(int button, bool down) noexcept {
    device_ = InputDevice::Gamepad;
    // 0=A interact, 1=B exit, 2=X, 3=Y, 4=LB crouch, 5=RB sprint
    switch (button) {
    case 0: pad_a_ = down; break;
    case 1: pad_b_ = down; break;
    case 2: pad_x_ = down; break;
    case 3: pad_y_ = down; break;
    case 4: pad_lb_ = down; break;
    case 5: pad_rb_ = down; break;
    default: break;
    }
}

void InputSystem::set_touch(int finger_id, bool down, float norm_x, float norm_y) noexcept {
    device_ = InputDevice::Touch;
    for (auto& f : fingers_) {
        if (f.id == finger_id || (!f.down && down)) {
            f.id = finger_id;
            f.down = down;
            f.x = std::clamp(norm_x, 0.f, 1.f);
            f.y = std::clamp(norm_y, 0.f, 1.f);
            if (!down) f.id = -1;
            return;
        }
    }
}

PlayerInput InputSystem::poll() noexcept {
    PlayerInput in;

    // Movement from keyboard
    float mx = 0.f, mz = 0.f;
    if (key_a_) mx -= 1.f;
    if (key_d_) mx += 1.f;
    if (key_w_) mz += 1.f;
    if (key_s_) mz -= 1.f;

    // Gamepad left stick overrides / blends
    if (std::fabs(pad_lx_) > 0.15f || std::fabs(pad_ly_) > 0.15f) {
        mx = pad_lx_;
        mz = -pad_ly_; // screen Y down → forward
    }

    // Touch virtual stick (left half of screen)
    for (const auto& f : fingers_) {
        if (!f.down) continue;
        if (f.x < 0.45f) {
            mx = (f.x - 0.22f) / 0.22f;
            mz = (0.75f - f.y) / 0.25f;
            mx = std::clamp(mx, -1.f, 1.f);
            mz = std::clamp(mz, -1.f, 1.f);
        }
    }

    // Normalize diagonal
    const float len = std::sqrt(mx * mx + mz * mz);
    if (len > 1.f) { mx /= len; mz /= len; }

    in.move_x = mx;
    in.move_z = mz;

    // Look from gamepad right stick or touch right half
    in.look_yaw = pad_rx_ * 1.4f;
    in.look_pitch = pad_ry_ * 1.0f;
    for (const auto& f : fingers_) {
        if (!f.down) continue;
        if (f.x > 0.55f) {
            in.look_yaw = (f.x - 0.78f) / 0.22f * 1.6f;
            in.look_pitch = (0.5f - f.y) / 0.3f;
        }
    }

    in.sprint = key_shift_ || pad_rb_;
    in.jump = key_space_ || pad_y_;
    in.crouch = key_ctrl_ || pad_lb_;

    // One-shot interact / exit with edge detection
    const bool interact_held = key_e_ || pad_a_;
    const bool exit_held = key_f_ || pad_b_;

    // Touch right-side tap buttons (top = interact, bottom = exit)
    for (const auto& f : fingers_) {
        if (!f.down) continue;
        if (f.x > 0.82f && f.y < 0.35f) { /* interact zone */ }
        if (f.x > 0.82f && f.y > 0.65f) { /* exit zone */ }
    }

    in.interact = interact_held && !prev_interact_;
    in.exit = exit_held && !prev_exit_;
    prev_interact_ = interact_held;
    prev_exit_ = exit_held;

    return in;
}

} // namespace exworld
