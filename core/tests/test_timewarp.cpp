// WP-10: TimeWarp (TimeWarp.cs), WarpTransition / WarpTransitionCalculator, PositionTarget and the vessel side of
// OrbitDriver (TRACK_Phys / IDLE / UPDATE transitions). The bit-exact numbers of time warp (warp rate, Time.fixedDeltaTime,
// TimeWarp.fixedDeltaTime, timeScale across five warp segments) are the in-game gate capture.orbit_rails; everything else
// here is derived from the decompiled source and the game's own data:
//   * prefab rate tables: analysis/physics/raw/run*/*_summary.csv (rt_TimeWarp.* from the running game) and the C8 capture;
//   * per-body altitude limits: the 17 body prefabs (CelestialBody.timeWarpAltitudeLimits) = altitudeLimits * Radius;
//   * rate-selection scenarios use Kerbin's atmosphere (70 km, pressure curve) and the rate/limit tables above; the expected
//     values are worked out from the C# control flow (comments next to each case).
// No golden or capture exists for getMaxOnRailsRateIdx / autoWarpTo / mode switching: see STATUS.md "WP-10" for the
// captures that would gate them.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/celestial/celestial_body_orbit.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/orbit/position_target.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/orbit/warp_transition.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/terrain/pqs_prefab.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/util/simplex.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using Modes = TimeWarp::Modes;
using PT = Orbit::PatchTransitionType;

namespace {

const std::string kHarness = KSPCORE_HARNESS_DIR;
const std::string kRoot = KSPCORE_PROJECT_ROOT;
const std::string kData = KSPCORE_DATA_DIR;

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}

// The in-game start-up (as tests/test_patched_conics.cpp) plus the flight-scene state TimeWarp reads.
struct Env {
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    TimeWarpEnvironment twEnv;
    Env() {
        Reset();
        // The screen-message checks below assume no Localizer instance (keys are posted verbatim, as before any
        // GameData load); another test file may have left one installed (Localizer.cs:154).
        Localizer::Shutdown();
        pg.physicsDatabaseFilename = kHarness + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        pg.CalculateValues(sys->Bodies(), planetarium.Sun);
        for (CelestialBody* b : sys->Bodies()) b->inverseRotation = false;  // inertial frame (as test_patched_conics)
        twEnv.LoadedSceneValue = GameScenes::FLIGHT;
        TimeWarp::environment = &twEnv;
        FlightGlobals::currentMainBody = Body("Kerbin");
        FlightGlobals::ready = true;
    }
    ~Env() {
        sys.reset();
        Reset();
    }
    static void Reset() {
        Time::timeScale = 1.0f;
        Time::fixedDeltaTime = 0.02f;
        Time::time = 0.0f;
        Time::deltaTime = 0.02f;
        TimeWarp::fetch = nullptr;
        TimeWarp::environment = nullptr;
        FlightGlobals::ready = false;
        FlightGlobals::ActiveVessel = nullptr;
        FlightGlobals::VesselsLoaded.clear();
        FlightGlobals::currentMainBody = nullptr;
        GameSettings::ORBIT_WARP_DOWN_AT_SOI = true;
        GameSettings::SHOW_PWARP_WARNING = true;
        GameSettings::ORBIT_WARP_MAXRATE_MODE = TimeWarp::MaxRailsRateMode::PeAltitude;
        GameSettings::ORBIT_WARP_PEMODE_SURFACE_MARGIN = 250.0;
        GameSettings::ORBIT_WARP_ALTMODE_LIMIT_MODIFIER = 1.0f;
        WarpTransitionCalculator::warpRates.reset();
        WarpTransitionCalculator::WarpRateTransitionPeriods.clear();
    }
    CelestialBody* Body(const char* n) const { return sys->GetBodyByName(n); }
};

terrain::MapLibrary& Maps() {
    static terrain::MapLibrary maps(kData + "/terrain");
    return maps;
}

// Vessel members TimeWarp reads (the Vessel package implements IFlightVessel for real).
struct StubVessel final : IFlightVessel {
    bool landed = false;
    CelestialBody* body = nullptr;
    Vector3f position;
    Orbit* orb = nullptr;
    double alt = 0.0;
    double gee = 0.0;
    Vector3d srf;
    bool patched = false;
    bool eva = false;
    bool ladder = false;
    bool LandedOrSplashed() const override { return landed; }
    CelestialBody* mainBody() const override { return body; }
    Vector3f TransformPosition() const override { return position; }
    Orbit* orbit() const override { return orb; }
    double altitude() const override { return alt; }
    double geeForce() const override { return gee; }
    Vector3d srf_velocity() const override { return srf; }
    bool PatchedConicsAttached() const override { return patched; }
    bool isEVA() const override { return eva; }
    bool OnALadder() const override { return ladder; }
    // Hover at `altitude` metres above the body centre line (+x), as the vessel position and Vessel.altitude
    void PlaceAt(double altitudeMetres) {
        alt = altitudeMetres;
        position = Vector3d(body->position + Vector3d(body->Radius + altitudeMetres, 0.0, 0.0)).ToVector3f();
    }
};

// A TimeWarp as the flight scene has it (prefab data, Awake, Start).
struct Warp {
    TimeWarp tw;
    Warp() {
        tw.LoadStockPrefab();
        REQUIRE(tw.Awake());
        tw.Start();
    }
};

std::vector<float> ParseFloats(const std::string& s) {
    std::vector<float> v;
    std::istringstream in(s);
    std::string tok;
    while (in >> tok) v.push_back(std::stof(tok));
    return v;
}

// The Kerbin PQS the game builds (TimeWarp.getMaxOnRailsRateIdx reads pqsController.radiusMax / radius / meshVertMax).
// Building a PQS reseeds the process-wide Simplex permutation tables and the global UnityRandom (as in KSP); restore them
// so later test cases see the state they expect (as tests/test_terrain.cpp's GlobalStateGuard).
struct KerbinTerrain {
    std::array<int, 256> simplexP = Simplex::StaticPermutation();
    std::array<int, 256> simplexNoiseP = SimplexNoise::StaticPermutation();
    UnityRandom::State random = UnityRandom::Global().state();
    std::unique_ptr<PQS> pqs;
    CelestialBody* body;
    explicit KerbinTerrain(CelestialBody* kerbin) : body(kerbin) {
        pqs = terrain::LoadStockPQS("Kerbin", Maps());
        kerbin->pqsController = pqs.get();
    }
    ~KerbinTerrain() {
        body->pqsController = nullptr;
        Simplex::StaticPermutation() = simplexP;
        SimplexNoise::StaticPermutation() = simplexNoiseP;
        UnityRandom::Global().SetState(random);
    }
};

}  // namespace

TEST_CASE("TimeWarp prefab data equals the running game's (19 runs) and the C8 capture's requested rates") {
    TimeWarp tw;
    tw.LoadStockPrefab();
    int rows = 0;
    const std::filesystem::path raw = dotnet::PathFromUtf8(kRoot + "/analysis/physics/raw");
    REQUIRE(std::filesystem::exists(raw));
    for (const auto& run : std::filesystem::directory_iterator(raw)) {
        if (!run.is_directory()) continue;
        for (const auto& f : std::filesystem::directory_iterator(run.path())) {
            const std::string name = f.path().filename().string();
            if (name.size() < 12 || name.substr(name.size() - 12) != "_summary.csv") continue;
            std::ifstream in(f.path());
            std::string line;
            while (std::getline(in, line)) {
                const size_t k = line.find("rt_TimeWarp.");
                if (k == std::string::npos) continue;
                const size_t c = line.find(',', k);
                const std::string field = line.substr(k + 12, c - k - 12);
                std::string val = line.substr(c + 1);
                while (!val.empty() && (val.back() == '\r' || val.back() == '\n')) val.pop_back();
                ++rows;
                if (field == "warpRates") CHECK(ParseFloats(val) == tw.warpRates);
                else if (field == "physicsWarpRates") CHECK(ParseFloats(val) == tw.physicsWarpRates);
                else if (field == "altitudeLimits") CHECK(ParseFloats(val) == tw.altitudeLimits);
                else if (field == "maxPhysicsRate_index") CHECK(std::stoi(val) == tw.maxPhysicsRate_index);
                else if (field == "maxModeSwitchRate_index") CHECK(std::stoi(val) == tw.maxModeSwitchRate_index);
                else FAIL("unexpected rt_TimeWarp field " << field);
            }
        }
    }
    CHECK(rows == 5 * 19);
    CHECK(tw.warpRates.size() == 8);
    CHECK(tw.altitudeLimits.size() == 8);
    CHECK(tw.physicsWarpRates.size() == 4);

    // the C8 capture (in-game): requested_rate of each warp segment is warpRates[rate_index], bit for bit
    const auto text = dotnet::ReadAllText(kHarness + "/captures/out2/orbit_rails_ingame.json");
    REQUIRE(text.has_value());
    const nlohmann::json doc = nlohmann::json::parse(*text);
    int checked = 0;
    for (const auto& t : doc["tables"]) {
        if (t["name"] != "segments") continue;
        std::vector<std::string> cols;
        for (const auto& c : t["columns"]) cols.push_back(c["name"].get<std::string>());
        const size_t ci = static_cast<size_t>(std::find(cols.begin(), cols.end(), "rate_index") - cols.begin());
        const size_t cr = static_cast<size_t>(std::find(cols.begin(), cols.end(), "requested_rate_hex") - cols.begin());
        for (const auto& r : t["rows"]) {
            const int idx = r[ci].get<int>();
            CHECK(Bits(tw.warpRates.at(static_cast<size_t>(idx))) == static_cast<uint32_t>(std::stoul(r[cr].get<std::string>(), nullptr, 16)));
            ++checked;
        }
    }
    CHECK(checked == 5);
}

