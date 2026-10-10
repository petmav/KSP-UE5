# PROGRESS WP-36 (funds, reputation, strategies, facilities, recovery, progress tracking)

Worktree C:\Projects\KSP-UE5\.worktrees\wp36, branch wp36 (from main ad208205).

## Facts found while reading (step 0)
- No funds/progress golden or capture in harness/golden/MANIFEST.json or harness/captures/out*/ (checked names).
- testdata save `physprofile/persistent.sfs` (sandbox) SCENARIOs: ProgressTracking (empty `Progress {}`), ScenarioAchievements (no body),
  ScenarioDestructibles (intact flags), VesselRecovery (no body). No Funding / Reputation / ScenarioUpgradeableFacilities / StrategySystem.
- GameVariables prefab curves (reputationAddition/Subtraction) and UpgradeableFacility upgradeLevels (levelCost) are Unity-serialized
  in scene/prefab assets; not in harness/golden/serialized (needs data). `GetActiveVesselMaxReputation` does not exist in GameVariables.cs.
- Strategies.cfg is in core/testdata/GameData-mirror/GameData/Squad/Strategies/ (612 lines).
- FlightGlobals is a stub (WP-19 pending): no GetHomeBody; CelestialBody has no progressTree field (add).
- ProgressUtilities / ContractDefs / AnalyticsUtil / MessageSystem are other WPs (38 / 59 / GUI): seams.

## Steps
(see git log; updated after each step)
- Step 1 (done): headers include/ksp/career/*.hpp + src/career/*.cpp written and compiling (currency, funding/reputation, GameVariables,
  upgradeables, progress + achievements, strategies, scenario_misc: ScenarioAchievements/ScenarioDestructibles/VesselRecovery).
- Next: tests/test_progress.cpp, tools/wp36/currency_reference.py + tests/data/wp36_reference.json, save round-trip test, check_cites,
  STATUS/README, WSL run.
