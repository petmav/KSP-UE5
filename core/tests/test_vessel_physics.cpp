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
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/aero/lifting_surface.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/modules/module_rcs.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/orbit/time_warp.hpp"
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

namespace {
// The flight-scene globals tests/flight_fixture.hpp sets for VesselPrecalculate (TimeWarp, Planetarium time, the frame flags), on top of
// a wp20b::World. Restores them on exit.
struct FlightGlobalsScope {
    TimeWarp tw;
    TimeWarpEnvironment twEnv;
    explicit FlightGlobalsScope(World& w) {
        w.pg.CalculateValues(w.sys->Bodies(), w.planetarium.Sun);
        for (CelestialBody* b : w.sys->Bodies()) b->inverseRotation = false;
        twEnv.LoadedSceneValue = GameScenes::FLIGHT;
        TimeWarp::environment = &twEnv;
        FlightGlobals::currentMainBody = &w.Kerbin();
        w.planetarium.fixedDeltaTime = 0.02;
        w.planetarium.timeScale = 1.0;
        w.planetarium.time = 1000.0;
        Time::fixedDeltaTime = 0.02f;
        TimeWarp::fetch = &tw;
        tw.warpRates = {1.0f, 5.0f, 10.0f, 50.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f};
        tw.physicsWarpRates = {1.0f, 2.0f, 3.0f, 4.0f};
        tw.maxPhysicsRate_index = 3;
        tw.Mode = TimeWarp::Modes::HIGH;
        tw.curr_rate = 1.0f;
        w.planetarium.UpdateCBs();
    }
    ~FlightGlobalsScope() {
        TimeWarp::fetch = nullptr;
        TimeWarp::environment = nullptr;
        Time::fixedDeltaTime = 0.02f;
        GameSettings::PHYSICS_EASE = true;
        GameSettings::ORBIT_DRIFT_COMPENSATION = true;
        FlightGlobals::ready = false;
        FlightGlobals::ActiveVessel = nullptr;
    }
};

// A real craft vessel unpacked on a circular orbit of the body: the orbit driver in TRACK_Phys (FlightEnv::MakeVessel's recipe)
void PutInOrbit(Vessel& v, CelestialBody& body, double sma) {
    v.orbitDriver->orbit = std::make_shared<Orbit>(0.0, 0.0, sma, 0.0, 0.0, 0.0, Planetarium::GetUniversalTime(), &body);
    v.orbitDriver->updateMode = OrbitDriver::UpdateMode::TRACK_Phys;
    v.orbitDriver->Start();
    v.orbitDriver->orbit = std::make_shared<Orbit>(0.0, 0.0, sma, 0.0, 0.0, 0.0, Planetarium::GetUniversalTime(), &body);
    v.lastBody = &body;
    v.Landed = false;
    v.Splashed = false;
    v.situation = Vessel::Situations::ORBITING;
    v.packed = false;
    v.loaded = true;
    v.staticPressurekPa = 0.0;
}

// the vessel-frame centre of mass of a vessel (rb.mass weights, KSP's rb.worldCenterOfMass of every part with a Rigidbody)
Vector3d VesselFrameCom(Vessel& v) {
    Vector3d s;
    double m = 0;
    for (Part* p : v.parts) {
        if (!p->hasRigidbody) continue;
        const double pm = PartRigidbodyMass(*p);
        s += Vector3d(p->fields.orgPos + p->fields.orgRot * p->fields.CoMOffset) * pm;
        m += pm;
    }
    return s * (1.0 / m);
}

struct CoastResult {
    double maxErr = 0, maxRadiusErr = 0, maxFrameSpeed = 0;
    int ticks = 0, corrections = 0;
    size_t recentres = 0;
};

// physics_orbit_hold (tests/test_flight3.cpp RunOrbitHold) on the reduced-model bodies: VesselPrecalculate.FixedUpdate (CalculatePhysicsStats
// through readPhysicsState, gravity, drift compensation), FlightIntegrator.Integrate (OffsetVelocity, integrationAccel through
// IntegratePartBuffers), Krakensbane (the Galilean frame velocity), the Jolt step, FloatingOrigin, VesselPrecalculate.Update. The error is
// the vessel CoM against the Kepler orbit the vessel started on.
CoastResult RunCoast(World& w, double orbits, bool krakensbane) {
    FlightGlobalsScope scope(w);
    CelestialBody& kerbin = w.Kerbin();
    for (CelestialBody* b : w.sys->Bodies())
        if (b->orbitDriver) b->orbitDriver->reverse = false;
    kerbin.orbitDriver->reverse = true;
    GameSettings::PHYSICS_EASE = false;
    GameSettings::ORBIT_DRIFT_COMPENSATION = true;
    Vessel::env.frameCount = 1000;
    const double r = kerbin.Radius + 80000.0;
    const double ut0 = Planetarium::GetUniversalTime();
    Orbit ref(0.0, 0.0, r, 0.0, 0.0, 0.0, ut0, &kerbin);
    Vector3d p0, v0;
    ref.GetOrbitalStateVectorsAtUT(ut0, p0, v0);
    auto v = w.Craft("PhysProf-S");
    PutInOrbit(*v, kerbin, r);
    FlightGlobals::ActiveVessel = v.get();
    FlightGlobals::ready = true;
    PhysicsWorld world;
    VesselPhysicsHost host(world);
    host.frameOriginWorld = [&kerbin](const Vessel&) { return kerbin.position; };
    host.Install();
    Krakensbane kb;
    FloatingOrigin fo;
    if (krakensbane) {
        REQUIRE(kb.Awake());
        fo.Awake();
        fo.ready = [] { return true; };
        kb.host.snapshot = [&] {
            FrameSnapshot s;
            s.ready = true;
            s.hasActiveVessel = true;
            s.activeHasRigidbody = true;
            s.active.isActive = true;
            s.active.packed = v->packed;
            s.active.radarAltitude = (v->CoMD - kerbin.position).magnitude() - kerbin.Radius;
            s.velocityD = v->velocityD;
            s.rb_velocityD = v->rb_velocityD;
            s.upAxis = (v->CoMD - kerbin.position).normalized();
            s.loaded.push_back(s.active);
            return s;
        };
        kb.host.changeWorldVelocity = [&](size_t, const Vector3d& dv) {
            if (VesselPhysics* p = host.Find(*v)) p->ChangeWorldVelocity(dv);  // Vessel.ChangeWorldVelocity (packed: nothing)
        };
        // Vessel.GoOffRails of the active vessel: Krakensbane.AddFrameVelocity(orbit velocity) before the parts unpack (Vessel.cs:2440),
        // then Part.ResumeVelocity sets rb.velocity = orbit velocity - frame velocity (Part.cs:3827): the orbital speed stays a double
        v->CoMD = kerbin.position + p0.xzy();  // the packed state (VesselPrecalculate's rails CoMD) the snapshot reads
        Krakensbane::AddFrameVelocity(v0.xzy());
        REQUIRE(SameBits(Krakensbane::GetFrameVelocity(), v0.xzy()));
    }
    // the vessel transform where its centre of mass is on the orbit (physics frame = Kerbin's inertial frame, origin at its centre)
    VesselPose pose;
    pose.position = p0.xzy() - VesselFrameCom(*v);
    VesselPhysics& vp = host.Create(*v, pose);
    vp.SetLinearVelocity(v0.xzy() - Krakensbane::GetFrameVelocity());
    vp.AfterStep();
    for (Part* p : v->parts) vp.SetAngularDrag(*p, 0.0f);  // FlightIntegrator in vacuum
    const double dt = static_cast<double>(TimeWarp::fixedDeltaTime());  // 0.02f: the step of the solver, Krakensbane and the UT alike
    CoastResult res;
    res.ticks = static_cast<int>(orbits * ref.period / dt);
    for (int k = 1; k <= res.ticks; k++) {
        w.planetarium.time += dt;
        v->precalc.FixedUpdate();
        if (v->precalc.wasCorrectingDrift) res.corrections++;
        if (!v->precalc.preIntegrationVelocityOffset.IsZero()) v->OffsetVelocity(v->precalc.preIntegrationVelocityOffset);  // FlightIntegrator.cs:665
        host.IntegrateAll();  // integrationAccel on every Rigidbody, the part force buffers
        if (krakensbane) {
            kb.FixedUpdate();
            res.maxFrameSpeed = std::max(res.maxFrameSpeed, vp.ClusterVelocity(0).magnitude());
        }
        host.Step(static_cast<float>(dt));
        if (krakensbane) fo.FixedUpdate();
        v->precalc.Update();
        Vector3d rp, rv;
        ref.GetOrbitalStateVectorsAtUT(Planetarium::GetUniversalTime(), rp, rv);
        const Vector3d x = v->CoMD - kerbin.position;
        res.maxErr = std::max(res.maxErr, (x - rp.xzy()).magnitude());
        res.maxRadiusErr = std::max(res.maxRadiusErr, std::fabs(x.magnitude() - r));
    }
    host.Uninstall();
    if (krakensbane) {
        kb.OnDestroy();
        fo.OnDestroy();
    }
    return res;
}
}  // namespace

