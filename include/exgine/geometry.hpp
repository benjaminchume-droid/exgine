#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace exgine {

struct Vec3 { float x=0.0f; float y=0.0f; float z=0.0f; };
struct Vec2 { float x=0.0f; float y=0.0f; };
struct Vertex { Vec3 position{}; Vec3 normal{}; Vec2 uv{}; };
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    [[nodiscard]] bool valid() const noexcept { return !vertices.empty() && !indices.empty() && indices.size()%3==0; }
};
struct MeshPart {
    std::string name;
    Mesh mesh;
    std::string material_slot;
    Vec3 position{};
    Vec3 scale{1.0f,1.0f,1.0f};
    Vec3 rotation{};
};
struct MeshAssembly {
    std::vector<MeshPart> parts;
    [[nodiscard]] bool valid() const noexcept;
};
struct BoxShape { Vec3 size{1.0f,1.0f,1.0f}; };
struct SphereShape { float radius=0.5f; std::uint32_t segments=24; std::uint32_t rings=12; };
struct CylinderShape { float radius=0.5f; float height=1.0f; std::uint32_t segments=24; };
Mesh make_box(BoxShape shape={});
Mesh make_sphere(SphereShape shape={});
Mesh make_cylinder(CylinderShape shape={});
Mesh make_capsule(float radius=0.25f,float height=1.0f,std::uint32_t segments=20,std::uint32_t rings=8);

} // namespace exgine
