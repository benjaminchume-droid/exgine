#pragma once
#include "exgine/animation.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace exgine {

enum class MotionNodeType : std::uint8_t { Constant, Oscillator, Noise, Curve, Transform, Additive, Blend, Multiply, Spring, Damping, LookAt, Aim, IK2Bone, TimeWarp, Mirror, Retarget };

struct MotionState { float speed=0, vertical_speed=0, turn=0, crouch=0, aim=0; bool grounded=true, sprinting=false; };

struct MotionNode {
    std::uint32_t id=0;
    MotionNodeType type=MotionNodeType::Constant;
    std::vector<std::uint32_t> inputs;
    BoneId bone=invalid_bone;
    BoneId secondary_bone=invalid_bone;
    BoneId target_bone=invalid_bone;
    std::array<float,8> p{};
    std::uint32_t seed=1;
};

struct MotionGraph {
    std::string name;
    std::vector<MotionNode> nodes;
    std::uint32_t output=0;
    float duration=1.0f;
    bool looping=true;
    std::uint32_t seed=1;
    [[nodiscard]] bool valid(const Skeleton&) const noexcept;
    [[nodiscard]] MotionNode* node(std::uint32_t) noexcept;
    [[nodiscard]] const MotionNode* node(std::uint32_t) const noexcept;
    std::uint32_t add(MotionNode);
};

struct ProceduralMotion {
    std::string name;
    float frequency=1, amplitude=1, bob=0, sway=0, stride=1, duration=1;
    bool looping=true;
    MotionGraph graph;
    std::uint32_t seed=1;
};

class ExAnimation {
public:
    explicit ExAnimation(const Skeleton& skeleton) noexcept : skeleton_(&skeleton) {}
    [[nodiscard]] AnimationClip generate(const ProceduralMotion&, AnimationClipId) const;
    [[nodiscard]] AnimationClip generate(const MotionGraph&, AnimationClipId) const;
    [[nodiscard]] AnimationClip locomotion(const MotionState&, AnimationClipId) const;
    [[nodiscard]] AnimationClip action(std::string_view, AnimationClipId) const;
    void evaluate(const MotionGraph&, float time, const MotionState&, SkeletonPose&) const noexcept;
    void evaluate(const MotionState&, float time, SkeletonPose&) const noexcept;
private:
    const Skeleton* skeleton_{};
    void apply_bone(SkeletonPose&, BoneId, const AnimTransform&) const noexcept;
};

struct AnimationState { std::string name; AnimationClipId clip=0; float blend=.15f, minimum_speed=0, maximum_speed=1.0e9f; };
class AnimationStateMachine {
public:
    explicit AnimationStateMachine(const Skeleton& skeleton):controller_(skeleton){}
    void add(AnimationState);
    bool set(std::string_view,const AnimationLibrary&) noexcept;
    void update(float,const MotionState&,const AnimationLibrary&) noexcept;
    [[nodiscard]] const SkeletonPose& pose() const noexcept{return controller_.pose();}
    [[nodiscard]] const AnimationPlayer& player() const noexcept{return controller_.player();}
private:
    AnimationController controller_; std::vector<AnimationState> states_; std::string current_;
};

[[nodiscard]] MotionGraph make_motion_graph(std::string_view name,std::uint32_t seed=1);
[[nodiscard]] MotionGraph motion_catalog(std::uint32_t id,const Skeleton&);
[[nodiscard]] MotionGraph catalog_animation(std::uint32_t id,const Skeleton&);

} // namespace exgine
