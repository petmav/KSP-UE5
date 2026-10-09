# child_1: 615 files, 73313 lines

## Assembly-CSharp/AbundanceRequest.cs
- L=20 kind=struct type=AbundanceRequest bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=6
- methods(0): 
- auto: A / Resources

## Assembly-CSharp/ActionGroupList.cs
- L=146 kind=class type=ActionGroupList bases=- anc=- ns=-
- unity(22): UnityEngine.UI=2, GameObject=1 | ksp: Planetarium=5, Vessel=3, ConfigNode=2, GameSettings=1 | fan_in=1
- methods(6): CooldownTimeRemaining, CopyFrom, Load, Save, SetGroup, ToggleGroup
- auto: A / Vessel

## Assembly-CSharp/ActionGroupsFlightController.cs
- L=161 kind=MonoBehaviour type=ActionGroupsFlightController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(64): GameObject=5, Time.=3, Lifecycle=3, Coroutine=3, UnityEngine.UI=2, SetActive=2 | ksp: GameEvents=10, FlightGlobals=8, InputLockManager=4, ConfigNode=3, Vessel=1 | fan_in=6
- methods(8): Awake, ExitPanelActions, GetActionGroupPanel, PauseTime, SaveActions, SelectPanelActions, Start, Update
- auto: B / Vessel

## Assembly-CSharp/ActiveJoint.cs
- L=658 kind=class type=ActiveJoint bases=IConfigNode anc=IConfigNode ns=-
- unity(38): Joint=12, Transform=9, Coroutine=4, GameObject=3, Rigidbody=2 | ksp: Part=29, GameEvents=16, PartJoint=6, AttachNode=3, Vessel=2 | fan_in=4
- methods(34): Create, DrawDebug, GetCtrlAxis, GetJointDrive, InitJoint, Load, Save, SetAxis, SetDriveMode, SetJointDrive, SetSecondaryAxis, Terminate, applyCoordsUpdate, findAttachJointForTargetPart, findJointAtNode, findJointBetweenParts, getAnchorOffset, getControlOrt, getControlPos, getInvControlOrt, getInvControlPos, onActiveJointNeedUpdate, onJointInit, onPartDestroyed ...
- auto: B / DLC-BreakingGround

## Assembly-CSharp/ActiveJointPivot.cs
- L=84 kind=class type=ActiveJointPivot bases=ActiveJoint anc=ActiveJoint/IConfigNode ns=-
- unity(41): Joint=4 | ksp: AttachNode=1 | fan_in=1
- methods(8): Create, GetControlAxis, GetLocalPivotRotation, SetLocalPivotTgtRotation, SetPivotAngleLimit, applyPivotLimit, onJointInit, setupPivot
- auto: B / DLC-BreakingGround

## Assembly-CSharp/AddonLoader.cs
- L=185 kind=MonoBehaviour type=AddonLoader bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(56): SceneManager=3, MonoBehaviour=3, Transform=2, Lifecycle=2, Instantiate/Destroy=2, GameObject=2 | ksp: HighLogic=1 | fan_in=1
- methods(8): AddonLevelTest, Awake, LoadedOnceContains, OnDestroy, OnLevelLoaded, OnSceneLoaded, StartAddon, StartAddons
- auto: B / Modding

## Assembly-CSharp/AdministrationFacility.cs
- L=58 kind=MonoBehaviour type=AdministrationFacility bases=SpaceCenterBuilding anc=SpaceCenterBuilding/MonoBehaviour ns=-
- unity(21): none | ksp: Localizer=6, HighLogic=5, GameEvents=4, InputLockManager=3 | fan_in=0
- methods(5): IsOpen, OnClicked, OnOnDestroy, OnPopupWarningDismiss, onDialogClose
- auto: B / Career

## Assembly-CSharp/AdvancedRagdoll.cs
- L=205 kind=MonoBehaviour type=AdvancedRagdoll bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(93): Transform=34, AnimationCurve=20, Animation=9, GetComponent=7, Renderer=4, GameObject=4 | ksp: none | fan_in=1
- methods(8): CopyMaterials, CopyTransforms, GetTransformPathToRoot, PoseToAnimation, Start, SynchRagdollIn, SynchRagdollOut, TransformToAnimationCurve
- auto: B / Kerbals

## Assembly-CSharp/AerodynamicsFX.cs
- L=281 kind=MonoBehaviour type=AerodynamicsFX bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(60): Transform=10, Lifecycle=5, Light=3, Texture=1, QualitySettings/SystemInfo=1, MonoBehaviour=1 | ksp: PhysicsGlobals=15, GameEvents=6, MapView=5, Vessel=4, FlightGlobals=4 | fan_in=1
- methods(8): Awake, LateUpdate, OnDestroy, OnPause, OnSettingsUpdate, OnUnpause, Start, Update
- auto: C / Rendering

## Assembly-CSharp/AeroFXState.cs
- L=27 kind=class type=AeroFXState bases=- anc=- ns=-
- unity(12): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / Rendering

## Assembly-CSharp/AlarmActions.cs
- L=70 kind=class type=AlarmActions bases=IConfigNode anc=IConfigNode ns=-
- unity(0): none | ksp: ConfigNode=2 | fan_in=4
- methods(4): Duplicate, Load, Save, ToString
- auto: A / FlightUI

## Assembly-CSharp/AlarmClockMessageDialog.cs
- L=136 kind=MonoBehaviour type=AlarmClockMessageDialog bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(85): GameObject=6, Transform=4, TMPro=4, Lifecycle=4, UnityEngine.UI=3, SetActive=3 | ksp: FlightGlobals=5, Vessel=3, Localizer=2, HighLogic=2, GameEvents=2 | fan_in=2
- methods(10): Awake, CloseDialog, OnAlarmRemoved, OnClose, OnDeleteOnClose, OnDestroy, OnJump, Spawn, Start, Update
- auto: C / FlightUI

## Assembly-CSharp/AlarmClockScenario.cs
- L=626 kind=ScenarioModule type=AlarmClockScenario bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(39): Texture=8, Lifecycle=2, GameObject=2, Instantiate/Destroy=1, GetComponent=1 | ksp: ConfigNode=11, TimeWarp=6, GameEvents=6, Planetarium=5, Vessel=3 | fan_in=14
- methods(24): AddAlarm, AlarmExists, CreateAlarmByMapNodeType, CreateAlarmByType, DeleteAlarm, GetLastAlarm, GetNextAlarm, GetNextOrLastAlarm, GetUniqueAlarmID, HandleWarpActions, LoadTexture, LoadTypes, MapNodeDefined, OnAwake, OnDestroy, OnLoad, OnManeuversLoaded, OnSave, OrderByUT, PlaySound, ShowAlarmMapButton, SortAlarmsByUTDescending, TryGetAlarm, Update
- auto: C / FlightUI

## Assembly-CSharp/AlarmClockScenarioAudio.cs
- L=214 kind=MonoBehaviour type=AlarmClockScenarioAudio bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(61): Audio=11, Lifecycle=3, MonoBehaviour=1, GetComponent=1, GameObject=1 | ksp: GameDatabase=5, GameSettings=1 | fan_in=1
- methods(10): AudioEventArgs, Awake, GetStockAlarmSounds, InitializeSoundController, LoadAudioClip, PlaySound, Start, Stop, Update, isClipPlaying
- auto: C / Audio

## Assembly-CSharp/AlarmClockSettings.cs
- L=26 kind=class type=AlarmClockSettings bases=IConfigNode anc=IConfigNode ns=-
- unity(0): none | ksp: ConfigNode=2 | fan_in=1
- methods(2): Load, Save
- auto: A / FlightUI

## Assembly-CSharp/AlarmClockSettingsUI.cs
- L=107 kind=class type=AlarmClockSettingsUI bases=DialogGUIVerticalLayout,ISettings anc=DialogGUIVerticalLayout/DialogGUILayoutBase/DialogGUIBase ns=-
- unity(0): none | ksp: Localizer=6 | fan_in=1
- methods(6): ApplySettings, DrawMiniSettings, DrawSettings, GetName, GetSettings, OnUpdate
- auto: A / FlightUI

## Assembly-CSharp/AlarmClockUIAlarmRow.cs
- L=144 kind=MonoBehaviour type=AlarmClockUIAlarmRow bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): UnityEngine.UI=4, GameObject=4, Transform=3, Lifecycle=3, TMPro=2, Instantiate/Destroy=2 | ksp: Localizer=5, GameSettings=4, Planetarium=1 | fan_in=1
- methods(10): DeleteAlarm, DeleteAlarmDialogOutput, OnDestroy, OnToggleChanged, SetTextColor, Setup, Start, UIUpdate, Update, UpdateRowUI
- auto: C / FlightUI

## Assembly-CSharp/AlarmClockUINextAlarm.cs
- L=137 kind=MonoBehaviour type=AlarmClockUINextAlarm bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(78): SetActive=6, GameObject=6, UnityEngine.UI=5, Lifecycle=3, TMPro=2, MonoBehaviour=1 | ksp: Localizer=6, GameSettings=3, Planetarium=1, HighLogic=1 | fan_in=0
- methods(7): DeleteAlarm, DeleteAlarmDialogOutput, OnDestroy, Start, UIUpdate, UIUpdate_NoAlarm, Update
- auto: C / FlightUI

## Assembly-CSharp/AlarmMapNodeButton.cs
- L=63 kind=MonoBehaviour type=AlarmMapNodeButton bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(54): Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(5): Awake, OnAddAlarm, OnDestroy, OnPointerEnter, OnPointerExit
- auto: B / MapView

## Assembly-CSharp/AlarmTypeApoapsis.cs
- L=71 kind=class type=AlarmTypeApoapsis bases=AlarmTypeFlightNodeBase anc=AlarmTypeFlightNodeBase/AlarmTypeBase/AppUI_Data ns=-
- unity(5): none | ksp: Localizer=3, Vessel=1 | fan_in=0
- methods(7): CanSetAlarm, CannotSetAlarmText, GetDefaultTitle, GetNodeUT, InitializeFromMapObject, MapNodeType, ShowAlarmMapObject
- auto: A / FlightUI

## Assembly-CSharp/AlarmTypeBase.cs
- L=372 kind=class type=AlarmTypeBase bases=AppUI_Data,ICloneable anc=AppUI_Data/IConfigNode ns=-
- unity(6): SetActive=1, GameObject=1 | ksp: Vessel=8, TimeWarp=4, ConfigNode=4, FlightGlobals=3, GameEvents=2 | fan_in=13
- methods(29): ActionAlarm, CanSetAlarm, CannotSetAlarmText, Clone, CloneAlarm, GetDefaultTitle, InitializeFromMapObject, MapNodeType, OnActioned, OnAlarmLoad, OnAlarmSave, OnInputPanelUpdate, OnLoad, OnManeuversLoaded, OnSave, OnScenarioUpdate, OnTriggered, OnUIEndInitialization, OnUIInitialization, OnUIInputPanelDataChanged, RequiresVessel, ScenarioUpdate, ShowAlarmMapObject, TriggerAlarm ...
- auto: A / FlightUI

## Assembly-CSharp/AlarmTypeFlightNodeBase.cs
- L=78 kind=class type=AlarmTypeFlightNodeBase bases=AlarmTypeBase anc=AlarmTypeBase/AppUI_Data/IConfigNode ns=-
- unity(0): none | ksp: Vessel=3, ConfigNode=2, Orbit=1 | fan_in=3
- methods(9): GetNodeUT, OnAlarmLoad, OnAlarmSave, OnInputPanelUpdate, OnScenarioUpdate, OnUIEndInitialization, OnUIInitialization, RequiresVessel, UpdateAlarmUT
- auto: A / FlightUI

## Assembly-CSharp/AlarmTypeManeuver.cs
- L=332 kind=class type=AlarmTypeManeuver bases=AlarmTypeBase anc=AlarmTypeBase/AppUI_Data/IConfigNode ns=-
- unity(12): SetActive=2, GameObject=2 | ksp: Vessel=4, Localizer=4, Planetarium=3, GameEvents=2, ConfigNode=2 | fan_in=1
- methods(18): BuildManeuverListUI, CanSetAlarm, CannotSetAlarmText, GetDefaultTitle, InitializeFromMapObject, MapNodeType, OnAlarmLoad, OnAlarmSave, OnInputPanelUpdate, OnManeuverNodeChanged, OnManeuversLoaded, OnScenarioUpdate, OnUIEndInitialization, OnUIInitialization, RequiresVessel, ShowAlarmMapObject, UpdateAlarmUT, UpdateManeuverList
- auto: B / FlightUI

## Assembly-CSharp/AlarmTypePeriapsis.cs
- L=71 kind=class type=AlarmTypePeriapsis bases=AlarmTypeFlightNodeBase anc=AlarmTypeFlightNodeBase/AlarmTypeBase/AppUI_Data ns=-
- unity(5): none | ksp: Localizer=3, Vessel=1 | fan_in=0
- methods(7): CanSetAlarm, CannotSetAlarmText, GetDefaultTitle, GetNodeUT, InitializeFromMapObject, MapNodeType, ShowAlarmMapObject
- auto: A / FlightUI

## Assembly-CSharp/AlarmTypeRaw.cs
- L=113 kind=class type=AlarmTypeRaw bases=AlarmTypeBase anc=AlarmTypeBase/AppUI_Data/IConfigNode ns=-
- unity(12): SetActive=1, GameObject=1 | ksp: Planetarium=1, Localizer=1 | fan_in=2
- methods(6): CanSetAlarm, GetDefaultTitle, OnInputPanelUpdate, OnUIEndInitialization, OnUIInitialization, RequiresVessel
- auto: A / FlightUI

## Assembly-CSharp/AlarmTypeSOI.cs
- L=71 kind=class type=AlarmTypeSOI bases=AlarmTypeFlightNodeBase anc=AlarmTypeFlightNodeBase/AlarmTypeBase/AppUI_Data ns=-
- unity(5): none | ksp: Localizer=3, Orbit=2, Vessel=1 | fan_in=0
- methods(7): CanSetAlarm, CannotSetAlarmText, GetDefaultTitle, GetNodeUT, InitializeFromMapObject, MapNodeType, ShowAlarmMapObject
- auto: A / FlightUI

## Assembly-CSharp/AlarmTypeTransferWindow.cs
- L=184 kind=class type=AlarmTypeTransferWindow bases=AlarmTypeBase anc=AlarmTypeBase/AppUI_Data/IConfigNode ns=-
- unity(0): none | ksp: FlightGlobals=8, Planetarium=5, CelestialBody=5, Vessel=4, Localizer=4 | fan_in=0
- methods(14): CanSetAlarm, CannotSetAlarmText, GetDefaultTitle, GetNextTransferWindow, OnAlarmLoad, OnAlarmSave, OnInputPanelUpdate, OnScenarioUpdate, OnUIEndInitialization, OnUIInitialization, OnUIInputPanelDataChanged, RequiresVessel, UpdateDestList, UpdateSourceList
- auto: A / FlightUI

## Assembly-CSharp/AltimeterDisplayState.cs
- L=6 kind=enum type=AltimeterDisplayState bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / FlightUI

## Assembly-CSharp/AltitudeMultiTextureRamp.cs
- L=189 kind=MonoBehaviour type=AltitudeMultiTextureRamp bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(53): Texture=6, Instantiate/Destroy=2, Application.=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(5): CreateTextureARGB, CreateTextureGS, Lerp, Reset, Setup
- auto: C / Rendering

## Assembly-CSharp/AmbienceControl.cs
- L=208 kind=MonoBehaviour type=AmbienceControl bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(53): Audio=5, Lifecycle=3, Transform=2, AnimationCurve=2, MonoBehaviour=1, GameObject=1 | ksp: FlightGlobals=12, GameEvents=9, MapView=4, GameSettings=4, HighLogic=2 | fan_in=0
- methods(9): Awake, OnDestroy, OnLeavingScene, OnSceneLoaded, Update, editorAmbienceGo, editorAmbienceStop, flightAmbienceGo, flightAmbienceStop
- auto: C / Audio

## Assembly-CSharp/AngleRenderEject.cs
- L=359 kind=MonoBehaviour type=AngleRenderEject bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(81): GameObject=17, Renderer=15, Transform=12, IMGUI=7, Camera=4, Material=3 | ksp: Vector3d=36, ScaledSpace=11, MapView=4, CelestialBody=2, Planetarium=1 | fan_in=0
- methods(12): ClampDegrees180, ClampDegrees360, DrawAngle, DrawArc, DrawLine, DrawLineArrow, HideAngle, InitLine, OnDestroy, OnGUI, OnPreCull, Start
- auto: C / MapView

## Assembly-CSharp/AnimationFX.cs
- L=86 kind=MonoBehaviour type=AnimationFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(37): Animation=2 | ksp: ConfigNode=4 | fan_in=0
- methods(5): FindAnimator, OnEvent, OnInitialize, OnLoad, OnSave
- auto: C / Rendering

## Assembly-CSharp/AntennaType.cs
- L=11 kind=enum type=AntennaType bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=5
- methods(0): 
- auto: A / CommNet

## Assembly-CSharp/ApplyScoreTutorialStep.cs
- L=219 kind=class type=ApplyScoreTutorialStep bases=IntermediateTutorialPageStep anc=IntermediateTutorialPageStep/TutorialPageConfig ns=-
- unity(18): GameObject=6 | ksp: GameEvents=4 | fan_in=1
- methods(20): AddTutorialStepConfig, CanHighlightApplyScoreSetup, FilterApplyNodeCriteria, HasApplyScoreNode, IsLinksConnected, OnEnterDragApplyScoreNode, OnEnterDragApplyScoreNodeHelper, OnEnterLinkExplanation, OnEnterSetupApplyScoreNode, OnEnterSetupApplyScoreNodeHelper, OnLeaveDragApplyScoreNode, OnLeaveDragApplyScoreNodeHelper, OnLeaveLinkExplanation, OnLeaveSetupApplyScoreNode, OnLeaveSetupApplyScoreNodeHelper, OnNodeFocused, OnSelectedGameObjectChange, OnUpdateLinkExplanation, OnUpdateSetupApplyScoreNode, OnUpdateSetupApplyScoreNodeHelper
- auto: B / Tutorials

## Assembly-CSharp/AppUIMaster.cs
- L=87 kind=MonoBehaviour type=AppUIMaster bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(76): GameObject=8, MonoBehaviour=3, Lifecycle=3, Instantiate/Destroy=1, GetComponent=1 | ksp: none | fan_in=1
- methods(5): Awake, SetupRowPrefabByType, Start, TryGetAppUIControlPrefab, Update
- auto: C / UI

## Assembly-CSharp/APSkillExtensions.cs
- L=77 kind=static class type=APSkillExtensions bases=- anc=- ns=-
- unity(0): none | ksp: HighLogic=15, Game=5, Vessel=1 | fan_in=0
- methods(2): AvailableAtLevel, GetRequiredSkill
- auto: A / Kerbals

## Assembly-CSharp/ArrayExtensions.cs
- L=22 kind=static class type=ArrayExtensions bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(1): IndexOf
- auto: A / Core

## Assembly-CSharp/ArrowPointer.cs
- L=204 kind=MonoBehaviour type=ArrowPointer bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(75): GameObject=8, Mesh=5, Transform=3, Instantiate/Destroy=3, GetComponent=3, Renderer=2 | ksp: none | fan_in=4
- methods(6): Create, CreateMesh, OnDestroy, Update, UpdateVertexColors, UpdateVerts
- auto: C / FlightUI

## Assembly-CSharp/ArrowPointerSystem.cs
- L=27 kind=MonoBehaviour type=ArrowPointerSystem bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(86): Material=4, Lifecycle=2, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(2): Awake, OnDestroy
- auto: B / FlightUI

## Assembly-CSharp/AssemblyInfo.cs
- L=20 kind=class type=AssemblyInfo bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Misc

## Assembly-CSharp/AssetBase.cs
- L=160 kind=MonoBehaviour type=AssetBase bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(82): GameObject=15, Texture=8, Lifecycle=3, IMGUI=3, Transform=2, MonoBehaviour=2 | ksp: none | fan_in=49
- methods(7): Awake, GetGUISkin, GetPrefab, GetSprite, GetTexture, OnDestroy, Start
- auto: C / Modding

## Assembly-CSharp/AsteroidCollider.cs
- L=3 kind=MonoBehaviour type=AsteroidCollider bases=SpaceObjectCollider anc=SpaceObjectCollider/MonoBehaviour ns=-
- unity(15): none | ksp: none | fan_in=3
- methods(0): 
- auto: B / Celestials

## Assembly-CSharp/AstronautComplexFacility.cs
- L=61 kind=MonoBehaviour type=AstronautComplexFacility bases=SpaceCenterBuilding anc=SpaceCenterBuilding/MonoBehaviour ns=-
- unity(21): none | ksp: HighLogic=9, Localizer=6, GameEvents=6, InputLockManager=5, Game=2 | fan_in=0
- methods(5): IsOpen, OnClicked, OnOnDestroy, OnPopupWarningDismiss, onAstronautComplexDialogClose
- auto: B / Scene

## Assembly-CSharp/AtlasSplitter.cs
- L=24 kind=MonoBehaviour type=AtlasSplitter bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): Texture=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(1): Reset
- auto: C / UI

## Assembly-CSharp/AtmosphereFromGround.cs
- L=268 kind=MonoBehaviour type=AtmosphereFromGround bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(89): Shader=47, Transform=9, Renderer=2, Material=2, Lifecycle=2, MonoBehaviour=1 | ksp: ScaledSpace=3, TimeWarp=1, Planetarium=1, MapView=1, CelestialBody=1 | fan_in=4
- methods(6): LateUpdate, SetKrESunLerp, SetMaterial, Start, UpdateAtmosphere, pow
- auto: C / Rendering

## Assembly-CSharp/AtmosphereProbe.cs
- L=27 kind=MonoBehaviour type=AtmosphereProbe bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(67): Lifecycle=2, Transform=1, MonoBehaviour=1 | ksp: FlightGlobals=6 | fan_in=0
- methods(2): Start, Update
- auto: B / Celestials

## Assembly-CSharp/Attachment.cs
- L=22 kind=class type=Attachment bases=- anc=- ns=-
- unity(6): none | ksp: Part=2, AttachNode=2 | fan_in=2
- methods(0): 
- auto: A / Part

## Assembly-CSharp/AttachModes.cs
- L=5 kind=enum type=AttachModes bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=13
- methods(0): 
- auto: A / Part

## Assembly-CSharp/AttachNode.cs
- L=202 kind=class type=AttachNode bases=- anc=- ns=-
- unity(47): Transform=12, Instantiate/Destroy=2, Renderer=1, Material=1, GetComponent=1, GameObject=1 | ksp: AttachNode=10, Part=4 | fan_in=44
- methods(6): ChangeSrfNodePosition, Clone, DestroyNodeIcon, FindAttachedPart, FindOpposingNode, ReverseSrfNodeDirection
- auto: B / Part

## Assembly-CSharp/AttachNodeMethod.cs
- L=9 kind=enum type=AttachNodeMethod bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=6
- methods(0): 
- auto: A / Part

## Assembly-CSharp/AttachNodeSnapshot.cs
- L=70 kind=class type=AttachNodeSnapshot bases=- anc=- ns=-
- unity(0): none | ksp: Part=2, Vessel=1, ProtoVessel=1, AttachNode=1 | fan_in=1
- methods(2): Save, findAttachedPart
- auto: A / Part

## Assembly-CSharp/AttachRules.cs
- L=78 kind=class type=AttachRules bases=- anc=- ns=-
- unity(5): none | ksp: none | fan_in=3
- methods(2): Parse, String
- auto: A / Part

## Assembly-CSharp/AudioFX.cs
- L=248 kind=MonoBehaviour type=AudioFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(61): Audio=12, Instantiate/Destroy=3, Lifecycle=2, GetComponent=2, GameObject=1 | ksp: GameSettings=5, GameEvents=4, ConfigNode=4, HighLogic=2, GameDatabase=2 | fan_in=6
- methods(13): CreateSource, OnDestroy, OnEvent, OnGamePause, OnGameUnpause, OnInitialize, OnLoad, OnSave, Play, SetSourceVolume, SetTime, Stop, Update
- auto: C / Rendering

## Assembly-CSharp/AudioFXSettings.cs
- L=142 kind=class type=AudioFXSettings bases=DialogGUIVerticalLayout,ISettings anc=DialogGUIVerticalLayout/DialogGUILayoutBase/DialogGUIBase ns=-
- unity(8): Audio=1 | ksp: GameSettings=20, Localizer=10 | fan_in=1
- methods(6): ApplySettings, DrawMiniSettings, DrawSettings, GetName, GetSettings, OnUpdate
- auto: C / Audio

## Assembly-CSharp/AudioLoopFX.cs
- L=131 kind=MonoBehaviour type=AudioLoopFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(59): Audio=5, Lifecycle=2, Instantiate/Destroy=1, GetComponent=1, GameObject=1 | ksp: GameEvents=4, ConfigNode=4, HighLogic=1, GameDatabase=1 | fan_in=0
- methods(11): CreateSource, OnDestroy, OnEvent, OnGamePause, OnGameUnpause, OnInitialize, OnLoad, OnSave, Play, Stop, Update
- auto: C / Audio

## Assembly-CSharp/AudioMultiFX.cs
- L=170 kind=MonoBehaviour type=AudioMultiFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(61): Transform=5, Audio=5, Lifecycle=2, GetComponent=1, GameObject=1 | ksp: GameEvents=4, ConfigNode=4, HighLogic=1, GameDatabase=1 | fan_in=0
- methods(10): CreateSource, OnDestroy, OnEvent, OnGamePause, OnGameUnpause, OnInitialize, OnLoad, OnSave, Play, Update
- auto: C / Audio

## Assembly-CSharp/AudioMultiPooledFX.cs
- L=290 kind=MonoBehaviour type=AudioMultiPooledFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(60): Transform=7, SetActive=4, Audio=4, Lifecycle=3, GameObject=3, Coroutine=2 | ksp: GameEvents=4, ConfigNode=4, HighLogic=2, GameDatabase=1 | fan_in=0
- methods(16): BorrowAndPlay, CheckEnd, Create, OnDestroy, OnDisable, OnEvent, OnGamePause, OnGameUnpause, OnInitialize, OnLoad, OnSave, Play, Reset, SetupAudioClip, StopAndRelease, Update
- auto: C / Audio

## Assembly-CSharp/AudioMuter.cs
- L=31 kind=MonoBehaviour type=AudioMuter bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(69): Lifecycle=2, MonoBehaviour=1, Audio=1 | ksp: GameEvents=4 | fan_in=0
- methods(4): Awake, OnDestroy, OnGameSceneLoadRequested, OnLevelLoaded
- auto: C / Audio

## Assembly-CSharp/AudioSingleFX.cs
- L=176 kind=MonoBehaviour type=AudioSingleFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(71): Audio=12, Instantiate/Destroy=4, Lifecycle=3, GetComponent=2, GameObject=1 | ksp: GameEvents=4, ConfigNode=4, HighLogic=1, GameDatabase=1 | fan_in=0
- methods(12): Awake, CreateSource, OnDestroy, OnEvent, OnGamePause, OnGameUnpause, OnInitialize, OnLoad, OnSave, Play, Stop, Update
- auto: C / Audio

## Assembly-CSharp/AvailablePart.cs
- L=309 kind=class type=AvailablePart bases=- anc=- ns=-
- unity(17): Renderer=3, GetComponent=1, GameObject=1 | ksp: AvailablePart=7, Localizer=5, ConfigNode=3, Part=1 | fan_in=84
- methods(6): APEntryCostGetDelegate, AddFileTime, GetVariant, Init, SetEntryCost, _GetEntryCost
- auto: B / Part

## Assembly-CSharp/AvailableVariantTheme.cs
- L=56 kind=class type=AvailableVariantTheme bases=IConfigNode anc=IConfigNode ns=-
- unity(7): none | ksp: Localizer=3, ConfigNode=2, AvailablePart=2 | fan_in=2
- methods(3): CreateVariantTheme, Load, Save
- auto: A / Part

## Assembly-CSharp/AxisBinding.cs
- L=135 kind=class type=AxisBinding bases=IConfigNode,ICloneable anc=IConfigNode ns=-
- unity(2): none | ksp: ConfigNode=2 | fan_in=7
- methods(5): Clone, GetAxis, IsNeutral, Load, Save
- auto: A / Input

## Assembly-CSharp/AxisBinding_Single.cs
- L=252 kind=class type=AxisBinding_Single bases=InputBinding,IConfigNode,ICloneable anc=InputBinding/IConfigNode ns=-
- unity(7): Input.=1 | ksp: ConfigNode=2, GameSettings=1 | fan_in=3
- methods(4): GetAxis, IsNeutral, Load, Save
- auto: A / Input

