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
