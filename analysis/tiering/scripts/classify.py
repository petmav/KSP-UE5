"""Auto-classification (suggested tier/subsystem), Unity dependency score, review digests.

Run: python -I classify.py           (after extract_features.py and depgraph.py)
Writes work/auto.json (per-file auto fields) and work/digests/chunk_NN.md + work/chunks.json
The hand review (work/review/chunk_NN.tsv) overrides tier/subsystem/ue5_counterpart/rationale; see merge.py.
"""
import json, math, os, re
from collections import OrderedDict

WORK = r"C:/Projects/KSP-UE5/analysis/tiering/work"

THIRD_PARTY = OrderedDict([
    ("TMPro", "UI"), ("Smooth", "Core"), ("VehiclePhysics", "PartModules-Wheels"),
    ("EdyCommonTools", "PartModules-Wheels"), ("LibNoise", "Terrain"), ("SoftMasking", "UI"),
    ("UniLinq", "Core"), ("DDSHeaders", "Rendering"), ("Highlighting", "Rendering"),
])

# ---------------------------------------------------------------- Unity dependency score
HEAVY = ["Rigidbody", "Physics.", "Raycast/Query", "Collider", "WheelCollider", "Joint", "PhysicMaterial",
         "Collision/Force", "Renderer", "Mesh", "Material", "Shader", "Texture", "Camera", "Light",
         "Graphics/GL", "LODGroup", "ParticleSystem", "Animation", "Audio", "Input.", "KeyCode",
         "UnityEngine.UI", "EventSystems", "TMPro", "IMGUI", "Resources.", "AssetBundle", "WWW/WebRequest",
         "Steam", "Analytics"]
OBJECT_MODEL = ["MonoBehaviour", "GameObject", "Transform", "GetComponent", "Instantiate/Destroy", "SetActive",
                "SendMessage/Invoke", "ScriptableObject", "Coroutine", "Lifecycle", "LayerMask", "SceneManager"]
UTIL = ["Time.", "Debug.", "Application.", "Screen/Cursor", "PlayerPrefs", "QualitySettings/SystemInfo",
        "AnimationCurve", "SerializeField"]
MATH = ["Vector2/3/4", "Quaternion", "Matrix4x4", "Mathf", "Color", "Rect/Bounds/Ray/Plane", "Random"]
PHYS = ["Rigidbody", "Physics.", "Raycast/Query", "Collider", "WheelCollider", "Joint", "PhysicMaterial", "Collision/Force"]
REND = ["Renderer", "Mesh", "Material", "Shader", "Texture", "Camera", "Light", "Graphics/GL", "LODGroup", "ParticleSystem"]
UIG = ["UnityEngine.UI", "EventSystems", "TMPro", "IMGUI"]
UNITY_COMPONENT_ROOTS = {"MonoBehaviour", "UIBehaviour", "Selectable", "Graphic", "MaskableGraphic", "Image", "Text",
                         "Button", "Toggle", "Slider", "ScrollRect", "InputField", "LayoutGroup", "BaseMeshEffect",
                         "BaseInputModule", "StandaloneInputModule", "Behaviour", "Component", "ScriptableObject",
                         "HorizontalOrVerticalLayoutGroup", "Dropdown", "Scrollbar", "RawImage", "Mask",
                         "PropertyAttribute", "PointerInputModule", "Shadow", "Outline", "ContentSizeFitter"}


def unity_score(api, lines, is_component):
    w = sum(3 * api[k] for k in HEAVY) + sum(2 * api[k] for k in OBJECT_MODEL) + \
        sum(1 * api[k] for k in UTIL) + sum(0.25 * api[k] for k in MATH)
    d = 100.0 * w / max(lines, 30)          # weighted Unity references per 100 lines
    s = 100.0 * d / (d + 25.0)             # saturating 0..100 (d=25 -> 50)
    if is_component:
        s = min(100.0, s + 15)             # lifecycle-driven by the Unity player loop
    return int(round(s))


