// WP-UE-16a: the Part side of the effects system: Part.Effects / InitializeEffects / LoadEffects / SaveEffects / Effect (Part.cs:6162-6200) and the
// FXGroup objects behind Part.fxGroups.
#include <algorithm>

#include "ksp/dotnet/text.hpp"
#include "ksp/fx/effect_classes.hpp"
#include "ksp/fx/effects.hpp"
#include "ksp/fx/fx_group.hpp"
#include "ksp/parts/part.hpp"

namespace ksp {

// Part.cs:1103 (EffectSetup :6162 creates the list in Awake; the cfg's EFFECTS nodes were loaded into the prefab by PartLoader.cs:533, and the
// instance's components are clones of the prefab's: the nodes are loaded into the new list on first use. ConstructEffectList only sees
// components, so an effect name without a valid class node has no entry in an instance)
EffectList& Part::Effects() {
    if (!effectList_) effectList_ = std::make_unique<EffectList>(this);
    if (!effectsBuilt_) {
        effectsBuilt_ = true;
        for (const auto& n : effects) effectList_->OnLoad(*n);
        effectList_->PruneEmpty();
    }
    return *effectList_;
}

// Part.cs:6170
void Part::InitializeEffects() { Effects().Initialize(); }

// Part.cs:6175
void Part::LoadEffects(const ConfigNode& node) {
    EffectList& list = Effects();
    list.OnLoad(node);
}

// Part.cs:6180
void Part::SaveEffects(ConfigNode& node) const { const_cast<Part*>(this)->Effects().OnSave(node); }

// Part.cs:6185
void Part::Effect(const std::string& effectName, int transformIdx) {
    if (effectCallHook) effectCallHook(*this, effectName, false, 0.0f, transformIdx);
    if (!HasEffects()) return;  // no EFFECTS node: the lookup in the empty list is a no-op
    Effects().Event(effectName, transformIdx);
}

// Part.cs:6190
void Part::Effect(const std::string& effectName, float effectPower, int transformIdx) {
    if (effectCallHook) effectCallHook(*this, effectName, true, effectPower, transformIdx);
    if (!HasEffects()) return;
    Effects().Event(effectName, effectPower, transformIdx);
}

// ---- legacy FXGroups -------------------------------------------------------------------------------------------------------

// Builds the FXGroup objects of fxGroupNames. The part's fx_* lines (PartLoader.cs:826) add one particle system / light per line to every group
// named from the 7th value on ("true" is the Keep Pos flag); the sound_* lines (PartLoader.cs:1507 LoadSound, ConstructSoundFXGroup :1556) set
// `sfx` of the groups named by the value (a missing group is created with the UNTRIMMED text). fx lines are applied before sound lines (the part
// keeps the two lists apart, so a sound line that precedes an fx line in the cfg would create its group earlier in C#: only the list order).
void Part::SyncFXGroups() {
    auto find = [&](const std::string& n) -> FXGroup* {
        for (auto& g : fxGroupObjects_)
            if (g->name == n) return g.get();
        return nullptr;
    };
    auto ensureName = [&](const std::string& n) {
        if (std::find(fxGroupNames.begin(), fxGroupNames.end(), n) == fxGroupNames.end()) fxGroupNames.push_back(n);
    };
    EffectList* sink = Effects().hostPart == this ? &Effects() : nullptr;
    if (!fxDefinitionsApplied_) {
        fxDefinitionsApplied_ = true;
        for (const std::string& n : fxGroupNames) {
            auto g = std::make_unique<FXGroup>(n);
            g->eventSink = sink;
            fxGroupObjects_.push_back(std::move(g));
        }
        fxGroupNamesSynced_ = fxGroupNames.size();
        for (const auto& def : fxDefinitions) {
            const std::vector<std::string> a = dotnet::Split(def.second, ",", false);
            for (size_t j = 6; j < a.size(); j++) {
                const std::string text = dotnet::Trim(a[j]);
                if (text == "true") continue;
                FXGroup* g = find(text);
                if (g == nullptr) {
                    ensureName(text);
                    auto ng = std::make_unique<FXGroup>(text);
                    ng->eventSink = sink;
                    g = ng.get();
                    fxGroupObjects_.push_back(std::move(ng));
                    fxGroupNamesSynced_ = fxGroupNames.size();
                }
                if (FXGroupEnvironment::fxPrefabParticleSystems) {
                    for (const auto& ps : FXGroupEnvironment::fxPrefabParticleSystems(def.first)) g->fxEmittersNewSystem.push_back(ps);
                } else {
                    g->fxEmittersNewSystem.push_back(FXParticleSystem{});
                }
                if (FXGroupEnvironment::fxPrefabLights)
                    for (const auto& l : FXGroupEnvironment::fxPrefabLights(def.first)) g->lights.push_back(l);
            }
        }
        for (const auto& def : soundDefinitions) {
            if (!fx_detail::ClipExists(def.first)) continue;  // LoadSound: "ERROR: no sound file found"
            const std::vector<std::string> a = dotnet::Split(def.second, ",", false);
            for (const std::string& text : a) {
                FXGroup* g = find(dotnet::Trim(text));
                if (g == nullptr) {
                    ensureName(text);
                    auto ng = std::make_unique<FXGroup>(text);
                    ng->eventSink = sink;
                    ng->sfx = def.first;
                    fxGroupObjects_.push_back(std::move(ng));
                    fxGroupNamesSynced_ = fxGroupNames.size();
                } else {
                    g->sfx = def.first;
                }
            }
        }
    }
    // names added later (parachute `deploy`, ...): a fresh group
    while (fxGroupNamesSynced_ < fxGroupNames.size()) {
        const std::string& n = fxGroupNames[fxGroupNamesSynced_++];
        if (find(n) == nullptr) {
            auto g = std::make_unique<FXGroup>(n);
            g->eventSink = sink;
            fxGroupObjects_.push_back(std::move(g));
        }
    }
}

// Part.cs:1525 (the object behind findFxGroup)
FXGroup* Part::FindFXGroup(const std::string& groupID) {
    SyncFXGroups();
    for (auto& g : fxGroupObjects_)
        if (g->name == groupID) return g.get();
    return nullptr;
}

// part.fxGroups.Add(group)
FXGroup* Part::AddFXGroup(std::unique_ptr<FXGroup> group) {
    SyncFXGroups();
    if (std::find(fxGroupNames.begin(), fxGroupNames.end(), group->name) == fxGroupNames.end()) fxGroupNames.push_back(group->name);
    fxGroupNamesSynced_ = fxGroupNames.size();
    group->eventSink = &Effects();
    fxGroupObjects_.push_back(std::move(group));
    return fxGroupObjects_.back().get();
}

// Part.Start (Part.cs line 1783): the fxGroups[i].begin(audioSource) loop, last to first
void Part::BeginFXGroups() {
    SyncFXGroups();
    size_t count = fxGroupObjects_.size();
    while (count-- > 0) fxGroupObjects_[count]->begin(&Effects().partAudioSource);
}

void Part::UpdateFX(float deltaTime) {
    if (effectList_) effectList_->Update(deltaTime);
    for (auto& g : fxGroupObjects_) g->Update();
}

}  // namespace ksp
