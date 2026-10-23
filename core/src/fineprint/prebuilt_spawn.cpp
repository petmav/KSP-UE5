// WP-38d: ContractSystem.SpawnPreBuiltShip / ConstructShip. See include/ksp/fineprint/prebuilt_spawn.hpp.
#include <cmath>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/career/career_environment.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/fineprint/prebuilt_spawn.hpp"
#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/missions/mission_data.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/space_center/pqs_city.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/wheels/wheel_modules.hpp"

namespace ksp::Contracts::Lite {

using GE = events::GameEvents;

void PreBuiltSpawnEnvironment::Reset() {
    putShipToGround = nullptr;
    checkLaunchClamps = nullptr;
    surfaceHeight = nullptr;
    toggleVesselComponents = nullptr;
    setVesselOrbit = nullptr;
    resetKSCMarkers = nullptr;
    loadedScene = nullptr;
}

namespace {
using Env = PreBuiltSpawnEnvironment;
GameScenes Scene() { return Env::loadedScene ? Env::loadedScene() : SaveEnvironment::Current().loadedScene; }
void Toggle(Vessel& v, bool on) {
    if (Env::toggleVesselComponents) Env::toggleVesselComponents(v, on);
}
// MissionSystem.putShiptoGround with spawnPointTransform == null (MissionSystem.cs:1352)
void PutShipToGround(ShipConstruct& ship, const PreBuiltVesselSituation& sit, const Vector3f& pos, const Quaternionf& rot) {
    if (Env::putShipToGround) {
        Env::putShipToGround(ship, sit, pos, rot);
        return;
    }
    SpawnPoint sp;
    sp.position = pos;
    sp.rotation = rot;
    ShipConstruction::PutShipToGround(ship, sp);
}
PreBuiltSpawnResult Fail(const std::string& message) {
    log::Error(message);
    PreBuiltSpawnResult r;
    r.error = message;
    return r;
}
}  // namespace

// ContractSystem.cs:1120
void PreBuiltShipSpawner::SpawnPreBuiltShip(PreBuiltCraftDefinition& craftDef, const PreBuiltVesselSituation& vesselSit) { last = ConstructShip(craftDef, vesselSit); }

// ContractSystem.cs:1125
PreBuiltSpawnResult PreBuiltShipSpawner::ConstructShip(PreBuiltCraftDefinition& craftDef, const PreBuiltVesselSituation& vesselSituation) {
    std::string value;
    craftDef.craftNode->TryGetValue("shipName", value);
    log::Info("[ContractSystem]: Constructing Ship (" + value + ")");
    SaveEnvironment& senv = SaveEnvironment::Current();
    Game* game = senv.currentGame;
    if (game == nullptr) throw NullReferenceException("HighLogic.CurrentGame");
    const uint32_t uniquepersistentId = senv.persistentIds.GetUniquepersistentId();
    ConfigNode::NodePtr node = ConfigNode::Create();
    auto shipOut = std::make_unique<ShipConstruct>();
    std::string message;
    bool resetCBRequired = false;
    bool originalCBInverseRotation = false;
    bool flag2 = false;
    Vector3d vector3d;
    double alt = 0.0;
    float num = 0.0f;
    float num2 = 0.0f;
    if (!shipOut->LoadShipWithParts(*craftDef.craftNode, uniquepersistentId, true, message)) {
        log::Error("[ContractSystem] Failed to load vessel: " + value);
        return Fail(message);
    }
    if (!vesselSituation.playerCreated) {
        shipOut->shipName = vesselSituation.vesselName;
        if (vesselSituation.situation == PreBuiltVesselSituation::StartSituation::ORBITING) {
            const bool ok = Env::checkLaunchClamps ? Env::checkLaunchClamps(*shipOut, vesselSituation, false) : true;
            if (!ok)
                return Fail("[MissionsExpansion] Failed to build mission vessel: " + vesselSituation.vesselName +
                            " as it contains launch clamps and has a starting situation of Orbiting.");
        }
    }
    VesselCrewManifest crewManifest;
    const std::string text;
    const std::string displaylandedAt;
    bool flag3 = vesselSituation.situation == PreBuiltVesselSituation::StartSituation::LANDED;
    bool flag4 = flag3 && vesselSituation.splashed;
    const bool flag6 = vesselSituation.situation == PreBuiltVesselSituation::StartSituation::ORBITING;
    CelestialBody* targetBody = vesselSituation.targetBody;
    // the orbit snapshot: new MissionOrbit(orbit) (orbiting) / new MissionOrbit(targetBody) (landed)
    Expansions::Missions::MissionOrbit snapshot = vesselSituation.hasOrbit ? Expansions::Missions::MissionOrbit(vesselSituation.orbit)
                                                                           : Expansions::Missions::MissionOrbit(targetBody);
    if (flag6) {
        // MissionSystem.setBodyRotation(orbitSnapShot.Body, ...) (MissionSystem.cs:1333)
        CelestialBody* body = snapshot.Body();
        originalCBInverseRotation = body->inverseRotation;
        if (Scene() == GameScenes::SPACECENTER && body->inverseRotation) {
            resetCBRequired = true;
            body->inverseRotation = false;
            body->CBUpdate();
        }
    }
    Orbit orbit = snapshot.RelativeOrbit(nullptr);
    if (flag3) orbit.referenceBody = targetBody;
    if (flag3) {
        CelestialFrame cf;
        CelestialFrame::SetFrame(0.0, 0.0, 0.0, cf);
        const Vector3d surfaceNVector = LatLonGetSurfaceNVector(cf, vesselSituation.latitude, vesselSituation.longitude);
        // the temporary "TempObject" GameObject parented to the body only fed alt below (it is destroyed at the end); its transform is not read
        if (Env::surfaceHeight) {
            alt = Env::surfaceHeight(targetBody, surfaceNVector) - targetBody->Radius;
        } else {
            if (targetBody->pqsController == nullptr) throw NullReferenceException("targetBody.pqsController");
            alt = targetBody->pqsController->GetSurfaceHeight(surfaceNVector, true) - targetBody->Radius;
        }
        if (flag4) {
            if (alt < 0.0) {
                alt = 0.0;
                // EditorDriver.editorFacility = vesselSituation.location.facility (the default VAB; the launch clamp check reads it)
                const bool ok = Env::checkLaunchClamps ? Env::checkLaunchClamps(*shipOut, vesselSituation, true) : true;
                if (!ok)
                    return Fail("[MissionsExpansion] Failed to build mission vessel: " + value +
                                " as it contains launch clamps and has a starting situation of Orbiting.");
            } else {
                flag4 = false;
            }
        }
        num = static_cast<float>(vesselSituation.latitude);
        num2 = static_cast<float>(vesselSituation.longitude);
        const Vector3d worldSurfacePosition = targetBody->GetWorldSurfacePosition(static_cast<double>(num), static_cast<double>(num2), alt);
        const Vector3d relSurfacePosition = targetBody->GetRelSurfacePosition(static_cast<double>(num), static_cast<double>(num2) + targetBody->directRotAngle, alt);
        const Quaternionf quaternion = vesselSituation.rotation;
        const std::string craftType = craftDef.craftNode->GetValue("type").value_or("");
        const Quaternionf quaternion2 = (craftType == "VAB") ? UnityNative::Euler(Vector3f(-90.0f, -90.0f, -90.0f)) : UnityNative::Euler(Vector3f(0.0f, -90.0f, -90.0f));
        // Quaternion.LookRotation(relSurfacePosition) * quaternion * quaternion2 (the Vector3d converts to Vector3; the product is QuaternionD-converted and back)
        const Quaternionf quaternionD = (UnityNative::LookRotation(relSurfacePosition.ToVector3f(), Vector3f::up()) * quaternion) * quaternion2;
        PutShipToGround(*shipOut, vesselSituation, worldSurfacePosition.ToVector3f(), quaternionD);
        const double num3 = (worldSurfacePosition - Vector3d()).magnitude();
        if (num3 > 10000.0) {
            flag2 = true;
            vector3d = worldSurfacePosition;
            log::Info("[MissionSystem]: Vessel " + shipOut->shipName + ". Distance " + std::to_string(num3) + " Setting pos to a vector");
        }
    }
    // AssembleForLaunch over the ship the placement left: the root part's world transform is the launch placement
    const std::string flagURL = shipOut->missionFlag != "" ? shipOut->missionFlag : game->flagURL.value_or("");
    LaunchPlacement placement;
    if (!shipOut->liveParts.empty()) {
        placement.rootPosition = shipOut->liveParts[0]->partTransform.position;
        placement.rootRotation = shipOut->liveParts[0]->partTransform.rotation;
    }
    // ShipConstruction reads HighLogic.CurrentGame (launchID++): the environment record points at it while ConstructShip runs
    struct CurrentGameScope {
        Game* previous;
        explicit CurrentGameScope(Game* g) : previous(ShipConstruction::env.CurrentGame) {
            if (previous == nullptr) ShipConstruction::env.CurrentGame = g;
        }
        ~CurrentGameScope() { ShipConstruction::env.CurrentGame = previous; }
    } currentGameScope(game);
    std::unique_ptr<Vessel> vessel2;
    try {
        vessel2 = ShipConstruction::AssembleForLaunch(*shipOut, text, displaylandedAt, flagURL, *game, &crewManifest, true, false, flag3, false,
                                                      std::make_shared<Orbit>(orbit), flag6, flag4, placement);
        Toggle(*vessel2, false);  // vessel2.gameObject.SetActive(false); MissionSystem.ToggleVesselComponents(vessel2, false)
    } catch (const std::exception& e) {
        PreBuiltSpawnResult failed = Fail("[MissionSystem]: Unable to Assemble Vessel :" + vesselSituation.vesselName + " Vessel spawning Failed");
        failed.error += std::string(" [") + e.what() + "]";  // (C++ addition: the C# swallows the exception)
        return failed;
    }
    ProtoVessel* proto = vessel2->OwnedProtoVessel();
    if (proto == nullptr) return Fail("[MissionSystem]: Unable to Assemble Vessel :" + vesselSituation.vesselName + " Vessel spawning Failed");
    if (flag4) {
        proto->altitude = alt;
        proto->situation = Vessel::Situations::SPLASHED;
        proto->skipGroundPositioning = true;
        proto->vesselSpawning = false;
    } else {
        proto->situation = flag6 ? Vessel::Situations::ORBITING : Vessel::Situations::LANDED;  // Enum.Parse(Vessel.Situations, location.situation.ToString())
        proto->skipGroundPositioning = vessel2->skipGroundPositioning;
        proto->vesselSpawning = vessel2->vesselSpawning;
    }
    if (flag3) {
        if (flag4) {
            proto->landed = false;
            proto->splashed = true;
        } else {
            proto->landed = true;
            proto->landedAt = vessel2->landedAt;
            proto->displaylandedAt = vessel2->displaylandedAt;
            proto->PQSminLevel = 0;
            vessel2->PQSminLevel = 0;
            proto->PQSmaxLevel = 0;
            vessel2->PQSmaxLevel = 0;
        }
        vessel2->latitude = (proto->latitude = static_cast<double>(num));
        vessel2->longitude = (proto->longitude = static_cast<double>(num2));
        if (flag2) vessel2->altitude = (proto->altitude = alt);
    }
    // the wheel brakes: brakeInput, the Brakes action group and the proto vessel's ACTIONGROUPS node
    std::vector<PartModule*> list = vessel2->FindPartModulesImplementing([](PartModule& m) { return dynamic_cast<ModuleWheels::ModuleWheelBrakes*>(&m) != nullptr; });
    if (!list.empty()) {
        for (PartModule* m : list) static_cast<ModuleWheels::ModuleWheelBrakes*>(m)->Set<float>("brakeInput", vesselSituation.brakesOn ? 1.0f : 0.0f);
        if (vesselSituation.brakesOn) {
            vessel2->ActionGroups().SetGroup(KSPActionGroup::Brakes, true);
            const int groupIndex = BaseAction::GetGroupIndex(static_cast<int64_t>(KSPActionGroup::Brakes));
            vessel2->ActionGroups().cooldownTimes[static_cast<size_t>(groupIndex)] = 0.0;
            proto->actionGroups = ConfigNode::Create();
            vessel2->ActionGroups().Save(*proto->actionGroups);
        }
    }
    proto->ResetProtoPartSnapShots();
    int num6 = 0;
    for (size_t j = 0; j < proto->protoPartSnapshots.size(); ++j) {
        ProtoPartSnapshot& pps = *proto->protoPartSnapshots[j];
        pps.flagURL = flagURL;
        if (pps.inverseStageIndex > num6) num6 = pps.inverseStageIndex;
    }
    proto->stage = num6 + 1;
    proto->refTransform = vessel2->referenceTransformId;
    vessel2->vesselType = shipOut->vesselType;
    proto->vesselType = shipOut->vesselType;
    proto->Save(*node);
    FlightRegistry::RemoveVessel(vessel2.get());
    // (outside FLIGHT the C# also DestroyImmediate's vessel2.gameObject; the Vessel object lives on here and is reloaded below)
    if (resetCBRequired) {
        if (flag6 && Scene() != GameScenes::FLIGHT) {
            snapshot.Body()->inverseRotation = originalCBInverseRotation;
            snapshot.Body()->CBUpdate();
        } else if (!flag6) {
            targetBody->inverseRotation = originalCBInverseRotation;
            targetBody->CBUpdate();
        }
    }
    proto->Load(game->flightState.get(), vessel2.get());
    if (flag6 && Scene() == GameScenes::FLIGHT && Env::setVesselOrbit) Env::setVesselOrbit(*vessel2);
    if (Scene() != GameScenes::FLIGHT) {
        proto->persistentId = senv.persistentIds.CheckVesselpersistentId(proto->persistentId, nullptr, false, true);
        vessel2->persistentId = proto->persistentId;
        for (size_t k = 0; k < proto->protoPartSnapshots.size(); ++k) {
            ProtoPartSnapshot& pps = *proto->protoPartSnapshots[k];
            pps.persistentId = senv.persistentIds.CheckProtoPartSnapShotpersistentId(pps.persistentId, nullptr, false, true, proto->persistentId);
        }
        // flightState.protoVessels.Add(vessel2.protoVessel): the list owns its ProtoVessel objects here, the Vessel keeps its own: a ProtoVessel of the saved node joins the list
        ConfigNode registered;
        proto->Save(registered);
        game->flightState->protoVessels.push_back(std::make_unique<ProtoVessel>(registered, game));
        if (vesselSituation.focusOnSpawn) game->flightState->activeVesselIdx = static_cast<int>(game->flightState->protoVessels.size()) - 1;
        GE::onNewVesselCreated.Fire(vessel2.get());
    }
    Vessel* returnVessel = proto->vesselRef != nullptr ? proto->vesselRef : vessel2.get();
    if (Scene() == GameScenes::FLIGHT && !vesselSituation.focusOnSpawn) {
        Toggle(*vessel2, false);
        vessel2->GoOnRails();
        if (flag2) {
            vessel2->SetPosition(vector3d);
            vessel2->CoMD = vector3d;
            vessel2->CoM = vector3d.ToVector3f();
            proto->CoM = vessel2->CoM;
        }
    }
    // vessel2.gameObject.SetActive(true); StartCoroutine(ReactivateComponents(vessel2)): WaitForEndOfFrame, then ToggleVesselComponents(vessel, true)
    Vessel* raw = vessel2.get();
    CareerEnvironment::Current().Scheduler().StartCoroutine([](Vessel* v) -> Enumerator {
        co_yield YieldInstruction::WaitForEndOfFrame();
        if (v != nullptr && !v->parts.empty()) Toggle(*v, true);
    }(raw));
    if (Scene() == GameScenes::SPACECENTER && Env::resetKSCMarkers) Env::resetKSCMarkers();
    PreBuiltSpawnResult result;
    result.spawned = true;
    result.vessel = returnVessel;
    result.vesselConfigNode = node;
    spawned.push_back({std::move(vessel2)});
    GE::Contract::onContractPreBuiltVesselSpawned.Fire(returnVessel);
    return result;
}

void PreBuiltShipSpawner::Install() {
    PreBuiltEnvironment::spawnPreBuiltShip = [](PreBuiltCraftDefinition& d, const PreBuiltVesselSituation& s) { SpawnPreBuiltShip(d, s); };
}
void PreBuiltShipSpawner::Uninstall() { PreBuiltEnvironment::spawnPreBuiltShip = nullptr; }
void PreBuiltShipSpawner::Reset() {
    last = PreBuiltSpawnResult();
    spawned.clear();
}

}  // namespace ksp::Contracts::Lite
