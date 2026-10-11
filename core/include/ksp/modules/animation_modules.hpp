// WP-27 animation-driven modules: ModuleAnimateGeneric.cs, ModuleAnimationGroup.cs, ModuleAnimationSetter.cs, ModuleAnimatorLandingGear.cs
// (IAnimatedModule.cs is ksp/modules/animated_module.hpp).
//
// The Unity Animation / AnimationState objects are the time-driven model of ksp/unity/animation.hpp; the model's animation components are
// provided by DeployablesEnvironment::modelAnimations (the host or a test). The scalar-module events (OnMoving / OnStop) are real events a
// ModuleCargoBay (WP-26) subscribes to; ModuleAnimateGeneric also exposes the cargo bay's failure hooks (FailAnimationForCargoBay /
// RestartFailedAnimationForCargoBay, ModuleCargoBay.cs:885, :915).
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/events/event_data.hpp"
#include "ksp/modules/animated_module.hpp"
#include "ksp/modules/deployables.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/unity/animation.hpp"

namespace ksp {

class Part;

struct AnimationModulesEnvironment {
    static inline float unscaledTime = 0.0f;       // Time.unscaledTime
    static inline float unscaledDeltaTime = 0.02f;  // Time.unscaledDeltaTime
    static inline std::vector<std::pair<std::string, float>> partEffects;  // Part.Effect(name, power) calls
    static inline int checkTransferDialogCalls = 0;  // Part.CheckTransferDialog (crew capacity changes)
    static inline std::vector<std::string> screenMessages;
    static inline std::vector<std::string> printed;  // MonoBehaviour.print texts
    static void Reset();
};

// ModuleAnimationSetter.cs:5
class ModuleAnimationSetter : public PartModule, public IScalarModule {
public:
    explicit ModuleAnimationSetter(const ModuleClassInfo& cls) : PartModule(cls) {}
    std::vector<AnimationState*> animStates;  // :17
    float animState = 0.0f;                   // :19
    float inputState = 0.0f;                  // :21
    int stateCount = 0;                       // :23
    events::EventData<float, float> OnMove{"OnMove"};   // :25
    events::EventData<float> OnStopped{"OnStop"};       // :27
    std::string ScalarModuleID() const override;        // :32
    float GetScalar() const override { return inputState; }  // :34
    bool CanMove() const override { return true; }      // :36
    events::EventData<float, float>* OnMoving() override { return &OnMove; }  // :38
    events::EventData<float>* OnStop() override { return &OnStopped; }        // :40
    IScalarModule* AsScalarModule() override { return this; }
    void OnAwake() override;                            // :42
    void OnStart(StartState state) override;            // :52
    virtual void Update();                              // :57 (Unity message)
    virtual void AnimStartup();                         // :62
    void SetScalar(float inputVal) override;            // :79
    virtual void UpdateAnim();                          // :88 (protected)
    void SetUIRead(bool) override {}                    // :99
    void SetUIWrite(bool) override {}                   // :103
    bool IsMoving() override { return false; }          // :107
    std::string GetModuleDisplayName() override;        // :112
};

// ModuleAnimateGeneric.cs:5
class ModuleAnimateGeneric : public PartModule, public IScalarModule, public IMultipleDragCube {
public:
    explicit ModuleAnimateGeneric(const ModuleClassInfo& cls) : PartModule(cls) {}
    enum class animationStates : int { LOCKED, MOVING, CLAMPED, FIXED };  // :7

    // ---- non-KSPField state ----
    float revertAnimSpeedOnSave = 1.0f;       // :70 private
    bool originalAllowManualControl = true;   // :107
    float revertDeployPercentOnSave = -1.0f;  // :124
    bool originalDeployPercentGUIActive = true;  // :126
    bool animationIsDisabledByVariant = false;   // :150
    bool stopAnimation = false;               // :152
    bool isManuallyMoving = false;            // :154
    std::shared_ptr<Animation> anim;          // :156
    float scalarLerpMaxDelta = 0.0f;          // :160
    float lastClamp = 0.0f;                   // :170
    bool behaviourEnabled = true;             // MonoBehaviour.enabled (FindAnimation clears it)
    BaseEvent* toggleEvt = nullptr;
    BaseAction* toggleAct = nullptr;
    std::unique_ptr<events::EventData<float, float>> onMove;  // :162
    std::unique_ptr<events::EventData<float>> onStop;         // :164

    animationStates aniState() const;
    void set_aniState(animationStates s);
    std::string Str(std::string_view field) const;
    float Progress() const { return Get<float>("animTime"); }  // :151
    bool AnimationIsDisabledByVariant() const { return animationIsDisabledByVariant; }  // :153

    std::string ScalarModuleID() const override;      // :155
    float GetScalar() const override { return Get<float>("animTime"); }  // :157
    bool CanMove() const override;                    // :159
    events::EventData<float, float>* OnMoving() override { return onMove.get(); }  // :171
    events::EventData<float>* OnStop() override { return onStop.get(); }           // :173
    IScalarModule* AsScalarModule() override { return this; }
    bool IsMultipleCubesActive() const override { return Get<bool>("useMultipleDragCubes"); }  // :175
    std::vector<std::string> GetDragCubeNames() const override { return {"A", "B"}; }           // :625
    void AssumeDragCubePosition(const std::string& name) override;                              // :630
    bool UsesProceduralDragCubes() const override { return false; }                             // :664

