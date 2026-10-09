"""Work-package definitions and file->package assignment rules (imported by report.py).

First matching rule wins. A rule = (wp, path regex or None, subsystems or None, tiers or None).
Fallbacks: SUB_WP[(subsystem, tier)] for A/B rows, C_WP[subsystem] for C rows.
"""
import re
from collections import OrderedDict

WPS = OrderedDict([
    # id: (name, verification, acceptance)
    ("WP-00", ("Core runtime skeleton: tick phases, timer/coroutine scheduler, double-precision transform hierarchy, component registry", "verify by inspection", "unit tests: tick order matches KSP Timing0..5/TimingPre/TimingFI phases; coroutine/timer semantics (WaitForSeconds/WaitForFixedUpdate/Invoke); transform maths vs `math_shim`")),
    ("WP-01", ("Core maths, curves and Unity value-type shim", "numerically", "golden `math_shim` (Vector3/Quaternion/Mathf incl. Round-to-even, Sign(0)=1, AngleAxis/LookRotation/FromToRotation), `curves_eval` (every FloatCurve/AnimationCurve key set in GameData sampled at 64 points, to 1 ulp) and `unity_random` (Unity Random sequences per seed, bit-exact)")),
    ("WP-02", ("ConfigNode, GameData tree, value parsers", "numerically", "golden `confignode_roundtrip`: parse+serialise every .cfg in GameData and sample .sfs/.craft, byte-identical; typed parse of all KSPField values")),
    ("WP-03", ("Event bus, FSM, misc core utilities", "verify by inspection", "unit tests: GameEvents subscribe/fire order, KFSM transitions; KSPUtil time/date/unit formatting vs golden `format_strings`")),
    ("WP-04", ("Save model, persistence, save upgrade pipeline", "numerically", "golden `sfs_roundtrip` (load+save stock and test saves byte-identical) and `save_upgrade` (legacy saves upgraded identically)")),
    ("WP-05", ("Data loading: GameDatabase config side, PartLoader data side, resource library", "numerically", "golden `part_database`: AvailablePart dump (fields, modules, resources, cost, mass, tech, drag cubes) for every stock part")),
    ("WP-06", ("Module host: PartModule/VesselModule/ScenarioModule, KSPField/Event/Action registry", "numerically", "golden `module_persistence`: every stock module's OnSave output after OnLoad of its config; event/action lists per part")),
    ("WP-07", ("Orbit maths, targeting, transfer maths", "numerically", "golden `orbit_propagation` (state vectors at UT, anomaly conversions, Ap/Pe/AN/DN times for 10k random orbits), `closest_approach`, `transfer_planner`")),
    ("WP-08", ("Patched conics, maneuver nodes, alarm clock and transfer-planner logic", "numerically", "golden `patched_conics` (patch chains, SOI transition UTs, encounters for scripted trajectories and maneuver previews) and `alarm_times`")),
    ("WP-09", ("Celestial bodies, planetarium, biomes, space objects", "numerically", "golden `body_catalog` + `atmosphere_curves` (p/T/rho/speed of sound on an altitude grid incl. latitude/sun offsets) + `biome_lookup` (biome at lat/lon grid)")),
    ("WP-10", ("Time warp, rails propagation, orbit driver", "verify by scenario", "scenario `warp_rules`: allowed warp rates vs altitude/situation, auto-warp to node, rails<->physics transitions preserve state vectors (1e-6 m)")),
    ("WP-11", ("LibNoise + PQSMod height/colour maths", "numerically", "golden `terrain_heights`: PQS surface height and vertex colour at a lat/lon grid for every body, bit-exact")),
    ("WP-12", ("PQS quadtree, mesh/collider build, PQSCity, land control", "verify by scenario", "scenario `ksc_ground`: KSC pad/runway/collider heights match KSP; collider heights match `terrain_heights`; LOD inspected")),
    ("WP-13", ("Part model and part tree", "numerically", "golden `craft_tree`: load every stock .craft; parent/child, attach nodes, symmetry groups, persistent ids, mass/cost equal")),
    ("WP-14", ("Vessel model, situations, mass properties", "numerically", "golden `vessel_state`: CoM, mass, MOI, situation, landed-at, control level for sample vessels")),
    ("WP-15", ("Resource flow", "numerically", "golden `resource_flow`: per-tick draw per part on stock craft (asparagus, crossfeed toggles, priorities, fuel lines, flow modes)")),
    ("WP-16", ("Staging, action groups, decouplers, decouple/couple tree ops", "numerically", "golden `staging_sequence`: inverse stages and separation indices for stock craft; vessel split results after each decouple")),
    ("WP-17", ("Delta-v calculation", "numerically", "golden `deltav_readout`: per-stage dV/TWR/ISP/burn time (vac, ASL, per body) for stock craft")),
    ("WP-18", ("Craft files and ship construction", "verify by scenario", "scenario `launch_craft`: every stock craft loads and spawns at pad/runway; costs match `part_database`")),
    ("WP-19", ("Vessel load/unload, pack/unpack, flight scene registry", "verify by scenario", "scenario `save_load_cycle`: positions/velocities within tolerance after load/quicksave/quickload; vessel switching and physics-range rules")),
    ("WP-20", ("Physics core: rigid-body/joint shim, floating origin, Krakensbane, collisions", "verify by scenario", "scenarios `free_fall`, `physics_orbit_hold` (energy drift), `crash_tolerance`, `joint_stiffness` vs KSP traces")),
    ("WP-21", ("Aerodynamics", "numerically", "golden `aero_forces` (per-part drag/lift vs velocity, density, Mach, AoA, occlusion) + scenario `ascent_profile`")),
    ("WP-22", ("Thermal model", "numerically", "golden `thermal_step`: skin/internal temperatures after N steps for re-entry and engine-heating cases")),
    ("WP-23", ("Engines, RCS, gimbals, intakes", "numerically", "golden `engine_thrust`: thrust/ISP/fuel flow vs throttle, pressure, velocity; flameout; scenario `ascent_profile`")),
    ("WP-24", ("Command, SAS/autopilot, reaction wheels, control state", "verify by scenario", "scenario `sas_hold`: attitude error vs time per SAS mode against KSP traces; torque availability vs golden")),
    ("WP-25", ("Docking, grapple, struts, vessel merge/split", "verify by scenario", "scenario `docking`: dock/undock, same-vessel docking, merged-vessel persistence and staging")),
    ("WP-26", ("Fairings, cargo bays, jettison, shielding, launch clamps", "numerically", "golden `shielding`: shielded part sets and fairing mass/cost for stock craft; fairing mesh inspected")),
    ("WP-27", ("Parachutes, deployables, animation-driven modules", "verify by scenario", "scenario `parachute_descent` (descent-rate golden) and `deploy_states` (deploy/retract/break)")),
    ("WP-28", ("Resource-processing modules (converters, drills, generators, solar, scanners)", "numerically", "golden `converter_rates`: outputs over time with EC/heat limits and crew bonuses; solar output vs sun angle/distance")),
    ("WP-29", ("Wheels and landing gear (VehiclePhysics subset port)", "verify by scenario", "scenarios `rover_drive` (top speed, brake distance, steering on runway) and `gear_settle` (suspension at rest)")),
    ("WP-30", ("Misc part modules (lights, flags, variants, animation helpers)", "verify by inspection", "behaviour inspection; variant switching changes mass/cost per `part_database`")),
    ("WP-31", ("Kerbal roster, experience, crew", "numerically", "golden `roster_xp`: XP/levels from flight logs; hiring costs; respawn timers")),
    ("WP-32", ("EVA kerbal controller", "verify by scenario", "scenario `eva_basics`: walk/run/swim speeds, jetpack dV, ladder grab, seat boarding, flag planting")),
    ("WP-33", ("Inventory and EVA construction", "verify by scenario", "scenario `eva_construction`: pick/attach/detach parts in flight; inventory mass/cost totals")),
    ("WP-34", ("IVA and internal props", "verify by inspection", "inspection of IVA instruments against flight state")),
    ("WP-35", ("Science, R&D and tech tree", "numerically", "golden `science_values` (subject value by body/situation/biome, diminishing returns, transmission) and `techtree`")),
    ("WP-36", ("Funds, reputation, strategies, facilities, recovery, progress tracking", "numerically", "golden `career_currency` (recovery value, strategy conversions, facility costs/limits) + scenario `progress_events`")),
    ("WP-37", ("Contract system core and stock templates", "numerically", "golden `contract_generation`: seeded offers, parameters and rewards from fixed save states; completion scenarios")),
    ("WP-38", ("FinePrint contracts, waypoints, Sentinel", "numerically", "golden `contract_generation` (FinePrint types) + waypoint positions; completion scenarios")),
    ("WP-39", ("CommNet and KerbNet", "numerically", "golden `commnet_graph`: link strengths, control paths, signal for a relay-network save; KerbNet FoV rules")),
    ("WP-40", ("Editor core: placement, symmetry, gizmos, pre-flight checks", "verify by scenario", "scenario `editor_build`: scripted placement with radial/mirror symmetry reproduces golden .craft; pre-flight checks match")),
    ("WP-41", ("Editor UI (part list, categories, craft browser, action-group UI)", "verify by inspection", "UI inspection against KSP screenshots")),
    ("WP-42", ("Scene flow, KSC, facilities, launch sites", "verify by scenario", "scenario `scene_flow`: main menu -> KSC -> VAB -> pad -> flight -> recovery -> KSC, persistence intact")),
    ("WP-43", ("Flight HUD (navball, staging UI, PAW, gauges, messages, crew portraits)", "verify by inspection", "UI inspection; displayed values come from core queries (spot-checked numerically)")),
    ("WP-44", ("Map view and tracking station", "verify by inspection", "inspection; drawn conics sampled from core `patched_conics` data")),
    ("WP-45", ("UI framework: DialogGUI, popups, tooltips, app launcher, widgets, rich text", "verify by inspection", "UI inspection; DialogGUI-built dialogs render equivalently")),
    ("WP-46", ("UI screens: main menu, settings, KSC building screens, load/save dialogs", "verify by inspection", "UI inspection")),
    ("WP-47", ("Cameras", "verify by inspection", "inspection of camera modes (auto/free/orbital/chase/locked, map, editor, IVA)")),
    ("WP-48", ("Rendering, FX, scaled-space visuals, asset import", "verify by inspection", "visual inspection; optional .mu importer verified on stock parts")),
    ("WP-49", ("Audio", "verify by inspection", "listening test")),
    ("WP-50", ("Input, bindings, input locks", "verify by scenario", "scenario `controls`: default bindings drive the expected FlightCtrlState/editor actions; input locks block correctly")),
    ("WP-51", ("Localization", "verify by inspection", "string-table import of KSP Localization cfgs; format shim unit tests (gender/plural)")),
    ("WP-52", ("Breaking Ground robotics", "verify by scenario", "scenario `servo_motion`: hinge/piston/rotor response and KAL-1000 sequences vs KSP traces")),
    ("WP-53", ("Breaking Ground deployed science and surface features", "numerically", "golden `deployed_science` (science accrual per tick) and `roc_placement` (seeded surface feature positions)")),
    ("WP-54", ("Making History mission runtime: Mission, MENode graph, flow, MissionSystem", "verify by scenario", "scenario `mission_playback`: stock missions run headless; node activation trace matches golden")),
    ("WP-55", ("Making History node library: tests, actions, adjusters", "verify by scenario", "per-node unit scenarios (each Test*/Action*/Adjuster* on a fixture save) + `mission_playback`")),
    ("WP-56", ("Making History mission editor UI", "verify by inspection", "UI inspection; saved .mission files round-trip through `confignode_roundtrip`")),
    ("WP-57", ("Tutorials", "verify by inspection", "play-through inspection")),
    ("WP-58", ("Debug tools and cheats -> console commands", "verify by inspection", "console commands exist for every CheatOptions flag gameplay reads")),
    ("WP-59", ("Drop list (platform, analytics, test scripts, obfuscator stubs, unused third-party)", "none", "nothing to build; confirm no gameplay reads")),
])

