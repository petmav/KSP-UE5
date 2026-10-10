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
