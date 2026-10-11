// WP-20a: physics world (Jolt Physics v5.6.0, vendored, JPH_DOUBLE_PRECISION), frame model (Krakensbane / FloatingOrigin without
// the shift, body-fixed <-> inertial switch) and their scenarios against analytic results. There is no KSP golden or capture
// for any of this (PhysX is not reproducible bit for bit); the gate is analytic: free fall, rest on a static floor, a Kepler
// orbit in the inertial frame with and without Krakensbane's frame velocity, the same orbit in the body-fixed frame with the
// Coriolis / centrifugal accelerations of FlightGlobals.cs:1356-1373, plus run-to-run and thread-count determinism.
//
// Jolt is deterministic per binary and call order (PHYSICS-SPIKE.md R8). It is not bit-identical across compilers / OSes in the
// default (non CROSS_PLATFORM_DETERMINISTIC) build, so no test here stores a hash; hashes are only compared between runs of the
// same process. Analytic comparisons use tolerances derived below, not Jolt-version-specific values.
#include <cmath>
#include <cstdint>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/core/timing_manager.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/physics/floating_origin.hpp"
#include "ksp/physics/krakensbane.hpp"
#include "ksp/physics/physics_world.hpp"
#include "ksp/physics/reference_frame.hpp"

using namespace ksp;
using namespace ksp::physics;

