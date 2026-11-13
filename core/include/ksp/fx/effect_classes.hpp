// WP-UE-16a: the effect classes of the EFFECTS system (one per EffectDefinition node name). See ksp/fx/effects.hpp for the data model.
//
// Every class keeps the C# public / [Persistent] fields under the C# names and its private runtime state in `rt_` (fresh after
// Instantiate, like private non-serialized fields). Persistent fields load as ConfigNode.LoadObjectFromConfig (ReadObject: every value of
// the node whose name is a field name, in node order, later duplicates win, a failed numeric TryParse leaves the field; Enum.Parse
// throws ArgumentException on an unknown name) and save in declaration order (CreateConfigFromObject: bool "True"/"False", float "G9",
// enum name, Vector3 "x,y,z" G9).
#pragma once

#include "ksp/fx/effects.hpp"

namespace ksp {

#define KSP_FX_CLONE(Class)                                                  \
    std::unique_ptr<EffectBehaviour> Clone() const override {                \
        auto c = std::make_unique<Class>(*this);                             \
        c->rt_ = Rt{};                                                       \
        c->owner = nullptr;                                                  \
        c->subscribedToPause = false;                                        \
        return c;                                                            \
    }

// AudioFX.cs:5 "AUDIO"
class AudioFX : public EffectBehaviour {
public:
    AudioChannel channel = AudioChannel::Ship;  // :18 [Persistent]
    bool loop = false;                           // :21
    int polyphony = 1;                           // :24
    std::string clip;                            // :27
    float maxVolumeDistance = 500.0f;            // :30
    FXCurve volume{"volume", 1.0f};              // :32
    FXCurve pitch{"pitch", 1.0f};                // :34
    const char* nodeName() const override { return "AUDIO"; }
    KSP_FX_CLONE(AudioFX)
    void OnLoad(const ConfigNode& node) override;
    void OnSave(ConfigNode& node) const override;
    void OnInitialize() override;
    void OnEventPower(float power) override;
    void OnEvent() override;
    void Update(float deltaTime) override;
    void OnGamePause() override;
    void OnGameUnpause() override;
    void Describe(EffectInstanceState& s) const override;
    void SetTime(float newTime);  // :150
    // AudioFX.SetSourceVolume (:227): source.volume = GameSettings.<channel>_VOLUME * volumeSet
    static void SetSourceVolume(AudioSourceState& source, float volumeSet, AudioChannel channel);
    ~AudioFX() override = default;

private:
    struct Rt {
        float volumeSet = 0.0f;
        bool hasClip = false;
        std::string clipName;
        std::vector<AudioSourceState> sources;
        bool isPaused = false;
    } rt_;
    void Play(float power);          // :115
    AudioSourceState* CreateSource();  // :212
};

// AudioLoopFX.cs:3 "AUDIO_LOOP"
class AudioLoopFX : public EffectBehaviour {
public:
    AudioChannel channel = AudioChannel::Ship;  // :6
    std::string clip;                            // :9
    FXCurve volume{"volume", 1.0f};              // :11
    FXCurve pitch{"pitch", 1.0f};                // :13
    const char* nodeName() const override { return "AUDIO_LOOP"; }
    KSP_FX_CLONE(AudioLoopFX)
    void OnLoad(const ConfigNode& node) override;
    void OnSave(ConfigNode& node) const override;
    void OnInitialize() override;
    void OnEventPower(float power) override;
    void OnEvent() override;
    void Update(float deltaTime) override;
    void OnGamePause() override;
    void OnGameUnpause() override;
    void Describe(EffectInstanceState& s) const override;

private:
    struct Rt {
        float volumeSet = 0.0f;
        bool hasClip = false;
        std::optional<AudioSourceState> src;
        bool isPaused = false;
    } rt_;
    void Play(float power);  // :76
};

// AudioSingleFX.cs:4 "AUDIO_SINGLE"
class AudioSingleFX : public EffectBehaviour {
public:
    AudioChannel channel = AudioChannel::Ship;  // :7
    int polyphony = 1;                           // :10
    std::string clip;                            // :13
    FXCurve volume{"volume", 1.0f};              // :15
    FXCurve pitch{"pitch", 1.0f};                // :17
    const char* nodeName() const override { return "AUDIO_SINGLE"; }
    KSP_FX_CLONE(AudioSingleFX)
    void OnLoad(const ConfigNode& node) override;
    void OnSave(ConfigNode& node) const override;
    void OnInitialize() override;
    void OnEventPower(float power) override;
    void OnEvent() override;
    void Update(float deltaTime) override;
    void OnGamePause() override;
    void OnGameUnpause() override;
    void Describe(EffectInstanceState& s) const override;

private:
    struct Rt {
        float volumeSet = 0.0f;
        bool hasClip = false;
        std::vector<AudioSourceState> sources;
        bool isPaused = false;
    } rt_;
    void Play(float power);  // :109
};

// AudioMultiFX.cs:5 "AUDIO_MULTI" (internal class in C#)
class AudioMultiFX : public EffectBehaviour {
public:
    AudioChannel channel = AudioChannel::Ship;  // :8
    std::string clip;                            // :11
    FXCurve volume{"volume", 1.0f};              // :13
    FXCurve pitch{"pitch", 1.0f};                // :15
    std::string transformName;                   // :18
    const char* nodeName() const override { return "AUDIO_MULTI"; }
    KSP_FX_CLONE(AudioMultiFX)
    void OnLoad(const ConfigNode& node) override;
    void OnSave(ConfigNode& node) const override;
    void OnInitialize() override;
    void OnEventPower(float power, int transformIdx) override;
    void OnEvent(int transformIdx) override;
    void Update(float deltaTime) override;
    void OnGamePause() override;
    void OnGameUnpause() override;
    void Describe(EffectInstanceState& s) const override;

private:
    struct Rt {
        bool hasClip = false;
        std::optional<AudioSourceState> source;
        std::vector<float> volumes;
        bool hasVolumes = false;
        int modelParentCount = 0;
        bool isPaused = false;
    } rt_;
    void Play(float power, int transformIdx);  // :109
};

// AudioMultiPooledFX.cs:8 "AUDIO_MULTI_POOL" (internal class in C#). The static pool (Smooth.Pools.Pool: Size = high-water mark of the free
// stack, Allocated = created count) hands out PooledAudioSource objects; maxAudioSource = 64.
class AudioMultiPooledFX : public EffectBehaviour {
public:
    AudioChannel channel = AudioChannel::Ship;  // :114
    std::string clip;                            // :117
    FXCurve volume{"volume", 1.0f};              // :119
    FXCurve pitch{"pitch", 1.0f};                // :121
    std::string transformName;                   // :124
    const char* nodeName() const override { return "AUDIO_MULTI_POOL"; }
    std::unique_ptr<EffectBehaviour> Clone() const override;
    AudioMultiPooledFX() = default;
    AudioMultiPooledFX(const AudioMultiPooledFX& o);
    ~AudioMultiPooledFX() override;
    void OnLoad(const ConfigNode& node) override;
    void OnSave(ConfigNode& node) const override;
    void OnInitialize() override;
    void OnEventPower(float power, int transformIdx) override;
    void OnEvent(int transformIdx) override;
    void Update(float deltaTime) override;
    void OnGamePause() override;
    void OnGameUnpause() override;
    void Describe(EffectInstanceState& s) const override;

