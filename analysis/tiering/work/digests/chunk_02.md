# Review chunk 02 (root): 279 files, 65945 lines

## Assembly-CSharp/DragCube.cs
- 214 lines; kind=class; type=DragCube; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=28
- ksp refs: none; fan_in=7 fan_out=0
- methods(2): Load, SaveToString
- auto suggestion: tier=A subsystem=Aero

## Assembly-CSharp/DragCubeList.cs **MUST-OPEN (>800 lines)**
- 1010 lines; kind=class; type=DragCubeList; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Time.=2, GameObject=1; unity_score=14
- ksp refs: PhysicsGlobals=8, Part=7, ConfigNode=4, FloatCurve=3, AttachNode=3; fan_in=5 fan_out=8
- methods(39): AddCubeArray, AddFacedDotProducts, AddSurfaceDragDirection, AddVectorToArray, AreaToCubeOperation, ClearCubes, CreateVectorFromArray, DivideCubeArray, DrawDragGizmos, ForceUpdate, GetCube, GetCubeAreaDir, GetCubeCoeffDir, GetCubeDepthDir, GetFaceDirection, GetFacingAreaSum, LoadCube, LoadCubes, MultiplyCubeArray, RequestOcclusionUpdate, Reset, ResetCubeArray, ResetCubeWeights, SaveCubes, SetCubeArray, SetCubeWeight, SetDrag, SetDragVectorRotation, SetDragWeights, SetDragWeights_List, SetDragWeights_Procedural, SetFacedDotProducts, SetFacedDotProductsNormalized, SetOcclusionMultiplier, SetPart, SetPartOcclusion, SubtractVectorFromArray, SubtractVectorFromArrayClamp, SumCubeArray
- auto suggestion: tier=A subsystem=Aero

## Assembly-CSharp/DragCubeSystem.cs
- 771 lines; kind=MonoBehaviour; type=DragCubeSystem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.Rendering
- unity: GameObject=35, GetComponent=15, Instantiate/Destroy=13, Coroutine=11, Material=10, Renderer=9; unity_score=79
- ksp refs: Part=15, ConfigNode=9, PartLoader=6, PartModule=3, Vessel=1, FlightCamera=1; fan_in=4 fan_out=14
- methods(23): Awake, CalculateAerodynamics, CalculatePartBounds, CleanUpPartMaterials, CreateAeroCamera, CreateAeroTextures, GetAreaModifier, GetCdModifier, GetPlanarCenter, GetPlanarDepth, GetPlanarDirection, GetPlanarSize, LoadDragCubes, OnDestroy, RenderDragCubes, RenderDragCubesCoroutine, RenderProceduralDragCube, SetAeroCamera, SetMainTexture, SetupDragCubeCoroutine, SetupPartForRender, TestRender, UpdateAeroTexture
- auto suggestion: tier=B subsystem=Aero

## Assembly-CSharp/DragPanel.cs
- 42 lines; kind=MonoBehaviour; type=DragPanel; bases=MonoBehaviour,IPointerDownHandler,IEventSystemHandler,IDragHandler; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=4, UnityEngine.UI=2, Lifecycle=2, Transform=1, MonoBehaviour=1; unity_score=87
- ksp refs: GameEvents=2; fan_in=4 fan_out=2
- methods(5): OnDestroy, OnDrag, OnGameSettingsApplied, OnPointerDown, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/DrawDepth.cs
- 170 lines; kind=MonoBehaviour; type=DrawDepth; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Material=7, Shader=5, QualitySettings/SystemInfo=4, Lifecycle=4, Camera=3, Texture=2; unity_score=79
- ksp refs: none; fan_in=1 fan_out=0
- methods(12): CheckResources, CheckShader, CheckShaderAndCreateMaterial, CheckSupport, CreateMaterial, DoDepthRender, LateUpdate, NotSupported, OnDisable, OnEnable, ReportAutoDisable, Start
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/Drawing.cs
- 182 lines; kind=static class; type=Drawing; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: IMGUI=11, Texture=7, Material=5, Graphics/GL=4; unity_score=67
- ksp refs: none; fan_in=1 fan_out=0
- methods(8): CubeBezier, DrawBezierLine, DrawGraph, DrawGraphLine, DrawLine, GetMaxValueX, GetMaxValueY, Initialize
- auto suggestion: tier=C subsystem=Core

## Assembly-CSharp/DynamicAmbientLight.cs
- 161 lines; kind=MonoBehaviour; type=DynamicAmbientLight; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Light=22, Shader=4, Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=86
- ksp refs: HighLogic=11, GameSettings=3, FlightGlobals=3, MapView=2, GameEvents=2, CelestialBody=1; fan_in=1 fan_out=7
- methods(4): Awake, GrabDefaultAmbient, OnDestroy, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/DynamicNodeSet.cs
- 29 lines; kind=struct; type=DynamicNodeSet; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=12
- ksp refs: ConfigNode=1; fan_in=1 fan_out=1
- methods(1): Load
- auto suggestion: tier=A subsystem=Misc

## Assembly-CSharp/DynamicShadowSettings.cs
- 125 lines; kind=MonoBehaviour; type=DynamicShadowSettings; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.SceneManagement
- unity: QualitySettings/SystemInfo=14, SceneManager=3, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=64
- ksp refs: GameSettings=6, HighLogic=1; fan_in=1 fan_out=4
- methods(5): ApplyShadowSettings, Awake, OnDestroy, OnLevelLoaded, OnSceneLoaded
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/EditorBackgroundFlag.cs
- 41 lines; kind=MonoBehaviour; type=EditorBackgroundFlag; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=3, Renderer=2, Material=2, Texture=1, MonoBehaviour=1; unity_score=84
- ksp refs: HighLogic=2, GameEvents=2, EditorLogic=2, GameDatabase=1; fan_in=0 fan_out=4
- methods(4): Awake, OnDestroy, Start, updateFlag
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/EditorBounds.cs
- 106 lines; kind=MonoBehaviour; type=EditorBounds; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=3, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=61
- ksp refs: none; fan_in=5 fan_out=1
- methods(8): Awake, CenterSceneryOrigin, ClampCameraDistance, ClampToCameraBounds, OnDestroy, OnDrawGizmosSelected, SetExtents, centerSceneryOrigin
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/EditorCamera.cs
- 26 lines; kind=MonoBehaviour; type=EditorCamera; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Highlighting, UnityEngine
- unity: Lifecycle=2, GetComponent=2, Camera=2, MonoBehaviour=1; unity_score=83
- ksp refs: none; fan_in=5 fan_out=1
- methods(2): Awake, OnDestroy
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/EditorCameraOffset.cs
- 23 lines; kind=MonoBehaviour; type=EditorCameraOffset; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, Screen/Cursor=1, MonoBehaviour=1, GetComponent=1; unity_score=70
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): Start, Update
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/EditorDriver.cs
- 592 lines; kind=MonoBehaviour; type=EditorDriver; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, Expansions.Missions, Expansions.Missions.Runtime, KSP.UI, UnityEngine, UnityEngine.Rendering, UnityEngine.SceneManagement, UnityEngine.UI, Upgradeables
- unity: GameObject=20, Coroutine=16, SceneManager=12, SetActive=8, GetComponent=7, UnityEngine.UI=5; unity_score=71
- ksp refs: HighLogic=33, EditorLogic=19, Game=11, GameSettings=8, InputLockManager=7, MissionSystem=3; fan_in=31 fan_out=28
- methods(18): Awake, IsValidSaveFolder, OnApplicationFocus, OnDestroy, OtherLaunchSitesValid, SetDefaultSaveFolder, SetInputLockFromGameParameters, Start, StartAndLoadVessel, StartEditor, SwitchEditor, ValidLaunchSite, initEditor, onEditorStarted, saveselectedLaunchSite, setDefaultLaunchSite, setLaunchSite, setupValidLaunchSites
- auto suggestion: tier=C subsystem=Scene

## Assembly-CSharp/EditorEnumExtensions.cs
- 88 lines; kind=static class; type=EditorEnumExtensions; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=4
- ksp refs: Localizer=2, ResearchAndDevelopment=1; fan_in=1 fan_out=7
- methods(5): GetEditorFacility, GetFacility, GetSymmetryMethodName, ToEditor, ToFacility
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/EditorFacilities.cs
- 10 lines; kind=enum; type=EditorFacilities; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=6 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/EditorFacility.cs
- 11 lines; kind=enum; type=EditorFacility; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=40 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Scene

## Assembly-CSharp/EditorGeometryUtil.cs
- 106 lines; kind=static class; type=EditorGeometryUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Transform=5, Collider=5, GameObject=3, WheelCollider=1, LayerMask=1, Camera=1; unity_score=65
- ksp refs: Part=6; fan_in=2 fan_out=2
- methods(8): GetPartBoundsSeparation, GetPixelDistance, MirrorDirection, MirrorPos, MirrorRotation, TestPartBoundsIntersect, TestPartBoundsSeparate, isValidPartCollider
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/EditorLogic.cs **MUST-OPEN (>800 lines)**
- 5490 lines; kind=MonoBehaviour; type=EditorLogic; bases=EditorLogicBase; ancestry=EditorLogicBase/MonoBehaviour; ns=(global)
- usings: EditorGizmos, Expansions, Expansions.Missions, Expansions.Missions.Editor, Expansions.Missions.Runtime, Expansions.Serenity, Highlighting, KSP.Localization, KSP.UI, KSP.UI.Screens, PreFlightTests, Steamworks, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.SceneManagement, UnityEngine.UI
- unity: Transform=144, GameObject=74, Input.=44, Analytics=41, SetActive=35, Steam=30; unity_score=64
- ksp refs: Part=98, Localizer=82, GameEvents=81, InputLockManager=59, HighLogic=59, FlightGlobals=31; fan_in=99 fan_out=133
- methods(156): AnyTextFieldHasFocus, AreAllPartsConnected, Awake, CenterDragPlane, CheckCopiedSymmetryPersistentId, CheckSymPartsAttach, CopyActionGroups, CountAllSceneParts, CraftBrowseCancelled, CreatePartForInventoryUse, DelayedAbort, DelayedUnlock, DeletePart, DestroySelectedPart, DuplicatePart, FinalizeAnalytics, FirstNonSymmetricalParentFrom, FixedUpdate, FlagSetup, GetMissionPreFlightCheck, GetPartExistsFilter, GetStockPreFlightCheck, HeldPartIsStacked, InitAnalytics, LateUpdate, LoadShipFromFile, Lock, MissionCheckLaunchClamps, MissionCraftInOtherFolder, MissionCraftIsCreators, NameOrDescriptionFocused, NewShip, OnDestroy, OnDrawGizmosSelected, OnEditorScreenChange, OnInputLocksModified, OnMissionFlagSelect, OnModeChange, OnNewRootSelect, OnNewShipConfirm, OnNewShipDialogDismiss, OnPartListBackgroundTap, OnPartListIconTap, OnPartPurchased, OnPartVesselNamingChanged ...
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/EditorLogicBase.cs
- 342 lines; kind=MonoBehaviour; type=EditorLogicBase; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, UnityEngine
- unity: Transform=6, GameObject=4, Raycast/Query=3, GetComponent=3, SetActive=2, Renderer=2; unity_score=63
- ksp refs: Part=19, Localizer=17, GameSettings=12, AttachNode=6, InputLockManager=1, GameEvents=1; fan_in=3 fan_out=18
- methods(12): AssignAttachIcon, CacheLocalStrings, CreateSelectedPartIcon, FindPartSurface, FindPartsInChildren, PartInHierarchy, clearAttachNodes, displayAttachNodeIcons, handleAttachNodeIcons, handleChildNodeIcons, partRotationInputUpdate, partRotationResetUpdate
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/EditorMarker.cs
- 33 lines; kind=MonoBehaviour; type=EditorMarker; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, GameObject=2, MonoBehaviour=1, Lifecycle=1; unity_score=77
- ksp refs: Part=1; fan_in=3 fan_out=1
- methods(3): Update, UpdateDirection, UpdatePosition
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/EditorMarker_CoL.cs
- 152 lines; kind=MonoBehaviour; type=EditorMarker_CoL; bases=EditorMarker; ancestry=EditorMarker/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=6, SetActive=4, Lifecycle=1; unity_score=58
- ksp refs: EditorLogic=14, HighLogic=11, FlightGlobals=7, Part=4, Planetarium=3; fan_in=2 fan_out=8
- methods(4): Awake, FindCoL, UpdatePosition, recurseParts
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/EditorMarker_CoM.cs
- 73 lines; kind=MonoBehaviour; type=EditorMarker_CoM; bases=EditorMarker; ancestry=EditorMarker/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=6, Lifecycle=1; unity_score=61
- ksp refs: EditorLogic=9, HighLogic=4, Part=3, FlightGlobals=3; fan_in=3 fan_out=5
- methods(4): Awake, UpdatePosition, findCenterOfMass, recurseParts
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/EditorMarker_CoT.cs
- 93 lines; kind=MonoBehaviour; type=EditorMarker_CoT; bases=EditorMarker; ancestry=EditorMarker/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=1, Lifecycle=1; unity_score=42
- ksp refs: EditorLogic=12, HighLogic=9, FlightGlobals=4, Part=3; fan_in=2 fan_out=7
- methods(4): Awake, FindCoT, UpdatePosition, recurseParts
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/EditorPartListFilter.cs
- 39 lines; kind=class; type=EditorPartListFilter; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=14 fan_out=0
- methods(2): FilterList, ToString
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/EditorPartListFilterList.cs
- 118 lines; kind=class; type=EditorPartListFilterList; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(8): AddFilter, FilterList, GetEnumerator, GetFilterKey, GetFilterKeySingleOrNothing, GetFilteredList, RemoveFilter, ToString
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/EditorReRootUtil.cs
- 117 lines; kind=static class; type=EditorReRootUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: GameObject=2; unity_score=17
- ksp refs: Part=10, EditorLogic=2; fan_in=1 fan_out=3
- methods(6): GetRootCandidates, MakeRoot, StripInvalidSymmetrySets, StripInvalidSymmetrySets_recurseChildren, SymmetrySetIsValid, isRootCandidate
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/EditorScreen.cs
- 7 lines; kind=enum; type=EditorScreen; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=10 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/EditorState.cs
- 7 lines; kind=enum; type=EditorState; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/EditorToolsUI.cs
- 262 lines; kind=MonoBehaviour; type=EditorToolsUI; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Serenity, KSP.UI, KSP.UI.Screens, UnityEngine, UnityEngine.UI
- unity: SetActive=14, GameObject=14, UnityEngine.UI=4, Lifecycle=3, Coroutine=2, MonoBehaviour=1; unity_score=73
- ksp refs: EditorLogic=10, GameEvents=9, GameSettings=4, InputLockManager=2, ShipConstruct=1, Part=1; fan_in=1 fan_out=16
- methods(12): OnDestroy, OnEditorLoad, OnEditorRestart, OnEditorScreenChange, SetMode, Start, Update, onEditorPartEvent, onMoveButtonInput, onPlaceButtonInput, onRootButtonInput, onRotateButtonInput
- auto suggestion: tier=C subsystem=Editor

