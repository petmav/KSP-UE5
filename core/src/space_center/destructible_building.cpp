// WP-42: DestructibleBuilding.cs.
#include "ksp/space_center/destructible_building.hpp"

#include <algorithm>
#include <cfloat>

#include "ksp/career/career_environment.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
// bool.Parse(node.GetValue(key))
bool ParseBoolValue(const ConfigNode& node, const char* key) {
    bool b = false;
    if (!dotnet::TryParseBool(*node.GetValue(key), b)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
    return b;
}
float TimeSinceLevelLoad() { return DestructibleBuilding::timeSinceLevelLoad ? DestructibleBuilding::timeSinceLevelLoad() : 0.0f; }
CoroutineScheduler& Sched() { return HighLogic::Scheduler(); }
}  // namespace

// Quaternion.Lerp (native). Unity Runtime/Math/Quaternion.h Lerp: short-way blend component-wise, then Normalize. UNVERIFIED (no capture).
Quaternionf QuaternionLerpNative(const Quaternionf& q1, const Quaternionf& q2, float t) {
    t = Mathf::Clamp01(t);
    const float dot = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
    Quaternionf tmp;
    if (dot < 0.0f) {
        tmp = {q1.x + t * (-q2.x - q1.x), q1.y + t * (-q2.y - q1.y), q1.z + t * (-q2.z - q1.z), q1.w + t * (-q2.w - q1.w)};
    } else {
        tmp = {q1.x + t * (q2.x - q1.x), q1.y + t * (q2.y - q1.y), q1.z + t * (q2.z - q1.z), q1.w + t * (q2.w - q1.w)};
    }
    const float mag2 = tmp.x * tmp.x + tmp.y * tmp.y + tmp.z * tmp.z + tmp.w * tmp.w;
    const float inv = 1.0f / UnityNative::SqrtF(mag2);
    return {tmp.x * inv, tmp.y * inv, tmp.z * inv, tmp.w * inv};
}

// ---- CollapsibleObject ------------------------------------------------------------------------------------------------------

// DestructibleBuilding.cs:54
void DestructibleBuilding::CollapsibleObject::Init() {
    if (!init) {
        stPos = localPosition;
        stRot = localRotation;
        endPos = stPos + collapseOffset;
        init = true;
    }
}

// DestructibleBuilding.cs:66
void DestructibleBuilding::CollapsibleObject::Load(const ConfigNode& node) {
    if (node.HasValue("intact")) {
        intact = ParseBoolValue(node, "intact");
        destroyed = !intact;
    }
    if (destroyed) SetDestroyed(destroyed);
}

// DestructibleBuilding.cs:79
void DestructibleBuilding::CollapsibleObject::Save(ConfigNode& node) { node.AddValue("intact", intact); }

// DestructibleBuilding.cs:84
void DestructibleBuilding::CollapsibleObject::Collapse(DestructibleBuilding& host) {
    if (!intact) return;
    intact = false;
    if (sharedWith != nullptr && !sharedWith->IsIntact()) {
        destroyed = true;
        return;
    }
    if (hasSecondaryFXPrefab) {
        // host.spawnVFX(SecondaryFXPrefab).Play(host.onFXComplete): the effect is destroyed when done (engine)
    }
    if (collapseBehaviour == Behaviour::Collapse)
        Sched().StartCoroutine(collapseCoroutine(), host.owner);
    else
        Sched().StartCoroutine(disableCoroutine(), host.owner);
}

// DestructibleBuilding.cs:111
Enumerator DestructibleBuilding::CollapsibleObject::disableCoroutine() {
    co_yield YieldInstruction::WaitForSeconds(collapseDuration);
    SetDestroyed(true);
    destroyed = true;
}