// WP-19a's physics_orbit_hold (6.4 mm over 10 orbits with an exact double single-body integrator) with the vessel's real bodies: a
// 21-part craft as one Jolt body, its state read through Vessel::env.readPhysicsState, the drift correction written through
// offsetVelocity, gravity through IntegratePartBuffers. Jolt keeps velocities in float: with Krakensbane's frame velocity (KSP's rule:
// the whole rb velocity moves into FrameVel every tick) the physics-frame velocity stays at a few m/s; without it the 2,279 m/s orbital
// velocity has a 2.4e-4 m/s float ulp whose rounding the drift compensation refits into the orbit every tick.
TEST_CASE("WP-20b scenario: 80 km orbit coast on the reduced-model bodies through VesselPrecalculate (WP-19a physics_orbit_hold)") {
    if (!GameAvailable()) return;
    World w;
#ifdef NDEBUG
    const double orbits = 1.0;
#else
    const double orbits = 0.1;  // the Debug build is ten times slower
#endif
    const CoastResult kb = RunCoast(w, orbits, true);
    const CoastResult plain = RunCoast(w, orbits, false);
    MESSAGE(orbits << " orbit(s), " << kb.ticks << " ticks: with Krakensbane max position error " << kb.maxErr << " m (radius " << kb.maxRadiusErr
                   << " m, physics-frame speed <= " << kb.maxFrameSpeed << " m/s, " << kb.corrections << " drift corrections); without "
                   << plain.maxErr << " m (radius " << plain.maxRadiusErr << " m)");
    // measured (RelWithDebInfo, one orbit): 1.4 mm / 0.33 mm with Krakensbane (WP-19a's exact double single body: 3 mm per orbit), 5.2 m /
    // 1.4 m without it (float velocities at orbital speed)
    CHECK(kb.corrections >= kb.ticks - 1);
    CHECK(kb.maxErr < 0.01);
    CHECK(kb.maxRadiusErr < 0.005);
    CHECK(kb.maxFrameSpeed < 1.0);
    CHECK(plain.maxErr < 20.0);
    CHECK(kb.maxErr * 100.0 < plain.maxErr);
}

