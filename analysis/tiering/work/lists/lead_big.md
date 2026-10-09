# Lead hand review: 127 files > 800 lines

## Assembly-CSharp-firstpass/DAE.cs
- L=1721 kind=MonoBehaviour type=DAE bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(58): GameObject=31, Texture=26, Transform=17, Shader=9, Material=8, GetComponent=7 | ksp: none | fan_in=4
- methods(25): Build, CanParseGeometry, CheckScale, DAEGetColor, FormatFaceData, GetDAEMaterials, GetFaceData, GetGeometryData, GetMaterial, GetSceneGeo, GetUpAxis, GetValidTexturePath, Load, NormalMap, ReadDAE, RemoveWS, ScaleTexture, SolveMaterials, Start, UpPosConv, UpRotDirConv, UpScaleConv, cf, ci ...
- auto: C / Rendering

## Assembly-CSharp-firstpass/OBJ.cs
- L=972 kind=MonoBehaviour type=OBJ bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(68): Texture=27, GameObject=19, Shader=14, Material=9, GetComponent=6, WWW/WebRequest=5 | ksp: none | fan_in=4
- methods(15): Build, CalculateScaleOffset, CheckScale, GetMaterial, Load, NormalMap, ScaleTexture, SetGeometryData, SetMaterialData, SolveMaterials, Start, cf, ci, gc, pushscaleoffset
- auto: C / Rendering

## Assembly-CSharp/AdvancedTutorial.cs
- L=1233 kind=ScenarioModule type=AdvancedTutorial bases=METutorialScenario anc=METutorialScenario/TutorialScenario/ScenarioModule ns=-
- unity(23): GetComponent=5, GameObject=5, Transform=2, UnityEngine.UI=1 | ksp: GameEvents=3, InputLockManager=2 | fan_in=0
- methods(71): AddEndNodeListener, ArrangeGraph, CreateTutorialPages, EnterVesselDrag, EnterVesselLink, EnterVesselMessage, EnterVesselName, EnterVesselWriteError, EnterVesselWriteMain, GetCurrentNodeParameters, GetCurrentTutorialStep, GetLinkNodes, GetMessageParameter, GetNodeEndParameter, GetTextNodes, GetVesselDropDown, HighlightTitleBarExportButton, IsAllDialogNodeEnd, IsAllTextValueValid, IsDragErrorStep, IsDragMainStep, IsExportDialogOpen, IsLinkErrorStep, IsLinkMainStep ...
- auto: B / Tutorials

## Assembly-CSharp/AeroGUI.cs
- L=914 kind=KSPAddon type=AeroGUI bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(86): IMGUI=173, Transform=9, UnityEngine.UI=3, GetComponent=3, Rigidbody=2, Lifecycle=2 | ksp: Localizer=45, PhysicsGlobals=20, Vector3d=19, FlightGlobals=5, Vessel=3 | fan_in=0
- methods(7): DrawWindow, DrawWindowThermal, GetAeroStats, GetThermalStats, OnGUI, Start, Update
- auto: B / Aero

## Assembly-CSharp/AnalyticsUtil.cs
- L=884 kind=static class type=AnalyticsUtil bases=- anc=- ns=-
- unity(15): QualitySettings/SystemInfo=11, Application.=6, Steam=3, Analytics=3, PlayerPrefs=2 | ksp: Vessel=16, Game=15, FlightGlobals=7, Contract=3, ProtoCrewMember=2 | fan_in=41
- methods(46): AddCommonSteamItemData, AddMissionGeneralData, AddSaveVesselDate, AltimeterModeUsage, CountVesselSituation, DeltaVAppUsage, DeltaVStageUsage, FixIncorrectTutorialName, Initialize, LogAsteroidEvent, LogAsteroidVesselEvent, LogCelestialBodyProgress, LogCometEvent, LogCometVesselEvent, LogContractAccepted, LogContractCancelled, LogContractCompleted, LogCustomEvent, LogCustomEventToFile, LogDebugWindow, LogEVAChuteDeployed, LogExitEditor, LogExitFlight, LogGameStart ...
- auto: C / Analytics

## Assembly-CSharp/AssemblyLoader.cs
- L=1004 kind=static class type=AssemblyLoader bases=- anc=- ns=-
- unity(9): none | ksp: ConfigNode=10, GameDatabase=1 | fan_in=34
- methods(40): Add, AddForbiddenString, AddForbiddenType, CheckDependencies, ClearPlugins, Contains, FlagDuplicatedPlugins, GetAssembly, GetByAssembly, GetClassByName, GetEnumerator, GetModulesImplementingInterface, GetPathByType, GetSubclassesOfParentClass, GetTypeByName, GetTypesOfClassesImplementingInterface, Initialize, InitializeForbidden, Load, LoadAssemblies, LoadAssembly, LoadAttribute_KSPAssembly, LoadAttribute_KSPAssemblyDependency, LoadAttribute_KSPAssemblyDependencyEqualMajor ...
- auto: A / Modding

## Assembly-CSharp/BaseField.cs
- L=931 kind=class type=BaseField bases=- anc=BaseField ns=-
- unity(12): none | ksp: KSPField=6, Localizer=4 | fan_in=55
- methods(14): CacheLocalStrings, CopyField, GetStringValue, GetValue, GuiString, Read, ReadPvt, SetOriginalValue, SetPvt, SetValue, SetupUIControl, SetupUIControlLabel, SetupUIControls, Write
- auto: A / Part

## Assembly-CSharp/BasicTutorial.cs
- L=1171 kind=ScenarioModule type=BasicTutorial bases=METutorialScenario anc=METutorialScenario/TutorialScenario/ScenarioModule ns=-
- unity(35): GameObject=30, GetComponent=5, UnityEngine.UI=1 | ksp: ConfigNode=5, Orbit=2, Localizer=2 | fan_in=0
- methods(100): AddTutorialPage, CreateDialog, CreateTutorialPages, CurrentNodeFilteredAreSelected, FacilityIsSetted, GetNextTutorialName, GetVesselToggleTypeParam, HighlightTitleBarSaveButton, IsPlayerBuiltCreation, IsSituationSetted, LaunchSiteIsSetted, ModifySaveButtonForTutorial, OnBriefingCancel, OnBriefingOkay, OnContinueButtonClick, OnDoneButtonClick, OnEnterChangeVesselName, OnEnterClickSaveButton, OnEnterConfirmLaunchSite, OnEnterConfirmOrbitPlanet, OnEnterCreateVesselNode, OnEnterDragOrbitNode, OnEnterDragVesselNode, OnEnterEmpty ...
- auto: B / Tutorials

## Assembly-CSharp/CelestialBody.cs
- L=1231 kind=MonoBehaviour type=CelestialBody bases=MonoBehaviour,ITargetable,IDiscoverable anc=MonoBehaviour ns=-
- unity(35): AnimationCurve=12, Transform=9, GetComponent=6, Texture=3, Renderer=2, MonoBehaviour=2 | ksp: Vector3d=60, PhysicsGlobals=20, Planetarium=19, FloatCurve=16, CelestialBody=16 | fan_in=241
- methods(65): Awake, CBUpdate, CacheLocalStrings, Equals, GetActiveTargetable, GetAltitude, GetAtmoThermalStats, GetBodyReferencing, GetDensity, GetDisplayName, GetFrameVel, GetFrameVelAtUT, GetFullTemperature, GetFwdVector, GetHashCode, GetImpactLatitudeAndLongitude, GetLatLonAlt, GetLatLonAltOrbital, GetLatitude, GetLatitudeAndLongitude, GetLongitude, GetName, GetObtVelocity, GetOrbit ...
- auto: B / Celestials

## Assembly-CSharp/CometManager.cs
- L=845 kind=MonoBehaviour type=CometManager bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(48): AnimationCurve=33, GameObject=5, Transform=4, Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: FloatCurve=33, Vessel=15, ConfigNode=8, FlightGlobals=6, GameEvents=5 | fan_in=10
- methods(30): Awake, GenerateDefinition, GenerateHomeSpawnDistance, GenerateSentinelSpawnDistance, GenerateWeightedCometType, GenerateWeightedDustTailColor, GenerateWeightedIonTailColor, GetCometOrbitType, GetSpawnChance, Load, LoadColorsList, LoadCometTypes, LoadManagerDefinitions, LoadTailColorLists, OnDestroy, OnGameDatabaseLoaded, OnVesselAdded, OnVesselRemoved, ReprimeParticles, Save, SetCollider, SetComa, SetDust, SetDustTail ...
- auto: B / Celestials

## Assembly-CSharp/CompoundPart.cs
- L=874 kind=MonoBehaviour type=CompoundPart bases=Part anc=Part/MonoBehaviour ns=-
- unity(61): Transform=32, GameObject=10, Raycast/Query=7, Coroutine=7, LayerMask=4, Input.=4 | ksp: HighLogic=19, Part=18, EditorLogic=14, GameEvents=6, InputLockManager=5 | fan_in=11
- methods(48): DisconnectAction, DisconnectCompoundPart, DisconnectEvent, DumpTarget, EndPreview, GetPartColliders, GetReferenceParent, GetReferenceTransform, LateUpdate, LockEVAEditor, OnConstructionModeUpdate, OnEditorEvent, OnInventoryModeDisable, OnInventoryModeEnable, OnLoad, OnPartCreatedFomInventory, OnSave, ParseCustomPartData, PreviewAttachment, SetLink, SetSymmetryValues, SetTarget, ToggleSymmetryCounterpartsTweak, ToggleTweakTarget ...
- auto: B / PartModules-Structural

## Assembly-CSharp/ConfigNode.cs
- L=4344 kind=class type=ConfigNode bases=- anc=- ns=-
- unity(15): GameObject=27, MonoBehaviour=13, Transform=8, Resources.=2, GetComponent=2, Instantiate/Destroy=1 | ksp: ConfigNode=131, QuaternionD=15, Vector3d=14, Localizer=2 | fan_in=667
- methods(115): Add, AddData, AddNode, AddValue, AssignLink, AssignLinkable, AssignTarget, CheckAndParseColor, CleanupInput, Clear, ClearData, ClearNodes, ClearValues, CompileConfig, Contains, CopyTo, CopyToRecursive, CountByName, CreateConfigFromObject, CreateCopy, CreateNodeObject, CreateObject, CreateObjectFromConfig, CustomEqualSplit ...
- auto: B / Save

## Assembly-CSharp/Contracts/Contract.cs
- L=2020 kind=class type=Contract bases=IContractParameterHost anc=IContractParameterHost ns=Contracts
- unity(9): Analytics=3, Lifecycle=1, Coroutine=1 | ksp: CelestialBody=56, Reputation=41, Localizer=31, Contract=26, ContractParameter=24 | fan_in=106
- methods(97): Accept, AddBodyNodes, AddBodyTrees, AddKeywords, AddKeywordsRequired, AddNextUnreachedBodyTrees, AddParameter, AwardAdvance, AwardCompletion, CanBeCancelled, CanBeDeclined, CanBeFailed, Cancel, Complete, CountBodies_Complete, CountBodies_Reached, Decline, Fail, Generate, GenerateFailed, GetBodies, GetBodies_Complete, GetBodies_InComplete, GetBodies_NextUnreached ...
- auto: A / Contracts

## Assembly-CSharp/Contracts/ContractParameter.cs
- L=841 kind=class type=ContractParameter bases=IContractParameterHost anc=IContractParameterHost ns=Contracts
- unity(11): Coroutine=2, Lifecycle=1 | ksp: ContractParameter=23, Reputation=11, ConfigNode=7, Localizer=6, Contract=6 | fan_in=60
- methods(39): AddParameter, AllChildParametersComplete, AnyChildParametersFailed, AwardCompletion, CreateID, Disable, Enable, GetDestinationWeight, GetHashString, GetMessageComplete, GetMessageFailed, GetMessageIncomplete, GetNotes, GetParameter, GetTitle, Load, NestToParent, OnLoad, OnParameterStateChange, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate ...
- auto: A / Contracts

## Assembly-CSharp/Contracts/ContractSystem.cs
- L=1551 kind=ScenarioModule type=ContractSystem bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=Contracts
- unity(38): Coroutine=11, GameObject=10, Transform=5, SetActive=2, Instantiate/Destroy=2, GetComponent=2 | ksp: Contract=81, ConfigNode=20, GameEvents=19, FlightGlobals=15, HighLogic=14 | fan_in=41
- methods(59): AdjustWeight, AnyCompletedContracts, AnyCurrentActiveContracts, AnyCurrentContracts, ClampedWeight, ClearContractsCurrent, ClearContractsFinished, ConstructShip, CountContracts, GenerateContract, GenerateContractTypes, GenerateContracts, GenerateMandatoryContracts, GenerateMandatoryTypes, GenerateParameterTypes, GeneratePredicateTypes, GenerateTypes, GetActiveContractCount, GetCompletedContracts, GetContractByGuid, GetContractCounts, GetContractType, GetCurrentActiveContracts, GetCurrentContracts ...
- auto: B / Contracts

## Assembly-CSharp/Contracts/Templates/PartTest.cs
- L=874 kind=ContractParameter type=PartTest bases=Contract anc=ContractParameter/IContractParameterHost ns=Contracts.Templates
- unity(6): GetComponent=1 | ksp: Vessel=97, Localizer=36, CelestialBody=13, ResearchAndDevelopment=7, Planetarium=7 | fan_in=0
- methods(17): CombineSituations, Generate, GetDescription, GetSituationString, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnAccepted, OnFinished, OnLoad, OnPartResearched, OnRegister, OnSave, OnUnregister, RemoveSituations
- auto: A / Contracts

## Assembly-CSharp/Contracts/Templates/RecoverAsset.cs
- L=1024 kind=Contract type=RecoverAsset bases=Contract anc=Contract/IContractParameterHost ns=Contracts.Templates
- unity(0): none | ksp: Localizer=56, ConfigNode=21, ProtoCrewMember=19, Planetarium=16, HighLogic=10 | fan_in=3
- methods(39): AcceptablePart, CheckComplete, ChooseLocation, ChooseOrbit, ChooseVesselType, Cleanup, CreateProcessedPartNode, DiscoverAsset, Generate, GenerateKerbalInPart, GenerateLandRecovery, GenerateLoneKerbal, GenerateLonePart, GenerateOrbitalRecovery, GetDescription, GetHashString, GetMultiplier, GetNotes, GetSynopsys, GetTitle, GetWeightBodies, GroundNode, Initialize, MeetRequirements ...
- auto: A / Contracts

