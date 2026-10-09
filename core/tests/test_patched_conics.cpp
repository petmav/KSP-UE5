// WP-08: PatchedConics / PatchedConicSolver / ManeuverNode / OrbitDriver on the spawned Kerbol system (the bit-exact
// numbers are the golden gates golden.patched_conics and capture.orbit_rails; these tests cover API behaviour):
//   Planetarium.Orbits registration, Kerbin -> Mun encounter -> escape chain, hyperbolic escape to Kerbol, degenerate
//   (no encounter, Sun-centred hyperbola, no Planetarium, IDLE driver, paused game), patch-limit handling, maneuver-node
//   burn-vector conventions and save/load, and an on-rails SOI transition through OrbitDriver.
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/celestial/celestial_body_orbit.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/orbit/maneuver_node.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/orbit/patched_conic_solver.hpp"
#include "ksp/orbit/patched_conics.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/orbit/trajectory.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/util/ksp_util.hpp"

using namespace ksp;
using PT = Orbit::PatchTransitionType;
using ESL = Orbit::EncounterSolutionLevel;

namespace {

const std::string kHarness = KSPCORE_HARNESS_DIR;
constexpr double kPI = 3.141592653589793;

// The in-game start-up (as tests/test_celestial.cpp), with the global state WP-08 touches reset around each test.
struct Env {
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    Env() {
        GameSettings::CONIC_PATCH_LIMIT = 3;
        GameSettings::ALWAYS_SHOW_TARGET_APPROACH_MARKERS = false;
        Time::timeScale = 1.0f;
        Time::fixedDeltaTime = 0.02f;
        Time::time = 0.0f;
        TimeWarp::fetch = nullptr;
        pg.physicsDatabaseFilename = kHarness + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        pg.CalculateValues(sys->Bodies(), planetarium.Sun);
        // An inertial frame (no body takes the inverse-rotation branch: the stock prefab's Mun/Minmus flags cleared as
        // PSystemSetup does), so frames do not turn while time advances.
        for (CelestialBody* b : sys->Bodies()) b->inverseRotation = false;
    }
    ~Env() {
        sys.reset();
        TimeWarp::fetch = nullptr;
        GameSettings::CONIC_PATCH_LIMIT = 3;
        Time::timeScale = 1.0f;
        Time::fixedDeltaTime = 0.02f;
        Time::time = 0.0f;
    }
    CelestialBody* Body(const char* n) const { return sys->GetBodyByName(n); }
};

// A vessel-like driver host (the Vessel package implements IOrbitDriverVessel for real vessels).
struct TestVessel final : IOrbitDriverVessel {
    Vector3d position;
    bool destroyed = false;
    Vessel* AsVessel() override { return nullptr; }
    std::string GetDisplayName() const override { return "test vessel"; }
    Vector3d RotatedLocalCoM() const override { return Vector3d::zero(); }
    void SetPosition(const Vector3d& p) override { position = p; }
    void UnloadAndDestroy() override { destroyed = true; }
    Vector3f TransformPosition() const override { return position.ToVector3f(); }
    bool HasRootPartRigidbody() const override { return false; }
    bool RootPartRigidbodyIsKinematic() const override { return true; }
    Vector3d CoMD() const override { return position; }
    Vector3d velocityD() const override { return Vector3d::zero(); }
    void ForceVesselLogFlyby(CelestialBody*) override {}
};

// harness OrbitGen.cs PatchCases: Kerbin -> Mun transfer timed for an encounter at apoapsis (+0.05 rad lead)
Orbit KerbinToMun(const Env& env) {
    CelestialBody* kerbin = env.Body("Kerbin");
    const double rp = 700000, ra = 12.0e6, a = (rp + ra) / 2, e = (ra - rp) / (ra + rp);
    const double n = std::sqrt(kerbin->gravParameter / (a * a * a));
    const Orbit& mun = *BodyOrbit(env.Body("Mun"));
    double phi = (mun.meanAnomalyAtEpoch + mun.meanMotion * (kPI / n) - kPI + 0.05) * 180 / kPI;
    phi = std::fmod(std::fmod(phi, 360.0) + 360.0, 360.0);
    return Orbit(0, e, a, 0, phi, 0, 0, kerbin);
}

Orbit KerbinEscape(const Env& env) {  // OrbitGen.cs: 1.3x escape speed at 700 km
    CelestialBody* kerbin = env.Body("Kerbin");
    const double rp = 700000, v = 1.3 * std::sqrt(2 * kerbin->gravParameter / rp);
    const double a = 1.0 / (2.0 / rp - v * v / kerbin->gravParameter);
    return Orbit(2, 1 - rp / a, a, 40, 75, 0, 0, kerbin);
}

// The patch loop of PatchedConicSolver.Update over plain orbits (as the harness generator re-hosts it).
std::vector<std::shared_ptr<Orbit>> Solve(const Orbit& first, double ut, int patchLimit, bool* more = nullptr) {
    PatchedConics::SolverParameters pars;
    std::vector<std::shared_ptr<Orbit>> p{std::make_shared<Orbit>(first)};
    for (int i = 0; i < patchLimit + 1; ++i) p.push_back(std::make_shared<Orbit>());
    p[0]->patchStartTransition = PT::INITIAL;
    p[0]->StartUT = ut;
    p[0]->EndUT = p[0]->eccentricity < 1.0 ? ut + p[0]->period : p[0]->period;
    int num = 0;
    bool m;
    do {
        m = PatchedConics::CalculatePatch(*p[num], *p[num + 1], num != 0 ? p[num]->epoch : ut, pars, nullptr);
        ++num;
    } while (m && num < patchLimit);
    if (more) *more = m;
    p.resize(static_cast<size_t>(num + (m ? 1 : 0)));
    return p;
}

double Rel(double a, double b) { return std::fabs(a - b) / std::max(std::fabs(b), 1e-300); }
double RelV(const Vector3d& a, const Vector3d& b) { return (a - b).magnitude() / std::max(b.magnitude(), 1e-300); }

}  // namespace

