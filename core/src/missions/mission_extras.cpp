// WP-54c: Assembly-CSharp/Expansions/Missions/{MissionScoreInfo,MissionLocalizer,MissionImporting}.cs and Editor/MissionEditorHistory.cs.
// See mission_extras.hpp. Cite placeholders `File.cs:@Name[#k]` are resolved by tools/wp55/check_cites.py --resolve.
#include "ksp/missions/mission_extras.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission_awards.hpp"
#include "ksp/missions/mission_actions.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp::Expansions::Missions {

using KSP::Localization::Localizer;
namespace fs = std::filesystem;

namespace {

using Env = MissionExtrasEnvironment;

std::string Tag(const char* t) { return Localizer::Format(t); }
std::string Fmt(const char* tag, std::initializer_list<std::string> args) { return Localizer::Format(tag, std::vector<std::string>(args)); }

// ---- the world the importer / score file / localizer read (seams, else std::filesystem) -----------------------------------------------
bool DirectoryExists(const std::string& p) {
    if (Env::directoryExists) return Env::directoryExists(p);
    std::error_code ec;
    return fs::is_directory(p, ec);
}
void CreateDirectory(const std::string& p) {
    if (Env::createDirectory) {
        Env::createDirectory(p);
        return;
    }
    std::error_code ec;
    fs::create_directories(p, ec);
}
void DeleteDirectory(const std::string& p) {
    if (Env::deleteDirectory) {
        Env::deleteDirectory(p);
        return;
    }
    std::error_code ec;
    fs::remove_all(p, ec);
}
std::vector<std::string> GetFiles(const std::string& dir, const std::string& extension) {
    if (Env::getFiles) return Env::getFiles(dir, extension);
    std::vector<std::string> out;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(dir, ec))
        if (e.is_regular_file(ec) && e.path().extension() == extension) out.push_back(dir + e.path().filename().string());
    std::sort(out.begin(), out.end());
    return out;
}
std::vector<std::string> GetDirectories(const std::string& dir) {
    if (Env::getDirectories) return Env::getDirectories(dir);
    std::vector<std::string> out;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(dir, ec))
        if (e.is_directory(ec)) out.push_back(dir + e.path().filename().string());
    std::sort(out.begin(), out.end());
    return out;
}
ConfigNode::NodePtr LoadConfig(const std::string& p) {
    if (Env::loadConfigNode) return Env::loadConfigNode(p);
    if (MissionDataEnvironment::loadConfigNode) return MissionDataEnvironment::loadConfigNode(p);
    return ConfigNode::Load(p);
}
void SaveConfig(const ConfigNode& n, const std::string& p) {
    if (Env::saveConfigNode) {
        Env::saveConfigNode(n, p);
        return;
    }
    if (MissionDataEnvironment::saveConfigNode) {
        MissionDataEnvironment::saveConfigNode(n, p);
        return;
    }
    n.Save(p);
}
bool FileExists(const std::string& p) {
    if (Env::fileExists) return Env::fileExists(p);
    std::error_code ec;
    return fs::exists(p, ec);
}
bool MoveFile(const std::string& src, const std::string& dst, bool overwrite) {
    if (Env::moveFile) return Env::moveFile(src, dst, overwrite);
    return KSPUtil::MoveFile(src, dst, overwrite);
}
// KSPUtil.SystemDateTime.DateTimeNow().ToFileTimeUtc()
int64_t NowFileTimeUtc() {
    if (Env::dateTimeNowFileTimeUtc) return Env::dateTimeNowFileTimeUtc();
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const int64_t ticks100ns = std::chrono::duration_cast<std::chrono::duration<int64_t, std::ratio<1, 10000000>>>(now).count();
    return ticks100ns + 116444736000000000LL;
}
std::string DateStamp() {
    if (Env::dateTimeStamp) return Env::dateTimeStamp();
    const std::time_t t = std::time(nullptr);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tmv, "%Y_%m_%d_%H_%M_%S");
    return ss.str();
}
// KSPUtil.GenerateFilePathWithDate (KSPUtil.cs:1406)
std::string GenerateFilePathWithDate(const std::string& filePath) {
    const size_t dot = filePath.rfind('.');
    if (dot == std::string::npos) throw std::out_of_range("startIndex");  // string.Insert(-1, ...)
    return filePath.substr(0, dot) + " (" + DateStamp() + ")" + filePath.substr(dot);
}
void Message(const std::string& text, float duration, ScreenMessageStyle style = ScreenMessageStyle::UPPER_CENTER) {
    NodeLibraryEnvironment::ScreenMessage m;
    m.text = text;
    m.duration = duration;
    m.style = style;
    m.persist = false;
    if (NodeLibraryEnvironment::postScreenMessage) NodeLibraryEnvironment::postScreenMessage(m);
    else NodeLibraryEnvironment::screenMessages.push_back(m);
}
void SpawnPopup(Env::PopupDialog&& d) {
    if (Env::spawnPopupDialog) Env::spawnPopupDialog(std::move(d));
    else Env::popups.push_back(std::move(d));
}

}  // namespace

