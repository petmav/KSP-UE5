// WP-20b unit tests: vessel rigid bodies on the reduced rigid-body model (ksp/physics/vessel_physics.hpp, core/PHYSICS-SPIKE.md).
//
// No golden or capture can cover rigid-body dynamics (PhysX is not reproducible; Jolt is a different solver), so the gate is:
//   * the KSP rules as data, against the decompiled formulas and PROFILE.md's runtime joint counts;
//   * mass / centre of mass / inertia against closed forms, and against the WP-14 CalculatePhysicsStats port BIT FOR BIT;
//   * dynamics against analytic results (rigid two-cluster craft under a force / torque, momentum at a decouple);
//   * the spike's reduced-model trajectories (core/spikes/jolt/results), determinism at 1 / 2 / 4 / 8 threads;
//   * scenarios: a vessel at rest on the pad for 10 s, an 80 km orbit coast through VesselPrecalculate (WP-19a physics_orbit_hold).
// Hashes are compared inside one process only (Jolt is deterministic per binary, PHYSICS-SPIKE.md R8).
#include <cmath>
#include <cstring>
#include <map>
#include <set>

#include "doctest/doctest.h"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/physics/floating_origin.hpp"
#include "ksp/physics/krakensbane.hpp"
#include "ksp/physics/reference_frame.hpp"
#include "vessel_physics_fixture.hpp"

using namespace ksp;
using namespace ksp::physics;
using namespace wp20b;

namespace {

bool SameBits(double a, double b) { return std::memcmp(&a, &b, sizeof a) == 0; }
bool SameBits(float a, float b) { return std::memcmp(&a, &b, sizeof a) == 0; }
bool SameBits(const Vector3d& a, const Vector3d& b) { return SameBits(a.x, b.x) && SameBits(a.y, b.y) && SameBits(a.z, b.z); }
bool SameBits(const Vector3f& a, const Vector3f& b) { return SameBits(a.x, b.x) && SameBits(a.y, b.y) && SameBits(a.z, b.z); }

int SubJoints(const std::vector<PartJointSpec>& joints, JointKind kind) {
    int n = 0;
    for (const PartJointSpec& j : joints)
        if (j.kind == kind) n += j.internalJoints;
    return n;
}

Part* FindPart(Vessel& v, const std::string& name) {
    for (Part* p : v.parts)
        if (p->partInfo != nullptr && p->partInfo->name == name) return p;
    return nullptr;
}

}  // namespace

TEST_CASE("WP-20b PartJoint data: sub-joint counts equal the runtime ConfigurableJoint counts of PROFILE.md (S 24, S-AS 44, M2 168, M2-AS 320, L2-AS 960)") {
    if (!GameAvailable()) return;
    World w;
    const std::vector<std::pair<std::string, int>> runtime = {
        {"PhysProf-S", 24}, {"PhysProf-S-AS", 44}, {"PhysProf-M2", 168}, {"PhysProf-M2-AS", 320}, {"PhysProf-L2-AS", 960}};
    for (const auto& [name, count] : runtime) {
        auto v = w.Craft(name);
        const std::vector<PartJointSpec> joints = BuildPartJoints(*v);
        const int attach = SubJoints(joints, JointKind::Attach), autostrut = SubJoints(joints, JointKind::AutoStrut);
        INFO(name << ": attach " << attach << " autostrut " << autostrut);
        CHECK(attach + autostrut == count);
        // every FULL non-root part makes one attach joint (all PhysProf parts are FULL); one size-1 autostrut per Grandparent part
        int full = 0;
        for (Part* p : v->parts) full += p->hasRigidbody && p->parent != nullptr ? 1 : 0;
        int attachJoints = 0;
        for (const PartJointSpec& j : joints) attachJoints += j.kind == JointKind::Attach ? 1 : 0;
        CHECK(attachJoints == full);
    }
}

