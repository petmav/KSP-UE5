// WP-52: ModuleRoboticController.cs (see include/ksp/serenity/robotic_controller.hpp). Every function cites ModuleRoboticController.cs:<line> of
// decompiled/KSP-Decompiled/Assembly-CSharp/Expansions/Serenity/.
#include "ksp/serenity/robotic_controller.hpp"

#include <algorithm>
#include <span>
#include <stdexcept>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/serenity/servo_modules.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Serenity {

using GE = GameEvents;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
float F(double v) { return f32store(v); }

template <class T>
void RemoveAt(std::vector<T>& v, size_t index) {  // List<T>.RemoveAt
    if (index >= v.size()) throw std::out_of_range("Index was out of range. Must be non-negative and less than the size of the collection.");
    v.erase(v.begin() + static_cast<std::ptrdiff_t>(index));
}

constexpr std::pair<std::string_view, ModuleRoboticController_SequenceDirectionOptions> kDirectionMembers[] = {
    {"Forward", ModuleRoboticController_SequenceDirectionOptions::Forward}, {"Reverse", ModuleRoboticController_SequenceDirectionOptions::Reverse}};
constexpr std::pair<std::string_view, ModuleRoboticController_SequenceLoopOptions> kLoopMembers[] = {
    {"Once", ModuleRoboticController_SequenceLoopOptions::Once},
    {"Repeat", ModuleRoboticController_SequenceLoopOptions::Repeat},
    {"PingPong", ModuleRoboticController_SequenceLoopOptions::PingPong},
    {"OnceRestart", ModuleRoboticController_SequenceLoopOptions::OnceRestart}};
const char* Name(ModuleRoboticController_SequenceDirectionOptions d) { return d == ModuleRoboticController_SequenceDirectionOptions::Forward ? "Forward" : "Reverse"; }
const char* Name(ModuleRoboticController_SequenceLoopOptions l) {
    switch (l) {
    case ModuleRoboticController_SequenceLoopOptions::Once: return "Once";
    case ModuleRoboticController_SequenceLoopOptions::Repeat: return "Repeat";
    case ModuleRoboticController_SequenceLoopOptions::PingPong: return "PingPong";
    default: return "OnceRestart";
    }
}
uint32_t ActionId(PartModule& m) { return SerenityMiscEnvironment::persistentActionId ? SerenityMiscEnvironment::persistentActionId(m) : 0u; }
}  // namespace

ModuleRoboticController::~ModuleRoboticController() {
    if (!destroyed) OnDestroy();
}

Part* ModuleRoboticController::PartOf() const { return dynamic_cast<Part*>(part); }

// :129
uint32_t ModuleRoboticController::PartPersistentId() const {
    Part* p = PartOf();
    return p == nullptr ? 0u : p->fields.persistentId;
}

// :297
void ModuleRoboticController::Start() {
    if (!SaveEnvironment::Current().IsExpansionInstalled("Serenity") && SaveEnvironment::Current().loadedSceneIsGame) {
        behaviourEnabled = false;
        destroyed = true;
    }
}

// :306
void ModuleRoboticController::OnAwake() {
    PartModule::OnAwake();
    controlledAxes.clear();
    controlledActions.clear();
    leavingScene = false;
    consumedResources_.clear();
    for (size_t i = 0, count = resHandler.inputResources.size(); i < count; i++) {
        PartResourceLibrary* lib = PartResourceLibrary::Instance();
        consumedResources_.push_back(lib != nullptr ? lib->GetDefinition(resHandler.inputResources[i]->name) : nullptr);
    }
    Actions().Bind("TogglePlayAction", [this](KSPActionParam& p) { TogglePlayAction(p); });
    Actions().Bind("ToggleLoopModeAction", [this](KSPActionParam& p) { ToggleLoopModeAction(p); });
    Actions().Bind("ToggleDirectionAction", [this](KSPActionParam& p) { ToggleDirectionAction(p); });
    Actions().Bind("ToggleControllerEnabledAction", [this](KSPActionParam& p) { ToggleControllerEnabledAction(p); });
    Actions().Bind("ToggleControllerEnabledOn", [this](KSPActionParam& p) { ToggleControllerEnabledOn(p); });
    Actions().Bind("ToggleControllerEnabledOff", [this](KSPActionParam& p) { ToggleControllerEnabledOff(p); });
    Actions().Bind("PlaySequenceAction", [this](KSPActionParam& p) { PlaySequenceAction(p); });
    Actions().Bind("StopSequenceAction", [this](KSPActionParam& p) { StopSequenceAction(p); });
    Actions().Bind("SequenceForwardAction", [this](KSPActionParam& p) { SequenceForwardAction(p); });
    Actions().Bind("SequenceReverseAction", [this](KSPActionParam& p) { SequenceReverseAction(p); });
    Actions().Bind("SequenceLoopOnceAction", [this](KSPActionParam& p) { SequenceLoopOnceAction(p); });
    Actions().Bind("SequenceLoopRepeatAction", [this](KSPActionParam& p) { SequenceLoopRepeatAction(p); });
    Actions().Bind("SequenceLoopPingPongAction", [this](KSPActionParam& p) { SequenceLoopPingPongAction(p); });
    Actions().Bind("SequenceLoopOnceRestartAction", [this](KSPActionParam& p) { SequenceLoopOnceRestartAction(p); });
    Actions().Bind("SequencePlaySpeedZeroAction", [this](KSPActionParam& p) { SequencePlaySpeedZeroAction(p); });
    Actions().Bind("SequencePlaySpeedFullAction", [this](KSPActionParam& p) { SequencePlaySpeedFullAction(p); });
    Events().Bind("OpenControllerEditor", BaseEvent::Handler([this] { OpenControllerEditor(); }));
}

