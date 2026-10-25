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
