# Review chunk 03 (root): 279 files, 70394 lines

## Assembly-CSharp/KerbalInventoryScenario.cs
- 133 lines; kind=ScenarioModule; type=KerbalInventoryScenario; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=14, Transform=7, Instantiate/Destroy=3, Lifecycle=2; unity_score=77
- ksp refs: GameEvents=2, ScenarioModule=1, ProtoCrewMember=1, HighLogic=1, ConfigNode=1; fan_in=3 fan_out=9
- methods(8): AddKerbalInventoryInstance, ContainsCrew, OnAwake, OnDestroy, OnKerbalNameChanged, OnSave, RemoveKerbalInventoryInstance, Update
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/KerbalMixingTransforms.cs
- 10 lines; kind=struct; type=KerbalMixingTransforms; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Transform=2; unity_score=40
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/KerbalMovementParameters.cs
- 17 lines; kind=class; type=KerbalMovementParameters; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=12
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/KerbalPreview.cs
- 82 lines; kind=MonoBehaviour; type=KerbalPreview; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: SetActive=8, Material=6, GameObject=5, Renderer=3, MonoBehaviour=1, Lifecycle=1; unity_score=90
- ksp refs: ProtoCrewMember=1; fan_in=2 fan_out=1
- methods(2): Awake, PreviewHelmetSelection
- auto suggestion: tier=C subsystem=Kerbals

## Assembly-CSharp/KerbalProp.cs
- 24 lines; kind=class; type=KerbalProp; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: GameObject=4, Transform=2, Renderer=1, Animation=1; unity_score=72
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/KerbalRagdollNode.cs
- 23 lines; kind=class; type=KerbalRagdollNode; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Transform=1, Rigidbody=1, GameObject=1, Collider=1; unity_score=62
- ksp refs: Vector3d=1; fan_in=1 fan_out=1
- methods(1): updateVelocity
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/KerbalRoster.cs **MUST-OPEN (>800 lines)**
- 1397 lines; kind=class; type=KerbalRoster; bases=-; ancestry=-; ns=(global)
- usings: Expansions, Expansions.Missions, Expansions.Missions.Runtime, Experience, FinePrint.Utilities, KSP.Localization, KSP.UI, UnityEngine
- unity: Coroutine=8, GetComponent=1; unity_score=9
- ksp refs: ProtoCrewMember=131, HighLogic=17, Localizer=15, Game=12, GameDatabase=9, KerbalRoster=6; fan_in=33 fan_out=39
- methods(54): AddApplicant, AddCrewMember, AddExperienceType, CalculateExperience, CalculateExperienceLevel, ChangeName, ChangeNameCalledFromPCM, CheatExperience, CheckRosterRespawn, DefaultCrewForVessel, Exists, ExperienceAddFlight, ExperienceBodyValue, ExperienceHomeTypeValue, ExperienceNotHomeTypeValue, GenerateExperienceEffectTypes, GenerateExperienceLog, GenerateExperienceTraitTypes, GenerateExperienceTypes, GenerateInitialCrewRoster, GetActiveCrewCount, GetAssignedCrewCount, GetAvailableCrewCount, GetExperienceEffectType, GetExperienceLevelRequirement, GetExperienceMaxLevel, GetExperienceTraitType, GetKIACrewCount, GetLocalizedExperienceTraitName, GetLostCrewCount, GetMissingCrewCount, GetNewKerbal, GetNextApplicant, GetNextAvailableKerbal, GetNextOrNewKerbal, HireApplicant, IndexOf, Init, InitializeExperienceTypes, Kerbals, PCMUpdate, Remove, RemoveDead, RemoveMIA, RepopulateCrewRoster ...
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/KerbalSeat.cs
- 313 lines; kind=PartModule; type=KerbalSeat; bases=PartModule,ICommNetControlSource,IConstruction; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: CommNet, Experience.Effects, KSP.Localization, UnityEngine
- unity: GameObject=12, Transform=9, GetComponent=3, Coroutine=2, SetActive=1, Collider=1; unity_score=61
- ksp refs: KSPField=7, KerbalEVA=5, Part=4, HighLogic=4, Localizer=2, KSPEvent=2; fan_in=8 fan_out=27
- methods(20): BoardSeat, CacheLocalStrings, CanBeDetached, CanBeOffset, CanBeRotated, EditorSeatBoundsAreClear, GetControlSourceState, IsCommCapable, LeaveSeat, MakeReferenceToggle, MakeReferenceTransform, OnChildRemove, OnInventoryModeEnable, OnLoad, OnSave, OnStart, OnStartFinished, SeatBoundsAreClear, SpawnBoardingKerbal, UpdateNetwork
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/KerbalVisorGlossiness.cs
- 12 lines; kind=MonoBehaviour; type=KerbalVisorGlossiness; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Material=2, Shader=1, Renderer=1, MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=88
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): Start
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/KeyBinding.cs
- 227 lines; kind=class; type=KeyBinding; bases=InputBinding,IConfigNode,ICloneable; ancestry=InputBinding/IConfigNode; ns=(global)
- usings: UnityEngine
- unity: KeyCode=12, Time.=8; unity_score=44
- ksp refs: ConfigNode=2; fan_in=6 fan_out=7
- methods(8): GetDoubleTapDown, GetDoubleTapUp, GetKey, GetKeyDown, GetKeyUp, IsNeutral, Load, Save
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/KeyboardLayout.cs
- 47 lines; kind=class; type=KeyboardLayout; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=8
- ksp refs: none; fan_in=2 fan_out=1
- methods(2): GetKeyboardLayout, GetKeyboardLayoutType
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/KeyboardLayoutType.cs
- 9 lines; kind=enum; type=KeyboardLayoutType; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/KeyCodeExtended.cs
- 62 lines; kind=class; type=KeyCodeExtended; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: KeyCode=8; unity_score=61
- ksp refs: none; fan_in=4 fan_out=0
- methods(1): ToString
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/KFSMCallback.cs
- 1 lines; kind=delegate; type=KFSMCallback; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=16 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/KFSMEvent.cs
- 24 lines; kind=class; type=KFSMEvent; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=23 fan_out=4
- methods(1): IsValid
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/KFSMEventCondition.cs
- 1 lines; kind=delegate; type=KFSMEventCondition; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/KFSMState.cs
- 57 lines; kind=class; type=KFSMState; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=45 fan_out=4
- methods(3): AddEvent, IsValid, ToString
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/KFSMStateChange.cs
- 1 lines; kind=delegate; type=KFSMStateChange; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=15 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/KFSMTimedEvent.cs
- 18 lines; kind=class; type=KFSMTimedEvent; bases=KFSMEvent; ancestry=KFSMEvent; ns=(global)
- usings: UnityEngine
- unity: Time.=1; unity_score=12
- ksp refs: none; fan_in=4 fan_out=2
- methods(1): WaitForTimer
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/KFSMUpdateMode.cs
- 7 lines; kind=enum; type=KFSMUpdateMode; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=18 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/KnowledgeItem.cs
- 85 lines; kind=class; type=KnowledgeItem; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=2
- methods(2): GetValue, UpdateDataCallback
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/Krakensbane.cs
- 209 lines; kind=MonoBehaviour; type=Krakensbane; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Rigidbody=5, Lifecycle=4, Time.=2, Collision/Force=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=57
- ksp refs: FlightGlobals=22, Vector3d=11, Vessel=6, Krakensbane=2, GameEvents=2, FloatingOrigin=2; fan_in=17 fan_out=5
- methods(12): AddExcess, AddFrameVelocity, Awake, FixedUpdate, GetFrameVelocity, GetFrameVelocityV3f, GetLastCorrection, OnDestroy, ResetVelocityFrame, SafeToEngage, Start, Zero
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/KSCPauseMenu.cs
- 462 lines; kind=MonoBehaviour; type=KSCPauseMenu; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, Expansions.Missions.Runtime, KSP.Localization, KSP.UI.Screens.DebugToolbar, SaveUpgradePipeline, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=24, SetActive=15, KeyCode=4, Input.=4, Lifecycle=3, TMPro=2; unity_score=68
- ksp refs: Localizer=37, HighLogic=34, MissionSystem=7, Game=5, ConfigNode=5, InputLockManager=3; fan_in=1 fan_out=44
- methods(24): Awake, BuildButtonList, CheckFilename, ConfirmDialog, Create, Dismiss, InitiateLoad, InitiateSave, LoadGame, MissionModeValid, OnDestroy, OnSaveDialogDestroy, SaveGame, Update, doSave, draw, drawMissionRevertOptions, onLoadDialogDismiss, onMiniSettingsFinished, onPipelineFailed, onPipelineFinished, onSaveDialogDismiss, quickLoad, spawnSaveDialog
- auto suggestion: tier=C subsystem=Scene

## Assembly-CSharp/KSCSiteNode.cs
- 32 lines; kind=class; type=KSCSiteNode; bases=ISiteNode; ancestry=ISiteNode; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens.Mapview, UnityEngine
- unity: Transform=1; unity_score=20
- ksp refs: Localizer=1; fan_in=3 fan_out=5
- methods(4): GetDisplayName, GetName, GetWorldPos, UpdateNodeCaption
- auto suggestion: tier=A subsystem=Scene

## Assembly-CSharp/KSCUpgradeableLevelText.cs
- 15 lines; kind=ScriptableObject; type=KSCUpgradeableLevelText; bases=ScriptableObject; ancestry=ScriptableObject; ns=(global)
- usings: UnityEngine
- unity: ScriptableObject=1; unity_score=29
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAction.cs
- 69 lines; kind=attribute; type=KSPAction; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: KSPAction=6; fan_in=52 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/KSPActionGroup.cs
- 44 lines; kind=enum; type=KSPActionGroup; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: Light=1; unity_score=21
- ksp refs: none; fan_in=42 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/KSPActionParam.cs
- 29 lines; kind=class; type=KSPActionParam; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=3
- ksp refs: none; fan_in=53 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/KSPActionType.cs
- 6 lines; kind=enum; type=KSPActionType; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: UnityEngine.UI=1; unity_score=29
- ksp refs: none; fan_in=28 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/KSPAddon.cs
- 35 lines; kind=attribute; type=KSPAddon; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/KSPAssembly.cs
- 29 lines; kind=attribute; type=KSPAssembly; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/KSPAssemblyDependency.cs
- 29 lines; kind=attribute; type=KSPAssemblyDependency; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/KSPAssemblyDependencyEqualMajor.cs
- 29 lines; kind=attribute; type=KSPAssemblyDependencyEqualMajor; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/KSPAxisField.cs
- 30 lines; kind=attribute; type=KSPAxisField; bases=KSPField; ancestry=KSPField/FieldAttribute/Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: KSPField=1; fan_in=19 fan_out=3
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/KSPAxisGroup.cs
- 36 lines; kind=enum; type=KSPAxisGroup; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=14 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/KSPAxisMode.cs
- 5 lines; kind=enum; type=KSPAxisMode; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=15 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/KSPCameraUtil.cs
- 38 lines; kind=class; type=KSPCameraUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=28
- ksp refs: GameSettings=10; fan_in=7 fan_out=2
- methods(3): AnyCameraInputDown, GetCamCoordsFromPosition, GetDistanceToFit
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/KSPCapability.cs
- 14 lines; kind=attribute; type=KSPCapability; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/KSPCapabilityValue.cs
- 18 lines; kind=attribute; type=KSPCapabilityValue; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/KSPCompression.cs
- 135 lines; kind=MonoBehaviour; type=KSPCompression; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Ionic.Zip, UnityEngine
- unity: MonoBehaviour=1; unity_score=26
- ksp refs: none; fan_in=2 fan_out=0
- methods(4): CompressDirectory, DecompressFile, FilesAtRoot, GetTopLevelDirectories
- auto suggestion: tier=B subsystem=Save

## Assembly-CSharp/KSPEvent.cs
- 79 lines; kind=attribute; type=KSPEvent; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=5
- ksp refs: KSPEvent=2, Localizer=1; fan_in=74 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/KSPField.cs
- 56 lines; kind=attribute; type=KSPField; bases=FieldAttribute; ancestry=FieldAttribute/Attribute; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=7
- ksp refs: KSPField=2, Localizer=1; fan_in=167 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/KSPFontAsset.cs
- 157 lines; kind=class; type=KSPFontAsset; bases=-; ancestry=-; ns=(global)
- usings: TMPro, UnityEngine
- unity: TMPro=8, Material=8, Shader=4, Texture=2, ScriptableObject=1; unity_score=68
- ksp refs: none; fan_in=2 fan_out=7
- methods(3): GetAtlasFromHash, GetFontAsset, GetMaterialFromHash
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/KSPFontAssetList.cs
- 7 lines; kind=class; type=KSPFontAssetList; bases=List<KSPFontAsset>; ancestry=List; ns=(global)
- usings: -
- unity: none; unity_score=12
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/KSPFontTypeConverter.cs
- 17 lines; kind=class; type=KSPFontTypeConverter; bases=SerializationBinder; ancestry=SerializationBinder; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(1): BindToType
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/KSPKerningPairList.cs
- 8 lines; kind=class; type=KSPKerningPairList; bases=List<KerningPair>; ancestry=List; ns=(global)
- usings: TMPro
- unity: none; unity_score=12
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/KSPLog.cs
- 307 lines; kind=MonoBehaviour; type=KSPLog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: QualitySettings/SystemInfo=9, Application.=6, Texture=3, Lifecycle=3, Transform=2, Instantiate/Destroy=2; unity_score=55
- ksp refs: GameSettings=5; fan_in=3 fan_out=6
- methods(15): AddLogCallback, Awake, ConvertLineEndings, LogCallback, LogHeader, LogRenderTextureFormats, LogToFile, LogToMemory, LogToScreen, OnDestroy, OnMemoryLogUpdated, RemoveLogCallback, SetScreenLogging, Start, TimeString
- auto suggestion: tier=B subsystem=Core

## Assembly-CSharp/KSPModelEvent.cs
- 18 lines; kind=attribute; type=KSPModelEvent; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/KSPModule.cs
- 12 lines; kind=attribute; type=KSPModule; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/KSPParseable.cs
- 181 lines; kind=struct; type=KSPParseable; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: none; fan_in=5 fan_out=1
- methods(1): Save
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/KSPPart.cs
- 12 lines; kind=attribute; type=KSPPart; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/KSPParticleEmitter.cs
- 418 lines; kind=MonoBehaviour; type=KSPParticleEmitter; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.Rendering
- unity: ParticleSystem=29, GetComponent=4, Transform=3, Time.=2, Material=2, Lifecycle=2; unity_score=73
- ksp refs: none; fan_in=7 fan_out=0
- methods(8): Awake, Emit, EmitParticle, EnsureParticleSystemCreated, Reset, SetDirty, SetupProperties, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/KSPRandom.cs
- 134 lines; kind=class; type=KSPRandom; bases=System.Random; ancestry=Random; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=3
- ksp refs: none; fan_in=54 fan_out=0
- methods(4): Next, NextBytes, NextDouble, Sample
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/KSPRichTextUtil.cs
- 17 lines; kind=class; type=KSPRichTextUtil; bases=RichTextUtil; ancestry=RichTextUtil; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: GameSettings=3, Planetarium=2; fan_in=2 fan_out=4
- methods(3): TextDate, TextDateCompact, TextDurationCompact
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSPScenario.cs
- 42 lines; kind=attribute; type=KSPScenario; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=24 fan_out=2
- methods(2): HasCreateOption, HasTargetScene
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/KSPScenarioType.cs
- 55 lines; kind=class; type=KSPScenarioType; bases=-; ancestry=-; ns=(global)
- usings: Expansions, UnityEngine
- unity: none; unity_score=13
- ksp refs: ScenarioModule=2; fan_in=3 fan_out=4
- methods(1): GetAllScenarioTypesInAssemblies
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/KSPSteamUtils.cs
- 102 lines; kind=class; type=KSPSteamUtils; bases=-; ancestry=-; ns=(global)
- usings: KSP.UI.Screens, Steamworks, UnityEngine
- unity: Steam=6, GameObject=1; unity_score=45
- ksp refs: none; fan_in=10 fan_out=4
- methods(4): GatherCraftFiles, GatherCraftFilesFileInfo, GetSteamCacheLocation, GetSteamIDFromSteamFolder
- auto suggestion: tier=C subsystem=Steam

## Assembly-CSharp/KSPUpgradePipeline.cs
- 97 lines; kind=static class; type=KSPUpgradePipeline; bases=-; ancestry=-; ns=(global)
- usings: SaveUpgradePipeline, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2; unity_score=25
- ksp refs: ConfigNode=6, InputLockManager=2, HighLogic=1; fan_in=6 fan_out=16
- methods(3): Init, OnFinish, Process
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSPUtil.cs **MUST-OPEN (>800 lines)**
- 2176 lines; kind=static class; type=KSPUtil; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization, UniLinq, UnityEngine
- unity: Transform=29, GameObject=5, Application.=5, MonoBehaviour=3, Texture=2, Screen/Cursor=2; unity_score=22
- ksp refs: Localizer=43, Vector3d=12, QuaternionD=10, Part=4, GameSettings=3, HighLogic=1; fan_in=212 fan_out=14
- methods(92): Add, AddUnits, AppendTimeToStringBuilder, AppendValueToString, BearingDegrees, BearingRadians, CacheLocalStrings, CalculateFolderSize, CheckDOSName, CheckVersion, ClampRectToScreen, DateTimeNow, DeepCompare, Diag, FindComponentsImplementing, FindInPartModel, FindTagsInChildren, FindTransformAtIndexPath, GenerateFilePathWithDate, GenerateSuperSeed, GenerateTexture2DArray, GetActivator, GetAttachNodeInfo, GetDateFromUT, GetEarthDateFromUT, GetKerbinDateFromUT, GetLinkID, GetOrCreatePath, GetPartByCraftID, GetPartInfo, GetPartName, GetRelativePath, GetTimeSpanDay, GetTimeSpanHour, GetTimeSpanMinute, GetTimeSpanSecond, GetTimeSpanYear, GetTransformIndexPathToRoot, GetTransformPathToRoot, GetUTDay, GetUTHour, GetUTMinute, GetUTSecond, GetUTYear, HasAncestorTransform ...
- auto suggestion: tier=B subsystem=Core

## Assembly-CSharp/KSPWheelController.cs
- 274 lines; kind=MonoBehaviour; type=KSPWheelController; bases=VehicleBase; ancestry=VehicleBase/MonoBehaviour; ns=(global)
- usings: UnityEngine, VehiclePhysics
- unity: Rigidbody=6, KeyCode=5, Input.=5, GetComponent=4, GameObject=4, Time.=3; unity_score=68
- ksp refs: HighLogic=1; fan_in=5 fan_out=6
- methods(8): Awake, Create, DoUpdateBlocks, Initialize, OnInitialize, OnUpdate, SetRigidbodyTarget, SetWheelTransform
- auto suggestion: tier=B subsystem=PartModules-Wheels

## Assembly-CSharp/KSPWheelGravity.cs
- 59 lines; kind=class; type=KSPWheelGravity; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=8
- ksp refs: none; fan_in=2 fan_out=0
- methods(1): Refresh
- auto suggestion: tier=A subsystem=PartModules-Wheels

