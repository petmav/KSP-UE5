// WP-19b unit tests: frames and the physics bubble (ksp/physics/physics_bubble.hpp) on the reduced model.
//
// No golden or capture covers rigid bodies in the flight scene (PhysX is not reproducible), so the gate is: the bit-exact pieces against
// the independent Python reference (tests/test_physics_bubble_ref.cpp), and here the scenarios with analytic references: the frame
// policy (float fields at KSP precision, a world rebase changes no physics state), two vessels in one bubble (load at 2.25 km, unpack
// at 200 m, both coast; the relative position against the two Kepler orbits), the rotating <-> inertial switch and the SOI change of the
// bubble in physics (continuity), Krakensbane's frame drift folded back with the static geometry gated, the physics-origin rebase, and
// the bindings' lifecycle.
#include <algorithm>
#include <cmath>

#include "doctest/doctest.h"
#include "ksp/physics/floating_origin.hpp"
#include "ksp/physics/krakensbane.hpp"
#include "ksp/physics/physical_object.hpp"
#include "physics_bubble_fixture.hpp"

using namespace ksp;
using namespace ksp::physics;
using namespace wp19b;

namespace {

double MaxAbs(const Vector3d& v) { return std::max(std::fabs(v.x), std::max(std::fabs(v.y), std::fabs(v.z))); }

}  // namespace

// STATUS.md WP-20b risk R1: CalculatePhysicsStats computes `CoM = CoMD; localCoM = vesselTransform.InverseTransformPoint(CoM)` in floats.
// kspcore's world has the dominant body at the origin, so a vessel 720 km out has 6 cm floats. The bubble's frame policy keeps ONE
// world frame (floatOrigin 0) and rebases its origin to the active vessel (FloatingOrigin's rule): the float fields get KSP's precision,
// and the rebase moves no physics body.
TEST_CASE("WP-19b frame policy: one world frame, origin rebased to the active vessel; CalculatePhysicsStats' floats at KSP precision, no physics body moves") {
    if (!wp20b::GameAvailable()) return;
    World w;
    BubbleScene s(w);
    auto v = w.Craft("PhysProf-S");
    CelestialBody& kerbin = w.Kerbin();
    Vector3d p0, v0;
    CircularStateAt(kerbin, kerbin.Radius + 120000.0, Vector3d(0.6, 0.48, -0.64), Vector3d(0.3, -0.9, 0.2), p0, v0);
    const double qn = std::sqrt(0.2 * 0.2 + 0.3 * 0.3 + 0.4 * 0.4 + 0.84 * 0.84);
    v->SetRotation(Quaternionf(static_cast<float>(0.2 / qn), static_cast<float>(-0.3 / qn), static_cast<float>(0.4 / qn),
                               static_cast<float>(0.84 / qn)));  // a vessel not aligned with the world axes
    PlaceOnOrbit(*v, kerbin, p0, v0, true);
    s.Start(*v, false);  // the world origin stays at Kerbin's centre for the first half
    v->GoOffRails();
    VesselPhysics* vp = s.bubble.Host().Find(*v);
    REQUIRE(vp != nullptr);
    CHECK(vp->floatOrigin.IsZero());  // the transform frame is the world frame (KSP's convention)
    const Vector3d exactLocalCoM = VesselFrameCom(*v);
    // before the rebase: the vessel's floats sit 720 km from the origin
    v->CalculatePhysicsStats();
    const double errFar = (Vector3d(v->localCoM) - exactLocalCoM).magnitude();
    const Vector3d comRelBefore = v->CoMD - kerbin.position;
    const Vector3d velBefore = v->rb_velocityD;
    const Vector3f moiBefore = v->MOI;
    const uint64_t hashBefore = s.world.StateHash();
    const Vector3d kerbinBefore = kerbin.position;
    // FloatingOrigin.SetOffset(active vessel position): the bubble's ShiftWorld
    const Vector3d offset(v->vesselTransform.position);
    FloatingOrigin::SetOffset(offset);
    CHECK(s.bubble.FloatingOriginInstance().ShiftCount() == 1);
    CHECK(s.world.StateHash() == hashBefore);                         // no Jolt body moved
    CHECK(SameBits(kerbin.position, kerbinBefore - offset));          // FloatingOrigin.cs:184
    CHECK(SameBits(s.bubble.FrameOriginWorld(), kerbinBefore - offset));
    CHECK(Vector3d(v->vesselTransform.position).magnitude() < 0.1);  // the vessel is at the origin now (the float offset's rounding)
    v->CalculatePhysicsStats();
    const double errNear = (Vector3d(v->localCoM) - exactLocalCoM).magnitude();
    // the physics state read back is the same one (relative to the body; positions in doubles: rounding of the shifted sums only)
    CHECK(((v->CoMD - kerbin.position) - comRelBefore).magnitude() < 1e-8);
    CHECK(SameBits(v->rb_velocityD, velBefore));
    CHECK(std::fabs(v->MOI.x - moiBefore.x) <= 1e-5f * std::fabs(moiBefore.x));
    MESSAGE("localCoM error: " << errFar << " m with the vessel 720 km from the world origin, " << errNear << " m after the rebase");
    CHECK(errNear < 1e-5);
    CHECK(errFar > 100.0 * errNear);
}

