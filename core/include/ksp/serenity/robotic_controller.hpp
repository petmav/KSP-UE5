// WP-52: the robotics controller of Breaking Ground (Assembly-CSharp/Expansions/Serenity): ModuleRoboticController.cs (the sequencer: position,
// play speed, direction, loop modes Once / Repeat / PingPong / OnceRestart, the timed action firing windows, the controlled axes and actions,
// symmetry handling, persistence), ControlledAxis.cs (the time -> value FloatCurve of one axis field, the curve editing helpers and the five
// stock presets), ControlledAction.cs (the action trigger times), RoboticControllerManager.cs (the per-frame priority-averaged axis update
// queue). ControlledBase.cs is WP-53 (serenity_misc.hpp).
//
// Seams (ControllerEnvironment): the part-level KSPActions (Part.Actions: not modelled by the WP-13 Part), the manager's scene (windows). Not ported
// (UI): RoboticControllerWindow* (the editor window and its rows / axis graph), CurvePanel*, AxisLimitLine, the PAW slider limits and interaction
// (UpdateUIElements keeps only the counters when the PAW is open), the localized GetInfo text.
//
// UNVERIFIED: AnimationCurve.MoveKey (native): the curve editing helpers replace the key at `index` (tangents as given, no re-smoothing) and keep the
// order; every use in ControlledAxis keeps the key times strictly increasing, so the native clamping to the neighbours' times never applies.
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ksp/core/exceptions.hpp"
#include "ksp/curves/float_curve.hpp"
#include "ksp/editor/editor_types.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/resources/resource_types.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/serenity/base_servo.hpp"
#include "ksp/serenity/serenity_misc.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::Expansions::Serenity {

// ModuleRoboticController.SequenceLoopOptions / SequenceDirectionOptions (ModuleRoboticController.cs:10-21); the opaque declarations are in
// events/game_event_types.hpp
enum class ModuleRoboticController_SequenceLoopOptions : int { Once = 0, Repeat = 1, PingPong = 2, OnceRestart = 3 };
enum class ModuleRoboticController_SequenceDirectionOptions : int { Forward = 0, Reverse = 1 };

// What the controller reads outside the module
struct ControllerEnvironment {
    // Part.Actions[name] (the part's own KSPActions); the WP-13 Part has none: unset = nullptr
    static inline std::function<BaseAction*(Part&, const std::string&)> partAction;
    static void Reset() { partAction = nullptr; }
};

// RoboticControllerManager.cs:6. One per scene; Instance is null until Awake.
class RoboticControllerManager {
public:
    static inline RoboticControllerManager* Instance = nullptr;  // :20
    // Awake (:46): Serenity not installed -> Destroy(this); a second instance is destroyed; else Instance = this
    void Awake();
    void OnDestroy();                                // :63 (the onGameSceneLoadRequested subscription)
    void Update() { UpdateFieldValues(); }           // :68
    void OnSceneLoad(GameScenes) { ClearUpdateQueue(); }  // :73 (CloseAllWindows is UI)
    void QueueFieldUpdate(BaseAxisField* axisField, float newValue, int priority);  // :107
    bool destroyed = false;                          // Object.Destroy(this)
    size_t QueueSize() const { return queue_.size(); }

private:
    struct AxisUpdate {
        int priority = 0;
        float newValue = 0.0f;
        int values = 0;
    };
    void ClearUpdateQueue() { queue_.clear(); }      // :133
    void UpdateFieldValues();                        // :138
    std::vector<std::pair<BaseAxisField*, AxisUpdate>> queue_;  // DictionaryValueList<BaseAxisField, AxisUpdate> (insertion order)
    float updateValueAverage = 0.0f;                 // :39
    friend struct ControllerTestAccess;
};

class ModuleRoboticController;

// ControlledAxis.cs:8
class ControlledAxis : public ControlledBase {
public:
    std::string axisName;                          // :11
    FloatCurve timeValue;                          // :14
    std::vector<BaseAxisField*> SymmetryFields;    // :17 (BaseAxisFieldList)
    BaseAxisField* AxisField = nullptr;            // :47

    float axisMin() const { if (AxisField == nullptr) throw NullReferenceException(); return AxisField->minValue; }  // :51
    float axisMax() const { if (AxisField == nullptr) throw NullReferenceException(); return AxisField->maxValue; }  // :53
    std::string BaseName() const override { return axisName; }  // :49

    ControlledAxis(const ControlledAxis& sourceAxis);  // :55 (NOT a field copy: a new axis on the same part / module / field with a copy of the curve keys)
    ControlledAxis(Part* part, PartModule* module, BaseAxisField* axisField, ModuleRoboticController* controller);  // :61
    ControlledAxis();                              // :74