// :333
void ModuleRoboticController::OnDestroy() {
    destroyed = true;
    GE::onPartPersistentIdChanged.Remove(this, &ModuleRoboticController::PartPersistentIdChanged);
    GE::onEditorPartEvent.Remove(this, &ModuleRoboticController::onEditorPartEvent);
    GE::onPartWillDie.Remove(this, &ModuleRoboticController::OnPartWillDie);
    GE::onGameSceneLoadRequested.Remove(this, &ModuleRoboticController::OnSceneLoadRequested);
    GE::onTimeWarpRateChanged.Remove(this, &ModuleRoboticController::OnWarpRateChanged);
    GE::onVesselWasModified.Remove(this, &ModuleRoboticController::VesselModified);
}

// :355
void ModuleRoboticController::OnStart(StartState state) {
    PartModule::OnStart(state);
    const bool flight = ServoEnvironment::SceneIsFlight();
    const bool editor = ServoEnvironment::SceneIsEditor();
    const bool missionBuilder = ServoEnvironment::sceneIsMissionBuilder ? ServoEnvironment::sceneIsMissionBuilder() : false;
    if (RoboticControllerManager::Instance == nullptr && !missionBuilder) {
        log::Error("[ModuleRoboticController]: There is no RoboticControllerManager running");
        destroyed = true;  // Object.Destroy(base.gameObject)
    }
    if (editor || flight) GE::onPartPersistentIdChanged.Add(this, &ModuleRoboticController::PartPersistentIdChanged);
    if (editor)
        GE::onEditorPartEvent.Add(this, &ModuleRoboticController::onEditorPartEvent);
    else if (flight)
        GE::onPartWillDie.Add(this, &ModuleRoboticController::OnPartWillDie);
    GE::onGameSceneLoadRequested.Add(this, &ModuleRoboticController::OnSceneLoadRequested);
    GE::onTimeWarpRateChanged.Add(this, &ModuleRoboticController::OnWarpRateChanged);
    GE::onVesselWasModified.Add(this, &ModuleRoboticController::VesselModified);
    sequenceAxisPositionAxis = dynamic_cast<BaseAxisField*>(Fields()["sequencePosition"]);
    if (sequenceAxisPositionAxis == nullptr) throw Nre();
    sequenceAxisPositionAxis->maxValue = SequenceLength();
    for (const char* n : {"sequencePosition", "sequencePlaySpeed", "sequencePlayingIndex", "sequenceDirectionIndex", "sequenceLoopIndex"})
        Fields()[n]->OnValueModified.push_back([this](const ModuleValue&) { sequenceFieldChanged(); });
    Fields()["priorityField"]->OnValueModified.push_back([this](const ModuleValue&) { PriorityChanged(); });
    Part* p = PartOf();
    if (Get<std::string>("displayName").empty() && p != nullptr && p->partInfo != nullptr) Set<std::string>("displayName", p->partInfo->title);
    size_t count = controlledAxes.size();
    while (count-- > 0) {
        if (!controlledAxes[count]->AssignReferenceVars()) {
            log::Warning("[ModuleRoboticController]: Removing the axis we couldnt bind from the controller");
            RemoveAt(controlledAxes, count);
        }
    }
    size_t count2 = controlledActions.size();
    while (count2-- > 0) {
        if (!controlledActions[count2]->AssignReferenceVars()) {
            log::Warning("[ModuleRoboticController]: Removing the action we couldnt bind from the controller");
            RemoveAt(controlledActions, count2);
        }
    }
    PriorityChanged();
    UpdateUIElements();
}

// :418
void ModuleRoboticController::OnUpdate() {
    PartModule::OnUpdate();
    if (!leavingScene && SequenceIsPlaying() && !pausedInWarp) UpdatePlayingSequencePosition();
}

// :427
void ModuleRoboticController::Update() {
    sequenceWasPlaying = SequenceIsPlaying();
    if (ServoEnvironment::SceneIsEditor() && SequenceIsPlaying()) UpdatePlayingSequencePosition();
}

// :436
void ModuleRoboticController::FixedUpdate() {
    if (SequenceIsPlaying() && controllerEnabled()) {
        if (ServoEnvironment::SceneIsEditor())
            hasEnoughResources = true;
        else
            hasEnoughResources = resHandler.UpdateModuleResourceInputs(consumptionString, 1.0, 0.999, /*returnOnFirstLack*/ false, /*average*/ false, /*stringOps*/ true) > 0.0;
    }
}

// :451
void ModuleRoboticController::UpdatePlayingSequencePosition() {
    if (hasEnoughResources) {
        updateTime = Time::deltaTime;
        if (SequenceDirection() == SequenceDirectionOptions::Reverse) updateTime = F(fx(updateTime) * fx(-1.0f));
        updateTime = F(fx(updateTime) * (fx(SequencePlaySpeed()) * fx(0.01f)));
        SetSequencePosition(F(fx(SequencePosition()) + fx(updateTime)));
        CheckAndFireActions();
    }
}

// :466
void ModuleRoboticController::CheckAndFireActions() {
    for (size_t i = 0; i < controlledActions.size(); i++) {
        for (size_t j = 0; j < controlledActions[i]->times.size(); j++) {
            bool flag = false;
            const double t = fx(controlledActions[i]->times[j]);
            const double len = fx(SequenceLength());
            if (t >= fx(actionCheck1StartTime) / len && t < fx(actionCheck1EndTime) / len) flag = true;
            if (t >= fx(actionCheck2StartTime) / len && t < fx(actionCheck2EndTime) / len) flag = true;
            if ((SequenceLoop() == SequenceLoopOptions::Once || SequenceLoop() == SequenceLoopOptions::OnceRestart) && t == fx(actionCheck1EndTime) / len) flag = true;
            if (!flag) continue;
            actionsToFire.push_back(controlledActions[i]->Action);
            for (size_t k = 0; k < controlledActions[i]->SymmetryActions.size(); k++) actionsToFire.push_back(controlledActions[i]->SymmetryActions[k]);
        }
    }
    size_t count = actionsToFire.size();
    while (count-- > 0) {
        BaseAction* a = actionsToFire[count];
        if (a == nullptr) throw Nre();
        if (a->active && (!a->requireFullControl || !InputLockManager::IsLocked(ControlTypes::TWEAKABLES_FULLONLY)) && (a->activeEditor || !ServoEnvironment::SceneIsEditor()))
            a->Invoke(controllerActionParam);
    }
    if (!actionsToFire.empty()) actionsToFire.clear();
}