## Assembly-CSharp/EditorVesselOverlays.cs
- 143 lines; kind=MonoBehaviour; type=EditorVesselOverlays; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: SetActive=12, GameObject=12, Lifecycle=4, Instantiate/Destroy=4, UnityEngine.UI=3, MonoBehaviour=1; unity_score=83
- ksp refs: EditorLogic=7, GameEvents=4, Part=1; fan_in=1 fan_out=9
- methods(10): Awake, DisableMarkers, OnDestroy, OnEditorPartEvent, OnEditorRestart, Start, ToggleCoL, ToggleCoM, ToggleCoT, Update
- auto suggestion: tier=C subsystem=Editor

## Assembly-CSharp/EffectBehaviour.cs
- 152 lines; kind=MonoBehaviour; type=EffectBehaviour; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=15, Rigidbody=4, Time.=2, GetComponent=2, MonoBehaviour=1; unity_score=78
- ksp refs: Vector3d=5, Krakensbane=2, ConfigNode=2, Part=1; fan_in=20 fan_out=5
- methods(7): AddParticleEmitter, OffsetParticles, OnEvent, OnInitialize, OnLoad, OnSave, RemoveParticleEmitter
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/EffectDefinition.cs
- 12 lines; kind=attribute; type=EffectDefinition; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=16 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/EffectList.cs
- 260 lines; kind=class; type=EffectList; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: GetComponent=2, GameObject=1; unity_score=12
- ksp refs: ConfigNode=6, Part=2; fan_in=2 fan_out=5
- methods(11): ConstructEffectList, CreateEffectTypes, EffectsStartingWith, Event, GetEffect, GetEffectType, Initialize, LoadEffect, OnLoad, OnSave, PlayRandomEffect
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/EngineType.cs
- 12 lines; kind=enum; type=EngineType; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Engines

## Assembly-CSharp/EnumExtensions.cs
- 75 lines; kind=static class; type=EnumExtensions; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=0 fan_out=1
- methods(5): Description, GetDescriptions, GetName, ToStringCached, displayDescription
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/EnvMapAnimator.cs
- 29 lines; kind=MonoBehaviour; type=EnvMapAnimator; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine
- unity: Time.=3, TMPro=2, Shader=1, MonoBehaviour=1, Material=1, Lifecycle=1; unity_score=92
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): Awake, Start
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ESAMissionScreenTutorial.cs
- 105 lines; kind=ScenarioModule; type=ESAMissionScreenTutorial; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=16
- ksp refs: GameSettings=2, ConfigNode=2; fan_in=1 fan_out=4
- methods(12): CreateESAMissionScreenTutorialPages, ESAMissionScreenTutorialPageOne, ESAMissionScreenTutorialPageThree, ESAMissionScreenTutorialPageTwo, OnAssetSetup, OnDoneButtonClick, OnEnterESAMissionScreenTutorialPageOne, OnEnterESAMissionScreenTutorialPageThree, OnEnterESAMissionScreenTutorialPageTwo, OnLoad, OnSave, OnTutorialSetup
- auto suggestion: tier=A subsystem=Tutorials

## Assembly-CSharp/EVACamera.cs
- 108 lines; kind=MonoBehaviour; type=EVACamera; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=11, Time.=7, Input.=7, KeyCode=4, GameObject=3, Lifecycle=2; unity_score=90
- ksp refs: GameSettings=12, FlightGlobals=2, Vessel=1; fan_in=0 fan_out=3
- methods(2): FixedUpdate, Start
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/EVAConstructionModeController.cs
- 635 lines; kind=MonoBehaviour; type=EVAConstructionModeController; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, KSP.UI.TooltipTypes, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Transform=19, GameObject=12, Instantiate/Destroy=7, SetActive=6, UnityEngine.UI=5, TMPro=4; unity_score=67
- ksp refs: GameEvents=18, FlightGlobals=12, InputLockManager=5, GameSettings=5, Localizer=4, FlightCamera=4; fan_in=23 fan_out=29
- methods(26): Awake, CanOpenCargoPanel, CanOpenConstructionPanel, ClosePanel, DestroyHeldIcons, OnActionGroupsOpened, OnDestroy, OnEditorPartEvent, OnGameSceneLoadRequested, OnLoadedScene, OnPointerEnter, OnPointerExit, OnVesselChange, OpenCargoPanel, OpenConstructionPanel, RegisterAppButtonCargo, RegisterAppButtonConstruction, RemoveInvDisplayItem, SearchForInventoryParts, SetAppLauncherButtonVisibility, SpawnMarkerCamera, Start, TryGetDisplayedInventory, Update, UpdateDisplayedInventories, UpdateInfoLabels
- auto suggestion: tier=C subsystem=Kerbals

## Assembly-CSharp/EVAConstructionModeEditor.cs **MUST-OPEN (>800 lines)**
- 1724 lines; kind=MonoBehaviour; type=EVAConstructionModeEditor; bases=EditorLogicBase; ancestry=EditorLogicBase/MonoBehaviour; ns=(global)
- usings: EditorGizmos, Expansions, Expansions.Serenity, FinePrint.Utilities, Highlighting, KSP.Localization, KSP.UI, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Transform=82, GameObject=13, Audio=11, Input.=8, Raycast/Query=7, Instantiate/Destroy=7; unity_score=66
- ksp refs: FlightGlobals=47, Part=40, GameEvents=30, Vessel=29, GameSettings=26, AttachNode=10; fan_in=1 fan_out=55
- methods(62): AttachInput, AttachPart, Awake, BackupPart, BreakSymmetry, CanPartBeEdited, CannotInteract, ChangeCoordSpace, CheckAttach, CoordToggleInput, CopyActionGroups, CreatePartNode, DestroyBackupPart, DestroyHeldPart, DetachInput, DetachPart, DropAttachablePart, DropPartInput, FetchColliders, FixedUpdate, ForceDrop, GetPivotOffset, GetProtoVesselNode, GetRootPart, HighLightPossibleAttach, InputCheckDetachCompundPart, InterruptKerbalWelding, OnConstructionMode, OnConstructionModeChanged, OnCurrentMousePartChanged, OnDestroy, OnGameAboutToQuicksave, OnMapViewEntered, OnOffsetGizmoUpdate, OnOffsetGizmoUpdated, OnPartDropped, OnRotateGizmoUpdate, OnRotateGizmoUpdated, OnVesselUnloaded, PickupPart, PickupPartInput, PlayAudioClip, ProcessAttachNodes, PutPartBackInInventory, RemoveAttachNodes ...
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/EVAConstructionToolsUI.cs
- 164 lines; kind=MonoBehaviour; type=EVAConstructionToolsUI; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: SetActive=8, GameObject=8, UnityEngine.UI=4, Lifecycle=3, MonoBehaviour=1; unity_score=73
- ksp refs: GameSettings=3, InputLockManager=1, GameEvents=1; fan_in=1 fan_out=7
- methods(11): Awake, HideModeTools, OnChangeModeBlocked, OnDestroy, SetMode, ShowModeTools, ToggleButtonListeners, Update, onMoveButtonInput, onPlaceButtonInput, onRotateButtonInput
- auto suggestion: tier=C subsystem=Kerbals

## Assembly-CSharp/EVAConstructionUtil.cs
- 15 lines; kind=class; type=EVAConstructionUtil; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: PhysicsGlobals=2, Vessel=1; fan_in=2 fan_out=2
- methods(1): GetConstructionGee
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/EVAResourceContainer.cs
- 113 lines; kind=PartModule; type=EVAResourceContainer; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Coroutine=5, Resources.=3, Transform=2, Time.=2, Lifecycle=1; unity_score=64
- ksp refs: KSPField=6, PartResource=5, FlightGlobals=4, Localizer=3, KSPEvent=3, GameEvents=2; fan_in=0 fan_out=10
- methods(8): GetResourceExternalEvent, OnDestroy, OnStart, PutResourceExternalEvent, StopTransfer, onPartMenuOpen, transferResources, updateModuleUI
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/EventData.cs
- 428 lines; kind=class; type=EventData; bases=BaseGameEvent; ancestry=BaseGameEvent; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=21
- ksp refs: none; fan_in=28 fan_out=2
- methods(4): Add, Fire, OnEvent, Remove
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/EventDataModifier.cs
- 118 lines; kind=class; type=EventDataModifier; bases=BaseGameEvent; ancestry=BaseGameEvent; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=19
- ksp refs: none; fan_in=1 fan_out=2
- methods(5): Add, DefaultModifier, Fire, OnEvent, Remove
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/EventDoubleMax.cs
- 115 lines; kind=class; type=EventDoubleMax; bases=BaseGameEvent; ancestry=BaseGameEvent; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=20
- ksp refs: none; fan_in=0 fan_out=2
- methods(4): Add, Fire, OnEvent, Remove
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/EventDoubleMin.cs
- 115 lines; kind=class; type=EventDoubleMin; bases=BaseGameEvent; ancestry=BaseGameEvent; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=20
- ksp refs: none; fan_in=0 fan_out=2
- methods(4): Add, Fire, OnEvent, Remove
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/EventExplanationTutorialStep.cs
- 129 lines; kind=class; type=EventExplanationTutorialStep; bases=IntermediateTutorialPageStep; ancestry=IntermediateTutorialPageStep/TutorialPageConfig; ns=(global)
- usings: Expansions.Missions.Editor, UnityEngine
- unity: GameObject=6; unity_score=27
- ksp refs: none; fan_in=1 fan_out=10
- methods(9): AddTutorialStepConfig, CanShowSetEventSelector, OnEnterSetEventNodeSetting, OnEnterSetEventNodeSettingHelper, OnLeaveSetEventNodeSetting, OnLeaveSetEventNodeSettingHelper, OnSelectedGameObjectChangeSetEvent, OnUpdateSetEventNodeSetting, OnUpdateSetEventNodeSettingHelper
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/EventFloatMax.cs
- 115 lines; kind=class; type=EventFloatMax; bases=BaseGameEvent; ancestry=BaseGameEvent; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=20
- ksp refs: none; fan_in=0 fan_out=2
- methods(4): Add, Fire, OnEvent, Remove
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/EventFloatMin.cs
- 115 lines; kind=class; type=EventFloatMin; bases=BaseGameEvent; ancestry=BaseGameEvent; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=20
- ksp refs: none; fan_in=0 fan_out=2
- methods(4): Add, Fire, OnEvent, Remove
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/EventIntMax.cs
- 115 lines; kind=class; type=EventIntMax; bases=BaseGameEvent; ancestry=BaseGameEvent; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=20
- ksp refs: none; fan_in=0 fan_out=2
- methods(4): Add, Fire, OnEvent, Remove
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/EventIntMin.cs
- 115 lines; kind=class; type=EventIntMin; bases=BaseGameEvent; ancestry=BaseGameEvent; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=20
- ksp refs: none; fan_in=0 fan_out=2
- methods(4): Add, Fire, OnEvent, Remove
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/EventReport.cs
- 27 lines; kind=class; type=EventReport; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=2; fan_in=20 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/EventSelectionTutorialStep.cs
- 118 lines; kind=class; type=EventSelectionTutorialStep; bases=IntermediateTutorialPageStep; ancestry=IntermediateTutorialPageStep/TutorialPageConfig; ns=(global)
- usings: Expansions.Missions.Editor, UnityEngine
- unity: GameObject=6; unity_score=29
- ksp refs: none; fan_in=1 fan_out=11
- methods(11): AddTutorialStepConfig, IsEventBoxSelected, IsScienceNodeSelected, OnEnterTickEventNode, OnEnterTickEventNodeHelper, OnLeaveOnEnterTickEventNode, OnLeaveOnEnterTickEventNodeHelper, OnSelectDuringTickEvent, OnUpdateTickEventNode, OnUpdateTickEventNodeHelper, ShowEventBoxSelector
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/EventStartTutorialStep.cs
- 15 lines; kind=class; type=EventStartTutorialStep; bases=IntermediateTutorialPageStep; ancestry=IntermediateTutorialPageStep/TutorialPageConfig; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): AddTutorialStepConfig
- auto suggestion: tier=A subsystem=Tutorials

## Assembly-CSharp/EventValueComparison.cs
- 117 lines; kind=class; type=EventValueComparison; bases=BaseGameEvent; ancestry=BaseGameEvent; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=1; unity_score=19
- ksp refs: none; fan_in=1 fan_out=2
- methods(5): Add, OnCompare, OnEvent, Remove, Update
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/EventValueOperation.cs
- 114 lines; kind=class; type=EventValueOperation; bases=BaseGameEvent; ancestry=BaseGameEvent; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=1; unity_score=20
- ksp refs: none; fan_in=1 fan_out=2
- methods(5): Add, OnEvent, OnOperation, Remove, Update
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/EventValueWrapper.cs
- 64 lines; kind=class; type=EventValueWrapper; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: Lifecycle=1; unity_score=11
- ksp refs: none; fan_in=1 fan_out=0
- methods(1): Update
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/EventVoid.cs
- 110 lines; kind=class; type=EventVoid; bases=BaseGameEvent; ancestry=BaseGameEvent; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=20
- ksp refs: none; fan_in=2 fan_out=2
- methods(4): Add, Fire, OnEvent, Remove
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/ExperimentResultDialogPage.cs
- 175 lines; kind=class; type=ExperimentResultDialogPage; bases=-; ancestry=-; ns=(global)
- usings: CommNet, KSP.Localization
- unity: none; unity_score=2
- ksp refs: ResearchAndDevelopment=9, Localizer=8, HighLogic=6, FlightGlobals=4, Part=2, Vessel=1; fan_in=3 fan_out=13
- methods(1): UpdatePageLabValue
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ExperimentSituations.cs
- 17 lines; kind=enum; type=ExperimentSituations; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=15 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ExperimentTransfer.cs
- 46 lines; kind=MonoBehaviour; type=ExperimentTransfer; bases=PartItemTransfer; ancestry=PartItemTransfer/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens.Flight.Dialogs, UnityEngine
- unity: GetComponent=1, GameObject=1; unity_score=41
- ksp refs: Part=6, Localizer=1; fan_in=2 fan_out=8
- methods(5): Create, IsSemiValidPart, IsValidPart, OnPartSelect, OnSrcPartSelect
- auto suggestion: tier=B subsystem=Science

