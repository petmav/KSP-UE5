// orbit_rails candidate (WP-08, gated against the in-game capture C8, harness/captures/out2/orbit_rails_ingame.json):
// on-rails propagation through the game loop with moving bodies, the rotating Planetarium frame, time warp from 1x to
// 100,000x and on-rails SOI transitions (OrbitDriver.UpdateOrbit -> updateFromParameters -> CheckDominantBody ->
// RecalculateOrbit -> OnRailsSOITransition). Mirrors the capture procedure in
// decompiled/KSP-Decompiled-profiling/Assembly-CSharp/PhysProf/HarnessCapture2Flight.cs:444-625 (C8()).
//
// Inputs (read from the capture file; none of them is a gated output):
//   * provenance.ut0: the in-game UT when the probes were spawned (game state after earlier capture items)
//   * initial.req_* (exact bits from the _hex columns) and initial.body: the requested probe orbits (the capture code's
//     literal list, plus the Mun-encounter orbit it constructed in game from the Mun's position)
//   (segments.requested_rate is no longer an input since WP-10: it is TimeWarp.warpRates[rate_index] from the game's
//   TimeWarp prefab data in kspcore, and gated)
//   * Kerbin.directRotAngle: hidden game state (set when the flight scene switched Kerbin to the rotating frame, long
//     before the capture). Recovered from the first frames row: (rotationAngle(UT) - inverseRotAngle) % 360; the runner
//     checks that this reproduces that row's inverseRotAngle bit for bit, every other row is a prediction.
// Scene state the runner sets (the flight scene the capture ran in):
//   * PSystemSetup.cs:978-990 (flight at the KSC): every body inverseRotation = false and orbitDriver.reverse = false,
//     then Kerbin inverseRotation = true, reverse = true (the stock prefab's Mun/Minmus inverseRotation is cleared).
//   * Planetarium time = ut0, fixedDeltaTime 0.02 (serialized), TimeWarp HIGH mode; the active vessel is landed on
//     Kerbin, so FlightGlobals.RefFrameIsRotating = Kerbin.rotates && Kerbin.inverseRotation.
//   * Body positions are whatever kspcore's moving bodies give (Kerbin, the reversed root, stays where it is; the game
//     keeps it near the floating origin). Only CheckDominantBody and the Kerbin->Mun BSP in OnRailsSOITransition read
//     world positions, through differences in which Kerbin's position cancels.
// Frame loop per physics frame: Planetarium.FixedUpdate (time += fixedDeltaTime * timeScale; CBUpdate pass), then
// every probe's OrbitDriver.UpdateOrbit (VesselPrecalculate.MainPhysics, UPDATE mode), then a stand-in for
// Vessel.CheckKill (Vessel.cs:2746, WP-14) on the unloaded probe (destroyed below altitude -0.1 m or above 1 kPa).
// Probes: ProtoVessel.CreateVesselNode's ORBIT node (ProtoVessel.cs:968, G17 text) -> OrbitSnapshot.Load ->
// OrbitDriver in UPDATE mode (ProtoVessel.cs:713-714) -> OrbitDriver.Start.
// Not reproduced (excluded in tests/CMakeLists.txt): world_rel_* (vessel.GetWorldPos3D() - mainBody.position: floating
// origin offsets and the vessel's transform / localCoM, WP-14/WP-20), packed / loaded (Vessel state, WP-14/WP-19),
// initial.note (free text) and the echoed inputs req_*; soi_events of the two asteroids (spawned by the
// game's discovery scenario, not part of the capture's inputs).
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "celestial_env.hpp"
#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_body_orbit.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/orbit/orbit_snapshot.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/save/save_enums.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using golden::ColType;
using json = nlohmann::json;

