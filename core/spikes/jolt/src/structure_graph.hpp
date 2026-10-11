// Jolt spike, model B: the statically indeterminate structural solve (struts / autostruts / docking loops share load).
//
// Every live PartJoint sub-joint (attach joints and autostruts) is a linear-elastic element between the two rigidbody-owning
// parts it connects, at its anchor p:
//     translation: k_lin * (x_b + th_b x r_b - x_a - th_a x r_a)       (PhysX: linear drive positionSpring = JointForce 1e20,
//                                                                       i.e. rigid; here a finite quasi-rigid k_lin)
//     rotation:    k_ang * (th_b - th_a)                                (the angular drive positionSpring = PartJoint stiffness;
//                                                                       rigid joints: k_rigid)
// The load each part needs from the structure (its inertial demand minus its external and contact forces: the same demand the
// tree solve accumulates) is applied as nodal force + moment; K u = -demand is solved by preconditioned conjugate gradients
// (6x6 block-Jacobi), warm-started from the previous tick, one node fixed per connected piece (the demands are self-equilibrated).
// Element loads are k * stretch. For a tree this reproduces the tree solve; with loops the split follows the element stiffness.
#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Math/DVec3.h>
#include <Jolt/Math/Vec3.h>

#include <array>
#include <vector>

#include "craft_model.hpp"

namespace spike {

class StructuralGraph {
public:
    struct Stats {
        int iterations = 0;
        double residual = 0.0;  // relative
        double ms = 0.0;
    };
    // `live[j]` per CraftModel joint (attach joints and autostruts): false for cut / dropped joints
    void Build(const CraftModel& craft, const std::vector<bool>& live, double kLinear);
    // nodeCom: per part (only rigidbody owners are nodes), physics frame; anchors: per joint centroid world position; demand:
    // force / moment (about the part CoM) per part, physicsless parts folded into their owner here
    Stats Solve(const std::vector<JPH::RVec3>& partCom, const std::vector<std::array<JPH::RVec3, 3>>& subAnchors,
                const std::vector<JPH::Vec3>& demandF, const std::vector<JPH::Vec3>& demandM, int maxIter, double tol);
    // Direct method for a single rigid cluster: K is constant in the cluster's frame, so it is assembled and factored once per
    // structural change (sparse block Cholesky, 6x6 node blocks, greedy minimum-degree order) and every tick costs two triangular
    // solves. `clusterRot` = the cluster body's rotation now; demands are rotated into the frame of the factorisation.
    Stats Factor(const std::vector<JPH::RVec3>& partCom, const std::vector<std::array<JPH::RVec3, 3>>& subAnchors, JPH::QuatArg clusterRot);
    Stats SolveFactored(const std::vector<JPH::RVec3>& partCom, const std::vector<JPH::Vec3>& demandF, const std::vector<JPH::Vec3>& demandM,
                        JPH::QuatArg clusterRot);
    bool Factored() const { return factored_; }
    size_t FactorBlocks() const { return lBlocks_; }  // off-diagonal 6x6 blocks of L (fill included)
    // per joint (CraftModel index): transmitted force and torque about the anchor centroid after Solve; -1 when not live
    std::vector<std::pair<float, float>> jointLoads;

private:
    using M6 = std::array<double, 36>;
    struct Col {
        std::vector<int> rows;  // later-eliminated nodes (positions in the order) with a nonzero block in this column
        std::vector<M6> L;      // L(row, this)
        M6 Lkk{};               // Cholesky factor of the diagonal block (lower)
    };
    void ElementLoads(const std::vector<std::array<JPH::RVec3, 3>>* subAnchorsWorld);
    bool factored_ = false;
    std::vector<int> order_, pos_;  // elimination order over free nodes; pos_[node] = index in order_ (-1: fixed)
    std::vector<Col> cols_;
    size_t lBlocks_ = 0;
    JPH::Quat factorRot_ = JPH::Quat::sIdentity();
    std::vector<JPH::DVec3> anchorF_;  // per element: anchor in the factor frame (for the torque arm about the joint centroid)
    struct Elem {
        int a = -1, b = -1;  // node indices
        int joint = -1, sub = 0;
        double kLin = 0, kAng = 0;
        JPH::DVec3 ra, rb;   // anchor relative to the node CoMs (set per solve)
    };
    using V6 = std::array<double, 6>;
    void Apply(const std::vector<V6>& u, std::vector<V6>& out) const;  // out = K u

    const CraftModel* craft_ = nullptr;
    std::vector<int> nodeOfPart_;  // part -> node (its rigidbody owner's node)
    std::vector<int> partOfNode_;
    std::vector<bool> fixed_;      // one fixed node per connected piece
    std::vector<Elem> elems_;
    std::vector<V6> u_;            // warm start (displacement, rotation) per node
    std::vector<std::array<double, 21>> precond_;  // packed inverse of each node's 6x6 diagonal block (lower triangle)
};

}  // namespace spike
