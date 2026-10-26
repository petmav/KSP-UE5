// FlightGlobals (FlightGlobals.cs): the registry and the members that read Vessels. The body-only half is
// src/celestial/flight_globals.cpp. See include/ksp/celestial/flight_globals.hpp for the kspcore reading of the MonoBehaviour.
// C# paths: Assembly-CSharp/FlightGlobals.cs.
#include <algorithm>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/collision_enhancer.hpp"
#include "ksp/physics/orbit_physics_manager.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_persistence.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/scene_drivers.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_autopilot.hpp"

namespace ksp {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
template <class T>
void EraseFirst(std::vector<T>& v, const T& x) {
    auto it = std::find(v.begin(), v.end(), x);
    if (it != v.end()) v.erase(it);
}
template <class T>
bool Contains(const std::vector<T>& v, const T& x) {
    return std::find(v.begin(), v.end(), x) != v.end();
}
PersistentIdRegistry& Ids() { return SaveEnvironment::Current().persistentIds; }
Vessel& Active() {
    if (FlightGlobals::ActiveVessel == nullptr) throw NullReferenceException("FlightGlobals.ActiveVessel is null");
    return *FlightGlobals::ActiveVessel;
}

// FlightGlobals.cs:900-938 (HookVesselEvents handlers). The C# subscribes INSTANCE methods of the FlightGlobals MonoBehaviour (EventData.EvtDelegate
// reads evt.Target.GetType(): a static method group would throw NullReferenceException), so the handlers are members of one object.
struct FlightGlobalsHooks {
    void OnPartEvent(Part* part) {
        if (part != nullptr && part->vessel != nullptr) GE::onVesselStandardModification.Fire(part->vessel);
    }
    void OnVesselEvent(Vessel* vessel) {
        if (vessel != nullptr) GE::onVesselStandardModification.Fire(vessel);
    }
    void OnPartEventTargetAction(GE::HostTargetAction<Part*, Part*> data) {
        if (data.host != nullptr && data.host->vessel != nullptr) GE::onVesselStandardModification.Fire(data.host->vessel);
        if (data.host != nullptr && data.target->vessel != nullptr) GE::onVesselStandardModification.Fire(data.target->vessel);
    }
    void OnPartEventFromToAction(GE::FromToAction<Part*, Part*> data) {
        if (data.from != nullptr && data.from->vessel != nullptr) GE::onVesselStandardModification.Fire(data.from->vessel);
        if (data.to != nullptr && data.to->vessel != nullptr) GE::onVesselStandardModification.Fire(data.to->vessel);
    }
    // FlightGlobals.cs:1258
    void SurfaceVesselEaseIn(GE::HostedFromToAction<Vessel*, Vessel_Situations> data) {
        if (data.host->easingInToSurface && data.to == Vessel_Situations::LANDED) FlightGlobals::ToggleVesselEaseIn(data.host, false);
    }
    // FlightGlobals.cs:747 OnSceneChange
    void OnSceneChange(GameScenes) { FlightGlobals::OnSceneChange(); }
    // FlightGlobals.cs:1240
    void OnGameUnpaused() { FlightGlobals::OnGameUnpaused(); }
};
FlightGlobalsHooks g_hooksObj;
bool g_hooked = false;
}  // namespace

// ============================================================ the registry

void FlightGlobals::RefreshViews() {
    ActiveVesselView = ActiveVessel != nullptr ? &ActiveVessel->FlightView() : nullptr;
    VesselsLoadedView.clear();
    for (Vessel* v : VesselsLoaded) VesselsLoadedView.push_back(&v->FlightView());
}

// FlightGlobals.cs:272
void FlightGlobals::AddVessel(Vessel* vessel) {
    if (!Ids().fetch) return;
    Vessels.push_back(vessel);
    if (vessel->loaded) VesselsLoaded.push_back(vessel);
    else VesselsUnloaded.push_back(vessel);
    Ids().persistentVesselIds.Add(vessel->persistentId, vessel);
    if (vessel->loaded) {
        for (Part* p : vessel->parts) Ids().persistentLoadedPartIds.Add(p->fields.persistentId, p);
    } else if (vessel->protoVessel != nullptr) {
        for (auto& pps : vessel->protoVessel->protoPartSnapshots) Ids().persistentUnloadedPartIds.Add(pps->persistentId, pps.get());
    }
    RefreshViews();
    GE::onFlightGlobalsAddVessel.Fire(vessel);
}

// FlightGlobals.cs:305
void FlightGlobals::RemoveVessel(Vessel* vessel) {
    if (!Ids().fetch) return;
    // The C# removes the first occurrence from the list matching `vessel.loaded`. A vessel registered twice (a host that adds what Initialize already
    // added) or whose `loaded` flag changed without SortVessel would stay behind in a list as a dangling pointer once destroyed, so every occurrence is
    // removed from all three lists (identical for every consistent registry)
    for (std::vector<Vessel*>* list : {&Vessels, &VesselsLoaded, &VesselsUnloaded}) list->erase(std::remove(list->begin(), list->end(), vessel), list->end());
    Ids().persistentVesselIds.Remove(vessel->persistentId);
    for (Part* p : vessel->parts) Ids().persistentLoadedPartIds.Remove(p->fields.persistentId);
    if (vessel->protoVessel != nullptr)
        for (auto& pps : vessel->protoVessel->protoPartSnapshots) Ids().persistentUnloadedPartIds.Remove(pps->persistentId);
    RefreshViews();
    GE::onFlightGlobalsRemoveVessel.Fire(vessel);
}

