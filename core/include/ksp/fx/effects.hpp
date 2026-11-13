// WP-UE-16a: the EFFECTS system (EffectList.cs, EffectBehaviour.cs, AudioFX / AudioLoopFX / AudioSingleFX / AudioMultiFX /
// AudioMultiPooledFX.cs, ModelParticleFX / ModelMultiParticleFX / PrefabParticleFX / PrefabMultiParticleFX.cs, the animation and
// spawn effects, ParticleConfigFX.cs) evaluated as plain data.
//
// What is ported: EffectList (OnLoad from the part's EFFECTS node, Event(name[, power][, transformIdx]), Initialize, OnSave,
// EffectsStartingWith, PlayRandomEffect), the registry of effect classes by cfg node name, every effect class's [Persistent] load /
// save and FXCurve evaluation, and what each class does with the power: the Unity calls (AudioSource.Play / volume / pitch,
// KSPParticleEmitter.emit / minEmission ..., ParticleSystem.Play / Stop / bursts / startLifetime / velocityOverLifetime,
// Animation.normalizedTime ...) are replaced by writes to plain state structs ("seams with KSP's names") that Describe() publishes:
//   * Snapshot(): one EffectInstanceState per effect instance in Dictionary order (insertion order, as .NET without removals), each
//     with its parameters, the audio sources it owns and its emitters (emit / playing flags, evaluated powers and, when the
//     emitter's base values are known, the final emission / energy / velocity values);
//   * the ordered one-shot EffectEvent list (emitter.Emit(), ParticleSystem.Play of an event without power, AudioSource.Play of a
//     one-shot, animation Play / Sample, prefab spawn), drained by TakeEvents().
//
// What the Unity side decides stays a seam of EffectEnvironment (GameDatabase.GetAudioClip / GetModel, Resources.Load,
// Part.FindModelTransforms counts, the base values of a model's KSPParticleEmitter or a prefab's ParticleSystem, clip lengths).
// The particle simulation, audio playback and animation sampling are not ported.
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/fx/fx_curve.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class EffectList;

// AudioFX.AudioChannel (AudioFX.cs:7)
enum class AudioChannel : int32_t { Ship, Voice, Ambient, Music, UI };
// AnimationFX.AnimationType / ModelAnimationFX.AnimationType / PrefabAnimationFX.AnimationType (HoldValue, Play)
enum class FXAnimationType : int32_t { HoldValue, Play };

// An AudioSource as data (the fields the effect classes write)
struct AudioSourceState {
    uint32_t id = 0;
    std::string clip;          // AudioSource.clip (clip name; empty = none)
    bool loop = false;
    float volume = 1.0f;       // AudioSource.volume (the channel setting is already applied: SetSourceVolume)
    float pitch = 1.0f;
    float spatialBlend = 0.0f;
    float maxDistance = 500.0f;
    bool playing = false;      // AudioSource.isPlaying
    bool paused = false;
    int transformIndex = -1;   // the model transform the source rides on (-1 = the part)
    float elapsed = 0.0f;      // seconds since Play (one-shot end detection, see EffectEnvironment::audioClipLength)
};

// KSPParticleEmitter / ParticleSystem as data
struct EmitterState {
    int index = 0;
    bool emit = false;         // KSPParticleEmitter.emit
    bool playing = false;      // ParticleSystem.isPlaying (Play / Stop)
    bool burstMode = false;    // PREFAB: the prefab's emission module has bursts (SetBursts) instead of rateOverTime
    bool resolved = false;     // the base values were known: the fields below are final
    int minEmission = 0, maxEmission = 0;   // KSPParticleEmitter.min/maxEmission, or burst counts / rateOverTime (= minEmission)
    float minEnergy = 0.0f, maxEnergy = 0.0f;  // KSPParticleEmitter.min/maxEnergy, or startLifetime constantMin / Max
    Vector3f velocity{};       // localVelocity / velocityOverLifetime x y z
    float emissionPower = 0.0f, energyPower = 0.0f, speedPower = 0.0f;  // the FXCurve values of the last powered event
};

