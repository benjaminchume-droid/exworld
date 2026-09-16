#include "exworld/interior.hpp"

#include <algorithm>
#include <cmath>

namespace exworld {

void InteriorNavigator::clear() {
    interiors_.clear();
    active_building_ = exgine::invalid_entity;
}

void InteriorNavigator::register_building(exgine::EntityId id, int floors, int rooms_per_floor,
                                          const exgine::Vec3& origin) {
    BuildingInterior bi;
    bi.building = id;
    floors = std::max(1, floors);
    rooms_per_floor = std::max(1, rooms_per_floor);

    int room_id = 0;
    for (int f = 0; f < floors; ++f) {
        for (int r = 0; r < rooms_per_floor; ++r) {
            InteriorRoom room;
            room.name = "F" + std::to_string(f) + "_R" + std::to_string(r);
            const float ox = (r % 2 == 0 ? -2.5f : 2.5f);
            const float oz = (r / 2) * 4.0f;
            room.center = {origin.x + ox, origin.y + 1.2f + f * 3.0f, origin.z + oz};
            room.half_extents = {2.2f, 1.3f, 2.2f};
            bi.rooms.push_back(room);
            ++room_id;
        }
    }
    interiors_.push_back(std::move(bi));
}

BuildingInterior* InteriorNavigator::find(exgine::EntityId id) {
    for (auto& b : interiors_)
        if (b.building == id) return &b;
    return nullptr;
}

bool InteriorNavigator::activate(exgine::EntityId building, exgine::Runtime& runtime) {
    auto* bi = find(building);
    if (!bi || bi->rooms.empty()) return false;

    bi->active = true;
    bi->current_room = 0;
    active_building_ = building;

    // Snap player into first room center
    // (caller still owns the player entity)
    (void)runtime;
    return true;
}

void InteriorNavigator::deactivate() {
    if (auto* bi = find(active_building_)) bi->active = false;
    active_building_ = exgine::invalid_entity;
}

const BuildingInterior* InteriorNavigator::current() const {
    for (const auto& b : interiors_)
        if (b.building == active_building_) return &b;
    return nullptr;
}

void InteriorNavigator::update(float dt, float move_x, float move_z, exgine::EntityId player,
                               exgine::Runtime& runtime) {
    auto* bi = find(active_building_);
    if (!bi || !bi->active || bi->rooms.empty()) return;

    auto* e = runtime.state().entities.get(player);
    if (!e) return;

    auto& room = bi->rooms[static_cast<std::size_t>(bi->current_room)];

    // Move inside room bounds
    const float speed = 2.8f;
    float nx = e->transform.x + move_x * speed * dt;
    float nz = e->transform.z + move_z * speed * dt;

    nx = std::clamp(nx, room.center.x - room.half_extents.x,
                        room.center.x + room.half_extents.x);
    nz = std::clamp(nz, room.center.z - room.half_extents.z,
                        room.center.z + room.half_extents.z);

    e->transform.x = nx;
    e->transform.z = nz;
    e->transform.y = room.center.y;

    // Cross to adjacent room when near edge
    if (move_x > 0.7f && nx > room.center.x + room.half_extents.x - 0.3f) {
        if (bi->current_room + 1 < static_cast<int>(bi->rooms.size()))
            ++bi->current_room;
    } else if (move_x < -0.7f && nx < room.center.x - room.half_extents.x + 0.3f) {
        if (bi->current_room > 0) --bi->current_room;
    }

    if (e->scene_node) {
        (void)runtime.scene().set_local_transform(
            e->scene_node,
            {{e->transform.x, e->transform.y, e->transform.z}, {}, {1, 1, 1}});
    }
}

} // namespace exworld
