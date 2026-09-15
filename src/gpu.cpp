#include "exgine/gpu.hpp"
#include "exgine/shaders.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

namespace exgine {
namespace {
constexpr GlEnum GL_COLOR_BUFFER_BIT = 0x00004000;
constexpr GlEnum GL_DEPTH_BUFFER_BIT = 0x00000100;
constexpr GlEnum GL_DEPTH_TEST = 0x0B71;
constexpr GlEnum GL_CULL_FACE = 0x0B44;
constexpr GlEnum GL_BLEND = 0x0BE2;
constexpr GlEnum GL_LESS = 0x0201;
constexpr GlEnum GL_SRC_ALPHA = 0x0302;
constexpr GlEnum GL_ONE_MINUS_SRC_ALPHA = 0x0303;
constexpr GlEnum GL_VERTEX_SHADER = 0x8B31;
constexpr GlEnum GL_FRAGMENT_SHADER = 0x8B30;
constexpr GlEnum GL_COMPILE_STATUS = 0x8B81;
constexpr GlEnum GL_LINK_STATUS = 0x8B82;
constexpr GlEnum GL_INFO_LOG_LENGTH = 0x8B84;
constexpr GlEnum GL_ARRAY_BUFFER = 0x8892;
constexpr GlEnum GL_ELEMENT_ARRAY_BUFFER = 0x8893;
constexpr GlEnum GL_STATIC_DRAW = 0x88E4;
constexpr GlEnum GL_FLOAT = 0x1406;
constexpr GlEnum GL_UNSIGNED_INT = 0x1405;
constexpr GlEnum GL_TRIANGLES = 0x0004;
constexpr GlEnum GL_TEXTURE_2D = 0x0DE1;
constexpr GlEnum GL_TEXTURE0 = 0x84C0;
constexpr GlEnum GL_TEXTURE_MIN_FILTER = 0x2801;
constexpr GlEnum GL_TEXTURE_MAG_FILTER = 0x2800;
constexpr GlEnum GL_TEXTURE_WRAP_S = 0x2802;
constexpr GlEnum GL_TEXTURE_WRAP_T = 0x2803;
constexpr GlEnum GL_LINEAR = 0x2601;
constexpr GlEnum GL_LINEAR_MIPMAP_LINEAR = 0x2703;
constexpr GlEnum GL_REPEAT = 0x2901;
constexpr GlEnum GL_RGBA8 = 0x8058;
constexpr GlEnum GL_RGBA = 0x1908;
constexpr GlEnum GL_UNSIGNED_BYTE = 0x1401;

std::uint64_t hash64(std::uint64_t x) noexcept {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

std::uint64_t mesh_key(const RenderDrawCall& draw) noexcept {
    const auto ptr = draw.animated()
        ? reinterpret_cast<std::uintptr_t>(draw.skinned_mesh.get())
        : reinterpret_cast<std::uintptr_t>(draw.geometry.get());
    std::uint64_t key = hash64(static_cast<std::uint64_t>(ptr));
    key ^= hash64(static_cast<std::uint64_t>(draw.part_index) + 0x9e3779b97f4a7c15ULL);
    key ^= hash64(static_cast<std::uint64_t>(draw.animated()) + 0x243f6a8885a308d3ULL);
    if (draw.animated() && draw.skinned_mesh) {
        key ^= hash64(static_cast<std::uint64_t>(draw.skinned_mesh->vertices.size()) << 1);
        key ^= hash64(static_cast<std::uint64_t>(draw.skinned_mesh->indices.size()) << 2);
    } else if (draw.geometry && draw.part_index < draw.geometry->parts.size()) {
        const auto& mesh = draw.geometry->parts[draw.part_index].mesh;
        key ^= hash64(static_cast<std::uint64_t>(mesh.vertices.size()) << 1);
        key ^= hash64(static_cast<std::uint64_t>(mesh.indices.size()) << 2);
    }
    return key == 0 ? 1 : key;
}

std::uint64_t texture_key(const std::shared_ptr<const Texture2D>& texture) noexcept {
    if (!texture) return 0;
    return hash64(static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(texture.get()))) ^
           hash64((static_cast<std::uint64_t>(texture->width) << 32) | texture->height);
}
} // namespace

bool OpenGLESApi::complete() const noexcept {
    return Clear && ClearColor && Viewport && Enable && Disable && DepthFunc && BlendFunc &&
           CreateShader && ShaderSource && CompileShader && GetShaderiv && GetShaderInfoLog &&
           DeleteShader && CreateProgram && AttachShader && LinkProgram && GetProgramiv &&
           GetProgramInfoLog && UseProgram && DeleteProgram && GetUniformLocation &&
           UniformMatrix4fv && Uniform3f && Uniform4f && Uniform1f && Uniform1i && GenBuffers &&
           BindBuffer && BufferData && DeleteBuffers && GenVertexArrays && BindVertexArray &&
           DeleteVertexArrays && EnableVertexAttribArray && VertexAttribPointer && DrawElements;
}

bool OpenGLESRenderer::ready() const noexcept {
    return api_.complete() && program_ != 0 && skinned_program_ != 0;
}

bool OpenGLESRenderer::texture_ready() const noexcept {
    return ready() && textured_program_ != 0 && skinned_textured_program_ != 0 &&
           api_.GenTextures && api_.BindTexture && api_.TexParameteri && api_.TexImage2D &&
           api_.GenerateMipmap && api_.ActiveTexture && api_.DeleteTextures;
}

bool OpenGLESRenderer::compile_shader(GlEnum type, const char* source, GlUInt& shader, std::string& error) {
    shader = api_.CreateShader(type);
    if (!shader) {
        error = "OpenGL ES glCreateShader failed";
        return false;
    }
    const GlInt length = static_cast<GlInt>(std::strlen(source));
    api_.ShaderSource(shader, 1, &source, &length);
    api_.CompileShader(shader);
    GlInt ok = 0;
    api_.GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GlInt log_length = 0;
        api_.GetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<char> log(static_cast<std::size_t>(std::max(1, log_length)), 0);
        GlInt written = 0;
        api_.GetShaderInfoLog(shader, log_length, &written, log.data());
        error.assign(log.data(), static_cast<std::size_t>(std::max(0, written)));
        api_.DeleteShader(shader);
        shader = 0;
        return false;
    }
    return true;
}

