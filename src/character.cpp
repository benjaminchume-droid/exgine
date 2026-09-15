#include "exgine/character.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace exgine {
namespace {

float variation(std::uint64_t seed, std::uint64_t salt, float amplitude) {
    std::uint64_t x = seed + 0x9e3779b97f4a7c15ULL + salt;
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    const float unit = static_cast<float>(x & 0xffffu) / 65535.0f;
    return (unit * 2.0f - 1.0f) * amplitude;
}

void add_part(MeshAssembly& out, std::string name, Mesh mesh, std::string material,
              Vec3 position, Vec3 scale = {1,1,1}) {
    out.parts.push_back({std::move(name), std::move(mesh), std::move(material), position, scale});
}

} // namespace

MeshAssembly generate_character(const CharacterDefinition& definition) {
    const auto& a = definition.appearance;
    const float h = std::clamp(a.height, 1.3f, 2.3f);
    const float build = std::clamp(a.build, 0.65f, 1.45f);
    const float shoulder = 0.38f * build;
    const float leg = h * 0.46f;
    const float torso_h = h * 0.31f;
    const float head_r = h * 0.105f;
    const float torso_y = leg + torso_h * 0.5f;
    const float head_y = leg + torso_h + head_r * 1.2f;
    const float arm_y = torso_y + torso_h * 0.05f;
    const float limb_radius = 0.075f * build;
    const float jitter = variation(a.seed, 11, 0.025f);

    MeshAssembly out;
    add_part(out, "torso", make_capsule(shoulder * 0.72f, torso_h, 16, 6), a.shirt_material,
             {jitter, torso_y, 0});
    add_part(out, "head", make_sphere({head_r, 20, 10}), a.skin_material,
             {jitter, head_y, 0});
    add_part(out, "left_arm", make_capsule(limb_radius, torso_h * 0.92f, 12, 5), a.shirt_material,
             {-shoulder - limb_radius, arm_y, 0});
    add_part(out, "right_arm", make_capsule(limb_radius, torso_h * 0.92f, 12, 5), a.shirt_material,
             {shoulder + limb_radius, arm_y, 0});
    add_part(out, "left_leg", make_capsule(limb_radius * 1.15f, leg * 0.92f, 12, 5), a.pants_material,
             {-0.12f * build, leg * 0.5f, 0});
    add_part(out, "right_leg", make_capsule(limb_radius * 1.15f, leg * 0.92f, 12, 5), a.pants_material,
             {0.12f * build, leg * 0.5f, 0});
    add_part(out, "left_foot", make_box({{0.18f*build, 0.10f*h, 0.30f*build}}), a.shoe_material,
             {-0.12f*build, 0.05f*h, 0.045f*h});
    add_part(out, "right_foot", make_box({{0.18f*build, 0.10f*h, 0.30f*build}}), a.shoe_material,
             {0.12f*build, 0.05f*h, 0.045f*h});

    if (a.watch) {
        add_part(out, "watch", make_cylinder({0.045f, 0.018f, 12}), "watch_metal",
                 {-shoulder - limb_radius, arm_y - torso_h * 0.34f, 0});
    }
    if (a.backpack) {
        add_part(out, "backpack", make_box({{0.28f*build, 0.34f*h, 0.12f*build}}), "backpack_fabric",
                 {0, torso_y, -shoulder * 0.55f});
    }
    return out;
}

} // namespace exgine
