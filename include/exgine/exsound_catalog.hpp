#pragma once
#include "exgine/exsound.hpp"
#include <string>
namespace exgine {
// 1000+ distinct procedural sound types are representable without copying DSP.
// The type identity selects a deterministic recipe; the synthesis remains data-driven.
template<std::uint32_t N> struct SoundClass {
 static constexpr std::uint32_t id=N;
 [[nodiscard]] static SoundRecipe recipe() { return SoundRecipe::catalog(N); }
};
static_assert(SoundClass<1>::id==1 && SoundClass<1000>::id==1000);
} // namespace exgine
