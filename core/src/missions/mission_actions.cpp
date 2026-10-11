// WP-55: Assembly-CSharp/Expansions/Missions/Actions/*.cs and VesselPartIDPair.cs. See mission_actions.hpp and node_library.hpp.
// Float rule (README "Mono float32 rule"): managed float expressions are f32store(fx(a) op fx(b)).
#include "ksp/missions/mission_actions.hpp"

#include <algorithm>
#include <cmath>

#include "../save/save_util.hpp"
#include "ksp/career/funding.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/missions/mission_awards.hpp"
#include "ksp/missions/mission_validator.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Missions {

using KSP::Localization::Localizer;

namespace {
std::string ReplaceAll(std::string s, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}
std::string Tag(const char* t) { return Localizer::Format(t); }
}  // namespace

// ---- VesselPartIDPair --------------------------------------------------------------------------------------------------------

// VesselPartIDPair.cs:26
bool VesselPartIDPair::Equals(const VesselPartIDPair& other) const { return VesselID == other.VesselID && partID == other.partID; }

// VesselPartIDPair.cs:45
void VesselPartIDPair::ValidatePartAgainstCraft(MENode* node, Editor::MissionEditorValidator* validator) {
    if (partID == 0 || !(node != nullptr)) return;
    // MissionCraft craft = node.mission.GetCraftBySituationsVesselID(VesselID); ShipConstruct ship = MissionEditorLogic.Instance.LoadVessel(craft)
    // (craft files / editor: seam, nullopt = no craft or no ship)
    if (!NodeLibraryEnvironment::craftPartIds) return;
    std::optional<std::vector<uint32_t>> parts = NodeLibraryEnvironment::craftPartIds(node, VesselID);
    if (!parts.has_value()) return;
    bool flag = true;
    for (size_t i = 0; i < parts->size(); ++i) {
        if ((*parts)[i] == partID) {
            flag = false;
            break;
        }
    }
    if (flag) validator->AddNodeFail(node, Tag("#autoLOC_8002115"));
}

// VesselPartIDPair.cs:72
void VesselPartIDPair::Save(ConfigNode& node) const {
    node.AddValue("partPersistentId", partID);
    node.AddValue("vesselPersistentId", VesselID);
    node.AddValue("partName", partName);
}

// VesselPartIDPair.cs:79
void VesselPartIDPair::Load(const ConfigNode& node) {
    node.TryGetValue("partPersistentId", partID);
    node.TryGetValue("vesselPersistentId", VesselID);
    node.TryGetValue("partName", partName);
}

}  // namespace ksp::Expansions::Missions

