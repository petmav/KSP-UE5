// WP-UE-16a: EffectList.cs / EffectBehaviour.cs (see ksp/fx/effects.hpp).
#include "ksp/fx/effects.hpp"

#include <algorithm>
#include <stdexcept>

#include "ksp/core/log.hpp"
#include "ksp/fx/effect_classes.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

void EffectEnvironment::Reset() {
    audioClipExists = nullptr;
    audioClipLength = nullptr;
    findModelTransforms = nullptr;
    transformActiveInHierarchy = nullptr;
    findModelTransform = nullptr;
    modelEmitter = nullptr;
    prefabParticle = nullptr;
    prefabExists = nullptr;
    findModelAnimator = nullptr;
    findEffectAnimation = nullptr;
    vesselIsEVA = nullptr;
    nextSourceId = 1;
}

namespace fx_detail {
// Path.ChangeExtension(path, null): drops the extension (the last '.' after the last directory separator)
std::string ChangeExtensionNull(const std::string& path) {
    const size_t dot = path.find_last_of('.');
    const size_t sep = path.find_last_of("/\\:");
    if (dot == std::string::npos || (sep != std::string::npos && dot < sep)) return path;
    return path.substr(0, dot);
}
bool ClipExists(const std::string& clip) { return EffectEnvironment::audioClipExists ? EffectEnvironment::audioClipExists(clip) : true; }
float ClipLength(const std::string& clip) { return EffectEnvironment::audioClipLength ? EffectEnvironment::audioClipLength(clip) : 0.0f; }
bool PartIsEVA(Part* part) {
    if (EffectEnvironment::vesselIsEVA && part != nullptr) return EffectEnvironment::vesselIsEVA(*part);
    // HighLogic.LoadedSceneIsFlight && hostPart.vessel != null && hostPart.vessel.isEVA
    return HighLogic::LoadedSceneIsFlight && part != nullptr && part->vessel != nullptr && part->vessel->isEVA();
}
bool LoadedSceneIsNotLoading() { return HighLogic::LoadedScene() != GameScenes::LOADING; }
int TransformCount(Part* part, const std::string& name) {
    if (part == nullptr || !EffectEnvironment::findModelTransforms) return 0;
    return EffectEnvironment::findModelTransforms(*part, name);
}
bool TransformActive(Part* part, const std::string& name, int index) {
    if (part == nullptr || !EffectEnvironment::transformActiveInHierarchy) return true;
    return EffectEnvironment::transformActiveInHierarchy(*part, name, index);
}
void PlaySource(AudioSourceState& s) {
    s.playing = true;
    s.paused = false;
    s.elapsed = 0.0f;
}
void StopSource(AudioSourceState& s) {
    s.playing = false;
    s.paused = false;
}
}  // namespace fx_detail

// ---- EffectBehaviour -------------------------------------------------------------------------------------------------------

void EffectBehaviour::Emit(const std::string& kind, int transformIdx, int emitterIndex, bool hasPower, float power) const {
    if (owner == nullptr) return;
    EffectEvent e;
    e.kind = kind;
    e.effectName = effectName;
    e.instanceName = instanceName.value_or("");
    e.className = nodeName();
    e.transformIdx = transformIdx;
    e.emitterIndex = emitterIndex;
    e.hasPower = hasPower;
    e.power = power;
    owner->RecordEvent(std::move(e));
}

EffectInstanceState EffectBehaviour::Snapshot() const {
    EffectInstanceState s;
    s.effectName = effectName;
    s.instanceName = instanceName.value_or("");
    s.className = nodeName();
    Describe(s);
    return s;
}

// ---- registry --------------------------------------------------------------------------------------------------------------

namespace {
struct EffectType {
    std::string name;
    std::function<std::unique_ptr<EffectBehaviour>()> factory;
};
std::vector<EffectType>& Types() {
    static std::vector<EffectType> t;
    return t;
}
template <class T>
std::unique_ptr<EffectBehaviour> Make() {
    return std::make_unique<T>();
}
const EffectType* GetEffectType(const std::string& name) {  // EffectList.cs:36
    for (const EffectType& t : Types())
        if (t.name == name) return &t;
    return nullptr;
}
}  // namespace