## Assembly-CSharp/AxisFieldLimit.cs
- L=10 kind=class type=AxisFieldLimit bases=- anc=- ns=-
- unity(6): none | ksp: none | fan_in=7
- methods(0): 
- auto: A / Input

## Assembly-CSharp/AxisGroupsInputHandler.cs
- L=58 kind=MonoBehaviour type=AxisGroupsInputHandler bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(44): Lifecycle=2, MonoBehaviour=1 | ksp: GameEvents=2, VesselModule=1, Vessel=1 | fan_in=0
- methods(4): Awake, AxisInputHandler, OnDestroy, onVesselChange
- auto: B / Input

## Assembly-CSharp/AxisGroupsManager.cs
- L=61 kind=class type=AxisGroupsManager bases=- anc=- ns=-
- unity(0): none | ksp: ConfigNode=5, PartModule=3 | fan_in=1
- methods(3): BuildBaseAxisFields, LoadAxisFieldNodes, SaveAxisFieldNodes
- auto: A / Input

## Assembly-CSharp/AxisGroupsModule.cs
- L=139 kind=VesselModule type=AxisGroupsModule bases=VesselModule anc=VesselModule/MonoBehaviour ns=-
- unity(20): Lifecycle=1 | ksp: GameEvents=4, PartModule=3, Vessel=2, VesselModule=1, GameSettings=1 | fan_in=1
- methods(12): AddAxis, BuildAxisGroups, GetActivation, IncrementAxisGroup, OnAwake, OnDestroy, OnLoadVessel, OnVesselOverrideGroupChanged, SetAxisGroup, ShouldBeActive, UpdateAxisGroup, onVesselWasModified
- auto: A / Input

## Assembly-CSharp/AxisKeyBinding.cs
- L=48 kind=class type=AxisKeyBinding bases=IConfigNode,ICloneable anc=IConfigNode ns=-
- unity(0): none | ksp: ConfigNode=3 | fan_in=1
- methods(3): Clone, Load, Save
- auto: A / Input

## Assembly-CSharp/AxisKeyBindingList.cs
- L=67 kind=class type=AxisKeyBindingList bases=IConfigNode,ICloneable anc=IConfigNode ns=-
- unity(0): none | ksp: ConfigNode=4 | fan_in=1
- methods(3): Clone, Load, Save
- auto: A / Input

## Assembly-CSharp/AxisTestModule.cs
- L=18 kind=PartModule type=AxisTestModule bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: PartModule=1, KSPField=1 | fan_in=0
- methods(1): OnStart
- auto: A / Input

## Assembly-CSharp/BaseAction.cs
- L=465 kind=class type=BaseAction bases=- anc=- ns=-
- unity(2): none | ksp: Part=12, PartModule=3, GameSettings=2, ConfigNode=2, Vessel=1 | fan_in=15
- methods(14): AddAction, ContainsNonDefaultActions, CopyAction, CreateActionList, CreateGroupList, FireAction, GetActionGroup, GetActionGroups, GetActionGroupsLength, GetGroupIndex, Invoke, OnLoad, OnSave, ParseGroup
- auto: A / Part

## Assembly-CSharp/BaseActionDelegate.cs
- L=1 kind=delegate type=BaseActionDelegate bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Part

## Assembly-CSharp/BaseActionList.cs
- L=166 kind=class type=BaseActionList bases=List<BaseAction> anc=List ns=-
- unity(2): none | ksp: KSPAction=7, ConfigNode=3, PartModule=2, Part=2 | fan_in=3
- methods(6): Add, Contains, GetReflectedAttributes, OnLoad, OnSave, ReflectActions
- auto: A / Part

## Assembly-CSharp/BaseAxisField.cs
- L=437 kind=class type=BaseAxisField bases=BaseField anc=BaseField/BaseField ns=-
- unity(0): none | ksp: Part=9, PartModule=6, Vessel=3, ConfigNode=2, TimeWarp=1 | fan_in=20
- methods(16): AddAxis, ContainsNonDefaultAxes, CopyField, CreateAxisList, CreateGroupList, GetAxisGroup, GetAxisGroups, GetAxisGroupsLength, IncrementAxis, IsInGroup, OnLoad, OnSave, ParseGroup, SetAxis, SetIncrementalSpeedMultiplier, UpdateAxis
- auto: A / Part

## Assembly-CSharp/BaseAxisFieldList.cs
- L=5 kind=class type=BaseAxisFieldList bases=List<BaseAxisField> anc=List ns=-
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / Part

## Assembly-CSharp/BaseConverter.cs
- L=781 kind=PartModule type=BaseConverter bases=PartModule,IAnimatedModule,IOverheatDisplay anc=PartModule/MonoBehaviour ns=-
- unity(21): MonoBehaviour=3, Time.=1, Lifecycle=1 | ksp: KSPField=28, Localizer=8, PartResourceDefinition=5, FloatCurve=4, KSPAction=3 | fan_in=6
- methods(46): CacheLocalStrings, CanBeDetached, CanBeOffset, CanBeRotated, CheckForShutdown, ConvertRecipeToUnits, DisableModule, DisplayCoreHeat, EnableModule, FixedUpdate, GetBestDeltaTime, GetCoreTemperature, GetCrewEfficiencyBonus, GetCrewHeatBonus, GetDeltaTime, GetEfficiencyMultiplier, GetFlux, GetGoalTemperature, GetHeatThrottle, GetModuleDisplayName, HasSpecialist, IsAnimationGroupDeployed, IsOverheating, IsSituationValid ...
- auto: A / PartModules-Resources

## Assembly-CSharp/BaseDrill.cs
- L=18 kind=PartModule type=BaseDrill bases=BaseConverter anc=BaseConverter/PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: KSPField=3, Localizer=1 | fan_in=3
- methods(1): GetModuleDisplayName
- auto: A / PartModules-Resources

## Assembly-CSharp/BaseEvent.cs
- L=266 kind=class type=BaseEvent bases=- anc=- ns=-
- unity(1): none | ksp: KSPEvent=2, ConfigNode=2, Localizer=1 | fan_in=25
- methods(5): Invoke, OnLoad, OnSave, ToString, VariantToggleEventDisabled
- auto: A / Part

## Assembly-CSharp/BaseEventDelegate.cs
- L=1 kind=delegate type=BaseEventDelegate bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Part

## Assembly-CSharp/BaseEventDetails.cs
- L=129 kind=class type=BaseEventDetails bases=- anc=- ns=-
- unity(22): GameObject=3, IMGUI=1 | ksp: none | fan_in=5
- methods(7): Get, GetBool, GetFloat, GetGameObject, GetInt, GetString, Set
- auto: A / Part

## Assembly-CSharp/BaseEventDetailsDelegate.cs
- L=1 kind=delegate type=BaseEventDetailsDelegate bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Part

## Assembly-CSharp/BaseEventList.cs
- L=346 kind=class type=BaseEventList bases=List<BaseEvent> anc=List ns=-
- unity(4): MonoBehaviour=2 | ksp: KSPEvent=10, ConfigNode=3, PartModule=2, Part=2 | fan_in=7
- methods(14): Add, AddBaseEventList, ClearDefault, Contains, CreateDelegates, GetByIndex, GetReflectedAttributes, LoadStagingIcon, OnLoad, OnSave, RemoveBaseEventList, Send, SendDefault, SetDefault
- auto: A / Part

## Assembly-CSharp/BaseFieldList.cs
- L=423 kind=class type=BaseFieldList bases=IEnumerable anc=IEnumerable ns=-
- unity(5): none | ksp: ConfigNode=8, KSPField=2, HighLogic=2 | fan_in=8
- methods(13): Add, AddBaseFieldList, CreateList, GetEnumerator, GetReflectedAttributes, GetValue, Load, ReadValue, RemoveBaseFieldList, Save, SetOriginalValue, SetValue, TryGetFieldUIControl
- auto: A / Part

## Assembly-CSharp/BaseGameEvent.cs
- L=35 kind=class type=BaseGameEvent bases=- anc=- ns=-
- unity(10): none | ksp: none | fan_in=12
- methods(1): FindEvent
- auto: A / Core

## Assembly-CSharp/BaseLabel.cs
- L=80 kind=MonoBehaviour type=BaseLabel bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(91): Transform=7, GameObject=7, SetActive=4, UnityEngine.UI=3, GetComponent=3, Lifecycle=2 | ksp: GameEvents=2 | fan_in=3
- methods(7): Awake, Disable, Enable, OnClick, OnDestroy, ResetParent, Setup
- auto: C / Part

## Assembly-CSharp/BasePAWGroup.cs
- L=22 kind=class type=BasePAWGroup bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=6
- methods(0): 
- auto: A / Part

## Assembly-CSharp/BiDictionaryOneToOne.cs
- L=108 kind=class type=BiDictionaryOneToOne bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(11): Add, Clear, GetByFirst, GetBySecond, RemoveByFirst, RemoveBySecond, TryAdd, TryGetByFirst, TryGetBySecond, TryRemoveByFirst, TryRemoveBySecond
- auto: A / Core

## Assembly-CSharp/BiomeLockData.cs
- L=6 kind=class type=BiomeLockData bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / Celestials

## Assembly-CSharp/BiomeTutorialStep.cs
- L=239 kind=class type=BiomeTutorialStep bases=IntermediateTutorialPageStep anc=IntermediateTutorialPageStep/TutorialPageConfig ns=-
- unity(29): GameObject=12 | ksp: none | fan_in=1
- methods(21): AddTutorialStepConfig, HasBiomeSelected, IsBiomeSettingSetted, IsMunSelected, IsVesselLandedSelected, MunBiomeSelected, OnEnterFindBiomeSetting, OnEnterFindBiomeSettingHelper, OnEnterSelectBiome, OnEnterSelectBiomeHelper, OnFindBiomeSettingHelper, OnLeaveFindBiomeSetting, OnLeaveFindBiomeSettingHelper, OnLeaveSelectBiome, OnLeaveSelectBiomeHelper, OnSelectBiome, OnUpdateFindBiomeSetting, OnUpdateFindBiomeSettingHelper, OnUpdateSelectBiome, OnUpdateSelectHelper, ShowBiomeSetttingSelector
- auto: B / Tutorials

## Assembly-CSharp/BlankMonoBehaviour.cs
- L=12 kind=MonoBehaviour type=BlankMonoBehaviour bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): Lifecycle=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(2): Start, Update
- auto: B / Core

## Assembly-CSharp/Bobber.cs
- L=33 kind=MonoBehaviour type=Bobber bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(78): Time.=3, Transform=2, Lifecycle=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(2): Start, Update
- auto: B / Physics

## Assembly-CSharp/BoxVolume.cs
- L=59 kind=MonoBehaviour type=BoxVolume bases=VolumetricObjectBase anc=VolumetricObjectBase/MonoBehaviour ns=-
- unity(75): Shader=6, Lifecycle=1 | ksp: none | fan_in=0
- methods(5): HasChanged, OnEnable, PopulateShaderName, SetChangedValues, UpdateVolume
- auto: C / Rendering

## Assembly-CSharp/BufferMaterialProperty.cs
- L=17 kind=class type=BufferMaterialProperty bases=MaterialPropertyExtensions.MaterialProperty anc=MaterialProperty ns=-
- unity(29): Material=1 | ksp: none | fan_in=0
- methods(1): Apply
- auto: C / Rendering

## Assembly-CSharp/ButtonHighlighter.cs
- L=12 kind=MonoBehaviour type=ButtonHighlighter bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(63): SetActive=1, MonoBehaviour=1, GameObject=1 | ksp: none | fan_in=2
- methods(1): Enable
- auto: C / UI

## Assembly-CSharp/Callback.cs
- L=5 kind=delegate type=Callback bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: A / Core

## Assembly-CSharp/CallbackUtil.cs
- L=212 kind=static class type=CallbackUtil bases=- anc=- ns=-
- unity(51): Coroutine=23, MonoBehaviour=4, Time.=2 | ksp: none | fan_in=44
- methods(7): Clear, DelayedCallback, DoUntil, HoldUntil, LazyCallback, WaitUntil, lazyCB
- auto: A / Core

## Assembly-CSharp/CameraFade.cs
- L=63 kind=MonoBehaviour type=CameraFade bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(82): IMGUI=5, Time.=2, Texture=2, Screen/Cursor=2, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=1
- methods(4): Awake, OnGUI, SetScreenOverlayColor, StartFade
- auto: C / Camera

## Assembly-CSharp/CameraFX.cs
- L=29 kind=MonoBehaviour type=CameraFX bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(67): Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: none | fan_in=5
- methods(2): Awake, OnDestroy
- auto: C / Camera

## Assembly-CSharp/CameraManager.cs
- L=451 kind=MonoBehaviour type=CameraManager bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(43): GameObject=5, Lifecycle=4, SetActive=3, Camera=3, GetComponent=2, Transform=1 | ksp: FlightCamera=20, FlightGlobals=13, GameSettings=9, GameEvents=7, ProtoCrewMember=5 | fan_in=27
- methods(18): Awake, GetCurrentCamera, GetNextIVA, ICameras_DeactivateAll, ICameras_ResetAll, NextCamera, NextCameraIVA, NextCameraMode, OnDestroy, OnVesselModified, PreviousCameraMode, SetCameraFlight, SetCameraIVA, SetCameraInternal, SetCameraMap, SetCameraMode, Start, Update
- auto: C / Camera

## Assembly-CSharp/CameraMouseLook.cs
- L=73 kind=MonoBehaviour type=CameraMouseLook bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(79): Input.=4, SceneManager=3, Screen/Cursor=2, Lifecycle=2, MonoBehaviour=1, KeyCode=1 | ksp: InputLockManager=2, GameSettings=2, GameEvents=2, HighLogic=1 | fan_in=8
- methods(7): Awake, GetMouseLook, OnDestroy, OnLevelLoaded, OnSceneLoaded, SetMouseLook, onGamePause
- auto: C / Camera

## Assembly-CSharp/CameraOffCenter.cs
- L=52 kind=MonoBehaviour type=CameraOffCenter bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): Screen/Cursor=2, MonoBehaviour=1, Lifecycle=1, Camera=1 | ksp: none | fan_in=1
- methods(2): LateUpdate, PerspectiveOffCenter
- auto: C / Camera

## Assembly-CSharp/CameraOrbitHold.cs
- L=5 kind=MonoBehaviour type=CameraOrbitHold bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(36): MonoBehaviour=1 | ksp: none | fan_in=0
- methods(0): 
- auto: C / Camera

## Assembly-CSharp/CanvasPixelPerfectHandler.cs
- L=43 kind=MonoBehaviour type=CanvasPixelPerfectHandler bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(75): SendMessage/Invoke=3, UnityEngine.UI=2, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=2
- methods(4): Disable, Enable, OnDestroy, TemporaryDisable
- auto: C / UI

## Assembly-CSharp/CanvasUtil.cs
- L=31 kind=static class type=CanvasUtil bases=- anc=- ns=-
- unity(70): UnityEngine.UI=4, Screen/Cursor=3 | ksp: none | fan_in=2
- methods(4): AnchorOffset, Contains, RectContains, ScreenToUISpacePos
- auto: C / UI

## Assembly-CSharp/Capability.cs
- L=216 kind=class type=Capability bases=- anc=- ns=-
- unity(7): Lifecycle=2 | ksp: none | fan_in=0
- methods(3): GetCapabilities, ToString, Update
- auto: A / Part

## Assembly-CSharp/CBAttributeMap.cs
- L=67 kind=class type=CBAttributeMap bases=- anc=- ns=-
- unity(26): Texture=1 | ksp: none | fan_in=0
- methods(1): GetAtt
- auto: A / Celestials

## Assembly-CSharp/CBAttributeMapSO.cs
- L=159 kind=ScriptableObject type=CBAttributeMapSO bases=MapSO anc=MapSO/ScriptableObject ns=-
- unity(15): none | ksp: Localizer=1 | fan_in=11
- methods(4): FormatLocalizationTags, GetAtt, GetPixelColor, RGBColorSqrMag
- auto: A / Celestials

## Assembly-CSharp/CBTextureAtlasSO.cs
- L=82 kind=ScriptableObject type=CBTextureAtlasSO bases=MapSO anc=MapSO/ScriptableObject ns=-
- unity(18): none | ksp: none | fan_in=1
- methods(2): GetCBTextureAtlasPoint, GetPixelCBTextureAtlasPoint
- auto: A / Celestials

## Assembly-CSharp/CelestialBodyLog.cs
- L=60 kind=class type=CelestialBodyLog bases=IConfigNode anc=IConfigNode ns=-
- unity(0): none | ksp: ConfigNode=2, CelestialBody=2 | fan_in=0
- methods(7): AddNewBody, At, GetBodies, HasBody, Load, MergeWith, Save
- auto: A / Celestials

## Assembly-CSharp/CelestialBodyScienceParams.cs
- L=23 kind=class type=CelestialBodyScienceParams bases=- anc=- ns=-
- unity(12): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Celestials

## Assembly-CSharp/CelestialBodyType.cs
- L=4 kind=enum type=CelestialBodyType bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Celestials

## Assembly-CSharp/CenterOfLiftQuery.cs
- L=32 kind=class type=CenterOfLiftQuery bases=- anc=- ns=-
- unity(9): none | ksp: none | fan_in=3
- methods(1): Reset
- auto: A / Aero

## Assembly-CSharp/CenterOfThrustQuery.cs
- L=17 kind=class type=CenterOfThrustQuery bases=- anc=- ns=-
- unity(6): none | ksp: none | fan_in=5
- methods(1): Reset
- auto: A / Aero

## Assembly-CSharp/CharacterAnimationState.cs
- L=37 kind=class type=CharacterAnimationState bases=- anc=- ns=-
- unity(65): Animation=3, Transform=2, Audio=1 | ksp: none | fan_in=5
- methods(1): string
- auto: B / Kerbals

## Assembly-CSharp/CharacterAnimationUtil.cs
- L=57 kind=class type=CharacterAnimationUtil bases=- anc=- ns=-
- unity(73): Animation=5, Transform=4, GetComponent=4, LayerMask=2, MonoBehaviour=1 | ksp: none | fan_in=2
- methods(2): GetAnimationsListFromScript, SetupAnimations
- auto: B / Kerbals

## Assembly-CSharp/ChatController.cs
- L=33 kind=MonoBehaviour type=ChatController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(84): TMPro=3, Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(3): AddToChatOutput, OnDisable, OnEnable
- auto: C / UI

## Assembly-CSharp/CheatOptions.cs
- L=28 kind=class type=CheatOptions bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=35
- methods(0): 
- auto: C / Debug

## Assembly-CSharp/ClearToSaveStatus.cs
- L=11 kind=enum type=ClearToSaveStatus bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=11
- methods(0): 
- auto: A / Save

## Assembly-CSharp/ColliderMenuMouseChecker.cs
- L=36 kind=MonoBehaviour type=ColliderMenuMouseChecker bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(33): MonoBehaviour=1 | ksp: none | fan_in=0
- methods(4): MouseHasExited, MouseIsHovering, OnMouseEnter, OnMouseExit
- auto: C / UI

## Assembly-CSharp/CollisionAnimationHandler.cs
- L=35 kind=MonoBehaviour type=CollisionAnimationHandler bases=MonoBehaviour,ICollisionEvents anc=MonoBehaviour ns=-
- unity(95): Collision/Force=6, MonoBehaviour=2, Animation=2, Lifecycle=1, GetComponent=1, GameObject=1 | ksp: none | fan_in=0
- methods(5): GetInstance, OnCollisionEnter, OnCollisionExit, OnCollisionStay, Start
- auto: B / Physics

## Assembly-CSharp/CollisionEnhancer.cs
- L=185 kind=MonoBehaviour type=CollisionEnhancer bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(88): Rigidbody=21, Transform=17, GetComponent=3, Raycast/Query=2, Lifecycle=2, Time.=1 | ksp: Krakensbane=11, GameEvents=4, Part=2, FlightGlobals=2, Localizer=1 | fan_in=8
- methods(2): FixedUpdate, Start
- auto: B / Physics

## Assembly-CSharp/CollisionEnhancerBehaviour.cs
- L=8 kind=enum type=CollisionEnhancerBehaviour bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Physics

## Assembly-CSharp/CollisionEventsHandler.cs
- L=70 kind=MonoBehaviour type=CollisionEventsHandler bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(86): Collision/Force=6, MonoBehaviour=4, Transform=3, GameObject=3, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=1
- methods(6): FindHandleUpwards, OnCollisionEnter, OnCollisionExit, OnCollisionStay, SetHandle, Start
- auto: B / Physics

## Assembly-CSharp/CollisionManager.cs
- L=201 kind=MonoBehaviour type=CollisionManager bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(73): Collider=14, Lifecycle=4, SetActive=2, Physics.=2, GetComponent=2, GameObject=2 | ksp: FlightGlobals=5, Vessel=3, GameEvents=2, Part=1 | fan_in=5
- methods(11): Awake, FixedUpdate, GetAllVesselColliders, IgnoreCollidersOnVessel, OnCollisionIgnoreUpdate, OnDestroy, SetCollidersOnVessel, Start, UpdateAllColliders, UpdateCollisionIgnores, UpdatePartCollisionIgnores
- auto: B / Physics

## Assembly-CSharp/ColorChangedEvent.cs
- L=8 kind=class type=ColorChangedEvent bases=UnityEvent<Color> anc=UnityEvent ns=-
- unity(14): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/ColorHSV.cs
- L=132 kind=class type=ColorHSV bases=- anc=- ns=-
- unity(10): none | ksp: none | fan_in=5
- methods(2): FromColor, ToColor
- auto: C / UI

## Assembly-CSharp/ColorImage.cs
- L=28 kind=MonoBehaviour type=ColorImage bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(85): UnityEngine.UI=3, Lifecycle=2, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=0
- methods(3): Awake, ColorChanged, OnDestroy
- auto: C / Rendering

## Assembly-CSharp/ColorLabel.cs
- L=79 kind=MonoBehaviour type=ColorLabel bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(62): Lifecycle=3, TMPro=2, MonoBehaviour=1, GetComponent=1, Application.=1 | ksp: none | fan_in=0
- methods(7): Awake, ColorChanged, ConvertToDisplayString, HSVChanged, OnDestroy, OnEnable, UpdateValue
- auto: C / UI

## Assembly-CSharp/ColorMaterialProperty.cs
- L=17 kind=class type=ColorMaterialProperty bases=MaterialPropertyExtensions.MaterialProperty anc=MaterialProperty ns=-
- unity(49): Shader=1, Material=1 | ksp: none | fan_in=1
- methods(1): Apply
- auto: C / Rendering

## Assembly-CSharp/ColorPicker.cs
- L=302 kind=MonoBehaviour type=ColorPicker bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(48): SetActive=5, Lifecycle=3, GameObject=3, UnityEngine.UI=1, Shader=1, MonoBehaviour=1 | ksp: none | fan_in=11
- methods(11): AssignColor, GetValue, HSVChanged, OnDestroy, OnEnable, RGBChanged, SendChangedEvent, SendInternalEvent, SetColor, Start, ToggleModeSliders
- auto: C / UI

## Assembly-CSharp/ColorPickerTester.cs
- L=25 kind=MonoBehaviour type=ColorPickerTester bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(86): Material=3, Lifecycle=2, Renderer=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(2): Start, Update
- auto: C / Debug

## Assembly-CSharp/ColorPresets.cs
- L=52 kind=MonoBehaviour type=ColorPresets bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(77): UnityEngine.UI=3, SetActive=2, MonoBehaviour=1, Lifecycle=1, GetComponent=1, GameObject=1 | ksp: none | fan_in=1
- methods(4): Awake, ColorChanged, CreatePresetButton, OnPresetSelected
- auto: C / UI

## Assembly-CSharp/ColorSlider.cs
- L=86 kind=MonoBehaviour type=ColorSlider bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(60): UnityEngine.UI=3, Lifecycle=2, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=0
- methods(5): ColorChanged, HSVChanged, OnDestroy, SliderChanged, Start
- auto: C / UI

## Assembly-CSharp/ColorSliderImage.cs
- L=199 kind=MonoBehaviour type=ColorSliderImage bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(74): UnityEngine.UI=14, Lifecycle=4, Texture=3, Instantiate/Destroy=2, Transform=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(7): Awake, ColorChanged, HSVChanged, OnDestroy, OnDisable, OnEnable, RegenerateTexture
- auto: C / UI

## Assembly-CSharp/ColorValues.cs
- L=10 kind=enum type=ColorValues bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=5
- methods(0): 
- auto: C / UI

## Assembly-CSharp/ComboButton.cs
- L=62 kind=MonoBehaviour type=ComboButton bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(93): UnityEngine.UI=6, EventSystems=6, SetActive=2, Lifecycle=2, GameObject=2, MonoBehaviour=1 | ksp: GameEvents=1 | fan_in=2
- methods(5): OnDestroy, OnSuitComboSelection, Reset, Select, Start
- auto: C / UI

## Assembly-CSharp/ComboSelector.cs
- L=62 kind=MonoBehaviour type=ComboSelector bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(81): UnityEngine.UI=4, Material=3, Lifecycle=2, MonoBehaviour=1 | ksp: GameEvents=1 | fan_in=4
- methods(5): Awake, OnButtonNext, OnButtonPrev, OnDestroy, SelectVariant
- auto: C / UI

## Assembly-CSharp/CometDefinition.cs
- L=46 kind=class type=CometDefinition bases=- anc=- ns=-
- unity(0): none | ksp: ConfigNode=3 | fan_in=5
- methods(1): CreateVesselNode
- auto: A / Celestials

## Assembly-CSharp/CometOrbitType.cs
- L=211 kind=class type=CometOrbitType bases=- anc=- ns=-
- unity(32): none | ksp: Orbit=8, FlightGlobals=3, Planetarium=2, ConfigNode=1, CelestialBody=1 | fan_in=6
- methods(5): CalculateHomeOrbit, CalculateOrbit, CalculateSentinelOrbit, GetRandomObjClass, Load
- auto: A / Orbit

## Assembly-CSharp/CometParticleController.cs
- L=170 kind=MonoBehaviour type=CometParticleController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(75): ParticleSystem=12, Lifecycle=3, Transform=1, MonoBehaviour=1 | ksp: TimeWarp=3, FloatCurve=3, GameEvents=2, Part=1 | fan_in=1
- methods(4): OnDestroy, OnPartWillDie, Start, Update
- auto: C / Rendering

## Assembly-CSharp/CometVessel.cs
- L=475 kind=VesselModule type=CometVessel bases=VesselModule anc=VesselModule/MonoBehaviour ns=-
- unity(50): GameObject=7, Coroutine=6, SetActive=4, GetComponent=4, Instantiate/Destroy=3, Lifecycle=2 | ksp: KSPField=18, GameEvents=15, FlightGlobals=8, Vessel=7, Planetarium=7 | fan_in=9
- methods(19): MoveCometVesselModule, OnAwake, OnDestroy, OnKnowledgeChange, OnPartDecouple, OnPartPersistentIdChanged, OnStart, OnVesselPersistentIdChanged, ReprimeParticles, SetCollider, SetComa, SetDust, SetDustTail, SetGeyser, SetIonTail, SetSituationRanges, Update, UpdateVFXDimensions, WaitAndUpdateVFXDimensions
- auto: B / Celestials

## Assembly-CSharp/CometVesselExtension.cs
- L=36 kind=static class type=CometVesselExtension bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(1): GetCopyOf
- auto: A / Celestials

## Assembly-CSharp/CometVFXController.cs
- L=319 kind=MonoBehaviour type=CometVFXController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(80): Transform=20, GameObject=11, GetComponent=8, Instantiate/Destroy=7, ParticleSystem=6, Lifecycle=3 | ksp: TimeWarp=7, GameEvents=6, Vector3d=5, Vessel=4, Part=3 | fan_in=2
- methods(13): DeSpawnVFX, DefineComaFX, DefineDustTailParticleFX, DefineIONTailParticleFX, Initialize, LateUpdate, OnCometVesselChanged, OnDestroy, OnPartWillDie, OnVesselWasLoaded, SpawnVFXObjects, Start, UpdateComaFX
- auto: C / Rendering

## Assembly-CSharp/ConeShadowTestRotate.cs
- L=11 kind=MonoBehaviour type=ConeShadowTestRotate bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(63): Transform=1, Time.=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(1): Update
- auto: C / Debug

## Assembly-CSharp/ConeVolume.cs
- L=80 kind=MonoBehaviour type=ConeVolume bases=VolumetricObjectBase anc=VolumetricObjectBase/MonoBehaviour ns=-
- unity(64): Shader=5, Lifecycle=1 | ksp: none | fan_in=0
- methods(5): HasChanged, OnEnable, PopulateShaderName, SetChangedValues, UpdateVolume
- auto: C / Rendering

