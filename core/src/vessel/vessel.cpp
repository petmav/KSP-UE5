// Vessel (Vessel.cs): identity, parts, reference transform, situations, crew, control level, modules, naming.
// The state transitions live in vessel_transitions.cpp, kinematics / mass / atmosphere in vessel_motion.cpp.
#include "ksp/commnet/comm_net_vessel.hpp"
#include "ksp/commnet/commnet_scene.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_autopilot.hpp"

#include <algorithm>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/util/enum_extensions.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/module_trip_logger.hpp"
#include <limits>

namespace ksp {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

// ============================================================ FlightMath

// FlightGlobals.cs:1471
double FlightMath::getAltitudeAtPos(const Vector3d& position, const CelestialBody& body) {
    return Vector3d::Distance(position, body.position) - body.Radius;
}

// FlightGlobals.cs:1491
Vector3d FlightMath::getUpAxis(const CelestialBody& body, const Vector3d& position) { return (position - body.position).normalized(); }

// FlightGlobals.cs:1435
double FlightMath::getStaticPressure(const Vector3d& position, const CelestialBody& body) {
    return body.GetPressure(getAltitudeAtPos(position, body));
}

// ============================================================ construction (Awake)

// Part.vessel hooks: what Part.cs asks its Vessel
void Vessel::InstallPartHooks() {
    Part::vesselHooks.isEVA = [](const Vessel& v) { return v.isEVA(); };
    Part::vesselHooks.partCount = [](const Vessel& v) { return static_cast<int>(v.parts.size()); };
    Part::vesselHooks.removeCrew = [](Vessel& v, const std::shared_ptr<ProtoCrewMember>& c) { v.RemoveCrew(c); };
    Part::vesselHooks.crewListSetDirty = [](Vessel& v) { v.CrewListSetDirty(); };
    // ProtoPartModuleSnapshot: !vessel.loaded || vessel.isUnloading || vessel.isBackingUp
    Part::vesselHooks.snapshotSavesModules = [](const Vessel& v) { return !v.loaded || v.isUnloading || v.isBackingUp; };
}

// IOrbitDriverVessel over a Vessel (OrbitDriver.cs reads the Vessel through these members)
class VesselOrbitDriverView final : public IOrbitDriverVessel {
public:
    explicit VesselOrbitDriverView(Vessel& v) : v_(v) {}
    Vessel* AsVessel() override { return &v_; }
    std::string GetDisplayName() const override { return v_.GetDisplayName(); }
    // OrbitDriver.cs:281: (QuaternionD)driverTransform.rotation * (Vector3d)vessel.localCoM
    Vector3d RotatedLocalCoM() const override {
        const Quaternionf r = v_.vesselTransform.rotation;
        return QuaternionD(static_cast<double>(r.x), static_cast<double>(r.y), static_cast<double>(r.z), static_cast<double>(r.w)) *
               Vector3d(v_.localCoM);
    }
    void SetPosition(const Vector3d& position) override { v_.SetPosition(position); }
    // updateFromParameters, NaN orbit: vessel.Unload(); Object.Destroy(vessel.gameObject)
    void UnloadAndDestroy() override {
        v_.Unload();
        v_.state = VesselState::DEAD;
    }
    Vector3f TransformPosition() const override { return v_.vesselTransform.position; }
    bool HasRootPartRigidbody() const override { return v_.rootPart != nullptr && v_.rootPart->hasRigidbody; }
    bool RootPartRigidbodyIsKinematic() const override { return false; }  // Rigidbody.isKinematic: the physics engine's
    Vector3d CoMD() const override { return v_.CoMD; }
    Vector3d velocityD() const override { return v_.velocityD; }
    // OrbitDriver.RecalculateOrbit: ModuleTripLogger.ForceVesselLog(vessel, Flyby, newReferenceBody)
    void ForceVesselLogFlyby(CelestialBody* body) override { ModuleTripLogger::ForceVesselLog(&v_, FlightLog::EntryType::Flyby, body); }
    // OrbitDriver.cs:75 Targetable: `if (vessel != null) return vessel;` (the Vessel's ITargetable, Vessel.cs:3761-3815)
    ITargetable* AsTargetable() override { return &v_; }

private:
    Vessel& v_;
};

// IFlightVessel over a Vessel: the Vessel members TimeWarp.cs reads through FlightGlobals.ActiveVessel / VesselsLoaded
class VesselFlightView final : public IFlightVessel {
public:
    explicit VesselFlightView(Vessel& v) : v_(v) {}
    bool LandedOrSplashed() const override { return v_.LandedOrSplashed(); }  // Vessel.cs:590
    // Vessel.cs:586 orbitDriver.orbit.referenceBody (null while the Vessel has no orbit driver)
    CelestialBody* mainBody() const override {
        Orbit* o = v_.GetOrbit();
        return o != nullptr ? o->referenceBody : nullptr;
    }
    Vector3f TransformPosition() const override { return v_.vesselTransform.position; }  // vessel.transform.position
    Orbit* orbit() const override { return v_.GetOrbit(); }                              // Vessel.cs:584
    double altitude() const override { return v_.altitude; }
    double geeForce() const override { return v_.geeForce; }
    Vector3d srf_velocity() const override { return v_.srf_velocity; }
    bool PatchedConicsAttached() const override { return v_.PatchedConicsAttached(); }  // Vessel.cs:795
    bool isEVA() const override { return v_.isEVA(); }                                  // Vessel.cs:737
    // TimeWarp.cs:511 vessel.GetComponent<KerbalEVA>().OnALadder (KerbalEVA.cs:1205); no KerbalEVA component: false
    bool OnALadder() const override { return v_.evaController != nullptr && v_.evaController->OnALadder(); }

private:
    Vessel& v_;
};

// Vessel.cs:1520 Awake
Vessel::Vessel() {
    static bool hooksInstalled = (InstallPartHooks(), true);
    (void)hooksInstalled;
    vesselTransform = PartTransform{};
    if (FlightGlobals::bodies != nullptr) SetStaticPressureAndExternalTemp();
    framesAtStartup_ = env.frameCount;
    accelSmoothing_.assign(static_cast<size_t>(accelSmoothingLength_), Vector3d());
    accelSmoothingCursor_ = 0;
    accelSmoothingLengthRecip_ = 1.0 / static_cast<double>(accelSmoothingLength_);
    actionGroups_ = std::make_unique<ActionGroupList>(this);
    OverrideDefault.assign(static_cast<size_t>(NumOverrideGroups), false);
    OverrideActionControl.assign(static_cast<size_t>(NumOverrideGroups), KSPActionGroup::None);
    OverrideAxisControl.assign(static_cast<size_t>(NumOverrideGroups), KSPAxisGroup::None);
    OverrideGroupNames.assign(static_cast<size_t>(NumOverrideGroups), std::nullopt);
    setControlStates.push_back(&ctrlState);
    for (int j = 1; j < NumOverrideGroups + 1; j++) {
        ownedControlStates_.push_back(std::make_unique<FlightCtrlState>(ctrlState.custom_axes));
        setControlStates.push_back(ownedControlStates_.back().get());
    }
    vesselValues_ = std::make_unique<VesselValues>(*this);
    autopilot_ = std::make_unique<VesselAutopilot>(*this);  // Vessel.cs:1553
    discoveryInfo_ = std::make_unique<DiscoveryInfo>(this);
    if (PhysicsGlobals::Instance() != nullptr) vesselRanges = VesselRanges(PhysicsGlobals::Instance()->VesselRangesDefault);
    VesselAnchorExceptions = {"ModuleWheelBase", "LaunchClamp", "FlagSite", "ModuleGroundPart"};
    GE::onVesselPrecalcAssign.Fire(this);
    GE::onCrewTransferred.Add(this, &Vessel::onCrewTransferred);
    // Vessel.cs:1573-1577 (WP-16: the part-count change of a decouple / couple rebuilds the PartSets of the vessel that lost / gained parts)
    GE::onVesselPartCountChanged.Add(this, &Vessel::OnVesselPartCountChanged);
    GE::onPartResourceListChange.Add(this, &Vessel::UpdateResourceSetsEventCheckPart);
    GE::onPartCrossfeedStateChange.Add(this, &Vessel::UpdateResourceSetsEventCheckPart);
    GE::onPartFuelLookupStateChange.Add(this, &Vessel::UpdateCrossfeedResourceSetFL);
    GE::onPartPriorityChanged.Add(this, &Vessel::UpdateResourceSetsEventCheckPart);
    GE::onPartVesselNamingChanged.Add(this, &Vessel::OnPartVesselNamingChanged);
    GE::onVesselWasModified.Add(this, &Vessel::OnVesselNamingVesselWasModified);
    VesselModuleManager::AddModulesToVessel(this, vesselModules);
    orbitDriverView_ = std::make_unique<VesselOrbitDriverView>(*this);
    flightView_ = std::make_unique<VesselFlightView>(*this);
}

// Vessel.cs:3341 OnDestroy (event listeners; the registry entry)
Vessel::~Vessel() {
    GE::onVesselPartCountChanged.Remove(this, &Vessel::OnVesselPartCountChanged);  // Vessel.cs:3343-3347
    GE::onPartResourceListChange.Remove(this, &Vessel::UpdateResourceSetsEventCheckPart);
    GE::onPartCrossfeedStateChange.Remove(this, &Vessel::UpdateResourceSetsEventCheckPart);
    GE::onPartFuelLookupStateChange.Remove(this, &Vessel::UpdateCrossfeedResourceSetFL);
    GE::onPartPriorityChanged.Remove(this, &Vessel::UpdateResourceSetsEventCheckPart);
    GE::onPartVesselNamingChanged.Remove(this, &Vessel::OnPartVesselNamingChanged);
    GE::onVesselWasModified.Remove(this, &Vessel::OnVesselNamingVesselWasModified);
    GE::onCrewTransferred.Remove(this, &Vessel::onCrewTransferred);
    precalc.OnDestroy();  // VesselPrecalculate.OnDestroy (a component of the same GameObject)
    FlightRegistry::RemoveVessel(this);
    if (FlightRegistry::ActiveVessel == this) FlightRegistry::ActiveVessel = nullptr;
    for (Part* p : parts)
        if (p != nullptr && p->vessel == this) p->vessel = nullptr;
}

// Vessel.cs:1583
void Vessel::SetStaticPressureAndExternalTemp() {
    CelestialBody* celestialBody = FlightGlobals::getMainBody(Vector3d(vesselTransform.position));
    staticPressurekPa = FlightMath::getStaticPressure(Vector3d(vesselTransform.position), *celestialBody);
    if (staticPressurekPa > 0.0) {
        externalTemperature = atmosphericTemperature =
            env.fullTemperatureAtPosition ? env.fullTemperatureAtPosition(*celestialBody, Vector3d(vesselTransform.position)) : -1.0;
    } else {
        externalTemperature = atmosphericTemperature = PhysicsGlobals::SpaceTemperature();
    }
}

// Vessel.cs:1597
void Vessel::Start() {
    if (orbitDriver != nullptr && orbitDriver->orbit != nullptr) lastBody = orbit().referenceBody;
    RebuildCrewList();
    UpdateVesselModuleActivation();
    if (!loaded && FlightGlobals::bodies != nullptr) SetStaticPressureAndExternalTemp();
}

// Vessel.cs:1634
bool Vessel::PhysicsHoldExpired() {
    if (useFramesAtStartupOverride_) {
        useFramesAtStartupOverride_ = env.frameCount - framesAtStartupOverride_ < hold_count_override_;
        return !useFramesAtStartupOverride_;
    }
    const int num = Landed ? 75 : 25;
    return env.frameCount - framesAtStartup_ >= num;
}

// Vessel.cs:1645
void Vessel::SetPhysicsHoldExpiryOverride(int newHoldCountValue) {
    framesAtStartup_ = 0;
    framesAtStartupOverride_ = env.frameCount;
    hold_count_override_ = newHoldCountValue;
    useFramesAtStartupOverride_ = true;
}

// ============================================================ indexers and properties

// Vessel.cs:538
Part* Vessel::byFlightID(uint32_t flightID) {
    int count = static_cast<int>(parts.size());
    while (count-- > 0)
        if (parts[static_cast<size_t>(count)]->fields.flightID == flightID) return parts[static_cast<size_t>(count)];
    return nullptr;
}

// Vessel.cs:558
Part* Vessel::byIndex(int index) {
    if (index >= 0 && static_cast<int>(parts.size()) > index) return parts[static_cast<size_t>(index)];
    return nullptr;
}

// Vessel.cs:572 (Part.GetReferenceTransform is the part's transform; the model reference transform is the model reader's)
PartTransform& Vessel::ReferenceTransform() {
    if (referenceTransformPart_ == nullptr) return vesselTransform;
    return referenceTransformPart_->partTransform;
}

// Vessel.cs:604
bool Vessel::LandedInKSC() const {
    const std::string& text = loaded ? landedAt : protoVessel->landedAt;
    return text.find("KSC") != std::string::npos || text.find("LaunchPad") != std::string::npos ||
           text.find("Runway") != std::string::npos;
}

// Vessel.cs:644
bool Vessel::isPersistent() {
    if (loaded) {
        for (Part* p : parts)
            if (p->fields.isPersistent) {
                persistent_ = true;
                return true;
            }
        persistent_ = false;
        return false;
    }
    return persistent_;
}

// Vessel.cs:679
bool Vessel::IsRecoverable() {
    if (LandedOrSplashed()) return mainBody()->isHomeWorld;
    return false;
}

// Vessel.cs:691
Vessel::Situations Vessel::BestSituation() const {
    if (lastSituation_ == Situations::FLYING) return lastSituation_;
    return situation;
}

// Vessel.cs:739
Vessel* Vessel::EVALadderVessel() {
    if (isEVA() && evaController != nullptr && evaController->LadderPart() != nullptr && evaController->LadderPart()->vessel != nullptr)
        return evaController->LadderPart()->vessel;
    return this;
}

// Vessel.cs:804-868 (obsolete range accessors)
void Vessel::set_distancePackThreshold(float v) {
    vesselRanges.orbit.pack = v;
    vesselRanges.subOrbital.pack = v;
    vesselRanges.flying.pack = v;
    vesselRanges.escaping.pack = v;
}
void Vessel::set_distanceUnpackThreshold(float v) {
    vesselRanges.orbit.unpack = v;
    vesselRanges.subOrbital.unpack = v;
    vesselRanges.flying.unpack = v;
    vesselRanges.escaping.unpack = v;
}
void Vessel::set_distanceLandedPackThreshold(float v) {
    vesselRanges.landed.pack = v;
    vesselRanges.splashed.pack = v;
    vesselRanges.prelaunch.pack = v;
}
void Vessel::set_distanceLandedUnpackThreshold(float v) {
    vesselRanges.landed.unpack = v;
    vesselRanges.splashed.unpack = v;
    vesselRanges.prelaunch.unpack = v;
}

// ============================================================ parts, reference transform

// Vessel.cs:909
Part* Vessel::PartsContain(uint32_t partPersistentId) {
    int count = static_cast<int>(parts.size());
    while (count-- > 0)
        if (parts[static_cast<size_t>(count)]->fields.persistentId == partPersistentId) return parts[static_cast<size_t>(count)];
    return nullptr;
}

// Vessel.cs:931 (GameEvents.onVesselReferenceTransformSwitch carries Unity Transforms; kspcore has none: nullptr, nullptr)
void Vessel::SetReferenceTransform(Part* p, bool storeRecall) {
    if (p != nullptr) {
        GE::onVesselReferenceTransformSwitch.Fire(nullptr, nullptr);
        referenceTransformId = p->fields.flightID;
        referenceTransformPart_ = p;
    } else {
        GE::onVesselReferenceTransformSwitch.Fire(nullptr, nullptr);
        referenceTransformId = 0u;
        referenceTransformPart_ = nullptr;
    }
    if (storeRecall) {
        referenceTransformIdRecall = referenceTransformId;
        referenceTransformPartRecall_ = referenceTransformPart_;
    }
}

// Vessel.cs:952
void Vessel::RecallReferenceTransform() {
    if ((referenceTransformPartRecall_ == nullptr && referenceTransformIdRecall == 0) ||
        (referenceTransformPartRecall_ == nullptr ? throw NullReferenceException() : referenceTransformPartRecall_->vessel == this)) {
        SetReferenceTransform(referenceTransformPartRecall_);
        return;
    }
    referenceTransformIdRecall = referenceTransformId;
    referenceTransformPartRecall_ = referenceTransformPart_;
}

// Vessel.cs:963
Part* Vessel::FindReferenceTransform(uint32_t refId) {
    Part* part = byFlightID(refId);
    if (part == nullptr) {
        log::Warning("Couldn't find part with id " + std::to_string(refId));
        referenceTransformId = 0u;
        return nullptr;
    }
    return part;
}

namespace {
// ShipConstruction.cs:447 findFirstCrewablePart
Part* FindFirstCrewablePart(Part* part) {
    if (part->fields.CrewCapacity > 0 && !part->protoModuleCrew.empty() && part->fields.isControlSource > 0) return part;
    for (Part* child : part->children) {
        Part* p = FindFirstCrewablePart(child);
        if (p != nullptr) return p;
    }
    return nullptr;
}
}  // namespace

// Vessel.cs:975
void Vessel::FallBackReferenceTransform() {
    if (byFlightID(referenceTransformId) == nullptr) SetReferenceTransform(rootPart != nullptr ? FindFirstCrewablePart(rootPart) : nullptr);
}

// Vessel.cs:983
Orbit* Vessel::GetCurrentOrbit() {
    if (orbitDriver != nullptr) return &orbit();
    log::Error("Vessel.GetCurrentOrbit: orbit driver is null!");
    return nullptr;
}

// Vessel.cs:996
Vector3d Vessel::GetWorldPos3D() {
    if (!precalc.firstStatsRunComplete) precalc.CalculatePhysicsStats();
    return CoMD;
}

// Vessel.cs:1005
void Vessel::SetLoaded(bool loaded_) {
    loaded = loaded_;
    UpdateVesselModuleActivation();
    FlightRegistry::SortVessel(this, loaded_);
}

// ============================================================ situations

// Vessel.cs:1015
bool Vessel::checkLanded() {
    if (loaded && !packed && !precalc.isEasingGravity()) {
        Landed = false;
        for (Part* part : parts)
            if (part->fields.GroundContact || part->fields.PermanentGroundContact) {
                Landed = true;
                break;
            }
        if (!Landed) SetLandedAt("");
        if (Landed && mainBody() != nullptr && env.pqsLevels) {
            int mn = PQSminLevel, mx = PQSmaxLevel;
            if (env.pqsLevels(*mainBody(), mn, mx)) {
                PQSminLevel = mn;
                PQSmaxLevel = mx;
            }
        }
    }
    return Landed;
}

// Vessel.cs:1044
bool Vessel::checkSplashed() {
    if (loaded) {
        Splashed = false;
        for (Part* part : parts)
            if (part->fields.WaterContact) {
                Splashed = true;
                Landed = false;
                break;
            }
    }
    return Splashed;
}

namespace {
// MiniBiome.cs:40 ConvertTagtoLandedAt
std::string ConvertTagtoLandedAt(const std::string& tagname) {
    if (tagname.empty()) return std::string();
    auto has = [&](const char* s) { return tagname.find(s) != std::string::npos; };
    auto replaceAll = [](std::string s, const std::string& a, const std::string& b) {
        size_t pos = 0;
        while ((pos = s.find(a, pos)) != std::string::npos) {
            s.replace(pos, a.size(), b);
            pos += b.size();
        }
        return s;
    };
    if (!has("Pad") && !has("LaunchPad")) {
        if (has("Runway")) return "Runway";
        std::string text = tagname;
        if (has("KSC") && tagname != "KSC") text = replaceAll(text, "KSC", "");
        if (has("Grounds") && tagname != "Grounds") text = replaceAll(text, "Grounds", "");
        std::replace(text.begin(), text.end(), '_', ' ');
        return dotnet::Trim(text);
    }
    return "LaunchPad";
}
}  // namespace

// Vessel.cs:3532
std::string Vessel::GetLandedAtString(const std::string& landedAt_) { return ConvertTagtoLandedAt(landedAt_); }

// Vessel.cs:1185 (the GameObject `gO` of C# is its Unity tag; the ROC branch is the Missions DLC's)
void Vessel::SetLandedAt(const std::string& landedAt_, const std::string& gameObjectTag, const std::string& inputdisplaylandedAt) {
    if (this->landedAt == landedAt_ || landedAt_ == "Untagged") return;
    this->landedAt = landedAt_;
    displaylandedAt = landedAt_;
    if (!landedAt_.empty()) landedAtLast = landedAt_;
    if (inputdisplaylandedAt != "") {
        displaylandedAt = inputdisplaylandedAt;
        return;
    }
    if (!(landedAt_ != "")) return;
    bool isFacility = false;
    displaylandedAt = "";
    bool isSite = env.isFacilityOrLaunchSite ? env.isFacilityOrLaunchSite(landedAt_, isFacility, displaylandedAt) : false;
    if (!isSite && !gameObjectTag.empty() && env.miniBiomeDisplayNameByUnityTag)
        displaylandedAt = env.miniBiomeDisplayNameByUnityTag(gameObjectTag);
    if (displaylandedAt.empty()) {
        const std::string biome = GetLandedAtString(landedAt_);
        displaylandedAt = (env.biomeDisplayName && mainBody() != nullptr) ? env.biomeDisplayName(*mainBody(), biome) : biome;
    }
}

// Vessel.cs:3133
void Vessel::updateSituation() {
    bool flag;
    Vessel* ladderVessel = EVALadderVessel();
    if (ladderVessel != this) {
        situation = evaController->LadderPart()->vessel->situation;
        flag = true;
    } else {
        flag = false;
        if (Landed) {
            if (situation == Situations::PRELAUNCH && (!(srfSpeed > 2.5) || precalc.isEasingGravity() || vesselSpawning))
                situation = Situations::PRELAUNCH;
            else
                situation = Situations::LANDED;
        } else if (Splashed) {
            situation = Situations::SPLASHED;
        } else if (staticPressurekPa > 0.0) {
            situation = Situations::FLYING;
        } else if (orbit().eccentricity < 1.0 && orbit().ApR() < mainBody()->sphereOfInfluence) {
            if (orbit().PeA() < (mainBody()->atmosphere ? mainBody()->atmosphereDepth : 0.0))
                situation = Situations::SUB_ORBITAL;
            else
                situation = Situations::ORBITING;
        } else {
            situation = Situations::ESCAPING;
        }
    }
    if (situation != lastSituation_) {
        GE::onVesselSituationChange.Fire(GE::HostedFromToAction<Vessel*, Vessel_Situations>(this, lastSituation_, situation));
        lastSituation_ = situation;
    }
    if (wasLadder_ != flag) wasLadder_ = flag;
}

// Vessel.cs:3190
void Vessel::UpdateLandedSplashed() {
    checkLanded();
    checkSplashed();
    updateSituation();
}

// Vessel.cs:3466 (the cacheAutoLOC_* strings are Localizer.Format of the Description attribute tags of Vessel.Situations)
std::string Vessel::GetSituationString(Situations s) {
    switch (s) {
        case Situations::SUB_ORBITAL: return Localizer::Format("#autoLOC_348562");
        case Situations::FLYING: return Localizer::Format("#autoLOC_348558");
        case Situations::LANDED: return Localizer::Format("#autoLOC_348559");
        case Situations::SPLASHED: return Localizer::Format("#autoLOC_348561");
        case Situations::PRELAUNCH: return Localizer::Format("#autoLOC_348557");
        default: return Localizer::Format("#autoLOC_348565");
        case Situations::DOCKED: return Localizer::Format("#autoLOC_348564");
        case Situations::ESCAPING: return Localizer::Format("#autoLOC_348563");
        case Situations::ORBITING: return Localizer::Format("#autoLOC_348560");
    }
}

namespace {
// System.Guid.CompareTo (reference source: _a as uint, _b, _c as ushort, then the bytes d..k)
int GuidCompareTo(const Guid& x, const Guid& y) {
    auto u32 = [](const Guid& g) { return static_cast<uint32_t>(g.bytes[0]) | static_cast<uint32_t>(g.bytes[1]) << 8 | static_cast<uint32_t>(g.bytes[2]) << 16 | static_cast<uint32_t>(g.bytes[3]) << 24; };
    auto u16 = [](const Guid& g, int o) { return static_cast<uint32_t>(g.bytes[o]) | static_cast<uint32_t>(g.bytes[o + 1]) << 8; };
    if (u32(x) != u32(y)) return u32(x) < u32(y) ? -1 : 1;
    if (u16(x, 4) != u16(y, 4)) return u16(x, 4) < u16(y, 4) ? -1 : 1;
    if (u16(x, 6) != u16(y, 6)) return u16(x, 6) < u16(y, 6) ? -1 : 1;
    for (int i = 8; i < 16; i++)
        if (x.bytes[i] != y.bytes[i]) return x.bytes[i] < y.bytes[i] ? -1 : 1;
    return 0;
}
}  // namespace

// Vessel.cs:2235
Vessel* Vessel::GetDominantVessel(Vessel* v1, Vessel* v2) {
    if (static_cast<int>(v1->vesselType) > static_cast<int>(v2->vesselType)) return v1;
    if (static_cast<int>(v1->vesselType) < static_cast<int>(v2->vesselType)) return v2;
    v1->GetTotalMass();
    v2->GetTotalMass();
    if (std::abs(v1->totalMass - v2->totalMass) < 0.009999999776482582) {
        if (GuidCompareTo(v1->id, v2->id) > 0) return v1;
        return v2;
    }
    if (v1->totalMass > v2->totalMass) return v1;
    return v2;
}

// ============================================================ crew

// Vessel.cs:1234
const std::vector<Vessel::PCM>& Vessel::GetVesselCrew() {
    if (loaded) {
        if (crewCachedPartCount_ != static_cast<int>(parts.size())) RebuildCrewList();
        return crew_;
    }
    return protoVessel->GetVesselCrew();
}

// Vessel.cs:1252 (the unloaded branch appends to `crew` without clearing it -- sic)
void Vessel::RebuildCrewList() {
    if (loaded) {
        crew_.clear();
        crewableParts = 0;
        crewedParts = 0;
        crewCachedPartCount_ = static_cast<int>(parts.size());
        for (int i = 0; i < crewCachedPartCount_; i++) {
            Part* part = parts[static_cast<size_t>(i)];
            const int count = static_cast<int>(part->protoModuleCrew.size());
            if (count > 0) {
                crewedParts++;
                crewableParts++;
                for (int j = 0; j < count; j++) crew_.push_back(part->protoModuleCrew[static_cast<size_t>(j)]);
            } else if (part->fields.CrewCapacity > 0) {
                crewableParts++;
            }
        }
    } else {
        const std::vector<PCM>& vesselCrew = protoVessel->GetVesselCrew();
        for (const PCM& c : vesselCrew) crew_.push_back(c);
        crewCachedPartCount_ = static_cast<int>(protoVessel->protoPartSnapshots.size());
        crewedParts = protoVessel->crewedParts;
        crewableParts = protoVessel->crewableParts;
    }
}

// Vessel.cs:1293
bool Vessel::RemoveCrew(const PCM& pcm) {
    auto it = std::find(crew_.begin(), crew_.end(), pcm);
    if (it == crew_.end()) return false;
    crew_.erase(it);
    return true;
}

// Vessel.cs:1298
bool Vessel::RemoveCrew(const std::string& name) {
    int count = static_cast<int>(crew_.size());
    while (count-- > 0)
        if (crew_[static_cast<size_t>(count)]->name() == name) {
            crew_.erase(crew_.begin() + count);
            return true;
        }
    return false;
}

// Vessel.cs:1313
void Vessel::RemoveCrewList(const std::vector<PCM>& pcmList, bool updatePartCount) {
    if (updatePartCount) crewCachedPartCount_ = static_cast<int>(parts.size());
    int num = 0;
    const int count = static_cast<int>(pcmList.size());
    int count2 = static_cast<int>(crew_.size());
    while (count2-- > 0) {
        int index = count;
        while (index-- > 0) {
            if (crew_[static_cast<size_t>(count2)] == pcmList[static_cast<size_t>(index)]) {
                crew_.erase(crew_.begin() + count2);
                num++;
                break;
            }
        }
        if (num == count) break;
    }
}

// Vessel.cs:1341
int Vessel::GetCrewCapacity() {
    int num = 0;
    for (Part* part : parts) num += part->fields.CrewCapacity;
    return num;
}

// Vessel.cs:1358
void Vessel::CrewWasModified(Vessel* vessel) {
    if (vessel != nullptr) {
        vessel->RebuildCrewList();
        GE::onVesselCrewWasModified.Fire(vessel);
    }
}

// Vessel.cs:1367
void Vessel::CrewWasModified(Vessel* vessel1, Vessel* vessel2) {
    CrewWasModified(vessel1);
    CrewWasModified(vessel2);
}

// Vessel.cs:1373
void Vessel::onCrewTransferred(GE::HostedFromToAction<ProtoCrewMember*, Part*>) { RebuildCrewList(); }

// Vessel.cs:3376 (ProtoCrewMember.Die and the log line)
void Vessel::MurderCrew() {
    std::vector<PCM> vesselCrew = GetVesselCrew();
    if (!vesselCrew.empty()) {
        for (const PCM& c : vesselCrew) c->Die();
        CrewWasModified(this);
    }
}

// ============================================================ control

// Vessel.cs:1378
ControlLevel Vessel::GetControlLevel() {
    if (isEVA() && !crew_.empty()) {
        if (!crew_[0]->inactive()) return ControlLevel::PARTIAL_MANNED;
        return ControlLevel::NONE;
    }
    if (env.commNetControlLevel) {
        if (std::optional<ControlLevel> l = env.commNetControlLevel(*this)) return *l;
    }
    if (connection != nullptr && CommNet::CommNetScenario::Instance() != nullptr && CommNet::CommNetScenario::CommNetEnabled())
        return connection->GetControlLevel();
    int level = 0;
    int count = static_cast<int>(parts.size());
    while (count-- > 0) {
        const int c = parts[static_cast<size_t>(count)]->fields.isControlSource;
        if (c > level) level = c;
    }
    return static_cast<ControlLevel>(level);
}

// Vessel.cs:1405 (InputLockManager locks are UI state: only the control-lock bookkeeping flags)
void Vessel::CheckControllable() {
    if (!loaded) {
        isControllable_ = false;
        controlLevel_ = ControlLevel::NONE;
        return;
    }
    controlLevel_ = GetControlLevel();
    if (static_cast<int>(maxControlLevel) < static_cast<int>(controlLevel_)) controlLevel_ = maxControlLevel;
    const bool flag = isControllable_;
    isControllable_ = controlLevel_ > ControlLevel::NONE;
    if (isControllable_ != flag) GE::onVesselControlStateChange.Fire(this, isControllable_);
    if (state != State::ACTIVE) return;
    switch (controlLevel_) {
        case ControlLevel::PARTIAL_UNMANNED:
            if (controlLockSet_ && !controlLockPartial_) controlLockSet_ = false;
            if (!controlLockSet_) {
                controlLockSet_ = true;
                controlLockPartial_ = true;
            }
            return;
        case ControlLevel::PARTIAL_MANNED:
            if (controlLockSet_ && !controlLockPartial_) controlLockSet_ = false;
            if (!controlLockSet_ && !EditableNodes) {
                controlLockSet_ = true;
                controlLockPartial_ = true;
            }
            return;
        case ControlLevel::FULL:
            if (controlLockSet_) {
                controlLockSet_ = false;
                controlLockPartial_ = false;
            }
            return;
        default: break;
    }
    if (controlLockSet_ && controlLockPartial_) controlLockSet_ = false;
    if (!controlLockSet_) {
        controlLockSet_ = true;
        controlLockPartial_ = false;
    }
}

namespace {
KSPAxisGroup BlockedMask(const std::vector<KSPAxisGroup>& overrides, int overrideGroup) {
    if (overrideGroup == 0) return KSPAxisGroup::None;
    return overrides.at(static_cast<size_t>(overrideGroup - 1));
}
KSPAxisGroup Not(KSPAxisGroup g) { return static_cast<KSPAxisGroup>(~static_cast<int>(g)); }
}  // namespace

// Vessel.cs:1994
void Vessel::SetGroupOverride(int newGroup) {
    if (newGroup == GroupOverride) return;
    const KSPAxisGroup m = (GroupOverride == 0) ? Not(BlockedMask(OverrideAxisControl, newGroup))
                                                : ((newGroup != 0) ? Not(BlockedMask(OverrideAxisControl, newGroup)) : KSPAxisGroup::None);
    if (m != KSPAxisGroup::None) setControlStates.at(static_cast<size_t>(newGroup))->MaskedCopyFrom(*setControlStates[0], m);
    GroupOverride = newGroup;
    GE::OnVesselOverrideGroupChanged.Fire(this);
}

// Vessel.cs:2017
void Vessel::SetControlState(const FlightCtrlState& st) {
    setControlStates.at(static_cast<size_t>(GroupOverride))->CopyFrom(st);
    if (GroupOverride > 0) {
        const KSPAxisGroup m = Not(BlockedMask(OverrideAxisControl, GroupOverride));
        if (m != KSPAxisGroup::None) setControlStates[0]->MaskedCopyFrom(st, m);
    }
}

// Vessel.cs:2030
void Vessel::GetControlState(FlightCtrlState& st) { st.CopyFrom(*setControlStates.at(static_cast<size_t>(GroupOverride))); }

// Vessel.cs:4045
bool Vessel::ActionControlBlocked(KSPActionGroup actionGroup) const {
    if (GroupOverride > 0 && GroupOverride <= static_cast<int>(OverrideActionControl.size()))
        return (static_cast<int>(OverrideActionControl[static_cast<size_t>(GroupOverride - 1)]) & static_cast<int>(actionGroup)) != 0;
    return false;
}

// Vessel.cs:4054
bool Vessel::AxisControlBlocked(KSPAxisGroup axisGroup) const {
    if (GroupOverride > 0 && GroupOverride <= static_cast<int>(OverrideAxisControl.size()))
        return (static_cast<int>(OverrideAxisControl[static_cast<size_t>(GroupOverride - 1)]) & static_cast<int>(axisGroup)) != 0;
    return false;
}

// Vessel.cs:4063
void Vessel::CopyOverrides(const Vessel& v) {
    GroupOverride = v.GroupOverride;
    OverrideDefault = v.OverrideDefault;
    OverrideActionControl = v.OverrideActionControl;
    OverrideAxisControl = v.OverrideAxisControl;
    OverrideGroupNames = v.OverrideGroupNames;
    GE::OnVesselOverrideGroupChanged.Fire(this);
}

// ============================================================ vessel modules

// Vessel.cs:1488
void Vessel::LoadVesselModules(const ConfigNode* node) {
    if (node == nullptr) return;
    for (auto& vesselModule : vesselModules) {
        ConfigNode* node2 = node->GetNode(vesselModule->ClassName());
        if (node2 != nullptr) vesselModule->Load(*node2);
    }
}

// Vessel.cs:1507
void Vessel::UpdateVesselModuleActivation() {
    for (auto& vesselModule : vesselModules) {
        const bool active = vesselModule->ShouldBeActive(env.LoadedSceneIsFlight, loaded);
        if (vesselModule->enabled != active) vesselModule->enabled = !vesselModule->enabled;
    }
}

// Vessel.cs:3870
VesselModule* Vessel::FindVesselModuleByName(const std::string& className) {
    for (auto& m : vesselModules)
        if (m->Class().Is(className)) return m.get();
    return nullptr;
}

// Vessel.cs:3851
PartModule* Vessel::FindPartModuleImplementingClass(const std::string& className) {
    for (Part* part : parts) {
        PartModuleList& modules = part->Modules();
        for (size_t j = 0; j < modules.Count(); j++)
            if (modules[j]->IsA(className)) return modules[j];
    }
    return nullptr;
}

// Vessel.cs:3831
std::vector<PartModule*> Vessel::FindPartModulesImplementing(const std::function<bool(PartModule&)>& is) {
    std::vector<PartModule*> list;
    for (Part* part : parts) {
        PartModuleList& modules = part->Modules();
        for (size_t j = 0; j < modules.Count(); j++)
            if (is(*modules[j])) list.push_back(modules[j]);
    }
    return list;
}

// ============================================================ naming, misc

// Vessel.cs:4137
bool Vessel::IsValidVesselName(const std::string& name) { return !name.empty(); }

// Vessel.cs:4146
void Vessel::OnPartVesselNamingChanged(Part* p) {
    if (p->vessel == this) UpdateVesselNaming();
}

// Vessel.cs:4154
void Vessel::OnVesselNamingVesselWasModified(Vessel* v) {
    if (v == this) UpdateVesselNaming();
}

// Vessel.cs:4162 (VesselNaming.FindPriorityNamePart, VesselNaming.cs:36)
bool Vessel::UpdateVesselNaming(bool noGameEvent) {
    Part* part = nullptr;
    int num = 0;
    for (Part* p : parts) {
        const VesselNaming* vn = p->vesselNaming.get();
        if (vn != nullptr && vn->namingPriority > num) {
            num = vn->namingPriority;
            part = p;
        }
    }
    if (part == nullptr) {
        vesselNamedBy_ = nullptr;
        return false;
    }
    RunVesselNamingUpdates(part, noGameEvent);
    return true;
}

// Vessel.cs:4174
void Vessel::RunVesselNamingUpdates(Part* pNewName, bool noGameEvent) {
    const std::string from = vesselName;
    bool flag = false;
    if (vesselNamedBy_ != pNewName) {
        vesselName = pNewName->vesselNaming->vesselName.value_or(std::string());
        vesselType = pNewName->vesselNaming->vesselType;
        vesselNamedBy_ = pNewName;
        flag = true;
    } else if (pNewName->vesselNaming->vesselName != vesselNamedBy_->vesselNaming->vesselName) {
        vesselName = pNewName->vesselNaming->vesselName.value_or(std::string());
        vesselType = pNewName->vesselNaming->vesselType;
        flag = true;
    } else if (pNewName->vesselNaming->vesselType != vesselNamedBy_->vesselNaming->vesselType) {
        vesselType = pNewName->vesselNaming->vesselType;
        flag = true;
    }
    if (flag && !noGameEvent) GE::onVesselRename.Fire(GE::HostedFromToAction<Vessel*, std::string>(this, from, vesselName));
}

// Vessel.cs:4202
VesselType Vessel::FindDefaultVesselType() {
    VesselType t = vesselType;
    for (Part* part : parts) {
        const auto pt = static_cast<VesselType>(part->fields.vesselType);
        if (static_cast<int>(pt) > static_cast<int>(t)) t = pt;
    }
    return t;
}

// Vessel.cs:4412
void Vessel::SetAutoClean(const std::string& reason) {
    if (!autoClean_) {
        autoClean_ = true;
        autoCleanReason_ = reason;
    }
}

// Vessel.cs:4421
void Vessel::Clean(const std::string& reason) {
    if (protoVessel != nullptr && env.cleanVessel) env.cleanVessel(*this, reason);
}

// Vessel.cs:3908
void Vessel::UpdateVesselSize() {
    if (env.calculateCraftSize) vesselSize = env.calculateCraftSize(parts, rootPart);
}

// Vessel.cs:3913 (PartSet rebuild: WP-15)
void Vessel::UpdateResourceSets() {
    resourcesDirty = false;
    if (env.updateResourceSets) env.updateResourceSets(*this);
}

// Vessel.cs:3892
void Vessel::OnVesselPartCountChanged(Vessel* v) {
    if (v == this) {
        if (updateResourcesOnEvent) UpdateResourceSets();
        else resourcesDirty = true;
        UpdateVesselSize();
    }
}

// Vessel.cs:3943
void Vessel::UpdateCrossfeedResourceSetFL(GE::HostedFromToAction<bool, Part*> data) {
    if (data.from->vessel == this) resourcesDirty = true;
}

// Vessel.cs:3951
void Vessel::UpdateResourceSetsEventCheckPart(Part* p) {
    if (p->vessel == this) {
        if (updateResourcesOnEvent) UpdateResourceSets();
        else resourcesDirty = true;
    }
}

// Vessel.cs:3935
void Vessel::UpdateResourceSetsIfDirty() {
    if (resourcesDirty) UpdateResourceSets();
}

// Vessel.cs:3816
std::vector<Part*> Vessel::GetActiveParts() {
    std::vector<Part*> list;
    for (Part* part : parts)
        if (part->State() == PartStates::ACTIVE && currentStage <= part->fields.inverseStage) list.push_back(part);
    return list;
}

// Vessel.cs:3537
std::string Vessel::GetMETString(const Vessel& v) {
    return KSPUtil::PrintTime(std::abs(v.missionTime), 5, false);
}

// Vessel.cs:3604
void Vessel::CheckAirstreamShields() {
    if (env.LoadedSceneIsFlight)
        for (Part* p : parts) (void)p;  // Part.OnShieldModified: airstream shielding is WP-21
}

// Vessel.cs:3615
void Vessel::OnSaveFlightState(KSPParseableDictionary& dataPool) {
    dataPool.Add("cPch", KSPParseable::FromFloat(campth));
    dataPool.Add("cHdg", KSPParseable::FromFloat(camhdg));
    dataPool.Add("cMod", KSPParseable::FromInt(static_cast<int32_t>(camMode)));
}

// Vessel.cs:3628
void Vessel::OnLoadFlightState(KSPParseableDictionary& dataPool) {
    if (dataPool.ContainsKey("cPch")) campth = dataPool.At("cPch").value_float();
    if (dataPool.ContainsKey("cHdg")) camhdg = dataPool.At("cHdg").value_float();
    if (dataPool.ContainsKey("cMod")) camMode = static_cast<float>(dataPool.At("cMod").value_int());
}

// Vessel.cs:3644 (ClearToSaveStatus.CLEAR = 0, NOT_IN_ATMOSPHERE = 1)
ClearToSaveStatus Vessel::IsClearToSave() {
    if (loaded && !packed && situation == Situations::FLYING) return static_cast<ClearToSaveStatus>(1);
    return static_cast<ClearToSaveStatus>(0);
}

// Vessel.cs:3657
bool Vessel::checkVisibility() {
    for (Vessel* vessel : FlightRegistry::Vessels) {
        if (vessel != this && vessel->isCommandable() &&
            !((Vector3d(vessel->vesselTransform.position) - Vector3d(vesselTransform.position)).sqrMagnitude() >= 25000000.0))
            return true;
    }
    return false;
}

// Vessel.cs:3542 (a live patched-conic solver's maneuver nodes are not read: its flight plan node is)
double Vessel::GetNextManeuverTime(Vessel* v, bool& hasManeuver) {
    hasManeuver = true;
    if (v != nullptr && v->flightPlanNode.nodes().Count() >= 1) {
        ConfigNode* node = v->flightPlanNode.GetNode("MANEUVER");
        double result = 0.0;
        if (node != nullptr && node->HasValue("UT") && dotnet::TryParseDouble(*node->GetValue("UT"), result)) return result;
        hasManeuver = false;
        return 0.0;
    }
    hasManeuver = false;
    return 0.0;
}

// Vessel.cs:3568
std::string Vessel::AutoRename(Vessel& v, const std::string& baseName) {
    int num = std::numeric_limits<int32_t>::max();
    IVesselAutoRename* best = nullptr;
    int count = static_cast<int>(v.parts.size());
    while (count-- > 0) {
        PartModuleList& modules = v.parts[static_cast<size_t>(count)]->Modules();
        int count2 = static_cast<int>(modules.Count());
        while (count2-- > 0) {
            if (auto* r = dynamic_cast<IVesselAutoRename*>(modules[static_cast<size_t>(count2)])) {
                const int num2 = static_cast<int>(r->GetVesselType());
                if (num2 < num) {
                    num = num2;
                    best = r;
                }
            }
        }
    }
    if (best != nullptr) return best->GetVesselName();
    const auto typeDesc = [](VesselType t) {
        return util::displayDescription(t);  // EnumExtensions.displayDescription
    };
    if (static_cast<int>(v.vesselType) > static_cast<int>(VesselType::Debris)) {
        const std::string d = typeDesc(v.vesselType);
        return baseName.find(d) != std::string::npos ? Localizer::Format(baseName) : (Localizer::Format(baseName) + " " + Localizer::Format(d));
    }
    const std::string deb = typeDesc(VesselType::Debris);
    return baseName.find(deb) != std::string::npos ? Localizer::Format(baseName)
                                                   : Localizer::Format("#autoLOC_7001353", {Localizer::Format(baseName) + "^N"});
}

// ============================================================ ITargetable / IDiscoverable

// Vessel.cs:3761 `return ReferenceTransform;` (Vessel.cs:572): vesselTransform (named like the Vessel's GameObject), else the
// reference part's GetReferenceTransform() (Part.cs:1547 name)
UnityEngine::Transform* Vessel::GetTransform() {
    const PartTransform& rt = ReferenceTransform();
    targetTransform_.name = referenceTransformPart_ == nullptr ? gameObjectName : referenceTransformPart_->GetReferenceTransformName();
    targetTransform_.position = rt.position;
    targetTransform_.rotation = rt.rotation;
    return &targetTransform_;
}
Vector3f Vessel::GetFwdVector() { return ReferenceTransform().forward(); }          // Vessel.cs:572
std::string Vessel::GetDisplayName() { return Localizer::Format(vesselName); }       // Vessel.cs:3791
std::string Vessel::RevealDisplayName() { return Localizer::Format(vesselName); }    // Vessel.cs:4378
double Vessel::RevealSpeed() { return LandedOrSplashed() ? horizontalSrfSpeed : obt_speed; }  // Vessel.cs:590
std::string Vessel::RevealSituationString() { return GetSituationString(*this); }  // Vessel.cs:4397
std::string Vessel::RevealType() { return KSPUtil::PrintSpacedStringFromCamelcase(dotnet::EnumToString(vesselType)); }  // Vessel.cs:4402

// Vessel.cs:3491. ResearchAndDevelopment.GetMiniBiomedisplayNameByScienceID(biome, formatted: true) is the science database's:
// env.biomeDisplayName stands for it (unset: the biome name).
std::string Vessel::GetSituationString(Vessel& v) {
    auto biomeName = [&](const std::string& landedAt_) {
        const std::string biome = GetLandedAtString(landedAt_);
        return (env.biomeDisplayName && v.mainBody() != nullptr) ? env.biomeDisplayName(*v.mainBody(), biome) : biome;
    };
    switch (v.situation) {
        case Situations::SUB_ORBITAL: return Localizer::Format("#autoLOC_145790");
        case Situations::FLYING: return Localizer::Format("#autoLOC_145801");
        case Situations::LANDED:
        case Situations::SPLASHED: {
            const char* key = v.situation == Situations::LANDED ? "#autoLOC_6100012" : "#autoLOC_6100016";
            if (v.landedAt.empty()) return Localizer::Format(key, {v.mainBody()->bodyDisplayName});
            if (v.displaylandedAt.empty()) return Localizer::Format(key, {biomeName(v.landedAt)});
            return Localizer::Format(key, {Localizer::Format(v.displaylandedAt)});
        }
        case Situations::PRELAUNCH: return Localizer::Format("#autoLOC_145785");
        default: return Localizer::Format("#autoLOC_145793");
        case Situations::DOCKED: return Localizer::Format("#autoLOC_145792");
        case Situations::ESCAPING: return Localizer::Format("#autoLOC_145791", {v.mainBody()->bodyDisplayName});
        case Situations::ORBITING: return Localizer::Format("#autoLOC_145788", {v.mainBody()->bodyDisplayName});
    }
}

}  // namespace ksp