namespace {

constexpr double kG = 9.80665;

struct TimeGuard {
    float fdt = Time::fixedDeltaTime, dt = Time::deltaTime, t = Time::time;
    int frames = Time::frameCount;
    ~TimeGuard() {
        Time::fixedDeltaTime = fdt;
        Time::deltaTime = dt;
        Time::time = t;
        Time::frameCount = frames;
    }
};

BodyId MakeBox(PhysicsWorld& w, Vector3d pos, Vector3f half, MotionType m = MotionType::Dynamic, float mass = 10.0f) {
    BodyDesc d;
    d.shape = ShapeDesc::Box(half);
    d.motion = m;
    d.position = pos;
    d.mass = mass;
    return w.CreateBody(d);
}

// Deterministic pile: 96 boxes dropped in a column grid onto a static floor, gravity as a force, then `steps` steps.
uint64_t RunPile(int threads, int steps, uint64_t* hashAtStep50 = nullptr) {
    PhysicsWorldConfig cfg;
    cfg.threads = threads;
    PhysicsWorld w(cfg);
    MakeBox(w, Vector3d(0, -0.5, 0), Vector3f(40, 0.5f, 40), MotionType::Static);
    std::vector<BodyId> ids;
    uint32_t s = 12345u;
    auto rnd = [&s] { s = s * 1664525u + 1013904223u; return static_cast<double>(s >> 8) / 16777216.0; };
    for (int i = 0; i < 96; ++i) {
        const int ix = i % 8, iz = (i / 8) % 4, iy = i / 32;
        Vector3d p(ix * 1.3 - 4.5 + rnd() * 0.2, 1.0 + iy * 1.6 + rnd() * 0.3, iz * 1.3 - 2.0 + rnd() * 0.2);
        BodyDesc d;
        d.shape = (i % 3 == 0) ? ShapeDesc::Sphere(0.55f) : ShapeDesc::Box(Vector3f(0.5f, 0.4f, 0.5f));
        d.position = p;
        d.mass = 5.0f + static_cast<float>(i % 7);
        d.linearVelocity = Vector3d(rnd() - 0.5, 0, rnd() - 0.5);
        ids.push_back(w.CreateBody(d));
    }
    for (int k = 0; k < steps; ++k) {
        for (BodyId id : ids) w.AddForce(id, Vector3d(0, -kG * w.GetMass(id), 0));
        w.Step(0.02f);
        if (hashAtStep50 && k == 49) *hashAtStep50 = w.StateHash();
    }
    return w.StateHash();
}

// Rotation of v about the unit axis n by `angle` (Rodrigues), for the rotating-frame scenario.
Vector3d Rotate(const Vector3d& v, const Vector3d& n, double angle) {
    const double c = std::cos(angle), s = std::sin(angle);
    const Vector3d nxv = Vector3d::Cross(n, v);
    const double ndv = Vector3d::Dot(n, v);
    return v * c + nxv * s + n * (ndv * (1.0 - c));
}

constexpr double kMuKerbin = 3.5316e12;  // Kerbin GM (KerbolSystemData, m^3/s^2)

// One body on a central-force orbit around the origin of the physics frame, inertial. With `useKrakensbane` the body's physics
// velocity is moved into Krakensbane's frame velocity by Krakensbane::FixedUpdate each tick (KSP's rule, or re-centred at
// `recentreSpeed` m/s), gravity is evaluated at the TRUE position (physics position + FloatingOrigin::FrameOffset()), exactly
// what VesselPrecalculate does with the double CoM (PHYSICS-SPIKE.md 5.4).
struct OrbitResult {
    double maxRadiusErr = 0;
    double endPosErr = 0;
    double maxFrameDev = 0;   // max |physics-frame velocity| after Krakensbane's FixedUpdate
    size_t velocityWrites = 0;
};

OrbitResult RunOrbit(double r0, double revolutions, bool useKrakensbane, double recentreSpeed, double* periodOut = nullptr) {
    TimeGuard tg;
    Time::fixedDeltaTime = 0.02f;
    PhysicsWorld w;
    const double v0 = std::sqrt(kMuKerbin / r0);
    const double T = 2.0 * 3.14159265358979323846 * std::sqrt(r0 * r0 * r0 / kMuKerbin);
    if (periodOut) *periodOut = T;
    BodyDesc d;
    d.shape = ShapeDesc::Box(Vector3f(1, 1, 1));
    d.mass = 1000.0f;
    d.position = Vector3d(r0, 0, 0);
    d.linearVelocity = Vector3d(0, 0, v0);
    BodyId id = w.CreateBody(d);

    Krakensbane k;
    FloatingOrigin fo;
    CHECK(k.Awake());
    fo.Awake();
    fo.ready = [] { return true; };
    k.recentreSpeed = recentreSpeed;
    OrbitResult res;
    k.host.snapshot = [&] {
        FrameSnapshot s;
        s.ready = true;
        s.hasActiveVessel = true;
        s.activeHasRigidbody = true;
        s.active.isActive = true;
        const Vector3d p = ReferenceFrame::TruePosition(w.GetPosition(id));
        s.rb_velocityD = w.GetLinearVelocity(id);
        s.velocityD = ReferenceFrame::TrueVelocity(s.rb_velocityD);
        s.upAxis = p.normalized();
        s.active.radarAltitude = p.magnitude() - 600000.0;
        s.loaded.push_back(s.active);
        return s;
    };
    k.host.changeWorldVelocity = [&](size_t, const Vector3d& dv) { res.velocityWrites += w.AddVelocityChangeToAllMoving(dv); };

    const double dt = static_cast<double>(Time::fixedDeltaTime);
    const uint64_t steps = static_cast<uint64_t>(revolutions * T / dt);
    for (uint64_t n = 0; n < steps; ++n) {
        if (useKrakensbane) {
            k.FixedUpdate();
            const double fv = w.GetLinearVelocity(id).magnitude();
            if (fv > res.maxFrameDev) res.maxFrameDev = fv;
        }
        const Vector3d p = ReferenceFrame::TruePosition(w.GetPosition(id));
        const double r = p.magnitude();
        const double re = std::fabs(r - r0);
        if (re > res.maxRadiusErr) res.maxRadiusErr = re;
        w.AddForce(id, p * (-kMuKerbin * 1000.0 / (r * r * r)));
        w.FixedStep();
        // FloatingOrigin consumes the tick's out-of-frame offset AFTER the step: gravity above read the true position of the
        // start of the tick (FrameOffset() only holds completed ticks). Consuming it before the step would evaluate gravity one
        // FrameVel * dt (45 m at orbital speed) ahead of the body.
        if (useKrakensbane) fo.FixedUpdate();
    }
    // analytic end position: circular orbit, angle = (v0 / r0) * t, motion from +x towards +z
    const double t = static_cast<double>(steps) * dt;
    const double th = v0 / r0 * t;
    const Vector3d expect(r0 * std::cos(th), 0.0, r0 * std::sin(th));
    const Vector3d p = ReferenceFrame::TruePosition(w.GetPosition(id));
    res.endPosErr = (p - expect).magnitude();
    // true velocity must be the physics velocity plus the frame velocity, and a circular orbit's speed is v0
    const Vector3d tv = ReferenceFrame::TrueVelocity(w.GetLinearVelocity(id));
    CHECK(std::fabs(tv.magnitude() - v0) < 0.5);
    return res;
}

}  // namespace

TEST_CASE("WP-20a Jolt: vendored build configuration (double precision positions, Static/Moving layers)") {
    PhysicsWorld w;
    // a position that single precision cannot hold: 600 km + 1 mm
    BodyDesc d;
    d.shape = ShapeDesc::Sphere(1.0f);
    d.motion = MotionType::Kinematic;
    d.position = Vector3d(600000.001, -3.5e-3, 12345678.125);
    BodyId id = w.CreateBody(d);
    REQUIRE(id != kInvalidBody);
    const Vector3d p = w.GetPosition(id);
    CHECK(p.x == 600000.001);
    CHECK(p.y == -3.5e-3);
    CHECK(p.z == 12345678.125);
    CHECK(w.BodyCount() == 1);
    CHECK(w.IsValid(id));
    w.RemoveBody(id);
    CHECK(w.BodyCount() == 0);
    CHECK_FALSE(w.IsValid(id));
    w.RemoveBody(id);  // unknown ids are ignored
}