// A published effect instance: "effect instance -> current parameters"
struct EffectInstanceState {
    std::string effectName;    // the EFFECTS child node name (running, power, ...)
    std::string instanceName;  // `name = ` of the class node ("" = none)
    std::string className;     // the class node name (AUDIO, MODEL_MULTI_PARTICLE ...)
    std::vector<std::pair<std::string, float>> floats;
    std::vector<std::pair<std::string, std::string>> strings;
    std::vector<AudioSourceState> sources;
    std::vector<EmitterState> emitters;
};

// One-shot events of a frame, in call order
struct EffectEvent {
    std::string kind;          // "emit", "play", "stop", "audioPlay", "audioStop", "animPlay", "animSample", "spawn", "burst", ...
    std::string effectName, instanceName, className;
    int transformIdx = -1;     // the transformIdx argument of the Event call
    int emitterIndex = -1;     // -1 = all emitters
    bool hasPower = false;
    float power = 0.0f;
};

struct KSPParticleEmitterBase {  // KSPParticleEmitter values of the model prefab (read by ModelParticleFX.OnInitialize)
    float minEmission = 0.0f, maxEmission = 0.0f, minEnergy = 0.0f, maxEnergy = 0.0f;
    Vector3f localVelocity{};
};
struct ModelEmitterInfo {        // GameDatabase.GetModel(modelName) and its KSPParticleEmitter
    bool modelFound = true;
    bool hasEmitter = true;
    bool baseKnown = false;      // false: the emitters exist but the base values are not provided (only the powers are published)
    KSPParticleEmitterBase base;
};
struct PrefabParticleInfo {      // Resources.Load("Effects/" + prefabName) and its ParticleSystem
    bool found = true;
    bool hasParticleSystem = true;
    bool baseKnown = false;
    int burstCount = 0;          // emission.burstCount
    float rateOverTime = 0.0f;   // emission.rateOverTime.constant
    float startLifetimeMin = 0.0f, startLifetimeMax = 0.0f;  // main.startLifetime constantMin / constantMax
    Vector3f velocityOverLifetime{};  // velocityOverLifetime x / y / z constants
};

// The seams (unset: the default in the comment). Process-wide like the other module environments.
struct EffectEnvironment {
    static inline std::function<bool(const std::string& clip)> audioClipExists;      // GameDatabase.GetAudioClip(clip) != null (unset: true)
    static inline std::function<float(const std::string& clip)> audioClipLength;     // AudioClip.length seconds (unset: 0 = a one-shot ends at the next Update)
    static inline std::function<int(Part&, const std::string& transformName)> findModelTransforms;  // Part.FindModelTransforms(name).Count (unset: 0)
    static inline std::function<bool(Part&, const std::string& transformName, int index)> transformActiveInHierarchy;  // (unset: true)
    static inline std::function<bool(Part&, const std::string& transformName)> findModelTransform;  // Part.FindModelTransform(name) != null (unset: count > 0 via findModelTransforms, else false)
    static inline std::function<ModelEmitterInfo(const std::string& modelName)> modelEmitter;       // (unset: found, has emitter, base unknown)
    static inline std::function<PrefabParticleInfo(const std::string& prefabName)> prefabParticle;  // (unset: found, has ParticleSystem, base unknown)
    static inline std::function<bool(Part&, const std::string& transformName, const std::string& clipName)> findModelAnimator;  // Part.FindModelAnimator(...) != null with the clip (unset: false)
    static inline std::function<bool(const std::string& modelOrPrefab, const std::string& animationName, const std::string& clipName)> findEffectAnimation;  // model.transform.Find(animationName).GetComponent<Animation>()[clipName] != null (unset: true)
    static inline std::function<bool(const std::string& prefabName)> prefabExists;  // Resources.Load("Effects/" + name) != null (unset: true)
    static inline std::function<bool(Part&)> vesselIsEVA;  // HighLogic.LoadedSceneIsFlight && part.vessel != null && part.vessel.isEVA (unset: from Part::vessel)
    static inline uint32_t nextSourceId = 1;
    static void Reset();  // tests: clears every seam
};

// EffectBehaviour.cs:5
class EffectBehaviour {
public:
    Part* hostPart = nullptr;                 // :11
    std::string effectName;                   // :13 (default "")
    std::optional<std::string> instanceName = std::string();  // :15 (default ""; nullopt = C# null, which LoadEffect assigns for an unnamed node)
    EffectList* owner = nullptr;              // the EffectList that holds the behaviour (event sink, part audio source)