def kind_of(r):
    p = r["primary"]
    if not p:
        return "assembly-attrs"
    anc = r["ancestry"]
    k = p["kind"]
    if k in ("enum", "interface", "delegate"):
        return k
    if k == "struct":
        return "struct"
    if p["name"] == "PartModule" or "PartModule" in anc:
        return "PartModule"
    if p["name"] == "VesselModule" or "VesselModule" in anc:
        return "VesselModule"
    if p["name"] == "ScenarioModule" or "ScenarioModule" in anc:
        return "ScenarioModule"
    if "ContractParameter" in anc:
        return "ContractParameter"
    if "Contract" in anc:
        return "Contract"
    if "PQSMod" in anc:
        return "PQSMod"
    if r["addon"]:
        return "KSPAddon"
    if "ScriptableObject" in anc:
        return "ScriptableObject"
    if any(a in UNITY_COMPONENT_ROOTS for a in anc) or p["name"] == "MonoBehaviour":
        if any(a in ("UIBehaviour", "Selectable", "Graphic", "MaskableGraphic") for a in anc):
            return "UIBehaviour"
        return "MonoBehaviour"
    if "static" in p["mods"]:
        return "static class"
    if p["name"].endswith("Attribute") or "Attribute" in anc:
        return "attribute"
    return "class"


