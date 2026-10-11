// Jolt spike, model B: the reduced rigid-body vessel (PHYSICS-ENGINE-REVIEW.md 11.3, lever 3).
//
// Parts between articulation points are welded into one Jolt body per cluster (compound of the parts' collider proxies; mass,
// CoM and inertia summed from the parts with KSP's per-part tensor rule). Clustering:
//   Whole              one body per connected vessel (decouplers are cut only when they fire)
//   SplitAtDecouplers  one body per piece between decouplers, pieces joined by Jolt FixedConstraints
// Forces are accumulated per part (Part.AddForceAtPosition semantics: KSP already does this for physicsless parts) and applied to
// the cluster body as one force + torque about its CoM.
//
// Structural graph: every attach joint keeps KSP's capacity (PartJoint break force / torque, PartJoint.cs:307-342). After each
// step the internal load of every attach joint is computed by a static-equilibrium pass over the part tree (O(parts)):
//   demand_i = m_i (a_cm + alpha x r_i + omega x (omega x r_i) - g) - F_ext,i - C_i         (C_i: contact share of the part)
//   J(edge) = sum over the child subtree of demand_i;  M(edge) = moments of the same about the joint point (+ I_i alpha + w x I_i w)
// where the cluster's a_cm / alpha come from its velocity change over the step (so contacts and inter-cluster joints are included)
// and the unexplained residual of a cluster (contacts) is shared by its parts touching static geometry (Jolt contact listener).
// A joint whose load exceeds its capacity breaks: the cluster is split there and rebuilt as new bodies (velocities inherited).
// Autostruts / struts: parallel load paths of the indeterminate graph; the spike reports loads WITHOUT their relief (tree solve,
// conservative) -- see PHYSICS-SPIKE.md for the graph solve WP-20 needs.
#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Constraints/TwoBodyConstraint.h>

#include <atomic>
#include <memory>
#include <vector>

#include "structure_graph.hpp"
#include "vessel_physics.hpp"

namespace spike {

class ReducedVessel final : public VesselPhysics {
public:
    enum class Clustering { Whole, SplitAtDecouplers };
    explicit ReducedVessel(Clustering c);
    ~ReducedVessel() override;
    const char* Name() const override { return clustering_ == Clustering::Whole ? "reduced-whole" : "reduced-split"; }
    void Build(World& world, const CraftModel& craft, const Placement& placement, unsigned vesselGroup) override;
    void ReadState() override;
    void AddForceAtPosition(int part, JPH::Vec3Arg force, JPH::RVec3Arg point) override;
    void AddUniformAcceleration(JPH::Vec3Arg accel) override;
    void SetAngularDrag(int part, float drag) override;
    void UpdateMass(const std::vector<float>& rbMass) override;
    void Flush() override;
    std::vector<BreakEvent> AfterStep(float dt, int tick) override;
    void ShiftOrigin(JPH::Vec3Arg delta) override;
    void SetLinearVelocity(JPH::Vec3Arg v) override {
        for (Cluster& c : clusters_) c.body->SetLinearVelocity(v);
    }
    void AddLinearVelocity(JPH::Vec3Arg dv) override {
        for (Cluster& c : clusters_) c.body->SetLinearVelocity(c.body->GetLinearVelocity() + dv);
    }
    int FireDecouplers() override;
    int BodyCount() const override;
    int ConstraintCount() const override;
    std::vector<std::pair<float, float>> AttachJointLoads(float dt) const override;

