// orbit_propagation candidate (WP-07 Orbit): mirrors harness/src/KspGolden/Generators/OrbitGen.cs input-for-input
// (cases, Times(), state-vector cases, Kepler grid, System.Random(20261008) for the pseudo-random cases) and evaluates
// every column with kspcore's Orbit. Tables: cases, state_at_ut, from_state_vectors, state_vector_cases, kepler (all
// of the golden file; the patched_conics file the same generator writes is WP-08's gate).
//
// Central bodies are rebuilt the way the harness built them (harness/src/KspGolden/KspEnv.cs InitBodies and
// Rehosted/CelestialBodyLifecycle.cs), from harness/golden/serialized/celestial_bodies.serialized.json:
//   Awake:    Mass = R*R*(GeeASL*9.80665)/6.67408E-11; gravParameter = Mass*6.67408E-11   (CelestialBody.cs:456-475)
//   Start:    solarRotationPeriod adjustment of rotationPeriod                              (CelestialBody.cs:477-530)
//   OrbitDriver.Start: orbit.Init(); orbit.UpdateFromUT(0)
//   CBUpdate: tidally locked -> rotationPeriod = orbit.period; rotPeriodRecip = 1/rotationPeriod (CelestialBody.cs:532)
// Orbit reads only gravParameter and Radius from them, plus initialRotation / rotPeriodRecip / inverseRotation /
// directRotAngle in Planetarium.ZupAtT: the Mun and Minmus prefabs carry inverseRotation = 1 and the harness never
// cleared it, so GetOrbitalStateVectorsAtUT and UpdateFromUT (sv_* and u_* columns) for Mun/Minmus orbits are in
// those bodies' rotating inverse-rotation frame, exactly as KSP computes them.
#include <cmath>
#include <deque>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/orbit/orbit.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;

