# PROGRESS wp29c (native WheelCollider + vessel integration) - branch wp29c, worktree core/.worktrees/wp29c, base origin/main cd5e4df4

Scope: WHEEL-SPIKE.md section 7 row WP-29c, option (A): the PhysX 4.1 suspension law as a small native WheelCollider in kspcore under the
WP-29a/29b managed port; wheels as force elements on the WP-20b cluster bodies, wired into the WP-19b/20d tick; physics-warp substeps of
0.02 s for wheeled vessels (decision delegated by the user, the spike's recommendation). Not Jolt's WheeledVehicleController.

- [x] read: CLAUDE.md, core/README.md, WHEEL-SPIKE.md, PROGRESS-wheelspike, PROGRESS-wp29a/b, STATUS WP-29a/29b/20b/20d, the spike's rover.cpp
- [x] PhysX 4.1 source (PxVehicleUpdate.cpp, PxVehicleSuspLimitConstraintShader.h; scratchpad only, not committed): processSuspTireWheels
      (:3203-3625), computeSuspensionRaycast (:1109), PxVehicleWheels4SuspensionRaycasts (:7255), intersectRayPlane (:2273), computeTireDirs /
      computeTireSlips (:1946-2043), integrateNoDriveWheelSpeeds (:4389), updateNoDrive (:6519), the susp-limit row (shader :130-142)
- [ ] decompiled/KSP-dev is NOT readable in this session (permission denied): the .mu wheel geometry cannot be extracted here -> drag-cube
      proxy (the spike's rule) behind a geometry seam; real-geometry gates deferred
- [x] native WheelCollider (include/ksp/wheels/native_wheel_collider.hpp, src/wheels/native_wheel_collider.cpp)
- [x] independent Python reference tools/wp29c/suspension_reference.py -> tests/data/wp29c_reference.json; tests/test_wheel_collider.cpp
- [x] shared-file seams (minimal, separate blocks): PhysicsWorld::RayCastIgnoring, VesselPhysicsOptions::massOnly + BufferedForces,
      PhysicsBubble::physicsStep
- [x] vessel integration (include/ksp/physics/vessel_wheels.hpp, src/physics/vessel_wheels.cpp): WheelPhysicsHost / VesselWheels
- [x] substep policy + test
- [x] scenarios (tests/test_vessel_wheels.cpp): settle ride height, accelerate, brake, steer, no jitter, 4x substeps, determinism
- [x] full suite, random order 11/27, ctest, check_cites
- [x] STATUS section + row 56, README row, NEXT-STEPS notes; commit, push, PR (not merged)

## Log
- 2026-10-13: native law written from the PhysX statements, Python reference first run: 2,000 cases 0 ULP on the first build.
- 2026-10-13: integration findings: (1) kspcore leaves Part.packed alone in GoOnRails / GoOffRails (wheelSetup's WaitUntil never fired): the
  host mirrors Part.Pack / Unpack's flag + the wheel modules' OnPartPack / OnPartUnpack; (2) GameSettings' wheel block is 0 until
  SetDefaultValues (integrationSteps 0: no tyre force): the fixture sets KSP's defaults; (3) Part.Update's PartValues refresh (overrideSkillUpdate
  for ModuleWheelSteering, Part.cs:1915 / :4328) is needed for CoMfwdLength (steering sign): the host runs it for wheel parts.
- 2026-10-13: scenarios on the stock Prospector Rover / Bug-E Buggy reproduce the spike's numbers (rest 0.26 / 0.34 mm drift, compression
  0.509 / 0.531, drive 19.42 / 7.95 m/s at 10 s, brake 7.38 / 8.30 m) and the warp findings (4x without substeps: 2.5 / 12.5 m drift); with
  substeps 2x-4x equal 1x. 13 WP-29c cases, 5,630 assertions, 18 s.
- 2026-10-13: first full ctest 47/48: unit.random_order.11 crashed (0xc0000374) in the pre-existing WP-39 run-alone case "CommNetVessel control
  state" (a VesselModule made before any PartModule came out as the base class); fixed in VesselModuleFactory::Create (registers the built-in
  behaviours first). check_cites (wp28 + wp55) on every touched file: 59 cites, 0 mismatches (5 stale WP-14 cites in vessel_module.cpp and
  one comment form in the steer test fixed). ctest 48/48 passed (153.7 s); unit 959 cases / 1,548,340 assertions pass in default order and
  with --order-by=rand -rs=11 / -rs=27.