## Assembly-CSharp/ConeVolumeOcclusion.cs
- L=158 kind=MonoBehaviour type=ConeVolumeOcclusion bases=VolumetricObjectBase anc=VolumetricObjectBase/MonoBehaviour ns=-
- unity(82): Transform=7, Shader=6, GameObject=6, Camera=4, Texture=3, GetComponent=3 | ksp: none | fan_in=0
- methods(9): CleanUp, ConeRenderTexture, CreateDepthCamera, HasChanged, OnEnable, PopulateShaderName, SetChangedValues, SetupCamera, UpdateVolume
- auto: C / Rendering

## Assembly-CSharp/ConfigurableJointExtensions.cs
- L=49 kind=static class type=ConfigurableJointExtensions bases=- anc=- ns=-
- unity(62): Joint=4 | ksp: none | fan_in=0
- methods(4): GetExternalToJointSpaceRotation, SetTargetRotation, SetTargetRotationInternal, SetTargetRotationLocal
- auto: B / Physics

## Assembly-CSharp/ConfirmDialog.cs
- L=33 kind=class type=ConfirmDialog bases=- anc=- ns=-
- unity(40): Screen/Cursor=2, IMGUI=1 | ksp: none | fan_in=2
- methods(0): 
- auto: C / UI

## Assembly-CSharp/ConsoleCommands.cs
- L=565 kind=MonoBehaviour type=ConsoleCommands bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(50): MonoBehaviour=1, Lifecycle=1 | ksp: FlightGlobals=29, HighLogic=12, Vessel=7, Reputation=7, Vector3d=6 | fan_in=0
- methods(19): AddStockCommands, OnBadassCommand, OnCommentCommand, OnFundsCommand, OnHelpCommand, OnInfoCommand, OnKSCCommand, OnRendezvousCommand, OnReputationCommand, OnScienceCommand, OnSettingsLoadCommand, OnSettingsSaveCommand, OnStackTraceCommand, OnSwitchCommand, OnTargetvCommand, OnTimeCommand, OnVesselsCommand, OnWaypointCommand, Start
- auto: C / Debug

## Assembly-CSharp/ConstructionEventType.cs
- L=22 kind=enum type=ConstructionEventType bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=20
- methods(0): 
- auto: A / Editor

## Assembly-CSharp/ConstructionMode.cs
- L=7 kind=enum type=ConstructionMode bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=7
- methods(0): 
- auto: A / Editor

## Assembly-CSharp/ConstructionVesselOverlays.cs
- L=70 kind=MonoBehaviour type=ConstructionVesselOverlays bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(67): UnityEngine.UI=3, Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: none | fan_in=0
- methods(6): Awake, OnDisable, Start, ToggleCoL, ToggleCoM, ToggleCoT
- auto: C / FlightUI

## Assembly-CSharp/ContractOrbitRenderer.cs
- L=65 kind=MonoBehaviour type=ContractOrbitRenderer bases=OrbitTargetRenderer anc=OrbitTargetRenderer/OrbitRenderer/OrbitRendererBase ns=-
- unity(15): none | ksp: Localizer=8, Contract=3, Orbit=1, FlightGlobals=1 | fan_in=3
- methods(6): Setup, UpdateLocals, apNode_OnUpdateCaption, ascNode_OnUpdateCaption, descNode_OnUpdateCaption, peNode_OnUpdateCaption
- auto: B / MapView

## Assembly-CSharp/ControlPoint.cs
- L=38 kind=class type=ControlPoint bases=- anc=- ns=-
- unity(65): Transform=6 | ksp: ConfigNode=1 | fan_in=5
- methods(1): Load
- auto: B / Misc

## Assembly-CSharp/ControlTypes.cs
- L=95 kind=enum type=ControlTypes bases=ulong anc=- ns=-
- unity(11): IMGUI=1 | ksp: none | fan_in=151
- methods(0): 
- auto: A / Input

## Assembly-CSharp/ConversionRecipe.cs
- L=58 kind=class type=ConversionRecipe bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=7
- methods(4): Clear, SetInputs, SetOutputs, SetRequirements
- auto: A / PartModules-Resources

## Assembly-CSharp/ConverterResults.cs
- L=6 kind=struct type=ConverterResults bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=6
- methods(0): 
- auto: A / PartModules-Resources

## Assembly-CSharp/CoroutineHost.cs
- L=39 kind=MonoBehaviour type=CoroutineHost bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(89): Coroutine=6, Instantiate/Destroy=3, GameObject=3, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=2
- methods(4): Create, RunAndDispose, StartCoroutine, Terminate
- auto: B / Core

## Assembly-CSharp/CostWidget.cs
- L=121 kind=MonoBehaviour type=CostWidget bases=CurrencyWidget anc=CurrencyWidget/MonoBehaviour ns=-
- unity(47): Transform=3, Lifecycle=2, TMPro=1 | ksp: GameEvents=10, EditorLogic=7, HighLogic=4, Funding=4, ShipConstruct=1 | fan_in=0
- methods(10): Awake, DelayedStart, OnAboutToStart, OnCrewModified, OnDestroy, onCostChange, onCraftFileSelected, onCrewDialogChange, onShipModified, onShipReset
- auto: B / Career

## Assembly-CSharp/CraftThumbnail.cs
- L=288 kind=static class type=CraftThumbnail bases=- anc=- ns=-
- unity(67): GameObject=13, Transform=9, Texture=8, Light=6, Shader=5, Camera=5 | ksp: ShipConstruct=4, HighLogic=3, Part=2, AvailablePart=2, ProtoPartSnapshot=1 | fan_in=3
- methods(7): CreateRenderTextures, GetPartIconTexturePath, GetThumbNailSetupIface, TakePartSnaphot, TakePartSnapshot, TakeSnaphot, TakeStockSnaphot
- auto: C / Rendering

## Assembly-CSharp/CrashObjectName.cs
- L=11 kind=MonoBehaviour type=CrashObjectName bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(44): MonoBehaviour=1 | ksp: none | fan_in=5
- methods(0): 
- auto: B / Physics

## Assembly-CSharp/CreditsMultiTextHeader.cs
- L=78 kind=MonoBehaviour type=CreditsMultiTextHeader bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(75): Transform=6, Coroutine=5, Time.=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=1
- methods(4): NextText, SetText, Start, shiftText
- auto: C / UI

## Assembly-CSharp/CreditsMusic.cs
- L=9 kind=MonoBehaviour type=CreditsMusic bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(70): MonoBehaviour=1, Lifecycle=1, GetComponent=1, Audio=1 | ksp: GameSettings=1 | fan_in=1
- methods(1): Awake
- auto: C / Audio

## Assembly-CSharp/CreditsSceneMain.cs
- L=17 kind=MonoBehaviour type=CreditsSceneMain bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): MonoBehaviour=1, Lifecycle=1, KeyCode=1, Input.=1 | ksp: HighLogic=1 | fan_in=0
- methods(2): Update, returnToMainMenu
- auto: B / Scene

## Assembly-CSharp/CrewGenerator.cs
- L=150 kind=class type=CrewGenerator bases=- anc=- ns=-
- unity(0): none | ksp: ProtoCrewMember=17, Localizer=4, KerbalRoster=3, HighLogic=3, ConfigNode=2 | fan_in=9
- methods(8): GetFileName, GetFullName, GetLastName, GetRandomName, Initialize, LoadNameArray, RandomCrewMemberPrototype, RemoveLastName
- auto: A / Kerbals

## Assembly-CSharp/CrewHatchController.cs
- L=249 kind=MonoBehaviour type=CrewHatchController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(71): Transform=8, GameObject=7, SetActive=4, Lifecycle=3, UnityEngine.UI=2, Raycast/Query=2 | ksp: GameEvents=10, Part=8, FlightGlobals=5, Vessel=3, ProtoCrewMember=3 | fan_in=4
- methods(19): Awake, DespawnUIs, DisableInterface, DismissDialog, EnableInterface, GetMouseUiPos, HideTooltip, LateUpdate, OnCrewDialogDismiss, OnCrewTransferDone, OnDestroy, OnEVABtn, OnTransferBtn, OnVesselSwitch, ShowHatchTooltip, ShowTooltip, SpawnCrewDialog, TooltipUpdate, onVesselSitChange
- auto: B / Kerbals

## Assembly-CSharp/CrewTransfer.cs
- L=115 kind=MonoBehaviour type=CrewTransfer bases=PartItemTransfer anc=PartItemTransfer/MonoBehaviour ns=-
- unity(37): GetComponent=2, GameObject=1, Coroutine=1 | ksp: Part=13, GameEvents=10, ProtoCrewMember=6, Localizer=2, FlightGlobals=2 | fan_in=2
- methods(11): AfterPartsFound, Create, HookAdditionalEvents, IsSemiValidPart, IsValidPart, MoveCrewTo, OnPartSelect, UnhookAdditionalEvents, onChangeInactive, onGoForEva, waitAndCompleteTransfer
- auto: B / Kerbals

## Assembly-CSharp/Currency.cs
- L=11 kind=enum type=Currency bases=- anc=- ns=-
- unity(0): none | ksp: Reputation=1 | fan_in=20
- methods(0): 
- auto: A / Career

## Assembly-CSharp/CurrencyModifierQuery.cs
- L=281 kind=class type=CurrencyModifierQuery bases=- anc=- ns=-
- unity(1): none | ksp: Reputation=11, ResearchAndDevelopment=2, Funding=2, GameEvents=1 | fan_in=21
- methods(9): AddDelta, CanAfford, GetCostLine, GetEffectDelta, GetEffectDeltaText, GetEffectPercentageText, GetInput, GetTotal, RunQuery
- auto: A / Career

## Assembly-CSharp/CurrencyWidget.cs
- L=28 kind=MonoBehaviour type=CurrencyWidget bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): Coroutine=2, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=5
- methods(3): DelayedStart, OnAboutToStart, Start
- auto: B / Career

## Assembly-CSharp/CutsceneCamera.cs
- L=183 kind=MonoBehaviour type=CutsceneCamera bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(62): Transform=13, Time.=2, Lifecycle=2, MonoBehaviour=1, GameObject=1 | ksp: none | fan_in=0
- methods(9): CreateKeyFrame, EvalEvents, EvalPos, EvalRot, GoToFrame, SortKeyFrames, Start, Update, kfCompare
- auto: C / Camera

## Assembly-CSharp/DatabaseGameObject.cs
- L=40 kind=MonoBehaviour type=DatabaseGameObject bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): Instantiate/Destroy=2, GameObject=2, MonoBehaviour=1, Lifecycle=1 | ksp: GameDatabase=1, ConfigNode=1 | fan_in=1
- methods(2): Awake, LoadFireworkFXDefinitions
- auto: B / Modding

## Assembly-CSharp/DatabaseLoader.cs
- L=46 kind=class type=DatabaseLoader bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=10
- methods(2): CleanUp, Load
- auto: A / Modding

## Assembly-CSharp/DatabaseLoaderAttrib.cs
- L=12 kind=attribute type=DatabaseLoaderAttrib bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=10
- methods(0): 
- auto: A / Modding

## Assembly-CSharp/DatabaseLoaderAudio.cs
- L=29 kind=class type=DatabaseLoaderAudio bases=DatabaseLoader<AudioClip> anc=DatabaseLoader ns=-
- unity(59): Coroutine=2, WWW/WebRequest=1, Audio=1 | ksp: none | fan_in=0
- methods(1): Load
- auto: A / Modding

## Assembly-CSharp/DatabaseLoaderModel_MU.cs
- L=24 kind=class type=DatabaseLoaderModel_MU bases=DatabaseLoader<GameObject> anc=DatabaseLoader ns=-
- unity(59): GameObject=5 | ksp: none | fan_in=0
- methods(1): Load
- auto: C / Rendering

## Assembly-CSharp/DatabaseLoaderTexture_DDS.cs
- L=75 kind=class type=DatabaseLoaderTexture_DDS bases=DatabaseLoader<GameDatabase.TextureInfo> anc=DatabaseLoader ns=-
- unity(59): Texture=7 | ksp: GameDatabase=3 | fan_in=0
- methods(2): Load, Read
- auto: C / Rendering

## Assembly-CSharp/DatabaseLoaderTexture_JPEG.cs
- L=46 kind=class type=DatabaseLoaderTexture_JPEG bases=DatabaseLoader<GameDatabase.TextureInfo> anc=DatabaseLoader ns=-
- unity(65): WWW/WebRequest=3, Texture=2, Coroutine=2 | ksp: GameDatabase=6 | fan_in=0
- methods(1): Load
- auto: C / Rendering

## Assembly-CSharp/DatabaseLoaderTexture_MBM.cs
- L=31 kind=class type=DatabaseLoaderTexture_MBM bases=DatabaseLoader<GameDatabase.TextureInfo> anc=DatabaseLoader ns=-
- unity(34): Texture=1 | ksp: GameDatabase=3 | fan_in=0
- methods(2): CleanUp, Load
- auto: C / Rendering

## Assembly-CSharp/DatabaseLoaderTexture_PNG.cs
- L=70 kind=class type=DatabaseLoaderTexture_PNG bases=DatabaseLoader<GameDatabase.TextureInfo> anc=DatabaseLoader ns=-
- unity(67): Texture=7, WWW/WebRequest=3, Coroutine=2 | ksp: GameDatabase=6 | fan_in=0
- methods(1): Load
- auto: C / Rendering

## Assembly-CSharp/DatabaseLoaderTexture_TGA.cs
- L=41 kind=class type=DatabaseLoaderTexture_TGA bases=DatabaseLoader<GameDatabase.TextureInfo> anc=DatabaseLoader ns=-
- unity(28): Texture=1 | ksp: GameDatabase=5 | fan_in=0
- methods(2): CleanUp, Load
- auto: C / Rendering

## Assembly-CSharp/DatabaseLoaderTexture_TRUECOLOR.cs
- L=33 kind=class type=DatabaseLoaderTexture_TRUECOLOR bases=DatabaseLoader<GameDatabase.TextureInfo> anc=DatabaseLoader ns=-
- unity(55): Texture=3 | ksp: GameDatabase=6 | fan_in=0
- methods(1): Load
- auto: C / Rendering

## Assembly-CSharp/DayNightGameObjectSwitch.cs
- L=137 kind=MonoBehaviour type=DayNightGameObjectSwitch bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(69): Coroutine=6, Lifecycle=5, GameObject=3, Instantiate/Destroy=2, Transform=1, SetActive=1 | ksp: HighLogic=3, GameEvents=2, CelestialBody=2 | fan_in=0
- methods(9): OnDestroy, OnDisable, OnEnable, Setup, Start, Update, UpdateObjects, onLevelWasLoaded, setObjects
- auto: C / Rendering

## Assembly-CSharp/Debug_FloatNaNs.cs
- L=22 kind=MonoBehaviour type=Debug_FloatNaNs bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(50): MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(3): Start, _clearfp, _control87
- auto: B / Physics

## Assembly-CSharp/DebugDrawUtil.cs
- L=17 kind=class type=DebugDrawUtil bases=- anc=- ns=-
- unity(47): none | ksp: none | fan_in=8
- methods(2): DrawCrosshairs, DrawDirection
- auto: C / Debug

## Assembly-CSharp/DefaultIcons.cs
- L=26 kind=enum type=DefaultIcons bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=7
- methods(0): 
- auto: C / UI

## Assembly-CSharp/DelegateExtensions.cs
- L=29 kind=static class type=DelegateExtensions bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(1): Clear
- auto: A / Core

## Assembly-CSharp/DeltaVApp.cs
- L=230 kind=MonoBehaviour type=DeltaVApp bases=UIApp anc=UIApp/MonoBehaviour ns=-
- unity(63): Transform=8, Instantiate/Destroy=4, GameObject=4, UnityEngine.UI=2, SetActive=2, Analytics=1 | ksp: HighLogic=6, GameEvents=5, GameSettings=3, Localizer=1 | fan_in=7
- methods(11): AnyTextFieldHasFocus, DisplayApp, GetAppScenes, GetAppScreenPos, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnSceneSwitch, Reset, TextFieldHasFocus
- auto: B / FlightUI

## Assembly-CSharp/DeltaVAppSituation.cs
- L=350 kind=MonoBehaviour type=DeltaVAppSituation bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(58): UnityEngine.UI=11, TMPro=4, Lifecycle=3, GetComponent=2, MonoBehaviour=1 | ksp: FlightGlobals=8, EditorLogic=4, Planetarium=2, CelestialBody=2, Localizer=1 | fan_in=2
- methods(18): AltitudeHasFocus, AltitudeSliderChanged, AltitudeTextChanged, Awake, BodyChanged, CacheLocalStrings, OnDestroy, RefreshBodyList, SetAtmosphereSituation, SliderEndDrag, SliderPointerUp, SliderStartDrag, Start, ToggleAltitudeOn, ToggleSeaLevelOn, ToggleVacuumOn, UpdatePressureDisplay, UpdateVesselDeltaVValues
- auto: C / FlightUI

## Assembly-CSharp/DeltaVAppStageInfo.cs
- L=77 kind=MonoBehaviour type=DeltaVAppStageInfo bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(81): UnityEngine.UI=5, Transform=3, Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=1
- methods(7): Awake, FillInfoToggles, HideAllClicked, OnDestroy, SetColumnLayout, ShowAllClicked, Start
- auto: C / FlightUI

## Assembly-CSharp/DeltaVAppStageInfoToggle.cs
- L=46 kind=MonoBehaviour type=DeltaVAppStageInfoToggle bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(81): Transform=3, Lifecycle=3, UnityEngine.UI=1, TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(5): OnDestroy, Setup, Start, ToggleCheckbox, Update
- auto: B / FlightUI

## Assembly-CSharp/DeltaVAppValues.cs
- L=127 kind=class type=DeltaVAppValues bases=- anc=- ns=-
- unity(0): none | ksp: GameSettings=2, GameEvents=1, FlightGlobals=1, CelestialBody=1 | fan_in=11
- methods(4): GetValue, InitEnabledValue, LoadEnabledInfoLines, UpdateEnabledInfoItems
- auto: A / FlightUI

## Assembly-CSharp/DeltaVCalc.cs
- L=76 kind=class type=DeltaVCalc bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Vessel

## Assembly-CSharp/DeltaVEngineBurnTotals.cs
- L=20 kind=class type=DeltaVEngineBurnTotals bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Vessel

## Assembly-CSharp/DeltaVEngineExtensions.cs
- L=42 kind=static class type=DeltaVEngineExtensions bases=- anc=- ns=-
- unity(0): none | ksp: Part=2 | fan_in=0
- methods(2): Contains, Get
- auto: A / Vessel

## Assembly-CSharp/DeltaVEngineInfo.cs
- L=796 kind=class type=DeltaVEngineInfo bases=- anc=- ns=-
- unity(6): none | ksp: HighLogic=9, FlightGlobals=5, Vessel=2 | fan_in=9
- methods(32): ApplyBurn, CalcThrustActual, CalculateBurn, CalculateFuelTime, CalculateISP, CalculateThrustVector, GetFuelTimeAtActiveThrottle, GetFuelTimeAtThrottle, GetFuelTimeMaxThrottle, GetISPASL, GetISPActual, GetISPVac, GetSituationISP, GetSituationThrust, GetSituationThrustVector, GetThrustASL, GetThrustActual, GetThrustVac, GetThrustVectorASL, GetThrustVectorActual, GetThrustVectorVac, PropellantDemand, PropellantMassBurnt, PropellantStarved ...
- auto: A / Vessel

## Assembly-CSharp/DeltaVEngineResourcePartExtensions.cs
- L=71 kind=static class type=DeltaVEngineResourcePartExtensions bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(5): ContainsPartInfo, Get, GetHighestPartSeparationIndex, PartInfoList, PartsMatchingSeparationIndex
- auto: A / Vessel

## Assembly-CSharp/DeltaVEngineResourcePartInfo.cs
- L=17 kind=class type=DeltaVEngineResourcePartInfo bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Vessel

## Assembly-CSharp/DeltaVEngineStageSet.cs
- L=507 kind=class type=DeltaVEngineStageSet bases=- anc=- ns=-
- unity(5): none | ksp: Vessel=16, HighLogic=1 | fan_in=1
- methods(9): AddEngineWorkingSet, AddStageInfo, Compare, RemoveInvalidEnginesWorkingSet, RemoveStagedStages, RemoveStaleStages, SetPayload, SortStages, UpdateStageInfo
- auto: A / Vessel

## Assembly-CSharp/DeltaVExtensions.cs
- L=15 kind=static class type=DeltaVExtensions bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(1): GetSwitchedValue
- auto: A / Vessel

## Assembly-CSharp/DeltaVGlobals.cs
- L=144 kind=MonoBehaviour type=DeltaVGlobals bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(40): Lifecycle=3, MonoBehaviour=1, GetComponent=1 | ksp: Localizer=14, GameEvents=5, GameSettings=3, FlightGlobals=1, CelestialBody=1 | fan_in=10
- methods(8): Awake, DisableStockSimluations, EnableStockSimluations, InitDeltaVGlobal, InitInfoLines, OnDestroy, OnSceneSwitch, Start
- auto: B / Vessel

## Assembly-CSharp/DeltaVPartExtensions.cs
- L=55 kind=static class type=DeltaVPartExtensions bases=- anc=- ns=-
- unity(0): none | ksp: Part=5 | fan_in=0
- methods(3): ContainsPart, Get, PartsInStage
- auto: A / Vessel

## Assembly-CSharp/DeltaVPartInfo.cs
- L=357 kind=class type=DeltaVPartInfo bases=- anc=- ns=-
- unity(5): none | ksp: Part=7, GameSettings=4, PartResourceDefinition=2, PartResource=2, AttachNode=2 | fan_in=6
- methods(12): CalculateMassValues, CalculateStagingValues, CheckDecoupler, FindDecouplerParent, GetCurrentFuelMass, GetSeparationStage, GetStageEndMass, GetStageStartMass, JettisonInStage, ResetStartSimulationResources, StageEndFuelMass, StageStartFuelMass
- auto: A / Vessel

## Assembly-CSharp/DeltaVPropellantInfo.cs
- L=68 kind=class type=DeltaVPropellantInfo bases=- anc=- ns=-
- unity(6): none | ksp: none | fan_in=6
- methods(2): Copy, MassBurnt
- auto: A / Vessel

## Assembly-CSharp/DeltaVSituationOptions.cs
- L=6 kind=enum type=DeltaVSituationOptions bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=11
- methods(0): 
- auto: A / Vessel

## Assembly-CSharp/DepletionData.cs
- L=15 kind=class type=DepletionData bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / Resources

## Assembly-CSharp/DepletionNode.cs
- L=10 kind=class type=DepletionNode bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / Resources

## Assembly-CSharp/DestroyAfterTime.cs
- L=16 kind=MonoBehaviour type=DestroyAfterTime bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): SendMessage/Invoke=1, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=0
- methods(2): Kill, Start
- auto: B / Core

## Assembly-CSharp/DestroyOnSceneSwitch.cs
- L=32 kind=MonoBehaviour type=DestroyOnSceneSwitch bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(86): SceneManager=3, Lifecycle=2, Instantiate/Destroy=2, GameObject=2, MonoBehaviour=1 | ksp: GameEvents=2 | fan_in=0
- methods(5): OnDestroy, OnLevelLoaded, OnSceneLoaded, OnSceneSwitch, Start
- auto: B / Scene

## Assembly-CSharp/DestructibleBuilding.cs
- L=718 kind=MonoBehaviour type=DestructibleBuilding bases=MonoBehaviour,ICollisionEvents anc=MonoBehaviour ns=-
- unity(66): Coroutine=19, GameObject=15, Transform=12, SetActive=8, Lifecycle=6, Collision/Force=6 | ksp: GameEvents=15, HighLogic=8, PhysicsGlobals=7, ConfigNode=4, Vessel=3 | fan_in=6
- methods(43): AddDamage, Awake, ClearID, Collapse, CompileID, Demolish, EndFlightNoDamagePeriod, FindVesselsOverStructure, FixedUpdate, GetInstance, GetRepairSequenceDuration, Init, Load, LoadOnSceneStart, OnCollisionEnter, OnCollisionExit, OnCollisionStay, OnDestroy, OnDisable, OnEnable, OnLeavingScene, OnLevelLoaded, OnPhysicsEase, OnSceneStart ...
- auto: B / Career

## Assembly-CSharp/Detonator.cs
- L=413 kind=MonoBehaviour type=Detonator bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(88): Material=32, Texture=16, Shader=16, GetComponent=10, GameObject=9, Resources.=8 | ksp: none | fan_in=12
- methods(15): Awake, DefaultFireballAMaterial, DefaultFireballBMaterial, DefaultGlowMaterial, DefaultHeatwaveMaterial, DefaultShockwaveMaterial, DefaultSmokeAMaterial, DefaultSmokeBMaterial, DefaultSparksMaterial, Explode, FillDefaultMaterials, Reset, Start, Update, UpdateComponents
- auto: C / Rendering

## Assembly-CSharp/DetonatorBurstEmitter.cs
- L=262 kind=MonoBehaviour type=DetonatorBurstEmitter bases=DetonatorComponent anc=DetonatorComponent/MonoBehaviour ns=-
- unity(77): ParticleSystem=22, Material=4, Time.=3, GetComponent=3, Lifecycle=2, GameObject=2 | ksp: none | fan_in=6
- methods(6): Awake, Explode, Init, Reset, SizeFunction, Update
- auto: C / Rendering

## Assembly-CSharp/DetonatorCloudRing.cs
- L=93 kind=MonoBehaviour type=DetonatorCloudRing bases=DetonatorComponent anc=DetonatorComponent/MonoBehaviour ns=-
- unity(66): Transform=4, Material=2, GameObject=2, GetComponent=1 | ksp: none | fan_in=0
- methods(6): BuildCloudRing, Explode, FillMaterials, Init, Reset, UpdateCloudRing
- auto: C / Rendering

## Assembly-CSharp/DetonatorComponent.cs
- L=72 kind=MonoBehaviour type=DetonatorComponent bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(46): MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=13
- methods(4): Explode, Init, MyDetonator, SetStartValues
- auto: C / Rendering

## Assembly-CSharp/DetonatorDriver.cs
- L=14 kind=MonoBehaviour type=DetonatorDriver bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(50): MonoBehaviour=1, Lifecycle=1 | ksp: GameSettings=1 | fan_in=0
- methods(2): Awake, GetDetonatorVolume
- auto: C / Rendering

## Assembly-CSharp/DetonatorFireball.cs
- L=211 kind=MonoBehaviour type=DetonatorFireball bases=DetonatorComponent anc=DetonatorComponent/MonoBehaviour ns=-
- unity(74): Transform=15, Material=6, GameObject=6, GetComponent=3, Camera=1 | ksp: none | fan_in=1
- methods(10): BuildFireShadow, BuildFireballA, BuildFireballB, Explode, FillMaterials, Init, Reset, UpdateFireShadow, UpdateFireballA, UpdateFireballB
- auto: C / Rendering

## Assembly-CSharp/DetonatorForce.cs
- L=109 kind=MonoBehaviour type=DetonatorForce bases=DetonatorComponent anc=DetonatorComponent/MonoBehaviour ns=-
- unity(85): Transform=7, GetComponent=4, Collider=3, Rigidbody=2, ParticleSystem=2, Instantiate/Destroy=2 | ksp: none | fan_in=1
- methods(4): Explode, Init, Reset, Update
- auto: C / Rendering

## Assembly-CSharp/DetonatorGlow.cs
- L=104 kind=MonoBehaviour type=DetonatorGlow bases=DetonatorComponent anc=DetonatorComponent/MonoBehaviour ns=-
- unity(66): Transform=4, Material=2, GameObject=2, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=1
- methods(7): BuildGlow, Explode, FillMaterials, Init, Reset, Update, UpdateGlow
- auto: C / Rendering

## Assembly-CSharp/DetonatorHeatwave.cs
- L=100 kind=MonoBehaviour type=DetonatorHeatwave bases=DetonatorComponent anc=DetonatorComponent/MonoBehaviour ns=-
- unity(89): Transform=7, Material=5, GameObject=4, GetComponent=3, Time.=2, Shader=2 | ksp: none | fan_in=1
- methods(4): Explode, Init, Reset, Update
- auto: C / Rendering

## Assembly-CSharp/DetonatorLight.cs
- L=64 kind=MonoBehaviour type=DetonatorLight bases=DetonatorComponent anc=DetonatorComponent/MonoBehaviour ns=-
- unity(77): Transform=3, Time.=3, Light=3, GameObject=2, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=1
- methods(4): Explode, Init, Reset, Update
- auto: C / Rendering

