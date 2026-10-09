# child_2: 615 files, 97834 lines

## Assembly-CSharp/LookAtObject.cs
- L=46 kind=MonoBehaviour type=LookAtObject bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(75): Lifecycle=4, Transform=3, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(5): Awake, FixedUpdate, LateUpdate, Update, onUpdate
- auto: C / Camera

## Assembly-CSharp/MainCanvasUtil.cs
- L=33 kind=static class type=MainCanvasUtil bases=- anc=- ns=-
- unity(67): UnityEngine.UI=5, GetComponent=1 | ksp: none | fan_in=2
- methods(0): 
- auto: C / UI

## Assembly-CSharp/MainMenuEnvLogic.cs
- L=114 kind=MonoBehaviour type=MainMenuEnvLogic bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(85): Transform=12, SetActive=3, TMPro=2, Renderer=2, Material=2, Lifecycle=2 | ksp: none | fan_in=1
- methods(5): Awake, DistanceFadeUI, GoCurrentStage, GoToStage, Update
- auto: B / Scene

## Assembly-CSharp/MainMenuExpressionManager.cs
- L=23 kind=MonoBehaviour type=MainMenuExpressionManager bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(88): Shader=3, MonoBehaviour=1, Lifecycle=1, Animation=1 | ksp: none | fan_in=0
- methods(1): Start
- auto: B / Scene

## Assembly-CSharp/MainMenuTerrainSelector.cs
- L=21 kind=MonoBehaviour type=MainMenuTerrainSelector bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(88): GameObject=7, SetActive=1, MonoBehaviour=1, Lifecycle=1 | ksp: GameSettings=2 | fan_in=0
- methods(1): Start
- auto: B / Scene

## Assembly-CSharp/MakingHistoryAboutDialog.cs
- L=80 kind=MonoBehaviour type=MakingHistoryAboutDialog bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(85): UnityEngine.UI=4, GameObject=3, Transform=2, Lifecycle=2, Instantiate/Destroy=2, Texture=1 | ksp: InputLockManager=2, GameSettings=2 | fan_in=1
- methods(7): Create, Dismiss, OnCloseButton, OnDontShowAgainToggle, OnMoreInfoButton, Start, Update
- auto: C / DLC-MakingHistory

## Assembly-CSharp/ManeuverGizmo.cs
- L=368 kind=MonoBehaviour type=ManeuverGizmo bases=ManeuverGizmoBase anc=ManeuverGizmoBase/MonoBehaviour ns=-
- unity(51): Input.=7, UnityEngine.UI=4, Lifecycle=3, GetComponent=2, Camera=2, Transform=1 | ksp: Orbit=8, InputLockManager=6, Vector3d=2, Planetarium=2, Localizer=1 | fan_in=3
- methods(26): DraggingForwardInTime, HandlesUpdatedCallback, LateUpdate, NextOrbit, NextOrbitPossible, OnAntinormalUpdate, OnDeletePress, OnDestroy, OnMinusOrbitPress, OnMouseDown, OnMouseDrag, OnMouseUp, OnNormalUpdate, OnPlusOrbitPress, OnProgradeUpdate, OnRadialInUpdate, OnRadialOutUpdate, OnRetrogradeUpdate, PreviousOrbit, PreviousOrbitPossible, SetMouseOverGizmo, SetPatches, Setup, Start ...
- auto: C / MapView

## Assembly-CSharp/ManeuverGizmoBase.cs
- L=234 kind=MonoBehaviour type=ManeuverGizmoBase bases=MonoBehaviour,IMouseEvents anc=MonoBehaviour ns=-
- unity(75): Transform=12, GameObject=9, SetActive=8, Time.=3, MonoBehaviour=2, Material=2 | ksp: Vector3d=7 | fan_in=6
- methods(17): ChangeToButtonMode, GetInstance, LateUpdate, OnAntinormalUpdate, OnMouseDown, OnMouseDrag, OnMouseEnter, OnMouseExit, OnMouseUp, OnNormalUpdate, OnProgradeUpdate, OnRadialInUpdate, OnRadialOutUpdate, OnRetrogradeUpdate, SetMouseOverGizmo, Start, Terminate
- auto: B / MapView

## Assembly-CSharp/ManeuverGizmoHandle.cs
- L=204 kind=MonoBehaviour type=ManeuverGizmoHandle bases=MonoBehaviour,IMouseEvents anc=MonoBehaviour ns=-
- unity(68): Transform=6, Material=4, Time.=3, GetComponent=3, Renderer=2, MonoBehaviour=2 | ksp: GameSettings=2 | fan_in=2
- methods(11): GetInstance, HandleUpdate, OnMouseDown, OnMouseDrag, OnMouseEnter, OnMouseExit, OnMouseUp, SetHandleColor, Start, Update, UpdateHandle
- auto: B / MapView

## Assembly-CSharp/ManeuverNode.cs
- L=133 kind=class type=ManeuverNode bases=IConfigNode anc=IConfigNode ns=-
- unity(25): GameObject=2, SetActive=1, Instantiate/Destroy=1, GetComponent=1 | ksp: Vector3d=10, Orbit=3, QuaternionD=2, ConfigNode=2, MapView=1 | fan_in=19
- methods(8): AttachGizmo, DetachGizmo, GetBurnVector, GetPartialDv, Load, OnGizmoUpdated, RemoveSelf, Save
- auto: B / Orbit

## Assembly-CSharp/ManeuverNodeEditorSlider.cs
- L=39 kind=MonoBehaviour type=ManeuverNodeEditorSlider bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(87): EventSystems=4, UnityEngine.UI=3, MonoBehaviour=1, Lifecycle=1 | ksp: GameSettings=2 | fan_in=0
- methods(3): OnPointerEnter, OnPointerExit, Update
- auto: C / MapView

## Assembly-CSharp/ManeuverNodeEditorTab.cs
- L=87 kind=MonoBehaviour type=ManeuverNodeEditorTab bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(79): EventSystems=4, Transform=3, UnityEngine.UI=2, Texture=2, Time.=1, MonoBehaviour=1 | ksp: InputLockManager=1 | fan_in=7
- methods(9): IsTabInteractable, OnPointerEnter, OnPointerExit, SetInitialValues, Setup, Update, UpdateUIElements, WrapperUpdateUIElements, setMouseOver
- auto: C / MapView

## Assembly-CSharp/ManeuverNodeEditorTabButton.cs
- L=29 kind=MonoBehaviour type=ManeuverNodeEditorTabButton bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(94): UnityEngine.UI=5, Transform=4, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=3
- methods(1): Setup
- auto: C / MapView

## Assembly-CSharp/ManeuverNodeEditorTabIntercept.cs
- L=347 kind=MonoBehaviour type=ManeuverNodeEditorTabIntercept bases=ManeuverNodeEditorTab anc=ManeuverNodeEditorTab/MonoBehaviour ns=-
- unity(42): TMPro=7, Lifecycle=1, GetComponent=1 | ksp: FlightGlobals=34, Localizer=22, Orbit=5, Planetarium=4, Vector3d=2 | fan_in=1
- methods(8): CacheLocalStrings, GetInterceptValues, IsTabInteractable, SetEditorTab, SetInitialValues, SetIntersectOrApproach, Start, UpdateUIElements
- auto: C / MapView

## Assembly-CSharp/ManeuverNodeEditorTabOrbitAdv.cs
- L=69 kind=MonoBehaviour type=ManeuverNodeEditorTabOrbitAdv bases=ManeuverNodeEditorTab anc=ManeuverNodeEditorTab/MonoBehaviour ns=-
- unity(75): TMPro=5, GetComponent=2, Lifecycle=1 | ksp: Planetarium=1, Orbit=1, InputLockManager=1, FlightGlobals=1 | fan_in=0
- methods(4): IsTabInteractable, SetInitialValues, Start, UpdateUIElements
- auto: C / MapView

## Assembly-CSharp/ManeuverNodeEditorTabOrbitBasic.cs
- L=83 kind=MonoBehaviour type=ManeuverNodeEditorTabOrbitBasic bases=ManeuverNodeEditorTab anc=ManeuverNodeEditorTab/MonoBehaviour ns=-
- unity(71): TMPro=5, GetComponent=2, Lifecycle=1 | ksp: Localizer=4, FlightGlobals=3, Planetarium=2, Orbit=1 | fan_in=1
- methods(5): CacheLocalStrings, IsTimeAndPeriodUnlocked, SetInitialValues, Start, UpdateUIElements
- auto: C / MapView

## Assembly-CSharp/ManeuverNodeEditorTabPosition.cs
- L=5 kind=enum type=ManeuverNodeEditorTabPosition bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / MapView

## Assembly-CSharp/ManeuverNodeEditorTabVectorHandles.cs
- L=204 kind=MonoBehaviour type=ManeuverNodeEditorTabVectorHandles bases=ManeuverNodeEditorTab anc=ManeuverNodeEditorTab/MonoBehaviour ns=-
- unity(72): UnityEngine.UI=11, GetComponent=9, TMPro=1, Lifecycle=1 | ksp: FlightGlobals=8, Localizer=2 | fan_in=1
- methods(16): AntiNormalStepUp, CacheLocalStrings, NextOrbitLoop, NormalStepUp, OnPrecisionValueChanged, PrevOrbitLoop, ProgradeStepUp, RadialInStepUp, RadialOutStepUp, RetrogradeStepUp, SetHandlesSensitivity, SetInitialValues, Start, TimeStepDown, TimeStepUp, UpdateUIElements
- auto: C / MapView

## Assembly-CSharp/ManeuverNodeEditorTabVectorInput.cs
- L=171 kind=MonoBehaviour type=ManeuverNodeEditorTabVectorInput bases=ManeuverNodeEditorTab anc=ManeuverNodeEditorTab/MonoBehaviour ns=-
- unity(65): TMPro=5, GetComponent=3, SetActive=2, Lifecycle=2, GameObject=2, UnityEngine.UI=1 | ksp: Localizer=4, InputLockManager=2, FlightGlobals=2, Vector3d=1 | fan_in=3
- methods(9): CacheLocalStrings, ChangeInputMode, OnDestroy, OnFieldDeselected, OnFieldSelected, OnTabToggleClicked, SetInitialValues, Start, UpdateUIElements
- auto: C / MapView

## Assembly-CSharp/ManeuverNodeEditorVectorHandle.cs
- L=214 kind=MonoBehaviour type=ManeuverNodeEditorVectorHandle bases=MonoBehaviour,IDragHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(76): EventSystems=12, UnityEngine.UI=4, Input.=4, Coroutine=4, Lifecycle=2, GetComponent=2 | ksp: GameSettings=2 | fan_in=1
- methods(8): MouseUpCooldown, OnDrag, OnPointerDown, OnPointerEnter, OnPointerExit, OnPointerUp, Start, Update
- auto: C / MapView

## Assembly-CSharp/ManeuverTool.cs
- L=514 kind=MonoBehaviour type=ManeuverTool bases=UIApp anc=UIApp/MonoBehaviour ns=-
- unity(41): GameObject=6, Transform=4, SetActive=3, UnityEngine.UI=2, Lifecycle=1, Instantiate/Destroy=1 | ksp: GameEvents=8, Localizer=6, FlightGlobals=5, Vessel=4, HighLogic=4 | fan_in=3
- methods(31): ChangeTime, CreateManeuver, CreateManeuverChecksComplete, DeletePriorManeuvers, DisplayApp, FollowingManevuersCheck, GenerateTransferTypes, GetAppScenes, GetAppScreenPos, GetTransferVisualPrefab, HideApp, Initial_MouseInput_PointerExit, InvokeAsync, IsAnyDropdownOpen, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnFollowingCheckCancel, OnFollowingCheckOk, OnFollowingLeave, OnManeuverChanged, OnPositionChanged, OnPriorCheckCancel, OnPriorCheckOk ...
- auto: C / MapView

## Assembly-CSharp/ManeuverToolCBContainer.cs
- L=206 kind=MonoBehaviour type=ManeuverToolCBContainer bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(94): Transform=17, GameObject=12, GetComponent=8, Shader=5, Renderer=5, Material=5 | ksp: Localizer=1 | fan_in=2
- methods(14): CreateThumbnailCamera, Hide, OnDestroy, OnFalse, OnPointerEnter, OnPointerExit, OnTrue, RotatePlanet, SetLightDirection, Setup, Show, StartRotation, StopRotation, Update
- auto: C / MapView

## Assembly-CSharp/ManeuverToolVisualTextBox.cs
- L=27 kind=MonoBehaviour type=ManeuverToolVisualTextBox bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): TMPro=2, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(3): SetNameText, SetValueText, Setup
- auto: B / MapView

## Assembly-CSharp/MapDisplayTypes.cs
- L=7 kind=enum type=MapDisplayTypes bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / MapView

## Assembly-CSharp/MapObject.cs
- L=196 kind=MonoBehaviour type=MapObject bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(60): Transform=5, GetComponent=5, Lifecycle=4, GameObject=4, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: CelestialBody=7, Vessel=6, Orbit=4, ScaledSpace=3 | fan_in=45
- methods(14): Awake, Create, GetDisplayName, GetName, GetReferenceBody, LateUpdate, OnAwake, OnDestroy, OnGetName, OnLateUpdate, OnStart, OnWillDestroy, Start, Terminate
- auto: B / MapView

## Assembly-CSharp/MapView.cs
- L=672 kind=MonoBehaviour type=MapView bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(74): Camera=22, Renderer=16, Transform=10, Material=10, GameObject=10, Coroutine=5 | ksp: FlightGlobals=8, Planetarium=7, GameEvents=6, OrbitDriver=5, HighLogic=5 | fan_in=61
- methods(19): AddLaunchSiteNode, Awake, CreateKSCNode, CreateLaunchSiteNodes, CreateOrbitRenderers, CreateVectorCanvasCam, EnterMapView, ExitMapView, GetEccOffset, OnDestroy, Start, Update, UpdateMap, createMapFXCamera, enableMapFXCameras, endEnterMapTransition, endExitMapTransition, enterMapView, exitMapView
- auto: C / MapView

## Assembly-CSharp/MapViewFiltering.cs
- L=478 kind=MonoBehaviour type=MapViewFiltering bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(38): Coroutine=4, UnityEngine.UI=3, Lifecycle=3, SetActive=1, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: GameEvents=19, Vessel=7, FlightGlobals=6, InputLockManager=4, ProtoVessel=2 | fan_in=9
- methods(24): Awake, CheckAgainstFilter, CountVessels, GetFilterFromButtonStates, GetFilterState, Init, Initialize, LoadFilterState, OnDestroy, OnInputLocksModified, RefreshCounts, RefreshInOneFrame, SetFilter, SetSolo, Start, ToggleFiltersPanel, UpdateVesselCounts, checkAgainstFilter, onKnowledgeChanged, onVesselCountChanged, onVesselRename, setFilter, updateButtonsToFilter, updateFilterFromButtons
- auto: C / MapView

## Assembly-CSharp/MaterialBasedOnGraphicsSetting.cs
- L=94 kind=MonoBehaviour type=MaterialBasedOnGraphicsSetting bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(76): Material=7, Renderer=2, Lifecycle=2, MonoBehaviour=1, GetComponent=1 | ksp: GameSettings=3, GameEvents=2, HighLogic=1 | fan_in=0
- methods(4): ApplyMaterials, Awake, OnDestroy, OnGameSettingsApplied
- auto: C / Rendering

## Assembly-CSharp/MaterialColorUpdater.cs
- L=90 kind=class type=MaterialColorUpdater bases=- anc=- ns=-
- unity(72): Renderer=8, Material=4, Transform=3, Shader=3, GetComponent=2 | ksp: Part=2 | fan_in=2
- methods(2): CreateRendererList, Update
- auto: C / Rendering

## Assembly-CSharp/MaterialExtension.cs
- L=15 kind=static class type=MaterialExtension bases=- anc=- ns=-
- unity(44): Material=2 | ksp: none | fan_in=0
- methods(1): CopyKeywordsFrom
- auto: C / Rendering

## Assembly-CSharp/MaterialPropertyExtensions.cs
- L=35 kind=static class type=MaterialPropertyExtensions bases=- anc=- ns=-
- unity(63): Material=3, Renderer=2 | ksp: none | fan_in=9
- methods(3): Apply, SetupProperties, UpdateProperties
- auto: C / Rendering

## Assembly-CSharp/MaterialSetDirection.cs
- L=37 kind=MonoBehaviour type=MaterialSetDirection bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(94): Transform=4, Renderer=3, Material=2, Lifecycle=2, Shader=1, MonoBehaviour=1 | ksp: none | fan_in=2
- methods(3): Reset, Start, Update
- auto: C / Rendering

## Assembly-CSharp/Mathfx.cs
- L=149 kind=class type=Mathfx bases=- anc=- ns=-
- unity(27): none | ksp: none | fan_in=7
- methods(18): Approx, Berp, Bounce, Clerp, Coserp, CubicHermiteSpline, Ease, Easerp, GetWeightedAvgVector3, Hermite, HermiteIntegral, Lerp, NearestPoint, NearestPointStrict, Sinerp, SinerpIntegral, SmoothStep, XLerp
- auto: A / Core

## Assembly-CSharp/Matrix4x4D.cs
- L=392 kind=struct type=Matrix4x4D bases=- anc=- ns=-
- unity(1): none | ksp: Vector3d=10 | fan_in=0
- methods(16): Det, Identity, Inverse, Matrix4x4D, MultiplyPoint3x4, RotateXaxis, RotateYaxis, RotateZaxis, Scale, SetIdentity, TransformPoint, TransformVector, Translate, Zero, det2x2, det3x3
- auto: A / Core

## Assembly-CSharp/MatrixArrayMaterialProperty.cs
- L=17 kind=class type=MatrixArrayMaterialProperty bases=MaterialPropertyExtensions.MaterialProperty anc=MaterialProperty ns=-
- unity(32): Material=1 | ksp: none | fan_in=0
- methods(1): Apply
- auto: C / Rendering

## Assembly-CSharp/MatrixMaterialProperty.cs
- L=17 kind=class type=MatrixMaterialProperty bases=MaterialPropertyExtensions.MaterialProperty anc=MaterialProperty ns=-
- unity(46): Shader=1, Material=1 | ksp: none | fan_in=0
- methods(1): Apply
- auto: C / Rendering

## Assembly-CSharp/MBMReader.cs
- L=116 kind=static class type=MBMReader bases=- anc=- ns=-
- unity(55): Texture=11 | ksp: none | fan_in=1
- methods(2): Read, ReadTexture2D
- auto: C / Rendering

## Assembly-CSharp/MCAvatarController.cs
- L=423 kind=MonoBehaviour type=MCAvatarController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(31): Time.=4, Lifecycle=3, Animation=2, MonoBehaviour=1 | ksp: Localizer=38 | fan_in=1
- methods(9): DetermineAndSetFallbackLoop, NewAvatarState, OnInstructorReady, OnReady, Start, TriggerAnim, TriggerAnimation, Update, getThoughtVector
- auto: B / DLC-MakingHistory

## Assembly-CSharp/MCDespawner.cs
- L=18 kind=MonoBehaviour type=MCDespawner bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(74): UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1, GameObject=1 | ksp: GameEvents=1 | fan_in=0
- methods(2): BtnExit, Start
- auto: B / DLC-MakingHistory

## Assembly-CSharp/MCSceneSpawner.cs
- L=32 kind=MonoBehaviour type=MCSceneSpawner bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(58): Lifecycle=2, MonoBehaviour=1 | ksp: GameEvents=4, HighLogic=1 | fan_in=0
- methods(4): Awake, OnDestroy, OnMCDespawn, OnMCSpawn
- auto: B / DLC-MakingHistory

## Assembly-CSharp/MEBannerBrowser.cs
- L=206 kind=MonoBehaviour type=MEBannerBrowser bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(38): UnityEngine.UI=2, Transform=2, MonoBehaviour=1 | ksp: Localizer=9, GameDatabase=4 | fan_in=1
- methods(12): Accept, BannerDeletedCallback, BannerSelectedCallback, ConfirmDeleteBanner, CreateBrowser, Dismiss, GetDuplicate, LoadBannerButtons, LoadGameDataBanners, LoadMissionBanners, OnButtonDelete, Setup
- auto: B / DLC-MakingHistory

## Assembly-CSharp/MEBannerEntry.cs
- L=134 kind=class type=MEBannerEntry bases=- anc=- ns=-
- unity(26): Texture=3, Instantiate/Destroy=1 | ksp: none | fan_in=5
- methods(9): CopySource, DeleteSource, DestroyTexture, GetBannerDefaultTexture, GetBannerGameDataPath, GetStockBannerGameDataPath, HasValidSource, LoadFromMissionFolder, SetToDefault
- auto: B / DLC-MakingHistory

## Assembly-CSharp/MEBannerType.cs
- L=6 kind=enum type=MEBannerType bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=9
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/MEBasicNodeListFilter.cs
- L=36 kind=class type=MEBasicNodeListFilter bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(2): FilterList, ToString
- auto: A / DLC-MakingHistory

## Assembly-CSharp/MEBasicNodeListFilterList.cs
- L=59 kind=class type=MEBasicNodeListFilterList bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(6): AddFilter, ClearFilter, FilterList, GetFilteredList, RemoveFilter, ToString
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Mechanic.cs
- L=128 kind=MonoBehaviour type=Mechanic bases=SpaceCenterCrew anc=SpaceCenterCrew/MonoBehaviour ns=-
- unity(29): Lifecycle=2 | ksp: none | fan_in=7
- methods(3): SetAnimation, Start, Update
- auto: B / Kerbals

## Assembly-CSharp/MechanicDriller.cs
- L=89 kind=MonoBehaviour type=MechanicDriller bases=SpaceCenterCrew anc=SpaceCenterCrew/MonoBehaviour ns=-
- unity(33): Lifecycle=2 | ksp: none | fan_in=0
- methods(3): SetAnimation, Start, Update
- auto: B / Kerbals

## Assembly-CSharp/MechanicHammer.cs
- L=128 kind=MonoBehaviour type=MechanicHammer bases=SpaceCenterCrew anc=SpaceCenterCrew/MonoBehaviour ns=-
- unity(29): Lifecycle=2 | ksp: none | fan_in=0
- methods(3): SetAnimation, Start, Update
- auto: B / Kerbals

## Assembly-CSharp/MechanicLugWrench.cs
- L=128 kind=MonoBehaviour type=MechanicLugWrench bases=SpaceCenterCrew anc=SpaceCenterCrew/MonoBehaviour ns=-
- unity(27): Lifecycle=2 | ksp: none | fan_in=0
- methods(3): SetAnimation, Start, Update
- auto: B / Kerbals

## Assembly-CSharp/MechanicScrew.cs
- L=128 kind=MonoBehaviour type=MechanicScrew bases=SpaceCenterCrew anc=SpaceCenterCrew/MonoBehaviour ns=-
- unity(29): Lifecycle=2 | ksp: none | fan_in=0
- methods(3): SetAnimation, Start, Update
- auto: B / Kerbals

## Assembly-CSharp/MechanicWrench.cs
- L=128 kind=MonoBehaviour type=MechanicWrench bases=SpaceCenterCrew anc=SpaceCenterCrew/MonoBehaviour ns=-
- unity(29): Lifecycle=2 | ksp: none | fan_in=0
- methods(3): SetAnimation, Start, Update
- auto: B / Kerbals

## Assembly-CSharp/MechanicWrenchEnglish.cs
- L=128 kind=MonoBehaviour type=MechanicWrenchEnglish bases=SpaceCenterCrew anc=SpaceCenterCrew/MonoBehaviour ns=-
- unity(29): Lifecycle=2 | ksp: none | fan_in=0
- methods(3): SetAnimation, Start, Update
- auto: B / Kerbals

## Assembly-CSharp/MEGapCategoryScroll.cs
- L=30 kind=MonoBehaviour type=MEGapCategoryScroll bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(83): EventSystems=4, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(3): OnDestroy, OnPointerEnter, OnPointerExit
- auto: C / DLC-MakingHistory

## Assembly-CSharp/MemoryCache.cs
- L=84 kind=static class type=MemoryCache bases=- anc=- ns=-
- unity(6): none | ksp: none | fan_in=4
- methods(3): CleanUp, GetBytes, GetColor32
- auto: A / Core

## Assembly-CSharp/MenuGravityOverride.cs
- L=11 kind=MonoBehaviour type=MenuGravityOverride bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(64): Physics.=1, MonoBehaviour=1, Lifecycle=1 | ksp: PhysicsGlobals=1 | fan_in=0
- methods(1): Start
- auto: B / Physics

## Assembly-CSharp/MenuMusic.cs
- L=78 kind=MonoBehaviour type=MenuMusic bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(80): SceneManager=3, Instantiate/Destroy=3, GameObject=3, Audio=3, SendMessage/Invoke=2, Lifecycle=2 | ksp: HighLogic=1, GameSettings=1 | fan_in=1
- methods(6): Awake, OnDestroy, OnLevelLoaded, OnSceneLoaded, PlayAmbienceLoop, PlayTheme
- auto: C / Audio

## Assembly-CSharp/MenuNavigation.cs
- L=689 kind=MonoBehaviour type=MenuNavigation bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(83): UnityEngine.UI=34, EventSystems=20, GameObject=17, Transform=16, KeyCode=13, Input.=13 | ksp: InputLockManager=3, GameEvents=1 | fan_in=22
- methods(26): CheckKeyboardInput, EnsureFocusIsWithinDialog, FocusNextItemFromTextInput, FocusOnItemByAnchoredPos, FocusOnItemByScrollbar, GetSelectableParent, IsSearchFieldFocused, IsSelectableValid, MouseIsHovering, OnDestroy, RefocusOnInputDetect, ResetAll, ResetSelectablesOnly, SelectLastArrowSelected, SetItemAsFirstSelected, SetLastItemSelected, SetMenuNavInputLock, SetSameParentExplicitNavigation, SetSearchField, SetSelectableItems, SetVerticalExplicitNavigation, SpawnMenuNavigation, Start, SumbmitOnSelectedToggle ...
- auto: C / Scene

## Assembly-CSharp/MenuNavInput.cs
- L=11 kind=enum type=MenuNavInput bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=5
- methods(0): 
- auto: A / Input

## Assembly-CSharp/MenuRandomKerbalAnims.cs
- L=35 kind=MonoBehaviour type=MenuRandomKerbalAnims bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(70): Lifecycle=2, Time.=1, MonoBehaviour=1, Animation=1 | ksp: none | fan_in=0
- methods(2): Start, Update
- auto: B / Kerbals

## Assembly-CSharp/MeshLocator.cs
- L=8 kind=MonoBehaviour type=MeshLocator bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(36): MonoBehaviour=1 | ksp: none | fan_in=0
- methods(0): 
- auto: C / Rendering

## Assembly-CSharp/MessageSystemAdvancedButton.cs
- L=31 kind=MonoBehaviour type=MessageSystemAdvancedButton bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(99): Texture=11, UnityEngine.UI=2, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/MiniBiome.cs
- L=71 kind=ScriptableObject type=MiniBiome bases=ScriptableObject anc=ScriptableObject ns=-
- unity(14): ScriptableObject=1 | ksp: Localizer=1 | fan_in=6
- methods(2): ConvertTagtoBiome, ConvertTagtoLandedAt
- auto: A / Celestials

## Assembly-CSharp/MiniKeyBindings.cs
- L=57 kind=MonoBehaviour type=MiniKeyBindings bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(75): UnityEngine.UI=2, Lifecycle=2, GameObject=2, MonoBehaviour=1, Instantiate/Destroy=1, GetComponent=1 | ksp: Localizer=2 | fan_in=1
- methods(5): Create, Dismiss, Start, Update, drawWindow
- auto: B / Input

## Assembly-CSharp/MiniSettings.cs
- L=129 kind=MonoBehaviour type=MiniSettings bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): UnityEngine.UI=6, GameObject=5, Lifecycle=2, Transform=1, SetActive=1, MonoBehaviour=1 | ksp: Localizer=5, HighLogic=5, GameSettings=1, GameEvents=1 | fan_in=2
- methods(8): ApplySettings, Create, Dismiss, OnDifficultyOptionsDismiss, OpenDifficultyOptions, Start, Update, drawWindow
- auto: C / UI

## Assembly-CSharp/MinMaxColor.cs
- L=15 kind=class type=MinMaxColor bases=- anc=- ns=-
- unity(21): none | ksp: none | fan_in=1
- methods(1): GetLerp
- auto: C / UI

## Assembly-CSharp/MinMaxFloat.cs
- L=20 kind=class type=MinMaxFloat bases=- anc=- ns=-
- unity(17): none | ksp: none | fan_in=3
- methods(2): GetInverseLerp, GetLerp
- auto: A / Core

## Assembly-CSharp/MissionBriefingTag.cs
- L=53 kind=MonoBehaviour type=MissionBriefingTag bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(74): UnityEngine.UI=2, Instantiate/Destroy=2, GetComponent=2, TMPro=1, MonoBehaviour=1 | ksp: GameEvents=1 | fan_in=1
- methods(3): OnRemoveMe, SetupContent, Spawn
- auto: C / DLC-MakingHistory

## Assembly-CSharp/MissionControlBuilding.cs
- L=56 kind=MonoBehaviour type=MissionControlBuilding bases=SpaceCenterBuilding anc=SpaceCenterBuilding/MonoBehaviour ns=-
- unity(22): none | ksp: Localizer=6, HighLogic=5, GameEvents=4, InputLockManager=3 | fan_in=0
- methods(5): IsOpen, OnClicked, OnOnDestroy, OnPopupWarningDismiss, onDialogClose
- auto: B / Scene

## Assembly-CSharp/MissionScreenTutorial.cs
- L=105 kind=ScenarioModule type=MissionScreenTutorial bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(16): none | ksp: GameSettings=2, ConfigNode=2 | fan_in=1
- methods(12): CreateMissionBriefingTutorialPage, CreateMissionScreenTutorialPage, CreateMissionScreenTutorialPages, CreateMissionTutorialPage, OnAssetSetup, OnDoneButtonClick, OnEnterCreateMissionBriefingTutorialPage, OnEnterMissionScreenTutorialPage, OnEnterMissionTutorialPage, OnLoad, OnSave, OnTutorialSetup
- auto: A / Tutorials

## Assembly-CSharp/ModalGUI.cs
- L=33 kind=MonoBehaviour type=ModalGUI bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(98): IMGUI=12, Screen/Cursor=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(2): Window, setHotControl
- auto: C / UI

## Assembly-CSharp/ModelAnimationFX.cs
- L=106 kind=MonoBehaviour type=ModelAnimationFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(69): Transform=7, Animation=3, GetComponent=1, GameObject=1 | ksp: ConfigNode=4, GameDatabase=1 | fan_in=0
- methods(4): OnEvent, OnInitialize, OnLoad, OnSave
- auto: C / Rendering

## Assembly-CSharp/ModelMultiParticleFX.cs
- L=203 kind=MonoBehaviour type=ModelMultiParticleFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(74): GameObject=11, Transform=7, ParticleSystem=6, Instantiate/Destroy=2, GetComponent=2, SetActive=1 | ksp: ConfigNode=4, GameDatabase=1 | fan_in=0
- methods(6): OnDestroy, OnEvent, OnInitialize, OnLoad, OnSave, SetEmitter
- auto: C / Rendering

## Assembly-CSharp/ModelPartEvent.cs
- L=21 kind=MonoBehaviour type=ModelPartEvent bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(50): MonoBehaviour=1, Lifecycle=1 | ksp: Part=1 | fan_in=1
- methods(2): Awake, RecieveEvent
- auto: B / Part

## Assembly-CSharp/ModelParticleFX.cs
- L=130 kind=MonoBehaviour type=ModelParticleFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(66): Transform=6, ParticleSystem=2, SetActive=1, Lifecycle=1, Instantiate/Destroy=1, GetComponent=1 | ksp: ConfigNode=4, GameDatabase=1 | fan_in=0
- methods(5): OnDestroy, OnEvent, OnInitialize, OnLoad, OnSave
- auto: C / Rendering

## Assembly-CSharp/ModifierChangeWhen.cs
- L=6 kind=enum type=ModifierChangeWhen bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=14
- methods(0): 
- auto: A / Misc

## Assembly-CSharp/ModifierStagingSituation.cs
- L=6 kind=enum type=ModifierStagingSituation bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=16
- methods(0): 
- auto: A / Vessel

## Assembly-CSharp/ModuleAblator.cs
- L=278 kind=PartModule type=ModuleAblator bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(30): Lifecycle=3, Resources.=2 | ksp: KSPField=21, Localizer=6, HighLogic=3, TimeWarp=2, PhysicsGlobals=2 | fan_in=0
- methods(7): FixedUpdate, GetInfo, GetModuleDisplayName, OnAwake, Start, Update, UpdateColor
- auto: A / Thermal

## Assembly-CSharp/ModuleActiveRadiator.cs
- L=523 kind=PartModule type=ModuleActiveRadiator bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(19): Lifecycle=3 | ksp: KSPField=13, Part=12, Localizer=11, PhysicsGlobals=7, KSPAction=3 | fan_in=4
- methods(25): Activate, ActivateAction, ApplyMaxEnergyTransferAdjustments, ApplyRadiatorActivation, CacheLocalStrings, CheckDebugFields, CheckIfDeployable, CheckPartCaches, CheckResources, FixedUpdate, GetInfo, GetModuleDisplayName, InternalCooling, IsAdjusterBlockingCooling, IsSibling, OnAwake, OnDestroy, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnVesselStandardModification, Shutdown, ShutdownAction, Start, ToggleRadiatorAction ...
- auto: A / Thermal

