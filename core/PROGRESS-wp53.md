# PROGRESS WP-53 (Breaking Ground deployed science, ROCs)

Branch wp53 from main af3798c6, worktree core/.worktrees/wp53. Build: `CMAKE_BUILD_PARALLEL_LEVEL=6 powershell -File run_tests.ps1` from core/.

## Data finding (for the final report)
GameData mirror (core/testdata/GameData-mirror) has NO SquadExpansion/Serenity: no ROC_DEFINITION cfgs, no DeployedScience part cfgs, no
DEPLOYEDSCIENCE node, no Serenity ContractDefs section, no Serenity ScienceDefs experiments. part_database capture ran with Serenity=False.
Real data used: stock groundAnchor / groundLight1 (ModuleGroundPart, Squad), ScienceDefs.cfg (temperatureScan, mysteryGoo), Traits.cfg
(DeployedSciencePowerSkill), the real biome maps of Kerbin / Mun / Minmus / Duna (names), the physprofile save (ProtoVessel with a
ModuleScienceContainer). Everything Serenity-specific in the tests (MODULE values, ROC_DEFINITION nodes, quad meshes) is a fixture.
Defaults living in C# code are ported: DeployedScience diminishing returns / seismic energies / delays, ContractDefs Serenity statics.

## Files
- include/ksp/modules/module_cargo_part.hpp + src/modules/module_cargo_part.cpp (ModuleCargoPart moved out of module_behaviours.cpp)
- include/ksp/serenity/{serenity_env,ground_modules,deployed_science,roc,serenity_misc}.hpp, src/serenity/*.cpp
- include/ksp/contracts/serenity_contracts.hpp, src/contracts/serenity_contracts.cpp
- tools/wp53/serenity_reference.py -> tests/data/wp53_reference.json (numpy, independent), tests/test_serenity.cpp

## Milestones
- [x] M0 baseline build, M1 ground modules, M2 deployed science runtime, M3 ROC, M4 contracts, M5 misc (ControlledBase, RoboticCollisions, SerenityUtils)
- [x] M6a reference + 0-ULP tests (power, dimret, science accumulation, seismic maths, ROC seeding + placement, KSPRandom picks)
- [x] M6b module / e2e / ROC manager / contract tests (committed)
- [ ] M6c remaining tests: seismic integration, booster power / comm unloaded, GetInfo traits, sun tracking, ControlledBase
- [ ] M7 check_cites.py (exact), STATUS.md section + summary row, README entry, fresh build from scratch (delete core/build), push branch, report

## Notes
- UnityEngine.Random.rotation consumption (4 draws) and Range(int) draws are shared assumptions with WP-01's capture-fitted model (C2).
- ROC prefab object name = prefabName + "(Clone)" is UNVERIFIED (needs the asset bundle).
- ValidateCBBiomeCombos keeps the C# quirk (index past the list after RemoveAt) -> std::out_of_range, tested.
- ROCScenario.Instance is never assigned in C# (sic), kept.
