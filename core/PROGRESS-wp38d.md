# PROGRESS WP-38d (comet / asteroid package, ship-spawning seams), branch wp38d from main aef2f54f

Scope found on reading STATUS: items 1, 2 and 4 of the task (the rest of the FinePrint contracts / parameters, ContractDefs, Sentinel, CompatibilityUtilities, the three
WP-37 templates) were ported in WP-38b and WP-38c (merged). What is left of WP-38 is the seams: item 3 (CometManager, CometOrbitType, CometDefinition,
DiscoverableObjectsUtil, ScenarioDiscoverableObjects; ContractSystem.ConstructShip / SpawnPreBuiltShip; CraftProfileInfo fields).
Build: CMAKE_BUILD_PARALLEL_LEVEL=4 powershell -File run_tests.ps1 -BuildOnly ; tests: build/RelWithDebInfo/tests/kspcore_tests.exe -tc="WP-38d*"
D: has ~2.7 GB free (parallel agents): a link failed once with LNK1116 (error 112 = disk full); retry.

- [x] worktree D:/Projects/wt-wp38d, baseline fresh build (481 s)
- [x] include/ksp/fineprint/space_objects.hpp, src/fineprint/space_objects.cpp (CometDefinition, CometOrbitType, CometManager)
- [x] src/fineprint/discoverable_objects.cpp (SpaceObjectNodes = ProtoVessel.CreateVesselNode / CreatePartNode / CreateDiscoveryNode with the real draws,
      DiscoverableObjectsUtil, ScenarioDiscoverableObjects, SpaceObjects::InstallSentinelSeams); SentinelEnvironment::discoverableObjectsProvider
- [x] tools/wp38d/space_objects_reference.py -> tests/data/wp38d_reference.json (472 KB, byte reproducible)
- [x] tests/test_space_objects.cpp part 1 (manager data, generators, orbits, names)
- [x] part 2 (ScenarioDiscoverableObjects spawners / behaviour, default AddVessel, Sentinel seams, CometManager events / fragments): 15 cases / 18,858 assertions green
- [x] ConstructShip / SpawnPreBuiltShip (prebuilt_spawn.*), CraftProfileInfo fields; Game::GetCometNumberedName dangling reference fixed
- [x] cites snapped, STATUS section + row 65, README rows, progress.csv, main dd87d6e8 merged, g++ -fsyntax-only clean, pr_review 0 failing
- [x] fresh verification (core/build deleted; ctest 50/50, 353 s build; unit 1083 cases / 1,677,635 assertions at -rs=11 and -rs=27), commit, push, PR
