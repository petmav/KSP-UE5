// PartSet.cs (WP-15): see ksp/resources/part_set.hpp.
#include "ksp/resources/part_set.hpp"

#include <algorithm>

#include "ksp/core/log.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/util/collections.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

using GE = events::GameEvents;
using RUI::Algorithms::SCCFlowGraph;

// ---- constructors ---------------------------------------------------------------------------------------------------------

// PartSet.cs:63
PartSet::PartSet(PartHashSetPtr parts) {
    setId = NextID();
    targetParts = std::move(parts);
    vesselWide = false;
    HookEvents();
}

// PartSet.cs:71
PartSet::PartSet(Vessel* v) {
    setId = NextID();
    vesselWide = true;
    ship = nullptr;
    vessel = v;
    HookEvents();
}

// PartSet.cs:80
PartSet::PartSet(const PartSet& set) {
    setId = NextID();
    vesselWide = set.vesselWide;
    if (vesselWide) {
        vessel = set.vessel;
        ship = set.ship;
    } else {
        targetParts = std::make_shared<PartHashSet>(*set.targetParts);
    }
    HookEvents();
}

// PartSet.cs:96
PartSet::PartSet(ShipConstruct* shipRef) {
    setId = NextID();
    vesselWide = true;
    vessel = nullptr;
    ship = shipRef;
    HookEvents();
}

// PartSet.cs:105
PartSet::PartSet(ShipConstruct* shipRef, PartHashSetPtr newParts) {
    setId = NextID();
    vesselWide = true;
    vessel = nullptr;
    ship = shipRef;
    if (newParts) targetParts = std::move(newParts);
    HookEvents();
}

// PartSet.cs:606
void PartSet::HookEvents() {
    GE::onPartResourceFlowStateChange.Add(this, &PartSet::OnFlowStateChange);
    GE::onPartResourceFlowModeChange.Add(this, &PartSet::OnFlowModeChange);
}

// PartSet.cs (finalizer)
PartSet::~PartSet() {
    GE::onPartResourceFlowStateChange.Remove(this, &PartSet::OnFlowStateChange);
    GE::onPartResourceFlowModeChange.Remove(this, &PartSet::OnFlowModeChange);
}

// ---- totals ---------------------------------------------------------------------------------------------------------------

// PartSet.cs:123
void PartSet::GetConnectedResourceTotals(int id, double& amount, double& maxAmount, bool pulling, bool simulate) {
    double num = 0.0;
    maxAmount = 0.0;
    amount = num;
    ResourcePrioritySet* orCreateList = GetOrCreateList(id, pulling, simulate);
    if (orCreateList == nullptr) return;
    int count = static_cast<int>(orCreateList->lists.size());
    while (count-- > 0) {
        std::vector<PartResource*>& list = orCreateList->lists[static_cast<size_t>(count)];
        int count2 = static_cast<int>(list.size());
        while (count2-- > 0) {
            PartResource* partResource = list[static_cast<size_t>(count2)];
            if (pulling) amount += partResource->amount;
            else amount += partResource->maxAmount - partResource->amount;
            maxAmount += partResource->maxAmount;
        }
    }
}

// PartSet.cs:159
void PartSet::GetConnectedResourceTotals(int id, double& amount, double& maxAmount, double threshold, bool pulling, bool simulate) {
    double num = 0.0;
    maxAmount = 0.0;
    amount = num;
    ResourcePrioritySet* orCreateList = GetOrCreateList(id, pulling, simulate);
    if (orCreateList == nullptr) return;
    int count = static_cast<int>(orCreateList->lists.size());
    while (count-- > 0) {
        std::vector<PartResource*>& list = orCreateList->lists[static_cast<size_t>(count)];
        int count2 = static_cast<int>(list.size());
        while (count2-- > 0) {
            PartResource* partResource = list[static_cast<size_t>(count2)];
            if (pulling) {
                double num2 = partResource->maxAmount * threshold;
                double num3 = partResource->maxAmount * (1.0 - num2);
                if (!(num3 < 1E-09)) {
                    double num4 = partResource->amount - num2;
                    if (num4 > 1E-09) amount += num4;
                    maxAmount += num3;
                }
                continue;
            }
            double num5 = partResource->maxAmount * threshold;
            if (!(num5 < 1E-09)) {
                double num6 = num5 - partResource->amount;
                if (num6 > 1E-09) amount += num6;
                maxAmount += num5;
            }
        }
    }
}

