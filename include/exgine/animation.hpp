#pragma once
#include "exgine/geometry.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace exgine {

struct Quat { float x=0,y=0,z=0,w=1; static Quat identity() noexcept; };
Quat normalize(Quat) noexcept;
Quat slerp(Quat,Quat,float) noexcept;
Quat multiply(Quat,Quat) noexcept;
struct AnimTransform { Vec3 translation{}; Quat rotation{}; Vec3 scale{1,1,1}; };
AnimTransform interpolate(AnimTransform,AnimTransform,float) noexcept;

using BoneId=std::uint32_t; inline constexpr BoneId invalid_bone=0;
struct Bone { BoneId id=invalid_bone; std::string name; BoneId parent=invalid_bone; AnimTransform local{}; };
struct Skeleton {
 std::string name; std::vector<Bone> bones;
 bool valid() const noexcept;
 BoneId find_bone(std::string_view) const noexcept;
};
struct SkeletonPose { std::vector<AnimTransform> local; std::vector<AnimTransform> model; bool valid_for(const Skeleton&) const noexcept; };
SkeletonPose make_bind_pose(const Skeleton&);
void update_pose(const Skeleton&,SkeletonPose&) noexcept;

struct SkinWeight { std::array<BoneId,4> bones{}; std::array<float,4> weights{}; bool valid(const Skeleton&) const noexcept; };
struct SkinnedVertex { Vec3 position{}; Vec3 normal{}; Vec2 uv{}; SkinWeight skin{}; };
struct SkinnedMesh { std::vector<SkinnedVertex> vertices; std::vector<std::uint32_t> indices; bool valid(const Skeleton&) const noexcept; };
std::vector<Vertex> skin_mesh(const SkinnedMesh&,const SkeletonPose&) ;

using AnimationClipId=std::uint64_t;
struct AnimationKeyframe { float time=0; AnimTransform transform{}; };
struct BoneTrack { BoneId bone=invalid_bone; std::vector<AnimationKeyframe> keys; };
struct AnimationClip { AnimationClipId id=0; std::string name; float duration=0; bool looping=true; std::vector<BoneTrack> tracks; bool valid(const Skeleton&) const noexcept; };
class AnimationLibrary { public: bool define(AnimationClip); bool erase(AnimationClipId) noexcept; const AnimationClip* find(AnimationClipId) const noexcept; const AnimationClip* find(std::string_view) const noexcept; void clear() noexcept; std::size_t size() const noexcept; private: std::unordered_map<AnimationClipId,AnimationClip> clips_; };

struct AnimationPlayer { AnimationClipId clip=0; AnimationClipId next_clip=0; float time=0; float playback_rate=1; float blend_duration=0.15f; float blend_time=0; bool playing=false; bool looping=true; };
class AnimationController {
 public:
  explicit AnimationController(const Skeleton&);
  bool play(AnimationClipId,float blend_seconds=0.15f) noexcept;
  void stop() noexcept;
  void update(float,const AnimationLibrary&) noexcept;
  const SkeletonPose& pose() const noexcept;
  const AnimationPlayer& player() const noexcept;
 private:
  const Skeleton* skeleton_{}; SkeletonPose pose_{}; AnimationPlayer player_{}; AnimationClipId previous_=0; float previous_time_=0;
  void sample(const AnimationClip*,float,SkeletonPose&) const noexcept;
};

Skeleton make_humanoid_skeleton();
AnimationClip make_idle_clip(const Skeleton&,AnimationClipId=1);
AnimationClip make_walk_clip(const Skeleton&,AnimationClipId=2);
AnimationClip make_run_clip(const Skeleton&,AnimationClipId=3);
AnimationClip make_jump_clip(const Skeleton&,AnimationClipId=4);

} // namespace exgine