// FlightGlobals.cs:335
void FlightGlobals::SortVessel(Vessel* vessel, bool loaded) {
    if (!Ids().fetch) return;
    if (loaded) {
        if (Contains(VesselsUnloaded, vessel)) {
            EraseFirst(VesselsUnloaded, vessel);
            VesselsLoaded.push_back(vessel);
            Ids().persistentVesselIds.Add(vessel->persistentId, vessel);
            for (Part* p : vessel->parts) Ids().persistentLoadedPartIds.Add(p->fields.persistentId, p);
        }
    } else if (Contains(VesselsLoaded, vessel)) {
        EraseFirst(VesselsLoaded, vessel);
        VesselsUnloaded.push_back(vessel);
        for (auto& pps : vessel->protoVessel->protoPartSnapshots) {
            Ids().persistentLoadedPartIds.Remove(pps->persistentId);
            Ids().persistentUnloadedPartIds.Add(pps->persistentId, pps.get());
        }
    }
    RefreshViews();
}

// FlightGlobals.cs:366 (searched from the end)
Vessel* FlightGlobals::FindVessel(const Guid& id) {
    if (!Ids().fetch) return nullptr;
    int count = static_cast<int>(Vessels.size());
    while (count-- > 0)
        if (Vessels[static_cast<size_t>(count)]->id == id) return Vessels[static_cast<size_t>(count)];
    return nullptr;
}

// FlightGlobals.cs:387
bool FlightGlobals::FindVessel(uint32_t id, Vessel*& vessel) {
    if (!Ids().fetch) {
        vessel = nullptr;
        return false;
    }
    if (Ids().persistentVesselIds.Contains(id)) {
        vessel = const_cast<Vessel*>(static_cast<const Vessel*>(Ids().persistentVesselIds[id]));
        return true;
    }
    vessel = nullptr;
    return false;
}

namespace {
// FlightGlobals.cs:413 CountSpaceObjects / :427 UntrackedObjectsCount
int CountSpaceObjectsImpl(bool untrackedOnly) {
    int num = 0;
    for (int num2 = static_cast<int>(FlightGlobals::Vessels.size()) - 1; num2 >= 0; num2--) {
        Vessel* vessel = FlightGlobals::Vessels[static_cast<size_t>(num2)];
        if (!untrackedOnly || (vessel != nullptr && !vessel->GetDiscoveryInfo().HaveKnowledgeAbout(DiscoveryLevels::StateVectors))) num++;
    }
    return num;
}
}  // namespace

// FlightGlobals.cs:403
int FlightGlobals::CountUntrackedSpaceObjects() { return CountSpaceObjectsImpl(true); }
// FlightGlobals.cs:408
int FlightGlobals::CountSpaceObjects() { return CountSpaceObjectsImpl(false); }
// FlightGlobals.cs:427
int FlightGlobals::UntrackedObjectsCount() {
    int num = 0;
    for (int num2 = static_cast<int>(Vessels.size()) - 1; num2 >= 0; num2--) {
        Vessel* vessel = Vessels[static_cast<size_t>(num2)];
        if (vessel != nullptr && !vessel->GetDiscoveryInfo().HaveKnowledgeAbout(DiscoveryLevels::StateVectors)) num++;
    }
    return num;
}

// FlightGlobals.cs:441
bool FlightGlobals::FindLoadedPart(uint32_t id, Part*& partout) {
    if (Ids().fetch && id != 0) {
        if (Ids().persistentLoadedPartIds.Contains(id)) {
            partout = const_cast<Part*>(static_cast<const Part*>(Ids().persistentLoadedPartIds[id]));
            return true;
        }
        partout = nullptr;
        return false;
    }
    partout = nullptr;
    return false;
}

// FlightGlobals.cs:457
bool FlightGlobals::FindUnloadedPart(uint32_t id, ProtoPartSnapshot*& partout) {
    if (!Ids().fetch) {
        partout = nullptr;
        return false;
    }
    if (Ids().persistentUnloadedPartIds.Contains(id)) {
        partout = Ids().persistentUnloadedPartIds[id];
        return true;
    }
    partout = nullptr;
    return false;
}

// FlightGlobals.cs:1653
Part* FlightGlobals::FindPartByID(uint32_t flightID) {
    if (!Ids().fetch) return nullptr;
    if (flightID == 0) {
        log::Warning("Warning: Tried to search for a part with ID 0");
        return nullptr;
    }
    int count = static_cast<int>(VesselsLoaded.size());
    while (count-- > 0) {
        Vessel* vessel = VesselsLoaded[static_cast<size_t>(count)];
        int count2 = static_cast<int>(vessel->parts.size());
        while (count2-- > 0) {
            Part* part = vessel->parts[static_cast<size_t>(count2)];
            if (part->fields.flightID == flightID) return part;
        }
    }
    return nullptr;
}