namespace ksp::Expansions::Missions::Actions {

using KSP::Localization::Localizer;

namespace {
double epsilon = 1E-15;  // ActionPartResourceAmount.cs:17
std::string Tag(const char* t) { return Localizer::Format(t); }
std::string ReplaceAll(std::string s, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}
}  // namespace

// ---- ActionMessage -----------------------------------------------------------------------------------------------------------

// ActionMessage.cs:18 Awake
ActionMessage::ActionMessage() : ActionModule(kTypeName) {
    title = Tag("#autoLOC_8000031");
    message = "";
    duration = 5.0f;
    style = ScreenMessageStyle::UPPER_LEFT;
}
// ActionMessage.cs:27
Enumerator ActionMessage::Fire() {
    NodeLibraryEnvironment::ScreenMessage m;
    m.text = message;
    m.duration = duration;
    m.style = style;
    m.persist = true;
    if (NodeLibraryEnvironment::postScreenMessage) {
        NodeLibraryEnvironment::postScreenMessage(m);
    } else {
        NodeLibraryEnvironment::screenMessages.push_back(m);
    }
    co_return;
}
// ActionMessage.cs:33
std::string ActionMessage::GetInfo() { return Tag("#autoLOC_8004021"); }
// ActionMessage.cs:38
void ActionMessage::Save(ConfigNode& node) const {
    ActionModule::Save(node);
    std::string text = ReplaceAll(message, "\n", "\\n");
    text = ReplaceAll(text, "\t", "\\t");
    node.AddValue("message", text);
    node.AddValue("duration", duration);
    node.AddValue("style", dotnet::EnumToString(style));
}
// ActionMessage.cs:48
void ActionMessage::Load(const ConfigNode& node) {
    ActionModule::Load(node);
    node.TryGetValue("message", message);
    message = ReplaceAll(message, "\\n", "\n");
    message = ReplaceAll(message, "\\t", "\t");
    node.TryGetValue("duration", duration);
    save_detail::TryGetEnum(node, "style", style, ScreenMessageStyle::UPPER_LEFT);
}

// ---- ActionDialogMessage -----------------------------------------------------------------------------------------------------

// ActionDialogMessage.cs:53 Awake
ActionDialogMessage::ActionDialogMessage() : ActionModule(kTypeName) {
    title = Tag("#autoLOC_8006007");
    messageHeading = "";
    message = "";
    textAreaSize = 135;
    pauseMission = false;
    screenArea = DialogMessageArea::Center;
    autoClose = false;
    autoCloseTimeOut = 20;
    missionInstructor = MissionInstructor();
}
// ActionDialogMessage.cs:67
void ActionDialogMessage::Initialize(MENode* n) { ActionModule::Initialize(n); }
// ActionDialogMessage.cs:72
Enumerator ActionDialogMessage::Fire() {
    if (pauseMission) {
        node->isPaused = true;
    } else {
        node->isPaused = false;
    }
    std::string instructorName = missionInstructor.instructorName;
    instructorName = ReplaceAll(instructorName, "Instructor_", "GAPKerbal_");
    instructorName = ReplaceAll(instructorName, "Strategy_", "GAPKerbal_");
    std::string instructorName2 = "";
    if (instructorName.find("Veteran") != std::string::npos) {
        if (missionInstructor.vintageSuit) instructorName += "_Vintage";
        if (instructorName.find("Jeb_") != std::string::npos) {
            instructorName2 = Tag("#autoLOC_20803");
            instructorName = ReplaceAll(instructorName, "Jeb_", "");
        } else if (instructorName.find("Bill_") != std::string::npos) {
            instructorName2 = Tag("#autoLOC_20811");
            instructorName = ReplaceAll(instructorName, "Bill_", "");
        } else if (instructorName.find("Bob_") != std::string::npos) {
            instructorName2 = Tag("#autoLOC_20819");
            instructorName = ReplaceAll(instructorName, "Bob_", "");
        } else if (instructorName.find("Val_") != std::string::npos) {
            instructorName2 = Tag("#autoLOC_20827");
            instructorName = ReplaceAll(instructorName, "Val_", "");
        }
    }
    // MessageNodeDialog.Spawn(messageHeading, message, instructorName, "#autoLOC_8000259", dialogClosed, textAreaSize, screenArea, autoClose,
    //                         autoCloseTimeOut, autoGrowDialogHeight, instructorName2)
    NodeLibraryEnvironment::DialogMessage d;
    d.heading = messageHeading;
    d.message = message;
    d.instructorName = instructorName;
    d.buttonLabel = "#autoLOC_8000259";
    d.dialogClosed = [this] { dialogClosed(); };
    d.textAreaSize = textAreaSize;
    d.screenArea = static_cast<int>(screenArea);
    d.autoClose = autoClose;
    d.autoCloseTimeOut = autoCloseTimeOut;
    d.autoGrowDialogHeight = autoGrowDialogHeight;
    d.instructorName2 = instructorName2;
    if (NodeLibraryEnvironment::spawnMessageDialog) {
        NodeLibraryEnvironment::spawnMessageDialog(std::move(d));
    } else {
        NodeLibraryEnvironment::dialogMessages.push_back(std::move(d));
    }
    SendStateMessage(messageHeading, message);
    co_return;
}
// ActionDialogMessage.cs:118
void ActionDialogMessage::dialogClosed() {
    node->isPaused = false;
    if (isBriefingMessage) node->mission->briefingNodeActive = false;
}
// ActionDialogMessage.cs:127 (MessageSystem.Instance != null: the seam; the Debug.Log line is dropped)
void ActionDialogMessage::SendStateMessage(const std::string& title_, const std::string& message_) {
    if (NodeLibraryEnvironment::addMessageSystemMessage) {
        NodeLibraryEnvironment::addMessageSystemMessage(title_, message_);
    } else {
        NodeLibraryEnvironment::systemMessages.emplace_back(title_, message_);
    }
}
// ActionDialogMessage.cs:146
std::string ActionDialogMessage::GetInfo() { return Tag("#autoLOC_8004012"); }
// ActionDialogMessage.cs:151 (OnautoClose(autoClose) only toggles a UI control)
void ActionDialogMessage::ParameterSetupComplete() { ActionModule::ParameterSetupComplete(); }
// ActionDialogMessage.cs:157
std::string ActionDialogMessage::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "missionInstructor") return missionInstructor.GetNodeBodyParameterString(&field);
    return ActionModule::GetNodeBodyParameterString(field);
}
// ActionDialogMessage.cs:166
void ActionDialogMessage::Save(ConfigNode& node_) const {
    ActionModule::Save(node_);
    if (!messageHeading.empty()) node_.AddValue("messageHeading", messageHeading);
    if (!message.empty()) {
        std::string text = ReplaceAll(message, "\n", "\\n");
        text = ReplaceAll(text, "\t", "\\t");
        node_.AddValue("message", text);
    }
    missionInstructor.Save(node_);
    node_.AddValue("textAreaSize", textAreaSize);
    node_.AddValue("pauseMission", pauseMission);
    node_.AddValue("screenArea", dotnet::EnumToString(screenArea));
    node_.AddValue("autoClose", autoClose);
    node_.AddValue("autoCloseTimeOut", autoCloseTimeOut);
    node_.AddValue("isBriefingMessage", isBriefingMessage);
    node_.AddValue("autoGrowDialogHeight", autoGrowDialogHeight);
}
// ActionDialogMessage.cs:189
void ActionDialogMessage::Load(const ConfigNode& node_) {
    ActionModule::Load(node_);
    node_.TryGetValue("messageHeading", messageHeading);
    node_.TryGetValue("message", message);
    message = ReplaceAll(message, "\\n", "\n");
    message = ReplaceAll(message, "\\t", "\t");
    node_.TryGetValue("textAreaSize", textAreaSize);
    node_.TryGetValue("pauseMission", pauseMission);
    save_detail::TryGetEnum(node_, "screenArea", screenArea, DialogMessageArea::Center);
    node_.TryGetValue("autoClose", autoClose);
    node_.TryGetValue("autoCloseTimeOut", autoCloseTimeOut);
    node_.TryGetValue("isBriefingMessage", isBriefingMessage);
    missionInstructor.Load(node_);
    if (!node_.TryGetValue("autoGrowDialogHeight", autoGrowDialogHeight)) node_.TryGetValue("maxDialogHeight", autoGrowDialogHeight);
}

