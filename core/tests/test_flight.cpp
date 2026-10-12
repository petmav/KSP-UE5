// WP-19a unit tests: flight-scene registry (FlightGlobals), on-rails propagation, VesselPrecalculate, OrbitPhysicsManager pack / unpack,
// Vessel load / unload by range, CheckKill, the situation state machine.
//
// Gate: tools/wp19a/flight_reference.py (independent of kspcore; tests/data/wp19a_reference.json) at 0 ULP for the gravity at the CoM, the
// Coriolis / centrifugal accelerations, the rotating-frame gravity integration, the orbit drift decision and compensation; exact decisions
// (and float32 distances under the Mono rule) for pack / unpack, load / unload, CheckKill and updateSituation. The in-game C8 capture of
// on-rails propagation through real Vessels is the golden_runner dataset `vessel_rails` (ctest capture.vessel_rails). Everything else here is
// checked against the C# line by line (the physics_orbit_hold scenario against the analytic Kepler orbit).
//
// No KSP game data are needed: the Kerbol system comes from the game's prefab data (KerbolSystemPrefab) and Physics.cfg from harness/inputs.
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "nlohmann/json.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/orbit_physics_manager.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/physics/reference_frame.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/flight_scene.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_precalculate.hpp"
#include "flight_fixture.hpp"

using namespace ksp;
using namespace flighttest;
using json = nlohmann::json;
using GE = events::GameEvents;


// ================================================================================================ pure formulas vs the Python reference

TEST_CASE("WP-19a getGeeForceAtPosition equals the reference (PrecalcMath and FlightGlobals), 0 ULP") {
    int n = 0;
    for (const json& c : Ref().at("gee")) {
        const Vector3d pos = V3(c["pos"]), bodyPos = V3(c["body"]), want = V3(c["out"]);
        const double g = D(c["g"]);
        CHECK(SameBits(PrecalcMath::GeeForceAtPosition(pos, bodyPos, g), want));
        CelestialBody body;
        body.position = bodyPos;
        body.gMagnitudeAtCenter = g;
        CHECK(SameBits(FlightGlobals::getGeeForceAtPosition(pos, body), want));
        n++;
    }
    CHECK(n == 300);
}

TEST_CASE("WP-19a getCentrifugalAcc / getCoriolisAcc equal the reference (FlightGlobals and ReferenceFrame), 0 ULP") {
    int n = 0;
    for (const json& c : Ref().at("fict")) {
        CelestialBody body;
        body.angularVelocity = V3(c["omega"]);
        body.position = V3(c["body"]);
        body.inverseRotation = c["inv"].get<bool>();
        const Vector3d pos = V3(c["pos"]), vel = V3(c["vel"]);
        CHECK(SameBits(FlightGlobals::getCentrifugalAcc(pos, body), V3(c["cen"])));
        CHECK(SameBits(FlightGlobals::getCoriolisAcc(vel, body), V3(c["cor"])));
        CHECK(SameBits(ReferenceFrame::getCentrifugalAcc(pos, body), V3(c["cen"])));
        CHECK(SameBits(ReferenceFrame::getCoriolisAcc(vel, body), V3(c["cor"])));
        n++;
    }
    CHECK(n == 300);
}

TEST_CASE("WP-19a orbit drift check (VesselPrecalculate.cs:278-290) equals the reference") {
    int counts[3] = {0, 0, 0};
    for (const json& c : Ref().at("drift_check")) {
        const PrecalcMath::DriftCheck r = PrecalcMath::CheckDrift(c["always"].get<bool>(), V3(c["d1"]), V3(c["d2"]), D(c["thr"]),
                                                                  c["wait"].get<int>(), c["frames"].get<int>());
        CHECK(static_cast<int>(r.action) == c["action"].get<int>());
        CHECK(r.framesUntilCorrect == c["newFrames"].get<int>());
        counts[c["action"].get<int>()]++;
    }
    CHECK(counts[0] > 100);
    CHECK(counts[1] > 50);
    CHECK(counts[2] > 50);
}

TEST_CASE("WP-19a orbit drift compensation (VesselPrecalculate.cs:293-314) equals the reference, 0 ULP, including the IsInvalid reset") {
    int invalid = 0;
    for (const json& c : Ref().at("drift_apply")) {
        PrecalcMath::DriftInputs in;
        in.railsPosNext = V3(c["rpn"]);
        in.railsVelNext = V3(c["rvn"]);
        in.rotFrameNext = V3(c["rfn"]);
        in.curRotFrameVel = V3(c["crf"]);
        in.referenceBodyPosition = V3(c["bodypos"]);
        in.CoMD = V3(c["comd"]);
        in.velocityD = V3(c["vel"]);
        in.fDeltaTime = D(c["fdt"]);
        in.fDeltaTimeRecip = D(c["rec"]);
        in.GraviticForceMultiplier = D(c["G"]);
        in.gravityMultiplier = D(c["gm"]);
        const PrecalcMath::DriftResult r = PrecalcMath::ApplyDriftCompensation(in);
        CHECK(SameBits(r.preIntegrationVelocityOffset, V3(c["pre"])));
        CHECK(SameBits(r.postIntegrationVelocityCorrection, V3(c["post"])));
        CHECK(SameBits(r.integrationAccel, V3(c["ia"])));
        CHECK(SameBits(r.gAccel, V3(c["g"])));
        CHECK(SameBits(r.gravityForPos, V3(c["gfp"])));
        CHECK(r.invalid == c["invalid"].get<bool>());
        invalid += r.invalid ? 1 : 0;
    }
    CHECK(invalid == 20);
}

