# PROGRESS WP-20d: the FlightIntegrator tick over the rigid bodies (branch `wp20d`, from local `wp19b` cd99f577)

Implementer package. Scope: PHYSICS-SPIKE.md section 7 row WP-20d on the WP-20b binding and the WP-19b bubble: the real
FlightIntegrator.FixedUpdate per loaded vessel (FlightIntegrator.cs:544-670) replacing the bubble's minimal stand-in: UpdateMassStats,
SmoothVelocity, atmosphere, DragCubeSetupAndPartAeroStats, thermal (WP-22 port through hooks), the packed branch,
IntegratePhysicalObjects (gravity + drag), the drift-compensation offset, Integrate (force half of WP-20b + UpdateAerodynamics per part in
the recursion); the part-module force slot (engines / RCS / lifting surfaces / reaction wheels -> Part.AddForce* buffers); an SoA aero
kernel bit-identical to the exact path. Ground height / landed: WP-20e. Not touched: WP-20c's files (structure graph, loads, Cut / Merge
internals). Local commits only, no push.

## Plan
1. `physics/flight_integrator.hpp/.cpp`: the FlightIntegrator VesselModule port over VesselPhysics (C# member names), thermal through
   FlightIntegratorThermal with transform / sun hooks, aero through FlightIntegratorAero (exact) or the SoA kernel (`physics/aero_soa`).
2. `physics/part_forces.hpp/.cpp`: Part.AddForce / AddImpulse / AddForceAtPosition / AddTorque buffer semantics (Part.cs:8663-8688) and
   the module FixedUpdate slot (seq 22-276) feeding the records of WP-21 / WP-23 / WP-24 into those buffers.
3. PhysicsBubble: the real FlightIntegrator by default (the stand-in goes), the module slot on FlightScene::hooks.beforeFlightIntegrator.
4. Gates: SoA == exact on every output / part / tick of PhysProf ascents (+ trajectory StateHash identity); the C10 in-game capture
   `aero_forces_vessel` through the SoA kernel at 0 ULP; independent references for the FI's own statements; scenarios ascent / coast /
   burn against independent references; WP-19b / WP-20b suites unchanged or explained.
5. STATUS row 54 + section, README, check_cites exact, random order 11 / 27, fresh build, WSL ASan / UBSan.

## Log
- start: read CLAUDE.md, README, PHYSICS-SPIKE.md, STATUS WP-20b / WP-19b, PROGRESS-wp20b / wp19b, PHYSICS-ENGINE-REVIEW.md 1.2 / 3,
  FlightIntegrator.cs (whole FixedUpdate path), Part.cs:8663-8688, the WP-09 / 21 / 22 / 23 ports, the spike's part_pipeline (SoA kernel).
  Worktree `core/.worktrees/wp20d` created from wp19b cd99f577; baseline build running.
- step 1-3 done (first version, commit dd9c59f9): FlightIntegrator (physics/flight_integrator.*), the SoA kernel (physics/aero_soa.*),
  the part buffers + module slot (physics/part_forces.*), the bubble runs the real FlightIntegrator of every loaded vessel (the stand-in
  is gone), FlightScene::hooks.beforeVesselUpdate (FI.Update / ModuleRCS.Update), StartPartsInFlight (Part.Start's flight block; the
  bubble runs it on onVesselLoaded; FlightIntegrator.Setup waits for `started` like the C#). WP-19b / WP-20b suites green with it
  (two_vessels 0.49 mm, frame_switch 0.27 mm, SOI 4.8e-6 m).
- finding: kspcore has no flight-scene resource-flow host (ResourceFlowEnvironment::vesselHost / Vessel::env.updateResourceSets are
  seams; tests wire VesselResourceSets as test_staging does); engines need it for RequestPropellant.
- finding: a module whose thrust transforms are installed after OnLoad has no multipliers -> ThrustUpdate's .at(i) throws; the slot
  applies OnLoad's 1 / count rule (ModuleEngines.cs:681-697) when the counts differ.
- first full launch: PhysProf-M2 from the pad through the bubble, 17 engines, real thrust transforms, fuel drain, drag, Krakensbane
  engaging: 2.5 km at 230 m/s (KSP C10: 2,502 m at 230.4 m/s).
- independent reference tools/wp20d/fi_reference.py (SmoothVelocity 3,200 steps incl. 417 spike states, IntegratePhysicalObjects 300
  cases): 0 ULP (after fixing the reference's own unary minus: Vector3d.operator- is 0.0 - x, +0 for zeros). Integrate's force half ==
  IntegratePartBuffers bit for bit (21 / 21 parts).
- gates: SoA == exact on 30,600 random part states (every branch; body lift needed PhysicsGlobals.Start's BodyLift curve, which the WP-20b
  World fixture never sets: finding), against the C10 in-game capture (4,224 live + 25,344 AoA states, 0 ULP), over S / M2-AS / L2-AS flights
  from the pad (1,008,969 part-ticks, 0 mismatches, Exact and SoA runs end in the same Jolt hash). FlightIntegrator slot 2.2x faster with SoA.
- ascent vs C10's own in-game states (KSP, SAS on): speed at altitude within 1 % for M2 / L2-AS / S while our AoA is small (to Mach 1 / 5 km;
  past that the crafts without SAS turn away, S tumbles at 9 s as KSP's did). C10's frame 0 is ~26-32 frames before our ignition at equal
  altitudes (pad structure 7.6 m above the PQS terrain); only offset-free quantities gated.
- burn: the first replay missed two KSP facts (findings): drift compensation stays engaged while burning in vacuum (gravity = Kepler step), and
  the CoM moves inside the draining vessel (0.167 m in 10 s): with both, the root part follows the independent replay within 1.7 / 4.4 mm.
- coast: Kepler within 1.8e-5 m over 60 s with thermal running (21 parts radiating). Module slot: Aeris 3A surfaces / reaction wheel records ==
  buffers. FixedUpdate mechanics (Setup / started, packed branch, mirrors, per-part pressure).
- check_cites exact: 26 / 0 (helpers renamed apart and aliased); WP-20b / 19b physics files 71 / 0.
- note: the session scratchpad is shared with other agents (a `rand11.log` of another worktree overwrote mine): WP-20d's files now live in
  scratchpad/wp20d/.
- docs: STATUS row 54 + section, README (tick paragraph, unit row, layout, tools/wp20d).
- random order (first try): WP-19a's FlightGlobals tick case failed for seeds 11 / 27: the speed modes leak from any flight scene with an orbiting
  active vessel; fixed in the hygiene listener (FlightGlobals::ResetSpeedModesForTests) -> 851 / 851 for default / 11 / 27.
- added FlightIntegrator's HookVesselEvents (:400-450) and an Update guard (memory safety while the thermal graph waits for its rebuild), registry
  checks in the tick loops; cites 31 / 0.
- verification: fresh MSVC at 5dcf2a02 38 / 38 (build 254 s, ctest 384 s), final commit ctest 38 / 38; unit default / rand 11 / rand 27 851 / 851
  (1,412,910 assertions); WSL g++ ASan + UBSan + LSan 851 / 851 (1,412,894), no report (note: an incremental WSL rebuild after rsync -a kept stale
  objects of files edited during the first build: touched and rebuilt, then rerun). Done.
- merge with origin/main (WP-20c / WP-19b / WP-12b / WP-29b / WP-32; fixture AddStaticMesh keyed): 2 / 47 ctest failures, `unit.shard.2` and
  `unit.random_order.27` (L2-AS: partTicks 657,511 != 730,743, 1,089 m after 30 s, C10 -24.5 % at 1 km).
- diagnosis (temporary per-tick break / load log over the PadScene launch): not the thrust / aero loads (flight max 0.044 of capacity for L2-AS,
  as WP-20c's 0.046); L2-AS broke at tick 7 of the settle: Mainsail sub-joint 28,861 / 20,000 kN, five radial decouplers 1,912-2,502 / 1,760 kN,
  46 parts lost. Cause: PadScene dropped the craft 5 cm at full gravity (WP-20c's documented impact deviation turns the one-step contact into
  a quasi-static load). The fixture never started the OrbitDriver (OrbitDriver.cs:98: no `ready`, lastMode never IDLE, so no physics easing,
  VesselPrecalculate.cs:290) and did not place the craft as CheckGroundCollision does. Variants: easing only still breaks (27,842 kN), exact
  placement only 0.66, both 0.37 (= the static 1 g load).
- fix in the fixture (OrbitDriver started, lowest proxy point on the pad), new structure checks in the flight / ascent / mechanics cases;
  WP-20d cases 11 / 11 (27,783 assertions).
- `test_flight3.cpp:382` under rand 27: bisected with `--first/--last` to `WP-32 parachute states` leaking `ControlPanelLock_<id>`
  (VesselSwitching.Update returns at the VESSEL_SWITCHING lock); the two cases alone reproduce it. Hygiene listener resets InputLockManager.
