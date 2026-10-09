// GamePersistence, quicksave naming, stock scenario types; see include/ksp/save/game_persistence.hpp.
#include "ksp/save/game_persistence.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <system_error>

#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

namespace fs = std::filesystem;
using Localizer = KSP::Localization::Localizer;

namespace {
fs::path P(const std::string& utf8) { return fs::path(dotnet::Utf8ToWide(utf8)); }
bool FileExists(const std::string& p) {
    std::error_code ec;
    return fs::is_regular_file(P(p), ec);
}
bool DirExists(const std::string& p) {
    std::error_code ec;
    return fs::is_directory(P(p), ec);
}
std::string Root() { return KSPUtil::ApplicationRootPath(); }
std::tm Now() {
    if (GamePersistence::DateTimeNow) return GamePersistence::DateTimeNow();
    const std::time_t t = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    return tm;
}
std::string Pad(int v, int width) {
    std::string s = dotnet::FormatInt64(v);
    while (static_cast<int>(s.size()) < width) s = "0" + s;
    return s;
}
// GamePersistence.cs:43-110 / 140-203 (the shared APPEND / ABORT / BACKUP handling); false = abort
bool PrepareSave(std::string& saveFileName, const std::string& saveFolder, SaveMode saveMode) {
    saveFileName = GamePersistence::ReplaceInvalidFileNameChars(saveFileName);
    const std::string dir = Root() + "saves/" + saveFolder + "/";
    if (!DirExists(dir)) fs::create_directories(P(dir));
    switch (saveMode) {
    case SaveMode::APPEND: {
        int num2 = 0;
        std::string text4;
        do {
            text4 = saveFileName + Pad(num2++, 3);  // num2++.ToString("000")
        } while (FileExists(dir + text4 + ".sfs"));
        saveFileName = text4;
        break;
    }
    case SaveMode::ABORT:
        if (FileExists(dir + saveFileName + ".sfs")) {
            log::Warning("Save Aborted! File already exists!");
            return false;
        }
        break;
    case SaveMode::BACKUP: {
        const std::string text = dir;
        const std::string text2 = text + "/Backup/";
        const std::string text3 = text + saveFileName + ".sfs";
        const std::string destFileName = text2 + GamePersistence::BackupFileName(saveFileName, Now());
        const int saveBackups = SaveEnvironment::Current().SAVE_BACKUPS;
        if (saveBackups <= 0) {
            if (DirExists(text2)) fs::remove_all(P(text2));
            break;
        }
        if (!FileExists(text3)) break;
        if (!DirExists(text2)) {
            fs::create_directories(P(text2));
            fs::copy_file(P(text3), P(destFileName), fs::copy_options::overwrite_existing);
            break;
        }
        std::vector<fs::path> list;
        for (const auto& e : fs::directory_iterator(P(text2)))
            if (e.is_regular_file() && e.path().extension() == ".sfs") list.push_back(e.path());
        // saveCompareToDate: a.LastWriteTime.CompareTo(b.LastWriteTime) (List.Sort: unstable introsort)
        dotnet::ListSort(list, [](const fs::path& a, const fs::path& b) {
            std::error_code ea, eb;
            const auto ta = fs::last_write_time(a, ea), tb = fs::last_write_time(b, eb);
            if (ea || eb) return 0;
            return ta < tb ? -1 : (tb < ta ? 1 : 0);
        });
        if (static_cast<int>(list.size()) < saveBackups) {
            fs::copy_file(P(text3), P(destFileName), fs::copy_options::overwrite_existing);
            break;
        }
        fs::copy_file(P(text3), P(destFileName), fs::copy_options::overwrite_existing);
        const int num = static_cast<int>(list.size()) + 1 - saveBackups;
        for (int j = 0; j < num; ++j) fs::remove(list[static_cast<size_t>(j)]);
        break;
    }
    default: break;
    }
    return true;
}
}  // namespace

// Path.GetInvalidFileNameChars (Windows)
std::string GamePersistence::ReplaceInvalidFileNameChars(std::string saveFileName) {
    for (char& c : saveFileName) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u < 32 || c == '"' || c == '<' || c == '>' || c == '|' || c == ':' || c == '*' || c == '?' || c == '\\' ||
            c == '/')
            c = '_';
    }
    return saveFileName;
}

std::string GamePersistence::BackupFileName(const std::string& saveFileName, const std::tm& now) {
    return saveFileName + " (" + Pad(now.tm_year + 1900, 4) + "_" + Pad(now.tm_mon + 1, 2) + "_" + Pad(now.tm_mday, 2) +
           "_" + Pad(now.tm_hour, 2) + "_" + Pad(now.tm_min, 2) + "_" + Pad(now.tm_sec, 2) + ").sfs";
}