## Assembly-CSharp/DetonatorShockwave.cs
- L=88 kind=MonoBehaviour type=DetonatorShockwave bases=DetonatorComponent anc=DetonatorComponent/MonoBehaviour ns=-
- unity(67): Transform=5, Material=2, GameObject=2, GetComponent=1 | ksp: none | fan_in=1
- methods(6): BuildShockwave, Explode, FillMaterials, Init, Reset, UpdateShockwave
- auto: C / Rendering

## Assembly-CSharp/DetonatorSmoke.cs
- L=165 kind=MonoBehaviour type=DetonatorSmoke bases=DetonatorComponent anc=DetonatorComponent/MonoBehaviour ns=-
- unity(78): Transform=16, Material=4, GameObject=4, GetComponent=2, Camera=2 | ksp: none | fan_in=1
- methods(8): BuildSmokeA, BuildSmokeB, Explode, FillMaterials, Init, Reset, UpdateSmokeA, UpdateSmokeB
- auto: C / Rendering

## Assembly-CSharp/DetonatorSound.cs
- L=83 kind=MonoBehaviour type=DetonatorSound bases=DetonatorComponent anc=DetonatorComponent/MonoBehaviour ns=-
- unity(72): Audio=4, Transform=2, Time.=2, Lifecycle=1, GetComponent=1, GameObject=1 | ksp: none | fan_in=1
- methods(4): Explode, Init, Reset, Update
- auto: C / Rendering

## Assembly-CSharp/DetonatorSparks.cs
- L=97 kind=MonoBehaviour type=DetonatorSparks bases=DetonatorComponent anc=DetonatorComponent/MonoBehaviour ns=-
- unity(69): Transform=4, Physics.=2, Material=2, GameObject=2, GetComponent=1 | ksp: none | fan_in=1
- methods(6): BuildSparks, Explode, FillMaterials, Init, Reset, UpdateSparks
- auto: C / Rendering

## Assembly-CSharp/DetonatorSpray.cs
- L=75 kind=MonoBehaviour type=DetonatorSpray bases=DetonatorComponent anc=DetonatorComponent/MonoBehaviour ns=-
- unity(75): Transform=5, Instantiate/Destroy=2, GameObject=2, Time.=1, Rigidbody=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(4): Explode, Init, Reset, Update
- auto: C / Rendering

## Assembly-CSharp/DialogCanvasUtil.cs
- L=33 kind=static class type=DialogCanvasUtil bases=- anc=- ns=-
- unity(67): UnityEngine.UI=5, GetComponent=1 | ksp: none | fan_in=25
- methods(0): 
- auto: C / UI

## Assembly-CSharp/DialogGUIBase.cs
- L=302 kind=class type=DialogGUIBase bases=- anc=- ns=-
- unity(60): UnityEngine.UI=15, SetActive=9, GetComponent=9, GameObject=6, TMPro=3, Transform=2 | ksp: none | fan_in=39
- methods(10): AddChild, AddChildren, Create, GetTextObject, Resize, SelectFirstItem, SetOptionText, SetUpTextObject, SetupTransformAndLayout, Update
- auto: C / UI

## Assembly-CSharp/DialogGUIBox.cs
- L=50 kind=class type=DialogGUIBox bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(70): GetComponent=5, UnityEngine.UI=3, Transform=2, SetActive=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=10
- methods(1): Create
- auto: C / UI

## Assembly-CSharp/DialogGUIButton.cs
- L=324 kind=class type=DialogGUIButton bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(62): UnityEngine.UI=19, GetComponent=15, SetActive=5, Texture=3, GameObject=3, Transform=2 | ksp: none | fan_in=85
- methods(5): ClearButtonImage, Create, OptionSelected, Resize, Update
- auto: C / UI

## Assembly-CSharp/DialogGUIContentSizer.cs
- L=57 kind=class type=DialogGUIContentSizer bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(82): UnityEngine.UI=15, GetComponent=4, Transform=2, GameObject=2, Lifecycle=1 | ksp: none | fan_in=13
- methods(3): Create, Resize, Update
- auto: C / UI

## Assembly-CSharp/DialogGUIFlexibleSpace.cs
- L=33 kind=class type=DialogGUIFlexibleSpace bases=DialogGUISpace anc=DialogGUISpace/DialogGUIBase ns=-
- unity(90): GetComponent=14, GameObject=12, UnityEngine.UI=6, Transform=3 | ksp: none | fan_in=21
- methods(1): Create
- auto: C / UI

## Assembly-CSharp/DialogGUIGridLayout.cs
- L=70 kind=class type=DialogGUIGridLayout bases=DialogGUILayoutBase anc=DialogGUILayoutBase/DialogGUIBase ns=-
- unity(77): UnityEngine.UI=12, GetComponent=5, Transform=2, GameObject=2, SetActive=1, Instantiate/Destroy=1 | ksp: none | fan_in=3
- methods(1): Create
- auto: C / UI

## Assembly-CSharp/DialogGUIHorizontalLayout.cs
- L=49 kind=class type=DialogGUIHorizontalLayout bases=DialogGUILayoutBase anc=DialogGUILayoutBase/DialogGUIBase ns=-
- unity(0): none | ksp: none | fan_in=35
- methods(0): 
- auto: C / UI

## Assembly-CSharp/DialogGUIImage.cs
- L=39 kind=class type=DialogGUIImage bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(79): UnityEngine.UI=3, Texture=3, GetComponent=3, Transform=2, SetActive=1, Instantiate/Destroy=1 | ksp: none | fan_in=15
- methods(1): Create
- auto: C / UI

## Assembly-CSharp/DialogGUILabel.cs
- L=123 kind=class type=DialogGUILabel bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(41): Transform=2, TMPro=2, SetActive=1, Lifecycle=1, Instantiate/Destroy=1, GetComponent=1 | ksp: none | fan_in=45
- methods(2): Create, Update
- auto: C / UI

## Assembly-CSharp/DialogGUILayoutBase.cs
- L=87 kind=class type=DialogGUILayoutBase bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(66): UnityEngine.UI=6, GetComponent=5, Transform=3, GameObject=2, SetActive=1, Instantiate/Destroy=1 | ksp: none | fan_in=6
- methods(1): Create
- auto: C / UI

## Assembly-CSharp/DialogGUILayoutEnd.cs
- L=11 kind=class type=DialogGUILayoutEnd bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(35): Transform=1, GameObject=1 | ksp: none | fan_in=9
- methods(1): Create
- auto: C / UI

## Assembly-CSharp/DialogGUIRadialLayout.cs
- L=128 kind=class type=DialogGUIRadialLayout bases=DialogGUILayoutBase anc=DialogGUILayoutBase/DialogGUIBase ns=-
- unity(74): UnityEngine.UI=10, Transform=9, GameObject=9, GetComponent=4, Input.=2, SetActive=1 | ksp: none | fan_in=1
- methods(4): Create, Resize, SelectedItemAtAngle, Update
- auto: C / UI

## Assembly-CSharp/DialogGUIScrollList.cs
- L=120 kind=class type=DialogGUIScrollList bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(81): UnityEngine.UI=20, GameObject=12, GetComponent=8, Transform=7, SetActive=5, Lifecycle=1 | ksp: none | fan_in=13
- methods(3): Create, Resize, Update
- auto: C / UI

## Assembly-CSharp/DialogGUISlider.cs
- L=59 kind=class type=DialogGUISlider bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(69): UnityEngine.UI=5, GetComponent=3, Transform=2, SetActive=1, Lifecycle=1, Instantiate/Destroy=1 | ksp: none | fan_in=7
- methods(2): Create, Update
- auto: C / UI

## Assembly-CSharp/DialogGUISpace.cs
- L=35 kind=class type=DialogGUISpace bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(76): Transform=4, UnityEngine.UI=3, GetComponent=3, GameObject=2 | ksp: none | fan_in=18
- methods(1): Create
- auto: C / UI

## Assembly-CSharp/DialogGUISprite.cs
- L=27 kind=class type=DialogGUISprite bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(78): UnityEngine.UI=2, Transform=2, Texture=2, GetComponent=2, SetActive=1, Instantiate/Destroy=1 | ksp: none | fan_in=1
- methods(1): Create
- auto: C / UI

## Assembly-CSharp/DialogGUITextInput.cs
- L=123 kind=class type=DialogGUITextInput bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(63): TMPro=8, GetComponent=5, UnityEngine.UI=2, Transform=2, SetActive=1, Lifecycle=1 | ksp: none | fan_in=9
- methods(2): Create, Update
- auto: C / UI

## Assembly-CSharp/DialogGUIToggle.cs
- L=150 kind=class type=DialogGUIToggle bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(70): UnityEngine.UI=11, Transform=6, Texture=4, SetActive=3, GetComponent=3, GameObject=3 | ksp: none | fan_in=17
- methods(2): Create, Update
- auto: C / UI

## Assembly-CSharp/DialogGUIToggleButton.cs
- L=48 kind=class type=DialogGUIToggleButton bases=DialogGUIToggle anc=DialogGUIToggle/DialogGUIBase ns=-
- unity(71): UnityEngine.UI=3, GetComponent=3, GameObject=3, Transform=2, SetActive=1, Instantiate/Destroy=1 | ksp: none | fan_in=12
- methods(1): Create
- auto: C / UI

## Assembly-CSharp/DialogGUIToggleGroup.cs
- L=26 kind=class type=DialogGUIToggleGroup bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(75): UnityEngine.UI=4, GetComponent=2, GameObject=2, Transform=1 | ksp: none | fan_in=5
- methods(1): Create
- auto: C / UI

## Assembly-CSharp/DialogGUIVerticalLayout.cs
- L=47 kind=class type=DialogGUIVerticalLayout bases=DialogGUILayoutBase anc=DialogGUILayoutBase/DialogGUIBase ns=-
- unity(15): Lifecycle=1 | ksp: none | fan_in=40
- methods(1): Update
- auto: C / UI

## Assembly-CSharp/DialogOrbitTutorialStep.cs
- L=149 kind=class type=DialogOrbitTutorialStep bases=IntermediateTutorialPageStep anc=IntermediateTutorialPageStep/TutorialPageConfig ns=-
- unity(0): none | ksp: GameEvents=7, Orbit=1 | fan_in=1
- methods(13): AddTutorialStepConfig, FilterDialogNodeCriteria, HighlightDialogueMessage, IsDialogLinked, IsDialogueMessageSetted, OnEnterDragDialogueNode, OnEnterSetDialogueTextHelper, OnLeaveOnEnterDragDialogueNode, OnLeaveSetDialogueTextHelper, OnNodeEventDialogueTuto, OnNodeFocused, OnUpdateDragDialogueNode, OnUpdateSetDialogueTextHelper
- auto: A / Tutorials

## Assembly-CSharp/DialogScienceTutorialStep.cs
- L=152 kind=class type=DialogScienceTutorialStep bases=IntermediateTutorialPageStep anc=IntermediateTutorialPageStep/TutorialPageConfig ns=-
- unity(0): none | ksp: GameEvents=7 | fan_in=1
- methods(13): AddTutorialStepConfig, FilterDialogNodeCriteria, HighlightDialogueMessage, IsDialogLinked, IsDialogueMessageSetted, OnEnterDragSecondDialogue, OnEnterSetDialogueTextHelper, OnLeaveLinkScienceEventNode, OnLeaveSetDialogueTextHelper, OnNodeEventScienceTuto, OnNodeFocused, OnUpdateLinkScienceEventNode, OnUpdateSetDialogueTextHelper
- auto: A / Tutorials

## Assembly-CSharp/DictionaryValueList.cs
- L=139 kind=class type=DictionaryValueList bases=- anc=- ns=-
- unity(8): none | ksp: none | fan_in=51
- methods(12): Add, At, Clear, Contains, ContainsKey, GetDictEnumerator, GetListEnumerator, IndexOf, KeyAt, Remove, SortList, TryGetValue
- auto: A / Core

## Assembly-CSharp/DifficultyOptionsMenu.cs
- L=767 kind=MonoBehaviour type=DifficultyOptionsMenu bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(40): UnityEngine.UI=6, GameObject=5, GetComponent=2, TMPro=1, SetActive=1, MonoBehaviour=1 | ksp: Localizer=38, Game=20, GameEvents=3, InputLockManager=2, ConfigNode=2 | fan_in=4
- methods(5): Create, CreateDifficultWindow, Dismiss, SetPreset, Start
- auto: C / UI

## Assembly-CSharp/DirectionTarget.cs
- L=34 kind=class type=DirectionTarget bases=PositionTarget anc=PositionTarget/ITargetable ns=-
- unity(19): Transform=1 | ksp: Vector3d=4, Vessel=1, FlightGlobals=1 | fan_in=4
- methods(1): Update
- auto: A / Orbit

## Assembly-CSharp/DirectoryActionGroup.cs
- L=466 kind=MonoBehaviour type=DirectoryActionGroup bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): GameObject=14, UnityEngine.UI=10, Transform=9, SetActive=8, GetComponent=7, EventSystems=6 | ksp: GameEvents=2 | fan_in=1
- methods(27): AddExtraSpacing, AddItemToContent, AttachListener, Awake, Collapse, Deselect, Expand, ExpandOverride, GetFileCount, Initialize, OnDestroy, OnDirectoryClicked, OnExpandToggleClick, OnScroll, RemoveItemFromContent, RescaleContentItems, Select, SetLabelColor, SetName, SetUIState, Start, UpdateContentSize, UpdateCraftFileCount, UpdateExpansionIcon ...
- auto: C / Vessel

## Assembly-CSharp/DisableOnPlay.cs
- L=9 kind=MonoBehaviour type=DisableOnPlay bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(67): SetActive=1, MonoBehaviour=1, Lifecycle=1, GameObject=1 | ksp: none | fan_in=0
- methods(1): Start
- auto: B / Misc

## Assembly-CSharp/DiscoverableObjectsUtil.cs
- L=109 kind=static class type=DiscoverableObjectsUtil bases=- anc=- ns=-
- unity(2): none | ksp: ProtoVessel=10, ConfigNode=10, HighLogic=9, Orbit=2, GameEvents=2 | fan_in=6
- methods(6): GenerateAsteroidName, GenerateCometName, SpawnAsteroid, SpawnComet, randomLetter, randomNumber
- auto: A / Celestials

## Assembly-CSharp/DiscoveryInfo.cs
- L=490 kind=class type=DiscoveryInfo bases=IConfigNode anc=IConfigNode ns=-
- unity(1): none | ksp: Localizer=47, Planetarium=2, GameEvents=2, ConfigNode=2 | fan_in=56
- methods(20): CacheLocalStrings, CompileInfoGetters, GetClassFromSize, GetClassRadius, GetMaxRadius, GetMinRadius, GetObjectClass, GetSignalLife, GetSignalStrength, GetSignalStrengthCaption, GetSizeClassDescription, GetSizeClassSizes, HaveKnowledgeAbout, Load, Save, SetLastObservedTime, SetLevel, SetReferenceLifetime, SetUnobservedLifetime, SetUntrackedObjectSize
- auto: A / Celestials

## Assembly-CSharp/DiscoveryLevels.cs
- L=13 kind=enum type=DiscoveryLevels bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=46
- methods(0): 
- auto: A / Celestials

## Assembly-CSharp/DishController.cs
- L=172 kind=MonoBehaviour type=DishController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(49): Transform=2, Time.=2, Lifecycle=2, MonoBehaviour=1 | ksp: Planetarium=4 | fan_in=0
- methods(4): CreateTimeStep, SetTimeStep, Start, Update
- auto: B / CommNet

## Assembly-CSharp/DisplacementExplosion.cs
- L=67 kind=MonoBehaviour type=DisplacementExplosion bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(85): Time.=2, Shader=2, Renderer=2, Material=2, Lifecycle=2, GetComponent=2 | ksp: none | fan_in=0
- methods(6): Awake, Update, UpdateExplosion, UpdateShaderAnimation, UpdateShaderClip, UpdateTransform
- auto: C / Rendering

## Assembly-CSharp/DistributionData.cs
- L=29 kind=class type=DistributionData bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=3
- methods(1): HasVariableAltitude
- auto: A / Resources

## Assembly-CSharp/DockedVesselInfo.cs
- L=107 kind=class type=DockedVesselInfo bases=IConfigNode anc=IConfigNode ns=-
- unity(3): none | ksp: ConfigNode=2 | fan_in=4
- methods(2): Load, Save
- auto: A / PartModules-Structural

## Assembly-CSharp/DoNotDestroy.cs
- L=9 kind=MonoBehaviour type=DoNotDestroy bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(67): MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=1
- methods(1): Awake
- auto: B / Scene

## Assembly-CSharp/DoubleCurve.cs
- L=279 kind=class type=DoubleCurve bases=IConfigNode anc=IConfigNode ns=-
- unity(3): none | ksp: ConfigNode=2 | fan_in=6
- methods(7): Add, Evaluate, FindMinMaxValue, GetInsertionIndex, Load, RecomputeTangents, Save
- auto: A / Core

## Assembly-CSharp/DoubleKeyframe.cs
- L=38 kind=struct type=DoubleKeyframe bases=IComparable anc=IComparable ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Core

## Assembly-CSharp/DoubleSlider.cs
- L=196 kind=MonoBehaviour type=DoubleSlider bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(50): UnityEngine.UI=6, GetComponent=2, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=3
- methods(8): CalculateFinalValues, OnRectTransformDimensionsChange, OnValueChanged, OnValueChanged_SliderFill, OnValueChanged_SliderMax, OnValueChanged_SliderMin, ResizeFillSection, Start
- auto: C / UI

## Assembly-CSharp/DoubleTypeParser.cs
- L=20 kind=class type=DoubleTypeParser bases=ITypeParser anc=ITypeParser ns=-
- unity(0): none | ksp: none | fan_in=3
- methods(2): Convert, Parse
- auto: A / Modding

## Assembly-CSharp/DragCube.cs
- L=214 kind=class type=DragCube bases=- anc=- ns=-
- unity(28): none | ksp: none | fan_in=7
- methods(2): Load, SaveToString
- auto: A / Aero

## Assembly-CSharp/DragCubeSystem.cs
- L=771 kind=MonoBehaviour type=DragCubeSystem bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(79): GameObject=35, GetComponent=15, Instantiate/Destroy=13, Coroutine=11, Material=10, Renderer=9 | ksp: Part=15, ConfigNode=9, PartLoader=6, PartModule=3, Vessel=1 | fan_in=4
- methods(23): Awake, CalculateAerodynamics, CalculatePartBounds, CleanUpPartMaterials, CreateAeroCamera, CreateAeroTextures, GetAreaModifier, GetCdModifier, GetPlanarCenter, GetPlanarDepth, GetPlanarDirection, GetPlanarSize, LoadDragCubes, OnDestroy, RenderDragCubes, RenderDragCubesCoroutine, RenderProceduralDragCube, SetAeroCamera, SetMainTexture, SetupDragCubeCoroutine, SetupPartForRender, TestRender, UpdateAeroTexture
- auto: B / Aero

## Assembly-CSharp/DragPanel.cs
- L=42 kind=MonoBehaviour type=DragPanel bases=MonoBehaviour,IPointerDownHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(87): EventSystems=4, UnityEngine.UI=2, Lifecycle=2, Transform=1, MonoBehaviour=1 | ksp: GameEvents=2 | fan_in=4
- methods(5): OnDestroy, OnDrag, OnGameSettingsApplied, OnPointerDown, Start
- auto: C / UI

## Assembly-CSharp/DrawDepth.cs
- L=170 kind=MonoBehaviour type=DrawDepth bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(79): Material=7, Shader=5, QualitySettings/SystemInfo=4, Lifecycle=4, Camera=3, Texture=2 | ksp: none | fan_in=1
- methods(12): CheckResources, CheckShader, CheckShaderAndCreateMaterial, CheckSupport, CreateMaterial, DoDepthRender, LateUpdate, NotSupported, OnDisable, OnEnable, ReportAutoDisable, Start
- auto: C / Rendering

## Assembly-CSharp/Drawing.cs
- L=182 kind=static class type=Drawing bases=- anc=- ns=-
- unity(67): IMGUI=11, Texture=7, Material=5, Graphics/GL=4 | ksp: none | fan_in=1
- methods(8): CubeBezier, DrawBezierLine, DrawGraph, DrawGraphLine, DrawLine, GetMaxValueX, GetMaxValueY, Initialize
- auto: C / Core

## Assembly-CSharp/DynamicAmbientLight.cs
- L=161 kind=MonoBehaviour type=DynamicAmbientLight bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(86): Light=22, Shader=4, Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: HighLogic=11, GameSettings=3, FlightGlobals=3, MapView=2, GameEvents=2 | fan_in=1
- methods(4): Awake, GrabDefaultAmbient, OnDestroy, Update
- auto: C / Rendering

## Assembly-CSharp/DynamicNodeSet.cs
- L=29 kind=struct type=DynamicNodeSet bases=- anc=- ns=-
- unity(12): none | ksp: ConfigNode=1 | fan_in=1
- methods(1): Load
- auto: A / Misc

## Assembly-CSharp/DynamicShadowSettings.cs
- L=125 kind=MonoBehaviour type=DynamicShadowSettings bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(64): QualitySettings/SystemInfo=14, SceneManager=3, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: GameSettings=6, HighLogic=1 | fan_in=1
- methods(5): ApplyShadowSettings, Awake, OnDestroy, OnLevelLoaded, OnSceneLoaded
- auto: C / Rendering

## Assembly-CSharp/EditorBackgroundFlag.cs
- L=41 kind=MonoBehaviour type=EditorBackgroundFlag bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(84): Lifecycle=3, Renderer=2, Material=2, Texture=1, MonoBehaviour=1 | ksp: HighLogic=2, GameEvents=2, EditorLogic=2, GameDatabase=1 | fan_in=0
- methods(4): Awake, OnDestroy, Start, updateFlag
- auto: B / Editor

## Assembly-CSharp/EditorBounds.cs
- L=106 kind=MonoBehaviour type=EditorBounds bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(61): Transform=3, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: none | fan_in=5
- methods(8): Awake, CenterSceneryOrigin, ClampCameraDistance, ClampToCameraBounds, OnDestroy, OnDrawGizmosSelected, SetExtents, centerSceneryOrigin
- auto: B / Editor

## Assembly-CSharp/EditorCamera.cs
- L=26 kind=MonoBehaviour type=EditorCamera bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(83): Lifecycle=2, GetComponent=2, Camera=2, MonoBehaviour=1 | ksp: none | fan_in=5
- methods(2): Awake, OnDestroy
- auto: C / Camera

## Assembly-CSharp/EditorCameraOffset.cs
- L=23 kind=MonoBehaviour type=EditorCameraOffset bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(70): Lifecycle=2, Screen/Cursor=1, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=0
- methods(2): Start, Update
- auto: B / Editor

## Assembly-CSharp/EditorDriver.cs
- L=592 kind=MonoBehaviour type=EditorDriver bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(71): GameObject=20, Coroutine=16, SceneManager=12, SetActive=8, GetComponent=7, UnityEngine.UI=5 | ksp: HighLogic=33, EditorLogic=19, Game=11, GameSettings=8, InputLockManager=7 | fan_in=31
- methods(18): Awake, IsValidSaveFolder, OnApplicationFocus, OnDestroy, OtherLaunchSitesValid, SetDefaultSaveFolder, SetInputLockFromGameParameters, Start, StartAndLoadVessel, StartEditor, SwitchEditor, ValidLaunchSite, initEditor, onEditorStarted, saveselectedLaunchSite, setDefaultLaunchSite, setLaunchSite, setupValidLaunchSites
- auto: C / Scene

## Assembly-CSharp/EditorEnumExtensions.cs
- L=88 kind=static class type=EditorEnumExtensions bases=- anc=- ns=-
- unity(4): none | ksp: Localizer=2, ResearchAndDevelopment=1 | fan_in=1
- methods(5): GetEditorFacility, GetFacility, GetSymmetryMethodName, ToEditor, ToFacility
- auto: A / Editor

## Assembly-CSharp/EditorFacilities.cs
- L=10 kind=enum type=EditorFacilities bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=6
- methods(0): 
- auto: A / Editor

## Assembly-CSharp/EditorFacility.cs
- L=11 kind=enum type=EditorFacility bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=40
- methods(0): 
- auto: A / Scene

## Assembly-CSharp/EditorGeometryUtil.cs
- L=106 kind=static class type=EditorGeometryUtil bases=- anc=- ns=-
- unity(65): Transform=5, Collider=5, GameObject=3, WheelCollider=1, LayerMask=1, Camera=1 | ksp: Part=6 | fan_in=2
- methods(8): GetPartBoundsSeparation, GetPixelDistance, MirrorDirection, MirrorPos, MirrorRotation, TestPartBoundsIntersect, TestPartBoundsSeparate, isValidPartCollider
- auto: B / Editor

## Assembly-CSharp/EditorLogicBase.cs
- L=342 kind=MonoBehaviour type=EditorLogicBase bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(63): Transform=6, GameObject=4, Raycast/Query=3, GetComponent=3, SetActive=2, Renderer=2 | ksp: Part=19, Localizer=17, GameSettings=12, AttachNode=6, InputLockManager=1 | fan_in=3
- methods(12): AssignAttachIcon, CacheLocalStrings, CreateSelectedPartIcon, FindPartSurface, FindPartsInChildren, PartInHierarchy, clearAttachNodes, displayAttachNodeIcons, handleAttachNodeIcons, handleChildNodeIcons, partRotationInputUpdate, partRotationResetUpdate
- auto: B / Editor

## Assembly-CSharp/EditorMarker.cs
- L=33 kind=MonoBehaviour type=EditorMarker bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(77): Transform=2, GameObject=2, MonoBehaviour=1, Lifecycle=1 | ksp: Part=1 | fan_in=3
- methods(3): Update, UpdateDirection, UpdatePosition
- auto: B / Editor

## Assembly-CSharp/EditorMarker_CoL.cs
- L=152 kind=MonoBehaviour type=EditorMarker_CoL bases=EditorMarker anc=EditorMarker/MonoBehaviour ns=-
- unity(58): Transform=6, SetActive=4, Lifecycle=1 | ksp: EditorLogic=14, HighLogic=11, FlightGlobals=7, Part=4, Planetarium=3 | fan_in=2
- methods(4): Awake, FindCoL, UpdatePosition, recurseParts
- auto: B / Editor

## Assembly-CSharp/EditorMarker_CoM.cs
- L=73 kind=MonoBehaviour type=EditorMarker_CoM bases=EditorMarker anc=EditorMarker/MonoBehaviour ns=-
- unity(61): Transform=6, Lifecycle=1 | ksp: EditorLogic=9, HighLogic=4, Part=3, FlightGlobals=3 | fan_in=3
- methods(4): Awake, UpdatePosition, findCenterOfMass, recurseParts
- auto: B / Editor

## Assembly-CSharp/EditorMarker_CoT.cs
- L=93 kind=MonoBehaviour type=EditorMarker_CoT bases=EditorMarker anc=EditorMarker/MonoBehaviour ns=-
- unity(42): Transform=1, Lifecycle=1 | ksp: EditorLogic=12, HighLogic=9, FlightGlobals=4, Part=3 | fan_in=2
- methods(4): Awake, FindCoT, UpdatePosition, recurseParts
- auto: B / Editor

## Assembly-CSharp/EditorPartListFilter.cs
- L=39 kind=class type=EditorPartListFilter bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=14
- methods(2): FilterList, ToString
- auto: A / Editor

## Assembly-CSharp/EditorPartListFilterList.cs
- L=118 kind=class type=EditorPartListFilterList bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(8): AddFilter, FilterList, GetEnumerator, GetFilterKey, GetFilterKeySingleOrNothing, GetFilteredList, RemoveFilter, ToString
- auto: A / Editor

## Assembly-CSharp/EditorReRootUtil.cs
- L=117 kind=static class type=EditorReRootUtil bases=- anc=- ns=-
- unity(17): GameObject=2 | ksp: Part=10, EditorLogic=2 | fan_in=1
- methods(6): GetRootCandidates, MakeRoot, StripInvalidSymmetrySets, StripInvalidSymmetrySets_recurseChildren, SymmetrySetIsValid, isRootCandidate
- auto: A / Editor

## Assembly-CSharp/EditorScreen.cs
- L=7 kind=enum type=EditorScreen bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=10
- methods(0): 
- auto: A / Editor

## Assembly-CSharp/EditorState.cs
- L=7 kind=enum type=EditorState bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: A / Editor