TEST_CASE("CelestialBody.resetTimeWarpLimits: altitudeLimits * Radius reproduces the body prefabs' limits") {
    Env env;
    Warp w;
    int bodies = 0, matched = 0;
    std::vector<std::string> differing;
    for (CelestialBody* b : env.sys->Bodies()) {
        const std::vector<float> prefab = b->timeWarpAltitudeLimits;
        REQUIRE(prefab.size() == 8);  // Start() found the count equal to TimeWarp's: nothing was reset
        ++bodies;
        b->resetTimeWarpLimits();
        REQUIRE(b->timeWarpAltitudeLimits.size() == 8);
        if (b->timeWarpAltitudeLimits == prefab) ++matched;
        else differing.push_back(b->bodyName);
        // restore the prefab values for the rest of the test
        b->timeWarpAltitudeLimits = prefab;
    }
    CHECK(bodies == 17);
    // Only Kerbin, Minmus and Tylo have prefab limits equal to altitudeLimits * Radius (the formula the game falls back to
    // when a body's array has the wrong length); the other 14 are hand-tuned per body (Eve 30 km at index 1 where the
    // formula gives 35 km, Mun 5 km where it gives 10 km, Jool 0/0/15000...).
    INFO("bodies whose prefab limits differ from the formula: " << [&] {
        std::string s;
        for (const auto& n : differing) s += n + " ";
        return s;
    }());
    CHECK(matched + static_cast<int>(differing.size()) == bodies);
    CHECK(matched == 3);
    CHECK(differing.size() == 14);
    CHECK(std::find(differing.begin(), differing.end(), "Kerbin") == differing.end());
    CHECK(std::find(differing.begin(), differing.end(), "Minmus") == differing.end());
    CHECK(std::find(differing.begin(), differing.end(), "Tylo") == differing.end());
    CHECK(std::find(differing.begin(), differing.end(), "Mun") != differing.end());

    // CelestialBody.Start: a limits array whose length differs from TimeWarp's is reset to the formula
    CelestialBody* mun = env.Body("Mun");
    mun->timeWarpAltitudeLimits = {0.0f, 1.0f};
    mun->Start();
    REQUIRE(mun->timeWarpAltitudeLimits.size() == 8);
    CHECK(mun->timeWarpAltitudeLimits[1] == f32store(fx(0.05f) * fx(static_cast<float>(mun->Radius))));
    CHECK(mun->timeWarpAltitudeLimits[7] == f32store(fx(1.0f) * fx(static_cast<float>(mun->Radius))));
}

TEST_CASE("TimeWarp.GetMaxRateForAltitude per body (rate index allowed at an altitude)") {
    Env env;
    Warp w;
    CelestialBody* kerbin = env.Body("Kerbin");
    CelestialBody* mun = env.Body("Mun");
    // Kerbin: 0, 30000, 30000, 60000, 120000, 240000, 480000, 600000 (first index whose limit >= altitude, minus 1)
    REQUIRE(kerbin->timeWarpAltitudeLimits.size() == 8);
    struct Case { double alt; int expect; };
    const Case kerbinCases[] = {{-5.0, 0},     {0.0, 0},      {1.0, 0},      {30000.0, 0},   {30000.5, 2},   {60000.0, 2},
                                {60000.5, 3},  {120000.0, 3}, {120001.0, 4}, {240001.0, 5},  {480001.0, 6},  {600000.0, 6},
                                {600000.5, 7}, {1.0e9, 7}};
    for (const Case& c : kerbinCases) {
        INFO("Kerbin altitude " << c.alt);
        CHECK(w.tw.GetMaxRateForAltitude(c.alt, *kerbin) == c.expect);
    }
    // Mun prefab limits (hand-tuned): 0, 5000, 5000, 10000, 25000, 50000, 100000, 200000
    REQUIRE(mun->timeWarpAltitudeLimits.size() == 8);
    CHECK(w.tw.GetMaxRateForAltitude(4999.0, *mun) == 0);
    CHECK(w.tw.GetMaxRateForAltitude(5000.5, *mun) == 2);
    CHECK(w.tw.GetMaxRateForAltitude(10000.5, *mun) == 3);
    CHECK(w.tw.GetMaxRateForAltitude(200000.0, *mun) == 6);
    CHECK(w.tw.GetMaxRateForAltitude(200000.5, *mun) == 7);
    // generic monotonicity over every body: more altitude never allows a lower index
    for (CelestialBody* b : env.sys->Bodies()) {
        int prev = 0;
        for (double a = -10.0; a < 2.0 * b->Radius; a = a * 1.3 + 7.0) {
            const int idx = w.tw.GetMaxRateForAltitude(a, *b);
            CHECK(idx >= prev);
            CHECK(idx <= 7);
            prev = idx;
        }
        CHECK(w.tw.GetAltitudeLimit(3, *b) == static_cast<double>(b->timeWarpAltitudeLimits[3]));
    }
}

TEST_CASE("TimeWarp: rate properties, Awake/OnDestroy singleton, updateRate in HIGH and LOW mode") {
    Env env;
    CHECK(TimeWarp::fetch == nullptr);
    // without an instance
    CHECK(TimeWarp::WarpMode() == Modes::HIGH);
    CHECK(TimeWarp::CurrentRate() == 1.0f);
    CHECK(TimeWarp::CurrentRateIndex() == 1);
    CHECK(TimeWarp::MaxRate() == 100000.0f);
    CHECK(TimeWarp::MaxPhysicsRate() == 4.0f);
    CHECK_FALSE(TimeWarp::CurrentRateIsTargetRate());
    CHECK(TimeWarp::fixedDeltaTime() == 0.02f);
    {
        Warp w;
        CHECK(TimeWarp::fetch == &w.tw);
        TimeWarp second;
        CHECK_FALSE(second.Awake());  // Awake: fetch exists -> Destroy(this)
        CHECK(TimeWarp::fetch == &w.tw);
        // Start(): setRate(0) -> rate 1x, timeScale 1
        CHECK(TimeWarp::CurrentRate() == 1.0f);
        CHECK(TimeWarp::CurrentRateIndex() == 0);
        CHECK(w.tw.tgt_rate() == 1.0f);
        CHECK(TimeWarp::MaxRate() == 100000.0f);
        CHECK(TimeWarp::MaxPhysicsRate() == 1.0f);  // warpRates[maxPhysicsRate_index = 0]
        CHECK(TimeWarp::fixedDeltaTime() == 0.02f);
        CHECK(Planetarium::TimeScale() == 1.0);
        CHECK(w.tw.showPhyWarpWarning);  // GameSettings.SHOW_PWARP_WARNING default true

        // HIGH mode (rails): Time.timeScale stays 1, Planetarium.TimeScale = rate, TimeWarp.fixedDeltaTime = dt * rate
        w.tw.curr_rate = 100.0f;
        w.tw.updateRate(100.0f);
        CHECK(Planetarium::TimeScale() == 100.0);
        CHECK(Time::timeScale == 1.0f);
        CHECK(Time::fixedDeltaTime == 0.02f);
        CHECK(TimeWarp::fixedDeltaTime() == f32store(fx(0.02f) * 100.0));
        CHECK(TimeWarp::deltaTime() == f32store(fx(0.02f) * 100.0));
        // one physics tick advances UT by dt * timeScale = TimeWarp.fixedDeltaTime (rails: 2 s per tick at 100x)
        const double ut0 = Planetarium::GetUniversalTime();
        env.planetarium.FixedUpdate();
        CHECK(Planetarium::GetUniversalTime() - ut0 == doctest::Approx(static_cast<double>(TimeWarp::fixedDeltaTime())).epsilon(1e-6));

        // LOW mode (physics warp 4x): Time.timeScale = rate, Time.fixedDeltaTime = 0.02f * timeScale, TimeWarp.fixedDeltaTime
        // = Time.fixedDeltaTime, UT per tick = 0.02 * 4
        w.tw.Mode = Modes::LOW;
        w.tw.curr_rate = 4.0f;
        w.tw.updateRate(4.0f);
        CHECK(Planetarium::TimeScale() == 4.0);
        CHECK(Time::timeScale == 4.0f);
        CHECK(Time::fixedDeltaTime == f32store(fx(0.02f) * 4.0));
        CHECK(TimeWarp::fixedDeltaTime() == Time::fixedDeltaTime);
        CHECK(TimeWarp::deltaTime() == Time::deltaTime);
        const double ut1 = Planetarium::GetUniversalTime();
        env.planetarium.FixedUpdate();
        CHECK(Planetarium::GetUniversalTime() - ut1 == doctest::Approx(static_cast<double>(TimeWarp::fixedDeltaTime())).epsilon(1e-6));
        w.tw.Mode = Modes::HIGH;
    }
    CHECK(TimeWarp::fetch == nullptr);  // OnDestroy
}