## Assembly-CSharp/ModuleAeroSurface.cs
- L=234 kind=PartModule type=ModuleAeroSurface bases=ModuleControlSurface,IMultipleDragCube anc=ModuleControlSurface/ModuleLiftingSurface/PartModule ns=-
- unity(30): Transform=1, Lifecycle=1 | ksp: Localizer=6, KSPField=5, TimeWarp=4, HighLogic=2, Vessel=1 | fan_in=2
- methods(14): ActionToggleBrakes, CacheLocalStrings, CtrlSurfaceEditorUpdate, CtrlSurfaceUpdate, GetInfo, GetModuleDisplayName, ModifyAeroAuthorityLimiter, ModifyAeroAuthorityLimiterUI, OnAwake, OnInventoryModeDisable, OnInventoryModeEnable, OnStart, OnStoredInInventory, Start
- auto: A / Aero

## Assembly-CSharp/ModuleAlternator.cs
- L=163 kind=PartModule type=ModuleAlternator bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(20): Lifecycle=1 | ksp: KSPField=9, Localizer=5, PartModule=1, HighLogic=1, ConfigNode=1 | fan_in=4
- methods(9): ApplyOutputAdjustments, FixedUpdate, GetInfo, GetModuleDisplayName, OnAwake, OnLoad, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStart
- auto: A / PartModules-Resources

## Assembly-CSharp/ModuleAnalysisResource.cs
- L=43 kind=PartModule type=ModuleAnalysisResource bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(31): Lifecycle=1 | ksp: KSPField=4, PartModule=1, HighLogic=1 | fan_in=3
- methods(3): OnStart, SetupAnalysis, Update
- auto: A / PartModules-Resources

## Assembly-CSharp/ModuleAnchoredDecoupler.cs
- L=100 kind=PartModule type=ModuleAnchoredDecoupler bases=ModuleDecouplerBase anc=ModuleDecouplerBase/PartModule/MonoBehaviour ns=-
- unity(52): Transform=3, Coroutine=2, SetActive=1, GameObject=1 | ksp: Part=4, Localizer=2, KSPField=2, GameEvents=1 | fan_in=5
- methods(6): ApplyEjectionForces, GetInfo, GetModuleDisplayName, OnActive, OnDecouple, OnStart
- auto: B / PartModules-Structural

## Assembly-CSharp/ModuleAnimateGeneric.cs
- L=754 kind=PartModule type=ModuleAnimateGeneric bases=PartModule,IScalarModule,IMultipleDragCube anc=PartModule/MonoBehaviour ns=-
- unity(28): Animation=4, UnityEngine.UI=2, Time.=1, Lifecycle=1, LayerMask=1 | ksp: KSPField=31, Localizer=8, HighLogic=8, ProtoPartModuleSnapshot=1, PartModule=1 | fan_in=5
- methods(27): AnimationHandler, AssumeDragCubePosition, CacheLocalStrings, CheckCrewState, CheckForRestrictions, FailAnimationForCargoBay, FindAnimation, FixedUpdate, GetAnimation, GetDragCubeNames, GetState, IsMoving, OnAwake, OnInventoryModeDisable, OnSave, OnStart, OnStoredInInventory, RestartFailedAnimationForCargoBay, SetDragState, SetScalar, SetUIRead, SetUIWrite, Toggle, ToggleAction ...
- auto: B / Part

## Assembly-CSharp/ModuleAnimateHeat.cs
- L=80 kind=PartModule type=ModuleAnimateHeat bases=ModuleAnimationSetter anc=ModuleAnimationSetter/PartModule/MonoBehaviour ns=-
- unity(24): Lifecycle=1 | ksp: KSPField=5, ConfigNode=1 | fan_in=0
- methods(5): HeatEffectStartup, OnLoad, OnStart, Update, UpdateHeatEffect
- auto: C / Rendering

## Assembly-CSharp/ModuleAnimationGroup.cs
- L=736 kind=PartModule type=ModuleAnimationGroup bases=PartModule,IMultipleDragCube anc=PartModule/MonoBehaviour ns=-
- unity(40): Time.=9, Animation=9, Lifecycle=4, LayerMask=3, ParticleSystem=2, MonoBehaviour=2 | ksp: KSPField=18, GameEvents=7, Localizer=6, HighLogic=6, KSPAction=3 | fan_in=5
- methods(38): AssumeDragCubePosition, CacheLocalStrings, CheckAnimationState, CheckForActivity, DeployComplete, DeployModule, DeployModuleAction, DisableModules, EnableModules, FindAnimations, FindModules, GetCurrentPlayingAnimationState, GetDragCubeNames, LateSetup, LateUpdate, OnDestroy, OnInventoryModeDisable, OnLoad, OnPause, OnStart, OnStoredInInventory, OnUnpause, PlayDeployAnimation, RetractComplete ...
- auto: B / Part

## Assembly-CSharp/ModuleAnimationSetter.cs
- L=121 kind=PartModule type=ModuleAnimationSetter bases=PartModule,IScalarModule anc=PartModule/MonoBehaviour ns=-
- unity(54): Animation=5, Lifecycle=1, LayerMask=1 | ksp: KSPField=4, FloatCurve=2, PartModule=1, Localizer=1 | fan_in=2
- methods(11): AnimStartup, CacheLocalStrings, GetModuleDisplayName, IsMoving, OnAwake, OnStart, SetScalar, SetUIRead, SetUIWrite, Update, UpdateAnim
- auto: B / Part

## Assembly-CSharp/ModuleAnimatorLandingGear.cs
- L=32 kind=PartModule type=ModuleAnimatorLandingGear bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: PartModule=1, KSPField=1, KSPEvent=1 | fan_in=0
- methods(2): InputGearToggle, OnStart
- auto: A / PartModules-Wheels

## Assembly-CSharp/ModuleAsteroid.cs
- L=404 kind=PartModule type=ModuleAsteroid bases=PartModule,IVesselAutoRename,IPartMassModifier anc=PartModule/MonoBehaviour ns=-
- unity(43): Transform=4, GameObject=4, Coroutine=2, Analytics=2, Resources.=1, Lifecycle=1 | ksp: KSPField=12, HighLogic=7, Vessel=6, Localizer=5, GameEvents=5 | fan_in=15
- methods(24): CacheLocalStrings, GetModuleMass, GetModuleMassChangeWhen, GetVesselName, GetVesselType, MakeTarget, OnAwake, OnDestroy, OnGenComplete, OnLoad, OnSave, OnStart, PostInit, Rangefinder, RenameAsteroidEvent, SetAsteroidMass, TakeSampleEVAEvent, UpdateDragCube, VesselsUndockingAsteroid, drawRenameWindow, onRenameConfirm, onRenameDismiss, performSampleExperiment, renameAsteroid
- auto: B / Celestials

## Assembly-CSharp/ModuleAsteroidAnalysis.cs
- L=80 kind=PartModule type=ModuleAsteroidAnalysis bases=ModuleSpaceObjectAnalysis anc=ModuleSpaceObjectAnalysis/PartModule/MonoBehaviour ns=-
- unity(24): Lifecycle=1 | ksp: Part=3, PartModule=2, GameEvents=2, Vessel=1, HighLogic=1 | fan_in=0
- methods(5): CheckForPotato, ClearAsteroidResources, FindAsteroidResources, OnDestroy, OnStart
- auto: A / Science

## Assembly-CSharp/ModuleAsteroidDrill.cs
- L=286 kind=PartModule type=ModuleAsteroidDrill bases=BaseDrill anc=BaseDrill/BaseConverter/PartModule ns=-
- unity(31): Raycast/Query=2, Transform=1, Physics.=1, GameObject=1 | ksp: Localizer=15, Part=3, KSPField=3, PartResourceDefinition=1, HighLogic=1 | fan_in=1
- methods(10): CacheLocalStrings, CheckForImpact, GetAttachedPotato, GetInfo, GetModuleDisplayName, IsSituationValid, OnStart, OnUpdate, PostProcess, PrepareRecipe
- auto: B / PartModules-Resources

## Assembly-CSharp/ModuleAsteroidInfo.cs
- L=48 kind=PartModule type=ModuleAsteroidInfo bases=ModuleSpaceObjectInfo anc=ModuleSpaceObjectInfo/PartModule/MonoBehaviour ns=-
- unity(29): Lifecycle=1 | ksp: Localizer=1, HighLogic=1 | fan_in=1
- methods(3): OnStart, SetupAsteroidResources, Update
- auto: A / Celestials

## Assembly-CSharp/ModuleAsteroidResource.cs
- L=3 kind=PartModule type=ModuleAsteroidResource bases=ModuleSpaceObjectResource anc=ModuleSpaceObjectResource/PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / PartModules-Resources

## Assembly-CSharp/ModuleBiomeScanner.cs
- L=57 kind=PartModule type=ModuleBiomeScanner bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(27): Lifecycle=1 | ksp: Localizer=2, FlightGlobals=2, CelestialBody=2, Vessel=1, PartModule=1 | fan_in=0
- methods(2): FixedUpdate, RunAnalysis
- auto: A / PartModules-Resources

## Assembly-CSharp/ModuleCargoPart.cs
- L=400 kind=PartModule type=ModuleCargoPart bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(48): Coroutine=5, Audio=4, Rigidbody=3, Transform=1, Resources.=1, Lifecycle=1 | ksp: GameEvents=16, Localizer=10, KSPField=8, Part=7, GameSettings=3 | fan_in=14
- methods(26): EnableModule, GetInfo, GetModuleDisplayName, GetTooltip, HasLinkedCompoundParts, HideUI, IsDeployedSciencePart, MakePartSettle, OnCompoundPartDetached, OnCompoundPartLinked, OnDestroy, OnEVAConstructionMode, OnInitialize, OnLoad, OnPartPack, OnPartUnpack, OnPartWillDie, OnStart, OnUpdate, OnUpdateHighlight, OnWeldFinish, OnWeldStart, PlayDeployAnimation, SetConstructionHighlight ...
- auto: B / Kerbals

## Assembly-CSharp/ModuleColorChanger.cs
- L=456 kind=PartModule type=ModuleColorChanger bases=PartModule,IScalarModule anc=PartModule/MonoBehaviour ns=-
- unity(41): Renderer=4, Shader=3, Material=3, Lifecycle=3, UnityEngine.UI=1 | ksp: KSPField=24, FloatCurve=8, TimeWarp=2, PartModule=1, Localizer=1 | fan_in=3
- methods(19): CacheLocalStrings, EditRenderers, FixedUpdate, GetModuleDisplayName, IsMoving, OnAwake, OnLoad, OnStart, ProcessMaterialsList, SetEventName, SetScalar, SetState, SetUIRead, SetUIWrite, Start, ToggleAction, ToggleEvent, Update, UpdateColor
- auto: C / Rendering

## Assembly-CSharp/ModuleCometAnalysis.cs
- L=80 kind=PartModule type=ModuleCometAnalysis bases=ModuleSpaceObjectAnalysis anc=ModuleSpaceObjectAnalysis/PartModule/MonoBehaviour ns=-
- unity(24): Lifecycle=1 | ksp: Part=3, PartModule=2, GameEvents=2, Vessel=1, HighLogic=1 | fan_in=0
- methods(5): CheckForComet, ClearCometResources, FindCometResources, OnDestroy, OnStart
- auto: A / Science

## Assembly-CSharp/ModuleCometDrill.cs
- L=286 kind=PartModule type=ModuleCometDrill bases=BaseDrill anc=BaseDrill/BaseConverter/PartModule ns=-
- unity(31): Raycast/Query=2, Transform=1, Physics.=1, GameObject=1 | ksp: Localizer=15, Part=3, KSPField=3, PartResourceDefinition=1, HighLogic=1 | fan_in=0
- methods(10): CacheLocalStrings, CheckForImpact, GetAttachedPotato, GetInfo, GetModuleDisplayName, IsSituationValid, OnStart, OnUpdate, PostProcess, PrepareRecipe
- auto: B / PartModules-Resources

## Assembly-CSharp/ModuleCometInfo.cs
- L=48 kind=PartModule type=ModuleCometInfo bases=ModuleSpaceObjectInfo anc=ModuleSpaceObjectInfo/PartModule/MonoBehaviour ns=-
- unity(29): Lifecycle=1 | ksp: Localizer=1, HighLogic=1 | fan_in=1
- methods(3): OnStart, SetupCometResources, Update
- auto: A / Celestials

## Assembly-CSharp/ModuleCometResource.cs
- L=3 kind=PartModule type=ModuleCometResource bases=ModuleSpaceObjectResource anc=ModuleSpaceObjectResource/PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / PartModules-Resources

## Assembly-CSharp/ModuleCommand.cs
- L=708 kind=PartModule type=ModuleCommand bases=PartModule,ICommNetControlSource,IResourceConsumer anc=PartModule/MonoBehaviour ns=-
- unity(47): Transform=17, GameObject=14, Lifecycle=3, Instantiate/Destroy=3 | ksp: Localizer=25, KSPField=13, HighLogic=8, GameSettings=6, Vessel=5 | fan_in=17
- methods(29): CacheLocalStrings, ChangeControlPoint, ControlPointsExist, DestroyCPArrows, FixedUpdate, GetConsumedResources, GetControlPoint, GetControlSourceState, GetInfo, GetModuleDisplayName, HibernateToggle, IsCommCapable, MakeReference, MakeReferenceToggle, OnAwake, OnDestroy, OnLoad, OnNetworkInitialised, OnSave, OnStart, RenameVessel, SetControlPoint, Start, ToggleControlPointVisual ...
- auto: B / PartModules-Control

## Assembly-CSharp/ModuleConductionMultiplier.cs
- L=41 kind=PartModule type=ModuleConductionMultiplier bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(43): Lifecycle=2 | ksp: KSPField=2, PartModule=1, HighLogic=1 | fan_in=0
- methods(3): FixedUpdate, OnAwake, Start
- auto: A / Thermal

## Assembly-CSharp/ModuleCoreHeat.cs
- L=705 kind=PartModule type=ModuleCoreHeat bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(20): Lifecycle=3, Time.=1, MonoBehaviour=1 | ksp: KSPField=30, PhysicsGlobals=15, Part=13, Localizer=7, Planetarium=3 | fan_in=1
- methods(24): AddEnergyToCore, AddFluxToRadiator, CheckCoreShutdown, CheckDebugFields, CheckPartCaches, CheckStartingTemperature, FixedUpdate, GetDeltaTime, GetInfo, GetLastFlux, GetModuleDisplayName, GetTransferAmount, HasActiveConverters, HasEnabledConverters, MoveCoreEnergyToRadiators, OnAwake, OnDestroy, OnLoad, OnSave, OnVesselStandardModification, ResolveConverterEnergy, ResolveNetEnergy, Start, UpdateConverterModuleCache
- auto: A / Thermal

## Assembly-CSharp/ModuleDecouple.cs
- L=178 kind=PartModule type=ModuleDecouple bases=ModuleDecouplerBase anc=ModuleDecouplerBase/PartModule/MonoBehaviour ns=-
- unity(51): Transform=6, Coroutine=4, GameObject=1 | ksp: Part=9, Vessel=3, Localizer=3, KSPField=3, FlightGlobals=2 | fan_in=9
- methods(8): ApplyEjectionForces, GetInfo, GetModuleDisplayName, OnActive, OnAwake, OnDecouple, OnStart, WaitAndSwitchFocus
- auto: B / PartModules-Structural

## Assembly-CSharp/ModuleDecouplerBase.cs
- L=335 kind=PartModule type=ModuleDecouplerBase bases=PartModule,IModuleInfo,IStageSeparator anc=PartModule/MonoBehaviour ns=-
- unity(24): Lifecycle=2, GameObject=1 | ksp: KSPField=9, Localizer=5, Part=3, AttachNode=3, Vessel=2 | fan_in=3
- methods(30): CheckShielded, ClosedAndLocked, Decouple, DecoupleAction, GetDrawModulePanelCallback, GetModuleTitle, GetPart, GetPrimaryField, GetStageIndex, GetStagingDisableText, GetStagingEnableText, GetVessel, IsAdjusterBlockingDecouple, IsEnginePlate, IsStageable, LateUpdate, OnActive, OnAwake, OnDecouple, OnDestroy, OnJettison, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStart ...
- auto: A / PartModules-Structural

## Assembly-CSharp/ModuleDeployableAntenna.cs
- L=59 kind=PartModule type=ModuleDeployableAntenna bases=ModuleDeployablePart anc=ModuleDeployablePart/PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: Localizer=4, HighLogic=2, ProtoPartModuleSnapshot=1, ConfigNode=1 | fan_in=1
- methods(4): Extend, GetModuleDisplayName, OnInventoryModeDisable, OnLoad
- auto: A / CommNet

## Assembly-CSharp/ModuleDeployableRadiator.cs
- L=36 kind=PartModule type=ModuleDeployableRadiator bases=ModuleDeployablePart anc=ModuleDeployablePart/PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: Localizer=3, ProtoPartModuleSnapshot=1, ConfigNode=1 | fan_in=3
- methods(4): CacheLocalStrings, GetModuleDisplayName, OnInventoryModeDisable, OnLoad
- auto: A / Thermal

## Assembly-CSharp/ModuleDeployableSolarPanel.cs
- L=397 kind=PartModule type=ModuleDeployableSolarPanel bases=ModuleDeployablePart,IContractObjectiveModule anc=ModuleDeployablePart/PartModule/MonoBehaviour ns=-
- unity(55): Transform=9, GameObject=7, LayerMask=4, Raycast/Query=3, Physics.=2, Lifecycle=2 | ksp: KSPField=19, Localizer=6, HighLogic=6, FloatCurve=6, ScaledSpace=3 | fan_in=8
- methods(18): ApplyEfficiencyAdjustments, CacheLocalStrings, CalculateTrackingLOS, CheckContractObjectiveValidity, FixedUpdate, GetContractObjectiveType, GetInfo, GetModuleDisplayName, OnAwake, OnDestroy, OnInventoryModeDisable, OnLoad, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnPartRepaired, OnStart, PostCalculateTracking, PostFSMUpdate
- auto: B / PartModules-Resources

## Assembly-CSharp/ModuleDisplaceTweak.cs
- L=101 kind=PartModule type=ModuleDisplaceTweak bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(33): Transform=1, Lifecycle=1 | ksp: KSPField=6, GameEvents=2, PartModule=1, Part=1 | fan_in=0
- methods(5): GetFinalPosition, OnDestroy, OnPartEvent, OnStart, UpdateMorph
- auto: A / PartModules-Structural

## Assembly-CSharp/ModuleDragAreaModifier.cs
- L=8 kind=PartModule type=ModuleDragAreaModifier bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: KSPField=2, PartModule=1 | fan_in=1
- methods(0): 
- auto: A / Aero

## Assembly-CSharp/ModuleDragModifier.cs
- L=8 kind=PartModule type=ModuleDragModifier bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: KSPField=2, PartModule=1 | fan_in=1
- methods(0): 
- auto: A / Aero

## Assembly-CSharp/ModuleDynamicNodes.cs
- L=167 kind=PartModule type=ModuleDynamicNodes bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(43): Transform=4, Lifecycle=2, SetActive=1, GameObject=1 | ksp: AttachNode=5, KSPField=4, ConfigNode=3, Part=2, GameEvents=2 | fan_in=0
- methods(11): ChangeNodeSet, FixedUpdate, InitializeNodeSets, OnAwake, OnDestroy, OnLoad, OnShipModified, OnStart, OnStartFinished, SetupAutoStrut, ToggleNodeSet
- auto: B / PartModules-Structural

## Assembly-CSharp/ModuleEffectTest.cs
- L=55 kind=PartModule type=ModuleEffectTest bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(35): Time.=1, Lifecycle=1 | ksp: KSPField=5, KSPEvent=2, HighLogic=2, PartModule=1 | fan_in=0
- methods(4): FireAction, OnStart, ToggleAction, Update
- auto: A / Part

## Assembly-CSharp/ModuleEnginesFX.cs
- L=191 kind=PartModule type=ModuleEnginesFX bases=ModuleEngines anc=ModuleEngines/PartModule/MonoBehaviour ns=-
- unity(17): none | ksp: KSPField=9, TimeWarp=2, PartResourceDefinition=1, Localizer=1 | fan_in=4
- methods(13): CacheLocalStrings, CanStart, DeactivateLoopingFX, DeactivatePowerFX, DeactivateRunningFX, FXReset, FXUpdate, InitializeFX, OnAwake, PlayEngageFX, PlayFlameoutFX, PlayShutdownFX, SetListener
- auto: A / PartModules-Engines

## Assembly-CSharp/ModuleEnviroSensor.cs
- L=176 kind=PartModule type=ModuleEnviroSensor bases=PartModule,IResourceConsumer anc=PartModule/MonoBehaviour ns=-
- unity(34): UnityEngine.UI=2, Transform=1, Lifecycle=1 | ksp: Localizer=9, PartResourceDefinition=3, KSPField=3, TimeWarp=1, PartModule=1 | fan_in=1
- methods(9): CacheLocalStrings, FixedUpdate, GetConsumedResources, GetInfo, GetModuleDisplayName, OnAwake, OnLoad, Toggle, ToggleAction
- auto: A / Science

## Assembly-CSharp/ModuleEvaChute.cs
- L=319 kind=PartModule type=ModuleEvaChute bases=ModuleParachute anc=ModuleParachute/PartModule/MonoBehaviour ns=-
- unity(76): Transform=21, Renderer=7, Shader=6, GetComponent=5, Material=3, Rigidbody=2 | ksp: KSPField=12, KerbalEVA=2, ProtoCrewMember=1, Part=1, KSPEvent=1 | fan_in=4
- methods(19): AllowRepack, CanCrewMemberUseParachute, CutParachute, FixedUpdate, GetFullyDeployedCanopyRotation, GetSemiDeployedCanopyRotation, IsStageable, OnActive, OnDestroy, OnParachuteFullyDeployed, OnParachuteSemiDeployed, OnStart, PassedAdditionalDeploymentChecks, SetCanopy, SetEVAChuteActive, ShowEVAChuteParamsChanged, ToggleEVAChuteParams, UpdateFullyDeployedParachuteMovement, UpdateSemiDeployedParachuteMovement
- auto: B / Kerbals

## Assembly-CSharp/ModuleExperienceManagement.cs
- L=73 kind=PartModule type=ModuleExperienceManagement bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(20): none | ksp: Localizer=8, HighLogic=3, ProtoCrewMember=2, Funding=2, PartModule=1 | fan_in=0
- methods(4): FinalizeExperience, GetInfo, GetModuleDisplayName, OnStart
- auto: A / Kerbals

## Assembly-CSharp/ModuleFlightDisplay.cs
- L=210 kind=PartModule type=ModuleFlightDisplay bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(85): IMGUI=40 | ksp: FlightGlobals=3, HighLogic=2, Vessel=1, PartModule=1, Localizer=1 | fan_in=0
- methods(13): GetActiveGUI, GetControlState, GetFirstHop, GetLastHop, GetSignalStrength, GetTotalHops, InitStyles, OnDraw, OnStart, OnWindow, Setup, ToggleGui, TurnOffGui
- auto: C / PartModules-Control

## Assembly-CSharp/ModuleFuelJettison.cs
- L=49 kind=PartModule type=ModuleFuelJettison bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(35): Resources.=1 | ksp: Localizer=3, PartResource=2, PartModule=1, KSPField=1, KSPEvent=1 | fan_in=0
- methods(4): Dump, FuelJettison, FuelJettisonAction, OnStart
- auto: A / PartModules-Structural

## Assembly-CSharp/ModuleGenerator.cs
- L=269 kind=PartModule type=ModuleGenerator bases=PartModule,IContractObjectiveModule anc=PartModule/MonoBehaviour ns=-
- unity(22): UnityEngine.UI=1, Lifecycle=1 | ksp: KSPField=15, Localizer=7, KSPAction=3, KSPEvent=2, PartModule=1 | fan_in=4
- methods(15): Activate, ActivateAction, ApplyEfficiencyAdjustments, CacheLocalStrings, CheckContractObjectiveValidity, FixedUpdate, GetContractObjectiveType, GetInfo, GetModuleDisplayName, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStart, Shutdown, ShutdownAction, ToggleAction
- auto: A / PartModules-Resources

## Assembly-CSharp/ModuleGimbal.cs
- L=466 kind=PartModule type=ModuleGimbal bases=PartModule,ITorqueProvider anc=PartModule/MonoBehaviour ns=-
- unity(47): Transform=18, Lifecycle=1 | ksp: KSPField=21, Localizer=9, KSPAction=6, TimeWarp=1, PartModule=1 | fan_in=3
- methods(24): ApplyControlAdjustments, CacheLocalStrings, CreateEngineList, FixedUpdate, FreeAction, GetInfo, GetModuleDisplayName, GetPotentialTorque, GimbalRotation, IsStageable, LockAction, OnActive, OnLoad, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStart, SetActionsSuffix, ToggleAction, TogglePitchAction, ToggleRollAction, ToggleToggles, ToggleYawAction, UpdatePAWUI, UpdateToggles
- auto: B / PartModules-Engines

## Assembly-CSharp/ModuleGPS.cs
- L=176 kind=PartModule type=ModuleGPS bases=PartModule,IAnimatedModule anc=PartModule/MonoBehaviour ns=-
- unity(25): Time.=1, MonoBehaviour=1, Lifecycle=1 | ksp: Localizer=10, KSPField=4, FlightGlobals=2, PartModule=1, HighLogic=1 | fan_in=2
- methods(9): CacheLocalStrings, CheckForAsteroidConnect, CheckForCometConnect, DisableModule, EnableModule, GetModuleDisplayName, IsSituationValid, ModuleIsActive, Update
- auto: A / Science

## Assembly-CSharp/ModuleGroundCommsPart.cs
- L=137 kind=PartModule type=ModuleGroundCommsPart bases=ModuleGroundSciencePart,ICommAntenna anc=ModuleGroundSciencePart/ModuleGroundPart/ModuleCargoPart ns=-
- unity(15): none | ksp: Localizer=10, KSPField=6, ConfigNode=3, ProtoPartModuleSnapshot=2, Part=1 | fan_in=2
- methods(6): CanComm, CanCommUnloaded, CanScienceTo, CommPowerUnloaded, GetInfo, GetModuleDisplayName
- auto: A / DLC-BreakingGround

## Assembly-CSharp/ModuleGroundExpControl.cs
- L=404 kind=PartModule type=ModuleGroundExpControl bases=ModuleGroundCommsPart,ICommNetControlSource anc=ModuleGroundCommsPart/ModuleGroundSciencePart/ModuleGroundPart ns=-
- unity(39): Transform=4, Coroutine=4, GetComponent=3, Lifecycle=2, Instantiate/Destroy=1 | ksp: GameEvents=19, Localizer=17, KSPField=6, Part=3, HighLogic=3 | fan_in=8
- methods(26): CacheLocalStrings, CommPowerUnloaded, GetControlSourceState, GetInfo, GetModuleDisplayName, GetTooltip, IsCommCapable, OnAwake, OnDestroy, OnGroundScienceClusterPowerStateChanged, OnGroundScienceClusterUpdated, OnGroundScienceModuleRemoved, OnGroundSciencePartDeployed, OnGroundSciencePartEnabledStateChanged, OnPartActionUIDismiss, OnPartActionUIOpened, OnSave, OnStart, OnUpdate, RecalcBoosterPower, ScanAndAddControlUnit, SetDeployedSciencePartVesselType, SetVesselNaming, Start ...
- auto: B / DLC-BreakingGround

## Assembly-CSharp/ModuleGroundExperiment.cs
- L=212 kind=PartModule type=ModuleGroundExperiment bases=ModuleGroundSciencePart anc=ModuleGroundSciencePart/ModuleGroundPart/ModuleCargoPart ns=-
- unity(30): Lifecycle=2, Instantiate/Destroy=1 | ksp: KSPField=7, Localizer=6, GameEvents=6, ResearchAndDevelopment=2, FloatCurve=2 | fan_in=9
- methods(11): CacheLocalStrings, GetInfo, GetModuleDisplayName, OnDestroy, OnGroundScienceGenerated, OnGroundScienceTransmitted, OnLoad, OnStart, RetrievePart, Start, UpdateModuleUI
- auto: A / DLC-BreakingGround

## Assembly-CSharp/ModuleGroundPart.cs
- L=479 kind=PartModule type=ModuleGroundPart bases=ModuleCargoPart,IAnimatedModule anc=ModuleCargoPart/PartModule/MonoBehaviour ns=-
- unity(51): Rigidbody=9, Coroutine=5, Instantiate/Destroy=3, Collision/Force=3, Joint=2, SendMessage/Invoke=1 | ksp: GameEvents=16, FlightGlobals=10, KSPField=6, HighLogic=5, Vessel=4 | fan_in=7
- methods(26): CacheLocalStrings, CanBeStored, DisableModule, EnableModule, GetModuleDisplayName, HasChildParts, IsSituationValid, MakePartKinematic, ModuleIsActive, OnDestroy, OnLeavingScene, OnPartActionUIDismiss, OnPartActionUIShown, OnPartPack, OnPartUnpack, OnPartWillDie, OnRetractCompleted, OnStart, OnUpdate, OnVesselChange, PlayDeployAnimation, PlayRetractAnimation, RetrievePart, RetrieveScienceData ...
- auto: B / DLC-BreakingGround

## Assembly-CSharp/ModuleGroundSciencePart.cs
- L=659 kind=PartModule type=ModuleGroundSciencePart bases=ModuleGroundPart anc=ModuleGroundPart/ModuleCargoPart/PartModule ns=-
- unity(51): Transform=16, GameObject=7, LayerMask=4, Raycast/Query=3, Lifecycle=3, Physics.=2 | ksp: Localizer=23, GameEvents=22, KSPField=17, TimeWarp=3, Part=3 | fan_in=14
- methods(23): CacheLocalStrings, CalculateTrackingLOS, FixedUpdate, GetInfo, GetModuleDisplayName, GetTooltip, OnAwake, OnDestroy, OnGroundScienceClusterPowerStateChanged, OnGroundScienceClusterUpdated, OnGroundScienceDeregisterCluster, OnLoad, OnPartActionUIDismiss, OnPartActionUIOpened, OnStart, OnUpdate, RetrievePart, SetDeployedOnGround, Start, SunTrackerToOrigin, ToggleActive, ToggleActiveAction, UpdateModuleUI
- auto: B / DLC-BreakingGround

## Assembly-CSharp/ModuleHeatEffects.cs
- L=69 kind=PartModule type=ModuleHeatEffects bases=PartModuleFXSetter anc=PartModuleFXSetter/PartModule/MonoBehaviour ns=-
- unity(25): Lifecycle=1 | ksp: KSPField=5 | fan_in=0
- methods(4): HeatEffectStartup, OnStart, Update, UpdateHeatEffect
- auto: C / Rendering

## Assembly-CSharp/ModuleJettison.cs
- L=667 kind=PartModule type=ModuleJettison bases=PartModule,IActivateOnDecouple,IScalarModule anc=PartModule/MonoBehaviour ns=-
- unity(71): Collider=21, Transform=17, GameObject=14, Rigidbody=12, SetActive=9, GetComponent=6 | ksp: KSPField=22, AttachNode=5, PhysicsGlobals=4, HighLogic=4, Localizer=2 | fan_in=5
- methods(31): AssumeDragCubePosition, CheckAttachNode, DecoupleAction, FetchActiveMeshTransform, GetDragCubeNames, GetModuleMass, GetModuleMassChangeWhen, GetStagingDisableText, GetStagingEnableText, IsMoving, IsStageable, Jettison, JettisonAction, LateUpdate, LoadJettisonMeshState, OnActive, OnDestroy, OnInventoryModeDisable, OnLoad, OnStart, SetActionsSuffix, SetFairingDragCube, SetIgnoreCollisionFlags, SetScalar ...
- auto: B / PartModules-Structural

## Assembly-CSharp/ModuleJointMotor.cs
- L=267 kind=PartModule type=ModuleJointMotor bases=PartModule,IJointLockState anc=PartModule/MonoBehaviour ns=-
- unity(42): Joint=3, Coroutine=2, Transform=1, GameObject=1 | ksp: ConfigNode=4, PartJoint=3, Part=2, GameEvents=2, AttachNode=2 | fan_in=1
- methods(23): GetMotorForce, GetMotorMode, GetMotorSpeed, InitJoint, IsJointUnlocked, OnJointInit, OnLoad, OnModuleLoad, OnModuleSave, OnModuleStart, OnMotorModeChanged, OnPartPack, OnSave, OnStart, SetMotorForce, SetMotorMode, SetMotorSpeed, applyCoordsUpdate, findJointAtNode, getControlOrt, onPartJointBreak, recurseCoordUpdate, startAfterJointsCreated
- auto: B / DLC-BreakingGround

## Assembly-CSharp/ModuleJointMotorTest.cs
- L=171 kind=PartModule type=ModuleJointMotorTest bases=PartModule,IActiveJointHost,IJointLockState anc=PartModule/MonoBehaviour ns=-
- unity(40): GameObject=2, Transform=1, Lifecycle=1 | ksp: KSPEvent=5, KSPField=3, PartModule=1, Part=1 | fan_in=0
- methods(16): GetHostPart, GetLocalTransform, GetMotorForce, GetMotorSpeed, IsJointUnlocked, LateUpdate, LinkToThrottle, MotorDrive, MotorNeutral, MotorPark, OnDriveModeChanged, OnJointInit, OnStart, Reverse, SetMotorForce, SetMotorSpeed
- auto: A / DLC-BreakingGround

## Assembly-CSharp/ModuleJointPivot.cs
- L=18 kind=PartModule type=ModuleJointPivot bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(36): Lifecycle=1 | ksp: ConfigNode=2, PartModule=1 | fan_in=0
- methods(4): OnLoad, OnSave, OnStart, Update
- auto: A / DLC-BreakingGround