TEST_CASE("Planetarium.Orbits: body OrbitDrivers registered in spawn order, removed with their owners") {
    CHECK(Planetarium::Orbits() == nullptr);
    {
        Env env;
        std::vector<OrbitDriver*>* orbits = Planetarium::Orbits();
        REQUIRE(orbits != nullptr);
        REQUIRE(orbits->size() == 16);
        size_t i = 0;
        for (CelestialBody* b : env.sys->SpawnOrder()) {
            if (!b->orbitDriver) continue;
            OrbitDriver* d = (*orbits)[i++];
            CHECK(d->celestialBody == b);
            CHECK(d->orbit.get() == BodyOrbit(b));
            CHECK(d->referenceBody() == b->orbitDriver->referenceBody);
            CHECK(d->updateMode == OrbitDriver::UpdateMode::UPDATE);
            CHECK(d->QueuedUpdate);  // CelestialBody.cs:483
            CHECK(d->Ready());
        }
        CHECK(FlightGlobals::bodies == &env.sys->Bodies());
        // a vessel driver joins the list at Start and leaves it when destroyed
        {
            OrbitDriver d;
            d.orbit = std::make_shared<Orbit>(0, 0, 700000, 0, 0, 0, 0, env.Body("Kerbin"));
            d.updateMode = OrbitDriver::UpdateMode::UPDATE;
            d.Start();
            CHECK(orbits->size() == 17);
            CHECK(orbits->back() == &d);
            CHECK(d.UpdateUT() == 0.0);
        }
        CHECK(orbits->size() == 16);
        env.sys.reset();
        CHECK(orbits->empty());
        CHECK(FlightGlobals::bodies == nullptr);
    }
    CHECK(Planetarium::Orbits() == nullptr);
}

