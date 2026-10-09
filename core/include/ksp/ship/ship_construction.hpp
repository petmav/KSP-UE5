// WP-18: ShipConstruction (Assembly-CSharp/ShipConstruction.cs, 1,349 lines): the static helpers around ShipConstruct -- craft file
// backups and paths, loading / saving, AssembleForLaunch (ship -> Vessel), craft size / centre / MOI, launch-site queries on the
// saved FlightState, template -> construct (through the save-upgrade pipeline), part costs (GetPartCosts, GetPartCostsAndMass,
// SanitizePartCosts) and the craft compatibility checks.
//
// Environment: everything the C# reads from HighLogic / EditorDriver / KSPUtil / MissionSystem is in ShipConstructionEnvironment
// (defaults = a sandbox game in the VAB without missions). Thumbnails and the subassembly icon (CraftThumbnail, cameras,
// RenderTexture; ShipConstruction.cs:779-935, 987-1111) are UI/rendering and are not ported; the SaveShip / LoadShip callers of
// CaptureThumbnail call the `captureThumbnail` hook.
//
// Names that exist in KSP 1.12.5's ShipConstruction.cs and are NOT part of this port: see STATUS.md "WP-18" (rendering / thumbnails).
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/save/upgrade_pipeline.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/ship/ship_template.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Game;
class FlightState;
class ProtoVessel;
class Orbit;
class Part;
class Vessel;
struct AvailablePart;
class ProtoPartSnapshot;
struct PartHeightQuery;

// What ShipConstruction.cs reads from HighLogic, EditorDriver, MissionSystem and the Steam / thumbnail UI
struct ShipConstructionEnvironment {
    std::string SaveFolder = "";                 // HighLogic.SaveFolder
    Game* CurrentGame = nullptr;                 // HighLogic.CurrentGame (Mode, launchID, IsMissionMode)
    EditorFacility editorFacility = EditorFacility::VAB;   // EditorDriver.editorFacility
    std::string DefaultCraftSavePath = "";       // EditorDriver.DefaultCraftSavePath
    // MissionSystem.missions[0].MissionInfo (Making History); nullopt = MissionSystem.missions.Count == 0
    struct MissionInfo {
        std::string folderName;
        std::string FolderPath;
    };
    std::optional<MissionInfo> mission;
    ShipConstruct* editorShip = nullptr;         // EditorLogic.fetch.ship
    // KSPSteamUtils.GetSteamIDFromSteamFolder(path); unset: 0
    std::function<uint64_t(const std::string&)> steamIdFromSteamFolder;
    // CaptureThumbnail(ship, baseFolder, thumbURL) (CraftThumbnail.TakeSnaphot, UI); unset: nothing
    std::function<void(ShipConstruct*, const std::string& baseFolder, const std::string& thumbURL)> captureThumbnail;
    // CaptureStockThumbnail(ship, craftPath, expansion) (CraftThumbnail.TakeStockSnaphot, UI); unset: nothing
    std::function<void(ShipConstruct*, const std::string& craftPath, bool expansion)> captureStockThumbnail;
    // PSystemSetup.Instance.GetSpaceCenterFacility(landedAt) == null -> IsStockLaunchSite(landedAt) (AssembleForLaunch; only a lookup)
    std::function<bool(const std::string&)> isStockLaunchSite;
    // Part.SendMessage("OnPutToGround", query): the part modules that adjust the lowest point (landing gear); unset: none
    std::function<void(Part&, PartHeightQuery&)> onPutToGround;
    // AnalyticsUtil.LogVesselLaunched (nothing)
};

// The Transform ShipConstruction.PutShipToGround receives (the launch site's spawn point)
struct SpawnPoint {
    Vector3f position{};
    Quaternionf rotation = Quaternionf::identity();
};

// Where the root part of an assembled ship is placed (the launch-site transform is engine data: the host supplies it)
struct LaunchPlacement {
    Vector3f rootPosition{};
    Quaternionf rootRotation = Quaternionf::identity();
};