TEST_CASE("TimeWarp.SetRate: landed vessel passes every rate, locks controls above the physics limit") {
    Env env;
    Warp w;
    CelestialBody* kerbin = env.Body("Kerbin");
    KerbinTerrain terrain(kerbin);
    StubVessel pad;
    pad.body = kerbin;
    pad.landed = true;
    pad.PlaceAt(67.0);
    FlightGlobals::ActiveVessel = &pad;
    FlightGlobals::VesselsLoaded = {&pad};
    int rateEvents = 0;
    const auto listener = events::EventVoid::OnEvent::Lambda(&rateEvents, [&rateEvents]() { ++rateEvents; });
    GameEvents::onTimeWarpRateChanged.Add(listener);

    const int idx[] = {0, 4, 5, 6, 7};  // the C8 capture's segments
    const float rate[] = {1.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f};
    for (int i = 0; i < 5; ++i) {
        TimeWarp::SetRate(idx[i], true);
        CHECK(w.tw.current_rate_index == idx[i]);
        CHECK(TimeWarp::CurrentRate() == rate[i]);
        CHECK(Planetarium::TimeScale() == static_cast<double>(rate[i]));
        CHECK(Time::timeScale == 1.0f);
        CHECK(Time::fixedDeltaTime == 0.02f);
        CHECK(TimeWarp::fixedDeltaTime() == f32store(fx(0.02f) * fx(rate[i])));
        // controls are locked above warpRates[maxPhysicsRate_index]; updateRate releases at <= MaxPhysicsRate
        CHECK(w.tw.controlsLocked == (idx[i] > 0));
        CHECK(env.twEnv.controlLocks.count("TimeWarpLock") == (idx[i] > 0 ? 1u : 0u));
    }
    CHECK(rateEvents == 5);
    // an out-of-range index is clamped to the last rate (TimeWarp.SetRate: Clamp(idx, 0, Length), setRate: Length - 1)
    TimeWarp::SetRate(99, true);
    CHECK(w.tw.current_rate_index == 7);
    CHECK(TimeWarp::CurrentRate() == 100000.0f);
    TimeWarp::SetRate(-3, true);
    CHECK(w.tw.current_rate_index == 0);
    CHECK_FALSE(w.tw.controlsLocked);

    // a non-instant change lerps over one second of Time.time: curr_rate = Lerp(last_rate, tgt_rate, Time.time - start)
    TimeWarp::SetRate(4, false);
    CHECK(w.tw.tgt_rate() == 100.0f);
    CHECK(TimeWarp::CurrentRate() == 1.0f);
    Time::time = Time::time + 0.5f;
    w.tw.FixedUpdate();
    CHECK(TimeWarp::CurrentRate() == 1.0f + (100.0f - 1.0f) * 0.5f);
    CHECK(Planetarium::TimeScale() == static_cast<double>(TimeWarp::CurrentRate()));
    Time::time = Time::time + 0.75f;
    w.tw.FixedUpdate();
    CHECK(TimeWarp::CurrentRate() == 100.0f);
    w.tw.FixedUpdate();  // Planetarium.TimeScale == tgt_rate: nothing left to lerp
    CHECK(TimeWarp::CurrentRate() == 100.0f);
    GameEvents::onTimeWarpRateChanged.Remove(listener);
}

TEST_CASE("TimeWarp.getMaxOnRailsRateIdx: atmosphere, acceleration, ladder, surface motion") {
    Env env;
    Warp w;
    CelestialBody* kerbin = env.Body("Kerbin");
    KerbinTerrain terrain(kerbin);
    StubVessel v;
    v.body = kerbin;
    FlightGlobals::ActiveVessel = &v;
    FlightGlobals::VesselsLoaded = {&v};
    ClearToSaveStatus reason = ClearToSaveStatus::CLEAR;

    SUBCASE("rate 0 is never limited") {
        v.PlaceAt(10000.0);
        CHECK(w.tw.getMaxOnRailsRateIdx(0, true, reason) == 0);
        CHECK(reason == ClearToSaveStatus::CLEAR);
    }
    SUBCASE("static pressure > 0: back to 1x (Kerbin's atmosphere is 70 km deep)") {
        v.PlaceAt(10000.0);
        CHECK(w.tw.getMaxOnRailsRateIdx(5, true, reason) == 0);
        CHECK(reason == ClearToSaveStatus::NOT_IN_ATMOSPHERE);
        v.PlaceAt(69999.0);
        CHECK(w.tw.getMaxOnRailsRateIdx(5, true, reason) == 0);
        CHECK(reason == ClearToSaveStatus::NOT_IN_ATMOSPHERE);
        // minPeAllowed = max(radiusMax - radius, atmosphereDepth, meshVertMax) = the 70 km atmosphere (no quads built)
        CHECK(w.tw.minPeAllowed == kerbin->atmosphereDepth);
    }
    SUBCASE("acceleration above GThreshold (0.1 g) in vacuum") {
        v.PlaceAt(100000.0);
        v.gee = 0.2;
        CHECK(w.tw.getMaxOnRailsRateIdx(5, true, reason) == 0);
        CHECK(reason == ClearToSaveStatus::NOT_UNDER_ACCELERATION);
        v.gee = 0.1;  // not strictly greater
        Orbit o(0, 0.0, 700000.0, 0, 0, 0, 0, kerbin);
        o.UpdateFromUT(0.0);
        v.orb = &o;
        CHECK(w.tw.getMaxOnRailsRateIdx(5, true, reason) == 5);
    }
    SUBCASE("an EVA kerbin on a ladder anywhere in the loaded set") {
        StubVessel eva;
        eva.body = kerbin;
        eva.eva = true;
        eva.ladder = true;
        FlightGlobals::VesselsLoaded = {&v, &eva};
        v.PlaceAt(100000.0);
        CHECK(w.tw.getMaxOnRailsRateIdx(4, true, reason) == 0);
        CHECK(reason == ClearToSaveStatus::NOT_WHILE_ON_A_LADDER);
        eva.ladder = false;  // free-floating EVA does not block
        Orbit o(0, 0.0, 700000.0, 0, 0, 0, 0, kerbin);
        o.UpdateFromUT(0.0);
        v.orb = &o;
        CHECK(w.tw.getMaxOnRailsRateIdx(4, true, reason) == 4);
    }
    SUBCASE("landed: moving over the surface (|srf|^2 > 0.09) blocks, standing still does not") {
        v.landed = true;
        v.PlaceAt(67.0);
        v.srf = Vector3d(0.31, 0.0, 0.0);
        CHECK(w.tw.getMaxOnRailsRateIdx(6, true, reason) == 0);
        CHECK(reason == ClearToSaveStatus::NOT_WHILE_MOVING_OVER_SURFACE);
        v.srf = Vector3d(0.29, 0.0, 0.0);
        CHECK(w.tw.getMaxOnRailsRateIdx(6, true, reason) == 6);
        CHECK(reason == ClearToSaveStatus::CLEAR);
    }
    SUBCASE("outside the flight scene nothing limits the rate") {
        env.twEnv.LoadedSceneValue = GameScenes::SPACECENTER;
        v.PlaceAt(10000.0);
        CHECK(w.tw.getMaxOnRailsRateIdx(7, true, reason) == 7);
        CHECK(w.tw.setRate(7, true));  // no limiting in setRate either
    }
}

