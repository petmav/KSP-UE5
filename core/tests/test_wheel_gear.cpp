// WP-29d: landing legs and landing gear on the wheel host with the parts' real models (core/data/wheel_models): the Two-Stage Lander's four
// LT-2 legs, the Aeris 3A's three LY-01 gear bays on a runway roll, the deployment animation (ModuleWheelDeployment over the rig), the
// stand-in sphere of a retracted gear.
//
// The craft are KSP's own (harness/inputs/Ships/VAB/Two-Stage Lander.craft, SPH/Aeris 3A.craft), loaded through the WP-18 / WP-14 path as the
// WP-29c rovers. References: closed forms (the suspension law's static compression, energy of the touchdown), the spike's measured numbers
// (WHEEL-SPIKE.md 4.4: legs at 3 m/s touchdown on Kerbin) as a cross-check. Not captures: no in-game gear tick exists (capture C11).
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>

#include "doctest/doctest.h"
#include "vessel_wheels_fixture.hpp"

using namespace ksp;
using namespace ksp::physics;
using namespace wp29c;

namespace {

constexpr double kRad = 3.14159265358979323846 / 180.0;
constexpr int kWheelsUp = 4;

struct GearRun {
    World w;
    LoadedRover r;
    std::unique_ptr<RoverScene> scene;
    explicit GearRun(const std::string& craft, double clearance = 0.0, double downSpeed = 0.0, WheelPhysicsOptions o = RealGeometry(), double heading = 0.0, bool airborne = false) {
        r = LoadRover(w, craft);
        scene = std::make_unique<RoverScene>(w, RoverOptions(r, o));
        scene->Place(r, heading, clearance, airborne);
        scene->pad.s.Start(*r.vessel);
        if (downSpeed != 0.0) Vp()->ChangeWorldVelocity(scene->pad.padUp * -downSpeed);
    }
    ~GearRun() {
        r.vessel.reset();
        scene.reset();
    }
    Vessel& V() { return *r.vessel; }
    VesselPhysics* Vp() { return scene->pad.s.bubble.Host().Find(V()); }
    Vector3d Up() const { return scene->pad.padUp; }
    void Tick() { scene->Tick(); }
    VesselWheels& Wheels() { return *scene->wheels.Find(V()); }
    Vector3d Com() { return VesselCoM(*Vp()); }
    Vector3d Vel() { return VesselVelocity(*Vp()); }
    double VerticalSpeed() { return Vector3d::Dot(Vel(), Up()); }
    double Altitude(const Vector3f& p) { return (Vector3d(p) - w.Kerbin().position).magnitude() - w.Kerbin().Radius; }
    double BodyUpDot() { return Vector3d::Dot(Vector3d(V().vesselTransform.rotation * r.craftUp).normalized(), Up()); }
    // the lowest tyre bottom (at full extension) of the set-up wheels above the ground sphere
    double LowestTyreBottom() {
        double lo = 1e9;
        for (auto& w : Wheels().Wheels()) {
            if (w->Controller() == nullptr || w->Controller()->wheelCollider == nullptr) continue;
            const auto& g = w->lastGeometry;
            const auto& wc = w->Controller()->wheelCollider->wheelColliderData;
            const Vector3d bottom = Vector3d(g.top) - Vector3d(g.up) * (static_cast<double>(wc.suspensionDistance) + static_cast<double>(wc.radius));
            lo = std::min(lo, Altitude(bottom.ToVector3f()));
        }
        return lo;
    }
};

bool Healthy(GearRun& run) {
    if (run.Vp() == nullptr) return false;
    const Vector3d v = run.Vel();
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && v.magnitude() < 200.0;
}

// A craft placed airborne with its lowest tyre bottom (at full extension) `fall` above the ground and `downSpeed` m/s down: the clearance comes from
// a measuring scene (Place puts the lowest drag-cube point at the clearance, the tyre bottoms hang below it)
std::unique_ptr<GearRun> Dropped(const std::string& craft, double fall, double downSpeed, const WheelPhysicsOptions& o = RealGeometry(), double heading = 0.0) {
    double bottom0 = 0.0;
    {
        GearRun m(craft, 0.0, 0.0, o, heading, true);
        for (int k = 0; k < 40; ++k) {
            m.Tick();
            if (k >= kWheelsUp && m.Vp() != nullptr && m.Wheels().SetUpCount() == static_cast<int>(m.Wheels().Wheels().size())) break;
        }
        m.Tick();
        bottom0 = m.LowestTyreBottom();
    }
    auto run = std::make_unique<GearRun>(craft, fall - bottom0, 0.0, o, heading, true);
    for (int k = 0; k < 10 && run->Vp() == nullptr; ++k) run->Tick();  // the vessel unpacks in the first ticks
    REQUIRE(run->Vp() != nullptr);
    if (downSpeed != 0.0) run->Vp()->ChangeWorldVelocity(run->Up() * -downSpeed);
    return run;
}

struct Touchdown {
    double touchSpeed = 0, maxCompression = 0, maxRebound = 0, settleTime = -1, finalCompression = 0, finalTilt = 0, finalSpeed = 0;
    int legsGrounded = 0, legs = 0, bottomedTicks = 0;
    bool healthy = true;
    double h0 = 0;
};

// The lander dropped so that its tyre bottoms reach the ground at `touch` m/s: the clearance comes from a measuring scene
Touchdown LanderTouchdown(double touch, WheelPhysicsOptions o = RealGeometry(), double fall = 0.03) {
    Touchdown t;
    // 1. the tyre-bottom height of the craft placed at clearance 0 (the proxy's lowest point on the ground)
    double bottom0 = 0.0;
    {
        GearRun m("Two-Stage Lander", 0.0, 0.0, o, 0.0, true);
        for (int k = 0; k < kWheelsUp; ++k) m.Tick();
        bottom0 = m.LowestTyreBottom();
    }
    // 2. the drop: the tyre bottom `fall` above the ground, the speed that reaches `touch` there (v^2 = v0^2 + 2 g h)
    GearRun run("Two-Stage Lander", fall - bottom0, 0.0, o, 0.0, true);
    const double gee = 9.81;  // Kerbin
    const double v0 = std::sqrt(std::max(0.0, touch * touch - 2.0 * gee * fall));
    for (int k = 0; k < 10 && run.Vp() == nullptr; ++k) run.Tick();  // the vessel unpacks in the first ticks
    REQUIRE(run.Vp() != nullptr);
    run.Vp()->ChangeWorldVelocity(run.Up() * -v0);
    t.h0 = fall;
    double lastMove = 0, prevVz = 0;
    for (int k = 0; k < 600; ++k) {
        prevVz = run.VerticalSpeed();
        run.Tick();
        if (!Healthy(run)) {
            t.healthy = false;
            break;
        }
        const double vz = run.VerticalSpeed();
        int grounded = 0;
        double comp = 0;
        for (auto& w : run.Wheels().Wheels()) {
            if (!w->last.grounded) continue;
            ++grounded;
            const auto& gm = w->lastGeometry;
            comp = std::max(comp, static_cast<double>((gm.maxDroop + w->last.jounce) / (gm.maxDroop + gm.maxCompression)));
            t.bottomedTicks += w->last.limitActive ? 1 : 0;
        }
        if (grounded > 0 && t.touchSpeed == 0.0) t.touchSpeed = -prevVz;  // the speed at the start of the tick that met the ground
        t.maxCompression = std::max(t.maxCompression, comp);
        if (t.touchSpeed > 0.0) {
            t.maxRebound = std::max(t.maxRebound, vz);
            if (std::fabs(vz) > 0.05) lastMove = (k + 1) * 0.02;
        }
        t.legsGrounded = grounded;
        t.finalCompression = comp;
        t.finalSpeed = run.Vel().magnitude();
    }
    t.settleTime = lastMove;
    t.legs = static_cast<int>(run.Wheels().Wheels().size());
    t.finalTilt = std::acos(std::max(-1.0, std::min(1.0, run.BodyUpDot()))) / kRad;
    return t;
}

}  // namespace

