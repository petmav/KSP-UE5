// WP-54: Making History mission runtime, slice 1. Enums, small value types and the environment of the Expansions.Missions namespace.
//
// Spec: Assembly-CSharp/Expansions/Missions/{MissionEndOptions,MissionDifficulty,MissionDifficultyWithColor,MissionCameraModeOptions,
// MissionCameraLockOptions,MissionTypes,ScoreModifierType,ActionNumberOperator,TestComparison*,Runtime/MissionAppMode,
// Editor/{ValidationStatus,ValidatorMode,MENodeConnectionType,DisplayParameter,SurfaceLocation,MissionValidationTestResult}}.cs
//
// Owners of the seams (every member of MissionsEnvironment: unset = the stated default):
//   WP-55  node library (the Tests.* / Actions.* modules register themselves in TestModuleRegistry / ActionModuleRegistry)
//   WP-46/47  UI, scene shell, MissionSystem host (screen messages, the MissionSystem coroutine host, analytics)
//   WP-19  flight scene (vessel queries read by the award modules)
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/dotnet/enum.hpp"

namespace ksp {
class Vessel;
}

namespace ksp::Expansions::Missions {

class MENode;
class Mission;
class TestGroup;
class TestModule;
class VesselSituation;
class Awards;
class MissionFileInfo;
class IActionModule;
class AwardDefinition;
class ITestNodeLabel;

// MissionEndOptions.cs:5
enum class MissionEndOptions { Success = 0, Fail = 1 };
// MissionDifficulty.cs:5
enum class MissionDifficulty { Beginner = 0, Intermediate = 1, Advanced = 2, Extreme = 3 };
// MissionDifficultyWithColor.cs:5
enum class MissionDifficultyWithColor { Beginner = 0, Intermediate = 1, Advanced = 2, Extreme = 3 };
// MissionCameraModeOptions.cs:5
enum class MissionCameraModeOptions { NoChange = 0, Flight = 1, IVA = 2 };
// MissionCameraLockOptions.cs:5
enum class MissionCameraLockOptions { NoChange = 0, LockAllowMap = 1, LockDisableMap = 2, Unlock = 3 };
// MissionTypes.cs:3
enum class MissionTypes { Stock = 0, User = 1, Steam = 2, Base = 3 };
// ScoreModifierType.cs:5
enum class ScoreModifierType { Multiply = 0, Divide = 1, Substract = 2, Set = 3 };
// ActionNumberOperator.cs:3
enum class ActionNumberOperator { Set = 0, Add = 1, Subtract = 2, Multiply = 3, Divide = 4 };
// TestComparisonOperator.cs:5
enum class TestComparisonOperator { LessThan = 0, LessThanorEqual = 1, Equal = 2, GreaterThanorEqual = 3, GreaterThan = 4 };
// TestComparisonLessGreaterEqual.cs:5
enum class TestComparisonLessGreaterEqual { LessOrEqual = 0, Equal = 1, GreaterOrEqual = 2 };
// TestComparisonLessGreaterOnly.cs:5
enum class TestComparisonLessGreaterOnly { LessThan = 0, GreaterThan = 1 };
// Runtime/MissionAppMode.cs:3
enum class MissionAppMode { Play = 0, Editor = 1, EditorFreeBuild = 2 };

namespace Editor {
// Editor/ValidationStatus.cs:5
enum class ValidationStatus { Pass = 0, Warn = 1, Fail = 2, None = 3 };
// Editor/ValidatorMode.cs:5
enum class ValidatorMode { Manual = 0, Save = 1, AutoAfterRun = 2, FullAuto = 3 };
// Editor/MENodeConnectionType.cs:3
enum class MENodeConnectionType { Input = 0, Output = 1 };

// Editor/DisplayParameter.cs:6 (a struct: Equals compares both strings; List.AddUnique uses it)
struct DisplayParameter {
    std::string module;
    std::string parameter;
    // Editor/DisplayParameter.cs:13
    void Load(const ConfigNode& node);
    bool operator==(const DisplayParameter&) const = default;
};

// Editor/SurfaceLocation.cs:3
struct SurfaceLocation {
    double altitude = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
};

// Editor/MissionValidationTestResult.cs:5
struct MissionValidationTestResult {
    Guid nodeId;
    ValidationStatus status = ValidationStatus::Pass;
    std::string nodeName;
    std::string type;
    std::string message;
};
}  // namespace Editor

// EnumExtensions.cs:44 Enum.Description(): the [Description] text of the member, else its name
template <class E>
std::string EnumDescription(E e) {
    const std::string name = dotnet::EnumToString(e);
    const std::optional<std::string> d = dotnet::EnumMemberDescription<E>(name);
    return d ? *d : name;
}

// ---------------------------------------------------------------------------------------------------------------------------
// The seams. All static inline, installed by the host (or a test); `Reset()` restores the defaults.
struct MissionsEnvironment {
    // Guid.NewGuid() (MENode.Awake :415, Mission.Awake :256, RegenerateMissionID, ScoreInfo). Unset: a process-wide deterministic counter.
    static inline std::function<Guid()> newGuid;
    // KSPUtil.SystemDateTime.DateTimeNow().Ticks (Mission.InitSeed :342). Unset: 0.
    static inline std::function<int64_t()> nowTicks;
    // Planetarium.GetUniversalTime() (MENode.ActivateNode :609, ScoreModule_Time :AwardScore). Unset: Planetarium::GetUniversalTime().
    static inline std::function<double()> universalTime;
    // HighLogic.CurrentGame.UniversalTime (StartMission :788, AwardModule_MissionTime). Unset: HighLogic::CurrentGame()->UniversalTime or 0.
    static inline std::function<double()> gameUniversalTime;
    // ExpansionsLoader.IsExpansionInstalled("MakingHistory") (Mission.CheckHash :361). Unset: SaveEnvironment::Current().
    static inline std::function<bool()> makingHistoryInstalled;
    // ExpansionsLoader.IsExpansionInstalled("MakingHistory", hashBytes, signature) (Mission.CheckHash). Unset: false.
    static inline std::function<bool()> makingHistorySignedMission;
    // ExpansionsLoader.BuildMissionFileHash(configNode) (Mission.Load :2128). Unset: empty.
    static inline std::function<std::vector<uint8_t>(const ConfigNode&)> buildMissionFileHash;
    // MissionSystem.Instance.StartCoroutine(...) (MENode.ActivateNode :634, Mission.ResumeMission :817). Unset: HighLogic::Scheduler().
    static inline std::function<void(Enumerator)> startCoroutine;
    // ScreenMessages.PostScreenMessage(text, duration, UPPER_LEFT, persist: true) (MENode.ActivateNode :641). Unset: recorded in screenMessages.
    static inline std::function<void(const std::string& text, float duration)> postScreenMessage;
    static inline std::vector<std::pair<std::string, float>> screenMessages;
    // MissionSystem.IsActive (Mission.UpdateMission :843). Unset: true.
    static inline std::function<bool()> missionSystemActive;
    // GameSettings.MISSION_VALIDATOR_MODE (MissionEditorValidator.Awake :36). Unset: AutoAfterRun.
    static inline Editor::ValidatorMode validatorSetting = Editor::ValidatorMode::AutoAfterRun;
    // HighLogic.LoadedScene == EDITOR / MISSIONBUILDER and the scene the host says is loaded are read through HighLogic.
    // Mission.situation defaults (MissionSituation(Mission) :224: KerbalRoster.GenerateInitialCrewRoster(MISSION) at level 5 and
    // GameParameters.GetDefaultParameters(MISSION, Normal)). Tests of the pure graph code switch them off; the roster needs the game data.
    static inline bool createSituationDefaults = true;
    // AnalyticsUtil.LogMissionStart / Completed / Failed (WP-59). Unset: dropped.
    static inline std::function<void(const char* what, Mission*)> analytics;
    // MissionEditorLogic.UpdateValidationLed() (end of MissionEditorValidator.RunValidation). Unset: dropped.
    static inline std::function<void()> updateValidationLed;
    // MissionEditorLogic.Instance.basicNodes icon lookup for MENode.Load (:1055). Unset: no editor, nothing looked up.
    static inline std::function<std::string(const std::string& basicNodeName)> basicNodeIconURL;
    // MissionEditorLogic.GetCategoryColor(category) (MEBasicNode.Load :168). Unset: Color.gray.
    static inline std::function<Color(const std::string& category)> categoryColor;
    // VesselLabels.AddNodeLabel(ITestNodeLabel) / RemoveNodeLabel(NodeLabel) (TestModule.InitializeTest :145 / ClearTest :160; WP-46 labels).
    // The opaque handle is the NodeLabel. Unset: no labels.
    static inline std::function<void*(ITestNodeLabel*)> addNodeLabel;
    static inline std::function<void(void*)> removeNodeLabel;
    // PartLoader.LoadedPartsList names (MissionPartFilter.GetExcludedParts :105). Unset: empty.
    static inline std::function<std::vector<std::string>()> loadedPartNames;
    // ---- Mission / MissionSituation seams (craft files, MissionSystem, FlightGlobals) ----
    // ExpansionsLoader.GetExpansionVersion("MakingHistory") (Mission.Save :2181). Unset: "".
    static inline std::function<std::string()> makingHistoryVersion;
    // Mission.RebuildCraftFileList (:1350, file system + MissionCraft: slice 2) and ProcessMappedVesselNodeMap (:1860: casts to the WP-55 test
    // and action classes). Unset: not run.
    static inline std::function<void(Mission&)> rebuildCraftFileList;
    static inline std::function<void(Mission&)> processMappedVesselNodeMap;
    // Mission.GetMissionCraftByFileName(name) != null (InitMission :524). Unset: true (nothing is removed).
    static inline std::function<bool(Mission&, const std::string& craftFileName)> craftFileExists;
    // MissionEditorLogic.MissionToTest (Mission.Load :1982). Unset: "".
    static inline std::function<std::string()> missionToTest;
    // Mission.UpdateMissionFileInfo(path) = MissionFileInfo.CreateFromPath (reads the .mission through MissionSystem: slice 2). Unset: not run.
    static inline std::function<std::shared_ptr<MissionFileInfo>(Mission&, const std::string& path)> updateMissionFileInfo;
    // actionCreateVessel.CreatePlaceholderCraft() (MissionSituation.FillVesselSituationList :564). Unset: not run.
    static inline std::function<void(IActionModule*)> createPlaceholderCraft;
    // MissionSystem.Instance.CheckFireBriefingDialog(mission, chckMissionStarted: false) (Mission.ResumeMission :808). Unset: not run.
    static inline std::function<void(Mission*)> checkFireBriefingDialog;
    // FlightGlobals.SetActiveVessel(vessel) (Mission.UpdateMission :859). Unset: not run.
    static inline std::function<void(Vessel*)> setActiveVessel;
    // GamePersistence.SaveGame("persistent", ...) + lastcreatevesselspawn.missionsfs copy (Mission.UpdateMission :869-:885). Unset: not run.
    static inline std::function<void(Mission*)> saveRevertToVesselSpawn;
    // FlightGlobals.PersistentVesselIds.ContainsKey / PersistentLoadedPartIds / PersistentUnloadedPartIds (Mission.CurrentVesselID :2258).
    // Unset: the AwardEnvironment vessel lookup and "part ids unknown".
    enum class PartLookup { NotFound, FoundNoVessel, Found };
    static inline std::function<bool(uint32_t persistentVesselId)> persistentVesselIdExists;
    static inline std::function<PartLookup(uint32_t partPersistentId, bool loaded, uint32_t& vesselPersistentId)> partVessel;
    // The vessel world read by the award modules (FlightGlobals / Vessel / PartResourceLibrary), WP-19: see AwardModuleEnvironment.