    void ReverseCurve();                           // :121
    void InvertCurve();                            // :135
    void RescaleCurveTime(float adjustmentRatio, float minSpace = 0.01f);  // :147
    void AlignCurveEnds();                         // :172
    void ClampAllPointValues();                    // :180
    void ClampPointValue(int index);               // :190
    void PresetFlat();                             // :200
    void PresetSine(float cycles, float phase);    // :217
    void PresetSquare(float cycles, float phase);  // :225
    void PresetTriangle(float cycles, float phase);  // :233
    void PresetSaw(float cycles, float phase);     // :241
    void PresetRevSaw(float cycles, float phase);  // :249
    void UpdatePreset(float cycles, float phase);  // :257
    void ClearPresetRefs();                        // :265
    void UpdateFieldValue(float time, float valueMultiple = 1.0f, bool updateSymmetryPartners = true);  // :377
    void UpdateSoftLimits(const Vector2f& newLimits, bool updateSymmetryPartners = true);  // :406
    std::string lastUsedRefName;                   // :29

protected:
    bool OnChangeSymmetryMaster(Part* newPart, uint32_t& oldPartId) override;  // :355
    bool OnAssignReferenceVars() override;         // :426
    void ClearSymmetryLists() override;            // :460
    void AddSymmetryPart(Part* part) override;     // :472
    void OnLoad(const ConfigNode& node) override;  // :480
    void OnSave(ConfigNode& node) override;        // :495

private:
    bool GenerateCurveFromReference(const std::string& refCurveID, float cycles, float phase, const FloatCurve* referenceCurve, FloatCurve& generatedCurve);  // :271
    static const FloatCurve& RefSine();
    static const FloatCurve& RefSquare();
    static const FloatCurve& RefTriangle();
    static const FloatCurve& RefSaw();
    static const FloatCurve& RefRevSaw();
    float lastUsedRefMin = 0.0f;                   // :31
    float lastUsedRefMax = 0.0f;                   // :33
    std::shared_ptr<FloatCurve> lastUsedRefCurve;  // :35
    std::shared_ptr<AnimationCurve> lastUsedCurve;  // :37
    std::vector<Keyframe> lastUsedRefKeys;         // :39
    bool lastUsedRefKeysSet = false;               // lastUsedRefKeys != null
    int refCurveCyclesStepIncrement = 10;          // :41
    int refCurveCyclesPadding = 4;                 // :43
};

// ControlledAction.cs:8
class ControlledAction : public ControlledBase {
public:
    std::string actionName;                        // :11
    std::vector<float> times;                      // :14
    std::vector<BaseAction*> SymmetryActions;      // :17
    BaseAction* Action = nullptr;                  // :21
    std::string BaseName() const override { return actionName; }  // :23

    ControlledAction(const ControlledAction& sourceAction);  // :25 (a new action on the same part / module / action with a copy of the times)
    ControlledAction(Part* part, PartModule* module, BaseAction* action, ModuleRoboticController* controller);  // :31
    ControlledAction();                            // :41
    void ReverseTimes();                           // :48
    void RescaleTimes(float adjustmentRatio, float minSpace = 0.01f);  // :60

protected:
    bool OnChangeSymmetryMaster(Part* newPart, uint32_t& oldPartId) override;  // :78
    bool OnAssignReferenceVars() override;         // :100
    void ClearSymmetryLists() override;            // :130
    void AddSymmetryPart(Part* part) override;     // :141
    void OnLoad(const ConfigNode& node) override;  // :162
    void OnSave(ConfigNode& node) override;        // :186

private:
    float tempTime = 0.0f;                         // :19
};

// ModuleRoboticController.cs:8
class ModuleRoboticController : public PartModule, public IShipConstructIDChanges, public IResourceConsumer {
public:
    using SequenceLoopOptions = ModuleRoboticController_SequenceLoopOptions;
    using SequenceDirectionOptions = ModuleRoboticController_SequenceDirectionOptions;