// The world rebase by KSP's rule inside the tick: a coasting vessel is rebased whenever it is 500 m from the origin (FloatingOrigin.cs:139),
// its transform never farther than threshold + one tick of motion; the trajectory relative to the body is the one of a run without rebases.
TEST_CASE("WP-19b FloatingOrigin in the tick: rebases by the 500 m rule keep the active vessel near the origin and do not change the trajectory") {
    if (!wp20b::GameAvailable()) return;
    struct Run {
        std::vector<Vector3d> rel;
        size_t shifts = 0;
        float maxTransform = 0.0f;
    };
    auto run = [](bool rebase) {
        Run r;
        World w;
        BubbleScene s(w);
        auto v = w.Craft("PhysProf-S");
        CelestialBody& kerbin = w.Kerbin();
        Vector3d p0, v0;
        CircularState(kerbin, kerbin.Radius + 120000.0, p0, v0);
        PlaceOnOrbit(*v, kerbin, p0, v0, true);
        s.Start(*v);  // both runs start with the world origin at the vessel (FlightDriver.Start)
        if (!rebase) s.bubble.FloatingOriginInstance().activeVesselPosition = nullptr;  // WP-20a's behaviour: no rebase decision
        for (int k = 0; k < 600; ++k) {
            s.Tick();
            if (k >= 2) r.rel.push_back(v->CoMD - kerbin.position);
            if (k >= 2) r.maxTransform = std::max(r.maxTransform, v->vesselTransform.position.magnitude());
        }
        r.shifts = s.bubble.FloatingOriginInstance().ShiftCount();
        return r;
    };
    const Run with = run(true);
    const Run without = run(false);
    REQUIRE(with.rel.size() == without.rel.size());
    double maxDiff = 0;
    for (size_t i = 0; i < with.rel.size(); ++i) maxDiff = std::max(maxDiff, (with.rel[i] - without.rel[i]).magnitude());
    MESSAGE("12 s at 120 km: " << with.shifts << " world rebases, vessel transform <= " << with.maxTransform << " m from the origin (without: "
                               << without.maxTransform << " m); trajectories differ by " << maxDiff << " m");
    CHECK(with.shifts > 20);
    CHECK(without.shifts == 1);  // FlightDriver.Start's only
    CHECK(with.maxTransform < 600.0f);  // threshold 500 m + one tick at 2.2 km/s (45 m)
    CHECK(without.maxTransform > 2.0e4f);  // 12 s at 2.2 km/s away from the origin
    CHECK(maxDiff < 1e-5);  // the same physics; only the rounding of the world sums differs
}

