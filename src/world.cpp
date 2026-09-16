#include "exworld/world.hpp"

#include <cmath>

namespace exworld {

void World::bootstrap(exgine::Runtime& runtime) {
    runtime_ = &runtime;
    (void)render_bridge_.sync(runtime, {0.f, 0.f, 0.f});
}

void World::clear() {
    if (runtime_) render_bridge_.clear(*runtime_);
    doors_.clear();
}

void World::register_building(exgine::EntityId id, exgine::Vec3 door_pos, float radius) {
    BuildingDoor d;
    d.building = id;
    d.world_position = door_pos;
    d.radius = radius;
    doors_.push_back(d);
}

exgine::EntityId World::nearest_door(const exgine::Vec3& pos, float max_dist) const {
    exgine::EntityId best = exgine::invalid_entity;
    float best_d = max_dist;
    for (const auto& d : doors_) {
        if (d.locked) continue;
        const float dx = d.world_position.x - pos.x;
        const float dy = d.world_position.y - pos.y;
        const float dz = d.world_position.z - pos.z;
        const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (dist < best_d && dist <= d.radius) {
            best_d = dist;
            best = d.building;
        }
    }
    return best;
}

const BuildingDoor* World::door(exgine::EntityId building) const {
    for (const auto& d : doors_)
        if (d.building == building) return &d;
    return nullptr;
}

BuildingDoor* World::door_mut(exgine::EntityId building) {
    for (auto& d : doors_)
        if (d.building == building) return &d;
    return nullptr;
}

void World::set_stream_focus(const exgine::Vec3& pos, exgine::Runtime& runtime) {
    runtime_ = &runtime;
    (void)runtime.stream_world(pos, 3);
    (void)render_bridge_.sync(runtime, pos);
}

} // namespace exworld