## Assembly-CSharp/EditorToolsUI.cs
- L=262 kind=MonoBehaviour type=EditorToolsUI bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(73): SetActive=14, GameObject=14, UnityEngine.UI=4, Lifecycle=3, Coroutine=2, MonoBehaviour=1 | ksp: EditorLogic=10, GameEvents=9, GameSettings=4, InputLockManager=2, ShipConstruct=1 | fan_in=1
- methods(12): OnDestroy, OnEditorLoad, OnEditorRestart, OnEditorScreenChange, SetMode, Start, Update, onEditorPartEvent, onMoveButtonInput, onPlaceButtonInput, onRootButtonInput, onRotateButtonInput
- auto: C / Editor

## Assembly-CSharp/EditorVesselOverlays.cs
- L=143 kind=MonoBehaviour type=EditorVesselOverlays bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(83): SetActive=12, GameObject=12, Lifecycle=4, Instantiate/Destroy=4, UnityEngine.UI=3, MonoBehaviour=1 | ksp: EditorLogic=7, GameEvents=4, Part=1 | fan_in=1
- methods(10): Awake, DisableMarkers, OnDestroy, OnEditorPartEvent, OnEditorRestart, Start, ToggleCoL, ToggleCoM, ToggleCoT, Update
- auto: C / Editor

## Assembly-CSharp/EffectBehaviour.cs
- L=152 kind=MonoBehaviour type=EffectBehaviour bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(78): ParticleSystem=15, Rigidbody=4, Time.=2, GetComponent=2, MonoBehaviour=1 | ksp: Vector3d=5, Krakensbane=2, ConfigNode=2, Part=1 | fan_in=20
- methods(7): AddParticleEmitter, OffsetParticles, OnEvent, OnInitialize, OnLoad, OnSave, RemoveParticleEmitter
- auto: C / Rendering

## Assembly-CSharp/EffectDefinition.cs
- L=12 kind=attribute type=EffectDefinition bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=16
- methods(0): 
- auto: C / Rendering

## Assembly-CSharp/EffectList.cs
- L=260 kind=class type=EffectList bases=- anc=- ns=-
- unity(12): GetComponent=2, GameObject=1 | ksp: ConfigNode=6, Part=2 | fan_in=2
- methods(11): ConstructEffectList, CreateEffectTypes, EffectsStartingWith, Event, GetEffect, GetEffectType, Initialize, LoadEffect, OnLoad, OnSave, PlayRandomEffect
- auto: C / Rendering

## Assembly-CSharp/EngineType.cs
- L=12 kind=enum type=EngineType bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / PartModules-Engines

## Assembly-CSharp/EnumExtensions.cs
- L=75 kind=static class type=EnumExtensions bases=- anc=- ns=-
- unity(0): none | ksp: Localizer=1 | fan_in=0
- methods(5): Description, GetDescriptions, GetName, ToStringCached, displayDescription
- auto: A / Core

## Assembly-CSharp/EnvMapAnimator.cs
- L=29 kind=MonoBehaviour type=EnvMapAnimator bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(92): Time.=3, TMPro=2, Shader=1, MonoBehaviour=1, Material=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(2): Awake, Start
- auto: C / Rendering

## Assembly-CSharp/ESAMissionScreenTutorial.cs
- L=105 kind=ScenarioModule type=ESAMissionScreenTutorial bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(16): none | ksp: GameSettings=2, ConfigNode=2 | fan_in=1
- methods(12): CreateESAMissionScreenTutorialPages, ESAMissionScreenTutorialPageOne, ESAMissionScreenTutorialPageThree, ESAMissionScreenTutorialPageTwo, OnAssetSetup, OnDoneButtonClick, OnEnterESAMissionScreenTutorialPageOne, OnEnterESAMissionScreenTutorialPageThree, OnEnterESAMissionScreenTutorialPageTwo, OnLoad, OnSave, OnTutorialSetup
- auto: A / Tutorials

## Assembly-CSharp/EVACamera.cs
- L=108 kind=MonoBehaviour type=EVACamera bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(90): Transform=11, Time.=7, Input.=7, KeyCode=4, GameObject=3, Lifecycle=2 | ksp: GameSettings=12, FlightGlobals=2, Vessel=1 | fan_in=0
- methods(2): FixedUpdate, Start
- auto: C / Camera

## Assembly-CSharp/EVAConstructionModeController.cs
- L=635 kind=MonoBehaviour type=EVAConstructionModeController bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(67): Transform=19, GameObject=12, Instantiate/Destroy=7, SetActive=6, UnityEngine.UI=5, TMPro=4 | ksp: GameEvents=18, FlightGlobals=12, InputLockManager=5, GameSettings=5, Localizer=4 | fan_in=23
- methods(26): Awake, CanOpenCargoPanel, CanOpenConstructionPanel, ClosePanel, DestroyHeldIcons, OnActionGroupsOpened, OnDestroy, OnEditorPartEvent, OnGameSceneLoadRequested, OnLoadedScene, OnPointerEnter, OnPointerExit, OnVesselChange, OpenCargoPanel, OpenConstructionPanel, RegisterAppButtonCargo, RegisterAppButtonConstruction, RemoveInvDisplayItem, SearchForInventoryParts, SetAppLauncherButtonVisibility, SpawnMarkerCamera, Start, TryGetDisplayedInventory, Update ...
- auto: C / Kerbals

## Assembly-CSharp/EVAConstructionToolsUI.cs
- L=164 kind=MonoBehaviour type=EVAConstructionToolsUI bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(73): SetActive=8, GameObject=8, UnityEngine.UI=4, Lifecycle=3, MonoBehaviour=1 | ksp: GameSettings=3, InputLockManager=1, GameEvents=1 | fan_in=1
- methods(11): Awake, HideModeTools, OnChangeModeBlocked, OnDestroy, SetMode, ShowModeTools, ToggleButtonListeners, Update, onMoveButtonInput, onPlaceButtonInput, onRotateButtonInput
- auto: C / Kerbals

## Assembly-CSharp/EVAConstructionUtil.cs
- L=15 kind=class type=EVAConstructionUtil bases=- anc=- ns=-
- unity(0): none | ksp: PhysicsGlobals=2, Vessel=1 | fan_in=2
- methods(1): GetConstructionGee
- auto: A / Kerbals

## Assembly-CSharp/EVAResourceContainer.cs
- L=113 kind=PartModule type=EVAResourceContainer bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(64): Coroutine=5, Resources.=3, Transform=2, Time.=2, Lifecycle=1 | ksp: KSPField=6, PartResource=5, FlightGlobals=4, Localizer=3, KSPEvent=3 | fan_in=0
- methods(8): GetResourceExternalEvent, OnDestroy, OnStart, PutResourceExternalEvent, StopTransfer, onPartMenuOpen, transferResources, updateModuleUI
- auto: A / Resources

## Assembly-CSharp/EventData.cs
- L=428 kind=class type=EventData bases=BaseGameEvent anc=BaseGameEvent ns=-
- unity(21): none | ksp: none | fan_in=28
- methods(4): Add, Fire, OnEvent, Remove
- auto: A / Core

## Assembly-CSharp/EventDataModifier.cs
- L=118 kind=class type=EventDataModifier bases=BaseGameEvent anc=BaseGameEvent ns=-
- unity(19): none | ksp: none | fan_in=1
- methods(5): Add, DefaultModifier, Fire, OnEvent, Remove
- auto: A / Core

## Assembly-CSharp/EventDoubleMax.cs
- L=115 kind=class type=EventDoubleMax bases=BaseGameEvent anc=BaseGameEvent ns=-
- unity(20): none | ksp: none | fan_in=0
- methods(4): Add, Fire, OnEvent, Remove
- auto: A / Core

## Assembly-CSharp/EventDoubleMin.cs
- L=115 kind=class type=EventDoubleMin bases=BaseGameEvent anc=BaseGameEvent ns=-
- unity(20): none | ksp: none | fan_in=0
- methods(4): Add, Fire, OnEvent, Remove
- auto: A / Core

## Assembly-CSharp/EventExplanationTutorialStep.cs
- L=129 kind=class type=EventExplanationTutorialStep bases=IntermediateTutorialPageStep anc=IntermediateTutorialPageStep/TutorialPageConfig ns=-
- unity(27): GameObject=6 | ksp: none | fan_in=1
- methods(9): AddTutorialStepConfig, CanShowSetEventSelector, OnEnterSetEventNodeSetting, OnEnterSetEventNodeSettingHelper, OnLeaveSetEventNodeSetting, OnLeaveSetEventNodeSettingHelper, OnSelectedGameObjectChangeSetEvent, OnUpdateSetEventNodeSetting, OnUpdateSetEventNodeSettingHelper
- auto: B / Tutorials

## Assembly-CSharp/EventFloatMax.cs
- L=115 kind=class type=EventFloatMax bases=BaseGameEvent anc=BaseGameEvent ns=-
- unity(20): none | ksp: none | fan_in=0
- methods(4): Add, Fire, OnEvent, Remove
- auto: A / Core

## Assembly-CSharp/EventFloatMin.cs
- L=115 kind=class type=EventFloatMin bases=BaseGameEvent anc=BaseGameEvent ns=-
- unity(20): none | ksp: none | fan_in=0
- methods(4): Add, Fire, OnEvent, Remove
- auto: A / Core

## Assembly-CSharp/EventIntMax.cs
- L=115 kind=class type=EventIntMax bases=BaseGameEvent anc=BaseGameEvent ns=-
- unity(20): none | ksp: none | fan_in=0
- methods(4): Add, Fire, OnEvent, Remove
- auto: A / Core

## Assembly-CSharp/EventIntMin.cs
- L=115 kind=class type=EventIntMin bases=BaseGameEvent anc=BaseGameEvent ns=-
- unity(20): none | ksp: none | fan_in=0
- methods(4): Add, Fire, OnEvent, Remove
- auto: A / Core

## Assembly-CSharp/EventReport.cs
- L=27 kind=class type=EventReport bases=- anc=- ns=-
- unity(0): none | ksp: Part=2 | fan_in=20
- methods(0): 
- auto: A / Core

## Assembly-CSharp/EventSelectionTutorialStep.cs
- L=118 kind=class type=EventSelectionTutorialStep bases=IntermediateTutorialPageStep anc=IntermediateTutorialPageStep/TutorialPageConfig ns=-
- unity(29): GameObject=6 | ksp: none | fan_in=1
- methods(11): AddTutorialStepConfig, IsEventBoxSelected, IsScienceNodeSelected, OnEnterTickEventNode, OnEnterTickEventNodeHelper, OnLeaveOnEnterTickEventNode, OnLeaveOnEnterTickEventNodeHelper, OnSelectDuringTickEvent, OnUpdateTickEventNode, OnUpdateTickEventNodeHelper, ShowEventBoxSelector
- auto: B / Tutorials

## Assembly-CSharp/EventStartTutorialStep.cs
- L=15 kind=class type=EventStartTutorialStep bases=IntermediateTutorialPageStep anc=IntermediateTutorialPageStep/TutorialPageConfig ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(1): AddTutorialStepConfig
- auto: A / Tutorials

## Assembly-CSharp/EventValueComparison.cs
- L=117 kind=class type=EventValueComparison bases=BaseGameEvent anc=BaseGameEvent ns=-
- unity(19): Lifecycle=1 | ksp: none | fan_in=1
- methods(5): Add, OnCompare, OnEvent, Remove, Update
- auto: A / Core

## Assembly-CSharp/EventValueOperation.cs
- L=114 kind=class type=EventValueOperation bases=BaseGameEvent anc=BaseGameEvent ns=-
- unity(20): Lifecycle=1 | ksp: none | fan_in=1
- methods(5): Add, OnEvent, OnOperation, Remove, Update
- auto: A / Core

## Assembly-CSharp/EventValueWrapper.cs
- L=64 kind=class type=EventValueWrapper bases=- anc=- ns=-
- unity(11): Lifecycle=1 | ksp: none | fan_in=1
- methods(1): Update
- auto: A / Core

## Assembly-CSharp/EventVoid.cs
- L=110 kind=class type=EventVoid bases=BaseGameEvent anc=BaseGameEvent ns=-
- unity(20): none | ksp: none | fan_in=2
- methods(4): Add, Fire, OnEvent, Remove
- auto: A / Core

## Assembly-CSharp/ExperimentResultDialogPage.cs
- L=175 kind=class type=ExperimentResultDialogPage bases=- anc=- ns=-
- unity(2): none | ksp: ResearchAndDevelopment=9, Localizer=8, HighLogic=6, FlightGlobals=4, Part=2 | fan_in=3
- methods(1): UpdatePageLabValue
- auto: A / Science

## Assembly-CSharp/ExperimentSituations.cs
- L=17 kind=enum type=ExperimentSituations bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=15
- methods(0): 
- auto: A / Science

## Assembly-CSharp/ExperimentTransfer.cs
- L=46 kind=MonoBehaviour type=ExperimentTransfer bases=PartItemTransfer anc=PartItemTransfer/MonoBehaviour ns=-
- unity(41): GetComponent=1, GameObject=1 | ksp: Part=6, Localizer=1 | fan_in=2
- methods(5): Create, IsSemiValidPart, IsValidPart, OnPartSelect, OnSrcPartSelect
- auto: B / Science

## Assembly-CSharp/ExperimentUsageReqs.cs
- L=9 kind=enum type=ExperimentUsageReqs bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / Science

## Assembly-CSharp/ExplosionDestroyer.cs
- L=21 kind=MonoBehaviour type=ExplosionDestroyer bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(77): Time.=2, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=0
- methods(2): Awake, Update
- auto: C / Rendering

## Assembly-CSharp/ExplosionSpawner.cs
- L=34 kind=MonoBehaviour type=ExplosionSpawner bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(92): Transform=4, GameObject=4, Time.=2, Lifecycle=2, Instantiate/Destroy=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(2): Awake, Update
- auto: C / Rendering

## Assembly-CSharp/ExplosionTester.cs
- L=20 kind=MonoBehaviour type=ExplosionTester bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(95): Raycast/Query=3, Input.=2, Physics.=1, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1 | ksp: none | fan_in=0
- methods(1): Update
- auto: C / Rendering

## Assembly-CSharp/ExtendedInput.cs
- L=126 kind=MonoBehaviour type=ExtendedInput bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(68): Input.=7, KeyCode=3, Time.=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=4
- methods(7): DetectKeyDown, GetKey, GetKeyDown, GetKeyUp, IsPressed, LateUpdate, WasPressed
- auto: B / Input

## Assembly-CSharp/FacilityScreenFitter.cs
- L=24 kind=MonoBehaviour type=FacilityScreenFitter bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(88): UnityEngine.UI=4, Lifecycle=2, Transform=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(2): Awake, Start
- auto: C / UI

## Assembly-CSharp/FieldAttribute.cs
- L=27 kind=attribute type=FieldAttribute bases=Attribute anc=Attribute ns=-
- unity(12): none | ksp: Localizer=1 | fan_in=4
- methods(0): 
- auto: A / Part

## Assembly-CSharp/FlagBrowser.cs
- L=291 kind=MonoBehaviour type=FlagBrowser bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(69): UnityEngine.UI=20, GameObject=3, Instantiate/Destroy=2, GetComponent=2, MonoBehaviour=1, Lifecycle=1 | ksp: GameDatabase=10, Localizer=6 | fan_in=8
- methods(9): Accept, CreateFlagBrowser, Dismiss, FlagSelectedCallback, ProcessButtons, ProcessFlagFolder, Start, ToggleAllOff, Toggled
- auto: C / UI

## Assembly-CSharp/FlagBrowserButton.cs
- L=85 kind=MonoBehaviour type=FlagBrowserButton bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(61): UnityEngine.UI=2, Texture=2, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1 | ksp: InputLockManager=4, HighLogic=3, GameEvents=2 | fan_in=3
- methods(6): OnFlagCancel, OnFlagSelect, SetFlag, Setup, SpawnBrowser, Start
- auto: C / UI

## Assembly-CSharp/FlagBrowserGUIButton.cs
- L=97 kind=class type=FlagBrowserGUIButton bases=DialogGUIButton anc=DialogGUIButton/DialogGUIBase ns=-
- unity(68): IMGUI=9, UnityEngine.UI=3, Texture=2, GetComponent=2, Instantiate/Destroy=1, GameObject=1 | ksp: GameDatabase=1 | fan_in=2
- methods(4): Draw, OnFlagCancel, OnFlagSelect, SpawnBrowser
- auto: C / UI

## Assembly-CSharp/FlagDecal.cs
- L=171 kind=PartModule type=FlagDecal bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(68): Transform=7, Material=4, GameObject=3, Renderer=2, Texture=1, Lifecycle=1 | ksp: HighLogic=5, GameEvents=4, EditorLogic=4, KSPField=3, Part=2 | fan_in=2
- methods(11): FlipTexture, GetModuleDisplayName, MirrorFlag, OnAwake, OnDestroy, OnStart, OnVariantApplied, ToggleFlag, UpdateDisplay, UpdateFlagTexture, updateFlag
- auto: C / Rendering

## Assembly-CSharp/FlagDecalBackground.cs
- L=660 kind=PartModule type=FlagDecalBackground bases=FlagDecal,IThumbnailSetup,IPartMassModifier anc=FlagDecal/PartModule/MonoBehaviour ns=-
- unity(34): GameObject=5, Transform=3, Material=2, GetComponent=2, Shader=1, SetActive=1 | ksp: EditorLogic=14, HighLogic=13, Part=10, KSPField=7, GameEvents=6 | fan_in=5
- methods(31): AssumeSnapshotPosition, CheckSymmetry, DisplayingPortrait_OnValueModified, DisplayingPortrait_OnValueSymmetryModified, EnableCurrentFlagMesh, FlagMeshIndex, GetModuleCost, GetModuleCostChangeWhen, GetModuleDisplayName, GetModuleMass, GetModuleMassChangeWhen, HasSurfaceAttachedParts, IsFlagOrientationCounterPartAvailable, LoadFlagsTransformAndRenderer, OnAwake, OnDestroy, OnEditorPartEvent, OnInventoryModeDisable, OnLoad, OnPartDetached, OnSave, OnStart, OnVariantApplied, SetFlag ...
- auto: C / Rendering

## Assembly-CSharp/FlagMesh.cs
- L=28 kind=class type=FlagMesh bases=- anc=- ns=-
- unity(44): Transform=1, Renderer=1 | ksp: none | fan_in=2
- methods(0): 
- auto: C / Rendering

## Assembly-CSharp/FlagNameManager.cs
- L=81 kind=class type=FlagNameManager bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(4): DisplayNames, HasSizeNames, Initialize, SizeNames
- auto: A / Misc

## Assembly-CSharp/FlagOrientation.cs
- L=5 kind=enum type=FlagOrientation bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: C / Rendering

## Assembly-CSharp/FlagPoleFacility.cs
- L=49 kind=MonoBehaviour type=FlagPoleFacility bases=SpaceCenterBuilding anc=SpaceCenterBuilding/MonoBehaviour ns=-
- unity(50): Instantiate/Destroy=1, GetComponent=1, GameObject=1 | ksp: HighLogic=6, InputLockManager=5, Localizer=3, GameEvents=2 | fan_in=0
- methods(5): OnClicked, OnFlagCancel, OnFlagSelect, OnPopupWarningDismiss, SpawnBrowser
- auto: B / Scene

## Assembly-CSharp/FlagPoleLaunchpad.cs
- L=43 kind=MonoBehaviour type=FlagPoleLaunchpad bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(78): Lifecycle=2, Texture=1, Renderer=1, MonoBehaviour=1, Material=1, Animation=1 | ksp: GameEvents=4, HighLogic=2, GameDatabase=1 | fan_in=0
- methods(4): OnDestroy, OnFlagSelect, OnSceneChange, Start
- auto: B / Misc

## Assembly-CSharp/FlagSite.cs
- L=529 kind=PartModule type=FlagSite bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(66): Joint=9, GameObject=6, SetActive=5, Lifecycle=5, Transform=4, GetComponent=4 | ksp: Localizer=6, KSPField=6, InputLockManager=6, GameEvents=4, FlightGlobals=4 | fan_in=4
- methods(29): AcceptSiteRename, CancelSiteRename, CreateFlag, DismissSiteRename, FixedUpdate, LateUpdate, MakeInvisible, MakeVisible, OnDestroy, OnJointBreak, OnLoad, OnPlacementComplete, OnPlacementFail, OnSave, OnVesselPack, OnVesselUnpack, PickUp, ReadPlaque, RenameSite, ScanSurroundingTerrain, SetJoint, SetupFSM, Start, TakeDown ...
- auto: B / Misc

## Assembly-CSharp/FlightAutoSave.cs
- L=65 kind=MonoBehaviour type=FlightAutoSave bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(73): Lifecycle=3, Coroutine=3, MonoBehaviour=2, SetActive=1, Instantiate/Destroy=1, GameObject=1 | ksp: HighLogic=3, GameSettings=3, Localizer=2, FlightGlobals=2, Vessel=1 | fan_in=0
- methods(4): Awake, OnDestroy, PersistentSave, Start
- auto: B / Save

## Assembly-CSharp/FlightCoMTracker.cs
- L=105 kind=MonoBehaviour type=FlightCoMTracker bases=MonoBehaviour,ITargetable anc=MonoBehaviour ns=-
- unity(59): Transform=4, Coroutine=2, MonoBehaviour=1, Lifecycle=1, GetComponent=1, GameObject=1 | ksp: FlightGlobals=4, PartModule=2, Vessel=1, OrbitDriver=1, Orbit=1 | fan_in=2
- methods(15): Create, FixedUpdate, GetActiveTargetable, GetDisplayName, GetFwdVector, GetName, GetObtVelocity, GetOrbit, GetOrbitDriver, GetSrfVelocity, GetTargetingMode, GetTransform, GetVessel, MakeTarget, makeTgtWhenReady
- auto: B / FlightUI

## Assembly-CSharp/FlightCtrlState.cs
- L=250 kind=class type=FlightCtrlState bases=IConfigNode anc=IConfigNode ns=-
- unity(2): none | ksp: ConfigNode=2, GameSettings=1 | fan_in=8
- methods(11): CopyFrom, GetPYR, GetWheels, GetXYZ, Load, MaskedCopyFrom, Neutralize, NeutralizeAll, NeutralizeStick, ResetTrim, Save
- auto: A / Vessel

## Assembly-CSharp/FlightDriver.cs
- L=630 kind=MonoBehaviour type=FlightDriver bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(48): Transform=13, MonoBehaviour=9, Time.=5, GameObject=4, Lifecycle=3, Coroutine=2 | ksp: HighLogic=25, GameEvents=19, FlightGlobals=17, InputLockManager=7, Game=7 | fan_in=51
- methods(21): Awake, FinalizeAnalytics, FocusTargetVessel, InitAnalytics, OnApplicationFocus, OnCameraChange, OnConstructionModeChange, OnCrewBoardVessel, OnCrewOnEva, OnDestroy, OnModeChange, PostInit, RemoveSavedVessel, ReturnToEditor, RevertToLaunch, RevertToPrelaunch, SetPause, Start, StartAndFocusVessel, StartWithNewLaunch, setStartupNewVessel
- auto: B / Scene

## Assembly-CSharp/FlightEndModes.cs
- L=7 kind=enum type=FlightEndModes bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Vessel

## Assembly-CSharp/FlightEVA.cs
- L=296 kind=MonoBehaviour type=FlightEVA bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(85): Transform=25, GetComponent=12, GameObject=9, Rigidbody=8, Raycast/Query=4, Coroutine=4 | ksp: Part=15, FlightGlobals=10, KerbalEVA=9, GameEvents=9, ProtoCrewMember=7 | fan_in=4
- methods(12): Awake, HatchIsObstructed, HatchIsObstructedMore, OnDestroy, SpawnEVA, SpawnKerbalEVADelegate, SwitchToEVAVesselWhenReady, _Spawn, hatchInsideFairing, onEVAAborted, onGoForEVA, spawnEVA
- auto: B / Kerbals

## Assembly-CSharp/FlightEvents.cs
- L=22 kind=enum type=FlightEvents bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=14
- methods(0): 
- auto: A / Vessel

## Assembly-CSharp/FlightInputCallback.cs
- L=1 kind=delegate type=FlightInputCallback bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=5
- methods(0): 
- auto: A / Input

## Assembly-CSharp/FlightInputHandler.cs
- L=711 kind=MonoBehaviour type=FlightInputHandler bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(55): Time.=25, UnityEngine.UI=18, Lifecycle=5, MonoBehaviour=3, Input.=3, KeyCode=2 | ksp: GameSettings=115, InputLockManager=28, FlightGlobals=11, Localizer=5, Vessel=4 | fan_in=26
- methods(11): Awake, FixedUpdate, OnApplicationFocus, OnDestroy, OnVesselOverrideGroupChanged, ProcessAxis, ResumeVesselCtrlState, SetLaunchCtrlState, SetNeutralControls, Start, Update
- auto: C / Input

## Assembly-CSharp/FlightLog.cs
- L=367 kind=class type=FlightLog bases=- anc=- ns=-
- unity(0): none | ksp: ConfigNode=2, Orbit=1 | fan_in=18
- methods(14): AddEntry, AddEntryUnique, AddFlight, Clear, CreateCopy, GetDistinctTargets, GetDistinctTypes, GetEntries, GetFlights, HasEntry, Last, Load, MergeWith, Save
- auto: A / Vessel

## Assembly-CSharp/FlightLogger.cs
- L=401 kind=MonoBehaviour type=FlightLogger bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(51): Lifecycle=5, Time.=4, Input.=4, SendMessage/Invoke=3, KeyCode=3, Coroutine=2 | ksp: Localizer=30, GameEvents=30, FlightGlobals=12, Vessel=7, HighLogic=6 | fan_in=10
- methods(26): AttemptVesselSwitch, FixedUpdate, IgnoreGeeForces, LateUpdate, LogEvent, OnDestroy, ResumeGForceReading, Start, Update, checkMissionEnd, getMissionStats, onCollision, onCrash, onCrashSplashdown, onCrewBoardVessel, onCrewKilled, onCrewOnEva, onJointBreak, onLaunch, onOverG, onOverPressure, onOverheat, onSplashDamage, onStageSeparation ...
- auto: B / Vessel

## Assembly-CSharp/FlightOverlays.cs
- L=104 kind=MonoBehaviour type=FlightOverlays bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(34): Lifecycle=2, MonoBehaviour=1 | ksp: GameSettings=11, Localizer=10, PhysicsGlobals=8, FlightGlobals=3, HighLogic=1 | fan_in=1
- methods(3): CacheLocalStrings, Start, Update
- auto: B / FlightUI

## Assembly-CSharp/FlightReflectionProbe.cs
- L=261 kind=MonoBehaviour type=FlightReflectionProbe bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(64): QualitySettings/SystemInfo=4, Transform=3, Lifecycle=3, LayerMask=3, GetComponent=3, Renderer=2 | ksp: FlightGlobals=5, GameSettings=2, GameEvents=2, HighLogic=1 | fan_in=1
- methods(11): Enable, ForceRender, GetSkyColor, OnDestroy, OnSettingsUpdate, Sign, Spawn, Start, Update, VectorMatrixMultiplication, expScale
- auto: C / Rendering

## Assembly-CSharp/FlightState.cs
- L=192 kind=class type=FlightState bases=- anc=- ns=-
- unity(9): GameObject=1 | ksp: Vessel=5, ProtoVessel=5, FlightGlobals=4, ConfigNode=4, GameSettings=3 | fan_in=9
- methods(3): ContainsFlightID, Load, Save
- auto: A / Save

## Assembly-CSharp/FlightUIElements.cs
- L=11 kind=enum type=FlightUIElements bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / FlightUI

## Assembly-CSharp/FlightUIMode.cs
- L=8 kind=enum type=FlightUIMode bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=13
- methods(0): 
- auto: A / FlightUI

## Assembly-CSharp/FlightUIModeController.cs
- L=703 kind=MonoBehaviour type=FlightUIModeController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(46): Transform=14, GameObject=6, Lifecycle=4, UnityEngine.UI=3, Coroutine=2, Screen/Cursor=1 | ksp: GameSettings=33, InputLockManager=9, GameEvents=7, MapView=5 | fan_in=17
- methods(25): AddModeCheckCallback, AddModeSwitchCallback, Awake, ChangeTabState, FinishedAllTransitionsCallback, NextAlarmToggle, OnDestroy, OnDockingPress, OnManNodeSelected, OnManeuverPress, OnMapPress, OnMapViewEnter, OnMapViewExit, OnNextAlarmToggle, OnStagingPress, RemoveModeCheckCallback, RemoveModeSwitchCallback, SetLastMode, SetMode, SetNavBallHPos, SetUIElementScale, SetUIElementScale_ModeControls, Start, Update ...
- auto: C / FlightUI