    // the static Pool<PooledAudioSource> (tests reset it)
    void OnPooledSourceReleased();  // the onRelease closure (source = null)
    static void ResetPool();
    static int PoolAllocated();
    static int PoolSize();
    static inline int maxAudioSource = 64;  // :14

private:
    struct Rt {
        bool hasClip = false;
        int source = -1;  // index into the pool arena (-1 = null)
        std::vector<float> volumes;
        bool hasVolumes = false;
        std::vector<int> modelParents;  // transform indices (FindModelTransforms order)
        bool isPaused = false;
    } rt_;
    void SetupAudioClip();                      // :189
    void Play(float power, int transformIdx);   // :231
    std::string clipName_;
};

// ModelParticleFX.cs:4 "MODEL_PARTICLE" and ModelMultiParticleFX.cs:5 "MODEL_MULTI_PARTICLE" share the fields; the single variant has one emitter.
class ModelParticleFXBase : public EffectBehaviour {
public:
    std::string modelName;                   // [Persistent]
    std::string transformName;
    Vector3f localRotation = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f localPosition = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f localScale = Vector3f(1.0f, 1.0f, 1.0f);
    FXCurve emission{"emission", 1.0f};
    FXCurve energy{"energy", 1.0f};
    FXCurve speed{"speed", 1.0f};
    void OnLoad(const ConfigNode& node) override;
    void OnSave(ConfigNode& node) const override;
    void Describe(EffectInstanceState& s) const override;

protected:
    struct Base {
        bool known = false;
        float minEmission = 0.0f, maxEmission = 0.0f, minEnergy = 0.0f, maxEnergy = 0.0f;
        Vector3f localVelocity{};
    };
    struct RtBase {
        bool initialized = false;  // the emitters exist
        Base base;
        std::vector<EmitterState> emitters;
        float emissionPower = 0.0f, energyPower = 0.0f, speedPower = 0.0f;
    };
    // ModelMultiParticleFX.SetEmitter (:181) / ModelParticleFX.OnEvent(float) (:105): the powered write of one emitter
    void SetEmitter(RtBase& r, size_t index, int minEmissionVal, int maxEmissionVal, float minEnergyVal, float maxEnergyVal, const Vector3f& localVelocityVal) const;
    void PoweredEvent(RtBase& r, float power, int transformIdx) const;
    bool InitEmitters(RtBase& r, size_t count) const;
};

class ModelParticleFX : public ModelParticleFXBase {
public:
    const char* nodeName() const override { return "MODEL_PARTICLE"; }
    KSP_FX_CLONE(ModelParticleFX)
    void OnInitialize() override;
    void OnEvent() override;
    void OnEventPower(float power) override;
    void Describe(EffectInstanceState& s) const override;

private:
    using Rt = RtBase;
    Rt rt_;
};

class ModelMultiParticleFX : public ModelParticleFXBase {
public:
    const char* nodeName() const override { return "MODEL_MULTI_PARTICLE"; }
    KSP_FX_CLONE(ModelMultiParticleFX)
    void OnInitialize() override;
    void OnEvent(int transformIdx) override;
    void OnEventPower(float power, int transformIdx) override;
    void Describe(EffectInstanceState& s) const override;

private:
    using Rt = RtBase;
    Rt rt_;
};

// PrefabParticleFX.cs:4 "PREFAB_PARTICLE" and PrefabMultiParticleFX.cs:5 "PREFAB_MULTI_PARTICLE"
class PrefabParticleFXBase : public EffectBehaviour {
public:
    std::string prefabName;                  // [Persistent]
    std::string transformName;
    FXCurve emission{"emission", 1.0f};
    FXCurve energy{"energy", 1.0f};
    FXCurve speed{"speed", 1.0f};
    Vector3f localOffset = Vector3f(0.0f, 0.0f, 0.0f);
    Vector4f localRotation = Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
    Vector3f localScale = Vector3f(1.0f, 1.0f, 1.0f);
    bool oneShot = false;
    void OnLoad(const ConfigNode& node) override;
    void OnSave(ConfigNode& node) const override;
    void Describe(EffectInstanceState& s) const override;

protected:
    struct RtBase {
        bool initialized = false;
        bool baseKnown = false;
        bool hasBursts = false;
        float minEmission = 0.0f, maxEmission = 0.0f, minEnergy = 0.0f, maxEnergy = 0.0f;
        Vector3f velocity{};
        std::vector<EmitterState> emitters;
        float emissionPower = 0.0f, energyPower = 0.0f, speedPower = 0.0f;
    };
    bool InitPrefab(RtBase& r) const;  // the shared tail of OnInitialize (prefab lookup, base values)
};

class PrefabParticleFX : public PrefabParticleFXBase {
public:
    const char* nodeName() const override { return "PREFAB_PARTICLE"; }
    KSP_FX_CLONE(PrefabParticleFX)
    void OnInitialize() override;
    void OnEvent() override;
    void OnEventPower(float power) override;
    void Describe(EffectInstanceState& s) const override;

private:
    using Rt = RtBase;
    Rt rt_;
};

class PrefabMultiParticleFX : public PrefabParticleFXBase {
public:
    const char* nodeName() const override { return "PREFAB_MULTI_PARTICLE"; }
    KSP_FX_CLONE(PrefabMultiParticleFX)
    void OnInitialize() override;
    void OnEvent(int transformIdx) override;
    void OnEventPower(float power, int transformIdx) override;
    void Describe(EffectInstanceState& s) const override;

private:
    using Rt = RtBase;
    Rt rt_;
    void SetEmitter(size_t index, int minEmissionVal, int maxEmissionVal, float minEnergyVal, float maxEnergyVal, const Vector3f& v);  // :200
};

// The animation effects: AnimationFX.cs:4 "ANIMATION", ModelAnimationFX.cs:4 "MODEL_ANIMATION", PrefabAnimationFX.cs:4 "PREFAB_ANIMATION"
class AnimationEffectBase : public EffectBehaviour {
public:
    std::string clipName;                    // [Persistent]
    std::string transformName;
    FXAnimationType animationType = FXAnimationType::HoldValue;
    void Describe(EffectInstanceState& s) const override;

protected:
    struct RtBase {
        bool hasAnim = false;       // `anim != null`
        bool hasState = false;      // `animState != null`
        float normalizedTime = 0.0f;
        bool playing = false;
    };
    void Fire(RtBase& r, bool hasPower, float power) const;  // the shared switch of OnEvent() / OnEvent(float)
};

class AnimationFX : public AnimationEffectBase {
public:
    const char* nodeName() const override { return "ANIMATION"; }
    KSP_FX_CLONE(AnimationFX)
    void OnLoad(const ConfigNode& node) override;
    void OnSave(ConfigNode& node) const override;
    void OnInitialize() override;
    void OnEvent() override;
    void OnEventPower(float power) override;
    void Describe(EffectInstanceState& s) const override;

private:
    using Rt = RtBase;
    Rt rt_;
    bool FindAnimator();  // :76
};

class ModelAnimationFX : public AnimationEffectBase {
public:
    std::string modelName;       // [Persistent]
    std::string animationName;
    const char* nodeName() const override { return "MODEL_ANIMATION"; }
    KSP_FX_CLONE(ModelAnimationFX)
    void OnLoad(const ConfigNode& node) override;
    void OnSave(ConfigNode& node) const override;
    void OnInitialize() override;
    void OnEvent() override;
    void OnEventPower(float power) override;
    void Describe(EffectInstanceState& s) const override;

private:
    using Rt = RtBase;
    Rt rt_;
};

class PrefabAnimationFX : public AnimationEffectBase {
public:
    std::string prefabName;      // [Persistent]
    std::string animationName;
    const char* nodeName() const override { return "PREFAB_ANIMATION"; }
    KSP_FX_CLONE(PrefabAnimationFX)
    void OnLoad(const ConfigNode& node) override;
    void OnSave(ConfigNode& node) const override;
    void OnInitialize() override;
    void OnEvent() override;
    void OnEventPower(float power) override;
    void Describe(EffectInstanceState& s) const override;

private:
    using Rt = RtBase;
    Rt rt_;
};

// PrefabSpawnFX.cs:4 "PREFAB_SPAWN"
class PrefabSpawnFX : public EffectBehaviour {
public:
    std::string prefabName;      // :7
    std::string transformName;   // :10
    bool setParent = true;       // :13
    const char* nodeName() const override { return "PREFAB_SPAWN"; }
    KSP_FX_CLONE(PrefabSpawnFX)
    void OnLoad(const ConfigNode& node) override;
    void OnSave(ConfigNode& node) const override;
    void OnInitialize() override;
    void OnEvent() override;
    void OnEventPower(float power) override;
    void Describe(EffectInstanceState& s) const override;

private:
    struct Rt {
        bool hasParent = false;
        bool hasPrefab = false;
        int spawned = 0;
    } rt_;
    void Spawn();
};

// ParticleConfigFX.cs:5 "PARTICLE_CONFIG": builds its own ParticleSystem from the node. The persistent scalar / vector / curve fields load and
// the per-event values of UpdateComponents are evaluated; the colour animation, the material and the nested-object persistence are not ported
// (no stock part uses this class).
class ParticleConfigFX : public EffectBehaviour {
public:
    float power = 0.0f;                                  // :56
    bool oneShot = false;                                // :101
    FXCurve minSize{"minSize", 0.1f}, maxSize{"maxSize", 0.1f};          // :64 :66
    FXCurve minEnergy{"minEnergy", 2.0f}, maxEnergy{"maxEnergy", 3.0f};  // :68 :70
    FXCurve minEmission{"minEmission", 80.0f}, maxEmission{"maxEmission", 100.0f};  // :72 :74
    FXCurve sizeGrow{"sizeGrow", 0.0f};                  // :124 (never loaded by OnLoad)
    FXCurve lengthScale{"lengthScale", 0.0f};            // :140 (never loaded by OnLoad)
    std::string transform;                               // OnLoad: node.GetValue("transform")
    const char* nodeName() const override { return "PARTICLE_CONFIG"; }
    KSP_FX_CLONE(ParticleConfigFX)
    void OnLoad(const ConfigNode& node) override;
    void OnInitialize() override;
    void OnEvent() override;
    void OnEventPower(float power) override;
    void Describe(EffectInstanceState& s) const override;

private:
    struct Rt {
        bool initialized = false;
        float startSizeMin = 0.0f, startSizeMax = 0.0f, startLifetimeMin = 0.0f, startLifetimeMax = 0.0f, sizeOverLifetimeEnd = 0.0f, lengthScaleValue = 0.0f;
    } rt_;
    void UpdateComponents();  // :250
};

// ROCsSFX.cs:4 "ROCSAUDIO": created and driven by the ROC code (no cfg fields, no Event overrides): registered so the node name loads.
class ROCsSFX : public EffectBehaviour {
public:
    bool loop = false;  // :14 [Persistent]
    const char* nodeName() const override { return "ROCSAUDIO"; }
    std::unique_ptr<EffectBehaviour> Clone() const override {
        auto c = std::make_unique<ROCsSFX>(*this);
        c->owner = nullptr;
        return c;
    }
    // no OnLoad / OnSave override in the C#: the [Persistent] loop field is never read from a cfg
};

namespace fx_detail {
std::string ChangeExtensionNull(const std::string& path);  // Path.ChangeExtension(path, null)
bool ClipExists(const std::string& clip);
float ClipLength(const std::string& clip);
bool PartIsEVA(Part* part);
bool LoadedSceneIsNotLoading();  // HighLogic.LoadedScene != GameScenes.LOADING
int TransformCount(Part* part, const std::string& name);
bool TransformActive(Part* part, const std::string& name, int index);
void PlaySource(AudioSourceState& s);
void StopSource(AudioSourceState& s);
}  // namespace fx_detail

}  // namespace ksp