// ---- ActionModifierFunds -----------------------------------------------------------------------------------------------------

// ActionModifierFunds.cs:15 Awake
ActionModifierFunds::ActionModifierFunds() : ActionModule(kTypeName) {
    title = "#autoLOC_8003125";
    modifierType = FundsModifierType::Add;
    modifierFunds = 0.0f;
}
// ActionModifierFunds.cs:23
Enumerator ActionModifierFunds::Fire() {
    if (!(Funding::Instance == nullptr)) {
        switch (modifierType) {
            default: Funding::Instance->AddFunds(fx(modifierFunds), TransactionReasons::Mission); break;
            case FundsModifierType::Multiply: {
                double num = Funding::Instance->Funds() * fx(modifierFunds);
                Funding::Instance->AddFunds(num - Funding::Instance->Funds(), TransactionReasons::Mission);
                break;
            }
            case FundsModifierType::Divide: {
                double num = Funding::Instance->Funds() / fx(modifierFunds);
                Funding::Instance->AddFunds(num - Funding::Instance->Funds(), TransactionReasons::Mission);
                break;
            }
            case FundsModifierType::Substract:
                Funding::Instance->AddFunds(fx(f32store(0.0 - fx(modifierFunds))), TransactionReasons::Mission);
                break;
            case FundsModifierType::Set:
                Funding::Instance->AddFunds(fx(modifierFunds) - Funding::Instance->Funds(), TransactionReasons::Mission);
                break;
        }
    }
    co_return;
}
// ActionModifierFunds.cs:55
std::string ActionModifierFunds::GetInfo() { return Tag("#autoLOC_8003124"); }
// ActionModifierFunds.cs:60
void ActionModifierFunds::Load(const ConfigNode& node) {
    ActionModule::Load(node);
    save_detail::TryGetEnum(node, "type", modifierType, FundsModifierType::Multiply);
    node.TryGetValue("modifier", modifierFunds);
}
// ActionModifierFunds.cs:67
void ActionModifierFunds::Save(ConfigNode& node) const {
    ActionModule::Save(node);
    node.AddValue("type", dotnet::EnumToString(modifierType));
    node.AddValue("modifier", modifierFunds);
}

// ---- ActionMissionScore / ActionModifierScore --------------------------------------------------------------------------------

