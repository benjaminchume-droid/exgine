#include "exgine/lighting.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace exgine { namespace {
float length(Vec3 v) noexcept{return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);}
bool finite3(Color3 c) noexcept{return std::isfinite(c.r)&&std::isfinite(c.g)&&std::isfinite(c.b);}
bool finite3(Vec3 v) noexcept{return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
}
bool Light::valid() const noexcept{if(!finite3(position)||!finite3(direction)||!finite3(color)||!std::isfinite(intensity)||!std::isfinite(range)||!std::isfinite(inner_cone)||!std::isfinite(outer_cone)||!std::isfinite(area_size.x)||!std::isfinite(area_size.y))return false;if(intensity<0||range<0||inner_cone<0||outer_cone<=inner_cone||outer_cone>1||area_size.x<=0||area_size.y<=0)return false;return length(direction)>std::numeric_limits<float>::epsilon();}
bool Camera::valid() const noexcept{if(!std::isfinite(vertical_fov_degrees)||!std::isfinite(aspect_ratio)||!std::isfinite(near_plane)||!std::isfinite(far_plane)||!std::isfinite(orthographic_size))return false;return vertical_fov_degrees>0&&vertical_fov_degrees<179&&aspect_ratio>0&&near_plane>0&&far_plane>near_plane&&(!orthographic||orthographic_size>0);}
std::uint64_t LightingWorld::create_light(Light light){if(next_light_id_==0||next_light_id_==std::numeric_limits<std::uint64_t>::max())return 0;light.id=next_light_id_++;if(!light.valid())return 0;lights_.push_back(light);return light.id;}
bool LightingWorld::destroy_light(std::uint64_t id) noexcept{const auto it=std::remove_if(lights_.begin(),lights_.end(),[id](const Light& l){return l.id==id;});if(it==lights_.end())return false;lights_.erase(it,lights_.end());return true;}
Light* LightingWorld::get_light(std::uint64_t id) noexcept{for(auto& l:lights_)if(l.id==id)return &l;return nullptr;} const Light* LightingWorld::get_light(std::uint64_t id) const noexcept{for(const auto& l:lights_)if(l.id==id)return &l;return nullptr;}
void LightingWorld::clear_lights() noexcept{lights_.clear();next_light_id_=1;}
std::vector<const Light*> LightingWorld::active_lights() const{std::vector<const Light*> out;out.reserve(lights_.size());for(const auto& l:lights_)if(l.active)out.push_back(&l);return out;}
bool LightingWorld::set_main_camera(Camera camera) noexcept{if(!camera.valid())return false;camera_=camera;return true;}
void LightingWorld::clear() noexcept{clear_lights();camera_={};settings_={};}
} // namespace exgine
