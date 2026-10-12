// WP-20c scenario tests: the structural layer in flight-like runs (ksp/physics/vessel_physics.hpp) and its wiring to the game events.
//   * the gravity ramp on the pad (the spike's `capacity` bench): the first joint to break and the gravity at that moment are the ones the
//     capacity rule predicts from the static loads at 1 g, and PhysProf-S breaks where the spike broke it (the radial decoupler, 33.7 g);
//   * decouple / couple / undock / part death through GameEvents (onPartDeCoupleNewVesselComplete, onPartCouple, onVesselsUndocking,
//     onPartDie): the bindings follow the vessels and keep the momentum of the parts; the module force hooks;
//   * a joint break through PartJoint.OnJointBreak -> onPartJointBreak -> Part.OnPartJointBreak -> Part.decouple (and the
//     IgnoreCollisionsFrames branch that keeps the part in its vessel);
//   * staging every decoupler of PhysProf-L2-AS in one frame: one rebind, parts / mass / momentum conserved, its cost;
//   * determinism of a ramp with breaks and rebinds at 1 / 2 / 4 / 8 threads.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

#include "doctest/doctest.h"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "vessel_physics_fixture.hpp"

using namespace ksp;
using namespace ksp::physics;
using namespace wp20b;
using GE = ksp::events::GameEvents;

