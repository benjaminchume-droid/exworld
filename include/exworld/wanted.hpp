#pragma once

#include "exgine/runtime.hpp"

#include <cstdint>

namespace exworld {

// Free-roam / GTA-style wanted foundation
enum class WantedLevel : std::uint8_t {
    None = 0,
    Level1,  // minor attention
    Level2,
    Level3,
    Level4,
    Level5   // full heat
};

class WantedSystem {
public:
    void reset() noexcept;

    void add_heat(float amount) noexcept;
    void decay(float dt) noexcept;

    void set_level(WantedLevel level) noexcept;
    [[nodiscard]] WantedLevel level() const noexcept { return level_; }
    [[nodiscard]] float heat() const noexcept { return heat_; }

    // Called when player commits a "crime" (hit NPC, steal car, etc.)
    void on_crime(float severity) noexcept;

    // Future: spawn police, set NPC reaction flags, play wanted radio
    void update(float dt, exgine::Runtime& runtime);

private:
    WantedLevel level_ = WantedLevel::None;
    float heat_ = 0.f;
    float decay_rate_ = 2.5f; // heat points per second when not committing crime
};

} // namespace exworld
