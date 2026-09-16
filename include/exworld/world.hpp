#pragma once

#include "exgine/runtime.hpp"
#include "exgine/world_render.hpp"

#include <string>
#include <vector>

namespace exworld {

struct BuildingDoor {
    exgine::EntityId building = exgine::invalid_entity;
    exgine::Vec3 world_position{};
    float radius = 1.9f;
    bool locked = false;
    bool interior_active = false;
};

class World {
public:
    void bootstrap(exgine::Runtime& runtime);
    void clear();

    void register_building(exgine::EntityId id, exgine::Vec3 door_pos, float radius = 1.9f);

    [[nodiscard]] exgine::EntityId nearest_door(const exgine::Vec3& pos, float max_dist) const;
    [[nodiscard]] const BuildingDoor* door(exgine::EntityId building) const;
    BuildingDoor* door_mut(exgine::EntityId building);

    void set_stream_focus(const exgine::Vec3& pos, exgine::Runtime& runtime);

    [[nodiscard]] std::size_t building_count() const noexcept { return doors_.size(); }
    [[nodiscard]] std::size_t streamed_terrain_count() const noexcept { return render_bridge_.terrain_entities(); }
    [[nodiscard]] std::size_t streamed_water_count() const noexcept { return render_bridge_.water_entities(); }

private:
    std::vector<BuildingDoor> doors_;
    exgine::WorldRenderBridge render_bridge_;
    exgine::Runtime* runtime_ = nullptr;
};

} // namespace exworld