TEST_CASE("PatchedConics: Kerbin -> Mun encounter -> Mun escape -> Kerbin chain") {
    Env env;
    CelestialBody* kerbin = env.Body("Kerbin");
    CelestialBody* mun = env.Body("Mun");
    bool more = false;
    const auto p = Solve(KerbinToMun(env), 0.0, 3, &more);
    REQUIRE(p.size() == 4);
    CHECK(more);
    // patch 0: Kerbin, ends at the Mun encounter
    CHECK(p[0]->referenceBody == kerbin);
    CHECK(p[0]->patchStartTransition == PT::INITIAL);
    CHECK(p[0]->patchEndTransition == PT::ENCOUNTER);
    CHECK(p[0]->closestEncounterLevel == ESL::SOI_INTERSECT_1);
    CHECK(p[0]->closestEncounterBody == mun);
    CHECK(p[0]->activePatch);
    CHECK(p[0]->nextPatch == p[1].get());
    CHECK(p[0]->EndUT == p[0]->UTsoi);
    CHECK(p[0]->ClAppr < mun->sphereOfInfluence);
    // the transition is on the Mun's SOI sphere and the vessel is approaching the Mun there
    const Vector3d relMun = p[0]->getTruePositionAtUT(p[0]->UTsoi) - mun->getTruePositionAtUT(p[0]->UTsoi);
    CHECK(Rel(relMun.magnitude(), mun->sphereOfInfluence) < 1e-6);
    // patch 1: Mun, starts where patch 0 ended (same position and velocity), leaves the Mun's SOI
    CHECK(p[1]->referenceBody == mun);
    CHECK(p[1]->patchStartTransition == PT::ENCOUNTER);
    CHECK(p[1]->patchEndTransition == PT::ESCAPE);
    CHECK(p[1]->previousPatch == p[0].get());
    CHECK(p[1]->StartUT == p[0]->EndUT);
    CHECK(p[1]->epoch == p[0]->UTsoi);
    CHECK(p[1]->eccentricity > 1.0);  // a flyby
    const double t1 = p[0]->UTsoi;
    CHECK(RelV(p[1]->getTruePositionAtUT(t1), p[0]->getTruePositionAtUT(t1)) < 1e-12);
    const Vector3d v0 = p[0]->getOrbitalVelocityAtUT(t1);
    const Vector3d v1 = p[1]->getOrbitalVelocityAtUT(t1) + BodyOrbit(mun)->getOrbitalVelocityAtUT(t1);
    CHECK(RelV(v1, v0) < 1e-9);
    CHECK(Rel(p[1]->getRelativePositionAtUT(p[1]->UTsoi).magnitude(), mun->sphereOfInfluence) < 1e-9);
    // patch 2: back around Kerbin
    CHECK(p[2]->referenceBody == kerbin);
    CHECK(p[2]->patchStartTransition == PT::ESCAPE);
    CHECK(p[2]->StartUT == p[1]->EndUT);
    CHECK(RelV(p[2]->getTruePositionAtUT(p[1]->UTsoi), p[1]->getTruePositionAtUT(p[1]->UTsoi)) < 1e-12);
    // agrees with the harness golden (patched_conics/kerbin_to_mun) for frame-independent quantities
    CHECK(Rel(p[0]->UTsoi, 21613.6800943523) < 1e-9);
    CHECK(Rel(p[1]->UTsoi, 32358.700211378764) < 1e-9);
    CHECK(p[3]->referenceBody == env.Body("Sun"));
}

TEST_CASE("PatchedConics: hyperbolic escape from Kerbin to Kerbol") {
    Env env;
    CelestialBody* kerbin = env.Body("Kerbin");
    CelestialBody* sun = env.Body("Sun");
    bool more = true;
    const auto p = Solve(KerbinEscape(env), 0.0, 6, &more);
    REQUIRE(p.size() == 2);
    CHECK_FALSE(more);
    CHECK(p[0]->eccentricity > 1.0);
    CHECK(p[0]->patchEndTransition == PT::ESCAPE);
    CHECK(p[0]->FEVp == p[0]->TrueAnomalyAtRadius(kerbin->sphereOfInfluence));
    CHECK(p[0]->SEVp == -p[0]->FEVp);
    CHECK(p[0]->UTsoi == 0.0 + p[0]->timeToTransition1);
    CHECK(Rel(p[0]->getRelativePositionAtUT(p[0]->UTsoi).magnitude(), kerbin->sphereOfInfluence) < 1e-9);
    CHECK(p[1]->referenceBody == sun);
    CHECK(p[1]->patchStartTransition == PT::ESCAPE);
    CHECK(p[1]->patchEndTransition == PT::FINAL);
    CHECK(p[1]->eccentricity < 1.0);  // captured by the Sun
    CHECK(p[1]->EndUT == p[1]->StartUT + p[1]->period);
    CHECK(p[1]->UTsoi == -1.0);
    const double t = p[0]->UTsoi;
    CHECK(RelV(p[1]->getTruePositionAtUT(t), p[0]->getTruePositionAtUT(t)) < 1e-12);
    const Vector3d vSun = p[0]->getOrbitalVelocityAtUT(t) + BodyOrbit(kerbin)->getOrbitalVelocityAtUT(t);
    CHECK(RelV(p[1]->getOrbitalVelocityAtUT(t), vSun) < 1e-12);
    CHECK(Rel(p[0]->UTsoi, 31133.878371757928) < 1e-9);  // golden patched_conics/kerbin_escape
}

