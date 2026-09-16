#include "exworld/game.hpp"

#include "exgine/android.hpp"
#include "exgine/render.hpp"
#include "exgine/materials.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string_view>

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

exgine::EntityId find_player_entity(exgine::Runtime& r) {
    static constexpr const char* kNames[] = {
        "Sebastian", "Player", "Explorer", "DawnOfLight"
    };
    for (const char* n : kNames) {
        auto id = find_by_name(r, n);
        if (!id) continue;
        auto* e = r.state().entities.get(id);
        if (!e) continue;
        if (e->kind == exgine::NodeKind::Player || e->name == "Sebastian" ||
            e->name == "Player" || e->name == "Explorer")
            return id;
    }
    if (auto id = find_by_name(r, "Sebastian")) return id;
    if (auto id = find_by_kind(r, exgine::NodeKind::Player)) return id;
    return exgine::invalid_entity;
}

bool ensure_render_basics(exgine::Runtime& runtime, const exgine::Vec3& focus) {
    // Camera (required by Renderer::build_frame)
    exgine::Camera cam;
    cam.position = {focus.x - 4.f, focus.y + 2.5f, focus.z + 4.f};
    cam.rotation = {-0.25f, 0.6f, 0.f};
    cam.vertical_fov_degrees = 58.f;
    cam.near_plane = 0.08f;
    cam.far_plane = 4000.f;
    if (!runtime.set_main_camera(cam)) {
        std::cerr << "EXWORLD: set_main_camera failed\n";
        return false;
    }

    // Sun
    exgine::Light sun;
    sun.type = exgine::LightType::Directional;
    sun.direction = {-0.45f, -0.78f, -0.2f};
    sun.color = {1.f, 0.92f, 0.78f};
    sun.intensity = 4.f;
    (void)runtime.create_light(sun);

    // Materials so geometry parts resolve (build_frame returns false on missing material)
    (void)runtime.define_material(exgine::make_real_world_material("concrete", 11));
    (void)runtime.define_material(exgine::make_real_world_material("asphalt", 22));
    (void)runtime.define_material(exgine::make_real_world_material("metal", 33));
    (void)runtime.define_material(exgine::make_real_world_material("paint", 44));
    (void)runtime.define_material(exgine::make_real_world_material("glass", 55));
    (void)runtime.define_material(exgine::make_real_world_material("skin", 66));
    (void)runtime.define_material(exgine::make_real_world_material("exworld_surface", 77));
    return true;
}

} // namespace

ExWorldGame::ExWorldGame(exgine::ProjectSourceLoader loader)
    : loader_(std::move(loader)), engine_({}, loader_) {}

void ExWorldGame::set_input(const PlayerInput& input) noexcept {
    forced_input_ = input;
    use_forced_input_ = true;
}

bool ExWorldGame::open(std::string_view content_root) {
    ready_ = configured_ = false;
    const std::filesystem::path root(content_root);
    std::string manifest;
    if (!loader_((root / "project.exg").string(), manifest) &&
        !loader_("project.exg", manifest) &&
        !loader_("first_light/project.exg", manifest)) {
        std::cerr << "EXWORLD: project.exg not found\n";
        return false;
    }
    return open_from_manifest(manifest);
}

bool ExWorldGame::open_from_manifest(std::string_view manifest_text) {
    ready_ = configured_ = false;

    if (!engine_.open_project(manifest_text)) {
        std::cerr << "EXWORLD: PlayableGame::open_project failed\n";
        return false;
    }

    (void)engine_.show_menu(); // may fail if already Menu — ignore

    if (!configure_systems()) {
        std::cerr << "EXWORLD: configure_systems soft-failed (continuing)\n";
    }
    (void)spawn_world_content();

    configured_ = true;
    return true;
}

bool ExWorldGame::start() noexcept {
    ready_ = configured_ && engine_.start();
    if (ready_ && player_.valid()) {
        auto& runtime = engine_.session().game().runtime();
        camera_.reset(player_.position(runtime));
        (void)runtime.set_main_camera(camera_.camera());
    }
    return ready_;
}

