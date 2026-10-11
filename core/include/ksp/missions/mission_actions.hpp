// WP-55: Assembly-CSharp/Expansions/Missions/Actions/*.cs (Expansions.Missions.Actions) and VesselPartIDPair.cs. See node_library.hpp.
//
// Ported here: ActionMessage, ActionDialogMessage, ActionModifierFunds (+ FundsModifierType), ActionMissionScore, ActionModifierScore,
// ActionVessel, ActionVesselActionGroup, ActionVesselExplode, ActionVesselStage, ActionSetCamera, ActionPartResourceAmount, ActionPartExplode.
// The Fire() coroutines are C++20 coroutines (ksp/core/coroutine.hpp); `yield return null` is co_yield YieldInstruction::Null().
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ksp/missions/mission.hpp"
#include "ksp/missions/mission_node.hpp"
#include "ksp/missions/mission_score.hpp"
#include "ksp/missions/node_library.hpp"
#include "ksp/save/save_enums.hpp"

namespace ksp::Expansions::Missions {

// VesselPartIDPair.cs:8 (the (vessel, part) pair of the part actions)
class VesselPartIDPair {
public:
    uint32_t VesselID = 0;                                   // :11
    uint32_t partID = 0;                                     // :14
    std::string partName = "#autoLOC_8001004";               // :17
    VesselPartIDPair() = default;                            // :19
    bool Equals(const VesselPartIDPair& other) const;        // :26
    void ValidatePartAgainstCraft(MENode* node, Editor::MissionEditorValidator* validator);  // :45
    void Save(ConfigNode& node) const;                       // :72
    void Load(const ConfigNode& node);                       // :79
};

}  // namespace ksp::Expansions::Missions