TEST_CASE("WP-20b PartJoint data: host / target, node choice, stiffness and break limits follow PartJoint.cs (Mono float rule)") {
    if (!GameAvailable()) return;
    World w;
    auto v = w.Craft("PhysProf-S");
    const std::vector<PartJointSpec> joints = BuildPartJoints(*v);
    const float jbf = PhysicsGlobals::JointBreakForceFactor(), jbt = PhysicsGlobals::JointBreakTorqueFactor();
    int checked = 0;
    for (const PartJointSpec& j : joints) {
        if (j.kind != JointKind::Attach) continue;
        CHECK(j.host == j.child);  // both FULL (PartJoint.cs:145)
        CHECK(j.target == j.parent);
        CHECK(j.hostBody == j.child);
        CHECK(j.targetBody == j.parent);
        // the smaller of the two nodes (PartJoint.cs:111-123); a surface-attached part uses its parent's srfAttachNode or its own
        const AttachNode* toParent = j.child->FindAttachNodeByPart(j.parent);
        const AttachNode* fromParent = j.parent->FindAttachNodeByPart(j.child);
        int size = 0;
        if (toParent && fromParent) size = std::min(toParent->size, fromParent->size);
        else if (toParent) size = toParent->size;
        else if (fromParent) size = fromParent->size;
        else size = j.parent->srfAttachNode->size;
        CHECK(j.nodeSize == size);
        CHECK(j.internalJoints == (size >= 2 ? 3 : 1));
        // num3 = (size + 1) * {2 stack, 0.8 surface}; stiffness = 1500 * num3 * 10; break = min(host, target) * num3 / joints
        const double factor = j.mode == 0 ? 2.0 : static_cast<double>(0.8f);
        const float num3 = static_cast<float>((static_cast<double>(size) + 1.0) * factor);
        CHECK(SameBits(j.stiffness, static_cast<float>(1500.0 * static_cast<double>(num3) * 10.0)));
        const float bf = static_cast<float>(static_cast<double>(std::min(j.host->fields.breakingForce, j.target->fields.breakingForce)) *
                                            static_cast<double>(num3) / static_cast<double>(j.internalJoints));
        const float bt = static_cast<float>(static_cast<double>(std::min(j.host->fields.breakingTorque, j.target->fields.breakingTorque)) *
                                            static_cast<double>(num3) / static_cast<double>(j.internalJoints));
        CHECK(SameBits(j.breakingForce, bf));
        CHECK(SameBits(j.breakingTorque, bt));
        CHECK(SameBits(j.breakForce, static_cast<float>(static_cast<double>(bf) * static_cast<double>(jbf))));  // not rigid: factor 1
        CHECK(SameBits(j.breakTorque, static_cast<float>(static_cast<double>(bt) * static_cast<double>(jbt))));
        // 3 sub-joints on a ring of radius size - 1 around the node, 120 degrees apart (PartJoint.cs:180)
        if (j.internalJoints == 3) {
            const Vector3f c = (j.anchors[0] + j.anchors[1] + j.anchors[2]) * (1.0f / 3.0f);
            for (int k = 0; k < 3; ++k) CHECK(std::fabs((j.anchors[static_cast<size_t>(k)] - c).magnitude() - static_cast<float>(size - 1)) < 1e-4f);
        }
        ++checked;
    }
    CHECK(checked == 20);
    // the Mainsail on the Rockomax tank: two size-2 stack nodes -> 3 sub-joints, stiffness 1500 * 6 * 10 = 90,000
    Part* mainsail = FindPart(*v, "liquidEngineMainsail.v2");
    REQUIRE(mainsail != nullptr);
    for (const PartJointSpec& j : joints)
        if (j.kind == JointKind::Attach && j.child == mainsail) {
            CHECK(j.internalJoints == 3);
            CHECK(j.stiffness == 90000.0f);
        }
}