void MissionExtrasEnvironment::Reset() {
    dateTimeNowFileTimeUtc = nullptr;
    dateTimeStamp = nullptr;
    directoryExists = nullptr;
    createDirectory = nullptr;
    deleteDirectory = nullptr;
    getFiles = nullptr;
    getDirectories = nullptr;
    fileExists = nullptr;
    moveFile = nullptr;
    loadConfigNode = nullptr;
    saveConfigNode = nullptr;
    zipFilesAtRoot = nullptr;
    zipTopLevelDirectories = nullptr;
    zipDecompress = nullptr;
    parseMissionPacksCFG = nullptr;
    spawnPopupDialog = nullptr;
    popups.clear();
    Editor::MissionEditorHistory::Environment::Reset();
}

// ---- MissionScoreInfo ---------------------------------------------------------------------------------------------------------------

// MissionScoreInfo.cs:12
MissionScoreInfo::MissionScoreInfo() = default;

// MissionScoreInfo.cs:17
void MissionScoreInfo::AddScore(Mission& mission) {
    if (mission.isScoreEnabled) {
        auto info = std::make_shared<ScoreInfo>();
        info->missionId = mission.id();
        info->score = mission.currentScore;
        info->timeTaken = MissionsEnvironment::GameUniversalTime();
        info->awardIds = mission.awards->awardedAwards;  // the C# stores the list itself (aliasing not reproduced)
        info->completedAt = NowFileTimeUtc();
        info->resultsText = mission.PrintScoreObjectives(true, false, true);
        scoreInfoDictionary.Add(mission.id(), info);
        auto list = scoreInfoDictionary[mission.id()];
        std::stable_sort(list->begin(), list->end(), [](const std::shared_ptr<ScoreInfo>& a, const std::shared_ptr<ScoreInfo>& b) { return CompareScoreInfo(*a, *b) < 0; });
    }
}

// MissionScoreInfo.cs:34
void MissionScoreInfo::DeleteScore(const Guid& scoreId) {
    const auto& entries = scoreInfoDictionary.GetEnumerator();
    for (const auto& kv : entries) {
        for (size_t i = 0, count = kv.second->size(); i < count; ++i) {
            std::shared_ptr<ScoreInfo> scoreInfo = (*kv.second)[i];
            if (scoreInfo->id == scoreId) {
                const Guid key = kv.first;
                scoreInfoDictionary.Remove(key, scoreInfo);
                return;
            }
        }
    }
}

// MissionScoreInfo.cs:60
void MissionScoreInfo::DeleteMissionScores(Mission* mission) {
    if (mission != nullptr) scoreInfoDictionary.Remove(mission->id());
}

// MissionScoreInfo.cs:68
std::shared_ptr<MissionScoreInfo::ScoreList> MissionScoreInfo::GetMissionScores(Mission* mission) {
    if (mission == nullptr) return std::make_shared<ScoreList>();
    return scoreInfoDictionary[mission->id()];
}