namespace ksp::Expansions::Missions::Actions {

// FundsModifierType.cs:5
enum class FundsModifierType { Add = 0, Multiply = 1, Divide = 2, Substract = 3, Set = 4 };

// ActionMessage.cs:7
class ActionMessage : public ActionModule {
public:
    static constexpr const char* kTypeName = "ActionMessage";
    ActionMessage();                                         // :18 Awake
    std::string message;                                     // :11
    float duration = 5.0f;                                   // :14
    ScreenMessageStyle style = ScreenMessageStyle::UPPER_LEFT;  // :16
    Enumerator Fire() override;                              // :27
    std::string GetInfo() override;                          // :33
    void Save(ConfigNode& node) const override;              // :38
    void Load(const ConfigNode& node) override;              // :48
};

// ActionDialogMessage.cs:12
class ActionDialogMessage : public ActionModule {
public:
    enum class DialogMessageArea { Left = 0, Center = 1, Right = 2 };  // :14
    static constexpr const char* kTypeName = "ActionDialogMessage";
    ActionDialogMessage();                                   // :53 Awake
    std::string messageHeading;                              // :27
    std::string message;                                     // :30
    MissionInstructor missionInstructor;                     // :33
    int textAreaSize = 135;                                  // :36
    DialogMessageArea screenArea = DialogMessageArea::Center;  // :39
    bool pauseMission = false;                               // :42
    bool autoClose = false;                                  // :45
    int autoCloseTimeOut = 20;                               // :48
    bool isBriefingMessage = false;                          // :50
    bool autoGrowDialogHeight = false;                       // :52
    void Initialize(MENode* node) override;                  // :67
    Enumerator Fire() override;                              // :72
    std::string GetInfo() override;                          // :146
    void ParameterSetupComplete() override;                  // :151
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :157
    void Save(ConfigNode& node) const override;              // :166
    void Load(const ConfigNode& node) override;              // :189

protected:
    void SendStateMessage(const std::string& title, const std::string& message);  // :127

private:
    void dialogClosed();                                     // :118
};

// ActionModifierFunds.cs:7
class ActionModifierFunds : public ActionModule {
public:
    static constexpr const char* kTypeName = "ActionModifierFunds";
    ActionModifierFunds();                                   // :15 Awake
    FundsModifierType modifierType = FundsModifierType::Add;  // :11
    float modifierFunds = 0.0f;                              // :13
    Enumerator Fire() override;                              // :23
    std::string GetInfo() override;                          // :55
    void Load(const ConfigNode& node) override;              // :60
    void Save(ConfigNode& node) const override;              // :67
};

// ActionMissionScore.cs:8
class ActionMissionScore : public ActionModule, public IActionMissionScore {
public:
    static constexpr const char* kTypeName = "ActionMissionScore";
    ActionMissionScore();                                    // :15 Awake
    std::unique_ptr<MissionScore> score;                     // :11
    bool IsA(std::string_view n) const override { return n == "ActionMissionScore" || ActionModule::IsA(n); }
    void Initialize(MENode* node) override;                  // :21
    Enumerator Fire() override;                              // :31
    std::string GetScoreDescription() override;              // :40
    std::string GetAwardedScoreDescription() override;       // :45
    std::string GetAppObjectiveInfo() override;              // :54 (BaseAPFieldList reflection over the fields: only the MissionScore field exists here)
    std::vector<IMENodeDisplay*> GetInternalParametersToDisplay() override;  // :78
    std::string GetInfo() override;                          // :97
    void Save(ConfigNode& node) const override;              // :102
    void Load(const ConfigNode& node) override;              // :108

protected:
    explicit ActionMissionScore(std::string typeName);       // Awake for a subclass (GetType().Name)
};

// ActionModifierScore.cs:3
class ActionModifierScore : public ActionMissionScore {
public:
    static constexpr const char* kTypeName = "ActionModifierScore";
    ActionModifierScore();
    bool IsA(std::string_view n) const override { return n == "ActionModifierScore" || ActionMissionScore::IsA(n); }
    void Initialize(MENode* node) override;                  // :5
};

// ActionVessel.cs:6
class ActionVessel : public ActionModule {
public:
    explicit ActionVessel(std::string typeName);             // :43 Awake
    uint32_t vesselID = 0;                                   // :11
    bool IsA(std::string_view n) const override { return n == "ActionVessel" || ActionModule::IsA(n); }
    Vessel* vessel();                                        // :18 (property)
    void OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) override;  // :48
    void OnVesselsDocking(uint32_t oldId, uint32_t newId) override;             // :56
    void OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) override;     // :69
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :86
    void Save(ConfigNode& node) const override;              // :114
    void Load(const ConfigNode& node) override;              // :124

protected:
    bool useActiveVessel = false;                            // :13

private:
    Vessel* vessel_ = nullptr;                               // :15
    uint32_t oldRootPart = 0;                                // :17
};

// ActionVesselActionGroup.cs:7
class ActionVesselActionGroup : public ActionModule {
public:
    static constexpr const char* kTypeName = "ActionVesselActionGroup";
    ActionVesselActionGroup();                               // :13 Awake
    Enumerator Fire() override;                              // :24
    std::string GetInfo() override;                          // :34
    void Save(ConfigNode& node) const override;              // :39
    void Load(const ConfigNode& node) override;             // :45
    KSPActionGroup actionGroup = KSPActionGroup::None;  // :11 (private in C#)
};

// ActionVesselExplode.cs:8
class ActionVesselExplode : public ActionVessel {
public:
    static constexpr const char* kTypeName = "ActionVesselExplode";
    ActionVesselExplode();                                   // :16 Awake
    float partsExplodeTime = 0.0f;                           // :13
    Enumerator Fire() override;                              // :24
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :50
    std::string GetInfo() override;                          // :59
    void Save(ConfigNode& node) const override;              // :64
    void Load(const ConfigNode& node) override;              // :70
};

