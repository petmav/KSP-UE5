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
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <tuple>
#include <unordered_map>
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

// WP-19b: `staticOn` is the static-collision gate (PhysicsWorld::SetStaticCollisions); it is only changed between steps.
class ObjVsBP final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool staticOn = true;
    bool ShouldCollide(JPH::ObjectLayer l, JPH::BroadPhaseLayer b) const override {
        if (l == Layers::STATIC) return staticOn && b == BPL::MOVING;
        return b == BPL::MOVING || staticOn;
    }
};

class ObjPair final : public JPH::ObjectLayerPairFilter {
public:
    bool staticOn = true;
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override {
        if (a == Layers::STATIC) return staticOn && b == Layers::MOVING;
        return b == Layers::MOVING || staticOn;
    }
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

Vector3d FromVec3(JPH::Vec3 v);
Vector3d FromRVec3(JPH::RVec3 v);

// WP-20c: records which sub-shapes (parts) of dynamic bodies touch static geometry (the spike's SupportListener,
// core/spikes/jolt/src/reduced.cpp, made exact): Jolt merges the contact manifolds of different sub-shapes of a body pair (manifold reduction)
// and reports only the first sub-shape in OnContactAdded / Persisted, so the touching sub-shapes are taken from OnContactValidate, which
// sees every sub-shape hit before the merge (returning AcceptContact keeps it coming and accepts everything). A hit counts when the shapes
// touch within 1 mm (speculative contacts further apart do not support). Jolt reuses a body pair's manifolds without a new collision test
// while the pair barely moves (its body-pair cache): such a pair, active without validation this step, keeps its last set. Jolt calls in
// from its worker threads: a mutex guards the records; the result is sorted after the step.
//
// WP-20e: the same listener also records the contact EVENTS per collider pair (PhysicsWorld::ContactEvents) when event tracking is on: a
// sub-shape pair touches when a validated contact is closer than the contact offset, or (body-pair cache, no validation this step) when the
// body pair still has a manifold and the sub-shape pair touched in the last step. Enter / Stay / Exit are the set differences between steps.
// Both record sets are independent: StaticContacts() is exactly WP-20c's whatever the event tracking does.
class StaticContactListener final : public JPH::ContactListener {
public:
    static constexpr float kTouch = 1.0e-3f;  // m: penetration depth above -kTouch (touching or penetrating)
    std::mutex mutex;
    bool staticOn = false;  // WP-20c's StaticContacts
    std::unordered_map<uint64_t, std::vector<uint32_t>> validated;   // this step: (dynamic, static) pair -> touching sub-shape user data
    std::unordered_map<uint64_t, uint32_t> active;                   // this step: pairs with a contact manifold -> the manifold's sub-shape
    std::unordered_map<uint64_t, std::vector<uint32_t>> lastParts;   // per pair: the last validated set (for cached manifolds)
    // ---- WP-20e events
    bool eventsOn = false;
    float contactOffset = 0.01f;
    using SubKey = std::pair<uint32_t, uint64_t>;                     // (sub1, other)
    struct PairRec {
        BodyId body1 = kInvalidBody, body2 = kInvalidBody;
        bool otherStatic = false;
        std::map<SubKey, PhysicsWorld::ContactEvent> subs;            // touching sub-shape pairs with their data (kind unused)
    };
    std::unordered_map<uint64_t, PairRec> evValidated;               // this step: body pair -> validated touching sub-shape pairs
    std::unordered_map<uint64_t, std::pair<BodyId, BodyId>> evActive;  // this step: body pairs with a manifold
    std::unordered_map<uint64_t, PairRec> evLast;                    // the last step's touching sets