// MissionScoreInfo.cs:77
std::shared_ptr<ScoreInfo> MissionScoreInfo::GetMissionScore(Mission* mission, int index) {
    if (mission == nullptr) return nullptr;
    return GetMissionScore(mission->id(), index);
}

// MissionScoreInfo.cs:86
std::shared_ptr<ScoreInfo> MissionScoreInfo::GetMissionScore(const Guid& missionId, int index) {
    std::shared_ptr<ScoreList> val;
    if (scoreInfoDictionary.TryGetValue(missionId, val) && index < static_cast<int>(val->size())) {
        if (index < 0) throw std::out_of_range("index");
        return (*val)[static_cast<size_t>(index)];
    }
    return nullptr;
}

// MissionScoreInfo.cs:95
int MissionScoreInfo::CompareScoreInfo(const ScoreInfo& a, const ScoreInfo& b) {
    if (a.score > b.score) return -1;
    if (a.score < b.score) return 1;
    if (a.timeTaken < b.timeTaken) return -1;
    if (a.timeTaken > b.timeTaken) return 1;
    return 0;
}

// MissionScoreInfo.cs:116
std::vector<std::string> MissionScoreInfo::GetMissionAwards(Mission* mission) {
    if (mission == nullptr) return {};
    std::shared_ptr<ScoreList> list = scoreInfoDictionary[mission->id()];
    std::vector<std::string> list2;
    for (size_t i = 0, count = list->size(); i < count; ++i) {
        for (const std::string& a : (*list)[i]->awardIds)  // List.AddUniqueRange
            if (std::find(list2.begin(), list2.end(), a) == list2.end()) list2.push_back(a);
    }
    return list2;
}

// MissionScoreInfo.cs:132
void MissionScoreInfo::SaveScores() {
    const std::string fileFullName = MissionsUtils::UsersMissionsPath() + saveFileName;
    ConfigNode configNode;
    ConfigNode* node = configNode.AddNode("MISSIONSCORES");
    Save(*node);
    SaveConfig(configNode, fileFullName);
}

// MissionScoreInfo.cs:141
std::unique_ptr<MissionScoreInfo> MissionScoreInfo::LoadScores() {
    ConfigNode::NodePtr configNode = LoadConfig(MissionsUtils::UsersMissionsPath() + saveFileName);
    auto missionScoreInfo = std::make_unique<MissionScoreInfo>();
    if (configNode != nullptr) {
        ConfigNode* node = nullptr;
        if (configNode->TryGetNode("MISSIONSCORES", node)) missionScoreInfo->Load(*node);
    }
    return missionScoreInfo;
}

// MissionScoreInfo.cs:156
void MissionScoreInfo::Load(const ConfigNode& node) {
    for (const ConfigNode* node2 : node.GetNodes("SCOREINFO")) {
        auto scoreInfo = std::make_shared<ScoreInfo>();
        scoreInfo->Load(*node2);
        scoreInfoDictionary.Add(scoreInfo->missionId, scoreInfo);
    }
}

// MissionScoreInfo.cs:167
void MissionScoreInfo::Save(ConfigNode& node) const {
    for (const auto& kv : scoreInfoDictionary.GetEnumerator()) {
        for (size_t i = 0, count = kv.second->size(); i < count; ++i) (*kv.second)[i]->Save(*node.AddNode("SCOREINFO"));
    }
}

// ---- MissionLocalizer -----------------------------------------------------------------------------------------------------------------

// MissionLocalizer.cs:8
std::vector<ConfigNode::NodePtr> MissionLocalizer::LoadMissionLocalizationFiles() {
    std::vector<ConfigNode::NodePtr> list;
    const std::string folder = MissionsUtils::GetMissionsFolder(MissionTypes::User);
    if (DirectoryExists(folder)) {
        const std::vector<std::string> directories = GetDirectories(folder);
        for (size_t i = 0; i < directories.size(); ++i) {
            if (!DirectoryExists(directories[i] + "/Localization")) continue;
            const std::vector<std::string> files = GetFiles(directories[i] + "/Localization/", ".cfg");
            for (size_t j = 0; j < files.size(); ++j) {
                ConfigNode::NodePtr loaded = LoadConfig(files[j]);
                if (loaded == nullptr) throw NullReferenceException();  // ConfigNode.Load(file).GetNodes(...) on a file that does not load
                for (ConfigNode* n : loaded->GetNodes("Localization")) {
                    // the C# list keeps the node objects; the parent is dropped with `loaded`, so each is deep-copied
                    list.push_back(n->CreateCopy());
                }
            }
        }
    }
    return list;
}

