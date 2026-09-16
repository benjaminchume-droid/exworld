#pragma once

#include "exgine/runtime.hpp"
#include "exworld/hud.hpp"

namespace exworld {

// Attaches real MeshAssembly geometry (characters, ground, HUD markers)
// and ensures materials resolve so Renderer::build_frame draws them.
// Uses EXGINE generate_character / generate_building / generate_vehicle
// for multi-part real 3D assets at meter scale.
class VisualSystem {
public:
    bool bootstrap(exgine::Runtime& runtime, exgine::EntityId player);
    void update_hud_markers(exgine::Runtime& runtime, const exgine::Camera& cam,
                            const HudLayout& layout, int screen_w, int screen_h);

    [[nodiscard]] bool ready() const noexcept { return ready_; }

private:
    bool ensure_materials(exgine::Runtime& runtime);
    bool ensure_character(exgine::Runtime& runtime, exgine::EntityId id, bool player);
    bool ensure_ground(exgine::Runtime& runtime, const exgine::Vec3& at);
    bool ensure_hud_entities(exgine::Runtime& runtime);
    bool ensure_buildings_vehicles(exgine::Runtime& runtime);

    bool ready_ = false;
    exgine::EntityId ground_ = exgine::invalid_entity;
    exgine::EntityId ground_grass_ = exgine::invalid_entity;
    exgine::EntityId ground_water_ = exgine::invalid_entity;
    exgine::EntityId hud_move_ = exgine::invalid_entity;
    exgine::EntityId hud_look_ = exgine::invalid_entity;
    exgine::EntityId hud_interact_ = exgine::invalid_entity;
    exgine::EntityId hud_exit_ = exgine::invalid_entity;
};

} // namespace exworld