## Assembly-CSharp/ExperimentUsageReqs.cs
- 9 lines; kind=enum; type=ExperimentUsageReqs; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ExplosionDestroyer.cs
- 21 lines; kind=MonoBehaviour; type=ExplosionDestroyer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Time.=2, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1; unity_score=77
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Awake, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ExplosionSpawner.cs
- 34 lines; kind=MonoBehaviour; type=ExplosionSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=4, GameObject=4, Time.=2, Lifecycle=2, Instantiate/Destroy=2, MonoBehaviour=1; unity_score=92
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Awake, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ExplosionTester.cs
- 20 lines; kind=MonoBehaviour; type=ExplosionTester; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Raycast/Query=3, Input.=2, Physics.=1, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1; unity_score=95
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ExtendedInput.cs
- 126 lines; kind=MonoBehaviour; type=ExtendedInput; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Input.=7, KeyCode=3, Time.=1, MonoBehaviour=1, Lifecycle=1; unity_score=68
- ksp refs: none; fan_in=4 fan_out=1
- methods(7): DetectKeyDown, GetKey, GetKeyDown, GetKeyUp, IsPressed, LateUpdate, WasPressed
- auto suggestion: tier=B subsystem=Input

## Assembly-CSharp/FacilityScreenFitter.cs
- 24 lines; kind=MonoBehaviour; type=FacilityScreenFitter; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: UnityEngine.UI=4, Lifecycle=2, Transform=1, MonoBehaviour=1; unity_score=88
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Awake, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/FieldAttribute.cs
- 27 lines; kind=attribute; type=FieldAttribute; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=12
- ksp refs: Localizer=1; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/FlagBrowser.cs
- 291 lines; kind=MonoBehaviour; type=FlagBrowser; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Contracts.Agents, KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=20, GameObject=3, Instantiate/Destroy=2, GetComponent=2, MonoBehaviour=1, Lifecycle=1; unity_score=69
- ksp refs: GameDatabase=10, Localizer=6; fan_in=8 fan_out=23
- methods(9): Accept, CreateFlagBrowser, Dismiss, FlagSelectedCallback, ProcessButtons, ProcessFlagFolder, Start, ToggleAllOff, Toggled
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/FlagBrowserButton.cs
- 85 lines; kind=MonoBehaviour; type=FlagBrowserButton; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, Texture=2, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1; unity_score=61
- ksp refs: InputLockManager=4, HighLogic=3, GameEvents=2; fan_in=3 fan_out=5
- methods(6): OnFlagCancel, OnFlagSelect, SetFlag, Setup, SpawnBrowser, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/FlagBrowserGUIButton.cs
- 97 lines; kind=class; type=FlagBrowserGUIButton; bases=DialogGUIButton; ancestry=DialogGUIButton/DialogGUIBase; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: IMGUI=9, UnityEngine.UI=3, Texture=2, GetComponent=2, Instantiate/Destroy=1, GameObject=1; unity_score=68
- ksp refs: GameDatabase=1; fan_in=2 fan_out=6
- methods(4): Draw, OnFlagCancel, OnFlagSelect, SpawnBrowser
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/FlagDecal.cs
- 171 lines; kind=PartModule; type=FlagDecal; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Transform=7, Material=4, GameObject=3, Renderer=2, Texture=1, Lifecycle=1; unity_score=68
- ksp refs: HighLogic=5, GameEvents=4, EditorLogic=4, KSPField=3, Part=2, KSPEvent=2; fan_in=2 fan_out=10
- methods(11): FlipTexture, GetModuleDisplayName, MirrorFlag, OnAwake, OnDestroy, OnStart, OnVariantApplied, ToggleFlag, UpdateDisplay, UpdateFlagTexture, updateFlag
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FlagDecalBackground.cs
- 660 lines; kind=PartModule; type=FlagDecalBackground; bases=FlagDecal,IThumbnailSetup,IPartMassModifier,IPartCostModifier; ancestry=FlagDecal/PartModule/MonoBehaviour; ns=(global)
- usings: Expansions, KSP.Localization, UnityEngine
- unity: GameObject=5, Transform=3, Material=2, GetComponent=2, Shader=1, SetActive=1; unity_score=34
- ksp refs: EditorLogic=14, HighLogic=13, Part=10, KSPField=7, GameEvents=6, ConfigNode=5; fan_in=5 fan_out=33
- methods(31): AssumeSnapshotPosition, CheckSymmetry, DisplayingPortrait_OnValueModified, DisplayingPortrait_OnValueSymmetryModified, EnableCurrentFlagMesh, FlagMeshIndex, GetModuleCost, GetModuleCostChangeWhen, GetModuleDisplayName, GetModuleMass, GetModuleMassChangeWhen, HasSurfaceAttachedParts, IsFlagOrientationCounterPartAvailable, LoadFlagsTransformAndRenderer, OnAwake, OnDestroy, OnEditorPartEvent, OnInventoryModeDisable, OnLoad, OnPartDetached, OnSave, OnStart, OnVariantApplied, SetFlag, SetFlagBrowserTexture, ThumbSuffix, ToolboxSetFlagTexture, UnsubscribeToFlagSelect, UpdateFlagRenderers, UpdateUIChooseOptions, updateFlag
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FlagMesh.cs
- 28 lines; kind=class; type=FlagMesh; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Transform=1, Renderer=1; unity_score=44
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FlagNameManager.cs
- 81 lines; kind=class; type=FlagNameManager; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=2
- methods(4): DisplayNames, HasSizeNames, Initialize, SizeNames
- auto suggestion: tier=A subsystem=Misc

## Assembly-CSharp/FlagOrientation.cs
- 5 lines; kind=enum; type=FlagOrientation; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FlagPoleFacility.cs
- 49 lines; kind=MonoBehaviour; type=FlagPoleFacility; bases=SpaceCenterBuilding; ancestry=SpaceCenterBuilding/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Instantiate/Destroy=1, GetComponent=1, GameObject=1; unity_score=50
- ksp refs: HighLogic=6, InputLockManager=5, Localizer=3, GameEvents=2; fan_in=0 fan_out=11
- methods(5): OnClicked, OnFlagCancel, OnFlagSelect, OnPopupWarningDismiss, SpawnBrowser
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/FlagPoleLaunchpad.cs
- 43 lines; kind=MonoBehaviour; type=FlagPoleLaunchpad; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, Texture=1, Renderer=1, MonoBehaviour=1, Material=1, Animation=1; unity_score=78
- ksp refs: GameEvents=4, HighLogic=2, GameDatabase=1; fan_in=0 fan_out=4
- methods(4): OnDestroy, OnFlagSelect, OnSceneChange, Start
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/FlagSite.cs
- 529 lines; kind=PartModule; type=FlagSite; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens.Flight.Dialogs, UnityEngine
- unity: Joint=9, GameObject=6, SetActive=5, Lifecycle=5, Transform=4, GetComponent=4; unity_score=66
- ksp refs: Localizer=6, KSPField=6, InputLockManager=6, GameEvents=4, FlightGlobals=4, Vessel=3; fan_in=4 fan_out=29
- methods(29): AcceptSiteRename, CancelSiteRename, CreateFlag, DismissSiteRename, FixedUpdate, LateUpdate, MakeInvisible, MakeVisible, OnDestroy, OnJointBreak, OnLoad, OnPlacementComplete, OnPlacementFail, OnSave, OnVesselPack, OnVesselUnpack, PickUp, ReadPlaque, RenameSite, ScanSurroundingTerrain, SetJoint, SetupFSM, Start, TakeDown, UnsetJoint, Update, findRigidbodyInVessel, onFlagPlaqueDialogDismiss, onGameSceneLoadRequested
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/FlightAutoSave.cs
- 65 lines; kind=MonoBehaviour; type=FlightAutoSave; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Lifecycle=3, Coroutine=3, MonoBehaviour=2, SetActive=1, Instantiate/Destroy=1, GameObject=1; unity_score=73
- ksp refs: HighLogic=3, GameSettings=3, Localizer=2, FlightGlobals=2, Vessel=1; fan_in=0 fan_out=9
- methods(4): Awake, OnDestroy, PersistentSave, Start
- auto suggestion: tier=B subsystem=Save

## Assembly-CSharp/FlightCamera.cs **MUST-OPEN (>800 lines)**
- 1182 lines; kind=MonoBehaviour; type=FlightCamera; bases=MonoBehaviour,IKSPCamera; ancestry=MonoBehaviour; ns=(global)
- usings: CameraFXModules, Highlighting, KSP.Localization, KSP.UI, UnityEngine, UnityEngine.EventSystems, UnityEngine.SceneManagement
- unity: Transform=56, GameObject=10, Time.=9, GetComponent=7, Input.=5, Camera=5; unity_score=63
- ksp refs: FlightGlobals=38, GameSettings=29, Vessel=18, Part=11, GameEvents=8, InputLockManager=3; fan_in=41 fan_out=32
- methods(53): ActivateUpdate, AdjustDistanceToFit, Awake, ClearTarget, CycleCameraHighlighter, DeactivateUpdate, DisableCamera, DisableCameraHighlighter, EnableCamera, GetAutoModeForVessel, GetCameraFoR, GetCameraTransform, GetChaseFoR, GetPivot, LateUpdate, MinDistanceBackaway, OnCameraChange, OnDestroy, OnLevelLoaded, OnNavigatorHandoff, OnNavigatorRequestControl, OnNavigatorTakeOver, OnSceneLoaded, OnSceneSwitch, OnTargetDestroyed, OnVesselChange, ResetFoV, ResumeFoV, SetCamCoordsFromPosition, SetDefaultFoV, SetDistance, SetDistanceImmediate, SetFoV, SetMode, SetModeImmediate, SetNextMode, SetTarget, SetTargetNone, SetTargetPart, SetTargetTransform, SetTargetVessel, Start, Startup, TargetActiveVessel, TrackIRisActive ...
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/FlightCoMTracker.cs
- 105 lines; kind=MonoBehaviour; type=FlightCoMTracker; bases=MonoBehaviour,ITargetable; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Transform=4, Coroutine=2, MonoBehaviour=1, Lifecycle=1, GetComponent=1, GameObject=1; unity_score=59
- ksp refs: FlightGlobals=4, PartModule=2, Vessel=1, OrbitDriver=1, Orbit=1, Localizer=1; fan_in=2 fan_out=9
- methods(15): Create, FixedUpdate, GetActiveTargetable, GetDisplayName, GetFwdVector, GetName, GetObtVelocity, GetOrbit, GetOrbitDriver, GetSrfVelocity, GetTargetingMode, GetTransform, GetVessel, MakeTarget, makeTgtWhenReady
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/FlightCtrlState.cs
- 250 lines; kind=class; type=FlightCtrlState; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=2
- ksp refs: ConfigNode=2, GameSettings=1; fan_in=8 fan_out=4
- methods(11): CopyFrom, GetPYR, GetWheels, GetXYZ, Load, MaskedCopyFrom, Neutralize, NeutralizeAll, NeutralizeStick, ResetTrim, Save
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/FlightDriver.cs
- 630 lines; kind=MonoBehaviour; type=FlightDriver; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Transform=13, MonoBehaviour=9, Time.=5, GameObject=4, Lifecycle=3, Coroutine=2; unity_score=48
- ksp refs: HighLogic=25, GameEvents=19, FlightGlobals=17, InputLockManager=7, Game=7, FloatingOrigin=5; fan_in=51 fan_out=43
- methods(21): Awake, FinalizeAnalytics, FocusTargetVessel, InitAnalytics, OnApplicationFocus, OnCameraChange, OnConstructionModeChange, OnCrewBoardVessel, OnCrewOnEva, OnDestroy, OnModeChange, PostInit, RemoveSavedVessel, ReturnToEditor, RevertToLaunch, RevertToPrelaunch, SetPause, Start, StartAndFocusVessel, StartWithNewLaunch, setStartupNewVessel
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/FlightEndModes.cs
- 7 lines; kind=enum; type=FlightEndModes; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/FlightEVA.cs
- 296 lines; kind=MonoBehaviour; type=FlightEVA; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Transform=25, GetComponent=12, GameObject=9, Rigidbody=8, Raycast/Query=4, Coroutine=4; unity_score=85
- ksp refs: Part=15, FlightGlobals=10, KerbalEVA=9, GameEvents=9, ProtoCrewMember=7, Vessel=4; fan_in=4 fan_out=22
- methods(12): Awake, HatchIsObstructed, HatchIsObstructedMore, OnDestroy, SpawnEVA, SpawnKerbalEVADelegate, SwitchToEVAVesselWhenReady, _Spawn, hatchInsideFairing, onEVAAborted, onGoForEVA, spawnEVA
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/FlightEvents.cs
- 22 lines; kind=enum; type=FlightEvents; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=14 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/FlightGlobals.cs **MUST-OPEN (>800 lines)**
- 2003 lines; kind=MonoBehaviour; type=FlightGlobals; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UniLinq, UnityEngine, UnityEngine.SceneManagement
- unity: Transform=21, GameObject=14, Lifecycle=5, SendMessage/Invoke=3, SceneManager=3, Coroutine=3; unity_score=37
- ksp refs: Vector3d=62, Vessel=56, GameEvents=44, CelestialBody=42, Part=19, Localizer=18; fan_in=431 fan_out=38
- methods(88): ActiveVesselGoOffRails, AddVessel, Awake, CheckPartpersistentId, CheckProtoPartSnapShotpersistentId, CheckVesselpersistentId, ClearAllVessels, ClearDeadVessels, ClearInverseRotation, ClearToSave, ClearpersistentIdDictionaries, CountSpaceObjects, CountUntrackedSpaceObjects, CycleSpeedModes, FindLoadedPart, FindNearestControllableVessel, FindNearestVesselWhere, FindPartByID, FindProtoPartByID, FindUnloadedPart, FindVessel, FixedUpdate, ForceSetActiveVessel, GGetCometPerturberDisplayName, GetBodyByName, GetBodyIndex, GetCometPerturber, GetCometPerturberIndex, GetCometPerturberName, GetDisplaySpeed, GetDisplayVelocity, GetFoR, GetHomeBody, GetHomeBodyDisplayName, GetHomeBodyIndex, GetHomeBodyName, GetMidSystemPlanet, GetNotClearToSaveStatusReason, GetPartPointerUpwardsCached, GetPartUpwardsCached, GetSqrAltitude, GetUniquepersistentId, HookVesselEvents, OnDestroy, OnGameUnpaused ...
- auto suggestion: tier=B subsystem=Vessel

