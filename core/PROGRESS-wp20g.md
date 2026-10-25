# PROGRESS WP-20g: host wiring for a flyable headless core

Branch `wp20g` from local `wp29d` `a46df8d8` (contains `wp20f` and main `aef2f54f`), worktree `D:/Projects/wt-wp20g`.

## Plan (scope items of the package)
1. Part.Pack / Part.Unpack (Part.cs:3747 / :3784) for every part on GoOnRails / GoOffRails, module callbacks (SendMessage OnPartPack /
   OnPartUnpack in module order, onPartPack / onPartUnpack events), Vessel SendMessage OnVesselPack / OnVesselUnpack.
2. GameSettings at KSP's defaults from host start-up (GameSettings.Awake), the test process the same; hygiene restores the defaults.
3. Module start-up (Part.Start's ModulesOnStart / OnStartFinished + first ModuleJettison.LateUpdate) in the host and WP-20d PadScene;
   re-baseline WP-20d numbers.
4. Flight-scene resource-flow host in the bubble (VesselResourceSets per Vessel, ResourceFlowEnvironment::vesselHost,
   Vessel::env.updateResourceSets).
5. Servo modules' FixedUpdate / Update in the module slot.
6. VAB launch onto the real pad (LaunchSiteFacility + PutShipToGround), compare with C6 79.29 m.
7. Host contract table in STATUS.

## Log
- step 0: worktree made, baseline build of wp29d (configure 2.6 s, build 479.0 s, `CMAKE_BUILD_PARALLEL_LEVEL=4`), baseline binary saved
  to the scratchpad for before / after numbers.
- step 1 (2026-10-26): the uncommitted WIP of the first session (items 1-5 code: part_pack.cpp, GameSettings::Awake, StartVesselInFlight,
  VesselResourceHost, servos in the module slot, hygiene defaults) built (no work left) and passed 50 / 50 ctest (212.5 s): committed
  `606186f6`; origin/main dd87d6e8 merged `f2eac17d` (ue5/ docs only).
- step 2: re-baseline attribution. Baseline binary (wp29d a46df8d8, previous session's scratchpad) vs the WIP: only WP-20d's ascent / flight
  messages and WP-20e's pad case moved; a build with PadScene::startModules = false reproduces every baseline number (timings aside), so all
  moves come from item 3 (the module start-up: KSP's drag-cube weights), none from the GameSettings defaults.
- step 3: tests/test_host_wiring.cpp (items 1-4) + GameSettings singleton (fetch guard, instance layoutConfigNodes) `c073ac55`.
- step 4: item 6 tests/test_vab_launch.cpp + SceneryObjectPoses / SceneryFindObject + CheckGroundCollision log format `ebb6ac67`:
  root 79.2776 m vs C6 79.2884 m (-1.08 cm); "putting ship to ground: -6.869718" = KSP.log; CheckGroundCollision moved 0.119 m (KSP 0.101).
- next: STATUS section + host contract table (item 7), README, progress.csv, fresh suite, random orders, WSL syntax, pr_review, PR.