## Assembly-CSharp/LadderEndCheck.cs
- 45 lines; kind=MonoBehaviour; type=LadderEndCheck; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Collision/Force=3, Collider=3, MonoBehaviour=1; unity_score=80
- ksp refs: KerbalEVA=1; fan_in=1 fan_out=1
- methods(3): OnTriggerEnter, OnTriggerExit, OnTriggerStay
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/LandClassROC.cs
- 404 lines; kind=class; type=LandClassROC; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: GameObject=34, Transform=19, SetActive=4, GetComponent=3, Renderer=2, Instantiate/Destroy=2; unity_score=59
- ksp refs: PQS=7, HighLogic=6, Game=1, CelestialBody=1; fan_in=3 fan_out=11
- methods(9): AddScatterMeshController, BuildCache, ClearCache, CreateCache, CreateScatterMesh, DestroyQuad, Setup, SphereActive, SphereInactive
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/LatLon.cs
- 121 lines; kind=static class; type=LatLon; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=1
- ksp refs: Vector3d=29, Planetarium=11; fan_in=8 fan_out=3
- methods(10): GetAltitude, GetImpactLatitudeAndLongitude, GetLatLongAlt, GetLatitude, GetLatitudeAndLongitude, GetLongitude, GetRelSurfaceNVector, GetRelSurfacePosition, GetSurfaceNVector, GetWorldSurfacePosition
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/LaunchClamp.cs
- 373 lines; kind=PartModule; type=LaunchClamp; bases=PartModule,IStageSeparator; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=8, Joint=8, GameObject=8, GetComponent=4, Collider=4, Coroutine=3; unity_score=72
- ksp refs: KSPField=8, HighLogic=4, ConfigNode=2, Vessel=1, PartModule=1, KSPEvent=1; fan_in=3 fan_out=12
- methods(20): CreatePoints, EnableExtension, ExtendTower, FindCollider, GetStageIndex, MakePoint, OnActive, OnInitialize, OnLoad, OnPutToGround, OnSave, OnStart, OnVesselPack, OnVesselUnpack, Release, ReleaseClamp, SetJoint, SetJointWhenPartStarted, UnsetJoint, Update
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/LaunchPadFX.cs
- 72 lines; kind=MonoBehaviour; type=LaunchPadFX; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=4, Lifecycle=3, MonoBehaviour=1, Material=1; unity_score=75
- ksp refs: FloatingOrigin=2; fan_in=1 fan_out=1
- methods(4): AddFX, LateUpdate, OnDestroy, Start
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/LaunchSite.cs
- 291 lines; kind=class; type=LaunchSite; bases=ISiteNode; ancestry=ISiteNode; ns=(global)
- usings: Expansions.Missions.Scenery.Scripts, KSP.Localization, KSP.UI.Screens.Mapview, UnityEngine
- unity: Transform=10, GetComponent=1, GameObject=1; unity_score=34
- ksp refs: Vector3d=3, PQS=2, CelestialBody=2, Localizer=1; fan_in=27 fan_out=10
- methods(11): GetName, GetSpawnPoint, GetSpawnPointLatLonAlt, GetSpawnPointTransform, GetWorldPos, GreatCircleDistance, SetSpawnPointLatLonAlt, SetSpawnPointsLatLonAlt, Setup, UpdateNodeCaption, setupGreatCircleVariables
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/LaunchSiteFacility.cs
- 412 lines; kind=MonoBehaviour; type=LaunchSiteFacility; bases=SpaceCenterBuilding; ancestry=SpaceCenterBuilding/MonoBehaviour; ns=(global)
- usings: Expansions, Expansions.Missions, KSP.Localization, KSP.UI.Screens, PreFlightTests, TMPro, UnityEngine
- unity: Texture=1, MonoBehaviour=1, IMGUI=1; unity_score=26
- ksp refs: HighLogic=16, Localizer=14, GameEvents=14, InputLockManager=11, Game=4, ProtoVessel=1; fan_in=2 fan_out=46
- methods(18): Cancel, ClearSite, IsOpen, NothingToLaunch, OnClicked, OnOnDestroy, OnStart, RemoveInputlock, ResumeFlightOnSite, ReturnToDialog, launchChecks, launchVessel, onGUILaunchScreenDespawn, onMissionDialogDismiss, onMissionDialogUp, shipSelected, showFacilityLocked, showShipSelection
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/LayerUtil.cs
- 4 lines; kind=static class; type=LayerUtil; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=21 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/LED.cs
- 101 lines; kind=MonoBehaviour; type=LED; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Renderer=4, Material=4, GetComponent=4, Coroutine=4, Texture=1, MonoBehaviour=1; unity_score=80
- ksp refs: none; fan_in=0 fan_out=0
- methods(6): Awake, blink, doBlink, setColor, setOff, setOn
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/LightStruct.cs
- 165 lines; kind=class; type=LightStruct; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Shader=8, Material=8, Transform=5, Renderer=4, Light=2, GetComponent=1; unity_score=69
- ksp refs: Part=1, ConfigNode=1; fan_in=1 fan_out=2
- methods(4): FindLightComponents, FixedUpdateLightsStatus, Load, UpdateLightStatusOnEditor
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/LinearGauge.cs
- 151 lines; kind=MonoBehaviour; type=LinearGauge; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GetComponent=5, Audio=5, Transform=3, Time.=2, Lifecycle=2, MonoBehaviour=1; unity_score=69
- ksp refs: HighLogic=1; fan_in=0 fan_out=2
- methods(4): Start, Update, setValue, startSound
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/LingoonaGrammarExtensions.cs
- 112 lines; kind=static class; type=LingoonaGrammarExtensions; bases=-; ancestry=-; ns=(global)
- usings: FinePrint.Utilities
- unity: none; unity_score=0
- ksp refs: ProtoCrewMember=3; fan_in=0 fan_out=2
- methods(10): LocalizeCommonNoun, LocalizeCommonNounFemale, LocalizeCommonNounMale, LocalizeCommonNounNeutral, LocalizeName, LocalizeNameFemale, LocalizeNameMale, LocalizeNameNeutral, LocalizeNoun, LocalizeRemoveGender
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/LinkedMesh.cs
- 781 lines; kind=class; type=LinkedMesh; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Mesh=5, Time.=4, Raycast/Query=1; unity_score=18
- ksp refs: none; fan_in=1 fan_out=0
- methods(21): CalculateTangets, ClearMesh, ClearSelection, ClearVerts, ComputeNormals, ComputeSelectionNormals, CreateLinks, GetMesh, HasVertsInTri, IsNeighbourOf, IsVertInTri, NormalizeMesh, RayIntersectMesh, RayIntersectTriangle, Raycast, SelectByLinks, SelectMesh, SetLinkTris, SetLinks, SetVerts, UpdateSelection
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/LinkedMeshSO.cs
- 499 lines; kind=ScriptableObject; type=LinkedMeshSO; bases=ScriptableObject; ancestry=ScriptableObject; ns=(global)
- usings: UnityEngine
- unity: Mesh=8, Time.=2, ScriptableObject=1; unity_score=32
- ksp refs: none; fan_in=0 fan_out=1
- methods(10): CalculateSize, CalculateTangets, ComputeNormals, Contains, CreateLinkedMesh, GetCloneMesh, GetOriginalMesh, IndexOf, NormalizeMesh, SetupClone
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ListDictionary.cs
- 102 lines; kind=class; type=ListDictionary; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=4
- ksp refs: none; fan_in=5 fan_out=0
- methods(5): Add, Clear, GetEnumerator, Remove, TryGetValue
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/ListExtension.cs
- 235 lines; kind=static class; type=ListExtension; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=0
- ksp refs: Part=6; fan_in=0 fan_out=1
- methods(9): AddUnique, AddUniqueRange, ContainsId, MaxAt, MinAt, OrderByAlphaNumeric, OrderByAlphanumeric, SetRange, Shuffle
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/LoadCraftDialog.cs
- 397 lines; kind=MonoBehaviour; type=LoadCraftDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, KSP.Localization, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=5, GameObject=4, Instantiate/Destroy=3, Time.=2, Texture=2, MonoBehaviour=1; unity_score=47
- ksp refs: Localizer=6, ShipConstruct=3, HighLogic=2, ConfigNode=2; fan_in=0 fan_out=26
- methods(15): BuildCraftList, CancelCallback, Create, CreateBrowserDialog, CreateBrowserItems, CreateDeleteFileConfirm, CreateWindowCleanupPrompt, DeleteInvalidSaves, OnDeleteConfirm, OnDestroy, SelectFileCallback, SelectItem, ShowDeleteFileConfirm, ShowWindowBrowser, ShowWindowCleanupPrompt
- auto suggestion: tier=C subsystem=Editor

## Assembly-CSharp/LoadGameDialog.cs **MUST-OPEN (>800 lines)**
- 934 lines; kind=MonoBehaviour; type=LoadGameDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Missions, KSP.Localization, KSP.UI, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=8, GameObject=8, Transform=6, Texture=6, Instantiate/Destroy=5, GetComponent=3; unity_score=52
- ksp refs: Localizer=25, ConfigNode=22, Game=10, PartLoader=4, ProtoVessel=1; fan_in=6 fan_out=38
- methods(29): AreVesselsInFlightCompatible, CancelLoadGame, ClearListItems, ConfirmDeleteGame, ConfirmLoadGame, Create, CreateLoadList, DismissDeleteGame, FinishedCallback, GetLastWriteTime, GetSFSMD5, GetSaveData, IsCompatible, Load, LoadDetailsFromGame, LoadFromMetaFile, LoadGame, LoadMenuNav, OnBtnCancel, OnButtonDelete, OnButtonLoad, OnSelectionChanged, PersistentLoadGame, Save, SaveToMetaFile, SetHidden, SetLocked, Start, Update
- auto suggestion: tier=C subsystem=Save

## Assembly-CSharp/LoadGameSearch.cs
- 166 lines; kind=MonoBehaviour; type=LoadGameSearch; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=6, Transform=6, GetComponent=6, TMPro=4, Lifecycle=4, Coroutine=4; unity_score=86
- ksp refs: InputLockManager=2; fan_in=0 fan_out=5
- methods(12): Awake, HasFiltered, ItemMatchesSearch, OnDestroy, OnDisable, SearchField_OnClick, SearchField_OnEndEdit, SearchField_OnValueChange, SearchRoutine, SearchStart, SearchStop, Update
- auto suggestion: tier=C subsystem=Save

## Assembly-CSharp/LoadingAnimation.cs
- 36 lines; kind=MonoBehaviour; type=LoadingAnimation; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, Transform=1, MonoBehaviour=1; unity_score=66
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Start, Update
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/LoadingBufferMask.cs
- 132 lines; kind=MonoBehaviour; type=LoadingBufferMask; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, TMPro, UnityEngine, UnityEngine.SceneManagement
- unity: Coroutine=9, SetActive=4, SceneManager=3, GameObject=3, Lifecycle=2, TMPro=1; unity_score=76
- ksp refs: HighLogic=6, FlightGlobals=4, GameEvents=2; fan_in=8 fan_out=6
- methods(8): Hide, OnDestroy, OnLevelLoaded, OnSceneChange, OnSceneLoaded, Show, ShowDuration, Start
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/LoadingScreen.cs
- 327 lines; kind=MonoBehaviour; type=LoadingScreen; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, TMPro, UnityEngine, UnityEngine.Networking, UnityEngine.SceneManagement, UnityEngine.UI
- unity: Time.=14, Coroutine=14, GameObject=7, Texture=6, TMPro=3, Lifecycle=3; unity_score=78
- ksp refs: Localizer=2, GameSettings=1, GameDatabase=1; fan_in=2 fan_out=10
- methods(10): Awake, FadeIn, FadeOut, LoadSystems, OnDestroy, SetTip, Start, StartLoadingScreens, UpdateLoadingScreen, UpdateProgressBar
- auto suggestion: tier=C subsystem=Scene

## Assembly-CSharp/LoadingSystem.cs
- 28 lines; kind=MonoBehaviour; type=LoadingSystem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=36
- ksp refs: none; fan_in=5 fan_out=0
- methods(5): IsReady, LoadWeight, ProgressFraction, ProgressTitle, StartLoad
- auto suggestion: tier=B subsystem=Modding

## Assembly-CSharp/LocalParticleGravity.cs
- 21 lines; kind=MonoBehaviour; type=LocalParticleGravity; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=6, Transform=1, MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=93
- ksp refs: FlightGlobals=2, PhysicsGlobals=1; fan_in=0 fan_out=2
- methods(1): Start
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/LocalPhysXGravity.cs
- 16 lines; kind=MonoBehaviour; type=LocalPhysXGravity; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, Lifecycle=2, Physics.=1, MonoBehaviour=1; unity_score=78
- ksp refs: FlightGlobals=2; fan_in=0 fan_out=1
- methods(2): FixedUpdate, Start
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/LocalSpace.cs
- 27 lines; kind=MonoBehaviour; type=LocalSpace; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=3, Lifecycle=2, Instantiate/Destroy=2, MonoBehaviour=1, GameObject=1; unity_score=86
- ksp refs: none; fan_in=3 fan_out=0
- methods(2): Awake, OnDestroy
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/LookAtObject.cs
- 46 lines; kind=MonoBehaviour; type=LookAtObject; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=4, Transform=3, MonoBehaviour=1; unity_score=75
- ksp refs: none; fan_in=1 fan_out=1
- methods(5): Awake, FixedUpdate, LateUpdate, Update, onUpdate
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/MainCanvasUtil.cs
- 33 lines; kind=static class; type=MainCanvasUtil; bases=-; ancestry=-; ns=(global)
- usings: KSP.UI, UnityEngine
- unity: UnityEngine.UI=5, GetComponent=1; unity_score=67
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/MainMenu.cs **MUST-OPEN (>800 lines)**
- 1023 lines; kind=MonoBehaviour; type=MainMenu; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, Expansions.Missions, Expansions.Missions.Runtime, KSP.Localization, KSP.Testing, KSP.UI.Util, SaveUpgradePipeline, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=31, SetActive=25, Texture=6, UnityEngine.UI=5, TMPro=3, Lifecycle=3; unity_score=60
- ksp refs: Localizer=39, GameSettings=23, HighLogic=22, InputLockManager=17, Game=16, ConfigNode=10; fan_in=5 fan_out=58
- methods(52): Awake, BuyExpansionPack, BuyExpansionPack2, CancelNewGame, CancelOverwriteNewGame, ConfirmNewGame, CreateAndStartGame, CreateNewGameDialog, DeleteDirectory, GoToMerchandisingSite, HighlightFirstItem, HighlightNextItem, LoadGame, MenuNavHandler, MissionEditor, MouseIsHovering, NewGame, OnApplicationFocus, OnDestroy, OnDifficultyOptionsDismiss, OnFlagCancel, OnFlagSelect, OnInputLocksModified, OnLoadDialogFinished, OnLoadDialogPipelineFail, OnLoadDialogPipelineFinished, OnMissionLoadDismiss, OnOpenFlagBrowser, OnScenarioLoadDismiss, OnScenarioLoadPipelineFailed, OnScenarioLoadPipelineFinished, OpenDifficultyOptions, OverWriteNewGame, PlayBaseMissions, PlayMissions, ResetInputLockOnQuit, ResetNavigationOnStageChange, SelectHighlithedItem, Start, StartScenario, StartTraining, UpdatedGameParameters, cancelStartGame, credits, goToKSPSite ...
- auto suggestion: tier=C subsystem=Scene

## Assembly-CSharp/MainMenuEnvLogic.cs
- 114 lines; kind=MonoBehaviour; type=MainMenuEnvLogic; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine
- unity: Transform=12, SetActive=3, TMPro=2, Renderer=2, Material=2, Lifecycle=2; unity_score=85
- ksp refs: none; fan_in=1 fan_out=2
- methods(5): Awake, DistanceFadeUI, GoCurrentStage, GoToStage, Update
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/MainMenuExpressionManager.cs
- 23 lines; kind=MonoBehaviour; type=MainMenuExpressionManager; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=3, MonoBehaviour=1, Lifecycle=1, Animation=1; unity_score=88
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): Start
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/MainMenuTerrainSelector.cs
- 21 lines; kind=MonoBehaviour; type=MainMenuTerrainSelector; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=7, SetActive=1, MonoBehaviour=1, Lifecycle=1; unity_score=88
- ksp refs: GameSettings=2; fan_in=0 fan_out=1
- methods(1): Start
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/MakingHistoryAboutDialog.cs
- 80 lines; kind=MonoBehaviour; type=MakingHistoryAboutDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, GameObject=3, Transform=2, Lifecycle=2, Instantiate/Destroy=2, Texture=1; unity_score=85
- ksp refs: InputLockManager=2, GameSettings=2; fan_in=1 fan_out=6
- methods(7): Create, Dismiss, OnCloseButton, OnDontShowAgainToggle, OnMoreInfoButton, Start, Update
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/ManeuverGizmo.cs
- 368 lines; kind=MonoBehaviour; type=ManeuverGizmo; bases=ManeuverGizmoBase; ancestry=ManeuverGizmoBase/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, UnityEngine, UnityEngine.UI
- unity: Input.=7, UnityEngine.UI=4, Lifecycle=3, GetComponent=2, Camera=2, Transform=1; unity_score=51
- ksp refs: Orbit=8, InputLockManager=6, Vector3d=2, Planetarium=2, Localizer=1; fan_in=3 fan_out=18
- methods(26): DraggingForwardInTime, HandlesUpdatedCallback, LateUpdate, NextOrbit, NextOrbitPossible, OnAntinormalUpdate, OnDeletePress, OnDestroy, OnMinusOrbitPress, OnMouseDown, OnMouseDrag, OnMouseUp, OnNormalUpdate, OnPlusOrbitPress, OnProgradeUpdate, OnRadialInUpdate, OnRadialOutUpdate, OnRetrogradeUpdate, PreviousOrbit, PreviousOrbitPossible, SetMouseOverGizmo, SetPatches, Setup, Start, UpdateOrbitButtons, onGizmoUpdate
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/ManeuverGizmoBase.cs
- 234 lines; kind=MonoBehaviour; type=ManeuverGizmoBase; bases=MonoBehaviour,IMouseEvents; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=12, GameObject=9, SetActive=8, Time.=3, MonoBehaviour=2, Material=2; unity_score=75
- ksp refs: Vector3d=7; fan_in=6 fan_out=3
- methods(17): ChangeToButtonMode, GetInstance, LateUpdate, OnAntinormalUpdate, OnMouseDown, OnMouseDrag, OnMouseEnter, OnMouseExit, OnMouseUp, OnNormalUpdate, OnProgradeUpdate, OnRadialInUpdate, OnRadialOutUpdate, OnRetrogradeUpdate, SetMouseOverGizmo, Start, Terminate
- auto suggestion: tier=B subsystem=MapView

## Assembly-CSharp/ManeuverGizmoHandle.cs
- 204 lines; kind=MonoBehaviour; type=ManeuverGizmoHandle; bases=MonoBehaviour,IMouseEvents; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=6, Material=4, Time.=3, GetComponent=3, Renderer=2, MonoBehaviour=2; unity_score=68
- ksp refs: GameSettings=2; fan_in=2 fan_out=3
- methods(11): GetInstance, HandleUpdate, OnMouseDown, OnMouseDrag, OnMouseEnter, OnMouseExit, OnMouseUp, SetHandleColor, Start, Update, UpdateHandle
- auto suggestion: tier=B subsystem=MapView

## Assembly-CSharp/ManeuverNode.cs
- 133 lines; kind=class; type=ManeuverNode; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: UnityEngine
- unity: GameObject=2, SetActive=1, Instantiate/Destroy=1, GetComponent=1; unity_score=25
- ksp refs: Vector3d=10, Orbit=3, QuaternionD=2, ConfigNode=2, MapView=1, GameEvents=1; fan_in=19 fan_out=13
- methods(8): AttachGizmo, DetachGizmo, GetBurnVector, GetPartialDv, Load, OnGizmoUpdated, RemoveSelf, Save
- auto suggestion: tier=B subsystem=Orbit