// DestructibleBuilding.cs:118
Enumerator DestructibleBuilding::CollapsibleObject::collapseCoroutine() {
    const float startT = TimeSinceLevelLoad();
    const float endT = f32store(fx(TimeSinceLevelLoad()) + fx(collapseDuration));
    const Vector3f onUnitSphere = UnityRandom::Global().onUnitSphere();
    const Vector3f euler(f32store(fx(collapseTiltMax.x) * fx(onUnitSphere.x)), f32store(fx(collapseTiltMax.y) * fx(onUnitSphere.y)),
                         f32store(fx(collapseTiltMax.z) * fx(onUnitSphere.z)));
    const Quaternionf endRot = UnityNative::Euler(euler) * stRot;
    while (TimeSinceLevelLoad() <= endT) {
        const float t = Mathfx::XLerp(0.0f, 1.0f, Mathf::InverseLerp(startT, endT, TimeSinceLevelLoad()), 3.0f);
        localPosition = Vector3f::Lerp(stPos, endPos, t);
        localRotation = QuaternionLerpNative(stRot, endRot, t);
        if (hasReplacementObject && !replacementObjectActive && TimeSinceLevelLoad() > f32store(fx(startT) + fx(replaceDelay))) replacementObjectActive = true;
        co_yield YieldInstruction::Null();
    }
    collapseObjectActive = false;
    destroyed = true;
}

// DestructibleBuilding.cs:140
void DestructibleBuilding::CollapsibleObject::Repair(DestructibleBuilding& host) {
    if (destroyed) {
        destroyed = false;
        if (sharedWith != nullptr && sharedWith->IsDestroyed())
            intact = true;
        else
            Sched().StartCoroutine(repairCoroutine(), host.owner);
    }
}

// DestructibleBuilding.cs:156
Enumerator DestructibleBuilding::CollapsibleObject::repairCoroutine() {
    co_yield YieldInstruction::WaitForSeconds(repairDuration);
    SetDestroyed(false);
    intact = true;
}

// DestructibleBuilding.cs:163
void DestructibleBuilding::CollapsibleObject::SetDestroyed(bool st) {
    if (st) {
        collapseObjectActive = false;
        if (hasReplacementObject) replacementObjectActive = true;
    } else {
        collapseObjectActive = true;
        localPosition = stPos;
        localRotation = stRot;
        if (hasReplacementObject) replacementObjectActive = false;
    }
    destroyed = st;
    intact = !st;
}

// DestructibleBuilding.cs:187
void DestructibleBuilding::CollapsibleObject::Reset() { SetDestroyed(false); }

// ---- DestructibleBuilding ---------------------------------------------------------------------------------------------------

DestructibleBuilding::DestructibleBuilding() = default;
DestructibleBuilding::~DestructibleBuilding() { OnDestroy(); }

void DestructibleBuilding::ClearStatics() {
    timeSinceLevelLoad = nullptr;
    saveGame = nullptr;
}

// DestructibleBuilding.cs:241
void DestructibleBuilding::Awake() {
    for (CollapsibleObject& c : CollapsibleObjects) c.Init();
    GE::onGameSceneLoadRequested.Add(this, &DestructibleBuilding::OnLeavingScene);
    invulnerableTime_ = 0.0f;
    GE::onPhysicsEaseStart.Add(this, &DestructibleBuilding::OnPhysicsEase);
    OnLevelLoaded(HighLogic::LoadedScene());
    GE::onLevelWasLoaded.Add(this, &DestructibleBuilding::OnSceneStart);
    eventsAdded_ = true;
}

// DestructibleBuilding.cs:260
void DestructibleBuilding::Start() {
    if (!RegisterInstance()) Reset();
}

// DestructibleBuilding.cs:268
void DestructibleBuilding::OnPhysicsEase(Vessel* v) {
    if (v == static_cast<Vessel*>(FlightRegistry::ActiveVessel) && invulnerableTime_ < PhysicsGlobals::BuildingEasingInvulnerableTime())
        invulnerableTime_ = PhysicsGlobals::BuildingEasingInvulnerableTime();
}

// DestructibleBuilding.cs:276
void DestructibleBuilding::OnLevelLoaded(GameScenes scn) {
    if (scn == GameScenes::FLIGHT) StartFlightNoDamagePeriod();
}