TEST_CASE("WP-20b Part.Start rigid bodies: physicsless parts ride on their RigidBodyPart, their mass in its physicsMass, no attach joint") {
    if (!GameAvailable()) return;
    World w;
    auto v = w.Craft("PhysProf-S");
    Part* cone = FindPart(*v, "pointyNoseConeB");
    REQUIRE(cone != nullptr);
    REQUIRE(cone->parent != nullptr);
    cone->fields.PhysicsSignificance = 1;  // as a part whose cfg says PhysicsSignificance = 1
    StartPartRigidbodies(*v);
    CHECK_FALSE(cone->hasRigidbody);
    CHECK(cone->fields.physicalSignificance == 1);
    CHECK(RigidBodyPart(cone) == cone->parent);
    CHECK(v->rootPart->hasRigidbody);
    v->UpdateMassStats();
    // FlightIntegrator.GetPhysicslessChildMass: the parent carries the cone's mass
    const float parentOwn = cone->parent->fields.mass + cone->parent->GetResourceMass();
    CHECK(static_cast<float>(cone->parent->fields.physicsMass) == doctest::Approx(parentOwn + cone->fields.mass + cone->GetResourceMass()).epsilon(1e-6));
    CHECK(cone->fields.physicsMass == 0.0);
    const std::vector<PartJointSpec> joints = BuildPartJoints(*v);
    for (const PartJointSpec& j : joints) CHECK(j.child != cone);
    CHECK(SubJoints(joints, JointKind::Attach) == 24 - 1);  // the cone's joint is gone (size-1 node)
    // the root is FULL whatever its PhysicsSignificance says (Part.cs:1830)
    v->rootPart->fields.PhysicsSignificance = 1;
    StartPartRigidbodies(*v);
    CHECK(v->rootPart->hasRigidbody);
    CHECK(v->rootPart->fields.physicalSignificance == 0);
}

TEST_CASE("WP-20b mass properties: rb.mass, ValidateInertiaTensor and the cluster sums against closed forms") {
    if (!GameAvailable()) return;
    World w;
    // rb.mass = Max(MinimumRBMass, physicsMass) (FlightIntegrator.cs:483); inertia = per-unit tensor * Max(1, rb.mass) (Part.cs:4267)
    CHECK(PartInertiaTensor(Vector3f(0.5f, 0.25f, 2.0f), 4.0f) == Vector3f(2.0f, 1.0f, 8.0f));
    CHECK(PartInertiaTensor(Vector3f(0.5f, 0.25f, 2.0f), 0.25f) == Vector3f(0.5f, 0.25f, 2.0f));          // the Max(1, m) floor
    CHECK(PartInertiaTensor(Vector3f(0.5f, 0.25f, 2.0f), 0.25f, false) == Vector3f(0.125f, 0.0625f, 0.5f));
    CHECK(PartInertiaTensor(Vector3f(1e-7f, 1.0f, 1.0f), 4.0f) == Vector3f(4e-7f, 4.0f, 4.0f));            // no override below 1e-6
    // a box of half extents (1, 2, 3): ((4 + 9) / 3, (1 + 9) / 3, (1 + 4) / 3) per unit mass
    ColliderProxy box;
    box.halfExtents = Vector3f(1.0f, 2.0f, 3.0f);
    const Vector3f iu = ProxyInertiaPerUnitMass({box});
    CHECK(iu.x == doctest::Approx(13.0 / 3.0));
    CHECK(iu.y == doctest::Approx(10.0 / 3.0));
    CHECK(iu.z == doctest::Approx(5.0 / 3.0));

    // Three real parts stacked on the Y axis (probe core / Rockomax X200-32 / Mainsail, the head of PhysProf-S), each given a unit
    // cube collider at its origin: M = sum m_i, CoM = sum m_i c_i / M, I = sum (I_i + m_i (|d|^2 E - d d^T)) about the CoM.
    auto v = w.CraftHead("PhysProf-S", 3);
    REQUIRE(v->parts.size() == 3);
    VesselPhysicsOptions opt;
    opt.colliders = [](const Part&) {
        ColliderProxy c;
        c.halfExtents = Vector3f(0.5f, 0.5f, 0.5f);
        return std::vector<ColliderProxy>{c};
    };
    PhysicsWorld world;
    VesselPhysics vp(world, *v, VesselPose{}, opt);
    REQUIRE(vp.ClusterCount() == 1);
    double M = 0, cy = 0, cx = 0, cz = 0;
    for (Part* p : v->parts) {
        const double m = std::max(static_cast<double>(p->partInfo->minimumRBMass), p->fields.physicsMass);
        CHECK(vp.PartMass(*p) == static_cast<float>(m));
        const Vector3f com = p->fields.orgPos + p->fields.orgRot * p->fields.CoMOffset;
        M += m;
        cx += m * com.x;
        cy += m * com.y;
        cz += m * com.z;
    }
    cx /= M;
    cy /= M;
    cz /= M;
    CHECK(vp.ClusterMass(0) == doctest::Approx(M).epsilon(1e-15));
    const Vector3d c = vp.ClusterCenterOfMass(0);
    CHECK(c.x == doctest::Approx(cx).epsilon(1e-12));
    CHECK(c.y == doctest::Approx(cy).epsilon(1e-12));
    CHECK(c.z == doctest::Approx(cz).epsilon(1e-12));
    double Ixx = 0, Iyy = 0, Izz = 0, Ixy = 0;
    for (Part* p : v->parts) {
        const double m = vp.PartMass(*p);
        const double own = (0.25 + 0.25) / 3.0 * std::max(1.0, m);  // (h^2 + h^2) / 3 * Max(1, m), h = 0.5
        const Vector3f pc = p->fields.orgPos + p->fields.orgRot * p->fields.CoMOffset;
        const double dx = pc.x - c.x, dy = pc.y - c.y, dz = pc.z - c.z;
        Ixx += own + m * (dy * dy + dz * dz);
        Iyy += own + m * (dx * dx + dz * dz);
        Izz += own + m * (dx * dx + dy * dy);
        Ixy += -m * dx * dy;
    }
    double I[3][3];
    vp.ClusterInertia(0, I);
    CHECK(I[0][0] == doctest::Approx(Ixx).epsilon(1e-9));
    CHECK(I[1][1] == doctest::Approx(Iyy).epsilon(1e-9));
    CHECK(I[2][2] == doctest::Approx(Izz).epsilon(1e-9));
    CHECK(I[0][1] == doctest::Approx(Ixy).epsilon(1e-9));
    CHECK(I[0][1] == I[1][0]);
    // the solver holds the same mass (float)
    CHECK(world.GetMass(vp.ClusterBody(0)) == doctest::Approx(M).epsilon(1e-6));
}