## Assembly-CSharp/FlightInputCallback.cs
- 1 lines; kind=delegate; type=FlightInputCallback; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/FlightInputHandler.cs
- 711 lines; kind=MonoBehaviour; type=FlightInputHandler; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Time.=25, UnityEngine.UI=18, Lifecycle=5, MonoBehaviour=3, Input.=3, KeyCode=2; unity_score=55
- ksp refs: GameSettings=115, InputLockManager=28, FlightGlobals=11, Localizer=5, Vessel=4, GameEvents=3; fan_in=26 fan_out=21
- methods(11): Awake, FixedUpdate, OnApplicationFocus, OnDestroy, OnVesselOverrideGroupChanged, ProcessAxis, ResumeVesselCtrlState, SetLaunchCtrlState, SetNeutralControls, Start, Update
- auto suggestion: tier=C subsystem=Input

## Assembly-CSharp/FlightIntegrator.cs **MUST-OPEN (>800 lines)**
- 2776 lines; kind=VesselModule; type=FlightIntegrator; bases=VesselModule; ancestry=VesselModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Rigidbody=29, Collision/Force=11, Transform=5, GetComponent=5, Lifecycle=3, Raycast/Query=2; unity_score=38
- ksp refs: PhysicsGlobals=77, Part=72, GameSettings=30, GameEvents=14, Vessel=13, FlightGlobals=12; fan_in=5 fan_out=31
- methods(67): ApplyAeroDrag, ApplyAeroLift, CalcConvectiveCoefficient, CalculateAerodynamicArea, CalculateAnalyticTemperature, CalculateAreaExposed, CalculateAreaRadiative, CalculateAtmosphericDensity, CalculateBackgroundRadiationTemperature, CalculateConstantsAtmosphere, CalculateConstantsVacuum, CalculateConvectiveCoefficient, CalculateConvectiveCoefficientMach, CalculateConvectiveCoefficientNewtonian, CalculateDensityThermalLerp, CalculateDragValue, CalculateDragValue_Conic, CalculateDragValue_Cube, CalculateDragValue_Cylindrical, CalculateDragValue_Spherical, CalculatePressure, CalculateShockTemperature, CalculateSunBodyFlux, CheckThermalGraph, DragCubeSetupAndPartAeroStats, FixedUpdate, GetBodyArea, GetOrder, GetPhysicslessChildMass, GetSunArea, HookVesselEvents, Integrate, IntegratePhysicalObjects, OnDestroy, OnDrawGizmosSelected, OnLoadVessel, OnPartEvent, OnPartEventFromToAction, OnPartEventTargetAction, OnStart, OnUnloadVessel, OnVesselEvent, PrecalcConduction, PrecalcConvection, PrecalcRadiation ...
- auto suggestion: tier=B subsystem=Aero

## Assembly-CSharp/FlightLog.cs
- 367 lines; kind=class; type=FlightLog; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2, Orbit=1; fan_in=18 fan_out=2
- methods(14): AddEntry, AddEntryUnique, AddFlight, Clear, CreateCopy, GetDistinctTargets, GetDistinctTypes, GetEntries, GetFlights, HasEntry, Last, Load, MergeWith, Save
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/FlightLogger.cs
- 401 lines; kind=MonoBehaviour; type=FlightLogger; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Dialogs, UnityEngine
- unity: Lifecycle=5, Time.=4, Input.=4, SendMessage/Invoke=3, KeyCode=3, Coroutine=2; unity_score=51
- ksp refs: Localizer=30, GameEvents=30, FlightGlobals=12, Vessel=7, HighLogic=6, Part=5; fan_in=10 fan_out=21
- methods(26): AttemptVesselSwitch, FixedUpdate, IgnoreGeeForces, LateUpdate, LogEvent, OnDestroy, ResumeGForceReading, Start, Update, checkMissionEnd, getMissionStats, onCollision, onCrash, onCrashSplashdown, onCrewBoardVessel, onCrewKilled, onCrewOnEva, onJointBreak, onLaunch, onOverG, onOverPressure, onOverheat, onSplashDamage, onStageSeparation, onUndock, waitAndEndFlight
- auto suggestion: tier=B subsystem=Vessel

## Assembly-CSharp/FlightOverlays.cs
- 104 lines; kind=MonoBehaviour; type=FlightOverlays; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens.Flight, UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1; unity_score=34
- ksp refs: GameSettings=11, Localizer=10, PhysicsGlobals=8, FlightGlobals=3, HighLogic=1; fan_in=1 fan_out=11
- methods(3): CacheLocalStrings, Start, Update
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/FlightReflectionProbe.cs
- 261 lines; kind=MonoBehaviour; type=FlightReflectionProbe; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.Rendering
- unity: QualitySettings/SystemInfo=4, Transform=3, Lifecycle=3, LayerMask=3, GetComponent=3, Renderer=2; unity_score=64
- ksp refs: FlightGlobals=5, GameSettings=2, GameEvents=2, HighLogic=1; fan_in=1 fan_out=5
- methods(11): Enable, ForceRender, GetSkyColor, OnDestroy, OnSettingsUpdate, Sign, Spawn, Start, Update, VectorMatrixMultiplication, expScale
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FlightState.cs
- 192 lines; kind=class; type=FlightState; bases=-; ancestry=-; ns=(global)
- usings: CommNet, FinePrint.Utilities, UnityEngine
- unity: GameObject=1; unity_score=9
- ksp refs: Vessel=5, ProtoVessel=5, FlightGlobals=4, ConfigNode=4, GameSettings=3, Planetarium=2; fan_in=9 fan_out=14
- methods(3): ContainsFlightID, Load, Save
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/FlightUIElements.cs
- 11 lines; kind=enum; type=FlightUIElements; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/FlightUIMode.cs
- 8 lines; kind=enum; type=FlightUIMode; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=13 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/FlightUIModeController.cs
- 703 lines; kind=MonoBehaviour; type=FlightUIModeController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: FlightUIModeControllerUtil, Highlighting, KSP.UI, KSP.UI.Screens, KSP.UI.Screens.Flight, UnityEngine, UnityEngine.UI
- unity: Transform=14, GameObject=6, Lifecycle=4, UnityEngine.UI=3, Coroutine=2, Screen/Cursor=1; unity_score=46
- ksp refs: GameSettings=33, InputLockManager=9, GameEvents=7, MapView=5; fan_in=17 fan_out=19
- methods(25): AddModeCheckCallback, AddModeSwitchCallback, Awake, ChangeTabState, FinishedAllTransitionsCallback, NextAlarmToggle, OnDestroy, OnDockingPress, OnManNodeSelected, OnManeuverPress, OnMapPress, OnMapViewEnter, OnMapViewExit, OnNextAlarmToggle, OnStagingPress, RemoveModeCheckCallback, RemoveModeSwitchCallback, SetLastMode, SetMode, SetNavBallHPos, SetUIElementScale, SetUIElementScale_ModeControls, Start, Update, UpdateHighlighting
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/FlightVesselOverlays.cs
- 282 lines; kind=MonoBehaviour; type=FlightVesselOverlays; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=15, SetActive=6, Lifecycle=5, Transform=4, Instantiate/Destroy=4, Camera=3; unity_score=69
- ksp refs: Vessel=16, FlightGlobals=9, GameEvents=4, HighLogic=3, FlightCamera=3; fan_in=1 fan_out=12
- methods(19): AddColMarker, AddComMarker, AddCotMarker, Awake, OnDestroy, OnDisable, OnVesselLoaded, OnVesselUnloaded, SetCamera, Start, TerminateColMarkers, TerminateComMarkers, TerminateCotMarkers, ToggleCoL, ToggleCoM, ToggleCoT, Update, ValidVesselOverride, ValidVesselType
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/FloatArrayMaterialProperty.cs
- 17 lines; kind=class; type=FloatArrayMaterialProperty; bases=MaterialPropertyExtensions.MaterialProperty; ancestry=MaterialProperty; ns=(global)
- usings: UnityEngine
- unity: Material=1; unity_score=29
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): Apply
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FloatCurve.cs
- 191 lines; kind=class; type=FloatCurve; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: UnityEngine
- unity: AnimationCurve=7, Animation=2; unity_score=28
- ksp refs: FloatCurve=3, ConfigNode=2; fan_in=32 fan_out=2
- methods(5): Add, Evaluate, FindMinMaxValue, Load, Save
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/FloatingOrigin.cs
- 368 lines; kind=MonoBehaviour; type=FloatingOrigin; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=9, Transform=8, GameObject=4, Lifecycle=3, Rigidbody=2, Time.=1; unity_score=58
- ksp refs: Vector3d=30, FlightGlobals=18, GameEvents=7, TimeWarp=4, Vessel=3, FloatingOrigin=2; fan_in=17 fan_out=13
- methods(14): Awake, FixedUpdate, OnCameraChange, OnDestroy, OnLevelLoaded, OnMapExited, RegisterParticleSystem, ResetOffset, ResetTerrainShaderOffset, SetOffset, SetOutOfFrameOffset, SetSafeToEngage, UnregisterParticleSystem, setOffset
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/FloatMaterialProperty.cs
- 17 lines; kind=class; type=FloatMaterialProperty; bases=MaterialPropertyExtensions.MaterialProperty; ancestry=MaterialProperty; ns=(global)
- usings: UnityEngine
- unity: Shader=1, Material=1; unity_score=44
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): Apply
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FloatTypeParser.cs
- 20 lines; kind=class; type=FloatTypeParser; bases=ITypeParser; ancestry=ITypeParser; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(2): Convert, Parse
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/FollowRot.cs
- 24 lines; kind=MonoBehaviour; type=FollowRot; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=6, MonoBehaviour=1, Lifecycle=1; unity_score=83
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): LateUpdate
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/FontLoader.cs
- 173 lines; kind=MonoBehaviour; type=FontLoader; bases=LoadingSystem; ancestry=LoadingSystem/MonoBehaviour; ns=(global)
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.Networking
- unity: Coroutine=9, TMPro=4, AssetBundle=4, WWW/WebRequest=1, Texture=1, Material=1; unity_score=72
- ksp refs: Localizer=1; fan_in=0 fan_out=7
- methods(12): AddGameSubFont, AddMenuSubFont, ChangeFontLanguage, FindFonts, IsReady, LoadBundle, LoadFonts, LoadWeight, ProgressFraction, ProgressTitle, StartLoad, SwitchFontLanguage
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/FoRModes.cs
- 11 lines; kind=enum; type=FoRModes; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/FuelFlowOverlay.cs
- 471 lines; kind=MonoBehaviour; type=FuelFlowOverlay; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: CompoundParts, RUI.Algorithms, TMPro, UnityEngine, Vectrosity
- unity: GameObject=30, Transform=23, Instantiate/Destroy=9, TMPro=8, Coroutine=8, LayerMask=4; unity_score=79
- ksp refs: Part=28, GameEvents=17, EditorLogic=4, HighLogic=2, ShipConstruct=1, FlightGlobals=1; fan_in=2 fan_out=13
- methods(21): Awake, Calculate, ClearOverlay, Connect, CreateText, DespawnEvent, InternalSpawn, OnDestroy, OnPartDestroyed, RedrawEvent, RedrawOverlay, RespawnEvent, RespawnOverlay, SpawnOverlay, Update, UpdateDaemon, UpdateLine, UpdateText, isConsumer, isConsumerOrProvider, isProvider
- auto suggestion: tier=C subsystem=Resources

## Assembly-CSharp/Funding.cs
- 207 lines; kind=ScenarioModule; type=Funding; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions, KSP.UI.Screens, KSP.UI.Screens.SpaceCenter.MissionSummaryDialog, UnityEngine
- unity: Lifecycle=1, GameObject=1; unity_score=27
- ksp refs: GameEvents=17, HighLogic=8, ProtoPartSnapshot=2, Funding=2, ConfigNode=2, AvailablePart=2; fan_in=33 fan_out=30
- methods(13): AddFunds, CanAfford, OnAwake, OnCurrenciesModified, OnDestroy, OnLoad, OnSave, SetFunds, onCrewHired, onPartPurchased, onUpgradePurchased, onVesselRecoveryProcessing, onVesselRollout
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/FundsWidget.cs
- 38 lines; kind=MonoBehaviour; type=FundsWidget; bases=CurrencyWidget; ancestry=CurrencyWidget/MonoBehaviour; ns=(global)
- usings: KSP.UI.Screens
- unity: Lifecycle=2; unity_score=45
- ksp refs: Funding=3, GameEvents=2, HighLogic=1; fan_in=0 fan_out=5
- methods(5): Awake, DelayedStart, OnAboutToStart, OnDestroy, onFundsChanged
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/FXCamera.cs
- 295 lines; kind=MonoBehaviour; type=FXCamera; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=23, Transform=9, Texture=7, Instantiate/Destroy=7, Camera=5, Lifecycle=4; unity_score=85
- ksp refs: GameEvents=8, FlightCamera=6, GameSettings=2, Vessel=1, FlightGlobals=1; fan_in=2 fan_out=7
- methods(11): ApplyObliqueness, Awake, LateUpdate, OnCameraChange, OnCameraDistanceAdjustedToFitVessel, OnDestroy, OnGUI, OnPreRender, OnSceneSwitch, OnSettingsUpdate, Start
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/FXCurve.cs
- 152 lines; kind=class; type=FXCurve; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: AnimationCurve=6, Animation=2; unity_score=28
- ksp refs: ConfigNode=2; fan_in=10 fan_out=1
- methods(6): AddKeyframe, CompileCurve, Load, Save, Value, float
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FXDepthCamera.cs
- 53 lines; kind=MonoBehaviour; type=FXDepthCamera; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=3, Shader=1, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1, Camera=1; unity_score=74
- ksp refs: GameEvents=4, FlightCamera=3, Vessel=1; fan_in=1 fan_out=4
- methods(6): LateUpdate, OnCameraDistanceAdjustedToFitVessel, OnDestroy, OnPreRender, OnSceneSwitch, Start
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/FXFadeByPressure.cs
- 47 lines; kind=MonoBehaviour; type=FXFadeByPressure; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=5, Lifecycle=2, GetComponent=2, Transform=1, MonoBehaviour=1; unity_score=85
- ksp refs: FlightGlobals=2, HighLogic=1; fan_in=0 fan_out=3
- methods(2): Start, Update
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/FXGroup.cs
- 363 lines; kind=class; type=FXGroup; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=10, Light=4, Coroutine=4, Audio=4, Time.=3; unity_score=43
- ksp refs: GameSettings=1; fan_in=12 fan_out=2
- methods(10): Burst, SetLatch, SetPower, SetPowerLatch, SetVisualMinMax, Unlatch, VolumeFade, begin, setActive, setActiveWithAudioFade
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FXModuleAnimateRCS.cs
- 174 lines; kind=PartModule; type=FXModuleAnimateRCS; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Animation=7, Transform=2, Lifecycle=1, LayerMask=1; unity_score=59
- ksp refs: KSPField=8, TimeWarp=2, HighLogic=2, ConfigNode=2, PartModule=1; fan_in=0 fan_out=6
- methods(4): FixedUpdate, OnLoad, OnSave, OnStart
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FXModuleAnimateThrottle.cs
- 192 lines; kind=PartModule; type=FXModuleAnimateThrottle; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Animation=9, Lifecycle=1, LayerMask=1; unity_score=56
- ksp refs: KSPField=16, TimeWarp=3, HighLogic=2, PartModule=1; fan_in=0 fan_out=5
- methods(3): FixedUpdate, OnInventoryModeDisable, OnStart
- auto suggestion: tier=B subsystem=PartModules-Engines

