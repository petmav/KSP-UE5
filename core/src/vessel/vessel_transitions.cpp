// Vessel state transitions and the ProtoVessel <-> Vessel conversions (Vessel.cs 1653-1992, 2047-2110, 2356-2490, 3240-3290;
// ProtoVessel.cs 149-252, 604-818; ProtoTargetInfo.cs 44-69, 143-200). No physics: rigid bodies, joints, raycasts and Unity
// object lifetime are the engine's; their calls are the VesselEnvironment hooks.
#include <cmath>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/orbit/orbit_snapshot.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
Quaternionf QInverse(const Quaternionf& q) { return {0.0f - q.x, 0.0f - q.y, 0.0f - q.z, q.w}; }
Quaternionf ToFloat(const QuaternionD& q) { return q.ToQuaternionf(); }
// Guid.NewGuid(): 128 random bits (version 4 layout)
Guid NewGuid() {
    Guid g;
    for (int i = 0; i < 4; i++) {
        const uint32_t r = static_cast<uint32_t>(SaveEnvironment::Current().NewGuidHashCode());
        for (int k = 0; k < 4; k++) g.bytes[i * 4 + k] = static_cast<uint8_t>(r >> (8 * k));
    }
    g.bytes[7] = static_cast<uint8_t>((g.bytes[7] & 0x0F) | 0x40);
    g.bytes[8] = static_cast<uint8_t>((g.bytes[8] & 0x3F) | 0x80);
    return g;
}
}  // namespace

// ============================================================ Initialize / assembly

// Vessel.cs:1776
void Vessel::FindVesselParts(Part* part, bool checkAttached) {
    parts.push_back(part);
    part->vessel = this;
    part->fields.orgPos = QInverse(rootOrgRot_) * (part->fields.orgPos - rootOrgPos_);
    part->fields.orgRot = QInverse(rootOrgRot_) * part->fields.orgRot;
    for (size_t i = 0; i < part->children.size(); i++) {
        Part* part2 = part->children[i];
        // `part2.GetComponent<Vessel>() != null`: the child is the root of another vessel
        const bool isOtherRoot = part2->vessel != nullptr && part2->vessel != this && part2->vessel->rootPart == part2;
        if (!checkAttached || (part2->isAttached() && !isOtherRoot)) FindVesselParts(part2, checkAttached);
    }
}

// The Vessel component sits on the root part's GameObject
void Vessel::AdoptParts(std::vector<std::unique_ptr<Part>> ownedParts, Part* root) {
    for (auto& p : ownedParts) ownedParts_.push_back(std::move(p));
    rootPart = root;
}

// Vessel.cs:1653
bool Vessel::Initialize(bool fromShipAssembly) { return Initialize(fromShipAssembly, false, false, true); }

