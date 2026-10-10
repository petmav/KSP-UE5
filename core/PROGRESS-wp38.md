# PROGRESS WP-38 (FinePrint)

Branch wp38 from main 85b0085f, worktree .worktrees/wp38.

## Scope decision (coherent subset, see STATUS WP-38 section when written)
Ported: Utilities (ProgressUtilities rest, CelestialUtilities all but PQSMod peak walk, SystemUtilities deviation maths + ShuffleList + LoadNode<CelestialBody>,
OrbitUtilities complete), Waypoint data side + WaypointManager.ChooseRandomPosition/Near/Distance, ProgressTrackingParameter, ExplorationContract.
Deferred: the other 24 FinePrint parameters, 9 other contract templates, Sentinel, CompatibilityUtilities, VesselUtilities (rest), SystemUtilities (rest), StringUtilities site names.

## Milestones
- [x] M1 Lite extensions (src/contracts/fineprint_utilities.cpp), build green
- [x] M2 OrbitUtilities, Waypoint/WaypointManager, ProgressTrackingParameter, ExplorationContract (src/fineprint/*), build green
- [ ] M3 Python reference tools/wp38/fineprint_reference.py -> tests/data/wp38_reference.json
- [ ] M4 tests/test_fineprint.cpp
- [ ] M5 cites (check_cites), README/STATUS, full Windows + WSL verification, merge main