## Assembly-CSharp/FlightVesselOverlays.cs
- L=282 kind=MonoBehaviour type=FlightVesselOverlays bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(69): GameObject=15, SetActive=6, Lifecycle=5, Transform=4, Instantiate/Destroy=4, Camera=3 | ksp: Vessel=16, FlightGlobals=9, GameEvents=4, HighLogic=3, FlightCamera=3 | fan_in=1
- methods(19): AddColMarker, AddComMarker, AddCotMarker, Awake, OnDestroy, OnDisable, OnVesselLoaded, OnVesselUnloaded, SetCamera, Start, TerminateColMarkers, TerminateComMarkers, TerminateCotMarkers, ToggleCoL, ToggleCoM, ToggleCoT, Update, ValidVesselOverride, ValidVesselType
- auto: B / FlightUI

## Assembly-CSharp/FloatArrayMaterialProperty.cs
- L=17 kind=class type=FloatArrayMaterialProperty bases=MaterialPropertyExtensions.MaterialProperty anc=MaterialProperty ns=-
- unity(29): Material=1 | ksp: none | fan_in=0
- methods(1): Apply
- auto: C / Rendering

## Assembly-CSharp/FloatCurve.cs
- L=191 kind=class type=FloatCurve bases=IConfigNode anc=IConfigNode ns=-
- unity(28): AnimationCurve=7, Animation=2 | ksp: FloatCurve=3, ConfigNode=2 | fan_in=32
- methods(5): Add, Evaluate, FindMinMaxValue, Load, Save
- auto: A / Core

## Assembly-CSharp/FloatingOrigin.cs
- L=368 kind=MonoBehaviour type=FloatingOrigin bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(58): ParticleSystem=9, Transform=8, GameObject=4, Lifecycle=3, Rigidbody=2, Time.=1 | ksp: Vector3d=30, FlightGlobals=18, GameEvents=7, TimeWarp=4, Vessel=3 | fan_in=17
- methods(14): Awake, FixedUpdate, OnCameraChange, OnDestroy, OnLevelLoaded, OnMapExited, RegisterParticleSystem, ResetOffset, ResetTerrainShaderOffset, SetOffset, SetOutOfFrameOffset, SetSafeToEngage, UnregisterParticleSystem, setOffset
- auto: B / Physics

## Assembly-CSharp/FloatMaterialProperty.cs
- L=17 kind=class type=FloatMaterialProperty bases=MaterialPropertyExtensions.MaterialProperty anc=MaterialProperty ns=-
- unity(44): Shader=1, Material=1 | ksp: none | fan_in=0
- methods(1): Apply
- auto: C / Rendering

## Assembly-CSharp/FloatTypeParser.cs
- L=20 kind=class type=FloatTypeParser bases=ITypeParser anc=ITypeParser ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(2): Convert, Parse
- auto: A / Modding

## Assembly-CSharp/FollowRot.cs
- L=24 kind=MonoBehaviour type=FollowRot bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(83): Transform=6, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(1): LateUpdate
- auto: C / Camera

## Assembly-CSharp/FontLoader.cs
- L=173 kind=MonoBehaviour type=FontLoader bases=LoadingSystem anc=LoadingSystem/MonoBehaviour ns=-
- unity(72): Coroutine=9, TMPro=4, AssetBundle=4, WWW/WebRequest=1, Texture=1, Material=1 | ksp: Localizer=1 | fan_in=0
- methods(12): AddGameSubFont, AddMenuSubFont, ChangeFontLanguage, FindFonts, IsReady, LoadBundle, LoadFonts, LoadWeight, ProgressFraction, ProgressTitle, StartLoad, SwitchFontLanguage
- auto: C / Localization

## Assembly-CSharp/FoRModes.cs
- L=11 kind=enum type=FoRModes bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / Orbit

## Assembly-CSharp/FuelFlowOverlay.cs
- L=471 kind=MonoBehaviour type=FuelFlowOverlay bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(79): GameObject=30, Transform=23, Instantiate/Destroy=9, TMPro=8, Coroutine=8, LayerMask=4 | ksp: Part=28, GameEvents=17, EditorLogic=4, HighLogic=2, ShipConstruct=1 | fan_in=2
- methods(21): Awake, Calculate, ClearOverlay, Connect, CreateText, DespawnEvent, InternalSpawn, OnDestroy, OnPartDestroyed, RedrawEvent, RedrawOverlay, RespawnEvent, RespawnOverlay, SpawnOverlay, Update, UpdateDaemon, UpdateLine, UpdateText, isConsumer, isConsumerOrProvider, isProvider
- auto: C / Resources

## Assembly-CSharp/Funding.cs
- L=207 kind=ScenarioModule type=Funding bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(27): Lifecycle=1, GameObject=1 | ksp: GameEvents=17, HighLogic=8, ProtoPartSnapshot=2, Funding=2, ConfigNode=2 | fan_in=33
- methods(13): AddFunds, CanAfford, OnAwake, OnCurrenciesModified, OnDestroy, OnLoad, OnSave, SetFunds, onCrewHired, onPartPurchased, onUpgradePurchased, onVesselRecoveryProcessing, onVesselRollout
- auto: A / Career

## Assembly-CSharp/FundsWidget.cs
- L=38 kind=MonoBehaviour type=FundsWidget bases=CurrencyWidget anc=CurrencyWidget/MonoBehaviour ns=-
- unity(45): Lifecycle=2 | ksp: Funding=3, GameEvents=2, HighLogic=1 | fan_in=0
- methods(5): Awake, DelayedStart, OnAboutToStart, OnDestroy, onFundsChanged
- auto: B / Career

## Assembly-CSharp/FXCamera.cs
- L=295 kind=MonoBehaviour type=FXCamera bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(85): Shader=23, Transform=9, Texture=7, Instantiate/Destroy=7, Camera=5, Lifecycle=4 | ksp: GameEvents=8, FlightCamera=6, GameSettings=2, Vessel=1, FlightGlobals=1 | fan_in=2
- methods(11): ApplyObliqueness, Awake, LateUpdate, OnCameraChange, OnCameraDistanceAdjustedToFitVessel, OnDestroy, OnGUI, OnPreRender, OnSceneSwitch, OnSettingsUpdate, Start
- auto: C / Camera

## Assembly-CSharp/FXCurve.cs
- L=152 kind=class type=FXCurve bases=- anc=- ns=-
- unity(28): AnimationCurve=6, Animation=2 | ksp: ConfigNode=2 | fan_in=10
- methods(6): AddKeyframe, CompileCurve, Load, Save, Value, float
- auto: C / Rendering

## Assembly-CSharp/FXDepthCamera.cs
- L=53 kind=MonoBehaviour type=FXDepthCamera bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(74): Lifecycle=3, Shader=1, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1, Camera=1 | ksp: GameEvents=4, FlightCamera=3, Vessel=1 | fan_in=1
- methods(6): LateUpdate, OnCameraDistanceAdjustedToFitVessel, OnDestroy, OnPreRender, OnSceneSwitch, Start
- auto: C / Camera

## Assembly-CSharp/FXFadeByPressure.cs
- L=47 kind=MonoBehaviour type=FXFadeByPressure bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(85): ParticleSystem=5, Lifecycle=2, GetComponent=2, Transform=1, MonoBehaviour=1 | ksp: FlightGlobals=2, HighLogic=1 | fan_in=0
- methods(2): Start, Update
- auto: B / Misc

## Assembly-CSharp/FXGroup.cs
- L=363 kind=class type=FXGroup bases=- anc=- ns=-
- unity(43): ParticleSystem=10, Light=4, Coroutine=4, Audio=4, Time.=3 | ksp: GameSettings=1 | fan_in=12
- methods(10): Burst, SetLatch, SetPower, SetPowerLatch, SetVisualMinMax, Unlatch, VolumeFade, begin, setActive, setActiveWithAudioFade
- auto: C / Rendering

## Assembly-CSharp/FXModuleAnimateRCS.cs
- L=174 kind=PartModule type=FXModuleAnimateRCS bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(59): Animation=7, Transform=2, Lifecycle=1, LayerMask=1 | ksp: KSPField=8, TimeWarp=2, HighLogic=2, ConfigNode=2, PartModule=1 | fan_in=0
- methods(4): FixedUpdate, OnLoad, OnSave, OnStart
- auto: C / Rendering

## Assembly-CSharp/FXModuleAnimateThrottle.cs
- L=192 kind=PartModule type=FXModuleAnimateThrottle bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(56): Animation=9, Lifecycle=1, LayerMask=1 | ksp: KSPField=16, TimeWarp=3, HighLogic=2, PartModule=1 | fan_in=0
- methods(3): FixedUpdate, OnInventoryModeDisable, OnStart
- auto: B / PartModules-Engines

## Assembly-CSharp/FXModuleConstrainPosition.cs
- L=137 kind=PartModule type=FXModuleConstrainPosition bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(55): Transform=8, Lifecycle=3 | ksp: ConfigNode=4, KSPField=3, PartModule=1 | fan_in=0
- methods(8): FixedUpdate, LateUpdate, Load, OnLoad, Save, SetupObjects, Track, Update
- auto: C / Rendering

## Assembly-CSharp/FXModuleLookAtConstraint.cs
- L=164 kind=PartModule type=FXModuleLookAtConstraint bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(45): Transform=5, Lifecycle=3 | ksp: ConfigNode=4, KSPField=2, PartModule=1 | fan_in=0
- methods(11): FixedUpdate, LateUpdate, Load, OnConstructionModeFixedUpdate, OnConstructionModeUpdate, OnLoad, OnStart, Save, SetupObjects, Track, Update
- auto: C / Rendering

## Assembly-CSharp/FXModuleThrottleEffects.cs
- L=72 kind=PartModule type=FXModuleThrottleEffects bases=PartModuleFXSetter anc=PartModuleFXSetter/PartModule/MonoBehaviour ns=-
- unity(46): Lifecycle=2, Animation=1 | ksp: KSPField=8, HighLogic=2, TimeWarp=1 | fan_in=0
- methods(3): FixedUpdate, OnStart, Update
- auto: A / PartModules-Engines

## Assembly-CSharp/FXMonger.cs
- L=630 kind=MonoBehaviour type=FXMonger bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(78): GameObject=41, Transform=18, Audio=18, GetComponent=14, ParticleSystem=10, SetActive=5 | ksp: Vector3d=16, GameSettings=11, FlightGlobals=11, Part=5 | fan_in=7
- methods(19): AddFireworkFX, Awake, Explode, ExplodeWithDebris, LateUpdate, OffsetPositions, OnDestroy, ROCExplode, RemoveFXOjbect, RemoveFireworkFX, SpawnDebrisExplosions, Splash, explode, explosionTest, offsetFireworks, offsetPositions, rocexplode, splash, splashMongered
- auto: C / Rendering

## Assembly-CSharp/FXObject.cs
- L=21 kind=class type=FXObject bases=- anc=- ns=-
- unity(67): ParticleSystem=2, GameObject=2, GetComponent=1, Audio=1 | ksp: none | fan_in=2
- methods(0): 
- auto: C / Rendering

## Assembly-CSharp/FXObjectPhoneHome.cs
- L=11 kind=MonoBehaviour type=FXObjectPhoneHome bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(50): MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=1
- methods(1): OnDestroy
- auto: C / Rendering

## Assembly-CSharp/FXPrefab.cs
- L=24 kind=MonoBehaviour type=FXPrefab bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(87): ParticleSystem=3, Lifecycle=3, MonoBehaviour=1, GetComponent=1 | ksp: FloatingOrigin=2 | fan_in=3
- methods(3): Awake, OnDisable, OnEnable
- auto: C / Rendering

## Assembly-CSharp/GalaxyCameraControl.cs
- L=30 kind=MonoBehaviour type=GalaxyCameraControl bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(77): Lifecycle=2, Camera=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(3): Awake, OnDestroy, SetFoV
- auto: C / Rendering

## Assembly-CSharp/GalaxyCubeControl.cs
- L=150 kind=MonoBehaviour type=GalaxyCubeControl bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(80): Transform=7, Lifecycle=5, GameObject=4, Renderer=2, Raycast/Query=2, Material=2 | ksp: Vector3d=3, FlightGlobals=3, HighLogic=2, ScaledSpace=1, QuaternionD=1 | fan_in=1
- methods(7): Awake, LateUpdate, LineOfSightToSun, OnDestroy, SetEnabled, Start, Update
- auto: C / Rendering

## Assembly-CSharp/Game.cs
- L=800 kind=class type=Game bases=- anc=- ns=-
- unity(8): Lifecycle=1, Instantiate/Destroy=1 | ksp: HighLogic=22, ConfigNode=20, Game=8, GameEvents=7, GameDatabase=7 | fan_in=103
- methods(17): AddProtoScenarioModule, AddVessel, DestroyVessel, GetCloneOf, GetCometNumberedName, IsResumable, Load, LoadCometNames, MergeLoaderInfo, PopulateGameSeed, PopulateROCGameSeed, RemoveProtoScenarioModule, Save, SaveCometNames, Start, ToString, Updated
- auto: A / Save

## Assembly-CSharp/GameBackup.cs
- L=29 kind=class type=GameBackup bases=- anc=- ns=-
- unity(0): none | ksp: ConfigNode=2, Game=1 | fan_in=2
- methods(0): 
- auto: A / Save

## Assembly-CSharp/GameEventsBase.cs
- L=4 kind=class type=GameEventsBase bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=12
- methods(0): 
- auto: A / Core

## Assembly-CSharp/GameObjectExtension.cs
- L=192 kind=static class type=GameObjectExtension bases=- anc=- ns=-
- unity(72): GameObject=23, Transform=18, GetComponent=10, Renderer=2, Material=2, LayerMask=2 | ksp: none | fan_in=0
- methods(10): AddOrGetComponent, DestroyGameObject, DestroyGameObjectImmediate, GetChild, GetComponentCached, GetComponentIndex, GetComponentOnParent, GetComponentUpwards, SetLayerRecursive, SetTagsRecursive
- auto: B / Core

## Assembly-CSharp/GamePersistence.cs
- L=372 kind=class type=GamePersistence bases=- anc=- ns=-
- unity(9): Application.=1 | ksp: Game=23, HighLogic=7, GameSettings=6, ConfigNode=6, Localizer=5 | fan_in=42
- methods(7): CreateNewGame, LoadGame, LoadGameCfg, LoadSFSFile, SaveGame, UpdateScenarioModules, saveCompareToDate
- auto: A / Save

## Assembly-CSharp/GameplaySettingsScreen.cs
- L=340 kind=class type=GameplaySettingsScreen bases=DialogGUIVerticalLayout,ISettings anc=DialogGUIVerticalLayout/DialogGUILayoutBase/DialogGUIBase ns=-
- unity(7): Time.=1, Application.=1 | ksp: GameSettings=78, Localizer=46, HighLogic=1, GameEvents=1 | fan_in=1
- methods(11): ApplyNavBallHPos, ApplySettings, ApplyUIElementScale, ApplyUIScalingAndAdjustments, DrawMiniSettings, DrawSettings, GetName, GetSettings, OnUpdate, ResetUIAdjustments, ResetUIScaling
- auto: C / UI

## Assembly-CSharp/GameScenes.cs
- L=14 kind=enum type=GameScenes bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=194
- methods(0): 
- auto: A / Scene

## Assembly-CSharp/GameVariables.cs
- L=681 kind=MonoBehaviour type=GameVariables bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(25): Lifecycle=2, AnimationCurve=2, MonoBehaviour=1, GameObject=1 | ksp: Contract=24, Vessel=16, HighLogic=14, Localizer=9, ProtoCrewMember=3 | fan_in=52
- methods(52): Awake, EVAIsPossible, GetActiveContractsLimit, GetActiveCrewLimit, GetActiveStrategyLimit, GetContractDestinationWeight, GetContractFundsAdvanceFactor, GetContractFundsCompletionFactor, GetContractFundsFailureFactor, GetContractLevelLimit, GetContractPrestigeFactor, GetContractReputationCompletionFactor, GetContractReputationFailureFactor, GetContractScienceCompletionFactor, GetCraftMassLimit, GetCraftSizeLimit, GetCrewLevelLimit, GetDSNPowerCurve, GetDSNRange, GetDSNRangeCurve, GetDSNScienceCurve, GetDataToScienceRatio, GetEVALockedReason, GetExperimentLevel ...
- auto: B / Career

## Assembly-CSharp/GAPCelestialBodyCamera.cs
- L=286 kind=MonoBehaviour type=GAPCelestialBodyCamera bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(67): Transform=18, AnimationCurve=6, Time.=5, Camera=3, Instantiate/Destroy=2, GameObject=2 | ksp: Vector3d=17, CelestialBody=5, QuaternionD=1, FlightGlobals=1 | fan_in=1
- methods(13): Destroy, Disable, Enable, GetLightRot, GetLocalSpaceCamera, OverrideDistance, OverridePosition, RefreshTarget, SetBoundries, SetLightCycle, SetPosition, UpdateMouse, UpdateTransform
- auto: B / DLC-MakingHistory

## Assembly-CSharp/GAPCelestialBodyCollisionSphere.cs
- L=23 kind=MonoBehaviour type=GAPCelestialBodyCollisionSphere bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(96): Transform=3, GetComponent=3, Mesh=2, Collider=2, Renderer=1, MonoBehaviour=1 | ksp: Vector3d=1, CelestialBody=1 | fan_in=1
- methods(2): Setup, ToggleVisibility
- auto: B / DLC-MakingHistory

## Assembly-CSharp/GAPKerbalAnimation.cs
- L=78 kind=MonoBehaviour type=GAPKerbalAnimation bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(81): Animation=7, Coroutine=5, Lifecycle=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(3): OnDestroy, PlayAnimations, Start
- auto: B / DLC-MakingHistory

## Assembly-CSharp/GAPProceduralAsteroid.cs
- L=39 kind=MonoBehaviour type=GAPProceduralAsteroid bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(69): Transform=2, Resources.=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=2
- methods(3): RangefinderGeneric, Setup, Start
- auto: B / DLC-MakingHistory

## Assembly-CSharp/GAPProceduralComet.cs
- L=36 kind=MonoBehaviour type=GAPProceduralComet bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(70): Transform=2, Resources.=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=2
- methods(3): RangefinderGeneric, Setup, Start
- auto: B / DLC-MakingHistory

## Assembly-CSharp/GAPUtil_BiSelector.cs
- L=152 kind=MonoBehaviour type=GAPUtil_BiSelector bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(92): Texture=18, UnityEngine.UI=9, GetComponent=6, GameObject=5, Instantiate/Destroy=3, Transform=2 | ksp: none | fan_in=3
- methods(9): AddDivider, AddSidebarButton, AddSidebarGAPButton, Awake, ClearSidebar, GetSidebarButton, GetSidebarGapButton, SetFooterText, SetTitleText
- auto: C / DLC-MakingHistory

## Assembly-CSharp/GAPUtil_DynamicCylinder.cs
- L=146 kind=MonoBehaviour type=GAPUtil_DynamicCylinder bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): Transform=5, Mesh=4, Renderer=2, Material=2, GetComponent=2, GameObject=2 | ksp: none | fan_in=1
- methods(3): CreateMesh, GetPivotPosition, UpdateValues
- auto: C / DLC-MakingHistory

## Assembly-CSharp/GAPVesselCamera.cs
- L=405 kind=MonoBehaviour type=GAPVesselCamera bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(79): Input.=19, Time.=16, Transform=11, KeyCode=10, GetComponent=3, Camera=2 | ksp: GameSettings=29, ShipConstruct=2 | fan_in=2
- methods(14): FocusPoint, FocusVessel, PlaceCamera, ResetCamera, Rotate, Setup, Start, Update, UpdateCameraControls, UpdateSPHCamera, UpdateSPHCameraControls, UpdateVABCamera, UpdateVABCameraControls, Zoom
- auto: B / DLC-MakingHistory

## Assembly-CSharp/GasGiantMaterialControls.cs
- L=251 kind=MonoBehaviour type=GasGiantMaterialControls bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(90): Material=29, Shader=19, Texture=9, Lifecycle=3, Renderer=2, MonoBehaviour=1 | ksp: Planetarium=1 | fan_in=0
- methods(5): Awake, OnEnable, Update, UpdateMaterialControlSettings, UpdateTimeDivisors
- auto: C / Rendering

## Assembly-CSharp/Gauge.cs
- L=141 kind=MonoBehaviour type=Gauge bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(68): GetComponent=5, Audio=5, Time.=3, Lifecycle=2, Transform=1, MonoBehaviour=1 | ksp: HighLogic=1 | fan_in=1
- methods(4): Start, Update, setValue, startSound
- auto: B / FlightUI

## Assembly-CSharp/GaugeLEDRange.cs
- L=25 kind=class type=GaugeLEDRange bases=- anc=- ns=-
- unity(35): Audio=1 | ksp: none | fan_in=2
- methods(0): 
- auto: A / FlightUI

## Assembly-CSharp/GizmoDrawer.cs
- L=5 kind=MonoBehaviour type=GizmoDrawer bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(36): MonoBehaviour=1 | ksp: none | fan_in=0
- methods(0): 
- auto: C / Debug

## Assembly-CSharp/GizmoDrawUtil.cs
- L=59 kind=class type=GizmoDrawUtil bases=- anc=- ns=-
- unity(62): none | ksp: none | fan_in=2
- methods(3): DrawArc, DrawCrosshairs, DrawReach
- auto: C / Debug

## Assembly-CSharp/GroundCrew.cs
- L=87 kind=MonoBehaviour type=GroundCrew bases=SpaceCenterCrew anc=SpaceCenterCrew/MonoBehaviour ns=-
- unity(34): Lifecycle=2 | ksp: none | fan_in=1
- methods(3): SetAnimation, Start, Update
- auto: B / Kerbals

## Assembly-CSharp/GUILayoutx.cs
- L=86 kind=class type=GUILayoutx bases=- anc=- ns=-
- unity(56): IMGUI=9 | ksp: none | fan_in=0
- methods(2): DoubleClickCallback, SelectionList
- auto: C / UI

## Assembly-CSharp/GUIUtil.cs
- L=293 kind=class type=GUIUtil bases=- anc=- ns=-
- unity(77): IMGUI=72, UnityEngine.UI=7, Screen/Cursor=2 | ksp: none | fan_in=1
- methods(17): ClearEditableFieldFlags, DrawEnumSelectionPane, EditableBoolField, EditableDoubleField, EditableEnumField, EditableField, EditableFloatField, EditableIntField, EditableStringField, EditableTextArea, EnumFlagField, LabelHorizontalSlider, LabelVerticalSlider, ProgressBar, ProgressBarVert, ProgressBarVertical, ScreenCenteredRect
- auto: C / UI

## Assembly-CSharp/HarvestTypes.cs
- L=13 kind=enum type=HarvestTypes bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=9
- methods(0): 
- auto: A / Resources

## Assembly-CSharp/HelixGauge.cs
- L=94 kind=MonoBehaviour type=HelixGauge bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(94): SetActive=16, GameObject=16, Transform=8, Lifecycle=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(3): LateUpdate, Start, UpdateGauge
- auto: B / FlightUI

## Assembly-CSharp/HelmetSuitPickerWindow.cs
- L=709 kind=MonoBehaviour type=HelmetSuitPickerWindow bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): GameObject=22, Transform=16, Instantiate/Destroy=9, Shader=8, Material=6, Texture=5 | ksp: ProtoCrewMember=18, GameEvents=15, Localizer=7, HighLogic=5, GameSettings=5 | fan_in=1
- methods(29): CheckStock, CleanWindow, CloseWindow, CreateThumbnailCamera, GetAvailableSuits, InitFromProtoCrewData, InitSelection, LocalizedSuitText, OnACDespawn, OnACSpawn, OnCloseClick, OnComboSelected, OnDestroy, OnHelmetNeckringSelection, OnSceneSwitch, OpenSuitLightPicker, SetDefaultCombo, SetExtraCombo, SetupComboButton, SetupComboSelector, SetupExtraCombos, SetupGlowColor, SetupHelmetNeckringButton, SetupPreview ...
- auto: C / Kerbals

## Assembly-CSharp/HexColorField.cs
- L=88 kind=MonoBehaviour type=HexColorField bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(63): TMPro=3, Lifecycle=2, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=0
- methods(6): ColorToHex, HexToColor, OnDestroy, Start, UpdateColor, UpdateHex
- auto: C / UI

## Assembly-CSharp/HierarchyUtil.cs
- L=140 kind=static class type=HierarchyUtil bases=- anc=- ns=-
- unity(42): Transform=12 | ksp: Part=10 | fan_in=5
- methods(7): CompareHierarchyDepths, CompileID, GetHierarchyDepth, compileID, isAncestor, isDescendant, isSameLevel
- auto: B / Core

## Assembly-CSharp/HighLogic.cs
- L=355 kind=MonoBehaviour type=HighLogic bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(82): SceneManager=14, Input.=14, KeyCode=12, Coroutine=10, Lifecycle=4, Time.=3 | ksp: GameEvents=7, MissionSystem=4, Planetarium=2, HighLogic=2, GameSettings=2 | fan_in=466
- methods(16): Awake, FireLoadedEvent, FireLoadedEventGUIReady, GetLoadedGameSceneFromBuildIndex, LoadScene, LoadSceneFromBundle, OnApplicationFocus, OnDestroy, OnLevelLoaded, OnSceneLoaded, SetLoadSceneEventsAndFlags, Start, Update, bufferedLoad, printCurrentGame, setLevelFlags
- auto: B / Scene

## Assembly-CSharp/HSVChangedEvent.cs
- L=5 kind=class type=HSVChangedEvent bases=UnityEvent<float,float,float> anc=UnityEvent ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/HsvColor.cs
- L=56 kind=struct type=HsvColor bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=3
- methods(1): ToString
- auto: C / UI

## Assembly-CSharp/HSVUtil.cs
- L=104 kind=static class type=HSVUtil bases=- anc=- ns=-
- unity(3): none | ksp: none | fan_in=4
- methods(2): ConvertHsvToRgb, ConvertRgbToHsv
- auto: C / UI

## Assembly-CSharp/IActivateOnDecouple.cs
- L=4 kind=interface type=IActivateOnDecouple bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / PartModules-Structural

## Assembly-CSharp/IActiveJointHost.cs
- L=12 kind=interface type=IActiveJointHost bases=- anc=- ns=-
- unity(21): Transform=1 | ksp: Part=1 | fan_in=4
- methods(0): 
- auto: A / DLC-BreakingGround

## Assembly-CSharp/IAirstreamShield.cs
- L=8 kind=interface type=IAirstreamShield bases=- anc=- ns=-
- unity(0): none | ksp: Vessel=1, Part=1 | fan_in=3
- methods(0): 
- auto: A / Aero

## Assembly-CSharp/IAnalyticOverheatModule.cs
- L=6 kind=interface type=IAnalyticOverheatModule bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Thermal

## Assembly-CSharp/IAnalyticPreview.cs
- L=6 kind=interface type=IAnalyticPreview bases=- anc=- ns=-
- unity(0): none | ksp: FlightIntegrator=1 | fan_in=2
- methods(0): 
- auto: A / Thermal

## Assembly-CSharp/IAnalyticTemperatureModifier.cs
- L=8 kind=interface type=IAnalyticTemperatureModifier bases=- anc=- ns=-
- unity(0): none | ksp: FlightIntegrator=1 | fan_in=2
- methods(0): 
- auto: A / Thermal

## Assembly-CSharp/IAnimatedModule.cs
- L=10 kind=interface type=IAnimatedModule bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=7
- methods(0): 
- auto: A / Part

## Assembly-CSharp/IAxisFieldLimits.cs
- L=23 kind=interface type=IAxisFieldLimits bases=- anc=- ns=-
- unity(12): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / Misc

## Assembly-CSharp/ICMTweakTarget.cs
- L=14 kind=interface type=ICMTweakTarget bases=- anc=- ns=-
- unity(43): Transform=1, Collider=1 | ksp: none | fan_in=2
- methods(0): 
- auto: A / Misc

## Assembly-CSharp/ICollisionEvents.cs
- L=12 kind=interface type=ICollisionEvents bases=- anc=- ns=-
- unity(73): Collision/Force=6, MonoBehaviour=1 | ksp: none | fan_in=3
- methods(0): 
- auto: A / Physics

## Assembly-CSharp/ICommAntenna.cs
- L=22 kind=interface type=ICommAntenna bases=- anc=- ns=-
- unity(0): none | ksp: ProtoPartModuleSnapshot=2 | fan_in=4
- methods(0): 
- auto: A / CommNet

## Assembly-CSharp/IConfigNode.cs
- L=6 kind=interface type=IConfigNode bases=- anc=- ns=-
- unity(0): none | ksp: ConfigNode=2 | fan_in=88
- methods(0): 
- auto: A / Save