namespace {

Part* FindByName(Vessel& v, const std::string& name, int nth = 0) {
    for (Part* p : v.parts)
        if (p->partInfo != nullptr && p->partInfo->name == name && nth-- == 0) return p;
    return nullptr;
}

std::vector<const VesselPhysics*> Bindings(const VesselPhysicsHost& host, const std::vector<Vessel*>& vessels) {
    std::vector<const VesselPhysics*> out;
    for (Vessel* v : vessels)
        if (const VesselPhysics* vp = host.Find(*v)) out.push_back(vp);
    return out;
}
// every Vessel holding parts of the original: the original and those Part.decouple / Undock spawned
std::vector<Vessel*> LiveVessels(Vessel& original) {
    std::vector<Vessel*> out{&original};
    for (auto& s : Vessel::SpawnedVessels())
        if (!s->parts.empty()) out.push_back(s.get());
    return out;
}

struct Totals {
    double mass = 0.0;
    Vector3d com, P, L;  // L about the world origin of the physics frame
    size_t parts = 0;
};
Totals Measure(const std::vector<const VesselPhysics*>& vps) {
    Totals t;
    Vector3d s;
    for (const VesselPhysics* vp : vps) {
        t.parts += vp->PartCount();
        for (size_t c = 0; c < vp->ClusterCount(); ++c) {
            const double m = vp->ClusterMass(c);
            const Vector3d v = vp->ClusterVelocity(c), w = vp->ClusterAngularVelocity(c), x = vp->ClusterCenterOfMass(c);
            double I[3][3];
            vp->ClusterInertia(c, I);
            t.mass += m;
            s += x * m;
            t.P += v * m;
            t.L += Vector3d(I[0][0] * w.x + I[0][1] * w.y + I[0][2] * w.z, I[1][0] * w.x + I[1][1] * w.y + I[1][2] * w.z,
                            I[2][0] * w.x + I[2][1] * w.y + I[2][2] * w.z) +
                   Vector3d::Cross(x, v * m);
        }
    }
    t.com = s * (1.0 / t.mass);
    return t;
}

// counts the structural events a test expects
struct EventLog {
    int jointBreaks = 0, jointBreakReports = 0;
    float lastBreakForce = 0.0f;
    void OnPartJointBreak(PartJoint* j, float f) {
        ++jointBreaks;
        lastBreakForce = f;
        (void)j;
    }
    void OnJointBreak(EventReport*) { ++jointBreakReports; }
    EventLog() {
        GE::onPartJointBreak.Add(this, &EventLog::OnPartJointBreak);
        GE::onJointBreak.Add(this, &EventLog::OnJointBreak);
    }
    ~EventLog() {
        GE::onPartJointBreak.Remove(this, &EventLog::OnPartJointBreak);
        GE::onJointBreak.Remove(this, &EventLog::OnJointBreak);
    }
};

// ---- the gravity ramp on the pad (the spike's `capacity` bench, core/spikes/jolt/src/bench.cpp) -----------------------------------------------
struct PadRampResult {
    int firstBreakTick = -1;
    double gAtBreak = 0.0;
    std::string brokenChild;
    double predictedG = 0.0;          // 1 / (max load / capacity at 1 g, end of the settle)
    std::string predictedChild;
    LoadSolve solveAtSettle = LoadSolve::None;
    size_t supportsAtSettle = 0, factors = 0;
    double solveMs = 0.0;
    uint64_t stateHash = 0;
    size_t bindingsAtEnd = 0;
};

// The vessel on the LaunchPad of the real PQS terrain (30 x 30 quads), engines off, no aero; gravity mu / r^2 at each binding's CoM
// (+ Coriolis / centrifugal, KSP's 16-tick easing); after `settle` ticks gravity grows by `ramp` g per second. With a host the breaks are
// acted on (Part.decouple, rebinds) and the run continues `after` ticks past the first break.
PadRampResult RunPadRamp(World& w, Vessel& v, double ramp, int settle, int maxTicks, bool useHost, int threads = 1, int after = 0) {
    PadRampResult res;
    CelestialBody& kerbin = w.Kerbin();
    const Vector3d bodyPos = kerbin.position;
    const double mu = kerbin.gMagnitudeAtCenter;
    const Vector3d omega(0.0, -2.0 * 3.14159265358979323846 / kerbin.rotationPeriod, 0.0);
    const PadFrame pad = MakePadFrame(w);
    double minY = 1e30;
    for (Part* p : v.parts) {
        if (!p->hasRigidbody) continue;
        for (const ColliderProxy& b : DragCubeColliderProxy(*p))
            for (int c = 0; c < 8; ++c) {
                const Vector3f corner = b.center + Vector3f((c & 1) ? b.halfExtents.x : -b.halfExtents.x, (c & 2) ? b.halfExtents.y : -b.halfExtents.y,
                                                            (c & 4) ? b.halfExtents.z : -b.halfExtents.z);
                minY = std::min(minY, static_cast<double>(p->fields.orgPos.y) + static_cast<double>((p->fields.orgRot * corner).y));
            }
    }
    VesselPose pose;
    pose.rotation = BasisRotation(pad.east, pad.up, pad.north);
    pose.position = pad.up * (pad.radius + 0.05 - minY);
    PhysicsWorldConfig wc;
    wc.threads = threads;
    PhysicsWorld world(wc);
    AddTerrainPatch(w, world, pad, 30);
    VesselPhysicsHost host(world);
    host.frameOriginWorld = [bodyPos](const Vessel&) { return bodyPos; };
    std::unique_ptr<VesselPhysics> own;
    if (useHost) {
        host.Install();
        host.Create(v, pose);
    } else {
        own = std::make_unique<VesselPhysics>(world, v, pose);
        own->frameOriginWorld = bodyPos;
    }
    world.OptimizeBroadPhase();
    const float dt = 0.02f;
    int stopAt = maxTicks;
    for (int tick = 0; tick < stopAt; ++tick) {
        const double gScale = tick >= settle ? 1.0 + ramp * (tick - settle) * static_cast<double>(dt) : 1.0;
        std::vector<VesselPhysics*> vps;
        if (useHost) {
            host.ProcessStructuralChanges();
            for (Vessel* x : LiveVessels(v))
                if (VesselPhysics* vp = host.Find(*x)) vps.push_back(vp);
        } else {
            vps.push_back(own.get());
        }
        for (VesselPhysics* vp : vps) {
            // the scenario drops the vessel a few cm onto the pad: no breaking during the first 60 ticks (the spike's grace, after
            // Vessel.ignoreCollisionsFrames = 60 of KSP's exact landed placement, Vessel.cs:2543)
            vp->SetBreakJoints(tick >= 60);
            const VesselPhysicsState st = vp->ReadState();
            if (!(st.totalMass > 0.0)) continue;
            const Vector3d com = st.CoMD - bodyPos;
            const double rl = com.magnitude();
            Vector3d g = com * (-mu / (rl * rl * rl));
            g += Vector3d::Cross(omega, st.rb_velocityD) * -2.0;
            g += Vector3d::Cross(omega, Vector3d::Cross(omega, com * -1.0));
            vp->AddUniformAcceleration(g * (std::min(1.0, 0.0625 * (tick + 1)) * gScale));
        }
        if (useHost) {
            host.Step(dt);
        } else {
            own->Flush(dt);
            world.Step(dt);
            own->AfterStep();
        }
        if (tick == settle - 1) {  // the static loads at 1 g predict the first break
            const VesselPhysics& vp = useHost ? *host.Find(v) : *own;
            res.predictedG = 1.0 / static_cast<double>(vp.MaxLoadRatio());
            if (vp.MaxLoadJoint() >= 0) res.predictedChild = vp.Joints()[static_cast<size_t>(vp.MaxLoadJoint())].child->partInfo->name;
            res.solveAtSettle = vp.LastSolve();
            res.supportsAtSettle = vp.SupportParts().size();
            res.solveMs = vp.LastLoadSolveMs();
        }
        if (res.firstBreakTick < 0) {
            std::string child;
            if (useHost && !host.LastBreaks().empty()) child = host.LastBreaks().front().child->partInfo->name;
            if (!useHost && !own->Breaks().empty()) child = own->Joints()[own->Breaks().front().joint].child->partInfo->name;
            if (!child.empty()) {
                res.firstBreakTick = tick;
                res.gAtBreak = gScale;
                res.brokenChild = child;
                if (!useHost) break;
                stopAt = std::min(maxTicks, tick + 1 + after);
            }
        }
    }
    if (!useHost) res.factors = own->FactorCount();
    res.stateHash = world.StateHash();
    res.bindingsAtEnd = host.Count();
    if (useHost) {
        host.Uninstall();
        host.DestroyAll();
    }
    return res;
}

}  // namespace

