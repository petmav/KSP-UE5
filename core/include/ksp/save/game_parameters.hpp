// GameParameters (Assembly-CSharp/GameParameters.cs) with every stock ParameterNode / CustomParameterNode:
// FlightParams, EditorParams, TrackingStationParams, SpaceCenterParams, DifficultyParams, CareerParams, AdvancedParams
// (GameParameters.cs), CommNet.CommNetParams (CommNet/CommNetParams.cs), Expansions.Missions.MissionParamsGeneral /
// MissionParamsExtras / MissionParamsFacilities (Expansions/Missions/*.cs); defaults, difficulty presets
// (SetDifficultyPresets / GetDefaultParameters), Load / Save.
//
// C# reflection is replaced by per-class member tables in Mono's Type.GetMembers(Instance | Public |
// FlattenHierarchy) order: methods, constructors, properties, events, fields. ParameterNode.Save therefore writes the
// read/write properties first, then the fields, each in declaration order (AdvancedParams: EnableKerbalExperience,
// ImmediateLevelUp, then AllowNegativeCurrency ...), exactly as the real saves show. Values are obj.ToString()
// (bool "True"/"False", float.ToString() = "G" 7 digits, int, string; null strings are skipped).
//
// The custom parameter classes are discovered in Mono Assembly.GetTypes() order of the shipped Assembly-CSharp.dll
// (TypeDef rows: MissionParamsExtras 2810, MissionParamsFacilities 2811, MissionParamsGeneral 2812, CommNetParams 3331,
// AdvancedParams 3697 -- nested types come last). The first GameParameters constructed in the process inserts them in
// reverse discovery order, then sorts the static list by Title (culture compare: "#autoLOC_8005012" (Facilities) <
// "#autoLOC_8005024" (Extras) < "#autoLOC_8005035" (General) < "Advanced Options" < "CommNet Options"); every later
// instance inserts in reverse sorted order, which is the order saves show: CommNetParams, AdvancedParams,
// MissionParamsGeneral, MissionParamsExtras, MissionParamsFacilities. Titles of AdvancedParams / CommNetParams are
// Localizer.Format(tag): the order depends on the game language, as in KSP (kspcore falls back to the en-us text when
// the Localizer has no such tag).
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ksp/save/save_enums.hpp"

namespace ksp {

class ConfigNode;
class GameParameters;

// Reflection substitute for one public instance member of a ParameterNode class.
struct ParameterMemberInfo {
    enum class Kind { Field, Property, ReadOnlyProperty, Method };
    enum class Type { None, String, Int, Float, Bool, Double };
    const char* name = "";
    Kind kind = Kind::Field;
    Type type = Type::None;
    bool autoPersistance = true;  // [CustomParameterUI(autoPersistance)] (true when there is no attribute)
};

class GameParameters {
public:
    using Preset = GameParameters_Preset;
    using GameMode = GameParameters_GameMode;

    // GameParameters.cs:436 ParameterNode
    class ParameterNode {
    public:
        virtual ~ParameterNode() = default;
        // GameParameters.cs:438 Load (nullptr: "Cannot find settings" warning). Unknown names are ignored; a name that
        // is a method throws NullReferenceException; int/float/bool/double.Parse exceptions propagate (as in KSP).
        void Load(const ConfigNode* node);
        // GameParameters.cs:533 Save
        void Save(ConfigNode& node) const;
        virtual void OnLoad(const ConfigNode& node) { (void)node; }
        virtual void OnSave(ConfigNode& node) const { (void)node; }

        // GetType().Name
        virtual const char* TypeName() const = 0;
        // public instance members in Mono GetMembers order
        virtual const std::vector<ParameterMemberInfo>& Members() const = 0;
        // obj.ToString() of a field/property (nullopt = null)
        virtual std::optional<std::string> GetMemberText(std::string_view name) const = 0;
        // assign a parsed value of the member's type
        virtual void SetMemberValue(std::string_view name, const std::string& text) = 0;
    };

    // GameParameters.cs:397 CustomParameterNode
    class CustomParameterNode : public ParameterNode {
    public:
        virtual std::string Title() const = 0;
        virtual std::string DisplaySection() const = 0;
        virtual std::string Section() const = 0;
        virtual int SectionOrder() const = 0;
        virtual GameMode GetGameMode() const = 0;  // C# property GameMode
        virtual bool HasPresets() const = 0;
        // GameParameters.cs:411 (NotImplementedException unless overridden)
        virtual void SetDifficultyPreset(Preset preset);
        // GameParameters.cs:416-431 (member = MemberInfo.Name)
        virtual bool Interactible(std::string_view member, const GameParameters& parameters) const;
        virtual bool Enabled(std::string_view member, const GameParameters& parameters) const;
        // IList ValidValues(member): null for every stock class
        virtual bool HasValidValues(std::string_view member) const;
    };