# ---------------------------------------------------------------- subsystem suggestion rules
PATH_RULES = [
    (r"^Assembly-CSharp-firstpass/Lingoona/", "Localization"),
    (r"^Assembly-CSharp-firstpass/LingoonaGrammarFormatter", "Localization"),
    (r"^Assembly-CSharp-firstpass/SpaceNavigator", "Input"),
    (r"^Assembly-CSharp-firstpass/(Controller|FlyAround|SmoothFollow2)", "Camera"),
    (r"^Assembly-CSharp-firstpass/", "Rendering"),
    (r"Expansions/Missions/", "DLC-MakingHistory"),
    (r"Expansions/Serenity/", "DLC-BreakingGround"),
    (r"Expansions/[^/]+$", "Modding"),
    (r"/(CommNet|KerbNet)/", "CommNet"),
    (r"/(Contracts|FinePrint|SentinelMission)/", "Contracts"),
    (r"/Experience/", "Kerbals"),
    (r"/(Strategies|Upgradeables|KSPAchievements)/", "Career"),
    (r"/SaveUpgradePipeline/", "Save"),
    (r"/PreFlightTests/", "Editor"),
    (r"/ModuleWheels/", "PartModules-Wheels"),
    (r"/(ProceduralFairings|CompoundParts)/", "PartModules-Structural"),
    (r"/Radiators/", "Thermal"),
    (r"/(CameraFXModules|CameraKeyFrameEvents)/", "Camera"),
    (r"/(Cursors|RUI)/", "UI"),
    (r"/EditorGizmos/", "Editor"),
    (r"/FlightUIModeControllerUtil/", "FlightUI"),
    (r"/KSP/UI/Screens/DebugToolbar/", "Debug"),
    (r"/KSP/UI/Screens/Editor/", "Editor"),
    (r"/KSP/UI/Screens/PartListCategories/", "Editor"),
    (r"/KSP/UI/Screens/Flight/", "FlightUI"),
    (r"/KSP/UI/Screens/Mapview/", "MapView"),
    (r"/KSP/(Localization|UI/Language)/", "Localization"),
    (r"/KSP/IO/", "Save"),
    (r"/KSP/Testing/", "Debug"),
    (r"/KSP/FX/", "Rendering"),
    (r"/KSP/UI/", "UI"),
    (r"/TestScripts/", "Debug"),
    (r"/PartToolsLib/", "Rendering"),
    (r"/(KSPAssets|Assets)/", "Modding"),
    (r"/UnityEngine/", "Core"),
    (r"/(A|Properties)/", "Misc"),
]
NAME_RULES = [
    (r"^ROC|^ModuleGround|^ModuleJointMotor|^ModuleJointPivot|^ActiveJoint|^IActiveJointHost|^IJointLockState|^SerenityAboutDialog|^RocCB|^ROCsSFX", "DLC-BreakingGround"),
    (r"^(ME[A-Z]|MC[A-Z]|GAP|MakingHistoryAbout|METutorial|MissionBriefingTag|MEBanner)", "DLC-MakingHistory"),
    (r"^Tutorial|Tutorial|TutorialStep$|^KerbalInstructor|^MunOrBustLogic|^ScenarioNewGameIntro", "Tutorials"),
    (r"^(PQ|PQS|PQSMod_|PQSCity|PQSLandControl|LandClass|SurfaceObject|Simplex|NoiseGenerator|MapSO|SphereBaseSO)", "Terrain"),
    (r"^(DatabaseLoaderTexture|DatabaseLoaderModel|PartReader|MBMReader|TGA|TextureTools|TextureScale|Material|.*MaterialProperty$|Shader|PropertyIDs|FX(Prefab|Group|Monger|Object|Curve|Module(?!AnimateThrottle|ThrottleEffects))|Effect|Particle|KSPParticleEmitter|Detonator|Explosion|Prefab|Model(Animation|MultiParticle|Particle)FX|SmokeTrail|ReentryFX|AerodynamicsFX|AeroFXState|SurfaceFX|ModuleSurfaceFX|CometVFX|CometParticle|LaunchPadFX|WeldFX|VFXSequencer|DisplacementExplosion|Flag(Decal|Mesh|Orientation)|InternalFlagDecal|DynamicAmbientLight|DynamicShadowSettings|LightStruct|FlightReflectionProbe|UnderwaterFog|ScaledSpaceFader|EnvMapAnimator|ShaderTimeOffset|NoiseEffect|LinkedMesh|MeshLocator|DrawDepth|LayerUtil|ScreenSpaceObjectScaling|VolumetricObjectBase|BoxVolume|ConeVolume|SphereVolume|Occlusion|TimeOfDayAnimation|DayNightGameObjectSwitch|CraftThumbnail|ScreenShot|AtmosphereFromGround|SunFlare|SunCoronas|SunShaderController|ScaledSun|SkySphere|ProceduralStarfield|ProceduralCubemap|Galaxy|GasGiant|AltitudeMultiTextureRamp|BufferMaterialProperty|AnimationFX|AudioFX(?!Settings)|MaterialColorUpdater|ModuleColorChanger|ModuleLight|FlagDecalBackground|ModuleHeatEffects|ModuleAnimateHeat|ColorImage|ModuleSeeThroughObject|PartModuleFXSetter|test_particles|LoadingBufferMask|KerbalVisorGlossiness|MaterialBasedOnGraphicsSetting)", "Rendering"),
    (r"^(Audio|Music|MenuMusic|CreditsMusic|AmbienceControl|Volume(Controller|Normalizer)|PauseAudioFadeHandler|AlarmClockScenarioAudio)", "Audio"),
    (r"(Camera(2)?$|^Camera|^TrackIR|^SpaceNavigator|^IKSPCamera|^IGameCamera|^KSPCameraUtil|^SimpleOrbitController|^TrackRigObject|^CameraFade|^CameraOffCenter|^CameraOrbitHold|^CameraMouseLook|^FollowRot|^KeepRot|^LookAtObject)", "Camera"),
    (r"^(Steam|KSPSteamUtils)", "Steam"),
    (r"^(Analytics|PrivacyDialog)", "Analytics"),
    (r"^(Localizer|Lingoona|KSPFont|FontLoader|KSPKerningPairList|LingoonaGrammarExtensions)", "Localization"),
    (r"^(Orbit(?!Renderer|TargetRenderer)|PatchedConicSolver|PatchedConics$|Trajectory|ManeuverNode$|Transfer|Targeting|ITargetable|DirectionTarget|PositionTarget|NavWaypoint|ProtoTargetInfo|VesselTargetModes|FoRModes|OrbitUtil|TimeWarp|WarpTransition|CometOrbitType)", "Orbit"),
    (r"^(Map|ManeuverGizmo|ManeuverNodeEditor|ManeuverTool|PatchRendering|PatchedConicRenderer|OrbitRenderer|OrbitTargetRenderer|ContractOrbitRenderer|AngleRenderEject|AlarmMapNodeButton|VesselLabel)", "MapView"),
    (r"^(CelestialBody|PSystem|Planetarium$|Sun$|ScaledSpace|ScaledMovement|ScaledFloatingOrigin|CBAttributeMap|CBTextureAtlas|Comet(?!VFX|Particle)|PComet|PAsteroid|ProceduralAsteroid|ProceduralComet|ProceduralSpaceObject|PSpaceObject|SpaceObject|ScenarioDiscoverableObjects|Discover|IDiscoverable|UntrackedObjectClass|MiniBiome|BiomeLockData|LatLon|AsteroidCollider|ModuleAsteroid$|ModuleComet$|ModuleAsteroidInfo|ModuleCometInfo|ModuleSpaceObjectInfo|CometVessel|AtmosphereProbe)", "Celestials"),
    (r"^(Module(Engines|EnginesFX|Gimbal|RCS|RCSFX|ResourceIntake)$|MultiModeEngine|Propellant|EngineType|IEngineStatus|IThrustProvider|FXModuleAnimateThrottle|FXModuleThrottleEffects|IThrottleControl)", "PartModules-Engines"),
    (r"^(ModuleWheel|KSPWheel|Wheel|SuspensionLoadBalancer|ModuleAnimatorLandingGear|TestVehicleController)", "PartModules-Wheels"),
    (r"^(Module(Command|SAS|ReactionWheel|RemoteController)$|ITorqueProvider|ISteerable|VesselAutopilot|ModuleFlightDisplay)", "PartModules-Control"),
    (r"^(Module(Decouple|DecouplerBase|AnchoredDecoupler|DockingNode|GrappleNode|ProceduralFairing|CargoBay|Jettison|StructuralNode|StructuralNodeToggle|DynamicNodes|Parachute|ToggleCrossfeed|FuelJettison|ServiceModule|DisplaceTweak)$|LaunchClamp|CompoundPart|IStageSeparator|IActivateOnDecouple|IToggleCrossfeed|DockedVesselInfo|IDynamicCargoOccluder)", "PartModules-Structural"),
    (r"^(Module(Generator|Alternator|ResourceConverter|ResourceHarvester|AsteroidDrill|CometDrill|ResourceScanner|ResourceDrain|AnalysisResource|BiomeScanner|OrbitalScanner|OrbitalSurveyor|DeployableSolarPanel|ResourceAutoShiftState|ResourceHandler|SpaceObjectResource|AsteroidResource|CometResource|Resource)$|BaseConverter|BaseDrill|ResourceConverter|ConversionRecipe|ConverterResults|ResourceRatio)", "PartModules-Resources"),
    (r"^(PartResource|Resource|PartSet$|Planetary|AbundanceRequest|HarvestTypes|Depletion|DistributionData|PlanetScanData|EVAResourceContainer|FuelFlowOverlay|IResource)", "Resources"),
    (r"^(Module(ActiveRadiator|DeployableRadiator|Ablator|CoreHeat|ConductionMultiplier|OverheatDisplay)$|PartThermalData|ThermalLink|TemperatureModel|IAnalytic|IOverheat)", "Thermal"),
    (r"^(FlightIntegrator|DragCube|Module(LiftingSurface|ControlSurface|AeroSurface|DragModifier|DragAreaModifier)$|ILiftProvider|CenterOf(Lift|Thrust)Query|AeroGUI|IAirstreamShield|IMultipleDragCube|PartBuoyancy)", "Aero"),
    (r"^(Science|ModuleScience|ExperimentResultDialog|Experiment|ResearchAndDevelopment|RDTech|ProtoTechNode|ProtoRDNode|RnD|TechPerk|ITechPerk|ModuleEnviroSensor|Module(Asteroid|Comet|SpaceObject)Analysis|ModuleSample|ModuleGPS|ModuleTripLogger|KnowledgeItem|CelestialBodyScienceParams|IScience|ScienceLabSearch|ModuleTestSubject|PartTestConstraint)", "Science"),
    (r"^(Funding|Reputation|Currency|TransactionReasons|AdministrationFacility|GameVariables|Progress|ScenarioUpgradeableFacilities|KSCUpgradeable|FundsWidget|ReputationWidget|ScienceWidget|CostWidget|VesselRecovery|RecoverySituations|ValueModifierQuery|ScenarioAchievements|ScenarioDestructibles|DestructibleBuilding)", "Career"),
    (r"^(Kerbal|ProtoCrewMember|Crew|PartCrewManifest|VesselCrewManifest|Internal|IVA|EVA|FlightEVA|Suit|HelmetSuit|AdvancedRagdoll|kerbalExpression|CharacterAnimation|MenuRandomKerbalAnims|Mechanic|GroundCrew|Scientist|ModuleInventoryPart|ModuleCargoPart|InventoryPanelController|StoredPart|RetractableLadder|LadderEndCheck|ModuleEvaChute|ModuleExperienceManagement|APSkillExtensions|SpaceCenterCrew|ProtoCrew)", "Kerbals"),
    (r"^(ModuleDataTransmitter|ModuleDeployableAntenna|AntennaType|ICommAntenna|ModuleKerbNetAccess|DishController|TrackingStationDishMovement|RnDCommsStream)", "CommNet"),
    (r"^(ConfigNode|IConfigNode|Game$|GamePersistence|GameParameters|Proto|QuickSaveLoad|GameBackup|KSPUpgradePipeline|SaveMode|ClearToSaveStatus|Persistent|ShipTemplate|ShipConstruct$|ShipConstruction|IPersistence|FlightState|Versioning|VersionCompare|FlightAutoSave|LoadGameDialog|LoadGameSearch|ScenarioLoadDialog|ScenarioCreationOptions|KSPCompression|IShipconstruct|IShipConstructIDChanges)", "Save"),
    (r"^(GameDatabase|UrlDir|AssemblyLoader|AddonLoader|KSPAddon|KSPAssembly|DatabaseLoader$|DatabaseLoaderAttrib|DatabaseLoaderAudio|DatabaseGameObject|LoadingSystem|PartUpgrade|PartStatsUpgradeModule|ReflectionUtil|ParserMethod|.*TypeParser$|KSPParseable|ITypeParser|IModular|AssetBase|ScenarioModule$|KSPScenario|ScenarioRunner)", "Modding"),
    (r"^(HighLogic|GameScenes|SceneTransitionMatrix|LoadingScreen|LoadingAnimation|MainMenu|SpaceCenter$|SpaceCenterMain|FlightDriver|EditorDriver|Timing|DestroyOnSceneSwitch|ResetTransformOnSceneSwitch|DoNotDestroy|CreditsSceneMain|PauseMenu|KSCPauseMenu|SpaceCenterBuilding|.*Building$|.*Facility$|LaunchSite|KSCSiteNode|SiteNode|ISiteNode|SpaceCenterSurfaceObject|SandCastleLogic|WhackAKerbal|MenuNavigation)", "Scene"),
    (r"^(FloatingOrigin|Krakensbane|PhysicsGlobals|PhysicsUtil|Collision|ICollisionEvents|LocalPhysXGravity|LocalParticleGravity|OrbitPhysicsManager|physicalObject|PhysicMaterial|ModulePhysicMaterial|RunwayCollisionHandler|Splashdown|CrashObjectName|ConfigurableJointExtensions|ModuleRotatingJoint|Rotato|Bobber|MenuGravityOverride|Debug_FloatNaNs)", "Physics"),
    (r"^(Part|AttachNode|Attach|AvailablePart|BaseField|BaseEvent|BaseAction|BaseAxis|BasePAWGroup|BaseLabel|KSPField|KSPEvent|KSPAction|KSPAxis|KSPModule|KSPPart|IPart|ModulePartVariants|IModuleInfo|IScalarModule|ScalarModuleSetHandler|IAnimatedModule|ModuleAnimate|ModuleAnimation|ModuleDeployablePart|ModuleStatusLight|ModulePartFirework|ModuleMirrorPlacement|ModuleEffectTest|FieldAttribute|UI_|AvailableVariantTheme|ModelPartEvent|KSPModelEvent|KSPCapability|Capability|IJointLockState|IConstruction)", "Part"),
    (r"^(Vessel|FlightGlobals|FlightCtrlState|ActionGroup|KSPActionGroup|KSPActionParam|KSPActionType|DeltaV(?!App)|Stag|StackIconGrouping|PreferredInitialStage|ModifierStagingSituation|SymmetryMethod|FlightLog|FlightEvents|FlightEndModes|IVesselAutoRename|DirectoryActionGroup|SteeringObject)", "Vessel"),
    (r"^(Editor|PartCategories|PartSelector|ConstructionMode|ConstructionEventType|LoadCraftDialog|PreFlightCheck|CraftBrowser|EVAConstruction)", "Editor"),
    (r"^(UIPartAction|ScreenMessage|NavBall|Altimeter|FlightUI|FlightOverlays|FlightVesselOverlays|ConstructionVesselOverlays|ActionGroupsFlightController|Gauge|LED|LinearGauge|HelixGauge|Tumbler|ArrowPointer|AlarmClock|AlarmType|AlarmActions|DeltaVApp|FlightCoMTracker|VesselAutopilotUI)", "FlightUI"),
    (r"^(Input|GameSettings|KeyBinding|KeyCode|Axis|Mouse|ExtendedInput|FlightInput|MenuNavInput|StandaloneInputModuleCustom|KeyboardLayout|MiniKeyBindings|ControlTypes|InputLockManager)", "Input"),
    (r"^(Debug|PDebug|ConsoleCommands|CheatOptions|UnitTestingWindow|ProfileTimer|Performance|.*Test(er)?$|test_|ConeShadowTestRotate|InspectorKeyValuePair|SystemInformation|DebugDrawUtil|GizmoDraw)", "Debug"),
    (r"^(Dialog|Popup|MultiOptionDialog|UI|Color|ComboButton|ComboSelector|X(Button|Graphic|Hoverable|Selectable)|Tooltip|CanvasUtil|MainCanvasUtil|AppUIMaster|TMProUtil|TMPProUtil|RichTextUtil|KSPRichTextUtil|SVBoxSlider|HexColorField|HSV|HsvColor|PieChart|ValueBox|TiltWindow|DragPanel|GUILayoutx|GUIUtil|ModalGUI|TextButton3D|TextProButton3D|TextMeshQueue|ScrollRect|Credits|WhatsNew|FlagBrowser|ButtonHighlighter|CanvasPixelPerfectHandler|PreviewPanel|PreviewType|XKCDColors|DefaultIcons|AtlasSplitter|ConfirmDialog|SavingBtnSoftLock|ChatController|MiniSettings|VideoSettings|GameplaySettingsScreen|DifficultyOptionsMenu|InputFieldHandler|TimeInputFieldHandler|SliderFocusType|DoubleSlider|SuitButton|iTween|iT$|MessageSystem|FacilityScreenFitter|ColliderMenuMouseChecker|MouseEventsHandler|MouseRayEventsHandler|IMouseEvents|CursorController|PartPointer|MinMaxColor)", "UI"),
    (r"^(Vector|Matrix4x4D|QuaternionD|UtilMath|Mathfx|Vect$|DoubleCurve|DoubleKeyframe|FloatCurve|Vector3Curve|Vector4Curve|MinMax|.*Extensions?$|ListDictionary|BiDictionary|DictionaryValueList|StringBuilderCache|Callback|DelegateExtensions|Event|GameEvents|BaseGameEvent|KFSM|KSPUtil|KSPRandom|MemoryCache|CoroutineHost|MonoUtilities|HierarchyUtil|PIDclamp|IPid|SafeAbortBackgroundWorker|SmoothDisposer|Drawing|BlankMonoBehaviour|KSPLog|IDateFormatter|IDateTimeFormatter|ISettings|IntersectInformation|ParseExtensions|TimedObjectDestructor|DestroyAfterTime|MeshOp|QuaternionExtensions|SimplexNoise)", "Core"),
]


