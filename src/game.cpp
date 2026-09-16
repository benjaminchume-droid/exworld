#include "exworld/game.hpp"

#include <filesystem>
#include <iostream>

namespace exworld {

namespace {

exgine::EntityId find_by_name(exgine::Runtime& r, std::string_view name) {
    for (auto id : r.state().entities.ids()) {
        auto* e = r.state().entities.get(id);
        if (e && e->name == name) return id;
    }
    return exgine::invalid_entity;
}

exgine::EntityId find_by_kind(exgine::Runtime& r, exgine::NodeKind kind) {
    for (auto id : r.state().entities.ids()) {
        auto* e = r.state().entities.get(id);
        if (e && e->kind == kind) return id;
    }
    return exgine::invalid_entity;
}

} // namespace

ExWorldGame::ExWorldGame(exgine::ProjectSourceLoader loader)
    : loader_(std::move(loader)), engine_({}, loader_) {}

bool ExWorldGame::open(std::string_view content_root) {
    ready_ = configured_ = false;

    const std::filesystem::path root(content_root);
    const auto manifest_path = root / "project.exg";

    std::string manifest;
    if (!loader_(manifest_path.string(), manifest) && !loader_("project.exg", manifest)) {
        std::cerr << "EXWORLD: project.exg not found under " << content_root << "\n";
        return false;
    }

    if (!engine_.open_project(manifest)) {
        std::cerr << "EXWORLD: PlayableGame failed to open project\n";
        return false;
    }

    if (!configure_systems()) return false;
    if (!spawn_world_content()) return false;

    configured_ = true;
    return engine_.show_menu();
}

bool ExWorldGame::start() noexcept {
    ready_ = configured_ && engine_.start();
    return ready_;
}

bool ExWorldGame::configure_systems() {
    auto& runtime = engine_.session().game().runtime();

    auto player_id = find_by_name(runtime, "Player");
    if (!player_id) player_id = find_by_kind(runtime, exgine::NodeKind::Player);
    if (!player_id) {
        std::cerr << "EXWORLD: no Player entity in scene\n";
        return false;
    }

    player_.bind(runtime, player_id);

    if (!animation_.initialize(runtime, player_id)) {
        std::cerr << "EXWORLD: AnimationDriver failed\n";
        return false;
    }

    sound_.initialize(runtime);
    world_.bootstrap(runtime);
    return true;
}

bool ExWorldGame::spawn_world_content() {
    auto& runtime = engine_.session().game().runtime();
    for (auto id : runtime.state().entities.ids()) {
        auto* e = runtime.state().entities.get(id);
        if (!e) continue;
        if (e->kind == exgine::NodeKind::Building) {
            world_.register_building(id, {e->transform.x, e->transform.y, e->transform.z + 2.0f});
        }
    }
    return true;
}

bool ExWorldGame::update(double dt) noexcept {
    if (!ready_ || dt < 0.0) return false;

    time_ += dt;
    auto& runtime = engine_.session().game().runtime();

    PlayerInput input;
    input.move_z = 0.6f;
    input.sprint = (static_cast<int>(time_ * 0.4) % 8) > 5;
    input.interact = false;

    player_.update(input, static_cast<float>(dt), runtime);
    animation_.update(static_cast<float>(dt), player_.motion(), runtime);

    const auto* e = runtime.state().entities.get(player_.entity());
    exgine::Vec3 pos{0, 0, 0};
    if (e) pos = {e->transform.x, e->transform.y, e->transform.z};

    const bool in_vehicle = player_.mode() == PlayerMode::InVehicle;
    sound_.update(static_cast<float>(dt), pos, player_.motion().speed, in_vehicle,
                  in_vehicle ? 1800.f : 0.f, runtime);
    world_.set_stream_focus(pos, runtime);

    return engine_.update(dt);
}

bool ExWorldGame::build_frame(exgine::RenderFrame& frame, exgine::RenderResult& result) noexcept {
    if (!ready_) return false;

    // Build the same frame for both desktop validation and the Android GLES presenter.
    // The presenter owns the actual GPU submission/swap operation.
    exgine::Renderer renderer({exgine::RenderBackend::OpenGLES, 1280, 720, true, true, 256, 128});
    if (!renderer.build_frame(engine_.session().game().runtime(), frame)) return false;
    if (!frame.valid()) return false;

    result = {};
    result.success = true;
    result.stats.submitted_draws = frame.draws.size();
    result.stats.visible_draws = frame.draws.size();
    for (const auto& draw : frame.draws) {
        if (draw.animated()) ++result.stats.animated_draws;
    }
    result.stats.submitted_lights = frame.lights.size();
    return true;
}

} // namespace exworld