TEST_CASE("WP-29d lander legs: the LT-2 geometry from the model (collider transforms, deployTgt, animation at the deployed state)") {
    if (!wp20b::GameAvailable()) return;
    GearRun run("Two-Stage Lander", 0.0, 0.0);
    for (int k = 0; k < kWheelsUp; ++k) run.Tick();
    auto& legs = run.Wheels().Wheels();
    REQUIRE(legs.size() == 4);
    for (auto& w : legs) {
        REQUIRE(w->rig != nullptr);
        CHECK(w->colliderNode >= 0);
        CHECK(w->base->IsWheelSetUp());
        CHECK(w->geometryLocked);
        // ModuleWheelDeployment.OnWheelSetup moved the collider transform to deployTgt: the collider host sits at deployTgt's pose
        VehiclePhysics::NativeTransform* tgt = WheelModuleEnvironment::findModelTransform(*w->part, "deployTgt");
        REQUIRE(tgt != nullptr);
        CHECK((Vector3d(tgt->position) - Vector3d(w->colliderHost.position)).magnitude() < 1e-4);
        // the deployed foot hangs below the leg's body: the suspension axis (collider up) is close to the vessel's up
        CHECK(Vector3d::Dot(Vector3d(w->colliderHost.up()), run.Up()) > 0.7);
    }
}