bool OpenGLESRenderer::link_program(const char*, GlUInt vertex_shader, const char* fragment_source,
                                    GlUInt& program, std::string& error) {
    GlUInt fragment_shader = 0;
    if (!compile_shader(GL_FRAGMENT_SHADER, fragment_source, fragment_shader, error)) return false;

    program = api_.CreateProgram();
    if (!program) {
        api_.DeleteShader(fragment_shader);
        error = "OpenGL ES glCreateProgram failed";
        return false;
    }

    api_.AttachShader(program, vertex_shader);
    api_.AttachShader(program, fragment_shader);
    api_.LinkProgram(program);
    api_.DeleteShader(vertex_shader);
    api_.DeleteShader(fragment_shader);

    GlInt ok = 0;
    api_.GetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        GlInt log_length = 0;
        api_.GetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<char> log(static_cast<std::size_t>(std::max(1, log_length)), 0);
        GlInt written = 0;
        api_.GetProgramInfoLog(program, log_length, &written, log.data());
        error.assign(log.data(), static_cast<std::size_t>(std::max(0, written)));
        api_.DeleteProgram(program);
        program = 0;
        return false;
    }
    return true;
}

bool OpenGLESRenderer::ensure_programs(std::string& error) {
    if (program_ && skinned_program_) return true;
    if (!api_.complete()) {
        error = "OpenGL ES API table is incomplete";
        return false;
    }

    const auto mobile = make_mobile_pbr_shader();
    const auto skinned = make_mobile_skinned_pbr_shader();
    const auto textured = make_mobile_textured_pbr_shader();
    const auto skinned_textured = make_mobile_skinned_textured_pbr_shader();

    GlUInt vertex_shader = 0;
    if (!compile_shader(GL_VERTEX_SHADER, mobile.vertex_source.c_str(), vertex_shader, error)) return false;
    if (!link_program(mobile.vertex_source.c_str(), vertex_shader, mobile.fragment_source.c_str(), program_, error)) return false;

    GlUInt skinned_vertex_shader = 0;
    if (!compile_shader(GL_VERTEX_SHADER, skinned.vertex_source.c_str(), skinned_vertex_shader, error)) {
        release();
        return false;
    }
    if (!link_program(skinned.vertex_source.c_str(), skinned_vertex_shader, skinned.fragment_source.c_str(), skinned_program_, error)) {
        release();
        return false;
    }

    u_vp_ = api_.GetUniformLocation(program_, "u_view_projection");
    u_model_ = api_.GetUniformLocation(program_, "u_model");
    u_camera_ = api_.GetUniformLocation(program_, "u_camera_position");
    u_ambient_ = api_.GetUniformLocation(program_, "u_ambient_intensity");
    u_base_ = api_.GetUniformLocation(program_, "u_base_roughness");
    u_metal_ = api_.GetUniformLocation(program_, "u_metal_specular");
    u_emission_ = api_.GetUniformLocation(program_, "u_emission_opacity");
    u_light_dir_ = api_.GetUniformLocation(program_, "u_light_direction");
    u_light_color_ = api_.GetUniformLocation(program_, "u_light_color");
    u_light_intensity_ = api_.GetUniformLocation(program_, "u_light_intensity");

    s_vp_ = api_.GetUniformLocation(skinned_program_, "u_view_projection");
    s_model_ = api_.GetUniformLocation(skinned_program_, "u_model");
    s_camera_ = api_.GetUniformLocation(skinned_program_, "u_camera_position");
    s_ambient_ = api_.GetUniformLocation(skinned_program_, "u_ambient_intensity");
    s_base_ = api_.GetUniformLocation(skinned_program_, "u_base_roughness");
    s_metal_ = api_.GetUniformLocation(skinned_program_, "u_metal_specular");
    s_emission_ = api_.GetUniformLocation(skinned_program_, "u_emission_opacity");
    s_light_dir_ = api_.GetUniformLocation(skinned_program_, "u_light_direction");
    s_light_color_ = api_.GetUniformLocation(skinned_program_, "u_light_color");
    s_light_intensity_ = api_.GetUniformLocation(skinned_program_, "u_light_intensity");
    s_bones_ = api_.GetUniformLocation(skinned_program_, "u_bones[0]");
    s_bone_count_ = api_.GetUniformLocation(skinned_program_, "u_bone_count");

    if (u_vp_ < 0 || u_model_ < 0 || u_camera_ < 0 || u_ambient_ < 0 || u_base_ < 0 || u_metal_ < 0 ||
        u_emission_ < 0 || u_light_dir_ < 0 || u_light_color_ < 0 || u_light_intensity_ < 0 ||
        s_vp_ < 0 || s_model_ < 0 || s_camera_ < 0 || s_ambient_ < 0 || s_base_ < 0 || s_metal_ < 0 ||
        s_emission_ < 0 || s_light_dir_ < 0 || s_light_color_ < 0 || s_light_intensity_ < 0 ||
        s_bones_ < 0 || s_bone_count_ < 0) {
        release();
        error = "OpenGL ES skeletal shader interface mismatch";
        return false;
    }

    if (texture_ready()) return true;
    if (api_.GenTextures && api_.BindTexture && api_.TexParameteri && api_.TexImage2D &&
        api_.GenerateMipmap && api_.ActiveTexture && api_.DeleteTextures) {
        std::string optional_error;
        GlUInt textured_vertex_shader = 0;
        if (compile_shader(GL_VERTEX_SHADER, textured.vertex_source.c_str(), textured_vertex_shader, optional_error) &&
            link_program(textured.vertex_source.c_str(), textured_vertex_shader, textured.fragment_source.c_str(), textured_program_, optional_error)) {
            GlUInt skinned_textured_vertex_shader = 0;
            if (compile_shader(GL_VERTEX_SHADER, skinned_textured.vertex_source.c_str(), skinned_textured_vertex_shader, optional_error) &&
                link_program(skinned_textured.vertex_source.c_str(), skinned_textured_vertex_shader, skinned_textured.fragment_source.c_str(), skinned_textured_program_, optional_error)) {
                t_base_ = api_.GetUniformLocation(textured_program_, "u_base_texture");
                t_rough_ = api_.GetUniformLocation(textured_program_, "u_roughness_texture");
                t_metal_ = api_.GetUniformLocation(textured_program_, "u_metallic_texture");
                t_normal_ = api_.GetUniformLocation(textured_program_, "u_normal_texture");
                t_ao_ = api_.GetUniformLocation(textured_program_, "u_ao_texture");
                t_emission_ = api_.GetUniformLocation(textured_program_, "u_emission_texture");
                t_opacity_ = api_.GetUniformLocation(textured_program_, "u_opacity_texture");
                t_use_ = api_.GetUniformLocation(textured_program_, "u_use_textures");
                st_base_ = api_.GetUniformLocation(skinned_textured_program_, "u_base_texture");
                st_rough_ = api_.GetUniformLocation(skinned_textured_program_, "u_roughness_texture");
                st_metal_ = api_.GetUniformLocation(skinned_textured_program_, "u_metallic_texture");
                st_normal_ = api_.GetUniformLocation(skinned_textured_program_, "u_normal_texture");
                st_ao_ = api_.GetUniformLocation(skinned_textured_program_, "u_ao_texture");
                st_emission_ = api_.GetUniformLocation(skinned_textured_program_, "u_emission_texture");
                st_opacity_ = api_.GetUniformLocation(skinned_textured_program_, "u_opacity_texture");
                st_use_ = api_.GetUniformLocation(skinned_textured_program_, "u_use_textures");
                const bool texture_locations_valid = t_base_ >= 0 && t_rough_ >= 0 && t_metal_ >= 0 && t_normal_ >= 0 &&
                    t_ao_ >= 0 && t_emission_ >= 0 && t_opacity_ >= 0 && t_use_ >= 0 &&
                    st_base_ >= 0 && st_rough_ >= 0 && st_metal_ >= 0 && st_normal_ >= 0 && st_ao_ >= 0 &&
                    st_emission_ >= 0 && st_opacity_ >= 0 && st_use_ >= 0;
                if (!texture_locations_valid) {
                    if (textured_program_ && api_.DeleteProgram) api_.DeleteProgram(textured_program_);
                    if (skinned_textured_program_ && api_.DeleteProgram) api_.DeleteProgram(skinned_textured_program_);
                    textured_program_ = skinned_textured_program_ = 0;
                }
            }
        }
    }
    return true;
}

