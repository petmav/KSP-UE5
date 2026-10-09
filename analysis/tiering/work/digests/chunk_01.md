# Review chunk 01 (root): 279 files, 50509 lines

## Assembly-CSharp/AbundanceRequest.cs
- 20 lines; kind=struct; type=AbundanceRequest; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=6 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/ActionGroupList.cs
- 146 lines; kind=class; type=ActionGroupList; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: UnityEngine.UI=2, GameObject=1; unity_score=22
- ksp refs: Planetarium=5, Vessel=3, ConfigNode=2, GameSettings=1; fan_in=1 fan_out=8
- methods(6): CooldownTimeRemaining, CopyFrom, Load, Save, SetGroup, ToggleGroup
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/ActionGroupsFlightController.cs
- 161 lines; kind=MonoBehaviour; type=ActionGroupsFlightController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, KSP.UI.Screens, UnityEngine, UnityEngine.UI
- unity: GameObject=5, Time.=3, Lifecycle=3, Coroutine=3, UnityEngine.UI=2, SetActive=2; unity_score=64
- ksp refs: GameEvents=10, FlightGlobals=8, InputLockManager=4, ConfigNode=3, Vessel=1, Part=1; fan_in=6 fan_out=17
- methods(8): Awake, ExitPanelActions, GetActionGroupPanel, PauseTime, SaveActions, SelectPanelActions, Start, Update
- auto suggestion: tier=B subsystem=Vessel

## Assembly-CSharp/ActiveJoint.cs
- 658 lines; kind=class; type=ActiveJoint; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: UnityEngine
- unity: Joint=12, Transform=9, Coroutine=4, GameObject=3, Rigidbody=2; unity_score=38
- ksp refs: Part=29, GameEvents=16, PartJoint=6, AttachNode=3, Vessel=2, ConfigNode=2; fan_in=4 fan_out=13
- methods(34): Create, DrawDebug, GetCtrlAxis, GetJointDrive, InitJoint, Load, Save, SetAxis, SetDriveMode, SetJointDrive, SetSecondaryAxis, Terminate, applyCoordsUpdate, findAttachJointForTargetPart, findJointAtNode, findJointBetweenParts, getAnchorOffset, getControlOrt, getControlPos, getInvControlOrt, getInvControlPos, onActiveJointNeedUpdate, onJointInit, onPartDestroyed, onPartJointBreak, onPartPack, onVesselModified, recurseCoordUpdate, restartJoint, restoreVesselCoords, startAfterJointsCreated, startForRefNode, storeVesselCoords, waitAndRestart
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/ActiveJointPivot.cs
- 84 lines; kind=class; type=ActiveJointPivot; bases=ActiveJoint; ancestry=ActiveJoint/IConfigNode; ns=(global)
- usings: UnityEngine
- unity: Joint=4; unity_score=41
- ksp refs: AttachNode=1; fan_in=1 fan_out=4
- methods(8): Create, GetControlAxis, GetLocalPivotRotation, SetLocalPivotTgtRotation, SetPivotAngleLimit, applyPivotLimit, onJointInit, setupPivot
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/AddonLoader.cs
- 185 lines; kind=MonoBehaviour; type=AddonLoader; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.SceneManagement
- unity: SceneManager=3, MonoBehaviour=3, Transform=2, Lifecycle=2, Instantiate/Destroy=2, GameObject=2; unity_score=56
- ksp refs: HighLogic=1; fan_in=1 fan_out=4
- methods(8): AddonLevelTest, Awake, LoadedOnceContains, OnDestroy, OnLevelLoaded, OnSceneLoaded, StartAddon, StartAddons
- auto suggestion: tier=B subsystem=Modding

## Assembly-CSharp/AdministrationFacility.cs
- 58 lines; kind=MonoBehaviour; type=AdministrationFacility; bases=SpaceCenterBuilding; ancestry=SpaceCenterBuilding/MonoBehaviour; ns=(global)
- usings: KSP.Localization, Strategies, UnityEngine
- unity: none; unity_score=21
- ksp refs: Localizer=6, HighLogic=5, GameEvents=4, InputLockManager=3; fan_in=0 fan_out=11
- methods(5): IsOpen, OnClicked, OnOnDestroy, OnPopupWarningDismiss, onDialogClose
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/AdvancedRagdoll.cs
- 205 lines; kind=MonoBehaviour; type=AdvancedRagdoll; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=34, AnimationCurve=20, Animation=9, GetComponent=7, Renderer=4, GameObject=4; unity_score=93
- ksp refs: none; fan_in=1 fan_out=0
- methods(8): CopyMaterials, CopyTransforms, GetTransformPathToRoot, PoseToAnimation, Start, SynchRagdollIn, SynchRagdollOut, TransformToAnimationCurve
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/AdvancedTutorial.cs **MUST-OPEN (>800 lines)**
- 1233 lines; kind=ScenarioModule; type=AdvancedTutorial; bases=METutorialScenario; ancestry=METutorialScenario/TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Editor, UnityEngine, UnityEngine.UI
- unity: GetComponent=5, GameObject=5, Transform=2, UnityEngine.UI=1; unity_score=23
- ksp refs: GameEvents=3, InputLockManager=2; fan_in=0 fan_out=21
- methods(71): AddEndNodeListener, ArrangeGraph, CreateTutorialPages, EnterVesselDrag, EnterVesselLink, EnterVesselMessage, EnterVesselName, EnterVesselWriteError, EnterVesselWriteMain, GetCurrentNodeParameters, GetCurrentTutorialStep, GetLinkNodes, GetMessageParameter, GetNodeEndParameter, GetTextNodes, GetVesselDropDown, HighlightTitleBarExportButton, IsAllDialogNodeEnd, IsAllTextValueValid, IsDragErrorStep, IsDragMainStep, IsExportDialogOpen, IsLinkErrorStep, IsLinkMainStep, IsParameterValid, IsParametersCorrect, IsTextValueValid, IsValidationDialogOpen, IsWriteErrorStep, IsWriteMainStep, LockBar, OnCheckboxValueChanged, OnDoneButtonClick, OnEnterConclusion, OnEnterCrew, OnEnterDragStep, OnEnterExport, OnEnterPart, OnEnterValidation, OnEnterVessel, OnExitCancelSave, OnExitCrew, OnExitMissionEditorScreen, OnInitCreateVesel, OnLeaveCrew ...
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/AerodynamicsFX.cs
- 281 lines; kind=MonoBehaviour; type=AerodynamicsFX; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: CameraFXModules, UnityEngine
- unity: Transform=10, Lifecycle=5, Light=3, Texture=1, QualitySettings/SystemInfo=1, MonoBehaviour=1; unity_score=60
- ksp refs: PhysicsGlobals=15, GameEvents=6, MapView=5, Vessel=4, FlightGlobals=4, GameSettings=2; fan_in=1 fan_out=16
- methods(8): Awake, LateUpdate, OnDestroy, OnPause, OnSettingsUpdate, OnUnpause, Start, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/AeroFXState.cs
- 27 lines; kind=class; type=AeroFXState; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=12
- ksp refs: none; fan_in=1 fan_out=2
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/AeroGUI.cs **MUST-OPEN (>800 lines)**
- 914 lines; kind=KSPAddon; type=AeroGUI; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: IMGUI=173, Transform=9, UnityEngine.UI=3, GetComponent=3, Rigidbody=2, Lifecycle=2; unity_score=86
- ksp refs: Localizer=45, PhysicsGlobals=20, Vector3d=19, FlightGlobals=5, Vessel=3, Planetarium=3; fan_in=0 fan_out=20
- methods(7): DrawWindow, DrawWindowThermal, GetAeroStats, GetThermalStats, OnGUI, Start, Update
- auto suggestion: tier=B subsystem=Aero

## Assembly-CSharp/AlarmActions.cs
- 70 lines; kind=class; type=AlarmActions; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=4 fan_out=2
- methods(4): Duplicate, Load, Save, ToString
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/AlarmClockMessageDialog.cs
- 136 lines; kind=MonoBehaviour; type=AlarmClockMessageDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=6, Transform=4, TMPro=4, Lifecycle=4, UnityEngine.UI=3, SetActive=3; unity_score=85
- ksp refs: FlightGlobals=5, Vessel=3, Localizer=2, HighLogic=2, GameEvents=2; fan_in=2 fan_out=15
- methods(10): Awake, CloseDialog, OnAlarmRemoved, OnClose, OnDeleteOnClose, OnDestroy, OnJump, Spawn, Start, Update
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/AlarmClockScenario.cs
- 626 lines; kind=ScenarioModule; type=AlarmClockScenario; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.UI, KSP.UI.Screens, UnityEngine
- unity: Texture=8, Lifecycle=2, GameObject=2, Instantiate/Destroy=1, GetComponent=1; unity_score=39
- ksp refs: ConfigNode=11, TimeWarp=6, GameEvents=6, Planetarium=5, Vessel=3, GameDatabase=2; fan_in=14 fan_out=25
- methods(24): AddAlarm, AlarmExists, CreateAlarmByMapNodeType, CreateAlarmByType, DeleteAlarm, GetLastAlarm, GetNextAlarm, GetNextOrLastAlarm, GetUniqueAlarmID, HandleWarpActions, LoadTexture, LoadTypes, MapNodeDefined, OnAwake, OnDestroy, OnLoad, OnManeuversLoaded, OnSave, OrderByUT, PlaySound, ShowAlarmMapButton, SortAlarmsByUTDescending, TryGetAlarm, Update
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/AlarmClockScenarioAudio.cs
- 214 lines; kind=MonoBehaviour; type=AlarmClockScenarioAudio; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Audio=11, Lifecycle=3, MonoBehaviour=1, GetComponent=1, GameObject=1; unity_score=61
- ksp refs: GameDatabase=5, GameSettings=1; fan_in=1 fan_out=2
- methods(10): AudioEventArgs, Awake, GetStockAlarmSounds, InitializeSoundController, LoadAudioClip, PlaySound, Start, Stop, Update, isClipPlaying
- auto suggestion: tier=C subsystem=Audio

## Assembly-CSharp/AlarmClockSettings.cs
- 26 lines; kind=class; type=AlarmClockSettings; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=1 fan_out=2
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/AlarmClockSettingsUI.cs
- 107 lines; kind=class; type=AlarmClockSettingsUI; bases=DialogGUIVerticalLayout,ISettings; ancestry=DialogGUIVerticalLayout/DialogGUILayoutBase/DialogGUIBase; ns=(global)
- usings: KSP.Localization, KSP.UI, UnityEngine
- unity: none; unity_score=0
- ksp refs: Localizer=6; fan_in=1 fan_out=11
- methods(6): ApplySettings, DrawMiniSettings, DrawSettings, GetName, GetSettings, OnUpdate
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/AlarmClockUIAlarmRow.cs
- 144 lines; kind=MonoBehaviour; type=AlarmClockUIAlarmRow; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, GameObject=4, Transform=3, Lifecycle=3, TMPro=2, Instantiate/Destroy=2; unity_score=72
- ksp refs: Localizer=5, GameSettings=4, Planetarium=1; fan_in=1 fan_out=8
- methods(10): DeleteAlarm, DeleteAlarmDialogOutput, OnDestroy, OnToggleChanged, SetTextColor, Setup, Start, UIUpdate, Update, UpdateRowUI
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/AlarmClockUINextAlarm.cs
- 137 lines; kind=MonoBehaviour; type=AlarmClockUINextAlarm; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, TMPro, UnityEngine, UnityEngine.UI
- unity: SetActive=6, GameObject=6, UnityEngine.UI=5, Lifecycle=3, TMPro=2, MonoBehaviour=1; unity_score=78
- ksp refs: Localizer=6, GameSettings=3, Planetarium=1, HighLogic=1; fan_in=0 fan_out=9
- methods(7): DeleteAlarm, DeleteAlarmDialogOutput, OnDestroy, Start, UIUpdate, UIUpdate_NoAlarm, Update
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/AlarmMapNodeButton.cs
- 63 lines; kind=MonoBehaviour; type=AlarmMapNodeButton; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI.Screens, UnityEngine, UnityEngine.UI
- unity: Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1; unity_score=54
- ksp refs: none; fan_in=1 fan_out=4
- methods(5): Awake, OnAddAlarm, OnDestroy, OnPointerEnter, OnPointerExit
- auto suggestion: tier=B subsystem=MapView

