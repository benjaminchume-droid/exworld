#include "exworld/animation_driver.hpp"

#include <iostream>

namespace exworld {

bool AnimationDriver::initialize(exgine::Runtime& runtime, exgine::EntityId entity) {
    entity_ = entity;
    skeleton_ = exgine::make_humanoid_skeleton();
    if (!skeleton_.valid()) {
        std::cerr << "EXWORLD: humanoid skeleton invalid\n";
        return false;
    }

    if (!runtime.attach_skeleton(entity_, skeleton_)) {
        std::cerr << "EXWORLD: attach_skeleton failed (continuing without attach)\n";
        // still try to generate clips for later
    }

    exanim_ = std::make_unique<exgine::ExAnimation>(skeleton_);
    if (!generate_clips(runtime)) {
        std::cerr << "EXWORLD: generate_clips partial/failed\n";
        // allow running with whatever clips succeeded
    }

    state_ = AnimState::Idle;
    if (idle_clip_)
        (void)runtime.play_animation(entity_, idle_clip_, 0.1f);

    // Success if we at least have a skeleton — clips are best-effort
    return skeleton_.valid();
}

bool AnimationDriver::generate_clips(exgine::Runtime& runtime) {
    auto make = [&](std::uint32_t catalog_id, exgine::AnimationClipId id,
                    const char* name, float duration, bool looping) -> exgine::AnimationClipId {
        if (!exanim_) return 0;
        auto graph = exgine::motion_catalog(catalog_id, skeleton_);
        graph.duration = duration;
        graph.looping = looping;
        auto clip = exanim_->generate(graph, id);
        clip.name = name;
        clip.looping = looping;
        if (!clip.valid(skeleton_) || !runtime.define_animation(clip)) return 0;
        return clip.id;
    };

    idle_clip_           = make(0,     2000, "exworld.idle",           1.0f,  true);
    walk_clip_           = make(1,     2001, "exworld.walk",           0.8f,  true);
    run_clip_            = make(2,     2002, "exworld.run",            0.55f, true);
    enter_vehicle_clip_  = make(0xE11, 2101, "exworld.enter_vehicle",  1.15f, false);
    exit_vehicle_clip_   = make(0xE12, 2102, "exworld.exit_vehicle",   0.95f, false);
    enter_building_clip_ = make(0xB01, 2201, "exworld.enter_building", 0.85f, false);

    return idle_clip_ != 0 || walk_clip_ != 0;
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

void AnimationDriver::play_exit_building(float duration) {
    state_ = AnimState::ExitBuilding;
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
            state_ = motion.sprinting ? AnimState::Sprint :
                     (motion.speed > 0.4f ? AnimState::Walk : AnimState::Idle);
        }
    } else if (state_ != AnimState::Drive) {
        if (motion.sprinting && motion.speed > 2.5f)
            state_ = AnimState::Sprint;
        else if (motion.speed > 0.35f)
            state_ = AnimState::Walk;
        else
            state_ = AnimState::Idle;
    }

    if (state_ != previous_) {
        exgine::AnimationClipId clip = idle_clip_;
        float blend = 0.12f;
        switch (state_) {
        case AnimState::Idle:          clip = idle_clip_; break;
        case AnimState::Walk:          clip = walk_clip_; break;
        case AnimState::Run:
        case AnimState::Sprint:        clip = run_clip_; break;
        case AnimState::EnterVehicle:  clip = enter_vehicle_clip_; blend = 0.07f; break;
        case AnimState::ExitVehicle:   clip = exit_vehicle_clip_;  blend = 0.07f; break;
        case AnimState::EnterBuilding: clip = enter_building_clip_; blend = 0.08f; break;
        default: break;
        }
        if (clip) (void)runtime.play_animation(entity_, clip, blend);
        previous_ = state_;
    }

    (void)runtime.update_animation(entity_, dt);
}

} // namespace exworld