// PartSet.cs:211
double PartSet::RequestResource(Part* part, int id, double demand, bool usePri, bool simulate) {
    bool pulling = demand > 0.0;
    return ProcessRequest(part, GetOrCreateList(id, pulling, simulate), demand, usePri, pulling, simulate);
}

// PartSet.cs:227
PartSet::ResourcePrioritySet* PartSet::GetOrCreateList(int id, bool pulling, bool simulate) {
    simulationSet = simulate;
    std::shared_ptr<ResourcePrioritySet> value;
    if (pulling) {
        if (!pullList.TryGetValue(id, value)) {
            value = BuildResList(id, true, simulate);
            pullList.Set(id, value);
        }
    } else {
        if (!pushList.TryGetValue(id, value)) {
            value = BuildResList(id, false, simulate);
            pushList.Set(id, value);
        }
    }
    return value.get();
}

// ---- the request solver -----------------------------------------------------------------------------------------------------

// PartSet.cs:265
double PartSet::ProcessRequest(Part* part, ResourcePrioritySet* resList, double demand, bool usePri, bool pulling, bool simulate) {
    if (resList == nullptr) {
        log::Info("PArtSet]: Unable to process resource request. PartSet has not been setup correctly.");
        return 0.0;
    }
    const double threshold = part->fields.resourceRequestRemainingThreshold;
    double num = demand;
    if (usePri) {
        int count = static_cast<int>(resList->lists.size());
        while (count-- > 0) {
            std::vector<PartResource*>& list = resList->lists[static_cast<size_t>(count)];
            double num2 = 0.0;
            double num3 = num;
            double num4 = 0.0;
            int count2 = static_cast<int>(list.size());
            while (count2-- > 0) {
                PartResource* partResource = list[static_cast<size_t>(count2)];
                num2 = !pulling ? num2 + (partResource->amount - partResource->maxAmount) : num2 + partResource->amount;
            }
            if (num2 == 0.0) continue;
            bool flag = false;
            if (pulling) {
                if (num2 > num) flag = true;
            } else if (num2 < num) {
                flag = true;
            }
            if (flag) {
                double num5 = (0.0 - num) / num2;
                int count3 = static_cast<int>(list.size());
                while (count3-- > 0) {
                    PartResource* partResource = list[static_cast<size_t>(count3)];
                    double num6 = num5;
                    num6 = !pulling ? num6 * (partResource->amount - partResource->maxAmount) : num6 * partResource->amount;
                    num4 += partResource->part->TransferResource(*partResource, num6, part, simulate);
                }
                num -= num4;
            }
            bool flag2 = true;
            if (!flag || num == num3) {
                num4 = 0.0;
                flag2 = false;
                int count4 = static_cast<int>(list.size());
                while (count4-- > 0) {
                    PartResource* partResource = list[static_cast<size_t>(count4)];
                    num4 += partResource->part->TransferResource(*partResource, 0.0 - demand, part, simulate);
                    double num7 = num - num4;
                    if (pulling) {
                        if (num7 <= threshold) return demand - num7;
                    } else if (!(num7 < 0.0 - threshold)) {
                        return demand - num7;
                    }
                }
                num -= num4;
            }
            if (!flag2) continue;
            if (pulling) {
                if (num <= threshold) return demand - num;
            } else if (!(num < 0.0 - threshold)) {
                return demand - num;
            }
        }
    } else {
        double num8 = 0.0;
        int count5 = static_cast<int>(resList->lists.size());
        while (count5-- > 0) {
            std::vector<PartResource*>& list = resList->lists[static_cast<size_t>(count5)];
            int count6 = static_cast<int>(list.size());
            while (count6-- > 0) {
                PartResource* partResource = list[static_cast<size_t>(count6)];
                num8 = !pulling ? num8 + (partResource->amount - partResource->maxAmount) : num8 + partResource->amount;
            }
        }
        if (num8 == 0.0) return 0.0;
        bool flag3 = false;
        if (pulling) {
            if (num8 > num) flag3 = true;
        } else if (num8 < num) {
            flag3 = true;
        }
        double num9 = num;
        double num10 = 0.0;
        if (flag3) {
            double num11 = (0.0 - num) / num8;
            int count7 = static_cast<int>(resList->lists.size());
            while (count7-- > 0) {
                std::vector<PartResource*>& list = resList->lists[static_cast<size_t>(count7)];
                int count8 = static_cast<int>(list.size());
                while (count8-- > 0) {
                    PartResource* partResource = list[static_cast<size_t>(count8)];
                    double num12 = num11;
                    num12 = !pulling ? num12 * (partResource->amount - partResource->maxAmount) : num12 * partResource->amount;
                    num10 += partResource->part->TransferResource(*partResource, num12, part, simulate);
                }
            }
            num -= num10;
        }
        if (!flag3 || num == num9) {
            num10 = 0.0;
            int count9 = static_cast<int>(resList->lists.size());
            while (count9-- > 0) {
                std::vector<PartResource*>& list = resList->lists[static_cast<size_t>(count9)];
                int count10 = static_cast<int>(list.size());
                while (count10-- > 0) {
                    PartResource* partResource = list[static_cast<size_t>(count10)];
                    num10 += partResource->part->TransferResource(*partResource, 0.0 - demand, part, simulate);
                    double num13 = num - num10;
                    if (pulling) {
                        if (num13 <= threshold) return demand - num13;
                    } else if (!(num13 < 0.0 - threshold)) {
                        return demand - num13;
                    }
                }
            }
            num -= num10;
        }
    }
    return demand - num;
}

