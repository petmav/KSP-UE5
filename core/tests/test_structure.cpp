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

// ===================================================================================== autostruts: the Heaviest walk and the cycle timing

TEST_CASE("WP-20c autostruts: Heaviest anchors through GetMassivePart (argmax of rb.mass over the tree, ties to the nearer part, the cache)") {
    if (!GameAvailable()) return;
    World w;
    auto v = w.Craft("PhysProf-S");
    v->UpdateMassStats();
    Part* mainsail = FindPart(*v, "liquidEngineMainsail.v2");
    Part* tank = FindPart(*v, "Rockomax32.BW");
    REQUIRE(mainsail->parent == tank);
    std::vector<Part*> engines;
    for (Part* p : v->parts)
        if (p->partInfo->name == "liquidEngine2") engines.push_back(p);
    REQUIRE(engines.size() == 2);
    engines[0]->fields.mass += 0.25f;  // break the symmetric tie of the two booster engines
    v->UpdateMassStats();
    // (1) every part Heaviest: its anchor is the part of largest rb.mass other than itself and, with autoStrutExcludeParent (true by
    // default, Part.cs:829), its parent (no free pivot: the walk reaches every part)
    int checked = 0;
    for (Part* self : v->parts) {
        self->fields.autoStrutMode = 2;
        Part* best = nullptr;
        float bestMass = -1.0f, second = -1.0f;
        for (Part* p : v->parts) {
            if (p == self || (self->fields.autoStrutExcludeParent && p == self->parent)) continue;
            const float m = PartRigidbodyMass(*p);
            if (m > bestMass) {
                second = bestMass;
                bestMass = m;
                best = p;
            } else if (m > second) {
                second = m;
            }
        }
        REQUIRE(bestMass > second);  // a unique heaviest part
        Part* got = GetAutoStrutAnchor(*v, *self);
        INFO(self->partInfo->name << ": anchor " << (got ? got->partInfo->name : std::string("none")) << ", heaviest " << best->partInfo->name);
        CHECK(got == best);
        ++checked;
    }
    CHECK(checked == 21);
    // autoStrutExcludeParent on the Mainsail: its parent, the Rockomax tank, is the heaviest part
    mainsail->fields.autoStrutExcludeParent = false;
    CHECK(GetAutoStrutAnchor(*v, *mainsail) == tank);
    mainsail->fields.autoStrutExcludeParent = true;
    CHECK(GetAutoStrutAnchor(*v, *mainsail) == engines[0]);
    // (2) a tie (Mathf.Approximately) goes to the part nearer to the strutted one, whatever the walk order (Part.cs:8459-8488): with
    // the tank, the Mainsail and the probe made light, the two equal booster engines are the heaviest; each booster's nose cone picks
    // its own booster's engine
    engines[0]->fields.mass -= 0.25f;
    for (size_t k = 0; k < tank->Resources.size(); ++k) tank->Resources[k].amount = 0.0;
    tank->fields.mass = 0.1f;
    mainsail->fields.mass = 0.1f;
    v->UpdateMassStats();
    REQUIRE(PartRigidbodyMass(*engines[0]) == PartRigidbodyMass(*engines[1]));
    auto boosterOf = [](const Part* p) {  // the radial decoupler a booster part hangs from
        while (p->parent != nullptr && !const_cast<Part*>(p)->isAnchoredDecoupler() && !const_cast<Part*>(p)->isDecoupler()) p = p->parent;
        return p;
    };
    int cones = 0;
    for (Part* p : v->parts) {
        if (p->partInfo->name != "pointyNoseConeB") continue;
        p->fields.autoStrutMode = 2;
        Part* own = boosterOf(engines[0]) == boosterOf(p) ? engines[0] : engines[1];
        REQUIRE(boosterOf(own) == boosterOf(p));
        CHECK(GetAutoStrutAnchor(*v, *p) == own);
        ++cones;
    }
    CHECK(cones == 2);
    // (3) the per-part cache (Part.cs:8349-8376): a second walk in the same fixed step starts from the last highest mass with no
    // highestPart, and MassivePartCheck's tie test dereferences it: the C# throws (the SecureAutoStruts coroutine dies)
    Part* probe = v->rootPart;
    probe->fields.autoStrutMode = 2;
    const double t1 = 100.0, t2 = 101.0;
    Part* first = GetAutoStrutAnchor(*v, *probe, nullptr, &t1);
    REQUIRE(first != nullptr);
    CHECK_THROWS_AS(GetAutoStrutAnchor(*v, *probe, nullptr, &t1), NullReferenceException);
    CHECK(GetAutoStrutAnchor(*v, *probe, nullptr, &t2) == first);  // the next step: a fresh walk
    // BuildAutoStruts: one autostrut per Heaviest part (no symmetry counterparts in the craft); a thrown walk drops that part's autostrut
    const size_t all = BuildAutoStruts(*v).size();
    CHECK(all == v->parts.size());
    const double t3 = 102.0;
    (void)GetAutoStrutAnchor(*v, *probe, nullptr, &t3);
    CHECK(BuildAutoStruts(*v, {}, nullptr, &t3).size() == all - 1);
}