TEST_CASE("TimeWarp.getMaxOnRailsRateIdx: altitude look-ahead on a 100 km circular and a skimming orbit") {
    Env env;
    Warp w;
    CelestialBody* kerbin = env.Body("Kerbin");
    KerbinTerrain terrain(kerbin);
    StubVessel v;
    v.body = kerbin;
    FlightGlobals::ActiveVessel = &v;
    FlightGlobals::VesselsLoaded = {&v};
    ClearToSaveStatus reason = ClearToSaveStatus::CLEAR;

    SUBCASE("circular 100 km, PeAltitude mode: peA 100 km > minPeAllowed (70 km) + 250 m: no altitude limiting") {
        Orbit o(0, 0.0, 700000.0, 0, 0, 0, 0, kerbin);
        o.UpdateFromUT(0.0);
        v.orb = &o;
        v.PlaceAt(100000.0);
        CHECK(w.tw.getMaxOnRailsRateIdx(7, true, reason) == 7);
        CHECK(reason == ClearToSaveStatus::CLEAR);
    }
    SUBCASE("circular 100 km, VesselAltitude mode: limit(7) = 600 km > 100 km: index of the altitude table, 3") {
        // GetMaxRateForAltitude(100000) = (first limit >= 100000 is limits[4] = 120000) - 1 = 3, then the look-ahead
        // loop checks altitude 100 km >= limits[3] = 60 km and keeps 3
        GameSettings::ORBIT_WARP_MAXRATE_MODE = TimeWarp::MaxRailsRateMode::VesselAltitude;
        Orbit o(0, 0.0, 700000.0, 0, 0, 0, 0, kerbin);
        o.UpdateFromUT(0.0);
        v.orb = &o;
        v.PlaceAt(100000.0);
        CHECK(w.tw.getMaxOnRailsRateIdx(7, true, reason) == 3);
        CHECK(reason == ClearToSaveStatus::NOT_WHILE_ABOUT_TO_CRASH);
        // low enough to be under limits[1] = 30 km is blocked entirely: rate index 0
        v.PlaceAt(500.0);
        CHECK(w.tw.getMaxOnRailsRateIdx(7, true, reason) == 0);
    }
    SUBCASE("pe 20 km / ap 100 km at apoapsis: the look-ahead lowers 100000x to 3 (PeAltitude, peA inside the atmosphere)") {
        // timeToPe ~ 896 s. idx 7: 2000 s > timeToPe -> altitude = peA = 20 km (in the atmosphere) -> idx 6: 200 s ahead,
        // altitude ~91 km (no pressure) stops the atmosphere loop (reason NOT_IN_ATMOSPHERE); peA is not above
        // 70 km + 250 m so the altitude-limit loop runs: 91 km < limits[6] 480 km, then idx 5 (240 km), idx 4 (120 km):
        // each fails; idx 3: 100 km >= 60 km holds -> result 3 (reason NOT_WHILE_ABOUT_TO_CRASH)
        Orbit o(0, (700000.0 - 620000.0) / 1320000.0, 660000.0, 0, 0, 3.141592653589793, 0, kerbin);
        o.UpdateFromUT(0.0);
        REQUIRE(o.PeA() == doctest::Approx(20000.0).epsilon(1e-9));
        REQUIRE(o.timeToPe > 800.0);
        REQUIRE(o.timeToPe < 1000.0);
        v.orb = &o;
        v.PlaceAt(o.radius - kerbin->Radius);
        CHECK(w.tw.getMaxOnRailsRateIdx(7, true, reason) == 3);
        CHECK(reason == ClearToSaveStatus::NOT_WHILE_ABOUT_TO_CRASH);
        // the same through setRate: current index, posted message (PeAltitude: #autoLOC_8003368), controls locked
        CHECK_FALSE(w.tw.setRate(7, false));  // returns num == rateIdx
        CHECK(w.tw.current_rate_index == 3);
        CHECK(w.tw.tgt_rate() == 50.0f);
        CHECK(env.twEnv.controlLocks.count("TimeWarpLock") == 1u);
        bool found = false;
        for (const auto& m : env.twEnv.screenMessages) found = found || m.rfind("#autoLOC_8003368", 0) == 0;
        CHECK(found);
    }
    SUBCASE("ORBIT_WARP_DOWN_AT_SOI: an imminent patch transition drops to index 3 (50x)") {
        // ClampRateToOrbitTransitions(rate, orbit, 3, 50): travel = (50^2 - TimeScale^2) / (2 * (50 - TimeScale)) = 25.5 s
        // at TimeScale 1; the clamp applies when EndUT - UT < 25.5 + 50
        Orbit o(0, 0.0, 700000.0, 0, 0, 0, 0, kerbin);
        o.UpdateFromUT(0.0);
        o.patchEndTransition = PT::ENCOUNTER;
        v.orb = &o;
        v.patched = true;
        v.PlaceAt(100000.0);
        o.EndUT = Planetarium::GetUniversalTime() + 75.4;
        CHECK(w.tw.getMaxOnRailsRateIdx(7, true, reason) == 3);
        CHECK(reason == ClearToSaveStatus::ORBIT_EVENT_IMMINENT);
        o.EndUT = Planetarium::GetUniversalTime() + 75.6;
        CHECK(w.tw.getMaxOnRailsRateIdx(7, true, reason) == 7);
        CHECK(reason == ClearToSaveStatus::CLEAR);
        // indices at or below 3 are never clamped; a FINAL patch and the setting both disable it
        o.EndUT = Planetarium::GetUniversalTime() + 1.0;
        CHECK(w.tw.getMaxOnRailsRateIdx(3, true, reason) == 3);
        o.patchEndTransition = PT::FINAL;
        CHECK(w.tw.getMaxOnRailsRateIdx(7, true, reason) == 7);
        o.patchEndTransition = PT::ESCAPE;
        GameSettings::ORBIT_WARP_DOWN_AT_SOI = false;
        CHECK(w.tw.getMaxOnRailsRateIdx(7, true, reason) == 7);
        GameSettings::ORBIT_WARP_DOWN_AT_SOI = true;
        v.patched = false;
        CHECK(w.tw.getMaxOnRailsRateIdx(7, true, reason) == 7);
    }
    SUBCASE("rate helpers: warp acceleration and the UT consumed by a rate change") {
        CHECK(TimeWarp::getWarpAccel(1.0, 50.0, 1.0) == 49.0);
        CHECK(TimeWarp::getRateChangeTravel(1.0, 50.0, 49.0) == 25.5);
        CHECK(TimeWarp::getRateChangeTravel(7.0, 7.0, 0.0) == 0.0);  // a == 0
        // the linear 1 s ramp from v0 to 1 consumes (1 + v0) / 2 s of UT
        const double v0 = 10000.0;
        CHECK(TimeWarp::getRateChangeTravel(v0, 1.0, TimeWarp::getWarpAccel(v0, 1.0, 1.0)) == doctest::Approx((1.0 + v0) / 2.0).epsilon(1e-12));
    }
}

