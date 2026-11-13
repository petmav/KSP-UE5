## WP-40b: editor placement loop, EditorLogic KFSM (branch `wp40b`, from main `d74ae28c`, merged main `82ef52e4`)

Spec: `EditorLogic.cs:1236-2366` (SetupFSM: 9 states, 28 events), `:466-604` Awake / Start, `:606-830` StartEditor, `:2367-2443` Update / LateUpdate / FixedUpdate /
onConstructionModeChanged, `:2621-2673` pickPart / CenterDragPlane / dragOverPlane, `:2708-2830` UndoRedoInputUpdate / RestoreState, `:2834-2864` the part-list taps,
`:3131` deleteInputUpdate, `:4023-4062` NewShip, `:4760-4830` ShipToLoadSelected, `:5365` partSearchUpdate, `:5439` HeldPartIsStacked, `EditorLogicBase.cs:75`
partRotationInputUpdate, `EditorToolsUI.cs` (SetMode, Update, onEditorPartEvent), `KerbalFSM.cs`, `Mouse.cs`. New files: `src/editor/editor_fsm.cpp`, `tests/test_editor_fsm.cpp`,
`tests/data/wp40b_reference.json`, `tools/wp40/session_ref_lib.py`, `tools/wp40/session_reference.py`. Edited (own lines only): `include/ksp/editor/editor_types.hpp` (seams),
`editor_logic.hpp` / `src/editor/editor_logic.cpp` (FSM members; spawn / delete / RestoreState / gizmo callbacks completed to the C# bodies), `editor_markers.hpp` (`EditorRay` moved to
editor_types.hpp), `include/ksp/inventory/construction.hpp` (`EditorScreen` moved to editor_types.hpp), `src/editor/editor_gizmos.cpp` (reads keys through the new accessors),
`tools/wp28/check_cites.py` (3 alias entries). `python -I tools/wp28/check_cites.py src/editor/editor_fsm.cpp src/editor/editor_logic.cpp`: 93 cites, 0 mismatches.
**There is no in-game editor capture** (`harness/captures/out*/` and `harness/golden/MANIFEST.json` have none): the gate is unit-level against an independent Python simulator.

### What is ported (all in `EditorLogic`)

