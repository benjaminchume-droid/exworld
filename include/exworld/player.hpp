#pragma once

#include "exgine/runtime.hpp"
#include "exgine/gameplay.hpp"
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
    float move_x = 0.f;
    float move_z = 0.f;
    float look_yaw = 0.f;
    float look_pitch = 0.f;
    bool sprint = false;
    bool jump = false;
    bool crouch = false;
    bool interact = false; // enter vehicle / door
    bool exit = false;     // exit vehicle / building
};

class Player {
public:
    void bind(exgine::Runtime& runtime, exgine::EntityId entity, exgine::CharacterId character);

    void set_mode(PlayerMode mode) noexcept { mode_ = mode; }
    [[nodiscard]] PlayerMode mode() const noexcept { return mode_; }
    [[nodiscard]] exgine::EntityId entity() const noexcept { return entity_; }
    [[nodiscard]] exgine::CharacterId character() const noexcept { return character_; }
    [[nodiscard]] bool valid() const noexcept { return entity_ != exgine::invalid_entity; }

    void update(const PlayerInput& input, float dt, exgine::Runtime& runtime);

    [[nodiscard]] bool try_enter_vehicle(exgine::EntityId vehicle, float enter_duration,
                                         exgine::Runtime& runtime);
    [[nodiscard]] bool try_exit_vehicle(float exit_duration, exgine::Runtime& runtime);

    [[nodiscard]] bool try_enter_building(exgine::EntityId building, exgine::Runtime& runtime);
    [[nodiscard]] bool try_exit_building(exgine::Runtime& runtime);

    [[nodiscard]] const exgine::MotionState& motion() const noexcept { return motion_; }
    [[nodiscard]] exgine::EntityId current_vehicle() const noexcept { return vehicle_; }
    [[nodiscard]] exgine::EntityId current_building() const noexcept { return building_; }
    [[nodiscard]] float enter_progress() const noexcept;
    [[nodiscard]] float exit_progress() const noexcept;

    // World position helper
    [[nodiscard]] exgine::Vec3 position(const exgine::Runtime& runtime) const;

private:
    exgine::EntityId entity_ = exgine::invalid_entity;
    exgine::CharacterId character_ = exgine::invalid_character;
    exgine::EntityId vehicle_ = exgine::invalid_entity;
    exgine::EntityId building_ = exgine::invalid_entity;
    PlayerMode mode_ = PlayerMode::OnFoot;
    exgine::MotionState motion_{};
    float enter_timer_ = 0.f;
    float enter_duration_ = 1.1f;
    float exit_timer_ = 0.f;
    float exit_duration_ = 0.9f;
    float yaw_ = 0.f;

    void update_on_foot(const PlayerInput& input, float dt, exgine::Runtime& runtime);
    void update_in_vehicle(const PlayerInput& input, float dt, exgine::Runtime& runtime);
    void update_transition(float dt);
};

} // namespace exworld
