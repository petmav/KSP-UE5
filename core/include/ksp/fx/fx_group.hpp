// WP-UE-16a: FXGroup.cs, the legacy per-part effect group (the particle systems / lights / sound a part cfg's `fx_*` and `sound_*` lines put
// into a named group; the old ModuleEngines / ModuleRCS / decouplers / parachutes drive groups by name).
//
// The Unity objects the group drives (ParticleSystem, Light, AudioSource) are plain state structs here: FXParticleSystem keeps the
// initial `main.startSize` / `main.startLifetime` constants (what FXGroup.begin reads) and the values SetPower writes back; FXLight the
// intensity; the AudioSource is an AudioSourceState (possibly shared with the part, `audio` is a pointer). The volume fade coroutine
// (setActiveWithAudioFade) runs through StepFade(): called once when the coroutine starts and then by Update() every frame.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/fx/effects.hpp"

namespace ksp {

struct FXParticleSystem {
    bool playing = false;
    // main.startSize / main.startLifetime as MinMaxCurve constants (constantMin / constantMax); `mode` is TwoConstants after SetPower
    float startSizeMin = 0.0f, startSizeMax = 0.0f, startLifetimeMin = 0.0f, startLifetimeMax = 0.0f;
    bool twoConstants = false;
    int playCount = 0;  // Play() calls (Burst / setActive)
};
struct FXLight {
    bool enabled = true;
    float intensity = 1.0f;
};

// The published state of a group
struct FXGroupState {
    std::string name;
    bool valid = false, active = false, activeLatch = false, clampVisualPower = false;
    float power = 0.0f, powerVariation = 0.0f, minVisualPower = 0.0f, maxVisualPower = 1.0f;
    std::vector<FXParticleSystem> emitters;
    std::vector<FXLight> lights;
    std::string sfx;
    bool hasAudio = false;
    AudioSourceState audio;
    int oneShots = 0;  // AudioSource.PlayOneShot(sfx) calls (Burst)
};

// Seams of the legacy group construction (PartLoader.cs:826 `fx_*` lines instantiate Resources "Effects/<name>" prefabs)
struct FXGroupEnvironment {
    // the ParticleSystems / Lights of the prefab `Effects/<fxName>` (unset: one ParticleSystem with zero constants, no light)
    static inline std::function<std::vector<FXParticleSystem>(const std::string& fxName)> fxPrefabParticleSystems;
    static inline std::function<std::vector<FXLight>(const std::string& fxName)> fxPrefabLights;
    // UnityEngine.Random.Range(min, max) of `audio.time = Random.Range(0f, audio.clip.length)` (unset: the global UnityRandom)
    static inline std::function<float(float, float)> randomRange;
};

// FXGroup.cs:8
class FXGroup {
public:
    std::vector<FXParticleSystem> fxEmittersNewSystem;  // :10
    std::vector<FXLight> lights;                        // :12
    std::string sfx;                                    // :14 (AudioClip name, empty = null)
    AudioSourceState* audio = nullptr;                  // :16 (null = no AudioSource)
    std::string name;                                   // :34
    bool clampVisualPower = false;                      // :44
    float powerVariation = 0.0f;                        // :54
    bool activeLatch = false;                           // :56
    EffectList* eventSink = nullptr;                    // the part's EffectList (Burst records an event there)
    static inline float unscaledTime = 0.0f;            // Time.unscaledTime (the host sets it per frame)

    explicit FXGroup(const std::string& groupID);       // :91
    bool isValid() const { return valid_; }             // :49
    float minVisualPower() const { return minVisualPower_; }  // :60
    void minVisualPower(float v) { minVisualPower_ = v; clampVisualPower = true; }
    float maxVisualPower() const { return maxVisualPower_; }  // :70
    void maxVisualPower(float v) { maxVisualPower_ = v; clampVisualPower = true; }
    bool Active() const { return active_; }             // :77
    float Power() const { return power_; }              // :82
    void Power(float v) { SetPower(v); }

    void begin(AudioSourceState* audioRef);             // :98
    void setActive(bool value);                         // :133
    void setActiveWithAudioFade(bool value, float audioFadeLength);  // :173
    void Burst();                                       // :240
    void SetVisualMinMax(float min, float max);         // :255
    void SetLatch(bool latch);                          // :262
    void SetPower(float pwr);                           // :271
    void SetPowerLatch(float pwr);                      // :310
    void Unlatch();                                     // :354
    void Update();                                      // the coroutine's per-frame step (VolumeFade)
    FXGroupState Snapshot() const;
    int oneShots() const { return oneShots_; }

private:
    bool valid_ = false;
    float minVisualPower_ = 0.0f;
    float maxVisualPower_ = 1.0f;
    bool active_ = false;
    float power_ = 0.0f;
    std::vector<float> initSizeValuesNewSystem_, initLifeValuesNewSystem_, initLightValues_, initSizeValuesNewSystemVariation_, initLifeValuesNewSystemVariation_;
    int oneShots_ = 0;
    struct Fade {
        bool running = false;
        float startTime = 0.0f, startVolume = 0.0f, endVolume = 0.0f, fadeLength = 0.0f;
    } fade_;
    void StartFade(float endVolume, float fadeLength);  // VolumeFade (:222)
    void StepFade();
};

}  // namespace ksp
