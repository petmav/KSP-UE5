# PROGRESS wp32 (EVA kerbal controller)

Branch wp32 from local wp20b (950c1d94). Worktree core/.worktrees/wp32. Local only (no push).

## Plan
1. Port KerbalEVA.cs (all methods), KerbalAnimationManager/State, KerbalRagdollNode, RetractableLadder.cs, FlightEVA.cs, Kerbal.cs,
   CrewTransfer.cs, CrewHatchController.cs behind the seams in include/ksp/eva/eva_engine.hpp.
2. Independent Python reference tools/wp32/eva_reference.py (parses the C# for the FSM tables, hand translation of the formulas) -> tests/data/wp32_reference.json.
3. Tests tests/test_eva*.cpp; STATUS row 53 + section, README, check_cites; sanitizers.

## Log
- port compiles (MSVC RelWithDebInfo): include/ksp/eva/*.hpp, src/eva/*.cpp (8 sources).
- tests: 6 test files, 32 cases / 29,059 assertions pass; Python reference tools/wp32 (families fps, movement, heading, pid, pack, conditions, decisions, helmet, visor, impulse, lowg, clamber, seatcam, rules, ragdoll, ladder, scenarios).
- full unit suite MSVC: 858 cases / 1,396,119 assertions; --order-by=rand -rs=11 and -rs=27 pass.
- mutation check: flipping a guard in SetupFSM fails test_eva_conditions (gate is sensitive).
- check_cites exact: 334 cites, 0 mismatches (NAME_ALIAS additions in tools/wp28/check_cites.py).
- STATUS row 53 + "WP-32" section, README entries written.
- deferred: see STATUS "WP-32" (physics bubble WP-19b/20d, animation rig, UE5); untested beyond tables: FlightEVA hatch/spawn, CrewTransfer flows, CrewHatchController, RetractableLadder extend/retract, Kerbal start/die.
