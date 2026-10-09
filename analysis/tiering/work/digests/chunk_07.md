# Review chunk 07 (KSP): 263 files, 41390 lines

## Assembly-CSharp/KSP/UI/Screens/Flight/METDisplay.cs
- 103 lines; kind=MonoBehaviour; type=METDisplay; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: KSP.Localization, TMPro, UnityEngine
- unity: Time.=4, TMPro=3, Lifecycle=3, MonoBehaviour=1, GetComponent=1; unity_score=63
- ksp refs: GameEvents=4, Planetarium=1, Localizer=1, FlightGlobals=1; fan_in=1 fan_out=7
- methods(9): CacheLocalStrings, LateUpdate, OnDestroy, Reset, SetTimeMode, Start, ToggleTimeMode, onGamePause, onGameUnPause
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/NavBall.cs
- 248 lines; kind=MonoBehaviour; type=NavBall; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: SetActive=24, GameObject=24, Transform=17, Texture=4, UnityEngine.UI=2, Lifecycle=2; unity_score=90
- ksp refs: FlightGlobals=29, FlightCamera=4, Vessel=2, Orbit=2, GameDatabase=2, Vector3d=1; fan_in=3 fan_out=10
- methods(6): ClearWaypoint, DrawOrbitalCues, SetVectorAlphaTint, SetWaypoint, Start, Update
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/NavBallBurnVector.cs
- 645 lines; kind=MonoBehaviour; type=NavBallBurnVector; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: KSP.Localization, SoftMasking, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=39, SetActive=38, Transform=22, UnityEngine.UI=6, TMPro=6, Lifecycle=4; unity_score=77
- ksp refs: FlightGlobals=23, GameSettings=22, Localizer=11, Planetarium=7, GameEvents=4, InputLockManager=3; fan_in=1 fan_out=23
- methods(20): AutoStepNodes, Awake, CacheLocalStrings, CalculateBurnTime, ClearStageMarkers, LateUpdate, OnDestroy, OnDismissButton, OnVesselSwitch, OnWarpToButton, RemoveRemainingStageMarkers, SetAdvancedMode, Start, UpdateBurnUIText, UpdateDVStageMarkers, UpdateNavballBurnVectors, btnPercentMinusClick, btnPercentPlusClick, navBallIsExpanded, onGameSettingsApplied
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/NavBallToggle.cs
- 136 lines; kind=MonoBehaviour; type=NavBallToggle; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine, UnityEngine.UI
- unity: Lifecycle=3, UnityEngine.UI=1, MonoBehaviour=1, IMGUI=1; unity_score=47
- ksp refs: InputLockManager=7, MapView=3, GameEvents=3, GameSettings=2; fan_in=3 fan_out=8
- methods(9): Awake, DisableManeuverMode, EnableManeuverMode, OnDestroy, OnMapEntered, OnMapExited, OnNavBallToggle, TogglePanel, Update
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/RCSDisplay.cs
- 44 lines; kind=MonoBehaviour; type=RCSDisplay; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine
- unity: Lifecycle=2, GetComponent=2, MonoBehaviour=1; unity_score=63
- ksp refs: FlightGlobals=4, KerbalEVA=1; fan_in=0 fan_out=5
- methods(4): LateUpdate, Reset, SetNewState, Start
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/SASDisplay.cs
- 63 lines; kind=MonoBehaviour; type=SASDisplay; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine
- unity: Lifecycle=2, GetComponent=2, MonoBehaviour=1; unity_score=54
- ksp refs: FlightGlobals=2, Vessel=1, GameSettings=1; fan_in=0 fan_out=6
- methods(4): LateUpdate, Reset, SetNewState, Start
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/SpeedDisplay.cs
- 81 lines; kind=MonoBehaviour; type=SpeedDisplay; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: Lifecycle=4, TMPro=2, UnityEngine.UI=1, MonoBehaviour=1; unity_score=63
- ksp refs: FlightGlobals=8, GameEvents=2, Orbit=1, Localizer=1; fan_in=0 fan_out=8
- methods(6): Awake, LateUpdate, OnChangeMode, OnDestroy, OnSetSpeedMode, Start
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/StageTumbler.cs
- 32 lines; kind=MonoBehaviour; type=StageTumbler; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine
- unity: Transform=3, Lifecycle=2, MonoBehaviour=1, GetComponent=1; unity_score=79
- ksp refs: GameSettings=3, FlightGlobals=1; fan_in=0 fan_out=3
- methods(4): Awake, LateUpdate, Reset, SetTumblerPosition
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/StagingLED.cs
- 50 lines; kind=MonoBehaviour; type=StagingLED; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine
- unity: Shader=2, MonoBehaviour=1, Lifecycle=1; unity_score=59
- ksp refs: InputLockManager=1, FlightGlobals=1; fan_in=0 fan_out=4
- methods(1): LateUpdate
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/TelemetryUpdate.cs
- 356 lines; kind=MonoBehaviour; type=TelemetryUpdate; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: CommNet, KSP.Localization, KSP.UI.TooltipTypes, UnityEngine, UnityEngine.UI
- unity: Texture=25, SetActive=9, GameObject=9, UnityEngine.UI=6, Lifecycle=4, Time.=1; unity_score=76
- ksp refs: Localizer=18, Vessel=2, MapView=1, FlightGlobals=1; fan_in=1 fan_out=13
- methods(10): Awake, CacheLocalStrings, ClearGui, HideModeButton, OnDestroy, SetEVAUI, SetIcon, ShowModeButton, Start, Update
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/TemperatureGauge.cs
- 179 lines; kind=MonoBehaviour; type=TemperatureGauge; bases=MonoBehaviour,IComparable<TemperatureGauge>; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: KSP.UI.Util, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, GameObject=4, SetActive=3, Lifecycle=2, Transform=1, MonoBehaviour=1; unity_score=63
- ksp refs: Part=5, GameSettings=2, FlightCamera=2; fan_in=1 fan_out=6
- methods(5): Awake, CompareTo, GaugeUpdate, OnDestroy, Setup
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/TemperatureGaugeSystem.cs
- 248 lines; kind=MonoBehaviour; type=TemperatureGaugeSystem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine
- unity: Instantiate/Destroy=5, Transform=4, Lifecycle=3, GameObject=3, MonoBehaviour=1; unity_score=49
- ksp refs: FlightGlobals=8, Part=6, PhysicsGlobals=4, Vessel=1, HighLogic=1, GameSettings=1; fan_in=1 fan_out=8
- methods(8): Awake, CreateGauges, DestroyGauges, OnDestroy, RebuildGaugeList, TestRebuild, TestRecreate, Update
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/ThrottleGauge.cs
- 30 lines; kind=MonoBehaviour; type=ThrottleGauge; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine
- unity: Time.=1, MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=64
- ksp refs: FlightGlobals=4, KerbalEVA=1; fan_in=0 fan_out=3
- methods(2): LateUpdate, Reset
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/VerticalSpeedGauge.cs
- 46 lines; kind=MonoBehaviour; type=VerticalSpeedGauge; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine
- unity: Lifecycle=2, Shader=1, MonoBehaviour=1, GetComponent=1; unity_score=64
- ksp refs: FlightGlobals=4; fan_in=0 fan_out=2
- methods(3): LateUpdate, Reset, Start
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/HelixGauge.cs
- 174 lines; kind=MonoBehaviour; type=HelixGauge; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=15, Transform=5, Lifecycle=3, MonoBehaviour=1; unity_score=76
- ksp refs: GameSettings=3; fan_in=0 fan_out=1
- methods(10): Awake, GetAngle, GetAngle180, GetAngle360, GetAngle90, GetReadoutWorldPos, LateUpdate, Start, UpdateGauge, UpdateReadoutField
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbApp.cs
- 146 lines; kind=MonoBehaviour; type=KbApp; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: Lifecycle=3, GameObject=3, SetActive=2, Instantiate/Destroy=2, Texture=1, MonoBehaviour=1; unity_score=57
- ksp refs: none; fan_in=7 fan_out=6
- methods(16): ActivateApp, Awake, DisablePanel, DisplayApp, EnablePanel, Hide, HideApp, Hover, HoverOut, OnDestroy, Restore, Setup, Show, Start, displayApp, hideApp
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbApp_Null.cs
- 17 lines; kind=MonoBehaviour; type=KbApp_Null; bases=KbApp; ancestry=KbApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=2
- methods(3): ActivateApp, DisplayApp, HideApp
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbApp_PlanetInfo.cs
- 35 lines; kind=MonoBehaviour; type=KbApp_PlanetInfo; bases=KbApp; ancestry=KbApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine
- unity: GameObject=5, TMPro=2, GetComponent=2, Instantiate/Destroy=1; unity_score=87
- ksp refs: Localizer=1, CelestialBody=1; fan_in=0 fan_out=7
- methods(3): ActivateApp, DisplayApp, HideApp
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbApp_PlanetParameters.cs
- 89 lines; kind=MonoBehaviour; type=KbApp_PlanetParameters; bases=KbApp; ancestry=KbApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, UnityEngine
- unity: Transform=2, Instantiate/Destroy=1; unity_score=37
- ksp refs: Localizer=26, PhysicsGlobals=1, CelestialBody=1; fan_in=0 fan_out=8
- methods(6): ActivateApp, CreateAtmosphericCharacteristics, CreatePhysicalCharacteristics, DisplayApp, HideApp, boolDelegate_KbApp_PlanetParameters
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbApp_PlanetResources.cs
- 524 lines; kind=MonoBehaviour; type=KbApp_PlanetResources; bases=KbApp; ancestry=KbApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems
- unity: GameObject=13, EventSystems=11, Instantiate/Destroy=8, Transform=6, GetComponent=6, SetActive=4; unity_score=64
- ksp refs: Localizer=3, PartResourceDefinition=1, CelestialBody=1; fan_in=0 fan_out=24
- methods(26): ActivateApp, AddResourceDataItem, CreateResourceList, DisplayApp, DisposeResourceList, HideApp, InitHeaderAndFooter, MouseInputResCtrlMinus, MouseInputResCtrlPlus, OnResourcesCtrlColorChange, OnResourcesCtrlStyleChange, OnSort, OnToggleAtmos, OnToggleOcean, OnToggleSurface, ResourceListAndPiechart_select, SetKBDisplay, SetResourceListAppearance, SetSliceOver, SetSliceOverOff, onResourcesListItem_Click, onResourcesListItem_Enter, onResourcesListItem_Exit, onSliceExit, onSliceOver, onSliceTap
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbApp_UnownedInfo.cs
- 139 lines; kind=MonoBehaviour; type=KbApp_UnownedInfo; bases=KbAppUnowned; ancestry=KbAppUnowned/KbApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=10, GameObject=7, Instantiate/Destroy=3, GetComponent=3, Transform=2, Time.=2; unity_score=82
- ksp refs: Localizer=6, Vessel=2, Planetarium=2, MapView=1; fan_in=1 fan_out=17
- methods(6): ActivateApp, CacheLocalStrings, CreateUnownedTrackingInfoList, FixedUpdate, UpdateNowQuestionmark, UpdateTrackingInfoList
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbApp_VesselCrew.cs
- 144 lines; kind=MonoBehaviour; type=KbApp_VesselCrew; bases=KbAppVessel; ancestry=KbAppVessel/KbApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, UnityEngine, UnityEngine.UI
- unity: Instantiate/Destroy=3, Transform=2, GameObject=2, UnityEngine.UI=1; unity_score=48
- ksp refs: ProtoCrewMember=12, AvailablePart=5, Vessel=2, PartLoader=1, Localizer=1; fan_in=0 fan_out=14
- methods(6): ActivateApp, CompareKVP, CompareSeatIdx, CreateVesselCrewList, SetupKerbalItem, boolDelegate_KbApp_VesselCrew
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbApp_VesselInfo.cs
- 331 lines; kind=MonoBehaviour; type=KbApp_VesselInfo; bases=KbAppVessel; ancestry=KbAppVessel/KbApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: CommNet, KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=16, Instantiate/Destroy=3, UnityEngine.UI=2, Transform=2, Time.=2, GameObject=2; unity_score=62
- ksp refs: Localizer=31, Vessel=9, HighLogic=2, ProtoPartSnapshot=1, MapView=1, GameDatabase=1; fan_in=1 fan_out=26
- methods(9): ActivateApp, CacheLocalStrings, CreateVesselInfoBox, CreateVesselInfoList, FixedUpdate, UpdateCommNet, UpdateNowQuestionmark, UpdateVesselInfoList, boolDelegate_KbApp_VesselInfo
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbAppFrame.cs
- 78 lines; kind=MonoBehaviour; type=KbAppFrame; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=9, Transform=5, TMPro=3, Lifecycle=2, GetComponent=2, SetActive=1; unity_score=91
- ksp refs: GameSettings=2, Localizer=1; fan_in=1 fan_out=7
- methods(4): Awake, OnDestroy, Reposition, Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbAppUnowned.cs
- 72 lines; kind=MonoBehaviour; type=KbAppUnowned; bases=KbApp; ancestry=KbApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine.EventSystems
- unity: Lifecycle=2, EventSystems=1; unity_score=43
- ksp refs: GameEvents=9, Vessel=7, InputLockManager=2; fan_in=1 fan_out=9
- methods(9): Awake, DisplayApp, HideApp, OnDestroy, OnInputLocksChange, OnKnowledgeChange, OnPanelHeaderTap, Setup, onVesselRename
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbAppVessel.cs
- 62 lines; kind=MonoBehaviour; type=KbAppVessel; bases=KbApp; ancestry=KbApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine.EventSystems
- unity: Lifecycle=2, EventSystems=1; unity_score=46
- ksp refs: GameEvents=6, Vessel=2, InputLockManager=2; fan_in=2 fan_out=7
- methods(8): Awake, DisplayApp, HideApp, OnDestroy, OnInputLocksChange, OnPanelHeaderTap, OnVesselRename, Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbItem.cs
- 9 lines; kind=MonoBehaviour; type=KbItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=36
- ksp refs: none; fan_in=7 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbItem_flagImage.cs
- 9 lines; kind=MonoBehaviour; type=KbItem_flagImage; bases=KbItem; ancestry=KbItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine.UI
- unity: UnityEngine.UI=1; unity_score=44
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbItem_kerbalInfo.cs
- 18 lines; kind=MonoBehaviour; type=KbItem_kerbalInfo; bases=KbItem; ancestry=KbItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine.UI
- unity: TMPro=3, UnityEngine.UI=1; unity_score=77
- ksp refs: none; fan_in=1 fan_out=3
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbItem_resourceFooter.cs
- 23 lines; kind=MonoBehaviour; type=KbItem_resourceFooter; bases=KbItem; ancestry=KbItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, TMPro=2; unity_score=82
- ksp refs: none; fan_in=1 fan_out=3
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbItem_resourceHeader.cs
- 15 lines; kind=MonoBehaviour; type=KbItem_resourceHeader; bases=KbItem; ancestry=KbItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine.UI
- unity: UnityEngine.UI=3; unity_score=70
- ksp refs: none; fan_in=1 fan_out=2
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbItem_resourceItem.cs
- 15 lines; kind=MonoBehaviour; type=KbItem_resourceItem; bases=KbItem; ancestry=KbItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro
- unity: TMPro=2; unity_score=59
- ksp refs: none; fan_in=1 fan_out=4
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbItem_unownedInfo.cs
- 24 lines; kind=MonoBehaviour; type=KbItem_unownedInfo; bases=KbItem; ancestry=KbItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine.UI
- unity: TMPro=6, UnityEngine.UI=1; unity_score=89
- ksp refs: none; fan_in=1 fan_out=3
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KbItem_vesselInfo.cs
- 19 lines; kind=MonoBehaviour; type=KbItem_vesselInfo; bases=KbItem; ancestry=KbItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro
- unity: TMPro=5; unity_score=82
- ksp refs: none; fan_in=1 fan_out=3
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KerbalListItem.cs
- 66 lines; kind=MonoBehaviour; type=KerbalListItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.UI.TooltipTypes, TMPro, UnityEngine, UnityEngine.UI
- unity: Instantiate/Destroy=4, Transform=3, Texture=3, Light=3, UnityEngine.UI=2, GetComponent=2; unity_score=94
- ksp refs: none; fan_in=1 fan_out=2
- methods(2): Initialize, OnDestroy
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KnowledgeBase.cs
- 253 lines; kind=MonoBehaviour; type=KnowledgeBase; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: Transform=7, GameObject=7, SetActive=6, Instantiate/Destroy=3, Lifecycle=2, Time.=1; unity_score=62
- ksp refs: MapView=9, GameEvents=7, Vessel=5, ProtoCrewMember=5, AvailablePart=5, CelestialBody=3; fan_in=4 fan_out=19
- methods(10): ActivateApps, Awake, Cleanup, GetUnloadedVesselMass, GetVesselCrewByAvailablePart, Hide, OnAppLauncherReady, OnDestroy, OnMapFocusChange, Show
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KSCFacilityContextMenu.cs
- 408 lines; kind=MonoBehaviour; type=KSCFacilityContextMenu; bases=AnchoredDialog; ancestry=AnchoredDialog/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=13, SetActive=12, TMPro=8, UnityEngine.UI=6, Transform=4, GetComponent=2; unity_score=71
- ksp refs: Localizer=14, GameEvents=12, Funding=5, HighLogic=2, Game=2, InputLockManager=1; fan_in=2 fan_out=20
- methods(19): Create, CreateWindowContent, Dismiss, GetWindowTitle, OnClickOut, OnDemolishButtonInput, OnDestroyThis, OnDowngradeButtonInput, OnEnterBtnInput, OnFacilityValuesModified, OnGameSceneLoadRequested, OnKSCStructureEvent, OnRepairButtonInput, OnUpgradeButtonHoverIn, OnUpgradeButtonHoverOut, OnUpgradeButtonInput, PreviewNextLevel, StartThis, getFundsTextColorTag
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KSCVesselMarker.cs
- 331 lines; kind=MonoBehaviour; type=KSCVesselMarker; bases=AnchoredDialog; ancestry=AnchoredDialog/MonoBehaviour; ns=KSP.UI.Screens
- usings: Expansions.Missions, KSP.Localization, KSP.UI.TooltipTypes, TMPro, UnityEngine, UnityEngine.UI
- unity: SetActive=9, GameObject=9, UnityEngine.UI=6, TMPro=6, GetComponent=4, Coroutine=3; unity_score=71
- ksp refs: Vessel=5, Localizer=5, InputLockManager=4, GameEvents=3, ProtoCrewMember=2, HighLogic=2; fan_in=1 fan_out=16
- methods(17): ClearCtrls, Collapse, Create, CreateWindowContent, Dismiss, Expand, GetWindowTitle, OnClickOut, OnDestroyThis, OnFlyButtonInput, OnLateUpdate, OnMarkerButtonInput, OnPanelSetupComplete, OnRecoverButtonInput, StartThis, makeShipname, onInputLocksModified
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KSCVesselMarkers.cs
- 143 lines; kind=MonoBehaviour; type=KSCVesselMarkers; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: Coroutine=4, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=46
- ksp refs: GameEvents=21, Vessel=5, HighLogic=5, FlightGlobals=3, Planetarium=1; fan_in=5 fan_out=14
- methods(14): Awake, ClearVesselMarkers, FlyVessel, OnDestroy, OnMarkerDismiss, RecoverVessel, RefreshMarkers, SpawnVesselMarkers, onGUIACScreenDespawn, onGUIScreenDespawn, onGUIScreenSpawn, onGUIVesselScreenSpawn, onSceneLoadRequested, onVesselDestroy
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/KSPediaApp.cs
- 41 lines; kind=MonoBehaviour; type=KSPediaApp; bases=UIApp; ancestry=UIApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSPAssets.KSPedia, UnityEngine
- unity: Transform=1; unity_score=36
- ksp refs: none; fan_in=0 fan_out=4
- methods(7): DisplayApp, GetAppScenes, GetAppScreenPos, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/LED.cs
- 122 lines; kind=MonoBehaviour; type=LED; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.UI
- unity: Coroutine=3, UnityEngine.UI=2, Time.=1, Texture=1, Shader=1, MonoBehaviour=1; unity_score=60
- ksp refs: none; fan_in=0 fan_out=0
- methods(7): Awake, Blink, BlinkCoroutine, Reset, SetColor, SetOn, setOff
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/LinearGauge.cs
- 95 lines; kind=MonoBehaviour; type=LinearGauge; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: UnityEngine.UI=2, Lifecycle=2, Time.=1, MonoBehaviour=1, GetComponent=1; unity_score=61
- ksp refs: none; fan_in=0 fan_out=0
- methods(4): Awake, LateUpdate, Reset, SetValue
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenu.cs
- 151 lines; kind=class; type=MapContextMenu; bases=-; ancestry=-; ns=KSP.UI.Screens.Mapview
- usings: KSP.Localization, KSP.UI.Screens.Mapview.MapContextMenuOptions, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Coroutine=6, UnityEngine.UI=3, Input.=1, GetComponent=1, EventSystems=1; unity_score=45
- ksp refs: MapView=4, Localizer=2, InputLockManager=2, GameSettings=1; fan_in=2 fan_out=11
- methods(7): Create, DialogUpdate, Dismiss, SetupTransform, UpdatePosition, dialogUpdateCoroutine, onDialogDismiss
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenuOptions/AddManeuver.cs
- 30 lines; kind=class; type=AddManeuver; bases=MapContextMenuOption; ancestry=MapContextMenuOption/DialogGUIButton/DialogGUIBase; ns=KSP.UI.Screens.Mapview.MapContextMenuOptions
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1, InputLockManager=1; fan_in=1 fan_out=7
- methods(1): OnSelect
- auto suggestion: tier=A subsystem=MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenuOptions/AutoWarpToUT.cs
- 62 lines; kind=class; type=AutoWarpToUT; bases=MapContextMenuOption; ancestry=MapContextMenuOption/DialogGUIButton/DialogGUIBase; ns=KSP.UI.Screens.Mapview.MapContextMenuOptions
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=3, GameSettings=2, TimeWarp=1, Planetarium=1; fan_in=1 fan_out=11
- methods(3): CheckAvailable, OnCheckEnabled, OnSelect
- auto suggestion: tier=A subsystem=MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenuOptions/DeleteCustomWaypoint.cs
- 48 lines; kind=class; type=DeleteCustomWaypoint; bases=MapContextMenuOption; ancestry=MapContextMenuOption/DialogGUIButton/DialogGUIBase; ns=KSP.UI.Screens.Mapview.MapContextMenuOptions
- usings: FinePrint, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=2, HighLogic=1; fan_in=1 fan_out=8
- methods(3): CheckAvailable, OnCheckEnabled, OnSelect
- auto suggestion: tier=A subsystem=MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenuOptions/FocusObject.cs
- 108 lines; kind=class; type=FocusObject; bases=MapContextMenuOption; ancestry=MapContextMenuOption/DialogGUIButton/DialogGUIBase; ns=KSP.UI.Screens.Mapview.MapContextMenuOptions
- usings: KSP.Localization, PreFlightTests
- unity: none; unity_score=0
- ksp refs: CelestialBody=5, Localizer=4, MapView=2, InputLockManager=2, Vessel=1, OrbitDriver=1; fan_in=1 fan_out=19
- methods(7): CheckAvailable, GetMode, OnCheckEnabled, OnSelect, TryViewInTrackingStation, onTrackingStationDismiss, onTrackingStationProceed
- auto suggestion: tier=A subsystem=MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenuOptions/MapContextMenuOption.cs
- 41 lines; kind=class; type=MapContextMenuOption; bases=DialogGUIButton; ancestry=DialogGUIButton/DialogGUIBase; ns=KSP.UI.Screens.Mapview.MapContextMenuOptions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=8 fan_out=1
- methods(4): CheckAvailable, CheckEnabled, OnCheckEnabled, OnSelect
- auto suggestion: tier=A subsystem=MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenuOptions/SetAsTarget.cs
- 60 lines; kind=class; type=SetAsTarget; bases=MapContextMenuOption; ancestry=MapContextMenuOption/DialogGUIButton/DialogGUIBase; ns=KSP.UI.Screens.Mapview.MapContextMenuOptions
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=2, FlightGlobals=2; fan_in=1 fan_out=6
- methods(3): CheckAvailable, OnCheckEnabled, OnSelect
- auto suggestion: tier=A subsystem=MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenuOptions/WaypointNavigation.cs
- 81 lines; kind=class; type=WaypointNavigation; bases=MapContextMenuOption; ancestry=MapContextMenuOption/DialogGUIButton/DialogGUIBase; ns=KSP.UI.Screens.Mapview.MapContextMenuOptions
- usings: FinePrint, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=6, HighLogic=1; fan_in=1 fan_out=7
- methods(3): CheckAvailable, OnCheckEnabled, OnSelect
- auto suggestion: tier=A subsystem=MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapNode.cs **MUST-OPEN (>800 lines)**
- 958 lines; kind=UIBehaviour; type=MapNode; bases=Selectable; ancestry=Selectable; ns=KSP.UI.Screens.Mapview
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=17, SetActive=13, UnityEngine.UI=8, Transform=8, EventSystems=7, TMPro=4; unity_score=63
- ksp refs: Vector3d=14, InputLockManager=5, Localizer=4, MapView=2, GameEvents=2, Vessel=1; fan_in=20 fan_out=23
- methods(30): AttachLabel, Awake, CacheLocalStrings, CaptionUpdate, CheckAndEnablePinnedCaption, Create, FormatCaption, GetIconIndex, HideAddAlarm, Init, MoveAwayFromEachOthers, NodeUpdate, OnAddAlarmPointerExit, OnDeselect, OnDestroy, OnEnable, OnMapEntered, OnPointerDown, OnPointerEnter, OnPointerExit, OnPointerUp, OnSelect, SetBackground, SetIcon, SetType, Terminate, UpdateCaptionText, UpdateCursorInput, UpdateHoverCaption, UpdateType
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapViewCanvasUtil.cs
- 137 lines; kind=MonoBehaviour; type=MapViewCanvasUtil; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Mapview
- usings: UnityEngine, UnityEngine.UI
- unity: Transform=10, UnityEngine.UI=9, Camera=4, GetComponent=3, LayerMask=2, GameObject=2; unity_score=84
- ksp refs: Vector3d=2; fan_in=6 fan_out=4
- methods(5): GetNodeCanvasContainer, OnUIScaleChange, ResetNodeCanvasContainer, ResetNodeCanvasContainerScale, ScaledToUISpacePos
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/KSP/UI/Screens/MCListItem.cs
- 45 lines; kind=MonoBehaviour; type=MCListItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: Contracts, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, TMPro=1, MonoBehaviour=1, Lifecycle=1; unity_score=62
- ksp refs: Contract=4; fan_in=1 fan_out=5
- methods(2): Awake, Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/MEPartCategoryButton.cs
- 101 lines; kind=MonoBehaviour; type=MEPartCategoryButton; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=5, EventSystems=4, Texture=2, Lifecycle=2, Input.=2, GetComponent=2; unity_score=84
- ksp refs: none; fan_in=2 fan_out=1
- methods(10): Awake, Lock, OnPointerEnter, OnPointerExit, SetCount, SetState, ShowCount, Unlock, Update, UpdateState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/MessageSystem.cs **MUST-OPEN (>800 lines)**
- 820 lines; kind=MonoBehaviour; type=MessageSystem; bases=UIApp; ancestry=UIApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems
- unity: Transform=21, GameObject=14, Instantiate/Destroy=8, SetActive=7, TMPro=5, UnityEngine.UI=4; unity_score=60
- ksp refs: GameEvents=11, ConfigNode=9, Localizer=7, GameSettings=6, HighLogic=5, Game=1; fan_in=9 fan_out=24
- methods(44): AddMessage, AddMessageInternal, AddMessageInternalNonDuplicate, Awake, ClearAllMessages, DeleteAllMessages, DiscardMessage, DiscardSelected, DisplayApp, Equals, FindMessages, GetAppScenes, GetAppScreenPos, GetHashCode, GetRichTextMessageTitle, HasNextMessage, HasPreviousMessage, HideApp, Instantiate, InstantiateAdvancedItem, InstantiatePopup, LoadMessage, LoadMessages, MouseInputAdvMessageItem, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnClick, OnDeleteAllMessagesClicked, OnRightClick, OnSceneLoadRequested, OnSceneLoaded, Reposition, RepositionDynamic, Resize, SaveMessage, SaveMessages, ShowMessage, ShowNextMessage, ShowPreviousMessage, UpdateLauncherButtonCount, UpdateLauncherButtonPlayAnim, UpdateLauncherButtonStopAnim, setupMode
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/MessageSystemButton.cs
- 158 lines; kind=MonoBehaviour; type=MessageSystemButton; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.Events, UnityEngine.UI
- unity: Texture=8, Shader=4, EventSystems=4, UnityEngine.UI=2, SetActive=1, MonoBehaviour=1; unity_score=76
- ksp refs: none; fan_in=10 fan_out=3
- methods(9): AddInputDelegate, InstantiateFromPrefab, MouseInput, OnClick, OnRightClick, SetAsRead, SetButtonColor, SetMessage, SetTexture
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/MessageSystemPopup.cs
- 139 lines; kind=MonoBehaviour; type=MessageSystemPopup; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=4, GameObject=3, TMPro=2, SetActive=2, Lifecycle=2, Instantiate/Destroy=2; unity_score=69
- ksp refs: GameEvents=2; fan_in=1 fan_out=3
- methods(13): Hide, InstantiateFromPrefab, MouseinputClose, MouseinputDiscard, OnDestroy, OnDiscard, OnSceneLoadRequest, OnShowNext, OnShowPrevious, Show, ShowNext, ShowPrevious, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/MissionControl.cs
- 600 lines; kind=MonoBehaviour; type=MissionControl; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: Contracts, Contracts.Agents, KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=41, SetActive=38, UnityEngine.UI=17, TMPro=5, Time.=3, Lifecycle=3; unity_score=78
- ksp refs: Contract=33, Localizer=16, Planetarium=2, GameSettings=2, GameEvents=2; fan_in=6 fan_out=20
- methods(31): AddItem, Awake, ClearInfoPanel, DestroyInstructor, OnClickAccept, OnClickActive, OnClickAgentInfo, OnClickArchive, OnClickArchiveAll, OnClickArchiveCancelled, OnClickArchiveCompleted, OnClickArchiveFailed, OnClickAvailable, OnClickCancel, OnClickDecline, OnDeselectContract, OnDestroy, OnSelectContract, RebuildContractList, RefreshContracts, RefreshUIControls, SetArchiveMode, SetDisplayModeActive, SetDisplayModeArchive, SetDisplayModeAvailable, SetInstructorText, SetupInstructor, Start, UpdateInfoPanelAgent, UpdateInfoPanelContract, UpdateInstructor
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/MissionRecoveryDialog.cs
- 509 lines; kind=MonoBehaviour; type=MissionRecoveryDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, KSP.UI.Screens.SpaceCenter.MissionSummaryDialog, KSP.UI.Util, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=17, SetActive=15, UnityEngine.UI=10, Texture=6, Transform=4, Lifecycle=4; unity_score=74
- ksp refs: Localizer=14, GameEvents=4, ProtoVessel=2, InputLockManager=2, HighLogic=2, Reputation=1; fan_in=20 fan_out=19
- methods(22): AddCrewWidget, AddDataWidget, AddPartWidget, AddResourceWidget, Awake, CreateFullDialog, CreateScienceDialog, GetProceedButtonCaption, MissionIsUninteresting, OnCrewTabPress, OnDestroy, OnGameSceneLoadRequested, OnPartsTabPress, OnProceedButtonClick, OnSciTabPress, SetMode, Start, Update, dismissDialog, updateCrewWindowContent, updatePartsWindowContent, updateScienceWindowContent
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/OnOpen.cs
- 4 lines; kind=delegate; type=OnOpen; bases=-; ancestry=-; ns=KSP.UI.Screens
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(1): OnOpen
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/OnSelectItem.cs
- 4 lines; kind=delegate; type=OnSelectItem; bases=-; ancestry=-; ns=KSP.UI.Screens
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(1): OnSelectItem
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/PartCategorizer.cs **MUST-OPEN (>800 lines)**
- 2181 lines; kind=MonoBehaviour; type=PartCategorizer; bases=BasePartCategorizer; ancestry=BasePartCategorizer/MonoBehaviour; ns=KSP.UI.Screens
- usings: Contracts.Agents, Expansions, KSP.Localization, KSP.UI.Screens.PartListCategories, RUI.Icons.Selectable, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Coroutine=17, GameObject=14, Transform=11, EventSystems=10, SetActive=8, Instantiate/Destroy=7; unity_score=41
- ksp refs: AvailablePart=98, ConfigNode=26, PartLoader=17, Localizer=14, EditorLogic=10, InputLockManager=3; fan_in=11 fan_out=31
- methods(99): AcceptCriteria, AcceptCriteriaPopupAddPart, AcceptNewCategory, AcceptNewSubcategory, AcceptNewSubcategoryCriteria, AcceptNewSubcategorySubassembly, AddCustomCategory, AddCustomCategorySubassembly, AddCustomFilter, AddCustomSubcategoryFilter, AddPart, AddPartToCategory, AddPartToCustomCategoryViaPopup, AddSubassembly, AddSubassemblyToSelectedCategory, AddSubcategory, Awake, CompileExclusionFilter, CompileExclusionFilterSubassembly, CreateCrossSectionFilters, CreateManufacturerFilters, CreateModuleFilters, CreateResourceFilters, CreateTechTierFilters, DeleteCategory, DeleteCategoryCriteria, DeleteSubcategory, DeleteSubcategoryCriteria, EditCategory, EditCategoryCriteria, EditSubCategoryCriteria, FlipAllCategoryButtons, FlipAllFilterButtons, FocusSearchField, ForceShowPanel, InsertSubcategoryButtons, InstantiatePartCategorizerButton, InstantiatePartCategorizerButtonPlus, InstantiatePopup, InstantiatePopupAddPart, LoadCustomPartCategories, LoadCustomSubassemblyCategories, MouseInputArrowLeft, MouseInputArrowRight, OnAcceptPopupAddPart ...
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/PartCategorizerButton.cs
- 257 lines; kind=MonoBehaviour; type=PartCategorizerButton; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, KSP.UI.TooltipTypes, RUI.Icons.Selectable, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: SetActive=18, GameObject=18, UnityEngine.UI=15, EventSystems=4, Transform=2, GetComponent=2; unity_score=84
- ksp refs: Localizer=1; fan_in=4 fan_out=5
- methods(14): DisableDividers, EnableDividerOverlay, EnableDividerSpaceUnder, ForceAspect, InitializeBtn, InitializeToggleBtn, MouseInput_click, MouseInput_pointerEnter, MouseInput_pointerExit, OnFalse, OnTrue, SetIcon, SetRadioGroup, UpdateIconState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/PartCategorizerPopup.cs
- 200 lines; kind=MonoBehaviour; type=PartCategorizerPopup; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, RUI.Icons.Selectable, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=5, GameObject=5, SetActive=4, GetComponent=2, Transform=1, TMPro=1; unity_score=66
- ksp refs: Localizer=5, HighLogic=4, InputLockManager=2; fan_in=1 fan_out=14
- methods(13): AcceptDelete, Awake, CreateIcon, CreateIconList, CriteriaAccept, CriteriaDelete, Hide, MouseinputAccept, MouseinputClose, MouseinputDelete, OnIconSelect, Show, UpdateIconList
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/PartCategorizerPopupAddPart.cs
- 191 lines; kind=MonoBehaviour; type=PartCategorizerPopupAddPart; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=14, Transform=12, GetComponent=7, UnityEngine.UI=6, SetActive=2, Lifecycle=2; unity_score=84
- ksp refs: Localizer=3, HighLogic=2; fan_in=1 fan_out=12
- methods(10): Awake, CreateIconList, CriteriaAccept, Hide, MouseinputAccept, MouseinputClose, OnDestroy, OnIconSelect, Show, UpdateIconList
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/PartDropZone.cs
- 68 lines; kind=MonoBehaviour; type=PartDropZone; bases=TooltipController; ancestry=TooltipController/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, KSP.UI.TooltipTypes, UnityEngine, UnityEngine.EventSystems
- unity: Lifecycle=2, GetComponent=1, EventSystems=1; unity_score=52
- ksp refs: EditorLogic=3, Localizer=2, AvailablePart=1; fan_in=1 fan_out=6
- methods(4): Awake, ButtonInputDelegate, OnDestroy, OnTooltipSpawned
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/PartListCategories/BulkheadProfile.cs
- 74 lines; kind=class; type=BulkheadProfile; bases=PartCategory; ancestry=PartCategory; ns=KSP.UI.Screens.PartListCategories
- usings: -
- unity: none; unity_score=5
- ksp refs: AvailablePart=2; fan_in=1 fan_out=2
- methods(3): ExclusionCriteria, appendValue, defaultTag
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/KSP/UI/Screens/PartListCategories/PartCategory.cs
- 33 lines; kind=class; type=PartCategory; bases=-; ancestry=-; ns=KSP.UI.Screens.PartListCategories
- usings: KSP.Localization, RUI.Icons.Selectable, UnityEngine
- unity: UnityEngine.UI=1; unity_score=35
- ksp refs: AvailablePart=3, Localizer=1; fan_in=2 fan_out=5
- methods(2): ExclusionCriteria, GetCategory
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/KSP/UI/Screens/PartListCategories/TechTier.cs
- 27 lines; kind=class; type=TechTier; bases=PartCategory; ancestry=PartCategory; ns=KSP.UI.Screens.PartListCategories
- usings: -
- unity: none; unity_score=12
- ksp refs: AvailablePart=1; fan_in=1 fan_out=4
- methods(1): ExclusionCriteria
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/KSP/UI/Screens/PrereleaseWarningDialog.cs
- 32 lines; kind=MonoBehaviour; type=PrereleaseWarningDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: Instantiate/Destroy=2, GameObject=2, UnityEngine.UI=1, Transform=1, TMPro=1, MonoBehaviour=1; unity_score=87
- ksp refs: Localizer=1; fan_in=0 fan_out=4
- methods(2): Awake, OnBtnContinue
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ProtoStageIcon.cs
- 242 lines; kind=class; type=ProtoStageIcon; bases=-; ancestry=-; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: none; unity_score=8
- ksp refs: Part=4, HighLogic=1, FlightGlobals=1; fan_in=7 fan_out=8
- methods(14): BlinkBorder, ClearInfoBoxes, CreateIcon, DisableIcon, DisplayInfo, Freeze, Highlight, RemoveIcon, RemoveInfo, SetBackgroundColor, SetBorderColor, SetIcon, SetIconColor, Unfreeze
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ProtoStageIconInfo.cs
- 129 lines; kind=class; type=ProtoStageIconInfo; bases=-; ancestry=-; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: none; unity_score=9
- ksp refs: none; fan_in=4 fan_out=2
- methods(9): SetCaption, SetLength, SetMessage, SetMsgBgColor, SetMsgTextColor, SetProgressBarBgColor, SetProgressBarColor, SetValue, StartInfoBox
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDArchivesController.cs
- 771 lines; kind=MonoBehaviour; type=RDArchivesController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: Instantiate/Destroy=10, GameObject=10, GetComponent=6, UnityEngine.UI=3, TMPro=2, Lifecycle=2; unity_score=45
- ksp refs: Localizer=14, HighLogic=3, ScienceSubject=1; fan_in=1 fan_out=21
- methods(34): AddPlanetRecursively, AddPlanets, CacheReportLists, Cascade, ClearALLSubFilterLists, ClearAllSubFilterSortingFlags, ClearSubFilterList, DestroyInstructor, DestroyReportLists, DropdownListCallback, DropdownListItemCallback, FilterBiomes, FilterExperiments, FilterList, FilterReports, FilterReportsFunc, FilterSituations, GetPlanetFilterFunction, GetPlanetFilterlist, OnDestroy, OnReportListSort, RandomFloat, RefreshALLSubFilterLists, RefreshSubFilterList, RefreshSubFilterListAndFilter, RefreshSubFilterListAndFilterKEEPSelection, SetInstructorText, SetupInstructor, Start, UpdateInstructor, UpdateReportListFromCache, UpdateReportListItem, UpdateReports, UpdateSelectedList
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDController.cs
- 291 lines; kind=MonoBehaviour; type=RDController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, RUI.Icons.Simple, TMPro, UnityEngine
- unity: GameObject=7, SetActive=5, TMPro=4, Lifecycle=4, Transform=2, UnityEngine.UI=1; unity_score=60
- ksp refs: Localizer=6, AvailablePart=6, ResearchAndDevelopment=1, PartLoader=1, HighLogic=1; fan_in=8 fan_out=22
- methods(11): ActionButtonClick, Awake, OnDestroy, OnEnable, RegisterNode, ShowNodePanel, ShowNothingPanel, SnapToStartNode, Start, UpdatePanel, UpdatePurchaseButton
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDDropDownList.cs
- 178 lines; kind=MonoBehaviour; type=RDDropDownList; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.EventSystems
- unity: GetComponent=7, Instantiate/Destroy=2, EventSystems=2, TMPro=1, SetActive=1, MonoBehaviour=1; unity_score=58
- ksp refs: none; fan_in=4 fan_out=8
- methods(15): AddItem, AddSelectedItem, ClearList, Close, CloseInternal, CreateInstanceFromPrefab, GetBiomeFilter, GetFilter, OnFalse, OnTrue, Open, OpenInternal, SetStandardToggleButtonInputDelegate, UpdateItem, UpdateSelectedItem
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDDropDownListContainer.cs
- 28 lines; kind=MonoBehaviour; type=RDDropDownListContainer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: MonoBehaviour=1, GetComponent=1; unity_score=50
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): Initialize
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDDropDownListItem.cs
- 21 lines; kind=MonoBehaviour; type=RDDropDownListItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine
- unity: TMPro=2, MonoBehaviour=1; unity_score=67
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDEnvironmentAdapter.cs
- 79 lines; kind=static class; type=RDEnvironmentAdapter; bases=-; ancestry=-; ns=KSP.UI.Screens
- usings: -
- unity: none; unity_score=0
- ksp refs: ResearchAndDevelopment=16, CelestialBody=2, ScienceSubject=1; fan_in=2 fan_out=5
- methods(8): GetBiomeTags, GetBiomeTagsLocalized, GetExperiment, GetExperimentIDs, GetResults, GetSituationTags, GetSituationTagsDescriptions, GetSubjects
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDGridArea.cs
- 6 lines; kind=MonoBehaviour; type=RDGridArea; bases=UIGridArea; ancestry=UIGridArea/MonoBehaviour; ns=KSP.UI.Screens
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDNode.cs
- 771 lines; kind=MonoBehaviour; type=RDNode; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, KSP.UI.TooltipTypes, RUI.Icons.Simple, UnityEngine, UnityEngine.UI, Vectrosity
- unity: Transform=38, GameObject=6, Instantiate/Destroy=3, SetActive=2, Material=2, Lifecycle=2; unity_score=59
- ksp refs: ConfigNode=7, Localizer=4, AvailablePart=2; fan_in=9 fan_out=16
- methods(26): Awake, DrawArrow, FakeGizmos, FindNodeByID, GetPosition, GetTooltipCaption, GetVectorArray, GetVisiblePartsCount, InitializeArrows, Load, LoadLinks, NodeInput, OnDrawGizmos, PartsInTotal, PartsNotUnlocked, Save, SelectNode, SetButtonState, SetIconState, Setup, ShowArrows, Start, UnselectNode, UpdateGraphics, Warmup, chamferCorner
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDNodeList.cs
- 60 lines; kind=MonoBehaviour; type=RDNodeList; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: SetActive=8, GameObject=8, GetComponent=2, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1; unity_score=89
- ksp refs: none; fan_in=1 fan_out=5
- methods(3): AddNodes, Awake, ClearList
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDNodeListItem.cs
- 13 lines; kind=MonoBehaviour; type=RDNodeListItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine
- unity: TMPro=1, MonoBehaviour=1; unity_score=55
- ksp refs: none; fan_in=1 fan_out=3
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDNodePrefab.cs
- 162 lines; kind=MonoBehaviour; type=RDNodePrefab; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.UI.TooltipTypes, RUI.Icons.Simple, TMPro, UnityEngine, UnityEngine.Events, UnityEngine.UI
- unity: UnityEngine.UI=11, Transform=8, Texture=6, GameObject=3, SetActive=2, Material=2; unity_score=85
- ksp refs: none; fan_in=2 fan_out=4
- methods(14): AddInputDelegate, GetArrowHeadPrefab, GetIconColor, HideAvailablePartsCircle, InstantiateArrowHeadAtPos, SelectNode, SetArrowHeadState, SetAvailablePartsCircle, SetIcon, SetIconColor, SetScale, SetViewable, Setup, UnselectNode
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDPartList.cs
- 193 lines; kind=MonoBehaviour; type=RDPartList; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, KSP.UI.Screens.Editor, UnityEngine, UnityEngine.UI
- unity: GetComponent=6, UnityEngine.UI=3, Texture=2, Lifecycle=2, Instantiate/Destroy=2, MonoBehaviour=1; unity_score=62
- ksp refs: AvailablePart=12, Localizer=2, Funding=2, PartLoader=1; fan_in=1 fan_out=16
- methods(10): AddPartListItem, AddParts, AddUpgradeListItem, AddUpgrades, Awake, OnDestroy, OnPartListUpdate, Refresh, SetPart, SetupParts
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDPartListItem.cs
- 29 lines; kind=MonoBehaviour; type=RDPartListItem; bases=EditorPartIcon; ancestry=EditorPartIcon/MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine.UI
- unity: UnityEngine.UI=1, TMPro=1; unity_score=59
- ksp refs: AvailablePart=2; fan_in=2 fan_out=5
- methods(1): Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDPlanetListItemContainer.cs
- 286 lines; kind=MonoBehaviour; type=RDPlanetListItemContainer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Transform=16, GameObject=10, GetComponent=7, Texture=4, EventSystems=4, Camera=4; unity_score=85
- ksp refs: Localizer=1; fan_in=1 fan_out=8
- methods(22): AddChild, CreateThumbnailCamera, GetParentInHierarchy, Hide, HideChildren, OnDestroy, OnFalse, OnPointerEnter, OnPointerExit, OnTrue, PopInPlanet, PopOutPlanet, RotatePlanet, SelectionCallback, SetLightDirection, SetSelectionCallback, Setup, Show, ShowChildren, StartRotation, StopRotation, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDReportListItemContainer.cs
- 97 lines; kind=MonoBehaviour; type=RDReportListItemContainer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=4, Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1, IMGUI=1; unity_score=65
- ksp refs: Localizer=4; fan_in=1 fan_out=8
- methods(9): ClosePopup, OnButtonInput, OnDestroy, PushDataToGUI, SetDataLabel, SetDescriptionLabel, SetScienceLabel, SetValueSlider, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDSceneDespawner.cs
- 20 lines; kind=MonoBehaviour; type=RDSceneDespawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1; unity_score=63
- ksp refs: GameEvents=1; fan_in=0 fan_out=1
- methods(2): BtnExit, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDSceneSpawner.cs
- 47 lines; kind=MonoBehaviour; type=RDSceneSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.Rendering
- unity: Light=6, Texture=2, Lifecycle=2, MonoBehaviour=1; unity_score=87
- ksp refs: GameEvents=4, ResearchAndDevelopment=1, HighLogic=1; fan_in=0 fan_out=8
- methods(4): Awake, OnDestroy, onRDDespawn, onRDSpawn
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDTechTree.cs
- 399 lines; kind=MonoBehaviour; type=RDTechTree; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: GameObject=6, GetComponent=2, MonoBehaviour=1, Lifecycle=1, LayerMask=1, Instantiate/Destroy=1; unity_score=40
- ksp refs: ConfigNode=27, HighLogic=4, ResearchAndDevelopment=3, GameDatabase=1; fan_in=5 fan_out=13
- methods(18): Awake, FindTech, GetCheapestUnavailableNodes, GetNextUnavailableNodes, GetRDScreenPrefab, GetTechNodesFromDB, GetTreeConfigNode, GetTreeNodes, GetTreeTechs, LoadTechTitles, LoadTechTree, PreLoad, ReLoad, RefreshUI, SaveTechTree, SpawnTechTreeNodes, WipeTechTree, recurseForNextTechs
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDTechTreeSearchBar.cs
- 305 lines; kind=MonoBehaviour; type=RDTechTreeSearchBar; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=3, Coroutine=3, Time.=2, Lifecycle=2, GetComponent=2, GameObject=2; unity_score=48
- ksp refs: AvailablePart=2; fan_in=4 fan_out=7
- methods(15): Awake, ClearSearchBox, DeselectSearchResults, PartMatchesSearch, SearchField_OnClick, SearchField_OnValueChange, SearchForNode, SearchRoutine, SearchStart, SearchStop, SearchTagSplit, SelectPartIcons, Start, TagMatchType, TermMatchesTag
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RDTestSceneLoader.cs
- 360 lines; kind=MonoBehaviour; type=RDTestSceneLoader; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: GameObject=9, Transform=2, Lifecycle=2, SetActive=1, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=48
- ksp refs: ConfigNode=17, AvailablePart=9, CelestialBody=4, ScienceSubject=3, PartResourceDefinition=1; fan_in=5 fan_out=11
- methods(12): Awake, GetBiomeTags, GetBiomeTagsLocalized, GetExperiment, GetExperimentIDs, GetMiniBiomeTags, GetMiniBiomeTagsLocalized, GetResults, GetSituationTags, GetSituationTagsDescriptions, LoadPartDefs, OnDestroy
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ResourceDisplay.cs
- 478 lines; kind=MonoBehaviour; type=ResourceDisplay; bases=UIApp; ancestry=UIApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, UnityEngine
- unity: GameObject=13, SetActive=11, Transform=4, Resources.=4, Instantiate/Destroy=4, Coroutine=3; unity_score=58
- ksp refs: FlightGlobals=12, Part=8, GameEvents=8, Vessel=7, PartResourceDefinition=5, PartResource=3; fan_in=4 fan_out=15
- methods(32): AddOnHideResource, AddOnShowResource, ClearResourceList, CreateResourceList, DisablePanel, DisplayApp, EnablePanel, ForceHideResourceList, GetAppScenes, GetAppScreenPos, HideApp, HideResourceList, Hover, HoverOut, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnHideResource, OnShowResource, OnVesselChanged, OnVesselDestroy, OnVesselGoOffRails, OnVesselWasModified, Refresh, RemoveOnHideResource, RemoveOnShowResource, ResizeList, SetItemSelection, ShowResourceList, ShowResourceListDetermine, ToggleStageView, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ResourceDisplayOptions.cs
- 24 lines; kind=MonoBehaviour; type=ResourceDisplayOptions; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, MonoBehaviour=1; unity_score=55
- ksp refs: none; fan_in=1 fan_out=2
- methods(2): OnStageInput, Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ResourceItem.cs
- 201 lines; kind=MonoBehaviour; type=ResourceItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.UI.TooltipTypes, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: TMPro=4, Lifecycle=3, UnityEngine.UI=2, Time.=2, EventSystems=2, Resources.=1; unity_score=60
- ksp refs: PartResource=2, Vessel=1, PartResourceDefinition=1, FlightGlobals=1; fan_in=2 fan_out=12
- methods(8): Awake, FixedUpdate, MouseInput_click, MouseInput_pointerEnter, MouseInput_pointerExit, Setup, Update, UpdateStaged
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RotationalGauge.cs
- 116 lines; kind=MonoBehaviour; type=RotationalGauge; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: Lifecycle=2, UnityEngine.UI=1, Time.=1, MonoBehaviour=1; unity_score=53
- ksp refs: none; fan_in=4 fan_out=0
- methods(3): Awake, LateUpdate, SetValue
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/RotationalGaugeOffsetMarker.cs
- 91 lines; kind=MonoBehaviour; type=RotationalGaugeOffsetMarker; bases=RotationalGauge; ancestry=RotationalGauge/MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine
- unity: Transform=4, Lifecycle=4, GameObject=4, UnityEngine.UI=2, TMPro=2, GetComponent=2; unity_score=85
- ksp refs: none; fan_in=1 fan_out=2
- methods(7): Awake, LateUpdate, OnDestroy, SetNextToValue, SetTextField, Start, ToggleOffsetMarker
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/AccessorBase.cs
- 181 lines; kind=class; type=AccessorBase; bases=-; ancestry=-; ns=KSP.UI.Screens.Settings.Controls
- usings: UnityEngine
- unity: none; unity_score=17
- ksp refs: none; fan_in=4 fan_out=2
- methods(3): Create, IsDigit, IsIdentifier
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/FieldAccessor.cs
- 26 lines; kind=class; type=FieldAccessor; bases=AccessorBase; ancestry=AccessorBase; ns=KSP.UI.Screens.Settings.Controls
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/PropertyAccessor.cs
- 27 lines; kind=class; type=PropertyAccessor; bases=AccessorBase; ancestry=AccessorBase; ns=KSP.UI.Screens.Settings.Controls
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/ReflectedSettingsWindowTab.cs
- 43 lines; kind=MonoBehaviour; type=ReflectedSettingsWindowTab; bases=SettingsControlBase; ancestry=SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: UnityEngine
- unity: GameObject=4, UnityEngine.UI=1, SetActive=1, Lifecycle=1; unity_score=73
- ksp refs: none; fan_in=1 fan_out=2
- methods(3): OnStart, ShowTab, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsAxisScale.cs
- 83 lines; kind=MonoBehaviour; type=SettingsAxisScale; bases=SettingsControlReflection; ancestry=SettingsControlReflection/SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1; unity_score=31
- ksp refs: none; fan_in=0 fan_out=3
- methods(5): OnStart, PreApply, SliderValueChange, ValueInitialized, ValueUpdated
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsBoolToggle.cs
- 47 lines; kind=MonoBehaviour; type=SettingsBoolToggle; bases=SettingsControlReflection; ancestry=SettingsControlReflection/SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=3
- methods(4): OnStart, OnToggled, ValueInitialized, ValueUpdated
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsBoolToggleBitMask.cs
- 45 lines; kind=MonoBehaviour; type=SettingsBoolToggleBitMask; bases=SettingsControlReflection; ancestry=SettingsControlReflection/SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=3
- methods(4): OnApply, OnStart, OnToggled, ValueInitialized
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsBoolTogglePPFX.cs
- 112 lines; kind=MonoBehaviour; type=SettingsBoolTogglePPFX; bases=SettingsControlReflection; ancestry=SettingsControlReflection/SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: Highlighting, UnityEngine
- unity: Lifecycle=1, GetComponent=1; unity_score=28
- ksp refs: GameSettings=1; fan_in=0 fan_out=6
- methods(6): OnStart, OnToggled, SetupAdditionalFields, Update, ValueInitialized, ValueUpdated
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsControlBase.cs
- 45 lines; kind=MonoBehaviour; type=SettingsControlBase; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: TMPro, UnityEngine
- unity: TMPro=1, MonoBehaviour=1; unity_score=46
- ksp refs: none; fan_in=11 fan_out=1
- methods(4): IsValid, OnApply, OnRevert, SetTitleText
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsControlReflection.cs
- 110 lines; kind=MonoBehaviour; type=SettingsControlReflection; bases=SettingsControlBase; ancestry=SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: TMPro, UnityEngine
- unity: TMPro=1, SendMessage/Invoke=1, Lifecycle=1, GameObject=1; unity_score=44
- ksp refs: GameSettings=1; fan_in=17 fan_out=5
- methods(10): GetAccessor, GetValue, OnApply, OnRevert, OnStart, PreApply, SetValue, Start, ValueInitialized, ValueUpdated
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsFloatRange.cs
- 76 lines; kind=MonoBehaviour; type=SettingsFloatRange; bases=SettingsControlReflection; ancestry=SettingsControlReflection/SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1; unity_score=36
- ksp refs: none; fan_in=0 fan_out=3
- methods(5): OnSliderValueChange, OnStart, SetColorForRecommendedUIScale, ValueInitialized, ValueUpdated
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsGraphicsResolution.cs
- 117 lines; kind=MonoBehaviour; type=SettingsGraphicsResolution; bases=SettingsControlBase; ancestry=SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: Screen/Cursor=4, UnityEngine.UI=2, TMPro=1, Lifecycle=1, Application.=1; unity_score=50
- ksp refs: GameSettings=2, Localizer=1; fan_in=0 fan_out=4
- methods(7): GetAvailableResolutions, OnApply, OnButtonDown, OnButtonUp, OnRevert, SetValue, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsInput.cs
- 208 lines; kind=MonoBehaviour; type=SettingsInput; bases=SettingsWindow; ancestry=SettingsWindow/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: UnityEngine
- unity: Transform=14, GameObject=8, Instantiate/Destroy=6, UnityEngine.UI=2, SendMessage/Invoke=2, Lifecycle=1; unity_score=73
- ksp refs: none; fan_in=0 fan_out=9
- methods(6): LoadFile, OnBindingRequest, OnBindingRequestFinished, OnBindingRequestModes, SaveFile, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsInputAxis.cs
- 140 lines; kind=MonoBehaviour; type=SettingsInputAxis; bases=SettingsControlReflection; ancestry=SettingsControlReflection/SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=6, TMPro=6; unity_score=66
- ksp refs: GameSettings=8, Localizer=2; fan_in=1 fan_out=9
- methods(12): OnChangePrimaryDeadzone, OnChangePrimarySensitivity, OnChangeSecondaryDeadzone, OnChangeSecondarySensitivity, OnClickPrimary, OnClickSecondary, OnStart, OnTogglePrimaryInvert, OnToggleSecondaryInvert, UpdateInputSettings, ValueInitialized, ValueUpdated
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsInputBinding.cs
- 375 lines; kind=MonoBehaviour; type=SettingsInputBinding; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: KeyCode=11, Transform=6, Instantiate/Destroy=5, UnityEngine.UI=4, TMPro=3, Input.=3; unity_score=66
- ksp refs: Localizer=17, ConfigNode=8; fan_in=3 fan_out=13
- methods(14): OnAccept, OnCancel, OnClear, OnModeDockingRotation, OnModeDockingTranslation, OnModeStaging, ResetAxisValues, SetAxis, SetKey, SetupAxis, SetupKey, SetupModes, Start, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsInputBindingMode.cs
- 12 lines; kind=MonoBehaviour; type=SettingsInputBindingMode; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: TMPro, UnityEngine
- unity: TMPro=1, MonoBehaviour=1; unity_score=55
- ksp refs: none; fan_in=1 fan_out=2
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsInputGroupTitle.cs
- 6 lines; kind=MonoBehaviour; type=SettingsInputGroupTitle; bases=SettingsControlBase; ancestry=SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsInputKey.cs
- 84 lines; kind=MonoBehaviour; type=SettingsInputKey; bases=SettingsControlReflection; ancestry=SettingsControlReflection/SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: KSP.Localization, TMPro, UnityEngine.UI
- unity: UnityEngine.UI=2, TMPro=2; unity_score=51
- ksp refs: Localizer=4, ConfigNode=1; fan_in=2 fan_out=9
- methods(7): GetValue, OnClickPrimary, OnClickSecondary, OnStart, UpdateInputSettings, ValueInitialized, ValueUpdated
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsIntArrayButtons.cs
- 92 lines; kind=MonoBehaviour; type=SettingsIntArrayButtons; bases=SettingsControlReflection; ancestry=SettingsControlReflection/SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: KSP.Localization, UnityEngine.UI
- unity: UnityEngine.UI=2; unity_score=36
- ksp refs: Localizer=1; fan_in=1 fan_out=3
- methods(5): OnButtonDown, OnButtonUp, OnStart, ValueInitialized, ValueUpdated
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsIntArrayRange.cs
- 82 lines; kind=MonoBehaviour; type=SettingsIntArrayRange; bases=SettingsControlReflection; ancestry=SettingsControlReflection/SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: KSP.Localization, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1; unity_score=29
- ksp refs: Localizer=1; fan_in=0 fan_out=3
- methods(4): OnSliderValueChange, OnStart, ValueInitialized, ValueUpdated
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsIntButtons.cs
- 41 lines; kind=MonoBehaviour; type=SettingsIntButtons; bases=SettingsControlReflection; ancestry=SettingsControlReflection/SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: UnityEngine.UI
- unity: UnityEngine.UI=2; unity_score=52
- ksp refs: none; fan_in=0 fan_out=2
- methods(3): OnButtonDown, OnButtonUp, OnStart
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsIntRange.cs
- 39 lines; kind=MonoBehaviour; type=SettingsIntRange; bases=SettingsControlReflection; ancestry=SettingsControlReflection/SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1; unity_score=40
- ksp refs: none; fan_in=0 fan_out=2
- methods(4): OnSliderValueChange, OnStart, ValueInitialized, ValueUpdated
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsIntToggle.cs
- 53 lines; kind=MonoBehaviour; type=SettingsIntToggle; bases=SettingsControlReflection; ancestry=SettingsControlReflection/SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=3
- methods(4): OnStart, OnToggled, ValueInitialized, ValueUpdated
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsKeyboardLayoutApply.cs
- 17 lines; kind=MonoBehaviour; type=SettingsKeyboardLayoutApply; bases=SettingsControlBase; ancestry=SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: UnityEngine.UI
- unity: UnityEngine.UI=1, Lifecycle=1; unity_score=55
- ksp refs: GameSettings=1; fan_in=1 fan_out=2
- methods(1): Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsKeyboardLayoutInput.cs
- 158 lines; kind=MonoBehaviour; type=SettingsKeyboardLayoutInput; bases=SettingsControlBase; ancestry=SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: KSP.Localization, TMPro, UnityEngine.UI
- unity: UnityEngine.UI=2, TMPro=1, Lifecycle=1; unity_score=37
- ksp refs: GameSettings=9, ConfigNode=5, Localizer=1; fan_in=1 fan_out=7
- methods(10): ChangeLayout, CleanLayout, Init, LoadCurrentLayout, OnApply, OnNext, OnPrev, OnRevert, SetKeysLayout, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsKeyboardLayoutOs.cs
- 18 lines; kind=MonoBehaviour; type=SettingsKeyboardLayoutOs; bases=SettingsControlBase; ancestry=SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: KSP.Localization, TMPro, UnityEngine
- unity: TMPro=1, Lifecycle=1; unity_score=59
- ksp refs: Localizer=1; fan_in=1 fan_out=4
- methods(1): Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsLabel.cs
- 20 lines; kind=MonoBehaviour; type=SettingsLabel; bases=SettingsControlReflection; ancestry=SettingsControlReflection/SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: none; fan_in=0 fan_out=2
- methods(1): OnStart
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsLayoutConfig.cs
- 138 lines; kind=MonoBehaviour; type=SettingsLayoutConfig; bases=SettingsWindow; ancestry=SettingsWindow/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: KSP.Localization, TMPro, UnityEngine
- unity: Transform=14, SetActive=4, Instantiate/Destroy=4, Lifecycle=3, TMPro=2, GetComponent=2; unity_score=80
- ksp refs: Localizer=3, ConfigNode=2; fan_in=3 fan_out=10
- methods(10): Awake, OnApply, OnDestroy, OnLayoutChange, OnLayoutResult, OnRebindResult, OnRevert, SaveChanges, ShowLayoutChangePopUp, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsScreenTab.cs
- 80 lines; kind=MonoBehaviour; type=SettingsScreenTab; bases=SettingsControlBase; ancestry=SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=5, SetActive=3, GameObject=3, Lifecycle=2, EventSystems=2, GetComponent=1; unity_score=81
- ksp refs: none; fan_in=1 fan_out=4
- methods(5): Activate, Awake, DeselectTab, SelectTab, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsTerrainDetail.cs
- 110 lines; kind=MonoBehaviour; type=SettingsTerrainDetail; bases=SettingsControlBase; ancestry=SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, TMPro=1, Lifecycle=1; unity_score=38
- ksp refs: Localizer=1, GameEvents=1; fan_in=0 fan_out=6
- methods(4): OnApply, OnRevert, OnSliderValueChange, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsTrackIRBool.cs
- 112 lines; kind=MonoBehaviour; type=SettingsTrackIRBool; bases=SettingsControlReflection; ancestry=SettingsControlReflection/SettingsControlBase/MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls
- usings: UnityEngine
- unity: GameObject=1; unity_score=29
- ksp refs: GameSettings=1; fan_in=0 fan_out=6
- methods(9): EnableInteraction, GetAccessor, GetValue, OnStart, OnToggled, SetValue, SettingChanged, ValueInitialized, ValueUpdated
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/Values/SettingsValueBoolTitleChange.cs
- 28 lines; kind=MonoBehaviour; type=SettingsValueBoolTitleChange; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls.Values
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=59
- ksp refs: none; fan_in=0 fan_out=2
- methods(2): Awake, UpdateValue
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/Values/SettingsValueIntToString.cs
- 25 lines; kind=MonoBehaviour; type=SettingsValueIntToString; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Settings.Controls.Values
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=59
- ksp refs: none; fan_in=0 fan_out=2
- methods(2): Awake, UpdateValue
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/ReflectedSettingsWindow.cs
- 370 lines; kind=MonoBehaviour; type=ReflectedSettingsWindow; bases=SettingsWindow; ancestry=SettingsWindow/MonoBehaviour; ns=KSP.UI.Screens.Settings
- usings: KSP.UI.Screens.Settings.Controls, UnityEngine
- unity: GameObject=6, Transform=3, Instantiate/Destroy=3, UnityEngine.UI=1, Lifecycle=1; unity_score=42
- ksp refs: none; fan_in=3 fan_out=9
- methods(9): GetField, GetTabParent, IsValid, IsValidSecondary, OnCloseWindow, OnOpenWindow, Setup, SetupReflectionValues, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/ReflectedSettingsWindow_6DOF.cs
- 10 lines; kind=MonoBehaviour; type=ReflectedSettingsWindow_6DOF; bases=ReflectedSettingsWindow; ancestry=ReflectedSettingsWindow/SettingsWindow/MonoBehaviour; ns=KSP.UI.Screens.Settings
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): IsValidSecondary
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/ReflectedSettingsWindow_Mouse.cs
- 10 lines; kind=MonoBehaviour; type=ReflectedSettingsWindow_Mouse; bases=ReflectedSettingsWindow; ancestry=ReflectedSettingsWindow/SettingsWindow/MonoBehaviour; ns=KSP.UI.Screens.Settings
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): IsValidSecondary
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/ReflectedSettingsWindow_TrackIR.cs
- 10 lines; kind=MonoBehaviour; type=ReflectedSettingsWindow_TrackIR; bases=ReflectedSettingsWindow; ancestry=ReflectedSettingsWindow/SettingsWindow/MonoBehaviour; ns=KSP.UI.Screens.Settings
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): IsValidSecondary
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsExpansion.cs
- 31 lines; kind=static class; type=SettingsExpansion; bases=-; ancestry=-; ns=KSP.UI.Screens.Settings
- usings: Expansions
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(1): IsExpansion
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsFieldWrapper.cs
- 17 lines; kind=class; type=SettingsFieldWrapper; bases=-; ancestry=-; ns=KSP.UI.Screens.Settings
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsPlatform.cs
- 57 lines; kind=static class; type=SettingsPlatform; bases=-; ancestry=-; ns=KSP.UI.Screens.Settings
- usings: UnityEngine
- unity: Application.=7; unity_score=33
- ksp refs: none; fan_in=2 fan_out=0
- methods(1): IsPlatform
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsScreen.cs
- 638 lines; kind=MonoBehaviour; type=SettingsScreen; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Settings
- usings: KSP.UI.Screens.Settings.Controls, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=43, Transform=42, GetComponent=14, GameObject=13, Instantiate/Destroy=9, SetActive=6; unity_score=85
- ksp refs: GameSettings=8, HighLogic=2, ConfigNode=2, GameEvents=1; fan_in=7 fan_out=15
- methods(26): Awake, CacheCurrentSelectedItemColumn, Clear, CloseResetPopUp, FocusSelectableWithinRange, GetKeyLayoutMap, LoadResetPopupSelectablesCo, LoadSelectables, LoadSelectablesCo, NavigateBetweenColumns, OnAccept, OnApply, OnCancel, OnCloseWindow, OnDestroy, OnOpenWindow, OnReset, OnRevert, Recreate, SetExplicitUINavigation, Setup, SetupTab, SetupWindow, Start, TryNavigatingNextContentColumn, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsScreenSetup.cs
- 11 lines; kind=MonoBehaviour; type=SettingsScreenSetup; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Settings
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=36
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsSetup.cs
- 335 lines; kind=class; type=SettingsSetup; bases=-; ancestry=-; ns=KSP.UI.Screens.Settings
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: none; fan_in=3 fan_out=5
- methods(10): FloatArrayToString, GetControlTypeFields, GetField, IntArrayToString, IsValid, SetupReflectionValues, StringArrayToString, StringToFloatArray, StringToIntArray, StringToStringArray
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsTemplate.cs
- 9 lines; kind=MonoBehaviour; type=SettingsTemplate; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Settings
- usings: UnityEngine
- unity: UnityEngine.UI=1, MonoBehaviour=1; unity_score=55
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsTemplateDualLayouts.cs
- 22 lines; kind=MonoBehaviour; type=SettingsTemplateDualLayouts; bases=SettingsTemplate; ancestry=SettingsTemplate/MonoBehaviour; ns=KSP.UI.Screens.Settings
- usings: UnityEngine
- unity: UnityEngine.UI=2; unity_score=59
- ksp refs: none; fan_in=2 fan_out=1
- methods(1): GetLayoutFlipFlop
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsValue.cs
- 15 lines; kind=attribute; type=SettingsValue; bases=Attribute; ancestry=Attribute; ns=KSP.UI.Screens.Settings
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=19 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsWindow.cs
- 16 lines; kind=MonoBehaviour; type=SettingsWindow; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Settings
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=36
- ksp refs: none; fan_in=6 fan_out=1
- methods(1): IsValid
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/SimpleLayout.cs
- 83 lines; kind=MonoBehaviour; type=SimpleLayout; bases=MonoBehaviour,ApplauncherLayout; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=10, GameObject=2, Transform=1, MonoBehaviour=1; unity_score=80
- ksp refs: none; fan_in=1 fan_out=3
- methods(11): GetBottomLeftSpacer, GetGameObject, GetKnowledgeBaseList, GetModList, GetModListBtnDown, GetModListBtnUp, GetModListDivider, GetModScrollRect, GetRectTransform, GetStockList, GetTopRightSpacer
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/SpaceCenter/BuildingPicker.cs
- 136 lines; kind=MonoBehaviour; type=BuildingPicker; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.SpaceCenter
- usings: UnityEngine, UnityEngine.UI
- unity: GameObject=5, SetActive=4, UnityEngine.UI=2, Transform=2, Coroutine=2, MonoBehaviour=1; unity_score=70
- ksp refs: InputLockManager=1; fan_in=0 fan_out=6
- methods(6): ConstructBuildingList, ConstructUI, HideUI, ShowUI, Start, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/SpaceCenter/BuildingPickerItem.cs
- 136 lines; kind=MonoBehaviour; type=BuildingPickerItem; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler,IPointerClickHandler; ancestry=MonoBehaviour; ns=KSP.UI.Screens.SpaceCenter
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: EventSystems=8, UnityEngine.UI=4, Transform=4, Lifecycle=2, Instantiate/Destroy=2, GameObject=2; unity_score=82
- ksp refs: GameEvents=2, FlightCamera=1; fan_in=1 fan_out=7
- methods(11): Awake, CreatePointer, DestroyPointer, OnBuildingClick, OnBuildingInView, OnDestroy, OnGameUnpause, OnPointerClick, OnPointerEnter, OnPointerExit, Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/SpaceCenter/MissionSummaryDialog/CrewWidget.cs
- 130 lines; kind=MonoBehaviour; type=CrewWidget; bases=MissionSummaryWidget; ancestry=MissionSummaryWidget/MonoBehaviour; ns=KSP.UI.Screens.SpaceCenter.MissionSummaryDialog
- usings: Expansions, Expansions.Missions, Expansions.Serenity, KSP.Localization, KSP.UI.Util, TMPro, UnityEngine, UnityEngine.UI
- unity: Texture=11, SetActive=4, GameObject=4, TMPro=2, UnityEngine.UI=1, Instantiate/Destroy=1; unity_score=81
- ksp refs: Localizer=5, ProtoCrewMember=4, KerbalRoster=4, HighLogic=3, GameDatabase=1, Game=1; fan_in=2 fan_out=17
- methods(3): Create, UpdateFields, selectSprite
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/SpaceCenter/MissionSummaryDialog/MissionSummaryWidget.cs
- 23 lines; kind=MonoBehaviour; type=MissionSummaryWidget; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.SpaceCenter.MissionSummaryDialog
- usings: TMPro, UnityEngine
- unity: UnityEngine.UI=3, TMPro=1, MonoBehaviour=1, GetComponent=1; unity_score=84
- ksp refs: none; fan_in=4 fan_out=3
- methods(1): Init
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/SpaceCenter/MissionSummaryDialog/PartWidget.cs
- 78 lines; kind=MonoBehaviour; type=PartWidget; bases=MissionSummaryWidget; ancestry=MissionSummaryWidget/MonoBehaviour; ns=KSP.UI.Screens.SpaceCenter.MissionSummaryDialog
- usings: KSP.Localization, KSP.UI.Util, UnityEngine
- unity: Instantiate/Destroy=1, GetComponent=1; unity_score=32
- ksp refs: Localizer=3, AvailablePart=3; fan_in=2 fan_out=7
- methods(5): AddDuplicate, Create, Equals, GetHashCode, UpdateFields
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/SpaceCenter/MissionSummaryDialog/ResourceWidget.cs
- 67 lines; kind=MonoBehaviour; type=ResourceWidget; bases=MissionSummaryWidget; ancestry=MissionSummaryWidget/MonoBehaviour; ns=KSP.UI.Screens.SpaceCenter.MissionSummaryDialog
- usings: KSP.Localization, KSP.UI.Util, UnityEngine
- unity: Instantiate/Destroy=1, GetComponent=1; unity_score=34
- ksp refs: Localizer=3, PartResourceDefinition=2; fan_in=2 fan_out=7
- methods(5): AddAmount, Create, Equals, GetHashCode, UpdateFields
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/SpaceCenter/MissionSummaryDialog/ScienceSubjectWidget.cs
- 39 lines; kind=MonoBehaviour; type=ScienceSubjectWidget; bases=MissionSummaryWidget; ancestry=MissionSummaryWidget/MonoBehaviour; ns=KSP.UI.Screens.SpaceCenter.MissionSummaryDialog
- usings: KSP.Localization, KSP.UI.Util, UnityEngine
- unity: Instantiate/Destroy=1, GetComponent=1; unity_score=44
- ksp refs: Localizer=3, ScienceSubject=2; fan_in=3 fan_out=7
- methods(2): Create, UpdateFields
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/SpaceTracking.cs **MUST-OPEN (>800 lines)**
- 1109 lines; kind=MonoBehaviour; type=SpaceTracking; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: CommNet, Expansions, Expansions.Missions, Expansions.Missions.Actions, Expansions.Missions.Editor, Expansions.Missions.Flow, Expansions.Missions.Runtime, FinePrint, KSP.IO, KSP.Localization, KSP.UI.Screens.Mapview, KSP.UI.TooltipTypes, UnityEngine, UnityEngine.UI
- unity: GameObject=29, SetActive=20, UnityEngine.UI=13, Coroutine=10, Instantiate/Destroy=8, GetComponent=6; unity_score=59
- ksp refs: HighLogic=47, GameEvents=30, Vessel=25, Game=18, Localizer=17, FlightGlobals=16; fan_in=7 fan_out=64
- methods(47): Awake, BtnOnClick_DeleteSelectedVessel, BtnOnClick_FlySelectedVessel, BtnOnClick_LeaveTrackingStation, BtnOnclick_RecoverSelectedVessel, BtnOnclick_TrackSelectedVessel, ClearUIList, CompleteStartUp, ConstructUIList, FlyVessel, GoToAndFocusVessel, MissionModeValid, MissionsObjectiveCallback, OnDestroy, OnDialogDismiss, OnMapViewFiltersModified, OnPlanetariumTargetChanged, OnRecoverConfirm, OnSceneLoadRequested, OnVesselDeleteConfirm, PostInitTgtFocus, RefreshMissionRequest, RequestVessel, SetVessel, Start, StartTrackingObject, StopTrackingObject, StopTrackingVessel, btnOnClick_MissionObjectives, btnOnClick_TrackedObjects, buildVesselsList, createMissionObjectivesItems, lockUI, onCBIconClicked, onInputLocksModified, onMissionNodeChanged, onSummaryDialogDespawn, onSummaryDialogSpawn, onTestGroupChanged, onVesselCreated, onVesselDestroyed, onVesselIconClick, setMissionObjectives, setRequestedVessel, setTrackedObjects ...
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/SpriteTumblerObject.cs
- 118 lines; kind=MonoBehaviour; type=SpriteTumblerObject; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: UnityEngine.UI=2, Texture=1, MonoBehaviour=1, Lifecycle=1; unity_score=55
- ksp refs: none; fan_in=1 fan_out=0
- methods(5): Awake, TumbleTo, UpdateDelta, afterAltModeTumbleTo, afterAltModeUpdateDelta
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/StageGroup.cs **MUST-OPEN (>800 lines)**
- 998 lines; kind=MonoBehaviour; type=StageGroup; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Transform=37, GameObject=22, UnityEngine.UI=19, SetActive=15, EventSystems=8, GetComponent=5; unity_score=68
- ksp refs: GameEvents=13, Part=10, HighLogic=4, GameSettings=2, Localizer=1, FlightGlobals=1; fan_in=3 fan_out=22
- methods(48): AddIcon, AddIconAt, AddIcons, AddStageAfter, Awake, CacheLocalStrings, Delete, DeltaVAppAtmosphereChanged, DeltaVCalcsCompleted, DisableDeltaVHeading, DisableInfoPanel, DisableStockInfoPanelDisplays, EditorStarted, EnableDeltaVHeading, EnableInfoPanel, EnableStockInfoPanelDisplays, FindSymmetryGroupleader, FixGroupUIState, GetStageGroupIndex, Hover, HoverOut, MoveAllIconsInto, MoveIconsInto, OnBeginDrag, OnDestroy, OnDrag, OnEndDrag, OnEndDragCompleted, OnInfoPanelSlideComplete, OnInfoPanelSlideUpdate, OnLeavingScene, OnPointerClick, RemoveIcon, Reset, ResetAvailable, SetDeltaVHeading_OnDeltaVCalcsCompleted, SetDeltaVHeading_OnUpdate, SetInverseStageIndex, SetManualStageOffset, SetPartIndices, SetSiblingIndexes, SetUiStageIndex, SpawnInfoObject, ToggleInfoPanel, Update ...
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/StageIcon.cs **MUST-OPEN (>800 lines)**
- 1411 lines; kind=MonoBehaviour; type=StageIcon; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: SetActive=27, GameObject=27, Transform=22, UnityEngine.UI=13, EventSystems=9, Texture=3; unity_score=58
- ksp refs: Part=74, GameEvents=3, HighLogic=2, GameDatabase=1; fan_in=4 fan_out=20
- methods(47): AddToGroup, Awake, Blink, BlinkBorder, ClearInfoBoxes, CollapseGroup, ConsolidateMembers, DisableGroup, DisplayInfo, ExpandGroup, ExpandGroupInUIOnly, ForceDisplayInfo, Freeze, Highlight, HighlightPart, HoldGroupedIcons, MouseInput_pointerEnter, MouseInput_pointerExit, OnBeginDrag, OnClick, OnDrag, OnEndDrag, OnFalse, OnTrue, RemoveFromGroup, RemoveFromGroupAndReshuffle, RemoveInfo, Reset, ResetAvailable, ResetStageGroup, ResetStageGroupAvailable, ResetStageIconSymmetryGroup, SetBackgroundColor, SetBorderColor, SetIcon, SetIconColor, SetInStageIndex, SetInStageIndexOFGroupedIcons, SetInverseSequenceIndex, SetManualStageOffset, SetSymmetryMarkers, SetSymmetryText, Setup, ShowBorder, SortGroupedIcons ...
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/StageIconInfoBox.cs
- 97 lines; kind=MonoBehaviour; type=StageIconInfoBox; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, SetActive=3, GameObject=3, TMPro=2, MonoBehaviour=1, Lifecycle=1; unity_score=78
- ksp refs: none; fan_in=2 fan_out=1
- methods(10): Awake, Collapse, Expand, SetCaption, SetMessage, SetMsgBgColor, SetMsgTextColor, SetProgressBarBgColor, SetProgressBarColor, SetValue
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/StageManager.cs **MUST-OPEN (>800 lines)**
- 1801 lines; kind=MonoBehaviour; type=StageManager; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Transform=48, GameObject=21, UnityEngine.UI=20, SetActive=12, Instantiate/Destroy=11, Coroutine=6; unity_score=57
- ksp refs: Part=45, GameEvents=40, HighLogic=17, FlightGlobals=9, MapView=8, EditorLogic=7; fan_in=33 fan_out=41
- methods(81): ActivateNextStage, ActivateStage, AddHeldIconsToStages, AddNewStageGroupsIfNeeded, AddPartsWithStageiconsToListRecursive, AddStageAt, Awake, BeginFlight, CacheLocalStrings, CheckNullrefsInStageIconList, CreateIcon, DecrementCurrentStage, DeleteEmptyStages, DeleteStage, DeltaVAppAtmosphereChanged, DeltaVButtonClicked, DeltaVCalcsCompleted, DisableDeltaVHeadings, DisableDeltaVTotal, DisableIcon, DisableInfoPanels, EnableDeltaVHeadings, EnableDeltaVTotal, EnableInfoPanels, FindLastInverseStage, FindPartsWithModuleSeparatingBeforeOtherPartsWithModule, FindStageIndices, ForceUpdateStageGroups, GenerateStagingSequence, GetNewStageIcon, GetNonDraggedStageGroupAboveGroup, GetParachuteStageIndex, GetStageCount, GetStageGroupInsertionIndex, GetStageGroupUnderCursor, HoldIcon, IncrementCurrentStage, InsertStageAt, OnDestroy, OnEditorLoad, OnEditorPartDeleted, OnEditorPartPicked, OnEditorPartPlaced, OnEditorPodDeleted, OnEditorPodSelected ...
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/SteamCraftInfo.cs
- 110 lines; kind=class; type=SteamCraftInfo; bases=-; ancestry=-; ns=KSP.UI.Screens
- usings: Steamworks
- unity: Steam=5; unity_score=35
- ksp refs: ConfigNode=6; fan_in=4 fan_out=3
- methods(3): CreateMetaData, ProcessMetaData, UpdateSteamState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/StrategyListItem.cs
- 74 lines; kind=MonoBehaviour; type=StrategyListItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.Events, UnityEngine.UI
- unity: EventSystems=2, UnityEngine.UI=1, Texture=1, TMPro=1, MonoBehaviour=1; unity_score=66
- ksp refs: none; fan_in=1 fan_out=4
- methods(4): Initialize, SetupButton, UpdateButton, updateTitle
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/SubassemblyDropZone.cs
- 203 lines; kind=MonoBehaviour; type=SubassemblyDropZone; bases=TooltipController; ancestry=TooltipController/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, KSP.UI.TooltipTypes, UnityEngine, UnityEngine.EventSystems
- unity: Coroutine=5, Lifecycle=3, EventSystems=2, GetComponent=1, GameObject=1; unity_score=51
- ksp refs: Localizer=15, EditorLogic=6, ShipConstruct=2, Part=2, InputLockManager=2, HighLogic=2; fan_in=1 fan_out=19
- methods(12): Awake, ButtonInputDelegate, EndDialogCoroutine, EscapeString, OnAppFocus, OnDestroy, OnDisable, OnPointerExit, OnSave, OnTooltipSpawned, SaveSubassembly, SpawnSaveDialog
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/TrackingStationObjectButton.cs
- 122 lines; kind=MonoBehaviour; type=TrackingStationObjectButton; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=8, Input.=4, GetComponent=3, Texture=2, Lifecycle=2, GameObject=2; unity_score=84
- ksp refs: none; fan_in=2 fan_out=1
- methods(10): Awake, Lock, OnMouseEnter, OnMouseExit, SetCount, SetState, ShowCount, Unlock, Update, UpdateState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/TrackingStationWidget.cs
- 204 lines; kind=MonoBehaviour; type=TrackingStationWidget; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: TMPro=3, UnityEngine.UI=2, Lifecycle=2, GetComponent=2, GameObject=2, EventSystems=2; unity_score=58
- ksp refs: Vessel=14, Localizer=9, Planetarium=2; fan_in=2 fan_out=11
- methods(12): Awake, CacheLocalStrings, CheckManeuverNode, GetMETString, GetManeuverString, GetMissionInfo, OnMouseEnter, OnMouseExit, SetVessel, Terminate, Update, onToggle
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Tumbler.cs
- 169 lines; kind=MonoBehaviour; type=Tumbler; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: Transform=8, Shader=5, Time.=3, Lifecycle=2, Renderer=1, MonoBehaviour=1; unity_score=69
- ksp refs: none; fan_in=0 fan_out=0
- methods(6): Awake, LateUpdate, SetColor, SetValue, TumbleTo, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/TypeButton.cs
- 35 lines; kind=class; type=TypeButton; bases=-; ancestry=-; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=3, UnityEngine.UI=2, TMPro=1; unity_score=65
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): Deselect, Select
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/uGUITumblerObject.cs
- 68 lines; kind=MonoBehaviour; type=uGUITumblerObject; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine
- unity: UnityEngine.UI=1, TMPro=1, MonoBehaviour=1, GetComponent=1; unity_score=57
- ksp refs: none; fan_in=0 fan_out=1
- methods(3): GetValue, TumbleTo, UpdateDelta
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/UIApp.cs
- 244 lines; kind=MonoBehaviour; type=UIApp; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.EventSystems
- unity: Coroutine=8, Lifecycle=2, Instantiate/Destroy=2, GameObject=2, EventSystems=2, Animation=2; unity_score=63
- ksp refs: GameEvents=2; fan_in=13 fan_out=4
- methods(27): AddToAppLauncher, Awake, DisablePanel, DisplayApp, EnablePanel, ForceAddToAppLauncher, GetAppScenes, GetAppScreenPos, Hide, HideApp, Hover, HoverOut, HoverOutCoroutine, MouseInput_PointerEnter, MouseInput_PointerExit, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnAppLauncherHide, OnAppLauncherReady, OnAppLauncherShow, OnAppStarted, OnDestroy, Reposition, Show, displayApp, hideApp
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/UIAppSpawner.cs
- 51 lines; kind=MonoBehaviour; type=UIAppSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: Lifecycle=2, GameObject=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=65
- ksp refs: GameEvents=2, HighLogic=1; fan_in=0 fan_out=6
- methods(3): Awake, OnDestroy, OnLevelLoaded
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/UIGridArea.cs
- 202 lines; kind=MonoBehaviour; type=UIGridArea; bases=MonoBehaviour,IScrollHandler,IEventSystemHandler; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Transform=8, UnityEngine.UI=6, Material=2, Lifecycle=2, EventSystems=2, TMPro=1; unity_score=71
- ksp refs: GameSettings=1; fan_in=1 fan_out=3
- methods(14): Awake, CalculateSnapBounds, CenterOfGrid, InputZoomDown, InputZoomUp, MousePointOnGrid, OnScroll, SetZoomButtonState, SetZoomLevelText, Snap, SnapToNode, Start, Zoom, ZoomTo
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/UIScrollRectState.cs
- 72 lines; kind=MonoBehaviour; type=UIScrollRectState; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, SetActive=2, GameObject=2, MonoBehaviour=1, Lifecycle=1; unity_score=66
- ksp refs: none; fan_in=1 fan_out=0
- methods(4): Awake, Disable, Enable, SetState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/UISpaceCenter.cs
- 261 lines; kind=MonoBehaviour; type=UISpaceCenter; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: Lifecycle=3, UnityEngine.UI=1, TMPro=1, SetActive=1, MonoBehaviour=1, KeyCode=1; unity_score=42
- ksp refs: GameEvents=29, InputLockManager=7, TimeWarp=2, GameSettings=2; fan_in=2 fan_out=10
- methods(22): Awake, OnApplicationFocus, OnDestroy, OnFacilityDespawn_AC, OnFacilityDespawn_ADM, OnFacilityDespawn_MC, OnFacilityDespawn_RD, OnFacilityDespawn_VSD, OnFacilitySpawn_AC, OnFacilitySpawn_ADM, OnFacilitySpawn_MC, OnFacilitySpawn_RD, OnFacilitySpawn_VSD, OnPauseMenuDismiss, Pause, QuitToMenu, SetBuildingText, UnPause, Update, lockUI, onInputLockModified, unlockUI
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/VesselIconSprite.cs
- 34 lines; kind=MonoBehaviour; type=VesselIconSprite; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, Texture=1, MonoBehaviour=1; unity_score=69
- ksp refs: none; fan_in=1 fan_out=1
- methods(1): SetType
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/VesselRenameDialog.cs
- 333 lines; kind=MonoBehaviour; type=VesselRenameDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: Expansions, KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.Events, UnityEngine.UI
- unity: SetActive=11, GameObject=11, UnityEngine.UI=5, Transform=4, TMPro=3, Instantiate/Destroy=3; unity_score=76
- ksp refs: Vessel=3, Localizer=2, InputLockManager=2, GameSettings=2, Part=1, HighLogic=1; fan_in=2 fan_out=16
- methods(14): OnButtonAccept, OnButtonDismiss, OnButtonRemove, OnNameFieldEndEdit, OnNameFieldModified, OnNameFieldSelected, OnPriorityChanged, OnToggle, Spawn, SpawnNameFromPart, Start, Terminate, ToggleSetup, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/VesselSpawnDialog.cs **MUST-OPEN (>800 lines)**
- 1526 lines; kind=MonoBehaviour; type=VesselSpawnDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: Expansions, KSP.Localization, PreFlightTests, SaveUpgradePipeline, Steamworks, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.Networking, UnityEngine.UI
- unity: UnityEngine.UI=36, GameObject=33, SetActive=32, Steam=25, Coroutine=20, TMPro=18; unity_score=76
- ksp refs: Localizer=50, HighLogic=21, GameEvents=9, ConfigNode=9, GameSettings=3, GameDatabase=3; fan_in=6 fan_out=63
- methods(60): AddCraftEntryWidget, AddVesselDataItem, AddVesselListItem, AstronautComplexDespawn, AstronautComplexSpawn, Awake, BuildSteamCraftList, ButtonClose, ButtonDelete, ButtonEdit, ButtonLaunch, ClearCraftList, ClearSelection, ConfirmLaunch, CraftSelectedCallback, CreateVesselList, GetCostTextColorHex, GetMassTextColorHex, GetPartCountTextColorHex, GetSizeTextColorHex, GetSteamFileId, GetVesselDataItem, InitiateGUI, LateSelectFireThing, LaunchSelectedVessel, LoadSteamItemPreviewURL, OnBtnSteamOverlay, OnDeleteConfirm, OnDeleteDismiss, OnDestroy, OnEntrySelected, OnFlagSelected, OnSelectionChanged, OnVesselListSort, SaveCraftSteamUnsubscribeWarning, SelectVesselDataItem, Start, SteamItemSubscribed, UpdateSteamListOfCraft, UpdateSteamSubscribedItems, UpdateVesselInfoPanel, getSteamWorkshopCraft, onBtnSteamItemDetails, onButtonSteamSubUnsub, onEditBtnAbort ...
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/VSDSceneSpawner.cs
- 34 lines; kind=MonoBehaviour; type=VSDSceneSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1, Coroutine=1; unity_score=63
- ksp refs: GameEvents=5; fan_in=0 fan_out=6
- methods(4): Awake, OnDestroy, onVSDDespawn, onVSDSpawn
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/SimpleAppFrame.cs
- 55 lines; kind=MonoBehaviour; type=SimpleAppFrame; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: KSP.UI.Screens, UnityEngine, UnityEngine.EventSystems, UnityEngine.Events
- unity: Transform=3, UnityEngine.UI=2, Lifecycle=2, GameObject=2, EventSystems=2, MonoBehaviour=1; unity_score=82
- ksp refs: none; fan_in=1 fan_out=3
- methods(5): AddGlobalInputDelegate, Awake, OnDestroy, Reposition, Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/SliderValueToText.cs
- 40 lines; kind=MonoBehaviour; type=SliderValueToText; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, TMPro=1, MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=77
- ksp refs: Localizer=1; fan_in=0 fan_out=2
- methods(2): Awake, SliderValueChangeListener
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/StageManagerParentResizer.cs
- 40 lines; kind=MonoBehaviour; type=StageManagerParentResizer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: UnityEngine.UI=3, Lifecycle=2, Transform=1, MonoBehaviour=1; unity_score=82
- ksp refs: none; fan_in=0 fan_out=2
- methods(3): Awake, OnDestroy, Resize
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Tooltip.cs
- 16 lines; kind=MonoBehaviour; type=Tooltip; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: UnityEngine.UI=4, Transform=1, MonoBehaviour=1, Lifecycle=1; unity_score=86
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): Awake
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/TooltipController.cs
- 115 lines; kind=MonoBehaviour; type=TooltipController; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler,ITooltipController; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: EventSystems=4, UnityEngine.UI=3, Lifecycle=3, MonoBehaviour=1, GetComponent=1, GameObject=1; unity_score=68
- ksp refs: GameEvents=2; fan_in=9 fan_out=3
- methods(11): Awake, OnAppFocus, OnDestroy, OnDisable, OnPointerEnter, OnPointerExit, OnTooltipAboutToDespawn, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipSpawned, OnTooltipUpdate
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/TooltipTypes/Tooltip_Text.cs
- 9 lines; kind=MonoBehaviour; type=Tooltip_Text; bases=Tooltip; ancestry=Tooltip/MonoBehaviour; ns=KSP.UI.TooltipTypes
- usings: TMPro
- unity: TMPro=1; unity_score=44
- ksp refs: none; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/TooltipTypes/Tooltip_TitleAndText.cs
- 11 lines; kind=MonoBehaviour; type=Tooltip_TitleAndText; bases=Tooltip; ancestry=Tooltip/MonoBehaviour; ns=KSP.UI.TooltipTypes
- usings: TMPro
- unity: TMPro=2; unity_score=59
- ksp refs: none; fan_in=6 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/TooltipTypes/TooltipController_CrewAC.cs
- 86 lines; kind=MonoBehaviour; type=TooltipController_CrewAC; bases=TooltipController; ancestry=TooltipController/MonoBehaviour; ns=KSP.UI.TooltipTypes
- usings: KSP.Localization
- unity: none; unity_score=15
- ksp refs: Localizer=5, HighLogic=5, ProtoCrewMember=3, KerbalRoster=1; fan_in=3 fan_out=8
- methods(4): CacheLocalStrings, OnTooltipAboutToSpawn, OnTooltipSpawned, SetTooltip
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/TooltipTypes/TooltipController_Text.cs
- 60 lines; kind=MonoBehaviour; type=TooltipController_Text; bases=TooltipController; ancestry=TooltipController/MonoBehaviour; ns=KSP.UI.TooltipTypes
- usings: KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=27
- ksp refs: Localizer=1; fan_in=34 fan_out=3
- methods(6): Awake, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipSpawned, OnTooltipUpdate, SetText
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/TooltipTypes/TooltipController_TitleAndText.cs
- 38 lines; kind=MonoBehaviour; type=TooltipController_TitleAndText; bases=TooltipController; ancestry=TooltipController/MonoBehaviour; ns=KSP.UI.TooltipTypes
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=4 fan_out=2
- methods(3): OnTooltipAboutToSpawn, OnTooltipSpawned, OnTooltipUpdate
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/TooltipTypes/UIRadioButtonTooltip.cs
- 74 lines; kind=MonoBehaviour; type=UIRadioButtonTooltip; bases=TooltipController; ancestry=TooltipController/MonoBehaviour; ns=KSP.UI.TooltipTypes
- usings: UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=2, Lifecycle=1, GetComponent=1; unity_score=50
- ksp refs: none; fan_in=0 fan_out=3
- methods(6): Awake, OnFalse, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipSpawned, OnTrue
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/TooltipTypes/UIStateButtonTooltip.cs
- 73 lines; kind=MonoBehaviour; type=UIStateButtonTooltip; bases=TooltipController; ancestry=TooltipController/MonoBehaviour; ns=KSP.UI.TooltipTypes
- usings: UnityEngine
- unity: Lifecycle=1, GetComponent=1; unity_score=33
- ksp refs: none; fan_in=0 fan_out=4
- methods(6): Awake, FindButtonStateTooltip, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipSpawned, OnValueChanged
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/TweeningController.cs
- 101 lines; kind=MonoBehaviour; type=TweeningController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: UnityEngine.UI=6, GameObject=6, Transform=5, GetComponent=4, Lifecycle=2, SetActive=1; unity_score=85
- ksp refs: GameEvents=2; fan_in=4 fan_out=4
- methods(7): Awake, OnDestroy, OnGameSceneLoadRequested, OnTweeningComplete, SpawnResizingLayoutElement, Tween, TweenIntoList
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/TweeningElement.cs
- 103 lines; kind=MonoBehaviour; type=TweeningElement; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: Transform=14, UnityEngine.UI=5, Coroutine=3, Time.=2, MonoBehaviour=1, Lifecycle=1; unity_score=86
- ksp refs: none; fan_in=1 fan_out=1
- methods(4): Awake, EaseInBack, StartTweening, Tween
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/U5Util.cs
- 57 lines; kind=static class; type=U5Util; bases=-; ancestry=-; ns=KSP.UI
- usings: UnityEngine, UnityEngine.UI
- unity: Transform=6, UnityEngine.UI=5, GameObject=2, LayerMask=1; unity_score=70
- ksp refs: none; fan_in=6 fan_out=0
- methods(4): ColorToHex, GetSimulatedScrollMultiplier, HexToColor, SetLayerRecursive
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIAvailability.cs
- 9 lines; kind=enum; type=UIAvailability; bases=-; ancestry=-; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIButtonToggle.cs
- 102 lines; kind=MonoBehaviour; type=UIButtonToggle; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=11, Texture=2, MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=79
- ksp refs: none; fan_in=21 fan_out=0
- methods(3): Awake, OnClick, SetState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UICameraBase.cs
- 15 lines; kind=MonoBehaviour; type=UICameraBase; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: Camera=2, MonoBehaviour=1, GetComponent=1; unity_score=74
- ksp refs: none; fan_in=3 fan_out=0
- methods(1): Reset
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UICanvasPrefab.cs
- 11 lines; kind=MonoBehaviour; type=UICanvasPrefab; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: UnityEngine.UI=1, MonoBehaviour=1; unity_score=55
- ksp refs: none; fan_in=9 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UICanvasPrefabSpawner.cs
- 37 lines; kind=MonoBehaviour; type=UICanvasPrefabSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, GameObject=1; unity_score=64
- ksp refs: none; fan_in=0 fan_out=2
- methods(1): Awake
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UICascadingList.cs
- 205 lines; kind=MonoBehaviour; type=UICascadingList; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=6, Transform=4, GetComponent=4, MonoBehaviour=1, EventSystems=1; unity_score=58
- ksp refs: none; fan_in=4 fan_out=3
- methods(11): Add, AddCascadingItem, AddSegment, AddSegmentBody, AddSegmentFooter, AddSegmentHeader, ClearList, RemoveCascadingItem, SegmentHeaderButtonInput, UpdateBodiesCallback, UpdateCascadingItem
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UICraftSaveFlyoutController.cs
- 308 lines; kind=MonoBehaviour; type=UICraftSaveFlyoutController; bases=UIHoverSlidePanel; ancestry=UIHoverSlidePanel/MonoBehaviour; ns=KSP.UI
- usings: Expansions.Missions, Expansions.Missions.Runtime, KSP.Localization, KSP.UI.Screens, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=8, Coroutine=6, Lifecycle=5, UnityEngine.UI=4, Transform=4, SetActive=4; unity_score=70
- ksp refs: MissionSystem=2, Localizer=2, HighLogic=2, GameEvents=2, Game=2; fan_in=2 fan_out=14
- methods(19): CloseFlyout, CreateFolderSaveItem, GetSubdirectoryList, OnDestroy, OnDisable, OnEnable, OnPointerEnter, OnPointerExit, OpenCraftFolderBrowser, ResetItems, SaveFolderSelectable_onPointerEnter, SaveFolderSelectable_onPointerExit, SetupSaveFolderChoices, SetupToggleGroup, Start, Update, UpdateFolderDisplay, UpdateSaveLocationForMissions, UpdateSelectionRadioButtons
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIDialogCamera.cs
- 24 lines; kind=MonoBehaviour; type=UIDialogCamera; bases=UICameraBase; ancestry=UICameraBase/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: Lifecycle=2, Camera=2; unity_score=72
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): Awake, OnDestroy
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIDragAndDropController.cs
- 61 lines; kind=MonoBehaviour; type=UIDragAndDropController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: UnityEngine.UI=5, Transform=3, Lifecycle=2, GetComponent=2, GameObject=2, SetActive=1; unity_score=86
- ksp refs: none; fan_in=2 fan_out=2
- methods(4): Awake, OnDestroy, Register, Unregister
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIDragPanel.cs
- 139 lines; kind=MonoBehaviour; type=UIDragPanel; bases=DragHandler; ancestry=DragHandler/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=7, Transform=6, UnityEngine.UI=4, Input.=2, GameObject=2, Instantiate/Destroy=1; unity_score=78
- ksp refs: InputLockManager=2; fan_in=4 fan_out=4
- methods(3): OnBeginDrag, OnDrag, OnEndDrag
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIExtensions.cs
- 70 lines; kind=static class; type=UIExtensions; bases=-; ancestry=-; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.Events, UnityEngine.UI
- unity: Transform=9, UnityEngine.UI=5, GameObject=3, EventSystems=2; unity_score=72
- ksp refs: none; fan_in=0 fan_out=0
- methods(9): AddEvent, Clear, ClearChildren, ClearChildrenImmediate, CopyValues, Lock, OnEventDelegate, SetLocalPositionZ, Unlock
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIHoverPanel.cs
- 79 lines; kind=MonoBehaviour; type=UIHoverPanel; bases=PointerEnterExitHandler; ancestry=PointerEnterExitHandler/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=5, Texture=2, EventSystems=2, UnityEngine.UI=1, SetActive=1, Lifecycle=1; unity_score=74
- ksp refs: none; fan_in=3 fan_out=1
- methods(8): Awake, DisableHover, EnableHover, OnPointerEnter, OnPointerExit, PointerEnter, PointerExit, SetStateObjects
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIHoverSlidePanel.cs
- 115 lines; kind=MonoBehaviour; type=UIHoverSlidePanel; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems
- unity: Coroutine=7, SetActive=4, EventSystems=4, UnityEngine.UI=2, GameObject=2, Time.=1; unity_score=80
- ksp refs: none; fan_in=5 fan_out=0
- methods(5): MoveToState, OnPointerEnter, OnPointerExit, Reset, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIHoverText.cs
- 71 lines; kind=UIBehaviour; type=UIHoverText; bases=XHoverable; ancestry=XHoverable/UIBehaviour; ns=KSP.UI
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=3, TMPro=2, Lifecycle=2, EventSystems=2, GetComponent=1; unity_score=79
- ksp refs: none; fan_in=5 fan_out=2
- methods(6): OnDisable, OnHover, OnUnhover, Start, UpdateTargetControl, UpdateText
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIHoverToggler.cs
- 71 lines; kind=UIBehaviour; type=UIHoverToggler; bases=XSelectable; ancestry=XSelectable/UIBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: EventSystems=3, SetActive=2, Lifecycle=2, UnityEngine.UI=1, GetComponent=1, GameObject=1; unity_score=75
- ksp refs: none; fan_in=1 fan_out=1
- methods(6): OnDeselect, OnDisable, OnHover, OnUnhover, SetText, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIInputExtraMouseButtons.cs
- 60 lines; kind=MonoBehaviour; type=UIInputExtraMouseButtons; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=3, Input.=3, Lifecycle=2, EventSystems=2, MonoBehaviour=1, GetComponent=1; unity_score=83
- ksp refs: none; fan_in=0 fan_out=0
- methods(4): Awake, OnMouseEnter, OnMouseExit, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UILaunchsiteController.cs
- 180 lines; kind=MonoBehaviour; type=UILaunchsiteController; bases=UIHoverSlidePanel; ancestry=UIHoverSlidePanel/MonoBehaviour; ns=KSP.UI
- usings: KSP.UI.Screens, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Coroutine=6, Lifecycle=4, EventSystems=4, UnityEngine.UI=2, Transform=2, GameObject=2; unity_score=69
- ksp refs: GameEvents=2; fan_in=1 fan_out=8
- methods(13): OnDestroy, OnDisable, OnEnable, OnPointerEnter, OnPointerExit, Start, addlaunchPadItem, launchSitesSelectable_onPointerEnter, launchSitesSelectable_onPointerExit, resetItems, setSelectedItem, setupItems, setupToggleGroup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIList.cs
- 398 lines; kind=MonoBehaviour; type=UIList; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.Events
- unity: Transform=26, SetActive=7, GameObject=7, UnityEngine.UI=5, MonoBehaviour=1, Lifecycle=1; unity_score=67
- ksp refs: none; fan_in=31 fan_out=5
- methods(18): AddItem, Awake, Clear, ClearUI, Contains, GetDropItemIndex, GetEnumerator, GetIndex, GetUiListItems, GetUilistItemAt, InsertItem, Refresh, RemoveItem, RemoveItemAndMove, SetActive, SetAnchor, Sort, SwapItems
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIListData.cs
- 15 lines; kind=class; type=UIListData; bases=-; ancestry=-; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIListDropArea.cs
- 61 lines; kind=MonoBehaviour; type=UIListDropArea; bases=DropHandler; ancestry=DropHandler/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine.EventSystems
- unity: GetComponent=3, Lifecycle=1, EventSystems=1; unity_score=57
- ksp refs: none; fan_in=0 fan_out=4
- methods(2): Awake, OnDrop
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIListItem.cs
- 74 lines; kind=MonoBehaviour; type=UIListItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, TMPro=3, GetComponent=3, MonoBehaviour=1; unity_score=73
- ksp refs: none; fan_in=43 fan_out=2
- methods(3): GetButtonElement, GetStateButtonElement, GetTextElement
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIListItem_spacer.cs
- 9 lines; kind=MonoBehaviour; type=UIListItem_spacer; bases=UIListItem; ancestry=UIListItem/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine.UI
- unity: UnityEngine.UI=1; unity_score=44
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIListMover.cs
- 71 lines; kind=MonoBehaviour; type=UIListMover; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: UnityEngine.UI=1, MonoBehaviour=1; unity_score=40
- ksp refs: none; fan_in=0 fan_out=2
- methods(1): SetAnchor
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIListSorter.cs
- 117 lines; kind=MonoBehaviour; type=UIListSorter; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1; unity_score=27
- ksp refs: none; fan_in=5 fan_out=1
- methods(6): AddOnSortCallback, ClickButton, OnSort, RemoveOnSortCallback, SetSortingMode, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIListToggleController.cs
- 34 lines; kind=MonoBehaviour; type=UIListToggleController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: SetActive=1, MonoBehaviour=1, Lifecycle=1; unity_score=56
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): ActivateList, RefreshCurrent, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIMainCamera.cs
- 34 lines; kind=MonoBehaviour; type=UIMainCamera; bases=UICameraBase; ancestry=UICameraBase/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: Camera=3, Lifecycle=2; unity_score=75
- ksp refs: none; fan_in=7 fan_out=1
- methods(2): Awake, OnDestroy
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIMasterController.cs
- 792 lines; kind=MonoBehaviour; type=UIMasterController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=53, GameObject=21, Transform=8, Instantiate/Destroy=7, Screen/Cursor=5, Lifecycle=5; unity_score=75
- ksp refs: GameEvents=14, GameSettings=5, InputLockManager=3; fan_in=74 fan_out=11
- methods(45): AddCanvas, AnyCornerOffScreen, Awake, CanSpawnCanvasPrefab, CheckScreenResolution, ClampToScreen, ClampToWindow, CutToScreen, CutToWindow, DespawnTooltip, DestroyCurrentTooltip, DragTooltip, FocusModalDialog, GetCanvasWrapper, GetMaxSuggestedUIScale, HideUI, LateUpdate, OnDestroy, OnHideUI, OnSceneChange, OnShowUI, PinTooltip, RegisterModalDialog, RegisterNonModalDialog, RemoveCanvas, RepositionTooltip, SetAppScale, SetNavigationMode, SetScale, SetUIActionCanvasPixelPerfect, SetUIMainCanvasPixelPerfect, SetUITooltipCanvasPixelPerfect, SetupCameraAndScale, SetupNavigationMode, ShowUI, SpawnTooltip, Start, UnpinTooltip, UnregisterModalDialog, UnregisterModalDialogs, UnregisterNonModalDialog, Update, UpdatePosition, UpdateTooltip, WorldToMainCanvas
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIOnClick.cs
- 48 lines; kind=MonoBehaviour; type=UIOnClick; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: EventSystems=4, UnityEngine.UI=2, MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=82
- ksp refs: none; fan_in=2 fan_out=1
- methods(2): Awake, OnPointerClick
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIOnHover.cs
- 23 lines; kind=MonoBehaviour; type=UIOnHover; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=4, EventSystems=4, MonoBehaviour=1; unity_score=93
- ksp refs: none; fan_in=1 fan_out=0
- methods(2): OnPointerEnter, OnPointerExit
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIPanelTransition.cs
- 238 lines; kind=MonoBehaviour; type=UIPanelTransition; bases=UITransitionBase; ancestry=UITransitionBase/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.Events
- unity: Coroutine=5, SetActive=4, Lifecycle=2, GameObject=2, UnityEngine.UI=1, Time.=1; unity_score=52
- ksp refs: none; fan_in=13 fan_out=1
- methods(7): Awake, GetState, Reset, Start, Transition, TransitionImmediate, TransitionState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIPanelTransitionManager.cs
- 116 lines; kind=MonoBehaviour; type=UIPanelTransitionManager; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: Coroutine=4, MonoBehaviour=1; unity_score=41
- ksp refs: none; fan_in=1 fan_out=1
- methods(4): AnyPanelsTransitioning, BringIn, Dismiss, PanelTransitionIn
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIPanelTransitionRTScaler.cs
- 93 lines; kind=MonoBehaviour; type=UIPanelTransitionRTScaler; bases=UITransitionBase; ancestry=UITransitionBase/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: Coroutine=4, Time.=1, Lifecycle=1; unity_score=57
- ksp refs: none; fan_in=0 fan_out=2
- methods(4): Awake, GetState, Transition, TransitionState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIPanelTransitionToggle.cs
- 45 lines; kind=MonoBehaviour; type=UIPanelTransitionToggle; bases=UIPanelTransition; ancestry=UIPanelTransition/UITransitionBase/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: Lifecycle=1, GameObject=1; unity_score=53
- ksp refs: none; fan_in=4 fan_out=1
- methods(5): Collapse, CollapseImmediate, Expand, ExpandImmediate, OnValidate
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIPanelTweener.cs
- 90 lines; kind=MonoBehaviour; type=UIPanelTweener; bases=UITransitionBase; ancestry=UITransitionBase/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.UI
- unity: Coroutine=4, UnityEngine.UI=3, GetComponent=2, Time.=1, Lifecycle=1; unity_score=68
- ksp refs: none; fan_in=1 fan_out=1
- methods(6): Awake, OnTransitionComplete, Reset, SetInteractable, Transition, TransitionState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIPlanetariumDateTime.cs
- 54 lines; kind=MonoBehaviour; type=UIPlanetariumDateTime; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: TMPro, UnityEngine
- unity: Lifecycle=3, TMPro=1, MonoBehaviour=1; unity_score=61
- ksp refs: GameEvents=4, Planetarium=2; fan_in=0 fan_out=5
- methods(6): OnDestroy, SetTime, Start, Update, onGamePause, onGameUnPause
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIPrefabSpawner.cs
- 70 lines; kind=MonoBehaviour; type=UIPrefabSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: UnityEngine.UI=3, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1, Coroutine=1; unity_score=67
- ksp refs: none; fan_in=0 fan_out=0
- methods(3): Awake, Spawn, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIRadioButton.cs
- 295 lines; kind=MonoBehaviour; type=UIRadioButton; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler,IRadioButton; ancestry=MonoBehaviour; ns=KSP.UI
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.Events, UnityEngine.UI
- unity: EventSystems=24, UnityEngine.UI=21, Lifecycle=2, GetComponent=2, GameObject=2, TMPro=1; unity_score=83
- ksp refs: GameEvents=2; fan_in=32 fan_out=6
- methods(7): Awake, MenuNavigationInputListener, OnDestroy, PopOtherButtonsInGroup, SetGroup, SetState, ToggleState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIRadioButtonColorChanger.cs
- 161 lines; kind=MonoBehaviour; type=UIRadioButtonColorChanger; bases=MonoBehaviour,IPointerDownHandler,IEventSystemHandler,IPointerEnterHandler,IPointerExitHandler,IRadioButton; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems
- unity: UnityEngine.UI=10, EventSystems=8, Lifecycle=3, MonoBehaviour=1, GetComponent=1; unity_score=77
- ksp refs: none; fan_in=0 fan_out=4
- methods(10): Awake, OnDestroy, OnFalse, OnPointerDown, OnPointerEnter, OnPointerExit, OnTrue, PopOtherButtonsInGroup, SetGroup, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIRadioButtonStateChanger.cs
- 60 lines; kind=MonoBehaviour; type=UIRadioButtonStateChanger; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=47
- ksp refs: none; fan_in=1 fan_out=2
- methods(2): Awake, SetState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIRectScaler.cs
- 114 lines; kind=MonoBehaviour; type=UIRectScaler; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: Lifecycle=4, UnityEngine.UI=3, Screen/Cursor=2, GetComponent=2, MonoBehaviour=1; unity_score=65
- ksp refs: GameSettings=4, GameEvents=2; fan_in=1 fan_out=3
- methods(8): Awake, OnDestroy, OverFlowProtector, Set, SetGameSettings, SetGameSettingsReciprocal, Start, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UISelectableGridLayoutGroup.cs
- 224 lines; kind=MonoBehaviour; type=UISelectableGridLayoutGroup; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: KSP.UI.Screens, UnityEngine, UnityEngine.UI
- unity: Transform=8, Instantiate/Destroy=5, GameObject=4, UnityEngine.UI=3, GetComponent=2, MonoBehaviour=1; unity_score=63
- ksp refs: none; fan_in=2 fan_out=3
- methods(10): AddItem, Awake, Clear, ClickItem, CreateItem, InsertItem, OnGridSelectItem, RemoveItem, Select, SelectSet
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UISelectableGridLayoutGroupItem.cs
- 51 lines; kind=MonoBehaviour; type=UISelectableGridLayoutGroupItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=6, Transform=1, MonoBehaviour=1, GetComponent=1; unity_score=81
- ksp refs: none; fan_in=11 fan_out=1
- methods(3): Deselect, Select, Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UISelectableList.cs
- 165 lines; kind=MonoBehaviour; type=UISelectableList; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.Events, UnityEngine.UI
- unity: UnityEngine.UI=8, Instantiate/Destroy=4, GameObject=2, Transform=1, MonoBehaviour=1, Lifecycle=1; unity_score=68
- ksp refs: none; fan_in=3 fan_out=1
- methods(10): AddItem, Clear, ClickItem, CreateAndInsertItem, CreateItem, InsertItem, OnDestroy, RemoveItem, Select, UpdateObjects
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UISelectableListItem.cs
- 24 lines; kind=MonoBehaviour; type=UISelectableListItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=36
- ksp refs: none; fan_in=3 fan_out=1
- methods(3): Deselect, Select, SetupListItem
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UISelectableListItem_ColorButton.cs
- 81 lines; kind=MonoBehaviour; type=UISelectableListItem_ColorButton; bases=UISelectableListItem; ancestry=UISelectableListItem/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=7, GetComponent=2, Lifecycle=1; unity_score=75
- ksp refs: none; fan_in=0 fan_out=3
- methods(5): Awake, Deselect, Select, SetState, SetupListItem
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UISelectableListItem_StateButton.cs
- 90 lines; kind=MonoBehaviour; type=UISelectableListItem_StateButton; bases=UISelectableListItem; ancestry=UISelectableListItem/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=9, Texture=4, GetComponent=3, Lifecycle=1; unity_score=83
- ksp refs: none; fan_in=0 fan_out=3
- methods(5): Awake, Deselect, Select, SetState, SetupListItem
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIShipDescriptionHoverSlidePanel.cs
- 92 lines; kind=MonoBehaviour; type=UIShipDescriptionHoverSlidePanel; bases=UIHoverSlidePanel; ancestry=UIHoverSlidePanel/MonoBehaviour; ns=KSP.UI
- usings: TMPro, UnityEngine.EventSystems
- unity: EventSystems=4, TMPro=2, Coroutine=2, Lifecycle=1; unity_score=66
- ksp refs: EditorLogic=3; fan_in=0 fan_out=4
- methods(7): InputFieldSelectable_onPointerEnter, InputFieldSelectable_onPointerExit, OnEndEdit, OnPointerEnter, OnPointerExit, Start, UpdateHoverState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIStateButton.cs
- 138 lines; kind=MonoBehaviour; type=UIStateButton; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.Events, UnityEngine.UI
- unity: UnityEngine.UI=19, EventSystems=7, GetComponent=3, MonoBehaviour=1, Lifecycle=1; unity_score=88
- ksp refs: none; fan_in=17 fan_out=1
- methods(5): Awake, Enable, Reset, SetState, ToggleState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIStateImage.cs
- 76 lines; kind=MonoBehaviour; type=UIStateImage; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, Texture=2, Lifecycle=2, MonoBehaviour=1, GetComponent=1; unity_score=68
- ksp refs: GameEvents=4, TimeWarp=1; fan_in=20 fan_out=2
- methods(6): Awake, OnDestroy, Reset, SetState, onGamePause, onGameUnPause
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIStatePanel.cs
- 113 lines; kind=MonoBehaviour; type=UIStatePanel; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: TMPro, UnityEngine
- unity: SetActive=2, GameObject=2, UnityEngine.UI=1, TMPro=1, MonoBehaviour=1, Lifecycle=1; unity_score=57
- ksp refs: none; fan_in=5 fan_out=2
- methods(6): Awake, Disable, Enable, SetState, SetText, SetTextOverflowMode
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIStateRawImage.cs
- 53 lines; kind=MonoBehaviour; type=UIStateRawImage; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, Texture=1, MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=70
- ksp refs: none; fan_in=4 fan_out=0
- methods(3): Awake, Reset, SetState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIStateText.cs
- 56 lines; kind=MonoBehaviour; type=UIStateText; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: TMPro, UnityEngine
- unity: TMPro=2, MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=64
- ksp refs: none; fan_in=2 fan_out=1
- methods(3): Awake, Reset, SetState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIStateToggleButton.cs
- 191 lines; kind=MonoBehaviour; type=UIStateToggleButton; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=27, Texture=4, EventSystems=4, Lifecycle=2, GetComponent=2, MonoBehaviour=1; unity_score=86
- ksp refs: none; fan_in=2 fan_out=1
- methods(6): Awake, OnPointerClick, SetState, Setup, Start, ToggleState
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UITextButton.cs
- 135 lines; kind=MonoBehaviour; type=UITextButton; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: EventSystems=5, UnityEngine.UI=2, Lifecycle=2, GetComponent=2, MonoBehaviour=1; unity_score=65
- ksp refs: none; fan_in=0 fan_out=0
- methods(8): Awake, OnMouseClick, OnMouseDown, OnMouseEnter, OnMouseExit, OnMouseUp, SetState, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UITimeWarpController.cs
- 169 lines; kind=MonoBehaviour; type=UITimeWarpController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: KSP.Localization, KSP.UI.TooltipTypes, UnityEngine, UnityEngine.UI
- unity: SetActive=4, GameObject=4, UnityEngine.UI=2, Lifecycle=2, MonoBehaviour=1, GetComponent=1; unity_score=57
- ksp refs: TimeWarp=20, Localizer=4, GameEvents=2, InputLockManager=1, HighLogic=1; fan_in=0 fan_out=9
- methods(12): CanSetHighRate, ClickTimewarp0, ClickTimewarp1, ClickTimewarp2, ClickTimewarp3, ClickTimewarp4, ClickTimewarp5, ClickTimewarp6, ClickTimewarp7, OnDestroy, OnTimeWarpRateChanged, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UITimeWarpHoverController.cs
- 48 lines; kind=MonoBehaviour; type=UITimeWarpHoverController; bases=UIHoverSlidePanel; ancestry=UIHoverSlidePanel/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: Lifecycle=2, Coroutine=2, Transform=1; unity_score=62
- ksp refs: TimeWarp=3; fan_in=0 fan_out=2
- methods(2): LateUpdate, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UITimeWarpScrubber.cs
- 50 lines; kind=MonoBehaviour; type=UITimeWarpScrubber; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: EventSystems=2, UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1, GetComponent=1, GameObject=1; unity_score=75
- ksp refs: Planetarium=4, TimeWarp=3; fan_in=0 fan_out=3
- methods(3): OnPointerDown, OnPointerUp, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UITogglePanel.cs
- 44 lines; kind=MonoBehaviour; type=UITogglePanel; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems
- unity: SetActive=4, GameObject=2, EventSystems=2, MonoBehaviour=1, Lifecycle=1; unity_score=82
- ksp refs: none; fan_in=0 fan_out=0
- methods(3): Awake, Hover, HoverOut
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UITransitionBase.cs
- 77 lines; kind=MonoBehaviour; type=UITransitionBase; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, Time.=1, MonoBehaviour=1, GetComponent=1; unity_score=69
- ksp refs: none; fan_in=3 fan_out=0
- methods(6): Lerp_Pre, Lerp_Transition, Linear_Pre, Linear_Transition, SetInteractable, SetMethod
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UITransparencyController.cs
- 153 lines; kind=MonoBehaviour; type=UITransparencyController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.SceneManagement
- unity: Shader=8, SceneManager=3, Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1, Material=1; unity_score=72
- ksp refs: HighLogic=2, GameSettings=2, GameEvents=2; fan_in=0 fan_out=4
- methods(8): OnDestroy, OnLevelLoaded, OnSceneLoaded, SetOpacity, Setup, Start, UpdateOpacity, onSettingsApplied
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UITreeView.cs
- 344 lines; kind=MonoBehaviour; type=UITreeView; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: KSP.Localization, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=9, Transform=4, Coroutine=3, Instantiate/Destroy=2, GetComponent=2, Time.=1; unity_score=55
- ksp refs: Localizer=4; fan_in=3 fan_out=2
- methods(14): AddItem, ClearItems, ClearItemsRecursive, FindItemByURL, GetItem, OnSetExpand, OnToggleExpand, Refresh, ScrollRectToLevelSelection, SelectFirstItem, SelectItem, SetupItemList, UnselectAllRecursive, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UITreeViewItem.cs
- 64 lines; kind=MonoBehaviour; type=UITreeViewItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=5, Texture=2, TMPro=1, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1; unity_score=81
- ksp refs: none; fan_in=1 fan_out=2
- methods(1): Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIVectorCamera.cs
- 24 lines; kind=MonoBehaviour; type=UIVectorCamera; bases=UICameraBase; ancestry=UICameraBase/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: Lifecycle=2, Camera=2; unity_score=72
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): Awake, OnDestroy
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIWarpToNextMorning.cs
- 166 lines; kind=MonoBehaviour; type=UIWarpToNextMorning; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: KSP.UI.Screens, UnityEngine, UnityEngine.UI
- unity: Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1; unity_score=34
- ksp refs: Vessel=14, FlightGlobals=12, HighLogic=5, TimeWarp=4, CelestialBody=3, Planetarium=1; fan_in=0 fan_out=12
- methods(5): CalculateTimeToDaylightFromVessel, Setup, Start, Update, WarpToMorning
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIWindow.cs
- 223 lines; kind=MonoBehaviour; type=UIWindow; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems
- unity: UnityEngine.UI=6, EventSystems=4, Lifecycle=3, Transform=1, MonoBehaviour=1; unity_score=58
- ksp refs: GameEvents=2; fan_in=2 fan_out=2
- methods(10): Awake, ClampToScreen, CutToScreen, MoveRectTransform, MoveWindow, OnDestroy, OnScreenResolutionModified, ResizeRectTransform, ResizeWindow, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIWindowArea.cs
- 97 lines; kind=MonoBehaviour; type=UIWindowArea; bases=MonoBehaviour,IBeginDragHandler,IEventSystemHandler,IDragHandler; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=6, Screen/Cursor=2, UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=72
- ksp refs: none; fan_in=0 fan_out=2
- methods(5): OnBeginDrag, OnDrag, OnMove, OnResize, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/UIWorldPointer.cs
- 322 lines; kind=MonoBehaviour; type=UIWorldPointer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, Vectrosity
- unity: UnityEngine.UI=6, GameObject=5, Transform=4, Texture=4, Material=4, Lifecycle=4; unity_score=74
- ksp refs: none; fan_in=2 fan_out=2
- methods(16): ChamferCorner, Create, CreateLine, GetAnchoredPoint, GetClosestCornerOrMidpoint, GetTexture, GetUIPoint, LateUpdate, OnDestroy, OnDisable, OnEnable, Setup, Terminate, UpdateDirect, UpdateKinked, UpdateLine
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Util/CanvasTools.cs
- 8 lines; kind=MonoBehaviour; type=CanvasTools; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Util
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=36
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Util/ImgText.cs
- 39 lines; kind=MonoBehaviour; type=ImgText; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Util
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, Texture=1, TMPro=1, MonoBehaviour=1; unity_score=72
- ksp refs: none; fan_in=5 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Util/RectContainmentDetector.cs
- 51 lines; kind=MonoBehaviour; type=RectContainmentDetector; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Util
- usings: UnityEngine
- unity: UnityEngine.UI=3, Lifecycle=2, Camera=2, MonoBehaviour=1, GetComponent=1; unity_score=80
- ksp refs: none; fan_in=1 fan_out=2
- methods(2): Start, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Util/RectUtil.cs
- 74 lines; kind=static class; type=RectUtil; bases=-; ancestry=-; ns=KSP.UI.Util
- usings: UnityEngine
- unity: UnityEngine.UI=12, Camera=3, Transform=1; unity_score=73
- ksp refs: none; fan_in=3 fan_out=1
- methods(2): GetRectContainment, WorldToUISpacePos
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Util/UISkinDefSO.cs
- 12 lines; kind=ScriptableObject; type=UISkinDefSO; bases=ScriptableObject; ancestry=ScriptableObject; ns=KSP.UI.Util
- usings: UnityEngine
- unity: ScriptableObject=1; unity_score=29
- ksp refs: none; fan_in=10 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/VesselListItem.cs
- 41 lines; kind=MonoBehaviour; type=VesselListItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: KSP.Localization, TMPro, UnityEngine
- unity: TMPro=6, MonoBehaviour=1; unity_score=81
- ksp refs: Localizer=3; fan_in=2 fan_out=3
- methods(1): Setup
- auto suggestion: tier=C subsystem=UI