TEST_CASE("WP-20c scenario: a gravity ramp on the pad breaks the joint the capacity rule predicts at the predicted load (S: the radial decoupler, as the spike)") {
    if (!GameAvailable()) return;
    World w(true);
    struct Ref {
        const char* craft;
        const char* spikeJoint;  // the spike's reduced model (tree and graph), results/capacity.md
        double spikeG;
    };
    for (const Ref& r : {Ref{"PhysProf-S", "radialDecoupler1-2", 33.7}, Ref{"PhysProf-M2", "liquidEngineMainsail.v2", 29.0}}) {
        auto v = w.Craft(r.craft);
        const PadRampResult res = RunPadRamp(w, *v, 1.0, 250, 250 + 3000, false);
        MESSAGE(std::string(r.craft) << ": static loads at 1 g (" << std::string(res.solveAtSettle == LoadSolve::Graph ? "graph" : "tree") << " solve, "
                        << res.supportsAtSettle << " supports, " << res.solveMs << " ms) predict " << res.predictedChild << " at " << res.predictedG
                        << " g; the ramp broke " << res.brokenChild << " at " << res.gAtBreak << " g (spike: " << std::string(r.spikeJoint) << " at " << r.spikeG
                        << " g); " << res.factors << " factorisations");
        REQUIRE(res.firstBreakTick > 0);
        CHECK(res.brokenChild == res.predictedChild);
        CHECK(res.gAtBreak >= 0.97 * res.predictedG);
        CHECK(res.gAtBreak <= 1.05 * res.predictedG);
        CHECK(res.brokenChild == r.spikeJoint);
        CHECK(std::fabs(res.gAtBreak / r.spikeG - 1.0) < 0.10);
    }
}

