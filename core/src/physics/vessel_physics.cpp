// WP-20b: VesselPhysics, the vessel <-> PhysicsWorld binding of the reduced rigid-body model. See ksp/physics/vessel_physics.hpp.
//
// Ported KSP statements (Part.Start rigid body rules, the rb.mass / inertia rules, CalculatePhysicsStats' sums, Part.ResumeVelocity,
// FlightIntegrator.Integrate's force half, Part.AddForce* semantics) follow the Mono float rule and use no std:: maths; they carry
// their C# citations. The rest (cluster sums, the 3x3 algebra, the bookkeeping of the solver) is engine glue in double.
#include "ksp/physics/vessel_physics.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/physics/reference_frame.hpp"
#include "ksp/unity/mathf.hpp"
#include "vessel_physics_internal.hpp"

namespace ksp::physics {

constexpr int kFull = 0;  // Part.PhysicalSignificance.FULL

namespace {

using detail::Cross;
using detail::Mat3;
using detail::Normalized;

uint32_t NextGroupId() {
    static uint32_t next = 0x40000000u;  // above typical persistent ids; only used when a vessel has none
    return ++next;
}

}  // namespace

// ===================================================================================== collider proxies

std::vector<ColliderProxy> DragCubeColliderProxy(const Part& part) {
    ColliderProxy c;
    if (!part.DragCubes.None() && !part.DragCubes.Cubes().empty()) {
        const DragCube& cube = part.DragCubes.Cubes().front();
        c.center = cube.Center();
        const Vector3f size = cube.Size();
        c.halfExtents = Vector3f(std::max(size.x * 0.5f, 0.05f), std::max(size.y * 0.5f, 0.05f), std::max(size.z * 0.5f, 0.05f));
    }
    return {c};
}

Vector3f ProxyInertiaPerUnitMass(const std::vector<ColliderProxy>& colliders) {
    if (colliders.empty()) return Vector3f(1.0f, 1.0f, 1.0f);
    if (colliders.size() == 1) {
        const Vector3f h = colliders.front().halfExtents;
        return Vector3f((h.y * h.y + h.z * h.z) / 3.0f, (h.x * h.x + h.z * h.z) / 3.0f, (h.x * h.x + h.y * h.y) / 3.0f);
    }
    double vol = 0.0;
    Vector3d c;
    for (const ColliderProxy& b : colliders) {
        const double v = 8.0 * b.halfExtents.x * b.halfExtents.y * b.halfExtents.z;
        vol += v;
        c += Vector3d(b.center) * v;
    }
    c = c * (1.0 / vol);
    Mat3 I;
    for (const ColliderProxy& b : colliders) {
        const double w = 8.0 * b.halfExtents.x * b.halfExtents.y * b.halfExtents.z / vol;
        const Vector3d h(b.halfExtents);
        const Vector3d d((h.y * h.y + h.z * h.z) / 3.0, (h.x * h.x + h.z * h.z) / 3.0, (h.x * h.x + h.y * h.y) / 3.0);
        I += Mat3::RotatedDiagonal(Mat3::Rotation(QuaternionD(b.rotation)), d * w);
        I += Mat3::ParallelAxis(w, Vector3d(b.center) - c);
    }
    return Vector3f(static_cast<float>(I.m[0][0]), static_cast<float>(I.m[1][1]), static_cast<float>(I.m[2][2]));
}

// ===================================================================================== construction

namespace {
// The vessel-frame pose of every part (pristine coordinates: orgPos / orgRot under the vessel transform)
PartTransform VesselFrameTransform(const Part& p) {
    PartTransform t;
    t.position = p.fields.orgPos;
    t.rotation = p.fields.orgRot;
    t.localScale = p.partTransform.localScale;
    return t;
}
}  // namespace

VesselPhysics::VesselPhysics(PhysicsWorld& world, Vessel& vessel, const VesselPose& pose, const VesselPhysicsOptions& options, uint32_t group)
    : world_(&world), vessel_(&vessel), options_(options) {
    // Vessel.SetPosition(pos, usePristineCoords) / SetRotation (Vessel.cs:3683-3740): part = vessel pose * (orgPos, orgRot). The
    // centre of mass of a part is its vessel-frame Transform.TransformPoint(CoMOffset) (rb.centerOfMass = CoMOffset, the float
    // computation of Unity), placed with the same pose.
    std::vector<PartPhysicsState> states(vessel.parts.size());
    for (size_t i = 0; i < vessel.parts.size(); ++i) {
        const Part& p = *vessel.parts[i];
        const PartTransform t = VesselFrameTransform(p);
        PartPhysicsState& s = states[i];
        s.position = pose.position + pose.rotation * Vector3d(t.position);
        s.rotation = pose.rotation * QuaternionD(t.rotation);
        s.centerOfMass = pose.position + pose.rotation * Vector3d(t.TransformPoint(p.fields.CoMOffset));
    }
    Init(states, group);
}

VesselPhysics::VesselPhysics(PhysicsWorld& world, Vessel& vessel, const std::vector<PartPhysicsState>& partStates,
                             const VesselPhysicsOptions& options, uint32_t group)
    : world_(&world), vessel_(&vessel), options_(options) {
    Init(partStates, group);
}

VesselPhysics::~VesselPhysics() { DestroyBodies(); }

// PartJoint data measured in the vessel frame (relative poses only): the binding's parts by their orgPos / orgRot
std::function<const PartTransform&(const Part&)> VesselPhysics::VesselFrameTransforms() {
    return [this](const Part& p) -> const PartTransform& {
        const int k = index_.Find(&p);
        return k >= 0 ? parts_[static_cast<size_t>(k)]->vesselFrame : p.partTransform;
    };
}

void VesselPhysics::Init(const std::vector<PartPhysicsState>& states, uint32_t group) {
    group_ = group != 0 ? group : (vessel_->persistentId != 0 ? vessel_->persistentId : NextGroupId());
    vessel_->UpdateMassStats();  // FlightIntegrator.cs:452 (physicsMass of every part, the physicsless children folded in)
    if (options_.loadSolve != LoadSolve::None) world_->SetStaticContactTracking(true);  // the ground supports of the load solve
    IndexParts();
    // the attach joints (Part.Start -> CreateAttachJoint) and the struts (CModuleStrut.OnTargetSet) now; the autostruts are secured by
    // the SecureAutoStruts coroutine after the next physics step (CycleAutoStrut on Part.Unpack -> ResetJoints, Part.cs:3824 / :8043)
    joints_ = BuildAttachJoints(*vessel_, options_.jointGlobals, VesselFrameTransforms());
    const std::vector<PartJointSpec> struts = BuildStrutJoints(*vessel_, options_.jointGlobals, VesselFrameTransforms());
    joints_.insert(joints_.end(), struts.begin(), struts.end());
    autoStrutBegin_ = joints_.size();
    cut_.assign(joints_.size(), false);
    subAlive_.assign(joints_.size(), 0);
    for (size_t j = 0; j < joints_.size(); ++j) subAlive_[j] = static_cast<uint8_t>((1u << joints_[j].internalJoints) - 1u);
    stepValid_ = false;
    structureDirty_ = true;
    breaks_.clear();
    jointLoads_.assign(joints_.size(), JointLoad{});
    BuildClusters(states);
    SyncPartTransforms();
    autoStrutCountdown_ = 0;
    if (options_.autoStrutDelaySteps <= 0) SecureAutoStruts();
    else autoStrutCountdown_ = options_.autoStrutDelaySteps;
}

void VesselPhysics::IndexParts() {
    parts_.clear();
    index_.Reserve(vessel_->parts.size());
    const size_t n = vessel_->parts.size();
    for (size_t i = 0; i < n; ++i) {
        auto r = std::make_unique<PartRec>();
        r->part = vessel_->parts[i];
        index_.Insert(r->part, static_cast<int>(i));
        parts_.push_back(std::move(r));
    }
    sameVesselCollisionAll_ = n > 0;
    for (size_t i = 0; i < n; ++i) {
        PartRec& r = *parts_[i];
        Part& p = *r.part;
        r.rb = p.hasRigidbody;
        Part* owner = RigidBodyPart(&p);
        r.owner = index_.Find(owner);
        r.colliders = options_.colliders ? options_.colliders(p) : DragCubeColliderProxy(p);
        r.inertiaPerUnit = ProxyInertiaPerUnitMass(r.colliders);
        r.rbMass = PartRigidbodyMass(p);
        r.inertia = r.rb ? PartInertiaTensor(r.inertiaPerUnit, r.rbMass, options_.inertiaFloor) : Vector3f();
        r.angularDrag = options_.defaultAngularDrag;
        r.sameVesselCollision = p.fields.sameVesselCollision;
        if (!r.sameVesselCollision) sameVesselCollisionAll_ = false;
        r.vesselFrame = VesselFrameTransform(p);
    }
    partForce_.assign(n, Vector3d());
    partMoment_.assign(n, Vector3d());
    stepForce_.assign(n, Vector3d());
    stepMoment_.assign(n, Vector3d());
}

namespace {
bool IsDecouplerPart(Part& p) { return p.isDecoupler() || p.isAnchoredDecoupler(); }
}  // namespace

void VesselPhysics::BuildClusters(const std::vector<PartPhysicsState>& states) {
    const size_t n = parts_.size();
    // ---- welded components: physicsless parts ride on their RigidBodyPart, attach joints weld their two bodies unless cut or an
    // articulation point (a decoupler in the split policy, the host's isArticulation)
    std::vector<int> uf(n);
    std::iota(uf.begin(), uf.end(), 0);
    auto find = [&uf](int x) {
        while (uf[static_cast<size_t>(x)] != x) x = uf[static_cast<size_t>(x)] = uf[static_cast<size_t>(uf[static_cast<size_t>(x)])];
        return x;
    };
    auto unite = [&](int a, int b) { uf[static_cast<size_t>(find(a))] = find(b); };
    for (size_t i = 0; i < n; ++i)
        if (!parts_[i]->rb && parts_[i]->owner >= 0) unite(static_cast<int>(i), parts_[i]->owner);
    for (size_t j = 0; j < joints_.size(); ++j) {
        const PartJointSpec& js = joints_[j];
        if (cut_[j] || js.kind != JointKind::Attach) continue;
        bool art = options_.policy == ClusterPolicy::SplitAtDecouplers && IsDecouplerPart(*js.child);
        if (!art && options_.isArticulation) art = options_.isArticulation(js);
        if (art) continue;
        const int a = index_.Find(js.hostBody), b = index_.Find(js.targetBody);
        if (a >= 0 && b >= 0) unite(a, b);
    }
    // ---- clusters in order of their first part
    clusters_.clear();
    std::vector<int> rootToCluster(n, -1);
    for (size_t i = 0; i < n; ++i) {
        const int r = find(static_cast<int>(i));
        if (rootToCluster[static_cast<size_t>(r)] < 0) {
            rootToCluster[static_cast<size_t>(r)] = static_cast<int>(clusters_.size());
            clusters_.emplace_back();
        }
        parts_[i]->cluster = rootToCluster[static_cast<size_t>(r)];
        clusters_[static_cast<size_t>(parts_[i]->cluster)].parts.push_back(static_cast<int>(i));
    }
    for (size_t i = 0; i < n; ++i) {  // the reference pose of every part (also those of a piece without a Rigidbody)
        PartRec& r = *parts_[i];
        r.pos0 = states[i].position;
        r.rot0 = states[i].rotation;
        r.com0 = states[i].centerOfMass;
    }
    const float maxAngular = PhysicsGlobals::Instance() != nullptr ? PhysicsGlobals::MaxAngularVelocity() : 50.0f;
    for (ClusterRec& c : clusters_) {
        // mass and centre of mass: sum of rb.mass * rb.worldCenterOfMass from the last part to the first (KSP's loop order,
        // VesselPrecalculate.cs:404-415), then * (1 / mass)
        double mass = 0.0;
        Vector3d sum;
        for (auto it = c.parts.rbegin(); it != c.parts.rend(); ++it) {
            const PartRec& r = *parts_[static_cast<size_t>(*it)];
            if (!r.rb) continue;
            sum += states[static_cast<size_t>(*it)].centerOfMass * static_cast<double>(r.rbMass);
            mass += static_cast<double>(r.rbMass);
        }
        c.mass = mass;
        if (!(mass > 0.0)) continue;  // no Rigidbody in this piece: no body (its parts keep their pose)
        const Vector3d com = sum * (1.0 / mass);
        c.origin0 = com;
        c.comBody = Vector3d();
        // inertia about the centre of mass (physics axes = body axes at creation), the same order
        Mat3 I;
        for (auto it = c.parts.rbegin(); it != c.parts.rend(); ++it) {
            PartRec& r = *parts_[static_cast<size_t>(*it)];
            r.localPos = r.pos0 - c.origin0;
            r.localCom = r.com0 - c.origin0;
            r.ownBody = r.rb ? Mat3::RotatedDiagonal(Mat3::Rotation(r.rot0), Vector3d(r.inertia)) : Mat3();
            if (!r.rb) continue;
            I += r.ownBody;
            I += Mat3::ParallelAxis(static_cast<double>(r.rbMass), r.localCom);
        }
        c.inertia = I;
        // velocities: the linear and angular momentum of the parts (each part moves with its own state)
        Vector3d P, L;
        for (int i : c.parts) {
            const PartRec& r = *parts_[static_cast<size_t>(i)];
            if (!r.rb) continue;
            const PartPhysicsState& s = states[static_cast<size_t>(i)];
            const double m = static_cast<double>(r.rbMass);
            P += s.velocity * m;
            L += r.ownBody * s.angularVelocity + Cross(r.localCom, s.velocity * m);
        }
        c.v = P * (1.0 / mass);
        c.w = I.Solve(L);
        // the compound of every part's colliders (physicsless parts included: their colliders belong to the parent's Rigidbody)
        CompoundBodyDesc d;
        for (int i : c.parts) {
            const PartRec& r = *parts_[static_cast<size_t>(i)];
            for (const ColliderProxy& box : r.colliders) {
                CompoundChild ch;
                ch.shape = ShapeDesc::Box(box.halfExtents);
                ch.position = (r.localPos + r.rot0 * Vector3d(box.center)).ToVector3f();
                ch.rotation = (r.rot0 * QuaternionD(box.rotation)).ToQuaternionf();
                ch.userData = static_cast<uint32_t>(i);
                d.children.push_back(ch);
            }
        }
        d.position = c.origin0;
        d.rotation = QuaternionD::identity();
        d.linearVelocity = c.v;
        d.angularVelocity = c.w;
        d.mass = mass;
        for (int a = 0; a < 3; ++a)
            for (int b = 0; b < 3; ++b) d.inertia[a][b] = I.m[a][b];
        d.friction = options_.friction;
        d.restitution = options_.restitution;
        d.maxAngularVelocity = maxAngular;  // rb.maxAngularVelocity = PhysicsGlobals.MaxAngularVelocity (Part.cs:1880)
        d.vesselGroup = group_;
        d.sameVesselCollision = sameVesselCollisionAll_;
        d.userData = static_cast<uint64_t>(c.parts.front());
        c.body = world_->CreateCompoundBody(d);
        c.origin = c.origin0;
        c.com = c.origin0;
        c.q = QuaternionD::identity();
        c.v = world_->GetLinearVelocity(c.body);   // what the solver holds (float)
        c.w = world_->GetAngularVelocity(c.body);
    }
    RefreshPartPoses();
    clusterVBefore_.assign(clusters_.size(), Vector3d());
    clusterWBefore_.assign(clusters_.size(), Vector3d());
    structureDirty_ = true;
    stepValid_ = false;
    welds_.clear();
    constraints_.clear();
    RebuildWelds();
}

// The inter-cluster welds: one fixed constraint per pair of clusters joined by a live joint (articulation points, and struts / autostruts
// between clusters), at the joint's anchor centroid as the parts stand now. Removed and re-created whenever the live joints change.
void VesselPhysics::RebuildWelds() {
    for (ConstraintId id : constraints_) world_->RemoveConstraint(id);
    constraints_.clear();
    welds_.clear();
    std::vector<std::pair<int, int>> linked;
    for (size_t j = 0; j < joints_.size(); ++j) {
        const PartJointSpec& js = joints_[j];
        if (cut_[j] || subAlive_[j] == 0) continue;
        const int a = index_.Find(js.hostBody), b = index_.Find(js.targetBody), ch = index_.Find(js.child);
        if (a < 0 || b < 0 || ch < 0) continue;
        const int ca = parts_[static_cast<size_t>(a)]->cluster, cb = parts_[static_cast<size_t>(b)]->cluster;
        if (ca == cb) continue;
        if (clusters_[static_cast<size_t>(ca)].body == kInvalidBody || clusters_[static_cast<size_t>(cb)].body == kInvalidBody) continue;
        const std::pair<int, int> key(std::min(ca, cb), std::max(ca, cb));
        if (std::find(linked.begin(), linked.end(), key) != linked.end()) continue;
        linked.push_back(key);
        Vector3d anchor;
        for (int k = 0; k < js.internalJoints; ++k) anchor += Vector3d(js.anchors[static_cast<size_t>(k)]);
        anchor = anchor * (1.0 / static_cast<double>(std::max(1, js.internalJoints)));
        const PartRec& cr = *parts_[static_cast<size_t>(ch)];
        const Vector3d point = cr.posNow + cr.rotNow * anchor;
        const ClusterRec& A = clusters_[static_cast<size_t>(ca)];
        const ConstraintId id = world_->CreateFixedConstraint(A.body, clusters_[static_cast<size_t>(cb)].body, point);
        if (id == kInvalidConstraint) continue;
        constraints_.push_back(id);
        Weld w;
        w.id = id;
        w.a = ca;
        w.b = cb;
        w.pointBodyA = QuaternionD::Inverse(A.q) * (point - A.origin);
        welds_.push_back(w);
    }
}

void VesselPhysics::DestroyBodies() {
    for (ConstraintId id : constraints_) world_->RemoveConstraint(id);
    constraints_.clear();
    welds_.clear();
    for (ClusterRec& c : clusters_)
        if (c.body != kInvalidBody) {
            world_->RemoveBody(c.body);
            c.body = kInvalidBody;
        }
}

// ===================================================================================== frames and poses

Vector3d VesselPhysics::WorldFromPhysics(const Vector3d& p) const { return frameOriginWorld + ReferenceFrame::TruePosition(p); }
Vector3d VesselPhysics::PhysicsFromWorld(const Vector3d& w) const { return ReferenceFrame::PhysicsPosition(w - frameOriginWorld); }

int VesselPhysics::PartIndex(const Part& part) const { return index_.Find(&part); }
bool VesselPhysics::PartHasRigidbody(size_t index) const { return index < parts_.size() && parts_[index]->rb; }

int VesselPhysics::ClusterOf(const Part& part) const {
    const int i = PartIndex(part);
    return i < 0 ? -1 : parts_[static_cast<size_t>(i)]->cluster;
}

int VesselPhysics::OwnerIndex(const Part& part) const {
    const int i = PartIndex(part);
    return i < 0 ? -1 : parts_[static_cast<size_t>(i)]->owner;
}

// x(t) = x0 + (origin(t) - origin0) + (q x_local - x_local): exactly x0 while the body has not moved (q == identity, origin ==
// origin0), otherwise origin(t) + q x_local up to rounding. Evaluated once per step for every part (RefreshPartPoses).
void VesselPhysics::RefreshPartPoses() {
    for (auto& rp : parts_) {
        PartRec& r = *rp;
        const ClusterRec& c = clusters_[static_cast<size_t>(r.cluster)];
        if (c.body == kInvalidBody) {
            r.posNow = r.pos0;
            r.comNow = r.com0;
            r.rotNow = r.rot0;
            continue;
        }
        const Vector3d shift = c.origin - c.origin0;
        r.posNow = r.pos0 + shift + (c.q * r.localPos - r.localPos);
        r.comNow = r.com0 + shift + (c.q * r.localCom - r.localCom);
        r.rotNow = c.q * r.rot0;
    }
}

Vector3d VesselPhysics::PartPositionNow(int i) const { return parts_[static_cast<size_t>(i)]->posNow; }
Vector3d VesselPhysics::PartComNow(int i) const { return parts_[static_cast<size_t>(i)]->comNow; }
QuaternionD VesselPhysics::PartRotationNow(int i) const { return parts_[static_cast<size_t>(i)]->rotNow; }

void VesselPhysics::CacheClusterStates() {
    for (ClusterRec& c : clusters_) {
        if (c.body == kInvalidBody) continue;
        c.origin = world_->GetPosition(c.body);
        c.com = world_->GetCenterOfMassPosition(c.body);
        c.q = Normalized(world_->GetRotation(c.body));
        c.v = world_->GetLinearVelocity(c.body);
        c.w = world_->GetAngularVelocity(c.body);
    }
    RefreshPartPoses();
}

size_t VesselPhysics::ClusterCount() const { return clusters_.size(); }
BodyId VesselPhysics::ClusterBody(size_t cluster) const { return clusters_[cluster].body; }
const std::vector<int>& VesselPhysics::ClusterParts(size_t cluster) const { return clusters_[cluster].parts; }
double VesselPhysics::ClusterMass(size_t cluster) const { return clusters_[cluster].mass; }
Vector3d VesselPhysics::ClusterCenterOfMass(size_t cluster) const { return clusters_[cluster].com; }
Vector3d VesselPhysics::ClusterVelocity(size_t cluster) const { return clusters_[cluster].v; }
Vector3d VesselPhysics::ClusterAngularVelocity(size_t cluster) const { return clusters_[cluster].w; }

void VesselPhysics::ClusterInertia(size_t cluster, double out[3][3]) const {
    const ClusterRec& c = clusters_[cluster];
    const Mat3 w = c.inertia.Rotated(Mat3::Rotation(c.q));
    for (int a = 0; a < 3; ++a)
        for (int b = 0; b < 3; ++b) out[a][b] = w.m[a][b];
}

float VesselPhysics::PartMass(const Part& part) const {
    const int i = PartIndex(part);
    return i < 0 ? 0.0f : parts_[static_cast<size_t>(i)]->rbMass;
}

Vector3f VesselPhysics::PartInertia(const Part& part) const {
    const int i = PartIndex(part);
    return i < 0 ? Vector3f() : parts_[static_cast<size_t>(i)]->inertia;
}

float VesselPhysics::AngularDrag(const Part& part) const {
    const int i = PartIndex(part);
    return i < 0 ? 0.0f : parts_[static_cast<size_t>(i)]->angularDrag;
}

PartPhysicsState VesselPhysics::PartState(const Part& part) const {
    const int i = PartIndex(part);
    return i < 0 ? PartPhysicsState{} : StateAt(i);
}

PartPhysicsState VesselPhysics::StateAt(int i) const {
    PartPhysicsState s;
    const PartRec& r = *parts_[static_cast<size_t>(i)];
    const ClusterRec& c = clusters_[static_cast<size_t>(r.cluster)];
    s.position = PartPositionNow(i);
    s.rotation = PartRotationNow(i);
    s.centerOfMass = PartComNow(i);
    s.velocity = c.body == kInvalidBody ? Vector3d() : c.v + Cross(c.w, s.centerOfMass - c.com);
    s.angularVelocity = c.body == kInvalidBody ? Vector3d() : c.w;
    s.cluster = r.cluster;
    return s;
}

std::vector<PartPhysicsState> VesselPhysics::PartStates() const {
    std::vector<PartPhysicsState> out(parts_.size());
    for (size_t i = 0; i < parts_.size(); ++i) out[i] = StateAt(static_cast<int>(i));
    return out;
}

void VesselPhysics::MapVesselParts() const {
    const size_t count = vessel_->parts.size();
    scratchIdx_.resize(count);
    bool same = count == parts_.size();
    for (size_t k = 0; same && k < count; ++k) same = vessel_->parts[k] == parts_[k]->part;
    for (size_t k = 0; k < count; ++k) scratchIdx_[k] = same ? static_cast<int>(k) : PartIndex(*vessel_->parts[k]);
}

void VesselPhysics::SyncPartTransforms() {
    for (size_t i = 0; i < parts_.size(); ++i) {
        PartTransform& t = parts_[i]->part->partTransform;
        t.position = TransformFromPhysics(PartPositionNow(static_cast<int>(i)));
        t.rotation = PartRotationNow(static_cast<int>(i)).ToQuaternionf();
    }
    // the Vessel component sits on the root part's GameObject: vessel.transform == rootPart.transform
    if (vessel_ != nullptr && vessel_->loaded && vessel_->rootPart != nullptr) {
        vessel_->vesselTransform.position = vessel_->rootPart->partTransform.position;
        vessel_->vesselTransform.rotation = vessel_->rootPart->partTransform.rotation;
    }
}

// ===================================================================================== state readback

// VesselPrecalculate.cs:388 CalculatePhysicsStats, its loaded branch (:388-460) over the rigid-body states of the reduced model, in
// the double frame: rb.worldCenterOfMass / rb.position are the parts' double world positions, rb.velocity the cluster's velocity
// at the part's centre of mass, rb.angularVelocity the cluster's (all parts of a rigid cluster share it).
VesselPhysicsState VesselPhysics::ReadState() const {
    VesselPhysicsState out;
    if (vessel_ == nullptr) return out;
    PartTransform& referenceTransform = vessel_->ReferenceTransform();
    const QuaternionD quaternionD = QuaternionD::Inverse(QuaternionD(referenceTransform.rotation));
    Vector3d zero, zero2, zero3;
    double num = 0.0;
    const int count = static_cast<int>(vessel_->parts.size());
    MapVesselParts();
    const std::vector<int>& idx = scratchIdx_;
    // quaternionD * rb.angularVelocity: the same for every part of a cluster (one rigid body), evaluated once per cluster
    std::vector<Vector3d>& wRef = scratchRel_;
    wRef.resize(clusters_.size());
    for (size_t c = 0; c < clusters_.size(); ++c) wRef[c] = quaternionD * clusters_[c].w;
    int index = count;
    while (index-- > 0) {
        const int pi = idx[static_cast<size_t>(index)];
        if (pi < 0 || !parts_[static_cast<size_t>(pi)]->rb) continue;
        const Part& part = *parts_[static_cast<size_t>(pi)]->part;
        const ClusterRec& c = clusters_[static_cast<size_t>(parts_[static_cast<size_t>(pi)]->cluster)];
        if (c.body == kInvalidBody) continue;
        const double physicsMass = part.fields.physicsMass;
        const Vector3d comPhys = PartComNow(pi);
        zero += WorldFromPhysics(comPhys) * physicsMass;
        const Vector3d vector3d = (c.v + Cross(c.w, comPhys - c.com)) * physicsMass;
        zero2 += vector3d;
        zero3 += wRef[static_cast<size_t>(parts_[static_cast<size_t>(pi)]->cluster)] * physicsMass;
        num += physicsMass;
    }
    if (!(num > 0.0)) return out;
    out.totalMass = num;
    const double num2 = 1.0 / num;
    out.CoMD = zero * num2;
    out.rb_velocityD = zero2 * num2;
    out.angularVelocityD = zero3 * num2;
    // MOI (VesselPrecalculate.cs:436-453): rb.inertiaTensor, part.transform.rotation (the synced transform), identity
    // rb.inertiaTensorRotation (the proxies are axis-aligned boxes), rb.position - CoMD and rb.mass, part order
    std::vector<PartRigidbodyState>& states = scratchStates_;
    std::vector<Vector3d>& rel = scratchRel_;  // (wRef is no longer needed)
    states.assign(static_cast<size_t>(count), PartRigidbodyState{});
    rel.assign(static_cast<size_t>(count), Vector3d());
    for (int i = 0; i < count; ++i) {
        const int pi = idx[static_cast<size_t>(i)];
        if (pi < 0) continue;
        const PartRec& r = *parts_[static_cast<size_t>(pi)];
        if (!r.rb || clusters_[static_cast<size_t>(r.cluster)].body == kInvalidBody) continue;
        PartRigidbodyState& s = states[static_cast<size_t>(i)];
        s.present = true;
        s.mass = r.rbMass;
        s.inertiaTensor = r.inertia;
        s.rotation = r.part->partTransform.rotation;
        rel[static_cast<size_t>(i)] = WorldFromPhysics(PartPositionNow(pi)) - out.CoMD;
    }
    out.MOI = Vessel::MOIFromRigidbodies(states, rel, quaternionD, referenceTransform);
    return out;
}

PartRigidbodyState VesselPhysics::RigidbodyState(const Part& part) const {
    PartRigidbodyState s;
    const int i = PartIndex(part);
    if (i < 0) return PartRigidbodyState::FromPart(part);
    const PartRec& r = *parts_[static_cast<size_t>(i)];
    const PartPhysicsState st = StateAt(i);
    s.present = r.rb;
    s.physicsMass = part.fields.physicsMass;
    s.mass = r.rbMass;
    s.worldCenterOfMass = TransformFromPhysics(st.centerOfMass);
    s.position = TransformFromPhysics(st.position);
    s.velocity = st.velocity.ToVector3f();
    s.angularVelocity = st.angularVelocity.ToVector3f();
    s.inertiaTensor = r.inertia;
    s.inertiaTensorRotation = Quaternionf::identity();
    s.rotation = st.rotation.ToQuaternionf();
    return s;
}

// rootPart.rb.GetPointVelocity(vessel.CoM) and rootPart.rb.angularVelocity (VesselPrecalculate.cs:491-497)
bool VesselPhysics::RootRigidbody(const Vector3f& point, Vector3f& pointVelocity, Vector3f& angularVelocity) const {
    Part* root = vessel_->rootPart;
    if (root == nullptr) return false;
    const int i = PartIndex(*root);
    if (i < 0 || !parts_[static_cast<size_t>(i)]->rb) return false;
    const ClusterRec& c = clusters_[static_cast<size_t>(parts_[static_cast<size_t>(i)]->cluster)];
    if (c.body == kInvalidBody) return false;
    const Vector3d p = PhysicsFromTransform(Vector3d(point));
    pointVelocity = (c.v + Cross(c.w, p - c.com)).ToVector3f();
    angularVelocity = c.w.ToVector3f();
    return true;
}

// ===================================================================================== velocities

// Vessel.cs:3197 ChangeWorldVelocity: every Rigidbody's velocity += dv (also Vessel.OffsetVelocity, Vessel.cs:3742, and Krakensbane's
// AddExcess); the Vessel's krakensbaneAcc bookkeeping of the C# is the caller's (WP-19b)
void VesselPhysics::ChangeWorldVelocity(const Vector3d& dv) {
    for (ClusterRec& c : clusters_) {
        if (c.body == kInvalidBody) continue;
        world_->SetLinearVelocity(c.body, world_->GetLinearVelocity(c.body) + dv);
        c.v = world_->GetLinearVelocity(c.body);
    }
}

void VesselPhysics::SetLinearVelocity(const Vector3d& v) {
    for (ClusterRec& c : clusters_) {
        if (c.body == kInvalidBody) continue;
        world_->SetLinearVelocity(c.body, v);
        c.v = world_->GetLinearVelocity(c.body);
    }
}

// Part.cs:3827 ResumeVelocity (to :3843) for the part's cluster, evaluated at the cluster's centre of mass
void VesselPhysics::ResumePartVelocity(Part& part) {
    const int i = PartIndex(part);
    if (i < 0) return;
    ClusterRec& c = clusters_[static_cast<size_t>(parts_[static_cast<size_t>(i)]->cluster)];
    if (c.body == kInvalidBody) return;
    Vector3d vel;
    if (!vessel_->LandedOrSplashed()) {
        const Orbit& orbit = vessel_->orbit();
        const CelestialBody* active = FlightGlobals::ActiveVessel != nullptr ? FlightGlobals::ActiveVessel->mainBody() : nullptr;
        const Vector3d rot = orbit.referenceBody->inverseRotation ? orbit.referenceBody->getRFrmVel(WorldFromPhysics(c.com)) : Vector3d();
        vel = orbit.GetVel(active) - rot - vessel_->FrameVelocity();
    }
    world_->SetLinearVelocity(c.body, vel);
    c.v = world_->GetLinearVelocity(c.body);
}

void VesselPhysics::ResumeVelocity() {
    for (ClusterRec& c : clusters_)
        if (c.body != kInvalidBody) ResumePartVelocity(*parts_[static_cast<size_t>(c.parts.front())]->part);
}

// ===================================================================================== forces

void VesselPhysics::AccumulateAt(int partIndex, const Vector3d& force, const Vector3d& point) {
    PartRec& r = *parts_[static_cast<size_t>(partIndex)];
    ClusterRec& c = clusters_[static_cast<size_t>(r.cluster)];
    if (c.body == kInvalidBody) return;
    c.force += force;
    c.moment += Cross(point - c.com, force);
    partForce_[static_cast<size_t>(partIndex)] += force;
    partMoment_[static_cast<size_t>(partIndex)] += Cross(point - PartComNow(partIndex), force);
}

void VesselPhysics::AddForceAtPhysicsPoint(Part& part, const Vector3d& force, const Vector3d& point) {
    const int o = OwnerIndex(part);
    if (o >= 0) AccumulateAt(o, force, point);
}

// Part.cs:8663 (RigidBodyPart.force += vec; FlightIntegrator.Integrate: rb.AddForce(part.force) at the body's centre of mass)
void VesselPhysics::AddForce(Part& part, const Vector3d& force) {
    const int o = OwnerIndex(part);
    if (o >= 0) AccumulateAt(o, force, PartComNow(o));
}

// Part.cs:8671
void VesselPhysics::AddImpulse(Part& part, const Vector3d& impulse) { AddForce(part, impulse / static_cast<double>(TimeWarp::fixedDeltaTime())); }

// Part.cs:8679
void VesselPhysics::AddForceAtPosition(Part& part, const Vector3d& force, const Vector3d& position) {
    AddForceAtPhysicsPoint(part, force, PhysicsFromTransform(position));
}

// Part.cs:8684
void VesselPhysics::AddTorque(Part& part, const Vector3d& torque) {
    const int o = OwnerIndex(part);
    if (o < 0) return;
    ClusterRec& c = clusters_[static_cast<size_t>(parts_[static_cast<size_t>(o)]->cluster)];
    if (c.body == kInvalidBody) return;
    c.moment += torque;
    partMoment_[static_cast<size_t>(o)] += torque;
}

namespace {
// Unity ForceMode -> the force over one tick of `dt`, for a body of `mass`
Vector3d ModeForce(const Vector3d& f, ForceMode mode, double mass, double dt) {
    switch (mode) {
        case ForceMode::Acceleration: return f * mass;
        case ForceMode::Impulse: return f / dt;
        case ForceMode::VelocityChange: return f * (mass / dt);
        case ForceMode::Force:
        default: return f;
    }
}
}  // namespace

void VesselPhysics::AddRigidbodyForce(Part& part, const Vector3d& force, ForceMode mode) {
    const int o = OwnerIndex(part);
    if (o < 0) return;
    const PartRec& r = *parts_[static_cast<size_t>(o)];
    AccumulateAt(o, ModeForce(force, mode, r.rbMass, TimeWarp::fixedDeltaTime()), PartComNow(o));
}

void VesselPhysics::AddRigidbodyForceAtPosition(Part& part, const Vector3d& force, const Vector3d& position, ForceMode mode) {
    const int o = OwnerIndex(part);
    if (o < 0) return;
    const PartRec& r = *parts_[static_cast<size_t>(o)];
    AccumulateAt(o, ModeForce(force, mode, r.rbMass, TimeWarp::fixedDeltaTime()), PhysicsFromTransform(position));
}

void VesselPhysics::SetAngularDrag(Part& part, float angularDrag) {
    const int i = PartIndex(part);
    if (i >= 0 && parts_[static_cast<size_t>(i)]->rb) parts_[static_cast<size_t>(i)]->angularDrag = angularDrag;
}

void VesselPhysics::AddUniformAcceleration(const Vector3d& acceleration) { uniformAccel_ += acceleration; }

// FlightIntegrator.cs:715 Integrate(part) (to :745), the force half (UpdateAerodynamics is WP-20d's), recursion from the root over the
// attached children in child order
void VesselPhysics::IntegratePartBuffers(const Vector3d& integrationAccel) {
    if (vessel_ == nullptr) return;
    UpdateMassProperties();
    bool any = false;
    for (const auto& r : parts_) any = any || r->rb;
    if (any) AddUniformAcceleration(integrationAccel);  // part.rb.AddForce(integrationAccel, ForceMode.Acceleration) on every rb
    std::vector<Part*> stack;
    if (vessel_->rootPart != nullptr) stack.push_back(vessel_->rootPart);
    while (!stack.empty()) {
        Part* part = stack.back();
        stack.pop_back();
        const int i = PartIndex(*part);
        if (i >= 0 && parts_[static_cast<size_t>(i)]->rb) {
            AccumulateAt(i, part->fields.force, PartComNow(i));  // rb.AddForce(part.force)
            ClusterRec& c = clusters_[static_cast<size_t>(parts_[static_cast<size_t>(i)]->cluster)];
            if (c.body != kInvalidBody) {  // rb.AddTorque(part.torque)
                c.moment += part->fields.torque;
                partMoment_[static_cast<size_t>(i)] += part->fields.torque;
            }
            int count = static_cast<int>(part->forces.size());
            while (count-- > 0)  // rb.AddForceAtPosition(forces[count].force, forces[count].pos)
                AccumulateAt(i, part->forces[static_cast<size_t>(count)].force, PhysicsFromTransform(part->forces[static_cast<size_t>(count)].pos));
        }
        part->forces.clear();
        part->fields.force = Vector3d();
        part->fields.torque = Vector3d();
        for (auto it = part->children.rbegin(); it != part->children.rend(); ++it)
            if ((*it)->isAttached()) stack.push_back(*it);
    }
}

size_t VesselPhysics::UpdateMassProperties() {
    size_t changed = 0;
    for (ClusterRec& c : clusters_) {
        if (c.body == kInvalidBody) continue;
        bool dirty = false;
        for (int i : c.parts) {
            PartRec& r = *parts_[static_cast<size_t>(i)];
            if (!r.rb) continue;
            const float m = PartRigidbodyMass(*r.part);
            if (m != r.rbMass) {
                r.rbMass = m;
                r.inertia = PartInertiaTensor(r.inertiaPerUnit, m, options_.inertiaFloor);
                r.ownBody = Mat3::RotatedDiagonal(Mat3::Rotation(r.rot0), Vector3d(r.inertia));
                dirty = true;
            }
        }
        if (!dirty) continue;
        ++changed;
        double mass = 0.0;
        Vector3d sum;
        for (auto it = c.parts.rbegin(); it != c.parts.rend(); ++it) {
            const PartRec& r = *parts_[static_cast<size_t>(*it)];
            if (!r.rb) continue;
            sum += r.localCom * static_cast<double>(r.rbMass);
            mass += static_cast<double>(r.rbMass);
        }
        if (!(mass > 0.0)) continue;
        const Vector3d comBody = sum * (1.0 / mass);
        Mat3 I;
        for (auto it = c.parts.rbegin(); it != c.parts.rend(); ++it) {
            const PartRec& r = *parts_[static_cast<size_t>(*it)];
            if (!r.rb) continue;
            I += r.ownBody;
            I += Mat3::ParallelAxis(static_cast<double>(r.rbMass), r.localCom - comBody);
        }
        double inertia[3][3];
        for (int a = 0; a < 3; ++a)
            for (int b = 0; b < 3; ++b) inertia[a][b] = I.m[a][b];
        const Vector3d shift = c.q * (comBody - c.comBody);  // physics frame
        world_->SetMassProperties(c.body, mass, inertia, comBody.ToVector3f());
        // the parts keep their velocities: the new centre of mass moves with v + w x shift
        world_->SetLinearVelocity(c.body, c.v + Cross(c.w, shift));
        const Vector3d oldCom = c.com;
        c.mass = mass;
        c.comBody = comBody;
        c.inertia = I;
        c.com = world_->GetCenterOfMassPosition(c.body);
        c.v = world_->GetLinearVelocity(c.body);
        c.moment += Cross(oldCom - c.com, c.force);  // keep the buffered moment about the (new) centre of mass
    }
    return changed;
}

// Pushes this tick's forces into the solver. Angular drag: PhysX damps each part body's angular velocity by
// max(0, 1 - dt * angularDrag) per step; for a rigid cluster the equivalent is the torque that removes the same angular momentum
// from each part's own spin: T = -(1/dt) sum_i (1 - max(0, 1 - dt d_i)) I_i w (I_i the part's own tensor; the orbital part
// m r x v of a part is not damped by angular drag in KSP either).
void VesselPhysics::Flush(float dt) {
    const double h = static_cast<double>(dt);
    // WP-20c: the step the load solve of AfterStep explains: the part forces of this tick (kept), the velocities before it
    const bool loads = options_.loadSolve != LoadSolve::None;
    if (loads) {
        stepForce_.swap(partForce_);
        stepMoment_.swap(partMoment_);
    }
    for (size_t ci = 0; ci < clusters_.size(); ++ci) {
        ClusterRec& c = clusters_[ci];
        if (c.body == kInvalidBody) {
            c.force = c.moment = Vector3d();
            continue;
        }
        if (loads) {
            clusterVBefore_[ci] = world_->GetLinearVelocity(c.body);
            clusterWBefore_[ci] = world_->GetAngularVelocity(c.body);
        }
        Vector3d force = c.force + uniformAccel_ * c.mass;
        Vector3d torque = c.moment;
        if (options_.angularDragModel == AngularDragModel::ClusterMassAverage) {
            double drag = 0.0;
            for (int i : c.parts) {
                const PartRec& r = *parts_[static_cast<size_t>(i)];
                if (r.rb) drag += static_cast<double>(r.angularDrag) * static_cast<double>(r.rbMass);
            }
            world_->SetAngularDamping(c.body, static_cast<float>(drag / c.mass));
        } else if (h > 0.0 && !c.w.IsZero()) {
            const Mat3 R = Mat3::Rotation(c.q);
            const Vector3d wb = Mat3::Rotation(QuaternionD::Inverse(c.q)) * c.w;
            Vector3d tb;
            for (int i : c.parts) {
                const PartRec& r = *parts_[static_cast<size_t>(i)];
                if (!r.rb || !(r.angularDrag > 0.0f)) continue;
                const double f = std::max(0.0, 1.0 - h * static_cast<double>(r.angularDrag));
                const Vector3d ti = (r.ownBody * wb) * (-(1.0 - f) / h);
                tb += ti;
                if (loads) stepMoment_[static_cast<size_t>(i)] += R * ti;  // the drag acts on each part's own body
            }
            torque += R * tb;
        }
        if (!force.IsZero()) world_->AddForce(c.body, force);
        if (!torque.IsZero()) world_->AddTorque(c.body, torque);
        c.force = c.moment = Vector3d();
    }
    if (loads) {
        stepAccel_ = uniformAccel_;
        stepDt_ = dt;
        stepValid_ = dt > 0.0f;
    }
    uniformAccel_ = Vector3d();
    std::fill(partForce_.begin(), partForce_.end(), Vector3d());
    std::fill(partMoment_.begin(), partMoment_.end(), Vector3d());
}

// After the world step: the cluster states and part transforms, then (WP-20c) the structural loads of the step and the sub-joints over
// capacity, then the autostrut countdown (SecureAutoStruts resumes after the fixed update, Part.cs:8067).
void VesselPhysics::AfterStep() {
    CacheClusterStates();
    SyncPartTransforms();
    breaks_.clear();
    lastSolve_ = LoadSolve::None;
    if (stepValid_ && options_.loadSolve != LoadSolve::None) ComputeLoads();
    stepValid_ = false;
    if (autoStrutCountdown_ > 0 && --autoStrutCountdown_ == 0) SecureAutoStruts();
}

// ===================================================================================== structural seams

size_t VesselPhysics::Cut(const std::vector<Part*>& children) {
    bool any = false;
    for (size_t j = 0; j < joints_.size(); ++j)
        if (!cut_[j] && joints_[j].kind == JointKind::Attach &&
            std::find(children.begin(), children.end(), joints_[j].child) != children.end()) {
            cut_[j] = true;
            any = true;
        }
    if (!any) return clusters_.size();
    // the tree pieces after the cut (attach joints and physicsless links); struts / autostruts between pieces are lost
    // (CompoundPart.onPartFixedUpdate DumpTarget, Part.CycleAutoStrut on onPartUndock / onVesselWasModified)
    const size_t n = parts_.size();
    std::vector<int> uf(n);
    std::iota(uf.begin(), uf.end(), 0);
    auto find = [&uf](int x) {
        while (uf[static_cast<size_t>(x)] != x) x = uf[static_cast<size_t>(x)] = uf[static_cast<size_t>(uf[static_cast<size_t>(x)])];
        return x;
    };
    for (size_t i = 0; i < n; ++i)
        if (!parts_[i]->rb && parts_[i]->owner >= 0) uf[static_cast<size_t>(find(static_cast<int>(i)))] = find(parts_[i]->owner);
    for (size_t j = 0; j < joints_.size(); ++j) {
        if (cut_[j] || joints_[j].kind != JointKind::Attach) continue;
        const int a = index_.Find(joints_[j].hostBody), b = index_.Find(joints_[j].targetBody);
        if (a >= 0 && b >= 0) uf[static_cast<size_t>(find(a))] = find(b);
    }
    for (size_t j = 0; j < joints_.size(); ++j) {
        if (cut_[j] || joints_[j].kind == JointKind::Attach) continue;
        const int a = index_.Find(joints_[j].hostBody), b = index_.Find(joints_[j].targetBody);
        if (a < 0 || b < 0 || find(a) != find(b)) cut_[j] = true;
    }
    const std::vector<PartPhysicsState> states = PartStates();
    DestroyBodies();
    BuildClusters(states);
    SyncPartTransforms();
    return clusters_.size();
}

void VesselPhysics::Rebuild() {
    std::unordered_map<const Part*, PartPhysicsState> byPart;
    for (size_t i = 0; i < parts_.size(); ++i) byPart[parts_[i]->part] = StateAt(static_cast<int>(i));
    // a part new to the binding: its vessel-frame pose relative to the root's current pose, at rest
    const auto rootIt = vessel_->rootPart != nullptr ? byPart.find(vessel_->rootPart) : byPart.end();
    std::vector<PartPhysicsState> states(vessel_->parts.size());
    for (size_t i = 0; i < vessel_->parts.size(); ++i) {
        const Part* p = vessel_->parts[i];
        const auto it = byPart.find(p);
        if (it != byPart.end()) {
            states[i] = it->second;
        } else if (rootIt != byPart.end()) {
            const PartPhysicsState& root = rootIt->second;
            const QuaternionD rootOrg = QuaternionD(vessel_->rootPart->fields.orgRot);
            const QuaternionD rot = root.rotation * QuaternionD::Inverse(rootOrg);  // vessel transform rotation
            const Vector3d origin = root.position - rot * Vector3d(vessel_->rootPart->fields.orgPos);
            PartTransform t;
            t.position = p->fields.orgPos;
            t.rotation = p->fields.orgRot;
            t.localScale = p->partTransform.localScale;
            states[i].position = origin + rot * Vector3d(t.position);
            states[i].rotation = rot * QuaternionD(t.rotation);
            states[i].centerOfMass = origin + rot * Vector3d(t.TransformPoint(p->fields.CoMOffset));
        }
    }
    DestroyBodies();
    Init(states, group_);
}

std::unique_ptr<VesselPhysics> VesselPhysics::SplitOff(Vessel& other) {
    std::unordered_map<const Part*, PartPhysicsState> byPart;
    for (size_t i = 0; i < parts_.size(); ++i) byPart[parts_[i]->part] = PartState(*parts_[i]->part);
    auto stateList = [&byPart](const Vessel& v) {
        std::vector<PartPhysicsState> s(v.parts.size());
        for (size_t i = 0; i < v.parts.size(); ++i) {
            const auto it = byPart.find(v.parts[i]);
            if (it != byPart.end()) s[i] = it->second;
        }
        return s;
    };
    auto split = std::make_unique<VesselPhysics>(*world_, other, stateList(other), options_);
    split->frameOriginWorld = frameOriginWorld;
    split->floatOrigin = floatOrigin;
    const std::vector<PartPhysicsState> mine = stateList(*vessel_);
    DestroyBodies();
    Init(mine, group_);
    return split;
}

std::unique_ptr<VesselPhysics> VesselPhysics::Merge(VesselPhysics& a, VesselPhysics& b, Vessel& merged) {
    std::unordered_map<const Part*, PartPhysicsState> byPart;
    for (VesselPhysics* vp : {&a, &b})
        for (size_t i = 0; i < vp->parts_.size(); ++i) byPart[vp->parts_[i]->part] = vp->PartState(*vp->parts_[i]->part);
    std::vector<PartPhysicsState> states(merged.parts.size());
    for (size_t i = 0; i < merged.parts.size(); ++i) {
        const auto it = byPart.find(merged.parts[i]);
        if (it != byPart.end()) states[i] = it->second;
    }
    a.DestroyBodies();
    b.DestroyBodies();
    auto out = std::make_unique<VesselPhysics>(*a.world_, merged, states, a.options_);
    out->frameOriginWorld = a.frameOriginWorld;
    out->floatOrigin = a.floatOrigin;
    out->SyncPartTransforms();
    return out;
}

}  // namespace ksp::physics