## Assembly-CSharp/ManeuverNodeEditorManager.cs **MUST-OPEN (>800 lines)**
- 814 lines; kind=MonoBehaviour; type=ManeuverNodeEditorManager; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.TooltipTypes, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=10, GameObject=7, SetActive=6, Lifecycle=4, Instantiate/Destroy=4, EventSystems=4; unity_score=52
- ksp refs: FlightGlobals=35, GameEvents=20, MapView=11, Localizer=11, HighLogic=3, InputLockManager=2; fan_in=12 fan_out=24
- methods(35): AssignSelectedManeuverNode, Awake, CacheLocalStrings, ChangeTooltipText, DeleteManNode, GetTabToggle, ManNodeCreateCooldown, ModifyBurnVector, NextManNode, OnDestroy, OnManNodeDeselected, OnManNodeSelected, OnMapEntered, OnMapExited, OnPointerEnter, OnPointerExit, OnSceneSwitch, OnTabButtonPressed, OnTargetChanged, PrevManNode, Reset, SetCurrentTab, SetManeuverNodeInitialValues, SetMouseOverGizmo, SetPauseButtonState, Start, TriggerManNodeChange, Update, UpdateButtonAvailability, UpdateDeltaVInNodeComponents, UpdateTabButtons, WarpToNextManNode, addTab, onUIModeFinishedChanging, setupAllTabs
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/ManeuverNodeEditorSlider.cs
- 39 lines; kind=MonoBehaviour; type=ManeuverNodeEditorSlider; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: EventSystems=4, UnityEngine.UI=3, MonoBehaviour=1, Lifecycle=1; unity_score=87
- ksp refs: GameSettings=2; fan_in=0 fan_out=1
- methods(3): OnPointerEnter, OnPointerExit, Update
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/ManeuverNodeEditorTab.cs
- 87 lines; kind=MonoBehaviour; type=ManeuverNodeEditorTab; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=4, Transform=3, UnityEngine.UI=2, Texture=2, Time.=1, MonoBehaviour=1; unity_score=79
- ksp refs: InputLockManager=1; fan_in=7 fan_out=5
- methods(9): IsTabInteractable, OnPointerEnter, OnPointerExit, SetInitialValues, Setup, Update, UpdateUIElements, WrapperUpdateUIElements, setMouseOver
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/ManeuverNodeEditorTabButton.cs
- 29 lines; kind=MonoBehaviour; type=ManeuverNodeEditorTabButton; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI.TooltipTypes, UnityEngine, UnityEngine.Events, UnityEngine.UI
- unity: UnityEngine.UI=5, Transform=4, MonoBehaviour=1, GetComponent=1; unity_score=94
- ksp refs: none; fan_in=3 fan_out=2
- methods(1): Setup
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/ManeuverNodeEditorTabIntercept.cs
- 347 lines; kind=MonoBehaviour; type=ManeuverNodeEditorTabIntercept; bases=ManeuverNodeEditorTab; ancestry=ManeuverNodeEditorTab/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.TooltipTypes, TMPro, UnityEngine
- unity: TMPro=7, Lifecycle=1, GetComponent=1; unity_score=42
- ksp refs: FlightGlobals=34, Localizer=22, Orbit=5, Planetarium=4, Vector3d=2, InputLockManager=1; fan_in=1 fan_out=21
- methods(8): CacheLocalStrings, GetInterceptValues, IsTabInteractable, SetEditorTab, SetInitialValues, SetIntersectOrApproach, Start, UpdateUIElements
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/ManeuverNodeEditorTabOrbitAdv.cs
- 69 lines; kind=MonoBehaviour; type=ManeuverNodeEditorTabOrbitAdv; bases=ManeuverNodeEditorTab; ancestry=ManeuverNodeEditorTab/MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine
- unity: TMPro=5, GetComponent=2, Lifecycle=1; unity_score=75
- ksp refs: Planetarium=1, Orbit=1, InputLockManager=1, FlightGlobals=1; fan_in=0 fan_out=13
- methods(4): IsTabInteractable, SetInitialValues, Start, UpdateUIElements
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/ManeuverNodeEditorTabOrbitBasic.cs
- 83 lines; kind=MonoBehaviour; type=ManeuverNodeEditorTabOrbitBasic; bases=ManeuverNodeEditorTab; ancestry=ManeuverNodeEditorTab/MonoBehaviour; ns=(global)
- usings: KSP.Localization, TMPro, UnityEngine
- unity: TMPro=5, GetComponent=2, Lifecycle=1; unity_score=71
- ksp refs: Localizer=4, FlightGlobals=3, Planetarium=2, Orbit=1; fan_in=1 fan_out=12
- methods(5): CacheLocalStrings, IsTimeAndPeriodUnlocked, SetInitialValues, Start, UpdateUIElements
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/ManeuverNodeEditorTabPosition.cs
- 5 lines; kind=enum; type=ManeuverNodeEditorTabPosition; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=MapView

## Assembly-CSharp/ManeuverNodeEditorTabVectorHandles.cs
- 204 lines; kind=MonoBehaviour; type=ManeuverNodeEditorTabVectorHandles; bases=ManeuverNodeEditorTab; ancestry=ManeuverNodeEditorTab/MonoBehaviour; ns=(global)
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=11, GetComponent=9, TMPro=1, Lifecycle=1; unity_score=72
- ksp refs: FlightGlobals=8, Localizer=2; fan_in=1 fan_out=8
- methods(16): AntiNormalStepUp, CacheLocalStrings, NextOrbitLoop, NormalStepUp, OnPrecisionValueChanged, PrevOrbitLoop, ProgradeStepUp, RadialInStepUp, RadialOutStepUp, RetrogradeStepUp, SetHandlesSensitivity, SetInitialValues, Start, TimeStepDown, TimeStepUp, UpdateUIElements
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/ManeuverNodeEditorTabVectorInput.cs
- 171 lines; kind=MonoBehaviour; type=ManeuverNodeEditorTabVectorInput; bases=ManeuverNodeEditorTab; ancestry=ManeuverNodeEditorTab/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.TooltipTypes, TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=5, GetComponent=3, SetActive=2, Lifecycle=2, GameObject=2, UnityEngine.UI=1; unity_score=65
- ksp refs: Localizer=4, InputLockManager=2, FlightGlobals=2, Vector3d=1; fan_in=3 fan_out=15
- methods(9): CacheLocalStrings, ChangeInputMode, OnDestroy, OnFieldDeselected, OnFieldSelected, OnTabToggleClicked, SetInitialValues, Start, UpdateUIElements
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/ManeuverNodeEditorVectorHandle.cs
- 214 lines; kind=MonoBehaviour; type=ManeuverNodeEditorVectorHandle; bases=MonoBehaviour,IDragHandler,IEventSystemHandler,IPointerEnterHandler,IPointerExitHandler,IPointerDownHandler,IPointerUpHandler; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: EventSystems=12, UnityEngine.UI=4, Input.=4, Coroutine=4, Lifecycle=2, GetComponent=2; unity_score=76
- ksp refs: GameSettings=2; fan_in=1 fan_out=5
- methods(8): MouseUpCooldown, OnDrag, OnPointerDown, OnPointerEnter, OnPointerExit, OnPointerUp, Start, Update
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/ManeuverTool.cs
- 514 lines; kind=MonoBehaviour; type=ManeuverTool; bases=UIApp; ancestry=UIApp/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, UnityEngine, UnityEngine.EventSystems
- unity: GameObject=6, Transform=4, SetActive=3, UnityEngine.UI=2, Lifecycle=1, Instantiate/Destroy=1; unity_score=41
- ksp refs: GameEvents=8, Localizer=6, FlightGlobals=5, Vessel=4, HighLogic=4, Game=2; fan_in=3 fan_out=24
- methods(31): ChangeTime, CreateManeuver, CreateManeuverChecksComplete, DeletePriorManeuvers, DisplayApp, FollowingManevuersCheck, GenerateTransferTypes, GetAppScenes, GetAppScreenPos, GetTransferVisualPrefab, HideApp, Initial_MouseInput_PointerExit, InvokeAsync, IsAnyDropdownOpen, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnFollowingCheckCancel, OnFollowingCheckOk, OnFollowingLeave, OnManeuverChanged, OnPositionChanged, OnPriorCheckCancel, OnPriorCheckOk, OnTransferTypeChanged, OnVesselSwitching, PopulatePositionList, PopulateTransferTypes, ReadjustPosition, Reposition, Update
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/ManeuverToolCBContainer.cs
- 206 lines; kind=MonoBehaviour; type=ManeuverToolCBContainer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Transform=17, GameObject=12, GetComponent=8, Shader=5, Renderer=5, Material=5; unity_score=94
- ksp refs: Localizer=1; fan_in=2 fan_out=5
- methods(14): CreateThumbnailCamera, Hide, OnDestroy, OnFalse, OnPointerEnter, OnPointerExit, OnTrue, RotatePlanet, SetLightDirection, Setup, Show, StartRotation, StopRotation, Update
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/ManeuverToolVisualTextBox.cs
- 27 lines; kind=MonoBehaviour; type=ManeuverToolVisualTextBox; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine
- unity: TMPro=2, MonoBehaviour=1; unity_score=72
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): SetNameText, SetValueText, Setup
- auto suggestion: tier=B subsystem=MapView

## Assembly-CSharp/MapDisplayTypes.cs
- 7 lines; kind=enum; type=MapDisplayTypes; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=MapView

## Assembly-CSharp/MapObject.cs
- 196 lines; kind=MonoBehaviour; type=MapObject; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Missions, KSP.UI.Screens.Mapview, UnityEngine
- unity: Transform=5, GetComponent=5, Lifecycle=4, GameObject=4, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=60
- ksp refs: CelestialBody=7, Vessel=6, Orbit=4, ScaledSpace=3; fan_in=45 fan_out=10
- methods(14): Awake, Create, GetDisplayName, GetName, GetReferenceBody, LateUpdate, OnAwake, OnDestroy, OnGetName, OnLateUpdate, OnStart, OnWillDestroy, Start, Terminate
- auto suggestion: tier=B subsystem=MapView

## Assembly-CSharp/MapSO.cs **MUST-OPEN (>800 lines)**
- 801 lines; kind=ScriptableObject; type=MapSO; bases=ScriptableObject; ancestry=ScriptableObject; ns=(global)
- usings: UnityEngine
- unity: Texture=34, ScriptableObject=1; unity_score=41
- ksp refs: none; fan_in=10 fan_out=1
- methods(25): CompileGreyscale, CompileHeightAlpha, CompileRGB, CompileRGBA, CompileToTexture, ConstructBilinearCoords, CreateGreyscaleFromAlpha, CreateGreyscaleFromRGB, CreateHeightAlpha, CreateMap, CreateMapRGBQuantized, CreateRGB, CreateRGBA, CreateRGBQuantized, GetPixelByte, GetPixelColor, GetPixelColor32, GetPixelFloat, GetPixelHeightAlpha, GreyByte, GreyFloat, Lerp, PixelByte, PixelIndex, SetPixel
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/MapView.cs
- 672 lines; kind=MonoBehaviour; type=MapView; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Runtime, KSP.Localization, KSP.UI.Screens.Flight, KSP.UI.Screens.Mapview, KSP.UI.Util, UnityEngine, Vectrosity
- unity: Camera=22, Renderer=16, Transform=10, Material=10, GameObject=10, Coroutine=5; unity_score=74
- ksp refs: FlightGlobals=8, Planetarium=7, GameEvents=6, OrbitDriver=5, HighLogic=5, InputLockManager=4; fan_in=61 fan_out=32
- methods(19): AddLaunchSiteNode, Awake, CreateKSCNode, CreateLaunchSiteNodes, CreateOrbitRenderers, CreateVectorCanvasCam, EnterMapView, ExitMapView, GetEccOffset, OnDestroy, Start, Update, UpdateMap, createMapFXCamera, enableMapFXCameras, endEnterMapTransition, endExitMapTransition, enterMapView, exitMapView
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/MapViewFiltering.cs
- 478 lines; kind=MonoBehaviour; type=MapViewFiltering; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, KSP.UI, KSP.UI.Screens, UnityEngine, UnityEngine.UI
- unity: Coroutine=4, UnityEngine.UI=3, Lifecycle=3, SetActive=1, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=38
- ksp refs: GameEvents=19, Vessel=7, FlightGlobals=6, InputLockManager=4, ProtoVessel=2, MapView=1; fan_in=9 fan_out=14
- methods(24): Awake, CheckAgainstFilter, CountVessels, GetFilterFromButtonStates, GetFilterState, Init, Initialize, LoadFilterState, OnDestroy, OnInputLocksModified, RefreshCounts, RefreshInOneFrame, SetFilter, SetSolo, Start, ToggleFiltersPanel, UpdateVesselCounts, checkAgainstFilter, onKnowledgeChanged, onVesselCountChanged, onVesselRename, setFilter, updateButtonsToFilter, updateFilterFromButtons
- auto suggestion: tier=C subsystem=MapView

## Assembly-CSharp/MaterialBasedOnGraphicsSetting.cs
- 94 lines; kind=MonoBehaviour; type=MaterialBasedOnGraphicsSetting; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Material=7, Renderer=2, Lifecycle=2, MonoBehaviour=1, GetComponent=1; unity_score=76
- ksp refs: GameSettings=3, GameEvents=2, HighLogic=1; fan_in=0 fan_out=4
- methods(4): ApplyMaterials, Awake, OnDestroy, OnGameSettingsApplied
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/MaterialColorUpdater.cs
- 90 lines; kind=class; type=MaterialColorUpdater; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Renderer=8, Material=4, Transform=3, Shader=3, GetComponent=2; unity_score=72
- ksp refs: Part=2; fan_in=2 fan_out=1
- methods(2): CreateRendererList, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/MaterialExtension.cs
- 15 lines; kind=static class; type=MaterialExtension; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Material=2; unity_score=44
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): CopyKeywordsFrom
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/MaterialPropertyExtensions.cs
- 35 lines; kind=static class; type=MaterialPropertyExtensions; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Material=3, Renderer=2; unity_score=63
- ksp refs: none; fan_in=9 fan_out=0
- methods(3): Apply, SetupProperties, UpdateProperties
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/MaterialSetDirection.cs
- 37 lines; kind=MonoBehaviour; type=MaterialSetDirection; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=4, Renderer=3, Material=2, Lifecycle=2, Shader=1, MonoBehaviour=1; unity_score=94
- ksp refs: none; fan_in=2 fan_out=1
- methods(3): Reset, Start, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/Mathfx.cs
- 149 lines; kind=class; type=Mathfx; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=27
- ksp refs: none; fan_in=7 fan_out=0
- methods(18): Approx, Berp, Bounce, Clerp, Coserp, CubicHermiteSpline, Ease, Easerp, GetWeightedAvgVector3, Hermite, HermiteIntegral, Lerp, NearestPoint, NearestPointStrict, Sinerp, SinerpIntegral, SmoothStep, XLerp
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/Matrix4x4D.cs
- 392 lines; kind=struct; type=Matrix4x4D; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=1
- ksp refs: Vector3d=10; fan_in=0 fan_out=1
- methods(16): Det, Identity, Inverse, Matrix4x4D, MultiplyPoint3x4, RotateXaxis, RotateYaxis, RotateZaxis, Scale, SetIdentity, TransformPoint, TransformVector, Translate, Zero, det2x2, det3x3
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/MatrixArrayMaterialProperty.cs
- 17 lines; kind=class; type=MatrixArrayMaterialProperty; bases=MaterialPropertyExtensions.MaterialProperty; ancestry=MaterialProperty; ns=(global)
- usings: UnityEngine
- unity: Material=1; unity_score=32
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): Apply
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/MatrixMaterialProperty.cs
- 17 lines; kind=class; type=MatrixMaterialProperty; bases=MaterialPropertyExtensions.MaterialProperty; ancestry=MaterialProperty; ns=(global)
- usings: UnityEngine
- unity: Shader=1, Material=1; unity_score=46
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): Apply
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/MBMReader.cs
- 116 lines; kind=static class; type=MBMReader; bases=-; ancestry=-; ns=(global)
- usings: PartToolsLib, UnityEngine
- unity: Texture=11; unity_score=55
- ksp refs: none; fan_in=1 fan_out=2
- methods(2): Read, ReadTexture2D
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/MCAvatarController.cs
- 423 lines; kind=MonoBehaviour; type=MCAvatarController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Time.=4, Lifecycle=3, Animation=2, MonoBehaviour=1; unity_score=31
- ksp refs: Localizer=38; fan_in=1 fan_out=4
- methods(9): DetermineAndSetFallbackLoop, NewAvatarState, OnInstructorReady, OnReady, Start, TriggerAnim, TriggerAnimation, Update, getThoughtVector
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/MCDespawner.cs
- 18 lines; kind=MonoBehaviour; type=MCDespawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1, GameObject=1; unity_score=74
- ksp refs: GameEvents=1; fan_in=0 fan_out=1
- methods(2): BtnExit, Start
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/MCSceneSpawner.cs
- 32 lines; kind=MonoBehaviour; type=MCSceneSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1; unity_score=58
- ksp refs: GameEvents=4, HighLogic=1; fan_in=0 fan_out=7
- methods(4): Awake, OnDestroy, OnMCDespawn, OnMCSpawn
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/MEBannerBrowser.cs
- 206 lines; kind=MonoBehaviour; type=MEBannerBrowser; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Missions, Expansions.Missions.Editor, KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, Transform=2, MonoBehaviour=1; unity_score=38
- ksp refs: Localizer=9, GameDatabase=4; fan_in=1 fan_out=23
- methods(12): Accept, BannerDeletedCallback, BannerSelectedCallback, ConfirmDeleteBanner, CreateBrowser, Dismiss, GetDuplicate, LoadBannerButtons, LoadGameDataBanners, LoadMissionBanners, OnButtonDelete, Setup
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/MEBannerEntry.cs
- 134 lines; kind=class; type=MEBannerEntry; bases=-; ancestry=-; ns=(global)
- usings: Expansions.Missions, UnityEngine
- unity: Texture=3, Instantiate/Destroy=1; unity_score=26
- ksp refs: none; fan_in=5 fan_out=5
- methods(9): CopySource, DeleteSource, DestroyTexture, GetBannerDefaultTexture, GetBannerGameDataPath, GetStockBannerGameDataPath, HasValidSource, LoadFromMissionFolder, SetToDefault
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/MEBannerType.cs
- 6 lines; kind=enum; type=MEBannerType; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=9 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/MEBasicNodeListFilter.cs
- 36 lines; kind=class; type=MEBasicNodeListFilter; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(2): FilterList, ToString
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/MEBasicNodeListFilterList.cs
- 59 lines; kind=class; type=MEBasicNodeListFilterList; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(6): AddFilter, ClearFilter, FilterList, GetFilteredList, RemoveFilter, ToString
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Mechanic.cs
- 128 lines; kind=MonoBehaviour; type=Mechanic; bases=SpaceCenterCrew; ancestry=SpaceCenterCrew/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2; unity_score=29
- ksp refs: none; fan_in=7 fan_out=1
- methods(3): SetAnimation, Start, Update
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/MechanicDriller.cs
- 89 lines; kind=MonoBehaviour; type=MechanicDriller; bases=SpaceCenterCrew; ancestry=SpaceCenterCrew/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2; unity_score=33
- ksp refs: none; fan_in=0 fan_out=2
- methods(3): SetAnimation, Start, Update
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/MechanicHammer.cs
- 128 lines; kind=MonoBehaviour; type=MechanicHammer; bases=SpaceCenterCrew; ancestry=SpaceCenterCrew/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2; unity_score=29
- ksp refs: none; fan_in=0 fan_out=2
- methods(3): SetAnimation, Start, Update
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/MechanicLugWrench.cs
- 128 lines; kind=MonoBehaviour; type=MechanicLugWrench; bases=SpaceCenterCrew; ancestry=SpaceCenterCrew/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2; unity_score=27
- ksp refs: none; fan_in=0 fan_out=2
- methods(3): SetAnimation, Start, Update
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/MechanicScrew.cs
- 128 lines; kind=MonoBehaviour; type=MechanicScrew; bases=SpaceCenterCrew; ancestry=SpaceCenterCrew/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2; unity_score=29
- ksp refs: none; fan_in=0 fan_out=2
- methods(3): SetAnimation, Start, Update
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/MechanicWrench.cs
- 128 lines; kind=MonoBehaviour; type=MechanicWrench; bases=SpaceCenterCrew; ancestry=SpaceCenterCrew/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2; unity_score=29
- ksp refs: none; fan_in=0 fan_out=2
- methods(3): SetAnimation, Start, Update
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/MechanicWrenchEnglish.cs
- 128 lines; kind=MonoBehaviour; type=MechanicWrenchEnglish; bases=SpaceCenterCrew; ancestry=SpaceCenterCrew/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2; unity_score=29
- ksp refs: none; fan_in=0 fan_out=2
- methods(3): SetAnimation, Start, Update
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/MEGapCategoryScroll.cs
- 30 lines; kind=MonoBehaviour; type=MEGapCategoryScroll; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Editor, UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=4, MonoBehaviour=1, Lifecycle=1; unity_score=83
- ksp refs: none; fan_in=0 fan_out=1
- methods(3): OnDestroy, OnPointerEnter, OnPointerExit
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/MemoryCache.cs
- 84 lines; kind=static class; type=MemoryCache; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: none; fan_in=4 fan_out=0
- methods(3): CleanUp, GetBytes, GetColor32
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/MenuGravityOverride.cs
- 11 lines; kind=MonoBehaviour; type=MenuGravityOverride; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Physics.=1, MonoBehaviour=1, Lifecycle=1; unity_score=64
- ksp refs: PhysicsGlobals=1; fan_in=0 fan_out=1
- methods(1): Start
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/MenuMusic.cs
- 78 lines; kind=MonoBehaviour; type=MenuMusic; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.SceneManagement
- unity: SceneManager=3, Instantiate/Destroy=3, GameObject=3, Audio=3, SendMessage/Invoke=2, Lifecycle=2; unity_score=80
- ksp refs: HighLogic=1, GameSettings=1; fan_in=1 fan_out=3
- methods(6): Awake, OnDestroy, OnLevelLoaded, OnSceneLoaded, PlayAmbienceLoop, PlayTheme
- auto suggestion: tier=C subsystem=Audio

