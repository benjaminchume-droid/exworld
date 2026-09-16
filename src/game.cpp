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

void ExWorldGame::set_input(const PlayerInput& input) noexcept {
    forced_input_ = input;
    use_forced_input_ = true;
}

bool ExWorldGame::open(std::string_view content_root) {
    ready_ = configured_ = false;

    const std::filesystem::path root(content_root);
    std::string manifest;
    if (!loader_((root / "project.exg").string(), manifest) && !loader_("project.exg", manifest)) {
        std::cerr << "EXWORLD: project.exg not found\n";
        return false;
    }

    if (!engine_.open_project(manifest)) {
        std::cerr << "EXWORLD: failed to open project\n";
        return false;
    }

    if (!configure_systems()) return false;
    if (!spawn_world_content()) return false;

    configured_ = true;
    return engine_.show_menu();
}

bool ExWorldGame::start() noexcept {
    ready_ = configured_ && engine_.start();
    if (ready_) {
        auto& runtime = engine_.session().game().runtime();
        camera_.reset(player_.position(runtime));
    }
    return ready_;
}

bool ExWorldGame::configure_systems() {
    auto& runtime = engine_.session().game().runtime();

    auto player_id = find_by_name(runtime, "Player");
    if (!player_id) player_id = find_by_kind(runtime, exgine::NodeKind::Player);
    if (!player_id) {
        std::cerr << "EXWORLD: no Player entity\n";
        return false;
    }

    exgine::CharacterId cid = exgine::invalid_character;
    const auto& bindings = runtime.character_bindings();
    auto it = bindings.find(player_id);
    if (it != bindings.end()) cid = it->second;

    player_.bind(runtime, player_id, cid);

    if (!animation_.initialize(runtime, player_id)) {
        std::cerr << "EXWORLD: AnimationDriver failed\n";
        return false;
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
            const int floors = 3;
            const int rooms = 4;
            world_.register_building(
                id,
                {e->transform.x, e->transform.y, e->transform.z + 2.2f},
                2.1f);
            interiors_.register_building(
                id, floors, rooms,
                {e->transform.x, e->transform.y, e->transform.z});
        }
        if (e->kind == exgine::NodeKind::Vehicle) {
            vehicles_.register_vehicle(runtime, id, e->name, {});
        }
        if (e->kind == exgine::NodeKind::NPC) {
            // Treat some NPCs as police units for wanted escalation
            police_.register_unit(id);
        }
    }

    std::cout << "EXWORLD: buildings=" << world_.building_count()
              << " vehicles=" << vehicles_.count()
              << " police_units=" << police_.active_chasers() << "\n";
    return true;
}

void ExWorldGame::handle_interactions(float /*dt*/, exgine::Runtime& runtime) {
    PlayerInput in = use_forced_input_ ? forced_input_ : input_.poll();
    const auto pos = player_.position(runtime);

    if (in.interact) {
        if (player_.mode() == PlayerMode::OnFoot) {
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
                if (bld != exgine::invalid_entity) {
                    if (player_.try_enter_building(bld, runtime)) {
                        animation_.play_enter_building(0.85f);
                        sound_.play_door(true, false, runtime);
                        interiors_.activate(bld, runtime);
                        if (auto* d = world_.door_mut(bld)) d->interior_active = true;
                    }
                }
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
                // Release possession
                auto& dyn = runtime.vehicle_dynamics();
                auto it = dyn.find(player_.current_vehicle());
                if (it != dyn.end() && it->second)
                    (void)it->second->possess(false);
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
    const float steer = in.move_x;

    vehicles_.update_driven(player_.current_vehicle(), throttle, steer, brake, dt, runtime);
}

void ExWorldGame::update_camera(exgine::Runtime& runtime) {
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

    // Soft auto-walk only when no real input is present (validation path)
    if (!use_forced_input_ &&
        in.move_x == 0.f && in.move_z == 0.f &&
        input_.last_device() == InputDevice::Keyboard) {
        // leave still if truly no keys; tests inject via set_input
    }

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
    wanted_.update(static_cast<float>(dt), runtime);
    police_.update(static_cast<float>(dt), pos, wanted_.level(), runtime);
    update_camera(runtime);

    use_forced_input_ = false; // one-shot
    return engine_.update(dt);
}

bool ExWorldGame::build_frame(exgine::RenderFrame& frame, exgine::RenderResult& result) noexcept {
    if (!ready_) return false;
    exgine::Renderer renderer({exgine::RenderBackend::Headless, 1280, 720, true, true, 256, 128});
    if (!renderer.build_frame(engine_.session().game().runtime(), frame)) return false;
    result = renderer.submit(frame);
    return result.success;
}

std::vector<std::uint8_t> ExWorldGame::save_game() const {
    return saves_.save(player_, wanted_, time_, engine_.session().game().runtime());
}

bool ExWorldGame::load_game(const std::vector<std::uint8_t>& bytes) {
    return saves_.load(bytes, player_, wanted_, time_,
                       engine_.session().game().runtime());
}

bool ExWorldGame::save_to_file(const std::string& path) const {
    return saves_.save_to_file(path, player_, wanted_, time_,
                               engine_.session().game().runtime());
}

bool ExWorldGame::load_from_file(const std::string& path) {
    return saves_.load_from_file(path, player_, wanted_, time_,
                                 engine_.session().game().runtime());
}

} // namespace exworld