// ===================================================================================== Part.Start rules (ksp::physics free functions)

namespace ksp {

using physics::kFull;

// Part.cs:1663 Start, its flight-scene block: :1788-1834 (PhysicsSignificance -> physicalSignificance, the robotic parent promotion, the
// root is FULL) and :1868-1885 (a Rigidbody for FULL parts). Idempotent on the same tree.
void physics::StartPartRigidbodies(Vessel& vessel) {
    for (Part* p : vessel.parts) {
        if (p->fields.PhysicsSignificance != -1) {
            if (p->parent != nullptr && p->parent->isRobotic()) {  // HighLogic.LoadedSceneIsFlight && parent.isRobotic()
                p->PromoteToPhysicalPart();
                if (p->fields.prefabMass < 0.0065f) {
                    p->fields.prefabMass = 0.0065f;
                    p->fields.mass = 0.0065f;
                }
            } else {
                p->fields.PhysicsSignificance = Mathf::Max(0, p->fields.PhysicsSignificance);
                p->fields.physicalSignificance = p->fields.PhysicsSignificance;
                p->fields.previousPhysicalSignificance = p->fields.physicalSignificance;
            }
        }
        if (p->fields.physicalSignificance != kFull && p->vessel != nullptr && p->vessel->rootPart == p) {  // :1830
            p->fields.physicalSignificance = kFull;
            p->fields.previousPhysicalSignificance = p->fields.physicalSignificance;
        }
        p->hasRigidbody = p->fields.physicalSignificance == kFull;  // :1868-1872 rb = AddComponent<Rigidbody>()
    }
}

// FlightIntegrator.cs:452 UpdateMassStats, the rb.mass statements :476-489
float physics::PartRigidbodyMass(const Part& part) {
    if (!part.hasRigidbody) return 0.0f;
    const float num = static_cast<float>(part.fields.physicsMass);  // `float num` of UpdateMassStats, stored in physicsMass
    const float minimum = part.partInfo != nullptr ? part.partInfo->minimumRBMass : PhysicsGlobals::PartRBMassMin();
    return Mathf::Max(minimum, num);
}

// Part.cs:4263 ValidateInertiaTensor (:4265-4268)
Vector3f physics::PartInertiaTensor(const Vector3f& perUnitMass, float rbMass, bool floor) {
    if (perUnitMass.x > 1E-06f && perUnitMass.y > 1E-06f && perUnitMass.z > 1E-06f)
        return perUnitMass * (floor ? Mathf::Max(1.0f, rbMass) : rbMass);
    return perUnitMass * rbMass;  // no override: PhysX's tensor scales with the mass
}

// Part.cs:1184 RigidBodyPart
Part* physics::RigidBodyPart(Part* part) {
    if (part == nullptr) return nullptr;
    if (!part->hasRigidbody) return part->parent != nullptr ? RigidBodyPart(part->parent) : nullptr;
    return part;
}

}  // namespace ksp
