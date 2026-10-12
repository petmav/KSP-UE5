// WP-20c unit tests: the structural layer of the reduced rigid-body model (ksp/physics/structure_graph.hpp, ksp/physics/vessel_physics.hpp).
//
// No golden or capture covers joint loads or breaking (PhysX's joint solve is not reproducible), so the gate is:
//   * the load model against an independent numpy reference (tools/wp20c/loads_reference.py: explicit subtree sums, dense global solve);
//   * the PartJoint capacity table of the six PhysProf crafts against an independent reference that reads the IN-GAME part database
//     capture (tools/wp20c/capacity_reference.py), bit for bit;
//   * analytic break accelerations: a payload on a size-0 / 1 / 2 node pushed axially and laterally, a 3-part stack with and without an
//     autostrut (closed-form elastic sharing), each must break between 1.00x and 1.05x of the predicted load and not below;
//   * scenarios: a gravity ramp on the pad breaks the joint the capacity rule predicts at the predicted load (and PhysProf-S where the
//     spike broke it, the radial decoupler at 33.7 g); decouple / undock / couple / part death through the game events keep the momentum;
//     staging every decoupler of L2-AS in one frame; joint breaks through PartJoint.OnJointBreak -> Part.decouple; determinism.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <set>

#include "doctest/doctest.h"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "ksp/physics/structure_graph.hpp"
#include "nlohmann/json.hpp"
#include "vessel_physics_fixture.hpp"

using namespace ksp;
using namespace ksp::physics;
using namespace wp20b;
using json = nlohmann::json;

namespace {

Vector3d V(const json& a) { return {a[0].get<double>(), a[1].get<double>(), a[2].get<double>()}; }
double RelErr(const Vector3d& a, const Vector3d& b, double scale) { return (a - b).magnitude() / std::max(scale, 1e-300); }
uint32_t FloatBits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, sizeof u);
    return u;
}
uint32_t HexBits(const json& j) { return static_cast<uint32_t>(std::stoul(j.get<std::string>(), nullptr, 16)); }

Part* FindPart(Vessel& v, const std::string& name) {
    for (Part* p : v.parts)
        if (p->partInfo != nullptr && p->partInfo->name == name) return p;
    return nullptr;
}

json LoadJson(const std::string& file) {
    std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/" + file, std::ios::binary);
    REQUIRE(f.good());
    return json::parse(f);
}

// composite centre of mass / linear and angular momentum (about `about`) of a set of bindings
Vector3d CompositeCom(const std::vector<const VesselPhysics*>& vps) {
    Vector3d s;
    double m = 0;
    for (const VesselPhysics* vp : vps)
        for (size_t c = 0; c < vp->ClusterCount(); ++c) {
            s += vp->ClusterCenterOfMass(c) * vp->ClusterMass(c);
            m += vp->ClusterMass(c);
        }
    return s * (1.0 / m);
}
double TotalMass(const std::vector<const VesselPhysics*>& vps) {
    double m = 0;
    for (const VesselPhysics* vp : vps)
        for (size_t c = 0; c < vp->ClusterCount(); ++c) m += vp->ClusterMass(c);
    return m;
}
void Momentum(const std::vector<const VesselPhysics*>& vps, const Vector3d& about, Vector3d& P, Vector3d& L) {
    P = Vector3d();
    L = Vector3d();
    for (const VesselPhysics* vp : vps)
        for (size_t c = 0; c < vp->ClusterCount(); ++c) {
            const double m = vp->ClusterMass(c);
            const Vector3d v = vp->ClusterVelocity(c), w = vp->ClusterAngularVelocity(c), x = vp->ClusterCenterOfMass(c);
            double I[3][3];
            vp->ClusterInertia(c, I);
            P += v * m;
            L += Vector3d(I[0][0] * w.x + I[0][1] * w.y + I[0][2] * w.z, I[1][0] * w.x + I[1][1] * w.y + I[1][2] * w.z,
                          I[2][0] * w.x + I[2][1] * w.y + I[2][2] * w.z) +
                 Vector3d::Cross(x - about, v * m);
        }
}
std::vector<const VesselPhysics*> All(const VesselPhysicsHost& host, const std::vector<Vessel*>& vessels) {
    std::vector<const VesselPhysics*> out;
    for (Vessel* v : vessels)
        if (const VesselPhysics* vp = host.Find(*v)) out.push_back(vp);
    return out;
}

