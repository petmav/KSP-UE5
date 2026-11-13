// WP-UE-16a: AnimationFX.cs, ModelAnimationFX.cs, PrefabAnimationFX.cs, PrefabSpawnFX.cs, ParticleConfigFX.cs (Animation.normalizedTime / Play /
// Sample and the spawn / particle-config writes as data).
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/fx/effect_classes.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"
#include "effect_persist.hpp"

namespace ksp {

namespace {
bool HasTransform(Part* part, const std::string& name) {
    if (fx_detail::TransformCount(part, name) > 0) return true;
    return EffectEnvironment::findModelTransform && part != nullptr && EffectEnvironment::findModelTransform(*part, name);
}
}  // namespace

void AnimationEffectBase::Describe(EffectInstanceState& s) const {
    s.strings = {{"clipName", clipName}, {"transformName", transformName}};
    s.floats = {{"animationType", static_cast<float>(animationType)}};
}

// The switch shared by OnEvent() (power 0) and OnEvent(float): Play -> animState.normalizedTime = power; anim.Play(clipName);
// HoldValue -> animState.normalizedTime = power; anim.Sample()
void AnimationEffectBase::Fire(RtBase& r, bool /*hasPower*/, float power) const {
    switch (animationType) {
        case FXAnimationType::Play:
            r.normalizedTime = power;
            r.playing = true;
            Emit("animPlay", -1, -1, true, power);
            break;
        case FXAnimationType::HoldValue:
            r.normalizedTime = power;
            Emit("animSample", -1, -1, true, power);
            break;
    }
}

// ---- ANIMATION -------------------------------------------------------------------------------------------------------------

// AnimationFX.cs:25
void AnimationFX::OnLoad(const ConfigNode& node) {
    persist::Read(node, "clipName", clipName);
    persist::Read(node, "transformName", transformName);
    persist::ReadEnum(node, "animationType", animationType, persist::kAnimationTypeNames);
}
// AnimationFX.cs:30 (`CreateConfigFromObject(this, node).CopyTo(node)`: the node is copied onto itself, so every value is written twice)
void AnimationFX::OnSave(ConfigNode& node) const {
    persist::Write(node, "clipName", clipName);
    persist::Write(node, "transformName", transformName);
    persist::WriteEnum(node, "animationType", animationType, persist::kAnimationTypeNames);
    persist::SelfCopy(node);
}
// AnimationFX.cs:35 / :76
void AnimationFX::OnInitialize() { FindAnimator(); }
bool AnimationFX::FindAnimator() {
    rt_.hasAnim = EffectEnvironment::findModelAnimator && hostPart != nullptr && EffectEnvironment::findModelAnimator(*hostPart, transformName, clipName);
    if (!rt_.hasAnim) return false;
    rt_.hasState = true;
    return true;
}
// AnimationFX.cs:40
void AnimationFX::OnEvent() {
    if (!rt_.hasAnim) return;
    Fire(rt_, false, 0.0f);
}
// AnimationFX.cs:58
void AnimationFX::OnEventPower(float power) {
    if (rt_.hasAnim || FindAnimator()) Fire(rt_, true, power);
}
void AnimationFX::Describe(EffectInstanceState& s) const {
    AnimationEffectBase::Describe(s);
    s.floats.emplace_back("found", rt_.hasAnim ? 1.0f : 0.0f);
    s.floats.emplace_back("normalizedTime", rt_.normalizedTime);
    s.floats.emplace_back("playing", rt_.playing ? 1.0f : 0.0f);
}

// ---- MODEL_ANIMATION / PREFAB_ANIMATION ------------------------------------------------------------------------------------

// ModelAnimationFX.cs:35
void ModelAnimationFX::OnLoad(const ConfigNode& node) {
    persist::Read(node, "modelName", modelName);
    persist::Read(node, "transformName", transformName);
    persist::Read(node, "animationName", animationName);
    persist::Read(node, "clipName", clipName);
    persist::ReadEnum(node, "animationType", animationType, persist::kAnimationTypeNames);
}
// ModelAnimationFX.cs:40
void ModelAnimationFX::OnSave(ConfigNode& node) const {
    persist::Write(node, "modelName", modelName);
    persist::Write(node, "transformName", transformName);
    persist::Write(node, "animationName", animationName);
    persist::Write(node, "clipName", clipName);
    persist::WriteEnum(node, "animationType", animationType, persist::kAnimationTypeNames);
    persist::SelfCopy(node);
}
// ModelAnimationFX.cs:45
void ModelAnimationFX::OnInitialize() {
    if (!HasTransform(hostPart, transformName)) {
        log::Error("ModelAnimationFX: Cannot find transform of name '" + transformName + "'");
        return;
    }
    ModelEmitterInfo info;
    if (EffectEnvironment::modelEmitter) info = EffectEnvironment::modelEmitter(modelName);
    if (!info.modelFound) {
        log::Error("ModelAnimationFX: Cannot find model of name '" + modelName + "'");
        return;
    }
    const bool found = EffectEnvironment::findEffectAnimation ? EffectEnvironment::findEffectAnimation(modelName, animationName, clipName) : true;
    if (!found) {
        log::Error("ModelAnimationFX: Cannot find animation component on transform of name '" + animationName + "'");
        return;
    }
    rt_.hasAnim = true;
    rt_.hasState = true;
}
// ModelAnimationFX.cs:77 / :92 (no null guard: a missing animState is a NullReferenceException in the C#)
void ModelAnimationFX::OnEvent() {
    if (!rt_.hasState) throw NullReferenceException("ModelAnimationFX.OnEvent: animState is null");
    Fire(rt_, false, 0.0f);
}
void ModelAnimationFX::OnEventPower(float power) {
    if (!rt_.hasState) throw NullReferenceException("ModelAnimationFX.OnEvent: animState is null");
    Fire(rt_, true, power);
}
void ModelAnimationFX::Describe(EffectInstanceState& s) const {
    AnimationEffectBase::Describe(s);
    s.strings.emplace_back("modelName", modelName);
    s.strings.emplace_back("animationName", animationName);
    s.floats.emplace_back("found", rt_.hasState ? 1.0f : 0.0f);
    s.floats.emplace_back("normalizedTime", rt_.normalizedTime);
    s.floats.emplace_back("playing", rt_.playing ? 1.0f : 0.0f);
}

// PrefabAnimationFX.cs:35
void PrefabAnimationFX::OnLoad(const ConfigNode& node) {
    persist::Read(node, "prefabName", prefabName);
    persist::Read(node, "transformName", transformName);
    persist::Read(node, "animationName", animationName);
    persist::Read(node, "clipName", clipName);
    persist::ReadEnum(node, "animationType", animationType, persist::kAnimationTypeNames);
}
// PrefabAnimationFX.cs:40
void PrefabAnimationFX::OnSave(ConfigNode& node) const {
    persist::Write(node, "prefabName", prefabName);
    persist::Write(node, "transformName", transformName);
    persist::Write(node, "animationName", animationName);
    persist::Write(node, "clipName", clipName);
    persist::WriteEnum(node, "animationType", animationType, persist::kAnimationTypeNames);
    persist::SelfCopy(node);
}
// PrefabAnimationFX.cs:45
void PrefabAnimationFX::OnInitialize() {
    if (!HasTransform(hostPart, transformName)) {
        log::Error("PrefabAnimationFX: Cannot find transform of name '" + transformName + "'");
        return;
    }
    if (EffectEnvironment::prefabExists && !EffectEnvironment::prefabExists(prefabName)) {
        log::Error("PrefabAnimationFX: Cannot find model of name '" + prefabName + "'");
        return;
    }
    const bool found = EffectEnvironment::findEffectAnimation ? EffectEnvironment::findEffectAnimation(prefabName, animationName, clipName) : true;
    if (!found) {
        log::Error("PrefabAnimationFX: Cannot find animation component on transform of name '" + animationName + "'");
        return;
    }
    rt_.hasAnim = true;
    rt_.hasState = true;
}
// PrefabAnimationFX.cs:77 / :92
void PrefabAnimationFX::OnEvent() {
    if (!rt_.hasState) throw NullReferenceException("PrefabAnimationFX.OnEvent: animState is null");
    Fire(rt_, false, 0.0f);
}
void PrefabAnimationFX::OnEventPower(float power) {
    if (!rt_.hasState) throw NullReferenceException("PrefabAnimationFX.OnEvent: animState is null");
    Fire(rt_, true, power);
}
void PrefabAnimationFX::Describe(EffectInstanceState& s) const {
    AnimationEffectBase::Describe(s);
    s.strings.emplace_back("prefabName", prefabName);
    s.strings.emplace_back("animationName", animationName);
    s.floats.emplace_back("found", rt_.hasState ? 1.0f : 0.0f);
    s.floats.emplace_back("normalizedTime", rt_.normalizedTime);
    s.floats.emplace_back("playing", rt_.playing ? 1.0f : 0.0f);
}

// ---- PREFAB_SPAWN ----------------------------------------------------------------------------------------------------------

// PrefabSpawnFX.cs:19
void PrefabSpawnFX::OnLoad(const ConfigNode& node) {
    persist::Read(node, "prefabName", prefabName);
    persist::Read(node, "transformName", transformName);
    persist::Read(node, "setParent", setParent);
}
// PrefabSpawnFX.cs:24
void PrefabSpawnFX::OnSave(ConfigNode& node) const {
    persist::Write(node, "prefabName", prefabName);
    persist::Write(node, "transformName", transformName);
    persist::Write(node, "setParent", setParent);
}
// PrefabSpawnFX.cs:29
void PrefabSpawnFX::OnInitialize() {
    rt_.hasParent = HasTransform(hostPart, transformName);
    if (!rt_.hasParent) {
        log::Error("ParticleModelFX: Cannot find transform of name '" + transformName + "'");
        return;
    }
    rt_.hasPrefab = EffectEnvironment::prefabExists ? EffectEnvironment::prefabExists(prefabName) : true;
    if (!rt_.hasPrefab) log::Error("PrefabSpawnFX: Cannot find prefab of name '" + prefabName + "'");
}
void PrefabSpawnFX::Spawn() {
    if (rt_.hasParent && rt_.hasPrefab) {
        rt_.spawned++;  // Instantiate(prefabObject); NestToParent(modelParent); parent = null when !setParent
        Emit("spawn", -1, -1, false, 0.0f);
    }
}
// PrefabSpawnFX.cs:44 / :57
void PrefabSpawnFX::OnEvent() { Spawn(); }
void PrefabSpawnFX::OnEventPower(float /*power*/) { Spawn(); }
void PrefabSpawnFX::Describe(EffectInstanceState& s) const {
    s.strings = {{"prefabName", prefabName}, {"transformName", transformName}};
    s.floats = {{"setParent", setParent ? 1.0f : 0.0f}, {"spawned", static_cast<float>(rt_.spawned)}};
}

// ---- PARTICLE_CONFIG -------------------------------------------------------------------------------------------------------

// ParticleConfigFX.cs:274 (LoadObjectFromConfig covers [Persistent] fields; only power and oneShot are modelled here; sizeGrow / lengthScale are not loaded)
void ParticleConfigFX::OnLoad(const ConfigNode& node) {
    persist::Read(node, "power", power);
    persist::Read(node, "oneShot", oneShot);
    if (auto v = node.GetValue("transform")) transform = *v;
    minSize.Load("minSize", node);
    maxSize.Load("maxSize", node);
    minEnergy.Load("minEnergy", node);
    maxEnergy.Load("maxEnergy", node);
    minEmission.Load("minEmission", node);
    maxEmission.Load("maxEmission", node);
}
// ParticleConfigFX.cs:307 (InitializeComponents :167: power = 0; ps created; UpdateComponents)
void ParticleConfigFX::OnInitialize() {
    power = 0.0f;
    rt_.initialized = true;
    UpdateComponents();
}
// ParticleConfigFX.cs:254
void ParticleConfigFX::UpdateComponents() {
    rt_.startSizeMin = minSize.Value(power);
    rt_.startSizeMax = maxSize.Value(power);
    rt_.startLifetimeMin = minEnergy.Value(power);
    rt_.startLifetimeMax = maxEnergy.Value(power);
    rt_.sizeOverLifetimeEnd = f32store((fx(1.0f) + fx(sizeGrow.Value(power))) * fx(maxEnergy.Value(power)));
    rt_.lengthScaleValue = lengthScale.Value(power);
}
// ParticleConfigFX.cs:312. Quirk kept: `power = Mathf.Clamp01(power)` assigns the PARAMETER, so UpdateComponents still reads the field.
void ParticleConfigFX::OnEventPower(float p) {
    if (!oneShot) {
        (void)Mathf::Clamp01(p);
        UpdateComponents();
    }
}
// ParticleConfigFX.cs:321 (ps.Emit(Mathf.FloorToInt(Random.Range(minEmission, maxEmission))): the FXCurves convert to their lastValue)
void ParticleConfigFX::OnEvent() {
    if (oneShot) {
        power = 1.0f;
        UpdateComponents();
        const int count = Mathf::FloorToInt(UnityRandom::Global().Range(minEmission.implicitFloat(), maxEmission.implicitFloat()));
        Emit("emit", -1, 0, true, static_cast<float>(count));
    }
}
void ParticleConfigFX::Describe(EffectInstanceState& s) const {
    s.strings = {{"transform", transform}};
    s.floats = {{"power", power},
                {"oneShot", oneShot ? 1.0f : 0.0f},
                {"initialized", rt_.initialized ? 1.0f : 0.0f},
                {"startSizeMin", rt_.startSizeMin},
                {"startSizeMax", rt_.startSizeMax},
                {"startLifetimeMin", rt_.startLifetimeMin},
                {"startLifetimeMax", rt_.startLifetimeMax},
                {"sizeOverLifetimeEnd", rt_.sizeOverLifetimeEnd},
                {"lengthScale", rt_.lengthScaleValue}};
}

}  // namespace ksp
