// WP-42: PSystemSetup.cs (the space-center facility / launch-site registry, the launch-site setup and the per-scene mode switches).
//
// Data: the serialized `facilities` and `stocklaunchsites` arrays of the PSystemSetup prefab live in the level scene and are NOT in
// any extracted dataset (see STATUS.md "Needs data"): they are filled by the host / a test fixture. The PQSCity / PQSCity2 components
// they bind to are the stock table of pqs_city.hpp.
// Seams (WP-47 scene / WP-12 PQS): the Unity hierarchy lookups (SceneGraph), FloatingOrigin.SetOffset / PQS.SetTarget / ForceInitPQS
// and the camera / sun toggles of SetFlight etc. (`sceneHook`), the MakingHistory bundle prefabs (`instantiateBundlePrefab`).
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/core/coroutine.hpp"
#include "ksp/space_center/launch_site.hpp"

namespace ksp {

class CelestialBody;
namespace Upgradeables {
class UpgradeableObject;
}

class PSystemSetup {
public:
    // :12
    class SpaceCenterFacility {
    public:
        // :15
        class SpawnPoint {
        public:
            std::string name;                // :18
            std::string spawnTransformURL;   // :19
            double latitude = 0.0;           // :20
            double longitude = 0.0;          // :21
            double altitude = 0.0;           // :22
            bool latlonaltSet = false;       // :23
            void Setup(SpaceCenterFacility* hostFacility);                                // :28
            std::optional<Vector3d> GetSpawnPointTransform();                            // :33
            void GetSpawnPointLatLonAlt(double& lat, double& lon, double& alt) const;    // :44
            void SetSpawnPointLatLonAlt();                                               // :50

        private:
            SpaceCenterFacility* host = nullptr;
            bool transformFound = false;
            Vector3d transformPosition;
        };

        std::string name;                   // :73
        std::string pqsName;                // :74
        std::string facilityTransformName;  // :75
        std::string facilityName;           // :76
        std::string facilityDisplayName;    // :77
        PQS* facilityPQS = nullptr;         // :78
        CelestialBody* hostBody = nullptr;  // :79
        bool facilityTransformFound = false;   // facilityTransform != null
        Vector3d facilityTransformPosition;
        EditorFacility editorFacility = EditorFacility::None;   // :81
        std::vector<SpawnPoint> spawnPoints;                    // :82
        Upgradeables::UpgradeableObject* upgradeableObject = nullptr;   // :83
        // scene data of ScenarioUpgradeableFacilities.getInitialState / getInitialMissionState (the Unity components on / above the
        // facility transform; "" = none): HierarchyUtil.CompileID(GetComponentInParent<UpgradeableObject>().transform, "SpaceCenter"),
        // the same for GetComponent<UpgradeableFacility>() and that component's MaxLevel
        std::string upgradeableObjectInParentId;
        std::string upgradeableFacilityId;
        int upgradeableFacilityMaxLevel = 0;

        void Setup(const std::vector<space_center::PQSInfo>& PQSs, const std::function<CelestialBody*(const space_center::PQSInfo&)>& bodyOf);   // :84
        void SetSpawnPointsLatLonAlt();                       // :112
        SpawnPoint* GetSpawnPoint(const std::string& spawnPointName);   // :120
        float GetFacilityDamage();                            // :146
        float GetFacilityLevel();                             // :150
        bool IsLaunchFacility() const;                        // :154
    };

    GameScenes nextScene = GameScenes::MAINMENU;   // :162
    bool loadTestScene = false;                    // :164
    std::string pqsToActivate = "Kerbin";          // :165
    std::string pqsTransformToCache = "KSC";       // :166

    static inline PSystemSetup* Instance = nullptr;   // :186 (private set)
    std::vector<SpaceCenterFacility> facilities;      // :168 SpaceCenterFacilities
    std::vector<SpaceCenterFacility*> spaceCenterFacilityLaunchSites;   // :170
    std::vector<space_center::PQSInfo> pqsArray;      // :173
    std::vector<std::unique_ptr<LaunchSite>> stocklaunchsites;   // :175 StockLaunchSites
    std::vector<LaunchSite*> launchsites;             // :177 LaunchSites (stock + added)
    std::function<CelestialBody*(const space_center::PQSInfo&)> bodyOfPQS;   // pQS.GetComponentInParent<CelestialBody>()

