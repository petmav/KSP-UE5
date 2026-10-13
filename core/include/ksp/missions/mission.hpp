// WP-54: Mission.cs (graph initialisation, Load / Save, the run-time node state machine, objective printing, lookups),
// MissionSituation.cs, MissionInstructor.cs, MissionPartFilter.cs (data), MissionMappedVessel.cs + MissionMappedVesselsExtension.cs,
// MissionPack.cs, the path / part-rename helpers of MissionsUtils.cs, and the stand-ins for the types other packages own
// (MEBannerEntry: WP-56, VesselSituation + the create actions: WP-54 slice 2 / WP-55, MissionFileInfo: slice 2).
//
// WP-54c ported the craft-file bookkeeping (RebuildCraftFileList, ProcessCraftFileFolder, ProcessCraftRenames, AddMappedPartVesselName, the Get*Craft*
// lookups), the vessel-event handlers that feed it (OnPartVesselNamingChanged, OnVesselRename, OnPartDecoupleNewVesselComplete, OnVesselPersistentIdChanged,
// OnPartPersistentIdChanged, OnVesselDocking, OnVesselsUndocking) and ProcessMappedVesselNodeMap / RemapVesselId; they are active when
// MissionsEnvironment::craftFileBookkeeping is true (the WP-54 hooks otherwise).
// Not ported here (see STATUS.md WP-54 "Not ported"): BlockPlayMission's dialog, MissionCriticalError / ContinueCriticalError (UI + scene reset), OnEditorStarted (R&D cheat), SetLockedCamera,
// the UpdateMission autosave file copy and the Steam / file lookup of Load (:1976-2012).
#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/missions/dynamic_module.hpp"
#include "ksp/missions/mission_display.hpp"
#include "ksp/missions/mission_flow.hpp"
#include "ksp/missions/mission_node.hpp"
#include "ksp/missions/mission_data.hpp"
#include "ksp/missions/mission_types.hpp"
#include "ksp/missions/vessel_restrictions.hpp"
#include "ksp/util/collections.hpp"

namespace ksp {
class KerbalRoster;
class VesselCrewManifest;
class GameParameters;
}  // namespace ksp