TEST_CASE("WP-20a PhysicsWorld: create, step, remove, destroy; bodies live in creation order; user data") {
    PhysicsWorld w;
    BodyDesc a;
    a.userData = 77;
    BodyDesc b;
    b.userData = 88;
    b.position = Vector3d(10, 0, 0);
    BodyId ia = w.CreateBody(a), ib = w.CreateBody(b);
    CHECK(w.Bodies() == std::vector<BodyId>{ia, ib});
    CHECK(w.UserData(ia) == 77);
    CHECK(w.UserData(ib) == 88);
    CHECK(w.GetMass(ia) == doctest::Approx(1.0f));
    w.Step(0.02f);
    w.Step(0.02f);
    CHECK(w.StepCount() == 2);
    w.RemoveBody(ia);
    CHECK(w.Bodies() == std::vector<BodyId>{ib});
    // the world is destroyed with a live body (bodies are released by the destructor)
}

TEST_CASE("WP-20a PhysicsWorld: free fall under a per-body force equals the semi-implicit Euler solution and the analytic fall") {
    PhysicsWorld w;
    const double y0 = 1000.0;
    const float dt = 0.02f;
    BodyId id = MakeBox(w, Vector3d(600000.0, y0, 0.0), Vector3f(0.5f, 0.5f, 0.5f));
    const double m = w.GetMass(id);
    CHECK(m == doctest::Approx(10.0));
    const int n = 250;  // 5 s
    for (int i = 0; i < n; ++i) {
        w.AddForce(id, Vector3d(0, -kG * m, 0));
        w.Step(dt);
    }
    const double t = n * static_cast<double>(dt);
    // Jolt integrates velocity first, then position with the new velocity: y_n = y0 - g dt^2 n (n+1) / 2 (up to float velocity rounding)
    const double dd = static_cast<double>(dt);
    const double discrete = y0 - kG * dd * dd * n * (n + 1) / 2.0;
    const Vector3d p = w.GetPosition(id);
    CHECK(std::fabs(p.y - discrete) < 5e-4);
    CHECK(p.x == 600000.0);  // the large x coordinate is not disturbed (double positions)
    CHECK(std::fabs(w.GetLinearVelocity(id).y - (-kG * dd * n)) < 1e-4);
    // against the continuous solution the scheme is off by g dt t / 2 at most
    const double continuous = y0 - 0.5 * kG * t * t;
    CHECK(std::fabs(p.y - continuous) <= kG * dd * t / 2.0 + 1e-3);
}

TEST_CASE("WP-20a PhysicsWorld: a box dropped on a static floor comes to rest on it (layers, contact, no sleeping)") {
    PhysicsWorld w;
    MakeBox(w, Vector3d(1.0e5, -0.5, -2.0e5), Vector3f(50, 0.5f, 50), MotionType::Static);  // top face at y = 0
    BodyId id = MakeBox(w, Vector3d(1.0e5, 5.0, -2.0e5), Vector3f(0.5f, 0.5f, 0.5f));
    const double m = w.GetMass(id);
    for (int i = 0; i < 400; ++i) {
        w.AddForce(id, Vector3d(0, -kG * m, 0));
        w.Step(0.02f);
    }
    const Vector3d p = w.GetPosition(id);
    CHECK(std::fabs(p.y - 0.5) < 0.05);  // Jolt's penetration slop is 0.02 m
    CHECK(std::fabs(w.GetLinearVelocity(id).y) < 0.05);
    CHECK(std::fabs(p.x - 1.0e5) < 1e-3);
    CHECK(std::fabs(p.z + 2.0e5) < 1e-3);
}

TEST_CASE("WP-20a PhysicsWorld: same-vessel bodies do not collide unless both set sameVesselCollision (GroupFilter)") {
    auto separation = [](uint32_t groupA, bool flagA, uint32_t groupB, bool flagB) {
        PhysicsWorld w;
        BodyDesc a;
        a.shape = ShapeDesc::Box(Vector3f(1, 1, 1));
        a.position = Vector3d(0, 0, 0);
        a.vesselGroup = groupA;
        a.sameVesselCollision = flagA;
        BodyDesc b = a;
        b.position = Vector3d(0.5, 0, 0);  // overlapping
        b.vesselGroup = groupB;
        b.sameVesselCollision = flagB;
        BodyId ia = w.CreateBody(a), ib = w.CreateBody(b);
        for (int i = 0; i < 30; ++i) w.Step(0.02f);
        return (w.GetPosition(ib) - w.GetPosition(ia)).magnitude();
    };
    CHECK(separation(7, false, 7, false) == doctest::Approx(0.5).epsilon(1e-6));  // same vessel: ignored
    CHECK(separation(7, true, 7, false) == doctest::Approx(0.5).epsilon(1e-6));   // only one flag
    CHECK(separation(7, true, 7, true) > 0.9);                                    // both flags: collide and separate
    CHECK(separation(7, false, 8, false) > 0.9);                                  // different vessels collide
    CHECK(separation(0, false, 0, false) > 0.9);                                  // ungrouped collide
}