## Assembly-CSharp/AlarmTypeApoapsis.cs
- 71 lines; kind=class; type=AlarmTypeApoapsis; bases=AlarmTypeFlightNodeBase; ancestry=AlarmTypeFlightNodeBase/AlarmTypeBase/AppUI_Data/IConfigNode; ns=(global)
- usings: KSP.Localization, KSP.UI, UnityEngine
- unity: none; unity_score=5
- ksp refs: Localizer=3, Vessel=1; fan_in=0 fan_out=5
- methods(7): CanSetAlarm, CannotSetAlarmText, GetDefaultTitle, GetNodeUT, InitializeFromMapObject, MapNodeType, ShowAlarmMapObject
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/AlarmTypeBase.cs
- 372 lines; kind=class; type=AlarmTypeBase; bases=AppUI_Data,ICloneable; ancestry=AppUI_Data/IConfigNode; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, UnityEngine
- unity: SetActive=1, GameObject=1; unity_score=6
- ksp refs: Vessel=8, TimeWarp=4, ConfigNode=4, FlightGlobals=3, GameEvents=2, Planetarium=1; fan_in=13 fan_out=17
- methods(29): ActionAlarm, CanSetAlarm, CannotSetAlarmText, Clone, CloneAlarm, GetDefaultTitle, InitializeFromMapObject, MapNodeType, OnActioned, OnAlarmLoad, OnAlarmSave, OnInputPanelUpdate, OnLoad, OnManeuversLoaded, OnSave, OnScenarioUpdate, OnTriggered, OnUIEndInitialization, OnUIInitialization, OnUIInputPanelDataChanged, RequiresVessel, ScenarioUpdate, ShowAlarmMapObject, TriggerAlarm, UIEndInitialization, UIInitialization, UIInputPanelDataChanged, UIInputPanelUpdate, UpdateTimeToAlarm
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/AlarmTypeFlightNodeBase.cs
- 78 lines; kind=class; type=AlarmTypeFlightNodeBase; bases=AlarmTypeBase; ancestry=AlarmTypeBase/AppUI_Data/IConfigNode; ns=(global)
- usings: KSP.UI, KSP.UI.Screens
- unity: none; unity_score=0
- ksp refs: Vessel=3, ConfigNode=2, Orbit=1; fan_in=3 fan_out=8
- methods(9): GetNodeUT, OnAlarmLoad, OnAlarmSave, OnInputPanelUpdate, OnScenarioUpdate, OnUIEndInitialization, OnUIInitialization, RequiresVessel, UpdateAlarmUT
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/AlarmTypeManeuver.cs
- 332 lines; kind=class; type=AlarmTypeManeuver; bases=AlarmTypeBase; ancestry=AlarmTypeBase/AppUI_Data/IConfigNode; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, UnityEngine
- unity: SetActive=2, GameObject=2; unity_score=12
- ksp refs: Vessel=4, Localizer=4, Planetarium=3, GameEvents=2, ConfigNode=2; fan_in=1 fan_out=19
- methods(18): BuildManeuverListUI, CanSetAlarm, CannotSetAlarmText, GetDefaultTitle, InitializeFromMapObject, MapNodeType, OnAlarmLoad, OnAlarmSave, OnInputPanelUpdate, OnManeuverNodeChanged, OnManeuversLoaded, OnScenarioUpdate, OnUIEndInitialization, OnUIInitialization, RequiresVessel, ShowAlarmMapObject, UpdateAlarmUT, UpdateManeuverList
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/AlarmTypePeriapsis.cs
- 71 lines; kind=class; type=AlarmTypePeriapsis; bases=AlarmTypeFlightNodeBase; ancestry=AlarmTypeFlightNodeBase/AlarmTypeBase/AppUI_Data/IConfigNode; ns=(global)
- usings: KSP.Localization, KSP.UI, UnityEngine
- unity: none; unity_score=5
- ksp refs: Localizer=3, Vessel=1; fan_in=0 fan_out=5
- methods(7): CanSetAlarm, CannotSetAlarmText, GetDefaultTitle, GetNodeUT, InitializeFromMapObject, MapNodeType, ShowAlarmMapObject
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/AlarmTypeRaw.cs
- 113 lines; kind=class; type=AlarmTypeRaw; bases=AlarmTypeBase; ancestry=AlarmTypeBase/AppUI_Data/IConfigNode; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens
- unity: SetActive=1, GameObject=1; unity_score=12
- ksp refs: Planetarium=1, Localizer=1; fan_in=2 fan_out=11
- methods(6): CanSetAlarm, GetDefaultTitle, OnInputPanelUpdate, OnUIEndInitialization, OnUIInitialization, RequiresVessel
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/AlarmTypeSOI.cs
- 71 lines; kind=class; type=AlarmTypeSOI; bases=AlarmTypeFlightNodeBase; ancestry=AlarmTypeFlightNodeBase/AlarmTypeBase/AppUI_Data/IConfigNode; ns=(global)
- usings: KSP.Localization, KSP.UI, UnityEngine
- unity: none; unity_score=5
- ksp refs: Localizer=3, Orbit=2, Vessel=1; fan_in=0 fan_out=6
- methods(7): CanSetAlarm, CannotSetAlarmText, GetDefaultTitle, GetNodeUT, InitializeFromMapObject, MapNodeType, ShowAlarmMapObject
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/AlarmTypeTransferWindow.cs
- 184 lines; kind=class; type=AlarmTypeTransferWindow; bases=AlarmTypeBase; ancestry=AlarmTypeBase/AppUI_Data/IConfigNode; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens
- unity: none; unity_score=0
- ksp refs: FlightGlobals=8, Planetarium=5, CelestialBody=5, Vessel=4, Localizer=4, GameSettings=2; fan_in=0 fan_out=14
- methods(14): CanSetAlarm, CannotSetAlarmText, GetDefaultTitle, GetNextTransferWindow, OnAlarmLoad, OnAlarmSave, OnInputPanelUpdate, OnScenarioUpdate, OnUIEndInitialization, OnUIInitialization, OnUIInputPanelDataChanged, RequiresVessel, UpdateDestList, UpdateSourceList
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/AltimeterDisplayState.cs
- 6 lines; kind=enum; type=AltimeterDisplayState; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/AltitudeMultiTextureRamp.cs
- 189 lines; kind=MonoBehaviour; type=AltitudeMultiTextureRamp; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Texture=6, Instantiate/Destroy=2, Application.=2, MonoBehaviour=1; unity_score=53
- ksp refs: none; fan_in=0 fan_out=1
- methods(5): CreateTextureARGB, CreateTextureGS, Lerp, Reset, Setup
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/AmbienceControl.cs
- 208 lines; kind=MonoBehaviour; type=AmbienceControl; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Audio=5, Lifecycle=3, Transform=2, AnimationCurve=2, MonoBehaviour=1, GameObject=1; unity_score=53
- ksp refs: FlightGlobals=12, GameEvents=9, MapView=4, GameSettings=4, HighLogic=2, Planetarium=1; fan_in=0 fan_out=7
- methods(9): Awake, OnDestroy, OnLeavingScene, OnSceneLoaded, Update, editorAmbienceGo, editorAmbienceStop, flightAmbienceGo, flightAmbienceStop
- auto suggestion: tier=C subsystem=Audio

## Assembly-CSharp/AnalyticsUtil.cs **MUST-OPEN (>800 lines)**
- 884 lines; kind=static class; type=AnalyticsUtil; bases=-; ancestry=-; ns=(global)
- usings: Contracts, Expansions, Expansions.Missions, KSP.Localization, KSP.UI.Screens, KSP.UI.Screens.Flight, KSPAchievements, Steamworks, UnityEngine, UnityEngine.Analytics
- unity: QualitySettings/SystemInfo=11, Application.=6, Steam=3, Analytics=3, PlayerPrefs=2; unity_score=15
- ksp refs: Vessel=16, Game=15, FlightGlobals=7, Contract=3, ProtoCrewMember=2, GameSettings=2; fan_in=41 fan_out=36
- methods(46): AddCommonSteamItemData, AddMissionGeneralData, AddSaveVesselDate, AltimeterModeUsage, CountVesselSituation, DeltaVAppUsage, DeltaVStageUsage, FixIncorrectTutorialName, Initialize, LogAsteroidEvent, LogAsteroidVesselEvent, LogCelestialBodyProgress, LogCometEvent, LogCometVesselEvent, LogContractAccepted, LogContractCancelled, LogContractCompleted, LogCustomEvent, LogCustomEventToFile, LogDebugWindow, LogEVAChuteDeployed, LogExitEditor, LogExitFlight, LogGameStart, LogJoystickUsage, LogKSPedia, LogMissionCompleted, LogMissionExitBuilder, LogMissionFailed, LogMissionStart, LogPointOfInterestReached, LogSaveGameClosed, LogSaveGameCreated, LogSaveGameResumed, LogSteamError, LogSteamItemCreated, LogSteamItemSubscribed, LogSteamItemUnsubscribed, LogSteamItemUpdated, LogSuitWindowUsage, LogTechTreeNodeUnlocked, LogTutorialCompleted, LogTutorialStart, LogVesselDocked, LogVesselLaunched ...
- auto suggestion: tier=C subsystem=Analytics

## Assembly-CSharp/AngleRenderEject.cs
- 359 lines; kind=MonoBehaviour; type=AngleRenderEject; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=17, Renderer=15, Transform=12, IMGUI=7, Camera=4, Material=3; unity_score=81
- ksp refs: Vector3d=36, ScaledSpace=11, MapView=4, CelestialBody=2, Planetarium=1, Orbit=1; fan_in=0 fan_out=8
- methods(12): ClampDegrees180, ClampDegrees360, DrawAngle, DrawArc, DrawLine, DrawLineArrow, HideAngle, InitLine, OnDestroy, OnGUI, OnPreCull, Start
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/AnimationFX.cs
- 86 lines; kind=MonoBehaviour; type=AnimationFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Animation=2; unity_score=37
- ksp refs: ConfigNode=4; fan_in=0 fan_out=4
- methods(5): FindAnimator, OnEvent, OnInitialize, OnLoad, OnSave
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/AntennaType.cs
- 11 lines; kind=enum; type=AntennaType; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/ApplyScoreTutorialStep.cs
- 219 lines; kind=class; type=ApplyScoreTutorialStep; bases=IntermediateTutorialPageStep; ancestry=IntermediateTutorialPageStep/TutorialPageConfig; ns=(global)
- usings: Expansions.Missions, Expansions.Missions.Editor, UnityEngine
- unity: GameObject=6; unity_score=18
- ksp refs: GameEvents=4; fan_in=1 fan_out=16
- methods(20): AddTutorialStepConfig, CanHighlightApplyScoreSetup, FilterApplyNodeCriteria, HasApplyScoreNode, IsLinksConnected, OnEnterDragApplyScoreNode, OnEnterDragApplyScoreNodeHelper, OnEnterLinkExplanation, OnEnterSetupApplyScoreNode, OnEnterSetupApplyScoreNodeHelper, OnLeaveDragApplyScoreNode, OnLeaveDragApplyScoreNodeHelper, OnLeaveLinkExplanation, OnLeaveSetupApplyScoreNode, OnLeaveSetupApplyScoreNodeHelper, OnNodeFocused, OnSelectedGameObjectChange, OnUpdateLinkExplanation, OnUpdateSetupApplyScoreNode, OnUpdateSetupApplyScoreNodeHelper
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/AppUIMaster.cs
- 87 lines; kind=MonoBehaviour; type=AppUIMaster; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, UnityEngine
- unity: GameObject=8, MonoBehaviour=3, Lifecycle=3, Instantiate/Destroy=1, GetComponent=1; unity_score=76
- ksp refs: none; fan_in=1 fan_out=1
- methods(5): Awake, SetupRowPrefabByType, Start, TryGetAppUIControlPrefab, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/APSkillExtensions.cs
- 77 lines; kind=static class; type=APSkillExtensions; bases=-; ancestry=-; ns=(global)
- usings: Expansions
- unity: none; unity_score=0
- ksp refs: HighLogic=15, Game=5, Vessel=1; fan_in=0 fan_out=7
- methods(2): AvailableAtLevel, GetRequiredSkill
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/ArrayExtensions.cs
- 22 lines; kind=static class; type=ArrayExtensions; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): IndexOf
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/ArrowPointer.cs
- 204 lines; kind=MonoBehaviour; type=ArrowPointer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.Rendering
- unity: GameObject=8, Mesh=5, Transform=3, Instantiate/Destroy=3, GetComponent=3, Renderer=2; unity_score=75
- ksp refs: none; fan_in=4 fan_out=1
- methods(6): Create, CreateMesh, OnDestroy, Update, UpdateVertexColors, UpdateVerts
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/ArrowPointerSystem.cs
- 27 lines; kind=MonoBehaviour; type=ArrowPointerSystem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Material=4, Lifecycle=2, MonoBehaviour=1; unity_score=86
- ksp refs: none; fan_in=1 fan_out=0
- methods(2): Awake, OnDestroy
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/AssemblyInfo.cs
- 20 lines; kind=class; type=AssemblyInfo; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Misc

## Assembly-CSharp/AssemblyLoader.cs **MUST-OPEN (>800 lines)**
- 1004 lines; kind=static class; type=AssemblyLoader; bases=-; ancestry=-; ns=(global)
- usings: KSP.Testing, Mono.Cecil, UnityEngine
- unity: none; unity_score=9
- ksp refs: ConfigNode=10, GameDatabase=1; fan_in=34 fan_out=6
- methods(40): Add, AddForbiddenString, AddForbiddenType, CheckDependencies, ClearPlugins, Contains, FlagDuplicatedPlugins, GetAssembly, GetByAssembly, GetClassByName, GetEnumerator, GetModulesImplementingInterface, GetPathByType, GetSubclassesOfParentClass, GetTypeByName, GetTypesOfClassesImplementingInterface, Initialize, InitializeForbidden, Load, LoadAssemblies, LoadAssembly, LoadAttribute_KSPAssembly, LoadAttribute_KSPAssemblyDependency, LoadAttribute_KSPAssemblyDependencyEqualMajor, LoadConfigAttributes, LoadExternalAssembly, LoadPlugin, LoadPluginInfo, LogBadAssemblyError, MyResolveEventHandler, PluginHighestVersionPath, PluginNameNotDuplicate, RemoveAt, RemoveRange, ScanForBadTypeRefs, SortAssemblies, TSort, TypeOperation, Unload, Visit
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/AssetBase.cs
- 160 lines; kind=MonoBehaviour; type=AssetBase; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI.Screens, UnityEngine
- unity: GameObject=15, Texture=8, Lifecycle=3, IMGUI=3, Transform=2, MonoBehaviour=2; unity_score=82
- ksp refs: none; fan_in=49 fan_out=1
- methods(7): Awake, GetGUISkin, GetPrefab, GetSprite, GetTexture, OnDestroy, Start
- auto suggestion: tier=C subsystem=Modding

## Assembly-CSharp/AsteroidCollider.cs
- 3 lines; kind=MonoBehaviour; type=AsteroidCollider; bases=SpaceObjectCollider; ancestry=SpaceObjectCollider/MonoBehaviour; ns=(global)
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/AstronautComplexFacility.cs
- 61 lines; kind=MonoBehaviour; type=AstronautComplexFacility; bases=SpaceCenterBuilding; ancestry=SpaceCenterBuilding/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=21
- ksp refs: HighLogic=9, Localizer=6, GameEvents=6, InputLockManager=5, Game=2; fan_in=0 fan_out=11
- methods(5): IsOpen, OnClicked, OnOnDestroy, OnPopupWarningDismiss, onAstronautComplexDialogClose
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/AtlasSplitter.cs
- 24 lines; kind=MonoBehaviour; type=AtlasSplitter; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Texture=1, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): Reset
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/AtmosphereFromGround.cs
- 268 lines; kind=MonoBehaviour; type=AtmosphereFromGround; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=47, Transform=9, Renderer=2, Material=2, Lifecycle=2, MonoBehaviour=1; unity_score=89
- ksp refs: ScaledSpace=3, TimeWarp=1, Planetarium=1, MapView=1, CelestialBody=1; fan_in=4 fan_out=6
- methods(6): LateUpdate, SetKrESunLerp, SetMaterial, Start, UpdateAtmosphere, pow
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/AtmosphereProbe.cs
- 27 lines; kind=MonoBehaviour; type=AtmosphereProbe; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, Transform=1, MonoBehaviour=1; unity_score=67
- ksp refs: FlightGlobals=6; fan_in=0 fan_out=1
- methods(2): Start, Update
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/Attachment.cs
- 22 lines; kind=class; type=Attachment; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: Part=2, AttachNode=2; fan_in=2 fan_out=3
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/AttachModes.cs
- 5 lines; kind=enum; type=AttachModes; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=13 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/AttachNode.cs
- 202 lines; kind=class; type=AttachNode; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Transform=12, Instantiate/Destroy=2, Renderer=1, Material=1, GetComponent=1, GameObject=1; unity_score=47
- ksp refs: AttachNode=10, Part=4; fan_in=44 fan_out=2
- methods(6): ChangeSrfNodePosition, Clone, DestroyNodeIcon, FindAttachedPart, FindOpposingNode, ReverseSrfNodeDirection
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/AttachNodeMethod.cs
- 9 lines; kind=enum; type=AttachNodeMethod; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=6 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/AttachNodeSnapshot.cs
- 70 lines; kind=class; type=AttachNodeSnapshot; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=2, Vessel=1, ProtoVessel=1, AttachNode=1; fan_in=1 fan_out=4
- methods(2): Save, findAttachedPart
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/AttachRules.cs
- 78 lines; kind=class; type=AttachRules; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=5
- ksp refs: none; fan_in=3 fan_out=0
- methods(2): Parse, String
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/AudioFX.cs
- 248 lines; kind=MonoBehaviour; type=AudioFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Audio=12, Instantiate/Destroy=3, Lifecycle=2, GetComponent=2, GameObject=1; unity_score=61
- ksp refs: GameSettings=5, GameEvents=4, ConfigNode=4, HighLogic=2, GameDatabase=2; fan_in=6 fan_out=9
- methods(13): CreateSource, OnDestroy, OnEvent, OnGamePause, OnGameUnpause, OnInitialize, OnLoad, OnSave, Play, SetSourceVolume, SetTime, Stop, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/AudioFXSettings.cs
- 142 lines; kind=class; type=AudioFXSettings; bases=DialogGUIVerticalLayout,ISettings; ancestry=DialogGUIVerticalLayout/DialogGUILayoutBase/DialogGUIBase; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Audio=1; unity_score=8
- ksp refs: GameSettings=20, Localizer=10; fan_in=1 fan_out=15
- methods(6): ApplySettings, DrawMiniSettings, DrawSettings, GetName, GetSettings, OnUpdate
- auto suggestion: tier=C subsystem=Audio