## Assembly-CSharp/FXModuleConstrainPosition.cs
- 137 lines; kind=PartModule; type=FXModuleConstrainPosition; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=8, Lifecycle=3; unity_score=55
- ksp refs: ConfigNode=4, KSPField=3, PartModule=1; fan_in=0 fan_out=3
- methods(8): FixedUpdate, LateUpdate, Load, OnLoad, Save, SetupObjects, Track, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FXModuleLookAtConstraint.cs
- 164 lines; kind=PartModule; type=FXModuleLookAtConstraint; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=5, Lifecycle=3; unity_score=45
- ksp refs: ConfigNode=4, KSPField=2, PartModule=1; fan_in=0 fan_out=3
- methods(11): FixedUpdate, LateUpdate, Load, OnConstructionModeFixedUpdate, OnConstructionModeUpdate, OnLoad, OnStart, Save, SetupObjects, Track, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FXModuleThrottleEffects.cs
- 72 lines; kind=PartModule; type=FXModuleThrottleEffects; bases=PartModuleFXSetter; ancestry=PartModuleFXSetter/PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, Animation=1; unity_score=46
- ksp refs: KSPField=8, HighLogic=2, TimeWarp=1; fan_in=0 fan_out=5
- methods(3): FixedUpdate, OnStart, Update
- auto suggestion: tier=A subsystem=PartModules-Engines

## Assembly-CSharp/FXMonger.cs
- 630 lines; kind=MonoBehaviour; type=FXMonger; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, KSP.FX.Fireworks, UnityEngine
- unity: GameObject=41, Transform=18, Audio=18, GetComponent=14, ParticleSystem=10, SetActive=5; unity_score=78
- ksp refs: Vector3d=16, GameSettings=11, FlightGlobals=11, Part=5; fan_in=7 fan_out=10
- methods(19): AddFireworkFX, Awake, Explode, ExplodeWithDebris, LateUpdate, OffsetPositions, OnDestroy, ROCExplode, RemoveFXOjbect, RemoveFireworkFX, SpawnDebrisExplosions, Splash, explode, explosionTest, offsetFireworks, offsetPositions, rocexplode, splash, splashMongered
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FXObject.cs
- 21 lines; kind=class; type=FXObject; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=2, GameObject=2, GetComponent=1, Audio=1; unity_score=67
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FXObjectPhoneHome.cs
- 11 lines; kind=MonoBehaviour; type=FXObjectPhoneHome; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1; unity_score=50
- ksp refs: none; fan_in=1 fan_out=2
- methods(1): OnDestroy
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/FXPrefab.cs
- 24 lines; kind=MonoBehaviour; type=FXPrefab; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=3, Lifecycle=3, MonoBehaviour=1, GetComponent=1; unity_score=87
- ksp refs: FloatingOrigin=2; fan_in=3 fan_out=1
- methods(3): Awake, OnDisable, OnEnable
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/GalaxyCameraControl.cs
- 30 lines; kind=MonoBehaviour; type=GalaxyCameraControl; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, Camera=2, MonoBehaviour=1; unity_score=77
- ksp refs: none; fan_in=0 fan_out=0
- methods(3): Awake, OnDestroy, SetFoV
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/GalaxyCubeControl.cs
- 150 lines; kind=MonoBehaviour; type=GalaxyCubeControl; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=7, Lifecycle=5, GameObject=4, Renderer=2, Raycast/Query=2, Material=2; unity_score=80
- ksp refs: Vector3d=3, FlightGlobals=3, HighLogic=2, ScaledSpace=1, QuaternionD=1, Planetarium=1; fan_in=1 fan_out=14
- methods(7): Awake, LateUpdate, LineOfSightToSun, OnDestroy, SetEnabled, Start, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/Game.cs
- 800 lines; kind=class; type=Game; bases=-; ancestry=-; ns=(global)
- usings: Expansions, Expansions.Missions, Expansions.Missions.Runtime, UnityEngine
- unity: Lifecycle=1, Instantiate/Destroy=1; unity_score=8
- ksp refs: HighLogic=22, ConfigNode=20, Game=8, GameEvents=7, GameDatabase=7, KerbalRoster=4; fan_in=103 fan_out=30
- methods(17): AddProtoScenarioModule, AddVessel, DestroyVessel, GetCloneOf, GetCometNumberedName, IsResumable, Load, LoadCometNames, MergeLoaderInfo, PopulateGameSeed, PopulateROCGameSeed, RemoveProtoScenarioModule, Save, SaveCometNames, Start, ToString, Updated
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/GameBackup.cs
- 29 lines; kind=class; type=GameBackup; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2, Game=1; fan_in=2 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/GameDatabase.cs **MUST-OPEN (>800 lines)**
- 1541 lines; kind=MonoBehaviour; type=GameDatabase; bases=LoadingSystem; ancestry=LoadingSystem/MonoBehaviour; ns=(global)
- usings: Experience, KSP.Localization, KSPAssets, KSPAssets.Loaders, UniLinq, UnityEngine
- unity: GameObject=47, Coroutine=20, Audio=17, Texture=15, Transform=9, Time.=9; unity_score=62
- ksp refs: ConfigNode=27, Localizer=7, PartLoader=4, GameDatabase=3, GameEvents=2; fan_in=111 fan_out=17
- methods(54): AudioClipNameContains, Awake, BitmapToUnityNormalMap, CleanupLoaders, CompileConfig, CompileConfigRecursive, CreateDatabase, ExistsAudioClip, ExistsConfigNode, ExistsModel, ExistsShader, ExistsTexture, GetAllTexturesInFolder, GetAllTexturesInFolderType, GetAudioClip, GetAudioFile, GetConfigNode, GetConfigNodes, GetConfigs, GetMergedConfigNodes, GetModel, GetModelFile, GetModelIn, GetModelPrefab, GetModelPrefabIn, GetShader, GetTexture, GetTextureFile, GetTextureIn, GetTextureInfo, GetTextureInfoIn, IsReady, LoadAssetBlacklist, LoadAssetBundleObjects, LoadEmpty, LoadLoaderInfo, LoadObjects, OnDestroy, ProgressFraction, ProgressTitle, RemoveAudioClip, RemoveModel, RemoveShader, RemoveTexture, ReplaceTexture ...
- auto suggestion: tier=C subsystem=Modding

## Assembly-CSharp/GameEvents.cs **MUST-OPEN (>800 lines)**
- 1058 lines; kind=class; type=GameEvents; bases=GameEventsBase; ancestry=GameEventsBase; ns=(global)
- usings: Contracts, Expansions.Missions, Expansions.Missions.Adjusters, Expansions.Missions.Editor, Expansions.Serenity, Expansions.Serenity.DeployedScience.Runtime, FinePrint, KSP.UI, KSP.UI.Screens, KSPAchievements, UnityEngine, Upgradeables
- unity: Transform=6, PhysicMaterial=4, Raycast/Query=2; unity_score=10
- ksp refs: Vessel=138, Part=132, ProtoCrewMember=56, CelestialBody=32, ConfigNode=30, KerbalEVA=22; fan_in=548 fan_out=107
- methods(1): FindEvent
- auto suggestion: tier=B subsystem=Core

## Assembly-CSharp/GameEventsBase.cs
- 4 lines; kind=class; type=GameEventsBase; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=12 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/GameObjectExtension.cs
- 192 lines; kind=static class; type=GameObjectExtension; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: GameObject=23, Transform=18, GetComponent=10, Renderer=2, Material=2, LayerMask=2; unity_score=72
- ksp refs: none; fan_in=0 fan_out=0
- methods(10): AddOrGetComponent, DestroyGameObject, DestroyGameObjectImmediate, GetChild, GetComponentCached, GetComponentIndex, GetComponentOnParent, GetComponentUpwards, SetLayerRecursive, SetTagsRecursive
- auto suggestion: tier=B subsystem=Core

## Assembly-CSharp/GameParameters.cs **MUST-OPEN (>800 lines)**
- 941 lines; kind=class; type=GameParameters; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=4
- ksp refs: ConfigNode=16, Game=10, Localizer=7, HighLogic=5, GameSettings=2; fan_in=35 fan_out=10
- methods(16): CustomParams, Enabled, GenerateParameterTypes, GetDefaultParameters, GetPresetColor, GetPresetColorHex, Interactible, KerbalExperienceEnabled, Load, LoadCustomNode, OnLoad, OnSave, Save, SetDifficultyPreset, SetDifficultyPresets, ValidValues
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/GamePersistence.cs
- 372 lines; kind=class; type=GamePersistence; bases=-; ancestry=-; ns=(global)
- usings: Expansions.Missions, KSP.Localization, UnityEngine
- unity: Application.=1; unity_score=9
- ksp refs: Game=23, HighLogic=7, GameSettings=6, ConfigNode=6, Localizer=5, FlightGlobals=3; fan_in=42 fan_out=21
- methods(7): CreateNewGame, LoadGame, LoadGameCfg, LoadSFSFile, SaveGame, UpdateScenarioModules, saveCompareToDate
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/GameplaySettingsScreen.cs
- 340 lines; kind=class; type=GameplaySettingsScreen; bases=DialogGUIVerticalLayout,ISettings; ancestry=DialogGUIVerticalLayout/DialogGUILayoutBase/DialogGUIBase; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens.Mapview, UnityEngine
- unity: Time.=1, Application.=1; unity_score=7
- ksp refs: GameSettings=78, Localizer=46, HighLogic=1, GameEvents=1; fan_in=1 fan_out=19
- methods(11): ApplyNavBallHPos, ApplySettings, ApplyUIElementScale, ApplyUIScalingAndAdjustments, DrawMiniSettings, DrawSettings, GetName, GetSettings, OnUpdate, ResetUIAdjustments, ResetUIScaling
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/GameScenes.cs
- 14 lines; kind=enum; type=GameScenes; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=194 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Scene

## Assembly-CSharp/GameSettings.cs **MUST-OPEN (>800 lines)**
- 2068 lines; kind=MonoBehaviour; type=GameSettings; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Editor, Highlighting, KSP.UI, KSP.UI.Screens, UnityEngine
- unity: KeyCode=129, QualitySettings/SystemInfo=13, Application.=10, Screen/Cursor=5, Time.=4, Lifecycle=3; unity_score=62
- ksp refs: ConfigNode=20, GameSettings=9, GameEvents=3, TimeWarp=2, Game=1; fan_in=256 fan_out=23
- methods(41): AddIncrementalSpeedMultiplierValue, ApplyEngineSettings, ApplySettings, Awake, DetectKeyboardLayout, GetAxiSpeedMultiplierIndex, GetAxisIncrementalSpeedMultiplierValues, GetAxisSpeedMultiplier, GetFireworkPresetColors, GetLightPresetColors, InitializeAxisGroupbindings, InitializeDefaultValues, LoadColor, LoadGameSettingsOnly, LoadLayoutKeyBindings, OnDestroy, OnGameSave, ParseCfg, ParseLauncherCfg, ParseLayoutRecursive, ParseLayoutsCfg, ReloadSettings, ResetSettings, SaveGameSettingsOnly, SaveSettings, SaveSettingsOnNextGameSave, SetCPVisualColors, SetCollapsedPawGroupNames, SetCollapsedPawGroupString, SetDefaultValues, SetFireworkPresetColors, SetHighlighterColor, SetHighlighterColors, SetLightPresetColors, SetRDSearchHighlightColors, SetUITextColors, Start, WaitAndSetResolution, WriteCfg, WriteLauncherCFG, WriteSettings
- auto suggestion: tier=B subsystem=Input