## Assembly-CSharp/ModuleKerbNetAccess.cs
- L=470 kind=PartModule type=ModuleKerbNetAccess bases=PartModule,IAccessKerbNet,IModuleInfo anc=PartModule/MonoBehaviour ns=-
- unity(22): SendMessage/Invoke=3, Lifecycle=1 | ksp: Vessel=17, Localizer=13, KSPField=7, GameEvents=7, ConfigNode=5 | fan_in=3
- methods(30): CacheLocalStrings, CheckRequiredEffects, CheckVesselSituationFoV, GetDrawModulePanelCallback, GetEnhancedSituations, GetInfo, GetKerbNetAnomalyChance, GetKerbNetDisplayModes, GetKerbNetErrorState, GetKerbNetMaximumFoV, GetKerbNetMinimumFoV, GetKerbNetPart, GetKerbNetVessel, GetModuleDisplayName, GetModuleTitle, GetPrimaryField, HasThisDialogOpen, InvokeCrewEffectCheck, LoadDisplayModes, LoadRequiredEffects, OnAnimationGroupStateChanged, OnAwake, OnDestroy, OnLoad ...
- auto: A / CommNet

## Assembly-CSharp/ModuleLiftingSurface.cs
- L=426 kind=PartModule type=ModuleLiftingSurface bases=PartModule,ILiftProvider anc=PartModule/MonoBehaviour ns=-
- unity(48): Transform=8, Instantiate/Destroy=4, GameObject=4, Rigidbody=2, Lifecycle=1 | ksp: KSPField=19, PhysicsGlobals=10, FloatCurve=8, Localizer=4, FlightGlobals=4 | fan_in=6
- methods(15): ApplyLiftForceAdjustments, CacheLocalStrings, DestroyLiftAndDragArrows, FixedUpdate, GetDragVector, GetInfo, GetLiftVector, GetModuleDisplayName, OnAwake, OnCenterOfLiftQuery, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStart, SetupCoefficients, UpdateAeroDisplay
- auto: B / Aero

## Assembly-CSharp/ModuleMirrorPlacement.cs
- L=46 kind=PartModule type=ModuleMirrorPlacement bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(28): none | ksp: KSPField=7, AttachNode=2, PartModule=1, Part=1 | fan_in=0
- methods(1): OnWillBeMirrored
- auto: A / Part

## Assembly-CSharp/ModuleOrbitalScanner.cs
- L=299 kind=PartModule type=ModuleOrbitalScanner bases=PartModule,IAnimatedModule anc=PartModule/MonoBehaviour ns=-
- unity(26): Lifecycle=3, Shader=1 | ksp: Localizer=9, KSPEvent=6, HighLogic=3, GameEvents=2, PartModule=1 | fan_in=1
- methods(24): ActivateScanner, CacheLocalStrings, DecreaseCutoff, DisableModule, EnableModule, FixedUpdate, GenerateOverlay, GetOverlayColor, HideOverlay, IncreaseCutoff, IsSituationValid, ModuleIsActive, OnActive, OnDestroy, OnResourceMapLoaded, SetColor, SetupMap, ShowResourceName, Start, SwitchResource, ToggleMenu, ToggleOverlay, ToggleSurvey, UpdateGUI
- auto: B / PartModules-Resources

## Assembly-CSharp/ModuleOrbitalSurveyor.cs
- L=165 kind=PartModule type=ModuleOrbitalSurveyor bases=PartModule,IAnimatedModule anc=PartModule/MonoBehaviour ns=-
- unity(24): Lifecycle=2 | ksp: Localizer=13, KSPField=5, ResearchAndDevelopment=3, GameEvents=3, Vessel=2 | fan_in=1
- methods(14): CheckForScanData, CompleteSurvey, DisableModule, EnableModule, GetInfo, GetModuleDisplayName, IsSituationValid, ModuleIsActive, OnDestroy, OnStart, PerformSurvey, Update, rescaleThresholds, sendDataToComms
- auto: A / PartModules-Resources

## Assembly-CSharp/ModuleOverheatDisplay.cs
- L=105 kind=PartModule type=ModuleOverheatDisplay bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(19): none | ksp: Localizer=2, KSPField=2, PartModule=1 | fan_in=1
- methods(3): CacheLocalStrings, OnStart, OnUpdate
- auto: A / Thermal

## Assembly-CSharp/ModulePartFirework.cs
- L=778 kind=PartModule type=ModulePartFirework bases=PartModule,IPartMassModifier anc=PartModule/MonoBehaviour ns=-
- unity(60): GameObject=12, Shader=11, Rigidbody=10, Transform=8, GetComponent=4, Coroutine=4 | ksp: Localizer=17, KSPField=17, HighLogic=7, GameEvents=4, UIPartActionWindow=3 | fan_in=0
- methods(29): GetCurrentColor, GetModuleMass, GetModuleMassChangeWhen, LaunchShell, LaunchShellAction, LaunchShellEvent, OnAwake, OnColorChanged, OnDestroy, OnLoad, OnPAWInit, OnPAWShown, OnSave, OnStart, PresetColors, UpdateFireworksColors, configureColorFields, configureSoundFX, fetchColorPickerFromID, getCurrentFXByType, getCurrentPSByType, getFXFromPrefab, matchColorPickers, pickerID2FieldName ...
- auto: B / Part

## Assembly-CSharp/ModulePartVariants.cs
- L=562 kind=PartModule type=ModulePartVariants bases=PartModule,IModuleInfo,IMultipleDragCube anc=PartModule/MonoBehaviour ns=-
- unity(49): Transform=13, Material=9, Renderer=2, Shader=1, GetComponent=1, GameObject=1 | ksp: EditorLogic=6, Localizer=5, HighLogic=5, Part=4, KSPField=4 | fan_in=11
- methods(33): ApplyVariant, AssumeDragCubePosition, GetCurrentVariantIndex, GetDragCubeNames, GetDrawModulePanelCallback, GetInfo, GetModuleCost, GetModuleCostChangeWhen, GetModuleDisplayName, GetModuleMass, GetModuleMassChangeWhen, GetModuleTitle, GetPrimaryField, GetTitle, GetVariantIndex, GetVariantNames, GetVariantThemeIndex, GetVariantThemeNames, HasVariant, HasVariantTheme, OnAwake, OnLoad, OnSave, OnStart ...
- auto: C / Part

## Assembly-CSharp/ModulePhysicMaterial.cs
- L=264 kind=PartModule type=ModulePhysicMaterial bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(56): Material=5, PhysicMaterial=4, Renderer=1, Lifecycle=1, GetComponent=1, GameObject=1 | ksp: ConfigNode=8, KSPField=6, HighLogic=4, GameEvents=3, Localizer=2 | fan_in=0
- methods(11): ApplyListMaterial, ChangePhysicMaterial, OnAwake, OnDestroy, OnLoad, OnSave, OnStart, OnVariantApplied, SetPhysicMaterial, SetPhysicMaterialColors, UpdatePhysicMaterialEvent
- auto: B / Physics

## Assembly-CSharp/ModuleRCSFX.cs
- L=45 kind=PartModule type=ModuleRCSFX bases=ModuleRCS anc=ModuleRCS/PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: Localizer=1, KSPField=1 | fan_in=1
- methods(7): CacheLocalStrings, DeactivateFX, DeactivatePowerFX, GetModuleDisplayName, OnAwake, SetupFX, UpdatePowerFX
- auto: A / PartModules-Engines

## Assembly-CSharp/ModuleReactionWheel.cs
- L=349 kind=PartModule type=ModuleReactionWheel bases=PartModule,IResourceConsumer,ITorqueProvider anc=PartModule/MonoBehaviour ns=-
- unity(25): UnityEngine.UI=1, Lifecycle=1 | ksp: Localizer=8, KSPField=8, KSPAction=4, PartResourceDefinition=3, ConfigNode=2 | fan_in=3
- methods(21): Activate, ActiveUpdate, ApplyTorqueAdjustments, CacheLocalStrings, CycleAction, Deactivate, FixedUpdate, GetAppliedTorque, GetConsumedResources, GetInfo, GetModuleDisplayName, GetPotentialTorque, IsAdjusterBreakingReactionWheel, OnAwake, OnLoad, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnSave, OnStart, OnToggle, Toggle
- auto: A / PartModules-Control

## Assembly-CSharp/ModuleRemoteController.cs
- L=18 kind=PartModule type=ModuleRemoteController bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(44): UnityEngine.UI=1 | ksp: PartModule=1, KSPField=1, KSPEvent=1 | fan_in=0
- methods(1): Toggle
- auto: A / PartModules-Control

## Assembly-CSharp/ModuleResource.cs
- L=147 kind=class type=ModuleResource bases=IConfigNode anc=IConfigNode ns=-
- unity(3): none | ksp: Localizer=4, PartResourceDefinition=3, ConfigNode=2 | fan_in=15
- methods(5): IsDeprived, Load, OnAwake, PrintRate, Save
- auto: A / PartModules-Resources

## Assembly-CSharp/ModuleResourceAutoShiftState.cs
- L=240 kind=PartModule type=ModuleResourceAutoShiftState bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(25): Resources.=1, Lifecycle=1, Instantiate/Destroy=1 | ksp: Localizer=11, KSPField=10, HighLogic=3, PartResourceDefinition=1, PartResource=1 | fan_in=1
- methods(8): ModifyAmountPercent, ModifyShutOffAmount, ModifyShutOffPercent, ModifyStartUpAmount, ModifyStartUpPercent, OnDestroy, OnStart, ToggleWarningVisibility
- auto: A / PartModules-Resources

## Assembly-CSharp/ModuleResourceConverter.cs
- L=127 kind=PartModule type=ModuleResourceConverter bases=BaseConverter anc=BaseConverter/PartModule/MonoBehaviour ns=-
- unity(21): MonoBehaviour=1 | ksp: Localizer=13, PartResourceDefinition=3, KSPField=1 | fan_in=0
- methods(5): GetInfo, GetModuleDisplayName, IsSituationValid, LoadRecipe, PrepareRecipe
- auto: A / PartModules-Resources

## Assembly-CSharp/ModuleResourceDrain.cs
- L=724 kind=PartModule type=ModuleResourceDrain bases=PartModule,IConstruction anc=PartModule/MonoBehaviour ns=-
- unity(25): Resources.=4, Lifecycle=2, Transform=1, Time.=1 | ksp: PartResource=6, Localizer=6, GameEvents=6, Part=5, KSPField=5 | fan_in=1
- methods(45): CacheLocalStrings, CanBeDetached, CanBeOffset, CanBeRotated, CheckDrainingForEffects, DrainPart, DrainResourceNotFinished, FixedUpdate, ForceUpdateResourcePAW, GetPartResource, GetResourceDrainStatus, HasEnoughDrainingResources, Initialize, IsResourceDraining, LoadResources, LoadResourcesAvailable, OnAwake, OnDestroy, OnLoad, OnPartMenuOpen, OnPartResourceListChanged, OnSave, OnStart, OnStartFinished ...
- auto: A / PartModules-Resources

## Assembly-CSharp/ModuleResourceHandler.cs
- L=380 kind=class type=ModuleResourceHandler bases=- anc=- ns=-
- unity(1): none | ksp: Localizer=6, TimeWarp=4, ConfigNode=4, PartModule=2, GameEvents=2 | fan_in=1
- methods(10): GetAverageInput, HasEnoughResourcesToAutoStart, IsResourceBelowShutOffLimit, LoadResList, OnAwake, OnLoad, PrintModuleResources, SetPartModule, UpdateModuleResourceInputs, UpdateModuleResourceOutputs
- auto: A / PartModules-Resources

## Assembly-CSharp/ModuleResourceHarvester.cs
- L=465 kind=PartModule type=ModuleResourceHarvester bases=BaseDrill,IResourceConsumer anc=BaseDrill/BaseConverter/PartModule ns=-
- unity(28): Transform=2, Raycast/Query=2, Physics.=1, MonoBehaviour=1 | ksp: Localizer=23, KSPField=7, PartResourceDefinition=6, HighLogic=5, Vessel=3 | fan_in=1
- methods(14): CacheLocalStrings, CheckForImpact, GetConsumedResources, GetInfo, GetIntakeMultiplier, GetLocationString, GetModuleDisplayName, IsSituationValid, LoadRecipe, OnStart, PostProcess, PostUpdateCleanup, PreProcessing, PrepareRecipe
- auto: B / PartModules-Resources

## Assembly-CSharp/ModuleResourceIntake.cs
- L=267 kind=PartModule type=ModuleResourceIntake bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(21): Transform=1, Lifecycle=1 | ksp: KSPField=15, Localizer=9, Vector3d=2, KSPEvent=2, FloatCurve=2 | fan_in=8
- methods(10): Activate, CacheLocalStrings, Deactivate, FixedUpdate, GetInfo, GetModuleDisplayName, OnAwake, OnLoad, OnStart, ToggleAction
- auto: A / PartModules-Engines

## Assembly-CSharp/ModuleResourceScanner.cs
- L=259 kind=PartModule type=ModuleResourceScanner bases=PartModule,IAnimatedModule anc=PartModule/MonoBehaviour ns=-
- unity(18): Lifecycle=1 | ksp: Localizer=14, KSPField=5, FlightGlobals=3, PartResourceDefinition=2, PartModule=1 | fan_in=1
- methods(15): CacheLocalStrings, CheckAbundanceDisplay, DisableModule, DisplayAbundance, EnableModule, GetDisplayFormat, GetInfo, GetModuleDisplayName, GetScanningMode, IsSituationValid, ModuleIsActive, OnAwake, OnStart, ToggleEvent, Update
- auto: A / PartModules-Resources

## Assembly-CSharp/ModuleRotatingJoint.cs
- L=281 kind=PartModule type=ModuleRotatingJoint bases=ModuleJointMotor anc=ModuleJointMotor/PartModule/MonoBehaviour ns=-
- unity(20): none | ksp: KSPField=10, KSPAction=6, KSPEvent=5, HighLogic=4, ConfigNode=2 | fan_in=0
- methods(19): EngageAction, EngageDrive, ForwardDrive, InvertAction, InvertDrive, LockAction, LockDrive, NeutralAction, NeutralDrive, OnJointInit, OnModuleLoad, OnModuleSave, OnModuleStart, OnMotorModeChanged, OnUpdate, ReverseDrive, ToggleControlMode, UpdateGuiNames, UpdateSavedState
- auto: A / Physics

## Assembly-CSharp/ModuleSampleCollector.cs
- L=160 kind=PartModule type=ModuleSampleCollector bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(76): IMGUI=16, UnityEngine.UI=2, Input.=1, GetComponent=1, GameObject=1 | ksp: KSPField=5, KSPEvent=3, PartModule=1, HighLogic=1 | fan_in=0
- methods(6): Collect, Discard, OnGUI, OnStart, Transfer, drawTransferDialog
- auto: C / Science

## Assembly-CSharp/ModuleSampleContainer.cs
- L=40 kind=PartModule type=ModuleSampleContainer bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: KSPField=3, PartModule=1, KSPEvent=1 | fan_in=1
- methods(3): Discard, OnStart, TransferSample
- auto: A / Science

## Assembly-CSharp/ModuleSAS.cs
- L=208 kind=PartModule type=ModuleSAS bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(29): Lifecycle=2, GetComponent=1 | ksp: KSPField=6, Localizer=5, HighLogic=2, PartModule=1 | fan_in=2
- methods(10): ApplySASServiceLevelAdjustments, CacheLocalStrings, FixedUpdate, GetInfo, GetModuleDisplayName, GetValue, OnDestroy, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStart
- auto: A / PartModules-Control

## Assembly-CSharp/ModuleScienceContainer.cs
- L=623 kind=PartModule type=ModuleScienceContainer bases=PartModule,IScienceDataContainer anc=PartModule/MonoBehaviour ns=-
- unity(18): Lifecycle=1, Coroutine=1 | ksp: Localizer=24, KSPField=15, KSPEvent=5, FlightGlobals=5, ConfigNode=5 | fan_in=7
- methods(34): AddData, CollectAllAction, CollectAllEvent, CollectDataExternalEvent, DumpData, GetActiveVesselDataCount, GetData, GetModuleDisplayName, GetScienceCount, GetStoredDataCount, HasData, IsRerunnable, OnDataTransfer, OnDestroy, OnLoad, OnSave, OnStart, RemoveData, ReturnData, ReviewData, ReviewDataEvent, ReviewDataItem, StoreData, StoreDataExternalEvent ...
- auto: A / Science

## Assembly-CSharp/ModuleScienceConverter.cs
- L=185 kind=PartModule type=ModuleScienceConverter bases=BaseConverter anc=BaseConverter/PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: KSPField=9, Localizer=7, ProtoCrewMember=1, FlightGlobals=1 | fan_in=2
- methods(12): CacheLocalStrings, CalculateResearchTime, CalculateScienceAmount, CalculateScienceRate, DataExpectationSummary, GetScientists, IsSituationValid, OnStart, PostProcess, PreProcessing, PrepareRecipe, UpdateDisplayStrings
- auto: A / Science

## Assembly-CSharp/ModuleScienceLab.cs
- L=509 kind=PartModule type=ModuleScienceLab bases=PartModule,IResourceConsumer,IScienceDataContainer anc=PartModule/MonoBehaviour ns=-
- unity(35): Coroutine=10, GameObject=2, Time.=1, MonoBehaviour=1, Lifecycle=1 | ksp: Localizer=19, KSPField=12, ConfigNode=7, ResearchAndDevelopment=5, PartResourceDefinition=4 | fan_in=5
- methods(24): CleanModulesEvent, CleanUpExperiment, CleanUpVesselExperiments, GetConsumedResources, GetInfo, GetModuleDisplayName, IsOperational, IsStorable, OnAwake, OnDestroy, OnLoad, OnSave, OnStart, OnTransmissionComplete, ProcessData, RecoverScienceLabs, StoreData, TransmissionErrorScreenMessage, TransmitScience, gatherProcessResources, lockdownAndGatherProcessResources, onPartActionUI, sendDataToComms, updateModuleUI
- auto: A / Science

## Assembly-CSharp/ModuleSeeThroughObject.cs
- L=145 kind=PartModule type=ModuleSeeThroughObject bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(77): Transform=4, GameObject=4, Shader=3, Renderer=2, Material=2, Camera=2 | ksp: KSPField=8, EditorLogic=2, PartModule=1, HighLogic=1 | fan_in=0
- methods(5): GetCursorProximity, LateUpdate, MouseFadeUpdate, OnStart, SetOpacity
- auto: C / Rendering

## Assembly-CSharp/ModuleServiceModule.cs
- L=388 kind=PartModule type=ModuleServiceModule bases=PartModule,IStageSeparator,IStageSeparatorChild anc=PartModule/MonoBehaviour ns=-
- unity(52): Transform=8, Collider=6, GameObject=5, SetActive=4, Lifecycle=1, GetComponent=1 | ksp: KSPField=9, Part=4, Vessel=2, HighLogic=2, GameEvents=2 | fan_in=1
- methods(27): BuildJettisonPartsList, CheckCutaway, GetInfo, GetModuleDisplayName, GetStageIndex, IsEnginePlate, IsMoving, OnActive, OnDestroy, OnFieldUpdated, OnStart, PartDetaches, PointInBox, ProcessJettisonModules, SetExteriorCollider, SetScalar, SetUIRead, SetUIWrite, Setup, SetupJettisonModules, ShedExternalParts, ToggleCutaway, ToggleShellMesh, ToggleShrouds ...
- auto: B / PartModules-Structural

## Assembly-CSharp/ModuleSpaceObjectAnalysis.cs
- L=35 kind=PartModule type=ModuleSpaceObjectAnalysis bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: PartModule=2, Part=1 | fan_in=2
- methods(1): FindMatchingAnalyzer
- auto: A / Science

## Assembly-CSharp/ModuleSpaceObjectInfo.cs
- L=112 kind=PartModule type=ModuleSpaceObjectInfo bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: KSPField=4, PartModule=1, HighLogic=1 | fan_in=2
- methods(1): SetupSpaceObjectResources
- auto: A / Celestials

## Assembly-CSharp/ModuleSpaceObjectResource.cs
- L=50 kind=PartModule type=ModuleSpaceObjectResource bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(22): none | ksp: KSPField=7, PartResourceDefinition=1, PartModule=1, ConfigNode=1 | fan_in=5
- methods(1): OnLoad
- auto: A / PartModules-Resources

## Assembly-CSharp/ModuleStatusLight.cs
- L=95 kind=PartModule type=ModuleStatusLight bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(76): Light=5, Renderer=2, Material=2, GameObject=2, Shader=1 | ksp: KSPField=5, PartModule=1 | fan_in=1
- methods(3): OnInventoryModeDisable, OnStart, SetStatus
- auto: C / Part

## Assembly-CSharp/ModuleStructuralNode.cs
- L=136 kind=PartModule type=ModuleStructuralNode bases=PartModule,IActivateOnDecouple anc=PartModule/MonoBehaviour ns=-
- unity(41): SetActive=2, GameObject=2, Transform=1, Lifecycle=1 | ksp: KSPField=7, AttachNode=7, PartModule=1, Part=1, HighLogic=1 | fan_in=1
- methods(8): CheckDisplay, CheckDisplayEditor, DecoupleAction, DespawnStructure, LateUpdate, OnStart, SetNodeState, SpawnStructure
- auto: B / PartModules-Structural

## Assembly-CSharp/ModuleStructuralNodeToggle.cs
- L=54 kind=PartModule type=ModuleStructuralNodeToggle bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(28): Lifecycle=1 | ksp: KSPField=4, PartModule=1 | fan_in=0
- methods(3): OnFieldUpdated, Start, UpdateNodes
- auto: A / PartModules-Structural

## Assembly-CSharp/ModuleSurfaceFX.cs
- L=237 kind=PartModule type=ModuleSurfaceFX bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(55): GameObject=5, Raycast/Query=3, Lifecycle=3, GetComponent=2, Transform=1, Physics.=1 | ksp: KSPField=5, FlightGlobals=3, Vector3d=1, PartModule=1, HighLogic=1 | fan_in=1
- methods(6): GetSurfaceFX, OnDestroy, OnDrawGizmos, Start, Update, UpdateSrfFX
- auto: C / Rendering

## Assembly-CSharp/ModuleTestSubject.cs
- L=204 kind=PartModule type=ModuleTestSubject bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(26): Coroutine=2, Lifecycle=1 | ksp: KSPField=6, Localizer=4, GameEvents=4, Contract=3, ConfigNode=2 | fan_in=3
- methods(15): CacheLocalStrings, GetModuleDisplayName, GetTestNotes, IsStageable, OnActive, OnAwake, OnContractStateChange, OnDestroy, OnLoad, OnStart, RunTest, RunTestEvent, SituationAvailable, delayedStart, onPartDie
- auto: A / Science

## Assembly-CSharp/ModuleToggleCrossfeed.cs
- L=209 kind=PartModule type=ModuleToggleCrossfeed bases=PartModule,IToggleCrossfeed anc=PartModule/MonoBehaviour ns=-
- unity(19): Lifecycle=1 | ksp: KSPField=9, HighLogic=5, KSPAction=3, Game=2, ResearchAndDevelopment=1 | fan_in=2
- methods(17): Activate, CrossfeedHasTech, CrossfeedRequiresTech, CrossfeedTech, CrossfeedToggleableEditor, CrossfeedToggleableFlight, Deactivate, DisableAction, EnableAction, GetModuleDisplayName, OnAwake, OnLoad, OnStart, Start, ToggleAction, ToggleEvent, UpdateCrossfeed
- auto: A / PartModules-Structural

## Assembly-CSharp/ModuleTripLogger.cs
- L=216 kind=PartModule type=ModuleTripLogger bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(28): SendMessage/Invoke=3, Lifecycle=1 | ksp: Vessel=12, GameEvents=8, CelestialBody=5, ConfigNode=3, Orbit=2 | fan_in=3
- methods(16): CacheLocalStrings, ForceLogEntry, ForceVesselLog, GetModuleDisplayName, InvokeLogVessel, LoadDeprecated, LogNewSituation, LogSituation, LogVesselSituation, OnAwake, OnDestroy, OnLoad, OnSave, OnStart, OnVesselSOIChanged, OnVesselSituationChanged
- auto: A / Science

## Assembly-CSharp/MonoUtilities.cs
- L=30 kind=MonoBehaviour type=MonoUtilities bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(50): MonoBehaviour=1, GetComponent=1 | ksp: UIPartActionWindow=4, Part=2 | fan_in=12
- methods(2): RefreshContextWindows, RefreshPartContextWindow
- auto: B / Core

## Assembly-CSharp/Mouse.cs
- L=397 kind=MonoBehaviour type=Mouse bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): Input.=22, Time.=7, Raycast/Query=7, Lifecycle=5, Coroutine=4, Screen/Cursor=3 | ksp: Part=7, GameSettings=3, HighLogic=2, FlightGlobals=2, EditorLogic=1 | fan_in=38
- methods(19): Awake, CheckButtons, CheckHoveredPart, ClearMouseState, GetAllMouseButtons, GetAllMouseButtonsDown, GetAllMouseButtonsUp, GetButton, GetButtonDown, GetButtonUp, GetClick, GetDoubleClick, GetDragDelta, GetValidHoverPart, OnDestroy, Start, TapRoutine, Update, WasDragging
- auto: B / Input

## Assembly-CSharp/MouseEventsHandler.cs
- L=3 kind=MonoBehaviour type=MouseEventsHandler bases=MouseEventsHandlerBase anc=MouseEventsHandlerBase/MonoBehaviour ns=-
- unity(15): none | ksp: none | fan_in=0
- methods(0): 
- auto: B / Input

## Assembly-CSharp/MouseEventsHandlerBase.cs
- L=78 kind=MonoBehaviour type=MouseEventsHandlerBase bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(70): MonoBehaviour=4, Transform=3, GameObject=3, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=2
- methods(7): FindHandleUpwards, OnMouseDown, OnMouseDrag, OnMouseEnter, OnMouseExit, OnMouseUp, Start
- auto: B / Input

## Assembly-CSharp/MouseRayEventsHandler.cs
- L=82 kind=MonoBehaviour type=MouseRayEventsHandler bases=MouseEventsHandlerBase anc=MouseEventsHandlerBase/MonoBehaviour ns=-
- unity(75): Input.=7, Time.=3, Lifecycle=2, Raycast/Query=1 | ksp: none | fan_in=2
- methods(3): LateUpdate, OnRayHit, Start
- auto: B / Input

## Assembly-CSharp/MultiModeEngine.cs
- L=415 kind=PartModule type=MultiModeEngine bases=PartModule,IEngineStatus anc=PartModule/MonoBehaviour ns=-
- unity(18): Lifecycle=1 | ksp: KSPField=10, KSPAction=4, GameEvents=4, KSPEvent=3, PartModule=2 | fan_in=4
- methods(17): ActivateAction, DisableAutoSwitch, EnableAutoSwitch, ModeAction, ModeEvent, OnAction, OnAutoSwitch, OnAwake, OnDestroy, OnLoad, OnStart, OnUpdate, SetPrimary, SetSecondary, ShutdownAction, ToggleMode, onVesselModified
- auto: A / PartModules-Engines

## Assembly-CSharp/MultiOptionDialog.cs
- L=112 kind=class type=MultiOptionDialog bases=DialogGUIBase anc=DialogGUIBase ns=-
- unity(33): Lifecycle=3, Transform=1, IMGUI=1, GameObject=1 | ksp: none | fan_in=81
- methods(6): Create, FixedUpdate, LateUpdate, OnRenderObject, Resize, Update
- auto: C / UI

## Assembly-CSharp/MunOrBustLogic.cs
- L=16 kind=MonoBehaviour type=MunOrBustLogic bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(80): Texture=1, Shader=1, MonoBehaviour=1, Material=1, Lifecycle=1 | ksp: GameDatabase=1 | fan_in=0
- methods(1): Start
- auto: B / Tutorials

## Assembly-CSharp/MusicLogic.cs
- L=549 kind=MonoBehaviour type=MusicLogic bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(69): Audio=26, Coroutine=23, Time.=5, SceneManager=5, Transform=2, Lifecycle=2 | ksp: GameSettings=10, GameEvents=8, TimeWarp=5, HighLogic=5, FlightGlobals=4 | fan_in=6
- methods(30): Awake, ConstructionMusic, CreditsMusic, Crossfade, FlightSpaceMusic, GetSettings, HandleEditorSwitch, MenuMusic, MissionBuilderMusic, OnDestroy, OnGameSceneChange, OnLevelLoaded, OnSceneLoaded, PauseWithCrossfade, PlayAmbienceLoop, PlayEditorAmbience, PlayFlight, PlayList, PlaySpaceCenter, PlayTheme, ResetConstValues, ResetFlightValues, ResetSpaceCenterValues, SetVolume ...
- auto: C / Audio

## Assembly-CSharp/NavBallVector.cs
- L=7 kind=enum type=NavBallVector bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / FlightUI

## Assembly-CSharp/NavWaypoint.cs
- L=338 kind=MonoBehaviour type=NavWaypoint bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(80): Transform=17, Material=12, GameObject=11, GetComponent=5, SetActive=4, Renderer=4 | ksp: FlightGlobals=5, HighLogic=4, Vessel=2, Planetarium=2, GameSettings=2 | fan_in=6
- methods(16): Activate, Awake, Clear, Deactivate, DeactivateIfWaypoint, IsUsing, IsVisible, LateUpdate, OnDestroy, OnGameSettingsApplied, SetMaterialColor, SetMaterialTexture, Setup, UpdateShowGhosted, UpdateVisualAppearance, UpdateVisualPosition
- auto: C / Orbit

## Assembly-CSharp/NestedPrefabSpawner.cs
- L=157 kind=MonoBehaviour type=NestedPrefabSpawner bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(83): GameObject=14, Transform=13, LayerMask=8, Instantiate/Destroy=3, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=1
- methods(7): Awake, CleanUp, Despawn, Spawn, SpawnPrefabs, Start, setLayersAndTagsRecursive
- auto: B / Misc

## Assembly-CSharp/NoiseEffect.cs
- L=142 kind=MonoBehaviour type=NoiseEffect bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(86): Shader=7, Material=7, Texture=4, Screen/Cursor=4, Instantiate/Destroy=4, Lifecycle=2 | ksp: none | fan_in=0
- methods(4): OnDisable, OnRenderImage, SanitizeParameters, Start
- auto: C / Rendering

## Assembly-CSharp/NoiseGenerator.cs
- L=190 kind=class type=NoiseGenerator bases=- anc=- ns=-
- unity(2): none | ksp: none | fan_in=1
- methods(9): GetSeed, SetSeed, b, coherentNoise, fastfloor, getDensity, kay, noise, shuffle
- auto: A / Terrain

## Assembly-CSharp/OcclusionCone.cs
- L=121 kind=class type=OcclusionCone bases=- anc=- ns=-
- unity(2): none | ksp: none | fan_in=2
- methods(2): GetShockRadius, Setup
- auto: C / Rendering

## Assembly-CSharp/OcclusionCylinder.cs
- L=26 kind=class type=OcclusionCylinder bases=- anc=- ns=-
- unity(9): none | ksp: none | fan_in=2
- methods(1): Setup
- auto: C / Rendering

## Assembly-CSharp/OcclusionData.cs
- L=319 kind=class type=OcclusionData bases=IComparable<OcclusionData> anc=IComparable ns=-
- unity(15): none | ksp: Vector3d=6, GameSettings=3, Part=1 | fan_in=4
- methods(10): AreaOfIntersection, CompareTo, CreateCornerArray, GetConvectionMultVerts, GetCylinderOcclusion, GetCylinderOcclusionVerts, GetShockStats, RectRectIntersection, Update, sA
- auto: C / Rendering

## Assembly-CSharp/OrbitDriver.cs
- L=363 kind=MonoBehaviour type=OrbitDriver bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(52): Lifecycle=4, Rigidbody=3, Renderer=3, Transform=2, MonoBehaviour=2, Instantiate/Destroy=2 | ksp: CelestialBody=11, Vector3d=7, Planetarium=7, GameEvents=4, Vessel=3 | fan_in=28
- methods(13): Awake, CelestialBodyDelegate, CheckDominantBody, FixedUpdate, OnDestroy, OnRailsSOITransition, RecalculateOrbit, SetOrbitMode, Start, TrackRigidbody, UpdateOrbit, unlockFrameSwitch, updateFromParameters
- auto: B / Orbit

## Assembly-CSharp/OrbitPhysicsManager.cs
- L=353 kind=MonoBehaviour type=OrbitPhysicsManager bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(64): Rigidbody=17, Transform=6, Lifecycle=4, MonoBehaviour=2, Coroutine=2, Instantiate/Destroy=1 | ksp: FlightGlobals=32, Vector3d=7, CelestialBody=7, Vessel=6, TimeWarp=6 | fan_in=2
- methods(14): Awake, CheckReferenceFrame, FixedUpdate, HoldVesselUnpack, LateUpdate, OnDestroy, SetDominantBody, SetReverseOrbit, SetRotatingFrame, Start, ToggleRotatingFrame, checkReferenceFrame, setDominantBody, setRotatingFrame
- auto: B / Orbit

## Assembly-CSharp/OrbitRenderer.cs
- L=123 kind=MonoBehaviour type=OrbitRenderer bases=OrbitRendererBase anc=OrbitRendererBase/MonoBehaviour ns=-
- unity(45): Lifecycle=2, Shader=1, Instantiate/Destroy=1, EventSystems=1 | ksp: MapView=3, GameEvents=2 | fan_in=7
- methods(10): CanDrawAnyIcons, DrawOrbit, GetActive, GetNodeColour, GetOrbitColour, IsDebris, OnDestroy, OnSceneChange, OrbitCast, Start
- auto: B / MapView

