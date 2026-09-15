#include "exgine/asset_runtime.hpp"

#include <cassert>
#include <cmath>
#include <string>

using namespace exgine;

namespace {
const char* cube_obj() {
    return "v -1 -1 0\nv 1 -1 0\nv 1 1 0\nv -1 1 0\nf 1 2 3 4\n";
}

void test_obj_import() {
    const auto result = import_obj("models/test.obj", cube_obj());
    assert(result.success);
    assert(result.format == ImportFormat::Obj);
    assert(result.meshes.size() == 1);
    assert(result.meshes[0].valid());
    assert(result.meshes[0].mesh.indices.size() == 6);
    assert(result.meshes[0].asset_id == make_asset_id("models/test.obj", std::vector<std::uint8_t>(cube_obj(), cube_obj() + std::char_traits<char>::length(cube_obj()))));
    assert(std::fabs(result.meshes[0].mesh.vertices[0].normal.z) > 0.9f);
}

void test_obj_bad_input() {
    assert(!import_obj("x.obj", "v 0 0\n").success);
    assert(detect_import_format("foo.obj", "") == ImportFormat::Obj);
    assert(detect_import_format("foo.gltf", "") == ImportFormat::Unknown);
}

void test_runtime_attachment() {
    IR ir;
    auto& prop = ir.root.add_child(NodeKind::Prop, "ImportedProp");
    Runtime runtime;
    assert(runtime.load(ir));
    const auto id = runtime.state().entities.ids().back();
    AssetRuntime assets;
    const auto result = assets.import_and_attach(runtime, id, "models/test.obj", cube_obj(), "stone");
    assert(result.success);
    assert(result.asset_id != invalid_asset);
    assert(assets.size() == 1);
    const auto* asset = assets.find(result.asset_id);
    assert(asset && asset->valid());
    const auto* entity = runtime.state().entities.get(id);
    assert(entity && entity->geometry && entity->geometry->valid());
    assert(entity->geometry->parts[0].material_slot == "stone");

    const auto again = assets.import_and_attach(runtime, id, "models/test.obj", cube_obj());
    assert(again.success);
    assert(again.asset_id == result.asset_id);
}
} // namespace

int main() {
    test_obj_import();
    test_obj_bad_input();
    test_runtime_attachment();
    return 0;
}