bool OpenGLESRenderer::ensure_mesh(const RenderDrawCall& draw, std::uint64_t frame_id,
                                   GpuCachedMesh*& output, std::string& error) {
    const std::uint64_t key = mesh_key(draw);
    auto existing = meshes_.find(key);
    if (existing != meshes_.end()) {
        if (!existing->second.valid()) {
            release_meshes();
            error = "cached OpenGL ES mesh is invalid";
            return false;
        }
        existing->second.last_used_frame = frame_id;
        output = &existing->second;
        return true;
    }

    std::vector<float> vertices;
    std::size_t index_count = 0;
    std::size_t vertex_count = 0;
    const bool animated = draw.animated();
    if (animated) {
        if (!draw.skinned_mesh || draw.skinned_mesh->vertices.empty() || draw.skinned_mesh->indices.empty()) {
            error = "invalid skinned mesh payload";
            return false;
        }
        const auto& mesh = *draw.skinned_mesh;
        vertex_count = mesh.vertices.size();
        index_count = mesh.indices.size();
        vertices.reserve(vertex_count * 16);
        for (const auto& v : mesh.vertices) {
            vertices.insert(vertices.end(), {
                v.position.x, v.position.y, v.position.z,
                v.normal.x, v.normal.y, v.normal.z,
                v.uv.x, v.uv.y,
                static_cast<float>(v.skin.bones[0]), static_cast<float>(v.skin.bones[1]),
                static_cast<float>(v.skin.bones[2]), static_cast<float>(v.skin.bones[3]),
                v.skin.weights[0], v.skin.weights[1], v.skin.weights[2], v.skin.weights[3]
            });
        }
    } else {
        if (!draw.geometry || draw.part_index >= draw.geometry->parts.size()) {
            error = "invalid mesh geometry reference";
            return false;
        }
        const auto& mesh = draw.geometry->parts[draw.part_index].mesh;
        if (!mesh.valid()) {
            error = "invalid mesh geometry";
            return false;
        }
        vertex_count = mesh.vertices.size();
        index_count = mesh.indices.size();
        vertices.reserve(vertex_count * 8);
        for (const auto& v : mesh.vertices) {
            vertices.insert(vertices.end(), {
                v.position.x, v.position.y, v.position.z,
                v.normal.x, v.normal.y, v.normal.z,
                v.uv.x, v.uv.y
            });
        }
    }

    GpuCachedMesh resource;
    resource.index_count = index_count;
    resource.vertex_count = vertex_count;
    resource.animated = animated;
    resource.last_used_frame = frame_id;
    api_.GenVertexArrays(1, &resource.vao);
    api_.GenBuffers(1, &resource.vertex_buffer);
    api_.GenBuffers(1, &resource.index_buffer);
    if (!resource.vao || !resource.vertex_buffer || !resource.index_buffer) {
        if (resource.vao) api_.DeleteVertexArrays(1, &resource.vao);
        if (resource.vertex_buffer) api_.DeleteBuffers(1, &resource.vertex_buffer);
        if (resource.index_buffer) api_.DeleteBuffers(1, &resource.index_buffer);
        error = "OpenGL ES buffer allocation failed";
        return false;
    }

    api_.BindVertexArray(resource.vao);
    api_.BindBuffer(GL_ARRAY_BUFFER, resource.vertex_buffer);
    api_.BufferData(GL_ARRAY_BUFFER, static_cast<GlSize>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);
    const auto indices = animated ? draw.skinned_mesh->indices : draw.geometry->parts[draw.part_index].mesh.indices;
    api_.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, resource.index_buffer);
    api_.BufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GlSize>(indices.size() * sizeof(GlUInt)), indices.data(), GL_STATIC_DRAW);

    const GlInt stride = static_cast<GlInt>((animated ? 16 : 8) * sizeof(float));
    api_.EnableVertexAttribArray(0);
    api_.VertexAttribPointer(0, 3, GL_FLOAT, 0, stride, nullptr);
    api_.EnableVertexAttribArray(1);
    api_.VertexAttribPointer(1, 3, GL_FLOAT, 0, stride, reinterpret_cast<const void*>(3 * sizeof(float)));
    api_.EnableVertexAttribArray(2);
    api_.VertexAttribPointer(2, 2, GL_FLOAT, 0, stride, reinterpret_cast<const void*>(6 * sizeof(float)));
    if (animated) {
        api_.EnableVertexAttribArray(3);
        api_.VertexAttribPointer(3, 4, GL_FLOAT, 0, stride, reinterpret_cast<const void*>(8 * sizeof(float)));
        api_.EnableVertexAttribArray(4);
        api_.VertexAttribPointer(4, 4, GL_FLOAT, 0, stride, reinterpret_cast<const void*>(12 * sizeof(float)));
    }

    const auto inserted = meshes_.emplace(key, resource);
    if (!inserted.second) {
        api_.DeleteBuffers(1, &resource.vertex_buffer);
        api_.DeleteBuffers(1, &resource.index_buffer);
        api_.DeleteVertexArrays(1, &resource.vao);
        error = "OpenGL ES mesh cache insertion failed";
        return false;
    }
    output = &inserted.first->second;
    return true;
}

