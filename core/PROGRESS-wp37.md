# PROGRESS WP-37 (contract system core and stock templates)

Worktree C:\Projects\KSP-UE5\.worktrees\wp37, branch wp37 (from main 7199d110, wp36 merged as e7649104).

## Facts found while reading (step 0)
- No contract golden or capture in harness/golden/MANIFEST.json or harness/captures/out*/ (no ContractSystem data at all).
- physprofile/persistent.sfs (sandbox) has NO ContractSystem SCENARIO (and none of the career scenarios): byte-exact round trip of a real
  ContractSystem node is impossible from repo data; career save needed (documented in STATUS).
- Contracts.cfg / Agents.cfg / StoryDefs.cfg / dictionary.cfg are in the GameData mirror (core/testdata). ContractDefs (WP-38) reads
  Contracts.cfg; here a thin ContractDefsLite reads the sections the WP-37 files use.
- Contract.Generate seeds UnityEngine.Random with the mission seed; SuperSeed(contract) uses Guid.NewGuid() (non-deterministic): env seam.
- ContractSystem.ContractTypes order = assembly TypeDef order (unknown; needs a career save WEIGHTS node): registration order here.

## Plan / slices
1. core: enums, ContractParameter/Predicate/Contract, Agents + mentalities, TextGen, ContractSystem (weights, generate, save/load), simple parameters
2. remaining parameters, predicates, PartTestConstraint, ModuleTestSubject, ScenarioCustomWaypoints + ProtoWaypointInfo
3. templates (8)
4. tests + python reference, STATUS / README

## Steps
- Slice 1 (done, commit d97885a6): include/ksp/contracts/{contract,contract_environment,contract_system,agents,text_gen,parameters,predicates,
  module_test_subject,fineprint_lite,templates,contracts}.hpp + src/contracts/*.cpp. ContractDefsLite (Contracts.cfg), ProgressMilestone /
  ProgressUtilities / CelestialUtilities ported into ksp::Contracts::Lite (WP-38 will replace). Templates so far: PlantFlag, CollectScience.
  check_cites.py extended: `// Templates/PlantFlag.cs:N` (sub path) disambiguates same-named files. cites: 0 mismatches.
- Windows build OK (run_tests.ps1 -BuildOnly).
- Next: tests/test_contracts.cpp + tools/wp37/contract_reference.py, waypoints, remaining 6 templates, STATUS/README, WSL run.
- Slice 2 (done, commit e2eea151): RecoverAsset + PartTest templates (src/contracts/templates_recover.cpp, templates_parttest.cpp), WaypointManager seam +
  ScenarioCustomWaypoints (waypoints.cpp; ProtoWaypointInfo data side is WP-04's, only Make/Find helpers added), TemplateEnvironment seams,
  tests/test_contracts.cpp (15 cases, 13065 assertions, all green) + tools/wp37/contract_reference.py -> tests/data/wp37_reference.json
  (rng, rewards 126, expiry, mentality 228 rows, counts, weights, text 189, trees 480 [284 generated], agents, part_constraints).
  Findings: Contract.AddKeywords adds only the FIRST new keyword (KSP bug, ported); ContractSystem update value is saved to the SCENARIO node
  but loaded from CONTRACTS (not restored); LoadContract mutates the node (removes `type`); TextGen needs GameDatabase-translated STORY_DEF.
  Python reads GameData-mirror cfgs + celestial_bodies.serialized.json; GameVariables contract factors = C# initialisers (needs data: prefab).
- NOT DONE yet: GrandTour, VesselRepairContract, OrbitalConstructionContract, RoverConstructionContract templates (need WP-38 FinePrint
  parameters: VesselSystemsParameter, VesselDestinationParameter, LocationAndSituationParameter, RepairPartParameter, ConstructionParameter,
  RoverWayPointParameter, CrewTraitParameter -> factory seam), tests for parameters / ModuleTestSubject / constraints / PartTest+RecoverAsset
  templates, STATUS.md section, README rows, WSL run, full run_tests.ps1 -Fresh.
- Slice 3 (done): GrandTour (factory seams for the WP-38 parameters), tests for OR/XOR/KerbalDeaths/Reach*/Acquire/Recover, PartTestConstraint over 75 parts, ModuleTestSubject,
  predicates, PartTest/RecoverAsset/GrandTour pieces; header citations snapped (tools/wp37/fix_header_cites.py); STATUS section + README rows; check_cites 0 mismatches.
- Found with MSVC ASAN (/fsanitize=address, build/asan, wp37_asan.ps1 outside the repo): my World37 teardown left ProgressTracking nodes subscribed (call pt->OnDestroy()); fixed, ASAN clean on all WP-37 cases + WP-25 docking.
- main (dcef6d17, wp36 merged) merged into wp37 (f7a12b85): Windows fresh-equivalent run 34/34, unit 565 cases / 935,110 assertions (WP-37: 29 cases / 29,484).
- WSL clang run started (log /tmp/wp37_clang.log in WSL); STATUS "Verification" placeholder to fill with its numbers.
- Deferred: VesselRepair / OrbitalConstruction / RoverConstruction templates (WP-38 dependencies; see STATUS).
