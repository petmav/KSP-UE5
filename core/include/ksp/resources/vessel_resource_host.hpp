// WP-20g: the flight scene's resource-flow host: the PartSets a KSP Vessel owns (Vessel.cs:451-464 resourcePartSet,
// simulationResourcePartSet, crossfeedSets, simulationCrossfeedSets) for every Vessel of the scene, behind the two WP-15 seams
// ResourceFlowEnvironment::vesselHost (Part.vessel -> the vessel's IResourceVessel: Vessel.RequestResource / GetConnectedResourceTotals,
// Vessel.cs:3980 / :3994 / :4022) and Vessel::env.updateResourceSets (Vessel.UpdateResourceSets, Vessel.cs:3913).
//
// KSP's Vessel holds these fields itself; kspcore's Vessel reaches them through the seams (WP-14 / WP-15), so a flight host owns one
// VesselResourceSets per Vessel. The call sites are the Vessel's own (kspcore ports them with the C#): Vessel.Initialize (Vessel.cs:1766),
// Vessel.Load (:1851), Vessel.Update's dirty check (:3111-3113), onVesselPartCountChanged (:3892-3903), the resource-list / crossfeed /
// priority events (:1574-1577 -> :3951 UpdateResourceSetsEventCheckPart), BuildCrossfeedPartSets on fuel-lookup changes (:3943).
// The physics bubble installs one (PhysicsBubbleOptions::resourceHost, on by default); a test that wires its own seams before or after
// the bubble keeps them (Install saves and Uninstall restores what was there).
//
// Lifetime: an entry is made at the vessel's first UpdateResourceSets (or its first resource request: then its sets are built at once, a
// kspcore addition for a vessel that existed before the host was installed) and dropped on onFlightGlobalsRemoveVessel (the Vessel's
// OnDestroy removes it from the registry, so its PartSets never outlive its parts).
#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <memory>

#include "ksp/resources/resource_flow_host.hpp"

namespace ksp {

class Vessel;

class VesselResourceHost {
public:
    VesselResourceHost();
    ~VesselResourceHost();  // Uninstall
    VesselResourceHost(const VesselResourceHost&) = delete;
    VesselResourceHost& operator=(const VesselResourceHost&) = delete;

    void Install();
    void Uninstall();
    bool Installed() const { return installed_; }

    // the vessel's sets (nullptr: none made yet)
    VesselResourceSets* Find(const Vessel& v);
    // drops the vessel's entry (its PartSets point at its parts)
    void Remove(const Vessel* v);
    size_t Count() const { return entries_.size(); }
    size_t Rebuilds() const { return rebuilds_; }  // UpdateResourceSets calls served

private:
    struct Entry;
    Entry& Get(Vessel& v);
    void OnRemoveVessel(Vessel* v);

    std::map<const Vessel*, std::unique_ptr<Entry>> entries_;
    bool installed_ = false;
    size_t rebuilds_ = 0;
    std::function<IResourceVessel*(Vessel*)> savedVesselHost_;
    std::function<void(Vessel&)> savedUpdateResourceSets_;
};

}  // namespace ksp
