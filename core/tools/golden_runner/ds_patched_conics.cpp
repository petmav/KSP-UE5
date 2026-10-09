// patched_conics candidate (WP-08 PatchedConics): mirrors harness/src/KspGolden/Generators/OrbitGen.cs PatchCases() and
// SolvePatches() (the re-hosted PatchedConicSolver.Update loop, PatchedConicSolver.cs:141-179, patchLimit 6, a fresh
// SolverParameters per case, targetBody null) around kspcore's PatchedConics::CalculatePatch.
//
// Environment = the harness's KspEnv (the runner's CelestialEnv): the Kerbol system spawned at UT 0 with the real
// run-time orbits, the harness frame (UseHarnessFrame), Planetarium.Orbits = the 16 body OrbitDrivers in spawn order
// (registered by OrbitDriver.Start inside CelestialSystem::Spawn, as KspEnv.InitBodies adds them).
// What this runner freezes, and why:
//   * Planetarium time stays at UT 0 and no FixedUpdate runs (KspEnv sets Planetarium.time = 0 and never advances it).
//   * Every CelestialBody.position is set to (0,0,0). The harness creates bodies with FormatterServices (position
//     never written) and its OrbitDriver rehost only calls Orbit.UpdateFromUT, so every body sits at the origin; kspcore's
//     spawn moves them (OrbitDriver.updateFromParameters: Mun = Kerbin + pos.xzy, Sun = Kerbin - pos.xzy). Patched conics
//     read positions in Orbit.SolveSOI / getPositionAtUT (referenceBody.position + ...) and in UpdateFromOrbitAtUT's
//     getTruePositionAtUT chain (ends at the Sun's position); with the origin everywhere the sums match the harness
//     bit for bit. (Measured: for these five cases the result is also bit-identical without the freeze, because every
//     position-dependent sum involves only Kerbin, which kspcore's reversed root keeps at the origin, and the SOI
//     changes to the Sun go through GetOrbitalStateVectorsAtUT; the freeze stays so other inputs cannot diverge.)
//   * The Mun / Minmus prefab flag inverseRotation = 1 is kept (the harness never cleared it): ZupAtT for Mun-centred
//     state vectors uses the Mun's rotating frame, exactly as the golden.
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "celestial_env.hpp"
#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/patched_conics.hpp"

using namespace ksp;
using golden::ColType;

namespace {

constexpr double kPI = 3.141592653589793;  // Math.PI

struct PatchCase {
    std::string id;
    std::shared_ptr<Orbit> orbit;
    double ut;
};

// OrbitGen.cs:236 PatchCases
std::vector<PatchCase> PatchCases(const CelestialSystem& sys) {
    namespace M = dotnet::Math;
    CelestialBody* kerbin = sys.GetBodyByName("Kerbin");
    CelestialBody* mun = sys.GetBodyByName("Mun");
    std::vector<PatchCase> cs;
    // 1. LEO, no encounter
    cs.push_back({"kerbin_leo", std::make_shared<Orbit>(0, 0, 700000, 0, 0, 0, 0, kerbin), 0.0});
    // 2. Hohmann-like Kerbin -> Mun, periapsis direction chosen so apoapsis meets the Mun (+0.05 rad lead)
    {
        const double rp = 700000, ra = 12.0e6, a = (rp + ra) / 2, e = (ra - rp) / (ra + rp);
        const double n = M::Sqrt(kerbin->gravParameter / (a * a * a));
        const double tHalf = kPI / n;
        const Orbit& munOrbit = *BodyOrbit(mun);
        const double munAngleAtAp = munOrbit.meanAnomalyAtEpoch + munOrbit.meanMotion * tHalf;
        double phi = (munAngleAtAp - kPI + 0.05) * 180 / kPI;
        phi = std::fmod(std::fmod(phi, 360.0) + 360.0, 360.0);
        cs.push_back({"kerbin_to_mun", std::make_shared<Orbit>(0, e, a, 0, phi, 0, 0, kerbin), 0.0});
    }
    // 3. Kerbin escape (1.3x escape speed at 700 km)
    {
        const double rp = 700000, vesc = M::Sqrt(2 * kerbin->gravParameter / rp), v = 1.3 * vesc;
        const double a = 1.0 / (2.0 / rp - v * v / kerbin->gravParameter);
        const double e = 1 - rp / a;
        cs.push_back({"kerbin_escape", std::make_shared<Orbit>(2, e, a, 40, 75, 0, 0, kerbin), 0.0});
    }
    // 4. Mun escape (hyperbolic at 30 km)
    {
        const double rp = 230000, vesc = M::Sqrt(2 * mun->gravParameter / rp), v = 1.2 * vesc;
        const double a = 1.0 / (2.0 / rp - v * v / mun->gravParameter);
        const double e = 1 - rp / a;
        cs.push_back({"mun_escape", std::make_shared<Orbit>(10, e, a, 0, 120, 0, 5000, mun), 5000.0});
    }
    // 5. Highly elliptical Kerbin orbit crossing the Mun and Minmus orbits
    cs.push_back({"kerbin_crossing", std::make_shared<Orbit>(4, 0.95, 4.0e7, 70, 10, 0, 0, kerbin), 0.0});
    return cs;
}

// OrbitGen.cs:272 SolvePatches: the PatchedConicSolver.Update loop (PatchedConicSolver.cs:141-179)
std::vector<std::shared_ptr<Orbit>> SolvePatches(const std::shared_ptr<Orbit>& first, double ut, int patchLimit) {
    PatchedConics::SolverParameters pars;
    std::vector<std::shared_ptr<Orbit>> patches{first};
    for (int i = 0; i < patchLimit + 1; ++i) patches.push_back(std::make_shared<Orbit>());
    first->patchStartTransition = Orbit::PatchTransitionType::INITIAL;
    first->StartUT = ut;
    first->EndUT = first->eccentricity < 1.0 ? first->StartUT + first->period : first->period;
    int num = 0;
    bool more;
    do {
        more = PatchedConics::CalculatePatch(*patches[static_cast<size_t>(num)], *patches[static_cast<size_t>(num + 1)],
                                             num != 0 ? patches[static_cast<size_t>(num)]->epoch : ut, pars, nullptr);
        ++num;
    } while (more && num < patchLimit);
    std::vector<std::shared_ptr<Orbit>> out;  // patches.Take(num + (more ? 1 : 0)).Where(p => p.referenceBody != null)
    const int take = num + (more ? 1 : 0);
    for (int i = 0; i < take; ++i)
        if (patches[static_cast<size_t>(i)]->referenceBody != nullptr) out.push_back(patches[static_cast<size_t>(i)]);
    return out;
}

}  // namespace