## Assembly-CSharp/MenuNavigation.cs
- 689 lines; kind=MonoBehaviour; type=MenuNavigation; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=34, EventSystems=20, GameObject=17, Transform=16, KeyCode=13, Input.=13; unity_score=83
- ksp refs: InputLockManager=3, GameEvents=1; fan_in=22 fan_out=8
- methods(26): CheckKeyboardInput, EnsureFocusIsWithinDialog, FocusNextItemFromTextInput, FocusOnItemByAnchoredPos, FocusOnItemByScrollbar, GetSelectableParent, IsSearchFieldFocused, IsSelectableValid, MouseIsHovering, OnDestroy, RefocusOnInputDetect, ResetAll, ResetSelectablesOnly, SelectLastArrowSelected, SetItemAsFirstSelected, SetLastItemSelected, SetMenuNavInputLock, SetSameParentExplicitNavigation, SetSearchField, SetSelectableItems, SetVerticalExplicitNavigation, SpawnMenuNavigation, Start, SumbmitOnSelectedToggle, ToggleHighlightedElements, Update
- auto suggestion: tier=C subsystem=Scene

## Assembly-CSharp/MenuNavInput.cs
- 11 lines; kind=enum; type=MenuNavInput; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp/MenuRandomKerbalAnims.cs
- 35 lines; kind=MonoBehaviour; type=MenuRandomKerbalAnims; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, Time.=1, MonoBehaviour=1, Animation=1; unity_score=70
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Start, Update
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/MeshLocator.cs
- 8 lines; kind=MonoBehaviour; type=MeshLocator; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=36
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/MessageSystemAdvancedButton.cs
- 31 lines; kind=MonoBehaviour; type=MessageSystemAdvancedButton; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.UI
- unity: Texture=11, UnityEngine.UI=2, MonoBehaviour=1; unity_score=99
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/METutorialScenario.cs **MUST-OPEN (>800 lines)**
- 864 lines; kind=ScenarioModule; type=METutorialScenario; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions, Expansions.Missions.Editor, Expansions.Missions.Runtime, KSP.UI, UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=1; unity_score=17
- ksp refs: Orbit=2, HighLogic=1; fan_in=14 fan_out=30
- methods(77): AddTutorialPage, At, CanSkipCurrentTutorialOnNodeDragEnd, ClearFilter, Count, CreateDialog, CreateTutorialPages, DestroyTutorialPage, DragNodeHelper, EnableAllTutorialPageButtons, EnableMeguiParameter, EnableTutorialPageButton, FilterNodes, FindButtons, GetApplyScoreScoreParam, GetDialogMessageParam, GetFacilityParam, GetLaunchSiteParam, GetMEGUINode, GetMEGUINodeIndex, GetMEGUINodes, GetMENode, GetMENodeByTemplateName, GetNextTutorialName, GetOrbitCelestialBody, GetParameter, GetPartPickerParam, GetScienceCelestialBodyParam, GetScienceExperimentParam, GetScienceIsEventNodeParam, GetScienceSituationParam, GetSelectedMEGUINode, GetSelectedMENode, GetSelectedNodeIndex, GetSelectedNodeTemplateName, GetSituationParam, GetSubParameter, GetTimeNodeOperator, GetTimeNodeStartNode, GetTimeNodeTime, GetVesselDestroyedVesselParam, GetVesselLandedBiomes, GetVesselLandedCelestialBody, GetVesselLandedLocationType, GetVesselLandedSettings ...
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/MiniBiome.cs
- 71 lines; kind=ScriptableObject; type=MiniBiome; bases=ScriptableObject; ancestry=ScriptableObject; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: ScriptableObject=1; unity_score=14
- ksp refs: Localizer=1; fan_in=6 fan_out=1
- methods(2): ConvertTagtoBiome, ConvertTagtoLandedAt
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/MiniKeyBindings.cs
- 57 lines; kind=MonoBehaviour; type=MiniKeyBindings; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, Lifecycle=2, GameObject=2, MonoBehaviour=1, Instantiate/Destroy=1, GetComponent=1; unity_score=75
- ksp refs: Localizer=2; fan_in=1 fan_out=13
- methods(5): Create, Dismiss, Start, Update, drawWindow
- auto suggestion: tier=B subsystem=Input

## Assembly-CSharp/MiniSettings.cs
- 129 lines; kind=MonoBehaviour; type=MiniSettings; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=6, GameObject=5, Lifecycle=2, Transform=1, SetActive=1, MonoBehaviour=1; unity_score=72
- ksp refs: Localizer=5, HighLogic=5, GameSettings=1, GameEvents=1; fan_in=2 fan_out=25
- methods(8): ApplySettings, Create, Dismiss, OnDifficultyOptionsDismiss, OpenDifficultyOptions, Start, Update, drawWindow
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/MinMaxColor.cs
- 15 lines; kind=class; type=MinMaxColor; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=21
- ksp refs: none; fan_in=1 fan_out=0
- methods(1): GetLerp
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/MinMaxFloat.cs
- 20 lines; kind=class; type=MinMaxFloat; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=17
- ksp refs: none; fan_in=3 fan_out=0
- methods(2): GetInverseLerp, GetLerp
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/MissionBriefingTag.cs
- 53 lines; kind=MonoBehaviour; type=MissionBriefingTag; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, Instantiate/Destroy=2, GetComponent=2, TMPro=1, MonoBehaviour=1; unity_score=74
- ksp refs: GameEvents=1; fan_in=1 fan_out=2
- methods(3): OnRemoveMe, SetupContent, Spawn
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/MissionControlBuilding.cs
- 56 lines; kind=MonoBehaviour; type=MissionControlBuilding; bases=SpaceCenterBuilding; ancestry=SpaceCenterBuilding/MonoBehaviour; ns=(global)
- usings: Contracts, KSP.Localization, UnityEngine
- unity: none; unity_score=22
- ksp refs: Localizer=6, HighLogic=5, GameEvents=4, InputLockManager=3; fan_in=0 fan_out=11
- methods(5): IsOpen, OnClicked, OnOnDestroy, OnPopupWarningDismiss, onDialogClose
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/MissionScreenTutorial.cs
- 105 lines; kind=ScenarioModule; type=MissionScreenTutorial; bases=TutorialScenario; ancestry=TutorialScenario/ScenarioModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=16
- ksp refs: GameSettings=2, ConfigNode=2; fan_in=1 fan_out=4
- methods(12): CreateMissionBriefingTutorialPage, CreateMissionScreenTutorialPage, CreateMissionScreenTutorialPages, CreateMissionTutorialPage, OnAssetSetup, OnDoneButtonClick, OnEnterCreateMissionBriefingTutorialPage, OnEnterMissionScreenTutorialPage, OnEnterMissionTutorialPage, OnLoad, OnSave, OnTutorialSetup
- auto suggestion: tier=A subsystem=Tutorials

## Assembly-CSharp/ModalGUI.cs
- 33 lines; kind=MonoBehaviour; type=ModalGUI; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: IMGUI=12, Screen/Cursor=2, MonoBehaviour=1; unity_score=98
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Window, setHotControl
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ModelAnimationFX.cs
- 106 lines; kind=MonoBehaviour; type=ModelAnimationFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=7, Animation=3, GetComponent=1, GameObject=1; unity_score=69
- ksp refs: ConfigNode=4, GameDatabase=1; fan_in=0 fan_out=5
- methods(4): OnEvent, OnInitialize, OnLoad, OnSave
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ModelMultiParticleFX.cs
- 203 lines; kind=MonoBehaviour; type=ModelMultiParticleFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=11, Transform=7, ParticleSystem=6, Instantiate/Destroy=2, GetComponent=2, SetActive=1; unity_score=74
- ksp refs: ConfigNode=4, GameDatabase=1; fan_in=0 fan_out=7
- methods(6): OnDestroy, OnEvent, OnInitialize, OnLoad, OnSave, SetEmitter
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ModelPartEvent.cs
- 21 lines; kind=MonoBehaviour; type=ModelPartEvent; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1; unity_score=50
- ksp refs: Part=1; fan_in=1 fan_out=1
- methods(2): Awake, RecieveEvent
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/ModelParticleFX.cs
- 130 lines; kind=MonoBehaviour; type=ModelParticleFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=6, ParticleSystem=2, SetActive=1, Lifecycle=1, Instantiate/Destroy=1, GetComponent=1; unity_score=66
- ksp refs: ConfigNode=4, GameDatabase=1; fan_in=0 fan_out=7
- methods(5): OnDestroy, OnEvent, OnInitialize, OnLoad, OnSave
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ModifierChangeWhen.cs
- 6 lines; kind=enum; type=ModifierChangeWhen; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=14 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Misc

## Assembly-CSharp/ModifierStagingSituation.cs
- 6 lines; kind=enum; type=ModifierStagingSituation; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=16 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/ModuleAblator.cs
- 278 lines; kind=PartModule; type=ModuleAblator; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: Lifecycle=3, Resources.=2; unity_score=30
- ksp refs: KSPField=21, Localizer=6, HighLogic=3, TimeWarp=2, PhysicsGlobals=2, PartResourceDefinition=2; fan_in=0 fan_out=13
- methods(7): FixedUpdate, GetInfo, GetModuleDisplayName, OnAwake, Start, Update, UpdateColor
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/ModuleActiveRadiator.cs
- 523 lines; kind=PartModule; type=ModuleActiveRadiator; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, KSP.Localization, Radiators
- unity: Lifecycle=3; unity_score=19
- ksp refs: KSPField=13, Part=12, Localizer=11, PhysicsGlobals=7, KSPAction=3, TimeWarp=2; fan_in=4 fan_out=23
- methods(25): Activate, ActivateAction, ApplyMaxEnergyTransferAdjustments, ApplyRadiatorActivation, CacheLocalStrings, CheckDebugFields, CheckIfDeployable, CheckPartCaches, CheckResources, FixedUpdate, GetInfo, GetModuleDisplayName, InternalCooling, IsAdjusterBlockingCooling, IsSibling, OnAwake, OnDestroy, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnVesselStandardModification, Shutdown, ShutdownAction, Start, ToggleRadiatorAction, UpdateStatus
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/ModuleAeroSurface.cs
- 234 lines; kind=PartModule; type=ModuleAeroSurface; bases=ModuleControlSurface,IMultipleDragCube; ancestry=ModuleControlSurface/ModuleLiftingSurface/PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Transform=1, Lifecycle=1; unity_score=30
- ksp refs: Localizer=6, KSPField=5, TimeWarp=4, HighLogic=2, Vessel=1, UIPartActionWindow=1; fan_in=2 fan_out=19
- methods(14): ActionToggleBrakes, CacheLocalStrings, CtrlSurfaceEditorUpdate, CtrlSurfaceUpdate, GetInfo, GetModuleDisplayName, ModifyAeroAuthorityLimiter, ModifyAeroAuthorityLimiterUI, OnAwake, OnInventoryModeDisable, OnInventoryModeEnable, OnStart, OnStoredInInventory, Start
- auto suggestion: tier=A subsystem=Aero

## Assembly-CSharp/ModuleAlternator.cs
- 163 lines; kind=PartModule; type=ModuleAlternator; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions, Expansions.Missions.Adjusters, Expansions.Serenity, KSP.Localization
- unity: Lifecycle=1; unity_score=20
- ksp refs: KSPField=9, Localizer=5, PartModule=1, HighLogic=1, ConfigNode=1; fan_in=4 fan_out=12
- methods(9): ApplyOutputAdjustments, FixedUpdate, GetInfo, GetModuleDisplayName, OnAwake, OnLoad, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStart
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ModuleAnalysisResource.cs
- 43 lines; kind=PartModule; type=ModuleAnalysisResource; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: Lifecycle=1; unity_score=31
- ksp refs: KSPField=4, PartModule=1, HighLogic=1; fan_in=3 fan_out=4
- methods(3): OnStart, SetupAnalysis, Update
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ModuleAnchoredDecoupler.cs
- 100 lines; kind=PartModule; type=ModuleAnchoredDecoupler; bases=ModuleDecouplerBase; ancestry=ModuleDecouplerBase/PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Transform=3, Coroutine=2, SetActive=1, GameObject=1; unity_score=52
- ksp refs: Part=4, Localizer=2, KSPField=2, GameEvents=1; fan_in=5 fan_out=8
- methods(6): ApplyEjectionForces, GetInfo, GetModuleDisplayName, OnActive, OnDecouple, OnStart
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ModuleAnimateGeneric.cs
- 754 lines; kind=PartModule; type=ModuleAnimateGeneric; bases=PartModule,IScalarModule,IMultipleDragCube; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Animation=4, UnityEngine.UI=2, Time.=1, Lifecycle=1, LayerMask=1; unity_score=28
- ksp refs: KSPField=31, Localizer=8, HighLogic=8, ProtoPartModuleSnapshot=1, PartModule=1, KSPEvent=1; fan_in=5 fan_out=22
- methods(27): AnimationHandler, AssumeDragCubePosition, CacheLocalStrings, CheckCrewState, CheckForRestrictions, FailAnimationForCargoBay, FindAnimation, FixedUpdate, GetAnimation, GetDragCubeNames, GetState, IsMoving, OnAwake, OnInventoryModeDisable, OnSave, OnStart, OnStoredInInventory, RestartFailedAnimationForCargoBay, SetDragState, SetScalar, SetUIRead, SetUIWrite, Toggle, ToggleAction, UpdateAnimSwitch, UsesProceduralDragCubes, VariantToggleAnimationDisabled
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/ModuleAnimateHeat.cs
- 80 lines; kind=PartModule; type=ModuleAnimateHeat; bases=ModuleAnimationSetter; ancestry=ModuleAnimationSetter/PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: Lifecycle=1; unity_score=24
- ksp refs: KSPField=5, ConfigNode=1; fan_in=0 fan_out=3
- methods(5): HeatEffectStartup, OnLoad, OnStart, Update, UpdateHeatEffect
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ModuleAnimationGroup.cs
- 736 lines; kind=PartModule; type=ModuleAnimationGroup; bases=PartModule,IMultipleDragCube; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Time.=9, Animation=9, Lifecycle=4, LayerMask=3, ParticleSystem=2, MonoBehaviour=2; unity_score=40
- ksp refs: KSPField=18, GameEvents=7, Localizer=6, HighLogic=6, KSPAction=3, KSPEvent=2; fan_in=5 fan_out=15
- methods(38): AssumeDragCubePosition, CacheLocalStrings, CheckAnimationState, CheckForActivity, DeployComplete, DeployModule, DeployModuleAction, DisableModules, EnableModules, FindAnimations, FindModules, GetCurrentPlayingAnimationState, GetDragCubeNames, LateSetup, LateUpdate, OnDestroy, OnInventoryModeDisable, OnLoad, OnPause, OnStart, OnStoredInInventory, OnUnpause, PlayDeployAnimation, RetractComplete, RetractModule, RetractModuleAction, SetDeployedState, SetDragCubes, SetRetractedState, Setup, Start, StopAnimations, ToggleActiveState, ToggleEmmitters, ToggleModuleAction, Update, UpdateAnimationTime, UsesProceduralDragCubes
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/ModuleAnimationSetter.cs
- 121 lines; kind=PartModule; type=ModuleAnimationSetter; bases=PartModule,IScalarModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Animation=5, Lifecycle=1, LayerMask=1; unity_score=54
- ksp refs: KSPField=4, FloatCurve=2, PartModule=1, Localizer=1; fan_in=2 fan_out=6
- methods(11): AnimStartup, CacheLocalStrings, GetModuleDisplayName, IsMoving, OnAwake, OnStart, SetScalar, SetUIRead, SetUIWrite, Update, UpdateAnim
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/ModuleAnimatorLandingGear.cs
- 32 lines; kind=PartModule; type=ModuleAnimatorLandingGear; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: none; unity_score=15
- ksp refs: PartModule=1, KSPField=1, KSPEvent=1; fan_in=0 fan_out=3
- methods(2): InputGearToggle, OnStart
- auto suggestion: tier=A subsystem=PartModules-Wheels

## Assembly-CSharp/ModuleAsteroid.cs
- 404 lines; kind=PartModule; type=ModuleAsteroid; bases=PartModule,IVesselAutoRename,IPartMassModifier; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Transform=4, GameObject=4, Coroutine=2, Analytics=2, Resources.=1, Lifecycle=1; unity_score=43
- ksp refs: KSPField=12, HighLogic=7, Vessel=6, Localizer=5, GameEvents=5, FlightGlobals=4; fan_in=15 fan_out=43
- methods(24): CacheLocalStrings, GetModuleMass, GetModuleMassChangeWhen, GetVesselName, GetVesselType, MakeTarget, OnAwake, OnDestroy, OnGenComplete, OnLoad, OnSave, OnStart, PostInit, Rangefinder, RenameAsteroidEvent, SetAsteroidMass, TakeSampleEVAEvent, UpdateDragCube, VesselsUndockingAsteroid, drawRenameWindow, onRenameConfirm, onRenameDismiss, performSampleExperiment, renameAsteroid
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/ModuleAsteroidAnalysis.cs
- 80 lines; kind=PartModule; type=ModuleAsteroidAnalysis; bases=ModuleSpaceObjectAnalysis; ancestry=ModuleSpaceObjectAnalysis/PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: Lifecycle=1; unity_score=24
- ksp refs: Part=3, PartModule=2, GameEvents=2, Vessel=1, HighLogic=1; fan_in=0 fan_out=9
- methods(5): CheckForPotato, ClearAsteroidResources, FindAsteroidResources, OnDestroy, OnStart
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ModuleAsteroidDrill.cs
- 286 lines; kind=PartModule; type=ModuleAsteroidDrill; bases=BaseDrill; ancestry=BaseDrill/BaseConverter/PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Raycast/Query=2, Transform=1, Physics.=1, GameObject=1; unity_score=31
- ksp refs: Localizer=15, Part=3, KSPField=3, PartResourceDefinition=1, HighLogic=1; fan_in=1 fan_out=17
- methods(10): CacheLocalStrings, CheckForImpact, GetAttachedPotato, GetInfo, GetModuleDisplayName, IsSituationValid, OnStart, OnUpdate, PostProcess, PrepareRecipe
- auto suggestion: tier=B subsystem=PartModules-Resources