R = []
AB = {"A", "B"}


def rule(wp, path=None, subs=None, tiers=None):
    R.append((wp, re.compile(path) if path else None, set(subs) if subs else None, set(tiers) if tiers else None))


# third-party first
rule("WP-11", r"/LibNoise/")
rule("WP-29", r"/(VehiclePhysics|EdyCommonTools)/")
rule("WP-45", r"/(TMPro|SoftMasking)/")
rule("WP-48", r"/(Highlighting|DDSHeaders)/")
rule("WP-59", r"/(Smooth|UniLinq)/")
# explicit files
rule("WP-16", r"/KSP/UI/Screens/StageManager\.cs$")
rule("WP-35", r"/KSP/UI/Screens/RDTechTree\.cs$")
rule("WP-23", r"/(ModuleEngines|ModuleEnginesFX|ModuleRCS|ModuleRCSFX|MultiModeEngine|ModuleGimbal|ModuleResourceIntake|Propellant|EngineType|IEngineStatus|IThrustProvider)\.cs$")
rule("WP-00", r"/(Timing\d|TimingFI|TimingPre|TimingManager|CoroutineHost|MonoUtilities|HierarchyUtil|TransformExtension|GameObjectExtension|BlankMonoBehaviour)\.cs$")
rule("WP-01", r"/(FloatCurve|DoubleCurve|DoubleKeyframe|Vector3Curve|Vector4Curve|Vector3d|Vector2d|Vector4d|QuaternionD|Matrix4x4D|UtilMath|Mathfx|Vect|VectorExtensions|MinMaxFloat|PIDclamp|IPid|KSPRandom|QuaternionExtensions)\.cs$")
rule("WP-02", r"/(ConfigNode|IConfigNode|UrlDir|ParseExtensions|KSPParseable|ParserMethod|ITypeParser|\w+TypeParser)\.cs$")
rule("WP-06", r"/(PartModule|PartModuleList|BaseField|BaseFieldList|BaseEvent\w*|BaseAction\w*|BaseAxisField\w*|KSPField|KSPEvent|KSPAction|KSPAxisField|KSPModule|FieldAttribute|UI_\w+|VesselModule|VesselModuleManager|ScenarioModule|ProtoScenarioModule|ScenarioRunner|KSPScenario\w*|KSPAddon|AddonLoader|BasePAWGroup|IModuleInfo|PartModuleUtil|ScalarModuleSetHandler|IScalarModule)\.cs$", None, AB)
rule("WP-05", r"/(GameDatabase|PartLoader|AvailablePart|PartResourceLibrary|PartResourceDefinition\w*|PartUpgrade\w*|PartStatsUpgradeModule|DatabaseLoader|DatabaseLoaderAttrib|LoadingSystem|PartCategories|PhysicMaterialLibrary|PhysicMaterialDefinition|DragCubeSystem)\.cs$", None, AB)
rule("WP-21", r"/(FlightIntegrator|DragCube|DragCubeList|CenterOfLiftQuery|CenterOfThrustQuery|ModuleLiftingSurface|ModuleControlSurface|ModuleAeroSurface|ModuleDragModifier|ModuleDragAreaModifier|ILiftProvider|IMultipleDragCube|IAirstreamShield)\.cs$", None, AB)
rule("WP-17", r"/(DeltaV(?!App)\w+|VesselDeltaV)\.cs$", None, AB)
rule("WP-15", r"/(PartSet|ResourceFlowMode|ResourceBroker|IResource\w*|PartResource|PartResourceList|ProtoPartResourceSnapshot|ResourceTransferMode|ModuleToggleCrossfeed|IToggleCrossfeed)\.cs$|/RUI/Algorithms/")
rule("WP-19", r"/(ProtoVessel|ProtoPartSnapshot|FlightGlobals|VesselRanges|VesselSwitching|VesselRetrieval|OrbitPhysicsManager|FlightState|VesselPrecalculate)\.cs$")
rule("WP-18", r"/(ShipConstruct|ShipConstruction|ShipTemplate)\.cs$")
rule("WP-10", r"/(TimeWarp|WarpTransition\w*|OrbitDriver)\.cs$", None, AB)
rule("WP-08", r"/(PatchedConicSolver|PatchedConics|ManeuverNode|Trajectory|AlarmClockScenario|AlarmType\w*|AlarmActions|TransferType\w*|TransferData\w*)\.cs$", None, AB)
rule("WP-20", r"/(FloatingOrigin|Krakensbane|PhysicsGlobals|CollisionEnhancer\w*|CollisionManager|CollisionEventsHandler|ICollisionEvents|PartBuoyancy|PartJoint|ConfigurableJointExtensions|LocalPhysXGravity|Splashdown|RunwayCollisionHandler|PhysicsUtil|physicalObject|ModulePhysicMaterial)\.cs$", None, AB)
rule("WP-24", r"/(VesselAutopilot|FlightCtrlState|SteeringObject|ITorqueProvider|ISteerable|ModuleCommand|ModuleSAS|ModuleReactionWheel|ModuleRemoteController)\.cs$", None, AB)
rule("WP-25", r"/(ModuleDockingNode|ModuleGrappleNode|DockedVesselInfo|CModuleStrut)\.cs$", None, AB)
rule("WP-16", r"/(ModuleDecouple|ModuleDecouplerBase|ModuleAnchoredDecoupler|IStageSeparator\w*|IActivateOnDecouple|ActionGroupList|KSPActionGroup|KSPActionParam|KSPActionType|PreferredInitialStage|ModifierStagingSituation|StackIconGrouping)\.cs$", None, AB)
rule("WP-26", r"/(ModuleProceduralFairing|ModuleCargoBay|ModuleJettison|ModuleServiceModule|LaunchClamp|ModuleStructuralNode\w*|ModuleDynamicNodes|DynamicNodeSet)\.cs$|/ProceduralFairings/", None, AB)
rule("WP-27", r"/(ModuleParachute|ModuleDeployablePart|ModuleDeployableSolarPanel|ModuleDeployableAntenna|ModuleDeployableRadiator|ModuleAnimateGeneric|ModuleAnimationGroup|ModuleAnimationSetter|ModuleAnimatorLandingGear|IAnimatedModule|ModuleEvaChute)\.cs$", None, AB)
rule("WP-33", r"/(ModuleInventoryPart|ModuleCargoPart|EVAConstruction\w*|KerbalInventoryScenario|StoredPart|InventoryPanelController|PartItemTransfer|ConstructionMode|ConstructionEventType|IConstruction|UIPartActionInventory\w*|UIPartActionControllerInventory)\.cs$")
rule("WP-36", r"/(Funding|Reputation|Currency\w*|TransactionReasons|ProgressTracking|ProgressNode|ProgressTree|VesselRecovery|RecoverySituations|GameVariables|ScenarioUpgradeableFacilities|AdministrationFacility|ValueModifierQuery|ScenarioAchievements|ScenarioDestructibles)\.cs$", None, AB)
# directories / DLC
rule("WP-56", r"/Expansions/Missions/Editor/", None, {"C"})
rule("WP-55", r"/Expansions/Missions/(Tests|Actions|Adjusters)/")
rule("WP-54", r"/Expansions/Missions/", None, AB)
rule("WP-56", r"/Expansions/Missions/")
rule("WP-55", None, {"DLC-MakingHistory"}, AB)
rule("WP-56", None, {"DLC-MakingHistory"}, {"C"})
rule("WP-52", r"/Expansions/Serenity/(BaseServo|Module(Robotic\w*|Rotor|Propeller|Hinge|Piston|Robot\w*)|Servo\w*|ControlledAxis|ControlledAction|IRoboticServo|RobotArmFX/|RoboticController\w*|CurvePanel|KAL\w*|AxisFieldLimit)")
rule("WP-52", r"/(ActiveJoint\w*|ModuleJointMotor\w*|ModuleJointPivot|IActiveJointHost|IJointLockState|ModuleRotatingJoint|Rotato)\.cs$", None, AB)
rule("WP-53", r"/Expansions/Serenity/")
rule("WP-53", None, {"DLC-BreakingGround"}, AB)
rule("WP-48", None, {"DLC-BreakingGround"}, {"C"})
rule("WP-37", r"Assembly-CSharp/Contracts/")
rule("WP-38", r"/(FinePrint|SentinelMission)/")
rule("WP-39", r"/(CommNet|KerbNet)/", None, AB)
rule("WP-04", r"/SaveUpgradePipeline/")
rule("WP-31", r"/Experience/")
rule("WP-36", r"/(Strategies|Upgradeables|KSPAchievements)/", None, AB)
rule("WP-40", r"/(PreFlightTests|EditorGizmos)/", None, AB)
rule("WP-29", r"/ModuleWheels/", None, AB)
rule("WP-15", r"/CompoundParts/CModuleFuelLine")
rule("WP-13", r"/CompoundParts/", None, AB)
rule("WP-58", r"/(TestScripts|KSP/Testing|KSP/UI/Screens/DebugToolbar)/")
rule("WP-50", r"/(GameSettings|InputSettings|MiniKeyBindings)\.cs$")
rule("WP-46", r"(/KSP/UI/Screens/|/(MainMenu\w*|PauseMenu|KSCPauseMenu|\w*Settings\w*|DifficultyOptionsMenu|Flag\w*Browser\w*|LoadGameDialog|LoadGameSearch|ScenarioLoadDialog|ScenarioCreationOptions|Credits\w*|WhatsNew\w*|PrivacyDialog|SteamWorkshopExportDialog)\.cs$)",
     {"UI", "Save", "Scene", "Career", "Science", "Contracts", "CommNet", "Input"}, {"C"})