// ---- MissionImporting -------------------------------------------------------------------------------------------------------------------

// MissionImporting.cs:11
void MissionImporting::ImportNewZips() {
    const std::string users = MissionsUtils::UsersMissionsPath();
    if (!DirectoryExists(users)) {
        // Debug.Log("[MissionImporting]:Skipping as Missions Path not yet created")
        return;
    }
    const std::vector<std::string> files = GetFiles(users, ".zip");
    if (files.size() >= 1) {
        const std::string completed = users + "_Imported/";  // MissionsUtils.MissionImportCompletedPath
        if (!DirectoryExists(completed)) CreateDirectory(completed);
        const size_t length = users.size();
        int num = 0;
        for (const std::string& text : files) {
            ImportZip(text, completed + text.substr(length), num);
            num++;
        }
    }
}

// MissionImporting.cs:36
bool MissionImporting::ImportZip(const std::string& filePath, const std::string& moveDestination, int fileNumber) {
    std::vector<std::string> directories;
    try {
        if (!Env::zipFilesAtRoot) throw std::runtime_error("KSPCompression.FilesAtRoot");
        if (Env::zipFilesAtRoot(filePath)) {
            // Debug.LogError("... has files at the root and will not be imported into the Missions Folder")
            Message(Tag("#autoLOC_8005323"), 6.0f, ScreenMessageStyle::UPPER_LEFT);
            MoveImportedFile(filePath, moveDestination);
            return false;
        }
        if (!Env::zipTopLevelDirectories) throw std::runtime_error("KSPCompression.GetTopLevelDirectories");
        directories = Env::zipTopLevelDirectories(filePath);
        if (directories.size() < 1) {
            // Debug.LogError("... has no directories. We wont import this into the Missions Folder")
            Message(Tag("#autoLOC_8005323"), 6.0f, ScreenMessageStyle::UPPER_LEFT);
            MoveImportedFile(filePath, moveDestination);
            return false;
        }
    } catch (const std::exception&) {
        Message("<color=red>[MissionImporting]:Zip File Parsing Failed. Check the log.</color>", 6.0f, ScreenMessageStyle::UPPER_LEFT);
        MoveImportedFile(filePath, moveDestination);
        return false;
    }
    std::string text;
    for (size_t i = 0; i < directories.size(); ++i) {
        if (DirectoryExists(MissionsUtils::UsersMissionsPath() + directories[i])) text = text + "\t" + directories[i] + "\n";
    }
    if (text != "") {
        const std::string text2 = filePath.substr(filePath.rfind('/') + 1);
        Env::PopupDialog d;
        d.name = "ConfirmMissionOverwrite" + std::to_string(fileNumber);
        d.message = Fmt("#autoLOC_8005324", {text2, text});
        d.title = Tag("#autoLOC_8005325");
        d.width = 350.0f;
        d.buttons.push_back({Tag("#autoLOC_8005326"), [filePath, directories, moveDestination] { ImportZipConfirmed(filePath, directories, moveDestination); }});
        d.buttons.push_back({Tag("#autoLOC_8005327"), [filePath, moveDestination] {
                                 // Debug.LogWarning("Zip file bypassed. Moved to Imported folder")
                                 MoveImportedFile(filePath, moveDestination);
                             }});
        SpawnPopup(std::move(d));
    } else {
        ImportZipConfirmed(filePath, directories, moveDestination);
    }
    return true;
}