// :518
void ModuleRoboticController::OnWarpRateChanged() {
    if (TimeWarp::CurrentRateIndex() > 0 && TimeWarp::WarpMode() == TimeWarp::Modes::HIGH)
        pausedInWarp = true;
    else if (pausedInWarp)
        pausedInWarp = false;
}

// :535
void ModuleRoboticController::ToggleControllerEnabled(bool enabled) { Set<bool>("controllerEnabled", enabled); }

// :545
void ModuleRoboticController::TogglePlay(bool play) {
    if (!play)
        SequenceStop();
    else
        SequencePlay();
}

// :557 (sic: the played event fires when the sequence WAS already playing)
void ModuleRoboticController::SequencePlay() {
    const bool num = !SequenceIsPlaying();
    if (num && SequenceLoop() == SequenceLoopOptions::OnceRestart && SequencePosition() >= SequenceLength()) SetSequencePositionStart();
    Set<int32_t>("sequencePlayingIndex", 0);
    if (!num) GE::onRoboticControllerSequencePlayed.Fire(this);
    UpdateUIElements();
}

// :572 (same quirk for the stopped event)
void ModuleRoboticController::SequenceStop() {
    const bool sequenceIsPlaying = SequenceIsPlaying();
    Set<int32_t>("sequencePlayingIndex", 1);
    if (!sequenceIsPlaying) GE::onRoboticControllerSequenceStopped.Fire(this);
    UpdateUIElements();
}

// :583
void ModuleRoboticController::ToggleDirection() {
    const SequenceDirectionOptions d = SequenceDirection();
    if (d != SequenceDirectionOptions::Forward && d == SequenceDirectionOptions::Reverse)
        SetDirection(SequenceDirectionOptions::Forward);
    else
        SetDirection(SequenceDirectionOptions::Reverse);
}

// :596
void ModuleRoboticController::SetDirection(SequenceDirectionOptions newDirection) {
    const bool num = SequenceDirection() != newDirection;
    Set<int32_t>("sequenceDirectionIndex", static_cast<int32_t>(newDirection));
    if (num) GE::onRoboticControllerSequenceDirectionChanged.Fire(this, SequenceDirection());
    UpdateUIElements();
}

// :607
void ModuleRoboticController::CycleLoopMode() {
    switch (SequenceLoop()) {
    case SequenceLoopOptions::Once: SetLoopMode(SequenceLoopOptions::Repeat); break;
    case SequenceLoopOptions::Repeat: SetLoopMode(SequenceLoopOptions::PingPong); break;
    case SequenceLoopOptions::PingPong: SetLoopMode(SequenceLoopOptions::OnceRestart); break;
    default: SetLoopMode(SequenceLoopOptions::Once); break;
    }
    GE::onRoboticControllerSequenceLoopModeChanged.Fire(this, SequenceLoop());
    UpdateUIElements();
}

// :628
void ModuleRoboticController::SetLoopMode(SequenceLoopOptions newMode) {
    const bool num = SequenceLoop() != newMode;
    Set<int32_t>("sequenceLoopIndex", static_cast<int32_t>(newMode));
    if (num) GE::onRoboticControllerSequenceLoopModeChanged.Fire(this, SequenceLoop());
    UpdateUIElements();
}

// :639
void ModuleRoboticController::SetDisplayName(const std::string& newName) {
    Set<std::string>("displayName", newName);
    UpdateUIElements();
}

// :645
void ModuleRoboticController::SetLength(float newLength) {
    if (newLength < 0.01f) newLength = 0.01f;
    float num = SequencePosition();
    if (num > 0.0f) num = F(fx(num) / fx(Get<float>("sequenceLength")) * fx(newLength));
    Set<float>("sequenceLength", newLength);
    Set<float>("sequencePosition", num);
    if (sequenceAxisPositionAxis == nullptr) throw Nre();
    sequenceAxisPositionAxis->maxValue = SequenceLength();
    UpdateUIElements();
}

// :662
void ModuleRoboticController::SetLength(float newLength, bool maintainPointsTime) {
    if (maintainPointsTime) {
        for (size_t i = 0; i < controlledAxes.size(); i++) controlledAxes[i]->RescaleCurveTime(F(fx(SequenceLength()) / fx(newLength)), F(fx(0.01f) / fx(newLength)));
        for (size_t j = 0; j < controlledActions.size(); j++) controlledActions[j]->RescaleTimes(F(fx(SequenceLength()) / fx(newLength)), F(fx(0.01f) / fx(newLength)));
    }
    SetLength(newLength);
}