TEST_CASE("WP-20b ReadState is CalculatePhysicsStats (WP-14 / WP-19a port) bit for bit, and the cluster sums equal its CoMD / total mass") {
    if (!GameAvailable()) return;
    World w;
    for (const char* name : {"PhysProf-S", "PhysProf-M2", "PhysProf-L2-AS"}) {
        INFO(name);
        auto v = w.Craft(name);
        v->packed = false;  // the MOI branch runs for unpacked vessels
        PhysicsWorld world;
        VesselPhysics vp(world, *v, VesselPose{}, {});
        REQUIRE(vp.ClusterCount() == 1);
        // (a) the per-part path of CalculatePhysicsStats over float Rigidbody states read from the part transforms
        v->CalculatePhysicsStats([&](Part& p) {
            PartRigidbodyState s = PartRigidbodyState::FromPart(p);
            s.mass = vp.PartMass(p);
            s.inertiaTensor = vp.PartInertia(p);
            return s;
        });
        const double totalA = v->totalMass;
        const Vector3d comA = v->CoMD;
        const Vector3f moiA = v->MOI, localA = v->localCoM;
        // (b) the double-frame path: Vessel::env.readPhysicsState = VesselPhysics::ReadState
        const VesselPhysicsState st = vp.ReadState();
        CHECK(SameBits(st.totalMass, totalA));
        CHECK(SameBits(st.CoMD, comA));
        CHECK(SameBits(st.MOI, moiA));
        CHECK(st.rb_velocityD.IsZero());
        CHECK(st.angularVelocityD.IsZero());
        v->MOI = Vector3f();
        Vessel::env.readPhysicsState = [&](Vessel&) { return std::optional<VesselPhysicsState>(vp.ReadState()); };
        v->CalculatePhysicsStats();
        Vessel::env.readPhysicsState = nullptr;
        CHECK(SameBits(v->totalMass, totalA));
        CHECK(SameBits(v->CoMD, comA));
        CHECK(SameBits(v->MOI, moiA));
        CHECK(SameBits(v->localCoM, localA));
        // the solver body: rb.mass sum and centre of mass in KSP's order equal the physicsMass ones (rb.mass == physicsMass here)
        bool same = true;
        for (Part* p : v->parts) same = same && static_cast<double>(vp.PartMass(*p)) == p->fields.physicsMass;
        CHECK(same);
        CHECK(SameBits(vp.ClusterMass(0), totalA));
        CHECK(SameBits(vp.ClusterCenterOfMass(0), comA));
        // UpdateMassStats is the WP-14 port: physicsMass sums to GetTotalMass for an all-FULL vessel
        double sum = 0;
        for (Part* p : v->parts) sum += p->fields.physicsMass;
        CHECK(sum == doctest::Approx(v->GetTotalMass()).epsilon(1e-6));
    }
}