// MissionImporting.cs:91
void MissionImporting::ImportZipConfirmed(const std::string& filePath, const std::vector<std::string>& directories, const std::string& moveDestination) {
    for (size_t i = 0; i < directories.size(); ++i) {
        if (DirectoryExists(MissionsUtils::UsersMissionsPath() + directories[i])) DeleteDirectory(MissionsUtils::UsersMissionsPath() + directories[i]);
    }
    try {
        if (!Env::zipDecompress) throw std::runtime_error("KSPCompression.DecompressFile");
        Env::zipDecompress(filePath, MissionsUtils::UsersMissionsPath());
        Message(Tag("#autoLOC_8005328"), 5.0f, ScreenMessageStyle::UPPER_LEFT);
        MoveImportedFile(filePath, moveDestination);
        if (Env::parseMissionPacksCFG) Env::parseMissionPacksCFG(MissionsUtils::UsersMissionsPath());
        GameEvents::Mission::onMissionImported.Fire();
    } catch (const std::exception&) {
        Message(Tag("#autoLOC_8005323"), 6.0f, ScreenMessageStyle::UPPER_LEFT);
        MoveImportedFile(filePath, moveDestination);
    }
}

// MissionImporting.cs:117
void MissionImporting::MoveImportedFile(const std::string& filePath, const std::string& destinationPath) {
    if (FileExists(destinationPath)) {
        const std::string text = destinationPath.substr(destinationPath.rfind('/') + 1);
        Env::PopupDialog d;
        d.name = "ConfirmImportedFileOverwrite";
        d.message = Fmt("#autoLOC_8005331", {text});
        d.title = Tag("#autoLOC_8005332");
        d.width = 350.0f;
        d.buttons.push_back({Tag("#autoLOC_8005333"), [filePath, destinationPath] { MoveFile(filePath, destinationPath, true); }});
        d.buttons.push_back({Tag("#autoLOC_8005334"), [filePath, destinationPath] { MoveFile(filePath, GenerateFilePathWithDate(destinationPath), true); }});
        SpawnPopup(std::move(d));
    } else if (MoveFile(filePath, destinationPath, true)) {
        Message(Tag("#autoLOC_8005329"), 5.0f);
        // Debug.Log("[MissionImporting]:Imported Mission File ... moved to ...")
    } else {
        Message(Tag("#autoLOC_8005330"), 10.0f, ScreenMessageStyle::UPPER_LEFT);
        // Debug.LogWarning("[MissionImporting]:Imported Mission File Move Failed")
    }
}

// ---- Editor/MissionEditorHistory ------------------------------------------------------------------------------------------------------

namespace Editor {

void MissionEditorHistory::Environment::Reset() {
    frameCount = nullptr;
    selectAffectedObject = nullptr;
    selected.clear();
    destroyObject = nullptr;
    destroyTargetObject = nullptr;
    destroyed.clear();
    destroyedTargets.clear();
}

// MissionEditorHistory.cs:11
struct MissionEditorHistory::HistoryAction {
    IMEHistoryTarget* target;
    ConfigNode::NodePtr data;
    ActionCallback callback;
    bool destroyOnClear;
    std::vector<DestroyHandle> destroyTargets;
    Guid StateId;