// PHYSICS-SPIKE.md section 7, WP-19b scenario `two_vessels`: a second vessel loads at 2.25 km, unpacks at 200 m, both coast; relative
// position error < 1 mm. B starts unloaded 3 km behind the active vessel A on a Clohessy-Wiltshire transfer that passes 40 m below A after
// 300 s; the reference is an independent two-body propagation of both initial states (TwoBody), not kspcore's Orbit.
TEST_CASE("WP-19b two vessels in one bubble: load at 2.25 km, unpack at 200 m, both coast; relative position within 1 mm of the Kepler orbits; pack at 350 m") {
    if (!wp20b::GameAvailable()) return;
    World w;
    BubbleScene s(w);
    auto a = w.Craft("PhysProf-S");
    auto b = w.Craft("PhysProf-S");
    CelestialBody& kerbin = w.Kerbin();
    const double mu = kerbin.gravParameter;
    const double r = kerbin.Radius + 120000.0;  // above the rotating-frame threshold (100 km): world axes inertial, the reference too
    const double n = std::sqrt(mu / (r * r * r));
    Vector3d pA, vA;
    CircularStateAt(kerbin, r, Vector3d(0.6, 0.48, -0.64), Vector3d(0.3, -0.9, 0.2), pA, vA);
    const Vector3d xh = pA.normalized(), yh = vA.normalized(), zh = Vector3d::Cross(xh, yh);
    // CW transfer from (x0, y0) = (0, -3000) to (xT, yT) = (-40, 0) in T = 300 s
    const double T = 300.0, y0 = -3000.0, xT = -40.0, yT = 0.0;
    const double s_ = std::sin(n * T), c_ = std::cos(n * T);
    const double m11 = s_ / n, m12 = 2.0 / n * (1.0 - c_), m21 = -2.0 / n * (1.0 - c_), m22 = (4.0 * s_ - 3.0 * n * T) / n;
    const double det = m11 * m22 - m12 * m21;
    const double rx = xT, ry = yT - y0;
    const double vx0 = (rx * m22 - m12 * ry) / det, vy0 = (m11 * ry - m21 * rx) / det;
    const Vector3d relPos = yh * y0;
    const Vector3d pB = pA + relPos;
    const Vector3d vB = vA + xh * vx0 + yh * vy0 + Vector3d::Cross(zh * n, relPos);  // Hill-frame velocity -> inertial
    PlaceOnOrbit(*a, kerbin, pA, vA, true);
    PlaceOnOrbit(*b, kerbin, pB, vB, true);
    const double ut0 = Planetarium::GetUniversalTime();
    const TwoBody refA{mu, ut0, pA, vA}, refB{mu, ut0, pB, vB};
    b->Unload();  // B starts unloaded (its ProtoVessel keeps the parts), 3 km away
    REQUIRE_FALSE(b->loaded);
    s.Start(*a);
    // KSP's two clocks: the UT advances by Planetarium.fixedDeltaTime = 0.02 (double, Planetarium.cs:82 / :205) per tick, the physics by
    // Time.fixedDeltaTime = 0.02f = 0.0199999995529... (TimeWarp.cs:423). VesselPrecalculate's drift compensation moves a vessel by
    // TimeWarp.fixedDeltaTime along its Kepler orbit each tick, so a vessel in physics runs on the PHYSICS clock: its reference is the Kepler
    // orbit at t = UT(unpack) + ticks * (double)0.02f. (The UT-clocked rails position runs ahead by 4.47e-10 s per tick, 1 um per tick at
    // 2.2 km/s: KSP's own behaviour, reported below.)
    const double dtf = static_cast<double>(Time::fixedDeltaTime);
    double loadDist = -1, unpackDist = -1, packDist = -1, maxRel = 0, maxA = 0, maxAut = 0, maxRelTransform = 0, railsJump = -1;
    int bothTicks = 0, ticks = 0, kB = -1;
    double utA0 = 0, utB0 = 0;
    bool wasLoaded = false, wasUnpacked = false;
    const int maxTicks = static_cast<int>((T + 80.0) / 0.02);
    for (int k = 0; k < maxTicks && packDist < 0; ++k) {
        s.Tick();
        ++ticks;
        const double ut = Planetarium::GetUniversalTime();
        if (k == 0) utA0 = ut;  // A unpacked in the LateUpdate of the first tick, at this UT's rails state
        const double tA = utA0 + (k * dtf);
        const Vector3d ra = a->CoMD - kerbin.position;
        if (k >= 1) {
            maxA = std::max(maxA, (ra - refA.At(tA)).magnitude());
            maxAut = std::max(maxAut, (ra - refA.At(ut)).magnitude());
        }
        const double dist = (Vector3d(b->vesselTransform.position) - Vector3d(a->vesselTransform.position)).magnitude();
        if (b->loaded && !wasLoaded) {
            wasLoaded = true;
            loadDist = dist;
        }
        if (!b->packed && !wasUnpacked) {
            wasUnpacked = true;
            unpackDist = dist;
            kB = k;
            utB0 = ut;
            CHECK(s.bubble.Host().Find(*b) != nullptr);
        } else if (wasUnpacked && !b->packed) {
            // both in physics: B's centre of mass relative to A's against the two Kepler orbits on their physics clocks
            const double tB = utB0 + ((k - kB) * dtf);
            const Vector3d rel = b->CoMD - a->CoMD;
            const Vector3d relRef = refB.At(tB) - refA.At(tA);
            maxRel = std::max(maxRel, (rel - relRef).magnitude());
            maxRelTransform = std::max(maxRelTransform, MaxAbs(Vector3d(b->vesselTransform.position)));
            ++bothTicks;
        }
        if (wasUnpacked && b->packed && packDist < 0) {
            packDist = dist;
            CHECK(s.bubble.Host().Find(*b) == nullptr);  // GoOnRails took its bodies
            // VesselPrecalculate.GoOnRails: CalculatePhysicsStats from the bodies, then the TrackRigidbody refit: the rails orbit continues
            // from the last physics state
            Vector3d rp, rv;
            b->orbit().GetOrbitalStateVectorsAtUT(b->orbitDriver->lastTrackUT, rp, rv);
            railsJump = (rp.xzy() - (b->CoMD - kerbin.position)).magnitude();
        }
    }
    MESSAGE("two vessels: B loaded at " << loadDist << " m, unpacked at " << unpackDist << " m, packed at " << packDist << " m; both in physics "
                                        << bothTicks << " ticks; relative position error <= " << maxRel << " m; A vs its Kepler orbit <= " << maxA
                                        << " m on the physics clock (" << ticks << " ticks; " << maxAut
                                        << " m on the UT clock: KSP's 0.02 vs 0.02f); B's rails orbit continues " << railsJump
                                        << " m from its last physics state");
    CHECK(loadDist > 2200.0);
    CHECK(loadDist <= 2250.0);
    CHECK(unpackDist > 150.0);
    CHECK(unpackDist < 200.0);
    CHECK(packDist > 350.0);
    CHECK(packDist < 400.0);
    CHECK(bothTicks > 1000);
    CHECK(maxRel < 1e-3);
    CHECK(maxA < 1e-3);
    CHECK(maxRelTransform < 1000.0);  // B's floats near the world origin too (the bubble rebases on the active vessel)
    CHECK(railsJump >= 0.0);
    CHECK(railsJump < 1e-6);
}