// ActionVesselStage.cs:8
class ActionVesselStage : public ActionModule {
public:
    static constexpr const char* kTypeName = "ActionVesselStage";
    ActionVesselStage();                                     // :10 Awake
    Enumerator Fire() override;                              // :16
    std::string GetInfo() override;                          // :27
    void Save(ConfigNode& node) const override;              // :32
    void Load(const ConfigNode& node) override;              // :37
};

// ActionSetCamera.cs:6
class ActionSetCamera : public ActionModule {
public:
    static constexpr const char* kTypeName = "ActionSetCamera";
    ActionSetCamera();                                       // :15 Awake
    MissionCameraModeOptions cameraLockMode = MissionCameraModeOptions::NoChange;  // :9
    MissionCameraLockOptions cameraLockOptions = MissionCameraLockOptions::NoChange;  // :12
    Enumerator Fire() override;                              // :21
    std::string GetInfo() override;                          // :27
    void Save(ConfigNode& node) const override;              // :32
    void Load(const ConfigNode& node) override;              // :39
};

// ActionPartResourceAmount.cs:9
class ActionPartResourceAmount : public ActionVessel {
public:
    static constexpr const char* kTypeName = "ActionPartResourceAmount";
    ActionPartResourceAmount();                              // :20 Awake
    std::string resourceName;                                // :12
    float adjustAmount = 0.0f;                               // :15
    Enumerator Fire() override;                              // :27
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :145
    std::string GetInfo() override;                          // :163
    void Save(ConfigNode& node) const override;              // :168
    void Load(const ConfigNode& node) override;              // :175

private:
    double adjustUnloadedPart(ProtoPartSnapshot* InProtoPart, double adjustAmount, bool pulling);  // :75
};

// ActionPartExplode.cs:7
class ActionPartExplode : public ActionModule {
public:
    static constexpr const char* kTypeName = "ActionPartExplode";
    ActionPartExplode();                                     // :55 Awake
    VesselPartIDPair vesselPartIDs;                          // :10
    void OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) override;  // :13
    void OnPartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId) override;  // :22
    void OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) override;     // :31
    Enumerator Fire() override;                              // :63
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :105
    std::string GetInfo() override;                          // :114
    void RunValidation(Editor::MissionEditorValidator* validator) override;     // :119
    void Save(ConfigNode& node) const override;              // :127
    void Load(const ConfigNode& node) override;              // :133
};

}  // namespace ksp::Expansions::Missions::Actions

// ---- enum metadata (declaration order, [Description] text) -------------------------------------------------------------------
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::ScreenMessageStyle> {
    using E = ksp::Expansions::Missions::ScreenMessageStyle;
    static constexpr bool flags = false;
    // ScreenMessageStyle.cs:3
    static constexpr EnumMember<E> members[] = {{E::UPPER_CENTER, "UPPER_CENTER", "#autoLOC_8000036"},
                                                {E::UPPER_LEFT, "UPPER_LEFT", "#autoLOC_8000037"},
                                                {E::UPPER_RIGHT, "UPPER_RIGHT", "#autoLOC_8000038"},
                                                {E::LOWER_CENTER, "LOWER_CENTER", "#autoLOC_8000039"},
                                                {E::KERBAL_EVA, "KERBAL_EVA", "#autoLOC_8320086"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Actions::FundsModifierType> {
    using E = ksp::Expansions::Missions::Actions::FundsModifierType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Add, "Add", "#autoLOC_8003126"},
                                                {E::Multiply, "Multiply", "#autoLOC_8100127"},
                                                {E::Divide, "Divide", "#autoLOC_8100128"},
                                                {E::Substract, "Substract", "#autoLOC_8003127"},
                                                {E::Set, "Set", "#autoLOC_8003128"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Actions::ActionDialogMessage::DialogMessageArea> {
    using E = ksp::Expansions::Missions::Actions::ActionDialogMessage::DialogMessageArea;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {
        {E::Left, "Left", "#autoLOC_8002036"}, {E::Center, "Center", "#autoLOC_8002037"}, {E::Right, "Right", "#autoLOC_8002038"}};
};