// Vessel.cs:1658 (VesselDeltaV, the map-view renderer and precalc.RunFirst are other packages')
bool Vessel::Initialize(bool fromShipAssembly, bool preCreate, bool orbiting, bool setActiveVessel) {
    updateResourcesOnEvent = false;
    if (rootPart == nullptr) throw NullReferenceException();
    rootOrgPos_ = rootPart->fields.orgPos;
    rootOrgRot_ = rootPart->fields.orgRot;
    SetLandedAt("");
    if (persistentId == 0) persistentId = SaveEnvironment::Current().persistentIds.GetUniquepersistentId();
    parts.clear();
    FindVesselParts(rootPart, !fromShipAssembly);
    if (!preCreate) GE::onVesselPartCountChanged.Fire(this);
    const int count = static_cast<int>(parts.size());
    if (!preCreate)
        for (int i = 0; i < count; i++) parts[static_cast<size_t>(i)]->InitializeModules();
    SyncVesselTransform();
    if (!orbitDriver) {
        orbitDriver = std::make_unique<OrbitDriver>();  // GetComponent<OrbitDriver>() ?? AddComponent<OrbitDriver>()
        orbitDriver->vessel = &OrbitDriverView();
    }
    if (orbitDriver->orbit != nullptr && orbiting) orbitDriver->setReferenceBody(orbit().referenceBody);
    else orbitDriver->setReferenceBody(FlightGlobals::getMainBody(Vector3d(vesselTransform.position)));
    if (env.pqsLevels) {
        int mn = PQSminLevel, mx = PQSmaxLevel;
        if (env.pqsLevels(*orbitDriver->referenceBody(), mn, mx)) {
            PQSminLevel = mn;
            PQSmaxLevel = mx;
        }
    }
    vesselType = FindDefaultVesselType();
    if (static_cast<int>(vesselType) > static_cast<int>(VesselType::Debris))
        for (Part* p : parts) p->set_isAttached(true);
    orbitDriver->updateMode = fromShipAssembly ? OrbitDriver::UpdateMode::IDLE : OrbitDriver::UpdateMode::TRACK_Phys;
    if (!preCreate) FlightRegistry::AddVessel(this);
    launchTime = Planetarium::GetUniversalTime();
    missionTime = 0.0;
    SetLoaded(true);
    for (auto& m : vesselModules) m->OnLoadVessel();
    persistent_ = isPersistent();
    MakeInactive();
    if (fromShipAssembly) {
        mainBody()->GetLatLonAlt(Vector3d(vesselTransform.position), latitude, longitude, altitude);
        srfRelRotation = QInverse(ToFloat(mainBody()->rotation)) * vesselTransform.rotation;
        upAxis = FlightMath::getUpAxis(*mainBody(), Vector3d(vesselTransform.position));
        heightFromTerrain = Vector3f::Dot(vesselTransform.position, upAxis.ToVector3f());
        terrainNormal = vesselTransform.InverseTransformDirection(upAxis.ToVector3f());
        lastSituation_ = Situations::PRELAUNCH;
        situation = Situations::PRELAUNCH;
        packed = true;
        if (!preCreate) {
            if (setActiveVessel) physicsHoldLock_ = true;  // AddPhysicsHoldLock
            GE::onVesselCreate.Fire(this);
        }
    } else {
        packed = false;
        lastUT = Planetarium::GetUniversalTime();
        checkLanded();
        checkSplashed();
        FallBackReferenceTransform();
        updateSituation();
        if (!preCreate) {
            GE::onVesselCreate.Fire(this);
            GE::onVesselWasModified.Fire(this);
        }
        RebuildCrewList();
    }
    protoVesselOwner_ = std::make_shared<ProtoVessel>(*this, preCreate);
    protoVessel = protoVesselOwner_.get();
    updateResourcesOnEvent = true;
    if (!preCreate) UpdateResourceSets();
    return true;
}

// Vessel.cs:1793
void Vessel::StartFromBackup(ProtoVessel* pv) { StartFromBackup(pv, nullptr); }

// Vessel.cs:1798 (the MISSION-mode branch needs HighLogic.CurrentGame.Mode; AddOrbitRenderer / VesselDeltaV are other packages')
void Vessel::StartFromBackup(ProtoVessel* pv, FlightState* /*st*/) {
    parts.clear();
    protoVessel = pv;
    protoVesselOwner_.reset();
}

// ============================================================ Load / Unload / Backup

