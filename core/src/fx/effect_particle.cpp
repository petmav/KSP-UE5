// WP-UE-16a: ModelParticleFX.cs, ModelMultiParticleFX.cs, PrefabParticleFX.cs, PrefabMultiParticleFX.cs (the emitter writes as EmitterState;
// the particle simulation itself is not ported).
#include <algorithm>
#include <cstdint>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/fx/effect_classes.hpp"
#include "ksp/unity/mathf.hpp"
#include "effect_persist.hpp"

namespace ksp {

namespace {
// Mathf.FloorToInt(base * power): the product is a float argument (Mono: double product rounded at the call)
int FloorProduct(float a, float b) { return Mathf::FloorToInt(f32store(fx(a) * fx(b))); }
float Product(float a, float b) { return f32store(fx(a) * fx(b)); }
Vector3f Scale(const Vector3f& v, float d) { return Vector3f(Product(v.x, d), Product(v.y, d), Product(v.z, d)); }  // Vector3 operator*(Vector3, float)

void DescribeEmitters(const std::vector<EmitterState>& e, EffectInstanceState& s) { s.emitters = e; }
}  // namespace

// ---- MODEL_PARTICLE / MODEL_MULTI_PARTICLE ---------------------------------------------------------------------------------

// ModelParticleFX.cs:47 / ModelMultiParticleFX.cs:50
void ModelParticleFXBase::OnLoad(const ConfigNode& node) {
    persist::Read(node, "modelName", modelName);
    persist::Read(node, "transformName", transformName);
    persist::Read(node, "localRotation", localRotation);
    persist::Read(node, "localPosition", localPosition);
    persist::Read(node, "localScale", localScale);
    emission.Load("emission", node);
    energy.Load("energy", node);
    speed.Load("speed", node);
}
// ModelParticleFX.cs:55 / ModelMultiParticleFX.cs:58
void ModelParticleFXBase::OnSave(ConfigNode& node) const {
    persist::Write(node, "modelName", modelName);
    persist::Write(node, "transformName", transformName);
    persist::Write(node, "localRotation", localRotation);
    persist::Write(node, "localPosition", localPosition);
    persist::Write(node, "localScale", localScale);
    emission.Save(node);
    energy.Save(node);
    speed.Save(node);
}
void ModelParticleFXBase::Describe(EffectInstanceState& s) const {
    s.strings = {{"modelName", modelName}, {"transformName", transformName}};
    s.floats = {{"localRotation.x", localRotation.x}, {"localRotation.y", localRotation.y}, {"localRotation.z", localRotation.z},
                {"localPosition.x", localPosition.x}, {"localPosition.y", localPosition.y}, {"localPosition.z", localPosition.z},
                {"localScale.x", localScale.x},       {"localScale.y", localScale.y},       {"localScale.z", localScale.z}};
}

// The tail of OnInitialize shared by both classes (GameDatabase.GetModel, the KSPParticleEmitter and its base values)
bool ModelParticleFXBase::InitEmitters(RtBase& r, size_t count) const {
    ModelEmitterInfo info;
    if (EffectEnvironment::modelEmitter) info = EffectEnvironment::modelEmitter(modelName);
    if (!info.modelFound) {
        log::Error("ParticleModelFX: Cannot find model of name '" + modelName + "'");
        return false;
    }
    if (!info.hasEmitter) {
        log::Error("ParticleModelFX: Cannot find particle emitter on model of name '" + modelName + "'");
        return false;
    }
    r.base.known = info.baseKnown;
    r.base.minEmission = info.base.minEmission;
    r.base.maxEmission = info.base.maxEmission;
    r.base.minEnergy = info.base.minEnergy;
    r.base.maxEnergy = info.base.maxEnergy;
    r.base.localVelocity = info.base.localVelocity;
    for (size_t i = 0; i < count; i++) {
        EmitterState e;
        e.index = static_cast<int>(r.emitters.size());
        r.emitters.push_back(e);
    }
    r.initialized = true;
    return true;
}

// ModelMultiParticleFX.cs:181 SetEmitter
void ModelParticleFXBase::SetEmitter(RtBase& r, size_t index, int minEmissionVal, int maxEmissionVal, float minEnergyVal, float maxEnergyVal,
                                     const Vector3f& localVelocityVal) const {
    EmitterState& e = r.emitters.at(index);
    e.emit = true;
    e.minEmission = minEmissionVal;
    e.maxEmission = maxEmissionVal;
    e.minEnergy = minEnergyVal;
    e.maxEnergy = maxEnergyVal;
    e.velocity = localVelocityVal;
    e.resolved = r.base.known;
    e.emissionPower = r.emissionPower;
    e.energyPower = r.energyPower;
    e.speedPower = r.speedPower;
}

// ModelMultiParticleFX.cs:141 (ModelParticleFX.cs:105 is the one-emitter case: the same values)
void ModelParticleFXBase::PoweredEvent(RtBase& r, float power, int transformIdx) const {
    if (!r.initialized) return;  // emitters == null / emitter == null
    if (power <= 0.0f) {
        if (transformIdx > -1 && transformIdx < static_cast<int>(r.emitters.size())) {
            r.emitters[static_cast<size_t>(transformIdx)].emit = false;
            return;
        }
        for (auto& e : r.emitters) e.emit = false;
        return;
    }
    r.emissionPower = emission.Value(power);
    r.energyPower = energy.Value(power);
    r.speedPower = speed.Value(power);
    int minEmissionVal = 0, maxEmissionVal = 0;
    float minEnergyVal = 0.0f, maxEnergyVal = 0.0f;
    Vector3f localVelocityVal{};
    if (r.base.known) {
        minEmissionVal = FloorProduct(r.base.minEmission, r.emissionPower);
        maxEmissionVal = FloorProduct(r.base.maxEmission, r.emissionPower);
        minEnergyVal = Product(r.base.minEnergy, r.energyPower);
        maxEnergyVal = Product(r.base.maxEnergy, r.energyPower);
        localVelocityVal = Scale(r.base.localVelocity, r.speedPower);
    }
    if (transformIdx > -1 && transformIdx < static_cast<int>(r.emitters.size())) {
        SetEmitter(r, static_cast<size_t>(transformIdx), minEmissionVal, maxEmissionVal, minEnergyVal, maxEnergyVal, localVelocityVal);
        return;
    }
    for (size_t j = 0; j < r.emitters.size(); j++) SetEmitter(r, j, minEmissionVal, maxEmissionVal, minEnergyVal, maxEnergyVal, localVelocityVal);
}

// ModelParticleFX.cs:63
void ModelParticleFX::OnInitialize() {
    if (!(fx_detail::TransformCount(hostPart, transformName) > 0 || (EffectEnvironment::findModelTransform && hostPart && EffectEnvironment::findModelTransform(*hostPart, transformName)))) {
        log::Error("ParticleModelFX: Cannot find transform of name '" + transformName + "'");
        return;
    }
    rt_.emitters.clear();
    InitEmitters(rt_, 1);
}
// ModelParticleFX.cs:97 (emitter.Emit())
void ModelParticleFX::OnEvent() {
    if (!rt_.initialized) return;
    Emit("emit", -1, 0, false, 0.0f);
}
// ModelParticleFX.cs:105
void ModelParticleFX::OnEventPower(float power) { PoweredEvent(rt_, power, -1); }
void ModelParticleFX::Describe(EffectInstanceState& s) const {
    ModelParticleFXBase::Describe(s);
    s.floats.emplace_back("initialized", rt_.initialized ? 1.0f : 0.0f);
    s.floats.emplace_back("emissionPower", rt_.emissionPower);
    s.floats.emplace_back("energyPower", rt_.energyPower);
    s.floats.emplace_back("speedPower", rt_.speedPower);
    DescribeEmitters(rt_.emitters, s);
}

// ModelMultiParticleFX.cs:66 (emitters are added to the list, never cleared: a second OnInitialize appends)
void ModelMultiParticleFX::OnInitialize() {
    const int n = fx_detail::TransformCount(hostPart, transformName);
    if (n == 0) {
        log::Error("ParticleModelFX: Cannot find transform of name '" + transformName + "'");
        return;
    }
    InitEmitters(rt_, static_cast<size_t>(n));
}
// ModelMultiParticleFX.cs:123
void ModelMultiParticleFX::OnEvent(int transformIdx) {
    if (!rt_.initialized) return;
    if (transformIdx > -1 && transformIdx < static_cast<int>(rt_.emitters.size())) {
        Emit("emit", transformIdx, transformIdx, false, 0.0f);
        return;
    }
    for (int i = 0; i < static_cast<int>(rt_.emitters.size()); i++) Emit("emit", transformIdx, i, false, 0.0f);
}
// ModelMultiParticleFX.cs:141
void ModelMultiParticleFX::OnEventPower(float power, int transformIdx) { PoweredEvent(rt_, power, transformIdx); }
void ModelMultiParticleFX::Describe(EffectInstanceState& s) const {
    ModelParticleFXBase::Describe(s);
    s.floats.emplace_back("initialized", rt_.initialized ? 1.0f : 0.0f);
    s.floats.emplace_back("emissionPower", rt_.emissionPower);
    s.floats.emplace_back("energyPower", rt_.energyPower);
    s.floats.emplace_back("speedPower", rt_.speedPower);
    DescribeEmitters(rt_.emitters, s);
}

// ---- PREFAB_PARTICLE / PREFAB_MULTI_PARTICLE -------------------------------------------------------------------------------

// PrefabParticleFX.cs:50 / PrefabMultiParticleFX.cs:53
void PrefabParticleFXBase::OnLoad(const ConfigNode& node) {
    persist::Read(node, "prefabName", prefabName);
    persist::Read(node, "transformName", transformName);
    persist::Read(node, "localOffset", localOffset);
    persist::Read(node, "localRotation", localRotation);
    persist::Read(node, "localScale", localScale);
    persist::Read(node, "oneShot", oneShot);
    emission.Load("emission", node);
    energy.Load("energy", node);
    speed.Load("speed", node);
}
// PrefabParticleFX.cs:58 / PrefabMultiParticleFX.cs:61
void PrefabParticleFXBase::OnSave(ConfigNode& node) const {
    persist::Write(node, "prefabName", prefabName);
    persist::Write(node, "transformName", transformName);
    persist::Write(node, "localOffset", localOffset);
    persist::Write(node, "localRotation", localRotation);
    persist::Write(node, "localScale", localScale);
    persist::Write(node, "oneShot", oneShot);
    emission.Save(node);
    energy.Save(node);
    speed.Save(node);
}
void PrefabParticleFXBase::Describe(EffectInstanceState& s) const {
    s.strings = {{"prefabName", prefabName}, {"transformName", transformName}};
    s.floats = {{"oneShot", oneShot ? 1.0f : 0.0f},       {"localOffset.x", localOffset.x},     {"localOffset.y", localOffset.y},     {"localOffset.z", localOffset.z},
                {"localRotation.x", localRotation.x},     {"localRotation.y", localRotation.y}, {"localRotation.z", localRotation.z}, {"localRotation.w", localRotation.w},
                {"localScale.x", localScale.x},           {"localScale.y", localScale.y},       {"localScale.z", localScale.z}};
}

// The prefab half of OnInitialize (:78 onwards): Resources.Load, GetComponentInChildren<ParticleSystem>, the base values. Quirk kept: with
// bursts the C# reads `new ParticleSystem.Burst[burstCount][0].minCount` = a fresh default struct, so minEmission = maxEmission = 0.
bool PrefabParticleFXBase::InitPrefab(RtBase& r) const {
    PrefabParticleInfo info;
    if (EffectEnvironment::prefabParticle) info = EffectEnvironment::prefabParticle(prefabName);
    if (!info.found) {
        log::Error("PrefabParticleFX: Cannot find prefab of name '" + prefabName + "'");
        return false;
    }
    if (!info.hasParticleSystem) {
        log::Error("PrefabParticleFX: Cannot find particle emitter on model of name '" + prefabName + "'");
        return false;
    }
    r.baseKnown = info.baseKnown;
    r.hasBursts = info.burstCount > 0;
    if (r.hasBursts) {
        r.minEmission = 0.0f;
        r.maxEmission = 0.0f;
    } else {
        r.minEmission = info.rateOverTime;
        r.maxEmission = info.rateOverTime;
    }
    r.minEnergy = info.startLifetimeMin;
    r.maxEnergy = info.startLifetimeMax;
    r.velocity = info.velocityOverLifetime;
    r.initialized = true;
    return true;
}

namespace {
void DescribePrefab(const PrefabParticleFXBase& self, bool initialized, float ep, float enp, float sp, const std::vector<EmitterState>& emitters, EffectInstanceState& s) {
    (void)self;
    s.floats.emplace_back("initialized", initialized ? 1.0f : 0.0f);
    s.floats.emplace_back("emissionPower", ep);
    s.floats.emplace_back("energyPower", enp);
    s.floats.emplace_back("speedPower", sp);
    s.emitters = emitters;
}
}  // namespace

// PrefabParticleFX.cs:66
void PrefabParticleFX::OnInitialize() {
    if (!(fx_detail::TransformCount(hostPart, transformName) > 0 || (EffectEnvironment::findModelTransform && hostPart && EffectEnvironment::findModelTransform(*hostPart, transformName)))) {
        log::Error("PrefabParticleFX: Cannot find transform of name '" + transformName + "'");
        return;
    }
    if (EffectEnvironment::prefabExists && !EffectEnvironment::prefabExists(prefabName)) {
        log::Error("PrefabParticleFX: Cannot find prefab of name '" + prefabName + "'");
        return;
    }
    if (!InitPrefab(rt_)) return;
    EmitterState e;
    e.index = 0;
    e.playing = false;  // emitter.Stop()
    e.burstMode = rt_.hasBursts;
    rt_.emitters.assign(1, e);
}
// PrefabParticleFX.cs:120 (emitter.Play())
void PrefabParticleFX::OnEvent() {
    if (!rt_.initialized) return;
    rt_.emitters[0].playing = true;
    Emit("play", -1, 0, false, 0.0f);
}
// PrefabParticleFX.cs:128. Quirk kept: `ParticleSystem.MinMaxCurve x = velocityOverLifetime.x; ... x.constant = velocity.x * speedPower;` writes the
// local struct copies and never assigns them back, so the speed curve changes nothing: the velocity stays the prefab's.
void PrefabParticleFX::OnEventPower(float power) {
    if (!rt_.initialized) return;
    EmitterState& e = rt_.emitters[0];
    if (power <= 0.0f) {
        e.playing = false;  // emitter.Stop()
        return;
    }
    rt_.emissionPower = emission.Value(power);
    rt_.energyPower = energy.Value(power);
    rt_.speedPower = speed.Value(power);
    e.burstMode = rt_.hasBursts;
    e.resolved = rt_.baseKnown;
    e.emissionPower = rt_.emissionPower;
    e.energyPower = rt_.energyPower;
    e.speedPower = rt_.speedPower;
    if (rt_.baseKnown) {
        if (rt_.hasBursts) {
            e.minEmission = static_cast<int16_t>(FloorProduct(rt_.minEmission, rt_.emissionPower));  // (short)
            e.maxEmission = static_cast<int16_t>(FloorProduct(rt_.maxEmission, rt_.emissionPower));
        } else {
            e.minEmission = FloorProduct(rt_.minEmission, rt_.emissionPower);  // rateOverTime
            e.maxEmission = e.minEmission;
        }
        e.minEnergy = Product(rt_.minEnergy, rt_.energyPower);
        e.maxEnergy = Product(rt_.maxEnergy, rt_.energyPower);
        e.velocity = rt_.velocity;
    }
    e.playing = true;  // emitter.Play()
}
void PrefabParticleFX::Describe(EffectInstanceState& s) const {
    PrefabParticleFXBase::Describe(s);
    DescribePrefab(*this, rt_.initialized, rt_.emissionPower, rt_.energyPower, rt_.speedPower, rt_.emitters, s);
}

// PrefabMultiParticleFX.cs:69
void PrefabMultiParticleFX::OnInitialize() {
    const int n = fx_detail::TransformCount(hostPart, transformName);
    if (n == 0) {
        log::Error("PrefabParticleFX: Cannot find transform of name '" + transformName + "'");
        return;
    }
    if (EffectEnvironment::prefabExists && !EffectEnvironment::prefabExists(prefabName)) {
        log::Error("PrefabParticleFX: Cannot find prefab of name '" + prefabName + "'");
        return;
    }
    if (!InitPrefab(rt_)) return;
    for (int i = 0; i < n; i++) {
        EmitterState e;
        e.index = static_cast<int>(rt_.emitters.size());
        e.playing = false;  // emitter.Stop()
        e.burstMode = rt_.hasBursts;
        rt_.emitters.push_back(e);
    }
}
// PrefabMultiParticleFX.cs:142
void PrefabMultiParticleFX::OnEvent(int transformIdx) {
    if (!rt_.initialized) return;
    if (transformIdx > -1 && transformIdx < static_cast<int>(rt_.emitters.size())) {
        rt_.emitters[static_cast<size_t>(transformIdx)].playing = true;
        Emit("play", transformIdx, transformIdx, false, 0.0f);
        return;
    }
    for (int i = 0; i < static_cast<int>(rt_.emitters.size()); i++) {
        rt_.emitters[static_cast<size_t>(i)].playing = true;
        Emit("play", transformIdx, i, false, 0.0f);
    }
}
// PrefabMultiParticleFX.cs:160
void PrefabMultiParticleFX::OnEventPower(float power, int transformIdx) {
    if (!rt_.initialized) return;
    if (power <= 0.0f) {
        if (transformIdx > -1 && transformIdx < static_cast<int>(rt_.emitters.size())) {
            rt_.emitters[static_cast<size_t>(transformIdx)].playing = false;
            return;
        }
        for (auto& e : rt_.emitters) e.playing = false;
        return;
    }
    rt_.emissionPower = emission.Value(power);
    rt_.energyPower = energy.Value(power);
    rt_.speedPower = speed.Value(power);
    int minEmissionVal = 0, maxEmissionVal = 0;
    float minEnergyVal = 0.0f, maxEnergyVal = 0.0f;
    Vector3f localVelocityVal{};
    if (rt_.baseKnown) {
        minEmissionVal = FloorProduct(rt_.minEmission, rt_.emissionPower);
        maxEmissionVal = FloorProduct(rt_.maxEmission, rt_.emissionPower);
        minEnergyVal = Product(rt_.minEnergy, rt_.energyPower);
        maxEnergyVal = Product(rt_.maxEnergy, rt_.energyPower);
        localVelocityVal = Scale(rt_.velocity, rt_.speedPower);
    }
    if (transformIdx > -1 && transformIdx < static_cast<int>(rt_.emitters.size())) {
        SetEmitter(static_cast<size_t>(transformIdx), minEmissionVal, maxEmissionVal, minEnergyVal, maxEnergyVal, localVelocityVal);
        return;
    }
    for (size_t j = 0; j < rt_.emitters.size(); j++) SetEmitter(j, minEmissionVal, maxEmissionVal, minEnergyVal, maxEnergyVal, localVelocityVal);
}
// PrefabMultiParticleFX.cs:200 (main.duration = 1; bursts: Burst(0, (short)min, (short)max); velocityOverLifetime x / y / z assigned)
void PrefabMultiParticleFX::SetEmitter(size_t index, int minEmissionVal, int maxEmissionVal, float minEnergyVal, float maxEnergyVal, const Vector3f& v) {
    EmitterState& e = rt_.emitters.at(index);
    e.burstMode = rt_.hasBursts;
    if (rt_.hasBursts) {
        e.minEmission = static_cast<int16_t>(minEmissionVal);
        e.maxEmission = static_cast<int16_t>(maxEmissionVal);
    } else {
        e.minEmission = minEmissionVal;  // rateOverTime
        e.maxEmission = minEmissionVal;
    }
    e.minEnergy = minEnergyVal;
    e.maxEnergy = maxEnergyVal;
    e.velocity = v;
    e.resolved = rt_.baseKnown;
    e.emissionPower = rt_.emissionPower;
    e.energyPower = rt_.energyPower;
    e.speedPower = rt_.speedPower;
    e.playing = true;  // particleSystem.Play()
}
void PrefabMultiParticleFX::Describe(EffectInstanceState& s) const {
    PrefabParticleFXBase::Describe(s);
    DescribePrefab(*this, rt_.initialized, rt_.emissionPower, rt_.energyPower, rt_.speedPower, rt_.emitters, s);
}

}  // namespace ksp
