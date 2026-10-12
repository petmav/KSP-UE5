// WP-20d: the part force buffers and the part-module FixedUpdate slot; see include/ksp/physics/part_forces.hpp. C# paths are relative
// to decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/physics/part_forces.hpp"

#include <vector>

#include "ksp/aero/float_eval.hpp"
#include "ksp/aero/lifting_surface.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/modules/module_rcs.hpp"
#include "ksp/modules/module_reaction_wheel.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/krakensbane.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::physics {

// ===================================================================================== Part.cs:8663-8688

// Part.cs:8663
void PartAddForce(Part& part, const Vector3d& vec) {
    if (Part* rb = RigidBodyPart(&part)) rb->fields.force += vec;
}

// Part.cs:8671
void PartAddImpulse(Part& part, const Vector3d& vec) {
    if (Part* rb = RigidBodyPart(&part)) rb->fields.force += vec / static_cast<double>(TimeWarp::fixedDeltaTime());
}

// Part.cs:8679 (no null check in the C#: a part without a RigidBodyPart throws)
void PartAddForceAtPosition(Part& part, const Vector3d& vec, const Vector3d& pos) {
    Part* rb = RigidBodyPart(&part);
    if (rb == nullptr) throw NullReferenceException();
    rb->forces.emplace_back(vec, pos);
}

// Part.cs:8684 (vessel.ResetRBAnchor: the landed anchor is WP-20e's)
void PartAddTorque(Part& part, const Vector3d& vec) {
    if (part.vessel == nullptr) throw NullReferenceException();
    Part* rb = RigidBodyPart(&part);
    if (rb == nullptr) throw NullReferenceException();
    rb->fields.torque += vec;
}

void PartBufferForces::AddRigidbodyForce(Part& part, const Vector3d& force, ForceMode mode) {
    if (binding_ != nullptr) binding_->AddRigidbodyForce(part, force, mode);
}
void PartBufferForces::AddRigidbodyForceAtPosition(Part& part, const Vector3d& force, const Vector3d& position, ForceMode mode) {
    if (binding_ != nullptr) binding_->AddRigidbodyForceAtPosition(part, force, position, mode);
}
void PartBufferForces::SetAngularDrag(Part& part, float angularDrag) {
    if (binding_ != nullptr) binding_->SetAngularDrag(part, angularDrag);
}
void PartBufferForces::AddUniformAcceleration(const Vector3d& acceleration) {
    if (binding_ != nullptr) binding_->AddUniformAcceleration(acceleration);
}

PartTransform ChildTransform(const PartTransform& part, const PartTransform& local) {
    PartTransform w;
    w.position = part.TransformPoint(local.position);
    w.rotation = UnityNative::Multiply(part.rotation, local.rotation);
    w.localScale = local.localScale;
    return w;
}

// ===================================================================================== the module slot

namespace {

// Rigidbody.GetPointVelocity(point) of the part's Rigidbody (the cluster of its RigidBodyPart): v + w x (p - worldCenterOfMass)
Vector3f PointVelocity(const VesselPhysics& vp, const Part& owner, const Vector3f& point) {
    const PartPhysicsState s = vp.PartState(owner);
    const Vector3d p = vp.PhysicsFromTransform(Vector3d(point));
    return (s.velocity + Vector3d::Cross(s.angularVelocity, p - s.centerOfMass)).ToVector3f();
}

// TimeWarp.CurrentRate > 1f && TimeWarp.WarpMode == HIGH (ModuleEngines.cs:1532, ModuleRCS.cs:283)
bool HighWarp() { return TimeWarp::CurrentRate() > 1.0f && TimeWarp::WarpMode() == TimeWarp::Modes::HIGH; }

}  // namespace

void PartModuleForces::FixedUpdate(VesselPhysicsHost& host) {
    engineForces = rcsForces = surfaceForces = wheelTorques = 0;
    const std::vector<Vessel*> vessels = FlightGlobals::Vessels;
    for (Vessel* v : vessels) {
        if (v == nullptr || !v->loaded || v->state == Vessel::State::DEAD) continue;
        FixedUpdateVessel(*v, host.Find(*v));
    }
}