namespace {

// An elliptic Kerbin orbit (periapsis 90 km, apoapsis 200 km, inclined) at radius `r`, ascending or descending, in world axes.
void EllipseState(const CelestialBody& body, double r, bool ascending, Vector3d& pos, Vector3d& vel) {
    const double rp = body.Radius + 90000.0, ra = body.Radius + 200000.0;
    const double e = (ra - rp) / (ra + rp), p = 2.0 * ra * rp / (ra + rp);
    double nu = std::acos((p / r - 1.0) / e);
    if (!ascending) nu = -nu;
    const Vector3d u = Vector3d(0.6, 0.48, -0.64).normalized();
    const Vector3d t = Vector3d::Cross(Vector3d(0.3, -0.9, 0.2).normalized(), u).normalized();
    const double h = std::sqrt(body.gravParameter / p);
    pos = (u * std::cos(nu) + t * std::sin(nu)) * r;
    vel = (u * -std::sin(nu) + t * (e + std::cos(nu))) * h;
}

Vector3d InertialFromWorld(const Vector3d& w) { return Planetarium::Zup.LocalToWorld(w.xzy()); }

struct SwitchRun {
    int switchTick = -1;
    double errBefore = 0, errEnd = 0, quirk = 0, dt = 0, jumpResidual = 0, jump = 0, postDrift = 0;
    Vector3d dvApplied, rFrmVelRoot;
    bool eventState = false;
    int events = 0;
    double posJump = 0;
};

// The vessel coasts through Kerbin's inverse-rotation threshold (100 km) in physics with the full bubble tick; OrbitPhysicsManager's
// checkReferenceFrame switches the frame and the bubble re-expresses the bodies' velocities (OrbitPhysicsManager.cs:235-302).
SwitchRun RunSwitch(bool ascending) {
    SwitchRun res;
    World w;
    BubbleScene s(w);
    auto v = w.Craft("PhysProf-S");
    CelestialBody& kerbin = w.Kerbin();
    Vector3d p0, v0;
    EllipseState(kerbin, kerbin.Radius + (ascending ? 99600.0 : 100400.0), ascending, p0, v0);
    const double qn = std::sqrt(0.2 * 0.2 + 0.3 * 0.3 + 0.4 * 0.4 + 0.84 * 0.84);
    v->SetRotation(Quaternionf(static_cast<float>(0.2 / qn), static_cast<float>(-0.3 / qn), static_cast<float>(0.4 / qn),
                               static_cast<float>(0.84 / qn)));  // root - CoM not along the rotation axis: KSP's omega x r_root rule shows
    PlaceOnOrbit(*v, kerbin, p0, v0, true);
    const Vector3d p0i = InertialFromWorld(p0), v0i = InertialFromWorld(v0);  // the frame is inertial at the set-up
    s.Start(*v);
    // wrap the bubble's setRotatingFrame: the cluster velocity right before and after, and KSP's rFrmVel at vessel.transform.position
    auto inner = OrbitPhysicsManager::env.setRotatingFrame;
    OrbitPhysicsManager::env.setRotatingFrame = [&](OrbitPhysicsManager& opm, bool rotating) {
        VesselPhysics* vp = s.bubble.Host().Find(*v);
        const Vector3d before = vp != nullptr ? vp->ClusterVelocity(0) : Vector3d();
        res.rFrmVelRoot = opm.dominantBody->getRFrmVel(Vector3d(v->vesselTransform.position));
        inner(opm, rotating);
        if (vp != nullptr) {
            res.dvApplied = vp->ClusterVelocity(0) - before;
            // KSP's rule moves every part by omega x r_root; the rigid-body truth is omega x r_com: the velocity error KSP accepts
            const Vector3d com = vp->WorldFromPhysics(vp->ClusterCenterOfMass(0));
            res.quirk = (opm.dominantBody->getRFrmVel(com) - res.rFrmVelRoot).magnitude();
        }
    };
    const auto hev = decltype(GameEvents::onRotatingFrameTransition)::OnEvent::Lambda(&res, [&](GameEvents::HostTargetAction<CelestialBody*, bool> d) {
        res.events++;
        res.eventState = d.target;
    });
    GameEvents::onRotatingFrameTransition.Add(hev);
    // the vessel unpacks in the LateUpdate of the first tick at the rails state of that tick's UT (ut0 + 0.02), then runs on the physics
    // clock (0.02f per tick, see the two-vessel test)
    const TwoBody ref{kerbin.gravParameter, -0.02, p0i, v0i};
    const double dtf = static_cast<double>(Time::fixedDeltaTime);
    const bool initialRotating = ascending;  // set by the first OrbitPhysicsManager.FixedUpdate (below 100 km: rotating)
    Vector3d lastRel, lastErr;
    TwoBody post;
    for (int k = 0; k < 1000; ++k) {
        const bool was = kerbin.inverseRotation;
        s.Tick();
        const Vector3d rel = v->CoMD - kerbin.position;
        const Vector3d errV = InertialFromWorld(rel) - ref.At(k * dtf);
        if (k >= 1 && was != kerbin.inverseRotation) {
            res.switchTick = k;
            res.posJump = (InertialFromWorld(rel) - InertialFromWorld(lastRel)).magnitude();  // one tick of motion, no jump
            // the switch happens in OrbitPhysicsManager.FixedUpdate (seq 280), after Planetarium.FixedUpdate set this tick's frame (seq 6):
            // the step of the switch tick runs the new frame's velocities in the old frame's axes, which displaces the trajectory by
            // -/+ (omega x r) dt once (the C#'s order: STATUS.md WP-19b)
            const Vector3d predicted = InertialFromWorld(kerbin.getRFrmVel(v->CoMD) * 0.02) * (ascending ? 1.0 : -1.0);
            res.jump = (errV - lastErr).magnitude();
            res.jumpResidual = ((errV - lastErr) - predicted).magnitude();
        }
        if (k >= 1 && res.switchTick < 0) res.errBefore = std::max(res.errBefore, errV.magnitude());
        if (res.switchTick >= 0 && k == res.switchTick + 1) {
            // after the switch the vessel coasts on the Kepler orbit of its state (inertial velocity: + omega x r in the rotating frame)
            const Vector3d vIn = v->velocityD + (kerbin.inverseRotation ? kerbin.getRFrmVel(v->CoMD) : Vector3d());
            post = TwoBody{kerbin.gravParameter, k * dtf, InertialFromWorld(rel), InertialFromWorld(vIn)};
        }
        if (res.switchTick >= 0 && k > res.switchTick + 1) res.postDrift = std::max(res.postDrift, (InertialFromWorld(rel) - post.At(k * dtf)).magnitude());
        res.errEnd = errV.magnitude();
        lastRel = rel;
        lastErr = errV;
        if (k == 0) {
            CHECK(kerbin.inverseRotation == initialRotating);
            res.events = 0;  // the first checkReferenceFrame's switch to the rotating frame (ascending case) is the set-up's
        }
    }
    res.dt = (1000 - res.switchTick) * dtf;
    GameEvents::onRotatingFrameTransition.Remove(hev);
    return res;
}

}  // namespace

