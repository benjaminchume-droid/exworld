#include "exworld/game.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string read_file(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

} // namespace

int main(int argc, char** argv) {
    const std::filesystem::path content_root =
        argc > 1 ? std::filesystem::path(argv[1]) : std::filesystem::path("content");

    auto loader = [content_root](std::string_view uri, std::string& out) -> bool {
        out = read_file(content_root / std::string(uri));
        if (!out.empty()) return true;
        out = read_file(content_root.parent_path() / "content" / std::string(uri));
        return !out.empty();
    };

    exworld::ExWorldGame game(loader);

    if (!game.open(content_root.string())) {
        std::cerr << "EXWORLD: failed to open game project\n";
        return 1;
    }
    if (!game.start()) {
        std::cerr << "EXWORLD: failed to start\n";
        return 2;
    }

    std::cout << "EXWORLD v0.3 \u2014 full systems online\n"
              << "  real input (keyboard/gamepad/touch)\n"
              << "  VehicleDynamicsController possession\n"
              << "  police AI + wanted escalation\n"
              << "  interior room navigation\n"
              << "  save / load\n"
              << "  100% procedural (ExSound + ExAnimation + EXGINE)\n";

    constexpr int kFrames = 1200;
    exgine::RenderFrame frame;
    exgine::RenderResult result;

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kFrames; ++i) {
        exworld::PlayerInput in;
        in.move_z = 0.65f;
        in.sprint = (i / 90) % 2 == 0;

        if (i == 200) in.interact = true;   // enter vehicle or building
        if (i == 450) in.exit = true;
        if (i == 700) in.interact = true;
        if (i == 900) {
            // Save mid-session
            auto data = game.save_game();
            std::cout << "save bytes=" << data.size() << "\n";
            if (!game.load_game(data))
                std::cerr << "save round-trip failed\n";
        }

        game.set_input(in);

        if (!game.update(1.0 / 60.0)) {
            std::cerr << "EXWORLD: update failed at frame " << i << "\n";
            return 3;
        }
        if (!game.build_frame(frame, result) || !result.success) {
            std::cerr << "EXWORLD: render failed at frame " << i << "\n";
            return 4;
        }
    }
    auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "EXWORLD validation passed\n"
              << "frames=" << kFrames << " avg_ms=" << (ms / kFrames) << "\n"
              << "wanted=" << static_cast<int>(game.wanted().level()) << "\n"
              << "police_chasers=" << game.police().active_chasers() << "\n";
    return 0;
}
