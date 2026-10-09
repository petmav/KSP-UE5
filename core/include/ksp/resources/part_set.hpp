// PartSet.cs (WP-15): the resource flow solver.
//
// A PartSet is a set of parts (a crossfeed set of the flow graph, the whole vessel, or the whole editor ship) that resource
// requests are served from. For every (resource id, pull/push) it keeps a ResourcePrioritySet: the PartResource objects
// that are flowing in that direction, grouped by the owning part's resource priority (GetResourcePriority) in ascending
// priority order. RequestResource then drains the highest-numbered list first (`while (count-- > 0)`), either proportionally to
// the amounts in the list (when the list can cover the demand: this is what the *_BALANCE flow modes are named for, but
// PartSet does not look at the mode) or sequentially.
//
// The lists are built lazily (GetOrCreateList) and kept in step with PartResource.flowState / flowMode changes by the
// onPartResourceFlowStateChange / onPartResourceFlowModeChange listeners each PartSet registers (HookEvents).
//
// C++ notes:
//   * HashSet<Part> -> dotnet::HashSet<Part*> in a shared_ptr; PartSets built by BuildPartSets share the superset objects of
//     the SCCFlowGraph that produced them (targetParts is the same HashSet object, as in C#).
//   * The finalizer (`~PartSet` removes the event listeners) is the destructor: it runs when the last shared_ptr to the set
//     goes away, where C# runs it at some later garbage collection. A dead set can no longer see events; the C# finalizer
//     leaves it listening until collected, but a set that nothing references is never read.
#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/hash_set.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/resources/flow_graph.hpp"
#include "ksp/resources/part_resource_list.hpp"
#include "ksp/resources/resource_flow_host.hpp"

namespace ksp {

class Vessel;
class ShipConstruct;

class PartSet {
public:
    // PartSet.cs:9
    struct ResourcePrioritySet {
        std::vector<std::vector<PartResource*>> lists;  // :11 ascending priority
        std::unordered_set<PartResource*> set;          // :13 (only Contains / Add / Remove are used)
    };
    // PartSet.cs:17 [Flags]
    enum PartBuildSetOptions : int { Real = 1, Simulate = 2, Both = 3 };

    static inline int _id = 0;                          // :24
    int setId = 0;                                      // :26

    // :49 NextID
    static int NextID() { return _id++; }
    bool ListsExist() const { return true; }            // :51 (the dictionaries are never null here)

    explicit PartSet(PartHashSetPtr parts);             // :63
    explicit PartSet(Vessel* v);                        // :71
    PartSet(const PartSet& set);                        // :80 (a new id; the lists are not copied)
    explicit PartSet(ShipConstruct* shipRef);           // :96
    PartSet(ShipConstruct* shipRef, PartHashSetPtr newParts);  // :105
    PartSet& operator=(const PartSet&) = delete;
    virtual ~PartSet();                                 // :~PartSet (finalizer)

    virtual void GetConnectedResourceTotals(int id, double& amount, double& maxAmount, bool pulling) {  // :118
        GetConnectedResourceTotals(id, amount, maxAmount, pulling, false);
    }
    virtual void GetConnectedResourceTotals(int id, double& amount, double& maxAmount, bool pulling, bool simulate);  // :123
    virtual void GetConnectedResourceTotals(int id, double& amount, double& maxAmount, double threshold, bool pulling) {  // :154
        GetConnectedResourceTotals(id, amount, maxAmount, threshold, pulling, false);
    }
    virtual void GetConnectedResourceTotals(int id, double& amount, double& maxAmount, double threshold, bool pulling, bool simulate);  // :159
    virtual double RequestResource(Part* part, int id, double demand, bool usePri) { return RequestResource(part, id, demand, usePri, false); }  // :206
    virtual double RequestResource(Part* part, int id, double demand, bool usePri, bool simulate);  // :211
    ResourcePrioritySet* GetResourceList(int id, bool pulling, bool simulate) { return GetOrCreateList(id, pulling, simulate); }  // :222