## Assembly-CSharp/DatabaseLoaderModel_DAE.cs
- L=1736 kind=class type=DatabaseLoaderModel_DAE bases=DatabaseLoader<GameObject> anc=DatabaseLoader ns=-
- unity(49): GameObject=49, Texture=27, GetComponent=18, Transform=17, Mesh=12, Shader=9 | ksp: GameDatabase=1 | fan_in=0
- methods(26): AddReducedMeshColliders, Build, CanParseGeometry, CheckScale, DAEGetColor, FormatFaceData, GetDAEMaterials, GetFaceData, GetGeometryData, GetMaterial, GetSceneGeo, GetUpAxis, GetValidTexturePath, Load, LoadDAE, NormalMap, ReadDAE, RemoveWS, ScaleTexture, SolveMaterials, UpPosConv, UpRotDirConv, UpScaleConv, cf ...
- auto: C / Rendering

## Assembly-CSharp/DebugToolbar.cs
- L=1941 kind=MonoBehaviour type=DebugToolbar bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(98): IMGUI=650, UnityEngine.UI=97, AnimationCurve=8, Coroutine=6, Input.=4, GetComponent=4 | ksp: PhysicsGlobals=88, HighLogic=42, FlightGlobals=29, GameDatabase=13, Contract=10 | fan_in=86
- methods(63): CheatInExperience, CheatInFunds, CheatInReputation, CheatInScience, ContractGenerate, ContractsActive, ContractsAdd, ContractsArchive, ContractsOffered, ContractsTools, DatabaseAssemblies, DatabaseAudio, DatabaseConfigPreview, DatabaseConfigs, DatabaseConfigsSpam, DatabaseModels, DatabaseOverview, DatabaseOverviewConfig, DatabaseTextures, DrawFlightStats, DrawWindowRnD, FloatCurveEditor, FloatField, GetAnimationCurvePoints ...
- auto: C / Debug

## Assembly-CSharp/DeltaVStageInfo.cs
- L=1224 kind=class type=DeltaVStageInfo bases=- anc=- ns=-
- unity(12): Resources.=1 | ksp: PhysicsGlobals=12, Vessel=11, FlightGlobals=10, HighLogic=5, GameSettings=5 | fan_in=12
- methods(35): CalcLerpDeltaV, CalculateEngineResourceFuelMass, CalculateISP, CalculateStartMass, CalculateTWR, CalculateTimeRequiredDV, CheckTimeStep, CoTForStage, ContainsAnchoredDecoupler, ContainsDecoupler, EnginesDeprived, GetCurrentStageMass, GetEngineResourceParts, GetHighestSeparationIndex, GetPartDisplayInfo, GetSituationDeltaV, GetSituationISP, GetSituationTWR, GetSituationThrust, GetSituationTotalExhaustVelocity, GetSituationVectoredExhaustVelocity, GetSituationVectoredThrust, GetStageMass, MatchingSeparationIndex ...
- auto: A / Vessel

## Assembly-CSharp/DragCubeList.cs
- L=1010 kind=class type=DragCubeList bases=- anc=- ns=-
- unity(14): Time.=2, GameObject=1 | ksp: PhysicsGlobals=8, Part=7, ConfigNode=4, FloatCurve=3, AttachNode=3 | fan_in=5
- methods(39): AddCubeArray, AddFacedDotProducts, AddSurfaceDragDirection, AddVectorToArray, AreaToCubeOperation, ClearCubes, CreateVectorFromArray, DivideCubeArray, DrawDragGizmos, ForceUpdate, GetCube, GetCubeAreaDir, GetCubeCoeffDir, GetCubeDepthDir, GetFaceDirection, GetFacingAreaSum, LoadCube, LoadCubes, MultiplyCubeArray, RequestOcclusionUpdate, Reset, ResetCubeArray, ResetCubeWeights, SaveCubes ...
- auto: A / Aero

## Assembly-CSharp/EditorLogic.cs
- L=5490 kind=MonoBehaviour type=EditorLogic bases=EditorLogicBase anc=EditorLogicBase/MonoBehaviour ns=-
- unity(64): Transform=144, GameObject=74, Input.=44, Analytics=41, SetActive=35, Steam=30 | ksp: Part=98, Localizer=82, GameEvents=81, InputLockManager=59, HighLogic=59 | fan_in=99
- methods(156): AnyTextFieldHasFocus, AreAllPartsConnected, Awake, CenterDragPlane, CheckCopiedSymmetryPersistentId, CheckSymPartsAttach, CopyActionGroups, CountAllSceneParts, CraftBrowseCancelled, CreatePartForInventoryUse, DelayedAbort, DelayedUnlock, DeletePart, DestroySelectedPart, DuplicatePart, FinalizeAnalytics, FirstNonSymmetricalParentFrom, FixedUpdate, FlagSetup, GetMissionPreFlightCheck, GetPartExistsFilter, GetStockPreFlightCheck, HeldPartIsStacked, InitAnalytics ...
- auto: B / Editor

## Assembly-CSharp/EVAConstructionModeEditor.cs
- L=1724 kind=MonoBehaviour type=EVAConstructionModeEditor bases=EditorLogicBase anc=EditorLogicBase/MonoBehaviour ns=-
- unity(66): Transform=82, GameObject=13, Audio=11, Input.=8, Raycast/Query=7, Instantiate/Destroy=7 | ksp: FlightGlobals=47, Part=40, GameEvents=30, Vessel=29, GameSettings=26 | fan_in=1
- methods(62): AttachInput, AttachPart, Awake, BackupPart, BreakSymmetry, CanPartBeEdited, CannotInteract, ChangeCoordSpace, CheckAttach, CoordToggleInput, CopyActionGroups, CreatePartNode, DestroyBackupPart, DestroyHeldPart, DetachInput, DetachPart, DropAttachablePart, DropPartInput, FetchColliders, FixedUpdate, ForceDrop, GetPivotOffset, GetProtoVesselNode, GetRootPart ...
- auto: B / Kerbals

## Assembly-CSharp/Expansions/Missions/Editor/MEGUINode.cs
- L=1390 kind=MonoBehaviour type=MEGUINode bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(65): SetActive=32, GameObject=31, UnityEngine.UI=27, Transform=14, GetComponent=9, Instantiate/Destroy=8 | ksp: ConfigNode=8, Localizer=1 | fan_in=16
- methods(74): AddNewLine, Awake, CanDock, CancelDrag, CheckDockStatus, CheckNodeDockingPosition, CleanNodeBodyParameters, CleanUp, ClearDockableIndicator, ClearHighlighter, ClearValidityIndicators, DelayedUpdateConnectors, Deselect, Destroy, DisplayModuleSection, DisplayNodeBodyParameters, DisplayParameter, DockNode, DoesConnectionExist, GetDepthOfDockedNode, GetHighlighterColor, GetInputButtonPosition, GetOutputButtonPosition, GetState ...
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionBriefingDialog.cs
- L=1257 kind=MonoBehaviour type=MissionBriefingDialog bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(79): Analytics=40, Steam=31, UnityEngine.UI=30, GameObject=22, TMPro=19, SetActive=12 | ksp: Localizer=63, HighLogic=5, GameEvents=5, GameDatabase=3, GameSettings=1 | fan_in=4
- methods(63): DestroyDialog, DisableExportButon, Display, ExportMissionAfterChecksPassed, ExportMissionNameCancelled, ExportMissionNameConfirmed, GetTempBanner, Hide, LoadMissionPacks, LocalizationLock, LocalizationLockAuthor, LocalizationLockBriefing, LocalizationLockTitle, OnAddTag, OnAuthorTextChanged, OnBannerCancel, OnBannerDeleted, OnBannerSelected, OnBriefTextChanged, OnCancel, OnDestroy, OnDifficultySliderChanged, OnEndEdit, OnExitAfterSave ...
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionEditorLogic.cs
- L=2639 kind=MonoBehaviour type=MissionEditorLogic bases=MonoBehaviour,IMEHistoryTarget anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(56): UnityEngine.UI=39, KeyCode=23, Input.=23, EventSystems=14, GetComponent=13, GameObject=12 | ksp: Localizer=63, InputLockManager=28, GameEvents=23, ConfigNode=22, GameSettings=13 | fan_in=87
- methods(149): AddConnectorToConnectorList, AddConnectorToSelectedList, AddNode, AddNodeToSelectedList, ArrangeGraphNodes, Awake, CanvasClicked, CheckDialogsBeforeExiting, CheckpointTestMissionSetupSuccess, CheckpointToLoadSelected, ClearAllSelections, ClearConnector, ClearConnectorGroupSelection, ClearEditor, ClearSelectedNodesList, ConnectNodes, ConnectorButtonClicked, ConnectorButtonDragged, ConnectorButtonDropped, ConnectorSelectionChange, ContinueToTrackingStation, CopyMissionBanners, CopySelectedNodes, CreateEmptyMission ...
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MENode.cs
- L=1255 kind=MonoBehaviour type=MENode bases=MonoBehaviour,IConfigNode,IMEFlowBlock anc=MonoBehaviour ns=Expansions.Missions
- unity(30): GameObject=8, GetComponent=5, Lifecycle=3, Transform=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: ConfigNode=27, HighLogic=6, GameEvents=4, OrbitDriver=3, Orbit=3 | fan_in=176
- methods(40): ActionsContainModule, ActivateNode, Activated, AddAction, AddParameterToNodeBody, AddParametersDisplayedInNodeBody, AddParametersDisplayedInSAP, Awake, CatchAllControlCreated, ClearTestGroups, Clone, DeactivateNode, Deactivated, GetAllActionModules, GetNodeLocationInWorld, HasNodeBodyParameter, HasTestOrbit, InitializeTestGroups, Load, OnDestroy, OnLevelLoaded, RemoveParameterFromNodeBody, RevealAltitude, RevealDisplayName ...
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Mission.cs
- L=2705 kind=MonoBehaviour type=Mission bases=MonoBehaviour,IConfigNode anc=MonoBehaviour ns=Expansions.Missions
- unity(26): GameObject=9, Transform=7, Coroutine=6, Analytics=3, Lifecycle=2, Steam=1 | ksp: HighLogic=41, ConfigNode=29, GameEvents=27, Localizer=21, FlightGlobals=13 | fan_in=83
- methods(83): AddMappedCraft, AddMappedPartVesselName, Awake, BlockPlayMission, CheckHash, ClearCraftFiles, ClearMappedVessels, ContinueCriticalError, CurrentVesselID, DoEndMissionActions, Export, GenerateMissionLaunchSites, GetActionCreateAsteroid, GetActionCreateComet, GetActionCreateFlag, GetActionCreateFlagByPersistentID, GetActionCreateKerbal, GetActionCreateVessel, GetAllActionModules, GetAllTestModules, GetAllVesselSituations, GetAllVesselSituationsGuid, GetAsteroidByPersistentID, GetBanner ...
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionSituation.cs
- L=889 kind=class type=MissionSituation bases=IConfigNode,IMENodeDisplay anc=IConfigNode ns=Expansions.Missions
- unity(5): SetActive=1, Instantiate/Destroy=1, GameObject=1 | ksp: Localizer=19, ConfigNode=19, HighLogic=6, Game=6, KerbalRoster=4 | fan_in=15
- methods(43): AddNodeWithVesselToBuild, AddParameterToNodeBody, AddParameterToNodeBodyAndUpdateUI, AddParameterToSAP, Destroy, DestroySituation, FillVesselSituationList, GetAvailableVesselSituations, GetDisplayName, GetInfo, GetInternalParametersToDisplay, GetName, GetNode, GetNodeBodyParameterString, GetVesselSituationByVesselID, HasNodeBodyParameter, HasSAPParameter, InitSituation, Initialize, Load, OnAutoPopulateCrewValueChange, OnMissionRosterClick, OnResourceROCMissionSeedChanged, OnResourceSeedChanged ...
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionPlayDialog.cs
- L=1893 kind=MonoBehaviour type=MissionPlayDialog bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Runtime
- unity(73): SetActive=57, GameObject=51, UnityEngine.UI=28, Steam=23, TMPro=17, Transform=15 | ksp: Localizer=49, GameSettings=12, MissionSystem=10, ConfigNode=10, GameEvents=4 | fan_in=5
- methods(60): AddTutorialLock, BuildList, BuildPacksList, BuildSteamList, CancelLoadGame, ClearListItems, ClearPacksList, ConfirmLoadGame, ConfirmRestart, Create, CreateSteamWidget, CreateWidget, DismissRestartGame, DisplayAddedMissions, FinishedCallback, GetLastWriteTime, GetSFSMD5, Load, LoadDetailsFromMission, LoadFromMetaFile, LoadSteamItemPreviewURL, OnBtnCancel, OnBtnSteamOverlay, OnButtonCreate ...
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionsApp.cs
- L=1452 kind=MonoBehaviour type=MissionsApp bases=UIApp anc=UIApp/MonoBehaviour ns=Expansions.Missions.Runtime
- unity(56): GameObject=26, Coroutine=20, GetComponent=18, SetActive=15, Transform=11, Instantiate/Destroy=9 | ksp: HighLogic=37, MissionSystem=26, EditorLogic=23, Localizer=22, GameEvents=19 | fan_in=6
- methods(63): AddMission, AddVesselSubParameter, CheckforVesselstoBuild, CreateItem, CreateMissionItem, CreateMissionsList, CreateObjectivesList, CreateTestModeSettingsItem, CreateTestModeSettingsList, CreateVesselDetails, CreateVesselsList, DisplayApp, EditorVesselCompleted, ExitEditor, FireCreateNextVessel, GetAppScenes, GetAppScreenPos, GetColoredMissionHeader, GetColoredObjectiveHeader, GetVesselListAppVesselInfo, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized ...
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionSystem.cs
- L=1809 kind=ScenarioModule type=MissionSystem bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=Expansions.Missions.Runtime
- unity(59): Coroutine=46, GameObject=43, Transform=22, GetComponent=11, Instantiate/Destroy=9, Renderer=4 | ksp: HighLogic=99, ConfigNode=31, Planetarium=21, GameEvents=14, Vessel=11 | fan_in=40
- methods(43): AddMission, AllowCameraSwitch, CheckFireBriefingDialog, ClearCameraModeAndLocks, ConstructShip, CreateCheckpoint, DisplayMissionEndMessage, EnforceCameraModeAndLocks, GenerateBriefingDialogNode, GetFlightStateActiveBody, HasFinishedMission, LoadMetaMission, LoadMission, OnAwake, OnDestroy, OnLoad, OnLoadRoutine, OnSave, ReactivateComponents, RemoveMissionObjects, RemoveMissonObject, ResetKSCMarkers, SetLockedCamera, SetupCraftSaveFiles ...
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestDistance.cs
- L=875 kind=MonoBehaviour type=TestDistance bases=TestModule,IScoreableObjective anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(46): SetActive=22, GameObject=22, TMPro=2, Lifecycle=1 | ksp: Localizer=37, Vessel=21, ConfigNode=11, CelestialBody=10, Vector3d=9 | fan_in=1
- methods(32): Awake, CheckCrew, GetInfo, GetKerbalLocation, GetNodeBodyParameterString, GetScoreModifier, GetVesselLocation, Initialize, Load, OnDistanceFromKerbalControlCreated, OnDistanceFromTargetControlCreated, OnDistanceFromValueChange, OnDistanceFromVesselControlCreated, OnDistanceToAsteroidControlCreated, OnDistanceToCelestialBodyControlCreated, OnDistanceToCometControlCreated, OnDistanceToFlagControlCreated, OnDistanceToKerbalControlCreated, OnDistanceToLaunchSiteControlCreated, OnDistanceToLocationControlCreated, OnDistanceToNodeControlCreated, OnDistanceToTargetControlCreated, OnDistanceToTargetControlSetup, OnDistanceToValueChange ...
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Serenity/BaseServo.cs
- L=1819 kind=PartModule type=BaseServo bases=PartModule,IRoboticServo,IResourceConsumer anc=PartModule/MonoBehaviour ns=Expansions.Serenity
- unity(46): Transform=46, GameObject=16, Rigidbody=8, Lifecycle=4, Joint=4, GetComponent=3 | ksp: KSPField=37, HighLogic=36, GameEvents=17, Localizer=16, Part=14 | fan_in=7
- methods(104): ApplyCoordsUpdate, AttachServoRigidBody, BaseObject, BuildPartSets, CacheLocalStrings, CalculateAverageRateOfMovement, CalculatePower, CalculateResourceDrain, CanBeDetached, CanBeOffset, CanBeRotated, CreateServoRigidbody, DemoteToPhysicslessPart, DisengageMotor, DisengageServoLock, EngageMotor, EngageServoLock, FixedUpdate, GetAxisFieldLimit, GetAxisFieldLimits, GetConsumedResources, GetFrameDisplacement, GetHardLimits, GetInfo ...
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/CurvePanel.cs
- L=967 kind=MonoBehaviour type=CurvePanel bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=Expansions.Serenity
- unity(56): AnimationCurve=26, GameObject=14, SetActive=10, Transform=8, Instantiate/Destroy=7, UnityEngine.UI=6 | ksp: FloatCurve=4 | fan_in=4
- methods(46): Awake, BeginDragPoint, ClearSelectedPoints, CurveTimeFromXPos, CurveValueFromYPos, DeletePoint, DeleteSelectedPoints, DrawAll, DrawLine, DrawPoints, EndDragPoint, GetCurvePointsCount, GetIndexOfPoint, HideLimits, HideMaxLimit, HideMinLimit, HidePoints, InsertPoint, InsertPointAtPosition, MousePointClick, MovePoint, OnDestroy, OnDragPoint, OnPointerClick ...
- auto: C / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/DeployedScience/Runtime/DeployedScienceExperiment.cs
- L=839 kind=MonoBehaviour type=DeployedScienceExperiment bases=MonoBehaviour,IConfigNode anc=MonoBehaviour ns=Expansions.Serenity.DeployedScience.Runtime
- unity(46): Coroutine=7, GameObject=6, Transform=4, Lifecycle=4, Rigidbody=2, MonoBehaviour=1 | ksp: GameEvents=11, Localizer=10, FlightGlobals=10, Planetarium=8, ResearchAndDevelopment=5 | fan_in=5
- methods(26): Awake, CalcScience, CalcSendSeismicScience, CalculateScience, FixedUpdate, GatherScienceData, GroundScienceClusterPowerStateChanged, GroundSciencePartEnabledStateChanged, Load, OnDestroy, ProcessSeismicEvent, Save, SeismicDataCalc, SeismicDataCalcOverrideAdd, SeismicDataCalcOverrideRemove, SeismicListContains, SendDataToComms, SetControllerVessel, Spawn, SpawnandLoad, Start, TimeToSendStoredData, UpdateScienceExperiment, UpdateScienceLimitExperiment ...
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ModuleRobotArmScanner.cs
- L=1290 kind=PartModule type=ModuleRobotArmScanner bases=ModuleDeployablePart anc=ModuleDeployablePart/PartModule/MonoBehaviour ns=Expansions.Serenity
- unity(65): GameObject=27, Transform=19, GetComponent=13, ParticleSystem=11, Time.=9, Raycast/Query=7 | ksp: KSPField=26, Localizer=24, TimeWarp=17, HighLogic=16, GameEvents=10 | fan_in=3
- methods(44): CacheLocalStrings, CalculateExtendedTargetRotations, CanScan, DoExtend, DoRepair, DoRetract, ExtendArm, GetInfo, GetModuleDisplayName, OnAwake, OnCollisionEnter, OnCurrentMousePartChanged, OnDestroy, OnInventoryModeDisable, OnInventoryModeEnable, OnLoad, OnPartActionUICreate, OnPause, OnSave, OnStart, OnTimeWarpRateChanged, OnTriggerEnter, OnTriggerExit, OnUnpause ...
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ModuleRoboticController.cs
- L=1546 kind=PartModule type=ModuleRoboticController bases=PartModule,IShipConstructIDChanges,IResourceConsumer anc=PartModule/MonoBehaviour ns=Expansions.Serenity
- unity(25): Lifecycle=4, UnityEngine.UI=3, Instantiate/Destroy=2, GameObject=2, Transform=1, Time.=1 | ksp: Part=53, GameEvents=31, KSPAction=16, KSPField=11, HighLogic=10 | fan_in=14
- methods(80): AddPartAction, AddPartActionForSymmetryPartner, AddPartAxis, AddPartAxisForSymmetryPartner, AssignReferenceVars_Debug, CheckAndFireActions, CycleLoopMode, FixedUpdate, GetConsumedResources, GetInfo, GetModuleDisplayName, HasPart, HasPartAction, HasPartAxisField, HasPartSymmetryAction, HasPartSymmetryAxisField, OnAwake, OnDestroy, OnLoad, OnPartWillDie, OnSave, OnSceneLoadRequested, OnStart, OnUpdate ...
- auto: C / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RoboticControllerWindow.cs
- L=1233 kind=MonoBehaviour type=RoboticControllerWindow bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Serenity
- unity(71): UnityEngine.UI=57, Transform=16, GameObject=16, TMPro=12, SetActive=12, GetComponent=7 | ksp: GameEvents=10, Localizer=9, HighLogic=5, EditorLogic=3, InputLockManager=2 | fan_in=6
- methods(70): AddInputFieldLock, AddRow, AnyTextFieldHasFocus, Awake, ClearSelectedPresetMode, CloseWindow, CollapseOtherRows, OnAxisAlignEndsClick, OnAxisClampValuesClick, OnAxisCopyClick, OnAxisDownClick, OnAxisFlipHorizontalClick, OnAxisFlipVerticalClick, OnAxisPasteClick, OnAxisPresetClick, OnAxisPresetFlatClick, OnAxisPresetRevSawClick, OnAxisPresetSawClick, OnAxisPresetSineClick, OnAxisPresetSquareClick, OnAxisPresetTriangleClick, OnAxisSelectAllClick, OnAxisUpClick, OnCloseClick ...
- auto: C / DLC-BreakingGround

