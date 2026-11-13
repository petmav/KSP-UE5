// WP-UE-16a: AudioFX.cs, AudioLoopFX.cs, AudioSingleFX.cs, AudioMultiFX.cs, AudioMultiPooledFX.cs (the AudioSource is AudioSourceState: the
// volume / pitch / play state the shell applies; AudioSource.isPlaying of a one-shot ends after EffectEnvironment::audioClipLength seconds of
// Update(dt), see effects.hpp).
#include <algorithm>
#include <stdexcept>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/fx/effect_classes.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/parts/part.hpp"
#include "effect_persist.hpp"

namespace ksp {

namespace {
uint32_t NextSourceId() { return EffectEnvironment::nextSourceId++; }

// the part of an AudioSource's life that Unity's mixer decides: a non-looping source stops after the clip length
void Advance(AudioSourceState& s, float dt) {
    if (!s.playing || s.loop) return;
    s.elapsed += dt;
    if (s.elapsed >= fx_detail::ClipLength(s.clip)) s.playing = false;
}

void DescribeSource(const AudioSourceState& src, EffectInstanceState& s) { s.sources.push_back(src); }
}  // namespace

// AudioFX.cs:227
void AudioFX::SetSourceVolume(AudioSourceState& source, float volumeSet, AudioChannel channel) {
    switch (channel) {
        case AudioChannel::Ship: source.volume = f32store(fx(GameSettings::SHIP_VOLUME) * fx(volumeSet)); break;
        case AudioChannel::Voice: source.volume = f32store(fx(GameSettings::VOICE_VOLUME) * fx(volumeSet)); break;
        case AudioChannel::Ambient: source.volume = f32store(fx(GameSettings::AMBIENCE_VOLUME) * fx(volumeSet)); break;
        case AudioChannel::Music: source.volume = f32store(fx(GameSettings::MUSIC_VOLUME) * fx(volumeSet)); break;
        case AudioChannel::UI: source.volume = f32store(fx(GameSettings::UI_VOLUME) * fx(volumeSet)); break;
    }
}

// ---- AudioFX ---------------------------------------------------------------------------------------------------------------

// AudioFX.cs:76
void AudioFX::OnLoad(const ConfigNode& node) {
    persist::ReadEnum(node, "channel", channel, persist::kAudioChannelNames);
    persist::Read(node, "loop", loop);
    persist::Read(node, "polyphony", polyphony);
    persist::Read(node, "clip", clip);
    persist::Read(node, "maxVolumeDistance", maxVolumeDistance);
    volume.Load("volume", node);
    pitch.Load("pitch", node);
    node.TryGetValue("maxVolumeDistance", maxVolumeDistance);
}

// AudioFX.cs:84
void AudioFX::OnSave(ConfigNode& node) const {
    persist::WriteEnum(node, "channel", channel, persist::kAudioChannelNames);
    persist::Write(node, "loop", loop);
    persist::Write(node, "polyphony", polyphony);
    persist::Write(node, "clip", clip);
    persist::Write(node, "maxVolumeDistance", maxVolumeDistance);
    volume.Save(node);
    pitch.Save(node);
}

// AudioFX.cs:91
void AudioFX::OnInitialize() {
    rt_.hasClip = fx_detail::ClipExists(clip);
    rt_.clipName = clip;
    if (!rt_.hasClip) log::Info("Cannot assign AudioClip '" + clip + "' to AudioFX");
    if (fx_detail::LoadedSceneIsNotLoading()) subscribedToPause = true;  // GameEvents.onGamePause.Add / onGameUnpause.Add
}

// AudioFX.cs:105 / :110
void AudioFX::OnEventPower(float power) { Play(power); }
void AudioFX::OnEvent() { Play(1.0f); }

// AudioFX.cs:212
AudioSourceState* AudioFX::CreateSource() {
    AudioSourceState s;
    s.id = NextSourceId();
    s.clip = rt_.clipName;
    s.volume = volume.implicitFloat();
    s.pitch = pitch.implicitFloat();
    s.loop = loop;
    s.maxDistance = maxVolumeDistance;
    rt_.sources.push_back(s);
    return &rt_.sources.back();
}

// AudioFX.cs:115
void AudioFX::Play(float power) {
    if (!rt_.hasClip) {
        if (power <= 0.0f) return;
        const std::string url = fx_detail::ChangeExtensionNull(clip);
        rt_.hasClip = fx_detail::ClipExists(url);
        if (!rt_.hasClip) return;
        rt_.clipName = url;
    }
    AudioSourceState* audioSource = nullptr;
    if (!fx_detail::PartIsEVA(hostPart)) {
        if (rt_.sources.size() >= static_cast<size_t>(std::max(polyphony, 0))) {
            if (rt_.sources.empty()) throw std::out_of_range("Index was out of range.");  // sources[0] with polyphony <= 0
            audioSource = &rt_.sources[0];
        } else {
            audioSource = CreateSource();
        }
    } else {
        audioSource = &owner->partAudioSource;  // hostPart.GetComponent<AudioSource>()
        audioSource->clip = rt_.clipName;
    }
    rt_.volumeSet = volume.Value(power);
    SetSourceVolume(*audioSource, rt_.volumeSet, channel);
    audioSource->pitch = pitch.Value(power);
    audioSource->spatialBlend = 1.0f;
    if (!rt_.isPaused && (!audioSource->playing || !loop)) {
        fx_detail::PlaySource(*audioSource);
        Emit("audioPlay", -1, static_cast<int>(audioSource->id), true, power);
    }
}

// AudioFX.cs:150
void AudioFX::SetTime(float /*newTime*/) {
    // sources[0].time = newTime (playback position: the shell's)
}

// AudioFX.cs:50
void AudioFX::Update(float deltaTime) {
    if (!rt_.isPaused)
        for (auto& s : rt_.sources) Advance(s, deltaTime);
    if (rt_.sources.empty() || rt_.isPaused) return;
    size_t count = rt_.sources.size();
    while (count-- > 0) {
        AudioSourceState& s = rt_.sources[count];
        if (!s.playing) {
            rt_.sources.erase(rt_.sources.begin() + static_cast<std::ptrdiff_t>(count));  // Object.Destroy(audioSource)
        } else {
            SetSourceVolume(s, rt_.volumeSet, channel);
        }
    }
}

// AudioFX.cs:178 (AudioSource.Pause: isPlaying is false while paused)
void AudioFX::OnGamePause() {
    rt_.isPaused = true;
    for (auto& s : rt_.sources) {
        s.paused = true;
        s.playing = false;
    }
}

// AudioFX.cs:195 (AudioSource.Play restarts the source)
void AudioFX::OnGameUnpause() {
    rt_.isPaused = false;
    for (auto& s : rt_.sources) fx_detail::PlaySource(s);
}

void AudioFX::Describe(EffectInstanceState& s) const {
    s.floats = {{"volumeSet", rt_.volumeSet}, {"isPaused", rt_.isPaused ? 1.0f : 0.0f}, {"channel", static_cast<float>(channel)},
                {"loop", loop ? 1.0f : 0.0f}, {"polyphony", static_cast<float>(polyphony)}, {"maxVolumeDistance", maxVolumeDistance}};
    s.strings = {{"clip", clip}};
    for (const auto& src : rt_.sources) DescribeSource(src, s);
}

// ---- AudioLoopFX -----------------------------------------------------------------------------------------------------------

// AudioLoopFX.cs:38
void AudioLoopFX::OnLoad(const ConfigNode& node) {
    persist::ReadEnum(node, "channel", channel, persist::kAudioChannelNames);
    persist::Read(node, "clip", clip);
    volume.Load("volume", node);
    pitch.Load("pitch", node);
}
// AudioLoopFX.cs:45
void AudioLoopFX::OnSave(ConfigNode& node) const {
    persist::WriteEnum(node, "channel", channel, persist::kAudioChannelNames);
    persist::Write(node, "clip", clip);
    volume.Save(node);
    pitch.Save(node);
}
// AudioLoopFX.cs:52
void AudioLoopFX::OnInitialize() {
    rt_.hasClip = fx_detail::ClipExists(clip);
    if (!rt_.hasClip) log::Info("Cannot assign AudioClip '" + clip + "' to AudioFX");
    if (fx_detail::LoadedSceneIsNotLoading()) subscribedToPause = true;
}
// AudioLoopFX.cs:66 / :71
void AudioLoopFX::OnEventPower(float power) { Play(power); }
void AudioLoopFX::OnEvent() { Play(1.0f); }

// AudioLoopFX.cs:76 (CreateSource :122: loop = true)
void AudioLoopFX::Play(float power) {
    if (!rt_.hasClip) return;
    if (!rt_.src) {
        AudioSourceState s;
        s.id = NextSourceId();
        s.clip = clip;
        s.volume = volume.implicitFloat();
        s.pitch = pitch.implicitFloat();
        s.loop = true;
        rt_.src = s;
    }
    rt_.volumeSet = volume.Value(power);
    AudioFX::SetSourceVolume(*rt_.src, rt_.volumeSet, channel);
    rt_.src->pitch = pitch.Value(power);
    rt_.src->spatialBlend = 1.0f;
    if (!rt_.src->playing && !rt_.isPaused) {
        fx_detail::PlaySource(*rt_.src);
        Emit("audioPlay", -1, static_cast<int>(rt_.src->id), true, power);
    }
}
// AudioLoopFX.cs:30
void AudioLoopFX::Update(float /*deltaTime*/) {
    if (!rt_.isPaused && rt_.src) AudioFX::SetSourceVolume(*rt_.src, rt_.volumeSet, channel);
}
// AudioLoopFX.cs:104
void AudioLoopFX::OnGamePause() {
    rt_.isPaused = true;
    if (rt_.src) {
        rt_.src->paused = true;
        rt_.src->playing = false;
    }
}
// AudioLoopFX.cs:113
void AudioLoopFX::OnGameUnpause() {
    rt_.isPaused = false;
    if (rt_.src && rt_.volumeSet > 0.0f) fx_detail::PlaySource(*rt_.src);
}
void AudioLoopFX::Describe(EffectInstanceState& s) const {
    s.floats = {{"volumeSet", rt_.volumeSet}, {"isPaused", rt_.isPaused ? 1.0f : 0.0f}, {"channel", static_cast<float>(channel)}};
    s.strings = {{"clip", clip}};
    if (rt_.src) DescribeSource(*rt_.src, s);
}

// ---- AudioSingleFX ---------------------------------------------------------------------------------------------------------

// AudioSingleFX.cs:71
void AudioSingleFX::OnLoad(const ConfigNode& node) {
    persist::ReadEnum(node, "channel", channel, persist::kAudioChannelNames);
    persist::Read(node, "polyphony", polyphony);
    persist::Read(node, "clip", clip);
    volume.Load("volume", node);
    pitch.Load("pitch", node);
}
// AudioSingleFX.cs:78
void AudioSingleFX::OnSave(ConfigNode& node) const {
    persist::WriteEnum(node, "channel", channel, persist::kAudioChannelNames);
    persist::Write(node, "polyphony", polyphony);
    persist::Write(node, "clip", clip);
    volume.Save(node);
    pitch.Save(node);
}
// AudioSingleFX.cs:85
void AudioSingleFX::OnInitialize() {
    rt_.hasClip = fx_detail::ClipExists(clip);
    if (!rt_.hasClip) log::Info("Cannot assign AudioClip '" + clip + "' to AudioFX");
    if (fx_detail::LoadedSceneIsNotLoading()) subscribedToPause = true;
}
// AudioSingleFX.cs:99 / :104
void AudioSingleFX::OnEventPower(float power) { Play(power); }
void AudioSingleFX::OnEvent() { Play(1.0f); }
// AudioSingleFX.cs:109 (CreateSource :166: loop = false)
void AudioSingleFX::Play(float power) {
    if (!rt_.hasClip) return;
    AudioSourceState* audioSource = nullptr;
    if (rt_.sources.size() >= static_cast<size_t>(std::max(polyphony, 0))) {
        if (rt_.sources.empty()) throw std::out_of_range("Index was out of range.");
        audioSource = &rt_.sources[0];
    } else {
        AudioSourceState s;
        s.id = NextSourceId();
        s.clip = clip;
        s.volume = volume.implicitFloat();
        s.pitch = pitch.implicitFloat();
        s.loop = false;
        rt_.sources.push_back(s);
        audioSource = &rt_.sources.back();
    }
    rt_.volumeSet = volume.Value(power);
    AudioFX::SetSourceVolume(*audioSource, rt_.volumeSet, channel);
    audioSource->pitch = pitch.Value(power);
    audioSource->spatialBlend = 1.0f;
    if (!audioSource->playing && !rt_.isPaused) {
        fx_detail::PlaySource(*audioSource);
        Emit("audioPlay", -1, static_cast<int>(audioSource->id), true, power);
    }
}
// AudioSingleFX.cs:45
void AudioSingleFX::Update(float deltaTime) {
    if (!rt_.isPaused)
        for (auto& s : rt_.sources) Advance(s, deltaTime);
    if (rt_.sources.empty() || rt_.isPaused) return;
    size_t count = rt_.sources.size();
    while (count-- > 0) {
        AudioSourceState& s = rt_.sources[count];
        if (!s.playing) rt_.sources.erase(rt_.sources.begin() + static_cast<std::ptrdiff_t>(count));
        else AudioFX::SetSourceVolume(s, rt_.volumeSet, channel);
    }
}
// AudioSingleFX.cs:146
void AudioSingleFX::OnGamePause() {
    rt_.isPaused = true;
    for (auto& s : rt_.sources) {
        s.paused = true;
        s.playing = false;
    }
}
// AudioSingleFX.cs:156
void AudioSingleFX::OnGameUnpause() {
    rt_.isPaused = false;
    for (auto& s : rt_.sources) fx_detail::PlaySource(s);
}
void AudioSingleFX::Describe(EffectInstanceState& s) const {
    s.floats = {{"volumeSet", rt_.volumeSet}, {"isPaused", rt_.isPaused ? 1.0f : 0.0f}, {"channel", static_cast<float>(channel)}, {"polyphony", static_cast<float>(polyphony)}};
    s.strings = {{"clip", clip}};
    for (const auto& src : rt_.sources) DescribeSource(src, s);
}

// ---- AudioMultiFX ----------------------------------------------------------------------------------------------------------

// AudioMultiFX.cs:50
void AudioMultiFX::OnLoad(const ConfigNode& node) {
    persist::ReadEnum(node, "channel", channel, persist::kAudioChannelNames);
    persist::Read(node, "clip", clip);
    persist::Read(node, "transformName", transformName);
    volume.Load("volume", node);
    pitch.Load("pitch", node);
}
// AudioMultiFX.cs:57
void AudioMultiFX::OnSave(ConfigNode& node) const {
    persist::WriteEnum(node, "channel", channel, persist::kAudioChannelNames);
    persist::Write(node, "clip", clip);
    persist::Write(node, "transformName", transformName);
    volume.Save(node);
    pitch.Save(node);
}
// AudioMultiFX.cs:64
void AudioMultiFX::OnInitialize() {
    rt_.hasClip = fx_detail::ClipExists(clip);
    if (!rt_.hasClip) log::Info("Cannot assign AudioClip '" + clip + "' to AudioFX");
    rt_.modelParentCount = fx_detail::TransformCount(hostPart, transformName);
    if (rt_.modelParentCount == 0) {
        log::Error("AudioFX: Cannot find transform of name '" + transformName + "'");
        rt_.hasClip = false;
        return;
    }
    rt_.volumes.assign(static_cast<size_t>(rt_.modelParentCount), 0.0f);
    rt_.hasVolumes = true;
    if (fx_detail::LoadedSceneIsNotLoading()) subscribedToPause = true;
}
// AudioMultiFX.cs:86
void AudioMultiFX::OnEventPower(float power, int transformIdx) {
    if (!rt_.hasVolumes) return;
    if (transformIdx >= 0 && transformIdx < static_cast<int>(rt_.volumes.size())) {
        Play(power, transformIdx);
        return;
    }
    for (int i = 0; i < static_cast<int>(rt_.volumes.size()); i++) Play(power, i);
}
// AudioMultiFX.cs:104
void AudioMultiFX::OnEvent(int transformIdx) { OnEventPower(1.0f, transformIdx); }
// AudioMultiFX.cs:109 (CreateSource :160: loop = false, rides on modelParents[0])
void AudioMultiFX::Play(float power, int transformIdx) {
    if (!rt_.hasClip) return;
    if (!rt_.source) {
        AudioSourceState s;
        s.id = NextSourceId();
        s.clip = clip;
        s.volume = volume.implicitFloat();
        s.pitch = pitch.implicitFloat();
        s.loop = false;
        s.transformIndex = 0;
        rt_.source = s;
    }
    rt_.volumes.at(static_cast<size_t>(transformIdx)) = volume.Value(power);
    float num = 0.0f;
    size_t i = rt_.volumes.size();
    while (i-- > 0) num = Mathf::Max(num, rt_.volumes[i]);
    AudioFX::SetSourceVolume(*rt_.source, num, channel);
    rt_.source->pitch = pitch.Value(power);
    rt_.source->spatialBlend = 1.0f;
    if (!rt_.isPaused) {
        if (!rt_.source->playing && num > 0.0f) {
            fx_detail::PlaySource(*rt_.source);
            Emit("audioPlay", transformIdx, static_cast<int>(rt_.source->id), true, power);
        } else if (rt_.source->playing && num <= 0.0f) {
            fx_detail::StopSource(*rt_.source);
            Emit("audioStop", transformIdx, static_cast<int>(rt_.source->id), true, power);
        }
    }
}
// AudioMultiFX.cs:36
void AudioMultiFX::Update(float deltaTime) {
    if (rt_.source && !rt_.isPaused) {
        Advance(*rt_.source, deltaTime);
        float num = 0.0f;
        size_t i = rt_.volumes.size();
        while (i-- > 0) num = Mathf::Max(num, rt_.volumes[i]);
        AudioFX::SetSourceVolume(*rt_.source, num, channel);
    }
}
// AudioMultiFX.cs:142
void AudioMultiFX::OnGamePause() {
    rt_.isPaused = true;
    if (rt_.source) {
        rt_.source->paused = true;
        rt_.source->playing = false;
    }
}
// AudioMultiFX.cs:151
void AudioMultiFX::OnGameUnpause() {
    rt_.isPaused = false;
    if (rt_.source) fx_detail::PlaySource(*rt_.source);
}
void AudioMultiFX::Describe(EffectInstanceState& s) const {
    s.floats = {{"isPaused", rt_.isPaused ? 1.0f : 0.0f}, {"channel", static_cast<float>(channel)}, {"transforms", static_cast<float>(rt_.modelParentCount)}};
    for (size_t i = 0; i < rt_.volumes.size(); i++) s.floats.emplace_back("volume" + std::to_string(i), rt_.volumes[i]);
    s.strings = {{"clip", clip}, {"transformName", transformName}};
    if (rt_.source) DescribeSource(*rt_.source, s);
}

// ---- AudioMultiPooledFX ----------------------------------------------------------------------------------------------------

namespace {
// AudioMultiPooledFX.PooledAudioSource (:10) and its static Pool (Smooth.Pools.Pool<T>: Borrow pops the stack or creates; Release resets, pushes,
// maxSize = max(maxSize, stack count); Size => maxSize, Allocated => created count)
struct PooledSource {
    AudioSourceState source;
    bool hostActive = false;
    bool enabled = false;
    bool parented = false;  // host.transform.parent != null
    int parentIndex = -1;
    AudioMultiPooledFX* owner = nullptr;  // the onRelease closure target
};
struct Pool {
    std::vector<std::unique_ptr<PooledSource>> arena;
    std::vector<int> freeStack;
    int allocated = 0;
    int maxSize = 0;
};
Pool& ThePool() {
    static Pool p;
    return p;
}
}  // namespace

void AudioMultiPooledFX::ResetPool() { ThePool() = Pool{}; }
int AudioMultiPooledFX::PoolAllocated() { return ThePool().allocated; }
int AudioMultiPooledFX::PoolSize() { return ThePool().maxSize; }

AudioMultiPooledFX::AudioMultiPooledFX(const AudioMultiPooledFX& o)
    : EffectBehaviour(), channel(o.channel), clip(o.clip), volume(o.volume), pitch(o.pitch), transformName(o.transformName) {
    effectName = o.effectName;
    instanceName = o.instanceName;
}
std::unique_ptr<EffectBehaviour> AudioMultiPooledFX::Clone() const {
    auto c = std::make_unique<AudioMultiPooledFX>(*this);
    return c;
}

namespace {
// PooledAudioSource.Reset (:40): onRelease(), enabled = false, clip = null, parent = null, SetActive(false); then Pool.Release pushes
void ReleaseSource(int idx) {
    Pool& p = ThePool();
    PooledSource& ps = *p.arena[static_cast<size_t>(idx)];
    if (ps.owner != nullptr) ps.owner->OnPooledSourceReleased();
    ps.owner = nullptr;
    ps.enabled = false;
    ps.source.clip.clear();
    ps.parented = false;
    ps.hostActive = false;
    p.freeStack.push_back(idx);
    p.maxSize = std::max(p.maxSize, static_cast<int>(p.freeStack.size()));
}
}  // namespace

// AudioMultiPooledFX.cs:130 OnDestroy / :141 OnDisable
AudioMultiPooledFX::~AudioMultiPooledFX() {
    if (rt_.source >= 0 && static_cast<size_t>(rt_.source) < ThePool().arena.size()) {  // (a pool reset by a test leaves nothing to release)
        PooledSource& ps = *ThePool().arena[static_cast<size_t>(rt_.source)];
        if (ps.owner == this) {
            fx_detail::StopSource(ps.source);  // StopAndRelease: source.Stop(); Pool.Release(this)
            ReleaseSource(rt_.source);
        }
    }
}

// AudioMultiPooledFX.cs:164
void AudioMultiPooledFX::OnLoad(const ConfigNode& node) {
    persist::ReadEnum(node, "channel", channel, persist::kAudioChannelNames);
    persist::Read(node, "clip", clip);
    persist::Read(node, "transformName", transformName);
    volume.Load("volume", node);
    pitch.Load("pitch", node);
    SetupAudioClip();
}
// AudioMultiPooledFX.cs:172
void AudioMultiPooledFX::OnSave(ConfigNode& node) const {
    persist::WriteEnum(node, "channel", channel, persist::kAudioChannelNames);
    persist::Write(node, "clip", clip);
    persist::Write(node, "transformName", transformName);
    volume.Save(node);
    pitch.Save(node);
}
// AudioMultiPooledFX.cs:179
void AudioMultiPooledFX::OnInitialize() {
    SetupAudioClip();
    if (fx_detail::LoadedSceneIsNotLoading()) subscribedToPause = true;
}
// AudioMultiPooledFX.cs:189
void AudioMultiPooledFX::SetupAudioClip() {
    rt_.hasClip = fx_detail::ClipExists(clip);
    clipName_ = clip;
    if (!rt_.hasClip) log::Info("Cannot assign AudioClip '" + clip + "' to AudioFX");
    const int n = fx_detail::TransformCount(hostPart, transformName);
    rt_.modelParents.clear();
    for (int i = 0; i < n; i++) rt_.modelParents.push_back(i);
    if (n == 0) {
        log::Error("AudioFX: Cannot find transform of name '" + transformName + "'");
        rt_.hasClip = false;
    } else {
        rt_.volumes.assign(static_cast<size_t>(n), 0.0f);  // volumes = new float[modelParents.Count]
        rt_.hasVolumes = true;
    }
}
// AudioMultiPooledFX.cs:208
void AudioMultiPooledFX::OnEventPower(float power, int transformIdx) {
    if (!rt_.hasVolumes) return;
    if (transformIdx >= 0 && transformIdx < static_cast<int>(rt_.volumes.size())) {
        Play(power, transformIdx);
        return;
    }
    for (int i = 0; i < static_cast<int>(rt_.volumes.size()); i++) Play(power, i);
}
// AudioMultiPooledFX.cs:226
void AudioMultiPooledFX::OnEvent(int transformIdx) { OnEventPower(1.0f, transformIdx); }

// AudioMultiPooledFX.cs:231 (BorrowAndPlay :49, PooledAudioSource.Update :75 / :81, StopAndRelease :88)
void AudioMultiPooledFX::Play(float power, int transformIdx) {
    if (!rt_.hasClip) return;
    rt_.volumes.at(static_cast<size_t>(transformIdx)) = volume.Value(power);
    float num = 0.0f;
    size_t i = rt_.volumes.size();
    while (i-- > 0) num = Mathf::Max(num, rt_.volumes[i]);
    if (rt_.isPaused) return;
    int parent = rt_.modelParents.at(0);
    for (size_t k = 0; k < rt_.modelParents.size(); k++) {
        if (fx_detail::TransformActive(hostPart, transformName, static_cast<int>(k))) {
            parent = rt_.modelParents[k];
            break;
        }
    }
    Pool& p = ThePool();
    if (rt_.source < 0 && num > 0.0f) {
        if (static_cast<int>(p.maxSize) == 0 && p.allocated > maxAudioSource) return;  // Pool.Size == 0 && Pool.Allocated > maxAudioSource: null
        int idx;
        if (!p.freeStack.empty()) {
            idx = p.freeStack.back();
            p.freeStack.pop_back();
        } else {
            p.allocated++;
            p.arena.push_back(std::make_unique<PooledSource>());
            idx = static_cast<int>(p.arena.size()) - 1;
            PooledSource& fresh = *p.arena.back();
            fresh.source.id = NextSourceId();
            fresh.source.volume = 0.0f;
            fresh.source.pitch = 0.0f;
            fresh.source.spatialBlend = 1.0f;
            fresh.source.loop = false;
        }
        PooledSource& ps = *p.arena[static_cast<size_t>(idx)];
        ps.owner = this;
        ps.parented = true;
        ps.parentIndex = parent;
        ps.hostActive = true;
        ps.enabled = true;
        ps.source.clip = clipName_;
        ps.source.pitch = pitch.Value(power);
        ps.source.transformIndex = parent;
        AudioFX::SetSourceVolume(ps.source, num, channel);
        // source.isActiveAndEnabled: the host is active, the parent transform must be active in the hierarchy too
        if (fx_detail::TransformActive(hostPart, transformName, parent)) {
            fx_detail::PlaySource(ps.source);
            Emit("audioPlay", transformIdx, static_cast<int>(ps.source.id), true, power);
        }
        rt_.source = idx;
    } else if (rt_.source >= 0 && num > 0.0f) {
        PooledSource& ps = *p.arena[static_cast<size_t>(rt_.source)];
        ps.source.pitch = pitch.Value(power);
        AudioFX::SetSourceVolume(ps.source, num, channel);
    } else if (rt_.source >= 0 && num <= 0.0f) {
        PooledSource& ps = *p.arena[static_cast<size_t>(rt_.source)];
        fx_detail::StopSource(ps.source);
        Emit("audioStop", transformIdx, static_cast<int>(ps.source.id), true, power);
        ReleaseSource(rt_.source);
    }
}

void AudioMultiPooledFX::OnPooledSourceReleased() { rt_.source = -1; }

// AudioMultiPooledFX.cs:150 Update, then the CheckEnd coroutine (:94) of the borrowed source (it releases a source that stopped playing)
void AudioMultiPooledFX::Update(float deltaTime) {
    if (rt_.source >= 0 && !rt_.isPaused) {
        PooledSource& ps = *ThePool().arena[static_cast<size_t>(rt_.source)];
        float a = 0.0f;
        size_t n = rt_.volumes.size();
        while (n-- > 0) a = Mathf::Max(a, rt_.volumes[n]);
        AudioFX::SetSourceVolume(ps.source, a, channel);  // PooledAudioSource.Update(volume, channel): source.volume = volume, then SetSourceVolume
    }
    if (rt_.source >= 0) {
        PooledSource& ps = *ThePool().arena[static_cast<size_t>(rt_.source)];
        Advance(ps.source, deltaTime);
        if (!ps.source.playing && ps.parented) ReleaseSource(rt_.source);
    }
}
// AudioMultiPooledFX.cs:274 (StopAndRelease, then isPaused = true; only when a source exists)
void AudioMultiPooledFX::OnGamePause() {
    if (rt_.source >= 0) {
        PooledSource& ps = *ThePool().arena[static_cast<size_t>(rt_.source)];
        fx_detail::StopSource(ps.source);
        ReleaseSource(rt_.source);
        rt_.isPaused = true;
    }
}
// AudioMultiPooledFX.cs:283
void AudioMultiPooledFX::OnGameUnpause() {
    if (rt_.isPaused) rt_.isPaused = false;
}
void AudioMultiPooledFX::Describe(EffectInstanceState& s) const {
    s.floats = {{"isPaused", rt_.isPaused ? 1.0f : 0.0f}, {"channel", static_cast<float>(channel)}, {"transforms", static_cast<float>(rt_.modelParents.size())}};
    for (size_t i = 0; i < rt_.volumes.size(); i++) s.floats.emplace_back("volume" + std::to_string(i), rt_.volumes[i]);
    s.strings = {{"clip", clip}, {"transformName", transformName}};
    if (rt_.source >= 0) DescribeSource(ThePool().arena[static_cast<size_t>(rt_.source)]->source, s);
}

}  // namespace ksp