TEST_CASE("PatchedConics: degenerate cases") {
    Env env;
    CelestialBody* kerbin = env.Body("Kerbin");
    CelestialBody* sun = env.Body("Sun");
    SUBCASE("low Kerbin orbit: one FINAL patch, nothing intersected") {
        bool more = true;
        const auto p = Solve(Orbit(0, 0, 700000, 0, 0, 0, 0, kerbin), 0.0, 6, &more);
        REQUIRE(p.size() == 1);
        CHECK_FALSE(more);
        CHECK(p[0]->patchEndTransition == PT::FINAL);
        CHECK(p[0]->closestEncounterLevel == ESL::NONE);
        CHECK(p[0]->closestEncounterBody == nullptr);
        CHECK(p[0]->UTsoi == -1.0);
        CHECK(p[0]->EndUT == p[0]->StartUT + p[0]->period);
    }
    SUBCASE("hyperbola around the Sun (infinite SOI): FINAL, open-ended") {
        Orbit o(0, 1.5, -2.0e10, 0, 0, 0, 0, sun);
        Orbit next;
        PatchedConics::SolverParameters pars;
        o.StartUT = 0;
        o.EndUT = o.period;
        CHECK_FALSE(PatchedConics::CalculatePatch(o, next, 0.0, pars, nullptr));
        CHECK(o.patchEndTransition == PT::FINAL);
        CHECK(std::isinf(o.EndUT));
        CHECK(std::isinf(o.UTsoi));
        CHECK(o.FEVp == dotnet::Math::Acos(0.0 - 1.0 / 1.5));
        CHECK(next.previousPatch == &o);
        CHECK(next.patchStartTransition == PT::FINAL);
    }
    SUBCASE("debug_disableEscapeCheck: an escaping orbit stays FINAL") {
        Orbit o = KerbinEscape(env);
        Orbit next;
        PatchedConics::SolverParameters pars;
        pars.debug_disableEscapeCheck = true;
        o.StartUT = 0;
        o.EndUT = o.period;
        CHECK_FALSE(PatchedConics::CalculatePatch(o, next, 0.0, pars, nullptr));
        CHECK(o.patchEndTransition == PT::FINAL);
    }
    SUBCASE("AngleWrap / TAIsWithinPatchBounds") {
        CHECK(PatchedConics::AngleWrap(0.0) == 0.0);
        CHECK(PatchedConics::AngleWrap(3 * kPI) == 3 * kPI - 2.0 * 2.0 * kPI);
        CHECK(PatchedConics::AngleWrap(-kPI) == -kPI);
        Orbit h(0, 1.5, -1.0e6, 0, 0, 0, 0, kerbin);
        h.fromV = -1.0;
        h.toV = 1.0;
        CHECK(PatchedConics::TAIsWithinPatchBounds(0.5, h));
        CHECK_FALSE(PatchedConics::TAIsWithinPatchBounds(2.0, h));
        Orbit e(0, 0.1, 1.0e6, 0, 0, 0, 0, kerbin);  // closed FINAL orbit: always inside
        e.patchEndTransition = PT::FINAL;
        e.fromV = -1.0;
        e.toV = 1.0;
        CHECK(PatchedConics::TAIsWithinPatchBounds(2.0, e));
    }
}

TEST_CASE("PatchedConics: no Planetarium -> the exception is caught and the patch is not continued") {
    REQUIRE(Planetarium::fetch == nullptr);
    Orbit o;  // no reference body (Init would need one); CalculatePatch fails at Planetarium.Orbits first
    o.semiMajorAxis = 700000;
    Orbit next;
    PatchedConics::SolverParameters pars;
    CHECK_FALSE(PatchedConics::CalculatePatch(o, next, 0.0, pars, nullptr));
    CHECK(o.activePatch);  // the statements before the failing Planetarium.Orbits access ran
    CHECK(o.patchEndTransition == PT::FINAL);
}