bool ExWorldGame::configure_systems() {
    auto& runtime = engine_.session().game().runtime();

    auto player_id = find_player_entity(runtime);
    if (!player_id) {
        std::cerr << "EXWORLD: no playable entity (Sebastian/Player)\n";
        return false;
    }

    exgine::CharacterId cid = exgine::invalid_character;
    const auto& bindings = runtime.character_bindings();
    auto it = bindings.find(player_id);
    if (it != bindings.end()) cid = it->second;

    player_.bind(runtime, player_id, cid);

    const auto pos = player_.position(runtime);
    if (!ensure_render_basics(runtime, pos)) {
        std::cerr << "EXWORLD: render basics failed\n";
    }

    if (!animation_.initialize(runtime, player_id)) {
        std::cerr << "EXWORLD: AnimationDriver unavailable (ok)\n";
    }

    sound_.initialize(runtime);
    world_.bootstrap(runtime);
    wanted_.reset();
    police_.clear();
    interiors_.clear();
    input_.reset();
    return true;
}

bool ExWorldGame::spawn_world_content() {
    auto& runtime = engine_.session().game().runtime();
    vehicles_.clear();
    world_.clear();
    police_.clear();
    interiors_.clear();

    for (auto id : runtime.state().entities.ids()) {
        auto* e = runtime.state().entities.get(id);
        if (!e) continue;
        if (e->kind == exgine::NodeKind::Building) {
            world_.register_building(
                id, {e->transform.x, e->transform.y, e->transform.z + 2.2f}, 2.1f);
            interiors_.register_building(
                id, 3, 4, {e->transform.x, e->transform.y, e->transform.z});
        }
        if (e->kind == exgine::NodeKind::Vehicle)
            vehicles_.register_vehicle(runtime, id, e->name, {});
        if (e->kind == exgine::NodeKind::NPC && e->name != "DawnOfLight")
            police_.register_unit(id);
    }
    return true;
}

void ExWorldGame::handle_interactions(float, exgine::Runtime& runtime) {
    if (!player_.valid()) return;
    PlayerInput in = use_forced_input_ ? forced_input_ : input_.poll();
    const auto pos = player_.position(runtime);

    if (in.interact && player_.mode() == PlayerMode::OnFoot) {
        auto veh = vehicles_.nearest_vehicle(pos, 3.5f, runtime);
        if (veh != exgine::invalid_entity) {
            const auto* seat = vehicles_.seat(veh);
            const float dur = seat ? seat->enter_duration : 1.15f;
            if (player_.try_enter_vehicle(veh, dur, runtime)) {
                animation_.play_enter_vehicle(dur);
                sound_.play_vehicle_enter(runtime);
                wanted_.on_crime(14.f);
                if (wanted_.level() >= WantedLevel::Level1)
                    sound_.play_wanted_alert(runtime);
            }
        } else {
            auto bld = world_.nearest_door(pos, 2.5f);
            if (bld != exgine::invalid_entity && player_.try_enter_building(bld, runtime)) {
                animation_.play_enter_building(0.85f);
                sound_.play_door(true, false, runtime);
                interiors_.activate(bld, runtime);
            }
        }
    }

    if (in.exit) {
        if (player_.mode() == PlayerMode::InVehicle) {
            const auto* seat = vehicles_.seat(player_.current_vehicle());
            const float dur = seat ? seat->exit_duration : 0.95f;
            if (player_.try_exit_vehicle(dur, runtime)) {
                animation_.play_exit_vehicle(dur);
                sound_.play_vehicle_exit(runtime);
                auto& dyn = runtime.vehicle_dynamics();
                auto it = dyn.find(player_.current_vehicle());
                if (it != dyn.end() && it->second) (void)it->second->possess(false);
            }
        } else if (player_.mode() == PlayerMode::InsideBuilding) {
            if (player_.try_exit_building(runtime)) {
                animation_.play_exit_building(0.75f);
                sound_.play_door(false, false, runtime);
                interiors_.deactivate();
            }
        }
    }
}

void ExWorldGame::update_vehicle_possession(float dt, const PlayerInput& in,
                                            exgine::Runtime& runtime) {
    if (player_.mode() != PlayerMode::InVehicle) return;
    const float throttle = std::max(0.f, in.move_z);
    const float brake = in.move_z < -0.1f ? -in.move_z : (in.crouch ? 1.f : 0.f);
    vehicles_.update_driven(player_.current_vehicle(), throttle, in.move_x, brake, dt, runtime);
}