TEST_CASE("WP-19a gravity integration in the rotating and the inertial frame (VesselPrecalculate.cs:317-343) equals the reference, 0 ULP") {
    int rotating = 0, inertial = 0;
    for (const json& c : Ref().at("gravity")) {
        PrecalcMath::GravityInputs in;
        in.gAccelTrue = V3(c["gtrue"]);
        in.inverseRotation = c["inv"].get<bool>();
        in.coriolis = V3(c["cor"]);
        in.centrifugal = V3(c["cen"]);
        in.GraviticForceMultiplier = D(c["G"]);
        in.gravityMultiplier = D(c["gm"]);
        in.CoMD = V3(c["comd"]);
        in.velocityD = V3(c["vel"]);
        in.mainBodyPosition = V3(c["bodypos"]);
        in.fDeltaTime = D(c["fdt"]);
        in.rotFrameVel = V3(c["rfv"]);
        const Vector3d w2 = V3(c["w2"]);
        const PrecalcMath::GravityResult r = PrecalcMath::IntegrationGravity(in, [&](const Vector3d& refPos) { return Vector3d::Cross(w2, refPos); });
        CHECK(SameBits(r.integrationAccel, V3(c["ia"])));
        CHECK(SameBits(r.gAccel, V3(c["g"])));
        CHECK(SameBits(r.gravityForPos, V3(c["gfp"])));
        (in.inverseRotation ? rotating : inertial)++;
    }
    CHECK(rotating > 250);
    CHECK(inertial > 100);
}

// ================================================================================================ decisions vs the reference

namespace {
// The vessel states the decision tables need: a loaded vessel with no parts at a float position, with a situation and (optionally) a
// FLYING history (Vessel.BestSituation reads lastSituation, which only updateSituation writes).
void SetSituation(Vessel& v, int situation, bool wasFlying) {
    v.Landed = false;
    v.Splashed = false;
    if (wasFlying) {
        v.staticPressurekPa = 1.0;
        v.updateSituation();  // situation = FLYING, lastSituation = FLYING
        v.staticPressurekPa = 0.0;
    }
    v.situation = static_cast<Vessel::Situations>(situation);
}
VesselRanges RangesOf(const json& j) {
    VesselRanges r;
    auto s = [&](const char* k) { return VesselRanges::Situation(Fl(j[k][0]), Fl(j[k][1]), Fl(j[k][2]), Fl(j[k][3])); };
    r.prelaunch = s("prelaunch");
    r.landed = s("landed");
    r.splashed = s("splashed");
    r.flying = s("flying");
    r.subOrbital = s("subOrbital");
    r.orbit = s("orbit");
    r.escaping = s("escaping");
    return r;
}
}  // namespace

