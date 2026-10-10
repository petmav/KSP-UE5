---
name: wp36-career-lessons
description: Facts learned porting funds / reputation / strategies / facilities / progress (WP-36): listener order, seams, test world, data gaps
metadata:
  type: project
---

- `EventData::Fire` calls listeners from the LAST registered to the first (WP-03, faithful to C#). Any float sum accumulated by listeners (CurrencyModifierQuery deltas) must be reproduced in reverse registration order in an independent reference, otherwise 1-4 ULP drift appears only when several listeners add to the same float.
- ScenarioModule subclasses: `ScenarioRunner` only runs `Awake`; `Start` (Reputation, ProgressTracking.Deploy) must be called by the host/test. Register types with `ScenarioModuleFactory::Register` (`RegisterCareerScenarioModules()` in `career/career.hpp`); a module is round-tripped with `ProtoScenarioModule proto(node); proto.Load(runner); ProtoScenarioModule snap(*module); snap.GetData()`.
- Test world for career code: `Game15` (stock database from the GameData mirror) + `PhysicsGlobals` + `Planetarium` + `CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium)` + `Game` with `Mode = CAREER` assigned to `SaveEnvironment::Current().currentGame` + `LoadLocalizer`. `FlightRegistry::Bodies()` (not `FlightGlobals`) is the body list; `FlightRegistry::ActiveVessel` is a `Vessel*`.
- `ProtoCrewMember::name()` and `ProtoPartSnapshot::partName` / `ProtoPartResourceSnapshot::resourceName` are `std::optional<std::string>`; `Part::fields.missionID` / `fields.flagURL`; `CelestialBody::name()` is a function.
- The only save in the repo (`core/testdata/saves/physprofile`) is a sandbox save: no Funding / Reputation / Facilities / StrategySystem nodes. GameVariables prefab curves and facility level costs are not in any extracted dataset.
- Python heredocs in the Bash tool collapse `\\n` inside C++ string literals into a real newline; edit C++ string escapes with the Edit tool.
- Writing a file the WSL build is compiling (it reads /mnt/c live) makes clang see torn sources: do not edit during a WSL build.

**Why:** each cost a debug cycle in WP-36. **How to apply:** before porting scenario modules / event-driven float accumulation or when starting the WSL run.
