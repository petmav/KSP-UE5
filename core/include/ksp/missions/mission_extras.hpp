// WP-54c: Assembly-CSharp/Expansions/Missions/{MissionScoreInfo,MissionLocalizer,MissionImporting}.cs and Editor/{MissionEditorHistory,IMEHistoryTarget,HistoryType}.cs.
//
// Seams (MissionExtrasEnvironment): the file system, KSPCompression (zip), the popup dialogs, the clock and ScreenMessages, so the logic of the importer and the
// score file is gated on an in-memory world. Conventions: see node_library.hpp.
#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/missions/mission_score.hpp"
#include "ksp/util/collections.hpp"

namespace ksp::Expansions::Missions {

struct MissionExtrasEnvironment {
    // KSPUtil.SystemDateTime.DateTimeNow().ToFileTimeUtc() (MissionScoreInfo.AddScore). Unset: the system clock.
    static inline std::function<int64_t()> dateTimeNowFileTimeUtc;
    // KSPUtil.SystemDateTime.DateTimeNow().ToString("yyyy_MM_dd_HH_mm_ss") (KSPUtil.GenerateFilePathWithDate). Unset: the system clock (local time).
    static inline std::function<std::string()> dateTimeStamp;
    // File system. Unset: std::filesystem. getFiles/getDirectories return full paths in the file system's (ordinal) order.
    static inline std::function<bool(const std::string&)> directoryExists;
    static inline std::function<void(const std::string&)> createDirectory;
    static inline std::function<void(const std::string&)> deleteDirectory;  // Directory.Delete(path, recursive: true)
    static inline std::function<std::vector<std::string>(const std::string& dir, const std::string& extension)> getFiles;  // Directory.GetFiles(dir, "*.<ext>")
    static inline std::function<std::vector<std::string>(const std::string& dir)> getDirectories;                         // Directory.GetDirectories(dir)
    static inline std::function<bool(const std::string&)> fileExists;
    static inline std::function<bool(const std::string& src, const std::string& dst, bool overwrite)> moveFile;           // KSPUtil.MoveFile
    static inline std::function<ConfigNode::NodePtr(const std::string&)> loadConfigNode;                                   // ConfigNode.Load(path)
    static inline std::function<void(const ConfigNode&, const std::string&)> saveConfigNode;                                // ConfigNode.Save(path)
    // KSPCompression (zip): FilesAtRoot / GetTopLevelDirectories / DecompressFile. They throw like the C# when the archive cannot be read.
    static inline std::function<bool(const std::string& zip)> zipFilesAtRoot;
    static inline std::function<std::vector<std::string>(const std::string& zip)> zipTopLevelDirectories;
    static inline std::function<void(const std::string& zip, const std::string& destination)> zipDecompress;
    // MissionsUtils.ParseMissionPacksCFG(path)
    static inline std::function<void(const std::string& path)> parseMissionPacksCFG;
    // PopupDialog.SpawnPopupDialog(new MultiOptionDialog(name, message, title, skin, width, buttons...)): the two-button confirmation popups
    struct PopupDialog {
        std::string name, message, title;
        float width = 0.0f;
        struct Button {
            std::string label;
            std::function<void()> callback;
        };
        std::vector<Button> buttons;
    };
    static inline std::function<void(PopupDialog&&)> spawnPopupDialog;
    static inline std::deque<PopupDialog> popups;  // unset: recorded
    static void Reset();
};

namespace Editor {

// HistoryType.cs:3
enum class HistoryType { Redo = 0, Undo = 1 };

// IMEHistoryTarget.cs:3
class IMEHistoryTarget {
public:
    virtual ~IMEHistoryTarget() = default;
    virtual ConfigNode::NodePtr GetState() = 0;  // :5
};

// MissionEditorHistory.cs:7 (a MonoBehaviour singleton; the undo / redo stacks of the mission editor)
class MissionEditorHistory {
public:
    using ActionCallback = std::function<void(ConfigNode& data, HistoryType type)>;  // :9
    // `MonoBehaviour[] destroyTargets` are opaque handles of the editor objects (Object.Destroy(target.gameObject))
    using DestroyHandle = int;