## Assembly-CSharp/GameVariables.cs
- 681 lines; kind=MonoBehaviour; type=GameVariables; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: CommNet, Contracts, KSP.Localization, UnityEngine
- unity: Lifecycle=2, AnimationCurve=2, MonoBehaviour=1, GameObject=1; unity_score=25
- ksp refs: Contract=24, Vessel=16, HighLogic=14, Localizer=9, ProtoCrewMember=3, Planetarium=2; fan_in=52 fan_out=17
- methods(52): Awake, EVAIsPossible, GetActiveContractsLimit, GetActiveCrewLimit, GetActiveStrategyLimit, GetContractDestinationWeight, GetContractFundsAdvanceFactor, GetContractFundsCompletionFactor, GetContractFundsFailureFactor, GetContractLevelLimit, GetContractPrestigeFactor, GetContractReputationCompletionFactor, GetContractReputationFailureFactor, GetContractScienceCompletionFactor, GetCraftMassLimit, GetCraftSizeLimit, GetCrewLevelLimit, GetDSNPowerCurve, GetDSNRange, GetDSNRangeCurve, GetDSNScienceCurve, GetDataToScienceRatio, GetEVALockedReason, GetExperimentLevel, GetMentalityDeadlineFactor, GetMentalityExpiryFactor, GetMentalityFundsFactor, GetMentalityReputationFactor, GetMentalityScienceFactor, GetOrbitDisplayMode, GetPartCountLimit, GetPatchesAheadLimit, GetRecoveredPartValue, GetRecoveredResourceValue, GetRecruitHireCost, GetScienceCostLimit, GetStrategyCommitRange, GetStrategyLevelLimit, GetTrackedObjectLimit, ManeuverToolAvailable, MinTrackedObjectSize, OnDestroy, ScoreFlightEnvelope, ScoreSituation, UnlockedActionGroupsCustom ...
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/GAPCelestialBodyCamera.cs
- 286 lines; kind=MonoBehaviour; type=GAPCelestialBodyCamera; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Editor, UnityEngine
- unity: Transform=18, AnimationCurve=6, Time.=5, Camera=3, Instantiate/Destroy=2, GameObject=2; unity_score=67
- ksp refs: Vector3d=17, CelestialBody=5, QuaternionD=1, FlightGlobals=1; fan_in=1 fan_out=6
- methods(13): Destroy, Disable, Enable, GetLightRot, GetLocalSpaceCamera, OverrideDistance, OverridePosition, RefreshTarget, SetBoundries, SetLightCycle, SetPosition, UpdateMouse, UpdateTransform
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/GAPCelestialBodyCollisionSphere.cs
- 23 lines; kind=MonoBehaviour; type=GAPCelestialBodyCollisionSphere; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=3, GetComponent=3, Mesh=2, Collider=2, Renderer=1, MonoBehaviour=1; unity_score=96
- ksp refs: Vector3d=1, CelestialBody=1; fan_in=1 fan_out=2
- methods(2): Setup, ToggleVisibility
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/GAPKerbalAnimation.cs
- 78 lines; kind=MonoBehaviour; type=GAPKerbalAnimation; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Animation=7, Coroutine=5, Lifecycle=2, MonoBehaviour=1; unity_score=81
- ksp refs: none; fan_in=0 fan_out=0
- methods(3): OnDestroy, PlayAnimations, Start
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/GAPProceduralAsteroid.cs
- 39 lines; kind=MonoBehaviour; type=GAPProceduralAsteroid; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, Resources.=1, MonoBehaviour=1, Lifecycle=1; unity_score=69
- ksp refs: none; fan_in=2 fan_out=2
- methods(3): RangefinderGeneric, Setup, Start
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/GAPProceduralComet.cs
- 36 lines; kind=MonoBehaviour; type=GAPProceduralComet; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, Resources.=1, MonoBehaviour=1, Lifecycle=1; unity_score=70
- ksp refs: none; fan_in=2 fan_out=2
- methods(3): RangefinderGeneric, Setup, Start
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/GAPUtil_BiSelector.cs
- 152 lines; kind=MonoBehaviour; type=GAPUtil_BiSelector; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Missions, Expansions.Missions.Editor, KSP.UI.Screens, KSP.UI.TooltipTypes, RUI.Icons.Selectable, TMPro, UnityEngine, UnityEngine.UI
- unity: Texture=18, UnityEngine.UI=9, GetComponent=6, GameObject=5, Instantiate/Destroy=3, Transform=2; unity_score=92
- ksp refs: none; fan_in=3 fan_out=7
- methods(9): AddDivider, AddSidebarButton, AddSidebarGAPButton, Awake, ClearSidebar, GetSidebarButton, GetSidebarGapButton, SetFooterText, SetTitleText
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/GAPUtil_DynamicCylinder.cs
- 146 lines; kind=MonoBehaviour; type=GAPUtil_DynamicCylinder; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=5, Mesh=4, Renderer=2, Material=2, GetComponent=2, GameObject=2; unity_score=72
- ksp refs: none; fan_in=1 fan_out=0
- methods(3): CreateMesh, GetPivotPosition, UpdateValues
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/GAPVesselCamera.cs
- 405 lines; kind=MonoBehaviour; type=GAPVesselCamera; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Editor, UnityEngine
- unity: Input.=19, Time.=16, Transform=11, KeyCode=10, GetComponent=3, Camera=2; unity_score=79
- ksp refs: GameSettings=29, ShipConstruct=2; fan_in=2 fan_out=7
- methods(14): FocusPoint, FocusVessel, PlaceCamera, ResetCamera, Rotate, Setup, Start, Update, UpdateCameraControls, UpdateSPHCamera, UpdateSPHCameraControls, UpdateVABCamera, UpdateVABCameraControls, Zoom
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/GasGiantMaterialControls.cs
- 251 lines; kind=MonoBehaviour; type=GasGiantMaterialControls; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Material=29, Shader=19, Texture=9, Lifecycle=3, Renderer=2, MonoBehaviour=1; unity_score=90
- ksp refs: Planetarium=1; fan_in=0 fan_out=1
- methods(5): Awake, OnEnable, Update, UpdateMaterialControlSettings, UpdateTimeDivisors
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/Gauge.cs
- 141 lines; kind=MonoBehaviour; type=Gauge; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GetComponent=5, Audio=5, Time.=3, Lifecycle=2, Transform=1, MonoBehaviour=1; unity_score=68
- ksp refs: HighLogic=1; fan_in=1 fan_out=2
- methods(4): Start, Update, setValue, startSound
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/GaugeLEDRange.cs
- 25 lines; kind=class; type=GaugeLEDRange; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Audio=1; unity_score=35
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/GizmoDrawer.cs
- 5 lines; kind=MonoBehaviour; type=GizmoDrawer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=36
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/GizmoDrawUtil.cs
- 59 lines; kind=class; type=GizmoDrawUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=62
- ksp refs: none; fan_in=2 fan_out=0
- methods(3): DrawArc, DrawCrosshairs, DrawReach
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/GroundCrew.cs
- 87 lines; kind=MonoBehaviour; type=GroundCrew; bases=SpaceCenterCrew; ancestry=SpaceCenterCrew/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2; unity_score=34
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): SetAnimation, Start, Update
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/GUILayoutx.cs
- 86 lines; kind=class; type=GUILayoutx; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: IMGUI=9; unity_score=56
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): DoubleClickCallback, SelectionList
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/GUIUtil.cs
- 293 lines; kind=class; type=GUIUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: IMGUI=72, UnityEngine.UI=7, Screen/Cursor=2; unity_score=77
- ksp refs: none; fan_in=1 fan_out=0
- methods(17): ClearEditableFieldFlags, DrawEnumSelectionPane, EditableBoolField, EditableDoubleField, EditableEnumField, EditableField, EditableFloatField, EditableIntField, EditableStringField, EditableTextArea, EnumFlagField, LabelHorizontalSlider, LabelVerticalSlider, ProgressBar, ProgressBarVert, ProgressBarVertical, ScreenCenteredRect
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/HarvestTypes.cs
- 13 lines; kind=enum; type=HarvestTypes; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=9 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/HelixGauge.cs
- 94 lines; kind=MonoBehaviour; type=HelixGauge; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: SetActive=16, GameObject=16, Transform=8, Lifecycle=2, MonoBehaviour=1; unity_score=94
- ksp refs: none; fan_in=0 fan_out=0
- methods(3): LateUpdate, Start, UpdateGauge
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/HelmetSuitPickerWindow.cs
- 709 lines; kind=MonoBehaviour; type=HelmetSuitPickerWindow; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, KSP.Localization, KSP.UI, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=22, Transform=16, Instantiate/Destroy=9, Shader=8, Material=6, Texture=5; unity_score=72
- ksp refs: ProtoCrewMember=18, GameEvents=15, Localizer=7, HighLogic=5, GameSettings=5, GameDatabase=2; fan_in=1 fan_out=20
- methods(29): CheckStock, CleanWindow, CloseWindow, CreateThumbnailCamera, GetAvailableSuits, InitFromProtoCrewData, InitSelection, LocalizedSuitText, OnACDespawn, OnACSpawn, OnCloseClick, OnComboSelected, OnDestroy, OnHelmetNeckringSelection, OnSceneSwitch, OpenSuitLightPicker, SetDefaultCombo, SetExtraCombo, SetupComboButton, SetupComboSelector, SetupExtraCombos, SetupGlowColor, SetupHelmetNeckringButton, SetupPreview, SetupStockCombos, SetupSuitTypeButtons, SetupWindowTransform, Start, UpdateDisplayName
- auto suggestion: tier=C subsystem=Kerbals

## Assembly-CSharp/HexColorField.cs
- 88 lines; kind=MonoBehaviour; type=HexColorField; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine
- unity: TMPro=3, Lifecycle=2, MonoBehaviour=1, GetComponent=1; unity_score=63
- ksp refs: none; fan_in=0 fan_out=2
- methods(6): ColorToHex, HexToColor, OnDestroy, Start, UpdateColor, UpdateHex
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/HierarchyUtil.cs
- 140 lines; kind=static class; type=HierarchyUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Transform=12; unity_score=42
- ksp refs: Part=10; fan_in=5 fan_out=1
- methods(7): CompareHierarchyDepths, CompileID, GetHierarchyDepth, compileID, isAncestor, isDescendant, isSameLevel
- auto suggestion: tier=B subsystem=Core

## Assembly-CSharp/HighLogic.cs
- 355 lines; kind=MonoBehaviour; type=HighLogic; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Runtime, FinePrint.Utilities, KSP.UI.Util, UnityEngine, UnityEngine.SceneManagement
- unity: SceneManager=14, Input.=14, KeyCode=12, Coroutine=10, Lifecycle=4, Time.=3; unity_score=82
- ksp refs: GameEvents=7, MissionSystem=4, Planetarium=2, HighLogic=2, GameSettings=2, Game=2; fan_in=466 fan_out=14
- methods(16): Awake, FireLoadedEvent, FireLoadedEventGUIReady, GetLoadedGameSceneFromBuildIndex, LoadScene, LoadSceneFromBundle, OnApplicationFocus, OnDestroy, OnLevelLoaded, OnSceneLoaded, SetLoadSceneEventsAndFlags, Start, Update, bufferedLoad, printCurrentGame, setLevelFlags
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/HSVChangedEvent.cs
- 5 lines; kind=class; type=HSVChangedEvent; bases=UnityEvent<float,float,float>; ancestry=UnityEvent; ns=(global)
- usings: UnityEngine.Events
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/HsvColor.cs
- 56 lines; kind=struct; type=HsvColor; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(1): ToString
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/HSVUtil.cs
- 104 lines; kind=static class; type=HSVUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=3
- ksp refs: none; fan_in=4 fan_out=1
- methods(2): ConvertHsvToRgb, ConvertRgbToHsv
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/IActivateOnDecouple.cs
- 4 lines; kind=interface; type=IActivateOnDecouple; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/IActiveJointHost.cs
- 12 lines; kind=interface; type=IActiveJointHost; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Transform=1; unity_score=21
- ksp refs: Part=1; fan_in=4 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/IAirstreamShield.cs
- 8 lines; kind=interface; type=IAirstreamShield; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Vessel=1, Part=1; fan_in=3 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Aero

## Assembly-CSharp/IAnalyticOverheatModule.cs
- 6 lines; kind=interface; type=IAnalyticOverheatModule; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/IAnalyticPreview.cs
- 6 lines; kind=interface; type=IAnalyticPreview; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: FlightIntegrator=1; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/IAnalyticTemperatureModifier.cs
- 8 lines; kind=interface; type=IAnalyticTemperatureModifier; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: FlightIntegrator=1; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/IAnimatedModule.cs
- 10 lines; kind=interface; type=IAnimatedModule; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=7 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/IAxisFieldLimits.cs
- 23 lines; kind=interface; type=IAxisFieldLimits; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=12
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Misc

## Assembly-CSharp/ICMTweakTarget.cs
- 14 lines; kind=interface; type=ICMTweakTarget; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Transform=1, Collider=1; unity_score=43
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Misc

## Assembly-CSharp/ICollisionEvents.cs
- 12 lines; kind=interface; type=ICollisionEvents; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Collision/Force=6, MonoBehaviour=1; unity_score=73
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Physics

## Assembly-CSharp/ICommAntenna.cs
- 22 lines; kind=interface; type=ICommAntenna; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ProtoPartModuleSnapshot=2; fan_in=4 fan_out=3
- methods(0): 
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/IConfigNode.cs
- 6 lines; kind=interface; type=IConfigNode; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=88 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/IConstruction.cs
- 8 lines; kind=interface; type=IConstruction; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=9 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/IContractObjectiveModule.cs
- 6 lines; kind=interface; type=IContractObjectiveModule; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=7 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Misc

## Assembly-CSharp/IDateFormatterExtensions.cs
- 22 lines; kind=static class; type=IDateFormatterExtensions; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): GetDateComponents, GetTimeComponents
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/IDateTimeFormatter.cs
- 34 lines; kind=interface; type=IDateTimeFormatter; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/IDiscoverable.cs
- 18 lines; kind=interface; type=IDiscoverable; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=13 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/IDynamicCargoOccluder.cs
- 4 lines; kind=interface; type=IDynamicCargoOccluder; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/IEngineStatus.cs
- 10 lines; kind=interface; type=IEngineStatus; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=6 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Engines

## Assembly-CSharp/IGameCamera.cs
- 16 lines; kind=MonoBehaviour; type=IGameCamera; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=36
- ksp refs: none; fan_in=3 fan_out=0
- methods(3): DisableCamera, EnableCamera, ResetCamera
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/IJointLockState.cs
- 4 lines; kind=interface; type=IJointLockState; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=6 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/IKSPCamera.cs
- 25 lines; kind=interface; type=IKSPCamera; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Transform=2; unity_score=38
- ksp refs: none; fan_in=6 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/ILiftProvider.cs
- 8 lines; kind=interface; type=ILiftProvider; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Aero

## Assembly-CSharp/IModular.cs
- 10 lines; kind=interface; type=IModular; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/IModuleInfo.cs
- 12 lines; kind=interface; type=IModuleInfo; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=3
- ksp refs: none; fan_in=14 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/IMouseEvents.cs
- 16 lines; kind=interface; type=IMouseEvents; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=21
- ksp refs: none; fan_in=6 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/IMultipleDragCube.cs
- 10 lines; kind=interface; type=IMultipleDragCube; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=13 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Aero