## Assembly-CSharp/FinePrint/ContractDefs.cs
- L=3533 kind=KSPAddon type=ContractDefs bases=MonoBehaviour anc=MonoBehaviour ns=FinePrint
- unity(16): Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1, GetComponent=1, GameObject=1 | ksp: ConfigNode=481, Reputation=177, Localizer=10, GameEvents=3, GameDatabase=3 | fan_in=45
- methods(14): AddCrewRequests, AddPartRequests, AddResourceRequests, AddSurveyDefinitions, Awake, CreateDefaultConfig, GetPreBuiltCraft, GetPreBuiltCraftPositions, LoadConfig, LoadConstructionPartsList, LoadContractCraftDefs, LoadDefinitions, OnDestroy, TechResearched
- auto: B / Contracts

## Assembly-CSharp/FinePrint/Contracts/ExplorationContract.cs
- L=846 kind=Contract type=ExplorationContract bases=Contract anc=Contract/IContractParameterHost ns=FinePrint.Contracts
- unity(0): none | ksp: Planetarium=17, Localizer=15, CelestialBody=12, FlightGlobals=2, ConfigNode=2 | fan_in=1
- methods(20): AllowableContracts, CanBeCancelled, CanBeDeclined, CanBeFailed, Compare, Generate, GetDescription, GetMissionMilestones, GetSeedMilestones, GetSynopsys, GetThemedMilestones, GetTitle, MeetRequirements, MessageCompleted, OnLoad, OnSave, ProgressTypeValue, SetLocals, SetPrestige, SetRewards
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/SatelliteContract.cs
- L=930 kind=Contract type=SatelliteContract bases=Contract,IUpdateWaypoints,ICheckSpecificVessels anc=Contract/IContractParameterHost ns=FinePrint.Contracts
- unity(1): none | ksp: Localizer=52, Planetarium=25, Vessel=14, Reputation=8, CelestialBody=7 | fan_in=0
- methods(23): AreSatellitesUnlocked, CanBeCancelled, CanBeDeclined, CheckSatelliteNetwork, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, NetworkSatelliteCount, OnAccepted, OnFinished, OnLoad, OnSave, PrestigeAppropriateOrbitType, ProbeCoresUnlocked, pickEasy, pickHard, pickMedium, setOrbitType
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/SurveyContract.cs
- L=943 kind=Contract type=SurveyContract bases=Contract anc=Contract/IContractParameterHost ns=FinePrint.Contracts
- unity(3): Texture=1 | ksp: Vessel=11, CelestialBody=10, Localizer=9, Planetarium=7, FlightGlobals=6 | fan_in=1
- methods(18): CachePossibleVessels, CanBeCancelled, CanBeDeclined, Generate, GetDescription, GetHashString, GetNotes, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave, ParamRange, PossibleDefinitions, RandomizeNearKSC, SetupParam
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/VesselUtilities.cs
- L=1291 kind=class type=VesselUtilities bases=- anc=- ns=FinePrint.Utilities
- unity(5): Renderer=4, Transform=1 | ksp: Vessel=51, FlightGlobals=21, ProtoCrewMember=14, Orbit=10, Part=8 | fan_in=23
- methods(29): ActiveVesselFallback, ActualCrewCapacity, ClassifyAllVesselsAt, ClassifyVesselType, CrewTraitMissionAvailability, DiscoverVessel, EstimatePartSize, FindFirstPartOrModuleName, FindVesselWithPartIDs, GenerateAdjustedVesselOrbit, GetFlightBand, GetPartIDList, GetPartName, SpecificVesselClassAt, VesselAtOrbit, VesselAtOrbitAccuracy, VesselCrewWithTrait, VesselCrewWithTraitCount, VesselHasAnyModules, VesselHasAnyParts, VesselHasAnyPartsOrModules, VesselHasModuleName, VesselHasPartName, VesselHasWheelsOnGround ...
- auto: B / Contracts

## Assembly-CSharp/FlightCamera.cs
- L=1182 kind=MonoBehaviour type=FlightCamera bases=MonoBehaviour,IKSPCamera anc=MonoBehaviour ns=-
- unity(63): Transform=56, GameObject=10, Time.=9, GetComponent=7, Input.=5, Camera=5 | ksp: FlightGlobals=38, GameSettings=29, Vessel=18, Part=11, GameEvents=8 | fan_in=41
- methods(53): ActivateUpdate, AdjustDistanceToFit, Awake, ClearTarget, CycleCameraHighlighter, DeactivateUpdate, DisableCamera, DisableCameraHighlighter, EnableCamera, GetAutoModeForVessel, GetCameraFoR, GetCameraTransform, GetChaseFoR, GetPivot, LateUpdate, MinDistanceBackaway, OnCameraChange, OnDestroy, OnLevelLoaded, OnNavigatorHandoff, OnNavigatorRequestControl, OnNavigatorTakeOver, OnSceneLoaded, OnSceneSwitch ...
- auto: C / Camera

## Assembly-CSharp/FlightGlobals.cs
- L=2003 kind=MonoBehaviour type=FlightGlobals bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(37): Transform=21, GameObject=14, Lifecycle=5, SendMessage/Invoke=3, SceneManager=3, Coroutine=3 | ksp: Vector3d=62, Vessel=56, GameEvents=44, CelestialBody=42, Part=19 | fan_in=431
- methods(88): ActiveVesselGoOffRails, AddVessel, Awake, CheckPartpersistentId, CheckProtoPartSnapShotpersistentId, CheckVesselpersistentId, ClearAllVessels, ClearDeadVessels, ClearInverseRotation, ClearToSave, ClearpersistentIdDictionaries, CountSpaceObjects, CountUntrackedSpaceObjects, CycleSpeedModes, FindLoadedPart, FindNearestControllableVessel, FindNearestVesselWhere, FindPartByID, FindProtoPartByID, FindUnloadedPart, FindVessel, FixedUpdate, ForceSetActiveVessel, GGetCometPerturberDisplayName ...
- auto: B / Vessel

## Assembly-CSharp/FlightIntegrator.cs
- L=2776 kind=VesselModule type=FlightIntegrator bases=VesselModule anc=VesselModule/MonoBehaviour ns=-
- unity(38): Rigidbody=29, Collision/Force=11, Transform=5, GetComponent=5, Lifecycle=3, Raycast/Query=2 | ksp: PhysicsGlobals=77, Part=72, GameSettings=30, GameEvents=14, Vessel=13 | fan_in=5
- methods(67): ApplyAeroDrag, ApplyAeroLift, CalcConvectiveCoefficient, CalculateAerodynamicArea, CalculateAnalyticTemperature, CalculateAreaExposed, CalculateAreaRadiative, CalculateAtmosphericDensity, CalculateBackgroundRadiationTemperature, CalculateConstantsAtmosphere, CalculateConstantsVacuum, CalculateConvectiveCoefficient, CalculateConvectiveCoefficientMach, CalculateConvectiveCoefficientNewtonian, CalculateDensityThermalLerp, CalculateDragValue, CalculateDragValue_Conic, CalculateDragValue_Cube, CalculateDragValue_Cylindrical, CalculateDragValue_Spherical, CalculatePressure, CalculateShockTemperature, CalculateSunBodyFlux, CheckThermalGraph ...
- auto: B / Aero

## Assembly-CSharp/GameDatabase.cs
- L=1541 kind=MonoBehaviour type=GameDatabase bases=LoadingSystem anc=LoadingSystem/MonoBehaviour ns=-
- unity(62): GameObject=47, Coroutine=20, Audio=17, Texture=15, Transform=9, Time.=9 | ksp: ConfigNode=27, Localizer=7, PartLoader=4, GameDatabase=3, GameEvents=2 | fan_in=111
- methods(54): AudioClipNameContains, Awake, BitmapToUnityNormalMap, CleanupLoaders, CompileConfig, CompileConfigRecursive, CreateDatabase, ExistsAudioClip, ExistsConfigNode, ExistsModel, ExistsShader, ExistsTexture, GetAllTexturesInFolder, GetAllTexturesInFolderType, GetAudioClip, GetAudioFile, GetConfigNode, GetConfigNodes, GetConfigs, GetMergedConfigNodes, GetModel, GetModelFile, GetModelIn, GetModelPrefab ...
- auto: C / Modding

