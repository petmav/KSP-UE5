# PROGRESS wp33 (Inventory and EVA construction)

Branch wp33 from main af3798c6. Worktree core/.worktrees/wp33.

## Done (2026-10-11)
- Port: include/ksp/inventory/{inventory,construction}.hpp, src/inventory/{inventory,module_inventory_part,construction,inventory_ui}.cpp
  (StoredPart, ModuleCargoPart, ModuleInventoryPart, KerbalInventoryScenario, kerbal glue, ProtoVesselNodes, EVAConstruction{Util,ModeController,ModeEditor,ToolsUI},
  PartItemTransfer, UIPartActionControllerInventory / InventorySlot / Inventory, InventoryPanelController). WP-06 stand-ins removed; PartModule has the six
  inventory / construction virtuals; GroundPartSeam (ModuleGroundPart family = ModuleCargoPart until WP-53).
- Reference: tools/wp33/inventory_reference.py -> tests/data/wp33_reference.json (0 ULP / exact).
- Tests: 34 cases / 24,883 assertions in 7 files (+ tests/inventory_fixture.hpp).
- check_cites.py exact mode: 301 cites, 0 mismatches (4 aliases added to NAME_ALIAS).
- Fixes found on the way: Unity Instantiate copy of ModuleInventoryPart (CopySerializedFrom) so crafts keep writing `STOREDPARTS {}` (WP-18 idempotence test); ModuleCargoPart.OnInitialize
  subscribes once (test_docking left 164 stale handlers); deferred Object.Destroy (pendingDestroy); GCC -Wall -Wextra clean (WSL g++ 13 -fsyntax-only on every new TU).
- STATUS.md section + summary row 34, README rows (unit WP-33, include/ksp/inventory/, tools/wp33/).

## Open
- final fresh verification + push (not main).

## Merge of origin/main (wp50, wp53, wp17b, wp42, wp38)
- ModuleCargoPart: single class, wp53's `modules/module_cargo_part.hpp/.cpp`; WP-33 members/behaviour added to it (inventory.cpp), `GroundPartSeam` and the duplicate registration removed, `RemoveEventHandlers()` in OnDestroy and the destructor, `settleCoroutine()` accessor + public MakePartSettle for tests (settle tests now drive ScienceEnvironment scheduler/UT).
- STATUS row renumbered 36 (34 is wp50); check_cites.py aliases unioned; unit 690 cases / 1,071,518 assertions, ctest 34/34, cites 301 / 0 mismatches.
