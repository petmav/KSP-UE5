# PROGRESS WP-21b (branch wp21b, from wp29d a46df8d8)

- [x] worktree D:/Projects/wt-wp21b, fresh build started (core/build, CMAKE_BUILD_PARALLEL_LEVEL=4)
- [x] reading: STATUS WP-29d / WP-20f; part_forces.cpp already runs ModuleLiftingSurface / ModuleControlSurface FixedUpdate in the slot (WP-20d)
- finding: the `no prefab ModuleControlSurface` error is the test World not setting SaveEnvironment::prefabModuleFloat (module IS registered)
- finding: ModuleControlSurface declares its own `new string transformName` (ModuleControlSurface.cs:11): the base field stays empty, so
  baseTransform = the part transform for control surfaces (ModuleLiftingSurface.cs:133); the slot's baseTransform = part transform is right