TEST_CASE("TimeWarp.FixedUpdate: entering the atmosphere under rails warp drops to 1x at once") {
    Env env;
    Warp w;
    CelestialBody* kerbin = env.Body("Kerbin");
    KerbinTerrain terrain(kerbin);
    StubVessel v;
    v.body = kerbin;
    v.landed = true;
    v.PlaceAt(67.0);
    FlightGlobals::ActiveVessel = &v;
    FlightGlobals::VesselsLoaded = {&v};
    TimeWarp::SetRate(4, true);  // 100x while landed and still
    REQUIRE(TimeWarp::CurrentRate() == 100.0f);
    int rateEvents = 0;
    const auto listener = events::EventVoid::OnEvent::Lambda(&rateEvents, [&rateEvents]() { ++rateEvents; });
    GameEvents::onTimeWarpRateChanged.Add(listener);
    // the vessel leaves the ground at 10 km altitude: curr_rate (100) > warpRates[0] and not landed -> setRate(index,
    // instant = false, instantIfLower = true, forceSwitch = false) -> limited to index 0 for NOT_IN_ATMOSPHERE -> instant
    v.landed = false;
    v.PlaceAt(10000.0);
    w.tw.FixedUpdate();
    CHECK(w.tw.current_rate_index == 0);
    CHECK(TimeWarp::CurrentRate() == 1.0f);
    CHECK(Planetarium::TimeScale() == 1.0);
    CHECK(rateEvents == 1);
    CHECK(env.twEnv.ignoreGeeForcesSeconds == 1.0f);
    CHECK_FALSE(w.tw.controlsLocked);
    CHECK(env.twEnv.controlLocks.count("TimeWarpLock") == 0u);
    // already at the limit: forceSwitch = false does not reassume the same index
    w.tw.FixedUpdate();
    CHECK(rateEvents == 1);
    GameEvents::onTimeWarpRateChanged.Remove(listener);
}

TEST_CASE("TimeWarp.Update: physics warp (LOW) and rails (HIGH) mode switching, keys and the physics-warp warning") {
    Env env;
    Warp w;
    CelestialBody* kerbin = env.Body("Kerbin");
    KerbinTerrain terrain(kerbin);
    StubVessel v;
    v.body = kerbin;
    FlightGlobals::ActiveVessel = &v;
    FlightGlobals::VesselsLoaded = {&v};

    SUBCASE("flying in the atmosphere -> LOW; reaching vacuum -> HIGH") {
        v.PlaceAt(10000.0);
        CHECK(w.tw.Mode == Modes::HIGH);
        w.tw.Update();
        CHECK(w.tw.Mode == Modes::LOW);
        w.tw.Update();
        CHECK(w.tw.Mode == Modes::LOW);
        v.PlaceAt(100000.0);
        w.tw.Update();
        CHECK(w.tw.Mode == Modes::HIGH);
        // not while the rate is changing (curr_rate != tgt_rate) or above maxModeSwitchRate_index
        v.PlaceAt(10000.0);
        TimeWarp::SetRate(0, false);
        w.tw.curr_rate = 0.5f;  // mid-lerp
        w.tw.Update();
        CHECK(w.tw.Mode == Modes::HIGH);
    }
    SUBCASE("landed: moving (|srf|^2 > GThreshold 0.1) -> LOW; standing still -> HIGH") {
        v.landed = true;
        v.PlaceAt(67.0);
        v.srf = Vector3d(0.5, 0.0, 0.0);
        w.tw.Update();
        CHECK(w.tw.Mode == Modes::LOW);
        v.srf = Vector3d(0.05, 0.0, 0.0);
        w.tw.Update();
        CHECK(w.tw.Mode == Modes::HIGH);
    }
    SUBCASE("keys: physics warp steps 1x -> 2x, the warning at index 2, 3x, the 1 s lerp and its Time.fixedDeltaTime") {
        v.PlaceAt(10000.0);
        w.tw.Update();  // -> LOW
        REQUIRE(w.tw.Mode == Modes::LOW);
        TimeWarpEnvironment& e = env.twEnv;
        e.TIME_WARP_INCREASE_down = true;
        w.tw.Update();  // index 1 (physics rate 2)
        e.TIME_WARP_INCREASE_down = false;
        CHECK(w.tw.current_rate_index == 1);
        CHECK(w.tw.tgt_rate() == 2.0f);
        // lerp half way: Lerp(1, 2, 0.5) = 1.5, Time.timeScale = 1.5, Time.fixedDeltaTime = 0.02f * 1.5f
        Time::time = Time::time + 0.5f;
        w.tw.FixedUpdate();
        CHECK(TimeWarp::CurrentRate() == 1.5f);
        CHECK(Time::timeScale == 1.5f);
        CHECK(Time::fixedDeltaTime == f32store(fx(0.02f) * 1.5));
        CHECK(TimeWarp::fixedDeltaTime() == Time::fixedDeltaTime);  // LOW: Time.fixedDeltaTime itself
        Time::time = Time::time + 0.6f;
        w.tw.FixedUpdate();
        CHECK(TimeWarp::CurrentRate() == 2.0f);
        // the next step would be index 2 (3x): the first time the warning dialog is shown instead of a rate change
        e.TIME_WARP_INCREASE_down = true;
        w.tw.Update();
        CHECK(e.physWarpWarningShown == 1);
        CHECK(e.paused);
        CHECK(w.tw.current_rate_index == 1);
        w.tw.OnPWarpWarningDismiss();
        CHECK_FALSE(e.paused);
        CHECK(e.settingsSaved);
        CHECK_FALSE(w.tw.showPhyWarpWarning);
        w.tw.Update();  // now index 2
        CHECK(w.tw.current_rate_index == 2);
        CHECK(w.tw.tgt_rate() == 3.0f);
        w.tw.Update();  // 4x
        w.tw.Update();  // clamped at physicsWarpRates.Length - 1
        e.TIME_WARP_INCREASE_down = false;
        CHECK(w.tw.current_rate_index == 3);
        CHECK(w.tw.tgt_rate() == 4.0f);
        // decrease steps down, stop goes to index 0
        e.TIME_WARP_DECREASE_down = true;
        w.tw.Update();
        e.TIME_WARP_DECREASE_down = false;
        CHECK(w.tw.current_rate_index == 2);
        e.TIME_WARP_STOP_down = true;
        w.tw.Update();
        e.TIME_WARP_STOP_down = false;
        CHECK(w.tw.current_rate_index == 0);
        CHECK(w.tw.tgt_rate() == 1.0f);
    }
    SUBCASE("modifier + increase in the flight scene selects physics warp; mode switch refused above maxModeSwitchRate_index") {
        v.PlaceAt(100000.0);
        TimeWarpEnvironment& e = env.twEnv;
        e.MODIFIER_KEY_held = true;
        e.TIME_WARP_INCREASE_down = true;
        w.tw.Update();  // setMode(LOW) at index 0 is allowed -> physics rate index 1
        CHECK(w.tw.Mode == Modes::LOW);
        CHECK(w.tw.current_rate_index == 1);
        e.TIME_WARP_INCREASE_down = false;
        e.MODIFIER_KEY_held = false;
        // back to HIGH needs index <= maxModeSwitchRate_index (0): refused at index 1 with a message
        const size_t before = e.screenMessages.size();
        CHECK_FALSE(w.tw.setMode(Modes::HIGH));
        CHECK(w.tw.Mode == Modes::LOW);
        CHECK(e.screenMessages.size() == before + 1);
        CHECK(e.screenMessages.back().rfind("#autoLOC_6001930", 0) == 0);
    }
    SUBCASE("difficulty parameters and the input lock") {
        TimeWarpEnvironment& e = env.twEnv;
        v.PlaceAt(100000.0);
        e.CanTimeWarpHigh = false;
        e.TIME_WARP_INCREASE_down = true;
        w.tw.Update();
        CHECK(w.tw.current_rate_index == 0);
        CHECK(e.screenMessages.back().rfind("#autoLOC_481056", 0) == 0);
        e.CanTimeWarpHigh = true;
        e.timewarpInputLocked = true;
        w.tw.Update();
        CHECK(w.tw.current_rate_index == 0);
        e.timewarpInputLocked = false;
        Orbit o(0, 0.0, 700000.0, 0, 0, 0, 0, kerbin);
        o.UpdateFromUT(0.0);
        v.orb = &o;
        w.tw.Update();
        CHECK(w.tw.current_rate_index == 1);
        CHECK(w.tw.tgt_rate() == 5.0f);
    }
}