namespace ksp::Expansions::Missions {

class MissionScore;
class MissionAwards;
class MissionPartFilter;
class MissionSituation;

namespace Editor {
struct MissionValidationTestResult;
}

// ---- MEBannerType.cs / MEBannerEntry.cs (WP-56 owns the real class: textures, file copy; this keeps the strings Mission reads) -------
enum class MEBannerType { Menu = 0, Success = 1, Fail = 2 };

class MEBannerEntry {
public:
    explicit MEBannerEntry(MEBannerType bannerType);                                 // MEBannerEntry.cs:50
    std::string fileName;                                                            // :10
    bool isInMissionFolder = false;                                                  // :12
    void LoadFromMissionFolder(const std::string& newFileName, MEBannerType bannerType, const class Mission* mission);  // :55
    void SetToDefault(MEBannerType bannerType);                                      // :94
    static std::string GetBannerGameDataPath(MEBannerType bannerType);               // :112
    static std::string GetStockBannerGameDataPath(MEBannerType bannerType);          // :122
    std::string FullPath(const std::string& applicationRootPath) const;             // :34
    std::string relativePath;                                                        // :20
    std::string missionFolderPath;                                                   // :22
};

// ---- MissionMappedVessel.cs:6 ------------------------------------------------------------------------------------------------
class MissionMappedVessel {
public:
    uint32_t partPersistentId = 0;          // :9
    uint32_t mappedVesselPersistentId = 0;  // :11
    uint32_t currentVesselPersistentId = 0;  // :13
    uint32_t originalVesselPersistentId = 0;  // :15
    std::string partVesselName;             // :17
    std::string craftFileName;              // :19
    std::string situationVesselName;        // :21
    // :23: mappedVesselPersistentId = FlightGlobals.GetUniquepersistentId()
    MissionMappedVessel(uint32_t partPersistentId, uint32_t originalVesselId, uint32_t currentVesselId, const std::string& partVslName,
                        const std::string& craftFileName, const std::string& situationVslName = "");
    // :35 / :61
    static std::vector<MissionMappedVessel> Load(const ConfigNode& node);
    static ConfigNode* Save(ConfigNode& node, const std::vector<MissionMappedVessel>& list);
    static inline std::function<uint32_t()> getUniquePersistentId;  // FlightGlobals.GetUniquepersistentId (WP-19). Unset: a counter from 1
};
// MissionMappedVesselsExtension.cs:6 (extension methods on List<MissionMappedVessel>)
namespace MappedVessels {
int IndexPartId(const std::vector<MissionMappedVessel>& list, uint32_t id);            // :8
int IndexMappedVesselId(const std::vector<MissionMappedVessel>& list, uint32_t id);    // :23
int IndexCurrentVesselId(const std::vector<MissionMappedVessel>& list, uint32_t id);   // :38
int MappedVesselName(const std::vector<MissionMappedVessel>& list, const std::string& vesselName);  // :53
uint32_t ConvertMappedId(const std::vector<MissionMappedVessel>& list, uint32_t id);   // :68
bool Exists(const std::vector<MissionMappedVessel>& list, uint32_t vslId, uint32_t partId, const std::string& vesselName, const std::string& craftName);  // :89
const MissionMappedVessel* Find(const std::vector<MissionMappedVessel>& list, uint32_t vslId, uint32_t partId, const std::string& vesselName, const std::string& craftName);  // :111
}  // namespace MappedVessels

// ---- MissionInstructor.cs:6 --------------------------------------------------------------------------------------------------
class MissionInstructor {
public:
    MissionInstructor();                                    // :20
    std::string instructorName;                             // :12
    bool vintageSuit = false;                               // :15
    std::string GetNodeBodyParameterString(const Editor::BaseAPField* field);  // :43 (the field argument is unused by the C#)
    bool Equals(const MissionInstructor& other) const;      // :103
    int32_t GetHashCode() const;                            // :112
    void Save(ConfigNode& node) const;                      // :123
    void Load(const ConfigNode& node);                      // :132
};

// ---- MissionPartFilter.cs:9 (the editor-facing half, MEGUI part pickers and EditorPartList filtering, is UI) ------------------
class MissionPartFilter {
public:
    enum class Choices { availableParts = 0, unavailableParts = 1 };  // :11
    MissionPartFilter(Mission* mission, VesselSituation* vesselSituation);  // :44
    Choices filterType = Choices::unavailableParts;         // :21
    std::vector<std::string> availableParts;                // :24 (private)
    std::vector<std::string> unavailableParts;              // :29 (private)
    std::vector<std::string> GetExcludedParts();            // :105
    bool CheckPartExcludelist(const std::string& partName) const;  // :155
    void Load(const ConfigNode& node);                      // :236
    void Save(ConfigNode& node) const;                      // :254
    Mission* mission = nullptr;
    VesselSituation* vesselSituation = nullptr;
};

// ---- VesselSituation.cs:11 ---------------------------------------------------------------------------------------------------
// (the editor controls: requiredPartsParameter / autoGenerateCrewParameter, UpdateRequiredPartsDisplayedParts, OnRequiredPartsParameterCreated,
// OnAutoGenerateCrewControlCreated and GetRequiredExcludedParts' dictionary are the MEGUI panel's)
class VesselSituation {
public:
    VesselSituation();                                      // :61
    VesselSituation(Mission* mission, MENode* node);        // :71
    ~VesselSituation();
    VesselSituation(const VesselSituation&) = delete;
    VesselSituation& operator=(const VesselSituation&) = delete;

    bool playerCreated = true;                              // :14
    std::string craftFile;                                  // :16
    uint32_t persistentId = 0;                              // :18
    std::string vesselName = "#autoLOC_8100086";            // :20
    std::string vesselDescription;                          // :23
    std::unique_ptr<VesselLocation> location;               // :26
    std::unique_ptr<VesselRestrictionList> vesselRestrictionList;  // :29
    Mission* mission = nullptr;                             // :32
    MENode* node = nullptr;                                 // :34
    std::vector<Crew> vesselCrew;                           // :37
    bool focusonSpawn = false;                              // :40
    bool autoGenerateCrew = false;                          // :43
    bool readyToLaunch = false;                             // :46 (internal)
    bool launched = false;                                  // :49 (internal)
    std::unique_ptr<MissionPartFilter> partFilter;          // :52
    std::vector<std::string> requiredParts;                 // :55