TEST_CASE("WP-20c autostruts: CycleAutoStrut timing - none at build, secured after the first step, released by onVesselWasModified for one step") {
    if (!GameAvailable()) return;
    World w;
    auto v = w.Craft("PhysProf-M2-AS");
    PhysicsWorld world;
    VesselPhysicsHost host(world);
    host.Install();
    VesselPhysics& vp = host.Create(*v, VesselPose{});
    const size_t expected = BuildAutoStruts(*v).size();
    CHECK(expected == 152);
    CHECK(vp.AutoStrutJointCount() == 0);
    CHECK(vp.AutoStrutsPending());
    host.Step(0.02f);
    CHECK(vp.AutoStrutJointCount() == expected);
    events::GameEvents::onVesselWasModified.Fire(v.get());  // Part.VesselModified -> CycleAutoStrut of every part
    VesselPhysics* same = host.Find(*v);
    REQUIRE(same == &vp);  // the parts did not change: no rebind
    CHECK_FALSE(host.StructuralChangesPending());
    CHECK(vp.AutoStrutJointCount() == 0);
    host.Step(0.02f);
    CHECK(vp.AutoStrutJointCount() == expected);
    host.Uninstall();
}

// ===================================================================================== analytic breaks

namespace {

// The break acceleration of a payload riding on one joint (the joint the base's push goes through), from the closed form of the elastic
// split (translation and rotation decouple on a centred ring): F = m a shared equally, the moment M = m (x_p - c) x a against the ring's
// rotational stiffness K = sum kl (|r|^2 I - r r^T) + n ka I; per sub-joint f_i = F / n + kl th x r_i, t = ka th, th = K^-1 M. Returns the
// acceleration magnitude at which the first sub-joint reaches breakForce or breakTorque (PartJoint.cs:340-341).
double AnalyticBreakAcceleration(const PartJointSpec& j, const PartPhysicsState& child, double payloadMass, const Vector3d& payloadCom,
                                 const Vector3d& dir, double kLinear) {
    std::vector<Vector3d> anchors;
    Vector3d c;
    for (int s = 0; s < j.internalJoints; ++s) {
        anchors.push_back(child.position + child.rotation * Vector3d(j.anchors[static_cast<size_t>(s)]));
        c += anchors.back();
    }
    c = c * (1.0 / static_cast<double>(anchors.size()));
    const double ka = (j.rigid || j.forceRigid) ? kLinear : static_cast<double>(j.stiffness);
    const double n = static_cast<double>(anchors.size());
    double K[3][3] = {{n * ka, 0, 0}, {0, n * ka, 0}, {0, 0, n * ka}};
    for (const Vector3d& a : anchors) {
        const Vector3d r = a - c;
        const double rv[3] = {r.x, r.y, r.z}, rr = r.x * r.x + r.y * r.y + r.z * r.z;
        for (int p = 0; p < 3; ++p)
            for (int q = 0; q < 3; ++q) K[p][q] += kLinear * ((p == q ? rr : 0.0) - rv[p] * rv[q]);
    }
    const Vector3d F = dir * payloadMass;  // per unit acceleration
    const Vector3d M = Vector3d::Cross(payloadCom - c, F);
    auto det3 = [](const double A[3][3]) {
        return A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1]) - A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0]) +
               A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
    };
    const double D = det3(K);
    double th[3];
    const double m[3] = {M.x, M.y, M.z};
    for (int k = 0; k < 3; ++k) {  // th = K^-1 M (Cramer)
        double A[3][3];
        for (int p = 0; p < 3; ++p)
            for (int q = 0; q < 3; ++q) A[p][q] = q == k ? m[p] : K[p][q];
        th[k] = det3(A) / D;
    }
    const Vector3d theta(th[0], th[1], th[2]);
    double ratio = 0.0;
    for (const Vector3d& a : anchors) {
        const Vector3d f = F * (1.0 / n) + Vector3d::Cross(theta, a - c) * kLinear;
        ratio = std::max({ratio, f.magnitude() / static_cast<double>(j.breakForce), (theta * ka).magnitude() / static_cast<double>(j.breakTorque)});
    }
    return 1.0 / ratio;
}

