"""Compose analysis/tiering/TIERING.md from narrative text + work/tables.md + work/wp_stats.json.

Run: python -I tiering_md.py      (after report.py)
"""
import json, os
from collections import Counter

BASE = r"C:/Projects/KSP-UE5/analysis/tiering"
WORK = os.path.join(BASE, "work")

DEPS = {
    "WP-00": ["WP-01"], "WP-01": [], "WP-02": ["WP-01"], "WP-03": ["WP-01"], "WP-04": ["WP-02", "WP-03"],
    "WP-05": ["WP-02", "WP-04"], "WP-06": ["WP-02", "WP-03", "WP-05"], "WP-07": ["WP-01"],
    "WP-08": ["WP-07", "WP-09"], "WP-09": ["WP-02", "WP-07"], "WP-10": ["WP-00", "WP-07", "WP-09"],
    "WP-11": ["WP-01", "WP-02", "WP-09"], "WP-12": ["WP-00", "WP-11", "WP-20"],
    "WP-13": ["WP-00", "WP-05", "WP-06"], "WP-14": ["WP-13", "WP-09"], "WP-15": ["WP-13"],
    "WP-16": ["WP-13", "WP-15"], "WP-17": ["WP-15", "WP-16", "WP-23"], "WP-18": ["WP-05", "WP-13"],
    "WP-19": ["WP-04", "WP-10", "WP-14"], "WP-20": ["WP-00", "WP-14", "Phase-0 physics spike"],
    "WP-21": ["WP-05", "WP-09", "WP-14", "WP-20"], "WP-22": ["WP-09", "WP-21"],
    "WP-23": ["WP-06", "WP-09", "WP-15", "WP-20"], "WP-24": ["WP-06", "WP-14", "WP-20"],
    "WP-25": ["WP-13", "WP-16", "WP-20"], "WP-26": ["WP-13", "WP-21"], "WP-27": ["WP-00", "WP-06", "WP-21"],
    "WP-28": ["WP-06", "WP-09", "WP-15"], "WP-29": ["WP-06", "WP-12", "WP-20"], "WP-30": ["WP-06"],
    "WP-31": ["WP-03", "WP-04"], "WP-32": ["WP-12", "WP-20", "WP-31"], "WP-33": ["WP-13", "WP-32", "WP-40"],
    "WP-34": ["WP-31", "WP-47"], "WP-35": ["WP-04", "WP-06", "WP-09"], "WP-36": ["WP-03", "WP-04"],
    "WP-37": ["WP-08", "WP-14", "WP-35", "WP-36"], "WP-38": ["WP-11", "WP-28", "WP-37"],
    "WP-39": ["WP-06", "WP-09", "WP-14"], "WP-40": ["WP-00", "WP-13", "WP-18"], "WP-41": ["WP-40", "WP-45"],
    "WP-42": ["WP-04", "WP-19", "WP-36"], "WP-43": ["WP-16", "WP-17", "WP-24", "WP-45"],
    "WP-44": ["WP-08", "WP-45"], "WP-45": ["WP-51"], "WP-46": ["WP-45"], "WP-47": ["WP-19"], "WP-48": ["WP-05"],
    "WP-49": [], "WP-50": ["WP-24"], "WP-51": ["WP-02"], "WP-52": ["WP-06", "WP-13", "WP-20"],
    "WP-53": ["WP-12", "WP-35"], "WP-54": ["WP-04", "WP-19", "WP-37"], "WP-55": ["WP-54", "WP-23", "WP-24", "WP-28"],
    "WP-56": ["WP-45", "WP-54"], "WP-57": ["WP-42", "WP-45"], "WP-58": ["WP-45"], "WP-59": [],
}
NOTES = {
    "WP-00": "New code (no direct KSP counterpart beyond Timing*/CoroutineHost). It sets the host contract that every B package builds on. Needs the Phase-0 architecture decisions.",
    "WP-01": "Includes the shims for Unity value types, `Mathf`, `AnimationCurve` and `Random` specified in unity-api-surface.md. Bit-exactness matters because everything downstream is compared to goldens.",
    "WP-02": "Highest fan-in file in the game (ConfigNode, 667 dependants). Includes the reflection-based object<->node mapping, which becomes generated C++ serialisers.",
    "WP-05": "Config side only. Model/texture import is WP-48. `DragCubeSystem` is listed here: import KSP's `PartDatabase.cfg` cube cache first.",
    "WP-06": "The KSPField/KSPEvent/KSPAction reflection layer becomes a C++ field registry (codegen or macros). Must support part-module adjusters (Making History hooks call into base modules).",
    "WP-09": "CelestialBody maths (atmosphere, gravity, rotation, lat/lon) is extracted as A. The MonoBehaviour/scaled-space shell side is B.",
    "WP-13": "Part.cs (9,023 lines) is the single largest re-architecture item. Split internally: tree/attach, mass/CoM, resources, crew, thermal state, joints, packing.",
    "WP-16": "Includes extracting the staging rules from `KSP/UI/Screens/StageManager.cs`.",
    "WP-20": "Blocked on the Phase-0 physics spike (FEASIBILITY.md section 6): reduced rigid-body model vs Chaos/Jolt jointed tree.",
    "WP-21": "FlightIntegrator's aero maths (drag cubes, lift curves, Mach/pressure terms) is A and can be transpiled ahead of WP-20 against `aero_forces`.",
    "WP-22": "FlightIntegrator's thermal solver (conduction graph, convection, radiation, shock heating) is A. Its integration hook into the physics tick is B.",
    "WP-29": "Ports the reachable VehiclePhysics/EdyCommonTools subset (third-party.md). PhysX WheelCollider suspension is native, so behaviour is tuned against scenario traces.",
    "WP-35": "Includes extracting tech-tree load/query rules from `KSP/UI/Screens/RDTechTree.cs` and science-value rules from `ExperimentResultDialogPage.cs`.",
    "WP-37": "Seeded generation uses `UnityEngine.Random.InitState`, so it needs WP-01's bit-exact Unity Random for `contract_generation` goldens.",
    "WP-45": "TMPro and SoftMasking (third-party) are retired here; KSP rich-text tags need UMG decorators.",
    "WP-51": "`Localizer` has the highest fan-in of any file (802). Its key lookup and format shim are needed early by every package that produces text, even though the tier is C.",
    "WP-54": "Mission runtime is mostly A (graph, flow, scoring, persistence). `MissionSystem` (spawning, checkpoints, camera locks) is B.",
    "WP-55": "About 130 small A files (tests, actions, adjusters, failures). This is the most parallelisable package. Adjusters need WP-06 hooks in the target modules.",
    "WP-59": "Steam, analytics, obfuscator stubs, test-only modules, unused Smooth/UniLinq. Confirm nothing in A/B reads them.",
}
GOLDENS = [
    ("math_shim", "WP-01", "Unity Vector3/Quaternion/Matrix4x4/Mathf results (incl. native Quaternion constructors) for randomised inputs, captured by a KSP-side capture plugin"),
    ("curves_eval", "WP-01", "Every FloatCurve/AnimationCurve key set found in GameData (engines, atmospheres, PQS, thermal) sampled at 64 points"),
    ("unity_random", "WP-01", "UnityEngine.Random sequences after InitState(seed) for 1k seeds: value, Range(int), Range(float), insideUnitSphere"),
    ("format_strings", "WP-03", "KSPUtil time/date/distance/unit formatting outputs for sample values"),
    ("confignode_roundtrip", "WP-02", "Parse+serialise of every .cfg in GameData plus sample .sfs/.craft/.mission: byte-identical output"),
    ("sfs_roundtrip / save_upgrade", "WP-04", "Load+save of stock and test saves; upgrade of legacy saves"),
    ("part_database", "WP-05", "Full AvailablePart dump for every stock part incl. modules, resources, drag cubes, costs, tech"),
    ("module_persistence", "WP-06", "OnSave output of every stock PartModule after OnLoad of its config node"),
    ("orbit_propagation / closest_approach / transfer_planner", "WP-07", "State vectors, anomalies, apsis/node times for 10k random orbits; closest approaches; transfer solutions"),
    ("patched_conics / alarm_times", "WP-08", "Patch chains and SOI transition times for scripted trajectories and maneuver previews; alarm UTs"),
    ("body_catalog / atmosphere_curves / biome_lookup", "WP-09", "Body constants; p/T/rho/speed of sound on altitude grids; biome at lat/lon grid"),
    ("terrain_heights", "WP-11", "PQS surface height and vertex colour at a lat/lon grid for every body"),
    ("craft_tree / vessel_state", "WP-13/14", "Part tree, attach nodes, symmetry; CoM/MOI/situation for sample vessels"),
    ("resource_flow / staging_sequence / deltav_readout", "WP-15/16/17", "Per-tick resource draws; inverse stages and separation indices; per-stage dV/TWR/ISP"),
    ("aero_forces / thermal_step", "WP-21/22", "Per-part drag/lift for given flow conditions; temperatures after N steps"),
    ("engine_thrust / shielding / converter_rates", "WP-23/26/28", "Engine thrust/ISP/flow curves; shielded part sets; converter outputs"),
    ("roster_xp / science_values / techtree / career_currency", "WP-31/35/36", "XP and levels, science subject values, tech costs, recovery value and strategy conversions"),
    ("contract_generation / commnet_graph", "WP-37/38/39", "Seeded contract offers from fixed saves; CommNet link strengths and paths"),
    ("deployed_science / roc_placement", "WP-53", "Deployed-science accrual per tick; seeded surface feature positions"),
]
SCENARIOS = "warp_rules, ksc_ground, launch_craft, save_load_cycle, free_fall, physics_orbit_hold, crash_tolerance, joint_stiffness, ascent_profile, sas_hold, docking, parachute_descent, deploy_states, rover_drive, gear_settle, eva_basics, eva_construction, progress_events, editor_build, scene_flow, controls, servo_motion, mission_playback"


