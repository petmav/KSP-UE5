---
name: gotchas-core-port
description: Pitfalls hit while porting modules into core/ (bash heredocs, null string KSPFields, ModuleData reallocation, part names, WP-17 delta-v capture traps)
metadata:
  type: feedback
---

- Bash tool heredocs containing a single quote (apostrophe, e.g. in a log string or a C++ comment) fail with "unexpected EOF"; write source files with the Write tool, not `cat <<EOF` or `python - <<'E'` blocks that contain prose.
- Tool output is lossy-compressed by the harness (`... N sections elided (caveman) ...`) when it has blank-line separated repeated records: print results as ONE line per call (join with ` || `) or grep/cut; the file on disk is intact.
- Windows python cannot open `/tmp/x` that git-bash created; write scratch files relative to the cwd (and delete them) or pass paths as arguments.
- `run_in_background` with a trailing `&` inside the command returns immediately (stale "completed" notices); run builds in the foreground (incremental build ~2 min, fresh ~2 min, WSL fresh >30 min -> use a Monitor on its log).
- String KSPFields without a C# initialiser are null after AddComponent (WP-06): `Get<std::string>` throws; check `IsNull(field)` first (module_structural.cpp `StrField`).
- `Part::AddModule` returns a pointer into `ModuleData` (a vector): a second AddModule invalidates it; re-fetch through `FindModuleOfClass`.
- Stock part names turn `_` into `.` (cfg `liquidEngineMini_v2` is `liquidEngineMini.v2`).
- Incremental Windows build+test: `run_tests.ps1 -BuildOnly`, then `build/RelWithDebInfo/tests/kspcore_tests.exe -tc="..."` for one doctest case; `-TestFilter <ctest regex>` for one gate; the golden runner can be run directly (`golden_runner.exe <dataset> --harness ../harness --out build/RelWithDebInfo/candidate`) and compared with `harness/compare/compare.py`.
- Delta-v capture (WP-17) replay needs editor state that no package ports yet: crew mass in Part.mass (first control-source part filled, 0.045 + 0.049 inventory per kerbal), CompoundPart.OnStartComplete for fuel lines, ModuleDockingNode `part.fuelCrossFeed = crossfeed`, `SaveEnvironment::loadedScene = EDITOR` (Part flow routes STAGE_PRIORITY_FLOW through the ship only in the editor scene).
- The `.mu` files in `core/testdata/GameData-mirror` are 0-byte stubs and `decompiled/KSP-dev` is denied to the agent: no model reader data (thrust transforms) is available.

**Why:** each cost a build/debug cycle in WP-26 / WP-17. **How to apply:** before writing module code, test files or analysis scripts.
