#include "exworld/player.hpp"

#include <algorithm>
#include <cmath>

namespace exworld {

void Player::bind(exgine::Runtime& runtime, exgine::EntityId entity) {
    entity_ = entity;
    mode_ = PlayerMode::OnFoot;
    vehicle_ = building_ = exgine::invalid_entity;
    motion_ = {};
    (void)runtime;
}

void Player::update(const PlayerInput& input, float dt, exgine::Runtime& runtime) {
    if (!valid() || dt <= 0.f) return;

    switch (mode_) {
    case PlayerMode::OnFoot:
        update_on_foot(input, dt, runtime);
        break;
    case PlayerMode::InVehicle:
        update_vehicle(input, dt, runtime);
        break;
    case PlayerMode::EnteringVehicle:
        enter_timer_ -= dt;
        if (enter_timer_ <= 0.f) {
            mode_ = PlayerMode::InVehicle;
        }
        break;
    case PlayerMode::ExitingVehicle:
        exit_timer_ -= dt;
        if (exit_timer_ <= 0.f) {
            mode_ = PlayerMode::OnFoot;
            vehicle_ = exgine::invalid_entity;
        }
        break;
    case PlayerMode::EnteringBuilding:
    case PlayerMode::InsideBuilding:
        // Building interior logic expands later
        break;
    default:
        break;
    }
}

void Player::update_on_foot(const PlayerInput& input, float dt, exgine::Runtime& runtime) {
    auto* e = runtime.state().entities.get(entity_);
    if (!e) return;

    // Simple kinematic move for now (physics character controller will replace this)
    const float speed = input.sprint ? 6.5f : 3.2f;
    const float dx = input.move_x * speed * dt;
    const float dz = input.move_z * speed * dt;

    e->transform.x += dx;
    e->transform.z += dz;

    // Update motion state for animation + sound
    const float planar = std::sqrt(dx * dx + dz * dz) / std::max(dt, 0.0001f);
    motion_.speed = planar;
    motion_.vertical_speed = 0.f;
    motion_.grounded = true;
    motion_.sprinting = input.sprint && planar > 1.0f;
    motion_.crouch = 0.f;
    motion_.aim = 0.f;
    motion_.turn = input.look_yaw;

    // Keep scene node in sync if present
    if (e->scene_node) {
        (void)runtime.scene().set_local_transform(
            e->scene_node,
            {{e->transform.x, e->transform.y, e->transform.z}, {}, {1, 1, 1}});
    }
}

void Player::update_vehicle(const PlayerInput& /*input*/, float /*dt*/, exgine::Runtime& /*runtime*/) {
    // Vehicle driving is handled by VehicleController + physics.
    // While inside we just keep motion state low for animation.
    motion_.speed = 0.f;
    motion_.sprinting = false;
    motion_.grounded = true;
}

bool Player::try_enter_vehicle(exgine::EntityId vehicle, exgine::Runtime& runtime) {
    if (mode_ != PlayerMode::OnFoot || vehicle == exgine::invalid_entity) return false;
    auto* v = runtime.state().entities.get(vehicle);
    if (!v || v->kind != exgine::NodeKind::Vehicle) return false;

    vehicle_ = vehicle;
    mode_ = PlayerMode::EnteringVehicle;
    enter_timer_ = 1.1f;
    return true;
}

bool Player::try_exit_vehicle(exgine::Runtime& runtime) {
    if (mode_ != PlayerMode::InVehicle || vehicle_ == exgine::invalid_entity) return false;
    (void)runtime;
    mode_ = PlayerMode::ExitingVehicle;
    exit_timer_ = 0.9f;
    return true;
}

bool Player::try_enter_building(exgine::EntityId building, exgine::Runtime& runtime) {
    if (mode_ != PlayerMode::OnFoot || building == exgine::invalid_entity) return false;
    auto* b = runtime.state().entities.get(building);
    if (!b || b->kind != exgine::NodeKind::Building) return false;

    building_ = building;
    mode_ = PlayerMode::EnteringBuilding;
    return true;
}

bool Player::try_exit_building(exgine::Runtime& /*runtime*/) {
    if (mode_ != PlayerMode::InsideBuilding) return false;
    mode_ = PlayerMode::OnFoot;
    building_ = exgine::invalid_entity;
    return true;
}

} // namespace exworld