## Assembly-CSharp/InputBinding.cs
- 66 lines; kind=class; type=InputBinding; bases=IConfigNode,ICloneable; ancestry=IConfigNode; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2, InputLockManager=1, HighLogic=1; fan_in=5 fan_out=7
- methods(7): Clone, CompareSwitchState, IsLocked, IsNeutral, IsUnlocked, Load, Save
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/InputBindingModes.cs
- 13 lines; kind=enum; type=InputBindingModes; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=7 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/InputDevices.cs
- 77 lines; kind=class; type=InputDevices; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: UnityEngine
- unity: Input.=1, Application.=1; unity_score=24
- ksp refs: ConfigNode=3; fan_in=3 fan_out=3
- methods(6): GetDeviceIndex, GetDeviceListLength, InitializeDevices, Load, Save, TrimDeviceName
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/InputFieldHandler.cs
- 84 lines; kind=class; type=InputFieldHandler; bases=-; ancestry=-; ns=(global)
- usings: TMPro
- unity: TMPro=2, Lifecycle=1; unity_score=28
- ksp refs: InputLockManager=2; fan_in=2 fan_out=3
- methods(9): GetFormattedValue, GetValueDelegate, OnDestroy, OnFieldDeselected, OnFieldSelected, OnValueChanged, ParseText, SetValueDelegate, UpdateField
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/InputLockManager.cs
- 156 lines; kind=static class; type=InputLockManager; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: IMGUI=2; unity_score=15
- ksp refs: GameEvents=11, Localizer=2, InputLockManager=1; fan_in=165 fan_out=3
- methods(14): ClearControlLocks, ControlLocks, DebugLockStack, GetControlLock, IsAllLocked, IsAnyUnlocked, IsLocked, IsLocking, IsUnlocked, IsUnlocking, PrintLockStack, RecalcMask, RemoveControlLock, SetControlLock
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/InputSettings.cs **MUST-OPEN (>800 lines)**
- 1440 lines; kind=class; type=InputSettings; bases=DialogGUIVerticalLayout,ISettings; ancestry=DialogGUIVerticalLayout/DialogGUILayoutBase/DialogGUIBase; ns=(global)
- usings: KSP.Localization, UnityEngine, UnityEngine.UI
- unity: KeyCode=12, UnityEngine.UI=7, SetActive=7, Input.=3, GetComponent=3, Lifecycle=1; unity_score=21
- ksp refs: GameSettings=337, ConfigNode=8, Localizer=4, MapView=2, FlightGlobals=2; fan_in=2 fan_out=33
- methods(23): ApplyAxisBinding, ApplyKeyBinding, ApplySettings, DrawAxisBinding, DrawKeyBinding, DrawMiniSettings, DrawSettings, GetName, GetSettings, GetValidKeys, ListenToAxis, ListenToKey, OnDismissListenModeDialog, OnUpdate, ResetAxisValues, SetAxis, SetKey, SpawnListenModeDialog, Update, drawInputPage, drawListenModeWindow, drawOtherPage, isEnabled
- auto suggestion: tier=C subsystem=Input

## Assembly-CSharp/InspectorKeyValuePair.cs
- 25 lines; kind=class; type=InspectorKeyValuePair; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=12
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): ToKeyValuePair
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/IntermediateTutorial.cs
- 139 lines; kind=ScenarioModule; type=IntermediateTutorial; bases=METutorialScenario; ancestry=METutorialScenario/TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions, Expansions.Missions.Editor
- unity: none; unity_score=15
- ksp refs: GameEvents=2, ConfigNode=2; fan_in=11 fan_out=18
- methods(13): CreateTutorialPages, DeleteTutorial, GetNextTutorialName, OnContinueButtonClick, OnDoneButtonClick, OnExitCancelSave, OnExitMissionEditorScreen, OnLoad, OnNodeChange, OnSave, OnTutorialSetup, SetupEditorForTutorial, SetupEditorToDefault
- auto suggestion: tier=A subsystem=Tutorials

## Assembly-CSharp/IntermediateTutorialPageStep.cs
- 19 lines; kind=class; type=IntermediateTutorialPageStep; bases=TutorialPageConfig,IDisposable; ancestry=TutorialPageConfig; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=10 fan_out=3
- methods(1): Dispose
- auto suggestion: tier=A subsystem=Tutorials

## Assembly-CSharp/InternalAltimeterThreeHands.cs
- 78 lines; kind=MonoBehaviour; type=InternalAltimeterThreeHands; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=9, Time.=1; unity_score=67
- ksp refs: KSPField=5; fan_in=0 fan_out=2
- methods(2): OnAwake, OnUpdate
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalAnimationAirSpeed.cs
- 54 lines; kind=MonoBehaviour; type=InternalAnimationAirSpeed; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Animation=2; unity_score=47
- ksp refs: KSPField=4; fan_in=0 fan_out=2
- methods(2): OnAwake, OnUpdate
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalAnimationAltimeter.cs
- 44 lines; kind=MonoBehaviour; type=InternalAnimationAltimeter; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Animation=2; unity_score=51
- ksp refs: KSPField=3; fan_in=0 fan_out=2
- methods(2): OnAwake, OnUpdate
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalAtmosphereDepth.cs
- 83 lines; kind=MonoBehaviour; type=InternalAtmosphereDepth; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=3, Time.=1; unity_score=46
- ksp refs: KSPField=7, FlightGlobals=3, CelestialBody=1; fan_in=0 fan_out=4
- methods(2): OnAwake, OnUpdate
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalAxisIndicatorPitch.cs
- 44 lines; kind=MonoBehaviour; type=InternalAxisIndicatorPitch; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=3, Time.=1; unity_score=60
- ksp refs: KSPField=4; fan_in=0 fan_out=3
- methods(2): OnAwake, OnUpdate
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalAxisIndicatorRoll.cs
- 44 lines; kind=MonoBehaviour; type=InternalAxisIndicatorRoll; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=3, Time.=1; unity_score=60
- ksp refs: KSPField=4; fan_in=0 fan_out=3
- methods(2): OnAwake, OnUpdate
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalAxisIndicatorYaw.cs
- 44 lines; kind=MonoBehaviour; type=InternalAxisIndicatorYaw; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=3, Time.=1; unity_score=60
- ksp refs: KSPField=4; fan_in=0 fan_out=3
- methods(2): OnAwake, OnUpdate
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalButton.cs
- 169 lines; kind=MonoBehaviour; type=InternalButton; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Time.=4, Input.=4, GameObject=3, Coroutine=3, GetComponent=2, MonoBehaviour=1; unity_score=60
- ksp refs: none; fan_in=4 fan_out=0
- methods(17): Create, InternalButtonDelegate, OnDoubleTap, OnDown, OnDrag, OnExit, OnMouseDoubleTap, OnMouseDown, OnMouseDrag, OnMouseExit, OnMouseOver, OnMouseTap, OnMouseUp, OnOver, OnTap, OnUp, TapRoutine
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalButtonLight.cs
- 107 lines; kind=MonoBehaviour; type=InternalButtonLight; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Light=6, GetComponent=3, Shader=2, Renderer=2, Material=2, Collider=2; unity_score=81
- ksp refs: KSPField=9; fan_in=0 fan_out=4
- methods(3): Button_OnTap, OnAwake, SetLight
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalCamera.cs
- 272 lines; kind=MonoBehaviour; type=InternalCamera; bases=IGameCamera; ancestry=IGameCamera/MonoBehaviour; ns=(global)
- usings: CameraFXModules, UnityEngine, UnityEngine.EventSystems
- unity: Transform=17, Time.=10, Input.=10, Lifecycle=4, KeyCode=4, Coroutine=2; unity_score=77
- ksp refs: GameSettings=23, FlightCamera=4; fan_in=2 fan_out=12
- methods(12): Awake, DisableCamera, EnableCamera, ManualReset, MiddleDoubleClick, OnDestroy, ResetCamera, SetFOV, SetTransform, Start, Update, UpdateState
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/InternalCameraSwitch.cs
- 59 lines; kind=MonoBehaviour; type=InternalCameraSwitch; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=4, GameObject=1; unity_score=60
- ksp refs: KSPField=2; fan_in=0 fan_out=5
- methods(2): Button_OnDoubleTap, OnAwake
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalCompass.cs
- 34 lines; kind=MonoBehaviour; type=InternalCompass; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=3, Time.=1; unity_score=64
- ksp refs: KSPField=3, FlightGlobals=1; fan_in=0 fan_out=3
- methods(2): OnAwake, OnUpdate
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalComponents.cs
- 105 lines; kind=MonoBehaviour; type=InternalComponents; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine
- unity: Transform=6, Lifecycle=2, SetActive=1, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1; unity_score=65
- ksp refs: none; fan_in=2 fan_out=2
- methods(4): Awake, CreateText, OnDestroy, getTMPAlignment
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalDialIncrement.cs
- 61 lines; kind=class; type=InternalDialIncrement; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=12
- ksp refs: ConfigNode=4; fan_in=2 fan_out=2
- methods(3): CalculateAngle, Load, Save
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/InternalFlagDecal.cs
- 66 lines; kind=MonoBehaviour; type=InternalFlagDecal; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=5, GameObject=3, Renderer=2, Coroutine=2, Texture=1, Material=1; unity_score=85
- ksp refs: KSPField=1, GameDatabase=1; fan_in=0 fan_out=3
- methods(4): OnAwake, OnDestroy, UpdateFlagTexture, WaitForPart
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/internalGeneric.cs
- 10 lines; kind=MonoBehaviour; type=internalGeneric; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: -
- unity: Lifecycle=2; unity_score=50
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): Start, Update
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/InternalIndicatorPanel.cs
- 167 lines; kind=MonoBehaviour; type=InternalIndicatorPanel; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: KSP.UI.Screens, UnityEngine
- unity: Shader=4, Material=4, Renderer=2; unity_score=59
- ksp refs: ConfigNode=6, KSPField=1, FlightGlobals=1; fan_in=0 fan_out=8
- methods(8): Create, Load, OnAwake, OnUpdate, Save, Set, SetOff, SetOn
- auto suggestion: tier=C subsystem=Kerbals

## Assembly-CSharp/InternalLabel.cs
- 70 lines; kind=MonoBehaviour; type=InternalLabel; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=1; unity_score=32
- ksp refs: KSPField=7, ConfigNode=1; fan_in=0 fan_out=5
- methods(3): OnAwake, OnLoad, SetText
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalLeverThrottle.cs
- 47 lines; kind=MonoBehaviour; type=InternalLeverThrottle; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, Input.=2, Collider=2, Time.=1, GetComponent=1, GameObject=1; unity_score=81
- ksp refs: KSPField=5; fan_in=0 fan_out=4
- methods(3): Lever_OnDrag, OnAwake, OnUpdate
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalModel.cs
- 507 lines; kind=MonoBehaviour; type=InternalModel; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=47, GameObject=13, Animation=7, GetComponent=6, Renderer=4, Light=2; unity_score=74
- ksp refs: ConfigNode=9, ProtoCrewMember=5, Part=2, Vessel=1, PartLoader=1; fan_in=8 fan_out=10
- methods(23): AddProp, AddPropModule, AssignToSeat, DespawnCrew, FindHeirarchyTransform, FindModelAnimators, FindModelComponent, FindModelComponents, FindModelTransform, GetAvailableSeatCount, GetAvailableSeats, GetNextAvailableSeat, GetNextAvailableSeatIndex, Initialize, InternalToWorld, Load, OnFixedUpdate, OnUpdate, SetVisible, SitKerbalAt, SpawnCrew, UnseatKerbal, UnseatKerbalAt
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalModule.cs
- 92 lines; kind=MonoBehaviour; type=InternalModule; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1; unity_score=41
- ksp refs: ConfigNode=4, Vessel=1, Part=1; fan_in=22 fan_out=7
- methods(9): Awake, Load, ModularSetup, OnAwake, OnFixedUpdate, OnLoad, OnSave, OnUpdate, Save
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalNavBall.cs
- 330 lines; kind=MonoBehaviour; type=InternalNavBall; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: FinePrint, UnityEngine
- unity: Transform=69, GameObject=22, SetActive=20, Renderer=2, Material=2, GetComponent=2; unity_score=91
- ksp refs: FlightGlobals=25, KSPField=18, Vessel=2, GameEvents=2, Orbit=1; fan_in=0 fan_out=11
- methods(4): OnAwake, OnDestroy, OnUpdate, OnVesselSwitch
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalPart.cs
- 86 lines; kind=MonoBehaviour; type=InternalPart; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=45
- ksp refs: ConfigNode=4, PartLoader=2, Part=1; fan_in=0 fan_out=4
- methods(9): AddProp, Load, OnActive, OnAwake, OnFixedUpdate, OnInactive, OnStart, OnUpdate, Save
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalProp.cs
- 300 lines; kind=MonoBehaviour; type=InternalProp; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=32, GameObject=7, Animation=7, GetComponent=3, MonoBehaviour=1; unity_score=75
- ksp refs: ConfigNode=4, Vessel=1, Part=1; fan_in=4 fan_out=7
- methods(13): AddModule, FindHeirarchyTransform, FindHeirarchyTransforms, FindModelAnimators, FindModelComponent, FindModelComponents, FindModelTransform, FindModelTransforms, Load, OnAwake, OnFixedUpdate, OnUpdate, Save
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalRadarAltitude.cs
- 47 lines; kind=MonoBehaviour; type=InternalRadarAltitude; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=3, Time.=1; unity_score=56
- ksp refs: KSPField=4; fan_in=0 fan_out=3
- methods(2): OnAwake, OnUpdate
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalSeat.cs
- 122 lines; kind=MonoBehaviour; type=InternalSeat; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Transform=11, Camera=2, Instantiate/Destroy=1, GameObject=1; unity_score=70
- ksp refs: KSPField=9, ProtoCrewMember=2, Localizer=2; fan_in=2 fan_out=5
- methods(4): DespawnCrew, OnAwake, SpawnCrew, UpdatePortraitOffset
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalSpace.cs
- 39 lines; kind=MonoBehaviour; type=InternalSpace; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=5, Lifecycle=2, MonoBehaviour=1; unity_score=81
- ksp refs: FlightGlobals=3; fan_in=4 fan_out=1
- methods(4): Awake, InternalToWorld, OnDestroy, WorldToInternal
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalSpaceOverlay.cs
- 97 lines; kind=MonoBehaviour; type=InternalSpaceOverlay; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=5, GameObject=4, Camera=4, Lifecycle=2, GetComponent=2, MonoBehaviour=1; unity_score=82
- ksp refs: GameEvents=7, Vessel=3, Part=2, ProtoCrewMember=1, FlightCamera=1; fan_in=1 fan_out=7
- methods(8): Create, Dismiss, LateUpdate, OnCameraChange, OnDestroy, OnVesselModified, Setup, onCrewTransferred
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalSpeed.cs
- 147 lines; kind=MonoBehaviour; type=InternalSpeed; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Transform=2, GameObject=1; unity_score=32
- ksp refs: KSPField=9, FlightGlobals=8, Localizer=7, Orbit=1, ConfigNode=1; fan_in=1 fan_out=9
- methods(5): CacheLocalStrings, OnAwake, OnButtonTap, OnLoad, OnUpdate
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalText.cs
- 17 lines; kind=MonoBehaviour; type=InternalText; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine
- unity: TMPro=2, MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=77
- ksp refs: none; fan_in=3 fan_out=1
- methods(1): Awake
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/InternalVSI.cs
- 40 lines; kind=MonoBehaviour; type=InternalVSI; bases=InternalModule; ancestry=InternalModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=3, Time.=1; unity_score=60
- ksp refs: KSPField=4, FlightGlobals=1; fan_in=0 fan_out=4
- methods(2): OnAwake, OnUpdate
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/IntersectInformation.cs
- 21 lines; kind=struct; type=IntersectInformation; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/IntTypeParser.cs
- 31 lines; kind=class; type=IntTypeParser; bases=ITypeParser; ancestry=ITypeParser; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(2): Convert, Parse
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/InventoryPanelController.cs
- 307 lines; kind=MonoBehaviour; type=InventoryPanelController; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI.Screens, UnityEngine, UnityEngine.EventSystems
- unity: Transform=6, Lifecycle=4, Instantiate/Destroy=4, EventSystems=4, GameObject=3, MonoBehaviour=1; unity_score=56
- ksp refs: GameEvents=6, EditorLogic=4, AvailablePart=3, ProtoCrewMember=2, ShipConstruct=1, PartLoader=1; fan_in=3 fan_out=14
- methods(14): Awake, DestroyHeldIcons, OnDestroy, OnEditorCrewModified, OnEditorScreenChange, OnEditorShipModified, OnPointerEnter, OnPointerExit, Reset, Start, TryGetDisplayedInventory, Update, UpdateDisplayedInventories, UpdateInventoryLists
- auto suggestion: tier=C subsystem=Kerbals