// :678
void ModuleRoboticController::SetSequencePosition(float newPosition) {
    actionCheck2EndTime = -1.0f;
    actionCheck2StartTime = -1.0f;
    actionCheck1EndTime = -1.0f;
    actionCheck1StartTime = -1.0f;
    if (!(newPosition > SequenceLength()) && newPosition >= 0.0f) {
        if (SequenceDirection() == SequenceDirectionOptions::Forward) {
            actionCheck1StartTime = SequencePosition();
            actionCheck1EndTime = newPosition;
        } else {
            actionCheck1StartTime = newPosition;
            actionCheck1EndTime = SequencePosition();
        }
    } else {
        switch (SequenceLoop()) {
        case SequenceLoopOptions::Repeat:
            if (newPosition < 0.0f) {
                newPosition = F(fx(SequenceLength()) + fx(newPosition));
                actionCheck1StartTime = 0.0f;
                actionCheck1EndTime = SequencePosition();
                actionCheck2StartTime = newPosition;
                actionCheck2EndTime = F(fx(SequenceLength()) + 1.0);
            } else {
                newPosition = F(fx(newPosition) - fx(SequenceLength()));
                actionCheck1StartTime = 0.0f;
                actionCheck1EndTime = newPosition;
                actionCheck2StartTime = SequencePosition();
                actionCheck2EndTime = F(fx(SequenceLength()) + 1.0);
            }
            break;
        case SequenceLoopOptions::PingPong:
            if (newPosition < 0.0f) {
                newPosition = F(0.0 - fx(newPosition));
                actionCheck1StartTime = 0.0f;
                actionCheck1EndTime = SequencePosition();
                actionCheck2StartTime = 0.0f;
                actionCheck2EndTime = newPosition;
            } else {
                newPosition = F(fx(SequenceLength()) - (fx(newPosition) - fx(SequenceLength())));
                actionCheck1StartTime = SequencePosition();
                actionCheck1EndTime = F(fx(SequenceLength()) + 1.0);
                actionCheck2StartTime = newPosition;
                actionCheck2EndTime = F(fx(SequenceLength()) + 1.0);
            }
            ToggleDirection();
            break;
        default:
            if (newPosition < 0.0f) {
                newPosition = 0.0f;
                actionCheck1StartTime = 0.0f;
                actionCheck1EndTime = SequencePosition();
            } else {
                newPosition = SequenceLength();
                actionCheck1StartTime = SequencePosition();
                actionCheck1EndTime = newPosition;
            }
            SequenceStop();
            break;
        }
    }
    newPosition = Mathf::Clamp(newPosition, 0.0f, SequenceLength());
    Set<float>("sequencePosition", newPosition);
    if (controllerEnabled()) {
        for (size_t i = 0; i < controlledAxes.size(); i++) {
            ControlledAxis& controlledAxis = *controlledAxes[i];
            if (controlledAxis.AxisField == nullptr) continue;
            auto* controller = dynamic_cast<ModuleRoboticController*>(dynamic_cast<PartModule*>(controlledAxis.AxisField->host()));
            if (controller != nullptr) {
                float valueMultiple = controller->SequenceLength();
                if (controlledAxis.axisName == "sequencePlaySpeed") valueMultiple = 100.0f;
                controlledAxis.UpdateFieldValue(F(fx(newPosition) / fx(SequenceLength()) * fx(controlledAxis.timeValue.maxTime())), valueMultiple);
            } else {
                controlledAxis.UpdateFieldValue(F(fx(newPosition) / fx(SequenceLength()) * fx(controlledAxis.timeValue.maxTime())));
            }
        }
    }
    UpdateUIElements();
}

// :790
void ModuleRoboticController::SetPlaySpeed(float newSpeed) {
    Set<float>("sequencePlaySpeed", Mathf::Clamp(newSpeed, 0.0f, 100.0f));
    UpdateUIElements();
}

// :806
void ModuleRoboticController::SetSequencePositionPrevKey() {
    float a = 0.0f;
    float num = 0.0f;
    for (size_t i = 0; i < controlledAxes.size(); i++) {
        ControlledAxis& axis = *controlledAxes[i];
        for (size_t j = 0; j < axis.timeValue.Curve().keys().size(); j++) {
            num = F(fx(axis.timeValue.Curve().keys()[j].time) / fx(axis.timeValue.maxTime()) * fx(SequenceLength()));
            if (num < SequencePosition()) a = Mathf::Max(a, num);
        }
    }
    for (size_t k = 0; k < controlledActions.size(); k++) {
        ControlledAction& action = *controlledActions[k];
        for (size_t l = 0; l < action.times.size(); l++) {
            num = F(fx(action.times[l]) * fx(SequenceLength()));
            if (num < SequencePosition()) a = Mathf::Max(a, num);
        }
    }
    SetSequencePosition(a);
}

// :837 (sic: the action loop tests `num < SequencePosition` like the previous-key one)
void ModuleRoboticController::SetSequencePositionNextKey() {
    float a = SequenceLength();
    float num = 0.0f;
    for (size_t i = 0; i < controlledAxes.size(); i++) {
        ControlledAxis& axis = *controlledAxes[i];
        for (size_t j = 0; j < axis.timeValue.Curve().keys().size(); j++) {
            num = F(fx(axis.timeValue.Curve().keys()[j].time) / fx(axis.timeValue.maxTime()) * fx(SequenceLength()));
            if (num > SequencePosition()) a = Mathf::Min(a, num);
        }
    }
    for (size_t k = 0; k < controlledActions.size(); k++) {
        ControlledAction& action = *controlledActions[k];
        for (size_t l = 0; l < action.times.size(); l++) {
            num = F(fx(action.times[l]) * fx(SequenceLength()));
            if (num < SequencePosition()) a = Mathf::Min(a, num);
        }
    }
    SetSequencePosition(a);
}

// :884 (the part-action window and the controller window are UI; the counters the PAW shows are state)
void ModuleRoboticController::UpdateUIElements() {
    Set<int32_t>("fieldsCount", static_cast<int32_t>(controlledAxes.size()));
    Set<int32_t>("actionsCount", static_cast<int32_t>(controlledActions.size()));
}

// :912
void ModuleRoboticController::sequenceFieldChanged() {
    if (!sequenceWasPlaying && SequenceIsPlaying() && SequenceLoop() == SequenceLoopOptions::OnceRestart && SequencePosition() >= SequenceLength()) SetSequencePositionStart();
    SetSequencePosition(SequencePosition());
}

