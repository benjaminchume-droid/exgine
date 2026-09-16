#pragma once
#include "exgine/exsound.hpp"
namespace exgine {
template<std::uint32_t N> struct SoundClass {
 static constexpr std::uint32_t id=N;
 [[nodiscard]] static SoundRecipe recipe(){return sound_catalog(N);}
};
static_assert(SoundClass<1>::id==1 && SoundClass<1000>::id==1000);
} // namespace exgine