// ActionMissionScore.cs:15 Awake
ActionMissionScore::ActionMissionScore() : ActionMissionScore(kTypeName) {}
ActionMissionScore::ActionMissionScore(std::string typeName) : ActionModule(std::move(typeName)) { title = Tag("#autoLOC_8000078"); }
// ActionMissionScore.cs:21
void ActionMissionScore::Initialize(MENode* n) {
    ActionModule::Initialize(n);
    score = std::make_unique<MissionScore>(n);
    if (score->activeModules.size() == 0) score->activeModules.push_back(std::make_unique<ScoreModule_Completion>(n));
}
// ActionMissionScore.cs:31
Enumerator ActionMissionScore::Fire() {
    if (node->mission->isScoreEnabled) score->AwardScore(node->mission);
    co_yield YieldInstruction::Null();
}
// ActionMissionScore.cs:40
std::string ActionMissionScore::GetScoreDescription() { return score->ScoreDescription(); }
// ActionMissionScore.cs:45
std::string ActionMissionScore::GetAwardedScoreDescription() {
    if (!node->mission->isScoreEnabled) return "";
    return score->AwarededScoreDescription();
}
// ActionMissionScore.cs:54 (the BaseAPFieldList of the MEGUI fields holds the MissionScore field only)
std::string ActionMissionScore::GetAppObjectiveInfo() {
    std::string text;
    if (score != nullptr) text += score->ScoreDescription() + "\n";
    return text;
}
// ActionMissionScore.cs:78
std::vector<IMENodeDisplay*> ActionMissionScore::GetInternalParametersToDisplay() { return score->GetInternalParameters(); }
// ActionMissionScore.cs:97
std::string ActionMissionScore::GetInfo() { return Tag("#autoLOC_8004002"); }
// ActionMissionScore.cs:102
void ActionMissionScore::Save(ConfigNode& node_) const {
    ActionModule::Save(node_);
    score->Save(node_);
}
// ActionMissionScore.cs:108
void ActionMissionScore::Load(const ConfigNode& node_) {
    ActionModule::Load(node_);
    score->Load(node_);
}

ActionModifierScore::ActionModifierScore() : ActionMissionScore(kTypeName) {}
// ActionModifierScore.cs:5
void ActionModifierScore::Initialize(MENode* n) {
    node = n;
    score = std::make_unique<MissionScore>(n);
    if (score->activeModules.size() == 0) score->activeModules.push_back(std::make_unique<ScoreModule_Modifier>(n));
}

// ---- ActionVessel ------------------------------------------------------------------------------------------------------------

// ActionVessel.cs:43 Awake
ActionVessel::ActionVessel(std::string typeName) : ActionModule(std::move(typeName)) {}
// ActionVessel.cs:18
Vessel* ActionVessel::vessel() {
    if (vessel_ == nullptr) {
        if (vesselID == 0) {
            if (AwardEnvironment::ActiveVessel() != nullptr) vessel_ = AwardEnvironment::ActiveVessel();
        } else {
            uint32_t key = node->mission->CurrentVesselID(node, vesselID);
            if (Vessel* v = AwardEnvironment::VesselByPersistentId(key); v != nullptr) vessel_ = v;
        }
    }
    return vessel_;
}
// ActionVessel.cs:48
void ActionVessel::OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) {
    if (vesselID == oldId) vesselID = newId;
}
// ActionVessel.cs:56
void ActionVessel::OnVesselsDocking(uint32_t oldId, uint32_t) {
    if (oldId == vesselID) {
        Vessel* v = nullptr;
        if (AwardEnvironment::findVessel) {
            AwardEnvironment::findVessel(oldId, v);
        } else {
            v = AwardEnvironment::VesselByPersistentId(oldId);
        }
        if (v != nullptr) oldRootPart = v->rootPart->fields.persistentId;
    }
}
// ActionVessel.cs:69
void ActionVessel::OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) {
    if (oldVessel->persistentId == vesselID || (newVessel->persistentId == vesselID && oldRootPart != 0)) {
        uint32_t vesselOfPart = 0;
        MissionsEnvironment::PartLookup r = MissionsEnvironment::PartLookup::NotFound;
        if (MissionsEnvironment::partVessel) r = MissionsEnvironment::partVessel(oldRootPart, true, vesselOfPart);
        if (r != MissionsEnvironment::PartLookup::NotFound) {
            if (r == MissionsEnvironment::PartLookup::FoundNoVessel) throw NullReferenceException();
            vesselID = vesselOfPart;
        } else {
            if (MissionsEnvironment::partVessel) r = MissionsEnvironment::partVessel(oldRootPart, false, vesselOfPart);
            if (r != MissionsEnvironment::PartLookup::NotFound) {
                if (r == MissionsEnvironment::PartLookup::FoundNoVessel) throw NullReferenceException();
                vesselID = vesselOfPart;
            }
        }
    }
}
// ActionVessel.cs:86
std::string ActionVessel::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "vesselID") {
        VesselSituation* vesselSituationByVesselID = node->mission->GetVesselSituationByVesselID(vesselID);
        if (vesselSituationByVesselID == nullptr) {
            return Localizer::Format("#autoLOC_8000069", {useActiveVessel ? Tag("#autoLOC_8004217") : Tag("#autoLOC_8001004")});
        }
        std::string text = vesselSituationByVesselID->vesselName;
        // HighLogic.LoadedSceneIsMissionBuilder: MissionEditorLogic.incompatibleCraft (MissionCraft: slice 2) is the editor's
        return Localizer::Format("#autoLOC_8000069", {text});
    }
    return ActionModule::GetNodeBodyParameterString(field);
}
// ActionVessel.cs:114
void ActionVessel::Save(ConfigNode& node_) const {
    ActionModule::Save(node_);
    node_.AddValue("vesselID", vesselID);
    if (oldRootPart != 0) node_.AddValue("oldRootPart", oldRootPart);
}
// ActionVessel.cs:124
void ActionVessel::Load(const ConfigNode& node_) {
    ActionModule::Load(node_);
    node_.TryGetValue("vesselID", vesselID);
    node_.TryGetValue("oldRootPart", oldRootPart);
}

