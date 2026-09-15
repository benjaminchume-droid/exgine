#include "exgine/editor.hpp"
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace exgine;

int main() {
    EditorSession editor;
    assert(editor.project().valid());
    const auto root = editor.project().root_id;
    const auto house = editor.create_node(NodeKind::Building, "House", root);
    const auto car = editor.create_node(NodeKind::Vehicle, "Car", house);
    assert(house != 0 && car != 0);
    assert(editor.selected() == car);
    assert(editor.set_transform(car, SceneTransform{{10.0F, 1.0F, -4.0F},{0.0F, 0.5F, 0.0F},{1.0F,1.0F,1.0F}}));
    assert(editor.set_property(car, {"mass", 1200.0}));
    std::uint64_t asset = 0;
    assert(editor.add_asset(EditorAssetType::Mesh, "car_mesh", "assets/car.glb", &asset));
    assert(asset != 0);
    assert(editor.preview());
    assert(editor.preview_runtime().state().world_entity != invalid_entity);

    const auto source = editor.emit_source();
    assert(source.find("building") != std::string::npos);
    assert(source.find("vehicle") != std::string::npos);
    assert(source.find("mass") != std::string::npos);

    assert(editor.undo());
    assert(editor.project().assets.empty());
    assert(editor.redo());
    assert(editor.project().assets.size() == 1);
    assert(editor.undo());
    assert(editor.undo());
    assert(editor.project().nodes.size() == 2);
    assert(editor.redo());
    assert(editor.redo());

    const auto path = (std::filesystem::temp_directory_path() / "exgine_phase14_test.exproj").string();
    assert(editor.save(path));
    EditorSession loaded;
    assert(loaded.load(path));
    assert(loaded.project().valid());
    assert(loaded.project().nodes.size() == editor.project().nodes.size());
    assert(loaded.project().assets.size() == editor.project().assets.size());
    assert(loaded.emit_source() == editor.emit_source());
    std::filesystem::remove(path);

    assert(!editor.set_parent(house, car));
    assert(!editor.set_transform(car, SceneTransform{{NAN,0,0},{0,0,0},{1,1,1}}));
    assert(editor.destroy_node(house));
    assert(editor.project().nodes.size() == 1);
    assert(editor.undo());
    assert(editor.project().nodes.size() == 3);

    std::cout << "EXGINE Phase 14 editor tests passed\n";
    return 0;
}
