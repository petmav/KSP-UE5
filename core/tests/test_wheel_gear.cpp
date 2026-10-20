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

TEST_CASE("WP-29d diag lander" * doctest::skip(true)) {
    if (!wp20b::GameAvailable()) return;
    GearRun run("Two-Stage Lander", 1.0, 0.0, RealGeometry(), 0.0, true);
    std::string s;
    for (int k = 0; k < 40; ++k) {
        run.Tick();
        char b[200];
        std::snprintf(b, sizeof b, "[%d vz=%.3f low=%.3f gr=%d]", k, run.Vp() ? run.VerticalSpeed() : 0.0, run.Vp() ? run.LowestTyreBottom() : 0.0, run.Vp() ? run.Wheels().GroundedCount() : -1);
        s += b;
    }
    MESSAGE(s);
}