// ---- ActionVesselActionGroup -------------------------------------------------------------------------------------------------

// ActionVesselActionGroup.cs:13 Awake
ActionVesselActionGroup::ActionVesselActionGroup() : ActionModule(kTypeName) { title = Tag("#autoLOC_8003179"); }
// ActionVesselActionGroup.cs:24
Enumerator ActionVesselActionGroup::Fire() {
    if (AwardEnvironment::ActiveVessel() != nullptr) {
        co_yield YieldInstruction::WaitForSeconds(0.1f);
        AwardEnvironment::ActiveVessel()->ActionGroups().ToggleGroup(actionGroup);
    }
    co_yield YieldInstruction::Null();
}
// ActionVesselActionGroup.cs:34
std::string ActionVesselActionGroup::GetInfo() { return Tag("#autoLOC_8003180"); }
// ActionVesselActionGroup.cs:39
void ActionVesselActionGroup::Save(ConfigNode& node) const {
    ActionModule::Save(node);
    node.AddValue("actionGroup", dotnet::EnumToString(actionGroup));
}
// ActionVesselActionGroup.cs:45
void ActionVesselActionGroup::Load(const ConfigNode& node) {
    ActionModule::Load(node);
    save_detail::TryGetEnum(node, "actionGroup", actionGroup, KSPActionGroup::None);
}

// ---- ActionVesselExplode -----------------------------------------------------------------------------------------------------

// ActionVesselExplode.cs:16 Awake
ActionVesselExplode::ActionVesselExplode() : ActionVessel(kTypeName) {
    title = Tag("#autoLOC_8000040");
    restartOnSceneLoad = true;
    useActiveVessel = true;
}
// ActionVesselExplode.cs:24
Enumerator ActionVesselExplode::Fire() {
    isRunning = true;
    if (vessel() != nullptr) {
        const float waitSeconds = f32store(fx(partsExplodeTime) / 1000.0);
        std::vector<Part*> parts;
        int count = static_cast<int>(vessel()->parts.size());
        while (count-- > 0) parts.push_back(vessel()->parts[static_cast<size_t>(count)]);
        int i = static_cast<int>(parts.size());
        while (i-- > 0) {
            if (NodeLibraryEnvironment::explodePart) {
                NodeLibraryEnvironment::explodePart(parts[static_cast<size_t>(i)]);
            } else {
                NodeLibraryEnvironment::explodedParts.push_back(parts[static_cast<size_t>(i)]);
            }
            co_yield YieldInstruction::WaitForSeconds(waitSeconds);
        }
        if (vessel() != nullptr) vessel()->Die();
    }
    isRunning = false;
}
// ActionVesselExplode.cs:50
std::string ActionVesselExplode::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "partsExplodeTime") return Localizer::Format("#autoLOC_8006045", {dotnet::FormatSingleG(partsExplodeTime)});
    return ActionVessel::GetNodeBodyParameterString(field);
}
// ActionVesselExplode.cs:59
std::string ActionVesselExplode::GetInfo() { return Tag("#autoLOC_8004001"); }
// ActionVesselExplode.cs:64
void ActionVesselExplode::Save(ConfigNode& node) const {
    ActionVessel::Save(node);
    node.AddValue("explodeTime", partsExplodeTime);
}
// ActionVesselExplode.cs:70
void ActionVesselExplode::Load(const ConfigNode& node) {
    ActionVessel::Load(node);
    node.TryGetValue("explodeTime", partsExplodeTime);
}