// WP-20a's `frame_switch` with vessels: velocity and position continuous across the rotating <-> inertial switch (OrbitPhysicsManager.cs:235,
// :322). Reference: an independent two-body propagation of the initial state in the inertial frame (Planetarium.Zup converts the world
// axes, which turn with the body while it is in the rotating frame).
TEST_CASE("WP-19b frame_switch: a vessel crossing the 100 km inverse-rotation threshold in physics, both ways; the bodies' velocities re-expressed as KSP does") {
    if (!wp20b::GameAvailable()) return;
    for (const bool ascending : {true, false}) {
        CAPTURE(ascending);
        const SwitchRun r = RunSwitch(ascending);
        MESSAGE((ascending ? "ascending (rotating -> inertial)" : "descending (inertial -> rotating)")
                << ": switch at tick " << r.switchTick << "; applied dv " << r.dvApplied.magnitude() << " m/s (KSP's omega x r_root; omega x (r_root - r_com) = "
                << r.quirk << " m/s); vs the inertial two-body orbit: <= " << r.errBefore << " m before, the switch tick displaces it by " << r.jump
                << " m (omega x r dt within " << r.jumpResidual << " m), afterwards a Kepler orbit within " << r.postDrift << " m for " << r.dt
                << " s (" << r.errEnd << " m from the initial orbit at the end)");
        REQUIRE(r.switchTick > 0);
        CHECK(r.events == 1);
        CHECK(r.eventState == !ascending);
        // the bubble applied exactly KSP's -/+ getRFrmVel(vessel.transform.position) to the bodies (float velocities)
        const Vector3d want = ascending ? r.rFrmVelRoot : -r.rFrmVelRoot;
        CHECK((r.dvApplied - want).magnitude() < 1e-4);
        CHECK(r.posJump < 50.0);  // one tick of motion (2.3 km/s): no position jump in the world frame
        CHECK(r.errBefore < 0.01);  // (ascending: the first ticks run before the drift compensation engages, orbitDriftFramesToWait)
        CHECK(r.jumpResidual < 1e-3);
        CHECK(r.postDrift < 0.01);
    }
}

// The SOI change of the bubble in physics (OrbitPhysicsManager.cs:155 setDominantBody, :322 checkReferenceFrame): the active vessel enters
// the Mun's sphere of influence; the orbit driver re-fits the orbit to the Mun, the dominant body becomes the Mun (the reversed orbit-driver
// chain keeps it still in the world), the bodies' velocities are resumed from the new orbit (orbit.GetVel() - FrameVel), positions are
// continuous. Reference: independent two-body propagations around Kerbin before and around the Mun after the crossing. The vessel is
// 9,600 km from Kerbin: the physics origin is re-centred on it (PHYSICS-SPIKE.md R3) on the way.
TEST_CASE("WP-19b SOI change in physics: the dominant body follows the active vessel into the Mun's SOI; reverse chain, velocity resume, continuity") {
    if (!wp20b::GameAvailable()) return;
    World w;
    BubbleScene s(w);
    auto v = w.Craft("PhysProf-S");
    CelestialBody& kerbin = w.Kerbin();
    CelestialBody& mun = *w.sys->GetBodyByName("Mun");
    const Vector3d pm = mun.position - kerbin.position;
    const Vector3d vm = (mun.GetFrameVel() - kerbin.GetFrameVel()).xzy();
    const Vector3d u = (pm.normalized() * -1.0 + Vector3d(0.0, 0.3, 0.0)).normalized();  // between Kerbin and the Mun, off the plane
    const Vector3d p0 = pm + u * (mun.sphereOfInfluence + 40.0);
    const Vector3d v0 = vm - u * 100.0;  // into the Mun's SOI at 100 m/s: crossing after about 0.4 s
    PlaceOnOrbit(*v, kerbin, p0, v0, true);
    const TwoBody refK{kerbin.gravParameter, -0.02, p0, v0};
    s.Start(*v);
    REQUIRE(s.opm->dominantBody == &kerbin);
    std::vector<std::string> events;
    const auto hev = decltype(GameEvents::onDominantBodyChange)::OnEvent::Lambda(&events, [&](GameEvents::FromToAction<CelestialBody*, CelestialBody*> d) {
        events.push_back(d.from->bodyName + ">" + d.to->bodyName);
    });
    GameEvents::onDominantBodyChange.Add(hev);
    const double dtf = static_cast<double>(Time::fixedDeltaTime);
    int soiTick = -1;
    double errKerbin = 0, errMun = 0, dvSoi = 0, posJump = 0;
    Vector3d lastRelMun, lastTrueVelMun, lastVMun;
    double jumpResidual = -1;
    TwoBody refM;
    double tM0 = 0;
    for (int k = 0; k < 150; ++k) {
        const bool before = s.opm->dominantBody == &kerbin;
        s.Tick();
        const double tk = k * dtf;
        // the true velocity of the vessel relative to the Mun: in the Kerbin-fixed world minus the Mun's velocity, then the world itself
        const Vector3d trueVelWorld = v->rb_velocityD + Krakensbane::GetFrameVelocity();
        const Vector3d trueVelMun = s.opm->dominantBody == &kerbin ? trueVelWorld - (mun.GetFrameVel() - kerbin.GetFrameVel()).xzy() : trueVelWorld;
        const Vector3d relMun = v->CoMD - mun.position;
        if (before && s.opm->dominantBody == &mun) {
            soiTick = k;
            dvSoi = (trueVelMun - lastTrueVelMun).magnitude();
            posJump = (relMun - lastRelMun).magnitude();
            // KSP's order: Planetarium.FixedUpdate (seq 6) still moved the Mun by its velocity this tick, then setDominantBody (seq 280)
            // resumed the vessel's velocity relative to the Mun before the step (seq 291): the vessel - Mun distance changes by
            // v_rel dt - V_Mun dt over this one tick (the Mun's motion of the tick is not followed), then the Mun is still
            jumpResidual = ((relMun - lastRelMun) - (trueVelMun * dtf - lastVMun * 0.02)).magnitude();
            // the Mun orbit the vessel continues on: the state at this tick, on the physics clock from here
            refM = TwoBody{mun.gravParameter, tk, relMun, trueVelMun};
            tM0 = tk;
        }
        if (k >= 1 && soiTick < 0 && v->mainBody() == &kerbin) errKerbin = std::max(errKerbin, ((v->CoMD - kerbin.position) - refK.At(tk)).magnitude());
        if (soiTick >= 0 && k > soiTick) errMun = std::max(errMun, (relMun - refM.At(tk)).magnitude());
        lastRelMun = relMun;
        lastTrueVelMun = trueVelMun;
        lastVMun = (mun.GetFrameVel() - kerbin.GetFrameVel()).xzy();
    }
    GameEvents::onDominantBodyChange.Remove(hev);
    MESSAGE("SOI change at tick " << soiTick << ": true velocity relative to the Mun changes by " << dvSoi << " m/s, position by " << posJump
                                  << " m over the tick (KSP's order: v_rel dt - V_Mun dt within " << jumpResidual << " m); vs Kerbin two-body before <= " << errKerbin << " m, vs Mun two-body after <= " << errMun
                                  << " m; physics-origin rebases " << s.bubble.PhysicsRebaseCount() << ", world rebases "
                                  << s.bubble.FloatingOriginInstance().ShiftCount() << ", tM0 " << tM0);
    REQUIRE(soiTick > 0);
    CHECK(v->mainBody() == &mun);
    CHECK(events == std::vector<std::string>{"Mun>Mun"});  // onDominantBodyChange(dominantBody, body): both the new body (C# order)
    // the reversed chain: the Mun and its parent Kerbin; the Mun is the root of Planetarium's update now (it stays still in the world)
    CHECK(mun.orbitDriver->reverse);
    CHECK(kerbin.orbitDriver->reverse);
    for (CelestialBody* b : w.sys->Bodies())
        if (b != &mun && b != &kerbin && b->orbitDriver) CHECK_FALSE(b->orbitDriver->reverse);
    const Vector3d munBefore = mun.position;
    w.planetarium.FixedUpdate();
    CHECK(w.planetarium.CurrentMainBody == &mun);
    CHECK(SameBits(mun.position, munBefore));
    CHECK(s.bubble.PhysicsRebaseCount() >= 1);  // 9,600 km from Kerbin's centre: the physics origin moved to the vessel
    CHECK(dvSoi < 0.05);                         // gravity over one tick (Kerbin and the Mun: ~1e-3 m/s) and the refit
    CHECK(jumpResidual >= 0.0);
    CHECK(jumpResidual < 0.01);
    CHECK(errKerbin < 1e-3);
    CHECK(errMun < 1e-3);
}

