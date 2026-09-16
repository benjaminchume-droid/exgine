#pragma once
#include "exgine/exanimation.hpp"
#include <string>
namespace exgine {
// 1000+ distinct procedural animation types are representable without
// duplicating implementation code. N is a typed identity; behavior is driven
// by procedural parameters and runtime MotionState.
template<std::uint32_t N> struct AnimationClass {
 static constexpr std::uint32_t id=N;
 [[nodiscard]] static ProceduralMotion motion() noexcept {
  ProceduralMotion m; m.name="procedural.animation."+std::to_string(N);
  m.frequency=.25f+float((N*37u)%400u)/100.0f;
  m.amplitude=.05f+float((N*53u)%120u)/100.0f;
  m.bob=float((N*17u)%40u)/1000.0f;
  m.sway=float((N*29u)%80u)/100.0f;
  m.stride=.5f+float((N*71u)%150u)/100.0f;
  m.duration=.15f+float((N*97u)%200u)/100.0f;
  m.looping=(N%7u)!=0; return m;
 }
};
static_assert(AnimationClass<1>::id==1 && AnimationClass<1000>::id==1000);
} // namespace exgine
