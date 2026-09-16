#pragma once

#include "exgine/runtime.hpp"
#include "exgine/exsound.hpp"

#include <cstdint>

namespace exworld {

class SoundDirector {
public:
    void initialize(exgine::Runtime& runtime);

    void update(float dt, const exgine::Vec3& listener_pos,
                float player_speed, bool in_vehicle,
                float vehicle_rpm, exgine::Runtime& runtime);

    void play_footstep(exgine::SoundMaterial material, float intensity, float speed,
                       exgine::Runtime& runtime);
    void play_door(bool open, bool metal, exgine::Runtime& runtime);
    void play_vehicle_enter(exgine::Runtime& runtime);
    void play_vehicle_exit(exgine::Runtime& runtime);
    void play_engine(float rpm, float load, exgine::Runtime& runtime);
    void play_impact(exgine::SoundMaterial material, float intensity, exgine::Runtime& runtime);
    void play_ambient_city(exgine::Runtime& runtime);
    void play_wanted_alert(exgine::Runtime& runtime);

    void set_listener(const exgine::Vec3& pos, const exgine::Vec3& forward,
                      const exgine::Vec3& up, exgine::Runtime& runtime);

private:
    bool started_ = false;
    float footstep_clock_ = 0.f;
    float ambient_clock_ = 0.f;
    float engine_clock_ = 0.f;
    std::uint32_t seed_ = 0xE58A11u;

    void fire(const exgine::SoundEventParams& p, exgine::Runtime& runtime);
};

} // namespace exworld
