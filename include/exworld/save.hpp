#pragma once

#include "exgine/runtime.hpp"
#include "exworld/player.hpp"
#include "exworld/wanted.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace exworld {

struct SaveGame {
    std::uint32_t version = 1;
    float player_x = 0, player_y = 1.1f, player_z = 0;
    float player_yaw = 0;
    std::uint8_t mode = 0;          // PlayerMode
    std::uint64_t vehicle_entity = 0;
    std::uint64_t building_entity = 0;
    float wanted_heat = 0;
    std::uint8_t wanted_level = 0;
    double world_time = 0;

    [[nodiscard]] bool valid() const noexcept { return version == 1; }
};

class SaveSystem {
public:
    // Serialize current game state
    [[nodiscard]] std::vector<std::uint8_t> save(const Player& player,
                                                 const WantedSystem& wanted,
                                                 double world_time,
                                                 const exgine::Runtime& runtime) const;

    // Restore into live systems
    [[nodiscard]] bool load(const std::vector<std::uint8_t>& bytes,
                            Player& player, WantedSystem& wanted,
                            double& world_time, exgine::Runtime& runtime);

    // Convenience file helpers (content/saves/...)
    [[nodiscard]] bool save_to_file(const std::string& path, const Player& player,
                                    const WantedSystem& wanted, double world_time,
                                    const exgine::Runtime& runtime) const;
    [[nodiscard]] bool load_from_file(const std::string& path, Player& player,
                                      WantedSystem& wanted, double& world_time,
                                      exgine::Runtime& runtime);
};

} // namespace exworld