## Assembly-CSharp/ModuleAsteroidInfo.cs
- 48 lines; kind=PartModule; type=ModuleAsteroidInfo; bases=ModuleSpaceObjectInfo; ancestry=ModuleSpaceObjectInfo/PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: Lifecycle=1; unity_score=29
- ksp refs: Localizer=1, HighLogic=1; fan_in=1 fan_out=5
- methods(3): OnStart, SetupAsteroidResources, Update
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/ModuleAsteroidResource.cs
- 3 lines; kind=PartModule; type=ModuleAsteroidResource; bases=ModuleSpaceObjectResource; ancestry=ModuleSpaceObjectResource/PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ModuleBiomeScanner.cs
- 57 lines; kind=PartModule; type=ModuleBiomeScanner; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: Lifecycle=1; unity_score=27
- ksp refs: Localizer=2, FlightGlobals=2, CelestialBody=2, Vessel=1, PartModule=1, KSPEvent=1; fan_in=0 fan_out=13
- methods(2): FixedUpdate, RunAnalysis
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ModuleCargoBay.cs **MUST-OPEN (>800 lines)**
- 942 lines; kind=PartModule; type=ModuleCargoBay; bases=PartModule,IAirstreamShield; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, KSP.Localization, UnityEngine
- unity: GameObject=10, Collider=8, LayerMask=5, Transform=4, Time.=4, Raycast/Query=3; unity_score=46
- ksp refs: Part=32, AttachNode=14, HighLogic=12, KSPField=11, GameEvents=8, Vessel=4; fan_in=4 fan_out=23
- methods(50): AddConnectingPart, ClearConnectingParts, ClosedAndLocked, ColliderListCleanUp, ColliderReset, CountAdjustersForcingCargoBayStuck, DisableShieldedVolume, EnableShieldedVolume, EndCapped, FindAttachNodes, FindBayColliders, FindConnectingNodes, FindEnclosedParts, FindNearbyParts, FindPartColliders, ForceCargoBayStuck, GetModuleDisplayName, GetOpposingNode, GetPart, GetVessel, ModifyShieldInEnclosedParts, NotifyShieldModified, OnAwake, OnDestroy, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnPartDestroyed, OnStart, OnStartFinished, PartWithinBounds, RemoveAdjusterForcingCargoBayStuck, RemoveConnectingPart, SelfClosedAndLocked, SetLookupCenter, SetLookupRadius, SetupDynamicCargoOccluders, ShieldEnclosedParts, TestAttachmentFit, UnShieldEnclosedParts, findModule, onCargoBayDoorsMoving, onCargoBayDoorsStopped, onCommandSeatInteraction, onEditorVesselModified, onVesselModified ...
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ModuleCargoPart.cs
- 400 lines; kind=PartModule; type=ModuleCargoPart; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions, Highlighting, KSP.Localization, UnityEngine
- unity: Coroutine=5, Audio=4, Rigidbody=3, Transform=1, Resources.=1, Lifecycle=1; unity_score=48
- ksp refs: GameEvents=16, Localizer=10, KSPField=8, Part=7, GameSettings=3, Vessel=2; fan_in=14 fan_out=20
- methods(26): EnableModule, GetInfo, GetModuleDisplayName, GetTooltip, HasLinkedCompoundParts, HideUI, IsDeployedSciencePart, MakePartSettle, OnCompoundPartDetached, OnCompoundPartLinked, OnDestroy, OnEVAConstructionMode, OnInitialize, OnLoad, OnPartPack, OnPartUnpack, OnPartWillDie, OnStart, OnUpdate, OnUpdateHighlight, OnWeldFinish, OnWeldStart, PlayDeployAnimation, SetConstructionHighlight, SetupFXGroup, ShowUI
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/ModuleColorChanger.cs
- 456 lines; kind=PartModule; type=ModuleColorChanger; bases=PartModule,IScalarModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Renderer=4, Shader=3, Material=3, Lifecycle=3, UnityEngine.UI=1; unity_score=41
- ksp refs: KSPField=24, FloatCurve=8, TimeWarp=2, PartModule=1, Localizer=1, KSPEvent=1; fan_in=3 fan_out=16
- methods(19): CacheLocalStrings, EditRenderers, FixedUpdate, GetModuleDisplayName, IsMoving, OnAwake, OnLoad, OnStart, ProcessMaterialsList, SetEventName, SetScalar, SetState, SetUIRead, SetUIWrite, Start, ToggleAction, ToggleEvent, Update, UpdateColor
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ModuleComet.cs **MUST-OPEN (>800 lines)**
- 815 lines; kind=PartModule; type=ModuleComet; bases=PartModule,IVesselAutoRename,IPartMassModifier; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: GameObject=20, ParticleSystem=16, Transform=14, Instantiate/Destroy=5, SetActive=4, GetComponent=4; unity_score=63
- ksp refs: KSPField=15, HighLogic=7, GameSettings=7, Vector3d=5, Localizer=5, GameEvents=5; fan_in=17 fan_out=52
- methods(32): ActivateGeysers, ActivateNearDust, CacheLocalStrings, CheckExplosion, CreateEmittersOnSurface, GetModuleMass, GetModuleMassChangeWhen, GetVesselName, GetVesselType, MakeTarget, OnAwake, OnDestroy, OnFixedUpdate, OnGenComplete, OnLoad, OnSave, OnStart, OnTimeWarpRateChanged, OnUpdate, PostInit, Rangefinder, RenameCometEvent, SetCometMass, SetDust, SetGeyser, TakeSampleEVAEvent, UpdateDragCube, drawRenameWindow, onRenameConfirm, onRenameDismiss, performSampleExperiment, renameComet
- auto suggestion: tier=C subsystem=Celestials

## Assembly-CSharp/ModuleCometAnalysis.cs
- 80 lines; kind=PartModule; type=ModuleCometAnalysis; bases=ModuleSpaceObjectAnalysis; ancestry=ModuleSpaceObjectAnalysis/PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: Lifecycle=1; unity_score=24
- ksp refs: Part=3, PartModule=2, GameEvents=2, Vessel=1, HighLogic=1; fan_in=0 fan_out=9
- methods(5): CheckForComet, ClearCometResources, FindCometResources, OnDestroy, OnStart
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ModuleCometDrill.cs
- 286 lines; kind=PartModule; type=ModuleCometDrill; bases=BaseDrill; ancestry=BaseDrill/BaseConverter/PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Raycast/Query=2, Transform=1, Physics.=1, GameObject=1; unity_score=31
- ksp refs: Localizer=15, Part=3, KSPField=3, PartResourceDefinition=1, HighLogic=1; fan_in=0 fan_out=17
- methods(10): CacheLocalStrings, CheckForImpact, GetAttachedPotato, GetInfo, GetModuleDisplayName, IsSituationValid, OnStart, OnUpdate, PostProcess, PrepareRecipe
- auto suggestion: tier=B subsystem=PartModules-Resources

## Assembly-CSharp/ModuleCometInfo.cs
- 48 lines; kind=PartModule; type=ModuleCometInfo; bases=ModuleSpaceObjectInfo; ancestry=ModuleSpaceObjectInfo/PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: Lifecycle=1; unity_score=29
- ksp refs: Localizer=1, HighLogic=1; fan_in=1 fan_out=5
- methods(3): OnStart, SetupCometResources, Update
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/ModuleCometResource.cs
- 3 lines; kind=PartModule; type=ModuleCometResource; bases=ModuleSpaceObjectResource; ancestry=ModuleSpaceObjectResource/PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ModuleCommand.cs
- 708 lines; kind=PartModule; type=ModuleCommand; bases=PartModule,ICommNetControlSource,IResourceConsumer; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: CommNet, Experience.Effects, KSP.Localization, UnityEngine
- unity: Transform=17, GameObject=14, Lifecycle=3, Instantiate/Destroy=3; unity_score=47
- ksp refs: Localizer=25, KSPField=13, HighLogic=8, GameSettings=6, Vessel=5, KSPEvent=4; fan_in=17 fan_out=39
- methods(29): CacheLocalStrings, ChangeControlPoint, ControlPointsExist, DestroyCPArrows, FixedUpdate, GetConsumedResources, GetControlPoint, GetControlSourceState, GetInfo, GetModuleDisplayName, HibernateToggle, IsCommCapable, MakeReference, MakeReferenceToggle, OnAwake, OnDestroy, OnLoad, OnNetworkInitialised, OnSave, OnStart, RenameVessel, SetControlPoint, Start, ToggleControlPointVisual, UpdateControlPointEvent, UpdateControlPointVisuals, UpdateControlSourceState, UpdateControlState, UpdateNetwork
- auto suggestion: tier=B subsystem=PartModules-Control

## Assembly-CSharp/ModuleConductionMultiplier.cs
- 41 lines; kind=PartModule; type=ModuleConductionMultiplier; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: Lifecycle=2; unity_score=43
- ksp refs: KSPField=2, PartModule=1, HighLogic=1; fan_in=0 fan_out=4
- methods(3): FixedUpdate, OnAwake, Start
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/ModuleControlSurface.cs **MUST-OPEN (>800 lines)**
- 994 lines; kind=PartModule; type=ModuleControlSurface; bases=ModuleLiftingSurface,IMultipleDragCube,ITorqueProvider; ancestry=ModuleLiftingSurface/PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, Expansions.Serenity, KSP.Localization, UnityEngine
- unity: Transform=14, Rigidbody=14, Lifecycle=3, GameObject=2; unity_score=47
- ksp refs: KSPField=23, KSPAction=14, Localizer=11, GameEvents=10, HighLogic=9, Part=7; fan_in=5 fan_out=36
- methods(44): ActionExtend, ActionRetract, ActionSign, ActionToggle, ActivateAllControls, ApplyActuatorSpeedAdjustments, AssumeDragCubePosition, CacheLocalStrings, CalcAngleOfAttack, CalculateCoM, CtrlSurfaceEditorUpdate, CtrlSurfaceUpdate, DeactivateAllControls, FixedCtrlSurfaceUpdate, FixedUpdate, GetDragCubeNames, GetInfo, GetModuleDisplayName, GetPotentialLift, GetPotentialTorque, LateUpdate, ModifyAuthorityLimiter, ModifyAuthorityLimiterUI, OnAwake, OnDestroy, OnEditorPartEvent, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnPartActionUIDismiss, OnPartActionUIShown, OnStart, PitchActive, PitchInactive, RollActive, RollInactive, RotatingCtrlSurfaceUpdate, TogglePitch, ToggleRoll, ToggleYaw, UsesProceduralDragCubes, YawActive, YawInactive, onVariantApplied, onVesselReferenceTransformSwitch
- auto suggestion: tier=B subsystem=Aero

## Assembly-CSharp/ModuleCoreHeat.cs
- 705 lines; kind=PartModule; type=ModuleCoreHeat; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Lifecycle=3, Time.=1, MonoBehaviour=1; unity_score=20
- ksp refs: KSPField=30, PhysicsGlobals=15, Part=13, Localizer=7, Planetarium=3, TimeWarp=2; fan_in=1 fan_out=20
- methods(24): AddEnergyToCore, AddFluxToRadiator, CheckCoreShutdown, CheckDebugFields, CheckPartCaches, CheckStartingTemperature, FixedUpdate, GetDeltaTime, GetInfo, GetLastFlux, GetModuleDisplayName, GetTransferAmount, HasActiveConverters, HasEnabledConverters, MoveCoreEnergyToRadiators, OnAwake, OnDestroy, OnLoad, OnSave, OnVesselStandardModification, ResolveConverterEnergy, ResolveNetEnergy, Start, UpdateConverterModuleCache
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/ModuleDataTransmitter.cs **MUST-OPEN (>800 lines)**
- 827 lines; kind=PartModule; type=ModuleDataTransmitter; bases=PartModule,IScienceDataTransmitter,IResourceConsumer,IContractObjectiveModule,ICommAntenna,IModuleInfo; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: CommNet, Expansions.Missions.Adjusters, KSP.Localization, UnityEngine
- unity: Coroutine=21, GameObject=1; unity_score=34
- ksp refs: Localizer=41, KSPField=13, ConfigNode=7, HighLogic=5, TimeWarp=3, ResearchAndDevelopment=3; fan_in=3 fan_out=41
- methods(43): AbortTransmission, ApplyPowerAdjustments, CanComm, CanCommUnloaded, CanScienceTo, CanSetFXModules, CanTransmit, CheckContractObjectiveValidity, CommPowerUnloaded, EvaluateScienceMultiplier, GetConsumedResources, GetContractObjectiveType, GetDrawModulePanelCallback, GetFxModuleScalar, GetInfo, GetModuleDisplayName, GetModuleTitle, GetModulesScalarMax, GetModulesScalarMin, GetPrimaryField, GetVesselSignalStrength, IsBusy, IsDataTransmitterBroken, OnAwake, OnLoad, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnSave, OnStart, ReturnDataToContainer, SetFXModules, SetFXModulesUI, StartTransmission, StartTransmissionAction, StopTransmission, TransmitData, TransmitIncompleteToggle, UpdatePowerText, WaitForFixedSeconds, findFxModules, onStartTransmission, queueVesselData, transmitQueuedData
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/ModuleDecouple.cs
- 178 lines; kind=PartModule; type=ModuleDecouple; bases=ModuleDecouplerBase; ancestry=ModuleDecouplerBase/PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Transform=6, Coroutine=4, GameObject=1; unity_score=51
- ksp refs: Part=9, Vessel=3, Localizer=3, KSPField=3, FlightGlobals=2, GameEvents=1; fan_in=9 fan_out=11
- methods(8): ApplyEjectionForces, GetInfo, GetModuleDisplayName, OnActive, OnAwake, OnDecouple, OnStart, WaitAndSwitchFocus
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ModuleDecouplerBase.cs
- 335 lines; kind=PartModule; type=ModuleDecouplerBase; bases=PartModule,IModuleInfo,IStageSeparator,IStageSeparatorChild,IAirstreamShield; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, KSP.Localization, UnityEngine
- unity: Lifecycle=2, GameObject=1; unity_score=24
- ksp refs: KSPField=9, Localizer=5, Part=3, AttachNode=3, Vessel=2, GameEvents=2; fan_in=3 fan_out=19
- methods(30): CheckShielded, ClosedAndLocked, Decouple, DecoupleAction, GetDrawModulePanelCallback, GetModuleTitle, GetPart, GetPrimaryField, GetStageIndex, GetStagingDisableText, GetStagingEnableText, GetVessel, IsAdjusterBlockingDecouple, IsEnginePlate, IsStageable, LateUpdate, OnActive, OnAwake, OnDecouple, OnDestroy, OnJettison, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStart, OnVesselWasModified, PartDetaches, SetAirstream, StagingEnabled, StagingToggleEnabledEditor, StagingToggleEnabledFlight
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/ModuleDeployableAntenna.cs
- 59 lines; kind=PartModule; type=ModuleDeployableAntenna; bases=ModuleDeployablePart; ancestry=ModuleDeployablePart/PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: none; unity_score=15
- ksp refs: Localizer=4, HighLogic=2, ProtoPartModuleSnapshot=1, ConfigNode=1; fan_in=1 fan_out=7
- methods(4): Extend, GetModuleDisplayName, OnInventoryModeDisable, OnLoad
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/ModuleDeployablePart.cs **MUST-OPEN (>800 lines)**
- 1423 lines; kind=PartModule; type=ModuleDeployablePart; bases=PartModule,IMultipleDragCube,IScalarModule,IConstruction; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, KSP.Localization, UnityEngine
- unity: Transform=25, GameObject=15, Rigidbody=11, GetComponent=5, Animation=5, Lifecycle=3; unity_score=49
- ksp refs: KSPField=39, Localizer=27, HighLogic=22, FlightGlobals=10, Planetarium=4, GameEvents=4; fan_in=13 fan_out=40
- methods(53): AllowConstructionDeployState, AssumeDragCubePosition, CacheLocalStrings, CalculateTracking, CalculateTrackingLOS, CanBeDetached, CanBeOffset, CanBeRotated, CheatRepair, ControlPanelsWithoutUsingSymmetry, DoExtend, DoRepair, DoRetract, EventRepairExternal, Extend, ExtendAction, ExtendPanelsAction, FindAnimations, FixedUpdate, GetDragCubeNames, GetInfo, GetModuleDisplayName, GetTrackingBodyTransforms, IsDeployablePartStuck, IsMoving, LateUpdate, OnAwake, OnCollisionEnter, OnDestroy, OnInventoryModeDisable, OnLoad, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnSave, OnStart, OnStoredInInventory, PostCalculateTracking, PostFSMUpdate, Retract, RetractAction, SetDragCubes, SetScalar, SetUIRead, SetUIWrite, ShouldBreakFromG ...
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/ModuleDeployableRadiator.cs
- 36 lines; kind=PartModule; type=ModuleDeployableRadiator; bases=ModuleDeployablePart; ancestry=ModuleDeployablePart/PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: none; unity_score=15
- ksp refs: Localizer=3, ProtoPartModuleSnapshot=1, ConfigNode=1; fan_in=3 fan_out=4
- methods(4): CacheLocalStrings, GetModuleDisplayName, OnInventoryModeDisable, OnLoad
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/ModuleDeployableSolarPanel.cs
- 397 lines; kind=PartModule; type=ModuleDeployableSolarPanel; bases=ModuleDeployablePart,IContractObjectiveModule; ancestry=ModuleDeployablePart/PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, KSP.Localization, UnityEngine
- unity: Transform=9, GameObject=7, LayerMask=4, Raycast/Query=3, Physics.=2, Lifecycle=2; unity_score=55
- ksp refs: KSPField=19, Localizer=6, HighLogic=6, FloatCurve=6, ScaledSpace=3, Planetarium=2; fan_in=8 fan_out=24
- methods(18): ApplyEfficiencyAdjustments, CacheLocalStrings, CalculateTrackingLOS, CheckContractObjectiveValidity, FixedUpdate, GetContractObjectiveType, GetInfo, GetModuleDisplayName, OnAwake, OnDestroy, OnInventoryModeDisable, OnLoad, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnPartRepaired, OnStart, PostCalculateTracking, PostFSMUpdate
- auto suggestion: tier=B subsystem=PartModules-Resources

