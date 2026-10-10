// UnityEngine.Animation / AnimationState state model (WP-27). See ksp/unity/animation.hpp: native Unity semantics, per-operation float32.
#include "ksp/unity/animation.hpp"

namespace ksp {

AnimationState& Animation::AddClip(const std::string& name, float length, WrapMode clipWrap) {
    states_.push_back(std::make_unique<AnimationState>(name, length, clipWrap));
    return *states_.back();
}

AnimationState* Animation::Find(std::string_view name) const {
    for (const auto& s : states_)
        if (s->name == name) return s.get();
    return nullptr;
}

WrapMode Animation::Effective(const AnimationState& s) const {
    const WrapMode m = s.wrapMode != WrapMode::Default ? s.wrapMode : wrapMode;
    return m == WrapMode::Default ? WrapMode::Once : m;  // Unity: a Default clip plays as Once
}

bool Animation::Play(std::string_view name) {
    AnimationState* s = Find(name);
    if (s == nullptr) return false;
    for (const auto& o : states_)
        if (o.get() != s && o->enabled && o->layer == s->layer) {
            o->enabled = false;  // Unity: StopSameLayer rewinds the stopped state
            o->time = 0.0f;
        }
    s->enabled = true;
    return true;
}

void Animation::Stop(std::string_view name) {
    if (AnimationState* s = Find(name)) {
        s->enabled = false;
        s->time = 0.0f;
    }
}

void Animation::Stop() {
    for (const auto& s : states_) {
        s->enabled = false;
        s->time = 0.0f;
    }
}

bool Animation::IsPlaying(std::string_view name) const {
    const AnimationState* s = Find(name);
    return s != nullptr && s->enabled;
}

bool Animation::isPlaying() const {
    for (const auto& s : states_)
        if (s->enabled) return true;
    return false;
}

void Animation::Tick(float deltaTime) {
    for (const auto& sp : states_) {
        AnimationState& s = *sp;
        if (!s.enabled) continue;
        s.time = s.time + s.speed * deltaTime;
        switch (Effective(s)) {
            case WrapMode::ClampForever:
                if (s.time > s.length) s.time = s.length;
                if (s.time < 0.0f) s.time = 0.0f;
                break;
            case WrapMode::Loop:
                if (s.length > 0.0f) {
                    while (s.time >= s.length) s.time -= s.length;
                    while (s.time < 0.0f) s.time += s.length;
                }
                break;
            case WrapMode::PingPong:
                if (s.length > 0.0f) {
                    if (s.time >= s.length) { s.time = s.length - (s.time - s.length); s.speed = -s.speed; }
                    else if (s.time <= 0.0f) { s.time = -s.time; s.speed = -s.speed; }
                }
                break;
            default:  // Once
                if ((s.speed > 0.0f && s.time >= s.length) || (s.speed < 0.0f && s.time <= 0.0f)) {
                    s.enabled = false;
                    s.time = 0.0f;
                }
                break;
        }
    }
}

}  // namespace ksp
