#include "exgine/geometry.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace exgine {
namespace {
constexpr float pi = 3.14159265358979323846f;

void add_quad(Mesh& mesh, Vertex a, Vertex b, Vertex c, Vertex d) {
    const auto base = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back(a); mesh.vertices.push_back(b);
    mesh.vertices.push_back(c); mesh.vertices.push_back(d);
    mesh.indices.insert(mesh.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

void add_ring(Mesh& mesh, float radius, float y, std::uint32_t segments,
              bool inward = false) {
    const float inv = 1.0f / static_cast<float>(segments);
    for (std::uint32_t i = 0; i < segments; ++i) {
        const float t = static_cast<float>(i) * inv * 2.0f * pi;
        const float c = std::cos(t), s = std::sin(t);
        const Vec3 n{c, 0.0f, s};
        mesh.vertices.push_back({{radius * c, y, radius * s}, inward ? Vec3{-c, 0.0f, -s} : n,
                                 {static_cast<float>(i) * inv, 0.0f}});
    }
}
} // namespace

bool MeshAssembly::valid() const noexcept {
    if (parts.empty()) return false;
    for (const auto& part : parts) if (!part.mesh.valid()) return false;
    return true;
}

Mesh make_box(BoxShape shape) {
    const Vec3 h{shape.size.x * 0.5f, shape.size.y * 0.5f, shape.size.z * 0.5f};
    Mesh mesh;
    add_quad(mesh, {{-h.x,-h.y,-h.z},{0,0,-1},{0,0}}, {{h.x,-h.y,-h.z},{0,0,-1},{1,0}},
             {{h.x,h.y,-h.z},{0,0,-1},{1,1}}, {{-h.x,h.y,-h.z},{0,0,-1},{0,1}});
    add_quad(mesh, {{h.x,-h.y,h.z},{0,0,1},{0,0}}, {{-h.x,-h.y,h.z},{0,0,1},{1,0}},
             {{-h.x,h.y,h.z},{0,0,1},{1,1}}, {{h.x,h.y,h.z},{0,0,1},{0,1}});
    add_quad(mesh, {{-h.x,-h.y,h.z},{-1,0,0},{0,0}}, {{-h.x,-h.y,-h.z},{-1,0,0},{1,0}},
             {{-h.x,h.y,-h.z},{-1,0,0},{1,1}}, {{-h.x,h.y,h.z},{-1,0,0},{0,1}});
    add_quad(mesh, {{h.x,-h.y,-h.z},{1,0,0},{0,0}}, {{h.x,-h.y,h.z},{1,0,0},{1,0}},
             {{h.x,h.y,h.z},{1,0,0},{1,1}}, {{h.x,h.y,-h.z},{1,0,0},{0,1}});
    add_quad(mesh, {{-h.x,h.y,-h.z},{0,1,0},{0,0}}, {{h.x,h.y,-h.z},{0,1,0},{1,0}},
             {{h.x,h.y,h.z},{0,1,0},{1,1}}, {{-h.x,h.y,h.z},{0,1,0},{0,1}});
    add_quad(mesh, {{-h.x,-h.y,h.z},{0,-1,0},{0,0}}, {{h.x,-h.y,h.z},{0,-1,0},{1,0}},
             {{h.x,-h.y,-h.z},{0,-1,0},{1,1}}, {{-h.x,-h.y,-h.z},{0,-1,0},{0,1}});
    return mesh;
}

Mesh make_sphere(SphereShape shape) {
    shape.segments = std::max<std::uint32_t>(3, shape.segments);
    shape.rings = std::max<std::uint32_t>(2, shape.rings);
    Mesh mesh;
    for (std::uint32_t r = 0; r <= shape.rings; ++r) {
        const float v = static_cast<float>(r) / static_cast<float>(shape.rings);
        const float phi = v * pi;
        const float y = std::cos(phi) * shape.radius;
        const float ring = std::sin(phi) * shape.radius;
        for (std::uint32_t s = 0; s < shape.segments; ++s) {
            const float u = static_cast<float>(s) / static_cast<float>(shape.segments);
            const float theta = u * 2.0f * pi;
            const float x = std::cos(theta) * ring, z = std::sin(theta) * ring;
            const float inv = shape.radius > std::numeric_limits<float>::epsilon() ? 1.0f / shape.radius : 0.0f;
            mesh.vertices.push_back({{x,y,z},{x*inv,y*inv,z*inv},{u,1.0f-v}});
        }
    }
    for (std::uint32_t r = 0; r < shape.rings; ++r) {
        for (std::uint32_t s = 0; s < shape.segments; ++s) {
            const auto a = r * shape.segments + s;
            const auto b = r * shape.segments + (s + 1) % shape.segments;
            const auto c = (r + 1) * shape.segments + (s + 1) % shape.segments;
            const auto d = (r + 1) * shape.segments + s;
            mesh.indices.insert(mesh.indices.end(), {a,b,c,a,c,d});
        }
    }
    return mesh;
}

Mesh make_cylinder(CylinderShape shape) {
    shape.segments = std::max<std::uint32_t>(3, shape.segments);
    Mesh mesh;
    const float h = shape.height * 0.5f;
    add_ring(mesh, shape.radius, -h, shape.segments);
    add_ring(mesh, shape.radius, h, shape.segments);
    for (std::uint32_t i = 0; i < shape.segments; ++i) {
        const auto j = (i + 1) % shape.segments;
        const auto a = i, b = j, c = shape.segments + j, d = shape.segments + i;
        mesh.indices.insert(mesh.indices.end(), {a,b,c,a,c,d});
    }
    const auto bottom_center = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({{0,-h,0},{0,-1,0},{0.5f,0.5f}});
    const auto top_center = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({{0,h,0},{0,1,0},{0.5f,0.5f}});
    for (std::uint32_t i = 0; i < shape.segments; ++i) {
        const auto j = (i + 1) % shape.segments;
        mesh.indices.insert(mesh.indices.end(), {bottom_center,j,i});
        mesh.indices.insert(mesh.indices.end(), {top_center,shape.segments+i,shape.segments+j});
    }
    return mesh;
}

Mesh make_capsule(float radius, float height, std::uint32_t segments, std::uint32_t rings) {
    segments = std::max<std::uint32_t>(3, segments);
    rings = std::max<std::uint32_t>(2, rings);
    radius = std::max(0.0f, radius);
    const float cylinder_height = std::max(0.0f, height - 2.0f * radius);
    Mesh mesh;
    const std::uint32_t total_rings = rings * 2 + 1;
    for (std::uint32_t r = 0; r <= total_rings; ++r) {
        const float t = static_cast<float>(r) / static_cast<float>(total_rings);
        float y = 0.0f, ring = radius, normal_y = 0.0f;
        if (r <= rings) {
            const float phi = (static_cast<float>(r) / static_cast<float>(rings)) * (pi * 0.5f);
            ring = std::cos(phi) * radius;
            y = cylinder_height * 0.5f + std::sin(phi) * radius;
            normal_y = std::sin(phi);
        } else {
            const float phi = (static_cast<float>(r - rings) / static_cast<float>(rings)) * (pi * 0.5f);
            ring = std::sin(phi) * radius;
            y = -cylinder_height * 0.5f - std::cos(phi) * radius;
            normal_y = -std::cos(phi);
        }
        for (std::uint32_t s = 0; s < segments; ++s) {
            const float u = static_cast<float>(s) / static_cast<float>(segments);
            const float theta = u * 2.0f * pi;
            const float x = std::cos(theta) * ring, z = std::sin(theta) * ring;
            const float inv = radius > std::numeric_limits<float>::epsilon() ? 1.0f / radius : 0.0f;
            mesh.vertices.push_back({{x,y,z},{x*inv,normal_y,z*inv},{u,1.0f-t}});
        }
    }
    for (std::uint32_t r = 0; r < total_rings; ++r) for (std::uint32_t s = 0; s < segments; ++s) {
        const auto a = r*segments+s, b=r*segments+(s+1)%segments;
        const auto c=(r+1)*segments+(s+1)%segments, d=(r+1)*segments+s;
        mesh.indices.insert(mesh.indices.end(), {a,b,c,a,c,d});
    }
    return mesh;
}

} // namespace exgine
