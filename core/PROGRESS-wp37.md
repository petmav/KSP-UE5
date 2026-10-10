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