namespace {

struct FoldRun {
    double trueYDrift = 0;    // max |true y - 5| while the frame drifts (the object is at rest in the true frame)
    double physMinY = 1e9;    // lowest physics-frame y during the drift (it passes the floor's physics position)
    bool staticOffWhileDrifting = true;
    size_t folds = 0;
    double restY = 0, restSpeed = 0;
};

// A free body at rest 5 m above a static floor; Krakensbane's frame velocity (50 m/s up) makes it move DOWN in the physics frame while it
// stays put in the true frame; then the frame velocity goes back to zero (Krakensbane.Zero) and gravity pulls it onto the floor.
FoldRun RunFold(bool fold) {
    FoldRun r;
    World w;
    PhysicsBubbleOptions opt;
    opt.foldFrameOffset = fold;
    BubbleScene s(w, opt);
    FlightGlobals::ready = true;  // no active vessel: Krakensbane.FixedUpdate returns, the host drives the frame velocity below
    PhysicsBubble& b = s.bubble;
    const Vector3d floorWorld = b.FrameOriginWorld() + Vector3d(0.0, -0.5, 0.0);
    BodyDesc floor;
    floor.motion = MotionType::Static;
    floor.shape = ShapeDesc::Box(Vector3f(50.0f, 0.5f, 50.0f));
    floor.position = b.StaticPhysicsFromWorld(floorWorld);  // static geometry is in the body frame (no FrameOffset)
    s.world.CreateBody(floor);
    BodyDesc box;
    box.shape = ShapeDesc::Box(Vector3f(0.5f, 0.5f, 0.5f));
    box.mass = 1.0f;
    physicalObject& o = b.CreatePhysicalObject(box, b.FrameOriginWorld() + Vector3d(0.0, 5.0, 0.0), Vector3d());
    Krakensbane& kb = b.KrakensbaneInstance();
    kb.AddExcess(Vector3d(0.0, 50.0, 0.0));  // Krakensbane.cs:121: the physical object gets -50 m/s, FrameVel +50 m/s
    for (int k = 0; k < 25; ++k) {
        FloatingOrigin::SetOutOfFrameOffset(kb.FrameVel * static_cast<double>(Time::fixedDeltaTime));  // Krakensbane.cs:83
        b.FrameUpdate();
        r.staticOffWhileDrifting = r.staticOffWhileDrifting && !s.world.StaticCollisions();
        b.PhysicsStep();
        r.trueYDrift = std::max(r.trueYDrift, std::fabs((b.PhysicalObjectPosition(o) - b.FrameOriginWorld()).y - 5.0));
        r.physMinY = std::min(r.physMinY, s.world.GetPosition(o.rb).y);
    }
    kb.Zero();  // Krakensbane.cs:150: the frame velocity goes back into the object
    for (int k = 0; k < 250; ++k) {
        b.FrameUpdate();  // the first one folds the drift into the dynamic bodies (with the option)
        s.world.AddForce(o.rb, Vector3d(0.0, -9.81, 0.0));
        b.PhysicsStep();
    }
    r.folds = b.FoldCount();
    r.restY = (b.PhysicalObjectPosition(o) - b.FrameOriginWorld()).y;
    r.restSpeed = s.world.GetLinearVelocity(o.rb).magnitude();
    return r;
}

}  // namespace

