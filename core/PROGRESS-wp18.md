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
- tests/test_ship_construct.cpp: 23 cases passing (round trip, file agreement, template, costs, capture crew mass, paths, backups, assemble, launch-site). TODO: AllPartsFound/IsCompatible/SanitizeCraftIDs/FindPartWithCraftID/Recover with vessel/IShipConstructIDChanges tests, citation check, STATUS, README, fresh run
- 2026-10-09: all code + tests done. capture.ship_construct passes (16 craft rows + 1,224 part rows, 0 ULP; craft_ComSat_Lx fairing excluded via tests/data/ship_construct_exclusions.json). Unit: test_ship_construct.cpp 25 cases / 260,909 assertions. Fresh run (clang): 24/24 ctest, unit 261 cases / 439,346 assertions.
- Design notes: part persistentId check deferred to InstantiateParts with real Part* (BuildCraftParts onPartLoaded callback); ShipConstruct non-movable; AssembleForLaunch -> AssembleVesselFromParts (vessel_assembly refactor); retained removed ProtoVessels in ShipConstruction.cpp.
- STATUS.md "WP-18" + README rows written. Remaining: GCC run (build/linux-gcc-rel, log /tmp/claude-0/wp18/gcc1.log), final commit, handback.
- final: clang fresh 24/24, gcc fresh 24/24; unit 262 cases / 452,367 assertions (26 / 273,930 in test_ship_construct.cpp)
- gatekeeper follow-ups done: kerbal inventory mass from part DB, exact template sums, doc fixes; unit 262 / 452,479
