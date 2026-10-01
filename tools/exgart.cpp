#include "exgine/geometry.hpp"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

using exgine::Mesh;
using exgine::Vertex;
using exgine::Vec3;

constexpr float kPi = 3.14159265358979323846f;

struct MaterialSpec {
    std::string name;
    exgine::Vec3 base_color{1.0f, 1.0f, 1.0f};
    float roughness = 0.5f;
    float metallic = 0.0f;
    float specular = 0.5f;
    float opacity = 1.0f;
};

struct PartSpec {
    std::string name;
    Mesh mesh;
    std::string material;
    Vec3 translation{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
    Vec3 rotation_degrees{};
};

struct ArtScene {
    std::vector<MaterialSpec> materials;
    std::unordered_map<std::string, std::size_t> material_index;
    std::vector<PartSpec> parts;
};

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool parse_float(std::istringstream& in, float& value) {
    std::string token;
    if (!(in >> token)) return false;
    try {
        std::size_t used = 0;
        value = std::stof(token, &used);
        return used == token.size() && std::isfinite(value);
    } catch (...) {
        return false;
    }
}

bool parse_uint(std::istringstream& in, std::uint32_t& value) {
    std::string token;
    if (!(in >> token)) return false;
    try {
        std::size_t used = 0;
        const auto parsed = std::stoul(token, &used);
        if (used != token.size()) return false;
        value = static_cast<std::uint32_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool parse_transform(std::istringstream& in, PartSpec& part, std::string& error) {
    if (!parse_float(in, part.translation.x) ||
        !parse_float(in, part.translation.y) ||
        !parse_float(in, part.translation.z) ||
        !parse_float(in, part.scale.x) ||
        !parse_float(in, part.scale.y) ||
        !parse_float(in, part.scale.z) ||
        !parse_float(in, part.rotation_degrees.x) ||
        !parse_float(in, part.rotation_degrees.y) ||
        !parse_float(in, part.rotation_degrees.z) ||
        !(in >> part.material)) {
        error = "part requires tx ty tz sx sy sz rx ry rz material";
        return false;
    }
    if (part.scale.x == 0.0f || part.scale.y == 0.0f || part.scale.z == 0.0f) {
        error = "part scale components must be non-zero";
        return false;
    }
    return true;
}

bool parse_art(std::istream& in, ArtScene& scene, std::string& error) {
    std::string line;
    std::size_t line_number = 0;
    bool header_seen = false;

    while (std::getline(in, line)) {
        ++line_number;
        const auto comment = line.find('#');
        if (comment != std::string::npos) line.erase(comment);
        line = trim(line);
        if (line.empty()) continue;

        std::istringstream tokens(line);
        std::string directive;
        tokens >> directive;

        if (!header_seen) {
            std::string version;
            if (directive != "EXART" || !(tokens >> version) || version != "1") {
                error = "first directive must be: EXART 1";
                return false;
            }
            header_seen = true;
            continue;
        }

        if (directive == "material") {
            MaterialSpec material;
            if (!(tokens >> material.name) ||
                !parse_float(tokens, material.base_color.x) ||
                !parse_float(tokens, material.base_color.y) ||
                !parse_float(tokens, material.base_color.z) ||
                !parse_float(tokens, material.roughness) ||
                !parse_float(tokens, material.metallic) ||
                !parse_float(tokens, material.specular) ||
                !parse_float(tokens, material.opacity)) {
                error = "invalid material at line " + std::to_string(line_number);
                return false;
            }
            if (material.name.empty() ||
                material.roughness < 0.0f || material.roughness > 1.0f ||
                material.metallic < 0.0f || material.metallic > 1.0f ||
                material.specular < 0.0f || material.specular > 1.0f ||
                material.opacity < 0.0f || material.opacity > 1.0f) {
                error = "material values out of range at line " + std::to_string(line_number);
                return false;
            }
            if (scene.material_index.contains(material.name)) {
                error = "duplicate material: " + material.name;
                return false;
            }
            scene.material_index.emplace(material.name, scene.materials.size());
            scene.materials.push_back(std::move(material));
            continue;
        }

        if (directive == "part") {
            PartSpec part;
            std::string primitive;
            if (!(tokens >> part.name >> primitive)) {
                error = "invalid part header at line " + std::to_string(line_number);
                return false;
            }

            if (primitive == "box") {
                exgine::BoxShape shape;
                if (!parse_float(tokens, shape.size.x) ||
                    !parse_float(tokens, shape.size.y) ||
                    !parse_float(tokens, shape.size.z)) {
                    error = "box requires size_x size_y size_z at line " + std::to_string(line_number);
                    return false;
                }
                part.mesh = exgine::make_box(shape);
            } else if (primitive == "sphere") {
                exgine::SphereShape shape;
                if (!parse_float(tokens, shape.radius) ||
                    !parse_uint(tokens, shape.segments) ||
                    !parse_uint(tokens, shape.rings)) {
                    error = "sphere requires radius segments rings at line " + std::to_string(line_number);
                    return false;
                }
                part.mesh = exgine::make_sphere(shape);
            } else if (primitive == "cylinder") {
                exgine::CylinderShape shape;
                if (!parse_float(tokens, shape.radius) ||
                    !parse_float(tokens, shape.height) ||
                    !parse_uint(tokens, shape.segments)) {
                    error = "cylinder requires radius height segments at line " + std::to_string(line_number);
                    return false;
                }
                part.mesh = exgine::make_cylinder(shape);
            } else if (primitive == "capsule") {
                float radius = 0.25f;
                float height = 1.0f;
                std::uint32_t segments = 20;
                std::uint32_t rings = 8;
                if (!parse_float(tokens, radius) ||
                    !parse_float(tokens, height) ||
                    !parse_uint(tokens, segments) ||
                    !parse_uint(tokens, rings)) {
                    error = "capsule requires radius height segments rings at line " + std::to_string(line_number);
                    return false;
                }
                part.mesh = exgine::make_capsule(radius, height, segments, rings);
            } else {
                error = "unsupported primitive '" + primitive + "' at line " + std::to_string(line_number);
                return false;
            }

            if (!part.mesh.valid()) {
                error = "primitive generated an invalid mesh at line " + std::to_string(line_number);
                return false;
            }

            if (!parse_transform(tokens, part, error)) {
                error += " at line " + std::to_string(line_number);
                return false;
            }

            if (!scene.material_index.contains(part.material)) {
                error = "part references unknown material '" + part.material + "'";
                return false;
            }
            scene.parts.push_back(std::move(part));
            continue;
        }

        error = "unknown directive '" + directive + "' at line " + std::to_string(line_number);
        return false;
    }

    if (!header_seen) {
        error = "empty art resource";
        return false;
    }
    if (scene.materials.empty() || scene.parts.empty()) {
        error = "art resource needs at least one material and one part";
        return false;
    }
    return true;
}

Vec3 rotate_xyz(Vec3 v, Vec3 degrees) {
    const Vec3 radians{
        degrees.x * kPi / 180.0f,
        degrees.y * kPi / 180.0f,
        degrees.z * kPi / 180.0f
    };

    const float cx = std::cos(radians.x), sx = std::sin(radians.x);
    const float cy = std::cos(radians.y), sy = std::sin(radians.y);
    const float cz = std::cos(radians.z), sz = std::sin(radians.z);

    const Vec3 x{
        v.x,
        v.y * cx - v.z * sx,
        v.y * sx + v.z * cx
    };
    const Vec3 y{
        x.x * cy + x.z * sy,
        x.y,
        -x.x * sy + x.z * cy
    };
    return {
        y.x * cz - y.y * sz,
        y.x * sz + y.y * cz,
        y.z
    };
}

Vec3 normalize(Vec3 v) {
    const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 1.0e-8f) return {0.0f, 1.0f, 0.0f};
    return {v.x / len, v.y / len, v.z / len};
}

Vertex transform_vertex(const Vertex& in, const PartSpec& part) {
    Vec3 position{
        in.position.x * part.scale.x,
        in.position.y * part.scale.y,
        in.position.z * part.scale.z
    };
    position = rotate_xyz(position, part.rotation_degrees);
    position.x += part.translation.x;
    position.y += part.translation.y;
    position.z += part.translation.z;

    // Transform normals by inverse scale, then by the part rotation.
    Vec3 normal{
        in.normal.x / part.scale.x,
        in.normal.y / part.scale.y,
        in.normal.z / part.scale.z
    };
    normal = normalize(rotate_xyz(normal, part.rotation_degrees));
    return {position, normal, in.uv};
}

bool write_mtl(const std::filesystem::path& output, const ArtScene& scene, std::string& error) {
    std::ofstream out(output);
    if (!out) {
        error = "cannot open material output: " + output.string();
        return false;
    }

    out << std::fixed << std::setprecision(6);
    out << "# EXGINE generated Wavefront material library\n";
    out << "# Source: .exart resource description\n\n";

    for (const auto& material : scene.materials) {
        out << "newmtl " << material.name << '\n';
        out << "Ka 0.000000 0.000000 0.000000\n";
        out << "Kd " << material.base_color.x << ' ' << material.base_color.y << ' ' << material.base_color.z << '\n';
        out << "Ks " << material.specular << ' ' << material.specular << ' ' << material.specular << '\n';
        out << "Ns " << (material.specular * 1000.0f) << '\n';
        out << "Pr " << material.roughness << '\n';
        out << "Pm " << material.metallic << '\n';
        out << "d " << material.opacity << '\n';
        out << "illum " << (material.opacity < 0.999f ? 4 : 2) << "\n\n";
    }

    return true;
}

bool write_obj(const std::filesystem::path& output,
               const std::filesystem::path& material_output,
               const ArtScene& scene,
               std::string& error) {
    std::ofstream out(output);
    if (!out) {
        error = "cannot open mesh output: " + output.string();
        return false;
    }

    out << std::fixed << std::setprecision(6);
    out << "# EXGINE generated Wavefront mesh\n";
    out << "# Source: .exart resource description\n";
    out << "mtllib " << material_output.filename().generic_string() << "\n\n";

    std::uint32_t base_index = 1;
    std::size_t total_vertices = 0;
    std::size_t total_triangles = 0;

    for (const auto& part : scene.parts) {
        out << "o " << part.name << '\n';
        out << "usemtl " << part.material << '\n';

        for (const auto& vertex : part.mesh.vertices) {
            const auto transformed = transform_vertex(vertex, part);
            out << "v " << transformed.position.x << ' '
                << transformed.position.y << ' '
                << transformed.position.z << '\n';
        }
        for (const auto& vertex : part.mesh.vertices) {
            out << "vt " << vertex.uv.x << ' ' << vertex.uv.y << '\n';
        }
        for (const auto& vertex : part.mesh.vertices) {
            const auto transformed = transform_vertex(vertex, part);
            out << "vn " << transformed.normal.x << ' '
                << transformed.normal.y << ' '
                << transformed.normal.z << '\n';
        }

        for (std::size_t i = 0; i < part.mesh.indices.size(); i += 3) {
            const auto a = base_index + part.mesh.indices[i];
            const auto b = base_index + part.mesh.indices[i + 1];
            const auto c = base_index + part.mesh.indices[i + 2];
            out << "f "
                << a << '/' << a << '/' << a << ' '
                << b << '/' << b << '/' << b << ' '
                << c << '/' << c << '/' << c << '\n';
        }

        base_index += static_cast<std::uint32_t>(part.mesh.vertices.size());
        total_vertices += part.mesh.vertices.size();
        total_triangles += part.mesh.indices.size() / 3;
        out << '\n';
    }

    if (!out) {
        error = "failed while writing mesh output";
        return false;
    }

    std::cout << "generated " << output << ": "
              << total_vertices << " vertices, "
              << total_triangles << " triangles, "
              << scene.parts.size() << " parts, "
              << scene.materials.size() << " materials\n";
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: exgart input.exart output.obj output.mtl\n";
        return 2;
    }

    const std::filesystem::path input_path = argv[1];
    const std::filesystem::path obj_path = argv[2];
    const std::filesystem::path mtl_path = argv[3];

    std::ifstream input(input_path);
    if (!input) {
        std::cerr << "cannot open input: " << input_path << '\n';
        return 3;
    }

    ArtScene scene;
    std::string error;
    if (!parse_art(input, scene, error)) {
        std::cerr << "art parse failed: " << error << '\n';
        return 4;
    }

    if (!obj_path.parent_path().empty()) std::filesystem::create_directories(obj_path.parent_path());
    if (!mtl_path.parent_path().empty()) std::filesystem::create_directories(mtl_path.parent_path());

    if (!write_mtl(mtl_path, scene, error)) {
        std::cerr << error << '\n';
        return 5;
    }
    if (!write_obj(obj_path, mtl_path, scene, error)) {
        std::cerr << error << '\n';
        return 6;
    }
    return 0;
}