TEST_CASE("PatchedConicSolver: patch list, patch limit, IDLE / pause") {
    Env env;
    OrbitDriver driver;
    driver.orbit = std::make_shared<Orbit>(KerbinToMun(env));
    driver.updateMode = OrbitDriver::UpdateMode::UPDATE;
    driver.Start();
    {
        PatchedConicSolver solver(driver);
        CHECK(solver.PatchLimit() == 3);  // GameSettings.CONIC_PATCH_LIMIT
        REQUIRE(solver.patches.size() == 150);
        CHECK(solver.patches[0].get() == driver.orbit.get());
        solver.Update();
        CHECK(solver.patchesAhead == 2);
        CHECK(solver.MorePatchesAhead);
        CHECK(solver.patches[0]->patchEndTransition == PT::ENCOUNTER);
        CHECK(solver.patches[1]->referenceBody == env.Body("Mun"));
        CHECK(solver.patches[2]->activePatch);
        CHECK_FALSE(solver.patches[3]->activePatch);
        CHECK(solver.LastActivePatch().get() == solver.patches[2].get());
        CHECK(solver.FindPatchContainingUT(1000.0).get() == solver.patches[0].get());
        CHECK(solver.FindPatchContainingUT(solver.patches[1]->StartUT + 1.0).get() == solver.patches[1].get());

        solver.DecreasePatchLimit();
        solver.DecreasePatchLimit();
        solver.DecreasePatchLimit();
        CHECK(solver.PatchLimit() == 1);  // Math.Max(1, patchLimit - 1)
        solver.Update();
        CHECK(solver.patchesAhead == 0);
        CHECK(solver.patches[0]->activePatch);
        CHECK_FALSE(solver.patches[1]->activePatch);
        CHECK(solver.LastActivePatch().get() == solver.patches[0].get());
        for (int i = 0; i < 200; ++i) solver.IncreasePatchLimit();
        CHECK(solver.PatchLimit() == 201);
        solver.Update();
        CHECK(solver.PatchLimit() == 149);  // clamped to maxTotalPatches - 1
        CHECK(solver.patchesAhead == 3);   // the chain ends FINAL around the Sun

        GameSettings::CONIC_PATCH_LIMIT = 0;  // OnGameSettingsApplied: Mathf.Max(limit, 1)
        GameEvents::OnGameSettingsApplied.Fire();
        CHECK(solver.PatchLimit() == 1);
        GameSettings::CONIC_PATCH_LIMIT = 5;
        GameEvents::OnGameSettingsApplied.Fire();
        CHECK(solver.PatchLimit() == 5);

        // IDLE driver / paused game / timeScale 0: Update leaves the patches alone (activePatch cleared or untouched)
        solver.Update();
        CHECK(solver.patches[0]->activePatch);
        Time::timeScale = 0.0f;
        solver.Update();
        CHECK(solver.patches[0]->activePatch);  // returned before clearing
        Time::timeScale = 1.0f;
        driver.updateMode = OrbitDriver::UpdateMode::IDLE;
        solver.Update();
        CHECK_FALSE(solver.patches[0]->activePatch);  // cleared, not recomputed
        driver.updateMode = OrbitDriver::UpdateMode::UPDATE;
    }
    GameSettings::CONIC_PATCH_LIMIT = 0;
    PatchedConicSolver s2(driver);
    CHECK(s2.PatchLimit() == 1);  // Awake: Mathf.Max(GameSettings.CONIC_PATCH_LIMIT, 1)
}

