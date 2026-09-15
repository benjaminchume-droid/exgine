#include "exgine/asset.hpp"

#include <cassert>
#include <vector>

using namespace exgine;

namespace {
AssetRecord make_asset(std::string uri, std::initializer_list<std::uint8_t> bytes, AssetType type = AssetType::Binary) {
    AssetRecord asset;
    asset.uri = std::move(uri);
    asset.data.assign(bytes.begin(), bytes.end());
    asset.type = type;
    asset.id = make_asset_id(asset.uri, asset.data);
    return asset;
}

void test_stable_id() {
    const auto a = make_asset("meshes/car.mesh", {1, 2, 3}, AssetType::Mesh);
    const auto b = make_asset("meshes/car.mesh", {1, 2, 3}, AssetType::Mesh);
    const auto c = make_asset("meshes/car.mesh", {1, 2, 4}, AssetType::Mesh);
    assert(a.id == b.id);
    assert(a.id != c.id);
    assert(a.id != invalid_asset);
}

void test_database_and_order() {
    AssetDatabase db;
    auto texture = make_asset("textures/car.bin", {7, 8}, AssetType::Texture);
    auto material = make_asset("materials/car.bin", {9}, AssetType::Material);
    material.dependencies.push_back(texture.id);
    auto mesh = make_asset("meshes/car.bin", {3, 4, 5}, AssetType::Mesh);
    mesh.dependencies.push_back(material.id);

    assert(db.add(mesh));
    assert(db.add(material));
    assert(db.add(texture));
    assert(db.size() == 3);
    assert(db.dependencies_resolved(mesh.id));
    const auto order = db.load_order();
    assert(order.size() == 3);
    assert(order[0] == texture.id);
    assert(order[1] == material.id);
    assert(order[2] == mesh.id);
    assert(!db.add(mesh));
}

void test_package_round_trip() {
    AssetDatabase source;
    auto texture = make_asset("textures/a", {1, 2}, AssetType::Texture);
    auto material = make_asset("materials/a", {3, 4}, AssetType::Material);
    material.dependencies = {texture.id};
    assert(source.add(material));
    assert(source.add(texture));

    const auto packed = source.pack();
    assert(packed.success);
    assert(!packed.bytes.empty());

    AssetDatabase restored;
    const auto result = restored.unpack(packed.bytes);
    assert(result.success);
    assert(restored.size() == 2);
    assert(restored.find_uri("materials/a") != nullptr);
    assert(restored.find_uri("textures/a") != nullptr);
    assert(restored.load_order().size() == 2);
}

void test_rejects_missing_dependency_and_malformed_package() {
    AssetDatabase db;
    auto material = make_asset("materials/missing", {1}, AssetType::Material);
    material.dependencies = {123456};
    const auto packed = pack_assets({material});
    assert(packed.success);
    AssetDatabase invalid;
    const auto result = invalid.unpack(packed.bytes);
    assert(!result.success);
    assert(!result.error.empty());

    std::vector<std::uint8_t> truncated{0x58, 0x47, 0x50};
    std::vector<AssetRecord> decoded;
    const auto malformed = unpack_assets(truncated, decoded);
    assert(!malformed.success);
}

void test_cycle_rejected() {
    AssetDatabase db;
    auto a = make_asset("a", {1});
    auto b = make_asset("b", {2});
    a.dependencies = {b.id};
    b.dependencies = {a.id};
    assert(db.add(a));
    assert(db.add(b));
    assert(db.load_order().empty());
    assert(!db.dependencies_resolved(a.id) || !db.dependencies_resolved(b.id));
}
} // namespace

int main() {
    test_stable_id();
    test_database_and_order();
    test_package_round_trip();
    test_rejects_missing_dependency_and_malformed_package();
    test_cycle_rejected();
    return 0;
}