// Vessel.cs:1813
void Vessel::Load() {
    if (loaded) return;
    updateResourcesOnEvent = false;
    if (!packed) GoOnRails();
    // ProtoVessel.LoadObjects (ProtoVessel.cs:820): the parts of the snapshots become the vessel's parts
    ProtoVessel::LoadedObjects lo = protoVessel->LoadObjects(this);
    for (auto& p : lo.parts) {
        p->vessel = this;
        parts.push_back(p.get());
    }
    rootPart = lo.rootPart;
    AdoptParts(std::move(lo.parts), lo.rootPart);
    // the parts are children of the vessel GameObject: their positions are vessel-relative until placed
    const Quaternionf vr = vesselTransform.rotation;
    const Vector3f vp = vesselTransform.position;
    for (Part* p : parts) {
        p->partTransform.position = vp + vr * p->fields.orgPos;
        p->partTransform.rotation = vr * p->fields.orgRot;
    }
    SyncVesselTransform();
    gameObjectName = gameObjectName + " (" + GetDisplayName() + ")";
    SetLoaded(true);
    RebuildCrewList();
    for (auto& m : vesselModules) m->OnLoadVessel();
    if (Landed) {
        altitude = getCorrectedLandedAltitude(latitude, longitude, altitude, *mainBody());
        if (isActiveVessel()) physicsHoldLock_ = true;  // AddPhysicsHoldLock
    }
    if (referenceTransformId != 0) SetReferenceTransform(FindReferenceTransform(referenceTransformId));
    staticPressurekPa = FlightMath::getStaticPressure(Vector3d(vesselTransform.position), *mainBody());
    updateSituation();
    GE::onVesselLoaded.Fire(this);
    updateResourcesOnEvent = true;
    UpdateResourceSets();
    if (targetObject == nullptr) {
        ITargetable* targetable = protoVessel->targetInfo.FindTarget();
        if (targetable != nullptr) {
            pTI = ProtoTargetInfo(protoVessel->targetInfo);
            targetObject = targetable;
        }
    }
    pWPI = protoVessel->waypointInfo;  // navigationWaypoint == null in kspcore
}

// Vessel.cs:1868 (Unity component teardown: the parts, the rigid bodies and joints go with the owned part objects)
void Vessel::Unload() {
    if (!loaded) return;
    isUnloading = true;
    for (auto& m : vesselModules) m->OnUnloadVessel();
    protoVesselOwner_ = std::make_shared<ProtoVessel>(*this);
    protoVessel = protoVesselOwner_.get();
    if (rootPart != nullptr) rootPart->OnJustAboutToBeDestroyed();
    if (this == FlightRegistry::ActiveVessel) OnJustAboutToBeDestroyed();
    for (Part* p : parts) p->vessel = nullptr;
    parts.clear();
    rootPart = nullptr;
    referenceTransformPart_ = nullptr;
    referenceTransformPartRecall_ = nullptr;
    ownedParts_.clear();
    if (!packed) GoOnRails();
    SetLoaded(false);
    gameObjectName = GetDisplayName() + " (Unloaded)";
    isUnloading = false;
    GE::onVesselUnloaded.Fire(this);
}

// Vessel.cs:1969
ProtoVessel* Vessel::BackupVessel() {
    isBackingUp = true;
    if (loaded) {
        protoVesselOwner_ = std::make_shared<ProtoVessel>(*this);
        protoVessel = protoVesselOwner_.get();
    } else {
        protoVessel->vesselName = vesselName;
        protoVessel->vesselType = vesselType;
        protoVessel->orbitSnapShot = OrbitSnapshot(orbit(), FlightRegistry::Bodies());
        protoVessel->discoveryInfo = std::make_shared<ConfigNode>("DISCOVERY");
        discoveryInfo_->Save(*protoVessel->discoveryInfo);
        if (PatchedConicsAttached()) protoVessel->flightPlan = std::make_shared<ConfigNode>("FLIGHTPLAN");  // patchedConicSolver.Save: WP-08 solver
        protoVessel->lastUT = lastUT;
    }
    isBackingUp = false;
    return protoVessel;
}

// Vessel.cs:2047 (camera, IVA, autopilot, staging icons and the target/waypoint resume are scene work)
void Vessel::MakeActive() {
    state = State::ACTIVE;
    if (!loaded) Load();
}

// Vessel.cs:2081
void Vessel::MakeInactive() {
    if (controlLockSet_) controlLockSet_ = false;
    if (state != State::DEAD) {
        if (state == State::ACTIVE) {
            // camhdg / campth / camMode of the flight camera: the host keeps them in the fields before this call
        }
        state = State::INACTIVE;
    }
}

// ============================================================ rails

// Vessel.cs:2356
void Vessel::GoOnRails() {
    if (packed) return;
    if (env.precalcGoOnRails) env.precalcGoOnRails(*this);
    for (auto& m : vesselModules) m->OnGoOnRails();
    GE::onVesselGoOnRails.Fire(this);
    ctrlState.Neutralize();
    if (loaded) {
        if (env.packPart)
            for (Part* p : parts) env.packPart(*p);
        if (Landed) GetHeightFromTerrain();
    }
    packed = true;
    if (LandedOrSplashed()) orbitDriver->SetOrbitMode(OrbitDriver::UpdateMode::IDLE);
    else orbitDriver->SetOrbitMode(OrbitDriver::UpdateMode::UPDATE);
}

