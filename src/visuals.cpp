#include "exworld/visuals.hpp"

#include "exgine/building.hpp"
#include "exgine/character.hpp"
#include "exgine/geometry.hpp"
#include "exgine/material.hpp"
#include "exgine/vehicle.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace exworld {
namespace {

exgine::MeshAssembly box_mesh(exgine::Vec3 size, std::string material) {
    exgine::MeshAssembly a;
    a.parts.push_back({"box", exgine::make_box({size}), std::move(material), {}, {1, 1, 1}, {}});
    return a;
}

exgine::MeshAssembly quad_mesh(float w, float h, std::string material) {
    exgine::Mesh x;
    x.vertices = {
        {{-w * .5f, -h * .5f, 0}, {0, 0, 1}, {0, 0}},
        {{w * .5f, -h * .5f, 0}, {0, 0, 1}, {1, 0}},
        {{w * .5f, h * .5f, 0}, {0, 0, 1}, {1, 1}},
        {{-w * .5f, h * .5f, 0}, {0, 0, 1}, {0, 1}},
    };
    x.indices = {0, 1, 2, 0, 2, 3};
    exgine::MeshAssembly a;
    a.parts.push_back({"quad", std::move(x), std::move(material), {}, {1, 1, 1}, {}});
    return a;
}

exgine::MeshAssembly capsule_body(float height, std::string material) {
    // Humanoid stand-in: capsule ~1.7m tall, radius 0.28m
    const float h = std::max(1.5f, height);
    const float r = 0.28f;
    exgine::MeshAssembly a;
    a.parts.push_back(
        {"body", exgine::make_capsule(r, h - 2.f * r, 16, 6), std::move(material), {0, h * 0.5f, 0}, {1, 1, 1}, {}});
    return a;
}

} // namespace

bool VisualSystem::ensure_materials(exgine::Runtime& runtime) {
    // Distinct seeds → different procedural texture responses
    (void)runtime.define_material(exgine::make_real_world_material("skin", 1001));
    (void)runtime.define_material(exgine::make_real_world_material("hair", 1002));
    (void)runtime.define_material(exgine::make_real_world_material("fabric", 1003));
    (void)runtime.define_material(exgine::make_real_world_material("denim", 1004));
    (void)runtime.define_material(exgine::make_real_world_material("leather", 1005));
    (void)runtime.define_material(exgine::make_real_world_material("concrete", 11));
    (void)runtime.define_material(exgine::make_real_world_material("asphalt", 22));
    (void)runtime.define_material(exgine::make_real_world_material("metal", 33));
    (void)runtime.define_material(exgine::make_real_world_material("paint", 44));
    (void)runtime.define_material(exgine::make_real_world_material("glass", 55));
    (void)runtime.define_material(exgine::make_real_world_material("wood", 66));
    (void)runtime.define_material(exgine::make_real_world_material("grass", 77));
    (void)runtime.define_material(exgine::make_real_world_material("water", 88));
    (void)runtime.define_material(exgine::make_real_world_material("hud", 99));
    (void)runtime.define_material(exgine::make_real_world_material("exworld_surface", 120));
    (void)runtime.define_material(exgine::make_real_world_material("rubber", 130));
    return true;
}

bool VisualSystem::ensure_character(exgine::Runtime& runtime, exgine::EntityId id, bool is_player) {
    auto* e = runtime.state().entities.get(id);
    if (!e) return false;

    // Prefer engine humanoid generator (real multi-part mesh)
    exgine::CharacterDefinition def;
    def.type = is_player ? exgine::CharacterType::Player : exgine::CharacterType::NPC;
    def.appearance.height = is_player ? 1.75f : 1.70f; // meters — smaller relative to buildings
    def.appearance.build = is_player ? 1.0f : 0.95f;
    def.appearance.seed = is_player ? 4242 : (1000 + static_cast<std::uint64_t>(id));
    def.appearance.archetype =
        is_player ? exgine::CharacterArchetype::Civilian : exgine::CharacterArchetype::Civilian;
    if (e->name.rfind("Cop_", 0) == 0)
        def.appearance.archetype = exgine::CharacterArchetype::Police;

    auto assembly = exgine::generate_character(def);
    if (!assembly.valid()) {
        // Fallback capsule body
        assembly = capsule_body(def.appearance.height, "skin");
    }
    if (!runtime.attach_geometry(id, std::move(assembly))) {
        std::cerr << "EXWORLD: attach_geometry failed for " << e->name << "\n";
        return false;
    }
    return true;
}

bool VisualSystem::ensure_ground(exgine::Runtime& runtime, const exgine::Vec3& at) {
    // Large ground plate so the world does not feel like floating grey boxes
    if (ground_ == exgine::invalid_entity) {
        ground_ = runtime.state().entities.create(exgine::NodeKind::Property, "GroundPlate");
    }
    if (!ground_) return false;
    auto* e = runtime.state().entities.get(ground_);
    if (!e) return false;
    e->transform.x = at.x;
    e->transform.y = -0.05f;
    e->transform.z = at.z;
    // 400m x 400m thin slab
    auto mesh = box_mesh({400.f, 0.1f, 400.f}, "asphalt");
    return runtime.attach_geometry(ground_, std::move(mesh));
}