## Assembly-CSharp/GameEvents.cs
- L=1058 kind=class type=GameEvents bases=GameEventsBase anc=GameEventsBase ns=-
- unity(10): Transform=6, PhysicMaterial=4, Raycast/Query=2 | ksp: Vessel=138, Part=132, ProtoCrewMember=56, CelestialBody=32, ConfigNode=30 | fan_in=548
- methods(1): FindEvent
- auto: B / Core

## Assembly-CSharp/GameParameters.cs
- L=941 kind=class type=GameParameters bases=IConfigNode anc=IConfigNode ns=-
- unity(4): none | ksp: ConfigNode=16, Game=10, Localizer=7, HighLogic=5, GameSettings=2 | fan_in=35
- methods(16): CustomParams, Enabled, GenerateParameterTypes, GetDefaultParameters, GetPresetColor, GetPresetColorHex, Interactible, KerbalExperienceEnabled, Load, LoadCustomNode, OnLoad, OnSave, Save, SetDifficultyPreset, SetDifficultyPresets, ValidValues
- auto: A / Save

## Assembly-CSharp/GameSettings.cs
- L=2068 kind=MonoBehaviour type=GameSettings bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(62): KeyCode=129, QualitySettings/SystemInfo=13, Application.=10, Screen/Cursor=5, Time.=4, Lifecycle=3 | ksp: ConfigNode=20, GameSettings=9, GameEvents=3, TimeWarp=2, Game=1 | fan_in=256
- methods(41): AddIncrementalSpeedMultiplierValue, ApplyEngineSettings, ApplySettings, Awake, DetectKeyboardLayout, GetAxiSpeedMultiplierIndex, GetAxisIncrementalSpeedMultiplierValues, GetAxisSpeedMultiplier, GetFireworkPresetColors, GetLightPresetColors, InitializeAxisGroupbindings, InitializeDefaultValues, LoadColor, LoadGameSettingsOnly, LoadLayoutKeyBindings, OnDestroy, OnGameSave, ParseCfg, ParseLauncherCfg, ParseLayoutRecursive, ParseLayoutsCfg, ReloadSettings, ResetSettings, SaveGameSettingsOnly ...
- auto: B / Input

## Assembly-CSharp/InputSettings.cs
- L=1440 kind=class type=InputSettings bases=DialogGUIVerticalLayout,ISettings anc=DialogGUIVerticalLayout/DialogGUILayoutBase/DialogGUIBase ns=-
- unity(21): KeyCode=12, UnityEngine.UI=7, SetActive=7, Input.=3, GetComponent=3, Lifecycle=1 | ksp: GameSettings=337, ConfigNode=8, Localizer=4, MapView=2, FlightGlobals=2 | fan_in=2
- methods(23): ApplyAxisBinding, ApplyKeyBinding, ApplySettings, DrawAxisBinding, DrawKeyBinding, DrawMiniSettings, DrawSettings, GetName, GetSettings, GetValidKeys, ListenToAxis, ListenToKey, OnDismissListenModeDialog, OnUpdate, ResetAxisValues, SetAxis, SetKey, SpawnListenModeDialog, Update, drawInputPage, drawListenModeWindow, drawOtherPage, isEnabled
- auto: C / Input

## Assembly-CSharp/iT.cs
- L=1281 kind=static class type=iT bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: C / UI

## Assembly-CSharp/iTween.cs
- L=4414 kind=MonoBehaviour type=iTween bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(72): Transform=275, GameObject=117, GetComponent=96, Audio=19, Renderer=17, Material=17 | ksp: none | fan_in=0
- methods(165): ApplyAudioToTargets, ApplyColorTargets, ApplyColorToTargets, ApplyFloatTargets, ApplyLookToTargets, ApplyMoveByTargets, ApplyMoveToPathTargets, ApplyMoveToTargets, ApplyPunchPositionTargets, ApplyPunchRotationTargets, ApplyPunchScaleTargets, ApplyRectTargets, ApplyRotateAddTargets, ApplyRotateToTargets, ApplyScaleToTargets, ApplyShakePositionTargets, ApplyShakeRotationTargets, ApplyShakeScaleTargets, ApplyStabTargets, ApplyTween, ApplyVector2Targets, ApplyVector3Targets, AudioFrom, AudioTo ...
- auto: C / UI

## Assembly-CSharp/KerbalEVA.cs
- L=7368 kind=PartModule type=KerbalEVA bases=PartModule anc=PartModule/MonoBehaviour ns=-
- unity(64): Transform=258, GameObject=78, Rigidbody=58, Coroutine=40, SetActive=35, GetComponent=35 | ksp: GameSettings=87, KSPField=67, GameEvents=50, Localizer=34, FlightGlobals=32 | fan_in=34
- methods(269): AcquirePosition, AcquireRotation, AddFlag, AddRBAnchor, AnchorUpdate, ApplyWingnutTorque, AutoTransition, Banana, BoardPart, BoardSeat, CacheLocalStrings, CalculateGroundLevelAngle, CanEVAWithoutHelmet, CanPlantFlag, CanRecover, CanSafelyRemoveHelmet, ChangeHelmet, ChangeNeckRing, CheckHelmetOffSafe, CheckLadderTriggers, ClearLadderHold, CompareContactsByNormalToSurface, ControlPanelInteractionFinished, Dzhanibekov ...
- auto: B / Kerbals

## Assembly-CSharp/KerbalRoster.cs
- L=1397 kind=class type=KerbalRoster bases=- anc=- ns=-
- unity(9): Coroutine=8, GetComponent=1 | ksp: ProtoCrewMember=131, HighLogic=17, Localizer=15, Game=12, GameDatabase=9 | fan_in=33
- methods(54): AddApplicant, AddCrewMember, AddExperienceType, CalculateExperience, CalculateExperienceLevel, ChangeName, ChangeNameCalledFromPCM, CheatExperience, CheckRosterRespawn, DefaultCrewForVessel, Exists, ExperienceAddFlight, ExperienceBodyValue, ExperienceHomeTypeValue, ExperienceNotHomeTypeValue, GenerateExperienceEffectTypes, GenerateExperienceLog, GenerateExperienceTraitTypes, GenerateExperienceTypes, GenerateInitialCrewRoster, GetActiveCrewCount, GetAssignedCrewCount, GetAvailableCrewCount, GetExperienceEffectType ...
- auto: A / Kerbals

## Assembly-CSharp/KSP/UI/Dialogs/KerbNetDialog.cs
- L=1224 kind=MonoBehaviour type=KerbNetDialog bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Dialogs
- unity(53): Texture=10, TMPro=10, UnityEngine.UI=9, GameObject=8, SetActive=6, Coroutine=5 | ksp: Vector3d=20, Localizer=14, GameEvents=6, CelestialBody=5, HighLogic=3 | fan_in=6
- methods(55): ActivateDisplayMode, AddKnownMode, AutoRefreshRoutine, Awake, CacheLocalStrings, ChangeMapPosition, Close, CreateMapTexture, DeactivateDisplayMode, Display, DrawAnomaliesOnMap, DrawContourLines, FullRefresh, GenerateAssemblyModes, GenerateKnownModes, GenerateResourceModes, GetModeByName, GetScanLatitudeAndLongitude, GetTexture, HasActiveConnection, HasValidAccessor, HasValidDisplayMode, HasVesselControl, NormalizedDistanceFromCenter ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/CraftBrowserDialog.cs
- L=1544 kind=MonoBehaviour type=CraftBrowserDialog bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(75): UnityEngine.UI=43, SetActive=39, GameObject=39, Steam=33, TMPro=16, Analytics=13 | ksp: Localizer=36, HighLogic=5, ConfigNode=5, GameSettings=3, InputLockManager=2 | fan_in=19
- methods(66): AddCraftEntryWidget, BuildPlayerCraftList, BuildSteamCraftList, BuildSteamSubscribedCraftList, BuildStockCraftList, CancelledCallback, ClearCraftEntry, ClearCraftList, ClearOldCraftEntries, ClearSelection, ConfirmLoadCraft, Dismiss, FilterUpdateSelectables, GetAllGameFiles, GetMissionCraftFiles, GetPlayerCraftFiles, Hide, HideForLater, InitializeMenuNavigation, LoadSteamItemPreviewURL, OnBtnSteamOverlay, OnDeleteConfirm, OnDestroy, OnDisable ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Editor/PartListTooltip.cs
- L=812 kind=MonoBehaviour type=PartListTooltip bases=Tooltip anc=Tooltip/MonoBehaviour ns=KSP.UI.Screens.Editor
- unity(68): SetActive=28, GameObject=24, Transform=21, TMPro=10, UnityEngine.UI=9, Instantiate/Destroy=6 | ksp: Localizer=26, PartModule=19, AvailablePart=12, Part=5, ResearchAndDevelopment=4 | fan_in=5
- methods(22): CacheLocalStrings, CreateExtendedInfo, CreateExtendedUpgradeInfo, DestroyExtendedInfo, DisplayExtendedInfo, GetNewTooltipWidget, GetPrimaryInfo, GetUpgradedPrimaryInfo, HidePreviousTooltipWidgets, OnDestroy, PartStatsDelegate, Setup, SetupGrayout, SetupScaleGauge, SetupUpgradeInfo, Update, UpdateCargoPartModuleInfo, UpdateVariantText, _GenerateStatsText, _GetPartStats, onBtnPurchaseRed, onPurchaseButton
- auto: C / Editor

## Assembly-CSharp/KSP/UI/Screens/EditorActionGroups.cs
- L=1963 kind=MonoBehaviour type=EditorActionGroups bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI.Screens
- unity(61): GameObject=43, GetComponent=41, UnityEngine.UI=29, Transform=23, EventSystems=12, KeyCode=6 | ksp: EditorLogic=31, Part=29, HighLogic=21, FlightGlobals=14, GameEvents=10 | fan_in=13
- methods(59): ActivateInFlightInterface, ActivateInterface, AddActionToGroup, AddToController, AddToGroup, AddToSelection, Awake, CacheCurrentSelectable, ClearSelection, CloseGroup, ConstructActionList, ConstructGroupActionList, ConstructGroupAxisList, ConstructGroupList, ConstructLists, ConstructPartActionList, ConstructPartAxisList, ConstructPartList, CreateGroups_Action, CreateGroups_Axis, CreateGroups_Controller, DeactivateInterface, DectivateInFlightInterface, GetCurrentSelectableColumn ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorPartIcon.cs
- L=978 kind=MonoBehaviour type=EditorPartIcon bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(77): GameObject=52, SetActive=30, Transform=18, Material=16, UnityEngine.UI=15, GetComponent=10 | ksp: AvailablePart=12, GameEvents=9, HighLogic=7, GameDatabase=5, EditorLogic=5 | fan_in=10
- methods(37): Awake, CheckExperimental, CheckMissionRequired, CleanUpMaterials, Create, CreateMaterialArray, DisableAddButton, DisableDeleteButton, EnableAddButton, EnableDeleteButton, GetCurrentVariant, Highlight, InstantiatePartIcon, LoadPartThumbnail, MouseInput_Add, MouseInput_Delete, MouseInput_PlacePart, MouseInput_PointerEnter, MouseInput_PointerExit, MouseInput_SpawnPart, OnDestroy, OnEditorDefaultVariantChanged, OnEditorPartEvent, OnInventoryPartOnMouseChanged ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Flight/KerbalPortraitGallery.cs
- L=903 kind=MonoBehaviour type=KerbalPortraitGallery bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(62): GameObject=22, SetActive=19, UnityEngine.UI=12, Transform=7, Coroutine=7, Lifecycle=6 | ksp: GameEvents=13, KerbalEVA=12, FlightGlobals=8, Vessel=4, Localizer=4 | fan_in=5
- methods(35): Awake, CheckInvalidActiveCrew, ClearPortraits, DespawnInactivePortraits, DespawnPortrait, GetMaxGalleryCapacity, GetPortrait, HoverArea_onPointerEnter, HoverArea_onPointerExit, OnCameraChange, OnCommandSeatInteraction, OnDestroy, OnDisable, OnEnable, OnKerbalLevelUp, RegisterActiveCrew, SetActivePortraitsForVessel, SpawnPortrait, Start, StartRefresh, StartReset, ToggleIVAOverlay, UIControlsUpdate, UnregisterActiveCrew ...
- auto: C / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapNode.cs
- L=958 kind=UIBehaviour type=MapNode bases=Selectable anc=Selectable ns=KSP.UI.Screens.Mapview
- unity(63): GameObject=17, SetActive=13, UnityEngine.UI=8, Transform=8, EventSystems=7, TMPro=4 | ksp: Vector3d=14, InputLockManager=5, Localizer=4, MapView=2, GameEvents=2 | fan_in=20
- methods(30): AttachLabel, Awake, CacheLocalStrings, CaptionUpdate, CheckAndEnablePinnedCaption, Create, FormatCaption, GetIconIndex, HideAddAlarm, Init, MoveAwayFromEachOthers, NodeUpdate, OnAddAlarmPointerExit, OnDeselect, OnDestroy, OnEnable, OnMapEntered, OnPointerDown, OnPointerEnter, OnPointerExit, OnPointerUp, OnSelect, SetBackground, SetIcon ...
- auto: C / MapView

