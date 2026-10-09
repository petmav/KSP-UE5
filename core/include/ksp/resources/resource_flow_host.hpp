// WP-15 seam to the Vessel (WP-14) and the editor ShipConstruct (WP-18).
//
// PartSet.cs / Part.cs read these members of Vessel and ShipConstruct (Vessel.cs:451-464, 3913-4040; ShipConstruct.cs:46,
// 916-970): `parts`, `resourcePartSet`, `simulationResourcePartSet`, `crossfeedSets`, `simulationCrossfeedSets`,
// UpdateResourceSets / BuildCrossfeedPartSets / RequestResource / GetConnectedResourceTotals. The flow code does not depend
// on the Vessel class: a Vessel (or ShipConstruct) is seen through IResourceVessel, and the PartSet owners are the two
// helper classes below, ported from Vessel.cs / ShipConstruct.cs.
//
// How WP-14 plugs in:
//   * `class Vessel : public IResourceVessel` (or has a VesselResourceSets member that forwards the four calls);
//   * ResourceFlowEnvironment::vesselHost = [](Vessel* v) { return static_cast<IResourceVessel*>(v); };
//   * Vessel.UpdateResourceSets() -> `resourceSets.UpdateResourceSets(this, parts)`; the three GameEvents subscriptions of
//     Vessel.cs:1574-1577 (onPartResourceListChange, onPartCrossfeedStateChange, onPartPriorityChanged ->
//     UpdateResourceSetsEventCheckPart) stay in the Vessel (they test `p.vessel == this`), and Vessel.cs:1766/1851/3113/3898
//     call UpdateResourceSets at the same points as in C#.
// How WP-18 plugs in: ShipConstruct owns a ShipResourceSets (ShipConstruct.UpdateResourceSets / RequestResource /
// GetConnectedResourceTotals forward to it) and registers ResourceFlowEnvironment::shipHost.
#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "ksp/resources/flow_graph.hpp"
#include "ksp/resources/resource_types.hpp"

namespace ksp {

class Part;
class PartSet;
class Vessel;
class ShipConstruct;

// What PartSet / Part.requestResource use of a Vessel or a ShipConstruct.
class IResourceVessel {
public:
    virtual ~IResourceVessel() = default;
    // Vessel.parts / ShipConstruct.parts
    virtual const std::vector<Part*>& ResourceParts() const = 0;
    // Vessel.cs:3980 / ShipConstruct.cs:927
    virtual double RequestResource(Part* part, int id, double demand, bool usePriority, bool simulate) = 0;
    // Vessel.cs:3994 / ShipConstruct.cs:916
    virtual void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, bool pulling) = 0;
    // Vessel.cs:4022 (Part.GetConnectedResourceTotals with a threshold)
    virtual void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, double threshold, bool pulling) = 0;
};

// The process-wide state the flow code reads from other subsystems (HighLogic, CheatOptions, the Vessel registry).
struct ResourceFlowEnvironment {
    // Part.vessel / Part.ship (opaque pointers) -> the object's IResourceVessel; nullptr result = unknown
    static inline std::function<IResourceVessel*(Vessel*)> vesselHost;
    static inline std::function<IResourceVessel*(ShipConstruct*)> shipHost;
    static IResourceVessel* Host(Vessel* v) { return v && vesselHost ? vesselHost(v) : nullptr; }
    static IResourceVessel* Host(ShipConstruct* s) { return s && shipHost ? shipHost(s) : nullptr; }

    // HighLogic.LoadedSceneIsEditor / LoadedSceneIsFlight (SaveEnvironment::loadedScene)
    static bool LoadedSceneIsEditor();
    static bool LoadedSceneIsFlight();

    // CheatOptions.InfinitePropellant / InfiniteElectricity (CheatOptions.cs static bools, off by default)
    static inline bool InfinitePropellant = false;
    static inline bool InfiniteElectricity = false;

    // UIPartActionController.Instance.ItemListContains(part, false): the part's PAW is open (ResourceConverter status text).
    // Default: no UI, never open.
    static inline std::function<bool(const Part*)> partActionUIContains;
};

// Vessel.cs:451-464 + 3913-4040: the PartSets of a vessel and their (re)building.
class VesselResourceSets {
public:
    std::shared_ptr<PartSet> resourcePartSet;                    // Vessel.cs:451
    std::shared_ptr<PartSet> simulationResourcePartSet;          // :454
    std::vector<std::shared_ptr<PartSet>> crossfeedSets;          // :462
    std::vector<std::shared_ptr<PartSet>> simulationCrossfeedSets;  // :464
    bool resourcesDirty = false;                                  // Vessel.resourcesDirty

    // Vessel.cs:3913 UpdateResourceSets(): `vessel` is the owning Vessel (PartSet(Vessel v)), `parts` = vessel.parts
    void UpdateResourceSets(Vessel* vessel, const std::vector<Part*>& parts);
    // Vessel.cs:3935
    void UpdateResourceSetsIfDirty(Vessel* vessel, const std::vector<Part*>& parts) {
        if (resourcesDirty) UpdateResourceSets(vessel, parts);
    }
    // Vessel.cs:3966 BuildCrossfeedPartSets
    void BuildCrossfeedPartSets(Vessel* vessel, const std::vector<Part*>& parts);
    // Vessel.cs:3980
    double RequestResource(Part* part, int id, double demand, bool usePriority, bool simulate);
    // Vessel.cs:3994 / 4022
    void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, bool pulling);
    void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, double threshold, bool pulling);
};

// ShipConstruct.cs:46-970: the editor ship's resource set (simulation only: no Vessel in the editor).
class ShipResourceSets {
public:
    std::shared_ptr<PartSet> resourcePartSet;  // ShipConstruct.cs:46
    // ShipConstruct.cs:941 UpdateResourceSets(inputParts, sccGraph): `inputParts` null = the ship's parts; Part.ship of every
    // part is set to `ship`
    void UpdateResourceSets(ShipConstruct* ship, const std::vector<Part*>& shipParts, const std::vector<Part*>* inputParts = nullptr,
                            RUI::Algorithms::SCCFlowGraph* sccGraph = nullptr);
    double RequestResource(Part* part, int id, double demand, bool usePriority, bool simulate);  // :927
    void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, bool pulling);  // :916

private:
    PartHashSetPtr cachedResourcePartSetParts;  // :48
};

}  // namespace ksp
