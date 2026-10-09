# Review chunk 05 (root): 278 files, 65231 lines

## Assembly-CSharp/ScenarioDestructibles.cs
- 253 lines; kind=ScenarioModule; type=ScenarioDestructibles; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=1, Instantiate/Destroy=1; unity_score=22
- ksp refs: ConfigNode=9, ScenarioModule=1; fan_in=3 fan_out=7
- methods(14): AddRef, AddTo, GetDamage, GetFacility, GetFacilityDamage, Load, OnAwake, OnDestroy, OnLoad, OnSave, RegisterDestructible, RemoveFrom, Save, UnregisterDestructible
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/ScenarioDiscoverableObjects.cs
- 390 lines; kind=ScenarioModule; type=ScenarioDiscoverableObjects; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSPAchievements, UnityEngine
- unity: GameObject=5, Coroutine=3, Lifecycle=2, Instantiate/Destroy=1; unity_score=43
- ksp refs: GameEvents=6, FlightGlobals=6, ConfigNode=6, Vessel=4, Orbit=4, ProtoVessel=2; fan_in=6 fan_out=29
- methods(22): DebugSpawnProbability, ExpireDiscoveredObjects, GetRandomDuration, GetSizeCurveBasedClass, LoadGameDBSettings, OnAwake, OnDestroy, OnKnowledgeChanged, OnLoad, OnSave, OnVesselDestroy, ReachedBody, SaveIDList, SpawnAsteroid, SpawnComet, SpawnDaemon, SpawnDresAsteroid, SpawnHomeAsteroid, SpawnLastAsteroid, Start, UpdateSpaceObjects, loadIDList
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/ScenarioLoadDialog.cs
- 578 lines; kind=MonoBehaviour; type=ScenarioLoadDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.Networking, UnityEngine.UI
- unity: GameObject=20, Texture=14, UnityEngine.UI=12, SetActive=9, Transform=6, Instantiate/Destroy=6; unity_score=76
- ksp refs: Localizer=13, Game=2, ConfigNode=2, GameDatabase=1, FlightGlobals=1; fan_in=1 fan_out=23
- methods(27): BuildList, CanRestart, CancelLoadGame, ClearListItems, ConfirmLoadGame, ConfirmRestart, Create, CreateWidget, DismissRestartGame, FinishedCallback, GatherSaveFiles, GatherScenarioInfo, LoadBannerTexture, LoadPath, OnBtnCancel, OnBtnScnLink, OnButtonLoad, OnButtonRestart, OnDestroy, OnSelectionChanged, SetBanner, SetExplicitNavigation, SetHidden, SetLocked, Start, Terminate, UpdateBlacklist
- auto suggestion: tier=C subsystem=Save

## Assembly-CSharp/ScenarioModule.cs
- 87 lines; kind=ScenarioModule; type=ScenarioModule; bases=MonoBehaviour,IConfigNode; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=46
- ksp refs: ConfigNode=4, ScenarioModule=1; fan_in=27 fan_out=7
- methods(7): Awake, Load, ModularSetup, OnAwake, OnLoad, OnSave, Save
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/ScenarioNewGameIntro.cs
- 211 lines; kind=ScenarioModule; type=ScenarioNewGameIntro; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=18
- ksp refs: Localizer=34, HighLogic=9, InputLockManager=6, GameEvents=2, ConfigNode=2, GameSettings=1; fan_in=0 fan_out=20
- methods(10): EditorTutorialSetup, KSCtutorialSetup, OnAssetSetup, OnAwake, OnLoad, OnOnDestroy, OnSave, OnTutorialSetup, TSTutorialSetup, onLeavingScene
- auto suggestion: tier=A subsystem=Tutorials

## Assembly-CSharp/ScenarioRunner.cs
- 213 lines; kind=MonoBehaviour; type=ScenarioRunner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Instantiate/Destroy=4, Lifecycle=2, MonoBehaviour=1, GetComponent=1, GameObject=1; unity_score=45
- ksp refs: ScenarioModule=10, GameEvents=4, ConfigNode=3, HighLogic=2; fan_in=6 fan_out=8
- methods(13): AddMainMenuScenarios, AddModule, Awake, ClearModules, GetLoadedModules, GetUpdatedProtoModules, LoadModules, OnDestroy, OnGameSceneLoadRequested, RemoveModule, SetProtoModules, UpdateModules, onLevelWasLoaded
- auto suggestion: tier=B subsystem=Modding

## Assembly-CSharp/ScenarioUpgradeableFacilities.cs
- 402 lines; kind=ScenarioModule; type=ScenarioUpgradeableFacilities; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions, KSP.Localization, UnityEngine, Upgradeables
- unity: Transform=2, GetComponent=2, Lifecycle=1, Instantiate/Destroy=1, Application.=1; unity_score=31
- ksp refs: ConfigNode=19, Localizer=9, HighLogic=4, GameEvents=2, Game=2, ScenarioModule=1; fan_in=54 fan_out=17
- methods(21): AddRef, CheatFacilities, CleanUpUnmanagedData, GetFacilityLevel, GetFacilityLevelCount, GetFacilityName, GetLevel, GetLevelCount, IsLaunchPad, IsRunway, Load, OnAwake, OnDestroy, OnLoad, OnSave, RegisterUpgradeable, Save, SlashSanitize, UnregisterUpgradeable, getInitialMissionState, getInitialState
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/SceneTransitionMatrix.cs
- 22 lines; kind=class; type=SceneTransitionMatrix; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=21
- ksp refs: none; fan_in=1 fan_out=1
- methods(1): GetTransitionValue
- auto suggestion: tier=A subsystem=Scene

## Assembly-CSharp/ScienceData.cs
- 100 lines; kind=class; type=ScienceData; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=3, ResearchAndDevelopment=2; fan_in=21 fan_out=5
- methods(3): CopyOf, Load, Save
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ScienceExperiment.cs
- 114 lines; kind=class; type=ScienceExperiment; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=3
- ksp refs: ConfigNode=4, ResearchAndDevelopment=1, CelestialBody=1; fan_in=24 fan_out=7
- methods(5): BiomeIsRelevantWhile, IsAvailableWhile, IsUnlocked, Load, Save
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ScienceLabSearch.cs
- 203 lines; kind=class; type=ScienceLabSearch; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Localizer=12, Vessel=4, Part=1; fan_in=3 fan_out=7
- methods(1): PostErrorToScreen
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ScienceNodeTutorialStep.cs
- 138 lines; kind=class; type=ScienceNodeTutorialStep; bases=IntermediateTutorialPageStep; ancestry=IntermediateTutorialPageStep/TutorialPageConfig; ns=(global)
- usings: Expansions.Missions, Expansions.Missions.Editor, UnityEngine
- unity: GameObject=6; unity_score=26
- ksp refs: GameEvents=5, Orbit=1; fan_in=1 fan_out=14
- methods(13): AddTutorialStepConfig, FilterScienceNodeCriteria, IsScienceLinked, OnEnterDragScienceEventNode, OnEnterDragScienceEventNodeHelper, OnEnterLinkScienceEventNode, OnLeaveDragScienceEventNodeHelper, OnLeaveOnEnterDragScienceEventNode, OnLeaveOnEnterLinkScienceEventNode, OnNodeEventScienceTuto, OnSelectDuringDragScienceTuto, OnUpdateDragScienceEventNodeHelper, OnUpdateLinkScienceEventNode
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/ScienceSubject.cs
- 174 lines; kind=class; type=ScienceSubject; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: KSP.Localization
- unity: none; unity_score=2
- ksp refs: ScienceSubject=6, Localizer=4, ConfigNode=3, CelestialBody=3; fan_in=24 fan_out=7
- methods(5): HasPartialIDstring, IsFromBody, IsFromSituation, Load, Save
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ScienceUtil.cs
- 574 lines; kind=class; type=ScienceUtil; bases=-; ancestry=-; ns=(global)
- usings: CommNet, Experience, Experience.Effects, KSP.Localization, UnityEngine
- unity: none; unity_score=4
- ksp refs: Localizer=45, Vessel=10, CelestialBody=10, FlightGlobals=9, ProtoCrewMember=8, ResearchAndDevelopment=4; fan_in=24 fan_out=18
- methods(18): BiomeIsUnlisted, CrewListHasEffect, CrewListHasType, GenerateLocalizedTitle, GenerateScienceSubjectRecoveryTitle, GenerateScienceSubjectTitle, GetBestTransmitter, GetBiomedisplayName, GetExperimentBiome, GetExperimentBiomeLocalized, GetExperimentBodyName, GetExperimentFieldsFromScienceID, GetExperimentSituation, GetLabScore, GetTransmitterScore, RequiredUsageExternalAvailable, RequiredUsageInternalAvailable, generateRecoveryLocalizedTitle
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ScienceWidget.cs
- 38 lines; kind=MonoBehaviour; type=ScienceWidget; bases=CurrencyWidget; ancestry=CurrencyWidget/MonoBehaviour; ns=(global)
- usings: TMPro
- unity: Lifecycle=2, TMPro=1; unity_score=57
- ksp refs: ResearchAndDevelopment=3, GameEvents=2, HighLogic=1; fan_in=1 fan_out=7
- methods(5): Awake, DelayedStart, OnAboutToStart, OnDestroy, onScienceChange
- auto suggestion: tier=B subsystem=Science

## Assembly-CSharp/Scientist.cs
- 79 lines; kind=MonoBehaviour; type=Scientist; bases=SpaceCenterCrew; ancestry=SpaceCenterCrew/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2; unity_score=35
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): SetAnimation, Start, Update
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/ScreenMessage.cs
- 30 lines; kind=class; type=ScreenMessage; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=17
- ksp refs: none; fan_in=20 fan_out=3
- methods(0): 
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/ScreenMessages.cs
- 451 lines; kind=MonoBehaviour; type=ScreenMessages; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=16, Time.=8, Transform=6, GameObject=6, SetActive=5, Screen/Cursor=4; unity_score=66
- ksp refs: GameEvents=4, Localizer=3, GameSettings=2; fan_in=122 fan_out=9
- methods(14): Awake, CreateTextInstance, DestroyMessage, DestroyMessages, OnDestroy, OnGameSceneLoadRequested, OnLevelLoaded, PostMessage, PostScreenMessage, RemoveMessage, SendStateMessage, SetParent, Start, Update
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/ScreenMessagesText.cs
- 12 lines; kind=MonoBehaviour; type=ScreenMessagesText; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine
- unity: TMPro=1, MonoBehaviour=1; unity_score=55
- ksp refs: none; fan_in=2 fan_out=1
- methods(1): SetText
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/ScreenMessageStyle.cs
- 15 lines; kind=enum; type=ScreenMessageStyle; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=101 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/ScreenShot.cs
- 111 lines; kind=MonoBehaviour; type=ScreenShot; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Application.=12, Lifecycle=3, MonoBehaviour=2, Transform=1, Camera=1; unity_score=64
- ksp refs: FlightCamera=9, GameEvents=8, GameSettings=3, HighLogic=1; fan_in=0 fan_out=6
- methods(6): HideUI, OnDestroy, OnGameUnpause, ShowUI, Start, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ScreenSpaceObjectScaling.cs
- 41 lines; kind=MonoBehaviour; type=ScreenSpaceObjectScaling; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=4, Screen/Cursor=2, Lifecycle=2, Camera=2, Time.=1, MonoBehaviour=1; unity_score=85
- ksp refs: none; fan_in=2 fan_out=0
- methods(3): GetScreenSize, LateUpdate, Start
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ScrollRectDragOverride.cs
- 28 lines; kind=MonoBehaviour; type=ScrollRectDragOverride; bases=ScrollRect; ancestry=ScrollRect; ns=(global)
- usings: UnityEngine.EventSystems, UnityEngine.UI
- unity: EventSystems=3, UnityEngine.UI=1, Lifecycle=1; unity_score=80
- ksp refs: none; fan_in=0 fan_out=1
- methods(4): Awake, OnBeginDrag, OnDrag, OnEndDrag
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ScrollRectPixelPerfectHandler.cs
- 51 lines; kind=MonoBehaviour; type=ScrollRectPixelPerfectHandler; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1; unity_score=75
- ksp refs: none; fan_in=0 fan_out=1
- methods(3): DisablePixelPerfect, Start, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/SerenityAboutDialog.cs
- 80 lines; kind=MonoBehaviour; type=SerenityAboutDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, GameObject=3, Transform=2, Lifecycle=2, Instantiate/Destroy=2, Texture=1; unity_score=85
- ksp refs: InputLockManager=2, GameSettings=2; fan_in=1 fan_out=6
- methods(7): Create, Dismiss, OnCloseButton, OnDontShowAgainToggle, OnMoreInfoButton, Start, Update
- auto suggestion: tier=C subsystem=DLC-BreakingGround

## Assembly-CSharp/ShaderTimeOffset.cs
- 28 lines; kind=MonoBehaviour; type=ShaderTimeOffset; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Renderer=2, Material=2, Lifecycle=2, Time.=1, Shader=1, MonoBehaviour=1; unity_score=91
- ksp refs: none; fan_in=0 fan_out=0
- methods(3): Reset, Start, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ShipConstruct.cs **MUST-OPEN (>800 lines)**
- 971 lines; kind=class; type=ShipConstruct; bases=IEnumerable<Part>,IEnumerable,IShipconstruct; ancestry=IEnumerable; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, RUI.Algorithms, UnityEngine
- unity: Transform=11, Resources.=3, SetActive=1, Instantiate/Destroy=1, GameObject=1; unity_score=17
- ksp refs: Part=36, Vessel=25, Localizer=17, ConfigNode=16, FlightGlobals=10, ShipConstruct=7; fan_in=84 fan_out=36
- methods(18): Add, AddToConstruct, AreAllPartsConnected, Clear, Contains, GetConnectedResourceTotals, GetEnumerator, GetShipCosts, GetShipMass, GetTotalMass, LoadShip, Remove, RequestResource, RunVesselNamingUpdates, SaveShip, UpdateResourceSets, UpdateVesselNaming, isControllable
- auto suggestion: tier=B subsystem=Save

## Assembly-CSharp/ShipConstruction.cs **MUST-OPEN (>800 lines)**
- 1349 lines; kind=class; type=ShipConstruction; bases=-; ancestry=-; ns=(global)
- usings: Expansions, Expansions.Missions.Runtime, KSP.Localization, SaveUpgradePipeline, UnityEngine, UnityEngine.Networking
- unity: GameObject=26, Transform=19, Texture=19, GetComponent=9, Coroutine=6, LayerMask=5; unity_score=48
- ksp refs: ShipConstruct=34, Part=29, HighLogic=22, ConfigNode=21, PartLoader=19, Vessel=13; fan_in=65 fan_out=50
- methods(53): AllPartsFound, AssembleForLaunch, AutoGenerateThumbnail, CalculateCraftSize, CaptureStockThumbnail, CaptureThumbnail, CheckCraftFileType, CheckLaunchSiteClear, ClearBackups, CreateBackup, CreateConstructFromTemplate, CreateSubassemblyIcon, DebugBackup, FindCraftCenter, FindCraftMOI, FindPartWithCraftID, FindVesselsAtLaunchSite, FindVesselsLandedAt, GetCurrentGameShipsPathFor, GetPartCosts, GetPartCostsAndMass, GetPlayerCraftThumbnailName, GetRootCraftSavePath, GetSABounds, GetSavePath, GetShipsPathFor, GetShipsSubfolderFor, GetThumbnail, GetUniqueFlightID, IsCompatible, LoadShip, LoadSubassembly, LoadTemplate, LoadThumbnail, OnPipelineFailed, OnPipelineFinished, PutShipToGround, RecoverVesselFromFlight, RenderCamera, RestoreBackup, SanitizeCraftIDs, SanitizePartCosts, SaveShip, SaveShipToPath, SaveSubassembly ...
- auto suggestion: tier=B subsystem=Save