// ---- ActionVesselStage -------------------------------------------------------------------------------------------------------

// ActionVesselStage.cs:10 Awake
ActionVesselStage::ActionVesselStage() : ActionModule(kTypeName) { title = Tag("#autoLOC_8000043"); }
// ActionVesselStage.cs:16
Enumerator ActionVesselStage::Fire() {
    if (AwardEnvironment::ActiveVessel() != nullptr) {
        co_yield YieldInstruction::WaitForSeconds(0.1f);
        if (NodeLibraryEnvironment::activateNextStage) {
            NodeLibraryEnvironment::activateNextStage();
        } else {
            ++NodeLibraryEnvironment::stagesActivated;
        }
        AwardEnvironment::ActiveVessel()->ActionGroups().ToggleGroup(KSPActionGroup::Stage);
    }
    co_yield YieldInstruction::Null();
}
// ActionVesselStage.cs:27
std::string ActionVesselStage::GetInfo() { return Tag("#autoLOC_8004005"); }
// ActionVesselStage.cs:32
void ActionVesselStage::Save(ConfigNode& node) const { ActionModule::Save(node); }
// ActionVesselStage.cs:37
void ActionVesselStage::Load(const ConfigNode& node) { ActionModule::Load(node); }

// ---- ActionSetCamera ---------------------------------------------------------------------------------------------------------

// ActionSetCamera.cs:15 Awake
ActionSetCamera::ActionSetCamera() : ActionModule(kTypeName) { title = Tag("#autoLOC_8004179"); }
// ActionSetCamera.cs:21
Enumerator ActionSetCamera::Fire() {
    node->mission->SetLockedCamera(cameraLockMode, cameraLockOptions);
    co_return;
}
// ActionSetCamera.cs:27
std::string ActionSetCamera::GetInfo() { return Tag("#autoLOC_8004180"); }
// ActionSetCamera.cs:32
void ActionSetCamera::Save(ConfigNode& node) const {
    ActionModule::Save(node);
    node.AddValue("cameraTypeOptions", dotnet::EnumToString(cameraLockMode));
    node.AddValue("cameraLockOptions", dotnet::EnumToString(cameraLockOptions));
}
// ActionSetCamera.cs:39
void ActionSetCamera::Load(const ConfigNode& node) {
    ActionModule::Load(node);
    save_detail::TryGetEnum(node, "cameraTypeOptions", cameraLockMode, MissionCameraModeOptions::NoChange);
    save_detail::TryGetEnum(node, "cameraLockOptions", cameraLockOptions, MissionCameraLockOptions::Unlock);
}

// ---- ActionPartResourceAmount ------------------------------------------------------------------------------------------------

