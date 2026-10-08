// The harness's KspEnv (harness/src/KspGolden/KspEnv.cs) rebuilt with kspcore, shared by the WP-09 datasets
// (physics_constants, body_catalog, atmosphere_curves, physics_drag_curve):
//   PhysicsGlobals: level0 prefab -> physicsDatabaseFilename = harness/inputs/Physics.cfg -> Awake -> LoadDatabase ->
//                   bodyLiftCurve = GetLiftingSurfaceCurve("BodyLift")
//   Planetarium:    level9 values (defaults) + Awake
//   bodies:         CelestialSystem::Spawn(KerbolSystemPrefab()) at UT 0, then PhysicsGlobals.CalculateValues
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
};

// Gate-only stand-in for the WP-07 run-time Orbit: Orbit.period exactly as Orbit.Init computes it
// (Orbit.cs:334 GetMeanMotion, Orbit.cs:370 period = 2*PI / meanMotion for e < 1). Everything else throws: the gate
// uses it only for CBUpdate's tidal lock (rotationPeriod) and solarDayLength. Replace with ksp::Orbit when WP-07 lands.
class StandInOrbitPeriod : public ksp::ICelestialBodyOrbit {
public:
    explicit StandInOrbitPeriod(const ksp::CelestialBody& body);
    double period() const override { return period_; }
    double trueAnomaly() const override;
    double radius() const override;
    double PeR() const override;
    double ApR() const override;
    ksp::Vector3d vel() const override;
    ksp::Vector3d getRelativePositionAtUT(double UT) const override;
    ksp::Vector3d getOrbitalVelocityAtUT(double UT) const override;
    void UpdateOrbit() override {}  // OrbitDriver.UpdateOrbit: propagation is WP-07's

private:
    double period_ = 0.0;
};

// ExactNumber.FormatDouble / FormatFloat (harness/src/KspGolden/Infra/ExactNumber.cs): the text written into `str`
// columns (shortest round-trip G15..G17 / G6..G9 on .NET Framework, "E" -> "e", ".0" appended to integers).
std::string ExactFormatDouble(double v);
std::string ExactFormatFloat(float f);