// EffectList.cs:76 CreateEffectTypes: the EffectDefinition classes of Assembly-CSharp (the loaded assemblies' types)
void EffectList::CreateEffectTypes() {
    if (!Types().empty()) return;
    RegisterEffectType("ANIMATION", Make<AnimationFX>);
    RegisterEffectType("AUDIO", Make<AudioFX>);
    RegisterEffectType("AUDIO_LOOP", Make<AudioLoopFX>);
    RegisterEffectType("AUDIO_MULTI", Make<AudioMultiFX>);
    RegisterEffectType("AUDIO_MULTI_POOL", Make<AudioMultiPooledFX>);
    RegisterEffectType("AUDIO_SINGLE", Make<AudioSingleFX>);
    RegisterEffectType("MODEL_ANIMATION", Make<ModelAnimationFX>);
    RegisterEffectType("MODEL_MULTI_PARTICLE", Make<ModelMultiParticleFX>);
    RegisterEffectType("MODEL_PARTICLE", Make<ModelParticleFX>);
    RegisterEffectType("PARTICLE_CONFIG", Make<ParticleConfigFX>);
    RegisterEffectType("PREFAB_ANIMATION", Make<PrefabAnimationFX>);
    RegisterEffectType("PREFAB_MULTI_PARTICLE", Make<PrefabMultiParticleFX>);
    RegisterEffectType("PREFAB_PARTICLE", Make<PrefabParticleFX>);
    RegisterEffectType("PREFAB_SPAWN", Make<PrefabSpawnFX>);
    RegisterEffectType("ROCSAUDIO", Make<ROCsSFX>);
    log::Info("EffectList: Created " + std::to_string(Types().size()) + " effect types");
}

void EffectList::RegisterEffectType(const std::string& nodeName, std::function<std::unique_ptr<EffectBehaviour>()> factory) {
    if (GetEffectType(nodeName) != nullptr) {
        log::Error("Effect definitions already contains an effect with node name of '" + nodeName + "'");
        return;
    }
    Types().push_back({nodeName, std::move(factory)});
}

std::vector<std::string> EffectList::EffectTypeNames() {
    CreateEffectTypes();
    std::vector<std::string> out;
    for (const EffectType& t : Types()) out.push_back(t.name);
    return out;
}

// ---- EffectList ------------------------------------------------------------------------------------------------------------

// EffectList.cs:28 (ConstructEffectList :105 collects the EffectBehaviour components already on the part: none for a fresh list)
EffectList::EffectList(Part* host) : hostPart(host) { CreateEffectTypes(); }

std::vector<std::unique_ptr<EffectBehaviour>>* EffectList::Find(const std::string& name) {
    for (auto& e : effectList_)
        if (e.first == name) return &e.second;
    return nullptr;
}
const std::vector<std::unique_ptr<EffectBehaviour>>* EffectList::Find(const std::string& name) const {
    for (const auto& e : effectList_)
        if (e.first == name) return &e.second;
    return nullptr;
}

size_t EffectList::EffectCount() const {
    size_t n = 0;
    for (const auto& e : effectList_) n += e.second.size();
    return n;
}

// EffectList.cs:121 (a null instanceName matches any effect of that class: two unnamed nodes of one class under one effect share one behaviour)
EffectBehaviour* EffectList::GetEffect(const std::string& effectName, const std::optional<std::string>& instanceName, const std::string& typeName) {
    auto* list = Find(effectName);
    if (list == nullptr) throw std::out_of_range("The given key was not present in the dictionary.");  // effectList[effectName]
    for (auto& b : *list) {
        if ((!instanceName.has_value() || !(b->instanceName != instanceName)) && typeName == b->nodeName()) return b.get();
    }
    return nullptr;
}

// EffectList.cs:142 (string.Contains)
std::vector<std::string> EffectList::EffectsStartingWith(const std::string& effectName) const {
    std::vector<std::string> list;
    for (const auto& e : effectList_)
        if (e.first.find(effectName) != std::string::npos) list.push_back(e.first);
    return list;
}

// EffectList.cs:155
void EffectList::Initialize() {
    for (auto& e : effectList_)
        for (auto& b : e.second) b->OnInitialize();
}

// EffectList.cs:169
void EffectList::OnLoad(const ConfigNode& node) {
    for (int i = 0; i < node.nodes().Count(); i++) LoadEffect(*node.nodes()[i]);
}