namespace {
// Composite linear momentum and angular momentum (about `about`) of every cluster of a binding
void Momentum(const VesselPhysics& vp, const Vector3d& about, Vector3d& P, Vector3d& L) {
    P = Vector3d();
    L = Vector3d();
    for (size_t c = 0; c < vp.ClusterCount(); ++c) {
        const double m = vp.ClusterMass(c);
        const Vector3d v = vp.ClusterVelocity(c), w = vp.ClusterAngularVelocity(c), x = vp.ClusterCenterOfMass(c);
        double I[3][3];
        vp.ClusterInertia(c, I);
        P += v * m;
        L += Vector3d(I[0][0] * w.x + I[0][1] * w.y + I[0][2] * w.z, I[1][0] * w.x + I[1][1] * w.y + I[1][2] * w.z,
                      I[2][0] * w.x + I[2][1] * w.y + I[2][2] * w.z) +
             Vector3d::Cross(x - about, v * m);
    }
}
Vector3d CompositeCom(const VesselPhysics& vp) {
    Vector3d s;
    double m = 0;
    for (size_t c = 0; c < vp.ClusterCount(); ++c) {
        s += vp.ClusterCenterOfMass(c) * vp.ClusterMass(c);
        m += vp.ClusterMass(c);
    }
    return s * (1.0 / m);
}
// I_total about the composite centre of mass (sum of cluster tensors + parallel-axis terms)
void CompositeInertia(const VesselPhysics& vp, double out[3][3]) {
    const Vector3d C = CompositeCom(vp);
    for (int a = 0; a < 3; ++a)
        for (int b = 0; b < 3; ++b) out[a][b] = 0.0;
    for (size_t c = 0; c < vp.ClusterCount(); ++c) {
        double I[3][3];
        vp.ClusterInertia(c, I);
        const Vector3d d = vp.ClusterCenterOfMass(c) - C;
        const double m = vp.ClusterMass(c), dd[3] = {d.x, d.y, d.z}, d2 = d.x * d.x + d.y * d.y + d.z * d.z;
        for (int a = 0; a < 3; ++a)
            for (int b = 0; b < 3; ++b) out[a][b] += I[a][b] + m * ((a == b ? d2 : 0.0) - dd[a] * dd[b]);
    }
}
double Det3(const double M[3][3]) {
    return M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) - M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0]) +
           M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]);
}
Vector3d Solve3(const double A[3][3], const Vector3d& b) {  // Cramer's rule
    const double det = Det3(A);
    double r[3];
    for (int k = 0; k < 3; ++k) {
        double M[3][3];
        for (int a = 0; a < 3; ++a)
            for (int c = 0; c < 3; ++c) M[a][c] = A[a][c];
        M[0][k] = b.x;
        M[1][k] = b.y;
        M[2][k] = b.z;
        r[k] = Det3(M) / det;
    }
    return {r[0], r[1], r[2]};
}
void StepAll(VesselPhysics& vp, PhysicsWorld& world, float dt) {
    vp.Flush(dt);
    world.Step(dt);
    vp.AfterStep();
}
}  // namespace

