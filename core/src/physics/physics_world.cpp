// WP-20a: PhysicsWorld over Jolt Physics v5.6.0 (third_party/jolt, JPH_DOUBLE_PRECISION). See ksp/physics/physics_world.hpp.
//
// This file is compiled with Jolt's usage requirements (include path, /arch:AVX2 or -mavx2, the JPH_* definitions) and is the
// only kspcore translation unit that sees a Jolt header. It is glue, not ported KSP code: no Mono float rule applies, and the
// std:: maths ban for ported code does not cover Jolt internals. The layer set and the group filter are the spike's
// (core/spikes/jolt/src/jolt_world.cpp), generalised to the API of the header.
#include "ksp/physics/physics_world.hpp"

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/GroupFilter.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <unordered_set>

#include "ksp/core/log.hpp"
#include "ksp/core/timing_manager.hpp"
#include "ksp/core/unity_time.hpp"

namespace ksp::physics {

namespace {

namespace Layers {
constexpr JPH::ObjectLayer STATIC = 0;
constexpr JPH::ObjectLayer MOVING = 1;
}  // namespace Layers

namespace BPL {
constexpr JPH::BroadPhaseLayer STATIC(0);
constexpr JPH::BroadPhaseLayer MOVING(1);
constexpr unsigned NUM = 2;
}  // namespace BPL

class BPLayers final : public JPH::BroadPhaseLayerInterface {
public:
    unsigned GetNumBroadPhaseLayers() const override { return BPL::NUM; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer l) const override {
        return l == Layers::STATIC ? BPL::STATIC : BPL::MOVING;
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer l) const override { return l == BPL::STATIC ? "STATIC" : "MOVING"; }
#endif
};

class ObjVsBP final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer l, JPH::BroadPhaseLayer b) const override { return l == Layers::STATIC ? b == BPL::MOVING : true; }
};

class ObjPair final : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override { return a == Layers::STATIC ? b == Layers::MOVING : true; }
};

// Bodies of the same vessel (same CollisionGroup group id) never collide, unless both carry sub-group flag 1
// (Part.sameVesselCollision on both; Assembly-CSharp/CollisionManager.cs:126-168).
class SameVesselGroupFilter final : public JPH::GroupFilter {
public:
    bool CanCollide(const JPH::CollisionGroup& a, const JPH::CollisionGroup& b) const override {
        if (a.GetGroupID() != b.GetGroupID()) return true;
        return a.GetSubGroupID() == 1 && b.GetSubGroupID() == 1;
    }
};

void TraceImpl(const char* fmt, ...) {
    char buf[512];
    va_list list;
    va_start(list, fmt);
    std::vsnprintf(buf, sizeof buf, fmt, list);
    va_end(list);
    log::Warning(std::string("Jolt: ") + buf);
}

// Process-wide Jolt registration (allocator, factory, types), reference counted over the live worlds.
std::mutex g_joltMutex;
int g_joltRefs = 0;

void JoltAcquire() {
    std::lock_guard<std::mutex> lock(g_joltMutex);
    if (g_joltRefs++ == 0) {
        JPH::RegisterDefaultAllocator();
        JPH::Trace = TraceImpl;
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
    }
}

void JoltRelease() {
    std::lock_guard<std::mutex> lock(g_joltMutex);
    if (--g_joltRefs == 0) {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }
}

JPH::Vec3 ToVec3(const Vector3d& v) { return JPH::Vec3(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)); }
JPH::RVec3 ToRVec3(const Vector3d& v) {
    return JPH::RVec3(static_cast<JPH::Real>(v.x), static_cast<JPH::Real>(v.y), static_cast<JPH::Real>(v.z));
}
Vector3d FromVec3(JPH::Vec3 v) { return Vector3d(v.GetX(), v.GetY(), v.GetZ()); }
Vector3d FromRVec3(JPH::RVec3 v) { return Vector3d(static_cast<double>(v.GetX()), static_cast<double>(v.GetY()), static_cast<double>(v.GetZ())); }

JPH::Ref<JPH::Shape> MakeShape(const ShapeDesc& s) {
    switch (s.kind) {
        case ShapeDesc::Kind::Sphere:
            return new JPH::SphereShape(s.radius);
        case ShapeDesc::Kind::Capsule:
            return new JPH::CapsuleShape(s.halfHeight, s.radius);
        case ShapeDesc::Kind::Box:
        default: {
            JPH::Vec3 h(s.halfExtents.x, s.halfExtents.y, s.halfExtents.z);
            float convexRadius = std::min(JPH::cDefaultConvexRadius, std::min(h.GetX(), std::min(h.GetY(), h.GetZ())));
            return new JPH::BoxShape(h, convexRadius);
        }
    }
}

uint64_t Fnv(uint64_t h, const void* data, size_t n) {
    const unsigned char* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}

}  // namespace