class ShipConstruction {
public:
    static inline ShipConstructionEnvironment env;
    // Object.DestroyImmediate(vessel.gameObject) of RecoverVesselFromFlight (the host owns the Vessel objects); unset: nothing
    static inline std::function<void(Vessel&)> destroyVesselGameObject;

    static inline std::vector<ConfigNode::NodePtr> backups;   // :14
    static inline ConfigNode::NodePtr ShipConfig;             // :16
    static inline VesselCrewManifest* ShipManifest = nullptr; // :18
    static inline EditorFacility ShipType = EditorFacility::None;  // :20

    static void ClearBackups();                                // :28
    static void CreateBackup(ShipConstruct& ship);             // :33
    static void ShiftAndCreateBackup(ShipConstruct& ship);     // :40
    static std::unique_ptr<ShipConstruct> RestoreBackup(int index);  // :51
    static void DebugBackup();                                 // :64

    static std::unique_ptr<ShipConstruct> LoadShip(const std::string& filePath);  // :73 (nullptr = the C# null)
    static std::unique_ptr<ShipConstruct> LoadShip();          // :96 (from ShipConfig)
    static std::string SaveShip(ShipConstruct& ship, const std::string& shipFilename);  // :112
    static std::string SaveShip(const std::string& shipFilename);                         // :120
    static std::string SaveShipToPath(std::string shipName, const std::string& path);    // :132
    static std::string SaveShipToPath(const std::string& gameFolder, EditorFacility editorFacility, const std::string& localPath,
                                      std::string shipName);                              // :154
    static std::string GetPlayerCraftThumbnailName(const std::string& fullpath, std::string craftFileName);  // :168
    static std::string GetPlayerCraftThumbnailName(const std::string& gameName, std::string subDirectoryPath, std::string shipName);  // :208
    static std::string GetSavePath(std::string shipName);                                  // :218
    static std::string GetCurrentGameShipsPathFor(EditorFacility facility);                // :236
    static std::string GetShipsPathFor(const std::string& gameName, EditorFacility facility);  // :241
    static std::string GetShipsPathFor(const std::string& gameName);                       // :246
    static std::string GetRootCraftSavePath();                                             // :251
    static EditorFacility CheckCraftFileType(const std::string& filePath);                 // :256
    static std::string GetShipsSubfolderFor(EditorFacility facility);                      // :278

    // :291 / :296. Returns the Vessel (the Unity scene owns it in the game). The ship's live parts move into the Vessel
    // (ship.ownedParts is emptied; ship.liveParts keeps pointing at them). The launch-site placement is `placement`.
    static std::unique_ptr<Vessel> AssembleForLaunch(ShipConstruct& ship, const std::string& landedAt, const std::string& displaylandedAt,
                                                     const std::string& flagURL, Game& sceneState, VesselCrewManifest* crewManifest,
                                                     const LaunchPlacement& placement = {});
    static std::unique_ptr<Vessel> AssembleForLaunch(ShipConstruct& ship, const std::string& landedAt, const std::string& displaylandedAt,
                                                     const std::string& flagURL, Game& sceneState, VesselCrewManifest* crewManifest,
                                                     bool fromShipAssembly, bool setActiveVessel, bool isLanded, bool preCreate,
                                                     std::shared_ptr<Orbit> orbit, bool orbiting, bool isSplashed,
                                                     const LaunchPlacement& placement = {});
    static uint32_t GetUniqueFlightID(const FlightState* flightState);   // :436
    static Part* findFirstCrewablePart(Part* part);                      // :447
    static Part* findFirstPod_Placeholder(Part* part);                   // :473
    static Part* findFirstControlSource(Vessel& v);                      // :499
    static void PutShipToGround(ShipConstruct& ship, const SpawnPoint& spawnPoint);  // :521

    static Vector3f CalculateCraftSize(ShipConstruct& ship);                      // :555
    static Vector3f CalculateCraftSize(const std::vector<Part*>& parts, Part* rootPart);  // :564
    static Vector3f CalculateCraftSize(const ShipTemplate& ship);                 // :599
    static Vector3f FindCraftCenter(ShipConstruct& ship);                         // :608
    static Vector3f FindCraftCenter(ShipConstruct& ship, bool excludeClamps);     // :613
    static Vector3f FindCraftMOI(Part& rootPart, const Vector3f& CoM);            // :631
    static Vector3f FindCraftMOI(Part& part, const Vector3f& CoM, Vector3f MOI);  // :636

