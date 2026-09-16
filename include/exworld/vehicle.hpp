#pragma once

#include "exgine/runtime.hpp"
#include "exgine/vehicle.hpp"

#include <string>
#include <vector>

namespace exworld {

struct VehicleSeat {
    exgine::Vec3 local_offset{0.4f, 0.6f, 0.1f};
    float enter_duration = 1.1f;
    float exit_duration = 0.9f;
};

class VehicleController {
public:
    void register_vehicle(exgine::Runtime& runtime, exgine::EntityId id,
                          std::string name, VehicleSeat seat = {});

    [[nodiscard]] exgine::EntityId nearest_vehicle(const exgine::Vec3& pos,
                                                   float max_distance,
                                                   exgine::Runtime& runtime) const;

    // Called while player is inside
    void update_driven(exgine::EntityId vehicle, float throttle, float steer,
                       float brake, float dt, exgine::Runtime& runtime);

    [[nodiscard]] const VehicleSeat* seat(exgine::EntityId id) const;

private:
    struct Entry {
        exgine::EntityId id = exgine::invalid_entity;
        std::string name;
        VehicleSeat seat;
    };
    std::vector<Entry> vehicles_;
};

} // namespace exworld