    void SetVesselCrew(KerbalRoster& crewRoster, VesselCrewManifest& newCrew);  // :86
    void SetCrewAvailable(KerbalRoster& crewRoster);        // :143
    void StartEditorEvents();                               // :177
    void ClearEditorEvents();                               // :182
    void UpdateAutoGenerateCrewSettings(bool newValue) { autoGenerateCrew = newValue; }  // :191 (the toggle is the panel's)
    std::vector<std::pair<std::string, std::vector<std::string>>> GetRequiredExcludedParts();  // :157
    bool OnRequieredPartsCheckpointValidator(const std::vector<std::string>& value);  // :219
    void Load(const ConfigNode& node);                      // :224
    void Save(ConfigNode& node);                            // :287
};

// ---- MissionSituation.cs:13 --------------------------------------------------------------------------------------------------
class MissionSituation : public IMENodeDisplay {
public:
    enum class StartTypeEnum { VAB = 0, SPH = 1, VesselList = 2, SpaceCenter = 3, SaveFile = 4 };  // :15
    MissionSituation();                                     // :211
    explicit MissionSituation(Mission* mission);            // :224
    ~MissionSituation() override;

    StartTypeEnum startType = StartTypeEnum::VesselList;    // :34
    uint32_t startVesselID = 0;                             // :36
    // :38 / :41 / :64 / :66 (internal)
    DictionaryValueList<VesselSituation*, Guid> vesselSituationList;
    std::vector<IActionModule*> startingActions;
    std::shared_ptr<ConfigNode> vesselsToBuildNode;
    std::shared_ptr<ConfigNode> startingActionsNode;
    Mission* mission = nullptr;                             // :43
    std::string missionRoster;                              // :46
    bool autoGenerateCrew = false;                          // :49
    std::shared_ptr<KerbalRoster> crewRoster;               // :51
    double startUT = 0.0;                                   // :54
    std::shared_ptr<GameParameters> gameParameters;         // :57
    std::vector<std::string> parametersDisplayedInSAP;      // :59
    std::unique_ptr<MissionPartFilter> partFilter;          // :62
    int resourceSeed = 0;                                   // :69
    int rocMissionSeed = 0;                                 // :72
    bool showBriefing = false;                              // :75
    MissionInstructor missionInstructor;                    // :78
    uint32_t currentVesselToBuildId = 0;                    // :82 (internal)
    int vesselsBuilt = 0;                                   // :84 (internal)

    DictionaryValueList<VesselSituation*, Guid>& VesselSituationList() { return vesselSituationList; }  // :88
    bool VesselsArePending() const;                         // :125
    int NumberofVesselsPending() const;                     // :137
    uint32_t CurrentVesselToBuildId() const { return currentVesselToBuildId; }  // :154
    VesselSituation* CurrentVesselToBuild();                // :156
    int VesselsBuilt() const { return vesselsBuilt; }       // :182

    void InitSituation();                                   // :238
    void DestroySituation();                                // :254
    void RunValidationWrapper(Editor::MissionEditorValidator* validator);  // :303 (internal)
    virtual void RunValidation(Editor::MissionEditorValidator* validator) { (void)validator; }  // :308
    void Load(const ConfigNode& node);                      // :312
    void SaveVesselsToBuild(ConfigNode& node);              // :435 (internal)
    void Save(ConfigNode& node);                            // :447
    VesselSituation* GetVesselSituationByVesselID(uint32_t PersistentId);  // :507
    bool VesselSituationListContains(uint32_t persistentId);  // :525
    void FillVesselSituationList(MENode* missionStartNode, bool replaceIfFound = false, bool fillAllNodes = false);  // :543
    void resetVesselsCountsLists();                         // :595 (internal)
    void UpdateVesselBuildValues();                         // :601
    std::vector<VesselSituation*> GetAvailableVesselSituations();  // :626
    bool AddNodeWithVesselToBuild(MENode* node);            // :647
    bool VesselSituationReadyToLaunch(VesselSituation* situation);   // :677
    bool VesselSituationRevertLaunch(VesselSituation* situation);    // :696
    bool VesselSituationLaunched(VesselSituation* situation);        // :715
    void RemoveLaunchedVessels();                           // :734