// Vessel.cs:2395
void Vessel::GoOffRails() {
    if (!packed || !loaded || (!PhysicsHoldExpired() && FindPartModuleImplementingClass("ModuleGroundPart") == nullptr)) return;
    if (Landed && !Splashed) {
        if (PQSminLevel != 0 && PQSmaxLevel != 0 && situation != Situations::PRELAUNCH && env.pqsLevels) {
            int mn = 0, mx = 0;
            if (env.pqsLevels(*mainBody(), mn, mx)) skipGroundPositioning = PQSminLevel == mn && PQSmaxLevel == mx;
        }
        if (parts.size() == 1 && !skipGroundPositioningForDroppedPart) skipGroundPositioning = false;
        if ((vesselSpawning || !skipGroundPositioning) && !(skipGroundPositioning || !env.checkGroundCollision || env.checkGroundCollision(*this))) {
            vesselSpawning = false;
            return;
        }
    }
    vesselSpawning = false;
    skipGroundPositioningForDroppedPart = false;
    physicsHoldLock_ = false;  // RemovePhysicsHoldLock
    if (env.precalcGoOffRails) env.precalcGoOffRails(*this);
    // FlightGlobals.RefFrameIsRotating (FlightGlobals.cs:224): currentMainBody.rotates && currentMainBody.inverseRotation, with
    // currentMainBody the active vessel's body (this vessel's when there is no active vessel)
    const CelestialBody* frameBody = FlightRegistry::ActiveVessel != nullptr ? FlightRegistry::ActiveVessel->mainBody() : mainBody();
    const bool refFrameIsRotating = frameBody->rotates && frameBody->inverseRotation;
    if (LandedOrSplashed() && !refFrameIsRotating) return;
    for (auto& m : vesselModules) m->OnGoOffRails();
    if (this == FlightRegistry::ActiveVessel && env.krakensbaneAddFrameVelocity) {
        const Vector3d rot = orbit().referenceBody->inverseRotation ? orbit().referenceBody->getRFrmVel(Vector3d(vesselTransform.position)) : Vector3d();
        env.krakensbaneAddFrameVelocity(orbit().GetVel(FlightRegistry::ActiveVessel->mainBody()) - rot -
                                        (env.krakensbaneFrameVelocity ? env.krakensbaneFrameVelocity() : Vector3d()));
    }
    packed = false;
    for (Part* p : parts) {
        if (env.unpackPart) env.unpackPart(*p);
        p->InitializeModules();
        if (p->fields.PermanentGroundContact) Landed = true;
    }
    orbitDriver->SetOrbitMode(OrbitDriver::UpdateMode::TRACK_Phys);
    if (env.resumePartVelocity)
        for (Part* p : parts) env.resumePartVelocity(*p);
    if (LandedOrSplashed()) {
        for (Vector3d& a : accelSmoothing_) a.Zero();
    } else {
        int num2 = accelSmoothingLength_;
        while (num2-- > 0) acceleration = graviticAcceleration;
    }
    GE::onVesselGoOffRails.Fire(this);
}

// Vessel.cs:3240 (the map/camera/scene teardown and Unity destruction are the host's)
void Vessel::Die() {
    GE::onVesselWillDestroy.Fire(this);
    state = State::DEAD;
    obt_velocity.Zero();
    srf_velocity.Zero();
    rb_velocity = Vector3f();
    rb_velocityD.Zero();
    verticalSpeed = 0.0;
    horizontalSrfSpeed = 0.0;
    angularVelocity = Vector3f();
    acceleration.Zero();
    geeForce = 0.0;
    if (FlightRegistry::ActiveVessel != this) {
        FlightRegistry::RemoveVessel(this);
        OnJustAboutToBeDestroyed();
    } else {
        OnJustAboutToBeDestroyed();
        if (rootPart != nullptr) rootPart->OnJustAboutToBeDestroyed();
    }
    if (!loaded) {
        log::Info("[Vessel " + GetDisplayName() + "]: Vessel was destroyed.");
        MurderCrew();
    }
    physicsHoldLock_ = false;
}

