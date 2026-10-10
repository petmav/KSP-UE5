// WP-39: CommNet/ModuleRelayEnabler.cs, CommNet/ModuleProbeControlPoint.cs (see ksp/modules/module_commnet_parts.hpp) and the registration of
// the CommNet module classes.
#include "ksp/modules/module_commnet_parts.hpp"

#include <algorithm>

#include "ksp/commnet/comm_net_vessel.hpp"
#include "ksp/commnet/commnet_scene.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/module_data_transmitter.hpp"
#include "ksp/modules/module_kerbnet_access.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/modules/vessel_module.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/proto_vessel.hpp"

namespace ksp::CommNet {

namespace {
const PartResourceDefinition* LookupDefinition(const std::string& name) {
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    return lib ? lib->GetDefinition(std::string_view(name)) : nullptr;
}

// the loop shared by CanRelayUnloaded / CanCommUnloaded: the last `valueName` entry of the snapshot's values, "False" means off
// (`while (!(values[count].name == name))` from the end; no entry = true)
bool ProtoFlag(ProtoPartModuleSnapshot* mSnap, const char* valueName) {
    int count = mSnap->moduleValues->values().Count();
    do {
        if (count-- <= 0) return true;
    } while (!(mSnap->moduleValues->values()[count].name == valueName));
    return mSnap->moduleValues->values()[count].value != "False";
}
}  // namespace

// ---- ModuleRelayEnabler.cs ---------------------------------------------------------------------------------------------------

// ModuleRelayEnabler.cs:23
void ModuleRelayEnabler::OnAwake() {
    consumedResources_.clear();
    for (size_t i = 0, count = resHandler.inputResources.size(); i < count; i++)
        consumedResources_.push_back(LookupDefinition(resHandler.inputResources[i]->name));
}

// ModuleRelayEnabler.cs:40
void ModuleRelayEnabler::OnSave(ConfigNode& node) {
    PartModule::OnSave(node);
    node.AddValue("canRelay", isRelaying);
}

// ModuleRelayEnabler.cs:46
void ModuleRelayEnabler::OnStart(StartState) { Fields()["status"]->guiActive = Get<bool>("showStatus"); }

// ModuleRelayEnabler.cs:51
void ModuleRelayEnabler::FixedUpdate() {
    if (!ResourceFlowEnvironment::LoadedSceneIsFlight()) return;
    std::string status = Get<std::string>("status");
    const bool ok = resHandler.UpdateModuleResourceInputs(status, 1.0, 0.9, true);
    Set<std::string>("status", status);
    if (ok) {
        isRelaying = true;
        if (Get<std::string>("status") != "Nominal") Set<std::string>("status", "Nominal");
    } else {
        isRelaying = false;
    }
}

// ModuleRelayEnabler.cs:81
bool ModuleRelayEnabler::CanRelayUnloaded(ProtoPartModuleSnapshot* mSnap) {
    if (mSnap == nullptr) return true;
    return ProtoFlag(mSnap, "canRelay");
}

// ---- ModuleProbeControlPoint.cs ----------------------------------------------------------------------------------------------

// ModuleProbeControlPoint.cs:23
bool ModuleProbeControlPoint::CanControl() { return !IsProbeControlPointBroken(adjusterCache_); }

// ModuleProbeControlPoint.cs:28
bool ModuleProbeControlPoint::CanControlUnloaded(ProtoPartModuleSnapshot* mSnap) {
    if (mSnap == nullptr) return true;
    std::vector<IProbeControlPointAdjuster*> list;
    if (CommNetModuleEnvironment::protoProbeAdjusters) list = CommNetModuleEnvironment::protoProbeAdjusters(*mSnap);
    if (IsProbeControlPointBroken(list)) return false;
    return true;
}

// ModuleProbeControlPoint.cs:50
void ModuleProbeControlPoint::OnModuleAdjusterRemoved(IProbeControlPointAdjuster* adjuster) {
    auto it = std::find(adjusterCache_.begin(), adjusterCache_.end(), adjuster);
    if (it != adjusterCache_.end()) adjusterCache_.erase(it);
}

// ModuleProbeControlPoint.cs:57
bool ModuleProbeControlPoint::IsProbeControlPointBroken(const std::vector<IProbeControlPointAdjuster*>& adjusterList) {
    size_t num = 0;
    while (true) {
        if (num < adjusterList.size()) {
            if (adjusterCache_.at(num)->IsProbeControlPointBroken()) break;  // sic: indexes the module's own cache
            num++;
            continue;
        }
        return false;
    }
    return true;
}

// ModuleProbeControlPoint.cs:83
std::string ModuleProbeControlPoint::GetInfo() {
    return Localizer::Format("#autoLOC_118898", {std::to_string(minimumCrew())}) +
           (multiHop() ? Localizer::Format("#autoLOC_118899") : Localizer::Format("#autoLOC_6004043"));
}

// ModuleProbeControlPoint.cs:88
std::string ModuleProbeControlPoint::GetModuleDisplayName() { return Localizer::Format("#autoLOC_7000026"); }

// shared by ModuleDataTransmitter::CanCommUnloaded
bool ProtoValueFlag(ProtoPartModuleSnapshot* mSnap, const char* valueName) { return ProtoFlag(mSnap, valueName); }

}  // namespace ksp::CommNet

namespace ksp::detail {

void RegisterCommNetModules() {
    ModuleFactory::Register("ModuleRelayEnabler", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<PartModule>(std::make_shared<CommNet::ModuleRelayEnabler>(c));
    });
    ModuleFactory::Register("ModuleProbeControlPoint", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<PartModule>(std::make_shared<CommNet::ModuleProbeControlPoint>(c));
    });
    ModuleFactory::Register("ModuleDataTransmitter", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<PartModule>(std::make_shared<ModuleDataTransmitter>(c));
    });
    ModuleFactory::Register("ModuleKerbNetAccess", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<PartModule>(std::make_shared<ModuleKerbNetAccess>(c));
    });
    VesselModuleFactory::Register("CommNetVessel", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<VesselModule>(std::make_shared<CommNet::CommNetVessel>(c));
    });
    ScenarioModuleFactory::Register("CommNetScenario", [](const std::string&) {
        return std::unique_ptr<ScenarioModule>(std::make_unique<CommNet::CommNetScenario>());
    });
}

}  // namespace ksp::detail