TEST_CASE("TimeWarp.getMaxWarpRateForTravel picks the fastest rate that still leaves time to warp down") {
    TimeWarp tw;
    tw.LoadStockPrefab();
    int idx = -1;
    // too short: stay at 1x
    CHECK(tw.getMaxWarpRateForTravel(5.0, 8.0, 2.5, idx) == 1.0);
    CHECK(idx == 0);
    // 100,000 s with (min = 8 s, max = 2.5 s) in warp (WarpTo's arguments are passed in this order): the rate whose
    // travel time first reaches <= 2.5 s is 100000x (1 s) but that is < 8 s, so the previous one, 10000x (10 s), is used
    CHECK(tw.getMaxWarpRateForTravel(100000.0, 8.0, 2.5, idx) == 10000.0);
    CHECK(idx == 6);
    // 1000 s: 1000/1000 = 1... walk: 5x -> 200, 10x -> 100, 50x -> 20, 100x -> 10, 1000x -> 1 (<= 2.5): 1 < 8 -> 100x
    CHECK(tw.getMaxWarpRateForTravel(1000.0, 8.0, 2.5, idx) == 100.0);
    CHECK(idx == 4);
    // never beyond the last rate
    CHECK(tw.getMaxWarpRateForTravel(1.0e12, 8.0, 2.5, idx) == 100000.0);
    CHECK(idx == 7);
}

TEST_CASE("TimeWarp.WarpTo: autoWarpTo plans, warps and warps down to the target UT") {
    Env env;
    Warp w;
    CelestialBody* kerbin = env.Body("Kerbin");
    KerbinTerrain terrain(kerbin);
    StubVessel v;
    v.body = kerbin;
    Orbit o(0, 0.0, 1300000.0, 0, 0, 0, 0, kerbin);  // 700 km circular: above every altitude limit of Kerbin
    o.UpdateFromUT(0.0);
    v.orb = &o;
    v.PlaceAt(700000.0);
    FlightGlobals::ActiveVessel = &v;
    FlightGlobals::VesselsLoaded = {&v};
    TimeWarpEnvironment& e = env.twEnv;

    const double ut0 = Planetarium::GetUniversalTime();
    const double target = ut0 + 100000.0;
    w.tw.WarpTo(target);
    CHECK(w.tw.setAutoWarp);
    w.tw.WarpTo(target + 1.0);  // not engaged yet: just overwrites the request
    CHECK(w.tw.warpToUT == target + 1.0);
    w.tw.WarpTo(target);

    // Update(): plan (10000x = index 6), raise one step, start the coroutine (control lock WARPTO)
    w.tw.Update();
    CHECK_FALSE(w.tw.setAutoWarp);
    CHECK(w.tw.autoWarpEngaged);
    CHECK(w.tw.AutoWarpRunning());
    CHECK(e.controlLocks.count("TimeWarpTo") == 1u);
    CHECK(w.tw.current_rate_index == 1);  // setRate(Min(current + 1, rateIdx))
    w.tw.WarpTo(target);                   // engaged: refuses (message, no new request)
    CHECK_FALSE(w.tw.setAutoWarp);

    int maxIndex = 0;
    int ticks = 0;
    double overshoot = 0.0;
    bool decelerated = false;
    while (w.tw.AutoWarpRunning() && ticks < 400000) {
        // one Unity frame: FixedUpdate (+ physics), WaitForFixedUpdate continuations, Update, yield-null continuations
        w.tw.FixedUpdate();
        env.planetarium.FixedUpdate();
        Time::time = Time::time + 0.02f;
        w.tw.ResumeAfterFixedUpdate();
        w.tw.Update();
        w.tw.ResumeAfterUpdate();
        maxIndex = std::max(maxIndex, w.tw.current_rate_index);
        if (w.tw.current_rate_index == 0) decelerated = true;
        ++ticks;
    }
    REQUIRE_FALSE(w.tw.AutoWarpRunning());
    overshoot = Planetarium::GetUniversalTime() - target;
    CHECK(maxIndex == 6);  // the planned maximum, never exceeded
    CHECK(decelerated);
    CHECK_FALSE(w.tw.autoWarpEngaged);
    CHECK(e.controlLocks.count("TimeWarpTo") == 0u);
    CHECK(w.tw.current_rate_index == 0);
    CHECK(TimeWarp::CurrentRate() == 1.0f);
    // The game logs "TgtUT Error"; the warp-down ramp is discretised by the 0.02 s tick at up to 10000x, so the error is
    // bounded by one tick at that rate (200 s) and in practice is far smaller
    CHECK(std::fabs(overshoot) < 200.0);
    INFO("UT error " << overshoot << " s after " << ticks << " ticks");

    // CancelAutoWarp while idle does nothing; WarpTo works again afterwards
    CHECK_FALSE(w.tw.CancelAutoWarp());
    w.tw.WarpTo(Planetarium::GetUniversalTime() + 50000.0);
    CHECK(w.tw.setAutoWarp);
}

TEST_CASE("TimeWarp.CancelAutoWarp: stop key and delayed lock release") {
    Env env;
    Warp w;
    CelestialBody* kerbin = env.Body("Kerbin");
    KerbinTerrain terrain(kerbin);
    StubVessel v;
    v.body = kerbin;
    Orbit o(0, 0.0, 1300000.0, 0, 0, 0, 0, kerbin);
    o.UpdateFromUT(0.0);
    v.orb = &o;
    v.PlaceAt(700000.0);
    FlightGlobals::ActiveVessel = &v;
    FlightGlobals::VesselsLoaded = {&v};
    TimeWarpEnvironment& e = env.twEnv;

    w.tw.WarpTo(Planetarium::GetUniversalTime() + 1.0e6);
    w.tw.Update();
    REQUIRE(w.tw.autoWarpEngaged);
    // the stop key during the coroutine: CancelAutoWarp(0, delay_unlock: true): rate 0, lock released one frame later
    e.TIME_WARP_STOP_down = true;
    w.tw.ResumeAfterFixedUpdate();
    e.TIME_WARP_STOP_down = false;
    CHECK_FALSE(w.tw.autoWarpEngaged);
    CHECK_FALSE(w.tw.AutoWarpRunning());
    CHECK(w.tw.current_rate_index == 0);
    CHECK(e.controlLocks.count("TimeWarpTo") == 1u);  // still locked until the delayed callback runs
    w.tw.ResumeAfterUpdate();
    CHECK(e.controlLocks.count("TimeWarpTo") == 0u);
}

TEST_CASE("WarpTransitionCalculator: UT needed to warp down between rate indices (stock rates)") {
    Env env;
    Warp w;
    using C = WarpTransitionCalculator;
    CHECK(C::CheckForTransitionChanges());       // first call computes
    CHECK_FALSE(C::CheckForTransitionChanges());  // unchanged
    REQUIRE(C::WarpRateTransitionPeriods.size() == 8);
    // UTToRateDown[i] = (r[i] + r[i-1]) / 2, UTToRateUp[i] = (r[i] + r[i+1]) / 2, UTTo1Times = running sum of UTToRateDown
    const double down[] = {0.0, 3.0, 7.5, 30.0, 75.0, 550.0, 5500.0, 55000.0};
    const double up[] = {3.0, 7.5, 30.0, 75.0, 550.0, 5500.0, 55000.0, 0.0};
    const double rates[] = {1.0, 5.0, 10.0, 50.0, 100.0, 1000.0, 10000.0, 100000.0};
    double sum = 0.0;
    for (int i = 0; i < 8; ++i) {
        const WarpTransition& t = C::WarpRateTransitionPeriods[static_cast<size_t>(i)];
        sum += down[i];
        CHECK(t.Index() == i);
        CHECK(t.Rate() == rates[i]);
        CHECK(t.UTToRateDown == down[i]);
        CHECK(t.UTToRateUp == up[i]);
        CHECK(t.UTTo1Times == sum);
    }
    CHECK(C::UTToRateDown(7, 0) == 61165.5);
    CHECK(C::UTToRateDown(4, 3) == 75.0);
    CHECK(C::UTToRateDown(3, 3) == 0.0);
    CHECK(C::UTToRateDown(2, 5) == 10.5 - 665.5);  // going the other way is negative
    CHECK(C::UTToRateDown(2, 99) == 10.5);  // unknown target index: nothing subtracted
    CHECK(C::UTToRateDown(99, 0) == 0.0);   // unknown source index: starts from 0
    // current-rate dependent shortcuts (TimeWarp.CurrentRateIndex)
    w.tw.current_rate_index = 4;
    CHECK(C::UTToRateDownOne() == 75.0);
    CHECK(C::UTToRateTimesOne() == 115.5);
    w.tw.current_rate_index = 0;
    CHECK(C::UTToRateDownOne() == 0.0);
    // SafeRateToUTPeriod: the highest rate index whose warp-down fits in `seconds`, plus one (clamped)
    CHECK(C::SafeRateToUTPeriod(0.0) == 0);
    CHECK(C::SafeRateToUTPeriod(0.5) == 1);
    CHECK(C::SafeRateToUTPeriod(3.0) == 1);
    CHECK(C::SafeRateToUTPeriod(3.5) == 2);
    CHECK(C::SafeRateToUTPeriod(100.0) == 4);
    CHECK(C::SafeRateToUTPeriod(115.5) == 4);
    CHECK(C::SafeRateToUTPeriod(115.6) == 5);
    CHECK(C::SafeRateToUTPeriod(1.0e9) == 7);
    // a change of the rate table recomputes; a missing TimeWarp does nothing
    w.tw.warpRates[2] = 20.0f;
    CHECK(C::CheckForTransitionChanges());
    CHECK(C::WarpRateTransitionPeriods[2].UTToRateDown == 12.5);
    w.tw.warpRates.push_back(200000.0f);
    CHECK(C::CheckForTransitionChanges());
    CHECK(C::WarpRateTransitionPeriods.size() == 9);
    CHECK(C::WarpRateTransitionPeriods[8].UTToRateDown == 150000.0);
    TimeWarp::fetch = nullptr;
    CHECK_FALSE(C::CheckForTransitionChanges());
}