// ActionPartResourceAmount.cs:20 Awake
ActionPartResourceAmount::ActionPartResourceAmount() : ActionVessel(kTypeName) {
    title = Tag("#autoLOC_8000016");
    useActiveVessel = true;
}
// ActionPartResourceAmount.cs:27
Enumerator ActionPartResourceAmount::Fire() {
    if (vessel() != nullptr) {
        PartResourceLibrary* lib = PartResourceLibrary::Instance();
        if (lib == nullptr) throw NullReferenceException();
        const PartResourceDefinition* def = lib->GetDefinition(resourceName);
        if (def == nullptr) {
            log::Error("[ActionPartResourceAmount] failed to find resource definition for " + resourceName);
            co_yield YieldInstruction::Null();
        }
        if (vessel()->loaded) {
            if (def == nullptr) throw NullReferenceException();  // def.id after the (non-returning) yield of the C#
            double num = 0.0;
            if (NodeLibraryEnvironment::vesselRequestResource) {
                num = NodeLibraryEnvironment::vesselRequestResource(vessel(), vessel()->rootPart, def->id(), fx(f32store(0.0 - fx(adjustAmount))), true);
            }
            if (std::fabs(num - fx(adjustAmount)) > epsilon) {
                // Debug.Log("... failed to adjust resource by full amount")
            }
            if (num > 0.0) GameEvents::Mission::onNodeChangedVesselResources.Fire(node, vessel(), vessel()->rootPart, nullptr);
        } else {
            double num = fx(adjustAmount);
            bool flag = adjustAmount < 0.0f;
            for (size_t i = 0; i < vessel()->protoVessel->protoPartSnapshots.size(); ++i) {
                ProtoPartSnapshot* snapshot = vessel()->protoVessel->protoPartSnapshots[i].get();
                double num2 = adjustUnloadedPart(snapshot, num, flag);
                num = !flag ? num - num2 : num + num2;
                if (num2 > 0.0) GameEvents::Mission::onNodeChangedVesselResources.Fire(node, vessel(), nullptr, snapshot);
                if (std::fabs(num) <= epsilon) break;
            }
            if (std::fabs(num - fx(adjustAmount)) > epsilon) {
                // Debug.Log("... failed to adjust resource by full amount")
            }
        }
    }
    co_yield YieldInstruction::Null();
}
// ActionPartResourceAmount.cs:75 (C# bug kept: a partly drained resource adds the drained amount to the remaining demand)
double ActionPartResourceAmount::adjustUnloadedPart(ProtoPartSnapshot* InProtoPart, double adjustAmount_, bool pulling) {
    double num = 0.0;
    adjustAmount_ = std::fabs(adjustAmount_);
    size_t num2 = 0;
    ProtoPartResourceSnapshot* protoPartResourceSnapshot = nullptr;
    while (true) {
        if (num2 < InProtoPart->resources.size()) {
            protoPartResourceSnapshot = InProtoPart->resources[num2].get();
            if (protoPartResourceSnapshot->resourceName.has_value() && *protoPartResourceSnapshot->resourceName == resourceName) {
                double num3 = 0.0;
                if (pulling) {
                    num3 = protoPartResourceSnapshot->amount;
                    if (num3 > 0.0) {
                        if (!(num3 < adjustAmount_)) {
                            protoPartResourceSnapshot->amount -= adjustAmount_;
                            return num + adjustAmount_;
                        }
                        protoPartResourceSnapshot->amount = 0.0;
                        num += num3;
                        adjustAmount_ += num3;
                    }
                } else {
                    num3 = protoPartResourceSnapshot->maxAmount - protoPartResourceSnapshot->amount;
                    if (num3 > 0.0) {
                        if (!(num3 < adjustAmount_)) break;
                        protoPartResourceSnapshot->amount = protoPartResourceSnapshot->maxAmount;
                        num += num3;
                        adjustAmount_ -= num3;
                    }
                }
            }
            num2++;
            continue;
        }
        return num;
    }
    protoPartResourceSnapshot->amount += adjustAmount_;
    return num + adjustAmount_;
}
// ActionPartResourceAmount.cs:145
std::string ActionPartResourceAmount::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "resourceName") {
        std::string text = Tag("#autoLOC_6003083");
        if (!resourceName.empty()) {
            PartResourceLibrary* lib = PartResourceLibrary::Instance();
            if (lib == nullptr) throw NullReferenceException();
            const PartResourceDefinition* definition = lib->GetDefinition(resourceName);
            if (definition != nullptr) text = definition->displayName();
        }
        return Localizer::Format("#autoLOC_8004190", {field.guiName, text});
    }
    return ActionVessel::GetNodeBodyParameterString(field);
}
// ActionPartResourceAmount.cs:163
std::string ActionPartResourceAmount::GetInfo() { return Tag("#autoLOC_8004000"); }
// ActionPartResourceAmount.cs:168
void ActionPartResourceAmount::Save(ConfigNode& node) const {
    ActionVessel::Save(node);
    node.AddValue("resourceName", resourceName);
    node.AddValue("adjustAmount", adjustAmount);
}
// ActionPartResourceAmount.cs:175
void ActionPartResourceAmount::Load(const ConfigNode& node) {
    ActionVessel::Load(node);
    node.TryGetValue("resourceName", resourceName);
    node.TryGetValue("adjustAmount", adjustAmount);
}

