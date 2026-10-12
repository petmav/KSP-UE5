// WP-20a: PhysicsWorld over Jolt Physics v5.6.0 (third_party/jolt, JPH_DOUBLE_PRECISION). See ksp/physics/physics_world.hpp.
//
// This file is compiled with Jolt's usage requirements (include path, /arch:AVX2 or -mavx2, the JPH_* definitions) and is the
// only kspcore translation unit that sees a Jolt header. It is glue, not ported KSP code: no Mono float rule applies, and the
// std:: maths ban for ported code does not cover Jolt internals. Compiled WITHOUT RTTI like Jolt itself (-fno-rtti, /GR-): a
// polymorphic class derived from a Jolt class would otherwise reference a typeinfo Jolt does not provide (GCC / Clang link error), so
// nothing here may use typeid / dynamic_cast, and the PlayerLoop hook (Delegate uses typeid) lives in physics_world_loop.cpp. The layer set and the group filter are the spike's
// (core/spikes/jolt/src/jolt_world.cpp), generalised to the API of the header.
#include "ksp/physics/physics_world.hpp"

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/GroupFilter.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <unordered_set>

#include "ksp/core/log.hpp"
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
    std::map<uint64_t, BodyId> statics;  // WP-12b: static colliders by caller key

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
    if (m.Bi().GetMotionType(b) == JPH::EMotionType::Static) {  // a static collider removed by body id leaves no stale key behind
        auto it = m.statics.find(m.Bi().GetUserData(b));
        if (it != m.statics.end() && it->second == id) m.statics.erase(it);
    }
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

// ---- static geometry (WP-12b) --------------------------------------------------------------------------------------------------

namespace {

// Query layer mask (PhysicsWorld::kLayer*) -> Jolt object layers
class MaskLayerFilter final : public JPH::ObjectLayerFilter {
public:
    explicit MaskLayerFilter(uint32_t mask) : mask_(mask) {}
    bool ShouldCollide(JPH::ObjectLayer l) const override {
        return ((l == Layers::STATIC ? PhysicsWorld::kLayerStatic : PhysicsWorld::kLayerMoving) & mask_) != 0;
    }

private:
    uint32_t mask_;
};

}  // namespace

BodyId PhysicsWorld::AddStaticMesh(uint64_t key, const Vector3d& centre, const std::vector<Vector3f>& vertices,
                                   const std::vector<int>& triangles, float friction, float restitution) {
    Impl& m = *impl_;
    RemoveStatic(key);
    if (triangles.size() < 3) return kInvalidBody;
    JPH::VertexList vl;
    vl.reserve(vertices.size());
    for (const Vector3f& v : vertices) vl.push_back(JPH::Float3(v.x, v.y, v.z));
    JPH::IndexedTriangleList tl;
    tl.reserve(triangles.size() / 3);
    for (size_t i = 0; i + 2 < triangles.size(); i += 3) {
        const int a = triangles[i], b = triangles[i + 1], c = triangles[i + 2];
        if (a < 0 || b < 0 || c < 0 || static_cast<size_t>(a) >= vertices.size() || static_cast<size_t>(b) >= vertices.size() ||
            static_cast<size_t>(c) >= vertices.size())
            return kInvalidBody;
        tl.push_back(JPH::IndexedTriangle(static_cast<JPH::uint32>(a), static_cast<JPH::uint32>(b), static_cast<JPH::uint32>(c)));
    }
    JPH::MeshShapeSettings ms(std::move(vl), std::move(tl));
    JPH::ShapeSettings::ShapeResult res = ms.Create();
    if (res.HasError()) return kInvalidBody;
    if (m.order.size() >= m.cfg.maxBodies) return kInvalidBody;
    JPH::BodyCreationSettings bcs(res.Get(), ToRVec3(centre), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::STATIC);
    bcs.mFriction = friction;
    bcs.mRestitution = restitution;
    bcs.mUserData = key;
    JPH::BodyID id = m.Bi().CreateAndAddBody(bcs, JPH::EActivation::DontActivate);
    if (id.IsInvalid()) return kInvalidBody;
    const BodyId raw = id.GetIndexAndSequenceNumber();
    m.order.push_back(raw);
    m.live.insert(raw);
    m.statics[key] = raw;
    return raw;
}

