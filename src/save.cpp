#include "exworld/save.hpp"

#include <cstring>
#include <fstream>

namespace exworld {
namespace {

void put_u32(std::vector<std::uint8_t>& b, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) b.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xff));
}
void put_u64(std::vector<std::uint8_t>& b, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) b.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xff));
}
void put_f32(std::vector<std::uint8_t>& b, float v) {
    std::uint32_t x; std::memcpy(&x, &v, 4); put_u32(b, x);
}
void put_f64(std::vector<std::uint8_t>& b, double v) {
    std::uint64_t x; std::memcpy(&x, &v, 8); put_u64(b, x);
}

bool get_u32(const std::vector<std::uint8_t>& b, std::size_t& p, std::uint32_t& v) {
    if (p + 4 > b.size()) return false;
    v = 0;
    for (int i = 0; i < 4; ++i) v |= static_cast<std::uint32_t>(b[p++]) << (8 * i);
    return true;
}
bool get_u64(const std::vector<std::uint8_t>& b, std::size_t& p, std::uint64_t& v) {
    if (p + 8 > b.size()) return false;
    v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<std::uint64_t>(b[p++]) << (8 * i);
    return true;
}
bool get_f32(const std::vector<std::uint8_t>& b, std::size_t& p, float& v) {
    std::uint32_t x; if (!get_u32(b, p, x)) return false; std::memcpy(&v, &x, 4); return true;
}
bool get_f64(const std::vector<std::uint8_t>& b, std::size_t& p, double& v) {
    std::uint64_t x; if (!get_u64(b, p, x)) return false; std::memcpy(&v, &x, 8); return true;
}

} // namespace

std::vector<std::uint8_t> SaveSystem::save(const Player& player, const WantedSystem& wanted,
                                           double world_time,
                                           const exgine::Runtime& runtime) const {
    std::vector<std::uint8_t> b;
    b.push_back('E'); b.push_back('X'); b.push_back('W'); b.push_back('S'); // magic
    put_u32(b, 1); // version

    const auto pos = player.position(runtime);
    put_f32(b, pos.x);
    put_f32(b, pos.y);
    put_f32(b, pos.z);
    put_f32(b, 0.f); // yaw placeholder
    b.push_back(static_cast<std::uint8_t>(player.mode()));
    put_u64(b, static_cast<std::uint64_t>(player.current_vehicle()));
    put_u64(b, static_cast<std::uint64_t>(player.current_building()));
    put_f32(b, wanted.heat());
    b.push_back(static_cast<std::uint8_t>(wanted.level()));
    put_f64(b, world_time);
    return b;
}

bool SaveSystem::load(const std::vector<std::uint8_t>& bytes, Player& player,
                      WantedSystem& wanted, double& world_time,
                      exgine::Runtime& runtime) {
    if (bytes.size() < 12) return false;
    if (bytes[0] != 'E' || bytes[1] != 'X' || bytes[2] != 'W' || bytes[3] != 'S') return false;

    std::size_t p = 4;
    std::uint32_t ver = 0;
    if (!get_u32(bytes, p, ver) || ver != 1) return false;

    float x, y, z, yaw;
    if (!get_f32(bytes, p, x) || !get_f32(bytes, p, y) || !get_f32(bytes, p, z) || !get_f32(bytes, p, yaw))
        return false;

    if (p >= bytes.size()) return false;
    const auto mode = static_cast<PlayerMode>(bytes[p++]);

    std::uint64_t veh = 0, bld = 0;
    if (!get_u64(bytes, p, veh) || !get_u64(bytes, p, bld)) return false;

    float heat = 0;
    if (!get_f32(bytes, p, heat)) return false;
    if (p >= bytes.size()) return false;
    const auto wlevel = static_cast<WantedLevel>(bytes[p++]);

    if (!get_f64(bytes, p, world_time)) return false;

    // Apply transform
    auto* e = runtime.state().entities.get(player.entity());
    if (e) {
        e->transform.x = x;
        e->transform.y = y;
        e->transform.z = z;
    }

    player.set_mode(mode);
    wanted.set_level(wlevel);
    // heat is derived from level in set_level; fine for v1

    (void)veh; (void)bld; (void)heat; (void)yaw;
    return true;
}

bool SaveSystem::save_to_file(const std::string& path, const Player& player,
                              const WantedSystem& wanted, double world_time,
                              const exgine::Runtime& runtime) const {
    auto data = save(player, wanted, world_time, runtime);
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(f);
}

bool SaveSystem::load_from_file(const std::string& path, Player& player,
                                WantedSystem& wanted, double& world_time,
                                exgine::Runtime& runtime) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(f)), {});
    return load(data, player, wanted, world_time, runtime);
}

} // namespace exworld
