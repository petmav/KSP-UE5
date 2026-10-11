// WP-19a unit tests, part 3: VesselPrecalculate wiring on real vessels (gravity in the rotating / inertial frame), the physics_orbit_hold scenario
// (drift compensation keeps a coasting vessel on its Kepler orbit), the flight scene tick order, Vessel.Update / LateUpdate state.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "doctest/doctest.h"

#include "ksp/input/input_lock_manager.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/vessel/vessel_switching.hpp"
#include "ksp/physics/reference_frame.hpp"
#include "flight_fixture.hpp"

using namespace ksp;
using namespace flighttest;
using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

TEST_CASE("WP-19a VesselPrecalculate.CalculateGravity wiring: rotating frame (Coriolis, centrifugal, GetRotFrameVel*) and inertial frame equal PrecalcMath over the same queries") {
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    Vessel* v = w.MakeVessel("v", kerbin, kerbin->Radius + 12000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    FlightGlobals::ActiveVessel = v;
    GameSettings::PHYSICS_EASE = false;
    v->orbitDriver->lastMode = OrbitDriver::UpdateMode::TRACK_Phys;
    for (int frame = 0; frame < 2; frame++) {
        kerbin->inverseRotation = frame == 0;
        v->CoMD = kerbin->position + Vector3d(kerbin->Radius + 12000.0, 0.0, 0.0);
        v->velocityD = Vector3d(0.0, 120.0, 230.0);
        v->precalc.physStatsNotDoneInUpdate = false;
        v->gravityMultiplier = 0.5;
        w.tw.curr_rate = 1.0f;
        v->precalc.FixedUpdate();
        const double fdt = static_cast<double>(TimeWarp::fixedDeltaTime());
        // expected, term by term
        const Vector3d gTrue = FlightGlobals::getGeeForceAtPosition(v->CoMD, *kerbin);
        PrecalcMath::GravityInputs in;
        in.gAccelTrue = gTrue;
        in.inverseRotation = kerbin->inverseRotation;
        in.coriolis = FlightGlobals::getCoriolisAcc(v->velocityD, *kerbin);
        in.centrifugal = FlightGlobals::getCentrifugalAcc(v->CoMD, *kerbin);
        in.GraviticForceMultiplier = PhysicsGlobals::GraviticForceMultiplier;
        in.gravityMultiplier = 0.5;
        in.CoMD = v->CoMD;
        in.velocityD = v->velocityD;
        in.mainBodyPosition = kerbin->position;
        in.fDeltaTime = fdt;
        Vector3d rfv = v->orbit().GetRotFrameVel(*kerbin);
        rfv.Swizzle();
        in.rotFrameVel = rfv;
        const PrecalcMath::GravityResult want =
            PrecalcMath::IntegrationGravity(in, [&](const Vector3d& p) { return v->orbit().GetRotFrameVelAtPos(*kerbin, p); });
        CHECK(SameBits(v->gravityTrue, gTrue));
        CHECK(SameBits(v->precalc.gAccelTrue, gTrue));
        CHECK(SameBits(v->precalc.integrationAccel, want.integrationAccel));
        CHECK(SameBits(v->precalc.gAccel, want.gAccel));
        CHECK(SameBits(v->gravityForPos, want.gravityForPos));
        CHECK(SameBits(v->graviticAcceleration, want.gAccel));
        CHECK_FALSE(v->precalc.wasCorrectingDrift);  // the vessel is inside the atmosphere
        if (frame == 0) {
            CHECK(SameBits(v->precalc.coriolis, in.coriolis));
            CHECK(SameBits(v->precalc.centrifugal, in.centrifugal));
            CHECK(v->precalc.coriolis.magnitude() > 0.0);
        } else {
            // inertial: the acceleration is just gAccelTrue * (G * gravityMultiplier)
            CHECK(SameBits(v->precalc.integrationAccel, gTrue * (PhysicsGlobals::GraviticForceMultiplier * 0.5)));
        }
        CHECK(v->precalc.ranFixedThisFrame == false);
    }
    kerbin->inverseRotation = false;
}

namespace {
struct HoldResult {
    double maxErr = 0.0, finalErr = 0.0, maxRadiusErr = 0.0;
    int corrections = 0, ticks = 0;
    double seconds = 0.0;
};

// A coasting vessel in a circular Kerbin orbit, simulated as KSP does per physics frame (a single rigid body, semi-implicit Euler like PhysX):
// Planetarium.FixedUpdate, VesselPrecalculate.FixedUpdate, FlightIntegrator (OffsetVelocity(pre); acceleration = integrationAccel), the physics
// step, VesselPrecalculate.Update (post-integration correction, CalculatePhysicsStats from the engine state, TrackRigidbody refit, UpdatePosVel,
// UpdateAcceleration). The error is measured against the original Kepler orbit (an Orbit built from the state at t0).
HoldResult RunOrbitHold(FlightEnv& w, Part& part, bool compensation, double orbits, double altitude, bool checkWiring) {
    CelestialBody* kerbin = w.Body("Kerbin");
    for (CelestialBody* b : w.sys->Bodies())
        if (b->orbitDriver) b->orbitDriver->reverse = false;
    kerbin->orbitDriver->reverse = true;
    GameSettings::PHYSICS_EASE = false;
    GameSettings::ORBIT_DRIFT_COMPENSATION = compensation;
    Vessel::env.frameCount = 1000;
    const double r = kerbin->Radius + altitude;
    const double ut0 = Planetarium::GetUniversalTime();
    Orbit ref(0.0, 0.0, r, 0.0, 0.0, 0.0, ut0, kerbin);
    Vector3d p0, v0;
    ref.GetOrbitalStateVectorsAtUT(ut0, p0, v0);
    Vector3d x = p0.xzy(), vel = v0.xzy();  // relative to the body, in the (inertial) physics frame
    Vessel* v = w.MakeVessel("hold", kerbin, r, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    v->rootPart = &part;
    v->parts = {&part};
    part.vessel = v;
    FlightGlobals::ActiveVessel = v;
    FlightGlobals::ready = true;
    Vessel::env.readPhysicsState = [&](Vessel&) {
        VesselPhysicsState s;
        s.totalMass = 1.0;
        s.CoMD = kerbin->position + x;
        s.rb_velocityD = vel;
        return std::optional<VesselPhysicsState>(s);
    };
    Vessel::env.offsetVelocity = [&](Vessel&, const Vector3d& dv) { vel = vel + dv; };
    v->staticPressurekPa = 0.0;
    const double dt = static_cast<double>(TimeWarp::fixedDeltaTime());
    const int ticks = static_cast<int>(orbits * ref.period / dt);
    HoldResult res;
    const auto t0 = std::chrono::steady_clock::now();
    for (int k = 1; k <= ticks; k++) {
        w.planetarium.time += dt;  // Planetarium.FixedUpdate
        v->precalc.FixedUpdate();  // MainPhysics
        if (checkWiring && (k == 7 || k == 500) && compensation && v->precalc.wasCorrectingDrift) {
            // the drift compensation of this tick, recomputed from the same queries
            const double uT = v->orbitDriver->lastTrackUT + dt;
            Vector3d rpn, rvn;
            v->orbit().GetOrbitalStateVectorsAtUT(uT, rpn, rvn);
            Vector3d rfn = v->orbit().GetRotFrameVelAtPos(*kerbin, rpn);
            rpn.Swizzle();
            rvn.Swizzle();
            rfn.Swizzle();
            Vector3d cpos, cvel;
            v->orbit().GetOrbitalStateVectorsAtUT(v->orbitDriver->lastTrackUT, cpos, cvel);
            Vector3d crf = v->orbit().GetRotFrameVelAtPos(*kerbin, cpos);
            crf.Swizzle();
            PrecalcMath::DriftInputs in;
            in.railsPosNext = rpn;
            in.railsVelNext = rvn;
            in.rotFrameNext = rfn;
            in.curRotFrameVel = crf;
            in.referenceBodyPosition = kerbin->position;
            in.CoMD = kerbin->position + x;
            in.velocityD = vel;
            in.fDeltaTime = dt;
            in.fDeltaTimeRecip = 1.0 / dt;
            in.GraviticForceMultiplier = PhysicsGlobals::GraviticForceMultiplier;
            in.gravityMultiplier = 1.0;
            const PrecalcMath::DriftResult want = PrecalcMath::ApplyDriftCompensation(in);
            CHECK(SameBits(v->precalc.preIntegrationVelocityOffset, want.preIntegrationVelocityOffset));
            CHECK(SameBits(v->precalc.postIntegrationVelocityCorrection, want.postIntegrationVelocityCorrection));
            CHECK(SameBits(v->precalc.gAccel, want.gAccel));
            CHECK(SameBits(v->gravityForPos, want.gravityForPos));
            CHECK(v->precalc.integrationAccel.IsZero());  // the compensation replaces gravity
        }
        if (v->precalc.wasCorrectingDrift) res.corrections++;
        if (!v->precalc.preIntegrationVelocityOffset.IsZero()) v->OffsetVelocity(v->precalc.preIntegrationVelocityOffset);  // FlightIntegrator.cs:665
        vel = vel + v->precalc.integrationAccel * dt;  // AddForce(integrationAccel, ForceMode.Acceleration) and the PhysX step
        x = x + vel * dt;
        v->vesselTransform.position = (kerbin->position + x).ToVector3f();
        v->precalc.Update();  // VesselPrecalculate.Update
        const double t = Planetarium::GetUniversalTime();
        Vector3d rp, rv;
        ref.GetOrbitalStateVectorsAtUT(t, rp, rv);
        const double err = (x - rp.xzy()).magnitude();
        res.maxErr = std::max(res.maxErr, err);
        res.maxRadiusErr = std::max(res.maxRadiusErr, std::abs(x.magnitude() - r));
        res.finalErr = err;
    }
    res.ticks = ticks;
    res.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    v->rootPart = nullptr;
    v->parts.clear();
    part.vessel = nullptr;
    Vessel::env.readPhysicsState = nullptr;
    Vessel::env.offsetVelocity = nullptr;
    return res;
}
}  // namespace

TEST_CASE("WP-19a physics_orbit_hold: a coasting vessel with drift compensation stays on its Kepler orbit (80 km, dt 0.02); without it the Euler drift shows") {
    auto part = std::make_unique<Part>();
    part->hasRigidbody = true;
    HoldResult off, on;
    {
        FlightEnv w;
        off = RunOrbitHold(w, *part, false, 1.0, 80000.0, false);
    }
    {
        FlightEnv w;
    #ifdef NDEBUG
        on = RunOrbitHold(w, *part, true, 10.0, 80000.0, true);  // 10 orbits: 937,405 ticks, about 6 s optimised
#else
        on = RunOrbitHold(w, *part, true, 2.0, 80000.0, true);   // the Debug build is ten times slower
#endif
    }
    MESSAGE("1 orbit without / 10 orbits with compensation: " << off.ticks << " / " << on.ticks << " ticks; without compensation max error " << off.maxErr << " m (radius " << off.maxRadiusErr << " m), with "
                        << on.maxErr << " m (radius " << on.maxRadiusErr << " m, " << on.corrections << " corrections); " << off.seconds << " s / "
                        << on.seconds << " s");
    CHECK(off.corrections == 0);
    CHECK(on.corrections >= on.ticks - 1);  // every tick but the first (the first MainPhysics runs the packed-style stats branch)
    // semi-implicit Euler at dt 0.02 without compensation: the O(dt) drift of PHYSICS-SPIKE.md (23-24 m radius error over one 80 km orbit)
    CHECK(off.maxRadiusErr > 15.0);
    CHECK(off.maxRadiusErr < 40.0);
    // with the compensation the vessel is held on the Kepler orbit
    CHECK(on.maxErr < 0.05);  // measured: 6.4 mm over 10 orbits (3 mm over one)
    CHECK(on.maxRadiusErr < 0.05);
    CHECK(on.maxErr * 20.0 < off.maxErr);
    CHECK(on.ticks > 1 * off.ticks);
}

TEST_CASE("WP-19a FlightScene tick order: Precalc -> OrbitDriver -> FlightGlobals -> OrbitPhysicsManager -> Vessel (the order measured in game), rails vessels advance, hold counter") {
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    Vessel* pad = w.MakeVessel("pad", kerbin, kerbin->Radius + 70.0, 0.0, true, false, OrbitDriver::UpdateMode::IDLE);
    pad->Landed = true;
    pad->situation = Vessel::Situations::LANDED;
    Vessel* probe = w.MakeVessel("probe", kerbin, 800000.0, 0.1, false, true);
    pad->packed = true;  // a landed vessel with no parts has no ground contact: checkLanded would reset Landed on an unpacked one
    FlightGlobals::ActiveVessel = pad;
    FlightGlobals::ready = true;
    OrbitPhysicsManager opm;
    opm.Start();
    OrbitPhysicsManager::HoldVesselUnpack(2);
    // observe the order through the hooks and the events each script produces
    std::vector<std::string> order;
    FlightScene::hooks.afterPrecalc = [&] { order.push_back("afterPrecalc"); };
    FlightScene::hooks.beforeFlightIntegrator = [&] { order.push_back("beforeFI"); };
    FlightScene::hooks.afterFlightIntegrator = [&] { order.push_back("afterFI"); };
    Orbit ref(0.0, 0.1, 800000.0, 0.0, 0.0, 0.0, Planetarium::GetUniversalTime(), kerbin);
    probe->IgnoreSpeed(2);
    CHECK(probe->IgnoreSpeedActive());
    const double ut0 = Planetarium::GetUniversalTime();
    ref.UpdateFromUT(ut0);  // (the Kepler solver iterates from the previous state: give both orbits the same history, as OrbitDriver.Start gave the probe)
    double prevX = probe->orbit().pos.x;
    for (int i = 1; i <= 5; i++) {
        w.planetarium.time = ut0 + 0.02 * i;
        order.clear();
        FlightScene::FixedUpdate();
        CHECK(order == std::vector<std::string>{"afterPrecalc", "beforeFI", "afterFI"});
        ref.UpdateFromUT(Planetarium::GetUniversalTime());
        INFO("tick " << i << " ut " << Planetarium::GetUniversalTime());
        // VesselPrecalculate.FixedUpdate moved the on-rails probe to the orbit state at this UT (the Kepler solver iterates from its previous
        // eccentric anomaly, so two Orbit objects with different call histories agree to its tolerance, 4 m here, not bit for bit)
        CHECK(std::abs(probe->orbit().pos.x - ref.pos.x) < 10.0);
        CHECK(probe->orbit().pos.x != prevX);
        prevX = probe->orbit().pos.x;
        CHECK(probe->precalc.ranFixedThisFrame);
        CHECK(FlightGlobals::ready);
    }
    CHECK_FALSE(probe->IgnoreSpeedActive());           // Vessel.FixedUpdate counted the two frames down
    CHECK(pad->state != Vessel::State::DEAD);
    CHECK_FALSE(opm.holdVesselUnpack);                  // OrbitPhysicsManager.FixedUpdate: 2, 1, 0, then released on the 4th
    // Update: VesselPrecalculate.Update does nothing for a packed vessel; Vessel.Update advances the mission clock
    pad->launchTime = 0.0;
    FlightScene::Update();
    CHECK(pad->missionTime == Planetarium::GetUniversalTime() - 0.0);
    // LateUpdate: OrbitPhysicsManager.LateUpdate (a far probe stays packed) and Vessel.LateUpdate (the situation of the probe becomes ORBITING)
    w.tw.curr_rate = 1.0f;
    FlightScene::LateUpdate();
    CHECK(probe->packed);
    CHECK(probe->situation == Vessel::Situations::ORBITING);
    CHECK(pad->situation == Vessel::Situations::LANDED);
}

TEST_CASE("WP-19a Vessel.Update / LateUpdate state: PRELAUNCH keeps resetting the launch time; the one-off bounced-off check; part g / pressure thresholds reset") {
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    Vessel* v = w.MakeVessel("v", kerbin, 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    Vessel* other = w.MakeVessel("o", kerbin, 710000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    FlightGlobals::ActiveVessel = other;
    FlightGlobals::ready = true;
    v->situation = Vessel::Situations::PRELAUNCH;
    v->launchTime = 5.0;
    v->Update();
    CHECK(v->launchTime == Planetarium::GetUniversalTime());  // PRELAUNCH: the mission has not started
    CHECK(v->missionTime == 0.0);
    v->situation = Vessel::Situations::ORBITING;
    w.planetarium.time += 12.5;
    v->Update();
    CHECK(v->missionTime == 12.5);
    v->state = Vessel::State::DEAD;  // a dead vessel does nothing
    const double mt = v->missionTime;
    w.planetarium.time += 1.0;
    v->Update();
    CHECK(v->missionTime == mt);
    v->state = Vessel::State::ACTIVE;
    // LateUpdate: partMax thresholds are cleared every frame; below 0.5 s since the level load the situation is not updated
    v->partMaxGThresh = 0.9;
    v->partMaxPresThresh = 0.9;
    Vessel::env.timeSinceLevelLoad = 0.2f;
    v->situation = Vessel::Situations::PRELAUNCH;
    v->LateUpdate();
    CHECK(v->partMaxGThresh == 0.0);
    CHECK(v->partMaxPresThresh == 0.0);
    CHECK(v->situation == Vessel::Situations::PRELAUNCH);  // not touched before 0.5 s
    Vessel::env.timeSinceLevelLoad = 10.0f;
    v->LateUpdate();                                        // > 5 s: UpdateLandedSplashed once, then updateSituation
    CHECK(v->situation == Vessel::Situations::ORBITING);
    // not ready: nothing happens
    FlightGlobals::ready = false;
    v->partMaxGThresh = 0.9;
    v->LateUpdate();
    CHECK(v->partMaxGThresh == 0.9);
}

TEST_CASE("WP-19a rails <-> physics: GoOnRails applies a pending drift correction and refits the orbit, state vectors are preserved; save/load of a vessel on rails") {
    auto part = std::make_unique<Part>();
    part->hasRigidbody = true;
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    GameSettings::PHYSICS_EASE = false;
    Vessel::env.frameCount = 1000;
    const double r = kerbin->Radius + 80000.0;
    Orbit ref(0.0, 0.0, r, 0.0, 0.0, 0.0, Planetarium::GetUniversalTime(), kerbin);
    Vector3d p0, v0;
    ref.GetOrbitalStateVectorsAtUT(Planetarium::GetUniversalTime(), p0, v0);
    Vector3d x = p0.xzy(), vel = v0.xzy();
    Vessel* v = w.MakeVessel("v", kerbin, r, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
    v->rootPart = part.get();
    v->parts = {part.get()};
    part->vessel = v;
    FlightGlobals::ActiveVessel = v;
    FlightGlobals::ready = true;
    Vessel::env.readPhysicsState = [&](Vessel&) {
        VesselPhysicsState s;
        s.totalMass = 1.0;
        s.CoMD = kerbin->position + x;
        s.rb_velocityD = vel;
        return std::optional<VesselPhysicsState>(s);
    };
    Vessel::env.offsetVelocity = [&](Vessel&, const Vector3d& dv) { vel = vel + dv; };
    v->vesselTransform.position = (kerbin->position + x).ToVector3f();
    v->staticPressurekPa = 0.0;
    const double dt = static_cast<double>(TimeWarp::fixedDeltaTime());
    for (int k = 1; k <= 200; k++) {
        w.planetarium.time += dt;
        v->precalc.FixedUpdate();
        if (!v->precalc.preIntegrationVelocityOffset.IsZero()) v->OffsetVelocity(v->precalc.preIntegrationVelocityOffset);
        vel = vel + v->precalc.integrationAccel * dt;
        x = x + vel * dt;
        v->vesselTransform.position = (kerbin->position + x).ToVector3f();
        if (k < 200) v->precalc.Update();
    }
    // a post-integration correction is pending (computed this tick, applied by Update): GoOnRails applies it first
    CHECK_FALSE(v->precalc.postIntegrationVelocityCorrection.IsZero());
    const Vector3d velBefore = vel;
    const Vector3d post = v->precalc.postIntegrationVelocityCorrection;
    std::vector<std::string> events;
    const auto hR = decltype(GE::onVesselGoOnRails)::OnEvent::Lambda(&events, [&](Vessel*) { events.push_back("rails"); });
    GE::onVesselGoOnRails.Add(hR);
    v->GoOnRails();
    GE::onVesselGoOnRails.Remove(hR);
    CHECK(events == std::vector<std::string>{"rails"});
    CHECK(SameBits(vel, velBefore + post));
    CHECK(v->packed);
    CHECK(v->orbitDriver->updateMode == OrbitDriver::UpdateMode::UPDATE);
    CHECK(v->precalc.postIntegrationVelocityCorrection.IsZero());
    CHECK_FALSE(v->precalc.wasCorrectingDrift);
    // the orbit passes through the physics state (TrackRigidbody refit at lastTrackUT): position within the tolerance of the refit
    Vector3d rp, rv;
    v->orbit().GetOrbitalStateVectorsAtUT(v->orbitDriver->lastTrackUT, rp, rv);
    CHECK((rp.xzy() - x).magnitude() < 1e-6);
    CHECK((rv.xzy() - vel).magnitude() < 1e-6);
    // and on rails the vessel follows that orbit exactly (the packed vessel's rigid bodies are kinematic: no engine state is read)
    Vessel::env.readPhysicsState = nullptr;
    w.planetarium.time += 100.0;
    v->precalc.FixedUpdate();
    Vector3d rp2, rv2;
    v->orbit().GetOrbitalStateVectorsAtUT(Planetarium::GetUniversalTime(), rp2, rv2);
    CHECK(SameBits(v->CoMD, kerbin->position + v->orbitDriver->pos));
    CHECK(((v->CoMD - kerbin->position) - rp2.xzy()).magnitude() < 1e-6);
    v->rootPart = nullptr;
    v->parts.clear();
    part->vessel = nullptr;
}

TEST_CASE("WP-19a VesselSwitching.Update: the next / previous loaded vessel, uncontrollable objects skipped without the modifier key, packed vessels refused when not clear to save (600 scenes)") {
    std::map<std::string, int> seen;
    for (const json& sc : Ref().at("vessel_switching")) {
        FlightEnv w;
        CelestialBody* kerbin = w.Body("Kerbin");
        std::vector<Vessel*> vs;
        for (const json& jv : sc["vessels"]) {
            Vessel* v = w.MakeVessel("v" + std::to_string(vs.size()), kerbin, 700000.0 + 1000.0 * static_cast<double>(vs.size()), 0.0, jv["loaded"].get<bool>(),
                                     jv["packed"].get<bool>(), OrbitDriver::UpdateMode::TRACK_Phys);
            v->vesselType = static_cast<VesselType>(jv["type"].get<int>());
            v->situation = Vessel::Situations::ORBITING;
            vs.push_back(v);
        }
        Vessel* active = vs[static_cast<size_t>(sc["active"].get<int>())];
        FlightGlobals::ActiveVessel = active;
        active->state = Vessel::State::ACTIVE;
        // ClearToSave of the active vessel: CLEAR, or NOT_IN_ATMOSPHERE (FLYING, unknown height)
        if (sc["status"].get<int>() != 0) active->situation = Vessel::Situations::FLYING;
        active->heightFromTerrain = -1.0f;
        Game game;
        game.Parameters = std::make_shared<GameParameters>();
        game.Parameters->Flight.CanSwitchVesselsNear = sc["canSwitch"].get<bool>();
        SaveEnvironment::Current().currentGame = sc["hasGame"].get<bool>() ? &game : nullptr;
        std::vector<std::string> messages;
        FlightGlobals::env.postScreenMessage = [&](const std::string& m) { messages.push_back(m); };
        VesselSwitching::env = VesselSwitching::Environment{};
        VesselSwitching::env.mapIsEnabled = [&] { return sc["mapEnabled"].get<bool>(); };
        VesselSwitching::env.paused = [&] { return sc["paused"].get<bool>(); };
        VesselSwitching::env.focusPrevKeyDown = [&] { return sc["prev"].get<bool>(); };
        VesselSwitching::env.focusNextKeyDown = [&] { return sc["next"].get<bool>(); };
        VesselSwitching::env.modifierKeyHeld = [&] { return sc["modifier"].get<bool>(); };
        VesselSwitching::env.loadedSceneIsFlight = [&] { return sc["sceneFlight"].get<bool>(); };
        VesselSwitching::env.postScreenMessage = [&](const std::string& m) { messages.push_back(m); };
        VesselSwitching::env.resumeVesselCtrlState = [&](Vessel&) {};
        if (sc["locked"].get<bool>()) InputLockManager::SetControlLock(ControlTypes::VESSEL_SWITCHING, "wp19a");
        VesselSwitching switching;
        Vessel* result = switching.Update();
        InputLockManager::RemoveControlLock("wp19a");
        const json& want = sc["result"];
        const std::string kind = want[0].get<std::string>();
        INFO("scene " << sc.dump().substr(0, 500));
        if (kind == "none") {
            CHECK(result == nullptr);
            CHECK(messages.empty());
            CHECK(FlightGlobals::ActiveVessel == active);
        } else if (kind == "msg") {
            CHECK(result == nullptr);
            REQUIRE(messages.size() == 1);
            CHECK(messages[0] == Localizer::Format("#autoLOC_" + want[1].get<std::string>()));
            CHECK(FlightGlobals::ActiveVessel == active);
        } else if (kind == "force") {
            CHECK(result == vs[static_cast<size_t>(want[1].get<int>())]);
            CHECK(FlightGlobals::ActiveVessel == result);
            CHECK(messages.empty());
        } else {  // "set": SetActiveVessel(candidates[0]) is refused (the active vessel is not clear to save)
            CHECK(result == nullptr);
            CHECK(FlightGlobals::ActiveVessel == active);
            REQUIRE(messages.size() == 1);
            CHECK(messages[0] == Localizer::Format("#autoLOC_6002346"));
        }
        seen[kind]++;
        SaveEnvironment::Current().currentGame = nullptr;
        VesselSwitching::env = VesselSwitching::Environment{};
    }
    CHECK(seen["force"] > 200);
    CHECK(seen["set"] > 10);
    CHECK(seen["msg"] > 50);
    CHECK(seen["none"] > 100);
}

TEST_CASE("WP-19a VesselSwitching: a second instance destroys itself; FindCandidates wraps around the list") {
    FlightEnv w;
    CelestialBody* kerbin = w.Body("Kerbin");
    std::vector<Vessel*> vs;
    for (int i = 0; i < 4; i++) vs.push_back(w.MakeVessel("v" + std::to_string(i), kerbin, 700000.0 + 1000.0 * i, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys));
    FlightGlobals::ActiveVessel = vs[3];
    VesselSwitching a;
    {
        VesselSwitching b;
        CHECK_FALSE(b.Alive());
    }
    bool skipped = false;
    std::vector<Vessel*> next = a.FindCandidates(1, false, skipped);
    CHECK(next == std::vector<Vessel*>{vs[0], vs[1], vs[2]});  // wraps past the end
    std::vector<Vessel*> prev = a.FindCandidates(-1, false, skipped);
    CHECK(prev == std::vector<Vessel*>{vs[2], vs[1], vs[0]});
    vs[1]->vesselType = VesselType::Debris;
    next = a.FindCandidates(1, false, skipped);
    CHECK(skipped);
    CHECK(next == std::vector<Vessel*>{vs[0], vs[2]});
    next = a.FindCandidates(1, true, skipped);
    CHECK(next == std::vector<Vessel*>{vs[0], vs[1], vs[2]});
}
