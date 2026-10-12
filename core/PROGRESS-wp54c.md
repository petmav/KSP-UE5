# PROGRESS wp54c (Making History mission runtime, slice 3) - branch wp54c, worktree core/.worktrees/wp54c, base main b9119103

Build: `export CMAKE_BUILD_PARALLEL_LEVEL=6; powershell -NoProfile -ExecutionPolicy Bypass -File run_tests.ps1 -BuildOnly` from the worktree core dir
(helper: C:/Users/pmavr/AppData/Local/Temp/claude/D--Projects-KSP-UE5/9035e596-1691-418f-908a-4d9deca60056/scratchpad/b.sh builds, prints errors).
Unit binary: `build/RelWithDebInfo/tests/kspcore_tests.exe` (baseline 810 cases / 1,366,529 assertions; `-tc="*WP-54b*"` etc.).
Python: `python -I` (not python3). Cites: write `// File.cs:@Name[#k]` placeholders then `python -I tools/wp55/check_cites.py --resolve --cs-root D:/Projects/KSP-UE5/decompiled/KSP-Decompiled/Assembly-CSharp file.cpp`, then the check without --resolve.

## Plan (checked = ported + gated)
- [x] baseline build; test world moved verbatim from test_missions2.cpp into tests/missions2_world.hpp (namespace ksp::mtest54b; WP-54b gate unchanged: 4 cases / 40,219 assertions)
- [~] A Mission craft-file bookkeeping + ProcessMappedVesselNodeMap + vessel event handlers: src/missions/mission_craft.cpp (flag MissionsEnvironment::craftFileBookkeeping keeps WP-54 gates bit-identical; seams MissionDataEnvironment::directoryExists/getFiles, MissionsEnvironment::refreshMissionRequested). CODE COMPILES, NOT YET GATED.
- [~] B ActionPartFailure / ActionPartRepair / ActionPartResourceDrain: mission_actions3.{hpp,cpp}; seams in NodeLibraryEnvironment (findUnloadedPart, partAddModuleAdjusters, partModuleNames, partModuleAdjusterIds, partRemoveModuleAdjuster, adjusterCalls, partRequestResource, partModuleDisplayName, getSpaceCenterFacility, getLaunchSite, launchSiteDisplayName); ProtoPartSnapshot::ProtoPartRepair + ProtoPartModuleSnapshot::Add/RemovePartModuleAdjusterNode added in save/; DynamicModuleList::Clone now concrete. COMPILES, NOT GATED.
- [~] C TestDistance: mission_tests4.{hpp,cpp}, mission_system.hpp (static missions list only so far). COMPILES, NOT GATED.
- [ ] D MissionScoreInfo, MissionLocalizer, MissionImporting, MissionEditorHistory
- [ ] E GetHashCode overrides (ScoreRange, VesselPartIDPair, SurfaceArea, SurfaceVolume, MissionOrbit, MissionInstructor, MissionKerbal, VesselGroundLocation, MissionBiome, DynamicModule name hash)
- [ ] F real BaseAPField / BaseAPFieldList (legacy stand-in members kept)
- [ ] G PositionMobileLaunchPad (scene seams)
- [ ] H MissionSystem (engine-agnostic subset; SetupMissionGame/SetupFlight/ConstructShip are scene-bound: deferred or seam-driven)
- [ ] I ActionCreateVessel scene branch (spawnCraftVessel) + validateCraft
- [ ] Gate: tools/wp54c/*.py -> tests/data/wp54c_reference.json; tests/test_missions3.cpp (shares tests/missions2_world.hpp)
- [ ] registry accounting (tools/wp55 node_library_reference.py: deferred names -> ported), STATUS section + row 48, README, check_cites, random order -rs=11/27, ASan/UBSan in WSL, fresh build

## Design notes
- Gate model = tools/wp54b (ops lists per case, scenarios synthetic). New python modules under tools/wp54c import ref_common from tools/wp54b via sys.path.
- craft bookkeeping gate: in-memory FS (symbolic prefix SHIPS/ for Mission::ShipsPath()), counter for FlightGlobals.GetUniquepersistentId via MissionMappedVessel::getUniquePersistentId hook, vessels = real ActionCreateVessel actions whose VesselSituation fields (persistentId, craftFile, vesselName) are set directly.
- C# quirks kept: ActionPartRepair.Save calls vesselPartIDs.Load(node) after Save; TestDistance.Load tests configNode2 for the DISTANCETOLOCATION load; TestDistance Vessel body text passes distanceFromVesselID to UpdateFromMappedVesselIDs; RunValidation `distanceFromKerbal == null` never true; Mission.OnDestroy leaks onVesselDocking/onVesselsUndocking subscriptions (C++ removes them).

## Findings