    JPH::ValidateResult OnContactValidate(const JPH::Body& b1, const JPH::Body& b2, JPH::RVec3Arg base, const JPH::CollideShapeResult& r) override {
        if (staticOn && b1.IsStatic() != b2.IsStatic() && r.mPenetrationDepth > -kTouch) {
            const JPH::Body& dyn = b1.IsStatic() ? b2 : b1;
            const JPH::Body& stat = b1.IsStatic() ? b1 : b2;
            const JPH::SubShapeID& id = b1.IsStatic() ? r.mSubShapeID2 : r.mSubShapeID1;
            const uint32_t ud = static_cast<uint32_t>(dyn.GetShape()->GetSubShapeUserData(id));
            std::lock_guard<std::mutex> lock(mutex);
            validated[Key(dyn, stat)].push_back(ud);
        }
        if (eventsOn && !(b1.IsStatic() && b2.IsStatic()) && r.mPenetrationDepth > -contactOffset) RecordEvent(b1, b2, base, r);
        return JPH::ValidateResult::AcceptContact;
    }
    void OnContactAdded(const JPH::Body& b1, const JPH::Body& b2, const JPH::ContactManifold& m, JPH::ContactSettings&) override { Handle(b1, b2, m); }
    void OnContactPersisted(const JPH::Body& b1, const JPH::Body& b2, const JPH::ContactManifold& m, JPH::ContactSettings&) override {
        Handle(b1, b2, m);
    }
    void BeginStep() {
        validated.clear();
        active.clear();
        evValidated.clear();
        evActive.clear();
    }
    // after the step: the union of the active pairs' touching parts, sorted (body id, user data), no duplicates
    void EndStep(std::vector<PhysicsWorld::StaticContact>& out) {
        out.clear();
        if (!staticOn) {
            lastParts.clear();
            return;
        }
        std::unordered_map<uint64_t, std::vector<uint32_t>> keep;
        for (const auto& [key, manifoldPart] : active) {
            std::vector<uint32_t> parts;
            const auto v = validated.find(key);
            if (v != validated.end()) parts = v->second;
            else if (const auto l = lastParts.find(key); l != lastParts.end()) parts = l->second;
            else parts.push_back(manifoldPart);
            for (uint32_t ud : parts) out.push_back(PhysicsWorld::StaticContact{static_cast<BodyId>(key >> 32), ud});
            keep[key] = std::move(parts);
        }
        lastParts.swap(keep);  // pairs no longer in contact are forgotten
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
    }
    // WP-20e: this step's touching sets (validated, or the last set of a cached active pair) against the last step's -> events
    void EndStepEvents(std::vector<PhysicsWorld::ContactEvent>& out) {
        using Kind = PhysicsWorld::ContactEvent::Kind;
        out.clear();
        if (!eventsOn) {
            evLast.clear();
            return;
        }
        std::unordered_map<uint64_t, PairRec> now;
        for (const auto& [key, bodies] : evActive) {
            const auto v = evValidated.find(key);
            if (v != evValidated.end()) now[key] = v->second;
            else if (const auto l = evLast.find(key); l != evLast.end()) now[key] = l->second;
        }
        for (const auto& [key, rec] : now) {
            const auto l = evLast.find(key);
            for (const auto& [sk, ev] : rec.subs) {
                PhysicsWorld::ContactEvent e = ev;
                e.kind = (l != evLast.end() && l->second.subs.count(sk) != 0) ? Kind::Stay : Kind::Enter;
                out.push_back(e);
            }
        }
        for (const auto& [key, rec] : evLast) {
            const auto n = now.find(key);
            for (const auto& [sk, ev] : rec.subs)
                if (n == now.end() || n->second.subs.count(sk) == 0) {
                    PhysicsWorld::ContactEvent e = ev;
                    e.kind = Kind::Exit;
                    out.push_back(e);
                }
        }
        evLast.swap(now);
        std::sort(out.begin(), out.end(), [](const PhysicsWorld::ContactEvent& a, const PhysicsWorld::ContactEvent& b) {
            if (a.body1 != b.body1) return a.body1 < b.body1;
            if (a.sub1 != b.sub1) return a.sub1 < b.sub1;
            if (a.body2 != b.body2) return a.body2 < b.body2;
            if (a.other != b.other) return a.other < b.other;
            return a.kind < b.kind;
        });
    }
    // a removed body leaves no touching record (its pairs exit on the next step's comparison anyway; dropping them here would hide the
    // Exit, so nothing is done: Jolt reports no manifold for a removed body and the Exit follows)

private:
    static uint64_t Key(const JPH::Body& dyn, const JPH::Body& stat) {
        return (static_cast<uint64_t>(dyn.GetID().GetIndexAndSequenceNumber()) << 32) | stat.GetID().GetIndexAndSequenceNumber();
    }
    // (body1, body2) of an event: the non-static body first, of two non-static bodies the lower id
    static bool Flip(const JPH::Body& b1, const JPH::Body& b2) {
        return b1.IsStatic() || (!b2.IsStatic() && b2.GetID().GetIndexAndSequenceNumber() < b1.GetID().GetIndexAndSequenceNumber());
    }
    static uint64_t PairKey(const JPH::Body& a, const JPH::Body& b) {
        return (static_cast<uint64_t>(a.GetID().GetIndexAndSequenceNumber()) << 32) | b.GetID().GetIndexAndSequenceNumber();
    }
    static void Kinematics(const JPH::Body& b, Vector3d& v, Vector3d& w, Vector3d& com) {
        if (b.IsStatic()) {
            v = w = com = Vector3d();
            return;
        }
        v = FromVec3(b.GetLinearVelocity());
        w = FromVec3(b.GetAngularVelocity());
        com = FromRVec3(b.GetCenterOfMassPosition());
    }
    void RecordEvent(const JPH::Body& b1, const JPH::Body& b2, JPH::RVec3Arg base, const JPH::CollideShapeResult& r) {
        const bool flip = Flip(b1, b2);
        const JPH::Body& A = flip ? b2 : b1;
        const JPH::Body& B = flip ? b1 : b2;
        PhysicsWorld::ContactEvent e;
        e.body1 = A.GetID().GetIndexAndSequenceNumber();
        e.body2 = B.GetID().GetIndexAndSequenceNumber();
        e.sub1 = static_cast<uint32_t>(A.GetShape()->GetSubShapeUserData(flip ? r.mSubShapeID2 : r.mSubShapeID1));
        e.otherStatic = B.IsStatic();
        e.other = e.otherStatic ? B.GetUserData() : B.GetShape()->GetSubShapeUserData(flip ? r.mSubShapeID1 : r.mSubShapeID2);
        e.point = FromRVec3(base + (flip ? r.mContactPointOn2 : r.mContactPointOn1));
        const JPH::Vec3 axis = r.mPenetrationAxis.NormalizedOr(JPH::Vec3::sZero());
        e.normal = FromVec3(flip ? axis : -axis);  // Jolt's axis moves shape 2 out: shape 1 goes along -axis
        e.depth = r.mPenetrationDepth;
        Kinematics(A, e.v1, e.w1, e.com1);
        Kinematics(B, e.v2, e.w2, e.com2);
        const SubKey sk(e.sub1, e.other);
        std::lock_guard<std::mutex> lock(mutex);
        PairRec& rec = evValidated[PairKey(A, B)];
        rec.body1 = e.body1;
        rec.body2 = e.body2;
        rec.otherStatic = e.otherStatic;
        auto it = rec.subs.find(sk);
        // several validations of one sub-shape pair in a step (collision steps, manifolds): the deepest is kept, ties by the point
        // (an order-independent choice, so the result does not depend on the worker threads)
        if (it == rec.subs.end()) {
            rec.subs.emplace(sk, e);
        } else if (e.depth > it->second.depth ||
                   (e.depth == it->second.depth && std::make_tuple(e.point.x, e.point.y, e.point.z) < std::make_tuple(it->second.point.x, it->second.point.y, it->second.point.z))) {
            it->second = e;
        }
    }
    void Handle(const JPH::Body& b1, const JPH::Body& b2, const JPH::ContactManifold& m) {
        if (eventsOn && !(b1.IsStatic() && b2.IsStatic())) {
            const bool flip = Flip(b1, b2);
            const JPH::Body& A = flip ? b2 : b1;
            const JPH::Body& B = flip ? b1 : b2;
            std::lock_guard<std::mutex> lock(mutex);
            evActive.emplace(PairKey(A, B), std::make_pair(A.GetID().GetIndexAndSequenceNumber(), B.GetID().GetIndexAndSequenceNumber()));
        }
        if (!staticOn || b1.IsStatic() == b2.IsStatic()) return;
        const JPH::Body& dyn = b1.IsStatic() ? b2 : b1;
        const JPH::Body& stat = b1.IsStatic() ? b1 : b2;
        const JPH::SubShapeID& id = b1.IsStatic() ? m.mSubShapeID2 : m.mSubShapeID1;
        const uint32_t ud = static_cast<uint32_t>(dyn.GetShape()->GetSubShapeUserData(id));
        std::lock_guard<std::mutex> lock(mutex);
        active.emplace(Key(dyn, stat), ud);
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
        case ShapeDesc::Kind::ConvexHull: {
            // WP-mucol: Unity's convex MeshCollider = the hull of the mesh vertices (PartReader.cs:233 forces convex). A hull Jolt
            // cannot build (fewer than 4 non-coplanar points) falls back to the axis-aligned box of the points.
            JPH::Array<JPH::Vec3> pts;
            Vector3f lo(1e30f, 1e30f, 1e30f), hi(-1e30f, -1e30f, -1e30f);
            if (s.hullPoints)
                for (const Vector3f& p : *s.hullPoints) {
                    pts.push_back(JPH::Vec3(p.x, p.y, p.z));
                    lo = Vector3f(std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z));
                    hi = Vector3f(std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z));
                }
            if (pts.size() >= 4) {
                JPH::ConvexHullShapeSettings hs(pts, JPH::cDefaultConvexRadius * 0.0f);
                JPH::ShapeSettings::ShapeResult r = hs.Create();
                if (r.IsValid()) return r.Get();
            }
            if (pts.empty()) return new JPH::BoxShape(JPH::Vec3(0.05f, 0.05f, 0.05f), 0.01f);
            // box of the points around their centre: wrap in an offset (RotatedTranslatedShape)
            JPH::Vec3 h(std::max(0.5f * (hi.x - lo.x), 0.01f), std::max(0.5f * (hi.y - lo.y), 0.01f), std::max(0.5f * (hi.z - lo.z), 0.01f));
            JPH::Ref<JPH::Shape> box = new JPH::BoxShape(h, std::min(0.01f, std::min(h.GetX(), std::min(h.GetY(), h.GetZ()))));
            return new JPH::RotatedTranslatedShape(JPH::Vec3(0.5f * (lo.x + hi.x), 0.5f * (lo.y + hi.y), 0.5f * (lo.z + hi.z)), JPH::Quat::sIdentity(), box);
        }
        case ShapeDesc::Kind::Box:
        default: {
            JPH::Vec3 h(s.halfExtents.x, s.halfExtents.y, s.halfExtents.z);
            float convexRadius = std::min(JPH::cDefaultConvexRadius, std::min(h.GetX(), std::min(h.GetY(), h.GetZ())));
            return new JPH::BoxShape(h, convexRadius);
        }
    }
}

}  // namespace

