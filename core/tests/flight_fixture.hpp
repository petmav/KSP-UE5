// Shared fixture of tests/test_flight.cpp and tests/test_flight2.cpp (WP-19a): the Kerbol system, Physics.cfg, a clean registry, vessels with no parts.
#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
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
#include "ksp/localization/localizer.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/orbit_physics_manager.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/flight_scene.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_precalculate.hpp"

namespace flighttest {

using namespace ksp;
using json = nlohmann::json;

inline const std::string kHarness = KSPCORE_HARNESS_DIR;

// ---- hex decoding of the reference file (doubles: 16 hex digits, floats: 8)
inline double D(const json& j) {
    const std::string s = j.get<std::string>();
    const uint64_t u = std::stoull(s, nullptr, 16);
    double d;
    std::memcpy(&d, &u, sizeof d);
    return d;
}
inline float Fl(const json& j) {
    const std::string s = j.get<std::string>();
    const uint32_t u = static_cast<uint32_t>(std::stoul(s, nullptr, 16));
    float f;
    std::memcpy(&f, &u, sizeof f);
    return f;
}
inline Vector3d V3(const json& j) { return {D(j[0]), D(j[1]), D(j[2])}; }
inline Vector3f V3f(const json& j) { return {Fl(j[0]), Fl(j[1]), Fl(j[2])}; }

inline bool SameBits(double a, double b) {
    if (std::isnan(a) && std::isnan(b)) return true;
    return std::memcmp(&a, &b, sizeof a) == 0;
}
inline bool SameBits(const Vector3d& a, const Vector3d& b) { return SameBits(a.x, b.x) && SameBits(a.y, b.y) && SameBits(a.z, b.z); }

inline const json& Ref() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp19a_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}


// ---- the world: Physics.cfg, an Awake Planetarium, the Kerbol system (as tests/test_timewarp.cpp), a clean registry
struct FlightEnv {
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    TimeWarp tw;
    TimeWarpEnvironment twEnv;
    FlightEnv() {
        Reset();
        KSP::Localization::Localizer::Shutdown();  // Format(template, params) needs the instance the game always has
        KSP::Localization::Localizer::Init();
        pg.physicsDatabaseFilename = kHarness + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        pg.CalculateValues(sys->Bodies(), planetarium.Sun);
        for (CelestialBody* b : sys->Bodies()) b->inverseRotation = false;
        twEnv.LoadedSceneValue = GameScenes::FLIGHT;
        TimeWarp::environment = &twEnv;
        FlightGlobals::currentMainBody = Body("Kerbin");
        planetarium.fixedDeltaTime = 0.02;
        planetarium.timeScale = 1.0;
        planetarium.time = 1000.0;
        Time::fixedDeltaTime = 0.02f;
        TimeWarp::fetch = &tw;
        tw.warpRates = {1.0f, 5.0f, 10.0f, 50.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f};
        tw.physicsWarpRates = {1.0f, 2.0f, 3.0f, 4.0f};
        tw.maxPhysicsRate_index = 3;
        tw.Mode = TimeWarp::Modes::HIGH;
        tw.curr_rate = 1.0f;
        planetarium.UpdateCBs();
    }
    ~FlightEnv() {
        vessels.clear();
        sys.reset();
        Reset();
        KSP::Localization::Localizer::Shutdown();
    }
    static void Reset() {
        FlightGlobals::Clear();
        FlightGlobals::overrideOrbit = false;
        FlightGlobals::warpDriveActive = false;
        FlightGlobals::env = FlightGlobals::Environment{};
        SaveEnvironment::Reset();
        Vessel::env = VesselEnvironment{};
        OrbitPhysicsManager::env = OrbitPhysicsManager::Environment{};
        FlightScene::hooks = FlightScene::Hooks{};
        TimeWarp::fetch = nullptr;
        TimeWarp::environment = nullptr;
        Time::timeScale = 1.0f;
        Time::fixedDeltaTime = 0.02f;
        Time::time = 0.0f;
        GameSettings::PHYSICS_EASE = true;
        GameSettings::ORBIT_DRIFT_COMPENSATION = true;
        VesselPrecalculate::allowDriftCompensation = true;
        VesselPrecalculate::driftAlwaysPassThreshChecks = false;
        VesselPrecalculate::disableRunInUpdate = false;
    }
    CelestialBody* Body(const char* n) const { return sys->GetBodyByName(n); }

    std::vector<std::unique_ptr<Vessel>> vessels;
    // A vessel with no parts, on an orbit around `body` (the unloaded, packed probe of the C8 capture when loaded == false)
    Vessel* MakeVessel(const std::string& name, CelestialBody* body, double sma = 700000.0, double ecc = 0.0, bool loaded = false, bool packed = true,
                       OrbitDriver::UpdateMode mode = OrbitDriver::UpdateMode::UPDATE, bool register_ = true) {
        auto v = std::make_unique<Vessel>();
        v->vesselName = name;
        v->id = Guid::Empty();
        v->persistentId = static_cast<uint32_t>(1000 + vessels.size());
        v->orbitDriver = std::make_unique<OrbitDriver>();
        v->orbitDriver->vessel = &v->OrbitDriverView();
        v->orbitDriver->orbit = std::make_shared<Orbit>(0.0, ecc, sma, 0.0, 0.0, 0.0, Planetarium::GetUniversalTime(), body);
        v->orbitDriver->updateMode = mode;
        v->orbitDriver->Start();
        if (mode != OrbitDriver::UpdateMode::UPDATE) {
            // TRACK_Phys / IDLE Start() fit the orbit to the (still empty) vessel state: give it back the requested Kepler orbit
            v->orbitDriver->orbit = std::make_shared<Orbit>(0.0, ecc, sma, 0.0, 0.0, 0.0, Planetarium::GetUniversalTime(), body);
        }
        v->lastBody = body;
        v->packed = packed;
        v->loaded = loaded;
        v->vesselType = VesselType::Probe;
        v->staticPressurekPa = 0.0;  // (the constructor measured the pressure at the origin, inside the body)
        Vessel* raw = v.get();
        if (!loaded) {  // an unloaded vessel always has the proto vessel Unload() makes (crew, snapshots, orbit)
            raw->loaded = true;
            raw->Unload();  // (GoOnRails packs it)
            raw->packed = packed;
        }
        vessels.push_back(std::move(v));
        if (register_) FlightGlobals::AddVessel(raw);
        return raw;
    }
};

}  // namespace flighttest