TEST_CASE("WP-20b host: GoOffRails builds the bodies at the rails state (CoM within 1e-6 m, orbit velocity), the seams read them, GoOnRails removes them") {
    if (!GameAvailable()) return;
    World w;
    FlightGlobalsScope scope(w);
    CelestialBody& kerbin = w.Kerbin();
    const double r = kerbin.Radius + 120000.0;
    auto v = w.Craft("PhysProf-S");
    PutInOrbit(*v, kerbin, r);
    v->packed = true;
    Vessel::env.frameCount = 1000;  // Vessel.PhysicsHoldExpired
    FlightGlobals::ActiveVessel = v.get();
    FlightGlobals::ready = true;
    // the rails state: CoMD on the orbit (VesselPrecalculate's packed branch), localCoM from the vessel frame
    Vector3d p0, v0;
    v->orbit().GetOrbitalStateVectorsAtUT(Planetarium::GetUniversalTime(), p0, v0);
    v->CoMD = kerbin.position + p0.xzy();
    v->localCoM = VesselFrameCom(*v).ToVector3f();
    PhysicsWorld world;
    VesselPhysicsHost host(world);
    host.frameOriginWorld = [&kerbin](const Vessel&) { return kerbin.position; };
    host.Install();
    REQUIRE(host.Find(*v) == nullptr);
    v->GoOffRails();
    CHECK_FALSE(v->packed);
    VesselPhysics* vp = host.Find(*v);
    REQUIRE(vp != nullptr);
    CHECK(vp->ClusterCount() == 1);
    CHECK(world.BodyCount() == 1);
    // the bodies stand where the rails put the vessel: CoM (rb.mass weights) at CoMD
    const Vector3d com = vp->WorldFromPhysics(vp->ClusterCenterOfMass(0));
    CHECK((com - v->CoMD).magnitude() < 1e-6);
    // Part.ResumeVelocity: the orbit velocity (no rotating frame, no frame velocity), float in the solver
    CHECK((vp->ClusterVelocity(0) - v->orbit().GetVel(&kerbin)).magnitude() < 1e-3);
    // CalculatePhysicsStats now reads the bodies through readPhysicsState
    v->CalculatePhysicsStats();
    CHECK((v->CoMD - com).magnitude() < 1e-6);
    CHECK((v->rb_velocityD - vp->ClusterVelocity(0)).magnitude() < 1e-9);
    // partRigidbody: the float Rigidbody of a part; rootRigidbody: the point velocity at the CoM
    const PartRigidbodyState rs = Vessel::env.partRigidbody(*v->rootPart);
    CHECK(rs.present);
    CHECK(rs.mass == vp->PartMass(*v->rootPart));
    Vector3f pv, av;
    REQUIRE(Vessel::env.rootRigidbody(*v, v->CoMD.ToVector3f(), pv, av));
    CHECK((Vector3d(pv) - vp->ClusterVelocity(0)).magnitude() < 1e-2);
    // Vessel.OffsetVelocity -> every body
    const Vector3d before = vp->ClusterVelocity(0);
    v->OffsetVelocity(Vector3d(0.5, 0.0, -0.25));
    CHECK((vp->ClusterVelocity(0) - (before + Vector3d(0.5, 0.0, -0.25))).magnitude() < 1e-3);
    // GoOnRails: Part.Pack -> the bodies go
    v->GoOnRails();
    CHECK(v->packed);
    CHECK(host.Find(*v) == nullptr);
    CHECK(world.BodyCount() == 0);
    host.Uninstall();
    CHECK_FALSE(static_cast<bool>(Vessel::env.readPhysicsState));
}

