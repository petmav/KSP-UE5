# PROGRESS WP-21b (branch wp21b, from wp29d a46df8d8)

- [x] worktree D:/Projects/wt-wp21b, fresh build started (core/build, CMAKE_BUILD_PARALLEL_LEVEL=4)
- [x] reading: STATUS WP-29d / WP-20f; part_forces.cpp already runs ModuleLiftingSurface / ModuleControlSurface FixedUpdate in the slot (WP-20d)
- finding: the `no prefab ModuleControlSurface` error is the test World not setting SaveEnvironment::prefabModuleFloat (module IS registered)
- finding: ModuleControlSurface declares its own `new string transformName` (ModuleControlSurface.cs:11): the base field stays empty, so
  baseTransform = the part transform for control surfaces (ModuleLiftingSurface.cs:133); the slot's baseTransform = part transform is right
- [x] code + tests committed (b761824c WIP, 39170a8f scenarios + reference tool, 46c9d69c cite fixes); README section committed; origin/main (dd87d6e8) merged clean (only ue5/ and NEXT-STEPS.md came in)
- [x] VTOL "attitude jump" resolved by measurement (tests/test_wheel_gear.cpp): the vessel does not rotate (< 0.05 deg over 40 ticks); the wheel colliders' part-space pose changes ONCE (tick 0 -> 1) by the gear-bay deployment pose (ModuleWheelDeployment.OnStart samples the Animation at the deployed state; the prefab pose is the stowed one): 5 deg, 0.1 m, all 4 bays
- [x] stock SPH crafts: 24 of 27 load (the 3 others need parts of other mods)
- [x] Aeris 3A on the real KSC runway: rest, roll, rotation, lift-off, climb, SAS torque sum, braking (test_scenery_scenarios.cpp), reference tool tools/wp21b/lift_reference.py (ctest aircraft.reference)
- [x] fresh verification: 52/52 ctest, unit 1073 cases x rs 11 / 27 pass, WSL syntax OK, pr_review 0 failing (numbers in STATUS.md "WP-21b")
- [x] STATUS.md section + summary row 66
- [ ] PR + CI