TEST_CASE("WP-20a PhysicsWorld: bit-identical states run to run and at 1, 2, 4 and 8 threads") {
    uint64_t at50a = 0, at50b = 0;
    const uint64_t a = RunPile(1, 200, &at50a);
    const uint64_t b = RunPile(1, 200, &at50b);
    CHECK(a == b);
    CHECK(at50a == at50b);
    CHECK(a != 0);
    for (int threads : {2, 4, 8}) {
        uint64_t at50 = 0;
        const uint64_t h = RunPile(threads, 200, &at50);
        INFO("threads " << threads);
        CHECK(at50 == at50a);
        CHECK(h == a);
    }
}

TEST_CASE("WP-20a PhysicsWorld: Attach runs FixedStep at the FixedUpdate.PhysicsFixedUpdate slot of the player loop") {
    TimeGuard tg;
    Time::fixedDeltaTime = 0.02f;
    CoroutineScheduler sched;
    PlayerLoop loop(sched);
    PhysicsWorld w;
    BodyId id = MakeBox(w, Vector3d(0, 100, 0), Vector3f(0.5f, 0.5f, 0.5f));
    w.SetLinearVelocity(id, Vector3d(0, 0, 10));
    w.Attach(loop);
    w.Attach(loop);  // idempotent
    loop.Frame(0.1f, 5);
    CHECK(w.StepCount() == 5);
    CHECK(w.GetPosition(id).z == doctest::Approx(10.0 * 5 * 0.02).epsilon(1e-5));
    w.Detach(loop);
    loop.Frame(0.1f, 3);
    CHECK(w.StepCount() == 5);
}

TEST_CASE("WP-20a Krakensbane: SafeToEngage rules (Krakensbane.cs:87-119)") {
    TimeGuard tg;
    Time::fixedDeltaTime = 0.02f;
    Krakensbane k;
    REQUIRE(k.Awake());
    auto make = [](double radar, double descend, bool landed = false) {
        FrameSnapshot s;
        s.ready = s.hasActiveVessel = s.activeHasRigidbody = true;
        s.active.isActive = true;
        s.active.radarAltitude = radar;
        s.active.landedOrSplashed = landed;
        s.upAxis = Vector3d(0, 1, 0);
        s.velocityD = Vector3d(0, -descend, 0);
        s.loaded.push_back(s.active);
        return s;
    };
    bool safeFO = false;
    // alone: radar - 100 >= 50
    CHECK_FALSE(k.SafeToEngage(make(149.0, 0), safeFO));
    CHECK(safeFO);  // the floating origin flag only depends on the OTHER vessels
    CHECK(k.SafeToEngage(make(150.0, 0), safeFO));
    // closing so fast that the next tick's descent exceeds the margin: 3000 m/s * 0.02 s = 60 m > 50 m
    CHECK_FALSE(k.SafeToEngage(make(150.0, 3000), safeFO));
    CHECK(k.SafeToEngage(make(150.0, 2000), safeFO));  // 40 m < 50 m
    CHECK_FALSE(k.SafeToEngage(make(1e6, 0, true), safeFO));  // landed or splashed
    // dead active vessel: not safe, not safe for the floating origin
    FrameSnapshot dead = make(1e6, 0);
    dead.active.dead = true;
    CHECK_FALSE(k.SafeToEngage(dead, safeFO));
    CHECK_FALSE(safeFO);
    // two loaded vessels: altThreshold 200 applies to the active vessel; another vessel on the ground or low blocks both
    FrameSnapshot two = make(250.0, 0);
    FrameVesselState other;
    other.radarAltitude = 5000.0;
    two.loaded.push_back(other);
    CHECK_FALSE(k.SafeToEngage(two, safeFO));  // 250 - 100 = 150 < altThreshold 200 (two vessels loaded)
    CHECK(safeFO);                             // the other vessel is high enough
    two = make(300.0, 0);
    two.loaded.push_back(other);
    CHECK(k.SafeToEngage(two, safeFO));   // 300 - 100 = 200 >= 200
    two = make(299.0, 0);
    two.loaded.push_back(other);
    CHECK_FALSE(k.SafeToEngage(two, safeFO));
    FrameSnapshot lowOther = make(300.0, 0);
    other.radarAltitude = 199.0;
    lowOther.loaded.push_back(other);
    CHECK_FALSE(k.SafeToEngage(lowOther, safeFO));
    CHECK_FALSE(safeFO);
    FrameSnapshot landedOther = make(300.0, 0);
    other.radarAltitude = 5000.0;
    other.landedOrSplashed = true;
    landedOther.loaded.push_back(other);
    CHECK_FALSE(k.SafeToEngage(landedOther, safeFO));
    CHECK_FALSE(safeFO);
}