    bool stressEnabled = true;
    bool breakingEnabled = true;
    // statically indeterminate solve (struts / autostruts share load: structure_graph.hpp) instead of the tree solve
    bool graphSolve = false;
    bool graphDirect = true;      // sparse block Cholesky factored per structural change (single cluster); else PCG
    double graphFactorMs = 0.0;   // last factorisation
    size_t graphFactorBlocks = 0;
    double graphKLinear = 1.0e8;  // kN/m: quasi-rigid translation of every sub-joint (PhysX: linear drive spring 1e20)
    int graphMaxIter = 300;
    double graphTol = 1.0e-6;
    StructuralGraph::Stats graphStats;  // last solve
    double graphMsSum = 0.0;
    long long graphItersSum = 0, graphSolves = 0;
    bool inertiaFloor = true;  // KSP's max(1, mass) per-part tensor scale (Part.cs:4263); droppable (PHYSICS-ENGINE-REVIEW.md 10)
    // per attach joint, filled by AfterStep (kN, kN m) and its capacity
    struct EdgeLoad {
        float force = 0.0f, torque = 0.0f;
        float forceCap = 0.0f, torqueCap = 0.0f;
    };
    std::vector<EdgeLoad> loads;
    // maximum load / capacity ratio over the live attach joints at the last AfterStep
    float maxLoadRatio = 0.0f;
    int maxLoadJoint = -1;
    double lastSplitMs = 0.0;  // wall time of the last cluster rebuild

private:
    struct Cluster {
        JPH::Body* body = nullptr;
        std::vector<int> parts;   // all parts (physical and physicsless) of the cluster
        double mass = 0.0;
        JPH::Vec3 force = JPH::Vec3::sZero(), torque = JPH::Vec3::sZero();
        float angularDrag = 0.0f;
        JPH::Vec3 vBefore = JPH::Vec3::sZero(), wBefore = JPH::Vec3::sZero();
        JPH::Mat44 inertiaBody;   // about the CoM, body frame
        bool alive = true;
    };
    class SupportListener;

    void Recluster(const std::vector<bool>& cut, bool inheritVelocity);
    void BuildClusterBody(Cluster& c, JPH::RVec3Arg comHint, JPH::Vec3Arg v, JPH::Vec3Arg w, JPH::RVec3Arg oldCom, bool hasOld);
    void ComputeLoads(float dt);
    void RebuildInterClusterJoints();

    Clustering clustering_;
    World* world_ = nullptr;
    const CraftModel* craft_ = nullptr;
    unsigned group_ = 0;
    std::vector<Cluster> clusters_;
    std::vector<int> clusterOfPart_;
    std::vector<JPH::Vec3> partLocalPos_;  // part transform origin in the cluster body frame (origin = cluster CoM)
    std::vector<JPH::Quat> partLocalRot_;
    std::vector<JPH::RVec3> partPosNow_;    // physics-frame part poses (kept for rebuilds)
    std::vector<JPH::Quat> partRotNow_;
    std::vector<float> partMass_;           // rbMass of physical parts (physicsless children folded in), 0 otherwise
    std::vector<JPH::Vec3> partExtF_, partExtM_;  // buffered external force per part and its moment about the part CoM
    JPH::Vec3 uniformAccel_ = JPH::Vec3::sZero(), lastUniformAccel_ = JPH::Vec3::sZero();
    std::vector<bool> cut_;                 // attach joints that no longer connect (fired or broken)
    std::vector<std::array<JPH::Vec3, 3>> subAnchorLocal_;  // per joint: each sub-joint anchor in the child part's frame
    std::vector<std::array<JPH::RVec3, 3>> subAnchorWorld_;
    std::vector<JPH::Vec3> demandF_, demandM_;
    StructuralGraph graph_;
    void RebuildGraph();
    bool graphDirty_ = true;
    std::vector<uint8_t> lastTouching_;  // parts touching static geometry at the last graph build
    std::vector<JPH::Vec3> anchorLocal_;    // per joint: centroid of the sub-joint anchors in the child part's frame
    std::vector<JPH::Ref<JPH::TwoBodyConstraint>> interJoints_;  // SplitAtDecouplers
    std::vector<int> interJointOf_;         // attach joint index of each inter-cluster constraint
    std::unique_ptr<SupportListener> listener_;
    std::vector<int> postOrder_;            // parts, children before parents
};

}  // namespace spike