def suggest_subsystem(path):
    rel = "/" + path.split("/", 1)[1] if path.startswith("Assembly-CSharp/") else path
    for top, sub in THIRD_PARTY.items():
        if rel.startswith("/" + top + "/"):
            return sub
    full = path
    for pat, sub in PATH_RULES:
        if re.search(pat, full):
            return sub
    stem = os.path.splitext(os.path.basename(path))[0]
    for pat, sub in NAME_RULES:
        if re.search(pat, stem):
            return sub
    return "Misc"


C_SUBSYSTEMS = {"UI", "Audio", "Camera", "Rendering", "Steam", "Analytics", "Debug", "Localization"}


def suggest_tier(r, kind, subsystem, api):
    phys = sum(api[k] for k in PHYS)
    rend = sum(api[k] for k in REND)
    ui = sum(api[k] for k in UIG)
    om = sum(api[k] for k in ["GameObject", "Transform", "GetComponent", "Instantiate/Destroy", "SetActive"])
    anim = api["Animation"]
    if kind in ("enum", "interface", "delegate", "attribute"):
        return "C" if subsystem in C_SUBSYSTEMS else "A"
    if subsystem in C_SUBSYSTEMS:
        return "C"
    if ui >= 3 and phys == 0:
        return "C"
    if kind in ("UIBehaviour",):
        return "C"
    component = kind in ("MonoBehaviour", "PartModule", "VesselModule", "ScenarioModule", "KSPAddon", "PQSMod")
    if rend > 6 and phys == 0 and subsystem not in ("Terrain",):
        return "C"
    if phys > 0 or anim > 2:
        return "B"
    if not component:
        return "A" if (rend + om) <= 3 else "B"
    if kind == "PQSMod":
        return "A" if (rend + om) <= 4 else "B"
    if kind in ("ScenarioModule", "PartModule", "VesselModule") and om <= 4 and rend == 0:
        return "A"
    return "B"


