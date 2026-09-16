#pragma once

#include "exgine/runtime.hpp"

#include <string>
#include <vector>

namespace exworld {

struct BuildingDoor {
    exgine::EntityId building = exgine::invalid_entity;
    exgine::Vec3 world_position{};
    float radius = 1.8f;
    bool locked = false;
};

class World {
public:
    void bootstrap(exgine::Runtime& runtime);

    // Query helpers for gameplay
    [[nodiscard]] exgine::EntityId nearest_door(const exgine::Vec3& pos, float max_dist) const;
    [[nodiscard]] const BuildingDoor* door(exgine::EntityId building) const;

    void register_building(exgine::EntityId id, exgine::Vec3 door_pos, float radius = 1.8f);

    // Open-world streaming focus point (player position)
    void set_stream_focus(const exgine::Vec3& pos, exgine::Runtime& runtime);

private:
    std::vector<BuildingDoor> doors_;
};

} // namespace exworld