bool OpenGLESRenderer::ensure_texture(const std::shared_ptr<const Texture2D>& texture, std::uint64_t frame_id,
                                      GpuCachedTexture*& output, std::string& error) {
    (void)frame_id;
    if (!texture_ready() || !texture || !texture->valid()) {
        error = "OpenGL ES texture residency is unavailable";
        return false;
    }
    const auto key = texture_key(texture);
    if (auto it = textures_.find(key); it != textures_.end()) {
        if (!it->second.valid()) {
            error = "cached OpenGL ES texture is invalid";
            return false;
        }
        output = &it->second;
        return true;
    }

    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(texture->width) * texture->height * 4U, 255U);
    for (std::uint32_t y = 0; y < texture->height; ++y) {
        for (std::uint32_t x = 0; x < texture->width; ++x) {
            const auto src = texture->index(x, y, 0);
            const auto dst = (static_cast<std::size_t>(y) * texture->width + x) * 4U;
            auto byte = [&](std::uint32_t channel, std::uint8_t fallback) {
                if (channel >= texture->channels) return fallback;
                const float v = std::clamp(texture->data[src + channel], 0.0f, 1.0f);
                return static_cast<std::uint8_t>(v * 255.0f + 0.5f);
            };
            rgba[dst] = byte(0, 255);
            rgba[dst + 1] = byte(1, texture->channels == 1 ? rgba[dst] : 255);
            rgba[dst + 2] = byte(2, texture->channels == 1 ? rgba[dst] : 255);
            rgba[dst + 3] = byte(3, 255);
        }
    }

    GpuCachedTexture resource;
    resource.width = texture->width;
    resource.height = texture->height;
    resource.source_key = key;
    api_.GenTextures(1, &resource.handle);
    if (!resource.handle) {
        error = "OpenGL ES texture allocation failed";
        return false;
    }
    api_.ActiveTexture(GL_TEXTURE0);
    api_.BindTexture(GL_TEXTURE_2D, resource.handle);
    api_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    api_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    api_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    api_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    api_.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GlInt>(texture->width), static_cast<GlInt>(texture->height),
                    0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    api_.GenerateMipmap(GL_TEXTURE_2D);

    const auto inserted = textures_.emplace(key, resource);
    if (!inserted.second) {
        api_.DeleteTextures(1, &resource.handle);
        error = "OpenGL ES texture cache insertion failed";
        return false;
    }
    output = &inserted.first->second;
    return true;
}

