// WP-UE-16a: the FXModule* part modules (FXModuleAnimateThrottle.cs, FXModuleAnimateRCS.cs, FXModuleThrottleEffects.cs + PartModuleFXSetter.cs,
// FXModuleLookAtConstraint.cs, FXModuleConstrainPosition.cs) as plain data.
//
// The arithmetic over throttle / engine state is ported 1:1 (the animation weight / normalizedTime / speed it would write to a Unity
// `AnimationState` are fields of AnimationStateData, published through the module's public members); the model lookups (the Animation
// components, FindModelTransform(s)) are seams of FXModuleEnvironment, and the Transform writes (LookAt, position / rotation copies) become
// ConstraintDescription records: which mover follows which target how, each tracked frame.
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "ksp/engines/engine_interfaces.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/unity/animation_curve.hpp"

namespace ksp {

class Part;
class ModuleRCS;

// UnityEngine.AnimationState as the modules write it
struct AnimationStateData {
    WrapMode wrapMode = WrapMode::Default;
    float weight = 0.0f;
    float speed = 1.0f;
    float normalizedTime = 0.0f;
    int layer = 0;
    bool playing = false;
};

struct FXModuleEnvironment {
    // Part.FindModelComponents<Animation>(): one entry per Animation component of the part's model in order, true when it holds the clip
    // `animationName` (`animation[animationName] != null`). Unset: the part has no Animation components.
    static inline std::function<std::vector<bool>(Part&, const std::string& animationName)> findAnimations;
    // Part.FindModelTransform(name) != null / Part.FindModelTransforms(name).Count (unset: none)
    static inline std::function<int(Part&, const std::string& transformName)> findModelTransformCount;
};

// PartModuleList.FindEngineNearby / FindEngineInterface (PartModuleList.cs:147 / :177): the part, its parent, then its children
IEngineStatus* FindEngineNearby(Part& part, const std::string& engineName, int engineIndex, bool preferMultiMode);

// FXModuleAnimateThrottle.cs:4
class FXModuleAnimateThrottle : public PartModule {
public:
    explicit FXModuleAnimateThrottle(const ModuleClassInfo& cls) : PartModule(cls) {}
    AnimationStateData animation;     // anim[animationName]
    int animationComponent = -1;      // index into the model's Animation components (-1: none)
    bool behaviourEnabled = true;     // MonoBehaviour.enabled (OnStart disables the module when the animation is missing)
    IEngineStatus* engineReference = nullptr;
    void OnStart(StartState state) override;  // :58
    void OnFixedUpdate() override { FixedUpdate(); }
    void FixedUpdate();               // :95
    void OnInventoryModeDisable() override;  // :172
};

// FXModuleAnimateRCS.cs:4
class FXModuleAnimateRCS : public PartModule {
public:
    explicit FXModuleAnimateRCS(const ModuleClassInfo& cls) : PartModule(cls) {}
    std::vector<float> animState;               // :27 public float[]
    std::vector<std::optional<AnimationStateData>> anims;  // one per Animation component: null when it lacks the clip
    bool behaviourEnabled = true;
    ModuleRCS* moduleRCSReference = nullptr;
    void OnStart(StartState state) override;    // :38
    void OnSave(ConfigNode& node) override;     // :85
    void OnLoad(const ConfigNode& node) override;  // :103
    void OnFixedUpdate() override { FixedUpdate(); }
    void FixedUpdate();                         // :117
};

// PartModuleFXSetter.cs:3 + FXModuleThrottleEffects.cs:3
class FXModuleThrottleEffects : public PartModule {
public:
    explicit FXModuleThrottleEffects(const ModuleClassInfo& cls) : PartModule(cls) {}
    std::vector<IScalarModule*> fxModules;      // PartModuleFXSetter.fxModules
    IEngineStatus* engineReference = nullptr;
    void OnAwake() override;                    // PartModuleFXSetter :12
    void OnStart(StartState state) override;    // FXModuleThrottleEffects :33 (-> PartModuleFXSetter.OnStart :21)
    void OnFixedUpdate() override { FixedUpdate(); }
    void OnUpdate() override { Update(); }
    void FixedUpdate();                         // :46
    void Update();                              // :65
    void SetFXModules(float scalar);            // PartModuleFXSetter :41
};

// A tracked constraint: what `mover.LookAt(target, part.up)` / `mover.position = target.position; mover.rotation = target.rotation` would do
struct ConstraintDescription {
    std::string kind;          // "lookAt" or "constrain"
    std::string targetName;
    std::string moverName;     // rotatorsName / moversName
    int moverIndex = 0;        // index among FindModelTransforms(moverName)
    bool upIsPartUp = false;   // LookAt's worldUp argument is the module's transform.up
    bool matchPosition = false, matchRotation = false;  // constrain only
};

// FXModuleLookAtConstraint.cs:5 / FXModuleConstrainPosition.cs:5: TrackMode FixedUpdate / Update / LateUpdate
enum class FXTrackMode : int { FixedUpdate = 0, Update = 1, LateUpdate = 2 };

struct ConstrainedObject {
    int moverCount = 0;        // movers.Count
    bool hasTarget = false;
    std::string targetName;
    std::string moversName;    // rotatorsName (look-at) or moversName (constrain)
};

class FXModuleLookAtConstraint : public PartModule {
public:
    explicit FXModuleLookAtConstraint(const ModuleClassInfo& cls) : PartModule(cls) {}
    FXTrackMode trackingMode = FXTrackMode::FixedUpdate;  // :28 (set by OnLoad from trackingModeString)
    std::vector<ConstrainedObject> ObjectsList;           // :35
    std::vector<ConstraintDescription> tracked;           // what the last Track() did (cleared by TakeTracked)
    void OnStart(StartState state) override;              // :57
    void OnLoad(const ConfigNode& node) override;         // :139
    void OnUpdate() override;                             // Unity Update (:69)
    void OnFixedUpdate() override;                        // FixedUpdate (:77)
    void LateUpdate();                                    // :85
    void OnConstructionModeUpdate() override;             // :93
    void OnConstructionModeFixedUpdate() override;        // :102
    void Track();                                         // :111
    void SetupObjects(ConstrainedObject co);              // :129
    std::vector<ConstraintDescription> TakeTracked();

private:
    bool ready_ = false;
};

class FXModuleConstrainPosition : public PartModule {
public:
    explicit FXModuleConstrainPosition(const ModuleClassInfo& cls) : PartModule(cls) {}
    FXTrackMode trackingMode = FXTrackMode::FixedUpdate;  // :39
    std::vector<ConstrainedObject> ObjectsList;           // :58
    std::vector<ConstraintDescription> tracked;
    void OnLoad(const ConfigNode& node) override;         // :112
    void OnUpdate() override;                             // :56
    void OnFixedUpdate() override;                        // :64
    void LateUpdate();                                    // :72
    void Track();                                         // :80
    void SetupObjects(ConstrainedObject co);              // :102
    std::vector<ConstraintDescription> TakeTracked();
};

namespace detail {
void RegisterFxModules();  // fx_modules.cpp
}

}  // namespace ksp