// DestructibleBuilding.cs:284
void DestructibleBuilding::OnSceneStart(GameScenes scn) {
    OnLevelLoaded(scn);
    if (static_cast<uint32_t>(static_cast<int>(scn) - 5) <= 2u && activeInHierarchy) Sched().StartCoroutine(LoadOnSceneStart(), owner);
}

// DestructibleBuilding.cs:293
Enumerator DestructibleBuilding::LoadOnSceneStart() {
    co_yield YieldInstruction::WaitForEndOfFrame();
    if (!RegisterInstance()) Reset();
}

// DestructibleBuilding.cs:303
void DestructibleBuilding::CompileID() {
    id = compiledId;
    preCompiledId = true;
}

// DestructibleBuilding.cs:310
void DestructibleBuilding::ClearID() {
    id = "";
    preCompiledId = false;
}

// DestructibleBuilding.cs:316
bool DestructibleBuilding::RegisterInstance() {
    if (!preCompiledId) id = compiledId;
    bool result = false;
    if (id.empty()) {
        log::Error("[DestructibleBuilding]: ID for this building is not defined correctly. Make sure all IDs are compiled and serialized before running the game.");
    } else {
        result = ScenarioDestructibles::RegisterDestructible(this, id);
        registered_ = true;
    }
    return result;
}

// DestructibleBuilding.cs:335
void DestructibleBuilding::UnregisterInstance(bool saveState) {
    if (registered_) {
        if (id.empty()) {
            log::Error("[DestructibleBuilding]: ID for this building is not defined correctly. Cannot unregister.");
        } else {
            ScenarioDestructibles::UnregisterDestructible(this, id, saveState);
            registered_ = false;
        }
    }
}

// DestructibleBuilding.cs:351
void DestructibleBuilding::Load(const ConfigNode& node) {
    if (node.HasValue("intact")) {
        intact_ = ParseBoolValue(node, "intact");
        destroyed_ = !intact_;
    }
    int num = static_cast<int>(CollapsibleObjects.size());
    while (num-- > 0) {
        CollapsibleObject& c = CollapsibleObjects[static_cast<size_t>(num)];
        c.Init();
        if (c.sharedWith != nullptr)
            c.SetDestroyed(!intact_ || !c.sharedWith->intact_);
        else
            c.SetDestroyed(!intact_);
    }
    OnLoaded.Fire(this);
}

// DestructibleBuilding.cs:374
void DestructibleBuilding::Save(ConfigNode& node) { node.AddValue("intact", intact_); }

// DestructibleBuilding.cs:379
void DestructibleBuilding::FixedUpdate(float fixedDeltaTime) {
    if (invulnerableTime_ > 0.0f) invulnerableTime_ = f32store(fx(invulnerableTime_) - fx(fixedDeltaTime));
    damageUpdate();
}

// DestructibleBuilding.cs:388
void DestructibleBuilding::damageUpdate() {
    if (damage_ > 0.0f) {
        damage_ = f32store(fx(damage_) - fx(impactMomentumThreshold) * fx(BuildingToughnessFactor) / fx(damageDecay) * fx(TimeWarp::fixedDeltaTime()));
        if (damage_ < 0.0f) damage_ = 0.0f;
    }
}

// DestructibleBuilding.cs:401
void DestructibleBuilding::Demolish() {
    if (destroyed_ || !intact_) return;
    for (CollapsibleObject& c : CollapsibleObjects) c.Collapse(*this);
    if (hasDemolitionFXPrefab) {
        if (playDemolitionFX) playDemolitionFX([this] { onDemolitionComplete(); });
    } else {
        float num = -FLT_MAX;
        int num2 = static_cast<int>(CollapsibleObjects.size());
        while (num2-- > 0) {
            const float collapseDuration = CollapsibleObjects[static_cast<size_t>(num2)].collapseDuration;
            if (collapseDuration > num) num = collapseDuration;
        }
        Sched().StartCoroutine(delayedCallback(num, [this] { onDemolitionComplete(); }), owner);
    }
    intact_ = false;
    GE::OnKSCStructureCollapsing.Fire(this);
}

