// GamePersistence (Assembly-CSharp/GamePersistence.cs) as pure logic over the file system, plus the quicksave naming of
// QuickSaveLoad.cs and the [KSPScenario] scenario-type table (KSPScenarioType.cs / ScenarioCreationOptions.cs) that
// CreateNewGame / UpdateScenarioModules use.
//
// Paths are KSPUtil.ApplicationRootPath() + "saves/" + folder + "/" + name + ".sfs" exactly as KSP builds them (the
// Backup folder is text + "/Backup/" with a doubled slash). The clock (KSPUtil.SystemDateTime.DateTimeNow) is a hook.
// Not ported: LoadGameDialog.PlayerProfileInfo.SaveToMetaFile (the .loadmeta written after a save: UI metadata, WP-UI),
// the popup dialogs (logged instead), MissionSystem objects.
#pragma once

#include <cstdint>
#include <ctime>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_enums.hpp"

namespace ksp {

// ScenarioCreationOptions.cs ([Flags])
enum class ScenarioCreationOptions : int {
    None = 0,
    AddToNewSandboxGames = 2,
    AddToExistingSandboxGames = 4,
    AddToNewScienceSandboxGames = 8,
    AddToExistingScienceSandboxGames = 16,
    AddToNewCareerGames = 32,
    AddToExistingCareerGames = 64,
    RemoveFromSandboxGames = 128,
    RemoveFromScienceSandboxGames = 256,
    RemoveFromCareerGames = 512,
    AddToNewMissionGames = 1024,
    AddToExistingMissionGames = 2048,
    AddToAllMissionGames = 3072,
    RemoveFromMissionGames = 4096,
    AddToNewGames = 42,
    AddToExistingGames = 84,
    AddToAllGames = 126,
    RemoveFromAllGames = 4992
};

// KSPScenarioType.cs: a ScenarioModule type with its [KSPScenario] attribute (KSPScenario.cs)
struct KSPScenarioType {
    std::string moduleTypeName;
    int createOptions = 0;
    std::vector<GameScenes> targetScenes;
    bool HasCreateOption(ScenarioCreationOptions option) const { return (createOptions & static_cast<int>(option)) != 0; }
    // KSPScenarioType.cs:18 GetAllScenarioTypesInAssemblies for the stock assembly (TypeDef order of the shipped
    // Assembly-CSharp.dll, src/save/scenario_types_data.inc; DeployedScience only with Breaking Ground)
    static std::vector<KSPScenarioType> GetAllScenarioTypesInAssemblies();
};

class GamePersistence {
public:
    // KSPUtil.SystemDateTime.DateTimeNow (local time); a hook so tests and hosts control backup names
    static inline std::function<std::tm()> DateTimeNow;

    // Path.GetInvalidFileNameChars() (Windows set: '"', '<', '>', '|', '\0', 1..31, ':', '*', '?', '\\', '/') -> '_'
    static std::string ReplaceInvalidFileNameChars(std::string saveFileName);

    // GamePersistence.cs:24 (returns the file name used, "" when skipped / aborted)
    static std::string SaveGame(Game& game, std::string saveFileName, const std::string& saveFolder, SaveMode saveMode);
    // GamePersistence.cs:132
    static std::string SaveGame(GameBackup& game, std::string saveFileName, const std::string& saveFolder,
                                SaveMode saveMode);
    // GamePersistence.cs:208 (nullptr when the file does not exist)
    static ConfigNode::NodePtr LoadSFSFile(const std::string& filename, const std::string& saveFolder);
    // GamePersistence.cs:219 (clears the persistentId registry; nullptr when incompatible and nullIfIncompatible)
    static std::unique_ptr<Game> LoadGameCfg(const ConfigNode::NodePtr& node, const std::string& saveName,
                                             bool nullIfIncompatible, bool suppressIncompatibleMessage);
    // GamePersistence.cs:237
    static std::unique_ptr<Game> LoadGame(const std::string& filename, const std::string& saveFolder,
                                          bool nullIfIncompatible, bool suppressIncompatibleMessage);
    // GamePersistence.cs:248
    static bool UpdateScenarioModules(Game& game, const std::vector<KSPScenarioType>& scenarioTypes);
    // GamePersistence.cs:304 (creates saves/<name>/Ships/{VAB,SPH}, the stock scenarios for the mode, saves
    // persistent.sfs; returns the game and sets saveFolder like HighLogic.SaveFolder)
    static std::unique_ptr<Game> CreateNewGame(const std::string& name, Game_Modes mode,
                                               std::shared_ptr<GameParameters> parameters, const std::string& flagURL,
                                               GameScenes startScene, EditorFacility editorFacility,
                                               const std::vector<KSPScenarioType>& scenarioTypes, std::string& saveFolder);

    // ---- QuickSaveLoad.cs naming ----
    // QuickSaveLoad.cs:346: Localizer "#autoLOC_6002266" ("quicksave"), then "quicksave #1", "#2", ... while the
    // file exists in saves/<folder>/
    static std::string NextQuicksaveFilename(const std::string& saveFolder);
    // QuickSaveLoad.cs:467 EscapeString (CR/LF -> ' ', '"' -> '\'', KSPUtil.SanitizeFilename)
    static std::string EscapeString(const std::string& str);
    // QuickSaveLoad.cs:476 CheckFilename
    enum class FilenameCheckResults : int { Empty, AlreadyExists, InvalidCharacters, Good };
    static FilenameCheckResults CheckFilename(const std::string& filename);
    // the backup file name GamePersistence writes: name + " (yyyy_MM_dd_HH_mm_ss).sfs"
    static std::string BackupFileName(const std::string& saveFileName, const std::tm& now);
};

}  // namespace ksp
