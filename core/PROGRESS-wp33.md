# PROGRESS wp33 (Inventory and EVA construction)

Branch wp33 from main af3798c6. Worktree core/.worktrees/wp33.

## Done
- 2026-10-11 first compiling port: include/ksp/inventory/{inventory,construction}.hpp, src/inventory/{inventory,module_inventory_part,construction,inventory_ui}.cpp
  (StoredPart, ModuleCargoPart, ModuleInventoryPart, KerbalInventoryScenario, EVAConstruction{Util,ModeController,ModeEditor,ToolsUI},
  PartItemTransfer, UIPartActionControllerInventory / InventorySlot / Inventory, InventoryPanelController, kerbal inventory glue).
  PartModule got the six inventory / construction virtuals; the WP-06 stand-ins in module_behaviours.cpp are gone.

## Next
- run the existing suite (regression), unit tests, Python reference, STATUS/README, cite check.
