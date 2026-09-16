#include "exworld/asset_package.hpp"

#include <cctype>
#include <sstream>

namespace exworld {
namespace {

std::string trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.remove_suffix(1);
    return std::string(s);
}

std::vector<std::string> split_csv(std::string_view v) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : v) {
        if (c == ',') {
            out.push_back(trim(cur));
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    out.push_back(trim(cur));
    return out;
}

} // namespace

bool AssetPackageRegistry::load_from_text(std::string_view index_text,
                                          const FileLoader& file_loader) {
    packages_.clear();
    by_name_.clear();
    loaded_ = false;
    if (index_text.empty() || !file_loader) return false;

    std::istringstream in{std::string(index_text)};
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const auto key = trim(line.substr(0, eq));
        const auto value = trim(line.substr(eq + 1));

        if (key == "package") {
            auto parts = split_csv(value);
            if (parts.size() < 2) continue;
            BakedPackage p;
            p.name = parts[0];
            p.path = parts[1];
            if (parts.size() >= 3) {
                try { p.version = std::stof(parts[2]); } catch (...) {}
            }
            if (parts.size() >= 4) p.kind = parts[3];
            std::string body;
            if (file_loader(p.path, body) || file_loader(std::string("baked/") + p.name + ".exg", body))
                p.body = std::move(body);
            by_name_[p.name] = packages_.size();
            packages_.push_back(std::move(p));
        } else if (key == "player_height") {
            try { scale_.player_height = std::stof(value); } catch (...) {}
        } else if (key == "car_length") {
            try { scale_.car_length = std::stof(value); } catch (...) {}
        } else if (key == "car_width") {
            try { scale_.car_width = std::stof(value); } catch (...) {}
        } else if (key == "floor_height") {
            try { scale_.floor_height = std::stof(value); } catch (...) {}
        } else if (key == "block_size") {
            try { scale_.block_size = std::stof(value); } catch (...) {}
        } else if (key == "stream_radius") {
            try { scale_.stream_radius = std::stof(value); } catch (...) {}
        }
    }

    loaded_ = !packages_.empty();
    return loaded_;
}

bool AssetPackageRegistry::has(std::string_view name) const {
    return by_name_.find(std::string(name)) != by_name_.end();
}

const BakedPackage* AssetPackageRegistry::get(std::string_view name) const {
    auto it = by_name_.find(std::string(name));
    if (it == by_name_.end()) return nullptr;
    return &packages_[it->second];
}

} // namespace exworld