// FlightGlobals.cs:1681
ProtoPartSnapshot* FlightGlobals::FindProtoPartByID(uint32_t flightID) {
    if (flightID == 0) {
        log::Warning("Warning: Tried to search for a part with ID 0");
        return nullptr;
    }
    int count = static_cast<int>(Vessels.size());
    while (count-- > 0) {
        Vessel* vessel = Vessels[static_cast<size_t>(count)];
        int count2 = static_cast<int>(vessel->protoVessel->protoPartSnapshots.size());
        while (count2-- > 0) {
            ProtoPartSnapshot* pps = vessel->protoVessel->protoPartSnapshots[static_cast<size_t>(count2)].get();
            if (pps->flightID == flightID) return pps;
        }
    }
    return nullptr;
}

// FlightGlobals.cs:1705
Vessel* FlightGlobals::FindNearestControllableVessel(Vessel* currentVessel) {
    Vessel* result = nullptr;
    float num = std::numeric_limits<float>::max();
    const int count = static_cast<int>(VesselsLoaded.size());
    for (int i = 0; i < count; i++) {
        Vessel* vessel = VesselsLoaded[static_cast<size_t>(i)];
        if (!vessel->packed && vessel->IsControllable() && vessel->state != Vessel::State::DEAD && vessel != currentVessel) {
            const float sqrMagnitude = (vessel->vesselTransform.position - currentVessel->vesselTransform.position).sqrMagnitude();
            if (sqrMagnitude < num) {
                num = sqrMagnitude;
                result = vessel;
            }
        }
    }
    return result;
}