## Assembly-CSharp/IConstruction.cs
- L=8 kind=interface type=IConstruction bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=9
- methods(0): 
- auto: A / Part

## Assembly-CSharp/IContractObjectiveModule.cs
- L=6 kind=interface type=IContractObjectiveModule bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=7
- methods(0): 
- auto: A / Misc

## Assembly-CSharp/IDateFormatterExtensions.cs
- L=22 kind=static class type=IDateFormatterExtensions bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(2): GetDateComponents, GetTimeComponents
- auto: A / Core

## Assembly-CSharp/IDateTimeFormatter.cs
- L=34 kind=interface type=IDateTimeFormatter bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / Core

## Assembly-CSharp/IDiscoverable.cs
- L=18 kind=interface type=IDiscoverable bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=13
- methods(0): 
- auto: A / Celestials

## Assembly-CSharp/IDynamicCargoOccluder.cs
- L=4 kind=interface type=IDynamicCargoOccluder bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / PartModules-Structural

## Assembly-CSharp/IEngineStatus.cs
- L=10 kind=interface type=IEngineStatus bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=6
- methods(0): 
- auto: A / PartModules-Engines

## Assembly-CSharp/IGameCamera.cs
- L=16 kind=MonoBehaviour type=IGameCamera bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(36): MonoBehaviour=1 | ksp: none | fan_in=3
- methods(3): DisableCamera, EnableCamera, ResetCamera
- auto: C / Camera

## Assembly-CSharp/IJointLockState.cs
- L=4 kind=interface type=IJointLockState bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=6
- methods(0): 
- auto: A / DLC-BreakingGround

## Assembly-CSharp/IKSPCamera.cs
- L=25 kind=interface type=IKSPCamera bases=- anc=- ns=-
- unity(38): Transform=2 | ksp: none | fan_in=6
- methods(0): 
- auto: C / Camera

## Assembly-CSharp/ILiftProvider.cs
- L=8 kind=interface type=ILiftProvider bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / Aero

## Assembly-CSharp/IModular.cs
- L=10 kind=interface type=IModular bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: A / Modding

## Assembly-CSharp/IModuleInfo.cs
- L=12 kind=interface type=IModuleInfo bases=- anc=- ns=-
- unity(3): none | ksp: none | fan_in=14
- methods(0): 
- auto: A / Part

## Assembly-CSharp/IMouseEvents.cs
- L=16 kind=interface type=IMouseEvents bases=- anc=- ns=-
- unity(21): MonoBehaviour=1 | ksp: none | fan_in=6
- methods(0): 
- auto: C / UI

## Assembly-CSharp/IMultipleDragCube.cs
- L=10 kind=interface type=IMultipleDragCube bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=13
- methods(0): 
- auto: A / Aero

## Assembly-CSharp/InputBinding.cs
- L=66 kind=class type=InputBinding bases=IConfigNode,ICloneable anc=IConfigNode ns=-
- unity(0): none | ksp: ConfigNode=2, InputLockManager=1, HighLogic=1 | fan_in=5
- methods(7): Clone, CompareSwitchState, IsLocked, IsNeutral, IsUnlocked, Load, Save
- auto: A / Input

## Assembly-CSharp/InputBindingModes.cs
- L=13 kind=enum type=InputBindingModes bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=7
- methods(0): 
- auto: A / Input

## Assembly-CSharp/InputDevices.cs
- L=77 kind=class type=InputDevices bases=IConfigNode anc=IConfigNode ns=-
- unity(24): Input.=1, Application.=1 | ksp: ConfigNode=3 | fan_in=3
- methods(6): GetDeviceIndex, GetDeviceListLength, InitializeDevices, Load, Save, TrimDeviceName
- auto: A / Input

## Assembly-CSharp/InputFieldHandler.cs
- L=84 kind=class type=InputFieldHandler bases=- anc=- ns=-
- unity(28): TMPro=2, Lifecycle=1 | ksp: InputLockManager=2 | fan_in=2
- methods(9): GetFormattedValue, GetValueDelegate, OnDestroy, OnFieldDeselected, OnFieldSelected, OnValueChanged, ParseText, SetValueDelegate, UpdateField
- auto: A / Input

## Assembly-CSharp/InputLockManager.cs
- L=156 kind=static class type=InputLockManager bases=- anc=- ns=-
- unity(15): IMGUI=2 | ksp: GameEvents=11, Localizer=2, InputLockManager=1 | fan_in=165
- methods(14): ClearControlLocks, ControlLocks, DebugLockStack, GetControlLock, IsAllLocked, IsAnyUnlocked, IsLocked, IsLocking, IsUnlocked, IsUnlocking, PrintLockStack, RecalcMask, RemoveControlLock, SetControlLock
- auto: A / Input

## Assembly-CSharp/InspectorKeyValuePair.cs
- L=25 kind=class type=InspectorKeyValuePair bases=- anc=- ns=-
- unity(12): none | ksp: none | fan_in=0
- methods(1): ToKeyValuePair
- auto: C / Debug

## Assembly-CSharp/IntermediateTutorial.cs
- L=139 kind=ScenarioModule type=IntermediateTutorial bases=METutorialScenario anc=METutorialScenario/TutorialScenario/ScenarioModule ns=-
- unity(15): none | ksp: GameEvents=2, ConfigNode=2 | fan_in=11
- methods(13): CreateTutorialPages, DeleteTutorial, GetNextTutorialName, OnContinueButtonClick, OnDoneButtonClick, OnExitCancelSave, OnExitMissionEditorScreen, OnLoad, OnNodeChange, OnSave, OnTutorialSetup, SetupEditorForTutorial, SetupEditorToDefault
- auto: A / Tutorials

## Assembly-CSharp/IntermediateTutorialPageStep.cs
- L=19 kind=class type=IntermediateTutorialPageStep bases=TutorialPageConfig,IDisposable anc=TutorialPageConfig ns=-
- unity(0): none | ksp: none | fan_in=10
- methods(1): Dispose
- auto: A / Tutorials

## Assembly-CSharp/InternalAltimeterThreeHands.cs
- L=78 kind=MonoBehaviour type=InternalAltimeterThreeHands bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(67): Transform=9, Time.=1 | ksp: KSPField=5 | fan_in=0
- methods(2): OnAwake, OnUpdate
- auto: B / Kerbals

## Assembly-CSharp/InternalAnimationAirSpeed.cs
- L=54 kind=MonoBehaviour type=InternalAnimationAirSpeed bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(47): Animation=2 | ksp: KSPField=4 | fan_in=0
- methods(2): OnAwake, OnUpdate
- auto: B / Kerbals

## Assembly-CSharp/InternalAnimationAltimeter.cs
- L=44 kind=MonoBehaviour type=InternalAnimationAltimeter bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(51): Animation=2 | ksp: KSPField=3 | fan_in=0
- methods(2): OnAwake, OnUpdate
- auto: B / Kerbals

## Assembly-CSharp/InternalAtmosphereDepth.cs
- L=83 kind=MonoBehaviour type=InternalAtmosphereDepth bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(46): Transform=3, Time.=1 | ksp: KSPField=7, FlightGlobals=3, CelestialBody=1 | fan_in=0
- methods(2): OnAwake, OnUpdate
- auto: B / Kerbals

## Assembly-CSharp/InternalAxisIndicatorPitch.cs
- L=44 kind=MonoBehaviour type=InternalAxisIndicatorPitch bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(60): Transform=3, Time.=1 | ksp: KSPField=4 | fan_in=0
- methods(2): OnAwake, OnUpdate
- auto: B / Kerbals

## Assembly-CSharp/InternalAxisIndicatorRoll.cs
- L=44 kind=MonoBehaviour type=InternalAxisIndicatorRoll bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(60): Transform=3, Time.=1 | ksp: KSPField=4 | fan_in=0
- methods(2): OnAwake, OnUpdate
- auto: B / Kerbals

## Assembly-CSharp/InternalAxisIndicatorYaw.cs
- L=44 kind=MonoBehaviour type=InternalAxisIndicatorYaw bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(60): Transform=3, Time.=1 | ksp: KSPField=4 | fan_in=0
- methods(2): OnAwake, OnUpdate
- auto: B / Kerbals

## Assembly-CSharp/InternalButton.cs
- L=169 kind=MonoBehaviour type=InternalButton bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(60): Time.=4, Input.=4, GameObject=3, Coroutine=3, GetComponent=2, MonoBehaviour=1 | ksp: none | fan_in=4
- methods(17): Create, InternalButtonDelegate, OnDoubleTap, OnDown, OnDrag, OnExit, OnMouseDoubleTap, OnMouseDown, OnMouseDrag, OnMouseExit, OnMouseOver, OnMouseTap, OnMouseUp, OnOver, OnTap, OnUp, TapRoutine
- auto: B / Kerbals

## Assembly-CSharp/InternalButtonLight.cs
- L=107 kind=MonoBehaviour type=InternalButtonLight bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(81): Light=6, GetComponent=3, Shader=2, Renderer=2, Material=2, Collider=2 | ksp: KSPField=9 | fan_in=0
- methods(3): Button_OnTap, OnAwake, SetLight
- auto: B / Kerbals

## Assembly-CSharp/InternalCamera.cs
- L=272 kind=MonoBehaviour type=InternalCamera bases=IGameCamera anc=IGameCamera/MonoBehaviour ns=-
- unity(77): Transform=17, Time.=10, Input.=10, Lifecycle=4, KeyCode=4, Coroutine=2 | ksp: GameSettings=23, FlightCamera=4 | fan_in=2
- methods(12): Awake, DisableCamera, EnableCamera, ManualReset, MiddleDoubleClick, OnDestroy, ResetCamera, SetFOV, SetTransform, Start, Update, UpdateState
- auto: C / Camera

## Assembly-CSharp/InternalCameraSwitch.cs
- L=59 kind=MonoBehaviour type=InternalCameraSwitch bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(60): Transform=4, GameObject=1 | ksp: KSPField=2 | fan_in=0
- methods(2): Button_OnDoubleTap, OnAwake
- auto: B / Kerbals

## Assembly-CSharp/InternalCompass.cs
- L=34 kind=MonoBehaviour type=InternalCompass bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(64): Transform=3, Time.=1 | ksp: KSPField=3, FlightGlobals=1 | fan_in=0
- methods(2): OnAwake, OnUpdate
- auto: B / Kerbals

## Assembly-CSharp/InternalComponents.cs
- L=105 kind=MonoBehaviour type=InternalComponents bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(65): Transform=6, Lifecycle=2, SetActive=1, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=2
- methods(4): Awake, CreateText, OnDestroy, getTMPAlignment
- auto: B / Kerbals

## Assembly-CSharp/InternalDialIncrement.cs
- L=61 kind=class type=InternalDialIncrement bases=IConfigNode anc=IConfigNode ns=-
- unity(12): none | ksp: ConfigNode=4 | fan_in=2
- methods(3): CalculateAngle, Load, Save
- auto: A / Kerbals

## Assembly-CSharp/InternalFlagDecal.cs
- L=66 kind=MonoBehaviour type=InternalFlagDecal bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(85): Transform=5, GameObject=3, Renderer=2, Coroutine=2, Texture=1, Material=1 | ksp: KSPField=1, GameDatabase=1 | fan_in=0
- methods(4): OnAwake, OnDestroy, UpdateFlagTexture, WaitForPart
- auto: C / Rendering

## Assembly-CSharp/internalGeneric.cs
- L=10 kind=MonoBehaviour type=internalGeneric bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(50): Lifecycle=2 | ksp: none | fan_in=0
- methods(2): Start, Update
- auto: B / Misc

## Assembly-CSharp/InternalIndicatorPanel.cs
- L=167 kind=MonoBehaviour type=InternalIndicatorPanel bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(59): Shader=4, Material=4, Renderer=2 | ksp: ConfigNode=6, KSPField=1, FlightGlobals=1 | fan_in=0
- methods(8): Create, Load, OnAwake, OnUpdate, Save, Set, SetOff, SetOn
- auto: C / Kerbals

## Assembly-CSharp/InternalLabel.cs
- L=70 kind=MonoBehaviour type=InternalLabel bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(32): Transform=1 | ksp: KSPField=7, ConfigNode=1 | fan_in=0
- methods(3): OnAwake, OnLoad, SetText
- auto: B / Kerbals

## Assembly-CSharp/InternalLeverThrottle.cs
- L=47 kind=MonoBehaviour type=InternalLeverThrottle bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(81): Transform=2, Input.=2, Collider=2, Time.=1, GetComponent=1, GameObject=1 | ksp: KSPField=5 | fan_in=0
- methods(3): Lever_OnDrag, OnAwake, OnUpdate
- auto: B / Kerbals

## Assembly-CSharp/InternalModel.cs
- L=507 kind=MonoBehaviour type=InternalModel bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(74): Transform=47, GameObject=13, Animation=7, GetComponent=6, Renderer=4, Light=2 | ksp: ConfigNode=9, ProtoCrewMember=5, Part=2, Vessel=1, PartLoader=1 | fan_in=8
- methods(23): AddProp, AddPropModule, AssignToSeat, DespawnCrew, FindHeirarchyTransform, FindModelAnimators, FindModelComponent, FindModelComponents, FindModelTransform, GetAvailableSeatCount, GetAvailableSeats, GetNextAvailableSeat, GetNextAvailableSeatIndex, Initialize, InternalToWorld, Load, OnFixedUpdate, OnUpdate, SetVisible, SitKerbalAt, SpawnCrew, UnseatKerbal, UnseatKerbalAt
- auto: B / Kerbals

## Assembly-CSharp/InternalModule.cs
- L=92 kind=MonoBehaviour type=InternalModule bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(41): MonoBehaviour=1, Lifecycle=1 | ksp: ConfigNode=4, Vessel=1, Part=1 | fan_in=22
- methods(9): Awake, Load, ModularSetup, OnAwake, OnFixedUpdate, OnLoad, OnSave, OnUpdate, Save
- auto: B / Kerbals

## Assembly-CSharp/InternalNavBall.cs
- L=330 kind=MonoBehaviour type=InternalNavBall bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(91): Transform=69, GameObject=22, SetActive=20, Renderer=2, Material=2, GetComponent=2 | ksp: FlightGlobals=25, KSPField=18, Vessel=2, GameEvents=2, Orbit=1 | fan_in=0
- methods(4): OnAwake, OnDestroy, OnUpdate, OnVesselSwitch
- auto: B / Kerbals

## Assembly-CSharp/InternalPart.cs
- L=86 kind=MonoBehaviour type=InternalPart bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(45): Transform=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: ConfigNode=4, PartLoader=2, Part=1 | fan_in=0
- methods(9): AddProp, Load, OnActive, OnAwake, OnFixedUpdate, OnInactive, OnStart, OnUpdate, Save
- auto: B / Kerbals

## Assembly-CSharp/InternalProp.cs
- L=300 kind=MonoBehaviour type=InternalProp bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(75): Transform=32, GameObject=7, Animation=7, GetComponent=3, MonoBehaviour=1 | ksp: ConfigNode=4, Vessel=1, Part=1 | fan_in=4
- methods(13): AddModule, FindHeirarchyTransform, FindHeirarchyTransforms, FindModelAnimators, FindModelComponent, FindModelComponents, FindModelTransform, FindModelTransforms, Load, OnAwake, OnFixedUpdate, OnUpdate, Save
- auto: B / Kerbals

## Assembly-CSharp/InternalRadarAltitude.cs
- L=47 kind=MonoBehaviour type=InternalRadarAltitude bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(56): Transform=3, Time.=1 | ksp: KSPField=4 | fan_in=0
- methods(2): OnAwake, OnUpdate
- auto: B / Kerbals

## Assembly-CSharp/InternalSeat.cs
- L=122 kind=MonoBehaviour type=InternalSeat bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(70): Transform=11, Camera=2, Instantiate/Destroy=1, GameObject=1 | ksp: KSPField=9, ProtoCrewMember=2, Localizer=2 | fan_in=2
- methods(4): DespawnCrew, OnAwake, SpawnCrew, UpdatePortraitOffset
- auto: B / Kerbals

## Assembly-CSharp/InternalSpace.cs
- L=39 kind=MonoBehaviour type=InternalSpace bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(81): Transform=5, Lifecycle=2, MonoBehaviour=1 | ksp: FlightGlobals=3 | fan_in=4
- methods(4): Awake, InternalToWorld, OnDestroy, WorldToInternal
- auto: B / Kerbals

## Assembly-CSharp/InternalSpaceOverlay.cs
- L=97 kind=MonoBehaviour type=InternalSpaceOverlay bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(82): Transform=5, GameObject=4, Camera=4, Lifecycle=2, GetComponent=2, MonoBehaviour=1 | ksp: GameEvents=7, Vessel=3, Part=2, ProtoCrewMember=1, FlightCamera=1 | fan_in=1
- methods(8): Create, Dismiss, LateUpdate, OnCameraChange, OnDestroy, OnVesselModified, Setup, onCrewTransferred
- auto: B / Kerbals

## Assembly-CSharp/InternalSpeed.cs
- L=147 kind=MonoBehaviour type=InternalSpeed bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(32): Transform=2, GameObject=1 | ksp: KSPField=9, FlightGlobals=8, Localizer=7, Orbit=1, ConfigNode=1 | fan_in=1
- methods(5): CacheLocalStrings, OnAwake, OnButtonTap, OnLoad, OnUpdate
- auto: B / Kerbals

## Assembly-CSharp/InternalText.cs
- L=17 kind=MonoBehaviour type=InternalText bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(77): TMPro=2, MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=3
- methods(1): Awake
- auto: B / Kerbals

## Assembly-CSharp/InternalVSI.cs
- L=40 kind=MonoBehaviour type=InternalVSI bases=InternalModule anc=InternalModule/MonoBehaviour ns=-
- unity(60): Transform=3, Time.=1 | ksp: KSPField=4, FlightGlobals=1 | fan_in=0
- methods(2): OnAwake, OnUpdate
- auto: B / Kerbals

## Assembly-CSharp/IntersectInformation.cs
- L=21 kind=struct type=IntersectInformation bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Core

## Assembly-CSharp/IntTypeParser.cs
- L=31 kind=class type=IntTypeParser bases=ITypeParser anc=ITypeParser ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(2): Convert, Parse
- auto: A / Modding

## Assembly-CSharp/InventoryPanelController.cs
- L=307 kind=MonoBehaviour type=InventoryPanelController bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(56): Transform=6, Lifecycle=4, Instantiate/Destroy=4, EventSystems=4, GameObject=3, MonoBehaviour=1 | ksp: GameEvents=6, EditorLogic=4, AvailablePart=3, ProtoCrewMember=2, ShipConstruct=1 | fan_in=3
- methods(14): Awake, DestroyHeldIcons, OnDestroy, OnEditorCrewModified, OnEditorScreenChange, OnEditorShipModified, OnPointerEnter, OnPointerExit, Reset, Start, TryGetDisplayedInventory, Update, UpdateDisplayedInventories, UpdateInventoryLists
- auto: C / Kerbals

## Assembly-CSharp/IOverheatDisplay.cs
- L=14 kind=interface type=IOverheatDisplay bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Thermal

## Assembly-CSharp/IPartCostModifier.cs
- L=6 kind=interface type=IPartCostModifier bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=7
- methods(0): 
- auto: A / Part

## Assembly-CSharp/IPartMassModifier.cs
- L=6 kind=interface type=IPartMassModifier bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=12
- methods(0): 
- auto: A / Part

## Assembly-CSharp/IPartSizeModifier.cs
- L=8 kind=interface type=IPartSizeModifier bases=- anc=- ns=-
- unity(6): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Part

## Assembly-CSharp/IPersistenceLoad.cs
- L=4 kind=interface type=IPersistenceLoad bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Save

## Assembly-CSharp/IPersistenceSave.cs
- L=4 kind=interface type=IPersistenceSave bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Save

## Assembly-CSharp/IPid.cs
- L=4 kind=interface type=IPid bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Core

## Assembly-CSharp/IResourceBroker.cs
- L=18 kind=interface type=IResourceBroker bases=- anc=- ns=-
- unity(0): none | ksp: Part=8 | fan_in=3
- methods(0): 
- auto: A / Resources

## Assembly-CSharp/IResourceConsumer.cs
- L=6 kind=interface type=IResourceConsumer bases=- anc=- ns=-
- unity(0): none | ksp: PartResourceDefinition=1 | fan_in=19
- methods(0): 
- auto: A / Resources

## Assembly-CSharp/IScalarModule.cs
- L=20 kind=interface type=IScalarModule bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=17
- methods(0): 
- auto: A / Part

## Assembly-CSharp/IScienceDataContainer.cs
- L=16 kind=interface type=IScienceDataContainer bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=6
- methods(0): 
- auto: A / Science

## Assembly-CSharp/IScienceDataTransmitter.cs
- L=14 kind=interface type=IScienceDataTransmitter bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=8
- methods(0): 
- auto: A / Science

## Assembly-CSharp/IScreenCaster.cs
- L=6 kind=interface type=IScreenCaster bases=- anc=- ns=-
- unity(3): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / Misc

## Assembly-CSharp/ISettings.cs
- L=14 kind=interface type=ISettings bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=5
- methods(0): 
- auto: A / Core

## Assembly-CSharp/IShipconstruct.cs
- L=6 kind=interface type=IShipconstruct bases=- anc=- ns=-
- unity(0): none | ksp: Part=1 | fan_in=2
- methods(0): 
- auto: A / Save

## Assembly-CSharp/IShipConstructIDChanges.cs
- L=6 kind=interface type=IShipConstructIDChanges bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Save

## Assembly-CSharp/ISiteNode.cs
- L=11 kind=interface type=ISiteNode bases=- anc=- ns=-
- unity(21): Transform=1 | ksp: none | fan_in=3
- methods(0): 
- auto: A / Scene

## Assembly-CSharp/IStageSeparator.cs
- L=4 kind=interface type=IStageSeparator bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=8
- methods(0): 
- auto: A / PartModules-Structural

## Assembly-CSharp/IStageSeparatorChild.cs
- L=8 kind=interface type=IStageSeparatorChild bases=- anc=- ns=-
- unity(0): none | ksp: Part=1 | fan_in=4
- methods(0): 
- auto: A / PartModules-Structural

## Assembly-CSharp/ISteerable.cs
- L=4 kind=interface type=ISteerable bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: A / PartModules-Control

## Assembly-CSharp/ITargetable.cs
- L=26 kind=interface type=ITargetable bases=- anc=- ns=-
- unity(27): Transform=1 | ksp: Vessel=1, OrbitDriver=1, Orbit=1 | fan_in=11
- methods(0): 
- auto: A / Orbit

## Assembly-CSharp/ITechPerk.cs
- L=6 kind=interface type=ITechPerk bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Science

## Assembly-CSharp/IThrottleControl.cs
- L=6 kind=interface type=IThrottleControl bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: A / PartModules-Engines

## Assembly-CSharp/IThrustProvider.cs
- L=10 kind=interface type=IThrustProvider bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / PartModules-Engines

## Assembly-CSharp/IThumbnailSetup.cs
- L=8 kind=interface type=IThumbnailSetup bases=- anc=- ns=-
- unity(21): GameObject=1 | ksp: ProtoPartSnapshot=2 | fan_in=3
- methods(0): 
- auto: A / Misc

## Assembly-CSharp/IToggleCrossfeed.cs
- L=12 kind=interface type=IToggleCrossfeed bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / PartModules-Structural

## Assembly-CSharp/ITorqueProvider.cs
- L=6 kind=interface type=ITorqueProvider bases=- anc=- ns=-
- unity(6): none | ksp: none | fan_in=5
- methods(0): 
- auto: A / PartModules-Control

## Assembly-CSharp/ITypeParser.cs
- L=6 kind=interface type=ITypeParser bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=7
- methods(0): 
- auto: A / Modding

## Assembly-CSharp/IVACamera.cs
- L=176 kind=MonoBehaviour type=IVACamera bases=IGameCamera anc=IGameCamera/MonoBehaviour ns=-
- unity(75): Transform=8, Input.=7, Time.=4, KeyCode=4, Lifecycle=2, Camera=2 | ksp: GameSettings=14, FlightCamera=10 | fan_in=1
- methods(10): Activate, Deactivate, DeactivateAll, DisableCamera, EnableCamera, ResetCamera, ResetState, Start, Update, UpdateState
- auto: C / Camera

## Assembly-CSharp/IVASun.cs
- L=37 kind=MonoBehaviour type=IVASun bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(90): Light=4, Transform=3, Lifecycle=2, GetComponent=2, MonoBehaviour=1 | ksp: FlightGlobals=1 | fan_in=0
- methods(2): LateUpdate, Start
- auto: B / Kerbals

## Assembly-CSharp/IVesselAutoRename.cs
- L=6 kind=interface type=IVesselAutoRename bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / Vessel

## Assembly-CSharp/KeepRot.cs
- L=16 kind=MonoBehaviour type=KeepRot bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(73): Transform=2, Lifecycle=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(2): LateUpdate, Start
- auto: C / Camera

## Assembly-CSharp/Kerbal.cs
- L=380 kind=MonoBehaviour type=Kerbal bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(68): Coroutine=8, Transform=5, Texture=4, Renderer=4, Camera=4, Time.=3 | ksp: FlightGlobals=6, ProtoCrewMember=4, Vessel=3, GameEvents=3, Part=1 | fan_in=33
- methods(17): Awake, CameraOverlayUpdate, CheckEVAUnlocked, DestroyAvatarTexture, IVADisable, IVAEnable, OnDestroy, SetVisibleInPortrait, SetupCache, SetupKerbalModel, ShowHelmet, Start, die, kerbalAvatarUpdate, kerbalAvatarUpdateCycle, kerbalSeatCamUpdate, onVesselSitChange
- auto: C / Kerbals

## Assembly-CSharp/KerbalAnimationManager.cs
- L=233 kind=class type=KerbalAnimationManager bases=- anc=- ns=-
- unity(33): Animation=5, GetComponent=3, LayerMask=2, Transform=1 | ksp: KerbalEVA=2 | fan_in=2
- methods(4): GetAllAnimations, Start, SyncAnimationLayers, setupAnimation
- auto: B / Kerbals

## Assembly-CSharp/KerbalAnimationState.cs
- L=66 kind=class type=KerbalAnimationState bases=- anc=- ns=-
- unity(54): Animation=4, Transform=2, GetComponent=1 | ksp: none | fan_in=1
- methods(3): GetAnimationString, SetManager, string
- auto: B / Kerbals

## Assembly-CSharp/KerbalExpressionAI.cs
- L=183 kind=MonoBehaviour type=KerbalExpressionAI bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(69): Transform=5, Mesh=5, Lifecycle=5, Time.=3, Coroutine=3, MonoBehaviour=1 | ksp: FlightGlobals=4, GameEvents=3, ProtoCrewMember=1 | fan_in=0
- methods(10): Awake, FixedUpdate, OnDestroy, Start, Update, UpdateExpressionAI, UpdateHeadMesh, UpdateHeadTransforms, explosionReaction, kerbalAvatarUpdateCycle
- auto: B / Kerbals

## Assembly-CSharp/kerbalExpressionSystem.cs
- L=396 kind=MonoBehaviour type=kerbalExpressionSystem bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(56): Animation=8, Time.=7, Lifecycle=5, Shader=4, Coroutine=2, MonoBehaviour=1 | ksp: FlightGlobals=5, GameEvents=3, ProtoCrewMember=2, Part=1, KerbalEVA=1 | fan_in=0
- methods(10): Awake, EVAUpdate, FixedUpdate, IVAUpdate, OnDestroy, SetNewIdleExpression, Start, Update, UpdateExpressionAI, explosionReaction
- auto: B / Kerbals

## Assembly-CSharp/KerbalFSM.cs
- L=214 kind=class type=KerbalFSM bases=- anc=- ns=-
- unity(16): Time.=6 | ksp: none | fan_in=10
- methods(9): AddEvent, AddEventExcluding, AddState, FixedUpdateFSM, LateUpdateFSM, RunEvent, StartFSM, UpdateFSM, updateFSM
- auto: A / Kerbals

## Assembly-CSharp/KerbalInstructor.cs
- L=153 kind=MonoBehaviour type=KerbalInstructor bases=KerbalInstructorBase anc=KerbalInstructorBase/MonoBehaviour ns=-
- unity(61): Animation=8, SendMessage/Invoke=3, Lifecycle=1 | ksp: GameSettings=4 | fan_in=6
- methods(6): PlayEmote, PlayEmoteQueued, PlayEmoteRepeating, RepeatEmote, Start, StopRepeatingEmote
- auto: B / Tutorials

