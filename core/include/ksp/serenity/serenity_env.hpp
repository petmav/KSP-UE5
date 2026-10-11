// WP-53 (Breaking Ground): everything the Serenity deployed-science / ROC code reads from systems that are other work packages, the
// physics engine or the UI. Each member is a seam: the owning package (or a test) installs it; the default is documented next to it.
// Same pattern as ContractsEnvironment (contracts/contract_environment.hpp) and TransmitterEnvironment (modules/module_data_transmitter.hpp).
//
// Real systems used directly (no seam): GameEvents (events/game_events.hpp), the persistentId registry of FlightGlobals
// (SaveEnvironment::Current().persistentIds; FlightGlobals.FindLoadedPart / FindUnloadedPart), ExpansionsLoader (SaveEnvironment),
// ResearchAndDevelopment, ScienceUtil, CommNetScenario / Vessel::connection, TimeWarp, the coroutine scheduler of ScienceEnvironment.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ksp {

class CelestialBody;
class Part;
class ProtoPartSnapshot;
class Vessel;
class ModuleCargoPart;

namespace Expansions::Serenity {

// Services of the Unity engine / UI that the Serenity code reaches. "Unset" says what happens then.
struct SerenityEnvironment {
    // ScreenMessages.PostScreenMessage(text, ...) texts, in order
    static inline std::vector<std::string> screenMessages;
    static void PostScreenMessage(std::string text) { screenMessages.push_back(std::move(text)); }

    // KSCVesselMarkers.fetch.RefreshMarkers() / vessel.orbitRenderer.RefreshMapObject() calls (counters)
    static inline int vesselMarkerRefreshes = 0;
    static inline int mapObjectRefreshes = 0;

    // ---- the EVA kerbal's ModuleInventoryPart (ModuleGroundPart.RetrievePart / CanBeStored / UpdateModuleUI) ----
    // FlightGlobals.ActiveVessel.parts[0].FindModuleImplementing<ModuleInventoryPart>(); unset = the active vessel has no inventory
    struct KerbalInventory {
        std::function<bool(Part&)> hasCapacity;                 // ModuleInventoryPart.HasCapacity(part)
        std::function<int()> firstEmptySlot;                    // ModuleInventoryPart.FirstEmptySlot()
        std::function<bool(Part&, int slot)> storeCargoPartAtSlot;  // StoreCargoPartAtSlot(part, slot)
        bool partBeingRetrieved = false;                        // ModuleInventoryPart.partBeingRetrieved
    };
    static inline std::shared_ptr<KerbalInventory> activeKerbalInventory;

    // ---- physics / scene seams of ModuleGroundPart (ModuleGroundPart.cs:236-319) ----
    // part.GroundContact (Part.cs); unset = true
    static inline std::function<bool(Part&)> groundContact;
    // part.Rigidbody.constraints = FreezeAll / None, part.PermanentGroundContact = true, autoStrutMode = ForceGrandparent (calls recorded in order)
    static inline std::function<void(Part&, const std::string& what)> physicsAction;
    static inline std::vector<std::string> physicsActions;
    // gameObject.AddComponent<FixedJoint>() / Destroy(joint): the number of live joints per module is tracked by the module
    // Vessel.srf_velocity.magnitude of the module's vessel (MakePartSettle); unset = Vessel::srf_velocity
    static inline std::function<double(Vessel&)> srfVelocityMagnitude;

    // ---- DeployedScienceExperiment seismic processing ----
    // part.rb.velocity.magnitude of a loaded part (processImpactVesselPart); nullopt (or unset) = no Rigidbody: the event velocity is used
    static inline std::function<std::optional<float>(ksp::Part&)> partRbVelocityMagnitude;

    // ---- sun tracking (ModuleGroundSciencePart.FixedUpdate / CalculateTrackingLOS): the Physics.Raycasts and the panel transform ----
    // The result of CalculateTrackingLOS(trackingDirection, ref blocker); unset = true (line of sight is clear)
    static inline std::function<bool(ksp::Part&, std::string& blocker)> trackingLineOfSight;
    // Part.FindModelTransform(name) != null (secondaryTransform / pivot lookups); unset = false
    static inline std::function<bool(ksp::Part&, const std::string& transformName)> hasModelTransform;
    // SunTrackerToOrigin's per-frame Quaternion.RotateTowards(panelRotationTransform.localRotation, rotationAngle, maxDegreesDelta) step: the engine
    // applies it and returns true once the pivot's localRotation == the packing rotation. Unset = already there.
    static inline std::function<bool(ksp::Part&)> panelAtOrigin;

    // ---- ROC world (PQS quad meshes, instantiated prefabs) ----
    // the prefab renderer bounds of a ROC type: (bounds.size.y, bounds.extents.y); unset = (1, 0.5)
    static inline std::function<std::pair<float, float>(const std::string& rocType)> rocPrefabBoundsY;

    static void Reset();
};

}  // namespace Expansions::Serenity
}  // namespace ksp
