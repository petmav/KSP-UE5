# PROGRESS WP-19a (flight registry, on-rails propagation, VesselPrecalculate)

Branch wp19a (from main 308a7fe9, WP-20a merged). Worktree D:\Projects\KSP-UE5\core\.worktrees\wp19a.

- [x] FlightGlobals: one registry (`FlightRegistry` is now an alias of `FlightGlobals`); TimeWarp's IFlightVessel view renamed `ActiveVesselView` / `VesselsLoadedView` and refreshed by every registry mutation
- [x] src/celestial/flight_globals.cpp (bodies, gee / Coriolis / centrifugal, pressures, home body, comet perturber ...), src/vessel/flight_globals_vessels.cpp (registry, persistent ids, SetActiveVessel, ClearToSave, UpdateInformation, targets, ease-in)
- [x] VesselPrecalculate (vessel_precalculate.hpp/.cpp): MainPhysics, CalculateGravity, drift compensation (PrecalcMath pure functions), easing, SetLandedPosRot, GoOnRails
- [x] OrbitPhysicsManager (pack / unpack decisions, hold counter); Vessel::Update / LateUpdate / FixedUpdate / CheckKill / OffsetVelocity (vessel_flight.cpp)
- [x] Build + existing suite green after the refactor (2 expected test edits: TimeWarp srf_velocity, MOI.Zero() no-op)
- [ ] tools/wp19a/flight_reference.py -> tests/data/wp19a_reference.json
- [ ] tests/test_flight.cpp (registry, precalc, decisions, situations)
- [ ] golden_runner dataset vessel_rails (real Vessels, FlightScene tick) vs orbit_rails_ingame.json
- [ ] physics_orbit_hold scenario (drift compensation, loaded vessel, hooks)
- [ ] check_cites exact; STATUS.md section + row 40; README; fresh run