TEST_CASE("PositionTarget: a fixed world point, terrain-height placement and the ITargetable answers") {
    Env env;
    CelestialBody* kerbin = env.Body("Kerbin");
    PositionTarget pt("KSC pad");
    CHECK(pt.GetName() == "KSC pad");
    CHECK(pt.GetDisplayName().empty());
    PositionTarget named("a", "b");
    CHECK(named.GetDisplayName() == "b");
    ITargetable& t = pt;
    CHECK(t.GetOrbit() == nullptr);
    CHECK(t.GetOrbitDriver() == nullptr);
    CHECK(t.GetVessel() == nullptr);
    CHECK_FALSE(t.GetActiveTargetable());
    CHECK(t.GetTargetingMode() == VesselTargetModes::Direction);
    CHECK(t.GetObtVelocity().x == 0.0f);
    CHECK(t.GetSrfVelocity().z == 0.0f);
    CHECK(t.GetFwdVector().y == 1.0f);
    CHECK(t.GetFwdVector().x == 0.0f);
    REQUIRE(t.GetTransform() != nullptr);

    // Update(Vector3d): transform.position = (Vector3)position (each component rounded to float)
    pt.Update(Vector3d(1.0e6 + 0.3, -2.5, 123456.789));
    CHECK(t.GetTransform()->position.x == static_cast<float>(1.0e6 + 0.3));
    CHECK(t.GetTransform()->position.y == -2.5f);
    CHECK(t.GetTransform()->position.z == static_cast<float>(123456.789));

    // Update(body, lat, lon, alt): the body's world surface position
    pt.Update(*kerbin, -0.0972, -74.5577, 67.0);
    Vector3f expect = kerbin->GetWorldSurfacePosition(-0.0972, -74.5577, 67.0).ToVector3f();
    CHECK(t.GetTransform()->position.x == expect.x);
    CHECK(t.GetTransform()->position.y == expect.y);
    CHECK(t.GetTransform()->position.z == expect.z);

    // Update(body, lat, lon): altitude = body.TerrainAltitude (0 without a PQS), then the same
    pt.Update(*kerbin, -0.0972, -74.5577);
    expect = kerbin->GetWorldSurfacePosition(-0.0972, -74.5577, 0.0).ToVector3f();
    CHECK(t.GetTransform()->position.x == expect.x);
    CHECK(t.GetTransform()->position.z == expect.z);
    {
        KerbinTerrain terrain(kerbin);
        const double h = kerbin->TerrainAltitude(-0.0972, -74.5577);
        // capture terrain_height scene_probe h_ksc (0x4122500191C4E878 = 600064.7847054144) minus the radius
        uint64_t hb = 0x4122500191C4E878ull;
        double hksc;
        std::memcpy(&hksc, &hb, 8);
        CHECK(h == hksc - 600000.0);
        pt.Update(*kerbin, -0.0972, -74.5577);
        expect = kerbin->GetWorldSurfacePosition(-0.0972, -74.5577, h).ToVector3f();
        CHECK(t.GetTransform()->position.x == expect.x);
        CHECK(t.GetTransform()->position.y == expect.y);
        CHECK(t.GetTransform()->position.z == expect.z);
        // sea: negative terrain is clamped to 0 unless allowNegative
        CHECK(kerbin->TerrainAltitude(0.0, 180.0) >= 0.0);
        kerbin->pqsController = nullptr;
    }
    CHECK(kerbin->TerrainAltitude(-0.0972, -74.5577) == 0.0);
}

namespace {

// A vessel with a rigidbody for the TRACK_Phys / IDLE paths (Vessel.CoMD, velocityD, rootPart.rb).
struct RbVessel final : IOrbitDriverVessel {
    Vector3d com;
    Vector3d vel;
    bool hasRb = true;
    bool kinematic = false;
    int soiLogs = 0;
    Vessel* AsVessel() override { return nullptr; }
    std::string GetDisplayName() const override { return "rb vessel"; }
    Vector3d RotatedLocalCoM() const override { return Vector3d::zero(); }
    void SetPosition(const Vector3d& p) override { com = p; }
    void UnloadAndDestroy() override {}
    Vector3f TransformPosition() const override { return com.ToVector3f(); }
    bool HasRootPartRigidbody() const override { return hasRb; }
    bool RootPartRigidbodyIsKinematic() const override { return kinematic; }
    Vector3d CoMD() const override { return com; }
    Vector3d velocityD() const override { return vel; }
    void ForceVesselLogFlyby(CelestialBody*) override { ++soiLogs; }
};

}  // namespace

