// Craft -> Vessel: the part-creating half of ShipConstruct.LoadShip (ShipConstruct.cs:378-640) over WP-04's craft form, then
// ShipConstruction.AssembleForLaunch (ShipConstruction.cs:296-427, WP-18 owns the editor / launch-site side): Vessel component on
// the local root, Initialize(fromShipAssembly), naming, landed state, flight ids, crew list, reference transform.
//
// WP-13 left the part half in its test helper; WP-14 needs it in the library for the vessel_state gate. Not ported: the launch
// site placement (the caller positions the root part), the crew manifest, FlightGlobals.SetActiveVessel, analytics.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/orbit/orbit.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

struct CraftParts {
    std::vector<std::unique_ptr<Part>> parts;  // ship.parts order (parts[0] is the root)
    bool ok = false;
    std::string missing;  // the first part name without a prefab when !ok
};

// ShipConstruct.LoadShip part creation: Instantiate(prefab), PART values, MODULE / RESOURCE / PARTDATA nodes, then
// parent / children / symmetry / attach-node links and InitializeModules. WP-18 additions: `shipOwner` becomes `Part.ship` of every part
// (ShipConstruct.cs:443), and the EVENTS / ACTIONS / EFFECTS / VESSELNAMING nodes of the PART are loaded (:600-620).
CraftParts BuildCraftParts(const ShipConstruct& ship, const std::function<const AvailablePart*(const std::string&)>& partInfoByName,
                           ShipConstruct* shipOwner = nullptr);

struct AssembleOptions {
    // WP-18 (ShipConstruction.AssembleForLaunch parameters)
    bool fromShipAssembly = true;
    bool preCreate = false;
    bool orbiting = false;
    std::shared_ptr<Orbit> orbit;             // AssembleForLaunch(orbit): becomes the OrbitDriver's orbit before Initialize
    uint32_t launchID = 0;                    // HighLogic.CurrentGame.launchID (the caller post-increments it)
    const FlightState* flightState = nullptr; // GetUniqueFlightID(sceneState.flightState)
    std::function<void()> crewManifestAssign; // crewManifest.AssignCrewToVessel(ship) (the editor's crew manifest, called before RebuildCrewList)
    Vector3f rootPosition;                    // world position of the root part (the launch site placement)
    Quaternionf rootRotation = Quaternionf::identity();
    std::string landedAt = "LaunchPad";       // ShipConstruction.AssembleForLaunch landedAt
    std::string displaylandedAt = "";
    std::string flagURL = "";
    bool isLanded = true;
    bool isSplashed = false;
    bool setActiveVessel = false;             // FlightGlobals.SetActiveVessel (the registry's ActiveVessel is set when true)
};

// ShipConstruction.AssembleForLaunch (ShipConstruction.cs:296) over already created parts (parts[0] is the ship's first part, the root);
// WP-18 factored it out of AssembleVesselFromCraft. The ship supplies name, persistentId and the override arrays.
std::unique_ptr<Vessel> AssembleVesselFromParts(std::vector<std::unique_ptr<Part>> parts, const ShipConstruct& ship,
                                                const AssembleOptions& options);

// BuildCraftParts + AssembleVesselFromParts (fromShipAssembly = true). Returns null when a part has no prefab.
std::unique_ptr<Vessel> AssembleVesselFromCraft(const ShipConstruct& ship,
                                                const std::function<const AvailablePart*(const std::string&)>& partInfoByName,
                                                const AssembleOptions& options);

}  // namespace ksp