HullMassProps HullMassProperties(const std::vector<Vector3f>& points) {
    HullMassProps out;
    if (points.size() < 4) return out;
    // Jolt's allocator / factory must be registered while any Jolt object lives (no PhysicsWorld need exist)
    struct Guard {
        Guard() { JoltAcquire(); }
        ~Guard() { JoltRelease(); }
    } guard;
    JPH::Array<JPH::Vec3> pts;
    for (const Vector3f& p : points) pts.push_back(JPH::Vec3(p.x, p.y, p.z));
    JPH::ConvexHullShapeSettings hs(pts, 0.0f);
    JPH::ShapeSettings::ShapeResult r = hs.Create();
    if (!r.IsValid()) return out;
    const JPH::ConvexHullShape* shape = static_cast<const JPH::ConvexHullShape*>(r.Get().GetPtr());
    const JPH::MassProperties mp = shape->GetMassProperties();
    if (!(mp.mMass > 0.0f)) return out;
    out.valid = true;
    out.volume = static_cast<double>(mp.mMass) / static_cast<double>(shape->GetDensity());
    const JPH::Vec3 c = shape->GetCenterOfMass();
    out.centroid = Vector3d(c.GetX(), c.GetY(), c.GetZ());
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) out.inertia[i][j] = static_cast<double>(mp.mInertia(i, j)) / static_cast<double>(mp.mMass);
    return out;
}

