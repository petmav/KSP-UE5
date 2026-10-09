// The planetary system: the PSystem prefab (KSP's stock Kerbol system as shipped), its spawn into live CelestialBody
// objects (PSystemManager.SpawnBody / PostSetup), and the FlightGlobals body list (FlightGlobals.Bodies, GetHomeBody,
// GetBodyByName).
//
// Prefab data: KerbolSystemPrefab() is generated (src/celestial/kerbol_system_data.cpp, tools/gen_wp09_data.py) from
//   harness/golden/serialized/celestial_bodies.serialized.json  every serialized CelestialBody/OrbitDriver field, extracted
//                                                               from decompiled/KSP-dev/KSP_x64_Data/sharedassets9.assets
//   core/data/kerbol_body_assets.json                           biome maps (attribute tables), mini-biomes, PQS maps,
//                                                               scaled-space mesh/textures, by origin (same asset files)
// Values are the exact float-derived doubles the game deserializes (e.g. Kerbin sma 13599840256, GeeASL
// 1.0003416049313476), never the rounded wiki numbers.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/celestial/celestial_body.hpp"

namespace ksp {

class Planetarium;

// One PSystemBody of the PSystem prefab (PSystemBody.cs) with its CelestialBody and OrbitDriver components.
struct CelestialBodyPrefab {
    int32_t spawnOrder = 0;   // depth-first PSystem order (PSystemManager.SpawnBody recursion)
    int32_t depth = 0;
    std::string parent;       // parent bodyName, empty for the root (Sun)
    int32_t flightGlobalsIndex = -1;
    bool planetariumCameraInitial = false;
    GameAssetRef psystemBody;  // origin of the PSystemBody / CelestialBody components
    CelestialBody body;        // serialized CelestialBody fields (+ BiomeMap, MiniBiomes, assets)
    std::optional<CelestialBodyOrbitDriver> orbitDriver;  // serialized OrbitDriver (elements, reverse, updateMode)
};

// PSystem.cs (the prefab root)
struct PSystemPrefab {
    std::string systemName;
    double systemScale = 1.0;
    double systemTimeScale = 1.0;
    std::vector<CelestialBodyPrefab> bodies;  // spawn order
};

// The stock system (17 bodies), generated data. Origin: KSP_x64_Data/sharedassets9.assets (PSystem prefab).
const PSystemPrefab& KerbolSystemPrefab();

// Spawned bodies + FlightGlobals.Bodies. Owns the bodies and the run-time orbit links.
class CelestialSystem {
public:
    // Creates the run-time Orbit for a body with an OrbitDriver (OrbitDriver.Start: orbit.Init(); updateFromParameters()).
    // The default is MakeCelestialBodyOrbit (celestial_body_orbit.hpp: ksp::Orbit behind CelestialBodyOrbitAdapter).
    // A factory may return null: CelestialBody.orbit then stays null, which KSP never has for a body with an OrbitDriver
    // (tidal locking and solarDayLength take their `orbit == null` branches; unit tests use it).
    using OrbitFactory = std::function<std::unique_ptr<ICelestialBodyOrbit>(CelestialBody& body)>;

    // The in-game start-up sequence for `prefab` on `planetarium` (which must be Awake):
    //   PSystemManager.SpawnBody depth-first (instantiate = copy, Awake, wire parent/children, Sun/Home/CurrentMainBody),
    //   PostSetup (FlightGlobals.Bodies sorted by flightGlobalsIndex, maximumSurfaceGeeASL), CelestialBody.Start for every
    //   body, OrbitDriver.Start (orbitFactory; null = MakeCelestialBodyOrbit) in spawn order, then one
    //   Planetarium.UpdateCBs() at the current UT (the first FixedUpdate's CBUpdate pass, without advancing time; it moves
    //   every body to its orbit position and, for the stock prefab, runs the Mun's and Minmus' inverse-rotation branch,
    //   see STATUS.md "Integration WP-07/WP-09"). PhysicsGlobals.CalculateValues (OnFlightGlobalsReady) is the caller's.
    // Afterwards body->getPositionAtUT(UT) / getTruePositionAtUT(UT) / GetFrameVelAtUT(UT) and BodyOrbit(body) work.
    static std::unique_ptr<CelestialSystem> Spawn(const PSystemPrefab& prefab, Planetarium& planetarium,
                                                  const OrbitFactory& orbitFactory = nullptr);

    const std::vector<CelestialBody*>& Bodies() const { return bodies_; }        // FlightGlobals.Bodies
    const std::vector<CelestialBody*>& SpawnOrder() const { return spawnOrder_; }  // PSystemManager.localBodies before sort
    CelestialBody* GetHomeBody() const;                          // FlightGlobals.cs:1813
    CelestialBody* GetBodyByName(const std::string& name) const;  // FlightGlobals.cs:1752
    double maximumSurfaceGeeASL() const { return maximumSurfaceGeeASL_; }  // PSystemManager.PostSetup

    CelestialSystem(const CelestialSystem&) = delete;
    CelestialSystem& operator=(const CelestialSystem&) = delete;
    ~CelestialSystem();

private:
    CelestialSystem() = default;
    void SpawnBody(CelestialBody* parent, const PSystemPrefab& prefab, size_t index, Planetarium& planetarium);
    std::vector<std::unique_ptr<CelestialBody>> owned_;
    std::vector<std::unique_ptr<ICelestialBodyOrbit>> orbits_;
    std::vector<CelestialBody*> spawnOrder_;
    std::vector<CelestialBody*> bodies_;
    double maximumSurfaceGeeASL_ = 0.0;
};

}  // namespace ksp
