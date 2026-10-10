---
name: wp25-docking-lessons
description: Non-obvious facts from the WP-25 docking/grapple/strut port that later module ports and flight-scene unit tests need
metadata:
  type: project
---

Test-scaffolding and host facts learned while porting ModuleDockingNode / ModuleGrappleNode / CModuleStrut (WP-25):

- `LoadModuleData` (prefab path) calls `module->Awake()` BEFORE it sets `module->part`; `Part::AddModule` / `Part::Instantiate` set `part` first. A module's `OnAwake` must tolerate `PartOf() == nullptr` and register part-side lists again in `OnStart`.
- `Part::Instantiate` does NOT run `OnLoad`; vessels built with `AssembleVesselFromCraft` do (vessel_assembly.cpp LoadModule). Hand-built stacks (test_vessel Stack14 style) must call `module->OnLoad(emptyNode)` themselves when the module creates state there.
- `AssembleVesselFromCraft` vessels come out `packed = true`; set `v->packed = false` and every `part->fields.packed = false` before using `FindNodeApproaches`-style code. They are already in `FlightRegistry::VesselsLoaded` (do not AddVessel twice). Give both vessels distinct `flightID`s (two copies of one craft share them and `Part::Undock(info)` resolves `rootPartUId` by flightID).
- `PartPrefab::attachNodes` is a value vector: a C# shared `AttachNode` reference (ModuleGrappleNode.grappleNode) has to be looked up by id after it is added.
- `CHECK_MESSAGE(cond, a + b)` does not compile in doctest (`<<` binds tighter than `+`): wrap the message in parentheses.
- Stock docking ports mount with the node named by `referenceAttachNode` ("top"); a craft port whose reference node holds ANY part starts in `PreAttached`, not only a port-to-port pair.
- Stock ModuleGrappleNode parts start `Disabled` until `deployAnimator` progress (animTime) is 1.
- Near cos(pi/2) the snapRotation roll dot is ~1e-17 and differs between the x87 FCOS semantics of `dotnet::Math::Cos` and numpy: compare those absolutely in reference tests.