## Assembly-CSharp/KSP/UI/Screens/MessageSystem.cs
- L=820 kind=MonoBehaviour type=MessageSystem bases=UIApp anc=UIApp/MonoBehaviour ns=KSP.UI.Screens
- unity(60): Transform=21, GameObject=14, Instantiate/Destroy=8, SetActive=7, TMPro=5, UnityEngine.UI=4 | ksp: GameEvents=11, ConfigNode=9, Localizer=7, GameSettings=6, HighLogic=5 | fan_in=9
- methods(44): AddMessage, AddMessageInternal, AddMessageInternalNonDuplicate, Awake, ClearAllMessages, DeleteAllMessages, DiscardMessage, DiscardSelected, DisplayApp, Equals, FindMessages, GetAppScenes, GetAppScreenPos, GetHashCode, GetRichTextMessageTitle, HasNextMessage, HasPreviousMessage, HideApp, Instantiate, InstantiateAdvancedItem, InstantiatePopup, LoadMessage, LoadMessages, MouseInputAdvMessageItem ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/PartCategorizer.cs
- L=2181 kind=MonoBehaviour type=PartCategorizer bases=BasePartCategorizer anc=BasePartCategorizer/MonoBehaviour ns=KSP.UI.Screens
- unity(41): Coroutine=17, GameObject=14, Transform=11, EventSystems=10, SetActive=8, Instantiate/Destroy=7 | ksp: AvailablePart=98, ConfigNode=26, PartLoader=17, Localizer=14, EditorLogic=10 | fan_in=11
- methods(99): AcceptCriteria, AcceptCriteriaPopupAddPart, AcceptNewCategory, AcceptNewSubcategory, AcceptNewSubcategoryCriteria, AcceptNewSubcategorySubassembly, AddCustomCategory, AddCustomCategorySubassembly, AddCustomFilter, AddCustomSubcategoryFilter, AddPart, AddPartToCategory, AddPartToCustomCategoryViaPopup, AddSubassembly, AddSubassemblyToSelectedCategory, AddSubcategory, Awake, CompileExclusionFilter, CompileExclusionFilterSubassembly, CreateCrossSectionFilters, CreateManufacturerFilters, CreateModuleFilters, CreateResourceFilters, CreateTechTierFilters ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/SpaceTracking.cs
- L=1109 kind=MonoBehaviour type=SpaceTracking bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(59): GameObject=29, SetActive=20, UnityEngine.UI=13, Coroutine=10, Instantiate/Destroy=8, GetComponent=6 | ksp: HighLogic=47, GameEvents=30, Vessel=25, Game=18, Localizer=17 | fan_in=7
- methods(47): Awake, BtnOnClick_DeleteSelectedVessel, BtnOnClick_FlySelectedVessel, BtnOnClick_LeaveTrackingStation, BtnOnclick_RecoverSelectedVessel, BtnOnclick_TrackSelectedVessel, ClearUIList, CompleteStartUp, ConstructUIList, FlyVessel, GoToAndFocusVessel, MissionModeValid, MissionsObjectiveCallback, OnDestroy, OnDialogDismiss, OnMapViewFiltersModified, OnPlanetariumTargetChanged, OnRecoverConfirm, OnSceneLoadRequested, OnVesselDeleteConfirm, PostInitTgtFocus, RefreshMissionRequest, RequestVessel, SetVessel ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/StageGroup.cs
- L=998 kind=MonoBehaviour type=StageGroup bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI.Screens
- unity(68): Transform=37, GameObject=22, UnityEngine.UI=19, SetActive=15, EventSystems=8, GetComponent=5 | ksp: GameEvents=13, Part=10, HighLogic=4, GameSettings=2, Localizer=1 | fan_in=3
- methods(48): AddIcon, AddIconAt, AddIcons, AddStageAfter, Awake, CacheLocalStrings, Delete, DeltaVAppAtmosphereChanged, DeltaVCalcsCompleted, DisableDeltaVHeading, DisableInfoPanel, DisableStockInfoPanelDisplays, EditorStarted, EnableDeltaVHeading, EnableInfoPanel, EnableStockInfoPanelDisplays, FindSymmetryGroupleader, FixGroupUIState, GetStageGroupIndex, Hover, HoverOut, MoveAllIconsInto, MoveIconsInto, OnBeginDrag ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/StageIcon.cs
- L=1411 kind=MonoBehaviour type=StageIcon bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(58): SetActive=27, GameObject=27, Transform=22, UnityEngine.UI=13, EventSystems=9, Texture=3 | ksp: Part=74, GameEvents=3, HighLogic=2, GameDatabase=1 | fan_in=4
- methods(47): AddToGroup, Awake, Blink, BlinkBorder, ClearInfoBoxes, CollapseGroup, ConsolidateMembers, DisableGroup, DisplayInfo, ExpandGroup, ExpandGroupInUIOnly, ForceDisplayInfo, Freeze, Highlight, HighlightPart, HoldGroupedIcons, MouseInput_pointerEnter, MouseInput_pointerExit, OnBeginDrag, OnClick, OnDrag, OnEndDrag, OnFalse, OnTrue ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/StageManager.cs
- L=1801 kind=MonoBehaviour type=StageManager bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(57): Transform=48, GameObject=21, UnityEngine.UI=20, SetActive=12, Instantiate/Destroy=11, Coroutine=6 | ksp: Part=45, GameEvents=40, HighLogic=17, FlightGlobals=9, MapView=8 | fan_in=33
- methods(81): ActivateNextStage, ActivateStage, AddHeldIconsToStages, AddNewStageGroupsIfNeeded, AddPartsWithStageiconsToListRecursive, AddStageAt, Awake, BeginFlight, CacheLocalStrings, CheckNullrefsInStageIconList, CreateIcon, DecrementCurrentStage, DeleteEmptyStages, DeleteStage, DeltaVAppAtmosphereChanged, DeltaVButtonClicked, DeltaVCalcsCompleted, DisableDeltaVHeadings, DisableDeltaVTotal, DisableIcon, DisableInfoPanels, EnableDeltaVHeadings, EnableDeltaVTotal, EnableInfoPanels ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/VesselSpawnDialog.cs
- L=1526 kind=MonoBehaviour type=VesselSpawnDialog bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(76): UnityEngine.UI=36, GameObject=33, SetActive=32, Steam=25, Coroutine=20, TMPro=18 | ksp: Localizer=50, HighLogic=21, GameEvents=9, ConfigNode=9, GameSettings=3 | fan_in=6
- methods(60): AddCraftEntryWidget, AddVesselDataItem, AddVesselListItem, AstronautComplexDespawn, AstronautComplexSpawn, Awake, BuildSteamCraftList, ButtonClose, ButtonDelete, ButtonEdit, ButtonLaunch, ClearCraftList, ClearSelection, ConfirmLaunch, CraftSelectedCallback, CreateVesselList, GetCostTextColorHex, GetMassTextColorHex, GetPartCountTextColorHex, GetSizeTextColorHex, GetSteamFileId, GetVesselDataItem, InitiateGUI, LateSelectFireThing ...
- auto: C / UI

## Assembly-CSharp/KSPUtil.cs
- L=2176 kind=static class type=KSPUtil bases=- anc=- ns=-
- unity(22): Transform=29, GameObject=5, Application.=5, MonoBehaviour=3, Texture=2, Screen/Cursor=2 | ksp: Localizer=43, Vector3d=12, QuaternionD=10, Part=4, GameSettings=3 | fan_in=212
- methods(92): Add, AddUnits, AppendTimeToStringBuilder, AppendValueToString, BearingDegrees, BearingRadians, CacheLocalStrings, CalculateFolderSize, CheckDOSName, CheckVersion, ClampRectToScreen, DateTimeNow, DeepCompare, Diag, FindComponentsImplementing, FindInPartModel, FindTagsInChildren, FindTransformAtIndexPath, GenerateFilePathWithDate, GenerateSuperSeed, GenerateTexture2DArray, GetActivator, GetAttachNodeInfo, GetDateFromUT ...
- auto: B / Core

## Assembly-CSharp/LoadGameDialog.cs
- L=934 kind=MonoBehaviour type=LoadGameDialog bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(52): UnityEngine.UI=8, GameObject=8, Transform=6, Texture=6, Instantiate/Destroy=5, GetComponent=3 | ksp: Localizer=25, ConfigNode=22, Game=10, PartLoader=4, ProtoVessel=1 | fan_in=6
- methods(29): AreVesselsInFlightCompatible, CancelLoadGame, ClearListItems, ConfirmDeleteGame, ConfirmLoadGame, Create, CreateLoadList, DismissDeleteGame, FinishedCallback, GetLastWriteTime, GetSFSMD5, GetSaveData, IsCompatible, Load, LoadDetailsFromGame, LoadFromMetaFile, LoadGame, LoadMenuNav, OnBtnCancel, OnButtonDelete, OnButtonLoad, OnSelectionChanged, PersistentLoadGame, Save ...
- auto: C / Save

## Assembly-CSharp/MainMenu.cs
- L=1023 kind=MonoBehaviour type=MainMenu bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(60): GameObject=31, SetActive=25, Texture=6, UnityEngine.UI=5, TMPro=3, Lifecycle=3 | ksp: Localizer=39, GameSettings=23, HighLogic=22, InputLockManager=17, Game=16 | fan_in=5
- methods(52): Awake, BuyExpansionPack, BuyExpansionPack2, CancelNewGame, CancelOverwriteNewGame, ConfirmNewGame, CreateAndStartGame, CreateNewGameDialog, DeleteDirectory, GoToMerchandisingSite, HighlightFirstItem, HighlightNextItem, LoadGame, MenuNavHandler, MissionEditor, MouseIsHovering, NewGame, OnApplicationFocus, OnDestroy, OnDifficultyOptionsDismiss, OnFlagCancel, OnFlagSelect, OnInputLocksModified, OnLoadDialogFinished ...
- auto: C / Scene

## Assembly-CSharp/ManeuverNodeEditorManager.cs
- L=814 kind=MonoBehaviour type=ManeuverNodeEditorManager bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(52): UnityEngine.UI=10, GameObject=7, SetActive=6, Lifecycle=4, Instantiate/Destroy=4, EventSystems=4 | ksp: FlightGlobals=35, GameEvents=20, MapView=11, Localizer=11, HighLogic=3 | fan_in=12
- methods(35): AssignSelectedManeuverNode, Awake, CacheLocalStrings, ChangeTooltipText, DeleteManNode, GetTabToggle, ManNodeCreateCooldown, ModifyBurnVector, NextManNode, OnDestroy, OnManNodeDeselected, OnManNodeSelected, OnMapEntered, OnMapExited, OnPointerEnter, OnPointerExit, OnSceneSwitch, OnTabButtonPressed, OnTargetChanged, PrevManNode, Reset, SetCurrentTab, SetManeuverNodeInitialValues, SetMouseOverGizmo ...
- auto: C / MapView

## Assembly-CSharp/MapSO.cs
- L=801 kind=ScriptableObject type=MapSO bases=ScriptableObject anc=ScriptableObject ns=-
- unity(41): Texture=34, ScriptableObject=1 | ksp: none | fan_in=10
- methods(25): CompileGreyscale, CompileHeightAlpha, CompileRGB, CompileRGBA, CompileToTexture, ConstructBilinearCoords, CreateGreyscaleFromAlpha, CreateGreyscaleFromRGB, CreateHeightAlpha, CreateMap, CreateMapRGBQuantized, CreateRGB, CreateRGBA, CreateRGBQuantized, GetPixelByte, GetPixelColor, GetPixelColor32, GetPixelFloat, GetPixelHeightAlpha, GreyByte, GreyFloat, Lerp, PixelByte, PixelIndex ...
- auto: B / Terrain

## Assembly-CSharp/METutorialScenario.cs
- L=864 kind=ScenarioModule type=METutorialScenario bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(17): EventSystems=1 | ksp: Orbit=2, HighLogic=1 | fan_in=14
- methods(77): AddTutorialPage, At, CanSkipCurrentTutorialOnNodeDragEnd, ClearFilter, Count, CreateDialog, CreateTutorialPages, DestroyTutorialPage, DragNodeHelper, EnableAllTutorialPageButtons, EnableMeguiParameter, EnableTutorialPageButton, FilterNodes, FindButtons, GetApplyScoreScoreParam, GetDialogMessageParam, GetFacilityParam, GetLaunchSiteParam, GetMEGUINode, GetMEGUINodeIndex, GetMEGUINodes, GetMENode, GetMENodeByTemplateName, GetNextTutorialName ...
- auto: A / DLC-MakingHistory

## Assembly-CSharp/ModuleCargoBay.cs
- L=942 kind=PartModule type=ModuleCargoBay bases=PartModule,IAirstreamShield anc=PartModule/MonoBehaviour ns=-
- unity(46): GameObject=10, Collider=8, LayerMask=5, Transform=4, Time.=4, Raycast/Query=3 | ksp: Part=32, AttachNode=14, HighLogic=12, KSPField=11, GameEvents=8 | fan_in=4
- methods(50): AddConnectingPart, ClearConnectingParts, ClosedAndLocked, ColliderListCleanUp, ColliderReset, CountAdjustersForcingCargoBayStuck, DisableShieldedVolume, EnableShieldedVolume, EndCapped, FindAttachNodes, FindBayColliders, FindConnectingNodes, FindEnclosedParts, FindNearbyParts, FindPartColliders, ForceCargoBayStuck, GetModuleDisplayName, GetOpposingNode, GetPart, GetVessel, ModifyShieldInEnclosedParts, NotifyShieldModified, OnAwake, OnDestroy ...
- auto: B / PartModules-Structural

## Assembly-CSharp/ModuleComet.cs
- L=815 kind=PartModule type=ModuleComet bases=PartModule,IVesselAutoRename,IPartMassModifier anc=PartModule/MonoBehaviour ns=-
- unity(63): GameObject=20, ParticleSystem=16, Transform=14, Instantiate/Destroy=5, SetActive=4, GetComponent=4 | ksp: KSPField=15, HighLogic=7, GameSettings=7, Vector3d=5, Localizer=5 | fan_in=17
- methods(32): ActivateGeysers, ActivateNearDust, CacheLocalStrings, CheckExplosion, CreateEmittersOnSurface, GetModuleMass, GetModuleMassChangeWhen, GetVesselName, GetVesselType, MakeTarget, OnAwake, OnDestroy, OnFixedUpdate, OnGenComplete, OnLoad, OnSave, OnStart, OnTimeWarpRateChanged, OnUpdate, PostInit, Rangefinder, RenameCometEvent, SetCometMass, SetDust ...
- auto: C / Celestials

## Assembly-CSharp/ModuleControlSurface.cs
- L=994 kind=PartModule type=ModuleControlSurface bases=ModuleLiftingSurface,IMultipleDragCube,ITorqueProvider anc=ModuleLiftingSurface/PartModule/MonoBehaviour ns=-
- unity(47): Transform=14, Rigidbody=14, Lifecycle=3, GameObject=2 | ksp: KSPField=23, KSPAction=14, Localizer=11, GameEvents=10, HighLogic=9 | fan_in=5
- methods(44): ActionExtend, ActionRetract, ActionSign, ActionToggle, ActivateAllControls, ApplyActuatorSpeedAdjustments, AssumeDragCubePosition, CacheLocalStrings, CalcAngleOfAttack, CalculateCoM, CtrlSurfaceEditorUpdate, CtrlSurfaceUpdate, DeactivateAllControls, FixedCtrlSurfaceUpdate, FixedUpdate, GetDragCubeNames, GetInfo, GetModuleDisplayName, GetPotentialLift, GetPotentialTorque, LateUpdate, ModifyAuthorityLimiter, ModifyAuthorityLimiterUI, OnAwake ...
- auto: B / Aero

## Assembly-CSharp/ModuleDataTransmitter.cs
- L=827 kind=PartModule type=ModuleDataTransmitter bases=PartModule,IScienceDataTransmitter,IResourceConsumer anc=PartModule/MonoBehaviour ns=-
- unity(34): Coroutine=21, GameObject=1 | ksp: Localizer=41, KSPField=13, ConfigNode=7, HighLogic=5, TimeWarp=3 | fan_in=3
- methods(43): AbortTransmission, ApplyPowerAdjustments, CanComm, CanCommUnloaded, CanScienceTo, CanSetFXModules, CanTransmit, CheckContractObjectiveValidity, CommPowerUnloaded, EvaluateScienceMultiplier, GetConsumedResources, GetContractObjectiveType, GetDrawModulePanelCallback, GetFxModuleScalar, GetInfo, GetModuleDisplayName, GetModuleTitle, GetModulesScalarMax, GetModulesScalarMin, GetPrimaryField, GetVesselSignalStrength, IsBusy, IsDataTransmitterBroken, OnAwake ...
- auto: A / CommNet