// The static geometry (terrain quads, KSC) lives in the body frame, the dynamic bodies in Krakensbane's Galilean frame (FrameOffset):
// while the two differ no contact between them is real (KSP's SafeToEngage keeps every loaded vessel >= 150-200 m above the terrain while
// Krakensbane is engaged), so the bubble gates them; when the frame velocity is zero again the drift is folded into the dynamic bodies.
TEST_CASE("WP-19b Krakensbane frame drift and static geometry: no false contact while FrameOffset != 0, folded back at disengagement, then the floor holds") {
    if (!wp20b::GameAvailable()) return;
    const FoldRun with = RunFold(true);
    const FoldRun without = RunFold(false);
    MESSAGE("with the gate: true y kept within " << with.trueYDrift << " m while the physics y went down to " << with.physMinY << " m; folds "
                                                 << with.folds << "; rests at y " << with.restY << " m (" << with.restSpeed
                                                 << " m/s). Without: true y disturbed by " << without.trueYDrift << " m");
    CHECK(with.staticOffWhileDrifting);
    CHECK(with.physMinY < -5.0);        // the body passed the floor's physics position ...
    CHECK(with.trueYDrift < 1e-3);      // ... without touching it: it stayed at rest in the true frame
    CHECK(with.folds == 1);
    CHECK(std::fabs(with.restY - 0.5) < 0.05);  // on the floor where the floor really is
    CHECK(with.restSpeed < 0.05);
    CHECK(without.trueYDrift > 1.0);    // without the gate the stale floor hits it
}

// Re-centring the physics origin (PHYSICS-SPIKE.md R3) moves every Jolt body, static ones included, by the same double vector and the
// anchor the other way: no world position changes, the coast continues as without it.
TEST_CASE("WP-19b physics-origin rebase: every body moved, the world unchanged, the trajectory as without it") {
    if (!wp20b::GameAvailable()) return;
    auto run = [](bool rebase, double& staticWorldErr) {
        std::vector<Vector3d> rel;
        World w;
        PhysicsBubbleOptions opt;
        opt.physicsOriginRebaseDistance = 0.0;  // only the explicit rebase below
        BubbleScene s(w, opt);
        auto v = w.Craft("PhysProf-S");
        CelestialBody& kerbin = w.Kerbin();
        Vector3d p0, v0;
        CircularStateAt(kerbin, kerbin.Radius + 120000.0, Vector3d(0.6, 0.48, -0.64), Vector3d(0.3, -0.9, 0.2), p0, v0);
        PlaceOnOrbit(*v, kerbin, p0, v0, true);
        s.Start(*v);
        BodyDesc slab;  // a static body far below (a terrain quad's stand-in)
        slab.motion = MotionType::Static;
        slab.shape = ShapeDesc::Box(Vector3f(5.0f, 0.5f, 5.0f));
        const Vector3d slabRel = p0 * 0.9;
        slab.position = s.bubble.StaticPhysicsFromWorld(kerbin.position + slabRel);
        const BodyId slabId = s.world.CreateBody(slab);
        for (int k = 0; k < 300; ++k) {
            if (rebase && k == 150) s.bubble.RebasePhysicsOrigin(Vector3d(123456.5, -87654.25, 432101.0));
            s.Tick();
            rel.push_back(v->CoMD - kerbin.position);
        }
        staticWorldErr = ((s.bubble.FrameOriginWorld() + s.world.GetPosition(slabId)) - (kerbin.position + slabRel)).magnitude();
        CHECK(s.bubble.PhysicsRebaseCount() == (rebase ? 1u : 0u));
        return rel;
    };
    double e1 = 0, e2 = 0;
    const std::vector<Vector3d> a = run(false, e1), b = run(true, e2);
    double maxDiff = 0;
    for (size_t i = 0; i < a.size(); ++i) maxDiff = std::max(maxDiff, (a[i] - b[i]).magnitude());
    MESSAGE("trajectories with / without a physics-origin rebase differ by " << maxDiff << " m; the static body's world position after it: "
                                                                              << e2 << " m off");
    CHECK(maxDiff < 1e-5);
    CHECK(e1 < 1e-6);
    CHECK(e2 < 1e-6);
}