## Assembly-CSharp/AudioLoopFX.cs
- 131 lines; kind=MonoBehaviour; type=AudioLoopFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Audio=5, Lifecycle=2, Instantiate/Destroy=1, GetComponent=1, GameObject=1; unity_score=59
- ksp refs: GameEvents=4, ConfigNode=4, HighLogic=1, GameDatabase=1; fan_in=0 fan_out=9
- methods(11): CreateSource, OnDestroy, OnEvent, OnGamePause, OnGameUnpause, OnInitialize, OnLoad, OnSave, Play, Stop, Update
- auto suggestion: tier=C subsystem=Audio

## Assembly-CSharp/AudioMultiFX.cs
- 170 lines; kind=MonoBehaviour; type=AudioMultiFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=5, Audio=5, Lifecycle=2, GetComponent=1, GameObject=1; unity_score=61
- ksp refs: GameEvents=4, ConfigNode=4, HighLogic=1, GameDatabase=1; fan_in=0 fan_out=9
- methods(10): CreateSource, OnDestroy, OnEvent, OnGamePause, OnGameUnpause, OnInitialize, OnLoad, OnSave, Play, Update
- auto suggestion: tier=C subsystem=Audio

## Assembly-CSharp/AudioMultiPooledFX.cs
- 290 lines; kind=MonoBehaviour; type=AudioMultiPooledFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: Smooth.Pools, UnityEngine
- unity: Transform=7, SetActive=4, Audio=4, Lifecycle=3, GameObject=3, Coroutine=2; unity_score=60
- ksp refs: GameEvents=4, ConfigNode=4, HighLogic=2, GameDatabase=1; fan_in=0 fan_out=10
- methods(16): BorrowAndPlay, CheckEnd, Create, OnDestroy, OnDisable, OnEvent, OnGamePause, OnGameUnpause, OnInitialize, OnLoad, OnSave, Play, Reset, SetupAudioClip, StopAndRelease, Update
- auto suggestion: tier=C subsystem=Audio

## Assembly-CSharp/AudioMuter.cs
- 31 lines; kind=MonoBehaviour; type=AudioMuter; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1, Audio=1; unity_score=69
- ksp refs: GameEvents=4; fan_in=0 fan_out=2
- methods(4): Awake, OnDestroy, OnGameSceneLoadRequested, OnLevelLoaded
- auto suggestion: tier=C subsystem=Audio

## Assembly-CSharp/AudioSingleFX.cs
- 176 lines; kind=MonoBehaviour; type=AudioSingleFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Audio=12, Instantiate/Destroy=4, Lifecycle=3, GetComponent=2, GameObject=1; unity_score=71
- ksp refs: GameEvents=4, ConfigNode=4, HighLogic=1, GameDatabase=1; fan_in=0 fan_out=9
- methods(12): Awake, CreateSource, OnDestroy, OnEvent, OnGamePause, OnGameUnpause, OnInitialize, OnLoad, OnSave, Play, Stop, Update
- auto suggestion: tier=C subsystem=Audio

## Assembly-CSharp/AvailablePart.cs
- 309 lines; kind=class; type=AvailablePart; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Renderer=3, GetComponent=1, GameObject=1; unity_score=17
- ksp refs: AvailablePart=7, Localizer=5, ConfigNode=3, Part=1; fan_in=84 fan_out=8
- methods(6): APEntryCostGetDelegate, AddFileTime, GetVariant, Init, SetEntryCost, _GetEntryCost
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/AvailableVariantTheme.cs
- 56 lines; kind=class; type=AvailableVariantTheme; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: KSP.Localization
- unity: none; unity_score=7
- ksp refs: Localizer=3, ConfigNode=2, AvailablePart=2; fan_in=2 fan_out=5
- methods(3): CreateVariantTheme, Load, Save
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/AxisBinding.cs
- 135 lines; kind=class; type=AxisBinding; bases=IConfigNode,ICloneable; ancestry=IConfigNode; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=2
- ksp refs: ConfigNode=2; fan_in=7 fan_out=5
- methods(5): Clone, GetAxis, IsNeutral, Load, Save
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/AxisBinding_Single.cs
- 252 lines; kind=class; type=AxisBinding_Single; bases=InputBinding,IConfigNode,ICloneable; ancestry=InputBinding/IConfigNode; ns=(global)
- usings: UnityEngine
- unity: Input.=1; unity_score=7
- ksp refs: ConfigNode=2, GameSettings=1; fan_in=3 fan_out=6
- methods(4): GetAxis, IsNeutral, Load, Save
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/AxisFieldLimit.cs
- 10 lines; kind=class; type=AxisFieldLimit; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: none; fan_in=7 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/AxisGroupsInputHandler.cs
- 58 lines; kind=MonoBehaviour; type=AxisGroupsInputHandler; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1; unity_score=44
- ksp refs: GameEvents=2, VesselModule=1, Vessel=1; fan_in=0 fan_out=8
- methods(4): Awake, AxisInputHandler, OnDestroy, onVesselChange
- auto suggestion: tier=B subsystem=Input

## Assembly-CSharp/AxisGroupsManager.cs
- 61 lines; kind=class; type=AxisGroupsManager; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=5, PartModule=3; fan_in=1 fan_out=5
- methods(3): BuildBaseAxisFields, LoadAxisFieldNodes, SaveAxisFieldNodes
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/AxisGroupsModule.cs
- 139 lines; kind=VesselModule; type=AxisGroupsModule; bases=VesselModule; ancestry=VesselModule/MonoBehaviour; ns=(global)
- usings: -
- unity: Lifecycle=1; unity_score=20
- ksp refs: GameEvents=4, PartModule=3, Vessel=2, VesselModule=1, GameSettings=1; fan_in=1 fan_out=8
- methods(12): AddAxis, BuildAxisGroups, GetActivation, IncrementAxisGroup, OnAwake, OnDestroy, OnLoadVessel, OnVesselOverrideGroupChanged, SetAxisGroup, ShouldBeActive, UpdateAxisGroup, onVesselWasModified
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/AxisKeyBinding.cs
- 48 lines; kind=class; type=AxisKeyBinding; bases=IConfigNode,ICloneable; ancestry=IConfigNode; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=3; fan_in=1 fan_out=4
- methods(3): Clone, Load, Save
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/AxisKeyBindingList.cs
- 67 lines; kind=class; type=AxisKeyBindingList; bases=IConfigNode,ICloneable; ancestry=IConfigNode; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=4; fan_in=1 fan_out=3
- methods(3): Clone, Load, Save
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/AxisTestModule.cs
- 18 lines; kind=PartModule; type=AxisTestModule; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: none; unity_score=15
- ksp refs: PartModule=1, KSPField=1; fan_in=0 fan_out=6
- methods(1): OnStart
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/BaseAction.cs
- 465 lines; kind=class; type=BaseAction; bases=-; ancestry=-; ns=(global)
- usings: Expansions, Expansions.Serenity, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Part=12, PartModule=3, GameSettings=2, ConfigNode=2, Vessel=1, Localizer=1; fan_in=15 fan_out=20
- methods(14): AddAction, ContainsNonDefaultActions, CopyAction, CreateActionList, CreateGroupList, FireAction, GetActionGroup, GetActionGroups, GetActionGroupsLength, GetGroupIndex, Invoke, OnLoad, OnSave, ParseGroup
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/BaseActionDelegate.cs
- 1 lines; kind=delegate; type=BaseActionDelegate; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/BaseActionList.cs
- 166 lines; kind=class; type=BaseActionList; bases=List<BaseAction>; ancestry=List; ns=(global)
- usings: -
- unity: none; unity_score=2
- ksp refs: KSPAction=7, ConfigNode=3, PartModule=2, Part=2; fan_in=3 fan_out=7
- methods(6): Add, Contains, GetReflectedAttributes, OnLoad, OnSave, ReflectActions
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/BaseAxisField.cs
- 437 lines; kind=class; type=BaseAxisField; bases=BaseField; ancestry=BaseField/BaseField; ns=(global)
- usings: Expansions.Serenity, UnityEngine
- unity: none; unity_score=0
- ksp refs: Part=9, PartModule=6, Vessel=3, ConfigNode=2, TimeWarp=1, GameSettings=1; fan_in=20 fan_out=13
- methods(16): AddAxis, ContainsNonDefaultAxes, CopyField, CreateAxisList, CreateGroupList, GetAxisGroup, GetAxisGroups, GetAxisGroupsLength, IncrementAxis, IsInGroup, OnLoad, OnSave, ParseGroup, SetAxis, SetIncrementalSpeedMultiplier, UpdateAxis
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/BaseAxisFieldList.cs
- 5 lines; kind=class; type=BaseAxisFieldList; bases=List<BaseAxisField>; ancestry=List; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/BaseConverter.cs
- 781 lines; kind=PartModule; type=BaseConverter; bases=PartModule,IAnimatedModule,IOverheatDisplay,IConstruction; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: MonoBehaviour=3, Time.=1, Lifecycle=1; unity_score=21
- ksp refs: KSPField=28, Localizer=8, PartResourceDefinition=5, FloatCurve=4, KSPAction=3, HighLogic=3; fan_in=6 fan_out=36
- methods(46): CacheLocalStrings, CanBeDetached, CanBeOffset, CanBeRotated, CheckForShutdown, ConvertRecipeToUnits, DisableModule, DisplayCoreHeat, EnableModule, FixedUpdate, GetBestDeltaTime, GetCoreTemperature, GetCrewEfficiencyBonus, GetCrewHeatBonus, GetDeltaTime, GetEfficiencyMultiplier, GetFlux, GetGoalTemperature, GetHeatThrottle, GetModuleDisplayName, HasSpecialist, IsAnimationGroupDeployed, IsOverheating, IsSituationValid, ModuleIsActive, OnAwake, OnLoad, OnOverheat, OnSave, OnStart, PostProcess, PostUpdateCleanup, PreProcessing, PrepareRecipe, SetEfficiencyBonus, SetupDebugging, SetupLabels, SetupModule, StartResourceConverter, StartResourceConverterAction, StopResourceConverter, StopResourceConverterAction, TallyEfficiencyModifiers, ToggleResourceConverterAction, UpdateConverterStatus ...
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/BaseDrill.cs
- 18 lines; kind=PartModule; type=BaseDrill; bases=BaseConverter; ancestry=BaseConverter/PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: none; unity_score=15
- ksp refs: KSPField=3, Localizer=1; fan_in=3 fan_out=3
- methods(1): GetModuleDisplayName
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/BaseEvent.cs
- 266 lines; kind=class; type=BaseEvent; bases=-; ancestry=-; ns=(global)
- usings: Expansions, KSP.Localization, UnityEngine
- unity: none; unity_score=1
- ksp refs: KSPEvent=2, ConfigNode=2, Localizer=1; fan_in=25 fan_out=10
- methods(5): Invoke, OnLoad, OnSave, ToString, VariantToggleEventDisabled
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/BaseEventDelegate.cs
- 1 lines; kind=delegate; type=BaseEventDelegate; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/BaseEventDetails.cs
- 129 lines; kind=class; type=BaseEventDetails; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: GameObject=3, IMGUI=1; unity_score=22
- ksp refs: none; fan_in=5 fan_out=1
- methods(7): Get, GetBool, GetFloat, GetGameObject, GetInt, GetString, Set
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/BaseEventDetailsDelegate.cs
- 1 lines; kind=delegate; type=BaseEventDetailsDelegate; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/BaseEventList.cs
- 346 lines; kind=class; type=BaseEventList; bases=List<BaseEvent>; ancestry=List; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=2; unity_score=4
- ksp refs: KSPEvent=10, ConfigNode=3, PartModule=2, Part=2; fan_in=7 fan_out=9
- methods(14): Add, AddBaseEventList, ClearDefault, Contains, CreateDelegates, GetByIndex, GetReflectedAttributes, LoadStagingIcon, OnLoad, OnSave, RemoveBaseEventList, Send, SendDefault, SetDefault
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/BaseField.cs **MUST-OPEN (>800 lines)**
- 931 lines; kind=class; type=BaseField; bases=-; ancestry=BaseField; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=12
- ksp refs: KSPField=6, Localizer=4; fan_in=55 fan_out=9
- methods(14): CacheLocalStrings, CopyField, GetStringValue, GetValue, GuiString, Read, ReadPvt, SetOriginalValue, SetPvt, SetValue, SetupUIControl, SetupUIControlLabel, SetupUIControls, Write
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/BaseFieldList.cs
- 423 lines; kind=class; type=BaseFieldList; bases=IEnumerable; ancestry=IEnumerable; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=5
- ksp refs: ConfigNode=8, KSPField=2, HighLogic=2; fan_in=8 fan_out=10
- methods(13): Add, AddBaseFieldList, CreateList, GetEnumerator, GetReflectedAttributes, GetValue, Load, ReadValue, RemoveBaseFieldList, Save, SetOriginalValue, SetValue, TryGetFieldUIControl
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/BaseGameEvent.cs
- 35 lines; kind=class; type=BaseGameEvent; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=10
- ksp refs: none; fan_in=12 fan_out=1
- methods(1): FindEvent
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/BaseLabel.cs
- 80 lines; kind=MonoBehaviour; type=BaseLabel; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Transform=7, GameObject=7, SetActive=4, UnityEngine.UI=3, GetComponent=3, Lifecycle=2; unity_score=91
- ksp refs: GameEvents=2; fan_in=3 fan_out=5
- methods(7): Awake, Disable, Enable, OnClick, OnDestroy, ResetParent, Setup
- auto suggestion: tier=C subsystem=Part

