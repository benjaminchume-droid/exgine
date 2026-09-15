#include "exgine/asset_runtime.hpp"

#include <cassert>
#include <string>
#include <vector>

using namespace exgine;

namespace {
const char* cube_obj() {
    return "v -1 -1 0\nv 1 -1 0\nv 1 1 0\nv -1 1 0\nf 1 2 3 4\n";
}

void test_import_and_registry() {
    const auto result = import_obj("models/test.obj", cube_obj());
    assert(result.success);
    assert(result.meshes.size() == 1);
    assert(result.meshes[0].valid());
    assert(result.meshes[0].mesh.indices.size() == 6);
    const std::vector<std::uint8_t> bytes(cube_obj(), cube_obj() + std::char_traits<char>::length(cube_obj()));
    assert(result.meshes[0].asset_id == make_asset_id("models/test.obj", bytes));
}

void test_rejections() {
    assert(!import_obj("x.obj", "v 0 0\n").success);
    assert(detect_import_format("foo.obj", "") == ImportFormat::Obj);
    assert(detect_import_format("foo.gltf", "") == ImportFormat::Unknown);
}

void test_runtime_attachment() {
    IR ir;
    ir.root.children.push_back(Node{NodeKind::Prop, "ImportedProp", {}, {}});
    Runtime runtime;
    assert(runtime.load(ir));
    EntityId entity = invalid_entity;
    for (const auto id : runtime.state().entities.ids()) {
        const auto* e = runtime.state().entities.get(id);
        if (e && e->kind == NodeKind::Prop) entity = id;
    }
    assert(entity != invalid_entity);

    AssetRuntime assets;
    const auto result = assets.import_and_attach(runtime, entity, "models/test.obj", cube_obj(), "stone");
    assert(result.success);
    assert(result.asset_id != invalid_asset);
    assert(assets.size() == 1);
    const auto* asset = assets.find(result.asset_id);
    assert(asset && asset->valid());
    const auto* live = runtime.state().entities.get(entity);
    assert(live && live->geometry && live->geometry->valid());
    assert(live->geometry->parts[0].material_slot == "stone");

    const auto again = assets.import_and_attach(runtime, entity, "models/test.obj", cube_obj());
    assert(again.success);
    assert(again.asset_id == result.asset_id);
}
} // namespace

int main() {
    test_import_and_registry();
    test_rejections();
    test_runtime_attachment();
    return 0;
}