struct RampResult {
    int breakTick = -1;
    double aBreak = 0.0, aPrev = 0.0;
    std::vector<VesselPhysics::JointBreak> breaks;
    LoadSolve solve = LoadSolve::None;
};

// Pushes `base` with F = M a(t) through the composite centre of mass (a pure translation), a(t) = rate t, until a sub-joint breaks.
RampResult RunPushRamp(VesselPhysics& vp, PhysicsWorld& world, Part& base, const Vector3d& dir, double rate, int maxTicks) {
    RampResult r;
    const float dt = 0.02f;
    double M = 0.0;
    for (size_t c = 0; c < vp.ClusterCount(); ++c) M += vp.ClusterMass(c);
    for (int k = 1; k <= maxTicks; ++k) {
        const double a = rate * k * static_cast<double>(dt);
        vp.AddForceAtPhysicsPoint(base, dir * (M * a), CompositeCom({&vp}));
        Step(vp, world, dt);
        if (!vp.Breaks().empty()) {
            r.breakTick = k;
            r.aBreak = a;
            r.aPrev = rate * (k - 1) * static_cast<double>(dt);
            r.breaks = vp.Breaks();
            r.solve = vp.LastSolve();
            return r;
        }
    }
    return r;
}

}  // namespace

TEST_CASE("WP-20c capacity: a payload on a size-0 / 1 / 2 node breaks at 1.00-1.05x of the analytic load, axially and laterally, not below") {
    if (!GameAvailable()) return;
    World w;
    int runs = 0;
    for (int size : {0, 1, 2}) {
        for (int lateral = 0; lateral < 2; ++lateral) {
            INFO("node size " << size << std::string(lateral ? " lateral" : " axial"));
            auto v = w.CraftHead("PhysProf-S", 2);  // the probe core (root, the payload) on the Rockomax tank (the base)
            REQUIRE(v->parts.size() == 2);
            Part* probe = v->rootPart;
            Part* tank = v->parts[1];
            for (Part* p : v->parts)
                for (AttachNode& n : p->attachNodes) n.size = size;
            PhysicsWorld world;
            VesselPhysicsOptions opt;
            opt.autoStrutDelaySteps = 0;
            VesselPhysics vp(world, *v, VesselPose{}, opt);
            for (Part* p : v->parts) vp.SetAngularDrag(*p, 0.0f);
            REQUIRE(vp.Joints().size() == 1);
            const PartJointSpec& j = vp.Joints()[0];
            CHECK(j.internalJoints == (size >= 2 ? 3 : 1));
            const Vector3d dir = lateral ? Vector3d(1, 0, 0) : Vector3d(0, 1, 0);
            const double aStar = AnalyticBreakAcceleration(j, vp.PartState(*tank), vp.PartMass(*probe), vp.PartState(*probe).centerOfMass, dir, opt.kLinear);
            const double rate = aStar / (200.0 * 0.02);  // a* reached at tick 200
            const RampResult r = RunPushRamp(vp, world, *tank, dir, rate, 400);
            MESSAGE("size " << size << std::string(lateral ? " lateral" : " axial") << ": sub-joints " << j.internalJoints << ", breakForce " << j.breakForce
                            << " kN, analytic a* " << aStar << " m/s^2, broke at tick " << r.breakTick << " a " << r.aBreak << " (" << r.aBreak / aStar << " a*)");
            REQUIRE(r.breakTick > 0);
            CHECK(r.breaks.size() == 1);
            CHECK(r.breaks[0].joint == 0);
            CHECK(r.solve == LoadSolve::Tree);
            CHECK(r.aBreak >= aStar * (1.0 - 1e-6));  // not below the capacity
            CHECK(r.aPrev < aStar * (1.0 + 1e-6));    // and at the first tick above it
            CHECK(r.aBreak <= 1.05 * aStar);
            ++runs;
        }
    }
    CHECK(runs == 6);
}

