#include "exgine/material_pipeline.hpp"
#include <cassert>
#include <cmath>
using namespace exgine;
int main() {
    Texture2D h; h.width=8; h.height=8; h.channels=1; h.format=TextureFormat::R32F; h.data.resize(64);
    for (std::uint32_t y=0;y<8;++y) for (std::uint32_t x=0;x<8;++x) h.set(x,y,0,float(x+y)/14.0f);
    auto n=generate_normal_from_height(h,2.0f); assert(n.valid()&&n.channels==3);
    auto m=generate_mip_chain(h); assert(m.valid()&&m.levels.size()==4&&m.levels[1].width==4&&m.levels.back().width==1);
    auto o=generate_noise_texture({8,8,{NoiseType::Value,2,1,.5f,2,1}},1);
    auto r=generate_noise_texture({8,8,{NoiseType::Perlin,3,1,.5f,2,2}},1);
    auto me=generate_noise_texture({8,8,{NoiseType::Value,4,1,.5f,2,3}},1);
    auto orm=pack_orm(o,r,me); assert(orm.valid());
    auto mat=make_real_world_material("steel",42); MaterialPipelineConfig cfg; cfg.width=16; cfg.height=16; cfg.mip_levels=5;
    auto set=generate_production_material_textures(mat,cfg); assert(set.valid()&&set.base_color->width==16);
    auto sample=sample_material(mat,set,.37f,.61f); assert(sample.roughness>=0&&sample.roughness<=1&&sample.metallic>=0&&sample.metallic<=1);
    assert(std::isfinite(sample.normal.x)&&std::isfinite(sample.normal.y)&&std::isfinite(sample.normal.z));
    return 0;
}