// ---- list building and maintenance ----------------------------------------------------------------------------------------

// PartSet.cs:446
std::shared_ptr<PartSet::ResourcePrioritySet> PartSet::BuildResList(int id, bool pull, bool simulate) {
    simulationSet = simulate;
    const std::vector<Part*>* vesselParts = nullptr;
    std::vector<Part*> targetList;  // enumeration of the HashSet in its order
    const std::vector<Part*>* enumerator = nullptr;
    if (simulate && targetParts && targetParts->Count() > 0) {
        targetList = targetParts->ToList();
        enumerator = &targetList;
    } else if (vesselWide) {
        if (vessel != nullptr) {
            if (IResourceVessel* h = ResourceFlowEnvironment::Host(vessel)) vesselParts = &h->ResourceParts();
        } else if (ship != nullptr) {
            if (IResourceVessel* h = ResourceFlowEnvironment::Host(ship)) vesselParts = &h->ResourceParts();
        }
        enumerator = vesselParts;
    } else {
        if (!targetParts) throw NullReferenceException("Object reference not set to an instance of an object");
        targetList = targetParts->ToList();
        enumerator = &targetList;
    }
    ListDictionary<int, PartResource*> listDictionary;
    auto resourcePrioritySet = std::make_shared<ResourcePrioritySet>();
    if (enumerator != nullptr) {
        for (size_t k = 0; k < enumerator->size(); k++) {
            Part* current = (*enumerator)[k];
            PartResource* partResource = simulate ? current->SimulationResources().Get(id) : current->Resources.Get(id);
            if (partResource != nullptr && partResource->Flowing(pull)) {
                int resourcePriority = current->GetResourcePriority();
                listDictionary.Add(resourcePriority, partResource);
                resourcePrioritySet->set.insert(partResource);
            }
        }
    }
    std::vector<int> list = listDictionary.Keys();
    dotnet::ListSort(list, [](int a, int b) { return a < b ? -1 : (a > b ? 1 : 0); });  // List<int>.Sort()
    for (size_t i = 0; i < list.size(); i++) resourcePrioritySet->lists.push_back(*listDictionary[list[i]]);
    return resourcePrioritySet;
}