def main():
    rows = [json.loads(l) for l in open(os.path.join(WORK, "features.jsonl"), encoding="utf-8")]
    dep = json.load(open(os.path.join(WORK, "depgraph.json"), encoding="utf-8"))
    auto = {}
    for r in rows:
        kind = kind_of(r)
        api = r["api"]
        comp = kind in ("MonoBehaviour", "PartModule", "VesselModule", "ScenarioModule", "KSPAddon", "PQSMod",
                        "UIBehaviour")
        score = unity_score(api, r["lines"], comp)
        sub = suggest_subsystem(r["path"])
        tp = next((t for t in THIRD_PARTY if r["path"].startswith("Assembly-CSharp/" + t + "/")), None)
        tier = "third-party" if tp else suggest_tier(r, kind, sub, api)
        unity_top = sorted(((v, k) for k, v in api.items() if v and k not in MATH + ["Debug.", "SerializeField"]), reverse=True)[:6]
        ksp_top = sorted(((v, k) for k, v in r["ksp"].items() if v), reverse=True)[:6]
        p = r["primary"] or {}
        auto[r["path"]] = {
            "lines": r["lines"], "kind": kind, "type": p.get("name", ""), "bases": p.get("bases", []),
            "ancestry": r["ancestry"], "namespace": r["namespace"], "usings": r["usings"],
            "unity_score": score, "subsystem": sub, "tier": tier, "third_party": tp or "",
            "unity_top": [f"{k}={v}" for v, k in unity_top], "ksp_top": [f"{k}={v}" for v, k in ksp_top],
            "fan_in": len(dep[r["path"]]["in"]), "fan_out": len(dep[r["path"]]["out"]),
            "methods": sorted(set(r["methods"])),
        }
    json.dump(auto, open(os.path.join(WORK, "auto.json"), "w", encoding="utf-8"))

    # ------------------------------------------------------------ review chunks (KSP-authored files only)
    def group(path):
        rel = path.split("/", 1)[1]
        if path.startswith("Assembly-CSharp-firstpass/"):
            return "small"
        top = rel.split("/")[0]
        if "/" not in rel:
            return "root"
        if top == "KSP":
            return "KSP"
        if rel.startswith("Expansions/Missions/Editor/") or rel.startswith("Expansions/Missions/Adjusters/"):
            return "ExpA"
        if top == "Expansions":
            return "ExpB"
        if top in ("CommNet", "KerbNet", "Contracts", "FinePrint", "SentinelMission"):
            return "Career"
        return "small"
    groups = OrderedDict((g, []) for g in ["root", "KSP", "ExpA", "ExpB", "Career", "small"])
    for path in sorted(auto, key=str.lower):
        if auto[path]["third_party"]:
            continue
        groups[group(path)].append(path)
    splits = {"root": 5, "KSP": 2, "ExpA": 1, "ExpB": 1, "Career": 1, "small": 1}
    chunks = []
    for g, paths in groups.items():
        n = splits[g]
        size = math.ceil(len(paths) / n)
        for i in range(n):
            part = paths[i * size:(i + 1) * size]
            if part:
                chunks.append({"id": f"{len(chunks) + 1:02d}", "group": g, "paths": part})
    os.makedirs(os.path.join(WORK, "digests"), exist_ok=True)
    os.makedirs(os.path.join(WORK, "review"), exist_ok=True)
    for c in chunks:
        lines = [f"# Review chunk {c['id']} ({c['group']}): {len(c['paths'])} files, "
                 f"{sum(auto[p]['lines'] for p in c['paths'])} lines", ""]
        for p in c["paths"]:
            a = auto[p]
            must = " **MUST-OPEN (>800 lines)**" if a["lines"] > 800 else ""
            lines.append(f"## {p}{must}")
            lines.append(f"- {a['lines']} lines; kind={a['kind']}; type={a['type']}; bases={','.join(a['bases']) or '-'}; "
                         f"ancestry={'/'.join(a['ancestry'][:4]) or '-'}; ns={a['namespace'] or '(global)'}")
            lines.append(f"- usings: {', '.join(u for u in a['usings'] if not u.startswith('System')) or '-'}")
            lines.append(f"- unity: {', '.join(a['unity_top']) or 'none'}; unity_score={a['unity_score']}")
            lines.append(f"- ksp refs: {', '.join(a['ksp_top']) or 'none'}; fan_in={a['fan_in']} fan_out={a['fan_out']}")
            ms = a["methods"]
            lines.append(f"- methods({len(ms)}): {', '.join(ms[:45])}{' ...' if len(ms) > 45 else ''}")
            lines.append(f"- auto suggestion: tier={a['tier']} subsystem={a['subsystem']}")
            lines.append("")
        open(os.path.join(WORK, "digests", f"chunk_{c['id']}.md"), "w", encoding="utf-8").write("\n".join(lines))
    json.dump(chunks, open(os.path.join(WORK, "chunks.json"), "w", encoding="utf-8"), indent=1)
    for c in chunks:
        print(c["id"], c["group"], len(c["paths"]), sum(auto[p]["lines"] for p in c["paths"]),
              sum(1 for p in c["paths"] if auto[p]["lines"] > 800), c["paths"][0], "..", c["paths"][-1])
    from collections import Counter
    print(Counter(a["tier"] for a in auto.values()))
    print(Counter(a["subsystem"] for a in auto.values()).most_common())


if __name__ == "__main__":
    main()