## Assembly-CSharp/ShipTemplate.cs
- 251 lines; kind=class; type=ShipTemplate; bases=-; ancestry=-; ns=(global)
- usings: Expansions, Expansions.Missions, Expansions.Missions.Runtime, UnityEngine
- unity: none; unity_score=7
- ksp refs: ConfigNode=13, MissionSystem=6, HighLogic=3, ResearchAndDevelopment=2, Game=2, PartLoader=1; fan_in=17 fan_out=11
- methods(5): GetPartId, GetPartName, GetShipSize, LoadShip, SetIfControllable
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SimpleOrbitController.cs
- 31 lines; kind=MonoBehaviour; type=SimpleOrbitController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, Lifecycle=2, Time.=1, MonoBehaviour=1; unity_score=74
- ksp refs: Vector3d=3, QuaternionD=2; fan_in=0 fan_out=2
- methods(2): FixedUpdate, Start
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/Simplex.cs
- 380 lines; kind=class; type=Simplex; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=1
- ksp refs: Vector3d=2; fan_in=15 fan_out=1
- methods(6): SetupPermTable, dot, fastfloor, noise, noiseNormalized, value
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/SimplexNoise.cs
- 354 lines; kind=static class; type=SimplexNoise; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=0
- ksp refs: Vector3d=2; fan_in=1 fan_out=1
- methods(4): SetupPermTable, dot, fastfloor, noise
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/SiteNode.cs
- 141 lines; kind=MonoBehaviour; type=SiteNode; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: FinePrint, KSP.Localization, KSP.UI.Screens.Mapview, UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1, GetComponent=1, GameObject=1; unity_score=41
- ksp refs: GameEvents=6, Vector3d=1, ScaledSpace=1, Localizer=1; fan_in=1 fan_out=14
- methods(13): Awake, LateUpdate, OnDestroy, OnMapEntered, OnMapExited, OnSceneLoaded, OnUpdateNodeCaption, OnUpdateNodeIcon, OnUpdateNodePosition, OnUpdateType, SetupMapNode, SetupWaypoint, Spawn
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/SkySphereControl.cs
- 57 lines; kind=MonoBehaviour; type=SkySphereControl; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=9, Transform=5, Material=3, Lifecycle=3, Renderer=1, MonoBehaviour=1; unity_score=96
- ksp refs: Vector3d=2, ScaledSpace=2, FlightGlobals=2, QuaternionD=1, Planetarium=1, MapView=1; fan_in=0 fan_out=7
- methods(3): FixedUpdate, Start, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/SliderFocusType.cs
- 6 lines; kind=enum; type=SliderFocusType; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: UnityEngine.UI=1; unity_score=29
- ksp refs: none; fan_in=7 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/SmokeTrailControl.cs
- 54 lines; kind=MonoBehaviour; type=SmokeTrailControl; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=2, Renderer=2, Material=2, Lifecycle=2, MonoBehaviour=1, GetComponent=1; unity_score=83
- ksp refs: FlightGlobals=2, HighLogic=1; fan_in=2 fan_out=3
- methods(2): Start, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/SmoothDisposer.cs
- 26 lines; kind=MonoBehaviour; type=SmoothDisposer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Smooth.Dispose, UnityEngine
- unity: Lifecycle=2, Instantiate/Destroy=2, MonoBehaviour=1; unity_score=74
- ksp refs: none; fan_in=1 fan_out=1
- methods(2): Awake, LateUpdate
- auto suggestion: tier=B subsystem=Core

## Assembly-CSharp/SpaceCenter.cs
- 86 lines; kind=MonoBehaviour; type=SpaceCenter; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=4, Lifecycle=3, GameObject=3, SetActive=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=74
- ksp refs: Vector3d=4, GameEvents=2, CelestialBody=2, Part=1; fan_in=36 fan_out=6
- methods(5): Awake, GreatCircleDistance, OnDestroy, OnGameSceneLoaded, Start
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/SpaceCenterBuilding.cs **MUST-OPEN (>800 lines)**
- 911 lines; kind=MonoBehaviour; type=SpaceCenterBuilding; bases=MonoBehaviour,ITooltipController; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, KSP.UI.TooltipTypes, UnityEngine, UnityEngine.EventSystems, Upgradeables
- unity: GetComponent=13, Transform=9, GameObject=7, Coroutine=7, Renderer=6, Material=6; unity_score=54
- ksp refs: Localizer=21, HighLogic=15, InputLockManager=14, GameEvents=14, ProtoVessel=11, Funding=6; fan_in=14 fan_out=39
- methods(55): ColliderHover, DemolishFacility, Despawntooltip, DowngradeFacility, EnterBuilding, FindVesselsAtFacility, FindVesselsAtGrounds, GetCollapseReputationHit, GetRepairsCost, GetStructureDamage, GetStructureDamageLevel, HighLightBuilding, IsFacilityOperational, IsOpen, LateUpdate, ObjectHasTerrainMaterials, OnClicked, OnContextMenuDismissed, OnContextMenuSpawn, OnDestroy, OnFacilityUpgraded, OnFacilityUpgrading, OnGameSceneChange, OnLeftClick, OnMouseDown, OnMouseTap, OnMouseUp, OnOnDestroy, OnRightClick, OnStart, OnStructureCollapsed, OnStructureCollapsing, OnStructureRepaired, OnStructureRepairing, OnTooltipAboutToDespawn, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipSpawned, OnTooltipUpdate, RendererHasTerrainMaterials, RepairFacility, RepairStructures, ResetStructures, SetHighlighted, SetInView ...
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/SpaceCenterBuildingCollider.cs
- 43 lines; kind=MonoBehaviour; type=SpaceCenterBuildingCollider; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Instantiate/Destroy=2, GameObject=2, MonoBehaviour=1, Lifecycle=1; unity_score=68
- ksp refs: GameEvents=3; fan_in=1 fan_out=3
- methods(5): OnDestroy, OnGameSceneChange, OnMouseExit, OnMouseOver, Setup
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/SpaceCenterCamera.cs
- 339 lines; kind=MonoBehaviour; type=SpaceCenterCamera; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Input.=34, KeyCode=29, Transform=13, Time.=7, GameObject=3, Screen/Cursor=2; unity_score=89
- ksp refs: Vector3d=7, QuaternionD=5, PQS=5, FlightCamera=3; fan_in=0 fan_out=4
- methods(19): Altitude, Elevate, GetSpeed, InputCamera, InputMovement, InputSpeedFactor, Pitch, Reset, ResetCamera, Rotate, SetPosition, Start, SwitchMode, Translate, Update, UpdateTransform, UpdateTransformOverview, UpdateTransformRover, Zoom
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/SpaceCenterCamera2.cs
- 440 lines; kind=MonoBehaviour; type=SpaceCenterCamera2; bases=MonoBehaviour,IKSPCamera; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.EventSystems
- unity: Input.=27, KeyCode=23, Transform=16, Time.=6, GameObject=5, Lifecycle=3; unity_score=83
- ksp refs: PQS=5, GameSettings=5, FlightCamera=5, InputLockManager=2, GameEvents=2, FlightGlobals=2; fan_in=1 fan_out=16
- methods(30): Altitude, Elevate, GetCameraTransform, GetPivot, GetSpeed, GetZoom, InputCamera, InputMovement, InputSpeedFactor, OnDestroy, OnNavigatorHandoff, OnNavigatorRequestControl, OnNavigatorTakeOver, OnSceneSwitch, Pitch, Reset, ResetCamera, Rotate, SetCamCoordsFromPosition, Start, SwitchMode, Translate, Update, UpdateTransform, UpdateTransformOverview, UpdateTransformRover, Zoom, getPitch, getReferenceFrame, getYaw
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/SpaceCenterCrew.cs
- 75 lines; kind=MonoBehaviour; type=SpaceCenterCrew; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=4, Rigidbody=1, MonoBehaviour=1, GameObject=1, Animation=1; unity_score=66
- ksp refs: none; fan_in=9 fan_out=3
- methods(2): CrewMovement, SetAnimation
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/SpaceCenterFacility.cs
- 23 lines; kind=enum; type=SpaceCenterFacility; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ResearchAndDevelopment=1; fan_in=69 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Scene

## Assembly-CSharp/SpaceCenterMain.cs
- 89 lines; kind=MonoBehaviour; type=SpaceCenterMain; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, UnityEngine
- unity: GameObject=9, SetActive=2, MonoBehaviour=1, Lifecycle=1, Coroutine=1; unity_score=72
- ksp refs: InputLockManager=6, GameEvents=4, HighLogic=3, Planetarium=1, Game=1; fan_in=0 fan_out=11
- methods(5): OnApplicationFocus, OnDestroy, OnGameSceneChange, Start, onInputLockModified
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/SpaceCenterSurfaceObject.cs
- 57 lines; kind=MonoBehaviour; type=SpaceCenterSurfaceObject; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, GameObject=2, MonoBehaviour=1, Coroutine=1; unity_score=61
- ksp refs: GameEvents=4, HighLogic=2, CelestialBody=2; fan_in=0 fan_out=7
- methods(5): Awake, OnDestroy, OnGameSceneLoadRequested, OnGameSceneLoaded, Setup
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/SpaceNavigatorCamera.cs
- 163 lines; kind=MonoBehaviour; type=SpaceNavigatorCamera; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.SceneManagement
- unity: GameObject=4, SceneManager=3, Lifecycle=3, Raycast/Query=2, Transform=1, Physics.=1; unity_score=67
- ksp refs: GameSettings=4, HighLogic=2; fan_in=2 fan_out=9
- methods(11): LateUpdate, OnCameraRequestControl, OnCameraUpdate, OnCameraWantsControl, OnDestroy, OnGetControl, OnLevelLoaded, OnSceneLoaded, Start, clampToRadius, getNextTranslation
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/SpaceNavigatorFreeCamera.cs
- 68 lines; kind=MonoBehaviour; type=SpaceNavigatorFreeCamera; bases=SpaceNavigatorCamera; ancestry=SpaceNavigatorCamera/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Time.=8, Transform=3; unity_score=66
- ksp refs: none; fan_in=0 fan_out=2
- methods(3): OnCameraUpdate, OnCameraWantsControl, OnGetControl
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/SpaceNavigatorLocalCamera.cs
- 74 lines; kind=MonoBehaviour; type=SpaceNavigatorLocalCamera; bases=SpaceNavigatorCamera; ancestry=SpaceNavigatorCamera/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Time.=7, Transform=1; unity_score=59
- ksp refs: none; fan_in=0 fan_out=2
- methods(4): OnCameraUpdate, OnCameraWantsControl, OnGetControl, clampToBounds
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/SpaceObjectCollider.cs **MUST-OPEN (>800 lines)**
- 931 lines; kind=MonoBehaviour; type=SpaceObjectCollider; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=7, Mesh=6, GameObject=6, Coroutine=5, Time.=2, Collider=2; unity_score=48
- ksp refs: none; fan_in=2 fan_out=3
- methods(37): AddChunk, BuildCompositeMesh, Clear, ClearNeighbours, ContainsChunk, CreateChunks, CreateCollider, DrawSolid, GenCoroutine, Generate, GetNeighbour, GetSlope, GetSrfCenter, GetSrfRadial, GetTri, GetTriIndex, LateUpdate, LinkChunks, MarchEdge, MarchNext, RandomColor, RemoveChunk, SetNeighbour, SetTri, Setup, TestEdge, TestWedge, UpdateGenRange, find_fix_shared_edge, fix_multi_shared_edges, fix_wind_errors, flip_winding_order, get_vertex, set_vertex, share_edge, shared_edges, winding_conflict
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/SpacePlaneHangarBuilding.cs
- 52 lines; kind=MonoBehaviour; type=SpacePlaneHangarBuilding; bases=SpaceCenterBuilding; ancestry=SpaceCenterBuilding/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Runtime, KSP.Localization, UnityEngine
- unity: MonoBehaviour=1; unity_score=31
- ksp refs: HighLogic=8, MissionSystem=3, Localizer=3, Game=3, InputLockManager=2; fan_in=0 fan_out=16
- methods(3): IsOpen, OnClicked, OnPopupWarningDismiss
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/SPHCamera.cs
- 315 lines; kind=MonoBehaviour; type=SPHCamera; bases=MonoBehaviour,IKSPCamera; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI.Screens, UnityEngine, UnityEngine.EventSystems
- unity: Transform=25, Input.=15, Time.=11, KeyCode=10, Lifecycle=3, GameObject=2; unity_score=85
- ksp refs: GameSettings=22, InputLockManager=4, GameEvents=4, EditorLogic=3, ShipConstruct=1; fan_in=1 fan_out=13
- methods(18): GetCameraTransform, GetPivot, OnDestroy, OnEditorRestart, OnEditorShipLoad, OnEnable, OnNavigatorHandoff, OnNavigatorRequestControl, OnNavigatorTakeOver, PlaceCamera, ResetCamera, SetCamCoordsFromPosition, SetInitialCameraPosition, Start, Update, getPitch, getReferenceFrame, getYaw
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/SphereBaseSO.cs
- 396 lines; kind=ScriptableObject; type=SphereBaseSO; bases=ScriptableObject; ancestry=ScriptableObject; ns=(global)
- usings: UnityEngine
- unity: Mesh=3, Time.=2, ScriptableObject=1; unity_score=22
- ksp refs: none; fan_in=2 fan_out=1
- methods(13): AddVertex, CalculateNormals, CalculateSize, Create, CreateMesh, CreateSphere, GetMidPoint, HasVertex, LinkEdges, LinkSphere, LinkTriangle, LinkTriangles, LinkVerts
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/SphereVolume.cs
- 76 lines; kind=MonoBehaviour; type=SphereVolume; bases=VolumetricObjectBase; ancestry=VolumetricObjectBase/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=8, Lifecycle=1; unity_score=74
- ksp refs: none; fan_in=1 fan_out=1
- methods(5): HasChanged, OnEnable, PopulateShaderName, SetChangedValues, UpdateVolume
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/Splashdown.cs
- 34 lines; kind=MonoBehaviour; type=Splashdown; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, Instantiate/Destroy=2, GameObject=2, SendMessage/Invoke=1, MonoBehaviour=1, GetComponent=1; unity_score=86
- ksp refs: GameSettings=1; fan_in=0 fan_out=1
- methods(3): Awake, Start, endSplashBurst
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/StackIconGrouping.cs
- 8 lines; kind=enum; type=StackIconGrouping; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/StageGroupInfoItem.cs
- 73 lines; kind=MonoBehaviour; type=StageGroupInfoItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=5, TMPro=2, GetComponent=2, MonoBehaviour=1, Lifecycle=1; unity_score=76
- ksp refs: GameSettings=2, HighLogic=1; fan_in=1 fan_out=6
- methods(4): Awake, OnUpdate, Setup, UpdateValue
- auto suggestion: tier=C subsystem=Vessel

## Assembly-CSharp/StandaloneInputModuleCustom.cs
- 9 lines; kind=MonoBehaviour; type=StandaloneInputModuleCustom; bases=StandaloneInputModule; ancestry=StandaloneInputModule; ns=(global)
- usings: UnityEngine.EventSystems
- unity: EventSystems=1; unity_score=44
- ksp refs: none; fan_in=1 fan_out=0
- methods(1): GetLastPointerEventDataPublic
- auto suggestion: tier=B subsystem=Input

## Assembly-CSharp/SteamManager.cs **MUST-OPEN (>800 lines)**
- 804 lines; kind=MonoBehaviour; type=SteamManager; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, Steamworks, TMPro, UnityEngine
- unity: Steam=52, TMPro=4, Lifecycle=4, Texture=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=68
- ksp refs: Localizer=5; fan_in=15 fan_out=5
- methods(43): AddAppDependency, Awake, CheckCloudQuota, CreateNewItem, GetAppDependencies, GetItemState, GetNumberSubscribedItems, GetSubscribedItems, GetUGCFailureReason, GetUGCItem, GetUGCItemDetails, OnDestroy, OnEnable, OnGetAppDependencies, OnItemAppDependenciesUpdated, OnItemDeleted, OnItemUpdated, OnNewItemCreated, OnPersonaStateChanged, OnRemoteFileSubscribed, OnRemoteFileUnsubscribed, OnRemoveAppDependency, OnSubscribeFile, OnUGCQuery, OnUnSubscribeFile, OpenSteamOverlayToWorkshopItem, QueryUGCItems, QueryUserUGCItems, RegisterRemoteSubUnsubEvents, RemoveAppDependency, RemoveRemoteCallbacks, RemoveRemoteSubUnsubEvents, SetupCallbacks, SetupRemoteCallbacks, SteamAPIDebugTextHook, Update, UpdateItem, checkWorkshopItemPreviewSize, deleteItem, getFriendName, getUserName, subscribeItem, unsubscribeItem
- auto suggestion: tier=C subsystem=Steam

