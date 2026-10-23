// WP-20d: the part force buffers and the part-module FixedUpdate slot; see include/ksp/physics/part_forces.hpp. C# paths are relative
// to decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/physics/part_forces.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

#include "ksp/aero/float_eval.hpp"
#include "ksp/aero/lifting_surface.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/modules/module_command.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/modules/module_gimbal.hpp"
#include "ksp/modules/module_rcs.hpp"
#include "ksp/modules/module_resource_intake.hpp"
#include "ksp/modules/module_reaction_wheel.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/krakensbane.hpp"
#include "ksp/resources/resource_flow_host.hpp"
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

// ===================================================================================== WP-20f: model transform chains

namespace {

bool SameBits(float a, float b) { return std::memcmp(&a, &b, sizeof(float)) == 0; }
bool SameBits(const Vector3f& a, const Vector3f& b) { return SameBits(a.x, b.x) && SameBits(a.y, b.y) && SameBits(a.z, b.z); }
bool SameBits(const Quaternionf& a, const Quaternionf& b) {
    return SameBits(a.x, b.x) && SameBits(a.y, b.y) && SameBits(a.z, b.z) && SameBits(a.w, b.w);
}
// a local rotation equal to the neutral one component by component (+0 == -0: AngleAxis of a zero actuation may carry a negative zero)
bool SameRotation(const Quaternionf& a, const Quaternionf& b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }
const Quaternionf& LocalRotationOf(const ModelTransformChain& c, size_t i, size_t overrideIndex, const Quaternionf& overrideRotation) {
    return i == overrideIndex ? overrideRotation : c[i].localRotation;
}

}  // namespace

// Unity's Transform.rotation of a model transform relative to the part (the WP-17b convention: leaf first, parent * rot upward)
Quaternionf ChainPartSpaceRotation(const ModelTransformChain& chain, size_t overrideIndex, const Quaternionf& overrideRotation) {
    if (chain.empty()) return Quaternionf::identity();
    Quaternionf rot = LocalRotationOf(chain, chain.size() - 1, overrideIndex, overrideRotation);
    for (size_t i = chain.size() - 1; i-- > 0;) rot = UnityNative::Multiply(LocalRotationOf(chain, i, overrideIndex, overrideRotation), rot);
    return rot;
}

// Unity's Transform.position relative to the part (root first: position += rotation * (lossy scale * localPosition))
Vector3f ChainPartSpacePosition(const ModelTransformChain& chain, size_t overrideIndex, const Quaternionf& overrideRotation) {
    Vector3f pos{};
    Quaternionf rot = Quaternionf::identity();
    Vector3f scl{1.0f, 1.0f, 1.0f};
    for (size_t i = 0; i < chain.size(); ++i) {
        pos = pos + rot * Vector3f::Scale(scl, chain[i].localPosition);
        rot = UnityNative::Multiply(rot, LocalRotationOf(chain, i, overrideIndex, overrideRotation));
        scl = Vector3f::Scale(scl, chain[i].localScale);
    }
    return pos;
}

bool ChainIsAncestorOrSelf(const ModelTransformChain& ancestor, const ModelTransformChain& chain) {
    if (ancestor.empty() || ancestor.size() > chain.size()) return false;
    for (size_t i = 0; i < ancestor.size(); ++i) {
        const ModelTransformNode& a = ancestor[i];
        const ModelTransformNode& b = chain[i];
        if (a.name != b.name || !SameBits(a.localPosition, b.localPosition) || !SameBits(a.localRotation, b.localRotation) ||
            !SameBits(a.localScale, b.localScale))
            return false;
    }
    return true;
}

PartTransform ChainPoseWithLocalRotation(const PartTransform& neutral, const ModelTransformChain& chain, size_t index,
                                         const Quaternionf& localRotation) {
    if (index >= chain.size() || SameRotation(localRotation, chain[index].localRotation)) return neutral;
    PartTransform out = neutral;
    out.rotation = ChainPartSpaceRotation(chain, index, localRotation);
    out.position = neutral.position + (ChainPartSpacePosition(chain, index, localRotation) - ChainPartSpacePosition(chain));
    return out;
}

std::function<std::vector<std::pair<PartTransform, Quaternionf>>(Part&, const std::string&)> GimbalFindModelTransformsHook(ModelTransformChains chains) {
    return [chains](Part& p, const std::string& name) {
        std::vector<std::pair<PartTransform, Quaternionf>> out;
        if (!chains) return out;
        for (const ModelTransformChain& c : chains(p, name)) {
            if (c.empty()) continue;
            PartTransform local;
            local.position = ChainPartSpacePosition(c);
            local.rotation = ChainPartSpaceRotation(c);
            out.emplace_back(ChildTransform(p.partTransform, local), c.back().localRotation);
        }
        return out;
    };
}

