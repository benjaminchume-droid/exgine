#include "exgine/asset.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
exgine::AssetType parse_type(const std::string& s) {
    if (s == "mesh") return exgine::AssetType::Mesh;
    if (s == "texture") return exgine::AssetType::Texture;
    if (s == "material") return exgine::AssetType::Material;
    if (s == "skeleton") return exgine::AssetType::Skeleton;
    if (s == "animation") return exgine::AssetType::Animation;
    if (s == "scene") return exgine::AssetType::Scene;
    if (s == "audio") return exgine::AssetType::Audio;
    if (s == "script") return exgine::AssetType::Script;
    return exgine::AssetType::Binary;
}
bool read_file(const std::string& path, std::vector<std::uint8_t>& bytes) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: exgpack output.exg type=uri:path [type=uri:path ...]\n";
        return 2;
    }
    std::vector<exgine::AssetRecord> assets;
    for (int i = 2; i < argc; ++i) {
        const std::string spec = argv[i];
        const auto eq = spec.find('=');
        const auto colon = spec.find(':', eq == std::string::npos ? 0 : eq + 1);
        if (eq == std::string::npos || colon == std::string::npos || eq == 0 || colon <= eq + 1 || colon + 1 >= spec.size()) {
            std::cerr << "invalid asset specification: " << spec << '\n';
            return 2;
        }
        const std::string type = spec.substr(0, eq);
        const std::string uri = spec.substr(eq + 1, colon - eq - 1);
        const std::string path = spec.substr(colon + 1);
        exgine::AssetRecord asset;
        asset.type = parse_type(type);
        asset.uri = uri;
        if (!read_file(path, asset.data)) {
            std::cerr << "cannot read asset: " << path << '\n';
            return 3;
        }
        asset.id = exgine::make_asset_id(asset.uri, asset.data);
        assets.push_back(std::move(asset));
    }
    const auto result = exgine::pack_assets(assets);
    if (!result.success) { std::cerr << "pack failed: " << result.error << '\n'; return 4; }
    std::ofstream out(argv[1], std::ios::binary);
    if (!out) { std::cerr << "cannot open output: " << argv[1] << '\n'; return 5; }
    out.write(reinterpret_cast<const char*>(result.bytes.data()), static_cast<std::streamsize>(result.bytes.size()));
    if (!out) { std::cerr << "cannot write output\n"; return 6; }
    std::cout << "packed " << assets.size() << " assets into " << argv[1] << " (" << result.bytes.size() << " bytes)\n";
    return 0;
}