// :921
void ModuleRoboticController::PriorityChanged() { priority = static_cast<int>(Mathf::Clamp(Get<float>("priorityField"), 1.0f, 5.0f)); }

// :930
void ModuleRoboticController::PartPersistentIdChanged(uint32_t, uint32_t oldId, uint32_t newId) {
    for (size_t i = 0; i < controlledAxes.size(); i++) {
        if (controlledAxes[i]->partId == oldId) {
            controlledAxes[i]->partId = newId;
            controlledAxes[i]->AssignReferenceVars();
        }
    }
    for (size_t j = 0; j < controlledActions.size(); j++) {
        if (controlledActions[j]->partId == oldId) {
            controlledActions[j]->partId = newId;
            controlledActions[j]->AssignReferenceVars();
        }
    }
}

// :957
void ModuleRoboticController::UpdatePersistentIDs(const std::map<uint32_t, uint32_t>& changedIDs) {
    for (size_t i = 0; i < controlledAxes.size(); i++) {
        auto it = changedIDs.find(controlledAxes[i]->partId);
        if (it != changedIDs.end()) controlledAxes[i]->partId = it->second;
    }
    for (size_t j = 0; j < controlledActions.size(); j++) {
        auto it = changedIDs.find(controlledActions[j]->partId);
        if (it != changedIDs.end()) controlledActions[j]->partId = it->second;
    }
}

// :975
void ModuleRoboticController::onEditorPartEvent(ConstructionEventType evt, Part* targetPart) {
    switch (evt) {
    case ConstructionEventType::PartDeleted: PartDeleted(targetPart); break;
    case ConstructionEventType::PartSymmetryDeleted: PartDeleted(targetPart); break;
    case ConstructionEventType::PartAttached: UpdatePartSymmetry(targetPart); break;
    default: break;
    }
}

// :1008
void ModuleRoboticController::SetWindowSizeAndPosition(const Vector2f& position, const Vector2f& size) {
    Set<Vector2f>("windowPosition", position);
    Set<Vector2f>("windowSize", size);
    HasWindowDimensions = true;
}

// :1015
void ModuleRoboticController::AddPartAxis(Part* targetPart, PartModule* module, BaseAxisField* axisField) {
    if (!HasPartAxisField(targetPart, axisField) && !HasPartSymmetryAxisField(targetPart, axisField)) {
        auto controlledAxis = std::make_shared<ControlledAxis>(targetPart, module, axisField, this);
        GE::onRoboticControllerAxesAdding.Fire(this, controlledAxis.get());
        controlledAxes.push_back(controlledAxis);
        GE::onRoboticControllerAxesChanged.Fire(this);
        UpdateUIElements();
    }
}

// :1027
void ModuleRoboticController::AddPartAction(Part* targetPart, PartModule* module, BaseAction* action) {
    if (!HasPartAction(targetPart, action) && !HasPartSymmetryAction(targetPart, action)) {
        auto controlledAction = std::make_shared<ControlledAction>(targetPart, module, action, this);
        GE::onRoboticControllerActionsAdding.Fire(this, controlledAction.get());
        controlledActions.push_back(controlledAction);
        GE::onRoboticControllerActionsChanged.Fire(this);
        UpdateUIElements();
    }
}

// :1039
void ModuleRoboticController::PartDeleted(Part* targetPart) {
    RemovePartAxis(targetPart, nullptr, true);
    RemovePartAction(targetPart, nullptr, true);
    RemovePartSymmetry(targetPart->fields.persistentId);
}

// :1046
void ModuleRoboticController::RemovePartAxis(Part* targetPart, BaseAxisField* axisField, bool transferToSymPartner) {
    bool flag = false;
    size_t count = controlledAxes.size();
    Part* self = PartOf();
    while (count-- > 0) {
        if (count >= controlledAxes.size()) throw std::out_of_range("Index was out of range. Must be non-negative and less than the size of the collection.");
        std::shared_ptr<ControlledAxis> controlledAxis = controlledAxes[count];
        if (!(controlledAxis->PartPtr() != nullptr) || controlledAxis->PartPtr()->fields.persistentId != targetPart->fields.persistentId ||
            (axisField != nullptr && (controlledAxis->AxisField == nullptr || !(controlledAxis->AxisField->name() == axisField->name()))))
            continue;
        if (!targetPart->symmetryCounterparts.empty() && transferToSymPartner) {
            bool flag2 = false;
            for (size_t i = 0; i < targetPart->symmetryCounterparts.size(); i++) {
                if (targetPart->symmetryCounterparts[i]->vessel == (self != nullptr ? self->vessel : nullptr)) {
                    flag2 = true;
                    AddPartAxisForSymmetryPartner(*controlledAxis, targetPart->symmetryCounterparts[i]);
                    RemoveAt(controlledAxes, count);
                    flag = true;
                }
            }
            if (!flag2) {
                GE::onRoboticControllerAxesRemoving.Fire(this, controlledAxis.get());
                RemoveAt(controlledAxes, count);
                flag = true;
            }
        } else {
            GE::onRoboticControllerAxesRemoving.Fire(this, controlledAxis.get());
            RemoveAt(controlledAxes, count);
            flag = true;
        }
    }
    if (flag) GE::onRoboticControllerAxesChanged.Fire(this);
    UpdateUIElements();
}