TEST_CASE("WP-20b dynamics: a craft split into two welded clusters translates and rotates as the analytic rigid body") {
    if (!GameAvailable()) return;
    World w;
    for (int policy = 0; policy < 2; ++policy) {
        INFO(std::string(policy == 0 ? "one cluster" : "two clusters (the Mainsail joint an articulation point)"));
        auto v = w.Craft("PhysProf-S");
        Part* mainsail = FindPart(*v, "liquidEngineMainsail.v2");
        REQUIRE(mainsail != nullptr);
        VesselPhysicsOptions opt;
        if (policy == 1) opt.isArticulation = [mainsail](const PartJointSpec& j) { return j.child == mainsail; };
        PhysicsWorld world;
        VesselPhysics vp(world, *v, VesselPose{}, opt);
        for (Part* p : v->parts) vp.SetAngularDrag(*p, 0.0f);  // FlightIntegrator in vacuum (FlightIntegrator.cs:2276)
        CHECK(vp.ClusterCount() == static_cast<size_t>(policy + 1));
        CHECK(vp.ConstraintCount() == static_cast<size_t>(policy));
        double M = 0;
        for (size_t c = 0; c < vp.ClusterCount(); ++c) M += vp.ClusterMass(c);
        const Vector3d C0 = CompositeCom(vp);
        double I[3][3];
        CompositeInertia(vp, I);
        // translation: F through the composite centre of mass for n ticks; semi-implicit Euler: v = a dt n, x = a dt^2 n (n + 1) / 2
        const Vector3d F(0.0, 0.0, 1000.0);
        const int n = 50;
        const float dt = 0.02f;
        for (int k = 0; k < n; ++k) {
            vp.AddForceAtPhysicsPoint(*v->rootPart, F, CompositeCom(vp));
            StepAll(vp, world, dt);
        }
        const double a = F.z / M, h = static_cast<double>(dt);
        const Vector3d C1 = CompositeCom(vp);
        Vector3d P, L;
        Momentum(vp, C1, P, L);
        CHECK((C1 - C0).z == doctest::Approx(a * h * h * n * (n + 1) / 2.0).epsilon(1e-4));
        CHECK(std::fabs((C1 - C0).x) < 1e-5);
        CHECK(std::fabs((C1 - C0).y) < 1e-5);
        CHECK(P.z / M == doctest::Approx(a * h * n).epsilon(1e-5));
        for (size_t c = 0; c < vp.ClusterCount(); ++c) {
            CHECK(vp.ClusterVelocity(c).z == doctest::Approx(a * h * n).epsilon(1e-3));  // welded: every cluster moves alike
            CHECK(vp.ClusterAngularVelocity(c).magnitude() < 1e-4);
        }
        // rotation: a pure torque about the transverse X axis; omega = I^-1 T t
        vp.SetLinearVelocity(Vector3d());
        const Vector3d T(500.0, 0.0, 0.0);
        const int m = 25;
        for (int k = 0; k < m; ++k) {
            vp.AddTorque(*v->rootPart, T);
            StepAll(vp, world, dt);
        }
        const Vector3d alpha = Solve3(I, T);
        const double wx = alpha.x * h * m;
        for (size_t c = 0; c < vp.ClusterCount(); ++c) {
            const Vector3d wc = vp.ClusterAngularVelocity(c);
            INFO("cluster " << c << " w " << wc.x << " " << wc.y << " " << wc.z << " expected x " << wx);
            CHECK(wc.x == doctest::Approx(wx).epsilon(policy == 0 ? 1e-4 : 5e-3));
            CHECK(std::fabs(wc.y) < 5e-3 * std::fabs(wx));
            CHECK(std::fabs(wc.z) < 5e-3 * std::fabs(wx));
        }
        if (policy == 1) {  // the weld holds the relative pose: the Mainsail stays where the stack puts it
            const PartPhysicsState ms = vp.PartState(*mainsail), root = vp.PartState(*v->rootPart);
            const Vector3d rel = QuaternionD::Inverse(root.rotation) * (ms.position - root.position);
            CHECK((rel - Vector3d(mainsail->fields.orgPos)).magnitude() < 5e-3);
        }
    }
}