TEST_CASE("WP-20c scenario: strutted craft on the pad (graph solve over the autostruts): M2-AS breaks the predicted joint at the predicted load, L2-AS holds at 1 g") {
    if (!GameAvailable()) return;
    World w(true);
    // M2-AS: the spike's graph solve broke the radial decoupler at 25.1 g (results/capacity.md); the articulated model the Mainsail joint
    // at 18.8 g (joint flex, risk R2)
    {
        auto v = w.Craft("PhysProf-M2-AS");
        const PadRampResult res = RunPadRamp(w, *v, 1.0, 250, 250 + 3000, false);
        MESSAGE("PhysProf-M2-AS: static loads at 1 g (" << std::string(res.solveAtSettle == LoadSolve::Graph ? "graph" : "tree") << " solve, "
                                                     << res.supportsAtSettle << " support, " << res.solveMs << " ms) predict " << res.predictedChild << " at "
                                                     << res.predictedG << " g; the ramp broke " << res.brokenChild << " at " << res.gAtBreak << " g (spike graph: "
                                                     << "radialDecoupler1-2 at 25.1 g); " << res.factors << " factorisations over " << res.firstBreakTick << " ticks");
        REQUIRE(res.firstBreakTick > 0);
        CHECK(res.solveAtSettle == LoadSolve::Graph);
        CHECK(res.brokenChild == res.predictedChild);
        CHECK(res.gAtBreak >= 0.97 * res.predictedG);
        CHECK(res.gAtBreak <= 1.05 * res.predictedG);
        CHECK(res.brokenChild == "radialDecoupler1-2");
        CHECK(std::fabs(res.gAtBreak / 25.1 - 1.0) < 0.05);
        CHECK(res.factors <= 3);  // one factorisation per structural change (the autostruts secured after the first step)
    }
    // L2-AS (680 t) stands on one proxy box (its core Mainsail's) and rocks on it (WP-20b: 0.04 m / 0.12 m/s of contact jitter): it holds
    // through the settle at 1 g; the ramp's dynamic loads break it somewhat below the static prediction
    {
        auto v = w.Craft("PhysProf-L2-AS");
        const PadRampResult res = RunPadRamp(w, *v, 1.0, 250, 250 + 3000, false);
        MESSAGE("PhysProf-L2-AS: static loads at 1 g (" << std::string(res.solveAtSettle == LoadSolve::Graph ? "graph" : "tree") << " solve, "
                                                     << res.supportsAtSettle << " support, " << res.solveMs << " ms) predict " << res.predictedChild << " at "
                                                     << res.predictedG << " g; the ramp broke " << res.brokenChild << " at " << res.gAtBreak << " g");
        REQUIRE(res.firstBreakTick > 250);
        CHECK(res.solveAtSettle == LoadSolve::Graph);
        CHECK(res.predictedG > 2.0);
        CHECK(res.gAtBreak >= 0.85 * res.predictedG);
        CHECK(res.gAtBreak <= 1.05 * res.predictedG);
    }
}

