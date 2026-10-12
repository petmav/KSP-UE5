// WP-20c: the structural load model of the reduced rigid-body model (core/PHYSICS-SPIKE.md sections 2.8, 3.4, 6 R1, 7).
//
// The reduced model welds the parts of a cluster into one rigid body, so the joints carry no state of their own. Their LOAD follows
// from the motion: every part with a Rigidbody (a NODE, its physicsless children folded in as their mass is) needs the structure to
// apply the force and moment that make it move with its cluster (its DEMAND), and the PartJoints between the nodes carry those demands.
// KSP's joints, as data (PartJoint.cs:176-205, :359-395): every PartJoint is 1 or 3 PhysX ConfigurableJoints (sub-joints), each a
// quasi-rigid linear drive (positionSpring = PhysicsGlobals.JointForce) at its anchor plus an angular drive (positionSpring =
// PartJoint.stiffness; locked when rigid). Here every sub-joint is a linear-elastic ELEMENT between its two nodes:
//     force on the host side  f = -kLinear (t_b + th_b x r_b - t_a - th_a x r_a)          (the linear drive; kLinear quasi-rigid)
//     torque on the host side t = -kAngular (th_b - th_a)                                  (the angular drive)
// and a part touching static geometry gets a GROUND SUPPORT element at its centre of mass. Each PhysX sub-joint breaks on its own
// force or torque (|f| > breakForce, |t| > breakTorque: PartJoint.cs:340-341 set them); the element loads are what that rule is
// applied to.
//
// Two solves of the same model:
//   tree   the statically determinate case (no struts / autostruts / docking loops, at most one support): every joint carries the
//          demands of the subtree behind it, O(parts), and its sub-joints share that resultant by the joint's own local stiffness
//          (JointSplit: a 6x6 solve, constant per joint).
//   graph  the indeterminate case: K u = -demand over all elements (one fixed node per connected piece: the ground, else the heaviest
//          part), assembled and factored ONCE per structural change (sparse block Cholesky over 6x6 node blocks, greedy minimum-degree
//          order, the spike's structure_graph_direct.cpp) in the frame the geometry was given in; a tick is two block-triangular
//          solves. On a tree it gives the tree solve's loads (to rounding): the element loads of a tree joint depend only on its subtree.
// Engine glue in double: no Mono float rule applies (PhysX's solver is not reproduced; the capacity numbers are, PartJointSpec).
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "ksp/math/vector3d.hpp"

