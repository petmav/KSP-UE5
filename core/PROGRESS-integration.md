# PROGRESS: integration WP-07 + WP-09 on main

Start: main at 29b71f6 (14/14 tests). wp07 at b09c31f. merge-base c051578.

## Steps
- [x] 1. merge --no-ff wp07 -> d2e66e7 (15/15 tests, all 0 ULP RelWithDebInfo; unit 64 cases/6563 asserts). golden_json: kept main Normalize.
- [x] 2a. Orbit -> ksp::CelestialBody (drop CelestialBodyRef)
- [x] 2b. BodyOrbit() -> real Orbit
- [x] 2c. ICelestialBodyOrbit adapter over ksp::Orbit
- [x] 2d. PlanetariumFrames -> Planetarium
- [x] 2e. runner StandInOrbitPeriod -> real Orbit via orbitFactory
- [x] 2f. drop COLUMNS on body_catalog/orbits
- [x] 3. integration unit test
- [x] 4. commits f93380a (A), 2fae513 (B), 93cd8ff (C test), e4d982c (D docs)

## Notes
- temp scripts in build/tmp (gitignored): resolve.py, summ.py (gate summary: python -I build/tmp/summ.py [Debug])

## Design decisions (integration)
- Orbit::referenceBody etc -> CelestialBody*; delete celestial_body_ref.{hpp,cpp}. CelestialBody gains getRFrmVelOrbit.
- ICelestialBodyOrbit gains pure `virtual Orbit* GetOrbit()`; BodyOrbit(const CelestialBody*) in orbit.hpp/.cpp = link->GetOrbit().
- Adapter: include/ksp/celestial/celestial_body_orbit.hpp `CelestialBodyOrbitAdapter` (owns ksp::Orbit; ctor = OrbitDriver.Start UPDATE: fields from
  orbitDriver->orbit + referenceBody, Init(), updateFromParameters(); UpdateOrbit = updateFromParameters (moves body / parent if reverse)).
  `MakeCelestialBodyOrbit(CelestialBody&)`; CelestialSystem::Spawn default (nullptr factory) = real orbits.
- One Planetarium: delete PlanetariumFrames; Orbit uses Planetarium::Zup/up()/right()/ZupAtT. Zup static initialised to Awake(0) frame;
  Rotation()/InverseRotAngle() fall back to Awake(0) values when fetch==null; setters throw NRE without fetch.
- Harness quirk: harness CBUpdate rehost omits inverse-rotation Planetarium write (Mun/Minmus prefab inverseRotation=1 -> InverseRotAngle 230,
  Zup rotated). Golden = Awake(0) frame. Runner env must mirror (restore frame + redo UpdateOrbit) -- verify empirically first.
- Harness bodies never move (positions 0); kspcore spawn moves them (updateFromParameters). Not gated now; note for WP-08 patched_conics.
- orbit_propagation runner -> use CelestialEnv (shared, like harness KspEnv) instead of its own BuildBodies.

## Queued by orchestrator (after integration is green + committed)
- merge --no-ff wp03 (event bus, FSM, KSPUtil, Localizer, UtilMath completion). Overlaps: dotnet Math.Min/Max (wp07+wp03; keep one, NaN rules),
  util_math (wp07 AngleBetween via dotnet::Math; wp03 moved ASinh..Coth/Flatten/RoundToPlaces/AngleBetween/BSPSolver): union, math_shim.f64 0 ULP;
  NullReferenceException moved config_node.hpp -> core/exceptions.hpp; root CMake: keep ONE root-finding mechanism (no flags);
  CMakeLists x3, README, STATUS union. Run fresh (wp03 ref test 11,032 assertions). Add to STATUS integration section. Do not delete worktrees/branches.
- Note: heredocs with apostrophes break -> write scripts with the Write tool.

## Status (at the time; since committed)
- All integration code done; 15/15 green RelWithDebInfo, body_catalog orbits 16x25 cols 0 ULP, orbit_propagation 0 ULP.
- Confirmed: without harness-frame mirroring all orbits after Mun's CBUpdate rotated by 130 deg (Zup 230). Fix in runner CelestialEnv::UseHarnessFrame.
- Integration test tests/test_celestial_orbit.cpp (2 cases, 934 assertions) passes.
- Next: Debug run, commits (A lib+tests+runner; B regate orbits: ds_body_catalog.cpp + tests/CMakeLists COLUMNS; C integration test; D docs), then wp03 merge.

## WP-03 merge (done)
- b9d4578 merge wp03; fresh Rel+Debug 15/15, unit 98 cases/19,187 asserts (wp03 ref 11,032), 41 tables 0 ULP.
- Final docs commit records b9d4578 in STATUS; temp files in build/tmp deleted.
