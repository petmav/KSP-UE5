// WP-54 slice 2: Assembly-CSharp/Expansions/Missions/VesselRestriction*.cs, MissionPreflightCheck.cs, MissionBlackListPreFlightCheck.cs.
// A vessel restriction is a DynamicModule that tests the ship in the editor (EditorLogic.fetch.ship) against a mission requirement; the
// VesselRestrictionList is the DynamicModuleList of a VesselSituation.
//
// The editor ship comes from EditorLogic::fetch->ship (GetShipCosts / GetTotalMass / liveParts / CalculateCraftSize / StageManager) unless a
// RestrictionEnvironment hook is set; with neither, the C# `EditorLogic.fetch.ship` is a NullReferenceException. The UI list entry
// (UIListItem + UIStateImage) is modelled by `UIEntry` (its text and state index are observable). Event subscriptions call UpdateAppEntry
// like the C# callbacks.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/events/event_data.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/missions/dynamic_module.hpp"
#include "ksp/missions/mission_data.hpp"
#include "ksp/missions/mission_types.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {
class Part;
class ShipConstruct;
class VesselCrewManifest;
}  // namespace ksp

namespace ksp::Expansions::Missions {

class VesselSituation;
class MissionSituation;

struct RestrictionEnvironment {
    // EditorLogic.fetch.ship, per query (each unset hook falls back to EditorLogic::fetch->ship, else NullReferenceException)
    static inline std::function<float()> shipCost;              // GetShipCosts(out dry, out fuel, ShipConstruction.ShipManifest)
    static inline std::function<float()> shipTotalMass;         // GetTotalMass()
    static inline std::function<int()> shipPartCount;           // parts.Count
    static inline std::function<Vector3f()> shipSize;           // ShipConstruction.CalculateCraftSize(ship)
    static inline std::function<int()> stageCount;              // StageManager.StageCount
    static inline std::function<std::vector<std::string>()> shipPartNames;  // parts[i].name
    // parts[i].Resources[j]: (resourceName, amount) in part order then resource order
    static inline std::function<std::vector<std::pair<std::string, double>>()> shipResources;
    // CrewAssignmentDialog.Instance.CurrentManifestUnsafe?.CrewCount / ?.GetAllCrew(includeNulls: false) traits (nullopt = dialog or manifest null). Unset: derived
    // from crewManifest.
    static inline std::function<std::optional<int>()> crewCount;
    static inline std::function<std::optional<std::vector<std::string>>()> crewTraits;
    // CrewAssignmentDialog.Instance.CurrentManifestUnsafe (nullptr = dialog or manifest null)
    static inline std::function<VesselCrewManifest*()> crewManifest;
    // PartResourceLibrary.Instance.resourceDefinitions first entry's name (VesselRestriction_Resource constructors)
    static inline std::function<std::string()> firstResourceName;
    // PartLoader.getPartInfoByName(name).title ("" = unknown part -> the name is used)
    static inline std::function<std::string(const std::string&)> partTitle;
    // Time.time (VesselRestriction.GetStateMessage)
    static inline std::function<float()> time;
    // MissionsApp.Instance.CurrentVessel.vesselSituation (nullptr = no app / no current vessel / no situation)
    static inline std::function<VesselSituation*()> appCurrentVesselSituation;
    // MissionsApp.Instance.SetVesselItemReadyIcon(vesselSituation, info)
    static inline std::function<void(VesselSituation*)> setVesselItemReadyIcon;
    // BaseCrewAssignmentDialog.onCrewDialogChange (static event of the crew dialog)
    static inline events::EventData<VesselCrewManifest*> onCrewDialogChange{"BaseCrewAssignmentDialog.onCrewDialogChange"};
    static void Reset();
};

// VesselRestrictionOptions.cs:5 ([VesselRestrictionOptions(listedInSAP = false)] on VesselRestriction_RequiredParts)

// VesselRestriction.cs:10
class VesselRestriction : public DynamicModule {
public:
    // The UIListItem / UIStateImage the app attached (AddAppUIReference): the label text, the state index (0 = complete, 1 = incomplete) and
    // `UIEntry.Data is MissionsAppVesselInfo` with its vesselSituation.
    struct UIEntryRef {
        bool present = false;
        std::string text;
        int state = 0;
        bool dataIsVesselInfo = false;
        VesselSituation* vesselSituation = nullptr;
    };
    UIEntryRef UIEntry;                                      // :12 / :14