// ============================================================ ProtoVessel <-> Vessel

// ProtoVessel.cs:149
ProtoVessel::ProtoVessel(Vessel& VesselRef) : ProtoVessel(VesselRef, true) {}

// ProtoVessel.cs:153
ProtoVessel::ProtoVessel(Vessel& VesselRef, bool /*preCreate*/) {
    vesselRef = &VesselRef;
    orbitSnapShot = OrbitSnapshot(VesselRef.orbit(), FlightRegistry::Bodies());
    vesselID = VesselRef.id;
    persistentId = VesselRef.persistentId;
    refTransform = VesselRef.referenceTransformId;
    vesselType = VesselRef.vesselType;
    situation = VesselRef.situation;
    landed = VesselRef.Landed;
    skipGroundPositioning = VesselRef.skipGroundPositioning;
    skipGroundPositioningForDroppedPart = VesselRef.skipGroundPositioningForDroppedPart;
    vesselSpawning = VesselRef.vesselSpawning;
    launchedFrom = VesselRef.launchedFrom;
    landedAt = VesselRef.landedAt;
    displaylandedAt = VesselRef.displaylandedAt;
    splashed = VesselRef.Splashed;
    vesselName = VesselRef.vesselName;
    distanceTraveled = VesselRef.distanceTraveled;
    missionTime = VesselRef.missionTime;
    launchTime = VesselRef.launchTime;
    lastUT = VesselRef.lastUT;
    autoClean = VesselRef.AutoClean();
    autoCleanReason = VesselRef.AutoCleanReason();
    wasControllable = VesselRef.IsControllable();
    for (Part* part : VesselRef.parts)
        if (part->State() != PartStates::DEAD) protoPartSnapshots.push_back(std::make_unique<ProtoPartSnapshot>(*part, this));
    for (auto& s : protoPartSnapshots) s->storePartRefs();
    CoM = VesselRef.localCoM;
    latitude = VesselRef.latitude;
    longitude = VesselRef.longitude;
    altitude = VesselRef.altitude;
    height = VesselRef.heightFromTerrain;
    normal = VesselRef.terrainNormal;
    altimeterDisplayState = VesselRef.altimeterDisplayState;
    rotation = VesselRef.srfRelRotation;
    PQSminLevel = VesselRef.PQSminLevel;
    PQSmaxLevel = VesselRef.PQSmaxLevel;
    stage = VesselRef.currentStage;
    persistent = VesselRef.isPersistent();
    VesselRef.protoVessel = this;
    actionGroups = std::make_shared<ConfigNode>();
    VesselRef.ActionGroups().Save(*actionGroups);
    GroupOverride = VesselRef.GroupOverride;
    OverrideDefault = VesselRef.OverrideDefault;
    OverrideActionControl = VesselRef.OverrideActionControl;
    OverrideAxisControl = VesselRef.OverrideAxisControl;
    OverrideGroupNames = VesselRef.OverrideGroupNames;
    discoveryInfo = std::make_shared<ConfigNode>();
    VesselRef.GetDiscoveryInfo().Save(*discoveryInfo);
    flightPlan = std::make_shared<ConfigNode>();
    VesselRef.flightPlanNode.CopyTo(*flightPlan);  // an attached patched-conic solver first rewrites flightPlanNode (WP-08 solver)
    targetInfo = ProtoTargetInfo(VesselRef.targetObject);
    waypointInfo = ProtoWaypointInfo();  // vesselRef.navigationWaypoint is null in kspcore
    ctrlState = std::make_shared<ConfigNode>();
    VesselRef.ctrlState.Save(*ctrlState);
    SaveVesselModules();
    VesselRef.OnSaveFlightState(vesselStateValues);
    RebuildCrewCounts();
    GE::onProtoVesselSave.Fire(GE::FromToAction<ProtoVessel*, ConfigNode*>(this, nullptr));
}