## Assembly-CSharp/OrbitRendererData.cs
- L=19 kind=class type=OrbitRendererData bases=- anc=- ns=-
- unity(6): none | ksp: CelestialBody=2 | fan_in=2
- methods(0): 
- auto: A / MapView

## Assembly-CSharp/OrbitSnapshot.cs
- L=93 kind=class type=OrbitSnapshot bases=- anc=- ns=-
- unity(0): none | ksp: Orbit=3, ConfigNode=3, FlightGlobals=2, CelestialBody=1 | fan_in=4
- methods(2): Load, Save
- auto: A / Orbit

## Assembly-CSharp/OrbitTargetRenderer.cs
- L=177 kind=MonoBehaviour type=OrbitTargetRenderer bases=OrbitRenderer anc=OrbitRenderer/OrbitRendererBase/MonoBehaviour ns=-
- unity(53): GameObject=4, Transform=2, GetComponent=2, Time.=1, Shader=1, Renderer=1 | ksp: Vector3d=4, ScaledSpace=4, FlightGlobals=4, OrbitDriver=2, MapView=2 | fan_in=2
- methods(11): ANDNNodes_OnUpdateIcon, AnimateOrbit, CanDrawAnyIcons, CheckVisibility, Cleanup, LockOrbit, Setup, Update, UpdateLocals, ascNode_OnUpdatePosition, descNode_OnUpdatePosition
- auto: B / MapView

## Assembly-CSharp/OrbitUtil.cs
- L=159 kind=class type=OrbitUtil bases=- anc=- ns=-
- unity(0): none | ksp: Orbit=15, Vector3d=13, CelestialBody=13 | fan_in=4
- methods(10): CalculateIntersections, CurrentEjectionAngle, CurrentPhaseAngle, FindCommonAncestor, FindPlanet, GetBodyHierarchy, GetSmaDistance, GetSmaDistanceToSun, GetTransferPhaseAngle, GetTransferTime
- auto: A / Orbit

## Assembly-CSharp/OverlayGenerator.cs
- L=550 kind=MonoBehaviour type=OverlayGenerator bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(43): Texture=11, Lifecycle=2, MonoBehaviour=1, GetComponent=1, GameObject=1 | ksp: PartResourceDefinition=10, MapView=4, CelestialBody=4, FlightGlobals=3, GameEvents=2 | fan_in=8
- methods(24): AverageDots, AverageLine, Awake, ClearDisplay, CreateTexture, FetchNextResource, FetchOverlayColor, GenerateOverlay, GenerateScanPoint, GetColor, GetLerp, GetMaxAbundance, GetMinAbundance, GetPoint, GetTexture, Interpolate, InvertColor, LoadResourceList, OnDestroy, OnEnterMapView, OnExitMapView, OnGameLoaded, OnMapFocusChange, ResetRNG
- auto: C / Misc

## Assembly-CSharp/OverlaySetup.cs
- L=75 kind=MonoBehaviour type=OverlaySetup bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(45): MonoBehaviour=1, Lifecycle=1, GetComponent=1, GameObject=1 | ksp: ConfigNode=2, GameDatabase=1 | fan_in=2
- methods(3): LoadOverlayConfig, OnDestroy, ResetMapConfig
- auto: B / Misc

## Assembly-CSharp/ParseExtensions.cs
- L=430 kind=static class type=ParseExtensions bases=- anc=- ns=-
- unity(12): none | ksp: QuaternionD=7, Vector3d=6 | fan_in=5
- methods(15): ArrayMinSize, ParseArray, TryParseBoolArray, TryParseColor, TryParseColor32, TryParseEnumIntArray, TryParseQuaternion, TryParseQuaternionD, TryParseRect, TryParseVector2, TryParseVector2d, TryParseVector3, TryParseVector3d, TryParseVector4, TryParseVector4d
- auto: A / Core

## Assembly-CSharp/ParserMethod.cs
- L=1 kind=delegate type=ParserMethod bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Modding

## Assembly-CSharp/PartBuoyancy.cs
- L=676 kind=MonoBehaviour type=PartBuoyancy bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(45): Rigidbody=15, SendMessage/Invoke=3, MonoBehaviour=2, Lifecycle=2, GetComponent=2, Instantiate/Destroy=1 | ksp: PhysicsGlobals=55, Vector3d=14, FlightGlobals=13, TimeWarp=8, Part=4 | fan_in=2
- methods(6): FindOverrideCube, FixedUpdate, OnDrawGizmosSelected, ReportSplashDown, Start, UpdateDisplacement
- auto: B / Aero

## Assembly-CSharp/PartCategories.cs
- L=41 kind=enum type=PartCategories bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=9
- methods(0): 
- auto: A / Part

## Assembly-CSharp/PartCrewManifest.cs
- L=274 kind=class type=PartCrewManifest bases=- anc=- ns=-
- unity(1): none | ksp: ProtoCrewMember=14, HighLogic=9, KerbalRoster=4, GameEvents=2, AvailablePart=2 | fan_in=15
- methods(14): AddCrewToSeat, AllSeatsEmpty, AnySeatTaken, AnySeats, CloneOf, Contains, CountCrewNotType, CountCrewType, FromConfigNode, GetCrewSeat, GetPartCrew, NoSeats, RemoveCrew, RemoveCrewFromSeat
- auto: A / Kerbals

## Assembly-CSharp/PartGameObjectInfo.cs
- L=22 kind=class type=PartGameObjectInfo bases=- anc=- ns=-
- unity(29): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Part

## Assembly-CSharp/PartGeometryUtil.cs
- L=251 kind=static class type=PartGeometryUtil bases=- anc=- ns=-
- unity(51): Renderer=8, Collider=4, GetComponent=3, GameObject=3, Transform=2, ParticleSystem=1 | ksp: Part=5 | fan_in=7
- methods(8): FindBoundsCentroid, GetColliderBounds, GetPartColliderBounds, GetPartRendererBound, GetPartRendererBounds, GetRendererBounds, GetRendererBoundsWithoutParticles, MergeBounds
- auto: B / Part

## Assembly-CSharp/PartHeightQuery.cs
- L=14 kind=class type=PartHeightQuery bases=- anc=- ns=-
- unity(0): none | ksp: Part=2 | fan_in=3
- methods(0): 
- auto: A / Part

## Assembly-CSharp/ParticleBillboardModifier.cs
- L=24 kind=MonoBehaviour type=ParticleBillboardModifier bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(55): MonoBehaviour=1, Mesh=1 | ksp: none | fan_in=0
- methods(3): ExploreSystemProperties, ReplaceHorizontalBillboardsWithMesh, Test
- auto: C / Rendering

## Assembly-CSharp/ParticleColliderLaunchSequence.cs
- L=97 kind=MonoBehaviour type=ParticleColliderLaunchSequence bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(68): Coroutine=4, Transform=2, Collider=2, Time.=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(4): GoForLaunch, LaunchSequence, Reset, Start
- auto: C / Rendering

## Assembly-CSharp/ParticleConfigFX.cs
- L=330 kind=MonoBehaviour type=ParticleConfigFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(78): ParticleSystem=28, Transform=5, Material=5, GetComponent=3, Shader=2, GameObject=2 | ksp: ConfigNode=5, GameDatabase=1 | fan_in=0
- methods(8): CreateMaterial, InitializeComponents, InitializeParticleSystem, OnEvent, OnInitialize, OnLoad, OnSave, UpdateComponents
- auto: C / Rendering

## Assembly-CSharp/ParticleSystemExt.cs
- L=16 kind=static class type=ParticleSystemExt bases=- anc=- ns=-
- unity(67): ParticleSystem=5 | ksp: none | fan_in=0
- methods(1): GetParticleBuffer
- auto: C / Rendering

## Assembly-CSharp/PartInfo.cs
- L=17 kind=attribute type=PartInfo bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=18
- methods(0): 
- auto: A / Part

## Assembly-CSharp/PartItemTransfer.cs
- L=198 kind=MonoBehaviour type=PartItemTransfer bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(39): Lifecycle=2, MonoBehaviour=1, KeyCode=1, Instantiate/Destroy=1, Input.=1, GameObject=1 | ksp: Part=17, GameEvents=8, Localizer=6, Vessel=4, InputLockManager=2 | fan_in=8
- methods(18): AfterPartsFound, Dismiss, DismissActive, DismissAlreadyRunning, DismissInterrupted, HookAdditionalEvents, IsSemiValidPart, IsValidPart, LateUpdate, OnDestroy, OnPartSelect, OnSemiValidPartSelect, OnSrcPartSelect, Setup, UnhookAdditionalEvents, onVesselChanged, onVesselModified, onVesselSituationChange
- auto: B / Part

## Assembly-CSharp/PartJoint.cs
- L=500 kind=MonoBehaviour type=PartJoint bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(79): Joint=29, GameObject=14, Rigidbody=13, Transform=9, Instantiate/Destroy=4, GetComponent=2 | ksp: Part=16, PartJoint=11, AttachNode=9, PhysicsGlobals=8, GameEvents=3 | fan_in=10
- methods(17): Create, DestroyJoint, JointToLocalSpaceDir, JointToLocalSpacePos, JointToLocalSpaceRot, LocaltoJointSpaceDir, LocaltoJointSpacePos, LocaltoJointSpaceRot, OnDestroy, OnJointBreak, OnPartPack, OnPartUnpack, SetBreakingForces, SetJointLimits, SetUnbreakable, SetupJoint, create
- auto: B / Part

## Assembly-CSharp/PartMaterialInfo.cs
- L=122 kind=class type=PartMaterialInfo bases=IConfigNode anc=IConfigNode ns=-
- unity(55): Shader=6, Texture=4, Material=2 | ksp: ConfigNode=3, GameDatabase=2 | fan_in=1
- methods(4): ApplyCommand, ApplyCommands, Load, Save
- auto: C / Part

## Assembly-CSharp/PartModuleFXSetter.cs
- L=48 kind=PartModule type=PartModuleFXSetter bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(15): none | ksp: PartModule=1, KSPField=1 | fan_in=2
- methods(3): OnAwake, OnStart, SetFXModules
- auto: C / Rendering

## Assembly-CSharp/PartModuleList.cs
- L=438 kind=class type=PartModuleList bases=IEnumerable anc=IEnumerable ns=-
- unity(10): GameObject=2, Instantiate/Destroy=1, GetComponent=1 | ksp: PartModule=23, Part=2 | fan_in=1
- methods(14): Add, Clear, Contains, FindEngineInterface, FindEngineNearby, GetEnumerator, GetModule, GetModules, GetScalarModule, GetScalarModules, IndexOf, Remove, Send, SendDefault
- auto: B / Part

## Assembly-CSharp/PartModuleUtil.cs
- L=60 kind=static class type=PartModuleUtil bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(1): PrintResourceRequirements
- auto: A / Part

## Assembly-CSharp/PartPointer.cs
- L=19 kind=MonoBehaviour type=PartPointer bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(44): MonoBehaviour=1 | ksp: Part=4 | fan_in=3
- methods(2): GetPart, SetPart
- auto: B / Part

## Assembly-CSharp/PartRawInfo.cs
- L=22 kind=class type=PartRawInfo bases=- anc=- ns=-
- unity(29): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Part

## Assembly-CSharp/PartResource.cs
- L=231 kind=class type=PartResource bases=- anc=- ns=-
- unity(2): none | ksp: PartResource=10, GameEvents=4, Part=3, PartResourceDefinition=2, ConfigNode=2 | fan_in=34
- methods(7): CanProvide, Copy, Flowing, GetInfo, Load, Save, SetInfo
- auto: A / Resources

## Assembly-CSharp/PartResourceDefinition.cs
- L=217 kind=class type=PartResourceDefinition bases=- anc=- ns=-
- unity(22): none | ksp: ConfigNode=9, PartResourceDefinition=4, Localizer=4 | fan_in=63
- methods(4): GetFlowModeDescription, GetShortName, Load, Save
- auto: A / Resources

## Assembly-CSharp/PartResourceDefinitionList.cs
- L=93 kind=class type=PartResourceDefinitionList bases=IEnumerable anc=IEnumerable ns=-
- unity(18): none | ksp: PartResourceDefinition=10, ConfigNode=1 | fan_in=1
- methods(4): Add, Clear, Contains, GetEnumerator
- auto: A / Resources

## Assembly-CSharp/PartResourceDrainDefinition.cs
- L=77 kind=class type=PartResourceDrainDefinition bases=- anc=- ns=-
- unity(24): none | ksp: ConfigNode=2 | fan_in=1
- methods(2): Load, Save
- auto: A / Resources

## Assembly-CSharp/PartResourceLibrary.cs
- L=90 kind=MonoBehaviour type=PartResourceLibrary bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(66): Transform=2, Lifecycle=2, Instantiate/Destroy=2, GameObject=2, MonoBehaviour=1 | ksp: PartResourceDefinition=3, GameDatabase=1, ConfigNode=1 | fan_in=55
- methods(6): Awake, GetDefaultFlowMode, GetDefinition, LoadDefinitions, OnDestroy, Reset
- auto: B / Resources

## Assembly-CSharp/PartResourceList.cs
- L=352 kind=class type=PartResourceList bases=IEnumerable<PartResource>,IEnumerable anc=IEnumerable ns=-
- unity(9): none | ksp: PartResource=36, Part=7, GameEvents=6, PartResourceDefinition=2, ConfigNode=1 | fan_in=3
- methods(15): Add, Clear, Contains, Get, GetAll, GetAllFlowing, GetEnumerator, GetFlowing, GetFlowingTotals, HasFlowable, HasFlowableUnhidden, Initializer, RefListInitializer, RefreshSimulationListAmounts, Remove
- auto: A / Resources

## Assembly-CSharp/PartSelector.cs
- L=98 kind=MonoBehaviour type=PartSelector bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(58): Lifecycle=2, GetComponent=2, GameObject=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: Part=9 | fan_in=2
- methods(7): Create, Dismiss, LateUpdate, OnDestroy, OnMouseEntered, OnMouseExited, Setup
- auto: B / Part

## Assembly-CSharp/PartSpaceMode.cs
- L=5 kind=enum type=PartSpaceMode bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=5
- methods(0): 
- auto: A / Part

## Assembly-CSharp/PartStates.cs
- L=10 kind=enum type=PartStates bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=34
- methods(0): 
- auto: A / Part

## Assembly-CSharp/PartStatsUpgradeModule.cs
- L=146 kind=PartModule type=PartStatsUpgradeModule bases=PartModule,IModuleInfo,IPartMassModifier anc=PartModule/MonoBehaviour ns=-
- unity(18): none | ksp: ConfigNode=9, Localizer=5, PartModule=1, PartLoader=1 | fan_in=0
- methods(10): GetDrawModulePanelCallback, GetInfo, GetModuleCost, GetModuleCostChangeWhen, GetModuleDisplayName, GetModuleMass, GetModuleMassChangeWhen, GetModuleTitle, GetPrimaryField, OnLoad
- auto: A / Modding

## Assembly-CSharp/PartTestConstraint.cs
- L=322 kind=class type=PartTestConstraint bases=IConfigNode anc=IConfigNode ns=-
- unity(5): none | ksp: Contract=4, ConfigNode=3, Vessel=2, Planetarium=2, CelestialBody=1 | fan_in=4
- methods(6): BodyAvailable, Load, PrestigeAvailable, Save, SituationAvailable, Test
- auto: A / Science

## Assembly-CSharp/PartThermalData.cs
- L=170 kind=class type=PartThermalData bases=- anc=- ns=-
- unity(11): none | ksp: GameSettings=4, Part=2, PartModule=1, KerbalEVA=1 | fan_in=5
- methods(1): GetUnifiedSkinTemp
- auto: A / Thermal

## Assembly-CSharp/PartThermalDataIntComparer.cs
- L=17 kind=class type=PartThermalDataIntComparer bases=IComparer<PartThermalData> anc=IComparer ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(1): Compare
- auto: A / Thermal

## Assembly-CSharp/PartThermalDataSkinComparer.cs
- L=17 kind=class type=PartThermalDataSkinComparer bases=IComparer<PartThermalData> anc=IComparer ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(1): Compare
- auto: A / Thermal

## Assembly-CSharp/PartTools.cs
- L=113 kind=MonoBehaviour type=PartTools bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(92): Shader=26, Texture=3, MonoBehaviour=1, Material=1 | ksp: none | fan_in=1
- methods(2): Reset, SetDefaultShaders
- auto: C / Part

## Assembly-CSharp/PartToolsLibrary.cs
- L=24 kind=MonoBehaviour type=PartToolsLibrary bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(83): Texture=2, SceneManager=1, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=0
- methods(1): Awake
- auto: B / Part

## Assembly-CSharp/PartUpgradeHandler.cs
- L=468 kind=class type=PartUpgradeHandler bases=IEnumerable anc=IEnumerable ns=-
- unity(1): none | ksp: HighLogic=30, Game=14, ConfigNode=10, PartModule=7, Part=5 | fan_in=10
- methods(23): AddUpgrade, AddUsedBy, CanHaveUpgrades, FillUpgrades, GetEnumerator, GetUpgrade, GetUpgradeCost, GetUpgradesForTech, GetUsedBy, GetUsedByStrings, IsAvailableToUnlock, IsEnabled, IsUnlocked, IsUsed, LinkUpgrades, OnLoad, OnSave, RemoveUpgrade, SetEnabled, SetFromInfo, SetFromUntrackedUpgrade, SetUnlocked, UgpradesAllowed
- auto: A / Modding

## Assembly-CSharp/PartUpgradeManager.cs
- L=59 kind=ScenarioModule type=PartUpgradeManager bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(36): Lifecycle=1, Instantiate/Destroy=1 | ksp: ConfigNode=2, ScenarioModule=1 | fan_in=7
- methods(4): OnAwake, OnDestroy, OnLoad, OnSave
- auto: A / Modding

## Assembly-CSharp/PartValues.cs
- L=32 kind=class type=PartValues bases=EventValueWrapper anc=EventValueWrapper ns=-
- unity(0): none | ksp: none | fan_in=16
- methods(0): 
- auto: A / Part

## Assembly-CSharp/PartVariant.cs
- L=592 kind=class type=PartVariant bases=IConfigNode anc=IConfigNode ns=-
- unity(42): Material=15, Transform=11, Shader=5, SetActive=1, GetComponent=1, GameObject=1 | ksp: AttachNode=18, ConfigNode=10, PartLoader=1, HighLogic=1, GameDatabase=1 | fan_in=17
- methods(11): FindChild, GetAttachedNodeIndex, GetExtraInfoValue, Load, MaterialExists, ParseNode, Save, TryCopyMaterial, UpdateMaterial, UpdateMaterialFromExtraInfo, UpdateModel
- auto: C / Part

## Assembly-CSharp/PAsteroid.cs
- L=60 kind=MonoBehaviour type=PAsteroid bases=PSpaceObject anc=PSpaceObject/MonoBehaviour ns=-
- unity(61): GetComponent=2, GameObject=2, Transform=1, Renderer=1 | ksp: Part=1 | fan_in=4
- methods(1): SetupPartParameters
- auto: B / Celestials

## Assembly-CSharp/PatchedConicRenderer.cs
- L=410 kind=MonoBehaviour type=PatchedConicRenderer bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(40): Transform=4, Lifecycle=4, Shader=3, Time.=2, MonoBehaviour=1, GetComponent=1 | ksp: MapView=14, GameEvents=6, Vector3d=4, Planetarium=3, Localizer=3 | fan_in=7
- methods(15): AddManeuverNode, Awake, CanDrawAnyNode, FindRenderingForPatch, GetPatchColor, LateUpdate, ManeuverUINode_OnClick, ManeuverUINode_OnUpdateCaption, ManeuverUINode_OnUpdatePosition, ManeuverUINode_OnUpdateVisible, OnDestroy, OnGameSettingsApplied, OnSceneSwitch, SetMouseOverGizmo, Start
- auto: B / MapView

## Assembly-CSharp/PatchedConics.cs
- L=472 kind=class type=PatchedConics bases=- anc=- ns=-
- unity(25): Camera=6, Raycast/Query=1, Input.=1 | ksp: Orbit=38, ScaledSpace=12, Vector3d=9, OrbitDriver=5, CelestialBody=4 | fan_in=8
- methods(20): AngleWrap, CalculatePatchDelegate, CheckEncounterDelegate, EncountersBodyDelegate, GetClosestApproachDelegate, GetScreenSpacePoint, GetUpdatedOrbitPoint, GetUpdatedOrigin, ScreenCastDelegate, ScreenCastWorkerDelegate, SolveDynamicTA_BSP, SolveLocalTA_BSP, SolveRelativeTA_BSP, TAIsWithinPatchBounds, _CalculatePatch, _CheckEncounter, _EncountersBody, _GetClosestApproach, _ScreenCast, _ScreenCastWorker
- auto: B / Orbit

## Assembly-CSharp/PatchedConicSolver.cs
- L=377 kind=MonoBehaviour type=PatchedConicSolver bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(31): Lifecycle=3, Time.=1, MonoBehaviour=1, GetComponent=1, GameObject=1, Coroutine=1 | ksp: Orbit=20, OrbitDriver=10, Planetarium=6, GameEvents=6, Vector3d=5 | fan_in=12
- methods(17): AddManeuverNode, Awake, CheckNextManeuver, DecreasePatchLimit, FindManeuverIndex, FindPatchContainingUT, IncreasePatchLimit, Load, OnDestroy, OnGameSettingsApplied, OnReferenceBodyChange, RemoveManeuverNode, Save, SortNodesByDate, Start, Update, UpdateFlightPlan
- auto: B / Orbit

## Assembly-CSharp/PatchRendering.cs
- L=682 kind=class type=PatchRendering bases=- anc=- ns=-
- unity(14): Material=3, Shader=2, LayerMask=1, Instantiate/Destroy=1, GameObject=1 | ksp: Orbit=26, Vector3d=10, ScaledSpace=10, Localizer=7, Planetarium=5 | fan_in=4
- methods(29): AttachUINodes, CanDrawAnyNode, CreateMapObjects, DestroyMapObjects, DestroyUINodes, DestroyVector, DetachUINodes, DrawSplines, GetScaledSpacePointFromTA, MakeVector, SetColor, SetColorSegments, Terminate, UpdatePR, UpdateSpline, UpdateUINodes, mnAp_OnUpdateCaption, mnAp_OnUpdateVisible, mnCB_OnUpdateCaption, mnCB_OnUpdateVisible, mnCBatUT_OnUpdatePosition, mnCBatUT_OnUpdateVisible, mnEnd_OnUpdateCaption, mnEnd_OnUpdateType ...
- auto: B / MapView

## Assembly-CSharp/PauseAudioFadeHandler.cs
- L=79 kind=MonoBehaviour type=PauseAudioFadeHandler bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(78): Coroutine=7, Audio=5, Time.=2, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(3): PauseWithFade, PauseWithFadeRoutine, UnpauseWithFade
- auto: C / Audio

## Assembly-CSharp/PauseMenu.cs
- L=756 kind=MonoBehaviour type=PauseMenu bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): GameObject=33, SetActive=30, Lifecycle=4, Analytics=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: HighLogic=93, Localizer=61, MissionSystem=15, GameEvents=11, Game=8 | fan_in=4
- methods(21): Awake, BuildButtonList, CancelExitToMainMenuConfirmation, Close, Display, MissionModeValid, OnDestroy, ShowExitToMainConfirmation, Start, SubmitExitToMainMenuConfirmation, Update, draw, drawExitScenarioOptions, drawExitTestMissionOptions, drawExitWithoutSaveOptions, drawMissionRestartOptions, drawMissionsRevertOptions, drawRevertOptions, drawStockRevertOptions, miniSettingsFinished, saveAndExit
- auto: B / Scene

## Assembly-CSharp/PComet.cs
- L=60 kind=MonoBehaviour type=PComet bases=PSpaceObject anc=PSpaceObject/MonoBehaviour ns=-
- unity(61): GetComponent=2, GameObject=2, Transform=1, Renderer=1 | ksp: Part=1 | fan_in=3
- methods(1): SetupPartParameters
- auto: B / Celestials

## Assembly-CSharp/PDebug.cs
- L=62 kind=static class type=PDebug bases=- anc=- ns=-
- unity(21): none | ksp: PartLoader=1, GameSettings=1, GameDatabase=1, ConfigNode=1 | fan_in=17
- methods(4): Error, General, Log, Warning
- auto: C / Debug

## Assembly-CSharp/Performance.cs
- L=219 kind=MonoBehaviour type=Performance bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(45): Time.=5, Lifecycle=4, Resources.=1, MonoBehaviour=1 | ksp: none | fan_in=3
- methods(7): Awake, GetFramePerSecondPoints, GetFrameTimePoints, OnDestroy, Start, Update, UpdateFrameTimes
- auto: C / Debug

## Assembly-CSharp/Persistent.cs
- L=27 kind=attribute type=Persistent bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=20
- methods(0): 
- auto: A / Save

## Assembly-CSharp/PersistentLinkable.cs
- L=6 kind=attribute type=PersistentLinkable bases=Attribute anc=Attribute ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Save

## Assembly-CSharp/PersistentRelation.cs
- L=6 kind=enum type=PersistentRelation bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Save

## Assembly-CSharp/physicalObject.cs
- L=98 kind=MonoBehaviour type=physicalObject bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(95): GameObject=8, Rigidbody=7, Renderer=5, GetComponent=5, Transform=3, Lifecycle=3 | ksp: FlightGlobals=4, TimeWarp=3, Part=1 | fan_in=0
- methods(6): Awake, CleanupMaterials, ConvertToPhysicalObject, OnDestroy, Start, Update
- auto: B / Physics

## Assembly-CSharp/PhysicMaterialColor.cs
- L=31 kind=class type=PhysicMaterialColor bases=- anc=- ns=-
- unity(33): none | ksp: ConfigNode=1 | fan_in=1
- methods(1): Load
- auto: A / Physics

## Assembly-CSharp/PhysicMaterialDefinition.cs
- L=124 kind=class type=PhysicMaterialDefinition bases=- anc=- ns=-
- unity(61): PhysicMaterial=12, Material=1 | ksp: ConfigNode=5 | fan_in=2
- methods(2): Load, Save
- auto: B / Physics

## Assembly-CSharp/PhysicMaterialLibrary.cs
- L=115 kind=MonoBehaviour type=PhysicMaterialLibrary bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(80): PhysicMaterial=6, Material=4, Transform=2, Lifecycle=2, Instantiate/Destroy=2, GameObject=2 | ksp: ConfigNode=2, GameDatabase=1 | fan_in=2
- methods(6): Awake, CreateDefaultPhysicMaterial, CreatePhysicMaterials, LoadDefinitions, OnDestroy, Reset
- auto: B / Physics

## Assembly-CSharp/PhysicsUtil.cs
- L=120 kind=class type=PhysicsUtil bases=- anc=- ns=-
- unity(78): Raycast/Query=17, Transform=14, Physics.=4, Collider=4 | ksp: none | fan_in=2
- methods(7): CapsuleCastAllIgnoreSelf, HasAncestorTransform, RaycastAllIgnoreSelf, RaycastHitDistCompare, SphereCastAllIgnoreSelf, SphereSweepTest, SphereSweepTestWhere
- auto: B / Physics

## Assembly-CSharp/PIDclamp.cs
- L=134 kind=class type=PIDclamp bases=IPid anc=IPid ns=-
- unity(3): none | ksp: none | fan_in=1
- methods(8): ClampIntegral, Parse, Reinitialize, Reset, ResetIntegral, ToString, Update, double
- auto: A / Core

## Assembly-CSharp/PieChart.cs
- L=327 kind=MonoBehaviour type=PieChart bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(69): Material=9, Transform=5, Mesh=4, GetComponent=4, GameObject=4, Instantiate/Destroy=3 | ksp: none | fan_in=2
- methods(15): AddOnExit, AddOnOver, AddOnTap, CreateDirection, CreateEdges, CreateEnds, CreateTopBottom, OnInputDelegate, RemoveOnExit, RemoveOnOver, RemoveOnTap, Reset, SetSlices, Start, UpdateChart
- auto: C / UI

## Assembly-CSharp/PieChartSlice.cs
- L=31 kind=MonoBehaviour type=PieChartSlice bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(87): EventSystems=6, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(3): OnPointerClick, OnPointerEnter, OnPointerExit
- auto: C / UI

## Assembly-CSharp/Planetarium.cs
- L=314 kind=MonoBehaviour type=Planetarium bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(26): Lifecycle=4, MonoBehaviour=1 | ksp: Vector3d=22, CelestialBody=10, QuaternionD=5, HighLogic=3, Planetarium=2 | fan_in=188
- methods(18): Awake, FindRootBody, FixedUpdate, FrameIsRotating, GetUniversalTime, LocalToWorld, OnDestroy, OrbitalFrame, PlanetaryFrame, SetFrame, SetUniversalTime, SphericalVector, Update, UpdateCBs, UpdateCBsRecursive, WorldToLocal, ZupAtT, findRotatingBodiesRecursive
- auto: B / Celestials

## Assembly-CSharp/PlanetariumCamera.cs
- L=585 kind=MonoBehaviour type=PlanetariumCamera bases=MonoBehaviour,IKSPCamera anc=MonoBehaviour ns=-
- unity(73): Transform=32, Input.=8, GetComponent=7, GameObject=7, Camera=7, Time.=6 | ksp: GameSettings=15, ScaledSpace=9, GameEvents=8, Planetarium=6, HighLogic=6 | fan_in=23
- methods(32): Activate, AddTarget, Awake, CreatePivot, Deactivate, FindNearestTarget, GetCameraTransform, GetPivot, GetTarget, GetTargetIndex, LateUpdate, OnDestroy, OnLevelLoaded, OnNavigatorHandoff, OnNavigatorRequestControl, OnNavigatorTakeOver, OnSceneChange, OnSceneLoaded, OnVesselDestroy, RemoveTarget, ResetPivot, SetCamCoordsFromPosition, SetDistance, SetTarget ...
- auto: C / Camera

## Assembly-CSharp/PlanetaryResource.cs
- L=8 kind=class type=PlanetaryResource bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / Resources

## Assembly-CSharp/PlanetarySample.cs
- L=6 kind=class type=PlanetarySample bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Resources

## Assembly-CSharp/PlanetScanData.cs
- L=4 kind=class type=PlanetScanData bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / Resources

## Assembly-CSharp/PopupDialog.cs
- L=284 kind=MonoBehaviour type=PopupDialog bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(91): GameObject=29, UnityEngine.UI=17, GetComponent=16, Transform=13, Instantiate/Destroy=6, SetActive=5 | ksp: HighLogic=1 | fan_in=71
- methods(15): CheckForOpenDialogs, ClearPopUps, Contains, Dismiss, DismissPopup, FixedUpdate, LateUpdate, OnDestroy, OnRectTransformDimensionsChange, OnRenderObject, SetDraggable, SetPopupData, SpawnPopupDialog, Start, Update
- auto: C / UI

## Assembly-CSharp/PopupDialogController.cs
- L=27 kind=MonoBehaviour type=PopupDialogController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): Transform=2, Lifecycle=2, MonoBehaviour=1 | ksp: none | fan_in=6
- methods(2): Awake, OnDestroy
- auto: C / UI

## Assembly-CSharp/PositionTarget.cs
- L=101 kind=class type=PositionTarget bases=ITargetable anc=ITargetable ns=-
- unity(34): Transform=3, GameObject=2, Instantiate/Destroy=1 | ksp: Vector3d=2, CelestialBody=2, Vessel=1, OrbitDriver=1, Orbit=1 | fan_in=1
- methods(12): GetActiveTargetable, GetDisplayName, GetFwdVector, GetName, GetObtVelocity, GetOrbit, GetOrbitDriver, GetSrfVelocity, GetTargetingMode, GetTransform, GetVessel, Update
- auto: B / Orbit

## Assembly-CSharp/PQS_Collider.cs
- L=103 kind=MonoBehaviour type=PQS_Collider bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(79): Transform=9, GameObject=5, SetActive=3, Lifecycle=2, MonoBehaviour=1, GetComponent=1 | ksp: PQS=2 | fan_in=0
- methods(7): ActivateCollider, DeactivateCollider, FixedUpdate, Reset, SetTargetSphere, Start, UpdateCollider
- auto: B / Terrain

## Assembly-CSharp/PQS_ColliderAdvanced.cs
- L=452 kind=MonoBehaviour type=PQS_ColliderAdvanced bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(86): Transform=32, Collider=19, Physics.=18, GameObject=18, GetComponent=17, Instantiate/Destroy=4 | ksp: Vector3d=21, QuaternionD=5, PQS=2, CelestialBody=1 | fan_in=0
- methods(20): ActivateGravityCollider, ActivateVelocityCollider, DeactivateGravityCollider, DeactivateVelocityCollider, FixedUpdate, GetNormalAtPoint, IterateVelocity, OnDestroy, OnReferenceBodyChange, RegisterColliders, Reset, SetTargetSphere, ShowDebugNormals, Start, UnregisterColliders, UpdateColliders, UpdateGravityCollider, UpdateNormalCollider, UpdateVelocityCollider, UpdateVerticalCollider
- auto: B / Terrain

## Assembly-CSharp/PQS_ColliderRigidbodyTest.cs
- L=59 kind=MonoBehaviour type=PQS_ColliderRigidbodyTest bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(92): Transform=8, Lifecycle=3, Input.=3, Time.=2, Rigidbody=2, MonoBehaviour=1 | ksp: PhysicsGlobals=1, PQS=1 | fan_in=0
- methods(4): FixedUpdate, Reset, Start, Update
- auto: B / Terrain