void PartModuleForces::FixedUpdateVessel(Vessel& v, VesselPhysics* vp) {
    const std::vector<Part*> parts = v.parts;
    for (Part* part : parts) {
        if (part == nullptr) continue;
        for (size_t i = 0; i < part->Modules().Count(); ++i) {
            PartModule* m = part->Modules().GetModule(i);
            if (auto* e = dynamic_cast<ModuleEngines*>(m)) {
                if (engines) EngineFixedUpdate(*part, *e, v, vp);
            } else if (auto* r = dynamic_cast<ModuleRCS*>(m)) {
                if (rcs) RcsFixedUpdate(*part, *r, v, vp);
            } else if (auto* ls = dynamic_cast<ModuleLiftingSurface*>(m)) {
                if (!liftingSurfaces) continue;
                // the Unity state of ModuleLiftingSurface.FixedUpdate (:170-176): baseTransform = the part transform (OnStart :133-149
                // falls back to it without a model transform), part.Rigidbody, Rigidbody.GetPointVelocity + the frame velocity
                const PartTransform& bt = part->partTransform;
                Part* owner = RigidBodyPart(part);
                LiftingSurfaceFrame fr;
                fr.hasRigidbody = owner != nullptr && vp != nullptr && !v.packed;
                fr.baseRotation = bt.rotation;
                fr.baseForward = bt.forward();
                fr.baseUp = bt.up();
                fr.baseRight = bt.rotation * Vector3f::right();
                fr.colWorld = bt.TransformPoint(part->fields.CoLOffset);
                fr.copWorld = bt.TransformPoint(part->fields.CoPOffset);
                if (fr.hasRigidbody) {
                    const Vector3f point = ls->Get<bool>("displaceVelocity") ? bt.TransformPoint(ls->Get<Vector3f>("velocityOffset")) : bt.position;
                    fr.pointVelocity = PointVelocity(*vp, *owner, point) + Krakensbane::GetFrameVelocityV3f();
                }
                SurfaceForces f;
                if (auto* cs = dynamic_cast<ModuleControlSurface*>(ls)) {
                    // ModuleControlSurface.FixedUpdate (:366) reads the vessel's control state, CurrentCoM and ReferenceTransform
                    ControlSurfaceFrame cf;
                    cf.surface = fr;
                    cf.hasCtrlSurface = true;
                    cf.ctrlSurfaceAtNeutral = cs->ctrlSurfaceLocalRotation == cs->neutral;
                    cf.currentCoM = v.CurrentCoM();
                    cf.localCoM = bt.InverseTransformPoint(v.CurrentCoM());
                    cf.basePosition = bt.position;
                    cf.referenceRotation = v.ReferenceTransform().rotation;
                    cf.referenceUp = v.ReferenceTransform().up();
                    cf.pitch = v.ctrlState.pitch;
                    cf.yaw = v.ctrlState.yaw;
                    cf.roll = v.ctrlState.roll;
                    cf.fixedDeltaTime = TimeWarp::fixedDeltaTime();
                    f = cs->FixedUpdateAero(cf);
                } else {
                    f = ls->FixedUpdateAero(fr);
                }
                // ModuleLiftingSurface.cs:188-192: part.AddForceAtPosition(liftForce, CoL), then the drag at CoP
                if (f.applied) {
                    PartAddForceAtPosition(*part, Vector3d(f.liftForce), Vector3d(f.liftPosition));
                    ++surfaceForces;
                    if (f.applyDrag) {
                        PartAddForceAtPosition(*part, Vector3d(f.dragForce), Vector3d(f.dragPosition));
                        ++surfaceForces;
                    }
                }
            } else if (auto* w = dynamic_cast<ModuleReactionWheel*>(m)) {
                if (!reactionWheels) continue;
                // ModuleReactionWheel.cs:158 FixedUpdate -> ActiveUpdate: part.AddTorque(the world torque)
                w->torqueApplied = false;
                w->FixedUpdate();
                if (w->torqueApplied && !ReactionWheelEnvironment::addTorque) {
                    PartAddTorque(*part, Vector3d(w->lastTorque));
                    ++wheelTorques;
                }
            }
        }
    }
}