    // Seams
    struct Environment {
        static inline std::function<int()> frameCount;                          // Time.frameCount. Unset: 0
        static inline std::function<void(IMEHistoryTarget*)> selectAffectedObject;  // SelectAffectedObject (MEGUINode / MEGUIParameter selection). Unset: recorded
        static inline std::vector<IMEHistoryTarget*> selected;
        static inline std::function<void(DestroyHandle)> destroyObject;          // UnityEngine.Object.Destroy(handle.gameObject). Unset: recorded
        static inline std::function<void(IMEHistoryTarget*)> destroyTargetObject;  // Object.Destroy(((MonoBehaviour)target).gameObject). Unset: recorded
        static inline std::vector<DestroyHandle> destroyed;
        static inline std::vector<IMEHistoryTarget*> destroyedTargets;
        static void Reset();
    };

    MissionEditorHistory();                      // :133 Awake (a second instance is destroyed: `destroyedAsDuplicate()`)
    ~MissionEditorHistory();
    MissionEditorHistory(const MissionEditorHistory&) = delete;
    MissionEditorHistory& operator=(const MissionEditorHistory&) = delete;
    bool destroyedAsDuplicate() const { return duplicate_; }

    static MissionEditorHistory* Instance() { return instance_; }  // :124
    bool ActionInProgress = false;                                  // :126
    static bool HasHistory();                                       // :128
    static Guid HistoryStateId();                                   // :130 (getter)
    static void SetHistoryStateId(const Guid& value);               // :130 (setter)
    static void PushUndoAction(IMEHistoryTarget* target, ActionCallback callback);                                                         // :146
    static void PushUndoAction(IMEHistoryTarget* target, ActionCallback callback, bool destroyOnClear, std::vector<DestroyHandle> destroyTargets = {});  // :151
    static void Undo();                                             // :162
    static void Redo();                                             // :189
    static void Clear();                                            // :232

    // The stacks as plain data for the gate: groups (oldest first) of [target, data dump, stateId, destroyOnClear, destroyTargets]
    struct ActionView {
        IMEHistoryTarget* target;
        ConfigNode::NodePtr data;
        Guid stateId;
        bool destroyOnClear;
        std::vector<DestroyHandle> destroyTargets;
    };
    std::vector<std::vector<ActionView>> UndoView() const;
    std::vector<std::vector<ActionView>> RedoView() const;

protected:
    struct HistoryAction;
    class ActionStack;
    void ClearRedoActions();                                        // :240

private:
    static void SelectAffectedObject(HistoryAction& action);        // :213
    std::unique_ptr<ActionStack> UndoActions, RedoActions;
    bool duplicate_ = false;
    static inline MissionEditorHistory* instance_ = nullptr;
};

}  // namespace Editor

// MissionScoreInfo.cs:6
class MissionScoreInfo {
public:
    using ScoreList = std::vector<std::shared_ptr<ScoreInfo>>;
    MissionScoreInfo();                                              // :14
    void AddScore(Mission& mission);                                 // :19
    void DeleteScore(const Guid& scoreId);                           // :35
    void DeleteMissionScores(Mission* mission);                      // :57
    std::shared_ptr<ScoreList> GetMissionScores(Mission* mission);   // :65 (the live list; a new empty list for a mission without scores)
    std::shared_ptr<ScoreInfo> GetMissionScore(Mission* mission, int index);  // :74
    std::shared_ptr<ScoreInfo> GetMissionScore(const Guid& missionId, int index);  // :83
    std::vector<std::string> GetMissionAwards(Mission* mission);     // :107
    void SaveScores();                                               // :124
    static std::unique_ptr<MissionScoreInfo> LoadScores();           // :133
    void Load(const ConfigNode& node);                               // :149
    void Save(ConfigNode& node) const;                               // :159
    static constexpr const char* saveFileName = "MissionScoreInfo.cfg";  // :8

private:
    static int CompareScoreInfo(const ScoreInfo& a, const ScoreInfo& b);  // :94
    ListDictionary<Guid, std::shared_ptr<ScoreInfo>> scoreInfoDictionary;  // :10
};

// MissionLocalizer.cs:7
class MissionLocalizer {
public:
    static std::vector<ConfigNode::NodePtr> LoadMissionLocalizationFiles();  // :9
};

// MissionImporting.cs:10
class MissionImporting {
public:
    static void ImportNewZips();  // :12

private:
    static bool ImportZip(const std::string& filePath, const std::string& moveDestination, int fileNumber);  // :33
    static void ImportZipConfirmed(const std::string& filePath, const std::vector<std::string>& directories, const std::string& moveDestination);  // :92
    static void MoveImportedFile(const std::string& filePath, const std::string& destinationPath);  // :114
};

}  // namespace ksp::Expansions::Missions