TEST_CASE("WP-20a Krakensbane + FloatingOrigin: frame velocity is a velocity offset, true motion is unchanged, nothing is shifted") {
    TimeGuard tg;
    Time::fixedDeltaTime = 0.02f;
    PhysicsWorld w;
    const Vector3d x0(600000.0, 70000.0, 0.0);
    const Vector3d V(2000.0, 0.0, 500.0);
    BodyDesc d;
    d.position = x0;
    d.linearVelocity = V;
    BodyId id = w.CreateBody(d);
    BodyDesc d2 = d;
    d2.position = x0 + Vector3d(50, 0, 0);
    BodyId id2 = w.CreateBody(d2);  // a second body (e.g. a part cluster of the same vessel) moves with the first
    BodyDesc fl;
    fl.motion = MotionType::Static;
    fl.position = Vector3d(0, 0, 0);
    BodyId stat = w.CreateBody(fl);

    Krakensbane k;
    FloatingOrigin fo;
    REQUIRE(k.Awake());
    fo.Awake();
    fo.ready = [] { return true; };
    int engage = 0, disengage = 0;
    Vector3d engageArg, disengageArg;
    struct Sub {
        int* e; int* d; Vector3d* ea; Vector3d* da;
        void OnE(Vector3d v) { ++*e; *ea = v; }
        void OnD(Vector3d v) { ++*d; *da = v; }
    } sub{&engage, &disengage, &engageArg, &disengageArg};
    GameEvents::onKrakensbaneEngage.Add(&sub, &Sub::OnE);
    GameEvents::onKrakensbaneDisengage.Add(&sub, &Sub::OnD);
    bool landed = false;
    k.host.snapshot = [&] {
        FrameSnapshot s;
        s.ready = s.hasActiveVessel = s.activeHasRigidbody = true;
        s.active.isActive = true;
        s.active.landedOrSplashed = landed;
        s.active.radarAltitude = 70000.0;
        s.rb_velocityD = w.GetLinearVelocity(id);
        s.velocityD = ReferenceFrame::TrueVelocity(s.rb_velocityD);
        s.upAxis = Vector3d(0, 1, 0);
        s.loaded.push_back(s.active);
        return s;
    };
    k.host.changeWorldVelocity = [&](size_t, const Vector3d& dv) { w.AddVelocityChangeToAllMoving(dv); };

    // tick 1: the vessel is faster than MaxV and safe: the whole rb velocity becomes the frame velocity (AddExcess)
    k.FixedUpdate();
    fo.FixedUpdate();
    CHECK(Krakensbane::GetFrameVelocity().x == 2000.0);
    CHECK(Krakensbane::GetFrameVelocity().z == 500.0);
    CHECK(Krakensbane::GetLastCorrection().x == -2000.0);
    CHECK(engage == 1);
    CHECK(engageArg.x == -2000.0);
    CHECK(w.GetLinearVelocity(id).magnitude() == 0.0);
    CHECK(w.GetLinearVelocity(id2).magnitude() == 0.0);
    CHECK(w.GetLinearVelocity(stat).magnitude() == 0.0);  // static bodies untouched
    CHECK(Krakensbane::GetFrameVelocityV3f().x == 2000.0f);

    const double dt = static_cast<double>(Time::fixedDeltaTime);
    const int n = 500;
    for (int i = 0; i < n; ++i) {
        k.FixedUpdate();   // velocityD still > MaxV: AddExcess(rb_velocityD = 0): nothing changes
        fo.FixedUpdate();
        w.FixedStep();
    }
    CHECK(engage == 1);
    // nothing moved in the physics frame; the drift is exactly the accumulated FrameVel * dt (one ADD per tick, not a shift)
    CHECK(w.GetPosition(id).x == x0.x);
    CHECK(w.GetPosition(id).y == x0.y);
    CHECK(w.GetPosition(id).z == x0.z);
    const Vector3d off = fo.FrameOffset();
    const double ticks = n + 1;  // tick 1 contributes too
    CHECK(off.x == doctest::Approx(2000.0 * dt * ticks).epsilon(1e-12));
    CHECK(off.z == doctest::Approx(500.0 * dt * ticks).epsilon(1e-12));
    // the true position of the body is x0 + V t (t = ticks * dt) to double rounding
    const Vector3d truePos = ReferenceFrame::TruePosition(w.GetPosition(id));
    CHECK(truePos.x == doctest::Approx(x0.x + V.x * dt * ticks).epsilon(1e-12));
    CHECK(truePos.z == doctest::Approx(x0.z + V.z * dt * ticks).epsilon(1e-12));
    CHECK(ReferenceFrame::TrueVelocity(w.GetLinearVelocity(id)).x == 2000.0);
    CHECK(ReferenceFrame::PhysicsPosition(truePos).x == doctest::Approx(x0.x).epsilon(1e-12));
    // the floating origin offsets the shift would have set stay zero
    CHECK(FloatingOrigin::Offset().magnitude() == 0.0);

    // slowing under MaxV (here: the vessel lands) disengages: the velocity is given back to the bodies
    landed = true;
    k.FixedUpdate();
    CHECK(disengage == 1);
    CHECK(disengageArg.x == 2000.0);
    CHECK(Krakensbane::GetFrameVelocity().IsZero());
    CHECK(w.GetLinearVelocity(id).x == 2000.0f);
    CHECK(w.GetLinearVelocity(id2).z == 500.0f);
    CHECK(Krakensbane::GetLastCorrection().x == 2000.0);
    GameEvents::onKrakensbaneEngage.Remove(&sub, &Sub::OnE);
    GameEvents::onKrakensbaneDisengage.Remove(&sub, &Sub::OnD);
}

