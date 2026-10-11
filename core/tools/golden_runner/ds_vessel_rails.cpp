// vessel_rails candidate (WP-19a, gated against the in-game capture C8, harness/captures/out2/orbit_rails_ingame.json): the same experiment as
// orbit_rails (ds_orbit_rails.cpp, WP-08/WP-10: on-rails propagation through the game loop with moving bodies, the rotating Planetarium frame,
// time warp from 1x to 100,000x and on-rails SOI transitions) but with REAL ksp::Vessel objects in the WP-19a registry instead of the OrbitDriver
// stand-ins: every probe is an unloaded, packed Vessel with an OrbitDriver in UPDATE mode (ProtoVessel.cs:713-714), registered in
// FlightGlobals; each physics frame runs the scripts in the measured Unity order (flight_scene.hpp):
//   Planetarium.FixedUpdate, VesselPrecalculate.FixedUpdate of every vessel (MainPhysics: OrbitDriver.UpdateOrbit in UPDATE mode,
//   CalculatePhysicsStats, UpdatePosVel, UpdateAcceleration, Vessel.CheckKill), OrbitDriver.FixedUpdate, FlightGlobals.FixedUpdate,
//   TimeWarp.FixedUpdate, Vessel.FixedUpdate.
// SOI changes are recorded from GameEvents.onVesselSOIChanged like the capture's handler (HarnessCapture2Flight.cs:434); the probe's death is the
// real Vessel.CheckKill (WP-19a), not a stand-in. The active vessel is a landed Vessel on Kerbin (the capture's PhysProf-S on the pad).
//
// What this adds to orbit_rails: `packed` and `loaded` (Vessel state), `world_rel_*` (vessel.GetWorldPos3D() - mainBody.position, reported
// in Unity world axes) are written for comparison. Inputs (read from the capture, none of them a gated output): provenance.ut0, initial.req_*,
// initial.body, Kerbin.directRotAngle (recovered from the first frames row, checked bit for bit against that row), as in orbit_rails.
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
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/orbit/orbit_snapshot.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/flight_scene.hpp"
#include "ksp/vessel/vessel.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using golden::ColType;
using json = nlohmann::json;
using GE = events::GameEvents;

namespace {

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
        throw std::runtime_error("vessel_rails: capture column missing: " + name);
    }
    const json& Rows() const { return (*t)["rows"]; }
};

CapTable Table(const json& doc, const std::string& name) {
    for (const json& t : doc["tables"])
        if (t["name"] == name) return CapTable{&t};
    throw std::runtime_error("vessel_rails: capture table missing: " + name);
}

struct CaseInput {
    std::string id, body;
    double inc, e, sma, lan, argPe, mEp, epoch;
};

golden::Table& AddTable(golden::File& g, const char* name, std::vector<std::pair<const char*, ColType>> cols, std::vector<std::string> key) {
    golden::Table t(name, "kspcore candidate");
    for (auto& [n, ct] : cols) t.Col(n, ct);
    t.key = std::move(key);
    g.tables.push_back(std::move(t));
    return g.tables.back();
}

double CsRem(double a, double b) { return std::fmod(a, b); }

// A vessel with no parts: the probe (an unloaded, packed Vessel; its ProtoVessel exists, as ProtoVessel.Load gives every spawned vessel one)
struct Probe {
    std::string id;
    std::unique_ptr<Vessel> v;
    CelestialBody* lastRef = nullptr;
};

}  // namespace

