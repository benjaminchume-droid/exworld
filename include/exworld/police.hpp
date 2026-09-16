#pragma once

#include "exgine/runtime.hpp"
#include "exworld/wanted.hpp"

#include <vector>

namespace exworld {

enum class PoliceState : std::uint8_t {
    Idle,
    Patrol,
    Alert,
    Chase,
    Search,
    Arrest
};

struct PoliceUnit {
    exgine::EntityId entity = exgine::invalid_entity;
    PoliceState state = PoliceState::Idle;
    float reaction_timer = 0.f;
    float speed = 4.5f;
};

// Minimal police AI driven by WantedSystem heat.
class PoliceAI {
public:
    void clear();
    void register_unit(exgine::EntityId npc);

    // Spawn extra units when wanted escalates (uses existing NPC pool for now)
    void on_wanted_changed(WantedLevel level, exgine::Runtime& runtime);

    void update(float dt, const exgine::Vec3& player_pos, WantedLevel level,
                exgine::Runtime& runtime);

    [[nodiscard]] std::size_t active_chasers() const noexcept;

private:
    std::vector<PoliceUnit> units_;
    WantedLevel last_level_ = WantedLevel::None;

    void assign_chase(PoliceUnit& u, const exgine::Vec3& target, float dt,
                      exgine::Runtime& runtime);
};

} // namespace exworld