    virtual ~EffectBehaviour() = default;
    virtual const char* nodeName() const = 0;                       // the EffectDefinition node name
    virtual std::unique_ptr<EffectBehaviour> Clone() const = 0;    // UnityEngine.Object.Instantiate: the serialized public fields, fresh private state
    virtual void OnLoad(const ConfigNode&) {}                       // :123
    virtual void OnSave(ConfigNode&) const {}                       // :127
    virtual void OnInitialize() {}                                  // :131
    virtual void OnEvent() {}                                       // :135 OnEvent()
    virtual void OnEvent(int /*transformIdx*/) { OnEvent(); }           // :139
    virtual void OnEventPower(float /*power*/) {}                   // :144 OnEvent(float power)
    virtual void OnEventPower(float power, int /*transformIdx*/) { OnEventPower(power); }  // :148
    virtual void Update(float /*deltaTime*/) {}                     // MonoBehaviour.Update (per rendered frame)
    virtual void OnGamePause() {}
    virtual void OnGameUnpause() {}
    virtual void Describe(EffectInstanceState&) const {}

    void Emit(const std::string& kind, int transformIdx, int emitterIndex, bool hasPower, float power) const;
    EffectInstanceState Snapshot() const;
    bool subscribedToPause = false;  // OnInitialize added the GameEvents.onGamePause / onGameUnpause handlers
};

// FXGroup.cs: the legacy per-part effect group (see fx_group.hpp)
// EffectList.cs:6
class EffectList {
public:
    explicit EffectList(Part* hostPart);                                         // :28
    static void CreateEffectTypes();                                             // :76 (the 15 EffectDefinition classes of Assembly-CSharp)
    static void RegisterEffectType(const std::string& nodeName, std::function<std::unique_ptr<EffectBehaviour>()> factory);
    static std::vector<std::string> EffectTypeNames();

    std::vector<std::string> EffectsStartingWith(const std::string& effectName) const;  // :142
    void Initialize();                                                           // :155
    void OnLoad(const ConfigNode& node);                                         // :169
    void OnSave(ConfigNode& node) const;                                         // :209
    void PlayRandomEffect();                                                     // :231
    void Event(const std::string& eventName, int transformIdx);                  // :237
    void Event(const std::string& eventName, float power, int transformIdx);     // :249

    // MonoBehaviour side: Update of every behaviour, the pause events, the published state
    void Update(float deltaTime);
    void OnGamePause();
    void OnGameUnpause();
    std::vector<EffectInstanceState> Snapshot() const;
    std::vector<EffectEvent> TakeEvents();
    const std::vector<EffectEvent>& events() const { return events_; }
    void RecordEvent(EffectEvent e) { events_.push_back(std::move(e)); }
    size_t EffectCount() const;                                                  // behaviours over every effect name
    std::vector<std::string> loadErrors;  // "EffectList: Cannot find an effect type with node name of 'X'" (Debug.LogError texts)
    AudioSourceState partAudioSource;     // hostPart.GetComponent<AudioSource>() (the EVA branch of AudioFX.Play)
    Part* hostPart = nullptr;

    // the C# Dictionary<string, List<EffectBehaviour>> in insertion order
    const std::vector<std::pair<std::string, std::vector<std::unique_ptr<EffectBehaviour>>>>& entries() const { return effectList_; }
    // UnityEngine.Object.Instantiate of the part: copies every behaviour's serialized fields
    void CopyFrom(const EffectList& original);
    // ConstructEffectList (Part.Awake of an instance) only sees EffectBehaviour components: an effect name whose class nodes were all unknown has no entry
    void PruneEmpty();

private:
    std::vector<std::pair<std::string, std::vector<std::unique_ptr<EffectBehaviour>>>> effectList_;
    std::vector<EffectEvent> events_;
    std::vector<std::unique_ptr<EffectBehaviour>>* Find(const std::string& name);
    const std::vector<std::unique_ptr<EffectBehaviour>>* Find(const std::string& name) const;
    EffectBehaviour* GetEffect(const std::string& effectName, const std::optional<std::string>& instanceName, const std::string& typeName);  // :121
    void LoadEffect(const ConfigNode& node);                                     // :178
};

}  // namespace ksp
