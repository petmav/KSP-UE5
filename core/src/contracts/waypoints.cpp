// WP-37: ProtoWaypointInfo.cs and ScenarioCustomWaypoints.cs (data side).
#include "ksp/contracts/waypoints.hpp"

#include <algorithm>

#include "ksp/career/career.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/util/misc_util.hpp"

namespace ksp {

using GE = events::GameEvents;

// ---- ProtoWaypointInfo (the members that need the Waypoint class) -----------------------------------------------------------

// ProtoWaypointInfo.cs:22
ProtoWaypointInfo MakeProtoWaypointInfo(const FinePrint::Waypoint* wp) {
    ProtoWaypointInfo info;
    if (wp != nullptr) {
        info.waypointType = ProtoWaypointInfo::Type::Custom;
        info.navigationId = wp->navigationId;
    } else {
        info.waypointType = ProtoWaypointInfo::Type::Null;
        info.navigationId = Guid::Empty();
    }
    return info;
}

// ProtoWaypointInfo.cs:66
FinePrint::Waypoint* FindWaypoint(const ProtoWaypointInfo& info) {
    switch (info.waypointType) {
        default: return nullptr;
        case ProtoWaypointInfo::Type::Custom:
        case ProtoWaypointInfo::Type::Survey:
            return FinePrint::WaypointManagerSeam::findWaypoint ? FinePrint::WaypointManagerSeam::findWaypoint(info.navigationId) : nullptr;
        case ProtoWaypointInfo::Type::Null: return nullptr;
    }
}

// ---- ScenarioCustomWaypoints ---------------------------------------------------------------------------------------------------

ScenarioCustomWaypoints::ScenarioCustomWaypoints() : ScenarioModule("ScenarioCustomWaypoints") {}
ScenarioCustomWaypoints::~ScenarioCustomWaypoints() { OnDestroy(); }

// ScenarioCustomWaypoints.cs:19 (the second instance destroys itself: Object.Destroy(this); here it just never becomes the Instance)
void ScenarioCustomWaypoints::OnAwake() {
    if (Instance != nullptr) return;
    Instance = this;
    waypoints_.clear();
    awake_ = true;
}

// ScenarioCustomWaypoints.cs:30
void ScenarioCustomWaypoints::OnDestroy() {
    if (awake_) {
        for (int num = static_cast<int>(waypoints_.size()) - 1; num >= 0; --num)
            if (FinePrint::WaypointManagerSeam::removeWaypoint) FinePrint::WaypointManagerSeam::removeWaypoint(waypoints_[static_cast<size_t>(num)]);
        waypoints_.clear();
    }
    if (Instance != nullptr && Instance == this) Instance = nullptr;
}

// ScenarioCustomWaypoints.cs:50
void ScenarioCustomWaypoints::AddWaypoint(FinePrint::Waypoint* waypoint) { AddWaypoint(waypoint, false); }

// ScenarioCustomWaypoints.cs:50
void ScenarioCustomWaypoints::AddWaypoint(FinePrint::Waypoint* waypoint, bool isMission) {
    if (!(Instance == nullptr)) {
        ProcessWaypoint(waypoint);
        if (isMission) {
            waypoint->isMission = true;
            waypoint->isCustom = false;
        }
        Instance->waypoints_.push_back(waypoint);
        if (FinePrint::WaypointManagerSeam::addWaypoint) FinePrint::WaypointManagerSeam::addWaypoint(waypoint);
    }
}

// ScenarioCustomWaypoints.cs:65
void ScenarioCustomWaypoints::RemoveWaypoint(FinePrint::Waypoint* waypoint) {
    if (!(Instance == nullptr)) {
        if (FinePrint::WaypointManagerSeam::removeWaypoint) FinePrint::WaypointManagerSeam::removeWaypoint(waypoint);
        auto it = std::find(Instance->waypoints_.begin(), Instance->waypoints_.end(), waypoint);
        if (it != Instance->waypoints_.end()) Instance->waypoints_.erase(it);
    }
}

// ScenarioCustomWaypoints.cs:74
void ScenarioCustomWaypoints::ProcessWaypoint(FinePrint::Waypoint* waypoint) {
    waypoint->seed = StringExtensions::GetHashCode_Net35(waypoint->name);
    waypoint->id = "custom";
    waypoint->isCustom = true;
    waypoint->isNavigatable = true;
    waypoint->isOnSurface = true;
    waypoint->landLocked = false;
    waypoint->index = 0;
    waypoint->SetFadeRange();
}

// ScenarioCustomWaypoints.cs:86
void ScenarioCustomWaypoints::OnLoad(ConfigNode& node) {
    std::vector<ConfigNode*> nodes = node.GetNodes("WAYPOINT");
    for (size_t i = 0; i < nodes.size(); ++i) {
        ConfigNode* configNode = nodes[i];
        auto wp = std::make_unique<FinePrint::Waypoint>();
        FinePrint::Waypoint* waypoint = wp.get();
        Contracts::Lite::LoadNode(configNode, "ScenarioCustomWaypoints", "name", waypoint->name, std::string("Custom Waypoint"));
        const CelestialBody* home = CareerEnvironment::Current().HomeBody();
        Contracts::Lite::LoadNode(configNode, "ScenarioCustomWaypoints", "celestialName", waypoint->celestialName,
                                  home != nullptr ? home->name() : std::string());
        Contracts::Lite::LoadNode(configNode, "ScenarioCustomWaypoints", "latitude", waypoint->latitude, 0.0);
        Contracts::Lite::LoadNode(configNode, "ScenarioCustomWaypoints", "longitude", waypoint->longitude, 0.0);
        Contracts::Lite::LoadNode(configNode, "ScenarioCustomWaypoints", "navigationId", waypoint->navigationId, Contracts::ContractsEnvironment::Current().NewGuid());
        if (waypoint->navigationId == Guid::Empty()) {
            log::Warning("Stored navigationId was empty for " + waypoint->name + " - Generating new id");
            waypoint->navigationId = Contracts::ContractsEnvironment::Current().NewGuid();
        }
        ProcessWaypoint(waypoint);
        GE::onCustomWaypointLoad.Fire(GE::FromToAction<FinePrint::Waypoint*, ConfigNode*>(waypoint, configNode));
        waypoints_.push_back(waypoint);
        owned_.push_back(std::move(wp));
        if (FinePrint::WaypointManagerSeam::addWaypoint) FinePrint::WaypointManagerSeam::addWaypoint(waypoint);
    }
}

// ScenarioCustomWaypoints.cs:112
void ScenarioCustomWaypoints::OnSave(ConfigNode& node) {
    for (size_t i = 0; i < waypoints_.size(); ++i) {
        FinePrint::Waypoint* waypoint = waypoints_[i];
        if (!waypoint->isMission) {
            ConfigNode* configNode = node.AddNode("WAYPOINT");
            configNode->AddValue("name", waypoint->name);
            configNode->AddValue("celestialName", waypoint->celestialName);
            configNode->AddValue("latitude", waypoint->latitude);
            configNode->AddValue("longitude", waypoint->longitude);
            configNode->AddValue("navigationId", waypoint->navigationId.ToString());
            GE::onCustomWaypointSave.Fire(GE::FromToAction<FinePrint::Waypoint*, ConfigNode*>(waypoint, configNode));
        }
    }
}

void ScenarioCustomWaypoints::Register() {
    ScenarioModuleFactory::Register("ScenarioCustomWaypoints", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new ScenarioCustomWaypoints()); });
}

}  // namespace ksp