## Assembly-CSharp/PQS_GameBindings.cs
- L=63 kind=class type=PQS_GameBindings bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(5): DispatchOnPQSCityLoaded, DispatchOnPQSCityUnloaded, GetPOIRange, GetPQSCityLoadRange, TerrainPresetListIsCompatible
- auto: A / Terrain

## Assembly-CSharp/PQS_KSPBinder.cs
- L=105 kind=MonoBehaviour type=PQS_KSPBinder bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(48): GameObject=3, Lifecycle=2, MonoBehaviour=1 | ksp: CelestialBody=6, PhysicsGlobals=4, GameSettings=4, PQS=2, GameEvents=2 | fan_in=0
- methods(13): Awake, GetCBForPQS, GetPQSCityLoadRange, OnDestroy, OnGetPOIRange, OnPQSCityLoaded, OnPQSCityUnloaded, pqsBindings_GetLoadedSceneIsGame, pqsBindings_GetPlanetForceShaderModel20, pqsBindings_GetPlanetScatterFactor, pqsBindings_GetPresetListCompatible, pqsBindings_GetSettingsReady, pqsBindings_GetUsePlanetScatter
- auto: B / Terrain

## Assembly-CSharp/PQS_PartCollider.cs
- L=473 kind=MonoBehaviour type=PQS_PartCollider bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(80): Collider=19, Physics.=18, Transform=17, GameObject=15, GetComponent=6, Lifecycle=4 | ksp: Vector3d=20, QuaternionD=5, PQS=2, Part=1, CelestialBody=1 | fan_in=1
- methods(20): ActivateGravityCollider, ActivateVelocityCollider, Awake, DeactivateGravityCollider, DeactivateVelocityCollider, FixedUpdate, GetNormalAtPoint, IterateVelocity, OnDestroy, OnReferenceBodyChange, RegisterColliders, Reset, SetTargetSphere, Start, UnregisterColliders, UpdateColliders, UpdateGravityCollider, UpdateNormalCollider, UpdateVelocityCollider, UpdateVerticalCollider
- auto: B / Terrain

## Assembly-CSharp/PQSCache.cs
- L=495 kind=MonoBehaviour type=PQSCache bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(58): GameObject=14, Transform=9, GetComponent=4, SetActive=2, MonoBehaviour=2, Mesh=2 | ksp: PQS=16, ConfigNode=11 | fan_in=6
- methods(17): Awake, CoroutineCreatePQCache, CreateCache, CreateDefaultPresetList, DestroyQuad, GetPreset, GetQuad, IncreasePQCache, Load, LoadPresetList, OnDestroy, OnEvent, Reset, ResetCache, Save, SavePresetList, SetPreset
- auto: B / Terrain

## Assembly-CSharp/PQSCity.cs
- L=413 kind=PQSMod type=PQSCity bases=PQSSurfaceObject anc=PQSSurfaceObject/PQSMod/MonoBehaviour ns=-
- unity(79): GameObject=37, Transform=28, SetActive=9, Renderer=4, GetComponent=3, Coroutine=2 | ksp: GameEvents=6, Vector3d=5, CelestialBody=4, Planetarium=3, PQS=2 | fan_in=7
- methods(20): CheckLocals, GetPQSCitySeed, GetStableHashCode, OnPostSetup, OnSetup, OnSphereActive, OnSphereInactive, OnSphereReset, OnSphereStart, OnUpdateFinished, Orientate, OrientateToOrigin, OrientateToWorld, PostSetup, Randomize, Reset, ResetCelestialBody, SetActive, Setup, Start
- auto: B / Terrain

## Assembly-CSharp/PQSCity2.cs
- L=513 kind=PQSMod type=PQSCity2 bases=PQSSurfaceObject anc=PQSSurfaceObject/PQSMod/MonoBehaviour ns=-
- unity(64): Transform=27, GameObject=7, SetActive=5, GetComponent=4, Raycast/Query=3, Lifecycle=3 | ksp: Vector3d=9, FloatingOrigin=8, QuaternionD=6, HighLogic=5, GameEvents=5 | fan_in=6
- methods(21): Awake, OnDestroy, OnPostSetup, OnScenerySettingChanged, OnSetup, OnSphereActive, OnSphereInactive, OnSphereReset, OnSphereStart, OnUpdateFinished, Orientate, OrientateOnWater, OrientateRayCast, Reset, SetActive, SetBody, SetInactive, SetRange, Setup, Start, setVisibility
- auto: B / Terrain

## Assembly-CSharp/PQSGlobeDisplay.cs
- L=149 kind=MonoBehaviour type=PQSGlobeDisplay bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(80): Mesh=6, GetComponent=5, Renderer=3, Material=3, Instantiate/Destroy=2, GameObject=2 | ksp: PQS=3 | fan_in=0
- methods(8): CreateGlobe, CreateLatitudeLine, CreateLatitudeLines, CreateLongitudeCircle, CreateLongitudeLines, OnDestroy, Reset, Start
- auto: B / Terrain

## Assembly-CSharp/PQSMeshPlanet.cs
- L=587 kind=MonoBehaviour type=PQSMeshPlanet bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(40): Time.=4, Mesh=4, GetComponent=2, GameObject=2, Coroutine=2, Renderer=1 | ksp: Vector3d=6, PQS=1 | fan_in=0
- methods(22): AddTriangle, Awake, CreateBox, CreateIcosahedron, CreateMesh, CreateMeshObject, CreatePlanetMesh, GetDoubleVert, GetEnumerator, GetVertex, HasVertex, RemoveVertex, ReplaceEdge, Reset, SetDoubleVert, SetEdges, SetVerts, SetupLists, Split, SplitWrap, Subdivide, Wrap
- auto: B / Terrain

## Assembly-CSharp/PQSMeshWrapper.cs
- L=155 kind=MonoBehaviour type=PQSMeshWrapper bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(54): Mesh=5, MonoBehaviour=1 | ksp: Vector3d=6, PQS=1 | fan_in=0
- methods(3): CreateTangents, CreateWrappedMesh, CreateWrappedMesh2
- auto: B / Terrain

## Assembly-CSharp/PQSMod.cs
- L=120 kind=MonoBehaviour type=PQSMod bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(24): MonoBehaviour=1 | ksp: PQS=5 | fan_in=70
- methods(22): GetVertexMaxHeight, GetVertexMinHeight, OnMeshBuild, OnPostSetup, OnPreUpdate, OnQuadBuilt, OnQuadCreate, OnQuadDestroy, OnQuadPreBuild, OnQuadUpdate, OnQuadUpdateNormals, OnSetup, OnSphereActive, OnSphereInactive, OnSphereReset, OnSphereStart, OnSphereStarted, OnSphereTransformUpdate, OnUpdateFinished, OnVertexBuild, OnVertexBuildHeight, RebuildSphere
- auto: B / Terrain

## Assembly-CSharp/PQSMod_AerialPerspectiveMaterial.cs
- L=55 kind=PQSMod type=PQSMod_AerialPerspectiveMaterial bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(79): Shader=8 | ksp: none | fan_in=1
- methods(3): OnSetup, OnSphereActive, OnUpdateFinished
- auto: B / Terrain

## Assembly-CSharp/PQSMod_AltitudeAlpha.cs
- L=34 kind=PQSMod type=PQSMod_AltitudeAlpha bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=2 | fan_in=0
- methods(3): OnSetup, OnVertexBuild, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_AltitudeUV.cs
- L=46 kind=PQSMod type=PQSMod_AltitudeUV bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=2 | fan_in=1
- methods(3): OnSetup, OnVertexBuild, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_BillboardObject.cs
- L=16 kind=PQSMod type=PQSMod_BillboardObject bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(61): Transform=2, Lifecycle=1 | ksp: Vector3d=1 | fan_in=0
- methods(1): FixedUpdate
- auto: A / Terrain

## Assembly-CSharp/PQSMod_CelestialBodyTransform.cs
- L=355 kind=PQSMod type=PQSMod_CelestialBodyTransform bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(59): Shader=8, Renderer=5, Transform=4, GameObject=4, Material=2, GetComponent=2 | ksp: FlightGlobals=9, GameSettings=6, PQS=5, HighLogic=3, MapView=2 | fan_in=2
- methods(11): DoFade, FadeOutSphere, OnPreUpdate, OnSetup, OnSphereInactive, OnSphereReset, OnSphereStart, OnSphereTransformUpdate, OnUpdateFinished, Reset, Setup
- auto: B / Terrain

## Assembly-CSharp/PQSMod_CreateSphereCollider.cs
- L=25 kind=PQSMod type=PQSMod_CreateSphereCollider bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(82): Collider=3, GetComponent=2, GameObject=1 | ksp: none | fan_in=0
- methods(2): OnSetup, Reset
- auto: B / Terrain

## Assembly-CSharp/PQSMod_FlattenArea.cs
- L=158 kind=PQSMod type=PQSMod_FlattenArea bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(38): GetComponent=2, GameObject=2, Transform=1 | ksp: Vector3d=4, Planetarium=3, PQS=3, CelestialBody=1 | fan_in=1
- methods(7): CubicHermite, Lerp, OnQuadBuilt, OnQuadPreBuild, OnSetup, OnVertexBuildHeight, Reset
- auto: B / Terrain

## Assembly-CSharp/PQSMod_FlattenAreaTangential.cs
- L=120 kind=PQSMod type=PQSMod_FlattenAreaTangential bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(21): none | ksp: Vector3d=3, PQS=3 | fan_in=0
- methods(6): CubicHermite, OnQuadBuilt, OnQuadPreBuild, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_FlattenOcean.cs
- L=38 kind=PQSMod type=PQSMod_FlattenOcean bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=2 | fan_in=0
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_GnomonicTest.cs
- L=198 kind=PQSMod type=PQSMod_GnomonicTest bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(32): Lifecycle=1, KeyCode=1, Input.=1 | ksp: PQS=22, Vector3d=2 | fan_in=0
- methods(5): GetGnomonicMapCoords, OnSetup, OnVertexBuild, OnVertexBuild2, Update
- auto: A / Terrain

## Assembly-CSharp/PQSMod_HeightColorMap.cs
- L=127 kind=PQSMod type=PQSMod_HeightColorMap bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(29): none | ksp: PQS=2 | fan_in=0
- methods(7): Clamp, CubicHermite, Lerp, OnSetup, OnVertexBuild, Reset, SelectLandClassByHeight
- auto: A / Terrain

## Assembly-CSharp/PQSMod_HeightColorMap2.cs
- L=139 kind=PQSMod type=PQSMod_HeightColorMap2 bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(28): none | ksp: PQS=2 | fan_in=0
- methods(7): Clamp, CubicHermite, Lerp, OnSetup, OnVertexBuild, Reset, SelectLandClassByHeight
- auto: A / Terrain

## Assembly-CSharp/PQSMod_HeightColorMapNoise.cs
- L=127 kind=PQSMod type=PQSMod_HeightColorMapNoise bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(29): none | ksp: PQS=2 | fan_in=0
- methods(7): Clamp, CubicHermite, Lerp, OnSetup, OnVertexBuild, Reset, SelectLandClassByHeight
- auto: A / Terrain

## Assembly-CSharp/PQSMod_LandClassController.cs
- L=201 kind=PQSMod type=PQSMod_LandClassController bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(26): Texture=1 | ksp: none | fan_in=0
- methods(3): Lerp, Reset, Setup
- auto: A / Terrain

## Assembly-CSharp/PQSMod_LandClassScatterQuad.cs
- L=148 kind=MonoBehaviour type=PQSMod_LandClassScatterQuad bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(67): Transform=4, SetActive=4, GameObject=3, Mesh=2, Instantiate/Destroy=2, Renderer=1 | ksp: none | fan_in=1
- methods(7): Destroy, OnDestroy, OnQuadDestroy, OnQuadInvisible, OnQuadUpdate, OnQuadVisible, Setup
- auto: B / Terrain

## Assembly-CSharp/PQSMod_MapDecal.cs
- L=283 kind=PQSMod type=PQSMod_MapDecal bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(20): none | ksp: Vector3d=4, PQS=4 | fan_in=0
- methods(9): GetColorSmoothing, GetHeightSmoothing, Lerp, OnQuadBuilt, OnQuadPreBuild, OnSetup, OnVertexBuild, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_MapDecalTangent.cs
- L=281 kind=PQSMod type=PQSMod_MapDecalTangent bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(20): none | ksp: Vector3d=5, PQS=4 | fan_in=0
- methods(9): GetColorSmoothing, GetHeightSmoothing, Lerp, OnQuadBuilt, OnQuadPreBuild, OnSetup, OnVertexBuild, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_MaterialFadeAltitude.cs
- L=64 kind=PQSMod type=PQSMod_MaterialFadeAltitude bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(72): Renderer=2, Material=2, GetComponent=2, Shader=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(3): OnSetup, Reset, Update
- auto: B / Terrain

## Assembly-CSharp/PQSMod_MaterialFadeAltitudeDouble.cs
- L=89 kind=PQSMod type=PQSMod_MaterialFadeAltitudeDouble bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(69): Shader=2, Renderer=2, Material=2, Lifecycle=2, GetComponent=2 | ksp: none | fan_in=0
- methods(4): OnDestroy, OnSetup, Reset, Update
- auto: B / Terrain

## Assembly-CSharp/PQSMod_MaterialQuadRelative.cs
- L=32 kind=PQSMod type=PQSMod_MaterialQuadRelative bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(100): Shader=9, Renderer=3, Material=3, GetComponent=3 | ksp: PQS=1 | fan_in=0
- methods(2): OnQuadBuilt, OnSetup
- auto: B / Terrain

## Assembly-CSharp/PQSMod_MaterialSetDirection.cs
- L=26 kind=PQSMod type=PQSMod_MaterialSetDirection bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(80): Transform=4, Shader=2 | ksp: none | fan_in=0
- methods(2): OnUpdateFinished, Reset
- auto: B / Terrain

## Assembly-CSharp/PQSMod_MeshScatter.cs
- L=343 kind=PQSMod type=PQSMod_MeshScatter bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(74): Mesh=12, Transform=10, GameObject=10, SetActive=3, GetComponent=3, Renderer=2 | ksp: PQS=10 | fan_in=1
- methods(10): AddScatterMeshController, AssignScatterMesh, BuildCache, BuildCacheMesh, BuildCacheQuad, OnQuadBuilt, OnSetup, RandomRange, Reset, UnassignScatterMesh
- auto: B / Terrain

## Assembly-CSharp/PQSMod_MeshScatter_QuadControl.cs
- L=112 kind=MonoBehaviour type=PQSMod_MeshScatter_QuadControl bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(53): Renderer=2, GetComponent=2, MonoBehaviour=1, Mesh=1, Instantiate/Destroy=1 | ksp: none | fan_in=1
- methods(5): OnQuadDestroy, OnQuadInvisible, OnQuadUpdate, OnQuadVisible, Setup
- auto: B / Terrain

## Assembly-CSharp/PQSMod_OceanFX.cs
- L=112 kind=PQSMod type=PQSMod_OceanFX bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(89): Shader=15, Texture=5, Renderer=2, Material=2, GetComponent=2, Time.=1 | ksp: none | fan_in=0
- methods(2): OnSetup, OnUpdateFinished
- auto: B / Terrain

## Assembly-CSharp/PQSMod_QuadEnhanceCoast.cs
- L=40 kind=PQSMod type=PQSMod_QuadEnhanceCoast bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: none | fan_in=0
- methods(3): OnQuadBuilt, OnSetup, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_QuadMeshColliders.cs
- L=45 kind=PQSMod type=PQSMod_QuadMeshColliders bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(74): Material=2, PhysicMaterial=1, GetComponent=1, GameObject=1, Collider=1 | ksp: none | fan_in=0
- methods(4): OnQuadBuilt, OnQuadDestroy, OnSetup, Reset
- auto: B / Terrain

## Assembly-CSharp/PQSMod_RemoveQuadMap.cs
- L=50 kind=PQSMod type=PQSMod_RemoveQuadMap bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=2 | fan_in=0
- methods(5): OnQuadBuilt, OnQuadPreBuild, OnSetup, OnVertexBuild, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_ROCScatterQuad.cs
- L=168 kind=MonoBehaviour type=PQSMod_ROCScatterQuad bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(64): Transform=8, GameObject=4, SetActive=3, MonoBehaviour=1, Instantiate/Destroy=1, GetComponent=1 | ksp: GameSettings=1 | fan_in=2
- methods(6): Destroy, OnQuadDestroy, OnQuadInvisible, OnQuadUpdate, OnQuadVisible, Setup
- auto: B / Terrain

## Assembly-CSharp/PQSMod_SmoothLatitudeRange.cs
- L=48 kind=PQSMod type=PQSMod_SmoothLatitudeRange bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=2 | fan_in=0
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_SurfaceObjectQuads.cs
- L=39 kind=PQSMod type=PQSMod_SurfaceObjectQuads bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(66): GetComponent=3, GameObject=2 | ksp: CelestialBody=2 | fan_in=0
- methods(4): OnQuadBuilt, OnQuadDestroy, OnSetup, Reset
- auto: B / Terrain

## Assembly-CSharp/PQSMod_TangentTextureRanges.cs
- L=67 kind=PQSMod type=PQSMod_TangentTextureRanges bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=5 | fan_in=0
- methods(4): OnMeshBuild, OnSetup, OnVertexBuild, SmoothStep
- auto: A / Terrain

## Assembly-CSharp/PQSMod_TextureAtlas.cs
- L=215 kind=PQSMod type=PQSMod_TextureAtlas bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(55): Material=11, Lifecycle=1 | ksp: Vector3d=6, HighLogic=5, PQS=4, CelestialBody=3, Planetarium=1 | fan_in=0
- methods(4): CanTextureAtlasModBeUsed, OnQuadBuilt, OnSetup, Start
- auto: B / Terrain

## Assembly-CSharp/PQSMod_UV_Quad.cs
- L=10 kind=PQSMod type=PQSMod_UV_Quad bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=1 | fan_in=0
- methods(1): OnSetup
- auto: A / Terrain

## Assembly-CSharp/PQSMod_UV_Sphere.cs
- L=10 kind=PQSMod type=PQSMod_UV_Sphere bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=1 | fan_in=0
- methods(1): OnSetup
- auto: A / Terrain

## Assembly-CSharp/PQSMod_UVPlanetRelativePosition.cs
- L=34 kind=PQSMod type=PQSMod_UVPlanetRelativePosition bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=12, Vector3d=3 | fan_in=0
- methods(2): OnQuadBuilt, OnQuadUpdateNormals
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexColorMap.cs
- L=17 kind=PQSMod type=PQSMod_VertexColorMap bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=2 | fan_in=0
- methods(2): OnSetup, OnVertexBuild
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexColorMapBlend.cs
- L=24 kind=PQSMod type=PQSMod_VertexColorMapBlend bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(18): none | ksp: PQS=2 | fan_in=0
- methods(3): OnSetup, OnVertexBuild, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexColorNoise.cs
- L=74 kind=PQSMod type=PQSMod_VertexColorNoise bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(20): none | ksp: PQS=2 | fan_in=0
- methods(3): OnSetup, OnVertexBuild, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexColorNoiseRGB.cs
- L=83 kind=PQSMod type=PQSMod_VertexColorNoiseRGB bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(20): none | ksp: PQS=2 | fan_in=0
- methods(3): OnSetup, OnVertexBuild, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexColorSolid.cs
- L=24 kind=PQSMod type=PQSMod_VertexColorSolid bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(21): none | ksp: PQS=2 | fan_in=0
- methods(3): OnSetup, OnVertexBuild, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexColorSolidBlend.cs
- L=17 kind=PQSMod type=PQSMod_VertexColorSolidBlend bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(18): none | ksp: PQS=2 | fan_in=0
- methods(2): OnSetup, OnVertexBuild
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexDefineCoastLine.cs
- L=41 kind=PQSMod type=PQSMod_VertexDefineCoastLine bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=2 | fan_in=0
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexHeightMap.cs
- L=54 kind=PQSMod type=PQSMod_VertexHeightMap bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=3 | fan_in=0
- methods(6): GetVertexMaxHeight, GetVertexMinHeight, Lerp, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexHeightMapStep.cs
- L=67 kind=PQSMod type=PQSMod_VertexHeightMapStep bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(30): Texture=1 | ksp: PQS=3 | fan_in=0
- methods(6): GetVertexMaxHeight, GetVertexMinHeight, Lerp, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexHeightNoise.cs
- L=70 kind=PQSMod type=PQSMod_VertexHeightNoise bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(16): none | ksp: PQS=2 | fan_in=0
- methods(4): GetVertexMaxHeight, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexHeightNoiseHeightMap.cs
- L=104 kind=PQSMod type=PQSMod_VertexHeightNoiseHeightMap bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(26): Texture=1 | ksp: PQS=2 | fan_in=0
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexHeightNoiseVertHeight.cs
- L=102 kind=PQSMod type=PQSMod_VertexHeightNoiseVertHeight bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(16): none | ksp: PQS=2 | fan_in=0
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexHeightNoiseVertHeightCurve.cs
- L=111 kind=PQSMod type=PQSMod_VertexHeightNoiseVertHeightCurve bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(22): AnimationCurve=2 | ksp: PQS=1 | fan_in=0
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexHeightNoiseVertHeightCurve2.cs
- L=127 kind=PQSMod type=PQSMod_VertexHeightNoiseVertHeightCurve2 bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(28): AnimationCurve=4 | ksp: PQS=2 | fan_in=0
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexHeightNoiseVertHeightCurve3.cs
- L=180 kind=PQSMod type=PQSMod_VertexHeightNoiseVertHeightCurve3 bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(28): AnimationCurve=4 | ksp: Vector3d=3, PQS=2 | fan_in=0
- methods(9): GetVertexMaxHeight, GetVertexMinHeight, Lerp, Noise, NoiseNormalized, OnSetup, OnVertexBuildHeight, Reset, Setup
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexHeightOblate.cs
- L=30 kind=PQSMod type=PQSMod_VertexHeightOblate bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=2 | fan_in=0
- methods(3): OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexHeightOffset.cs
- L=17 kind=PQSMod type=PQSMod_VertexHeightOffset bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=1 | fan_in=0
- methods(2): OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexNoise.cs
- L=75 kind=PQSMod type=PQSMod_VertexNoise bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(16): none | ksp: PQS=2 | fan_in=0
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexPlanet.cs
- L=361 kind=PQSMod type=PQSMod_VertexPlanet bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(23): none | ksp: PQS=4 | fan_in=0
- methods(11): Clamp, CubicHermite, GetVertexMaxHeight, GetVertexMinHeight, Lerp, OnSetup, OnVertexBuild, OnVertexBuildHeight, Reset, SelectLandClassByHeight, Setup
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexRidgedAltitudeCurve.cs
- L=115 kind=PQSMod type=PQSMod_VertexRidgedAltitudeCurve bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(29): AnimationCurve=4 | ksp: PQS=2 | fan_in=0
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexSimplexColorRGB.cs
- L=54 kind=PQSMod type=PQSMod_VertexSimplexColorRGB bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(20): none | ksp: PQS=2 | fan_in=0
- methods(3): OnSetup, OnVertexBuild, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexSimplexHeight.cs
- L=48 kind=PQSMod type=PQSMod_VertexSimplexHeight bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=2 | fan_in=0
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexSimplexHeightAbsolute.cs
- L=48 kind=PQSMod type=PQSMod_VertexSimplexHeightAbsolute bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=2 | fan_in=0
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexSimplexHeightFlatten.cs
- L=60 kind=PQSMod type=PQSMod_VertexSimplexHeightFlatten bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=2 | fan_in=0
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexSimplexHeightMap.cs
- L=64 kind=PQSMod type=PQSMod_VertexSimplexHeightMap bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=2 | fan_in=0
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexSimplexMultiChromatic.cs
- L=91 kind=PQSMod type=PQSMod_VertexSimplexMultiChromatic bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(18): none | ksp: PQS=2 | fan_in=0
- methods(3): OnSetup, OnVertexBuild, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexSimplexNoiseColor.cs
- L=45 kind=PQSMod type=PQSMod_VertexSimplexNoiseColor bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(27): none | ksp: PQS=2 | fan_in=0
- methods(3): OnSetup, OnVertexBuild, Reset
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VertexVoronoi.cs
- L=134 kind=PQSMod type=PQSMod_VertexVoronoi bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(15): none | ksp: PQS=1 | fan_in=0
- methods(5): GetValue, IntValueNoise3D, OnSetup, OnVertexBuildHeight, ValueNoise3D
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VoronoiCraters.cs
- L=83 kind=PQSMod type=PQSMod_VoronoiCraters bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(27): AnimationCurve=2 | ksp: PQS=2 | fan_in=0
- methods(3): OnSetup, OnVertexBuild, OnVertexBuildHeight
- auto: A / Terrain

## Assembly-CSharp/PQSMod_VoronoiCraters2.cs
- L=105 kind=PQSMod type=PQSMod_VoronoiCraters2 bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(21): AnimationCurve=1 | ksp: PQS=2, Vector3d=1 | fan_in=0
- methods(3): OnSetup, OnVertexBuild, OnVertexBuildHeight
- auto: A / Terrain

## Assembly-CSharp/PQSROCControl.cs
- L=160 kind=PQSMod type=PQSROCControl bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(57): GameObject=6, Mesh=2, GetComponent=2, Transform=1, Lifecycle=1 | ksp: PQS=3, CelestialBody=2 | fan_in=2
- methods(8): Awake, OnQuadBuilt, OnQuadDestroy, OnQuadPreBuild, OnSetup, OnSphereReset, OnSphereStarted, OnVertexBuild
- auto: B / Terrain

## Assembly-CSharp/PQSSurfaceObject.cs
- L=8 kind=PQSMod type=PQSSurfaceObject bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(36): Transform=1 | ksp: Vector3d=1 | fan_in=6
- methods(0): 
- auto: A / Terrain

## Assembly-CSharp/PrefabAnimationFX.cs
- L=106 kind=MonoBehaviour type=PrefabAnimationFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(73): Transform=7, Animation=3, GameObject=2, Resources.=1, GetComponent=1 | ksp: ConfigNode=4 | fan_in=0
- methods(4): OnEvent, OnInitialize, OnLoad, OnSave
- auto: C / Rendering

## Assembly-CSharp/PrefabMultiParticleFX.cs
- L=241 kind=MonoBehaviour type=PrefabMultiParticleFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(80): ParticleSystem=17, GameObject=10, Transform=7, Instantiate/Destroy=3, GetComponent=2, SetActive=1 | ksp: ConfigNode=4 | fan_in=0
- methods(6): OnDestroy, OnEvent, OnInitialize, OnLoad, OnSave, SetEmitter
- auto: C / Rendering

## Assembly-CSharp/PrefabParticleFX.cs
- L=174 kind=MonoBehaviour type=PrefabParticleFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(83): ParticleSystem=16, GameObject=7, Transform=6, Instantiate/Destroy=2, SetActive=1, Resources.=1 | ksp: ConfigNode=4 | fan_in=0
- methods(5): OnDestroy, OnEvent, OnInitialize, OnLoad, OnSave
- auto: C / Rendering

## Assembly-CSharp/PrefabSpawner.cs
- L=17 kind=MonoBehaviour type=PrefabSpawner bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(92): Transform=6, GameObject=3, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1 | ksp: none | fan_in=0
- methods(1): Awake
- auto: C / Rendering

## Assembly-CSharp/PrefabSpawnFX.cs
- L=69 kind=MonoBehaviour type=PrefabSpawnFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(84): GameObject=10, Transform=5, Instantiate/Destroy=2, Resources.=1 | ksp: ConfigNode=4 | fan_in=0
- methods(4): OnEvent, OnInitialize, OnLoad, OnSave
- auto: C / Rendering

## Assembly-CSharp/PreferredInitialStage.cs
- L=7 kind=enum type=PreferredInitialStage bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Vessel

## Assembly-CSharp/PreFlightCheck.cs
- L=116 kind=class type=PreFlightCheck bases=- anc=- ns=-
- unity(16): none | ksp: GameSettings=6, HighLogic=4, Localizer=1 | fan_in=8
- methods(5): Abort, AddTest, Complete, RunTests, runNextTest
- auto: A / Editor

## Assembly-CSharp/PreviewPanel.cs
- L=17 kind=MonoBehaviour type=PreviewPanel bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(80): EventSystems=4, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(2): OnPointerEnter, OnPointerExit
- auto: C / UI

## Assembly-CSharp/PreviewType.cs
- L=12 kind=class type=PreviewType bases=- anc=- ns=-
- unity(29): GameObject=1 | ksp: ProtoCrewMember=2 | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/PrivacyDialog.cs
- L=64 kind=MonoBehaviour type=PrivacyDialog bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(87): GameObject=3, UnityEngine.UI=2, Transform=2, Lifecycle=2, Instantiate/Destroy=2, SetActive=1 | ksp: InputLockManager=2 | fan_in=1
- methods(8): Create, Dismiss, OnCloseButton, OnPrivacyURLFailure, OnPrivacyURLReceived, OnUnityData, Start, Update
- auto: C / Analytics

## Assembly-CSharp/ProceduralAsteroid.cs
- L=133 kind=MonoBehaviour type=ProceduralAsteroid bases=ProceduralSpaceObject anc=ProceduralSpaceObject/MonoBehaviour ns=-
- unity(75): Transform=7, Mesh=3, GetComponent=3, GameObject=3, Time.=2, Application.=2 | ksp: Part=2 | fan_in=3
- methods(4): CreatePAsteroid, Generate, Rebuild, Start
- auto: B / Celestials

## Assembly-CSharp/ProceduralAsteroidTest.cs
- L=88 kind=MonoBehaviour type=ProceduralAsteroidTest bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(83): Transform=3, Rigidbody=3, GetComponent=3, Instantiate/Destroy=2, GameObject=2, Application.=2 | ksp: none | fan_in=0
- methods(6): AddRigidbody, NewSeed, Randomize, RangefinderGeneric, Rebuild, Start
- auto: B / Celestials

## Assembly-CSharp/ProceduralComet.cs
- L=154 kind=MonoBehaviour type=ProceduralComet bases=ProceduralSpaceObject anc=ProceduralSpaceObject/MonoBehaviour ns=-
- unity(73): Transform=7, Mesh=3, GetComponent=3, GameObject=3, Time.=2, Application.=2 | ksp: Part=2 | fan_in=3
- methods(4): CreatePComet, Generate, Rebuild, Start
- auto: B / Celestials

## Assembly-CSharp/ProceduralCometTest.cs
- L=99 kind=MonoBehaviour type=ProceduralCometTest bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(88): Transform=7, GetComponent=4, GameObject=4, Rigidbody=3, Instantiate/Destroy=3, Resources.=2 | ksp: none | fan_in=0
- methods(6): AddRigidbody, NewSeed, Randomize, RangefinderGeneric, Rebuild, Start
- auto: B / Celestials

## Assembly-CSharp/ProceduralCubemap.cs
- L=120 kind=MonoBehaviour type=ProceduralCubemap bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(99): Texture=25, Camera=13, Transform=6, GameObject=6, Instantiate/Destroy=4, GetComponent=2 | ksp: none | fan_in=0
- methods(7): CreateCamera, CreateCameraOrtho, CreateCubeMap, CreateIndividualFaces, RenderCamera, RenderCubemap, RenderIndividualFaces
- auto: C / Rendering

## Assembly-CSharp/ProceduralSpaceObject.cs
- L=232 kind=MonoBehaviour type=ProceduralSpaceObject bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(62): Transform=2, PhysicMaterial=2, Mesh=2, Material=2, GetComponent=2, MonoBehaviour=1 | ksp: PQS=5 | fan_in=2
- methods(10): CreateMeshCollider, CreateMeshVisual, GetSphereVolume, HasWrapperOfTypeName, Mod_Build, Mod_OnSetup, NewSeed, RangefinderGeneric, Reset, UpdateWrappers
- auto: B / Celestials

## Assembly-CSharp/ProceduralStarfield.cs
- L=166 kind=MonoBehaviour type=ProceduralStarfield bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(74): ParticleSystem=7, Transform=5, Time.=2, Material=2, GetComponent=2, GameObject=2 | ksp: none | fan_in=0
- methods(5): CreateStarFields, CreateStarsWithNewSystem, ExclusionZoneTest, Reset, Start
- auto: C / Rendering

## Assembly-CSharp/ProfileTimer.cs
- L=58 kind=static class type=ProfileTimer bases=- anc=- ns=-
- unity(17): Time.=2 | ksp: none | fan_in=2
- methods(3): OutputTimer, Pop, Push
- auto: C / Debug

## Assembly-CSharp/ProgressNode.cs
- L=280 kind=class type=ProgressNode bases=IConfigNode anc=IConfigNode ns=-
- unity(3): none | ksp: GameEvents=8, ResearchAndDevelopment=6, ConfigNode=6, Reputation=4, Localizer=4 | fan_in=35
- methods(15): Achieve, AddOrAppendWorldFirstMessage, AwardProgress, AwardProgressInterval, AwardProgressRandomTech, AwardProgressStandard, CheatComplete, Complete, CrewSensitiveComplete, GetNodeSummary, Load, OnLoad, OnSave, Reach, Save
- auto: A / Career