TEST_CASE("WP-20c strut sharing: a 3-part stack with and without a Grandparent autostrut breaks the joint and at the load the elastic sharing predicts") {
    if (!GameAvailable()) return;
    World w;
    for (int drained = 0; drained < 2; ++drained) {
        for (int strut = 0; strut < 2; ++strut) {
            INFO(std::string(drained ? "empty" : "full") << " tank, " << std::string(strut ? "with" : "without") << " the autostrut");
            auto v = w.CraftHead("PhysProf-S", 3);  // probe (root, top) - Rockomax tank - Mainsail (pushed)
            REQUIRE(v->parts.size() == 3);
            Part* probe = v->parts[0];
            Part* tank = v->parts[1];
            Part* engine = v->parts[2];
            REQUIRE(engine->parent == tank);
            REQUIRE(tank->parent == probe);
            if (drained)
                for (size_t k = 0; k < tank->Resources.size(); ++k) tank->Resources[k].amount = 0.0;
            engine->fields.autoStrutMode = strut ? 3 : 0;  // Grandparent: the probe core
            PhysicsWorld world;
            VesselPhysicsOptions opt;
            VesselPhysics vp(world, *v, VesselPose{}, opt);
            for (Part* p : v->parts) vp.SetAngularDrag(*p, 0.0f);
            const PartJointSpec* jT = nullptr;  // tank -> probe
            const PartJointSpec* jE = nullptr;  // Mainsail -> tank
            for (const PartJointSpec& j : vp.Joints()) {
                if (j.child == tank) jT = &j;
                if (j.child == engine) jE = &j;
            }
            REQUIRE(jT != nullptr);
            REQUIRE(jE != nullptr);
            const double nT = jT->internalJoints, nE = jE->internalJoints;
            const double fP = vp.PartMass(*probe), fT = vp.PartMass(*tank);  // demands per unit acceleration (axial)
            // per-sub-joint axial loads per unit acceleration: without the strut the tree; with it the 2-node elastic system
            // [[nT + 1, -nT], [-nT, nT + nE]] (u_P, u_T) = (fP, fT) (units kLinear, u_E = 0): strut u_P, Mainsail joint u_T, tank joint u_P - u_T
            double perT = 0, perE = 0;
            if (!strut) {
                perT = fP / nT;
                perE = (fP + fT) / nE;
            } else {
                const double a11 = nT + 1, a12 = -nT, a22 = nT + nE, det = a11 * a22 - a12 * a12;
                const double uP = (a22 * fP - a12 * fT) / det, uT = (a11 * fT - a12 * fP) / det;
                perT = std::fabs(uP - uT);
                perE = std::fabs(uT);
            }
            const double ratioT = perT / jT->breakForce, ratioE = perE / jE->breakForce;
            const double aStar = 1.0 / std::max(ratioT, ratioE);
            const Part* expectChild = ratioT > ratioE ? tank : engine;
            const RampResult r = RunPushRamp(vp, world, *engine, Vector3d(0, 1, 0), aStar / (200.0 * 0.02), 400);
            MESSAGE(std::string(drained ? "empty" : "full") << " tank, " << std::string(strut ? "with" : "without") << " autostrut: per-sub-joint load per m/s^2 tank joint " << perT
                                                 << " / Mainsail joint " << perE << " -> " << expectChild->partInfo->name << " joint first at a* " << aStar
                                                 << "; broke at " << r.aBreak << " (" << r.aBreak / aStar << " a*), solve "
                                                 << std::string(r.solve == LoadSolve::Graph ? "graph" : "tree"));
            REQUIRE(r.breakTick > 0);
            REQUIRE(r.breaks.size() == 1);
            CHECK(vp.Joints()[r.breaks[0].joint].child == expectChild);
            CHECK(r.solve == (strut ? LoadSolve::Graph : LoadSolve::Tree));
            CHECK(vp.AutoStrutJointCount() == (strut ? 1u : 0u));
            CHECK(r.aBreak >= aStar * (1.0 - 1e-4));
            CHECK(r.aPrev < aStar * (1.0 + 1e-4));
            CHECK(r.aBreak <= 1.05 * aStar);
        }
    }
}

