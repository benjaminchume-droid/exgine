#pragma once
#include "exgine/animation.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace exgine {

// ExAnimation is the procedural layer above Exgine's skeletal runtime. It
// generates reusable motion from gameplay state instead of requiring a clip
// for every velocity, slope, or action.
struct MotionState {
    float speed = 0.0f;
    float vertical_speed = 0.0f;
    float turn = 0.0f;
    float crouch = 0.0f;
    float aim = 0.0f;
    bool grounded = true;
    bool sprinting = false;
};

struct ProceduralMotion {
    std::string name;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float bob = 0.0f;
    float sway = 0.0f;
    float stride = 1.0f;
    float duration = 1.0f;
    bool looping = true;
};

class ExAnimation {
public:
    explicit ExAnimation(const Skeleton& skeleton) noexcept : skeleton_(&skeleton) {}

    [[nodiscard]] AnimationClip generate(const ProceduralMotion&, AnimationClipId id) const;
    [[nodiscard]] AnimationClip locomotion(const MotionState&, AnimationClipId id) const;
    [[nodiscard]] AnimationClip action(std::string_view name, AnimationClipId id) const;

    // Evaluates a pose directly from gameplay state. This is useful when a
    // game wants continuous procedural motion instead of storing a clip.
    void evaluate(const MotionState&, float time, SkeletonPose&) const noexcept;

private:
    const Skeleton* skeleton_{};
    void apply_bone(SkeletonPose&, std::string_view, const AnimTransform&) const noexcept;
};

struct AnimationState {
    std::string name;
    AnimationClipId clip = 0;
    float blend = 0.15f;
    float minimum_speed = 0.0f;
    float maximum_speed = 1.0e9f;
};

class AnimationStateMachine {
public:
    explicit AnimationStateMachine(const Skeleton& skeleton) : controller_(skeleton) {}
    void add(AnimationState state);
    bool set(std::string_view name, const AnimationLibrary& library) noexcept;
    void update(float dt, const MotionState&, const AnimationLibrary&) noexcept;
    [[nodiscard]] const SkeletonPose& pose() const noexcept { return controller_.pose(); }
    [[nodiscard]] const AnimationPlayer& player() const noexcept { return controller_.player(); }

private:
    AnimationController controller_;
    std::vector<AnimationState> states_;
    std::string current_;
};

} // namespace exgine
