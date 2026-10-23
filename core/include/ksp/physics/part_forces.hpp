// WP-20d: the part force buffers and the part-module FixedUpdate slot of the flight tick.
//
// KSP's per-part force producers never touch a Rigidbody: they write the three buffers of the part that carries the Rigidbody
// (Part.AddForce / AddImpulse / AddForceAtPosition / AddTorque, Part.cs:8663-8688, into RigidBodyPart.force / .forces / .torque), and
// FlightIntegrator.Integrate (FlightIntegrator.cs:715-745) applies and clears them in the same tick (PHYSICS-ENGINE-REVIEW.md 1.3:
// the module FixedUpdates run before FlightIntegrator.FixedUpdate in the measured script order, seq 22-276 vs 278).
//
// The modules ported in WP-21 / WP-23 / WP-24 return their forces as records instead of calling Part.AddForceAtPosition / AddTorque
// (EngineThrustResult, RCSForce, SurfaceForces, ModuleReactionWheel::lastTorque). PartModuleForces is the slot that runs those
// modules' FixedUpdate with the Unity state they read (built from the rigid bodies of the WP-20b binding) and hands every record to
// the part buffers exactly as the C# would have:
//   ModuleEngines / ModuleEnginesFX  FixedUpdate (ModuleEngines.cs:1587: UpdateThrottle, UpdatePropellantStatus, ThrustUpdate :1540):
//                     part.AddForceAtPosition(-transform.forward * finalThrust * multiplier, transform.position) per thrust transform,
//                     part.AddThermalFlux; ModuleEnginesFX.FXUpdate (the spool value). The thrust transforms follow the part every tick
//                     from their PART-SPACE pose (`Hooks::engineThrustTransforms`: the model reader; the WP-17b data in tests).
//   ModuleRCS         FixedUpdate (ModuleRCS.cs:277; thrusters from `Hooks::rcsThrusters`, part space) -> part.AddForceAtPosition
//                     (:357); the per-frame input mapping ModuleRCS.Update (:211) runs in Update().
//   ModuleLiftingSurface / ModuleControlSurface / ModuleAeroSurface  FixedUpdate (ModuleLiftingSurface.cs:170 / ModuleControlSurface.cs:366)
//                     with baseTransform = the part transform (no model transforms) -> part.AddForceAtPosition lift, then drag.
//   ModuleReactionWheel  FixedUpdate (ModuleReactionWheel.cs:158) -> part.AddTorque(lastTorque).
//   WP-21b: ModuleResourceIntake FixedUpdate (ModuleResourceIntake.cs:201: the vessel's atmosphere values, the intake transform's forward from
//                     `Hooks::modelTransform`) -> the part's IntakeAir; ModuleControlSurface.GetPotentialTorque for VesselSAS (ControlSurfaceTorque).
//   WP-20f, the control modules of the measured order (run1-L_calltrace.csv seq 23-28: ModuleCommand, ModuleReactionWheel, ModuleSAS,
//   then per engine part ModuleEngines before ModuleGimbal):
//   ModuleCommand     FixedUpdate (ModuleCommand.cs:461) -> UpdateControlState (Part.isControlSource, the command's resource draw).
//   ModuleSAS         FixedUpdate (ModuleSAS.cs:113): SASServiceLevel, the standalone state.
//   ModuleGimbal      FixedUpdate (ModuleGimbal.cs:161) on the gimbal transforms of the model hierarchy (`Hooks::modelTransformChains`):
//                     every gimbal transform first at its neutral local rotation (`t.localRotation = initRots[i]`), the actuation
//                     maths of WP-23, then the new local rotation kept on the module (newLocalRotations). The thrust transforms below a
//                     gimbal transform follow its CURRENT local rotation the next time they are posed, i.e. the next tick's engine
//                     FixedUpdate (ModuleEngines runs before ModuleGimbal on the same part, so KSP's thrust also lags one tick).
//                     PoseGimbalTransforms puts every gimbal transform on its part's current pose (Unity's hierarchy does that
//                     implicitly): the bubble calls it before the input feed for every vessel whose VesselSAS is connected to
//                     OnAutopilotUpdate, the only reader between two ticks (its GetPotentialTorque).
// Not run here: Part.FixedUpdate itself (temperature / g / pressure limits, crew, ModulesOnFixedUpdate of ACTIVE parts) and every
// module without a force record or control role (they belong to their packages: ModuleAlternator, FX modules ...); exhaust damage
// raycasts; gimbal transforms that are not on the chain of a model transform the host reports (no actuation without a chain).
#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "ksp/math/vector3d.hpp"
#include "ksp/physics/vessel_physics.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {
class Part;
class Vessel;
class ModuleEngines;
class ModuleGimbal;
class ModuleRCS;
class ModuleResourceIntake;
class PartModule;
struct PartTransform;
struct RCSThruster;
}  // namespace ksp