TEST_CASE("WP-19a OrbitPhysicsManager.LateUpdate: pack / unpack by distance and time warp equals the reference (500 scenes)") {
    int onCount = 0, offCount = 0, scenes = 0;
    for (const json& sc : Ref().at("late_update")) {
        FlightEnv w;
        Vessel::env.frameCount = 0;
        std::vector<Vessel*> vs;
        Vessel* active = w.MakeVessel("active", w.Body("Kerbin"), 700000.0, 0.0, true, sc["active"]["packed"].get<bool>(), OrbitDriver::UpdateMode::TRACK_Phys);
        active->vesselTransform.position = V3f(sc["active"]["pos"]);
        for (const json& v : sc["vessels"]) {
            Vessel* x = v["isActive"].get<bool>() ? active : w.MakeVessel("v", w.Body("Kerbin"), 700000.0, 0.0, true, v["packed"].get<bool>(),
                                                                          OrbitDriver::UpdateMode::TRACK_Phys);
            if (!v["isActive"].get<bool>()) {
                x->vesselTransform.position = V3f(v["pos"]);
                x->vesselRanges = RangesOf(v["ranges"]);
                SetSituation(*x, v["situation"].get<int>(), v["wasFlying"].get<bool>());
                if (!v["hasDriver"].get<bool>()) {
                    // `vessel.orbitDriver == null`: take the driver out of the vessel (the registry keeps the vessel)
                    x->orbitDriver.reset();
                }
            }
            vs.push_back(x);
        }
        Vessel::env.frameCount = 1000;  // PhysicsHoldExpired for every vessel
        FlightGlobals::ActiveVessel = active;
        // the actions are what GoOnRails / GoOffRails report through the precalc hooks
        std::vector<std::pair<int, std::string>> acts;
        auto indexOf = [&](Vessel& v) { return &v == active ? -1 : static_cast<int>(std::find(vs.begin(), vs.end(), &v) - vs.begin()); };
        Vessel::env.precalcGoOnRails = [&](Vessel& v) { acts.emplace_back(indexOf(v), "on"); };
        Vessel::env.precalcGoOffRails = [&](Vessel& v) { acts.emplace_back(indexOf(v), "off"); };
        w.tw.curr_rate = Fl(sc["rate"]);
        w.tw.physicsWarpRates = {Fl(sc["maxPhys"])};
        w.tw.warpRates = {Fl(sc["maxPhys"])};
        w.tw.maxPhysicsRate_index = 0;
        w.tw.Mode = sc["warpHigh"].get<bool>() ? TimeWarp::Modes::HIGH : TimeWarp::Modes::LOW;
        OrbitPhysicsManager opm;
        FlightGlobals::ready = true;
        FlightGlobals::currentMainBody = w.Body("Kerbin");
        if (sc["started"].get<bool>()) opm.Start();
        FlightGlobals::ready = sc["ready"].get<bool>();
        FlightGlobals::overrideOrbit = sc["override"].get<bool>();
        FlightGlobals::warpDriveActive = sc["warpDrive"].get<bool>();
        opm.holdVesselUnpack = sc["hold"].get<bool>();
        opm.LateUpdate();
        // expected
        std::vector<std::pair<int, std::string>> want;
        for (const json& a : sc["actions"]) want.emplace_back(a[0].get<int>(), a[1].get<std::string>());
        CHECK(acts == want);
        CHECK(active->packed == sc["activePacked"].get<bool>());
        for (size_t i = 0; i < vs.size(); i++)
            if (vs[i] != active) CHECK(vs[i]->packed == sc["packed"][i].get<bool>());
        for (auto& a : acts) (a.second == "on" ? onCount : offCount)++;
        FlightGlobals::warpDriveActive = false;
        scenes++;
    }
    CHECK(scenes == 500);
    CHECK(onCount > 300);
    CHECK(offCount > 100);
}