    // GameParameters.cs:110
    class FlightParams : public ParameterNode {
    public:
        bool CanQuickSave = true, CanQuickLoad = true, CanAutoSave = true, CanUseMap = true, CanSwitchVesselsNear = true,
             CanSwitchVesselsFar = true, CanTimeWarpHigh = true, CanTimeWarpLow = true, CanEVA = true, CanIVA = true,
             CanBoard = true, CanRestart = true, CanLeaveToEditor = true, CanLeaveToTrackingStation = true,
             CanLeaveToSpaceCenter = true, CanLeaveToMainMenu = true;
        const char* TypeName() const override { return "FlightParams"; }
        const std::vector<ParameterMemberInfo>& Members() const override;
        std::optional<std::string> GetMemberText(std::string_view name) const override;
        void SetMemberValue(std::string_view name, const std::string& text) override;
    };

    // GameParameters.cs:147
    class EditorParams : public ParameterNode {
    public:
        bool CanSave = true, CanLoad = true, CanStartNew = true, CanLaunch = true, CanLeaveToSpaceCenter = true,
             CanLeaveToMainMenu = false;
        int startUpMode = 0;
        std::string craftFileToLoad = "";
        const char* TypeName() const override { return "EditorParams"; }
        const std::vector<ParameterMemberInfo>& Members() const override;
        std::optional<std::string> GetMemberText(std::string_view name) const override;
        void SetMemberValue(std::string_view name, const std::string& text) override;
    };

    // GameParameters.cs:166
    class TrackingStationParams : public ParameterNode {
    public:
        bool CanFlyVessel = true, CanAbortVessel = true, CanLeaveToSpaceCenter = true, CanLeaveToMainMenu = false;
        const char* TypeName() const override { return "TrackingStationParams"; }
        const std::vector<ParameterMemberInfo>& Members() const override;
        std::optional<std::string> GetMemberText(std::string_view name) const override;
        void SetMemberValue(std::string_view name, const std::string& text) override;
    };

    // GameParameters.cs:177
    class SpaceCenterParams : public ParameterNode {
    public:
        bool CanGoInVAB = true, CanGoInSPH = true, CanGoInTrackingStation = true, CanLaunchAtPad = true,
             CanLaunchAtRunway = true, CanGoToAdmin = true, CanGoToAstronautC = true, CanGoToMissionControl = true,
             CanGoToRnD = true, CanSelectFlag = true, CanLeaveToMainMenu = true;
        const char* TypeName() const override { return "SpaceCenterParams"; }
        const std::vector<ParameterMemberInfo>& Members() const override;
        std::optional<std::string> GetMemberText(std::string_view name) const override;
        void SetMemberValue(std::string_view name, const std::string& text) override;
    };

    // GameParameters.cs:202 (RespawnTimer = (float)GameSettings.DEFAULT_KERBAL_RESPAWN_TIMER at construction)
    class DifficultyParams : public ParameterNode {
    public:
        DifficultyParams();
        bool AutoHireCrews = false;
        bool MissingCrewsRespawn = true;
        float RespawnTimer = 7200.0f;
        bool BypassEntryPurchaseAfterResearch = true;
        bool AllowStockVessels = false;
        bool IndestructibleFacilities = false;
        float ResourceAbundance = 1.0f;
        float ReentryHeatScale = 1.0f;
        bool EnableCommNet = false;
        bool AllowOtherLaunchSites = false;
        bool persistKerbalInventories = false;
        const char* TypeName() const override { return "DifficultyParams"; }
        const std::vector<ParameterMemberInfo>& Members() const override;
        std::optional<std::string> GetMemberText(std::string_view name) const override;
        void SetMemberValue(std::string_view name, const std::string& text) override;
    };

    // GameParameters.cs:227
    class CareerParams : public ParameterNode {
    public:
        std::string TechTreeUrl = "GameData/Squad/Resources/TechTree.cfg";
        float StartingFunds = 25000.0f;
        float StartingScience = 0.0f;
        float StartingReputation = 0.0f;
        float FundsGainMultiplier = 1.0f;
        float RepGainMultiplier = 1.0f;
        float ScienceGainMultiplier = 1.0f;
        float FundsLossMultiplier = 1.0f;
        float RepLossMultiplier = 1.0f;
        float RepLossDeclined = 1.0f;
        const char* TypeName() const override { return "CareerParams"; }
        const std::vector<ParameterMemberInfo>& Members() const override;
        std::optional<std::string> GetMemberText(std::string_view name) const override;
        void SetMemberValue(std::string_view name, const std::string& text) override;
    };

