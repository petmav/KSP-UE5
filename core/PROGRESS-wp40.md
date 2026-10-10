# WP-40 progress (editor core)

Branch wp40 (worktree .worktrees/wp40), from main ad208205.

## Done
- Step 1: native quaternion LookRotation / Euler / eulerAngles (include/ksp/unity/quaternion_native.hpp), editor_types.hpp (EditorEnvironment / EditorLimits seams),
  editor_logic.hpp/.cpp (EditorLogicBase + EditorLogic data side), editor_geometry.hpp/.cpp (EditorBounds, EditorGeometryUtil, EditorEnumExtensions,
  filter list, EditorReRootUtil), preflight_tests.hpp/.cpp (PreFlightCheck + all PreFlightTests). Builds.

## Todo
- editor_markers (CoM/CoL/CoT), editor_gizmos (GizmoOffset/GizmoRotate maths)
- tools/wp40/editor_reference.py + tests/data/wp40_reference.json, tests/test_editor.cpp (4 required test groups)
- editor_staging.hpp coherence; check_cites; STATUS / README; full run_tests.ps1 -Fresh; WSL run

## Slice 2 (done)
- editor_markers (CoM / CoL / CoT), editor_gizmos (GizmoOffset / GizmoRotate), tools/wp40/editor_reference.py + tests/data/wp40_reference.json,
  tests/test_editor.cpp: roundtrip (57 crafts, all lines byte-exact except attPos0 / attRot0 re-derived from the world pose), re-root, and the reference
  tests (local pose 3187 parts, symmetry 300 cases / 952 poses, mirror + gizmo 845 + 7320 cases, CoM + CoT 39 crafts) all 0 ULP.
- sinf / cosf in the Python reference come from ucrtbase through ctypes (UCRT sinf is not the correctly rounded value).

## Todo
- pre-flight tests expected outcomes (Python) + C++ test; flow test (checkAttach / createSymmetry / Drop / undo); EditorSession coherence;
  native quaternion / enum / filter / EditorBounds unit tests; remove WP40_TRACE debug; check_cites; STATUS / README; full run; WSL run.

## Slice 3 (state at ~700k tokens)
Done+uncommitted: preflight test, flow test, API tests, EditorLogicStagingHost, README rows, cite fixes, regenerated reference json.
Remaining (coordinator list): 1 commit (rm full_run.log); 2 single Euler/eulerAngles impl (grep WP-24 AutopilotEnvironment::eulerAngles / CommandEnvironment::quaternionEuler) + unverified comments;
3 CoL: leave, STATUS says "aggregation ported, untested against any reference"; 4 full run -Fresh (34/34, read compare_report_*.json of ship_construct/staging_sequence/deltav_stages), WSL run in background -> /tmp/wp40_clang.log;
5 git merge main, rerun unit; 6 STATUS section + summary row 30; check_cites; 7 commit + final report.

## Final slice (done)
Single Euler/eulerAngles impl (commit 841480d5), cite fixes, merge main, Windows -Fresh 34/34, unit 464 cases / 826,521 assertions (WP-40: 11 / 95,023), STATUS section + row 30 written. WSL clang run started in background, log /tmp/wp40_clang.log (WSL side).