## Assembly-CSharp/ModuleDisplaceTweak.cs
- 101 lines; kind=PartModule; type=ModuleDisplaceTweak; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=1, Lifecycle=1; unity_score=33
- ksp refs: KSPField=6, GameEvents=2, PartModule=1, Part=1; fan_in=0 fan_out=5
- methods(5): GetFinalPosition, OnDestroy, OnPartEvent, OnStart, UpdateMorph
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/ModuleDockingNode.cs **MUST-OPEN (>800 lines)**
- 2059 lines; kind=PartModule; type=ModuleDockingNode; bases=PartModule,ITargetable,IStageSeparator,IContractObjectiveModule,IResourceConsumer,IJointLockState; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, KSP.Localization, UnityEngine
- unity: Joint=14, Transform=10, GameObject=7, Lifecycle=4, Coroutine=4, Rigidbody=2; unity_score=35
- ksp refs: KSPField=48, GameEvents=21, FlightGlobals=17, Part=14, HighLogic=13, KSPEvent=9; fan_in=7 fan_out=46
- methods(93): ApplyCoordsUpdate, CalculateResourceDrain, CheckContractObjectiveValidity, CheckDockContact, CrossfeedToggleableEditor, CrossfeedToggleableFlight, Decouple, DecoupleAction, DemoteToPhysicslessPart, DestroySameVesselJoint, DisableXFeed, DisableXFeedAction, DisengageNodeLock, DockToSameVessel, DockToVessel, EnableXFeed, EnableXFeedAction, EngageNodeLock, FindNodeApproaches, FindOtherNode, FixedUpdate, GetActiveTargetable, GetConsumedResources, GetContractObjectiveType, GetDisplayName, GetDominantNode, GetFwdVector, GetInfo, GetModuleDisplayName, GetName, GetObtVelocity, GetOrbit, GetOrbitDriver, GetRotationAxis, GetSrfVelocity, GetStageIndex, GetStagingDisableText, GetStagingEnableText, GetTargetingMode, GetTransform, GetVessel, IsAdjusterBlockingUndock, IsJointUnlocked, IsStageable, LateUpdate ...
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ModuleDragAreaModifier.cs
- 8 lines; kind=PartModule; type=ModuleDragAreaModifier; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: none; unity_score=15
- ksp refs: KSPField=2, PartModule=1; fan_in=1 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Aero

## Assembly-CSharp/ModuleDragModifier.cs
- 8 lines; kind=PartModule; type=ModuleDragModifier; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: none; unity_score=15
- ksp refs: KSPField=2, PartModule=1; fan_in=1 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Aero

## Assembly-CSharp/ModuleDynamicNodes.cs
- 167 lines; kind=PartModule; type=ModuleDynamicNodes; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=4, Lifecycle=2, SetActive=1, GameObject=1; unity_score=43
- ksp refs: AttachNode=5, KSPField=4, ConfigNode=3, Part=2, GameEvents=2, ShipConstruct=1; fan_in=0 fan_out=12
- methods(11): ChangeNodeSet, FixedUpdate, InitializeNodeSets, OnAwake, OnDestroy, OnLoad, OnShipModified, OnStart, OnStartFinished, SetupAutoStrut, ToggleNodeSet
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ModuleEffectTest.cs
- 55 lines; kind=PartModule; type=ModuleEffectTest; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Time.=1, Lifecycle=1; unity_score=35
- ksp refs: KSPField=5, KSPEvent=2, HighLogic=2, PartModule=1; fan_in=0 fan_out=4
- methods(4): FireAction, OnStart, ToggleAction, Update
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/ModuleEngines.cs **MUST-OPEN (>800 lines)**
- 2217 lines; kind=PartModule; type=ModuleEngines; bases=PartModule,IEngineStatus,IModuleInfo,IResourceConsumer,IThrustProvider,IConstruction; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, KSP.Localization, KSP.UI.Screens, KSP.UI.TooltipTypes, UnityEngine
- unity: Transform=33, Instantiate/Destroy=14, GameObject=11, ParticleSystem=6, Light=5, GetComponent=4; unity_score=41
- ksp refs: KSPField=80, Localizer=28, FloatCurve=16, GameEvents=9, TimeWarp=8, PartResourceDefinition=6; fan_in=19 fan_out=51
- methods(82): Activate, ActivateAction, ActivatePowerFX, ActivateRunningFX, ApplyThrottleAdjustments, AutoPlaceFXGroup, BurstFlameoutGroups, CacheLocalStrings, CalculateThrust, CanBeDetached, CanBeOffset, CanBeRotated, CheckDeprived, CheckTransformsUnderwater, DeactivateLoopingFX, DeactivatePowerFX, DeactivateRunningFX, EngineExhaustDamage, FXReset, FXUpdate, FixedUpdate, Flameout, GetConsumedResources, GetCurrentThrust, GetDrawModulePanelCallback, GetEngineThrust, GetEngineType, GetInfo, GetInfoThrust, GetMaxThrust, GetModuleDisplayName, GetModuleTitle, GetPrimaryField, GetThrottlingMult, HijackFX, InitializeFX, IsEngineDead, IsStageable, MassFlow, MaxThrustOutputAtm, MaxThrustOutputVac, ModifyFlow, OnAction, OnActive, OnAwake ...
- auto suggestion: tier=B subsystem=PartModules-Engines

## Assembly-CSharp/ModuleEnginesFX.cs
- 191 lines; kind=PartModule; type=ModuleEnginesFX; bases=ModuleEngines; ancestry=ModuleEngines/PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=17
- ksp refs: KSPField=9, TimeWarp=2, PartResourceDefinition=1, Localizer=1; fan_in=4 fan_out=8
- methods(13): CacheLocalStrings, CanStart, DeactivateLoopingFX, DeactivatePowerFX, DeactivateRunningFX, FXReset, FXUpdate, InitializeFX, OnAwake, PlayEngageFX, PlayFlameoutFX, PlayShutdownFX, SetListener
- auto suggestion: tier=A subsystem=PartModules-Engines

## Assembly-CSharp/ModuleEnviroSensor.cs
- 176 lines; kind=PartModule; type=ModuleEnviroSensor; bases=PartModule,IResourceConsumer; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: UnityEngine.UI=2, Transform=1, Lifecycle=1; unity_score=34
- ksp refs: Localizer=9, PartResourceDefinition=3, KSPField=3, TimeWarp=1, PartModule=1, KSPEvent=1; fan_in=1 fan_out=16
- methods(9): CacheLocalStrings, FixedUpdate, GetConsumedResources, GetInfo, GetModuleDisplayName, OnAwake, OnLoad, Toggle, ToggleAction
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ModuleEvaChute.cs
- 319 lines; kind=PartModule; type=ModuleEvaChute; bases=ModuleParachute; ancestry=ModuleParachute/PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=21, Renderer=7, Shader=6, GetComponent=5, Material=3, Rigidbody=2; unity_score=76
- ksp refs: KSPField=12, KerbalEVA=2, ProtoCrewMember=1, Part=1, KSPEvent=1; fan_in=4 fan_out=10
- methods(19): AllowRepack, CanCrewMemberUseParachute, CutParachute, FixedUpdate, GetFullyDeployedCanopyRotation, GetSemiDeployedCanopyRotation, IsStageable, OnActive, OnDestroy, OnParachuteFullyDeployed, OnParachuteSemiDeployed, OnStart, PassedAdditionalDeploymentChecks, SetCanopy, SetEVAChuteActive, ShowEVAChuteParamsChanged, ToggleEVAChuteParams, UpdateFullyDeployedParachuteMovement, UpdateSemiDeployedParachuteMovement
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/ModuleExperienceManagement.cs
- 73 lines; kind=PartModule; type=ModuleExperienceManagement; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: CommNet, KSP.Localization, UnityEngine
- unity: none; unity_score=20
- ksp refs: Localizer=8, HighLogic=3, ProtoCrewMember=2, Funding=2, PartModule=1, KerbalRoster=1; fan_in=0 fan_out=15
- methods(4): FinalizeExperience, GetInfo, GetModuleDisplayName, OnStart
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/ModuleFlightDisplay.cs
- 210 lines; kind=PartModule; type=ModuleFlightDisplay; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: IMGUI=40; unity_score=85
- ksp refs: FlightGlobals=3, HighLogic=2, Vessel=1, PartModule=1, Localizer=1, KSPEvent=1; fan_in=0 fan_out=7
- methods(13): GetActiveGUI, GetControlState, GetFirstHop, GetLastHop, GetSignalStrength, GetTotalHops, InitStyles, OnDraw, OnStart, OnWindow, Setup, ToggleGui, TurnOffGui
- auto suggestion: tier=C subsystem=PartModules-Control

## Assembly-CSharp/ModuleFuelJettison.cs
- 49 lines; kind=PartModule; type=ModuleFuelJettison; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: Resources.=1; unity_score=35
- ksp refs: Localizer=3, PartResource=2, PartModule=1, KSPField=1, KSPEvent=1, KSPAction=1; fan_in=0 fan_out=9
- methods(4): Dump, FuelJettison, FuelJettisonAction, OnStart
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/ModuleGenerator.cs
- 269 lines; kind=PartModule; type=ModuleGenerator; bases=PartModule,IContractObjectiveModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, KSP.Localization
- unity: UnityEngine.UI=1, Lifecycle=1; unity_score=22
- ksp refs: KSPField=15, Localizer=7, KSPAction=3, KSPEvent=2, PartModule=1; fan_in=4 fan_out=12
- methods(15): Activate, ActivateAction, ApplyEfficiencyAdjustments, CacheLocalStrings, CheckContractObjectiveValidity, FixedUpdate, GetContractObjectiveType, GetInfo, GetModuleDisplayName, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStart, Shutdown, ShutdownAction, ToggleAction
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ModuleGimbal.cs
- 466 lines; kind=PartModule; type=ModuleGimbal; bases=PartModule,ITorqueProvider; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, KSP.Localization, UnityEngine
- unity: Transform=18, Lifecycle=1; unity_score=47
- ksp refs: KSPField=21, Localizer=9, KSPAction=6, TimeWarp=1, PartModule=1, Part=1; fan_in=3 fan_out=19
- methods(24): ApplyControlAdjustments, CacheLocalStrings, CreateEngineList, FixedUpdate, FreeAction, GetInfo, GetModuleDisplayName, GetPotentialTorque, GimbalRotation, IsStageable, LockAction, OnActive, OnLoad, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStart, SetActionsSuffix, ToggleAction, TogglePitchAction, ToggleRollAction, ToggleToggles, ToggleYawAction, UpdatePAWUI, UpdateToggles
- auto suggestion: tier=B subsystem=PartModules-Engines

## Assembly-CSharp/ModuleGPS.cs
- 176 lines; kind=PartModule; type=ModuleGPS; bases=PartModule,IAnimatedModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Time.=1, MonoBehaviour=1, Lifecycle=1; unity_score=25
- ksp refs: Localizer=10, KSPField=4, FlightGlobals=2, PartModule=1, HighLogic=1, CelestialBody=1; fan_in=2 fan_out=12
- methods(9): CacheLocalStrings, CheckForAsteroidConnect, CheckForCometConnect, DisableModule, EnableModule, GetModuleDisplayName, IsSituationValid, ModuleIsActive, Update
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ModuleGrappleNode.cs **MUST-OPEN (>800 lines)**
- 996 lines; kind=PartModule; type=ModuleGrappleNode; bases=PartModule,IActiveJointHost,IJointLockState,IContractObjectiveModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, KSP.Localization, UnityEngine
- unity: Analytics=12, Transform=10, GameObject=9, Coroutine=5, Raycast/Query=3, Lifecycle=3; unity_score=48
- ksp refs: Part=20, FlightGlobals=12, KSPField=10, HighLogic=9, KSPEvent=6, Localizer=4; fan_in=4 fan_out=39
- methods(43): CheckContractObjectiveValidity, CheckGrappleContact, CountAdjustersBlockingGrappleRelease, Decouple, DecoupleAction, DestroySameVesselJoint, FindContactParts, FixedUpdate, GetContractObjectiveType, GetHostPart, GetInfo, GetLocalTransform, GetModuleDisplayName, Grapple, GrappleSameVessel, IsAdjusterBlockingGrappleGrab, IsJointUnlocked, IsLoose, LateUpdate, LockPivot, MakeReferenceToggle, MakeReferenceTransform, NodeIsTooFar, OnDriveModeChanged, OnJointInit, OnLoad, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnOtherNodeSameVesselDisconnect, OnSave, OnStart, OtherPartIsLost, Release, ReleaseAction, ReleaseSameVessel, RemoveAdjusterBlockingGrappleNodeAbilityToRelease, SetLoose, SetUpAdjusterBlockingGrappleNodeAbilityToRelease, SetupFSM, Update, WaitAndSwitchFocus, lateFSMStart, updateGrappleTransform
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ModuleGroundCommsPart.cs
- 137 lines; kind=PartModule; type=ModuleGroundCommsPart; bases=ModuleGroundSciencePart,ICommAntenna; ancestry=ModuleGroundSciencePart/ModuleGroundPart/ModuleCargoPart/PartModule; ns=(global)
- usings: CommNet, KSP.Localization
- unity: none; unity_score=15
- ksp refs: Localizer=10, KSPField=6, ConfigNode=3, ProtoPartModuleSnapshot=2, Part=1, FlightGlobals=1; fan_in=2 fan_out=15
- methods(6): CanComm, CanCommUnloaded, CanScienceTo, CommPowerUnloaded, GetInfo, GetModuleDisplayName
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/ModuleGroundExpControl.cs
- 404 lines; kind=PartModule; type=ModuleGroundExpControl; bases=ModuleGroundCommsPart,ICommNetControlSource; ancestry=ModuleGroundCommsPart/ModuleGroundSciencePart/ModuleGroundPart/ModuleCargoPart; ns=(global)
- usings: CommNet, Expansions, Expansions.Serenity.DeployedScience.Runtime, KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Transform=4, Coroutine=4, GetComponent=3, Lifecycle=2, Instantiate/Destroy=1; unity_score=39
- ksp refs: GameEvents=19, Localizer=17, KSPField=6, Part=3, HighLogic=3, Vessel=1; fan_in=8 fan_out=27
- methods(26): CacheLocalStrings, CommPowerUnloaded, GetControlSourceState, GetInfo, GetModuleDisplayName, GetTooltip, IsCommCapable, OnAwake, OnDestroy, OnGroundScienceClusterPowerStateChanged, OnGroundScienceClusterUpdated, OnGroundScienceModuleRemoved, OnGroundSciencePartDeployed, OnGroundSciencePartEnabledStateChanged, OnPartActionUIDismiss, OnPartActionUIOpened, OnSave, OnStart, OnUpdate, RecalcBoosterPower, ScanAndAddControlUnit, SetDeployedSciencePartVesselType, SetVesselNaming, Start, UpdateModuleUI, UpdateNetwork
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/ModuleGroundExperiment.cs
- 212 lines; kind=PartModule; type=ModuleGroundExperiment; bases=ModuleGroundSciencePart; ancestry=ModuleGroundSciencePart/ModuleGroundPart/ModuleCargoPart/PartModule; ns=(global)
- usings: Expansions, Expansions.Serenity.DeployedScience.Runtime, KSP.Localization, UnityEngine
- unity: Lifecycle=2, Instantiate/Destroy=1; unity_score=30
- ksp refs: KSPField=7, Localizer=6, GameEvents=6, ResearchAndDevelopment=2, FloatCurve=2, HighLogic=1; fan_in=9 fan_out=17
- methods(11): CacheLocalStrings, GetInfo, GetModuleDisplayName, OnDestroy, OnGroundScienceGenerated, OnGroundScienceTransmitted, OnLoad, OnStart, RetrievePart, Start, UpdateModuleUI
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/ModuleGroundPart.cs
- 479 lines; kind=PartModule; type=ModuleGroundPart; bases=ModuleCargoPart,IAnimatedModule; ancestry=ModuleCargoPart/PartModule/MonoBehaviour; ns=(global)
- usings: Expansions, Expansions.Serenity.DeployedScience.Runtime, KSP.Localization, UnityEngine
- unity: Rigidbody=9, Coroutine=5, Instantiate/Destroy=3, Collision/Force=3, Joint=2, SendMessage/Invoke=1; unity_score=51
- ksp refs: GameEvents=16, FlightGlobals=10, KSPField=6, HighLogic=5, Vessel=4, Part=4; fan_in=7 fan_out=27
- methods(26): CacheLocalStrings, CanBeStored, DisableModule, EnableModule, GetModuleDisplayName, HasChildParts, IsSituationValid, MakePartKinematic, ModuleIsActive, OnDestroy, OnLeavingScene, OnPartActionUIDismiss, OnPartActionUIShown, OnPartPack, OnPartUnpack, OnPartWillDie, OnRetractCompleted, OnStart, OnUpdate, OnVesselChange, PlayDeployAnimation, PlayRetractAnimation, RetrievePart, RetrieveScienceData, SetDeployedOnGroundFromCoRoutine, UpdateModuleUI
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/ModuleGroundSciencePart.cs
- 659 lines; kind=PartModule; type=ModuleGroundSciencePart; bases=ModuleGroundPart; ancestry=ModuleGroundPart/ModuleCargoPart/PartModule/MonoBehaviour; ns=(global)
- usings: Expansions, Expansions.Serenity.DeployedScience.Runtime, Experience, Experience.Effects, KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Transform=16, GameObject=7, LayerMask=4, Raycast/Query=3, Lifecycle=3, Physics.=2; unity_score=51
- ksp refs: Localizer=23, GameEvents=22, KSPField=17, TimeWarp=3, Part=3, Vessel=2; fan_in=14 fan_out=34
- methods(23): CacheLocalStrings, CalculateTrackingLOS, FixedUpdate, GetInfo, GetModuleDisplayName, GetTooltip, OnAwake, OnDestroy, OnGroundScienceClusterPowerStateChanged, OnGroundScienceClusterUpdated, OnGroundScienceDeregisterCluster, OnLoad, OnPartActionUIDismiss, OnPartActionUIOpened, OnStart, OnUpdate, RetrievePart, SetDeployedOnGround, Start, SunTrackerToOrigin, ToggleActive, ToggleActiveAction, UpdateModuleUI
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/ModuleHeatEffects.cs
- 69 lines; kind=PartModule; type=ModuleHeatEffects; bases=PartModuleFXSetter; ancestry=PartModuleFXSetter/PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: Lifecycle=1; unity_score=25
- ksp refs: KSPField=5; fan_in=0 fan_out=2
- methods(4): HeatEffectStartup, OnStart, Update, UpdateHeatEffect
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ModuleInventoryPart.cs **MUST-OPEN (>800 lines)**
- 2002 lines; kind=PartModule; type=ModuleInventoryPart; bases=PartModule,IPartMassModifier,IPartCostModifier; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Experience.Effects, Highlighting, KSP.Localization, UnityEngine
- unity: Transform=31, GameObject=10, GetComponent=8, Time.=6, Renderer=6, Coroutine=6; unity_score=47
- ksp refs: Part=22, GameEvents=22, ConfigNode=22, HighLogic=19, GameSettings=15, PartLoader=9; fan_in=26 fan_out=51
- methods(72): CanStackInSlot, CancelPartPlacementMode, CheckPartStorage, ClearPartAtSlot, ContainsPart, CreatePartForInventoryUse, CreatePartNode, CreatePartObject, DeletePartObject, DeployGroundPart, DeployInventoryItem, FirstEmptySlot, FirstFullSlot, GetInfo, GetModuleCost, GetModuleCostChangeWhen, GetModuleDisplayName, GetModuleMass, GetModuleMassChangeWhen, GetPartMass, GetProtoVesselNode, GetStackAmountAtSlot, GetStackCapacityAtSlot, GetVariantNameForSlot, HasCapacity, HasStackingSpace, IgnoreKerbalInventoryLimits, InitializeInventory, InitializeMassVolumeDisplay, InventoryItemCanBeDeployed, IsSlotEmpty, IsStackable, KerbalStateChanged, OnCopy, OnDestroy, OnDrawGizmosSelected, OnEditorPartEvent, OnLoad, OnModuleInventoryChanged, OnPartPurchased, OnSave, OnStart, OnUpdate, PartDroppedOnInventory, PreviewLimits ...
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/ModuleJettison.cs
- 667 lines; kind=PartModule; type=ModuleJettison; bases=PartModule,IActivateOnDecouple,IScalarModule,IMultipleDragCube,IPartMassModifier; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Collider=21, Transform=17, GameObject=14, Rigidbody=12, SetActive=9, GetComponent=6; unity_score=71
- ksp refs: KSPField=22, AttachNode=5, PhysicsGlobals=4, HighLogic=4, Localizer=2, GameEvents=2; fan_in=5 fan_out=25
- methods(31): AssumeDragCubePosition, CheckAttachNode, DecoupleAction, FetchActiveMeshTransform, GetDragCubeNames, GetModuleMass, GetModuleMassChangeWhen, GetStagingDisableText, GetStagingEnableText, IsMoving, IsStageable, Jettison, JettisonAction, LateUpdate, LoadJettisonMeshState, OnActive, OnDestroy, OnInventoryModeDisable, OnLoad, OnStart, SetActionsSuffix, SetFairingDragCube, SetIgnoreCollisionFlags, SetScalar, SetUIRead, SetUIWrite, SetVariants, SetupJettisonState, Update, UsesProceduralDragCubes, onVariantApplied
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ModuleJointMotor.cs
- 267 lines; kind=PartModule; type=ModuleJointMotor; bases=PartModule,IJointLockState; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Joint=3, Coroutine=2, Transform=1, GameObject=1; unity_score=42
- ksp refs: ConfigNode=4, PartJoint=3, Part=2, GameEvents=2, AttachNode=2, PartModule=1; fan_in=1 fan_out=11
- methods(23): GetMotorForce, GetMotorMode, GetMotorSpeed, InitJoint, IsJointUnlocked, OnJointInit, OnLoad, OnModuleLoad, OnModuleSave, OnModuleStart, OnMotorModeChanged, OnPartPack, OnSave, OnStart, SetMotorForce, SetMotorMode, SetMotorSpeed, applyCoordsUpdate, findJointAtNode, getControlOrt, onPartJointBreak, recurseCoordUpdate, startAfterJointsCreated
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/ModuleJointMotorTest.cs
- 171 lines; kind=PartModule; type=ModuleJointMotorTest; bases=PartModule,IActiveJointHost,IJointLockState; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=2, Transform=1, Lifecycle=1; unity_score=40
- ksp refs: KSPEvent=5, KSPField=3, PartModule=1, Part=1; fan_in=0 fan_out=8
- methods(16): GetHostPart, GetLocalTransform, GetMotorForce, GetMotorSpeed, IsJointUnlocked, LateUpdate, LinkToThrottle, MotorDrive, MotorNeutral, MotorPark, OnDriveModeChanged, OnJointInit, OnStart, Reverse, SetMotorForce, SetMotorSpeed
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/ModuleJointPivot.cs
- 18 lines; kind=PartModule; type=ModuleJointPivot; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: Lifecycle=1; unity_score=36
- ksp refs: ConfigNode=2, PartModule=1; fan_in=0 fan_out=2
- methods(4): OnLoad, OnSave, OnStart, Update
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/ModuleKerbNetAccess.cs
- 470 lines; kind=PartModule; type=ModuleKerbNetAccess; bases=PartModule,IAccessKerbNet,IModuleInfo; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: FinePrint.Utilities, KSP.Localization, KSP.UI.Dialogs, UnityEngine
- unity: SendMessage/Invoke=3, Lifecycle=1; unity_score=22
- ksp refs: Vessel=17, Localizer=13, KSPField=7, GameEvents=7, ConfigNode=5, GameDatabase=3; fan_in=3 fan_out=19
- methods(30): CacheLocalStrings, CheckRequiredEffects, CheckVesselSituationFoV, GetDrawModulePanelCallback, GetEnhancedSituations, GetInfo, GetKerbNetAnomalyChance, GetKerbNetDisplayModes, GetKerbNetErrorState, GetKerbNetMaximumFoV, GetKerbNetMinimumFoV, GetKerbNetPart, GetKerbNetVessel, GetModuleDisplayName, GetModuleTitle, GetPrimaryField, HasThisDialogOpen, InvokeCrewEffectCheck, LoadDisplayModes, LoadRequiredEffects, OnAnimationGroupStateChanged, OnAwake, OnDestroy, OnLoad, OnStart, OnVesselSituationChanged, OpenKerbNet, OpenKerbNetAction, PartHasRequiredEffects, SituationIsEnhanced
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/ModuleLiftingSurface.cs
- 426 lines; kind=PartModule; type=ModuleLiftingSurface; bases=PartModule,ILiftProvider; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, KSP.Localization, UnityEngine
- unity: Transform=8, Instantiate/Destroy=4, GameObject=4, Rigidbody=2, Lifecycle=1; unity_score=48
- ksp refs: KSPField=19, PhysicsGlobals=10, FloatCurve=8, Localizer=4, FlightGlobals=4, HighLogic=2; fan_in=6 fan_out=15
- methods(15): ApplyLiftForceAdjustments, CacheLocalStrings, DestroyLiftAndDragArrows, FixedUpdate, GetDragVector, GetInfo, GetLiftVector, GetModuleDisplayName, OnAwake, OnCenterOfLiftQuery, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStart, SetupCoefficients, UpdateAeroDisplay
- auto suggestion: tier=B subsystem=Aero

