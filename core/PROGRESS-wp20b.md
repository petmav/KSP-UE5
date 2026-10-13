# PROGRESS WP-20b: vessel rigid bodies on the reduced rigid-body model (branch `wp20b`, from main `b9119103`)

Optimizer-tier package. Scope: `core/PHYSICS-SPIKE.md` section 7 row WP-20b, as adopted by the user 2026-10-12.

## Plan
1. PhysicsWorld (WP-20a glue) additions: compound cluster bodies with explicit mass / inertia / CoM, CoM shift + mass update,
   CoM position, fixed constraints between bodies. Existing API unchanged.
2. `ksp/physics/vessel_physics.hpp`: Part.Start rigid body rules, PartJoint specs (attach, autostrut, strut), cluster build
   (Whole / SplitAtDecouplers + articulation hook), mass / CoM / inertia from WP-14 data, ReadState (CalculatePhysicsStats sums
   in the double frame), part transform sync, ForceAccumulator (Part.AddForce* semantics, FlightIntegrator.Integrate force half),
   angular drag, structural seams (cut / split / merge with momentum-conserving velocities).
3. `VesselPhysicsHost`: Vessel::env hooks (readPhysicsState, rootRigidbody, offsetVelocity, partRigidbody, pack / unpack /
   resumePartVelocity), per-tick Integrate / Flush / AfterStep.
4. Tests: closed forms, WP-14 bit identity, two-cluster dynamics, momentum on decouple, spike regression (S / M2-AS / L2-AS in
   ctest; XL1 / XL2 in a bench), determinism 1/2/4/8 threads, pad at rest 10 s, 80 km orbit coast (physics_orbit_hold).
5. STATUS row 46, README, check_cites, sanitizers, fresh run.

## Log
- start: read optimizer.md, PHYSICS-SPIKE.md, spike sources, STATUS WP-14/18/19a/20a, NEXT-STEPS session 4, C# Part / PartJoint /
  FlightIntegrator / VesselPrecalculate / CModuleStrut. Baseline build started.
- step 1 done: PhysicsWorld glue additions (compound cluster bodies with given mass / inertia / CoM, SetMassProperties via an
  OffsetCenterOfMassShape swap, SetAngularDamping, fixed constraints + impulses, AddStaticMesh, OptimizeBroadPhase).
- step 2 done (first version): vessel_physics.hpp / .cpp / _joints.cpp / _forces.cpp / _host.cpp; Vessel::MOIFromRigidbodies.
- tests (10 cases green, RelWithDebInfo): joint counts == PROFILE.md runtime counts (24/44/168/320/960), PartJoint formulas,
  physicsless parts, closed forms, ReadState == CalculatePhysicsStats bit for bit (S, M2, L2-AS), two-cluster dynamics vs analytic,
  momentum at Cut, spike regression, determinism 1/1/2/4/8 threads, pad at rest 10 s.
- finding: the spike's cluster angular damping (mass-weighted drag on the whole I_c) is much stronger than KSP's per-part
  damping; default is PerPart (KSP), ClusterMassAverage reproduces the spike (S 8 s 2e-5 %, M2-AS -0.09/-0.23 %, L2-AS 0.01 %).
- finding: on a 4 x 4 terrain patch M2-AS limit-cycles on the pad (7 cm/s) where the spike's 30 x 30 patch gives exact rest:
  Jolt's contact order follows body ids. Fixture now builds the spike's 30 x 30 patch (heights cached per World).
- bench tools/wp20b/bench.cpp is built in the spike tree (Release) for timings comparable with the spike's tables.
- tests added: orbit coast (1 orbit: 1.4 mm / 0.33 mm with Krakensbane started as GoOffRails does; 5.2 m without), host wiring
  (GoOffRails / GoOnRails, seams), mass update (CoM shift), forces (Part.AddForce*, Integrate buffers, ForceMode, module records),
  vessel split / merge through Part.decouple / Part.Couple (momentum kept).
- perf: per-part pose cache, flat part index map, bit-identical diagonal-only MOI loop (Vessel::MOIFromRigidbodies 0.036 -> 0.021 ms
  at 457 parts); Release pinned (mask 0x4) ticks: S 0.017, M2-AS 0.112, L2-AS 0.337, XL1-AS 0.788, XL2-AS 1.619 ms (spike same
  session: 0.017 / 0.105 / 0.312 / 0.854 / 1.484).
- fix: World(loadPqs) restores the Simplex / UnityEngine.Random global state (WP-03 SIMPLEX reference failed after it otherwise).
- check_cites exact: 46 cites over the WP-20b files + vessel_motion.cpp, 0 mismatches (aliases added to tools/wp28/check_cites.py).
- docs: STATUS section WP-20b + summary row 46, README (physics paragraph, unit row, layout, tools/wp20b), spikes/jolt/results/wp20b_regression.md.
- verification: fresh MSVC build, ctest 38/38; unit random order seeds 11 and 27: 826/826 each; WSL g++ ASan+UBSan+LSan whole unit binary:
  826/826, no report. Done.