namespace {

constexpr double kPI = 3.141592653589793;
constexpr double kGravitationalAcceleration = 9.80665;  // PhysicsGlobals.GravitationalAcceleration (PhysicsGlobals.cs:111)
constexpr double kG = 6.67408E-11;

struct BodySystem {
    std::map<std::string, std::unique_ptr<CelestialBodyRef>> body;
    std::deque<Orbit> orbits;  // OrbitDriver orbits (stable addresses)
};

// KspEnv.InitBodies(ut = 0) restricted to the fields Orbit reads.
void BuildBodies(const std::string& serializedPath, BodySystem& sys) {
    auto text = dotnet::ReadAllText(serializedPath);
    if (!text) throw std::runtime_error("cannot read " + serializedPath);
    const auto j = nlohmann::ordered_json::parse(*text);
    std::vector<const nlohmann::ordered_json*> entries;
    for (const auto& b : j.at("bodies")) entries.push_back(&b);
    std::stable_sort(entries.begin(), entries.end(), [](auto* a, auto* b) {
        return a->at("spawnOrder").template get<int>() < b->at("spawnOrder").template get<int>();
    });
    struct Extra {
        CelestialBodyRef* cb;
        bool tidallyLocked, solarRotationPeriod;
        double GeeASL;
    };
    std::vector<Extra> extra;
    for (const auto* e : entries) {
        const auto& c = e->at("celestialBody");
        auto cb = std::make_unique<CelestialBodyRef>();
        cb->bodyName = c.at("bodyName").get<std::string>();
        cb->Radius = c.at("Radius").get<double>();
        cb->rotates = c.at("rotates").get<int>() != 0;
        cb->rotationPeriod = c.at("rotationPeriod").get<double>();
        cb->initialRotation = c.at("initialRotation").get<double>();
        cb->directRotAngle = c.at("directRotAngle").get<double>();
        cb->inverseRotation = c.at("inverseRotation").get<int>() != 0;
        const double GeeASL = c.at("GeeASL").get<double>();
        // CelestialBodyLifecycle.Awake (CelestialBody.cs Awake)
        const double Mass = cb->Radius * cb->Radius * (GeeASL * kGravitationalAcceleration) / kG;
        cb->gravParameter = Mass * kG;
        CelestialBodyRef* raw = cb.get();
        if (e->contains("orbitDriver") && e->at("orbitDriver").is_object()) {
            const auto& o = e->at("orbitDriver").at("orbit");
            Orbit& orb = sys.orbits.emplace_back();
            orb.inclination = o.at("inclination").get<double>();
            orb.eccentricity = o.at("eccentricity").get<double>();
            orb.semiMajorAxis = o.at("semiMajorAxis").get<double>();
            orb.LAN = o.at("LAN").get<double>();
            orb.argumentOfPeriapsis = o.at("argumentOfPeriapsis").get<double>();
            orb.meanAnomalyAtEpoch = o.at("meanAnomalyAtEpoch").get<double>();
            orb.epoch = o.at("epoch").get<double>();
            orb.referenceBody = sys.body.at(e->at("parent").get<std::string>()).get();
            orb.referenceBody->orbitingBodies.push_back(raw);
            raw->orbit = &orb;
        }
        extra.push_back({raw, c.at("tidallyLocked").get<int>() != 0, c.at("solarRotationPeriod").get<int>() != 0, GeeASL});
        sys.body[cb->bodyName] = std::move(cb);
    }
    // CelestialBodyLifecycle.Start: rotationPeriod for solarRotationPeriod bodies (sphereOfInfluence etc. not needed)
    for (const Extra& x : extra) {
        if (x.cb->orbit != nullptr && x.solarRotationPeriod) {
            const Orbit& orbit = *x.cb->orbit;
            const double num = x.cb->rotationPeriod;
            const double num2 = kPI * 2.0 * dotnet::Math::Sqrt(dotnet::Math::Pow(dotnet::Math::Abs(orbit.semiMajorAxis), 3.0) /
                                                               orbit.referenceBody->gravParameter);
            x.cb->rotationPeriod = num * num2 / (num2 + num);
        }
    }
    // OrbitDriver.Start: orbit.Init(); updateFromParameters -> orbit.UpdateFromUT(0)
    for (const Extra& x : extra) {
        if (x.cb->orbit != nullptr) {
            x.cb->orbit->Init();
            x.cb->orbit->UpdateFromUT(0.0);
        }
    }
    // CBUpdate (numeric part used here): tidal lock, rotPeriodRecip
    for (const Extra& x : extra) {
        CelestialBodyRef* cb = x.cb;
        if (cb->rotates && cb->rotationPeriod != 0.0 && (!x.tidallyLocked || (cb->orbit != nullptr && cb->orbit->period != 0.0))) {
            if (x.tidallyLocked) cb->rotationPeriod = cb->orbit->period;
            cb->rotPeriodRecip = 1.0 / cb->rotationPeriod;
        }
    }
}

struct Case {
    std::string Id, Body, Note;
    double Inc, E, Sma, Lan, ArgPe, MEp, Epoch;
};

// OrbitGen.cs Cases()
std::vector<Case> Cases(BodySystem& sys) {
    std::vector<Case> cs;
    auto Add = [&](std::string id, std::string body, double inc, double e, double sma, double lan, double argPe,
                   double mEp, double epoch, std::string note) {
        cs.push_back({std::move(id), std::move(body), std::move(note), inc, e, sma, lan, argPe, mEp, epoch});
    };
    struct Ep {
        double mEp, ep;
        const char* tag;
    };
    for (const Ep& x : {Ep{0.0, 0.0, "a"}, Ep{2.5, 12345.6, "b"}}) {
        const double mEp = x.mEp, ep = x.ep;
        const std::string tag = x.tag;
        Add("kerbin_leo_circ_equ_" + tag, "Kerbin", 0, 0, 680000, 0, 0, mEp, ep, "circular equatorial (e=0: argPe from an; inc=0: an=right)");
        Add("kerbin_circ_inc28_" + tag, "Kerbin", 28.5, 0, 700000, 77.7, 0, mEp, ep, "circular inclined");
        Add("kerbin_polar_" + tag, "Kerbin", 90, 0.001, 750000, 120, 45, mEp, ep, "polar, near-circular");
        Add("kerbin_retro_" + tag, "Kerbin", 135, 0.05, 1.0e6, 300, 200, mEp, ep, "retrograde");
        Add("kerbin_inc180_" + tag, "Kerbin", 180, 0.02, 9.0e5, 0, 30, mEp, ep, "inclination 180");
        Add("kerbin_gto_" + tag, "Kerbin", 5, 0.5, 6.0e6, 10, 270, mEp, ep, "elliptical, Std Kepler solver (e<0.8)");
        Add("kerbin_e079_" + tag, "Kerbin", 3, 0.79, 3.0e6, 45, 90, mEp, ep, "just below the 0.8 solver switch");
        Add("kerbin_e081_" + tag, "Kerbin", 3, 0.81, 3.0e6, 45, 90, mEp, ep, "just above: extreme-eccentricity solver (8 fixed iterations)");
        Add("kerbin_mun_transfer_" + tag, "Kerbin", 0, 0.8897637795275591, 6.35e6, 0, 0, mEp, ep, "Pe 700 km / Ap 12000 km");
        Add("kerbin_e099_" + tag, "Kerbin", 12, 0.99, 4.0e7, 200, 10, mEp, ep, "e=0.99, Ap inside SOI");
        Add("kerbin_hyp12_" + tag, "Kerbin", 10, 1.2, -4.0e6, 30, 60, mEp, ep, "hyperbolic (sma<0)");
        Add("kerbin_hyp3_" + tag, "Kerbin", 100, 3.0, -5.0e5, 250, 300, mEp, ep, "strongly hyperbolic");
        Add("mun_low_circ_" + tag, "Mun", 0, 0, 230000, 0, 0, mEp, ep, "Mun 30 km circular");
        Add("mun_incl_ell_" + tag, "Mun", 45, 0.3, 500000, 15, 75, mEp, ep, "Mun inclined elliptical");
        Add("sun_transfer_" + tag, "Sun", 1.0, 0.2, 1.7e10, 0, 0, mEp, ep, "Kerbol, Kerbin-Duna-like transfer");
        Add("sun_hyp_" + tag, "Sun", 7, 1.05, -1.0e11, 33, 44, mEp, ep, "Kerbol hyperbolic");
    }
    // deterministic pseudo-random coverage (System.Random(20261008)); C# evaluates arguments left to right and the
    // array initializer `new[] { 0.0, a, b, c }[i % 4]` draws all three values every iteration.
    dotnet::SystemRandom rnd(20261008);
    const char* bodies[] = {"Kerbin", "Mun", "Sun", "Jool", "Eve", "Minmus"};
    for (int i = 0; i < 60; i++) {
        const std::string b = bodies[i % 6];
        const CelestialBodyRef& cb = *sys.body.at(b);
        const double e1 = rnd.NextDouble() * 0.8;
        const double e2 = 0.8 + rnd.NextDouble() * 0.19;
        const double e3 = 1.0 + rnd.NextDouble() * 4;
        const double es[4] = {0.0, e1, e2, e3};
        const double e = es[i % 4];
        const double pe = cb.Radius * (1.05 + rnd.NextDouble() * 20);
        const double sma = e < 1 ? pe / (1 - e) : pe / (1 - e);  // negative for e>1
        const double inc = rnd.NextDouble() * 180;
        const double lan = rnd.NextDouble() * 360;
        const double argPe = rnd.NextDouble() * 360;
        const double mEp = (rnd.NextDouble() * 2 - 1) * kPI;
        const double epoch = dotnet::Math::Round(rnd.NextDouble() * 1e6, 3, dotnet::MidpointRounding::ToEven);
        char id[16];
        std::snprintf(id, sizeof id, "rand_%02d", i);
        Add(id, b, inc, e, sma, lan, argPe, mEp, epoch, "pseudo-random");
    }
    return cs;
}

// OrbitGen.cs Times()
std::vector<double> Times(const Orbit& o, const Case& c) {
    std::vector<double> ts;
    if (o.eccentricity < 1) {
        for (double f : {0.0, 0.1234, 0.25, 0.5, 0.75, 0.999, 1.0, 3.7, 1000.25}) ts.push_back(c.Epoch + f * o.period);
        ts.push_back(c.Epoch - 0.3 * o.period);
        ts.push_back(1.0e7);
        ts.push_back(3.0e8);
    } else {
        const double n = o.meanMotion;
        for (double k : {-5.0, -1.0, -0.1, 0.0, 0.1, 1.0, 5.0, 50.0}) ts.push_back(c.Epoch + k / n);
    }
    return ts;
}

using golden::ColType;

golden::Table& AddTable(golden::File& g, const char* name, const char* desc, std::vector<std::pair<const char*, ColType>> cols,
                        std::vector<std::string> key) {
    golden::Table t(name, desc);
    for (auto& [n, ct] : cols) t.Col(n, ct);
    t.key = std::move(key);
    g.tables.push_back(std::move(t));
    return g.tables.back();
}

}  // namespace