TEST_CASE("WP-20b mass update: rb.mass changes move the centre of mass inside the body, the parts keep their velocities") {
    if (!GameAvailable()) return;
    World w;
    auto v = w.Craft("PhysProf-S");
    PhysicsWorld world;
    VesselPhysics vp(world, *v, VesselPose{}, {});
    vp.SetLinearVelocity(Vector3d(1.0, 2.0, 3.0));
    world.SetAngularVelocity(vp.ClusterBody(0), Vector3d(0.0, 0.0, 0.4));
    vp.AfterStep();
    const std::vector<PartPhysicsState> before = vp.PartStates();
    CHECK(vp.UpdateMassProperties() == 0);  // nothing changed
    // drain half of every resource of the Rockomax tank (FlightIntegrator.UpdateMassStats -> physicsMass -> rb.mass)
    Part* tank = FindPart(*v, "Rockomax32.BW");
    REQUIRE(tank != nullptr);
    const double massBefore = vp.ClusterMass(0);
    const float tankBefore = vp.PartMass(*tank);
    for (size_t k = 0; k < tank->Resources.size(); ++k) tank->Resources[k].amount *= 0.5;
    v->UpdateMassStats();
    CHECK(vp.UpdateMassProperties() == 1);
    CHECK(vp.PartMass(*tank) < tankBefore);
    CHECK(vp.ClusterMass(0) == doctest::Approx(massBefore - (tankBefore - vp.PartMass(*tank))).epsilon(1e-9));
    // the new centre of mass = the rb.mass-weighted mean of the parts' centres of mass; its velocity = the parts' momentum / mass
    Vector3d s, P;
    double m = 0;
    for (size_t i = 0; i < v->parts.size(); ++i) {
        const double pm = vp.PartMass(*v->parts[i]);
        s += before[i].centerOfMass * pm;
        P += before[i].velocity * pm;
        m += pm;
    }
    CHECK((vp.ClusterCenterOfMass(0) - s * (1.0 / m)).magnitude() < 1e-6);
    CHECK((vp.ClusterVelocity(0) - P * (1.0 / m)).magnitude() < 1e-5);
    // the parts did not move, the solver's mass is the new one
    const std::vector<PartPhysicsState> after = vp.PartStates();
    for (size_t i = 0; i < after.size(); ++i) CHECK((after[i].position - before[i].position).magnitude() < 1e-9);
    CHECK(world.GetMass(vp.ClusterBody(0)) == doctest::Approx(vp.ClusterMass(0)).epsilon(1e-6));
    // the inertia equals a fresh build from the same part states
    PhysicsWorld world2;
    VesselPhysics fresh(world2, *v, vp.PartStates(), {});
    double I1[3][3], I2[3][3];
    vp.ClusterInertia(0, I1);
    fresh.ClusterInertia(0, I2);
    for (int a = 0; a < 3; ++a)
        for (int b = 0; b < 3; ++b) CHECK(I1[a][b] == doctest::Approx(I2[a][b]).epsilon(1e-9).scale(1.0));
}

