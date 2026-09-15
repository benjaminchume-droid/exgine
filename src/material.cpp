#include "exgine/material.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>
namespace exgine {
namespace { std::uint64_t hash_seed(std::uint64_t x) noexcept{x+=0x9e3779b97f4a7c15ULL;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;return x^(x>>31);} float unit(std::uint64_t seed) noexcept{return static_cast<float>(hash_seed(seed)>>40)/static_cast<float>(1u<<24);} }
bool Material::valid() const noexcept{if(name.empty()||!std::isfinite(roughness)||!std::isfinite(metallic)||!std::isfinite(specular)||!std::isfinite(opacity))return false;return roughness>=0&&roughness<=1&&metallic>=0&&metallic<=1&&specular>=0&&specular<=1&&opacity>=0&&opacity<=1;}
bool MaterialLibrary::define(Material material){if(!material.valid())return false;materials_.insert_or_assign(material.name,std::move(material));return true;}
bool MaterialLibrary::erase(std::string_view name) noexcept{return materials_.erase(std::string{name})!=0;}
const Material* MaterialLibrary::find(std::string_view name) const noexcept{const auto it=materials_.find(std::string{name});return it==materials_.end()?nullptr:&it->second;}
Material make_real_world_material(std::string_view name,std::uint64_t seed){Material m;m.name=std::string{name};const std::uint64_t h=hash_seed(seed^std::hash<std::string_view>{}(name));const float j=(unit(h)-0.5f)*0.08f;
if(name=="wood"){m.base_color={0.42f+j,0.20f+j*0.5f,0.07f};m.roughness=.62f;m.layers={{"grain",7,.8f,seed}};}
else if(name=="steel"){m.base_color={.48f,.50f,.53f};m.roughness=.34f;m.metallic=.92f;m.layers={{"scratches",28,.18f,seed},{"noise",4,.12f,seed+1}};}
else if(name=="aluminium"){m.base_color={.67f,.69f,.71f};m.roughness=.29f;m.metallic=.96f;}
else if(name=="copper"){m.base_color={.68f,.27f,.10f};m.roughness=.31f;m.metallic=.94f;m.layers={{"oxidation",2,.08f,seed}};}
else if(name=="glass"){m.base_color={.93f,.98f,1};m.roughness=.06f;m.specular=.9f;m.opacity=.18f;}
else if(name=="rubber"){m.base_color={.025f,.025f,.023f};m.roughness=.82f;m.layers={{"grain",18,.25f,seed}};}
else if(name=="water"){m.base_color={.03f,.23f,.34f};m.roughness=.04f;m.specular=.95f;m.opacity=.72f;m.layers={{"waves",1.5f,.35f,seed},{"noise",5,.12f,seed+1}};}
else if(name=="grass"){m.base_color={.12f,.34f,.06f};m.roughness=.88f;m.layers={{"fbm",3,.32f,seed},{"grain",24,.18f,seed+1}};}
else if(name=="sand"){m.base_color={.70f,.55f,.32f};m.roughness=.91f;m.layers={{"noise",7,.24f,seed},{"grain",38,.20f,seed+1}};}
else if(name=="concrete"){m.base_color={.48f,.47f,.44f};m.roughness=.83f;m.layers={{"noise",5,.24f,seed},{"cracks",9,.12f,seed+1}};}
else if(name=="snow"){m.base_color={.92f,.95f,1};m.roughness=.93f;m.layers={{"fbm",2,.16f,seed}};}
else {m.base_color={.5f+j,.5f+j,.5f+j};m.roughness=std::clamp(.5f+j,.05f,.95f);} return m;}
} // namespace exgine