TEST_CASE("WP-20b decouple: Cut rebuilds the clusters with the linear and angular momentum of their parts; ejection forces keep the total momentum") {
    if (!GameAvailable()) return;
    World w;
    auto v = w.Craft("PhysProf-S");
    PhysicsWorld world;
    VesselPhysics vp(world, *v, VesselPose{}, {});
    REQUIRE(vp.ClusterCount() == 1);
    // a translating, tumbling vessel
    vp.SetLinearVelocity(Vector3d(3.0, -2.0, 120.0));
    world.SetAngularVelocity(vp.ClusterBody(0), Vector3d(0.3, 0.05, -0.2));
    vp.AfterStep();
    const Vector3d C = CompositeCom(vp);
    Vector3d P0, L0;
    Momentum(vp, C, P0, L0);
    std::vector<Part*> decouplers;
    for (Part* p : v->parts)
        if (p->isAnchoredDecoupler() || p->isDecoupler()) decouplers.push_back(p);
    REQUIRE(decouplers.size() == 2);
    CHECK(vp.Cut(decouplers) == 3);  // core stack and two boosters
    CHECK((CompositeCom(vp) - C).magnitude() < 1e-9);
    Vector3d P1, L1;
    Momentum(vp, C, P1, L1);
    INFO("P0 " << P0.x << " " << P0.y << " " << P0.z << " | P1 " << P1.x << " " << P1.y << " " << P1.z);
    INFO("L0 " << L0.x << " " << L0.y << " " << L0.z << " | L1 " << L1.x << " " << L1.y << " " << L1.z);
    CHECK((P1 - P0).magnitude() < 1e-6 * P0.magnitude());  // the solver's velocities are float
    CHECK((L1 - L0).magnitude() < 1e-6 * L0.magnitude());
    int cut = 0;
    for (size_t j = 0; j < vp.Joints().size(); ++j) cut += vp.JointCut(j) ? 1 : 0;
    CHECK(cut == 2);
    // the decoupler's ejection force on the two sides (part.AddForce(+-F): each at its rigid body's centre of mass): internal forces,
    // the momentum stays P1
    const Vector3d F(0.0, 0.0, 2500.0);
    for (int k = 0; k < 25; ++k) {
        if (k == 0)
            for (Part* d : decouplers) {
                vp.AddForce(*d, F);
                vp.AddForce(*d->parent, Vector3d() - F);
            }
        StepAll(vp, world, 0.02f);
    }
    Vector3d P2, L2;
    Momentum(vp, CompositeCom(vp), P2, L2);
    CHECK((P2 - P1).magnitude() < 1e-5 * P1.magnitude());
    CHECK(vp.ClusterCount() == 3);
}

// The spike's numbers (core/spikes/jolt/results/scaling_ascent_pinned.md, reproduced by the spike binary of this branch:
// `jolt_spike bench scaling --models reduced-whole,articulated`, PhysProf-S with `--ticks 400`): altitude / speed of the vessel CoM
// after 30 s (100 ticks of settle, then full thrust), and PhysProf-S after 8 s (the S tumbles at Mach 1 in every model and diverges
// after ~10 s, PHYSICS-SPIKE.md 2.2). Tolerance: the spike's own `ascent_rigid` gate, 0.5 % of altitude and speed.
//   * with the spike's cluster damping (AngularDragModel::ClusterMassAverage) the binding is the spike's reduced-whole model:
//     compared with the spike's reduced-whole numbers;
//   * with KSP's per-part angular drag (the default) it is compared with the spike's ARTICULATED model (one Jolt body per part,
//     PartJoint springs, 50 steps: the PhysX-equivalent reference), which damps every part body on its own as PhysX does.
TEST_CASE("WP-20b regression: the spike's ascents (S 8 s, M2-AS and L2-AS 30 s) on the production binding stay within the spike's 0.5 %") {
    if (!GameAvailable()) return;
    struct Ref {
        const char* craft;
        int ticks;
        double reducedAltitude, reducedSpeed, articulatedAltitude, articulatedSpeed;
    };
    const Ref refs[] = {{"PhysProf-S", 400, 870.172, 262.358, 870.093, 262.273},
                        {"PhysProf-M2-AS", 1500, 4921.85, 332.952, 4909.24, 330.207},
                        {"PhysProf-L2-AS", 1500, 3043.59, 204.875, 3044.87, 206.000}};
    World w(true);
    for (const Ref& r : refs) {
        for (const AngularDragModel model : {AngularDragModel::ClusterMassAverage, AngularDragModel::PerPart}) {
            auto v = w.Craft(r.craft);
            AscentConfig cfg;
            cfg.ticks = r.ticks;
            cfg.angularDrag = model;
            const AscentResult res = RunAscent(w, *v, cfg);
            const bool spikeModel = model == AngularDragModel::ClusterMassAverage;
            const double alt = spikeModel ? r.reducedAltitude : r.articulatedAltitude, spd = spikeModel ? r.reducedSpeed : r.articulatedSpeed;
            MESSAGE(std::string(r.craft) << std::string(spikeModel ? " (spike damping) vs spike reduced-whole" : " (per-part angular drag) vs spike articulated")
                                         << ": altitude " << res.altitudeEnd << " / " << alt << " (" << 100.0 * (res.altitudeEnd / alt - 1.0)
                                         << " %), speed " << res.speedEnd << " / " << spd << " (" << 100.0 * (res.speedEnd / spd - 1.0) << " %); tick "
                                         << res.total.mean << " ms: pipeline " << res.pipeline.mean << ", ReadState + gravity " << res.precalc.mean
                                         << ", Flush " << res.flush.mean << ", step " << res.step.mean << ", AfterStep " << res.after.mean);
            CHECK_FALSE(res.exploded);
            CHECK(res.bodies == 1);
            // vs the spike's reduced-whole model: its 0.5 %; vs the articulated model: 1 % (the spike's reduced model itself is
            // 0.26 % / 0.84 % off the articulated one on M2-AS; per-part damping brings that to 0.17 % / 0.51 %)
            const double tol = spikeModel ? 0.005 : 0.01;
            CHECK(std::fabs(res.altitudeEnd / alt - 1.0) < tol);
            CHECK(std::fabs(res.speedEnd / spd - 1.0) < tol);
        }
    }
}

