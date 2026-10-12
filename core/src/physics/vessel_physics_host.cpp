// WP-20b: VesselPhysicsHost, the Vessel::env wiring of the reduced model (see ksp/physics/vessel_physics.hpp).
// WP-20c: the structural game events (deferred rebinds), the module force hooks and the joint breaks (PartJoint.OnJointBreak).
#include <algorithm>
#include <chrono>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "ksp/modules/module_docking_node.hpp"
#include "ksp/physics/reference_frame.hpp"
#include "ksp/physics/vessel_physics.hpp"

namespace ksp::physics {

using GE = events::GameEvents;

struct VesselPhysicsHost::SavedHooks {
    std::function<void(Part&, const Vector3f&)> addForce;
    std::function<void(Part&, const Vector3f&, const Vector3f&)> addForceAtPosition;
    std::function<void(Part&, const Vector3f&)> addTorque;
    std::function<Vector3f(Part&)> partVelocity;
};

VesselPhysicsHost::VesselPhysicsHost(PhysicsWorld& world, const VesselPhysicsOptions& opts) : options(opts), world_(&world) {}

VesselPhysicsHost::~VesselPhysicsHost() {
    Uninstall();
    DestroyAll();
}

void VesselPhysicsHost::ApplyFrame(VesselPhysics& vp) const {
    if (vp.VesselPtr() == nullptr) return;
    Vessel& v = vp.GetVessel();
    if (frameOriginWorld) vp.frameOriginWorld = frameOriginWorld(v);
    else vp.frameOriginWorld = v.mainBody() != nullptr ? v.mainBody()->position : Vector3d();
    vp.floatOrigin = floatOrigin;
}

// OrbitDriver.updateFromParameters (OrbitDriver.cs:283-284) places the vessel transform at referenceBody.position + pos -
// rotation * localCoM, i.e. CoMD - rotation * localCoM with the packed CoMD of CalculatePhysicsStats (VesselPrecalculate.cs:469-480:
// the orbit position, or worldSurfacePos + worldSurfaceRot * localCoM when landed).
VesselPose VesselPhysicsHost::DefaultUnpackPose(Vessel& v) const {
    VesselPose pose;
    pose.rotation = QuaternionD(v.vesselTransform.rotation);
    const Vector3d rootWorld = v.CoMD - pose.rotation * Vector3d(v.localCoM);
    const Vector3d origin = frameOriginWorld ? frameOriginWorld(v) : (v.mainBody() != nullptr ? v.mainBody()->position : Vector3d());
    pose.position = ReferenceFrame::PhysicsPosition(rootWorld - origin);
    return pose;
}

VesselPhysics* VesselPhysicsHost::Find(const Vessel& vessel) const {
    for (const auto& vp : vessels_)
        if (vp->VesselPtr() == &vessel) return vp.get();
    return nullptr;
}

VesselPhysics& VesselPhysicsHost::Create(Vessel& vessel, const VesselPose& pose) {
    Destroy(vessel);
    auto vp = std::make_unique<VesselPhysics>(*world_, vessel, pose, options);
    ApplyFrame(*vp);
    vp->SyncPartTransforms();
    vessels_.push_back(std::move(vp));
    return *vessels_.back();
}

VesselPhysics& VesselPhysicsHost::Adopt(std::unique_ptr<VesselPhysics> physics) {
    if (physics->VesselPtr() != nullptr) Destroy(physics->GetVessel());
    ApplyFrame(*physics);
    physics->SyncPartTransforms();
    vessels_.push_back(std::move(physics));
    return *vessels_.back();
}

void VesselPhysicsHost::Forget(const VesselPhysics* vp) {
    pendingSources_.erase(std::remove(pendingSources_.begin(), pendingSources_.end(), vp), pendingSources_.end());
}

void VesselPhysicsHost::Destroy(const Vessel& vessel) {
    for (const auto& vp : vessels_)
        if (vp->VesselPtr() == &vessel) Forget(vp.get());
    vessels_.erase(std::remove_if(vessels_.begin(), vessels_.end(), [&](const std::unique_ptr<VesselPhysics>& vp) { return vp->VesselPtr() == &vessel; }),
                   vessels_.end());
    pendingTargets_.erase(std::remove(pendingTargets_.begin(), pendingTargets_.end(), &vessel), pendingTargets_.end());
}

void VesselPhysicsHost::DestroyAll() {
    vessels_.clear();
    pendingSources_.clear();
    pendingTargets_.clear();
}

VesselPhysics* VesselPhysicsHost::OnVesselSplit(Vessel& original, Vessel& split) {
    VesselPhysics* vp = Find(original);
    if (vp == nullptr) return nullptr;
    std::unique_ptr<VesselPhysics> part = vp->SplitOff(split);
    ApplyFrame(*vp);
    vp->SyncPartTransforms();
    return &Adopt(std::move(part));
}

VesselPhysics* VesselPhysicsHost::OnVesselMerge(Vessel& a, Vessel& b, Vessel& merged) { return OnVesselMerge(Find(a), Find(b), merged); }

VesselPhysics* VesselPhysicsHost::OnVesselMerge(VesselPhysics* pa, VesselPhysics* pb, Vessel& merged) {
    if (pa == nullptr || pb == nullptr) return nullptr;
    std::unique_ptr<VesselPhysics> m = VesselPhysics::Merge(*pa, *pb, merged);
    Forget(pa);
    Forget(pb);
    vessels_.erase(std::remove_if(vessels_.begin(), vessels_.end(),
                                  [&](const std::unique_ptr<VesselPhysics>& vp) { return vp.get() == pa || vp.get() == pb; }),
                   vessels_.end());
    return &Adopt(std::move(m));
}

void VesselPhysicsHost::RefreshFrames() {
    ProcessStructuralChanges();
    for (auto& vp : vessels_) ApplyFrame(*vp);
}

void VesselPhysicsHost::IntegrateAll() {
    ProcessStructuralChanges();
    for (auto& vp : vessels_)
        if (vp->VesselPtr() != nullptr) vp->IntegratePartBuffers(vp->GetVessel().precalc.integrationAccel);
}

void VesselPhysicsHost::FlushAll(float dt) {
    ProcessStructuralChanges();
    for (auto& vp : vessels_) vp->Flush(dt);
}

void VesselPhysicsHost::AfterStepAll() {
    for (auto& vp : vessels_) vp->AfterStep();
    // the joint breaks of the step, binding by binding (creation order), joint by joint
    lastBreaks_.clear();
    std::vector<VesselPhysics*> bound;
    for (auto& vp : vessels_) bound.push_back(vp.get());
    for (VesselPhysics* vp : bound) {
        const std::vector<VesselPhysics::JointBreak> breaks = vp->Breaks();
        for (const VesselPhysics::JointBreak& b : breaks) HandleBreak(*vp, b);
    }
    ProcessStructuralChanges();
}

void VesselPhysicsHost::Step(float dt) {
    FlushAll(dt);
    world_->Step(dt);
    AfterStepAll();
}

// ===================================================================================== WP-20c: breaks and structural events

// PartJoint.cs:397 OnJointBreak for a PartJoint whose ConfigurableJoints `b.mask` PhysX broke (Unity removes them after the message):
// with the host vessel ignoring collisions the part stays (the joint loses those sub-joints; with none left the clusters split inside the
// vessel); otherwise GameEvents.onPartJointBreak and child.OnPartJointBreak (Part.cs:3159 -> Part.decouple, whose event rebinds), and the
// PartJoint is destroyed with all its sub-joints. Unity passes OnJointBreak the joint's break force: here the broken sub-joint's
// breakForce (PartJoint.cs:340), so OnPartJointBreak takes its `breakForce > 0` branch (the onJointBreak report).
void VesselPhysicsHost::HandleBreak(VesselPhysics& vp, const VesselPhysics::JointBreak& b) {
    if (b.joint >= vp.Joints().size()) return;
    const PartJointSpec spec = vp.Joints()[b.joint];
    BreakRecord rec;
    rec.child = spec.child;
    rec.parent = spec.parent;
    rec.joint = b.joint;
    rec.mask = b.mask;
    rec.breakForce = spec.breakForce;
    rec.force = b.force;
    rec.torque = b.torque;
    Vessel* hostVessel = spec.host != nullptr ? spec.host->vessel : nullptr;
    if (hostVessel != nullptr && hostVessel->IgnoreCollisionsFrames() > 0) {  // :399
        vp.DestroySubJoints(b.joint, b.mask);
        lastBreaks_.push_back(rec);
        return;
    }
    PartJoint joint(spec);
    GE::onPartJointBreak.Fire(&joint, rec.breakForce);  // :407
    if (spec.child != nullptr && spec.child->vessel != nullptr) {
        const bool wasRoot = spec.child->vessel->rootPart == spec.child;
        spec.child->OnPartJointBreak(rec.breakForce);  // :408
        rec.decoupled = !wasRoot;
    }
    lastBreaks_.push_back(rec);
}

void VesselPhysicsHost::MarkRebind(VesselPhysics* source, Vessel* target) {
    if (source != nullptr && std::find(pendingSources_.begin(), pendingSources_.end(), source) == pendingSources_.end())
        pendingSources_.push_back(source);
    if (target != nullptr && std::find(pendingTargets_.begin(), pendingTargets_.end(), target) == pendingTargets_.end())
        pendingTargets_.push_back(target);
}

void VesselPhysicsHost::RequestRebind(Vessel& vessel) {
    if (VesselPhysics* vp = Find(vessel)) MarkRebind(vp, &vessel);
}

// Part.cs:2833 onPartDeCoupleNewVesselComplete(original, new): both vessels are rebound from the original's binding (or from the
// pending rebind the original is already part of: several decouples in one frame)
void VesselPhysicsHost::OnDecoupleNewVessel(Vessel* original, Vessel* created) {
    if (original == nullptr) return;
    VesselPhysics* vp = Find(*original);
    const bool pending = std::find(pendingTargets_.begin(), pendingTargets_.end(), original) != pendingTargets_.end();
    if (vp == nullptr && !pending) return;
    MarkRebind(vp, original);
    MarkRebind(nullptr, created);
}

// Part.cs:3352 onVesselsUndocking(old, new)
void VesselPhysicsHost::OnVesselsUndocking(Vessel* original, Vessel* created) { OnDecoupleNewVessel(original, created); }

// Part.cs:3173 onPartCouple(from, to), fired before the tree operation: from.vessel is destroyed by Couple (:3201), so its binding lets
// go of the Vessel now; both bindings are rebound into to.vessel, which then holds every part
void VesselPhysicsHost::OnPartCouple(GE::FromToAction<Part*, Part*> action) {
    if (action.from == nullptr || action.to == nullptr || action.from->vessel == nullptr || action.to->vessel == nullptr) return;
    Vessel* source = action.from->vessel;
    Vessel* target = action.to->vessel;
    VesselPhysics* a = Find(*source);
    VesselPhysics* b = Find(*target);
    const bool pendingA = std::find(pendingTargets_.begin(), pendingTargets_.end(), source) != pendingTargets_.end();
    const bool pendingB = std::find(pendingTargets_.begin(), pendingTargets_.end(), target) != pendingTargets_.end();
    if (a == nullptr && b == nullptr && !pendingA && !pendingB) return;
    if (a != nullptr) {
        MarkRebind(a, nullptr);
        a->DetachVessel();
    }
    pendingTargets_.erase(std::remove(pendingTargets_.begin(), pendingTargets_.end(), source), pendingTargets_.end());
    MarkRebind(b, target);
}

// Part.cs:4464 onPartDie: the part has left its vessel; the vessel is rebound without it
void VesselPhysicsHost::OnPartDie(Part* part) {
    if (part == nullptr || part->vessel == nullptr) return;
    if (VesselPhysics* vp = Find(*part->vessel)) MarkRebind(vp, part->vessel);
}

// onVesselWasModified -> Part.VesselModified (Part.cs:7214) -> CycleAutoStrut for the vessel's parts; a binding whose parts or rigid
// bodies no longer match its vessel is rebound
void VesselPhysicsHost::OnVesselWasModified(Vessel* vessel) {
    if (vessel == nullptr) return;
    VesselPhysics* vp = Find(*vessel);
    if (vp == nullptr) return;
    vp->CycleAutoStruts();
    bool same = vessel->parts.size() == vp->PartCount();
    for (size_t i = 0; same && i < vessel->parts.size(); ++i) same = vp->PartIndex(*vessel->parts[i]) == static_cast<int>(i) && vp->PartHasRigidbody(i) == vessel->parts[i]->hasRigidbody;
    if (!same) MarkRebind(vp, vessel);
}

void VesselPhysicsHost::ProcessStructuralChanges() {
    if (pendingSources_.empty() && pendingTargets_.empty()) return;
    const auto t0 = std::chrono::steady_clock::now();
    const std::vector<VesselPhysics*> sources = pendingSources_;
    std::vector<Vessel*> targets = pendingTargets_;
    pendingSources_.clear();
    pendingTargets_.clear();
    // a target that keeps a binding which is not a source (bound but not involved) is left alone
    targets.erase(std::remove_if(targets.begin(), targets.end(),
                                 [&](Vessel* v) {
                                     VesselPhysics* vp = Find(*v);
                                     return vp != nullptr && std::find(sources.begin(), sources.end(), vp) == sources.end();
                                 }),
                  targets.end());
    std::vector<std::unique_ptr<VesselPhysics>> made = VesselPhysics::Rebind(*world_, sources, targets, options);
    vessels_.erase(std::remove_if(vessels_.begin(), vessels_.end(),
                                  [&](const std::unique_ptr<VesselPhysics>& vp) {
                                      return std::find(sources.begin(), sources.end(), vp.get()) != sources.end();
                                  }),
                   vessels_.end());
    for (auto& m : made) Adopt(std::move(m));
    ++rebinds_;
    lastRebindMs_ = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

VesselPhysics* VesselPhysicsHost::BindingForPart(Part& p) {
    ProcessStructuralChanges();
    return p.vessel != nullptr ? Find(*p.vessel) : nullptr;
}

// ===================================================================================== Install

void VesselPhysicsHost::Install() {
    if (installed_) return;
    saved_ = Vessel::env;
    VesselEnvironment& env = Vessel::env;
    // Vessel.GoOffRails: Part.Unpack of every part (Part.cs:3784), then Part.ResumeVelocity (:3827)
    env.unpackPart = [this](Part& p) {
        ProcessStructuralChanges();
        if (p.vessel == nullptr || Find(*p.vessel) != nullptr) return;
        Vessel& v = *p.vessel;
        // Part.Start made the Rigidbodies when the parts entered the scene; a host that never ran it gets it here
        if (std::none_of(v.parts.begin(), v.parts.end(), [](const Part* q) { return q->hasRigidbody; })) StartPartRigidbodies(v);
        Create(v, unpackPose ? unpackPose(v) : DefaultUnpackPose(v));
    };
    env.resumePartVelocity = [this](Part& p) {
        if (VesselPhysics* vp = BindingForPart(p)) vp->ResumePartVelocity(p);
    };
    // Vessel.GoOnRails: Part.Pack (Part.cs:3747): the bodies go
    env.packPart = [this](Part& p) {
        ProcessStructuralChanges();
        if (p.vessel != nullptr) Destroy(*p.vessel);
    };
    env.readPhysicsState = [this](Vessel& v) -> std::optional<VesselPhysicsState> {
        ProcessStructuralChanges();
        if (VesselPhysics* vp = Find(v)) return vp->ReadState();
        return std::nullopt;
    };
    env.partRigidbody = [this](Part& p) -> PartRigidbodyState {
        if (VesselPhysics* vp = BindingForPart(p)) return vp->RigidbodyState(p);
        return PartRigidbodyState::FromPart(p);
    };
    env.rootRigidbody = [this](Vessel& v, const Vector3f& point, Vector3f& pointVelocity, Vector3f& angularVelocity) {
        ProcessStructuralChanges();
        if (VesselPhysics* vp = Find(v)) return vp->RootRigidbody(point, pointVelocity, angularVelocity);
        return false;
    };
    env.offsetVelocity = [this](Vessel& v, const Vector3d& dv) {
        ProcessStructuralChanges();
        if (VesselPhysics* vp = Find(v)) vp->ChangeWorldVelocity(dv);
    };
    // WP-20c: the module force hooks (Part.AddForce / AddForceAtPosition / AddTorque, Part.cs:8663-8688; part.rb.velocity)
    savedHooks_ = std::make_unique<SavedHooks>();
    savedHooks_->addForce = ModuleDecouplerBase::addForce;
    savedHooks_->addForceAtPosition = ModuleDecouplerBase::addForceAtPosition;
    savedHooks_->addTorque = DockingEnvironment::addTorque;
    savedHooks_->partVelocity = DockingEnvironment::partVelocity;
    ModuleDecouplerBase::addForce = [this](Part& p, const Vector3f& f) {
        if (VesselPhysics* vp = BindingForPart(p)) vp->AddForce(p, Vector3d(f));
    };
    ModuleDecouplerBase::addForceAtPosition = [this](Part& p, const Vector3f& f, const Vector3f& position) {
        if (VesselPhysics* vp = BindingForPart(p)) vp->AddForceAtPosition(p, Vector3d(f), Vector3d(position));
    };
    DockingEnvironment::addTorque = [this](Part& p, const Vector3f& t) {
        if (VesselPhysics* vp = BindingForPart(p)) vp->AddTorque(p, Vector3d(t));
    };
    DockingEnvironment::partVelocity = [this](Part& p) -> Vector3f {
        if (VesselPhysics* vp = BindingForPart(p)) return vp->RigidbodyState(p).velocity;
        return Vector3f();
    };
    // WP-20c: the structural game events
    GE::onPartDeCoupleNewVesselComplete.Add(this, &VesselPhysicsHost::OnDecoupleNewVessel);
    GE::onVesselsUndocking.Add(this, &VesselPhysicsHost::OnVesselsUndocking);
    GE::onPartCouple.Add(this, &VesselPhysicsHost::OnPartCouple);
    GE::onPartDie.Add(this, &VesselPhysicsHost::OnPartDie);
    GE::onVesselWasModified.Add(this, &VesselPhysicsHost::OnVesselWasModified);
    installed_ = true;
}

void VesselPhysicsHost::Uninstall() {
    if (!installed_) return;
    ProcessStructuralChanges();
    VesselEnvironment& env = Vessel::env;
    env.unpackPart = saved_.unpackPart;
    env.resumePartVelocity = saved_.resumePartVelocity;
    env.packPart = saved_.packPart;
    env.readPhysicsState = saved_.readPhysicsState;
    env.partRigidbody = saved_.partRigidbody;
    env.rootRigidbody = saved_.rootRigidbody;
    env.offsetVelocity = saved_.offsetVelocity;
    if (savedHooks_) {
        ModuleDecouplerBase::addForce = savedHooks_->addForce;
        ModuleDecouplerBase::addForceAtPosition = savedHooks_->addForceAtPosition;
        DockingEnvironment::addTorque = savedHooks_->addTorque;
        DockingEnvironment::partVelocity = savedHooks_->partVelocity;
        savedHooks_.reset();
    }
    GE::onPartDeCoupleNewVesselComplete.Remove(this, &VesselPhysicsHost::OnDecoupleNewVessel);
    GE::onVesselsUndocking.Remove(this, &VesselPhysicsHost::OnVesselsUndocking);
    GE::onPartCouple.Remove(this, &VesselPhysicsHost::OnPartCouple);
    GE::onPartDie.Remove(this, &VesselPhysicsHost::OnPartDie);
    GE::onVesselWasModified.Remove(this, &VesselPhysicsHost::OnVesselWasModified);
    installed_ = false;
}

}  // namespace ksp::physics
