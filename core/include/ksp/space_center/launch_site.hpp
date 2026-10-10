// WP-42: LaunchSite.cs, ISiteNode.cs, KSCSiteNode.cs.
//
// The Unity Transform hierarchy (pqs.transform.Find(url), launchSiteTransform.Find(spawnTransformURL), transform.position) is the
// scene's (WP-47): SceneGraph below are its lookups. Both return the transform's world position, or nullopt for "Find returned
// null". Unset = nothing is ever found (a LaunchSite.Setup then fails with the "unable to find LaunchSite Transform" error like a
// scene without the object). The world positions feed CelestialBody.GetLatLonAlt, so a host and the CelestialBody must agree on
// the world frame.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/math/vector3d.hpp"
#include "ksp/save/save_enums.hpp"

namespace ksp {

class CelestialBody;
class PQS;
class PQSCity;
class PQSCity2;

namespace space_center {

struct SceneGraph {
    // PQSs[i].transform.Find(path) (the PQS game object named pqsName): the found transform's world position
    static inline std::function<std::optional<Vector3d>(const std::string& pqsName, const std::string& path)> FindInPQS;
    // <transform found at pqsName/parentPath>.Find(childPath)
    static inline std::function<std::optional<Vector3d>(const std::string& pqsName, const std::string& parentPath, const std::string& childPath)> FindChild;
    static void Reset() {
        FindInPQS = nullptr;
        FindChild = nullptr;
    }
};

// a PQS sphere game object (PSystemSetup.pqsArray element): GameObject name and the sphere
struct PQSInfo {
    std::string gameObjectName;
    PQS* pqs = nullptr;
};

}  // namespace space_center

// ISiteNode.cs:5
class ISiteNode {
public:
    virtual ~ISiteNode() = default;
    virtual std::string GetName() = 0;
    virtual std::optional<Vector3d> GetWorldPos() = 0;             // Transform GetWorldPos(): the world position, nullopt = null
    virtual std::string NodeCaptionHeader() = 0;                   // UpdateNodeCaption: data.Header = ...
};

// LaunchSite.cs:8
class LaunchSite : public ISiteNode {
public:
    // :11
    class SpawnPoint {
    public:
        SpawnPoint() = default;
        SpawnPoint(std::string spawnName, std::string url) : name(std::move(spawnName)), spawnTransformURL(std::move(url)) {}
        std::string name;                 // :13
        std::string spawnTransformURL;    // :15
        double latitude = 0.0;            // :17
        double longitude = 0.0;           // :19
        double altitude = 0.0;            // :21
        bool latlonaltSet = false;        // :23

        void Setup(LaunchSite* hostSite);                                    // :30
        std::optional<Vector3d> GetSpawnPointTransform();                    // :35 (the transform's world position; nullopt = null)
        void GetSpawnPointLatLonAlt(double& lat, double& lon, double& alt) const;   // :49
        void SetSpawnPointLatLonAlt();                                       // :56

    private:
        LaunchSite* host = nullptr;
        bool transformFound = false;     // spawnPointTransform != null
        Vector3d transformPosition;
    };

    std::string name;                     // :80
    std::string pqsName;                  // :82
    PQS* launchsitePQS = nullptr;         // :84
    PQSCity* pqsCity = nullptr;           // :86
    PQSCity2* pqsCity2 = nullptr;         // :88
    std::string launchSiteName;           // :92 (a localisation tag)
    std::vector<SpawnPoint> spawnPoints;  // :94
    std::string launchSiteTransformURL;   // :98
    std::string prefabPath;               // :101
    std::vector<std::string> additionalprefabPaths;   // :104
    std::string BundleName;               // :107
    EditorFacility editorFacility = EditorFacility::None;   // :109
    int nodeType = 0;                     // :111 (MapNode.SiteType)
    bool requiresPOIVisit = false;        // :115
    std::string poiName;                  // :117
    bool launchSiteTransformFound = false;   // launchSiteTransform != null
    Vector3d launchSiteTransformPosition;

    bool isPQSCity() const { return pqsCity != nullptr; }     // :125
    bool isPQSCity2() const { return pqsCity2 != nullptr; }   // :127
    bool IsSetup() const { return isSetup_; }                 // :129
    CelestialBody* Body();                                    // :131
    bool HasLaunchSiteTransform() const { return launchSiteTransformFound; }

    LaunchSite(const std::string& name, const std::string& pqsName, const std::string& launchSiteName, std::vector<SpawnPoint> spawnPoints,
               const std::string& launchSiteTransformURL, EditorFacility editorFacility);   // :150
    bool Setup(PQSCity* city, const std::vector<space_center::PQSInfo>& PQSs);       // :160
    bool Setup(PQSCity2* city, const std::vector<space_center::PQSInfo>& PQSs);      // :160
    SpawnPoint* GetSpawnPoint(const std::string& spawnPointName);                    // :212
    void SetSpawnPointsLatLonAlt();                                                  // :232
    std::string GetName() override;                                                  // :240
    std::optional<Vector3d> GetWorldPos() override;                                  // :245
    std::string NodeCaptionHeader() override;                                        // :258
    double GreatCircleDistance(const Vector3d& fromSrfNrm);                          // :263

private:
    bool SetupCommon(const std::vector<space_center::PQSInfo>& PQSs);
    void setupGreatCircleVariables();                                                // :277
    bool isSetup_ = false;
    CelestialBody* body_ = nullptr;
    Vector3d srfNVector_;
    bool greatCircleVarsSet_ = false;
};

// KSCSiteNode.cs:5
class KSCSiteNode : public ISiteNode {
public:
    std::string GetName() override { return "KSC"; }                // :9
    std::string GetDisplayName();                                   // :14
    std::optional<Vector3d> GetWorldPos() override;                 // :19 (the Runway facility transform)
    std::string NodeCaptionHeader() override { return GetDisplayName(); }   // :28
};

}  // namespace ksp