    void HookEvents();                                  // :606
    virtual void RebuildVessel(Vessel* newVessel);      // :618
    virtual void RebuildVessel(ShipConstruct* newShip);  // :627
    virtual void RebuildVessel(ShipConstruct* newShip, PartHashSetPtr newParts);  // :636
    virtual void RebuildParts(PartHashSetPtr newParts);  // :649
    virtual void RemovePart(Part* part);                // :658
    virtual void RebuildInPlace();                      // :673

    // :680-:777 (a null vessel skips the vessel-level lists; the PartSets are stored on Part.crossfeedPartSet /
    // simulationCrossfeedPartSet and, with a vessel, in `crossfeedSets` / `simulationCrossfeedSets` of its VesselResourceSets)
    // `v` is the vessel's VesselResourceSets (C#: the Vessel whose crossfeedSets / simulationCrossfeedSets are filled), null for
    // none
    static void BuildPartSets(const std::vector<Part*>& parts, VesselResourceSets* v, bool simulate = false);  // :680 / :685
    static void BuildPartSets(const std::vector<Part*>& parts, VesselResourceSets* v, PartBuildSetOptions buildoptions,
                              RUI::Algorithms::SCCFlowGraph* sccGraph = nullptr);  // :690 / :695
    // :778 / :783
    static dotnet::HashSet<std::shared_ptr<PartSet>> BuildPartSimulationSets(const std::vector<Part*>& parts,
                                                                              RUI::Algorithms::SCCFlowGraph* sccGraph = nullptr);
    // :815 [Obsolete] AddPartToSet: recursive crossfeed walk without the flow graph
    static void AddPartToSet(PartHashSet& set, Part* p, Vessel* v);

    bool ContainsPart(Part* p);                         // :871
    PartHashSetPtr GetParts();                          // :888
    bool SetsEqual(const PartHashSet& set);             // :897

    // state read by tests / WP-14
    bool IsVesselWide() const { return vesselWide; }
    bool IsSimulationSet() const { return simulationSet; }
    Vessel* GetVessel() const { return vessel; }

protected:
    PartHashSetPtr targetParts;                         // :28
    Vessel* vessel = nullptr;                           // :30
    ShipConstruct* ship = nullptr;                      // :33
    bool vesselWide = false;                            // :36
    bool simulationSet = false;                         // :39
    dotnet::Dictionary<int, std::shared_ptr<ResourcePrioritySet>> pullList;  // :42
    dotnet::Dictionary<int, std::shared_ptr<ResourcePrioritySet>> pushList;  // :45
    static inline std::unordered_map<const PartHashSet*, std::shared_ptr<PartSet>> setsHolder;  // :47 Dictionary<HashSet<Part>, PartSet> (reference keys)

    ResourcePrioritySet* GetOrCreateList(int id, bool pulling) { return GetOrCreateList(id, pulling, false); }  // :217
    ResourcePrioritySet* GetOrCreateList(int id, bool pulling, bool simulate);  // :227
    double ProcessRequest(Part* part, ResourcePrioritySet* resList, double demand, bool usePri, bool pulling) {  // :260
        return ProcessRequest(part, resList, demand, usePri, pulling, false);
    }
    double ProcessRequest(Part* part, ResourcePrioritySet* resList, double demand, bool usePri, bool pulling, bool simulate);  // :265
    std::shared_ptr<ResourcePrioritySet> BuildResList(int id, bool pull) { return BuildResList(id, pull, false); }  // :441
    std::shared_ptr<ResourcePrioritySet> BuildResList(int id, bool pull, bool simulate);  // :446
    void AddPartResource(PartResource* r, dotnet::Dictionary<int, std::shared_ptr<ResourcePrioritySet>>& dict);     // :498
    void RemovePartResource(PartResource* r, dotnet::Dictionary<int, std::shared_ptr<ResourcePrioritySet>>& dict);  // :555
    void OnFlowStateChange(events::GameEvents::HostedFromToAction<PartResource*, bool> data);  // :906
    void OnFlowModeChange(events::GameEvents::HostedFromToAction<PartResource*, PartResource_FlowMode> data);  // :930

    friend class VesselResourceSets;
    friend class ShipResourceSets;
};

}  // namespace ksp