TEST_CASE("WP-20b determinism: the same ascent is bit-identical run to run and at 1, 2, 4 and 8 threads") {
    if (!GameAvailable()) return;
    World w(true);
    uint64_t hash0 = 0;
    Vector3d com0;
    int run = 0;
    for (int threads : {1, 1, 2, 4, 8}) {
        auto v = w.Craft("PhysProf-M2-AS");
        AscentConfig cfg;
        cfg.ticks = 300;
        cfg.threads = threads;
        const AscentResult res = RunAscent(w, *v, cfg);
        if (run++ == 0) {
            hash0 = res.stateHash;
            com0 = res.comEnd;
        }
        INFO("threads " << threads);
        CHECK(res.stateHash == hash0);
        CHECK(SameBits(res.comEnd, com0));
    }
    // the split clustering too (17 bodies, 16 fixed constraints)
    hash0 = 0;
    run = 0;
    for (int threads : {1, 4}) {
        auto v = w.Craft("PhysProf-M2-AS");
        AscentConfig cfg;
        cfg.ticks = 200;
        cfg.threads = threads;
        cfg.policy = ClusterPolicy::SplitAtDecouplers;
        const AscentResult res = RunAscent(w, *v, cfg);
        CHECK(res.bodies == 17);
        CHECK(res.constraints == 16);
        if (run++ == 0) hash0 = res.stateHash;
        CHECK(res.stateHash == hash0);
    }
}

// A vessel resting on the pad for 10 s (5 s settle, then 10 s measured; engines off, no aero), the spike's `precision` bench:
// the spike measured PhysProf-S 0 / 0 and L2-AS 0.12 m/s / 0.043 m (contact jitter of a 680 t body on 49 engine bells, on its 900
// terrain quads). Bounds: S and M2-AS 1e-4 m / 1e-4 m/s, L2-AS 0.05 m / 0.15 m/s.
TEST_CASE("WP-20b scenario: a vessel at rest on the pad for 10 s does not drift") {
    if (!GameAvailable()) return;
    World w(true);
    struct Bound {
        const char* craft;
        double drift, speed;
    };
    for (const Bound& b : {Bound{"PhysProf-S", 1e-4, 1e-4}, Bound{"PhysProf-M2-AS", 1e-4, 1e-4}, Bound{"PhysProf-L2-AS", 0.05, 0.15}}) {
        auto v = w.Craft(b.craft);
        AscentConfig cfg;
        cfg.ticks = 750;  // the spike's precision bench: 5 s settle (the 5 cm drop and the gravity easing), then 10 s measured
        cfg.settleTicks = 250;
        cfg.throttle = 0.0f;
        cfg.aero = false;
        const AscentResult res = RunAscent(w, *v, cfg);
        MESSAGE(std::string(b.craft) << " on the pad: max CoM drift " << res.maxDriftAfterSettle << " m, max speed " << res.maxSpeedAfterSettle << " m/s over 10 s");
        CHECK(res.maxDriftAfterSettle < b.drift);
        CHECK(res.maxSpeedAfterSettle < b.speed);
    }
}