GpuSubmitResult OpenGLESRenderer::submit(const RenderFrame& frame) {
    GpuSubmitResult output;
    if (!frame.valid()) {
        output.error = "invalid render frame";
        return output;
    }
    if (frame.config.backend != RenderBackend::OpenGLES) {
        output.error = "render frame backend is not OpenGLES";
        return output;
    }
    std::string error;
    if (!ensure_programs(error)) {
        output.error = std::move(error);
        return output;
    }
    if (frame.config.width == 0 || frame.config.height == 0) {
        output.error = "invalid framebuffer dimensions";
        return output;
    }

    api_.Viewport(0, 0, static_cast<GlInt>(frame.config.width), static_cast<GlInt>(frame.config.height));
    const auto sky = frame.lighting.environment.sky_color;
    api_.ClearColor(sky.r, sky.g, sky.b, 1.0f);
    api_.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    api_.Enable(GL_DEPTH_TEST);
    api_.DepthFunc(GL_LESS);
    api_.Enable(GL_CULL_FACE);

    std::vector<const RenderDrawCall*> draws;
    draws.reserve(frame.draws.size());
    for (const auto& draw : frame.draws) draws.push_back(&draw);
    std::stable_sort(draws.begin(), draws.end(), [&](const auto* a, const auto* b) {
        if (a->pass != b->pass) return a->pass == RenderPass::Opaque;
        if (a->pass != RenderPass::Transparent) return a->entity_id < b->entity_id;
        const auto center = [](const Bounds3& bounds) {
            return Vec3{(bounds.min.x + bounds.max.x) * 0.5f,
                        (bounds.min.y + bounds.max.y) * 0.5f,
                        (bounds.min.z + bounds.max.z) * 0.5f};
        };
        const auto ca = center(a->world_bounds);
        const auto cb = center(b->world_bounds);
        const auto da = (ca.x - frame.camera.position.x) * (ca.x - frame.camera.position.x) +
                        (ca.y - frame.camera.position.y) * (ca.y - frame.camera.position.y) +
                        (ca.z - frame.camera.position.z) * (ca.z - frame.camera.position.z);
        const auto db = (cb.x - frame.camera.position.x) * (cb.x - frame.camera.position.x) +
                        (cb.y - frame.camera.position.y) * (cb.y - frame.camera.position.y) +
                        (cb.z - frame.camera.position.z) * (cb.z - frame.camera.position.z);
        return da > db;
    });

    for (const auto* draw : draws) {
        GpuCachedMesh* mesh = nullptr;
        if (!ensure_mesh(*draw, frame.frame_id, mesh, error)) {
            output.error = std::move(error);
            return output;
        }
        api_.BindVertexArray(mesh->vao);
        const bool textured = texture_ready() && draw->material.textures && draw->material.textures->valid();
        GpuCachedTexture* base = nullptr;
        GpuCachedTexture* rough = nullptr;
        GpuCachedTexture* metal = nullptr;
        GpuCachedTexture* normal = nullptr;
        GpuCachedTexture* ao = nullptr;
        GpuCachedTexture* emission = nullptr;
        GpuCachedTexture* opacity = nullptr;
        if (textured) {
            const auto& textures = draw->material.textures;
            if (!ensure_texture(textures->base_color, frame.frame_id, base, error) ||
                !ensure_texture(textures->roughness, frame.frame_id, rough, error) ||
                !ensure_texture(textures->metallic, frame.frame_id, metal, error) ||
                !ensure_texture(textures->normal, frame.frame_id, normal, error) ||
                !ensure_texture(textures->ambient_occlusion, frame.frame_id, ao, error) ||
                !ensure_texture(textures->emission, frame.frame_id, emission, error) ||
                !ensure_texture(textures->opacity, frame.frame_id, opacity, error)) {
                output.error = std::move(error);
                return output;
            }
        }

        auto bind_textures = [&](bool skinned) {
            const GlInt base_loc = skinned ? st_base_ : t_base_;
            const GlInt rough_loc = skinned ? st_rough_ : t_rough_;
            const GlInt metal_loc = skinned ? st_metal_ : t_metal_;
            const GlInt normal_loc = skinned ? st_normal_ : t_normal_;
            const GlInt ao_loc = skinned ? st_ao_ : t_ao_;
            const GlInt emission_loc = skinned ? st_emission_ : t_emission_;
            const GlInt opacity_loc = skinned ? st_opacity_ : t_opacity_;
            const GlInt use_loc = skinned ? st_use_ : t_use_;
            GpuCachedTexture* maps[] = {base, rough, metal, normal, ao, emission, opacity};
            const GlInt locations[] = {base_loc, rough_loc, metal_loc, normal_loc, ao_loc, emission_loc, opacity_loc};
            for (int i = 0; i < 7; ++i) {
                api_.ActiveTexture(GL_TEXTURE0 + static_cast<GlEnum>(i));
                api_.BindTexture(GL_TEXTURE_2D, maps[i]->handle);
                api_.Uniform1i(locations[i], i);
            }
            api_.Uniform1f(use_loc, 1.0f);
        };

        if (draw->animated()) {
            api_.UseProgram(textured && textured_program_ ? skinned_textured_program_ : skinned_program_);
            const bool use_textured_program = textured && textured_program_ && skinned_textured_program_;
            api_.UniformMatrix4fv(s_vp_, 1, 0, frame.view_projection.m.data());
            api_.UniformMatrix4fv(s_model_, 1, 0, draw->model.m.data());
            api_.UniformMatrix4fv(s_bones_, static_cast<GlInt>(draw->bone_palette.size()), 0,
                                  reinterpret_cast<const GlFloat*>(draw->bone_palette.data()));
            api_.Uniform1i(s_bone_count_, static_cast<GlInt>(draw->bone_palette.size()));
            api_.Uniform3f(s_camera_, frame.camera.position.x, frame.camera.position.y, frame.camera.position.z);
            api_.Uniform1f(s_ambient_, std::max(0.0f, frame.lighting.environment.ambient_intensity));
            api_.Uniform3f(s_light_dir_, 0.0f, -1.0f, 0.0f);
            api_.Uniform3f(s_light_color_, 1.0f, 1.0f, 1.0f);
            api_.Uniform1f(s_light_intensity_, frame.lights.empty() ? 0.0f : std::max(0.0f, frame.lights.front().intensity));
            const auto& material = draw->material.material;
            api_.Uniform4f(s_base_, material.base_color.r, material.base_color.g, material.base_color.b,
                           std::clamp(material.roughness, 0.045f, 1.0f));
            api_.Uniform4f(s_metal_, std::clamp(material.metallic, 0.0f, 1.0f),
                           std::clamp(material.specular, 0.0f, 1.0f), 0.0f, 0.0f);
            api_.Uniform4f(s_emission_, material.emission.r, material.emission.g, material.emission.b,
                           std::clamp(material.opacity, 0.0f, 1.0f));
            if (use_textured_program) bind_textures(true);
        } else {
            api_.UseProgram(textured && textured_program_ ? textured_program_ : program_);
            const bool use_textured_program = textured && textured_program_;
            api_.UniformMatrix4fv(u_vp_, 1, 0, frame.view_projection.m.data());
            api_.UniformMatrix4fv(u_model_, 1, 0, draw->model.m.data());
            api_.Uniform3f(u_camera_, frame.camera.position.x, frame.camera.position.y, frame.camera.position.z);
            api_.Uniform1f(u_ambient_, std::max(0.0f, frame.lighting.environment.ambient_intensity));

            Vec3 light_direction{0.0f, -1.0f, 0.0f};
            Color3 light_color{1.0f, 1.0f, 1.0f};
            float light_intensity = 0.0f;
            if (!frame.lights.empty()) {
                const auto& light = frame.lights.front();
                light_direction = light.direction;
                light_color = light.color;
                light_intensity = light.intensity;
            }
            api_.Uniform3f(u_light_dir_, light_direction.x, light_direction.y, light_direction.z);
            api_.Uniform3f(u_light_color_, light_color.r, light_color.g, light_color.b);
            api_.Uniform1f(u_light_intensity_, std::max(0.0f, light_intensity));

            const auto& material = draw->material.material;
            api_.Uniform4f(u_base_, material.base_color.r, material.base_color.g, material.base_color.b,
                           std::clamp(material.roughness, 0.045f, 1.0f));
            api_.Uniform4f(u_metal_, std::clamp(material.metallic, 0.0f, 1.0f),
                           std::clamp(material.specular, 0.0f, 1.0f), 0.0f, 0.0f);
            api_.Uniform4f(u_emission_, material.emission.r, material.emission.g, material.emission.b,
                           std::clamp(material.opacity, 0.0f, 1.0f));
            if (use_textured_program) bind_textures(false);
        }

        if (draw->pass == RenderPass::Transparent) {
            api_.Enable(GL_BLEND);
            api_.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            api_.Disable(GL_BLEND);
        }

        api_.DrawElements(GL_TRIANGLES, static_cast<GlInt>(mesh->index_count), GL_UNSIGNED_INT, nullptr);
        ++output.draw_calls;
        output.triangles += mesh->index_count / 3;
        if (draw->animated()) ++output.animated_draw_calls;
    }

    api_.Disable(GL_BLEND);
    api_.Disable(GL_CULL_FACE);
    output.success = true;
    return output;
}