// ProtoVessel.cs:604-611
void ProtoVessel::SaveLiveVesselModules() {
    if (vesselRef == nullptr) return;
    vesselModules->ClearData();
    for (auto& vesselModule : vesselRef->vesselModules) {
        ConfigNode* node = vesselModules->AddNode(vesselModule->ClassName());
        vesselModule->Save(*node);
    }
}

// ProtoVessel.cs:615
void ProtoVessel::ResetProtoPartSnapShots() {
    protoPartSnapshots.clear();
    for (Part* part : vesselRef->parts)
        if (part->State() != PartStates::DEAD) protoPartSnapshots.push_back(std::make_unique<ProtoPartSnapshot>(*part, this));
    for (auto& s : protoPartSnapshots) s->storePartRefs();
}

// ProtoVessel.cs:633 / 638 (the missing-part popup becomes a log line plus the crew respawn)
std::unique_ptr<Vessel> ProtoVessel::Load(FlightState* st, Vessel* vessel) {
    GE::onProtoVesselLoad.Fire(GE::FromToAction<ProtoVessel*, ConfigNode*>(this, nullptr));
    std::vector<std::string> missing;
    for (auto& pps : protoPartSnapshots) {
        if ((pps->partInfo == nullptr || !pps->partInfo->partPrefab) && pps->partName &&
            std::find(missing.begin(), missing.end(), *pps->partName) == missing.end())
            missing.push_back(*pps->partName);
    }
    if (!missing.empty()) {
        std::string text;
        for (const auto& m : missing) text += m + "\n";
        log::Error("Vessel Loading Error: " + GetDisplayName() + " is missing parts:\n" + text);
        for (const PCM& c : crew_) c->StartRespawnPeriod();
        return nullptr;
    }
    std::unique_ptr<Vessel> created;
    if (vessel == nullptr) {
        created = std::make_unique<Vessel>();
        vesselRef = created.get();
        vesselRef->id = vesselID == Guid::Empty() ? NewGuid() : vesselID;
        if (persistentId == 0) persistentId = SaveEnvironment::Current().persistentIds.GetUniquepersistentId();
        vesselRef->persistentId = persistentId;
        vesselRef->IgnoreGForces(10);
    } else {
        vesselRef = vessel;
    }
    vesselRef->gameObjectName = GetDisplayName() + " (unloaded)";
    vesselRef->vesselName = vesselName;
    vesselRef->vesselType = vesselType;
    vesselRef->situation = situation;
    vesselRef->Landed = landed;
    vesselRef->skipGroundPositioning = skipGroundPositioning;
    vesselRef->skipGroundPositioningForDroppedPart = skipGroundPositioningForDroppedPart;
    vesselRef->vesselSpawning = vesselSpawning;
    vesselRef->launchedFrom = launchedFrom;
    vesselRef->Splashed = splashed;
    vesselRef->packed = true;
    vesselRef->localCoM = CoM;
    vesselRef->missionTime = missionTime;
    vesselRef->launchTime = launchTime;
    vesselRef->lastUT = lastUT;
    vesselRef->distanceTraveled = distanceTraveled;
    if (autoClean) vesselRef->SetAutoClean(autoCleanReason);
    if (vessel == nullptr) {
        vesselRef->referenceTransformId = refTransform;
        vesselRef->orbitDriver = std::make_unique<OrbitDriver>();
        vesselRef->orbitDriver->vessel = &vesselRef->OrbitDriverView();
        vesselRef->orbitDriver->orbit = std::make_shared<Orbit>(orbitSnapShot->Load(FlightRegistry::Bodies()));
        vesselRef->orbitDriver->updateMode = (!vesselRef->LandedOrSplashed()) ? OrbitDriver::UpdateMode::UPDATE : OrbitDriver::UpdateMode::IDLE;
        vesselRef->latitude = latitude;
        vesselRef->longitude = longitude;
        vesselRef->altitude = altitude;
        vesselRef->heightFromTerrain = height;
        vesselRef->terrainNormal = normal;
        vesselRef->PQSminLevel = PQSminLevel;
        vesselRef->PQSmaxLevel = PQSmaxLevel;
        vesselRef->altimeterDisplayState = altimeterDisplayState;
        CelestialBody* body = vesselRef->orbit().referenceBody;
        vesselRef->vesselTransform.position = body->GetWorldSurfacePosition(latitude, longitude, altitude).ToVector3f();
        vesselRef->srfRelRotation = rotation;
        vesselRef->vesselTransform.rotation = ToFloat(body->rotation) * rotation;
    }
    vesselRef->currentStage = stage;
    vesselRef->set_isPersistent(persistent);
    vesselRef->GroupOverride = GroupOverride;
    vesselRef->OverrideDefault = OverrideDefault;
    vesselRef->OverrideActionControl = OverrideActionControl;
    vesselRef->OverrideAxisControl = OverrideAxisControl;
    vesselRef->OverrideGroupNames = OverrideGroupNames;
    vesselRef->SetLandedAt(landedAt, "", displaylandedAt);
    if (actionGroups) vesselRef->ActionGroups().Load(*actionGroups);
    if (discoveryInfo) vesselRef->GetDiscoveryInfo().Load(*discoveryInfo);
    if (flightPlan) {
        vesselRef->flightPlanNode.ClearData();
        flightPlan->CopyTo(vesselRef->flightPlanNode);  // `flightPlanNode = flightPlan` (the same node object in C#)
    }
    if (ctrlState) vesselRef->ctrlState.Load(*ctrlState);
    if (vessel == nullptr) {
        vesselRef->LoadVesselModules(vesselModules.get());
        vesselRef->StartFromBackup(this, st);
    }
    FlightRegistry::AddVessel(vesselRef);
    vesselRef->OnLoadFlightState(vesselStateValues);
    if (vessel == nullptr) GE::onVesselCreate.Fire(vesselRef);
    return created;
}