void ExWorldGame::update_camera(exgine::Runtime& runtime) {
    if (!player_.valid()) return;
    const auto pos = player_.position(runtime);
    PlayerInput in = use_forced_input_ ? forced_input_ : input_.poll();
    camera_.set_look(camera_.yaw() + in.look_yaw * 0.025f,
                     camera_.pitch() + in.look_pitch * 0.025f);
    const bool in_veh = player_.mode() == PlayerMode::InVehicle ||
                        player_.mode() == PlayerMode::EnteringVehicle;
    camera_.update(1.f / 60.f, pos, in_veh, runtime);
}

bool ExWorldGame::update(double dt) noexcept {
    if (!ready_ || dt < 0.0) return false;
    time_ += dt;
    auto& runtime = engine_.session().game().runtime();
    PlayerInput in = use_forced_input_ ? forced_input_ : input_.poll();

    if (player_.valid()) {
        handle_interactions(static_cast<float>(dt), runtime);
        update_vehicle_possession(static_cast<float>(dt), in, runtime);

        if (player_.mode() == PlayerMode::InsideBuilding && interiors_.active()) {
            interiors_.update(static_cast<float>(dt), in.move_x, in.move_z,
                              player_.entity(), runtime);
        } else {
            player_.update(in, static_cast<float>(dt), runtime);
        }

        animation_.update(static_cast<float>(dt), player_.motion(), runtime);

        const auto pos = player_.position(runtime);
        const bool in_vehicle = player_.mode() == PlayerMode::InVehicle;
        sound_.update(static_cast<float>(dt), pos, player_.motion().speed, in_vehicle,
                      in_vehicle ? 2200.f : 0.f, runtime);
        world_.set_stream_focus(pos, runtime);
        police_.update(static_cast<float>(dt), pos, wanted_.level(), runtime);
        update_camera(runtime);
    }

    wanted_.update(static_cast<float>(dt), runtime);
    use_forced_input_ = false;
    return engine_.update(dt);
}

bool ExWorldGame::build_frame(exgine::RenderFrame& frame, exgine::RenderResult& result) noexcept {
    if (!ready_) return false;
    exgine::Renderer renderer({exgine::RenderBackend::Headless, 1280, 720, true, true, 256, 128});
    if (!renderer.build_frame(engine_.session().game().runtime(), frame)) return false;
    result = renderer.submit(frame);
    return result.success;
}

bool ExWorldGame::present(exgine::AndroidEglPresenter& presenter, int width, int height) noexcept {
    if (!ready_) return false;
    const std::uint32_t w = width > 0 ? static_cast<std::uint32_t>(width) : 1280u;
    const std::uint32_t h = height > 0 ? static_cast<std::uint32_t>(height) : 720u;
    exgine::RenderFrame frame;
    exgine::Renderer renderer({exgine::RenderBackend::OpenGLES, w, h, true, true, 256, 128});
    auto& rt = engine_.session().game().runtime();
    if (!renderer.build_frame(rt, frame)) {
        // Last-resort: still try a camera-only frame so EGL keeps swapping
        (void)ensure_render_basics(rt, player_.valid() ? player_.position(rt) : exgine::Vec3{0, 2, 0});
        if (!renderer.build_frame(rt, frame)) return false;
    }
    if (!renderer.validate(frame)) return false;
    return presenter.present(frame);
}

std::vector<std::uint8_t> ExWorldGame::save_game() const {
    return saves_.save(player_, wanted_, time_, engine_.session().game().runtime());
}

bool ExWorldGame::load_game(const std::vector<std::uint8_t>& bytes) {
    return saves_.load(bytes, player_, wanted_, time_, engine_.session().game().runtime());
}

bool ExWorldGame::save_to_file(const std::string& path) const {
    return saves_.save_to_file(path, player_, wanted_, time_, engine_.session().game().runtime());
}

bool ExWorldGame::load_from_file(const std::string& path) {
    return saves_.load_from_file(path, player_, wanted_, time_, engine_.session().game().runtime());
}

} // namespace exworld