// EffectList.cs:178
void EffectList::LoadEffect(const ConfigNode& node) {
    const std::string name = node.name;
    if (Find(name) == nullptr) effectList_.emplace_back(name, std::vector<std::unique_ptr<EffectBehaviour>>());
    for (int i = 0; i < node.nodes().Count(); i++) {
        const ConfigNode& configNode = *node.nodes()[i];
        const EffectType* effectType = GetEffectType(configNode.name);
        if (effectType == nullptr) {
            const std::string msg = "EffectList: Cannot find an effect type with node name of '" + configNode.name + "'";
            log::Error(msg);
            loadErrors.push_back(msg);
            continue;
        }
        const std::optional<std::string> value = configNode.GetValue("name");
        EffectBehaviour* b = GetEffect(name, value, effectType->name);
        if (b == nullptr) {
            auto created = effectType->factory();  // hostPart.gameObject.AddComponent(type)
            b = created.get();
            b->owner = this;
            b->effectName = name;
            b->instanceName = value;
            Find(name)->push_back(std::move(created));
        }
        b->hostPart = hostPart;
        b->OnLoad(configNode);
    }
}

// EffectList.cs:209
void EffectList::OnSave(ConfigNode& node) const {
    for (const auto& e : effectList_) {
        ConfigNode* configNode = node.AddNode(e.first);
        if (configNode == nullptr) throw NullReferenceException("EffectList.OnSave: AddNode returned null (blank effect name)");
        for (const auto& b : e.second) {
            ConfigNode* configNode2 = configNode->AddNode(b->nodeName());  // GetEffectType(type).name
            if (b->instanceName.has_value() && !b->instanceName->empty()) configNode2->AddValue("name", *b->instanceName);
            b->OnSave(*configNode2);
        }
    }
}

// EffectList.cs:231 (UnityEngine.Random.Range(0, Count - 1) is exclusive: the last effect is never picked)
void EffectList::PlayRandomEffect() {
    const int count = static_cast<int>(effectList_.size());
    const int idx = UnityRandom::Global().Range(0, count - 1);
    if (idx < 0 || idx >= count) throw std::out_of_range("Index was out of range.");
    Event(effectList_[static_cast<size_t>(idx)].first, -1);
}

// EffectList.cs:237
void EffectList::Event(const std::string& eventName, int transformIdx) {
    auto* list = Find(eventName);
    if (list == nullptr) return;
    for (size_t i = 0; i < list->size(); i++) (*list)[i]->OnEvent(transformIdx);
}

// EffectList.cs:249
void EffectList::Event(const std::string& eventName, float power, int transformIdx) {
    auto* list = Find(eventName);
    if (list == nullptr) return;
    for (size_t i = 0; i < list->size(); i++) (*list)[i]->OnEventPower(power, transformIdx);
}

void EffectList::Update(float deltaTime) {
    for (auto& e : effectList_)
        for (auto& b : e.second) b->Update(deltaTime);
}
void EffectList::OnGamePause() {
    for (auto& e : effectList_)
        for (auto& b : e.second)
            if (b->subscribedToPause) b->OnGamePause();
}
void EffectList::OnGameUnpause() {
    for (auto& e : effectList_)
        for (auto& b : e.second)
            if (b->subscribedToPause) b->OnGameUnpause();
}

std::vector<EffectInstanceState> EffectList::Snapshot() const {
    std::vector<EffectInstanceState> out;
    for (const auto& e : effectList_)
        for (const auto& b : e.second) out.push_back(b->Snapshot());
    return out;
}

std::vector<EffectEvent> EffectList::TakeEvents() {
    std::vector<EffectEvent> out;
    out.swap(events_);
    return out;
}

// Unity Instantiate of the part: every EffectBehaviour component is cloned with its serialized fields; ConstructEffectList (Awake) then
// groups the clones by effectName in component order (= the order of the original's AddComponent calls)
void EffectList::CopyFrom(const EffectList& original) {
    effectList_.clear();
    for (const auto& e : original.effectList_) {
        std::vector<std::unique_ptr<EffectBehaviour>> v;
        for (const auto& b : e.second) {
            auto c = b->Clone();
            c->owner = this;
            c->hostPart = hostPart;
            c->effectName = b->effectName;
            c->instanceName = b->instanceName;
            v.push_back(std::move(c));
        }
        if (!v.empty()) effectList_.emplace_back(e.first, std::move(v));
    }
}

void EffectList::PruneEmpty() {
    effectList_.erase(std::remove_if(effectList_.begin(), effectList_.end(), [](const auto& e) { return e.second.empty(); }), effectList_.end());
}

}  // namespace ksp