TEST_CASE("OrbitDriver: TRACK_Phys follows the rigidbody; offsets, lastMode, lastTrackUT, IDLE, UPDATE hand-over") {
    Env env;
    Warp w;
    CelestialBody* kerbin = env.Body("Kerbin");
    const double mu = kerbin->gravParameter;
    const double r = 700000.0;
    const double vc = std::sqrt(mu / r);

    RbVessel rb;
    rb.com = kerbin->position + Vector3d(r, 0.0, 0.0);
    rb.vel = Vector3d(0.0, 0.0, vc);  // world axes; orbit space is xzy: (0, vc, 0), a circular equatorial orbit
    OrbitDriver d;
    d.vessel = &rb;
    d.name = "rb";
    d.QueuedUpdate = true;  // we drive UpdateOrbit by hand
    d.updateMode = OrbitDriver::UpdateMode::TRACK_Phys;
    env.planetarium.time = 1000.0;
    d.Start();  // referenceBody null -> FlightGlobals.getMainBody(transform position) = Kerbin; TrackRigidbody(-fdtLast)
    REQUIRE(d.referenceBody() == kerbin);
    CHECK(d.Ready());
    // Start's TrackRigidbody uses fdtLast = 0.02: updateUT = UT + (0 - fdtLast)?? -> UT - 0.02 (KSP: rigidbody states are
    // those of the previous physics tick)
    CHECK(d.lastTrackUT == doctest::Approx(1000.0 - 0.02).epsilon(1e-15));
    const Orbit& o = *d.orbit;
    CHECK(o.eccentricity < 1e-9);
    CHECK(o.semiMajorAxis == doctest::Approx(r).epsilon(1e-9));
    CHECK(o.inclination < 1e-6);
    CHECK(o.period == doctest::Approx(2.0 * 3.141592653589793 * std::sqrt(r * r * r / mu)).epsilon(1e-9));
    // pos/vel are the swizzled (Unity-axis) state vectors after TrackRigidbody
    CHECK(d.pos.x == doctest::Approx(r).epsilon(1e-12));
    CHECK(d.vel.z == doctest::Approx(vc).epsilon(1e-12));

    // UpdateOrbit in TRACK_Phys: lastMode; the rigidbody state is back-dated by fdtLast (Start: 0.02, then
    // (double)TimeWarp.fixedDeltaTime of the previous update)
    CHECK(d.lastTrackUT == 1000.0 + (0.0 - 0.02));
    TimeWarp::SetRate(0, true);
    d.UpdateOrbit();
    CHECK(d.lastMode == OrbitDriver::UpdateMode::TRACK_Phys);
    CHECK(d.lastTrackUT == 1000.0 + (0.0 - 0.02));  // fdtLast was still the Start value
    w.tw.curr_rate = 100.0f;
    w.tw.updateRate(100.0f);
    REQUIRE(TimeWarp::fixedDeltaTime() == 2.0f);
    d.UpdateOrbit();  // fdtLast = (double)0.02f from the previous update
    CHECK(d.lastTrackUT == 1000.0 + (0.0 - static_cast<double>(0.02f)));
    d.UpdateOrbit();  // now fdtLast = 2: the rigidbody state is 2 s old (one rails tick)
    CHECK(d.lastTrackUT == 1000.0 - 2.0);
    // offset = false: fdtLast = -0.0, no back-dating
    d.UpdateOrbit(false);
    CHECK(d.lastTrackUT == 1000.0);

    // kinematic rigidbody: not back-dated, and the circular orbit is untouched by the (absurd) velocity
    rb.kinematic = true;
    rb.vel = Vector3d(0.0, 0.0, 1.0e9);
    d.UpdateOrbit();
    CHECK(d.lastTrackUT == 1000.0);
    rb.kinematic = false;
    rb.vel = Vector3d(0.0, 0.0, vc);
    d.UpdateOrbit();  // fdtLast = 2 again
    CHECK(d.orbit->eccentricity < 1e-9);

    // no rigidbody: UpdateOrbit does nothing but record lastMode and fdtLast
    rb.hasRb = false;
    const double t0 = d.lastTrackUT;
    rb.com = rb.com + Vector3d(10.0, 0.0, 0.0);
    d.UpdateOrbit();
    CHECK(d.lastTrackUT == t0);
    rb.hasRb = true;
    rb.com = kerbin->position + Vector3d(r, 0.0, 0.0);

    // IDLE with a kinematic rigidbody: the velocity is the rotating-frame velocity of the orbit (zero in the inertial
    // frame), UT is not back-dated, mode recorded
    {
        RbVessel idleVessel;
        idleVessel.com = kerbin->position + Vector3d(r, 0.0, 0.0);
        idleVessel.kinematic = true;
        OrbitDriver idle;
        idle.vessel = &idleVessel;
        idle.name = "idle";
        idle.QueuedUpdate = true;
        idle.updateMode = OrbitDriver::UpdateMode::IDLE;
        idle.Start();
        CHECK(idle.vel.magnitude() == 0.0);
        CHECK(idle.lastTrackUT == 1000.0);
        idle.UpdateOrbit();
        CHECK(idle.lastMode == OrbitDriver::UpdateMode::IDLE);
        idle.SetOrbitMode(OrbitDriver::UpdateMode::TRACK_Phys);
        CHECK(idle.updateMode == OrbitDriver::UpdateMode::TRACK_Phys);
    }

    // hand-over to rails: UPDATE propagates the orbit from the current UT and moves the vessel
    d.SetOrbitMode(OrbitDriver::UpdateMode::UPDATE);
    env.planetarium.time = 1000.0 + o.period / 4.0;
    d.UpdateOrbit();
    CHECK(d.lastMode == OrbitDriver::UpdateMode::UPDATE);
    CHECK(d.UpdateUT() == env.planetarium.time);
    // the circular orbit was fixed at the last TRACK_Phys update (epoch t0, vessel on +x): a quarter period + (UT - t0 - P/4)
    // later it has turned theta = 2 pi (UT - t0) / P (orbit-space y = world z)
    const Vector3d rel = rb.com - kerbin->position;
    const double theta = 2.0 * 3.141592653589793 * (env.planetarium.time - t0) / o.period;
    CHECK(rel.magnitude() == doctest::Approx(r).epsilon(1e-9));
    CHECK(rel.x == doctest::Approx(r * std::cos(theta)).epsilon(1e-6));
    CHECK(rel.z == doctest::Approx(r * std::sin(theta)).epsilon(1e-6));
}

TEST_CASE("OrbitDriver: TRACK_Phys SOI change re-references the orbit through TrackRigidbody") {
    Env env;
    Warp w;
    CelestialBody* kerbin = env.Body("Kerbin");
    CelestialBody* mun = env.Body("Mun");
    env.planetarium.time = 2.0e6;
    env.planetarium.UpdateCBs();

    RbVessel rb;
    rb.com = kerbin->position + Vector3d(0.0, 0.0, 800000.0);
    rb.vel = Vector3d(std::sqrt(kerbin->gravParameter / 800000.0), 0.0, 0.0);  // circular, orbit-space y -> world z
    OrbitDriver d;
    d.vessel = &rb;
    d.name = "rb";
    d.QueuedUpdate = true;
    d.updateMode = OrbitDriver::UpdateMode::TRACK_Phys;
    d.setReferenceBody(kerbin);
    d.Start();
    REQUIRE(d.referenceBody() == kerbin);
    CelestialBody* changed = nullptr;
    d.OnReferenceBodyChange += OrbitDriver::CelestialBodyDelegate::Lambda(&changed, [&changed](CelestialBody* b) { changed = b; });

    // the vessel is inside the Mun's SOI and moves with it plus 100 m/s along orbit-space x (same frame velocity in the
    // Kerbin frame as the Mun, so relative to the Mun only the 100 m/s remain)
    REQUIRE(mun->sphereOfInfluence > 1.0e6);
    rb.com = mun->position + Vector3d(0.0, 0.0, 0.5 * mun->sphereOfInfluence);
    const Vector3d munVelOrbitSpace = mun->GetFrameVel() - kerbin->GetFrameVel() + Vector3d(100.0, 0.0, 0.0);
    rb.vel = munVelOrbitSpace.xzy();  // TrackRigidbody: velocityD.xzy + rotating-frame velocity (0, inertial)
    d.UpdateOrbit();
    CHECK(d.referenceBody() == mun);
    CHECK(changed == mun);
    CHECK(d.frameShift);           // locked for one second (Invoke "unlockFrameSwitch")
    CHECK(rb.soiLogs == 1);        // ModuleTripLogger.ForceVesselLog(Flyby)
    // relative to the Mun the vessel sits half an SOI radius out, moving at 100 m/s (frame velocities cancel:
    // vel += Kerbin.GetFrameVel() - Mun.GetFrameVel())
    CHECK(d.pos.magnitude() == doctest::Approx(0.5 * mun->sphereOfInfluence).epsilon(1e-12));
    CHECK(d.vel.magnitude() == doctest::Approx(100.0).epsilon(1e-6));
    CHECK(d.orbit->referenceBody == mun);
    CHECK(d.orbit->eccentricity < 1.0);  // 100 m/s at half the SOI is below the Mun's escape speed there (~330 m/s)
    // overrideOrbit suppresses the reference change
    FlightGlobals::overrideOrbit = true;
    d.frameShift = false;
    rb.com = kerbin->position + Vector3d(0.0, 0.0, 800000.0);
    d.UpdateOrbit();
    CHECK(d.referenceBody() == mun);
    FlightGlobals::overrideOrbit = false;
}

TEST_CASE("OrbitDriver::Targetable: vessel, celestial body or nothing") {
    Env env;
    struct TargetableVessel final : IOrbitDriverVessel {
        PositionTarget t{"tv"};
        Vessel* AsVessel() override { return nullptr; }
        std::string GetDisplayName() const override { return ""; }
        Vector3d RotatedLocalCoM() const override { return Vector3d::zero(); }
        void SetPosition(const Vector3d&) override {}
        void UnloadAndDestroy() override {}
        Vector3f TransformPosition() const override { return {}; }
        bool HasRootPartRigidbody() const override { return false; }
        bool RootPartRigidbodyIsKinematic() const override { return false; }
        Vector3d CoMD() const override { return Vector3d::zero(); }
        Vector3d velocityD() const override { return Vector3d::zero(); }
        void ForceVesselLogFlyby(CelestialBody*) override {}
        ITargetable* AsTargetable() override { return &t; }
    } tv;
    OrbitDriver d;
    CHECK(d.Targetable() == nullptr);
    PositionTarget bodyTarget("body");
    d.celestialBody = env.Body("Mun");
    CHECK(d.Targetable() == nullptr);  // the host supplies the body's ITargetable
    d.celestialBodyTargetable = &bodyTarget;
    CHECK(d.Targetable() == &bodyTarget);
    d.vessel = &tv;
    CHECK(d.Targetable() == &tv.t);  // the vessel wins
}
