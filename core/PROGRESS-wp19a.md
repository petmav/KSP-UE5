# PROGRESS WP-19a (flight registry, on-rails propagation, VesselPrecalculate)

Branch wp19a (from main 308a7fe9, WP-20a merged). Worktree D:\Projects\KSP-UE5\core\.worktrees\wp19a.

- [x] FlightGlobals: one registry (`FlightRegistry` is now an alias of `FlightGlobals`); TimeWarp's IFlightVessel view renamed `ActiveVesselView` / `VesselsLoadedView` and refreshed by every registry mutation
- [x] src/celestial/flight_globals.cpp (bodies, gee / Coriolis / centrifugal, pressures, home body, comet perturber ...), src/vessel/flight_globals_vessels.cpp (registry, persistent ids, SetActiveVessel, ClearToSave, UpdateInformation, targets, ease-in)
- [x] VesselPrecalculate (vessel_precalculate.hpp/.cpp): MainPhysics, CalculateGravity, drift compensation (PrecalcMath pure functions), easing, SetLandedPosRot, GoOnRails
- [x] OrbitPhysicsManager (pack / unpack decisions, hold counter); Vessel::Update / LateUpdate / FixedUpdate / CheckKill / OffsetVelocity (vessel_flight.cpp); VesselSwitching; FlightScene (measured script order)
- [x] `VesselPhysicsState` / `readPhysicsState` seam (double-frame CalculatePhysicsStats), `MOI.Zero()` no-op fix (CHANGED vs WP-14)
- [x] tools/wp19a/flight_reference.py -> tests/data/wp19a_reference.json (9 + 1 sections); tests/test_flight.cpp, test_flight2.cpp, test_flight3.cpp, flight_fixture.hpp
- [x] golden_runner dataset `vessel_rails` + ctest `capture.vessel_rails` (0 ULP on every gated column, incl. packed / loaded / update_mode / frames)
- [x] physics_orbit_hold: 6.4 mm over 10 orbits with compensation, 22.8 m radius error per orbit without
- [x] check_cites exact mode: 106 citations, 0 mismatches over the 7 new / ported .cpp files (the 14 old mismatches in vessel.cpp are WP-14's, unchanged)
- [x] Linux GCC 13.3 (WSL, copy in ~/wp19a, linux-gcc-rel): builds without warnings in the new files; WP-19a unit cases and capture.vessel_rails / capture.orbit_rails pass
- [x] STATUS.md section + summary row 40, README rows
- [x] final: core/build deleted + run_tests.ps1 -Fresh: configure 2.3 s, build 240 s, ctest 35/35 (210 s); unit 760 cases / 1,211,729 assertions (WP-19a: 27 / 18,711)