## Assembly-CSharp/ModuleDeployablePart.cs
- L=1423 kind=PartModule type=ModuleDeployablePart bases=PartModule,IMultipleDragCube,IScalarModule anc=PartModule/MonoBehaviour ns=-
- unity(49): Transform=25, GameObject=15, Rigidbody=11, GetComponent=5, Animation=5, Lifecycle=3 | ksp: KSPField=39, Localizer=27, HighLogic=22, FlightGlobals=10, Planetarium=4 | fan_in=13
- methods(53): AllowConstructionDeployState, AssumeDragCubePosition, CacheLocalStrings, CalculateTracking, CalculateTrackingLOS, CanBeDetached, CanBeOffset, CanBeRotated, CheatRepair, ControlPanelsWithoutUsingSymmetry, DoExtend, DoRepair, DoRetract, EventRepairExternal, Extend, ExtendAction, ExtendPanelsAction, FindAnimations, FixedUpdate, GetDragCubeNames, GetInfo, GetModuleDisplayName, GetTrackingBodyTransforms, IsDeployablePartStuck ...
- auto: B / Part

## Assembly-CSharp/ModuleDockingNode.cs
- L=2059 kind=PartModule type=ModuleDockingNode bases=PartModule,ITargetable,IStageSeparator anc=PartModule/MonoBehaviour ns=-
- unity(35): Joint=14, Transform=10, GameObject=7, Lifecycle=4, Coroutine=4, Rigidbody=2 | ksp: KSPField=48, GameEvents=21, FlightGlobals=17, Part=14, HighLogic=13 | fan_in=7
- methods(93): ApplyCoordsUpdate, CalculateResourceDrain, CheckContractObjectiveValidity, CheckDockContact, CrossfeedToggleableEditor, CrossfeedToggleableFlight, Decouple, DecoupleAction, DemoteToPhysicslessPart, DestroySameVesselJoint, DisableXFeed, DisableXFeedAction, DisengageNodeLock, DockToSameVessel, DockToVessel, EnableXFeed, EnableXFeedAction, EngageNodeLock, FindNodeApproaches, FindOtherNode, FixedUpdate, GetActiveTargetable, GetConsumedResources, GetContractObjectiveType ...
- auto: B / PartModules-Structural

## Assembly-CSharp/ModuleEngines.cs
- L=2217 kind=PartModule type=ModuleEngines bases=PartModule,IEngineStatus,IModuleInfo anc=PartModule/MonoBehaviour ns=-
- unity(41): Transform=33, Instantiate/Destroy=14, GameObject=11, ParticleSystem=6, Light=5, GetComponent=4 | ksp: KSPField=80, Localizer=28, FloatCurve=16, GameEvents=9, TimeWarp=8 | fan_in=19
- methods(82): Activate, ActivateAction, ActivatePowerFX, ActivateRunningFX, ApplyThrottleAdjustments, AutoPlaceFXGroup, BurstFlameoutGroups, CacheLocalStrings, CalculateThrust, CanBeDetached, CanBeOffset, CanBeRotated, CheckDeprived, CheckTransformsUnderwater, DeactivateLoopingFX, DeactivatePowerFX, DeactivateRunningFX, EngineExhaustDamage, FXReset, FXUpdate, FixedUpdate, Flameout, GetConsumedResources, GetCurrentThrust ...
- auto: B / PartModules-Engines

## Assembly-CSharp/ModuleGrappleNode.cs
- L=996 kind=PartModule type=ModuleGrappleNode bases=PartModule,IActiveJointHost,IJointLockState anc=PartModule/MonoBehaviour ns=-
- unity(48): Analytics=12, Transform=10, GameObject=9, Coroutine=5, Raycast/Query=3, Lifecycle=3 | ksp: Part=20, FlightGlobals=12, KSPField=10, HighLogic=9, KSPEvent=6 | fan_in=4
- methods(43): CheckContractObjectiveValidity, CheckGrappleContact, CountAdjustersBlockingGrappleRelease, Decouple, DecoupleAction, DestroySameVesselJoint, FindContactParts, FixedUpdate, GetContractObjectiveType, GetHostPart, GetInfo, GetLocalTransform, GetModuleDisplayName, Grapple, GrappleSameVessel, IsAdjusterBlockingGrappleGrab, IsJointUnlocked, IsLoose, LateUpdate, LockPivot, MakeReferenceToggle, MakeReferenceTransform, NodeIsTooFar, OnDriveModeChanged ...
- auto: B / PartModules-Structural

## Assembly-CSharp/ModuleInventoryPart.cs
- L=2002 kind=PartModule type=ModuleInventoryPart bases=PartModule,IPartMassModifier,IPartCostModifier anc=PartModule/MonoBehaviour ns=-
- unity(47): Transform=31, GameObject=10, GetComponent=8, Time.=6, Renderer=6, Coroutine=6 | ksp: Part=22, GameEvents=22, ConfigNode=22, HighLogic=19, GameSettings=15 | fan_in=26
- methods(72): CanStackInSlot, CancelPartPlacementMode, CheckPartStorage, ClearPartAtSlot, ContainsPart, CreatePartForInventoryUse, CreatePartNode, CreatePartObject, DeletePartObject, DeployGroundPart, DeployInventoryItem, FirstEmptySlot, FirstFullSlot, GetInfo, GetModuleCost, GetModuleCostChangeWhen, GetModuleDisplayName, GetModuleMass, GetModuleMassChangeWhen, GetPartMass, GetProtoVesselNode, GetStackAmountAtSlot, GetStackCapacityAtSlot, GetVariantNameForSlot ...
- auto: B / Kerbals

## Assembly-CSharp/ModuleLight.cs
- L=1269 kind=PartModule type=ModuleLight bases=PartModule,IResourceConsumer,IScalarModule anc=PartModule/MonoBehaviour ns=-
- unity(62): Shader=26, Material=24, Transform=11, Renderer=10, Light=10, Animation=5 | ksp: KSPField=34, Localizer=8, KSPEvent=6, HighLogic=6, GameEvents=6 | fan_in=6
- methods(47): ApplyIntensityAdjustments, Blink, BlinkLightOff, BlinkLightOn, CacheLocalStrings, FixedUpdate, GetAngleVector, GetAnims, GetConsumedResources, GetCurrentColor, GetInfo, GetModuleDisplayName, IsMoving, LightOffAction, LightOnAction, LightVarsModified, LightsOff, LightsOn, ModifyCurrentBlinkTime, ModifyLightVector, OnAwake, OnColorChanged, OnDestroy, OnInventoryModeDisable ...
- auto: C / Rendering

## Assembly-CSharp/ModuleParachute.cs
- L=989 kind=PartModule type=ModuleParachute bases=PartModule,IMultipleDragCube anc=PartModule/MonoBehaviour ns=-
- unity(45): GameObject=12, SetActive=10, Transform=9, Animation=4, Rigidbody=2, Lifecycle=2 | ksp: KSPField=34, Localizer=16, PhysicsGlobals=14, Vessel=5, KSPEvent=4 | fan_in=13
- methods(39): AssumeDragCubePosition, CacheLocalStrings, CalcBaseStats, CutAction, CutParachute, Deploy, DeployAction, DeploySafe, Disarm, FixedUpdate, GetAnimationNormalizedTime, GetArea, GetDragCubeNames, GetInfo, GetModuleDisplayName, IsAnimationPlaying, IsMovingFastEnoughToDeploy, OnActive, OnAwake, OnDestroy, OnLoad, OnParachuteFullyDeployed, OnParachuteSemiDeployed, OnPartUnpack ...
- auto: B / PartModules-Structural

## Assembly-CSharp/ModuleProceduralFairing.cs
- L=2220 kind=PartModule type=ModuleProceduralFairing bases=PartModule,IPartMassModifier,IPartCostModifier anc=PartModule/MonoBehaviour ns=-
- unity(65): Transform=33, Rigidbody=33, Material=21, Shader=18, SetActive=13, Texture=12 | ksp: KSPField=40, Part=33, GameEvents=25, HighLogic=22, EditorLogic=22 | fan_in=10
- methods(93): AddFlagToFairing, AddPanelColliders, ApplyVariantToPanel, AssumeClosedDragCube, AssumeDeployedDragCube, BuildFairing, CacheLocalStrings, CheckForFairingEdit, ClampH, ClampR, DecoupleAction, DeleteFairing, DeployFairing, DeployFairingAction, DumpInterstage, EditFairing, FindPayloadFromTop, FixedUpdate, GetDrawModulePanelCallback, GetFairingArea, GetInfo, GetModuleCost, GetModuleCostChangeWhen, GetModuleDisplayName ...
- auto: B / PartModules-Structural

## Assembly-CSharp/ModuleRCS.cs
- L=978 kind=PartModule type=ModuleRCS bases=PartModule,IResourceConsumer,ITorqueProvider anc=PartModule/MonoBehaviour ns=-
- unity(39): Transform=11, GameObject=9, Lifecycle=3, SetActive=1, Resources.=1, ParticleSystem=1 | ksp: KSPField=34, Localizer=10, ConfigNode=6, TimeWarp=4, PartResourceDefinition=4 | fan_in=5
- methods(41): ApplyInputLinearAdjustments, ApplyInputRotationAdjustments, CacheLocalStrings, CalculateThrust, CanBeDetached, CanBeOffset, CanBeRotated, DeactivateFX, DeactivatePowerFX, FindThrusters, FixedUpdate, GetConsumedResources, GetInfo, GetLeverDistance, GetLeverDistanceOriginal, GetModuleDisplayName, GetPotentialTorque, GetStagingDisableText, GetStagingEnableText, IsAdjusterBreakingRCS, IsStageable, OnActive, OnAwake, OnDestroy ...
- auto: B / PartModules-Engines

## Assembly-CSharp/ModuleScienceExperiment.cs
- L=1239 kind=PartModule type=ModuleScienceExperiment bases=PartModule,IScienceDataContainer anc=PartModule/MonoBehaviour ns=-
- unity(36): Coroutine=18, GameObject=8, SetActive=4, ScriptableObject=2, Lifecycle=2 | ksp: Localizer=42, KSPField=36, GameEvents=15, KSPEvent=8, Vessel=5 | fan_in=7
- methods(65): CanBeDetached, CanBeOffset, CanBeRotated, CleanUpExperimentExternal, CollectDataExternalEvent, DeployAction, DeployExperiment, DeployExperimentExternal, DumpData, FindContainer, GetData, GetInfo, GetModuleDisplayName, GetScienceCount, GetStagingDisableText, GetStagingEnableText, IsRerunnable, IsStageable, Load, OnActive, OnAwake, OnCommandSeatInteraction, OnDataTransfer, OnDestroy ...
- auto: B / Science

## Assembly-CSharp/ModuleWheelBase.cs
- L=1125 kind=PartModule type=ModuleWheelBase bases=PartModule,IModuleInfo,IContractObjectiveModule anc=PartModule/MonoBehaviour ns=-
- unity(64): GameObject=26, Rigidbody=23, Transform=13, GetComponent=11, WheelCollider=6, Collider=6 | ksp: KSPField=33, GameEvents=14, Part=10, HighLogic=7, Vessel=6 | fan_in=7
- methods(53): ATsymPartUpdate, ActAutoFrictionToggle, ActionUIUpdate, ApplyGeeBias, CheckContractObjectiveValidity, CheckSubsteps, CheckSuspensionToggle, DemoteToPhysicslessPart, DisableSuspension, DisableWheelCollider, EnableSuspension, EnableWheelCollider, EvtAutoFrictionToggle, FixedUpdate, GetContractObjectiveType, GetDrawModulePanelCallback, GetInfo, GetModuleDisplayName, GetModuleTitle, GetPrimaryField, IgnoreForcesOnSameVesselContact, InopUpdate, InopWheelCollider, InopWheelTransform ...
- auto: B / PartModules-Wheels

## Assembly-CSharp/ModuleWheels/ModuleWheelDeployment.cs
- L=898 kind=PartModule type=ModuleWheelDeployment bases=ModuleWheelSubmodule,IMultipleDragCube anc=ModuleWheelSubmodule/PartModule/MonoBehaviour ns=ModuleWheels
- unity(43): Transform=11, GameObject=10, Collider=4, Lifecycle=3, Animation=3, Instantiate/Destroy=2 | ksp: KSPField=16, Localizer=13, HighLogic=11, ProtoPartModuleSnapshot=2, ConfigNode=2 | fan_in=5
- methods(24): ActionToggle, AssumeDragCubePosition, CacheLocalStrings, CreateStandInCollider, EventToggle, FSMSetup, FindTransformAnimationTime, FixedUpdate, GetDragCubeNames, GetModuleDisplayName, GetToggleEventName, LateUpdate, OnFSMStateChange, OnGatherInfo, OnInventoryModeDisable, OnLoad, OnSave, OnStart, OnStoredInInventory, OnWheelSetup, SetInopSubsystems, ToggleDeployment, Update, UsesProceduralDragCubes
- auto: B / PartModules-Wheels

## Assembly-CSharp/Orbit.cs
- L=1854 kind=class type=Orbit bases=- anc=- ns=-
- unity(7): none | ksp: Vector3d=133, Orbit=68, ScaledSpace=28, Planetarium=25, CelestialBody=13 | fan_in=96
- methods(110): AscendingNodeTrueAnomaly, Clamp_dt, CreateRandomOrbitAround, CreateRandomOrbitFlyBy, CreateRandomOrbitNearby, DescendingNodeTrueAnomaly, DrawOrbit, DumpData, EccentricAnomalyAtObT, EccentricAnomalyAtUT, FindClosestPointsDelegate, FindClosestPoints_old, GetANVector, GetDTforTrueAnomaly, GetDTforTrueAnomalyAtUT, GetEccVector, GetEccentricAnomaly, GetFrameVel, GetFrameVelAtUT, GetMeanAnomaly, GetMeanMotion, GetNextApoapsisTime, GetNextPeriapsisTime, GetNextTimeOfRadius ...
- auto: A / Orbit

## Assembly-CSharp/OrbitRendererBase.cs
- L=994 kind=MonoBehaviour type=OrbitRendererBase bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(38): Transform=6, Shader=4, Material=3, Lifecycle=3, Camera=3, Time.=2 | ksp: Localizer=35, ScaledSpace=18, Vessel=8, Vector3d=8, MapView=6 | fan_in=11
- methods(45): ANDNNodes_OnUpdateIcon, ApNode_OnUpdateIcon, AttachNodeUIs, CacheLocalStrings, CanDrawAnyIcons, CreateScaledSpaceNodes, DestroyScaledSpaceNodes, DetachNodeUIs, DrawNodes, DrawOrbit, DrawSpline, FindMapObject, GetCurrentDrawMode, GetMouseOverNode, GetNodeColour, GetOrbitColour, GetScreenSpacePoint, GetSegmentCount, GetUpdatedOrbitPoint, HaveStateVectorKnowledge, IsRenderableOrbit, LateUpdate, MakeLine, OnDestroy ...
- auto: B / MapView

