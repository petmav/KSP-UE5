// WP-UE-16a: FXGroup.cs (see ksp/fx/fx_group.hpp).
#include "ksp/fx/fx_group.hpp"

#include "ksp/dotnet/mono_float.hpp"
#include "ksp/fx/effect_classes.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"

namespace ksp {

// FXGroup.cs:91
FXGroup::FXGroup(const std::string& groupID) : name(groupID) {
    minVisualPower_ = 0.0f;
    maxVisualPower_ = 1.0f;
}

// FXGroup.cs:98
void FXGroup::begin(AudioSourceState* audioRef) {
    valid_ = true;
    initSizeValuesNewSystem_.clear();
    initLifeValuesNewSystem_.clear();
    initLightValues_.clear();
    initSizeValuesNewSystemVariation_.clear();
    initLifeValuesNewSystemVariation_.clear();
    for (FXLight& light : lights) {
        initLightValues_.push_back(light.intensity);
        light.enabled = false;
    }
    for (FXParticleSystem& ps : fxEmittersNewSystem) {
        initSizeValuesNewSystem_.push_back(Mathf::Max(ps.startSizeMax, ps.startSizeMin));
        initLifeValuesNewSystem_.push_back(Mathf::Max(ps.startLifetimeMax, ps.startLifetimeMin));
        initSizeValuesNewSystemVariation_.push_back(Mathf::Abs(f32store(fx(ps.startSizeMax) - fx(ps.startSizeMin))));
        initLifeValuesNewSystemVariation_.push_back(Mathf::Abs(f32store(fx(ps.startLifetimeMax) - fx(ps.startLifetimeMin))));
        ps.playing = false;  // particleSystem.Stop()
    }
    if (audio == nullptr) audio = audioRef;
    if (!sfx.empty() && audio != nullptr) audio->clip = sfx;
    active_ = false;
}

namespace {
float RandomTime(float length) {
    return FXGroupEnvironment::randomRange ? FXGroupEnvironment::randomRange(0.0f, length) : UnityRandom::Global().Range(0.0f, length);
}
void SetParticles(std::vector<FXParticleSystem>& list, bool play) {
    for (FXParticleSystem& ps : list) {
        ps.playing = play;
        if (play) ps.playCount++;
    }
}
}  // namespace

// FXGroup.cs:133
void FXGroup::setActive(bool value) {
    if (!valid_) return;
    for (FXLight& l : lights) l.enabled = value;
    SetParticles(fxEmittersNewSystem, value);
    if (!sfx.empty() && audio != nullptr) {
        audio->clip = sfx;
        if (value) {
            if (!audio->playing) {
                const float t = RandomTime(fx_detail::ClipLength(sfx));  // audio.time = Random.Range(0f, clip.length)
                fx_detail::PlaySource(*audio);
                audio->elapsed = t;
            }
        } else {
            fx_detail::StopSource(*audio);
        }
    }
    active_ = value;
}

// FXGroup.cs:173 (the coroutine's first iteration runs inside StartCoroutine, before audio.Play(): a source that is not playing yet takes the end
// volume at once)
void FXGroup::setActiveWithAudioFade(bool value, float audioFadeLength) {
    if (!valid_) return;
    for (FXLight& l : lights) l.enabled = value;
    SetParticles(fxEmittersNewSystem, value);
    if (!sfx.empty() && audio != nullptr) {
        audio->clip = sfx;
        fade_.running = false;  // fadeCoroutineHost.StopCoroutine(fadeCoroutine)
        if (value) {
            StartFade(GameSettings::SHIP_VOLUME, audioFadeLength);
            if (!audio->playing) {
                const float t = RandomTime(fx_detail::ClipLength(sfx));
                fx_detail::PlaySource(*audio);
                audio->elapsed = t;
            }
        } else {
            StartFade(0.0f, audioFadeLength);
        }
    }
    active_ = value;
}

// FXGroup.cs:222 VolumeFade: startTime = Time.unscaledTime, startVolume = audioSource.volume
void FXGroup::StartFade(float endVolume, float fadeLength) {
    fade_.running = true;
    fade_.startTime = unscaledTime;
    fade_.startVolume = audio->volume;
    fade_.endVolume = endVolume;
    fade_.fadeLength = fadeLength;
    StepFade();
}

void FXGroup::StepFade() {
    if (!fade_.running || audio == nullptr) return;
    if (audio->playing && !(static_cast<double>(unscaledTime) >= fx(fade_.startTime) + fx(fade_.fadeLength))) {
        float num = f32store((fx(fade_.startTime) + fx(fade_.fadeLength) - fx(unscaledTime)) / fx(fade_.fadeLength));
        num = f32store(fx(num) * fx(num));
        audio->volume = f32store(fx(num) * fx(fade_.startVolume) + fx(fade_.endVolume) * (fx(1.0f) - fx(num)));
        return;  // yield return null
    }
    audio->volume = fade_.endVolume;
    if (fade_.endVolume == 0.0f) fx_detail::StopSource(*audio);
    fade_.running = false;
}

void FXGroup::Update() { StepFade(); }

// FXGroup.cs:240
void FXGroup::Burst() {
    if (!valid_) return;
    for (FXParticleSystem& ps : fxEmittersNewSystem) {
        ps.playing = true;
        ps.playCount++;
    }
    if (!sfx.empty() && audio != nullptr) {
        oneShots_++;  // audio.PlayOneShot(sfx)
        if (eventSink != nullptr) {
            EffectEvent e;
            e.kind = "fxGroupBurst";
            e.effectName = name;
            e.className = "FXGroup";
            e.instanceName = sfx;
            eventSink->RecordEvent(std::move(e));
        }
    }
}

// FXGroup.cs:255
void FXGroup::SetVisualMinMax(float min, float max) {
    minVisualPower_ = min;
    maxVisualPower_ = max;
    clampVisualPower = true;
}

// FXGroup.cs:262
void FXGroup::SetLatch(bool latch) {
    if (valid_ && latch) {
        setActive(true);
        activeLatch = true;
    }
}

// FXGroup.cs:271
void FXGroup::SetPower(float pwr) {
    if (clampVisualPower) power_ = Mathf::Lerp(minVisualPower_, maxVisualPower_, pwr);
    else power_ = pwr;
    if (!valid_) return;
    for (size_t i = 0; i < lights.size(); i++) lights[i].intensity = f32store(fx(initLightValues_.at(i)) * fx(pwr));
    for (size_t j = 0; j < fxEmittersNewSystem.size(); j++) {
        FXParticleSystem& ps = fxEmittersNewSystem[j];
        ps.startLifetimeMin = f32store((fx(initLifeValuesNewSystem_.at(j)) - fx(initLifeValuesNewSystemVariation_.at(j))) * fx(power_));
        ps.startLifetimeMax = f32store(fx(initLifeValuesNewSystem_[j]) * fx(power_));
        ps.startSizeMin = f32store((fx(initSizeValuesNewSystem_.at(j)) - fx(initSizeValuesNewSystemVariation_.at(j))) * fx(power_));
        ps.startSizeMax = f32store(fx(initSizeValuesNewSystem_[j]) * fx(power_));
        ps.twoConstants = true;
    }
    if (!sfx.empty() && audio != nullptr) audio->pitch = pwr;
}

// FXGroup.cs:310
void FXGroup::SetPowerLatch(float pwr) {
    if (!activeLatch) return;
    if (clampVisualPower) power_ = Mathf::Lerp(minVisualPower_, maxVisualPower_, pwr);
    else power_ = Mathf::Clamp01(pwr);
    if (!valid_) return;
    for (size_t i = 0; i < lights.size(); i++) lights[i].intensity = Mathf::Max(f32store(fx(initLightValues_.at(i)) * fx(pwr)), lights[i].intensity);
    for (size_t j = 0; j < fxEmittersNewSystem.size(); j++) {
        FXParticleSystem& ps = fxEmittersNewSystem[j];
        ps.startLifetimeMin = Mathf::Max(f32store(fx(initLifeValuesNewSystem_.at(j)) * fx(power_) - fx(powerVariation) * fx(0.5f)), ps.startLifetimeMin);
        ps.startLifetimeMax = Mathf::Max(f32store(fx(initLifeValuesNewSystem_[j]) * fx(power_) + fx(powerVariation) * fx(0.5f)), ps.startLifetimeMax);
        ps.startSizeMin = Mathf::Max(f32store(fx(initSizeValuesNewSystem_.at(j)) * fx(power_) - fx(powerVariation) * fx(0.5f)), ps.startSizeMin);
        ps.startSizeMax = Mathf::Max(f32store(fx(initSizeValuesNewSystem_[j]) * fx(power_) + fx(powerVariation) * fx(0.5f)), ps.startSizeMax);
        ps.twoConstants = true;
    }
    if (!sfx.empty() && audio != nullptr) audio->pitch = Mathf::Max(pwr, audio->pitch);
}

// FXGroup.cs:354
void FXGroup::Unlatch() {
    if (valid_ && activeLatch) {
        setActive(false);
        activeLatch = false;
        SetPower(power_);
    }
}

FXGroupState FXGroup::Snapshot() const {
    FXGroupState s;
    s.name = name;
    s.valid = valid_;
    s.active = active_;
    s.activeLatch = activeLatch;
    s.clampVisualPower = clampVisualPower;
    s.power = power_;
    s.powerVariation = powerVariation;
    s.minVisualPower = minVisualPower_;
    s.maxVisualPower = maxVisualPower_;
    s.emitters = fxEmittersNewSystem;
    s.lights = lights;
    s.sfx = sfx;
    s.hasAudio = audio != nullptr;
    if (audio != nullptr) s.audio = *audio;
    s.oneShots = oneShots_;
    return s;
}

}  // namespace ksp