TEST_CASE("WP-20b forces: Part.AddForce* semantics, FlightIntegrator.Integrate's buffers, ForceMode, the module force records") {
    if (!GameAvailable()) return;
    World w;
    auto v = w.Craft("PhysProf-S");
    Part* cone = FindPart(*v, "pointyNoseConeB");
    REQUIRE(cone != nullptr);
    cone->fields.PhysicsSignificance = 1;  // a physicsless part: its forces go to its RigidBodyPart
    StartPartRigidbodies(*v);
    v->UpdateMassStats();
    PhysicsWorld world;
    VesselPhysics vp(world, *v, VesselPose{}, {});
    for (Part* p : v->parts) vp.SetAngularDrag(*p, 0.0f);
    const double M = vp.ClusterMass(0);
    double I[3][3];
    vp.ClusterInertia(0, I);
    const Vector3d c = vp.ClusterCenterOfMass(0);
    const float dt = 0.02f;
    const double h = static_cast<double>(dt);
    // (1) Part.AddForce on the physicsless cone: applied at its RigidBodyPart's centre of mass
    const Vector3d F(10.0, 0.0, 0.0);
    vp.AddForce(*cone, F);
    const Vector3d ownerCom = vp.PartState(*cone->parent).centerOfMass;
    CHECK(vp.PartExternalForces()[static_cast<size_t>(vp.PartIndex(*cone->parent))].x == 10.0);
    StepAll(vp, world, dt);
    CHECK(vp.ClusterVelocity(0).x == doctest::Approx(F.x / M * h).epsilon(1e-5));
    const Vector3d alpha = Solve3(I, Vector3d::Cross(ownerCom - c, F));
    const Vector3d w1 = vp.ClusterAngularVelocity(0);
    CHECK(w1.z == doctest::Approx(alpha.z * h).epsilon(1e-4));
    // (2) FlightIntegrator.Integrate: integrationAccel on every Rigidbody, part.force / torque / forces consumed and cleared
    vp.SetLinearVelocity(Vector3d());
    world.SetAngularVelocity(vp.ClusterBody(0), Vector3d());
    vp.AfterStep();
    Part* tank = FindPart(*v, "Rockomax32.BW");
    tank->fields.force = Vector3d(0.0, 50.0, 0.0);
    tank->fields.torque = Vector3d(0.0, 3.0, 0.0);
    const Vector3f tankPos = tank->partTransform.position;
    tank->forces.push_back(Part_ForceHolder(Vector3d(0.0, 0.0, 7.0), Vector3d(tankPos)));
    cone->forces.push_back(Part_ForceHolder(Vector3d(1.0, 0.0, 0.0), Vector3d(tankPos)));  // a physicsless part's buffer is dropped (Integrate :735)
    vp.IntegratePartBuffers(Vector3d(0.0, -9.0, 0.0));
    CHECK(tank->forces.empty());
    CHECK(tank->fields.force.IsZero());
    CHECK(tank->fields.torque.IsZero());
    CHECK(cone->forces.empty());
    StepAll(vp, world, dt);
    const Vector3d vNow = vp.ClusterVelocity(0);
    CHECK(vNow.x == doctest::Approx(0.0).epsilon(1e-9).scale(1e-6));
    CHECK(vNow.y == doctest::Approx((50.0 / M - 9.0) * h).epsilon(1e-5));
    CHECK(vNow.z == doctest::Approx(7.0 / M * h).epsilon(1e-5));
    // (3) ForceMode: Acceleration scales by the RigidBodyPart's rb.mass, Impulse / VelocityChange spread over the tick
    vp.SetLinearVelocity(Vector3d());
    world.SetAngularVelocity(vp.ClusterBody(0), Vector3d());
    vp.AfterStep();
    const double mt = vp.PartMass(*tank);
    const Vector3d tc = vp.PartState(*tank).centerOfMass;
    vp.AddRigidbodyForceAtPosition(*tank, Vector3d(2.0, 0.0, 0.0), Vector3d(vp.TransformFromPhysics(tc)), ForceMode::Acceleration);
    vp.AddRigidbodyForce(*tank, Vector3d(0.0, 0.0, 0.1), ForceMode::VelocityChange);
    vp.AddImpulse(*tank, Vector3d(0.0, 0.4, 0.0));
    StepAll(vp, world, dt);
    CHECK(vp.ClusterVelocity(0).x == doctest::Approx(2.0 * mt / M * h).epsilon(1e-4));
    CHECK(vp.ClusterVelocity(0).z == doctest::Approx(0.1 * mt / M).epsilon(1e-4));
    CHECK(vp.ClusterVelocity(0).y == doctest::Approx(0.4 / M).epsilon(1e-4));
    // (4) the module records: aero drag / lift with rb.angularDrag, engine thrust, RCS, lifting surface
    AeroForces af;
    af.setAngularDrag = true;
    af.angularDrag = 0.75f;
    af.applyDrag = true;
    af.dragForce = Vector3f(-1.0f, 0.0f, 0.0f);
    af.dragPosition = tank->partTransform.position;
    af.mode = AeroForceMode::Acceleration;
    ApplyAeroForces(vp, *tank, af);
    CHECK(vp.AngularDrag(*tank) == 0.75f);
    ApplyAeroForces(vp, *cone, af);  // part.rb == null: the drag goes to part.Rigidbody, rb.angularDrag is not touched
    CHECK(vp.AngularDrag(*cone) == 0.05f);
    const size_t ti = static_cast<size_t>(vp.PartIndex(*tank));
    const size_t oi = static_cast<size_t>(vp.PartIndex(*cone->parent));
    CHECK(vp.PartExternalForces()[ti].x == doctest::Approx(-mt));                                    // Acceleration * rb.mass
    CHECK(vp.PartExternalForces()[oi].x == doctest::Approx(-static_cast<double>(vp.PartMass(*cone->parent))));
    EngineThrustResult et;
    et.forces.push_back({Vector3f(0.0f, 100.0f, 0.0f), tank->partTransform.position});
    et.forces.push_back({Vector3f(0.0f, 50.0f, 0.0f), tank->partTransform.position});
    ApplyEngineForces(vp, *tank, et);
    CHECK(vp.PartExternalForces()[ti].y == doctest::Approx(150.0));
    ApplyRcsForces(vp, *tank, {RCSForce{Vector3f(0.0f, 0.0f, 2.0f), tank->partTransform.position}});
    SurfaceForces sf;
    sf.applied = true;
    sf.liftForce = Vector3f(0.0f, 0.0f, 3.0f);
    sf.liftPosition = tank->partTransform.position;
    sf.applyDrag = true;
    sf.dragForce = Vector3f(0.0f, 0.0f, -1.0f);
    sf.dragPosition = tank->partTransform.position;
    ApplySurfaceForces(vp, *tank, sf);
    CHECK(vp.PartExternalForces()[ti].z == doctest::Approx(4.0));
    vp.Flush(dt);
    CHECK(vp.PartExternalForces()[ti].IsZero());  // cleared by Flush
}

