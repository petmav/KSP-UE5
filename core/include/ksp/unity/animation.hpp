// UnityEngine.Animation / AnimationState as a time-driven state model (WP-27). The engine's animation system samples clips onto
// transforms; none of that is modelled. What KSP's modules read and write is the state: normalizedTime, speed, enabled, weight,
// layer, wrapMode, Play / Stop / IsPlaying / isPlaying. The host (UE shell or a test) calls Animation::Tick(deltaTime) once per
// frame, where Unity's animation update would run (after the scripts' Update, before the next FixedUpdate); `Animation.Sample`
// style transform writes are the host's.
//
// This is native Unity code: plain per-operation float32 (no Mono fx()/f32store() rule). The semantics that are NOT documented by
// KSP's own code are Unity's documented behaviour and are marked "Unity:" below; none is observed by a capture (STATUS.md WP-27 lists
// the in-game capture that would turn them into a gate).
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/unity/animation_curve.hpp"

namespace ksp {

// UnityEngine.WrapMode is defined in ksp/unity/animation_curve.hpp

// UnityEngine.AnimationState (legacy Animation component)
class AnimationState {
public:
    AnimationState(std::string stateName, float clipLength, WrapMode clipWrap = WrapMode::Default)
        : name(std::move(stateName)), length(clipLength), wrapMode(clipWrap) {}
    std::string name;
    float length = 1.0f;       // AnimationState.length (clip length in seconds)
    float time = 0.0f;         // AnimationState.time
    float speed = 1.0f;        // AnimationState.speed
    float weight = 0.0f;       // AnimationState.weight
    bool enabled = false;      // AnimationState.enabled
    int layer = 0;             // AnimationState.layer
    WrapMode wrapMode = WrapMode::Default;

    // Unity: normalizedTime = time / length (0 for a zero-length clip), setter time = value * length
    float normalizedTime() const { return length > 0.0f ? time / length : 0.0f; }
    void set_normalizedTime(float v) { time = v * length; }
    // Unity: normalizedSpeed = speed / length
    float normalizedSpeed() const { return length > 0.0f ? speed / length : 0.0f; }
    void set_normalizedSpeed(float v) { speed = v * length; }
};

// UnityEngine.Animation
class Animation {
public:
    Animation() = default;
    WrapMode wrapMode = WrapMode::Default;  // Animation.wrapMode (the default for states whose own wrapMode is Default)

    // AnimationState AddClip: the host registers the clips of the model
    AnimationState& AddClip(const std::string& name, float length, WrapMode clipWrap = WrapMode::Default);
    // Animation.GetClip(name) != null
    bool GetClip(std::string_view name) const { return Find(name) != nullptr; }
    // Animation[name]: nullptr for an unknown clip
    AnimationState* operator[](std::string_view name) { return Find(name); }
    const AnimationState* operator[](std::string_view name) const { return Find(name); }
    // Animation.Play(name): Unity: enables the state and stops the other states of the same layer (PlayMode.StopSameLayer);
    // the state's time is kept. Returns false for an unknown clip.
    bool Play(std::string_view name);
    // Animation.Stop(name) / Stop(): Unity: a stopped state is disabled and rewound to time 0
    void Stop(std::string_view name);
    void Stop();
    // Animation.IsPlaying(name) / isPlaying (a state is playing while it is enabled)
    bool IsPlaying(std::string_view name) const;
    bool isPlaying() const;
    // The animation update of one frame. Unity: every enabled state advances time += speed * dt, then the wrap mode applies:
    // ClampForever clamps to [0, length] and keeps playing; Once (and Default, whose legacy meaning is Once) stops the state when
    // time reaches length (speed > 0) or 0 (speed < 0) and rewinds it to 0; Loop wraps; PingPong reflects.
    void Tick(float deltaTime);

    std::vector<std::unique_ptr<AnimationState>>& States() { return states_; }

private:
    std::vector<std::unique_ptr<AnimationState>> states_;
    AnimationState* Find(std::string_view name) const;
    WrapMode Effective(const AnimationState& s) const;
};

}  // namespace ksp