std::function<bool(Part&, const ModuleGimbal&, size_t, const ModuleEngines&, size_t)> ThrustBelowGimbalHook(ModelTransformChains chains) {
    return [chains](Part& p, const ModuleGimbal& g, size_t gimbalIndex, const ModuleEngines& e, size_t thrustIndex) {
        if (!chains) return false;
        const std::vector<ModelTransformChain> gc = chains(p, g.Get<std::string>("gimbalTransformName"));
        const std::vector<ModelTransformChain> tc = chains(p, e.Get<std::string>("thrustVectorTransformName"));
        return gimbalIndex < gc.size() && thrustIndex < tc.size() && ChainIsAncestorOrSelf(gc[gimbalIndex], tc[thrustIndex]);
    };
}

// ===================================================================================== the module slot

namespace {

// Rigidbody.GetPointVelocity(point) of the part's Rigidbody (the cluster of its RigidBodyPart): v + w x (p - worldCenterOfMass)
Vector3f PointVelocity(const VesselPhysics& vp, const Part& owner, const Vector3f& point) {
    const PartPhysicsState s = vp.PartState(owner);
    const Vector3d p = vp.PhysicsFromTransform(Vector3d(point));
    return (s.velocity + Vector3d::Cross(s.angularVelocity, p - s.centerOfMass)).ToVector3f();
}

// The Unity state of ModuleLiftingSurface.FixedUpdate (:170-176): baseTransform = the part transform (ModuleLiftingSurface.OnStart :133-149
// falls back to it without a transformName; ModuleControlSurface declares its own `new string transformName` :11, so the base class's stays
// empty and its baseTransform is the part transform too), part.Rigidbody, Rigidbody.GetPointVelocity + the frame velocity
LiftingSurfaceFrame LiftingFrameOf(Part& part, ModuleLiftingSurface& ls, VesselPhysics* vp, Vessel& v) {
    const PartTransform& bt = part.partTransform;
    Part* owner = RigidBodyPart(&part);
    LiftingSurfaceFrame fr;
    fr.hasRigidbody = owner != nullptr && vp != nullptr && !v.packed;
    fr.baseRotation = bt.rotation;
    fr.baseForward = bt.forward();
    fr.baseUp = bt.up();
    fr.baseRight = bt.rotation * Vector3f::right();
    fr.colWorld = bt.TransformPoint(part.fields.CoLOffset);
    fr.copWorld = bt.TransformPoint(part.fields.CoPOffset);
    if (fr.hasRigidbody) {
        const Vector3f point = ls.Get<bool>("displaceVelocity") ? bt.TransformPoint(ls.Get<Vector3f>("velocityOffset")) : bt.position;
        fr.pointVelocity = PointVelocity(*vp, *owner, point) + Krakensbane::GetFrameVelocityV3f();
    }
    return fr;
}

// TimeWarp.CurrentRate > 1f && TimeWarp.WarpMode == HIGH (ModuleEngines.cs:1532, ModuleRCS.cs:283)
bool HighWarp() { return TimeWarp::CurrentRate() > 1.0f && TimeWarp::WarpMode() == TimeWarp::Modes::HIGH; }

}  // namespace