static int RunOrbitPropagation(const RunnerOptions& opt) {
    BodySystem sys;
    BuildBodies(opt.harness + "/golden/serialized/celestial_bodies.serialized.json", sys);
    PlanetariumFrames::Awake(0.0);  // serialized Planetarium.inverseRotAngle = 0 (scene_singletons.serialized.json)

    golden::File g;
    g.dataset = "orbit_propagation";
    g.description = "kspcore candidate for orbit_propagation (OrbitGen.cs inputs, ksp::Orbit).";
    g.tables.reserve(5);
    constexpr ColType S = ColType::Str, F = ColType::F64;
    auto& tc = AddTable(g, "cases", "Input orbital elements (Orbit constructor arguments) and Init() outputs.",
                        {{"case_id", S}, {"body", S}, {"mu", F}, {"inclination", F}, {"eccentricity", F}, {"semiMajorAxis", F},
                         {"LAN", F}, {"argumentOfPeriapsis", F}, {"meanAnomalyAtEpoch", F}, {"epoch", F}, {"meanMotion", F},
                         {"period", F}, {"ObTAtEpoch", F}, {"h_x", F}, {"h_y", F}, {"h_z", F}, {"eccVec_x", F}, {"eccVec_y", F},
                         {"eccVec_z", F}, {"an_x", F}, {"an_y", F}, {"an_z", F}, {"note", S}},
                        {"case_id"});
    auto& ts = AddTable(g, "state_at_ut", "Forward propagation at UT.",
                        {{"case_id", S}, {"UT", F}, {"ObT", F}, {"meanAnomaly", F}, {"eccentricAnomaly", F}, {"trueAnomaly", F},
                         {"pos_x", F}, {"pos_y", F}, {"pos_z", F}, {"vel_x", F}, {"vel_y", F}, {"vel_z", F}, {"sv_radius", F},
                         {"sv_pos_x", F}, {"sv_pos_y", F}, {"sv_pos_z", F}, {"sv_vel_x", F}, {"sv_vel_y", F}, {"sv_vel_z", F},
                         {"u_radius", F}, {"u_orbitalSpeed", F}, {"u_altitude", F}, {"u_orbitalEnergy", F}, {"u_timeToPe", F},
                         {"u_timeToAp", F}, {"u_orbitPercent", F}, {"GetTimeToPeriapsis_final", F},
                         {"GetTimeToPeriapsis_default", F}, {"nextPeTime", F}, {"nextApTime", F}},
                        {"case_id", "UT"});
    auto& ti = AddTable(g, "from_state_vectors",
                        "Inverse: new Orbit().UpdateFromStateVectors(getRelativePositionAtUT(UT), getOrbitalVelocityAtUT(UT), body, UT).",
                        {{"case_id", S}, {"UT", F}, {"inclination", F}, {"eccentricity", F}, {"semiMajorAxis", F}, {"LAN", F},
                         {"argumentOfPeriapsis", F}, {"meanAnomalyAtEpoch", F}, {"epoch", F}, {"trueAnomaly", F},
                         {"eccentricAnomaly", F}, {"meanAnomaly", F}, {"period", F}, {"ObT", F}, {"timeToPe", F},
                         {"timeToAp", F}, {"orbitalEnergy", F}},
                        {"case_id", "UT"});

    for (const Case& c : Cases(sys)) {
        CelestialBodyRef* body = sys.body.at(c.Body).get();
        const Orbit o(c.Inc, c.E, c.Sma, c.Lan, c.ArgPe, c.MEp, c.Epoch, body);
        tc.Add({c.Id, c.Body, body->gravParameter, c.Inc, c.E, c.Sma, c.Lan, c.ArgPe, c.MEp, c.Epoch, o.meanMotion, o.period,
                o.ObTAtEpoch, o.h.x, o.h.y, o.h.z, o.eccVec.x, o.eccVec.y, o.eccVec.z, o.an.x, o.an.y, o.an.z, c.Note});
        for (double ut : Times(o, c)) {
            Orbit fresh(c.Inc, c.E, c.Sma, c.Lan, c.ArgPe, c.MEp, c.Epoch, body);
            const double obt = fresh.getObtAtUT(ut);
            const double M = fresh.getMeanAnomalyAtUT(ut);
            const double E = fresh.EccentricAnomalyAtUT(ut);
            const double tA = fresh.TrueAnomalyAtUT(ut);
            const Vector3d p = fresh.getRelativePositionAtUT(ut);
            const Vector3d v = fresh.getOrbitalVelocityAtUT(ut);
            Vector3d sp, sv;
            const double r = fresh.GetOrbitalStateVectorsAtUT(ut, sp, sv);
            (void)fresh.GetTimeToPeriapsis();  // ttpDefault (computed, unused by the generator)
            fresh.UpdateFromUT(ut);
            const double ttpDefault2 = fresh.GetTimeToPeriapsis();
            fresh.patchEndTransition = Orbit::PatchTransitionType::FINAL;
            const double ttpFinal = fresh.GetTimeToPeriapsis();
            const double nextPe = fresh.GetNextPeriapsisTime(ut);
            const double nextAp = fresh.eccentricity < 1 ? fresh.GetNextApoapsisTime(ut) : std::numeric_limits<double>::quiet_NaN();
            ts.Add({c.Id, ut, obt, M, E, tA, p.x, p.y, p.z, v.x, v.y, v.z, r, sp.x, sp.y, sp.z, sv.x, sv.y, sv.z, fresh.radius,
                    fresh.orbitalSpeed, fresh.altitude, fresh.orbitalEnergy, fresh.timeToPe, fresh.timeToAp, fresh.orbitPercent,
                    ttpFinal, ttpDefault2, nextPe, nextAp});
            Orbit inv;
            inv.UpdateFromStateVectors(p, v, body, ut);
            ti.Add({c.Id, ut, inv.inclination, inv.eccentricity, inv.semiMajorAxis, inv.LAN, inv.argumentOfPeriapsis,
                    inv.meanAnomalyAtEpoch, inv.epoch, inv.trueAnomaly, inv.eccentricAnomaly, inv.meanAnomaly, inv.period, inv.ObT,
                    inv.timeToPe, inv.timeToAp, inv.orbitalEnergy});
        }
    }

    // hand-made state vectors, including degenerate ones
    auto& tsv = AddTable(g, "state_vector_cases",
                         "UpdateFromStateVectors on hand-made vectors (zup local frame), incl. degenerate cases.",
                         {{"case_id", S}, {"body", S}, {"UT", F}, {"pos_x", F}, {"pos_y", F}, {"pos_z", F}, {"vel_x", F},
                          {"vel_y", F}, {"vel_z", F}, {"inclination", F}, {"eccentricity", F}, {"semiMajorAxis", F}, {"LAN", F},
                          {"argumentOfPeriapsis", F}, {"meanAnomalyAtEpoch", F}, {"trueAnomaly", F}, {"period", F},
                          {"timeToPe", F}, {"timeToAp", F}},
                         {"case_id"});
    CelestialBodyRef* kerbin = sys.body.at("Kerbin").get();
    const double vc = dotnet::Math::Sqrt(kerbin->gravParameter / 700000.0);
    struct SvCase {
        const char* id;
        Vector3d p, v;
    };
    const SvCase svCases[] = {
        {"circular_x", {700000, 0, 0}, {0, vc, 0}},
        {"circular_y_retro", {0, 700000, 0}, {vc, 0, 0}},
        {"circular_polar", {700000, 0, 0}, {0, 0, vc}},
        {"elliptic_pe", {700000, 0, 0}, {0, vc * 1.2, 0}},
        {"elliptic_inclined", {500000, 400000, 100000}, {-1500, 1800, 400}},
        {"escape_exact", {700000, 0, 0}, {0, vc * dotnet::Math::Sqrt(2.0), 0}},
        {"hyperbolic", {700000, 0, 0}, {0, vc * 1.6, 100}},
        {"radial_only", {700000, 0, 0}, {1000, 0, 0}},
        {"zero_velocity", {700000, 0, 0}, {0, 0, 0}},
    };
    for (const SvCase& sc : svCases) {
        Orbit o;
        o.UpdateFromStateVectors(sc.p, sc.v, kerbin, 1000.0);
        tsv.Add({sc.id, "Kerbin", 1000.0, sc.p.x, sc.p.y, sc.p.z, sc.v.x, sc.v.y, sc.v.z, o.inclination, o.eccentricity,
                 o.semiMajorAxis, o.LAN, o.argumentOfPeriapsis, o.meanAnomalyAtEpoch, o.trueAnomaly, o.period, o.timeToPe,
                 o.timeToAp});
    }

    // Kepler equation solver grid
    auto& tk = AddTable(g, "kepler",
                        "Orbit.solveEccentricAnomaly(M, e) with Orbit.eccentricity = e (the solvers read the field), and anomaly conversions.",
                        {{"e", F}, {"M", F}, {"E", F}, {"trueAnomaly_of_E", F}, {"E_of_trueAnomaly", F}, {"M_of_E", F}},
                        {"e", "M"});
    for (double e : {0.0, 1e-6, 0.01, 0.3, 0.7, 0.79999, 0.8, 0.85, 0.9, 0.99, 0.999, 0.9999, 1.0001, 1.01, 1.5, 5.0, 50.0}) {
        const Orbit o(0, e, e < 1 ? 1.0e6 : -1.0e6, 0, 0, 0, 0, kerbin);
        const std::vector<double> Ms =
            e < 1 ? std::vector<double>{-3.0, -1e-9, 0.0, 1e-12, 1e-6, 0.001, 0.1, 0.5, 1.0, 2.0, kPI - 1e-9, kPI, kPI + 1e-9, 4.0,
                                        2 * kPI - 1e-12, 2 * kPI, 7.0, 100.0}
                  : std::vector<double>{-100.0, -10.0, -1.0, -1e-6, 0.0, 1e-9, 1e-3, 0.5, 1.0, 3.0, 10.0, 50.0, 1000.0};
        for (double M : Ms) {
            const double E = o.solveEccentricAnomaly(M, e);
            const double tA = o.GetTrueAnomaly(E);
            tk.Add({e, M, E, tA, o.GetEccentricAnomaly(tA), o.GetMeanAnomaly(E)});
        }
    }
    return WriteCandidate(opt, g, "orbit_propagation/orbit_propagation.json");
}

KSP_GOLDEN_DATASET("orbit_propagation", 70, RunOrbitPropagation);