## Assembly-CSharp/SteamWorkshopExportDialog.cs
- 324 lines; kind=MonoBehaviour; type=SteamWorkshopExportDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, KSP.Localization, KSP.UI, KSP.UI.Screens, TMPro, UnityEngine, UnityEngine.Events, UnityEngine.UI
- unity: UnityEngine.UI=9, TMPro=9, SetActive=9, GameObject=9, Transform=4, Lifecycle=2; unity_score=78
- ksp refs: GameDatabase=2, Localizer=1; fan_in=2 fan_out=9
- methods(10): CloseDialog, OnCancel, OnConfirm, OnDestroy, OnModsList, OnToggle, Spawn, Start, ToggleSetup, onBtnLegalAgreement
- auto suggestion: tier=C subsystem=Steam

## Assembly-CSharp/SteeringObject.cs
- 86 lines; kind=class; type=SteeringObject; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Transform=3; unity_score=33
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): FindAlignmentAxis
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/StoredPart.cs
- 99 lines; kind=class; type=StoredPart; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=4
- ksp refs: ConfigNode=5, ProtoPartSnapshot=3; fan_in=6 fan_out=2
- methods(3): Copy, Load, Save
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/StringBuilderCache.cs
- 49 lines; kind=static class; type=StringBuilderCache; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=29 fan_out=0
- methods(4): Acquire, Format, Release, ToStringAndRelease
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/StringExtensions.cs
- 18 lines; kind=static class; type=StringExtensions; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): GetHashCode_Net35
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/StringTypeParser.cs
- 16 lines; kind=class; type=StringTypeParser; bases=ITypeParser; ancestry=ITypeParser; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(2): Convert, Parse
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/SuitButton.cs
- 182 lines; kind=MonoBehaviour; type=SuitButton; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=7, Transform=4, EventSystems=4, SetActive=3, Lifecycle=3, GameObject=3; unity_score=80
- ksp refs: GameEvents=5, ProtoCrewMember=3; fan_in=4 fan_out=7
- methods(12): GenerateButtonName, OnButtonClicked, OnDestroy, OnHelmetNeckringSelection, OnPointerEnter, OnPointerExit, OpenSuitLightPicker, Reset, RotatePreview, Select, Start, Update
- auto suggestion: tier=C subsystem=Kerbals

## Assembly-CSharp/SuitCombo.cs
- 43 lines; kind=class; type=SuitCombo; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Texture=2; unity_score=39
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/SuitCombos.cs
- 203 lines; kind=MonoBehaviour; type=SuitCombos; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, UnityEngine
- unity: UnityEngine.UI=2, Transform=2, Screen/Cursor=2, Material=2, Lifecycle=2, Texture=1; unity_score=62
- ksp refs: ConfigNode=5, GameSettings=2, GameEvents=2, ProtoCrewMember=1, GameDatabase=1; fan_in=4 fan_out=8
- methods(8): CreateHelmetSuitPickerWindow, GetCombo, GetDefaultTexture, GetSuitCombos, OnDestroy, Start, UpdateTextureScale, ValidateCombo
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/SuitLightColorPicker.cs
- 64 lines; kind=MonoBehaviour; type=SuitLightColorPicker; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=9, GetComponent=3, Transform=2, Shader=2, Lifecycle=2, GameObject=2; unity_score=94
- ksp refs: ProtoCrewMember=1; fan_in=3 fan_out=3
- methods(4): OnColorChange, OnDestroy, OnPickColorButton, Start
- auto suggestion: tier=C subsystem=Kerbals

## Assembly-CSharp/Sun.cs
- 324 lines; kind=MonoBehaviour; type=Sun; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: QualitySettings/SystemInfo=18, Transform=7, Light=6, Lifecycle=4, GetComponent=2, MonoBehaviour=1; unity_score=63
- ksp refs: Vector3d=14, CelestialBody=5, ScaledSpace=4, FlightGlobals=4, GameSettings=2, GameEvents=2; fan_in=57 fan_out=13
- methods(7): Awake, GameSceneLoadRequested, GetLocalTimeAtPosition, LateUpdate, OnDestroy, Start, SunlightEnabled
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/SunCoronas.cs
- 45 lines; kind=MonoBehaviour; type=SunCoronas; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=6, Lifecycle=2, Time.=1, MonoBehaviour=1, Camera=1; unity_score=84
- ksp refs: none; fan_in=3 fan_out=0
- methods(2): FixedUpdate, Start
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/SunFlare.cs
- 73 lines; kind=MonoBehaviour; type=SunFlare; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=6, Lifecycle=3, GetComponent=3, Collider=2, Renderer=1, MonoBehaviour=1; unity_score=81
- ksp refs: Vector3d=6, ScaledSpace=3, HighLogic=2, CelestialBody=2; fan_in=4 fan_out=7
- methods(4): Awake, LateUpdate, OnDestroy, SunlightEnabled
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/SunShaderController.cs
- 113 lines; kind=MonoBehaviour; type=SunShaderController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: AnimationCurve=20, Shader=13, Material=5, Texture=3, Renderer=3, Lifecycle=2; unity_score=94
- ksp refs: Planetarium=2; fan_in=0 fan_out=1
- methods(4): OnDestroy, Start, Update, UpdateRampMap
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/SurfaceFX.cs
- 274 lines; kind=MonoBehaviour; type=SurfaceFX; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=17, Lifecycle=4, Transform=2, Time.=2, GameObject=2, MonoBehaviour=1; unity_score=70
- ksp refs: FloatingOrigin=6, FlightGlobals=1; fan_in=1 fan_out=3
- methods(10): AddSource, Awake, FindNearestFX, GetWeightedAvgVector, LateUpdate, OnDestroy, RemoveSource, Start, Terminate, UpdateParticle
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/SurfaceObject.cs
- 206 lines; kind=MonoBehaviour; type=SurfaceObject; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=25, GameObject=5, GetComponent=4, Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=77
- ksp refs: CelestialBody=5, Vector3d=2; fan_in=3 fan_out=3
- methods(9): Create, FixedUpdate, LateUpdate, PopToSceneRoot, ReturnToParent, Start, Terminate, Update, onUpdate
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/SuspensionLoadBalancer.cs
- 86 lines; kind=VesselModule; type=SuspensionLoadBalancer; bases=VesselModule; ancestry=VesselModule/MonoBehaviour; ns=(global)
- usings: ModuleWheels, UnityEngine
- unity: Lifecycle=2; unity_score=32
- ksp refs: GameEvents=2, VesselModule=1, Vessel=1; fan_in=1 fan_out=4
- methods(6): FixedUpdate, OnAwake, OnDestroy, OnStart, UpdateSuspensionModules, VesselChanged
- auto suggestion: tier=A subsystem=PartModules-Wheels

## Assembly-CSharp/SVBoxSlider.cs
- 103 lines; kind=MonoBehaviour; type=SVBoxSlider; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=5, Lifecycle=4, Texture=2, Instantiate/Destroy=2, GetComponent=2, Transform=1; unity_score=77
- ksp refs: none; fan_in=0 fan_out=4
- methods(7): Awake, HSVChanged, OnDestroy, OnDisable, OnEnable, RegenerateSVTexture, SliderChanged
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/SymmetryMethod.cs
- 5 lines; kind=enum; type=SymmetryMethod; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=12 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/SystemInformation.cs
- 17 lines; kind=MonoBehaviour; type=SystemInformation; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Renderer=1, QualitySettings/SystemInfo=1, MonoBehaviour=1, Lifecycle=1; unity_score=67
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): Awake
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/Targeting.cs **MUST-OPEN (>800 lines)**
- 1112 lines; kind=class; type=Targeting; bases=-; ancestry=-; ns=(global)
- usings: Smooth.Pools
- unity: Raycast/Query=2; unity_score=3
- ksp refs: Vector3d=100, Orbit=1; fan_in=1 fan_out=4
- methods(38): Borrow, ClosestPoint, Create, Curvature, FindRoot, GetFrame, GetInfo, GetTgt, Intercepts, Intersept, Link, Normal, Point, Project, Radius, Release, ReleaseAllBorrowed, ReleaseContent, Reset, Set, Subdivide, Tangent, ToString, TrueAnomaly, add_crossing_subdivisions, add_interval, check_void_transition, find_region_edge, find_search_intervals, get_normal_intercept_frames, get_normal_intercepts, point_inside, point_is_real, sample_orbits, sample_v, subdivide_intervals, tangent_xy, true_anomaly_xy
- auto suggestion: tier=B subsystem=Orbit

## Assembly-CSharp/TechPerk.cs
- 21 lines; kind=class; type=TechPerk; bases=ITechPerk; ancestry=ITechPerk; ns=(global)
- usings: UnityEngine
- unity: Texture=1; unity_score=29
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): GetPerkState, SetPerk
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/TemperatureModel.cs
- 326 lines; kind=class; type=TemperatureModel; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(9): AddConstantStep, AddInitialConstantStep, AddInitialLinearStep, AddInitialLinearStepEquation, AddInitialQuadraticStep, AddLinearStep, AddQuadraticStep, clearIntervals, getTemperatureAtAltitude
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/test_particles.cs
- 223 lines; kind=MonoBehaviour; type=test_particles; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: Input.=16, ParticleSystem=14, KeyCode=13, Time.=8, Lifecycle=3, Transform=2; unity_score=89
- ksp refs: none; fan_in=0 fan_out=0
- methods(5): LateUpdate, MouseDrag, SetPower, Start, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/TestVehicleController.cs
- 215 lines; kind=MonoBehaviour; type=TestVehicleController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Joint=5, Rigidbody=4, Transform=2, Lifecycle=2, MonoBehaviour=1; unity_score=68
- ksp refs: none; fan_in=0 fan_out=4
- methods(8): BoostCurve, ConfigurableJointUpdate, DistanceFromCoM, FixedUpdate, Start, SuspensionLoadUpdate, SuspensionSpringUpdate, updateMassDistribution
- auto suggestion: tier=B subsystem=PartModules-Wheels

## Assembly-CSharp/TextButton3D.cs
- 141 lines; kind=MonoBehaviour; type=TextButton3D; bases=MonoBehaviour,IMouseEvents; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Material=6, Renderer=3, MonoBehaviour=2, GetComponent=2, Coroutine=2, Time.=1; unity_score=70
- ksp refs: none; fan_in=0 fan_out=1
- methods(9): GetInstance, Lock, OnMouseDown, OnMouseDrag, OnMouseEnter, OnMouseExit, OnMouseTap, OnMouseUp, Unlock
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/TextMeshQueue.cs
- 126 lines; kind=MonoBehaviour; type=TextMeshQueue; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine
- unity: TMPro=2, Instantiate/Destroy=2, UnityEngine.UI=1, Transform=1, MonoBehaviour=1, GameObject=1; unity_score=56
- ksp refs: none; fan_in=3 fan_out=1
- methods(4): AddLine, DestroyIfEmpty, RemoveAllLines, RemoveLine
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/TextProButton3D.cs
- 189 lines; kind=MonoBehaviour; type=TextProButton3D; bases=MonoBehaviour,IMouseEvents; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine
- unity: TMPro=4, Collider=3, MonoBehaviour=2, GetComponent=2, Coroutine=2, Time.=1; unity_score=59
- ksp refs: none; fan_in=1 fan_out=2
- methods(15): Awake, GetInstance, Highlight, IsButtonLocked, KeyStrokeLock, Lock, OnMouseDown, OnMouseDrag, OnMouseEnter, OnMouseExit, OnMouseTap, OnMouseUp, SetEnable, UnHighlight, Unlock
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/TextureMaterialProperty.cs
- 17 lines; kind=class; type=TextureMaterialProperty; bases=MaterialPropertyExtensions.MaterialProperty; ancestry=MaterialProperty; ns=(global)
- usings: UnityEngine
- unity: Texture=5, Shader=1, Material=1; unity_score=74
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): Apply
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/TextureScale.cs
- 92 lines; kind=class; type=TextureScale; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Texture=3; unity_score=33
- ksp refs: none; fan_in=2 fan_out=1
- methods(6): Bilinear, BilinearScale, ColorLerpUnclamped, Point, PointScale, Scale
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/TextureTools.cs
- 163 lines; kind=MonoBehaviour; type=TextureTools; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: AnimationCurve=14, Texture=9, MonoBehaviour=1; unity_score=69
- ksp refs: none; fan_in=0 fan_out=2
- methods(5): Add, Convert, EvaluateColor, EvaluateVector, Reset
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/TGAHeader.cs
- 95 lines; kind=class; type=TGAHeader; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=4
- ksp refs: none; fan_in=1 fan_out=1
- methods(1): GetData
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/TGAImage.cs
- 253 lines; kind=class; type=TGAImage; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Texture=7; unity_score=33
- ksp refs: none; fan_in=1 fan_out=3
- methods(6): CreateTexture, ReadImage, ReadRTETrueColorImage, ReadRTETrueColorImageAlpha, ReadRTETrueColorImageNoAlpha, ReadTrueColorImage
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/TGAImageType.cs
- 11 lines; kind=enum; type=TGAImageType; bases=byte; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ThermalLink.cs
- 60 lines; kind=class; type=ThermalLink; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=3
- ksp refs: Part=3, AttachNode=3; fan_in=2 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/TiltWindow.cs
- 29 lines; kind=MonoBehaviour; type=TiltWindow; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, Screen/Cursor=2, Lifecycle=2, Time.=1, MonoBehaviour=1, Input.=1; unity_score=86
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Start, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/TimedObjectDestructor.cs
- 22 lines; kind=MonoBehaviour; type=TimedObjectDestructor; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=1, SendMessage/Invoke=1, MonoBehaviour=1, Lifecycle=1, GameObject=1; unity_score=72
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Awake, DestroyNow
- auto suggestion: tier=B subsystem=Core

## Assembly-CSharp/TimeInputFieldHandler.cs
- 108 lines; kind=class; type=TimeInputFieldHandler; bases=InputFieldHandler; ancestry=InputFieldHandler; ns=(global)
- usings: TMPro, UnityEngine
- unity: TMPro=5; unity_score=36
- ksp refs: none; fan_in=1 fan_out=4
- methods(4): ChangeInputMode, GetFormattedValue, ParseText, ParseUT
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/TimeOfDayAnimation.cs
- 164 lines; kind=MonoBehaviour; type=TimeOfDayAnimation; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, Upgradeables
- unity: Shader=5, Renderer=5, Material=5, Transform=3, Lifecycle=3, MonoBehaviour=1; unity_score=76
- ksp refs: GameEvents=8; fan_in=0 fan_out=4
- methods(9): LateUpdate, MaterialPropertiesDirty, OnDestroy, SetColor, Start, UpdateInstances, UpdateRenderers, onDestructibleEvent, onFacilityEvent
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/TimeSinceTutorialStep.cs
- 277 lines; kind=class; type=TimeSinceTutorialStep; bases=IntermediateTutorialPageStep; ancestry=IntermediateTutorialPageStep/TutorialPageConfig; ns=(global)
- usings: Expansions.Missions, Expansions.Missions.Editor, UnityEngine
- unity: GameObject=10; unity_score=22
- ksp refs: GameEvents=4; fan_in=1 fan_out=17
- methods(21): AddTutorialStepConfig, CanHighLightStartNode, CanHighLightTime, CanHighlightOperator, FilterTimeNodeCriteria, HasTimeSinceNode, IsTimeNodeSetted, OnEnterDragTimeSinceNode, OnEnterDragTimeSinceNodeHelper, OnEnterSetupTimeNode, OnEnterSetupTimeNodeHelper, OnLeaveDragTimeSinceNode, OnLeaveDragTimeSinceNodeHelper, OnLeaveSetupTimeNode, OnLeaveSetupTimeNodeHelper, OnNodeFocused, OnSelectedGameObjectChange, OnUpdateSetupTimeNode, OnUpdateSetupTimeNodeHelper, TimeEqualOneDay, TimeStartNodeSetupped
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/TimeWarp.cs **MUST-OPEN (>800 lines)**
- 865 lines; kind=MonoBehaviour; type=TimeWarp; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Time.=12, Coroutine=7, Lifecycle=5, Transform=3, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=36
- ksp refs: Localizer=19, Planetarium=18, GameSettings=18, FlightGlobals=18, HighLogic=14, InputLockManager=11; fan_in=85 fan_out=25
- methods(26): Awake, CacheLocalStrings, CancelAutoWarp, ClampRateToOrbitTransitions, FixedUpdate, GetAltitudeLimit, GetMaxRateForAltitude, OnDestroy, OnPWarpWarningDismiss, PostScreenMessage, SetRate, ShowPWarpWarning, Start, Update, WarpTo, assumeWarpRate, autoWarpTo, btnSetHighRate, btnSetLowRate, getMaxOnRailsRateIdx, getMaxWarpRateForTravel, getRateChangeTravel, getWarpAccel, setMode, setRate, updateRate
- auto suggestion: tier=B subsystem=Orbit

