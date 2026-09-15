#pragma once

#include "exgine/asset.hpp"
#include "exgine/geometry.hpp"
#include "exgine/material.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace exgine {

enum class ImportFormat : std::uint8_t { Unknown = 0, Obj = 1, Gltf = 2, Glb = 3 };

struct ImportedMesh {
    std::string uri;
    Mesh mesh;
    AssetId asset_id = invalid_asset;
    std::string material_name;
    std::uint32_t source_mesh = 0;
    std::uint32_t source_primitive = 0;
    [[nodiscard]] bool valid() const noexcept { return !uri.empty() && asset_id != invalid_asset && mesh.valid(); }
};

struct ImportedMaterial {
    std::string name;
    Material material;
    std::uint32_t source_material = 0;
    [[nodiscard]] bool valid() const noexcept { return material.valid(); }
};

struct ImportedNode {
    std::string name;
    std::uint32_t source_node = 0;
    std::int32_t parent = -1;
    std::int32_t mesh_index = -1;
    Vec3 translation{};
    Vec3 rotation{};
    Vec3 scale{1,1,1};
    [[nodiscard]] bool valid() const noexcept;
};

struct ImportResult {
    bool success = false;
    ImportFormat format = ImportFormat::Unknown;
    std::vector<ImportedMesh> meshes;
    std::vector<ImportedMaterial> materials;
    std::vector<ImportedNode> nodes;
    std::string error;
};

[[nodiscard]] ImportFormat detect_import_format(std::string_view uri, std::string_view payload) noexcept;
[[nodiscard]] ImportResult import_obj(std::string_view uri, std::string_view text);
[[nodiscard]] ImportResult import_gltf(std::string_view uri, std::string_view text_or_glb);
[[nodiscard]] ImportResult import_glb(std::string_view uri, std::string_view binary);

class AssetImporter {
public:
    virtual ~AssetImporter() = default;
    [[nodiscard]] virtual bool accepts(std::string_view uri, std::string_view payload) const noexcept = 0;
    [[nodiscard]] virtual ImportResult import(std::string_view uri, std::string_view payload) const = 0;
};

class ObjAssetImporter final : public AssetImporter {
public:
    [[nodiscard]] bool accepts(std::string_view uri, std::string_view payload) const noexcept override;
    [[nodiscard]] ImportResult import(std::string_view uri, std::string_view payload) const override;
};

class GltfAssetImporter final : public AssetImporter {
public:
    [[nodiscard]] bool accepts(std::string_view uri, std::string_view payload) const noexcept override;
    [[nodiscard]] ImportResult import(std::string_view uri, std::string_view payload) const override;
};

} // namespace exgine