namespace {
// ListExtension.cs:159 ContainsId(List<Part>, Part) / :199 ContainsId(HashSet<Part>, Part): persistentId match (util::ContainsId reads a
// `persistentId` member; Part keeps it in `fields`)
template <class Range>
bool ContainsPersistentId(const Range& parts, const Part* p) {
    for (const Part* e : parts) {
        if (e == nullptr) continue;
        if (p == nullptr) throw NullReferenceException("Object reference not set to an instance of an object");
        if (e->fields.persistentId == p->fields.persistentId) return true;
    }
    return false;
}
std::string PartTitle(const Part* p) { return p->partInfo != nullptr ? p->partInfo->title : std::string(); }
}  // namespace

// PartSet.cs:498
void PartSet::AddPartResource(PartResource* r, dotnet::Dictionary<int, std::shared_ptr<ResourcePrioritySet>>& dict) {
    if (simulationSet && !r->simulationResource) {
        r->part->ResetSimulationResources();
        PartResource* partResource = r->part->SimulationResources().Get(r->info->id());
        if (partResource == nullptr) {
            log::Info("[PartSet]: Failed to add Resource " + std::to_string(r->info->id()) + " to Simulation PartSet:" + std::to_string(setId) +
                      " as corresponding Part " + PartTitle(r->part) + "-" + std::to_string(r->part->fields.persistentId) +
                      " SimulationResource was not found.");
            return;
        }
        r = partResource;
    }
    std::shared_ptr<ResourcePrioritySet> value;
    if (!dict.TryGetValue(r->info->id(), value) || value->set.count(r) != 0) return;
    int resourcePriority = r->part->GetResourcePriority();
    int num = -1;
    int count = static_cast<int>(value->lists.size());
    while (count-- > 0) {
        int resourcePriority2 = value->lists[static_cast<size_t>(count)][0]->part->GetResourcePriority();
        if (resourcePriority2 < resourcePriority) break;
        if (resourcePriority2 != resourcePriority) {
            num = count;
            continue;
        }
        value->lists[static_cast<size_t>(count)].push_back(r);
        value->set.insert(r);
        if (!simulationSet) r->part->ResetSimulationResources();
        return;
    }
    std::vector<PartResource*> list;
    list.push_back(r);
    if (num < 0) value->lists.push_back(std::move(list));
    else value->lists.insert(value->lists.begin() + num, std::move(list));
    value->set.insert(r);
    if (!simulationSet) r->part->ResetSimulationResources();
}

// PartSet.cs:555
void PartSet::RemovePartResource(PartResource* r, dotnet::Dictionary<int, std::shared_ptr<ResourcePrioritySet>>& dict) {
    if (simulationSet && !r->simulationResource) {
        r->part->ResetSimulationResources();
        PartResource* partResource = r->part->SimulationResources().Get(r->info->id());
        if (partResource == nullptr) {
            log::Info("[PartSet]: Failed to remove Resource " + std::to_string(r->info->id()) + " from Simulation PartSet:" +
                      std::to_string(setId) + " as corresponding Part " + PartTitle(r->part) + "-" +
                      std::to_string(r->part->fields.persistentId) + " SimulationResource was not found.");
            return;
        }
        r = partResource;
    }
    std::shared_ptr<ResourcePrioritySet> value;
    if (!dict.TryGetValue(r->info->id(), value) || value->set.count(r) == 0) return;
    int resourcePriority = r->part->GetResourcePriority();
    int count = static_cast<int>(value->lists.size());
    std::vector<PartResource*>* list;
    do {
        if (count-- > 0) {
            list = &value->lists[static_cast<size_t>(count)];
            continue;
        }
        return;
    } while (list->empty() || (*list)[0]->part->GetResourcePriority() != resourcePriority);
    int count2 = static_cast<int>(list->size());
    do {
        if (count2-- <= 0) return;
    } while ((*list)[static_cast<size_t>(count2)] != r);
    list->erase(list->begin() + count2);
    value->set.erase(r);
    if (list->empty()) value->lists.erase(value->lists.begin() + count);
    if (!simulationSet) r->part->ResetSimulationResources();
}