TEST_CASE("WP-20c events: decouple, couple and undock through GameEvents rebind the vessels and keep the momentum; the module force hooks") {
    if (!GameAvailable()) return;
    World w;
    Vessel::SpawnedVessels().clear();
    auto v = w.Craft("PhysProf-S");
    {
        PhysicsWorld world;
        VesselPhysicsHost host(world);
        host.Install();
        VesselPhysics& vp = host.Create(*v, VesselPose{});
        vp.SetLinearVelocity(Vector3d(0.0, 3.0, 50.0));
        world.SetAngularVelocity(vp.ClusterBody(0), Vector3d(0.05, 0.2, -0.1));
        vp.AfterStep();
        const Totals t0 = Measure({&vp});
        Part* d12 = FindByName(*v, "radialDecoupler1-2");
        REQUIRE(d12 != nullptr);
        Part* tank = d12->parent;
        // (1) Part.decouple: onPartDeCoupleNewVesselComplete -> a deferred rebind into the two vessels
        Vessel* booster = d12->decouple();
        REQUIRE(booster != nullptr);
        CHECK(host.StructuralChangesPending());
        CHECK(host.Count() == 1);
        host.ProcessStructuralChanges();
        CHECK_FALSE(host.StructuralChangesPending());
        CHECK(host.Count() == 2);
        REQUIRE(host.Find(*booster) != nullptr);
        REQUIRE(host.Find(*v) != nullptr);
        const Totals t1 = Measure(Bindings(host, {v.get(), booster}));
        CHECK(t1.parts == 21);
        CHECK(t1.mass == doctest::Approx(t0.mass).epsilon(1e-12));
        CHECK((t1.com - t0.com).magnitude() < 1e-9);
        CHECK((t1.P - t0.P).magnitude() < 1e-6 * t0.P.magnitude());
        CHECK((t1.L - t0.L).magnitude() < 1e-6 * t0.L.magnitude());
        // (2) the anchored decoupler's ejection pair through the module hooks (ModuleAnchoredDecoupler.cs:41: part.AddForce(-F),
        // otherPart.AddForceAtPosition(F, part.position)): internal forces, the linear momentum stays
        const Vector3f F(0.0f, 0.0f, 300.0f);
        REQUIRE(static_cast<bool>(ModuleDecouplerBase::addForce));
        ModuleDecouplerBase::addForce(*d12, -F);
        ModuleDecouplerBase::addForceAtPosition(*tank, F, d12->partTransform.position);
        host.Step(0.02f);
        const Totals t2 = Measure(Bindings(host, {v.get(), booster}));
        CHECK((t2.P - t1.P).magnitude() < 1e-5 * t1.P.magnitude());
        CHECK(host.Find(*booster)->ClusterVelocity(0).z < host.Find(*v)->ClusterVelocity(0).z);  // pushed apart
        for (int k = 0; k < 10; ++k) host.Step(0.02f);
        // (3) Part.Couple (docking back): onPartCouple lets go of the booster Vessel (Couple destroys it) and rebinds both into the core
        const Totals t3 = Measure(Bindings(host, {v.get(), booster}));
        d12->Couple(tank);
        host.ProcessStructuralChanges();
        CHECK(host.Count() == 1);
        REQUIRE(host.Find(*v) != nullptr);
        CHECK(host.Find(*v)->ClusterCount() == 1);
        CHECK(world.BodyCount() == 1);
        const Totals t4 = Measure({host.Find(*v)});
        CHECK(t4.parts == 21);
        CHECK((t4.P - t3.P).magnitude() < 1e-6 * t3.P.magnitude());
        CHECK((t4.L - t3.L).magnitude() < 1e-5 * t3.L.magnitude());
        // (4) Part.Undock (no DockedVesselInfo): onVesselsUndocking -> the two vessels again, momentum kept
        Vessel* undocked = d12->Undock(nullptr);
        REQUIRE(undocked != nullptr);
        host.ProcessStructuralChanges();
        CHECK(host.Count() == 2);
        const Totals t5 = Measure(Bindings(host, {v.get(), undocked}));
        CHECK(t5.parts == 21);
        CHECK((t5.P - t4.P).magnitude() < 1e-6 * t4.P.magnitude());
        CHECK((t5.L - t4.L).magnitude() < 1e-5 * t4.L.magnitude());
        host.Uninstall();
        CHECK_FALSE(static_cast<bool>(ModuleDecouplerBase::addForce));
        host.DestroyAll();
    }
    Vessel::SpawnedVessels().clear();
}