void Step(VesselPhysics& vp, PhysicsWorld& world, float dt) {
    vp.Flush(dt);
    world.Step(dt);
    vp.AfterStep();
}

}  // namespace

// ===================================================================================== the load model vs the numpy reference

TEST_CASE("WP-20c load model: demands, tree solve, sub-joint split and graph solve equal the independent numpy reference (26 structures)") {
    const json ref = LoadJson("wp20c_loads_reference.json");
    int cases = 0, treeJoints = 0, graphElements = 0;
    double worstDemand = 0, worstTree = 0, worstSplit = 0, worstGraph = 0, worstTreeVsGraph = 0;
    for (const json& c : ref["cases"]) {
        INFO(c["name"].get<std::string>());
        const size_t n = c["nodes"].size();
        std::vector<Vector3d> com(n);
        std::vector<double> mass(n);
        std::vector<NodeDemand> balanced(n);
        const json& kin = c["kin"];
        for (size_t k = 0; k < n; ++k) {
            const json& nd = c["nodes"][k];
            com[k] = V(nd["com"]);
            mass[k] = nd["mass"].get<double>();
            double I[3][3];
            for (int a = 0; a < 3; ++a)
                for (int b = 0; b < 3; ++b) I[a][b] = nd["I"][a][b].get<double>();
            // (1) the demand of a rigid part moving with its cluster
            const NodeDemand d = RigidPartDemand(mass[k], com[k], I, V(kin["com"]), V(kin["a"]), V(kin["alpha"]), V(kin["w"]), V(kin["g"]), V(nd["F"]),
                                                 V(nd["M"]));
            const Vector3d rf = V(nd["demand"]["force"]), rm = V(nd["demand"]["moment"]);
            worstDemand = std::max({worstDemand, RelErr(d.force, rf, rf.magnitude() + 1.0), RelErr(d.moment, rm, rm.magnitude() + 1.0)});
            balanced[k].force = V(nd["balanced"]["force"]);
            balanced[k].moment = V(nd["balanced"]["moment"]);
        }
        std::vector<StructElement> elements;
        for (const json& e : c["elements"]) {
            StructElement el;
            el.a = e["a"].get<int>();
            el.b = e["b"].get<int>();
            el.anchor = V(e["anchor"]);
            el.kLinear = e["kl"].get<double>();
            el.kAngular = e["ka"].get<double>();
            el.joint = e["joint"].get<int>();
            el.sub = e["sub"].get<int>();
            elements.push_back(el);
        }
        // (2) the graph solve
        StructuralGraph g;
        REQUIRE(g.Build(com, mass, elements));
        std::vector<ElementLoad> loads;
        g.Solve(balanced, loads);
        double scale = 0;
        for (const json& gl : c["graph"]) scale = std::max(scale, V(gl["force"]).magnitude());
        for (size_t e = 0; e < elements.size(); ++e) {
            worstGraph = std::max({worstGraph, RelErr(loads[e].force, V(c["graph"][e]["force"]), scale),
                                   RelErr(loads[e].torque, V(c["graph"][e]["torque"]), scale)});
            ++graphElements;
        }
        // (3) the tree solve and the per-joint split on the tree joints
        std::vector<int> parentNode = c["parentNode"].get<std::vector<int>>(), parentJoint = c["parentJoint"].get<std::vector<int>>();
        std::vector<Vector3d> centroids;
        for (const json& j : c["joints"]) centroids.push_back(V(j["centroid"]));
        std::vector<JointResultant> res;
        TreeSolve(parentNode, parentJoint, com, balanced, centroids, res);
        for (size_t j = 0; j < c["joints"].size(); ++j) {
            const json& jt = c["joints"][j];
            if (!jt["tree"].get<bool>()) continue;
            const Vector3d F = V(jt["resultant"]["force"]), M = V(jt["resultant"]["moment"]);
            worstTree = std::max({worstTree, RelErr(res[j].force, F, F.magnitude() + 1.0), RelErr(res[j].moment, M, M.magnitude() + 1.0)});
            std::vector<Vector3d> rel;
            std::vector<double> kl, ka;
            for (const json& e : jt["elements"]) {
                const StructElement& el = elements[e.get<size_t>()];
                rel.push_back(el.anchor - centroids[j]);
                kl.push_back(el.kLinear);
                ka.push_back(el.kAngular);
            }
            JointSplit sp;
            REQUIRE(sp.Setup(rel, kl, ka));
            std::vector<ElementLoad> parts(rel.size());
            sp.Apply(res[j].force, res[j].moment, parts.data());
            for (size_t k = 0; k < rel.size(); ++k) {
                const size_t e = jt["elements"][k].get<size_t>();
                worstSplit = std::max({worstSplit, RelErr(parts[k].force, V(c["split"][e]["force"]), scale),
                                       RelErr(parts[k].torque, V(c["split"][e]["torque"]), scale)});
                if (c["loops"].get<int>() == 0 && c["supports"].get<int>() == 0)  // a tree: the graph solve is the tree split
                    worstTreeVsGraph = std::max(worstTreeVsGraph, RelErr(parts[k].force, loads[e].force, scale));
            }
            ++treeJoints;
        }
        ++cases;
    }
    MESSAGE(cases << " structures, " << treeJoints << " tree joints, " << graphElements << " graph elements; worst relative differences: demand "
                  << worstDemand << ", tree resultant " << worstTree << ", split " << worstSplit << ", graph " << worstGraph << ", tree split vs graph (C++) "
                  << worstTreeVsGraph);
    CHECK(cases == 26);
    CHECK(worstDemand < 1e-12);
    CHECK(worstTree < 1e-10);
    CHECK(worstSplit < 1e-8);
    CHECK(worstGraph < 1e-6);
    CHECK(worstTreeVsGraph < 1e-6);
}