    // MissionEditorHistory.cs:11
    HistoryAction(IMEHistoryTarget* t, ActionCallback cb, bool destroyOnClear_, const Guid& currentHistoryStateID, std::vector<DestroyHandle> destroyTargets_)
        : target(t), callback(std::move(cb)), destroyOnClear(destroyOnClear_), destroyTargets(std::move(destroyTargets_)) {
        if (t == nullptr) throw NullReferenceException();
        data = t->GetState();
        StateId = currentHistoryStateID;
    }
    // MissionEditorHistory.cs:25
    explicit HistoryAction(const HistoryAction& action) : target(action.target), callback(action.callback), destroyOnClear(action.destroyOnClear), destroyTargets(action.destroyTargets) {
        data = target->GetState();
        StateId = action.StateId;
    }
};

// MissionEditorHistory.cs:46
class MissionEditorHistory::ActionStack {
public:
    // MissionEditorHistory.cs:46
    explicit ActionStack(int stackLimit = -1) : limit(stackLimit < 0 ? 0x7FFFFFFF : stackLimit) {}
    int Count() const { return static_cast<int>(items.size()); }  // :@Count
    // MissionEditorHistory.cs:62
    void Push(std::shared_ptr<HistoryAction> item) {
        const int frame = Environment::frameCount ? Environment::frameCount() : 0;
        if (lastFramePush != frame) {
            if (static_cast<int>(items.size()) >= limit) items.erase(items.begin());
            items.push_back({std::move(item)});
            lastFramePush = frame;
        } else {
            if (items.empty()) throw std::out_of_range("index");  // items[items.Count - 1]
            items.back().push_back(std::move(item));
        }
    }
    // MissionEditorHistory.cs:88
    std::vector<std::shared_ptr<HistoryAction>> Pop() {
        if (items.size() > 0) {
            auto result = std::move(items.back());
            items.pop_back();
            return result;
        }
        throw NullReferenceException();  // the C# returns null; every caller dereferences it
    }
    // MissionEditorHistory.cs:99
    void Clear() {
        lastFramePush = -1;
        items.clear();
    }
    std::vector<std::vector<std::shared_ptr<HistoryAction>>> items;
    int limit;
    int lastFramePush = -1;
};

// MissionEditorHistory.cs:132
MissionEditorHistory::MissionEditorHistory() {
    if (instance_ != nullptr) {
        duplicate_ = true;  // UnityEngine.Object.DestroyImmediate(this)
        return;
    }
    instance_ = this;
    UndoActions = std::make_unique<ActionStack>();
    RedoActions = std::make_unique<ActionStack>();
}
MissionEditorHistory::~MissionEditorHistory() {
    if (instance_ == this) instance_ = nullptr;
}

// MissionEditorHistory.cs:114
bool MissionEditorHistory::HasHistory() {
    if (instance_ == nullptr) throw NullReferenceException();
    return instance_->UndoActions->Count() > 0;
}

// MissionEditorHistory.cs:116
Guid MissionEditorHistory::HistoryStateId() {
    Mission* m = MissionsEnvironment::editorMission ? MissionsEnvironment::editorMission() : nullptr;
    if (m != nullptr) return m->historyId;
    return Guid();
}

void MissionEditorHistory::SetHistoryStateId(const Guid& value) {
    Mission* m = MissionsEnvironment::editorMission ? MissionsEnvironment::editorMission() : nullptr;
    if (m == nullptr) throw NullReferenceException();  // MissionEditorLogic.Instance.EditorMission.historyId = value
    m->historyId = value;
}

// MissionEditorHistory.cs:144
void MissionEditorHistory::PushUndoAction(IMEHistoryTarget* target, ActionCallback callback) { PushUndoAction(target, std::move(callback), false); }

// MissionEditorHistory.cs:149
void MissionEditorHistory::PushUndoAction(IMEHistoryTarget* target, ActionCallback callback, bool destroyOnClear, std::vector<DestroyHandle> destroyTargets) {
    if (instance_ == nullptr) throw NullReferenceException();
    Mission* m = MissionsEnvironment::editorMission ? MissionsEnvironment::editorMission() : nullptr;
    if (!instance_->ActionInProgress) {
        if (m == nullptr) throw NullReferenceException();  // MissionEditorLogic.Instance.EditorMission.IsTutorialMission
        if (!m->IsTutorialMission()) {
            instance_->UndoActions->Push(std::make_shared<HistoryAction>(target, std::move(callback), destroyOnClear, HistoryStateId(), std::move(destroyTargets)));
            SetHistoryStateId(MissionsEnvironment::NewGuid());
            instance_->ClearRedoActions();
        }
    }
}

// MissionEditorHistory.cs:159
void MissionEditorHistory::Undo() {
    if (instance_ == nullptr) throw NullReferenceException();
    if (instance_->UndoActions->Count() <= 0) return;
    instance_->ActionInProgress = true;
    try {
        std::vector<std::shared_ptr<HistoryAction>> list = instance_->UndoActions->Pop();
        for (size_t i = 0; i < list.size(); ++i) {
            std::shared_ptr<HistoryAction> historyAction = list[i];
            instance_->RedoActions->Push(std::make_shared<HistoryAction>(*historyAction));
            historyAction->callback(*historyAction->data, HistoryType::Undo);
            SelectAffectedObject(*historyAction);
            SetHistoryStateId(historyAction->StateId);
        }
    } catch (const std::exception&) {
        // Debug.LogException / "Exception caught during undo! Flushing undo cache to keep editor working!"
        Clear();
    }
    instance_->ActionInProgress = false;
}

// MissionEditorHistory.cs:187
void MissionEditorHistory::Redo() {
    if (instance_ == nullptr) throw NullReferenceException();
    if (instance_->RedoActions->Count() <= 0) return;
    instance_->ActionInProgress = true;
    try {
        std::vector<std::shared_ptr<HistoryAction>> list = instance_->RedoActions->Pop();
        for (size_t i = 0; i < list.size(); ++i) {
            std::shared_ptr<HistoryAction> historyAction = list[i];
            instance_->UndoActions->Push(std::make_shared<HistoryAction>(*historyAction));
            historyAction->callback(*historyAction->data, HistoryType::Redo);
            SelectAffectedObject(*historyAction);
            SetHistoryStateId(historyAction->StateId);
        }
    } catch (const std::exception&) {
        // Debug.LogException / "Exception caught during redo! Flushing undo cache to keep editor working!"
        Clear();
    }
    instance_->ActionInProgress = false;
}

// MissionEditorHistory.cs:215
void MissionEditorHistory::SelectAffectedObject(HistoryAction& action) {
    // the MEGUINode / MEGUIParameter selection of the editor (a seam: with no editor nothing is selected)
    if (Environment::selectAffectedObject) Environment::selectAffectedObject(action.target);
    else Environment::selected.push_back(action.target);
}

// MissionEditorHistory.cs:99
void MissionEditorHistory::Clear() {
    if (instance_ == nullptr) throw NullReferenceException();
    instance_->UndoActions->Clear();
    instance_->ClearRedoActions();
}

// MissionEditorHistory.cs:241
void MissionEditorHistory::ClearRedoActions() {
    while (RedoActions->Count() > 0) {
        std::vector<std::shared_ptr<HistoryAction>> list = RedoActions->Pop();
        for (size_t i = 0; i < list.size(); ++i) {
            std::shared_ptr<HistoryAction> historyAction = list[i];
            if (!historyAction->destroyOnClear) continue;
            if (historyAction->destroyTargets.size() != 0) {
                for (size_t j = 0, num = historyAction->destroyTargets.size(); j < num; ++j) {
                    if (Environment::destroyObject) Environment::destroyObject(historyAction->destroyTargets[j]);
                    else Environment::destroyed.push_back(historyAction->destroyTargets[j]);
                }
            } else {
                if (Environment::destroyTargetObject) Environment::destroyTargetObject(historyAction->target);
                else Environment::destroyedTargets.push_back(historyAction->target);
            }
        }
    }
}

std::vector<std::vector<MissionEditorHistory::ActionView>> MissionEditorHistory::UndoView() const {
    std::vector<std::vector<ActionView>> out;
    for (const auto& group : UndoActions->items) {
        out.emplace_back();
        for (const auto& a : group) out.back().push_back({a->target, a->data, a->StateId, a->destroyOnClear, a->destroyTargets});
    }
    return out;
}
std::vector<std::vector<MissionEditorHistory::ActionView>> MissionEditorHistory::RedoView() const {
    std::vector<std::vector<ActionView>> out;
    for (const auto& group : RedoActions->items) {
        out.emplace_back();
        for (const auto& a : group) out.back().push_back({a->target, a->data, a->StateId, a->destroyOnClear, a->destroyTargets});
    }
    return out;
}

}  // namespace Editor

}  // namespace ksp::Expansions::Missions