    // GameParameters.cs:250
    class AdvancedParams : public CustomParameterNode {
    public:
        bool AllowNegativeCurrency = false;
        bool PressurePartLimits = false;
        bool GPartLimits = false;
        bool GKerbalLimits = false;
        float KerbalGToleranceMult = 1.0f;
        bool ResourceTransferObeyCrossfeed = false;
        bool ActionGroupsAlways = false;
        float BuildingImpactDamageMult = 0.05f;
        bool PartUpgradesInSandbox = false;
        bool PartUpgradesInCareer = true;
        bool PartUpgradesInMission = false;
        bool EnableFullSASInSandbox = false;
        bool EnableFullSASInMissions = false;
        bool ImmediateLevelUp = false;  // auto-property

        // GameParameters.cs:317 (unset: HighLogic.CurrentGame == null -> true, else Mode == CAREER || MISSION)
        bool GetEnableKerbalExperience() const;
        void SetEnableKerbalExperience(bool value) { enableKerbalExperience = value; }
        // GameParameters.cs:342
        bool KerbalExperienceEnabled(Game_Modes gameMode) const;

        std::string Title() const override;
        std::string DisplaySection() const override { return "#autoLoc_6002170"; }
        std::string Section() const override { return "Advanced"; }
        int SectionOrder() const override { return 0; }
        GameMode GetGameMode() const override { return GameMode::ANY; }
        bool HasPresets() const override { return true; }
        void SetDifficultyPreset(Preset preset) override;  // GameParameters.cs:356
        bool Interactible(std::string_view member, const GameParameters& parameters) const override;  // :378
        void OnLoad(const ConfigNode& node) override;  // GameParameters.cs:394

        static constexpr const char* kTypeName = "AdvancedParams";
        const char* TypeName() const override { return kTypeName; }
        const std::vector<ParameterMemberInfo>& Members() const override;
        std::optional<std::string> GetMemberText(std::string_view name) const override;
        void SetMemberValue(std::string_view name, const std::string& text) override;

        std::optional<bool> enableKerbalExperience;  // private bool? in C#
    };

    // ---- GameParameters fields (GameParameters.cs:598-614) ----
    FlightParams Flight;
    EditorParams Editor;
    TrackingStationParams TrackingStation;
    SpaceCenterParams SpaceCenter;
    DifficultyParams Difficulty;
    CareerParams Career;
    Preset preset = Preset::Normal;

    // GameParameters.cs:625
    GameParameters();
    // GameParameters.cs:641
    explicit GameParameters(const ConfigNode& node);
    GameParameters(const GameParameters&) = delete;
    GameParameters& operator=(const GameParameters&) = delete;

    // GameParameters.cs:661 Load / 692 Save
    void Load(const ConfigNode& node);
    void Save(ConfigNode& node) const;

    // GameParameters.cs:714 CustomParams<T>() (ArgumentException when missing)
    template <class T>
    T& CustomParams() const {
        for (const auto& [type, node] : customParams_)
            if (type == T::kTypeName) return static_cast<T&>(*node);
        throw_missing(T::kTypeName);
    }
    // GameParameters.cs:723 CustomParams(Type) (nullptr + warning when missing)
    CustomParameterNode* CustomParams(std::string_view typeName) const;
    // the custom parameter nodes in dictionary (save) order
    std::vector<std::pair<std::string, CustomParameterNode*>> CustomParamsInOrder() const;

    // GameParameters.cs:759 SetDifficultyPresets / 840 GetDefaultParameters
    static void SetDifficultyPresets();
    static std::shared_ptr<GameParameters> GetDefaultParameters(Game_Modes mode, Preset p);
    static std::map<Preset, std::shared_ptr<GameParameters>>& DifficultyPresets();

    // ---- the static ParameterTypes list (GameParameters.cs:616 / 646) ----
    struct ParameterType {
        std::string name;  // Type.Name
        std::function<std::unique_ptr<CustomParameterNode>()> create;
    };
    // current order of the static list (discovery order until the first GameParameters is constructed)
    static std::vector<const ParameterType*>& ParameterTypes();
    // kspcore: put the static list back into discovery order (the state before the first construction; tests)
    static void ResetParameterTypes();

private:
    void LoadCustomNode(const ConfigNode& node);  // GameParameters.cs:735
    [[noreturn]] static void throw_missing(const char* name);
    std::vector<std::pair<std::string, std::unique_ptr<CustomParameterNode>>> customParams_;  // Dictionary order
};

namespace CommNet {
// CommNet/CommNetParams.cs
class CommNetParams : public GameParameters::CustomParameterNode {
public:
    bool requireSignalForControl = false;
    bool plasmaBlackout = false;
    float rangeModifier = 1.0f;
    float DSNModifier = 1.0f;
    float occlusionMultiplierVac = 0.9f;
    float occlusionMultiplierAtm = 0.75f;
    bool enableGroundStations = true;

