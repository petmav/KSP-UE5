// WP-12c: SceneryCityHost (see ksp/physics/scenery_host.hpp).
#include "ksp/physics/scenery_host.hpp"

#include <algorithm>

#include "ksp/celestial/celestial_body.hpp"

namespace ksp::physics {

SceneryCityHost::SceneryCityHost(PhysicsBubble& bubble, const SceneryCity& city, uint64_t keyBase, const SceneryLevels& levels)
    : host_(bubble.Contacts()), city_(city), keyBase_(keyBase), levels_(levels) {
    if (city.className == "PQSCity2") {
        pqsCity2_ = std::make_unique<PQSCity2Colliders>(bubble.World(), keyBase);
        ApplySceneryLod(*pqsCity2_, city, levels);
        pqsCity2_->set.onPlaced = [this](uint64_t key, const CityObject& o) { Register(key, o); };
        pqsCity2_->set.onRemoved = [this](uint64_t key) { host_.UnregisterStatic(key); };
    } else {
        pqsCity_ = std::make_unique<PQSCityColliders>(bubble.World(), keyBase);
        ApplySceneryLod(*pqsCity_, city, levels);
        pqsCity_->set.onPlaced = [this](uint64_t key, const CityObject& o) { Register(key, o); };
        pqsCity_->set.onRemoved = [this](uint64_t key) { host_.UnregisterStatic(key); };
    }
    initiallyActive_ = SceneryInitiallyActive(city, levels);
    hook_ = host_.AddRefreshHook([this] { Refresh(); });
}

SceneryCityHost::~SceneryCityHost() {
    host_.RemoveRefreshHook(hook_);
    // the sets' destructors remove the static bodies; their onRemoved would reach a dying host: detach first
    set().onRemoved = nullptr;
    for (size_t i = 0; i < set().ObjectCount(); ++i)
        for (size_t c = 0; c < set().Object(i).colliders.size(); ++c) host_.UnregisterStatic(set().KeyOf(i, c));
    set().onPlaced = nullptr;
}

void SceneryCityHost::Register(uint64_t key, const CityObject& o) {
    StaticColliderInfo info;
    info.layer = o.layer;
    info.tag = o.tag;
    info.name = o.name;
    info.crashObjectName = o.crashObjectName;
    info.isMobileLaunchPad = o.mobileLaunchPad;
    host_.RegisterStatic(key, info);
}

void SceneryCityHost::SetPose(const Vector3d& physicsPosition, const QuaternionD& physicsRotation) {
    position_ = physicsPosition;
    rotation_ = physicsRotation;
    set().SetPose(position_, rotation_);
}

void SceneryCityHost::PlaceOnBody(const CelestialBody& body, const Vector3d& pqsLocalPosition, const QuaternionD& pqsLocalRotation) {
    body_ = &body;
    localPosition_ = pqsLocalPosition;
    localRotation_ = pqsLocalRotation;
    Refresh();
}

void SceneryCityHost::Refresh() {
    if (body_ == nullptr) return;
    const QuaternionD br = host_.TerrainRotation(*body_);
    SetPose(host_.TerrainOrigin(*body_) + br * localPosition_, br * localRotation_);
}

void SceneryCityHost::EnsureStarted() {
    if (started_) return;
    started_ = true;
    if (pqsCity_) {
        pqsCity_->OnSetup();                // PQSCity.cs:168
        pqsCity_->OnSphereStart(false);     // :179 the sphere is not alive yet: the children are off until a target is found
    } else {
        pqsCity2_->OnSetup();               // PQSCity2.cs:407
    }
    for (size_t i : initiallyActive_) set().SetObjectActive(i, true);
}

void SceneryCityHost::Update(const Vector3d& target, bool hasTarget) {
    EnsureStarted();
    if (pqsCity_) pqsCity_->OnUpdateFinished(hasTarget, target);
    else pqsCity2_->OnUpdateFinished(hasTarget, target);
}

void SceneryCityHost::ActivateAll() { Update(position_, true); }

void SceneryCityHost::Deactivate() {
    EnsureStarted();
    if (pqsCity_) pqsCity_->OnSphereInactive();
    else pqsCity2_->OnSphereInactive();
}

// CollapsibleObject.SetDestroyed (DestructibleBuilding.cs, declaration at line 163) applied to a whole building
void SceneryCityHost::SetBuildingDestroyed(size_t index, bool destroyed) {
    const SceneryDestructible& d = city_.destructibles.at(index);
    const std::vector<bool> present = SceneryPresence(city_, levels_);
    for (const SceneryCollapsible& c : d.collapsibles) {
        for (size_t i : SceneryActiveSubtree(city_, c.collapse, present)) set().SetObjectActive(i, !destroyed);
        if (c.replacement >= 0)
            for (size_t i : SceneryActiveSubtree(city_, static_cast<size_t>(c.replacement), present)) set().SetObjectActive(i, destroyed);
    }
}

}  // namespace ksp::physics
