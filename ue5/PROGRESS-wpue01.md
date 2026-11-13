# PROGRESS-wpue01 (WP-UE-01: bridge rebuild on the Jolt-linked core)

Worktree `C:\Projects\KSP-UE5\.worktrees\wpue01`, branch `wpue01` from main da6db948.

## Done (written, not yet built unless noted)
- core: `include/kspcore/kspcore_test.h`, `src/capi/kspcore_test.cpp` (test ABI: pile scenario hash, MXCSR probe, Jolt config probe),
  `src/CMakeLists.txt` (kspcore_test.cpp in kspcore, `file(GENERATE)` link manifest `<build>/kspcore_link.txt`),
  `tests/capi/capi_jolt_step.c` + ctest `capi.jolt_step` (writes `<build>/kspcore_jolt_step.txt`).
- ue5: `KspCoreBridge.Build.cs` reads the manifest; `FKspFpGuard` (MXCSR 0x1F80) + facade test wrappers; tests moved/added in
  `Source/KspUe5/Tests/KspBridgeTests.cpp` (LinksCore, FpGuard, JoltStep).

## Plan / todo
1. Core build finishes (CMAKE_BUILD_PARALLEL_LEVEL=4, log in %TEMP%/wpue01_core_build0.log); regenerate (manifest), build capi_jolt_step, run ctest -R capi.
2. test.ps1: run the ctest twin first. Genproj (new source file), build editor + game, 0 warnings.
3. Run automation twice; clean builds; timings.log; STATUS/README/DESIGN 1.6; core README/STATUS short rows; progress.csv WP-UE-01.
4. Self-verification list from the brief (core fresh suite, g++ -fsyntax-only in WSL, pr_review.py), fetch/merge origin/main, push, PR.

## Notes
- Core Release build 676 steps; contention with other agents.

## Update (continuation)
- Verified in UE 5.8 UBT sources: ModuleRules.FPSemantics (ModuleRules.cs:784), PrivateDefinitions, ExternalDependencies exist.
- core/README.md rows added. Waiting for first core build (run 0 started before CMake edits); then run a second build for test ABI + manifest.
- Core built (571 s cold). Manifest verified (kspcore.lib, Jolt.lib at third_party/jolt/, define=JPH_DOUBLE_PRECISION). capi.jolt_step passes (hash 0x0be3de1d6affe387 at 200 steps). MXCSR sticky flag bits made 0x1FA0 -> compare control bits 0xFFC0.
- Next: genproj, UE editor+game builds, automation x2.
- UE editor build (first in worktree, 171 s) and game build (137 s) rc=0, no warning/error lines. Automation x2: 5/5 pass both runs (LinksCore, FpGuard, JoltStep, Float32VsLwc, CubeSpherePatch). JoltStep: UE hash 0x0be3de1d6affe387 == ctest hash. First-ever full run in the new worktree hung (frame counter stalled, no test started, ~6 min) and I stopped it; not reproduced in 3 single-test runs + 2 full runs afterwards (observation only).
- Next: clean builds of both targets for the checklist, docs, core suite, WSL syntax, pr_review, progress.csv, merge main, push, PR.
- Clean builds: editor 158.6 s, game 144.1 s, 0 'warning' lines. Automation after clean builds, 2 runs, 5/5 pass each. docs README/DESIGN done.
- Todo: STATUS.md, core/STATUS.md short section, progress.csv, core fresh suite, WSL syntax, pr_review, merge main, push, PR.