    // :654 / :661 [Obsolete]
    static bool CheckLaunchSiteClear(FlightState* flightState, const std::string& launchSiteName, bool doCleanup);
    static bool CheckLaunchSiteClear(FlightState* flightState, const std::string& launchSiteName, bool doCleanup, int& firstVesselOnPadIndex);
    static std::vector<ProtoVessel*> FindVesselsLandedAt(const FlightState* flightState, const std::string& landedAt);  // :701
    static void FindVesselsLandedAt(const FlightState* flightState, const std::string& landedAt, int& count, std::string& name, int& idx,
                                    VesselType& vType);                          // :718
    // :748 [Obsolete]: the C# body calls itself (StackOverflowException); throws ksp::Exception("StackOverflowException")
    static std::vector<ProtoVessel*> FindVesselsAtLaunchSite(const FlightState* flightState, const std::string& launchSiteName);
    static void RecoverVesselFromFlight(ProtoVessel& vessel, FlightState& fromState);               // :753
    static void RecoverVesselFromFlight(ProtoVessel& vessel, FlightState& fromState, bool quick);   // :758

    // :829 / :846: the part-0 highlight is reset (Part.HighlightRecursive, UI), then the thumbnail hook of the environment runs
    static void CaptureStockThumbnail(ShipConstruct* ship, const std::string& craftPath, bool expansion);
    static void CaptureThumbnail(ShipConstruct* ship, const std::string& baseFolder, const std::string& thumbURL);

    static std::unique_ptr<ShipTemplate> LoadTemplate(const std::string& shipFilename);  // :937
    static std::string SaveSubassembly(ShipConstruct& ship, const std::string& shipFilename);   // :950
    static bool SubassemblyExists(const std::string& shipFilename);                      // :959
    static std::unique_ptr<ShipConstruct> LoadSubassembly(const std::string& shipFilename);     // :966

    // :1113 / :1124 / :1132: template.config goes through KSPUpgradePipeline.Process (LoadContext.Craft); `onComplete(nullptr)` when
    // the load fails. The callbacks run synchronously when the pipeline does.
    static void CreateConstructFromTemplate(ShipTemplate& tmpl, const std::function<void(std::unique_ptr<ShipConstruct>)>& onComplete);
    static void SanitizeCraftIDs(const std::vector<Part*>& parts, bool preserveIDsOnGivenParts);  // :1155

    static float GetPartCosts(ProtoPartSnapshot& protoPart, const AvailablePart* aP, float& dryCost, float& fuelCost);  // :1197
    static float GetPartCosts(ProtoPartSnapshot& protoPart, bool includeModuleCosts, const AvailablePart* aP, float& dryCost,
                              float& fuelCost);                                       // :1202
    static float GetPartCostsAndMass(const ConfigNode& partNode, const AvailablePart* aP, float& dryCost, float& fuelCost, float& dryMass,
                                     float& fuelMass);                                 // :1238
    static void SanitizePartCosts(AvailablePart& aP, const ConfigNode& partNode);      // :1292
    static Part* FindPartWithCraftID(uint32_t craftID);                                // :1304
    static bool AllPartsFound(const ConfigNode* root, std::string* error);             // :1309
    static bool IsCompatible(const ConfigNode* root, std::string* error);              // :1335

private:
    static void OnPipelineFailed(KSPUpgradePipeline::UpgradeFailOption option, const ConfigNode::NodePtr& n, ShipTemplate& tmpl,
                                 const std::function<void(std::unique_ptr<ShipConstruct>)>& onComplete);  // :1124
    static void OnPipelineFinished(const ConfigNode::NodePtr& node, ShipTemplate& tmpl,
                                   const std::function<void(std::unique_ptr<ShipConstruct>)>& onComplete);  // :1132
};

}  // namespace ksp