void OpenGLESRenderer::release_meshes() noexcept {
    for (auto& [_, mesh] : meshes_) {
        if (mesh.vertex_buffer && api_.DeleteBuffers) api_.DeleteBuffers(1, &mesh.vertex_buffer);
        if (mesh.index_buffer && api_.DeleteBuffers) api_.DeleteBuffers(1, &mesh.index_buffer);
        if (mesh.vao && api_.DeleteVertexArrays) api_.DeleteVertexArrays(1, &mesh.vao);
    }
    meshes_.clear();
}

void OpenGLESRenderer::release_textures() noexcept {
    for (auto& [_, texture] : textures_) {
        if (texture.handle && api_.DeleteTextures) api_.DeleteTextures(1, &texture.handle);
    }
    textures_.clear();
}

void OpenGLESRenderer::release() noexcept {
    release_meshes();
    release_textures();
    if (program_ && api_.DeleteProgram) api_.DeleteProgram(program_);
    if (skinned_program_ && api_.DeleteProgram) api_.DeleteProgram(skinned_program_);
    if (textured_program_ && api_.DeleteProgram) api_.DeleteProgram(textured_program_);
    if (skinned_textured_program_ && api_.DeleteProgram) api_.DeleteProgram(skinned_textured_program_);
    program_ = skinned_program_ = textured_program_ = skinned_textured_program_ = 0;
    u_vp_ = u_model_ = u_camera_ = u_ambient_ = u_base_ = u_rough_ = u_metal_ = u_emission_ =
        u_opacity_ = u_light_dir_ = u_light_color_ = u_light_intensity_ = -1;
    s_vp_ = s_model_ = s_camera_ = s_ambient_ = s_base_ = s_metal_ = s_emission_ =
        s_light_dir_ = s_light_color_ = s_light_intensity_ = s_bones_ = s_bone_count_ = -1;
    t_base_ = t_rough_ = t_metal_ = t_normal_ = t_ao_ = t_emission_ = t_opacity_ = t_use_ = -1;
    st_base_ = st_rough_ = st_metal_ = st_normal_ = st_ao_ = st_emission_ = st_opacity_ = st_use_ = -1;
}

} // namespace exgine
