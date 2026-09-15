#pragma once

#include "exgine/asset.hpp"
#include "exgine/geometry.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace exgine {

enum class ImportFormat : std::uint8_t { Unknown = 0, Obj = 1 };

struct ImportedMesh {
    std::string uri;
    Mesh mesh;
    AssetId asset_id = invalid_asset;
    std::string material_name;
    [[nodiscard]] bool valid() const noexcept { return !uri.empty() && asset_id != invalid_asset && mesh.valid(); }
};

struct ImportResult {
    bool success = false;
    ImportFormat format = ImportFormat::Unknown;
    std::vector<ImportedMesh> meshes;
    std::string error;
};

[[nodiscard]] ImportFormat detect_import_format(std::string_view uri, std::string_view text) noexcept;
[[nodiscard]] ImportResult import_obj(std::string_view uri, std::string_view text);

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

} // namespace exgine