namespace {
// the part's own inertia tensor in physics axes times w, plus its orbital angular momentum about the physics-frame origin
Vector3d PartAngularMomentum(const VesselPhysics& vp, const Part& p) {
    const PartPhysicsState s = vp.PartState(p);
    const Vector3f d = vp.PartInertia(p);
    const double x = s.rotation.x, y = s.rotation.y, z = s.rotation.z, q = s.rotation.w;
    const double R[3][3] = {{1 - 2 * (y * y + z * z), 2 * (x * y - z * q), 2 * (x * z + y * q)},
                            {2 * (x * y + z * q), 1 - 2 * (x * x + z * z), 2 * (y * z - x * q)},
                            {2 * (x * z - y * q), 2 * (y * z + x * q), 1 - 2 * (x * x + y * y)}};
    const double dd[3] = {d.x, d.y, d.z}, wv[3] = {s.angularVelocity.x, s.angularVelocity.y, s.angularVelocity.z};
    double out[3] = {0, 0, 0};
    for (int a = 0; a < 3; ++a)
        for (int b = 0; b < 3; ++b) out[a] += (R[a][0] * dd[0] * R[b][0] + R[a][1] * dd[1] * R[b][1] + R[a][2] * dd[2] * R[b][2]) * wv[b];
    return Vector3d(out[0], out[1], out[2]) + Vector3d::Cross(s.centerOfMass, s.velocity * static_cast<double>(vp.PartMass(p)));
}
}  // namespace

TEST_CASE("WP-20c events: Part.Die drops the part (its momentum leaves with it), its FULL children decouple into new vessels") {
    if (!GameAvailable()) return;
    World w;
    Vessel::SpawnedVessels().clear();
    auto v = w.Craft("PhysProf-S");
    {
        PhysicsWorld world;
        VesselPhysicsHost host(world);
        host.Install();
        VesselPhysics& vp = host.Create(*v, VesselPose{});
        vp.SetLinearVelocity(Vector3d(1.0, -2.0, 30.0));
        world.SetAngularVelocity(vp.ClusterBody(0), Vector3d(0.1, 0.0, 0.3));
        vp.AfterStep();
        // (1) a leaf: a booster's nose cone
        Part* cone = FindByName(*v, "pointyNoseConeB");
        REQUIRE(cone != nullptr);
        const Totals t0 = Measure({&vp});
        const PartPhysicsState cs = vp.PartState(*cone);
        const double mc = vp.PartMass(*cone);
        const Vector3d Lc = PartAngularMomentum(vp, *cone);
        cone->Die();
        CHECK(cone->State() == PartStates::DEAD);
        CHECK(std::find(v->parts.begin(), v->parts.end(), cone) == v->parts.end());
        CHECK(host.StructuralChangesPending());
        host.ProcessStructuralChanges();
        REQUIRE(host.Find(*v) != nullptr);
        const Totals t1 = Measure({host.Find(*v)});
        CHECK(t1.parts == 20);
        CHECK(t1.mass == doctest::Approx(t0.mass - mc).epsilon(1e-9));
        CHECK((t1.P - (t0.P - cs.velocity * mc)).magnitude() < 1e-6 * t0.P.magnitude());
        CHECK((t1.L - (t0.L - Lc)).magnitude() < 1e-5 * t0.L.magnitude());
        // (2) a part in the middle of the other booster (its second flat tank): its FULL child decouples into a new vessel, the part leaves
        Part* d2 = FindByName(*v, "radialDecoupler1-2", 1);
        REQUIRE(d2 != nullptr);
        Part* lower = d2->children.front();  // the booster's first flat tank
        Part* mid = nullptr;
        for (Part* c : lower->children)
            if (c->partInfo->name == "fuelTankSmallFlat") mid = c;
        REQUIRE(mid != nullptr);
        REQUIRE(mid->children.size() == 1);
        const VesselPhysics& vp1 = *host.Find(*v);
        const Totals t2 = Measure({&vp1});
        const PartPhysicsState ms = vp1.PartState(*mid);
        const double mm = vp1.PartMass(*mid);
        const Vector3d Lm = PartAngularMomentum(vp1, *mid);
        mid->Die();
        host.ProcessStructuralChanges();
        const std::vector<Vessel*> live = LiveVessels(*v);
        CHECK(live.size() == 2);  // the core and the upper part of the booster
        CHECK(host.Count() == 2);
        const Totals t3 = Measure(Bindings(host, live));
        CHECK(t3.parts == 19);
        CHECK(t3.mass == doctest::Approx(t2.mass - mm).epsilon(1e-9));
        CHECK((t3.P - (t2.P - ms.velocity * mm)).magnitude() < 1e-6 * t2.P.magnitude());
        CHECK((t3.L - (t2.L - Lm)).magnitude() < 1e-5 * t2.L.magnitude());
        host.Uninstall();
        host.DestroyAll();
    }
    Vessel::SpawnedVessels().clear();
}

