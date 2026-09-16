#pragma once
#include "exgine/exanimation.hpp"
namespace exgine {
template<std::uint32_t N> struct AnimationClass {
 static constexpr std::uint32_t id=N;
 [[nodiscard]] static MotionGraph graph(){return motion_catalog(N, Skeleton{});}
 [[nodiscard]] static MotionGraph graph(const Skeleton&s){return motion_catalog(N,s);}
};
static_assert(AnimationClass<1>::id==1 && AnimationClass<1000>::id==1000);
} // namespace exgine
