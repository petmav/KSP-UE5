// Vessel.cs / ShipConstruct.cs resource-set members (WP-15 seam, see ksp/resources/resource_flow_host.hpp).
#include "ksp/resources/resource_flow_host.hpp"

#include "ksp/parts/part.hpp"
#include "ksp/resources/part_set.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

bool ResourceFlowEnvironment::LoadedSceneIsEditor() { return SaveEnvironment::Current().loadedScene == GameScenes::EDITOR; }
bool ResourceFlowEnvironment::LoadedSceneIsFlight() { return SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT; }

// ---- VesselResourceSets (Vessel.cs) -----------------------------------------------------------------------------------------

// Vessel.cs:3913
void VesselResourceSets::UpdateResourceSets(Vessel* vessel, const std::vector<Part*>& parts) {
    resourcesDirty = false;
    if (!resourcePartSet) resourcePartSet = std::make_shared<PartSet>(vessel);
    else resourcePartSet->RebuildVessel(vessel);
    if (!simulationResourcePartSet) simulationResourcePartSet = std::make_shared<PartSet>(vessel);
    else simulationResourcePartSet->RebuildVessel(vessel);
    BuildCrossfeedPartSets(vessel, parts);
}

// Vessel.cs:3966 (the verbose-log line depends on GameSettings.VERBOSE_DEBUG_LOG and the vessel name: not modelled)
void VesselResourceSets::BuildCrossfeedPartSets(Vessel* /*vessel*/, const std::vector<Part*>& parts) {
    PartSet::BuildPartSets(parts, this, PartSet::Both, nullptr);
}

// Vessel.cs:3980
double VesselResourceSets::RequestResource(Part* part, int id, double demand, bool usePriority, bool simulate) {
    if (simulate) return simulationResourcePartSet->RequestResource(part, id, demand, usePriority, simulate);
    return resourcePartSet->RequestResource(part, id, demand, usePriority, simulate);
}

// Vessel.cs:3994
void VesselResourceSets::GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, bool pulling) {
    if (simulate) {
        if (simulationResourcePartSet) {
            simulationResourcePartSet->GetConnectedResourceTotals(id, amount, maxAmount, pulling, simulate);
            return;
        }
        amount = 0.0;
        maxAmount = 0.0;
    } else if (resourcePartSet) {
        resourcePartSet->GetConnectedResourceTotals(id, amount, maxAmount, pulling, simulate);
    } else {
        amount = 0.0;
        maxAmount = 0.0;
    }
}

// Vessel.cs:4022
void VesselResourceSets::GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, double threshold, bool pulling) {
    if (simulate) {
        if (simulationResourcePartSet) {
            simulationResourcePartSet->GetConnectedResourceTotals(id, amount, maxAmount, threshold, pulling, simulate);
            return;
        }
        amount = 0.0;
        maxAmount = 0.0;
    } else if (resourcePartSet) {
        resourcePartSet->GetConnectedResourceTotals(id, amount, maxAmount, threshold, pulling, simulate);
    } else {
        amount = 0.0;
        maxAmount = 0.0;
    }
}

// ---- ShipResourceSets (ShipConstruct.cs) -------------------------------------------------------------------------------------

// ShipConstruct.cs:916
void ShipResourceSets::GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, bool pulling) {
    if (resourcePartSet) {
        resourcePartSet->GetConnectedResourceTotals(id, amount, maxAmount, pulling, simulate);
        return;
    }
    amount = 0.0;
    maxAmount = 0.0;
}

// ShipConstruct.cs:927
double ShipResourceSets::RequestResource(Part* part, int id, double demand, bool usePriority, bool simulate) {
    if (resourcePartSet) return resourcePartSet->RequestResource(part, id, demand, usePriority, simulate);
    return 0.0;
}

// ShipConstruct.cs:941
void ShipResourceSets::UpdateResourceSets(ShipConstruct* ship, const std::vector<Part*>& shipParts, const std::vector<Part*>* inputParts,
                                          RUI::Algorithms::SCCFlowGraph* sccGraph) {
    const std::vector<Part*>& input = inputParts != nullptr ? *inputParts : shipParts;
    if (input.empty()) return;
    auto b = std::make_shared<PartHashSet>(input);
    if (!cachedResourcePartSetParts || !KSPUtil::DeepCompare(*cachedResourcePartSetParts, *b)) {
        PartSet::BuildPartSets(input, nullptr, PartSet::Simulate, sccGraph);
        cachedResourcePartSetParts = b;
        for (Part* p : input) p->ship = ship;
        if (!resourcePartSet) resourcePartSet = std::make_shared<PartSet>(ship, cachedResourcePartSetParts);
        else resourcePartSet->RebuildVessel(ship, std::make_shared<PartHashSet>(input));
    }
}

}  // namespace ksp
