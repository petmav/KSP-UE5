// The harness's KspEnv (harness/src/KspGolden/KspEnv.cs) rebuilt with kspcore, shared by the WP-09 and WP-07 datasets
// (physics_constants, body_catalog, atmosphere_curves, physics_drag_curve, orbit_propagation):
//   PhysicsGlobals: level0 prefab -> physicsDatabaseFilename = harness/inputs/Physics.cfg -> Awake -> LoadDatabase ->
//                   bodyLiftCurve = GetLiftingSurfaceCurve("BodyLift")
//   Planetarium:    level9 values (defaults) + Awake
//   bodies:         CelestialSystem::Spawn(KerbolSystemPrefab()) at UT 0 with the real run-time Orbit
//                   (CelestialBodyOrbitAdapter), UseHarnessFrame (the harness's CBUpdate skips the Mun's and
//                   Minmus' inverse-rotation frame write), then PhysicsGlobals.CalculateValues
// Created once per process.
#pragma once

#include <memory>
#include <string>

#include "datasets.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/physics/physics_globals.hpp"

struct CelestialEnv {
    std::unique_ptr<ksp::PhysicsGlobals> physics;
    std::unique_ptr<ksp::Planetarium> planetarium;
    std::unique_ptr<ksp::CelestialSystem> system;
    std::string physicsCfgPath;  // as the harness wrote it (Path.Combine, backslashes)

    static CelestialEnv& Get(const RunnerOptions& opt);
    // After the spawn: the harness's Planetarium frame (inverse-rotation write of the first CBUpdate pass undone,
    // orbits re-updated); see celestial_env.cpp.
    static void UseHarnessFrame(ksp::Planetarium& planetarium);
};

// ExactNumber.FormatDouble / FormatFloat (harness/src/KspGolden/Infra/ExactNumber.cs): the text written into `str`
// columns (shortest round-trip G15..G17 / G6..G9 on .NET Framework, "E" -> "e", ".0" appended to integers).
std::string ExactFormatDouble(double v);
std::string ExactFormatFloat(float f);