// DestructibleBuilding.cs:440
void DestructibleBuilding::onDemolitionComplete() {
    destroyed_ = true;
    clearVFX();
    GE::OnKSCStructureCollapsed.Fire(this);
}

// DestructibleBuilding.cs:448
void DestructibleBuilding::Repair() {
    if (intact_) {
        log::Warning("[Destructible Building]: Building does not require repairs.");
        return;
    }
    if (!destroyed_) {
        log::Warning("[Destructible Building]: Building is already undergoing repairs.");
        return;
    }
    for (CollapsibleObject& c : CollapsibleObjects) c.Repair(*this);
    if (hasRepairFXPrefab) {
        if (playRepairFX) playRepairFX([this] { onRepairComplete(); });
    } else {
        float num = -FLT_MAX;
        int num2 = static_cast<int>(CollapsibleObjects.size());
        while (num2-- > 0) {
            const float repairDuration = CollapsibleObjects[static_cast<size_t>(num2)].repairDuration;
            if (repairDuration > num) num = repairDuration;
        }
        Sched().StartCoroutine(delayedCallback(num, [this] { onRepairComplete(); }), owner);
    }
    destroyed_ = false;
    GE::OnKSCStructureRepairing.Fire(this);
}

// DestructibleBuilding.cs:488
void DestructibleBuilding::onRepairComplete() {
    intact_ = true;
    clearVFX();
    GE::OnKSCStructureRepaired.Fire(this);
}

// DestructibleBuilding.cs:495
void DestructibleBuilding::Reset() {
    intact_ = true;
    destroyed_ = false;
    damage_ = 0.0f;
    invulnerableTime_ = 0.0f;
    clearVFX();
    for (CollapsibleObject& c : CollapsibleObjects) c.Reset();
}

// DestructibleBuilding.cs:512
float DestructibleBuilding::GetRepairSequenceDuration() const {
    float num = -FLT_MAX;
    int num2 = static_cast<int>(CollapsibleObjects.size());
    while (num2-- > 0) {
        const float repairDuration = CollapsibleObjects[static_cast<size_t>(num2)].repairDuration;
        if (repairDuration > num) num = repairDuration;
    }
    return num;
}

// DestructibleBuilding.cs:549
void DestructibleBuilding::OnLeavingScene(GameScenes scn) {
    if (HighLogic::LoadedScene() == GameScenes::SPACECENTER || HighLogic::LoadedScene() == GameScenes::FLIGHT) {
        UnregisterInstance(true);
        clearVFX();
        Sched().StopAllCoroutines(owner);
    }
    if (scn == GameScenes::MAINMENU || scn == GameScenes::EDITOR || scn == GameScenes::FLIGHT || scn == GameScenes::TRACKSTATION) Reset();
}

// DestructibleBuilding.cs:563
void DestructibleBuilding::OnDisable() { needsResetOnReEnable_ = true; }

// DestructibleBuilding.cs:568
void DestructibleBuilding::OnEnable() {
    if (needsResetOnReEnable_) {
        if (!RegisterInstance()) Reset();
        needsResetOnReEnable_ = false;
    }
}

// DestructibleBuilding.cs:580
void DestructibleBuilding::OnDestroy() {
    if (registered_) UnregisterInstance(false);
    clearVFX();
    if (eventsAdded_) {
        GE::onGameSceneLoadRequested.Remove(this, &DestructibleBuilding::OnLeavingScene);
        GE::onLevelWasLoaded.Remove(this, &DestructibleBuilding::OnSceneStart);
        GE::onVesselGoOffRails.Remove(this, &DestructibleBuilding::EndFlightNoDamagePeriod);
        GE::onPhysicsEaseStart.Remove(this, &DestructibleBuilding::OnPhysicsEase);
        eventsAdded_ = false;
    }
}

// DestructibleBuilding.cs:600
Enumerator DestructibleBuilding::delayedCallback(float delay, std::function<void()> cb) {
    co_yield YieldInstruction::WaitForSeconds(delay);
    cb();
}

