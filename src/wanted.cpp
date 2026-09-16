#include "exworld/wanted.hpp"

#include <algorithm>

namespace exworld {

void WantedSystem::reset() noexcept {
    level_ = WantedLevel::None;
    heat_ = 0.f;
}

void WantedSystem::add_heat(float amount) noexcept {
    heat_ = std::clamp(heat_ + amount, 0.f, 100.f);
    if (heat_ >= 80.f) level_ = WantedLevel::Level5;
    else if (heat_ >= 60.f) level_ = WantedLevel::Level4;
    else if (heat_ >= 40.f) level_ = WantedLevel::Level3;
    else if (heat_ >= 20.f) level_ = WantedLevel::Level2;
    else if (heat_ >= 5.f)  level_ = WantedLevel::Level1;
    else level_ = WantedLevel::None;
}

void WantedSystem::decay(float dt) noexcept {
    if (heat_ <= 0.f) return;
    heat_ = std::max(0.f, heat_ - decay_rate_ * dt);
    if (heat_ < 5.f) level_ = WantedLevel::None;
    else if (heat_ < 20.f) level_ = WantedLevel::Level1;
    else if (heat_ < 40.f) level_ = WantedLevel::Level2;
    else if (heat_ < 60.f) level_ = WantedLevel::Level3;
    else if (heat_ < 80.f) level_ = WantedLevel::Level4;
}

void WantedSystem::set_level(WantedLevel level) noexcept {
    level_ = level;
    switch (level) {
    case WantedLevel::None:   heat_ = 0.f; break;
    case WantedLevel::Level1: heat_ = 10.f; break;
    case WantedLevel::Level2: heat_ = 25.f; break;
    case WantedLevel::Level3: heat_ = 45.f; break;
    case WantedLevel::Level4: heat_ = 65.f; break;
    case WantedLevel::Level5: heat_ = 90.f; break;
    }
}

void WantedSystem::on_crime(float severity) noexcept {
    add_heat(severity);
}

void WantedSystem::update(float dt, exgine::Runtime& /*runtime*/) {
    decay(dt);
    // Future: spawn police units, set NPC flee/attack flags, radio chatter
}

} // namespace exworld
