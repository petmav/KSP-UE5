# PROGRESS WP-18 (Craft files and ship construction)

Branch wp18 (from wp14 7bc606d6). Notes after every major step.

## Plan / design decisions
- New dirs include/ksp/ship/, src/ship/: ShipTemplate (ShipTemplate.cs), ShipConstruction (ShipConstruction.cs statics), live part half of ShipConstruct
  (src/ship/ship_construct_live.cpp, members added to include/ksp/save/ship_construct.hpp; WP-04's data side `parts` (PartData) untouched).
- C# ShipConstruct.parts (List<Part>) = `liveParts` (raw Part*, owned in `ownedParts`); `parts` stays WP-04's PartData list.
- ShipConstruct becomes an IResourceVessel (WP-15 seam, ShipResourceSets) and a VesselCrewManifest seam (editor, not ported) is defined.
- AssembleForLaunch: refactor vessel_assembly.cpp so AssembleVesselFromCraft = BuildCraftParts + AssembleVesselFromParts; ShipConstruction::AssembleForLaunch calls the latter.
- Gate: dataset ship_construct against harness/captures/out2/deltav_readout.json (table craft: parts, total_mass, dry_mass, fuel_mass; table parts: mass, module_mass).
- Unit tests: tests/test_ship_construct.cpp.
- Names in the task text that do not exist in 1.12.5 ShipConstruction.cs: GetPartTotalMass, GetShipCosts, GetShipMass, ExtractShipName/ExtractShipTag, PlaceOnLaunchSite.

## Status
- [x] read specs; baseline build started (/tmp/claude-0/wp18/baseline.log)
- code written: ship_template, ship_construction, ship_construct_live, vessel_assembly refactor; builds. Next: dataset ds_ship_construct + gate, tests/test_ship_construct.cpp
- gate capture.ship_construct passes (17 crafts dry/fuel mass + 1225 part mass/module_mass, 0 ULP; ComSat_Lx fairing excluded: panelMass WP-26). Next: tests/test_ship_construct.cpp full, STATUS, README
