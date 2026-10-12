// WP-20b: VesselPhysicsHost, the Vessel::env wiring of the reduced model (see ksp/physics/vessel_physics.hpp).
#include <algorithm>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/physics/reference_frame.hpp"
#include "ksp/physics/vessel_physics.hpp"

namespace ksp::physics {

VesselPhysicsHost::VesselPhysicsHost(PhysicsWorld& world, const VesselPhysicsOptions& opts) : options(opts), world_(&world) {}

VesselPhysicsHost::~VesselPhysicsHost() {
    Uninstall();
    DestroyAll();
}

void VesselPhysicsHost::ApplyFrame(VesselPhysics& vp) const {
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
        if (&vp->GetVessel() == &vessel) return vp.get();
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
    Destroy(physics->GetVessel());
    ApplyFrame(*physics);
    physics->SyncPartTransforms();
    vessels_.push_back(std::move(physics));
    return *vessels_.back();
}

void VesselPhysicsHost::Destroy(const Vessel& vessel) {
    vessels_.erase(std::remove_if(vessels_.begin(), vessels_.end(), [&](const std::unique_ptr<VesselPhysics>& vp) { return &vp->GetVessel() == &vessel; }),
                   vessels_.end());
}

void VesselPhysicsHost::DestroyAll() { vessels_.clear(); }

VesselPhysics* VesselPhysicsHost::OnVesselSplit(Vessel& original, Vessel& split) {
    VesselPhysics* vp = Find(original);
    if (vp == nullptr) return nullptr;
    std::unique_ptr<VesselPhysics> part = vp->SplitOff(split);
    ApplyFrame(*vp);
    vp->SyncPartTransforms();
    return &Adopt(std::move(part));
}

VesselPhysics* VesselPhysicsHost::OnVesselMerge(Vessel& a, Vessel& b, Vessel& merged) {
    VesselPhysics* pa = Find(a);
    VesselPhysics* pb = Find(b);
    if (pa == nullptr || pb == nullptr) return nullptr;
    std::unique_ptr<VesselPhysics> m = VesselPhysics::Merge(*pa, *pb, merged);
    Destroy(a);
    Destroy(b);
    return &Adopt(std::move(m));
}

void VesselPhysicsHost::RefreshFrames() {
    for (auto& vp : vessels_) ApplyFrame(*vp);
}

void VesselPhysicsHost::IntegrateAll() {
    for (auto& vp : vessels_) vp->IntegratePartBuffers(vp->GetVessel().precalc.integrationAccel);
}

void VesselPhysicsHost::FlushAll(float dt) {
    for (auto& vp : vessels_) vp->Flush(dt);
}

void VesselPhysicsHost::AfterStepAll() {
    for (auto& vp : vessels_) vp->AfterStep();
}

void VesselPhysicsHost::Step(float dt) {
    FlushAll(dt);
    world_->Step(dt);
    AfterStepAll();
}

void VesselPhysicsHost::Install() {
    if (installed_) return;
    saved_ = Vessel::env;
    VesselEnvironment& env = Vessel::env;
    // Vessel.GoOffRails: Part.Unpack of every part (Part.cs:3784), then Part.ResumeVelocity (:3827)
    env.unpackPart = [this](Part& p) {
        if (p.vessel == nullptr || Find(*p.vessel) != nullptr) return;
        Vessel& v = *p.vessel;
        // Part.Start made the Rigidbodies when the parts entered the scene; a host that never ran it gets it here
        if (std::none_of(v.parts.begin(), v.parts.end(), [](const Part* q) { return q->hasRigidbody; })) StartPartRigidbodies(v);
        Create(v, unpackPose ? unpackPose(v) : DefaultUnpackPose(v));
    };
    env.resumePartVelocity = [this](Part& p) {
        if (p.vessel == nullptr) return;
        if (VesselPhysics* vp = Find(*p.vessel)) vp->ResumePartVelocity(p);
    };
    // Vessel.GoOnRails: Part.Pack (Part.cs:3747): the bodies go
    env.packPart = [this](Part& p) {
        if (p.vessel != nullptr) Destroy(*p.vessel);
    };
    env.readPhysicsState = [this](Vessel& v) -> std::optional<VesselPhysicsState> {
        if (VesselPhysics* vp = Find(v)) return vp->ReadState();
        return std::nullopt;
    };
    env.partRigidbody = [this](Part& p) -> PartRigidbodyState {
        if (p.vessel != nullptr)
            if (VesselPhysics* vp = Find(*p.vessel)) return vp->RigidbodyState(p);
        return PartRigidbodyState::FromPart(p);
    };
    env.rootRigidbody = [this](Vessel& v, const Vector3f& point, Vector3f& pointVelocity, Vector3f& angularVelocity) {
        if (VesselPhysics* vp = Find(v)) return vp->RootRigidbody(point, pointVelocity, angularVelocity);
        return false;
    };
    env.offsetVelocity = [this](Vessel& v, const Vector3d& dv) {
        if (VesselPhysics* vp = Find(v)) vp->ChangeWorldVelocity(dv);
    };
    installed_ = true;
}

void VesselPhysicsHost::Uninstall() {
    if (!installed_) return;
    VesselEnvironment& env = Vessel::env;
    env.unpackPart = saved_.unpackPart;
    env.resumePartVelocity = saved_.resumePartVelocity;
    env.packPart = saved_.packPart;
    env.readPhysicsState = saved_.readPhysicsState;
    env.partRigidbody = saved_.partRigidbody;
    env.rootRigidbody = saved_.rootRigidbody;
    env.offsetVelocity = saved_.offsetVelocity;
    installed_ = false;
}

}  // namespace ksp::physics