static int RunVesselRails(const RunnerOptions& opt) {
    // ---- capture inputs (as ds_orbit_rails.cpp)
    const std::string capPath = opt.harness + "/captures/out2/orbit_rails_ingame.json";
    std::ifstream in(capPath, std::ios::binary);
    if (!in) throw std::runtime_error("vessel_rails: cannot open " + capPath);
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
            throw std::runtime_error("vessel_rails: provenance ut0 differs from the first case's requested epoch");
    }
    double frame0UT = 0.0, frame0IRA = 0.0;
    {
        const CapTable tf = Table(doc, "frames");
        const json& r0 = tf.Rows().at(0);
        frame0UT = HexF64(r0[tf.Col("UT_hex")].get<std::string>());
        frame0IRA = HexF64(r0[tf.Col("inverseRotAngle_hex")].get<std::string>());
    }

    // ---- environment: the game's start-up system, then the flight scene (PSystemSetup.cs:978-990)
    CelestialEnv& env = CelestialEnv::Get(opt);
    Planetarium& planetarium = *env.planetarium;
    const CelestialSystem& sys = *env.system;
    CelestialBody* kerbin = sys.GetBodyByName("Kerbin");
    CelestialBody* mun = sys.GetBodyByName("Mun");
    CelestialBody* minmus = sys.GetBodyByName("Minmus");
    for (CelestialBody* b : sys.Bodies()) {
        b->inverseRotation = false;
        if (b->orbitDriver) b->orbitDriver->reverse = false;
    }
    kerbin->inverseRotation = true;
    kerbin->orbitDriver->reverse = true;
    {
        const double ra = CsRem(kerbin->initialRotation + 360.0 * kerbin->rotPeriodRecip * frame0UT, 360.0);
        kerbin->directRotAngle = CsRem(ra - frame0IRA, 360.0);
        const double back = CsRem(ra - kerbin->directRotAngle, 360.0);
        if (std::memcmp(&back, &frame0IRA, sizeof back) != 0)
            throw std::runtime_error("vessel_rails: Kerbin.directRotAngle does not reproduce the first frame's inverseRotAngle");
    }
    FlightGlobals::Clear();
    SaveEnvironment::Reset();
    Vessel::env = VesselEnvironment{};
    FlightGlobals::bodies = &sys.Bodies();
    TimeWarpEnvironment twEnv;
    twEnv.LoadedSceneValue = GameScenes::FLIGHT;
    twEnv.flightStarted = true;
    TimeWarp::environment = &twEnv;
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

    // The active vessel: PhysProf-S landed on the KSC pad (loaded, unpacked, Landed). It is the active vessel and not part of the experiment;
    // it is not registered in the vessel list, so the scene tick below does not run its (physics-less) VesselPrecalculate.
    Vessel pad;
    pad.vesselName = "PhysProf-S";
    pad.orbitDriver = std::make_unique<OrbitDriver>();
    pad.orbitDriver->vessel = &pad.OrbitDriverView();
    pad.orbitDriver->orbit = std::make_shared<Orbit>(0.0, 0.0, kerbin->Radius + 70.0, 0.0, 0.0, 0.0, ut0, kerbin);
    pad.orbitDriver->updateMode = OrbitDriver::UpdateMode::IDLE;
    pad.orbitDriver->Start();
    pad.loaded = true;
    pad.packed = false;
    pad.Landed = true;
    pad.altitude = 0.0;
    pad.geeForce = 1.0;
    pad.vesselType = VesselType::Ship;
    FlightGlobals::ready = true;
    FlightGlobals::ActiveVessel = &pad;
    FlightGlobals::ActiveVesselView = &pad.FlightView();
    FlightGlobals::VesselsLoadedView = {&pad.FlightView()};

    // ---- probes (spawned in the ut0 frame): ProtoVessel.CreateVesselNode's ORBIT node -> OrbitSnapshot.Load -> OrbitDriver in UPDATE mode
    std::vector<Probe> probes;
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
        Probe p;
        p.id = c.id;
        p.v = std::make_unique<Vessel>();
        Vessel& v = *p.v;
        v.vesselName = "HC8 " + c.id;
        v.persistentId = static_cast<uint32_t>(2000 + probes.size());
        v.vesselType = VesselType::Probe;
        v.orbitDriver = std::make_unique<OrbitDriver>();
        v.orbitDriver->vessel = &v.OrbitDriverView();
        v.orbitDriver->name = v.vesselName;
        v.orbitDriver->orbit = std::make_shared<Orbit>(OrbitSnapshot(orbitNode).Load(sys.Bodies()));  // ProtoVessel.cs:713
        v.orbitDriver->updateMode = OrbitDriver::UpdateMode::UPDATE;                                // ProtoVessel.cs:714
        v.orbitDriver->Start();
        v.loaded = true;  // an unloaded vessel keeps the ProtoVessel that Unload() makes (ProtoVessel.Load gives every spawned vessel one)
        v.Unload();       // (GoOnRails first: the vessel is packed)
        v.packed = true;
        v.lastBody = v.orbit().referenceBody;  // Vessel.Start (Vessel.cs:1597)
        FlightGlobals::AddVessel(&v);
        probes.push_back(std::move(p));
    }
    // GameEvents.onVesselSOIChanged as the capture's handler records it (HarnessCapture2Flight.cs:434)
    auto onSoi = [&](GE::HostedFromToAction<Vessel*, CelestialBody*> d) {
        for (Probe& p : probes) {
            if (p.v.get() != d.host) continue;
            const Orbit& ob = d.host->orbit();
            events.push_back({d.host->vesselName, p.lastRef != nullptr ? p.lastRef->bodyName : "", d.to->bodyName, Planetarium::GetUniversalTime(),
                              ob.semiMajorAxis, ob.eccentricity, ob.inclination, ob.LAN, ob.argumentOfPeriapsis, ob.meanAnomalyAtEpoch, ob.epoch,
                              TimeWarp::CurrentRate()});
        }
    };
    const auto soiHandler = decltype(GE::onVesselSOIChanged)::OnEvent::Lambda(&probes, onSoi);
    GE::onVesselSOIChanged.Add(soiHandler);

    auto fixedFrame = [&]() {
        planetarium.FixedUpdate();  // Planetarium.FixedUpdate (measured seq 6)
        for (Probe& p : probes) p.lastRef = p.v->orbitDriver->referenceBody();
        FlightScene::FixedUpdate();  // VesselPrecalculate / OrbitDriver / FlightGlobals / Vessel (seq 7-281)
        timeWarp.FixedUpdate();      // TimeWarp.FixedUpdate (seq 19; a no-op once SetRate(instant) has been applied)
        Time::time = Time::time + Time::fixedDeltaTime;
        for (Probe& p : probes)
            if (p.v->orbitDriver) p.v->orbitDriver->RunDelayedCalls();
    };

    // ---- candidate tables
    golden::File g;
    g.dataset = "orbit_rails_ingame";
    g.description = "kspcore candidate for the C8 capture through real Vessels (WP-19a): on-rails propagation, rotating frame, time warp, SOI transitions.";
    g.tables.reserve(5);  // initial, samples, frames, segments, soi_events (references are kept)
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
                         {"relpos_at_UT_x", F}, {"relpos_at_UT_y", F}, {"relpos_at_UT_z", F},
                         {"world_rel_x", F}, {"world_rel_y", F}, {"world_rel_z", F}, {"semiMajorAxis", F},
                         {"eccentricity", F}, {"inclination", F}, {"LAN", F}, {"argumentOfPeriapsis", F},
                         {"meanAnomalyAtEpoch", F}, {"epoch", F}, {"ObT", F}, {"meanAnomaly", F}, {"trueAnomaly", F},
                         {"packed", B}, {"loaded", B}, {"update_mode", S}},
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
        const Orbit& o2 = probes[k].v->orbit();
        ti.Add({c.id, o2.referenceBody->bodyName, c.inc, c.e, c.sma, c.lan, c.argPe, c.mEp, c.epoch, o2.inclination,
                o2.eccentricity, o2.semiMajorAxis, o2.LAN, o2.argumentOfPeriapsis, o2.meanAnomalyAtEpoch, o2.epoch,
                o2.meanMotion, o2.period, o2.ObTAtEpoch, o2.referenceBody->gravParameter, true});
    }

    const int rateIdx[5] = {0, 4, 5, 6, 7};
    const int segFrames[5] = {500, 1000, 1000, 1500, 500};
    int frame = 0;
    for (int s = 0; s < 5; ++s) {
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
            for (Probe& p : probes) {
                Vessel& v = *p.v;
                if (v.state == Vessel::State::DEAD) continue;
                const Orbit& o = v.orbit();
                const Vector3d rel = o.getRelativePositionAtUT(ut);
                const Vector3d wr = v.GetWorldPos3D() - o.referenceBody->position;  // Unity world axes (y up)
                ts.Add({p.id, frame, s, ut, rate, o.referenceBody->bodyName, v.orbitDriver->UpdateUT(), o.pos.x, o.pos.y, o.pos.z, o.vel.x, o.vel.y,
                        o.vel.z, rel.x, rel.y, rel.z, wr.x, wr.y, wr.z, o.semiMajorAxis, o.eccentricity, o.inclination, o.LAN,
                        o.argumentOfPeriapsis, o.meanAnomalyAtEpoch, o.epoch, o.ObT, o.meanAnomaly, o.trueAnomaly, v.packed, v.loaded,
                        ToString(v.orbitDriver->updateMode)});
            }
            for (CelestialBody* b : {mun, minmus}) {
                const auto* ad = dynamic_cast<const CelestialBodyOrbitAdapter*>(b->orbit());
                if (ad == nullptr) throw std::runtime_error("vessel_rails: body without a run-time OrbitDriver");
                const Orbit& o = ad->orbit;
                const Vector3d rel = o.getRelativePositionAtUT(ut);
                // body rows: no vessel state (packed / loaded / world_rel are the vessel rows' columns; the capture has them for bodies too, as
                // `False/False/world` of the body's transform; the gate keeps them for vessels only)
                ts.Add({"body:" + b->bodyName, frame, s, ut, rate, o.referenceBody->bodyName, ad->driver.UpdateUT(), o.pos.x, o.pos.y, o.pos.z, o.vel.x,
                        o.vel.y, o.vel.z, rel.x, rel.y, rel.z, 0.0, 0.0, 0.0, o.semiMajorAxis, o.eccentricity, o.inclination, o.LAN,
                        o.argumentOfPeriapsis, o.meanAnomalyAtEpoch, o.epoch, o.ObT, o.meanAnomaly, o.trueAnomaly, false, false,
                        ToString(ad->driver.updateMode)});
            }
        }
        tseg.Add({s, rateIdx[s], timeWarp.warpRates.at(static_cast<size_t>(rateIdx[s])), achieved, segFrames[s], utStart, Planetarium::GetUniversalTime(),
                  Time::fixedDeltaTime, TimeWarp::fixedDeltaTime(), Time::timeScale});
    }
    for (const SoiEvent& e : events)
        tev.Add({e.vessel, e.UT, e.from, e.to, e.sma, e.e, e.inc, e.lan, e.argPe, e.mEp, e.epoch, e.rate});

    GE::onVesselSOIChanged.Remove(soiHandler);
    TimeWarp::fetch = nullptr;
    TimeWarp::environment = nullptr;
    FlightGlobals::Clear();
    for (Probe& p : probes) FlightGlobals::RemoveVessel(p.v.get());
    probes.clear();  // OrbitDriver.OnDestroy: out of Planetarium.Orbits
    return WriteCandidate(opt, g, "vessel_rails/vessel_rails.json");
}

KSP_GOLDEN_DATASET("vessel_rails", 91, RunVesselRails);
