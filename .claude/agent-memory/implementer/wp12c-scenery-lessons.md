---
name: wp12c-scenery-lessons
description: WP-12c scenery collider extraction/hosting lessons - UnityPy facts, spawner replay, game materials, test-fixture frames, tooling traps
metadata:
  type: project
---

- KSC buildings are NOT children of the PQS prefab: `Kerbin/KSC/SpaceCenter` is a NestedPrefabSpawner (prefab sharedassets9 5726, root activeSelf false but PQSCity.transformsActive forces direct children on) and facility levels are UpgradeableObject prefabs. Replay both in the extractor; tags on buildings come from spawners with setTags.
- UnityPy 1.25: `read_typetree()` boost reader fails on classes with `string[]` (generator types it as `string`): patch node type to `vector`, read with `TypeTreeHelper.read_typetree(nodes, EndianBinaryReader(raw), True, None, False, assets_file)`. Mesh: `MeshHandler(mesh).process()` gives float32 verts; ignores baseVertex. Run with harness venv `python -I`; `-I` drops the script dir from sys.path (insert path explicitly, Windows form).
- Game physics: all colliders without a material (every part, every scenery collider) use `PhysicsManager.m_DefaultMaterial` = "Metal" 0.25/0.25/0, Maximum/Minimum; terrain "Ground" 0.9 Max/Min on 6 bodies. PhysX combine = highest-priority mode (Avg<Min<Mul<Max), implemented via Jolt shape PhysicsMaterial + SetCombineFriction; off by default.
- Test frames: `ReadState().CoMD` is NOT the physics frame; use `wp29c::VesselCoM(vp)` (physics frame) vs ray hits; `vesselTransform.position` is floating-origin scene coords.
- Bash tool: apostrophes inside python heredocs break -> write patch scripts with the Write tool; printed C++/py bodies get elided -> `awk '{print "|" $0}'`; sleep >= 2 blocked, use Bash run_in_background + until-loop. First Windows build in a new worktree ~15 min at -j4; incremental 1 min.
- Files written by python text mode on Windows become CRLF; repo is eol=lf: `sed -i 's/\r$//'`.
**Why:** each cost a debug round. **How to apply:** scenery / collider / material / UnityPy work.