## Assembly-CSharp/BasePAWGroup.cs
- 22 lines; kind=class; type=BasePAWGroup; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=6 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/BasicTutorial.cs **MUST-OPEN (>800 lines)**
- 1171 lines; kind=ScenarioModule; type=BasicTutorial; bases=METutorialScenario; ancestry=METutorialScenario/TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions, Expansions.Missions.Editor, KSP.Localization, UnityEngine, UnityEngine.UI
- unity: GameObject=30, GetComponent=5, UnityEngine.UI=1; unity_score=35
- ksp refs: ConfigNode=5, Orbit=2, Localizer=2; fan_in=0 fan_out=23
- methods(100): AddTutorialPage, CreateDialog, CreateTutorialPages, CurrentNodeFilteredAreSelected, FacilityIsSetted, GetNextTutorialName, GetVesselToggleTypeParam, HighlightTitleBarSaveButton, IsPlayerBuiltCreation, IsSituationSetted, LaunchSiteIsSetted, ModifySaveButtonForTutorial, OnBriefingCancel, OnBriefingOkay, OnContinueButtonClick, OnDoneButtonClick, OnEnterChangeVesselName, OnEnterClickSaveButton, OnEnterConfirmLaunchSite, OnEnterConfirmOrbitPlanet, OnEnterCreateVesselNode, OnEnterDragOrbitNode, OnEnterDragVesselNode, OnEnterEmpty, OnEnterFindOrbitNode, OnEnterFindVesselNode, OnEnterLinkOrbitNode, OnEnterLinkVesselNode, OnEnterNameMission, OnEnterNodeDragging, OnEnterNodeDraggingTutorialPage, OnEnterOrbitNodeExplanation, OnEnterSave, OnEnterSelectLaunchSite, OnEnterSelectOrbitPlanet, OnEnterSetMunInVesselNode, OnEnterSettingsTutorialPage, OnEnterShowFacilityParameter, OnEnterShowOrbitPlanetSettings, OnEnterShowSituationParameter, OnEnterTrySaving, OnEnterTrySetMunInVesselNode, OnEnterWelcomeTutorialPage, OnExitCancelSave, OnExitMissionEditorScreen ...
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/BiDictionaryOneToOne.cs
- 108 lines; kind=class; type=BiDictionaryOneToOne; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(11): Add, Clear, GetByFirst, GetBySecond, RemoveByFirst, RemoveBySecond, TryAdd, TryGetByFirst, TryGetBySecond, TryRemoveByFirst, TryRemoveBySecond
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/BiomeLockData.cs
- 6 lines; kind=class; type=BiomeLockData; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/BiomeTutorialStep.cs
- 239 lines; kind=class; type=BiomeTutorialStep; bases=IntermediateTutorialPageStep; ancestry=IntermediateTutorialPageStep/TutorialPageConfig; ns=(global)
- usings: Expansions.Missions.Editor, UnityEngine
- unity: GameObject=12; unity_score=29
- ksp refs: none; fan_in=1 fan_out=12
- methods(21): AddTutorialStepConfig, HasBiomeSelected, IsBiomeSettingSetted, IsMunSelected, IsVesselLandedSelected, MunBiomeSelected, OnEnterFindBiomeSetting, OnEnterFindBiomeSettingHelper, OnEnterSelectBiome, OnEnterSelectBiomeHelper, OnFindBiomeSettingHelper, OnLeaveFindBiomeSetting, OnLeaveFindBiomeSettingHelper, OnLeaveSelectBiome, OnLeaveSelectBiomeHelper, OnSelectBiome, OnUpdateFindBiomeSetting, OnUpdateFindBiomeSettingHelper, OnUpdateSelectBiome, OnUpdateSelectHelper, ShowBiomeSetttingSelector
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/BlankMonoBehaviour.cs
- 12 lines; kind=MonoBehaviour; type=BlankMonoBehaviour; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Start, Update
- auto suggestion: tier=B subsystem=Core

## Assembly-CSharp/Bobber.cs
- 33 lines; kind=MonoBehaviour; type=Bobber; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Time.=3, Transform=2, Lifecycle=2, MonoBehaviour=1; unity_score=78
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Start, Update
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/BoxVolume.cs
- 59 lines; kind=MonoBehaviour; type=BoxVolume; bases=VolumetricObjectBase; ancestry=VolumetricObjectBase/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=6, Lifecycle=1; unity_score=75
- ksp refs: none; fan_in=0 fan_out=1
- methods(5): HasChanged, OnEnable, PopulateShaderName, SetChangedValues, UpdateVolume
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/BufferMaterialProperty.cs
- 17 lines; kind=class; type=BufferMaterialProperty; bases=MaterialPropertyExtensions.MaterialProperty; ancestry=MaterialProperty; ns=(global)
- usings: UnityEngine
- unity: Material=1; unity_score=29
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): Apply
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ButtonHighlighter.cs
- 12 lines; kind=MonoBehaviour; type=ButtonHighlighter; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: SetActive=1, MonoBehaviour=1, GameObject=1; unity_score=63
- ksp refs: none; fan_in=2 fan_out=0
- methods(1): Enable
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/Callback.cs
- 5 lines; kind=delegate; type=Callback; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/CallbackUtil.cs
- 212 lines; kind=static class; type=CallbackUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Coroutine=23, MonoBehaviour=4, Time.=2; unity_score=51
- ksp refs: none; fan_in=44 fan_out=0
- methods(7): Clear, DelayedCallback, DoUntil, HoldUntil, LazyCallback, WaitUntil, lazyCB
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/CameraFade.cs
- 63 lines; kind=MonoBehaviour; type=CameraFade; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: IMGUI=5, Time.=2, Texture=2, Screen/Cursor=2, MonoBehaviour=1, Lifecycle=1; unity_score=82
- ksp refs: none; fan_in=1 fan_out=0
- methods(4): Awake, OnGUI, SetScreenOverlayColor, StartFade
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraFX.cs
- 29 lines; kind=MonoBehaviour; type=CameraFX; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: CameraFXModules, UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=67
- ksp refs: none; fan_in=5 fan_out=1
- methods(2): Awake, OnDestroy
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraManager.cs
- 451 lines; kind=MonoBehaviour; type=CameraManager; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Runtime, KSP.Localization, KSP.UI.Screens.Flight, UnityEngine
- unity: GameObject=5, Lifecycle=4, SetActive=3, Camera=3, GetComponent=2, Transform=1; unity_score=43
- ksp refs: FlightCamera=20, FlightGlobals=13, GameSettings=9, GameEvents=7, ProtoCrewMember=5, MissionSystem=5; fan_in=27 fan_out=26
- methods(18): Awake, GetCurrentCamera, GetNextIVA, ICameras_DeactivateAll, ICameras_ResetAll, NextCamera, NextCameraIVA, NextCameraMode, OnDestroy, OnVesselModified, PreviousCameraMode, SetCameraFlight, SetCameraIVA, SetCameraInternal, SetCameraMap, SetCameraMode, Start, Update
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraMouseLook.cs
- 73 lines; kind=MonoBehaviour; type=CameraMouseLook; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.SceneManagement
- unity: Input.=4, SceneManager=3, Screen/Cursor=2, Lifecycle=2, MonoBehaviour=1, KeyCode=1; unity_score=79
- ksp refs: InputLockManager=2, GameSettings=2, GameEvents=2, HighLogic=1; fan_in=8 fan_out=7
- methods(7): Awake, GetMouseLook, OnDestroy, OnLevelLoaded, OnSceneLoaded, SetMouseLook, onGamePause
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraOffCenter.cs
- 52 lines; kind=MonoBehaviour; type=CameraOffCenter; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Screen/Cursor=2, MonoBehaviour=1, Lifecycle=1, Camera=1; unity_score=59
- ksp refs: none; fan_in=1 fan_out=0
- methods(2): LateUpdate, PerspectiveOffCenter
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraOrbitHold.cs
- 5 lines; kind=MonoBehaviour; type=CameraOrbitHold; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=36
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CanvasPixelPerfectHandler.cs
- 43 lines; kind=MonoBehaviour; type=CanvasPixelPerfectHandler; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: SendMessage/Invoke=3, UnityEngine.UI=2, MonoBehaviour=1, Lifecycle=1; unity_score=75
- ksp refs: none; fan_in=2 fan_out=0
- methods(4): Disable, Enable, OnDestroy, TemporaryDisable
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/CanvasUtil.cs
- 31 lines; kind=static class; type=CanvasUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: UnityEngine.UI=4, Screen/Cursor=3; unity_score=70
- ksp refs: none; fan_in=2 fan_out=0
- methods(4): AnchorOffset, Contains, RectContains, ScreenToUISpacePos
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/Capability.cs
- 216 lines; kind=class; type=Capability; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: Lifecycle=2; unity_score=7
- ksp refs: none; fan_in=0 fan_out=2
- methods(3): GetCapabilities, ToString, Update
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/CBAttributeMap.cs
- 67 lines; kind=class; type=CBAttributeMap; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Texture=1; unity_score=26
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): GetAtt
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/CBAttributeMapSO.cs
- 159 lines; kind=ScriptableObject; type=CBAttributeMapSO; bases=MapSO; ancestry=MapSO/ScriptableObject; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=15
- ksp refs: Localizer=1; fan_in=11 fan_out=3
- methods(4): FormatLocalizationTags, GetAtt, GetPixelColor, RGBColorSqrMag
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/CBTextureAtlasSO.cs
- 82 lines; kind=ScriptableObject; type=CBTextureAtlasSO; bases=MapSO; ancestry=MapSO/ScriptableObject; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: none; fan_in=1 fan_out=2
- methods(2): GetCBTextureAtlasPoint, GetPixelCBTextureAtlasPoint
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/CelestialBody.cs **MUST-OPEN (>800 lines)**
- 1231 lines; kind=MonoBehaviour; type=CelestialBody; bases=MonoBehaviour,ITargetable,IDiscoverable; ancestry=MonoBehaviour; ns=(global)
- usings: FinePrint.Utilities, KSP.Localization, KSPAchievements, UnityEngine, UnityEngine.Rendering
- unity: AnimationCurve=12, Transform=9, GetComponent=6, Texture=3, Renderer=2, MonoBehaviour=2; unity_score=35
- ksp refs: Vector3d=60, PhysicsGlobals=20, Planetarium=19, FloatCurve=16, CelestialBody=16, Localizer=8; fan_in=241 fan_out=35
- methods(65): Awake, CBUpdate, CacheLocalStrings, Equals, GetActiveTargetable, GetAltitude, GetAtmoThermalStats, GetBodyReferencing, GetDensity, GetDisplayName, GetFrameVel, GetFrameVelAtUT, GetFullTemperature, GetFwdVector, GetHashCode, GetImpactLatitudeAndLongitude, GetLatLonAlt, GetLatLonAltOrbital, GetLatitude, GetLatitudeAndLongitude, GetLongitude, GetName, GetObtVelocity, GetOrbit, GetOrbitDriver, GetPressure, GetPressureAtm, GetRandomLatitudeAndLongitude, GetRelSurfaceDirection, GetRelSurfaceNVector, GetRelSurfacePosition, GetSolarAtmosphericEffects, GetSolarPowerFactor, GetSpeedOfSound, GetSrfVelocity, GetSurfaceNVector, GetTargetingMode, GetTemperature, GetTransform, GetVessel, GetWorldSurfacePosition, HasChild, HasParent, HideSurfaceResource, OnDrawGizmos ...
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/CelestialBodyLog.cs
- 60 lines; kind=class; type=CelestialBodyLog; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2, CelestialBody=2; fan_in=0 fan_out=3
- methods(7): AddNewBody, At, GetBodies, HasBody, Load, MergeWith, Save
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/CelestialBodyScienceParams.cs
- 23 lines; kind=class; type=CelestialBodyScienceParams; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=12
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/CelestialBodyType.cs
- 4 lines; kind=enum; type=CelestialBodyType; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/CenterOfLiftQuery.cs
- 32 lines; kind=class; type=CenterOfLiftQuery; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=9
- ksp refs: none; fan_in=3 fan_out=0
- methods(1): Reset
- auto suggestion: tier=A subsystem=Aero

## Assembly-CSharp/CenterOfThrustQuery.cs
- 17 lines; kind=class; type=CenterOfThrustQuery; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: none; fan_in=5 fan_out=0
- methods(1): Reset
- auto suggestion: tier=A subsystem=Aero

## Assembly-CSharp/CharacterAnimationState.cs
- 37 lines; kind=class; type=CharacterAnimationState; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Animation=3, Transform=2, Audio=1; unity_score=65
- ksp refs: none; fan_in=5 fan_out=0
- methods(1): string
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/CharacterAnimationUtil.cs
- 57 lines; kind=class; type=CharacterAnimationUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Animation=5, Transform=4, GetComponent=4, LayerMask=2, MonoBehaviour=1; unity_score=73
- ksp refs: none; fan_in=2 fan_out=1
- methods(2): GetAnimationsListFromScript, SetupAnimations
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/ChatController.cs
- 33 lines; kind=MonoBehaviour; type=ChatController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=3, Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1; unity_score=84
- ksp refs: none; fan_in=0 fan_out=2
- methods(3): AddToChatOutput, OnDisable, OnEnable
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/CheatOptions.cs
- 28 lines; kind=class; type=CheatOptions; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=35 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/ClearToSaveStatus.cs
- 11 lines; kind=enum; type=ClearToSaveStatus; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=11 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/ColliderMenuMouseChecker.cs
- 36 lines; kind=MonoBehaviour; type=ColliderMenuMouseChecker; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=33
- ksp refs: none; fan_in=0 fan_out=1
- methods(4): MouseHasExited, MouseIsHovering, OnMouseEnter, OnMouseExit
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/CollisionAnimationHandler.cs
- 35 lines; kind=MonoBehaviour; type=CollisionAnimationHandler; bases=MonoBehaviour,ICollisionEvents; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Collision/Force=6, MonoBehaviour=2, Animation=2, Lifecycle=1, GetComponent=1, GameObject=1; unity_score=95
- ksp refs: none; fan_in=0 fan_out=1
- methods(5): GetInstance, OnCollisionEnter, OnCollisionExit, OnCollisionStay, Start
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/CollisionEnhancer.cs
- 185 lines; kind=MonoBehaviour; type=CollisionEnhancer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Rigidbody=21, Transform=17, GetComponent=3, Raycast/Query=2, Lifecycle=2, Time.=1; unity_score=88
- ksp refs: Krakensbane=11, GameEvents=4, Part=2, FlightGlobals=2, Localizer=1, FloatingOrigin=1; fan_in=8 fan_out=12
- methods(2): FixedUpdate, Start
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/CollisionEnhancerBehaviour.cs
- 8 lines; kind=enum; type=CollisionEnhancerBehaviour; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Physics

