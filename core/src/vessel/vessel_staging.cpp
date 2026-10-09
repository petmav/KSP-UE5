// WP-16: the staging members of Vessel.cs (ClearStaging, ResumeStaging) and the Vessel lifetime helpers Part.decouple / Couple /
// Undock need (AddComponent<Vessel>, DestroyImmediate, ownership of the parts that move between vessels).
#include <algorithm>

#include "ksp/events/game_events.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = GameEvents;

// Vessel.cs:2130
void Vessel::ClearStaging() {
    const size_t count = parts.size();
    for (size_t i = 0; i < count; i++) parts[i]->RemoveStageIcon(false);
    GE::onVesselClearStaging.Fire(this);
}

// Vessel.cs:2140
void Vessel::ResumeStaging() {
    const size_t count = parts.size();
    for (size_t i = 0; i < count; i++) {
        Part* part = parts[i];
        part->UpdateStageability(false, false);
        if (part->fields.stagingOn) part->CreateStageIcon(false);
    }
    GE::onVesselResumeStaging.Fire(this);
}

std::vector<std::unique_ptr<Vessel>>& Vessel::SpawnedVessels() {
    static std::vector<std::unique_ptr<Vessel>> spawned;
    return spawned;
}

// part.gameObject.AddComponent<Vessel>() (Awake runs in the constructor)
Vessel* Vessel::SpawnVessel() {
    SpawnedVessels().push_back(std::make_unique<Vessel>());
    return SpawnedVessels().back().get();
}

// UnityEngine.Object.DestroyImmediate(vessel)
void Vessel::DestroyImmediate(Vessel* v) {
    if (v == nullptr) return;
    v->parts.clear();  // the parts moved to another vessel keep their `vessel` pointer: the destructor must not null it
    v->rootPart = nullptr;
    auto& sp = SpawnedVessels();
    auto it = std::find_if(sp.begin(), sp.end(), [v](const std::unique_ptr<Vessel>& u) { return u.get() == v; });
    if (it != sp.end()) sp.erase(it);  // ~Vessel leaves the registry
    else {
        FlightRegistry::RemoveVessel(v);
        if (FlightRegistry::ActiveVessel == v) FlightRegistry::ActiveVessel = nullptr;
    }
}

std::vector<std::unique_ptr<Part>> Vessel::ReleaseOwnedParts(const std::vector<Part*>& which) {
    std::vector<std::unique_ptr<Part>> out;
    for (Part* p : which) {
        auto it = std::find_if(ownedParts_.begin(), ownedParts_.end(), [p](const std::unique_ptr<Part>& u) { return u.get() == p; });
        if (it != ownedParts_.end()) {
            out.push_back(std::move(*it));
            ownedParts_.erase(it);
        }
    }
    return out;
}

void Vessel::AcceptOwnedParts(std::vector<std::unique_ptr<Part>> moved) {
    for (auto& p : moved) ownedParts_.push_back(std::move(p));
}

void Vessel::RunDelayedFrameCalls() {
    std::vector<std::function<void()>> calls;
    calls.swap(delayedFrameCalls);
    for (auto& c : calls) c();
}

}  // namespace ksp
