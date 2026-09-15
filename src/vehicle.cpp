#include "exgine/vehicle.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace exgine {
namespace {
constexpr float pi = 3.14159265358979323846f;

bool finite(float v) noexcept { return std::isfinite(v); }
bool finite3(Vec3 v) noexcept { return finite(v.x) && finite(v.y) && finite(v.z); }
std::uint64_t mix64(std::uint64_t v) noexcept {
    v += 0x9e3779b97f4a7c15ULL;
    v = (v ^ (v >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    v = (v ^ (v >> 27U)) * 0x94d049bb133111ebULL;
    return v ^ (v >> 31U);
}
std::uint64_t id(std::uint64_t seed, std::uint64_t salt) noexcept { return mix64(seed ^ salt) | 1ULL; }
float unit(std::uint64_t v) noexcept { return static_cast<float>(mix64(v) >> 40U) / static_cast<float>(1U << 24U); }

void add_part(MeshAssembly& a, std::string name, Mesh mesh, std::string material, Vec3 position, Vec3 scale={1,1,1}, Vec3 rotation={}) {
    if (!mesh.valid() || material.empty()) return;
    a.parts.push_back({std::move(name), std::move(mesh), std::move(material), position, scale, rotation});
}
void add_box(MeshAssembly& a, std::string name, Vec3 size, const std::string& material, Vec3 p, Vec3 rotation={}) {
    if (size.x > 0 && size.y > 0 && size.z > 0) add_part(a, std::move(name), make_box({size}), material, p, {1,1,1}, rotation);
}
void add_cylinder(MeshAssembly& a, std::string name, float radius, float height, const std::string& material, Vec3 p, Vec3 rotation={}) {
    if (radius > 0 && height > 0) add_part(a, std::move(name), make_cylinder({radius,height,24}), material, p, {1,1,1}, rotation);
}
void add_sphere(MeshAssembly& a, std::string name, float radius, const std::string& material, Vec3 p) {
    if (radius > 0) add_part(a, std::move(name), make_sphere({radius,20,10}), material, p);
}

VehicleConfig sanitize(VehicleConfig c) {
    c.length = std::max(c.length, 0.8f);
    c.width = std::max(c.width, 0.35f);
    c.height = std::max(c.height, 0.45f);
    c.wheel_radius = std::clamp(c.wheel_radius, 0.03f, std::min(c.height * 0.45f, c.length * 0.2f));
    c.wheel_width = std::clamp(c.wheel_width, 0.02f, c.width * 0.25f);
    c.ground_clearance = std::clamp(c.ground_clearance, 0.0f, c.height * 0.45f);
    const float axle_span_limit = std::max(0.1f, c.length - 2.0f * c.wheel_radius);
    const float track_limit = std::max(0.05f, c.width - 2.0f * c.wheel_width);
    c.wheelbase = std::clamp(c.wheelbase, std::min(0.1f, axle_span_limit), axle_span_limit);
    c.track_width = std::clamp(c.track_width, std::min(0.05f, track_limit), track_limit);
    c.axles = std::clamp(c.axles, 0U, 8U);
    c.wheels_per_axle = std::clamp(c.wheels_per_axle, 1U, 4U);
    c.cabin_length = std::clamp(c.cabin_length, 0.1f, c.length);
    c.cabin_width = std::clamp(c.cabin_width, 0.1f, c.width);
    c.cabin_height = std::clamp(c.cabin_height, 0.1f, c.height);
    c.front_overhang = std::max(0.0f, c.front_overhang);
    c.rear_overhang = std::max(0.0f, c.rear_overhang);
    c.chassis_height = std::clamp(c.chassis_height, 0.02f, c.height);
    c.hood_length = std::clamp(c.hood_length, 0.0f, c.length);
    c.doors = std::clamp(c.doors, 0U, 8U);
    c.seats = std::clamp(c.seats, 0U, 64U);
    c.wing_span = std::clamp(c.wing_span, 0.0f, c.width);
    c.waterline = std::clamp(c.waterline, -c.height, c.height);
    return c;
}

void add_collision(VehicleDefinition& v, Vec3 size, Vec3 p, bool wheel=false) {
    const auto next = static_cast<std::uint64_t>(v.collision.size()+1U);
    v.collision.push_back({next, {p.x-size.x*.5f,p.y-size.y*.5f,p.z-size.z*.5f}, {p.x+size.x*.5f,p.y+size.y*.5f,p.z+size.z*.5f}, wheel});
}

void add_wheels(VehicleDefinition& v, float axle_front_z, float axle_rear_z, bool steering_front) {
    const auto& c=v.config;
    const std::uint32_t axles = c.axles;
    const std::uint32_t per = c.wheels_per_axle;
    if (axles == 0) return;
    for (std::uint32_t axle=0; axle<axles; ++axle) {
        const float t = axles==1 ? 0.5f : static_cast<float>(axle)/static_cast<float>(axles-1U);
        const float z = axle_front_z*(1.0f-t) + axle_rear_z*t;
        for (std::uint32_t wi=0; wi<per; ++wi) {
            const bool symmetric = per == 2U || per == 4U;
            const float lateral = symmetric ? (wi % 2U == 0U ? -0.5f : 0.5f) * c.track_width : 0.0f;
            const bool steering = steering_front && axle==0U;
            const bool driven = axle == axles-1U || c.type==VehicleType::SportsCar;
            const auto role = steering ? VehicleWheelRole::Steering : (driven ? VehicleWheelRole::Driven : VehicleWheelRole::FreeRolling);
            const auto wid = id(v.seed, 0x5000ULL + (static_cast<std::uint64_t>(axle)<<16U) + wi);
            v.wheels.push_back({wid,axle,wi,role,{lateral,c.wheel_radius,z},c.wheel_radius,c.wheel_width,c.wheel_radius*.25f,c.wheel_radius*.35f,driven,steering});
            add_cylinder(v.geometry,"wheel_"+std::to_string(axle)+"_"+std::to_string(wi),c.wheel_radius,c.wheel_width,c.rubber_material,{lateral,c.wheel_radius,z},{0,0,pi*.5f});
            add_collision(v,{c.wheel_width,c.wheel_radius*2.0f,c.wheel_radius*2.0f},{lateral,c.wheel_radius,z},true);
            const auto attach=id(v.seed,0x6000ULL + (static_cast<std::uint64_t>(axle)<<16U) + wi);
            v.physics_attachments.push_back({attach,"wheel_mount_"+std::to_string(axle)+"_"+std::to_string(wi),{lateral,c.ground_clearance+c.wheel_radius,z},{0,1,0}});
        }
    }
}

void add_land_vehicle(VehicleDefinition& v) {
    auto& c=v.config;
    const float bottom=c.ground_clearance;
    const float chassis_y=bottom + c.chassis_height*.5f;
    add_box(v.geometry,"chassis",{c.width*.86f,c.chassis_height,c.length*.94f},c.metal_material,{0,chassis_y,0});
    add_collision(v,{c.width*.86f,c.chassis_height,c.length*.94f},{0,chassis_y,0});

    const float body_h=std::max(c.chassis_height+0.12f,c.height*0.38f);
    const float body_center_y=bottom + c.chassis_height + body_h*.5f;
    add_box(v.geometry,"body_lower",{c.width*.94f,body_h,c.length*.90f},c.body_material,{0,body_center_y,0});
    add_collision(v,{c.width*.94f,body_h,c.length*.90f},{0,body_center_y,0});

    if (c.type==VehicleType::Pickup || c.type==VehicleType::Truck || c.type==VehicleType::Construction) {
        const float rear = std::max(0.0f, c.length*.22f);
        add_box(v.geometry,"cargo_bed",{c.width*.88f,body_h*.65f,rear},c.body_material,{0,bottom+c.chassis_height+body_h*.72f,c.length*.5f-rear*.5f});
    }

    const float cabin_base=bottom+c.chassis_height+body_h*.72f;
    const float cabin_center_z = (c.front_overhang - c.rear_overhang) * 0.5f;
    float cabin_len=std::min(c.cabin_length,c.length);
    if(c.type==VehicleType::Pickup || c.type==VehicleType::Truck || c.type==VehicleType::Construction) cabin_len=std::min(cabin_len,c.length*.55f);
    add_box(v.geometry,"cabin",{c.cabin_width,c.cabin_height,cabin_len},c.glass_material,{0,cabin_base+c.cabin_height*.5f,cabin_center_z}, {0,0,0});
    add_box(v.geometry,"interior",{c.cabin_width*.94f,c.cabin_height*.72f,cabin_len*.94f},c.interior_material,{0,cabin_base+c.cabin_height*.42f,cabin_center_z});
    add_collision(v,{c.cabin_width,c.cabin_height,cabin_len},{0,cabin_base+c.cabin_height*.5f,cabin_center_z});

    const float half=c.length*.5f;
    add_box(v.geometry,"front_panel",{c.width*.90f,c.height*.16f,std::min(c.hood_length,c.length*.3f)},c.body_material,{0,bottom+c.chassis_height+body_h+c.height*.08f,-half+std::min(c.hood_length,c.length*.3f)*.5f});

    const std::uint32_t door_count=std::min(c.doors,8U);
    for(std::uint32_t i=0;i<door_count;++i){
        const bool rear=(i>=2U);
        const float side=(i%2U==0U)?-1.0f:1.0f;
        const float z=(rear?-c.length*.05f:c.length*.12f) + (unit(v.seed+i)-.5f)*std::min(.15f,cabin_len*.05f);
        const auto did=id(v.seed,0x1000ULL+i);
        const Vec3 p{side*(c.width*.5f+.006f),cabin_base+c.cabin_height*.48f,z};
        const Vec3 s{.04f,c.cabin_height*.78f,std::max(.25f,cabin_len*.32f)};
        v.doors.push_back({did,(side<0)?VehicleDoorSide::Left:VehicleDoorSide::Right,p,s,false});
        add_box(v.geometry,"door_"+std::to_string(i),s,c.body_material,p);
    }

    const std::uint32_t seats=std::min<std::uint32_t>(c.seats,64U);
    for(std::uint32_t i=0;i<seats;++i){
        const std::uint32_t row=i/2U, side_index=i%2U;
        const float x=(side_index==0U?-1.0f:1.0f)*std::min(c.cabin_width*.27f,c.width*.2f);
        const float z=-cabin_len*.32f + static_cast<float>(row)*std::min(.62f,cabin_len*.34f);
        const auto sid=id(v.seed,0x2000ULL+i);
        v.seats.push_back({sid,{x,cabin_base+.20f,z},c.cabin_width*.28f,i==0U});
        add_box(v.geometry,"seat_"+std::to_string(i),{c.cabin_width*.25f,.24f,std::min(.48f,cabin_len*.24f)},c.interior_material,{x,cabin_base+.16f,z});
    }

    const float axle_front = std::clamp(c.wheelbase*.5f,-c.length*.5f+c.wheel_radius,c.length*.5f-c.wheel_radius);
    const float axle_rear = -axle_front;
    add_wheels(v,axle_front,axle_rear,true);

    const float light_z=-c.length*.5f+c.front_overhang*.55f;
    v.lights.push_back({id(v.seed,0x3000),{-c.width*.30f,bottom+c.height*.58f,light_z},{0,0,-1},1.0f,false,false});
    v.lights.push_back({id(v.seed,0x3001),{ c.width*.30f,bottom+c.height*.58f,light_z},{0,0,-1},1.0f,false,false});
    v.lights.push_back({id(v.seed,0x3002),{-c.width*.30f,bottom+c.height*.42f,c.length*.5f-c.rear_overhang*.45f},{0,0,1},1.0f,true,false});
    v.lights.push_back({id(v.seed,0x3003),{ c.width*.30f,bottom+c.height*.42f,c.length*.5f-c.rear_overhang*.45f},{0,0,1},1.0f,true,false});
    for(const auto& l:v.lights) add_sphere(v.geometry,"light_"+std::to_string(l.id),.035f,c.metal_material,l.position);

    v.physics_attachments.push_back({id(v.seed,0x7000),"center_of_mass",{0,bottom+c.height*.5f,0},{0,1,0}});
    v.physics_attachments.push_back({id(v.seed,0x7001),"front_axle",{0,c.wheel_radius,axle_front},{0,1,0}});
    v.physics_attachments.push_back({id(v.seed,0x7002),"rear_axle",{0,c.wheel_radius,axle_rear},{0,1,0}});
}

void add_motorcycle(VehicleDefinition& v) {
    auto& c=v.config;
    add_box(v.geometry,"frame",{c.width*.28f,c.chassis_height,c.length*.72f},c.metal_material,{0,c.ground_clearance+c.chassis_height*.5f,0});
    add_box(v.geometry,"tank",{c.width*.55f,c.height*.24f,c.length*.22f},c.body_material,{0,c.ground_clearance+c.chassis_height+c.height*.25f,-c.length*.08f});
    add_box(v.geometry,"seat",{c.width*.48f,c.height*.12f,c.length*.24f},c.interior_material,{0,c.ground_clearance+c.height*.48f,c.length*.18f});
    const float front=c.length*.35f,rear=-c.length*.35f;
    add_wheels(v,front,rear,true);
    v.physics_attachments.push_back({id(v.seed,0x7000),"center_of_mass",{0,c.ground_clearance+c.height*.4f,0},{0,1,0}});
}

void add_boat(VehicleDefinition& v) {
    auto& c=v.config;
    const float hull_h=std::max(.1f,c.height*.45f);
    add_box(v.geometry,"hull",{c.width*.92f,hull_h,c.length*.92f},c.body_material,{0,c.waterline-hull_h*.1f,0});
    add_box(v.geometry,"deck",{c.width*.82f,c.height*.10f,c.length*.76f},c.body_material,{0,c.waterline+c.height*.18f,0});
    add_box(v.geometry,"cabin",{c.cabin_width,c.cabin_height,c.cabin_length},c.glass_material,{0,c.waterline+c.height*.18f+c.cabin_height*.5f,c.length*.08f});
    add_collision(v,{c.width*.92f,hull_h,c.length*.92f},{0,c.waterline-hull_h*.1f,0});
    v.physics_attachments.push_back({id(v.seed,0x7000),"buoyancy_center",{0,c.waterline-c.height*.10f,0},{0,1,0}});
    v.physics_attachments.push_back({id(v.seed,0x7001),"center_of_mass",{0,c.waterline+c.height*.10f,0},{0,1,0}});
}

void add_aircraft(VehicleDefinition& v) {
    auto& c=v.config;
    const float span=c.wing_span>0?std::min(c.wing_span,c.width):c.width*.9f;
    add_box(v.geometry,"fuselage",{c.width*.36f,c.height*.34f,c.length*.92f},c.body_material,{0,c.ground_clearance+c.height*.45f,0});
    add_box(v.geometry,"wing",{span,c.height*.06f,std::max(.3f,c.length*.28f)},c.metal_material,{0,c.ground_clearance+c.height*.42f,0});
    add_box(v.geometry,"tailplane",{span*.45f,c.height*.045f,std::max(.2f,c.length*.12f)},c.metal_material,{0,c.ground_clearance+c.height*.55f,c.length*.36f});
    add_box(v.geometry,"cockpit",{c.width*.31f,c.height*.18f,c.length*.18f},c.glass_material,{0,c.ground_clearance+c.height*.61f,-c.length*.23f});
    add_collision(v,{c.width*.36f,c.height*.34f,c.length*.92f},{0,c.ground_clearance+c.height*.45f,0});
    v.physics_attachments.push_back({id(v.seed,0x7000),"center_of_mass",{0,c.ground_clearance+c.height*.45f,0},{0,1,0}});
    const float gear_z=-c.length*.24f;
    for(int i=0;i<3;++i){const float x=i==0?0:(i==1?-span*.25f:span*.25f);add_cylinder(v.geometry,"landing_gear_"+std::to_string(i),c.wheel_radius,c.wheel_width,c.rubber_material,{x,c.wheel_radius,gear_z},{0,0,pi*.5f});}
}
} // namespace

VehicleConfig make_vehicle_config(VehicleType type) {
    VehicleConfig c; c.type=type;
    switch(type) {
    case VehicleType::SUV: c.length=4.9f;c.width=2.0f;c.height=1.8f;c.wheelbase=2.9f;c.track_width=1.68f;c.wheel_radius=.37f;c.cabin_length=2.35f;c.cabin_width=1.76f;c.cabin_height=.9f;c.seats=7;c.doors=4;break;
    case VehicleType::SportsCar: c.length=4.45f;c.width=1.92f;c.height=1.22f;c.ground_clearance=.11f;c.wheelbase=2.65f;c.track_width=1.62f;c.wheel_radius=.34f;c.cabin_length=1.85f;c.cabin_height=.65f;c.seats=2;c.doors=2;break;
    case VehicleType::Pickup: c.length=5.5f;c.width=2.0f;c.height=1.85f;c.wheelbase=3.3f;c.track_width=1.7f;c.wheel_radius=.40f;c.cabin_length=2.1f;c.cabin_height=.85f;c.seats=5;c.doors=4;break;
    case VehicleType::Truck: c.length=10.5f;c.width=2.5f;c.height=3.4f;c.ground_clearance=.35f;c.wheelbase=5.8f;c.track_width=2.05f;c.wheel_radius=.55f;c.wheel_width=.32f;c.axles=3;c.cabin_length=2.2f;c.cabin_width=2.1f;c.cabin_height=1.25f;c.seats=3;c.doors=2;break;
    case VehicleType::Bus: c.length=12.0f;c.width=2.5f;c.height=3.1f;c.ground_clearance=.3f;c.wheelbase=6.0f;c.track_width=2.05f;c.wheel_radius=.52f;c.wheel_width=.30f;c.axles=3;c.cabin_length=8.0f;c.cabin_width=2.2f;c.cabin_height=1.3f;c.seats=40;c.doors=3;break;
    case VehicleType::Motorcycle: c.length=2.25f;c.width=.86f;c.height=1.45f;c.ground_clearance=.13f;c.wheelbase=1.5f;c.track_width=.55f;c.wheel_radius=.31f;c.wheel_width=.12f;c.axles=2;c.wheels_per_axle=1;c.cabin_length=.5f;c.seats=2;c.doors=0;break;
    case VehicleType::Construction: c.length=8.0f;c.width=2.6f;c.height=3.5f;c.ground_clearance=.4f;c.wheelbase=4.5f;c.track_width=2.1f;c.wheel_radius=.62f;c.wheel_width=.38f;c.axles=3;c.cabin_length=2.3f;c.cabin_width=2.2f;c.cabin_height=1.2f;c.seats=2;c.doors=2;break;
    case VehicleType::Emergency: c.length=5.4f;c.width=2.0f;c.height=2.0f;c.wheelbase=3.2f;c.track_width=1.7f;c.wheel_radius=.39f;c.cabin_length=2.6f;c.cabin_height=.95f;c.seats=5;c.doors=4;break;
    case VehicleType::Boat: c.length=8.0f;c.width=2.8f;c.height=2.6f;c.cabin_length=2.2f;c.cabin_width=2.0f;c.cabin_height=.95f;c.axles=0;c.wheels_per_axle=1;c.seats=8;c.doors=1;c.waterline=.0f;break;
    case VehicleType::Aircraft: c.length=14.0f;c.width=3.2f;c.height=4.0f;c.ground_clearance=.2f;c.wheel_radius=.28f;c.wheel_width=.16f;c.axles=1;c.wheels_per_axle=2;c.cabin_length=3.2f;c.cabin_width=1.9f;c.cabin_height=1.0f;c.wing_span=14.0f;c.seats=8;c.doors=2;break;
    case VehicleType::Car: break;
    }
    return c;
}

bool VehicleDefinition::valid() const noexcept {
    if (!geometry.valid() || config.length<=0 || config.width<=0 || config.height<=0 || !finite(config.length) || !finite(config.width) || !finite(config.height)) return false;
    for (const auto& w:wheels) if (!w.id || !finite3(w.position) || w.radius<=0 || w.width<=0 || w.position.x < -config.width*.5f-w.width || w.position.x > config.width*.5f+w.width || w.position.z < -config.length*.5f-w.radius || w.position.z > config.length*.5f+w.radius) return false;
    for (const auto& d:doors) if (!d.id || !finite3(d.position) || !finite3(d.size) || d.size.x<=0 || d.size.y<=0 || d.size.z<=0) return false;
    for (const auto& s:seats) if (!s.id || !finite3(s.position) || s.width<=0 || !finite(s.width)) return false;
    for (const auto& l:lights) if (!l.id || !finite3(l.position) || !finite3(l.direction)) return false;
    for (const auto& a:physics_attachments) if (!a.id || a.name.empty() || !finite3(a.position) || !finite3(a.axis)) return false;
    for (const auto& c:collision) if (!c.id || !finite3(c.min) || !finite3(c.max) || c.min.x>c.max.x || c.min.y>c.max.y || c.min.z>c.max.z) return false;
    return true;
}

VehicleDefinition generate_vehicle(const VehicleConfig& input) {
    VehicleDefinition v; v.config=sanitize(input); v.seed=v.config.seed;
    if (v.seed==0) v.seed=0xE6A5E6A5ULL;
    v.config.seed=v.seed;
    switch(v.config.type) {
    case VehicleType::Motorcycle: add_motorcycle(v); break;
    case VehicleType::Boat: add_boat(v); break;
    case VehicleType::Aircraft: add_aircraft(v); break;
    default: add_land_vehicle(v); break;
    }
    if(v.geometry.parts.empty()) return v;
    return v;
}

bool set_vehicle_door_open(VehicleDefinition& vehicle, std::uint64_t door_id, bool open) noexcept {
    for (auto& d:vehicle.doors) if(d.id==door_id){d.open=open;return true;}
    return false;
}
const VehicleDoor* find_vehicle_door(const VehicleDefinition& vehicle, std::uint64_t id) noexcept { for(const auto& d:vehicle.doors) if(d.id==id)return &d; return nullptr; }
const VehicleWheel* find_vehicle_wheel(const VehicleDefinition& vehicle, std::uint64_t id) noexcept { for(const auto& w:vehicle.wheels) if(w.id==id)return &w; return nullptr; }
std::vector<VehicleCollisionVolume> active_vehicle_collision(const VehicleDefinition& vehicle) {
    std::vector<VehicleCollisionVolume> result; result.reserve(vehicle.collision.size());
    for(const auto& c:vehicle.collision) if(!c.wheel) result.push_back(c);
    for(const auto& c:vehicle.collision) if(c.wheel) result.push_back(c);
    return result;
}

} // namespace exgine