TEST_CASE("WP-20b vessel split and merge: Part.decouple / Part.Couple through the host keep the momentum of the parts") {
    if (!GameAvailable()) return;
    World w;
    Vessel::SpawnedVessels().clear();
    auto v = w.Craft("PhysProf-S");
    PhysicsWorld world;
    VesselPhysicsHost host(world);
    VesselPhysics& vp = host.Create(*v, VesselPose{});
    vp.SetLinearVelocity(Vector3d(0.0, 0.0, 50.0));
    world.SetAngularVelocity(vp.ClusterBody(0), Vector3d(0.05, 0.2, 0.0));
    vp.AfterStep();
    const Vector3d C = CompositeCom(vp);
    Vector3d P0, L0;
    Momentum(vp, C, P0, L0);
    const double massBefore = vp.ClusterMass(0);
    Part* d12 = nullptr;
    for (Part* p : v->parts)
        if (p->gameObjectName == "radialDecoupler1-2") d12 = p;
    REQUIRE(d12 != nullptr);
    Part* parentTank = d12->parent;
    Vessel* booster = d12->decouple();  // WP-16 Part.decouple: the decoupler and its subtree become a new Vessel
    REQUIRE(booster != nullptr);
    VesselPhysics* bp = host.OnVesselSplit(*v, *booster);
    REQUIRE(bp != nullptr);
    VesselPhysics* cp = host.Find(*v);
    REQUIRE(cp != nullptr);
    CHECK(host.Count() == 2);
    CHECK(cp->ClusterCount() == 1);
    CHECK(bp->ClusterCount() == 1);
    CHECK(world.BodyCount() == 2);
    CHECK(bp->ClusterMass(0) + cp->ClusterMass(0) == doctest::Approx(massBefore).epsilon(1e-12));
    Vector3d Pa, La, Pb, Lb;
    Momentum(*cp, C, Pa, La);
    Momentum(*bp, C, Pb, Lb);
    CHECK(((Pa + Pb) - P0).magnitude() < 1e-6 * P0.magnitude());
    CHECK(((La + Lb) - L0).magnitude() < 1e-6 * L0.magnitude());
    // a few free ticks, then dock the booster back (Part.Couple): one body again, the momentum of both kept
    for (int k = 0; k < 10; ++k) host.Step(0.02f);
    Vector3d P1a, L1a, P1b, L1b;
    const Vector3d C1 = (CompositeCom(*cp) * cp->ClusterMass(0) + CompositeCom(*bp) * bp->ClusterMass(0)) * (1.0 / (cp->ClusterMass(0) + bp->ClusterMass(0)));
    Momentum(*cp, C1, P1a, L1a);
    Momentum(*bp, C1, P1b, L1b);
    d12->Couple(parentTank);  // the booster Vessel is destroyed by Couple: the bindings were looked up before
    VesselPhysics* merged = host.OnVesselMerge(bp, cp, *v);
    REQUIRE(merged != nullptr);
    CHECK(host.Count() == 1);
    CHECK(merged->ClusterCount() == 1);
    CHECK(world.BodyCount() == 1);
    Vector3d P2, L2;
    Momentum(*merged, C1, P2, L2);
    CHECK((P2 - (P1a + P1b)).magnitude() < 1e-6 * P2.magnitude());
    CHECK((L2 - (L1a + L1b)).magnitude() < 1e-5 * L2.magnitude());
    host.DestroyAll();
    Vessel::SpawnedVessels().clear();
}