## Assembly-CSharp/ProgressTracking.cs
- L=533 kind=ScenarioModule type=ProgressTracking bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(22): Lifecycle=3, GameObject=1 | ksp: Localizer=50, FlightGlobals=5, GameEvents=2, ConfigNode=2, ScenarioModule=1 | fan_in=25
- methods(22): CheatEarlyProgression, CheatProgression, DebugAchievements, DebugSummary, FindNode, FindNodeRecurse, GenerateSummary, GetBodyTree, NodeComplete, NodeCompleteManned, NodeCompleteUnmanned, NodeReached, OnAchievementComplete, OnAwake, OnDestroy, OnLoad, OnSave, RecurseCheatProgression, Start, TestProgressDataPost, Update, generateAchievementsTree
- auto: A / Career

## Assembly-CSharp/ProgressTree.cs
- L=158 kind=class type=ProgressTree bases=IConfigNode anc=IConfigNode ns=-
- unity(7): none | ksp: ConfigNode=2, Vessel=1 | fan_in=3
- methods(10): AddNode, AllComplete, Contains, Deploy, GetEnumerator, GetTreeSummary, IterateVessels, Load, Save, Stow
- auto: A / Career

## Assembly-CSharp/PropCollider.cs
- L=7 kind=MonoBehaviour type=PropCollider bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(55): MonoBehaviour=1, Collider=1 | ksp: none | fan_in=0
- methods(0): 
- auto: B / Misc

## Assembly-CSharp/Propellant.cs
- L=144 kind=class type=Propellant bases=- anc=- ns=-
- unity(8): none | ksp: PartResourceDefinition=4, ConfigNode=2, Part=1 | fan_in=5
- methods(6): GetFlowMode, GetFlowModeDescription, Load, Save, UpdateConnectedResources, getPartResourceDefinition
- auto: A / PartModules-Engines

## Assembly-CSharp/PropertyIDs.cs
- L=59 kind=MonoBehaviour type=PropertyIDs bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(100): Shader=32, MonoBehaviour=1, Lifecycle=1, LayerMask=1 | ksp: none | fan_in=13
- methods(1): Awake
- auto: C / Rendering

## Assembly-CSharp/PropObject.cs
- L=146 kind=MonoBehaviour type=PropObject bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(97): GameObject=15, Mesh=14, GetComponent=9, Transform=7, Material=5, Instantiate/Destroy=5 | ksp: none | fan_in=0
- methods(5): Create, CreateProxies, GetComponentOnParent, RecreateProxies, RefreshProxy
- auto: B / Misc

## Assembly-CSharp/PropTools.cs
- L=213 kind=MonoBehaviour type=PropTools bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(34): MonoBehaviour=1 | ksp: none | fan_in=1
- methods(4): CreatePropInfo, CreatePropInfoList, CreateProxyInfo, ProxyError
- auto: B / Misc

## Assembly-CSharp/ProtoPartModuleSnapshot.cs
- L=224 kind=class type=ProtoPartModuleSnapshot bases=- anc=- ns=-
- unity(5): GameObject=1 | ksp: ConfigNode=28, GameEvents=12, ProtoPartModuleSnapshot=7, PartModule=4, Part=1 | fan_in=30
- methods(9): AddPartModuleAdjuster, AddPartModuleAdjusterList, EnsureModuleValuesIsInitialized, GetListOfActiveAdjusters, Load, ProtoPartModuleRepair, RemovePartModuleAdjuster, RemovePartModuleAdjusterList, Save
- auto: A / Save

## Assembly-CSharp/ProtoPartResourceSnapshot.cs
- L=101 kind=class type=ProtoPartResourceSnapshot bases=- anc=- ns=-
- unity(0): none | ksp: Localizer=6, ConfigNode=5, AvailablePart=3, PartResourceDefinition=2, PartResource=2 | fan_in=11
- methods(4): GetCurrentResourceInfo, Load, Save, UpdateConfigNodeAmounts
- auto: A / Save

## Assembly-CSharp/ProtoRDNode.cs
- L=93 kind=class type=ProtoRDNode bases=- anc=- ns=-
- unity(0): none | ksp: ConfigNode=5, ResearchAndDevelopment=2 | fan_in=1
- methods(3): FindNodeByID, LoadLinks, Save
- auto: A / Science

## Assembly-CSharp/ProtoScenarioModule.cs
- L=101 kind=class type=ProtoScenarioModule bases=- anc=- ns=-
- unity(4): none | ksp: ConfigNode=7, ScenarioModule=4 | fan_in=9
- methods(4): GetData, Load, Save, SetTargetScenes
- auto: A / Save

## Assembly-CSharp/ProtoTargetInfo.cs
- L=200 kind=class type=ProtoTargetInfo bases=IConfigNode anc=IConfigNode ns=-
- unity(24): GameObject=5, MonoBehaviour=2, GetComponent=1 | ksp: PartModule=8, Part=8, FlightGlobals=6, CelestialBody=6, Vessel=5 | fan_in=2
- methods(3): FindTarget, Load, Save
- auto: B / Orbit

## Assembly-CSharp/ProtoTechNode.cs
- L=99 kind=class type=ProtoTechNode bases=- anc=- ns=-
- unity(11): none | ksp: ConfigNode=5, AvailablePart=3, PartLoader=1 | fan_in=10
- methods(2): Save, UpdateFromTechNode
- auto: A / Science

## Assembly-CSharp/ProtoWaypointInfo.cs
- L=86 kind=class type=ProtoWaypointInfo bases=IConfigNode anc=IConfigNode ns=-
- unity(0): none | ksp: ConfigNode=3 | fan_in=2
- methods(3): FindWaypoint, Load, Save
- auto: A / Save

## Assembly-CSharp/PSpaceObject.cs
- L=171 kind=MonoBehaviour type=PSpaceObject bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(93): Mesh=11, Transform=8, LayerMask=6, GameObject=6, Renderer=5, PhysicMaterial=4 | ksp: Part=1 | fan_in=3
- methods(10): Awake, CreateCollider, CreateConvexCollider, CreateVisual, GetVisualObjectRenderer, OnDestroy, SetMaterialColor, Setup, SetupPartParameters, onSOCCSetupComplete
- auto: B / Celestials

## Assembly-CSharp/PSystem.cs
- L=248 kind=MonoBehaviour type=PSystem bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(55): GameObject=9, Transform=4, GetComponent=4, MonoBehaviour=1 | ksp: ConfigNode=10, OrbitDriver=1, CelestialBody=1 | fan_in=1
- methods(10): AddBody, FindBody, GetBodies, LoadBody, LoadDatabase, OnDrawGizmos, OnDrawGizmosBody, Reset, SaveBody, SaveDatabase
- auto: B / Celestials

## Assembly-CSharp/PSystemBody.cs
- L=21 kind=MonoBehaviour type=PSystemBody bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(50): MonoBehaviour=1, GameObject=1 | ksp: PQS=1, OrbitDriver=1, CelestialBody=1 | fan_in=4
- methods(0): 
- auto: B / Celestials

## Assembly-CSharp/PSystemManager.cs
- L=249 kind=MonoBehaviour type=PSystemManager bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(85): GameObject=27, Transform=20, GetComponent=11, Instantiate/Destroy=6, Lifecycle=2, MonoBehaviour=1 | ksp: CelestialBody=13, Planetarium=3, ScaledSpace=2, PQS=2, OrbitDriver=2 | fan_in=9
- methods(12): Awake, OnDestroy, PostSetup, Reset, SetBodyReferences, SetupLocal, SetupLocalSpace, SetupPSystem, SetupScaled, SetupScaledSpace, SpawnBody, Start
- auto: B / Celestials

## Assembly-CSharp/QuickSaveLoad.cs
- L=494 kind=MonoBehaviour type=QuickSaveLoad bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(54): GameObject=7, SetActive=5, KeyCode=5, Input.=5, Lifecycle=3, SendMessage/Invoke=2 | ksp: HighLogic=22, Localizer=21, GameSettings=9, InputLockManager=7, ConfigNode=6 | fan_in=2
- methods(21): Awake, CacheLocalStrings, CheckFilename, ConfirmDialog, EscapeString, FinishedSaveLoadDialogCallback, OnDestroy, OnLoadClose, OnSaveAsClose, QuickSave, QuickSaveClearToSave, SpawnQuickLoadDialog, SpawnSaveAsDialog, Update, doSave, onHoldComplete, onQuickloadPipelineFailed, onQuickloadPipelineFinished, quickLoad, quickSave, start
- auto: C / Save

## Assembly-CSharp/RDArchivesAvatarController.cs
- L=110 kind=MonoBehaviour type=RDArchivesAvatarController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(35): MonoBehaviour=1, Lifecycle=1 | ksp: Localizer=5 | fan_in=1
- methods(3): SetAvatarState, Start, getThoughtVector
- auto: B / Misc

## Assembly-CSharp/RDPartCollisionHandler.cs
- L=39 kind=MonoBehaviour type=RDPartCollisionHandler bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(84): Collision/Force=6, MonoBehaviour=1, Lifecycle=1 | ksp: KerbalEVA=1 | fan_in=1
- methods(4): OnCollisionEnter, OnCollisionExit, OnCollisionStay, Start
- auto: B / Misc

## Assembly-CSharp/RDTech.cs
- L=231 kind=MonoBehaviour type=RDTech bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(40): GameObject=3, Lifecycle=2, MonoBehaviour=1, Analytics=1 | ksp: GameEvents=11, AvailablePart=11, ResearchAndDevelopment=4, PartLoader=3, Localizer=2 | fan_in=17
- methods(11): AutoPurchaseAllParts, HandlePurchase, Load, OnDestroy, PartIsPurchased, PurchasePart, ResearchTech, Save, Start, UnlockTech, Warmup
- auto: B / Science

## Assembly-CSharp/RecoverySituations.cs
- L=15 kind=enum type=RecoverySituations bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Career

## Assembly-CSharp/ReentryFXParticleTrail.cs
- L=44 kind=MonoBehaviour type=ReentryFXParticleTrail bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(91): ParticleSystem=3, Transform=2, Lifecycle=2, GetComponent=2, Time.=1, Rigidbody=1 | ksp: Vessel=2 | fan_in=0
- methods(2): FixedUpdate, Start
- auto: C / Rendering

## Assembly-CSharp/ReflectionUtil.cs
- L=61 kind=static class type=ReflectionUtil bases=- anc=- ns=-
- unity(12): none | ksp: none | fan_in=1
- methods(1): GetAttributedTypesInAssemblies
- auto: A / Modding

## Assembly-CSharp/Reputation.cs
- L=192 kind=ScenarioModule type=Reputation bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(35): Lifecycle=2, GameObject=1 | ksp: GameEvents=12, HighLogic=4, Reputation=3, ConfigNode=2, ScenarioModule=1 | fan_in=43
- methods(13): AddReputation, ModifyReputationDelta, OnAwake, OnCrewKilled, OnCurrenciesModified, OnDestroy, OnLoad, OnSave, SetReputation, Start, addReputation_discrete, addReputation_granular, onvesselRecoveryProcessing
- auto: A / Career

## Assembly-CSharp/ReputationWidget.cs
- L=38 kind=MonoBehaviour type=ReputationWidget bases=CurrencyWidget anc=CurrencyWidget/MonoBehaviour ns=-
- unity(46): Lifecycle=2 | ksp: Reputation=5, GameEvents=2, HighLogic=1 | fan_in=0
- methods(5): Awake, DelayedStart, OnAboutToStart, OnDestroy, onReputationChanged
- auto: B / Career

## Assembly-CSharp/ResetTransformOnSceneSwitch.cs
- L=34 kind=MonoBehaviour type=ResetTransformOnSceneSwitch bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(86): Transform=4, SceneManager=3, Lifecycle=2, MonoBehaviour=1 | ksp: GameEvents=2 | fan_in=1
- methods(5): OnDestroy, OnLevelLoaded, OnSceneLoaded, OnSceneSwitch, Start
- auto: B / Scene

## Assembly-CSharp/ResourceBroker.cs
- L=71 kind=class type=ResourceBroker bases=IResourceBroker anc=IResourceBroker ns=-
- unity(0): none | ksp: Part=8 | fan_in=2
- methods(4): AmountAvailable, RequestResource, StorageAvailable, StoreResource
- auto: A / Resources

## Assembly-CSharp/ResourceCache.cs
- L=126 kind=MonoBehaviour type=ResourceCache bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(35): MonoBehaviour=1, Lifecycle=1, GetComponent=1, GameObject=1 | ksp: FlightGlobals=2, CelestialBody=2, GameDatabase=1 | fan_in=3
- methods(5): LoadAbundanceCache, LoadResourceInfo, OnDestroy, RequestSummary, ResetCache
- auto: B / Resources

## Assembly-CSharp/ResourceConverter.cs
- L=168 kind=class type=ResourceConverter bases=- anc=- ns=-
- unity(0): none | ksp: Localizer=6, PartResourceDefinition=3, PartModule=1, Part=1, GameEvents=1 | fan_in=2
- methods(2): CacheLocalStrings, ProcessRecipe
- auto: A / PartModules-Resources

## Assembly-CSharp/ResourceData.cs
- L=12 kind=class type=ResourceData bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / Resources

## Assembly-CSharp/ResourceFlowMode.cs
- L=12 kind=enum type=ResourceFlowMode bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=22
- methods(0): 
- auto: A / Resources

## Assembly-CSharp/ResourceGameSettings.cs
- L=288 kind=class type=ResourceGameSettings bases=- anc=- ns=-
- unity(8): none | ksp: ConfigNode=16, MissionSystem=3, HighLogic=2, Planetarium=1, Game=1 | fan_in=2
- methods(15): CheckDepletionNode, GenerateNewROCMissionSeed, GenerateNewSeed, GetBiomeLockInfo, GetDepletionInfo, GetPlanetScanInfo, GetValue, Load, Save, SaveBiomeUnlockNode, SaveDepletionNodeValue, SavePlanetUnlockNode, SetupBiomeLockInfo, SetupDepletionInfo, SetupPlanetScanInfo
- auto: A / Resources

## Assembly-CSharp/ResourceMap.cs
- L=688 kind=MonoBehaviour type=ResourceMap bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(21): Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: FlightGlobals=9, CelestialBody=5, PartResourceDefinition=2, Planetarium=1, GameEvents=1 | fan_in=15
- methods(21): Awake, DeDuplicate, DetermineBiomeName, FetchAllResourceNames, GenerateAbundanceSeed, GetAbundance, GetAbundanceFromCache, GetBestResourceData, GetCacheNames, GetDefaultSituation, GetDepletionNode, GetDepletionNodeValue, GetResourceByName, GetResourceItemList, IsBiomeUnlocked, IsPlanetScanned, OnDestroy, ResetCache, SetDepletionNodeValue, UnlockBiome, UnlockPlanet
- auto: B / Resources

## Assembly-CSharp/ResourceRatio.cs
- L=63 kind=struct type=ResourceRatio bases=- anc=- ns=-
- unity(11): none | ksp: ConfigNode=2 | fan_in=8
- methods(2): Load, Save
- auto: A / PartModules-Resources

## Assembly-CSharp/ResourceScenario.cs
- L=58 kind=ScenarioModule type=ResourceScenario bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(56): Lifecycle=2, Instantiate/Destroy=1, GetComponent=1, GameObject=1 | ksp: ConfigNode=2, ScenarioModule=1 | fan_in=8
- methods(5): OnAwake, OnDestroy, OnLoad, OnSave, Start
- auto: A / Resources

## Assembly-CSharp/ResourceSetup.cs
- L=40 kind=MonoBehaviour type=ResourceSetup bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): MonoBehaviour=1, Lifecycle=1, GetComponent=1, GameObject=1 | ksp: GameDatabase=2 | fan_in=5
- methods(2): LoadResourceConfig, OnDestroy
- auto: B / Resources

## Assembly-CSharp/ResourceTransferMode.cs
- L=6 kind=enum type=ResourceTransferMode bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Resources

## Assembly-CSharp/ResourceUtilities.cs
- L=250 kind=static class type=ResourceUtilities bases=- anc=- ns=-
- unity(1): none | ksp: ConfigNode=9, Vessel=1, HighLogic=1, CelestialBody=1 | fan_in=17
- methods(17): DeepClone, Deg2Rad, GetAltitude, GetBiome, GetDifficultyLevel, GetMaxDeltaTime, GetValue, HSL2RGB, ImportBiomeLockNodeList, ImportConfigNodeList, ImportDepletionNodeList, ImportPlanetScanNodeList, LoadNodeProperties, Rad2Lat, Rad2Lon, clampLat, clampLon
- auto: A / Resources

## Assembly-CSharp/RetractableLadder.cs
- L=365 kind=PartModule type=RetractableLadder bases=PartModule,IMultipleDragCube anc=PartModule/MonoBehaviour ns=-
- unity(64): Transform=14, Animation=6, GetComponent=5, GameObject=5, Lifecycle=2, Collider=2 | ksp: KSPField=5, HighLogic=4, KSPAction=3, ProtoPartModuleSnapshot=2, KSPEvent=2 | fan_in=1
- methods(17): AssumeDragCubePosition, DemoteToPhysicslessPart, Extend, ExtendAction, FixedUpdate, GetDragCubeNames, OnInventoryModeDisable, OnPartCreatedFomInventory, OnStart, OnStoredInInventory, Retract, RetractAction, SetAnimationToRetracted, SetDragState, ToggleAction, Update, UsesProceduralDragCubes
- auto: B / Kerbals

## Assembly-CSharp/RichTextUtil.cs
- L=61 kind=class type=RichTextUtil bases=- anc=- ns=-
- unity(16): none | ksp: none | fan_in=3
- methods(8): Text, TextAdvance, TextAgent, TextAward, TextParam, TextParamsAward, TextPenalty, Title
- auto: C / UI

## Assembly-CSharp/RnDBuilding.cs
- L=55 kind=MonoBehaviour type=RnDBuilding bases=SpaceCenterBuilding anc=SpaceCenterBuilding/MonoBehaviour ns=-
- unity(22): none | ksp: Localizer=6, HighLogic=5, GameEvents=4, InputLockManager=3, ResearchAndDevelopment=2 | fan_in=0
- methods(5): IsOpen, OnClicked, OnOnDestroy, OnPopupWarningDismiss, onDialogClose
- auto: B / Science

## Assembly-CSharp/RnDCommsStream.cs
- L=92 kind=class type=RnDCommsStream bases=- anc=- ns=-
- unity(29): Coroutine=3, Time.=2 | ksp: ScienceSubject=3, ResearchAndDevelopment=3, ProtoVessel=3 | fan_in=1
- methods(3): StreamData, submitStreamData, timeoutCoroutine
- auto: A / Science

## Assembly-CSharp/RnDDebugUtil.cs
- L=715 kind=class type=RnDDebugUtil bases=- anc=- ns=-
- unity(66): IMGUI=86, UnityEngine.UI=22, PlayerPrefs=6, Texture=2, GameObject=2 | ksp: AvailablePart=12, ConfigNode=10, HighLogic=5, ResearchAndDevelopment=4, PartResourceDefinition=1 | fan_in=2
- methods(25): CompareRDNodes, DiscardPart, DrawFilesPage, DrawNodePage, DrawPartsPage, DrawSciencePage, DrawSettingsPage, DrawTechPage, DrawTreePage, DrawWindow, GetSortedRDNodes, HasTextEditor, Init, LoadSettings, OnNodeSelected, OnNodeUnselected, OnTreeDespawn, OnTreeSpawn, RebuildTree, RefreshUI, ReloadPartAssignments, ReloadParts, SavePart, SaveSettings ...
- auto: C / Science

## Assembly-CSharp/ROC.cs
- L=430 kind=MonoBehaviour type=ROC bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(67): GameObject=19, Transform=11, Instantiate/Destroy=5, Raycast/Query=3, Lifecycle=3, GetComponent=2 | ksp: GameEvents=4, HighLogic=3, FloatCurve=2, Vessel=1, ResearchAndDevelopment=1 | fan_in=8
- methods(16): Awake, CheckCollision, CompareTo, DestroyRocFinderArrow, DestroyScanPointArrows, GetClosestScanPosition, GetClosestScanPositionWithRaycasts, GetVFXForceScale, OnDestroy, PerformExperiment, PickUpROC, SetRocEmitters, SetStats, Start, UpdateModelTransform, UpdateRocArrow
- auto: B / DLC-BreakingGround

## Assembly-CSharp/RocCBDefinition.cs
- L=28 kind=class type=RocCBDefinition bases=- anc=- ns=-
- unity(12): none | ksp: ConfigNode=1 | fan_in=5
- methods(1): Load
- auto: A / DLC-BreakingGround

## Assembly-CSharp/ROCDefinition.cs
- L=188 kind=class type=ROCDefinition bases=- anc=- ns=-
- unity(35): none | ksp: FloatCurve=3, ConfigNode=3 | fan_in=7
- methods(2): ContainsCBBiome, Load
- auto: A / DLC-BreakingGround

## Assembly-CSharp/ROCEmitter.cs
- L=376 kind=MonoBehaviour type=ROCEmitter bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(61): GetComponent=6, Transform=5, Lifecycle=5, GameObject=5, Time.=4, ParticleSystem=4 | ksp: Part=6, GameEvents=4, KSPField=2 | fan_in=1
- methods(23): AddForceTarget, ApplyForces, Awake, BurstTimer, OnDisable, OnEnable, OnGamePause, OnGameUnpause, OnTriggerEnter, OnTriggerExit, RemoveForceTarget, ResetSecondsTillBurst, SetBurstEmitterMaxWait, SetBurstEmitterMinWait, SetForceScale, SetSFXClipPath, SetSFXPower, Start, StartBurstEmision, StartIdleEmision, StopBurstEmision, StopIdleEmision, Update
- auto: B / DLC-BreakingGround

## Assembly-CSharp/ROCManager.cs
- L=498 kind=MonoBehaviour type=ROCManager bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(56): GameObject=18, Transform=6, Instantiate/Destroy=4, GetComponent=4, Lifecycle=2, SetActive=1 | ksp: ConfigNode=6, CelestialBody=4, GameEvents=3, PQS=2, HighLogic=1 | fan_in=13
- methods(24): AddROCStats_Quad, AddROCStats_ROCs, Awake, ClearROCsCache, ClearStats, DisableAllROCs, EnableAllROCs, GetROCControlFromCB, GetTerrainTag, LoadRemovedROCs, OnGameNewStart, OnGameStateLoad, PopulateROCControlFromConfig, ROCRemoved, RemoveROC, SaveRemoveROCs, SetROCsStateForGame, Start, SubtractROCStats_Quad, SubtractROCStats_ROCs, ValidCBBiome, ValidCelestialBody, ValidateCBBiomeCombos, onMissionStarted
- auto: B / DLC-BreakingGround

## Assembly-CSharp/ROCScenario.cs
- L=22 kind=ScenarioModule type=ROCScenario bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(50): Lifecycle=1, Instantiate/Destroy=1 | ksp: ScenarioModule=1 | fan_in=0
- methods(1): Start
- auto: A / DLC-BreakingGround

## Assembly-CSharp/ROCsSFX.cs
- L=197 kind=MonoBehaviour type=ROCsSFX bases=EffectBehaviour anc=EffectBehaviour/MonoBehaviour ns=-
- unity(54): Audio=5, Lifecycle=4, Time.=2, GetComponent=1, GameObject=1 | ksp: GameEvents=4, KSPField=2, GameDatabase=1 | fan_in=1
- methods(16): CreateSource, FadeIn, FadeInPlayback, FadeOut, FadeOutPlayback, OnDisable, OnEnable, OnGamePause, OnGameUnpause, Play, PlayBurstSFX, PlayIdleSFX, SetClipsPath, SetSFXVolume, Start, Update
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Rotato.cs
- L=15 kind=MonoBehaviour type=Rotato bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(67): Lifecycle=2, Transform=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(2): FixedUpdate, Start
- auto: B / Physics

## Assembly-CSharp/RUIHoverController.cs
- L=102 kind=class type=RUIHoverController bases=- anc=- ns=-
- unity(35): Coroutine=5, MonoBehaviour=2 | ksp: none | fan_in=1
- methods(3): Deschedule, Schedule, UpdateDaemon
- auto: A / Misc

## Assembly-CSharp/RUIutils.cs
- L=316 kind=static class type=RUIutils bases=- anc=- ns=-
- unity(8): Camera=2 | ksp: none | fan_in=34
- methods(17): All, And, Any, CamToCamReposition, ColorToHex, Compare, Count, CutString, DecToHex, GetYesNoUIString, HexToChar, Repeat, SortAscDescPrimarySecondary, StringListToString, Sum, Where, WhereMutating
- auto: A / Misc

## Assembly-CSharp/RunwayCollisionHandler.cs
- L=176 kind=MonoBehaviour type=RunwayCollisionHandler bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(60): SetActive=3, Lifecycle=3, GameObject=3, Collision/Force=2, Collider=2, Transform=1 | ksp: GameEvents=6 | fan_in=0
- methods(12): Awake, Disable, OnAllSectionsLoaded, OnCollisionEnter, OnDestroy, OnSceneLoadRequested, OnSectionLoaded, ReEnable, Reset, Start, onStructureCollapsing, onStructureRepaired
- auto: B / Physics

## Assembly-CSharp/SafeAbortBackgroundWorker.cs
- L=38 kind=class type=SafeAbortBackgroundWorker bases=BackgroundWorker anc=BackgroundWorker ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(2): Abort, OnDoWork
- auto: A / Core

## Assembly-CSharp/SandCastleLogic.cs
- L=14 kind=MonoBehaviour type=SandCastleLogic bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(68): MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=0
- methods(1): Start
- auto: B / Scene

## Assembly-CSharp/SaveMode.cs
- L=7 kind=enum type=SaveMode bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=39
- methods(0): 
- auto: A / Save

## Assembly-CSharp/SavingBtnSoftLock.cs
- L=15 kind=MonoBehaviour type=SavingBtnSoftLock bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(80): EventSystems=4, MonoBehaviour=1 | ksp: InputLockManager=2 | fan_in=0
- methods(2): OnPointerEnter, OnPointerExit
- auto: C / UI

## Assembly-CSharp/ScalarModuleSetHandler.cs
- L=55 kind=class type=ScalarModuleSetHandler bases=IConfigNode anc=IConfigNode ns=-
- unity(0): none | ksp: ConfigNode=2, Part=1 | fan_in=0
- methods(4): Initialize, Load, Save, SetFXModules
- auto: A / Part

## Assembly-CSharp/ScaledCamera.cs
- L=68 kind=MonoBehaviour type=ScaledCamera bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(84): Camera=6, Transform=5, Lifecycle=3, MonoBehaviour=1, GetComponent=1 | ksp: ScaledSpace=1, FlightCamera=1 | fan_in=13
- methods(8): Awake, LateUpdate, OnDestroy, ResetFoV, ResetTarget, SetCameraClearFlag, SetFoV, SetTarget
- auto: C / Camera

## Assembly-CSharp/ScaledFloatingOrigin.cs
- L=11 kind=MonoBehaviour type=ScaledFloatingOrigin bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): Transform=3, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(1): FixedUpdate
- auto: B / Celestials

## Assembly-CSharp/ScaledMovement.cs
- L=88 kind=MonoBehaviour type=ScaledMovement bases=MapObject anc=MapObject/MonoBehaviour ns=-
- unity(69): Transform=7, GetComponent=3, GameObject=3 | ksp: Vessel=4, ScaledSpace=4, CelestialBody=3, FlightGlobals=1 | fan_in=8
- methods(2): Create, OnLateUpdate
- auto: B / Celestials

## Assembly-CSharp/ScaledSpace.cs
- L=178 kind=MonoBehaviour type=ScaledSpace bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(67): GameObject=8, Transform=5, Lifecycle=4, SetActive=2, Instantiate/Destroy=2, UnityEngine.UI=1 | ksp: Vector3d=8, ScaledSpace=2, CelestialBody=1 | fan_in=35
- methods(10): AddScaledSpaceObject, Awake, LateUpdate, LocalToScaledSpace, OnDestroy, RemoveScaledSpaceObject, ScaledToLocalSpace, Start, Toggle, ToggleAll
- auto: B / Celestials

## Assembly-CSharp/ScaledSpaceFader.cs
- L=121 kind=MonoBehaviour type=ScaledSpaceFader bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(89): Shader=8, Renderer=6, Material=5, Transform=3, Lifecycle=3, GetComponent=2 | ksp: FlightGlobals=3, GameSettings=2, Vector3d=1, MapView=1, CelestialBody=1 | fan_in=3
- methods(5): GetAngularSize, OnDestroy, Reset, Start, Update
- auto: C / Rendering

## Assembly-CSharp/ScaledSun.cs
- L=26 kind=MonoBehaviour type=ScaledSun bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(67): Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: none | fan_in=1
- methods(2): Awake, OnDestroy
- auto: C / Rendering

## Assembly-CSharp/ScenarioAchievements.cs
- L=330 kind=ScenarioModule type=ScenarioAchievements bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(30): Lifecycle=2, Instantiate/Destroy=2, Coroutine=2 | ksp: GameEvents=24, Vessel=11, HighLogic=7, CelestialBody=5, Part=4 | fan_in=0
- methods(21): CheckAchievement, EndInTutorials, GetAchievementComplete, OnAwake, OnContractCompleted, OnCrewEVA, OnDestroy, OnFlagPlant, OnFlyby, OnLanding, OnLaunch, OnLoad, OnOrbit, OnPartsCoupled, OnReturnFromOrbit, OnSave, OnTargetedLanding, OnVesselRecovered, SetAchievementComplete, SetProvider, Start
- auto: A / Career

## Assembly-CSharp/ScenarioCreationOptions.cs
- L=21 kind=enum type=ScenarioCreationOptions bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=26
- methods(0): 
- auto: A / Save

## Assembly-CSharp/ScenarioCustomWaypoints.cs
- L=132 kind=ScenarioModule type=ScenarioCustomWaypoints bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(28): Lifecycle=1, Instantiate/Destroy=1 | ksp: ConfigNode=8, GameEvents=4, ScenarioModule=1, FlightGlobals=1 | fan_in=5
- methods(7): AddWaypoint, OnAwake, OnDestroy, OnLoad, OnSave, ProcessWaypoint, RemoveWaypoint
- auto: A / Misc

## Assembly-CSharp/ScenarioDestructibles.cs
- L=253 kind=ScenarioModule type=ScenarioDestructibles bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(22): Lifecycle=1, Instantiate/Destroy=1 | ksp: ConfigNode=9, ScenarioModule=1 | fan_in=3
- methods(14): AddRef, AddTo, GetDamage, GetFacility, GetFacilityDamage, Load, OnAwake, OnDestroy, OnLoad, OnSave, RegisterDestructible, RemoveFrom, Save, UnregisterDestructible
- auto: A / Career

## Assembly-CSharp/ScenarioDiscoverableObjects.cs
- L=390 kind=ScenarioModule type=ScenarioDiscoverableObjects bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(43): GameObject=5, Coroutine=3, Lifecycle=2, Instantiate/Destroy=1 | ksp: GameEvents=6, FlightGlobals=6, ConfigNode=6, Vessel=4, Orbit=4 | fan_in=6
- methods(22): DebugSpawnProbability, ExpireDiscoveredObjects, GetRandomDuration, GetSizeCurveBasedClass, LoadGameDBSettings, OnAwake, OnDestroy, OnKnowledgeChanged, OnLoad, OnSave, OnVesselDestroy, ReachedBody, SaveIDList, SpawnAsteroid, SpawnComet, SpawnDaemon, SpawnDresAsteroid, SpawnHomeAsteroid, SpawnLastAsteroid, Start, UpdateSpaceObjects, loadIDList
- auto: B / Celestials

## Assembly-CSharp/ScenarioLoadDialog.cs
- L=578 kind=MonoBehaviour type=ScenarioLoadDialog bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(76): GameObject=20, Texture=14, UnityEngine.UI=12, SetActive=9, Transform=6, Instantiate/Destroy=6 | ksp: Localizer=13, Game=2, ConfigNode=2, GameDatabase=1, FlightGlobals=1 | fan_in=1
- methods(27): BuildList, CanRestart, CancelLoadGame, ClearListItems, ConfirmLoadGame, ConfirmRestart, Create, CreateWidget, DismissRestartGame, FinishedCallback, GatherSaveFiles, GatherScenarioInfo, LoadBannerTexture, LoadPath, OnBtnCancel, OnBtnScnLink, OnButtonLoad, OnButtonRestart, OnDestroy, OnSelectionChanged, SetBanner, SetExplicitNavigation, SetHidden, SetLocked ...
- auto: C / Save

## Assembly-CSharp/ScenarioModule.cs
- L=87 kind=ScenarioModule type=ScenarioModule bases=MonoBehaviour,IConfigNode anc=MonoBehaviour ns=-
- unity(46): MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: ConfigNode=4, ScenarioModule=1 | fan_in=27
- methods(7): Awake, Load, ModularSetup, OnAwake, OnLoad, OnSave, Save
- auto: A / Modding

## Assembly-CSharp/ScenarioNewGameIntro.cs
- L=211 kind=ScenarioModule type=ScenarioNewGameIntro bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(18): none | ksp: Localizer=34, HighLogic=9, InputLockManager=6, GameEvents=2, ConfigNode=2 | fan_in=0
- methods(10): EditorTutorialSetup, KSCtutorialSetup, OnAssetSetup, OnAwake, OnLoad, OnOnDestroy, OnSave, OnTutorialSetup, TSTutorialSetup, onLeavingScene
- auto: A / Tutorials

## Assembly-CSharp/ScenarioRunner.cs
- L=213 kind=MonoBehaviour type=ScenarioRunner bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(45): Instantiate/Destroy=4, Lifecycle=2, MonoBehaviour=1, GetComponent=1, GameObject=1 | ksp: ScenarioModule=10, GameEvents=4, ConfigNode=3, HighLogic=2 | fan_in=6
- methods(13): AddMainMenuScenarios, AddModule, Awake, ClearModules, GetLoadedModules, GetUpdatedProtoModules, LoadModules, OnDestroy, OnGameSceneLoadRequested, RemoveModule, SetProtoModules, UpdateModules, onLevelWasLoaded
- auto: B / Modding

