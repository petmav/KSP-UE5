# PROGRESS wp32 (EVA kerbal controller)

Branch wp32 from local wp20b (950c1d94). Worktree core/.worktrees/wp32. Local only (no push).

## Plan
1. Port KerbalEVA.cs (all methods), KerbalAnimationManager/State, KerbalRagdollNode, RetractableLadder.cs, FlightEVA.cs, Kerbal.cs,
   CrewTransfer.cs, CrewHatchController.cs behind the seams in include/ksp/eva/eva_engine.hpp.
2. Independent Python reference tools/wp32/eva_reference.py (parses the C# for the FSM tables, hand translation of the formulas) -> tests/data/wp32_reference.json.
3. Tests tests/test_eva*.cpp; STATUS row 53 + section, README, check_cites; sanitizers.

## Log
- port compiles (MSVC RelWithDebInfo): include/ksp/eva/*.hpp, src/eva/*.cpp (8 sources).