// ---- rebuilding -------------------------------------------------------------------------------------------------------------

// PartSet.cs:618
void PartSet::RebuildVessel(Vessel* newVessel) {
    vessel = newVessel;
    vesselWide = true;
    setId = NextID();
    pushList.Clear();
    pullList.Clear();
}

// PartSet.cs:627
void PartSet::RebuildVessel(ShipConstruct* newShip) {
    ship = newShip;
    vesselWide = true;
    setId = NextID();
    pushList.Clear();
    pullList.Clear();
}

// PartSet.cs:636
void PartSet::RebuildVessel(ShipConstruct* newShip, PartHashSetPtr newParts) {
    ship = newShip;
    vesselWide = true;
    setId = NextID();
    if (newParts) targetParts = std::move(newParts);
    pushList.Clear();
    pullList.Clear();
}

// PartSet.cs:649
void PartSet::RebuildParts(PartHashSetPtr newParts) {
    targetParts = std::move(newParts);
    setId = NextID();
    vesselWide = false;
    pushList.Clear();
    pullList.Clear();
}

// PartSet.cs:658
void PartSet::RemovePart(Part* part) {
    if (!targetParts) throw NullReferenceException("Object reference not set to an instance of an object");
    auto hashSet = std::make_shared<PartHashSet>();
    for (Part* current : *targetParts)
        if (current->fields.persistentId != part->fields.persistentId) hashSet->Add(current);
    RebuildParts(hashSet);
}

// PartSet.cs:673
void PartSet::RebuildInPlace() {
    setId = NextID();
    pushList.Clear();
    pullList.Clear();
}

// ---- BuildPartSets ----------------------------------------------------------------------------------------------------------

// PartSet.cs:680, :685
void PartSet::BuildPartSets(const std::vector<Part*>& parts, VesselResourceSets* v, bool simulate) {
    BuildPartSets(parts, v, !simulate ? Real : Simulate, nullptr);
}

// PartSet.cs:690 / :695
void PartSet::BuildPartSets(const std::vector<Part*>& parts, VesselResourceSets* v, PartBuildSetOptions buildoptions, SCCFlowGraph* sccGraph) {
    std::unique_ptr<SCCFlowGraph> owned;
    if (sccGraph == nullptr) {
        owned = std::make_unique<SCCFlowGraph>(parts);
        sccGraph = owned.get();
    }
    bool flag = v != nullptr;
    if (flag) {
        if ((buildoptions & Real) == Real) v->crossfeedSets.clear();
        if ((buildoptions & Simulate) == Simulate) v->simulationCrossfeedSets.clear();
    }
    int count = static_cast<int>(parts.size());
    while (count-- > 0) {
        Part* part = parts[static_cast<size_t>(count)];
        PartHashSetPtr allRequests = sccGraph->GetAllRequests(part);
        auto it = setsHolder.find(allRequests.get());
        if (it != setsHolder.end()) {
            std::shared_ptr<PartSet> value = it->second;
            if ((buildoptions & Real) == Real) {
                value->simulationSet = false;
                part->crossfeedPartSet = value;
            }
            if ((buildoptions & Simulate) == Simulate) {
                std::shared_ptr<PartSet> partSet = (buildoptions != Both) ? value : std::make_shared<PartSet>(*value);
                partSet->simulationSet = true;
                part->simulationCrossfeedPartSet = partSet;
            }
            continue;
        }
        std::shared_ptr<PartSet> value = std::make_shared<PartSet>(allRequests);
        if (flag) {
            if ((buildoptions & Real) == Real) {
                value->simulationSet = false;
                v->crossfeedSets.push_back(value);
            }
            if ((buildoptions & Simulate) == Simulate) {
                std::shared_ptr<PartSet> partSet = (buildoptions != Both) ? value : std::make_shared<PartSet>(*value);
                partSet->simulationSet = true;
                v->simulationCrossfeedSets.push_back(partSet);
            }
        }
        if ((buildoptions & Real) == Real) {
            value->simulationSet = false;
            part->crossfeedPartSet = value;
        }
        if ((buildoptions & Simulate) == Simulate) {
            std::shared_ptr<PartSet> partSet = (buildoptions != Both) ? value : std::make_shared<PartSet>(*value);
            partSet->simulationSet = true;
            part->simulationCrossfeedPartSet = partSet;
        }
        setsHolder[allRequests.get()] = value;
    }
    setsHolder.clear();
}

