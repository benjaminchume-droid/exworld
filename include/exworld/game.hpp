#pragma once

#include "exgine/playable.hpp"
#include "exgine/runtime.hpp"
#include "exgine/exanimation.hpp"
#include "exgine/exsound.hpp"

#include "exworld/player.hpp"
#include "exworld/vehicle.hpp"
#include "exworld/world.hpp"
#include "exworld/animation_driver.hpp"
#include "exworld/sound_director.hpp"
#include "exworld/wanted.hpp"
#include "exworld/camera.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace exworld {

class ExWorldGame {
public:
    explicit ExWorldGame(exgine::ProjectSourceLoader loader = {});

    [[nodiscard]] bool open(std::string_view content_root = "content");
    [[nodiscard]] bool start() noexcept;

    [[nodiscard]] bool update(double dt) noexcept;
    [[nodiscard]] bool build_frame(exgine::RenderFrame& frame, exgine::RenderResult& result) noexcept;

    // Inject real input from platform later
    void set_input(const PlayerInput& input) noexcept { pending_input_ = input; }

    [[nodiscard]] bool ready() const noexcept { return ready_; }
    [[nodiscard]] Player& player() noexcept { return player_; }
    [[nodiscard]] const Player& player() const noexcept { return player_; }
    [[nodiscard]] World& world() noexcept { return world_; }
    [[nodiscard]] SoundDirector& sound() noexcept { return sound_; }
    [[nodiscard]] AnimationDriver& animation() noexcept { return animation_; }
    [[nodiscard]] WantedSystem& wanted() noexcept { return wanted_; }
    [[nodiscard]] GameCamera& camera() noexcept { return camera_; }

    [[nodiscard]] exgine::PlayableGame& engine() noexcept { return engine_; }
    [[nodiscard]] const exgine::PlayableGame& engine() const noexcept { return engine_; }

private:
    exgine::ProjectSourceLoader loader_;
    exgine::PlayableGame engine_;

    World world_;
    Player player_;
    VehicleController vehicles_;
    AnimationDriver animation_;
    SoundDirector sound_;
    WantedSystem wanted_;
    GameCamera camera_;

    PlayerInput pending_input_{};
    bool ready_ = false;
    bool configured_ = false;
    double time_ = 0.0;

    [[nodiscard]] bool configure_systems();
    [[nodiscard]] bool spawn_world_content();
    void handle_interactions(float dt, exgine::Runtime& runtime);
    void update_camera(exgine::Runtime& runtime);
};

} // namespace exworld