    // IMENodeDisplay (:753-889)
    std::string GetName() override;
    std::string GetDisplayName() override;
    void AddParameterToSAP(const std::string& parameter) override;
    void RemoveParameterFromSAP(const std::string& parameter) override;
    void AddParameterToNodeBody(const std::string& parameter) override;
    void AddParameterToNodeBodyAndUpdateUI(const std::string& parameter) override;
    void RemoveParameterFromNodeBody(const std::string& parameter) override;
    void RemoveParameterFromNodeBodyAndUpdateUI(const std::string& parameter) override;
    bool HasNodeBodyParameter(const std::string& parameter) override;
    bool HasSAPParameter(const std::string& parameter) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // editor reflection: not ported
    void UpdateNodeBodyUI() override;
    std::vector<IMENodeDisplay*> GetInternalParametersToDisplay() override;
    MENode* GetNode() override;
    std::string GetInfo() override;
    void ParameterSetupComplete() override;
    void Initialize() {}
    void Destroy() {}

private:
    void setStartType();                                    // :352
    MENode* StartNode();                                    // :184
    MENode* startNode_ = nullptr;
};

// ---- MissionPack.cs:7 --------------------------------------------------------------------------------------------------------
class MissionPack {
public:
    MissionPack();                                          // :27
    std::string name, displayName, description;             // :10-:16
    int order = 0;                                          // :19
    std::string color = "#5576AEFF";                        // :22
    static int CompareOrder(const MissionPack& a, const MissionPack& b);        // :35
    static int CompareDisplayName(const MissionPack& a, const MissionPack& b);  // :45
    void Load(const ConfigNode& node);                      // :50
    void Save(ConfigNode& node) const;                      // :72
};

// ---- MissionFileInfo.cs:12 (the path properties; the constructor reads the .mission through MissionSystem and Steam: slice 2) ---------
class MissionFileInfo {
public:
    std::string idName;                                     // :14 (internal)
    std::string folderName;                                 // :16
    MissionTypes missionType = MissionTypes::Stock;         // :18
    std::string title, briefing, author, modsBriefing, packName;  // :20-:28
    int order = 0;                                          // :30
    bool hardIcon = false;                                  // :32
    MissionDifficulty difficulty = MissionDifficulty::Intermediate;  // :34
    std::vector<std::string> tags;                          // :36
    bool IsTutorialMission() const { return packName == "squad_MakingHistory_Tutorial"; }  // :95
    std::string FolderPath() const;                         // :97
    std::string ShipFolderPath() const;                     // :99
    std::string FilePath() const;                           // :101
    std::string BannerPath() const;                         // :113
    std::string SaveFolderPath() const;                     // :115
};

// ---- MissionsUtils.cs:11 (the pure helpers) ----------------------------------------------------------------------------------
namespace MissionsUtils {
inline constexpr const char* fileExtension = ".mission";    // :130
// the folder roots (KSPUtil.ApplicationRootPath, ExpansionsUtils.ExpansionsGameDataPath): host paths, "/"-terminated
struct Paths {
    static inline std::string applicationRootPath = "";                       // KSPUtil.ApplicationRootPath
    static inline std::string expansionsGameDataPath = "GameData/SquadExpansion/";  // KSPExpansionsUtils.ExpansionsGameDataPath
    static inline std::string steamWorkshopFolder = "";                       // SteamManager.KSPSteamWorkshopFolder
    static inline bool isTestMode = false;                                    // MissionSystem.IsTestMode
    static inline bool loadedSceneIsMissionBuilder = false;                   // HighLogic.LoadedScene == MISSIONBUILDER
};
std::string ExpansionRootPath();                            // :178
std::string StockMissionsPath();                            // :180
std::string BaseMissionsPath();                             // :182
std::string UsersMissionsPath();                            // :184
std::string SavesRootPath();                                // :192
std::string SavesPath();                                    // :194
std::string GetMissionsFolder(MissionTypes missionType);    // :320
std::string UpdatePartName(const std::string& partName);    // :593 (internal)
void UpdatePartNames(std::vector<std::string>& partNames);  // :582 (internal)
std::vector<MENode*> GetNodesOrdered(Mission* m, bool objectivesOnly);  // :521 (internal)
// MissionsUtils.Objectives.BreadthSearch (:58) over the adjacency list built by GetNodesOrdered; returns the visit order
std::vector<MENode*> BreadthSearch(const std::vector<MENode*>& vertices, const std::vector<std::pair<MENode*, MENode*>>& edges, MENode* start);
}  // namespace MissionsUtils

// ---- Mission.cs:14 -----------------------------------------------------------------------------------------------------------
class Mission {
public:
    // Mission.Spawn() :314 (+ Awake :254): the MonoBehaviour is a plain object; the host owns it
    Mission();
    explicit Mission(std::shared_ptr<MissionFileInfo> missionInfo);          // :323
    ~Mission();
    Mission(const Mission&) = delete;
    Mission& operator=(const Mission&) = delete;
    // :331 SpawnAndLoad
    static std::unique_ptr<Mission> SpawnAndLoad(std::shared_ptr<MissionFileInfo> missionInfo, const ConfigNode& missionNode);