TEST_CASE("WP-20a Krakensbane: a second instance is destroyed; without an instance the readers are zero; packed resets") {
    Krakensbane a;
    REQUIRE(a.Awake());
    Krakensbane b;
    CHECK_FALSE(b.Awake());
    CHECK(Krakensbane::Instance() == &a);
    CHECK(Krakensbane::SqrThreshold() == 25.0);
    a.FrameVel = Vector3d(1, 2, 3);
    CHECK(Krakensbane::GetFrameVelocity().y == 2.0);
    Krakensbane::ResetVelocityFrame(false);
    CHECK(a.FrameVel.IsZero());
    a.OnDestroy();
    CHECK(Krakensbane::Instance() == nullptr);
    CHECK(Krakensbane::GetFrameVelocity().IsZero());

    // packed active vessel: FrameVel and excessV are zeroed (Krakensbane.cs:76-80)
    Krakensbane c;
    REQUIRE(c.Awake());
    c.FrameVel = Vector3d(5, 6, 7);
    c.excessV = Vector3d(1, 1, 1);
    c.host.snapshot = [] {
        FrameSnapshot s;
        s.ready = s.hasActiveVessel = s.activeHasRigidbody = true;
        s.active.isActive = true;
        s.active.packed = true;
        s.loaded.push_back(s.active);
        return s;
    };
    FloatingOrigin fo;
    fo.Awake();
    c.FixedUpdate();
    CHECK(c.FrameVel.IsZero());
    CHECK(c.excessV.IsZero());
}

TEST_CASE("WP-20a frame: Kepler orbit in the inertial frame, 1 revolution, against the analytic circle") {
    // 100 km orbit around Kerbin (r = 700 km), 0.02 s ticks, one body, gravity at the true position each tick. The body is a float
    // solver (velocities in float): without a frame velocity the orbital speed (2246 m/s) has a 2.4e-4 m/s ulp, so the orbit
    // degrades with the rounding of every gravity increment; with Krakensbane's frame velocity the physics-frame velocity stays
    // small and the orbit is held. Thresholds are set from the measured values (below and STATUS.md WP-20a), with margin.
    double T = 0;
    const OrbitResult plain = RunOrbit(700000.0, 1.0, false, 0.0, &T);
    const OrbitResult frame = RunOrbit(700000.0, 1.0, true, 0.0);
    MESSAGE("T " << T << " plain: radius err " << plain.maxRadiusErr << " end err " << plain.endPosErr);
    MESSAGE("frame: radius err " << frame.maxRadiusErr << " end err " << frame.endPosErr << " writes " << frame.velocityWrites);
    CHECK(T == doctest::Approx(1958.0).epsilon(0.002));
    // Measured (RelWithDebInfo, MSVC 19.4x, AVX2): plain radius 23.7 m / end 8.0 m; frame velocity radius 22.5 m / end 0.20 m.
    // The ~22 m radius is the O(dt) offset of semi-implicit Euler at dt = 0.02 s (the spike's 23 m), identical in both; the
    // end position is where the float velocity matters: 8 m without the frame velocity, 0.2 m with it.
    CHECK(frame.maxRadiusErr < 30.0);
    CHECK(plain.maxRadiusErr < 30.0);
    CHECK(frame.endPosErr < 2.0);
    CHECK(plain.endPosErr < 40.0);
    CHECK(frame.endPosErr < plain.endPosErr);
    CHECK(frame.maxFrameDev < 1.0);   // the physics-frame velocity never exceeds one tick of gravity (7.2 m/s^2 * 0.02 s)
    CHECK(frame.velocityWrites > 0);  // one velocity write per tick (KSP's rule)
}