def main():
    tables = open(os.path.join(WORK, "tables.md"), encoding="utf-8").read()
    stats = json.load(open(os.path.join(WORK, "wp_stats.json"), encoding="utf-8"))
    summary = json.load(open(os.path.join(WORK, "summary.json"), encoding="utf-8"))
    tf, tl = summary["tier_files"], summary["tier_lines"]
    ksp_f = tf["A"] + tf["B"] + tf["C"]
    ksp_l = tl["A"] + tl["B"] + tl["C"]
    # split generated tables into sections
    parts = tables.split("### ")
    sec = {p.split("\n", 1)[0].strip(): "### " + p for p in parts if p.strip()}
    L = []
    a = L.append
    a("# KSP 1.12.5 -> UE5: porting work breakdown (tiering)")
    a("")
    a("Date: 2026-10-08. Scope: every KSP-authored source file of `decompiled/KSP-Decompiled/Assembly-CSharp/` "
      "(3,299 files, 627,747 lines) plus the reviewed part of `Assembly-CSharp-firstpass/` (29 files; Steamworks and "
      "Vectrosity are excluded as third-party). Embedded third-party namespaces are listed separately (third-party.md).")
    a("")
    a("Deliverables in `analysis/tiering/`:")
    a("")
    a("| File | Contents |")
    a("|---|---|")
    a("| `tiering.csv` | One row per file (3,328 rows): path, lines, kind, tier, subsystem, unity_dependency_score, key_dependencies, ue5_counterpart, rationale, review_source, fan_in, namespace |")
    a("| `work_packages.csv` | One row per file: the work package it belongs to |")
    a("| `TIERING.md` | This document |")
    a("| `third-party.md` | Embedded third-party libraries: use and port/replace/drop decision |")
    a("| `unity-api-surface.md` | Ranked Unity API usage by tier and the core shim spec |")
    a("| `scripts/` | Reproducible pipeline (`python -I`): extract_features -> depgraph -> classify -> make_lists -> merge -> report -> unity_surface -> surface_report -> tiering_md |")
    a("| `work/` | Intermediate data: features, dependency graph, digests, review TSVs, rubric |")
    a("")
    a("## 1. Tier definitions (as applied)")
    a("")
    a("- **A, Transpile**: pure logic, maths or data with little Unity dependency. It goes into the C++ core almost "
      "mechanically and is checked against golden outputs. MonoBehaviour inheritance alone does not make a file B: "
      "ScenarioModules and PartModules whose logic is rules/state/resources and that touch Unity only through lifecycle "
      "hooks, value types, logging and timers are A ('needs host').")
    a("- **B, Re-architect**: gameplay entangled with Unity physics, transforms, animation-driven state, scene "
      "orchestration or gameplay mesh/collider generation. Behaviour is preserved but the structure is redesigned for "
      "the core/shell split. Base classes that bridge the Unity lifecycle (Part, Vessel, PartModule, VesselModule, "
      "ScenarioModule, PQSMod) are B. UI classes that own a gameplay rule are B with the rule marked for extraction.")
    a("- **C, Rewrite in UE5 / discard**: presentation and platform code (UI, rendering, cameras, audio, input devices, "
      "FX, asset loading, Steam, analytics, localization plumbing, debug tools, tutorials). Each row names the UE5 "
      "counterpart or `drop`.")
    a("- **unity_dependency_score** (0-100): saturating weighted density of Unity API references per 100 lines (heavy "
      "engine APIs x3, object model x2, utilities x1, value types x0.25; score = 100*d/(d+25)), +15 for Unity "
      "components. It is a sorting aid; the tier is a reviewed judgement.")
    a("")
    a("## 2. Method and provenance (what was reviewed by hand vs by script)")
    a("")
    a("1. `extract_features.py` parses every file: line count, namespace, usings, type declarations and base classes "
      "(with an inheritance closure, so `kind` resolves PartModule/VesselModule/ScenarioModule/Contract/PQSMod/"
      "MonoBehaviour/UIBehaviour/class/struct/enum/interface/delegate), counts of 57 Unity API families, references to "
      "56 KSP core types, and method names.")
    a("2. `depgraph.py` builds a file-to-file type-reference graph (C# namespace visibility applied). `fan_in` is the "
      "number of distinct KSP files that reference a type declared in the file.")
    a("3. `classify.py` produces an automatic tier/subsystem suggestion (path and name rules plus API counts). This was "
      "only an input to review. The final tiers differ from it on many rows.")
    a("4. Review:")
    a("   - **Lead (Opus) hand review**: all 127 KSP-authored files over 800 lines were opened and classified individually "
      "(`work/review/lead_big_*.tsv`). The lead also checked the automatic classification of the 282 files in the small "
      "directories outside the mandatory scope as a list (with features) and wrote or corrected 114 of them individually "
      "(`script_overrides.tsv`). The other 168 are homogeneous rows (SaveUpgradePipeline, PreFlightTests, KSPAchievements, "
      "Experience, Strategies, CameraFXModules) that use directory templates. The lead rewrote 381 tier-A reviewer rationales that were group templates into file-specific lines, "
      "correcting 69 tiers in the process (`KSP/IO` to C/drop, mission-editor metadata to C), and made 2 explicit tier "
      "overrides (`ModuleEnginesFX`, `ModuleRCSFX` to B).")
    a("   - **Sonnet reviewers (4, model `sonnet`)**: classified the remaining files of 800 lines or fewer in the root "
      "directory and in `KSP/`, `Expansions/`, `CommNet/`, `Contracts/`, `FinePrint/`, `SentinelMission/` and `KerbNet/`, "
      "from compact digests plus selective source reads, against a shared rubric with fixed anchor decisions "
      "(`work/RUBRIC.md`). Reviewers 1 and 2 (root files) wrote per-file rationales. Reviewers 3 (`KSP/` UI, contracts, "
      "CommNet) and 4 (`Expansions/`) assigned many rows by path pattern with group-level rationales. Every row whose "
      "rationale is a group template now carries an appended `[evidence: type : base; Unity deps; methods]` clause and "
      "is labelled `(group rationale + script evidence)` in `review_source`. That covers 598 rows, 589 of them tier C UI "
      "widgets.")
    a("   - An earlier round of 11 reviewers was stopped by the orchestrator (concurrency/model cap) before writing "
      "anything; none of its output is used.")
    a("5. Consistency pass (`consistency.py`): all 77 fixed anchor decisions hold; A rows with physics coupling, C rows "
      "that are gameplay hosts, and B rows without engine coupling were listed and checked by the lead.")
    a("")
    a(sec.get("Review provenance", ""))
    a("")
    a("Caveat: the per-file tier is reliable for the lead-reviewed rows and for spot-checked reviewer rows (a random "
      "sample of each reviewer's rows and a curated list of about 120 gameplay-critical files of 800 lines or fewer were "
      "audited by the lead). Group-rationale C rows "
      "were not individually re-read. Their tier is low-risk (UI widgets), but a few may hide small rules.")
    a("")
    a("## 3. Summary")
    a("")
    a(sec.get("Totals per tier", ""))
    a("")
    a(sec.get("Totals per subsystem (KSP-authored files only)", ""))
    a("")
    a("## 4. What stands out")
    a("")
    findings = [
        f"**B is small in files but large in lines.** {tf['B']} files ({100*tf['B']/ksp_f:.0f}% of KSP files) hold "
        f"{tl['B']:,} lines ({100*tl['B']/ksp_l:.0f}%). Re-architecture is concentrated in a few huge classes: Part (9,023), "
        "KerbalEVA (7,368), EditorLogic (5,490), Vessel (4,664), PQS (3,168), FlightIntegrator (2,776), ModuleProceduralFairing "
        "(2,220), ModuleEngines (2,217), PartLoader (2,205), ModuleDockingNode (2,059), FlightGlobals (2,003).",
        f"**A is a third of the code and most of the career game.** {tl['A']:,} lines: contracts and FinePrint (34k), "
        "Making History mission runtime (28k), save/persistence, orbit maths, science/R&D, CommNet, resource flow, delta-v. "
        "This is the cheap, numerically verifiable part.",
        "**Gameplay rules live in UI classes**: `KSP/UI/Screens/StageManager` owns the staging sequence; `RDTechTree` "
        "loads the tech tree and answers unlock queries; `ExperimentResultDialogPage` computes lab/transmit/recovery science "
        "values; `NavBallBurnVector` computes burn time; `UIWarpToNextMorning` computes time-to-dawn; `LaunchSiteFacility`, "
        "`CrewTransfer`, `CrewHatchController`, `ExperimentTransfer`, `QuickSaveLoad`, `EditorMarker_CoM/CoT` hold rules. "
        "These are B (extract) or noted in their package.",
        "**`RUI/Algorithms` is not UI.** Its SCC flow graph is used by `PartSet` and `DeltaVStageInfo` for resource "
        "crossfeed ordering, so it is resource-flow core (tier A).",
        "**Native Unity behaviour sits under pure-looking code.** `FloatCurve` wraps `AnimationCurve.Evaluate` (native), "
        "and seeded contract, asteroid, comet and surface-feature generation calls `UnityEngine.Random.InitState` (native "
        "xorshift128). Quaternion constructors and all Transform maths are native too. These need golden capture "
        "(`curves_eval`, `unity_random`, `math_shim`) before any downstream golden can match.",
        "**Wheels run on a third-party vehicle physics library plus native PhysX.** VehiclePhysics zeroes PhysX tyre "
        "friction and applies its own tyre-friction curves (Flat/Lineal/Smooth/Parametric/Pacejka; Smooth by default); suspension still comes from the native WheelCollider.",
        "**Making History code is compiled into the base assembly** (463 files, 69k lines): 244 A files (runtime graph, tests, "
        "actions, awards, scoring), 204 C (editor UI). Its adjusters hook into 29 base-game PartModule types "
        "(`IsBlockingDecouple`, `ApplyThrottleAdjustment`, ...), so the module host (WP-06) must support adjusters from day one.",
        "**Breaking Ground code leaks into the base game**: `ROC*` surface features (including `PQSMod_ROCScatterQuad` "
        "in the terrain pipeline and `ROCScenario`, which destroys itself without the DLC), `ModuleGround*`, `ActiveJoint`, "
        "`ModuleJointMotor`, `LandClassROC` are all root-level files.",
        "**No C# mod compatibility survives**: `AssemblyLoader` and `KSP/IO` (the sandboxed plugin file API) are dropped. "
        "Config-level modding (ConfigNode, GameDatabase, part upgrades) is core.",
        "**Dead and test code ships in the assembly**: test PartModules (`ModuleJointMotorTest`, `ModuleEffectTest`, "
        "`AxisTestModule`), `KSP/Testing` (an in-game unit-test harness), `TestScripts/`, debug/cheat tooling (121 files in the Debug subsystem), "
        "procedural asteroid/comet test classes, and a Crypto Obfuscator stub (`A/-.cs`).",
        "**Third-party bulk is mostly unused**: Smooth has 104 files but only 7 are reachable; VehiclePhysics has 99, about "
        "26 reachable; LibNoise has 45, 11 reachable. TMPro (121 files) disappears with the UI rewrite.",
        "**`Localizer` has the highest fan-in (802 files)** and `ConfigNode` the next (667). Both are needed before anything "
        "else can be ported, even though Localizer is tier C (a key lookup + format shim suffices for the core).",
        "**Tick order is explicit**: `Timing0..5`, `TimingPre` and `TimingFI` are script-execution-order proxies. They define the "
        "per-frame phase order the core scheduler must reproduce (WP-00).",
        "**Facility-level rules live in `GameVariables`** (a MonoBehaviour singleton): contract/strategy limits, crew and "
        "EVA limits, craft size/mass limits, DSN power, reward factors. Core career data, tier A.",
    ]
    for f in findings:
        a(f"- {f}")
    a("")
    a("## 5. Dependency order")
    a("")
    a("What must exist before what. Each layer only needs the layers above it. Work packages are listed in brackets.")
    a("")
    a("```")
    a("L0  Foundations      Core maths + Unity value-type/curve/Random shims [WP-01] -> ConfigNode/UrlDir [WP-02]")
    a("                     -> event bus, FSM, utils [WP-03] -> localization key/format shim [WP-51]")
    a("L1  Runtime & data   Core runtime skeleton: tick phases, scheduler, transform hierarchy [WP-00]")
    a("                     Save model + upgrade pipeline [WP-04] -> GameDatabase/PartLoader data side [WP-05]")
    a("                     -> module host + KSPField registry [WP-06]")
    a("L2  Space            Orbit maths [WP-07] -> celestial bodies/atmospheres/biomes [WP-09]")
    a("                     -> patched conics/maneuvers/alarms [WP-08] -> time warp + rails [WP-10]")
    a("                     LibNoise + PQSMod heights [WP-11]")
    a("L3  Craft model      Part tree [WP-13] -> vessel model [WP-14] -> resource flow [WP-15]")
    a("                     -> staging/decouple [WP-16] -> delta-v [WP-17]; craft files [WP-18]")
    a("L4  Physics          (Phase-0 spike decision) physics core shim [WP-20] -> load/unload, packing [WP-19]")
    a("                     -> aero [WP-21] -> thermal [WP-22]; PQS mesh/colliders [WP-12]")
    a("L5  Part modules     engines [WP-23], control/SAS [WP-24], docking [WP-25], fairings [WP-26],")
    a("                     deployables/chutes [WP-27], resource modules [WP-28], wheels [WP-29], misc [WP-30]")
    a("L6  Kerbals          roster/XP [WP-31] -> EVA [WP-32] -> inventory/EVA construction [WP-33]; IVA [WP-34]")
    a("L7  Career           science/R&D [WP-35], funds/rep/strategies/facilities/progress [WP-36]")
    a("                     -> contracts [WP-37] -> FinePrint/Sentinel [WP-38]; CommNet/KerbNet [WP-39]")
    a("L8  Editor & scenes  editor core [WP-40], scene flow/KSC [WP-42]")
    a("L9  Shell            UI framework [WP-45] -> HUD [WP-43], map [WP-44], screens [WP-46], editor UI [WP-41];")
    a("                     cameras [WP-47], rendering/FX [WP-48], audio [WP-49], input [WP-50]")
    a("L10 DLC              Breaking Ground robotics [WP-52], deployed science/ROC [WP-53];")
    a("                     Making History runtime [WP-54] -> node library [WP-55] -> mission editor UI [WP-56]")
    a("L11 Last             tutorials [WP-57], debug console [WP-58]; drop list [WP-59] needs no work")
    a("```")
    a("")
    a("The packages of L0-L2 (WP-01..09, 11) do not depend on the physics model, so they can start before the Phase-0 "
      "physics spike concludes. The tier-A career packages (WP-31, 35-39) can also start early if they are developed "
      "against vessel/part query interfaces with fixture data from saves (WP-04). L4-L6 wait for the physics decision.")
    a("")
    a("## 6. Top 40 highest-leverage files")
    a("")
    a(sec.get("Top 40 highest-leverage files", "").split("\n", 1)[1] if "Top 40 highest-leverage files" in sec else "")
    a("")
    a("## 7. Work packages")
    a("")
    a("60 packages (WP-00..WP-59), each sized for one Opus agent; WP-59 is the drop list. Every row of `tiering.csv` "
      "is assigned to exactly one package in `work_packages.csv` (rules in `scripts/wp_rules.py`). Third-party files "
      "are assigned to the package that ports or retires them.")
    a("")
    vc = Counter(s["verify"] for s in stats.values())
    a("Verification modes: " + ", ".join(f"**{k}**: {v}" for k, v in sorted(vc.items())) + ". "
      "'numerically' = golden dataset comparison; 'verify by scenario' = scripted headless or in-shell scenario "
      "compared with KSP traces or expected outcomes; 'verify by inspection' = human or agent visual/behavioural check. "
      "The user does not want to test every feature, so the numerical goldens are concentrated where the risk is "
      "(maths, data, rules), and most UI packages are inspection-only.")
    a("")
    a(sec.get("Work-package sizes (generated from work_packages.csv)", "").split("\n", 1)[1]
      if "Work-package sizes (generated from work_packages.csv)" in sec else "")
    a("")
    a("### Package details")
    a("")
    for wp, s in stats.items():
        deps = ", ".join(DEPS.get(wp, [])) or "none"
        files = ", ".join("`" + p.replace("Assembly-CSharp/", "") + "`" for p in s["largest"][:6])
        a(f"**{wp}: {s['name']}** ({s['files']} files, {s['lines']:,} lines; A {s['A']} / B {s['B']} / C {s['C']}"
          + (f" / third-party {s['3p']}" if s['3p'] else "") + f"). Verify: {s['verify']}.")
        a(f"- Depends on: {deps}.")
        a(f"- Acceptance: {s['acceptance']}.")
        a(f"- Largest files: {files}.")
        if wp in NOTES:
            a(f"- Note: {NOTES[wp]}")
        a("")
    a("## 8. Golden datasets for the planned harness")
    a("")
    a("`C:\\Projects\\KSP-UE5\\harness\\` does not exist yet, so these dataset names are proposals. Each would be captured "
      "from the instrumented dev copy (`decompiled/KSP-dev/`) by a capture plugin or a patched build, stored as "
      "versioned files, and replayed by the C++ test runner.")
    a("")
    a("| Dataset | Package(s) | Content |")
    a("|---|---|---|")
    for n, w, c in GOLDENS:
        a(f"| `{n}` | {w} | {c} |")
    a("")
    a(f"Scenario traces (for 'verify by scenario'): {SCENARIOS}.")
    a("")
    a("## 9. Where to start (first 10 packages)")
    a("")
    start = [
        ("WP-01", "Core maths, curves, Unity value-type/Random shims", "Everything else compares against goldens that pass through these; capture `math_shim`, `curves_eval`, `unity_random` first."),
        ("WP-02", "ConfigNode + GameData tree", "Highest-fan-in A file; unlocks all data loading. Byte-exact round-trip is a crisp acceptance test."),
        ("WP-03", "Event bus, FSM, utilities", "GameEvents (548 dependants) and KSPUtil formatting are prerequisites for nearly every A file."),
        ("WP-07", "Orbit maths", "Pure double maths with an analytic oracle; the best agent target per FEASIBILITY.md."),
        ("WP-09", "Celestial bodies, atmospheres, biomes", "Needed by orbits, aero, science and contracts; mostly A after extracting the maths from CelestialBody."),
        ("WP-08", "Patched conics, maneuver nodes, alarms", "Builds directly on WP-07/09; golden-testable."),
        ("WP-11", "LibNoise + PQSMod height maths", "Terrain heights must match KSP bit-exactly; independent of the physics decision."),
        ("WP-04", "Save model + upgrade pipeline", "Lets the core load real .sfs saves as fixtures for every later test."),
        ("WP-05", "GameDatabase/PartLoader data side", "Produces the part catalog every craft-level package needs; `part_database` golden."),
        ("WP-06", "Module host + KSPField registry", "Defines how ~150 PartModules (and mission adjusters) plug into the core; must be designed before any part-module package."),
    ]
    a("| Order | Package | Why first |")
    a("|---:|---|---|")
    for i, (w, n, y) in enumerate(start, 1):
        a(f"| {i} | {w} {n} | {y} |")
    a("")
    a("In parallel, run the Phase-0 physics spike (FEASIBILITY.md section 6), which gates WP-00/WP-20 and all of L4-L6, and "
      "build the capture plugin for the golden datasets above.")
    a("")
    open(os.path.join(BASE, "TIERING.md"), "w", encoding="utf-8").write("\n".join(L))
    print("TIERING.md lines", len(L))


if __name__ == "__main__":
    main()
