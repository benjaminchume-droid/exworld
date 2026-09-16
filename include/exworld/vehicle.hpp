#pragma once

#include "exgine/runtime.hpp"
#include "exgine/vehicle.hpp"

#include <string>
#include <vector>

namespace exworld {

struct VehicleSeat {
    exgine::Vec3 local_offset{0.35f, 0.55f, 0.15f};
    float enter_duration = 1.15f;
    float exit_duration = 0.95f;
    float interact_radius = 2.6f;
};

class VehicleController {
public:
    void clear();
    void register_vehicle(exgine::Runtime& runtime, exgine::EntityId id,
                          std::string name, VehicleSeat seat = {});

    [[nodiscard]] exgine::EntityId nearest_vehicle(const exgine::Vec3& pos,
                                                   float max_distance,
                                                   exgine::Runtime& runtime) const;

    // While player is driving
    void update_driven(exgine::EntityId vehicle, float throttle, float steer,
                       float brake, float dt, exgine::Runtime& runtime);

    [[nodiscard]] const VehicleSeat* seat(exgine::EntityId id) const;
    [[nodiscard]] std::size_t count() const noexcept { return vehicles_.size(); }

private:
    struct Entry {
        exgine::EntityId id = exgine::invalid_entity;
        std::string name;
        VehicleSeat seat;
    };
    std::vector<Entry> vehicles_;
};

} // namespace exworld
