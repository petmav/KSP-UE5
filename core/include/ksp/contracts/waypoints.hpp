// WP-37: ProtoWaypointInfo.cs and ScenarioCustomWaypoints.cs (data side). The Waypoint class and the WaypointManager belong to WP-38
// (FinePrint/Waypoint.cs, WaypointManager.cs); only the members these two files read and write are modelled here (FinePrint::Waypoint
// below is the WP-38 class reduced to those members), the manager calls are seams (WaypointManagerSeam). WP-38 replaces both.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/proto_vessel.hpp"

namespace ksp::FinePrint {

// FinePrint/Waypoint.cs (reduced)
class Waypoint {
public:
    std::string name = "Waypoint";              // Waypoint.cs: public string name
    std::string celestialName = "";             // celestialName
    double latitude = 0.0;                      // latitude
    double longitude = 0.0;                     // longitude
    Guid navigationId;                          // navigationId
    int seed = 0;                               // seed
    std::string id = "";                        // id
    bool isCustom = false;                      // isCustom
    bool isNavigatable = false;                 // isNavigatable
    bool isOnSurface = false;                   // isOnSurface
    bool landLocked = false;                    // landLocked
    bool isMission = false;                     // isMission
    int index = 0;                              // index
    void SetFadeRange() {}                      // Waypoint.SetFadeRange (the fade distances are GUI)
};

// FinePrint.WaypointManager (seam; unset: nothing is registered / found)
struct WaypointManagerSeam {
    static inline std::function<void(Waypoint*)> addWaypoint;
    static inline std::function<void(Waypoint*)> removeWaypoint;
    static inline std::function<Waypoint*(const Guid&)> findWaypoint;
};

}  // namespace ksp::FinePrint

namespace ksp {

// ProtoWaypointInfo.cs: WP-04 ported the data side (ksp/save/proto_vessel.hpp: Load / Save / the ConfigNode constructor); the two members that
// need the Waypoint class and the WaypointManager are here
// ProtoWaypointInfo.cs:22 ProtoWaypointInfo(Waypoint wp)
ProtoWaypointInfo MakeProtoWaypointInfo(const FinePrint::Waypoint* wp);
// ProtoWaypointInfo.cs:66 FindWaypoint()
FinePrint::Waypoint* FindWaypoint(const ProtoWaypointInfo& info);

// ScenarioCustomWaypoints.cs:8 ([KSPScenario(3198, FLIGHT, TRACKSTATION, MISSIONBUILDER)])
class ScenarioCustomWaypoints : public ScenarioModule {
public:
    ScenarioCustomWaypoints();
    ~ScenarioCustomWaypoints() override;
    static inline ScenarioCustomWaypoints* Instance = nullptr;      // :17
    void OnAwake() override;                                        // :19
    void OnDestroy();                                               // :31
    static void AddWaypoint(FinePrint::Waypoint* waypoint);         // :48
    static void AddWaypoint(FinePrint::Waypoint* waypoint, bool isMission);  // :53
    static void RemoveWaypoint(FinePrint::Waypoint* waypoint);      // :66
    void OnLoad(ConfigNode& node) override;                         // :90
    void OnSave(ConfigNode& node) override;                         // :114
    const std::vector<std::unique_ptr<FinePrint::Waypoint>>& Waypoints() const { return owned_; }
    static void Register();

private:
    static void ProcessWaypoint(FinePrint::Waypoint* waypoint);     // :77
    std::vector<FinePrint::Waypoint*> waypoints_;                   // :14 (null until OnAwake)
    bool awake_ = false;
    std::vector<std::unique_ptr<FinePrint::Waypoint>> owned_;       // the waypoints created by OnLoad
};

}  // namespace ksp
