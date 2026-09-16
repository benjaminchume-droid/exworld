#include "exworld/animation_driver.hpp"

#include <iostream>

namespace exworld {

bool AnimationDriver::initialize(exgine::Runtime& runtime, exgine::EntityId entity) {
    entity_ = entity;
    skeleton_ = exgine::make_humanoid_skeleton();
    if (!skeleton_.valid()) return false;

    if (!runtime.attach_skeleton(entity_, skeleton_)) {
        std::cerr << "EXWORLD: failed to attach skeleton\n";
        return false;
    }

    exanim_ = std::make_unique<exgine::ExAnimation>(skeleton_);
    if (!generate_clips(runtime)) return false;

    // Start in idle
    state_ = AnimState::Idle;
    if (idle_clip_ && !runtime.play_animation(entity_, idle_clip_, 0.1f))
        return false;

    return true;
}

bool AnimationDriver::generate_clips(exgine::Runtime& runtime) {
    // Locomotion via ExAnimation procedural graphs
    {
        auto graph = exgine::motion_catalog(1, skeleton_); // walk-like
        auto clip = exanim_->generate(graph, 1001);
        clip.name = "exworld.walk";
        if (!clip.valid(skeleton_) || !runtime.define_animation(clip)) return false;
        walk_clip_ = clip.id;
    }
    {
        auto graph = exgine::motion_catalog(2, skeleton_); // run/sprint-like
        auto clip = exanim_->generate(graph, 1002);
        clip.name = "exworld.run";
        if (!clip.valid(skeleton_) || !runtime.define_animation(clip)) return false;
        run_clip_ = clip.id;
    }
    {
        auto graph = exgine::motion_catalog(0, skeleton_); // idle
        auto clip = exanim_->generate(graph, 1000);
        clip.name = "exworld.idle";
        if (!clip.valid(skeleton_) || !runtime.define_animation(clip)) return false;
        idle_clip_ = clip.id;
    }
    {
        // Enter vehicle — slightly longer, more deliberate motion
        auto graph = exgine::motion_catalog(0xE11, skeleton_);
        graph.duration = 1.1f;
        graph.looping = false;
        auto clip = exanim_->generate(graph, 1101);
        clip.name = "exworld.enter_vehicle";
        clip.looping = false;
        if (!clip.valid(skeleton_) || !runtime.define_animation(clip)) return false;
        enter_vehicle_clip_ = clip.id;
    }
    {
        auto graph = exgine::motion_catalog(0xE12, skeleton_);
        graph.duration = 0.9f;
        graph.looping = false;
        auto clip = exanim_->generate(graph, 1102);
        clip.name = "exworld.exit_vehicle";
        clip.looping = false;
        if (!clip.valid(skeleton_) || !runtime.define_animation(clip)) return false;
        exit_vehicle_clip_ = clip.id;
    }

    return true;
}

void AnimationDriver::set_state(AnimState state) noexcept {
    previous_ = state_;
    state_ = state;
}

void AnimationDriver::play_enter_vehicle(float duration) {
    state_ = AnimState::EnterVehicle;
    action_playing_ = true;
    action_timer_ = 0.f;
    action_duration_ = duration;
}

void AnimationDriver::play_exit_vehicle(float duration) {
    state_ = AnimState::ExitVehicle;
    action_playing_ = true;
    action_timer_ = 0.f;
    action_duration_ = duration;
}

void AnimationDriver::play_enter_building(float duration) {
    state_ = AnimState::EnterBuilding;
    action_playing_ = true;
    action_timer_ = 0.f;
    action_duration_ = duration;
}

void AnimationDriver::update(float dt, const exgine::MotionState& motion, exgine::Runtime& runtime) {
    if (entity_ == exgine::invalid_entity || dt <= 0.f) return;

    if (action_playing_) {
        action_timer_ += dt;
        if (action_timer_ >= action_duration_) {
            action_playing_ = false;
            // Fall back to locomotion after one-shot
            state_ = motion.sprinting ? AnimState::Sprint :
                     (motion.speed > 0.4f ? AnimState::Walk : AnimState::Idle);
        }
    } else {
        // Choose locomotion from motion state
        if (motion.sprinting && motion.speed > 2.5f)
            state_ = AnimState::Sprint;
        else if (motion.speed > 0.35f)
            state_ = AnimState::Walk;
        else
            state_ = AnimState::Idle;
    }

    // Play the appropriate clip when state changes
    if (state_ != previous_) {
        exgine::AnimationClipId clip = idle_clip_;
        float blend = 0.12f;

        switch (state_) {
        case AnimState::Idle:          clip = idle_clip_; break;
        case AnimState::Walk:          clip = walk_clip_; break;
        case AnimState::Run:
        case AnimState::Sprint:        clip = run_clip_; break;
        case AnimState::EnterVehicle:  clip = enter_vehicle_clip_; blend = 0.08f; break;
        case AnimState::ExitVehicle:   clip = exit_vehicle_clip_;  blend = 0.08f; break;
        default: break;
        }

        if (clip)
            (void)runtime.play_animation(entity_, clip, blend);

        previous_ = state_;
    }

    (void)runtime.update_animation(entity_, dt);
}

} // namespace exworld
