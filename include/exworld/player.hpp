#pragma once

#include "exgine/runtime.hpp"
#include "exgine/character.hpp"
#include "exgine/exanimation.hpp"

#include <optional>

namespace exworld {

enum class PlayerMode : std::uint8_t {
    OnFoot,
    EnteringVehicle,
    InVehicle,
    ExitingVehicle,
    EnteringBuilding,
    InsideBuilding
};

struct PlayerInput {
    float move_x = 0;      // -1 .. 1
    float move_z = 0;      // -1 .. 1
    float look_yaw = 0;
    float look_pitch = 0;
    bool sprint = false;
    bool jump = false;
    bool interact = false; // enter vehicle / door
    bool exit = false;     // exit vehicle / building
};

class Player {
public:
    void bind(exgine::Runtime& runtime, exgine::EntityId entity);
    void set_mode(PlayerMode mode) noexcept { mode_ = mode; }

    [[nodiscard]] PlayerMode mode() const noexcept { return mode_; }
    [[nodiscard]] exgine::EntityId entity() const noexcept { return entity_; }
    [[nodiscard]] bool valid() const noexcept { return entity_ != exgine::invalid_entity; }

    // Called every frame
    void update(const PlayerInput& input, float dt, exgine::Runtime& runtime);

    // Vehicle possession
    [[nodiscard]] bool try_enter_vehicle(exgine::EntityId vehicle, exgine::Runtime& runtime);
    [[nodiscard]] bool try_exit_vehicle(exgine::Runtime& runtime);

    // Building entry
    [[nodiscard]] bool try_enter_building(exgine::EntityId building, exgine::Runtime& runtime);
    [[nodiscard]] bool try_exit_building(exgine::Runtime& runtime);

    [[nodiscard]] const exgine::MotionState& motion() const noexcept { return motion_; }
    [[nodiscard]] exgine::EntityId current_vehicle() const noexcept { return vehicle_; }
    [[nodiscard]] exgine::EntityId current_building() const noexcept { return building_; }

private:
    exgine::EntityId entity_ = exgine::invalid_entity;
    exgine::EntityId vehicle_ = exgine::invalid_entity;
    exgine::EntityId building_ = exgine::invalid_entity;
    PlayerMode mode_ = PlayerMode::OnFoot;
    exgine::MotionState motion_{};
    float enter_timer_ = 0.f;
    float exit_timer_ = 0.f;

    void update_on_foot(const PlayerInput& input, float dt, exgine::Runtime& runtime);
    void update_vehicle(const PlayerInput& input, float dt, exgine::Runtime& runtime);
};

} // namespace exworld
