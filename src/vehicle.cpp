#include "exworld/vehicle.hpp"

#include <cmath>

namespace exworld {

void VehicleController::register_vehicle(exgine::Runtime& /*runtime*/, exgine::EntityId id,
                                         std::string name, VehicleSeat seat) {
    vehicles_.push_back({id, std::move(name), seat});
}

exgine::EntityId VehicleController::nearest_vehicle(const exgine::Vec3& pos, float max_distance,
                                                    exgine::Runtime& runtime) const {
    exgine::EntityId best = exgine::invalid_entity;
    float best_d = max_distance;

    for (const auto& v : vehicles_) {
        auto* e = runtime.state().entities.get(v.id);
        if (!e) continue;
        const float dx = e->transform.x - pos.x;
        const float dy = e->transform.y - pos.y;
        const float dz = e->transform.z - pos.z;
        const float d = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (d < best_d) {
            best_d = d;
            best = v.id;
        }
    }
    return best;
}

void VehicleController::update_driven(exgine::EntityId vehicle, float throttle, float steer,
                                      float brake, float dt, exgine::Runtime& runtime) {
    auto* e = runtime.state().entities.get(vehicle);
    if (!e) return;

    // Temporary kinematic drive until full vehicle dynamics possession is hooked
    const float speed = (throttle - brake) * 14.0f;
    e->transform.x += std::sin(steer) * speed * dt;
    e->transform.z += std::cos(steer) * speed * dt;

    if (e->scene_node) {
        (void)runtime.scene().set_local_transform(
            e->scene_node,
            {{e->transform.x, e->transform.y, e->transform.z}, {}, {1, 1, 1}});
    }
}

const VehicleSeat* VehicleController::seat(exgine::EntityId id) const {
    for (const auto& v : vehicles_)
        if (v.id == id) return &v.seat;
    return nullptr;
}

} // namespace exworld
