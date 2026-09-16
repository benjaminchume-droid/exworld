#pragma once

#include "exgine/runtime.hpp"
#include "exgine/exanimation.hpp"
#include "exgine/animation.hpp"

#include <memory>

namespace exworld {

enum class AnimState : std::uint8_t {
    Idle,
    Walk,
    Run,
    Sprint,
    Jump,
    EnterVehicle,
    ExitVehicle,
    EnterBuilding,
    ExitBuilding,
    Drive,
    Custom
};

class AnimationDriver {
public:
    [[nodiscard]] bool initialize(exgine::Runtime& runtime, exgine::EntityId entity);

    void set_state(AnimState state) noexcept;
    [[nodiscard]] AnimState state() const noexcept { return state_; }

    void update(float dt, const exgine::MotionState& motion, exgine::Runtime& runtime);

    void play_enter_vehicle(float duration = 1.15f);
    void play_exit_vehicle(float duration = 0.95f);
    void play_enter_building(float duration = 0.85f);
    void play_exit_building(float duration = 0.75f);

private:
    exgine::EntityId entity_ = exgine::invalid_entity;
    exgine::Skeleton skeleton_{};
    std::unique_ptr<exgine::ExAnimation> exanim_;
    AnimState state_ = AnimState::Idle;
    AnimState previous_ = AnimState::Idle;

    exgine::AnimationClipId idle_clip_ = 0;
    exgine::AnimationClipId walk_clip_ = 0;
    exgine::AnimationClipId run_clip_ = 0;
    exgine::AnimationClipId enter_vehicle_clip_ = 0;
    exgine::AnimationClipId exit_vehicle_clip_ = 0;
    exgine::AnimationClipId enter_building_clip_ = 0;

    float action_timer_ = 0.f;
    float action_duration_ = 0.f;
    bool action_playing_ = false;

    [[nodiscard]] bool generate_clips(exgine::Runtime& runtime);
};

} // namespace exworld