## Assembly-CSharp/Timing0.cs
- 56 lines; kind=MonoBehaviour; type=Timing0; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/Timing1.cs
- 56 lines; kind=MonoBehaviour; type=Timing1; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/Timing2.cs
- 56 lines; kind=MonoBehaviour; type=Timing2; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/Timing3.cs
- 56 lines; kind=MonoBehaviour; type=Timing3; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/Timing4.cs
- 56 lines; kind=MonoBehaviour; type=Timing4; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/Timing5.cs
- 56 lines; kind=MonoBehaviour; type=Timing5; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/TimingFI.cs
- 56 lines; kind=MonoBehaviour; type=TimingFI; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/TimingManager.cs
- 541 lines; kind=MonoBehaviour; type=TimingManager; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Instantiate/Destroy=9, GameObject=9, GetComponent=8, Lifecycle=5, MonoBehaviour=1; unity_score=47
- ksp refs: FlightIntegrator=7; fan_in=10 fan_out=9
- methods(12): Awake, FixedUpdate, FixedUpdateAdd, FixedUpdateRemove, LateUpdate, LateUpdateAdd, LateUpdateRemove, OnDestroy, Update, UpdateAction, UpdateAdd, UpdateRemove
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/TimingPre.cs
- 56 lines; kind=MonoBehaviour; type=TimingPre; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/TMPProUtil.cs
- 83 lines; kind=static class; type=TMPProUtil; bases=-; ancestry=-; ns=(global)
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=4, UnityEngine.UI=3, GetComponent=1, GameObject=1; unity_score=55
- ksp refs: none; fan_in=1 fan_out=3
- methods(4): FontStyle, GetComponentByInstanceID, LineType, TextAlignment
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/TMProUtil.cs
- 10 lines; kind=class; type=TMProUtil; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/TrackingStationBuilding.cs
- 32 lines; kind=MonoBehaviour; type=TrackingStationBuilding; bases=SpaceCenterBuilding; ancestry=SpaceCenterBuilding/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=21
- ksp refs: HighLogic=7, Localizer=3, InputLockManager=2, Game=2; fan_in=0 fan_out=13
- methods(3): IsOpen, OnClicked, OnPopupWarningDismiss
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/TrackingStationDishMovement.cs
- 27 lines; kind=MonoBehaviour; type=TrackingStationDishMovement; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, Time.=1, MonoBehaviour=1, Lifecycle=1; unity_score=72
- ksp refs: Planetarium=2; fan_in=0 fan_out=1
- methods(1): Update
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/TrackIR.cs
- 353 lines; kind=MonoBehaviour; type=TrackIR; bases=MonoBehaviour,IConfigNode; ancestry=MonoBehaviour; ns=(global)
- usings: KSPTrackIR, UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1; unity_score=28
- ksp refs: ConfigNode=8, GameSettings=4, GameEvents=4; fan_in=8 fan_out=5
- methods(12): Awake, FindInstance, GetAxis, Init, Load, OnDestroy, OnSceneLoaded, OnSettingsApplied, Save, Shutdown, TrackIRUpdate, Update
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/TrackRigObject.cs
- 87 lines; kind=MonoBehaviour; type=TrackRigObject; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=4, Transform=3, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=1 fan_out=0
- methods(6): Awake, FixedUpdate, Initialize, LateUpdate, Track, Update
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/Trajectory.cs
- 436 lines; kind=class; type=Trajectory; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=1
- ksp refs: Vector3d=50, CelestialBody=33, Orbit=8, Planetarium=1; fan_in=1 fan_out=5
- methods(36): ConvertPointToLerped, ConvertPointToLocal, ConvertPointToLocalAtUT, ConvertPointToRelative, GetApoapsisLerped, GetApoapsisLocal, GetApoapsisLocalAtUT, GetApoapsisRelative, GetColors, GetPatchEndLerped, GetPatchEndLocal, GetPatchEndLocalAtUT, GetPatchEndRelative, GetPatchStartLerped, GetPatchStartLocal, GetPatchStartLocalAtUT, GetPatchStartRelative, GetPeriapsisLerped, GetPeriapsisLocal, GetPeriapsisLocalAtUT, GetPeriapsisRelative, GetPoints, GetPointsLerped, GetPointsLocal, GetPointsLocalAtUT, GetPointsRelative, GetRefBodyPosLerped, GetRefBodyPosLocal, GetRefBodyPosLocalAtUT, GetRefBodyPosRelative, GetTimes, ReframeToLerped, ReframeToLocal, ReframeToLocalAtUT, ReframeToRelative, UpdateFromOrbit
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/TransactionReasons.cs
- 33 lines; kind=enum; type=TransactionReasons; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=37 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/TransferDataBase.cs
- 63 lines; kind=class; type=TransferDataBase; bases=AppUI_Data; ancestry=AppUI_Data/IConfigNode; ns=(global)
- usings: KSP.UI, KSP.UI.Screens
- unity: none; unity_score=6
- ksp refs: Vessel=1, Orbit=1; fan_in=5 fan_out=5
- methods(3): GetAlarmDescription, GetAlarmTitle, IsAnyDropdownOpen
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/TransferDataSimple.cs
- 201 lines; kind=class; type=TransferDataSimple; bases=TransferDataBase; ancestry=TransferDataBase/AppUI_Data/IConfigNode; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, UnityEngine
- unity: none; unity_score=6
- ksp refs: Vector3d=6, CelestialBody=5, Localizer=4, FlightGlobals=1; fan_in=3 fan_out=13
- methods(3): GetAlarmDescription, GetAlarmTitle, IsAnyDropdownOpen
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/TransferDataSimpleTopData.cs
- 24 lines; kind=class; type=TransferDataSimpleTopData; bases=TransferDataTopDataBase; ancestry=TransferDataTopDataBase/AppUI_Data/IConfigNode; ns=(global)
- usings: KSP.UI
- unity: none; unity_score=12
- ksp refs: none; fan_in=2 fan_out=3
- methods(0): 
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/TransferDataTopDataBase.cs
- 13 lines; kind=class; type=TransferDataTopDataBase; bases=AppUI_Data; ancestry=AppUI_Data/IConfigNode; ns=(global)
- usings: KSP.UI
- unity: none; unity_score=12
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/TransferMath.cs **MUST-OPEN (>800 lines)**
- 1870 lines; kind=class; type=TransferMath; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=5
- ksp refs: Vector3d=145, Orbit=65, CelestialBody=11, Planetarium=6, QuaternionD=1; fan_in=2 fan_out=12
- methods(40): AlignmentTime, Bisect, BisectFunc, CalcCircularizeDV, CalcEjectionValues, CalcSameSOITransferDV, CalcTransferTime, CalcdV, CalcdVReturn, CalculateCorrection, CalculateCorrectionBurn, CalculateStartEndXferTimes, ChangingPeriapsis, D8RT, DVApPhaseHohmann, DVTimeHohmann, DVTimeInterCBXfer, DVToChangeApoapsis, DVToChangePeriapsis, DeltaVToInterceptAtTime, EjectionAngleCalc, FinalizeLambert, FindIntercept, HohmannTimeOfFlight, InterceptBody, LamGTL, LamGXL, LambertRHG, LambertSolver, NoBurnRequired, OrbitalPeriod, Root, SafeOrbitRadius, SameSOITransfer, SearchDVRange, SolveLambert, SuperiorLambert, TimeToANDN, TransferDV, TransferOrbit
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/TransferTypeBase.cs
- 107 lines; kind=class; type=TransferTypeBase; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization, KSP.UI, UnityEngine
- unity: none; unity_score=7
- ksp refs: Localizer=9, HighLogic=4; fan_in=2 fan_out=11
- methods(10): ChangePosition, ChangeTime, CreateManeuver, DisplayName, DrawTypeControls, FollowingManeuversCheck, HideApp, OnUpdate, PriorManeuversCheck, UpdateTransferData
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/TransferTypeSimple.cs **MUST-OPEN (>800 lines)**
- 1320 lines; kind=class; type=TransferTypeSimple; bases=TransferTypeBase; ancestry=TransferTypeBase; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, UnityEngine
- unity: SetActive=28, GameObject=25, GetComponent=5, Coroutine=2, Application.=1; unity_score=28
- ksp refs: FlightGlobals=29, Localizer=22, Planetarium=12, CelestialBody=9, Vector3d=8, Orbit=7; fan_in=0 fan_out=27
- methods(28): CalculationComplete, ChangePosition, ChangeTime, CompleteDrawControls, CreateManeuver, DidWeIntercept, DisplayName, DoMathCalcs, DrawTypeControls, FinalizeTransferDV, HideApp, InclinationTooBig, OnTargetClicked, OnUpdate, OnVisualizer, RefreshUIControls, ScalePlanetToAltitude, SetSourceBody, SetupCBNames, SetupVisualPrefabs, UpdateTopData, UpdateTransferData, UpdateVisualPanel, ValidCelestialBodies, ValidSituation, bw_CalculateTransfer, bw_ProcessCompleted, bw_ProgressChanged
- auto suggestion: tier=B subsystem=Orbit

## Assembly-CSharp/TransformExtension.cs
- 124 lines; kind=static class; type=TransformExtension; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Transform=25, GameObject=7, Shader=2, SetActive=2, Renderer=2, Material=1; unity_score=74
- ksp refs: none; fan_in=3 fan_out=0
- methods(7): FindChild, FindChildRecursive, FindParent, FindParentRecursive, NestToParent, SetLayerRecursive, SetShader
- auto suggestion: tier=B subsystem=Core

## Assembly-CSharp/Tumbler.cs
- 47 lines; kind=MonoBehaviour; type=Tumbler; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=1, Time.=1, MonoBehaviour=1, Lifecycle=1; unity_score=53
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Update, tumbleTo
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/Tumblers.cs
- 42 lines; kind=MonoBehaviour; type=Tumblers; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1; unity_score=43
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Awake, setValue
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/TutorialAscent.cs
- 531 lines; kind=ScenarioModule; type=TutorialAscent; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens.Flight, UnityEngine
- unity: Instantiate/Destroy=3, GameObject=3; unity_score=24
- ksp refs: Localizer=56, FlightGlobals=36, Vessel=12, GameSettings=11, InputLockManager=10, Vector3d=6; fan_in=0 fan_out=29
- methods(4): OnAssetSetup, OnOnDestroy, OnTutorialSetup, UpdateDirectionTarget
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/TutorialAsteroid1.cs
- 180 lines; kind=ScenarioModule; type=TutorialAsteroid1; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Transform=2, Texture=1, GetComponent=1; unity_score=33
- ksp refs: Localizer=33, FlightGlobals=8, GameSettings=6, Vessel=3, InputLockManager=3, MapView=2; fan_in=0 fan_out=19
- methods(3): OnAssetSetup, OnOnDestroy, OnTutorialSetup
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/TutorialAsteroid2.cs
- 192 lines; kind=ScenarioModule; type=TutorialAsteroid2; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: GetComponent=3; unity_score=26
- ksp refs: Localizer=29, FlightGlobals=9, Vessel=3, Part=2, InputLockManager=2, Planetarium=1; fan_in=0 fan_out=18
- methods(3): OnAssetSetup, OnOnDestroy, OnTutorialSetup
- auto suggestion: tier=A subsystem=Tutorials

## Assembly-CSharp/TutorialDemo.cs
- 125 lines; kind=ScenarioModule; type=TutorialDemo; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: IMGUI=18, UnityEngine.UI=6; unity_score=85
- ksp refs: none; fan_in=0 fan_out=5
- methods(3): OnAssetSetup, OnTutorialSetup, SomethingUnplanned
- auto suggestion: tier=C subsystem=Tutorials

## Assembly-CSharp/TutorialDocking.cs
- 266 lines; kind=ScenarioModule; type=TutorialDocking; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Transform=2, Texture=2, GetComponent=1; unity_score=33
- ksp refs: Localizer=44, FlightGlobals=23, GameSettings=21, Vessel=4, MapView=2, InputLockManager=2; fan_in=0 fan_out=22
- methods(3): OnAssetSetup, OnOnDestroy, OnTutorialSetup
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/TutorialDuna.cs
- 178 lines; kind=ScenarioModule; type=TutorialDuna; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.UI.Screens, UnityEngine
- unity: Transform=6, GameObject=6, SetActive=1, GetComponent=1; unity_score=54
- ksp refs: FlightGlobals=7, Vessel=2, Planetarium=1, InputLockManager=1, GameSettings=1; fan_in=0 fan_out=15
- methods(3): OnAssetSetup, OnOnDestroy, OnTutorialSetup
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/TutorialEditor.cs
- 497 lines; kind=ScenarioModule; type=TutorialEditor; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Texture=1; unity_score=18
- ksp refs: Localizer=53, InputLockManager=47, EditorLogic=43, AvailablePart=18, Part=16, GameSettings=6; fan_in=0 fan_out=29
- methods(5): OnAssetSetup, OnLoad, OnOnDestroy, OnSave, OnTutorialSetup
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/TutorialEditorAdvanced.cs **MUST-OPEN (>800 lines)**
- 3020 lines; kind=ScenarioModule; type=TutorialEditorAdvanced; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Transform=2, SetActive=2, GameObject=2, Texture=1, Application.=1; unity_score=17
- ksp refs: Part=210, Localizer=106, EditorLogic=59, InputLockManager=26, GameSettings=16, AvailablePart=6; fan_in=0 fan_out=31
- methods(30): AttachedNum, CheckAbort, CheckChute, CheckChuteStats, CheckDecouplerPod, CheckElectric, CheckEngineUpper, CheckFins, CheckLowerDec, CheckLowerEngine, CheckLowerEngineStats, CheckLowerTanks, CheckNosecones, CheckPod, CheckRCSTank, CheckRCSTankLoading, CheckRCSThrusters, CheckRadialDecs, CheckSolids, CheckSolidsStats, CheckT45Staging, CheckTanks, CheckTanksDry, CheckTanksWet, HasShip, OnAssetSetup, OnLoad, OnOnDestroy, OnSave, OnTutorialSetup
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/TutorialEditorMid.cs
- 747 lines; kind=ScenarioModule; type=TutorialEditorMid; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Transform=10, Texture=2; unity_score=28
- ksp refs: Localizer=42, Part=41, EditorLogic=38, InputLockManager=20, HighLogic=9, AvailablePart=6; fan_in=0 fan_out=30
- methods(15): CheckCraftFile, Check_BaseVessel, Check_BaseVesselNoStats, Check_Dec, Check_Engine, Check_Fins, Check_Goo, Check_Name, Check_Tanks, CopyCraftFile, OnAssetSetup, OnLoad, OnOnDestroy, OnSave, OnTutorialSetup
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/TutorialFlightBasics.cs
- 386 lines; kind=ScenarioModule; type=TutorialFlightBasics; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Instantiate/Destroy=3, GameObject=3, Texture=2, UnityEngine.UI=1; unity_score=34
- ksp refs: Localizer=52, GameSettings=13, FlightGlobals=13, Vessel=11, InputLockManager=10, HighLogic=3; fan_in=0 fan_out=23
- methods(3): OnAssetSetup, OnOnDestroy, OnTutorialSetup
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/TutorialFlightSuborbital.cs
- 611 lines; kind=ScenarioModule; type=TutorialFlightSuborbital; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Instantiate/Destroy=3, GameObject=3; unity_score=24
- ksp refs: Localizer=69, FlightGlobals=34, Vessel=24, GameSettings=21, InputLockManager=13, Part=3; fan_in=0 fan_out=25
- methods(4): OnAssetSetup, OnOnDestroy, OnTutorialSetup, UpdateDirectionTarget
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/TutorialFromMun.cs
- 424 lines; kind=ScenarioModule; type=TutorialFromMun; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Texture=1; unity_score=19
- ksp refs: Localizer=51, FlightGlobals=25, GameSettings=14, Vessel=13, InputLockManager=9, TimeWarp=5; fan_in=0 fan_out=26
- methods(4): OnAssetSetup, OnOnDestroy, OnTutorialSetup, UpdateDirectionTarget
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/TutorialMunDescent.cs
- 233 lines; kind=ScenarioModule; type=TutorialMunDescent; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Texture=1; unity_score=20
- ksp refs: Localizer=38, FlightGlobals=18, Vessel=9, InputLockManager=7, TimeWarp=2, MapView=2; fan_in=0 fan_out=20
- methods(3): OnAssetSetup, OnOnDestroy, OnTutorialSetup
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/TutorialMunFlight1.cs
- 267 lines; kind=ScenarioModule; type=TutorialMunFlight1; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: none; unity_score=15
- ksp refs: Localizer=40, FlightGlobals=27, InputLockManager=12, TimeWarp=3, Planetarium=3, MapView=2; fan_in=0 fan_out=19
- methods(3): OnAssetSetup, OnOnDestroy, OnTutorialSetup
- auto suggestion: tier=A subsystem=Tutorials

