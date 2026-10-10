---
name: wp39-commnet-lessons
description: Facts from porting CommNet (WP-39): test scaffolding order, event-handler leaks, scheduler quirk, cfg reference filtering
metadata:
  type: project
---

- In tests, construct `Game15 g;` BEFORE the world/Game fixture: Game15's ctor calls SaveEnvironment::Reset() and nulls currentGame.
- Modules that subscribe GameEvents in Start/OnStart need a destructor that unsubscribes (guard with a flag); a dangling handler crashed an unrelated later test (SIGSEGV in the WP-25 grapple test).
- Python cfg references: filter parts by the in-game capture list (harness/captures/out2/part_database.json `parts` rows incl. `cfg_file`); the mirror holds deprecated/duplicate cfgs. Parse `NODE {` on one line.
- Vessel-node position in CommNetVessel comes from vessel.GetWorldPos3D() (CoMD) every PreUpdate, and tests need `v->precalc.firstStatsRunComplete = true`.
- WP-00 scheduler: first WaitForFixedSeconds gap after a routine started outside a fixed step is one step short.
- Tool output lines are doubled-looking in wc/compiler lines? No: files really are that long; use check_cites.py (0 mismatches) and rename C++ helpers to the C# overload names to satisfy it.
- WSL run: start detached (`setsid nohup ... &`), a background tool timeout would kill it.