// PartSet.cs:778 / :783
dotnet::HashSet<std::shared_ptr<PartSet>> PartSet::BuildPartSimulationSets(const std::vector<Part*>& parts, SCCFlowGraph* sccGraph) {
    std::unique_ptr<SCCFlowGraph> owned;
    if (sccGraph == nullptr) {
        owned = std::make_unique<SCCFlowGraph>(parts);
        sccGraph = owned.get();
    }
    dotnet::HashSet<std::shared_ptr<PartSet>> hashSet;
    int count = static_cast<int>(parts.size());
    while (count-- > 0) {
        Part* part = parts[static_cast<size_t>(count)];
        PartHashSetPtr allRequests = sccGraph->GetAllRequests(part);
        auto it = setsHolder.find(allRequests.get());
        if (it != setsHolder.end()) {
            std::shared_ptr<PartSet> value = it->second;
            hashSet.Add(value);
            part->simulationCrossfeedPartSet = value;
            value->simulationSet = true;
        } else {
            std::shared_ptr<PartSet> value = std::make_shared<PartSet>(allRequests);
            value->simulationSet = true;
            hashSet.Add(value);
            part->simulationCrossfeedPartSet = value;
            setsHolder[allRequests.get()] = value;
        }
    }
    setsHolder.clear();
    return hashSet;
}

// PartSet.cs:815 [Obsolete("Use FlowGraph (i.e. BuildPartSets) instead, unless you just need one set, not the whole vessel")]
void PartSet::AddPartToSet(PartHashSet& set, Part* p, Vessel* v) {
    if (set.Contains(p)) return;
    set.Add(p);
    int count = static_cast<int>(p->fuelLookupTargets.size());
    if (count > 0) {
        int index = count;
        while (index-- > 0) {
            Part* part = p->fuelLookupTargets[static_cast<size_t>(index)];
            if (part != nullptr && (part->vessel == v || ResourceFlowEnvironment::LoadedSceneIsEditor()) &&
                ((part == p->parent && p->isAttached()) || (part != p->parent && part->isAttached()))) {
                AddPartToSet(set, part, v);
            }
        }
    }
    if (!p->fields.fuelCrossFeed) return;
    int count2 = static_cast<int>(p->children.size());
    while (count2-- > 0) {
        Part* part = p->children[static_cast<size_t>(count2)];
        if (part->srfAttachNode->attachedPart == p && part->fields.fuelCrossFeed) AddPartToSet(set, part, v);
    }
    const std::string key = p->fields.NoCrossFeedNodeKey.value_or(std::string());
    int count3 = static_cast<int>(p->attachNodes.size());
    while (count3-- > 0) {
        AttachNode& attachNode = p->attachNodes[static_cast<size_t>(count3)];
        if (key.empty() || attachNode.id.find(key) == std::string::npos) {
            Part* part = attachNode.attachedPart;
            if (attachNode.ResourceXFeed && !(part == nullptr)) AddPartToSet(set, part, v);
        }
    }
    if (p->parent != nullptr) {
        AttachNode* attachNode2 = p->FindAttachNodeByPart(p->parent);
        if (attachNode2 != nullptr && (key.empty() || attachNode2->id.find(key) == std::string::npos)) AddPartToSet(set, p->parent, v);
    }
}