## Assembly-CSharp/ModuleLight.cs **MUST-OPEN (>800 lines)**
- 1269 lines; kind=PartModule; type=ModuleLight; bases=PartModule,IResourceConsumer,IScalarModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, KSP.Localization, UnityEngine
- unity: Shader=26, Material=24, Transform=11, Renderer=10, Light=10, Animation=5; unity_score=62
- ksp refs: KSPField=34, Localizer=8, KSPEvent=6, HighLogic=6, GameEvents=6, PartResourceDefinition=4; fan_in=6 fan_out=34
- methods(47): ApplyIntensityAdjustments, Blink, BlinkLightOff, BlinkLightOn, CacheLocalStrings, FixedUpdate, GetAngleVector, GetAnims, GetConsumedResources, GetCurrentColor, GetInfo, GetModuleDisplayName, IsMoving, LightOffAction, LightOnAction, LightVarsModified, LightsOff, LightsOn, ModifyCurrentBlinkTime, ModifyLightVector, OnAwake, OnColorChanged, OnDestroy, OnInventoryModeDisable, OnLoad, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnPAWNotShown, OnPAWShown, OnStart, PresetColors, ProcessLightEditorScene, SetBlinkRateLimits, SetBlinkState, SetEmission, SetFlareColor, SetLightState, SetLimits, SetPitchLimits, SetRotationLimits, SetScalar, SetUIRead, SetUIWrite, ToggleBlink, ToggleLightAction ...
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ModuleMirrorPlacement.cs
- 46 lines; kind=PartModule; type=ModuleMirrorPlacement; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=28
- ksp refs: KSPField=7, AttachNode=2, PartModule=1, Part=1; fan_in=0 fan_out=4
- methods(1): OnWillBeMirrored
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/ModuleOrbitalScanner.cs
- 299 lines; kind=PartModule; type=ModuleOrbitalScanner; bases=PartModule,IAnimatedModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: Lifecycle=3, Shader=1; unity_score=26
- ksp refs: Localizer=9, KSPEvent=6, HighLogic=3, GameEvents=2, PartModule=1, KSPField=1; fan_in=1 fan_out=17
- methods(24): ActivateScanner, CacheLocalStrings, DecreaseCutoff, DisableModule, EnableModule, FixedUpdate, GenerateOverlay, GetOverlayColor, HideOverlay, IncreaseCutoff, IsSituationValid, ModuleIsActive, OnActive, OnDestroy, OnResourceMapLoaded, SetColor, SetupMap, ShowResourceName, Start, SwitchResource, ToggleMenu, ToggleOverlay, ToggleSurvey, UpdateGUI
- auto suggestion: tier=B subsystem=PartModules-Resources

## Assembly-CSharp/ModuleOrbitalSurveyor.cs
- 165 lines; kind=PartModule; type=ModuleOrbitalSurveyor; bases=PartModule,IAnimatedModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: Lifecycle=2; unity_score=24
- ksp refs: Localizer=13, KSPField=5, ResearchAndDevelopment=3, GameEvents=3, Vessel=2, HighLogic=2; fan_in=1 fan_out=22
- methods(14): CheckForScanData, CompleteSurvey, DisableModule, EnableModule, GetInfo, GetModuleDisplayName, IsSituationValid, ModuleIsActive, OnDestroy, OnStart, PerformSurvey, Update, rescaleThresholds, sendDataToComms
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ModuleOverheatDisplay.cs
- 105 lines; kind=PartModule; type=ModuleOverheatDisplay; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=19
- ksp refs: Localizer=2, KSPField=2, PartModule=1; fan_in=1 fan_out=5
- methods(3): CacheLocalStrings, OnStart, OnUpdate
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/ModuleParachute.cs **MUST-OPEN (>800 lines)**
- 989 lines; kind=PartModule; type=ModuleParachute; bases=PartModule,IMultipleDragCube; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: GameObject=12, SetActive=10, Transform=9, Animation=4, Rigidbody=2, Lifecycle=2; unity_score=45
- ksp refs: KSPField=34, Localizer=16, PhysicsGlobals=14, Vessel=5, KSPEvent=4, TimeWarp=3; fan_in=13 fan_out=33
- methods(39): AssumeDragCubePosition, CacheLocalStrings, CalcBaseStats, CutAction, CutParachute, Deploy, DeployAction, DeploySafe, Disarm, FixedUpdate, GetAnimationNormalizedTime, GetArea, GetDragCubeNames, GetInfo, GetModuleDisplayName, IsAnimationPlaying, IsMovingFastEnoughToDeploy, OnActive, OnAwake, OnDestroy, OnLoad, OnParachuteFullyDeployed, OnParachuteSemiDeployed, OnPartUnpack, OnStart, OnVesselWasModified, PassedAdditionalDeploymentChecks, PlayDeployAnimation, Repack, SetConvectiveStats, SetDragCube_Deployed, SetDragCube_SemiDeployed, SetDragCubes_Stowed, SetUIEVents, SetupAnimation, ShouldDeploy, UpdateCut, UpdateSymmetry, UsesProceduralDragCubes
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ModulePartFirework.cs
- 778 lines; kind=PartModule; type=ModulePartFirework; bases=PartModule,IPartMassModifier; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.FX.Fireworks, KSP.Localization, UnityEngine
- unity: GameObject=12, Shader=11, Rigidbody=10, Transform=8, GetComponent=4, Coroutine=4; unity_score=60
- ksp refs: Localizer=17, KSPField=17, HighLogic=7, GameEvents=4, UIPartActionWindow=3, FlightGlobals=3; fan_in=0 fan_out=35
- methods(29): GetCurrentColor, GetModuleMass, GetModuleMassChangeWhen, LaunchShell, LaunchShellAction, LaunchShellEvent, OnAwake, OnColorChanged, OnDestroy, OnLoad, OnPAWInit, OnPAWShown, OnSave, OnStart, PresetColors, UpdateFireworksColors, configureColorFields, configureSoundFX, fetchColorPickerFromID, getCurrentFXByType, getCurrentPSByType, getFXFromPrefab, matchColorPickers, pickerID2FieldName, setAllColors, trailTypeModified, updateFireworkShotLabel, updatePickerColor, waitAndRefreshColors
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/ModulePartVariants.cs
- 562 lines; kind=PartModule; type=ModulePartVariants; bases=PartModule,IModuleInfo,IMultipleDragCube,IPartMassModifier,IPartCostModifier; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Transform=13, Material=9, Renderer=2, Shader=1, GetComponent=1, GameObject=1; unity_score=49
- ksp refs: EditorLogic=6, Localizer=5, HighLogic=5, Part=4, KSPField=4, ConfigNode=4; fan_in=11 fan_out=24
- methods(33): ApplyVariant, AssumeDragCubePosition, GetCurrentVariantIndex, GetDragCubeNames, GetDrawModulePanelCallback, GetInfo, GetModuleCost, GetModuleCostChangeWhen, GetModuleDisplayName, GetModuleMass, GetModuleMassChangeWhen, GetModuleTitle, GetPrimaryField, GetTitle, GetVariantIndex, GetVariantNames, GetVariantThemeIndex, GetVariantThemeNames, HasVariant, HasVariantTheme, OnAwake, OnLoad, OnSave, OnStart, RefreshVariant, SetCubes, SetVariant, SetVariantTheme, UpdateNode, UpdatePartPosition, UsesProceduralDragCubes, onVariantChanged, onVariantSymmetrycallyChanged
- auto suggestion: tier=C subsystem=Part

## Assembly-CSharp/ModulePhysicMaterial.cs
- 264 lines; kind=PartModule; type=ModulePhysicMaterial; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Material=5, PhysicMaterial=4, Renderer=1, Lifecycle=1, GetComponent=1, GameObject=1; unity_score=56
- ksp refs: ConfigNode=8, KSPField=6, HighLogic=4, GameEvents=3, Localizer=2, PartModule=1; fan_in=0 fan_out=13
- methods(11): ApplyListMaterial, ChangePhysicMaterial, OnAwake, OnDestroy, OnLoad, OnSave, OnStart, OnVariantApplied, SetPhysicMaterial, SetPhysicMaterialColors, UpdatePhysicMaterialEvent
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/ModuleProceduralFairing.cs **MUST-OPEN (>800 lines)**
- 2220 lines; kind=PartModule; type=ModuleProceduralFairing; bases=PartModule,IPartMassModifier,IPartCostModifier,IScalarModule,IModuleInfo,IDynamicCargoOccluder,IActivateOnDecouple; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, ProceduralFairings, UnityEngine
- unity: Transform=33, Rigidbody=33, Material=21, Shader=18, SetActive=13, Texture=12; unity_score=65
- ksp refs: KSPField=40, Part=33, GameEvents=25, HighLogic=22, EditorLogic=22, Localizer=15; fan_in=10 fan_out=62
- methods(93): AddFlagToFairing, AddPanelColliders, ApplyVariantToPanel, AssumeClosedDragCube, AssumeDeployedDragCube, BuildFairing, CacheLocalStrings, CheckForFairingEdit, ClampH, ClampR, DecoupleAction, DeleteFairing, DeployFairing, DeployFairingAction, DumpInterstage, EditFairing, FindPayloadFromTop, FixedUpdate, GetDrawModulePanelCallback, GetFairingArea, GetInfo, GetModuleCost, GetModuleCostChangeWhen, GetModuleDisplayName, GetModuleMass, GetModuleMassChangeWhen, GetModuleSize, GetModuleSizeChangeWhen, GetModuleTitle, GetMouseHeight, GetMouseLatOffset, GetMouseXPlane, GetPrimaryField, GetTotalCost, GetTotalMass, GizmoDisplay, IsMoving, JettisonPanels, LateUpdate, LockEditor, MakePanelPhysical, MeshPreviewUpdate, MouseFadeUpdate, OnActive, OnAwake ...
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ModuleRCS.cs **MUST-OPEN (>800 lines)**
- 978 lines; kind=PartModule; type=ModuleRCS; bases=PartModule,IResourceConsumer,ITorqueProvider,IConstruction; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Transform=11, GameObject=9, Lifecycle=3, SetActive=1, Resources.=1, ParticleSystem=1; unity_score=39
- ksp refs: KSPField=34, Localizer=10, ConfigNode=6, TimeWarp=4, PartResourceDefinition=4, FloatCurve=4; fan_in=5 fan_out=32
- methods(41): ApplyInputLinearAdjustments, ApplyInputRotationAdjustments, CacheLocalStrings, CalculateThrust, CanBeDetached, CanBeOffset, CanBeRotated, DeactivateFX, DeactivatePowerFX, FindThrusters, FixedUpdate, GetConsumedResources, GetInfo, GetLeverDistance, GetLeverDistanceOriginal, GetModuleDisplayName, GetPotentialTorque, GetStagingDisableText, GetStagingEnableText, IsAdjusterBreakingRCS, IsStageable, OnActive, OnAwake, OnDestroy, OnLoad, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStageIconDestroy, OnStart, RequestPropellant, SetResource, SetupFX, SetupPropellant, ToggleAction, ToggleToggles, Update, UpdatePowerFX, UpdatePropellantGauge, UpdatePropellantStatus, UpdateToggles, getMaxFuelFlow
- auto suggestion: tier=B subsystem=PartModules-Engines

## Assembly-CSharp/ModuleRCSFX.cs
- 45 lines; kind=PartModule; type=ModuleRCSFX; bases=ModuleRCS; ancestry=ModuleRCS/PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: none; unity_score=15
- ksp refs: Localizer=1, KSPField=1; fan_in=1 fan_out=3
- methods(7): CacheLocalStrings, DeactivateFX, DeactivatePowerFX, GetModuleDisplayName, OnAwake, SetupFX, UpdatePowerFX
- auto suggestion: tier=A subsystem=PartModules-Engines

## Assembly-CSharp/ModuleReactionWheel.cs
- 349 lines; kind=PartModule; type=ModuleReactionWheel; bases=PartModule,IResourceConsumer,ITorqueProvider; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, KSP.Localization, UnityEngine
- unity: UnityEngine.UI=1, Lifecycle=1; unity_score=25
- ksp refs: Localizer=8, KSPField=8, KSPAction=4, PartResourceDefinition=3, ConfigNode=2, TimeWarp=1; fan_in=3 fan_out=22
- methods(21): Activate, ActiveUpdate, ApplyTorqueAdjustments, CacheLocalStrings, CycleAction, Deactivate, FixedUpdate, GetAppliedTorque, GetConsumedResources, GetInfo, GetModuleDisplayName, GetPotentialTorque, IsAdjusterBreakingReactionWheel, OnAwake, OnLoad, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnSave, OnStart, OnToggle, Toggle
- auto suggestion: tier=A subsystem=PartModules-Control

## Assembly-CSharp/ModuleRemoteController.cs
- 18 lines; kind=PartModule; type=ModuleRemoteController; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: UnityEngine.UI=1; unity_score=44
- ksp refs: PartModule=1, KSPField=1, KSPEvent=1; fan_in=0 fan_out=3
- methods(1): Toggle
- auto suggestion: tier=A subsystem=PartModules-Control

## Assembly-CSharp/ModuleResource.cs
- 147 lines; kind=class; type=ModuleResource; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: KSP.Localization
- unity: none; unity_score=3
- ksp refs: Localizer=4, PartResourceDefinition=3, ConfigNode=2; fan_in=15 fan_out=8
- methods(5): IsDeprived, Load, OnAwake, PrintRate, Save
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ModuleResourceAutoShiftState.cs
- 240 lines; kind=PartModule; type=ModuleResourceAutoShiftState; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Resources.=1, Lifecycle=1, Instantiate/Destroy=1; unity_score=25
- ksp refs: Localizer=11, KSPField=10, HighLogic=3, PartResourceDefinition=1, PartResource=1, PartModule=1; fan_in=1 fan_out=13
- methods(8): ModifyAmountPercent, ModifyShutOffAmount, ModifyShutOffPercent, ModifyStartUpAmount, ModifyStartUpPercent, OnDestroy, OnStart, ToggleWarningVisibility
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ModuleResourceConverter.cs
- 127 lines; kind=PartModule; type=ModuleResourceConverter; bases=BaseConverter; ancestry=BaseConverter/PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: MonoBehaviour=1; unity_score=21
- ksp refs: Localizer=13, PartResourceDefinition=3, KSPField=1; fan_in=0 fan_out=9
- methods(5): GetInfo, GetModuleDisplayName, IsSituationValid, LoadRecipe, PrepareRecipe
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ModuleResourceDrain.cs
- 724 lines; kind=PartModule; type=ModuleResourceDrain; bases=PartModule,IConstruction; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Resources.=4, Lifecycle=2, Transform=1, Time.=1; unity_score=25
- ksp refs: PartResource=6, Localizer=6, GameEvents=6, Part=5, KSPField=5, KSPAction=4; fan_in=1 fan_out=24
- methods(45): CacheLocalStrings, CanBeDetached, CanBeOffset, CanBeRotated, CheckDrainingForEffects, DrainPart, DrainResourceNotFinished, FixedUpdate, ForceUpdateResourcePAW, GetPartResource, GetResourceDrainStatus, HasEnoughDrainingResources, Initialize, IsResourceDraining, LoadResources, LoadResourcesAvailable, OnAwake, OnDestroy, OnLoad, OnPartMenuOpen, OnPartResourceListChanged, OnSave, OnStart, OnStartFinished, PartContainsResource, PartPlaced, ResourcesToDrain, ResourcesToDrainHaveFX, SelectFXGroupByName, StartResourceDrainAction, StopResourceDrainAction, ToggleDrain, ToggleDrainPartEvent, TogglePartFlowMode, TogglePartResource, ToggleResourceDrainAction, ToggleResourceDrainFlowAction, TurnOffDrain, TurnOnDrain, UpdatePriorityIndices, UpdateResourceDrainRate, UpdateResourceDrainRateAndStatus, UpdateResourceDrainStatus, UpdateResourcesToRelease, UpdateVFXToUse
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ModuleResourceHandler.cs
- 380 lines; kind=class; type=ModuleResourceHandler; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization
- unity: none; unity_score=1
- ksp refs: Localizer=6, TimeWarp=4, ConfigNode=4, PartModule=2, GameEvents=2, PartResource=1; fan_in=1 fan_out=11
- methods(10): GetAverageInput, HasEnoughResourcesToAutoStart, IsResourceBelowShutOffLimit, LoadResList, OnAwake, OnLoad, PrintModuleResources, SetPartModule, UpdateModuleResourceInputs, UpdateModuleResourceOutputs
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ModuleResourceHarvester.cs
- 465 lines; kind=PartModule; type=ModuleResourceHarvester; bases=BaseDrill,IResourceConsumer; ancestry=BaseDrill/BaseConverter/PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Transform=2, Raycast/Query=2, Physics.=1, MonoBehaviour=1; unity_score=28
- ksp refs: Localizer=23, KSPField=7, PartResourceDefinition=6, HighLogic=5, Vessel=3, TimeWarp=1; fan_in=1 fan_out=30
- methods(14): CacheLocalStrings, CheckForImpact, GetConsumedResources, GetInfo, GetIntakeMultiplier, GetLocationString, GetModuleDisplayName, IsSituationValid, LoadRecipe, OnStart, PostProcess, PostUpdateCleanup, PreProcessing, PrepareRecipe
- auto suggestion: tier=B subsystem=PartModules-Resources