    explicit VesselRestriction(std::string typeName) : DynamicModule(std::move(typeName)) {}                       // :20
    VesselRestriction(std::string typeName, MENode* node) : DynamicModule(std::move(typeName), node) {}            // :24

    virtual void SuscribeToEvents() {}                       // :29
    virtual void ClearEvents() {}                            // :33
    std::string GetDisplayName() override;                   // :37
    void AddAppUIReference(bool dataIsVesselInfo, VesselSituation* vesselSituation);  // :42
    void ClearAppUIREference();                              // :49
    virtual void UpdateAppEntry();                           // :55
    virtual std::string GetStateMessage();                   // :70
    virtual bool IsComplete() { return true; }               // :75
    bool SameComparatorWrapper(VesselRestriction& otherRestriction);  // :80 (internal)
    virtual bool SameComparator(VesselRestriction& otherRestriction) { (void)otherRestriction; return true; }  // :89
    // :94 GetPreflightCheck(): new MissionPreflightCheck(this)
    static bool TestFloat(float currentValue, float targetValue, TestComparisonLessGreaterEqual comparisonOperator);  // :99
    static bool TestInt(int currentValue, int targetValue, TestComparisonLessGreaterEqual comparisonOperator);       // :115

    // the Callback_* delegates of the subclasses: every one only calls UpdateAppEntry()
    void CbPart(Part*) { UpdateAppEntry(); }
    void CbShip(ShipConstruct*) { UpdateAppEntry(); }
    void CbLoad(ShipConstruct*, ksp::KSP::UI::Screens::CraftBrowserDialog_LoadType) { UpdateAppEntry(); }
    void CbCrew(VesselCrewManifest*) { UpdateAppEntry(); }
    void CbVoid() { UpdateAppEntry(); }
    void CbInt(int) { UpdateAppEntry(); }
    VesselRestriction* self() { return this; }

private:
    static constexpr double epsilon = 1E-15;                 // :17
};

// VesselRestrictionList.cs:7
class VesselRestrictionList : public DynamicModuleList {
public:
    explicit VesselRestrictionList(MENode* node) : DynamicModuleList(node) {}  // :32
    std::vector<VesselRestriction*> ActiveRestrictions();    // :9
    std::vector<std::string> GetSupportedTypes() override;   // :37 (every VesselRestriction subclass except [listedInSAP = false])
    void StartAppEvents();                                   // :60
    void ClearAppEvents();                                   // :70
    std::unique_ptr<DynamicModuleList> Clone() const override;
};

#define KSP_RESTRICTION_COMMON(Cls)                                                                         \
    static constexpr const char* kTypeName = #Cls;                                                          \
    Cls() : DynamicModuleT<Cls, VesselRestriction>(kTypeName) { Init(); }                                   \
    explicit Cls(MENode* node) : DynamicModuleT<Cls, VesselRestriction>(kTypeName, node) { Init(); }

// VesselRestriction_Cost.cs:8
class VesselRestriction_Cost : public DynamicModuleT<VesselRestriction_Cost, VesselRestriction> {
public:
    KSP_RESTRICTION_COMMON(VesselRestriction_Cost)
    TestComparisonLessGreaterEqual comparisonOperator = TestComparisonLessGreaterEqual::GreaterOrEqual;  // :11
    float targetCost = 0.0f;                                 // :14 (private)
    void SuscribeToEvents() override;                        // :26
    void ClearEvents() override;                             // :34
    std::string GetDisplayName() override;                   // :42
    std::string GetStateMessage() override;                  // :47
    bool SameComparator(VesselRestriction& other) override;  // :64
    bool IsComplete() override;                              // :69
    void Load(const ConfigNode& node) override;              // :80
    void Save(ConfigNode& node) const override;              // :101
    float GetShipCurrentCost();                              // :74
private:
    void Init() {}
};

// VesselRestriction_Crew.cs:10
class VesselRestriction_Crew : public DynamicModuleT<VesselRestriction_Crew, VesselRestriction> {
public:
    enum class CrewRestrictionType { TotalCrew = 0, Engineers = 1, Scientists = 2, Pilots = 3 };  // :12
    KSP_RESTRICTION_COMMON(VesselRestriction_Crew)
    CrewRestrictionType crewType = CrewRestrictionType::TotalCrew;  // :27
    TestComparisonLessGreaterEqual comparisonOperator = TestComparisonLessGreaterEqual::GreaterOrEqual;  // :30
    int targetCrew = 0;                                      // :33
    void SuscribeToEvents() override;                        // :46
    void ClearEvents() override;                             // :53
    std::string GetDisplayName() override;                   // :60
    void Load(const ConfigNode& node) override;              // :65
    void Save(ConfigNode& node) const override;             // :87
    bool SameComparator(VesselRestriction& other) override;  // :95 (casts to VesselRestriction_Mass: InvalidCastException)
    bool IsComplete() override;                              // :100
    int GetCrewCount();                                      // :105
    std::string GetStateMessage() override;                  // :138
private:
    int GetCrewTraitCount(const std::string& kerbalTrait);   // :123
    void Init() { crewType = CrewRestrictionType::TotalCrew; }  // :36
};

// VesselRestriction_Mass.cs:8
class VesselRestriction_Mass : public DynamicModuleT<VesselRestriction_Mass, VesselRestriction> {
public:
    KSP_RESTRICTION_COMMON(VesselRestriction_Mass)
    TestComparisonLessGreaterEqual comparisonOperator = TestComparisonLessGreaterEqual::GreaterOrEqual;  // :11
    float targetMass = 0.0f;                                 // :14
    void SuscribeToEvents() override;                        // :26
    void ClearEvents() override;                             // :36
    std::string GetDisplayName() override;                   // :46
    void Load(const ConfigNode& node) override;              // :51
    void Save(ConfigNode& node) const override;              // :71
    bool SameComparator(VesselRestriction& other) override;  // :78
    bool IsComplete() override;                              // :83
    float GetCurrentMass();                                  // :88
    std::string GetStateMessage() override;                  // :93
private:
    void Init() {}
};

// VesselRestriction_PartCount.cs:8
class VesselRestriction_PartCount : public DynamicModuleT<VesselRestriction_PartCount, VesselRestriction> {
public:
    KSP_RESTRICTION_COMMON(VesselRestriction_PartCount)
    TestComparisonLessGreaterEqual comparisonOperator = TestComparisonLessGreaterEqual::GreaterOrEqual;  // :11
    int targetParts = 0;                                     // :14
    void SuscribeToEvents() override;                        // :24
    void ClearEvents() override;                             // :32
    std::string GetDisplayName() override;                   // :40
    void Load(const ConfigNode& node) override;              // :45
    void Save(ConfigNode& node) const override;              // :65
    bool SameComparator(VesselRestriction& other) override;  // :72
    bool IsComplete() override;                              // :77
    std::string GetStateMessage() override;                  // :87
private:
    int GetCurrentParts();                                   // :82
    void Init() {}
};

// VesselRestriction_RequiredParts.cs:9 ([VesselRestrictionOptions(listedInSAP = false)])
class VesselRestriction_RequiredParts : public DynamicModuleT<VesselRestriction_RequiredParts, VesselRestriction> {
public:
    KSP_RESTRICTION_COMMON(VesselRestriction_RequiredParts)
    void SuscribeToEvents() override;                        // :22
    void ClearEvents() override;                             // :30
    std::string GetDisplayName() override;                   // :38
    void Load(const ConfigNode& node) override;              // :43 (empty)
    void Save(ConfigNode& node) const override;              // :47 (empty)
    bool IsComplete() override;                              // :51
    std::string GetStateMessage() override;                  // :75
private:
    std::vector<std::string> requiredPartsInUse;             // :11
    bool requiredPartsInUseSet = false;
    void Init() {}
};

// VesselRestriction_Resource.cs:9
class VesselRestriction_Resource : public DynamicModuleT<VesselRestriction_Resource, VesselRestriction> {
public:
    KSP_RESTRICTION_COMMON(VesselRestriction_Resource)
    std::string resourceName;                                // :12
    TestComparisonLessGreaterEqual comparisonOperator = TestComparisonLessGreaterEqual::GreaterOrEqual;  // :15
    float targetAmount = 0.0f;                               // :18
    void SuscribeToEvents() override;                        // :36
    void ClearEvents() override;                             // :46
    std::string GetDisplayName() override;                   // :56
    void Load(const ConfigNode& node) override;              // :61
    void Save(ConfigNode& node) const override;              // :84
    bool SameComparator(VesselRestriction& other) override;  // :92
    bool IsComplete() override;                              // :97
    double GetCurrentResourceValue();                        // :102
    std::string GetStateMessage() override;                  // :120
private:
    void Init();                                             // :21 / :28
};

// VesselRestriction_Size.cs:10
class VesselRestriction_Size : public DynamicModuleT<VesselRestriction_Size, VesselRestriction> {
public:
    KSP_RESTRICTION_COMMON(VesselRestriction_Size)
    TestComparisonLessGreaterEqual comparisonOperator = TestComparisonLessGreaterEqual::GreaterOrEqual;  // :13
    float tempInputY = 0.0f;                                 // :16
    float tempInputX = 0.0f;                                 // :19
    float tempInputZ = 0.0f;                                 // :22
    Vector3f targetSize;                                     // :24
    void SuscribeToEvents() override;                        // :36
    void ClearEvents() override;                             // :44
    std::string GetDisplayName() override;                   // :52
    void Load(const ConfigNode& node) override;              // :57
    void Save(ConfigNode& node) const override;              // :86
    bool SameComparator(VesselRestriction& other) override;  // :95
    bool IsComplete() override;                              // :100
    std::string GetStateMessage() override;                  // :119
private:
    bool Check(const Vector3f& currentSize, const Vector3f& targetSize, TestComparisonLessGreaterEqual op);  // :105
    Vector3f GetShipSize();                                  // :114
    std::string GetShipDimensionsString(const Vector3f& dimensions);  // :138
    void Init();                                             // :26 / :31
};

// VesselRestriction_Stages.cs:8
class VesselRestriction_Stages : public DynamicModuleT<VesselRestriction_Stages, VesselRestriction> {
public:
    KSP_RESTRICTION_COMMON(VesselRestriction_Stages)
    TestComparisonLessGreaterEqual comparisonOperator = TestComparisonLessGreaterEqual::GreaterOrEqual;  // :11
    int targetStages = 0;                                    // :14
    void SuscribeToEvents() override;                        // :24
    void ClearEvents() override;                             // :35
    std::string GetDisplayName() override;                   // :46
    void Load(const ConfigNode& node) override;              // :51
    void Save(ConfigNode& node) const override;              // :71
    bool SameComparator(VesselRestriction& other) override;  // :78
    bool IsComplete() override;                              // :83
    std::string GetStateMessage() override;                  // :93
private:
    int GetCurrentStageCount();                              // :88
    void Init() {}
};

// MissionPreflightCheck.cs:6 (IPreFlightTest)
class MissionPreflightCheck {
public:
    explicit MissionPreflightCheck(VesselRestriction* vesselRestriction) : vesselRestriction(vesselRestriction) {}  // :10
    bool Test();                                             // :15
    std::string GetWarningTitle();                           // :20
    std::string GetWarningDescription();                     // :25
    std::string GetProceedOption();                          // :30 (null: empty)
    std::string GetAbortOption();                            // :35
    bool HasProceedOption() const { return false; }
private:
    VesselRestriction* vesselRestriction;
};

// MissionBlackListPreFlightCheck.cs:8 (IPreFlightTest)
class MissionBlackListPreFlightCheck {
public:
    // `ship.parts[i].partInfo.name` for every part of the ship; `missionSituation` = the first mission's situation when vesselSituation is null
    // (MissionSystem.missions[0].situation; nullptr = none)
    MissionBlackListPreFlightCheck(VesselSituation* situation, const std::vector<std::string>& shipPartNames, MissionSituation* firstMissionSituation);  // :17
    bool Test();                                             // :70
    std::string GetWarningTitle();                           // :79
    std::string GetWarningDescription();                     // :84
    std::string GetProceedOption();                          // :95
    std::string GetAbortOption();                            // :100
    // the Dictionary<AvailablePart, int> keyed by part name, in insertion order
    std::vector<std::pair<std::string, int>> blackListedPartsOnShip;

private:
    VesselSituation* vesselSituation = nullptr;
    std::vector<std::string> missionExcludedParts;           // :12
    bool missionCheckReqd = false;                           // :14
};

// Registers the VesselRestriction family with DynamicModuleRegistry (idempotent).
void RegisterVesselRestrictions();

}  // namespace ksp::Expansions::Missions

template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::VesselRestriction_Crew::CrewRestrictionType> {
    using E = ksp::Expansions::Missions::VesselRestriction_Crew::CrewRestrictionType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::TotalCrew, "TotalCrew", "#autoLOC_8005005"},
                                                {E::Engineers, "Engineers", "#autoLOC_8005008"},
                                                {E::Scientists, "Scientists", "#autoLOC_8005007"},
                                                {E::Pilots, "Pilots", "#autoLOC_8005006"}};
};