TEST_CASE("WP-20a frame: re-centring the frame velocity only above a speed gives the same orbit with far fewer velocity writes") {
    const OrbitResult every = RunOrbit(700000.0, 1.0, true, 0.0);
    const OrbitResult rare = RunOrbit(700000.0, 1.0, true, 50.0);
    MESSAGE("every tick: " << every.velocityWrites << " writes, radius err " << every.maxRadiusErr << ", end err " << every.endPosErr);
    MESSAGE("recentre 50 m/s: " << rare.velocityWrites << " writes, radius err " << rare.maxRadiusErr << ", end err " << rare.endPosErr);
    CHECK(rare.velocityWrites * 100 < every.velocityWrites);
    CHECK(rare.maxFrameDev < 51.0 + 8.0);
    CHECK(rare.maxRadiusErr < 30.0);
    CHECK(rare.endPosErr < 2.0);
}

TEST_CASE("WP-20a frame: rotating <-> inertial decision and velocity re-expression (OrbitPhysicsManager.cs:234-343)") {
    CelestialBody kerbin;
    kerbin.rotates = true;
    kerbin.inverseRotThresholdAltitude = 70000.0f;
    const double w = 2.0 * 3.14159265358979323846 / 21549.425;
    kerbin.angularVelocity = Vector3d(0, -w, 0);  // CelestialBody.cs: Vector3d.down * (2 pi / rotationPeriod)
    kerbin.position = Vector3d(0, 0, 0);

    // decision
    CHECK(ReferenceFrame::RotatingFrameTransition(kerbin, 10000.0) == +1);  // low and inertial: switch to body-fixed
    kerbin.inverseRotation = true;
    CHECK(ReferenceFrame::RotatingFrameTransition(kerbin, 10000.0) == 0);
    CHECK(ReferenceFrame::RotatingFrameTransition(kerbin, 70000.0) == 0);  // exactly at the threshold: no change
    CHECK(ReferenceFrame::RotatingFrameTransition(kerbin, 70000.5) == -1);
    kerbin.inverseRotation = false;
    CHECK(ReferenceFrame::RotatingFrameTransition(kerbin, 70000.5) == 0);
    kerbin.rotates = false;
    CHECK(ReferenceFrame::RotatingFrameTransition(kerbin, 10000.0) == 0);
    kerbin.rotates = true;

    // fictitious accelerations: hand-derived for omega = (0, -w, 0), r = (R, 0, 0), v = (100, 0, 0)
    const double R = 600000.0;
    CHECK(ReferenceFrame::getCoriolisAcc(Vector3d(100, 0, 0), kerbin).IsZero());      // inertial: none
    CHECK(ReferenceFrame::getCentrifugalAcc(Vector3d(R, 0, 0), kerbin).IsZero());
    kerbin.inverseRotation = true;
    const Vector3d cor = ReferenceFrame::getCoriolisAcc(Vector3d(100, 0, 0), kerbin);
    CHECK(cor.x == 0.0);
    CHECK(cor.y == 0.0);
    CHECK(cor.z == doctest::Approx(-200.0 * w).epsilon(1e-14));
    const Vector3d cen = ReferenceFrame::getCentrifugalAcc(Vector3d(R, 0, 0), kerbin);
    CHECK(cen.x == doctest::Approx(w * w * R).epsilon(1e-14));  // outward
    CHECK(cen.y == 0.0);
    CHECK(cen.z == 0.0);
    kerbin.inverseRotation = false;

    // setRotatingFrame: every body of the group loses / regains omega x r of the GROUP position (vessel.transform.position)
    PhysicsWorld world;
    BodyDesc d;
    d.position = Vector3d(R, 0, 0);
    d.linearVelocity = Vector3d(0, 0, 2300.0);
    BodyId a = world.CreateBody(d);
    d.position = Vector3d(R, 3.0, 0);
    BodyId b = world.CreateBody(d);
    FrameRebaseGroup g;
    g.position = Vector3d(R, 0, 0);
    g.bodies = {a, b};
    Vector3d kacc(0, 0, 0);
    g.krakensbaneAcc = &kacc;
    int transitions = 0;
    bool lastState = false;
    struct Sub {
        int* n; bool* s;
        void On(GameEvents::HostTargetAction<CelestialBody*, bool> x) { ++*n; *s = x.target; }
    } sub{&transitions, &lastState};
    GameEvents::onRotatingFrameTransition.Add(&sub, &Sub::On);
    float gee = 0;
    // omega x r with omega = (0,-w,0), r = (R,0,0) is (0, 0, w R) = (0, 0, 435.1 m/s): the ground speed at the equator
    const double rot = w * R;
    CHECK(ReferenceFrame::CheckReferenceFrame(kerbin, 5000.0, world, {g}, [&](float s) { gee = s; }) == +1);
    CHECK(kerbin.inverseRotation);
    CHECK(gee == 0.5f);
    CHECK(transitions == 1);
    CHECK(lastState);
    CHECK(world.GetLinearVelocity(a).z == doctest::Approx(2300.0 - rot).epsilon(1e-6));
    CHECK(world.GetLinearVelocity(b).z == doctest::Approx(2300.0 - rot).epsilon(1e-6));  // same group position: same offset
    CHECK(kacc.z == doctest::Approx(-rot).epsilon(1e-12));
    CHECK(ReferenceFrame::CheckReferenceFrame(kerbin, 5000.0, world, {g}) == 0);  // no change, no event
    CHECK(transitions == 1);
    CHECK(ReferenceFrame::CheckReferenceFrame(kerbin, 90000.0, world, {g}) == -1);
    CHECK_FALSE(kerbin.inverseRotation);
    CHECK(world.GetLinearVelocity(a).z == doctest::Approx(2300.0).epsilon(1e-6));
    CHECK(std::fabs(kacc.z) < 1e-9);
    CHECK(transitions == 2);
    CHECK_FALSE(lastState);
    GameEvents::onRotatingFrameTransition.Remove(&sub, &Sub::On);
}