## Assembly-CSharp/TutorialOrbit101.cs
- 426 lines; kind=ScenarioModule; type=TutorialOrbit101; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens.Flight, UnityEngine
- unity: Texture=1; unity_score=23
- ksp refs: Localizer=79, FlightGlobals=22, TimeWarp=15, GameSettings=14, MapView=3, InputLockManager=3; fan_in=0 fan_out=24
- methods(4): OnAssetSetup, OnOnDestroy, OnTutorialSetup, remoteNudge
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/TutorialPageConfig.cs
- 50 lines; kind=class; type=TutorialPageConfig; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=3
- methods(3): AddTutorialStepConfig, InitializeTutoStepsConfig, OnEnterEmpty
- auto suggestion: tier=A subsystem=Tutorials

## Assembly-CSharp/TutorialScenario.cs
- 563 lines; kind=ScenarioModule; type=TutorialScenario; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, UnityEngine
- unity: GameObject=9, Lifecycle=5, Instantiate/Destroy=5, Screen/Cursor=3, GetComponent=3, Texture=2; unity_score=52
- ksp refs: Localizer=4, InputLockManager=4, ConfigNode=3, HighLogic=2, GameSettings=2, ScenarioModule=1; fan_in=33 fan_out=33
- methods(38): AddPage, AddPages, AddState, CalcDialogXRatio, CloseTutorialWindow, CompleteTutorial, CreateButtonUi, CreateDialog, CreateDoneDialog, CreateMultiButtonDialog, CreateNextDialog, CreateNoButtonDialog, CreateOkDialog, CreateTutorialPage, FixedUpdate, GetContinueButtonUi, GetCurrentStateName, GetDoneButtonUi, GetNextButtonUi, GetOkButtonUi, GetTutorialNode, GoToLastPage, GoToNextPage, LateUpdate, OnAssetSetup, OnCloseButtonClick, OnContinueButtonClick, OnDestroy, OnDoneButtonClick, OnNextButtonClick, OnOkButtonClick, OnOnDestroy, OnTutorialSetup, SetAdvanceCondition, SetDialogRect, Start, StartTutorial, Update
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/TutorialScience.cs
- 216 lines; kind=ScenarioModule; type=TutorialScience; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, UnityEngine
- unity: GameObject=10, Light=6, GetComponent=6, Transform=2, Screen/Cursor=1; unity_score=66
- ksp refs: Localizer=27, ResearchAndDevelopment=2, PartLoader=2, GameEvents=2, ConfigNode=2, HighLogic=1; fan_in=0 fan_out=16
- methods(6): EnterRD, OnAssetSetup, OnLoad, OnOnDestroy, OnSave, OnTutorialSetup
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/UI_ChooseOption.cs
- 35 lines; kind=attribute; type=UI_ChooseOption; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: UniLinq
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=3 fan_out=3
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_ColorPicker.cs
- 9 lines; kind=attribute; type=UI_ColorPicker; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_Control.cs
- 112 lines; kind=attribute; type=UI_Control; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=12
- ksp refs: ConfigNode=5; fan_in=41 fan_out=4
- methods(7): Load, ParseEnabled, ParseFloat, ParseString, Save, SetSceneVisibility, Setup
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_Cycle.cs
- 24 lines; kind=attribute; type=UI_Cycle; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: UniLinq
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=6 fan_out=3
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_EnumAttribute.cs
- 15 lines; kind=MonoBehaviour; type=UI_EnumAttribute; bases=PropertyAttribute; ancestry=PropertyAttribute; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/UI_FieldFloatRange.cs
- 29 lines; kind=attribute; type=UI_FieldFloatRange; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=1 fan_out=2
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_FloatEdit.cs
- 33 lines; kind=attribute; type=UI_FloatEdit; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=1 fan_out=2
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_FloatRange.cs
- 32 lines; kind=attribute; type=UI_FloatRange; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=32 fan_out=2
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_Grid.cs
- 16 lines; kind=attribute; type=UI_Grid; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=3
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_Group.cs
- 6 lines; kind=attribute; type=UI_Group; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_Label.cs
- 15 lines; kind=attribute; type=UI_Label; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=17 fan_out=2
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_MinMaxRange.cs
- 38 lines; kind=attribute; type=UI_MinMaxRange; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=4 fan_out=2
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_ProgressBar.cs
- 29 lines; kind=attribute; type=UI_ProgressBar; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=3 fan_out=2
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_Resources.cs
- 7 lines; kind=attribute; type=UI_Resources; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_ScaleEdit.cs
- 37 lines; kind=attribute; type=UI_ScaleEdit; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=1 fan_out=2
- methods(4): Load, MaxValue, MinValue, Save
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_Scene.cs
- 7 lines; kind=enum; type=UI_Scene; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=63 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_Toggle.cs
- 48 lines; kind=attribute; type=UI_Toggle; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=2, ConfigNode=2; fan_in=25 fan_out=3
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_VariantSelector.cs
- 8 lines; kind=attribute; type=UI_VariantSelector; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UI_Vector2.cs
- 35 lines; kind=attribute; type=UI_Vector2; bases=UI_Control; ancestry=UI_Control/Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=1 fan_out=2
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/UIAutoContentSizer.cs
- 12 lines; kind=MonoBehaviour; type=UIAutoContentSizer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Start, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UIButtonTextColor.cs
- 21 lines; kind=MonoBehaviour; type=UIButtonTextColor; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine
- unity: TMPro=1, MonoBehaviour=1; unity_score=57
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): SetIdleColor, SetOverColor
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UICameraController.cs
- 43 lines; kind=MonoBehaviour; type=UICameraController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, UnityEngine
- unity: Lifecycle=3, Camera=2, Screen/Cursor=1, MonoBehaviour=1, GetComponent=1; unity_score=76
- ksp refs: GameEvents=4; fan_in=0 fan_out=3
- methods(5): Awake, OnDestroy, OnSceneChange, ScreenResolutionModified, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UIConfirmDialog.cs
- 179 lines; kind=MonoBehaviour; type=UIConfirmDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=5, TMPro=5, Transform=4, GameObject=4, SetActive=3, Lifecycle=3; unity_score=80
- ksp refs: GameEvents=2; fan_in=9 fan_out=6
- methods(8): CloseDialog, OnCancel, OnConfirm, OnDestroy, OnLeavingScene, Spawn, Start, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UIConstructionPanelPart.cs
- 33 lines; kind=MonoBehaviour; type=UIConstructionPanelPart; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, SetActive=3, Lifecycle=3, GameObject=3, Texture=2, MonoBehaviour=1; unity_score=96
- ksp refs: none; fan_in=0 fan_out=0
- methods(4): OnButtonCollapsePressed, OnDestroy, Start, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UIFolderManagementDialog.cs
- 163 lines; kind=MonoBehaviour; type=UIFolderManagementDialog; bases=UIConfirmDialog; ancestry=UIConfirmDialog/MonoBehaviour; ns=(global)
- usings: KSP.Localization, TMPro, UnityEngine
- unity: Transform=4, GameObject=3, SetActive=2, TMPro=1, Instantiate/Destroy=1, GetComponent=1; unity_score=57
- ksp refs: Localizer=11; fan_in=1 fan_out=5
- methods(4): CountItemsInDirectory, InitCreateDialog, InitDeleteDialog, Spawn
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UIListClamper.cs
- 69 lines; kind=MonoBehaviour; type=UIListClamper; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: UnityEngine.UI=4, Time.=3, SetActive=3, GameObject=3, Lifecycle=2, MonoBehaviour=1; unity_score=82
- ksp refs: none; fan_in=0 fan_out=0
- methods(4): Awake, Clamp, FixedUpdate, OnRectTransformDimensionsChange
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UIMarquee.cs
- 402 lines; kind=MonoBehaviour; type=UIMarquee; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Transform=32, UnityEngine.UI=20, Coroutine=20, GetComponent=4, Lifecycle=3, GameObject=3; unity_score=82
- ksp refs: none; fan_in=2 fan_out=4
- methods(13): AutoStartMarquee, Awake, Configure, Field_OnValueModified, Initialize, MoveMarquee, OnDestroy, OnPointerEnter, OnPointerExit, ResetMovingFlags, Start, StartMarquee, StopMarquee
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UIMarquee_ActionField.cs
- 26 lines; kind=MonoBehaviour; type=UIMarquee_ActionField; bases=UIMarquee; ancestry=UIMarquee/MonoBehaviour; ns=(global)
- usings: KSP.UI, UnityEngine
- unity: Lifecycle=2, GetComponent=1; unity_score=59
- ksp refs: GameEvents=2; fan_in=0 fan_out=3
- methods(3): OnDestroy, OnPartActionUIDismiss, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UIMarquee_PAW.cs
- 47 lines; kind=MonoBehaviour; type=UIMarquee_PAW; bases=UIMarquee; ancestry=UIMarquee/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, GetComponent=1; unity_score=49
- ksp refs: Part=4, GameEvents=4, UIPartActionWindow=1; fan_in=0 fan_out=5
- methods(4): OnDestroy, OnPartActionUIDismiss, OnPartActionUIShown, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UINavExplicit.cs
- 39 lines; kind=MonoBehaviour; type=UINavExplicit; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, Lifecycle=2, GetComponent=2, MonoBehaviour=1; unity_score=77
- ksp refs: GameEvents=2; fan_in=1 fan_out=3
- methods(3): CheckForNonInteractable, OnDestroy, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UINavMouseChecker.cs
- 75 lines; kind=MonoBehaviour; type=UINavMouseChecker; bases=MonoBehaviour,ISelectHandler,IEventSystemHandler,IPointerEnterHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=6, MonoBehaviour=1; unity_score=67
- ksp refs: none; fan_in=2 fan_out=1
- methods(6): MouseHasExited, MouseIsHovering, OnPointerEnter, OnPointerExit, OnSelect, SetMenuNavReference
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UIPartActionAeroDisplay.cs
- 139 lines; kind=MonoBehaviour; type=UIPartActionAeroDisplay; bases=UIPartActionItem; ancestry=UIPartActionItem/MonoBehaviour; ns=(global)
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=15, SetActive=5, GameObject=5, UnityEngine.UI=2, Transform=1, Lifecycle=1; unity_score=88
- ksp refs: Localizer=9, UIPartActionWindow=1, Part=1; fan_in=2 fan_out=9
- methods(3): Awake, Setup, UpdateItem
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionButton.cs
- 70 lines; kind=MonoBehaviour; type=UIPartActionButton; bases=UIPartActionEventItem; ancestry=UIPartActionEventItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine.UI
- unity: UnityEngine.UI=1, TMPro=1, Lifecycle=1; unity_score=46
- ksp refs: EditorLogic=3, HighLogic=1; fan_in=2 fan_out=6
- methods(3): Awake, OnClick, UpdateItem
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/UIPartActionChooseOption.cs
- 141 lines; kind=MonoBehaviour; type=UIPartActionChooseOption; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: KSP.UI, TMPro, UnityEngine.UI
- unity: TMPro=2, UnityEngine.UI=1; unity_score=35
- ksp refs: InputLockManager=3, UIPartActionWindow=1, PartModule=1, Part=1; fan_in=0 fan_out=12
- methods(9): GetDisplay, GetFieldValue, OnTap_dec, OnTap_inc, OnValueChanged, SetFieldValue, Setup, UpdateDisplay, UpdateState
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionColorPicker.cs
- 143 lines; kind=MonoBehaviour; type=UIPartActionColorPicker; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=6, GameObject=6, SetActive=4, Lifecycle=2, GetComponent=2, Transform=1; unity_score=75
- ksp refs: UIPartActionWindow=1, PartModule=1, Part=1; fan_in=4 fan_out=11
- methods(7): CloseColorPickWindow, InitializeColourPresets, OnColorChange, OnDestroy, OnEnable, Setup, ToggleColorPickWindow
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionController.cs **MUST-OPEN (>800 lines)**
- 879 lines; kind=MonoBehaviour; type=UIPartActionController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, KSP.UI.Screens, UnityEngine, UnityEngine.EventSystems
- unity: GameObject=21, SetActive=11, Instantiate/Destroy=7, Transform=4, Lifecycle=4, Camera=4; unity_score=55
- ksp refs: UIPartActionWindow=39, Part=15, EditorLogic=12, GameEvents=9, HighLogic=6, PartResource=3; fan_in=32 fan_out=38
- methods(35): Activate, Awake, CreatePartUI, Deactivate, Deselect, GetControl, GetFieldControl, GetItem, GetSelectionModifier, HandleMouseClick, HiddenItemListGet, HiddenResourceItemListGet, InventoryAndCargoPartExist, ItemListContains, ItemListContainsCounterparts, ItemListGet, MouseClickCoroutine, OnCameraChange, OnDestroy, OnHideResource, OnShowResource, PartWindowIsFromAnInventory, SelectPart, SetupItemControls, SetupResourceTransfer, Show, ShowTransfers, SpawnPartActionWindow, Start, TrySelect, Update, UpdateActiveWindows, UpdateEditor, UpdateFlight, UpdateResourceWindows
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/UIPartActionControllerInventory.cs
- 725 lines; kind=MonoBehaviour; type=UIPartActionControllerInventory; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, KSP.UI.Screens, KSP.UI.TooltipTypes, UnityEngine, UnityEngine.SceneManagement
- unity: GameObject=26, Transform=20, Instantiate/Destroy=10, SetActive=8, Rigidbody=5, UnityEngine.UI=4; unity_score=70
- ksp refs: Part=15, HighLogic=13, GameEvents=13, PartLoader=4, FlightGlobals=3, AvailablePart=3; fan_in=11 fan_out=26
- methods(28): Awake, ConfigureInventoryPart, CreateFlightTooltip, CreatePartFromInventory, DestroyHeldPartAsIcon, DestroyTooltip, IsKerbalWithinRange, MoveSelectedPart, OnCameraChange, OnCargoPartConstructionEvent, OnDestroy, OnInventoryPartHover, OnPartPurchased, OnVesselDestroy, PlayPartDroppedSFX, PlayPartSelectedSFX, ResetInventoryCacheValues, ReturnHeldPart, SetIconAsPart, SetPartAsIcon, Start, ToggleCargoPartsLight, UpdateAudioVolume, UpdateCurrentCargoPart, UpdateCursorOverPAWs, UpdatePartIconScale, UpdateSelectedPartVisibility, dragOverPlane
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/UIPartActionControllerSpawner.cs
- 11 lines; kind=MonoBehaviour; type=UIPartActionControllerSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1; unity_score=59
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): Start
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/UIPartActionCycle.cs
- 53 lines; kind=MonoBehaviour; type=UIPartActionCycle; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, TMPro
- unity: TMPro=2; unity_score=46
- ksp refs: UIPartActionWindow=1, PartModule=1, Part=1, Localizer=1, InputLockManager=1; fan_in=0 fan_out=13
- methods(5): GetFieldValue, OnTap, SetButtonState, Setup, UpdateItem
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/UIPartActionEventItem.cs
- 95 lines; kind=MonoBehaviour; type=UIPartActionEventItem; bases=UIPartActionItem; ancestry=UIPartActionItem/MonoBehaviour; ns=(global)
- usings: -
- unity: Transform=1; unity_score=23
- ksp refs: FlightGlobals=6, PartModule=2, Part=2, GameSettings=2, UIPartActionWindow=1; fan_in=1 fan_out=10
- methods(4): CheckInRange, FireSymCounterparts, IsItemValid, Setup
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/UIPartActionFieldFloatRange.cs
- 71 lines; kind=MonoBehaviour; type=UIPartActionFieldFloatRange; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1; unity_score=30
- ksp refs: UIPartActionWindow=1, PartModule=1, Part=1, InputLockManager=1; fan_in=0 fan_out=11
- methods(5): OnValueChanged, SetSliderValue, Setup, UpdateFieldValues, UpdateItem
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/UIPartActionFieldItem.cs
- 155 lines; kind=MonoBehaviour; type=UIPartActionFieldItem; bases=UIPartActionItem; ancestry=UIPartActionItem/MonoBehaviour; ns=(global)
- usings: -
- unity: Transform=1; unity_score=20
- ksp refs: FlightGlobals=4, Part=3, PartModule=2, GameSettings=2, UIPartActionWindow=1, HighLogic=1; fan_in=18 fan_out=13
- methods(7): CheckInRange, FireSymmetryEvents, GetField, IsItemValid, SetFieldValue, SetSymCounterpartValue, Setup
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/UIPartActionFloatEdit.cs
- 231 lines; kind=MonoBehaviour; type=UIPartActionFloatEdit; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: KSP.UI, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=5, SetActive=4, UnityEngine.UI=3, TMPro=2, GetComponent=1; unity_score=54
- ksp refs: InputLockManager=5, UIPartActionWindow=1, PartModule=1, Part=1; fan_in=0 fan_out=13
- methods(14): AdjustValue, Clamp, GetFieldValue, IntervalBase, OnTap_decLarge, OnTap_decSmall, OnTap_incLarge, OnTap_incSmall, OnValueChanged, Setup, SliderInterval, UpdateControlStates, UpdateDisplay, UpdateSlider
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionFloatRange.cs
- 133 lines; kind=MonoBehaviour; type=UIPartActionFloatRange; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=4, SetActive=4, GameObject=4, UnityEngine.UI=1, Lifecycle=1; unity_score=66
- ksp refs: GameEvents=2, UIPartActionWindow=1, PartModule=1, Part=1, InputLockManager=1, GameSettings=1; fan_in=1 fan_out=15
- methods(8): GetFieldValue, OnDestroy, OnFieldInput, OnValueChanged, SetSliderValue, Setup, ToggleNumericSlider, UpdateItem
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionFuelFlowOverlay.cs
- 47 lines; kind=MonoBehaviour; type=UIPartActionFuelFlowOverlay; bases=UIPartActionItem; ancestry=UIPartActionItem/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, TMPro, UnityEngine
- unity: TMPro=1; unity_score=45
- ksp refs: UIPartActionWindow=1, Part=1, Localizer=1; fan_in=2 fan_out=9
- methods(2): FlowToggle, Setup
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/UIPartActionGroup.cs
- 162 lines; kind=MonoBehaviour; type=UIPartActionGroup; bases=UIPartActionItem; ancestry=UIPartActionItem/MonoBehaviour; ns=(global)
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=10, Transform=4, Texture=2, Lifecycle=2, TMPro=1, Instantiate/Destroy=1; unity_score=76
- ksp refs: GameSettings=5, GameEvents=2, UIPartActionWindow=1, Localizer=1; fan_in=2 fan_out=7
- methods(10): AddItemToContent, Collapse, CollapseGroupToggle, Expand, Initialize, OnDestroy, RescaleContentItems, SetUIState, Start, UpdateContentSize
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionInventory.cs
- 573 lines; kind=MonoBehaviour; type=UIPartActionInventory; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, KSP.UI.Screens.Editor, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=18, UnityEngine.UI=13, Transform=8, GetComponent=6, SetActive=5, Instantiate/Destroy=4; unity_score=68
- ksp refs: GameEvents=19, HighLogic=9, Vessel=4, Localizer=3, Part=2, InputLockManager=2; fan_in=5 fan_out=32
- methods(26): Awake, DestroyHeldPart, DestroyItem, GetFieldValue, HideGridObject, InitializeMassVolumeDisplay, InitializeSlots, OnDestroy, OnModuleInventoryChanged, OnModuleInventorySlotChanged, OnVesselChange, OnVesselSituationChange, ResetPlacePartIcons, SetAllSlotsNotSelected, Setup, SetupConstruction, ShowGridObject, SpawnEmptySlot, SpawnLoadedPart, StartPartPlacement, ToggleGridObject, Update, UpdateMassLimits, UpdatePlacePartIcons, UpdateSlot, UpdateVolumeLimits
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionInventorySlot.cs
- 535 lines; kind=MonoBehaviour; type=UIPartActionInventorySlot; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler,IPointerEnterHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=(global)
- usings: Highlighting, KSP.Localization, KSP.UI, KSP.UI.Screens, KSP.UI.Screens.Editor, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GetComponent=10, EventSystems=8, Transform=3, UnityEngine.UI=2, Resources.=1, MonoBehaviour=1; unity_score=48
- ksp refs: Part=10, HighLogic=9, GameSettings=5, FlightGlobals=5, Vessel=4, GameEvents=4; fan_in=4 fan_out=27
- methods(23): CanStackInSlot, ClearEmptiedSlot, CreatePartFromThisSlot, CreatePartIconFromThisSlot, EmptyStack, OnCargoModeToggled, OnDestroy, OnEditorPartDeleted, OnPointerClick, OnPointerEnter, OnPointerExit, ProcessClickNoHeldPart, ProcessClickWithHeldPart, RemovePartFromStack, ReturnHeldPartToThisSlot, SetStackedPartsInSlot, Setup, SlotClicked, StackPartsInSlot, StackPartsInThisSlot, StorePartInEmptySlot, UpdateCurrentSelectedSlot, UpdateStackUI
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionItem.cs
- 72 lines; kind=MonoBehaviour; type=UIPartActionItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=25
- ksp refs: PartModule=4, Part=4, UIPartActionWindow=3, InputLockManager=2; fan_in=12 fan_out=8
- methods(5): AddInputFieldLock, IsItemValid, RemoveInputfieldLock, SetupItem, UpdateItem
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/UIPartActionLabel.cs
- 43 lines; kind=MonoBehaviour; type=UIPartActionLabel; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine.UI
- unity: UnityEngine.UI=2, TMPro=2, GetComponent=2, GameObject=2, Lifecycle=1; unity_score=82
- ksp refs: none; fan_in=0 fan_out=3
- methods(2): Awake, UpdateItem
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionMinMaxRange.cs
- 177 lines; kind=MonoBehaviour; type=UIPartActionMinMaxRange; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine
- unity: TMPro=5, SetActive=2, GameObject=2, Lifecycle=1; unity_score=54
- ksp refs: GameEvents=2, UIPartActionWindow=1, PartModule=1, Part=1, InputLockManager=1, GameSettings=1; fan_in=0 fan_out=16
- methods(9): GetFieldValue, OnDestroy, OnValueChanged, SetNumericMaxValue, SetNumericMinValue, SetSliderValue, Setup, ToggleNumericSlider, UpdateItem
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionProgressBar.cs
- 31 lines; kind=MonoBehaviour; type=UIPartActionProgressBar; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine.UI
- unity: TMPro=2, UnityEngine.UI=1; unity_score=69
- ksp refs: none; fan_in=0 fan_out=4
- methods(1): UpdateItem
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionResource.cs
- 110 lines; kind=MonoBehaviour; type=UIPartActionResource; bases=UIPartActionResourceItem; ancestry=UIPartActionResourceItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: KSP.UI, KSP.UI.TooltipTypes, TMPro, UnityEngine.UI
- unity: TMPro=3, GameObject=2, UnityEngine.UI=1, SetActive=1, GetComponent=1; unity_score=57
- ksp refs: UIPartActionWindow=2, PartResource=1, Part=1, InputLockManager=1, FlightGlobals=1; fan_in=2 fan_out=15
- methods(5): FlowBtnToggled, IsItemValid, SetButtonState, Setup, UpdateItem
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionResourceDrain.cs
- 134 lines; kind=MonoBehaviour; type=UIPartActionResourceDrain; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine, UnityEngine.UI
- unity: GameObject=4, Lifecycle=2, GetComponent=2, UnityEngine.UI=1, Transform=1, Instantiate/Destroy=1; unity_score=56
- ksp refs: Localizer=3, UIPartActionWindow=1, PartResource=1, PartModule=1, Part=1; fan_in=1 fan_out=12
- methods(10): Awake, CacheLocalStrings, GetFieldValue, InitializeResourceFieldsDrainStatus, InitializeResources, OnDestroy, Setup, UpdatePartResourceStatus, UpdateResourcesDrainRate, UpdateResourcesDrainRateAndStatus
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/UIPartActionResourceEditor.cs
- 176 lines; kind=MonoBehaviour; type=UIPartActionResourceEditor; bases=UIPartActionResourceItem; ancestry=UIPartActionResourceItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: KSP.UI, KSP.UI.TooltipTypes, TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=5, GameObject=4, SetActive=3, UnityEngine.UI=1, Lifecycle=1, GetComponent=1; unity_score=60
- ksp refs: GameEvents=3, UIPartActionWindow=1, PartResource=1, Part=1, InputLockManager=1, GameSettings=1; fan_in=2 fan_out=18
- methods(10): FlowBtnToggled, IsItemValid, OnDestroy, OnFieldInput, OnSliderChanged, SetButtonState, Setup, ToggleNumericSlider, UpdateItem, onSliderChangeProcess
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionResourceItem.cs
- 36 lines; kind=MonoBehaviour; type=UIPartActionResourceItem; bases=UIPartActionItem; ancestry=UIPartActionItem/MonoBehaviour; ns=(global)
- usings: -
- unity: none; unity_score=15
- ksp refs: PartResource=3, UIPartActionWindow=1, Part=1; fan_in=3 fan_out=6
- methods(3): SetSymCounterpartsAmount, SetSymCounterpartsFlowState, Setup
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/UIPartActionResourcePriority.cs
- 89 lines; kind=MonoBehaviour; type=UIPartActionResourcePriority; bases=UIPartActionItem; ancestry=UIPartActionItem/MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, TMPro=3, Lifecycle=1; unity_score=68
- ksp refs: GameSettings=2, UIPartActionWindow=1, Part=1; fan_in=2 fan_out=7
- methods(6): Awake, OnDecClick, OnIncClick, OnResetClick, Setup, UpdateItem
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionResourceToggle.cs
- 91 lines; kind=MonoBehaviour; type=UIPartActionResourceToggle; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=2, Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1; unity_score=55
- ksp refs: PartResource=3, Localizer=2; fan_in=1 fan_out=3
- methods(10): Awake, CacheLocalStrings, InitializeItem, OnDestroy, PartStatus, PartStatusToggle, UpdateItemName, UpdateItemNameAndStatus, UpdateStatusText, UpdateStatusTextAndStatus
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionResourceTransfer.cs
- 244 lines; kind=MonoBehaviour; type=UIPartActionResourceTransfer; bases=UIPartActionResourceItem; ancestry=UIPartActionResourceItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: SetActive=19, GameObject=19, UnityEngine.UI=3, Lifecycle=2; unity_score=75
- ksp refs: InputLockManager=3, UIPartActionWindow=2, Planetarium=2, PartResource=1, Part=1, FlightGlobals=1; fan_in=2 fan_out=12
- methods(11): Awake, FlowIn, FlowOut, FlowStop, IsItemValid, OnBtnIn, OnBtnOut, OnBtnStop, OnDestroy, Setup, UpdateItem
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionRoboticJointDisplay.cs
- 155 lines; kind=MonoBehaviour; type=UIPartActionRoboticJointDisplay; bases=UIPartActionItem; ancestry=UIPartActionItem/MonoBehaviour; ns=(global)
- usings: Expansions.Serenity, KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=8, SetActive=7, GameObject=7, UnityEngine.UI=1, Lifecycle=1; unity_score=78
- ksp refs: Localizer=4, UIPartActionWindow=1, Part=1; fan_in=2 fan_out=13
- methods(5): Awake, DebugText, NumberFormat, Setup, UpdateItem
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionScaleEdit.cs
- 146 lines; kind=MonoBehaviour; type=UIPartActionScaleEdit; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: KSP.UI, TMPro, UnityEngine.UI
- unity: TMPro=2, UnityEngine.UI=1; unity_score=35
- ksp refs: InputLockManager=3, UIPartActionWindow=1, PartModule=1, Part=1; fan_in=0 fan_out=13
- methods(10): FindInterval, GetFieldValue, GetStep, OnTap_dec, OnTap_inc, OnValueChanged, Setup, UpdateDisplay, UpdateInterval, UpdateSlider
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionThermalDisplay.cs
- 66 lines; kind=MonoBehaviour; type=UIPartActionThermalDisplay; bases=UIPartActionItem; ancestry=UIPartActionItem/MonoBehaviour; ns=(global)
- usings: KSP.Localization, TMPro, UnityEngine
- unity: TMPro=9, Lifecycle=1; unity_score=85
- ksp refs: Localizer=10, UIPartActionWindow=1, Part=1; fan_in=2 fan_out=8
- methods(3): Awake, Setup, UpdateItem
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionToggle.cs
- 86 lines; kind=MonoBehaviour; type=UIPartActionToggle; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: KSP.UI, TMPro, UnityEngine.UI
- unity: TMPro=3, UnityEngine.UI=2, Lifecycle=1, GetComponent=1, GameObject=1; unity_score=64
- ksp refs: UIPartActionWindow=1, PartModule=1, Part=1, InputLockManager=1; fan_in=0 fan_out=13
- methods(6): Awake, GetFieldValue, OnTap, SetButtonState, Setup, UpdateItem
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionVariantButton.cs
- 81 lines; kind=MonoBehaviour; type=UIPartActionVariantButton; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: EventSystems=6, UnityEngine.UI=5, SetActive=3, GameObject=3, Shader=2, MonoBehaviour=1; unity_score=88
- ksp refs: none; fan_in=1 fan_out=1
- methods(5): OnButtonPressed, Reset, Select, SetColor, Setup
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionVariantSelector.cs
- 200 lines; kind=MonoBehaviour; type=UIPartActionVariantSelector; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, Transform=3, Instantiate/Destroy=2, GetComponent=2, GameObject=2, TMPro=1; unity_score=55
- ksp refs: Part=4, Localizer=3, GameEvents=2, UIPartActionWindow=1, PartModule=1; fan_in=1 fan_out=14
- methods(14): AddVariantButton, GetFieldValue, GetSurfaceAttachedPartCount, OnButtonNext, OnButtonPressed, OnButtonPrev, OnDestroy, OnEditorPartEvent, RefreshVariantButtons, ResetButtonOverText, SelectVariant, SetButtonOverText, Setup, UpdateItem
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UIPartActionVector2.cs
- 57 lines; kind=MonoBehaviour; type=UIPartActionVector2; bases=UIPartActionFieldItem; ancestry=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, Lifecycle=1; unity_score=49
- ksp refs: UIPartActionWindow=1, PartModule=1, Part=1, InputLockManager=1; fan_in=0 fan_out=11
- methods(6): Awake, GetFieldValue, OnValueChanged, SetSliderValue, Setup, UpdateItem
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/UIPartActionWindow.cs **MUST-OPEN (>800 lines)**
- 2006 lines; kind=MonoBehaviour; type=UIPartActionWindow; bases=MonoBehaviour,IDragHandler,IEventSystemHandler,IPointerEnterHandler,IPointerExitHandler,IBeginDragHandler,IEndDragHandler; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, KSP.UI, RUI.Algorithms, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Transform=70, GameObject=37, Instantiate/Destroy=27, SetActive=22, UnityEngine.UI=14, EventSystems=12; unity_score=62
- ksp refs: Part=46, GameEvents=23, PartResource=15, PartModule=14, GameSettings=14, FlightGlobals=11; fan_in=46 fan_out=53
- methods(85): AddAeroControl, AddCrewInventory, AddEventControl, AddFieldControl, AddFuelFlowOverlay, AddGroup, AddItem, AddParameterGroup, AddResourceEditorControl, AddResourceFlightControl, AddResourcePriorityControl, AddResourceTransferControl, AddRoboticJointControl, AddThermalControl, Awake, CanActivateAeroDisplay, CanActivateEvent, CanActivateField, CanActivateFuelFlowOverlay, CanActivateResource, CanActivateResourcePriorityDisplay, CanActivateRoboticJointDisplay, CanActivateThermalDisplay, ClearList, ClearReferences, CloseAllColorPickers, ContainsResourceControl, ContainsResourceTransferControl, CreatePartList, GetPartCrew, GetPartHover, HasKerbalInventory, NumericInputModeAndFocusedItem, OnBeginDrag, OnCrewBoardVessel, OnCrewTransferred, OnDestroy, OnDrag, OnEditorPartEvent, OnEditorShipCrewModified, OnEnable, OnEndDrag, OnGameSettingsApplied, OnKerbalEVA, OnNumericSwap ...
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/UISkinDef.cs
- 52 lines; kind=class; type=UISkinDef; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=7
- ksp refs: none; fan_in=45 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UISkinManager.cs
- 88 lines; kind=MonoBehaviour; type=UISkinManager; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI.Util, TMPro, UnityEngine
- unity: GameObject=10, Lifecycle=3, TMPro=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=77
- ksp refs: none; fan_in=38 fan_out=3
- methods(5): Awake, GetPrefab, GetSkin, OnDestroy, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UIStyle.cs
- 65 lines; kind=class; type=UIStyle; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: none; fan_in=7 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UIStyleState.cs
- 10 lines; kind=class; type=UIStyleState; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Texture=1; unity_score=36
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/UnderwaterFog.cs
- 270 lines; kind=MonoBehaviour; type=UnderwaterFog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=26, Lifecycle=3, Transform=1, MonoBehaviour=1, Coroutine=1; unity_score=73
- ksp refs: FlightGlobals=10, HighLogic=5, GameEvents=5, Planetarium=4, FlightCamera=4, CelestialBody=4; fan_in=1 fan_out=12
- methods(12): Awake, LateUpdate, OnDestroy, OnPostRender, OnPreRender, OnSceneChange, SetDefaultCB, Start, UpdateCBProperties, UpdateShaderProperties, UpdateShaderPropertiesDefault, VesselSOIChange
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/UnitTestingWindow.cs
- 89 lines; kind=MonoBehaviour; type=UnitTestingWindow; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Testing, UnityEngine
- unity: IMGUI=15, Screen/Cursor=2, Lifecycle=2, GameObject=2, UnityEngine.UI=1, MonoBehaviour=1; unity_score=90
- ksp refs: none; fan_in=1 fan_out=3
- methods(6): Awake, DrawState, DrawUnitTestWindow, OnDestroy, OnGUI, OpenWindow
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/UntrackedObjectClass.cs
- 23 lines; kind=enum; type=UntrackedObjectClass; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=24 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/UrlDir.cs **MUST-OPEN (>800 lines)**
- 927 lines; kind=class; type=UrlDir; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Coroutine=10, Application.=3, Texture=1, AssetBundle=1; unity_score=20
- ksp refs: ConfigNode=9; fan_in=17 fan_out=1
- methods(22): AddConfig, ConfigExists, ConfigureFile, ConstructURL, ContainsConfig, Create, CreateApplicationPath, CreateDirectory, CreateNodeList, DirectoryExists, Exists, FileExists, GetConfig, GetConfigs, GetDirectory, GetEnumerator, GetFile, GetFiles, PathCombine, SaveConfigs, StripExtension, UrlSplit
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/UtilMath.cs
- 548 lines; kind=class; type=UtilMath; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Vector3d=13; fan_in=58 fan_out=1
- methods(35): ACosh, ACoth, ACsch, ASech, ASinh, ATanh, AngleBetween, Approximately, BSPSolver, Clamp, Clamp01, ClampDegrees180, ClampDegrees360, ClampRadians, ClampRadiansPI, ClampRadiansTwoPI, Coth, Csch, DegreesToRadians, Flatten, InverseLerp, IsDivisible, IsPowerOfTwo, Lerp, LerpUnclamped, Max, MaxFrom, Min, MinFrom, RadiansToDegrees, RoundToPlaces, Sech, SphereIntersection, SwapValues, WrapAround
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/VABCamera.cs
- 288 lines; kind=MonoBehaviour; type=VABCamera; bases=MonoBehaviour,IKSPCamera; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI.Screens, UnityEngine, UnityEngine.EventSystems
- unity: Transform=27, Time.=7, Input.=4, Lifecycle=3, GameObject=2, Texture=1; unity_score=76
- ksp refs: GameSettings=18, GameEvents=4, InputLockManager=3, EditorLogic=3, ShipConstruct=1, HighLogic=1; fan_in=1 fan_out=15
- methods(19): GetCameraTransform, GetPivot, OnDestroy, OnEditorRestart, OnEditorShipLoad, OnEnable, OnNavigatorHandoff, OnNavigatorRequestControl, OnNavigatorTakeOver, PlaceCamera, ResetCamera, SetCamCoordsFromPosition, SetInitialCameraPosition, Start, Update, UpdateCamera, getPitch, getReferenceFrame, getYaw
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/ValueBox.cs
- 9 lines; kind=class; type=ValueBox; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ValueModifierQuery.cs
- 94 lines; kind=class; type=ValueModifierQuery; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: GameEvents=1; fan_in=3 fan_out=1
- methods(6): AddDelta, GetEffectDelta, GetEffectDeltaText, GetEffectPercentageText, GetInput, RunQuery
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/Vect.cs
- 19 lines; kind=static class; type=Vect; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=17
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/Vector3Curve.cs
- 56 lines; kind=class; type=Vector3Curve; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: AnimationCurve=6; unity_score=47
- ksp refs: none; fan_in=1 fan_out=0
- methods(3): Add, EvaluateColor, EvaluateVector
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/Vector3d.cs
- 354 lines; kind=struct; type=Vector3d; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=3
- ksp refs: Vector3d=129; fan_in=184 fan_out=0
- methods(29): Angle, AngleBetween, Basis, Cross, Distance, Dot, Equals, Exclude, GetHashCode, Internal_OrthoNormalize2, Internal_OrthoNormalize3, IsZero, Lerp, Magnitude, Max, Min, Normalize, OrthoNormalize, Project, Reflect, RotateTowards, Scale, Slerp, SqrMagnitude, Swizzle, ToString, Vector3, Vector3d, Zero
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/Vector4Curve.cs
- 62 lines; kind=class; type=Vector4Curve; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: AnimationCurve=8; unity_score=50
- ksp refs: none; fan_in=1 fan_out=0
- methods(3): Add, EvaluateColor, EvaluateVector
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/VectorArrayMaterialProperty.cs
- 17 lines; kind=class; type=VectorArrayMaterialProperty; bases=MaterialPropertyExtensions.MaterialProperty; ancestry=MaterialProperty; ns=(global)
- usings: UnityEngine
- unity: Material=1; unity_score=32
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): Apply
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/VectorExtensions.cs
- 112 lines; kind=static class; type=VectorExtensions; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=26
- ksp refs: Vector3d=1; fan_in=1 fan_out=2
- methods(8): BearingDegrees, BearingRadians, HeadingDegrees, HeadingRadians, IsInvalid, IsSmallerThan, IsZero, Zero
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/VectorMaterialProperty.cs
- 17 lines; kind=class; type=VectorMaterialProperty; bases=MaterialPropertyExtensions.MaterialProperty; ancestry=MaterialProperty; ns=(global)
- usings: UnityEngine
- unity: Shader=1, Material=1; unity_score=46
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): Apply
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/VehicleAssemblyBuilding.cs
- 51 lines; kind=MonoBehaviour; type=VehicleAssemblyBuilding; bases=SpaceCenterBuilding; ancestry=SpaceCenterBuilding/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Runtime, KSP.Localization, UnityEngine
- unity: none; unity_score=19
- ksp refs: HighLogic=8, MissionSystem=3, Localizer=3, Game=3, InputLockManager=2; fan_in=12 fan_out=16
- methods(3): IsOpen, OnClicked, OnPopupWarningDismiss
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/VersionCompareResult.cs
- 7 lines; kind=enum; type=VersionCompareResult; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=17 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/Versioning.cs
- 538 lines; kind=MonoBehaviour; type=Versioning; bases=VersioningBase; ancestry=VersioningBase/MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.Networking
- unity: IMGUI=14, Application.=11, Coroutine=5, WWW/WebRequest=4, UnityEngine.UI=3, Steam=2; unity_score=58
- ksp refs: GameSettings=5, InputLockManager=2; fan_in=24 fan_out=7
- methods(21): BytesToHex, CancelUpdate, CheckForUpdates, FileMD5Bytes, FileMD5String, GetOSCode, GetUpdateChannel, GetVersion, GetVersionFromString, GetVersionStringFull, GetVersionStringWithExperimental, GetVersionStringWithPrerelease, OnAwake, PatcherDownloaded, QuitAndStartPatcher, Start, Update, UpdateGame, UpdatePatcher, checkForUpdate, drawNewVersionWindow
- auto suggestion: tier=C subsystem=Save