    void ToggleAction(KSPActionParam& param);          // :178 [KSPAction]
    AnimationState* GetState() { return anim ? (*anim)[Str("animationName")] : nullptr; }  // :186
    Animation* GetAnimation() { return anim.get(); }   // :191
    void Toggle();                                     // :197 [KSPEvent]
    void OnAwake() override;                           // :234
    void OnStart(StartState state) override;           // :250
    void OnSave(ConfigNode& node) override;            // :669
    virtual void FixedUpdate();                        // :381 (Unity message)
    void SetScalar(float t) override;                  // :537
    void SetUIRead(bool state) override;               // :595
    void SetUIWrite(bool state) override;              // :600
    bool IsMoving() override;                          // :606
    void VariantToggleAnimationDisabled(bool disabled) { animationIsDisabledByVariant = disabled; }  // :703 (ModulePartVariants.cs)
    void FailAnimationForCargoBay(float newDeployPercent, bool freezeMidMovement);   // :708 internal
    void RestartFailedAnimationForCargoBay();          // :721 internal
    void OnStoredInInventory();                        // :358
    void OnStoredInInventory(ModuleInventoryPart&) override { OnStoredInInventory(); }  // PartModule.OnStoredInInventory (WP-33)

    void FindAnimation();                              // :309 (private)
    void CheckForRestrictions();                       // :365 (private)
    void AnimationHandler();                           // :471 (private)
    void SetDragState(float b);                        // :615 (private)
    void CheckCrewState();                             // :681 (private)
    void UpdateAnimSwitch();                           // :210 (private)
};

// ModuleAnimationGroup.cs:7
class ModuleAnimationGroup : public PartModule, public IMultipleDragCube {
public:
    explicit ModuleAnimationGroup(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleAnimationGroup() override;

    std::vector<IAnimatedModule*> _Modules;  // :9
    bool hasModules = false;                  // _Modules != null
    float animationStartTime = 0.0f;          // :55 protected
    float pauseStartTime = 0.0f;              // :57
    float savedAnimationSpeed = 0.0f;         // :59 private
    std::string savedAnimationName;           // :61
    bool _lateSetupComplete = false;          // :63
    bool _animationLock = false;              // :65
    std::function<void()> _postAnimationMethod;  // :67
    std::shared_ptr<Animation> DeployAnimation, ActiveAnimation, DeactivateAnimation;  // :72-:76 (properties)
    bool emittersEnabled = false;             // ToggleEmmitters(state): the last state set
    int deployCompleteCalls = 0, retractCompleteCalls = 0;
    bool subscribedPause = false;

    std::string Str(std::string_view field) const;
    bool IsDeployed() const { return Get<bool>("isDeployed"); }

    bool IsMultipleCubesActive() const override { return true; }  // :86
    std::vector<std::string> GetDragCubeNames() const override;   // :639
    void AssumeDragCubePosition(const std::string& name) override;  // :660
    bool UsesProceduralDragCubes() const override { return false; }  // :727

    void OnAwake() override;                        // (binds the events / actions)
    void DeployModule();                            // :89 [KSPEvent]
    void RetractModule();                           // :96 [KSPEvent]
    void DeployModuleAction(KSPActionParam& param);   // :104 [KSPAction]
    void RetractModuleAction(KSPActionParam& param);  // :113
    void ToggleModuleAction(KSPActionParam& param);   // :122
    virtual void Start();                           // :134 (Unity message)
    void OnStart(StartState state) override;        // :174
    void OnInventoryModeDisable();                  // :181
    void OnStoredInInventory();                     // :198
    void OnStoredInInventory(ModuleInventoryPart&) override { OnStoredInInventory(); }  // PartModule.OnStoredInInventory (WP-33)
    virtual void OnDestroy();                       // :208
    void LateUpdate();                              // :240
    void OnLoad(const ConfigNode& node) override;   // :271
    void Update();                                  // :278 (Unity message; OnUpdate in KSP's term)
    virtual void UpdateAnimationTime();             // :312 (protected)
    virtual void OnPause();                         // :350
    virtual void OnUnpause();                       // :363
    void CheckAnimationState();                     // :376 (private)
    void FindModules();                             // :389
    void CheckForActivity();                        // :397
    void ToggleActiveState(int speed, bool state);  // :418
    void ToggleEmmitters(bool state);               // :477
    void SetRetractedState(int speed, bool resetAnimationTime);  // :488
    void DeployComplete();                          // :510
    void RetractComplete();                         // :521
    void SetDeployedState(int speed, bool resetAnimationTime);  // :529
    void SetDragCubes(const std::string& cube);     // :548
    void PlayDeployAnimation(int speed, std::function<void()> postAnimate, bool resetAnimationTime);  // :568 (private)
    void DisableModules();                          // :609
    void EnableModules();                           // :622
    AnimationState* GetCurrentPlayingAnimationState();  // :332 (protected)

private:
    void FindAnimations();
    void StopAnimations();
    void LateSetup();
    void Setup();
    std::shared_ptr<Animation> FirstAnimator(const std::string& clip) const;
};

// ModuleAnimatorLandingGear.cs:1
class ModuleAnimatorLandingGear : public PartModule {
public:
    using PartModule::PartModule;
    void OnAwake() override;                          // (binds the event)
    void InputGearToggle();                           // :7 [KSPEvent]
    void OnStart(StartState state) override;          // :21
};

namespace detail {
void RegisterAnimationModules();  // animation_modules.cpp
}

}  // namespace ksp