namespace {

// The active vessel of the capture run (PhysProf-S landed on the KSC pad), seen through the members TimeWarp reads
// (IFlightVessel). Stand-in for WP-14's Vessel: landed, no surface speed relative to the rotating frame, no
// patched-conic solver attached (a landed vessel's orbit is not part of the experiment; the C8 capture's achieved
// rates equal the requested ones for every segment, so no on-rails limit applied).
class PadVessel : public IFlightVessel {
public:
    explicit PadVessel(CelestialBody* body) : body_(body) {}
    bool LandedOrSplashed() const override { return true; }
    CelestialBody* mainBody() const override { return body_; }
    Vector3f TransformPosition() const override { return {static_cast<float>(body_->Radius), 0.0f, 0.0f}; }
    Orbit* orbit() const override { return nullptr; }
    double altitude() const override { return 0.0; }
    double geeForce() const override { return 1.0; }
    Vector3d srf_velocity() const override { return {0.0, 0.0, 0.0}; }
    bool PatchedConicsAttached() const override { return false; }
    bool isEVA() const override { return false; }
    bool OnALadder() const override { return false; }

private:
    CelestialBody* body_;
};

double HexF64(const std::string& h) {
    const uint64_t u = std::stoull(h, nullptr, 16);
    double d;
    std::memcpy(&d, &u, sizeof d);
    return d;
}

struct CapTable {
    const json* t = nullptr;
    int Col(const std::string& name) const {
        const json& cols = (*t)["columns"];
        for (size_t i = 0; i < cols.size(); ++i)
            if (cols[i]["name"] == name) return static_cast<int>(i);
        throw std::runtime_error("orbit_rails: capture column missing: " + name);
    }
    const json& Rows() const { return (*t)["rows"]; }
};

CapTable Table(const json& doc, const std::string& name) {
    for (const json& t : doc["tables"])
        if (t["name"] == name) return CapTable{&t};
    throw std::runtime_error("orbit_rails: capture table missing: " + name);
}

struct CaseInput {
    std::string id, body;
    double inc, e, sma, lan, argPe, mEp, epoch;
};

// A probe: an unloaded, packed vessel on rails (one probeCoreOcto) as far as its OrbitDriver sees it.
class Probe final : public IOrbitDriverVessel {
public:
    std::string id;
    std::string vesselName;
    OrbitDriver driver;
    bool dead = false;
    Vector3d worldPos;
    CelestialBody* lastRef = nullptr;

    Vessel* AsVessel() override { return nullptr; }
    std::string GetDisplayName() const override { return vesselName; }
    Vector3d RotatedLocalCoM() const override { return Vector3d::zero(); }  // vessel transform (WP-14): not modelled
    void SetPosition(const Vector3d& position) override { worldPos = position; }
    void UnloadAndDestroy() override { dead = true; }
    Vector3f TransformPosition() const override { return worldPos.ToVector3f(); }
    bool HasRootPartRigidbody() const override { return false; }  // packed: only UPDATE mode is exercised
    bool RootPartRigidbodyIsKinematic() const override { return true; }
    Vector3d CoMD() const override { return worldPos; }
    Vector3d velocityD() const override { return Vector3d::zero(); }
    void ForceVesselLogFlyby(CelestialBody*) override {}
};

// Stand-in for Vessel.CheckKill (Vessel.cs:2746) for a packed, unloaded probe that is not the active vessel: destroyed
// above 1 kPa static pressure, or below altitude -0.1 m (terrainAltitude unknown for an unloaded vessel; the capture's
// Mun-encounter probe dives to 7.9 km from the Mun's centre).
void CheckKillStandIn(Probe& p) {
    const Orbit& o = *p.driver.orbit;
    const CelestialBody& body = *o.referenceBody;
    const double altitude = o.pos.magnitude() - body.Radius;
    if (body.GetPressure(altitude) > 1.0) p.dead = true;
    else if (altitude < -0.1) p.dead = true;
}

golden::Table& AddTable(golden::File& g, const char* name, std::vector<std::pair<const char*, ColType>> cols,
                        std::vector<std::string> key) {
    golden::Table t(name, "kspcore candidate");
    for (auto& [n, ct] : cols) t.Col(n, ct);
    t.key = std::move(key);
    g.tables.push_back(std::move(t));
    return g.tables.back();
}

double CsRem(double a, double b) { return std::fmod(a, b); }

}  // namespace