## Assembly-CSharp/IOverheatDisplay.cs
- 14 lines; kind=interface; type=IOverheatDisplay; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/IPartCostModifier.cs
- 6 lines; kind=interface; type=IPartCostModifier; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=7 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/IPartMassModifier.cs
- 6 lines; kind=interface; type=IPartMassModifier; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=12 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/IPartSizeModifier.cs
- 8 lines; kind=interface; type=IPartSizeModifier; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: none; fan_in=1 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/IPersistenceLoad.cs
- 4 lines; kind=interface; type=IPersistenceLoad; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/IPersistenceSave.cs
- 4 lines; kind=interface; type=IPersistenceSave; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/IPid.cs
- 4 lines; kind=interface; type=IPid; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/IResourceBroker.cs
- 18 lines; kind=interface; type=IResourceBroker; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=8; fan_in=3 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/IResourceConsumer.cs
- 6 lines; kind=interface; type=IResourceConsumer; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: PartResourceDefinition=1; fan_in=19 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/IScalarModule.cs
- 20 lines; kind=interface; type=IScalarModule; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=17 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/IScienceDataContainer.cs
- 16 lines; kind=interface; type=IScienceDataContainer; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=6 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/IScienceDataTransmitter.cs
- 14 lines; kind=interface; type=IScienceDataTransmitter; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=8 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/IScreenCaster.cs
- 6 lines; kind=interface; type=IScreenCaster; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=3
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Misc

## Assembly-CSharp/ISettings.cs
- 14 lines; kind=interface; type=ISettings; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/IShipconstruct.cs
- 6 lines; kind=interface; type=IShipconstruct; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=1; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/IShipConstructIDChanges.cs
- 6 lines; kind=interface; type=IShipConstructIDChanges; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/ISiteNode.cs
- 11 lines; kind=interface; type=ISiteNode; bases=-; ancestry=-; ns=(global)
- usings: KSP.UI.Screens.Mapview, UnityEngine
- unity: Transform=1; unity_score=21
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Scene

## Assembly-CSharp/IStageSeparator.cs
- 4 lines; kind=interface; type=IStageSeparator; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=8 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/IStageSeparatorChild.cs
- 8 lines; kind=interface; type=IStageSeparatorChild; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=1; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/ISteerable.cs
- 4 lines; kind=interface; type=ISteerable; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Control

## Assembly-CSharp/iT.cs **MUST-OPEN (>800 lines)**
- 1281 lines; kind=static class; type=iT; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ITargetable.cs
- 26 lines; kind=interface; type=ITargetable; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Transform=1; unity_score=27
- ksp refs: Vessel=1, OrbitDriver=1, Orbit=1; fan_in=11 fan_out=4
- methods(0): 
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/ITechPerk.cs
- 6 lines; kind=interface; type=ITechPerk; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/IThrottleControl.cs
- 6 lines; kind=interface; type=IThrottleControl; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Engines

## Assembly-CSharp/IThrustProvider.cs
- 10 lines; kind=interface; type=IThrustProvider; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Engines

## Assembly-CSharp/IThumbnailSetup.cs
- 8 lines; kind=interface; type=IThumbnailSetup; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: GameObject=1; unity_score=21
- ksp refs: ProtoPartSnapshot=2; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Misc

## Assembly-CSharp/IToggleCrossfeed.cs
- 12 lines; kind=interface; type=IToggleCrossfeed; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/ITorqueProvider.cs
- 6 lines; kind=interface; type=ITorqueProvider; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: none; fan_in=5 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Control

## Assembly-CSharp/iTween.cs **MUST-OPEN (>800 lines)**
- 4414 lines; kind=MonoBehaviour; type=iTween; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: Transform=275, GameObject=117, GetComponent=96, Audio=19, Renderer=17, Material=17; unity_score=72
- ksp refs: none; fan_in=0 fan_out=0
- methods(165): ApplyAudioToTargets, ApplyColorTargets, ApplyColorToTargets, ApplyFloatTargets, ApplyLookToTargets, ApplyMoveByTargets, ApplyMoveToPathTargets, ApplyMoveToTargets, ApplyPunchPositionTargets, ApplyPunchRotationTargets, ApplyPunchScaleTargets, ApplyRectTargets, ApplyRotateAddTargets, ApplyRotateToTargets, ApplyScaleToTargets, ApplyShakePositionTargets, ApplyShakeRotationTargets, ApplyShakeScaleTargets, ApplyStabTargets, ApplyTween, ApplyVector2Targets, ApplyVector3Targets, AudioFrom, AudioTo, AudioUpdate, Awake, CallBack, CameraFadeDepth, CameraFadeDestroy, CameraFadeFrom, CameraFadeTo, CameraTexture, CleanArgs, ColorFrom, ColorTo, ColorUpdate, ConflictCheck, Count, DisableKinematic, Dispose, DrawLine, DrawLineGizmos, DrawLineHandles, DrawLineHelper, DrawPath ...
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ITypeParser.cs
- 6 lines; kind=interface; type=ITypeParser; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=7 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/IVACamera.cs
- 176 lines; kind=MonoBehaviour; type=IVACamera; bases=IGameCamera; ancestry=IGameCamera/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=8, Input.=7, Time.=4, KeyCode=4, Lifecycle=2, Camera=2; unity_score=75
- ksp refs: GameSettings=14, FlightCamera=10; fan_in=1 fan_out=5
- methods(10): Activate, Deactivate, DeactivateAll, DisableCamera, EnableCamera, ResetCamera, ResetState, Start, Update, UpdateState
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/IVASun.cs
- 37 lines; kind=MonoBehaviour; type=IVASun; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Light=4, Transform=3, Lifecycle=2, GetComponent=2, MonoBehaviour=1; unity_score=90
- ksp refs: FlightGlobals=1; fan_in=0 fan_out=3
- methods(2): LateUpdate, Start
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/IVesselAutoRename.cs
- 6 lines; kind=interface; type=IVesselAutoRename; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/KeepRot.cs
- 16 lines; kind=MonoBehaviour; type=KeepRot; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, Lifecycle=2, MonoBehaviour=1; unity_score=73
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): LateUpdate, Start
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/Kerbal.cs
- 380 lines; kind=MonoBehaviour; type=Kerbal; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI.Screens.Flight, UnityEngine
- unity: Coroutine=8, Transform=5, Texture=4, Renderer=4, Camera=4, Time.=3; unity_score=68
- ksp refs: FlightGlobals=6, ProtoCrewMember=4, Vessel=3, GameEvents=3, Part=1; fan_in=33 fan_out=12
- methods(17): Awake, CameraOverlayUpdate, CheckEVAUnlocked, DestroyAvatarTexture, IVADisable, IVAEnable, OnDestroy, SetVisibleInPortrait, SetupCache, SetupKerbalModel, ShowHelmet, Start, die, kerbalAvatarUpdate, kerbalAvatarUpdateCycle, kerbalSeatCamUpdate, onVesselSitChange
- auto suggestion: tier=C subsystem=Kerbals

## Assembly-CSharp/KerbalAnimationManager.cs
- 233 lines; kind=class; type=KerbalAnimationManager; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Animation=5, GetComponent=3, LayerMask=2, Transform=1; unity_score=33
- ksp refs: KerbalEVA=2; fan_in=2 fan_out=2
- methods(4): GetAllAnimations, Start, SyncAnimationLayers, setupAnimation
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/KerbalAnimationState.cs
- 66 lines; kind=class; type=KerbalAnimationState; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Animation=4, Transform=2, GetComponent=1; unity_score=54
- ksp refs: none; fan_in=1 fan_out=2
- methods(3): GetAnimationString, SetManager, string
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/KerbalEVA.cs **MUST-OPEN (>800 lines)**
- 7368 lines; kind=PartModule; type=KerbalEVA; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions, FinePrint.Utilities, KSP.Localization, KSP.UI.Screens.Flight, UnityEngine
- unity: Transform=258, GameObject=78, Rigidbody=58, Coroutine=40, SetActive=35, GetComponent=35; unity_score=64
- ksp refs: GameSettings=87, KSPField=67, GameEvents=50, Localizer=34, FlightGlobals=32, PhysicsGlobals=27; fan_in=34 fan_out=98
- methods(269): AcquirePosition, AcquireRotation, AddFlag, AddRBAnchor, AnchorUpdate, ApplyWingnutTorque, AutoTransition, Banana, BoardPart, BoardSeat, CacheLocalStrings, CalculateGroundLevelAngle, CanEVAWithoutHelmet, CanPlantFlag, CanRecover, CanSafelyRemoveHelmet, ChangeHelmet, ChangeNeckRing, CheckHelmetOffSafe, CheckLadderTriggers, ClearLadderHold, CompareContactsByNormalToSurface, ControlPanelInteractionFinished, Dzhanibekov, EjectFromSeat, EnableCharacterAndLadderColliders, EnterConstructionMode, ExitConstructionMode, FindClamberSrf, FindControlPanel, FinishedPlayingGolf, FinishedSmashingBanana, FinishedSpinningWingnut, FixedUpdate, GetClamberPath, GetCurrentColor, GetEjectPoint, GetL19Contact, GetPreLoadPressure, HandleMovementInput, HasWeldLineOfSight, InitHelmetSetup, IntegrateRagdollRigidbodyForces, InterpolateLadders, InterruptWeld ...
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/KerbalExpressionAI.cs
- 183 lines; kind=MonoBehaviour; type=KerbalExpressionAI; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=5, Mesh=5, Lifecycle=5, Time.=3, Coroutine=3, MonoBehaviour=1; unity_score=69
- ksp refs: FlightGlobals=4, GameEvents=3, ProtoCrewMember=1; fan_in=0 fan_out=4
- methods(10): Awake, FixedUpdate, OnDestroy, Start, Update, UpdateExpressionAI, UpdateHeadMesh, UpdateHeadTransforms, explosionReaction, kerbalAvatarUpdateCycle
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/kerbalExpressionSystem.cs
- 396 lines; kind=MonoBehaviour; type=kerbalExpressionSystem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Animation=8, Time.=7, Lifecycle=5, Shader=4, Coroutine=2, MonoBehaviour=1; unity_score=56
- ksp refs: FlightGlobals=5, GameEvents=3, ProtoCrewMember=2, Part=1, KerbalEVA=1; fan_in=0 fan_out=6
- methods(10): Awake, EVAUpdate, FixedUpdate, IVAUpdate, OnDestroy, SetNewIdleExpression, Start, Update, UpdateExpressionAI, explosionReaction
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/KerbalFSM.cs
- 214 lines; kind=class; type=KerbalFSM; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Time.=6; unity_score=16
- ksp refs: none; fan_in=10 fan_out=3
- methods(9): AddEvent, AddEventExcluding, AddState, FixedUpdateFSM, LateUpdateFSM, RunEvent, StartFSM, UpdateFSM, updateFSM
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/KerbalInstructor.cs
- 153 lines; kind=MonoBehaviour; type=KerbalInstructor; bases=KerbalInstructorBase; ancestry=KerbalInstructorBase/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Animation=8, SendMessage/Invoke=3, Lifecycle=1; unity_score=61
- ksp refs: GameSettings=4; fan_in=6 fan_out=3
- methods(6): PlayEmote, PlayEmoteQueued, PlayEmoteRepeating, RepeatEmote, Start, StopRepeatingEmote
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/KerbalInstructorBase.cs
- 69 lines; kind=MonoBehaviour; type=KerbalInstructorBase; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Animation=4, Lifecycle=2, GetComponent=2, Transform=1, Texture=1, MonoBehaviour=1; unity_score=83
- ksp refs: none; fan_in=3 fan_out=2
- methods(5): ClearCamera, OnDestroy, SetupAnimations, SetupCamera, Start
- auto suggestion: tier=B subsystem=Tutorials
