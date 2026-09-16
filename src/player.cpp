#include "exworld/player.hpp"

#include <algorithm>
#include <cmath>

namespace exworld {

void Player::bind(exgine::Runtime& runtime, exgine::EntityId entity, exgine::CharacterId character) {
    entity_ = entity;
    character_ = character;
    mode_ = PlayerMode::OnFoot;
    vehicle_ = building_ = exgine::invalid_entity;
    motion_ = {};
    yaw_ = 0.f;
    (void)runtime;
}

exgine::Vec3 Player::position(const exgine::Runtime& runtime) const {
    auto* e = runtime.state().entities.get(entity_);
    if (!e) return {};
    return {e->transform.x, e->transform.y, e->transform.z};
}

float Player::enter_progress() const noexcept {
    if (enter_duration_ <= 0.f) return 1.f;
    return 1.f - std::clamp(enter_timer_ / enter_duration_, 0.f, 1.f);
}

float Player::exit_progress() const noexcept {
    if (exit_duration_ <= 0.f) return 1.f;
    return 1.f - std::clamp(exit_timer_ / exit_duration_, 0.f, 1.f);
}

void Player::update(const PlayerInput& input, float dt, exgine::Runtime& runtime) {
    if (!valid() || dt <= 0.f) return;

    switch (mode_) {
    case PlayerMode::OnFoot:
        update_on_foot(input, dt, runtime);
        break;
    case PlayerMode::InVehicle:
        update_in_vehicle(input, dt, runtime);
        break;
    case PlayerMode::EnteringVehicle:
    case PlayerMode::ExitingVehicle:
        update_transition(dt);
        break;
    case PlayerMode::EnteringBuilding:
        // short transition then inside
        enter_timer_ -= dt;
        if (enter_timer_ <= 0.f) mode_ = PlayerMode::InsideBuilding;
        break;
    case PlayerMode::InsideBuilding:
        // limited movement inside for now
        update_on_foot(input, dt, runtime);
        break;
    default:
        break;
    }
}

void Player::update_on_foot(const PlayerInput& input, float dt, exgine::Runtime& runtime) {
    // Prefer the real character controller when available
    if (character_ != exgine::invalid_character) {
        exgine::CharacterControllerInput cinput;
        cinput.move_x = input.move_x;
        cinput.move_z = input.move_z;
        cinput.run = input.sprint;
        cinput.crouch = input.crouch;
        cinput.jump = input.jump;
        (void)runtime.update_character(character_, cinput, dt);
    }

    auto* e = runtime.state().entities.get(entity_);
    if (!e) return;

    // If character controller didn't move the entity yet, apply light kinematic fallback
    if (character_ == exgine::invalid_character) {
        const float speed = input.sprint ? 6.2f : 3.1f;
        e->transform.x += input.move_x * speed * dt;
        e->transform.z += input.move_z * speed * dt;
    } else {
        // Sync entity transform from character if possible (future: pull from CharacterControllerState)
    }

    yaw_ += input.look_yaw;

    const float planar = std::sqrt(input.move_x * input.move_x + input.move_z * input.move_z);
    const float speed = planar * (input.sprint ? 6.2f : 3.1f);

    motion_.speed = speed;
    motion_.vertical_speed = 0.f;
    motion_.grounded = true;
    motion_.sprinting = input.sprint && planar > 0.2f;
    motion_.crouch = input.crouch ? 1.f : 0.f;
    motion_.aim = 0.f;
    motion_.turn = yaw_;

    if (e->scene_node) {
        (void)runtime.scene().set_local_transform(
            e->scene_node,
            {{e->transform.x, e->transform.y, e->transform.z}, {}, {1, 1, 1}});
    }
}

void Player::update_in_vehicle(const PlayerInput& /*input*/, float /*dt*/, exgine::Runtime& runtime) {
    // While in vehicle the vehicle controller owns motion.
    // Keep player entity snapped to vehicle seat.
    auto* v = runtime.state().entities.get(vehicle_);
    auto* e = runtime.state().entities.get(entity_);
    if (!v || !e) return;

    e->transform.x = v->transform.x;
    e->transform.y = v->transform.y + 0.55f;
    e->transform.z = v->transform.z;

    motion_.speed = 0.f;
    motion_.sprinting = false;
    motion_.grounded = true;
}

void Player::update_transition(float dt) {
    if (mode_ == PlayerMode::EnteringVehicle) {
        enter_timer_ -= dt;
        if (enter_timer_ <= 0.f) mode_ = PlayerMode::InVehicle;
    } else if (mode_ == PlayerMode::ExitingVehicle) {
        exit_timer_ -= dt;
        if (exit_timer_ <= 0.f) {
            mode_ = PlayerMode::OnFoot;
            vehicle_ = exgine::invalid_entity;
        }
    }
}

bool Player::try_enter_vehicle(exgine::EntityId vehicle, float enter_duration,
                               exgine::Runtime& runtime) {
    if (mode_ != PlayerMode::OnFoot || vehicle == exgine::invalid_entity) return false;
    auto* v = runtime.state().entities.get(vehicle);
    if (!v || v->kind != exgine::NodeKind::Vehicle) return false;

    vehicle_ = vehicle;
    mode_ = PlayerMode::EnteringVehicle;
    enter_duration_ = enter_duration > 0.1f ? enter_duration : 1.15f;
    enter_timer_ = enter_duration_;
    return true;
}

bool Player::try_exit_vehicle(float exit_duration, exgine::Runtime& runtime) {
    if (mode_ != PlayerMode::InVehicle || vehicle_ == exgine::invalid_entity) return false;
    (void)runtime;
    mode_ = PlayerMode::ExitingVehicle;
    exit_duration_ = exit_duration > 0.1f ? exit_duration : 0.95f;
    exit_timer_ = exit_duration_;
    return true;
}

bool Player::try_enter_building(exgine::EntityId building, exgine::Runtime& runtime) {
    if (mode_ != PlayerMode::OnFoot || building == exgine::invalid_entity) return false;
    auto* b = runtime.state().entities.get(building);
    if (!b || b->kind != exgine::NodeKind::Building) return false;

    building_ = building;
    mode_ = PlayerMode::EnteringBuilding;
    enter_duration_ = 0.85f;
    enter_timer_ = enter_duration_;
    return true;
}

bool Player::try_exit_building(exgine::Runtime& /*runtime*/) {
    if (mode_ != PlayerMode::InsideBuilding) return false;
    mode_ = PlayerMode::OnFoot;
    building_ = exgine::invalid_entity;
    return true;
}

} // namespace exworld
