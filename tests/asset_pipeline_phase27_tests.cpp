#include "exgine/asset_pipeline.hpp"

#include <cassert>
#include <string>
#include <vector>

using namespace exgine;

int main() {
    ImageDecoderRegistry decoders;
    const std::vector<std::uint8_t> ppm{
        'P','3',' ','2',' ','1',' ','2','5','5','\n',
        '2','5','5',' ','0',' ','0',' ','0',' ','2','5','5',' ','0','\n'
    };

    const std::string gltf = R"JSON({
      "asset":{"version":"2.0"},
      "materials":[{"name":"Paint","pbrMetallicRoughness":{"baseColorFactor":[0.8,0.2,0.1,1.0]}}],
      "images":[{"uri":"paint.ppm"}]
    })JSON";

    AssetPipeline pipeline;
    const auto result = pipeline.process(
        "assets/car.gltf",
        gltf,
        [&](std::string_view uri, std::vector<std::uint8_t>& bytes, std::string& error) {
            if (uri == "assets/paint.ppm") { bytes = ppm; return true; }
            error = "missing asset";
            return false;
        },
        decoders);

    assert(result.success);
    assert(result.imported.success);
    assert(result.imported.materials.size() == 1);
    assert(result.materials.size() == 1);
    assert(!result.materials[0].maps.empty());
    assert(result.materials[0].maps[0].valid());
    assert(result.materials[0].textures.base_color);
    assert(result.materials[0].textures.base_color->valid());
    assert(result.materials[0].textures.base_color->width == 2);
    assert(result.materials[0].textures.base_color->height == 1);
    return 0;
}
