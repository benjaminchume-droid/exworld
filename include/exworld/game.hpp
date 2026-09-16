#pragma once

#include "exgine/playable.hpp"
#include "exgine/runtime.hpp"
#include "exgine/android.hpp"

#include "exworld/player.hpp"
#include "exworld/vehicle.hpp"
#include "exworld/world.hpp"
#include "exworld/animation_driver.hpp"
#include "exworld/sound_director.hpp"
#include "exworld/wanted.hpp"
#include "exworld/camera.hpp"
#include "exworld/input.hpp"
#include "exworld/police.hpp"
#include "exworld/interior.hpp"
#include "exworld/save.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace exworld {

class ExWorldGame {
public:
    explicit ExWorldGame(exgine::ProjectSourceLoader loader = {});

    // Desktop / path-based
    [[nodiscard]] bool open(std::string_view content_root = "content");

    // Android / asset-based: manifest already loaded from AAssetManager
    [[nodiscard]] bool open_from_manifest(std::string_view manifest_text);

    [[nodiscard]] bool start() noexcept;

    [[nodiscard]] bool update(double dt) noexcept;
    [[nodiscard]] bool build_frame(exgine::RenderFrame& frame, exgine::RenderResult& result) noexcept;

    // Real device present (OpenGLES)
    [[nodiscard]] bool present(exgine::AndroidEglPresenter& presenter, int width, int height) noexcept;

    InputSystem& input() noexcept { return input_; }
    void set_input(const PlayerInput& input) noexcept;

    [[nodiscard]] std::vector<std::uint8_t> save_game() const;
    [[nodiscard]] bool load_game(const std::vector<std::uint8_t>& bytes);
    [[nodiscard]] bool save_to_file(const std::string& path) const;
    [[nodiscard]] bool load_from_file(const std::string& path);

    [[nodiscard]] bool ready() const noexcept { return ready_; }
    [[nodiscard]] Player& player() noexcept { return player_; }
    [[nodiscard]] WantedSystem& wanted() noexcept { return wanted_; }
    [[nodiscard]] PoliceAI& police() noexcept { return police_; }
    [[nodiscard]] GameCamera& camera() noexcept { return camera_; }
    [[nodiscard]] InteriorNavigator& interiors() noexcept { return interiors_; }
    [[nodiscard]] exgine::PlayableGame& engine() noexcept { return engine_; }

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
    InputSystem input_;
    PoliceAI police_;
    InteriorNavigator interiors_;
    SaveSystem saves_;

    PlayerInput forced_input_{};
    bool use_forced_input_ = false;
    bool ready_ = false;
    bool configured_ = false;
    double time_ = 0.0;

    [[nodiscard]] bool configure_systems();
    [[nodiscard]] bool spawn_world_content();
    void handle_interactions(float dt, exgine::Runtime& runtime);
    void update_vehicle_possession(float dt, const PlayerInput& in, exgine::Runtime& runtime);
    void update_camera(exgine::Runtime& runtime);
};

} // namespace exworld
