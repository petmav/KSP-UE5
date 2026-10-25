// WP-38d: ContractSystem.SpawnPreBuiltShip / ConstructShip (Assembly-CSharp/Contracts/ContractSystem.cs:1120-1551): a pre-built craft file becomes a vessel in the game's flight
// state. The pipeline is WP-18's (ShipConstruct::LoadShipWithParts, ShipConstruction::AssembleForLaunch / PutShipToGround) and WP-19a's (ProtoVessel::Save / Load); what
// ConstructShip adds is ported here: the landed / splashed / orbiting placement, the proto vessel fixes after the assembly, the wheel brakes, the persistentId
// bookkeeping of the non-flight scenes, the flight state registration and onContractPreBuiltVesselSpawned.
//
// ConstructShip is an IEnumerator without a single `yield return` before its end (only `yield break`), so Unity runs it to completion inside StartCoroutine: it is a
// plain function here. Not ported (the contract templates never reach them: they build LANDED and ORBITING situations only, see prebuilt.hpp): the PRELAUNCH branches
// (launch sites and space center facilities, ContractSystem.cs:1213-1330: PSystemSetup.GetSpaceCenterFacility / GetLaunchSite, PQSCity.Orientate, MissionsUtils.GetFacilityLimit).
// Scene / engine seams (PreBuiltSpawnEnvironment): MissionSystem.putShiptoGround's terrain raycast, MissionSystem.ToggleVesselComponents (renderers, colliders),
// EditorLogic.MissionCheckLaunchClamps, MissionSystem.setVesselOrbitPosition, KSCVesselMarkers.RefreshMarkers.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/fineprint/prebuilt.hpp"
#include "ksp/save/game.hpp"

namespace ksp {
class ShipConstruct;
class Vessel;
class ProtoVessel;
}  // namespace ksp

namespace ksp::Contracts::Lite {

struct PreBuiltSpawnEnvironment {
    // MissionSystem.putShiptoGround(ship, vesselSituation, null, worldSurfacePosition, rotation) (MissionSystem.cs:1352): the physics raycast from 1500 m above the point
    // against the terrain colliders (layer 32768, 5000 m) moves the spawn point to the collider surface, then ShipConstruction.PutShipToGround. Unset: PutShipToGround on a
    // spawn point at (position, rotation) with no raycast (the terrain collider is the physics bubble's: WP-12b / WP-20e).
    static inline std::function<void(ShipConstruct&, const PreBuiltVesselSituation&, const Vector3f& position, const Quaternionf& rotation)> putShipToGround;
    // EditorLogic.MissionCheckLaunchClamps(ship, ship.parts[0].localRoot, vesselSituation, out ship[, overrideSituationCheck: true]) (EditorLogic.cs): false = the craft
    // has launch clamps that cannot stay. Unset: true (nothing is removed).
    static inline std::function<bool(ShipConstruct&, const PreBuiltVesselSituation&, bool overrideSituationCheck)> checkLaunchClamps;
    // PQSController.GetSurfaceHeight(nvector, overrideQuadBuildCheck: true). Unset: body->pqsController (no PQS: the C# NullReferenceException).
    static inline std::function<double(CelestialBody*, const Vector3d& nvector)> surfaceHeight;
    // MissionSystem.ToggleVesselComponents(vessel, toggle) (MissionSystem.cs:1276): renderers, skinned renderers, fairing and colliders. Unset: nothing.
    static inline std::function<void(Vessel&, bool)> toggleVesselComponents;
    // MissionSystem.setVesselOrbitPosition(vessel) (MissionSystem.cs:1312) with the FlightGlobals.overrideOrbit window (ContractSystem.setVesselOrbit :1527). Unset: nothing.
    static inline std::function<void(Vessel&)> setVesselOrbit;
    // MissionSystem.ResetKSCMarkers (SPACECENTER only). Unset: nothing.
    static inline std::function<void()> resetKSCMarkers;
    // HighLogic.LoadedScene. Unset: SaveEnvironment::Current().loadedScene.
    static inline std::function<GameScenes()> loadedScene;
    static void Reset();
};

// ContractSystem.returnVessel / vesselConfigNode (ContractSystem.cs, the last spawn) and the objects the spawns own
struct PreBuiltSpawnResult {
    bool spawned = false;
    Vessel* vessel = nullptr;                 // returnVessel
    ConfigNode::NodePtr vesselConfigNode;     // the proto vessel as it was saved before the reload
    std::string error;                        // the Debug.LogError text of the failure (empty when spawned)
};

class PreBuiltShipSpawner {
public:
    static inline PreBuiltSpawnResult last;   // returnVessel / vesselConfigNode of ContractSystem.Instance
    struct Owned {
        std::unique_ptr<Vessel> vessel;
    };
    static inline std::vector<Owned> spawned; // the Vessel objects (the scene owns them in the game)
    static void SpawnPreBuiltShip(PreBuiltCraftDefinition& craftDef, const PreBuiltVesselSituation& vesselSit);       // ContractSystem.cs:1120
    static PreBuiltSpawnResult ConstructShip(PreBuiltCraftDefinition& craftDef, const PreBuiltVesselSituation& vesselSituation);  // :1125
    // PreBuiltEnvironment::spawnPreBuiltShip = SpawnPreBuiltShip (the host calls it once a scene with parts exists; FinePrint::InstallAll does not)
    static void Install();
    static void Uninstall();
    static void Reset();
};

}  // namespace ksp::Contracts::Lite