    // ---- fields (public in C#) ----
    std::string expansionVersion;                           // :16
    static inline std::string lastCompatibleVersion = "1.0.0";  // :18
    std::string briefing, author, modsBriefing;             // :27-:31
    std::string packName;                                   // :33
    int order = 0;                                          // :35 (Awake: int.MaxValue)
    bool hardIcon = false;                                  // :37
    MissionDifficulty difficulty = MissionDifficulty::Intermediate;  // :39
    bool isBriefingSet = false;                             // :43 (internal)
    std::string flagURL;                                    // :49
    bool isScoreEnabled = true;                             // :57
    float maxScore = 0.0f;                                  // :59
    std::unique_ptr<MissionSituation> situation;            // :66
    std::unique_ptr<MissionFlow> flow;                      // :68
    DictionaryValueList<Guid, MENode*> nodes;         // :70
    std::vector<std::string> tags;                          // :72
    std::unique_ptr<MissionScore> globalScore;              // :74
    std::unique_ptr<MissionAwards> awards;                  // :76
    double startedUT = 0.0;                                 // :80
    float currentScore = 0.0f;                              // :82
    std::string exportName;                                 // :84
    std::string missionNameAtLastExport;                    // :86 (internal)
    uint64_t steamPublishedFileId = 0;                      // :88
    bool briefingNodeActive = false;                        // :90
    bool switchActiveVessel = false;                        // :98 (private)
    Vessel* vesselToSwitchActive = nullptr;                 // :100 (private)
    bool saveRevertOnSwitchActiveVessel = false;            // :102 (internal)
    MissionCameraModeOptions cameraLockMode = MissionCameraModeOptions::Flight;     // :104
    MissionCameraLockOptions cameraLockOptions = MissionCameraLockOptions::Unlock;  // :106
    Guid historyId;                                         // :108
    std::vector<MissionMappedVessel> mappedVessels;         // :110
    std::vector<MissionMappedVessel> mappedVesselKeys;      // :112 (private)
    std::vector<uint8_t> hashBytes;                         // :114 (private)
    std::optional<std::string> signature;                   // :116 (private)
    bool isSimple = false;                                  // :118 (internal)
    bool registeredGameEvents = false;                      // :120 (private)
    std::vector<uint32_t> missingVessels;                   // :122
    std::shared_ptr<std::vector<Editor::MissionValidationTestResult>> validationResults;  // :128 (internal; the validator's own list)
    bool hasBeenValidated = false;                          // :130 (internal)
    std::string gameObjectName;                             // gameObject.name (:350 GetGameObjectName)
    MEBannerEntry bannerMenu{MEBannerType::Menu};           // :51 (private)
    MEBannerEntry bannerSuccess{MEBannerType::Success};     // :53
    MEBannerEntry bannerFail{MEBannerType::Fail};           // :55
    std::shared_ptr<MissionFileInfo> missionInfo;           // :61 _missionInfo
    DictionaryValueList<std::string, std::shared_ptr<MissionCraft>> craftFileList;  // :61 (WP-54c; filled by RebuildCraftFileList)