namespace ksp::physics {

// ---- Part.cs:8663-8688 -------------------------------------------------------------------------------------------------------
void PartAddForce(Part& part, const Vector3d& vec);                                // :8663 RigidBodyPart.force += vec
void PartAddImpulse(Part& part, const Vector3d& vec);                              // :8671 force += vec / TimeWarp.fixedDeltaTime
void PartAddForceAtPosition(Part& part, const Vector3d& vec, const Vector3d& pos); // :8679 RigidBodyPart.forces.Add
void PartAddTorque(Part& part, const Vector3d& vec);                               // :8684 RigidBodyPart.torque += vec

// A ForceAccumulator over the part buffers: the Part.AddForce* calls buffer (applied by FlightIntegrator.Integrate), the
// Rigidbody.AddForce* calls, angular drag and the uniform acceleration go straight to the binding (they are Rigidbody calls in the C#).
class PartBufferForces final : public ForceAccumulator {
public:
    explicit PartBufferForces(VesselPhysics* binding = nullptr) : binding_(binding) {}
    void AddForce(Part& part, const Vector3d& force) override { PartAddForce(part, force); }
    void AddImpulse(Part& part, const Vector3d& impulse) override { PartAddImpulse(part, impulse); }
    void AddForceAtPosition(Part& part, const Vector3d& force, const Vector3d& position) override { PartAddForceAtPosition(part, force, position); }
    void AddTorque(Part& part, const Vector3d& torque) override { PartAddTorque(part, torque); }
    void AddRigidbodyForce(Part& part, const Vector3d& force, ForceMode mode) override;
    void AddRigidbodyForceAtPosition(Part& part, const Vector3d& force, const Vector3d& position, ForceMode mode) override;
    void SetAngularDrag(Part& part, float angularDrag) override;
    void AddUniformAcceleration(const Vector3d& acceleration) override;

private:
    VesselPhysics* binding_;
};

// The Unity transform of a child of the part (a model transform) placed on the part's current transform: position =
// partTransform.TransformPoint(local.position), rotation = partTransform.rotation * local.rotation (native quaternion product).
PartTransform ChildTransform(const PartTransform& part, const PartTransform& local);

// ---- WP-20f: the part model's transform hierarchy (PartLoader.CompileModel: the part's "model" child and the .mu nodes below it) -----
// One Unity Transform of the model with its local pose.
struct ModelTransformNode {
    std::string name;
    Vector3f localPosition{};
    Quaternionf localRotation = Quaternionf::identity();
    Vector3f localScale{1.0f, 1.0f, 1.0f};
};
// The chain from the part's "model" child (first) down to one transform (last).
using ModelTransformChain = std::vector<ModelTransformNode>;
inline constexpr size_t kNoChainOverride = static_cast<size_t>(-1);
// Transform.rotation relative to the part: the leaf's local rotation, then rot = parent.localRotation * rot upward (native product; the
// convention of WP-17b's EngineThrustTransforms::ChainRotation that the thrust transforms use). Node `overrideIndex` takes
// `overrideRotation` as its local rotation (a gimbal transform's current one).
Quaternionf ChainPartSpaceRotation(const ModelTransformChain& chain, size_t overrideIndex = kNoChainOverride,
                                   const Quaternionf& overrideRotation = Quaternionf::identity());
// Transform.position relative to the part: p = parent position + parent rotation * (parent lossy scale * local position), root first
// (the composition of tools/wp17/mu_transforms.py part_space_position), with the same override.
Vector3f ChainPartSpacePosition(const ModelTransformChain& chain, size_t overrideIndex = kNoChainOverride,
                                const Quaternionf& overrideRotation = Quaternionf::identity());
// `ancestor` is `chain` or one of its parents (a prefix: the same names and local poses, bit for bit)
bool ChainIsAncestorOrSelf(const ModelTransformChain& ancestor, const ModelTransformChain& chain);
// The part-space pose of a model transform whose chain has node `index` at `localRotation` instead of its neutral one: `neutral` is the
// transform's neutral part-space pose (the host's data); the rotation is recomposed, the position moves by the chain's displacement.
// Bit for bit `neutral` when `localRotation` equals the node's own local rotation (component by component, +0 == -0).
PartTransform ChainPoseWithLocalRotation(const PartTransform& neutral, const ModelTransformChain& chain, size_t index,
                                         const Quaternionf& localRotation);
// Part.FindModelTransforms(name) of every model transform called `name` (chain order = FindModelTransforms order)
using ModelTransformChains = std::function<std::vector<ModelTransformChain>(Part&, const std::string& name)>;
// For ModuleGimbal::findModelTransforms: each gimbal transform placed on the part (neutral) with its local rotation (initRots)
std::function<std::vector<std::pair<PartTransform, Quaternionf>>(Part&, const std::string&)> GimbalFindModelTransformsHook(ModelTransformChains chains);
// For ModuleGimbal::isThrustTransformInChildren (CreateEngineList): the gimbal chain is an ancestor of (or is) the thrust chain
std::function<bool(Part&, const ModuleGimbal&, size_t, const ModuleEngines&, size_t)> ThrustBelowGimbalHook(ModelTransformChains chains);

class PartModuleForces {
public:
    struct Hooks {
        // Part-space thrust transforms of an engine module (position / rotation relative to the part, FindModelTransforms order).
        // Unset or empty: the module's thrustTransforms are left as they are (they then do not follow the part).
        std::function<std::vector<PartTransform>(Part&, const ModuleEngines&)> engineThrustTransforms;
        // Part-space RCS thrusters (position, up, forward relative to the part). Unset: the module's thrusterTransforms are left alone.
        std::function<std::vector<RCSThruster>(Part&, const ModuleRCS&)> rcsThrusters;
        // FlightInputHandler.fetch.precisionMode (ModuleRCS)
        bool precisionMode = false;
        // WP-20f: the model hierarchy (Part.FindModelTransforms with each transform's chain). Unset: gimbals do not actuate (their
        // transforms, if any, stay where ModuleGimbal.OnStart found them) and thrust transforms stay at their neutral pose. The answers
        // are cached per AvailablePart and name (the hierarchy is the prefab's): ClearModelChainCache() after replacing the hook.
        ModelTransformChains modelTransformChains;
        // WP-21b: Part.FindModelTransform(name) placed in PART space (position / rotation relative to the part transform, neutral pose);
        // false when the part's model has no such transform. Used for a control surface's ctrlSurface and an intake's transform. Unset:
        // the control surface is present and the intake's transform is the part's own.
        std::function<bool(Part&, const std::string& name, PartTransform& local)> modelTransform;
    };
    Hooks hooks;
    bool engines = true, rcs = true, liftingSurfaces = true, reactionWheels = true;
    bool command = true, sas = true, gimbals = true;  // WP-20f
    bool intakes = true;                              // WP-21b: ModuleResourceIntake.FixedUpdate