    std::string Title() const override;  // Localizer.Format("#autoLOC_117012")
    std::string DisplaySection() const override;  // Localizer.Format("#autoLoc_6002170")
    std::string Section() const override { return "Advanced"; }
    int SectionOrder() const override { return 1; }
    GameParameters::GameMode GetGameMode() const override { return GameParameters::GameMode::ANY; }
    bool HasPresets() const override { return true; }
    bool Interactible(std::string_view member, const GameParameters& parameters) const override;
    void SetDifficultyPreset(GameParameters::Preset preset) override;

    static constexpr const char* kTypeName = "CommNetParams";
    const char* TypeName() const override { return kTypeName; }
    const std::vector<ParameterMemberInfo>& Members() const override;
    std::optional<std::string> GetMemberText(std::string_view name) const override;
    void SetMemberValue(std::string_view name, const std::string& text) override;
};
}  // namespace CommNet

namespace Expansions::Missions {
// Expansions/Missions/MissionParamsGeneral.cs
class MissionParamsGeneral : public GameParameters::CustomParameterNode {
public:
    bool enableFunding = false;
    float startingFunds = 100000.0f;
    std::string spacer = "";
    bool enableKerbalLevels = false;
    int kerbalLevelPilot = 4;
    int kerbalLevelScientist = 4;
    int kerbalLevelEngineer = 4;
    std::string spacer2 = "";
    bool preventVesselRecovery = false;

    std::string Title() const override { return "#autoLOC_8005035"; }
    std::string DisplaySection() const override { return "#autoLOC_8005013"; }
    std::string Section() const override { return "Mission"; }
    int SectionOrder() const override { return 0; }
    GameParameters::GameMode GetGameMode() const override { return GameParameters::GameMode::MISSION; }
    bool HasPresets() const override { return true; }
    bool Interactible(std::string_view member, const GameParameters& parameters) const override;
    void SetDifficultyPreset(GameParameters::Preset preset) override;

    static constexpr const char* kTypeName = "MissionParamsGeneral";
    const char* TypeName() const override { return kTypeName; }
    const std::vector<ParameterMemberInfo>& Members() const override;
    std::optional<std::string> GetMemberText(std::string_view name) const override;
    void SetMemberValue(std::string_view name, const std::string& text) override;
};

// Expansions/Missions/MissionParamsExtras.cs
class MissionParamsExtras : public GameParameters::CustomParameterNode {
public:
    bool facilityOpenAC = false;
    bool astronautHiresAreFree = true;
    std::string spacer = "";
    bool facilityOpenEditor = false;
    bool launchSitesOpen = false;
    std::string spacer2 = "";
    bool cheatsEnabled = false;

    std::string Title() const override { return "#autoLOC_8005024"; }
    std::string DisplaySection() const override { return "#autoLOC_8005013"; }
    std::string Section() const override { return "Mission"; }
    int SectionOrder() const override { return 2; }
    GameParameters::GameMode GetGameMode() const override { return GameParameters::GameMode::MISSION; }
    bool HasPresets() const override { return true; }
    bool Interactible(std::string_view member, const GameParameters& parameters) const override;
    void SetDifficultyPreset(GameParameters::Preset preset) override;

    static constexpr const char* kTypeName = "MissionParamsExtras";
    const char* TypeName() const override { return kTypeName; }
    const std::vector<ParameterMemberInfo>& Members() const override;
    std::optional<std::string> GetMemberText(std::string_view name) const override;
    void SetMemberValue(std::string_view name, const std::string& text) override;
};

// Expansions/Missions/MissionParamsFacilities.cs
class MissionParamsFacilities : public GameParameters::CustomParameterNode {
public:
    int facilityLevelAdmin = 3, facilityLevelAC = 3, facilityLevelLaunchpad = 3, facilityLevelMC = 3,
        facilityLevelRD = 3, facilityLevelRunway = 3, facilityLevelSPH = 3, facilityLevelTS = 3, facilityLevelVAB = 3;

    std::string Title() const override { return "#autoLOC_8005012"; }
    std::string DisplaySection() const override { return "#autoLOC_8005013"; }
    std::string Section() const override { return "Mission"; }
    int SectionOrder() const override { return 1; }
    GameParameters::GameMode GetGameMode() const override { return GameParameters::GameMode::MISSION; }
    bool HasPresets() const override { return true; }
    void SetDifficultyPreset(GameParameters::Preset preset) override;

    static constexpr const char* kTypeName = "MissionParamsFacilities";
    const char* TypeName() const override { return kTypeName; }
    const std::vector<ParameterMemberInfo>& Members() const override;
    std::optional<std::string> GetMemberText(std::string_view name) const override;
    void SetMemberValue(std::string_view name, const std::string& text) override;
};
}  // namespace Expansions::Missions

}  // namespace ksp