## Assembly-CSharp/CollisionEventsHandler.cs
- 70 lines; kind=MonoBehaviour; type=CollisionEventsHandler; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Collision/Force=6, MonoBehaviour=4, Transform=3, GameObject=3, Lifecycle=1, GetComponent=1; unity_score=86
- ksp refs: none; fan_in=1 fan_out=1
- methods(6): FindHandleUpwards, OnCollisionEnter, OnCollisionExit, OnCollisionStay, SetHandle, Start
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/CollisionManager.cs
- 201 lines; kind=MonoBehaviour; type=CollisionManager; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Collider=14, Lifecycle=4, SetActive=2, Physics.=2, GetComponent=2, GameObject=2; unity_score=73
- ksp refs: FlightGlobals=5, Vessel=3, GameEvents=2, Part=1; fan_in=5 fan_out=4
- methods(11): Awake, FixedUpdate, GetAllVesselColliders, IgnoreCollidersOnVessel, OnCollisionIgnoreUpdate, OnDestroy, SetCollidersOnVessel, Start, UpdateAllColliders, UpdateCollisionIgnores, UpdatePartCollisionIgnores
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/ColorChangedEvent.cs
- 8 lines; kind=class; type=ColorChangedEvent; bases=UnityEvent<Color>; ancestry=UnityEvent; ns=(global)
- usings: UnityEngine, UnityEngine.Events
- unity: none; unity_score=14
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ColorHSV.cs
- 132 lines; kind=class; type=ColorHSV; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=10
- ksp refs: none; fan_in=5 fan_out=0
- methods(2): FromColor, ToColor
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ColorImage.cs
- 28 lines; kind=MonoBehaviour; type=ColorImage; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, Lifecycle=2, MonoBehaviour=1, GetComponent=1; unity_score=85
- ksp refs: none; fan_in=0 fan_out=1
- methods(3): Awake, ColorChanged, OnDestroy
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ColorLabel.cs
- 79 lines; kind=MonoBehaviour; type=ColorLabel; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine
- unity: Lifecycle=3, TMPro=2, MonoBehaviour=1, GetComponent=1, Application.=1; unity_score=62
- ksp refs: none; fan_in=0 fan_out=3
- methods(7): Awake, ColorChanged, ConvertToDisplayString, HSVChanged, OnDestroy, OnEnable, UpdateValue
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ColorMaterialProperty.cs
- 17 lines; kind=class; type=ColorMaterialProperty; bases=MaterialPropertyExtensions.MaterialProperty; ancestry=MaterialProperty; ns=(global)
- usings: UnityEngine
- unity: Shader=1, Material=1; unity_score=49
- ksp refs: none; fan_in=1 fan_out=1
- methods(1): Apply
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ColorPicker.cs
- 302 lines; kind=MonoBehaviour; type=ColorPicker; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: SetActive=5, Lifecycle=3, GameObject=3, UnityEngine.UI=1, Shader=1, MonoBehaviour=1; unity_score=48
- ksp refs: none; fan_in=11 fan_out=5
- methods(11): AssignColor, GetValue, HSVChanged, OnDestroy, OnEnable, RGBChanged, SendChangedEvent, SendInternalEvent, SetColor, Start, ToggleModeSliders
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ColorPickerTester.cs
- 25 lines; kind=MonoBehaviour; type=ColorPickerTester; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Material=3, Lifecycle=2, Renderer=1, MonoBehaviour=1; unity_score=86
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): Start, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/ColorPresets.cs
- 52 lines; kind=MonoBehaviour; type=ColorPresets; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, SetActive=2, MonoBehaviour=1, Lifecycle=1, GetComponent=1, GameObject=1; unity_score=77
- ksp refs: none; fan_in=1 fan_out=1
- methods(4): Awake, ColorChanged, CreatePresetButton, OnPresetSelected
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ColorSlider.cs
- 86 lines; kind=MonoBehaviour; type=ColorSlider; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, Lifecycle=2, MonoBehaviour=1, GetComponent=1; unity_score=60
- ksp refs: none; fan_in=0 fan_out=4
- methods(5): ColorChanged, HSVChanged, OnDestroy, SliderChanged, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ColorSliderImage.cs
- 199 lines; kind=MonoBehaviour; type=ColorSliderImage; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=14, Lifecycle=4, Texture=3, Instantiate/Destroy=2, Transform=1, MonoBehaviour=1; unity_score=74
- ksp refs: none; fan_in=0 fan_out=3
- methods(7): Awake, ColorChanged, HSVChanged, OnDestroy, OnDisable, OnEnable, RegenerateTexture
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ColorValues.cs
- 10 lines; kind=enum; type=ColorValues; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ComboButton.cs
- 62 lines; kind=MonoBehaviour; type=ComboButton; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=6, EventSystems=6, SetActive=2, Lifecycle=2, GameObject=2, MonoBehaviour=1; unity_score=93
- ksp refs: GameEvents=1; fan_in=2 fan_out=2
- methods(5): OnDestroy, OnSuitComboSelection, Reset, Select, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ComboSelector.cs
- 62 lines; kind=MonoBehaviour; type=ComboSelector; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, Material=3, Lifecycle=2, MonoBehaviour=1; unity_score=81
- ksp refs: GameEvents=1; fan_in=4 fan_out=3
- methods(5): Awake, OnButtonNext, OnButtonPrev, OnDestroy, SelectVariant
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/CometDefinition.cs
- 46 lines; kind=class; type=CometDefinition; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=3; fan_in=5 fan_out=2
- methods(1): CreateVesselNode
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/CometManager.cs **MUST-OPEN (>800 lines)**
- 845 lines; kind=MonoBehaviour; type=CometManager; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: FinePrint.Utilities, SentinelMission, UnityEngine
- unity: AnimationCurve=33, GameObject=5, Transform=4, Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=48
- ksp refs: FloatCurve=33, Vessel=15, ConfigNode=8, FlightGlobals=6, GameEvents=5, ProtoVessel=4; fan_in=10 fan_out=28
- methods(30): Awake, GenerateDefinition, GenerateHomeSpawnDistance, GenerateSentinelSpawnDistance, GenerateWeightedCometType, GenerateWeightedDustTailColor, GenerateWeightedIonTailColor, GetCometOrbitType, GetSpawnChance, Load, LoadColorsList, LoadCometTypes, LoadManagerDefinitions, LoadTailColorLists, OnDestroy, OnGameDatabaseLoaded, OnVesselAdded, OnVesselRemoved, ReprimeParticles, Save, SetCollider, SetComa, SetDust, SetDustTail, SetGeyser, SetIonTail, ShouldSpawnComet, SpawnCometFragment, SpawnCometFragments, Start
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/CometOrbitType.cs
- 211 lines; kind=class; type=CometOrbitType; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=32
- ksp refs: Orbit=8, FlightGlobals=3, Planetarium=2, ConfigNode=1, CelestialBody=1; fan_in=6 fan_out=8
- methods(5): CalculateHomeOrbit, CalculateOrbit, CalculateSentinelOrbit, GetRandomObjClass, Load
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/CometParticleController.cs
- 170 lines; kind=MonoBehaviour; type=CometParticleController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=12, Lifecycle=3, Transform=1, MonoBehaviour=1; unity_score=75
- ksp refs: TimeWarp=3, FloatCurve=3, GameEvents=2, Part=1; fan_in=1 fan_out=5
- methods(4): OnDestroy, OnPartWillDie, Start, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/CometVessel.cs
- 475 lines; kind=VesselModule; type=CometVessel; bases=VesselModule; ancestry=VesselModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=7, Coroutine=6, SetActive=4, GetComponent=4, Instantiate/Destroy=3, Lifecycle=2; unity_score=50
- ksp refs: KSPField=18, GameEvents=15, FlightGlobals=8, Vessel=7, Planetarium=7, ScaledSpace=5; fan_in=9 fan_out=19
- methods(19): MoveCometVesselModule, OnAwake, OnDestroy, OnKnowledgeChange, OnPartDecouple, OnPartPersistentIdChanged, OnStart, OnVesselPersistentIdChanged, ReprimeParticles, SetCollider, SetComa, SetDust, SetDustTail, SetGeyser, SetIonTail, SetSituationRanges, Update, UpdateVFXDimensions, WaitAndUpdateVFXDimensions
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/CometVesselExtension.cs
- 36 lines; kind=static class; type=CometVesselExtension; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): GetCopyOf
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/CometVFXController.cs
- 319 lines; kind=MonoBehaviour; type=CometVFXController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=20, GameObject=11, GetComponent=8, Instantiate/Destroy=7, ParticleSystem=6, Lifecycle=3; unity_score=80
- ksp refs: TimeWarp=7, GameEvents=6, Vector3d=5, Vessel=4, Part=3, ScaledSpace=1; fan_in=2 fan_out=13
- methods(13): DeSpawnVFX, DefineComaFX, DefineDustTailParticleFX, DefineIONTailParticleFX, Initialize, LateUpdate, OnCometVesselChanged, OnDestroy, OnPartWillDie, OnVesselWasLoaded, SpawnVFXObjects, Start, UpdateComaFX
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/CompoundPart.cs **MUST-OPEN (>800 lines)**
- 874 lines; kind=MonoBehaviour; type=CompoundPart; bases=Part; ancestry=Part/MonoBehaviour; ns=(global)
- usings: CompoundParts, UnityEngine
- unity: Transform=32, GameObject=10, Raycast/Query=7, Coroutine=7, LayerMask=4, Input.=4; unity_score=61
- ksp refs: HighLogic=19, Part=18, EditorLogic=14, GameEvents=6, InputLockManager=5, ConfigNode=5; fan_in=11 fan_out=22
- methods(48): DisconnectAction, DisconnectCompoundPart, DisconnectEvent, DumpTarget, EndPreview, GetPartColliders, GetReferenceParent, GetReferenceTransform, LateUpdate, LockEVAEditor, OnConstructionModeUpdate, OnEditorEvent, OnInventoryModeDisable, OnInventoryModeEnable, OnLoad, OnPartCreatedFomInventory, OnSave, ParseCustomPartData, PreviewAttachment, SetLink, SetSymmetryValues, SetTarget, ToggleSymmetryCounterpartsTweak, ToggleTweakTarget, UnLockEVAEditor, UnsetLink, UpdateTargetCoords, UpdateWorldValues, findTargetDirection, getAnchorRot, lockEditor, onAttachUpdate, onCopy, onEditorEndTweak, onEditorStartTweak, onEditorUpdate, onPartAttach, onPartAwake, onPartDestroy, onPartDetach, onPartFixedUpdate, onStartComplete, onTargetDestroy, onTargetDetach, onTargetReattach ...
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ConeShadowTestRotate.cs
- 11 lines; kind=MonoBehaviour; type=ConeShadowTestRotate; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=1, Time.=1, MonoBehaviour=1, Lifecycle=1; unity_score=63
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/ConeVolume.cs
- 80 lines; kind=MonoBehaviour; type=ConeVolume; bases=VolumetricObjectBase; ancestry=VolumetricObjectBase/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=5, Lifecycle=1; unity_score=64
- ksp refs: none; fan_in=0 fan_out=1
- methods(5): HasChanged, OnEnable, PopulateShaderName, SetChangedValues, UpdateVolume
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ConeVolumeOcclusion.cs
- 158 lines; kind=MonoBehaviour; type=ConeVolumeOcclusion; bases=VolumetricObjectBase; ancestry=VolumetricObjectBase/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=7, Shader=6, GameObject=6, Camera=4, Texture=3, GetComponent=3; unity_score=82
- ksp refs: none; fan_in=0 fan_out=2
- methods(9): CleanUp, ConeRenderTexture, CreateDepthCamera, HasChanged, OnEnable, PopulateShaderName, SetChangedValues, SetupCamera, UpdateVolume
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ConfigNode.cs **MUST-OPEN (>800 lines)**
- 4344 lines; kind=class; type=ConfigNode; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: GameObject=27, MonoBehaviour=13, Transform=8, Resources.=2, GetComponent=2, Instantiate/Destroy=1; unity_score=15
- ksp refs: ConfigNode=131, QuaternionD=15, Vector3d=14, Localizer=2; fan_in=667 fan_out=12
- methods(115): Add, AddData, AddNode, AddValue, AssignLink, AssignLinkable, AssignTarget, CheckAndParseColor, CleanupInput, Clear, ClearData, ClearNodes, ClearValues, CompileConfig, Contains, CopyTo, CopyToRecursive, CountByName, CreateConfigFromObject, CreateCopy, CreateNodeObject, CreateObject, CreateObjectFromConfig, CustomEqualSplit, CustomNameSplit, DebugStringArray, DistinctNames, FindIEnumerable, GetByLinkID, GetElementType, GetEnumerator, GetNode, GetNodeID, GetNodes, GetValue, GetValues, GetValuesList, GetValuesStartsWith, HasAttribute, HasNode, HasNodeID, HasValue, HasValues, IsArray, IsArrayType ...
- auto suggestion: tier=B subsystem=Save

## Assembly-CSharp/ConfigurableJointExtensions.cs
- 49 lines; kind=static class; type=ConfigurableJointExtensions; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Joint=4; unity_score=62
- ksp refs: none; fan_in=0 fan_out=0
- methods(4): GetExternalToJointSpaceRotation, SetTargetRotation, SetTargetRotationInternal, SetTargetRotationLocal
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/ConfirmDialog.cs
- 33 lines; kind=class; type=ConfirmDialog; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Screen/Cursor=2, IMGUI=1; unity_score=40
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ConsoleCommands.cs
- 565 lines; kind=MonoBehaviour; type=ConsoleCommands; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: FinePrint, KSP.UI.Screens, KSP.UI.Screens.DebugToolbar, UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1; unity_score=50
- ksp refs: FlightGlobals=29, HighLogic=12, Vessel=7, Reputation=7, Vector3d=6, ResearchAndDevelopment=5; fan_in=0 fan_out=29
- methods(19): AddStockCommands, OnBadassCommand, OnCommentCommand, OnFundsCommand, OnHelpCommand, OnInfoCommand, OnKSCCommand, OnRendezvousCommand, OnReputationCommand, OnScienceCommand, OnSettingsLoadCommand, OnSettingsSaveCommand, OnStackTraceCommand, OnSwitchCommand, OnTargetvCommand, OnTimeCommand, OnVesselsCommand, OnWaypointCommand, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/ConstructionEventType.cs
- 22 lines; kind=enum; type=ConstructionEventType; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=20 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/ConstructionMode.cs
- 7 lines; kind=enum; type=ConstructionMode; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=7 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/ConstructionVesselOverlays.cs
- 70 lines; kind=MonoBehaviour; type=ConstructionVesselOverlays; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=67
- ksp refs: none; fan_in=0 fan_out=1
- methods(6): Awake, OnDisable, Start, ToggleCoL, ToggleCoM, ToggleCoT
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/ContractOrbitRenderer.cs
- 65 lines; kind=MonoBehaviour; type=ContractOrbitRenderer; bases=OrbitTargetRenderer; ancestry=OrbitTargetRenderer/OrbitRenderer/OrbitRendererBase/MonoBehaviour; ns=(global)
- usings: Contracts, KSP.Localization, KSP.UI.Screens.Mapview
- unity: none; unity_score=15
- ksp refs: Localizer=8, Contract=3, Orbit=1, FlightGlobals=1; fan_in=3 fan_out=7
- methods(6): Setup, UpdateLocals, apNode_OnUpdateCaption, ascNode_OnUpdateCaption, descNode_OnUpdateCaption, peNode_OnUpdateCaption
- auto suggestion: tier=B subsystem=MapView

## Assembly-CSharp/ControlPoint.cs
- 38 lines; kind=class; type=ControlPoint; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Transform=6; unity_score=65
- ksp refs: ConfigNode=1; fan_in=5 fan_out=2
- methods(1): Load
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/ControlTypes.cs
- 95 lines; kind=enum; type=ControlTypes; bases=ulong; ancestry=-; ns=(global)
- usings: -
- unity: IMGUI=1; unity_score=11
- ksp refs: none; fan_in=151 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/ConversionRecipe.cs
- 58 lines; kind=class; type=ConversionRecipe; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=7 fan_out=1
- methods(4): Clear, SetInputs, SetOutputs, SetRequirements
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ConverterResults.cs
- 6 lines; kind=struct; type=ConverterResults; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=6 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/CoroutineHost.cs
- 39 lines; kind=MonoBehaviour; type=CoroutineHost; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Coroutine=6, Instantiate/Destroy=3, GameObject=3, MonoBehaviour=1, GetComponent=1; unity_score=89
- ksp refs: none; fan_in=2 fan_out=0
- methods(4): Create, RunAndDispose, StartCoroutine, Terminate
- auto suggestion: tier=B subsystem=Core

