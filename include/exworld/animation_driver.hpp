#pragma once

#include "exgine/runtime.hpp"
#include "exgine/exanimation.hpp"
#include "exgine/animation.hpp"

#include <memory>
#include <string>

namespace exworld {

// High-level animation states for GTA-style gameplay
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
    // Attach a humanoid skeleton + generate base locomotion set via ExAnimation
    [[nodiscard]] bool initialize(exgine::Runtime& runtime, exgine::EntityId entity);

    void set_state(AnimState state) noexcept;
    [[nodiscard]] AnimState state() const noexcept { return state_; }

    // Drive every frame from player motion + mode
    void update(float dt, const exgine::MotionState& motion, exgine::Runtime& runtime);

    // Explicit one-shot actions (enter/exit vehicle etc.)
    void play_enter_vehicle(float duration = 1.1f);
    void play_exit_vehicle(float duration = 0.9f);
    void play_enter_building(float duration = 0.8f);

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

    float action_timer_ = 0.f;
    float action_duration_ = 0.f;
    bool action_playing_ = false;

    [[nodiscard]] bool generate_clips(exgine::Runtime& runtime);
};

} // namespace exworld