BodyId PhysicsWorld::AddStaticShape(uint64_t key, const ShapeDesc& shape, const Vector3d& position, const QuaternionD& rotation,
                                    float friction, float restitution) {
    Impl& m = *impl_;
    RemoveStatic(key);
    if (m.order.size() >= m.cfg.maxBodies) return kInvalidBody;
    JPH::BodyCreationSettings bcs(MakeShape(shape), ToRVec3(position),
                                  JPH::Quat(static_cast<float>(rotation.x), static_cast<float>(rotation.y), static_cast<float>(rotation.z),
                                            static_cast<float>(rotation.w)),
                                  JPH::EMotionType::Static, Layers::STATIC);
    bcs.mFriction = friction;
    bcs.mRestitution = restitution;
    bcs.mUserData = key;
    JPH::BodyID id = m.Bi().CreateAndAddBody(bcs, JPH::EActivation::DontActivate);
    if (id.IsInvalid()) return kInvalidBody;
    const BodyId raw = id.GetIndexAndSequenceNumber();
    m.order.push_back(raw);
    m.live.insert(raw);
    m.statics[key] = raw;
    return raw;
}

bool PhysicsWorld::RemoveStatic(uint64_t key) {
    Impl& m = *impl_;
    auto it = m.statics.find(key);
    if (it == m.statics.end()) return false;
    const BodyId id = it->second;
    m.statics.erase(it);
    RemoveBody(id);
    return true;
}

BodyId PhysicsWorld::StaticBody(uint64_t key) const {
    auto it = impl_->statics.find(key);
    return it == impl_->statics.end() ? kInvalidBody : it->second;
}

size_t PhysicsWorld::StaticCount() const { return impl_->statics.size(); }

std::vector<uint64_t> PhysicsWorld::StaticKeys() const {
    std::vector<uint64_t> k;
    k.reserve(impl_->statics.size());
    for (const auto& kv : impl_->statics) k.push_back(kv.first);
    return k;
}

void PhysicsWorld::OptimizeBroadPhase() { impl_->system.OptimizeBroadPhase(); }

PhysicsWorld::RayHit PhysicsWorld::RayCast(const Vector3d& origin, const Vector3d& direction, double maxDistance, uint32_t layers) const {
    RayHit out;
    const double len = direction.magnitude();
    if (!(len > 0.0) || !(maxDistance > 0.0)) return out;
    const Vector3d dir = direction * (1.0 / len);
    const JPH::RRayCast ray(ToRVec3(origin), JPH::Vec3(static_cast<float>(dir.x * maxDistance), static_cast<float>(dir.y * maxDistance),
                                                       static_cast<float>(dir.z * maxDistance)));
    JPH::RayCastSettings settings;
    settings.SetBackFaceMode(JPH::EBackFaceMode::CollideWithBackFaces);
    JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> collector;
    const MaskLayerFilter olf(layers);
    impl_->system.GetNarrowPhaseQueryNoLock().CastRay(ray, settings, collector, {}, olf);
    if (!collector.HadHit()) return out;
    const JPH::RayCastResult& h = collector.mHit;
    out.hit = true;
    out.body = h.mBodyID.GetIndexAndSequenceNumber();
    out.distance = static_cast<double>(h.mFraction) * maxDistance;
    out.point = origin + dir * out.distance;
    JPH::BodyLockRead lock(impl_->system.GetBodyLockInterfaceNoLock(), h.mBodyID);
    if (lock.Succeeded()) {
        const JPH::Body& b = lock.GetBody();
        out.userData = b.GetUserData();
        JPH::Vec3 n = b.GetWorldSpaceSurfaceNormal(h.mSubShapeID2, ray.GetPointOnRay(h.mFraction));
        // face the ray origin (a back-face hit reports the triangle normal of its front side)
        if (n.Dot(JPH::Vec3(static_cast<float>(dir.x), static_cast<float>(dir.y), static_cast<float>(dir.z))) > 0.0f) n = -n;
        out.normal = Vector3d(n.GetX(), n.GetY(), n.GetZ());
    }
    return out;
}

size_t PhysicsWorld::OverlapSphere(const Vector3d& centre, double radius, std::vector<BodyId>& out, uint32_t layers) const {
    out.clear();
    JPH::Ref<JPH::SphereShape> sphere = new JPH::SphereShape(static_cast<float>(radius));
    JPH::CollideShapeSettings settings;
    settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;
    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
    const MaskLayerFilter olf(layers);
    // the transform is the shape's WORLD transform; the base offset (also `centre`, for precision) only moves the origin of the
    // returned contact data
    impl_->system.GetNarrowPhaseQueryNoLock().CollideShape(sphere, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(ToRVec3(centre)),
                                                           settings, ToRVec3(centre), collector, {}, olf);
    for (const JPH::CollideShapeResult& r : collector.mHits) out.push_back(r.mBodyID2.GetIndexAndSequenceNumber());
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out.size();
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
