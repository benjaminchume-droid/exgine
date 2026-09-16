#pragma once
#include "exgine/gpu.hpp"
#include "exgine/material.hpp"
#include "exgine/texture.hpp"
#include <array>
#include <cstdint>
#include <string>
namespace exgine {
struct VegetationMaterialGpu { Color3 base_color{}; float roughness=.5f; float metallic=0.f; float specular=.5f; Color3 emission{}; float opacity=1.f; float use_base=0.f,use_roughness=0.f,use_metallic=0.f,use_normal=0.f,use_ao=0.f,use_emission=0.f,use_opacity=0.f; };
struct VegetationTextureHandles { std::array<GlUInt,7> handles{}; std::array<bool,7> valid{}; };
struct VegetationPbrBinding { GlUInt material_ssbo=0; VegetationMaterialGpu material{}; VegetationTextureHandles textures{}; };
bool upload_vegetation_material(OpenGLESApi&,const Material&,const VegetationTextureHandles&,VegetationPbrBinding&,std::string&) noexcept;
bool bind_vegetation_material_textures(OpenGLESApi&,const VegetationPbrBinding&) noexcept;
void release_vegetation_material(OpenGLESApi&,VegetationPbrBinding&) noexcept;
const char* vegetation_pbr_vertex_shader() noexcept;
const char* vegetation_pbr_fragment_shader() noexcept;
}