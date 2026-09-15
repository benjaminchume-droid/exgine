#include "exgine/asset_runtime.hpp"

namespace exgine {

bool AssetRuntime::add_imported_mesh(const ImportedMesh& mesh) {
    if (!mesh.valid()) return false;
    if (meshes_.contains(mesh.asset_id) || find_uri(mesh.uri) != nullptr) return false;
    AssetInstance instance;
    instance.asset_id = mesh.asset_id;
    instance.uri = mesh.uri;
    instance.geometry = std::make_shared<MeshAssembly>();
    MeshPart part;
    part.name = mesh.uri;
    part.mesh = mesh.mesh;
    part.material_slot = mesh.material_name;
    instance.geometry->parts.push_back(std::move(part));
    if (!instance.geometry->valid()) return false;
    meshes_.emplace(instance.asset_id, std::move(instance));
    return true;
}

const AssetInstance* AssetRuntime::find(AssetId id) const noexcept {
    const auto it = meshes_.find(id);
    return it == meshes_.end() ? nullptr : &it->second;
}

const AssetInstance* AssetRuntime::find_uri(std::string_view uri) const noexcept {
    for (const auto& [id, mesh] : meshes_) if (mesh.uri == uri) return &mesh;
    return nullptr;
}

AssetRuntimeLoadResult AssetRuntime::attach(Runtime& runtime, EntityId entity, AssetId id, std::string material_slot) const {
    AssetRuntimeLoadResult result;
    result.asset_id = id;
    const auto* asset = find(id);
    if (!asset) { result.error = "asset not found"; return result; }
    if (runtime.state().entities.get(entity) == nullptr) { result.error = "entity not found"; return result; }
    MeshAssembly copy = *asset->geometry;
    for (auto& part : copy.parts) if (!material_slot.empty()) part.material_slot = material_slot;
    if (!runtime.attach_geometry(entity, std::move(copy))) { result.error = "runtime rejected geometry"; return result; }
    result.success = true;
    return result;
}

AssetRuntimeLoadResult AssetRuntime::import_and_attach(Runtime& runtime, EntityId entity,
                                                        std::string_view uri, std::string_view obj_source,
                                                        std::string material_slot) {
    AssetRuntimeLoadResult result;
    const auto imported = import_obj(uri, obj_source);
    if (!imported.success || imported.meshes.empty()) {
        result.error = imported.error.empty() ? "asset import failed" : imported.error;
        return result;
    }
    if (!add_imported_mesh(imported.meshes.front())) {
        const auto* existing = find_uri(uri);
        if (!existing) { result.error = "asset registration failed"; return result; }
        result.asset_id = existing->asset_id;
    } else {
        result.asset_id = imported.meshes.front().asset_id;
    }
    return attach(runtime, entity, result.asset_id, std::move(material_slot));
}

} // namespace exgine