    static Guid NewGuid();
    static int64_t NowTicks();
    static double UniversalTime();
    static double GameUniversalTime();
    static bool MakingHistoryInstalled();
    static void StartCoroutine(Enumerator routine);
    static void PostScreenMessage(const std::string& text, float duration);
    static bool MissionSystemActive();
    static void Reset();
};

}  // namespace ksp::Expansions::Missions

// ---- enum metadata (declaration order, [Description] text) -------------------------------------------------------------------
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::MissionEndOptions> {
    using E = ksp::Expansions::Missions::MissionEndOptions;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Success, "Success", "#autoLOC_8000199"}, {E::Fail, "Fail", "#autoLOC_8000200"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::MissionDifficulty> {
    using E = ksp::Expansions::Missions::MissionDifficulty;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Beginner, "Beginner", "#autoLOC_8003167"},
                                                {E::Intermediate, "Intermediate", "#autoLOC_8003168"},
                                                {E::Advanced, "Advanced", "#autoLOC_8003169"},
                                                {E::Extreme, "Extreme", "#autoLOC_8003170"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::MissionDifficultyWithColor> {
    using E = ksp::Expansions::Missions::MissionDifficultyWithColor;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Beginner, "Beginner", "#autoLOC_8003171"},
                                                {E::Intermediate, "Intermediate", "#autoLOC_8003172"},
                                                {E::Advanced, "Advanced", "#autoLOC_8003173"},
                                                {E::Extreme, "Extreme", "#autoLOC_8003174"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::MissionCameraModeOptions> {
    using E = ksp::Expansions::Missions::MissionCameraModeOptions;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        {E::NoChange, "NoChange", "#autoLOC_8004183"}, {E::Flight, "Flight", "#autoLOC_8004184"}, {E::IVA, "IVA", "#autoLOC_8004185"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::MissionCameraLockOptions> {
    using E = ksp::Expansions::Missions::MissionCameraLockOptions;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::NoChange, "NoChange", "#autoLOC_8004183"},
                                                {E::LockAllowMap, "LockAllowMap", "#autoLOC_8004187"},
                                                {E::LockDisableMap, "LockDisableMap", "#autoLOC_8004189"},
                                                {E::Unlock, "Unlock", "#autoLOC_8004188"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::MissionTypes> {
    using E = ksp::Expansions::Missions::MissionTypes;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        {E::Stock, "Stock"}, {E::User, "User"}, {E::Steam, "Steam"}, {E::Base, "Base"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::ScoreModifierType> {
    using E = ksp::Expansions::Missions::ScoreModifierType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Multiply, "Multiply", "#autoLOC_8100127"},
                                                {E::Divide, "Divide", "#autoLOC_8100128"},
                                                {E::Substract, "Substract", "#autoLOC_8100129"},
                                                {E::Set, "Set", "#autoLOC_8100130"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::ActionNumberOperator> {
    using E = ksp::Expansions::Missions::ActionNumberOperator;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        {E::Set, "Set"}, {E::Add, "Add"}, {E::Subtract, "Subtract"}, {E::Multiply, "Multiply"}, {E::Divide, "Divide"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::TestComparisonOperator> {
    using E = ksp::Expansions::Missions::TestComparisonOperator;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::LessThan, "LessThan", "#autoLOC_8000054"},
                                                {E::LessThanorEqual, "LessThanorEqual", "#autoLOC_8000055"},
                                                {E::Equal, "Equal", "#autoLOC_8000056"},
                                                {E::GreaterThanorEqual, "GreaterThanorEqual", "#autoLOC_8000057"},
                                                {E::GreaterThan, "GreaterThan", "#autoLOC_8000058"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::TestComparisonLessGreaterEqual> {
    using E = ksp::Expansions::Missions::TestComparisonLessGreaterEqual;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::LessOrEqual, "LessOrEqual", "#autoLOC_8000055"},
                                                {E::Equal, "Equal", "#autoLOC_8000056"},
                                                {E::GreaterOrEqual, "GreaterOrEqual", "#autoLOC_8000057"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::TestComparisonLessGreaterOnly> {
    using E = ksp::Expansions::Missions::TestComparisonLessGreaterOnly;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::LessThan, "LessThan", "#autoLOC_8000054"},
                                                {E::GreaterThan, "GreaterThan", "#autoLOC_8000058"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::MissionAppMode> {
    using E = ksp::Expansions::Missions::MissionAppMode;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Play, "Play"}, {E::Editor, "Editor"}, {E::EditorFreeBuild, "EditorFreeBuild"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Editor::ValidationStatus> {
    using E = ksp::Expansions::Missions::Editor::ValidationStatus;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Pass, "Pass", "#autoLOC_8006047"},
                                                {E::Warn, "Warn", "#autoLOC_8006048"},
                                                {E::Fail, "Fail", "#autoLOC_8003072"},
                                                {E::None, "None", "#autoLOC_6003083"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Editor::ValidatorMode> {
    using E = ksp::Expansions::Missions::Editor::ValidatorMode;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Manual, "Manual", "#autoLOC_8200118"},
                                                {E::Save, "Save", "#autoLOC_8200119"},
                                                {E::AutoAfterRun, "AutoAfterRun", "#autoLOC_8200120"},
                                                {E::FullAuto, "FullAuto", "#autoLOC_8200121"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Editor::MENodeConnectionType> {
    using E = ksp::Expansions::Missions::Editor::MENodeConnectionType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Input, "Input"}, {E::Output, "Output"}};
};

// System.Guid as a Dictionary / unordered key (the .NET Guid.GetHashCode is not observable here: lookup only)
template <>
struct std::hash<ksp::Guid> {
    size_t operator()(const ksp::Guid& g) const noexcept {
        uint64_t a = 0, b = 0;
        for (int i = 0; i < 8; ++i) {
            a = (a << 8) | g.bytes[i];
            b = (b << 8) | g.bytes[8 + i];
        }
        return static_cast<size_t>(a * 0x9E3779B97F4A7C15ull ^ (b + 0x7F4A7C15ull + (a << 6) + (a >> 2)));
    }
};
