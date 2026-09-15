#pragma once

#include "exgine/asset.hpp"
#include "exgine/import.hpp"
#include "exgine/runtime.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace exgine {

struct AssetInstance {
    AssetId asset_id = invalid_asset;
    EntityId entity_id = invalid_entity;
    std::string uri;
    std::shared_ptr<MeshAssembly> geometry;
    [[nodiscard]] bool valid() const noexcept { return asset_id != invalid_asset && entity_id != invalid_entity && geometry && geometry->valid(); }
};

struct AssetRuntimeLoadResult {
    bool success = false;
    AssetId asset_id = invalid_asset;
    std::string error;
};

class AssetRuntime {
public:
    bool add_imported_mesh(const ImportedMesh& mesh);
    [[nodiscard]] const AssetInstance* find(AssetId id) const noexcept;
    [[nodiscard]] const AssetInstance* find_uri(std::string_view uri) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return meshes_.size(); }
    void clear() noexcept { meshes_.clear(); }
    [[nodiscard]] AssetRuntimeLoadResult attach(Runtime& runtime, EntityId entity, AssetId id, std::string material_slot = {}) const;
    [[nodiscard]] AssetRuntimeLoadResult import_and_attach(Runtime& runtime, EntityId entity,
                                                            std::string_view uri, std::string_view obj_source,
                                                            std::string material_slot = {});

private:
    std::unordered_map<AssetId, AssetInstance> meshes_;
};

} // namespace exgine