bool VisualSystem::ensure_hud_entities(exgine::Runtime& runtime) {
    auto make = [&](exgine::EntityId& slot, const char* name) {
        if (slot == exgine::invalid_entity)
            slot = runtime.state().entities.create(exgine::NodeKind::Property, name);
        return slot != exgine::invalid_entity;
    };
    if (!make(hud_move_, "HUD_MoveStick")) return false;
    if (!make(hud_look_, "HUD_LookStick")) return false;
    if (!make(hud_interact_, "HUD_Interact")) return false;
    if (!make(hud_exit_, "HUD_Exit")) return false;

    (void)runtime.attach_geometry(hud_move_, quad_mesh(0.35f, 0.35f, "hud"));
    (void)runtime.attach_geometry(hud_look_, quad_mesh(0.35f, 0.35f, "hud"));
    (void)runtime.attach_geometry(hud_interact_, quad_mesh(0.28f, 0.18f, "hud"));
    (void)runtime.attach_geometry(hud_exit_, quad_mesh(0.28f, 0.18f, "hud"));
    return true;
}

bool VisualSystem::ensure_buildings_vehicles(exgine::Runtime& runtime) {
    for (auto id : runtime.state().entities.ids()) {
        auto* e = runtime.state().entities.get(id);
        if (!e) continue;
        if (e->kind == exgine::NodeKind::Building) {
            if (!e->geometry || !e->geometry->valid()) {
                exgine::BuildingConfig cfg;
                cfg.floors = 4;
                cfg.floor_height = 3.0f; // real meters
                cfg.width = 14.f;
                cfg.depth = 12.f;
                cfg.seed = 1000 + static_cast<std::uint64_t>(id);
                (void)runtime.generate_building(id, cfg);
            }
        }
        if (e->kind == exgine::NodeKind::Vehicle) {
            if (!e->geometry || !e->geometry->valid()) {
                exgine::VehicleConfig cfg;
                cfg.seed = 2000 + static_cast<std::uint64_t>(id);
                (void)runtime.generate_vehicle(id, cfg);
            }
        }
    }
    return true;
}

bool VisualSystem::bootstrap(exgine::Runtime& runtime, exgine::EntityId player) {
    ensure_materials(runtime);

    if (player) {
        if (!ensure_character(runtime, player, true))
            std::cerr << "EXWORLD: player mesh failed\n";
    }

    for (auto id : runtime.state().entities.ids()) {
        auto* e = runtime.state().entities.get(id);
        if (!e) continue;
        if (e->kind == exgine::NodeKind::NPC ||
            (e->kind == exgine::NodeKind::Player && id != player)) {
            (void)ensure_character(runtime, id, false);
        }
    }

    exgine::Vec3 at{};
    if (player) {
        if (auto* e = runtime.state().entities.get(player)) {
            at = {e->transform.x, e->transform.y, e->transform.z};
        }
    }
    (void)ensure_ground(runtime, at);
    (void)ensure_buildings_vehicles(runtime);
    (void)ensure_hud_entities(runtime);

    ready_ = true;
    std::cerr << "EXWORLD: visuals bootstrap OK (character+ground+HUD meshes)\n";
    return true;
}

void VisualSystem::update_hud_markers(exgine::Runtime& runtime, const exgine::Camera& cam,
                                      const HudLayout& layout, int screen_w, int screen_h) {
    if (!ready_) return;
    (void)screen_w;
    (void)screen_h;
    (void)layout;

    // Place HUD quads in front of the camera so the player can SEE control zones.
    // Positions are approximate NDC mapped into view space (~2m ahead).
    const float dist = 2.2f;
    const float cy = std::cos(cam.rotation.y);
    const float sy = std::sin(cam.rotation.y);
    const float cp = std::cos(cam.rotation.x);
    const float sp = std::sin(cam.rotation.x);

    // Forward vector from yaw/pitch
    const float fx = sy * cp;
    const float fy = -sp;
    const float fz = cy * cp;
    // Right vector
    const float rx = cy;
    const float rz = -sy;
    // Up roughly
    const float ux = -sy * sp;
    const float uy = cp;
    const float uz = -cy * sp;

    auto place = [&](exgine::EntityId id, float ndc_x, float ndc_y, float scale) {
        if (!id) return;
        auto* e = runtime.state().entities.get(id);
        if (!e) return;
        // ndc -1..1 → offset in view plane
        const float ox = ndc_x * 1.1f;
        const float oy = ndc_y * 0.65f;
        e->transform.x = cam.position.x + fx * dist + rx * ox + ux * oy;
        e->transform.y = cam.position.y + fy * dist + uy * oy;
        e->transform.z = cam.position.z + fz * dist + rz * ox + uz * oy;
        e->transform.sx = scale;
        e->transform.sy = scale;
        e->transform.sz = scale;
        e->active = true;
    };

    // Left stick, right stick, interact (upper right), exit (lower right)
    place(hud_move_, -0.55f, -0.45f, 1.f);
    place(hud_look_, 0.45f, -0.15f, 1.f);
    place(hud_interact_, 0.75f, 0.55f, 1.f);
    place(hud_exit_, 0.75f, -0.55f, 1.f);
}

} // namespace exworld
