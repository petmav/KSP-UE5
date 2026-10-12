# PROGRESS WP-20c: the reduced model's structural layer (branch `wp20c`, from local `wp20b` 950c1d94; not pushed)

Scope: the WP-20b "Deferred to WP-20c" list (STATUS WP-20b "Seams"): the structural load graph with KSP's PartJoint capacity rule, the
load solve and joint breaking, the real Heaviest autostrut walk, and Cut / SplitOff / Merge / Rebuild wired to the game events with
momentum conservation. Gate: core/PHYSICS-SPIKE.md section 7 row WP-20c, analytic and scenario tests, independent references.

## Plan
1. PhysicsWorld: the parts touching static geometry in the last step (a Jolt contact listener; the ground supports of the load solve).
2. `ksp/physics/structure_graph.hpp`: the elastic sub-joint graph (PhysX's ConfigurableJoint per sub-joint: quasi-rigid linear drive,
   angular drive = PartJoint.stiffness, locked when rigid), part demands, the tree solve (O(parts)), the per-joint sub-joint split,
   the indeterminate graph solve (sparse block Cholesky per structural change, two triangular solves per tick).
3. Capacity: PhysX's per-ConfigurableJoint rule (|f| > breakForce or |t| > breakTorque, PartJoint.cs:340-341), OnJointBreak semantics
   (PartJoint.cs:397: IgnoreCollisionsFrames, onPartJointBreak, child.OnPartJointBreak -> Part.decouple).
4. Autostruts: GetMassivePart port (Part.cs:8342-8509, per-part cache), CycleAutoStrut timing (release now, secure after the next step).
5. VesselPhysics: step snapshot at Flush, loads / breaks in AfterStep, Rebind (one new binding per vessel now holding the parts).
6. Part.cs ports: OnPartJointBreak (:3159), Die (:4409, data side), explode (:4373, data side).
7. VesselPhysicsHost: GameEvents wiring (decouple / undock / couple / part die / vessel modified), deferred rebind, break processing,
   the module force hooks (ModuleDecouplerBase::addForce*, DockingEnvironment::addTorque / partVelocity).
8. Tests + independent references: tools/wp20c/capacity_reference.py (craft files + the in-game part_database capture, numpy float32),
   tools/wp20c/loads_reference.py (dense numpy solve of synthetic structures); analytic stack breaks (size 0/1/2, axial / lateral),
   strut sharing, gravity ramp on the pad, decouple / couple / undock / die momentum, staging L2-AS, determinism 1/2/4/8 threads.
9. STATUS row 50 + section, README, check_cites exact, random order 11 / 27, WSL ASan/UBSan.

## Log
- start: read CLAUDE.md, PHYSICS-SPIKE.md, PROGRESS-wp20b.md, STATUS WP-20b, the spike's reduced.cpp / structure_graph*.cpp /
  articulated.cpp / bench capacity + loads, PartJoint.cs, Part.cs (decouple, Couple, Undock, Die, OnPartJointBreak, autostruts,
  GetMassivePart), CModuleStrut.cs. Worktree core/.worktrees/wp20c created from wp20b; baseline RelWithDebInfo build 284 s.
- steps 1-7 done (commit fa218bb3): StaticContacts in PhysicsWorld; structure_graph (RigidPartDemand, TreeSolve, JointSplit,
  StructuralGraph); GetMassivePart port with the per-part cache (Part fields autoStrutHighMass / CacheVessel / CacheTick); attach /
  autostrut / strut builders split; VesselPhysics: step snapshot at Flush, ComputeLoads in AfterStep, autostrut countdown, welds with
  their impulses in the residual, Rebind; Part::OnPartJointBreak / Part::Die; host events, deferred ProcessStructuralChanges, HandleBreak,
  module force hooks. tools/wp20c/loads_reference.py (26 structures) and capacity_reference.py (in-game part_database capture).
- tests (commit aa857d2e): 15 cases. Reference: demands 3.5e-16, tree 1.1e-15, split 5e-16, graph 2.1e-11 (rel.); capacity table
  1,256 joints / 1,392 sub-joints bit for bit (twice). Analytic payload breaks at 1.000-1.005 a* (size 0/1/2, axial / lateral); strut
  sharing (4 variants) at 1.000-1.005 a*, the joint the elastic split predicts. Pad ramps: S radial decoupler 33.72 g (spike 33.7),
  M2 Mainsail 29.0 g (spike 29.0), M2-AS (graph) radial decoupler 25.14 g (spike graph 25.1), each the joint and load the 1-g static
  solve predicts (within 0.1 %). Events: decouple / ejection hooks / couple / undock / Die, momentum 1e-6. Break chain with and without
  IgnoreCollisionsFrames. L2-AS staging: 48 decouplers, one rebind 1.05 ms (RelWithDebInfo). Determinism 1/1/2/4/8 threads with breaks.
- finding: Jolt's manifold reduction merges the contacts of different sub-shapes of a body pair and reports the first sub-shape only.
  First fix (vessel bodies without manifold reduction) changed the contact solve: the g++ ASan binary's PhysProf-S then crept 1.4 mm in
  10 s on the pad (WP-20b gate 1e-4 m). Final fix (148d6a84): the supports come from OnContactValidate (per sub-shape hit, before the
  merge), manifold reduction stays on: WP-20b's contact numbers are back bit for bit; a compound test checks both resting boxes are seen.
  The PhysProf crafts stand on ONE proxy box (the core Mainsail's): L2-AS rocks on it, its ramp breaks a joint 1.3 % below the static
  prediction of another one.
- finding: the 5 cm placement drop of the scenarios breaks L2-AS's Mainsail joint on impact (a one-step acceleration spike in the rigid
  model): the scenarios use the spike's 60-tick grace (KSP: Vessel.ignoreCollisionsFrames = 60 after its exact landed placement).
- finding: autoStrutExcludeParent defaults to true (Part.cs:829): the Heaviest anchor of a part on the heaviest part is the next one.
- check_cites exact over vessel_physics*.cpp, structure_graph.cpp, part_decouple.cpp, vessel_motion.cpp: 66 cites, 0 mismatches (3
  older WP-16 cites in part_decouple.cpp fixed on the way; aliases CycleAutoStruts / HandleBreak / StartStaging added).
- perf (Release, pinned): the load layer costs ~90 ns (tree) / ~200 ns (graph) per part per tick: L2-AS +0.04 / +0.09 ms on 0.35 ms;
  factorisation 0.24 / 1.14 / 2.2 / 6.4 ms at 153 / 457 / 1,027 / 2,053 parts. Body-frame demands, cached model and scratch buffers did not
  move the numbers beyond the machine's noise (the graph's triangular solves dominate). spikes/jolt/results/wp20c_loads.md.
- articulated comparison (wp20c_loads, the spike's articulated model in the same process; JoltInit / JoltShutdown around it): tree craft
  force within 0.53 % (S) / 0.07 % (M2) of the articulated impulses; strutted craft = the spike's graph solve (median ~20 % off PhysX: R1).
- verification: full ctest 38/38 (before the listener change; rerun after the docs), unit random order 11 / 27: 841 / 841 cases,
  1,395,241 assertions each (after the listener change).
