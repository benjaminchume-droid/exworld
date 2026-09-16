#include "exworld/input.hpp"

#include <algorithm>
#include <cmath>

namespace exworld {

namespace {
// Must match HudLayout in hud.hpp
constexpr float kMoveCx = 0.22f, kMoveCy = 0.72f, kMoveR = 0.16f;
constexpr float kLookCx = 0.78f, kLookCy = 0.55f, kLookR = 0.16f;
bool in_rect(float x, float y, float x0, float y0, float x1, float y1) {
    return x >= x0 && x <= x1 && y >= y0 && y <= y1;
}
} // namespace

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
    switch (keycode) {
    case 'W': case 'w': key_w_ = down; break;
    case 'A': case 'a': key_a_ = down; break;
    case 'S': case 's': key_s_ = down; break;
    case 'D': case 'd': key_d_ = down; break;
    case 16:  key_shift_ = down; break;
    case 32:  key_space_ = down; break;
    case 'E': case 'e': key_e_ = down; break;
    case 'F': case 'f': key_f_ = down; break;
    case 17:  key_ctrl_ = down; break;
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

    float mx = 0.f, mz = 0.f;
    if (key_a_) mx -= 1.f;
    if (key_d_) mx += 1.f;
    if (key_w_) mz += 1.f;
    if (key_s_) mz -= 1.f;

    if (std::fabs(pad_lx_) > 0.15f || std::fabs(pad_ly_) > 0.15f) {
        mx = pad_lx_;
        mz = -pad_ly_;
    }

    bool touch_interact = false;
    bool touch_exit = false;
    bool touch_sprint = false;

    for (const auto& f : fingers_) {
        if (!f.down) continue;

        // Move stick (left)
        const float mdx = f.x - kMoveCx;
        const float mdy = f.y - kMoveCy;
        if (mdx * mdx + mdy * mdy < kMoveR * kMoveR * 2.5f && f.x < 0.5f) {
            mx = std::clamp(mdx / kMoveR, -1.f, 1.f);
            mz = std::clamp(-mdy / kMoveR, -1.f, 1.f);
        }

        // Look stick (right)
        const float ldx = f.x - kLookCx;
        const float ldy = f.y - kLookCy;
        if (ldx * ldx + ldy * ldy < kLookR * kLookR * 2.5f && f.x > 0.5f && f.x < 0.82f) {
            in.look_yaw = std::clamp(ldx / kLookR, -1.f, 1.f) * 1.6f;
            in.look_pitch = std::clamp(-ldy / kLookR, -1.f, 1.f);
        }

        // Buttons
        if (in_rect(f.x, f.y, 0.82f, 0.05f, 0.98f, 0.28f)) touch_interact = true;
        if (in_rect(f.x, f.y, 0.82f, 0.72f, 0.98f, 0.95f)) touch_exit = true;
        if (in_rect(f.x, f.y, 0.82f, 0.38f, 0.98f, 0.55f)) touch_sprint = true;
    }

    if (std::fabs(pad_rx_) > 0.1f || std::fabs(pad_ry_) > 0.1f) {
        in.look_yaw = pad_rx_ * 1.4f;
        in.look_pitch = pad_ry_ * 1.0f;
    }

    const float len = std::sqrt(mx * mx + mz * mz);
    if (len > 1.f) { mx /= len; mz /= len; }
    in.move_x = mx;
    in.move_z = mz;

    in.sprint = key_shift_ || pad_rb_ || touch_sprint;
    in.jump = key_space_ || pad_y_;
    in.crouch = key_ctrl_ || pad_lb_;

    const bool interact_held = key_e_ || pad_a_ || touch_interact;
    const bool exit_held = key_f_ || pad_b_ || touch_exit;
    in.interact = interact_held && !prev_interact_;
    in.exit = exit_held && !prev_exit_;
    prev_interact_ = interact_held;
    prev_exit_ = exit_held;

    return in;
}

} // namespace exworld