## Assembly-CSharp/ScenarioUpgradeableFacilities.cs
- L=402 kind=ScenarioModule type=ScenarioUpgradeableFacilities bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(31): Transform=2, GetComponent=2, Lifecycle=1, Instantiate/Destroy=1, Application.=1 | ksp: ConfigNode=19, Localizer=9, HighLogic=4, GameEvents=2, Game=2 | fan_in=54
- methods(21): AddRef, CheatFacilities, CleanUpUnmanagedData, GetFacilityLevel, GetFacilityLevelCount, GetFacilityName, GetLevel, GetLevelCount, IsLaunchPad, IsRunway, Load, OnAwake, OnDestroy, OnLoad, OnSave, RegisterUpgradeable, Save, SlashSanitize, UnregisterUpgradeable, getInitialMissionState, getInitialState
- auto: B / Career

## Assembly-CSharp/SceneTransitionMatrix.cs
- L=22 kind=class type=SceneTransitionMatrix bases=- anc=- ns=-
- unity(21): none | ksp: none | fan_in=1
- methods(1): GetTransitionValue
- auto: A / Scene

## Assembly-CSharp/ScienceData.cs
- L=100 kind=class type=ScienceData bases=IConfigNode anc=IConfigNode ns=-
- unity(0): none | ksp: ConfigNode=3, ResearchAndDevelopment=2 | fan_in=21
- methods(3): CopyOf, Load, Save
- auto: A / Science

## Assembly-CSharp/ScienceExperiment.cs
- L=114 kind=class type=ScienceExperiment bases=- anc=- ns=-
- unity(3): none | ksp: ConfigNode=4, ResearchAndDevelopment=1, CelestialBody=1 | fan_in=24
- methods(5): BiomeIsRelevantWhile, IsAvailableWhile, IsUnlocked, Load, Save
- auto: A / Science

## Assembly-CSharp/ScienceLabSearch.cs
- L=203 kind=class type=ScienceLabSearch bases=- anc=- ns=-
- unity(2): none | ksp: Localizer=12, Vessel=4, Part=1 | fan_in=3
- methods(1): PostErrorToScreen
- auto: A / Science

## Assembly-CSharp/ScienceNodeTutorialStep.cs
- L=138 kind=class type=ScienceNodeTutorialStep bases=IntermediateTutorialPageStep anc=IntermediateTutorialPageStep/TutorialPageConfig ns=-
- unity(26): GameObject=6 | ksp: GameEvents=5, Orbit=1 | fan_in=1
- methods(13): AddTutorialStepConfig, FilterScienceNodeCriteria, IsScienceLinked, OnEnterDragScienceEventNode, OnEnterDragScienceEventNodeHelper, OnEnterLinkScienceEventNode, OnLeaveDragScienceEventNodeHelper, OnLeaveOnEnterDragScienceEventNode, OnLeaveOnEnterLinkScienceEventNode, OnNodeEventScienceTuto, OnSelectDuringDragScienceTuto, OnUpdateDragScienceEventNodeHelper, OnUpdateLinkScienceEventNode
- auto: B / Tutorials

## Assembly-CSharp/ScienceSubject.cs
- L=174 kind=class type=ScienceSubject bases=IConfigNode anc=IConfigNode ns=-
- unity(2): none | ksp: ScienceSubject=6, Localizer=4, ConfigNode=3, CelestialBody=3 | fan_in=24
- methods(5): HasPartialIDstring, IsFromBody, IsFromSituation, Load, Save
- auto: A / Science

## Assembly-CSharp/ScienceUtil.cs
- L=574 kind=class type=ScienceUtil bases=- anc=- ns=-
- unity(4): none | ksp: Localizer=45, Vessel=10, CelestialBody=10, FlightGlobals=9, ProtoCrewMember=8 | fan_in=24
- methods(18): BiomeIsUnlisted, CrewListHasEffect, CrewListHasType, GenerateLocalizedTitle, GenerateScienceSubjectRecoveryTitle, GenerateScienceSubjectTitle, GetBestTransmitter, GetBiomedisplayName, GetExperimentBiome, GetExperimentBiomeLocalized, GetExperimentBodyName, GetExperimentFieldsFromScienceID, GetExperimentSituation, GetLabScore, GetTransmitterScore, RequiredUsageExternalAvailable, RequiredUsageInternalAvailable, generateRecoveryLocalizedTitle
- auto: A / Science

## Assembly-CSharp/ScienceWidget.cs
- L=38 kind=MonoBehaviour type=ScienceWidget bases=CurrencyWidget anc=CurrencyWidget/MonoBehaviour ns=-
- unity(57): Lifecycle=2, TMPro=1 | ksp: ResearchAndDevelopment=3, GameEvents=2, HighLogic=1 | fan_in=1
- methods(5): Awake, DelayedStart, OnAboutToStart, OnDestroy, onScienceChange
- auto: B / Science

## Assembly-CSharp/Scientist.cs
- L=79 kind=MonoBehaviour type=Scientist bases=SpaceCenterCrew anc=SpaceCenterCrew/MonoBehaviour ns=-
- unity(35): Lifecycle=2 | ksp: none | fan_in=1
- methods(3): SetAnimation, Start, Update
- auto: B / Kerbals

## Assembly-CSharp/ScreenMessage.cs
- L=30 kind=class type=ScreenMessage bases=- anc=- ns=-
- unity(17): none | ksp: none | fan_in=20
- methods(0): 
- auto: A / FlightUI

## Assembly-CSharp/ScreenMessages.cs
- L=451 kind=MonoBehaviour type=ScreenMessages bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(66): UnityEngine.UI=16, Time.=8, Transform=6, GameObject=6, SetActive=5, Screen/Cursor=4 | ksp: GameEvents=4, Localizer=3, GameSettings=2 | fan_in=122
- methods(14): Awake, CreateTextInstance, DestroyMessage, DestroyMessages, OnDestroy, OnGameSceneLoadRequested, OnLevelLoaded, PostMessage, PostScreenMessage, RemoveMessage, SendStateMessage, SetParent, Start, Update
- auto: C / FlightUI

## Assembly-CSharp/ScreenMessagesText.cs
- L=12 kind=MonoBehaviour type=ScreenMessagesText bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(55): TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=2
- methods(1): SetText
- auto: B / FlightUI

## Assembly-CSharp/ScreenMessageStyle.cs
- L=15 kind=enum type=ScreenMessageStyle bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=101
- methods(0): 
- auto: A / FlightUI

## Assembly-CSharp/ScreenShot.cs
- L=111 kind=MonoBehaviour type=ScreenShot bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(64): Application.=12, Lifecycle=3, MonoBehaviour=2, Transform=1, Camera=1 | ksp: FlightCamera=9, GameEvents=8, GameSettings=3, HighLogic=1 | fan_in=0
- methods(6): HideUI, OnDestroy, OnGameUnpause, ShowUI, Start, Update
- auto: C / Rendering

## Assembly-CSharp/ScreenSpaceObjectScaling.cs
- L=41 kind=MonoBehaviour type=ScreenSpaceObjectScaling bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(85): Transform=4, Screen/Cursor=2, Lifecycle=2, Camera=2, Time.=1, MonoBehaviour=1 | ksp: none | fan_in=2
- methods(3): GetScreenSize, LateUpdate, Start
- auto: C / Rendering

## Assembly-CSharp/ScrollRectDragOverride.cs
- L=28 kind=MonoBehaviour type=ScrollRectDragOverride bases=ScrollRect anc=ScrollRect ns=-
- unity(80): EventSystems=3, UnityEngine.UI=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(4): Awake, OnBeginDrag, OnDrag, OnEndDrag
- auto: C / UI

## Assembly-CSharp/ScrollRectPixelPerfectHandler.cs
- L=51 kind=MonoBehaviour type=ScrollRectPixelPerfectHandler bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(75): UnityEngine.UI=3, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=0
- methods(3): DisablePixelPerfect, Start, Update
- auto: C / UI

## Assembly-CSharp/SerenityAboutDialog.cs
- L=80 kind=MonoBehaviour type=SerenityAboutDialog bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(85): UnityEngine.UI=4, GameObject=3, Transform=2, Lifecycle=2, Instantiate/Destroy=2, Texture=1 | ksp: InputLockManager=2, GameSettings=2 | fan_in=1
- methods(7): Create, Dismiss, OnCloseButton, OnDontShowAgainToggle, OnMoreInfoButton, Start, Update
- auto: C / DLC-BreakingGround

## Assembly-CSharp/ShaderTimeOffset.cs
- L=28 kind=MonoBehaviour type=ShaderTimeOffset bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(91): Renderer=2, Material=2, Lifecycle=2, Time.=1, Shader=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(3): Reset, Start, Update
- auto: C / Rendering

## Assembly-CSharp/ShipTemplate.cs
- L=251 kind=class type=ShipTemplate bases=- anc=- ns=-
- unity(7): none | ksp: ConfigNode=13, MissionSystem=6, HighLogic=3, ResearchAndDevelopment=2, Game=2 | fan_in=17
- methods(5): GetPartId, GetPartName, GetShipSize, LoadShip, SetIfControllable
- auto: A / Save

## Assembly-CSharp/SimpleOrbitController.cs
- L=31 kind=MonoBehaviour type=SimpleOrbitController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(74): Transform=2, Lifecycle=2, Time.=1, MonoBehaviour=1 | ksp: Vector3d=3, QuaternionD=2 | fan_in=0
- methods(2): FixedUpdate, Start
- auto: C / Camera

## Assembly-CSharp/Simplex.cs
- L=380 kind=class type=Simplex bases=- anc=- ns=-
- unity(1): none | ksp: Vector3d=2 | fan_in=15
- methods(6): SetupPermTable, dot, fastfloor, noise, noiseNormalized, value
- auto: A / Terrain

## Assembly-CSharp/SimplexNoise.cs
- L=354 kind=static class type=SimplexNoise bases=- anc=- ns=-
- unity(0): none | ksp: Vector3d=2 | fan_in=1
- methods(4): SetupPermTable, dot, fastfloor, noise
- auto: A / Terrain

## Assembly-CSharp/SiteNode.cs
- L=141 kind=MonoBehaviour type=SiteNode bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(41): Lifecycle=3, MonoBehaviour=1, GetComponent=1, GameObject=1 | ksp: GameEvents=6, Vector3d=1, ScaledSpace=1, Localizer=1 | fan_in=1
- methods(13): Awake, LateUpdate, OnDestroy, OnMapEntered, OnMapExited, OnSceneLoaded, OnUpdateNodeCaption, OnUpdateNodeIcon, OnUpdateNodePosition, OnUpdateType, SetupMapNode, SetupWaypoint, Spawn
- auto: B / Scene

## Assembly-CSharp/SkySphereControl.cs
- L=57 kind=MonoBehaviour type=SkySphereControl bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(96): Shader=9, Transform=5, Material=3, Lifecycle=3, Renderer=1, MonoBehaviour=1 | ksp: Vector3d=2, ScaledSpace=2, FlightGlobals=2, QuaternionD=1, Planetarium=1 | fan_in=0
- methods(3): FixedUpdate, Start, Update
- auto: C / Rendering

## Assembly-CSharp/SliderFocusType.cs
- L=6 kind=enum type=SliderFocusType bases=- anc=- ns=-
- unity(29): UnityEngine.UI=1 | ksp: none | fan_in=7
- methods(0): 
- auto: C / UI

## Assembly-CSharp/SmokeTrailControl.cs
- L=54 kind=MonoBehaviour type=SmokeTrailControl bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(83): Shader=2, Renderer=2, Material=2, Lifecycle=2, MonoBehaviour=1, GetComponent=1 | ksp: FlightGlobals=2, HighLogic=1 | fan_in=2
- methods(2): Start, Update
- auto: C / Rendering

## Assembly-CSharp/SmoothDisposer.cs
- L=26 kind=MonoBehaviour type=SmoothDisposer bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(74): Lifecycle=2, Instantiate/Destroy=2, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(2): Awake, LateUpdate
- auto: B / Core

## Assembly-CSharp/SpaceCenter.cs
- L=86 kind=MonoBehaviour type=SpaceCenter bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(74): Transform=4, Lifecycle=3, GameObject=3, SetActive=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: Vector3d=4, GameEvents=2, CelestialBody=2, Part=1 | fan_in=36
- methods(5): Awake, GreatCircleDistance, OnDestroy, OnGameSceneLoaded, Start
- auto: B / Scene

## Assembly-CSharp/SpaceCenterBuildingCollider.cs
- L=43 kind=MonoBehaviour type=SpaceCenterBuildingCollider bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(68): Instantiate/Destroy=2, GameObject=2, MonoBehaviour=1, Lifecycle=1 | ksp: GameEvents=3 | fan_in=1
- methods(5): OnDestroy, OnGameSceneChange, OnMouseExit, OnMouseOver, Setup
- auto: B / Scene

## Assembly-CSharp/SpaceCenterCamera.cs
- L=339 kind=MonoBehaviour type=SpaceCenterCamera bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(89): Input.=34, KeyCode=29, Transform=13, Time.=7, GameObject=3, Screen/Cursor=2 | ksp: Vector3d=7, QuaternionD=5, PQS=5, FlightCamera=3 | fan_in=0
- methods(19): Altitude, Elevate, GetSpeed, InputCamera, InputMovement, InputSpeedFactor, Pitch, Reset, ResetCamera, Rotate, SetPosition, Start, SwitchMode, Translate, Update, UpdateTransform, UpdateTransformOverview, UpdateTransformRover, Zoom
- auto: C / Camera

## Assembly-CSharp/SpaceCenterCamera2.cs
- L=440 kind=MonoBehaviour type=SpaceCenterCamera2 bases=MonoBehaviour,IKSPCamera anc=MonoBehaviour ns=-
- unity(83): Input.=27, KeyCode=23, Transform=16, Time.=6, GameObject=5, Lifecycle=3 | ksp: PQS=5, GameSettings=5, FlightCamera=5, InputLockManager=2, GameEvents=2 | fan_in=1
- methods(30): Altitude, Elevate, GetCameraTransform, GetPivot, GetSpeed, GetZoom, InputCamera, InputMovement, InputSpeedFactor, OnDestroy, OnNavigatorHandoff, OnNavigatorRequestControl, OnNavigatorTakeOver, OnSceneSwitch, Pitch, Reset, ResetCamera, Rotate, SetCamCoordsFromPosition, Start, SwitchMode, Translate, Update, UpdateTransform ...
- auto: C / Camera

## Assembly-CSharp/SpaceCenterCrew.cs
- L=75 kind=MonoBehaviour type=SpaceCenterCrew bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(66): Transform=4, Rigidbody=1, MonoBehaviour=1, GameObject=1, Animation=1 | ksp: none | fan_in=9
- methods(2): CrewMovement, SetAnimation
- auto: B / Kerbals

## Assembly-CSharp/SpaceCenterFacility.cs
- L=23 kind=enum type=SpaceCenterFacility bases=- anc=- ns=-
- unity(0): none | ksp: ResearchAndDevelopment=1 | fan_in=69
- methods(0): 
- auto: A / Scene

## Assembly-CSharp/SpaceCenterMain.cs
- L=89 kind=MonoBehaviour type=SpaceCenterMain bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): GameObject=9, SetActive=2, MonoBehaviour=1, Lifecycle=1, Coroutine=1 | ksp: InputLockManager=6, GameEvents=4, HighLogic=3, Planetarium=1, Game=1 | fan_in=0
- methods(5): OnApplicationFocus, OnDestroy, OnGameSceneChange, Start, onInputLockModified
- auto: B / Scene

## Assembly-CSharp/SpaceCenterSurfaceObject.cs
- L=57 kind=MonoBehaviour type=SpaceCenterSurfaceObject bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(61): Lifecycle=2, GameObject=2, MonoBehaviour=1, Coroutine=1 | ksp: GameEvents=4, HighLogic=2, CelestialBody=2 | fan_in=0
- methods(5): Awake, OnDestroy, OnGameSceneLoadRequested, OnGameSceneLoaded, Setup
- auto: B / Scene

## Assembly-CSharp/SpaceNavigatorCamera.cs
- L=163 kind=MonoBehaviour type=SpaceNavigatorCamera bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(67): GameObject=4, SceneManager=3, Lifecycle=3, Raycast/Query=2, Transform=1, Physics.=1 | ksp: GameSettings=4, HighLogic=2 | fan_in=2
- methods(11): LateUpdate, OnCameraRequestControl, OnCameraUpdate, OnCameraWantsControl, OnDestroy, OnGetControl, OnLevelLoaded, OnSceneLoaded, Start, clampToRadius, getNextTranslation
- auto: C / Camera

## Assembly-CSharp/SpaceNavigatorFreeCamera.cs
- L=68 kind=MonoBehaviour type=SpaceNavigatorFreeCamera bases=SpaceNavigatorCamera anc=SpaceNavigatorCamera/MonoBehaviour ns=-
- unity(66): Time.=8, Transform=3 | ksp: none | fan_in=0
- methods(3): OnCameraUpdate, OnCameraWantsControl, OnGetControl
- auto: C / Camera

## Assembly-CSharp/SpaceNavigatorLocalCamera.cs
- L=74 kind=MonoBehaviour type=SpaceNavigatorLocalCamera bases=SpaceNavigatorCamera anc=SpaceNavigatorCamera/MonoBehaviour ns=-
- unity(59): Time.=7, Transform=1 | ksp: none | fan_in=0
- methods(4): OnCameraUpdate, OnCameraWantsControl, OnGetControl, clampToBounds
- auto: C / Camera

## Assembly-CSharp/SpacePlaneHangarBuilding.cs
- L=52 kind=MonoBehaviour type=SpacePlaneHangarBuilding bases=SpaceCenterBuilding anc=SpaceCenterBuilding/MonoBehaviour ns=-
- unity(31): MonoBehaviour=1 | ksp: HighLogic=8, MissionSystem=3, Localizer=3, Game=3, InputLockManager=2 | fan_in=0
- methods(3): IsOpen, OnClicked, OnPopupWarningDismiss
- auto: B / Scene

## Assembly-CSharp/SPHCamera.cs
- L=315 kind=MonoBehaviour type=SPHCamera bases=MonoBehaviour,IKSPCamera anc=MonoBehaviour ns=-
- unity(85): Transform=25, Input.=15, Time.=11, KeyCode=10, Lifecycle=3, GameObject=2 | ksp: GameSettings=22, InputLockManager=4, GameEvents=4, EditorLogic=3, ShipConstruct=1 | fan_in=1
- methods(18): GetCameraTransform, GetPivot, OnDestroy, OnEditorRestart, OnEditorShipLoad, OnEnable, OnNavigatorHandoff, OnNavigatorRequestControl, OnNavigatorTakeOver, PlaceCamera, ResetCamera, SetCamCoordsFromPosition, SetInitialCameraPosition, Start, Update, getPitch, getReferenceFrame, getYaw
- auto: C / Camera

## Assembly-CSharp/SphereBaseSO.cs
- L=396 kind=ScriptableObject type=SphereBaseSO bases=ScriptableObject anc=ScriptableObject ns=-
- unity(22): Mesh=3, Time.=2, ScriptableObject=1 | ksp: none | fan_in=2
- methods(13): AddVertex, CalculateNormals, CalculateSize, Create, CreateMesh, CreateSphere, GetMidPoint, HasVertex, LinkEdges, LinkSphere, LinkTriangle, LinkTriangles, LinkVerts
- auto: A / Terrain

## Assembly-CSharp/SphereVolume.cs
- L=76 kind=MonoBehaviour type=SphereVolume bases=VolumetricObjectBase anc=VolumetricObjectBase/MonoBehaviour ns=-
- unity(74): Shader=8, Lifecycle=1 | ksp: none | fan_in=1
- methods(5): HasChanged, OnEnable, PopulateShaderName, SetChangedValues, UpdateVolume
- auto: C / Rendering

## Assembly-CSharp/Splashdown.cs
- L=34 kind=MonoBehaviour type=Splashdown bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(86): Lifecycle=2, Instantiate/Destroy=2, GameObject=2, SendMessage/Invoke=1, MonoBehaviour=1, GetComponent=1 | ksp: GameSettings=1 | fan_in=0
- methods(3): Awake, Start, endSplashBurst
- auto: B / Physics

## Assembly-CSharp/StackIconGrouping.cs
- L=8 kind=enum type=StackIconGrouping bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / Vessel

## Assembly-CSharp/StageGroupInfoItem.cs
- L=73 kind=MonoBehaviour type=StageGroupInfoItem bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(76): UnityEngine.UI=5, TMPro=2, GetComponent=2, MonoBehaviour=1, Lifecycle=1 | ksp: GameSettings=2, HighLogic=1 | fan_in=1
- methods(4): Awake, OnUpdate, Setup, UpdateValue
- auto: C / Vessel

## Assembly-CSharp/StandaloneInputModuleCustom.cs
- L=9 kind=MonoBehaviour type=StandaloneInputModuleCustom bases=StandaloneInputModule anc=StandaloneInputModule ns=-
- unity(44): EventSystems=1 | ksp: none | fan_in=1
- methods(1): GetLastPointerEventDataPublic
- auto: B / Input

## Assembly-CSharp/SteamWorkshopExportDialog.cs
- L=324 kind=MonoBehaviour type=SteamWorkshopExportDialog bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(78): UnityEngine.UI=9, TMPro=9, SetActive=9, GameObject=9, Transform=4, Lifecycle=2 | ksp: GameDatabase=2, Localizer=1 | fan_in=2
- methods(10): CloseDialog, OnCancel, OnConfirm, OnDestroy, OnModsList, OnToggle, Spawn, Start, ToggleSetup, onBtnLegalAgreement
- auto: C / Steam

## Assembly-CSharp/SteeringObject.cs
- L=86 kind=class type=SteeringObject bases=- anc=- ns=-
- unity(33): Transform=3 | ksp: none | fan_in=0
- methods(1): FindAlignmentAxis
- auto: A / Vessel

## Assembly-CSharp/StoredPart.cs
- L=99 kind=class type=StoredPart bases=- anc=- ns=-
- unity(4): none | ksp: ConfigNode=5, ProtoPartSnapshot=3 | fan_in=6
- methods(3): Copy, Load, Save
- auto: A / Kerbals

## Assembly-CSharp/StringBuilderCache.cs
- L=49 kind=static class type=StringBuilderCache bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=29
- methods(4): Acquire, Format, Release, ToStringAndRelease
- auto: A / Core

## Assembly-CSharp/StringExtensions.cs
- L=18 kind=static class type=StringExtensions bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(1): GetHashCode_Net35
- auto: A / Core

## Assembly-CSharp/StringTypeParser.cs
- L=16 kind=class type=StringTypeParser bases=ITypeParser anc=ITypeParser ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(2): Convert, Parse
- auto: A / Modding

## Assembly-CSharp/SuitButton.cs
- L=182 kind=MonoBehaviour type=SuitButton bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(80): UnityEngine.UI=7, Transform=4, EventSystems=4, SetActive=3, Lifecycle=3, GameObject=3 | ksp: GameEvents=5, ProtoCrewMember=3 | fan_in=4
- methods(12): GenerateButtonName, OnButtonClicked, OnDestroy, OnHelmetNeckringSelection, OnPointerEnter, OnPointerExit, OpenSuitLightPicker, Reset, RotatePreview, Select, Start, Update
- auto: C / Kerbals

## Assembly-CSharp/SuitCombo.cs
- L=43 kind=class type=SuitCombo bases=- anc=- ns=-
- unity(39): Texture=2 | ksp: none | fan_in=4
- methods(0): 
- auto: A / Kerbals

## Assembly-CSharp/SuitCombos.cs
- L=203 kind=MonoBehaviour type=SuitCombos bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(62): UnityEngine.UI=2, Transform=2, Screen/Cursor=2, Material=2, Lifecycle=2, Texture=1 | ksp: ConfigNode=5, GameSettings=2, GameEvents=2, ProtoCrewMember=1, GameDatabase=1 | fan_in=4
- methods(8): CreateHelmetSuitPickerWindow, GetCombo, GetDefaultTexture, GetSuitCombos, OnDestroy, Start, UpdateTextureScale, ValidateCombo
- auto: B / Kerbals

## Assembly-CSharp/SuitLightColorPicker.cs
- L=64 kind=MonoBehaviour type=SuitLightColorPicker bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(94): UnityEngine.UI=9, GetComponent=3, Transform=2, Shader=2, Lifecycle=2, GameObject=2 | ksp: ProtoCrewMember=1 | fan_in=3
- methods(4): OnColorChange, OnDestroy, OnPickColorButton, Start
- auto: C / Kerbals

## Assembly-CSharp/Sun.cs
- L=324 kind=MonoBehaviour type=Sun bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(63): QualitySettings/SystemInfo=18, Transform=7, Light=6, Lifecycle=4, GetComponent=2, MonoBehaviour=1 | ksp: Vector3d=14, CelestialBody=5, ScaledSpace=4, FlightGlobals=4, GameSettings=2 | fan_in=57
- methods(7): Awake, GameSceneLoadRequested, GetLocalTimeAtPosition, LateUpdate, OnDestroy, Start, SunlightEnabled
- auto: B / Celestials

## Assembly-CSharp/SunCoronas.cs
- L=45 kind=MonoBehaviour type=SunCoronas bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(84): Transform=6, Lifecycle=2, Time.=1, MonoBehaviour=1, Camera=1 | ksp: none | fan_in=3
- methods(2): FixedUpdate, Start
- auto: C / Rendering

## Assembly-CSharp/SunFlare.cs
- L=73 kind=MonoBehaviour type=SunFlare bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(81): Transform=6, Lifecycle=3, GetComponent=3, Collider=2, Renderer=1, MonoBehaviour=1 | ksp: Vector3d=6, ScaledSpace=3, HighLogic=2, CelestialBody=2 | fan_in=4
- methods(4): Awake, LateUpdate, OnDestroy, SunlightEnabled
- auto: C / Rendering

## Assembly-CSharp/SunShaderController.cs
- L=113 kind=MonoBehaviour type=SunShaderController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(94): AnimationCurve=20, Shader=13, Material=5, Texture=3, Renderer=3, Lifecycle=2 | ksp: Planetarium=2 | fan_in=0
- methods(4): OnDestroy, Start, Update, UpdateRampMap
- auto: C / Rendering

## Assembly-CSharp/SurfaceFX.cs
- L=274 kind=MonoBehaviour type=SurfaceFX bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(70): ParticleSystem=17, Lifecycle=4, Transform=2, Time.=2, GameObject=2, MonoBehaviour=1 | ksp: FloatingOrigin=6, FlightGlobals=1 | fan_in=1
- methods(10): AddSource, Awake, FindNearestFX, GetWeightedAvgVector, LateUpdate, OnDestroy, RemoveSource, Start, Terminate, UpdateParticle
- auto: C / Rendering

## Assembly-CSharp/SurfaceObject.cs
- L=206 kind=MonoBehaviour type=SurfaceObject bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(77): Transform=25, GameObject=5, GetComponent=4, Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: CelestialBody=5, Vector3d=2 | fan_in=3
- methods(9): Create, FixedUpdate, LateUpdate, PopToSceneRoot, ReturnToParent, Start, Terminate, Update, onUpdate
- auto: B / Terrain

## Assembly-CSharp/SuspensionLoadBalancer.cs
- L=86 kind=VesselModule type=SuspensionLoadBalancer bases=VesselModule anc=VesselModule/MonoBehaviour ns=-
- unity(32): Lifecycle=2 | ksp: GameEvents=2, VesselModule=1, Vessel=1 | fan_in=1
- methods(6): FixedUpdate, OnAwake, OnDestroy, OnStart, UpdateSuspensionModules, VesselChanged
- auto: A / PartModules-Wheels

## Assembly-CSharp/SVBoxSlider.cs
- L=103 kind=MonoBehaviour type=SVBoxSlider bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(77): UnityEngine.UI=5, Lifecycle=4, Texture=2, Instantiate/Destroy=2, GetComponent=2, Transform=1 | ksp: none | fan_in=0
- methods(7): Awake, HSVChanged, OnDestroy, OnDisable, OnEnable, RegenerateSVTexture, SliderChanged
- auto: C / UI

## Assembly-CSharp/SymmetryMethod.cs
- L=5 kind=enum type=SymmetryMethod bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=12
- methods(0): 
- auto: A / Vessel

## Assembly-CSharp/SystemInformation.cs
- L=17 kind=MonoBehaviour type=SystemInformation bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(67): Renderer=1, QualitySettings/SystemInfo=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(1): Awake
- auto: C / Debug

## Assembly-CSharp/TechPerk.cs
- L=21 kind=class type=TechPerk bases=ITechPerk anc=ITechPerk ns=-
- unity(29): Texture=1 | ksp: none | fan_in=0
- methods(2): GetPerkState, SetPerk
- auto: A / Science

## Assembly-CSharp/TemperatureModel.cs
- L=326 kind=class type=TemperatureModel bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(9): AddConstantStep, AddInitialConstantStep, AddInitialLinearStep, AddInitialLinearStepEquation, AddInitialQuadraticStep, AddLinearStep, AddQuadraticStep, clearIntervals, getTemperatureAtAltitude
- auto: A / Thermal

## Assembly-CSharp/test_particles.cs
- L=223 kind=MonoBehaviour type=test_particles bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(89): Input.=16, ParticleSystem=14, KeyCode=13, Time.=8, Lifecycle=3, Transform=2 | ksp: none | fan_in=0
- methods(5): LateUpdate, MouseDrag, SetPower, Start, Update
- auto: C / Rendering

## Assembly-CSharp/TestVehicleController.cs
- L=215 kind=MonoBehaviour type=TestVehicleController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(68): Joint=5, Rigidbody=4, Transform=2, Lifecycle=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(8): BoostCurve, ConfigurableJointUpdate, DistanceFromCoM, FixedUpdate, Start, SuspensionLoadUpdate, SuspensionSpringUpdate, updateMassDistribution
- auto: B / PartModules-Wheels

## Assembly-CSharp/TextButton3D.cs
- L=141 kind=MonoBehaviour type=TextButton3D bases=MonoBehaviour,IMouseEvents anc=MonoBehaviour ns=-
- unity(70): Material=6, Renderer=3, MonoBehaviour=2, GetComponent=2, Coroutine=2, Time.=1 | ksp: none | fan_in=0
- methods(9): GetInstance, Lock, OnMouseDown, OnMouseDrag, OnMouseEnter, OnMouseExit, OnMouseTap, OnMouseUp, Unlock
- auto: C / UI

## Assembly-CSharp/TextMeshQueue.cs
- L=126 kind=MonoBehaviour type=TextMeshQueue bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(56): TMPro=2, Instantiate/Destroy=2, UnityEngine.UI=1, Transform=1, MonoBehaviour=1, GameObject=1 | ksp: none | fan_in=3
- methods(4): AddLine, DestroyIfEmpty, RemoveAllLines, RemoveLine
- auto: C / UI

## Assembly-CSharp/TextProButton3D.cs
- L=189 kind=MonoBehaviour type=TextProButton3D bases=MonoBehaviour,IMouseEvents anc=MonoBehaviour ns=-
- unity(59): TMPro=4, Collider=3, MonoBehaviour=2, GetComponent=2, Coroutine=2, Time.=1 | ksp: none | fan_in=1
- methods(15): Awake, GetInstance, Highlight, IsButtonLocked, KeyStrokeLock, Lock, OnMouseDown, OnMouseDrag, OnMouseEnter, OnMouseExit, OnMouseTap, OnMouseUp, SetEnable, UnHighlight, Unlock
- auto: C / UI

## Assembly-CSharp/TextureMaterialProperty.cs
- L=17 kind=class type=TextureMaterialProperty bases=MaterialPropertyExtensions.MaterialProperty anc=MaterialProperty ns=-
- unity(74): Texture=5, Shader=1, Material=1 | ksp: none | fan_in=0
- methods(1): Apply
- auto: C / Rendering

## Assembly-CSharp/TextureScale.cs
- L=92 kind=class type=TextureScale bases=- anc=- ns=-
- unity(33): Texture=3 | ksp: none | fan_in=2
- methods(6): Bilinear, BilinearScale, ColorLerpUnclamped, Point, PointScale, Scale
- auto: C / Rendering

## Assembly-CSharp/TextureTools.cs
- L=163 kind=MonoBehaviour type=TextureTools bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(69): AnimationCurve=14, Texture=9, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(5): Add, Convert, EvaluateColor, EvaluateVector, Reset
- auto: C / Rendering

## Assembly-CSharp/TGAHeader.cs
- L=95 kind=class type=TGAHeader bases=- anc=- ns=-
- unity(4): none | ksp: none | fan_in=1
- methods(1): GetData
- auto: C / Rendering

## Assembly-CSharp/TGAImage.cs
- L=253 kind=class type=TGAImage bases=- anc=- ns=-
- unity(33): Texture=7 | ksp: none | fan_in=1
- methods(6): CreateTexture, ReadImage, ReadRTETrueColorImage, ReadRTETrueColorImageAlpha, ReadRTETrueColorImageNoAlpha, ReadTrueColorImage
- auto: C / Rendering

## Assembly-CSharp/TGAImageType.cs
- L=11 kind=enum type=TGAImageType bases=byte anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: C / Rendering

## Assembly-CSharp/ThermalLink.cs
- L=60 kind=class type=ThermalLink bases=- anc=- ns=-
- unity(3): none | ksp: Part=3, AttachNode=3 | fan_in=2
- methods(0): 
- auto: A / Thermal