TEST_CASE("ManeuverNode: burn vector conventions, flight plan, save/load") {
    Env env;
    CelestialBody* kerbin = env.Body("Kerbin");
    OrbitDriver driver;
    driver.orbit = std::make_shared<Orbit>(0, 0, 700000, 0, 0, 0, 0, kerbin);  // circular, equatorial, prograde
    driver.updateMode = OrbitDriver::UpdateMode::UPDATE;
    driver.Start();
    PatchedConicSolver solver(driver);
    solver.Update();
    const double ut = 600.0;
    const std::shared_ptr<ManeuverNode> node = solver.AddManeuverNode(ut);
    REQUIRE(node->patch.get() != nullptr);
    REQUIRE(node->nextPatch.get() != nullptr);
    CHECK(node->patch.get() == solver.patches[0].get());
    CHECK(node->solver == &solver);
    CHECK(node->nextPatch->patchStartTransition == PT::MANEUVER);
    CHECK(node->nextPatch->StartUT == ut);
    CHECK(node->nextPatch->previousPatch == node->patch.get());
    // zero burn: the post-burn orbit is the same orbit
    CHECK(node->GetBurnVector(node->patch.get()).magnitude() < 1e-9);

    const Vector3d r = node->patch->getRelativePositionAtUT(ut).xzy();  // Unity axes
    const Vector3d v = node->patch->getOrbitalVelocityAtUT(ut).xzy();
    const Vector3d rHat = r.normalized(), vHat = v.normalized();
    const Vector3d nHat = Vector3d::Cross(-r, v).normalized();  // LookRotation's up: the node's normal axis
    auto burn = [&](const Vector3d& dv) {
        node->OnGizmoUpdated(dv, ut);  // DeltaV/UT set, flight plan rebuilt (no gizmo attached)
        return node->GetBurnVector(node->patch.get());
    };
    // DeltaV.z = prograde
    Vector3d b = burn(Vector3d(0, 0, 100));
    CHECK(std::fabs(b.magnitude() - 100.0) < 1e-6);
    CHECK(Vector3d::Dot(b.normalized(), vHat) > 1.0 - 1e-12);
    CHECK(node->nextPatch->semiMajorAxis > node->patch->semiMajorAxis);
    const Vector3d partial = node->GetPartialDv();  // in the (prograde, normal) LookRotation frame of the patch
    CHECK(std::fabs(partial.z - 100.0) < 1e-6);
    CHECK(std::fabs(partial.x) < 1e-6);
    CHECK(std::fabs(partial.y) < 1e-6);
    // DeltaV.y = normal (Cross(-pos, vel) in Unity axes), DeltaV.x = radial: Cross(up, forward) = -r for this orbit
    b = burn(Vector3d(0, 100, 0));
    CHECK(Vector3d::Dot(b.normalized(), nHat) > 1.0 - 1e-12);
    CHECK(std::fabs(node->nextPatch->inclination) > 0.1);
    b = burn(Vector3d(100, 0, 0));
    const double radialDot = Vector3d::Dot(b.normalized(), rHat);
    CHECK(radialDot > 1.0 - 1e-12);  // DeltaV.x = radial out
    const Vector3d expectedRight = Vector3d::Cross(nHat, vHat);  // Unity left-handed LookRotation: right = up x forward
    CHECK(Vector3d::Dot(b.normalized(), expectedRight) > 1.0 - 1e-12);
    // the solver's own transform, for any DeltaV
    const Vector3d dv(12.5, -7.25, 30.0);
    b = burn(dv);
    const Vector3d expected = QuaternionD::LookRotation(v, Vector3d::Cross(-r, v)) * dv;
    CHECK(RelV(b, expected) < 1e-9);
    // nodeRotation (PatchedConicRenderer): forward axis = prograde
    node->AttachGizmo();
    REQUIRE(node->attachedGizmo.get() != nullptr);
    CHECK(node->attachedGizmo->patchBefore.get() == node->patch.get());
    node->UpdateNodeRotation();
    const QuaternionD q(node->nodeRotation);
    CHECK(Vector3d::Dot((q * Vector3d(0, 0, 1)).normalized(), vHat) > 1.0 - 1e-6);
    CHECK(node->attachedGizmo->rotation.Equals(node->nodeRotation));
    node->DetachGizmo();
    CHECK(node->attachedGizmo.get() == nullptr);

    // save / load (ManeuverNode.cs:105/117; PatchedConicSolver.Save/Load + the Start coroutine)
    ConfigNode saved("VESSEL");
    solver.Save(saved);
    REQUIRE(saved.GetNodes("MANEUVER").size() == 1);
    ManeuverNode loaded;
    loaded.Load(*saved.GetNodes("MANEUVER")[0]);
    CHECK(loaded.UT == node->UT);
    CHECK(loaded.DeltaV.x == node->DeltaV.x);
    CHECK(loaded.DeltaV.y == node->DeltaV.y);
    CHECK(loaded.DeltaV.z == node->DeltaV.z);
    PatchedConicSolver solver2(driver);
    solver2.Load(saved);
    CHECK(solver2.maneuverNodes.empty());
    CHECK(solver2.StartMoveNext());   // yield before the first node
    CHECK(solver2.maneuverNodes.empty());
    solver2.Update();
    CHECK_FALSE(solver2.StartMoveNext());  // node added, flight plan built
    REQUIRE(solver2.maneuverNodes.size() == 1);
    CHECK(solver2.maneuverNodes[0]->solver == &solver2);
    CHECK(solver2.maneuverNodes[0]->nextPatch.get() != nullptr);
    CHECK_FALSE(solver2.StartMoveNext());

    // two nodes: sorted by UT, the second one starts from the first one's post-burn orbit
    const auto n2 = solver.AddManeuverNode(ut + 300.0);
    const auto n0 = solver.AddManeuverNode(ut - 300.0);
    REQUIRE(solver.maneuverNodes.size() == 3);
    CHECK(solver.maneuverNodes[0].get() == n0.get());
    CHECK(solver.maneuverNodes[2].get() == n2.get());
    CHECK(solver.FindManeuverIndex(*n2) == 2);
    CHECK(n2->patch.get() == node->nextPatch.get());
    CHECK(node->patch.get() == n0->nextPatch.get());
    CHECK(node->patch->patchEndTransition == PT::MANEUVER);
    CHECK(node->patch->EndUT == node->UT);
    // RemoveSelf: the node leaves the list, the plan is rebuilt
    node->RemoveSelf();
    REQUIRE(solver.maneuverNodes.size() == 2);
    CHECK(n2->patch.get() == n0->nextPatch.get());
    n0->RemoveSelf();
    n2->RemoveSelf();
    CHECK(solver.maneuverNodes.empty());
    CHECK(solver.flightPlan.empty());
}