// :1091
void ModuleRoboticController::RemovePartAction(Part* targetPart, BaseAction* action, bool transferToSymPartner) {
    bool flag = false;
    size_t count = controlledActions.size();
    Part* self = PartOf();
    while (count-- > 0) {
        if (count >= controlledActions.size()) throw std::out_of_range("Index was out of range. Must be non-negative and less than the size of the collection.");
        std::shared_ptr<ControlledAction> controlledAction = controlledActions[count];
        const bool partCond = !(controlledAction->PartPtr() != nullptr) || controlledAction->PartPtr()->fields.persistentId != targetPart->fields.persistentId || action != nullptr;
        const bool actCond = controlledAction->Action == nullptr || action == nullptr || !(controlledAction->Action->name == action->name) ||
                             (!(controlledAction->Module() == nullptr) && action->listParent != nullptr && !(action->listParent->module == nullptr) &&
                              controlledAction->moduleId != action->listParent->module->PersistentId());
        if (partCond && actCond) continue;
        if (!targetPart->symmetryCounterparts.empty() && transferToSymPartner) {
            bool flag2 = false;
            for (size_t i = 0; i < targetPart->symmetryCounterparts.size(); i++) {
                if (targetPart->symmetryCounterparts[i]->vessel == (self != nullptr ? self->vessel : nullptr)) {
                    flag2 = true;
                    AddPartActionForSymmetryPartner(*controlledAction, targetPart->symmetryCounterparts[i]);
                    RemoveAt(controlledActions, count);
                    flag = true;
                }
            }
            if (!flag2) {
                GE::onRoboticControllerActionsRemoving.Fire(this, controlledAction.get());
                RemoveAt(controlledActions, count);
                flag = true;
            }
        } else {
            GE::onRoboticControllerActionsRemoving.Fire(this, controlledAction.get());
            RemoveAt(controlledActions, count);
            flag = true;
        }
    }
    if (flag) GE::onRoboticControllerActionsChanged.Fire(this);
    UpdateUIElements();
}

// :1136
void ModuleRoboticController::RemovePartAxis(ControlledAxis& axis, bool transferToSymPartner) { RemovePartAxis(axis.PartPtr(), axis.AxisField, transferToSymPartner); }

// :1141
void ModuleRoboticController::RemovePartAction(ControlledAction& action, bool transferToSymPartner) { RemovePartAction(action.PartPtr(), action.Action, transferToSymPartner); }

// :1146
void ModuleRoboticController::AddPartAxisForSymmetryPartner(ControlledAxis& oldAxis, Part* newPart) {
    if (oldAxis.AxisField == nullptr || dynamic_cast<PartModule*>(oldAxis.AxisField->host()) == nullptr) return;
    BaseAxisField* baseAxisField = nullptr;
    PartModule* partModule = nullptr;
    if (oldAxis.Module() == nullptr) {
        partModule = newPart->Modules()[dynamic_cast<PartModule*>(oldAxis.AxisField->host())->ClassName()];
        if (partModule == nullptr) throw Nre();
        baseAxisField = dynamic_cast<BaseAxisField*>(partModule->Fields()[oldAxis.axisName]);
    } else {
        const int num = oldAxis.PartPtr()->Modules().IndexOf(oldAxis.Module());
        if (num > -1) {
            partModule = newPart->Modules()[static_cast<size_t>(num)];
            baseAxisField = dynamic_cast<BaseAxisField*>(partModule->Fields()[oldAxis.axisName]);
        }
    }
    if (baseAxisField != nullptr) {
        auto controlledAxis = std::make_shared<ControlledAxis>(newPart, partModule, baseAxisField, this);
        controlledAxis->timeValue = FloatCurve(oldAxis.timeValue.Curve().keys());
        controlledAxes.push_back(controlledAxis);
    }
}

// :1177
void ModuleRoboticController::AddPartActionForSymmetryPartner(ControlledAction& oldAction, Part* newPart) {
    BaseAction* baseAction = nullptr;
    PartModule* partModule = nullptr;
    if (oldAction.Module() == nullptr) {
        if (ControllerEnvironment::partAction) baseAction = ControllerEnvironment::partAction(*newPart, oldAction.actionName);
    } else {
        const int num = oldAction.PartPtr()->Modules().IndexOf(oldAction.Module());
        if (num > -1) {
            partModule = newPart->Modules()[static_cast<size_t>(num)];
            baseAction = partModule->Actions()[oldAction.actionName];
        }
    }
    if (baseAction != nullptr) {
        auto controlledAction = std::make_shared<ControlledAction>(newPart, partModule, baseAction, this);
        controlledAction->times = oldAction.times;
        controlledActions.push_back(controlledAction);
    }
}

// :1206
bool ModuleRoboticController::HasPartAxisField(Part* testPart, BaseAxisField* testAxisField) {
    ControlledAxis* axis = nullptr;
    return TryGetPartAxisField(testPart, testAxisField, axis);
}

// :1212
bool ModuleRoboticController::HasPartSymmetryAxisField(Part* testPart, BaseAxisField* testAxisField) {
    bool symmetryPartner = false;
    ControlledAxis* action = nullptr;
    return TryGetPartAxisField(testPart, testAxisField, action, symmetryPartner) && symmetryPartner;
}

// :1219
bool ModuleRoboticController::TryGetPartAxisField(Part* testPart, BaseAxisField* testAxisField, ControlledAxis*& axis) {
    bool symmetryPartner = false;
    const bool result = TryGetPartAxisField(testPart, testAxisField, axis, symmetryPartner);
    if (symmetryPartner) {
        axis = nullptr;
        return false;
    }
    return result;
}