| C# | port |
|---|---|
| `SetupFSM` `:1236-2365` | `EditorLogic::SetupFSM`: the 9 states (OnEnter / OnUpdate chains in the C# order, `st_root_unselected.OnUpdate` overwritten with an empty delegate as in the source, `st_podSelect` locks `EditorLogic_rootPartMode`) and the 28 events with their conditions, bodies and `AddEvent` / `AddEventExcluding` registrations in the C# order (the order of an event list decides which event fires first in a frame) |
| `Awake` / `Start` / `Update` / `LateUpdate` / `FixedUpdate` | `Awake` (listeners `onEditorConstructionModeChange`, `onEditorSymmetryMethodChange`; the EditorToolsUI part-event listener is wired here too), `Start` (`SetupFSM`, `StartEditor(false)`, `StartFSM(rootPart ? st_idle : st_podSelect)`), `Update` (`fsm.UpdateFSM`, the PAUSE key for a compound part being placed, then `EditorToolsUI.Update`), `RunCoroutines` (the 1-frame `DelayedCallback` of RestoreState), `LateUpdate`, `FixedUpdate` |
| `StartEditor` `:606`, `NewShip`, `OnNewShipConfirm`, `ShipToLoadSelected` (path, node, Normal / Merge), `LoadShipFromFile` | `StartEditor(isRestart)` (START_CLEAN / LOAD_FROM_CACHE / LOAD_FROM_FILE, backups, `on_shipLoaded` / `on_newShip`), `NewShip` (unsaved-changes dialog = `EditorEnvironment::newShipDialog`, default Discard), the loads |
| `pickPart`, `CenterDragPlane`, `dragOverPlane`, `HeldPartIsStacked` | same names; `pickPart` keeps the side effects of the C# (grab offset and drag plane are set even when the caller discards the part) |
| `symInputUpdate`, `snapInputUpdate`, `snapButton`, `UndoRedoInputUpdate`, `deleteInputUpdate`, `partOffsetInputUpdate`, `partRotationInputUpdate()` (EditorLogicBase), `partRotationResetUpdate` | keys through `GameSettings::Editor_*` / `InputEnvironment::Input()` / `InputLockManager` (WP-50); the player build on Windows / Linux (not the Unity editor, not OSX) for undo / redo |
| `OnPartListIconTap`, `OnPartListBackgroundTap`, `ReleasePartToIcon`, `OnPodSpawn` / `SpawnPart` / `SpawnConstruct` / `SetIconAsPart` tails (drag position, `RunEvent`), `DestroySelectedPart` tail (`on_podDeleted` / `on_partDeleted`), `OnNewRootSelect` (full body), `RestoreState` (`on_undoRedo`, `on_podSelect`, `skipPartAttach`) | completed from the WP-40 data-only versions |
| `EditorToolsUI` `SetMode`, `Update` (keys 1-4), `onEditorPartEvent` | `toolsSetMode`, `toolsUpdate`, `toolsOnEditorPartEvent`: a part event calls `SetMode(Place)`, which fires `onEditorConstructionModeChange`, whose listener runs `on_goToModePlace` INSIDE the event that triggered it (reproduced; session `s27`) |
| `Part.frozen` / `freeze` / `unfreeze` | kept in `EditorLogic` (the C++ `Part` has no such field; `Object.Instantiate` copies it, `InstantiateCopy` does too) |
| gizmos | `st_offset_tweak` / `st_rotate_tweak` create real `EditorGizmos::GizmoOffset` / `GizmoRotate` objects (`OffsetGizmo()` / `RotateGizmo()` for the shell to drive the handles); the `gizmoOffset` / `gizmoRotate` state the WP-40 callbacks read is synchronised around every callback |
| scene objects | `SceneParts()` = every Part the editor created or loaded, in creation order, destroyed ones included (a Unity `x != null` on a destroyed part is `IsDestroyed`) |

### Seams (`EditorEnvironment`, `include/ksp/editor/editor_types.hpp`; unset = an empty scene)

`cameraPosition() -> Vector3f`; `screenPointToRay(screenPoint) -> EditorRay`; `worldToScreenPoint(world) -> Vector3f`;
`raycast(ray, maxDistance, layerMask) -> optional<EditorSurfaceHit>` (`Physics.Raycast` + `GetPartUpwardsCached`; the checkAttach ray uses `Input.mousePosition - srfAttachCursorOffset` and
mask `DefaultEquivalent`, pickPart the `layerMask | ...` masks); `nodeIconUnderCursor(part, node, worldPoint) -> bool`; `findPartSurface(...)`; `pointerOverUI()`; `mouseOverModalArea()`;
`mouseOverGizmo(offset)`; `heldPartIsStacked()`; `playOneShot(EditorClip)`; `onEditorStartTweak / onEditorEndTweak(Part&)` (CompoundPart overrides); `onHighlightSelected`; `newShipDialog()`;
`revealPart(AvailablePart&)`; `hasFlagDecalBackground(Part&)`; `fineTweakKey / modifierKeyHeld / useAngleSnapSetting` (`Awake` wires them to GameSettings; unset, the plain fields apply). Keys,
buttons and the mouse position come from the WP-50 `IInputDevice`, `Mouse` and `InputLockManager`. The old `surfaceRaycast()` seam still works when `screenPointToRay` / `raycast` are unset.

### Gate

| gate | result |
|---|---|
| independent reference | `tools/wp40/session_ref_lib.py` + `session_reference.py`: a Python simulator of the whole loop written from the C# (KerbalFSM, the 9 states / 28 events, checkAttach, createSymmetry, CheckSymPartsAttach, UpdateSymmetry, attach / detach, backups and RestoreState through saved world poses, re-root `isRootCandidate / MakeRoot / SetHierarchyRoot`, Mouse.cs, EditorToolsUI, gizmo handles) on the numeric primitives of `tools/wp40/editor_reference.py` and `tools/wp12/pqs_ref_math.py` (FromToRotation); it shares no code with kspcore |
| `unit` `tests/test_editor_fsm.cpp` | 2 cases: the session replay and the stock-craft rebuild |
| sessions | **48 sessions, 1,535 frames, 5,610 trace rows, 83,769 part values compared bit for bit, 3,427 scene-seam calls checked, 0 mismatches**; trace and a hash of the full part snapshot compared on every frame |
| coverage | all 9 states and all 28 events (fire counts: on_partAttached 89, on_partCreated 81, on_podSelect 49, on_partPicked 30, on_partDropped 10, on_goToModePlace 9, on_goToModeOffset 8, on_goToModeRotate 7, on_offsetSelect 7, on_partDeleted 5, on_goToModeRoot 4, on_newShip 4, on_shipLoaded 4, on_podDeleted 3, on_rotateSelect 3, on_rootPickSet 2, on_undoRedo 2, and once each on_partCopied, on_partReveal, on_partLost, on_partOverInventoryPAW, on_offsetSelect-deselect pair, on_offsetReset, on_rotateDeselect, on_rotateReset, on_rootDeselect, on_rootSelect, on_rootSelectFail); entered states: idle 168, place 114, offset_select 9, offset_tweak 9, podSelect 7, rotate_tweak 7, rotate_select 6, root_select 4, root_unselected 4 |
| what a frame compares | the ordered trace: `ev:` FSM events (`OnEventCalled`), `st:` state changes (`OnStateChange`), `ge:` 20 GameEvents of the editor with their arguments, `clip:` audio clips, `hl:` highlight calls, `tw:` tweak hooks, `cmd:` host commands; and an FNV-1a 64 hash of the canonical snapshot: fsm state, selected / root part, undo level, backup count, symmetry mode / method / space, ship order, and per part: id (creation order), name, parent, position, rotation, attPos, attPos0, attRotation, attRotation0 (bit patterns), attach mode, symMethod, children, symmetry counterparts, attach node links, surface node link and mesh name, frozen flag, ship membership; destroyed parts by name |
| scene seams | the simulator answers the camera ray, `Physics.Raycast`, node icon bounds and FindPartSurface with ANALYTIC TEST FIXTURES (a sphere around each part, a 0.35 sphere around each node, a ray table) and records every call with its answer; the C++ test replays the answers and fails on a different question, input or call order. These fixtures are not part data and are not claimed to equal the game colliders |
| parts / crafts | stock part prefabs from `core/testdata/GameData-mirror` (`mk1pod.v2`, `fuelTankSmallFlat`, `liquidEngine2`, `radialDecoupler`, `batteryPack`, `stackBiCoupler.v2`, ...) with attach nodes / rules from `harness/captures/out2/part_database.json`; loaded crafts `Jumping Flea` and `Ion-Powered Space Probe` (`harness/inputs/Ships/VAB`) |
| UNVERIFIED | `Quaternion.LookRotation` and `Quaternion.Euler` (native, no capture; the reference uses the same formulation as the port: a consistency check), `Transform.localPosition / localRotation` derived from the parent matrix (attPos0 / attRot0 after attach), `Bounds.Contains` in dragOverPlane (no EditorBounds instance: everything inside). The JSON lists per session which of them it reached (`unverified`): 29 sessions reach all three, 14 Euler + local pose, 2 LookRotation + Euler, 1 LookRotation + local pose, 2 none (`s01`-class stack-only sessions reach Euler only through the drag; see the field) |

Stock crafts rebuilt part by part (`TEST_CASE "WP-40b EditorLogic: stock crafts rebuilt ..."`): the host spawns each part, aims the cursor, presses the symmetry keys and clicks; the saved craft
(`ship->SaveShip()`) part tree is compared with the original's as a canonical string. Compared: part name, attach mode, symmetry set size and method, linked attach nodes by node id and partner part
name, surface link partner name, children as a sorted multiset, part count. Excluded: craft ids / persistent ids (re-issued), pos / rot / attPos0 / attRot0 (the largest deviation of a part position from the original relative to the root is
printed: the editor places by node alignment, the originals carry the player's cursor path), mir, stage numbers, module data. Result: **`PhysProf-S` (21 parts), `PhysProf-M` (150), `PhysProf-L` (446), `Jumping Flea` (8, radial symmetry), `Ion-Powered Space Probe` (10)
save an equal tree (pinned)**; 7 other crafts are attempted and the driver reports why it stopped (AeroEquus, Ariane 5, Orbiter One, PT Series Munsplorer, Science Jr, Space Station Core, Super-Heavy Lander: a drop refused by the host driver, usually a
part that needs a rotated placement, a stack-symmetry count or a node pair the host did not identify; these are limits of the host driver, not claims about the editor).

### Findings (rules the gate pinned down)

- `st_root_unselected.OnUpdate` is assigned an empty delegate AFTER three handlers were combined into it: undo / snap keys do nothing in that state (sic).
- `on_partPicked.OnCheckCondition` assigns `selectedPart` (and sets the grab offset and drag plane) even when it returns false; the pick of an attached part returns from `OnEvent` before `PartPicked` is fired.
- `Mouse.Update` updates the buttons BEFORE `screenPos`: `dragDelta` lags a frame, and `WasDragging` at a button-up reads the delta of the last frame the button was held (stale across clicks).
- `EditorToolsUI.onEditorPartEvent(PartDeleted)` runs `SetMode(Place)`, which runs `on_goToModePlace` from inside `DestroySelectedPart`; the outer `on_partDeleted` then continues from the state the nested event reached.
- `SetHierarchyRoot` on a surface-attached part reverses the surface node only when both nodes have an owner: a part without `node_attach` has the default-constructed `srfAttachNode` (no owner), so re-rooting leaves its link alone (found by the gate: the simulator assumed an owner).
- `OnNewRootSelect` picks with `LayerUtil.DefaultEquivalent | 4 | 0x200000`, not the `layerMask` field; the pick is ignored but still moves the drag plane.
- A surface-attachable part dragged near a ship part is attached by SURFACE even when its nodes are in range (the surface branch of checkAttach overrides the stack branch whenever the cursor ray hits a ship part); the sessions turn `allowSrfAttachment` off for node attaches.
- The gate also found: the WP-40 data-only `OnNewRootSelect` omitted the pick, the clips and `on_rootSelect`; the simulator mistakes it caught in itself (listener reactions of `PartCreated` / `PartDeleted`, static gizmo coordinate spaces leaking between sessions, a layer mask).

### Not ported / deferred (C# line ranges)

- Shell: widgets, part list / categorizer / search (`:1251-1256`, `:1306-1307`, `:5365`), `EditorPartList` filters, camera `ResetCamera` (`:1256`), screen messages, highlighting / layers / `SetLayerRecursive`, flag browser (`FlagSetup :812-870`; `FlagURL` stays as loaded), crew assignment (`ResetCrewAssignment` / `RefreshCrewAssignment` at `:1289-1298`, `:1787-1790`, `:1971-1974`, `:3113`), `VesselDeltaV` (`:1278-1282`, `:1736-1738`), text-field focus guards in the key handlers, mission-builder `editorSwitchInput` (`:2392-2395`), analytics.
- Inventory / cargo UI branches (`UIPartActionControllerInventory`, `ModuleCargoPart`): `:1336-1342`, `:1463-1466`, `:1804-1807`, `:1921-1925`, `:1975-1984`, `:2042-2052`; `Mouse.HoveredPart` over an inventory is wired (ModuleInventoryPart.TotalEmptySlots) but not gated.
- CompoundPart tweak mode (`CompoundPart.cs:290-320`): `onEditorStartTweak / EndTweak` are seams only.
- `EditorToolsUI.OnEditorLoad / OnEditorRestart` (panel visibility; the `SetMode(Place)` on load into a hidden panel): the sessions load while the panel is in Place mode.
- `st_rotate_tweak.OnLeave` towards `st_place` with a surface-attached part that is not in the ship (`:1623-1625`) is ported but no session reaches it; `TestPartBoundsSeparate` (offset gizmo clamp) is ported in WP-40 but needs part bounds that the part model does not have: the sessions' offset drags run without a clamp.
- The editor collision world (`Part.editorCollision` from physics callbacks) is a seam (`EditorEnvironment::editorCollision`), not an implementation (ue5 / `DESIGN.md` 6.5).

### Verification

- `core/build` deleted, `powershell -NoProfile -File core/run_tests.ps1 -Fresh` (CMAKE_BUILD_PARALLEL_LEVEL=4): FRESH_RESULT_PLACEHOLDER
- `./build/RelWithDebInfo/tests/kspcore_tests.exe --order-by=rand -rs=11` and `-rs=27`: RAND_RESULT_PLACEHOLDER; the WP-40 symmetry-mode case also passes alone (`-tc="*symmetry mode cycling*"`): the order dependence recorded in the README no longer reproduces (hygiene listener), nothing was changed for it.
- `g++ 13.3 -std=c++20 -fsyntax-only -Wall -Wextra` (WSL) over `editor_fsm.cpp`, `editor_logic.cpp`, `editor_gizmos.cpp`, `editor_markers.cpp`, `test_editor_fsm.cpp`: no new diagnostics (only the existing `part.hpp:410` unused-parameter warning).
- `python -I core/tools/ci/pr_review.py --base main`: PR_REVIEW_PLACEHOLDER.