## Assembly-CSharp/KerbalInstructorBase.cs
- L=69 kind=MonoBehaviour type=KerbalInstructorBase bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(83): Animation=4, Lifecycle=2, GetComponent=2, Transform=1, Texture=1, MonoBehaviour=1 | ksp: none | fan_in=3
- methods(5): ClearCamera, OnDestroy, SetupAnimations, SetupCamera, Start
- auto: B / Tutorials

## Assembly-CSharp/KerbalInventoryScenario.cs
- L=133 kind=ScenarioModule type=KerbalInventoryScenario bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(77): GameObject=14, Transform=7, Instantiate/Destroy=3, Lifecycle=2 | ksp: GameEvents=2, ScenarioModule=1, ProtoCrewMember=1, HighLogic=1, ConfigNode=1 | fan_in=3
- methods(8): AddKerbalInventoryInstance, ContainsCrew, OnAwake, OnDestroy, OnKerbalNameChanged, OnSave, RemoveKerbalInventoryInstance, Update
- auto: B / Kerbals

## Assembly-CSharp/KerbalMixingTransforms.cs
- L=10 kind=struct type=KerbalMixingTransforms bases=- anc=- ns=-
- unity(40): Transform=2 | ksp: none | fan_in=1
- methods(0): 
- auto: A / Kerbals

## Assembly-CSharp/KerbalMovementParameters.cs
- L=17 kind=class type=KerbalMovementParameters bases=- anc=- ns=-
- unity(12): none | ksp: none | fan_in=0
- methods(0): 
- auto: A / Kerbals

## Assembly-CSharp/KerbalPreview.cs
- L=82 kind=MonoBehaviour type=KerbalPreview bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(90): SetActive=8, Material=6, GameObject=5, Renderer=3, MonoBehaviour=1, Lifecycle=1 | ksp: ProtoCrewMember=1 | fan_in=2
- methods(2): Awake, PreviewHelmetSelection
- auto: C / Kerbals

## Assembly-CSharp/KerbalProp.cs
- L=24 kind=class type=KerbalProp bases=- anc=- ns=-
- unity(72): GameObject=4, Transform=2, Renderer=1, Animation=1 | ksp: none | fan_in=1
- methods(0): 
- auto: B / Kerbals

## Assembly-CSharp/KerbalRagdollNode.cs
- L=23 kind=class type=KerbalRagdollNode bases=- anc=- ns=-
- unity(62): Transform=1, Rigidbody=1, GameObject=1, Collider=1 | ksp: Vector3d=1 | fan_in=1
- methods(1): updateVelocity
- auto: B / Kerbals

## Assembly-CSharp/KerbalSeat.cs
- L=313 kind=PartModule type=KerbalSeat bases=PartModule,ICommNetControlSource,IConstruction anc=PartModule/MonoBehaviour ns=-
- unity(61): GameObject=12, Transform=9, GetComponent=3, Coroutine=2, SetActive=1, Collider=1 | ksp: KSPField=7, KerbalEVA=5, Part=4, HighLogic=4, Localizer=2 | fan_in=8
- methods(20): BoardSeat, CacheLocalStrings, CanBeDetached, CanBeOffset, CanBeRotated, EditorSeatBoundsAreClear, GetControlSourceState, IsCommCapable, LeaveSeat, MakeReferenceToggle, MakeReferenceTransform, OnChildRemove, OnInventoryModeEnable, OnLoad, OnSave, OnStart, OnStartFinished, SeatBoundsAreClear, SpawnBoardingKerbal, UpdateNetwork
- auto: B / Kerbals

## Assembly-CSharp/KerbalVisorGlossiness.cs
- L=12 kind=MonoBehaviour type=KerbalVisorGlossiness bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(88): Material=2, Shader=1, Renderer=1, MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=0
- methods(1): Start
- auto: C / Rendering

## Assembly-CSharp/KeyBinding.cs
- L=227 kind=class type=KeyBinding bases=InputBinding,IConfigNode,ICloneable anc=InputBinding/IConfigNode ns=-
- unity(44): KeyCode=12, Time.=8 | ksp: ConfigNode=2 | fan_in=6
- methods(8): GetDoubleTapDown, GetDoubleTapUp, GetKey, GetKeyDown, GetKeyUp, IsNeutral, Load, Save
- auto: A / Input

## Assembly-CSharp/KeyboardLayout.cs
- L=47 kind=class type=KeyboardLayout bases=- anc=- ns=-
- unity(8): none | ksp: none | fan_in=2
- methods(2): GetKeyboardLayout, GetKeyboardLayoutType
- auto: A / Input

## Assembly-CSharp/KeyboardLayoutType.cs
- L=9 kind=enum type=KeyboardLayoutType bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Input

## Assembly-CSharp/KeyCodeExtended.cs
- L=62 kind=class type=KeyCodeExtended bases=- anc=- ns=-
- unity(61): KeyCode=8 | ksp: none | fan_in=4
- methods(1): ToString
- auto: A / Input

## Assembly-CSharp/KFSMCallback.cs
- L=1 kind=delegate type=KFSMCallback bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=16
- methods(0): 
- auto: A / Core

## Assembly-CSharp/KFSMEvent.cs
- L=24 kind=class type=KFSMEvent bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=23
- methods(1): IsValid
- auto: A / Core

## Assembly-CSharp/KFSMEventCondition.cs
- L=1 kind=delegate type=KFSMEventCondition bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Core

## Assembly-CSharp/KFSMState.cs
- L=57 kind=class type=KFSMState bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=45
- methods(3): AddEvent, IsValid, ToString
- auto: A / Core

## Assembly-CSharp/KFSMStateChange.cs
- L=1 kind=delegate type=KFSMStateChange bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=15
- methods(0): 
- auto: A / Core

## Assembly-CSharp/KFSMTimedEvent.cs
- L=18 kind=class type=KFSMTimedEvent bases=KFSMEvent anc=KFSMEvent ns=-
- unity(12): Time.=1 | ksp: none | fan_in=4
- methods(1): WaitForTimer
- auto: A / Core

## Assembly-CSharp/KFSMUpdateMode.cs
- L=7 kind=enum type=KFSMUpdateMode bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=18
- methods(0): 
- auto: A / Core

## Assembly-CSharp/KnowledgeItem.cs
- L=85 kind=class type=KnowledgeItem bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(2): GetValue, UpdateDataCallback
- auto: A / Science

## Assembly-CSharp/Krakensbane.cs
- L=209 kind=MonoBehaviour type=Krakensbane bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(57): Rigidbody=5, Lifecycle=4, Time.=2, Collision/Force=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: FlightGlobals=22, Vector3d=11, Vessel=6, Krakensbane=2, GameEvents=2 | fan_in=17
- methods(12): AddExcess, AddFrameVelocity, Awake, FixedUpdate, GetFrameVelocity, GetFrameVelocityV3f, GetLastCorrection, OnDestroy, ResetVelocityFrame, SafeToEngage, Start, Zero
- auto: B / Physics

## Assembly-CSharp/KSCPauseMenu.cs
- L=462 kind=MonoBehaviour type=KSCPauseMenu bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(68): GameObject=24, SetActive=15, KeyCode=4, Input.=4, Lifecycle=3, TMPro=2 | ksp: Localizer=37, HighLogic=34, MissionSystem=7, Game=5, ConfigNode=5 | fan_in=1
- methods(24): Awake, BuildButtonList, CheckFilename, ConfirmDialog, Create, Dismiss, InitiateLoad, InitiateSave, LoadGame, MissionModeValid, OnDestroy, OnSaveDialogDestroy, SaveGame, Update, doSave, draw, drawMissionRevertOptions, onLoadDialogDismiss, onMiniSettingsFinished, onPipelineFailed, onPipelineFinished, onSaveDialogDismiss, quickLoad, spawnSaveDialog
- auto: C / Scene

## Assembly-CSharp/KSCSiteNode.cs
- L=32 kind=class type=KSCSiteNode bases=ISiteNode anc=ISiteNode ns=-
- unity(20): Transform=1 | ksp: Localizer=1 | fan_in=3
- methods(4): GetDisplayName, GetName, GetWorldPos, UpdateNodeCaption
- auto: A / Scene

## Assembly-CSharp/KSCUpgradeableLevelText.cs
- L=15 kind=ScriptableObject type=KSCUpgradeableLevelText bases=ScriptableObject anc=ScriptableObject ns=-
- unity(29): ScriptableObject=1 | ksp: none | fan_in=2
- methods(0): 
- auto: A / Career

## Assembly-CSharp/KSPAction.cs
- L=69 kind=attribute type=KSPAction bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: KSPAction=6 | fan_in=52
- methods(0): 
- auto: A / Part

## Assembly-CSharp/KSPActionGroup.cs
- L=44 kind=enum type=KSPActionGroup bases=- anc=- ns=-
- unity(21): Light=1 | ksp: none | fan_in=42
- methods(0): 
- auto: A / Part

## Assembly-CSharp/KSPActionParam.cs
- L=29 kind=class type=KSPActionParam bases=- anc=- ns=-
- unity(3): none | ksp: none | fan_in=53
- methods(0): 
- auto: A / Part

## Assembly-CSharp/KSPActionType.cs
- L=6 kind=enum type=KSPActionType bases=- anc=- ns=-
- unity(29): UnityEngine.UI=1 | ksp: none | fan_in=28
- methods(0): 
- auto: A / Part

## Assembly-CSharp/KSPAddon.cs
- L=35 kind=attribute type=KSPAddon bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / Modding

## Assembly-CSharp/KSPAssembly.cs
- L=29 kind=attribute type=KSPAssembly bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Modding

## Assembly-CSharp/KSPAssemblyDependency.cs
- L=29 kind=attribute type=KSPAssemblyDependency bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: A / Modding

## Assembly-CSharp/KSPAssemblyDependencyEqualMajor.cs
- L=29 kind=attribute type=KSPAssemblyDependencyEqualMajor bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: A / Modding

## Assembly-CSharp/KSPAxisField.cs
- L=30 kind=attribute type=KSPAxisField bases=KSPField anc=KSPField/FieldAttribute/Attribute ns=-
- unity(0): none | ksp: KSPField=1 | fan_in=19
- methods(0): 
- auto: A / Part

## Assembly-CSharp/KSPAxisGroup.cs
- L=36 kind=enum type=KSPAxisGroup bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=14
- methods(0): 
- auto: A / Part

## Assembly-CSharp/KSPAxisMode.cs
- L=5 kind=enum type=KSPAxisMode bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=15
- methods(0): 
- auto: A / Part

## Assembly-CSharp/KSPCameraUtil.cs
- L=38 kind=class type=KSPCameraUtil bases=- anc=- ns=-
- unity(28): none | ksp: GameSettings=10 | fan_in=7
- methods(3): AnyCameraInputDown, GetCamCoordsFromPosition, GetDistanceToFit
- auto: C / Camera

## Assembly-CSharp/KSPCapability.cs
- L=14 kind=attribute type=KSPCapability bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Part

## Assembly-CSharp/KSPCapabilityValue.cs
- L=18 kind=attribute type=KSPCapabilityValue bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Part

## Assembly-CSharp/KSPCompression.cs
- L=135 kind=MonoBehaviour type=KSPCompression bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(26): MonoBehaviour=1 | ksp: none | fan_in=2
- methods(4): CompressDirectory, DecompressFile, FilesAtRoot, GetTopLevelDirectories
- auto: B / Save

## Assembly-CSharp/KSPEvent.cs
- L=79 kind=attribute type=KSPEvent bases=Attribute anc=Attribute ns=-
- unity(5): none | ksp: KSPEvent=2, Localizer=1 | fan_in=74
- methods(0): 
- auto: A / Part

## Assembly-CSharp/KSPField.cs
- L=56 kind=attribute type=KSPField bases=FieldAttribute anc=FieldAttribute/Attribute ns=-
- unity(7): none | ksp: KSPField=2, Localizer=1 | fan_in=167
- methods(0): 
- auto: A / Part

## Assembly-CSharp/KSPFontAsset.cs
- L=157 kind=class type=KSPFontAsset bases=- anc=- ns=-
- unity(68): TMPro=8, Material=8, Shader=4, Texture=2, ScriptableObject=1 | ksp: none | fan_in=2
- methods(3): GetAtlasFromHash, GetFontAsset, GetMaterialFromHash
- auto: C / Localization

## Assembly-CSharp/KSPFontAssetList.cs
- L=7 kind=class type=KSPFontAssetList bases=List<KSPFontAsset> anc=List ns=-
- unity(12): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / Localization

## Assembly-CSharp/KSPFontTypeConverter.cs
- L=17 kind=class type=KSPFontTypeConverter bases=SerializationBinder anc=SerializationBinder ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(1): BindToType
- auto: C / Localization

## Assembly-CSharp/KSPKerningPairList.cs
- L=8 kind=class type=KSPKerningPairList bases=List<KerningPair> anc=List ns=-
- unity(12): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / Localization

## Assembly-CSharp/KSPLog.cs
- L=307 kind=MonoBehaviour type=KSPLog bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(55): QualitySettings/SystemInfo=9, Application.=6, Texture=3, Lifecycle=3, Transform=2, Instantiate/Destroy=2 | ksp: GameSettings=5 | fan_in=3
- methods(15): AddLogCallback, Awake, ConvertLineEndings, LogCallback, LogHeader, LogRenderTextureFormats, LogToFile, LogToMemory, LogToScreen, OnDestroy, OnMemoryLogUpdated, RemoveLogCallback, SetScreenLogging, Start, TimeString
- auto: B / Core

## Assembly-CSharp/KSPModelEvent.cs
- L=18 kind=attribute type=KSPModelEvent bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: A / Part

## Assembly-CSharp/KSPModule.cs
- L=12 kind=attribute type=KSPModule bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Part

## Assembly-CSharp/KSPParseable.cs
- L=181 kind=struct type=KSPParseable bases=- anc=- ns=-
- unity(6): none | ksp: none | fan_in=5
- methods(1): Save
- auto: A / Modding

## Assembly-CSharp/KSPPart.cs
- L=12 kind=attribute type=KSPPart bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: A / Part

## Assembly-CSharp/KSPParticleEmitter.cs
- L=418 kind=MonoBehaviour type=KSPParticleEmitter bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(73): ParticleSystem=29, GetComponent=4, Transform=3, Time.=2, Material=2, Lifecycle=2 | ksp: none | fan_in=7
- methods(8): Awake, Emit, EmitParticle, EnsureParticleSystemCreated, Reset, SetDirty, SetupProperties, Update
- auto: C / Rendering

## Assembly-CSharp/KSPRandom.cs
- L=134 kind=class type=KSPRandom bases=System.Random anc=Random ns=-
- unity(3): none | ksp: none | fan_in=54
- methods(4): Next, NextBytes, NextDouble, Sample
- auto: A / Core

## Assembly-CSharp/KSPRichTextUtil.cs
- L=17 kind=class type=KSPRichTextUtil bases=RichTextUtil anc=RichTextUtil ns=-
- unity(0): none | ksp: GameSettings=3, Planetarium=2 | fan_in=2
- methods(3): TextDate, TextDateCompact, TextDurationCompact
- auto: C / UI

## Assembly-CSharp/KSPScenario.cs
- L=42 kind=attribute type=KSPScenario bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=24
- methods(2): HasCreateOption, HasTargetScene
- auto: A / Modding

## Assembly-CSharp/KSPScenarioType.cs
- L=55 kind=class type=KSPScenarioType bases=- anc=- ns=-
- unity(13): none | ksp: ScenarioModule=2 | fan_in=3
- methods(1): GetAllScenarioTypesInAssemblies
- auto: A / Modding

## Assembly-CSharp/KSPSteamUtils.cs
- L=102 kind=class type=KSPSteamUtils bases=- anc=- ns=-
- unity(45): Steam=6, GameObject=1 | ksp: none | fan_in=10
- methods(4): GatherCraftFiles, GatherCraftFilesFileInfo, GetSteamCacheLocation, GetSteamIDFromSteamFolder
- auto: C / Steam

## Assembly-CSharp/KSPUpgradePipeline.cs
- L=97 kind=static class type=KSPUpgradePipeline bases=- anc=- ns=-
- unity(25): UnityEngine.UI=2 | ksp: ConfigNode=6, InputLockManager=2, HighLogic=1 | fan_in=6
- methods(3): Init, OnFinish, Process
- auto: A / Save

## Assembly-CSharp/KSPWheelController.cs
- L=274 kind=MonoBehaviour type=KSPWheelController bases=VehicleBase anc=VehicleBase/MonoBehaviour ns=-
- unity(68): Rigidbody=6, KeyCode=5, Input.=5, GetComponent=4, GameObject=4, Time.=3 | ksp: HighLogic=1 | fan_in=5
- methods(8): Awake, Create, DoUpdateBlocks, Initialize, OnInitialize, OnUpdate, SetRigidbodyTarget, SetWheelTransform
- auto: B / PartModules-Wheels

## Assembly-CSharp/KSPWheelGravity.cs
- L=59 kind=class type=KSPWheelGravity bases=- anc=- ns=-
- unity(8): none | ksp: none | fan_in=2
- methods(1): Refresh
- auto: A / PartModules-Wheels

## Assembly-CSharp/LadderEndCheck.cs
- L=45 kind=MonoBehaviour type=LadderEndCheck bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(80): Collision/Force=3, Collider=3, MonoBehaviour=1 | ksp: KerbalEVA=1 | fan_in=1
- methods(3): OnTriggerEnter, OnTriggerExit, OnTriggerStay
- auto: B / Kerbals

## Assembly-CSharp/LandClassROC.cs
- L=404 kind=class type=LandClassROC bases=- anc=- ns=-
- unity(59): GameObject=34, Transform=19, SetActive=4, GetComponent=3, Renderer=2, Instantiate/Destroy=2 | ksp: PQS=7, HighLogic=6, Game=1, CelestialBody=1 | fan_in=3
- methods(9): AddScatterMeshController, BuildCache, ClearCache, CreateCache, CreateScatterMesh, DestroyQuad, Setup, SphereActive, SphereInactive
- auto: B / Terrain

## Assembly-CSharp/LatLon.cs
- L=121 kind=static class type=LatLon bases=- anc=- ns=-
- unity(1): none | ksp: Vector3d=29, Planetarium=11 | fan_in=8
- methods(10): GetAltitude, GetImpactLatitudeAndLongitude, GetLatLongAlt, GetLatitude, GetLatitudeAndLongitude, GetLongitude, GetRelSurfaceNVector, GetRelSurfacePosition, GetSurfaceNVector, GetWorldSurfacePosition
- auto: A / Celestials

## Assembly-CSharp/LaunchClamp.cs
- L=373 kind=PartModule type=LaunchClamp bases=PartModule,IStageSeparator anc=PartModule/MonoBehaviour ns=-
- unity(72): Transform=8, Joint=8, GameObject=8, GetComponent=4, Collider=4, Coroutine=3 | ksp: KSPField=8, HighLogic=4, ConfigNode=2, Vessel=1, PartModule=1 | fan_in=3
- methods(20): CreatePoints, EnableExtension, ExtendTower, FindCollider, GetStageIndex, MakePoint, OnActive, OnInitialize, OnLoad, OnPutToGround, OnSave, OnStart, OnVesselPack, OnVesselUnpack, Release, ReleaseClamp, SetJoint, SetJointWhenPartStarted, UnsetJoint, Update
- auto: B / PartModules-Structural

## Assembly-CSharp/LaunchPadFX.cs
- L=72 kind=MonoBehaviour type=LaunchPadFX bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(75): ParticleSystem=4, Lifecycle=3, MonoBehaviour=1, Material=1 | ksp: FloatingOrigin=2 | fan_in=1
- methods(4): AddFX, LateUpdate, OnDestroy, Start
- auto: C / Rendering

## Assembly-CSharp/LaunchSite.cs
- L=291 kind=class type=LaunchSite bases=ISiteNode anc=ISiteNode ns=-
- unity(34): Transform=10, GetComponent=1, GameObject=1 | ksp: Vector3d=3, PQS=2, CelestialBody=2, Localizer=1 | fan_in=27
- methods(11): GetName, GetSpawnPoint, GetSpawnPointLatLonAlt, GetSpawnPointTransform, GetWorldPos, GreatCircleDistance, SetSpawnPointLatLonAlt, SetSpawnPointsLatLonAlt, Setup, UpdateNodeCaption, setupGreatCircleVariables
- auto: B / Scene

## Assembly-CSharp/LaunchSiteFacility.cs
- L=412 kind=MonoBehaviour type=LaunchSiteFacility bases=SpaceCenterBuilding anc=SpaceCenterBuilding/MonoBehaviour ns=-
- unity(26): Texture=1, MonoBehaviour=1, IMGUI=1 | ksp: HighLogic=16, Localizer=14, GameEvents=14, InputLockManager=11, Game=4 | fan_in=2
- methods(18): Cancel, ClearSite, IsOpen, NothingToLaunch, OnClicked, OnOnDestroy, OnStart, RemoveInputlock, ResumeFlightOnSite, ReturnToDialog, launchChecks, launchVessel, onGUILaunchScreenDespawn, onMissionDialogDismiss, onMissionDialogUp, shipSelected, showFacilityLocked, showShipSelection
- auto: B / Scene

## Assembly-CSharp/LayerUtil.cs
- L=4 kind=static class type=LayerUtil bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=21
- methods(0): 
- auto: C / Rendering

## Assembly-CSharp/LED.cs
- L=101 kind=MonoBehaviour type=LED bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(80): Renderer=4, Material=4, GetComponent=4, Coroutine=4, Texture=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(6): Awake, blink, doBlink, setColor, setOff, setOn
- auto: C / FlightUI

## Assembly-CSharp/LightStruct.cs
- L=165 kind=class type=LightStruct bases=- anc=- ns=-
- unity(69): Shader=8, Material=8, Transform=5, Renderer=4, Light=2, GetComponent=1 | ksp: Part=1, ConfigNode=1 | fan_in=1
- methods(4): FindLightComponents, FixedUpdateLightsStatus, Load, UpdateLightStatusOnEditor
- auto: C / Rendering

## Assembly-CSharp/LinearGauge.cs
- L=151 kind=MonoBehaviour type=LinearGauge bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(69): GetComponent=5, Audio=5, Transform=3, Time.=2, Lifecycle=2, MonoBehaviour=1 | ksp: HighLogic=1 | fan_in=0
- methods(4): Start, Update, setValue, startSound
- auto: B / FlightUI

## Assembly-CSharp/LingoonaGrammarExtensions.cs
- L=112 kind=static class type=LingoonaGrammarExtensions bases=- anc=- ns=-
- unity(0): none | ksp: ProtoCrewMember=3 | fan_in=0
- methods(10): LocalizeCommonNoun, LocalizeCommonNounFemale, LocalizeCommonNounMale, LocalizeCommonNounNeutral, LocalizeName, LocalizeNameFemale, LocalizeNameMale, LocalizeNameNeutral, LocalizeNoun, LocalizeRemoveGender
- auto: C / Localization

## Assembly-CSharp/LinkedMesh.cs
- L=781 kind=class type=LinkedMesh bases=- anc=- ns=-
- unity(18): Mesh=5, Time.=4, Raycast/Query=1 | ksp: none | fan_in=1
- methods(21): CalculateTangets, ClearMesh, ClearSelection, ClearVerts, ComputeNormals, ComputeSelectionNormals, CreateLinks, GetMesh, HasVertsInTri, IsNeighbourOf, IsVertInTri, NormalizeMesh, RayIntersectMesh, RayIntersectTriangle, Raycast, SelectByLinks, SelectMesh, SetLinkTris, SetLinks, SetVerts, UpdateSelection
- auto: C / Rendering

## Assembly-CSharp/LinkedMeshSO.cs
- L=499 kind=ScriptableObject type=LinkedMeshSO bases=ScriptableObject anc=ScriptableObject ns=-
- unity(32): Mesh=8, Time.=2, ScriptableObject=1 | ksp: none | fan_in=0
- methods(10): CalculateSize, CalculateTangets, ComputeNormals, Contains, CreateLinkedMesh, GetCloneMesh, GetOriginalMesh, IndexOf, NormalizeMesh, SetupClone
- auto: C / Rendering

## Assembly-CSharp/ListDictionary.cs
- L=102 kind=class type=ListDictionary bases=- anc=- ns=-
- unity(4): none | ksp: none | fan_in=5
- methods(5): Add, Clear, GetEnumerator, Remove, TryGetValue
- auto: A / Core

## Assembly-CSharp/ListExtension.cs
- L=235 kind=static class type=ListExtension bases=- anc=- ns=-
- unity(0): none | ksp: Part=6 | fan_in=0
- methods(9): AddUnique, AddUniqueRange, ContainsId, MaxAt, MinAt, OrderByAlphaNumeric, OrderByAlphanumeric, SetRange, Shuffle
- auto: A / Core

## Assembly-CSharp/LoadCraftDialog.cs
- L=397 kind=MonoBehaviour type=LoadCraftDialog bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(47): UnityEngine.UI=5, GameObject=4, Instantiate/Destroy=3, Time.=2, Texture=2, MonoBehaviour=1 | ksp: Localizer=6, ShipConstruct=3, HighLogic=2, ConfigNode=2 | fan_in=0
- methods(15): BuildCraftList, CancelCallback, Create, CreateBrowserDialog, CreateBrowserItems, CreateDeleteFileConfirm, CreateWindowCleanupPrompt, DeleteInvalidSaves, OnDeleteConfirm, OnDestroy, SelectFileCallback, SelectItem, ShowDeleteFileConfirm, ShowWindowBrowser, ShowWindowCleanupPrompt
- auto: C / Editor

## Assembly-CSharp/LoadGameSearch.cs
- L=166 kind=MonoBehaviour type=LoadGameSearch bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(86): UnityEngine.UI=6, Transform=6, GetComponent=6, TMPro=4, Lifecycle=4, Coroutine=4 | ksp: InputLockManager=2 | fan_in=0
- methods(12): Awake, HasFiltered, ItemMatchesSearch, OnDestroy, OnDisable, SearchField_OnClick, SearchField_OnEndEdit, SearchField_OnValueChange, SearchRoutine, SearchStart, SearchStop, Update
- auto: C / Save

## Assembly-CSharp/LoadingAnimation.cs
- L=36 kind=MonoBehaviour type=LoadingAnimation bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(66): Lifecycle=2, Transform=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(2): Start, Update
- auto: B / Scene

## Assembly-CSharp/LoadingBufferMask.cs
- L=132 kind=MonoBehaviour type=LoadingBufferMask bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(76): Coroutine=9, SetActive=4, SceneManager=3, GameObject=3, Lifecycle=2, TMPro=1 | ksp: HighLogic=6, FlightGlobals=4, GameEvents=2 | fan_in=8
- methods(8): Hide, OnDestroy, OnLevelLoaded, OnSceneChange, OnSceneLoaded, Show, ShowDuration, Start
- auto: C / Rendering

## Assembly-CSharp/LoadingScreen.cs
- L=327 kind=MonoBehaviour type=LoadingScreen bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(78): Time.=14, Coroutine=14, GameObject=7, Texture=6, TMPro=3, Lifecycle=3 | ksp: Localizer=2, GameSettings=1, GameDatabase=1 | fan_in=2
- methods(10): Awake, FadeIn, FadeOut, LoadSystems, OnDestroy, SetTip, Start, StartLoadingScreens, UpdateLoadingScreen, UpdateProgressBar
- auto: C / Scene

## Assembly-CSharp/LoadingSystem.cs
- L=28 kind=MonoBehaviour type=LoadingSystem bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(36): MonoBehaviour=1 | ksp: none | fan_in=5
- methods(5): IsReady, LoadWeight, ProgressFraction, ProgressTitle, StartLoad
- auto: B / Modding

## Assembly-CSharp/LocalParticleGravity.cs
- L=21 kind=MonoBehaviour type=LocalParticleGravity bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(93): ParticleSystem=6, Transform=1, MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: FlightGlobals=2, PhysicsGlobals=1 | fan_in=0
- methods(1): Start
- auto: B / Physics

## Assembly-CSharp/LocalPhysXGravity.cs
- L=16 kind=MonoBehaviour type=LocalPhysXGravity bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(78): Transform=2, Lifecycle=2, Physics.=1, MonoBehaviour=1 | ksp: FlightGlobals=2 | fan_in=0
- methods(2): FixedUpdate, Start
- auto: B / Physics

## Assembly-CSharp/LocalSpace.cs
- L=27 kind=MonoBehaviour type=LocalSpace bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(86): Transform=3, Lifecycle=2, Instantiate/Destroy=2, MonoBehaviour=1, GameObject=1 | ksp: none | fan_in=3
- methods(2): Awake, OnDestroy
- auto: B / Misc