static int RunOrbitRails(const RunnerOptions& opt) {
    // ---- capture inputs ----
    const std::string capPath = opt.harness + "/captures/out2/orbit_rails_ingame.json";
    std::ifstream in(capPath, std::ios::binary);
    if (!in) throw std::runtime_error("orbit_rails: cannot open " + capPath);
    const json doc = json::parse(in);
    const double ut0 = doc["provenance"]["ut0"].get<double>();
    std::vector<CaseInput> cases;
    {
        const CapTable ti = Table(doc, "initial");
        const int cId = ti.Col("id"), cBody = ti.Col("body");
        const int cH[7] = {ti.Col("req_inc_hex"), ti.Col("req_e_hex"), ti.Col("req_sma_hex"), ti.Col("req_LAN_hex"),
                           ti.Col("req_argPe_hex"), ti.Col("req_mEp_hex"), ti.Col("req_epoch_hex")};
        for (const json& r : ti.Rows()) {
            CaseInput c;
            c.id = r[cId].get<std::string>();
            c.body = r[cBody].get<std::string>();
            double v[7];
            for (int k = 0; k < 7; ++k) v[k] = HexF64(r[cH[k]].get<std::string>());
            c.inc = v[0], c.e = v[1], c.sma = v[2], c.lan = v[3], c.argPe = v[4], c.mEp = v[5], c.epoch = v[6];
            cases.push_back(c);
        }
        if (cases.empty() || std::memcmp(&cases[0].epoch, &ut0, sizeof ut0) != 0)
            throw std::runtime_error("orbit_rails: provenance ut0 differs from the first case's requested epoch");
    }
    double frame0UT = 0.0, frame0IRA = 0.0;
    {
        const CapTable tf = Table(doc, "frames");
        const json& r0 = tf.Rows().at(0);
        frame0UT = HexF64(r0[tf.Col("UT_hex")].get<std::string>());
        frame0IRA = HexF64(r0[tf.Col("inverseRotAngle_hex")].get<std::string>());
    }

    // ---- environment: the game's start-up system (CelestialEnv = KspEnv), then the flight scene ----
    CelestialEnv& env = CelestialEnv::Get(opt);
    Planetarium& planetarium = *env.planetarium;
    const CelestialSystem& sys = *env.system;
    CelestialBody* kerbin = sys.GetBodyByName("Kerbin");
    CelestialBody* mun = sys.GetBodyByName("Mun");
    CelestialBody* minmus = sys.GetBodyByName("Minmus");
    for (CelestialBody* b : sys.Bodies()) {  // PSystemSetup.cs:978-990
        b->inverseRotation = false;
        if (b->orbitDriver) b->orbitDriver->reverse = false;
    }
    kerbin->inverseRotation = true;
    kerbin->orbitDriver->reverse = true;
    // Kerbin.directRotAngle (hidden state): rotationAngle at the first sampled frame minus the captured inverseRotAngle
    {
        const double ra = CsRem(kerbin->initialRotation + 360.0 * kerbin->rotPeriodRecip * frame0UT, 360.0);
        kerbin->directRotAngle = CsRem(ra - frame0IRA, 360.0);
        const double back = CsRem(ra - kerbin->directRotAngle, 360.0);
        if (std::memcmp(&back, &frame0IRA, sizeof back) != 0)
            throw std::runtime_error("orbit_rails: Kerbin.directRotAngle does not reproduce the first frame's inverseRotAngle");
    }
    // The flight scene's TimeWarp: the game's prefab data (TimeWarp::LoadStockPrefab), Start() as Unity ran it when the
    // scene loaded (SetRate(0) -> tgt_rate = warpRates[0], curr_rate = 1, timeScale 1, fixedDeltaTime 0.02).
    TimeWarpEnvironment twEnv;
    twEnv.LoadedSceneValue = GameScenes::FLIGHT;
    twEnv.flightStarted = true;
    TimeWarp::environment = &twEnv;
    PadVessel padVessel(kerbin);
    FlightGlobals::ready = true;
    FlightGlobals::ActiveVesselView = &padVessel;
    FlightGlobals::VesselsLoadedView = {&padVessel};
    FlightGlobals::currentMainBody = kerbin;
    TimeWarp timeWarp;
    timeWarp.LoadStockPrefab();
    timeWarp.Awake();
    Time::timeScale = 1.0f;
    Time::fixedDeltaTime = 0.02f;
    Time::time = 0.0f;
    planetarium.fixedDeltaTime = 0.02;
    planetarium.timeScale = 1.0;
    planetarium.time = ut0;
    timeWarp.Start();
    planetarium.UpdateCBs();  // the frame in which the capture read ut0: bodies and Zup at ut0

    // ---- probes (spawned in the ut0 frame) ----
    std::vector<std::unique_ptr<Probe>> probes;
    struct SoiEvent {
        std::string vessel, from, to;
        double UT, sma, e, inc, lan, argPe, mEp, epoch;
        float rate;
    };
    std::vector<SoiEvent> events;
    for (const CaseInput& c : cases) {
        CelestialBody* body = sys.GetBodyByName(c.body);
        const Orbit o(c.inc, c.e, c.sma, c.lan, c.argPe, c.mEp, c.epoch, body);
        ConfigNode orbitNode("ORBIT");
        OrbitSnapshot(o, sys.Bodies()).Save(orbitNode);  // ProtoVessel.CreateOrbitNode (G17 text)
        auto p = std::make_unique<Probe>();
        p->id = c.id;
        p->vesselName = "HC8 " + c.id;
        p->driver.vessel = p.get();
        p->driver.name = p->vesselName;
        p->driver.orbit = std::make_shared<Orbit>(OrbitSnapshot(orbitNode).Load(sys.Bodies()));  // ProtoVessel.cs:713
        p->driver.updateMode = OrbitDriver::UpdateMode::UPDATE;                                // ProtoVessel.cs:714
        p->driver.Start();
        Probe* raw = p.get();
        p->driver.OnReferenceBodyChange += OrbitDriver::CelestialBodyDelegate::Lambda(raw, [raw, &events](CelestialBody* to) {
            // GameEvents.onVesselSOIChanged as the capture's handler records it (HarnessCapture2Flight.cs:434)
            const Orbit& ob = *raw->driver.orbit;
            events.push_back({raw->vesselName, raw->lastRef != nullptr ? raw->lastRef->bodyName : "", to->bodyName,
                              Planetarium::GetUniversalTime(), ob.semiMajorAxis, ob.eccentricity, ob.inclination, ob.LAN,
                              ob.argumentOfPeriapsis, ob.meanAnomalyAtEpoch, ob.epoch, TimeWarp::CurrentRate()});
        });
        probes.push_back(std::move(p));
    }

    auto fixedFrame = [&]() {
        timeWarp.FixedUpdate();  // TimeWarp.FixedUpdate (rate lerp; a no-op once SetRate(instant) has been applied)
        planetarium.FixedUpdate();
        for (auto& p : probes) {
            if (p->dead) continue;
            p->lastRef = p->driver.referenceBody();
            p->driver.UpdateOrbit();
            CheckKillStandIn(*p);
        }
        Time::time = Time::time + Time::fixedDeltaTime;
        for (auto& p : probes) p->driver.RunDelayedCalls();
    };

    // ---- candidate tables ----
    golden::File g;
    g.dataset = "orbit_rails_ingame";
    g.description = "kspcore candidate for the C8 capture (on-rails propagation, rotating frame, time warp, SOI transitions).";
    g.tables.reserve(5);
    constexpr ColType S = ColType::Str, F = ColType::F64, I = ColType::I64, B = ColType::Bool, F32 = ColType::F32;
    auto& ti = AddTable(g, "initial",
                        {{"id", S}, {"body", S}, {"req_inc", F}, {"req_e", F}, {"req_sma", F}, {"req_LAN", F}, {"req_argPe", F},
                         {"req_mEp", F}, {"req_epoch", F}, {"inclination", F}, {"eccentricity", F}, {"semiMajorAxis", F},
                         {"LAN", F}, {"argumentOfPeriapsis", F}, {"meanAnomalyAtEpoch", F}, {"epoch", F}, {"meanMotion", F},
                         {"period", F}, {"ObTAtEpoch", F}, {"mu", F}, {"spawned", B}},
                        {"id"});
    auto& ts = AddTable(g, "samples",
                        {{"id", S}, {"frame", I}, {"segment", I}, {"UT", F}, {"warp_rate", F32}, {"ref_body", S}, {"updateUT", F},
                         {"pos_x", F}, {"pos_y", F}, {"pos_z", F}, {"vel_x", F}, {"vel_y", F}, {"vel_z", F},
                         {"relpos_at_UT_x", F}, {"relpos_at_UT_y", F}, {"relpos_at_UT_z", F}, {"semiMajorAxis", F},
                         {"eccentricity", F}, {"inclination", F}, {"LAN", F}, {"argumentOfPeriapsis", F},
                         {"meanAnomalyAtEpoch", F}, {"epoch", F}, {"ObT", F}, {"meanAnomaly", F}, {"trueAnomaly", F},
                         {"update_mode", S}},
                        {"id", "frame"});
    auto& tfr = AddTable(g, "frames",
                         {{"frame", I}, {"UT", F}, {"Zup_X_x", F}, {"Zup_X_y", F}, {"Zup_X_z", F}, {"Zup_Y_x", F}, {"Zup_Y_y", F},
                          {"Zup_Y_z", F}, {"Zup_Z_x", F}, {"Zup_Z_y", F}, {"Zup_Z_z", F}, {"inverseRotAngle", F},
                          {"frame_is_rotating", B}},
                         {"frame"});
    auto& tseg = AddTable(g, "segments",
                          {{"segment", I}, {"rate_index", I}, {"requested_rate", F32}, {"achieved_rate", F32}, {"frames", I},
                           {"UT_start", F}, {"UT_end", F}, {"Time_fixedDeltaTime", F32}, {"TimeWarp_fixedDeltaTime", F32},
                           {"timeScale", F32}},
                          {"segment"});
    auto& tev = AddTable(g, "soi_events",
                         {{"vessel", S}, {"UT", F}, {"from", S}, {"to", S}, {"semiMajorAxis", F}, {"eccentricity", F},
                          {"inclination", F}, {"LAN", F}, {"argumentOfPeriapsis", F}, {"meanAnomalyAtEpoch", F}, {"epoch", F},
                          {"warp_rate", F32}},
                         {"vessel", "UT"});

    // for (int i = 0; i < 5; i++) yield return new WaitForFixedUpdate();
    for (int i = 0; i < 5; ++i) fixedFrame();
    for (size_t k = 0; k < cases.size(); ++k) {
        const CaseInput& c = cases[k];
        const Probe& p = *probes[k];
        const Orbit& o2 = *p.driver.orbit;
        ti.Add({c.id, o2.referenceBody->bodyName, c.inc, c.e, c.sma, c.lan, c.argPe, c.mEp, c.epoch, o2.inclination,
                o2.eccentricity, o2.semiMajorAxis, o2.LAN, o2.argumentOfPeriapsis, o2.meanAnomalyAtEpoch, o2.epoch,
                o2.meanMotion, o2.period, o2.ObTAtEpoch, o2.referenceBody->gravParameter, true});
    }

    const int rateIdx[5] = {0, 4, 5, 6, 7};
    const int segFrames[5] = {500, 1000, 1000, 1500, 500};
    int frame = 0;
    for (int s = 0; s < 5; ++s) {
        // TimeWarp.SetRate(rateIdx[s], instant: true) (HighLogic.CurrentGame: flight scene, HIGH mode): clamps the index,
        // setRate -> getMaxOnRailsRateIdx (the landed pad vessel does not limit it) -> assumeWarpRate(instant) -> updateRate
        TimeWarp::SetRate(rateIdx[s], true);
        fixedFrame();
        ++frame;
        const double utStart = Planetarium::GetUniversalTime();
        const float achieved = TimeWarp::CurrentRate();
        for (int f = 0; f < segFrames[s]; ++f) {
            fixedFrame();
            ++frame;
            if (frame % 10 != 0) continue;
            const double ut = Planetarium::GetUniversalTime();
            const float rate = TimeWarp::CurrentRate();
            const CelestialFrame& z = Planetarium::Zup;
            tfr.Add({frame, ut, z.X.x, z.X.y, z.X.z, z.Y.x, z.Y.y, z.Y.z, z.Z.x, z.Z.y, z.Z.z, Planetarium::InverseRotAngle(),
                     kerbin->rotates && kerbin->inverseRotation});
            auto sample = [&](const std::string& id, const Orbit& o, double updateUT, OrbitDriver::UpdateMode mode) {
                const Vector3d rel = o.getRelativePositionAtUT(ut);
                ts.Add({id, frame, s, ut, rate, o.referenceBody->bodyName, updateUT, o.pos.x, o.pos.y, o.pos.z, o.vel.x, o.vel.y,
                        o.vel.z, rel.x, rel.y, rel.z, o.semiMajorAxis, o.eccentricity, o.inclination, o.LAN,
                        o.argumentOfPeriapsis, o.meanAnomalyAtEpoch, o.epoch, o.ObT, o.meanAnomaly, o.trueAnomaly,
                        ToString(mode)});
            };
            for (const auto& p : probes)
                if (!p->dead) sample(p->id, *p->driver.orbit, p->driver.UpdateUT(), p->driver.updateMode);
            for (CelestialBody* b : {mun, minmus}) {
                const auto* ad = dynamic_cast<const CelestialBodyOrbitAdapter*>(b->orbit());
                if (ad == nullptr) throw std::runtime_error("orbit_rails: body without a run-time OrbitDriver");
                sample("body:" + b->bodyName, ad->orbit, ad->driver.UpdateUT(), ad->driver.updateMode);
            }
        }
        tseg.Add({s, rateIdx[s], timeWarp.warpRates.at(static_cast<size_t>(rateIdx[s])), achieved, segFrames[s], utStart, Planetarium::GetUniversalTime(),
                  Time::fixedDeltaTime, TimeWarp::fixedDeltaTime(), Time::timeScale});
    }
    for (const SoiEvent& e : events)
        tev.Add({e.vessel, e.UT, e.from, e.to, e.sma, e.e, e.inc, e.lan, e.argPe, e.mEp, e.epoch, e.rate});

    TimeWarp::fetch = nullptr;
    TimeWarp::environment = nullptr;
    FlightGlobals::ready = false;
    FlightGlobals::ActiveVesselView = nullptr;
    FlightGlobals::VesselsLoadedView.clear();
    FlightGlobals::currentMainBody = nullptr;
    probes.clear();  // OrbitDriver.OnDestroy: out of Planetarium.Orbits
    return WriteCandidate(opt, g, "orbit_rails/orbit_rails.json");
}

KSP_GOLDEN_DATASET("orbit_rails", 90, RunOrbitRails);
