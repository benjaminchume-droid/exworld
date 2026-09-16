#pragma once

#include "exworld/player.hpp"
#include "exworld/wanted.hpp"

#include <cstdint>
#include <string>

namespace exworld {

// Virtual on-screen control regions (normalized 0..1). Drawn by overlay later;
// InputSystem uses the same layout.
struct HudLayout {
    // Left move stick
    float move_cx = 0.22f;
    float move_cy = 0.72f;
    float move_r = 0.14f;
    // Right look stick
    float look_cx = 0.78f;
    float look_cy = 0.55f;
    float look_r = 0.14f;
    // Interact (E) top-right
    float interact_x0 = 0.82f, interact_y0 = 0.05f, interact_x1 = 0.98f, interact_y1 = 0.28f;
    // Exit (F) bottom-right
    float exit_x0 = 0.82f, exit_y0 = 0.72f, exit_x1 = 0.98f, exit_y1 = 0.95f;
    // Sprint
    float sprint_x0 = 0.82f, sprint_y0 = 0.38f, sprint_x1 = 0.98f, sprint_y1 = 0.55f;
};

struct HudState {
    HudLayout layout{};
    bool show_prompt = false;
    std::string prompt; // "Enter vehicle" / "Enter building"
    int wanted_stars = 0;
    float speed_kmh = 0.f;
    bool in_vehicle = false;
    bool in_building = false;
    std::string region_name = "Downtown";
};

class Hud {
public:
    void update(const Player& player, const WantedSystem& wanted, float speed_mps,
                std::string_view region);
    [[nodiscard]] const HudState& state() const noexcept { return state_; }
    [[nodiscard]] const HudLayout& layout() const noexcept { return state_.layout; }

private:
    HudState state_{};
};

} // namespace exworld