TEST_CASE("WP-20c joint break: PartJoint.OnJointBreak -> onPartJointBreak -> Part.OnPartJointBreak -> Part.decouple; IgnoreCollisionsFrames keeps the part") {
    if (!GameAvailable()) return;
    World w;
    for (int ignore = 0; ignore < 2; ++ignore) {
        INFO(std::string(ignore ? "IgnoreCollisionsFrames > 0" : "normal"));
        Vessel::SpawnedVessels().clear();
        auto v = w.Craft("PhysProf-S");
        {
            EventLog log;
            PhysicsWorld world;
            VesselPhysicsHost host(world);
            host.Install();
            VesselPhysics& vp0 = host.Create(*v, VesselPose{});
            for (Part* p : v->parts) vp0.SetAngularDrag(*p, 0.0f);
            if (ignore) v->SetIgnoreCollisionsFrames(100000);
            Part* tank = FindByName(*v, "Rockomax32.BW");
            // a lateral push of the core through the composite centre of mass, growing until a joint gives
            const float dt = 0.02f;
            int breakTick = -1;
            Totals before, after;
            Vector3d Fk, X;
            for (int k = 1; k <= 2000 && breakTick < 0; ++k) {
                VesselPhysics* vp = host.Find(*v);
                const std::vector<const VesselPhysics*> all = Bindings(host, LiveVessels(*v));
                before = Measure(all);
                X = before.com;
                Fk = Vector3d(before.mass * 2.0 * k, 0.0, 0.0);  // 2 m/s^2 more per tick
                vp->AddForceAtPhysicsPoint(*tank, Fk, X);
                host.Step(dt);
                if (!host.LastBreaks().empty()) {
                    breakTick = k;
                    after = Measure(Bindings(host, LiveVessels(*v)));
                }
            }
            REQUIRE(breakTick > 0);
            const VesselPhysicsHost::BreakRecord& br = host.LastBreaks().front();
            MESSAGE(std::string(ignore ? "ignoring collisions" : "normal") << ": broke at tick " << breakTick << " (" << 2.0 * breakTick << " m/s^2): "
                                                                << br.child->partInfo->name << " -> " << br.parent->partInfo->name << ", sub-joint force " << br.force
                                                                << " kN (breakForce " << br.breakForce << ")");
            CHECK(br.force > br.breakForce);  // the sub-joint force over its PhysX limit
            // the momentum over the break tick: the push's impulse, nothing else (the split keeps the parts' momentum)
            CHECK((after.P - (before.P + Fk * static_cast<double>(dt))).magnitude() < 1e-5 * after.P.magnitude());
            CHECK((after.L - (before.L + Vector3d::Cross(X, Fk * static_cast<double>(dt)))).magnitude() < 1e-4 * after.L.magnitude());
            CHECK(after.parts == 21);
            if (!ignore) {
                CHECK(br.decoupled);
                CHECK(log.jointBreaks >= 1);
                CHECK(log.jointBreakReports >= 1);  // OnPartJointBreak's onJointBreak report (breakForce > 0)
                CHECK(log.lastBreakForce > 0.0f);
                CHECK(LiveVessels(*v).size() >= 2);
                CHECK(host.Count() == LiveVessels(*v).size());
                CHECK(br.child->vessel != v.get());
            } else {
                CHECK_FALSE(br.decoupled);
                CHECK(log.jointBreaks == 0);
                CHECK(LiveVessels(*v).size() == 1);
                CHECK(host.Count() == 1);
                CHECK(host.Find(*v)->ClusterCount() >= 2);  // the joint is gone: the pieces move apart inside one vessel
                CHECK(br.child->vessel == v.get());
            }
            host.Uninstall();
            host.DestroyAll();
        }
        Vessel::SpawnedVessels().clear();
    }
}

