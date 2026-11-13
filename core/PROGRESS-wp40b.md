# WP-40b progress (editor placement loop, EditorLogic KFSM)

Branch wp40b, worktree .worktrees/wp40b, from main d74ae28c.

## Plan
1. editor_types.hpp: scene seams (camera, raycast, pointer-over-UI, audio clip, tweak hooks) in EditorEnvironment.
2. editor_logic.hpp / new src/editor/editor_fsm.cpp: SetupFSM (9 states, 28 events), Update / LateUpdate / FixedUpdate, pickPart,
   dragOverPlane, input updates (sym / snap / undo / delete / offset / rotate), StartEditor (new / load ship), RestoreState tail
   (on_undoRedo, skipPartAttach coroutine), frozen set, gizmo objects (GizmoOffset / GizmoRotate) wired to the callbacks.
3. tools/wp40/session_reference.py: independent Python simulator of the whole loop (KerbalFSM, events, placement, symmetry, undo),
   scripted sessions, seam answers recorded -> tests/data/wp40b_reference.json.
4. tests/test_editor_fsm.cpp: replay (>= 40 sessions) + stock-craft reconstruction test + order-independence of symmetry test.
5. STATUS.md section, README rows, CMake, progress.csv, check_cites, WSL syntax, fresh build, random order.

## Findings
- Tool output elides function bodies when printing py/cs files with sed -n directly; join lines with `tr '\n' '~'` to read them.
- C# `Part.frozen` / freeze() / unfreeze() are not in the C++ Part: kept in EditorLogic (frozenParts_), no shared header edit.

## Done (session 1)
- C++: editor_types.hpp seams, editor_logic.hpp FSM members, src/editor/editor_fsm.cpp (SetupFSM, Update, StartEditor, tools, pickPart, dragOverPlane, input helpers);
  editor_logic.cpp updated (spawn / delete / RestoreState tails, gizmo sync, partRotationResetUpdate). Builds (core + tests), WP-40 tests still green.
- Python: tools/wp40/session_ref_lib.py (simulator), session_reference.py (Sc DSL, s01 only); tests/test_editor_fsm.cpp (replay engine).
## Next
1. get s01 green in C++, then write >= 40 sessions (coverage printed on stderr), Python re-root (isRootCandidate / MakeRoot / SetHierarchyRoot),
   reconstruct-stock-craft test, STATUS / README / CMake / progress.csv, self-verification checklist.
## Notes
- Python reference needs `python -I`; heredocs with apostrophes break: use PYEOF heredocs without apostrophes or the Write tool.
- Part ids in snapshots = creation order (SceneParts sorted by instanceID); trace tags: ev: st: ge: clip: hl: tw: cmd: frame.

## Session 1 end state (gate green)
- 48 sessions pass bit-exact (1535 frames, 5610 trace rows, 83,769 part values, 3,427 seam calls); every state and event reached (coverage in the JSON).
- Rebuild test: PhysProf-S/M/L, Jumping Flea, Ion probe saved trees equal; 7 others attempted, driver refusals listed in the message.
- Python bugs found by the gate (fixed): PartDeleted/PartCreated listeners, gizmo static spaces across sessions, default srf node owner, OnNewRootSelect mask.
- TODO: STATUS/README/progress.csv, cites check (tools/wp28/check_cites.py), WSL g++ syntax, pr_review.py, fresh build + random order, merge main, commit, push, PR.