## Assembly-CSharp/VersioningBase.cs
- 60 lines; kind=MonoBehaviour; type=VersioningBase; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1, GetComponent=1; unity_score=55
- ksp refs: none; fan_in=8 fan_out=0
- methods(5): Awake, GetVersion, GetVersionString, OnAwake, OnDestroy
- auto suggestion: tier=B subsystem=Save

## Assembly-CSharp/VersionReadout.cs
- 26 lines; kind=MonoBehaviour; type=VersionReadout; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: GetComponent=4, TMPro=3, MonoBehaviour=1, Lifecycle=1; unity_score=89
- ksp refs: Localizer=2; fan_in=0 fan_out=4
- methods(1): Start
- auto suggestion: tier=C subsystem=Misc

## Assembly-CSharp/Vessel.cs **MUST-OPEN (>800 lines)**
- 4664 lines; kind=MonoBehaviour; type=Vessel; bases=MonoBehaviour,IShipconstruct,ITargetable,IDiscoverable; ancestry=MonoBehaviour; ns=(global)
- usings: CommNet, Contracts, Expansions.Missions.Scenery.Scripts, FinePrint, KSP.Localization, KSP.UI.Screens, RUI.Algorithms, UnityEngine, VehiclePhysics
- unity: GameObject=48, GetComponent=37, Instantiate/Destroy=28, Transform=27, LayerMask=12, Time.=11; unity_score=46
- ksp refs: Part=64, FlightGlobals=56, Vector3d=49, GameEvents=46, Localizer=45, PhysicsGlobals=25; fan_in=393 fan_out=111
- methods(179): ActionControlBlocked, AddOrbitRenderer, AddPhysicsHoldLock, AddRBAnchor, AttachPatchedConicsSolver, AutoRename, Awake, AxisControlBlocked, BackupVessel, BlockedMask, BuildCrossfeedPartSets, CacheLocalStrings, CacheScreenMessageObjects, ChangeWorldVelocity, CheckAirstreamShields, CheckControllable, CheckGroundCollision, CheckKill, Clean, ClearActiveInternalSpace, ClearStaging, ContainsCollider, CopyOverrides, CountSerenityControllerFields, CountSerenityHingeParts, CountSerenityModRoboticParts, CountSerenityPistonParts, CountSerenityRoboticController, CountSerenityRoboticParts, CountSerenityRotationServoParts, CountSerenityRotorParts, CrewListSetDirty, CrewWasModified, CycleAllAutoStrut, DespawnCrew, DestroyVesselComponents, DetachPatchedConicsSolver, Die, FallBackReferenceTransform, FeedInputFeed, FindDefaultVesselType, FindPartModuleImplementing, FindPartModulesImplementing, FindReferenceTransform, FindVesselModuleImplementing ...
- auto suggestion: tier=B subsystem=Vessel

