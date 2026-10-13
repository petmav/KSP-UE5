# PROGRESS WP-52 (Breaking Ground robotics)

Branch wp52 from origin/main cd5e4df4, worktree core/.worktrees/wp52. Build: `CMAKE_BUILD_PARALLEL_LEVEL=6 powershell -File run_tests.ps1` from core/ (full build 213 s, incremental ~20 s).

## Data finding
GameData mirror has NO SquadExpansion/Serenity: no servo part cfgs (hinge / piston / rotor / rotation servo), no controller part. Every MODULE value the
tests give a servo (RESOURCE rate, servoTransformName, hardMinMaxLimits, ...) is a labelled SYNTHETIC fixture; only the C# defaults are real
(`module_schema.inc`, generated from the C# attributes). No in-game capture of a servo exists.

## Decision (joint drive)
The reduced rigid-body model (WP-20b) has one Jolt body per cluster and fixed constraints only: no articulated bodies. The servo logic is ported 1:1 and
writes the ConfigurableJoint properties into `ConfigurableJointData` (servo_joint.hpp), the contract of the seam; `HeadlessServoPlant` is a documented
1-DOF reference plant (not PhysX) for closed-loop tests; `ServoArticulation(PartJointSpec)` is the `VesselPhysicsOptions::isArticulation` hook
(the joint a servo's moving side uses); `BaseServo` is an `IJointLockState` seen by `Part.HasFreePivot` (vessel_physics_joints.cpp). A real Jolt hinge /
slider / motor constraint is the follow-up (needs PhysicsWorld constraint types, WP-20e style).

## Files
- include/ksp/serenity/{servo_joint,base_servo,servo_modules,robotic_controller}.hpp, src/serenity/{servo_joint,base_servo,servo_hinge,servo_piston,servo_rotor,servo_rotation,servo_plant,controlled_axis,robotic_controller,servo_register}.cpp (+ servo_internal.hpp)
- tools/wp52/servo_reference.py -> tests/data/wp52_reference.json, tests/test_robotics.cpp

## Milestones
- [x] M0 baseline build (full 213 s)
- [x] M1 port compiles (joint model, BaseServo, 4 servos, plant, controller, axis / action, manager)
- [x] M2 Python reference + 0-ULP gate for the servo motion math (92 runs, 404,308 comparisons)
- [x] M3 controller sequencer gate (60 runs, 47,854), presets (80 cases, 1,799 keys bit for bit)
- [x] M4 joint seam tests (SetTargetRotationLocal vs double algebra, drive model vs Python twin, articulation hook on PhysProf-S, closed loops on the plant)
- [x] M5 derived cases (27), check_cites exact (289 / 0), docs (STATUS section + row 57, README, NEXT-STEPS)
- [ ] M6 full ctest (48 incl. random order 11 / 27 / 42), push, PR

## Notes
- The mutation check: changing the rotor spool step to Time.deltaTime or the PingPong fold fails the scripted gates on the first scenario.
- Findings kept from the C# are listed in STATUS.md "Findings kept from the C# (quirks)".
