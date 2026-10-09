# Tiering review rubric (input for review agents; not a deliverable)

Project: private, local-only reimplementation of Kerbal Space Program 1.12.5 on Unreal Engine 5. Architecture: an
engine-agnostic C++20 simulation core (double precision, headless, deterministic, unit/scenario tested) plus a thin
UE5 shell (rendering, UI, input, audio, optional local physics). The decompiled game
(`C:\Projects\KSP-UE5\decompiled\KSP-Decompiled\Assembly-CSharp\`) is the behavioural spec. Scope is the WHOLE game:
career, science, contracts, Kerbals, both DLC code paths, CommNet, editors, UI.

Every KSP-authored .cs file gets a tier, a subsystem, a UE5 counterpart (mandatory for tier C) and a one-line rationale.

## Output format (strict)

Write `C:\Projects\KSP-UE5\analysis\tiering\work\review\chunk_NN.tsv` (NN = your chunk id). UTF-8, tab-separated,
header line exactly:

    path<TAB>tier<TAB>subsystem<TAB>ue5_counterpart<TAB>rationale

- Exactly one row per `## path` heading in your digest, path copied verbatim (e.g. `Assembly-CSharp/Part.cs`).
- tier is one of `A`, `B`, `C`.
- subsystem is one value from the taxonomy below, spelled exactly.
- ue5_counterpart: required for C (UE5 feature/system, or `drop`); optional for B (what the shell side becomes,
  e.g. `core + Chaos/Jolt shim`); for A use `core` (or leave empty).
- rationale: one line, <= 30 words, no tabs. Say what the class does AND the decisive reason for the tier. Be specific
  ("Kepler elements, anomaly solvers, state vectors; pure double math -> transpile; golden vs KSP"), not generic
  ("UI class").
- No tabs or newlines inside fields. Do not quote fields.

## Tier definitions and decision rules

**A, Transpile.** Pure logic/math/data with little or no Unity dependency. Can be translated nearly mechanically
into the C++ core and checked against golden outputs captured from KSP. Typical: math types, orbit maths, ConfigNode,
curves, enums/interfaces/data classes used by gameplay, persistence/snapshot data, rules engines (contracts,
strategies, science value, experience, funding/reputation), resource flow, CommNet graph, tech tree data, save upgrade
pipeline, delta-v calculation.
- MonoBehaviour inheritance alone does NOT force B. ScenarioModules and PartModules whose behaviour is state/resource/
  rule logic are A when their Unity use is limited to lifecycle hooks (OnStart/OnUpdate/OnFixedUpdate/OnLoad/OnSave),
  math value types used as data (Vector3/Quaternion/Mathf/Color), Debug.Log, Time reads, GameEvents, coroutines used
  only as timers. Note "needs PartModule host" (or ScenarioModule host) in the rationale.
- Reading Vessel/Part/CelestialBody *state* through their public API is fine for A (the core will expose the same
  queries). Writing to Rigidbody/Transform is not.

**B, Re-architect.** Gameplay logic entangled with Unity physics, scene graph, transforms, or the MonoBehaviour
lifecycle, whose behaviour must be preserved but whose structure is redesigned for the core + shell split.
- Touches Rigidbody / forces / joints / colliders / raycasts / physics materials / WheelCollider.
- Uses the Transform hierarchy for gameplay meaning (thrust transforms, attach node positions, part placement, CoM,
  docking alignment, EVA movement, ladder/hatch triggers).
- Animation that drives gameplay state (deployables, gear, parachutes, cargo bays, solar tracking).
- Scene/lifecycle orchestration: vessel load/unload, on-rails/off-rails, packing, FlightGlobals, FlightDriver,
  scene managers, time warp, floating origin, frame shifting.
- Gameplay-relevant mesh/collider generation (PQS quads and colliders, procedural fairings, asteroid/comet colliders,
  drag cube rendering).
- UI-heavy or UI-namespaced classes that are the canonical owner of a gameplay rule (e.g. `KSP/UI/Screens/StageManager`
  owns the staging sequence) are B with rationale "extract <rule> to core; UI -> UMG".
- Base classes that bridge Unity lifecycle to gameplay (PartModule, VesselModule, ScenarioModule, PQSMod, Part,
  Vessel) are B (the host is redesigned even if subclasses are A).

**C, Rewrite in UE5 / discard.** Presentation and platform: Unity UI (UnityEngine.UI, TMPro consumers, IMGUI),
rendering, materials/shaders, cameras, audio, input devices/bindings, particles/FX, Unity asset loading
(AssetBundles, textures, .mu/.dae/.obj model readers), Steam, analytics, localization plumbing, KSPedia, debug/cheat
tools and test scripts, purely cosmetic MonoBehaviours. Give the UE5 counterpart or `drop`.
- Enums/interfaces/delegates follow the subsystem they serve: A if used by gameplay logic, C if purely UI/render.
- Debug/cheat/test-only code: C, counterpart `drop` or `console commands / Gameplay Debugger`.

When a file is genuinely mixed, pick the dominant tier by lines and say "mixed: <minor part> is <tier>" in the
rationale.

## Fixed anchor decisions (use these; they set the bar for similar files)

| File | Tier | Note |
|---|---|---|
| ConfigNode.cs, IConfigNode.cs, UrlDir.cs | A | cfg parser/serialiser, GameData virtual tree |
| FloatCurve.cs, DoubleCurve.cs, Vector3Curve.cs | A | FloatCurve wraps Unity AnimationCurve: shim must replicate Unity Hermite evaluation exactly |
| Vector3d.cs, QuaternionD.cs, Matrix4x4D.cs, UtilMath.cs | A | math |
| Orbit.cs, PatchedConicSolver.cs, PatchedConics.cs, TransferMath.cs | A | orbit maths (solver is a MonoBehaviour shell around pure logic) |
| Planetarium.cs | A | inertial frame rotation and UT |
| OrbitDriver.cs, TimeWarp.cs, Krakensbane.cs, FloatingOrigin.cs | B | rails/physics sync, frame shifting |
| CelestialBody.cs | B | MonoBehaviour+transforms; atmosphere/gravity/rotation math inside is extractable A |
| Part.cs, Vessel.cs, PartModule.cs, PartJoint.cs, VesselModule.cs, ScenarioModule.cs | B | core object model hosts |
| FlightIntegrator.cs | B | VesselModule driving aero+thermal through Unity physics; the maths is extracted as A |
| FlightGlobals.cs, FlightDriver.cs, HighLogic.cs | B | scene/state orchestration |
| PQS.cs, PQ.cs, PQSMod.cs | B | quadtree + mesh build; PQSMod is the hook host |
| PQSMod_Vertex*Height*/Noise/Simplex/Voronoi (vertex maths only) | A | height/colour functions |
| PQSMod_* creating GameObjects/meshes/colliders | B if gameplay (colliders, cities, scatter colliders) else C (materials/UV/fade) |
| EditorLogic.cs | B | editor state machine, part placement, symmetry |
| ShipConstruct.cs | A | craft data model; ShipConstruction.cs is B (spawns vessels) |
| ModuleEngines.cs, ModuleRCS.cs, ModuleGimbal.cs, ModuleWheelBase.cs, ModuleWheels/*, KerbalEVA.cs, ModuleDockingNode.cs, ModuleDecouple.cs | B | |
| KSP/UI/Screens/StageManager.cs | B | owns staging sequence; extract |
| UIPartActionWindow.cs, NavBall, all KSP/UI widgets | C | UMG |
| DeltaVStageInfo.cs and DeltaV calc classes | A | |
| PartSet.cs, PartResource*.cs, ResourceFlowMode.cs | A | resource flow |
| ResearchAndDevelopment.cs, ScienceSubject.cs, Funding.cs, Reputation.cs, Contracts/ContractSystem.cs, Contracts/Contract.cs, CommNet/CommNetwork.cs, KerbalRoster.cs, ProtoCrewMember.cs, Game.cs, GamePersistence.cs | A | rules/data |
| ProtoVessel.cs, ProtoPartSnapshot.cs | B | snapshot data is A but they instantiate live parts (load/unload bridge) |
| ProtoPartModuleSnapshot.cs, ProtoPartResourceSnapshot.cs | A | |
| GameDatabase.cs, PartLoader.cs | B | config side is core, model/texture/audio side is shell |
| AssemblyLoader.cs | C | C# plugin loading; drop (UE5 plugin/mod API later) |
| PhysicsGlobals.cs | A | Physics.cfg constants and curves (MonoBehaviour singleton holding data) |
| DragCubeList.cs, DragCube.cs | A | drag cube data/maths; DragCubeSystem (renders parts to compute cubes) is B |
| GameSettings.cs | C | settings + key bindings -> UE GameUserSettings + Enhanced Input |
| KSP/Localization/Localizer.cs | C | localization plumbing -> UE FText/StringTable + format shim |
| Steam*, Analytics* | C | Online Subsystem Steam / drop |

Third-party code (TMPro, Smooth, VehiclePhysics, EdyCommonTools, LibNoise, SoftMasking, UniLinq, DDSHeaders,
Highlighting, Vectrosity, Steamworks) is handled separately and is not in your chunk.

## Subsystem taxonomy (use exactly these spellings)

| Subsystem | Covers |
|---|---|
| Core | math types, curves, utilities, extension methods, collections, event bus (GameEvents/EventData), KFSM state machine, PID, logging |
| Save | ConfigNode, persistence (Game, GamePersistence, GameParameters), Proto* snapshots, craft data (ShipConstruct/ShipTemplate), SaveUpgradePipeline, KSP/IO, quicksave, versioning |
| Modding | GameDatabase config side, UrlDir, AssemblyLoader, KSPAddon, scenario registration, part upgrades, type parsers, Expansions loader, asset bundle definitions |
| Scene | HighLogic, GameScenes, scene drivers (FlightDriver, EditorDriver), loading screens, main menu flow, KSC scene and buildings, launch sites, pause menus, Timing* |
| Orbit | Orbit, OrbitDriver, patched conics, trajectories, maneuver node logic, transfer maths, targeting, time warp |
| Celestials | CelestialBody, PSystem*, Planetarium, Sun, scaled space logic, biomes/CB attribute maps, asteroids/comets (space objects) |
| Terrain | PQS*, PQSMod_*, PQSCity, land control, surface objects, map SOs |
| Physics | FloatingOrigin, Krakensbane, PhysicsGlobals, collision handling, buoyancy helpers, physic materials, gravity helpers, joint helpers |
| Vessel | Vessel, VesselModule infra, staging, action groups, delta-v, VesselPrecalculate, flight control state, flight log, vessel naming/type |
| Part | Part, PartModule framework (BaseField/BaseEvent/BaseAction, KSPField/KSPEvent/KSPAction, UI_* field attributes), AttachNode, PartJoint, PartLoader, AvailablePart, part variants, generic animation modules, CompoundPart base |
| Resources | resource definitions, PartResource*, PartSet / flow modes, resource broker, planetary resource maps, harvest/depletion |
| PartModules-Engines | engines, RCS, gimbals, intakes, propellants, engine FX modules |
| PartModules-Wheels | wheels, landing gear, KSPWheelController, wheel submodules |
| PartModules-Control | command, SAS, reaction wheels, autopilot, remote control |
| PartModules-Structural | decouplers, docking, grapple, fairings, cargo bays, jettisons, struts/fuel lines, launch clamps, parachutes, structural nodes, crossfeed toggles |
| PartModules-Resources | generators, converters, harvesters/drills, scanners, solar panels, fuel jettison |
| PartModules-Misc | lights, colour changers, flags, other modules not above |
| Aero | FlightIntegrator, drag cubes, lifting/control surfaces, aero queries (CoL/CoT), aero debug |
| Thermal | thermal data/links, radiators, ablators, core heat, temperature model, overheat display |
| Editor | VAB/SPH logic and editor UI, part list/categories, symmetry, editor markers/gizmos, craft browser, pre-flight checks, EVA construction editor |
| FlightUI | flight HUD: navball, staging UI, PAW (UIPartAction*), altimeter, gauges, screen messages, delta-v app, alarm clock app, crew portraits, overlays |
| MapView | map view, orbit/patched-conic rendering, map nodes, maneuver gizmo/editor, tracking station UI |
| UI | generic UI framework and screens: dialogs (DialogGUI*, PopupDialog), tooltips, app launcher, settings, main menu UI, KSC UI, flag browser, widgets |
| Career | funds, reputation, currency queries, strategies, facility upgrades (Upgradeables), administration, progress tracking/achievements, recovery value |
| Science | R&D, science subjects/data/experiments, science modules, tech tree nodes, science lab (KerbNet goes to CommNet) |
| Contracts | Contracts/, FinePrint/, SentinelMission/, waypoints for contracts |
| Kerbals | roster, ProtoCrewMember, Experience, EVA, IVA/Internal*, crew transfer, inventory/cargo parts, kerbal animation/expressions, suits |
| CommNet | CommNet/, KerbNet/, antennas, data transmitters, remote control relay |
| Localization | Localizer, Lingoona, fonts |
| Audio | audio FX, music, ambience, volume |
| Camera | flight/map/editor/IVA/KSC cameras, camera FX, TrackIR/SpaceNavigator |
| Rendering | materials, shaders, FX/particles, explosions, highlighting, texture/model loaders, lights, sky/sun visuals, flags, thumbnails/screenshots |
| Input | GameSettings bindings, InputLockManager, key/axis bindings, mouse, flight input handler, control types |
| Steam | Steam integration |
| Analytics | analytics, privacy |
| Tutorials | tutorial scenarios and steps, instructor |
| Debug | debug toolbar, cheats, console, test scripts, profiling |
| DLC-MakingHistory | Expansions/Missions (mission runtime, editor, nodes, tests, actions, adjusters), GAP*/ME*/MC* helpers in root |
| DLC-BreakingGround | Expansions/Serenity (robotics, deployed science, rotors), ROC surface features, ModuleGround*, ActiveJoint/ModuleJointMotor |
| Misc | anything that fits nowhere else (obfuscator artefacts, assembly info) |

## UE5 counterpart vocabulary (for C, prefer these terms)

UMG widget, CommonUI, Slate, UMG + core query (when a UI shows core data), Enhanced Input, UE GameUserSettings,
Niagara, Material / Material Instance, Dynamic Material Instance, Post Process Volume, Sky Atmosphere,
Volumetric Clouds, Directional/Point/Spot Light, Decal Component, Procedural Mesh / RealtimeMesh, Instanced Static
Mesh / HISM, Camera Component / Spring Arm, Cine Camera + Sequencer, Audio Component / MetaSounds / Sound Cue,
Online Subsystem Steam, UE Localization (FText / String Table), Asset Manager / Primary Data Asset,
Interchange / glTF importer, Line Batcher / Procedural Mesh lines, Render Target / Scene Capture, Widget Component
(3D UI), Console Variables, Gameplay Debugger, Unreal Insights, Automation Tests, `drop`.

## How to work

- Your digest `C:\Projects\KSP-UE5\analysis\tiering\work\digests\chunk_NN.md` lists each file with extracted features
  (line count, kind, ancestry, usings, Unity API counts, KSP type references, method names, fan-in/out, and an
  automatic suggestion). The automatic suggestion is a crude heuristic; overrule it whenever your reading says so.
- Every file marked **MUST-OPEN (>800 lines)** must be opened and skimmed (member list plus the key methods) before you
  decide. For other files, open them whenever the digest is not conclusive (especially MonoBehaviours with
  middling unity_score, and anything in the root directory whose role is unclear from its name).
- `unity_score` is 0-100: a saturating weighted density of Unity API references per 100 lines (+15 if the class is a
  Unity component). Unity math value types weigh little.
- The decompiled tree is READ-ONLY. Another agent may be editing it on a branch; ignore any changes. Do not run git
  commands. Write only your own TSV file under `analysis\tiering\work\review\`.