## Assembly-CSharp/VesselActuatorMode.cs
- 6 lines; kind=enum; type=VesselActuatorMode; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/VesselAutopilot.cs **MUST-OPEN (>800 lines)**
- 1274 lines; kind=class; type=VesselAutopilot; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Transform=3, Rigidbody=3, Lifecycle=3, Instantiate/Destroy=3, Time.=2; unity_score=14
- ksp refs: GameSettings=24, Vessel=15, FlightGlobals=11, TimeWarp=10, Vector3d=6, Localizer=3; fan_in=4 fan_out=23
- methods(54): AngularTrim, AutoTuneReset, AutoTuneScalar, CanEngageSAS, CanSetMode, CheckCoasting, CheckDamping, CheckPitchInput, CheckRollInput, CheckStoredTransform, CheckYawInput, ConnectFlyByWire, ControlUpdate, Destroy, Disable, DisconnectFlyByWire, Enable, EvaluateScalar, GetRotationDelta, GetTotalVesselTorque, KillAngularVelocity, LockRotation, ManeuverLockInvalid, ModuleSetup, PitchYawAngle, ResetAllPIDS, ResetDecay, ResetTuningScalars, SASUpdate, SetAutopilot, SetAutopilotOrbit, SetAutopilotSurface, SetAutopilotTarget, SetDampingMode, SetLockPitchPID, SetLockRollPID, SetLockYawPID, SetMode, SetTargetOrientation, SetTuningScalar, SetVessel, SetupModules, StabilityDecay, Start, TargetLockInvalid ...
- auto suggestion: tier=B subsystem=PartModules-Control

## Assembly-CSharp/VesselAutopilotUI.cs
- 246 lines; kind=MonoBehaviour; type=VesselAutopilotUI; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, UnityEngine
- unity: SetActive=10, GameObject=10, Lifecycle=4, UnityEngine.UI=3, MonoBehaviour=1, GetComponent=1; unity_score=65
- ksp refs: FlightGlobals=12, GameEvents=5, Vessel=1, ProtoCrewMember=1; fan_in=0 fan_out=7
- methods(13): Awake, GetButtonIndex, IsSkillActive, LateUpdate, OnClickButton, OnDestroy, OnKerbalLevelUp, OnVesselChange, SetButtonTrue, SetPilotSkill, Start, ToggleUI, onGameParametersChanged
- auto suggestion: tier=C subsystem=PartModules-Control

## Assembly-CSharp/VesselCrewManifest.cs
- 581 lines; kind=class; type=VesselCrewManifest; bases=-; ancestry=-; ns=(global)
- usings: Expansions.Missions, UnityEngine
- unity: GameObject=1; unity_score=5
- ksp refs: ProtoCrewMember=19, HighLogic=14, KerbalRoster=9, ConfigNode=3, ShipConstruct=2, Part=2; fan_in=34 fan_out=9
- methods(25): AddCrewMembers, AnyCrewInState, AnyCrewWithTrait, AssignCrewToVessel, CloneOf, Contains, DebugManifest, Filter, FromConfigNode, GetAllCrew, GetCrewInventoryCosts, GetCrewInventoryMass, GetCrewResourceMass, GetCrewableParts, GetPartCrewManifest, GetPartForCrew, HasAnyCrew, MergeInto, RemoveCrewMember, RemovePartManifest, SetCrewMember, SetPartManifest, SetPartManifestNoOverwrite, UpdateCrewForVessel, UpdatePartManifest
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/VesselDeltaV.cs **MUST-OPEN (>800 lines)**
- 1270 lines; kind=MonoBehaviour; type=VesselDeltaV; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Missions, Expansions.Missions.Adjusters, KSP.UI, KSP.UI.Screens, UnityEngine
- unity: Time.=11, Coroutine=8, Lifecycle=4, GetComponent=4, GameObject=2, MonoBehaviour=1; unity_score=33
- ksp refs: GameEvents=54, HighLogic=24, Vessel=22, FlightGlobals=18, GameSettings=13, Part=11; fan_in=13 fan_out=34
- methods(53): CalculateSeparationStageIndexes, Callback_CrewChange, CheckDirtyAndRun, Create, DisableStockSimluation, EnableStockSimluation, FixedUpdate, GetActivatedEngines, GetHighestSeparationStage, GetSituationTotalDeltaV, GetStage, GetWorkingStage, OnDestroy, OnDockingComplete, OnEngineActiveChange, OnEngineThrustPercentageChanged, OnGUIStageSequenceModified, OnMultiModeEngineSwitchActive, OnNodeChangedVesselResources, OnPartModuleAdjusterAdded, OnPartModuleAdjusterRemoved, OnStageActivate, OnStagingSeparationIndices, OnVesselChange, OnVesselGoOffRails, OnVesselsUndocking, ProcessActiveUpdateEngines, ProcessDecouplePartInfoChildren, ProcessEnginePartInfoChildren, ProcessPayloadStages, ReCalculateVesselTotals, ResetPartInfo, ResetSeparationIndexes, ResetSimulationResources, ResetStagePartCaches, RunCalculations, SetCalcsDirty, SimulateFlightScene, SimulateLastStage, StagesChanged, Start, Update, UpdateModuleEngines, UpdateResourceSetsEvent, UpdateResourceSetsEventSetFL ...
- auto suggestion: tier=B subsystem=Vessel