struct PhysicsWorld::Impl {
    PhysicsWorldConfig cfg;
    std::unique_ptr<JPH::TempAllocatorImpl> temp;
    std::unique_ptr<JPH::JobSystemThreadPool> jobs;
    BPLayers bpl;
    ObjVsBP ovb;
    ObjPair olp;
    JPH::PhysicsSystem system;
    JPH::Ref<SameVesselGroupFilter> filter;
    std::vector<BodyId> order;           // live bodies in creation order
    std::unordered_set<BodyId> live;
    uint64_t steps = 0;

    JPH::BodyInterface& Bi() { return system.GetBodyInterfaceNoLock(); }
    const JPH::BodyInterface& Bi() const { return system.GetBodyInterfaceNoLock(); }
};

PhysicsWorld::PhysicsWorld(const PhysicsWorldConfig& cfg) {
    JoltAcquire();
    impl_ = std::make_unique<Impl>();
    Impl& m = *impl_;
    m.cfg = cfg;
    m.temp = std::make_unique<JPH::TempAllocatorImpl>(cfg.tempAllocatorBytes);
    m.jobs = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
                                                        cfg.threads > 1 ? cfg.threads - 1 : 0);
    m.system.Init(cfg.maxBodies, 0, cfg.maxBodyPairs, cfg.maxContacts, m.bpl, m.ovb, m.olp);
    // KSP: Physics.gravity = 0; gravity is a per-vessel force (VesselPrecalculate.cs:281-368).
    m.system.SetGravity(JPH::Vec3::sZero());
    JPH::PhysicsSettings s = m.system.GetPhysicsSettings();
    s.mNumVelocitySteps = static_cast<unsigned>(cfg.velocitySteps);
    s.mNumPositionSteps = static_cast<unsigned>(cfg.positionSteps);
    m.system.SetPhysicsSettings(s);
    m.filter = new SameVesselGroupFilter();
}

PhysicsWorld::~PhysicsWorld() {
    if (impl_) {
        // Bodies must be removed before the system dies (Jolt asserts on leaked bodies).
        JPH::BodyInterface& bi = impl_->Bi();
        for (BodyId id : impl_->order) {
            JPH::BodyID b(id);
            bi.RemoveBody(b);
            bi.DestroyBody(b);
        }
        impl_.reset();
    }
    JoltRelease();
}

const PhysicsWorldConfig& PhysicsWorld::Config() const { return impl_->cfg; }

void PhysicsWorld::Step(float dt, int collisionSteps) {
    impl_->system.Update(dt, collisionSteps, impl_->temp.get(), impl_->jobs.get());
    ++impl_->steps;
}

void PhysicsWorld::FixedStep() { Step(Time::fixedDeltaTime); }

uint64_t PhysicsWorld::StepCount() const { return impl_->steps; }

void PhysicsWorld::Attach(PlayerLoop& loop) {
    if (!(attached_ == nullptr)) return;
    attached_ = Delegate<void()>::Lambda(this, [this] { FixedStep(); });
    loop.physicsStep += attached_;
}

void PhysicsWorld::Detach(PlayerLoop& loop) {
    if (attached_ == nullptr) return;
    loop.physicsStep -= attached_;
    attached_ = Delegate<void()>();
}