SUB_WP = {
    ("Core", "A"): "WP-03", ("Core", "B"): "WP-00",
    ("Save", "A"): "WP-04", ("Save", "B"): "WP-19",
    ("Modding", "A"): "WP-05", ("Modding", "B"): "WP-05",
    ("Scene", "A"): "WP-42", ("Scene", "B"): "WP-42",
    ("Orbit", "A"): "WP-07", ("Orbit", "B"): "WP-10",
    ("Celestials", "A"): "WP-09", ("Celestials", "B"): "WP-09",
    ("Terrain", "A"): "WP-11", ("Terrain", "B"): "WP-12",
    ("Physics", "A"): "WP-20", ("Physics", "B"): "WP-20",
    ("Vessel", "A"): "WP-14", ("Vessel", "B"): "WP-14",
    ("Part", "A"): "WP-13", ("Part", "B"): "WP-13",
    ("Resources", "A"): "WP-15", ("Resources", "B"): "WP-28",
    ("PartModules-Engines", "A"): "WP-23", ("PartModules-Engines", "B"): "WP-23",
    ("PartModules-Wheels", "A"): "WP-29", ("PartModules-Wheels", "B"): "WP-29",
    ("PartModules-Control", "A"): "WP-24", ("PartModules-Control", "B"): "WP-24",
    ("PartModules-Structural", "A"): "WP-26", ("PartModules-Structural", "B"): "WP-26",
    ("PartModules-Resources", "A"): "WP-28", ("PartModules-Resources", "B"): "WP-28",
    ("PartModules-Misc", "A"): "WP-30", ("PartModules-Misc", "B"): "WP-30",
    ("Aero", "A"): "WP-21", ("Aero", "B"): "WP-21",
    ("Thermal", "A"): "WP-22", ("Thermal", "B"): "WP-22",
    ("Editor", "A"): "WP-40", ("Editor", "B"): "WP-40",
    ("FlightUI", "A"): "WP-08", ("FlightUI", "B"): "WP-43",
    ("MapView", "A"): "WP-44", ("MapView", "B"): "WP-44",
    ("UI", "A"): "WP-45", ("UI", "B"): "WP-45",
    ("Career", "A"): "WP-36", ("Career", "B"): "WP-36",
    ("Science", "A"): "WP-35", ("Science", "B"): "WP-35",
    ("Contracts", "A"): "WP-37", ("Contracts", "B"): "WP-38",
    ("Kerbals", "A"): "WP-31", ("Kerbals", "B"): "WP-32",
    ("CommNet", "A"): "WP-39", ("CommNet", "B"): "WP-39",
    ("Input", "A"): "WP-50", ("Input", "B"): "WP-50",
    ("Localization", "A"): "WP-51", ("Localization", "B"): "WP-51",
    ("Tutorials", "A"): "WP-57", ("Tutorials", "B"): "WP-57",
    ("Debug", "A"): "WP-58", ("Debug", "B"): "WP-58",
    ("Rendering", "A"): "WP-48", ("Rendering", "B"): "WP-48",
    ("Camera", "A"): "WP-47", ("Camera", "B"): "WP-47",
    ("Audio", "A"): "WP-49", ("Audio", "B"): "WP-49",
    ("Misc", "A"): "WP-03", ("Misc", "B"): "WP-03",
}
C_WP = {"Editor": "WP-41", "FlightUI": "WP-43", "MapView": "WP-44", "UI": "WP-45", "Camera": "WP-47",
        "Rendering": "WP-48", "Audio": "WP-49", "Input": "WP-50", "Localization": "WP-51", "Tutorials": "WP-57",
        "Debug": "WP-58", "Steam": "WP-59", "Analytics": "WP-59", "Misc": "WP-59", "Kerbals": "WP-34",
        "Terrain": "WP-48", "Celestials": "WP-48", "Scene": "WP-42", "Save": "WP-46", "Modding": "WP-59",
        "CommNet": "WP-43", "Science": "WP-46", "Career": "WP-46", "Contracts": "WP-46", "Orbit": "WP-44",
        "Vessel": "WP-43", "Part": "WP-43", "Aero": "WP-58", "Thermal": "WP-43", "Physics": "WP-48",
        "Core": "WP-45", "PartModules-Engines": "WP-48", "PartModules-Misc": "WP-48",
        "PartModules-Resources": "WP-48", "PartModules-Structural": "WP-48", "PartModules-Control": "WP-43",
        "PartModules-Wheels": "WP-48", "Resources": "WP-43", "DLC-MakingHistory": "WP-56",
        "DLC-BreakingGround": "WP-48"}
KERBAL_IVA = re.compile(r"/(Internal\w*|IVA\w*|InternalSpace\w*|KerbalSeat|kerbalExpression\w*|KerbalExpressionAI)\.cs$")


def assign(r):
    p, sub, tier = r["path"], r["subsystem"], r["tier"]
    for wp, rx, subs, tiers in R:
        if rx and not rx.search(p):
            continue
        if subs and sub not in subs:
            continue
        if tiers and tier not in tiers:
            continue
        return wp
    if KERBAL_IVA.search(p) and tier != "A":
        return "WP-34"
    if tier == "C":
        return C_WP.get(sub, "WP-45")
    if tier == "third-party":
        return "WP-59"
    if sub in ("Steam", "Analytics"):
        return "WP-59"
    return SUB_WP.get((sub, tier), "WP-03")
