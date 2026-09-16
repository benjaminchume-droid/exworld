#pragma once

#include "exgine/runtime.hpp"

#include <string>
#include <vector>

namespace exworld {

struct InteriorRoom {
    std::string name;
    exgine::Vec3 center{};
    exgine::Vec3 half_extents{2.f, 1.4f, 2.f};
    bool accessible = true;
};

struct BuildingInterior {
    exgine::EntityId building = exgine::invalid_entity;
    std::vector<InteriorRoom> rooms;
    int current_room = 0;
    bool active = false;
};

// Simple room-to-room navigation inside enterable buildings.
class InteriorNavigator {
public:
    void clear();
    void register_building(exgine::EntityId id, int floors, int rooms_per_floor,
                           const exgine::Vec3& origin);

    // Called when player enters a building
    bool activate(exgine::EntityId building, exgine::Runtime& runtime);
    void deactivate();

    // Move player between rooms with simple collision against room bounds
    void update(float dt, float move_x, float move_z, exgine::EntityId player,
                exgine::Runtime& runtime);

    [[nodiscard]] bool active() const noexcept { return active_building_ != exgine::invalid_entity; }
    [[nodiscard]] const BuildingInterior* current() const;

private:
    std::vector<BuildingInterior> interiors_;
    exgine::EntityId active_building_ = exgine::invalid_entity;

    BuildingInterior* find(exgine::EntityId id);
};

} // namespace exworld