BodyId PhysicsWorld::CreateBody(const BodyDesc& d) {
    Impl& m = *impl_;
    if (m.order.size() >= m.cfg.maxBodies) return kInvalidBody;
    const bool isStatic = d.motion == MotionType::Static;
    const JPH::EMotionType mt = isStatic ? JPH::EMotionType::Static
                                         : (d.motion == MotionType::Kinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic);
    JPH::BodyCreationSettings bcs(MakeShape(d.shape), ToRVec3(d.position),
                                  JPH::Quat(static_cast<float>(d.rotation.x), static_cast<float>(d.rotation.y),
                                            static_cast<float>(d.rotation.z), static_cast<float>(d.rotation.w)),
                                  mt, isStatic ? Layers::STATIC : Layers::MOVING);
    if (mt == JPH::EMotionType::Dynamic) {
        bcs.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        bcs.mMassPropertiesOverride.mMass = d.mass;
    }
    bcs.mLinearVelocity = ToVec3(d.linearVelocity);
    bcs.mAngularVelocity = ToVec3(d.angularVelocity);
    bcs.mFriction = d.friction;
    bcs.mRestitution = d.restitution;
    bcs.mLinearDamping = d.linearDamping;
    bcs.mAngularDamping = d.angularDamping;
    bcs.mMaxLinearVelocity = d.maxLinearVelocity;
    bcs.mMaxAngularVelocity = d.maxAngularVelocity;
    bcs.mAllowSleeping = d.allowSleeping;
    bcs.mUserData = d.userData;
    if (d.vesselGroup != 0) {
        bcs.mCollisionGroup.SetGroupFilter(m.filter);
        bcs.mCollisionGroup.SetGroupID(d.vesselGroup);
        bcs.mCollisionGroup.SetSubGroupID(d.sameVesselCollision ? 1u : 0u);
    }
    JPH::BodyID id = m.Bi().CreateAndAddBody(bcs, isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
    if (id.IsInvalid()) return kInvalidBody;
    BodyId raw = id.GetIndexAndSequenceNumber();
    m.order.push_back(raw);
    m.live.insert(raw);
    return raw;
}

void PhysicsWorld::RemoveBody(BodyId id) {
    Impl& m = *impl_;
    if (m.live.erase(id) == 0) return;
    m.order.erase(std::find(m.order.begin(), m.order.end(), id));
    JPH::BodyID b(id);
    m.Bi().RemoveBody(b);
    m.Bi().DestroyBody(b);
}

size_t PhysicsWorld::BodyCount() const { return impl_->order.size(); }
const std::vector<BodyId>& PhysicsWorld::Bodies() const { return impl_->order; }
bool PhysicsWorld::IsValid(BodyId id) const { return impl_->live.count(id) != 0; }
uint64_t PhysicsWorld::UserData(BodyId id) const { return impl_->Bi().GetUserData(JPH::BodyID(id)); }

Vector3d PhysicsWorld::GetPosition(BodyId id) const { return FromRVec3(impl_->Bi().GetPosition(JPH::BodyID(id))); }
QuaternionD PhysicsWorld::GetRotation(BodyId id) const {
    JPH::Quat q = impl_->Bi().GetRotation(JPH::BodyID(id));
    return QuaternionD(q.GetX(), q.GetY(), q.GetZ(), q.GetW());
}
Vector3d PhysicsWorld::GetLinearVelocity(BodyId id) const { return FromVec3(impl_->Bi().GetLinearVelocity(JPH::BodyID(id))); }
Vector3d PhysicsWorld::GetAngularVelocity(BodyId id) const { return FromVec3(impl_->Bi().GetAngularVelocity(JPH::BodyID(id))); }
void PhysicsWorld::SetPosition(BodyId id, const Vector3d& p) {
    impl_->Bi().SetPosition(JPH::BodyID(id), ToRVec3(p), JPH::EActivation::DontActivate);
}
void PhysicsWorld::SetLinearVelocity(BodyId id, const Vector3d& v) { impl_->Bi().SetLinearVelocity(JPH::BodyID(id), ToVec3(v)); }
void PhysicsWorld::SetAngularVelocity(BodyId id, const Vector3d& w) { impl_->Bi().SetAngularVelocity(JPH::BodyID(id), ToVec3(w)); }

float PhysicsWorld::GetMass(BodyId id) const {
    JPH::BodyLockRead lock(impl_->system.GetBodyLockInterfaceNoLock(), JPH::BodyID(id));
    if (!lock.Succeeded() || !lock.GetBody().IsDynamic()) return 0.0f;
    return 1.0f / lock.GetBody().GetMotionProperties()->GetInverseMass();
}

void PhysicsWorld::AddForce(BodyId id, const Vector3d& f) { impl_->Bi().AddForce(JPH::BodyID(id), ToVec3(f), JPH::EActivation::DontActivate); }
void PhysicsWorld::AddForceAtPosition(BodyId id, const Vector3d& f, const Vector3d& p) {
    impl_->Bi().AddForce(JPH::BodyID(id), ToVec3(f), ToRVec3(p), JPH::EActivation::DontActivate);
}
void PhysicsWorld::AddTorque(BodyId id, const Vector3d& t) { impl_->Bi().AddTorque(JPH::BodyID(id), ToVec3(t), JPH::EActivation::DontActivate); }
void PhysicsWorld::AddImpulse(BodyId id, const Vector3d& i) { impl_->Bi().AddImpulse(JPH::BodyID(id), ToVec3(i)); }

size_t PhysicsWorld::AddVelocityChangeToAllMoving(const Vector3d& dv) {
    Impl& m = *impl_;
    size_t n = 0;
    const JPH::Vec3 d = ToVec3(dv);
    for (BodyId id : m.order) {
        JPH::BodyID b(id);
        if (m.Bi().GetMotionType(b) == JPH::EMotionType::Static) continue;
        m.Bi().SetLinearVelocity(b, m.Bi().GetLinearVelocity(b) + d);
        ++n;
    }
    return n;
}

uint64_t PhysicsWorld::StateHash() const {
    uint64_t h = 1469598103934665603ull;
    const JPH::BodyInterface& bi = impl_->Bi();
    for (BodyId id : impl_->order) {
        JPH::BodyID b(id);
        JPH::RVec3 p = bi.GetPosition(b);
        JPH::Quat q = bi.GetRotation(b);
        JPH::Vec3 v = bi.GetLinearVelocity(b);
        JPH::Vec3 w = bi.GetAngularVelocity(b);
        const double pd[3] = {static_cast<double>(p.GetX()), static_cast<double>(p.GetY()), static_cast<double>(p.GetZ())};
        const float qf[4] = {q.GetX(), q.GetY(), q.GetZ(), q.GetW()};
        const float vf[6] = {v.GetX(), v.GetY(), v.GetZ(), w.GetX(), w.GetY(), w.GetZ()};
        h = Fnv(h, pd, sizeof pd);
        h = Fnv(h, qf, sizeof qf);
        h = Fnv(h, vf, sizeof vf);
    }
    return h;
}

}  // namespace ksp::physics