// FlightGlobals.cs:1726 (LINQ Where + stable OrderBy)
std::vector<Vessel*> FlightGlobals::FindNearestVesselWhere(const Vector3d& refPos, const std::function<bool(Vessel&)>& criteria) {
    std::vector<Vessel*> list;
    for (Vessel* v : Vessels)
        if (criteria(*v)) list.push_back(v);
    if (!list.empty()) {
        std::vector<std::pair<double, Vessel*>> keyed;
        for (Vessel* v : list) keyed.emplace_back((v->GetWorldPos3D() - refPos).sqrMagnitude(), v);
        std::stable_sort(keyed.begin(), keyed.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        list.clear();
        for (auto& kv : keyed) list.push_back(kv.second);
    }
    return list;
}

// FlightGlobals.cs:776 OnDestroy (plus the test reset): the scene is gone
void FlightGlobals::Clear() {
    ActiveVessel = nullptr;
    currentMainBody = nullptr;
    Vessels.clear();
    VesselsLoaded.clear();
    VesselsUnloaded.clear();
    ActiveVesselView = nullptr;
    VesselsLoadedView.clear();
    ready = false;
    _vesselTarget = nullptr;
    vesselTargetTransform = nullptr;
    vesselTargetMode = VesselTargetModes::None;
    bodyNames.clear();
    homeBody = nullptr;
    cometPerturber = nullptr;
    physicalObjects.clear();
    disableOverrideAt_.clear();
    setPositionInProgress = false;
    GE::onVesselSituationChange.Remove(&g_hooksObj, &FlightGlobalsHooks::SurfaceVesselEaseIn);  // OnDestroy (FlightGlobals.cs:773)
    if (g_hooked) {
        GE::onGameSceneLoadRequested.Remove(&g_hooksObj, &FlightGlobalsHooks::OnSceneChange);
        GE::onGameUnpause.Remove(&g_hooksObj, &FlightGlobalsHooks::OnGameUnpaused);  // WP-19c (FlightGlobals.cs:784)
        // UnhookVesselEvents (FlightGlobals.cs:889)
        GE::onPartAttach.Remove(&g_hooksObj, &FlightGlobalsHooks::OnPartEventTargetAction);
        GE::onPartRemove.Remove(&g_hooksObj, &FlightGlobalsHooks::OnPartEventTargetAction);
        GE::onPartCouple.Remove(&g_hooksObj, &FlightGlobalsHooks::OnPartEventFromToAction);
        GE::onPartDie.Remove(&g_hooksObj, &FlightGlobalsHooks::OnPartEvent);
        GE::onPartUndock.Remove(&g_hooksObj, &FlightGlobalsHooks::OnPartEvent);
        GE::onVesselWasModified.Remove(&g_hooksObj, &FlightGlobalsHooks::OnVesselEvent);
        GE::onVesselPartCountChanged.Remove(&g_hooksObj, &FlightGlobalsHooks::OnVesselEvent);
        g_hooked = false;
    }
}

// FlightGlobals.cs:1066
void FlightGlobals::ClearAllVessels() {
    int count = static_cast<int>(Vessels.size());
    while (count-- > 0) {
        Vessel* vessel = Vessels[static_cast<size_t>(count)];
        RemoveVessel(vessel);  // UnityEngine.Object.Destroy(vessel): the owner of the C++ object frees it
    }
}

// FlightGlobals.cs:1080 / :1085
void FlightGlobals::addPhysicalObject(physicalObject* pObject) { physicalObjects.push_back(pObject); }
void FlightGlobals::removePhysicalObject(physicalObject* pObject) { EraseFirst(physicalObjects, pObject); }

// ============================================================ properties

// FlightGlobals.cs:224
bool FlightGlobals::RefFrameIsRotating() {
    if (currentMainBody == nullptr) throw NullReferenceException("FlightGlobals.currentMainBody is null");
    if (currentMainBody->rotates) return currentMainBody->inverseRotation;
    return false;
}
Vector3d FlightGlobals::upAxis() { return getUpAxis(*Active().mainBody(), Vector3d(Active().vesselTransform.position)); }  // :240
double FlightGlobals::ship_altitude() { return Active().altitude; }
double FlightGlobals::ship_verticalSpeed() { return Active().verticalSpeed; }
double FlightGlobals::ship_geeForce() { return Active().geeForce; }
Vector3d FlightGlobals::ship_velocity() { return Vector3d(Active().rb_velocity); }
Vector3d FlightGlobals::ship_obtVelocity() { return Active().obt_velocity; }
double FlightGlobals::ship_obtSpeed() { return Active().obt_speed; }
// FlightGlobals.cs:196
Vector3d FlightGlobals::ship_srfVelocity() { return Active().srf_velocity; }
double FlightGlobals::ship_srfSpeed() { return Active().srfSpeed; }
Vector3d FlightGlobals::ship_acceleration() { return Active().acceleration; }
Vector3d FlightGlobals::ship_angularVelocity() { return Vector3d(Active().angularVelocity); }
Vector3d FlightGlobals::ship_position() { return Vector3d(Active().vesselTransform.position); }
Vector3d FlightGlobals::ship_CoM() { return Vector3d(Active().CoM); }
Vector3d FlightGlobals::ship_upAxis() { return Active().upAxis; }
Vector3f FlightGlobals::ship_MOI() { return Active().MOI; }
Vector3f FlightGlobals::ship_angularMomentum() { return Active().angularMomentum; }
Orbit& FlightGlobals::ship_orbit() { return Active().orbit(); }
double FlightGlobals::ship_latitude() { return Active().latitude; }
double FlightGlobals::ship_longitude() { return Active().longitude; }
double FlightGlobals::ship_stP() { return Active().staticPressurekPa; }
Part* FlightGlobals::activeTarget() { return Active().rootPart; }

// FlightGlobals.cs:1333
CelestialBody* FlightGlobals::getMainBody() {
    if (ActiveVessel != nullptr) return ActiveVessel->orbitDriver->referenceBody();
    return getMainBody(Vector3d::zero());
}

// FlightGlobals.cs:1375
double FlightGlobals::getExternalTemperature() { return getExternalTemperature(ship_altitude()); }

// FlightGlobals.cs:1419
double FlightGlobals::getStaticPressure(const Vector3d& position) {
    if (!Vessel::env.LoadedSceneIsFlight) return 101.325;
    CelestialBody* mainBody = getMainBody(position);
    return getStaticPressure(getAltitudeAtPos(position, mainBody), mainBody);
}

// FlightGlobals.cs:1434
double FlightGlobals::getStaticPressure() {
    if (ActiveVessel != nullptr) return ActiveVessel->staticPressurekPa;
    return getStaticPressure(ship_altitude());
}

// FlightGlobals.cs:1481
Vector3d FlightGlobals::getUpAxis() { return getUpAxis(*Active().mainBody(), Vector3d(Active().vesselTransform.position)); }

// ============================================================ ClearToSave

// FlightGlobals.cs:1496 / :1501
ClearToSaveStatus FlightGlobals::ClearToSave(bool logMsg) {
    Vessel& av = Active();
    if (av.state == Vessel::State::DEAD) {
        if (logMsg) log::Info("[FlightGlobals]: Active Vessel is dead. Saving not impossible but not advisable unless leaving flight.");
        return ClearToSaveStatus::CLEAR;
    }
    if (av.isEVA() && av.evaController != nullptr && av.evaController->OnALadder()) {
        if (logMsg) log::Info("[FlightGlobals]: There are Kerbals on a ladder. Cannot save");
        return ClearToSaveStatus::NOT_WHILE_ON_A_LADDER;
    }
    if ((av.situation == Vessel::Situations::SUB_ORBITAL || av.situation == Vessel::Situations::FLYING) &&
        av.heightFromTerrain < Mathf::Max(0.5f, GameSettings::QUICKSAVE_MINIMUM_ALTITUDE) && av.heightFromTerrain != -1.0f) {
        if (logMsg) log::Info("[FlightGlobals]: Active Vessel is about to crash. Cannot save");
        return ClearToSaveStatus::NOT_WHILE_ABOUT_TO_CRASH;
    }
    if (av.situation == Vessel::Situations::FLYING) {
        if (logMsg) log::Info("[FlightGlobals]: Active Vessel is in atmosphere. Cannot save.");
        return ClearToSaveStatus::NOT_IN_ATMOSPHERE;
    }
    if (av.geeForce > 0.1 && !av.LandedOrSplashed()) {
        if (logMsg) log::Info("[FlightGlobals]: Active Vessel is under acceleration. Cannot save.");
        return ClearToSaveStatus::NOT_UNDER_ACCELERATION;
    }
    if (av.LandedOrSplashed() && av.srf_velocity.sqrMagnitude() > 0.09000000357627869) {
        if (logMsg) log::Info("[FlightGlobals]: Active Vessel is moving. Cannot save.");
        return ClearToSaveStatus::NOT_WHILE_MOVING_OVER_SURFACE;
    }
    const int count = static_cast<int>(Vessels.size());
    for (int num = 0; num < count; num++) {
        Vessel* vessel = Vessels[static_cast<size_t>(num)];
        if (vessel->isEVA() && vessel->evaController != nullptr && vessel->evaController->OnALadder()) {
            if (logMsg) log::Info("[FlightGlobals]: There are Kerbals on a ladder. Cannot save");
            return ClearToSaveStatus::NOT_WHILE_ON_A_LADDER;
        }
    }
    return ClearToSaveStatus::CLEAR;
}

// FlightGlobals.cs:1574
std::string FlightGlobals::GetNotClearToSaveStatusReason(ClearToSaveStatus status, const std::string& attempt) {
    switch (status) {
        default: return Localizer::Format("#autoLOC_7003254", {attempt});
        case ClearToSaveStatus::NOT_IN_ATMOSPHERE: return Localizer::Format("#autoLOC_7003248", {attempt});
        case ClearToSaveStatus::NOT_UNDER_ACCELERATION: return Localizer::Format("#autoLOC_7003249", {attempt});
        case ClearToSaveStatus::NOT_WHILE_MOVING_OVER_SURFACE: return Localizer::Format("#autoLOC_7003251", {attempt});
        case ClearToSaveStatus::NOT_WHILE_ABOUT_TO_CRASH: return Localizer::Format("#autoLOC_7003250", {attempt});
        case ClearToSaveStatus::NOT_WHILE_ON_A_LADDER: return Localizer::Format("#autoLOC_7003252", {attempt});
        case ClearToSaveStatus::NOT_WHILE_THROTTLED_UP: return Localizer::Format("#autoLOC_7003253", {attempt});
    }
}

// ============================================================ life cycle

// FlightGlobals.cs:724
void FlightGlobals::Awake() {
    ready = false;
    ActiveVessel = nullptr;
    RefreshViews();
}

// FlightGlobals.cs:733 (HookVesselEvents is :878)
void FlightGlobals::Start() {
    if (env.ignoreGeeForces) env.ignoreGeeForces(3.0f);
    overrideOrbit = true;
    lastVel = Vector3d::zero();
    disableOverrideAt_.push_back(Time::time + 1.0f);  // Invoke("disableOverride", 1f)
    if (!g_hooked) {
        GE::onGameSceneLoadRequested.Add(&g_hooksObj, &FlightGlobalsHooks::OnSceneChange);
        GE::onGameUnpause.Add(&g_hooksObj, &FlightGlobalsHooks::OnGameUnpaused);  // WP-19c (FlightGlobals.cs:742)
        // HookVesselEvents (FlightGlobals.cs:878)
        GE::onPartAttach.Add(&g_hooksObj, &FlightGlobalsHooks::OnPartEventTargetAction);
        GE::onPartRemove.Add(&g_hooksObj, &FlightGlobalsHooks::OnPartEventTargetAction);
        GE::onPartCouple.Add(&g_hooksObj, &FlightGlobalsHooks::OnPartEventFromToAction);
        GE::onPartDie.Add(&g_hooksObj, &FlightGlobalsHooks::OnPartEvent);
        GE::onPartUndock.Add(&g_hooksObj, &FlightGlobalsHooks::OnPartEvent);
        GE::onVesselWasModified.Add(&g_hooksObj, &FlightGlobalsHooks::OnVesselEvent);
        GE::onVesselPartCountChanged.Add(&g_hooksObj, &FlightGlobalsHooks::OnVesselEvent);
        g_hooked = true;
    }
}

// FlightGlobals.cs:747
void FlightGlobals::OnSceneChange() {
    SetSpeedMode(commandedSpeedMode);
    GE::OnFlightGlobalsReady.Fire(false);
    ready = false;
}

// FlightGlobals.cs:761
void FlightGlobals::OnLevelLoaded() {
    if (env.ignoreGeeForces) env.ignoreGeeForces(3.0f);
    lastVel.Zero();
    overrideOrbit = true;
    disableOverrideAt_.push_back(Time::time + 1.0f);  // Invoke("disableOverride", 1f)
}

// FlightGlobals.cs:1305
void FlightGlobals::disableOverride() {
    physics::CollisionEnhancer::bypass = false;
    overrideOrbit = false;
}

// Unity's delayed-call pass for Invoke("disableOverride", ...): every pending call whose time has come, in the order they were made
void FlightGlobals::RunDelayedCalls(float time) {
    std::vector<float> due;
    for (auto it = disableOverrideAt_.begin(); it != disableOverrideAt_.end();) {
        if (time >= *it) {
            due.push_back(*it);
            it = disableOverrideAt_.erase(it);
        } else {
            ++it;
        }
    }
    for (size_t i = 0; i < due.size(); i++) disableOverride();
}

// FlightGlobals.cs:789
void FlightGlobals::FixedUpdate() {
    if (ActiveVessel != nullptr) {
        UpdateInformation(true);
        if (!ready) {
            ready = true;
            GE::OnFlightGlobalsReady.Fire(true);
        }
    }
}

// FlightGlobals.cs:802
void FlightGlobals::Update() {
    if (ActiveVessel != nullptr && currentMainBody != nullptr) {
        UpdateInformation(false);
        UpdateSpeedMode();
    }
}

// FlightGlobals.cs:811 (try / catch: an exception is logged and swallowed). WP-19c: lastVel and the navball orientation block (ship_orientation_offset,
// ship_orientation, ship_heading) through the Unity-native Quaternion.LookRotation / Euler / eulerAngles of ksp/unity/quaternion_native.hpp
// (UNVERIFIED against the game, see there: approximate).
void FlightGlobals::UpdateInformation(bool fixedUpdate) {
    try {
        Vessel& av = *ActiveVessel;
        currentMainBody = av.orbitDriver->referenceBody();
        lastVel = ship_obtVelocity();
        camera_position = env.cameraPosition ? env.cameraPosition() : Vector3d(av.vesselTransform.position);
        camera_altitude = Vector3d::Distance(camera_position, currentMainBody->position) - currentMainBody->Radius;
        ship_temp = av.externalTemperature;
        ship_dns = av.atmDensity;
        // rotationOffset = Quaternion.Euler(90f, 0f, 0f) (FlightGlobals.cs:100, a static initialiser)
        static const Quaternionf rotationOffset = UnityNative::Euler(Vector3f(90.0f, 0.0f, 0.0f));
        ship_orientation_offset = rotationOffset * UnityNative::Inverse(av.ReferenceTransform().rotation);
        const Vector3d vesselPos(av.vesselTransform.position);
        const Vector3f normalized =
            Vector3f::ProjectOnPlane((currentMainBody->position + currentMainBody->transformUp * currentMainBody->Radius - vesselPos).ToVector3f(),
                                     (vesselPos - currentMainBody->position).normalized().ToVector3f())
                .normalized();
        const Vector3f upwards = (vesselPos - currentMainBody->position).normalized().ToVector3f();
        ship_orientation = ship_orientation_offset * UnityNative::LookRotation(normalized, upwards);
        ship_heading = UnityNative::EulerAngles(UnityNative::Inverse(ship_orientation)).y;
        vesselTargetDelta = Vector3f();
        vesselTargetDirection = Vector3f();
        ship_tgtVelocity = Vector3d();
        ship_tgtSpeed = 0.0;
        if (_vesselTarget != nullptr) {
            if (_vesselTarget->GetOrbitDriver() == nullptr ? false : _vesselTarget->GetOrbitDriver()->celestialBody == currentMainBody &&
                                                                          _vesselTarget->GetOrbitDriver()->celestialBody != nullptr) {
                SetVesselTarget(nullptr);
            } else if (_vesselTarget->GetOrbitDriver() != nullptr && _vesselTarget->GetOrbitDriver()->celestialBody != nullptr &&
                       currentMainBody->HasParent(_vesselTarget->GetOrbitDriver()->celestialBody)) {
                SetVesselTarget(nullptr);
            } else if (_vesselTarget->GetTransform() == nullptr) {
                SetVesselTarget(nullptr);
            } else if (!_vesselTarget->GetActiveTargetable() && _vesselTarget->GetVessel() == ActiveVessel) {
                SetVesselTarget(nullptr);
            }
        } else if (vesselTargetMode != VesselTargetModes::None) {
            SetVesselTarget(nullptr);
        }
        if (vesselTargetTransform != nullptr) {
            Part* referenceTransformPart = av.GetReferenceTransformPart();
            if (referenceTransformPart != nullptr) vesselTargetDelta = vesselTargetTransform->position - referenceTransformPart->partTransform.position;
            else vesselTargetDelta = vesselTargetTransform->position - av.vesselTransform.position;
            vesselTargetDirection = vesselTargetDelta.normalized();
            if (static_cast<int>(vesselTargetMode) >= static_cast<int>(VesselTargetModes::DirectionAndVelocity)) {
                ship_tgtVelocity = ship_obtVelocity() - Vector3d(_vesselTarget->GetObtVelocity());
                ship_tgtSpeed = ship_tgtVelocity.magnitude();
            }
        }
    } catch (const std::exception& ex) {
        log::Error(std::string("[FlightGlobals]: threw during UpdateInformation, fixed=") + (fixedUpdate ? "True" : "False") + ". Exception: " + ex.what());
    }
}

// ============================================================ target and active vessel

// FlightGlobals.cs:668 (ScreenMessages are the env.postScreenMessage seam; the orbit targeter is the map view's)
void FlightGlobals::SetVesselTarget(ITargetable* tgt, bool overrideInputLock) {
    std::string message;
    if (InputLockManager::IsUnlocked(ControlTypes::TARGETING) || overrideInputLock) {
        _vesselTarget = tgt;
        vesselTargetTransform = tgt != nullptr ? tgt->GetTransform() : nullptr;
        auto* disc = dynamic_cast<IDiscoverable*>(tgt);
        if (disc != nullptr && !disc->GetDiscoveryInfo().HaveKnowledgeAbout(DiscoveryLevels::StateVectors)) vesselTargetMode = VesselTargetModes::None;
        else if (tgt != nullptr) vesselTargetMode = tgt->GetTargetingMode();
        else vesselTargetMode = VesselTargetModes::None;
        if (ActiveVessel != nullptr) ActiveVessel->targetObject = tgt;
        if (tgt != nullptr) {
            if (disc != nullptr) message = Localizer::Format("#autoLOC_339130", {disc->RevealDisplayName()  /* DiscoveryInfo.displayName.Value: UI text, see STATUS */});
            else message = Localizer::Format("#autoLOC_339134", {tgt->GetDisplayName()});
        } else {
            message = Localizer::Format("#autoLOC_339139");
        }
    } else {
        message = Localizer::Format("#autoLOC_339144");
    }
    if (env.postScreenMessage) env.postScreenMessage(message);
}

// FlightGlobals.cs:940 / :949 / :958
bool FlightGlobals::SetActiveVessel(Vessel* v, bool clearDeadVessels) { return setActiveVessel(v, false, clearDeadVessels); }
bool FlightGlobals::SetActiveVessel(Vessel* v) { return setActiveVessel(v, false, true); }
bool FlightGlobals::ForceSetActiveVessel(Vessel* v) { return setActiveVessel(v, true, true); }

// FlightGlobals.cs:967
bool FlightGlobals::setActiveVessel(Vessel* v, bool force, bool clearDeadVessels) {
    if (v == nullptr) return false;
    if (v == ActiveVessel) return false;
    Vessel* data = ActiveVessel;
    GE::OnFlightGlobalsReady.Fire(false);
    ready = false;
    if (ActiveVessel != nullptr) {
        if (!force) {
            const char* refusal = nullptr;
            switch (ClearToSave(true)) {
                case ClearToSaveStatus::NOT_IN_ATMOSPHERE: refusal = "#autoLOC_6002346"; break;
                case ClearToSaveStatus::NOT_UNDER_ACCELERATION: refusal = "#autoLOC_6002347"; break;
                case ClearToSaveStatus::NOT_WHILE_MOVING_OVER_SURFACE: refusal = "#autoLOC_6002348"; break;
                case ClearToSaveStatus::NOT_WHILE_ABOUT_TO_CRASH: refusal = "#autoLOC_6002349"; break;
                case ClearToSaveStatus::NOT_WHILE_ON_A_LADDER: refusal = "#autoLOC_6002350"; break;
                case ClearToSaveStatus::NOT_WHILE_THROTTLED_UP: refusal = "#autoLOC_6002351"; break;
                default: break;
            }
            if (refusal == nullptr && v->GetDiscoveryInfo().Level() != DiscoveryLevels::Owned) refusal = "#autoLOC_6002352";
            if (refusal != nullptr) {
                if (env.postScreenMessage) env.postScreenMessage(Localizer::Format(refusal));
                return false;
            }
        }
        ActiveVessel->MakeInactive();
        ActiveVessel->ClearStaging();
        if (!v->loaded) {
            GE::onVesselSwitchingToUnloaded.Fire(data, v);
            // WP-19c: unset seams = the C# calls (the switch beyond the bubble is a new flight scene: FlightDriver.Start loads the saved game)
            if (env.savePersistent) env.savePersistent();
            else GamePersistence::SaveGame("persistent", HighLogic::SaveFolder(), SaveMode::OVERWRITE, GameScenes::FLIGHT);
            const auto it = std::find(Vessels.begin(), Vessels.end(), v);
            const int index = it == Vessels.end() ? -1 : static_cast<int>(it - Vessels.begin());  // vessels.IndexOf(v)
            if (env.startAndFocusVessel) {
                env.startAndFocusVessel(index);
            } else {
                Game* current = HighLogic::CurrentGame();
                if (current == nullptr) throw NullReferenceException("HighLogic.CurrentGame is null");
                Game& updated = current->Updated();
                // FlightDriver.StartAndFocusVessel(HighLogic.CurrentGame.Updated(), ...): the cache is the current game itself (shared when it is
                // shared-owned, else referenced: the C# GC keeps it either way)
                std::shared_ptr<Game> cache = updated.weak_from_this().lock();
                if (!cache) cache = std::shared_ptr<Game>(&updated, [](Game*) {});
                FlightDriver::StartAndFocusVessel(cache, index);
            }
            return true;
        }
        if (v->packed) {
            const int count = static_cast<int>(Vessels.size());
            for (int i = 0; i < count; i++) Vessels[static_cast<size_t>(i)]->GoOnRails();
        }
    }
    GE::onVesselSwitching.Fire(data, v);
    ActiveVessel = v;
    currentMainBody = v->orbit().referenceBody;
    RefreshViews();
    OrbitPhysicsManager::CheckReferenceFrame();
    OrbitPhysicsManager::HoldVesselUnpack();
    if (env.flightCameraSetTarget) env.flightCameraSetTarget(v);
    v->MakeActive();
    if (clearDeadVessels) ClearDeadVessels();
    GE::onVesselChange.Fire(v);
    return true;
}

// FlightGlobals.cs:1052
void FlightGlobals::ClearDeadVessels() {
    int count = static_cast<int>(Vessels.size());
    while (count-- > 0) {
        Vessel* vessel = Vessels[static_cast<size_t>(count)];
        if (vessel->state == Vessel::State::DEAD) RemoveVessel(vessel);  // UnityEngine.Object.Destroy(vessel): the owner frees the object
    }
}

// FlightGlobals.cs:1266
void FlightGlobals::ToggleVesselEaseIn(Vessel* vessel, bool enableEaseIn, double easeInMultiplier) {
    if (enableEaseIn) {
        if (!vessel->easingInToSurface) {
            vessel->easeInMultiplier = easeInMultiplier;
            vessel->gravityMultiplier *= easeInMultiplier;
            vessel->easingInToSurface = true;
            GE::onVesselSituationChange.Add(&g_hooksObj, &FlightGlobalsHooks::SurfaceVesselEaseIn);
        } else {
            vessel->gravityMultiplier /= vessel->easeInMultiplier;
            vessel->easeInMultiplier = easeInMultiplier;
            vessel->gravityMultiplier *= easeInMultiplier;
        }
    } else if (vessel->easingInToSurface) {
        vessel->gravityMultiplier /= vessel->easeInMultiplier;
        vessel->easingInToSurface = false;
        GE::onVesselSituationChange.Remove(&g_hooksObj, &FlightGlobalsHooks::SurfaceVesselEaseIn);
    }
}

// ============================================================ speed display

// FlightGlobals.cs:1932
void FlightGlobals::UpdateSpeedMode() {
    Vessel& av = Active();
    if (commandedSpeedMode == SpeedDisplayModes::Target && _vesselTarget == nullptr) {
        commandedSpeedMode = SpeedDisplayModes::Orbit;
        SetSpeedMode(commandedSpeedMode);
    } else if (commandedSpeedMode != SpeedDisplayModes::Target && _vesselTarget != nullptr &&
               static_cast<int>(vesselTargetMode) >= static_cast<int>(VesselTargetModes::DirectionAndVelocity) &&
               (av.vesselTransform.position - _vesselTarget->GetTransform()->position).sqrMagnitude() < TargetSwitchSqrThresh &&
               (!av.Autopilot().Enabled() || av.Autopilot().Mode() == VesselAutopilot_AutopilotMode::StabilityAssist ||
                static_cast<int>(av.Autopilot().Mode()) >= static_cast<int>(VesselAutopilot_AutopilotMode::Target))) {
        commandedSpeedMode = SpeedDisplayModes::Target;
        SetSpeedMode(commandedSpeedMode);
    } else if (commandedSpeedMode == SpeedDisplayModes::Surface) {
        if (ship_altitude() > currentMainBody->Radius * static_cast<double>(currentMainBody->navballSwitchRadiusMult)) {
            commandedSpeedMode = SpeedDisplayModes::Orbit;
            SetSpeedMode(commandedSpeedMode);
        }
    } else if (commandedSpeedMode == SpeedDisplayModes::Orbit &&
               ship_altitude() < currentMainBody->Radius * static_cast<double>(currentMainBody->navballSwitchRadiusMultLow)) {
        commandedSpeedMode = SpeedDisplayModes::Surface;
        SetSpeedMode(commandedSpeedMode);
    }
}

// FlightGlobals.cs:1978
double FlightGlobals::GetDisplaySpeed() {
    switch (speedMode) {
        default: return ship_obtSpeed();
        case SpeedDisplayModes::Surface: return ship_srfSpeed();
        case SpeedDisplayModes::Target: return ship_tgtSpeed;
    }
}

// FlightGlobals.cs:1991
Vector3d FlightGlobals::GetDisplayVelocity() {
    switch (speedMode) {
        default: return Active().obt_velocity;
        case SpeedDisplayModes::Surface: return Active().srf_velocity;
        case SpeedDisplayModes::Target: return ship_tgtVelocity;
    }
}

}  // namespace ksp
