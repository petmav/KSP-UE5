# KSP 1.12.5 -> UE5: porting work breakdown (tiering)

Date: 2026-10-08. Scope: every KSP-authored source file of `decompiled/KSP-Decompiled/Assembly-CSharp/` (3,299 files, 627,747 lines) plus the reviewed part of `Assembly-CSharp-firstpass/` (29 files; Steamworks and Vectrosity are excluded as third-party). Embedded third-party namespaces are listed separately (third-party.md).

Deliverables in `analysis/tiering/`:

| File | Contents |
|---|---|
| `tiering.csv` | One row per file (3,328 rows): path, lines, kind, tier, subsystem, unity_dependency_score, key_dependencies, ue5_counterpart, rationale, review_source, fan_in, namespace |
| `work_packages.csv` | One row per file: the work package it belongs to |
| `TIERING.md` | This document |
| `third-party.md` | Embedded third-party libraries: use and port/replace/drop decision |
| `unity-api-surface.md` | Ranked Unity API usage by tier and the core shim spec |
| `scripts/` | Reproducible pipeline (`python -I`): extract_features -> depgraph -> classify -> make_lists -> merge -> report -> unity_surface -> surface_report -> tiering_md |
| `work/` | Intermediate data: features, dependency graph, digests, review TSVs, rubric |

## 1. Tier definitions (as applied)

- **A, Transpile**: pure logic, maths or data with little Unity dependency. It goes into the C++ core almost mechanically and is checked against golden outputs. MonoBehaviour inheritance alone does not make a file B: ScenarioModules and PartModules whose logic is rules/state/resources and that touch Unity only through lifecycle hooks, value types, logging and timers are A ('needs host').
- **B, Re-architect**: gameplay entangled with Unity physics, transforms, animation-driven state, scene orchestration or gameplay mesh/collider generation. Behaviour is preserved but the structure is redesigned for the core/shell split. Base classes that bridge the Unity lifecycle (Part, Vessel, PartModule, VesselModule, ScenarioModule, PQSMod) are B. UI classes that own a gameplay rule are B with the rule marked for extraction.
- **C, Rewrite in UE5 / discard**: presentation and platform code (UI, rendering, cameras, audio, input devices, FX, asset loading, Steam, analytics, localization plumbing, debug tools, tutorials). Each row names the UE5 counterpart or `drop`.
- **unity_dependency_score** (0-100): saturating weighted density of Unity API references per 100 lines (heavy engine APIs x3, object model x2, utilities x1, value types x0.25; score = 100*d/(d+25)), +15 for Unity components. It is a sorting aid; the tier is a reviewed judgement.

## 2. Method and provenance (what was reviewed by hand vs by script)

