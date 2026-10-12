// WP-20c: the structural layer of VesselPhysics (see ksp/physics/vessel_physics.hpp and ksp/physics/structure_graph.hpp): autostrut
// cycling, the load model of a binding, the per-step load solve and the capacity check, and Rebind. Engine glue in double; the KSP rules
// it applies (PartJoint capacities, OnJointBreak, CycleAutoStrut timing) carry their C# citations.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <unordered_map>

#include "ksp/core/log.hpp"
#include "ksp/physics/vessel_physics.hpp"
#include "vessel_physics_internal.hpp"

namespace ksp::physics {

using detail::Cross;
using detail::Mat3;

namespace {
double MsSince(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}
uint8_t FullMask(const PartJointSpec& j) { return static_cast<uint8_t>((1u << j.internalJoints) - 1u); }
void ToArray(const Mat3& m, double out[3][3]) {
    for (int a = 0; a < 3; ++a)
        for (int b = 0; b < 3; ++b) out[a][b] = m.m[a][b];
}
}  // namespace

// ===================================================================================== autostruts

// Part.cs:8065 SecureAutoStruts for every part of the vessel (one round): the anchors from the current tree and rb.masses, the cache of
// GetMassivePart keyed by the physics step count when the timing is KSP's (a delay of at least one step)
void VesselPhysics::SecureAutoStruts() {
    autoStrutCountdown_ = 0;
    ReleaseAutoStruts();
    if (vessel_ == nullptr) return;
    const double fixedTime = static_cast<double>(world_->StepCount());
    const std::vector<PartJointSpec> autos =
        BuildAutoStruts(*vessel_, options_.jointGlobals, VesselFrameTransforms(), options_.autoStrutDelaySteps > 0 ? &fixedTime : nullptr);
    for (const PartJointSpec& j : autos) {
        if (index_.Find(j.hostBody) < 0 || index_.Find(j.targetBody) < 0) continue;  // both ends in this binding
        joints_.push_back(j);
        cut_.push_back(false);
        subAlive_.push_back(FullMask(j));
    }
    structureDirty_ = true;
    if (!autos.empty()) RebuildWelds();
}

// Part.cs:8217 ReleaseAutoStruts (every part's autostrut PartJoints destroyed)
void VesselPhysics::ReleaseAutoStruts() {
    if (joints_.size() == autoStrutBegin_) return;
    joints_.resize(autoStrutBegin_);
    cut_.resize(autoStrutBegin_);
    subAlive_.resize(autoStrutBegin_);
    structureDirty_ = true;
    RebuildWelds();
}

// Part.cs:8043 CycleAutoStrut: release now, secure after WaitForFixedUpdate; a cycle already waiting is not restarted (autoStrutCycling)
void VesselPhysics::CycleAutoStruts() {
    if (autoStrutCountdown_ > 0) return;
    ReleaseAutoStruts();
    if (options_.autoStrutDelaySteps <= 0) SecureAutoStruts();
    else autoStrutCountdown_ = options_.autoStrutDelaySteps;
}

size_t VesselPhysics::AutoStrutJointCount() const { return joints_.size() - autoStrutBegin_; }

// PhysX removed the broken ConfigurableJoints (Unity destroys a Joint after OnJointBreak); a PartJoint left without any is no joint
void VesselPhysics::DestroySubJoints(size_t joint, uint8_t mask) {
    if (joint >= joints_.size()) return;
    subAlive_[joint] = static_cast<uint8_t>(subAlive_[joint] & ~mask);
    structureDirty_ = true;
    if (subAlive_[joint] != 0 || cut_[joint]) return;
    if (joints_[joint].kind == JointKind::Attach) {
        Cut({joints_[joint].child});
    } else {
        cut_[joint] = true;
        RebuildWelds();
    }
}

// ===================================================================================== the load model

// the factor frame: the clusters' creation frame (body axes = physics axes then), positions relative to the first body's origin there
Vector3d VesselPhysics::FactorFrameOrigin() const {
    for (const ClusterRec& c : clusters_)
        if (c.body != kInvalidBody) return c.origin0;
    return {};
}

// The nodes (parts with a Rigidbody in a cluster with a body), every live sub-joint as an element at its anchor (the child part's
// reference pose, factor frame), the tree edges of the attach joints with their JointSplit, and for the graph solve the ground supports
// and the factorisation.
void VesselPhysics::BuildLoadModel(const std::vector<int>& supports) {
    const auto t0 = std::chrono::steady_clock::now();
    const size_t n = parts_.size();
    const Vector3d O = FactorFrameOrigin();
    nodeOfPart_.assign(n, -1);
    partOfNode_.clear();
    nodeCom0_.clear();
    std::vector<double> nodeMass;
    for (size_t i = 0; i < n; ++i) {
        const PartRec& r = *parts_[i];
        if (!r.rb || clusters_[static_cast<size_t>(r.cluster)].body == kInvalidBody) continue;
        nodeOfPart_[i] = static_cast<int>(partOfNode_.size());
        partOfNode_.push_back(static_cast<int>(i));
        nodeCom0_.push_back(r.com0 - O);
        nodeMass.push_back(static_cast<double>(r.rbMass));
    }
    for (size_t i = 0; i < n; ++i)
        if (!parts_[i]->rb && parts_[i]->owner >= 0) nodeOfPart_[i] = nodeOfPart_[static_cast<size_t>(parts_[i]->owner)];
    const size_t nn = partOfNode_.size();
    const size_t J = joints_.size();
    centroid0_.assign(J, Vector3d());
    anchor0_.assign(J, std::array<Vector3d, 3>{});
    splits_.assign(J, JointSplit{});
    splitSub_.assign(J, std::array<int, 3>{-1, -1, -1});
    treeParentNode_.assign(nn, -1);
    treeParentJoint_.assign(nn, -1);
    elements_.clear();
    const double kl = options_.kLinear;
    for (size_t j = 0; j < J; ++j) {
        const PartJointSpec& js = joints_[j];
        if (cut_[j] || subAlive_[j] == 0) continue;
        const int ia = index_.Find(js.targetBody), ib = index_.Find(js.hostBody), ic = index_.Find(js.child);
        if (ia < 0 || ib < 0 || ic < 0) continue;
        const int a = nodeOfPart_[static_cast<size_t>(ia)], b = nodeOfPart_[static_cast<size_t>(ib)];
        if (a < 0 || b < 0 || a == b) continue;
        // PartJoint.SetJointLimits (PartJoint.cs:359): the angular drives are locked (+inf) when rigid || forceRigid
        const double ka = (js.rigid || js.forceRigid) ? kl : static_cast<double>(js.stiffness);
        const PartRec& cr = *parts_[static_cast<size_t>(ic)];
        std::vector<Vector3d> anchors;
        std::vector<int> subs;
        for (int s = 0; s < js.internalJoints; ++s) {
            anchor0_[j][static_cast<size_t>(s)] = cr.pos0 + cr.rot0 * Vector3d(js.anchors[static_cast<size_t>(s)]) - O;
            if (!(subAlive_[j] & (1u << s))) continue;
            anchors.push_back(anchor0_[j][static_cast<size_t>(s)]);
            subs.push_back(s);
        }
        Vector3d c;
        for (const Vector3d& p : anchors) c += p;
        c = c * (1.0 / static_cast<double>(anchors.size()));
        centroid0_[j] = c;
        std::vector<Vector3d> rel;
        for (size_t k = 0; k < anchors.size(); ++k) {
            StructElement e;
            e.a = a;
            e.b = b;
            e.anchor = anchors[k];
            e.kLinear = kl;
            e.kAngular = ka;
            e.joint = static_cast<int>(j);
            e.sub = subs[k];
            elements_.push_back(e);
            rel.push_back(anchors[k] - c);
            splitSub_[j][k] = subs[k];
        }
        if (js.kind == JointKind::Attach && treeParentNode_[static_cast<size_t>(b)] < 0) {
            treeParentNode_[static_cast<size_t>(b)] = a;
            treeParentJoint_[static_cast<size_t>(b)] = static_cast<int>(j);
            splits_[j].Setup(rel, std::vector<double>(rel.size(), kl), std::vector<double>(rel.size(), ka));
        }
    }
    modelSupports_ = supports;
    structureDirty_ = false;
    if (modelGraph_) {
        for (int s : supports) {
            const int node = nodeOfPart_[static_cast<size_t>(s)];
            if (node < 0) continue;
            StructElement e;
            e.a = StructElement::kGround;
            e.b = node;
            e.anchor = nodeCom0_[static_cast<size_t>(node)];
            e.kLinear = kl;
            e.kAngular = options_.kSupportAngular;
            elements_.push_back(e);
        }
        if (!graph_.Build(nodeCom0_, nodeMass, elements_)) {
            log::Warning("VesselPhysics: the structural graph is not positive definite; the tree solve is used");
            modelGraph_ = false;
        }
        ++factorCount_;
        lastFactorMs_ = MsSince(t0);
    }
}

// The loads of the last step and the capacity check (PartJoint.cs:340-341: each ConfigurableJoint breaks on its own force / torque).
void VesselPhysics::ComputeLoads() {
    const auto t0 = std::chrono::steady_clock::now();
    maxRatio_ = 0.0f;
    maxRatioJoint_ = -1;
    const double dt = static_cast<double>(stepDt_);
    const double inv = 1.0 / dt;
    const size_t n = parts_.size();
    // ---- the ground supports: parts (their RigidBodyPart) whose colliders touched static geometry during the step
    std::vector<int> supports;
    for (const PhysicsWorld::StaticContact& sc : world_->StaticContacts()) {
        int cl = -1;
        for (size_t c = 0; c < clusters_.size(); ++c)
            if (clusters_[c].body == sc.body) cl = static_cast<int>(c);
        if (cl < 0 || sc.subShapeUserData >= n) continue;
        const PartRec& r = *parts_[sc.subShapeUserData];
        if (r.cluster != cl) continue;
        const int owner = r.rb ? static_cast<int>(sc.subShapeUserData) : r.owner;
        if (owner >= 0) supports.push_back(owner);
    }
    std::sort(supports.begin(), supports.end());
    supports.erase(std::unique(supports.begin(), supports.end()), supports.end());
    supportParts_ = supports;
    // ---- which solve
    bool graph = options_.loadSolve == LoadSolve::Graph;
    if (options_.loadSolve == LoadSolve::Auto) {
        graph = supports.size() >= 2;
        for (size_t j = 0; j < joints_.size() && !graph; ++j) {
            const PartJointSpec& js = joints_[j];
            if (js.kind == JointKind::Attach || cut_[j] || subAlive_[j] == 0) continue;
            const int ia = index_.Find(js.targetBody), ib = index_.Find(js.hostBody);
            if (ia >= 0 && ib >= 0 && parts_[static_cast<size_t>(ia)]->rb && parts_[static_cast<size_t>(ib)]->rb && ia != ib) graph = true;
        }
    }
    if (structureDirty_ || graph != modelWantGraph_ || (graph && supports != modelSupports_)) {
        modelWantGraph_ = graph;
        modelGraph_ = graph;
        BuildLoadModel(supports);
    }
    graph = modelGraph_;
    // ---- cluster kinematics over the step and the residual (what neither the part forces nor the welds explain: contacts, the
    // gyroscopic term the solver omits, clamps)
    const size_t C = clusters_.size();
    std::vector<Vector3d> ca(C), calpha(C), resF(C), resM(C);
    for (size_t c = 0; c < C; ++c) {
        const ClusterRec& cl = clusters_[c];
        if (cl.body == kInvalidBody) continue;
        ca[c] = (cl.v - clusterVBefore_[c]) * inv;
        calpha[c] = (cl.w - clusterWBefore_[c]) * inv;
        const Mat3 Ic = cl.inertia.Rotated(Mat3::Rotation(cl.q));
        Vector3d F = stepAccel_ * cl.mass, M;
        for (int i : cl.parts) {
            const PartRec& r = *parts_[static_cast<size_t>(i)];
            F += stepForce_[static_cast<size_t>(i)];
            M += Cross(r.comNow - cl.com, stepForce_[static_cast<size_t>(i)]) + stepMoment_[static_cast<size_t>(i)];
        }
        resF[c] = ca[c] * cl.mass - F;
        resM[c] = Ic * calpha[c] + Cross(cl.w, Ic * cl.w) - M;
    }
    for (const Weld& w : welds_) {  // the weld impulse applies to b, its opposite to a (PhysicsWorld::GetConstraintImpulse)
        Vector3d lin, ang;
        world_->GetConstraintImpulse(w.id, lin, ang);
        const Vector3d f = lin * inv, t = ang * inv;
        const ClusterRec& A = clusters_[static_cast<size_t>(w.a)];
        const ClusterRec& B = clusters_[static_cast<size_t>(w.b)];
        const Vector3d p = A.origin + A.q * w.pointBodyA;
        resF[static_cast<size_t>(w.b)] -= f;
        resM[static_cast<size_t>(w.b)] -= Cross(p - B.com, f) + t;
        resF[static_cast<size_t>(w.a)] += f;
        resM[static_cast<size_t>(w.a)] += Cross(p - A.com, f) + t;
    }
    // ---- node demands (physics frame), the residual shared out: in the graph solve the ground supports carry it where the cluster
    // touches static geometry; otherwise the supporting parts take equal shares and a common couple (tree solve), or every part its mass
    // share and a common couple (no support: a body force)
    const size_t nn = partOfNode_.size();
    demand_.assign(nn, NodeDemand{});
    std::vector<int> nodeSupport(nn, 0);
    for (int s : supports) {
        const int node = nodeOfPart_[static_cast<size_t>(s)];
        if (node >= 0) nodeSupport[static_cast<size_t>(node)] = 1;
    }
    for (size_t k = 0; k < nn; ++k) {
        const PartRec& r = *parts_[static_cast<size_t>(partOfNode_[k])];
        const ClusterRec& cl = clusters_[static_cast<size_t>(r.cluster)];
        const size_t c = static_cast<size_t>(r.cluster);
        double I[3][3];
        ToArray(r.ownBody.Rotated(Mat3::Rotation(cl.q)), I);
        demand_[k] = RigidPartDemand(static_cast<double>(r.rbMass), r.comNow, I, cl.com, ca[c], calpha[c], cl.w, stepAccel_,
                                     stepForce_[static_cast<size_t>(partOfNode_[k])], stepMoment_[static_cast<size_t>(partOfNode_[k])]);
    }
    for (size_t c = 0; c < C; ++c) {
        const ClusterRec& cl = clusters_[c];
        if (cl.body == kInvalidBody) continue;
        std::vector<int> members, touching;
        for (int i : cl.parts) {
            const int node = nodeOfPart_[static_cast<size_t>(i)];
            if (node < 0 || partOfNode_[static_cast<size_t>(node)] != i) continue;
            members.push_back(node);
            if (nodeSupport[static_cast<size_t>(node)]) touching.push_back(node);
        }
        if (members.empty()) continue;
        if (!touching.empty() && graph) continue;  // the ground elements carry the residual
        const std::vector<int>& share = touching.empty() ? members : touching;
        Vector3d mf;
        for (int node : share) {
            const PartRec& r = *parts_[static_cast<size_t>(partOfNode_[static_cast<size_t>(node)])];
            const double w = touching.empty() ? static_cast<double>(r.rbMass) / cl.mass : 1.0 / static_cast<double>(share.size());
            const Vector3d cf = resF[c] * w;
            demand_[static_cast<size_t>(node)].force -= cf;
            mf += Cross(r.comNow - cl.com, cf);
        }
        const Vector3d couple = (resM[c] - mf) * (1.0 / static_cast<double>(share.size()));
        for (int node : share) demand_[static_cast<size_t>(node)].moment -= couple;
    }
    // ---- into the factor frame (each cluster's rotation since its creation undone)
    for (size_t k = 0; k < nn; ++k) {
        const ClusterRec& cl = clusters_[static_cast<size_t>(parts_[static_cast<size_t>(partOfNode_[k])]->cluster)];
        const QuaternionD qi = QuaternionD::Inverse(cl.q);
        demand_[k].force = qi * demand_[k].force;
        demand_[k].moment = qi * demand_[k].moment;
    }
    // ---- solve
    const size_t J = joints_.size();
    subLoads_.assign(J, std::array<ElementLoad, 3>{});
    jointLoads_.assign(J, JointLoad{});
    std::vector<uint8_t> solved(J, 0);
    if (graph) {
        graph_.Solve(demand_, elementLoads_);
        for (size_t e = 0; e < elements_.size(); ++e) {
            const StructElement& el = elements_[e];
            if (el.joint < 0) continue;
            subLoads_[static_cast<size_t>(el.joint)][static_cast<size_t>(el.sub)] = elementLoads_[e];
            solved[static_cast<size_t>(el.joint)] = 1;
        }
        lastSolve_ = LoadSolve::Graph;
    } else {
        std::vector<JointResultant> res;
        TreeSolve(treeParentNode_, treeParentJoint_, nodeCom0_, demand_, centroid0_, res);
        std::array<ElementLoad, 3> tmp{};
        for (size_t k = 0; k < nn; ++k) {
            const int j = treeParentJoint_[k];
            if (j < 0) continue;
            const JointSplit& sp = splits_[static_cast<size_t>(j)];
            sp.Apply(res[static_cast<size_t>(j)].force, res[static_cast<size_t>(j)].moment, tmp.data());
            for (size_t e = 0; e < sp.Count(); ++e)
                subLoads_[static_cast<size_t>(j)][static_cast<size_t>(splitSub_[static_cast<size_t>(j)][e])] = tmp[e];
            solved[static_cast<size_t>(j)] = 1;
        }
        lastSolve_ = LoadSolve::Tree;
    }
    // ---- per joint: resultant (physics frame), the largest sub-joint loads and the capacity (PhysX: force > breakForce || torque >
    // breakTorque on each ConfigurableJoint; autostruts / struts have +inf limits, CheatOptions.UnbreakableJoints makes all unbreakable)
    for (size_t j = 0; j < J; ++j) {
        if (!solved[j]) continue;
        const PartJointSpec& js = joints_[j];
        JointLoad& L = jointLoads_[j];
        L.valid = true;
        const int ih = index_.Find(js.hostBody);
        const ClusterRec& hc = clusters_[static_cast<size_t>(parts_[static_cast<size_t>(ih)]->cluster)];
        const bool breakable = options_.breakJoints && !options_.unbreakableJoints && js.kind == JointKind::Attach;
        JointBreak br;
        br.joint = j;
        Vector3d F, M;
        for (int s = 0; s < js.internalJoints; ++s) {
            if (!(subAlive_[j] & (1u << s))) continue;
            const ElementLoad& el = subLoads_[j][static_cast<size_t>(s)];
            F += el.force;
            M += el.torque + Cross(anchor0_[j][static_cast<size_t>(s)] - centroid0_[j], el.force);
            const float f = static_cast<float>(el.force.magnitude());
            const float t = static_cast<float>(el.torque.magnitude());
            L.maxForce = std::max(L.maxForce, f);
            L.maxTorque = std::max(L.maxTorque, t);
            const float ratio = std::max(f / js.breakForce, t / js.breakTorque);
            if (ratio > L.ratio) {
                L.ratio = ratio;
                L.critical = s;
            }
            if (breakable && (f > js.breakForce || t > js.breakTorque)) {
                br.mask = static_cast<uint8_t>(br.mask | (1u << s));
                if (std::max(f / js.breakForce, t / js.breakTorque) >= std::max(br.force / js.breakForce, br.torque / js.breakTorque)) {
                    br.force = f;
                    br.torque = t;
                }
            }
        }
        L.force = hc.q * F;
        L.moment = hc.q * M;
        if (L.ratio > maxRatio_) {
            maxRatio_ = L.ratio;
            maxRatioJoint_ = static_cast<int>(j);
        }
        if (br.mask != 0) breaks_.push_back(br);
    }
    lastSolveMs_ = MsSince(t0);
}

// ===================================================================================== Rebind

std::vector<std::unique_ptr<VesselPhysics>> VesselPhysics::Rebind(PhysicsWorld& world, const std::vector<VesselPhysics*>& sources,
                                                                  const std::vector<Vessel*>& targets, const VesselPhysicsOptions& options) {
    std::unordered_map<const Part*, PartPhysicsState> byPart;
    for (VesselPhysics* s : sources)
        for (size_t i = 0; i < s->parts_.size(); ++i) byPart[s->parts_[i]->part] = s->StateAt(static_cast<int>(i));
    std::vector<std::unique_ptr<VesselPhysics>> out;
    for (Vessel* v : targets) {
        if (v == nullptr || v->parts.empty()) continue;
        // a part of v with a known state: the root if known, else the first known part in vessel order
        const Part* ref = nullptr;
        if (v->rootPart != nullptr && byPart.count(v->rootPart) != 0) ref = v->rootPart;
        for (size_t i = 0; ref == nullptr && i < v->parts.size(); ++i)
            if (byPart.count(v->parts[i]) != 0) ref = v->parts[i];
        if (ref == nullptr) continue;
        const PartPhysicsState& rs = byPart.at(ref);
        // the vessel transform from the reference part's pose and its pristine coordinates (orgPos / orgRot: Vessel.SetPosition rules)
        const QuaternionD rot = rs.rotation * QuaternionD::Inverse(QuaternionD(ref->fields.orgRot));
        const Vector3d origin = rs.position - rot * Vector3d(ref->fields.orgPos);
        std::vector<PartPhysicsState> states(v->parts.size());
        for (size_t i = 0; i < v->parts.size(); ++i) {
            const Part* p = v->parts[i];
            const auto it = byPart.find(p);
            if (it != byPart.end()) {
                states[i] = it->second;
                continue;
            }
            PartTransform t;
            t.position = p->fields.orgPos;
            t.rotation = p->fields.orgRot;
            t.localScale = p->partTransform.localScale;
            PartPhysicsState& s = states[i];
            s.position = origin + rot * Vector3d(t.position);
            s.rotation = rot * QuaternionD(t.rotation);
            s.centerOfMass = origin + rot * Vector3d(t.TransformPoint(p->fields.CoMOffset));
            s.angularVelocity = rs.angularVelocity;  // the reference part's rigid motion
            s.velocity = rs.velocity + Cross(rs.angularVelocity, s.centerOfMass - rs.centerOfMass);
        }
        auto b = std::make_unique<VesselPhysics>(world, *v, states, options);
        if (!sources.empty()) {
            b->frameOriginWorld = sources.front()->frameOriginWorld;
            b->floatOrigin = sources.front()->floatOrigin;
        }
        b->SyncPartTransforms();
        out.push_back(std::move(b));
    }
    return out;
}

}  // namespace ksp::physics
