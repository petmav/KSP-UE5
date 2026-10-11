# PROGRESS WP-38 (FinePrint)

Branch wp38 from main 85b0085f (main 6da79774 merged), worktree .worktrees/wp38.

## Scope decision (coherent subset; see the STATUS WP-38 section)
Ported: Utilities (ProgressUtilities rest, CelestialUtilities all but the PQSMod peak walk, SystemUtilities deviation maths + ShuffleList + LoadNode<CelestialBody>,
OrbitUtilities complete), Waypoint data side + WaypointManager.ChooseRandomPosition/Near/Distance, ProgressTrackingParameter, ExplorationContract.
Deferred: the other 24 FinePrint parameters, 8 other contracts, Sentinel, CompatibilityUtilities, VesselUtilities, rest of SystemUtilities/StringUtilities, full ContractDefs.

## Milestones
- [x] M1 Lite extensions (src/contracts/fineprint_utilities.cpp)
- [x] M2 OrbitUtilities, Waypoint/WaypointManager, ProgressTrackingParameter, ExplorationContract (src/fineprint/*)
- [x] M3 Python reference tools/wp38/fineprint_reference.py -> tests/data/wp38_reference.json
- [x] M4 tests/test_fineprint.cpp (13 cases, 27,984 assertions, 0 ULP)
- [x] M5 cites (0 mismatches), README / STATUS, Windows full run 34/34 (579 cases / 963,143 assertions)
- [x] M6 WSL clang 34/34, unit 579 cases / 963,127 assertions