// ---- queries --------------------------------------------------------------------------------------------------------------

// PartSet.cs:871
bool PartSet::ContainsPart(Part* p) {
    if (vesselWide) {
        if (vessel != nullptr) return p->vessel == vessel;
        if (ship != nullptr) {
            IResourceVessel* h = ResourceFlowEnvironment::Host(ship);
            return h != nullptr && ContainsPersistentId(h->ResourceParts(), p);
        }
        return false;
    }
    return ContainsPersistentId(*targetParts, p);
}

// PartSet.cs:888
PartHashSetPtr PartSet::GetParts() {
    if (!targetParts) return std::make_shared<PartHashSet>();
    return targetParts;
}

// PartSet.cs:897
bool PartSet::SetsEqual(const PartHashSet& set) {
    if (vesselWide) return false;
    return KSPUtil::DeepCompare(*targetParts, set);
}

// ---- event listeners --------------------------------------------------------------------------------------------------------

// PartSet.cs:906
void PartSet::OnFlowStateChange(GE::HostedFromToAction<PartResource*, bool> data) {
    PartResource* host = data.host;
    Part* part = host->part;
    if (part == nullptr) return;
    if ((!vesselWide || !(part->vessel == vessel)) && (vesselWide || !(targetParts && targetParts->Contains(part)))) return;
    if (data.from) {
        RemovePartResource(host, pullList);
        RemovePartResource(host, pushList);
        return;
    }
    if (host->Flowing(true)) AddPartResource(host, pullList);
    if (host->Flowing(false)) AddPartResource(host, pushList);
}

// PartSet.cs:930
void PartSet::OnFlowModeChange(GE::HostedFromToAction<PartResource*, PartResource_FlowMode> data) {
    PartResource* host = data.host;
    Part* part = host->part;
    if (part == nullptr) return;
    if ((!vesselWide || !(part->vessel == vessel)) && (vesselWide || !(targetParts && targetParts->Contains(part)))) return;
    using FM = PartResourceFlowMode;
    const FM from = static_cast<FM>(static_cast<int>(data.from));
    const FM to = static_cast<FM>(static_cast<int>(data.to));
    switch (from) {
        case FM::None:
            switch (to) {
                default:
                    if (host->Flowing(true)) AddPartResource(host, pullList);
                    if (host->Flowing(false)) AddPartResource(host, pushList);
                    break;
                case FM::In:
                    if (host->Flowing(false)) AddPartResource(host, pushList);
                    break;
                case FM::Out:
                    if (host->Flowing(true)) AddPartResource(host, pullList);
                    break;
            }
            return;
        case FM::In:
            switch (to) {
                case FM::Both:
                    if (host->Flowing(true)) AddPartResource(host, pullList);
                    break;
                default:
                    if (host->Flowing(true)) AddPartResource(host, pullList);
                    RemovePartResource(host, pushList);
                    break;
                case FM::None:
                    RemovePartResource(host, pushList);
                    break;
            }
            return;
        case FM::Both:
            switch (to) {
                default:
                    RemovePartResource(host, pushList);
                    RemovePartResource(host, pullList);
                    break;
                case FM::In:
                    RemovePartResource(host, pullList);
                    break;
                case FM::Out:
                    RemovePartResource(host, pushList);
                    break;
            }
            return;
        default:
            break;
    }
    // from == Out
    switch (to) {
        case FM::Both:
            if (host->Flowing(false)) AddPartResource(host, pushList);
            return;
        case FM::None:
            RemovePartResource(host, pullList);
            return;
        default:
            break;
    }
    if (host->Flowing(false)) AddPartResource(host, pushList);
    RemovePartResource(host, pullList);
}

}  // namespace ksp