// GamePersistence.cs:24
std::string GamePersistence::SaveGame(Game& game, std::string saveFileName, const std::string& saveFolder,
                                      SaveMode saveMode) {
    if (saveFileName == "persistent" && game.Parameters && !game.Parameters->Flight.CanAutoSave) {
        log::Warning("[GamePersistence]: Saving to persistent.sfs is disabled in FLIGHT scenario options (disabled autosave)");
        return "";
    }
    if (!PrepareSave(saveFileName, saveFolder, saveMode)) return "";
    ConfigNode configNode;
    game.Save(configNode);
    configNode.Save(Root() + "saves/" + saveFolder + "/" + saveFileName + ".sfs");
    log::Info("Game State Saved to saves/" + saveFolder + "/" + saveFileName);
    return saveFileName;
}

// GamePersistence.cs:132
std::string GamePersistence::SaveGame(GameBackup& game, std::string saveFileName, const std::string& saveFolder,
                                      SaveMode saveMode) {
    if (!PrepareSave(saveFileName, saveFolder, saveMode)) return "";
    game.Config->Save(Root() + "saves/" + saveFolder + "/" + saveFileName + ".sfs");
    log::Info("Game State Saved as " + saveFileName);
    return saveFileName;
}

// GamePersistence.cs:208
ConfigNode::NodePtr GamePersistence::LoadSFSFile(const std::string& filename, const std::string& saveFolder) {
    const std::string text = Root() + "saves/" + saveFolder + "/" + filename + ".sfs";
    if (!FileExists(text)) {
        log::Info("No save file found for path: " + text);
        return nullptr;
    }
    return ConfigNode::Load(text);
}

// GamePersistence.cs:219
std::unique_ptr<Game> GamePersistence::LoadGameCfg(const ConfigNode::NodePtr& node, const std::string& saveName,
                                                   bool nullIfIncompatible, bool suppressIncompatibleMessage) {
    SaveEnvironment::Current().persistentIds.ClearpersistentIdDictionaries();
    auto game = std::make_unique<Game>(node.get());
    if (!game->compatible && nullIfIncompatible) {
        if (!suppressIncompatibleMessage) log::Error("Load Failed: " + saveName + " is incompatible");
        return nullptr;
    }
    return game;
}

// GamePersistence.cs:237
std::unique_ptr<Game> GamePersistence::LoadGame(const std::string& filename, const std::string& saveFolder,
                                                bool nullIfIncompatible, bool suppressIncompatibleMessage) {
    ConfigNode::NodePtr configNode = LoadSFSFile(filename, saveFolder);
    SaveEnvironment::Current().persistentIds.ClearpersistentIdDictionaries();
    if (configNode) return LoadGameCfg(configNode, filename, nullIfIncompatible, suppressIncompatibleMessage);
    return nullptr;
}

// GamePersistence.cs:248
bool GamePersistence::UpdateScenarioModules(Game& game, const std::vector<KSPScenarioType>& scenarioTypes) {
    using O = ScenarioCreationOptions;
    bool result = false;
    for (const KSPScenarioType& t : scenarioTypes) {
        O add = O::None, remove = O::None;
        switch (game.Mode) {
        case Game_Modes::SANDBOX: add = O::AddToExistingSandboxGames; remove = O::RemoveFromSandboxGames; break;
        case Game_Modes::SCIENCE_SANDBOX:
            add = O::AddToExistingScienceSandboxGames;
            remove = O::RemoveFromScienceSandboxGames;
            break;
        case Game_Modes::CAREER: add = O::AddToExistingCareerGames; remove = O::RemoveFromCareerGames; break;
        case Game_Modes::MISSION:
        case Game_Modes::MISSION_BUILDER: add = O::AddToExistingMissionGames; remove = O::RemoveFromMissionGames; break;
        default: continue;
        }
        if (t.HasCreateOption(add) && game.AddProtoScenarioModule(t.moduleTypeName, t.targetScenes) != nullptr) result = true;
        if (t.HasCreateOption(remove) && game.RemoveProtoScenarioModule(t.moduleTypeName)) result = true;
    }
    return result;
}