TEST_CASE("OrbitDriver: on-rails SOI transition Kerbin -> Mun under time warp matches the predicted patch") {
    Env env;
    CelestialBody* kerbin = env.Body("Kerbin");
    CelestialBody* mun = env.Body("Mun");
    const Orbit start = KerbinToMun(env);
    const auto predicted = Solve(start, 0.0, 2);
    REQUIRE(predicted[0]->patchEndTransition == PT::ENCOUNTER);

    TimeWarp tw;
    tw.LoadStockPrefab();  // updateRate reads MaxPhysicsRate = warpRates[maxPhysicsRate_index]
    TimeWarp::fetch = &tw;
    tw.curr_rate = 1000.0f;  // assumeWarpRate(instant) -> updateRate
    tw.updateRate(tw.curr_rate);
    CHECK(env.planetarium.timeScale == 1000.0);
    CHECK(TimeWarp::fixedDeltaTime() == 20.0f);

    TestVessel vessel;
    OrbitDriver driver;
    driver.vessel = &vessel;
    driver.orbit = std::make_shared<Orbit>(start);
    driver.updateMode = OrbitDriver::UpdateMode::UPDATE;
    driver.Start();
    std::vector<CelestialBody*> changes;
    driver.OnReferenceBodyChange += OrbitDriver::CelestialBodyDelegate::Lambda(&changes, [&changes](CelestialBody* b) { changes.push_back(b); });
    int soiEvents = 0;
    const auto listener = decltype(GameEvents::onVesselSOIChanged)::OnEvent::Lambda(
        &soiEvents, [&soiEvents, kerbin, mun](GameEvents::HostedFromToAction<Vessel*, CelestialBody*> a) {
            ++soiEvents;
            CHECK(a.from == kerbin);
            CHECK(a.to == mun);
        });
    GameEvents::onVesselSOIChanged.Add(listener);

    double transitionUT = -1.0;
    for (int frame = 0; frame < 3000 && transitionUT < 0; ++frame) {
        env.planetarium.FixedUpdate();
        driver.UpdateOrbit();
        Time::time = Time::time + Time::fixedDeltaTime;
        driver.RunDelayedCalls();
        if (driver.referenceBody() == mun) transitionUT = Planetarium::GetUniversalTime();
    }
    GameEvents::onVesselSOIChanged.Remove(listener);
    REQUIRE(transitionUT > 0);
    CHECK(changes.size() == 1);
    CHECK(soiEvents == 1);
    CHECK(driver.frameShift);  // locked until Invoke("unlockFrameSwitch", 1f) runs
    CHECK(driver.FrameSwitchUnlockPending());
    // the crossing found by OnRailsSOITransition (BSP over the last frame) is the one the patch solver predicted
    const Orbit& now = *driver.orbit;
    CHECK(transitionUT - 20.0 <= predicted[0]->UTsoi);
    CHECK(predicted[0]->UTsoi <= transitionUT);
    CHECK(std::fabs(now.epoch - predicted[0]->UTsoi) < 1e-3);
    CHECK(Rel(now.semiMajorAxis, predicted[1]->semiMajorAxis) < 1e-6);
    CHECK(Rel(now.eccentricity, predicted[1]->eccentricity) < 1e-6);
    CHECK(Rel(now.PeR(), predicted[1]->PeR()) < 1e-6);
    // updateFromParameters moved the vessel before CheckDominantBody switched the orbit: in the transition frame pos and
    // the vessel position are still Kerbin-relative (KSP order); the next update is Mun-relative
    CHECK(RelV(vessel.position, kerbin->position + driver.pos) < 1e-15);
    env.planetarium.FixedUpdate();
    driver.UpdateOrbit();
    CHECK(driver.referenceBody() == mun);
    CHECK(RelV(vessel.position, mun->position + driver.pos) < 1e-15);
    CHECK(changes.size() == 1);  // frameShift still locked: no second RecalculateOrbit
    // frameShift is released 1 s (Unity Time.time) later
    for (int i = 0; i < 60 && driver.frameShift; ++i) {
        Time::time = Time::time + Time::fixedDeltaTime;
        driver.RunDelayedCalls();
    }
    CHECK_FALSE(driver.frameShift);
    TimeWarp::fetch = nullptr;
}