    // ---- properties ----
    const Guid& id() const { return id_; }                  // :132
    const std::string& idName() const { return idName_; }   // :134 (internal)
    const std::string& title() const { return title_; }     // :136
    void SetTitle(const std::string& value);                // :140
    const std::string& ActiveNodeName() const { return activeNodeName_; }  // :148
    int Seed() const { return seed_; }                      // :150 (internal)
    MENode* activeNode() const { return activeNode_; }      // :246
    MENode* startNode() const { return startNode_; }        // :248
    bool isInitialized() const { return isInitialized_; }   // :250
    bool isStarted() const { return isStarted_; }           // :252
    bool isEnded() const { return isEnded_; }               // :254
    bool isSuccesful() const { return isSuccesful_; }       // :256
    std::vector<MENode*>& InactiveEventNodes() { return inactiveEventNodes_; }  // :258
    bool IsTutorialMission() const { return packName == "squad_MakingHistory_Tutorial"; }  // :260
    const std::vector<uint8_t>& HashBytes() const { return hashBytes; }
    const std::optional<std::string>& Signature() const { return signature; }
    std::shared_ptr<std::vector<Editor::MissionValidationTestResult>> ValidationResults() { return validationResults; }  // :264
    bool HasBeenValidated() const { return hasBeenValidated; }  // :266
    void SetIdForTests(const Guid& g) { id_ = g; }
    void SetActiveNodeForTests(MENode* n) { activeNode_ = n; }
    void SetStartedForTests(bool started) { isStarted_ = started; }