void PartModuleForces::FixedUpdate(VesselPhysicsHost& host) {
    engineForces = rcsForces = surfaceForces = wheelTorques = 0;
    gimbalUpdates = gimbaledThrustTransforms = 0;
    const std::vector<Vessel*> vessels = FlightGlobals::Vessels;
    for (Vessel* v : vessels) {
        // (a vessel destroyed by an earlier one's modules in this pass gets no further callbacks, as in FlightScene)
        if (v == nullptr || std::find(FlightGlobals::Vessels.begin(), FlightGlobals::Vessels.end(), v) == FlightGlobals::Vessels.end()) continue;
        if (!v->loaded || v->state == Vessel::State::DEAD) continue;
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
                // the Unity state of ModuleLiftingSurface.FixedUpdate (:170-176): see LiftingFrameOf
                const PartTransform& bt = part->partTransform;
                LiftingSurfaceFrame fr = LiftingFrameOf(*part, *ls, vp, v);
                SurfaceForces f;
                if (auto* cs = dynamic_cast<ModuleControlSurface*>(ls)) {
                    // ModuleControlSurface.FixedUpdate (:366) reads the vessel's control state, CurrentCoM and ReferenceTransform
                    ControlSurfaceFrame cf;
                    cf.surface = fr;
                    // ctrlSurface = FindModelTransform(transformName) (ModuleControlSurface.cs:294): without a model hook it is assumed present
                    PartTransform ctrlLocal;
                    cf.hasCtrlSurface = !hooks.modelTransform || hooks.modelTransform(*part, cs->Get<std::string>("transformName"), ctrlLocal);
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
                if (hooks.afterSurface) hooks.afterSurface(*part, *ls, v, vp);
            } else if (auto* in = dynamic_cast<ModuleResourceIntake*>(m)) {
                if (intakes) IntakeFixedUpdate(*part, *in, v);
            } else if (auto* c = dynamic_cast<ModuleCommand*>(m)) {
                if (command) c->FixedUpdate();  // ModuleCommand.cs:461 -> UpdateControlState (:466)
            } else if (auto* sm = dynamic_cast<ModuleSAS*>(m)) {
                if (sas) sm->FixedUpdate();     // ModuleSAS.cs:113
            } else if (auto* g = dynamic_cast<ModuleGimbal*>(m)) {
                if (gimbals) GimbalFixedUpdate(*part, *g, v);
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

// ModuleResourceIntake.cs:201 FixedUpdate: the vessel's atmosphere values and the intake transform's forward (FindModelTransform
// (intakeTransformName), placed on the part: `Hooks::modelTransform`; without the hook the intake transform is taken as present and pointing
// along the part's forward)
void PartModuleForces::IntakeFixedUpdate(Part& part, ModuleResourceIntake& in, Vessel& v) {
    IntakeFlightInput fi;
    CelestialBody* body = v.mainBody();
    fi.hasVessel = true;
    fi.atmosphereContainsOxygen = body != nullptr && body->atmosphereContainsOxygen;
    fi.oceanBody = body != nullptr && body->ocean;
    fi.oceanDensity = body != nullptr ? body->oceanDensity : 0.0;
    fi.staticPressurekPa = v.staticPressurekPa;
    fi.srf_vel_direction = v.srf_vel_direction.ToVector3f();
    fi.srfSpeed = v.srfSpeed;
    fi.mach = v.mach;
    fi.atmDensity = v.atmDensity;
    fi.hasIntakeTransform = in.hasIntakeTransform;
    PartTransform world = part.partTransform;
    if (hooks.modelTransform) {
        PartTransform local;
        if (hooks.modelTransform(part, in.Get<std::string>("intakeTransformName"), local)) world = ChildTransform(part.partTransform, local);
    }
    fi.intakeAltitude = body != nullptr ? FlightGlobals::getAltitudeAtPos(Vector3d(world.position), body) : 1.0;
    fi.intakeForward = world.forward();
    fi.fixedDeltaTime = TimeWarp::fixedDeltaTime();
    in.FixedUpdate(fi);
}

// ModuleControlSurface.cs:476 GetPotentialTorque as VesselAutopilot.VesselSAS.GetTotalVesselTorque (VesselAutopilot.cs:474) calls it
// through AutopilotEnvironment::torqueProviderHook: Part.Rigidbody.worldCenterOfMass of the part's rigid body (transform frame)
bool PartModuleForces::ControlSurfaceTorque(Vessel& v, PartModule& m, VesselPhysics* vp, Vector3f& pos, Vector3f& neg) {
    auto* cs = dynamic_cast<ModuleControlSurface*>(&m);
    if (cs == nullptr) return false;
    pos = neg = Vector3f::zero();
    Part* part = dynamic_cast<Part*>(m.part);
    Part* owner = part != nullptr ? RigidBodyPart(part) : nullptr;
    if (part == nullptr || owner == nullptr || vp == nullptr || v.packed) return true;  // no Rigidbody: nothing to read (the C# would throw)
    const LiftingSurfaceFrame fr = LiftingFrameOf(*part, *cs, vp, v);
    const Vector3d rbCoM = Vector3d(vp->TransformFromPhysics(vp->PartState(*owner).centerOfMass));
    cs->GetPotentialTorque(pos, neg, rbCoM, v.CurrentCoM(), v.ReferenceTransform(), fr);
    return true;
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
    // WP-20f: a thrust transform below a gimbal transform follows the gimbal's current local rotation (set by ModuleGimbal.FixedUpdate,
    // which runs after this module on the same part: the previous tick's actuation, as in KSP)
    if (gimbals && hooks.modelTransformChains && hooks.engineThrustTransforms && !e.thrustTransforms.empty()) {
        std::vector<PartTransform> local;
        const std::vector<ModelTransformChain>* thrustChains = nullptr;
        bool moved = false, loaded = false, usable = true;
        for (size_t i = 0; i < part.Modules().Count() && usable; ++i) {
            auto* g = dynamic_cast<ModuleGimbal*>(part.Modules().GetModule(i));
            if (g == nullptr || g->gimbalTransforms.empty() || g->newLocalRotations.size() != g->initRots.size()) continue;
            const std::vector<ModelTransformChain>& gimbalChains = Chains(part, g->Get<std::string>("gimbalTransformName"));
            if (gimbalChains.size() != g->initRots.size()) continue;
            for (size_t k = 0; k < gimbalChains.size() && usable; ++k) {
                if (gimbalChains[k].empty() || SameRotation(g->newLocalRotations[k], g->initRots[k])) continue;  // at its neutral rotation
                if (!loaded) {
                    loaded = true;
                    local = hooks.engineThrustTransforms(part, e);
                    thrustChains = &Chains(part, e.Get<std::string>("thrustVectorTransformName"));
                    usable = local.size() == e.thrustTransforms.size() && thrustChains->size() == local.size();
                    if (!usable) break;
                }
                for (size_t j = 0; j < local.size(); ++j)
                    if (ChainIsAncestorOrSelf(gimbalChains[k], (*thrustChains)[j])) {
                        local[j] = ChainPoseWithLocalRotation(local[j], (*thrustChains)[j], gimbalChains[k].size() - 1, g->newLocalRotations[k]);
                        moved = true;
                        ++gimbaledThrustTransforms;
                    }
            }
        }
        if (moved && usable)
            for (size_t j = 0; j < local.size(); ++j) e.thrustTransforms[j] = ChildTransform(part.partTransform, local[j]);
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

// WP-20f: the model hierarchy is the part prefab's (PartLoader.CompileModel): the answer for one AvailablePart and name is cached
const std::vector<ModelTransformChain>& PartModuleForces::Chains(Part& part, const std::string& name) {
    static const std::vector<ModelTransformChain> none;
    if (part.partInfo == nullptr) return none;  // no prefab, no model
    const auto key = std::make_pair(static_cast<const void*>(part.partInfo), name);
    auto it = chainCache_.find(key);
    if (it == chainCache_.end())
        it = chainCache_.emplace(key, hooks.modelTransformChains ? hooks.modelTransformChains(part, name) : std::vector<ModelTransformChain>{}).first;
    return it->second;
}

// WP-20f: ModuleGimbal's transforms on the part. A gimbal transform's local rotation is initRots[i] (neutral) or the one its last
// FixedUpdate assigned (newLocalRotations[i], the Unity Transform keeps it); its position does not depend on its own rotation.
bool PartModuleForces::PoseGimbal(Part& part, ModuleGimbal& g, bool neutral) {
    if (!hooks.modelTransformChains || g.gimbalTransforms.empty() || g.initRots.size() != g.gimbalTransforms.size()) return false;
    const std::vector<ModelTransformChain>& chains = Chains(part, g.Get<std::string>("gimbalTransformName"));
    if (chains.size() != g.gimbalTransforms.size()) return false;
    const bool actuated = !neutral && g.newLocalRotations.size() == g.initRots.size();
    for (size_t i = 0; i < chains.size(); ++i) {
        if (chains[i].empty()) continue;
        PartTransform local;
        local.position = ChainPartSpacePosition(chains[i]);
        local.rotation = ChainPartSpaceRotation(chains[i], chains[i].size() - 1, actuated ? g.newLocalRotations[i] : g.initRots[i]);
        g.gimbalTransforms[i] = ChildTransform(part.partTransform, local);
    }
    return true;
}

// ModuleGimbal.cs:161 FixedUpdate: the scene guard (HighLogic.LoadedSceneIsFlight; vessel and ReferenceTransform exist here), the
// `gimbalTransforms == null` OnStart (done before the transforms are posed: ModuleGimbal::FixedUpdate would otherwise run it after),
// `t.localRotation = initRots[i]` for every transform before the maths, the new local rotations after it
void PartModuleForces::GimbalFixedUpdate(Part& part, ModuleGimbal& g, Vessel& v) {
    if (!ResourceFlowEnvironment::LoadedSceneIsFlight()) return;
    if (g.Get<bool>("gimbalActive") && g.isEnabled && g.gimbalTransforms.empty() && g.initRots.empty()) g.OnStart(StartState::Flying);
    const bool posed = PoseGimbal(part, g, true);
    const bool moved = g.FixedUpdate(v.ReferenceTransform(), v.CurrentCoM(), v.ctrlState, TimeWarp::fixedDeltaTime());
    if (posed) {
        PoseGimbal(part, g, false);  // an early return leaves the transforms at their current rotation (the C# never reset them)
        if (moved) ++gimbalUpdates;
    }
}

void PartModuleForces::PoseGimbalTransforms() {
    if (!gimbals || !hooks.modelTransformChains) return;
    for (Vessel* v : FlightGlobals::Vessels)
        if (v != nullptr && v->loaded && v->state != Vessel::State::DEAD) PoseGimbalTransforms(*v);
}

void PartModuleForces::PoseGimbalTransforms(Vessel& v) {
    if (!gimbals || !hooks.modelTransformChains) return;
    for (Part* part : v.parts)
        for (size_t i = 0; i < part->Modules().Count(); ++i)
            if (auto* g = dynamic_cast<ModuleGimbal*>(part->Modules().GetModule(i))) PoseGimbal(*part, *g, false);
}

}  // namespace ksp::physics
