// WP-20g: the flight scene's resource-flow host; see include/ksp/resources/vessel_resource_host.hpp. C# paths are relative to
// decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/resources/vessel_resource_host.hpp"

#include "ksp/events/game_events.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;

// One Vessel's resource fields (Vessel.cs:451-464) seen as the IResourceVessel the flow code asks (Vessel.cs:3980 / :3994 / :4022)
struct VesselResourceHost::Entry final : IResourceVessel {
    Vessel* vessel;
    VesselResourceSets sets;
    explicit Entry(Vessel* v) : vessel(v) {}
    const std::vector<Part*>& ResourceParts() const override { return vessel->parts; }
    // Vessel.cs:3980
    double RequestResource(Part* part, int id, double demand, bool usePriority, bool simulate) override {
        return sets.RequestResource(part, id, demand, usePriority, simulate);
    }
    // Vessel.cs:3994
    void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, bool pulling) override {
        sets.GetConnectedResourceTotals(id, simulate, amount, maxAmount, pulling);
    }
    // Vessel.cs:4022
    void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, double threshold, bool pulling) override {
        sets.GetConnectedResourceTotals(id, simulate, amount, maxAmount, threshold, pulling);
    }
};

VesselResourceHost::VesselResourceHost() = default;
VesselResourceHost::~VesselResourceHost() { Uninstall(); }

void VesselResourceHost::Install() {
    if (installed_) return;
    savedVesselHost_ = ResourceFlowEnvironment::vesselHost;
    savedUpdateResourceSets_ = Vessel::env.updateResourceSets;
    // Part.vessel -> the Vessel's own fields (the flow code's `vessel.RequestResource` / `vessel.GetConnectedResourceTotals`)
    ResourceFlowEnvironment::vesselHost = [this](Vessel* v) -> IResourceVessel* {
        if (v == nullptr) return nullptr;
        const bool fresh = entries_.find(v) == entries_.end();
        Entry& e = Get(*v);
        // kspcore: a vessel made before the host was installed never ran UpdateResourceSets with it; KSP's Vessel built its sets in
        // Initialize / Load (Vessel.cs:1766 / :1851), so they are built now (a request on a null resourcePartSet would throw)
        if (fresh && !e.sets.resourcePartSet) {
            e.sets.UpdateResourceSets(v, v->parts);
            ++rebuilds_;
        }
        return &e;
    };
    // Vessel.cs:3913 UpdateResourceSets (resourcesDirty = false is the Vessel's own statement): the PartSets rebuilt in place
    Vessel::env.updateResourceSets = [this](Vessel& v) {
        Get(v).sets.UpdateResourceSets(&v, v.parts);
        ++rebuilds_;
    };
    GE::onFlightGlobalsRemoveVessel.Add(this, &VesselResourceHost::OnRemoveVessel);
    installed_ = true;
}

void VesselResourceHost::Uninstall() {
    if (!installed_) return;
    GE::onFlightGlobalsRemoveVessel.Remove(this, &VesselResourceHost::OnRemoveVessel);
    ResourceFlowEnvironment::vesselHost = savedVesselHost_;
    Vessel::env.updateResourceSets = savedUpdateResourceSets_;
    savedVesselHost_ = nullptr;
    savedUpdateResourceSets_ = nullptr;
    // the parts of the vessels still listed drop their crossfeed sets (found through Part.allParts: a vessel destroyed without a registry
    // removal is never dereferenced; its parts' `vessel` was cleared by its destructor)
    for (Part* p : Part::allParts)
        if (p != nullptr && p->vessel != nullptr && entries_.count(p->vessel) != 0) {
            p->crossfeedPartSet.reset();
            p->simulationCrossfeedPartSet.reset();
        }
    entries_.clear();
    installed_ = false;
}

VesselResourceHost::Entry& VesselResourceHost::Get(Vessel& v) {
    auto it = entries_.find(&v);
    if (it == entries_.end()) it = entries_.emplace(&v, std::make_unique<Entry>(&v)).first;
    return *it->second;
}

VesselResourceSets* VesselResourceHost::Find(const Vessel& v) {
    const auto it = entries_.find(&v);
    return it == entries_.end() ? nullptr : &it->second->sets;
}

void VesselResourceHost::Remove(const Vessel* v) {
    const auto it = entries_.find(v);
    if (it == entries_.end()) return;
    // the parts' crossfeed sets (Part.crossfeedPartSet, built by BuildCrossfeedPartSets) belong to the vessel's: KSP destroys the parts
    // with the vessel; a kspcore part that outlives its vessel's registration must not keep a set that points at it
    for (Part* p : it->second->vessel->parts)
        if (p != nullptr && p->vessel == v) {
            p->crossfeedPartSet.reset();
            p->simulationCrossfeedPartSet.reset();
        }
    entries_.erase(it);
}

void VesselResourceHost::OnRemoveVessel(Vessel* v) { Remove(v); }

}  // namespace ksp