## Assembly-CSharp/VesselEngiUtil.cs
- 7 lines; kind=static class; type=VesselEngiUtil; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Vessel=1; fan_in=0 fan_out=1
- methods(1): GetDeltaV
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/VesselLabel.cs
- 21 lines; kind=MonoBehaviour; type=VesselLabel; bases=BaseLabel; ancestry=BaseLabel/MonoBehaviour; ns=(global)
- usings: UnityEngine.EventSystems
- unity: EventSystems=2; unity_score=59
- ksp refs: Vessel=2; fan_in=1 fan_out=3
- methods(2): OnClick, Setup
- auto suggestion: tier=B subsystem=MapView

## Assembly-CSharp/VesselLabels.cs
- 505 lines; kind=MonoBehaviour; type=VesselLabels; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Missions, KSP.Localization, KSP.UI, KSP.UI.Screens, TMPro, UnityEngine
- unity: Transform=10, Instantiate/Destroy=7, GameObject=6, Lifecycle=3, UnityEngine.UI=2, Texture=1; unity_score=51
- ksp refs: Vessel=14, GameSettings=7, FlightGlobals=7, GameEvents=6, Localizer=5, FlightCamera=2; fan_in=4 fan_out=24
- methods(20): AddNodeLabel, Awake, CreateNodeLabel, CreateVesselLabel, DestroyAllLabels, DestroyNodeLabels, DestroyVesselLabel, DisableAllLabels, EnableAllLabels, GetCurrentTarget, GetLabel, GetLabelType, LateUpdate, OnDestroy, OnDoubleClickLabel, OnSceneLoad, OnVesselCreate, OnVesselDestroy, ProcessLabel, RemoveNodeLabel
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/VesselModule.cs
- 134 lines; kind=VesselModule; type=VesselModule; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1, GetComponent=1; unity_score=36
- ksp refs: Vessel=4, ConfigNode=4, HighLogic=2, VesselModule=1; fan_in=9 fan_out=4
- methods(15): Awake, GetActivation, GetOrder, Load, OnAwake, OnGoOffRails, OnGoOnRails, OnLoad, OnLoadVessel, OnSave, OnStart, OnUnloadVessel, Save, ShouldBeActive, Start
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/VesselModuleManager.cs
- 198 lines; kind=static class; type=VesselModuleManager; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: GameObject=11, GetComponent=4, Instantiate/Destroy=3; unity_score=44
- ksp refs: VesselModule=10, Vessel=3; fan_in=2 fan_out=4
- methods(7): AddModulesToVessel, CompileModules, GetModules, GetWrapper, RemoveModuleOfType, RemoveModulesFromVessel, SetWrapperActive
- auto suggestion: tier=B subsystem=Vessel

## Assembly-CSharp/VesselNaming.cs
- 64 lines; kind=class; type=VesselNaming; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=4, ConfigNode=3, Vessel=1, ShipConstruct=1; fan_in=6 fan_out=6
- methods(3): FindPriorityNamePart, Load, Save
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/VesselPrecalculate.cs
- 525 lines; kind=MonoBehaviour; type=VesselPrecalculate; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Rigidbody=13, Lifecycle=4, Transform=3, Physics.=1, MonoBehaviour=1, GetComponent=1; unity_score=48
- ksp refs: Vector3d=31, PhysicsGlobals=8, OrbitDriver=6, GameEvents=5, Vessel=4, Part=4; fan_in=2 fan_out=18
- methods(16): ApplyVelocityCorrection, Awake, CacheLocalStrings, CalculateGravity, CalculatePhysicsStats, FixedUpdate, GoOffRails, GoOnRails, MainPhysics, OnDestroy, RunFirst, SetLandedPosRot, StartEasing, StopEasing, Update, onDockingComplete
- auto suggestion: tier=B subsystem=Vessel

## Assembly-CSharp/VesselRanges.cs
- 159 lines; kind=class; type=VesselRanges; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: -
- unity: none; unity_score=5
- ksp refs: Vessel=7, ConfigNode=6; fan_in=4 fan_out=3
- methods(3): GetSituationRanges, Load, Save
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/VesselRecovery.cs
- 181 lines; kind=ScenarioModule; type=VesselRecovery; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, KSP.UI.Screens.SpaceCenter.MissionSummaryDialog, UnityEngine
- unity: Lifecycle=2; unity_score=29
- ksp refs: Reputation=7, ResearchAndDevelopment=6, GameEvents=6, Funding=5, ProtoVessel=3, ProtoCrewMember=3; fan_in=6 fan_out=25
- methods(6): OnAwake, OnDestroy, OnVesselRecovered, Start, checkLaunchSites, recoverVesselCrew
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/VesselRetrieval.cs
- 93 lines; kind=MonoBehaviour; type=VesselRetrieval; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.SceneManagement
- unity: SceneManager=3, Lifecycle=2, Coroutine=2, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1; unity_score=61
- ksp refs: Vessel=7, HighLogic=4, GameEvents=3, FlightGlobals=2; fan_in=0 fan_out=7
- methods(8): Awake, OnDestroy, OnLevelLoaded, OnSceneLoaded, getVesselsToRecover, onVesselRecoveryRequested, recoverVessel, recoverVessels
- auto suggestion: tier=B subsystem=Vessel

## Assembly-CSharp/VesselSpawnSearch.cs
- 223 lines; kind=MonoBehaviour; type=VesselSpawnSearch; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, KSP.UI.Screens, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=8, GetComponent=7, GameObject=6, Transform=4, Lifecycle=4, SetActive=3; unity_score=81
- ksp refs: InputLockManager=2; fan_in=1 fan_out=6
- methods(15): AdjustSearchInputSize, Awake, HasFiltered, ItemMatchesSearch, OnDestroy, OnDisable, SearchField_OnClick, SearchField_OnEndEdit, SearchField_OnValueChange, SearchRestart, SearchRoutine, SearchStart, SearchStop, SteamCraftListReceived, Update
- auto suggestion: tier=C subsystem=Vessel

## Assembly-CSharp/VesselSwitching.cs
- 130 lines; kind=MonoBehaviour; type=VesselSwitching; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=40
- ksp refs: FlightGlobals=9, HighLogic=5, Vessel=4, GameSettings=4, Localizer=3, MapView=1; fan_in=0 fan_out=15
- methods(3): Awake, OnDestroy, Update
- auto suggestion: tier=B subsystem=Vessel

## Assembly-CSharp/VesselTargetModes.cs
- 7 lines; kind=enum; type=VesselTargetModes; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=9 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/VesselTripLog.cs
- 47 lines; kind=class; type=VesselTripLog; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Vessel=1, ProtoVessel=1, ProtoPartSnapshot=1, ProtoPartModuleSnapshot=1, Part=1; fan_in=5 fan_out=7
- methods(2): FromProtoVessel, FromVessel
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/VesselTutorialStep.cs
- 194 lines; kind=class; type=VesselTutorialStep; bases=IntermediateTutorialPageStep; ancestry=IntermediateTutorialPageStep/TutorialPageConfig; ns=(global)
- usings: Expansions.Missions, Expansions.Missions.Editor, UnityEngine
- unity: GameObject=12; unity_score=33
- ksp refs: GameEvents=2; fan_in=1 fan_out=13
- methods(17): AddTutorialStepConfig, CanHighlightVesselLanded, FilterVesselLandedNodeCriteria, IsDialogLinked, IsPlanetSetted, OnEnterDragAndLinkVesselNode, OnEnterSelectKerbinPlanet, OnEnterSelectKerbinPlanetHelper, OnLeaveDragAndLinkVesselNode, OnLeaveSelectKerbinPlanet, OnLeaveSelectKerbinPlanetHelper, OnNodeFocused, OnSelectedGameObjectChange, OnSelectedGameObjectChangeSetPlanet, OnUpdateLinkVesselNode, OnUpdateSelectKerbinPlanet, OnUpdateSelectKerbinPlanetHelper
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/VesselType.cs
- 39 lines; kind=enum; type=VesselType; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=2
- ksp refs: none; fan_in=70 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/VesselValues.cs
- 144 lines; kind=class; type=VesselValues; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Time.=2; unity_score=5
- ksp refs: Part=19, Vessel=5; fan_in=11 fan_out=3
- methods(1): ResetValueCache
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/VFXSequencer.cs
- 146 lines; kind=MonoBehaviour; type=VFXSequencer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=6, Coroutine=6, Time.=3, Audio=2, MonoBehaviour=1, Lifecycle=1; unity_score=73
- ksp refs: GameSettings=1; fan_in=1 fan_out=1
- methods(5): GetFXDuration, Play, Start, getSubFXHierarchyDuration, playSequence
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/VideoSettings.cs
- 642 lines; kind=class; type=VideoSettings; bases=DialogGUIVerticalLayout,ISettings; ancestry=DialogGUIVerticalLayout/DialogGUILayoutBase/DialogGUIBase; ns=(global)
- usings: CommNet, Highlighting, KSP.Localization, UnityEngine
- unity: QualitySettings/SystemInfo=10, Screen/Cursor=2, Application.=2, Texture=1, SetActive=1; unity_score=14
- ksp refs: GameSettings=65, Localizer=56, HighLogic=1; fan_in=1 fan_out=19
- methods(7): ApplySettings, DrawMiniSettings, DrawSettings, GetAvailableResolutions, GetName, GetSettings, OnUpdate
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/VolumeController.cs
- 10 lines; kind=MonoBehaviour; type=VolumeController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GetComponent=2, Audio=2, MonoBehaviour=1, Lifecycle=1; unity_score=81
- ksp refs: GameSettings=1; fan_in=0 fan_out=1
- methods(1): Start
- auto suggestion: tier=C subsystem=Audio

## Assembly-CSharp/VolumeNormalizer.cs
- 49 lines; kind=MonoBehaviour; type=VolumeNormalizer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Audio=5, Time.=2, Lifecycle=2, MonoBehaviour=1; unity_score=81
- ksp refs: GameSettings=6; fan_in=0 fan_out=1
- methods(2): Awake, Update
- auto suggestion: tier=C subsystem=Audio

## Assembly-CSharp/VolumetricObjectBase.cs
- 237 lines; kind=MonoBehaviour; type=VolumetricObjectBase; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Mesh=12, Material=5, Lifecycle=4, Instantiate/Destroy=4, Camera=4, Renderer=3; unity_score=81
- ksp refs: none; fan_in=4 fan_out=0
- methods(14): CleanUp, CopyMesh, CreateCube, HasChanged, LateUpdate, OnDestroy, OnDisable, OnEnable, PopulateShaderName, ScaleMesh, ScaleVector, SetChangedValues, SetupUnitVerts, UpdateVolume
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/WarpTransition.cs
- 23 lines; kind=class; type=WarpTransition; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/WarpTransitionCalculator.cs
- 123 lines; kind=static class; type=WarpTransitionCalculator; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: TimeWarp=13; fan_in=2 fan_out=2
- methods(4): CalcWarpRateTransitions, CheckForTransitionChanges, SafeRateToUTPeriod, UTToRateDown
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/WeldFX.cs
- 252 lines; kind=MonoBehaviour; type=WeldFX; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=30, Coroutine=12, ParticleSystem=4, Lifecycle=3, Time.=2, Renderer=2; unity_score=82
- ksp refs: GameEvents=2, KerbalEVA=1, GameSettings=1; fan_in=2 fan_out=5
- methods(16): AdjustWeldingTransform, Awake, DisableMesh, EnableMesh, FadeInLight, FadeOutLight, GetAttachPoint, Play, PlaySequence, Reset, SetSparksGravity, Start, Stop, ToggleMesh, Update, UpdateGunAiming
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/WhackAKerbal.cs
- 108 lines; kind=MonoBehaviour; type=WhackAKerbal; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.Rendering
- unity: IMGUI=19, GameObject=14, GetComponent=6, Input.=5, UnityEngine.UI=3, Transform=3; unity_score=100
- ksp refs: InputLockManager=3; fan_in=1 fan_out=3
- methods(5): OnDestroy, OnGUI, Start, Update, drawProjectileOptions
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/WhatsNewDialog.cs
- 254 lines; kind=MonoBehaviour; type=WhatsNewDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=6, GameObject=5, TMPro=3, SetActive=3, Lifecycle=3, KeyCode=3; unity_score=73
- ksp refs: Localizer=7, GameSettings=7, InputLockManager=2; fan_in=1 fan_out=12
- methods(11): Create, Dismiss, GetChangeLogText, OnCloseButton, OnDestroy, OnDontShowAgainToggle, OnMerchButton, OnMoreInfoButton, ParseReadMeFile, Start, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/WhatsNewModes.cs
- 5 lines; kind=enum; type=WhatsNewModes; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/Wheel.cs
- 159 lines; kind=class; type=Wheel; bases=IConfigNode; ancestry=Block; ns=(global)
- usings: UnityEngine
- unity: Transform=6, SetActive=6, GameObject=6, WheelCollider=2, Raycast/Query=2, Physics.=1; unity_score=59
- ksp refs: ConfigNode=2, TimeWarp=1; fan_in=0 fan_out=4
- methods(5): Load, Save, Update, damageWheel, repairWheel
- auto suggestion: tier=B subsystem=PartModules-Wheels

## Assembly-CSharp/WheelType.cs
- 6 lines; kind=enum; type=WheelType; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Wheels

## Assembly-CSharp/WindmillAnimScript.cs
- 50 lines; kind=MonoBehaviour; type=WindmillAnimScript; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=3, Transform=1, MonoBehaviour=1; unity_score=61
- ksp refs: GameEvents=4; fan_in=0 fan_out=3
- methods(5): OnDestroy, Start, Update, onPause, onUnPause
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/XButton.cs
- 118 lines; kind=MonoBehaviour; type=XButton; bases=Button; ancestry=Button; ns=(global)
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: EventSystems=12, GameObject=2, UnityEngine.UI=1, Lifecycle=1, Instantiate/Destroy=1, GetComponent=1; unity_score=77
- ksp refs: none; fan_in=0 fan_out=1
- methods(10): Awake, Create, OnDeselect, OnMove, OnPointerDown, OnPointerEnter, OnPointerExit, OnPointerUp, OnSelect, Terminate
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/XGraphic.cs
- 12 lines; kind=UIBehaviour; type=XGraphic; bases=Graphic; ancestry=Graphic; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1; unity_score=44
- ksp refs: none; fan_in=1 fan_out=0
- methods(1): OnFillVBO
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/XHoverable.cs
- 40 lines; kind=UIBehaviour; type=XHoverable; bases=UIBehaviour,IEventSystemHandler,IPointerEnterHandler,IPointerExitHandler; ancestry=UIBehaviour; ns=(global)
- usings: UnityEngine.EventSystems
- unity: EventSystems=7, Lifecycle=1; unity_score=85
- ksp refs: none; fan_in=1 fan_out=0
- methods(4): Awake, Clear, OnPointerEnter, OnPointerExit
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/XKCDColors.cs **MUST-OPEN (>800 lines)**
- 3871 lines; kind=static class; type=XKCDColors; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=33
- ksp refs: none; fan_in=81 fan_out=0
- methods(4): A, FromHtml, ToHex, ToHexA
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/XSelectable.cs
- 104 lines; kind=UIBehaviour; type=XSelectable; bases=UIBehaviour,IEventSystemHandler,IPointerEnterHandler,IPointerExitHandler,IPointerDownHandler,IPointerUpHandler,ISelectHandler,IDeselectHandler,IMoveHandler; ancestry=UIBehaviour; ns=(global)
- usings: UnityEngine.EventSystems
- unity: EventSystems=19, Lifecycle=1; unity_score=84
- ksp refs: none; fan_in=10 fan_out=0
- methods(9): Awake, Clear, OnDeselect, OnMove, OnPointerDown, OnPointerEnter, OnPointerExit, OnPointerUp, OnSelect
- auto suggestion: tier=C subsystem=UI