## Assembly-CSharp/CostWidget.cs
- 121 lines; kind=MonoBehaviour; type=CostWidget; bases=CurrencyWidget; ancestry=CurrencyWidget/MonoBehaviour; ns=(global)
- usings: KSP.UI, TMPro, UnityEngine
- unity: Transform=3, Lifecycle=2, TMPro=1; unity_score=47
- ksp refs: GameEvents=10, EditorLogic=7, HighLogic=4, Funding=4, ShipConstruct=1; fan_in=0 fan_out=15
- methods(10): Awake, DelayedStart, OnAboutToStart, OnCrewModified, OnDestroy, onCostChange, onCraftFileSelected, onCrewDialogChange, onShipModified, onShipReset
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/CraftThumbnail.cs
- 288 lines; kind=static class; type=CraftThumbnail; bases=-; ancestry=-; ns=(global)
- usings: KSP.UI.Screens, UnityEngine
- unity: GameObject=13, Transform=9, Texture=8, Light=6, Shader=5, Camera=5; unity_score=67
- ksp refs: ShipConstruct=4, HighLogic=3, Part=2, AvailablePart=2, ProtoPartSnapshot=1, PartLoader=1; fan_in=3 fan_out=16
- methods(7): CreateRenderTextures, GetPartIconTexturePath, GetThumbNailSetupIface, TakePartSnaphot, TakePartSnapshot, TakeSnaphot, TakeStockSnaphot
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/CrashObjectName.cs
- 11 lines; kind=MonoBehaviour; type=CrashObjectName; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=44
- ksp refs: none; fan_in=5 fan_out=0
- methods(0): 
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/CreditsMultiTextHeader.cs
- 78 lines; kind=MonoBehaviour; type=CreditsMultiTextHeader; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=6, Coroutine=5, Time.=1, MonoBehaviour=1, Lifecycle=1; unity_score=75
- ksp refs: none; fan_in=1 fan_out=0
- methods(4): NextText, SetText, Start, shiftText
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/CreditsMusic.cs
- 9 lines; kind=MonoBehaviour; type=CreditsMusic; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, GetComponent=1, Audio=1; unity_score=70
- ksp refs: GameSettings=1; fan_in=1 fan_out=1
- methods(1): Awake
- auto suggestion: tier=C subsystem=Audio

## Assembly-CSharp/CreditsSceneMain.cs
- 17 lines; kind=MonoBehaviour; type=CreditsSceneMain; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, KeyCode=1, Input.=1; unity_score=72
- ksp refs: HighLogic=1; fan_in=0 fan_out=2
- methods(2): Update, returnToMainMenu
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/CrewGenerator.cs
- 150 lines; kind=class; type=CrewGenerator; bases=-; ancestry=-; ns=(global)
- usings: Expansions.Missions, Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: ProtoCrewMember=17, Localizer=4, KerbalRoster=3, HighLogic=3, ConfigNode=2, Game=1; fan_in=9 fan_out=10
- methods(8): GetFileName, GetFullName, GetLastName, GetRandomName, Initialize, LoadNameArray, RandomCrewMemberPrototype, RemoveLastName
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/CrewHatchController.cs
- 249 lines; kind=MonoBehaviour; type=CrewHatchController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI.Screens.Flight.Dialogs, KSP.UI.TooltipTypes, UnityEngine, UnityEngine.EventSystems
- unity: Transform=8, GameObject=7, SetActive=4, Lifecycle=3, UnityEngine.UI=2, Raycast/Query=2; unity_score=71
- ksp refs: GameEvents=10, Part=8, FlightGlobals=5, Vessel=3, ProtoCrewMember=3, FlightCamera=1; fan_in=4 fan_out=21
- methods(19): Awake, DespawnUIs, DisableInterface, DismissDialog, EnableInterface, GetMouseUiPos, HideTooltip, LateUpdate, OnCrewDialogDismiss, OnCrewTransferDone, OnDestroy, OnEVABtn, OnTransferBtn, OnVesselSwitch, ShowHatchTooltip, ShowTooltip, SpawnCrewDialog, TooltipUpdate, onVesselSitChange
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/CrewTransfer.cs
- 115 lines; kind=MonoBehaviour; type=CrewTransfer; bases=PartItemTransfer; ancestry=PartItemTransfer/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: GetComponent=2, GameObject=1, Coroutine=1; unity_score=37
- ksp refs: Part=13, GameEvents=10, ProtoCrewMember=6, Localizer=2, FlightGlobals=2, Vessel=1; fan_in=2 fan_out=10
- methods(11): AfterPartsFound, Create, HookAdditionalEvents, IsSemiValidPart, IsValidPart, MoveCrewTo, OnPartSelect, UnhookAdditionalEvents, onChangeInactive, onGoForEva, waitAndCompleteTransfer
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/Currency.cs
- 11 lines; kind=enum; type=Currency; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Reputation=1; fan_in=20 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/CurrencyModifierQuery.cs
- 281 lines; kind=class; type=CurrencyModifierQuery; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=1
- ksp refs: Reputation=11, ResearchAndDevelopment=2, Funding=2, GameEvents=1; fan_in=21 fan_out=10
- methods(9): AddDelta, CanAfford, GetCostLine, GetEffectDelta, GetEffectDeltaText, GetEffectPercentageText, GetInput, GetTotal, RunQuery
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/CurrencyWidget.cs
- 28 lines; kind=MonoBehaviour; type=CurrencyWidget; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Coroutine=2, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1; unity_score=72
- ksp refs: none; fan_in=5 fan_out=0
- methods(3): DelayedStart, OnAboutToStart, Start
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/CutsceneCamera.cs
- 183 lines; kind=MonoBehaviour; type=CutsceneCamera; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: CameraKeyFrameEvents, UnityEngine
- unity: Transform=13, Time.=2, Lifecycle=2, MonoBehaviour=1, GameObject=1; unity_score=62
- ksp refs: none; fan_in=0 fan_out=2
- methods(9): CreateKeyFrame, EvalEvents, EvalPos, EvalRot, GoToFrame, SortKeyFrames, Start, Update, kfCompare
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/DatabaseGameObject.cs
- 40 lines; kind=MonoBehaviour; type=DatabaseGameObject; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.FX.Fireworks, UnityEngine
- unity: Instantiate/Destroy=2, GameObject=2, MonoBehaviour=1, Lifecycle=1; unity_score=72
- ksp refs: GameDatabase=1, ConfigNode=1; fan_in=1 fan_out=3
- methods(2): Awake, LoadFireworkFXDefinitions
- auto suggestion: tier=B subsystem=Modding

## Assembly-CSharp/DatabaseLoader.cs
- 46 lines; kind=class; type=DatabaseLoader; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=10 fan_out=2
- methods(2): CleanUp, Load
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/DatabaseLoaderAttrib.cs
- 12 lines; kind=attribute; type=DatabaseLoaderAttrib; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=10 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/DatabaseLoaderAudio.cs
- 29 lines; kind=class; type=DatabaseLoaderAudio; bases=DatabaseLoader<AudioClip>; ancestry=DatabaseLoader; ns=(global)
- usings: UnityEngine, UnityEngine.Networking
- unity: Coroutine=2, WWW/WebRequest=1, Audio=1; unity_score=59
- ksp refs: none; fan_in=0 fan_out=4
- methods(1): Load
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/DatabaseLoaderModel_DAE.cs **MUST-OPEN (>800 lines)**
- 1736 lines; kind=class; type=DatabaseLoaderModel_DAE; bases=DatabaseLoader<GameObject>; ancestry=DatabaseLoader; ns=(global)
- usings: UnityEngine
- unity: GameObject=49, Texture=27, GetComponent=18, Transform=17, Mesh=12, Shader=9; unity_score=49
- ksp refs: GameDatabase=1; fan_in=0 fan_out=9
- methods(26): AddReducedMeshColliders, Build, CanParseGeometry, CheckScale, DAEGetColor, FormatFaceData, GetDAEMaterials, GetFaceData, GetGeometryData, GetMaterial, GetSceneGeo, GetUpAxis, GetValidTexturePath, Load, LoadDAE, NormalMap, ReadDAE, RemoveWS, ScaleTexture, SolveMaterials, UpPosConv, UpRotDirConv, UpScaleConv, cf, ci, pushtooffsetscale
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DatabaseLoaderModel_MU.cs
- 24 lines; kind=class; type=DatabaseLoaderModel_MU; bases=DatabaseLoader<GameObject>; ancestry=DatabaseLoader; ns=(global)
- usings: UnityEngine
- unity: GameObject=5; unity_score=59
- ksp refs: none; fan_in=0 fan_out=4
- methods(1): Load
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DatabaseLoaderTexture_DDS.cs
- 75 lines; kind=class; type=DatabaseLoaderTexture_DDS; bases=DatabaseLoader<GameDatabase.TextureInfo>; ancestry=DatabaseLoader; ns=(global)
- usings: DDSHeaders, UnityEngine
- unity: Texture=7; unity_score=59
- ksp refs: GameDatabase=3; fan_in=0 fan_out=8
- methods(2): Load, Read
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DatabaseLoaderTexture_JPEG.cs
- 46 lines; kind=class; type=DatabaseLoaderTexture_JPEG; bases=DatabaseLoader<GameDatabase.TextureInfo>; ancestry=DatabaseLoader; ns=(global)
- usings: UnityEngine, UnityEngine.Networking
- unity: WWW/WebRequest=3, Texture=2, Coroutine=2; unity_score=65
- ksp refs: GameDatabase=6; fan_in=0 fan_out=5
- methods(1): Load
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DatabaseLoaderTexture_MBM.cs
- 31 lines; kind=class; type=DatabaseLoaderTexture_MBM; bases=DatabaseLoader<GameDatabase.TextureInfo>; ancestry=DatabaseLoader; ns=(global)
- usings: UnityEngine
- unity: Texture=1; unity_score=34
- ksp refs: GameDatabase=3; fan_in=0 fan_out=6
- methods(2): CleanUp, Load
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DatabaseLoaderTexture_PNG.cs
- 70 lines; kind=class; type=DatabaseLoaderTexture_PNG; bases=DatabaseLoader<GameDatabase.TextureInfo>; ancestry=DatabaseLoader; ns=(global)
- usings: UnityEngine, UnityEngine.Networking
- unity: Texture=7, WWW/WebRequest=3, Coroutine=2; unity_score=67
- ksp refs: GameDatabase=6; fan_in=0 fan_out=5
- methods(1): Load
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DatabaseLoaderTexture_TGA.cs
- 41 lines; kind=class; type=DatabaseLoaderTexture_TGA; bases=DatabaseLoader<GameDatabase.TextureInfo>; ancestry=DatabaseLoader; ns=(global)
- usings: UnityEngine
- unity: Texture=1; unity_score=28
- ksp refs: GameDatabase=5; fan_in=0 fan_out=6
- methods(2): CleanUp, Load
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DatabaseLoaderTexture_TRUECOLOR.cs
- 33 lines; kind=class; type=DatabaseLoaderTexture_TRUECOLOR; bases=DatabaseLoader<GameDatabase.TextureInfo>; ancestry=DatabaseLoader; ns=(global)
- usings: UnityEngine
- unity: Texture=3; unity_score=55
- ksp refs: GameDatabase=6; fan_in=0 fan_out=4
- methods(1): Load
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DayNightGameObjectSwitch.cs
- 137 lines; kind=MonoBehaviour; type=DayNightGameObjectSwitch; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Coroutine=6, Lifecycle=5, GameObject=3, Instantiate/Destroy=2, Transform=1, SetActive=1; unity_score=69
- ksp refs: HighLogic=3, GameEvents=2, CelestialBody=2; fan_in=0 fan_out=5
- methods(9): OnDestroy, OnDisable, OnEnable, Setup, Start, Update, UpdateObjects, onLevelWasLoaded, setObjects
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/Debug_FloatNaNs.cs
- 22 lines; kind=MonoBehaviour; type=Debug_FloatNaNs; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1; unity_score=50
- ksp refs: none; fan_in=0 fan_out=0
- methods(3): Start, _clearfp, _control87
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/DebugDrawUtil.cs
- 17 lines; kind=class; type=DebugDrawUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=47
- ksp refs: none; fan_in=8 fan_out=0
- methods(2): DrawCrosshairs, DrawDirection
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/DebugToolbar.cs **MUST-OPEN (>800 lines)**
- 1941 lines; kind=MonoBehaviour; type=DebugToolbar; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Contracts, KSP.UI, KSP.UI.Screens, UniLinq, UnityEngine, UnityEngine.Profiling, UnityEngine.SceneManagement
- unity: IMGUI=650, UnityEngine.UI=97, AnimationCurve=8, Coroutine=6, Input.=4, GetComponent=4; unity_score=98
- ksp refs: PhysicsGlobals=88, HighLogic=42, FlightGlobals=29, GameDatabase=13, Contract=10, ResearchAndDevelopment=7; fan_in=86 fan_out=42
- methods(63): CheatInExperience, CheatInFunds, CheatInReputation, CheatInScience, ContractGenerate, ContractsActive, ContractsAdd, ContractsArchive, ContractsOffered, ContractsTools, DatabaseAssemblies, DatabaseAudio, DatabaseConfigPreview, DatabaseConfigs, DatabaseConfigsSpam, DatabaseModels, DatabaseOverview, DatabaseOverviewConfig, DatabaseTextures, DrawFlightStats, DrawWindowRnD, FloatCurveEditor, FloatField, GetAnimationCurvePoints, GetSurfaceDragCurvePoints, KerbalDatabaseSelector, KerbalRosterAll, KerbalRosterCareer, KerbalRosterDetails, KerbalRosterFlight, MissionsActive, MissionsCompleted, MissionsTools, OnDestroy, OnGUI, OnLevelLoaded, OnRnDDespawn, OnRnDSpawn, OnSceneLoaded, RecompileAssets, SetBiomesVisible, ShowWhackAKerbalDialog, Start, Update, Window ...
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/DefaultIcons.cs
- 26 lines; kind=enum; type=DefaultIcons; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=7 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DelegateExtensions.cs
- 29 lines; kind=static class; type=DelegateExtensions; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): Clear
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/DeltaVApp.cs
- 230 lines; kind=MonoBehaviour; type=DeltaVApp; bases=UIApp; ancestry=UIApp/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, UnityEngine
- unity: Transform=8, Instantiate/Destroy=4, GameObject=4, UnityEngine.UI=2, SetActive=2, Analytics=1; unity_score=63
- ksp refs: HighLogic=6, GameEvents=5, GameSettings=3, Localizer=1; fan_in=7 fan_out=13
- methods(11): AnyTextFieldHasFocus, DisplayApp, GetAppScenes, GetAppScreenPos, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnSceneSwitch, Reset, TextFieldHasFocus
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/DeltaVAppSituation.cs
- 350 lines; kind=MonoBehaviour; type=DeltaVAppSituation; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=11, TMPro=4, Lifecycle=3, GetComponent=2, MonoBehaviour=1; unity_score=58
- ksp refs: FlightGlobals=8, EditorLogic=4, Planetarium=2, CelestialBody=2, Localizer=1, GameEvents=1; fan_in=2 fan_out=15
- methods(18): AltitudeHasFocus, AltitudeSliderChanged, AltitudeTextChanged, Awake, BodyChanged, CacheLocalStrings, OnDestroy, RefreshBodyList, SetAtmosphereSituation, SliderEndDrag, SliderPointerUp, SliderStartDrag, Start, ToggleAltitudeOn, ToggleSeaLevelOn, ToggleVacuumOn, UpdatePressureDisplay, UpdateVesselDeltaVValues
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/DeltaVAppStageInfo.cs
- 77 lines; kind=MonoBehaviour; type=DeltaVAppStageInfo; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI.Screens, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=5, Transform=3, Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1; unity_score=81
- ksp refs: none; fan_in=1 fan_out=4
- methods(7): Awake, FillInfoToggles, HideAllClicked, OnDestroy, SetColumnLayout, ShowAllClicked, Start
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/DeltaVAppStageInfoToggle.cs
- 46 lines; kind=MonoBehaviour; type=DeltaVAppStageInfoToggle; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: Transform=3, Lifecycle=3, UnityEngine.UI=1, TMPro=1, MonoBehaviour=1; unity_score=81
- ksp refs: none; fan_in=1 fan_out=3
- methods(5): OnDestroy, Setup, Start, ToggleCheckbox, Update
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/DeltaVAppValues.cs
- 127 lines; kind=class; type=DeltaVAppValues; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: GameSettings=2, GameEvents=1, FlightGlobals=1, CelestialBody=1; fan_in=11 fan_out=7
- methods(4): GetValue, InitEnabledValue, LoadEnabledInfoLines, UpdateEnabledInfoItems
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/DeltaVCalc.cs
- 76 lines; kind=class; type=DeltaVCalc; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/DeltaVEngineBurnTotals.cs
- 20 lines; kind=class; type=DeltaVEngineBurnTotals; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/DeltaVEngineExtensions.cs
- 42 lines; kind=static class; type=DeltaVEngineExtensions; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=2; fan_in=0 fan_out=2
- methods(2): Contains, Get
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/DeltaVEngineInfo.cs
- 796 lines; kind=class; type=DeltaVEngineInfo; bases=-; ancestry=-; ns=(global)
- usings: KSP.UI.Screens, UnityEngine
- unity: none; unity_score=6
- ksp refs: HighLogic=9, FlightGlobals=5, Vessel=2; fan_in=9 fan_out=17
- methods(32): ApplyBurn, CalcThrustActual, CalculateBurn, CalculateFuelTime, CalculateISP, CalculateThrustVector, GetFuelTimeAtActiveThrottle, GetFuelTimeAtThrottle, GetFuelTimeMaxThrottle, GetISPASL, GetISPActual, GetISPVac, GetSituationISP, GetSituationThrust, GetSituationThrustVector, GetThrustASL, GetThrustActual, GetThrustVac, GetThrustVectorASL, GetThrustVectorActual, GetThrustVectorVac, PropellantDemand, PropellantMassBurnt, PropellantStarved, RequiresAir, Reset, ResetCalcVariables, ResetPropellantBurnValues, ResetResourceTotals, ResetStageInfo, SwitchEngine, ThrottleLimited
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/DeltaVEngineResourcePartExtensions.cs
- 71 lines; kind=static class; type=DeltaVEngineResourcePartExtensions; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=2
- methods(5): ContainsPartInfo, Get, GetHighestPartSeparationIndex, PartInfoList, PartsMatchingSeparationIndex
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/DeltaVEngineResourcePartInfo.cs
- 17 lines; kind=class; type=DeltaVEngineResourcePartInfo; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/DeltaVEngineStageSet.cs
- 507 lines; kind=class; type=DeltaVEngineStageSet; bases=-; ancestry=-; ns=(global)
- usings: KSP.UI.Screens, UnityEngine
- unity: none; unity_score=5
- ksp refs: Vessel=16, HighLogic=1; fan_in=1 fan_out=7
- methods(9): AddEngineWorkingSet, AddStageInfo, Compare, RemoveInvalidEnginesWorkingSet, RemoveStagedStages, RemoveStaleStages, SetPayload, SortStages, UpdateStageInfo
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/DeltaVExtensions.cs
- 15 lines; kind=static class; type=DeltaVExtensions; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): GetSwitchedValue
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/DeltaVGlobals.cs
- 144 lines; kind=MonoBehaviour; type=DeltaVGlobals; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1, GetComponent=1; unity_score=40
- ksp refs: Localizer=14, GameEvents=5, GameSettings=3, FlightGlobals=1, CelestialBody=1; fan_in=10 fan_out=9
- methods(8): Awake, DisableStockSimluations, EnableStockSimluations, InitDeltaVGlobal, InitInfoLines, OnDestroy, OnSceneSwitch, Start
- auto suggestion: tier=B subsystem=Vessel

