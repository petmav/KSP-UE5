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
