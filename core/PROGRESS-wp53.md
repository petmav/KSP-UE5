# PROGRESS WP-53 (Breaking Ground deployed science, ROCs)

Branch wp53 from main af3798c6, worktree core/.worktrees/wp53.

## Data finding
GameData mirror (core/testdata/GameData-mirror) has NO SquadExpansion/Serenity: no ROC_DEFINITION cfgs, no DeployedScience part cfgs,
no DEPLOYEDSCIENCE node, no Serenity ContractDefs section. part_database capture ran with Serenity=False. Only the stock Squad
groundAnchor / groundLight1/2 (ModuleGroundPart) exist. Defaults that live in C# code (diminishing returns, seismic energies, ContractDefs statics) are ported.

## Plan / milestones
- [ ] M0 baseline build
- [ ] M1 ModuleCargoPart header + ground modules (ModuleGroundPart/SciencePart/CommsPart/ExpControl/Experiment)
- [ ] M2 DeployedScience runtime (scenario, cluster, part, experiment, extensions)
- [ ] M3 ROC (ROCDefinition, RocCBDefinition, ROC, ROCManager, ROCScenario, LandClassROC spawn maths, PQSROCControl)
- [ ] M4 contracts (CollectDeployedScience, CollectROCScienceArm/Retrieval, DeployedScienceContract, ROCScienceArm/RetrievalContract)
- [ ] M5 ControlledBase, RoboticCollisions, SerenityUtils
- [ ] M6 Python reference + tests, cites, STATUS/README
