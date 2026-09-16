#include "exgine/exanimation.hpp"
#include <algorithm>
#include <cmath>

namespace exgine {
namespace {
constexpr float pi = 3.14159265358979323846f;
float clamp01(float x) noexcept { return std::clamp(x, 0.0f, 1.0f); }
Quat pitch(float a) noexcept { const float s = std::sin(a * .5f); return normalize({s,0,0,std::cos(a*.5f)}); }
Quat yaw(float a) noexcept { const float s = std::sin(a * .5f); return normalize({0,s,0,std::cos(a*.5f)}); }
}

void ExAnimation::apply_bone(SkeletonPose& p, std::string_view name, const AnimTransform& delta) const noexcept {
    if (!skeleton_ || !p.valid_for(*skeleton_)) return;
    const BoneId id = skeleton_->find_bone(name);
    if (!id || id > p.local.size()) return;
    auto& t = p.local[id - 1];
    t.translation = {t.translation.x + delta.translation.x, t.translation.y + delta.translation.y, t.translation.z + delta.translation.z};
    t.rotation = multiply(t.rotation, delta.rotation);
    t.scale = {t.scale.x * delta.scale.x, t.scale.y * delta.scale.y, t.scale.z * delta.scale.z};
}

AnimationClip ExAnimation::generate(const ProceduralMotion& m, AnimationClipId id) const {
    AnimationClip clip; clip.id=id; clip.name=m.name; clip.duration=std::max(.01f,m.duration); clip.looping=m.looping;
    if (!skeleton_ || !skeleton_->valid()) return clip;
    const int samples = 16;
    for (const auto& b : skeleton_->bones) {
        if (b.name == "root" || b.name == "spine" || b.name == "chest" || b.name.find("leg") != std::string::npos || b.name.find("arm") != std::string::npos) {
            BoneTrack track; track.bone=b.id;
            for (int i=0;i<samples;++i) {
                const float t=clip.duration*static_cast<float>(i)/(samples-1);
                const float phase=t/clip.duration*2*pi*m.frequency;
                const float wave=std::sin(phase);
                AnimTransform x=b.local;
                if (b.name.find("leg_l")!=std::string::npos || b.name.find("arm_l")!=std::string::npos) x.rotation=pitch(wave*m.amplitude*m.stride);
                else if (b.name.find("leg_r")!=std::string::npos || b.name.find("arm_r")!=std::string::npos) x.rotation=pitch(-wave*m.amplitude*m.stride);
                else if (b.name=="root") x.translation.y += std::fabs(wave)*m.bob;
                else if (b.name=="chest") x.rotation=yaw(std::sin(phase*.5f)*m.sway);
                track.keys.push_back({t,x});
            }
            clip.tracks.push_back(std::move(track));
        }
    }
    return clip;
}

AnimationClip ExAnimation::locomotion(const MotionState& state, AnimationClipId id) const {
    ProceduralMotion m; m.name=state.sprinting ? "procedural_sprint" : (state.speed > .05f ? "procedural_walk" : "procedural_idle");
    const float speed=std::max(0.0f,state.speed); m.frequency=std::clamp(.8f+speed*.28f,.8f,3.5f); m.amplitude=state.sprinting?.7f:.42f; m.bob=state.grounded ? (state.sprinting?.06f:.025f) : .0f; m.sway=.035f; m.stride=state.sprinting?1.25f:1.0f; m.duration=std::clamp(1.2f/(m.frequency),.28f,1.5f); return generate(m,id);
}

AnimationClip ExAnimation::action(std::string_view name, AnimationClipId id) const {
    ProceduralMotion m; m.name=std::string(name); m.looping=false;
    if (name=="jump") { m.frequency=1; m.amplitude=.5f; m.bob=.15f; m.duration=.85f; }
    else if (name=="attack") { m.frequency=1.5f; m.amplitude=.9f; m.sway=.2f; m.duration=.45f; }
    else if (name=="hit") { m.frequency=2; m.amplitude=.65f; m.sway=.35f; m.duration=.32f; }
    else { m.frequency=1; m.amplitude=.25f; m.duration=.6f; }
    return generate(m,id);
}

void ExAnimation::evaluate(const MotionState& state, float time, SkeletonPose& pose) const noexcept {
    if (!skeleton_ || !pose.valid_for(*skeleton_) || !std::isfinite(time)) return;
    const float speed=std::max(0.0f,state.speed); const float stride=state.sprinting?1.25f:1.0f; const float amp=state.sprinting?.7f:.42f;
    const float phase=time*(.8f+speed*.28f)*2*pi;
    apply_bone(pose,"root",{{0,state.grounded?std::fabs(std::sin(phase))*(state.sprinting?.06f:.025f):0,0},{},{1,1,1}});
    apply_bone(pose,"upper_leg_l",{{},{pitch(std::sin(phase)*amp*stride)},{1,1,1}});
    apply_bone(pose,"lower_leg_l",{{},{pitch(-std::sin(phase)*amp*.65f*stride)},{1,1,1}});
    apply_bone(pose,"upper_leg_r",{{},{pitch(-std::sin(phase)*amp*stride)},{1,1,1}});
    apply_bone(pose,"lower_leg_r",{{},{pitch(std::sin(phase)*amp*.65f*stride)},{1,1,1}});
    apply_bone(pose,"chest",{{},{yaw(std::sin(phase*.5f)*.035f+state.turn*.08f)},{1,1,1}});
    update_pose(*skeleton_,pose);
}

void AnimationStateMachine::add(AnimationState state) { if (!state.name.empty()) states_.push_back(std::move(state)); }

bool AnimationStateMachine::set(std::string_view name, const AnimationLibrary& library) noexcept {
    for (const auto& s:states_) if (s.name==name && library.find(s.clip)) { if (!controller_.play(s.clip,s.blend)) return false; current_=s.name; return true; }
    return false;
}

void AnimationStateMachine::update(float dt, const MotionState& motion, const AnimationLibrary& library) noexcept {
    for (const auto& s:states_) if (motion.speed>=s.minimum_speed && motion.speed<s.maximum_speed) { if (current_!=s.name) set(s.name,library); break; }
    controller_.update(dt,library);
}

} // namespace exgine
