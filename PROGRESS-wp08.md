# WP-08 progress (worktree core/.worktrees/wp08, branch wp08, from main f964447)

## Read so far
- CLAUDE.md, README, STATUS (WP-07/09/integration), work_packages WP-08 rows (alarm clock rows exist but not in assignment)
- harness patched_conics generator: OrbitGen.cs:203-298 (5 cases, patchLimit 6, SolvePatches rehost, pars default)
- harness KspEnv: bodies static (position never set: harness OrbitDriver = orbit.UpdateFromUT only); Planetarium.orbits = 16 body drivers in spawn order
- capture C8 code: decompiled/KSP-Decompiled-profiling/Assembly-CSharp/PhysProf/HarnessCapture2Flight.cs:391-625
  (ut0 provenance; 5 frames after spawn; segments rateIdx {0,4,5,6,7} frames {500,1000,1000,1500,500}; each seg: SetRate + 1 frame, then frames; sample every 10th frame)

## Plan / status
(see below)

## Design (decided)
- New: include/ksp/orbit/{orbit_driver,time_warp,patched_conics,patched_conic_solver,maneuver_node,trajectory}.hpp (+src),
  include/ksp/celestial/flight_globals.hpp (getMainBody/inSOI/overrideOrbit/Bodies ptr; WP-19 shim)
- OrbitDriver::orbit = std::shared_ptr<Orbit> (C# reference); vessel via IOrbitDriverVessel (WP-10/14 Vessel implements)
- Planetarium: std::vector<OrbitDriver*> orbits; static Orbits(); OrbitDriver::Start registers, dtor removes
- CelestialBodyOrbitAdapter owns an OrbitDriver (celestialBody=&body, reverse from prefab, UPDATE)
- Time: +timeScale, fixedDeltaTime; GameSettings: +CONIC_PATCH_LIMIT=3, ALWAYS_SHOW_TARGET_APPROACH_MARKERS=false (GameSettings.cs:999-1000)
- patched_conics runner: UseHarnessFrame + all body positions frozen at 0 (harness never writes CelestialBody.position)
- orbit_rails runner (capture C8): UT from provenance ut0 + Planetarium.fixedDeltaTime*rate (verified 450/450 in python);
  warp rates from segments.requested_rate; Kerbin.directRotAngle = -140.0008910891089 (hidden state, from frames row; verified const);
  scene: PSystemSetup.cs:978-990 (all inverseRotation/reverse false, Kerbin true/true); probes UPDATE drivers;
  exclude world_rel_* (floating origin / vessel transform), packed/loaded (Vessel); soi_events: keep HC8 rows, key (vessel,UT)
- filter_golden.py: add --rows t:col^prefix and --key t:c1,c2 ; RunGolden/ksp_golden_gate ROWS/KEYS

## Status log
- [done] shims (Time, GameSettings, TimeWarp, FlightGlobals), OrbitDriver (full minus rendering), Planetarium.orbits,
  adapter owns OrbitDriver, PatchedConics, PatchedConicSolver, ManeuverNode: compiled clean; 15/15 tests still green
- [next] ds_patched_conics runner + gate; then ds_orbit_rails (C8) + filter ROWS/KEYS; unit tests; Trajectory; docs
- [done] golden.patched_conics: 11 rows, all columns max_abs 0 / ULP 0 (bit-exact). Also bit-exact WITHOUT the position freeze (experiment).
- [done] capture.orbit_rails: initial 6, samples 3465, frames 450, segments 5, soi_events 4 (HC8 rows): ALL bit-exact, 0 missing/extra
  (incl. mun_encounter death after sample 315 via CheckKill stand-in). Excluded world_rel: capture-internal |world_rel - pos.xzy|
  bodies <= 3.725e-9 m (7.9e-17 rel), vessels <= 1.036e-9 m (3.3e-15 rel) = floating-origin rounding.
- [next] full suite, commit checkpoint; unit tests test_patched_conics.cpp; Trajectory port; STATUS/README
- [done] full suite 17/17 green; checkpoint commit 04d00cd on wp08
- (resumed after rate-limit stop; state verified clean at 04d00cd)
- [next] tests/test_patched_conics.cpp; Trajectory port; STATUS/README; final commit + report
- [done] tests/test_patched_conics.cpp: 8 cases, 285 assertions, pass (DeltaV: x radial-out, y normal=Cross(-r,v), z prograde)
- [done] Trajectory port (src/orbit/trajectory.cpp) + test case (89 assertions); quirk: Reframe* throw NRE, tTimes +period after negative fromE
- [next] full suite; STATUS.md WP-08 section; README rows; commit; report
- [done] full suite 17/17 RelWithDebInfo + Debug; unit 107 cases / 19,561 assertions; STATUS WP-08 section + README rows
- [next] final commit + report
- [done] commits 04d00cd, 3028ccb (+ docs hash commit); report sent