// ===================================================================================== ground contacts

TEST_CASE("WP-20c PhysicsWorld::StaticContacts: every touching sub-shape of a compound body, not only the first of a merged manifold") {
    // A compound of three boxes on a static floor: two resting side by side (user data 1 and 2: Jolt's manifold reduction merges their
    // manifolds, same normal, and reports only one sub-shape in OnContactAdded), one 0.5 m up (3: no support). Gravity as a force.
    std::vector<PhysicsWorld::StaticContact> first;
    for (int threads : {1, 4}) {
        PhysicsWorldConfig cfg;
        cfg.threads = threads;
        PhysicsWorld world(cfg);
        world.SetStaticContactTracking(true);
        BodyDesc floor;
        floor.motion = MotionType::Static;
        floor.shape = ShapeDesc::Box(Vector3f(20.0f, 0.5f, 20.0f));
        floor.position = Vector3d(0.0, -0.5, 0.0);
        const BodyId ground = world.CreateBody(floor);
        REQUIRE(ground != kInvalidBody);
        CompoundBodyDesc d;
        for (int k = 0; k < 3; ++k) {
            CompoundChild ch;
            ch.shape = ShapeDesc::Box(Vector3f(0.5f, 0.5f, 0.5f));
            ch.position = k < 2 ? Vector3f(k == 0 ? -1.5f : 1.5f, 0.0f, 0.0f) : Vector3f(0.0f, 1.0f, 0.0f);
            ch.userData = static_cast<uint32_t>(k + 1);
            d.children.push_back(ch);
        }
        d.position = Vector3d(0.0, 0.5, 0.0);  // the lower boxes' bottoms on the floor
        d.mass = 3.0;
        const BodyId body = world.CreateCompoundBody(d);
        REQUIRE(body != kInvalidBody);
        for (int s = 0; s < 50; ++s) {
            world.AddForce(body, Vector3d(0.0, -9.81 * 3.0, 0.0));
            world.Step(0.02f);
        }
        const std::vector<PhysicsWorld::StaticContact>& c = world.StaticContacts();
        INFO("threads " << threads);
        REQUIRE(c.size() == 2);
        CHECK(c[0].body == body);
        CHECK(c[0].subShapeUserData == 1u);
        CHECK(c[1].subShapeUserData == 2u);
        if (threads == 1) first = c;
        else CHECK(c == first);
        world.SetStaticContactTracking(false);
        world.Step(0.02f);
        CHECK(world.StaticContacts().empty());
    }
}
