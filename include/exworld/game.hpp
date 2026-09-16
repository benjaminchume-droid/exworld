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

#include <memory>
#include <string>
#include <string_view>

namespace exworld {

class ExWorldGame {
public:
    explicit ExWorldGame(exgine::ProjectSourceLoader loader = {});

    // Boot the full open-world game from content/project.exg
    [[nodiscard]] bool open(std::string_view content_root = "content");
    [[nodiscard]] bool start() noexcept;

    // Main loop
    [[nodiscard]] bool update(double dt) noexcept;
    [[nodiscard]] bool build_frame(exgine::RenderFrame& frame, exgine::RenderResult& result) noexcept;

    // High-level state
    [[nodiscard]] bool ready() const noexcept { return ready_; }
    [[nodiscard]] Player& player() noexcept { return player_; }
    [[nodiscard]] const Player& player() const noexcept { return player_; }
    [[nodiscard]] World& world() noexcept { return world_; }
    [[nodiscard]] SoundDirector& sound() noexcept { return sound_; }
    [[nodiscard]] AnimationDriver& animation() noexcept { return animation_; }

    [[nodiscard]] exgine::PlayableGame& engine() noexcept { return engine_; }
    [[nodiscard]] const exgine::PlayableGame& engine() const noexcept { return engine_; }

private:
    exgine::ProjectSourceLoader loader_;
    exgine::PlayableGame engine_;

    World world_;
    Player player_;
    AnimationDriver animation_;
    SoundDirector sound_;

    bool ready_ = false;
    bool configured_ = false;
    double time_ = 0.0;

    [[nodiscard]] bool configure_systems();
    [[nodiscard]] bool spawn_world_content();
};

} // namespace exworld