## Assembly-CSharp/TiltWindow.cs
- L=29 kind=MonoBehaviour type=TiltWindow bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(86): Transform=2, Screen/Cursor=2, Lifecycle=2, Time.=1, MonoBehaviour=1, Input.=1 | ksp: none | fan_in=0
- methods(2): Start, Update
- auto: C / UI

## Assembly-CSharp/TimedObjectDestructor.cs
- L=22 kind=MonoBehaviour type=TimedObjectDestructor bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): Transform=1, SendMessage/Invoke=1, MonoBehaviour=1, Lifecycle=1, GameObject=1 | ksp: none | fan_in=0
- methods(2): Awake, DestroyNow
- auto: B / Core

## Assembly-CSharp/TimeInputFieldHandler.cs
- L=108 kind=class type=TimeInputFieldHandler bases=InputFieldHandler anc=InputFieldHandler ns=-
- unity(36): TMPro=5 | ksp: none | fan_in=1
- methods(4): ChangeInputMode, GetFormattedValue, ParseText, ParseUT
- auto: C / UI

## Assembly-CSharp/TimeOfDayAnimation.cs
- L=164 kind=MonoBehaviour type=TimeOfDayAnimation bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(76): Shader=5, Renderer=5, Material=5, Transform=3, Lifecycle=3, MonoBehaviour=1 | ksp: GameEvents=8 | fan_in=0
- methods(9): LateUpdate, MaterialPropertiesDirty, OnDestroy, SetColor, Start, UpdateInstances, UpdateRenderers, onDestructibleEvent, onFacilityEvent
- auto: C / Rendering

## Assembly-CSharp/TimeSinceTutorialStep.cs
- L=277 kind=class type=TimeSinceTutorialStep bases=IntermediateTutorialPageStep anc=IntermediateTutorialPageStep/TutorialPageConfig ns=-
- unity(22): GameObject=10 | ksp: GameEvents=4 | fan_in=1
- methods(21): AddTutorialStepConfig, CanHighLightStartNode, CanHighLightTime, CanHighlightOperator, FilterTimeNodeCriteria, HasTimeSinceNode, IsTimeNodeSetted, OnEnterDragTimeSinceNode, OnEnterDragTimeSinceNodeHelper, OnEnterSetupTimeNode, OnEnterSetupTimeNodeHelper, OnLeaveDragTimeSinceNode, OnLeaveDragTimeSinceNodeHelper, OnLeaveSetupTimeNode, OnLeaveSetupTimeNodeHelper, OnNodeFocused, OnSelectedGameObjectChange, OnUpdateSetupTimeNode, OnUpdateSetupTimeNodeHelper, TimeEqualOneDay, TimeStartNodeSetupped
- auto: B / Tutorials

## Assembly-CSharp/Timing0.cs
- L=56 kind=MonoBehaviour type=Timing0 bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): Lifecycle=3, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto: B / Scene

## Assembly-CSharp/Timing1.cs
- L=56 kind=MonoBehaviour type=Timing1 bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): Lifecycle=3, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto: B / Scene

## Assembly-CSharp/Timing2.cs
- L=56 kind=MonoBehaviour type=Timing2 bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): Lifecycle=3, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto: B / Scene

## Assembly-CSharp/Timing3.cs
- L=56 kind=MonoBehaviour type=Timing3 bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): Lifecycle=3, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto: B / Scene

## Assembly-CSharp/Timing4.cs
- L=56 kind=MonoBehaviour type=Timing4 bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): Lifecycle=3, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto: B / Scene

## Assembly-CSharp/Timing5.cs
- L=56 kind=MonoBehaviour type=Timing5 bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): Lifecycle=3, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto: B / Scene

## Assembly-CSharp/TimingFI.cs
- L=56 kind=MonoBehaviour type=TimingFI bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): Lifecycle=3, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto: B / Scene

## Assembly-CSharp/TimingManager.cs
- L=541 kind=MonoBehaviour type=TimingManager bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(47): Instantiate/Destroy=9, GameObject=9, GetComponent=8, Lifecycle=5, MonoBehaviour=1 | ksp: FlightIntegrator=7 | fan_in=10
- methods(12): Awake, FixedUpdate, FixedUpdateAdd, FixedUpdateRemove, LateUpdate, LateUpdateAdd, LateUpdateRemove, OnDestroy, Update, UpdateAction, UpdateAdd, UpdateRemove
- auto: B / Scene

## Assembly-CSharp/TimingPre.cs
- L=56 kind=MonoBehaviour type=TimingPre bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): Lifecycle=3, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(3): FixedUpdate, LateUpdate, Update
- auto: B / Scene

## Assembly-CSharp/TMPProUtil.cs
- L=83 kind=static class type=TMPProUtil bases=- anc=- ns=-
- unity(55): TMPro=4, UnityEngine.UI=3, GetComponent=1, GameObject=1 | ksp: none | fan_in=1
- methods(4): FontStyle, GetComponentByInstanceID, LineType, TextAlignment
- auto: C / UI

## Assembly-CSharp/TMProUtil.cs
- L=10 kind=class type=TMProUtil bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: C / UI

## Assembly-CSharp/TrackingStationBuilding.cs
- L=32 kind=MonoBehaviour type=TrackingStationBuilding bases=SpaceCenterBuilding anc=SpaceCenterBuilding/MonoBehaviour ns=-
- unity(21): none | ksp: HighLogic=7, Localizer=3, InputLockManager=2, Game=2 | fan_in=0
- methods(3): IsOpen, OnClicked, OnPopupWarningDismiss
- auto: B / Scene

## Assembly-CSharp/TrackingStationDishMovement.cs
- L=27 kind=MonoBehaviour type=TrackingStationDishMovement bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): Transform=2, Time.=1, MonoBehaviour=1, Lifecycle=1 | ksp: Planetarium=2 | fan_in=0
- methods(1): Update
- auto: B / CommNet

## Assembly-CSharp/TrackIR.cs
- L=353 kind=MonoBehaviour type=TrackIR bases=MonoBehaviour,IConfigNode anc=MonoBehaviour ns=-
- unity(28): Lifecycle=3, MonoBehaviour=1 | ksp: ConfigNode=8, GameSettings=4, GameEvents=4 | fan_in=8
- methods(12): Awake, FindInstance, GetAxis, Init, Load, OnDestroy, OnSceneLoaded, OnSettingsApplied, Save, Shutdown, TrackIRUpdate, Update
- auto: C / Camera

## Assembly-CSharp/TrackRigObject.cs
- L=87 kind=MonoBehaviour type=TrackRigObject bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): Lifecycle=4, Transform=3, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(6): Awake, FixedUpdate, Initialize, LateUpdate, Track, Update
- auto: C / Camera

## Assembly-CSharp/Trajectory.cs
- L=436 kind=class type=Trajectory bases=- anc=- ns=-
- unity(1): none | ksp: Vector3d=50, CelestialBody=33, Orbit=8, Planetarium=1 | fan_in=1
- methods(36): ConvertPointToLerped, ConvertPointToLocal, ConvertPointToLocalAtUT, ConvertPointToRelative, GetApoapsisLerped, GetApoapsisLocal, GetApoapsisLocalAtUT, GetApoapsisRelative, GetColors, GetPatchEndLerped, GetPatchEndLocal, GetPatchEndLocalAtUT, GetPatchEndRelative, GetPatchStartLerped, GetPatchStartLocal, GetPatchStartLocalAtUT, GetPatchStartRelative, GetPeriapsisLerped, GetPeriapsisLocal, GetPeriapsisLocalAtUT, GetPeriapsisRelative, GetPoints, GetPointsLerped, GetPointsLocal ...
- auto: A / Orbit

## Assembly-CSharp/TransactionReasons.cs
- L=33 kind=enum type=TransactionReasons bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=37
- methods(0): 
- auto: A / Career

## Assembly-CSharp/TransferDataBase.cs
- L=63 kind=class type=TransferDataBase bases=AppUI_Data anc=AppUI_Data/IConfigNode ns=-
- unity(6): none | ksp: Vessel=1, Orbit=1 | fan_in=5
- methods(3): GetAlarmDescription, GetAlarmTitle, IsAnyDropdownOpen
- auto: A / Orbit

## Assembly-CSharp/TransferDataSimple.cs
- L=201 kind=class type=TransferDataSimple bases=TransferDataBase anc=TransferDataBase/AppUI_Data/IConfigNode ns=-
- unity(6): none | ksp: Vector3d=6, CelestialBody=5, Localizer=4, FlightGlobals=1 | fan_in=3
- methods(3): GetAlarmDescription, GetAlarmTitle, IsAnyDropdownOpen
- auto: A / Orbit

## Assembly-CSharp/TransferDataSimpleTopData.cs
- L=24 kind=class type=TransferDataSimpleTopData bases=TransferDataTopDataBase anc=TransferDataTopDataBase/AppUI_Data/IConfigNode ns=-
- unity(12): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Orbit

## Assembly-CSharp/TransferDataTopDataBase.cs
- L=13 kind=class type=TransferDataTopDataBase bases=AppUI_Data anc=AppUI_Data/IConfigNode ns=-
- unity(12): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / Orbit

## Assembly-CSharp/TransferTypeBase.cs
- L=107 kind=class type=TransferTypeBase bases=- anc=- ns=-
- unity(7): none | ksp: Localizer=9, HighLogic=4 | fan_in=2
- methods(10): ChangePosition, ChangeTime, CreateManeuver, DisplayName, DrawTypeControls, FollowingManeuversCheck, HideApp, OnUpdate, PriorManeuversCheck, UpdateTransferData
- auto: A / Orbit

## Assembly-CSharp/TransformExtension.cs
- L=124 kind=static class type=TransformExtension bases=- anc=- ns=-
- unity(74): Transform=25, GameObject=7, Shader=2, SetActive=2, Renderer=2, Material=1 | ksp: none | fan_in=3
- methods(7): FindChild, FindChildRecursive, FindParent, FindParentRecursive, NestToParent, SetLayerRecursive, SetShader
- auto: B / Core

## Assembly-CSharp/Tumbler.cs
- L=47 kind=MonoBehaviour type=Tumbler bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(53): Transform=1, Time.=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(2): Update, tumbleTo
- auto: B / FlightUI

## Assembly-CSharp/Tumblers.cs
- L=42 kind=MonoBehaviour type=Tumblers bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(43): MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(2): Awake, setValue
- auto: B / FlightUI

## Assembly-CSharp/TutorialAscent.cs
- L=531 kind=ScenarioModule type=TutorialAscent bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(24): Instantiate/Destroy=3, GameObject=3 | ksp: Localizer=56, FlightGlobals=36, Vessel=12, GameSettings=11, InputLockManager=10 | fan_in=0
- methods(4): OnAssetSetup, OnOnDestroy, OnTutorialSetup, UpdateDirectionTarget
- auto: B / Tutorials

## Assembly-CSharp/TutorialAsteroid1.cs
- L=180 kind=ScenarioModule type=TutorialAsteroid1 bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(33): Transform=2, Texture=1, GetComponent=1 | ksp: Localizer=33, FlightGlobals=8, GameSettings=6, Vessel=3, InputLockManager=3 | fan_in=0
- methods(3): OnAssetSetup, OnOnDestroy, OnTutorialSetup
- auto: B / Tutorials

## Assembly-CSharp/TutorialAsteroid2.cs
- L=192 kind=ScenarioModule type=TutorialAsteroid2 bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(26): GetComponent=3 | ksp: Localizer=29, FlightGlobals=9, Vessel=3, Part=2, InputLockManager=2 | fan_in=0
- methods(3): OnAssetSetup, OnOnDestroy, OnTutorialSetup
- auto: A / Tutorials

## Assembly-CSharp/TutorialDemo.cs
- L=125 kind=ScenarioModule type=TutorialDemo bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(85): IMGUI=18, UnityEngine.UI=6 | ksp: none | fan_in=0
- methods(3): OnAssetSetup, OnTutorialSetup, SomethingUnplanned
- auto: C / Tutorials

## Assembly-CSharp/TutorialDocking.cs
- L=266 kind=ScenarioModule type=TutorialDocking bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(33): Transform=2, Texture=2, GetComponent=1 | ksp: Localizer=44, FlightGlobals=23, GameSettings=21, Vessel=4, MapView=2 | fan_in=0
- methods(3): OnAssetSetup, OnOnDestroy, OnTutorialSetup
- auto: B / Tutorials

## Assembly-CSharp/TutorialDuna.cs
- L=178 kind=ScenarioModule type=TutorialDuna bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(54): Transform=6, GameObject=6, SetActive=1, GetComponent=1 | ksp: FlightGlobals=7, Vessel=2, Planetarium=1, InputLockManager=1, GameSettings=1 | fan_in=0
- methods(3): OnAssetSetup, OnOnDestroy, OnTutorialSetup
- auto: B / Tutorials

## Assembly-CSharp/TutorialEditor.cs
- L=497 kind=ScenarioModule type=TutorialEditor bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(18): Texture=1 | ksp: Localizer=53, InputLockManager=47, EditorLogic=43, AvailablePart=18, Part=16 | fan_in=0
- methods(5): OnAssetSetup, OnLoad, OnOnDestroy, OnSave, OnTutorialSetup
- auto: B / Tutorials

## Assembly-CSharp/TutorialEditorMid.cs
- L=747 kind=ScenarioModule type=TutorialEditorMid bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(28): Transform=10, Texture=2 | ksp: Localizer=42, Part=41, EditorLogic=38, InputLockManager=20, HighLogic=9 | fan_in=0
- methods(15): CheckCraftFile, Check_BaseVessel, Check_BaseVesselNoStats, Check_Dec, Check_Engine, Check_Fins, Check_Goo, Check_Name, Check_Tanks, CopyCraftFile, OnAssetSetup, OnLoad, OnOnDestroy, OnSave, OnTutorialSetup
- auto: B / Tutorials

## Assembly-CSharp/TutorialFlightBasics.cs
- L=386 kind=ScenarioModule type=TutorialFlightBasics bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(34): Instantiate/Destroy=3, GameObject=3, Texture=2, UnityEngine.UI=1 | ksp: Localizer=52, GameSettings=13, FlightGlobals=13, Vessel=11, InputLockManager=10 | fan_in=0
- methods(3): OnAssetSetup, OnOnDestroy, OnTutorialSetup
- auto: B / Tutorials

## Assembly-CSharp/TutorialFlightSuborbital.cs
- L=611 kind=ScenarioModule type=TutorialFlightSuborbital bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(24): Instantiate/Destroy=3, GameObject=3 | ksp: Localizer=69, FlightGlobals=34, Vessel=24, GameSettings=21, InputLockManager=13 | fan_in=0
- methods(4): OnAssetSetup, OnOnDestroy, OnTutorialSetup, UpdateDirectionTarget
- auto: B / Tutorials

## Assembly-CSharp/TutorialFromMun.cs
- L=424 kind=ScenarioModule type=TutorialFromMun bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(19): Texture=1 | ksp: Localizer=51, FlightGlobals=25, GameSettings=14, Vessel=13, InputLockManager=9 | fan_in=0
- methods(4): OnAssetSetup, OnOnDestroy, OnTutorialSetup, UpdateDirectionTarget
- auto: B / Tutorials

## Assembly-CSharp/TutorialMunDescent.cs
- L=233 kind=ScenarioModule type=TutorialMunDescent bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(20): Texture=1 | ksp: Localizer=38, FlightGlobals=18, Vessel=9, InputLockManager=7, TimeWarp=2 | fan_in=0
- methods(3): OnAssetSetup, OnOnDestroy, OnTutorialSetup
- auto: B / Tutorials

## Assembly-CSharp/TutorialMunFlight1.cs
- L=267 kind=ScenarioModule type=TutorialMunFlight1 bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(15): none | ksp: Localizer=40, FlightGlobals=27, InputLockManager=12, TimeWarp=3, Planetarium=3 | fan_in=0
- methods(3): OnAssetSetup, OnOnDestroy, OnTutorialSetup
- auto: A / Tutorials

## Assembly-CSharp/TutorialOrbit101.cs
- L=426 kind=ScenarioModule type=TutorialOrbit101 bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(23): Texture=1 | ksp: Localizer=79, FlightGlobals=22, TimeWarp=15, GameSettings=14, MapView=3 | fan_in=0
- methods(4): OnAssetSetup, OnOnDestroy, OnTutorialSetup, remoteNudge
- auto: B / Tutorials

## Assembly-CSharp/TutorialPageConfig.cs
- L=50 kind=class type=TutorialPageConfig bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(3): AddTutorialStepConfig, InitializeTutoStepsConfig, OnEnterEmpty
- auto: A / Tutorials

## Assembly-CSharp/TutorialScenario.cs
- L=563 kind=ScenarioModule type=TutorialScenario bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(52): GameObject=9, Lifecycle=5, Instantiate/Destroy=5, Screen/Cursor=3, GetComponent=3, Texture=2 | ksp: Localizer=4, InputLockManager=4, ConfigNode=3, HighLogic=2, GameSettings=2 | fan_in=33
- methods(38): AddPage, AddPages, AddState, CalcDialogXRatio, CloseTutorialWindow, CompleteTutorial, CreateButtonUi, CreateDialog, CreateDoneDialog, CreateMultiButtonDialog, CreateNextDialog, CreateNoButtonDialog, CreateOkDialog, CreateTutorialPage, FixedUpdate, GetContinueButtonUi, GetCurrentStateName, GetDoneButtonUi, GetNextButtonUi, GetOkButtonUi, GetTutorialNode, GoToLastPage, GoToNextPage, LateUpdate ...
- auto: B / Tutorials

## Assembly-CSharp/TutorialScience.cs
- L=216 kind=ScenarioModule type=TutorialScience bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(66): GameObject=10, Light=6, GetComponent=6, Transform=2, Screen/Cursor=1 | ksp: Localizer=27, ResearchAndDevelopment=2, PartLoader=2, GameEvents=2, ConfigNode=2 | fan_in=0
- methods(6): EnterRD, OnAssetSetup, OnLoad, OnOnDestroy, OnSave, OnTutorialSetup
- auto: B / Tutorials

## Assembly-CSharp/UI_ChooseOption.cs
- L=35 kind=attribute type=UI_ChooseOption bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: ConfigNode=2 | fan_in=3
- methods(2): Load, Save
- auto: A / Part

## Assembly-CSharp/UI_ColorPicker.cs
- L=9 kind=attribute type=UI_ColorPicker bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / Part

## Assembly-CSharp/UI_Control.cs
- L=112 kind=attribute type=UI_Control bases=Attribute anc=Attribute ns=-
- unity(12): none | ksp: ConfigNode=5 | fan_in=41
- methods(7): Load, ParseEnabled, ParseFloat, ParseString, Save, SetSceneVisibility, Setup
- auto: A / Part

## Assembly-CSharp/UI_Cycle.cs
- L=24 kind=attribute type=UI_Cycle bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: ConfigNode=2 | fan_in=6
- methods(2): Load, Save
- auto: A / Part

## Assembly-CSharp/UI_EnumAttribute.cs
- L=15 kind=MonoBehaviour type=UI_EnumAttribute bases=PropertyAttribute anc=PropertyAttribute ns=-
- unity(15): none | ksp: none | fan_in=0
- methods(0): 
- auto: B / Part

## Assembly-CSharp/UI_FieldFloatRange.cs
- L=29 kind=attribute type=UI_FieldFloatRange bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: ConfigNode=2 | fan_in=1
- methods(2): Load, Save
- auto: A / Part

## Assembly-CSharp/UI_FloatEdit.cs
- L=33 kind=attribute type=UI_FloatEdit bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: ConfigNode=2 | fan_in=1
- methods(2): Load, Save
- auto: A / Part

## Assembly-CSharp/UI_FloatRange.cs
- L=32 kind=attribute type=UI_FloatRange bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: ConfigNode=2 | fan_in=32
- methods(2): Load, Save
- auto: A / Part

## Assembly-CSharp/UI_Grid.cs
- L=16 kind=attribute type=UI_Grid bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Part

## Assembly-CSharp/UI_Group.cs
- L=6 kind=attribute type=UI_Group bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Part

## Assembly-CSharp/UI_Label.cs
- L=15 kind=attribute type=UI_Label bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: ConfigNode=2 | fan_in=17
- methods(2): Load, Save
- auto: A / Part

## Assembly-CSharp/UI_MinMaxRange.cs
- L=38 kind=attribute type=UI_MinMaxRange bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: ConfigNode=2 | fan_in=4
- methods(2): Load, Save
- auto: A / Part

## Assembly-CSharp/UI_ProgressBar.cs
- L=29 kind=attribute type=UI_ProgressBar bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: ConfigNode=2 | fan_in=3
- methods(2): Load, Save
- auto: A / Part

## Assembly-CSharp/UI_Resources.cs
- L=7 kind=attribute type=UI_Resources bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Part

## Assembly-CSharp/UI_ScaleEdit.cs
- L=37 kind=attribute type=UI_ScaleEdit bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: ConfigNode=2 | fan_in=1
- methods(4): Load, MaxValue, MinValue, Save
- auto: A / Part

## Assembly-CSharp/UI_Scene.cs
- L=7 kind=enum type=UI_Scene bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=63
- methods(0): 
- auto: A / Part

## Assembly-CSharp/UI_Toggle.cs
- L=48 kind=attribute type=UI_Toggle bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: Localizer=2, ConfigNode=2 | fan_in=25
- methods(2): Load, Save
- auto: A / Part

## Assembly-CSharp/UI_VariantSelector.cs
- L=8 kind=attribute type=UI_VariantSelector bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Part

## Assembly-CSharp/UI_Vector2.cs
- L=35 kind=attribute type=UI_Vector2 bases=UI_Control anc=UI_Control/Attribute ns=-
- unity(0): none | ksp: ConfigNode=2 | fan_in=1
- methods(2): Load, Save
- auto: A / Part

## Assembly-CSharp/UIAutoContentSizer.cs
- L=12 kind=MonoBehaviour type=UIAutoContentSizer bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): Lifecycle=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(2): Start, Update
- auto: C / UI

## Assembly-CSharp/UIButtonTextColor.cs
- L=21 kind=MonoBehaviour type=UIButtonTextColor bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(57): TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(2): SetIdleColor, SetOverColor
- auto: C / UI

## Assembly-CSharp/UICameraController.cs
- L=43 kind=MonoBehaviour type=UICameraController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(76): Lifecycle=3, Camera=2, Screen/Cursor=1, MonoBehaviour=1, GetComponent=1 | ksp: GameEvents=4 | fan_in=0
- methods(5): Awake, OnDestroy, OnSceneChange, ScreenResolutionModified, Start
- auto: C / UI

## Assembly-CSharp/UIConfirmDialog.cs
- L=179 kind=MonoBehaviour type=UIConfirmDialog bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(80): UnityEngine.UI=5, TMPro=5, Transform=4, GameObject=4, SetActive=3, Lifecycle=3 | ksp: GameEvents=2 | fan_in=9
- methods(8): CloseDialog, OnCancel, OnConfirm, OnDestroy, OnLeavingScene, Spawn, Start, Update
- auto: C / UI

## Assembly-CSharp/UIConstructionPanelPart.cs
- L=33 kind=MonoBehaviour type=UIConstructionPanelPart bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(96): UnityEngine.UI=3, SetActive=3, Lifecycle=3, GameObject=3, Texture=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(4): OnButtonCollapsePressed, OnDestroy, Start, Update
- auto: C / UI

## Assembly-CSharp/UIFolderManagementDialog.cs
- L=163 kind=MonoBehaviour type=UIFolderManagementDialog bases=UIConfirmDialog anc=UIConfirmDialog/MonoBehaviour ns=-
- unity(57): Transform=4, GameObject=3, SetActive=2, TMPro=1, Instantiate/Destroy=1, GetComponent=1 | ksp: Localizer=11 | fan_in=1
- methods(4): CountItemsInDirectory, InitCreateDialog, InitDeleteDialog, Spawn
- auto: C / UI

## Assembly-CSharp/UIListClamper.cs
- L=69 kind=MonoBehaviour type=UIListClamper bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(82): UnityEngine.UI=4, Time.=3, SetActive=3, GameObject=3, Lifecycle=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(4): Awake, Clamp, FixedUpdate, OnRectTransformDimensionsChange
- auto: C / UI

## Assembly-CSharp/UIMarquee.cs
- L=402 kind=MonoBehaviour type=UIMarquee bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(82): Transform=32, UnityEngine.UI=20, Coroutine=20, GetComponent=4, Lifecycle=3, GameObject=3 | ksp: none | fan_in=2
- methods(13): AutoStartMarquee, Awake, Configure, Field_OnValueModified, Initialize, MoveMarquee, OnDestroy, OnPointerEnter, OnPointerExit, ResetMovingFlags, Start, StartMarquee, StopMarquee
- auto: C / UI

## Assembly-CSharp/UIMarquee_ActionField.cs
- L=26 kind=MonoBehaviour type=UIMarquee_ActionField bases=UIMarquee anc=UIMarquee/MonoBehaviour ns=-
- unity(59): Lifecycle=2, GetComponent=1 | ksp: GameEvents=2 | fan_in=0
- methods(3): OnDestroy, OnPartActionUIDismiss, Start
- auto: C / UI

## Assembly-CSharp/UIMarquee_PAW.cs
- L=47 kind=MonoBehaviour type=UIMarquee_PAW bases=UIMarquee anc=UIMarquee/MonoBehaviour ns=-
- unity(49): Lifecycle=2, GetComponent=1 | ksp: Part=4, GameEvents=4, UIPartActionWindow=1 | fan_in=0
- methods(4): OnDestroy, OnPartActionUIDismiss, OnPartActionUIShown, Start
- auto: C / UI

## Assembly-CSharp/UINavExplicit.cs
- L=39 kind=MonoBehaviour type=UINavExplicit bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(77): UnityEngine.UI=2, Lifecycle=2, GetComponent=2, MonoBehaviour=1 | ksp: GameEvents=2 | fan_in=1
- methods(3): CheckForNonInteractable, OnDestroy, Start
- auto: C / UI

## Assembly-CSharp/UINavMouseChecker.cs
- L=75 kind=MonoBehaviour type=UINavMouseChecker bases=MonoBehaviour,ISelectHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(67): EventSystems=6, MonoBehaviour=1 | ksp: none | fan_in=2
- methods(6): MouseHasExited, MouseIsHovering, OnPointerEnter, OnPointerExit, OnSelect, SetMenuNavReference
- auto: C / UI

## Assembly-CSharp/UIPartActionAeroDisplay.cs
- L=139 kind=MonoBehaviour type=UIPartActionAeroDisplay bases=UIPartActionItem anc=UIPartActionItem/MonoBehaviour ns=-
- unity(88): TMPro=15, SetActive=5, GameObject=5, UnityEngine.UI=2, Transform=1, Lifecycle=1 | ksp: Localizer=9, UIPartActionWindow=1, Part=1 | fan_in=2
- methods(3): Awake, Setup, UpdateItem
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionButton.cs
- L=70 kind=MonoBehaviour type=UIPartActionButton bases=UIPartActionEventItem anc=UIPartActionEventItem/UIPartActionItem/MonoBehaviour ns=-
- unity(46): UnityEngine.UI=1, TMPro=1, Lifecycle=1 | ksp: EditorLogic=3, HighLogic=1 | fan_in=2
- methods(3): Awake, OnClick, UpdateItem
- auto: B / FlightUI

## Assembly-CSharp/UIPartActionChooseOption.cs
- L=141 kind=MonoBehaviour type=UIPartActionChooseOption bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(35): TMPro=2, UnityEngine.UI=1 | ksp: InputLockManager=3, UIPartActionWindow=1, PartModule=1, Part=1 | fan_in=0
- methods(9): GetDisplay, GetFieldValue, OnTap_dec, OnTap_inc, OnValueChanged, SetFieldValue, Setup, UpdateDisplay, UpdateState
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionColorPicker.cs
- L=143 kind=MonoBehaviour type=UIPartActionColorPicker bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(75): UnityEngine.UI=6, GameObject=6, SetActive=4, Lifecycle=2, GetComponent=2, Transform=1 | ksp: UIPartActionWindow=1, PartModule=1, Part=1 | fan_in=4
- methods(7): CloseColorPickWindow, InitializeColourPresets, OnColorChange, OnDestroy, OnEnable, Setup, ToggleColorPickWindow
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionControllerInventory.cs
- L=725 kind=MonoBehaviour type=UIPartActionControllerInventory bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(70): GameObject=26, Transform=20, Instantiate/Destroy=10, SetActive=8, Rigidbody=5, UnityEngine.UI=4 | ksp: Part=15, HighLogic=13, GameEvents=13, PartLoader=4, FlightGlobals=3 | fan_in=11
- methods(28): Awake, ConfigureInventoryPart, CreateFlightTooltip, CreatePartFromInventory, DestroyHeldPartAsIcon, DestroyTooltip, IsKerbalWithinRange, MoveSelectedPart, OnCameraChange, OnCargoPartConstructionEvent, OnDestroy, OnInventoryPartHover, OnPartPurchased, OnVesselDestroy, PlayPartDroppedSFX, PlayPartSelectedSFX, ResetInventoryCacheValues, ReturnHeldPart, SetIconAsPart, SetPartAsIcon, Start, ToggleCargoPartsLight, UpdateAudioVolume, UpdateCurrentCargoPart ...
- auto: B / FlightUI

## Assembly-CSharp/UIPartActionControllerSpawner.cs
- L=11 kind=MonoBehaviour type=UIPartActionControllerSpawner bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(59): MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1 | ksp: none | fan_in=0
- methods(1): Start
- auto: B / FlightUI

## Assembly-CSharp/UIPartActionCycle.cs
- L=53 kind=MonoBehaviour type=UIPartActionCycle bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(46): TMPro=2 | ksp: UIPartActionWindow=1, PartModule=1, Part=1, Localizer=1, InputLockManager=1 | fan_in=0
- methods(5): GetFieldValue, OnTap, SetButtonState, Setup, UpdateItem
- auto: B / FlightUI

## Assembly-CSharp/UIPartActionEventItem.cs
- L=95 kind=MonoBehaviour type=UIPartActionEventItem bases=UIPartActionItem anc=UIPartActionItem/MonoBehaviour ns=-
- unity(23): Transform=1 | ksp: FlightGlobals=6, PartModule=2, Part=2, GameSettings=2, UIPartActionWindow=1 | fan_in=1
- methods(4): CheckInRange, FireSymCounterparts, IsItemValid, Setup
- auto: B / FlightUI

## Assembly-CSharp/UIPartActionFieldFloatRange.cs
- L=71 kind=MonoBehaviour type=UIPartActionFieldFloatRange bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(30): UnityEngine.UI=1 | ksp: UIPartActionWindow=1, PartModule=1, Part=1, InputLockManager=1 | fan_in=0
- methods(5): OnValueChanged, SetSliderValue, Setup, UpdateFieldValues, UpdateItem
- auto: B / FlightUI

## Assembly-CSharp/UIPartActionFieldItem.cs
- L=155 kind=MonoBehaviour type=UIPartActionFieldItem bases=UIPartActionItem anc=UIPartActionItem/MonoBehaviour ns=-
- unity(20): Transform=1 | ksp: FlightGlobals=4, Part=3, PartModule=2, GameSettings=2, UIPartActionWindow=1 | fan_in=18
- methods(7): CheckInRange, FireSymmetryEvents, GetField, IsItemValid, SetFieldValue, SetSymCounterpartValue, Setup
- auto: B / FlightUI

## Assembly-CSharp/UIPartActionFloatEdit.cs
- L=231 kind=MonoBehaviour type=UIPartActionFloatEdit bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(54): GameObject=5, SetActive=4, UnityEngine.UI=3, TMPro=2, GetComponent=1 | ksp: InputLockManager=5, UIPartActionWindow=1, PartModule=1, Part=1 | fan_in=0
- methods(14): AdjustValue, Clamp, GetFieldValue, IntervalBase, OnTap_decLarge, OnTap_decSmall, OnTap_incLarge, OnTap_incSmall, OnValueChanged, Setup, SliderInterval, UpdateControlStates, UpdateDisplay, UpdateSlider
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionFloatRange.cs
- L=133 kind=MonoBehaviour type=UIPartActionFloatRange bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(66): TMPro=4, SetActive=4, GameObject=4, UnityEngine.UI=1, Lifecycle=1 | ksp: GameEvents=2, UIPartActionWindow=1, PartModule=1, Part=1, InputLockManager=1 | fan_in=1
- methods(8): GetFieldValue, OnDestroy, OnFieldInput, OnValueChanged, SetSliderValue, Setup, ToggleNumericSlider, UpdateItem
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionFuelFlowOverlay.cs
- L=47 kind=MonoBehaviour type=UIPartActionFuelFlowOverlay bases=UIPartActionItem anc=UIPartActionItem/MonoBehaviour ns=-
- unity(45): TMPro=1 | ksp: UIPartActionWindow=1, Part=1, Localizer=1 | fan_in=2
- methods(2): FlowToggle, Setup
- auto: B / FlightUI

## Assembly-CSharp/UIPartActionGroup.cs
- L=162 kind=MonoBehaviour type=UIPartActionGroup bases=UIPartActionItem anc=UIPartActionItem/MonoBehaviour ns=-
- unity(76): UnityEngine.UI=10, Transform=4, Texture=2, Lifecycle=2, TMPro=1, Instantiate/Destroy=1 | ksp: GameSettings=5, GameEvents=2, UIPartActionWindow=1, Localizer=1 | fan_in=2
- methods(10): AddItemToContent, Collapse, CollapseGroupToggle, Expand, Initialize, OnDestroy, RescaleContentItems, SetUIState, Start, UpdateContentSize
- auto: C / FlightUI