## Assembly-CSharp/DeltaVPartExtensions.cs
- 55 lines; kind=static class; type=DeltaVPartExtensions; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=5; fan_in=0 fan_out=2
- methods(3): ContainsPart, Get, PartsInStage
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/DeltaVPartInfo.cs
- 357 lines; kind=class; type=DeltaVPartInfo; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=5
- ksp refs: Part=7, GameSettings=4, PartResourceDefinition=2, PartResource=2, AttachNode=2; fan_in=6 fan_out=21
- methods(12): CalculateMassValues, CalculateStagingValues, CheckDecoupler, FindDecouplerParent, GetCurrentFuelMass, GetSeparationStage, GetStageEndMass, GetStageStartMass, JettisonInStage, ResetStartSimulationResources, StageEndFuelMass, StageStartFuelMass
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/DeltaVPropellantInfo.cs
- 68 lines; kind=class; type=DeltaVPropellantInfo; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=6
- ksp refs: none; fan_in=6 fan_out=2
- methods(2): Copy, MassBurnt
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/DeltaVSituationOptions.cs
- 6 lines; kind=enum; type=DeltaVSituationOptions; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=11 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/DeltaVStageInfo.cs **MUST-OPEN (>800 lines)**
- 1224 lines; kind=class; type=DeltaVStageInfo; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, RUI.Algorithms, UnityEngine
- unity: Resources.=1; unity_score=12
- ksp refs: PhysicsGlobals=12, Vessel=11, FlightGlobals=10, HighLogic=5, GameSettings=5, Localizer=3; fan_in=12 fan_out=24
- methods(35): CalcLerpDeltaV, CalculateEngineResourceFuelMass, CalculateISP, CalculateStartMass, CalculateTWR, CalculateTimeRequiredDV, CheckTimeStep, CoTForStage, ContainsAnchoredDecoupler, ContainsDecoupler, EnginesDeprived, GetCurrentStageMass, GetEngineResourceParts, GetHighestSeparationIndex, GetPartDisplayInfo, GetSituationDeltaV, GetSituationISP, GetSituationTWR, GetSituationThrust, GetSituationTotalExhaustVelocity, GetSituationVectoredExhaustVelocity, GetSituationVectoredThrust, GetStageMass, MatchingSeparationIndex, PartsActivateInStage, PartsActiveInStage, PartsDecoupledInStage, ProcessActiveEngines, ProcessParts, Reset, ResetPartCaches, SimulateDeltaV, StoreEndFuelMass, StoreInterimDeltaV, StoreStartFuelMass
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/DepletionData.cs
- 15 lines; kind=class; type=DepletionData; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/DepletionNode.cs
- 10 lines; kind=class; type=DepletionNode; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/DestroyAfterTime.cs
- 16 lines; kind=MonoBehaviour; type=DestroyAfterTime; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: SendMessage/Invoke=1, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1, GameObject=1; unity_score=72
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Kill, Start
- auto suggestion: tier=B subsystem=Core