    explicit ModuleRoboticController(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleRoboticController() override;
    Part* PartOf() const;

    // ---- state ----
    int priority = 0;                              // :69
    std::string consumptionString;                 // :77
    std::vector<std::shared_ptr<ControlledAxis>> controlledAxes;      // :80 (ControlledAxes / the obsolete Axes)
    std::vector<std::shared_ptr<ControlledAction>> controlledActions; // :83 (ControlledActions)
    bool hasEnoughResources = false;               // :87
    bool leavingScene = false;                     // :89
    BaseAxisField* sequenceAxisPositionAxis = nullptr;  // :99
    float updateTime = 0.0f;                       // :101
    float actionCheck1StartTime = 0.0f, actionCheck1EndTime = 0.0f, actionCheck2StartTime = 0.0f, actionCheck2EndTime = 0.0f;  // :103-:109
    KSPActionParam controllerActionParam{0, KSPActionType::Toggle};   // :111
    std::vector<BaseAction*> actionsToFire;        // :113
    bool pausedInWarp = false;                     // :115
    bool sequenceWasPlaying = false;               // :119
    bool HasWindowDimensions = false;              // :127
    bool destroyed = false;                        // Object.Destroy(this / gameObject)
    bool behaviourEnabled = true;
    std::vector<std::string> uiLog;                // (C++) "OpenControllerEditor" and the UpdateUIElements calls the UI would see

    // ---- properties ----
    int Priority() const { return priority; }                                          // :121
    Vector2f WindowPosition() const { return Get<Vector2f>("windowPosition"); }        // :123
    Vector2f WindowSize() const { return Get<Vector2f>("windowSize"); }                // :125
    uint32_t PartPersistentId() const;                                                 // :129
    float SequencePosition() const { return Get<float>("sequencePosition"); }          // :148
    float SequenceLength() const { return Get<float>("sequenceLength"); }              // :150
    float SequencePlaySpeed() const { return Get<float>("sequencePlaySpeed"); }        // :152
    bool SequenceIsPlaying() const { return Get<int32_t>("sequencePlayingIndex") == 0; }  // :154
    SequenceDirectionOptions SequenceDirection() const { return static_cast<SequenceDirectionOptions>(Get<int32_t>("sequenceDirectionIndex")); }  // :166
    SequenceLoopOptions SequenceLoop() const { return static_cast<SequenceLoopOptions>(Get<int32_t>("sequenceLoopIndex")); }  // :178
    bool controllerEnabled() const { return Get<bool>("controllerEnabled"); }
    std::vector<const PartResourceDefinition*> GetConsumedResources() override { return consumedResources_; }  // :190

    // ---- lifecycle ----
    void OnAwake() override;                       // :306
    void OnStart(StartState state) override;       // :355
    void OnUpdate() override;                      // :418
    void Update();                                 // :427
    void FixedUpdate();                            // :436
    void Start();                                  // :297
    void OnDestroy();                              // :333
    void OnSceneLoadRequested(GameScenes) { leavingScene = true; }  // :328
    void OnSave(ConfigNode& node) override;        // :1447
    void OnLoad(const ConfigNode& node) override;  // :1471
    void UpdatePersistentIDs(const std::map<uint32_t, uint32_t>& changedIDs) override;  // :957

    // ---- the sequence ----
    void TogglePlayAction(KSPActionParam&) { TogglePlay(); }            // :196
    void ToggleLoopModeAction(KSPActionParam&) { CycleLoopMode(); }     // :202
    void ToggleDirectionAction(KSPActionParam&) { ToggleDirection(); }  // :208
    void ToggleControllerEnabledAction(KSPActionParam&) { ToggleControllerEnabled(); }  // :214
    void ToggleControllerEnabledOn(KSPActionParam&) { ToggleControllerEnabled(true); }  // :220
    void ToggleControllerEnabledOff(KSPActionParam&) { ToggleControllerEnabled(false); }  // :226
    void PlaySequenceAction(KSPActionParam&) { TogglePlay(true); }      // :232
    void StopSequenceAction(KSPActionParam&) { TogglePlay(false); }     // :238
    void SequenceForwardAction(KSPActionParam&) { SetDirection(SequenceDirectionOptions::Forward); }  // :244
    void SequenceReverseAction(KSPActionParam&) { SetDirection(SequenceDirectionOptions::Reverse); }  // :250
    void SequenceLoopOnceAction(KSPActionParam&) { SetLoopMode(SequenceLoopOptions::Once); }          // :256
    void SequenceLoopRepeatAction(KSPActionParam&) { SetLoopMode(SequenceLoopOptions::Repeat); }      // :262
    void SequenceLoopPingPongAction(KSPActionParam&) { SetLoopMode(SequenceLoopOptions::PingPong); }  // :268
    void SequenceLoopOnceRestartAction(KSPActionParam&) { SetLoopMode(SequenceLoopOptions::OnceRestart); }  // :274
    void SequencePlaySpeedZeroAction(KSPActionParam&) { SetPlaySpeed(0.0f); }    // :280
    void SequencePlaySpeedFullAction(KSPActionParam&) { SetPlaySpeed(100.0f); }  // :286
    void OpenControllerEditor() { uiLog.push_back("OpenControllerEditor"); }     // :292 (RoboticControllerWindow.Spawn: UI)
    void UpdatePlayingSequencePosition();          // :451
    void CheckAndFireActions();                    // :466
    void OnWarpRateChanged();                      // :518
    void ToggleControllerEnabled() { ToggleControllerEnabled(!controllerEnabled()); }  // :530
    void ToggleControllerEnabled(bool enabled);    // :535
    void TogglePlay() { TogglePlay(!SequenceIsPlaying()); }  // :540
    void TogglePlay(bool play);                    // :545
    void SequencePlay();                           // :557
    void SequenceStop();                           // :572
    void ToggleDirection();                        // :583
    void SetDirection(SequenceDirectionOptions newDirection);  // :596
    void CycleLoopMode();                          // :607
    void SetLoopMode(SequenceLoopOptions newMode); // :628
    void SetDisplayName(const std::string& newName);  // :639
    void SetLength(float newLength);               // :645
    void SetLength(float newLength, bool maintainPointsTime);  // :662
    void SetSequencePosition(float newPosition);   // :678
    void SetPlaySpeed(float newSpeed);             // :790
    void SetSequencePositionStart() { SetSequencePosition(0.0f); }  // :796
    void SetSequencePositionEnd() { SetSequencePosition(SequenceLength()); }  // :801
    void SetSequencePositionPrevKey();             // :806
    void SetSequencePositionNextKey();             // :837
    void UpdateUIElements();                       // :884 (counters only; the PAW and window are UI)
    void sequenceFieldChanged();                   // :912
    void PriorityChanged();                        // :921
    void PartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId);  // :930
    void onEditorPartEvent(ConstructionEventType evt, Part* targetPart);  // :975
    void OnPartWillDie(Part* targetPart) { PartDeleted(targetPart); }  // :991
    void SetWindowSizeAndPosition(const Vector2f& position, const Vector2f& size);  // :1008 (the RectTransform is UI)