TEST_CASE("WP-20c load model: closed forms (a single joint, a ground support, a 3-sub-joint ring under torsion and bending)") {
    // one element between two nodes: it carries the whole demand; force on b = the demand of b
    {
        StructuralGraph g;
        std::vector<StructElement> el(1);
        el[0].a = 0;
        el[0].b = 1;
        el[0].anchor = Vector3d(0, 1, 0);
        el[0].kLinear = 1e8;
        el[0].kAngular = 6e4;
        REQUIRE(g.Build({Vector3d(0, 0, 0), Vector3d(0, 2, 0)}, {10.0, 1.0}, el));  // node 0 (heavier) fixed
        CHECK(g.NodeFixed(0));
        CHECK_FALSE(g.NodeFixed(1));
        std::vector<ElementLoad> out;
        g.Solve({NodeDemand{Vector3d(-3, 0, 0), Vector3d(1, 2, 0)}, NodeDemand{Vector3d(3, 0, 0), Vector3d(0, 0, 5)}}, out);
        CHECK(out[0].force.x == doctest::Approx(3.0).epsilon(1e-9));
        // the moment about the anchor: the node's moment plus (x_b - anchor) x F = (0, 0, 5) + (0, 1, 0) x (3, 0, 0) = (0, 0, 2)
        CHECK(out[0].torque.z == doctest::Approx(2.0).epsilon(1e-9));
        CHECK(std::fabs(out[0].torque.x) < 1e-9);
    }
    // a ground support: the ground carries the node's demand
    {
        StructuralGraph g;
        StructElement s;
        s.a = StructElement::kGround;
        s.b = 0;
        s.anchor = Vector3d(5, 6, 7);
        s.kLinear = 1e8;
        s.kAngular = 1e8;
        REQUIRE(g.Build({Vector3d(5, 6, 7)}, {1.0}, {s}));
        CHECK_FALSE(g.NodeFixed(0));
        std::vector<ElementLoad> out;
        g.Solve({NodeDemand{Vector3d(0, 9.81, 0), Vector3d(1, 0, 0)}}, out);
        CHECK(out[0].force.y == doctest::Approx(9.81).epsilon(1e-12));
        CHECK(out[0].torque.x == doctest::Approx(1.0).epsilon(1e-12));
    }
    // a ring of 3 sub-joints (radius R, axis y): axial force F -> F/3 each; torsion T about y -> tangential T / (3 R) each (+ 3 drives);
    // bending M about x -> axial forces kl th d_i with th = M / (1.5 kl R^2 + 3 ka)
    {
        const double R = 1.0, kl = 1e8, ka = 9e4;
        std::vector<Vector3d> rel;
        for (int i = 0; i < 3; ++i) {
            const double ang = 2.0 * 3.14159265358979323846 * i / 3.0;
            rel.emplace_back(R * std::cos(ang), 0.0, R * std::sin(ang));
        }
        JointSplit sp;
        REQUIRE(sp.Setup(rel, {kl, kl, kl}, {ka, ka, ka}));
        ElementLoad out[3];
        sp.Apply(Vector3d(0, 300, 0), Vector3d(), out);
        for (const ElementLoad& e : out) CHECK(e.force.y == doctest::Approx(100.0).epsilon(1e-12));
        const double T = 900.0;
        sp.Apply(Vector3d(), Vector3d(0, T, 0), out);
        const double thT = T / (3.0 * kl * R * R + 3.0 * ka);
        for (const ElementLoad& e : out) {
            CHECK(e.force.magnitude() == doctest::Approx(kl * thT * R).epsilon(1e-9));
            CHECK(e.torque.y == doctest::Approx(ka * thT).epsilon(1e-9));
        }
        const double Mb = 1200.0;
        sp.Apply(Vector3d(), Vector3d(Mb, 0, 0), out);
        const double thB = Mb / (1.5 * kl * R * R + 3.0 * ka);
        for (int i = 0; i < 3; ++i) CHECK(out[i].force.y == doctest::Approx(-kl * thB * rel[static_cast<size_t>(i)].z).epsilon(1e-9).scale(1.0));
    }
}

