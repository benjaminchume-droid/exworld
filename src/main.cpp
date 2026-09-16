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
        const auto path = content_root / std::string(uri);
        out = read_file(path);
        if (!out.empty()) return true;
        // fallback relative to content parent
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

    std::cout << "EXWORLD — open world running\n";
    std::cout << "Controls (placeholder keyboard mapping later): free roam + vehicle enter\n";

    constexpr int kFrames = 600; // ~10 seconds at 60 Hz for headless validation
    exgine::RenderFrame frame;
    exgine::RenderResult result;

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kFrames; ++i) {
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
              << "frames=" << kFrames << " total_ms=" << ms
              << " avg_frame_ms=" << (ms / kFrames) << "\n";
    return 0;
}