// ModuleEngines.cs:1587 FixedUpdate (Unity's FixedUpdate: runs whatever the PartModule.isEnabled flag says)
void PartModuleForces::EngineFixedUpdate(Part& part, ModuleEngines& e, Vessel& v, VesselPhysics* vp) {
    (void)vp;
    // the thrust transforms are children of the part: their world pose follows the part's (Unity's transform hierarchy)
    if (hooks.engineThrustTransforms) {
        const std::vector<PartTransform> local = hooks.engineThrustTransforms(part, e);
        if (!local.empty()) {
            e.thrustTransforms.resize(local.size());
            for (size_t i = 0; i < local.size(); ++i) e.thrustTransforms[i] = ChildTransform(part.partTransform, local[i]);
        }
    }
    // ModuleEngines.cs:681-697 (OnLoad): without a multiplier per thrust transform every transform gets 1 / count (a module whose
    // transforms were found after OnLoad, e.g. a host that installs the model reader late)
    if (e.thrustTransformMultipliers.size() != e.thrustTransforms.size()) {
        using E = aero::FE<aero::MonoFloat>;
        const float item = aero::S(E(1.0f) / E(static_cast<float>(e.thrustTransforms.size())));
        e.thrustTransformMultipliers.assign(e.thrustTransforms.size(), item);
    }
    EngineFlightInput in;
    in.flightReady = FlightGlobals::ready;
    in.oceanBody = v.mainBody() != nullptr && v.mainBody()->ocean;
    in.fixedDeltaTime = TimeWarp::fixedDeltaTime();
    // ModuleEngines.cs:1516 CheckTransformsUnderwater
    in.underwater = false;
    if (v.mainBody() != nullptr) {
        size_t count = e.thrustTransforms.size();
        while (count-- > 0)
            if (FlightGlobals::getAltitudeAtPos(Vector3d(e.thrustTransforms[count].position), v.mainBody()) < 0.0) {
                in.underwater = true;
                break;
            }
    }
    e.flightInput = in;
    e.warping = HighWarp() || v.packed;  // ModuleEngines.cs:1530 TimeWarping (part.packed: the vessel's)
    e.FixedUpdate();
    if (!e.warping)
        if (auto* fx = dynamic_cast<ModuleEnginesFX*>(&e)) fx->FXUpdate(in.fixedDeltaTime);  // ModuleEnginesFX.cs:59 (the spool)
    // ModuleEngines.cs:1551-1566: part.AddForceAtPosition per thrust transform, then part.AddThermalFlux
    for (const EngineForce& f : e.lastThrust.forces) {
        PartAddForceAtPosition(part, Vector3d(f.force), Vector3d(f.position));
        ++engineForces;
    }
    if (e.lastThrust.hasThermalFlux) part.AddThermalFlux(e.lastThrust.thermalFluxKilowatts);
}

// ModuleRCS.cs:277 FixedUpdate
void PartModuleForces::RcsFixedUpdate(Part& part, ModuleRCS& r, Vessel& v, VesselPhysics* vp) {
    (void)vp;
    if (hooks.rcsThrusters) {
        const std::vector<RCSThruster> local = hooks.rcsThrusters(part, r);
        if (!local.empty()) {
            r.thrusterTransforms.resize(local.size());
            for (size_t i = 0; i < local.size(); ++i) {
                RCSThruster& t = r.thrusterTransforms[i];
                t.position = part.partTransform.TransformPoint(local[i].position);
                t.up = part.partTransform.TransformDirection(local[i].up);
                t.forward = part.partTransform.TransformDirection(local[i].forward);
                t.activeInHierarchy = local[i].activeInHierarchy;
            }
        }
    }
    RCSFlightInput in;
    in.flightScene = true;
    in.highWarp = HighWarp();
    in.hasVessel = true;
    in.rcsActionGroup = v.ActionGroups()[KSPActionGroup::RCS];
    in.currentCoM = v.CurrentCoM();
    in.staticPressureAtm = static_cast<float>(part.fields.staticPressureAtm);
    in.shieldedFromAirstream = part.ShieldedFromAirstream();
    in.precisionMode = hooks.precisionMode;
    in.fixedDeltaTime = TimeWarp::fixedDeltaTime();
    r.shieldedFromAirstream = part.ShieldedFromAirstream();
    if (v.packed) return;  // KSP: a packed vessel's thrusters are off (the FlightIntegrator clears the buffers anyway)
    r.FixedUpdate(in);
    for (const RCSForce& f : r.lastForces) {  // ModuleRCS.cs:357
        PartAddForceAtPosition(part, Vector3d(f.force), Vector3d(f.position));
        ++rcsForces;
    }
}

// ModuleRCS.cs:211 Update (frame rate): the control input of every loaded vessel's RCS modules
void PartModuleForces::Update() {
    if (!rcs) return;
    const std::vector<Vessel*> vessels = FlightGlobals::Vessels;
    for (Vessel* v : vessels) {
        if (v == nullptr || !v->loaded || v->packed) continue;
        for (Part* part : v->parts)
            for (size_t i = 0; i < part->Modules().Count(); ++i)
                if (auto* r = dynamic_cast<ModuleRCS*>(part->Modules().GetModule(i)))
                    r->Update(v->ctrlState, v->ReferenceTransform().rotation, hooks.precisionMode);
    }
}

}  // namespace ksp::physics