TEST_CASE("WP-20c staging: every decoupler of PhysProf-L2-AS fired in one frame is one rebind; parts, mass and momentum conserved") {
    if (!GameAvailable()) return;
    World w;
    Vessel::SpawnedVessels().clear();
    auto v = w.Craft("PhysProf-L2-AS");
    {
        PhysicsWorld world;
        VesselPhysicsHost host(world);
        host.Install();
        VesselPhysics& vp = host.Create(*v, VesselPose{});
        vp.SetLinearVelocity(Vector3d(0.0, 120.0, 0.0));
        world.SetAngularVelocity(vp.ClusterBody(0), Vector3d(0.0, 0.05, 0.02));
        host.Step(0.02f);  // a first step: the autostruts are secured
        CHECK(host.Find(*v)->AutoStrutJointCount() == 456);
        const Totals t0 = Measure({host.Find(*v)});
        std::vector<Part*> decouplers;
        for (Part* p : v->parts)
            if (p->isAnchoredDecoupler() || p->isDecoupler()) decouplers.push_back(p);
        const auto c0 = std::chrono::steady_clock::now();
        for (Part* d : decouplers)
            if (d->parent != nullptr && d->vessel == v.get()) d->decouple();
        const double decoupleMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - c0).count();
        const size_t rebindsBefore = host.RebindCount();
        host.ProcessStructuralChanges();
        CHECK(host.RebindCount() == rebindsBefore + 1);
        const std::vector<Vessel*> live = LiveVessels(*v);
        MESSAGE("L2-AS: " << decouplers.size() << " decouplers fired in one frame -> " << live.size() << " vessels, " << host.Count()
                          << " bindings, " << world.BodyCount() << " bodies; one rebind " << host.LastRebindMs() << " ms (Part.decouple calls "
                          << decoupleMs << " ms)");
        CHECK(live.size() == decouplers.size() + 1);
        CHECK(host.Count() == live.size());
        const Totals t1 = Measure(Bindings(host, live));
        CHECK(t1.parts == 457);
        CHECK(t1.mass == doctest::Approx(t0.mass).epsilon(1e-12));
        CHECK((t1.com - t0.com).magnitude() < 1e-8);
        CHECK((t1.P - t0.P).magnitude() < 1e-6 * t0.P.magnitude());
        CHECK((t1.L - t0.L).magnitude() < 1e-5 * t0.L.magnitude());
#ifdef NDEBUG
        CHECK(host.LastRebindMs() < 25.0);  // the spike's staging gate is 5 ms one-off (Release); sanitizer builds are slower
#else
        CHECK(host.LastRebindMs() < 250.0);
#endif
        host.Uninstall();
        host.DestroyAll();
    }
    Vessel::SpawnedVessels().clear();
}

TEST_CASE("WP-20c determinism: a gravity ramp on the pad with breaks and rebinds is bit-identical run to run and at 1, 2, 4 and 8 threads") {
    if (!GameAvailable()) return;
    World w(true);
    uint64_t hash0 = 0;
    int tick0 = -1;
    size_t bindings0 = 0;
    int run = 0;
    for (int threads : {1, 1, 2, 4, 8}) {
        Vessel::SpawnedVessels().clear();
        auto v = w.Craft("PhysProf-S");
        const PadRampResult r = RunPadRamp(w, *v, 5.0, 100, 100 + 600, true, threads, 25);
        INFO("threads " << threads);
        REQUIRE(r.firstBreakTick > 0);
        if (run++ == 0) {
            hash0 = r.stateHash;
            tick0 = r.firstBreakTick;
            bindings0 = r.bindingsAtEnd;
            MESSAGE("PhysProf-S, 5 g/s ramp: first break at tick " << r.firstBreakTick << " (" << r.gAtBreak << " g, " << r.brokenChild << "), "
                                                                  << r.bindingsAtEnd << " bindings 25 ticks later");
        }
        CHECK(r.firstBreakTick == tick0);
        CHECK(r.bindingsAtEnd == bindings0);
        CHECK(r.stateHash == hash0);
        Vessel::SpawnedVessels().clear();
    }
}
