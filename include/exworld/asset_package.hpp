#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace exworld {

struct BakedPackage {
    std::string name;
    std::string path;
    std::string kind;
    float version = 1.f;
    std::string body; // raw .exg text loaded from assets
};

struct WorldScale {
    float player_height = 1.80f;
    float car_length = 4.50f;
    float car_width = 1.85f;
    float floor_height = 3.00f;
    float block_size = 96.f;
    float stream_radius = 400.f;
};

// Reads content/baked/index.exg + listed packages (CI-prebuilt, shipped in APK).
class AssetPackageRegistry {
public:
    using Loader = bool (*)(std::string_view path, std::string& out); // not used; lambda via template-free std::function alternative

    bool load_from_text(std::string_view index_text,
                        const std::function<bool(std::string_view, std::string&)>& file_loader);

    [[nodiscard]] bool has(std::string_view name) const;
    [[nodiscard]] const BakedPackage* get(std::string_view name) const;
    [[nodiscard]] const std::vector<BakedPackage>& all() const noexcept { return packages_; }
    [[nodiscard]] const WorldScale& scale() const noexcept { return scale_; }
    [[nodiscard]] bool loaded() const noexcept { return loaded_; }

private:
    std::vector<BakedPackage> packages_;
    std::unordered_map<std::string, std::size_t> by_name_;
    WorldScale scale_{};
    bool loaded_ = false;
};

} // namespace exworld

#include <functional>