// DestructibleBuilding.cs:606
void DestructibleBuilding::OnCollisionEnter(const ContactInfo& c) {
    if (c.contactNormals.empty() || invulnerableTime_ > 0.0f) return;
    Vector3f lhs = Vector3f::zero();
    float num = -FLT_MAX;
    int num2 = static_cast<int>(c.contactNormals.size());
    const Vector3f relNorm = c.relativeVelocity.normalized();
    while (num2-- > 0) {
        const Vector3f normal = c.contactNormals[static_cast<size_t>(num2)];
        const float num3 = Mathf::Abs(Vector3f::Dot(normal, relNorm));
        if (num3 > num) {
            num = num3;
            lhs = normal;
        }
    }
    float f = Vector3f::Dot(lhs, relNorm);
    float num4 = c.relativeVelocity.sqrMagnitude();
    float num5 = FLT_MAX;
    float num6 = FLT_MAX;
    if (c.hasPart) {
        if (c.hasPartRigidbody) num5 = c.partRigidbodyVelocity.magnitude();
        if (c.hasVessel) num6 = c.vesselRbVelocity.sqrMagnitude();
    }
    if (f32store(fx(num4) * fx(PhysicsGlobals::BuildingImpactDamageMaxVelocityMult())) > num5) {
        num4 = num5;
        f = Vector3f::Dot(lhs, c.partRigidbodyVelocity.normalized());
    }
    if (f32store(fx(num4) * fx(PhysicsGlobals::BuildingImpactDamageMaxVelocityMult())) > num6) {
        num4 = num6;
        f = Vector3f::Dot(lhs, c.vesselRbVelocity.normalized());
    }
    if (PhysicsGlobals::BuildingImpactDamageUseMomentum()) num4 = Mathf::Sqrt(num4);
    f = Mathfx::Ease(Mathf::Abs(f), 3.0f);
    Game* game = SaveEnvironment::Current().currentGame;
    const float mult = game->Parameters->CustomParams<GameParameters::AdvancedParams>().BuildingImpactDamageMult;
    float d = f32store(fx(num4) * fx(c.rigidbodyMass) * fx(f) * fx(mult));
    if (damage_ > 0.0f && c.hasPart) {
        if (PhysicsGlobals::BuildingImpactDamageUseMomentum()) num6 = Mathf::Sqrt(num6);
        d = f32store(fx(num6) * fx(static_cast<float>(c.vesselTotalMass)) * fx(f) * fx(mult));
    }
    AddDamage(d);
}

// DestructibleBuilding.cs:682
void DestructibleBuilding::AddDamage(float d) {
    Game* game = SaveEnvironment::Current().currentGame;
    if (!SaveEnvironment::Current().loadedSceneIsGame ||
        (!game->Parameters->Difficulty.IndestructibleFacilities && !indestructible_ && d > PhysicsGlobals::BuildingImpactDamageMinDamageFraction())) {
        damage_ = f32store(fx(damage_) + fx(d));
        if (damage_ > f32store(fx(impactMomentumThreshold) * fx(BuildingToughnessFactor)) && intact_) {
            Demolish();
            if (saveGame) saveGame();
        }
    }
}

// DestructibleBuilding.cs:709
void DestructibleBuilding::StartFlightNoDamagePeriod() {
    indestructible_ = true;
    if (!noDamageHandlerAdded_) {
        GE::onVesselGoOffRails.Add(this, &DestructibleBuilding::EndFlightNoDamagePeriod);
        noDamageHandlerAdded_ = true;
    }
}

// DestructibleBuilding.cs:715 (v.StartCoroutine(CallbackUtil.DelayedCallback(1f, ...)): the one second delay on the shared scheduler)
void DestructibleBuilding::EndFlightNoDamagePeriod(Vessel* v) {
    if (v == static_cast<Vessel*>(FlightRegistry::ActiveVessel)) {
        GE::onVesselGoOffRails.Remove(this, &DestructibleBuilding::EndFlightNoDamagePeriod);
        noDamageHandlerAdded_ = false;
        Sched().StartCoroutine(delayedCallback(1.0f, [this] { indestructible_ = false; }), owner);
    }
}

}  // namespace ksp
