# Review chunk 09 (ExpB): 239 files, 53052 lines

## Assembly-CSharp/Expansions/BundleLoader.cs
- 163 lines; kind=MonoBehaviour; type=BundleLoader; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions
- usings: UnityEngine, UnityEngine.Networking
- unity: AssetBundle=4, Texture=3, WWW/WebRequest=2, MonoBehaviour=2, Coroutine=2, Lifecycle=1; unity_score=68
- ksp refs: HighLogic=1; fan_in=5 fan_out=2
- methods(6): Awake, IsBundleLoaded, LoadAsset, LoadAssetBundle, LoadScene, LoadTextureAsset
- auto suggestion: tier=B subsystem=Modding

## Assembly-CSharp/Expansions/ExpansionsLoader.cs
- 592 lines; kind=MonoBehaviour; type=ExpansionsLoader; bases=LoadingSystem; ancestry=LoadingSystem/MonoBehaviour; ns=Expansions
- usings: Expansions.Missions, KSP.Localization, UnityEngine, UnityEngine.Networking
- unity: Coroutine=8, AssetBundle=3, WWW/WebRequest=2, Time.=2, Lifecycle=2, Instantiate/Destroy=1; unity_score=43
- ksp refs: ConfigNode=15, Localizer=14, ProtoCrewMember=5, GameEvents=1; fan_in=100 fan_out=11
- methods(24): Awake, BuildMissionFileHash, FindMissionNode, GetExpansionDirectory, GetExpansionVersion, GetExpansionsThatFailedToLoad, GetInstalledExpansions, GetInstalledExpansionsString, GetMissionVerificationStrings, InitPublicKeyCryptoProvider, InitializeExpansion, IsAnyExpansionInstalled, IsExpansionAnyKerbalSuitInstalled, IsExpansionInstalled, IsExpansionKerbalSuitInstalled, IsInstalled, IsReady, LoadExpansions, OnDestroy, ProgressFraction, ProgressTitle, ReportInstalledExpansions, StartLoad, VerifyHashSignature
- auto suggestion: tier=B subsystem=Modding

