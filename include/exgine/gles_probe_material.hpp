#pragma once
#include "exgine/gles_production_complete.hpp"
#include <array>
#include <cstdint>
namespace exgine { struct ProbeMaterialSet { std::array<const ReflectionProbe*,4> probes{}; std::array<float,4> weights{}; std::uint32_t count=0; }; ProbeMaterialSet select_probe_material_set(const ReflectionProbeRegistry&,Vec3,std::uint32_t max_probes=4) noexcept; bool bind_probe_material_set(OpenGLESApi&,GlUInt,const ReflectionProbeRegistry&,const ProbeMaterialSet&) noexcept; const char* probe_blended_pbr_fragment_shader() noexcept; }