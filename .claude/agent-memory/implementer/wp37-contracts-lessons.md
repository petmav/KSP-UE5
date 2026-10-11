---
name: wp37-contracts-lessons
description: WP-37 contracts port: KSP quirks ported faithfully, test-scaffolding facts, tool pitfalls (heredoc apostrophes, stale exe, check_cites subpaths)
metadata:
  type: project
---

- Contract.AddKeywords(params) adds ONLY the first keyword not yet in the list (KSP bug, ported); ContractSystem.OnSave writes `update` to the SCENARIO node but OnLoad reads it from CONTRACTS (not restored); LoadContract removes the `type` values from the node it loads (copy before a round trip); Contract.SetState calls Register() BEFORE storing the new state, so parameters that test `Root.ContractState == Active` in OnRegister (AcquireCrew, RecoverKerbal, ...) only subscribe on a later Register while Active.
- TextGen needs GameDatabase-translated STORY_DEF (Localizer.TranslateBranch before TextGen::Setup(node)); the Lingoona managed fallback cuts a parameter at '^' (body displayName "The Mun^N").
- `x = float*float` stored into a C# double is the exact double product (C7 rule): SetExpiry/SetDeadline* TimeExpiry/TimeDeadline are NOT float-rounded; Mathf.Round(arg) arguments are.
- Tests: World37 fixture (core/tests/test_contracts.cpp) = Game15 + Kerbol system + CAREER game with flightState + localized body names (CelestialBody.Awake) + ContractDefsLite + StoryDefs + AgentList + ProgressTracking; deterministic Guid via ContractsEnvironment::newGuid. Progress scenario is loaded with ProgressTracking::OnLoad(Progress node, `completed = 0` per node id).
- Tool pitfalls: bash heredocs with an apostrophe break (use Write for any file text); a python replace with "\\n" inside a heredoc can inject real newlines; ALWAYS check the build log before running the old exe (a failed build leaves the previous binary, I debugged a stale exe for 4 rounds); nlohmann json `j[0]` literal ambiguity.
- check_cites.py now accepts `// Templates/PlantFlag.cs:N` (sub path) to disambiguate same-named C# files (Parameters/PlantFlag vs Templates/PlantFlag).
- Part fields live in `part->fields.X` (flightID, launchID, flagURL optional<string>, mass, CrewCapacity, stagingOn); ProtoCrewMember::name() is optional<string>; ProgressNode is ksp::ProgressNode (not KSPAchievements::).
**Why:** each cost a build/debug cycle in WP-37. **How to apply:** before porting contract/FinePrint code or writing test fixtures.
