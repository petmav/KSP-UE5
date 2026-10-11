#include "reduced.hpp"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>

#include "ksp/physics/physics_globals.hpp"
#include "shapes.hpp"

namespace spike {

using namespace JPH;

// Marks the parts of this vessel that touch static geometry (terrain): the contact share of the load solve.
class ReducedVessel::SupportListener final : public ContactListener {
public:
    explicit SupportListener(size_t parts) : touching(parts) {}
    std::vector<std::atomic<uint8_t>> touching;
    void Clear() {
        for (auto& t : touching) t.store(0, std::memory_order_relaxed);
    }
    void Mark(const Body& b, const SubShapeID& id) {
        const uint64 part = b.GetShape()->GetSubShapeUserData(id);
        if (part < touching.size()) touching[part].store(1, std::memory_order_relaxed);
    }
    void OnContactAdded(const Body& b1, const Body& b2, const ContactManifold& m, ContactSettings&) override { Handle(b1, b2, m); }
    void OnContactPersisted(const Body& b1, const Body& b2, const ContactManifold& m, ContactSettings&) override { Handle(b1, b2, m); }

private:
    void Handle(const Body& b1, const Body& b2, const ContactManifold& m) {
        if (b1.IsStatic() && b2.IsDynamic()) Mark(b2, m.mSubShapeID2);
        else if (b2.IsStatic() && b1.IsDynamic()) Mark(b1, m.mSubShapeID1);
    }
};

namespace {

Mat44 PartInertiaWorld(Vec3Arg diag, QuatArg rot) {
    const Mat44 r = Mat44::sRotation(rot);
    return r * Mat44::sScale(diag) * r.Transposed3x3();
}

Mat44 Add3x3(const Mat44& a, const Mat44& b) {
    Mat44 m = Mat44::sZero();
    for (int c = 0; c < 3; c++) m.SetColumn3(c, a.GetColumn3(c) + b.GetColumn3(c));
    m.SetColumn4(3, Vec4(0, 0, 0, 1));
    return m;
}

Mat44 ParallelAxis(float mass, Vec3Arg d) {
    // m (|d|^2 E - d d^T)
    const float d2 = d.LengthSq();
    Mat44 m = Mat44::sZero();
    for (int c = 0; c < 3; c++) {
        Vec3 col = -d * d[c];
        col.SetComponent(c, col[c] + d2);
        m.SetColumn3(c, col * mass);
    }
    m.SetColumn4(3, Vec4(0, 0, 0, 1));
    return m;
}

}  // namespace

ReducedVessel::ReducedVessel(Clustering c) : clustering_(c) {}

ReducedVessel::~ReducedVessel() {
    if (world_ && listener_) world_->System().SetContactListener(nullptr);
}

void ReducedVessel::Build(World& world, const CraftModel& craft, const Placement& pl, unsigned vesselGroup) {
    world_ = &world;
    craft_ = &craft;
    group_ = vesselGroup;
    const size_t n = craft.parts.size();
    partPosNow_.resize(n);
    partRotNow_.resize(n);
    partMass_.assign(n, 0.0f);
    for (size_t i = 0; i < n; i++) {
        const PartSpec& s = craft.parts[i];
        partPosNow_[i] = pl.ToPhysics(s.pos);
        partRotNow_[i] = (pl.rotation * s.rot).Normalized();
        partMass_[i] = s.physical ? s.rbMass : 0.0f;
    }
    partExtF_.assign(n, Vec3::sZero());
    partExtM_.assign(n, Vec3::sZero());
    cut_.assign(craft.joints.size(), false);
    loads.assign(craft.attachJointCount, EdgeLoad{});
    anchorLocal_.assign(craft.joints.size(), Vec3::sZero());
    for (size_t j = 0; j < craft.joints.size(); j++) {
        const JointSpec& js = craft.joints[j];
        DVec3 a = DVec3::sZero();
        for (int q = 0; q < js.internalJoints; q++) a += js.anchors[q];
        a /= static_cast<double>(js.internalJoints);
        const PartSpec& cs = craft.parts[js.child];
        const DVec3 rel = a - cs.pos;
        anchorLocal_[j] = cs.rot.Conjugated() * Vec3(static_cast<float>(rel.GetX()), static_cast<float>(rel.GetY()), static_cast<float>(rel.GetZ()));
    }
    for (int j = 0; j < craft.attachJointCount; j++) {
        const JointSpec& js = craft.joints[j];
        loads[j].forceCap = js.breakForceSub * static_cast<float>(js.internalJoints);
        loads[j].torqueCap = js.breakTorqueSub * static_cast<float>(js.internalJoints) +
                             (js.internalJoints == 3 ? 1.5f * js.ringRadius * js.breakForceSub : 0.0f);
    }
    // post-order of the part tree (children before parents)
    postOrder_.clear();
    std::vector<std::pair<int, size_t>> stack{{craft.root, 0}};
    while (!stack.empty()) {
        auto& [p, k] = stack.back();
        const auto& ch = craft.parts[p].children;
        if (k < ch.size()) {
            const int c = ch[k++];
            stack.push_back({c, 0});
        } else {
            postOrder_.push_back(p);
            stack.pop_back();
        }
    }
    listener_ = std::make_unique<SupportListener>(n);
    world.System().SetContactListener(listener_.get());
    clusterOfPart_.assign(n, -1);
    partLocalPos_.assign(n, Vec3::sZero());
    partLocalRot_.assign(n, Quat::sIdentity());
    Recluster(cut_, false);
    partPos.assign(n, RVec3::sZero());
    partRot.assign(n, Quat::sIdentity());
    partVel.assign(n, Vec3::sZero());
}

void ReducedVessel::Recluster(const std::vector<bool>& cut, bool inheritVelocity) {
    const auto t0 = std::chrono::steady_clock::now();
    const CraftModel& craft = *craft_;
    const size_t n = craft.parts.size();
    // union-find over the welded attach joints (+ physicsless parts onto their parent)
    std::vector<int> uf(n);
    std::iota(uf.begin(), uf.end(), 0);
    auto find = [&](int x) {
        while (uf[x] != x) x = uf[x] = uf[uf[x]];
        return x;
    };
    auto unite = [&](int a, int b) { uf[find(a)] = find(b); };
    for (size_t i = 0; i < n; i++)
        if (!craft.parts[i].physical && craft.parts[i].parent >= 0) unite(static_cast<int>(i), craft.parts[i].parent);
    for (int j = 0; j < craft.attachJointCount; j++) {
        if (cut[j]) continue;
        const JointSpec& js = craft.joints[j];
        if (clustering_ == Clustering::SplitAtDecouplers && craft.parts[js.child].isDecoupler) continue;
        unite(js.child, js.parent);
    }
    // old state for velocity inheritance
    struct Old {
        Vec3 v, w;
        RVec3 com;
    };
    std::vector<Old> old(clusters_.size());
    for (size_t c = 0; c < clusters_.size(); c++)
        if (clusters_[c].body) old[c] = {clusters_[c].body->GetLinearVelocity(), clusters_[c].body->GetAngularVelocity(),
                                        clusters_[c].body->GetCenterOfMassPosition()};
    std::vector<int> oldClusterOfPart = clusterOfPart_;
    // remove the old bodies
    BodyInterface& bi = world_->Bodies();
    for (auto& ij : interJoints_) world_->System().RemoveConstraint(ij);
    interJoints_.clear();
    interJointOf_.clear();
    for (Cluster& c : clusters_)
        if (c.body) {
            bi.RemoveBody(c.body->GetID());
            bi.DestroyBody(c.body->GetID());
            c.body = nullptr;
        }
    clusters_.clear();
    // new clusters in part order
    std::vector<int> rootToCluster(n, -1);
    for (size_t i = 0; i < n; i++) {
        const int r = find(static_cast<int>(i));
        if (rootToCluster[r] < 0) {
            rootToCluster[r] = static_cast<int>(clusters_.size());
            clusters_.emplace_back();
        }
        clusterOfPart_[i] = rootToCluster[r];
        clusters_[rootToCluster[r]].parts.push_back(static_cast<int>(i));
    }
    for (Cluster& c : clusters_) {
        Vec3 v = Vec3::sZero(), w = Vec3::sZero();
        RVec3 oc = RVec3::sZero();
        bool has = false;
        if (inheritVelocity && !oldClusterOfPart.empty() && oldClusterOfPart[c.parts.front()] >= 0) {
            const Old& o = old[oldClusterOfPart[c.parts.front()]];
            v = o.v;
            w = o.w;
            oc = o.com;
            has = true;
        }
        BuildClusterBody(c, RVec3::sZero(), v, w, oc, has);
    }
    RebuildInterClusterJoints();
    lastSplitMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

void ReducedVessel::BuildClusterBody(Cluster& c, RVec3Arg, Vec3Arg v, Vec3Arg w, RVec3Arg oldCom, bool hasOld) {
    const CraftModel& craft = *craft_;
    // mass, CoM (double), inertia about the CoM in the physics frame (the body starts with identity rotation)
    double m = 0.0;
    DVec3 comSum = DVec3::sZero();
    for (int p : c.parts) {
        if (!craft.parts[p].physical) continue;
        const RVec3 pc = partPosNow_[p] + partRotNow_[p] * craft.parts[p].comLocal;
        comSum += DVec3(pc) * static_cast<double>(partMass_[p]);
        m += partMass_[p];
    }
    const DVec3 comD = comSum / m;
    const RVec3 com(static_cast<Real>(comD.GetX()), static_cast<Real>(comD.GetY()), static_cast<Real>(comD.GetZ()));
    Mat44 inertia = Mat44::sZero();
    inertia.SetColumn4(3, Vec4(0, 0, 0, 1));
    StaticCompoundShapeSettings compound;
    for (int p : c.parts) {
        const PartSpec& s = craft.parts[p];
        partLocalPos_[p] = Vec3(partPosNow_[p] - com);
        partLocalRot_[p] = partRotNow_[p];
        if (!s.physical) continue;
        const float pm = partMass_[p];
        const Vec3 diag = s.inertiaPerMass * (inertiaFloor ? std::max(1.0f, pm) : pm);
        const Vec3 d = partLocalPos_[p] + partLocalRot_[p] * s.comLocal;
        inertia = Add3x3(inertia, Add3x3(PartInertiaWorld(diag, partLocalRot_[p]), ParallelAxis(pm, d)));
        Ref<Shape> box = PartBoxShape(s, static_cast<uint64>(p));
        compound.AddShape(partLocalPos_[p], partLocalRot_[p], box, static_cast<uint32>(p));
    }
    auto cres = compound.Create();
    if (cres.HasError()) throw std::runtime_error(cres.GetError().c_str());
    Ref<Shape> cs = cres.Get();
    OffsetCenterOfMassShapeSettings oc(-cs->GetCenterOfMass(), cs);
    BodyCreationSettings b(oc.Create().Get(), com, Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
    KspBodyDefaults(b, ksp::PhysicsGlobals::MaxAngularVelocity());
    b.mMassPropertiesOverride.mMass = static_cast<float>(m);
    b.mMassPropertiesOverride.mInertia = inertia;
    b.mCollisionGroup = CollisionGroup(world_->VesselFilter(), group_, 0);
    Vec3 vNew = v;
    if (hasOld) vNew = v + w.Cross(Vec3(com - oldCom));
    b.mLinearVelocity = vNew;
    b.mAngularVelocity = w;
    BodyInterface& bi = world_->Bodies();
    c.body = bi.CreateBody(b);
    bi.AddBody(c.body->GetID(), EActivation::Activate);
    c.mass = m;
    c.inertiaBody = inertia;
}

void ReducedVessel::RebuildInterClusterJoints() {
    const CraftModel& craft = *craft_;
    for (int j = 0; j < craft.attachJointCount; j++) {
        if (cut_[j]) continue;
        const JointSpec& js = craft.joints[j];
        const int ca = clusterOfPart_[js.child], cb = clusterOfPart_[js.parent];
        if (ca == cb) continue;
        // anchor: the child's attach point now (craft-frame anchors mapped through the child's current pose)
        const PartSpec& cs = craft.parts[js.child];
        DVec3 a = DVec3::sZero();
        for (int k = 0; k < js.internalJoints; k++) a += js.anchors[k];
        a /= static_cast<double>(js.internalJoints);
        const DVec3 rel = a - cs.pos;
        const Vec3 local = cs.rot.Conjugated() * Vec3(static_cast<float>(rel.GetX()), static_cast<float>(rel.GetY()), static_cast<float>(rel.GetZ()));
        const RVec3 anchor = partPosNow_[js.child] + partRotNow_[js.child] * local;
        FixedConstraintSettings s;
        s.mSpace = EConstraintSpace::WorldSpace;
        s.mAutoDetectPoint = false;
        s.mPoint1 = s.mPoint2 = anchor;
        TwoBodyConstraint* c = static_cast<TwoBodyConstraint*>(s.Create(*clusters_[ca].body, *clusters_[cb].body));
        world_->System().AddConstraint(c);
        interJoints_.push_back(c);
        interJointOf_.push_back(j);
    }
}

void ReducedVessel::ReadState() {
    const size_t n = craft_->parts.size();
    DVec3 comSum = DVec3::sZero(), velSum = DVec3::sZero();
    double m = 0.0;
    for (const Cluster& c : clusters_) {
        comSum += DVec3(c.body->GetCenterOfMassPosition()) * c.mass;
        velSum += DVec3(c.body->GetLinearVelocity()) * c.mass;
        m += c.mass;
    }
    mass = m;
    const DVec3 cd = comSum / m;
    com = RVec3(static_cast<Real>(cd.GetX()), static_cast<Real>(cd.GetY()), static_cast<Real>(cd.GetZ()));
    comVel = (velSum / m).ToVec3RoundDown();
    for (size_t i = 0; i < n; i++) {
        const Body* b = clusters_[clusterOfPart_[i]].body;
        const Quat r = b->GetRotation();
        partPos[i] = b->GetPosition() + r * partLocalPos_[i];
        partRot[i] = r * partLocalRot_[i];
        partVel[i] = b->GetPointVelocity(partPos[i] + partRot[i] * craft_->parts[i].comLocal);
    }
    partPosNow_ = partPos;
    partRotNow_ = partRot;
}

void ReducedVessel::AddForceAtPosition(int part, Vec3Arg f, RVec3Arg point) {
    Cluster& c = clusters_[clusterOfPart_[part]];
    c.force += f;
    c.torque += Vec3(point - c.body->GetCenterOfMassPosition()).Cross(f);
    partExtF_[part] += f;
    partExtM_[part] += Vec3(point - (partPos[part] + partRot[part] * craft_->parts[part].comLocal)).Cross(f);
}

void ReducedVessel::AddUniformAcceleration(Vec3Arg a) { uniformAccel_ += a; }

void ReducedVessel::SetAngularDrag(int part, float drag) {
    // a cluster takes the mass-weighted angular drag of its parts (rb.angularDrag is per part body in KSP)
    Cluster& c = clusters_[clusterOfPart_[part]];
    c.angularDrag += drag * partMass_[part] / static_cast<float>(c.mass);
}

void ReducedVessel::UpdateMass(const std::vector<float>& rbMass) {
    // mass drain: re-sum mass and inertia of each cluster (CoM shift ignored by the spike: the body's CoM stays put)
    for (size_t i = 0; i < rbMass.size(); i++)
        if (craft_->parts[i].physical) partMass_[i] = rbMass[i];
    for (Cluster& c : clusters_) {
        double m = 0.0;
        Mat44 inertia = Mat44::sZero();
        inertia.SetColumn4(3, Vec4(0, 0, 0, 1));
        for (int p : c.parts) {
            const PartSpec& s = craft_->parts[p];
            if (!s.physical) continue;
            const float pm = partMass_[p];
            m += pm;
            const Vec3 diag = s.inertiaPerMass * (inertiaFloor ? std::max(1.0f, pm) : pm);
            const Vec3 d = partLocalPos_[p] + partLocalRot_[p] * s.comLocal;
            inertia = Add3x3(inertia, Add3x3(PartInertiaWorld(diag, partLocalRot_[p]), ParallelAxis(pm, d)));
        }
        c.mass = m;
        c.inertiaBody = inertia;
        MotionProperties* mp = c.body->GetMotionProperties();
        MassProperties props;
        props.mMass = static_cast<float>(m);
        props.mInertia = inertia;
        mp->SetMassProperties(EAllowedDOFs::All, props);
    }
}

void ReducedVessel::Flush() {
    for (Cluster& c : clusters_) {
        const float m = static_cast<float>(c.mass);
        c.body->AddForce(c.force + uniformAccel_ * m);
        c.body->AddTorque(c.torque);
        c.body->GetMotionProperties()->SetAngularDamping(c.angularDrag);
        c.vBefore = c.body->GetLinearVelocity();
        c.wBefore = c.body->GetAngularVelocity();
        c.force = Vec3::sZero();
        c.torque = Vec3::sZero();
        c.angularDrag = 0.0f;
    }
    lastUniformAccel_ = uniformAccel_;
    uniformAccel_ = Vec3::sZero();
    if (listener_) listener_->Clear();
}

void ReducedVessel::ComputeLoads(float dt) {
    const CraftModel& craft = *craft_;
    const size_t n = craft.parts.size();
    const float inv = 1.0f / dt;
    std::vector<RVec3> pcom(n);
    for (size_t i = 0; i < n; i++) pcom[i] = partPos[i] + partRot[i] * craft.parts[i].comLocal;
    // ---- per cluster: rigid-body kinematics over the step and the unexplained residual (contacts), force AND moment
    struct Kin {
        Vec3 a, alpha, w;
        RVec3 com;
        Vec3 resF, resM;  // residual force / moment about the cluster CoM: contacts (and solver effects such as angular damping)
        int supports;
    };
    std::vector<Kin> kin(clusters_.size());
    for (size_t c = 0; c < clusters_.size(); c++) {
        const Cluster& cl = clusters_[c];
        Kin& k = kin[c];
        k.w = cl.body->GetAngularVelocity();
        k.a = (cl.body->GetLinearVelocity() - cl.vBefore) * inv;
        k.alpha = (k.w - cl.wBefore) * inv;
        k.com = cl.body->GetCenterOfMassPosition();
        const Mat44 R = Mat44::sRotation(cl.body->GetRotation());
        const Mat44 Iw = R * cl.inertiaBody * R.Transposed3x3();
        const Vec3 H = Iw.Multiply3x3(k.alpha) + k.w.Cross(Iw.Multiply3x3(k.w));
        Vec3 F = lastUniformAccel_ * static_cast<float>(cl.mass), M = Vec3::sZero();
        k.supports = 0;
        for (int p : cl.parts) {
            F += partExtF_[p];
            M += Vec3(pcom[p] - k.com).Cross(partExtF_[p]) + partExtM_[p];
            k.supports += listener_->touching[p].load(std::memory_order_relaxed) ? 1 : 0;
        }
        k.resF = k.a * static_cast<float>(cl.mass) - F;
        k.resM = H - M;
    }
    // inter-cluster joints are tree edges, not contacts: take their force and torque out of the residuals
    // (Jolt applies the constraint impulse +lambda to body 2 and -lambda to body 1)
    for (size_t i = 0; i < interJoints_.size(); i++) {
        if (!interJoints_[i]->GetEnabled()) continue;
        const auto* fc = static_cast<const FixedConstraint*>(interJoints_[i].GetPtr());
        const Vec3 f = fc->GetTotalLambdaPosition() * inv;
        const Vec3 t = fc->GetTotalLambdaRotation() * inv;
        const Body* b1 = fc->GetBody1();
        const RVec3 p = b1->GetCenterOfMassTransform() * fc->GetConstraintToBody1Matrix().GetTranslation();
        const int j = interJointOf_[i];
        Kin& k1 = kin[clusterOfPart_[craft.joints[j].child]];   // body 1
        Kin& k2 = kin[clusterOfPart_[craft.joints[j].parent]];  // body 2
        k1.resF += f;
        k1.resM += Vec3(p - k1.com).Cross(f) + t;
        k2.resF -= f;
        k2.resM -= Vec3(p - k2.com).Cross(f) + t;
    }
    // contact share per part: equal force over the touching parts plus the couple that makes the moment balance exact
    // (no touching part: the residual is spread over every part by mass, i.e. treated as a body force)
    std::vector<Vec3> cf(n, Vec3::sZero()), cm(n, Vec3::sZero());
    for (size_t c = 0; c < clusters_.size(); c++) {
        const Cluster& cl = clusters_[c];
        const Kin& k = kin[c];
        if (k.supports > 0) {
            const float share = 1.0f / static_cast<float>(k.supports);
            Vec3 mf = Vec3::sZero();
            for (int p : cl.parts)
                if (listener_->touching[p].load(std::memory_order_relaxed)) {
                    cf[p] = k.resF * share;
                    mf += Vec3(pcom[p] - k.com).Cross(cf[p]);
                }
            const Vec3 couple = (k.resM - mf) * share;
            for (int p : cl.parts)
                if (listener_->touching[p].load(std::memory_order_relaxed)) cm[p] = couple;
        } else {
            const float invM = 1.0f / static_cast<float>(cl.mass);
            Vec3 mf = Vec3::sZero();
            for (int p : cl.parts) {
                cf[p] = k.resF * (partMass_[p] * invM);
                mf += Vec3(pcom[p] - k.com).Cross(cf[p]);
            }
            const Vec3 couple = (k.resM - mf) * (1.0f / static_cast<float>(cl.parts.size()));
            for (int p : cl.parts) cm[p] = couple;
        }
    }
    // ---- demand per part, accumulated children-first; moments about the vessel CoM
    std::vector<Vec3> sf(n, Vec3::sZero()), sm(n, Vec3::sZero());
    const RVec3 origin = com;
    maxLoadRatio = 0.0f;
    maxLoadJoint = -1;
    for (int p : postOrder_) {
        const PartSpec& s = craft.parts[p];
        const Kin& k = kin[clusterOfPart_[p]];
        const float m = partMass_[p];
        const Vec3 r = Vec3(pcom[p] - k.com);
        Vec3 d = -partExtF_[p] - cf[p];
        Vec3 rot = -partExtM_[p] - cm[p];
        if (s.physical) {
            const Vec3 acc = k.a + k.alpha.Cross(r) + k.w.Cross(k.w.Cross(r)) - lastUniformAccel_;
            d += acc * m;
            const Mat44 I = PartInertiaWorld(s.inertiaPerMass * (inertiaFloor ? std::max(1.0f, m) : m), partRot[p]);
            rot += I.Multiply3x3(k.alpha) + k.w.Cross(I.Multiply3x3(k.w));
        }
        sf[p] += d;
        sm[p] += Vec3(pcom[p] - origin).Cross(d) + rot;
        if (s.parent >= 0) {
            sf[s.parent] += sf[p];
            sm[s.parent] += sm[p];
        }
        // the attach joint of p carries the subtree of p
        const int j = s.attachJoint;
        if (j >= 0 && j < craft.attachJointCount && !cut_[j]) {
            const Vec3 P = Vec3(partPos[p] + partRot[p] * anchorLocal_[j] - origin);
            EdgeLoad& L = loads[j];
            L.force = sf[p].Length();
            L.torque = (sm[p] - P.Cross(sf[p])).Length();
            const float ratio = std::max(L.force / L.forceCap, L.torque / L.torqueCap);
            if (ratio > maxLoadRatio) {
                maxLoadRatio = ratio;
                maxLoadJoint = j;
            }
        }
    }
}

std::vector<BreakEvent> ReducedVessel::AfterStep(float dt, int tick) {
    std::vector<BreakEvent> out;
    if (stressEnabled) {
        ReadState();
        ComputeLoads(dt);
        if (breakingEnabled && breakAllowed) {
            std::vector<bool> cut = cut_;
            for (int j = 0; j < craft_->attachJointCount; j++) {
                if (cut_[j]) continue;
                const EdgeLoad& L = loads[j];
                if (L.force > L.forceCap || L.torque > L.torqueCap) {
                    cut[j] = true;
                    out.push_back({j, tick, L.force / craft_->joints[j].internalJoints, L.torque / craft_->joints[j].internalJoints});
                }
            }
            if (!out.empty()) {
                cut_ = cut;
                Recluster(cut_, true);
                ReadState();
            }
        }
    }
    for (size_t i = 0; i < partExtF_.size(); i++) {
        partExtF_[i] = Vec3::sZero();
        partExtM_[i] = Vec3::sZero();
    }
    return out;
}

void ReducedVessel::ShiftOrigin(Vec3Arg delta) {
    BodyInterface& bi = world_->Bodies();
    for (Cluster& c : clusters_) bi.SetPosition(c.body->GetID(), c.body->GetPosition() + delta, EActivation::DontActivate);
}

int ReducedVessel::FireDecouplers() {
    bool any = false;
    for (int j = 0; j < craft_->attachJointCount; j++)
        if (!cut_[j] && craft_->parts[craft_->joints[j].child].isDecoupler) {
            cut_[j] = true;
            any = true;
        }
    if (!any) return BodyCount();
    if (clustering_ == Clustering::SplitAtDecouplers) {
        // the pieces are bodies already: drop the inter-cluster constraints of the fired joints only
        const auto t0 = std::chrono::steady_clock::now();
        for (size_t i = 0; i < interJoints_.size(); i++)
            if (cut_[interJointOf_[i]] && interJoints_[i]->GetEnabled()) {
                world_->System().RemoveConstraint(interJoints_[i]);
                interJoints_[i]->SetEnabled(false);
            }
        lastSplitMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    } else {
        ReadState();
        Recluster(cut_, true);
    }
    ReadState();
    return BodyCount();
}

int ReducedVessel::BodyCount() const { return static_cast<int>(clusters_.size()); }

int ReducedVessel::ConstraintCount() const {
    int c = 0;
    for (const auto& ij : interJoints_) c += ij->GetEnabled() ? 1 : 0;
    return c;
}

}  // namespace spike