// A binding leaves the bubble with its vessel: Vessel.Unload (the parts are destroyed before GoOnRails), Die of a vessel that is not the
// active one (FlightGlobals.RemoveVessel), the Vessel object's destruction. Uninstall restores every seam.
TEST_CASE("WP-19b lifecycle: bindings dropped on unload, death and destruction; Uninstall restores the seams") {
    if (!wp20b::GameAvailable()) return;
    World w;
    BubbleScene s(w);
    auto a = w.Craft("PhysProf-S");
    auto b = w.CraftHead("PhysProf-S", 3);
    auto c = w.CraftHead("PhysProf-S", 3);
    auto d = w.CraftHead("PhysProf-S", 3);
    CelestialBody& kerbin = w.Kerbin();
    Vector3d p0, v0;
    CircularStateAt(kerbin, kerbin.Radius + 120000.0, Vector3d(0.6, 0.48, -0.64), Vector3d(0.3, -0.9, 0.2), p0, v0);
    const Vector3d along = v0.normalized();
    PlaceOnOrbit(*a, kerbin, p0, v0, true);
    PlaceOnOrbit(*b, kerbin, p0 + along * 60.0, v0, true);
    PlaceOnOrbit(*c, kerbin, p0 + along * 120.0, v0, true);
    PlaceOnOrbit(*d, kerbin, p0 + along * 180.0, v0, true);
    s.Start(*a);
    for (int k = 0; k < 3; ++k) s.Tick();  // OrbitPhysicsManager.LateUpdate unpacks all four (within 200 m)
    REQUIRE(s.bubble.Host().Count() == 4);
    const size_t bodies = s.world.BodyCount();
    b->Unload();
    CHECK(s.bubble.Host().Find(*b) == nullptr);
    CHECK(s.bubble.Host().Count() == 3);
    c->Die();
    CHECK(s.bubble.Host().Find(*c) == nullptr);
    d.reset();
    CHECK(s.bubble.Host().Count() == 1);
    CHECK(s.world.BodyCount() < bodies);
    for (int k = 0; k < 5; ++k) s.Tick();  // nothing left points at the freed parts; B (unloaded, 60 m away) loads and unpacks again
    CHECK(b->loaded);
    CHECK_FALSE(b->packed);
    CHECK(s.bubble.Host().Find(*b) != nullptr);
    CHECK(s.bubble.Host().Count() == 2);
    s.bubble.Uninstall();
    CHECK(Krakensbane::Instance() == nullptr);
    CHECK(FloatingOrigin::Instance() == nullptr);
    CHECK_FALSE(static_cast<bool>(Vessel::env.readPhysicsState));
    CHECK_FALSE(static_cast<bool>(Vessel::env.setWorldVelocity));
    CHECK_FALSE(static_cast<bool>(OrbitPhysicsManager::env.setRotatingFrame));
    CHECK_FALSE(static_cast<bool>(FlightScene::hooks.afterFlightIntegrator));
}

// physicalObject (physicalObject.cs): registered with FlightGlobals, its rigid body moved by Krakensbane (float VelocityChange), its
// velocity re-expressed at the frame switch, and the Update rule (high warp, maxDistance from the active vessel).
TEST_CASE("WP-19b physicalObject: registry, Krakensbane velocity change, frame switch, self-destruction rule") {
    if (!wp20b::GameAvailable()) return;
    World w;
    BubbleScene s(w);
    auto v = w.Craft("PhysProf-S");
    CelestialBody& kerbin = w.Kerbin();
    Vector3d p0, v0;
    CircularStateAt(kerbin, kerbin.Radius + 120000.0, Vector3d(0.6, 0.48, -0.64), Vector3d(0.3, -0.9, 0.2), p0, v0);
    PlaceOnOrbit(*v, kerbin, p0, v0, true);
    s.Start(*v);
    s.Tick();
    BodyDesc box;
    box.shape = ShapeDesc::Box(Vector3f(0.3f, 0.3f, 0.3f));
    box.mass = 0.05f;
    const Vector3d objWorld = Vector3d(v->vesselTransform.position) + Vector3d(0.0, 0.0, 30.0);
    const Vector3d trueVel = v0 + Vector3d(1.0, 2.0, -3.0);
    physicalObject& o = s.bubble.CreatePhysicalObject(box, objWorld, trueVel);
    REQUIRE(FlightGlobals::physicalObjects.size() == 1);
    CHECK(FlightGlobals::physicalObjects[0] == &o);
    CHECK((s.bubble.PhysicalObjectPosition(o) - objWorld).magnitude() < 1e-6);
    // true velocity = physics velocity + frame velocity (Krakensbane is engaged: the vessel orbits at 2.2 km/s)
    CHECK((s.world.GetLinearVelocity(o.rb) + Krakensbane::GetFrameVelocity() - trueVel).magnitude() < 1e-3);
    for (int k = 0; k < 20; ++k) s.Tick();
    // every Krakensbane re-centring moved the object with the vessels: its true velocity only changed by gravity (the bubble's
    // FlightIntegrator stand-in applies IntegratePhysicalObjects' gravity at the active vessel's CoM to it)
    const Vector3d tv = s.world.GetLinearVelocity(o.rb) + Krakensbane::GetFrameVelocity();
    CHECK((tv - trueVel).magnitude() < 21 * 0.02 * 7.5);
    // the frame switch moves it like the vessels (OrbitPhysicsManager.cs:286-300)
    const Vector3d before = s.world.GetLinearVelocity(o.rb);
    const Vector3d rFrm = kerbin.getRFrmVel(Vector3d(s.bubble.PhysicalObjectPosition(o).ToVector3f()));
    s.bubble.setRotatingFrame(*s.opm, true);
    CHECK((s.world.GetLinearVelocity(o.rb) - (before - rFrm)).magnitude() < 1e-3);
    s.bubble.setRotatingFrame(*s.opm, false);
    CHECK((s.world.GetLinearVelocity(o.rb) - before).magnitude() < 1e-3);
    // physicalObject.Update: 30 m away is fine, 1 km away it destroys itself; and at high warp
    s.bubble.Update();
    CHECK(s.bubble.PhysicalObjectCount() == 1);
    CHECK(o.UpdateWantsDestroy(Vector3f(0.0f, 0.0f, 1001.0f), true, Vector3f()));
    CHECK_FALSE(o.UpdateWantsDestroy(Vector3f(0.0f, 0.0f, 999.0f), true, Vector3f()));
    s.tw.curr_rate = 5.0f;  // HIGH mode above 1x
    CHECK(o.UpdateWantsDestroy(Vector3f(), true, Vector3f()));
    s.tw.curr_rate = 1.0f;
    s.world.SetPosition(o.rb, s.world.GetPosition(o.rb) + Vector3d(0.0, 2000.0, 0.0));
    s.bubble.Update();
    CHECK(s.bubble.PhysicalObjectCount() == 0);
    CHECK(FlightGlobals::physicalObjects.empty());
}