TEST_CASE("WP-20b Rebuild: a part demoted to physicsless folds into its parent's Rigidbody; the motion and the total mass are kept") {
    if (!GameAvailable()) return;
    World w;
    auto v = w.Craft("PhysProf-S");
    PhysicsWorld world;
    VesselPhysics vp(world, *v, VesselPose{}, {});
    vp.SetLinearVelocity(Vector3d(0.0, 10.0, 0.0));
    world.SetAngularVelocity(vp.ClusterBody(0), Vector3d(0.0, 0.3, 0.0));
    vp.AfterStep();
    const double mass = vp.ClusterMass(0);
    const size_t joints = vp.Joints().size();
    const Vector3d v0 = vp.ClusterVelocity(0), w0 = vp.ClusterAngularVelocity(0), c0 = vp.ClusterCenterOfMass(0);
    Part* cone = FindPart(*v, "pointyNoseConeB");
    REQUIRE(cone != nullptr);
    cone->DemoteToPhysicslessPart();  // Part.cs:2990: physicalSignificance NONE, the Rigidbody goes
    CHECK_FALSE(cone->hasRigidbody);
    vp.Rebuild();
    CHECK(vp.PartMass(*cone) == 0.0f);
    CHECK(vp.Joints().size() == joints - 1);  // the cone's attach joint is gone (only FULL parts make one)
    CHECK(vp.ClusterMass(0) == doctest::Approx(mass).epsilon(1e-9));  // its mass rides in its parent's physicsMass
    // the same rigid motion: the parts' momentum (with the cone's mass now at its parent's centre of mass, as KSP's rb.mass has it)
    // gives back the angular velocity, and the new centre of mass moves with v0 + w0 x (c1 - c0)
    const Vector3d c1 = vp.ClusterCenterOfMass(0);
    CHECK((c1 - c0).magnitude() > 1e-4);  // the cone's mass moved down to its parent's centre of mass
    CHECK((vp.ClusterAngularVelocity(0) - w0).magnitude() < 1e-6);
    CHECK((vp.ClusterVelocity(0) - (v0 + Vector3d::Cross(w0, c1 - c0))).magnitude() < 1e-5);
}