TEST_CASE("Trajectory: patch sampling and frame conversions") {
    Env env;
    CelestialBody* kerbin = env.Body("Kerbin");
    CelestialBody* mun = env.Body("Mun");
    SUBCASE("closed FINAL orbit: a full ellipse in eccentric anomaly") {
        Orbit o(0, 0.2, 2.0e6, 0, 30, 0, 0, kerbin);
        o.patchEndTransition = PT::FINAL;
        o.StartUT = 0;
        o.EndUT = o.period;
        Trajectory t;
        t.UpdateFromOrbit(o, 64);
        REQUIRE(t.GetPoints() != nullptr);
        REQUIRE(t.GetPoints()->size() == 64);
        CHECK(t.referenceBody == kerbin);
        CHECK(t.patch == &o);
        CHECK(o.sampleInterval == 2.0 * kPI / 64.0);
        for (int i = 0; i < 64; ++i) {
            const double E = i * o.sampleInterval;
            CHECK(std::fabs((*t.GetPoints())[i].magnitude() - o.semiMajorAxis * (1.0 - o.eccentricity * std::cos(E))) < 1e-6);
        }
        CHECK(RelV((*t.GetPoints())[0], o.getRelativePositionAtT(0.0).xzy()) < 1e-12);  // E = 0: periapsis
        CHECK(RelV(t.GetPeriapsisLocal(), o.getRelativePositionAtT(0.0).xzy() + kerbin->position) < 1e-12);
        std::vector<Color> colors(64);
        t.GetColors(Color(1, 1, 1, 1), colors);
        CHECK(colors[0].a == 1.0f);  // FINAL: no faded ends
    }
    SUBCASE("non-final patch: StartUT..EndUT plus two samples on each side; ends faded") {
        const auto p = Solve(KerbinToMun(env), 0.0, 2);
        Orbit& o = *p[0];  // Kerbin, ends at the Mun encounter
        Trajectory t;
        t.UpdateFromOrbit(o, 25);
        const auto& pts = *t.GetPoints();
        CHECK(RelV(pts[2], o.getRelativePositionAtUT(o.StartUT).xzy()) < 1e-9);  // fromE + 2 intervals
        CHECK(RelV(pts[22], o.getRelativePositionAtUT(o.EndUT).xzy()) < 1e-9);   // toE
        // KSP quirk kept: fromE went negative (the patch starts at periapsis, minus two intervals), so every sample with
        // E > 0 gets one period added to its time (Trajectory.cs:73)
        const auto& times = *t.GetTimes();
        CHECK(std::fabs(times[22] - (o.EndUT + o.period)) < 1e-6 * o.EndUT);
        CHECK(RelV(t.GetPatchEndLocal(), o.getRelativePositionAtUT(o.EndUT).xzy() + kerbin->position) < 1e-12);
        // relative to the Mun at the encounter time: the end point lies on the Mun's SOI sphere
        const Vector3d rel = t.GetPatchEndRelative(mun) - mun->position;
        CHECK(Rel(rel.magnitude(), mun->sphereOfInfluence) < 1e-6);
        std::vector<Color> colors(25);
        t.GetColors(Color(1, 1, 1, 1), colors);
        CHECK(colors[0].a == 0.0f);
        CHECK(colors[1].a == 0.0f);
        CHECK(colors[2].a == 1.0f);
        CHECK(colors[23].a == 0.0f);
        // KSP quirk: Reframe* fill the new trajectory's (null) point array
        CHECK_THROWS_AS(t.ReframeToLocal(), NullReferenceException);
    }
    SUBCASE("hyperbolic escape patch: sampled from StartUT to the SOI exit") {
        const auto p = Solve(KerbinEscape(env), 0.0, 2);
        Orbit& o = *p[0];
        Trajectory t;
        t.UpdateFromOrbit(o, 30);
        CHECK(o.fromE == doctest::Approx(o.EccentricAnomalyAtUT(o.StartUT)));
        const auto& pts = *t.GetPoints();
        CHECK(Rel(pts.back().magnitude(), kerbin->sphereOfInfluence) < 1e-6);
        CHECK(t.GetApoapsisLocal().x == 0.0);  // hyperbola: no apoapsis
        CHECK(Rel(t.GetPeriapsisLocal().magnitude(), o.PeR()) < 1e-12);
    }
}
