// WP-37: ProtoWaypointInfo.cs and ScenarioCustomWaypoints.cs (data side). The Waypoint class and the WaypointManager belong to WP-38
// (FinePrint/Waypoint.cs, WaypointManager.cs); only the members these two files read and write are modelled here (FinePrint::Waypoint
// below is the WP-38 class reduced to those members), the manager calls are seams (WaypointManagerSeam). WP-38 replaces both.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/proto_vessel.hpp"

namespace ksp {
class CelestialBody;
namespace Contracts {
class Contract;
}
}  // namespace ksp

namespace ksp::FinePrint {

// FinePrint/Waypoint.cs (WP-38: the data side; the map node, icon, fade and click members are GUI (WP-46) and are not here)
class Waypoint {
public:
    int seed = 0;                               // :14
    int index = 0;                              // :16
    std::string id = "default";                 // :18
    std::string name = "Site";                  // :20
    std::string celestialName;                  // :22
    double latitude = 0.0;                      // :24
    double longitude = 0.0;                     // :26
    double altitude = 0.0;                      // :28
    double height = 0.0;                        // :30
    double radius = 0.0;                        // :32
    bool isOccluded = false;                    // :34
    bool isExplored = false;                    // :36
    bool isClustered = false;                   // :38
    bool isOnSurface = false;                   // :40
    bool isNavigatable = false;                 // :42
    bool isCustom = false;                      // :44
    bool visible = true;                        // :46
    bool landLocked = false;                    // :48
    bool enableMarker = true;                   // :50
    bool enableTooltip = true;                  // :52
    bool blocksInput = true;                    // :54
    bool isMission = false;                     // :56
    int iconSize = 16;                          // :72
    float alphaRatio = -1.0f;                   // :78
    float iconAlpha = 0.0f;                     // :80
    double fadeRange = 1.7976931348623157e308;  // :82
    Guid navigationId;                          // :98 (Guid.Empty until the constructor assigns Guid.NewGuid())
    ksp::Contracts::Contract* contractReference = nullptr;  // :58
    Vector3f orbitPosition;                     // :62
    std::string nodeCaption1, nodeCaption2, nodeCaption3;  // :92 / :94 / :96

    Waypoint();                                 // :126 (navigationId = Guid.NewGuid(): ContractsEnvironment::NewGuid)
    Waypoint(const Waypoint& that);             // :131 (also draws a Guid when that.navigationId is empty)
    CelestialBody* celestialBody();             // :100 (cachedBody walk over FlightGlobals.Bodies by GetName(), falls back to Bodies[0])
    int uniqueSeed() const { return (index + seed) * (index + seed + 1) / 2 + seed; }  // :122 (int arithmetic, wraps like C#)
    // :174 SetFadeRange(referenceAltitude): the fade range is map-icon geometry (ScaledSpace.InverseScaleFactor, WP-46); recorded as a no-op
    void SetFadeRange(double referenceAltitude = -1.0) { (void)referenceAltitude; }
    // :165 setName(uniqueSites, allowNamed): StringUtilities.GenerateSiteName + #autoLOC_7001301
    void setName(bool uniqueSites = true, bool allowNamed = true);
    // :189 / :227: generator null -> new KSPRandom(uniqueSeed)
    void RandomizeNear(double centerLatitude, double centerLongitude, double minimumDistance, double maximumDistance, bool waterAllowed = true,
                       KSPRandom* generator = nullptr);
    void RandomizeNear(double centerLatitude, double centerLongitude, double maximumDistance, bool waterAllowed = true, KSPRandom* generator = nullptr);
    // :233 / :264
    void RandomizeAwayFrom(double originLatitude, double originLongitude, double minimumDistance, double maximumDistance, int samples = 3,
                           bool waterAllowed = true, KSPRandom* generator = nullptr);
    void RandomizeAwayFrom(double originLatitude, double originLongitude, double maximumDistance, int samples = 3, bool waterAllowed = true,
                           KSPRandom* generator = nullptr);

private:
    CelestialBody* cachedBody_ = nullptr;
};

// FinePrint.WaypointManager (WP-38: the static random-position helpers are real; the MonoBehaviour list (seams below) is not)
class WaypointManager {
public:
    // WaypointManager.cs:219 ChooseRandomPosition(out latitude, out longitude, celestialName, waterAllowed, equatorial, generator)
    static void ChooseRandomPosition(double& latitude, double& longitude, const std::string& celestialName, bool waterAllowed = true, bool equatorial = false,
                                     KSPRandom* generator = nullptr);
    // WaypointManager.cs:280 ChooseRandomPositionNear(...)
    static void ChooseRandomPositionNear(double& latitude, double& longitude, double centerLatitude, double centerLongitude, const std::string& celestialName,
                                         double searchRadius, bool waterAllowed = true, KSPRandom* generator = nullptr);
    // WaypointManager.cs:153 Distance(...) in metres as float
    static float Distance(double latitude1, double longitude1, double altitude1, double latitude2, double longitude2, double altitude2, CelestialBody* body);
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
    void OnDestroy();                                               // :30
    static void AddWaypoint(FinePrint::Waypoint* waypoint);         // :50
    static void AddWaypoint(FinePrint::Waypoint* waypoint, bool isMission);  // :50
    static void RemoveWaypoint(FinePrint::Waypoint* waypoint);      // :65
    void OnLoad(ConfigNode& node) override;                         // :86
    void OnSave(ConfigNode& node) override;                         // :112
    const std::vector<std::unique_ptr<FinePrint::Waypoint>>& Waypoints() const { return owned_; }
    static void Register();

private:
    static void ProcessWaypoint(FinePrint::Waypoint* waypoint);     // :74
    std::vector<FinePrint::Waypoint*> waypoints_;                   // (C++ addition) (null until OnAwake)
    bool awake_ = false;
    std::vector<std::unique_ptr<FinePrint::Waypoint>> owned_;       // the waypoints created by OnLoad
};

}  // namespace ksp