static int RunPatchedConics(const RunnerOptions& opt) {
    CelestialEnv& env = CelestialEnv::Get(opt);
    const CelestialSystem& sys = *env.system;
    for (CelestialBody* b : sys.Bodies()) b->position = Vector3d::zero();  // the harness's bodies never moved (see top)

    golden::File g;
    g.dataset = "patched_conics";
    g.description = "kspcore candidate for patched_conics (OrbitGen.cs PatchCases, PatchedConics::CalculatePatch, patchLimit 6).";
    golden::Table t("patches", "Patch sequences from PatchedConics.CalculatePatch, iterated as PatchedConicSolver.Update does (patchLimit 6).");
    constexpr ColType S = ColType::Str, F = ColType::F64;
    for (auto [n, ct] : std::vector<std::pair<const char*, ColType>>{
             {"case_id", S}, {"patch", ColType::I64}, {"referenceBody", S}, {"patchStartTransition", S}, {"patchEndTransition", S},
             {"StartUT", F}, {"EndUT", F}, {"UTsoi", F}, {"inclination", F}, {"eccentricity", F}, {"semiMajorAxis", F},
             {"LAN", F}, {"argumentOfPeriapsis", F}, {"meanAnomalyAtEpoch", F}, {"epoch", F}, {"PeR", F}, {"ApR", F},
             {"closestEncounterLevel", S}, {"closestEncounterBody", S}, {"ClAppr", F}, {"UTappr", F}})
        t.Col(n, ct);
    t.key = {"case_id", "patch"};
    for (const PatchCase& c : PatchCases(sys)) {
        const auto patches = SolvePatches(c.orbit, c.ut, 6);
        for (size_t i = 0; i < patches.size(); ++i) {
            const Orbit& p = *patches[i];
            t.Add({c.id, static_cast<int64_t>(i), p.referenceBody->bodyName, ToString(p.patchStartTransition),
                   ToString(p.patchEndTransition), p.StartUT, p.EndUT, p.UTsoi, p.inclination, p.eccentricity, p.semiMajorAxis,
                   p.LAN, p.argumentOfPeriapsis, p.meanAnomalyAtEpoch, p.epoch, p.PeR(), p.ApR(),
                   ToString(p.closestEncounterLevel), p.closestEncounterBody != nullptr ? p.closestEncounterBody->bodyName : "",
                   p.ClAppr, p.UTappr});
        }
    }
    g.tables.push_back(std::move(t));
    return WriteCandidate(opt, g, "patched_conics/patched_conics.json");
}

KSP_GOLDEN_DATASET("patched_conics", 80, RunPatchedConics);
