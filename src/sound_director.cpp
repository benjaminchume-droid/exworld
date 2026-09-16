#include "exworld/sound_director.hpp"

#include <algorithm>
#include <cmath>

namespace exworld {

void SoundDirector::initialize(exgine::Runtime& runtime) {
    started_ = runtime.start_audio(48000);
    seed_ = 0xE58A11u;
    footstep_clock_ = ambient_clock_ = engine_clock_ = 0.f;

    // Immediate ambient bed so the world never feels silent
    if (started_) {
        play_ambient_city(runtime);
    }
}

void SoundDirector::fire(const exgine::SoundEventParams& p, exgine::Runtime& runtime) {
    if (!started_) return;
    auto sample = runtime.generate_sound(p);
    if (sample.valid())
        (void)runtime.play_sound(sample);
}

void SoundDirector::play_footstep(exgine::SoundMaterial material, float intensity, float speed,
                                  exgine::Runtime& runtime) {
    exgine::SoundEventParams p;
    p.event = exgine::SoundEvent::Footstep;
    p.material = material;
    p.intensity = std::clamp(intensity, 0.15f, 1.0f);
    p.speed = speed;
    p.mass = 75.f;
    p.room = 0.12f;
    p.variation = std::sin(float(seed_++) * 0.01f);
    p.seed = seed_++;
    fire(p, runtime);
}

void SoundDirector::play_door(bool open, exgine::Runtime& runtime) {
    exgine::SoundEventParams p;
    p.event = exgine::SoundEvent::Door;
    p.material = exgine::SoundMaterial::Wood;
    p.intensity = open ? 0.55f : 0.4f;
    p.seed = seed_++;
    fire(p, runtime);
}

void SoundDirector::play_vehicle_enter(exgine::Runtime& runtime) {
    exgine::SoundEventParams p;
    p.event = exgine::SoundEvent::Door;
    p.material = exgine::SoundMaterial::Metal;
    p.intensity = 0.65f;
    p.seed = seed_++;
    fire(p, runtime);
}

void SoundDirector::play_vehicle_exit(exgine::Runtime& runtime) {
    play_vehicle_enter(runtime); // same door family for now
}

void SoundDirector::play_engine(float rpm, float load, exgine::Runtime& runtime) {
    exgine::SoundEventParams p;
    p.event = exgine::SoundEvent::Vehicle;
    p.intensity = std::clamp(load, 0.1f, 1.0f);
    p.speed = std::max(400.f, rpm);
    p.mass = 1400.f;
    p.room = 0.08f;
    p.seed = seed_++;
    fire(p, runtime);
}

void SoundDirector::play_impact(exgine::SoundMaterial material, float intensity,
                                exgine::Runtime& runtime) {
    exgine::SoundEventParams p;
    p.event = exgine::SoundEvent::Impact;
    p.material = material;
    p.intensity = intensity;
    p.seed = seed_++;
    fire(p, runtime);
}

void SoundDirector::play_ambient_city(exgine::Runtime& runtime) {
    exgine::SoundEventParams p;
    p.event = exgine::SoundEvent::Ambient;
    p.material = exgine::SoundMaterial::Concrete;
    p.intensity = 0.28f;
    p.room = 0.45f;
    p.distance = 0.f;
    p.seed = seed_++;
    fire(p, runtime);
}

void SoundDirector::set_listener(const exgine::Vec3& pos, const exgine::Vec3& forward,
                                 const exgine::Vec3& up, exgine::Runtime& /*runtime*/) {
    // Runtime audio listener is currently driven through the older AudioWorld path
    // in Showcase; we will unify later. For now position is enough for distance.
    (void)pos; (void)forward; (void)up;
}

void SoundDirector::update(float dt, const exgine::Vec3& listener_pos,
                           float player_speed, bool in_vehicle,
                           float vehicle_rpm, exgine::Runtime& runtime) {
    if (!started_ || dt <= 0.f) return;

    (void)listener_pos;

    // Footsteps while on foot
    if (!in_vehicle && player_speed > 0.4f) {
        footstep_clock_ += dt;
        const float interval = player_speed > 4.0f ? 0.28f : 0.42f;
        if (footstep_clock_ >= interval) {
            play_footstep(exgine::SoundMaterial::Concrete,
                          std::clamp(player_speed / 6.0f, 0.25f, 1.0f),
                          player_speed, runtime);
            footstep_clock_ = 0.f;
        }
    }

    // Engine while driving
    if (in_vehicle) {
        engine_clock_ += dt;
        if (engine_clock_ > 0.35f) {
            play_engine(vehicle_rpm, 0.55f, runtime);
            engine_clock_ = 0.f;
        }
    }

    // City ambient bed
    ambient_clock_ += dt;
    if (ambient_clock_ > 4.5f) {
        play_ambient_city(runtime);
        ambient_clock_ = 0.f;
    }
}

} // namespace exworld
