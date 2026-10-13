// WP-19b test support: a flight scene with the physics bubble (ksp/physics/physics_bubble.hpp) over the WP-20b World (game data,
// Physics.cfg, the Kerbol system), the WP-19a registry and OrbitPhysicsManager. Shared by tests/test_physics_bubble.cpp and
// tests/test_physics_bubble_ref.cpp.
//
// Declaration order in a test (the process rule "fixtures declare vessels last"): World, then BubbleScene, then the vessels, so that the
// vessels go first (their FlightGlobals.RemoveVessel drops their bindings), then the bubble, then the world.
#pragma once

#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/core/unity_time.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/orbit_physics_manager.hpp"
#include "ksp/physics/physics_bubble.hpp"
#include "ksp/physics/reference_frame.hpp"
#include "ksp/vessel/flight_scene.hpp"
#include "vessel_physics_fixture.hpp"

namespace wp19b {

using namespace ksp;
using namespace ksp::physics;
using wp20b::World;

inline bool SameBits(double a, double b) { return std::memcmp(&a, &b, sizeof a) == 0; }
inline bool SameBits(float a, float b) { return std::memcmp(&a, &b, sizeof a) == 0; }
inline bool SameBits(const Vector3d& a, const Vector3d& b) { return SameBits(a.x, b.x) && SameBits(a.y, b.y) && SameBits(a.z, b.z); }
inline bool SameBits(const Vector3f& a, const Vector3f& b) { return SameBits(a.x, b.x) && SameBits(a.y, b.y) && SameBits(a.z, b.z); }

// The vessel-frame centre of mass of a vessel (rb.mass weights over the parts with a Rigidbody)
inline Vector3d VesselFrameCom(Vessel& v) {
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

// The flight scene of a test: TimeWarp, the Planetarium clock, the registry flags, an OrbitPhysicsManager and the bubble (installed).
// Statics it touches are reset by the destructor.
struct BubbleScene {
    World& w;
    TimeWarp tw;
    TimeWarpEnvironment twEnv;
    PhysicsWorld world;
    PhysicsBubble bubble;
    std::unique_ptr<OrbitPhysicsManager> opm;
    explicit BubbleScene(World& w_, const PhysicsBubbleOptions& options = {}) : w(w_), bubble(world, options) {
        w.pg.CalculateValues(w.sys->Bodies(), w.planetarium.Sun);
        for (CelestialBody* b : w.sys->Bodies()) b->inverseRotation = false;
        twEnv.LoadedSceneValue = GameScenes::FLIGHT;
        TimeWarp::environment = &twEnv;
        FlightGlobals::currentMainBody = &w.Kerbin();
        w.planetarium.fixedDeltaTime = 0.02;
        w.planetarium.timeScale = 1.0;
        w.planetarium.time = 1000.0;
        Time::fixedDeltaTime = 0.02f;
        Time::timeScale = 1.0f;
        TimeWarp::fetch = &tw;
        tw.warpRates = {1.0f, 5.0f, 10.0f, 50.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f};
        tw.physicsWarpRates = {1.0f, 2.0f, 3.0f, 4.0f};
        tw.maxPhysicsRate_index = 3;
        tw.Mode = TimeWarp::Modes::HIGH;
        tw.curr_rate = 1.0f;
        w.planetarium.UpdateCBs();
        GameSettings::PHYSICS_EASE = false;
        GameSettings::ORBIT_DRIFT_COMPENSATION = true;
        Vessel::env.frameCount = 1000;  // Vessel.PhysicsHoldExpired
        bubble.Install();
        bubble.SetFrameOriginWorld(w.Kerbin().position);  // the physics origin at the dominant body's centre (WP-20b's choice)
    }
    ~BubbleScene() {
        opm.reset();
        bubble.Uninstall();
        TimeWarp::fetch = nullptr;
        TimeWarp::environment = nullptr;
        Time::fixedDeltaTime = 0.02f;
        GameSettings::PHYSICS_EASE = true;
        GameSettings::ORBIT_DRIFT_COMPENSATION = true;
        FlightGlobals::ready = false;
        FlightGlobals::ActiveVessel = nullptr;
        OrbitPhysicsManager::env = OrbitPhysicsManager::Environment{};
        FlightScene::hooks = FlightScene::Hooks{};
    }
    // FlightDriver.Start (FlightDriver.cs:212-224): the vessels are on their orbits (PlaceOnOrbit), the world origin moves to the active
    // vessel (FloatingOrigin.SetOffset(vessel.transform.position)); FlightGlobals.ready; OrbitPhysicsManager.Start (dominantBody = the
    // current main body)
    void Start(Vessel& active, bool rebaseToActive = true) {
        if (rebaseToActive) FloatingOrigin::SetOffset(Vector3d(active.vesselTransform.position));
        FlightGlobals::ActiveVessel = &active;
        FlightGlobals::currentMainBody = active.mainBody();
        FlightGlobals::ready = true;
        Vessel::env.frameCount += 1000;  // every vessel made so far is past its physics hold (Vessel.PhysicsHoldExpired)
        opm = std::make_unique<OrbitPhysicsManager>();
        opm->Start();
    }
    // one frame: Planetarium.FixedUpdate (seq 6), the bubble's FixedUpdate (seq 7-291), Update, LateUpdate
    void Tick() {
        ++Vessel::env.frameCount;  // Time.frameCount
        w.planetarium.FixedUpdate();
        bubble.FixedUpdate();
        bubble.Update();
        bubble.LateUpdate();
    }
};

// A vessel on a Kepler orbit given by state vectors relative to `body` (Unity axes: xzy of the orbit frame), packed on rails or
// unpacked in TRACK_Phys, registered. The orbit is the exact Kepler orbit of the state at the current UT.
inline void PlaceOnOrbit(Vessel& v, CelestialBody& body, const Vector3d& relPosWorld, const Vector3d& velWorld, bool packed) {
    const double ut = Planetarium::GetUniversalTime();
    auto kepler = [&] {
        auto o = std::make_shared<Orbit>();
        o->UpdateFromStateVectors(relPosWorld.xzy(), velWorld.xzy(), &body, ut);
        return o;
    };
    v.lastBody = &body;
    v.Landed = false;
    v.Splashed = false;
    v.situation = Vessel::Situations::ORBITING;
    v.packed = packed;
    v.loaded = true;
    v.staticPressurekPa = 0.0;
    v.localCoM = VesselFrameCom(v).ToVector3f();
    v.CoMD = body.position + relPosWorld;
    // OrbitDriver.Start in UPDATE mode (orbit.Init, updateFromParameters: the vessel placed on the orbit, Planetarium.Orbits.Add), then
    // the requested mode with the exact Kepler orbit (TRACK_Phys would refit it to the not yet placed parts)
    v.orbitDriver->orbit = kepler();
    v.orbitDriver->updateMode = OrbitDriver::UpdateMode::UPDATE;
    v.orbitDriver->Start();
    v.orbitDriver->orbit = kepler();
    v.orbitDriver->updateMode = packed ? OrbitDriver::UpdateMode::UPDATE : OrbitDriver::UpdateMode::TRACK_Phys;
    // (Vessel.Initialize of the craft assembly registered it already: a second AddVessel would list it twice)
    if (std::find(FlightGlobals::Vessels.begin(), FlightGlobals::Vessels.end(), &v) == FlightGlobals::Vessels.end()) FlightGlobals::AddVessel(&v);
    FlightGlobals::SortVessel(&v, true);
    // WP-20d: Part.Start of the flight scene (FlightIntegrator.Setup waits for the root part's `started`, FlightIntegrator.cs:255)
    StartPartsInFlight(v);
}

// circular orbit state at radius r (in the orbit's reference plane: position on +x, velocity on +z of the Unity frame)
inline void CircularState(const CelestialBody& body, double r, Vector3d& pos, Vector3d& vel) {
    const double vc = std::sqrt(body.gravParameter / r);
    pos = Vector3d(r, 0.0, 0.0);
    vel = Vector3d(0.0, 0.0, vc);
}

// the same at a general position direction `dir` (inclined orbit, prograde about `normal`): no coordinate is zero
inline void CircularStateAt(const CelestialBody& body, double r, const Vector3d& dir, const Vector3d& normal, Vector3d& pos, Vector3d& vel) {
    const Vector3d u = dir.normalized();
    const Vector3d t = Vector3d::Cross(normal, u).normalized();
    pos = u * r;
    vel = t * std::sqrt(body.gravParameter / r);
}

// An independent two-body reference (no kspcore orbit code): Lagrange f / g coefficients with the eccentric-anomaly difference solved by
// Newton to machine precision (any elliptic orbit, circular included). Positions relative to the body, in the frame of r0 / v0.
struct TwoBody {
    double mu = 0.0, t0 = 0.0;
    Vector3d r0, v0;
    Vector3d At(double t) const {
        const double r0m = r0.magnitude();
        const double a = 1.0 / (2.0 / r0m - v0.sqrMagnitude() / mu);
        const double sqa = std::sqrt(a);
        const double n = std::sqrt(mu / (a * a * a));
        const double sigma0 = Vector3d::Dot(r0, v0) / std::sqrt(mu);
        const double M = n * (t - t0);
        double dE = M;
        for (int i = 0; i < 60; ++i) {
            const double f = dE + sigma0 / sqa * (1.0 - std::cos(dE)) - (1.0 - r0m / a) * std::sin(dE) - M;
            const double fp = 1.0 + sigma0 / sqa * std::sin(dE) - (1.0 - r0m / a) * std::cos(dE);
            const double step = f / fp;
            dE -= step;
            if (std::fabs(step) < 1e-16) break;
        }
        const double f = 1.0 - a / r0m * (1.0 - std::cos(dE));
        const double g = (t - t0) - std::sqrt(a * a * a / mu) * (dE - std::sin(dE));
        lastDE_ = dE;
        return r0 * f + v0 * g;
    }
    Vector3d VelAt(double t) const {
        At(t);
        const double dE = lastDE_;
        const double r0m = r0.magnitude();
        const double a = 1.0 / (2.0 / r0m - v0.sqrMagnitude() / mu);
        const double sigma0 = Vector3d::Dot(r0, v0) / std::sqrt(mu);
        const double r = a + (r0m - a) * std::cos(dE) + sigma0 * std::sqrt(a) * std::sin(dE);
        const double fdot = -std::sqrt(mu * a) / (r * r0m) * std::sin(dE);
        const double gdot = 1.0 - a / r * (1.0 - std::cos(dE));
        return r0 * fdot + v0 * gdot;
    }
    mutable double lastDE_ = 0.0;  // the eccentric-anomaly difference of the last At()
};

}  // namespace wp19b
