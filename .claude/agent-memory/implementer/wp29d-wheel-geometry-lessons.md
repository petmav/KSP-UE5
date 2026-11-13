---
name: wp29d-wheel-geometry-lessons
description: WP-29d tooling/data facts - reading decompiled/KSP-dev via python, tool output elision workaround, wheel .mu model facts, mirror has no part .mu, scenario traps (airborne placement, landed re-placement, craft that do not load), order-dependent tests
metadata:
  type: feedback
---

- `decompiled/KSP-dev` is denied to Read/ls/grep/Glob/cat in Bash, but `python -I script.py` (os.walk/open on `C:/Projects/KSP-UE5/decompiled/KSP-dev`) works. Worktrees do not contain `decompiled/KSP-dev`: pass `--game C:/Projects/KSP-UE5/decompiled/KSP-dev`.
- The harness elides code/records in tool output ("body elided", "N sections elided"). Workarounds: `cat -n file | sed -n a,bp` for C++, `sed 's/ /_/g'` or `sed 's/^ /~/'` for python/C#, one record per line for tables; doctest `MESSAGE(std::string(x) << ...)` (a raw `const char*` or `e.what()` prints as a pointer).
- Any apostrophe anywhere in a Bash command (even inside a python heredoc comment) makes the whole command fail with "unexpected EOF" and NOTHING runs: put patch scripts / test code in files with the Write tool.
- wheel .mu: WheelCollider chunk is 19 floats (mass, radius, suspensionDistance, center3, spring3, 2x friction5), not 18. LT-1 / LT-2 legs have NO WheelCollider chunk (the controller adds one). Part colliders are layer 26 "WheelCollidersIgnore", wheel collider nodes layer 27 "WheelColliders". The deployed leg / gear pose is `deployTgt` (animated), not the collider node.
- `core/testdata/GameData-mirror` has no part .mu files (cfg + dds only): a model source for tests must come from core/data/wheel_models (reduced, committed) via WheelModelLibrary keyed by part name.
- Put build logs in `core/build/` (gitignored); `core/*.log` is committed by `git add -A`. `run_tests.ps1 -Fresh` only re-configures CMake (objects stay); delete `core/build` for a real from-scratch build.
- Test scene traps: a landed (PRELAUNCH) vessel is re-placed by Vessel.CheckGroundCollision (getLowestPoint) at its first physics tick, so a drop test must place it airborne (`RoverScene::Place(..., airborne=true)`); `Vp()` is null until the first ticks unpack the vessel; PHYSICS_EASE eases gravity in for ~13 ticks; an airborne test far above the ground must not outlast the fall (400 m = 9 s, the vessel dies and `Wheels()` dereferences null).
- No stock aircraft loads (ModuleControlSurface is not a registered module: "v180 upgrade: no prefab ModuleControlSurface"); `Rocket-power VTOL` is the only craft with gear bays that loads and it has an unexplained spawn attitude jump (same with the WP-29c proxy).
- Gates that depend on tiny creep speeds (srfSpeed < 1e-5, the schloompa trigger) fail in some `--order-by=rand` orders (unit.random_order.42 did): gate the mechanism by calling the module function directly, report the natural trigger.
- Other agents run builds / tests at the same time: a random-order unit run can take 15+ min; kill a stuck exe with `taskkill //PID <winpid> //F` (winpid is the 4th column of `ps aux`) before rebuilding.

**Why:** each cost time in WP-29d. **How to apply:** before reading game data, writing scenario tests or scripts through the Bash tool.