TEST_CASE("WP-29d lander legs: 3 m/s touchdown on Kerbin (no bounce-launch, compression 0.35-0.75 of the travel, settles within 3 s)") {
    if (!wp20b::GameAvailable()) return;
    const Touchdown t = LanderTouchdown(3.0);
    MESSAGE("lander: touch " << t.touchSpeed << " m/s, max compression " << t.maxCompression << " of the travel, max rebound " << t.maxRebound << " m/s, settled after "
                             << t.settleTime << " s, final compression " << t.finalCompression << ", legs grounded " << t.legsGrounded << "/" << t.legs << ", bottomed ticks "
                             << t.bottomedTicks << ", tilt " << t.finalTilt << " deg, speed " << t.finalSpeed);
    CHECK(t.healthy);
    CHECK(t.touchSpeed == doctest::Approx(3.0).epsilon(0.08));
    CHECK(t.maxCompression > 0.35);                         // WHEEL-SPIKE.md 4.4: 50 % with the proxy, 62 % with Jolt's VehicleConstraint
    CHECK(t.maxCompression < 0.75);
    CHECK(t.bottomedTicks == 0);                            // the travel limit is not reached
    CHECK(t.maxRebound < t.touchSpeed);                     // no bounce-launch
    CHECK(t.settleTime > 0.0);
    CHECK(t.settleTime < 3.0);                              // settles
    CHECK(t.legsGrounded == t.legs);
    CHECK(t.finalTilt < 0.5);
    CHECK(t.finalSpeed < 0.1);
}


namespace {
std::vector<ModuleWheels::ModuleWheelDeployment*> Deployments(Vessel& v, std::vector<Part*>* parts = nullptr) {
    std::vector<ModuleWheels::ModuleWheelDeployment*> out;
    for (Part* p : v.parts)
        for (size_t i = 0; i < p->Modules().Count(); ++i)
            if (auto* d = dynamic_cast<ModuleWheels::ModuleWheelDeployment*>(p->Modules()[i])) {
                out.push_back(d);
                if (parts) parts->push_back(p);
            }
    return out;
}
}  // namespace