    // ---- methods ----
    static std::string GetGameObjectName(const MissionFileInfo& info);       // :347
    static std::string GetGameObjectName(const Mission& mission);            // :354
    void RegenerateMissionID();                             // :375 (internal)
    // ---- WP-54c: craft-file bookkeeping, public part ----
    void RebuildCraftFileList();                            // :1350
    MissionCraft* GetCraftBySituationsVesselID(uint32_t persistentId);  // :1510
    MissionCraft* GetMissionCraftByFileName(const std::string& craftFileName);  // :1535
    VesselSituation* GetMissionSituationByCraftFileName(const std::string& craftFileName);  // :1543
    MissionCraft* GetMissionCraftByName(const std::string& vesselName);  // :1562
    int GetSituationsIndexByVessel(uint32_t persistentId);  // :1584
    int GetSituationsIndexByPart(uint32_t persistentId);    // :1602
    int GetCraftFileIndexByPart(uint32_t persistentId);     // :1625
    void ProcessMappedVesselNodeMap(uint32_t mappedPersistentId = 0, uint32_t newPersistentId = 0);  // :1964 (internal)
    void RemapVesselId(uint32_t currentVslId);              // :2144 (internal)
    void InitMission();                                     // :401
    bool BlockPlayMission(bool showDialog = false);         // :577 (dialog skipped); returns the C# bool
    const std::vector<std::string>& BlockedMessages() const { return blockedMessages_; }
    void StartMission();                                    // :773
    void ResumeMission();                                   // :797
    void SetStartNode(MENode* newStart);                    // :829
    void UpdateMission();                                   // :835
    void SetNextObjectives(MENode* node);                   // :957 (internal)
    bool IsNextObjective(MENode* node);                     // :986
    void RemoveInactiveEvent(MENode* node);                 // :995 (internal)
    void UpdateOrphanNodeState(MENode* node, bool makeOrphan);  // :1003 (internal)
    std::string PrintObjectives(bool onlyPrintActivatedNodes, bool startWithActiveNode);  // :1027
    std::string PrintScoreObjectives(bool onlyPrintActivatedNodes, bool startWithActiveNode, bool onlyAwardedScores);  // :1032
    void Load(const ConfigNode& node);                      // :1688
    void Load(const ConfigNode& node, bool simple);         // :1693
    void Save(ConfigNode& node);                            // :2174
    uint32_t CurrentVesselID(MENode* node, uint32_t vesselID);  // :2258
    MENode* GetNodeById(const Guid& guid);                  // :2296
    void SetLockedCamera(MissionCameraModeOptions newCameraMode, MissionCameraLockOptions newCameraLock);  // :1665 (WP-55)
    Guid GetNodeGuidByVesselID(uint32_t persistentId);      // :2315
    Guid GetNodeGuidByVesselID(uint32_t persistentId, bool processMappedVessels);  // :2320
    bool PendingVesselLaunch(MENode* node);                 // :2344
    VesselSituation* GetVesselSituationByVesselID(uint32_t persistentId);  // :2349
    bool UpdateFromMappedVesselIDs(uint32_t vesselId, std::string& vesselName, VesselSituation*& vesselSituation);  // :1375 (internal)
    VesselSituation* GetVesselSituationByVesselID(uint32_t persistentId, bool processMappedVessels);  // :2354
    Asteroid* GetAsteroidByPersistentID(uint32_t persistentId);  // :2372
    Comet* GetCometByPersistentID(uint32_t PersistentId);   // :2392
    IActionCreateFlag* GetActionCreateFlagByPersistentID(uint32_t persistentId);  // :2412
    IActionCreateVessel* GetActionCreateVessel(MENode* node);  // :2432
    IActionCreateKerbal* GetActionCreateKerbal(MENode* node);  // :2452
    IActionCreateAsteroid* GetActionCreateAsteroid(MENode* node);  // :2472
    IActionCreateComet* GetActionCreateComet(MENode* node);  // :2492
    IActionCreateFlag* GetActionCreateFlag(MENode* node);   // :2512
    std::vector<VesselSituation*> GetAllVesselSituations();  // :2532
    DictionaryValueList<VesselSituation*, Guid> GetAllVesselSituationsGuid();  // :2558
    template <class T>
    std::vector<T*> GetAllActionModules() {                 // :2582
        std::vector<T*> list;
        for (MENode* n : nodes.GetListEnumerator()) {
            for (auto& am : n->actionModules)
                if (am->TypeName() == T::kTypeName)
                    if (T* t = dynamic_cast<T*>(am.get())) list.push_back(t);
        }
        return list;
    }
    template <class T>
    std::vector<T*> GetAllTestModules() {                   // :2608
        std::vector<T*> list;
        for (MENode* n : nodes.GetListEnumerator()) {
            for (auto& tg : n->testGroups)
                for (auto& tm : tg->testModules)
                    if (static_cast<TestModule*>(tm.get())->TypeName() == T::kTypeName)
                        if (T* t = dynamic_cast<T*>(tm.get())) list.push_back(t);
        }
        return list;
    }
    bool MissionHasLaunchSite(const std::string& name);     // :2646
    MEBannerEntry* GetBanner(MEBannerType bannerType);      // :2668
    void SetBanner(const MEBannerEntry& newBanner, MEBannerType bannerType);  // :2686
    // MENode ownership: MENode::Spawn registers the node here (the C# node is a child GameObject of the mission's)
    MENode* AdoptNode(std::unique_ptr<MENode> node);
    std::string BannersPath() const;                        // :201
    std::string ShipsPath() const;                          // :187
    bool MissionLoadedFromSFS() const;                      // :176
    std::string PersistentSaveName() const;                 // :164

private:
    friend class MENode;
    friend class Flow::MEFlowParser;
    void InitSeed();                                        // :342
    void OnEditorStarted();                                 // :1195
    bool craftFileExists(const std::string& craftFileName);  // GetMissionCraftByFileName(name) != null
    // ---- WP-54c: craft-file bookkeeping (Mission.cs) ----
    void ClearCraftFiles();                                 // :1363
    void ClearMappedVessels();                              // :1392
    void ProcessCraftFileFolder(const std::string& saveShipFolderPath);  // :1399
    void ProcessCraftRenames(const std::string& craftFileName, MissionCraft* craftFile);  // :1413
    MissionMappedVessel AddMappedPartVesselName(uint32_t partId, uint32_t vslPersistentId, const std::string& craftFileName, MissionCraft* craftFile, bool mappedItem, VesselSituation* vesselSituation);  // :1465
    void AddMappedCraft(const std::string& craftFileName, MissionCraft* craftFile);  // :1493
    uint32_t UpgradeMappedVesselId(uint32_t currentVslId, MENode* currentNode);  // :2154
    void RefreshMissionRequested();                         // `if (MissionsApp.Instance) MissionsApp.Instance.RefreshMissionRequested(this, false)`
    void OnPartVesselNamingChanged(Part* part);             // :1217
    void OnVesselRename(GameEvents::HostedFromToAction<Vessel*, std::string> hostedFromTo);  // :1244
    void OnPartDecoupleNewVesselComplete(Vessel* oldVessel, Vessel* newVessel);  // :1262
    void OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId);  // :1281
    void OnPartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId);  // :1297
    void OnVesselDocking(uint32_t oldId, uint32_t newId);   // :1313
    void OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel);  // :1329
    bool CheckHash();                                       // :361
    void OnGameSceneSwitchRequested(GameEvents::FromToAction<GameScenes, GameScenes> scenes);  // :381
    void RegisterGameEvents();                              // :389
    void SetStartNode();                                    // :533
    void SetActiveNode();                                   // :555
    void SetDockedNodes();                                  // :569
    void DoEndMissionActions();                             // :922
    void SwitchActiveNodes(MENode* newNode);                // :948
    std::string PrintObjectives(bool onlyPrintActivatedNodes, bool startWithActiveNode, bool onlyPrintScore, bool onlyAwardedScores);  // :1037
    void RecursivelyCreateObjectiveString(MENode* currentNode, std::string& objectiveString, std::string indent, bool onlyPrintActivatedNodes, std::vector<MENode*>& visitedNodesList, bool rejectOrphanNodes);  // :1087
    void RecursivelyCreateObjectiveScoreString(MENode* currentNode, std::string& objectiveString, bool onlyPrintActivatedNodes, bool onlyAwardedScores, std::vector<MENode*>& visitedNodesList, bool rejectOrphanNodes);  // :1116

    Guid id_;
    std::string idName_;
    std::string title_;
    std::string activeNodeName_;
    int seed_ = 0;
    Guid loadActiveNodeID_;
    std::vector<MENode*> orphanNodes_;
    std::vector<MENode*> nextObjectivesNodes_;
    bool nextObjectivesNodesSet_ = false;
    std::vector<MENode*> inactiveEventNodes_;
    bool inactiveEventNodesSet_ = false;
    std::vector<std::string> blockedMessages_;
    MENode* activeNode_ = nullptr;
    MENode* startNode_ = nullptr;
    bool isInitialized_ = false;
    bool isStarted_ = false;
    bool isEnded_ = false;
    bool isSuccesful_ = false;
    std::vector<std::unique_ptr<MENode>> ownedNodes_;
};

}  // namespace ksp::Expansions::Missions

template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::MissionPartFilter::Choices> {
    using E = ksp::Expansions::Missions::MissionPartFilter::Choices;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::availableParts, "availableParts", "#autoLOC_8100288"},
                                                {E::unavailableParts, "unavailableParts", "#autoLOC_8100289"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::MissionSituation::StartTypeEnum> {
    using E = ksp::Expansions::Missions::MissionSituation::StartTypeEnum;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::VAB, "VAB"}, {E::SPH, "SPH"}, {E::VesselList, "VesselList"}, {E::SpaceCenter, "SpaceCenter"}, {E::SaveFile, "SaveFile"}};
};
