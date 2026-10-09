# PROGRESS WP-14 (Vessel model, situations, mass properties)

Branch wp14 (from wp13 688c5711). Notes written after every major step.

## Plan / design decisions
- New dirs: include/ksp/vessel/, src/vessel/. Vessel is a plain C++ class (no GameObject); Part::vessel stays the opaque `Vessel*`.
- Seams (documented in STATUS): FlightCtrlState (WP-24: minimal struct + Save/Load/Neutralize), ActionGroupList (WP-16: Load/Save only),
  DiscoveryInfo (data side, assigned to WP-09 but absent), VesselPrecalculate (WP-19: isEasingGravity, CalculatePhysicsStats mass maths),
  PartSet / resources (WP-15: not touched), FlightGlobals (WP-19/WP-10 seams kept), CommNet connection (WP-39: stub), PatchedConicSolver (WP-08 exists).
- Vessel::Situations = existing VesselSituations; complete the opaque Vessel_Situations of game_event_types.hpp by aliasing.
- Gate: harness/captures/out2/aero_forces_vessel.json structure (part tree) + states (atmosphere-derived), runner dataset vessel_state.
- Unit tests: tests/test_vessel.cpp.

## Status
- [x] baseline fresh build of wp13 tip started (log /tmp/claude-0/wp14/baseline_build.log)
- merged wp13 (ae3e81fa) into wp14; check class/struct fwd decl matches
- written: vessel_types, flight_ctrl_state, action_group_list, discovery_info, vessel_values, vessel.hpp, vessel.cpp, vessel_motion.cpp; TODO vessel_transitions/proto, trip logger, flight logger, com tracker, axis groups module, CMake, build, tests, gate
- lib builds (all vessel sources); baseline wp13 tests: see /tmp/claude-0/wp14/baseline_test.log
- gate capture.vessel_state passes (structure 631 rows, states 20 rows, 0 ULP); next: tests/test_vessel.cpp, STATUS, README, fresh run
- tests so far: types, ctrl state, action groups, discovery, craft vs C10 structure, physprofile round trip (1335/1337 MODULE nodes; 2 unported OnSave). TODO tests: situations, kinematics, mass/CoM, crew/control, rails/load/unload, naming, trip logger, flight logger, tracker, axis groups, target info, values; then STATUS/README, merge-check, fresh run
- tests: situations, mass/CoM, kinematics, accel, atmosphere, crew/control, overrides, naming, transitions, reference transform, positioning (18 cases). TODO tests: trip logger, flight logger, tracker, axis groups, target info, values, FlightRegistry; then STATUS/README, fresh run
- tests complete (24 cases); citation pass done (/tmp/claude-0/wp14/tools/cite_check*.py); TODO: STATUS.md section, README rows, fresh full run, final report
- STATUS.md WP-14 section, README rows written; fresh run 22/22 passed (197 unit cases / 147,884 assertions). Remaining: final commit, handback.
- gatekeeper follow-ups: states.spd dropped (input), consistency docs, CalculatePhysicsStats deviations, VESSELMODULES note, stale comments, 2845 citation; 22/22

## Integration: merge wp13 (with main: WP-06, WP-10) into wp14, unify ITargetable (2026-10-09)
- Merge in progress; STATUS.md/README resolved by the orchestrator (kept). Tree did not compile: two ITargetable definitions.
- Decision: keep WP-10's ITargetable (ksp/orbit/targetable.hpp, GetTransform() -> UnityEngine::Transform*). Drop the WP-14 copy in
  vessel_types.hpp. UnityEngine::Transform gains `name` and `rotation` (GameObject name, Transform.rotation).
- Vessel::GetTransform(): a Transform owned by the Vessel, refreshed from ReferenceTransform() (name/position/rotation) on each call.
- FlightCoMTracker: owns its Transform (trf) directly; FixedUpdate writes trf.position.
- IFlightVessel: Vessel cannot inherit it (fields altitude/geeForce/srf_velocity clash with the interface's functions, orbit()
  returns Orbit& vs Orbit*), so an adapter VesselFlightView like VesselOrbitDriverView; Vessel::FlightView().
- Done: ITargetable unified (vessel_types copy removed; Vessel/FlightCoMTracker/ProtoTargetInfo/TModule on GetTransform()); Transform
  stand-in has name + rotation; PositionTarget's GameObject named "New Game Object"; VesselOrbitDriverView::AsTargetable -> Vessel;
  VesselFlightView (IFlightVessel) + IEvaController::OnALadder; tests added in test_vessel.cpp.
- Fresh runs: linux-rel (clang 18) 22/22, linux-gcc-rel (GCC 13) 22/22, all 429 gate columns max_ulp=0; unit 214 cases / 149,925
  assertions (test_vessel.cpp 24 / 6,901). STATUS.md unit row, WP-14 section (deviations: two registries left for WP-19) and README updated.
- Next: merge commit (no push).
- After merging main with WP-15: 23/23 ctest, every gate 0 ULP; unit 236 cases / 178,437 assertions.