TEST_CASE("WP-19a Vessel.Update: load / unload by VesselRanges equals the reference (500 scenes)") {
    int loads = 0, unloads = 0;
    for (const json& sc : Ref().at("vessel_update")) {
        FlightEnv w;
        Vessel* active = w.MakeVessel("active", w.Body("Kerbin"), 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
        active->vesselTransform.position = V3f(sc["apos"]);
        Vessel* v = w.MakeVessel("v", w.Body("Kerbin"), 700000.0, 0.0, sc["loaded"].get<bool>(), true, OrbitDriver::UpdateMode::UPDATE);
        v->vesselTransform.position = V3f(sc["pos"]);
        v->vesselRanges = RangesOf(sc["ranges"]);
        v->situation = static_cast<Vessel::Situations>(sc["situation"].get<int>());
        FlightGlobals::ActiveVessel = active;
        FlightGlobals::ready = true;
        // Load needs the proto vessel an unloaded vessel keeps; Unload creates it
        std::vector<std::string> acts;
        auto onLoad = [&](Vessel*) { acts.push_back("load"); };
        auto onUnload = [&](Vessel*) { acts.push_back("unload"); };
        const auto hLoad = decltype(GE::onVesselLoaded)::OnEvent::Lambda(&acts, onLoad);
        const auto hUnload = decltype(GE::onVesselUnloaded)::OnEvent::Lambda(&acts, onUnload);
        GE::onVesselLoaded.Add(hLoad);
        GE::onVesselUnloaded.Add(hUnload);
        v->Update();
        GE::onVesselLoaded.Remove(hLoad);
        GE::onVesselUnloaded.Remove(hUnload);
        std::vector<std::string> want;
        for (const json& a : sc["actions"]) want.push_back(a.get<std::string>());
        CHECK(acts == want);
        CHECK(v->loaded == sc["finalLoaded"].get<bool>());
        for (auto& a : acts) (a == "load" ? loads : unloads)++;
    }
    CHECK(loads > 30);
    CHECK(unloads > 80);
}

TEST_CASE("WP-19a Vessel.CheckKill equals the reference (600 scenes): on-rails pressure kill, crash through terrain loaded / unloaded") {
    std::map<std::string, int> seen;
    for (const json& sc : Ref().at("check_kill")) {
        FlightEnv w;
        // pressureAbove1: Kerbin (101 kPa up to 25 km, the case altitudes are below that), else the airless Mun (GetPressure == 0)
        CelestialBody* body = sc["pressureAbove1"].get<bool>() ? w.Body("Kerbin") : w.Body("Mun");
        Vessel* other = w.MakeVessel("other", body, 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys);
        other->vesselTransform.position = V3f(sc["apos"]);
        Vessel* v = w.MakeVessel("v", body, 700000.0, 0.0, sc["loaded"].get<bool>(), sc["packed"].get<bool>());
        v->vesselTransform.position = V3f(sc["pos"]);
        v->vesselRanges = RangesOf(sc["ranges"]);
        SetSituation(*v, sc["situation"].get<int>(), sc["wasFlying"].get<bool>());
        v->Landed = sc["landedOrSplashed"].get<bool>();  // LandedOrSplashed = Landed || Splashed
        v->altitude = D(sc["altitude"]);
        v->terrainAltitude = D(sc["terrainAltitude"]);
        CHECK((v->mainBody()->GetPressure(v->altitude) > 1.0) == sc["pressureAbove1"].get<bool>());
        Vessel::env.LoadedSceneIsFlight = sc["sceneFlight"].get<bool>();
        FlightGlobals::ActiveVessel = !sc["hasActive"].get<bool>() ? nullptr : (sc["isActive"].get<bool>() ? v : other);
        FlightGlobals::ready = sc["ready"].get<bool>();
        FlightGlobals::overrideOrbit = sc["override"].get<bool>();
        Vessel::env.frameCount = sc["holdExpired"].get<bool>() ? 1000 : 0;
        bool crashEvent = false, explode = false;
        const auto hCrash = decltype(GE::onVesselExplodeGroundCollision)::OnEvent::Lambda(&seen, [&](Vessel*) { crashEvent = true; });
        const auto hExplode = decltype(GE::onCrash)::OnEvent::Lambda(&seen, [&](EventReport*) { explode = true; });
        GE::onVesselExplodeGroundCollision.Add(hCrash);
        GE::onCrash.Add(hExplode);
        v->CheckKill();
        GE::onVesselExplodeGroundCollision.Remove(hCrash);
        GE::onCrash.Remove(hExplode);
        std::string got = "none";
        if (explode) got = "crash_explode";
        else if (crashEvent && v->state == Vessel::State::DEAD) got = "crash_die";
        else if (v->state == Vessel::State::DEAD) got = "die";
        INFO("scene " << sc.dump().substr(0, 600));
        CHECK(got == sc["result"].get<std::string>());
        seen[got]++;
    }
    CHECK(seen["die"] > 20);
    CHECK(seen["crash_die"] > 10);
    CHECK(seen["crash_explode"] > 10);
    CHECK(seen["none"] > 50);
}

TEST_CASE("WP-19a updateSituation equals the reference (800 scenes): the LANDED / SPLASHED / FLYING / SUB_ORBITAL / ORBITING / ESCAPING / PRELAUNCH thresholds") {
    std::map<int, int> seen;
    for (const json& sc : Ref().at("situation")) {
        FlightEnv w;
        CelestialBody body;
        body.bodyName = "Fixture";
        body.sphereOfInfluence = D(sc["soi"]);
        body.atmosphere = sc["atmo"].get<bool>();
        body.atmosphereDepth = D(sc["depth"]);
        body.Radius = D(sc["radius"]);
        Vessel* v = w.MakeVessel("v", w.Body("Kerbin"), 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::TRACK_Phys, false);
        v->orbitDriver->orbit->referenceBody = &body;
        v->orbitDriver->orbit->eccentricity = D(sc["ecc"]);
        v->orbitDriver->orbit->semiMajorAxis = D(sc["sma"]);
        v->Landed = sc["landed"].get<bool>();
        v->Splashed = sc["splashed"].get<bool>();
        v->situation = static_cast<Vessel::Situations>(sc["situation"].get<int>());
        v->srfSpeed = D(sc["srfSpeed"]);
        v->precalc.set_isEasingGravity(false);
        v->vesselSpawning = sc["spawning"].get<bool>();
        v->staticPressurekPa = D(sc["pressure"]);
        if (sc["easing"].get<bool>()) v->precalc.set_isEasingGravity(true);
        v->updateSituation();
        CHECK(static_cast<int>(v->situation) == sc["result"].get<int>());
        seen[sc["result"].get<int>()]++;
        v->precalc.set_isEasingGravity(false);
        v->orbitDriver->orbit->referenceBody = w.Body("Kerbin");
    }
    for (int s : {1, 2, 4, 8, 16, 32, 64}) CHECK(seen[s] > 10);
    CHECK(seen[128] == 0);  // DOCKED is never produced by updateSituation (it is a mask used by Contracts / VesselRanges)
}