namespace {

// WP-20b: a child collider of a cluster compound. Boxes keep a small convex radius (at most half the thinnest half extent,
// as the spike's part proxies: core/spikes/jolt/src/shapes.hpp) so thin proxies stay box-like.
JPH::Ref<JPH::Shape> MakeChildShape(const ShapeDesc& s, uint32_t userData) {
    JPH::Ref<JPH::Shape> shape;
    if (s.kind == ShapeDesc::Kind::Box) {
        JPH::Vec3 h(s.halfExtents.x, s.halfExtents.y, s.halfExtents.z);
        const float cr = std::min(0.05f, 0.5f * std::min(h.GetX(), std::min(h.GetY(), h.GetZ())));
        shape = new JPH::BoxShape(h, cr);
    } else {
        shape = MakeShape(s);
    }
    shape->SetUserData(userData);
    return shape;
}

JPH::Mat44 InertiaMat(const double I[3][3]) {
    JPH::Mat44 m = JPH::Mat44::sZero();
    for (int c = 0; c < 3; ++c)
        m.SetColumn3(c, JPH::Vec3(static_cast<float>(I[0][c]), static_cast<float>(I[1][c]), static_cast<float>(I[2][c])));
    m.SetColumn4(3, JPH::Vec4(0, 0, 0, 1));
    return m;
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
    // WP-20b: compound bodies keep their inner compound (without the centre-of-mass offset) and its volume centre, so that
    // SetMassProperties can move the centre of mass by swapping only the OffsetCenterOfMassShape decorator.
    struct CompoundInfo {
        JPH::Ref<JPH::Shape> inner;
        JPH::Vec3 innerCom;
    };
    std::unordered_map<BodyId, CompoundInfo> compounds;
    struct ConstraintSlot {
        JPH::Ref<JPH::TwoBodyConstraint> c;
        BodyId a = kInvalidBody, b = kInvalidBody;
    };
    std::vector<ConstraintSlot> constraints;  // index = ConstraintId; a removed slot has c == nullptr
    size_t liveConstraints = 0;
    // WP-20c: static contacts of the last step (tracking on: the listener is installed)
    std::unique_ptr<StaticContactListener> contactListener;
    std::vector<PhysicsWorld::StaticContact> staticContacts;
    std::vector<PhysicsWorld::ContactEvent> contactEvents;  // WP-20e
    // the listener lives while static-contact tracking or event tracking is on
    void SyncListener(bool staticOn, bool eventsOn, float offset) {
        if (!staticOn && !eventsOn) {
            if (contactListener) {
                system.SetContactListener(nullptr);
                contactListener.reset();
            }
            staticContacts.clear();
            contactEvents.clear();
            return;
        }
        if (!contactListener) {
            contactListener = std::make_unique<StaticContactListener>();
            system.SetContactListener(contactListener.get());
        }
        if (contactListener->staticOn != staticOn) {
            contactListener->staticOn = staticOn;
            contactListener->lastParts.clear();
            staticContacts.clear();
        }
        if (contactListener->eventsOn != eventsOn) {
            contactListener->eventsOn = eventsOn;
            contactListener->evLast.clear();
            contactEvents.clear();
        }
        contactListener->contactOffset = offset;
    }

    JPH::BodyInterface& Bi() { return system.GetBodyInterfaceNoLock(); }
    const JPH::BodyInterface& Bi() const { return system.GetBodyInterfaceNoLock(); }

    void RemoveConstraintSlot(ConstraintSlot& s) {
        if (!s.c) return;
        system.RemoveConstraint(s.c);
        s.c = nullptr;
        s.a = s.b = kInvalidBody;
        --liveConstraints;
    }
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
        // Constraints first (they reference bodies), then the bodies, before the system dies (Jolt asserts on leaked bodies).
        for (Impl::ConstraintSlot& s : impl_->constraints) impl_->RemoveConstraintSlot(s);
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
    Impl& m = *impl_;
    if (m.contactListener) m.contactListener->BeginStep();
    m.system.Update(dt, collisionSteps, m.temp.get(), m.jobs.get());
    ++m.steps;
    if (m.contactListener) {
        m.contactListener->EndStep(m.staticContacts);
        m.contactListener->EndStepEvents(m.contactEvents);
    }
}

void PhysicsWorld::SetStaticContactTracking(bool on) {
    Impl& m = *impl_;
    const bool events = m.contactListener && m.contactListener->eventsOn;
    const float offset = m.contactListener ? m.contactListener->contactOffset : 0.01f;
    if (on == StaticContactTracking()) return;
    m.SyncListener(on, events, offset);
}

bool PhysicsWorld::StaticContactTracking() const { return impl_->contactListener && impl_->contactListener->staticOn; }

const std::vector<PhysicsWorld::StaticContact>& PhysicsWorld::StaticContacts() const { return impl_->staticContacts; }

// WP-20e
void PhysicsWorld::SetContactEventTracking(bool on, float contactOffset) {
    Impl& m = *impl_;
    m.SyncListener(StaticContactTracking(), on, contactOffset);
}

bool PhysicsWorld::ContactEventTracking() const { return impl_->contactListener && impl_->contactListener->eventsOn; }

const std::vector<PhysicsWorld::ContactEvent>& PhysicsWorld::ContactEvents() const { return impl_->contactEvents; }

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
    for (Impl::ConstraintSlot& s : m.constraints)
        if (s.c && (s.a == id || s.b == id)) m.RemoveConstraintSlot(s);
    m.compounds.erase(id);
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

// ---- static geometry (WP-12b; WP-20b's pad terrain patch uses AddStaticMesh too) ------------------------------------------------

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
    if (triangles.size() < 3 || m.order.size() >= m.cfg.maxBodies) return kInvalidBody;
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
    if (res.HasError()) {
        log::Warning(std::string("PhysicsWorld::AddStaticMesh: ") + res.GetError().c_str());
        return kInvalidBody;
    }
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

// ---- WP-20b: compound cluster bodies and fixed constraints ------------------------------------------------------------------

BodyId PhysicsWorld::CreateCompoundBody(const CompoundBodyDesc& d) {
    Impl& m = *impl_;
    if (m.order.size() >= m.cfg.maxBodies || d.children.empty()) return kInvalidBody;
    // the inner shape: one child as a RotatedTranslatedShape (Jolt's compounds need two or more), else a StaticCompoundShape
    JPH::Ref<JPH::Shape> inner;
    if (d.children.size() == 1) {
        const CompoundChild& c = d.children.front();
        JPH::RotatedTranslatedShapeSettings rts(JPH::Vec3(c.position.x, c.position.y, c.position.z),
                                                JPH::Quat(c.rotation.x, c.rotation.y, c.rotation.z, c.rotation.w).Normalized(),
                                                MakeChildShape(c.shape, c.userData));
        JPH::ShapeSettings::ShapeResult r = rts.Create();
        if (r.HasError()) {
            log::Warning(std::string("PhysicsWorld::CreateCompoundBody: ") + r.GetError().c_str());
            return kInvalidBody;
        }
        inner = r.Get();
    } else {
        JPH::StaticCompoundShapeSettings sc;
        for (const CompoundChild& c : d.children)
            sc.AddShape(JPH::Vec3(c.position.x, c.position.y, c.position.z),
                        JPH::Quat(c.rotation.x, c.rotation.y, c.rotation.z, c.rotation.w).Normalized(), MakeChildShape(c.shape, c.userData),
                        c.userData);
        JPH::ShapeSettings::ShapeResult r = sc.Create();
        if (r.HasError()) {
            log::Warning(std::string("PhysicsWorld::CreateCompoundBody: ") + r.GetError().c_str());
            return kInvalidBody;
        }
        inner = r.Get();
    }
    // the centre of mass is the caller's, not the volume centre of the colliders: c_vol + (com - c_vol); com == 0 gives exactly 0
    const JPH::Vec3 innerCom = inner->GetCenterOfMass();
    const JPH::Vec3 com(d.centerOfMass.x, d.centerOfMass.y, d.centerOfMass.z);
    JPH::Ref<JPH::Shape> shape = new JPH::OffsetCenterOfMassShape(inner, com - innerCom);
    const bool isStatic = d.motion == MotionType::Static;
    const JPH::EMotionType mt = isStatic ? JPH::EMotionType::Static
                                         : (d.motion == MotionType::Kinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic);
    JPH::BodyCreationSettings bcs(shape, ToRVec3(d.position),
                                  JPH::Quat(static_cast<float>(d.rotation.x), static_cast<float>(d.rotation.y),
                                            static_cast<float>(d.rotation.z), static_cast<float>(d.rotation.w)),
                                  mt, isStatic ? Layers::STATIC : Layers::MOVING);
    if (mt == JPH::EMotionType::Dynamic) {
        bcs.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
        bcs.mMassPropertiesOverride.mMass = static_cast<float>(d.mass);
        bcs.mMassPropertiesOverride.mInertia = InertiaMat(d.inertia);
    }
    bcs.mLinearVelocity = ToVec3(d.linearVelocity);
    bcs.mAngularVelocity = ToVec3(d.angularVelocity);
    bcs.mFriction = d.friction;
    bcs.mRestitution = d.restitution;
    bcs.mLinearDamping = 0.0f;
    bcs.mAngularDamping = 0.0f;
    bcs.mMaxLinearVelocity = d.maxLinearVelocity;
    bcs.mMaxAngularVelocity = d.maxAngularVelocity;
    bcs.mAllowSleeping = d.allowSleeping;
    bcs.mGravityFactor = 0.0f;
    bcs.mUseManifoldReduction = d.useManifoldReduction;
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
    m.compounds[raw] = Impl::CompoundInfo{inner, innerCom};
    return raw;
}

Vector3d PhysicsWorld::GetCenterOfMassPosition(BodyId id) const {
    return FromRVec3(impl_->Bi().GetCenterOfMassPosition(JPH::BodyID(id)));
}

void PhysicsWorld::SetMassProperties(BodyId id, double mass, const double inertia[3][3], const Vector3f& centerOfMass) {
    Impl& m = *impl_;
    auto it = m.compounds.find(id);
    if (it == m.compounds.end()) return;
    const JPH::Vec3 com(centerOfMass.x, centerOfMass.y, centerOfMass.z);
    JPH::Ref<JPH::Shape> shape = new JPH::OffsetCenterOfMassShape(it->second.inner, com - it->second.innerCom);
    // keeps the body origin in place and moves the centre of mass (Body::UpdateCenterOfMassInternal)
    m.Bi().SetShape(JPH::BodyID(id), shape, false, JPH::EActivation::DontActivate);
    JPH::BodyLockWrite lock(m.system.GetBodyLockInterfaceNoLock(), JPH::BodyID(id));
    if (!lock.Succeeded() || !lock.GetBody().IsDynamic()) return;
    JPH::MassProperties props;
    props.mMass = static_cast<float>(mass);
    props.mInertia = InertiaMat(inertia);
    lock.GetBody().GetMotionProperties()->SetMassProperties(JPH::EAllowedDOFs::All, props);
}

void PhysicsWorld::SetAngularDamping(BodyId id, float damping) {
    JPH::BodyLockWrite lock(impl_->system.GetBodyLockInterfaceNoLock(), JPH::BodyID(id));
    if (!lock.Succeeded() || !lock.GetBody().IsDynamic()) return;
    lock.GetBody().GetMotionProperties()->SetAngularDamping(damping);
}

ConstraintId PhysicsWorld::CreateFixedConstraint(BodyId a, BodyId b, const Vector3d& point) {
    Impl& m = *impl_;
    if (!IsValid(a) || !IsValid(b) || a == b) return kInvalidConstraint;
    JPH::FixedConstraintSettings s;
    s.mSpace = JPH::EConstraintSpace::WorldSpace;
    s.mAutoDetectPoint = false;
    s.mPoint1 = s.mPoint2 = ToRVec3(point);
    JPH::TwoBodyConstraint* c = m.Bi().CreateConstraint(&s, JPH::BodyID(a), JPH::BodyID(b));
    if (c == nullptr) return kInvalidConstraint;
    m.system.AddConstraint(c);
    Impl::ConstraintSlot slot;
    slot.c = c;
    slot.a = a;
    slot.b = b;
    m.constraints.push_back(slot);
    ++m.liveConstraints;
    return static_cast<ConstraintId>(m.constraints.size() - 1);
}

// WP-20e: the RB anchor's FixedJoint to the world (Vessel.cs:1162). Body 1 is Jolt's static sFixedToWorld, so the constraint's lambda is
// the impulse on `body` (GetConstraintImpulse's convention: applied to body b).
ConstraintId PhysicsWorld::CreateFixedToWorldConstraint(BodyId body, const Vector3d& point) {
    Impl& m = *impl_;
    if (!IsValid(body) || IsStatic(body)) return kInvalidConstraint;
    JPH::FixedConstraintSettings s;
    s.mSpace = JPH::EConstraintSpace::WorldSpace;
    s.mAutoDetectPoint = false;
    s.mPoint1 = s.mPoint2 = ToRVec3(point);
    JPH::TwoBodyConstraint* c = m.Bi().CreateConstraint(&s, JPH::BodyID(), JPH::BodyID(body));
    if (c == nullptr) return kInvalidConstraint;
    m.system.AddConstraint(c);
    Impl::ConstraintSlot slot;
    slot.c = c;
    slot.a = kInvalidBody;
    slot.b = body;
    m.constraints.push_back(slot);
    ++m.liveConstraints;
    return static_cast<ConstraintId>(m.constraints.size() - 1);
}

void PhysicsWorld::RemoveConstraint(ConstraintId id) {
    Impl& m = *impl_;
    if (id >= m.constraints.size()) return;
    m.RemoveConstraintSlot(m.constraints[id]);
}

size_t PhysicsWorld::ConstraintCount() const { return impl_->liveConstraints; }

void PhysicsWorld::GetConstraintImpulse(ConstraintId id, Vector3d& linear, Vector3d& angular) const {
    linear = Vector3d();
    angular = Vector3d();
    const Impl& m = *impl_;
    if (id >= m.constraints.size() || !m.constraints[id].c) return;
    const auto* fc = static_cast<const JPH::FixedConstraint*>(m.constraints[id].c.GetPtr());
    linear = FromVec3(fc->GetTotalLambdaPosition());
    angular = FromVec3(fc->GetTotalLambdaRotation());
}

// ---- WP-19b: frames of the physics bubble ---------------------------------------------------------------------------------

bool PhysicsWorld::IsStatic(BodyId id) const {
    if (!IsValid(id)) return false;
    return impl_->Bi().GetMotionType(JPH::BodyID(id)) == JPH::EMotionType::Static;
}

size_t PhysicsWorld::TranslateBodies(const Vector3d& delta, bool moving, bool statics) {
    Impl& m = *impl_;
    size_t n = 0;
    for (BodyId id : m.order) {
        const JPH::BodyID b(id);
        const bool isStatic = m.Bi().GetMotionType(b) == JPH::EMotionType::Static;
        if (isStatic ? !statics : !moving) continue;
        // the body origin in double (Body::GetPosition), moved by the double delta: no float rounding of the position
        m.Bi().SetPosition(b, m.Bi().GetPosition(b) + ToRVec3(delta), JPH::EActivation::DontActivate);
        ++n;
    }
    return n;
}

void PhysicsWorld::SetStaticCollisions(bool enabled) {
    impl_->ovb.staticOn = enabled;
    impl_->olp.staticOn = enabled;
}

bool PhysicsWorld::StaticCollisions() const { return impl_->olp.staticOn; }

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
