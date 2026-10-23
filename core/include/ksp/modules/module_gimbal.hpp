// WP-23: ModuleGimbal.cs (numeric and data side).
//
// Ported: the [KSPField] schema, OnLoad (the four range fallbacks), OnStart (the transform list and the initial rotations, through the
// model reader hook), the PAW activity flags (UpdatePAWUI / UpdateToggles as BaseField / BaseEvent / BaseAction state), the
// events and actions (ToggleToggles, ToggleAction, LockAction, FreeAction, TogglePitchAction, ToggleYawAction, ToggleRollAction),
// FixedUpdate's actuation maths (returns the new localRotation of every gimbal transform instead of writing Transform.localRotation),
// GimbalRotation (the gimbal angle maths: roll offsets, the range / limiter clamp), CreateEngineList and GetPotentialTorque
// (ITorqueProvider: the interface itself is WP-24's, the method is ported), IsStageable (false), OnActive.
//
// Float policy: MonoFloat (the game): the angle maths is a chain of float32 multiplications; every statement is written with
// aero::FE<MonoFloat>.
//
// Not ported: GetInfo / GetModuleDisplayName text, SetActionsSuffix's guiName rewrites (UI), the Missions adjuster plumbing
// (AdjusterGimbalBase: ApplyControlAdjustments is the identity), the Transform.localRotation write (the caller applies
// `newLocalRotations`).
#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "ksp/aero/float_eval.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class Vessel;
class FlightCtrlState;
class ModuleEngines;
struct PartTransform;

// The Unity scene state of GimbalRotation: Vessel.ReferenceTransform and the gimbal Transform.
struct GimbalFrame {
    const PartTransform* referenceTransform = nullptr;  // vessel.ReferenceTransform
    const PartTransform* gimbalTransform = nullptr;     // t
};

class ModuleGimbal : public PartModule {
public:
    explicit ModuleGimbal(const ModuleClassInfo& cls) : PartModule(cls) {}

    // ---- state that is not a [KSPField] (ModuleGimbal.cs public / private members) ----
    std::vector<PartTransform> gimbalTransforms;
    std::vector<Quaternionf> initRots;
    Vector3f actuationLocal{};
    std::vector<Vector3f> oldActuationLocal;
    Vector3f actuation{};
    std::vector<std::vector<std::pair<ModuleEngines*, float>>> engineMultsList;
    bool engineMultsListCreated = false;                  // `engineMultsList != null`
    std::vector<Quaternionf> newLocalRotations;           // what FixedUpdate asks the host to assign to the transforms

    // The model reader's hook: Part.FindModelTransforms(name) with each transform's localRotation (unset: none).
    // The first of the pair is the world transform, the second Transform.localRotation.
    static inline std::function<std::vector<std::pair<PartTransform, Quaternionf>>(Part&, const std::string&)> findModelTransforms;
    // Part.FindTransformInChildrenExplicit(parent, find) != null (default: the same transform)
    static inline std::function<bool(const PartTransform& parent, const PartTransform& find)> isTransformInChildren;
    // WP-20f: the same question by identity, for a host that knows the model hierarchy (gimbal transform `gimbalIndex` of this module,
    // thrust transform `thrustIndex` of `engine`); asked first when set (physics/part_forces.hpp ThrustBelowGimbalHook)
    static inline std::function<bool(Part&, const ModuleGimbal&, size_t gimbalIndex, const ModuleEngines& engine, size_t thrustIndex)>
        isThrustTransformInChildren;

    // ---- lifecycle ----
    void OnAwake() override;                           // binds the [KSPEvent] / [KSPAction] methods
    void OnLoad(const ConfigNode& node) override;      // :296
    void OnStart(StartState state) override;           // :247
    void OnActive() override;                          // :331 (gimbalActive = true)
    bool IsStageable() override { return false; }      // :326

    // ---- [KSPEvent] / [KSPAction] (:100-150) ----
    void ToggleToggles();
    void ToggleAction(KSPActionParam& param);
    void LockAction(KSPActionParam& param);
    void FreeAction(KSPActionParam& param);
    void TogglePitchAction(KSPActionParam& param);
    void ToggleYawAction(KSPActionParam& param);
    void ToggleRollAction(KSPActionParam& param);

    // ---- maths ----
    // :153 FixedUpdate over explicit inputs (the vessel's reference transform / CoM / control state / TimeWarp.fixedDeltaTime).
    // Returns false when the C# returns before moving the gimbals (not flight, inactive, disabled).
    bool FixedUpdate(const PartTransform& referenceTransform, const Vector3f& currentCoM, const FlightCtrlState& ctrlState,
                     float fixedDeltaTime);
    // :196 GimbalRotation (the transforms are inputs)
    Vector3f GimbalRotation(const PartTransform& referenceTransform, const PartTransform& t, Vector3f PYR, const Vector3f& localCoM) const;
    void CreateEngineList();                            // :336
    // :374 GetPotentialTorque(out pos, out neg)
    void GetPotentialTorque(Vector3f& pos, Vector3f& neg, const PartTransform& referenceTransform, const Vector3f& currentCoM);
    Vector3f ApplyControlAdjustments(const Vector3f& control) const { return control; }  // :450 (no Missions adjusters)

    void UpdatePAWUI();   // :279 (PAW state: BaseField.guiActive / BaseAction.active)
    void UpdateToggles();  // :231

private:
    Part* PartPtr() const;
};

namespace detail {
void RegisterGimbalModule();  // module_gimbal.cpp
}

}  // namespace ksp