## Assembly-CSharp/DestroyOnSceneSwitch.cs
- 32 lines; kind=MonoBehaviour; type=DestroyOnSceneSwitch; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.SceneManagement
- unity: SceneManager=3, Lifecycle=2, Instantiate/Destroy=2, GameObject=2, MonoBehaviour=1; unity_score=86
- ksp refs: GameEvents=2; fan_in=0 fan_out=2
- methods(5): OnDestroy, OnLevelLoaded, OnSceneLoaded, OnSceneSwitch, Start
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/DestructibleBuilding.cs
- 718 lines; kind=MonoBehaviour; type=DestructibleBuilding; bases=MonoBehaviour,ICollisionEvents; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Coroutine=19, GameObject=15, Transform=12, SetActive=8, Lifecycle=6, Collision/Force=6; unity_score=66
- ksp refs: GameEvents=15, HighLogic=8, PhysicsGlobals=7, ConfigNode=4, Vessel=3, FlightGlobals=3; fan_in=6 fan_out=23
- methods(43): AddDamage, Awake, ClearID, Collapse, CompileID, Demolish, EndFlightNoDamagePeriod, FindVesselsOverStructure, FixedUpdate, GetInstance, GetRepairSequenceDuration, Init, Load, LoadOnSceneStart, OnCollisionEnter, OnCollisionExit, OnCollisionStay, OnDestroy, OnDisable, OnEnable, OnLeavingScene, OnLevelLoaded, OnPhysicsEase, OnSceneStart, RegisterInstance, Repair, Reset, Save, SetDestroyed, Start, StartFlightNoDamagePeriod, UnregisterInstance, clearVFX, collapseCoroutine, damageUpdate, delayedCallback, disableCoroutine, onDemolitionComplete, onFXComplete, onRepairComplete, repairCoroutine, restartNoDamagePeriod, spawnVFX
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/Detonator.cs
- 413 lines; kind=MonoBehaviour; type=Detonator; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Material=32, Texture=16, Shader=16, GetComponent=10, GameObject=9, Resources.=8; unity_score=88
- ksp refs: none; fan_in=12 fan_out=9
- methods(15): Awake, DefaultFireballAMaterial, DefaultFireballBMaterial, DefaultGlowMaterial, DefaultHeatwaveMaterial, DefaultShockwaveMaterial, DefaultSmokeAMaterial, DefaultSmokeBMaterial, DefaultSparksMaterial, Explode, FillDefaultMaterials, Reset, Start, Update, UpdateComponents
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DetonatorBurstEmitter.cs
- 262 lines; kind=MonoBehaviour; type=DetonatorBurstEmitter; bases=DetonatorComponent; ancestry=DetonatorComponent/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=22, Material=4, Time.=3, GetComponent=3, Lifecycle=2, GameObject=2; unity_score=77
- ksp refs: none; fan_in=6 fan_out=1
- methods(6): Awake, Explode, Init, Reset, SizeFunction, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DetonatorCloudRing.cs
- 93 lines; kind=MonoBehaviour; type=DetonatorCloudRing; bases=DetonatorComponent; ancestry=DetonatorComponent/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=4, Material=2, GameObject=2, GetComponent=1; unity_score=66
- ksp refs: none; fan_in=0 fan_out=3
- methods(6): BuildCloudRing, Explode, FillMaterials, Init, Reset, UpdateCloudRing
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DetonatorComponent.cs
- 72 lines; kind=MonoBehaviour; type=DetonatorComponent; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, GetComponent=1; unity_score=46
- ksp refs: none; fan_in=13 fan_out=1
- methods(4): Explode, Init, MyDetonator, SetStartValues
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DetonatorDriver.cs
- 14 lines; kind=MonoBehaviour; type=DetonatorDriver; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1; unity_score=50
- ksp refs: GameSettings=1; fan_in=0 fan_out=2
- methods(2): Awake, GetDetonatorVolume
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DetonatorFireball.cs
- 211 lines; kind=MonoBehaviour; type=DetonatorFireball; bases=DetonatorComponent; ancestry=DetonatorComponent/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=15, Material=6, GameObject=6, GetComponent=3, Camera=1; unity_score=74
- ksp refs: none; fan_in=1 fan_out=3
- methods(10): BuildFireShadow, BuildFireballA, BuildFireballB, Explode, FillMaterials, Init, Reset, UpdateFireShadow, UpdateFireballA, UpdateFireballB
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DetonatorForce.cs
- 109 lines; kind=MonoBehaviour; type=DetonatorForce; bases=DetonatorComponent; ancestry=DetonatorComponent/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=7, GetComponent=4, Collider=3, Rigidbody=2, ParticleSystem=2, Instantiate/Destroy=2; unity_score=85
- ksp refs: none; fan_in=1 fan_out=2
- methods(4): Explode, Init, Reset, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DetonatorGlow.cs
- 104 lines; kind=MonoBehaviour; type=DetonatorGlow; bases=DetonatorComponent; ancestry=DetonatorComponent/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=4, Material=2, GameObject=2, Lifecycle=1, GetComponent=1; unity_score=66
- ksp refs: none; fan_in=1 fan_out=3
- methods(7): BuildGlow, Explode, FillMaterials, Init, Reset, Update, UpdateGlow
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DetonatorHeatwave.cs
- 100 lines; kind=MonoBehaviour; type=DetonatorHeatwave; bases=DetonatorComponent; ancestry=DetonatorComponent/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=7, Material=5, GameObject=4, GetComponent=3, Time.=2, Shader=2; unity_score=89
- ksp refs: none; fan_in=1 fan_out=2
- methods(4): Explode, Init, Reset, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DetonatorLight.cs
- 64 lines; kind=MonoBehaviour; type=DetonatorLight; bases=DetonatorComponent; ancestry=DetonatorComponent/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=3, Time.=3, Light=3, GameObject=2, Lifecycle=1, GetComponent=1; unity_score=77
- ksp refs: none; fan_in=1 fan_out=2
- methods(4): Explode, Init, Reset, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DetonatorShockwave.cs
- 88 lines; kind=MonoBehaviour; type=DetonatorShockwave; bases=DetonatorComponent; ancestry=DetonatorComponent/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=5, Material=2, GameObject=2, GetComponent=1; unity_score=67
- ksp refs: none; fan_in=1 fan_out=3
- methods(6): BuildShockwave, Explode, FillMaterials, Init, Reset, UpdateShockwave
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DetonatorSmoke.cs
- 165 lines; kind=MonoBehaviour; type=DetonatorSmoke; bases=DetonatorComponent; ancestry=DetonatorComponent/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=16, Material=4, GameObject=4, GetComponent=2, Camera=2; unity_score=78
- ksp refs: none; fan_in=1 fan_out=3
- methods(8): BuildSmokeA, BuildSmokeB, Explode, FillMaterials, Init, Reset, UpdateSmokeA, UpdateSmokeB
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DetonatorSound.cs
- 83 lines; kind=MonoBehaviour; type=DetonatorSound; bases=DetonatorComponent; ancestry=DetonatorComponent/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Audio=4, Transform=2, Time.=2, Lifecycle=1, GetComponent=1, GameObject=1; unity_score=72
- ksp refs: none; fan_in=1 fan_out=2
- methods(4): Explode, Init, Reset, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DetonatorSparks.cs
- 97 lines; kind=MonoBehaviour; type=DetonatorSparks; bases=DetonatorComponent; ancestry=DetonatorComponent/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=4, Physics.=2, Material=2, GameObject=2, GetComponent=1; unity_score=69
- ksp refs: none; fan_in=1 fan_out=3
- methods(6): BuildSparks, Explode, FillMaterials, Init, Reset, UpdateSparks
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DetonatorSpray.cs
- 75 lines; kind=MonoBehaviour; type=DetonatorSpray; bases=DetonatorComponent; ancestry=DetonatorComponent/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=5, Instantiate/Destroy=2, GameObject=2, Time.=1, Rigidbody=1, Lifecycle=1; unity_score=75
- ksp refs: none; fan_in=0 fan_out=2
- methods(4): Explode, Init, Reset, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DialogCanvasUtil.cs
- 33 lines; kind=static class; type=DialogCanvasUtil; bases=-; ancestry=-; ns=(global)
- usings: KSP.UI, UnityEngine
- unity: UnityEngine.UI=5, GetComponent=1; unity_score=67
- ksp refs: none; fan_in=25 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUIBase.cs
- 302 lines; kind=class; type=DialogGUIBase; bases=-; ancestry=-; ns=(global)
- usings: KSP.UI.TooltipTypes, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=15, SetActive=9, GetComponent=9, GameObject=6, TMPro=3, Transform=2; unity_score=60
- ksp refs: none; fan_in=39 fan_out=11
- methods(10): AddChild, AddChildren, Create, GetTextObject, Resize, SelectFirstItem, SetOptionText, SetUpTextObject, SetupTransformAndLayout, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUIBox.cs
- 50 lines; kind=class; type=DialogGUIBox; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: GetComponent=5, UnityEngine.UI=3, Transform=2, SetActive=1, Instantiate/Destroy=1, GameObject=1; unity_score=70
- ksp refs: none; fan_in=10 fan_out=6
- methods(1): Create
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUIButton.cs
- 324 lines; kind=class; type=DialogGUIButton; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=19, GetComponent=15, SetActive=5, Texture=3, GameObject=3, Transform=2; unity_score=62
- ksp refs: none; fan_in=85 fan_out=8
- methods(5): ClearButtonImage, Create, OptionSelected, Resize, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUIContentSizer.cs
- 57 lines; kind=class; type=DialogGUIContentSizer; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=15, GetComponent=4, Transform=2, GameObject=2, Lifecycle=1; unity_score=82
- ksp refs: none; fan_in=13 fan_out=3
- methods(3): Create, Resize, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUIFlexibleSpace.cs
- 33 lines; kind=class; type=DialogGUIFlexibleSpace; bases=DialogGUISpace; ancestry=DialogGUISpace/DialogGUIBase; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: GetComponent=14, GameObject=12, UnityEngine.UI=6, Transform=3; unity_score=90
- ksp refs: none; fan_in=21 fan_out=2
- methods(1): Create
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUIGridLayout.cs
- 70 lines; kind=class; type=DialogGUIGridLayout; bases=DialogGUILayoutBase; ancestry=DialogGUILayoutBase/DialogGUIBase; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=12, GetComponent=5, Transform=2, GameObject=2, SetActive=1, Instantiate/Destroy=1; unity_score=77
- ksp refs: none; fan_in=3 fan_out=4
- methods(1): Create
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUIHorizontalLayout.cs
- 49 lines; kind=class; type=DialogGUIHorizontalLayout; bases=DialogGUILayoutBase; ancestry=DialogGUILayoutBase/DialogGUIBase; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=0
- ksp refs: none; fan_in=35 fan_out=2
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUIImage.cs
- 39 lines; kind=class; type=DialogGUIImage; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, Texture=3, GetComponent=3, Transform=2, SetActive=1, Instantiate/Destroy=1; unity_score=79
- ksp refs: none; fan_in=15 fan_out=3
- methods(1): Create
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUILabel.cs
- 123 lines; kind=class; type=DialogGUILabel; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: TMPro, UnityEngine
- unity: Transform=2, TMPro=2, SetActive=1, Lifecycle=1, Instantiate/Destroy=1, GetComponent=1; unity_score=41
- ksp refs: none; fan_in=45 fan_out=6
- methods(2): Create, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUILayoutBase.cs
- 87 lines; kind=class; type=DialogGUILayoutBase; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=6, GetComponent=5, Transform=3, GameObject=2, SetActive=1, Instantiate/Destroy=1; unity_score=66
- ksp refs: none; fan_in=6 fan_out=6
- methods(1): Create
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUILayoutEnd.cs
- 11 lines; kind=class; type=DialogGUILayoutEnd; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: UnityEngine
- unity: Transform=1, GameObject=1; unity_score=35
- ksp refs: none; fan_in=9 fan_out=2
- methods(1): Create
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUIRadialLayout.cs
- 128 lines; kind=class; type=DialogGUIRadialLayout; bases=DialogGUILayoutBase; ancestry=DialogGUILayoutBase/DialogGUIBase; ns=(global)
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=10, Transform=9, GameObject=9, GetComponent=4, Input.=2, SetActive=1; unity_score=74
- ksp refs: none; fan_in=1 fan_out=5
- methods(4): Create, Resize, SelectedItemAtAngle, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUIScrollList.cs
- 120 lines; kind=class; type=DialogGUIScrollList; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=20, GameObject=12, GetComponent=8, Transform=7, SetActive=5, Lifecycle=1; unity_score=81
- ksp refs: none; fan_in=13 fan_out=5
- methods(3): Create, Resize, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUISlider.cs
- 59 lines; kind=class; type=DialogGUISlider; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: UnityEngine, UnityEngine.Events, UnityEngine.UI
- unity: UnityEngine.UI=5, GetComponent=3, Transform=2, SetActive=1, Lifecycle=1, Instantiate/Destroy=1; unity_score=69
- ksp refs: none; fan_in=7 fan_out=3
- methods(2): Create, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUISpace.cs
- 35 lines; kind=class; type=DialogGUISpace; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: Transform=4, UnityEngine.UI=3, GetComponent=3, GameObject=2; unity_score=76
- ksp refs: none; fan_in=18 fan_out=3
- methods(1): Create
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUISprite.cs
- 27 lines; kind=class; type=DialogGUISprite; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, Transform=2, Texture=2, GetComponent=2, SetActive=1, Instantiate/Destroy=1; unity_score=78
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): Create
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUITextInput.cs
- 123 lines; kind=class; type=DialogGUITextInput; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=8, GetComponent=5, UnityEngine.UI=2, Transform=2, SetActive=1, Lifecycle=1; unity_score=63
- ksp refs: none; fan_in=9 fan_out=6
- methods(2): Create, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUIToggle.cs
- 150 lines; kind=class; type=DialogGUIToggle; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=11, Transform=6, Texture=4, SetActive=3, GetComponent=3, GameObject=3; unity_score=70
- ksp refs: none; fan_in=17 fan_out=5
- methods(2): Create, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUIToggleButton.cs
- 48 lines; kind=class; type=DialogGUIToggleButton; bases=DialogGUIToggle; ancestry=DialogGUIToggle/DialogGUIBase; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, GetComponent=3, GameObject=3, Transform=2, SetActive=1, Instantiate/Destroy=1; unity_score=71
- ksp refs: none; fan_in=12 fan_out=5
- methods(1): Create
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUIToggleGroup.cs
- 26 lines; kind=class; type=DialogGUIToggleGroup; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, GetComponent=2, GameObject=2, Transform=1; unity_score=75
- ksp refs: none; fan_in=5 fan_out=3
- methods(1): Create
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogGUIVerticalLayout.cs
- 47 lines; kind=class; type=DialogGUIVerticalLayout; bases=DialogGUILayoutBase; ancestry=DialogGUILayoutBase/DialogGUIBase; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=1; unity_score=15
- ksp refs: none; fan_in=40 fan_out=2
- methods(1): Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DialogOrbitTutorialStep.cs
- 149 lines; kind=class; type=DialogOrbitTutorialStep; bases=IntermediateTutorialPageStep; ancestry=IntermediateTutorialPageStep/TutorialPageConfig; ns=(global)
- usings: Expansions.Missions, Expansions.Missions.Editor
- unity: none; unity_score=0
- ksp refs: GameEvents=7, Orbit=1; fan_in=1 fan_out=14
- methods(13): AddTutorialStepConfig, FilterDialogNodeCriteria, HighlightDialogueMessage, IsDialogLinked, IsDialogueMessageSetted, OnEnterDragDialogueNode, OnEnterSetDialogueTextHelper, OnLeaveOnEnterDragDialogueNode, OnLeaveSetDialogueTextHelper, OnNodeEventDialogueTuto, OnNodeFocused, OnUpdateDragDialogueNode, OnUpdateSetDialogueTextHelper
- auto suggestion: tier=A subsystem=Tutorials

## Assembly-CSharp/DialogScienceTutorialStep.cs
- 152 lines; kind=class; type=DialogScienceTutorialStep; bases=IntermediateTutorialPageStep; ancestry=IntermediateTutorialPageStep/TutorialPageConfig; ns=(global)
- usings: Expansions.Missions, Expansions.Missions.Editor
- unity: none; unity_score=0
- ksp refs: GameEvents=7; fan_in=1 fan_out=13
- methods(13): AddTutorialStepConfig, FilterDialogNodeCriteria, HighlightDialogueMessage, IsDialogLinked, IsDialogueMessageSetted, OnEnterDragSecondDialogue, OnEnterSetDialogueTextHelper, OnLeaveLinkScienceEventNode, OnLeaveSetDialogueTextHelper, OnNodeEventScienceTuto, OnNodeFocused, OnUpdateLinkScienceEventNode, OnUpdateSetDialogueTextHelper
- auto suggestion: tier=A subsystem=Tutorials

## Assembly-CSharp/DictionaryValueList.cs
- 139 lines; kind=class; type=DictionaryValueList; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=8
- ksp refs: none; fan_in=51 fan_out=0
- methods(12): Add, At, Clear, Contains, ContainsKey, GetDictEnumerator, GetListEnumerator, IndexOf, KeyAt, Remove, SortList, TryGetValue
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/DifficultyOptionsMenu.cs
- 767 lines; kind=MonoBehaviour; type=DifficultyOptionsMenu; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, Expansions.Missions, KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=6, GameObject=5, GetComponent=2, TMPro=1, SetActive=1, MonoBehaviour=1; unity_score=40
- ksp refs: Localizer=38, Game=20, GameEvents=3, InputLockManager=2, ConfigNode=2; fan_in=4 fan_out=30
- methods(5): Create, CreateDifficultWindow, Dismiss, SetPreset, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DirectionTarget.cs
- 34 lines; kind=class; type=DirectionTarget; bases=PositionTarget; ancestry=PositionTarget/ITargetable; ns=(global)
- usings: -
- unity: Transform=1; unity_score=19
- ksp refs: Vector3d=4, Vessel=1, FlightGlobals=1; fan_in=4 fan_out=5
- methods(1): Update
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/DirectoryActionGroup.cs
- 466 lines; kind=MonoBehaviour; type=DirectoryActionGroup; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI.Screens, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=14, UnityEngine.UI=10, Transform=9, SetActive=8, GetComponent=7, EventSystems=6; unity_score=72
- ksp refs: GameEvents=2; fan_in=1 fan_out=6
- methods(27): AddExtraSpacing, AddItemToContent, AttachListener, Awake, Collapse, Deselect, Expand, ExpandOverride, GetFileCount, Initialize, OnDestroy, OnDirectoryClicked, OnExpandToggleClick, OnScroll, RemoveItemFromContent, RescaleContentItems, Select, SetLabelColor, SetName, SetUIState, Start, UpdateContentSize, UpdateCraftFileCount, UpdateExpansionIcon, UpdateFileCount, UpdateSteamFileCount, UpdateStockFileCount
- auto suggestion: tier=C subsystem=Vessel

## Assembly-CSharp/DisableOnPlay.cs
- 9 lines; kind=MonoBehaviour; type=DisableOnPlay; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: SetActive=1, MonoBehaviour=1, Lifecycle=1, GameObject=1; unity_score=67
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): Start
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/DiscoverableObjectsUtil.cs
- 109 lines; kind=static class; type=DiscoverableObjectsUtil; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: ProtoVessel=10, ConfigNode=10, HighLogic=9, Orbit=2, GameEvents=2, Localizer=1; fan_in=6 fan_out=12
- methods(6): GenerateAsteroidName, GenerateCometName, SpawnAsteroid, SpawnComet, randomLetter, randomNumber
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/DiscoveryInfo.cs
- 490 lines; kind=class; type=DiscoveryInfo; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=1
- ksp refs: Localizer=47, Planetarium=2, GameEvents=2, ConfigNode=2; fan_in=56 fan_out=9
- methods(20): CacheLocalStrings, CompileInfoGetters, GetClassFromSize, GetClassRadius, GetMaxRadius, GetMinRadius, GetObjectClass, GetSignalLife, GetSignalStrength, GetSignalStrengthCaption, GetSizeClassDescription, GetSizeClassSizes, HaveKnowledgeAbout, Load, Save, SetLastObservedTime, SetLevel, SetReferenceLifetime, SetUnobservedLifetime, SetUntrackedObjectSize
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/DiscoveryLevels.cs
- 13 lines; kind=enum; type=DiscoveryLevels; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=46 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/DishController.cs
- 172 lines; kind=MonoBehaviour; type=DishController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, Time.=2, Lifecycle=2, MonoBehaviour=1; unity_score=49
- ksp refs: Planetarium=4; fan_in=0 fan_out=1
- methods(4): CreateTimeStep, SetTimeStep, Start, Update
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/DisplacementExplosion.cs
- 67 lines; kind=MonoBehaviour; type=DisplacementExplosion; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Time.=2, Shader=2, Renderer=2, Material=2, Lifecycle=2, GetComponent=2; unity_score=85
- ksp refs: none; fan_in=0 fan_out=0
- methods(6): Awake, Update, UpdateExplosion, UpdateShaderAnimation, UpdateShaderClip, UpdateTransform
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DistributionData.cs
- 29 lines; kind=class; type=DistributionData; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(1): HasVariableAltitude
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/DockedVesselInfo.cs
- 107 lines; kind=class; type=DockedVesselInfo; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: -
- unity: none; unity_score=3
- ksp refs: ConfigNode=2; fan_in=4 fan_out=3
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/DoNotDestroy.cs
- 9 lines; kind=MonoBehaviour; type=DoNotDestroy; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1, GameObject=1; unity_score=67
- ksp refs: none; fan_in=1 fan_out=0
- methods(1): Awake
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/DoubleCurve.cs
- 279 lines; kind=class; type=DoubleCurve; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=3
- ksp refs: ConfigNode=2; fan_in=6 fan_out=3
- methods(7): Add, Evaluate, FindMinMaxValue, GetInsertionIndex, Load, RecomputeTangents, Save
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/DoubleKeyframe.cs
- 38 lines; kind=struct; type=DoubleKeyframe; bases=IComparable; ancestry=IComparable; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/DoubleSlider.cs
- 196 lines; kind=MonoBehaviour; type=DoubleSlider; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=6, GetComponent=2, MonoBehaviour=1, Lifecycle=1; unity_score=50
- ksp refs: none; fan_in=3 fan_out=0
- methods(8): CalculateFinalValues, OnRectTransformDimensionsChange, OnValueChanged, OnValueChanged_SliderFill, OnValueChanged_SliderMax, OnValueChanged_SliderMin, ResizeFillSection, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DoubleTypeParser.cs
- 20 lines; kind=class; type=DoubleTypeParser; bases=ITypeParser; ancestry=ITypeParser; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=1
- methods(2): Convert, Parse
- auto suggestion: tier=A subsystem=Modding