// ---- ActionPartExplode -------------------------------------------------------------------------------------------------------

// ActionPartExplode.cs:55 Awake
ActionPartExplode::ActionPartExplode() : ActionModule(kTypeName) {
    title = Tag("#autoLOC_8000060");
    vesselPartIDs = VesselPartIDPair();
    vesselPartIDs.partID = 0u;
}
// ActionPartExplode.cs:13
void ActionPartExplode::OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) {
    if (vesselPartIDs.VesselID == oldId) vesselPartIDs.VesselID = newId;
}
// ActionPartExplode.cs:22
void ActionPartExplode::OnPartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId) {
    if (vesselPartIDs.VesselID == vesselID && vesselPartIDs.partID == oldId) vesselPartIDs.partID = newId;
}
// ActionPartExplode.cs:31
void ActionPartExplode::OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) {
    if (vesselPartIDs.VesselID != oldVessel->persistentId && vesselPartIDs.VesselID != newVessel->persistentId) return;
    size_t num = 0;
    while (true) {
        if (num < newVessel->parts.size()) {
            if (vesselPartIDs.partID == newVessel->parts[num]->fields.persistentId) break;
            num++;
            continue;
        }
        return;
    }
    vesselPartIDs.VesselID = newVessel->persistentId;
}
// ActionPartExplode.cs:63
Enumerator ActionPartExplode::Fire() {
    if (vesselPartIDs.partID != 0) {
        Part* partout = NodeLibraryEnvironment::findLoadedPart ? NodeLibraryEnvironment::findLoadedPart(vesselPartIDs.partID) : nullptr;
        if (partout != nullptr) {
            if (NodeLibraryEnvironment::explodePart) {
                NodeLibraryEnvironment::explodePart(partout);
            } else {
                NodeLibraryEnvironment::explodedParts.push_back(partout);
            }
            Vessel* active = AwardEnvironment::ActiveVessel();
            if (active == nullptr) throw NullReferenceException();
            if (partout == active->rootPart) {
                Vessel* v = NodeLibraryEnvironment::findNearestControllableVessel ? NodeLibraryEnvironment::findNearestControllableVessel(active) : nullptr;
                if (v != nullptr && NodeLibraryEnvironment::forceSetActiveVessel) NodeLibraryEnvironment::forceSetActiveVessel(v);
            }
        } else {
            // Debug.LogWarning("Unable to perform action, part was not found on a loaded vessel")
        }
    } else {
        uint32_t key = node->mission->CurrentVesselID(node, vesselPartIDs.VesselID);
        Vessel* vessel2 = AwardEnvironment::VesselByPersistentId(key);
        if (vessel2 == nullptr) throw KeyNotFoundException("The given key was not present in the dictionary.");  // FlightGlobals.PersistentVesselIds[key]
        if (vessel2->loaded) {
            const int count = static_cast<int>(vessel2->Parts().size());
            const int lo = (1 < count) ? 1 : 0;
            const int idx = NodeLibraryEnvironment::randomRange ? NodeLibraryEnvironment::randomRange(lo, count) : lo;
            Part* p = vessel2->Parts()[static_cast<size_t>(idx)];
            if (NodeLibraryEnvironment::explodePart) {
                NodeLibraryEnvironment::explodePart(p);
            } else {
                NodeLibraryEnvironment::explodedParts.push_back(p);
            }
        }
    }
    co_yield YieldInstruction::Null();
}
// ActionPartExplode.cs:105
std::string ActionPartExplode::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "vesselPartIDs") return Localizer::Format("#autoLOC_8004190", {field.guiName, Localizer::Format(vesselPartIDs.partName)});
    return ActionModule::GetNodeBodyParameterString(field);
}
// ActionPartExplode.cs:114
std::string ActionPartExplode::GetInfo() { return Tag("#autoLOC_8004003"); }
// ActionPartExplode.cs:119
void ActionPartExplode::RunValidation(Editor::MissionEditorValidator* validator) { vesselPartIDs.ValidatePartAgainstCraft(node, validator); }
// ActionPartExplode.cs:127
void ActionPartExplode::Save(ConfigNode& node_) const {
    ActionModule::Save(node_);
    vesselPartIDs.Save(node_);
}
// ActionPartExplode.cs:133
void ActionPartExplode::Load(const ConfigNode& node_) {
    ActionModule::Load(node_);
    vesselPartIDs.Load(node_);
}

}  // namespace ksp::Expansions::Missions::Actions