// ============================================================ ProtoTargetInfo (ITargetable side)

// ProtoTargetInfo.cs:35 (Part and CelestialBody are not ITargetable objects in kspcore: they have no target branch here)
ProtoTargetInfo::ProtoTargetInfo(ITargetable* tgt) {
    if (auto* pm = dynamic_cast<PartModule*>(tgt)) {
        targetType = Type::PartModule;
        Part* part = static_cast<Part*>(pm->part);
        partModuleIndex = part->Modules().IndexOf(pm);
        partUID = part->fields.flightID;
        vesselId = part->vessel->id;
    } else if (auto* v = dynamic_cast<Vessel*>(tgt)) {
        targetType = Type::Vessel;
        vesselId = v->id;
    } else if (tgt != nullptr) {
        targetType = Type::Generic;
        // ProtoTargetInfo.cs:64 tgt.GetTransform().name (a destroyed PositionTarget hands out null: NullReferenceException)
        UnityEngine::Transform* transform = tgt->GetTransform();
        if (transform == nullptr) throw NullReferenceException();
        uniqueName = transform->name;
    } else {
        targetType = Type::Null;
    }
}

// ProtoTargetInfo.cs:134
ITargetable* ProtoTargetInfo::FindTarget() const {
    switch (targetType) {
        default: return nullptr;
        case Type::Vessel: return FlightRegistry::FindVessel(vesselId);
        case Type::PartModule: {
            Part* part = FlightRegistry::FindPartByID(partUID);
            PartModule* partModule = nullptr;
            if (part != nullptr) partModule = part->Modules()[static_cast<size_t>(partModuleIndex)];
            if (partModule != nullptr) return dynamic_cast<ITargetable*>(partModule);
            return FlightRegistry::FindVessel(vesselId);
        }
        case Type::Part: return FlightRegistry::FindVessel(vesselId);  // `part is ITargetable` is false for kspcore parts
        case Type::CelestialBody: return nullptr;  // FlightGlobals.GetBodyByName: bodies are not ITargetable here
        case Type::Generic: return nullptr;        // GameObject.Find
    }
}

}  // namespace ksp
