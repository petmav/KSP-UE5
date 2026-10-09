# PROGRESS wp10 (Time warp, rails propagation, orbit driver)

Branch wp10 (worktree .worktrees/wp10), from main f50d6121. Implementer: Sonnet.

## Findings (step 0: reading)
- WP-08 already ported OrbitDriver (minus rendering) with IOrbitDriverVessel seam, TimeWarp subset, capture.orbit_rails gate.
- TimeWarp prefab data (warpRates, altitudeLimits, physicsWarpRates, maxPhysicsRate_index, maxModeSwitchRate_index):
  not in harness/golden; runtime values from analysis/physics/raw/run*/*_summary.csv (rt_TimeWarp.*, 19 runs agree);
  warpRates[0,4,5,6,7] confirmed by capture segments.requested_rate; altitudeLimits*Radius confirmed by body prefab limits.
- Capture C8 segments table carries requested_rate (an input so far), achieved_rate, Time.fixedDeltaTime,
  TimeWarp.fixedDeltaTime, timeScale: all gated; requested_rate can be un-dropped once TimeWarp prefab data exists.
- No capture exists of getMaxOnRailsRateIdx / autoWarp / mode switching: unit-gated, list as "needs".

## Plan
1. FlightGlobals/GameSettings/Time/PQS/CelestialBody additive subsets (ActiveVessel seam, getAltitudeAtPos, getStaticPressure, ORBIT_WARP_*, meshVertMax, pqsController, TerrainAltitude)
2. TimeWarp full port + TimeWarpHost seam (scene, input locks, keys, messages) + autoWarpTo coroutine state machine
3. WarpTransition(+Calculator), ITargetable, PositionTarget, OrbitDriver.Targetable
4. Runner: use TimeWarp::SetRate + prefab data; un-drop segments.requested_rate
5. tests/test_timewarp.cpp; STATUS/README

## Done
- [x] step 1: additive subsets (FlightGlobals IFlightVessel/ActiveVessel/getAltitudeAtPos/getStaticPressure, GameSettings ORBIT_WARP_*, Time::deltaTime, PQS.meshVertMax, CelestialBody.pqsController/TerrainAltitude, VesselTargetModes dedup)
- [x] step 2: time_warp.hpp/.cpp full port (+ TimeWarpEnvironment, autoWarpTo state machine); compiles
- [x] step 3: warp_transition.{hpp,cpp}, targetable.hpp, position_target.{hpp,cpp}, transform.hpp, OrbitDriver::Targetable
- WP-08 test amended: tw.LoadStockPrefab() (updateRate reads warpRates[maxPhysicsRate_index])
- next: runner switch to TimeWarp::SetRate with stock prefab + un-drop segments.requested_rate; tests/test_timewarp.cpp; STATUS/README
- [x] step 4: runner uses TimeWarp::LoadStockPrefab/Start/SetRate (+ PadVessel stand-in); segments.requested_rate un-dropped and gated (capture.orbit_rails still 0 ULP, segments: 5 rows)
- [x] step 5: tests/test_timewarp.cpp written (all pass). Findings: only Kerbin/Minmus/Tylo prefab body limits equal altitudeLimits*Radius, other 14 hand-tuned (Mun 5000 etc.)
- next: wider scrutiny (Debug build, full fresh run), STATUS.md WP-10 section, README rows, commit
- [x] step 6: STATUS/README written; full run green (21/21 ctest; unit 159 cases / 42,082 assertions). Remaining: commit, fresh run, final report
- [x] step 7: fresh full run 21/21 green; unit 159 cases / 42,082 assertions. DONE.