    // ---- axes and actions ----
    void AddPartAxis(Part* targetPart, PartModule* module, BaseAxisField* axisField);     // :1015
    void AddPartAction(Part* targetPart, PartModule* module, BaseAction* action);         // :1027
    void PartDeleted(Part* targetPart);                                                   // :1039
    void RemovePartAxis(Part* targetPart, BaseAxisField* axisField, bool transferToSymPartner);  // :1046
    void RemovePartAction(Part* targetPart, BaseAction* action, bool transferToSymPartner);      // :1091
    void RemovePartAxis(ControlledAxis& axis, bool transferToSymPartner);           // :1136
    void RemovePartAction(ControlledAction& action, bool transferToSymPartner);     // :1141
    bool HasPartAxisField(Part* testPart, BaseAxisField* testAxisField);            // :1206
    bool HasPartSymmetryAxisField(Part* testPart, BaseAxisField* testAxisField);    // :1212
    bool TryGetPartAxisField(Part* testPart, BaseAxisField* testAxisField, ControlledAxis*& axis);  // :1219
    bool TryGetPartAxisField(Part* testPart, BaseAxisField* testAction, ControlledAxis*& action, bool& symmetryPartner);  // :1231
    bool HasPartAction(Part* testPart, BaseAction* testAction);                     // :1262
    bool TryGetPartAction(Part* testPart, BaseAction* testAction, ControlledAction*& action);  // :1268
    bool HasPartSymmetryAction(Part* testPart, BaseAction* testAction);             // :1280
    bool TryGetPartAction(Part* testPart, BaseAction* testAction, ControlledAction*& action, bool& symmetryPartner);  // :1287
    bool HasPart(const std::vector<Part*>& testParts);                              // :1321
    bool HasPart(Part* testPart);                                                   // :1338
    void UpdatePartSymmetry(Part* targetPart);                                            // :1355
    void RemovePartSymmetry(uint32_t oldPartId);                                    // :1378
    bool isSameActionId(Part* testPart, ControlledAction& cAction);                 // :1393
    void VesselModified(Vessel* modifiedVessel);                                    // :1413
    void RemoveOtherVesselItems();                                                  // :1421
    void AssignReferenceVars_Debug();                                               // :1524

private:
    void AddPartAxisForSymmetryPartner(ControlledAxis& oldAxis, Part* newPart);     // :1146
    void AddPartActionForSymmetryPartner(ControlledAction& oldAction, Part* newPart);  // :1177
    std::vector<const PartResourceDefinition*> consumedResources_;
};

void RegisterRoboticModules();   // serenity/robotic_register.cpp: ModuleRoboticController (called from RegisterBuiltinModules)

}  // namespace ksp::Expansions::Serenity