## Assembly-CSharp/ModuleResourceIntake.cs
- 267 lines; kind=PartModule; type=ModuleResourceIntake; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Transform=1, Lifecycle=1; unity_score=21
- ksp refs: KSPField=15, Localizer=9, Vector3d=2, KSPEvent=2, FloatCurve=2, FlightGlobals=2; fan_in=8 fan_out=18
- methods(10): Activate, CacheLocalStrings, Deactivate, FixedUpdate, GetInfo, GetModuleDisplayName, OnAwake, OnLoad, OnStart, ToggleAction
- auto suggestion: tier=A subsystem=PartModules-Engines

## Assembly-CSharp/ModuleResourceScanner.cs
- 259 lines; kind=PartModule; type=ModuleResourceScanner; bases=PartModule,IAnimatedModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: Lifecycle=1; unity_score=18
- ksp refs: Localizer=14, KSPField=5, FlightGlobals=3, PartResourceDefinition=2, PartModule=1, Part=1; fan_in=1 fan_out=18
- methods(15): CacheLocalStrings, CheckAbundanceDisplay, DisableModule, DisplayAbundance, EnableModule, GetDisplayFormat, GetInfo, GetModuleDisplayName, GetScanningMode, IsSituationValid, ModuleIsActive, OnAwake, OnStart, ToggleEvent, Update
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ModuleRotatingJoint.cs
- 281 lines; kind=PartModule; type=ModuleRotatingJoint; bases=ModuleJointMotor; ancestry=ModuleJointMotor/PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=20
- ksp refs: KSPField=10, KSPAction=6, KSPEvent=5, HighLogic=4, ConfigNode=2; fan_in=0 fan_out=8
- methods(19): EngageAction, EngageDrive, ForwardDrive, InvertAction, InvertDrive, LockAction, LockDrive, NeutralAction, NeutralDrive, OnJointInit, OnModuleLoad, OnModuleSave, OnModuleStart, OnMotorModeChanged, OnUpdate, ReverseDrive, ToggleControlMode, UpdateGuiNames, UpdateSavedState
- auto suggestion: tier=A subsystem=Physics

## Assembly-CSharp/ModuleSampleCollector.cs
- 160 lines; kind=PartModule; type=ModuleSampleCollector; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: IMGUI=16, UnityEngine.UI=2, Input.=1, GetComponent=1, GameObject=1; unity_score=76
- ksp refs: KSPField=5, KSPEvent=3, PartModule=1, HighLogic=1; fan_in=0 fan_out=9
- methods(6): Collect, Discard, OnGUI, OnStart, Transfer, drawTransferDialog
- auto suggestion: tier=C subsystem=Science

## Assembly-CSharp/ModuleSampleContainer.cs
- 40 lines; kind=PartModule; type=ModuleSampleContainer; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: none; unity_score=15
- ksp refs: KSPField=3, PartModule=1, KSPEvent=1; fan_in=1 fan_out=4
- methods(3): Discard, OnStart, TransferSample
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ModuleSAS.cs
- 208 lines; kind=PartModule; type=ModuleSAS; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Adjusters, Experience.Effects, KSP.Localization, UnityEngine
- unity: Lifecycle=2, GetComponent=1; unity_score=29
- ksp refs: KSPField=6, Localizer=5, HighLogic=2, PartModule=1; fan_in=2 fan_out=12
- methods(10): ApplySASServiceLevelAdjustments, CacheLocalStrings, FixedUpdate, GetInfo, GetModuleDisplayName, GetValue, OnDestroy, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStart
- auto suggestion: tier=A subsystem=PartModules-Control

## Assembly-CSharp/ModuleScienceContainer.cs
- 623 lines; kind=PartModule; type=ModuleScienceContainer; bases=PartModule,IScienceDataContainer; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: CommNet, KSP.Localization, KSP.UI.Screens.Flight.Dialogs
- unity: Lifecycle=1, Coroutine=1; unity_score=18
- ksp refs: Localizer=24, KSPField=15, KSPEvent=5, FlightGlobals=5, ConfigNode=5, Part=4; fan_in=7 fan_out=23
- methods(34): AddData, CollectAllAction, CollectAllEvent, CollectDataExternalEvent, DumpData, GetActiveVesselDataCount, GetData, GetModuleDisplayName, GetScienceCount, GetStoredDataCount, HasData, IsRerunnable, OnDataTransfer, OnDestroy, OnLoad, OnSave, OnStart, RemoveData, ReturnData, ReviewData, ReviewDataEvent, ReviewDataItem, StoreData, StoreDataExternalEvent, TransferDataEvent, TransferToContainer, discardData, keepData, onPartActionUIOpened, reviewData, sendDataToComms, sendDataToLab, sortContainerList, updateModuleUI
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ModuleScienceConverter.cs
- 185 lines; kind=PartModule; type=ModuleScienceConverter; bases=BaseConverter; ancestry=BaseConverter/PartModule/MonoBehaviour; ns=(global)
- usings: Experience.Effects, KSP.Localization
- unity: none; unity_score=15
- ksp refs: KSPField=9, Localizer=7, ProtoCrewMember=1, FlightGlobals=1; fan_in=2 fan_out=14
- methods(12): CacheLocalStrings, CalculateResearchTime, CalculateScienceAmount, CalculateScienceRate, DataExpectationSummary, GetScientists, IsSituationValid, OnStart, PostProcess, PreProcessing, PrepareRecipe, UpdateDisplayStrings
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ModuleScienceExperiment.cs **MUST-OPEN (>800 lines)**
- 1239 lines; kind=PartModule; type=ModuleScienceExperiment; bases=PartModule,IScienceDataContainer; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: CommNet, Experience.Effects, KSP.Localization, KSP.UI.Screens.Flight.Dialogs, UnityEngine
- unity: Coroutine=18, GameObject=8, SetActive=4, ScriptableObject=2, Lifecycle=2; unity_score=36
- ksp refs: Localizer=42, KSPField=36, GameEvents=15, KSPEvent=8, Vessel=5, FlightGlobals=5; fan_in=7 fan_out=48
- methods(65): CanBeDetached, CanBeOffset, CanBeRotated, CleanUpExperimentExternal, CollectDataExternalEvent, DeployAction, DeployExperiment, DeployExperimentExternal, DumpData, FindContainer, GetData, GetInfo, GetModuleDisplayName, GetScienceCount, GetStagingDisableText, GetStagingEnableText, IsRerunnable, IsStageable, Load, OnActive, OnAwake, OnCommandSeatInteraction, OnDataTransfer, OnDestroy, OnGamePause, OnGameUnpause, OnInventoryModeDisable, OnInventoryModeEnable, OnLoad, OnModuleInventoryChanged, OnSave, OnScienceComplete, OnScienceCompleteDelay, OnStart, OnTransferWarning, OnVesselModified, OnVesselSituationChange, ResetAction, ResetCallback, ResetExperiment, ResetExperimentExternal, ReturnData, ReviewData, ReviewDataEvent, ReviewDataItem ...
- auto suggestion: tier=B subsystem=Science

## Assembly-CSharp/ModuleScienceLab.cs
- 509 lines; kind=PartModule; type=ModuleScienceLab; bases=PartModule,IResourceConsumer,IScienceDataContainer; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: CommNet, Experience.Effects, KSP.Localization, KSP.UI.Screens, KSP.UI.Screens.SpaceCenter.MissionSummaryDialog, UnityEngine
- unity: Coroutine=10, GameObject=2, Time.=1, MonoBehaviour=1, Lifecycle=1; unity_score=35
- ksp refs: Localizer=19, KSPField=12, ConfigNode=7, ResearchAndDevelopment=5, PartResourceDefinition=4, GameEvents=4; fan_in=5 fan_out=36
- methods(24): CleanModulesEvent, CleanUpExperiment, CleanUpVesselExperiments, GetConsumedResources, GetInfo, GetModuleDisplayName, IsOperational, IsStorable, OnAwake, OnDestroy, OnLoad, OnSave, OnStart, OnTransmissionComplete, ProcessData, RecoverScienceLabs, StoreData, TransmissionErrorScreenMessage, TransmitScience, gatherProcessResources, lockdownAndGatherProcessResources, onPartActionUI, sendDataToComms, updateModuleUI
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ModuleSeeThroughObject.cs
- 145 lines; kind=PartModule; type=ModuleSeeThroughObject; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=4, GameObject=4, Shader=3, Renderer=2, Material=2, Camera=2; unity_score=77
- ksp refs: KSPField=8, EditorLogic=2, PartModule=1, HighLogic=1; fan_in=0 fan_out=6
- methods(5): GetCursorProximity, LateUpdate, MouseFadeUpdate, OnStart, SetOpacity
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ModuleServiceModule.cs
- 388 lines; kind=PartModule; type=ModuleServiceModule; bases=PartModule,IStageSeparator,IStageSeparatorChild,IScalarModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=8, Collider=6, GameObject=5, SetActive=4, Lifecycle=1, GetComponent=1; unity_score=52
- ksp refs: KSPField=9, Part=4, Vessel=2, HighLogic=2, GameEvents=2, PartModule=1; fan_in=1 fan_out=15
- methods(27): BuildJettisonPartsList, CheckCutaway, GetInfo, GetModuleDisplayName, GetStageIndex, IsEnginePlate, IsMoving, OnActive, OnDestroy, OnFieldUpdated, OnStart, PartDetaches, PointInBox, ProcessJettisonModules, SetExteriorCollider, SetScalar, SetUIRead, SetUIWrite, Setup, SetupJettisonModules, ShedExternalParts, ToggleCutaway, ToggleShellMesh, ToggleShrouds, UpdateCutaway, UpdateShrouds, UpdateStaging
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ModuleSpaceObjectAnalysis.cs
- 35 lines; kind=PartModule; type=ModuleSpaceObjectAnalysis; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: none; unity_score=15
- ksp refs: PartModule=2, Part=1; fan_in=2 fan_out=3
- methods(1): FindMatchingAnalyzer
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ModuleSpaceObjectInfo.cs
- 112 lines; kind=PartModule; type=ModuleSpaceObjectInfo; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: none; unity_score=15
- ksp refs: KSPField=4, PartModule=1, HighLogic=1; fan_in=2 fan_out=6
- methods(1): SetupSpaceObjectResources
- auto suggestion: tier=A subsystem=Celestials

## Assembly-CSharp/ModuleSpaceObjectResource.cs
- 50 lines; kind=PartModule; type=ModuleSpaceObjectResource; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=22
- ksp refs: KSPField=7, PartResourceDefinition=1, PartModule=1, ConfigNode=1; fan_in=5 fan_out=6
- methods(1): OnLoad
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ModuleStatusLight.cs
- 95 lines; kind=PartModule; type=ModuleStatusLight; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Light=5, Renderer=2, Material=2, GameObject=2, Shader=1; unity_score=76
- ksp refs: KSPField=5, PartModule=1; fan_in=1 fan_out=3
- methods(3): OnInventoryModeDisable, OnStart, SetStatus
- auto suggestion: tier=C subsystem=Part

## Assembly-CSharp/ModuleStructuralNode.cs
- 136 lines; kind=PartModule; type=ModuleStructuralNode; bases=PartModule,IActivateOnDecouple; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: SetActive=2, GameObject=2, Transform=1, Lifecycle=1; unity_score=41
- ksp refs: KSPField=7, AttachNode=7, PartModule=1, Part=1, HighLogic=1; fan_in=1 fan_out=6
- methods(8): CheckDisplay, CheckDisplayEditor, DecoupleAction, DespawnStructure, LateUpdate, OnStart, SetNodeState, SpawnStructure
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ModuleStructuralNodeToggle.cs
- 54 lines; kind=PartModule; type=ModuleStructuralNodeToggle; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: Lifecycle=1; unity_score=28
- ksp refs: KSPField=4, PartModule=1; fan_in=0 fan_out=5
- methods(3): OnFieldUpdated, Start, UpdateNodes
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/ModuleSurfaceFX.cs
- 237 lines; kind=PartModule; type=ModuleSurfaceFX; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=5, Raycast/Query=3, Lifecycle=3, GetComponent=2, Transform=1, Physics.=1; unity_score=55
- ksp refs: KSPField=5, FlightGlobals=3, Vector3d=1, PartModule=1, HighLogic=1, GameSettings=1; fan_in=1 fan_out=11
- methods(6): GetSurfaceFX, OnDestroy, OnDrawGizmos, Start, Update, UpdateSrfFX
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ModuleTestSubject.cs
- 204 lines; kind=PartModule; type=ModuleTestSubject; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: Contracts, Contracts.Parameters, Contracts.Templates, KSP.Localization
- unity: Coroutine=2, Lifecycle=1; unity_score=26
- ksp refs: KSPField=6, Localizer=4, GameEvents=4, Contract=3, ConfigNode=2, Vessel=1; fan_in=3 fan_out=17
- methods(15): CacheLocalStrings, GetModuleDisplayName, GetTestNotes, IsStageable, OnActive, OnAwake, OnContractStateChange, OnDestroy, OnLoad, OnStart, RunTest, RunTestEvent, SituationAvailable, delayedStart, onPartDie
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ModuleToggleCrossfeed.cs
- 209 lines; kind=PartModule; type=ModuleToggleCrossfeed; bases=PartModule,IToggleCrossfeed; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: Lifecycle=1; unity_score=19
- ksp refs: KSPField=9, HighLogic=5, KSPAction=3, Game=2, ResearchAndDevelopment=1, PartModule=1; fan_in=2 fan_out=17
- methods(17): Activate, CrossfeedHasTech, CrossfeedRequiresTech, CrossfeedTech, CrossfeedToggleableEditor, CrossfeedToggleableFlight, Deactivate, DisableAction, EnableAction, GetModuleDisplayName, OnAwake, OnLoad, OnStart, Start, ToggleAction, ToggleEvent, UpdateCrossfeed
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/ModuleTripLogger.cs
- 216 lines; kind=PartModule; type=ModuleTripLogger; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization
- unity: SendMessage/Invoke=3, Lifecycle=1; unity_score=28
- ksp refs: Vessel=12, GameEvents=8, CelestialBody=5, ConfigNode=3, Orbit=2, ProtoVessel=1; fan_in=3 fan_out=12
- methods(16): CacheLocalStrings, ForceLogEntry, ForceVesselLog, GetModuleDisplayName, InvokeLogVessel, LoadDeprecated, LogNewSituation, LogSituation, LogVesselSituation, OnAwake, OnDestroy, OnLoad, OnSave, OnStart, OnVesselSOIChanged, OnVesselSituationChanged
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ModuleWheelBase.cs **MUST-OPEN (>800 lines)**
- 1125 lines; kind=PartModule; type=ModuleWheelBase; bases=PartModule,IModuleInfo,IContractObjectiveModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, ModuleWheels, UnityEngine, VehiclePhysics
- unity: GameObject=26, Rigidbody=23, Transform=13, GetComponent=11, WheelCollider=6, Collider=6; unity_score=64
- ksp refs: KSPField=33, GameEvents=14, Part=10, HighLogic=7, Vessel=6, GameSettings=3; fan_in=7 fan_out=43
- methods(53): ATsymPartUpdate, ActAutoFrictionToggle, ActionUIUpdate, ApplyGeeBias, CheckContractObjectiveValidity, CheckSubsteps, CheckSuspensionToggle, DemoteToPhysicslessPart, DisableSuspension, DisableWheelCollider, EnableSuspension, EnableWheelCollider, EvtAutoFrictionToggle, FixedUpdate, GetContractObjectiveType, GetDrawModulePanelCallback, GetInfo, GetModuleDisplayName, GetModuleTitle, GetPrimaryField, IgnoreForcesOnSameVesselContact, InopUpdate, InopWheelCollider, InopWheelTransform, LSchloomphaVPPProcessing, LandedDetectionNeedsUpdate, LandingDetectionUpdate, OnAwake, OnDestroy, OnInventoryModeDisable, OnInventoryModeEnable, OnPutToGround, OnStart, OnSubsystemsModified, OnVesselModified, OnWheelSetup, PromoteToPhysicalPart, RegisterSubmodule, ResetWheelGroundCheck, SOIChange, UnregisterSubmodule, UpdateFriction, getFixFwd, getFixTorque, holdWheelDamage ...
- auto suggestion: tier=B subsystem=PartModules-Wheels

## Assembly-CSharp/MonoUtilities.cs
- 30 lines; kind=MonoBehaviour; type=MonoUtilities; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, GetComponent=1; unity_score=50
- ksp refs: UIPartActionWindow=4, Part=2; fan_in=12 fan_out=3
- methods(2): RefreshContextWindows, RefreshPartContextWindow
- auto suggestion: tier=B subsystem=Core

## Assembly-CSharp/Mouse.cs
- 397 lines; kind=MonoBehaviour; type=Mouse; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Input.=22, Time.=7, Raycast/Query=7, Lifecycle=5, Coroutine=4, Screen/Cursor=3; unity_score=72
- ksp refs: Part=7, GameSettings=3, HighLogic=2, FlightGlobals=2, EditorLogic=1; fan_in=38 fan_out=10
- methods(19): Awake, CheckButtons, CheckHoveredPart, ClearMouseState, GetAllMouseButtons, GetAllMouseButtonsDown, GetAllMouseButtonsUp, GetButton, GetButtonDown, GetButtonUp, GetClick, GetDoubleClick, GetDragDelta, GetValidHoverPart, OnDestroy, Start, TapRoutine, Update, WasDragging
- auto suggestion: tier=B subsystem=Input

## Assembly-CSharp/MouseEventsHandler.cs
- 3 lines; kind=MonoBehaviour; type=MouseEventsHandler; bases=MouseEventsHandlerBase; ancestry=MouseEventsHandlerBase/MonoBehaviour; ns=(global)
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=1
- methods(0): 
- auto suggestion: tier=B subsystem=Input
