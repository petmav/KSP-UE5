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
