// WP-15: the resource-flow members of Part.cs: RequestResource (all overloads), TransferResource, GetConnectedResourceTotals,
// CanCrossfeed and the [Obsolete] fuel lookup (RequestFuel / FindFuel / DrainFuel).
//
// Part.requestResource picks the PartSet that serves the request from the resource's ResourceFlowMode:
//   NO_FLOW / NULL (default)                     this part's own resource list
//   ALL_VESSEL, ALL_VESSEL_BALANCE               the whole vessel (or editor ship when simulating), no priorities
//   STAGE_PRIORITY_FLOW(_BALANCE)                the whole vessel, by resource priority (stage order)
//   STACK_PRIORITY_SEARCH, STAGE_STACK_FLOW(_BALANCE)   the part's crossfeed PartSet (flow graph), by priority
// The vessel and ship are reached through IResourceVessel (ksp/resources/resource_flow_host.hpp).
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/part_set.hpp"
#include "ksp/resources/resource_flow_host.hpp"

namespace ksp {

namespace {
using GE = events::GameEvents;

NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
}  // namespace

// ---- RequestResource overloads (Part.cs:6404-6473) ----------------------------------------------------------------------------

// Part.cs:6404 [Obsolete]
float Part::RequestResource(int resourceID, float demand) {
    ResourceFlowMode defaultFlowMode = AsFlowMode(PartResourceLibrary::GetDefaultFlowMode(resourceID));
    if (defaultFlowMode != ResourceFlowMode::NULL_) return static_cast<float>(requestResource(this, resourceID, defaultFlowMode, static_cast<double>(demand)));
    return 0.0f;
}

// Part.cs:6415 [Obsolete]
float Part::RequestResource(std::string_view resourceName, float demand) {
    ResourceFlowMode defaultFlowMode = AsFlowMode(PartResourceLibrary::GetDefaultFlowMode(resourceName));
    if (defaultFlowMode != ResourceFlowMode::NULL_)
        return static_cast<float>(requestResource(this, MonoStringHash(resourceName), defaultFlowMode, static_cast<double>(demand)));
    return 0.0f;
}

// Part.cs:6425
double Part::RequestResource(int resourceID, double demand) { return RequestResource(resourceID, demand, false); }

// Part.cs:6430
double Part::RequestResource(int resourceID, double demand, bool simulate) {
    ResourceFlowMode defaultFlowMode = AsFlowMode(PartResourceLibrary::GetDefaultFlowMode(resourceID));
    if (defaultFlowMode != ResourceFlowMode::NULL_) return requestResource(this, resourceID, defaultFlowMode, demand, simulate);
    return 0.0;
}

// Part.cs:6440
double Part::RequestResource(std::string_view resourceName, double demand) { return RequestResource(resourceName, demand, false); }

// Part.cs:6445
double Part::RequestResource(std::string_view resourceName, double demand, bool simulate) {
    ResourceFlowMode defaultFlowMode = AsFlowMode(PartResourceLibrary::GetDefaultFlowMode(resourceName));
    if (defaultFlowMode != ResourceFlowMode::NULL_) return requestResource(this, MonoStringHash(resourceName), defaultFlowMode, demand, simulate);
    return 0.0;
}

// Part.cs:6455
double Part::RequestResource(int resourceID, double demand, ResourceFlowMode flowMode) { return RequestResource(resourceID, demand, flowMode, false); }

// Part.cs:6460
double Part::RequestResource(int resourceID, double demand, ResourceFlowMode flowMode, bool simulate) {
    return requestResource(this, resourceID, flowMode, demand, simulate);
}

// Part.cs:6465
double Part::RequestResource(std::string_view resourceName, double demand, ResourceFlowMode flowMode) {
    return RequestResource(resourceName, demand, flowMode, false);
}

// Part.cs:6470
double Part::RequestResource(std::string_view resourceName, double demand, ResourceFlowMode flowMode, bool simulate) {
    return requestResource(this, MonoStringHash(resourceName), flowMode, demand, simulate);
}

// Part.cs:6480 (the 4-argument overload at :6475 is simulate = false)
double Part::requestResource(Part* /*origin*/, int resourceID, ResourceFlowMode flowMode, double demand, bool simulate) {
    if (demand == 0.0) return 0.0;
    if (ResourceFlowEnvironment::InfiniteElectricity && resourceID == PartResourceLibrary::ElectricityHashcode() && demand > 0.0) return demand;
    switch (flowMode) {
        case ResourceFlowMode::ALL_VESSEL:
        case ResourceFlowMode::ALL_VESSEL_BALANCE:
        case ResourceFlowMode::STAGE_PRIORITY_FLOW:
        case ResourceFlowMode::STAGE_PRIORITY_FLOW_BALANCE: {
            const bool usePriority = flowMode == ResourceFlowMode::STAGE_PRIORITY_FLOW || flowMode == ResourceFlowMode::STAGE_PRIORITY_FLOW_BALANCE;
            if (vessel == nullptr && ship == nullptr) return 0.0;
            if (simulate && ship != nullptr && ResourceFlowEnvironment::LoadedSceneIsEditor()) {
                IResourceVessel* h = ResourceFlowEnvironment::Host(ship);
                if (h == nullptr) throw Nre();
                return h->RequestResource(this, resourceID, demand, usePriority, simulate);
            }
            if (vessel != nullptr) {
                IResourceVessel* h = ResourceFlowEnvironment::Host(vessel);
                if (h == nullptr) throw Nre();
                return h->RequestResource(this, resourceID, demand, usePriority, simulate);
            }
            return 0.0;
        }
        case ResourceFlowMode::STACK_PRIORITY_SEARCH:
        case ResourceFlowMode::STAGE_STACK_FLOW:
        case ResourceFlowMode::STAGE_STACK_FLOW_BALANCE:
            if (!simulate) {
                if (!crossfeedPartSet) throw Nre();
                return crossfeedPartSet->RequestResource(this, resourceID, demand, true, simulate);
            }
            if (!simulationCrossfeedPartSet) throw Nre();
            return simulationCrossfeedPartSet->RequestResource(this, resourceID, demand, true, simulate);
        default: {
            PartResource* partResource = simulate ? simulationResources_.GetFlowing(resourceID, demand > 0.0) : Resources.GetFlowing(resourceID, demand > 0.0);
            if (partResource != nullptr) return TransferResource(*partResource, 0.0 - demand, this, simulate);
            return 0.0;
        }
    }
}

// ---- TransferResource (Part.cs:6589-6695) ------------------------------------------------------------------------------------

// Part.cs:6589
double Part::TransferResource(int resourceID, double amount) {
    PartResource* partResource = Resources.Get(resourceID);
    if (partResource == nullptr) {
        log::Error("Resource System Error: Resource id " + std::to_string(resourceID) + " does not exist in part");
        return 0.0;
    }
    return TransferResource(*partResource, amount, this);
}

// Part.cs:6600
double Part::TransferResource(PartResource& resource, double amount, Part* other) { return TransferResource(resource, amount, other, false); }

// Part.cs:6605
double Part::TransferResource(PartResource& resource, double amount, Part* other, bool simulate) {
    PartResourceFlowMode flowMode = resource.flowMode;
    if (flowMode != PartResourceFlowMode::None && amount != 0.0) {
        bool flag;
        if ((flag = amount > 0.0)) {
            if (flowMode == PartResourceFlowMode::Out) return 0.0;
        } else if (flowMode == PartResourceFlowMode::In) {
            return 0.0;
        }
        const bool flight = ResourceFlowEnvironment::LoadedSceneIsFlight();
        double amount2 = resource.amount;
        double num = static_cast<double>(resource.info->density());
        const double specificHeat = static_cast<double>(resource.info->specificHeatCapacity());
        if (flag) {
            double num2 = resource.maxAmount - resource.amount;
            if (amount >= num2) {
                resource.amount = resource.maxAmount;
                if (num > 0.0 && flight && other != this && !simulate) {
                    double num3 = num2 * num * specificHeat;
                    fields.temperature = (fields.temperature * fields.thermalMass + num3 * other->fields.temperature) / (fields.thermalMass + num3);
                }
                if (amount2 > 0.0) {
                    if (num2 > 0.0 && !simulate) GE::onPartResourceNonemptyFull.Fire(&resource);
                } else if (!simulate) {
                    GE::onPartResourceEmptyFull.Fire(&resource);
                }
                return 0.0 - num2;
            }
            resource.amount += amount;
            if (num > 0.0 && flight && other != this && !simulate) {
                double num4 = amount * num * specificHeat;
                fields.temperature = (fields.temperature * fields.thermalMass + num4 * other->fields.temperature) / (fields.thermalMass + num4);
            }
            if (amount2 == 0.0 && !simulate) GE::onPartResourceEmptyNonempty.Fire(&resource);
            return 0.0 - amount;
        }
        amount = 0.0 - amount;
        if (amount >= resource.amount) {
            resource.amount = 0.0;
            if (num > 0.0 && flight && other != this && !simulate) {
                double num5 = amount2 * num * specificHeat;
                other->fields.temperature = (other->fields.temperature * other->fields.thermalMass + num5 * fields.temperature) / (other->fields.thermalMass + num5);
            }
            if (amount2 < resource.maxAmount) {
                if (amount2 > 0.0 && !simulate) GE::onPartResourceNonemptyEmpty.Fire(&resource);
            } else if (!simulate) {
                GE::onPartResourceFullEmpty.Fire(&resource);
            }
            return amount2;
        }
        resource.amount -= amount;
        if (num > 0.0 && flight && other != this && !simulate) {
            double num6 = amount * num * specificHeat;
            other->fields.temperature = (other->fields.temperature * other->fields.thermalMass + num6 * fields.temperature) / (other->fields.thermalMass + num6);
        }
        if (amount2 == resource.maxAmount && !simulate) GE::onPartResourceFullNonempty.Fire(&resource);
        return amount;
    }
    return 0.0;
}

// ---- GetConnectedResourceTotals (Part.cs:6697-6821) ---------------------------------------------------------------------------

namespace {
ResourceFlowMode DefaultFlow(int resourceID) {
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    const PartResourceDefinition* d = lib ? lib->GetDefinition(resourceID) : nullptr;
    if (d == nullptr) throw NullReferenceException("Object reference not set to an instance of an object");
    return AsFlowMode(d->resourceFlowMode());
}
}  // namespace

// Part.cs:6697
void Part::GetConnectedResourceTotals(int resourceID, double& amount, double& maxAmount, bool pulling) {
    GetConnectedResourceTotals(resourceID, DefaultFlow(resourceID), amount, maxAmount, pulling);
}

// Part.cs:6702
void Part::GetConnectedResourceTotals(int resourceID, ResourceFlowMode flowMode, double& amount, double& maxAmount, bool pulling) {
    GetConnectedResourceTotals(resourceID, flowMode, false, amount, maxAmount, pulling);
}

// Part.cs:6707
void Part::GetConnectedResourceTotals(int resourceID, ResourceFlowMode flowMode, bool simulate, double& amount, double& maxAmount, bool pulling) {
    switch (flowMode) {
        case ResourceFlowMode::ALL_VESSEL:
        case ResourceFlowMode::STAGE_PRIORITY_FLOW:
        case ResourceFlowMode::ALL_VESSEL_BALANCE:
        case ResourceFlowMode::STAGE_PRIORITY_FLOW_BALANCE:
            if (ship != nullptr && simulate && ResourceFlowEnvironment::LoadedSceneIsEditor()) {
                IResourceVessel* h = ResourceFlowEnvironment::Host(ship);
                if (h == nullptr) throw Nre();
                h->GetConnectedResourceTotals(resourceID, true, amount, maxAmount, pulling);
                break;
            }
            if (vessel != nullptr) {
                IResourceVessel* h = ResourceFlowEnvironment::Host(vessel);
                if (h == nullptr) throw Nre();
                h->GetConnectedResourceTotals(resourceID, simulate, amount, maxAmount, pulling);
                break;
            }
            amount = 0.0;
            maxAmount = 0.0;
            break;
        case ResourceFlowMode::STACK_PRIORITY_SEARCH:
        case ResourceFlowMode::STAGE_STACK_FLOW:
        case ResourceFlowMode::STAGE_STACK_FLOW_BALANCE:
            if (simulate) {
                if (!simulationCrossfeedPartSet) throw Nre();
                simulationCrossfeedPartSet->GetConnectedResourceTotals(resourceID, amount, maxAmount, pulling, simulate);
            } else {
                if (!crossfeedPartSet) throw Nre();
                crossfeedPartSet->GetConnectedResourceTotals(resourceID, amount, maxAmount, pulling, simulate);
            }
            break;
        default:
            if (simulate) simulationResources_.GetFlowingTotals(resourceID, amount, maxAmount, pulling);
            else Resources.GetFlowingTotals(resourceID, amount, maxAmount, pulling);
            break;
    }
}

// Part.cs:6753
void Part::GetConnectedResourceTotals(int resourceID, double& amount, double& maxAmount, double threshold, bool pulling) {
    GetConnectedResourceTotals(resourceID, DefaultFlow(resourceID), amount, maxAmount, threshold, pulling);
}

// Part.cs:6758
void Part::GetConnectedResourceTotals(int resourceID, ResourceFlowMode flowMode, double& amount, double& maxAmount, double threshold, bool pulling) {
    GetConnectedResourceTotals(resourceID, flowMode, false, amount, maxAmount, threshold, pulling);
}

// Part.cs:6763
void Part::GetConnectedResourceTotals(int resourceID, ResourceFlowMode flowMode, bool simulate, double& amount, double& maxAmount, double threshold,
                                      bool pulling) {
    switch (flowMode) {
        case ResourceFlowMode::ALL_VESSEL:
        case ResourceFlowMode::STAGE_PRIORITY_FLOW:
        case ResourceFlowMode::ALL_VESSEL_BALANCE:
        case ResourceFlowMode::STAGE_PRIORITY_FLOW_BALANCE: {
            IResourceVessel* h = ResourceFlowEnvironment::Host(vessel);
            if (h == nullptr) throw Nre();
            h->GetConnectedResourceTotals(resourceID, simulate, amount, maxAmount, threshold, pulling);
            return;
        }
        case ResourceFlowMode::STACK_PRIORITY_SEARCH:
        case ResourceFlowMode::STAGE_STACK_FLOW:
        case ResourceFlowMode::STAGE_STACK_FLOW_BALANCE:
            if (simulate) {
                if (!simulationCrossfeedPartSet) throw Nre();
                simulationCrossfeedPartSet->GetConnectedResourceTotals(resourceID, amount, maxAmount, threshold, pulling);
            } else {
                if (!crossfeedPartSet) throw Nre();
                crossfeedPartSet->GetConnectedResourceTotals(resourceID, amount, maxAmount, threshold, pulling);
            }
            return;
        default:
            break;
    }
    if (simulate) simulationResources_.GetFlowingTotals(resourceID, amount, maxAmount, pulling);
    else Resources.GetFlowingTotals(resourceID, amount, maxAmount, pulling);
    if (pulling) {
        double num = maxAmount * threshold;
        amount -= num;
        if (amount < 1E-09) amount = 0.0;
        maxAmount *= 1.0 - threshold;
        if (maxAmount < 1E-09) maxAmount = 0.0;
    } else {
        maxAmount *= threshold;
        amount = maxAmount - amount;
        if (amount < 1E-09) amount = 0.0;
        if (maxAmount < 1E-09) maxAmount = 0.0;
    }
}

// Part.cs:6823
bool Part::CanCrossfeed(Part* target, std::string_view resName, ResourceFlowMode flow) {
    return CanCrossfeed(target, MonoStringHash(resName), flow);
}

// Part.cs:6828
bool Part::CanCrossfeed(Part* target, int resourceID, ResourceFlowMode flow) {
    if (flow == ResourceFlowMode::NULL_) flow = DefaultFlow(resourceID);
    switch (flow) {
        case ResourceFlowMode::ALL_VESSEL:
        case ResourceFlowMode::STAGE_PRIORITY_FLOW:
        case ResourceFlowMode::ALL_VESSEL_BALANCE:
        case ResourceFlowMode::STAGE_PRIORITY_FLOW_BALANCE:
            return target->vessel == vessel;
        case ResourceFlowMode::STACK_PRIORITY_SEARCH:
        case ResourceFlowMode::STAGE_STACK_FLOW:
        case ResourceFlowMode::STAGE_STACK_FLOW_BALANCE:
            if (!crossfeedPartSet) throw Nre();
            return crossfeedPartSet->ContainsPart(target);
        default:
            return target == this;
    }
}

// ---- [Obsolete] fuel lookup (Part.cs:4681-4829). No caller exists in the game. ---------------------------------------------------

// Part.cs:4681
bool Part::RequestFuel(Part* source, float amount, uint32_t reqId) {
    if (reqId == fields.lastFuelRequestId) return false;
    fields.lastFuelRequestId = reqId;
    if (State() == PartStates::DEAD) return false;
    if (!fuelLookupTargets.empty()) {
        bool result = false;
        int count = static_cast<int>(fuelLookupTargets.size());
        float amount2 = f32store(fx(amount) / fx(static_cast<float>(count)));
        for (int i = 0; i < count; i++) {
            Part* t = fuelLookupTargets[static_cast<size_t>(i)];
            if ((t == parent && isAttached()) || (t != parent && t->isAttached())) {
                if (t->RequestFuel(source, amount2, reqId)) result = true;
                else i--;  // sic: a target that refuses is asked again, and refuses again (same reqId): the loop never ends
            }
        }
        return result;
    }
    if (!fields.fuelCrossFeed && !(source == this)) return false;
    const std::string key = fields.NoCrossFeedNodeKey.value_or(std::string());
    resourceTargets.clear();
    int count2 = static_cast<int>(attachNodes.size());
    for (int j = 0; j < count2; j++) {
        AttachNode& attachNode = attachNodes[static_cast<size_t>(j)];
        if ((key.empty() || attachNode.id.find(key) == std::string::npos) && !(attachNode.attachedPart == nullptr) &&
            attachNode.attachedPart->fields.lastFuelRequestId != reqId)
            resourceTargets.push_back(attachNode.attachedPart);
    }
    int count3 = static_cast<int>(resourceTargets.size());
    if (count3 > 0) {
        bool flag = false;
        float amount3 = f32store(fx(amount) / fx(static_cast<float>(count3)));
        for (int k = 0; k < count3; k++)
            if (resourceTargets[static_cast<size_t>(k)]->RequestFuel(source, amount3, reqId)) flag = true;
        if (flag) return true;
    }
    if (parent != nullptr) {
        AttachNode* attachNode2 = FindAttachNodeByPart(parent);
        if (attachNode2 != nullptr && (key.empty() || attachNode2->id.find(key) == std::string::npos) && parent->RequestFuel(source, amount, reqId))
            return true;
    }
    return false;
}

// Part.cs:4756
bool Part::FindFuel(Part* source, std::vector<Part*>& fuelSources, uint32_t reqId) {
    if (reqId == fields.lastFuelRequestId) return false;
    fields.lastFuelRequestId = reqId;
    if (State() == PartStates::DEAD) return false;
    if (!fuelLookupTargets.empty()) {
        bool flag = false;
        for (size_t i = 0, count = fuelLookupTargets.size(); i < count; i++) {
            Part* t = fuelLookupTargets[i];
            if (((t == parent && isAttached()) || (t != parent && t->isAttached())) && t->FindFuel(source, fuelSources, reqId)) flag = true;
        }
        if (flag) return true;
    }
    if (!fields.fuelCrossFeed && !(source == this)) return false;
    const std::string key = fields.NoCrossFeedNodeKey.value_or(std::string());
    resourceTargets.clear();
    for (size_t j = 0, count2 = attachNodes.size(); j < count2; j++) {
        AttachNode& attachNode = attachNodes[j];
        if ((key.empty() || attachNode.id.find(key) == std::string::npos) && !(attachNode.attachedPart == nullptr) &&
            attachNode.attachedPart->fields.lastFuelRequestId != reqId)
            resourceTargets.push_back(attachNode.attachedPart);
    }
    int count3 = static_cast<int>(resourceTargets.size());
    if (count3 > 0) {
        bool flag2 = false;
        for (int k = 0; k < count3; k++)
            if (resourceTargets[static_cast<size_t>(k)]->FindFuel(source, fuelSources, reqId)) flag2 = true;
        if (flag2) return true;
    }
    if (parent != nullptr) {
        AttachNode* attachNode2 = FindAttachNodeByPart(parent);
        if (attachNode2 != nullptr && (key.empty() || attachNode2->id.find(key) == std::string::npos) && parent->FindFuel(source, fuelSources, reqId))
            return true;
    }
    return false;
}

}  // namespace ksp