## Assembly-CSharp/Expansions/ExpansionSO.cs
- 77 lines; kind=ScriptableObject; type=ExpansionSO; bases=ScriptableObject; ancestry=ScriptableObject; ns=Expansions
- usings: UnityEngine
- unity: ScriptableObject=1; unity_score=32
- ksp refs: ProtoCrewMember=3; fan_in=1 fan_out=1
- methods(2): CreateExpansion, SetBundleInfoList
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/Expansions/KSPExpansionsUtils.cs
- 9 lines; kind=static class; type=KSPExpansionsUtils; bases=-; ancestry=-; ns=Expansions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=9 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/Expansions/Missions/ActionModule.cs
- 256 lines; kind=MonoBehaviour; type=ActionModule; bases=MonoBehaviour,IConfigNode,IActionModule,IMENodeDisplay; ancestry=MonoBehaviour; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1, Coroutine=1; unity_score=31
- ksp refs: GameEvents=8, Vessel=4, Localizer=3, ConfigNode=2; fan_in=23 fan_out=14
- methods(36): ActionLocation, AddParameterToNodeBody, AddParameterToNodeBodyAndUpdateUI, AddParameterToSAP, Awake, Fire, GetAppObjectiveInfo, GetDisplayName, GetInfo, GetInternalParametersToDisplay, GetName, GetNode, GetNodeBodyParameterString, HasNodeBodyParameter, HasSAPParameter, Initialize, Load, OnCloned, OnDestroy, OnPartPersistentIdChanged, OnVesselPersistentIdChanged, OnVesselsDocking, OnVesselsUndocking, ParameterSetupComplete, PartPersistentIdChangedWrapper, RemoveParameterFromNodeBody, RemoveParameterFromNodeBodyAndUpdateUI, RemoveParameterFromSAP, RunValidation, RunValidationWrapper, Save, Start, UpdateNodeBodyUI, VesselDockingWrapper, VesselPersistentIdChangedWrapper, VesselsUndockingWrapper
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ActionNumberOperator.cs
- 11 lines; kind=enum; type=ActionNumberOperator; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionCreateAsteroid.cs
- 173 lines; kind=MonoBehaviour; type=ActionCreateAsteroid; bases=ActionModule,INodeOrbit; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=23
- ksp refs: ConfigNode=11, Orbit=6, Vessel=4, ProtoVessel=3, Localizer=3, HighLogic=3; fan_in=8 fan_out=20
- methods(12): ActionLocation, Awake, Fire, GetInfo, GetNodeBodyParameterString, GetNodeOrbit, GetProtoVesselNode, HasNodeOrbit, Load, OnCloned, Save, SpawnAsteroid
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionCreateComet.cs
- 183 lines; kind=MonoBehaviour; type=ActionCreateComet; bases=ActionModule,INodeOrbit; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=23
- ksp refs: ConfigNode=14, Orbit=6, Vessel=4, ProtoVessel=3, Localizer=3, HighLogic=3; fan_in=8 fan_out=22
- methods(12): ActionLocation, Awake, Fire, GetInfo, GetNodeBodyParameterString, GetNodeOrbit, GetProtoVesselNode, HasNodeOrbit, Load, OnCloned, Save, SpawnComet
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionCreateFlag.cs
- 126 lines; kind=MonoBehaviour; type=ActionCreateFlag; bases=ActionModule; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=25
- ksp refs: ConfigNode=10, Localizer=5, ProtoVessel=2, Orbit=2, HighLogic=2, FlightGlobals=2; fan_in=6 fan_out=16
- methods(10): ActionLocation, Awake, CreateFlagPartNode, Fire, GetInfo, GetNodeBodyParameterString, GetProtoVesselNode, Load, OnCloned, Save
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionCreateKerbal.cs
- 354 lines; kind=MonoBehaviour; type=ActionCreateKerbal; bases=ActionModule,INodeOrbit,IMissionKerbal; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1, GetComponent=1; unity_score=24
- ksp refs: ProtoCrewMember=13, ConfigNode=12, Orbit=6, HighLogic=5, Vessel=4, ProtoVessel=4; fan_in=9 fan_out=32
- methods(22): ActionLocation, Awake, CreateKerbalPartNode, Fire, GetInfo, GetNodeBodyParameterString, GetNodeOrbit, GetProtoVesselNode, HasNodeOrbit, Initialize, KerbalAdded, KerbalNameChange, KerbalRemoved, KerbalRosterStatusChange, KerbalTypeChange, Load, NodeDeleted, OnCloned, OnHelmetValueChange, OnNeckRingControlCreated, Save, SpawnKerbal
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionCreateLaunchSite.cs
- 99 lines; kind=MonoBehaviour; type=ActionCreateLaunchSite; bases=ActionModule; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1, Coroutine=1; unity_score=32
- ksp refs: Localizer=8, ConfigNode=3, FlightGlobals=1; fan_in=4 fan_out=9
- methods(8): Awake, Fire, GetInfo, GetNodeBodyParameterString, Initialize, Load, RunValidation, Save
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionCreateVessel.cs
- 539 lines; kind=MonoBehaviour; type=ActionCreateVessel; bases=ActionModule,INodeOrbit; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, Expansions.Missions.Runtime, FinePrint.Utilities, KSP.Localization, KSP.UI.Screens, PreFlightTests, UnityEngine
- unity: Coroutine=8, Transform=2, SetActive=1, Lifecycle=1, Instantiate/Destroy=1, GameObject=1; unity_score=35
- ksp refs: Localizer=42, HighLogic=14, MissionSystem=6, FlightGlobals=5, Orbit=4, TimeWarp=3; fan_in=10 fan_out=52
- methods(16): ActionLocation, Awake, CreatePlaceholderCraft, CreateVesselScreenMessage, DeletePlaceholderCraft, Fire, GetAppObjectiveInfo, GetInfo, GetNodeBodyParameterString, GetNodeOrbit, HasNodeOrbit, Initialize, Load, OnCloned, RunValidation, Save
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionDialogMessage.cs
- 213 lines; kind=MonoBehaviour; type=ActionDialogMessage; bases=ActionModule; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, Expansions.Missions.Runtime, KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: SetActive=1, Lifecycle=1, GameObject=1; unity_score=27
- ksp refs: Localizer=8, ConfigNode=2; fan_in=2 fan_out=16
- methods(12): Awake, Fire, GetInfo, GetNodeBodyParameterString, Initialize, Load, OnautoClose, OnautoCloseTimeoutCreated, ParameterSetupComplete, Save, SendStateMessage, dialogClosed
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionMessage.cs
- 58 lines; kind=MonoBehaviour; type=ActionMessage; bases=ActionModule; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=27
- ksp refs: Localizer=2, ConfigNode=2; fan_in=0 fan_out=10
- methods(5): Awake, Fire, GetInfo, Load, Save
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionMissionScore.cs
- 114 lines; kind=MonoBehaviour; type=ActionMissionScore; bases=ActionModule; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1, Coroutine=1; unity_score=27
- ksp refs: Localizer=2, ConfigNode=2; fan_in=4 fan_out=12
- methods(12): Awake, Fire, GetAppObjectiveInfo, GetAwardedScoreDescription, GetInfo, GetInternalParametersToDisplay, GetScoreDescription, Initialize, Load, OnDockStatusChange, OnMissionScoreModuleCreated, Save
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionModifierFunds.cs
- 74 lines; kind=MonoBehaviour; type=ActionModifierFunds; bases=ActionModule; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=25
- ksp refs: Funding=11, ConfigNode=2, Localizer=1; fan_in=0 fan_out=10
- methods(5): Awake, Fire, GetInfo, Load, Save
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionModifierScore.cs
- 15 lines; kind=MonoBehaviour; type=ActionModifierScore; bases=ActionMissionScore; ancestry=ActionMissionScore/ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=4
- methods(1): Initialize
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionPartExplode.cs
- 139 lines; kind=MonoBehaviour; type=ActionPartExplode; bases=ActionModule; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1, Coroutine=1; unity_score=36
- ksp refs: FlightGlobals=6, Vessel=4, Localizer=4, ConfigNode=2, Part=1; fan_in=1 fan_out=11
- methods(10): Awake, Fire, GetInfo, GetNodeBodyParameterString, Load, OnPartPersistentIdChanged, OnVesselPersistentIdChanged, OnVesselsUndocking, RunValidation, Save
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionPartFailure.cs
- 338 lines; kind=MonoBehaviour; type=ActionPartFailure; bases=ActionModule; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Adjusters, Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Coroutine=4, Lifecycle=2; unity_score=34
- ksp refs: Localizer=13, GameEvents=5, Vessel=4, FlightGlobals=4, Part=3, ConfigNode=2; fan_in=2 fan_out=26
- methods(19): Awake, DynamicModuleListControlCreated, Fire, GetInfo, GetInternalParametersToDisplay, GetNodeBodyParameterString, Initialize, Load, OnAdjusterListModified, OnDestroy, OnPartIDChanged, OnPartPersistentIdChanged, OnVesselPersistentIdChanged, OnVesselsUndocking, RunValidation, Save, VesselPartSelectorControlCreated, addAdjustersVessel, onVesselSituationChanged
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionPartRepair.cs
- 527 lines; kind=MonoBehaviour; type=ActionPartRepair; bases=ActionModule; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Adjusters, Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: GameObject=14, SetActive=9, Lifecycle=2, Coroutine=2, GetComponent=1; unity_score=46
- ksp refs: Localizer=18, GameEvents=6, PartModule=4, ConfigNode=4, FlightGlobals=3, Vessel=2; fan_in=1 fan_out=28
- methods(20): Awake, Fire, GetInfo, GetNodeBodyParameterString, Load, OnDestroy, OnFailureDropdownControlCreated, OnPartIDChanged, OnPartModuleDropdownControlCreated, OnRepairTypeControlCreated, OnRepairTypeValueChange, RepairFailures, RunValidation, Save, SetFailureNamesForDropdown, SetPartModuleNamesForDropdown, SetRepairTypeDropdown, VesselPartSelectorControlCreated, onFailureListChanged, onVesselSituationChanged
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionPartResourceAmount.cs
- 182 lines; kind=MonoBehaviour; type=ActionPartResourceAmount; bases=ActionVessel; ancestry=ActionVessel/ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Coroutine=2, Lifecycle=1; unity_score=32
- ksp refs: PartResourceDefinition=4, Localizer=4, GameEvents=2, ConfigNode=2, ProtoPartSnapshot=1; fan_in=0 fan_out=15
- methods(9): Awake, Fire, GetInfo, GetNodeBodyParameterString, Load, OnResourceNameControlSetup, Save, SetResourceNames, adjustUnloadedPart
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionPartResourceDrain.cs
- 406 lines; kind=MonoBehaviour; type=ActionPartResourceDrain; bases=ActionModule; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Coroutine=3, Lifecycle=1; unity_score=29
- ksp refs: Localizer=11, FlightGlobals=5, Vessel=3, PartResourceDefinition=3, GameEvents=3, ProtoPartSnapshot=2; fan_in=1 fan_out=21
- methods(17): APRD_SetDropDownValues, Awake, Fire, GetInfo, GetNodeBodyParameterString, Load, OnPartPersistentIdChanged, OnVesselPersistentIdChanged, OnVesselsUndocking, RunValidation, Save, Setup, drainPart, drainUnloadedPart, drainVessel, findPart, findVessel
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionSetCamera.cs
- 46 lines; kind=MonoBehaviour; type=ActionSetCamera; bases=ActionModule; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=30
- ksp refs: Localizer=2, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Awake, Fire, GetInfo, Load, Save
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionVessel.cs
- 131 lines; kind=MonoBehaviour; type=ActionVessel; bases=ActionModule; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=21
- ksp refs: FlightGlobals=7, Vessel=5, Localizer=5, ConfigNode=2, ProtoPartSnapshot=1, Part=1; fan_in=2 fan_out=15
- methods(8): Awake, GetNodeBodyParameterString, Load, OnVesselPersistentIdChanged, OnVesselsDocking, OnVesselsUndocking, Save, vesselSelectorCreated
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionVesselActionGroup.cs
- 51 lines; kind=MonoBehaviour; type=ActionVesselActionGroup; bases=ActionModule; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Coroutine=3, UnityEngine.UI=1, Lifecycle=1; unity_score=61
- ksp refs: Localizer=2, FlightGlobals=2, ConfigNode=2; fan_in=0 fan_out=7
- methods(6): Awake, Fire, GetInfo, Load, Save, actionGroupControlComplete
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionVesselExplode.cs
- 76 lines; kind=MonoBehaviour; type=ActionVesselExplode; bases=ActionVessel; ancestry=ActionVessel/ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Coroutine=3, Lifecycle=1; unity_score=45
- ksp refs: Localizer=3, Part=2, ConfigNode=2; fan_in=0 fan_out=7
- methods(6): Awake, Fire, GetInfo, GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionVesselStage.cs
- 42 lines; kind=MonoBehaviour; type=ActionVesselStage; bases=ActionModule; ancestry=ActionModule/MonoBehaviour; ns=Expansions.Missions.Actions
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Coroutine=3, UnityEngine.UI=1, Lifecycle=1; unity_score=66
- ksp refs: Localizer=2, FlightGlobals=2, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Awake, Fire, GetInfo, Load, Save
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/FundsModifierType.cs
- 18 lines; kind=enum; type=FundsModifierType; bases=-; ancestry=-; ns=Expansions.Missions.Actions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Asteroid.cs
- 98 lines; kind=class; type=Asteroid; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, UnityEngine
- unity: GameObject=2, GetComponent=1; unity_score=23
- ksp refs: ConfigNode=3, FlightGlobals=1; fan_in=8 fan_out=9
- methods(6): GetAsteroidSeed, Load, OnGAPPrefabInstantiated, Randomize, Save, SetGAPAsteroid
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardDefinition.cs
- 44 lines; kind=class; type=AwardDefinition; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: UnityEngine
- unity: Texture=2; unity_score=41
- ksp refs: GameDatabase=2, ConfigNode=2; fan_in=14 fan_out=3
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule.cs
- 98 lines; kind=class; type=AwardModule; bases=DynamicModule; ancestry=DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, Expansions.Missions.Runtime, KSP.Localization, UnityEngine
- unity: none; unity_score=4
- ksp refs: MissionSystem=2, ConfigNode=2, Localizer=1, HighLogic=1; fan_in=13 fan_out=10
- methods(7): EvaluateAward, EvaluateCondition, GetDisplayName, Load, Save, StartTracking, StopTracking
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_Fuel.cs
- 148 lines; kind=class; type=AwardModule_Fuel; bases=AwardModule; ancestry=AwardModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor
- unity: none; unity_score=0
- ksp refs: Vessel=8, ConfigNode=7, GameEvents=4, PartResourceDefinition=3, HighLogic=2, Game=1; fan_in=0 fan_out=17
- methods(9): EvaluateCondition, GetCurrentResourceFromVessel, Load, OnVesselCreated, OnVesselWillDestroy, Save, SetResourceNames, StartTracking, StopTracking
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_KerbalsDied.cs
- 30 lines; kind=class; type=AwardModule_KerbalsDied; bases=AwardModule; ancestry=AwardModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: ProtoCrewMember=1, KerbalRoster=1, HighLogic=1; fan_in=0 fan_out=7
- methods(1): EvaluateCondition
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_KerbalsSurvived.cs
- 57 lines; kind=class; type=AwardModule_KerbalsSurvived; bases=AwardModule; ancestry=AwardModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=4, HighLogic=2, GameEvents=2, Game=1; fan_in=0 fan_out=9
- methods(6): EvaluateCondition, Load, OnCrewKilled, Save, StartTracking, StopTracking
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_LandedOn.cs
- 75 lines; kind=class; type=AwardModule_LandedOn; bases=AwardModule; ancestry=AwardModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, FinePrint.Utilities, UnityEngine
- unity: none; unity_score=5
- ksp refs: Vessel=4, FlightGlobals=4, ConfigNode=2, CelestialBody=1; fan_in=0 fan_out=15
- methods(4): EvaluateCondition, GetVessel, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_MissionTime.cs
- 37 lines; kind=class; type=AwardModule_MissionTime; bases=AwardModule; ancestry=AwardModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor
- unity: none; unity_score=0
- ksp refs: ConfigNode=2, HighLogic=1; fan_in=0 fan_out=7
- methods(3): EvaluateCondition, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_PlanetsLanded.cs
- 73 lines; kind=class; type=AwardModule_PlanetsLanded; bases=AwardModule; ancestry=AwardModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor
- unity: none; unity_score=0
- ksp refs: ConfigNode=5, Vessel=3, GameEvents=3, HighLogic=2, Game=1; fan_in=0 fan_out=11
- methods(6): EvaluateCondition, Load, OnVesselSitutationChange, Save, StartTracking, StopTracking
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_Score.cs
- 38 lines; kind=class; type=AwardModule_Score; bases=AwardModule; ancestry=AwardModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=1 fan_out=5
- methods(3): EvaluateCondition, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_TraveledLessThan.cs
- 68 lines; kind=class; type=AwardModule_TraveledLessThan; bases=AwardModule; ancestry=AwardModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor
- unity: none; unity_score=0
- ksp refs: ConfigNode=4, HighLogic=2, GameEvents=2, FlightGlobals=2, Vessel=1, Game=1; fan_in=0 fan_out=12
- methods(6): EvaluateCondition, Load, OnVesselWillDestroy, Save, StartTracking, StopTracking
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_TraveledMoreThan.cs
- 68 lines; kind=class; type=AwardModule_TraveledMoreThan; bases=AwardModule; ancestry=AwardModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor
- unity: none; unity_score=0
- ksp refs: ConfigNode=4, HighLogic=2, GameEvents=2, FlightGlobals=2, Vessel=1, Game=1; fan_in=0 fan_out=12
- methods(6): EvaluateCondition, Load, OnVesselWillDestroy, Save, StartTracking, StopTracking
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_VesselsUsed.cs
- 75 lines; kind=class; type=AwardModule_VesselsUsed; bases=AwardModule; ancestry=AwardModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor
- unity: none; unity_score=0
- ksp refs: ConfigNode=5, HighLogic=2, GameEvents=2, Vessel=1, Game=1; fan_in=0 fan_out=11
- methods(6): EvaluateCondition, Load, OnVesselChange, Save, StartTracking, StopTracking
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Awards.cs
- 43 lines; kind=MonoBehaviour; type=Awards; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1; unity_score=51
- ksp refs: none; fan_in=6 fan_out=1
- methods(3): Awake, GetAwardDefinition, SetupAwards
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardWidget.cs
- 49 lines; kind=MonoBehaviour; type=AwardWidget; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions
- usings: Expansions.Missions.Runtime, KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=4, TMPro=2, Instantiate/Destroy=2, GetComponent=2, UnityEngine.UI=1, Transform=1; unity_score=87
- ksp refs: MissionSystem=1; fan_in=2 fan_out=5
- methods(2): Create, Start
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/CheckpointValidationType.cs
- 10 lines; kind=enum; type=CheckpointValidationType; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=17 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Comet.cs
- 146 lines; kind=class; type=Comet; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, UnityEngine
- unity: GameObject=2, GetComponent=1; unity_score=17
- ksp refs: ConfigNode=3, FlightGlobals=1; fan_in=7 fan_out=13
- methods(8): GetRandomCometSeed, Load, OnCometTypeChanged, OnGAPPrefabInstantiated, Randomize, Save, SetCometTypes, SetGAPComet
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Crew.cs
- 66 lines; kind=class; type=Crew; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: KSP.Localization
- unity: none; unity_score=6
- ksp refs: Localizer=4, ConfigNode=3; fan_in=15 fan_out=3
- methods(3): Load, LocalizeCrewMember, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/DynamicModule.cs
- 216 lines; kind=class; type=DynamicModule; bases=IConfigNode,IMENodeDisplay,ICloneable; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Instantiate/Destroy=1; unity_score=4
- ksp refs: ConfigNode=2, Localizer=1; fan_in=9 fan_out=7
- methods(27): AddParameterToNodeBody, AddParameterToNodeBodyAndUpdateUI, AddParameterToSAP, Clone, Destroy, Equals, GetDefaultPinnedParameters, GetDisplayName, GetDisplayToolTip, GetHashCode, GetInfo, GetInternalParametersToDisplay, GetModuleInstanceID, GetName, GetNode, GetNodeBodyParameterString, HasNodeBodyParameter, HasSAPParameter, Initialize, Load, ParameterSetupComplete, RemoveParameterFromNodeBody, RemoveParameterFromNodeBodyAndUpdateUI, RemoveParameterFromSAP, Save, SetNode, UpdateNodeBodyUI
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/DynamicModuleList.cs
- 150 lines; kind=class; type=DynamicModuleList; bases=IConfigNode,ICloneable; ancestry=IConfigNode; ns=Expansions.Missions
- usings: UnityEngine
- unity: none; unity_score=3
- ksp refs: ConfigNode=8; fan_in=5 fan_out=6
- methods(10): AddDynamicModule, Clone, ContainsModule, Equals, GetHashCode, GetInternalParameters, GetSupportedTypes, Load, Save, SetSupportedTypes
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/IMEFlowBlock.cs
- 15 lines; kind=interface; type=IMEFlowBlock; bases=-; ancestry=-; ns=Expansions.Missions.Flow
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowBlock.cs
- 166 lines; kind=class; type=MEFlowBlock; bases=IMEFlowBlock,IConfigNode; ancestry=IMEFlowBlock; ns=Expansions.Missions.Flow
- usings: UnityEngine
- unity: none; unity_score=5
- ksp refs: ConfigNode=5; fan_in=4 fan_out=8
- methods(4): AddBlock, Load, Save, UpdateMissionFlowUI
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowConvergence.cs
- 15 lines; kind=class; type=MEFlowConvergence; bases=-; ancestry=-; ns=Expansions.Missions.Flow
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowExtensions.cs
- 38 lines; kind=static class; type=MEFlowExtensions; bases=-; ancestry=-; ns=Expansions.Missions.Flow
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=2
- methods(3): CountStartNodes, DistinctStartNodes, GetPathsStarting
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowOrBlock.cs
- 20 lines; kind=class; type=MEFlowOrBlock; bases=MEFlowBlock; ancestry=MEFlowBlock/IMEFlowBlock; ns=Expansions.Missions.Flow
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=3
- methods(1): UpdateMissionFlowUI
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowParser.cs
- 600 lines; kind=MonoBehaviour; type=MEFlowParser; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Flow
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Transform=20, GameObject=15, Instantiate/Destroy=4, TMPro=3, UnityEngine.UI=2, SetActive=2; unity_score=59
- ksp refs: none; fan_in=16 fan_out=18
- methods(15): Awake, BuildPathsByDFS, BuildReversePathsByDFS, ClearChildren, Create, CreateFlowStartNodes, CreateFlowUIBlock, CreateMissionFlowUI, CreateMissionFlowUI_Button, CreateMissionFlowUI_Toggle, ExtractPathGroups, ParseBlock, ParseMission, Start, UpdateFlowUIItems
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowThenBlock.cs
- 20 lines; kind=class; type=MEFlowThenBlock; bases=MEFlowBlock; ancestry=MEFlowBlock/IMEFlowBlock; ns=Expansions.Missions.Flow
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=3
- methods(1): UpdateMissionFlowUI
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowUIGroup_Or.cs
- 45 lines; kind=MonoBehaviour; type=MEFlowUIGroup_Or; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Flow
- usings: UnityEngine
- unity: GameObject=4, Transform=2, SetActive=2, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1; unity_score=84
- ksp refs: none; fan_in=1 fan_out=2
- methods(3): Create, OnDestroy, OnUpdateFlowUI
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowUIGroup_Then.cs
- 67 lines; kind=MonoBehaviour; type=MEFlowUIGroup_Then; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Flow
- usings: UnityEngine, UnityEngine.UI
- unity: GameObject=4, Transform=2, SetActive=2, UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1; unity_score=79
- ksp refs: none; fan_in=1 fan_out=2
- methods(5): Create, OnDestroy, OnUpdateFlowUI, SetBracketColor, SetUnreachable
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowUINode.cs
- 381 lines; kind=MonoBehaviour; type=MEFlowUINode; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Flow
- usings: Expansions.Missions.Actions, Expansions.Missions.Runtime, KSP.Localization, KSP.UI, KSP.UI.TooltipTypes, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=35, SetActive=23, GetComponent=7, UnityEngine.UI=6, EventSystems=6, Lifecycle=3; unity_score=82
- ksp refs: Localizer=14, HighLogic=11, Game=3, MissionSystem=1; fan_in=4 fan_out=27
- methods(11): Awake, Create, OnButtonClick, OnDestroy, OnToggle, OnUpdateFlowUI, OnVesselNodeButtonCallback, SetNode, SetNodeInfo, SetStatus, Start
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowUINodeLeader.cs
- 13 lines; kind=class; type=MEFlowUINodeLeader; bases=-; ancestry=-; ns=Expansions.Missions.Flow
- usings: UnityEngine
- unity: Texture=1; unity_score=35
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowUIText.cs
- 10 lines; kind=MonoBehaviour; type=MEFlowUIText; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Flow
- usings: TMPro, UnityEngine
- unity: TMPro=1, MonoBehaviour=1; unity_score=55
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MENodePathInfo.cs
- 66 lines; kind=class; type=MENodePathInfo; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions.Flow
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: ConfigNode=4; fan_in=5 fan_out=5
- methods(3): AddPath, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEPath.cs
- 94 lines; kind=class; type=MEPath; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions.Flow
- usings: UnityEngine
- unity: none; unity_score=4
- ksp refs: ConfigNode=2; fan_in=6 fan_out=5
- methods(5): AddNodes, Clone, GetConvergence, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEPathGroup.cs
- 32 lines; kind=class; type=MEPathGroup; bases=-; ancestry=-; ns=Expansions.Missions.Flow
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/IActionModule.cs
- 21 lines; kind=interface; type=IActionModule; bases=IConfigNode,IMENodeDisplay; ancestry=-; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, UnityEngine
- unity: none; unity_score=3
- ksp refs: none; fan_in=6 fan_out=5
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/IMENodeDisplay.cs
- 40 lines; kind=interface; type=IMENodeDisplay; bases=-; ancestry=-; ns=Expansions.Missions
- usings: Expansions.Missions.Editor
- unity: none; unity_score=0
- ksp refs: none; fan_in=14 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/IMissionKerbal.cs
- 17 lines; kind=interface; type=IMissionKerbal; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: ProtoCrewMember=9; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/INodeBody.cs
- 9 lines; kind=interface; type=INodeBody; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: CelestialBody=1; fan_in=8 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/INodeOrbit.cs
- 9 lines; kind=interface; type=INodeOrbit; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: Orbit=1; fan_in=8 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/INodeWaypoint.cs
- 11 lines; kind=interface; type=INodeWaypoint; bases=-; ancestry=-; ns=Expansions.Missions
- usings: FinePrint
- unity: none; unity_score=0
- ksp refs: none; fan_in=12 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/IScoreableObjective.cs
- 9 lines; kind=interface; type=IScoreableObjective; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=37 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ITestModule.cs
- 21 lines; kind=interface; type=ITestModule; bases=IConfigNode,IMENodeDisplay; ancestry=-; ns=Expansions.Missions
- usings: Expansions.Missions.Editor
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=5
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ITestNodeLabel.cs
- 17 lines; kind=interface; type=ITestNodeLabel; bases=-; ancestry=-; ns=Expansions.Missions
- usings: UnityEngine
- unity: none; unity_score=3
- ksp refs: none; fan_in=14 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/LaunchSiteSituation.cs
- 261 lines; kind=class; type=LaunchSiteSituation; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: CommNet, Expansions.Missions.Editor, Expansions.Missions.Scenery.Scripts, KSP.Localization, UnityEngine
- unity: Coroutine=8, GetComponent=4, GameObject=4, Transform=3, Instantiate/Destroy=1; unity_score=39
- ksp refs: ConfigNode=5, Planetarium=3, Localizer=3, Vector3d=1, HighLogic=1; fan_in=2 fan_out=22
- methods(6): AddLaunchSite, Load, RemoveLaunchSite, Save, createEmptyLaunchSite, createLaunchSiteObject
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MEGUINodeIconGroup.cs
- 89 lines; kind=MonoBehaviour; type=MEGUINodeIconGroup; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=5, SetActive=4, GameObject=4, Texture=2, Lifecycle=2, TMPro=1; unity_score=86
- ksp refs: none; fan_in=1 fan_out=2
- methods(6): Awake, CollapseGroup, Create, OnButtonCollapsePressed, OnDestroy, SetHeaderColor
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MENode.cs **MUST-OPEN (>800 lines)**
- 1255 lines; kind=MonoBehaviour; type=MENode; bases=MonoBehaviour,IConfigNode,IMEFlowBlock,IDiscoverable; ancestry=MonoBehaviour; ns=Expansions.Missions
- usings: Expansions.Missions.Actions, Expansions.Missions.Editor, Expansions.Missions.Flow, Expansions.Missions.Runtime, Expansions.Missions.Tests, KSP.Localization, UnityEngine
- unity: GameObject=8, GetComponent=5, Lifecycle=3, Transform=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=30
- ksp refs: ConfigNode=27, HighLogic=6, GameEvents=4, OrbitDriver=3, Orbit=3, Localizer=2; fan_in=176 fan_out=54
- methods(40): ActionsContainModule, ActivateNode, Activated, AddAction, AddParameterToNodeBody, AddParametersDisplayedInNodeBody, AddParametersDisplayedInSAP, Awake, CatchAllControlCreated, ClearTestGroups, Clone, DeactivateNode, Deactivated, GetAllActionModules, GetNodeLocationInWorld, HasNodeBodyParameter, HasTestOrbit, InitializeTestGroups, Load, OnDestroy, OnLevelLoaded, RemoveParameterFromNodeBody, RevealAltitude, RevealDisplayName, RevealMass, RevealName, RevealSituationString, RevealSpeed, RevealType, RunTests, RunValidation, RunValidationWrapper, Save, SetCatchAllNode, SetEndNode, SetNodeColor, Spawn, Start, ToggleCatchAllNode, UpdateMissionFlowUI
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MEScoreModule.cs
- 29 lines; kind=attribute; type=MEScoreModule; bases=Attribute; ancestry=Attribute; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=42 fan_out=0
- methods(1): IsModuleAllowed
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Mission.cs **MUST-OPEN (>800 lines)**
- 2705 lines; kind=MonoBehaviour; type=Mission; bases=MonoBehaviour,IConfigNode; ancestry=MonoBehaviour; ns=Expansions.Missions
- usings: Expansions.Missions.Actions, Expansions.Missions.Editor, Expansions.Missions.Flow, Expansions.Missions.Runtime, Expansions.Missions.Tests, KSP.Localization, UnityEngine
- unity: GameObject=9, Transform=7, Coroutine=6, Analytics=3, Lifecycle=2, Steam=1; unity_score=26
- ksp refs: HighLogic=41, ConfigNode=29, GameEvents=27, Localizer=21, FlightGlobals=13, Game=10; fan_in=83 fan_out=82
- methods(83): AddMappedCraft, AddMappedPartVesselName, Awake, BlockPlayMission, CheckHash, ClearCraftFiles, ClearMappedVessels, ContinueCriticalError, CurrentVesselID, DoEndMissionActions, Export, GenerateMissionLaunchSites, GetActionCreateAsteroid, GetActionCreateComet, GetActionCreateFlag, GetActionCreateFlagByPersistentID, GetActionCreateKerbal, GetActionCreateVessel, GetAllActionModules, GetAllTestModules, GetAllVesselSituations, GetAllVesselSituationsGuid, GetAsteroidByPersistentID, GetBanner, GetCometByPersistentID, GetCraftBySituationsVesselID, GetCraftFileIndexByPart, GetGameObjectName, GetMissionCraftByFileName, GetMissionCraftByName, GetMissionSituationByCraftFileName, GetNodeById, GetNodeGuidByVesselID, GetSituationsIndexByPart, GetSituationsIndexByVessel, GetVesselSituationByVesselID, InitMission, InitSeed, IsNextObjective, Load, MissionCriticalError, MissionHasLaunchSite, OnDestroy, OnEditorStarted, OnGameSceneSwitchRequested ...
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionAwards.cs
- 116 lines; kind=class; type=MissionAwards; bases=DynamicModuleList; ancestry=DynamicModuleList/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, Expansions.Missions.Runtime, UnityEngine
- unity: none; unity_score=6
- ksp refs: MissionSystem=1, HighLogic=1; fan_in=2 fan_out=11
- methods(6): ContainsDefinition, EvaluateAwards, FillAwardsList, GetSupportedTypes, StartTracking, StopTracking
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionBlackListPreFlightCheck.cs
- 95 lines; kind=class; type=MissionBlackListPreFlightCheck; bases=IPreFlightTest; ancestry=IPreFlightTest; ns=Expansions.Missions
- usings: Expansions.Missions.Runtime, KSP.Localization, PreFlightTests
- unity: none; unity_score=0
- ksp refs: AvailablePart=4, Localizer=3, MissionSystem=2, ShipConstruct=1, PartLoader=1; fan_in=1 fan_out=8
- methods(5): GetAbortOption, GetProceedOption, GetWarningDescription, GetWarningTitle, Test
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionCameraLockOptions.cs
- 16 lines; kind=enum; type=MissionCameraLockOptions; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionCameraModeOptions.cs
- 14 lines; kind=enum; type=MissionCameraModeOptions; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionCelestialBody.cs
- 120 lines; kind=class; type=MissionCelestialBody; bases=-; ancestry=-; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: FlightGlobals=6, CelestialBody=4, Localizer=3, ConfigNode=2; fan_in=15 fan_out=6
- methods(5): GetNodeBodyParameterString, IsValid, Load, Save, SetCelestialBodies
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionCheckpointValidator.cs
- 193 lines; kind=class; type=MissionCheckpointValidator; bases=-; ancestry=-; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, Expansions.Missions.Flow, UnityEngine
- unity: none; unity_score=2
- ksp refs: none; fan_in=6 fan_out=5
- methods(7): CheckCheckpointPathRecursive, CompareAnyLists, CompareObjectLists, CompareStructLists, IsCheckpointNodeDirty, ModulesAreEqual, ValidateCheckpoint
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionCraft.cs
- 183 lines; kind=class; type=MissionCraft; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=2
- ksp refs: ConfigNode=10, FlightGlobals=3, PartLoader=1; fan_in=13 fan_out=4
- methods(5): Clear, GetPartName, GetPartNameInVessel, GetPartPersistentId, checkPersistentIds
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionDifficulty.cs
- 16 lines; kind=enum; type=MissionDifficulty; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionDifficultyWithColor.cs
- 16 lines; kind=enum; type=MissionDifficultyWithColor; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionEndOptions.cs
- 12 lines; kind=enum; type=MissionEndOptions; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionFileInfo.cs
- 309 lines; kind=class; type=MissionFileInfo; bases=-; ancestry=-; ns=Expansions.Missions
- usings: Expansions.Missions.Runtime, KSP.UI.Screens, Steamworks, UnityEngine
- unity: Steam=3; unity_score=15
- ksp refs: MissionSystem=4, ConfigNode=4, HighLogic=1; fan_in=11 fan_out=19
- methods(6): CreateFromPath, GetSteamFileId, IsCompatible, IsCraftCompatible, IsIndividualFolderCraftCompatible, UpdateSteamState
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionFlow.cs
- 42 lines; kind=class; type=MissionFlow; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Flow
- unity: none; unity_score=0
- ksp refs: ConfigNode=5; fan_in=2 fan_out=7
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionImporting.cs
- 142 lines; kind=class; type=MissionImporting; bases=-; ancestry=-; ns=Expansions.Missions
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=22
- ksp refs: Localizer=14, HighLogic=2, GameEvents=1; fan_in=3 fan_out=13
- methods(4): ImportNewZips, ImportZip, ImportZipConfirmed, MoveImportedFile
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionInstructor.cs
- 133 lines; kind=class; type=MissionInstructor; bases=-; ancestry=-; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=24, ConfigNode=2; fan_in=3 fan_out=9
- methods(7): Equals, GetHashCode, GetNodeBodyParameterString, Load, Save, SetInstructors, UpdateNodeBodyUI
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionKerbal.cs
- 367 lines; kind=class; type=MissionKerbal; bases=-; ancestry=-; ns=Expansions.Missions
- usings: Expansions.Missions.Actions, Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=1
- ksp refs: ProtoCrewMember=35, Localizer=14, ConfigNode=2; fan_in=12 fan_out=10
- methods(20): Equals, GetHashCode, GetNodeBodyParameterString, Initialize, IsValid, KerbalChanged, KerbalTypeChanged, Load, OnKerbalControlCreated, RebuildDropDownOnChange, Save, SetKerbalList, SetKerbalTypeList, UpdateNodeBodyUI, onKerbalAdded, onKerbalNameChange, onKerbalRemoved, onKerbalStatusChange, onKerbalTypeChange, setcurrentKerbalAvailable
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionListGroup.cs
- 121 lines; kind=MonoBehaviour; type=MissionListGroup; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=5, SetActive=4, GameObject=4, Texture=2, Lifecycle=2, TMPro=1; unity_score=79
- ksp refs: none; fan_in=2 fan_out=4
- methods(9): AddMission, Awake, CollapseGroup, ContainsMission, Create, CreateDefault, OnButtonCollapsePressed, OnDestroy, SetHeaderColor
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionLocalizer.cs
- 34 lines; kind=class; type=MissionLocalizer; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=5; fan_in=1 fan_out=3
- methods(1): LoadMissionLocalizationFiles
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionMappedVessel.cs
- 88 lines; kind=class; type=MissionMappedVessel; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=4
- ksp refs: ConfigNode=7, FlightGlobals=1; fan_in=4 fan_out=2
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionMappedVesselsExtension.cs
- 141 lines; kind=static class; type=MissionMappedVesselsExtension; bases=-; ancestry=-; ns=Expansions.Missions
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=0 fan_out=2
- methods(7): ConvertMappedId, Exists, Find, IndexCurrentVesselId, IndexMappedVesselId, IndexPartId, MappedVesselName
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionOrbit.cs
- 341 lines; kind=class; type=MissionOrbit; bases=OrbitSnapshot; ancestry=OrbitSnapshot; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Orbit=25, FlightGlobals=6, ConfigNode=4, CelestialBody=3, Planetarium=1, Localizer=1; fan_in=12 fan_out=15
- methods(10): Equals, GetEditorMaxOrbitRadius, GetHashCode, GetNodeBodyParameterString, GetUTOrbitRotation, Load, RelativeOrbit, Reset, Save, SetCelestialBodies
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionOrbitRenderer.cs
- 66 lines; kind=MonoBehaviour; type=MissionOrbitRenderer; bases=OrbitTargetRenderer; ancestry=OrbitTargetRenderer/OrbitRenderer/OrbitRendererBase/MonoBehaviour; ns=Expansions.Missions
- usings: KSP.Localization, KSP.UI.Screens.Mapview
- unity: none; unity_score=15
- ksp refs: Localizer=12, Vessel=1, Orbit=1; fan_in=3 fan_out=8
- methods(5): Setup, apNode_OnUpdateCaption, ascNode_OnUpdateCaption, descNode_OnUpdateCaption, peNode_OnUpdateCaption
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionPack.cs
- 73 lines; kind=class; type=MissionPack; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: KSP.Localization
- unity: none; unity_score=5
- ksp refs: Localizer=3, ConfigNode=2; fan_in=5 fan_out=4
- methods(4): CompareDisplayName, CompareOrder, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionParamsExtras.cs
- 90 lines; kind=class; type=MissionParamsExtras; bases=GameParameters.CustomParameterNode; ancestry=CustomParameterNode/ParameterNode/IConfigNode; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=4
- ksp refs: none; fan_in=5 fan_out=1
- methods(2): Interactible, SetDifficultyPreset
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionParamsFacilities.cs
- 98 lines; kind=class; type=MissionParamsFacilities; bases=GameParameters.CustomParameterNode; ancestry=CustomParameterNode/ParameterNode/IConfigNode; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=4
- ksp refs: none; fan_in=2 fan_out=1
- methods(1): SetDifficultyPreset
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionParamsGeneral.cs
- 104 lines; kind=class; type=MissionParamsGeneral; bases=GameParameters.CustomParameterNode; ancestry=CustomParameterNode/ParameterNode/IConfigNode; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=4
- ksp refs: none; fan_in=8 fan_out=1
- methods(2): Interactible, SetDifficultyPreset
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionPartFilter.cs
- 300 lines; kind=class; type=MissionPartFilter; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: none; unity_score=1
- ksp refs: Localizer=3, AvailablePart=3, ConfigNode=2, PartLoader=1; fan_in=2 fan_out=15
- methods(15): ApplyPartsFilter, CheckPartExcludelist, GetExcludedParts, GetExcludedPartsFilterDictionary, Load, OnAvailablePartsCheckpointValidator, OnAvailablePartsParameterCreated, OnFilterValueChange, OnPartPickerNeedsToUpdateExcludedPartsFilter, OnUnavailablePartsCheckpointValidator, OnUnavailablePartsParameterCreated, RemovePartsFilter, Save, SetUpdatePartnerExcludedPartFilterCallback, UpdateExcludedPartsFilter
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionPreflightCheck.cs
- 40 lines; kind=class; type=MissionPreflightCheck; bases=IPreFlightTest; ancestry=IPreFlightTest; ns=Expansions.Missions
- usings: KSP.Localization, PreFlightTests
- unity: none; unity_score=0
- ksp refs: Localizer=3; fan_in=1 fan_out=3
- methods(5): GetAbortOption, GetProceedOption, GetWarningDescription, GetWarningTitle, Test
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionQuaternion.cs
- 83 lines; kind=class; type=MissionQuaternion; bases=-; ancestry=-; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, UnityEngine
- unity: none; unity_score=7
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionScore.cs
- 124 lines; kind=class; type=MissionScore; bases=DynamicModuleList; ancestry=DynamicModuleList/IConfigNode; ns=Expansions.Missions
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=6
- ksp refs: Localizer=1; fan_in=5 fan_out=9
- methods(6): AddGlobalScoreModule, AwardScore, AwarededScoreDescription, GetSupportedTypes, ScoreDescription, SetDefaultGlobalScoreModules
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionScoreInfo.cs
- 187 lines; kind=class; type=MissionScoreInfo; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=11, HighLogic=1; fan_in=2 fan_out=8
- methods(11): AddScore, CompareScoreInfo, DeleteMissionScores, DeleteScore, GetMissionAwards, GetMissionScore, GetMissionScores, Load, LoadScores, Save, SaveScores
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionSituation.cs **MUST-OPEN (>800 lines)**
- 889 lines; kind=class; type=MissionSituation; bases=IConfigNode,IMENodeDisplay; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Actions, Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: SetActive=1, Instantiate/Destroy=1, GameObject=1; unity_score=5
- ksp refs: Localizer=19, ConfigNode=19, HighLogic=6, Game=6, KerbalRoster=4, GameEvents=1; fan_in=15 fan_out=38
- methods(43): AddNodeWithVesselToBuild, AddParameterToNodeBody, AddParameterToNodeBodyAndUpdateUI, AddParameterToSAP, Destroy, DestroySituation, FillVesselSituationList, GetAvailableVesselSituations, GetDisplayName, GetInfo, GetInternalParametersToDisplay, GetName, GetNode, GetNodeBodyParameterString, GetVesselSituationByVesselID, HasNodeBodyParameter, HasSAPParameter, InitSituation, Initialize, Load, OnAutoPopulateCrewValueChange, OnMissionRosterClick, OnResourceROCMissionSeedChanged, OnResourceSeedChanged, OnShowBriefingChange, OninstructorCreated, ParameterSetupComplete, RemoveLaunchedVessels, RemoveParameterFromNodeBody, RemoveParameterFromNodeBodyAndUpdateUI, RemoveParameterFromSAP, RunValidation, RunValidationWrapper, Save, SaveVesselsToBuild, UpdateNodeBodyUI, UpdateVesselBuildValues, VesselSituationLaunched, VesselSituationListContains, VesselSituationReadyToLaunch, VesselSituationRevertLaunch, resetVesselsCountsLists, setStartType
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionsUtils.cs
- 664 lines; kind=static class; type=MissionsUtils; bases=-; ancestry=-; ns=Expansions.Missions
- usings: Expansions.Missions.Adjusters, Expansions.Missions.Editor, Expansions.Missions.Flow, Expansions.Missions.Runtime, KSP.Localization, Steamworks, UnityEngine
- unity: GameObject=12, Steam=6, Texture=5, Transform=2, GetComponent=1; unity_score=28
- ksp refs: HighLogic=10, ConfigNode=6, Localizer=4, GameSettings=3, MissionSystem=1, GameDatabase=1; fan_in=57 fan_out=29
- methods(29): AddEdge, AddMissionPackFromNode, AddMissionsScenarios, AddVertex, BreadthSearch, GatherMissionFiles, GatherMissionPacks, GetBaseMissionsPath, GetFacilityLimit, GetFieldValueForDisplay, GetMissionsFolder, GetNodesOrdered, GetSteamWorkshopMissions, GetStockMissionsPath, GetTextureInExternalPath, GetUsersMissionsPath, InitialiseAdjusterTypes, InstantiateMEFlowParser, MEPrefab, METexture, OpenMissionBuilder, ParseMissionPacksCFG, RebuildPacksList, RemoveMissionSaves, SortMissions, UpdatePartName, UpdatePartNames, VerifyUsersMissionsFolder, onSteamQueryItemsCallback
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionTypes.cs
- 10 lines; kind=enum; type=MissionTypes; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=10 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/NodeLabel.cs
- 16 lines; kind=MonoBehaviour; type=NodeLabel; bases=BaseLabel; ancestry=BaseLabel/MonoBehaviour; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=2 fan_out=4
- methods(1): Setup
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ParamChoices_CelestialBodySurface.cs
- 149 lines; kind=class; type=ParamChoices_CelestialBodySurface; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, FinePrint, KSP.Localization, UnityEngine
- unity: none; unity_score=1
- ksp refs: ConfigNode=4, Localizer=3; fan_in=10 fan_out=13
- methods(9): GetExtraText, GetWaypoint, GetWorldPosition, HasNodeLabel, HasWaypoint, HasWorldPosition, Load, NodeBodyParameterString, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ParamChoices_VesselSimpleLocation.cs
- 65 lines; kind=class; type=ParamChoices_VesselSimpleLocation; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor
- unity: none; unity_score=0
- ksp refs: ConfigNode=4; fan_in=6 fan_out=7
- methods(3): GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MessageNodeDialog.cs
- 389 lines; kind=MonoBehaviour; type=MessageNodeDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Runtime
- usings: Expansions.Missions.Actions, KSP.UI, KSP.UI.Screens, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=28, SetActive=15, UnityEngine.UI=8, Transform=7, GetComponent=7, Instantiate/Destroy=6; unity_score=83
- ksp refs: HighLogic=3, Planetarium=2, GameEvents=2; fan_in=1 fan_out=12
- methods(10): AssignInstructorPosition, Awake, ClearInstructorPosition, CloseDialog, OnDestroy, OnSceneChange, Spawn, Start, Update, onClickContinueBtn
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionAppMode.cs
- 9 lines; kind=enum; type=MissionAppMode; bases=-; ancestry=-; ns=Expansions.Missions.Runtime
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionEndDialog.cs
- 323 lines; kind=MonoBehaviour; type=MissionEndDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Runtime
- usings: Expansions.Missions.Editor, KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: SetActive=14, GameObject=14, TMPro=10, UnityEngine.UI=8, GetComponent=7, Lifecycle=3; unity_score=82
- ksp refs: HighLogic=13, Localizer=6, FlightGlobals=4, MissionSystem=2, InputLockManager=2, ConfigNode=2; fan_in=1 fan_out=31
- methods(12): Awake, Close, Display, DisplayDetails, DisplayScore, OnDestroy, Restart, Revert, SaveAndQuit, SetupGUI, Update, drawRevertOptions
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionPlayDialog.cs **MUST-OPEN (>800 lines)**
- 1893 lines; kind=MonoBehaviour; type=MissionPlayDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Runtime
- usings: Expansions.Missions.Editor, Expansions.Missions.Flow, KSP.Localization, KSP.UI.Screens, KSP.UI.Util, Steamworks, TMPro, UnityEngine, UnityEngine.Networking, UnityEngine.UI
- unity: SetActive=57, GameObject=51, UnityEngine.UI=28, Steam=23, TMPro=17, Transform=15; unity_score=73
- ksp refs: Localizer=49, GameSettings=12, MissionSystem=10, ConfigNode=10, GameEvents=4, InputLockManager=2; fan_in=5 fan_out=63
- methods(60): AddTutorialLock, BuildList, BuildPacksList, BuildSteamList, CancelLoadGame, ClearListItems, ClearPacksList, ConfirmLoadGame, ConfirmRestart, Create, CreateSteamWidget, CreateWidget, DismissRestartGame, DisplayAddedMissions, FinishedCallback, GetLastWriteTime, GetSFSMD5, Load, LoadDetailsFromMission, LoadFromMetaFile, LoadSteamItemPreviewURL, OnBtnCancel, OnBtnSteamOverlay, OnButtonCreate, OnButtonEdit, OnButtonLoad, OnButtonRestart, OnCloseWindow, OnGameSettingsWritten, OnMissionLoadDismiss, OnSelectionChanged, OnTabCommunity, OnTabSteam, OnTabStock, OnTabStockInCode, RemoveTutorialLock, Save, SaveMissionSteamUnsubscribeWarning, SaveToMetaFile, SetHidden, SetLocked, Start, SteamItemSubscribed, Terminate, Update ...
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionsApp.cs **MUST-OPEN (>800 lines)**
- 1452 lines; kind=MonoBehaviour; type=MissionsApp; bases=UIApp; ancestry=UIApp/MonoBehaviour; ns=Expansions.Missions.Runtime
- usings: Expansions.Missions.Actions, Expansions.Missions.Flow, KSP.Localization, KSP.UI, KSP.UI.Screens, KSP.UI.TooltipTypes, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=26, Coroutine=20, GetComponent=18, SetActive=15, Transform=11, Instantiate/Destroy=9; unity_score=56
- ksp refs: HighLogic=37, MissionSystem=26, EditorLogic=23, Localizer=22, GameEvents=19, ConfigNode=8; fan_in=6 fan_out=61
- methods(63): AddMission, AddVesselSubParameter, CheckforVesselstoBuild, CreateItem, CreateMissionItem, CreateMissionsList, CreateObjectivesList, CreateTestModeSettingsItem, CreateTestModeSettingsList, CreateVesselDetails, CreateVesselsList, DisplayApp, EditorVesselCompleted, ExitEditor, FireCreateNextVessel, GetAppScenes, GetAppScreenPos, GetColoredMissionHeader, GetColoredObjectiveHeader, GetVesselListAppVesselInfo, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnCreateVesselWarningOK, OnMissionsLoaded, RefreshCurrentVesselToBuild, RefreshMission, RefreshMissionParameter, RefreshMissionRequest, RefreshMissionRequested, RefreshVesselsToCreate, RemoveMission, ResetVesselsCountsLists, SetAppIconState, SetVesselItemReadyIcon, Start, StayOnEditor, UpdateLauncherButtonPlayAnim, UpdateLauncherButtonStopAnim, UpdateMissionsDaemon, UpdateVesselsDaemon, buildStartedMissionVessels, changeVesselListState, changeVessels ...
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionsAppObjectiveInfo.cs
- 23 lines; kind=class; type=MissionsAppObjectiveInfo; bases=-; ancestry=-; ns=Expansions.Missions.Runtime
- usings: KSP.UI.TooltipTypes, TMPro, UnityEngine.UI
- unity: UnityEngine.UI=2, TMPro=2; unity_score=62
- ksp refs: none; fan_in=0 fan_out=4
- methods(0): 
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionsAppVesselInfo.cs
- 28 lines; kind=class; type=MissionsAppVesselInfo; bases=-; ancestry=-; ns=Expansions.Missions.Runtime
- usings: KSP.UI, KSP.UI.TooltipTypes, TMPro, UnityEngine.UI
- unity: UnityEngine.UI=2, TMPro=1; unity_score=55
- ksp refs: none; fan_in=3 fan_out=5
- methods(0): 
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionSteamItemWidget.cs
- 72 lines; kind=MonoBehaviour; type=MissionSteamItemWidget; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Runtime
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=5, UnityEngine.UI=3, Texture=2, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1; unity_score=86
- ksp refs: none; fan_in=1 fan_out=3
- methods(4): Create, SetHeaderText, SetSubIconState, Start
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionSystem.cs **MUST-OPEN (>800 lines)**
- 1809 lines; kind=ScenarioModule; type=MissionSystem; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=Expansions.Missions.Runtime
- usings: Expansions.Missions.Actions, Expansions.Missions.Editor, KSP.Localization, KSP.UI.Screens, ModuleWheels, UnityEngine, Upgradeables
- unity: Coroutine=46, GameObject=43, Transform=22, GetComponent=11, Instantiate/Destroy=9, Renderer=4; unity_score=59
- ksp refs: HighLogic=99, ConfigNode=31, Planetarium=21, GameEvents=14, Vessel=11, Localizer=11; fan_in=40 fan_out=85
- methods(43): AddMission, AllowCameraSwitch, CheckFireBriefingDialog, ClearCameraModeAndLocks, ConstructShip, CreateCheckpoint, DisplayMissionEndMessage, EnforceCameraModeAndLocks, GenerateBriefingDialogNode, GetFlightStateActiveBody, HasFinishedMission, LoadMetaMission, LoadMission, OnAwake, OnDestroy, OnLoad, OnLoadRoutine, OnSave, ReactivateComponents, RemoveMissionObjects, RemoveMissonObject, ResetKSCMarkers, SetLockedCamera, SetupCraftSaveFiles, SetupFlight, SetupMissionGame, SetupMissionParameters, ToggleVesselComponents, Update, UpdateLockedCameraSettings, UpdateMissions, ValidateCrewAssignments, disableOverride, extractmissionNode, onGameSceneLoadRequested, onMissionEnded, onMissionNodeChanged, onMissionStarted, onVesselLoaded, putShiptoGround, setBodyRotation, setVesselOrbit, setVesselOrbitPosition
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Scenery/Scripts/PositionMobileLaunchPad.cs
- 415 lines; kind=MonoBehaviour; type=PositionMobileLaunchPad; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Scenery.Scripts
- usings: UnityEngine
- unity: Transform=28, Material=9, GameObject=5, Collider=5, SetActive=4, Raycast/Query=3; unity_score=77
- ksp refs: HighLogic=4, Planetarium=3, Vessel=2, GameEvents=2, Vector3d=1, ProtoVessel=1; fan_in=4 fan_out=9
- methods(15): Awake, CheckForVessels, CheckGrounded, CompleteOrientation, CreatePoints, ExtendLegs, FindCollider, MakePoint, MoveModel, OnDestroy, OnScenerySettingChanged, ResetLegs, ResetPositioning, ToggleObjects, ToggleRampObjects
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreInfo.cs
- 66 lines; kind=class; type=ScoreInfo; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=2 fan_out=2
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreModifierType.cs
- 16 lines; kind=enum; type=ScoreModifierType; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreModule.cs
- 65 lines; kind=class; type=ScoreModule; bases=DynamicModule; ancestry=DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor
- unity: none; unity_score=0
- ksp refs: ConfigNode=2; fan_in=6 fan_out=5
- methods(6): AwardScore, AwarededScoreDescription, GetNodeBodyParameterString, Load, Save, ScoreDescription
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreModule_Accuracy.cs
- 123 lines; kind=class; type=ScoreModule_Accuracy; bases=ScoreModule; ancestry=ScoreModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=5, ConfigNode=5; fan_in=9 fan_out=8
- methods(10): AwardScore, Equals, GetDefaultPinnedParameters, GetDisplayName, GetHashCode, GetInfo, GetNodeBodyParameterString, Load, Save, ScoreDescription
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreModule_Completion.cs
- 85 lines; kind=class; type=ScoreModule_Completion; bases=ScoreModule; ancestry=ScoreModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=4, ConfigNode=2; fan_in=3 fan_out=6
- methods(10): AwardScore, Equals, GetDefaultPinnedParameters, GetDisplayName, GetDisplayToolTip, GetHashCode, GetInfo, Load, Save, ScoreDescription
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreModule_Modifier.cs
- 124 lines; kind=class; type=ScoreModule_Modifier; bases=ScoreModule; ancestry=ScoreModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=10, ConfigNode=2; fan_in=2 fan_out=9
- methods(11): AwardScore, Equals, GetDefaultPinnedParameters, GetDisplayName, GetDisplayToolTip, GetHashCode, GetInfo, GetNodeBodyParameterString, Load, Save, ScoreDescription
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreModule_Resource.cs
- 145 lines; kind=class; type=ScoreModule_Resource; bases=ScoreModule; ancestry=ScoreModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=5, ConfigNode=5, PartResourceDefinition=3, Vessel=2, FlightGlobals=1; fan_in=34 fan_out=14
- methods(11): AwardScore, Equals, GetDefaultPinnedParameters, GetDisplayName, GetHashCode, GetInfo, GetNodeBodyParameterString, Load, Save, ScoreDescription, SetResourceNames
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreModule_Time.cs
- 126 lines; kind=class; type=ScoreModule_Time; bases=ScoreModule; ancestry=ScoreModule/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=5, ConfigNode=5, Planetarium=1; fan_in=34 fan_out=10
- methods(10): AwardScore, Equals, GetDefaultPinnedParameters, GetDisplayName, GetHashCode, GetInfo, GetNodeBodyParameterString, Load, Save, ScoreDescription
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreRange.cs
- 61 lines; kind=class; type=ScoreRange; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: UnityEngine
- unity: none; unity_score=2
- ksp refs: ConfigNode=2; fan_in=6 fan_out=3
- methods(5): Equals, GetHashCode, Load, Save, isValueInRange
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/SteamMissionFileInfo.cs
- 85 lines; kind=class; type=SteamMissionFileInfo; bases=-; ancestry=-; ns=Expansions.Missions
- usings: KSP.Localization, Steamworks
- unity: Steam=1; unity_score=12
- ksp refs: ConfigNode=6, Localizer=1; fan_in=2 fan_out=6
- methods(3): CreateMetaData, ProcessMetaData, SetMissionFileInfo
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/SurfaceArea.cs
- 177 lines; kind=class; type=SurfaceArea; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, FinePrint, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=3
- ksp refs: FlightGlobals=8, Localizer=3, ConfigNode=3, CelestialBody=2; fan_in=11 fan_out=11
- methods(14): Equals, GetExtraText, GetHashCode, GetNodeBodyParameterString, GetWaypoint, GetWorldPosition, HasNodeLabel, HasWaypoint, HasWorldPosition, IsPointInCircle, Load, PointInCircleAccuracy, Save, SetCelestialBodies
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/SurfaceVolume.cs
- 295 lines; kind=class; type=SurfaceVolume; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, FinePrint, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=3
- ksp refs: Localizer=14, FlightGlobals=8, CelestialBody=5, ConfigNode=3, Vector3d=2; fan_in=3 fan_out=12
- methods(15): AjustToBody, Equals, GetExtraText, GetHashCode, GetNodeBodyParameterString, GetWaypoint, GetWorldPosition, HasNodeLabel, HasWaypoint, HasWorldPosition, IsInsideVolume, Load, ResetBodyToDefaultValues, Save, SetCelestialBodies
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/TestComparisonLessGreaterEqual.cs
- 14 lines; kind=enum; type=TestComparisonLessGreaterEqual; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=9 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/TestComparisonLessGreaterOnly.cs
- 12 lines; kind=enum; type=TestComparisonLessGreaterOnly; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=8 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/TestComparisonOperator.cs
- 18 lines; kind=enum; type=TestComparisonOperator; bases=-; ancestry=-; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=9 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/TestGroup.cs
- 159 lines; kind=class; type=TestGroup; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, UnityEngine
- unity: GetComponent=1, GameObject=1; unity_score=13
- ksp refs: ConfigNode=6, GameEvents=2; fan_in=19 fan_out=9
- methods(13): AddTestModule, ClearGroup, ClearTestModules, Cleared, InitializeTestModules, Initialized, Load, OnCloned, RunValidation, RunValidationWrapper, Save, Test, initializeGroup
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/TestModule.cs
- 320 lines; kind=MonoBehaviour; type=TestModule; bases=MonoBehaviour,IConfigNode,ITestModule,IMENodeDisplay; ancestry=MonoBehaviour; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, FinePrint, KSP.Localization, UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=26
- ksp refs: GameEvents=10, Localizer=5, Vessel=4, ConfigNode=2; fan_in=31 fan_out=26
- methods(40): AddParameterToNodeBody, AddParameterToNodeBodyAndUpdateUI, AddParameterToSAP, Awake, ClearTest, Cleared, GetAppObjectiveInfo, GetDisplayName, GetInfo, GetInternalParametersToDisplay, GetName, GetNode, GetNodeBodyParameterString, HasNodeBodyParameter, HasSAPParameter, Initialize, InitializeTest, Initialized, Load, OnDestroy, OnPartPersistentIdChanged, OnVesselDocking, OnVesselPersistentIdChanged, OnVesselsUndocking, ParameterSetupComplete, PartPersistentIdChangedWrapper, RemoveParameterFromNodeBody, RemoveParameterFromNodeBodyAndUpdateUI, RemoveParameterFromSAP, RunValidation, RunValidationWrapper, Save, SetupPersistentIdEvents, ShouldCreateCheckpoint, Start, Test, UpdateNodeBodyUI, VesselDockingWrapper, VesselPersistentIdChangedWrapper, VesselsUndockingWrapper
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestAccuracy.cs
- 77 lines; kind=MonoBehaviour; type=TestAccuracy; bases=TestModule; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=32
- ksp refs: Localizer=3, ConfigNode=2; fan_in=0 fan_out=10
- methods(6): Awake, GetInfo, GetNodeBodyParameterString, Load, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestCrewAssignment.cs
- 246 lines; kind=MonoBehaviour; type=TestCrewAssignment; bases=TestModule,IScoreableObjective; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=25
- ksp refs: Localizer=9, FlightGlobals=9, Vessel=3, ProtoCrewMember=3, ConfigNode=2, ProtoPartSnapshot=1; fan_in=1 fan_out=21
- methods(13): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Initialize, Load, OnPartPersistentIdChanged, OnVesselPersistentIdChanged, OnVesselsUndocking, RunValidation, Save, Test, partContainsKerbal
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestDistance.cs **MUST-OPEN (>800 lines)**
- 875 lines; kind=MonoBehaviour; type=TestDistance; bases=TestModule,IScoreableObjective; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Actions, Expansions.Missions.Editor, Expansions.Missions.Runtime, FinePrint.Utilities, KSP.Localization, TMPro, UnityEngine
- unity: SetActive=22, GameObject=22, TMPro=2, Lifecycle=1; unity_score=46
- ksp refs: Localizer=37, Vessel=21, ConfigNode=11, CelestialBody=10, Vector3d=9, FlightGlobals=9; fan_in=1 fan_out=56
- methods(32): Awake, CheckCrew, GetInfo, GetKerbalLocation, GetNodeBodyParameterString, GetScoreModifier, GetVesselLocation, Initialize, Load, OnDistanceFromKerbalControlCreated, OnDistanceFromTargetControlCreated, OnDistanceFromValueChange, OnDistanceFromVesselControlCreated, OnDistanceToAsteroidControlCreated, OnDistanceToCelestialBodyControlCreated, OnDistanceToCometControlCreated, OnDistanceToFlagControlCreated, OnDistanceToKerbalControlCreated, OnDistanceToLaunchSiteControlCreated, OnDistanceToLocationControlCreated, OnDistanceToNodeControlCreated, OnDistanceToTargetControlCreated, OnDistanceToTargetControlSetup, OnDistanceToValueChange, OnDistanceToVesselControlCreated, OnVesselPersistentIdChanged, ParameterSetupComplete, RunValidation, Save, SortDropDownItemByText, Test, TestPointsDistance
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestEmptyFalse.cs
- 23 lines; kind=MonoBehaviour; type=TestEmptyFalse; bases=TestModule; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: KSP.Localization
- unity: Lifecycle=1; unity_score=36
- ksp refs: Localizer=2; fan_in=0 fan_out=2
- methods(3): Awake, GetInfo, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestEmptyTrue.cs
- 23 lines; kind=MonoBehaviour; type=TestEmptyTrue; bases=TestModule; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: KSP.Localization
- unity: Lifecycle=1; unity_score=36
- ksp refs: Localizer=2; fan_in=0 fan_out=2
- methods(3): Awake, GetInfo, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestFlyThrough.cs
- 141 lines; kind=MonoBehaviour; type=TestFlyThrough; bases=TestVessel,IScoreableObjective,INodeWaypoint,ITestNodeLabel; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, FinePrint, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=23
- ksp refs: FlightGlobals=3, Localizer=2, ConfigNode=2; fan_in=0 fan_out=15
- methods(14): Awake, GetExtraText, GetInfo, GetNodeBodyParameterString, GetNodeWaypoint, GetScoreModifier, GetWorldPosition, HasNodeLabel, HasNodeWaypoint, HasWorldPosition, Load, OnShowLabelChanged, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestFundsRecovery.cs
- 136 lines; kind=MonoBehaviour; type=TestFundsRecovery; bases=TestModule,IScoreableObjective; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, KSP.UI.Screens
- unity: Lifecycle=1; unity_score=21
- ksp refs: Localizer=3, GameEvents=2, ConfigNode=2, ProtoVessel=1; fan_in=0 fan_out=16
- methods(10): Awake, Cleared, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Initialized, Load, Recovered, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestGameEvent.cs
- 55 lines; kind=MonoBehaviour; type=TestGameEvent; bases=TestModule,IScoreableObjective; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: -
- unity: Lifecycle=1; unity_score=28
- ksp refs: ConfigNode=2, GameEvents=1; fan_in=0 fan_out=5
- methods(7): Awake, EventCallback_StageSeparation, GetScoreModifier, Load, Save, SuscribeToEvent, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestGoTo.cs
- 161 lines; kind=MonoBehaviour; type=TestGoTo; bases=TestVessel,IScoreableObjective,INodeWaypoint,ITestNodeLabel; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, FinePrint, KSP.Localization, UnityEngine
- unity: SetActive=2, GameObject=2, SendMessage/Invoke=1, Lifecycle=1; unity_score=40
- ksp refs: FlightGlobals=4, Vessel=2, Localizer=2, ConfigNode=2; fan_in=0 fan_out=19
- methods(16): Awake, GetExtraText, GetInfo, GetNodeBodyParameterString, GetNodeWaypoint, GetScoreModifier, GetWorldPosition, HasNodeLabel, HasNodeWaypoint, HasWorldPosition, Load, OnIgnoreSurfaceVelocity, OnSurfaceVelocityControlCreated, OnSurfaceVelocitySetState, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestGrapple.cs
- 434 lines; kind=MonoBehaviour; type=TestGrapple; bases=TestModule; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: SetActive=6, GameObject=6, Lifecycle=1; unity_score=37
- ksp refs: Localizer=18, FlightGlobals=14, GameEvents=9, Vessel=6, Part=5, ConfigNode=4; fan_in=0 fan_out=28
- methods(22): Awake, Cleared, GetInfo, GetNodeBodyParameterString, Initialized, Load, OnAsteroidControlCreated, OnCometControlCreated, OnSpaceObjectChoiceControlCreated, OnSpaceObjectChoiceControlSetup, OnSpaceObjectChoiceValueChange, OnVesselControlCreated, OnVesselDocking, OnVesselPersistentIdChanged, ParameterSetupComplete, RunValidation, Save, SetDockedUndockedDropdown, Test, onPartCouple, onPartUndock, testPartVessels
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestKerbalBoarded.cs
- 261 lines; kind=MonoBehaviour; type=TestKerbalBoarded; bases=TestModule,IScoreableObjective,INodeWaypoint,ITestNodeLabel; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, FinePrint, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=18
- ksp refs: FlightGlobals=8, Localizer=7, GameEvents=5, ProtoCrewMember=3, ConfigNode=2, Vessel=1; fan_in=0 fan_out=30
- methods(21): Awake, CheckSituation, Cleared, GetExtraText, GetInfo, GetNodeBodyParameterString, GetNodeWaypoint, GetScoreModifier, GetWorldPosition, HasNodeLabel, HasNodeWaypoint, HasWorldPosition, Initialize, Initialized, Load, OnBoarded, OnSeated, OnVesselPersistentIdChanged, RunValidation, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestKerbalEVA.cs
- 244 lines; kind=MonoBehaviour; type=TestKerbalEVA; bases=TestModule,IScoreableObjective,INodeWaypoint,ITestNodeLabel; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, FinePrint, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=19
- ksp refs: FlightGlobals=8, Localizer=7, GameEvents=3, ProtoCrewMember=2, Part=2, ConfigNode=2; fan_in=0 fan_out=29
- methods(20): Awake, Cleared, GetExtraText, GetInfo, GetNodeBodyParameterString, GetNodeWaypoint, GetScoreModifier, GetWorldPosition, HasNodeLabel, HasNodeWaypoint, HasWorldPosition, Initialize, Initialized, Load, OnEVA, OnVesselPersistentIdChanged, RunValidation, Save, ShouldCreateCheckpoint, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestKerbalKilled.cs
- 103 lines; kind=MonoBehaviour; type=TestKerbalKilled; bases=TestModule; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=22
- ksp refs: ProtoCrewMember=5, Localizer=3, HighLogic=2, GameEvents=2, ConfigNode=2; fan_in=0 fan_out=13
- methods(11): Awake, Cleared, GetInfo, GetNodeBodyParameterString, Initialize, Initialized, Load, OnKilled, RunValidation, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestKerbalRecovery.cs
- 141 lines; kind=MonoBehaviour; type=TestKerbalRecovery; bases=TestModule,IScoreableObjective; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=20
- ksp refs: ProtoCrewMember=5, Localizer=3, HighLogic=2, GameEvents=2, ConfigNode=2, ProtoVessel=1; fan_in=0 fan_out=18
- methods(13): Awake, Cleared, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Initialize, Initialized, Load, OnRecovered, RunValidation, Save, Test, checkAlreadyRecovered
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestKerbalRescue.cs
- 169 lines; kind=MonoBehaviour; type=TestKerbalRescue; bases=TestModule,IScoreableObjective; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=20
- ksp refs: GameEvents=9, ProtoCrewMember=6, Vessel=3, Part=3, Localizer=3, FlightGlobals=3; fan_in=0 fan_out=22
- methods(15): Awake, CheckNewKerbal, Cleared, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Initialize, Initialized, Load, OnCrewBoardVessel, OnCrewTransferred, RunValidation, Save, Test, onKnowledgeChanged
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestMETTime.cs
- 83 lines; kind=MonoBehaviour; type=TestMETTime; bases=TestVessel,IScoreableObjective; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=24
- ksp refs: Localizer=2, ConfigNode=2, FlightGlobals=1; fan_in=0 fan_out=13
- methods(7): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestMissionFunds.cs
- 110 lines; kind=MonoBehaviour; type=TestMissionFunds; bases=TestModule,IScoreableObjective; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=25
- ksp refs: Funding=6, Localizer=3, ConfigNode=2; fan_in=0 fan_out=10
- methods(7): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestMissionScore.cs
- 106 lines; kind=MonoBehaviour; type=TestMissionScore; bases=TestModule,IScoreableObjective; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=25
- ksp refs: Localizer=3, ConfigNode=2; fan_in=0 fan_out=9
- methods(7): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestMissionTime.cs
- 80 lines; kind=MonoBehaviour; type=TestMissionTime; bases=TestModule,IScoreableObjective; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=24
- ksp refs: Planetarium=2, Localizer=2, ConfigNode=2, FlightGlobals=1; fan_in=0 fan_out=14
- methods(7): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestOrbit.cs
- 241 lines; kind=MonoBehaviour; type=TestOrbit; bases=TestVessel,IScoreableObjective,INodeOrbit; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: Coroutine=4, Lifecycle=1; unity_score=30
- ksp refs: Localizer=20, FlightGlobals=9, Orbit=5, HighLogic=4, ConfigNode=4, Planetarium=2; fan_in=1 fan_out=24
- methods(15): Awake, Cleared, CreateOrbitRenderer, DrawOrbit, GetInfo, GetNodeBodyParameterString, GetNodeOrbit, GetScoreModifier, HasNodeOrbit, Initialized, Load, OnOrbitAccuracyChanged, Save, Test, initializeDrawOrbit
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestOrbitParams.cs
- 684 lines; kind=MonoBehaviour; type=TestOrbitParams; bases=TestVessel,IScoreableObjective; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: SetActive=8, GameObject=8, Lifecycle=2; unity_score=34
- ksp refs: Localizer=11, Vessel=4, FlightGlobals=4, CelestialBody=3, Planetarium=2, Orbit=2; fan_in=0 fan_out=24
- methods(39): Awake, Cleared, GetInfo, GetMaxAp, GetNodeBodyParameterString, GetScoreModifier, Initialized, Load, OnBodyChanged, OnDestroy, OnInclinationAccuracyChanged, OnLANAccuracyChanged, ParameterSetupComplete, RunValidation, Save, Test, onApMaxValueChanged, onApMinValueChanged, onApoapsisMaxCreated, onApoapsisMinCreated, onBodyCreated, onCheckApoapsisValueChanged, onCheckIncValueChanged, onCheckLANValueChanged, onCheckPeriapsisValueChanged, onIncValueChanged, onInclinationAccuracyCreated, onInclinationCreated, onLANAccuracyCreated, onLANCreated, onLANValueChanged, onPeMaxValueChanged, onPeMinValueChanged, onPeriapsisMaxCreated, onPeriapsisMinCreated, toggleApoapsisParms, toggleIncParms, togglePeriapsisParms, togglelLANParms
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestPartDocking.cs
- 375 lines; kind=MonoBehaviour; type=TestPartDocking; bases=TestModule; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=23
- ksp refs: Localizer=26, ConfigNode=10, GameEvents=9, FlightGlobals=6, Part=3, Vessel=2; fan_in=1 fan_out=18
- methods(18): Awake, Cleared, GetInfo, GetNodeBodyParameterString, Initialized, Load, OnPartPersistentIdChanged, OnVesselDocking, OnVesselPersistentIdChanged, RunValidation, Save, SetDockedUndockedDropdown, Test, findPartVesselId, onPartCouple, onPartUndock, setPlayerCreatedRootParts, testPartVessels
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestPlantFlag.cs
- 170 lines; kind=MonoBehaviour; type=TestPlantFlag; bases=TestModule,IScoreableObjective,INodeWaypoint,ITestNodeLabel,INodeBody; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, FinePrint, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=20
- ksp refs: FlightGlobals=4, Localizer=2, GameEvents=2, ConfigNode=2, CelestialBody=2, Vessel=1; fan_in=0 fan_out=23
- methods(17): AfterFlagPlanted, Awake, Cleared, GetExtraText, GetInfo, GetNodeBody, GetNodeWaypoint, GetScoreModifier, GetWorldPosition, HasNodeBody, HasNodeLabel, HasNodeWaypoint, HasWorldPosition, Initialized, Load, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestResourcesRecovery.cs
- 203 lines; kind=MonoBehaviour; type=TestResourcesRecovery; bases=TestVessel,IScoreableObjective; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Lifecycle=1; unity_score=21
- ksp refs: PartResourceDefinition=4, Localizer=4, FlightGlobals=3, ProtoPartSnapshot=2, GameEvents=2, ConfigNode=2; fan_in=0 fan_out=20
- methods(13): Awake, Cleared, CompareResource, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Initialized, Load, ParameterSetupComplete, Recovered, Save, SetResourceNames, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestScienceExperiment.cs
- 282 lines; kind=MonoBehaviour; type=TestScienceExperiment; bases=TestModule,IScoreableObjective,INodeBody; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=18
- ksp refs: Localizer=19, ResearchAndDevelopment=5, GameEvents=4, ConfigNode=4, FlightGlobals=2, ScienceSubject=1; fan_in=0 fan_out=24
- methods(19): Awake, Cleared, ConductScienceTest, GetAppObjectiveInfo, GetInfo, GetNodeBody, GetNodeBodyParameterString, GetScoreModifier, HasNodeBody, Initialized, Load, OnExperimentDeployed, OnScienceReceived, RunValidation, Save, TSEDoR_SetDropDownValues, TSEExpId_SetDropDownValues, TSEExpSit_SetDropDownValues, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestSOILeft.cs
- 99 lines; kind=MonoBehaviour; type=TestSOILeft; bases=TestVessel,IScoreableObjective,INodeBody; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=26
- ksp refs: FlightGlobals=4, GameEvents=3, Localizer=2, ConfigNode=2, CelestialBody=2, Vessel=1; fan_in=0 fan_out=14
- methods(11): Awake, Cleared, GetInfo, GetNodeBody, GetScoreModifier, HasNodeBody, Initialized, Load, OnEscapedSOI, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestSOIReached.cs
- 98 lines; kind=MonoBehaviour; type=TestSOIReached; bases=TestVessel,IScoreableObjective,INodeBody; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=26
- ksp refs: FlightGlobals=4, GameEvents=3, Localizer=2, ConfigNode=2, CelestialBody=2, Vessel=1; fan_in=0 fan_out=14
- methods(11): Awake, Cleared, GetInfo, GetNodeBody, GetScoreModifier, HasNodeBody, Initialized, Load, OnReachedSOI, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestTakeKerbal.cs
- 316 lines; kind=MonoBehaviour; type=TestTakeKerbal; bases=TestModule,IScoreableObjective,INodeWaypoint,ITestNodeLabel; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, FinePrint, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=18
- ksp refs: FlightGlobals=10, Vessel=8, Localizer=7, GameEvents=6, ProtoCrewMember=5, ConfigNode=2; fan_in=0 fan_out=28
- methods(23): Awake, CheckCrew, Cleared, GetExtraText, GetInfo, GetNodeBodyParameterString, GetNodeWaypoint, GetScoreModifier, GetWorldPosition, HasNodeLabel, HasNodeWaypoint, HasWorldPosition, Initialize, Initialized, Load, RunValidation, Save, Test, onVesselChange, onVesselCrewWasModified, scanVessels, targetVesselStillValid, testLocation
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestTimeSinceNode.cs
- 128 lines; kind=MonoBehaviour; type=TestTimeSinceNode; bases=TestModule,IScoreableObjective; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=21
- ksp refs: Localizer=5, Planetarium=2, ConfigNode=2, FlightGlobals=1; fan_in=0 fan_out=18
- methods(9): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Initialize, Load, RunValidation, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVessel.cs
- 161 lines; kind=MonoBehaviour; type=TestVessel; bases=TestModule; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=22
- ksp refs: Localizer=7, FlightGlobals=6, Vessel=4, ConfigNode=2, ProtoPartSnapshot=1, PartResourceDefinition=1; fan_in=22 fan_out=17
- methods(9): Awake, GetNodeBodyParameterString, Load, OnVesselDocking, OnVesselPersistentIdChanged, OnVesselsUndocking, Save, Test, vesselSelectorCreated
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselActive.cs
- 27 lines; kind=MonoBehaviour; type=TestVesselActive; bases=TestVessel; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: KSP.Localization
- unity: Lifecycle=1; unity_score=36
- ksp refs: Localizer=1, FlightGlobals=1; fan_in=0 fan_out=4
- methods(3): Awake, GetInfo, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselActiveCount.cs
- 179 lines; kind=MonoBehaviour; type=TestVesselActiveCount; bases=TestModule; ancestry=TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=19
- ksp refs: GameEvents=4, Vessel=3, Localizer=2, FlightGlobals=2, ConfigNode=2; fan_in=0 fan_out=16
- methods(11): Awake, Cleared, GetInfo, GetNodeBodyParameterString, Initialized, Load, OnAddVessel, OnRemoveVessel, Save, Test, UpdateVesselCount
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselAltitude.cs
- 118 lines; kind=MonoBehaviour; type=TestVesselAltitude; bases=TestVessel,IScoreableObjective; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=24
- ksp refs: Localizer=5, FlightGlobals=3, ConfigNode=2; fan_in=0 fan_out=14
- methods(7): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselCrewCount.cs
- 167 lines; kind=MonoBehaviour; type=TestVesselCrewCount; bases=TestVessel,IScoreableObjective; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=22
- ksp refs: Localizer=6, GameDatabase=5, FlightGlobals=3, ProtoCrewMember=2, ConfigNode=2, KerbalRoster=1; fan_in=0 fan_out=17
- methods(8): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, SetKerbalRoles, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselMass.cs
- 97 lines; kind=MonoBehaviour; type=TestVesselMass; bases=TestVessel,IScoreableObjective; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=26
- ksp refs: Localizer=3, FlightGlobals=3, ConfigNode=2; fan_in=0 fan_out=13
- methods(7): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselPartCount.cs
- 147 lines; kind=MonoBehaviour; type=TestVesselPartCount; bases=TestVessel,IScoreableObjective; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=23
- ksp refs: FlightGlobals=7, Localizer=3, ConfigNode=2; fan_in=0 fan_out=13
- methods(8): Awake, CheckForUnloadedVessel, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselRecovery.cs
- 44 lines; kind=MonoBehaviour; type=TestVesselRecovery; bases=TestVessel; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: KSP.Localization, KSP.UI.Screens
- unity: Lifecycle=1; unity_score=30
- ksp refs: GameEvents=2, ProtoVessel=1, Localizer=1; fan_in=0 fan_out=5
- methods(6): Awake, Cleared, GetInfo, Initialized, Recovered, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselResource.cs
- 167 lines; kind=MonoBehaviour; type=TestVesselResource; bases=TestVessel,IScoreableObjective; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=22
- ksp refs: PartResourceDefinition=4, Localizer=2, ConfigNode=2, ProtoPartSnapshot=1; fan_in=0 fan_out=15
- methods(8): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, SetResourceNames, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselSituation.cs
- 237 lines; kind=MonoBehaviour; type=TestVesselSituation; bases=TestVessel,IScoreableObjective; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, FinePrint, KSP.Localization, UnityEngine
- unity: SetActive=1, Lifecycle=1, GameObject=1; unity_score=26
- ksp refs: Localizer=6, FlightGlobals=3, Vessel=2, Planetarium=2, ConfigNode=2, CelestialBody=1; fan_in=3 fan_out=19
- methods(18): Awake, GetExtraText, GetInfo, GetNodeBody, GetNodeBodyParameterString, GetNodeWaypoint, GetScoreModifier, GetWorldPosition, HasNodeBody, HasNodeLabel, HasNodeWaypoint, HasWorldPosition, Initialized, Load, OnIgnoreSurfaceVelocity, OnSurfaceVelocityControlCreated, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselSituationFlying.cs
- 68 lines; kind=MonoBehaviour; type=TestVesselSituationFlying; bases=TestVesselSituation,IScoreableObjective,INodeWaypoint,ITestNodeLabel,INodeBody; ancestry=TestVesselSituation/TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=26
- ksp refs: Localizer=2, FlightGlobals=2, ConfigNode=2, Vessel=1; fan_in=0 fan_out=19
- methods(5): Awake, GetInfo, Load, ParameterSetupComplete, Save
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselSituationLanded.cs
- 69 lines; kind=MonoBehaviour; type=TestVesselSituationLanded; bases=TestVesselSituation,INodeWaypoint,ITestNodeLabel,INodeBody; ancestry=TestVesselSituation/TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=25
- ksp refs: Localizer=2, FlightGlobals=2, ConfigNode=2, Vessel=1; fan_in=0 fan_out=19
- methods(5): Awake, GetInfo, Load, ParameterSetupComplete, Save
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselSituationSplashed.cs
- 64 lines; kind=MonoBehaviour; type=TestVesselSituationSplashed; bases=TestVesselSituation,INodeWaypoint,ITestNodeLabel,INodeBody; ancestry=TestVesselSituation/TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: Lifecycle=1; unity_score=26
- ksp refs: FlightGlobals=2, ConfigNode=2, Vessel=1, Localizer=1; fan_in=0 fan_out=19
- methods(4): Awake, Load, ParameterSetupComplete, Save
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselStage.cs
- 121 lines; kind=MonoBehaviour; type=TestVesselStage; bases=TestVessel,IScoreableObjective; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=24
- ksp refs: Localizer=3, FlightGlobals=3, ConfigNode=2; fan_in=0 fan_out=13
- methods(7): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselStageActivated.cs
- 88 lines; kind=MonoBehaviour; type=TestVesselStageActivated; bases=TestVessel,IScoreableObjective; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=27
- ksp refs: FlightGlobals=4, Localizer=2, GameEvents=2, ConfigNode=2; fan_in=0 fan_out=11
- methods(9): Awake, Cleared, GetInfo, GetScoreModifier, Initialized, Load, Save, StageActivated, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselState.cs
- 70 lines; kind=MonoBehaviour; type=TestVesselState; bases=TestVessel,IScoreableObjective; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=34
- ksp refs: Vessel=3, FlightGlobals=3, ConfigNode=2, Localizer=1; fan_in=0 fan_out=10
- methods(5): Awake, GetScoreModifier, Load, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselStateCrashed.cs
- 128 lines; kind=MonoBehaviour; type=TestVesselStateCrashed; bases=TestVessel,IScoreableObjective; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=24
- ksp refs: FlightGlobals=5, Localizer=2, GameEvents=2, ConfigNode=2, Vessel=1; fan_in=0 fan_out=17
- methods(10): Awake, Cleared, GetInfo, GetScoreModifier, Initialized, Load, OnVesselToBeDestroyed, Save, Test, TestVesselLocation
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselVelocity.cs
- 360 lines; kind=MonoBehaviour; type=TestVesselVelocity; bases=TestVessel; ancestry=TestVessel/TestModule/MonoBehaviour; ns=Expansions.Missions.Tests
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: SetActive=10, GameObject=10, Lifecycle=1; unity_score=48
- ksp refs: Localizer=10, FlightGlobals=10, Vessel=6, ConfigNode=5, ProtoCrewMember=3; fan_in=0 fan_out=29
- methods(18): Awake, CheckCrew, GetInfo, GetNodeBodyParameterString, Initialize, Load, OnSpeedTypeControlCreated, OnSpeedTypeControlSetup, OnVelocityWRTAsteroidControlCreated, OnVelocityWRTBodyControlCreated, OnVelocityWRTBodyControlSetup, OnVelocityWRTBodyValueChange, OnVelocityWRTCometControlCreated, OnVelocityWRTKerbalControlCreated, OnVelocityWRTVesselControlCreated, ParameterSetupComplete, Save, Test
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselGroundLocation.cs
- 243 lines; kind=class; type=VesselGroundLocation; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, FinePrint, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: FlightGlobals=8, Localizer=3, ConfigNode=3, CelestialBody=1; fan_in=14 fan_out=21
- methods(14): Equals, GetExtraText, GetHashCode, GetNodeBodyParameterString, GetWaypoint, GetWorldPosition, HasNodeLabel, HasWaypoint, Load, OnGizmoIconChangeDelegate, OnVesselIconChange, Save, SetAvailableGizmoIcons, SetCelestialBodies
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselLocation.cs
- 130 lines; kind=class; type=VesselLocation; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor
- unity: none; unity_score=3
- ksp refs: ConfigNode=4, FlightGlobals=2, ResearchAndDevelopment=1; fan_in=5 fan_out=17
- methods(4): Load, Save, SelectedlaunchSite_SetDropDownValues, createZeroOrbit
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselPartIDPair.cs
- 88 lines; kind=class; type=VesselPartIDPair; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=4
- ksp refs: ConfigNode=2, ShipConstruct=1, Localizer=1; fan_in=8 fan_out=10
- methods(5): Equals, GetHashCode, Load, Save, ValidatePartAgainstCraft
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction.cs
- 127 lines; kind=class; type=VesselRestriction; bases=DynamicModule; ancestry=DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Runtime, KSP.Localization, KSP.UI, PreFlightTests, TMPro, UnityEngine
- unity: Time.=1, TMPro=1, GetComponent=1; unity_score=16
- ksp refs: Localizer=2; fan_in=13 fan_out=11
- methods(13): AddAppUIReference, ClearAppUIREference, ClearEvents, GetDisplayName, GetPreflightCheck, GetStateMessage, IsComplete, SameComparator, SameComparatorWrapper, SuscribeToEvents, TestFloat, TestInt, UpdateAppEntry
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_Cost.cs
- 115 lines; kind=class; type=VesselRestriction_Cost; bases=VesselRestriction; ancestry=VesselRestriction/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization, KSP.UI.Screens
- unity: none; unity_score=0
- ksp refs: GameEvents=8, Localizer=3, ConfigNode=3, ShipConstruct=1, Part=1, EditorLogic=1; fan_in=0 fan_out=14
- methods(11): Callback_PlacePart, Callback_ShipLoad, ClearEvents, GetDisplayName, GetShipCurrentCost, GetStateMessage, IsComplete, Load, SameComparator, Save, SuscribeToEvents
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_Crew.cs
- 168 lines; kind=class; type=VesselRestriction_Crew; bases=VesselRestriction; ancestry=VesselRestriction/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization, KSP.UI, KSP.UI.Screens
- unity: none; unity_score=0
- ksp refs: Localizer=4, GameEvents=4, KerbalRoster=3, ConfigNode=3, ShipConstruct=1, ProtoCrewMember=1; fan_in=0 fan_out=16
- methods(12): Callback_CrewChange, Callback_ShipLoad, ClearEvents, GetCrewCount, GetCrewTraitCount, GetDisplayName, GetStateMessage, IsComplete, Load, SameComparator, Save, SuscribeToEvents
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_Mass.cs
- 125 lines; kind=class; type=VesselRestriction_Mass; bases=VesselRestriction; ancestry=VesselRestriction/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization, KSP.UI.Screens
- unity: none; unity_score=0
- ksp refs: GameEvents=10, Localizer=5, ConfigNode=3, ShipConstruct=2, Part=1, EditorLogic=1; fan_in=2 fan_out=14
- methods(13): Callback_CrewModified, Callback_PlacePart, Callback_ShipLoad, Callback_ShipModified, ClearEvents, GetCurrentMass, GetDisplayName, GetStateMessage, IsComplete, Load, SameComparator, Save, SuscribeToEvents
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_PartCount.cs
- 113 lines; kind=class; type=VesselRestriction_PartCount; bases=VesselRestriction; ancestry=VesselRestriction/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization, KSP.UI.Screens
- unity: none; unity_score=0
- ksp refs: GameEvents=8, Localizer=5, ConfigNode=3, ShipConstruct=1, Part=1, EditorLogic=1; fan_in=1 fan_out=13
- methods(11): Callback_PlacePart, Callback_ShipLoad, ClearEvents, GetCurrentParts, GetDisplayName, GetStateMessage, IsComplete, Load, SameComparator, Save, SuscribeToEvents
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_RequiredParts.cs
- 101 lines; kind=class; type=VesselRestriction_RequiredParts; bases=VesselRestriction; ancestry=VesselRestriction/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Runtime, KSP.Localization, KSP.UI.Screens
- unity: none; unity_score=0
- ksp refs: GameEvents=8, ShipConstruct=2, Localizer=2, ConfigNode=2, PartLoader=1, Part=1; fan_in=1 fan_out=14
- methods(9): Callback_PlacePart, Callback_ShipLoad, ClearEvents, GetDisplayName, GetStateMessage, IsComplete, Load, Save, SuscribeToEvents
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_Resource.cs
- 161 lines; kind=class; type=VesselRestriction_Resource; bases=VesselRestriction; ancestry=VesselRestriction/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization, KSP.UI.Screens
- unity: none; unity_score=0
- ksp refs: GameEvents=10, PartResourceDefinition=4, Localizer=4, ConfigNode=3, ShipConstruct=2, Part=2; fan_in=0 fan_out=18
- methods(14): Callback_CrewModified, Callback_PlacePart, Callback_ShipLoad, Callback_ShipModified, ClearEvents, GetCurrentResourceValue, GetDisplayName, GetStateMessage, IsComplete, Load, SameComparator, Save, SetResourceNames, SuscribeToEvents
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_Size.cs
- 153 lines; kind=class; type=VesselRestriction_Size; bases=VesselRestriction; ancestry=VesselRestriction/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: none; unity_score=5
- ksp refs: Localizer=11, GameEvents=8, ConfigNode=3, ShipConstruct=1, Part=1, EditorLogic=1; fan_in=1 fan_out=15
- methods(13): CallBack_ShipLoad, Callback_PlacePart, Check, ClearEvents, GetDisplayName, GetShipDimensionsString, GetShipSize, GetStateMessage, IsComplete, Load, SameComparator, Save, SuscribeToEvents
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_Stages.cs
- 127 lines; kind=class; type=VesselRestriction_Stages; bases=VesselRestriction; ancestry=VesselRestriction/DynamicModule/IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization, KSP.UI.Screens
- unity: none; unity_score=0
- ksp refs: GameEvents=12, Localizer=4, ConfigNode=3, Part=1; fan_in=0 fan_out=10
- methods(13): Callback_AddStage, Callback_ModifyStage, Callback_PlacePart, Callback_RemoveStage, ClearEvents, GetCurrentStageCount, GetDisplayName, GetStateMessage, IsComplete, Load, SameComparator, Save, SuscribeToEvents
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestrictionList.cs
- 73 lines; kind=class; type=VesselRestrictionList; bases=DynamicModuleList; ancestry=DynamicModuleList/IConfigNode; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=4
- methods(3): ClearAppEvents, GetSupportedTypes, StartAppEvents
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestrictionOptions.cs
- 9 lines; kind=attribute; type=VesselRestrictionOptions; bases=Attribute; ancestry=Attribute; ns=Expansions.Missions
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselSituation.cs
- 309 lines; kind=class; type=VesselSituation; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=4
- ksp refs: ProtoCrewMember=5, ConfigNode=5, Localizer=3, KerbalRoster=2, HighLogic=2, Game=1; fan_in=33 fan_out=29
- methods(13): ClearEditorEvents, GetRequiredExcludedParts, Load, OnAutoGenerateCrewControlCreated, OnPartPickerNeedsToUpdateExcludedPartsFilter, OnRequieredPartsCheckpointValidator, OnRequiredPartsParameterCreated, Save, SetCrewAvailable, SetVesselCrew, StartEditorEvents, UpdateAutoGenerateCrewSettings, UpdateRequiredPartsDisplayedParts
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Serenity/AxisLimitLine.cs
- 79 lines; kind=MonoBehaviour; type=AxisLimitLine; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler,IBeginDragHandler,IEndDragHandler,IDragHandler; ancestry=MonoBehaviour; ns=Expansions.Serenity
- usings: KSP.UI.TooltipTypes, UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=8, Transform=2, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1; unity_score=82
- ksp refs: none; fan_in=1 fan_out=3
- methods(9): Awake, OnBeginDrag, OnDestroy, OnDrag, OnEndDrag, OnPointerClick, SetToolTipValue, Setup, UpdateYPosition
- auto suggestion: tier=C subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/BaseServo.cs **MUST-OPEN (>800 lines)**
- 1819 lines; kind=PartModule; type=BaseServo; bases=PartModule,IRoboticServo,IResourceConsumer,IPartCostModifier,IPartMassModifier,IAxisFieldLimits,IJointLockState,IConstruction; ancestry=PartModule/MonoBehaviour; ns=Expansions.Serenity
- usings: KSP.Localization, UnityEngine
- unity: Transform=46, GameObject=16, Rigidbody=8, Lifecycle=4, Joint=4, GetComponent=3; unity_score=46
- ksp refs: KSPField=37, HighLogic=36, GameEvents=17, Localizer=16, Part=14, EditorLogic=8; fan_in=7 fan_out=50
- methods(104): ApplyCoordsUpdate, AttachServoRigidBody, BaseObject, BuildPartSets, CacheLocalStrings, CalculateAverageRateOfMovement, CalculatePower, CalculateResourceDrain, CanBeDetached, CanBeOffset, CanBeRotated, CreateServoRigidbody, DemoteToPhysicslessPart, DisengageMotor, DisengageServoLock, EngageMotor, EngageServoLock, FixedUpdate, GetAxisFieldLimit, GetAxisFieldLimits, GetConsumedResources, GetFrameDisplacement, GetHardLimits, GetInfo, GetMainAxis, GetModuleCost, GetModuleCostChangeWhen, GetModuleMass, GetModuleMassChangeWhen, GetSoftLimits, HasAxisFieldLimit, HasAxisFieldLimits, InitAxisFieldLimits, InitJoint, IsJointUnlocked, IsMoving, ManuallyModifyEngaged, ModifyEngaged, ModifyLocked, ModifyServo, MotorOffAction, MotorOnAction, MovingObject, NodeRigidBody, OnAwake ...
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/Contracts/CollectDeployedScience.cs
- 108 lines; kind=ContractParameter; type=CollectDeployedScience; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Expansions.Serenity.Contracts
- usings: Contracts, KSP.Localization
- unity: none; unity_score=4
- ksp refs: Contract=6, GameEvents=3, CelestialBody=3, Localizer=2, ConfigNode=2, ScienceSubject=1; fan_in=1 fan_out=11
- methods(7): GetHashString, GetTitle, OnLoad, OnRegister, OnSave, OnScience, OnUnregister
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/Contracts/CollectROCScienceArm.cs
- 130 lines; kind=ContractParameter; type=CollectROCScienceArm; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Expansions.Serenity.Contracts
- usings: Contracts, KSP.Localization
- unity: none; unity_score=3
- ksp refs: Contract=6, GameEvents=3, CelestialBody=3, Localizer=2, ConfigNode=2, ScienceSubject=1; fan_in=1 fan_out=12
- methods(7): GetHashString, GetTitle, OnLoad, OnRegister, OnSave, OnScience, OnUnregister
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/Contracts/CollectROCScienceRetrieval.cs
- 138 lines; kind=ContractParameter; type=CollectROCScienceRetrieval; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Expansions.Serenity.Contracts
- usings: Contracts, KSP.Localization, KSP.UI.Screens
- unity: none; unity_score=3
- ksp refs: Contract=4, ConfigNode=4, Localizer=3, CelestialBody=3, ProtoPartSnapshot=2, GameEvents=2; fan_in=1 fan_out=13
- methods(7): GetHashString, GetTitle, OnLoad, OnRegister, OnSave, OnUnregister, OnVesselRecoveryProcessing
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/Contracts/DeployedScienceContract.cs
- 222 lines; kind=Contract; type=DeployedScienceContract; bases=Contract; ancestry=Contract/IContractParameterHost; ns=Expansions.Serenity.Contracts
- usings: Contracts, FinePrint, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=7
- ksp refs: CelestialBody=9, ResearchAndDevelopment=4, Localizer=3, Reputation=2, Planetarium=2, FlightGlobals=2; fan_in=0 fan_out=25
- methods(11): Generate, GenerateContract_Surface, GetDescription, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave, PotentialsContainBody
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/Contracts/ROCScienceArmContract.cs
- 316 lines; kind=Contract; type=ROCScienceArmContract; bases=Contract; ancestry=Contract/IContractParameterHost; ns=Expansions.Serenity.Contracts
- usings: Contracts, FinePrint, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=5
- ksp refs: Localizer=8, Reputation=6, CelestialBody=6, AvailablePart=3, PartLoader=2, FlightGlobals=2; fan_in=0 fan_out=24
- methods(9): Generate, GetDescription, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/Contracts/ROCScienceRetrievalContract.cs
- 232 lines; kind=Contract; type=ROCScienceRetrievalContract; bases=Contract; ancestry=Contract/IContractParameterHost; ns=Expansions.Serenity.Contracts
- usings: Contracts, FinePrint, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=5
- ksp refs: Localizer=6, CelestialBody=6, Reputation=2, FlightGlobals=2, ConfigNode=2, ScienceSubject=1; fan_in=0 fan_out=24
- methods(9): Generate, GetDescription, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ControlledAction.cs
- 204 lines; kind=class; type=ControlledAction; bases=ControlledBase; ancestry=ControlledBase/IConfigNode; ns=Expansions.Serenity
- usings: UnityEngine
- unity: none; unity_score=11
- ksp refs: Part=10, ConfigNode=5, PartModule=1; fan_in=6 fan_out=7
- methods(8): AddSymmetryPart, ClearSymmetryLists, OnAssignReferenceVars, OnChangeSymmetryMaster, OnLoad, OnSave, RescaleTimes, ReverseTimes
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ControlledAxis.cs
- 501 lines; kind=class; type=ControlledAxis; bases=ControlledBase; ancestry=ControlledBase/IConfigNode; ns=Expansions.Serenity
- usings: UnityEngine
- unity: AnimationCurve=49; unity_score=31
- ksp refs: FloatCurve=20, Part=9, ConfigNode=4, PartModule=1; fan_in=6 fan_out=12
- methods(23): AddSymmetryPart, AlignCurveEnds, ClampAllPointValues, ClampPointValue, ClearPresetRefs, ClearSymmetryLists, GenerateCurveFromReference, InvertCurve, OnAssignReferenceVars, OnChangeSymmetryMaster, OnLoad, OnSave, PresetFlat, PresetRevSaw, PresetSaw, PresetSine, PresetSquare, PresetTriangle, RescaleCurveTime, ReverseCurve, UpdateFieldValue, UpdatePreset, UpdateSoftLimits
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ControlledBase.cs
- 184 lines; kind=class; type=ControlledBase; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Serenity
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: Part=16, ConfigNode=5, PartModule=2, FlightGlobals=1; fan_in=4 fan_out=7
- methods(12): AddSymmetryPart, AssignReferenceVars, ChangeSymmetryMaster, ClearSymmetryLists, Load, OnAssignReferenceVars, OnChangeSymmetryMaster, OnLoad, OnSave, RebuildSymmetryList, Save, SetPartNickName
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/CurvePanel.cs **MUST-OPEN (>800 lines)**
- 967 lines; kind=MonoBehaviour; type=CurvePanel; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler; ancestry=MonoBehaviour; ns=Expansions.Serenity
- usings: KSP.UI, UnityEngine, UnityEngine.EventSystems, Vectrosity
- unity: AnimationCurve=26, GameObject=14, SetActive=10, Transform=8, Instantiate/Destroy=7, UnityEngine.UI=6; unity_score=56
- ksp refs: FloatCurve=4; fan_in=4 fan_out=4
- methods(46): Awake, BeginDragPoint, ClearSelectedPoints, CurveTimeFromXPos, CurveValueFromYPos, DeletePoint, DeleteSelectedPoints, DrawAll, DrawLine, DrawPoints, EndDragPoint, GetCurvePointsCount, GetIndexOfPoint, HideLimits, HideMaxLimit, HideMinLimit, HidePoints, InsertPoint, InsertPointAtPosition, MousePointClick, MovePoint, OnDestroy, OnDragPoint, OnPointerClick, OnTangentDrag, PanelXPosFromTime, PanelYPosFromValue, RebuildSelectedPointIndexes, RemoveLimitClamping, SelectAllPoints, SelectPoint, SetBackgroundElements, SetCurve, SetEditable, SetLimitClamping, SetLimits, SetTangentSharp, SetTangentSmooth, Setup, ShowLimits, ShowMaxLimit, ShowMinLimit, ShowPoints, Start, Update ...
- auto suggestion: tier=C subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/CurvePanelPoint.cs
- 232 lines; kind=MonoBehaviour; type=CurvePanelPoint; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler,IBeginDragHandler,IEndDragHandler,IDragHandler; ancestry=MonoBehaviour; ns=Expansions.Serenity
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=11, AnimationCurve=10, EventSystems=8, Transform=7, SetActive=6, UnityEngine.UI=4; unity_score=86
- ksp refs: none; fan_in=6 fan_out=2
- methods(18): Awake, Delete, Deselect, HideDelete, OnBeginDrag, OnDeleteClick, OnDestroy, OnDrag, OnEndDrag, OnPointerClick, OnTangentDrag, Select, SetKeyFrame, SetKeyFrameTangent, SetKeyFrameTangents, SetTangentHandles, Setup, ShowDelete
- auto suggestion: tier=C subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/CurvePanelPointHandle.cs
- 45 lines; kind=MonoBehaviour; type=CurvePanelPointHandle; bases=MonoBehaviour,IDragHandler,IEventSystemHandler; ancestry=MonoBehaviour; ns=Expansions.Serenity
- usings: KSP.UI, UnityEngine, UnityEngine.EventSystems
- unity: UnityEngine.UI=2, EventSystems=2, Transform=1, MonoBehaviour=1, GameObject=1; unity_score=82
- ksp refs: none; fan_in=0 fan_out=2
- methods(1): OnDrag
- auto suggestion: tier=C subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/DeployedScience/Runtime/DeployedScience.cs
- 634 lines; kind=ScenarioModule; type=DeployedScience; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=Expansions.Serenity.DeployedScience.Runtime
- usings: KSP.UI.Screens, UnityEngine
- unity: GameObject=14, Instantiate/Destroy=5, Transform=2, Lifecycle=2, GetComponent=2, Coroutine=1; unity_score=42
- ksp refs: GameEvents=24, ConfigNode=23, FlightGlobals=6, Vessel=5, TimeWarp=4, Part=3; fan_in=9 fan_out=23
- methods(24): DeRegisterCluster, ExpDimReturnsRate, ExpMaxScienceMultiplier, FixedUpdate, GetDiminishingReturnsMultiplier, OnAwake, OnDestroy, OnGameStateLoad, OnLoad, OnPartExplodeGroundCollision, OnSave, OnVesselExplodeGroundCollision, OnVesselRecovered, OnVesselTerminated, OnVesselWillDestroy, RegisterCluster, RemoveMannedScienceObjects, ScanAndProcessSeismicEvent, SetDeployedSciencePartVesselType, SetDiminishingScienceRate, SetupDiminishingReturns, SetupSeismicEnergy, UpdateCluster, UpdateSciencePart
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/DeployedScience/Runtime/DeployedScienceCluster.cs
- 484 lines; kind=MonoBehaviour; type=DeployedScienceCluster; bases=MonoBehaviour,IConfigNode; ancestry=MonoBehaviour; ns=Expansions.Serenity.DeployedScience.Runtime
- usings: UnityEngine
- unity: GameObject=10, Lifecycle=3, GetComponent=3, Transform=2, Coroutine=2, MonoBehaviour=1; unity_score=47
- ksp refs: GameEvents=8, ConfigNode=8, Planetarium=4, Part=1, FlightGlobals=1, CelestialBody=1; fan_in=7 fan_out=13
- methods(14): Awake, GroundScienceModuleRemoved, GroundSciencePartEnabledStateChanged, Load, OnDestroy, Save, SolarPanelUnitsProduced, Spawn, SpawnandLoad, Start, UpdateCluster, UpdateExperimentDiminishReturns, UpdatePowerState, UpdateScience
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/DeployedScience/Runtime/DeployedScienceExperiment.cs **MUST-OPEN (>800 lines)**
- 839 lines; kind=MonoBehaviour; type=DeployedScienceExperiment; bases=MonoBehaviour,IConfigNode; ancestry=MonoBehaviour; ns=Expansions.Serenity.DeployedScience.Runtime
- usings: CommNet, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: Coroutine=7, GameObject=6, Transform=4, Lifecycle=4, Rigidbody=2, MonoBehaviour=1; unity_score=46
- ksp refs: GameEvents=11, Localizer=10, FlightGlobals=10, Planetarium=8, ResearchAndDevelopment=5, Vessel=4; fan_in=5 fan_out=28
- methods(26): Awake, CalcScience, CalcSendSeismicScience, CalculateScience, FixedUpdate, GatherScienceData, GroundScienceClusterPowerStateChanged, GroundSciencePartEnabledStateChanged, Load, OnDestroy, ProcessSeismicEvent, Save, SeismicDataCalc, SeismicDataCalcOverrideAdd, SeismicDataCalcOverrideRemove, SeismicListContains, SendDataToComms, SetControllerVessel, Spawn, SpawnandLoad, Start, TimeToSendStoredData, UpdateScienceExperiment, UpdateScienceLimitExperiment, UpdateScienceValueExperiment, processImpactVesselPart
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/DeployedScience/Runtime/DeployedScienceExtensions.cs
- 97 lines; kind=static class; type=DeployedScienceExtensions; bases=-; ancestry=-; ns=Expansions.Serenity.DeployedScience.Runtime
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=4
- methods(4): Get, GetExperiment, GetExperimentByPartID, GetExperiments
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/DeployedScience/Runtime/DeployedSciencePart.cs
- 269 lines; kind=MonoBehaviour; type=DeployedSciencePart; bases=MonoBehaviour,IConfigNode; ancestry=MonoBehaviour; ns=Expansions.Serenity.DeployedScience.Runtime
- usings: UnityEngine
- unity: GameObject=13, Transform=10, Lifecycle=3, GetComponent=3, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=67
- ksp refs: GameEvents=6, ConfigNode=6, Part=2, FlightGlobals=2; fan_in=9 fan_out=13
- methods(10): Awake, Load, OnDestroy, OnGroundSciencePartRemoved, PartIsLoaded, Save, Spawn, SpawnandLoad, Start, UpdateSciencePart
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/IRoboticServo.cs
- 17 lines; kind=interface; type=IRoboticServo; bases=-; ancestry=-; ns=Expansions.Serenity
- usings: UnityEngine
- unity: Rigidbody=2, GameObject=2; unity_score=57
- ksp refs: AttachNode=2; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ModuleRobotArmScanner.cs **MUST-OPEN (>800 lines)**
- 1290 lines; kind=PartModule; type=ModuleRobotArmScanner; bases=ModuleDeployablePart; ancestry=ModuleDeployablePart/PartModule/MonoBehaviour; ns=Expansions.Serenity
- usings: Expansions.Serenity.RobotArmFX, KSP.Localization, UnityEngine
- unity: GameObject=27, Transform=19, GetComponent=13, ParticleSystem=11, Time.=9, Raycast/Query=7; unity_score=65
- ksp refs: KSPField=26, Localizer=24, TimeWarp=17, HighLogic=16, GameEvents=10, ConfigNode=9; fan_in=3 fan_out=22
- methods(44): CacheLocalStrings, CalculateExtendedTargetRotations, CanScan, DoExtend, DoRepair, DoRetract, ExtendArm, GetInfo, GetModuleDisplayName, OnAwake, OnCollisionEnter, OnCurrentMousePartChanged, OnDestroy, OnInventoryModeDisable, OnInventoryModeEnable, OnLoad, OnPartActionUICreate, OnPause, OnSave, OnStart, OnTimeWarpRateChanged, OnTriggerEnter, OnTriggerExit, OnUnpause, OnUpdate, PackArm, PerformExperiment, PerformScanROCNullCheck, PlayEditorAnimation, PlayPackAnimation, PlayScanAnimation, PlayUnpackAnimation, RayCastToROC, ReturnArmToPrePackState, SelectROCToScan, SetupScannerEffectList, SetupScannerTransforms, Start, UnpackArm, UpdateAnimationTime, UpdatePartActionUI, breakPanels, startFSM, updateFSM
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ModuleRoboticController.cs **MUST-OPEN (>800 lines)**
- 1546 lines; kind=PartModule; type=ModuleRoboticController; bases=PartModule,IShipConstructIDChanges,IResourceConsumer; ancestry=PartModule/MonoBehaviour; ns=Expansions.Serenity
- usings: KSP.Localization, UnityEngine
- unity: Lifecycle=4, UnityEngine.UI=3, Instantiate/Destroy=2, GameObject=2, Transform=1, Time.=1; unity_score=25
- ksp refs: Part=53, GameEvents=31, KSPAction=16, KSPField=11, HighLogic=10, ConfigNode=10; fan_in=14 fan_out=42
- methods(80): AddPartAction, AddPartActionForSymmetryPartner, AddPartAxis, AddPartAxisForSymmetryPartner, AssignReferenceVars_Debug, CheckAndFireActions, CycleLoopMode, FixedUpdate, GetConsumedResources, GetInfo, GetModuleDisplayName, HasPart, HasPartAction, HasPartAxisField, HasPartSymmetryAction, HasPartSymmetryAxisField, OnAwake, OnDestroy, OnLoad, OnPartWillDie, OnSave, OnSceneLoadRequested, OnStart, OnUpdate, OnWarpRateChanged, OpenControllerEditor, PartDeleted, PartPersistentIdChanged, PlaySequenceAction, PriorityChanged, RemoveOtherVesselItems, RemovePartAction, RemovePartAxis, RemovePartSymmetry, SequenceForwardAction, SequenceLoopOnceAction, SequenceLoopOnceRestartAction, SequenceLoopPingPongAction, SequenceLoopRepeatAction, SequencePlay, SequencePlaySpeedFullAction, SequencePlaySpeedZeroAction, SequenceReverseAction, SequenceStop, SetDirection ...
- auto suggestion: tier=C subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ModuleRoboticRotationServo.cs
- 756 lines; kind=PartModule; type=ModuleRoboticRotationServo; bases=BaseServo; ancestry=BaseServo/PartModule/MonoBehaviour; ns=Expansions.Serenity
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Transform=12, Joint=11, GameObject=4, Time.=1, SetActive=1, Lifecycle=1; unity_score=47
- ksp refs: KSPField=17, HighLogic=10, Localizer=6, GameEvents=4, TimeWarp=3, Part=3; fan_in=2 fan_out=27
- methods(39): CheckSymmetry, GetFrameDisplacement, GetInfo, GetModuleDisplayName, InitAxisFieldLimits, IsMoving, MaximumAngleAction, MinimumAngleAction, ModifyInverted, ModifyLimits, ModifyLimitsMode, ModifyTargetAngle, ModifyTraverseLimits, OnCopy, OnDestroy, OnEditorPartEvent, OnFixedUpdate, OnJointInit, OnModifyServo, OnPostStartJointInit, OnPreModifyServo, OnResourceNonemptyEmpty, OnSaveShip, OnServoLockApplied, OnServoLockRemoved, OnServoMotorDisengaged, OnServoMotorEngaged, OnStart, OnVisualizeServo, ResetLaunchPosition, SetInitialDisplacement, SetJointHighLowLimits, SetMaximumAngle, SetMinimumAngle, SetTargetRotation, ToggleServoAction, UpdateAxisFieldHardLimit, UpdateAxisFieldSoftLimit, UpdatePAWUI
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ModuleRoboticServoHinge.cs
- 742 lines; kind=PartModule; type=ModuleRoboticServoHinge; bases=BaseServo,IMultipleDragCube; ancestry=BaseServo/PartModule/MonoBehaviour; ns=Expansions.Serenity
- usings: KSP.Localization, UnityEngine
- unity: Transform=11, Joint=11, GameObject=4, Time.=1, SetActive=1, Lifecycle=1; unity_score=47
- ksp refs: KSPField=16, HighLogic=7, Localizer=6, GameEvents=4, TimeWarp=3, Part=3; fan_in=2 fan_out=26
- methods(42): AssumeDragCubePosition, CheckSymmetry, GetDragCubeNames, GetFrameDisplacement, GetInfo, GetModuleDisplayName, InitAxisFieldLimits, IsMoving, MaximumAngleAction, MinimumAngleAction, ModifyLimits, ModifyTraverseLimits, OnCopy, OnDestroy, OnEditorPartEvent, OnFixedUpdate, OnInventoryModeDisable, OnInventoryModeEnable, OnJointInit, OnModifyServo, OnPostStartJointInit, OnPreModifyServo, OnResourceNonemptyEmpty, OnSaveShip, OnServoLockApplied, OnServoLockRemoved, OnServoMotorDisengaged, OnServoMotorEngaged, OnStart, OnVisualizeServo, ResetLaunchPosition, SetDragCubes, SetInitialDisplacement, SetMaximumAngle, SetMinimumAngle, SetTargetRotation, ToggleHingeAction, TogglePistons, UpdateAxisFieldHardLimit, UpdateAxisFieldSoftLimit, UpdatePAWUI, UsesProceduralDragCubes
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ModuleRoboticServoPiston.cs
- 675 lines; kind=PartModule; type=ModuleRoboticServoPiston; bases=BaseServo,IMultipleDragCube; ancestry=BaseServo/PartModule/MonoBehaviour; ns=Expansions.Serenity
- usings: KSP.Localization, UnityEngine
- unity: Transform=15, Joint=10, Lifecycle=2, GameObject=2, Time.=1, GetComponent=1; unity_score=50
- ksp refs: KSPField=12, Localizer=7, HighLogic=4, TimeWarp=3, KSPAction=3, GameEvents=2; fan_in=4 fan_out=23
- methods(41): AssumeDragCubePosition, CacheLocalStrings, CalcTargetPosition, ExtendPiston, ExtendPistonAction, GetDragCubeNames, GetFrameDisplacement, GetInfo, GetModuleDisplayName, InitAxisFieldLimits, IsMoving, LateUpdate, ModifyLimits, OnDestroy, OnFixedUpdate, OnJointInit, OnModifyServo, OnPostStartJointInit, OnPreModifyServo, OnResourceNonemptyEmpty, OnSaveShip, OnServoLockApplied, OnServoLockRemoved, OnServoMotorDisengaged, OnServoMotorEngaged, OnStart, OnVisualizeServo, ResetLaunchPosition, RetractPiston, RetractPistonAction, SetDragCubes, SetInitialDisplacement, SetJointTargetPosition, TogglePistonAction, UpdateAxisFieldHardLimit, UpdateAxisFieldSoftLimit, UpdateCurrentExtension, UpdatePAWUI, UpdateTargetPosition, UpdateTraverseVelocity, UsesProceduralDragCubes
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ModuleRoboticServoRotor.cs
- 748 lines; kind=PartModule; type=ModuleRoboticServoRotor; bases=BaseServo; ancestry=BaseServo/PartModule/MonoBehaviour; ns=Expansions.Serenity
- usings: KSP.Localization, UnityEngine
- unity: Joint=15, Collision/Force=8, Transform=7, Time.=5, UnityEngine.UI=1, Lifecycle=1; unity_score=53
- ksp refs: KSPField=14, HighLogic=9, Localizer=6, GameEvents=4, TimeWarp=3, KSPAction=3; fan_in=4 fan_out=24
- methods(37): BrakeAction, GetFrameDisplacement, GetInfo, GetModuleDisplayName, InitAxisFieldLimits, IsMoving, ModifyDirection, ModifyRPMLimits, MotorDirectionAction, MotorPowerAction, OnDestroy, OnFixedUpdate, OnJointInit, OnModifyServo, OnPostStartJointInit, OnPreModifyServo, OnResourceNonemptyEmpty, OnSaveShip, OnServoLockApplied, OnServoLockRemoved, OnServoMotorDisengaged, OnServoMotorEngaged, OnStart, OnUpdate, OnVesselChange, OnVisualizeServo, RatchetLock, ResetLaunchPosition, RotateOnMainAxis, SetInitialDisplacement, SetJointDrive, ToggleMotorDirection, ToggleMotorPower, UpdateAxisFieldHardLimit, UpdateAxisFieldSoftLimit, UpdateLoadMass, UpdatePAWUI
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RobotArmFX/RobotArmFXDrill.cs
- 133 lines; kind=ScriptableObject; type=RobotArmFXDrill; bases=RobotArmScannerFX; ancestry=RobotArmScannerFX/ScriptableObject; ns=Expansions.Serenity.RobotArmFX
- usings: UnityEngine
- unity: ParticleSystem=6, Transform=3, GetComponent=2, Raycast/Query=1; unity_score=52
- ksp refs: ConfigNode=2, Part=1; fan_in=0 fan_out=5
- methods(6): Load, OnEffectStart, OnEffectStop, OnStart, OnUpdate, Save
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RobotArmFX/RobotArmFXLaser.cs
- 113 lines; kind=ScriptableObject; type=RobotArmFXLaser; bases=RobotArmScannerFX; ancestry=RobotArmScannerFX/ScriptableObject; ns=Expansions.Serenity.RobotArmFX
- usings: UnityEngine
- unity: Renderer=2, Raycast/Query=2, Material=2, Transform=1, Shader=1, GetComponent=1; unity_score=52
- ksp refs: ConfigNode=2, Part=1; fan_in=0 fan_out=5
- methods(6): Load, OnEffectStart, OnEffectStop, OnStart, OnUpdate, Save
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RobotArmFX/RobotArmFXSpectrometer.cs
- 70 lines; kind=ScriptableObject; type=RobotArmFXSpectrometer; bases=RobotArmScannerFX; ancestry=RobotArmScannerFX/ScriptableObject; ns=Expansions.Serenity.RobotArmFX
- usings: UnityEngine
- unity: Light=2, Transform=1, GetComponent=1; unity_score=41
- ksp refs: ConfigNode=2, Part=1; fan_in=0 fan_out=3
- methods(6): Load, OnEffectStart, OnEffectStop, OnStart, OnUpdate, Save
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RobotArmFX/RobotArmScannerFX.cs
- 60 lines; kind=ScriptableObject; type=RobotArmScannerFX; bases=ScriptableObject,IConfigNode; ancestry=ScriptableObject; ns=Expansions.Serenity.RobotArmFX
- usings: UnityEngine
- unity: ScriptableObject=3; unity_score=33
- ksp refs: Part=4, ConfigNode=2; fan_in=4 fan_out=4
- methods(7): CreateInstanceOfRobotArmScannerFX, Load, OnEffectStart, OnEffectStop, OnStart, OnUpdate, Save
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RoboticCollisions.cs
- 50 lines; kind=MonoBehaviour; type=RoboticCollisions; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Serenity
- usings: UnityEngine
- unity: Collision/Force=6, Collider=2, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1; unity_score=86
- ksp refs: Part=1, HighLogic=1; fan_in=1 fan_out=3
- methods(5): OnCollisionEnter, OnCollisionExit, OnTriggerEnter, OnTriggerExit, Start
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RoboticControllerManager.cs
- 148 lines; kind=MonoBehaviour; type=RoboticControllerManager; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Serenity
- usings: UnityEngine
- unity: Lifecycle=3, Instantiate/Destroy=3, GameObject=2, MonoBehaviour=1; unity_score=49
- ksp refs: GameEvents=2, FloatCurve=1; fan_in=6 fan_out=8
- methods(10): AnyWindowTextFieldHasFocus, Awake, ClearUpdateQueue, CloseAllWindows, OnDestroy, OnSceneLoad, QueueFieldUpdate, Reset, Update, UpdateFieldValues
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RoboticControllerWindow.cs **MUST-OPEN (>800 lines)**
- 1233 lines; kind=MonoBehaviour; type=RoboticControllerWindow; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Serenity
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=57, Transform=16, GameObject=16, TMPro=12, SetActive=12, GetComponent=7; unity_score=71
- ksp refs: GameEvents=10, Localizer=9, HighLogic=5, EditorLogic=3, InputLockManager=2, FloatCurve=2; fan_in=6 fan_out=31
- methods(70): AddInputFieldLock, AddRow, AnyTextFieldHasFocus, Awake, ClearSelectedPresetMode, CloseWindow, CollapseOtherRows, OnAxisAlignEndsClick, OnAxisClampValuesClick, OnAxisCopyClick, OnAxisDownClick, OnAxisFlipHorizontalClick, OnAxisFlipVerticalClick, OnAxisPasteClick, OnAxisPresetClick, OnAxisPresetFlatClick, OnAxisPresetRevSawClick, OnAxisPresetSawClick, OnAxisPresetSineClick, OnAxisPresetSquareClick, OnAxisPresetTriangleClick, OnAxisSelectAllClick, OnAxisUpClick, OnCloseClick, OnControllerChanged, OnControllerItemsChanged, OnDestroy, OnDirectionClick, OnEditorStarted, OnLengthEdited, OnLoopClick, OnNameEdited, OnOpenActionGroupsClick, OnPlayPauseClick, OnPlaySpeedSliderChanged, OnPointAddClick, OnPointClampValuesClick, OnPointDeleteClick, OnPointDragging, OnPointInfoEdited, OnPointSelected, OnPointValueSharpClick, OnPointValueSmoothClick, OnPositionSliderChanged, OnPresetCyclesSliderChanged ...
- auto suggestion: tier=C subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RoboticControllerWindowAction.cs
- 283 lines; kind=MonoBehaviour; type=RoboticControllerWindowAction; bases=RoboticControllerWindowBaseRow; ancestry=RoboticControllerWindowBaseRow/MonoBehaviour; ns=Expansions.Serenity
- usings: KSP.Localization, KSP.UI, UnityEngine
- unity: Transform=4, UnityEngine.UI=3, SetActive=3, GameObject=3, GetComponent=2, Instantiate/Destroy=1; unity_score=52
- ksp refs: Part=4, Localizer=3, FloatCurve=2; fan_in=1 fan_out=12
- methods(23): HidePoints, InsertPoint, LoadTimesCurve, OnPanelLeftClick, OnPointAdded, OnPointDeleted, OnPointDragging, OnPointMoved, OnPointSelectionChanged, OnRowCollapsed, OnRowDestroy, OnRowExpanded, OnRowStart, RedrawCurve, ReloadCurve, ReverseCurve, SelectAllPoints, SelectPointAtTime, SetLayoutElements, Setup, ShowPoints, Spawn, UpdateUILayout
- auto suggestion: tier=C subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RoboticControllerWindowAxis.cs
- 490 lines; kind=MonoBehaviour; type=RoboticControllerWindowAxis; bases=RoboticControllerWindowBaseRow; ancestry=RoboticControllerWindowBaseRow/MonoBehaviour; ns=Expansions.Serenity
- usings: KSP.Localization, KSP.UI, UnityEngine, UnityEngine.EventSystems
- unity: Transform=5, SetActive=5, GameObject=5, UnityEngine.UI=4, GetComponent=4, EventSystems=3; unity_score=52
- ksp refs: Part=5, Localizer=3, HighLogic=2; fan_in=2 fan_out=16
- methods(36): AlignCurveEnds, BeginDragAxisLimit, ClampAllPointValues, ClampPointValues, ClearPresetRefs, EndDragAxisLimit, HidePoints, InsertPoint, InvertCurve, MoveAxisLimits, OnDragAxisLimit, OnLimitsChanged, OnPanelLeftClick, OnPointDragging, OnPointSelectionChanged, OnRowCollapsed, OnRowDestroy, OnRowExpanded, OnRowStart, PresetFlat, PresetRevSaw, PresetSaw, PresetSine, PresetSquare, PresetTriangle, RedrawCurve, ReloadCurve, ReverseCurve, SelectAllPoints, SelectPointAtTime, SetLayoutElements, Setup, ShowPoints, Spawn, UpdatePreset, UpdateUILayout
- auto suggestion: tier=C subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RoboticControllerWindowBaseRow.cs
- 326 lines; kind=MonoBehaviour; type=RoboticControllerWindowBaseRow; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=Expansions.Serenity
- usings: Highlighting, KSP.UI, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=5, EventSystems=5, UnityEngine.UI=4, TMPro=3, SetActive=3, Lifecycle=3; unity_score=63
- ksp refs: Part=6, HighLogic=1; fan_in=4 fan_out=16
- methods(30): AnyTextFieldHasFocus, AssignBaseReferenceVars, Awake, Collapse, EditButtonClicked, Expand, HighlightParts, InputFieldDone, InsertPoint, OnClickRow, OnDestroy, OnPointDragging, OnPointSelectionChanged, OnPointerEnter, OnPointerExit, OnRemoveRowClick, OnRowCollapsed, OnRowDestroy, OnRowExpanded, OnRowStart, RedrawCurve, ReloadCurve, ReverseCurve, SelectAllPoints, SelectPointAtTime, SetPartHighlight, Start, ToggleExpansion, UnHighlightParts, UpdateUILayout
- auto suggestion: tier=C subsystem=DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/SerenityUtils.cs
- 17 lines; kind=static class; type=SerenityUtils; bases=-; ancestry=-; ns=Expansions.Serenity
- usings: UnityEngine
- unity: GameObject=2, Texture=1; unity_score=48
- ksp refs: none; fan_in=7 fan_out=1
- methods(2): SerenityPrefab, SerenityTexture
- auto suggestion: tier=A subsystem=DLC-BreakingGround