// GamePersistence.cs:304 (MissionsUtils.SavesPath for mission modes is "Missions/" in KSP: not handled -- mission
// games are the MakingHistory package)
std::unique_ptr<Game> GamePersistence::CreateNewGame(const std::string& name, Game_Modes mode,
                                                     std::shared_ptr<GameParameters> parameters,
                                                     const std::string& flagURL, GameScenes startScene,
                                                     EditorFacility editorFacility,
                                                     const std::vector<KSPScenarioType>& scenarioTypes,
                                                     std::string& saveFolder) {
    SaveEnvironment::Current().persistentIds.ClearpersistentIdDictionaries();
    const std::string localized = Localizer::Instance() ? Localizer::Format(name) : name;
    const std::string text2 = Root() + "saves/" + localized;
    fs::create_directories(P(text2));
    fs::create_directories(P(text2 + "/Ships"));
    fs::create_directories(P(text2 + "/Ships/VAB"));
    fs::create_directories(P(text2 + "/Ships/SPH"));
    saveFolder = localized;
    auto game = std::make_unique<Game>();
    game->Mode = mode;
    game->Parameters = std::move(parameters);
    game->Title = name + " (" + dotnet::EnumToString(mode) + ")";
    game->Description = "No description available.";
    game->startScene = startScene;
    game->editorFacility = editorFacility;
    game->flagURL = flagURL;
    game->CrewRoster = KerbalRoster::GenerateInitialCrewRoster(mode);
    if (Planetarium::fetch != nullptr) Planetarium::SetUniversalTime(0.0);
    using O = ScenarioCreationOptions;
    for (const KSPScenarioType& t : scenarioTypes) {
        switch (mode) {
        case Game_Modes::CAREER:
            if (t.HasCreateOption(O::AddToNewCareerGames)) game->AddProtoScenarioModule(t.moduleTypeName, t.targetScenes);
            break;
        case Game_Modes::SCIENCE_SANDBOX:
            if (t.HasCreateOption(O::AddToNewScienceSandboxGames))
                game->AddProtoScenarioModule(t.moduleTypeName, t.targetScenes);
            break;
        case Game_Modes::SANDBOX:
            if (t.HasCreateOption(O::AddToNewSandboxGames)) game->AddProtoScenarioModule(t.moduleTypeName, t.targetScenes);
            break;
        case Game_Modes::MISSION:
        case Game_Modes::MISSION_BUILDER:
            if (t.HasCreateOption(O::AddToNewMissionGames) &&
                (t.moduleTypeName != "Funding" ||
                 game->Parameters->CustomParams<Expansions::Missions::MissionParamsGeneral>().enableFunding))
                game->AddProtoScenarioModule(t.moduleTypeName, t.targetScenes);
            break;
        default: break;
        }
    }
    SaveGame(*game, "persistent", saveFolder, SaveMode::OVERWRITE);
    return game;
}

// QuickSaveLoad.cs:346
std::string GamePersistence::NextQuicksaveFilename(const std::string& saveFolder) {
    auto base = [] {
        if (Localizer::Instance() != nullptr) {
            std::string v;
            if (Localizer::TryGetStringByTag("#autoLOC_6002266", v)) return Localizer::Format("#autoLOC_6002266");
        }
        return std::string("quicksave");
    };
    std::string quicksaveFilename = base();
    int num = 0;
    while (FileExists(Root() + "saves/" + saveFolder + "/" + quicksaveFilename + ".sfs"))
        quicksaveFilename = base() + " #" + dotnet::FormatInt64(++num);
    return quicksaveFilename;
}

// QuickSaveLoad.cs:467
std::string GamePersistence::EscapeString(const std::string& str) {
    std::string s = dotnet::Replace(str, '\r', ' ');
    s = dotnet::Replace(s, '\n', ' ');
    s = dotnet::Replace(s, '"', '\'');
    return KSPUtil::SanitizeFilename(s);
}

// QuickSaveLoad.cs:476
GamePersistence::FilenameCheckResults GamePersistence::CheckFilename(const std::string& filename) {
    if (filename.empty()) return FilenameCheckResults::Empty;
    if (KSPUtil::SanitizeFilename(filename) != filename) return FilenameCheckResults::InvalidCharacters;
    return FilenameCheckResults::Good;
}

// KSPScenarioType.cs:18 (stock assembly)
std::vector<KSPScenarioType> KSPScenarioType::GetAllScenarioTypesInAssemblies() {
    struct Row {
        const char* name;
        int options;
        std::vector<int> scenes;
    };
    static const std::vector<Row> rows = {
#define KSP_SCENARIO_TYPE(name, options, ...) Row{name, options, std::vector<int> __VA_ARGS__},
#include "scenario_types_data.inc"
#undef KSP_SCENARIO_TYPE
    };
    std::vector<KSPScenarioType> list;
    for (const Row& r : rows) {
        if (std::string_view(r.name) == "DeployedScience" && !SaveEnvironment::Current().IsExpansionInstalled("Serenity"))
            continue;
        KSPScenarioType t;
        t.moduleTypeName = r.name;
        t.createOptions = r.options;
        for (int s : r.scenes) t.targetScenes.push_back(static_cast<GameScenes>(s));
        list.push_back(std::move(t));
    }
    return list;
}

}  // namespace ksp