    // seams
    // PQSCity / PQSCity2 component on pqsName/launchSiteTransformURL (transform.GetComponent<PQSCity>()); nullptr = none
    std::function<PQSCity*(const std::string& pqsName, const std::string& path)> findPQSCity;
    std::function<PQSCity2*(const std::string& pqsName, const std::string& path)> findPQSCity2;
    // Expansions bundle prefab of a launch site (BundleLoader.LoadAsset + Instantiate under the PQS): returns the created component, nullptr = none
    std::function<bool(LaunchSite& site, const std::string& pqsName)> instantiateBundlePrefab;
    // FloatingOrigin.SetOffset(position) / PQS.SetTarget / ForceInitPQS / Planetarium reset etc. of SetupLaunchSites and the scene modes
    // `position` (when not null) is the world position the C# passes to FloatingOrigin.SetOffset in that step
    std::function<void(const std::string& what, const Vector3d* position)> sceneHook;
    // transform.GetComponent<UpgradeableObject>() of a facility transform (onUpgradeableObjLevelChange); nullptr = none
    std::function<Upgradeables::UpgradeableObject*(SpaceCenterFacility&)> facilityUpgradeableObject;
    void OnSceneLoaded(int buildIndex);                                                      // :826 (SceneManager.sceneLoaded)

    PSystemSetup();
    ~PSystemSetup();
    PSystemSetup(const PSystemSetup&) = delete;
    PSystemSetup& operator=(const PSystemSetup&) = delete;

    std::vector<SpaceCenterFacility>& SpaceCenterFacilities() { return facilities; }         // :194
    std::vector<LaunchSite*> NonStockLaunchSites() const;                                    // :207
    std::vector<LaunchSite*>& LaunchSites() { return launchsites; }                          // :233

    void Awake();                                                                            // :238
    void Start();                                                                            // :248
    void OnDestroy();                                                                        // :265
    void onUpgradeableObjLevelChange(Upgradeables::UpgradeableObject* upgradeObject, int level);   // :276
    void SetupFacilities();                                                                  // :378
    Enumerator SetupLaunchSites();                                                           // :387
    bool IsStockLaunchSite(const std::string& name) const;                                   // :608
    bool IsFacilityOrLaunchSite(const std::string& inputName, bool& isFacility, std::string& displayName);   // :628
    bool IsFacility(const std::string& inputName, std::string& displayName);                 // :641
    bool IsLaunchSite(const std::string& inputName, std::string& displayName);               // :659
    void RemoveNonStockLaunchSites() {}                                                      // :677
    LaunchSite* GetLaunchSite(const std::string& name);                                      // :682
    CelestialBody* GetLaunchSiteBody(const std::string& name);                               // :709
    std::string GetLaunchSiteBodyName(const std::string& name);                              // :719
    std::string GetLaunchSiteDisplayName(const std::string& name);                           // :729
    bool AddLaunchSite(LaunchSite* launchsite, bool overRide = false);                       // :745
    bool RemoveLaunchSite(const std::string& name);                                          // :779
    SpaceCenterFacility* GetSpaceCenterFacility(const std::string& name);                    // :813
    void LoadTestScene(GameScenes sceneBase);                                                // :835
    void OnLevelLoaded(GameScenes level);                                                    // :843
    void OnSceneChange(GameScenes scene);                                                    // :879

    // Scene modes (SetSpaceCentre etc.): the data effects (Planetarium pause / timeScale, the bodies' inverseRotation / orbitDriver.reverse,
    // FlightGlobals.currentMainBody); the camera / sun / scaled-space toggles are `sceneHook(<mode>)`.
    CelestialBody* kscBody = nullptr;   // cb.body (the PQSMod_CelestialBodyTransform's body of pqsToActivate)
    void SetMainMenu();           // :902
    void SetSpaceCentre();        // :925
    void SetMissionEditor();      // :969
    void SetFlight();             // :976
    void SetTrackingStation();    // :993
    void SetEditor();             // :1016
    void SetDisabled();           // :1040
    // PQS.isDisabled / ResetSphere / ResetAndWait / ForceStart over pqsArray: engine PQS runtime, forwarded to sceneHook
    void SetPQSInactive(bool keepActiveInFlight = false);   // :1109
    void SetPQSActive(PQS* pqs);                            // :1130
    void SetPQSActive();                                    // :1146
    void SetPQSDisabled();                                  // :1157
    void ResetTransforms();                                 // :1167

    // Installs the seams the other packages read (CareerEnvironment spaceCenterBody / launchSites / isStockLaunchSite / launchSiteDistance /
    // spaceCenterFacilities / launchSiteFound, PreFlightEnvironment isSpaceCenterFacility / facilityDamage / isLaunchSite, Vessel's
    // isFacilityOrLaunchSite, ShipConstruction's stock-launch-site lookup) through PSystemSetup::Instance and SpaceCenter::Instance.
    static void InstallSeams();
    static void UninstallSeams();
};

}  // namespace ksp