TEST_CASE("WP-20a frame: the same orbit simulated in the BODY-FIXED frame (Coriolis + centrifugal) matches the inertial analytic orbit") {
    // Kerbin rotates about -Y; an orbit in the x-z plane. Start at rest-frame state of the inertial circle at t = 0, integrate
    // a quarter of the orbit in the rotating frame with getCoriolisAcc / getCentrifugalAcc, and compare with the inertial circle
    // rotated back into the body-fixed frame (Rodrigues rotation by omega t about the rotation axis).
    TimeGuard tg;
    Time::fixedDeltaTime = 0.02f;
    CelestialBody kerbin;
    kerbin.rotates = true;
    kerbin.inverseRotation = true;
    const double w = 2.0 * 3.14159265358979323846 / 21549.425;
    kerbin.angularVelocity = Vector3d(0, -w, 0);
    const double r0 = 700000.0;
    const double v0 = std::sqrt(kMuKerbin / r0);
    const Vector3d p0(r0, 0, 0);
    const Vector3d vIn(0, 0, v0);
    const Vector3d vRot = vIn - kerbin.getRFrmVel(p0);  // velocity seen in the body-fixed frame

    PhysicsWorld world;
    BodyDesc d;
    d.shape = ShapeDesc::Box(Vector3f(1, 1, 1));
    d.mass = 1000.0f;
    d.position = p0;
    d.linearVelocity = vRot;
    BodyId id = world.CreateBody(d);
    // the physics-frame velocity of this body is the (large) rotating-frame velocity; use a frame velocity as the flight code would
    const double T = 2.0 * 3.14159265358979323846 * std::sqrt(r0 * r0 * r0 / kMuKerbin);
    const double dt = static_cast<double>(Time::fixedDeltaTime);
    const uint64_t steps = static_cast<uint64_t>(0.25 * T / dt);
    for (uint64_t n = 0; n < steps; ++n) {
        const Vector3d p = world.GetPosition(id);
        const Vector3d v = world.GetLinearVelocity(id);
        const double r = p.magnitude();
        Vector3d a = p * (-kMuKerbin / (r * r * r)) + ReferenceFrame::getCoriolisAcc(v, kerbin) + ReferenceFrame::getCentrifugalAcc(p, kerbin);
        world.AddForce(id, a * 1000.0);
        world.FixedStep();
    }
    const double t = static_cast<double>(steps) * dt;
    const double th = v0 / r0 * t;
    const Vector3d inertial(r0 * std::cos(th), 0.0, r0 * std::sin(th));
    // body-fixed coordinates: rotate the inertial vector by -(angle w t) about the rotation axis n = (0,-1,0)
    const Vector3d expect = Rotate(inertial, Vector3d(0, -1, 0), -w * t);
    const Vector3d got = world.GetPosition(id);
    MESSAGE("quarter orbit " << t << " s: error " << (got - expect).magnitude() << " m");
    // measured 42.6 m (the 22 m Euler radius offset plus the rotating-frame terms); Kerbin's equator moves 435 m/s
    CHECK((got - expect).magnitude() < 100.0);
    CHECK(std::fabs(got.magnitude() - r0) < 40.0);
}