1. `extract_features.py` parses every file: line count, namespace, usings, type declarations and base classes (with an inheritance closure, so `kind` resolves PartModule/VesselModule/ScenarioModule/Contract/PQSMod/MonoBehaviour/UIBehaviour/class/struct/enum/interface/delegate), counts of 57 Unity API families, references to 56 KSP core types, and method names.
2. `depgraph.py` builds a file-to-file type-reference graph (C# namespace visibility applied). `fan_in` is the number of distinct KSP files that reference a type declared in the file.
3. `classify.py` produces an automatic tier/subsystem suggestion (path and name rules plus API counts). This was only an input to review. The final tiers differ from it on many rows.
4. Review:
   - **Lead (Opus) hand review**: all 127 KSP-authored files over 800 lines were opened and classified individually (`work/review/lead_big_*.tsv`). The lead also checked the automatic classification of the 282 files in the small directories outside the mandatory scope as a list (with features) and wrote or corrected 114 of them individually (`script_overrides.tsv`). The other 168 are homogeneous rows (SaveUpgradePipeline, PreFlightTests, KSPAchievements, Experience, Strategies, CameraFXModules) that use directory templates. The lead rewrote 381 tier-A reviewer rationales that were group templates into file-specific lines, correcting 69 tiers in the process (`KSP/IO` to C/drop, mission-editor metadata to C), and made 2 explicit tier overrides (`ModuleEnginesFX`, `ModuleRCSFX` to B).
   - **Sonnet reviewers (4, model `sonnet`)**: classified the remaining files of 800 lines or fewer in the root directory and in `KSP/`, `Expansions/`, `CommNet/`, `Contracts/`, `FinePrint/`, `SentinelMission/` and `KerbNet/`, from compact digests plus selective source reads, against a shared rubric with fixed anchor decisions (`work/RUBRIC.md`). Reviewers 1 and 2 (root files) wrote per-file rationales. Reviewers 3 (`KSP/` UI, contracts, CommNet) and 4 (`Expansions/`) assigned many rows by path pattern with group-level rationales. Every row whose rationale is a group template now carries an appended `[evidence: type : base; Unity deps; methods]` clause and is labelled `(group rationale + script evidence)` in `review_source`. That covers 598 rows, 589 of them tier C UI widgets.
   - An earlier round of 11 reviewers was stopped by the orchestrator (concurrency/model cap) before writing anything; none of its output is used.
5. Consistency pass (`consistency.py`): all 77 fixed anchor decisions hold; A rows with physics coupling, C rows that are gameplay hosts, and B rows without engine coupling were listed and checked by the lead.

### Review provenance

| Source | Rows |
|---|---:|
| hand:sonnet-child_1 | 608 |
| hand:sonnet-child_2 | 606 |
| hand:sonnet-child_3 (group rationale + script evidence) | 485 |
| third-party rule | 461 |
| hand:sonnet-child_4 > lead-rationale | 226 |
| hand:sonnet-child_4 | 222 |
| script+group-template | 168 |
| hand:sonnet-child_3 > lead-rationale | 155 |
| hand:lead | 127 |
| script+lead-check | 114 |
| hand:sonnet-child_4 (group rationale + script evidence) | 99 |
| hand:sonnet-child_3 | 41 |
| hand:sonnet-child_1 (group rationale + script evidence) | 7 |
| hand:sonnet-child_2 (group rationale + script evidence) | 7 |
| hand:sonnet-child_2 > lead-override | 2 |



Caveat: the per-file tier is reliable for the lead-reviewed rows and for spot-checked reviewer rows (a random sample of each reviewer's rows and a curated list of about 120 gameplay-critical files of 800 lines or fewer were audited by the lead). Group-rationale C rows were not individually re-read. Their tier is low-risk (UI widgets), but a few may hide small rules.

## 3. Summary

### Totals per tier

| Tier | Files | % of KSP files | Lines | % of KSP lines |
|---|---:|---:|---:|---:|
| A | 1,109 | 38.7% | 171,643 | 30.6% |
| B | 238 | 8.3% | 134,094 | 23.9% |
| C | 1,520 | 53.0% | 254,825 | 45.5% |
| **KSP-authored total** | **2,867** | 100% | **560,562** | 100% |
| third-party (excluded) | 461 | | 73,769 | |
| **All rows** | **3,328** | | **634,331** | |

| Assembly | A files | B files | C files | third-party files | Lines (A/B/C/3p) |
|---|---:|---:|---:|---:|---|
| Assembly-CSharp | 1108 | 238 | 1492 | 461 | 171,609 / 134,094 / 248,275 / 73,769 |
| Assembly-CSharp-firstpass | 1 | 0 | 28 | 0 | 34 / 0 / 6,550 / 0 |



### Totals per subsystem (KSP-authored files only)

| Subsystem | Files | Lines | A files | A lines | B files | B lines | C files | C lines |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| DLC-MakingHistory | 463 | 69,115 | 244 | 28,150 | 15 | 6,960 | 204 | 34,005 |
| UI | 321 | 42,433 | 0 | 0 | 0 | 0 | 321 | 42,433 |
| Contracts | 149 | 36,192 | 142 | 34,321 | 2 | 825 | 5 | 1,046 |
| Editor | 126 | 30,513 | 55 | 4,045 | 12 | 8,573 | 59 | 17,895 |
| FlightUI | 172 | 29,930 | 11 | 2,014 | 1 | 645 | 160 | 27,271 |
| Rendering | 190 | 29,258 | 0 | 0 | 0 | 0 | 190 | 29,258 |
| Kerbals | 115 | 26,737 | 35 | 5,424 | 17 | 12,812 | 63 | 8,501 |
| Part | 96 | 24,102 | 77 | 5,572 | 15 | 18,202 | 4 | 328 |
| DLC-BreakingGround | 64 | 22,272 | 21 | 5,155 | 21 | 11,791 | 22 | 5,326 |
| Save | 84 | 18,145 | 76 | 12,061 | 4 | 4,417 | 4 | 1,667 |
| Vessel | 44 | 16,798 | 33 | 6,722 | 10 | 9,853 | 1 | 223 |
| Terrain | 79 | 15,476 | 50 | 5,102 | 20 | 9,326 | 9 | 1,048 |
| Tutorials | 38 | 14,611 | 0 | 0 | 0 | 0 | 38 | 14,611 |
| MapView | 53 | 14,344 | 0 | 0 | 0 | 0 | 53 | 14,344 |
| Debug | 121 | 13,503 | 0 | 0 | 0 | 0 | 121 | 13,503 |
| PartModules-Structural | 35 | 12,851 | 14 | 678 | 21 | 12,173 | 0 | 0 |
| Core | 66 | 11,445 | 61 | 10,643 | 1 | 140 | 4 | 662 |
| Career | 78 | 11,073 | 59 | 8,758 | 1 | 196 | 18 | 2,119 |
| Scene | 54 | 10,677 | 4 | 79 | 20 | 5,371 | 30 | 5,227 |
| Science | 49 | 10,430 | 24 | 5,780 | 2 | 445 | 23 | 4,205 |
| Celestials | 40 | 10,360 | 19 | 2,698 | 20 | 7,338 | 1 | 324 |
| Modding | 51 | 9,485 | 19 | 2,451 | 5 | 2,461 | 27 | 4,573 |
| Orbit | 19 | 8,412 | 15 | 6,730 | 4 | 1,682 | 0 | 0 |
| Input | 43 | 8,182 | 3 | 252 | 2 | 769 | 38 | 7,161 |
| Aero | 18 | 7,896 | 12 | 1,781 | 5 | 5,201 | 1 | 914 |
| CommNet | 43 | 7,281 | 31 | 4,534 | 0 | 0 | 12 | 2,747 |
| Camera | 54 | 6,906 | 0 | 0 | 0 | 0 | 54 | 6,906 |
| Physics | 20 | 6,639 | 2 | 3,883 | 16 | 2,710 | 2 | 46 |
| Resources | 39 | 5,754 | 39 | 5,754 | 0 | 0 | 0 | 0 |
| PartModules-Wheels | 22 | 5,290 | 13 | 1,461 | 9 | 3,829 | 0 | 0 |
| PartModules-Resources | 27 | 4,911 | 22 | 2,753 | 5 | 2,158 | 0 | 0 |
| PartModules-Engines | 12 | 4,761 | 6 | 597 | 6 | 4,164 | 0 | 0 |
| PartModules-Control | 9 | 2,851 | 6 | 1,859 | 2 | 746 | 1 | 246 |
| Audio | 17 | 2,496 | 0 | 0 | 0 | 0 | 17 | 2,496 |
| Thermal | 18 | 2,464 | 15 | 2,210 | 0 | 0 | 3 | 254 |
| PartModules-Misc | 6 | 2,194 | 1 | 176 | 2 | 1,307 | 3 | 711 |
| Localization | 18 | 2,047 | 0 | 0 | 0 | 0 | 18 | 2,047 |
| Steam | 4 | 1,340 | 0 | 0 | 0 | 0 | 4 | 1,340 |
| Analytics | 3 | 965 | 0 | 0 | 0 | 0 | 3 | 965 |
| Misc | 7 | 423 | 0 | 0 | 0 | 0 | 7 | 423 |



## 4. What stands out

- **B is small in files but large in lines.** 238 files (8% of KSP files) hold 134,094 lines (24%). Re-architecture is concentrated in a few huge classes: Part (9,023), KerbalEVA (7,368), EditorLogic (5,490), Vessel (4,664), PQS (3,168), FlightIntegrator (2,776), ModuleProceduralFairing (2,220), ModuleEngines (2,217), PartLoader (2,205), ModuleDockingNode (2,059), FlightGlobals (2,003).
- **A is a third of the code and most of the career game.** 171,643 lines: contracts and FinePrint (34k), Making History mission runtime (28k), save/persistence, orbit maths, science/R&D, CommNet, resource flow, delta-v. This is the cheap, numerically verifiable part.
- **Gameplay rules live in UI classes**: `KSP/UI/Screens/StageManager` owns the staging sequence; `RDTechTree` loads the tech tree and answers unlock queries; `ExperimentResultDialogPage` computes lab/transmit/recovery science values; `NavBallBurnVector` computes burn time; `UIWarpToNextMorning` computes time-to-dawn; `LaunchSiteFacility`, `CrewTransfer`, `CrewHatchController`, `ExperimentTransfer`, `QuickSaveLoad`, `EditorMarker_CoM/CoT` hold rules. These are B (extract) or noted in their package.
- **`RUI/Algorithms` is not UI.** Its SCC flow graph is used by `PartSet` and `DeltaVStageInfo` for resource crossfeed ordering, so it is resource-flow core (tier A).
- **Native Unity behaviour sits under pure-looking code.** `FloatCurve` wraps `AnimationCurve.Evaluate` (native), and seeded contract, asteroid, comet and surface-feature generation calls `UnityEngine.Random.InitState` (native xorshift128). Quaternion constructors and all Transform maths are native too. These need golden capture (`curves_eval`, `unity_random`, `math_shim`) before any downstream golden can match.
- **Wheels run on a third-party vehicle physics library plus native PhysX.** VehiclePhysics zeroes PhysX tyre friction and applies its own tyre-friction curves (Flat/Lineal/Smooth/Parametric/Pacejka; Smooth by default); suspension still comes from the native WheelCollider.
- **Making History code is compiled into the base assembly** (463 files, 69k lines): 244 A files (runtime graph, tests, actions, awards, scoring), 204 C (editor UI). Its adjusters hook into 29 base-game PartModule types (`IsBlockingDecouple`, `ApplyThrottleAdjustment`, ...), so the module host (WP-06) must support adjusters from day one.
- **Breaking Ground code leaks into the base game**: `ROC*` surface features (including `PQSMod_ROCScatterQuad` in the terrain pipeline and `ROCScenario`, which destroys itself without the DLC), `ModuleGround*`, `ActiveJoint`, `ModuleJointMotor`, `LandClassROC` are all root-level files.
- **No C# mod compatibility survives**: `AssemblyLoader` and `KSP/IO` (the sandboxed plugin file API) are dropped. Config-level modding (ConfigNode, GameDatabase, part upgrades) is core.
- **Dead and test code ships in the assembly**: test PartModules (`ModuleJointMotorTest`, `ModuleEffectTest`, `AxisTestModule`), `KSP/Testing` (an in-game unit-test harness), `TestScripts/`, debug/cheat tooling (121 files in the Debug subsystem), procedural asteroid/comet test classes, and a Crypto Obfuscator stub (`A/-.cs`).
- **Third-party bulk is mostly unused**: Smooth has 104 files but only 7 are reachable; VehiclePhysics has 99, about 26 reachable; LibNoise has 45, 11 reachable. TMPro (121 files) disappears with the UI rewrite.
- **`Localizer` has the highest fan-in (802 files)** and `ConfigNode` the next (667). Both are needed before anything else can be ported, even though Localizer is tier C (a key lookup + format shim suffices for the core).
- **Tick order is explicit**: `Timing0..5`, `TimingPre` and `TimingFI` are script-execution-order proxies. They define the per-frame phase order the core scheduler must reproduce (WP-00).
- **Facility-level rules live in `GameVariables`** (a MonoBehaviour singleton): contract/strategy limits, crew and EVA limits, craft size/mass limits, DSN power, reward factors. Core career data, tier A.

## 5. Dependency order

What must exist before what. Each layer only needs the layers above it. Work packages are listed in brackets.

```
L0  Foundations      Core maths + Unity value-type/curve/Random shims [WP-01] -> ConfigNode/UrlDir [WP-02]
                     -> event bus, FSM, utils [WP-03] -> localization key/format shim [WP-51]
L1  Runtime & data   Core runtime skeleton: tick phases, scheduler, transform hierarchy [WP-00]
                     Save model + upgrade pipeline [WP-04] -> GameDatabase/PartLoader data side [WP-05]
                     -> module host + KSPField registry [WP-06]
L2  Space            Orbit maths [WP-07] -> celestial bodies/atmospheres/biomes [WP-09]
                     -> patched conics/maneuvers/alarms [WP-08] -> time warp + rails [WP-10]
                     LibNoise + PQSMod heights [WP-11]
L3  Craft model      Part tree [WP-13] -> vessel model [WP-14] -> resource flow [WP-15]
                     -> staging/decouple [WP-16] -> delta-v [WP-17]; craft files [WP-18]
L4  Physics          (Phase-0 spike decision) physics core shim [WP-20] -> load/unload, packing [WP-19]
                     -> aero [WP-21] -> thermal [WP-22]; PQS mesh/colliders [WP-12]
L5  Part modules     engines [WP-23], control/SAS [WP-24], docking [WP-25], fairings [WP-26],
                     deployables/chutes [WP-27], resource modules [WP-28], wheels [WP-29], misc [WP-30]
L6  Kerbals          roster/XP [WP-31] -> EVA [WP-32] -> inventory/EVA construction [WP-33]; IVA [WP-34]
L7  Career           science/R&D [WP-35], funds/rep/strategies/facilities/progress [WP-36]
                     -> contracts [WP-37] -> FinePrint/Sentinel [WP-38]; CommNet/KerbNet [WP-39]
L8  Editor & scenes  editor core [WP-40], scene flow/KSC [WP-42]
L9  Shell            UI framework [WP-45] -> HUD [WP-43], map [WP-44], screens [WP-46], editor UI [WP-41];
                     cameras [WP-47], rendering/FX [WP-48], audio [WP-49], input [WP-50]
L10 DLC              Breaking Ground robotics [WP-52], deployed science/ROC [WP-53];
                     Making History runtime [WP-54] -> node library [WP-55] -> mission editor UI [WP-56]
L11 Last             tutorials [WP-57], debug console [WP-58]; drop list [WP-59] needs no work
```

The packages of L0-L2 (WP-01..09, 11) do not depend on the physics model, so they can start before the Phase-0 physics spike concludes. The tier-A career packages (WP-31, 35-39) can also start early if they are developed against vessel/part query interfaces with fixture data from saves (WP-04). L4-L6 wait for the physics decision.

## 6. Top 40 highest-leverage files


Leverage = fan-in (distinct KSP files referencing a type declared in the file) + lines/25. So 25 lines of code weigh the same as one dependent file.

| # | File | Lines | Fan-in | Tier | Subsystem | Work package |
|---:|---|---:|---:|---|---|---|
| 1 | `ConfigNode.cs` | 4,344 | 667 | A | Save | WP-02 |
| 2 | `KSP/Localization/Localizer.cs` | 494 | 802 | C | Localization | WP-51 |
| 3 | `Part.cs` | 9,023 | 389 | B | Part | WP-13 |
| 4 | `GameEvents.cs` | 1,058 | 548 | A | Core | WP-03 |
| 5 | `Vessel.cs` | 4,664 | 393 | B | Vessel | WP-14 |
| 6 | `FlightGlobals.cs` | 2,003 | 431 | B | Vessel | WP-19 |
| 7 | `HighLogic.cs` | 355 | 466 | B | Scene | WP-42 |
| 8 | `GameSettings.cs` | 2,068 | 256 | C | Input | WP-50 |
| 9 | `KerbalEVA.cs` | 7,368 | 34 | B | Kerbals | WP-32 |
| 10 | `EditorLogic.cs` | 5,490 | 99 | B | Editor | WP-40 |
| 11 | `KSPUtil.cs` | 2,176 | 212 | A | Core | WP-03 |
| 12 | `CelestialBody.cs` | 1,231 | 241 | B | Celestials | WP-09 |
| 13 | `XKCDColors.cs` | 3,871 | 81 | C | UI | WP-45 |
| 14 | `Expansions/Missions/MENode.cs` | 1,255 | 176 | A | DLC-MakingHistory | WP-54 |
| 15 | `PartModule.cs` | 1,256 | 167 | B | Part | WP-06 |
| 16 | `PhysicsGlobals.cs` | 3,875 | 55 | A | Physics | WP-20 |
| 17 | `PQS.cs` | 3,168 | 79 | B | Terrain | WP-12 |
| 18 | `Planetarium.cs` | 314 | 188 | A | Celestials | WP-09 |
| 19 | `Vector3d.cs` | 354 | 184 | A | Core | WP-01 |
| 20 | `GameScenes.cs` | 14 | 194 | A | Scene | WP-42 |
| 21 | `Expansions/Missions/Editor/MissionEditorLogic.cs` | 2,639 | 87 | C | DLC-MakingHistory | WP-56 |
| 22 | `Expansions/Missions/Mission.cs` | 2,705 | 83 | A | DLC-MakingHistory | WP-54 |
| 23 | `Contracts/Contract.cs` | 2,020 | 106 | A | Contracts | WP-37 |
| 24 | `FinePrint/ContractDefs.cs` | 3,533 | 45 | A | Contracts | WP-38 |
| 25 | `iTween.cs` | 4,414 | 0 | C | UI | WP-45 |
| 26 | `GameDatabase.cs` | 1,541 | 111 | B | Modding | WP-05 |
| 27 | `InputLockManager.cs` | 156 | 165 | A | Input | WP-50 |
| 28 | `Orbit.cs` | 1,854 | 96 | A | Orbit | WP-07 |
| 29 | `KSPField.cs` | 56 | 167 | A | Part | WP-06 |
| 30 | `ProtoCrewMember.cs` | 1,375 | 110 | A | Kerbals | WP-31 |
| 31 | `DebugToolbar.cs` | 1,941 | 86 | C | Debug | WP-58 |
| 32 | `PartLoader.cs` | 2,205 | 74 | B | Part | WP-05 |
| 33 | `ControlTypes.cs` | 95 | 151 | A | Input | WP-50 |
| 34 | `ScreenMessages.cs` | 451 | 122 | C | FlightUI | WP-43 |
| 35 | `Game.cs` | 800 | 103 | A | Save | WP-04 |
| 36 | `ResearchAndDevelopment.cs` | 1,210 | 83 | A | Science | WP-35 |
| 37 | `UIPartActionWindow.cs` | 2,006 | 46 | C | FlightUI | WP-43 |
| 38 | `Expansions/ExpansionsLoader.cs` | 592 | 100 | B | Modding | WP-05 |
| 39 | `ShipConstruct.cs` | 971 | 84 | A | Save | WP-18 |
| 40 | `ProtoVessel.cs` | 1,262 | 72 | B | Save | WP-19 |



## 7. Work packages

60 packages (WP-00..WP-59), each sized for one Opus agent; WP-59 is the drop list. Every row of `tiering.csv` is assigned to exactly one package in `work_packages.csv` (rules in `scripts/wp_rules.py`). Third-party files are assigned to the package that ports or retires them.

Verification modes: **none**: 1, **numerically**: 26, **verify by inspection**: 16, **verify by scenario**: 17. 'numerically' = golden dataset comparison; 'verify by scenario' = scripted headless or in-shell scenario compared with KSP traces or expected outcomes; 'verify by inspection' = human or agent visual/behavioural check. The user does not want to test every feature, so the numerical goldens are concentrated where the risk is (maths, data, rules), and most UI packages are inspection-only.


| WP | Name | Files | Lines | A | B | C | 3p | Verify | Largest files |
|---|---|---:|---:|---:|---:|---:|---:|---|---|
| WP-00 | Core runtime skeleton: tick phases, timer/coroutine scheduler, double-precision transform hierarchy, component registry | 15 | 1,526 | 0 | 10 | 5 | 0 | verify by inspection | `TimingManager.cs`, `GameObjectExtension.cs`, `HierarchyUtil.cs`, `TransformExtension.cs` |
| WP-01 | Core maths, curves and Unity value-type shim | 19 | 3,548 | 19 | 0 | 0 | 0 | numerically | `UtilMath.cs`, `QuaternionD.cs`, `Matrix4x4D.cs`, `Vector3d.cs` |
| WP-02 | ConfigNode, GameData tree, value parsers | 11 | 5,982 | 11 | 0 | 0 | 0 | numerically | `ConfigNode.cs`, `UrlDir.cs`, `ParseExtensions.cs`, `KSPParseable.cs` |
| WP-03 | Event bus, FSM, misc core utilities | 42 | 7,095 | 42 | 0 | 0 | 0 | verify by inspection | `KSPUtil.cs`, `GameEvents.cs`, `EventData.cs`, `Simplex.cs` |
| WP-04 | Save model, persistence, save upgrade pipeline | 69 | 6,095 | 69 | 0 | 0 | 0 | numerically | `GameParameters.cs`, `Game.cs`, `CraftProfileInfo.cs`, `GamePersistence.cs` |
| WP-05 | Data loading: GameDatabase config side, PartLoader data side, resource library | 21 | 6,994 | 13 | 8 | 0 | 0 | numerically | `PartLoader.cs`, `GameDatabase.cs`, `DragCubeSystem.cs`, `ExpansionsLoader.cs` |
| WP-06 | Module host: PartModule/VesselModule/ScenarioModule, KSPField/Event/Action registry | 50 | 6,657 | 45 | 5 | 0 | 0 | numerically | `PartModule.cs`, `BaseField.cs`, `BaseAction.cs`, `PartModuleList.cs` |
| WP-07 | Orbit maths, targeting, transfer maths | 9 | 5,166 | 9 | 0 | 0 | 0 | numerically | `TransferMath.cs`, `Orbit.cs`, `Targeting.cs`, `OrbitUtil.cs` |
| WP-08 | Patched conics, maneuver nodes, alarm clock and transfer-planner logic | 15 | 3,432 | 15 | 0 | 0 | 0 | numerically | `AlarmClockScenario.cs`, `PatchedConics.cs`, `Trajectory.cs`, `PatchedConicSolver.cs` |
| WP-09 | Celestial bodies, planetarium, biomes, space objects | 39 | 10,036 | 19 | 20 | 0 | 0 | numerically | `CelestialBody.cs`, `PSystemSetup.cs`, `SpaceObjectCollider.cs`, `CometManager.cs` |
| WP-10 | Time warp, rails propagation, orbit driver | 5 | 1,475 | 2 | 3 | 0 | 0 | verify by scenario | `TimeWarp.cs`, `OrbitDriver.cs`, `WarpTransitionCalculator.cs`, `PositionTarget.cs` |
| WP-11 | LibNoise + PQSMod height/colour maths | 95 | 7,864 | 50 | 0 | 0 | 45 | numerically | `MapSO.cs`, `PQSMod_VertexPlanet.cs`, `PQSMod_MapDecal.cs`, `PQSMod_MapDecalTangent.cs` |
| WP-12 | PQS quadtree, mesh/collider build, PQSCity, land control | 20 | 9,326 | 0 | 20 | 0 | 0 | verify by scenario | `PQS.cs`, `PQ.cs`, `PQSLandControl.cs`, `PQSMeshPlanet.cs` |
| WP-13 | Part model and part tree | 34 | 11,473 | 28 | 6 | 0 | 0 | numerically | `Part.cs`, `PartVariant.cs`, `ModulePartVariants.cs`, `PartGeometryUtil.cs` |
| WP-14 | Vessel model, situations, mass properties | 17 | 6,461 | 14 | 3 | 0 | 0 | numerically | `Vessel.cs`, `FlightLogger.cs`, `FlightLog.cs`, `ModuleTripLogger.cs` |
| WP-15 | Resource flow | 40 | 5,750 | 40 | 0 | 0 | 0 | numerically | `PartSet.cs`, `ResourceMap.cs`, `ModuleResourceHandler.cs`, `PartResourceList.cs` |
| WP-16 | Staging, action groups, decouplers, decouple/couple tree ops | 13 | 2,668 | 9 | 4 | 0 | 0 | numerically | `StageManager.cs`, `ModuleDecouplerBase.cs`, `ModuleDecouple.cs`, `ActionGroupList.cs` |
| WP-17 | Delta-v calculation | 15 | 4,668 | 15 | 0 | 0 | 0 | numerically | `VesselDeltaV.cs`, `DeltaVStageInfo.cs`, `DeltaVEngineInfo.cs`, `DeltaVEngineStageSet.cs` |
| WP-18 | Craft files and ship construction | 3 | 2,571 | 2 | 1 | 0 | 0 | verify by scenario | `ShipConstruction.cs`, `ShipConstruct.cs`, `ShipTemplate.cs` |
| WP-19 | Vessel load/unload, pack/unpack, flight scene registry | 10 | 6,523 | 2 | 8 | 0 | 0 | verify by scenario | `FlightGlobals.cs`, `ProtoPartSnapshot.cs`, `ProtoVessel.cs`, `VesselPrecalculate.cs` |
| WP-20 | Physics core: rigid-body/joint shim, floating origin, Krakensbane, collisions | 17 | 6,854 | 2 | 15 | 0 | 0 | verify by scenario | `PhysicsGlobals.cs`, `PartBuoyancy.cs`, `PartJoint.cs`, `FloatingOrigin.cs` |
| WP-21 | Aerodynamics | 16 | 6,211 | 12 | 4 | 0 | 0 | numerically | `FlightIntegrator.cs`, `DragCubeList.cs`, `ModuleControlSurface.cs`, `ModuleLiftingSurface.cs` |
| WP-22 | Thermal model | 15 | 2,210 | 15 | 0 | 0 | 0 | numerically | `ModuleCoreHeat.cs`, `ModuleActiveRadiator.cs`, `TemperatureModel.cs`, `ModuleAblator.cs` |
| WP-23 | Engines, RCS, gimbals, intakes | 12 | 4,761 | 6 | 6 | 0 | 0 | numerically | `ModuleEngines.cs`, `ModuleRCS.cs`, `ModuleGimbal.cs`, `MultiModeEngine.cs` |
| WP-24 | Command, SAS/autopilot, reaction wheels, control state | 10 | 2,941 | 7 | 3 | 0 | 0 | verify by scenario | `VesselAutopilot.cs`, `ModuleCommand.cs`, `ModuleReactionWheel.cs`, `FlightCtrlState.cs` |
| WP-25 | Docking, grapple, struts, vessel merge/split | 4 | 3,270 | 1 | 3 | 0 | 0 | verify by scenario | `ModuleDockingNode.cs`, `ModuleGrappleNode.cs`, `CModuleStrut.cs`, `DockedVesselInfo.cs` |
| WP-26 | Fairings, cargo bays, jettison, shielding, launch clamps | 18 | 7,390 | 6 | 12 | 0 | 0 | numerically | `ModuleProceduralFairing.cs`, `ModuleCargoBay.cs`, `FairingPanel.cs`, `CompoundPart.cs` |
| WP-27 | Parachutes, deployables, animation-driven modules | 11 | 4,876 | 2 | 9 | 0 | 0 | verify by scenario | `ModuleDeployablePart.cs`, `ModuleParachute.cs`, `ModuleAnimateGeneric.cs`, `ModuleAnimationGroup.cs` |
| WP-28 | Resource-processing modules (converters, drills, generators, solar, scanners) | 26 | 4,514 | 22 | 4 | 0 | 0 | numerically | `BaseConverter.cs`, `ModuleResourceDrain.cs`, `ModuleResourceHarvester.cs`, `ModuleOrbitalScanner.cs` |
| WP-29 | Wheels and landing gear (VehiclePhysics subset port) | 163 | 29,217 | 12 | 8 | 0 | 143 | verify by scenario | `VehicleBase.cs`, `TextureCanvas.cs`, `ModuleWheelBase.cs`, `GColor.cs` |
| WP-30 | Misc part modules (lights, flags, variants, animation helpers) | 3 | 1,483 | 1 | 2 | 0 | 0 | verify by inspection | `ModulePartFirework.cs`, `FlagSite.cs`, `ModuleGPS.cs` |
| WP-31 | Kerbal roster, experience, crew | 33 | 5,310 | 33 | 0 | 0 | 0 | numerically | `KerbalRoster.cs`, `ProtoCrewMember.cs`, `VesselCrewManifest.cs`, `PartCrewManifest.cs` |
| WP-32 | EVA kerbal controller | 7 | 8,818 | 0 | 7 | 0 | 0 | verify by scenario | `KerbalEVA.cs`, `Kerbal.cs`, `RetractableLadder.cs`, `FlightEVA.cs` |
| WP-33 | Inventory and EVA construction | 16 | 7,547 | 5 | 4 | 7 | 0 | verify by scenario | `ModuleInventoryPart.cs`, `EVAConstructionModeEditor.cs`, `UIPartActionControllerInventory.cs`, `EVAConstructionModeController.cs` |
| WP-34 | IVA and internal props | 75 | 10,295 | 0 | 7 | 68 | 0 | verify by inspection | `HelmetSuitPickerWindow.cs`, `InternalModel.cs`, `BaseCrewAssignmentDialog.cs`, `KerbalPortrait.cs` |
| WP-35 | Science, R&D and tech tree | 26 | 6,225 | 24 | 2 | 0 | 0 | numerically | `ModuleScienceExperiment.cs`, `ResearchAndDevelopment.cs`, `ModuleScienceContainer.cs`, `ScienceUtil.cs` |
| WP-36 | Funds, reputation, strategies, facilities, recovery, progress tracking | 61 | 9,201 | 59 | 2 | 0 | 0 | numerically | `Strategy.cs`, `GameVariables.cs`, `ProgressTracking.cs`, `ScenarioUpgradeableFacilities.cs` |
| WP-37 | Contract system core and stock templates | 69 | 13,867 | 69 | 0 | 0 | 0 | numerically | `Contract.cs`, `ContractSystem.cs`, `RecoverAsset.cs`, `PartTest.cs` |
| WP-38 | FinePrint contracts, waypoints, Sentinel | 77 | 21,322 | 73 | 2 | 2 | 0 | numerically | `ContractDefs.cs`, `VesselUtilities.cs`, `SurveyContract.cs`, `SatelliteContract.cs` |
| WP-39 | CommNet and KerbNet | 31 | 4,534 | 31 | 0 | 0 | 0 | numerically | `ModuleDataTransmitter.cs`, `CommNetVessel.cs`, `ModuleKerbNetAccess.cs`, `CommNetwork.cs` |
| WP-40 | Editor core: placement, symmetry, gizmos, pre-flight checks | 64 | 10,865 | 53 | 11 | 0 | 0 | verify by scenario | `EditorLogic.cs`, `EditorLogicBase.cs`, `GizmoOffset.cs`, `GizmoRotate.cs` |
| WP-41 | Editor UI (part list, categories, craft browser, action-group UI) | 57 | 17,096 | 0 | 0 | 57 | 0 | verify by inspection | `PartCategorizer.cs`, `EditorActionGroups.cs`, `CraftBrowserDialog.cs`, `EditorPartIcon.cs` |
| WP-42 | Scene flow, KSC, facilities, launch sites | 37 | 5,522 | 4 | 10 | 23 | 0 | verify by scenario | `SpaceCenterBuilding.cs`, `DestructibleBuilding.cs`, `FlightDriver.cs`, `EditorDriver.cs` |
| WP-43 | Flight HUD (navball, staging UI, PAW, gauges, messages, crew portraits) | 177 | 29,718 | 0 | 1 | 176 | 0 | verify by inspection | `UIPartActionWindow.cs`, `StageIcon.cs`, `TransferTypeSimple.cs`, `KerbNetDialog.cs` |
| WP-44 | Map view and tracking station | 53 | 14,344 | 0 | 0 | 53 | 0 | verify by inspection | `SpaceTracking.cs`, `OrbitTargeter.cs`, `OrbitRendererBase.cs`, `MapNode.cs` |
| WP-45 | UI framework: DialogGUI, popups, tooltips, app launcher, widgets, rich text | 377 | 59,604 | 0 | 0 | 234 | 143 | verify by inspection | `TMP_Text.cs`, `iTween.cs`, `XKCDColors.cs`, `TMP_InputField.cs` |
| WP-46 | UI screens: main menu, settings, KSC building screens, load/save dialogs | 153 | 26,726 | 0 | 0 | 153 | 0 | verify by inspection | `VesselSpawnDialog.cs`, `MainMenu.cs`, `LoadGameDialog.cs`, `MessageSystem.cs` |
| WP-47 | Cameras | 50 | 6,302 | 0 | 0 | 50 | 0 | verify by inspection | `FlightCamera.cs`, `PlanetariumCamera.cs`, `CameraManager.cs`, `SpaceCenterCamera2.cs` |
| WP-48 | Rendering, FX, scaled-space visuals, asset import | 224 | 34,345 | 0 | 0 | 211 | 13 | verify by inspection | `DatabaseLoaderModel_DAE.cs`, `DAE.cs`, `ModuleLight.cs`, `PartReader.cs` |
| WP-49 | Audio | 17 | 2,496 | 0 | 0 | 17 | 0 | verify by inspection | `MusicLogic.cs`, `AudioMultiPooledFX.cs`, `AudioFX.cs`, `AlarmClockScenarioAudio.cs` |
| WP-50 | Input, bindings, input locks | 33 | 7,026 | 3 | 2 | 28 | 0 | verify by scenario | `GameSettings.cs`, `InputSettings.cs`, `FlightInputHandler.cs`, `Mouse.cs` |
| WP-51 | Localization | 18 | 2,047 | 0 | 0 | 18 | 0 | verify by inspection | `Localizer.cs`, `Language.cs`, `LanguageDefinition.cs`, `FontLoader.cs` |
| WP-52 | Breaking Ground robotics | 29 | 13,722 | 7 | 10 | 12 | 0 | verify by scenario | `BaseServo.cs`, `ModuleRoboticController.cs`, `ModuleRobotArmScanner.cs`, `RoboticControllerWindow.cs` |
| WP-53 | Breaking Ground deployed science and surface features | 27 | 7,420 | 14 | 11 | 2 | 0 | numerically | `DeployedScienceExperiment.cs`, `ModuleGroundSciencePart.cs`, `DeployedScience.cs`, `ROCManager.cs` |
| WP-54 | Making History mission runtime: Mission, MENode graph, flow, MissionSystem | 122 | 19,899 | 112 | 10 | 0 | 0 | verify by scenario | `Mission.cs`, `MissionSystem.cs`, `MENode.cs`, `MissionSituation.cs` |
| WP-55 | Making History node library: tests, actions, adjusters | 139 | 15,470 | 132 | 5 | 2 | 0 | verify by scenario | `TestDistance.cs`, `TestOrbitParams.cs`, `ActionCreateVessel.cs`, `ActionPartRepair.cs` |
| WP-56 | Making History mission editor UI | 202 | 33,746 | 0 | 0 | 202 | 0 | verify by inspection | `MissionEditorLogic.cs`, `MissionPlayDialog.cs`, `MissionsApp.cs`, `MEGUINode.cs` |
| WP-57 | Tutorials | 38 | 14,611 | 0 | 0 | 38 | 0 | verify by inspection | `TutorialEditorAdvanced.cs`, `AdvancedTutorial.cs`, `BasicTutorial.cs`, `METutorialScenario.cs` |
| WP-58 | Debug tools and cheats -> console commands | 121 | 14,405 | 0 | 0 | 121 | 0 | verify by inspection | `DebugToolbar.cs`, `AeroGUI.cs`, `RnDDebugUtil.cs`, `ConsoleCommands.cs` |
| WP-59 | Drop list (platform, analytics, test scripts, obfuscator stubs, unused third-party) | 158 | 22,811 | 0 | 0 | 41 | 117 | none | `Enumerable.cs`, `Slinq.cs`, `Tuple.cs`, `AssemblyLoader.cs` |


### Package details

**WP-00: Core runtime skeleton: tick phases, timer/coroutine scheduler, double-precision transform hierarchy, component registry** (15 files, 1,526 lines; A 0 / B 10 / C 5). Verify: verify by inspection.
- Depends on: WP-01.
- Acceptance: unit tests: tick order matches KSP Timing0..5/TimingPre/TimingFI phases; coroutine/timer semantics (WaitForSeconds/WaitForFixedUpdate/Invoke); transform maths vs `math_shim`.
- Largest files: `TimingManager.cs`, `GameObjectExtension.cs`, `HierarchyUtil.cs`, `TransformExtension.cs`, `Timing0.cs`, `Timing1.cs`.
- Note: New code (no direct KSP counterpart beyond Timing*/CoroutineHost). It sets the host contract that every B package builds on. Needs the Phase-0 architecture decisions.

**WP-01: Core maths, curves and Unity value-type shim** (19 files, 3,548 lines; A 19 / B 0 / C 0). Verify: numerically.
- Depends on: none.
- Acceptance: golden `math_shim` (Vector3/Quaternion/Mathf incl. Round-to-even, Sign(0)=1, AngleAxis/LookRotation/FromToRotation), `curves_eval` (every FloatCurve/AnimationCurve key set in GameData sampled at 64 points, to 1 ulp) and `unity_random` (Unity Random sequences per seed, bit-exact).
- Largest files: `UtilMath.cs`, `UnityEngine/QuaternionD.cs`, `Matrix4x4D.cs`, `Vector3d.cs`, `DoubleCurve.cs`, `UnityEngine/Vector4d.cs`.
- Note: Includes the shims for Unity value types, `Mathf`, `AnimationCurve` and `Random` specified in unity-api-surface.md. Bit-exactness matters because everything downstream is compared to goldens.

**WP-02: ConfigNode, GameData tree, value parsers** (11 files, 5,982 lines; A 11 / B 0 / C 0). Verify: numerically.
- Depends on: WP-01.
- Acceptance: golden `confignode_roundtrip`: parse+serialise every .cfg in GameData and sample .sfs/.craft, byte-identical; typed parse of all KSPField values.
- Largest files: `ConfigNode.cs`, `UrlDir.cs`, `ParseExtensions.cs`, `KSPParseable.cs`, `IntTypeParser.cs`, `DoubleTypeParser.cs`.
- Note: Highest fan-in file in the game (ConfigNode, 667 dependants). Includes the reflection-based object<->node mapping, which becomes generated C++ serialisers.

**WP-03: Event bus, FSM, misc core utilities** (42 files, 7,095 lines; A 42 / B 0 / C 0). Verify: verify by inspection.
- Depends on: WP-01.
- Acceptance: unit tests: GameEvents subscribe/fire order, KFSM transitions; KSPUtil time/date/unit formatting vs golden `format_strings`.
- Largest files: `KSPUtil.cs`, `GameEvents.cs`, `EventData.cs`, `Simplex.cs`, `SimplexNoise.cs`, `ListExtension.cs`.

**WP-04: Save model, persistence, save upgrade pipeline** (69 files, 6,095 lines; A 69 / B 0 / C 0). Verify: numerically.
- Depends on: WP-02, WP-03.
- Acceptance: golden `sfs_roundtrip` (load+save stock and test saves byte-identical) and `save_upgrade` (legacy saves upgraded identically).
- Largest files: `GameParameters.cs`, `Game.cs`, `KSP/UI/Screens/CraftProfileInfo.cs`, `GamePersistence.cs`, `SaveUpgradePipeline/PartReplace.cs`, `SaveUpgradePipeline/SaveUpgradePipeline.cs`.

**WP-05: Data loading: GameDatabase config side, PartLoader data side, resource library** (21 files, 6,994 lines; A 13 / B 8 / C 0). Verify: numerically.
- Depends on: WP-02, WP-04.
- Acceptance: golden `part_database`: AvailablePart dump (fields, modules, resources, cost, mass, tech, drag cubes) for every stock part.
- Largest files: `PartLoader.cs`, `GameDatabase.cs`, `DragCubeSystem.cs`, `Expansions/ExpansionsLoader.cs`, `PartUpgradeHandler.cs`, `AvailablePart.cs`.
- Note: Config side only. Model/texture import is WP-48. `DragCubeSystem` is listed here: import KSP's `PartDatabase.cfg` cube cache first.

**WP-06: Module host: PartModule/VesselModule/ScenarioModule, KSPField/Event/Action registry** (50 files, 6,657 lines; A 45 / B 5 / C 0). Verify: numerically.
- Depends on: WP-02, WP-03, WP-05.
- Acceptance: golden `module_persistence`: every stock module's OnSave output after OnLoad of its config; event/action lists per part.
- Largest files: `PartModule.cs`, `BaseField.cs`, `BaseAction.cs`, `PartModuleList.cs`, `BaseAxisField.cs`, `BaseFieldList.cs`.
- Note: The KSPField/KSPEvent/KSPAction reflection layer becomes a C++ field registry (codegen or macros). Must support part-module adjusters (Making History hooks call into base modules).

**WP-07: Orbit maths, targeting, transfer maths** (9 files, 5,166 lines; A 9 / B 0 / C 0). Verify: numerically.
- Depends on: WP-01.
- Acceptance: golden `orbit_propagation` (state vectors at UT, anomaly conversions, Ap/Pe/AN/DN times for 10k random orbits), `closest_approach`, `transfer_planner`.
- Largest files: `TransferMath.cs`, `Orbit.cs`, `Targeting.cs`, `OrbitUtil.cs`, `OrbitSnapshot.cs`, `DirectionTarget.cs`.

**WP-08: Patched conics, maneuver nodes, alarm clock and transfer-planner logic** (15 files, 3,432 lines; A 15 / B 0 / C 0). Verify: numerically.
- Depends on: WP-07, WP-09.
- Acceptance: golden `patched_conics` (patch chains, SOI transition UTs, encounters for scripted trajectories and maneuver previews) and `alarm_times`.
- Largest files: `AlarmClockScenario.cs`, `PatchedConics.cs`, `Trajectory.cs`, `PatchedConicSolver.cs`, `AlarmTypeBase.cs`, `AlarmTypeManeuver.cs`.

**WP-09: Celestial bodies, planetarium, biomes, space objects** (39 files, 10,036 lines; A 19 / B 20 / C 0). Verify: numerically.
- Depends on: WP-02, WP-07.
- Acceptance: golden `body_catalog` + `atmosphere_curves` (p/T/rho/speed of sound on an altitude grid incl. latitude/sun offsets) + `biome_lookup` (biome at lat/lon grid).
- Largest files: `CelestialBody.cs`, `PSystemSetup.cs`, `SpaceObjectCollider.cs`, `CometManager.cs`, `ModuleComet.cs`, `DiscoveryInfo.cs`.
- Note: CelestialBody maths (atmosphere, gravity, rotation, lat/lon) is extracted as A. The MonoBehaviour/scaled-space shell side is B.

**WP-10: Time warp, rails propagation, orbit driver** (5 files, 1,475 lines; A 2 / B 3 / C 0). Verify: verify by scenario.
- Depends on: WP-00, WP-07, WP-09.
- Acceptance: scenario `warp_rules`: allowed warp rates vs altitude/situation, auto-warp to node, rails<->physics transitions preserve state vectors (1e-6 m).
- Largest files: `TimeWarp.cs`, `OrbitDriver.cs`, `WarpTransitionCalculator.cs`, `PositionTarget.cs`, `WarpTransition.cs`.

**WP-11: LibNoise + PQSMod height/colour maths** (95 files, 7,864 lines; A 50 / B 0 / C 0 / third-party 45). Verify: numerically.
- Depends on: WP-01, WP-02, WP-09.
- Acceptance: golden `terrain_heights`: PQS surface height and vertex colour at a lat/lon grid for every body, bit-exact.
- Largest files: `MapSO.cs`, `PQSMod_VertexPlanet.cs`, `PQSMod_MapDecal.cs`, `PQSMod_MapDecalTangent.cs`, `PQSMod_LandClassController.cs`, `LibNoise/GradientNoiseBasis.cs`.

**WP-12: PQS quadtree, mesh/collider build, PQSCity, land control** (20 files, 9,326 lines; A 0 / B 20 / C 0). Verify: verify by scenario.
- Depends on: WP-00, WP-11, WP-20.
- Acceptance: scenario `ksc_ground`: KSC pad/runway/collider heights match KSP; collider heights match `terrain_heights`; LOD inspected.
- Largest files: `PQS.cs`, `PQ.cs`, `PQSLandControl.cs`, `PQSMeshPlanet.cs`, `PQSCity2.cs`, `PQSCache.cs`.

**WP-13: Part model and part tree** (34 files, 11,473 lines; A 28 / B 6 / C 0). Verify: numerically.
- Depends on: WP-00, WP-05, WP-06.
- Acceptance: golden `craft_tree`: load every stock .craft; parent/child, attach nodes, symmetry groups, persistent ids, mass/cost equal.
- Largest files: `Part.cs`, `PartVariant.cs`, `ModulePartVariants.cs`, `PartGeometryUtil.cs`, `AttachNode.cs`, `ModuleDisplaceTweak.cs`.
- Note: Part.cs (9,023 lines) is the single largest re-architecture item. Split internally: tree/attach, mass/CoM, resources, crew, thermal state, joints, packing.

**WP-14: Vessel model, situations, mass properties** (17 files, 6,461 lines; A 14 / B 3 / C 0). Verify: numerically.
- Depends on: WP-13, WP-09.
- Acceptance: golden `vessel_state`: CoM, mass, MOI, situation, landed-at, control level for sample vessels.
- Largest files: `Vessel.cs`, `FlightLogger.cs`, `FlightLog.cs`, `ModuleTripLogger.cs`, `ProtoTargetInfo.cs`, `VesselValues.cs`.

**WP-15: Resource flow** (40 files, 5,750 lines; A 40 / B 0 / C 0). Verify: numerically.
- Depends on: WP-13.
- Acceptance: golden `resource_flow`: per-tick draw per part on stock craft (asparagus, crossfeed toggles, priorities, fuel lines, flow modes).
- Largest files: `PartSet.cs`, `ResourceMap.cs`, `ModuleResourceHandler.cs`, `PartResourceList.cs`, `ResourceGameSettings.cs`, `ResourceUtilities.cs`.

**WP-16: Staging, action groups, decouplers, decouple/couple tree ops** (13 files, 2,668 lines; A 9 / B 4 / C 0). Verify: numerically.
- Depends on: WP-13, WP-15.
- Acceptance: golden `staging_sequence`: inverse stages and separation indices for stock craft; vessel split results after each decouple.
- Largest files: `KSP/UI/Screens/StageManager.cs`, `ModuleDecouplerBase.cs`, `ModuleDecouple.cs`, `ActionGroupList.cs`, `ModuleAnchoredDecoupler.cs`, `KSPActionGroup.cs`.
- Note: Includes extracting the staging rules from `KSP/UI/Screens/StageManager.cs`.

**WP-17: Delta-v calculation** (15 files, 4,668 lines; A 15 / B 0 / C 0). Verify: numerically.
- Depends on: WP-15, WP-16, WP-23.
- Acceptance: golden `deltav_readout`: per-stage dV/TWR/ISP/burn time (vac, ASL, per body) for stock craft.
- Largest files: `VesselDeltaV.cs`, `DeltaVStageInfo.cs`, `DeltaVEngineInfo.cs`, `DeltaVEngineStageSet.cs`, `DeltaVPartInfo.cs`, `DeltaVGlobals.cs`.

**WP-18: Craft files and ship construction** (3 files, 2,571 lines; A 2 / B 1 / C 0). Verify: verify by scenario.
- Depends on: WP-05, WP-13.
- Acceptance: scenario `launch_craft`: every stock craft loads and spawns at pad/runway; costs match `part_database`.
- Largest files: `ShipConstruction.cs`, `ShipConstruct.cs`, `ShipTemplate.cs`.

**WP-19: Vessel load/unload, pack/unpack, flight scene registry** (10 files, 6,523 lines; A 2 / B 8 / C 0). Verify: verify by scenario.
- Depends on: WP-04, WP-10, WP-14.
- Acceptance: scenario `save_load_cycle`: positions/velocities within tolerance after load/quicksave/quickload; vessel switching and physics-range rules.
- Largest files: `FlightGlobals.cs`, `ProtoPartSnapshot.cs`, `ProtoVessel.cs`, `VesselPrecalculate.cs`, `QuickSaveLoad.cs`, `OrbitPhysicsManager.cs`.

**WP-20: Physics core: rigid-body/joint shim, floating origin, Krakensbane, collisions** (17 files, 6,854 lines; A 2 / B 15 / C 0). Verify: verify by scenario.
- Depends on: WP-00, WP-14, Phase-0 physics spike.
- Acceptance: scenarios `free_fall`, `physics_orbit_hold` (energy drift), `crash_tolerance`, `joint_stiffness` vs KSP traces.
- Largest files: `PhysicsGlobals.cs`, `PartBuoyancy.cs`, `PartJoint.cs`, `FloatingOrigin.cs`, `ModulePhysicMaterial.cs`, `Krakensbane.cs`.
- Note: Blocked on the Phase-0 physics spike (FEASIBILITY.md section 6): reduced rigid-body model vs Chaos/Jolt jointed tree.

**WP-21: Aerodynamics** (16 files, 6,211 lines; A 12 / B 4 / C 0). Verify: numerically.
- Depends on: WP-05, WP-09, WP-14, WP-20.
- Acceptance: golden `aero_forces` (per-part drag/lift vs velocity, density, Mach, AoA, occlusion) + scenario `ascent_profile`.
- Largest files: `FlightIntegrator.cs`, `DragCubeList.cs`, `ModuleControlSurface.cs`, `ModuleLiftingSurface.cs`, `OcclusionData.cs`, `ModuleAeroSurface.cs`.
- Note: FlightIntegrator's aero maths (drag cubes, lift curves, Mach/pressure terms) is A and can be transpiled ahead of WP-20 against `aero_forces`.

**WP-22: Thermal model** (15 files, 2,210 lines; A 15 / B 0 / C 0). Verify: numerically.
- Depends on: WP-09, WP-21.
- Acceptance: golden `thermal_step`: skin/internal temperatures after N steps for re-entry and engine-heating cases.
- Largest files: `ModuleCoreHeat.cs`, `ModuleActiveRadiator.cs`, `TemperatureModel.cs`, `ModuleAblator.cs`, `PartThermalData.cs`, `ThermalLink.cs`.
- Note: FlightIntegrator's thermal solver (conduction graph, convection, radiation, shock heating) is A. Its integration hook into the physics tick is B.

**WP-23: Engines, RCS, gimbals, intakes** (12 files, 4,761 lines; A 6 / B 6 / C 0). Verify: numerically.
- Depends on: WP-06, WP-09, WP-15, WP-20.
- Acceptance: golden `engine_thrust`: thrust/ISP/fuel flow vs throttle, pressure, velocity; flameout; scenario `ascent_profile`.
- Largest files: `ModuleEngines.cs`, `ModuleRCS.cs`, `ModuleGimbal.cs`, `MultiModeEngine.cs`, `ModuleResourceIntake.cs`, `ModuleEnginesFX.cs`.

**WP-24: Command, SAS/autopilot, reaction wheels, control state** (10 files, 2,941 lines; A 7 / B 3 / C 0). Verify: verify by scenario.
- Depends on: WP-06, WP-14, WP-20.
- Acceptance: scenario `sas_hold`: attitude error vs time per SAS mode against KSP traces; torque availability vs golden.
- Largest files: `VesselAutopilot.cs`, `ModuleCommand.cs`, `ModuleReactionWheel.cs`, `FlightCtrlState.cs`, `ModuleSAS.cs`, `SteeringObject.cs`.

**WP-25: Docking, grapple, struts, vessel merge/split** (4 files, 3,270 lines; A 1 / B 3 / C 0). Verify: verify by scenario.
- Depends on: WP-13, WP-16, WP-20.
- Acceptance: scenario `docking`: dock/undock, same-vessel docking, merged-vessel persistence and staging.
- Largest files: `ModuleDockingNode.cs`, `ModuleGrappleNode.cs`, `CompoundParts/CModuleStrut.cs`, `DockedVesselInfo.cs`.

**WP-26: Fairings, cargo bays, jettison, shielding, launch clamps** (18 files, 7,390 lines; A 6 / B 12 / C 0). Verify: numerically.
- Depends on: WP-13, WP-21.
- Acceptance: golden `shielding`: shielded part sets and fairing mass/cost for stock craft; fairing mesh inspected.
- Largest files: `ModuleProceduralFairing.cs`, `ModuleCargoBay.cs`, `ProceduralFairings/FairingPanel.cs`, `CompoundPart.cs`, `ModuleJettison.cs`, `ModuleServiceModule.cs`.

**WP-27: Parachutes, deployables, animation-driven modules** (11 files, 4,876 lines; A 2 / B 9 / C 0). Verify: verify by scenario.
- Depends on: WP-00, WP-06, WP-21.
- Acceptance: scenario `parachute_descent` (descent-rate golden) and `deploy_states` (deploy/retract/break).
- Largest files: `ModuleDeployablePart.cs`, `ModuleParachute.cs`, `ModuleAnimateGeneric.cs`, `ModuleAnimationGroup.cs`, `ModuleDeployableSolarPanel.cs`, `ModuleEvaChute.cs`.

**WP-28: Resource-processing modules (converters, drills, generators, solar, scanners)** (26 files, 4,514 lines; A 22 / B 4 / C 0). Verify: numerically.
- Depends on: WP-06, WP-09, WP-15.
- Acceptance: golden `converter_rates`: outputs over time with EC/heat limits and crew bonuses; solar output vs sun angle/distance.
- Largest files: `BaseConverter.cs`, `ModuleResourceDrain.cs`, `ModuleResourceHarvester.cs`, `ModuleOrbitalScanner.cs`, `ModuleAsteroidDrill.cs`, `ModuleCometDrill.cs`.

**WP-29: Wheels and landing gear (VehiclePhysics subset port)** (163 files, 29,217 lines; A 12 / B 8 / C 0 / third-party 143). Verify: verify by scenario.
- Depends on: WP-06, WP-12, WP-20.
- Acceptance: scenarios `rover_drive` (top speed, brake distance, steering on runway) and `gear_settle` (suspension at rest).
- Largest files: `VehiclePhysics/VehicleBase.cs`, `EdyCommonTools/TextureCanvas.cs`, `ModuleWheelBase.cs`, `EdyCommonTools/GColor.cs`, `VehiclePhysics/Gearbox.cs`, `ModuleWheels/ModuleWheelDeployment.cs`.
- Note: Ports the reachable VehiclePhysics/EdyCommonTools subset (third-party.md). PhysX WheelCollider suspension is native, so behaviour is tuned against scenario traces.

**WP-30: Misc part modules (lights, flags, variants, animation helpers)** (3 files, 1,483 lines; A 1 / B 2 / C 0). Verify: verify by inspection.
- Depends on: WP-06.
- Acceptance: behaviour inspection; variant switching changes mass/cost per `part_database`.
- Largest files: `ModulePartFirework.cs`, `FlagSite.cs`, `ModuleGPS.cs`.

**WP-31: Kerbal roster, experience, crew** (33 files, 5,310 lines; A 33 / B 0 / C 0). Verify: numerically.
- Depends on: WP-03, WP-04.
- Acceptance: golden `roster_xp`: XP/levels from flight logs; hiring costs; respawn timers.
- Largest files: `KerbalRoster.cs`, `ProtoCrewMember.cs`, `VesselCrewManifest.cs`, `PartCrewManifest.cs`, `Experience/ExperienceTrait.cs`, `CrewGenerator.cs`.

**WP-32: EVA kerbal controller** (7 files, 8,818 lines; A 0 / B 7 / C 0). Verify: verify by scenario.
- Depends on: WP-12, WP-20, WP-31.
- Acceptance: scenario `eva_basics`: walk/run/swim speeds, jetpack dV, ladder grab, seat boarding, flag planting.
- Largest files: `KerbalEVA.cs`, `Kerbal.cs`, `RetractableLadder.cs`, `FlightEVA.cs`, `CrewHatchController.cs`, `CrewTransfer.cs`.

**WP-33: Inventory and EVA construction** (16 files, 7,547 lines; A 5 / B 4 / C 7). Verify: verify by scenario.
- Depends on: WP-13, WP-32, WP-40.
- Acceptance: scenario `eva_construction`: pick/attach/detach parts in flight; inventory mass/cost totals.
- Largest files: `ModuleInventoryPart.cs`, `EVAConstructionModeEditor.cs`, `UIPartActionControllerInventory.cs`, `EVAConstructionModeController.cs`, `UIPartActionInventory.cs`, `UIPartActionInventorySlot.cs`.

**WP-34: IVA and internal props** (75 files, 10,295 lines; A 0 / B 7 / C 68). Verify: verify by inspection.
- Depends on: WP-31, WP-47.
- Acceptance: inspection of IVA instruments against flight state.
- Largest files: `HelmetSuitPickerWindow.cs`, `InternalModel.cs`, `KSP/UI/BaseCrewAssignmentDialog.cs`, `KSP/UI/Screens/Flight/KerbalPortrait.cs`, `kerbalExpressionSystem.cs`, `KSP/UI/Screens/AstronautComplex.cs`.

**WP-35: Science, R&D and tech tree** (26 files, 6,225 lines; A 24 / B 2 / C 0). Verify: numerically.
- Depends on: WP-04, WP-06, WP-09.
- Acceptance: golden `science_values` (subject value by body/situation/biome, diminishing returns, transmission) and `techtree`.
- Largest files: `ModuleScienceExperiment.cs`, `ResearchAndDevelopment.cs`, `ModuleScienceContainer.cs`, `ScienceUtil.cs`, `ModuleScienceLab.cs`, `KSP/UI/Screens/RDTechTree.cs`.
- Note: Includes extracting tech-tree load/query rules from `KSP/UI/Screens/RDTechTree.cs` and science-value rules from `ExperimentResultDialogPage.cs`.

**WP-36: Funds, reputation, strategies, facilities, recovery, progress tracking** (61 files, 9,201 lines; A 59 / B 2 / C 0). Verify: numerically.
- Depends on: WP-03, WP-04.
- Acceptance: golden `career_currency` (recovery value, strategy conversions, facility costs/limits) + scenario `progress_events`.
- Largest files: `Strategies/Strategy.cs`, `GameVariables.cs`, `ProgressTracking.cs`, `ScenarioUpgradeableFacilities.cs`, `Strategies/StrategySystem.cs`, `ScenarioAchievements.cs`.

**WP-37: Contract system core and stock templates** (69 files, 13,867 lines; A 69 / B 0 / C 0). Verify: numerically.
- Depends on: WP-08, WP-14, WP-35, WP-36.
- Acceptance: golden `contract_generation`: seeded offers, parameters and rewards from fixed save states; completion scenarios.
- Largest files: `Contracts/Contract.cs`, `Contracts/ContractSystem.cs`, `Contracts/Templates/RecoverAsset.cs`, `Contracts/Templates/PartTest.cs`, `Contracts/ContractParameter.cs`, `Contracts/Templates/VesselRepairContract.cs`.
- Note: Seeded generation uses `UnityEngine.Random.InitState`, so it needs WP-01's bit-exact Unity Random for `contract_generation` goldens.

**WP-38: FinePrint contracts, waypoints, Sentinel** (77 files, 21,322 lines; A 73 / B 2 / C 2). Verify: numerically.
- Depends on: WP-11, WP-28, WP-37.
- Acceptance: golden `contract_generation` (FinePrint types) + waypoint positions; completion scenarios.
- Largest files: `FinePrint/ContractDefs.cs`, `FinePrint/Utilities/VesselUtilities.cs`, `FinePrint/Contracts/SurveyContract.cs`, `FinePrint/Contracts/SatelliteContract.cs`, `FinePrint/Contracts/ExplorationContract.cs`, `FinePrint/Utilities/ProgressUtilities.cs`.

**WP-39: CommNet and KerbNet** (31 files, 4,534 lines; A 31 / B 0 / C 0). Verify: numerically.
- Depends on: WP-06, WP-09, WP-14.
- Acceptance: golden `commnet_graph`: link strengths, control paths, signal for a relay-network save; KerbNet FoV rules.
- Largest files: `ModuleDataTransmitter.cs`, `CommNet/CommNetVessel.cs`, `ModuleKerbNetAccess.cs`, `CommNet/CommNetwork.cs`, `CommNet/Network/Net.cs`, `CommNet/Network/Path.cs`.

**WP-40: Editor core: placement, symmetry, gizmos, pre-flight checks** (64 files, 10,865 lines; A 53 / B 11 / C 0). Verify: verify by scenario.
- Depends on: WP-00, WP-13, WP-18.
- Acceptance: scenario `editor_build`: scripted placement with radial/mirror symmetry reproduces golden .craft; pre-flight checks match.
- Largest files: `EditorLogic.cs`, `EditorLogicBase.cs`, `EditorGizmos/GizmoOffset.cs`, `EditorGizmos/GizmoRotate.cs`, `PreFlightTests/AntennaPresent.cs`, `PreFlightTests/DockingPortFacing.cs`.

**WP-41: Editor UI (part list, categories, craft browser, action-group UI)** (57 files, 17,096 lines; A 0 / B 0 / C 57). Verify: verify by inspection.
- Depends on: WP-40, WP-45.
- Acceptance: UI inspection against KSP screenshots.
- Largest files: `KSP/UI/Screens/PartCategorizer.cs`, `KSP/UI/Screens/EditorActionGroups.cs`, `KSP/UI/Screens/CraftBrowserDialog.cs`, `KSP/UI/Screens/EditorPartIcon.cs`, `KSP/UI/Screens/Editor/PartListTooltip.cs`, `KSP/UI/Screens/DirectoryController.cs`.

**WP-42: Scene flow, KSC, facilities, launch sites** (37 files, 5,522 lines; A 4 / B 10 / C 23). Verify: verify by scenario.
- Depends on: WP-04, WP-19, WP-36.
- Acceptance: scenario `scene_flow`: main menu -> KSC -> VAB -> pad -> flight -> recovery -> KSC, persistence intact.
- Largest files: `SpaceCenterBuilding.cs`, `DestructibleBuilding.cs`, `FlightDriver.cs`, `EditorDriver.cs`, `LaunchSiteFacility.cs`, `HighLogic.cs`.

**WP-43: Flight HUD (navball, staging UI, PAW, gauges, messages, crew portraits)** (177 files, 29,718 lines; A 0 / B 1 / C 176). Verify: verify by inspection.
- Depends on: WP-16, WP-17, WP-24, WP-45.
- Acceptance: UI inspection; displayed values come from core queries (spot-checked numerically).
- Largest files: `UIPartActionWindow.cs`, `KSP/UI/Screens/StageIcon.cs`, `TransferTypeSimple.cs`, `KSP/UI/Dialogs/KerbNetDialog.cs`, `KSP/UI/Screens/StageGroup.cs`, `KSP/UI/Screens/Flight/KerbalPortraitGallery.cs`.

**WP-44: Map view and tracking station** (53 files, 14,344 lines; A 0 / B 0 / C 53). Verify: verify by inspection.
- Depends on: WP-08, WP-45.
- Acceptance: inspection; drawn conics sampled from core `patched_conics` data.
- Largest files: `KSP/UI/Screens/SpaceTracking.cs`, `OrbitTargeter.cs`, `OrbitRendererBase.cs`, `KSP/UI/Screens/Mapview/MapNode.cs`, `ManeuverNodeEditorManager.cs`, `PatchRendering.cs`.

**WP-45: UI framework: DialogGUI, popups, tooltips, app launcher, widgets, rich text** (377 files, 59,604 lines; A 0 / B 0 / C 234 / third-party 143). Verify: verify by inspection.
- Depends on: WP-51.
- Acceptance: UI inspection; DialogGUI-built dialogs render equivalently.
- Largest files: `TMPro/TMP_Text.cs`, `iTween.cs`, `XKCDColors.cs`, `TMPro/TMP_InputField.cs`, `TMPro/TextMeshProUGUI.cs`, `TMPro/TextMeshPro.cs`.
- Note: TMPro and SoftMasking (third-party) are retired here; KSP rich-text tags need UMG decorators.

**WP-46: UI screens: main menu, settings, KSC building screens, load/save dialogs** (153 files, 26,726 lines; A 0 / B 0 / C 153). Verify: verify by inspection.
- Depends on: WP-45.
- Acceptance: UI inspection.
- Largest files: `KSP/UI/Screens/VesselSpawnDialog.cs`, `MainMenu.cs`, `LoadGameDialog.cs`, `KSP/UI/Screens/MessageSystem.cs`, `KSP/UI/Screens/RDArchivesController.cs`, `KSP/UI/Screens/RDNode.cs`.

**WP-47: Cameras** (50 files, 6,302 lines; A 0 / B 0 / C 50). Verify: verify by inspection.
- Depends on: WP-19.
- Acceptance: inspection of camera modes (auto/free/orbital/chase/locked, map, editor, IVA).
- Largest files: `FlightCamera.cs`, `PlanetariumCamera.cs`, `CameraManager.cs`, `SpaceCenterCamera2.cs`, `TrackIR.cs`, `SpaceCenterCamera.cs`.

**WP-48: Rendering, FX, scaled-space visuals, asset import** (224 files, 34,345 lines; A 0 / B 0 / C 211 / third-party 13). Verify: verify by inspection.
- Depends on: WP-05.
- Acceptance: visual inspection; optional .mu importer verified on stock parts.
- Largest files: `DatabaseLoaderModel_DAE.cs`, `Assembly-CSharp-firstpass/DAE.cs`, `ModuleLight.cs`, `PartReader.cs`, `Highlighting/Highlighter.cs`, `Assembly-CSharp-firstpass/OBJ.cs`.

**WP-49: Audio** (17 files, 2,496 lines; A 0 / B 0 / C 17). Verify: verify by inspection.
- Depends on: none.
- Acceptance: listening test.
- Largest files: `MusicLogic.cs`, `AudioMultiPooledFX.cs`, `AudioFX.cs`, `AlarmClockScenarioAudio.cs`, `AmbienceControl.cs`, `AudioSingleFX.cs`.

**WP-50: Input, bindings, input locks** (33 files, 7,026 lines; A 3 / B 2 / C 28). Verify: verify by scenario.
- Depends on: WP-24.
- Acceptance: scenario `controls`: default bindings drive the expected FlightCtrlState/editor actions; input locks block correctly.
- Largest files: `GameSettings.cs`, `InputSettings.cs`, `FlightInputHandler.cs`, `Mouse.cs`, `Assembly-CSharp-firstpass/SpaceNavigator.cs`, `AxisBinding_Single.cs`.

**WP-51: Localization** (18 files, 2,047 lines; A 0 / B 0 / C 18). Verify: verify by inspection.
- Depends on: WP-02.
- Acceptance: string-table import of KSP Localization cfgs; format shim unit tests (gender/plural).
- Largest files: `KSP/Localization/Localizer.cs`, `KSP/UI/Language/Language.cs`, `KSP/UI/Language/LanguageDefinition.cs`, `FontLoader.cs`, `KSPFontAsset.cs`, `Assembly-CSharp-firstpass/Lingoona/Grammar.cs`.
- Note: `Localizer` has the highest fan-in of any file (802). Its key lookup and format shim are needed early by every package that produces text, even though the tier is C.

**WP-52: Breaking Ground robotics** (29 files, 13,722 lines; A 7 / B 10 / C 12). Verify: verify by scenario.
- Depends on: WP-06, WP-13, WP-20.
- Acceptance: scenario `servo_motion`: hinge/piston/rotor response and KAL-1000 sequences vs KSP traces.
- Largest files: `Expansions/Serenity/BaseServo.cs`, `Expansions/Serenity/ModuleRoboticController.cs`, `Expansions/Serenity/ModuleRobotArmScanner.cs`, `Expansions/Serenity/RoboticControllerWindow.cs`, `Expansions/Serenity/CurvePanel.cs`, `Expansions/Serenity/ModuleRoboticRotationServo.cs`.

**WP-53: Breaking Ground deployed science and surface features** (27 files, 7,420 lines; A 14 / B 11 / C 2). Verify: numerically.
- Depends on: WP-12, WP-35.
- Acceptance: golden `deployed_science` (science accrual per tick) and `roc_placement` (seeded surface feature positions).
- Largest files: `Expansions/Serenity/DeployedScience/Runtime/DeployedScienceExperiment.cs`, `ModuleGroundSciencePart.cs`, `Expansions/Serenity/DeployedScience/Runtime/DeployedScience.cs`, `ROCManager.cs`, `Expansions/Serenity/DeployedScience/Runtime/DeployedScienceCluster.cs`, `ModuleGroundPart.cs`.

**WP-54: Making History mission runtime: Mission, MENode graph, flow, MissionSystem** (122 files, 19,899 lines; A 112 / B 10 / C 0). Verify: verify by scenario.
- Depends on: WP-04, WP-19, WP-37.
- Acceptance: scenario `mission_playback`: stock missions run headless; node activation trace matches golden.
- Largest files: `Expansions/Missions/Mission.cs`, `Expansions/Missions/Runtime/MissionSystem.cs`, `Expansions/Missions/MENode.cs`, `Expansions/Missions/MissionSituation.cs`, `Expansions/Missions/MissionsUtils.cs`, `Expansions/Missions/Flow/MEFlowParser.cs`.
- Note: Mission runtime is mostly A (graph, flow, scoring, persistence). `MissionSystem` (spawning, checkpoints, camera locks) is B.

**WP-55: Making History node library: tests, actions, adjusters** (139 files, 15,470 lines; A 132 / B 5 / C 2). Verify: verify by scenario.
- Depends on: WP-54, WP-23, WP-24, WP-28.
- Acceptance: per-node unit scenarios (each Test*/Action*/Adjuster* on a fixture save) + `mission_playback`.
- Largest files: `Expansions/Missions/Tests/TestDistance.cs`, `Expansions/Missions/Tests/TestOrbitParams.cs`, `Expansions/Missions/Actions/ActionCreateVessel.cs`, `Expansions/Missions/Actions/ActionPartRepair.cs`, `Expansions/Missions/Tests/TestGrapple.cs`, `Expansions/Missions/Actions/ActionPartResourceDrain.cs`.
- Note: About 130 small A files (tests, actions, adjusters, failures). This is the most parallelisable package. Adjusters need WP-06 hooks in the target modules.

**WP-56: Making History mission editor UI** (202 files, 33,746 lines; A 0 / B 0 / C 202). Verify: verify by inspection.
- Depends on: WP-45, WP-54.
- Acceptance: UI inspection; saved .mission files round-trip through `confignode_roundtrip`.
- Largest files: `Expansions/Missions/Editor/MissionEditorLogic.cs`, `Expansions/Missions/Runtime/MissionPlayDialog.cs`, `Expansions/Missions/Runtime/MissionsApp.cs`, `Expansions/Missions/Editor/MEGUINode.cs`, `Expansions/Missions/Editor/MissionBriefingDialog.cs`, `Expansions/Missions/Editor/MECrewAssignmentDialog.cs`.

**WP-57: Tutorials** (38 files, 14,611 lines; A 0 / B 0 / C 38). Verify: verify by inspection.
- Depends on: WP-42, WP-45.
- Acceptance: play-through inspection.
- Largest files: `TutorialEditorAdvanced.cs`, `AdvancedTutorial.cs`, `BasicTutorial.cs`, `METutorialScenario.cs`, `TutorialEditorMid.cs`, `TutorialFlightSuborbital.cs`.

**WP-58: Debug tools and cheats -> console commands** (121 files, 14,405 lines; A 0 / B 0 / C 121). Verify: verify by inspection.
- Depends on: WP-45.
- Acceptance: console commands exist for every CheatOptions flag gameplay reads.
- Largest files: `DebugToolbar.cs`, `AeroGUI.cs`, `RnDDebugUtil.cs`, `ConsoleCommands.cs`, `KSP/UI/Screens/DebugToolbar/Screens/Cheats/CheatsCareer.cs`, `KSP/UI/Screens/DebugToolbar/DebugScreen.cs`.

**WP-59: Drop list (platform, analytics, test scripts, obfuscator stubs, unused third-party)** (158 files, 22,811 lines; A 0 / B 0 / C 41 / third-party 117). Verify: none.
- Depends on: none.
- Acceptance: nothing to build; confirm no gameplay reads.
- Largest files: `UniLinq/Enumerable.cs`, `Smooth/Slinq/Slinq.cs`, `Smooth/Algebraics/Tuple.cs`, `AssemblyLoader.cs`, `AnalyticsUtil.cs`, `SteamManager.cs`.
- Note: Steam, analytics, obfuscator stubs, test-only modules, unused Smooth/UniLinq. Confirm nothing in A/B reads them.

## 8. Golden datasets for the planned harness

`C:\Projects\KSP-UE5\harness\` does not exist yet, so these dataset names are proposals. Each would be captured from the instrumented dev copy (`decompiled/KSP-dev/`) by a capture plugin or a patched build, stored as versioned files, and replayed by the C++ test runner.

| Dataset | Package(s) | Content |
|---|---|---|
| `math_shim` | WP-01 | Unity Vector3/Quaternion/Matrix4x4/Mathf results (incl. native Quaternion constructors) for randomised inputs, captured by a KSP-side capture plugin |
| `curves_eval` | WP-01 | Every FloatCurve/AnimationCurve key set found in GameData (engines, atmospheres, PQS, thermal) sampled at 64 points |
| `unity_random` | WP-01 | UnityEngine.Random sequences after InitState(seed) for 1k seeds: value, Range(int), Range(float), insideUnitSphere |
| `format_strings` | WP-03 | KSPUtil time/date/distance/unit formatting outputs for sample values |
| `confignode_roundtrip` | WP-02 | Parse+serialise of every .cfg in GameData plus sample .sfs/.craft/.mission: byte-identical output |
| `sfs_roundtrip / save_upgrade` | WP-04 | Load+save of stock and test saves; upgrade of legacy saves |
| `part_database` | WP-05 | Full AvailablePart dump for every stock part incl. modules, resources, drag cubes, costs, tech |
| `module_persistence` | WP-06 | OnSave output of every stock PartModule after OnLoad of its config node |
| `orbit_propagation / closest_approach / transfer_planner` | WP-07 | State vectors, anomalies, apsis/node times for 10k random orbits; closest approaches; transfer solutions |
| `patched_conics / alarm_times` | WP-08 | Patch chains and SOI transition times for scripted trajectories and maneuver previews; alarm UTs |
| `body_catalog / atmosphere_curves / biome_lookup` | WP-09 | Body constants; p/T/rho/speed of sound on altitude grids; biome at lat/lon grid |
| `terrain_heights` | WP-11 | PQS surface height and vertex colour at a lat/lon grid for every body |
| `craft_tree / vessel_state` | WP-13/14 | Part tree, attach nodes, symmetry; CoM/MOI/situation for sample vessels |
| `resource_flow / staging_sequence / deltav_readout` | WP-15/16/17 | Per-tick resource draws; inverse stages and separation indices; per-stage dV/TWR/ISP |
| `aero_forces / thermal_step` | WP-21/22 | Per-part drag/lift for given flow conditions; temperatures after N steps |
| `engine_thrust / shielding / converter_rates` | WP-23/26/28 | Engine thrust/ISP/flow curves; shielded part sets; converter outputs |
| `roster_xp / science_values / techtree / career_currency` | WP-31/35/36 | XP and levels, science subject values, tech costs, recovery value and strategy conversions |
| `contract_generation / commnet_graph` | WP-37/38/39 | Seeded contract offers from fixed saves; CommNet link strengths and paths |
| `deployed_science / roc_placement` | WP-53 | Deployed-science accrual per tick; seeded surface feature positions |

Scenario traces (for 'verify by scenario'): warp_rules, ksc_ground, launch_craft, save_load_cycle, free_fall, physics_orbit_hold, crash_tolerance, joint_stiffness, ascent_profile, sas_hold, docking, parachute_descent, deploy_states, rover_drive, gear_settle, eva_basics, eva_construction, progress_events, editor_build, scene_flow, controls, servo_motion, mission_playback.

## 9. Where to start (first 10 packages)

| Order | Package | Why first |
|---:|---|---|
| 1 | WP-01 Core maths, curves, Unity value-type/Random shims | Everything else compares against goldens that pass through these; capture `math_shim`, `curves_eval`, `unity_random` first. |
| 2 | WP-02 ConfigNode + GameData tree | Highest-fan-in A file; unlocks all data loading. Byte-exact round-trip is a crisp acceptance test. |
| 3 | WP-03 Event bus, FSM, utilities | GameEvents (548 dependants) and KSPUtil formatting are prerequisites for nearly every A file. |
| 4 | WP-07 Orbit maths | Pure double maths with an analytic oracle; the best agent target per FEASIBILITY.md. |
| 5 | WP-09 Celestial bodies, atmospheres, biomes | Needed by orbits, aero, science and contracts; mostly A after extracting the maths from CelestialBody. |
| 6 | WP-08 Patched conics, maneuver nodes, alarms | Builds directly on WP-07/09; golden-testable. |
| 7 | WP-11 LibNoise + PQSMod height maths | Terrain heights must match KSP bit-exactly; independent of the physics decision. |
| 8 | WP-04 Save model + upgrade pipeline | Lets the core load real .sfs saves as fixtures for every later test. |
| 9 | WP-05 GameDatabase/PartLoader data side | Produces the part catalog every craft-level package needs; `part_database` golden. |
| 10 | WP-06 Module host + KSPField registry | Defines how ~150 PartModules (and mission adjusters) plug into the core; must be designed before any part-module package. |

In parallel, run the Phase-0 physics spike (FEASIBILITY.md section 6), which gates WP-00/WP-20 and all of L4-L6, and build the capture plugin for the golden datasets above.