## Assembly-CSharp/OrbitTargeter.cs
- L=1081 kind=MonoBehaviour type=OrbitTargeter bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(35): Renderer=7, Lifecycle=5, GameObject=3, Input.=2, GetComponent=2, Shader=1 | ksp: Vector3d=34, Orbit=15, Localizer=13, MapView=10, Planetarium=8 | fan_in=3
- methods(38): Awake, CacheLocalStrings, CanDrawAnyLines, CanDrawAnyNode, CleanupMarker, ClearMenus, CursorInputUpdate, DestroyLine, DottedLineUpdate, DrawLine, DropInvalidTargets, FindPatch, GetETAString, GetSeparations, IsTargetCb, LateUpdate, NodeCleanup, NodeUpdate, OnDestroy, OnUpdateCaption, OnUpdatePosition, OnUpdateType, OnUpdateVisible, ReferencePatchSelect ...
- auto: C / Orbit

## Assembly-CSharp/Part.cs
- L=9023 kind=MonoBehaviour type=Part bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(55): Transform=130, GameObject=123, Rigidbody=60, GetComponent=59, LayerMask=27, Instantiate/Destroy=26 | ksp: Part=173, GameEvents=82, HighLogic=72, AttachNode=58, PartModule=44 | fan_in=389
- methods(386): AddAttachNode, AddCrewmember, AddCrewmemberAt, AddExposedThermalFlux, AddForce, AddForceAtPosition, AddImpulse, AddInternalPart, AddModule, AddOnMouseDown, AddOnMouseEnter, AddOnMouseExit, AddPartModuleAdjusterList, AddResource, AddShield, AddSkinThermalFlux, AddThermalFlux, AddTorque, AimCamera, AllowAutoStruts, AlreadyProcessedRequest, ApplyRigidAttachment, AttachedPartNode, AttributeSetup ...
- auto: B / Part

## Assembly-CSharp/PartLoader.cs
- L=2205 kind=MonoBehaviour type=PartLoader bases=LoadingSystem anc=LoadingSystem/MonoBehaviour ns=-
- unity(78): GameObject=122, Transform=61, Material=47, Renderer=25, GetComponent=23, Shader=22 | ksp: AvailablePart=53, ConfigNode=27, Part=25, GameDatabase=19, PartLoader=10 | fan_in=74
- methods(64): AddPartReplacement, AddVariants, ApplyPartValue, Awake, CheckPartDatabaseTimestamps, CheckPartDatabaseVersion, ClearAll, CompileAll, CompileInternalProps, CompileInternalSpaces, CompileModel, CompilePartInfo, CompileParts, CompileVariantThemes, ConstructSoundFXGroup, CreatePart, CreatePartDatabase, CreatePartIcon, DebugHeirarchy, DoesPartExist, DoesPartHaveReplacement, GetAvailableAndPurchaseableCargoParts, GetAvailableCargoInventoryPartNames, GetAvailableCargoInventoryParts ...
- auto: B / Part

## Assembly-CSharp/PartModule.cs
- L=1256 kind=PartModule type=PartModule bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(22): GetComponent=3, Coroutine=2, MonoBehaviour=1, Lifecycle=1 | ksp: ConfigNode=33, PartModule=13, KSPField=10, Part=6, Localizer=4 | fan_in=167
- methods(77): AddPartModuleAdjuster, AddPartModuleAdjusterList, AppliedUpgrades, ApplyAdjustersOnStart, ApplyUpgradeNode, ApplyUpgrades, Awake, ClearPersistentId, Copy, DemoteToPhysicslessPart, FindUpgrades, GetCurrentColor, GetInfo, GetModuleDisplayName, GetPersistenActiontId, GetPersistentId, GetReflectedAttributes, GetStagingDisableText, GetStagingEnableText, GetUpgrade, GetUpgradeInfo, HasUpgrades, IsStageable, IsValidContractObjective ...
- auto: A / Part

## Assembly-CSharp/PartReader.cs
- L=1104 kind=static class type=PartReader bases=- anc=- ns=-
- unity(73): Shader=50, GameObject=46, Material=41, Transform=20, Collider=20, Animation=19 | ksp: GameDatabase=1 | fan_in=1
- methods(25): AddMaterialDummy, AddTextureDummy, Contains, FindChildByName, Get, NormalMapToUnityNormalMap, Read, ReadAnimation, ReadAnimationEvents, ReadCamera, ReadChild, ReadColor, ReadLight, ReadMaterial, ReadMaterial4, ReadMaterialTexture, ReadMesh, ReadMeshRenderer, ReadParticles, ReadSkinnedMeshRenderer, ReadTagAndLayer, ReadTextures, ReadVector2, ReadVector3 ...
- auto: C / Rendering

## Assembly-CSharp/PartSet.cs
- L=1022 kind=class type=PartSet bases=- anc=- ns=-
- unity(5): Resources.=1 | ksp: PartResource=44, Part=39, Vessel=8, GameEvents=6, ShipConstruct=5 | fan_in=6
- methods(21): AddPartResource, AddPartToSet, BuildPartSets, BuildPartSimulationSets, BuildResList, ContainsPart, GetConnectedResourceTotals, GetOrCreateList, GetParts, GetResourceList, HookEvents, OnFlowModeChange, OnFlowStateChange, ProcessRequest, RebuildInPlace, RebuildParts, RebuildVessel, RemovePart, RemovePartResource, RequestResource, SetsEqual
- auto: A / Resources

## Assembly-CSharp/PhysicsGlobals.cs
- L=3875 kind=MonoBehaviour type=PhysicsGlobals bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(38): AnimationCurve=54, PhysicMaterial=12, Transform=3, Lifecycle=3, Shader=2, Resources.=1 | ksp: ConfigNode=43, FloatCurve=33, Part=9, GameSettings=6, Planetarium=5 | fan_in=55
- methods(16): Awake, CalculateValues, DragCurveValue, GetBlackBodyRadiation, GetLiftingSurfaceCurve, Load, LoadDatabase, LoadDefaultLiftingSurfaceCurves, LoadLiftingSurfaceCurves, OnDestroy, OnDrawGizmosSelected, OnFlightGlobalsReady, Save, SaveDatabase, Start, UpdateKerbalEVAPhysicsMaterial
- auto: B / Physics

## Assembly-CSharp/PQ.cs
- L=1077 kind=MonoBehaviour type=PQ bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(35): Transform=8, GameObject=6, Mesh=2, LayerMask=2, Instantiate/Destroy=2, Time.=1 | ksp: PQS=32, Vector3d=12, QuaternionD=2 | fan_in=0
- methods(30): Awake, Build, ClearAndDestroy, ClearCache, Collapse, Destroy, FastUpdateSubQuadsPosition, GetEdge, GetEdgeQuads, GetEdgeState, GetRightmostCornerPQ, GetSidePQ, PreciseUpdateSubQuadsPosition, QuadDelegate, QueueForCornerNormalUpdate, QueueForNormalUpdate, SetCache, SetEdgeState, SetInvisible, SetMasterInvisible, SetMasterVisible, SetNeighbour, SetVisible, SetupQuad ...
- auto: B / Terrain

## Assembly-CSharp/PQS.cs
- L=3168 kind=MonoBehaviour type=PQS bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(42): GameObject=24, Transform=20, Material=16, Texture=14, Coroutine=9, GetComponent=6 | ksp: Vector3d=61, PQS=12, QuaternionD=7, GameEvents=3, GameSettings=2 | fan_in=79
- methods(125): ActivateSphere, AddPQToLocalSpaceStorage, ApplyTerrainShaderSettings, AssignMeshTangents, AssignQuad, BackupEdgeNormals, BilinearInterpColorMap, BilinearInterpFloatMap, BuildMeshColorChannel, BuildMeshCustomNormals, BuildMeshSphereNormals, BuildMeshTangents, BuildMeshUV2Channel, BuildMeshUV3Channel, BuildMeshUV4Channel, BuildMeshUVChannel, BuildNormals, BuildNormalsNoClear, BuildQuad, BuildTangents, BuildVertexColor, BuildVertexHeight, BuildVertexMapCoords, BuildVertexQuadUV ...
- auto: B / Terrain

## Assembly-CSharp/PQSLandControl.cs
- L=930 kind=PQSMod type=PQSLandControl bases=PQSMod anc=PQSMod/MonoBehaviour ns=-
- unity(50): GameObject=19, Transform=10, Instantiate/Destroy=4, Mesh=3, GetComponent=3, SetActive=2 | ksp: PQS=15, GameSettings=2, GameEvents=2 | fan_in=3
- methods(26): AddScatterMeshController, ApplyCastShadowsSetting, BuildCache, BuildCacheMesh, BuildCacheQuad, Clamp, ClearAll, CreateScatterMesh, CubicHermite, DestroyQuad, DoubleLerp, Lerp, OnDestroy, OnQuadBuilt, OnQuadPreBuild, OnSetup, OnSphereReset, OnSphereStarted, OnVertexBuild, OnVertexBuildHeight, RandomRange, Reset, Setup, SphereActive ...
- auto: B / Terrain

## Assembly-CSharp/ProceduralFairings/FairingPanel.cs
- L=897 kind=class type=FairingPanel bases=- anc=- ns=ProceduralFairings
- unity(43): Transform=20, GameObject=9, Material=8, Renderer=6, Mesh=5, GetComponent=5 | ksp: HighLogic=4, Part=2, AttachNode=1 | fan_in=3
- methods(41): AddFlag, AssignMesh, BuildMesh, CleanUp, ContainsSection, Despawn, FetchExistingFlagReferences, GenerateCompoundColliders, GenerateConvexMesh, GeneratePanelColliders, GenerateSectionColliders, GetArea, GetBarycenter, GetBaseHeight, GetBounds, GetCursorProximity, GetMidPoint, GetPanelHeight, GetPanelSlope, GetSlope, LazyInitMeshArray, ReadyFlagsForJettison, RemoveFlag, ReparentFlagstoPanels ...
- auto: B / PartModules-Structural

## Assembly-CSharp/ProtoCrewMember.cs
- L=1375 kind=class type=ProtoCrewMember bases=- anc=- ns=-
- unity(10): GameObject=9, GetComponent=5, Instantiate/Destroy=2, Resources.=1 | ksp: ProtoCrewMember=18, GameEvents=18, PhysicsGlobals=16, HighLogic=15, ConfigNode=13 | fan_in=110
- methods(45): ActiveFixedUpdate, ActiveUpdate, ArchiveFlightLog, CalculateExperiencePoints, ChangeName, CheckActive, CheckRespawnTimer, Die, Equals, GetEffect, GetHashCode, GetKerbalEVAPartName, GetKerbalIVAName, GetKerbalIconSuitPath, GetKerbalIconSuitSuffix, GetLocalizedStatus, GetLocalizedTrait, GetLocalizedType, HasEffect, InventoryCosts, InventoryMass, OnGameStateCreated, OnPartPack, OnPartUnpack ...
- auto: B / Kerbals

## Assembly-CSharp/ProtoPartSnapshot.cs
- L=1312 kind=class type=ProtoPartSnapshot bases=- anc=- ns=-
- unity(37): Transform=28, GetComponent=19, GameObject=15, Animation=14, Instantiate/Destroy=5, SetActive=2 | ksp: ProtoCrewMember=33, ConfigNode=23, PartLoader=18, Part=15, ProtoPartSnapshot=13 | fan_in=46
- methods(17): AddProtoPartModuleAdjusters, ConfigurePart, CreatePart, FindAttachNode, FindAttachNodeSnapShot, FindModule, GetCrew, GetCrewIndex, HasCrew, Init, Load, LoadEVAInfoBySuitGender, ProtoPartRepair, RemoveCrew, RemoveProtoPartModuleAdjusters, Save, storePartRefs
- auto: B / Save

## Assembly-CSharp/ProtoVessel.cs
- L=1262 kind=class type=ProtoVessel bases=- anc=- ns=-
- unity(17): GameObject=12, Transform=7, GetComponent=6, Resources.=2, Instantiate/Destroy=2 | ksp: ConfigNode=58, Vessel=35, Localizer=28, ProtoPartSnapshot=18, GameEvents=11 | fan_in=72
- methods(23): AddCrew, CacheLocalStrings, Clean, CreateDiscoveryNode, CreateOrbitNode, CreatePartNode, CreateVesselNode, CurrentFunds, CurrentReputation, CurrentScience, GetAllProtoPartsFromCrew, GetAllProtoPartsIncludingCargo, GetDisplayName, GetSituationString, GetVesselCrew, Load, LoadObjects, RebuildCrewCounts, RemoveCrew, ResetProtoPartSnapShots, Save, SaveVesselModules, verifyCrew
- auto: B / Save

## Assembly-CSharp/PSystemSetup.cs
- L=1176 kind=MonoBehaviour type=PSystemSetup bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(66): GameObject=47, Transform=35, GetComponent=19, Coroutine=18, Instantiate/Destroy=8, SetActive=7 | ksp: PQS=27, Planetarium=19, FlightCamera=18, FlightGlobals=9, CelestialBody=9 | fan_in=40
- methods(47): AddLaunchSite, Awake, FindPQS, ForceInitPQS, GetFacilityDamage, GetFacilityLevel, GetLaunchSite, GetLaunchSiteBody, GetLaunchSiteBodyName, GetLaunchSiteDisplayName, GetSpaceCenterFacility, GetSpawnPoint, GetSpawnPointLatLonAlt, GetSpawnPointTransform, IsFacility, IsFacilityOrLaunchSite, IsLaunchFacility, IsLaunchSite, IsStockLaunchSite, LoadTestScene, OnDestroy, OnLevelLoaded, OnPSystemReady, OnSceneChange ...
- auto: B / Celestials

## Assembly-CSharp/ResearchAndDevelopment.cs
- L=1210 kind=ScenarioModule type=ResearchAndDevelopment bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(21): Lifecycle=2, GameObject=2, GetComponent=1 | ksp: ScienceSubject=38, Localizer=26, AvailablePart=26, PartLoader=15, HighLogic=14 | fan_in=83
- methods(56): AddExperimentalPart, AddScience, CanAfford, CheatAddScience, CheatTechnology, CheckForMissingParts, CountUniversalScience, GetBiomeTags, GetBiomeTagsLocalized, GetExperiment, GetExperimentIDs, GetExperimentSubject, GetMiniBiomeTags, GetMiniBiomeTagsLocalized, GetMiniBiomedisplayNameByScienceID, GetMiniBiomedisplayNameByUnityTag, GetNextScienceValue, GetReferenceDataValue, GetResults, GetScienceValue, GetSituationTags, GetSituationTagsDescriptions, GetSubjectByID, GetSubjectValue ...
- auto: A / Science