void RetractDeploy(const std::string& craft, size_t expectedParts, bool expectStandIn) {
    if (!wp20b::GameAvailable()) return;
    GearRun run(craft, 3000.0, 0.0, RealGeometry(), 0.0, true);  // high above the ground (24 s of free fall): only the gear is under test
    for (int k = 0; k < 8; ++k) run.Tick();
    std::vector<Part*> depParts;
    auto deps = Deployments(run.V(), &depParts);
    REQUIRE(deps.size() == expectedParts);
    for (auto* d : deps) {
        CHECK(d->Position() == doctest::Approx(1.0f));
        CHECK(!d->StandInColliderActive());
    }
    auto foot = [&](size_t i) {  // the deploy target in the part frame (the model rig answers in part space)
        VehiclePhysics::NativeTransform pt;
        pt.position = depParts[i]->partTransform.position;
        pt.rotation = depParts[i]->partTransform.rotation;
        return Vector3d(pt.InverseTransformPoint(deps[i]->DeployTarget()->position));
    };
    std::vector<Vector3d> footDeployed;
    for (size_t i = 0; i < deps.size(); ++i) footDeployed.push_back(foot(i));
    for (auto* d : deps) d->ActionToggle(KSPActionType::Deactivate);  // the Gear action group: retract
    bool standIn = false, wheelOffSeen = false, standInBuilt = false;
    double minPos = 1.0;
    for (int k = 0; k < 400; ++k) {  // 8 s
        run.Tick();
        for (auto* d : deps) {
            standIn = standIn || d->StandInColliderActive();
            minPos = std::min(minPos, static_cast<double>(d->Position()));
        }
        for (auto& w : run.Wheels().Wheels()) {
            wheelOffSeen = wheelOffSeen || !w->nativeOn;
            standInBuilt = standInBuilt || w->standInBuilt;
        }
    }
    MESSAGE("retracted: position " << deps[0]->Position() << " (min " << minPos << "), stand-in seen " << standIn << ", wheel off seen " << wheelOffSeen << ", state "
                                   << deps[0]->Get<std::string>("stateString") << ", foot y " << foot(0).y << " (deployed " << footDeployed[0].y << ")");
    CHECK(deps[0]->Position() < 0.05f);
    CHECK(standIn == expectStandIn);        // useStandInCollider: the LY gear bays have one, the LT-2 legs do not
    CHECK(standInBuilt == expectStandIn);   // ... and it is a sphere in the cluster compound while the gear is not deployed
    CHECK(wheelOffSeen);
    CHECK(deps[0]->Get<std::string>("stateString") == "Retracted");
    CHECK(foot(0).y > footDeployed[0].y + 0.3);  // the foot came up in the part frame
    for (auto* d : deps) d->ActionToggle(KSPActionType::Activate);  // deploy again
    for (int k = 0; k < 400; ++k) run.Tick();
    MESSAGE("deployed again: position " << deps[0]->Position() << ", state " << deps[0]->Get<std::string>("stateString") << ", foot y " << foot(0).y);
    CHECK(deps[0]->Position() > 0.95f);
    CHECK(!deps[0]->StandInColliderActive());
    CHECK(deps[0]->Get<std::string>("stateString") == "Deployed");
    for (auto& w : run.Wheels().Wheels()) CHECK(!w->standInBuilt);
    CHECK((foot(0) - footDeployed[0]).magnitude() < 0.01);
}

TEST_CASE("WP-29d lander legs retract and deploy: the animation over the model rig, TsubSys, the wheel on and off (no stand-in on the LT-2)") { RetractDeploy("Two-Stage Lander", 4, false); }

TEST_CASE("WP-29d gear bays retract and deploy: the animation, TsubSys, the stand-in sphere in the cluster compound, the wheel on and off (LY-01 on the VTOL craft)") { RetractDeploy("Rocket-power VTOL", 4, true); }

// Vessel.CheckGroundCollision / getLowestPoint (Vessel.cs:2578-2640) on a landed craft with legs: the lowest collider decides, a WheelCollider
// component by its centre minus its radius minus 0.15. The lander is placed landed (PRELAUNCH) and the host moves it to the ground: its foot
// colliders (the piston colliders ModuleWheelSuspension disables are enabled for the measure) rest within a few centimetres of the ground,
// where the first port of the C# (no collider roles, no disabled colliders) left the tyre bottoms 0.53 m below it.
TEST_CASE("WP-29d landed placement: the lander is put on the ground by its foot colliders (getLowestPoint), the tyre bottom a few centimetres below it") {
    if (!wp20b::GameAvailable()) return;
    GearRun run("Two-Stage Lander", 0.0, 0.0);
    for (int k = 0; k < kWheelsUp + 4; ++k) run.Tick();
    const double low = run.LowestTyreBottom();
    MESSAGE("landed placement: tyre bottom at full extension " << low << " m from the ground, speed " << run.Vel().magnitude());
    CHECK(low < 0.0);
    CHECK(low > -0.15);
    CHECK(run.Wheels().GroundedCount() == 4);
}

