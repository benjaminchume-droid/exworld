#include "exworld/police.hpp"

#include <cmath>

namespace exworld {

void PoliceAI::clear() {
    units_.clear();
    last_level_ = WantedLevel::None;
}

void PoliceAI::register_unit(exgine::EntityId npc) {
    PoliceUnit u;
    u.entity = npc;
    u.state = PoliceState::Patrol;
    units_.push_back(u);
}

void PoliceAI::on_wanted_changed(WantedLevel level, exgine::Runtime& /*runtime*/) {
    if (level == last_level_) return;
    last_level_ = level;

    // Escalate behaviour
    for (auto& u : units_) {
        if (level >= WantedLevel::Level2) {
            u.state = PoliceState::Alert;
            u.speed = 5.5f;
        }
        if (level >= WantedLevel::Level3) {
            u.state = PoliceState::Chase;
            u.speed = 7.0f;
        }
        if (level == WantedLevel::None) {
            u.state = PoliceState::Patrol;
            u.speed = 3.5f;
        }
    }
}

void PoliceAI::assign_chase(PoliceUnit& u, const exgine::Vec3& target, float dt,
                            exgine::Runtime& runtime) {
    auto* e = runtime.state().entities.get(u.entity);
    if (!e) return;

    const float dx = target.x - e->transform.x;
    const float dz = target.z - e->transform.z;
    const float dist = std::sqrt(dx * dx + dz * dz);
    if (dist < 0.4f) {
        u.state = PoliceState::Arrest;
        return;
    }

    const float inv = 1.f / std::max(dist, 0.001f);
    e->transform.x += dx * inv * u.speed * dt;
    e->transform.z += dz * inv * u.speed * dt;

    if (e->scene_node) {
        (void)runtime.scene().set_local_transform(
            e->scene_node,
            {{e->transform.x, e->transform.y, e->transform.z}, {}, {1, 1, 1}});
    }
}

void PoliceAI::update(float dt, const exgine::Vec3& player_pos, WantedLevel level,
                      exgine::Runtime& runtime) {
    on_wanted_changed(level, runtime);

    for (auto& u : units_) {
        switch (u.state) {
        case PoliceState::Chase:
        case PoliceState::Alert:
            assign_chase(u, player_pos, dt, runtime);
            break;
        case PoliceState::Patrol:
            // simple idle wander later
            break;
        case PoliceState::Arrest:
            // future: trigger arrest UI / fail state
            break;
        default:
            break;
        }
    }
}

std::size_t PoliceAI::active_chasers() const noexcept {
    std::size_t n = 0;
    for (const auto& u : units_)
        if (u.state == PoliceState::Chase || u.state == PoliceState::Alert) ++n;
    return n;
}

} // namespace exworld