## Assembly-CSharp/ShipConstruct.cs
- L=971 kind=class type=ShipConstruct bases=IEnumerable<Part>,IEnumerable,IShipconstruct anc=IEnumerable ns=-
- unity(17): Transform=11, Resources.=3, SetActive=1, Instantiate/Destroy=1, GameObject=1 | ksp: Part=36, Vessel=25, Localizer=17, ConfigNode=16, FlightGlobals=10 | fan_in=84
- methods(18): Add, AddToConstruct, AreAllPartsConnected, Clear, Contains, GetConnectedResourceTotals, GetEnumerator, GetShipCosts, GetShipMass, GetTotalMass, LoadShip, Remove, RequestResource, RunVesselNamingUpdates, SaveShip, UpdateResourceSets, UpdateVesselNaming, isControllable
- auto: B / Save

## Assembly-CSharp/ShipConstruction.cs
- L=1349 kind=class type=ShipConstruction bases=- anc=- ns=-
- unity(48): GameObject=26, Transform=19, Texture=19, GetComponent=9, Coroutine=6, LayerMask=5 | ksp: ShipConstruct=34, Part=29, HighLogic=22, ConfigNode=21, PartLoader=19 | fan_in=65
- methods(53): AllPartsFound, AssembleForLaunch, AutoGenerateThumbnail, CalculateCraftSize, CaptureStockThumbnail, CaptureThumbnail, CheckCraftFileType, CheckLaunchSiteClear, ClearBackups, CreateBackup, CreateConstructFromTemplate, CreateSubassemblyIcon, DebugBackup, FindCraftCenter, FindCraftMOI, FindPartWithCraftID, FindVesselsAtLaunchSite, FindVesselsLandedAt, GetCurrentGameShipsPathFor, GetPartCosts, GetPartCostsAndMass, GetPlayerCraftThumbnailName, GetRootCraftSavePath, GetSABounds ...
- auto: B / Save

## Assembly-CSharp/SpaceCenterBuilding.cs
- L=911 kind=MonoBehaviour type=SpaceCenterBuilding bases=MonoBehaviour,ITooltipController anc=MonoBehaviour ns=-
- unity(54): GetComponent=13, Transform=9, GameObject=7, Coroutine=7, Renderer=6, Material=6 | ksp: Localizer=21, HighLogic=15, InputLockManager=14, GameEvents=14, ProtoVessel=11 | fan_in=14
- methods(55): ColliderHover, DemolishFacility, Despawntooltip, DowngradeFacility, EnterBuilding, FindVesselsAtFacility, FindVesselsAtGrounds, GetCollapseReputationHit, GetRepairsCost, GetStructureDamage, GetStructureDamageLevel, HighLightBuilding, IsFacilityOperational, IsOpen, LateUpdate, ObjectHasTerrainMaterials, OnClicked, OnContextMenuDismissed, OnContextMenuSpawn, OnDestroy, OnFacilityUpgraded, OnFacilityUpgrading, OnGameSceneChange, OnLeftClick ...
- auto: B / Scene

## Assembly-CSharp/SpaceObjectCollider.cs
- L=931 kind=MonoBehaviour type=SpaceObjectCollider bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(48): Transform=7, Mesh=6, GameObject=6, Coroutine=5, Time.=2, Collider=2 | ksp: none | fan_in=2
- methods(37): AddChunk, BuildCompositeMesh, Clear, ClearNeighbours, ContainsChunk, CreateChunks, CreateCollider, DrawSolid, GenCoroutine, Generate, GetNeighbour, GetSlope, GetSrfCenter, GetSrfRadial, GetTri, GetTriIndex, LateUpdate, LinkChunks, MarchEdge, MarchNext, RandomColor, RemoveChunk, SetNeighbour, SetTri ...
- auto: B / Celestials

## Assembly-CSharp/SteamManager.cs
- L=804 kind=MonoBehaviour type=SteamManager bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(68): Steam=52, TMPro=4, Lifecycle=4, Texture=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: Localizer=5 | fan_in=15
- methods(43): AddAppDependency, Awake, CheckCloudQuota, CreateNewItem, GetAppDependencies, GetItemState, GetNumberSubscribedItems, GetSubscribedItems, GetUGCFailureReason, GetUGCItem, GetUGCItemDetails, OnDestroy, OnEnable, OnGetAppDependencies, OnItemAppDependenciesUpdated, OnItemDeleted, OnItemUpdated, OnNewItemCreated, OnPersonaStateChanged, OnRemoteFileSubscribed, OnRemoteFileUnsubscribed, OnRemoveAppDependency, OnSubscribeFile, OnUGCQuery ...
- auto: C / Steam

## Assembly-CSharp/Targeting.cs
- L=1112 kind=class type=Targeting bases=- anc=- ns=-
- unity(3): Raycast/Query=2 | ksp: Vector3d=100, Orbit=1 | fan_in=1
- methods(38): Borrow, ClosestPoint, Create, Curvature, FindRoot, GetFrame, GetInfo, GetTgt, Intercepts, Intersept, Link, Normal, Point, Project, Radius, Release, ReleaseAllBorrowed, ReleaseContent, Reset, Set, Subdivide, Tangent, ToString, TrueAnomaly ...
- auto: B / Orbit

## Assembly-CSharp/TimeWarp.cs
- L=865 kind=MonoBehaviour type=TimeWarp bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(36): Time.=12, Coroutine=7, Lifecycle=5, Transform=3, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: Localizer=19, Planetarium=18, GameSettings=18, FlightGlobals=18, HighLogic=14 | fan_in=85
- methods(26): Awake, CacheLocalStrings, CancelAutoWarp, ClampRateToOrbitTransitions, FixedUpdate, GetAltitudeLimit, GetMaxRateForAltitude, OnDestroy, OnPWarpWarningDismiss, PostScreenMessage, SetRate, ShowPWarpWarning, Start, Update, WarpTo, assumeWarpRate, autoWarpTo, btnSetHighRate, btnSetLowRate, getMaxOnRailsRateIdx, getMaxWarpRateForTravel, getRateChangeTravel, getWarpAccel, setMode ...
- auto: B / Orbit

## Assembly-CSharp/TransferMath.cs
- L=1870 kind=class type=TransferMath bases=- anc=- ns=-
- unity(5): none | ksp: Vector3d=145, Orbit=65, CelestialBody=11, Planetarium=6, QuaternionD=1 | fan_in=2
- methods(40): AlignmentTime, Bisect, BisectFunc, CalcCircularizeDV, CalcEjectionValues, CalcSameSOITransferDV, CalcTransferTime, CalcdV, CalcdVReturn, CalculateCorrection, CalculateCorrectionBurn, CalculateStartEndXferTimes, ChangingPeriapsis, D8RT, DVApPhaseHohmann, DVTimeHohmann, DVTimeInterCBXfer, DVToChangeApoapsis, DVToChangePeriapsis, DeltaVToInterceptAtTime, EjectionAngleCalc, FinalizeLambert, FindIntercept, HohmannTimeOfFlight ...
- auto: A / Orbit

## Assembly-CSharp/TransferTypeSimple.cs
- L=1320 kind=class type=TransferTypeSimple bases=TransferTypeBase anc=TransferTypeBase ns=-
- unity(28): SetActive=28, GameObject=25, GetComponent=5, Coroutine=2, Application.=1 | ksp: FlightGlobals=29, Localizer=22, Planetarium=12, CelestialBody=9, Vector3d=8 | fan_in=0
- methods(28): CalculationComplete, ChangePosition, ChangeTime, CompleteDrawControls, CreateManeuver, DidWeIntercept, DisplayName, DoMathCalcs, DrawTypeControls, FinalizeTransferDV, HideApp, InclinationTooBig, OnTargetClicked, OnUpdate, OnVisualizer, RefreshUIControls, ScalePlanetToAltitude, SetSourceBody, SetupCBNames, SetupVisualPrefabs, UpdateTopData, UpdateTransferData, UpdateVisualPanel, ValidCelestialBodies ...
- auto: B / Orbit

## Assembly-CSharp/TutorialEditorAdvanced.cs
- L=3020 kind=ScenarioModule type=TutorialEditorAdvanced bases=TutorialScenario anc=TutorialScenario/ScenarioModule/MonoBehaviour ns=-
- unity(17): Transform=2, SetActive=2, GameObject=2, Texture=1, Application.=1 | ksp: Part=210, Localizer=106, EditorLogic=59, InputLockManager=26, GameSettings=16 | fan_in=0
- methods(30): AttachedNum, CheckAbort, CheckChute, CheckChuteStats, CheckDecouplerPod, CheckElectric, CheckEngineUpper, CheckFins, CheckLowerDec, CheckLowerEngine, CheckLowerEngineStats, CheckLowerTanks, CheckNosecones, CheckPod, CheckRCSTank, CheckRCSTankLoading, CheckRCSThrusters, CheckRadialDecs, CheckSolids, CheckSolidsStats, CheckT45Staging, CheckTanks, CheckTanksDry, CheckTanksWet ...
- auto: B / Tutorials

## Assembly-CSharp/UIPartActionController.cs
- L=879 kind=MonoBehaviour type=UIPartActionController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(55): GameObject=21, SetActive=11, Instantiate/Destroy=7, Transform=4, Lifecycle=4, Camera=4 | ksp: UIPartActionWindow=39, Part=15, EditorLogic=12, GameEvents=9, HighLogic=6 | fan_in=32
- methods(35): Activate, Awake, CreatePartUI, Deactivate, Deselect, GetControl, GetFieldControl, GetItem, GetSelectionModifier, HandleMouseClick, HiddenItemListGet, HiddenResourceItemListGet, InventoryAndCargoPartExist, ItemListContains, ItemListContainsCounterparts, ItemListGet, MouseClickCoroutine, OnCameraChange, OnDestroy, OnHideResource, OnShowResource, PartWindowIsFromAnInventory, SelectPart, SetupItemControls ...
- auto: B / FlightUI

## Assembly-CSharp/UIPartActionWindow.cs
- L=2006 kind=MonoBehaviour type=UIPartActionWindow bases=MonoBehaviour,IDragHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(62): Transform=70, GameObject=37, Instantiate/Destroy=27, SetActive=22, UnityEngine.UI=14, EventSystems=12 | ksp: Part=46, GameEvents=23, PartResource=15, PartModule=14, GameSettings=14 | fan_in=46
- methods(85): AddAeroControl, AddCrewInventory, AddEventControl, AddFieldControl, AddFuelFlowOverlay, AddGroup, AddItem, AddParameterGroup, AddResourceEditorControl, AddResourceFlightControl, AddResourcePriorityControl, AddResourceTransferControl, AddRoboticJointControl, AddThermalControl, Awake, CanActivateAeroDisplay, CanActivateEvent, CanActivateField, CanActivateFuelFlowOverlay, CanActivateResource, CanActivateResourcePriorityDisplay, CanActivateRoboticJointDisplay, CanActivateThermalDisplay, ClearList ...
- auto: C / FlightUI

## Assembly-CSharp/UrlDir.cs
- L=927 kind=class type=UrlDir bases=- anc=- ns=-
- unity(20): Coroutine=10, Application.=3, Texture=1, AssetBundle=1 | ksp: ConfigNode=9 | fan_in=17
- methods(22): AddConfig, ConfigExists, ConfigureFile, ConstructURL, ContainsConfig, Create, CreateApplicationPath, CreateDirectory, CreateNodeList, DirectoryExists, Exists, FileExists, GetConfig, GetConfigs, GetDirectory, GetEnumerator, GetFile, GetFiles, PathCombine, SaveConfigs, StripExtension, UrlSplit
- auto: A / Modding

## Assembly-CSharp/Vessel.cs
- L=4664 kind=MonoBehaviour type=Vessel bases=MonoBehaviour,IShipconstruct,ITargetable anc=MonoBehaviour ns=-
- unity(46): GameObject=48, GetComponent=37, Instantiate/Destroy=28, Transform=27, LayerMask=12, Time.=11 | ksp: Part=64, FlightGlobals=56, Vector3d=49, GameEvents=46, Localizer=45 | fan_in=393
- methods(179): ActionControlBlocked, AddOrbitRenderer, AddPhysicsHoldLock, AddRBAnchor, AttachPatchedConicsSolver, AutoRename, Awake, AxisControlBlocked, BackupVessel, BlockedMask, BuildCrossfeedPartSets, CacheLocalStrings, CacheScreenMessageObjects, ChangeWorldVelocity, CheckAirstreamShields, CheckControllable, CheckGroundCollision, CheckKill, Clean, ClearActiveInternalSpace, ClearStaging, ContainsCollider, CopyOverrides, CountSerenityControllerFields ...
- auto: B / Vessel

## Assembly-CSharp/VesselAutopilot.cs
- L=1274 kind=class type=VesselAutopilot bases=- anc=- ns=-
- unity(14): Transform=3, Rigidbody=3, Lifecycle=3, Instantiate/Destroy=3, Time.=2 | ksp: GameSettings=24, Vessel=15, FlightGlobals=11, TimeWarp=10, Vector3d=6 | fan_in=4
- methods(54): AngularTrim, AutoTuneReset, AutoTuneScalar, CanEngageSAS, CanSetMode, CheckCoasting, CheckDamping, CheckPitchInput, CheckRollInput, CheckStoredTransform, CheckYawInput, ConnectFlyByWire, ControlUpdate, Destroy, Disable, DisconnectFlyByWire, Enable, EvaluateScalar, GetRotationDelta, GetTotalVesselTorque, KillAngularVelocity, LockRotation, ManeuverLockInvalid, ModuleSetup ...
- auto: B / PartModules-Control

## Assembly-CSharp/VesselDeltaV.cs
- L=1270 kind=MonoBehaviour type=VesselDeltaV bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(33): Time.=11, Coroutine=8, Lifecycle=4, GetComponent=4, GameObject=2, MonoBehaviour=1 | ksp: GameEvents=54, HighLogic=24, Vessel=22, FlightGlobals=18, GameSettings=13 | fan_in=13
- methods(53): CalculateSeparationStageIndexes, Callback_CrewChange, CheckDirtyAndRun, Create, DisableStockSimluation, EnableStockSimluation, FixedUpdate, GetActivatedEngines, GetHighestSeparationStage, GetSituationTotalDeltaV, GetStage, GetWorkingStage, OnDestroy, OnDockingComplete, OnEngineActiveChange, OnEngineThrustPercentageChanged, OnGUIStageSequenceModified, OnMultiModeEngineSwitchActive, OnNodeChangedVesselResources, OnPartModuleAdjusterAdded, OnPartModuleAdjusterRemoved, OnStageActivate, OnStagingSeparationIndices, OnVesselChange ...
- auto: B / Vessel

## Assembly-CSharp/XKCDColors.cs
- L=3871 kind=static class type=XKCDColors bases=- anc=- ns=-
- unity(33): none | ksp: none | fan_in=81
- methods(4): A, FromHtml, ToHex, ToHexA
- auto: C / UI