// ===================================================================================== the capacity table vs the in-game part database

TEST_CASE("WP-20c PartJoint capacity table of the six PhysProf crafts == the reference from the in-game part database (bit for bit)") {
    if (!GameAvailable()) return;
    const json ref = LoadJson("wp20c_capacity_reference.json");
    World w;
    CHECK(PhysicsGlobals::JointBreakForceFactor() == ref["physics"]["jointBreakForceFactor"].get<float>());
    int joints = 0, subJoints = 0;
    for (const auto& [craft, recs] : ref["crafts"].items()) {
        INFO(craft);
        auto v = w.Craft(craft);
        for (int pass = 0; pass < 2; ++pass) {  // the craft's rigidAttachment, then every part forced rigid
            if (pass == 1)
                for (Part* p : v->parts) p->fields.rigidAttachment = true;
            const std::vector<PartJointSpec> specs = BuildAttachJoints(*v);
            REQUIRE(specs.size() == recs.size());
            std::map<uint32_t, const PartJointSpec*> byChild;
            for (const PartJointSpec& s : specs) byChild[s.child->fields.persistentId] = &s;
            for (const json& r : recs) {
                const auto it = byChild.find(r["child"].get<uint32_t>());
                REQUIRE(it != byChild.end());
                const PartJointSpec& s = *it->second;
                INFO(r["childName"].get<std::string>() << " -> " << r["parent"].get<uint32_t>());
                CHECK(s.parent->fields.persistentId == r["parent"].get<uint32_t>());
                CHECK(s.nodeSize == r["size"].get<int>());
                CHECK(s.mode == r["mode"].get<int>());
                CHECK(s.internalJoints == r["internalJoints"].get<int>());
                CHECK(s.rigid == r["rigid"].get<bool>());
                CHECK(FloatBits(s.stiffness) == HexBits(r["stiffness"]));
                CHECK(FloatBits(s.breakingForce) == HexBits(r["breakingForce"]));
                CHECK(FloatBits(s.breakingTorque) == HexBits(r["breakingTorque"]));
                CHECK(FloatBits(s.breakForce) == HexBits(r[pass == 0 ? "breakForce" : "breakForceRigid"]));
                CHECK(FloatBits(s.breakTorque) == HexBits(r[pass == 0 ? "breakTorque" : "breakTorqueRigid"]));
                if (pass == 0) {
                    ++joints;
                    subJoints += s.internalJoints;
                }
            }
        }
    }
    MESSAGE(joints << " attach joints (" << subJoints << " sub-joints) of 6 crafts compared bit for bit, twice (craft rigidAttachment / forced)");
    CHECK(joints == 20 + 20 + 152 + 152 + 456 + 456);
    CHECK(subJoints == 24 + 24 + 168 + 168 + 504 + 504);
}