// :1231 (the C# do { ... continue; ... } while (cond): `continue` evaluates the loop condition)
bool ModuleRoboticController::TryGetPartAxisField(Part* testPart, BaseAxisField* testAction, ControlledAxis*& action, bool& symmetryPartner) {
    int count = static_cast<int>(controlledAxes.size());
    ControlledAxis* controlledAxis = nullptr;
    PartModule* testModule = dynamic_cast<PartModule*>(testAction->host());
    while (true) {
        if (count-- > 0) {
            controlledAxis = controlledAxes[static_cast<size_t>(count)].get();
            if (controlledAxis->PartPtr() != nullptr && controlledAxis->AxisField != nullptr && controlledAxis->AxisField->name() == testAction->name() &&
                controlledAxis->PartPtr()->isSymmetryCounterPart(testPart)) {
                if (testModule == nullptr) throw Nre();  // testAction.module.PersistentActionsId = ...
                const uint32_t actionsId = ActionId(*testModule);
                if (testModule == nullptr || actionsId == controlledAxis->PersistentActionId()) {
                    action = controlledAxis;
                    symmetryPartner = true;
                    return true;
                }
            }
        } else {
            action = nullptr;
            symmetryPartner = false;
            return false;
        }
        const bool again = !(controlledAxis->PartPtr() != nullptr) || controlledAxis->AxisField == nullptr ||
                           controlledAxis->PartPtr()->fields.persistentId != testPart->fields.persistentId || !(controlledAxis->AxisField->name() == testAction->name()) ||
                           (!(testModule == nullptr) && testModule->PersistentId() != controlledAxis->moduleId);
        if (!again) break;
    }
    action = controlledAxis;
    symmetryPartner = false;
    return true;
}

// :1262
bool ModuleRoboticController::HasPartAction(Part* testPart, BaseAction* testAction) {
    ControlledAction* action = nullptr;
    return TryGetPartAction(testPart, testAction, action);
}

// :1268
bool ModuleRoboticController::TryGetPartAction(Part* testPart, BaseAction* testAction, ControlledAction*& action) {
    bool symmetryPartner = false;
    const bool result = TryGetPartAction(testPart, testAction, action, symmetryPartner);
    if (symmetryPartner) {
        action = nullptr;
        return false;
    }
    return result;
}

// :1280
bool ModuleRoboticController::HasPartSymmetryAction(Part* testPart, BaseAction* testAction) {
    bool symmetryPartner = false;
    ControlledAction* action = nullptr;
    return TryGetPartAction(testPart, testAction, action, symmetryPartner) && symmetryPartner;
}

// :1287
bool ModuleRoboticController::TryGetPartAction(Part* testPart, BaseAction* testAction, ControlledAction*& action, bool& symmetryPartner) {
    int count = static_cast<int>(controlledActions.size());
    ControlledAction* controlledAction = nullptr;
    if (testAction->listParent == nullptr) throw Nre();
    PartModule* testModule = testAction->listParent->module;
    while (true) {
        if (count-- > 0) {
            controlledAction = controlledActions[static_cast<size_t>(count)].get();
            if (controlledAction->PartPtr() != nullptr && controlledAction->Action != nullptr && controlledAction->Action->name == testAction->name &&
                controlledAction->PartPtr()->isSymmetryCounterPart(testPart)) {
                // (the C# stores module.PersistentActionsId = GetPersistenActiontId() when the module is not null, then compares)
                if (testModule == nullptr || ActionId(*testModule) == controlledAction->PersistentActionId()) {
                    action = controlledAction;
                    symmetryPartner = true;
                    return true;
                }
            }
        } else {
            action = nullptr;
            symmetryPartner = false;
            return false;
        }
        const bool again = !(controlledAction->PartPtr() != nullptr) || controlledAction->Action == nullptr ||
                           controlledAction->PartPtr()->fields.persistentId != testPart->fields.persistentId || !(controlledAction->Action->name == testAction->name) ||
                           (!(testModule == nullptr) && testModule->PersistentId() != controlledAction->moduleId);
        if (!again) break;
    }
    action = controlledAction;
    symmetryPartner = false;
    return true;
}

// :1321
bool ModuleRoboticController::HasPart(const std::vector<Part*>& testParts) {
    size_t count = controlledAxes.size();
    while (count-- > 0) {
        ControlledAxis& controlledAxis = *controlledAxes[count];
        for (size_t i = 0; i < testParts.size(); i++)
            if (controlledAxis.PartPtr() != nullptr && controlledAxis.PartPtr()->fields.persistentId == testParts[i]->fields.persistentId) return true;
    }
    return false;
}

// :1338
bool ModuleRoboticController::HasPart(Part* testPart) {
    size_t count = controlledAxes.size();
    while (count-- > 0) {
        ControlledAxis& controlledAxis = *controlledAxes[count];
        if (controlledAxis.PartPtr() != nullptr && controlledAxis.PartPtr()->fields.persistentId == testPart->fields.persistentId) return true;
    }
    return false;
}

// :1355
void ModuleRoboticController::UpdatePartSymmetry(Part* targetPart) {
    size_t count = controlledAxes.size();
    while (count-- > 0) {
        ControlledAxis& controlledAxis = *controlledAxes[count];
        if (controlledAxis.PartPtr() != nullptr && controlledAxis.PartPtr()->fields.persistentId == targetPart->fields.persistentId) controlledAxis.RebuildSymmetryList();
    }
    size_t count2 = controlledActions.size();
    while (count2-- > 0) {
        ControlledAction& controlledAction = *controlledActions[count2];
        if (controlledAction.PartPtr() != nullptr && controlledAction.PartPtr()->fields.persistentId == targetPart->fields.persistentId) controlledAction.RebuildSymmetryList();
    }
    UpdateUIElements();
}

// :1378
void ModuleRoboticController::RemovePartSymmetry(uint32_t oldPartId) {
    size_t count = controlledAxes.size();
    while (count-- > 0) controlledAxes[count]->RebuildSymmetryList({oldPartId});
    size_t count2 = controlledActions.size();
    while (count2-- > 0) controlledActions[count2]->RebuildSymmetryList({oldPartId});
    UpdateUIElements();
}