namespace ksp::physics {

// What the structure must apply to a node: force and moment about the node's centre of mass.
struct NodeDemand {
    Vector3d force;
    Vector3d moment;
};

// One element's load on its b (host) node: the force at the anchor and the angular-drive torque.
struct ElementLoad {
    Vector3d force;
    Vector3d torque;
};

// The resultant a joint applies to the part of the structure on its child side: force and moment about the joint's anchor centroid.
struct JointResultant {
    Vector3d force;
    Vector3d moment;
};

// One element of the graph. a / b: node indices (a = kGround for a ground support); the anchor is in the frame of the solve.
struct StructElement {
    static constexpr int kGround = -1;
    int a = kGround;
    int b = -1;
    Vector3d anchor;
    double kLinear = 0.0;
    double kAngular = 0.0;
    int joint = -1;  // the owner joint (-1: a ground support)
    int sub = 0;     // its sub-joint index
};

// The demand of a rigid part moving with its cluster: centre of mass x, mass m, own inertia tensor I (frame axes); the cluster's
// centre of mass `com`, acceleration a, angular acceleration alpha and angular velocity w; the uniform acceleration g every Rigidbody
// gets (integrationAccel: gravity, Coriolis, centrifugal); the external force F through the part's centre of mass and moment M:
//     force  = m (a + alpha x r + w x (w x r) - g) - F,     moment = I alpha + w x (I w) - M,     r = x - com.
NodeDemand RigidPartDemand(double m, const Vector3d& x, const double I[3][3], const Vector3d& com, const Vector3d& a, const Vector3d& alpha,
                           const Vector3d& w, const Vector3d& g, const Vector3d& F, const Vector3d& M);

// The tree solve. parentNode[i] = the node on the other side of node i's joint towards the root (-1: a root of its piece), parentJoint[i]
// = that joint's index (into jointCentroid / out). Children are accumulated into parents in a fixed order (descending node index after
// a stable topological sort), so the result is deterministic. out[j] = the resultant joint j applies to the subtree of its child node
// (= the sum of that subtree's demands, moment about jointCentroid[j]); joints that are no tree edge keep zero.
void TreeSolve(const std::vector<int>& parentNode, const std::vector<int>& parentJoint, const std::vector<Vector3d>& nodeCom,
               const std::vector<NodeDemand>& demand, const std::vector<Vector3d>& jointCentroid, std::vector<JointResultant>& out);
// The two halves of TreeSolve: the children-first order of a forest (TreeSolve's), computed once per structure, and the accumulation over
// it (an edge whose parent is not deeper-first in `order` is skipped, as a cycle is in TreeOrder)
std::vector<int> TreeOrder(const std::vector<int>& parentNode);
struct TreeScratch {  // the working storage of TreeSolveOrdered (reused across calls: no allocation per tick)
    std::vector<int> rank;
    std::vector<Vector3d> sf, sm;
};
void TreeSolveOrdered(const std::vector<int>& order, const std::vector<int>& parentNode, const std::vector<int>& parentJoint,
                      const std::vector<Vector3d>& nodeCom, const std::vector<NodeDemand>& demand, const std::vector<Vector3d>& jointCentroid,
                      std::vector<JointResultant>& out, TreeScratch* scratch = nullptr);

// The sub-joints of one joint sharing a resultant between two rigid sides by their stiffness (the local 6x6 system of the elements;
// translation and rotation decouple when the anchors are centred on the centroid, as KSP's 120-degree rings are).
class JointSplit {
public:
    // anchors relative to the joint's anchor centroid; false when the local stiffness is singular
    bool Setup(const std::vector<Vector3d>& anchorFromCentroid, const std::vector<double>& kLinear, const std::vector<double>& kAngular);
    // the resultant (F, M about the centroid) the joint applies to one side -> every element's load on that side
    void Apply(const Vector3d& force, const Vector3d& moment, ElementLoad* out) const;
    size_t Count() const { return rel_.size(); }

private:
    std::array<double, 36> inv_{};
    std::vector<Vector3d> rel_;
    std::vector<double> kl_, ka_;
};

// The indeterminate solve (see the header comment).
class StructuralGraph {
public:
    // Assembles K over `nodeCount` nodes (centres of mass nodeCom, masses for the choice of the fixed node) and the elements, and factors
    // it. Returns false when K is not positive definite (the caller falls back to the tree solve).
    bool Build(const std::vector<Vector3d>& nodeCom, const std::vector<double>& nodeMass, const std::vector<StructElement>& elements);
    bool Factored() const { return factored_; }
    // K u = -demand (demands in the frame of Build; the fixed nodes' rows are dropped: their reaction is the imbalance of the demands,
    // zero for self-equilibrated ones); out[e] = element e's load on its b node. Requires a successful Build.
    void Solve(const std::vector<NodeDemand>& demand, std::vector<ElementLoad>& out);
    // the resultant joint j's elements apply to their b nodes (moment about `centroid`)
    static JointResultant Resultant(const std::vector<StructElement>& elements, const std::vector<ElementLoad>& loads, int joint, const Vector3d& centroid);

    size_t NodeCount() const { return pos_.size(); }
    size_t FreeNodeCount() const { return order_.size(); }
    size_t FactorBlocks() const { return lBlocks_; }  // off-diagonal 6x6 blocks of L (fill included)
    bool NodeFixed(size_t node) const { return pos_[node] < 0; }
    const std::vector<StructElement>& Elements() const { return elems_; }

private:
    using M6 = std::array<double, 36>;
    using V6 = std::array<double, 6>;
    struct Col {
        std::vector<int> rows;  // later-eliminated positions with a nonzero block in this column, ascending
        std::vector<M6> L;      // L(row, this column)
        M6 Lkk{};               // Cholesky factor of the diagonal block (lower triangular)
    };
    std::vector<StructElement> elems_;
    std::vector<Vector3d> ra_, rb_;  // anchor relative to the a / b node's centre of mass (ra: 0 for ground)
    std::vector<int> order_, pos_;   // elimination order over the free nodes; pos_[node] = its position in order_, -1: fixed
    std::vector<Col> cols_;
    size_t lBlocks_ = 0;
    bool factored_ = false;
    std::vector<V6> rhs_;            // scratch of Solve
    std::vector<V6> u_;              // displacement per node (fixed: 0)
};

}  // namespace ksp::physics