    // WP-20f: every gimbal transform of the loaded vessels (or of one vessel) on its part's current pose at its current local rotation
    // (see above). Between two ticks only VesselSAS reads them in kspcore (ModuleGimbal.GetPotentialTorque); a host that reads them
    // elsewhere calls this first.
    void PoseGimbalTransforms();
    void PoseGimbalTransforms(Vessel& v);
    void ClearModelChainCache() { chainCache_.clear(); }
    // WP-21b: ModuleControlSurface.GetPotentialTorque (ModuleControlSurface.cs:476) for VesselSAS's torque sum (AutopilotEnvironment::
    // torqueProviderHook): true when `m` is a control surface (pos / neg filled), false for any other module
    static bool ControlSurfaceTorque(Vessel& v, PartModule& m, VesselPhysics* vp, Vector3f& pos, Vector3f& neg);

    // seq 22-276: every loaded vessel of the registry (FlightGlobals.Vessels order), its parts in vessel order, each part's modules in
    // module order. `host` gives the vessels' bindings (a vessel without one runs its modules' packed / no-Rigidbody branches).
    void FixedUpdate(VesselPhysicsHost& host);
    void FixedUpdateVessel(Vessel& vessel, VesselPhysics* binding);
    // the frame-rate half (ModuleRCS.Update: the control input mapping)
    void Update();

    // what the last FixedUpdate handed to the part buffers (counts)
    size_t engineForces = 0, rcsForces = 0, surfaceForces = 0, wheelTorques = 0;
    // WP-20f: ModuleGimbal.FixedUpdates that moved transforms; thrust transforms posed off their neutral pose (last FixedUpdate)
    size_t gimbalUpdates = 0, gimbaledThrustTransforms = 0;

private:
    void EngineFixedUpdate(Part& part, ModuleEngines& e, Vessel& v, VesselPhysics* vp);
    void RcsFixedUpdate(Part& part, ModuleRCS& r, Vessel& v, VesselPhysics* vp);
    void GimbalFixedUpdate(Part& part, ModuleGimbal& g, Vessel& v);
    void IntakeFixedUpdate(Part& part, ModuleResourceIntake& in, Vessel& v);  // WP-21b
    // the gimbal's transforms on the part at their current (`neutral` false) or neutral local rotation; false without chains
    bool PoseGimbal(Part& part, ModuleGimbal& g, bool neutral);
    const std::vector<ModelTransformChain>& Chains(Part& part, const std::string& name);
    std::map<std::pair<const void*, std::string>, std::vector<ModelTransformChain>> chainCache_;
};

}  // namespace ksp::physics