// :1393
bool ModuleRoboticController::isSameActionId(Part* testPart, ControlledAction& cAction) {
    auto* component = dynamic_cast<ModuleRoboticController*>(testPart->FindModuleOfClass("ModuleRoboticController"));
    if (component == nullptr) throw Nre();
    for (size_t num = 0; num < component->controlledActions.size(); num++)
        if (component->controlledActions[num]->PersistentActionId() == cAction.PersistentActionId()) return true;
    return false;
}

// :1413
void ModuleRoboticController::VesselModified(Vessel* modifiedVessel) {
    Part* p = PartOf();
    if (ServoEnvironment::SceneIsFlight() && modifiedVessel != nullptr && p != nullptr && p->vessel != nullptr && modifiedVessel->persistentId == p->vessel->persistentId)
        RemoveOtherVesselItems();
}

// :1421
void ModuleRoboticController::RemoveOtherVesselItems() {
    if (GameSettings::SERENITY_CONTROLLER_IGNORES_VESSEL) return;
    Part* p = PartOf();
    Vessel* vessel = p != nullptr ? p->vessel : nullptr;
    size_t count = controlledAxes.size();
    while (count-- > 0) {
        if (count >= controlledAxes.size()) throw std::out_of_range("Index was out of range. Must be non-negative and less than the size of the collection.");
        ControlledAxis& a = *controlledAxes[count];
        if (a.PartPtr() != nullptr && a.PartPtr()->vessel != nullptr && vessel != nullptr && a.PartPtr()->vessel->persistentId != vessel->persistentId) {
            log::Info("[ModuleRoboticController]: Removing the " + a.PartNickName() + "(" + std::to_string(a.partId) + ") axis from the controller as its on another vessel");
            RemovePartAxis(*controlledAxes[count], true);
        }
    }
    size_t count2 = controlledActions.size();
    while (count2-- > 0) {
        if (count2 >= controlledActions.size()) throw std::out_of_range("Index was out of range. Must be non-negative and less than the size of the collection.");
        ControlledAction& a = *controlledActions[count2];
        if (a.PartPtr() != nullptr && a.PartPtr()->vessel != nullptr && vessel != nullptr && a.PartPtr()->vessel->persistentId != vessel->persistentId) {
            log::Info("[ModuleRoboticController]: Removing the " + a.PartNickName() + "(" + std::to_string(a.partId) + ") action from the controller as its on another vessel");
            RemovePartAction(*controlledActions[count2], true);
        }
    }
}

// :1447
void ModuleRoboticController::OnSave(ConfigNode& node) {
    PartModule::OnSave(node);
    node.AddValue("sequenceIsPlaying", SequenceIsPlaying());
    node.AddValue("sequenceDirection", std::string(Name(SequenceDirection())));
    node.AddValue("sequenceLoopMode", std::string(Name(SequenceLoop())));
    ConfigNode* configNode = node.AddNode("CONTROLLEDAXES");
    for (size_t i = 0; i < controlledAxes.size(); i++) controlledAxes[i]->Save(*configNode->AddNode("AXIS"));
    ConfigNode* configNode2 = node.AddNode("CONTROLLEDACTIONS");
    for (size_t j = 0; j < controlledActions.size(); j++) controlledActions[j]->Save(*configNode2->AddNode("ACTION"));
}

// :1471
void ModuleRoboticController::OnLoad(const ConfigNode& node) {
    PartModule::OnLoad(node);
    HasWindowDimensions = node.HasValue("windowPosition");
    bool value = false;
    node.TryGetValue("sequenceIsPlaying", value);
    Set<int32_t>("sequencePlayingIndex", value ? 0 : 1);
    SequenceDirectionOptions value2 = SequenceDirectionOptions::Forward;
    node.TryGetEnum("sequenceDirection", value2, SequenceDirectionOptions::Forward, std::span<const std::pair<std::string_view, SequenceDirectionOptions>>(kDirectionMembers));
    Set<int32_t>("sequenceDirectionIndex", static_cast<int32_t>(value2));
    SequenceLoopOptions value3 = SequenceLoopOptions::Once;
    node.TryGetEnum("sequenceLoopMode", value3, SequenceLoopOptions::Once, std::span<const std::pair<std::string_view, SequenceLoopOptions>>(kLoopMembers));
    Set<int32_t>("sequenceLoopIndex", static_cast<int32_t>(value3));
    controlledAxes.clear();
    ConfigNode* node2 = nullptr;
    if (node.TryGetNode("CONTROLLEDAXES", node2)) {
        for (int i = 0; i < node2->nodes().Count(); i++) {
            auto controlledAxis = std::make_shared<ControlledAxis>();
            controlledAxis->Load(*node2->nodes()[i]);
            controlledAxis->Controller = this;
            controlledAxes.push_back(controlledAxis);
        }
    }
    controlledActions.clear();
    ConfigNode* node3 = nullptr;
    if (node.TryGetNode("CONTROLLEDACTIONS", node3)) {
        for (int j = 0; j < node3->nodes().Count(); j++) {
            auto controlledAction = std::make_shared<ControlledAction>();
            controlledAction->Load(*node3->nodes()[j]);
            controlledAction->Controller = this;
            controlledActions.push_back(controlledAction);
        }
    }
}

// :1524
void ModuleRoboticController::AssignReferenceVars_Debug() {
    size_t count = controlledAxes.size();
    while (count-- > 0) {
        if (!controlledAxes[count]->AssignReferenceVars()) {
            log::Warning("[ModuleRoboticController]: Removing the axis we couldnt bind from the controller");
            RemoveAt(controlledAxes, count);
        }
    }
    size_t count2 = controlledActions.size();
    while (count2-- > 0) {
        if (!controlledActions[count2]->AssignReferenceVars()) {
            log::Warning("[ModuleRoboticController]: Removing the action we couldnt bind from the controller");
            RemoveAt(controlledActions, count2);
        }
    }
}

}  // namespace ksp::Expansions::Serenity
