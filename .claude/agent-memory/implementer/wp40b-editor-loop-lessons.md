---
name: wp40b-editor-loop-lessons
description: WP-40b editor KFSM port lessons: session-replay gate design, tool output elision, C# quirks the gate found
metadata:
  type: reference
---

- Reading py/cs with `sed -n` can print function bodies as `...`; join lines (`tr '\n' '~'`) or strip CR (`tr -d '\r'`) first. Bash heredocs with apostrophes fail: use PYEOF heredocs without apostrophes or the Write tool.
- Gate pattern that worked for a scene-driven state machine without a capture: an independent Python simulator logs every scene-seam call and its analytic answer; the C++ test replays the answers, asserts the same question and order, and compares per-frame trace plus an FNV hash of a canonical part snapshot. Debug with `session_reference.py --dump <session> <frame>` against the C++ failure text.
- Python-side bugs were as common as port bugs (static gizmo spaces leaking between sessions, listener reactions, layer masks): read the failing trace diff first.
- Default `srfAttachNode` of parts without node_attach has no owner (ReverseSrfNodeDirection no-op); `Part::allParts` includes prefabs (use EditorLogic::SceneParts); full fresh build 620 s, ctest 280-320 s at -j4; ctest can look stalled under load but progresses.
