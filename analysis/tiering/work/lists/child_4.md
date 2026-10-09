# child_4: 547 files, 71564 lines

## Assembly-CSharp/Expansions/BundleLoader.cs
- L=163 kind=MonoBehaviour type=BundleLoader bases=MonoBehaviour anc=MonoBehaviour ns=Expansions
- unity(68): AssetBundle=4, Texture=3, WWW/WebRequest=2, MonoBehaviour=2, Coroutine=2, Lifecycle=1 | ksp: HighLogic=1 | fan_in=5
- methods(6): Awake, IsBundleLoaded, LoadAsset, LoadAssetBundle, LoadScene, LoadTextureAsset
- auto: B / Modding

## Assembly-CSharp/Expansions/ExpansionsLoader.cs
- L=592 kind=MonoBehaviour type=ExpansionsLoader bases=LoadingSystem anc=LoadingSystem/MonoBehaviour ns=Expansions
- unity(43): Coroutine=8, AssetBundle=3, WWW/WebRequest=2, Time.=2, Lifecycle=2, Instantiate/Destroy=1 | ksp: ConfigNode=15, Localizer=14, ProtoCrewMember=5, GameEvents=1 | fan_in=100
- methods(24): Awake, BuildMissionFileHash, FindMissionNode, GetExpansionDirectory, GetExpansionVersion, GetExpansionsThatFailedToLoad, GetInstalledExpansions, GetInstalledExpansionsString, GetMissionVerificationStrings, InitPublicKeyCryptoProvider, InitializeExpansion, IsAnyExpansionInstalled, IsExpansionAnyKerbalSuitInstalled, IsExpansionInstalled, IsExpansionKerbalSuitInstalled, IsInstalled, IsReady, LoadExpansions, OnDestroy, ProgressFraction, ProgressTitle, ReportInstalledExpansions, StartLoad, VerifyHashSignature
- auto: B / Modding

## Assembly-CSharp/Expansions/ExpansionSO.cs
- L=77 kind=ScriptableObject type=ExpansionSO bases=ScriptableObject anc=ScriptableObject ns=Expansions
- unity(32): ScriptableObject=1 | ksp: ProtoCrewMember=3 | fan_in=1
- methods(2): CreateExpansion, SetBundleInfoList
- auto: A / Modding

## Assembly-CSharp/Expansions/KSPExpansionsUtils.cs
- L=9 kind=static class type=KSPExpansionsUtils bases=- anc=- ns=Expansions
- unity(0): none | ksp: none | fan_in=9
- methods(0): 
- auto: A / Modding

## Assembly-CSharp/Expansions/Missions/ActionModule.cs
- L=256 kind=MonoBehaviour type=ActionModule bases=MonoBehaviour,IConfigNode,IActionModule anc=MonoBehaviour ns=Expansions.Missions
- unity(31): Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1, Coroutine=1 | ksp: GameEvents=8, Vessel=4, Localizer=3, ConfigNode=2 | fan_in=23
- methods(36): ActionLocation, AddParameterToNodeBody, AddParameterToNodeBodyAndUpdateUI, AddParameterToSAP, Awake, Fire, GetAppObjectiveInfo, GetDisplayName, GetInfo, GetInternalParametersToDisplay, GetName, GetNode, GetNodeBodyParameterString, HasNodeBodyParameter, HasSAPParameter, Initialize, Load, OnCloned, OnDestroy, OnPartPersistentIdChanged, OnVesselPersistentIdChanged, OnVesselsDocking, OnVesselsUndocking, ParameterSetupComplete ...
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ActionNumberOperator.cs
- L=11 kind=enum type=ActionNumberOperator bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionCreateAsteroid.cs
- L=173 kind=MonoBehaviour type=ActionCreateAsteroid bases=ActionModule,INodeOrbit anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(23): Lifecycle=1 | ksp: ConfigNode=11, Orbit=6, Vessel=4, ProtoVessel=3, Localizer=3 | fan_in=8
- methods(12): ActionLocation, Awake, Fire, GetInfo, GetNodeBodyParameterString, GetNodeOrbit, GetProtoVesselNode, HasNodeOrbit, Load, OnCloned, Save, SpawnAsteroid
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionCreateComet.cs
- L=183 kind=MonoBehaviour type=ActionCreateComet bases=ActionModule,INodeOrbit anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(23): Lifecycle=1 | ksp: ConfigNode=14, Orbit=6, Vessel=4, ProtoVessel=3, Localizer=3 | fan_in=8
- methods(12): ActionLocation, Awake, Fire, GetInfo, GetNodeBodyParameterString, GetNodeOrbit, GetProtoVesselNode, HasNodeOrbit, Load, OnCloned, Save, SpawnComet
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionCreateFlag.cs
- L=126 kind=MonoBehaviour type=ActionCreateFlag bases=ActionModule anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(25): Lifecycle=1 | ksp: ConfigNode=10, Localizer=5, ProtoVessel=2, Orbit=2, HighLogic=2 | fan_in=6
- methods(10): ActionLocation, Awake, CreateFlagPartNode, Fire, GetInfo, GetNodeBodyParameterString, GetProtoVesselNode, Load, OnCloned, Save
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionCreateKerbal.cs
- L=354 kind=MonoBehaviour type=ActionCreateKerbal bases=ActionModule,INodeOrbit,IMissionKerbal anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(24): Lifecycle=1, GetComponent=1 | ksp: ProtoCrewMember=13, ConfigNode=12, Orbit=6, HighLogic=5, Vessel=4 | fan_in=9
- methods(22): ActionLocation, Awake, CreateKerbalPartNode, Fire, GetInfo, GetNodeBodyParameterString, GetNodeOrbit, GetProtoVesselNode, HasNodeOrbit, Initialize, KerbalAdded, KerbalNameChange, KerbalRemoved, KerbalRosterStatusChange, KerbalTypeChange, Load, NodeDeleted, OnCloned, OnHelmetValueChange, OnNeckRingControlCreated, Save, SpawnKerbal
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionCreateLaunchSite.cs
- L=99 kind=MonoBehaviour type=ActionCreateLaunchSite bases=ActionModule anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(32): Lifecycle=1, Coroutine=1 | ksp: Localizer=8, ConfigNode=3, FlightGlobals=1 | fan_in=4
- methods(8): Awake, Fire, GetInfo, GetNodeBodyParameterString, Initialize, Load, RunValidation, Save
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionCreateVessel.cs
- L=539 kind=MonoBehaviour type=ActionCreateVessel bases=ActionModule,INodeOrbit anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(35): Coroutine=8, Transform=2, SetActive=1, Lifecycle=1, Instantiate/Destroy=1, GameObject=1 | ksp: Localizer=42, HighLogic=14, MissionSystem=6, FlightGlobals=5, Orbit=4 | fan_in=10
- methods(16): ActionLocation, Awake, CreatePlaceholderCraft, CreateVesselScreenMessage, DeletePlaceholderCraft, Fire, GetAppObjectiveInfo, GetInfo, GetNodeBodyParameterString, GetNodeOrbit, HasNodeOrbit, Initialize, Load, OnCloned, RunValidation, Save
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionDialogMessage.cs
- L=213 kind=MonoBehaviour type=ActionDialogMessage bases=ActionModule anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(27): SetActive=1, Lifecycle=1, GameObject=1 | ksp: Localizer=8, ConfigNode=2 | fan_in=2
- methods(12): Awake, Fire, GetInfo, GetNodeBodyParameterString, Initialize, Load, OnautoClose, OnautoCloseTimeoutCreated, ParameterSetupComplete, Save, SendStateMessage, dialogClosed
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionMessage.cs
- L=58 kind=MonoBehaviour type=ActionMessage bases=ActionModule anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(27): Lifecycle=1 | ksp: Localizer=2, ConfigNode=2 | fan_in=0
- methods(5): Awake, Fire, GetInfo, Load, Save
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionMissionScore.cs
- L=114 kind=MonoBehaviour type=ActionMissionScore bases=ActionModule anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(27): Lifecycle=1, Coroutine=1 | ksp: Localizer=2, ConfigNode=2 | fan_in=4
- methods(12): Awake, Fire, GetAppObjectiveInfo, GetAwardedScoreDescription, GetInfo, GetInternalParametersToDisplay, GetScoreDescription, Initialize, Load, OnDockStatusChange, OnMissionScoreModuleCreated, Save
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionModifierFunds.cs
- L=74 kind=MonoBehaviour type=ActionModifierFunds bases=ActionModule anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(25): Lifecycle=1 | ksp: Funding=11, ConfigNode=2, Localizer=1 | fan_in=0
- methods(5): Awake, Fire, GetInfo, Load, Save
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionModifierScore.cs
- L=15 kind=MonoBehaviour type=ActionModifierScore bases=ActionMissionScore anc=ActionMissionScore/ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(15): none | ksp: none | fan_in=0
- methods(1): Initialize
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionPartExplode.cs
- L=139 kind=MonoBehaviour type=ActionPartExplode bases=ActionModule anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(36): Lifecycle=1, Coroutine=1 | ksp: FlightGlobals=6, Vessel=4, Localizer=4, ConfigNode=2, Part=1 | fan_in=1
- methods(10): Awake, Fire, GetInfo, GetNodeBodyParameterString, Load, OnPartPersistentIdChanged, OnVesselPersistentIdChanged, OnVesselsUndocking, RunValidation, Save
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionPartFailure.cs
- L=338 kind=MonoBehaviour type=ActionPartFailure bases=ActionModule anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(34): Coroutine=4, Lifecycle=2 | ksp: Localizer=13, GameEvents=5, Vessel=4, FlightGlobals=4, Part=3 | fan_in=2
- methods(19): Awake, DynamicModuleListControlCreated, Fire, GetInfo, GetInternalParametersToDisplay, GetNodeBodyParameterString, Initialize, Load, OnAdjusterListModified, OnDestroy, OnPartIDChanged, OnPartPersistentIdChanged, OnVesselPersistentIdChanged, OnVesselsUndocking, RunValidation, Save, VesselPartSelectorControlCreated, addAdjustersVessel, onVesselSituationChanged
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionPartRepair.cs
- L=527 kind=MonoBehaviour type=ActionPartRepair bases=ActionModule anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(46): GameObject=14, SetActive=9, Lifecycle=2, Coroutine=2, GetComponent=1 | ksp: Localizer=18, GameEvents=6, PartModule=4, ConfigNode=4, FlightGlobals=3 | fan_in=1
- methods(20): Awake, Fire, GetInfo, GetNodeBodyParameterString, Load, OnDestroy, OnFailureDropdownControlCreated, OnPartIDChanged, OnPartModuleDropdownControlCreated, OnRepairTypeControlCreated, OnRepairTypeValueChange, RepairFailures, RunValidation, Save, SetFailureNamesForDropdown, SetPartModuleNamesForDropdown, SetRepairTypeDropdown, VesselPartSelectorControlCreated, onFailureListChanged, onVesselSituationChanged
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionPartResourceAmount.cs
- L=182 kind=MonoBehaviour type=ActionPartResourceAmount bases=ActionVessel anc=ActionVessel/ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(32): Coroutine=2, Lifecycle=1 | ksp: PartResourceDefinition=4, Localizer=4, GameEvents=2, ConfigNode=2, ProtoPartSnapshot=1 | fan_in=0
- methods(9): Awake, Fire, GetInfo, GetNodeBodyParameterString, Load, OnResourceNameControlSetup, Save, SetResourceNames, adjustUnloadedPart
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionPartResourceDrain.cs
- L=406 kind=MonoBehaviour type=ActionPartResourceDrain bases=ActionModule anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(29): Coroutine=3, Lifecycle=1 | ksp: Localizer=11, FlightGlobals=5, Vessel=3, PartResourceDefinition=3, GameEvents=3 | fan_in=1
- methods(17): APRD_SetDropDownValues, Awake, Fire, GetInfo, GetNodeBodyParameterString, Load, OnPartPersistentIdChanged, OnVesselPersistentIdChanged, OnVesselsUndocking, RunValidation, Save, Setup, drainPart, drainUnloadedPart, drainVessel, findPart, findVessel
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionSetCamera.cs
- L=46 kind=MonoBehaviour type=ActionSetCamera bases=ActionModule anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(30): Lifecycle=1 | ksp: Localizer=2, ConfigNode=2 | fan_in=0
- methods(5): Awake, Fire, GetInfo, Load, Save
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionVessel.cs
- L=131 kind=MonoBehaviour type=ActionVessel bases=ActionModule anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(21): Lifecycle=1 | ksp: FlightGlobals=7, Vessel=5, Localizer=5, ConfigNode=2, ProtoPartSnapshot=1 | fan_in=2
- methods(8): Awake, GetNodeBodyParameterString, Load, OnVesselPersistentIdChanged, OnVesselsDocking, OnVesselsUndocking, Save, vesselSelectorCreated
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionVesselActionGroup.cs
- L=51 kind=MonoBehaviour type=ActionVesselActionGroup bases=ActionModule anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(61): Coroutine=3, UnityEngine.UI=1, Lifecycle=1 | ksp: Localizer=2, FlightGlobals=2, ConfigNode=2 | fan_in=0
- methods(6): Awake, Fire, GetInfo, Load, Save, actionGroupControlComplete
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionVesselExplode.cs
- L=76 kind=MonoBehaviour type=ActionVesselExplode bases=ActionVessel anc=ActionVessel/ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(45): Coroutine=3, Lifecycle=1 | ksp: Localizer=3, Part=2, ConfigNode=2 | fan_in=0
- methods(6): Awake, Fire, GetInfo, GetNodeBodyParameterString, Load, Save
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/ActionVesselStage.cs
- L=42 kind=MonoBehaviour type=ActionVesselStage bases=ActionModule anc=ActionModule/MonoBehaviour ns=Expansions.Missions.Actions
- unity(66): Coroutine=3, UnityEngine.UI=1, Lifecycle=1 | ksp: Localizer=2, FlightGlobals=2, ConfigNode=2 | fan_in=0
- methods(5): Awake, Fire, GetInfo, Load, Save
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Actions/FundsModifierType.cs
- L=18 kind=enum type=FundsModifierType bases=- anc=- ns=Expansions.Missions.Actions
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterActiveRadiatorBase.cs
- L=31 kind=class type=AdjusterActiveRadiatorBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=3
- methods(3): ApplyMaxEnergyTransferAdjustment, GetTargetPartModule, IsBlockingCooling
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterActiveRadiatorMaxEnergyTransfer.cs
- L=60 kind=class type=AdjusterActiveRadiatorMaxEnergyTransfer bases=AdjusterActiveRadiatorBase anc=AdjusterActiveRadiatorBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(2): none | ksp: Localizer=4, ConfigNode=2 | fan_in=0
- methods(5): Activate, ApplyMaxEnergyTransferAdjustment, GetNodeBodyParameterString, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterAlternatorBase.cs
- L=26 kind=class type=AdjusterAlternatorBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=2
- methods(2): ApplyOutputAdjustment, GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterAlternatorOutput.cs
- L=60 kind=class type=AdjusterAlternatorOutput bases=AdjusterAlternatorBase anc=AdjusterAlternatorBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(2): none | ksp: Localizer=3, ConfigNode=2 | fan_in=0
- methods(5): Activate, ApplyOutputAdjustment, GetNodeBodyParameterString, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterAnchoredDecouplerBase.cs
- L=26 kind=class type=AdjusterAnchoredDecouplerBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=1
- methods(2): GetTargetPartModule, IsBlockingDecouple
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterCargoBayBase.cs
- L=26 kind=class type=AdjusterCargoBayBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=2
- methods(2): GetTargetPartModule, IsCargoBayStuck
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterControlSurfaceActuatorSpeed.cs
- L=60 kind=class type=AdjusterControlSurfaceActuatorSpeed bases=AdjusterControlSurfaceBase anc=AdjusterControlSurfaceBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(2): none | ksp: Localizer=2, ConfigNode=2 | fan_in=0
- methods(5): Activate, ApplyActuatorSpeedAdjustment, GetNodeBodyParameterString, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterControlSurfaceBase.cs
- L=26 kind=class type=AdjusterControlSurfaceBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=3
- methods(2): ApplyActuatorSpeedAdjustment, GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterDataTransmitterBase.cs
- L=31 kind=class type=AdjusterDataTransmitterBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=3
- methods(3): ApplyPowerAdjustment, GetTargetPartModule, IsDataTransmitterBroken
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterDataTransmitterPower.cs
- L=75 kind=class type=AdjusterDataTransmitterPower bases=AdjusterDataTransmitterBase anc=AdjusterDataTransmitterBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(1): none | ksp: Localizer=3, ConfigNode=2 | fan_in=0
- methods(7): Activate, ApplyPowerAdjustment, GetNodeBodyParameterString, Load, PowerValueChanged, Save, UpdatePowerValueDisplay
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterDecoupleBase.cs
- L=26 kind=class type=AdjusterDecoupleBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=2
- methods(2): GetTargetPartModule, IsBlockingDecouple
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterDeployablePartBase.cs
- L=26 kind=class type=AdjusterDeployablePartBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=2
- methods(2): GetTargetPartModule, IsDeployablePartStuck
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterDeployableSolarPanelBase.cs
- L=26 kind=class type=AdjusterDeployableSolarPanelBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=2
- methods(2): ApplyEfficiencyAdjustment, GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterDeployableSolarPanelEfficiency.cs
- L=60 kind=class type=AdjusterDeployableSolarPanelEfficiency bases=AdjusterDeployableSolarPanelBase anc=AdjusterDeployableSolarPanelBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(2): none | ksp: Localizer=2, ConfigNode=2 | fan_in=0
- methods(5): Activate, ApplyEfficiencyAdjustment, GetNodeBodyParameterString, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterDockingNodeBase.cs
- L=26 kind=class type=AdjusterDockingNodeBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=2
- methods(2): GetTargetPartModule, IsBlockingUndock
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterEnginesBase.cs
- L=31 kind=class type=AdjusterEnginesBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=3
- methods(3): ApplyThrottleAdjustment, GetTargetPartModule, IsEngineDead
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterEnginesThrottle.cs
- L=60 kind=class type=AdjusterEnginesThrottle bases=AdjusterEnginesBase anc=AdjusterEnginesBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(2): none | ksp: Localizer=2, ConfigNode=2 | fan_in=0
- methods(5): Activate, ApplyThrottleAdjustment, GetNodeBodyParameterString, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterFuelLineBase.cs
- L=22 kind=class type=AdjusterFuelLineBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=1
- methods(1): GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterGeneratorBase.cs
- L=26 kind=class type=AdjusterGeneratorBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=2
- methods(2): ApplyEfficiencyAdjustment, GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterGeneratorEfficiency.cs
- L=60 kind=class type=AdjusterGeneratorEfficiency bases=AdjusterGeneratorBase anc=AdjusterGeneratorBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(2): none | ksp: Localizer=2, ConfigNode=2 | fan_in=0
- methods(5): Activate, ApplyEfficiencyAdjustment, GetNodeBodyParameterString, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterGimbalBase.cs
- L=27 kind=class type=AdjusterGimbalBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(6): none | ksp: none | fan_in=2
- methods(2): ApplyControlAdjustment, GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterGimbalLimitControl.cs
- L=91 kind=class type=AdjusterGimbalLimitControl bases=AdjusterGimbalBase anc=AdjusterGimbalBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(2): none | ksp: Localizer=4, ConfigNode=2 | fan_in=0
- methods(5): Activate, ApplyControlAdjustment, GetNodeBodyParameterString, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterGPSBase.cs
- L=21 kind=class type=AdjusterGPSBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=1
- methods(1): GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterGrappleNodeBase.cs
- L=31 kind=class type=AdjusterGrappleNodeBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=3
- methods(3): GetTargetPartModule, IsBlockingGrappleGrab, IsBlockingGrappleRelease
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterKerbNetAccessBase.cs
- L=21 kind=class type=AdjusterKerbNetAccessBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=1
- methods(1): GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterKerbNetAccessDisabled.cs
- L=38 kind=class type=AdjusterKerbNetAccessDisabled bases=AdjusterKerbNetAccessBase anc=AdjusterKerbNetAccessBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=1 | fan_in=0
- methods(1): Activate
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterLiftingSurfaceBase.cs
- L=27 kind=class type=AdjusterLiftingSurfaceBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(6): none | ksp: none | fan_in=2
- methods(2): ApplyLiftForceAdjustment, GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterLiftingSurfaceLiftForce.cs
- L=60 kind=class type=AdjusterLiftingSurfaceLiftForce bases=AdjusterLiftingSurfaceBase anc=AdjusterLiftingSurfaceBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(5): none | ksp: Localizer=2, ConfigNode=2 | fan_in=0
- methods(5): Activate, ApplyLiftForceAdjustment, GetNodeBodyParameterString, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterLightBase.cs
- L=26 kind=class type=AdjusterLightBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=4
- methods(2): ApplyIntensityAdjustment, GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterLightIntensity.cs
- L=60 kind=class type=AdjusterLightIntensity bases=AdjusterLightBase anc=AdjusterLightBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(2): none | ksp: Localizer=2, ConfigNode=2 | fan_in=0
- methods(5): Activate, ApplyIntensityAdjustment, GetNodeBodyParameterString, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterPartModuleBase.cs
- L=279 kind=class type=AdjusterPartModuleBase bases=DynamicModule,IConfigNode,IPartModuleAdjuster anc=DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(8): none | ksp: ConfigNode=4, PartModule=2, Localizer=2, KSPField=1, KSPEvent=1 | fan_in=59
- methods(17): Activate, CleanUp, CreateInstanceOfModuleAdjuster, CreateModuleAdjusterList, Deactivate, GetDisplayName, GetNodeBodyParameterString, GetPartModuleTargetList, GetPartModulesThatCanBeAffected, GetTargetPartModule, Load, ModularSetup, OnPartModuleAdjusterListModified, RemoveAdjuster, Save, Setup, UpdateStatusMessage
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterProbeControlPointBase.cs
- L=27 kind=class type=AdjusterProbeControlPointBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=2
- methods(2): GetTargetPartModule, IsProbeControlPointBroken
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterRCSBase.cs
- L=37 kind=class type=AdjusterRCSBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(10): none | ksp: none | fan_in=3
- methods(4): ApplyInputLinearAdjustment, ApplyInputRotationAdjustment, GetTargetPartModule, IsRCSBroken
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterReactionWheelBase.cs
- L=32 kind=class type=AdjusterReactionWheelBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(6): none | ksp: none | fan_in=4
- methods(3): ApplyTorqueAdjustment, GetTargetPartModule, IsReactionWheelBroken
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterReactionWheelTorque.cs
- L=91 kind=class type=AdjusterReactionWheelTorque bases=AdjusterReactionWheelBase anc=AdjusterReactionWheelBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(2): none | ksp: Localizer=5, ConfigNode=2 | fan_in=0
- methods(5): Activate, ApplyTorqueAdjustment, GetNodeBodyParameterString, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterResourceIntakeBase.cs
- L=21 kind=class type=AdjusterResourceIntakeBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=1
- methods(1): GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterSASBase.cs
- L=26 kind=class type=AdjusterSASBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=2
- methods(2): ApplySASServiceLevelAdjustment, GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterSASServiceLevel.cs
- L=54 kind=class type=AdjusterSASServiceLevel bases=AdjusterSASBase anc=AdjusterSASBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=2, ConfigNode=2 | fan_in=0
- methods(5): Activate, ApplySASServiceLevelAdjustment, GetNodeBodyParameterString, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterScienceExperimentBase.cs
- L=21 kind=class type=AdjusterScienceExperimentBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=1
- methods(1): GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterToggleCrossfeedBase.cs
- L=21 kind=class type=AdjusterToggleCrossfeedBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=1
- methods(1): GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterWheelBrakesBase.cs
- L=27 kind=class type=AdjusterWheelBrakesBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=2
- methods(2): ApplyTorqueAdjustment, GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterWheelBrakesTorque.cs
- L=60 kind=class type=AdjusterWheelBrakesTorque bases=AdjusterWheelBrakesBase anc=AdjusterWheelBrakesBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(2): none | ksp: Localizer=2, ConfigNode=2 | fan_in=0
- methods(5): Activate, ApplyTorqueAdjustment, GetNodeBodyParameterString, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterWheelDeploymentBase.cs
- L=22 kind=class type=AdjusterWheelDeploymentBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=1
- methods(1): GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterWheelMotorBase.cs
- L=27 kind=class type=AdjusterWheelMotorBase bases=AdjusterPartModuleBase anc=AdjusterPartModuleBase/DynamicModule/IConfigNode ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=2
- methods(2): ApplyTorqueAdjustment, GetTargetPartModule
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterWheelMotorTorque.cs
- L=60 kind=class type=AdjusterWheelMotorTorque bases=AdjusterWheelMotorBase anc=AdjusterWheelMotorBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(2): none | ksp: Localizer=2, ConfigNode=2 | fan_in=0
- methods(5): Activate, ApplyTorqueAdjustment, GetNodeBodyParameterString, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureActiveRadiatorStuck.cs
- L=79 kind=class type=FailureActiveRadiatorStuck bases=AdjusterActiveRadiatorBase anc=AdjusterActiveRadiatorBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: ConfigNode=2, Localizer=1 | fan_in=0
- methods(4): Activate, IsBlockingCooling, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureAnchoredDecouplerCantDecouple.cs
- L=37 kind=class type=FailureAnchoredDecouplerCantDecouple bases=AdjusterAnchoredDecouplerBase anc=AdjusterAnchoredDecouplerBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=3 | fan_in=0
- methods(2): Activate, IsBlockingDecouple
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureCargoBayStuck.cs
- L=72 kind=class type=FailureCargoBayStuck bases=AdjusterCargoBayBase anc=AdjusterCargoBayBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: ConfigNode=2, Localizer=1 | fan_in=0
- methods(5): Activate, Deactivate, IsCargoBayStuck, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureControlSurfaceDisableControl.cs
- L=37 kind=class type=FailureControlSurfaceDisableControl bases=AdjusterControlSurfaceBase anc=AdjusterControlSurfaceBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=3 | fan_in=0
- methods(2): Activate, ApplyActuatorSpeedAdjustment
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureCrossfeedStuck.cs
- L=64 kind=class type=FailureCrossfeedStuck bases=AdjusterToggleCrossfeedBase anc=AdjusterToggleCrossfeedBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: ConfigNode=2, Localizer=1 | fan_in=0
- methods(3): Activate, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureDataTransmitterBroken.cs
- L=42 kind=class type=FailureDataTransmitterBroken bases=AdjusterDataTransmitterBase anc=AdjusterDataTransmitterBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=1 | fan_in=0
- methods(2): Activate, IsDataTransmitterBroken
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureDecoupleCantDecouple.cs
- L=37 kind=class type=FailureDecoupleCantDecouple bases=AdjusterDecoupleBase anc=AdjusterDecoupleBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=1 | fan_in=0
- methods(2): Activate, IsBlockingDecouple
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureDeployablePartStuck.cs
- L=69 kind=class type=FailureDeployablePartStuck bases=AdjusterDeployablePartBase anc=AdjusterDeployablePartBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: ConfigNode=2, Localizer=1 | fan_in=0
- methods(4): Activate, IsDeployablePartStuck, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureDockingNodeCantUndock.cs
- L=31 kind=class type=FailureDockingNodeCantUndock bases=AdjusterDockingNodeBase anc=AdjusterDockingNodeBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=1 | fan_in=0
- methods(2): Activate, IsBlockingUndock
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureEnginesDeadEngine.cs
- L=45 kind=class type=FailureEnginesDeadEngine bases=AdjusterEnginesBase anc=AdjusterEnginesBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=1 | fan_in=0
- methods(2): Activate, IsEngineDead
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureFuelLineBlocked.cs
- L=48 kind=class type=FailureFuelLineBlocked bases=AdjusterFuelLineBase anc=AdjusterFuelLineBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=1 | fan_in=0
- methods(2): Activate, Deactivate
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureGPSNoDisplay.cs
- L=32 kind=class type=FailureGPSNoDisplay bases=AdjusterGPSBase anc=AdjusterGPSBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=3 | fan_in=0
- methods(1): Activate
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureGrappleNodeCantGrapple.cs
- L=31 kind=class type=FailureGrappleNodeCantGrapple bases=AdjusterGrappleNodeBase anc=AdjusterGrappleNodeBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=1 | fan_in=0
- methods(2): Activate, IsBlockingGrappleGrab
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureGrappleNodeCantRelease.cs
- L=46 kind=class type=FailureGrappleNodeCantRelease bases=AdjusterGrappleNodeBase anc=AdjusterGrappleNodeBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=3 | fan_in=0
- methods(3): Activate, Deactivate, IsBlockingGrappleRelease
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureLightFlickering.cs
- L=89 kind=class type=FailureLightFlickering bases=AdjusterLightBase anc=AdjusterLightBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(8): Time.=1 | ksp: Localizer=4, ConfigNode=2 | fan_in=0
- methods(6): Activate, ApplyIntensityAdjustment, GetNodeBodyParameterString, Load, Save, UpdateFlickeringData
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureLightStuck.cs
- L=64 kind=class type=FailureLightStuck bases=AdjusterLightBase anc=AdjusterLightBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: ConfigNode=2, Localizer=1 | fan_in=0
- methods(3): Activate, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureProbeControlPointBroken.cs
- L=29 kind=class type=FailureProbeControlPointBroken bases=AdjusterProbeControlPointBase anc=AdjusterProbeControlPointBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=0
- methods(1): IsProbeControlPointBroken
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureRCSFireRandomly.cs
- L=43 kind=class type=FailureRCSFireRandomly bases=AdjusterRCSBase anc=AdjusterRCSBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(17): none | ksp: Localizer=1 | fan_in=0
- methods(3): Activate, ApplyInputLinearAdjustment, ApplyInputRotationAdjustment
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureRCSStopWorking.cs
- L=37 kind=class type=FailureRCSStopWorking bases=AdjusterRCSBase anc=AdjusterRCSBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=1 | fan_in=0
- methods(2): Activate, IsRCSBroken
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureReactionWheelBroken.cs
- L=37 kind=class type=FailureReactionWheelBroken bases=AdjusterReactionWheelBase anc=AdjusterReactionWheelBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=1 | fan_in=0
- methods(2): Activate, IsReactionWheelBroken
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureReactionWheelStuck.cs
- L=63 kind=class type=FailureReactionWheelStuck bases=AdjusterReactionWheelBase anc=AdjusterReactionWheelBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(6): none | ksp: ConfigNode=2, Localizer=1 | fan_in=0
- methods(4): Activate, ApplyTorqueAdjustment, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureResourceIntakeStuck.cs
- L=64 kind=class type=FailureResourceIntakeStuck bases=AdjusterResourceIntakeBase anc=AdjusterResourceIntakeBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: ConfigNode=2, Localizer=1 | fan_in=0
- methods(3): Activate, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureScienceExperimentDisable.cs
- L=32 kind=class type=FailureScienceExperimentDisable bases=AdjusterScienceExperimentBase anc=AdjusterScienceExperimentBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: Localizer=1 | fan_in=0
- methods(1): Activate
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureWheelDeploymentStuck.cs
- L=65 kind=class type=FailureWheelDeploymentStuck bases=AdjusterWheelDeploymentBase anc=AdjusterWheelDeploymentBase/AdjusterPartModuleBase/DynamicModule ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: ConfigNode=2, Localizer=1 | fan_in=0
- methods(3): Activate, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/IPartModuleAdjuster.cs
- L=6 kind=interface type=IPartModuleAdjuster bases=- anc=- ns=Expansions.Missions.Adjusters
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Asteroid.cs
- L=98 kind=class type=Asteroid bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(23): GameObject=2, GetComponent=1 | ksp: ConfigNode=3, FlightGlobals=1 | fan_in=8
- methods(6): GetAsteroidSeed, Load, OnGAPPrefabInstantiated, Randomize, Save, SetGAPAsteroid
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardDefinition.cs
- L=44 kind=class type=AwardDefinition bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(41): Texture=2 | ksp: GameDatabase=2, ConfigNode=2 | fan_in=14
- methods(2): Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule.cs
- L=98 kind=class type=AwardModule bases=DynamicModule anc=DynamicModule/IConfigNode ns=Expansions.Missions
- unity(4): none | ksp: MissionSystem=2, ConfigNode=2, Localizer=1, HighLogic=1 | fan_in=13
- methods(7): EvaluateAward, EvaluateCondition, GetDisplayName, Load, Save, StartTracking, StopTracking
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_Fuel.cs
- L=148 kind=class type=AwardModule_Fuel bases=AwardModule anc=AwardModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: Vessel=8, ConfigNode=7, GameEvents=4, PartResourceDefinition=3, HighLogic=2 | fan_in=0
- methods(9): EvaluateCondition, GetCurrentResourceFromVessel, Load, OnVesselCreated, OnVesselWillDestroy, Save, SetResourceNames, StartTracking, StopTracking
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_KerbalsDied.cs
- L=30 kind=class type=AwardModule_KerbalsDied bases=AwardModule anc=AwardModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: ProtoCrewMember=1, KerbalRoster=1, HighLogic=1 | fan_in=0
- methods(1): EvaluateCondition
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_KerbalsSurvived.cs
- L=57 kind=class type=AwardModule_KerbalsSurvived bases=AwardModule anc=AwardModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: ConfigNode=4, HighLogic=2, GameEvents=2, Game=1 | fan_in=0
- methods(6): EvaluateCondition, Load, OnCrewKilled, Save, StartTracking, StopTracking
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_LandedOn.cs
- L=75 kind=class type=AwardModule_LandedOn bases=AwardModule anc=AwardModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(5): none | ksp: Vessel=4, FlightGlobals=4, ConfigNode=2, CelestialBody=1 | fan_in=0
- methods(4): EvaluateCondition, GetVessel, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_MissionTime.cs
- L=37 kind=class type=AwardModule_MissionTime bases=AwardModule anc=AwardModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: ConfigNode=2, HighLogic=1 | fan_in=0
- methods(3): EvaluateCondition, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_PlanetsLanded.cs
- L=73 kind=class type=AwardModule_PlanetsLanded bases=AwardModule anc=AwardModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: ConfigNode=5, Vessel=3, GameEvents=3, HighLogic=2, Game=1 | fan_in=0
- methods(6): EvaluateCondition, Load, OnVesselSitutationChange, Save, StartTracking, StopTracking
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_Score.cs
- L=38 kind=class type=AwardModule_Score bases=AwardModule anc=AwardModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: ConfigNode=2 | fan_in=1
- methods(3): EvaluateCondition, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_TraveledLessThan.cs
- L=68 kind=class type=AwardModule_TraveledLessThan bases=AwardModule anc=AwardModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: ConfigNode=4, HighLogic=2, GameEvents=2, FlightGlobals=2, Vessel=1 | fan_in=0
- methods(6): EvaluateCondition, Load, OnVesselWillDestroy, Save, StartTracking, StopTracking
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_TraveledMoreThan.cs
- L=68 kind=class type=AwardModule_TraveledMoreThan bases=AwardModule anc=AwardModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: ConfigNode=4, HighLogic=2, GameEvents=2, FlightGlobals=2, Vessel=1 | fan_in=0
- methods(6): EvaluateCondition, Load, OnVesselWillDestroy, Save, StartTracking, StopTracking
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardModule_VesselsUsed.cs
- L=75 kind=class type=AwardModule_VesselsUsed bases=AwardModule anc=AwardModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: ConfigNode=5, HighLogic=2, GameEvents=2, Vessel=1, Game=1 | fan_in=0
- methods(6): EvaluateCondition, Load, OnVesselChange, Save, StartTracking, StopTracking
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Awards.cs
- L=43 kind=MonoBehaviour type=Awards bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions
- unity(51): MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=6
- methods(3): Awake, GetAwardDefinition, SetupAwards
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/AwardWidget.cs
- L=49 kind=MonoBehaviour type=AwardWidget bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions
- unity(87): GameObject=4, TMPro=2, Instantiate/Destroy=2, GetComponent=2, UnityEngine.UI=1, Transform=1 | ksp: MissionSystem=1 | fan_in=2
- methods(2): Create, Start
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/CheckpointValidationType.cs
- L=10 kind=enum type=CheckpointValidationType bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=17
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Comet.cs
- L=146 kind=class type=Comet bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(17): GameObject=2, GetComponent=1 | ksp: ConfigNode=3, FlightGlobals=1 | fan_in=7
- methods(8): GetRandomCometSeed, Load, OnCometTypeChanged, OnGAPPrefabInstantiated, Randomize, Save, SetCometTypes, SetGAPComet
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Crew.cs
- L=66 kind=class type=Crew bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(6): none | ksp: Localizer=4, ConfigNode=3 | fan_in=15
- methods(3): Load, LocalizeCrewMember, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/DynamicModule.cs
- L=216 kind=class type=DynamicModule bases=IConfigNode,IMENodeDisplay,ICloneable anc=IConfigNode ns=Expansions.Missions
- unity(4): Instantiate/Destroy=1 | ksp: ConfigNode=2, Localizer=1 | fan_in=9
- methods(27): AddParameterToNodeBody, AddParameterToNodeBodyAndUpdateUI, AddParameterToSAP, Clone, Destroy, Equals, GetDefaultPinnedParameters, GetDisplayName, GetDisplayToolTip, GetHashCode, GetInfo, GetInternalParametersToDisplay, GetModuleInstanceID, GetName, GetNode, GetNodeBodyParameterString, HasNodeBodyParameter, HasSAPParameter, Initialize, Load, ParameterSetupComplete, RemoveParameterFromNodeBody, RemoveParameterFromNodeBodyAndUpdateUI, RemoveParameterFromSAP ...
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/DynamicModuleList.cs
- L=150 kind=class type=DynamicModuleList bases=IConfigNode,ICloneable anc=IConfigNode ns=Expansions.Missions
- unity(3): none | ksp: ConfigNode=8 | fan_in=5
- methods(10): AddDynamicModule, Clone, ContainsModule, Equals, GetHashCode, GetInternalParameters, GetSupportedTypes, Load, Save, SetSupportedTypes
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/ActionPaneDisplay.cs
- L=349 kind=UIBehaviour type=ActionPaneDisplay bases=Selectable,IPointerClickHandler,IEventSystemHandler anc=Selectable ns=Expansions.Missions.Editor
- unity(89): UnityEngine.UI=22, EventSystems=16, Raycast/Query=11, Texture=6, GetComponent=6, MonoBehaviour=5 | ksp: none | fan_in=6
- methods(28): AddObject, AddToolbarButton, AddToolbarToggle, Awake, Clean, ClearToolbar, ClearToolbarEvents, Destroy, GetMousePointOnCamera, GetToolbarButton, OnBeginDrag, OnDeselect, OnDisplayClick, OnDisplayClickUp, OnDisplayDrag, OnDisplayDragEnd, OnDrag, OnEndDrag, OnMouseOver, OnPointerClick, OnPointerEnter, OnPointerExit, OnPointerUp, OnSelect ...
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/ActionPaneDisplay_BiSelector.cs
- L=22 kind=UIBehaviour type=ActionPaneDisplay_BiSelector bases=ActionPaneDisplay anc=ActionPaneDisplay/Selectable ns=Expansions.Missions.Editor
- unity(92): Transform=3, UnityEngine.UI=2, GetComponent=2, GameObject=2, Lifecycle=1, Instantiate/Destroy=1 | ksp: none | fan_in=0
- methods(1): Awake
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/ActionPaneDisplay_CelestialBody.cs
- L=24 kind=UIBehaviour type=ActionPaneDisplay_CelestialBody bases=ActionPaneDisplay anc=ActionPaneDisplay/Selectable ns=Expansions.Missions.Editor
- unity(90): Transform=3, UnityEngine.UI=2, GetComponent=2, Lifecycle=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=1
- methods(1): Awake
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/AreaState.cs
- L=9 kind=enum type=AreaState bases=- anc=- ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/BaseAPField.cs
- L=366 kind=class type=BaseAPField bases=BaseField<MEGUI_Control> anc=BaseField/BaseField ns=Expansions.Missions.Editor
- unity(6): none | ksp: none | fan_in=82
- methods(3): GetValue, SetOriginalValue, SetValue
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/BaseAPFieldList.cs
- L=14 kind=class type=BaseAPFieldList bases=BaseFieldList<BaseAPField,MEGUI_Control> anc=BaseFieldList/IEnumerable ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=13
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/Category.cs
- L=6 kind=class type=Category bases=- anc=- ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/CheckpointBrowserDialog.cs
- L=449 kind=MonoBehaviour type=CheckpointBrowserDialog bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(59): UnityEngine.UI=13, GameObject=4, SetActive=3, Transform=2, Instantiate/Destroy=2, GetComponent=2 | ksp: Localizer=19, ConfigNode=8, InputLockManager=3, GameEvents=3, MissionSystem=1 | fan_in=1
- methods(32): AddMissionEntryWidget, BuildCheckpointList, CancelledCallback, ClearMissionList, ClearSelection, DeleteEntry, Dismiss, Hide, InjectMissionDataIntoCheckpoint, Lock, MissionCompare, OnButtonCancel, OnButtonRemoveDirty, OnButtonReset, OnButtonTest, OnDeleteConfirm, OnDeleteDismiss, OnDeleteEntry, OnEntrySelected, OnInputLocksModified, OnRemoveDirtyConfirm, OnRemoveDirtyDismiss, OnResetConfirm, OnSelectionChanged ...
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/CheckpointEntry.cs
- L=230 kind=MonoBehaviour type=CheckpointEntry bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(58): UnityEngine.UI=4, TMPro=3, SetActive=2, Instantiate/Destroy=2, GameObject=2, MonoBehaviour=1 | ksp: Localizer=12, ConfigNode=8, Game=6, FlightGlobals=1 | fan_in=1
- methods(10): Create, GetMissionNode, GetSaveGame, GetSaveGameMeta, Init, LoadMission, OnButtonRemove, OnValueChanged, Terminate, Validate
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/CrewCreationDialog.cs
- L=227 kind=MonoBehaviour type=CrewCreationDialog bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(71): UnityEngine.UI=9, SetActive=5, GameObject=5, TMPro=4, Lifecycle=2, GetComponent=2 | ksp: ProtoCrewMember=21, GameDatabase=6, Localizer=4, KerbalRoster=4, HighLogic=2 | fan_in=1
- methods(14): CancelledCallback, CreateCallback, Dismiss, Hide, OnButtonCancel, OnButtonCreate, OnButtonRandom, OnDestroy, OnRoleChanged, ResetValues, SetValues, Setup, Show, Start
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/DisplayParameter.cs
- L=18 kind=struct type=DisplayParameter bases=- anc=- ns=Expansions.Missions.Editor
- unity(12): none | ksp: ConfigNode=1 | fan_in=4
- methods(1): Load
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBody.cs
- L=634 kind=UIBehaviour type=GAPCelestialBody bases=ActionPaneDisplay_CelestialBody anc=ActionPaneDisplay_CelestialBody/ActionPaneDisplay/Selectable ns=Expansions.Missions.Editor
- unity(69): GameObject=14, Transform=9, SetActive=9, LayerMask=9, Instantiate/Destroy=8, Raycast/Query=7 | ksp: CelestialBody=8, Vector3d=7, Localizer=5, ScaledSpace=4, Orbit=2 | fan_in=20
- methods(33): CallbackPQSLoaded, Clean, Destroy, DisplayResource, DoLocalSpaceRay, DoLocalSpaceRayAll, GetLightTimeCycle, GetUTFormattedString, Initialize, LateUpdate, Load, LoadPQS, LoadPQSCities, LoadPlanet, OnDisplayClick, OnDisplayClickUp, OnDisplayDrag, OnDisplayDragEnd, OnMouseOver, OnResourcesButtonClicked, SetFooterText, SetState, SliderAction_LightCycle, SliderAction_Zoom ...
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBody_InteractiveSurfaceGizmo.cs
- L=182 kind=MonoBehaviour type=GAPCelestialBody_InteractiveSurfaceGizmo bases=GAPCelestialBody_SurfaceGizmo anc=GAPCelestialBody_SurfaceGizmo/MonoBehaviour ns=Expansions.Missions.Editor
- unity(56): Transform=5, GameObject=4, Texture=2, Collider=2, SetActive=1 | ksp: CelestialBody=4, Vector3d=1 | fan_in=7
- methods(20): GetFocusTarget, GetFooterText, Initialize, IsActive, IsDragReady, OnButtonPressed_FocusTarget, OnGAPClick, OnGAPDrag, OnGAPDragEnd, OnGAPOver, OnGizmoHandleDrag, OnGizmoHandleDragEnd, OnGizmoHandleOver, OnGizmoHandleSelected, OnLoadPlanet, OnPointTranslate, OnUpdate, SetGizmoPosition, TryEndHover, TrySelectHandle
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBody_SurfaceGizmo.cs
- L=100 kind=MonoBehaviour type=GAPCelestialBody_SurfaceGizmo bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(52): Transform=5, MonoBehaviour=1, Lifecycle=1 | ksp: Vector3d=6, CelestialBody=3, QuaternionD=2 | fan_in=3
- methods(8): Initialize, IsActive, OnDestroy, OnLoadPlanet, OnUpdate, SetGizmoPosition, SetInSurfaceFromPoint, SetInSurfaceFromPointAtSeaLevel
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBody_SurfaceGizmo_Area.cs
- L=212 kind=MonoBehaviour type=GAPCelestialBody_SurfaceGizmo_Area bases=GAPCelestialBody_InteractiveSurfaceGizmo anc=GAPCelestialBody_InteractiveSurfaceGizmo/GAPCelestialBody_SurfaceGizmo/MonoBehaviour ns=Expansions.Missions.Editor
- unity(45): Transform=2, Shader=2, Material=2, GetComponent=1, Collider=1 | ksp: CelestialBody=3, Vector3d=1, Localizer=1 | fan_in=1
- methods(14): CheckAreaSelected, GetFooterText, Initialize, IsActive, OnGAPClick, OnGAPDrag, OnGAPDragEnd, OnGAPOver, OnGizmoHandleSelected, OnLoadPlanet, OnRadiusChange, OnUpdate, SetAreaColor, SetAreaRadius
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBody_SurfaceGizmo_Icon.cs
- L=24 kind=MonoBehaviour type=GAPCelestialBody_SurfaceGizmo_Icon bases=GAPCelestialBody_SurfaceGizmo anc=GAPCelestialBody_SurfaceGizmo/MonoBehaviour ns=Expansions.Missions.Editor
- unity(27): none | ksp: CelestialBody=1 | fan_in=1
- methods(1): Initialize
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBody_SurfaceGizmo_PlaceVessel.cs
- L=191 kind=MonoBehaviour type=GAPCelestialBody_SurfaceGizmo_PlaceVessel bases=GAPCelestialBody_InteractiveSurfaceGizmo anc=GAPCelestialBody_InteractiveSurfaceGizmo/GAPCelestialBody_SurfaceGizmo/MonoBehaviour ns=Expansions.Missions.Editor
- unity(63): Transform=8, SetActive=6, GameObject=3, Renderer=1, Input.=1 | ksp: Localizer=1 | fan_in=1
- methods(15): AddTransformButtons, ClearMode, GetFooterText, GetGizmoCenterAngle, Initialize, OnGizmoHandleDrag, OnGizmoHandleDragEnd, OnGizmoHandleSelected, OnPointRotate, OnUpdate, SelectRotate, SelectTranslate, SetGizmoIcon, SetGizmoRotation, SetMode
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBody_SurfaceGizmo_Volume.cs
- L=421 kind=MonoBehaviour type=GAPCelestialBody_SurfaceGizmo_Volume bases=GAPCelestialBody_InteractiveSurfaceGizmo anc=GAPCelestialBody_InteractiveSurfaceGizmo/GAPCelestialBody_SurfaceGizmo/MonoBehaviour ns=Expansions.Missions.Editor
- unity(57): SetActive=8, Transform=7, GameObject=7, Material=4, Shader=2, AnimationCurve=2 | ksp: CelestialBody=12, Localizer=3, Vector3d=2 | fan_in=1
- methods(24): GetFooterText, HideShape, Initialize, IsActive, OnButtonPressed_Shape, OnDoubleSliderValue, OnGAPClick, OnGAPDrag, OnGAPDragEnd, OnGAPOver, OnGAPShapeBoundsChange, OnGAPShapeChange, OnGAPValueChange, OnGizmoHandleSelected, OnLoadPlanet, OnSimpleSliderValue, OnUpdate, SetAreaColor, SetConeBounds, SetShape, UpdateAreaState, UpdateFooterText_Cone, UpdateFooterText_Sphere, UpdateLineRenderer
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBodyGizmoHandle.cs
- L=52 kind=MonoBehaviour type=GAPCelestialBodyGizmoHandle bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(87): Shader=5, Material=2, Renderer=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=4
- methods(5): OnHoverEnd, OnHoverStart, SetAlpha, SetColor, Start
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBodyState.cs
- L=10 kind=enum type=GAPCelestialBodyState bases=- anc=- ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=7
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBodyState_Base.cs
- L=489 kind=class type=GAPCelestialBodyState_Base bases=- anc=- ns=Expansions.Missions.Editor
- unity(30): GameObject=5, Instantiate/Destroy=4, Transform=3, SetActive=3, Raycast/Query=3, Lifecycle=2 | ksp: Vessel=17, Orbit=9, Localizer=3, CelestialBody=3, QuaternionD=1 | fan_in=4
- methods(26): ClearAdditionalInfo, CreateSimpleOrbit, CreateSurfaceAreaGizmo, CreateSurfaceIcon, End, Init, LateUpdate, LoadAdditionalInfo, LoadPlanet, OnClick, OnClickUp, OnDrag, OnDragEnd, OnFilterButton, OnFilterButton_Asteroids, OnFilterButton_Comets, OnFilterButton_Flags, OnFilterButton_Kerbals, OnFilterButton_LaunchSites, OnFilterButton_Objectives, OnFilterButton_Vessels, OnMouseOver, SetFilter, UnloadPlanet ...
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBodyState_Biomes.cs
- L=183 kind=class type=GAPCelestialBodyState_Biomes bases=GAPCelestialBodyState_Base anc=GAPCelestialBodyState_Base ns=Expansions.Missions.Editor
- unity(57): Shader=9, Material=3, Transform=2, Renderer=2, GetComponent=2, Time.=1 | ksp: CelestialBody=17, Localizer=1 | fan_in=2
- methods(13): End, GetBiomeIndex, HasSelectedBiome, Init, LoadPlanet, OnBiomeSelected, OnClickUp, OnMouseOver, SelectBiome, SelectBiomeByName, UnloadPlanet, Update, UpdateGAPText
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBodyState_Orbit.cs
- L=266 kind=class type=GAPCelestialBodyState_Orbit bases=GAPCelestialBodyState_Base anc=GAPCelestialBodyState_Base ns=Expansions.Missions.Editor
- unity(43): Instantiate/Destroy=4, Transform=3, Raycast/Query=3, Input.=3, GameObject=3, SetActive=1 | ksp: Vector3d=8, CelestialBody=3, ScaledSpace=2, Localizer=1 | fan_in=4
- methods(21): CalculateBodyEditorAngle, CreateMapNode, End, GizmoDragCastHit, Init, LateUpdate, LoadPlanet, OnClick, OnDrag, OnMouseOver, OnUpdateCaptionUser, OnUpdatePositionHover, OnUpdatePositionToUI, OnUpdatePositionUser, OnUpdateType, OnUpdateVisibleHover, OnUpdateVisibleUser, ToggleMeanAnomalyIcon, UnloadPlanet, UpdateGizmoPosition, UpdateOrbit
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBodyState_SurfaceGizmo.cs
- L=160 kind=class type=GAPCelestialBodyState_SurfaceGizmo bases=GAPCelestialBodyState_Base anc=GAPCelestialBodyState_Base ns=Expansions.Missions.Editor
- unity(48): Raycast/Query=4, Input.=3, Instantiate/Destroy=2, GameObject=2, Lifecycle=1, GetComponent=1 | ksp: CelestialBody=5, Vector3d=4, QuaternionD=1 | fan_in=1
- methods(13): ButtonAction_Recenter, End, GetSurfaceLocation, Init, InstantiateGizmo, LoadPlanet, OnClickUp, OnDrag, OnDragEnd, OnMouseOver, UnloadPlanet, Update, UpdateFooterText
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPOrbitRenderer.cs
- L=248 kind=MonoBehaviour type=GAPOrbitRenderer bases=OrbitRendererBase anc=OrbitRendererBase/MonoBehaviour ns=Expansions.Missions.Editor
- unity(65): Instantiate/Destroy=8, GameObject=7, Transform=4, Shader=2, Lifecycle=2, GetComponent=2 | ksp: MapView=4, Vector3d=2, OrbitDriver=2, ScaledSpace=1, Orbit=1 | fan_in=2
- methods(13): AttachNodeUIs, AttachObjectNode, Create, Destroy, Display, DrawOrbit, DrawSpline, LateUpdate, OnUpdateCaption, OnUpdatePosition, OnUpdatePositionToUI, OnUpdateVisibleHover, Start
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPPartPicker.cs
- L=46 kind=UIBehaviour type=GAPPartPicker bases=ActionPaneDisplay anc=ActionPaneDisplay/Selectable ns=Expansions.Missions.Editor
- unity(85): Instantiate/Destroy=3, GameObject=3, UnityEngine.UI=2, SetActive=2, Transform=1, GetComponent=1 | ksp: none | fan_in=1
- methods(3): Clean, Destroy, Setup
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPPrefabDisplay.cs
- L=103 kind=UIBehaviour type=GAPPrefabDisplay bases=ActionPaneDisplay anc=ActionPaneDisplay/Selectable ns=Expansions.Missions.Editor
- unity(90): GameObject=10, Instantiate/Destroy=9, SetActive=3, GetComponent=3, Transform=2, LayerMask=2 | ksp: none | fan_in=5
- methods(6): Clean, Destroy, LoadTarget, OnDisplayDrag, Setup, Update
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPSurfaceIcon.cs
- L=93 kind=class type=GAPSurfaceIcon bases=- anc=- ns=Expansions.Missions.Editor
- unity(33): Transform=3, UnityEngine.UI=1, Instantiate/Destroy=1 | ksp: Vector3d=3, CelestialBody=2 | fan_in=1
- methods(8): CreateMapNode, Destroy, Display, OnUpdate, OnUpdateCaption, OnUpdatePosition, OnUpdatePositionToUI, OnUpdateVisibleHover
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPUtil_CelestialBody.cs
- L=222 kind=MonoBehaviour type=GAPUtil_CelestialBody bases=GAPUtil_BiSelector anc=GAPUtil_BiSelector/MonoBehaviour ns=Expansions.Missions.Editor
- unity(68): UnityEngine.UI=5, SetActive=5, GameObject=5, Lifecycle=3, Light=2, EventSystems=2 | ksp: none | fan_in=3
- methods(14): Awake, ClearEvents, ClearHoveredSlider, InitializeSliderEvents, OnDestroy, OnPointerDown, OnPointerUp, OnSliderHovered, OnSliderMouseDown, OnSliderMouseEnter, OnSliderMouseExit, OnSliderMouseUp, ScrollWithButtons, Update
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPUtil_VesselFrame.cs
- L=100 kind=MonoBehaviour type=GAPUtil_VesselFrame bases=GAPUtil_BiSelector anc=GAPUtil_BiSelector/MonoBehaviour ns=Expansions.Missions.Editor
- unity(54): Lifecycle=2, EventSystems=2, UnityEngine.UI=1 | ksp: Localizer=2, ShipConstruct=1 | fan_in=2
- methods(9): Awake, OnCategoryValueChange, OnDestroy, OnPointerDown, OnPointerUp, ScrollWithButtons, SetPartTypes, SetTitleText, categoryValueChange
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPVesselDisplay.cs
- L=445 kind=UIBehaviour type=GAPVesselDisplay bases=ActionPaneDisplay anc=ActionPaneDisplay/Selectable ns=Expansions.Missions.Editor
- unity(69): GetComponent=13, Instantiate/Destroy=10, GameObject=9, SetActive=7, Renderer=4, Collider=4 | ksp: Part=9, Localizer=7, ShipConstruct=5, FlightGlobals=1 | fan_in=2
- methods(18): ChangeHoverPart, ChangeSelectedPart, Clean, Destroy, DestroyCurrentVessel, HideFairings, HideVessel, LoadVessel, OnDisplayClick, OnMouseOver, OnNextVessel, OnPartCategoryChanged, OnPrevVessel, Setup, SetupVessel, ShowFairings, ShowVessel, UpdateFrameFooter
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/HistoryType.cs
- L=8 kind=enum type=HistoryType bases=- anc=- ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=26
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/IMEHistoryTarget.cs
- L=7 kind=interface type=IMEHistoryTarget bases=- anc=- ns=Expansions.Missions.Editor
- unity(0): none | ksp: ConfigNode=1 | fan_in=7
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEActionPane.cs
- L=661 kind=MonoBehaviour type=MEActionPane bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(67): Transform=13, GameObject=13, UnityEngine.UI=12, Instantiate/Destroy=9, GetComponent=8, SetActive=7 | ksp: GameSettings=3, Localizer=1, GameEvents=1 | fan_in=3
- methods(33): AddParameterBlock, Awake, CheckDeletedNodeFromGap, Clean, ClearCache, CurrentLockdeNode, DisplayModuleFooter, DisplayModuleHeader, DisplayNodeModuleSection, DisplayNodeSettingsSection, DisplayParameter, GAPInitialize, GetInputFieldsList, GetNextInputTabStop, GetParameterFromFieldID, InitializeGapMinHeight, InputfieldStringCatcher, OnDestroy, OnGAPLockValueChange, OnParameterClick, OnSapScrollbarValueChange, RemoveParameterGroup, SAPDisplayConnectorParameters, SAPDisplayMultipleSelectedItemsMessage ...
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEBasicNode.cs
- L=207 kind=class type=MEBasicNode bases=IConfigNode anc=IConfigNode ns=Expansions.Missions.Editor
- unity(13): GameObject=2, Instantiate/Destroy=1 | ksp: Localizer=8, ConfigNode=8 | fan_in=7
- methods(4): BuildExtendedInfoListFromObject, Initialize, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MECrewAssignmentDialog.cs
- L=774 kind=MonoBehaviour type=MECrewAssignmentDialog bases=BaseCrewAssignmentDialog anc=BaseCrewAssignmentDialog/MonoBehaviour ns=Expansions.Missions.Editor
- unity(57): GetComponent=19, UnityEngine.UI=17, GameObject=9, SetActive=4, Instantiate/Destroy=4, Transform=2 | ksp: ProtoCrewMember=28, Vessel=6, Localizer=4, KerbalRoster=4 | fan_in=1
- methods(40): AddItem, AddSpacer, AddVesselTitle, ChangeKerbalEditorMode, CleanSelection, CreateAvailList, CreateCrewList, CreateTempKerbalList, Dismiss, DropOnAvailList, DropOnCrewList, DropOnTempList, GetCurrentCrewRoster, Hide, ListItemButtonClick, MoveCrewToAvail, MoveCrewToEmptySeat, OkCallback, OnButtonCancel, OnButtonOk, OnCrewEntrySelected, OnCrewMemberCreate, OnCrewMemberCreateDialogCancelled, OnCrewMemberDeleteConfirm ...
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Asteroid.cs
- L=10 kind=attribute type=MEGUI_Asteroid bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_AsteroidSelect.cs
- L=6 kind=attribute type=MEGUI_AsteroidSelect bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Button.cs
- L=12 kind=attribute type=MEGUI_Button bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Category.cs
- L=6 kind=attribute type=MEGUI_Category bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_CelestialBody.cs
- L=14 kind=attribute type=MEGUI_CelestialBody bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=6
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_CelestialBody_Biomes.cs
- L=6 kind=attribute type=MEGUI_CelestialBody_Biomes bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_CelestialBody_Orbit.cs
- L=7 kind=attribute type=MEGUI_CelestialBody_Orbit bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Checkbox.cs
- L=7 kind=attribute type=MEGUI_Checkbox bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=20
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_ColorPicker.cs
- L=6 kind=attribute type=MEGUI_ColorPicker bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Comet.cs
- L=10 kind=attribute type=MEGUI_Comet bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_CometSelect.cs
- L=6 kind=attribute type=MEGUI_CometSelect bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Control.cs
- L=109 kind=attribute type=MEGUI_Control bases=FieldAttribute anc=FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(4): none | ksp: Localizer=1 | fan_in=78
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_CrewList.cs
- L=6 kind=attribute type=MEGUI_CrewList bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Dropdown.cs
- L=19 kind=attribute type=MEGUI_Dropdown bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=63
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_DynamicModule.cs
- L=6 kind=attribute type=MEGUI_DynamicModule bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_DynamicModuleList.cs
- L=16 kind=attribute type=MEGUI_DynamicModuleList bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=5
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_FlagSelect.cs
- L=6 kind=attribute type=MEGUI_FlagSelect bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_GameParameters.cs
- L=6 kind=attribute type=MEGUI_GameParameters bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_InputField.cs
- L=15 kind=attribute type=MEGUI_InputField bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=41
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Label.cs
- L=13 kind=attribute type=MEGUI_Label bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_LaunchSiteSelect.cs
- L=6 kind=attribute type=MEGUI_LaunchSiteSelect bases=MEGUI_Dropdown anc=MEGUI_Dropdown/MEGUI_Control/FieldAttribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_LaunchSiteSituation.cs
- L=10 kind=attribute type=MEGUI_LaunchSiteSituation bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_ListOrder.cs
- L=12 kind=attribute type=MEGUI_ListOrder bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_MissionInstructor.cs
- L=6 kind=attribute type=MEGUI_MissionInstructor bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_MissionKerbal.cs
- L=11 kind=attribute type=MEGUI_MissionKerbal bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: ProtoCrewMember=1 | fan_in=11
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_NodeLabelNodeSelect.cs
- L=6 kind=attribute type=MEGUI_NodeLabelNodeSelect bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_NodeSelect.cs
- L=6 kind=attribute type=MEGUI_NodeSelect bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_NumberRange.cs
- L=19 kind=attribute type=MEGUI_NumberRange bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=31
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_ParameterSwitchCompound.cs
- L=12 kind=attribute type=MEGUI_ParameterSwitchCompound bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=14
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_ParameterSwitchCompound_KeyField.cs
- L=10 kind=attribute type=MEGUI_ParameterSwitchCompound_KeyField bases=MEGUI_Dropdown anc=MEGUI_Dropdown/MEGUI_Control/FieldAttribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_PartPicker.cs
- L=62 kind=attribute type=MEGUI_PartPicker bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(6): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Quaternion.cs
- L=10 kind=attribute type=MEGUI_Quaternion bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_ScoreRangeList.cs
- L=31 kind=attribute type=MEGUI_ScoreRangeList bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=5
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_ScrollViewSelector.cs
- L=6 kind=attribute type=MEGUI_ScrollViewSelector bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_SurfaceArea.cs
- L=6 kind=attribute type=MEGUI_SurfaceArea bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_SurfaceVolume.cs
- L=6 kind=attribute type=MEGUI_SurfaceVolume bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_TextArea.cs
- L=15 kind=attribute type=MEGUI_TextArea bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=6
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Time.cs
- L=6 kind=attribute type=MEGUI_Time bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=9
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_VesselGroundLocation.cs
- L=20 kind=attribute type=MEGUI_VesselGroundLocation bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=6
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_VesselLocation.cs
- L=6 kind=attribute type=MEGUI_VesselLocation bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_VesselPartSelect.cs
- L=6 kind=attribute type=MEGUI_VesselPartSelect bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=7
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_VesselSelect.cs
- L=9 kind=attribute type=MEGUI_VesselSelect bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=11
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_VesselSituation.cs
- L=10 kind=attribute type=MEGUI_VesselSituation bases=MEGUI_Control anc=MEGUI_Control/FieldAttribute/Attribute ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUICompoundParameter.cs
- L=212 kind=MonoBehaviour type=MEGUICompoundParameter bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(64): Transform=10, GameObject=6, SetActive=4, Lifecycle=2, Instantiate/Destroy=1, Input.=1 | ksp: none | fan_in=20
- methods(8): AddSubParameter, Display, GetSubParameter, OnDestroy, Setup, SortParameters, SortParametersByOrder, Update
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIConnector.cs
- L=409 kind=MonoBehaviour type=MEGUIConnector bases=MonoBehaviour,IMEHistoryTarget anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(46): Instantiate/Destroy=5, GameObject=3, Coroutine=3, UnityEngine.UI=2, SetActive=2, MonoBehaviour=1 | ksp: GameEvents=4, ConfigNode=3 | fan_in=4
- methods(23): Activate, AddNewEnd, CleanUp, ConnectLogic, DelayedUpdateLine, Deselect, Destroy, DisconnectLogic, FirstUpdate, GetConnectedNode, GetState, HideLine, IsConnectedToNode, IsNewConnectionValid, MakeCurveType1, MakeCurveType2, OnHistoryCreate, OnHistoryDestroy, ResetLineWidth, Select, SetUpConnector, UpdateLine, WasMouseClickWithinTolerance
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIConnectorDrag.cs
- L=88 kind=MonoBehaviour type=MEGUIConnectorDrag bases=MonoBehaviour,IPointerDownHandler,IEventSystemHandler anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(72): EventSystems=7, MonoBehaviour=1, GetComponent=1, GameObject=1 | ksp: none | fan_in=0
- methods(2): GetMeGUIParent, IsValidDrop
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIDropDownItem.cs
- L=18 kind=class type=MEGUIDropDownItem bases=- anc=- ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=26
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIDropDownItemListExtensions.cs
- L=37 kind=static class type=MEGUIDropDownItemListExtensions bases=- anc=- ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=0
- methods(3): GetDisplayStrings, GetKeys, GetValues
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIFooterAdditionalButton.cs
- L=100 kind=MonoBehaviour type=MEGUIFooterAdditionalButton bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(51): Transform=3, TMPro=2, Instantiate/Destroy=1 | ksp: none | fan_in=1
- methods(4): Create, OnAdditionalValueChanged, OnRemovevalueChanged, Setup
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIListOrderItem.cs
- L=15 kind=MonoBehaviour type=MEGUIListOrderItem bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(74): UnityEngine.UI=2, TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(0): 
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUINodeBodyParameter.cs
- L=26 kind=MonoBehaviour type=MEGUINodeBodyParameter bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(87): Transform=4, UnityEngine.UI=2, TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(1): Setup
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUINodeIcon.cs
- L=387 kind=MonoBehaviour type=MEGUINodeIcon bases=MonoBehaviour,IBeginDragHandler,IEventSystemHandler anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(69): EventSystems=13, UnityEngine.UI=10, Lifecycle=2, Transform=1, TMPro=1, SetActive=1 | ksp: none | fan_in=12
- methods(15): CompareTo, OnBeginDrag, OnClickCanvas, OnDrag, OnEndDrag, OnPointerClick, OnPointerEnter, OnPointerExit, ResetCanvasSetting, SetHighlighterColor, SetUp, Start, ToggleHighlighter, Update, onToggle
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIPanel.cs
- L=341 kind=MonoBehaviour type=MEGUIPanel bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(73): UnityEngine.UI=13, Screen/Cursor=12, SetActive=6, GameObject=6, Coroutine=6, GetComponent=5 | ksp: none | fan_in=3
- methods(13): Awake, FixedUpdate, Hide, Maximize, OnMaximizeValueChange, OnVisibilityValueChange, RebuildPanelLayout, Reset, Restore, Show, Start, VisibilityHideAnimation, VisibilityShowAnimation
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIPanelSeparator.cs
- L=86 kind=MonoBehaviour type=MEGUIPanelSeparator bases=MonoBehaviour,IDragHandler,IEventSystemHandler anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(81): EventSystems=10, Screen/Cursor=6, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(6): OnBeginDrag, OnDrag, OnEndDrag, OnPointerEnter, OnPointerExit, Start
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameter.cs
- L=569 kind=MonoBehaviour type=MEGUIParameter bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(72): UnityEngine.UI=18, EventSystems=12, GameObject=11, GetComponent=10, Transform=9, SetActive=6 | ksp: ConfigNode=4 | fan_in=37
- methods(34): Awake, BlockMaskedControls, ChangeColor, Create, Display, DisplayGAP, DisplayParameterObject, GetGroupName, GetState, HideColor, HideGAPButton, LockLocalizedText, MaskScrollControls, OnEnable, OnHistoryPinned, OnParameterReset, OnPinValueChanged, OnPointerEnter, OnPointerExit, RefreshUI, RemoveParameterByGroup, ResetDefaultValue, Select, SetPinToggleImage ...
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterAsteroid.cs
- L=94 kind=MonoBehaviour type=MEGUIParameterAsteroid bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(51): TMPro=2, UnityEngine.UI=1, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=0
- methods(6): Button_RegenerateSeed, DisplayGAP, InputText_NewSeed, RefreshGapPrefab, Setup, Start
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterAsteroidDropdownList.cs
- L=141 kind=MonoBehaviour type=MEGUIParameterAsteroidDropdownList bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(27): TMPro=1, Lifecycle=1 | ksp: GameEvents=4, ConfigNode=1 | fan_in=3
- methods(9): Display, OnDestroy, OnHistoryValueChange, OnParameterValueChanged, RefreshUI, SetDropdownValues, Setup, onBuilderNodeAdded, onBuilderNodeDeleted
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterAwardModule.cs
- L=101 kind=MonoBehaviour type=MEGUIParameterAwardModule bases=MEGUIParameterDynamicModule anc=MEGUIParameterDynamicModule/MEGUICompoundParameter/MEGUIParameter ns=Expansions.Missions.Editor
- unity(72): Transform=6, SetActive=3, UnityEngine.UI=2, TMPro=2, GameObject=2 | ksp: none | fan_in=1
- methods(5): OnAwardedPointsEndEdit, OnAwardedPointsValueChange, OnModuleEnabledToggleValueChange, Setup, SetupAwardModule
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterButton.cs
- L=60 kind=MonoBehaviour type=MEGUIParameterButton bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(44): UnityEngine.UI=1, TMPro=1 | ksp: none | fan_in=0
- methods(2): OnButtonClick, Setup
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCelestialBody.cs
- L=73 kind=MonoBehaviour type=MEGUIParameterCelestialBody bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(15): none | ksp: none | fan_in=4
- methods(5): DisplayGAP, OnLeftGapClick, OnParameterValueChanged, OnRightGapClick, Setup
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCelestialBody_Area.cs
- L=159 kind=MonoBehaviour type=MEGUIParameterCelestialBody_Area bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(15): none | ksp: CelestialBody=1 | fan_in=0
- methods(12): DisplayGAP, OnDragRadius, OnDropDownBody, OnGAPPointTranslate, OnInputLatitude, OnInputLongitude, OnInputRadius, OnLeftGapClick, OnRightGapClick, Setup, UpdateGizmo, UpdateMaxRadiusValues
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCelestialBody_Biomes.cs
- L=99 kind=MonoBehaviour type=MEGUIParameterCelestialBody_Biomes bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(15): none | ksp: FlightGlobals=1, CelestialBody=1 | fan_in=2
- methods(8): DisplayGAP, OnDropDownBiome, OnDropDownBody, OnGAPBiomeSelected, OnGapLeftArrow, OnGapRightArrow, Setup, UpdateValue
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCelestialBody_VesselGroundLocation.cs
- L=222 kind=MonoBehaviour type=MEGUIParameterCelestialBody_VesselGroundLocation bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(51): SetActive=6, GameObject=6, Transform=3 | ksp: none | fan_in=3
- methods(12): DisplayGAP, OnDropDownBody, OnGAPPointTranslate, OnGapLeftArrow, OnGapPointRotate, OnGapRightArrow, OnGizmoIconChange, OnInputLatitude, OnInputLongitude, OnInputRot, Setup, UpdateGizmo
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCelestialBody_Volume.cs
- L=339 kind=MonoBehaviour type=MEGUIParameterCelestialBody_Volume bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(51): SetActive=12, GameObject=12 | ksp: CelestialBody=1 | fan_in=0
- methods(23): AttachCallbacks, DisplayGAP, MinValuesUpdate, OnDragRadius, OnDropDownBody, OnDropDownShape, OnGAPPointTranslate, OnGAPShapeChange, OnGAPSliderConeBounds, OnGAPSliderSphere, OnInputHeightConeMax, OnInputHeightConeMin, OnInputHeightSphere, OnInputLatitude, OnInputLongitude, OnInputRadius, OnLeftGapClick, OnRightGapClick, RefreshMinHeightValues, Setup, UpdateGizmo, UpdateGizmoShape, UpdateMaxValues
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCelestialBodyOrbit.cs
- L=244 kind=MonoBehaviour type=MEGUIParameterCelestialBodyOrbit bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(27): SetActive=2, GameObject=2 | ksp: Orbit=12, Vector3d=6, ConfigNode=6, CelestialBody=2, QuaternionD=1 | fan_in=1
- methods(16): DisplayGAP, GetState, OnDropDownBody, OnGapLeftArrow, OnGapRightArrow, OnGlobalGizmoUpdate, OnHistoryValueChange, OnOrbitReset, OnPointGizmoUpdate, OnSubParameterResetValue, OnSubParameterValueChange, OnUpdateSliderValues, OnUpdateValues, RefreshUI, Setup, UpdateSemiMajorAxisLimits
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCheckbox.cs
- L=80 kind=MonoBehaviour type=MEGUIParameterCheckbox bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(28): UnityEngine.UI=1 | ksp: ConfigNode=1 | fan_in=9
- methods(5): OnHistoryValueChange, OnParameterValueChanged, RefreshUI, ResetDefaultValue, Setup
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterColorPicker.cs
- L=109 kind=MonoBehaviour type=MEGUIParameterColorPicker bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(78): UnityEngine.UI=9, GetComponent=3, Transform=2, GameObject=2, Instantiate/Destroy=1 | ksp: ConfigNode=4 | fan_in=0
- methods(7): GetState, OnColorChange, OnHistoryValueChange, OnPickColorButton, RefreshUI, ResetDefaultValue, Setup
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterComet.cs
- L=94 kind=MonoBehaviour type=MEGUIParameterComet bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(51): TMPro=2, UnityEngine.UI=1, Lifecycle=1, GetComponent=1 | ksp: Localizer=1 | fan_in=0
- methods(6): Button_RegenerateSeed, DisplayGAP, InputText_NewSeed, RefreshGapPrefab, Setup, Start
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCometDropdownList.cs
- L=141 kind=MonoBehaviour type=MEGUIParameterCometDropdownList bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(27): TMPro=1, Lifecycle=1 | ksp: GameEvents=4, ConfigNode=1 | fan_in=3
- methods(9): Display, OnDestroy, OnHistoryValueChange, OnParameterValueChanged, RefreshUI, SetDropdownValues, Setup, onBuilderNodeAdded, onBuilderNodeDeleted
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCrewList.cs
- L=73 kind=MonoBehaviour type=MEGUIParameterCrewList bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(29): TMPro=1 | ksp: ProtoCrewMember=4, KerbalRoster=1, ConfigNode=1 | fan_in=0
- methods(5): OnHistoryValueChange, OnParameterValueChanged, RefreshUI, ResetDefaultValue, Setup
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterDropdownList.cs
- L=300 kind=MonoBehaviour type=MEGUIParameterDropdownList bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(33): GameObject=3, UnityEngine.UI=1, Transform=1, TMPro=1, Lifecycle=1 | ksp: ConfigNode=4 | fan_in=28
- methods(14): BuildDropDownItemsList, DisplayGAP, GetItemIndex, GetState, OnEnable, OnHistoryValueChange, OnParameterValueChanged, RebuildDropDown, RefreshGapPrefab, RefreshUI, ResetDefaultValue, SetExcludedOptions, Setup, ValueComparer
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterDynamicModule.cs
- L=78 kind=MonoBehaviour type=MEGUIParameterDynamicModule bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(75): Transform=6, SetActive=3, UnityEngine.UI=2, GameObject=2 | ksp: none | fan_in=2
- methods(4): Create, OnRemoveButton, Setup, ToggleExpand
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterDynamicModuleList.cs
- L=313 kind=MonoBehaviour type=MEGUIParameterDynamicModuleList bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(65): GameObject=11, SetActive=9, Transform=4, UnityEngine.UI=3, Coroutine=3, Texture=2 | ksp: ConfigNode=11 | fan_in=6
- methods(15): AddModule, AddModules, CollapseGroup, FieldValueContainsModule, GetState, OnAddValueChange, OnButtonCollapsePressed, OnEnable, OnHistoryAddModule, OnHistoryRemoveModule, RefreshUI, RemoveModule, RemoveModules, Setup, WaitForDropdownList
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterFlagDropdownList.cs
- L=141 kind=MonoBehaviour type=MEGUIParameterFlagDropdownList bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(27): TMPro=1, Lifecycle=1 | ksp: GameEvents=4, ConfigNode=1 | fan_in=1
- methods(9): Display, OnDestroy, OnHistoryValueChange, OnParameterValueChanged, RefreshUI, SetDropdownValues, Setup, onBuilderNodeAdded, onBuilderNodeDeleted
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterGameParameters.cs
- L=274 kind=MonoBehaviour type=MEGUIParameterGameParameters bases=MEGUIParameter,IMEHistoryTarget anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(22): UnityEngine.UI=1, Instantiate/Destroy=1 | ksp: ConfigNode=8, GameEvents=2, Game=1 | fan_in=0
- methods(10): CopyDeleteStockCraft, GetState, OnDifficultySettingsOpen, OnGameParametersChanged, OnHistoryValueChanged, OpenDifficultySettings, ResetDefaultValue, Setup, craftFileInUse, getStockFiles
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterGroup.cs
- L=156 kind=MonoBehaviour type=MEGUIParameterGroup bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(78): UnityEngine.UI=7, SetActive=7, GameObject=7, Transform=3, Texture=2, GetComponent=2 | ksp: none | fan_in=5
- methods(8): CollapseGroup, Create, Display, OnButtonCollapsePressed, OnParameterGroupReset, Setup, SortChilden, SortParametersByOrder
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterGroupHeader.cs
- L=45 kind=MonoBehaviour type=MEGUIParameterGroupHeader bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(87): EventSystems=4, UnityEngine.UI=3, Lifecycle=2, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=1
- methods(4): Awake, OnPointerEnter, OnPointerExit, Start
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterInputField.cs
- L=268 kind=MonoBehaviour type=MEGUIParameterInputField bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(40): TMPro=5, UnityEngine.UI=1, Transform=1, GameObject=1 | ksp: Localizer=9, ConfigNode=1 | fan_in=4
- methods(11): Convert, LockLocalizedText, OnHistoryValueChange, OnParameterEndEdit, OnParameterValueChanged, OnSelectInput, OnValidatePercentageInput, Parse, RefreshUI, ResetDefaultValue, Setup
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterLabel.cs
- L=61 kind=MonoBehaviour type=MEGUIParameterLabel bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(46): TMPro=1, SetActive=1, GameObject=1 | ksp: none | fan_in=2
- methods(2): RefreshUI, Setup
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterLaunchSiteDropdownList.cs
- L=79 kind=MonoBehaviour type=MEGUIParameterLaunchSiteDropdownList bases=MEGUIParameterDropdownList anc=MEGUIParameterDropdownList/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(24): Lifecycle=1 | ksp: GameEvents=4 | fan_in=1
- methods(5): BuildDropDownItemsList, OnDestroy, Setup, onBuilderNodeAdded, onBuilderNodeDeleted
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterLaunchSiteSituation.cs
- L=114 kind=MonoBehaviour type=MEGUIParameterLaunchSiteSituation bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(79): UnityEngine.UI=6, Transform=5, SetActive=3, GameObject=3, GetComponent=2, TMPro=1 | ksp: ConfigNode=4 | fan_in=0
- methods(6): Display, GetState, InputTextAction_SetLaunchSiteName, Setup, onHistorysetLaunchSiteName, toggleExpand
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterListOrder.cs
- L=166 kind=MonoBehaviour type=MEGUIParameterListOrder bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(70): Transform=7, SetActive=6, GameObject=4, UnityEngine.UI=2, Lifecycle=2, GetComponent=2 | ksp: none | fan_in=0
- methods(9): CleanList, DownButton, FillList, GetTitle, OnDisable, OnEnable, Setup, ToggleExpand, UpButton
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterMissionInstructor.cs
- L=132 kind=MonoBehaviour type=MEGUIParameterMissionInstructor bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(61): GameObject=11, SetActive=2, GetComponent=1 | ksp: Localizer=4 | fan_in=1
- methods(6): DisplayGAP, OnParameterValueChanged, RefreshGapPrefab, SetGAPKerbal, Setup, onVintageSuitChanged
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterMissionKerbal.cs
- L=105 kind=MonoBehaviour type=MEGUIParameterMissionKerbal bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(34): GameObject=3 | ksp: ProtoCrewMember=6 | fan_in=2
- methods(6): Display, DisplayGAP, OnParameterValueChanged, RefreshGapPrefab, SetGAPKerbal, Setup
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterNodeDropdownList.cs
- L=148 kind=MonoBehaviour type=MEGUIParameterNodeDropdownList bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(39): TMPro=4 | ksp: ConfigNode=1 | fan_in=2
- methods(8): Display, GetNodeList, OnHistoryValueChange, OnParameterValueChanged, RefreshNodeList, RefreshUI, ResetDefaultValue, Setup
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterNodeLabelNodeDropdownList.cs
- L=28 kind=MonoBehaviour type=MEGUIParameterNodeLabelNodeDropdownList bases=MEGUIParameterNodeDropdownList anc=MEGUIParameterNodeDropdownList/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(15): none | ksp: none | fan_in=1
- methods(2): GetNodeList, Setup
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterNumberRange.cs
- L=432 kind=MonoBehaviour type=MEGUIParameterNumberRange bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(66): GameObject=10, SetActive=8, UnityEngine.UI=7, TMPro=4, Input.=3, EventSystems=3 | ksp: Localizer=1, ConfigNode=1 | fan_in=10
- methods(17): EnableSlider, OnButtonClicked, OnHistoryValueChange, OnInputFieldLoseFocus, OnInputHoverEnd, OnInputHoverStart, OnSAPScroll, OnSliderMouserUp, OnSliderValueChange, RefreshUI, ResetDefaultValue, SetUnitsText, Setup, Toggle, Update, UpdateSlider, UpdateValueText
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterPartPicker.cs
- L=146 kind=MonoBehaviour type=MEGUIParameterPartPicker bases=MEGUIParameter,IMEHistoryTarget anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(30): UnityEngine.UI=1, TMPro=1 | ksp: ConfigNode=4, Localizer=3, AvailablePart=1 | fan_in=5
- methods(9): DisplayGAP, GetState, OnHistoryValueChange, OnPartPickerOpen, OnPartSelectionChange, RefreshUI, ResetDefaultValue, Setup, UpdateDisplayedParts
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterQuaternion.cs
- L=82 kind=MonoBehaviour type=MEGUIParameterQuaternion bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(47): UnityEngine.UI=3 | ksp: none | fan_in=1
- methods(5): OnValueChange, OnValueChanged, SetQuaternion, Setup, ToggleEulerAxis
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParametersController.cs
- L=90 kind=MonoBehaviour type=MEGUIParametersController bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(59): Lifecycle=3, GameObject=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: none | fan_in=5
- methods(5): Awake, GetControl, OnDestroy, SetupParameterControls, Start
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterScoreRangeList.cs
- L=89 kind=MonoBehaviour type=MEGUIParameterScoreRangeList bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(62): Transform=4, TMPro=3, UnityEngine.UI=1 | ksp: none | fan_in=0
- methods(4): AddScoreRange, OnAddButton, OnRemoveScoreRange, Setup
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterScrollViewSelector.cs
- L=137 kind=MonoBehaviour type=MEGUIParameterScrollViewSelector bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(79): GameObject=11, UnityEngine.UI=4, Transform=4, SetActive=4, GetComponent=2, TMPro=1 | ksp: none | fan_in=0
- methods(9): ButtonCallback, ExpandEvent, FillList, IsExpanded, OnExpandView, OnSelectedElement, SelectedElementEvent, Setup, ToggleExpand
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterSwitchCompound.cs
- L=120 kind=MonoBehaviour type=MEGUIParameterSwitchCompound bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(51): SetActive=3, GameObject=3, Transform=2 | ksp: none | fan_in=1
- methods(3): Display, OnControlToDisplaySet, Setup
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterTextArea.cs
- L=137 kind=MonoBehaviour type=MEGUIParameterTextArea bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(43): TMPro=2, UnityEngine.UI=1, Transform=1, GameObject=1 | ksp: Localizer=9, GameEvents=1, ConfigNode=1 | fan_in=4
- methods(9): LockLocalizedText, OnHistoryValueChange, OnMouseOver, OnParameterEndEdit, OnParameterValueChanged, RefreshUI, ResetDefaultValue, Setup, onLocalizationLockOverriden
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterTime.cs
- L=54 kind=MonoBehaviour type=MEGUIParameterTime bases=MEGUIParameter anc=MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(49): UnityEngine.UI=1, Transform=1, GameObject=1 | ksp: ConfigNode=1 | fan_in=1
- methods(3): OnHistoryValueChange, OnValueChange, Setup
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterVessel.cs
- L=72 kind=MonoBehaviour type=MEGUIParameterVessel bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(25): GameObject=1 | ksp: Part=1 | fan_in=3
- methods(5): OnChangePartSelection, OnNextVessel, OnPrevVessel, SetMission, Setup
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterVesselDropdownList.cs
- L=248 kind=MonoBehaviour type=MEGUIParameterVesselDropdownList bases=MEGUIParameterVessel anc=MEGUIParameterVessel/MEGUICompoundParameter/MEGUIParameter ns=Expansions.Missions.Editor
- unity(22): TMPro=1, Lifecycle=1 | ksp: GameEvents=8, Localizer=5, ConfigNode=1 | fan_in=7
- methods(15): Display, DisplayGAP, OnDestroy, OnHistoryValueChange, OnNextVessel, OnParameterValueChanged, OnPrevVessel, OverrideDefaultValue, RefreshUI, ResetDefaultValue, SetCraftOnly, SetDropdownValues, Setup, onBuilderNodeAdded, onBuilderNodeDeleted
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterVesselLocation.cs
- L=305 kind=MonoBehaviour type=MEGUIParameterVesselLocation bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(75): SetActive=25, GameObject=25, UnityEngine.UI=4, Transform=1, Lifecycle=1 | ksp: ConfigNode=4, FlightGlobals=1, CelestialBody=1 | fan_in=1
- methods(10): Create, Display, DropDownAction_Facility, DropDownAction_LaunchSite, DropDownAction_Situation, GetState, OnEnable, OnHistoryUpdateDropdowns, Setup, setSituationType
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterVesselPartSelector.cs
- L=238 kind=MonoBehaviour type=MEGUIParameterVesselPartSelector bases=MEGUIParameterVessel anc=MEGUIParameterVessel/MEGUICompoundParameter/MEGUIParameter ns=Expansions.Missions.Editor
- unity(21): Lifecycle=2 | ksp: Localizer=6, GameEvents=4, ConfigNode=4, Part=3, ShipConstruct=1 | fan_in=3
- methods(17): Awake, DisplayGAP, GetSelectedPart, GetState, LoadSelectedPart, OnChangePartSelection, OnDestroy, OnHistoryValueChange, OnNextVessel, OnPrevVessel, OnVesselValueChange, RefreshUI, ResetDefaultValue, SetFieldValues, Setup, UpdateSelectedPartInDisplay, onVesselSituationChanged
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterVesselSituation.cs
- L=550 kind=MonoBehaviour type=MEGUIParameterVesselSituation bases=MEGUICompoundParameter anc=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour ns=Expansions.Missions.Editor
- unity(71): GameObject=23, SetActive=22, UnityEngine.UI=14, Transform=8, TMPro=6, GetComponent=2 | ksp: Localizer=13, ConfigNode=9, GameEvents=6, FlightGlobals=4, MissionSystem=3 | fan_in=1
- methods(21): ButtonAction_CreateVessel, ButtonAction_ResetVessel, ButtonAction_SetCrew, CreateVessel_SaveCompleted, DelegateCallback, Display, DropDownAction_Vessels, GetState, InputTextAction_SetVesselName, OnDestroy, OnHistorySetCrew, Setup, ToggleAction_AutoPopulateCrew, ToggleAction_PlayerBuilt, onCreateVesselMissionSetupSuccess, onHistoryDropDownVessels, onHistoryResetVessel, onHistoryTogglePlayerBuilt, onHistorysetVesselName, setupVessels, toggleExpand
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIScoreRangeItem.cs
- L=149 kind=MonoBehaviour type=MEGUIScoreRangeItem bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(77): TMPro=15, Transform=2, Instantiate/Destroy=2, UnityEngine.UI=1, MonoBehaviour=1, GameObject=1 | ksp: none | fan_in=2
- methods(11): Create, OnInputValueChange, OnMaxRangeInputEndEdit, OnMinRangeInputEndEdit, OnRemoveButton, OnScoreInputEndEdit, OnSelectMaxRangeInput, OnSelectMinRangeInput, OnValidatePercentageInput, Setup, Terminate
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIScoreRangeTimeItem.cs
- L=34 kind=MonoBehaviour type=MEGUIScoreRangeTimeItem bases=MEGUIScoreRangeItem anc=MEGUIScoreRangeItem/MonoBehaviour ns=Expansions.Missions.Editor
- unity(15): none | ksp: none | fan_in=1
- methods(3): OnMaxRangeTimeValueChange, OnMinRangeTimeValueChange, Setup
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUITimeControl.cs
- L=203 kind=MonoBehaviour type=MEGUITimeControl bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(70): TMPro=7, GameObject=6, SetActive=5, UnityEngine.UI=2, Lifecycle=2, MonoBehaviour=1 | ksp: none | fan_in=3
- methods(7): Awake, EditTime, Start, UpdateButtonText, UpdateTextValues, UpdateTimeFromInput, UpdateValues
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MENodeCanvas.cs
- L=543 kind=MonoBehaviour type=MENodeCanvas bases=MonoBehaviour,IScrollHandler,IEventSystemHandler anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(72): Transform=33, Screen/Cursor=18, EventSystems=13, UnityEngine.UI=4, Camera=4, Lifecycle=3 | ksp: ConfigNode=4, GameSettings=2, GameEvents=2 | fan_in=5
- methods(35): AdjustOrthographicsSizeAndCameraPosition, Awake, CalculateBorders, CheckSnap, CheckZoomBoundries, FitCameraToArea, FitCameraToSelectedNodes, FocusNode, FocusStartNode, GetCanvasDragPosition, GetClampedPosition, GetMousePointOnGrid, GetNodeBounds, Initialize, Load, OnDestroy, OnDrag, OnMissionLoaded, OnPointerClick, OnPointerDown, OnPointerUp, OnScroll, PointerInsideCanvasView, Save ...
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MENodeCanvasBackground.cs
- L=31 kind=MonoBehaviour type=MENodeCanvasBackground bases=MonoBehaviour,IDragHandler,IEventSystemHandler anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(93): EventSystems=8, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(4): OnDrag, OnPointerClick, OnPointerDown, OnPointerUp
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MENodeCategorizer.cs
- L=672 kind=MonoBehaviour type=MENodeCategorizer bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(69): Transform=34, GameObject=21, SetActive=15, Instantiate/Destroy=5, GetComponent=4, Coroutine=3 | ksp: Localizer=4, ConfigNode=4, InputLockManager=3, GameDatabase=2 | fan_in=6
- methods(31): AddCanvasNodeButton, Awake, ClearNodesInCategory, ClearSearchField, ContainsCategoryConfig, CreateSearchToolboxGroups, DeselectLastCategoryButton, DisplayNodesInCategory, DisplayStoredIcon, EnableDrag, FocusSearchField, GetCategoryImage, GetDisplayedNodeNames, GetNodes, HighLightDisplayedNodeIcon, HighLightSelectedSearchResults, HighlightNodes, HightLightCategoryButton, HightLightNodeIcon, NodeMatchesSearch, RefreshCurrentDisplay, RemoveCanvasNodeButton, RemoveSearchToolboxGroups, ResetStoredIcons ...
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MENodeCategoryButton.cs
- L=101 kind=MonoBehaviour type=MENodeCategoryButton bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(66): UnityEngine.UI=4, Texture=2, SetActive=1, MonoBehaviour=1, GameObject=1 | ksp: none | fan_in=1
- methods(6): DeselectCategory, OnMouseClick, SelectCategory, SetIcon, Setup, ToggleHighlighter
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MENodeCategoryColor.cs
- L=13 kind=class type=MENodeCategoryColor bases=- anc=- ns=Expansions.Missions.Editor
- unity(17): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MENodeColors.cs
- L=16 kind=class type=MENodeColors bases=- anc=- ns=Expansions.Missions.Editor
- unity(17): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MENodeConnectionType.cs
- L=8 kind=enum type=MENodeConnectionType bases=- anc=- ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEPartCategorizer.cs
- L=228 kind=MonoBehaviour type=MEPartCategorizer bases=BasePartCategorizer anc=BasePartCategorizer/MonoBehaviour ns=Expansions.Missions.Editor
- unity(79): GameObject=7, SetActive=6, GetComponent=6, EventSystems=5, KeyCode=4, Input.=4 | ksp: AvailablePart=21, PartLoader=1 | fan_in=1
- methods(14): Awake, InstantiatePartCategorizerButton, OnClick, OnDestroy, OnFalse, OnPointerDown, OnPointerUp, OnTrue, PartSelected, Reset, ScrollWithButtons, SearchFilterResult, SelectedPartsOfCategory, Setup
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEPartCategorizerButton.cs
- L=10 kind=MonoBehaviour type=MEPartCategorizerButton bases=PartCategorizerButton anc=PartCategorizerButton/MonoBehaviour ns=Expansions.Missions.Editor
- unity(44): UnityEngine.UI=1 | ksp: none | fan_in=1
- methods(0): 
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEPartSelectorBrowser.cs
- L=322 kind=MonoBehaviour type=MEPartSelectorBrowser bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(67): UnityEngine.UI=8, GameObject=7, Transform=6, SetActive=5, Instantiate/Destroy=3, Lifecycle=2 | ksp: AvailablePart=25, PartLoader=1, Localizer=1 | fan_in=4
- methods(23): AddPartIcon, Awake, BuildPartList, CancelledCallback, ClearPartList, DisablePart, Dismiss, Hide, OnDestroy, OnFilterSelectionChange, OnPartListUpdate, OnPartSelectionChange, OnSearchChange, PartSelectionCallback, SelectionCallback, SetPartSelectionStatus, Setup, Show, SortingCallback, Spawn, Start, onButtonCancel, onButtonSelect
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEPartSelectorEntry.cs
- L=46 kind=MonoBehaviour type=MEPartSelectorEntry bases=EditorPartIcon anc=EditorPartIcon/MonoBehaviour ns=Expansions.Missions.Editor
- unity(72): UnityEngine.UI=3, GetComponent=2, GameObject=1 | ksp: AvailablePart=2 | fan_in=1
- methods(3): Create, OnPartSelectorClick, UpdateSelection
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEValidityHighlighColor.cs
- L=13 kind=class type=MEValidityHighlighColor bases=- anc=- ns=Expansions.Missions.Editor
- unity(17): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionBiome.cs
- L=94 kind=class type=MissionBiome bases=- anc=- ns=Expansions.Missions.Editor
- unity(0): none | ksp: FlightGlobals=5, Localizer=3, ConfigNode=2, CelestialBody=2 | fan_in=11
- methods(7): Equals, GetHashCode, GetNodeBodyParameterString, Load, Save, SetBiomes, SetCelestialBodies
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionEditorHistory.cs
- L=269 kind=MonoBehaviour type=MissionEditorHistory bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(44): MonoBehaviour=5, Instantiate/Destroy=3, Time.=2, GameObject=2, Lifecycle=1 | ksp: ConfigNode=2 | fan_in=27
- methods(11): ActionCallback, Awake, Clear, ClearRedoActions, Peek, Pop, Push, PushUndoAction, Redo, SelectAffectedObject, Undo
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionEditorMapView.cs
- L=27 kind=MonoBehaviour type=MissionEditorMapView bases=MapView anc=MapView/MonoBehaviour ns=Expansions.Missions.Editor
- unity(55): Lifecycle=1, Camera=1 | ksp: MapView=1 | fan_in=3
- methods(2): Awake, CreateMapFXCamera
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionEditorValidator.cs
- L=265 kind=MonoBehaviour type=MissionEditorValidator bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(31): Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: Localizer=1, GameSettings=1 | fan_in=32
- methods(13): AddNodeFail, AddNodePass, AddNodeResult, AddNodeWarn, AddResults, Awake, GetStatusColor, GetValidationColor, NextMode, ResetValidator, RunValidation, RunValidationOnParamChange, Start
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionsBrowserDialog.cs
- L=541 kind=MonoBehaviour type=MissionsBrowserDialog bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(67): UnityEngine.UI=10, GameObject=10, Transform=8, Texture=5, Coroutine=5, SetActive=4 | ksp: Localizer=20, GameSettings=3, GameEvents=2, MissionSystem=1 | fan_in=1
- methods(25): BuildMissionList, BuildMissionListRoutine, BuildPacksList, CancelledCallback, ClearListItems, ClearSelection, ConfirmLoadGame, CreateWidget, Dismiss, DisplayAddedMissions, Hide, OnDeleteConfirm, OnSelectionChanged, OnTabStock, OnTabUser, PromptDeleteFileConfirm, SelectFileCallback, Show, Spawn, Start, Update, onButtonCancel, onButtonDelete, onButtonLoad ...
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionValidationDialog.cs
- L=172 kind=MonoBehaviour type=MissionValidationDialog bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(74): UnityEngine.UI=4, Transform=3, Lifecycle=3, Instantiate/Destroy=3, GameObject=3, TMPro=2 | ksp: GameSettings=4 | fan_in=1
- methods(11): BuildDropDown, ClearValidationResults, DestroyDialog, Display, OnButtonValidate, OnCancel, OnDestroy, OnOK, PrintValidationResults, Start, Update
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionValidationEntry.cs
- L=63 kind=MonoBehaviour type=MissionValidationEntry bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(81): Transform=3, TMPro=3, UnityEngine.UI=2, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: Localizer=1 | fan_in=1
- methods(4): Awake, Create, OnToggleValueChange, Start
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionValidationTestResult.cs
- L=17 kind=class type=MissionValidationTestResult bases=- anc=- ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=5
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/NodeListTooltip.cs
- L=188 kind=MonoBehaviour type=NodeListTooltip bases=Tooltip anc=Tooltip/MonoBehaviour ns=Expansions.Missions.Editor
- unity(84): GameObject=11, SetActive=10, UnityEngine.UI=8, Transform=5, TMPro=4, Instantiate/Destroy=4 | ksp: none | fan_in=2
- methods(8): CreateExtInfoElement, CreateExtendedInfo, DisplayExtendedInfo, GetNewTooltipSpacer, GetNewTooltipWidget, HidePreviousTooltipWidgets, OnDestroy, Setup
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/NodeListTooltipController.cs
- L=83 kind=MonoBehaviour type=NodeListTooltipController bases=PinnableTooltipController anc=PinnableTooltipController/TooltipController/MonoBehaviour ns=Expansions.Missions.Editor
- unity(67): UnityEngine.UI=4, EventSystems=2, Transform=1, GetComponent=1 | ksp: Localizer=3 | fan_in=2
- methods(5): CreateTooltip, GetTooltipHintText, OnPointerClick, OnTooltipDespawned, OnTooltipSpawned
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/NodeListTooltipMasterController.cs
- L=36 kind=MonoBehaviour type=NodeListTooltipMasterController bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(68): Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=1
- methods(2): Awake, OnDestroy
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/OrbitGizmo.cs
- L=216 kind=MonoBehaviour type=OrbitGizmo bases=ManeuverGizmoBase anc=ManeuverGizmoBase/MonoBehaviour ns=Expansions.Missions.Editor
- unity(50): SetActive=4, Transform=3, GameObject=2, Lifecycle=1, Instantiate/Destroy=1, Input.=1 | ksp: Vector3d=17 | fan_in=3
- methods(22): ChangeToGlobalMode, ChangeToPointMode, Create, HandlesUpdatedCallback, OnAntiSMAUpdate, OnAntinormalUpdate, OnEccInUpdate, OnEccOutUpdate, OnGlobalModeButtonPress, OnIncClockwiseUpdate, OnIncCounterUpdate, OnMouseDrag, OnMouseUp, OnNormalUpdate, OnPointModeButtonPress, OnProgradeUpdate, OnRadialInUpdate, OnRadialOutUpdate, OnResetButtonPress, OnRetrogradeUpdate, OnSMAUpdate, Start
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/OrbitGizmoMode.cs
- L=9 kind=enum type=OrbitGizmoMode bases=- anc=- ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/SpriteButton.cs
- L=81 kind=MonoBehaviour type=SpriteButton bases=MouseRayEventsHandler anc=MouseRayEventsHandler/MouseEventsHandlerBase/MonoBehaviour ns=Expansions.Missions.Editor
- unity(43): Texture=1, Renderer=1, Lifecycle=1 | ksp: none | fan_in=1
- methods(5): Awake, OnMouseDown, OnMouseEnter, OnMouseExit, OnMouseUp
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/SurfaceLocation.cs
- L=11 kind=struct type=SurfaceLocation bases=- anc=- ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/UICollapsibleSection.cs
- L=31 kind=MonoBehaviour type=UICollapsibleSection bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Editor
- unity(99): UnityEngine.UI=6, SetActive=3, GameObject=3, Texture=2, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(2): OnButtonCollapsePressed, Start
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/ValidationColors.cs
- L=17 kind=class type=ValidationColors bases=- anc=- ns=Expansions.Missions.Editor
- unity(21): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/ValidationStatus.cs
- L=16 kind=enum type=ValidationStatus bases=- anc=- ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/ValidatorMode.cs
- L=16 kind=enum type=ValidatorMode bases=- anc=- ns=Expansions.Missions.Editor
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/IMEFlowBlock.cs
- L=15 kind=interface type=IMEFlowBlock bases=- anc=- ns=Expansions.Missions.Flow
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowBlock.cs
- L=166 kind=class type=MEFlowBlock bases=IMEFlowBlock,IConfigNode anc=IMEFlowBlock ns=Expansions.Missions.Flow
- unity(5): none | ksp: ConfigNode=5 | fan_in=4
- methods(4): AddBlock, Load, Save, UpdateMissionFlowUI
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowConvergence.cs
- L=15 kind=class type=MEFlowConvergence bases=- anc=- ns=Expansions.Missions.Flow
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowExtensions.cs
- L=38 kind=static class type=MEFlowExtensions bases=- anc=- ns=Expansions.Missions.Flow
- unity(0): none | ksp: none | fan_in=0
- methods(3): CountStartNodes, DistinctStartNodes, GetPathsStarting
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowOrBlock.cs
- L=20 kind=class type=MEFlowOrBlock bases=MEFlowBlock anc=MEFlowBlock/IMEFlowBlock ns=Expansions.Missions.Flow
- unity(0): none | ksp: none | fan_in=3
- methods(1): UpdateMissionFlowUI
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowParser.cs
- L=600 kind=MonoBehaviour type=MEFlowParser bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Flow
- unity(59): Transform=20, GameObject=15, Instantiate/Destroy=4, TMPro=3, UnityEngine.UI=2, SetActive=2 | ksp: none | fan_in=16
- methods(15): Awake, BuildPathsByDFS, BuildReversePathsByDFS, ClearChildren, Create, CreateFlowStartNodes, CreateFlowUIBlock, CreateMissionFlowUI, CreateMissionFlowUI_Button, CreateMissionFlowUI_Toggle, ExtractPathGroups, ParseBlock, ParseMission, Start, UpdateFlowUIItems
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowThenBlock.cs
- L=20 kind=class type=MEFlowThenBlock bases=MEFlowBlock anc=MEFlowBlock/IMEFlowBlock ns=Expansions.Missions.Flow
- unity(0): none | ksp: none | fan_in=4
- methods(1): UpdateMissionFlowUI
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowUIGroup_Or.cs
- L=45 kind=MonoBehaviour type=MEFlowUIGroup_Or bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Flow
- unity(84): GameObject=4, Transform=2, SetActive=2, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1 | ksp: none | fan_in=1
- methods(3): Create, OnDestroy, OnUpdateFlowUI
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowUIGroup_Then.cs
- L=67 kind=MonoBehaviour type=MEFlowUIGroup_Then bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Flow
- unity(79): GameObject=4, Transform=2, SetActive=2, UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=1
- methods(5): Create, OnDestroy, OnUpdateFlowUI, SetBracketColor, SetUnreachable
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowUINode.cs
- L=381 kind=MonoBehaviour type=MEFlowUINode bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Flow
- unity(82): GameObject=35, SetActive=23, GetComponent=7, UnityEngine.UI=6, EventSystems=6, Lifecycle=3 | ksp: Localizer=14, HighLogic=11, Game=3, MissionSystem=1 | fan_in=4
- methods(11): Awake, Create, OnButtonClick, OnDestroy, OnToggle, OnUpdateFlowUI, OnVesselNodeButtonCallback, SetNode, SetNodeInfo, SetStatus, Start
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowUINodeLeader.cs
- L=13 kind=class type=MEFlowUINodeLeader bases=- anc=- ns=Expansions.Missions.Flow
- unity(35): Texture=1 | ksp: none | fan_in=0
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEFlowUIText.cs
- L=10 kind=MonoBehaviour type=MEFlowUIText bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Flow
- unity(55): TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(0): 
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MENodePathInfo.cs
- L=66 kind=class type=MENodePathInfo bases=IConfigNode anc=IConfigNode ns=Expansions.Missions.Flow
- unity(6): none | ksp: ConfigNode=4 | fan_in=5
- methods(3): AddPath, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEPath.cs
- L=94 kind=class type=MEPath bases=IConfigNode anc=IConfigNode ns=Expansions.Missions.Flow
- unity(4): none | ksp: ConfigNode=2 | fan_in=6
- methods(5): AddNodes, Clone, GetConvergence, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Flow/MEPathGroup.cs
- L=32 kind=class type=MEPathGroup bases=- anc=- ns=Expansions.Missions.Flow
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/IActionModule.cs
- L=21 kind=interface type=IActionModule bases=IConfigNode,IMENodeDisplay anc=- ns=Expansions.Missions
- unity(3): none | ksp: none | fan_in=6
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/IMENodeDisplay.cs
- L=40 kind=interface type=IMENodeDisplay bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=14
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/IMissionKerbal.cs
- L=17 kind=interface type=IMissionKerbal bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: ProtoCrewMember=9 | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/INodeBody.cs
- L=9 kind=interface type=INodeBody bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: CelestialBody=1 | fan_in=8
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/INodeOrbit.cs
- L=9 kind=interface type=INodeOrbit bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: Orbit=1 | fan_in=8
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/INodeWaypoint.cs
- L=11 kind=interface type=INodeWaypoint bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=12
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/IScoreableObjective.cs
- L=9 kind=interface type=IScoreableObjective bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=37
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ITestModule.cs
- L=21 kind=interface type=ITestModule bases=IConfigNode,IMENodeDisplay anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ITestNodeLabel.cs
- L=17 kind=interface type=ITestNodeLabel bases=- anc=- ns=Expansions.Missions
- unity(3): none | ksp: none | fan_in=14
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/LaunchSiteSituation.cs
- L=261 kind=class type=LaunchSiteSituation bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(39): Coroutine=8, GetComponent=4, GameObject=4, Transform=3, Instantiate/Destroy=1 | ksp: ConfigNode=5, Planetarium=3, Localizer=3, Vector3d=1, HighLogic=1 | fan_in=2
- methods(6): AddLaunchSite, Load, RemoveLaunchSite, Save, createEmptyLaunchSite, createLaunchSiteObject
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MEGUINodeIconGroup.cs
- L=89 kind=MonoBehaviour type=MEGUINodeIconGroup bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions
- unity(86): UnityEngine.UI=5, SetActive=4, GameObject=4, Texture=2, Lifecycle=2, TMPro=1 | ksp: none | fan_in=1
- methods(6): Awake, CollapseGroup, Create, OnButtonCollapsePressed, OnDestroy, SetHeaderColor
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MEScoreModule.cs
- L=29 kind=attribute type=MEScoreModule bases=Attribute anc=Attribute ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=42
- methods(1): IsModuleAllowed
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionAwards.cs
- L=116 kind=class type=MissionAwards bases=DynamicModuleList anc=DynamicModuleList/IConfigNode ns=Expansions.Missions
- unity(6): none | ksp: MissionSystem=1, HighLogic=1 | fan_in=2
- methods(6): ContainsDefinition, EvaluateAwards, FillAwardsList, GetSupportedTypes, StartTracking, StopTracking
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionBlackListPreFlightCheck.cs
- L=95 kind=class type=MissionBlackListPreFlightCheck bases=IPreFlightTest anc=IPreFlightTest ns=Expansions.Missions
- unity(0): none | ksp: AvailablePart=4, Localizer=3, MissionSystem=2, ShipConstruct=1, PartLoader=1 | fan_in=1
- methods(5): GetAbortOption, GetProceedOption, GetWarningDescription, GetWarningTitle, Test
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionCameraLockOptions.cs
- L=16 kind=enum type=MissionCameraLockOptions bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionCameraModeOptions.cs
- L=14 kind=enum type=MissionCameraModeOptions bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionCelestialBody.cs
- L=120 kind=class type=MissionCelestialBody bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: FlightGlobals=6, CelestialBody=4, Localizer=3, ConfigNode=2 | fan_in=15
- methods(5): GetNodeBodyParameterString, IsValid, Load, Save, SetCelestialBodies
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionCheckpointValidator.cs
- L=193 kind=class type=MissionCheckpointValidator bases=- anc=- ns=Expansions.Missions
- unity(2): none | ksp: none | fan_in=6
- methods(7): CheckCheckpointPathRecursive, CompareAnyLists, CompareObjectLists, CompareStructLists, IsCheckpointNodeDirty, ModulesAreEqual, ValidateCheckpoint
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionCraft.cs
- L=183 kind=class type=MissionCraft bases=- anc=- ns=Expansions.Missions
- unity(2): none | ksp: ConfigNode=10, FlightGlobals=3, PartLoader=1 | fan_in=13
- methods(5): Clear, GetPartName, GetPartNameInVessel, GetPartPersistentId, checkPersistentIds
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionDifficulty.cs
- L=16 kind=enum type=MissionDifficulty bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=5
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionDifficultyWithColor.cs
- L=16 kind=enum type=MissionDifficultyWithColor bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionEndOptions.cs
- L=12 kind=enum type=MissionEndOptions bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionFileInfo.cs
- L=309 kind=class type=MissionFileInfo bases=- anc=- ns=Expansions.Missions
- unity(15): Steam=3 | ksp: MissionSystem=4, ConfigNode=4, HighLogic=1 | fan_in=11
- methods(6): CreateFromPath, GetSteamFileId, IsCompatible, IsCraftCompatible, IsIndividualFolderCraftCompatible, UpdateSteamState
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionFlow.cs
- L=42 kind=class type=MissionFlow bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: ConfigNode=5 | fan_in=2
- methods(2): Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionImporting.cs
- L=142 kind=class type=MissionImporting bases=- anc=- ns=Expansions.Missions
- unity(22): none | ksp: Localizer=14, HighLogic=2, GameEvents=1 | fan_in=3
- methods(4): ImportNewZips, ImportZip, ImportZipConfirmed, MoveImportedFile
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionInstructor.cs
- L=133 kind=class type=MissionInstructor bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: Localizer=24, ConfigNode=2 | fan_in=3
- methods(7): Equals, GetHashCode, GetNodeBodyParameterString, Load, Save, SetInstructors, UpdateNodeBodyUI
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionKerbal.cs
- L=367 kind=class type=MissionKerbal bases=- anc=- ns=Expansions.Missions
- unity(1): none | ksp: ProtoCrewMember=35, Localizer=14, ConfigNode=2 | fan_in=12
- methods(20): Equals, GetHashCode, GetNodeBodyParameterString, Initialize, IsValid, KerbalChanged, KerbalTypeChanged, Load, OnKerbalControlCreated, RebuildDropDownOnChange, Save, SetKerbalList, SetKerbalTypeList, UpdateNodeBodyUI, onKerbalAdded, onKerbalNameChange, onKerbalRemoved, onKerbalStatusChange, onKerbalTypeChange, setcurrentKerbalAvailable
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionListGroup.cs
- L=121 kind=MonoBehaviour type=MissionListGroup bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions
- unity(79): UnityEngine.UI=5, SetActive=4, GameObject=4, Texture=2, Lifecycle=2, TMPro=1 | ksp: none | fan_in=2
- methods(9): AddMission, Awake, CollapseGroup, ContainsMission, Create, CreateDefault, OnButtonCollapsePressed, OnDestroy, SetHeaderColor
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionLocalizer.cs
- L=34 kind=class type=MissionLocalizer bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: ConfigNode=5 | fan_in=1
- methods(1): LoadMissionLocalizationFiles
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionMappedVessel.cs
- L=88 kind=class type=MissionMappedVessel bases=- anc=- ns=Expansions.Missions
- unity(4): none | ksp: ConfigNode=7, FlightGlobals=1 | fan_in=4
- methods(2): Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionMappedVesselsExtension.cs
- L=141 kind=static class type=MissionMappedVesselsExtension bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: Localizer=1 | fan_in=0
- methods(7): ConvertMappedId, Exists, Find, IndexCurrentVesselId, IndexMappedVesselId, IndexPartId, MappedVesselName
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionOrbit.cs
- L=341 kind=class type=MissionOrbit bases=OrbitSnapshot anc=OrbitSnapshot ns=Expansions.Missions
- unity(2): none | ksp: Orbit=25, FlightGlobals=6, ConfigNode=4, CelestialBody=3, Planetarium=1 | fan_in=12
- methods(10): Equals, GetEditorMaxOrbitRadius, GetHashCode, GetNodeBodyParameterString, GetUTOrbitRotation, Load, RelativeOrbit, Reset, Save, SetCelestialBodies
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionOrbitRenderer.cs
- L=66 kind=MonoBehaviour type=MissionOrbitRenderer bases=OrbitTargetRenderer anc=OrbitTargetRenderer/OrbitRenderer/OrbitRendererBase ns=Expansions.Missions
- unity(15): none | ksp: Localizer=12, Vessel=1, Orbit=1 | fan_in=3
- methods(5): Setup, apNode_OnUpdateCaption, ascNode_OnUpdateCaption, descNode_OnUpdateCaption, peNode_OnUpdateCaption
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionPack.cs
- L=73 kind=class type=MissionPack bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(5): none | ksp: Localizer=3, ConfigNode=2 | fan_in=5
- methods(4): CompareDisplayName, CompareOrder, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionParamsExtras.cs
- L=90 kind=class type=MissionParamsExtras bases=GameParameters.CustomParameterNode anc=CustomParameterNode/ParameterNode/IConfigNode ns=Expansions.Missions
- unity(4): none | ksp: none | fan_in=5
- methods(2): Interactible, SetDifficultyPreset
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionParamsFacilities.cs
- L=98 kind=class type=MissionParamsFacilities bases=GameParameters.CustomParameterNode anc=CustomParameterNode/ParameterNode/IConfigNode ns=Expansions.Missions
- unity(4): none | ksp: none | fan_in=2
- methods(1): SetDifficultyPreset
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionParamsGeneral.cs
- L=104 kind=class type=MissionParamsGeneral bases=GameParameters.CustomParameterNode anc=CustomParameterNode/ParameterNode/IConfigNode ns=Expansions.Missions
- unity(4): none | ksp: none | fan_in=8
- methods(2): Interactible, SetDifficultyPreset
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionPartFilter.cs
- L=300 kind=class type=MissionPartFilter bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(1): none | ksp: Localizer=3, AvailablePart=3, ConfigNode=2, PartLoader=1 | fan_in=2
- methods(15): ApplyPartsFilter, CheckPartExcludelist, GetExcludedParts, GetExcludedPartsFilterDictionary, Load, OnAvailablePartsCheckpointValidator, OnAvailablePartsParameterCreated, OnFilterValueChange, OnPartPickerNeedsToUpdateExcludedPartsFilter, OnUnavailablePartsCheckpointValidator, OnUnavailablePartsParameterCreated, RemovePartsFilter, Save, SetUpdatePartnerExcludedPartFilterCallback, UpdateExcludedPartsFilter
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionPreflightCheck.cs
- L=40 kind=class type=MissionPreflightCheck bases=IPreFlightTest anc=IPreFlightTest ns=Expansions.Missions
- unity(0): none | ksp: Localizer=3 | fan_in=1
- methods(5): GetAbortOption, GetProceedOption, GetWarningDescription, GetWarningTitle, Test
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionQuaternion.cs
- L=83 kind=class type=MissionQuaternion bases=- anc=- ns=Expansions.Missions
- unity(7): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionScore.cs
- L=124 kind=class type=MissionScore bases=DynamicModuleList anc=DynamicModuleList/IConfigNode ns=Expansions.Missions
- unity(6): none | ksp: Localizer=1 | fan_in=5
- methods(6): AddGlobalScoreModule, AwardScore, AwarededScoreDescription, GetSupportedTypes, ScoreDescription, SetDefaultGlobalScoreModules
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionScoreInfo.cs
- L=187 kind=class type=MissionScoreInfo bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: ConfigNode=11, HighLogic=1 | fan_in=2
- methods(11): AddScore, CompareScoreInfo, DeleteMissionScores, DeleteScore, GetMissionAwards, GetMissionScore, GetMissionScores, Load, LoadScores, Save, SaveScores
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionsUtils.cs
- L=664 kind=static class type=MissionsUtils bases=- anc=- ns=Expansions.Missions
- unity(28): GameObject=12, Steam=6, Texture=5, Transform=2, GetComponent=1 | ksp: HighLogic=10, ConfigNode=6, Localizer=4, GameSettings=3, MissionSystem=1 | fan_in=57
- methods(29): AddEdge, AddMissionPackFromNode, AddMissionsScenarios, AddVertex, BreadthSearch, GatherMissionFiles, GatherMissionPacks, GetBaseMissionsPath, GetFacilityLimit, GetFieldValueForDisplay, GetMissionsFolder, GetNodesOrdered, GetSteamWorkshopMissions, GetStockMissionsPath, GetTextureInExternalPath, GetUsersMissionsPath, InitialiseAdjusterTypes, InstantiateMEFlowParser, MEPrefab, METexture, OpenMissionBuilder, ParseMissionPacksCFG, RebuildPacksList, RemoveMissionSaves ...
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/MissionTypes.cs
- L=10 kind=enum type=MissionTypes bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=10
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/NodeLabel.cs
- L=16 kind=MonoBehaviour type=NodeLabel bases=BaseLabel anc=BaseLabel/MonoBehaviour ns=Expansions.Missions
- unity(15): none | ksp: none | fan_in=2
- methods(1): Setup
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ParamChoices_CelestialBodySurface.cs
- L=149 kind=class type=ParamChoices_CelestialBodySurface bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(1): none | ksp: ConfigNode=4, Localizer=3 | fan_in=10
- methods(9): GetExtraText, GetWaypoint, GetWorldPosition, HasNodeLabel, HasWaypoint, HasWorldPosition, Load, NodeBodyParameterString, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ParamChoices_VesselSimpleLocation.cs
- L=65 kind=class type=ParamChoices_VesselSimpleLocation bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: ConfigNode=4 | fan_in=6
- methods(3): GetNodeBodyParameterString, Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MessageNodeDialog.cs
- L=389 kind=MonoBehaviour type=MessageNodeDialog bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Runtime
- unity(83): GameObject=28, SetActive=15, UnityEngine.UI=8, Transform=7, GetComponent=7, Instantiate/Destroy=6 | ksp: HighLogic=3, Planetarium=2, GameEvents=2 | fan_in=1
- methods(10): AssignInstructorPosition, Awake, ClearInstructorPosition, CloseDialog, OnDestroy, OnSceneChange, Spawn, Start, Update, onClickContinueBtn
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionAppMode.cs
- L=9 kind=enum type=MissionAppMode bases=- anc=- ns=Expansions.Missions.Runtime
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionEndDialog.cs
- L=323 kind=MonoBehaviour type=MissionEndDialog bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Runtime
- unity(82): SetActive=14, GameObject=14, TMPro=10, UnityEngine.UI=8, GetComponent=7, Lifecycle=3 | ksp: HighLogic=13, Localizer=6, FlightGlobals=4, MissionSystem=2, InputLockManager=2 | fan_in=1
- methods(12): Awake, Close, Display, DisplayDetails, DisplayScore, OnDestroy, Restart, Revert, SaveAndQuit, SetupGUI, Update, drawRevertOptions
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionsAppObjectiveInfo.cs
- L=23 kind=class type=MissionsAppObjectiveInfo bases=- anc=- ns=Expansions.Missions.Runtime
- unity(62): UnityEngine.UI=2, TMPro=2 | ksp: none | fan_in=0
- methods(0): 
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionsAppVesselInfo.cs
- L=28 kind=class type=MissionsAppVesselInfo bases=- anc=- ns=Expansions.Missions.Runtime
- unity(55): UnityEngine.UI=2, TMPro=1 | ksp: none | fan_in=3
- methods(0): 
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Runtime/MissionSteamItemWidget.cs
- L=72 kind=MonoBehaviour type=MissionSteamItemWidget bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Runtime
- unity(86): TMPro=5, UnityEngine.UI=3, Texture=2, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1 | ksp: none | fan_in=1
- methods(4): Create, SetHeaderText, SetSubIconState, Start
- auto: C / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Scenery/Scripts/PositionMobileLaunchPad.cs
- L=415 kind=MonoBehaviour type=PositionMobileLaunchPad bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Missions.Scenery.Scripts
- unity(77): Transform=28, Material=9, GameObject=5, Collider=5, SetActive=4, Raycast/Query=3 | ksp: HighLogic=4, Planetarium=3, Vessel=2, GameEvents=2, Vector3d=1 | fan_in=4
- methods(15): Awake, CheckForVessels, CheckGrounded, CompleteOrientation, CreatePoints, ExtendLegs, FindCollider, MakePoint, MoveModel, OnDestroy, OnScenerySettingChanged, ResetLegs, ResetPositioning, ToggleObjects, ToggleRampObjects
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreInfo.cs
- L=66 kind=class type=ScoreInfo bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: ConfigNode=2 | fan_in=2
- methods(2): Load, Save
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreModifierType.cs
- L=16 kind=enum type=ScoreModifierType bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreModule.cs
- L=65 kind=class type=ScoreModule bases=DynamicModule anc=DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: ConfigNode=2 | fan_in=6
- methods(6): AwardScore, AwarededScoreDescription, GetNodeBodyParameterString, Load, Save, ScoreDescription
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreModule_Accuracy.cs
- L=123 kind=class type=ScoreModule_Accuracy bases=ScoreModule anc=ScoreModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: Localizer=5, ConfigNode=5 | fan_in=9
- methods(10): AwardScore, Equals, GetDefaultPinnedParameters, GetDisplayName, GetHashCode, GetInfo, GetNodeBodyParameterString, Load, Save, ScoreDescription
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreModule_Completion.cs
- L=85 kind=class type=ScoreModule_Completion bases=ScoreModule anc=ScoreModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: Localizer=4, ConfigNode=2 | fan_in=3
- methods(10): AwardScore, Equals, GetDefaultPinnedParameters, GetDisplayName, GetDisplayToolTip, GetHashCode, GetInfo, Load, Save, ScoreDescription
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreModule_Modifier.cs
- L=124 kind=class type=ScoreModule_Modifier bases=ScoreModule anc=ScoreModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: Localizer=10, ConfigNode=2 | fan_in=2
- methods(11): AwardScore, Equals, GetDefaultPinnedParameters, GetDisplayName, GetDisplayToolTip, GetHashCode, GetInfo, GetNodeBodyParameterString, Load, Save, ScoreDescription
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreModule_Resource.cs
- L=145 kind=class type=ScoreModule_Resource bases=ScoreModule anc=ScoreModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: Localizer=5, ConfigNode=5, PartResourceDefinition=3, Vessel=2, FlightGlobals=1 | fan_in=34
- methods(11): AwardScore, Equals, GetDefaultPinnedParameters, GetDisplayName, GetHashCode, GetInfo, GetNodeBodyParameterString, Load, Save, ScoreDescription, SetResourceNames
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreModule_Time.cs
- L=126 kind=class type=ScoreModule_Time bases=ScoreModule anc=ScoreModule/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: Localizer=5, ConfigNode=5, Planetarium=1 | fan_in=34
- methods(10): AwardScore, Equals, GetDefaultPinnedParameters, GetDisplayName, GetHashCode, GetInfo, GetNodeBodyParameterString, Load, Save, ScoreDescription
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/ScoreRange.cs
- L=61 kind=class type=ScoreRange bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(2): none | ksp: ConfigNode=2 | fan_in=6
- methods(5): Equals, GetHashCode, Load, Save, isValueInRange
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/SteamMissionFileInfo.cs
- L=85 kind=class type=SteamMissionFileInfo bases=- anc=- ns=Expansions.Missions
- unity(12): Steam=1 | ksp: ConfigNode=6, Localizer=1 | fan_in=2
- methods(3): CreateMetaData, ProcessMetaData, SetMissionFileInfo
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/SurfaceArea.cs
- L=177 kind=class type=SurfaceArea bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(3): none | ksp: FlightGlobals=8, Localizer=3, ConfigNode=3, CelestialBody=2 | fan_in=11
- methods(14): Equals, GetExtraText, GetHashCode, GetNodeBodyParameterString, GetWaypoint, GetWorldPosition, HasNodeLabel, HasWaypoint, HasWorldPosition, IsPointInCircle, Load, PointInCircleAccuracy, Save, SetCelestialBodies
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/SurfaceVolume.cs
- L=295 kind=class type=SurfaceVolume bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(3): none | ksp: Localizer=14, FlightGlobals=8, CelestialBody=5, ConfigNode=3, Vector3d=2 | fan_in=3
- methods(15): AjustToBody, Equals, GetExtraText, GetHashCode, GetNodeBodyParameterString, GetWaypoint, GetWorldPosition, HasNodeLabel, HasWaypoint, HasWorldPosition, IsInsideVolume, Load, ResetBodyToDefaultValues, Save, SetCelestialBodies
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/TestComparisonLessGreaterEqual.cs
- L=14 kind=enum type=TestComparisonLessGreaterEqual bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=9
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/TestComparisonLessGreaterOnly.cs
- L=12 kind=enum type=TestComparisonLessGreaterOnly bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=8
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/TestComparisonOperator.cs
- L=18 kind=enum type=TestComparisonOperator bases=- anc=- ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=9
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/TestGroup.cs
- L=159 kind=class type=TestGroup bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(13): GetComponent=1, GameObject=1 | ksp: ConfigNode=6, GameEvents=2 | fan_in=19
- methods(13): AddTestModule, ClearGroup, ClearTestModules, Cleared, InitializeTestModules, Initialized, Load, OnCloned, RunValidation, RunValidationWrapper, Save, Test, initializeGroup
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/TestModule.cs
- L=320 kind=MonoBehaviour type=TestModule bases=MonoBehaviour,IConfigNode,ITestModule anc=MonoBehaviour ns=Expansions.Missions
- unity(26): Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: GameEvents=10, Localizer=5, Vessel=4, ConfigNode=2 | fan_in=31
- methods(40): AddParameterToNodeBody, AddParameterToNodeBodyAndUpdateUI, AddParameterToSAP, Awake, ClearTest, Cleared, GetAppObjectiveInfo, GetDisplayName, GetInfo, GetInternalParametersToDisplay, GetName, GetNode, GetNodeBodyParameterString, HasNodeBodyParameter, HasSAPParameter, Initialize, InitializeTest, Initialized, Load, OnDestroy, OnPartPersistentIdChanged, OnVesselDocking, OnVesselPersistentIdChanged, OnVesselsUndocking ...
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestAccuracy.cs
- L=77 kind=MonoBehaviour type=TestAccuracy bases=TestModule anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(32): Lifecycle=1 | ksp: Localizer=3, ConfigNode=2 | fan_in=0
- methods(6): Awake, GetInfo, GetNodeBodyParameterString, Load, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestCrewAssignment.cs
- L=246 kind=MonoBehaviour type=TestCrewAssignment bases=TestModule,IScoreableObjective anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(25): Lifecycle=1 | ksp: Localizer=9, FlightGlobals=9, Vessel=3, ProtoCrewMember=3, ConfigNode=2 | fan_in=1
- methods(13): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Initialize, Load, OnPartPersistentIdChanged, OnVesselPersistentIdChanged, OnVesselsUndocking, RunValidation, Save, Test, partContainsKerbal
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestEmptyFalse.cs
- L=23 kind=MonoBehaviour type=TestEmptyFalse bases=TestModule anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(36): Lifecycle=1 | ksp: Localizer=2 | fan_in=0
- methods(3): Awake, GetInfo, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestEmptyTrue.cs
- L=23 kind=MonoBehaviour type=TestEmptyTrue bases=TestModule anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(36): Lifecycle=1 | ksp: Localizer=2 | fan_in=0
- methods(3): Awake, GetInfo, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestFlyThrough.cs
- L=141 kind=MonoBehaviour type=TestFlyThrough bases=TestVessel,IScoreableObjective,INodeWaypoint anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(23): Lifecycle=1 | ksp: FlightGlobals=3, Localizer=2, ConfigNode=2 | fan_in=0
- methods(14): Awake, GetExtraText, GetInfo, GetNodeBodyParameterString, GetNodeWaypoint, GetScoreModifier, GetWorldPosition, HasNodeLabel, HasNodeWaypoint, HasWorldPosition, Load, OnShowLabelChanged, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestFundsRecovery.cs
- L=136 kind=MonoBehaviour type=TestFundsRecovery bases=TestModule,IScoreableObjective anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(21): Lifecycle=1 | ksp: Localizer=3, GameEvents=2, ConfigNode=2, ProtoVessel=1 | fan_in=0
- methods(10): Awake, Cleared, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Initialized, Load, Recovered, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestGameEvent.cs
- L=55 kind=MonoBehaviour type=TestGameEvent bases=TestModule,IScoreableObjective anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(28): Lifecycle=1 | ksp: ConfigNode=2, GameEvents=1 | fan_in=0
- methods(7): Awake, EventCallback_StageSeparation, GetScoreModifier, Load, Save, SuscribeToEvent, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestGoTo.cs
- L=161 kind=MonoBehaviour type=TestGoTo bases=TestVessel,IScoreableObjective,INodeWaypoint anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(40): SetActive=2, GameObject=2, SendMessage/Invoke=1, Lifecycle=1 | ksp: FlightGlobals=4, Vessel=2, Localizer=2, ConfigNode=2 | fan_in=0
- methods(16): Awake, GetExtraText, GetInfo, GetNodeBodyParameterString, GetNodeWaypoint, GetScoreModifier, GetWorldPosition, HasNodeLabel, HasNodeWaypoint, HasWorldPosition, Load, OnIgnoreSurfaceVelocity, OnSurfaceVelocityControlCreated, OnSurfaceVelocitySetState, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestGrapple.cs
- L=434 kind=MonoBehaviour type=TestGrapple bases=TestModule anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(37): SetActive=6, GameObject=6, Lifecycle=1 | ksp: Localizer=18, FlightGlobals=14, GameEvents=9, Vessel=6, Part=5 | fan_in=0
- methods(22): Awake, Cleared, GetInfo, GetNodeBodyParameterString, Initialized, Load, OnAsteroidControlCreated, OnCometControlCreated, OnSpaceObjectChoiceControlCreated, OnSpaceObjectChoiceControlSetup, OnSpaceObjectChoiceValueChange, OnVesselControlCreated, OnVesselDocking, OnVesselPersistentIdChanged, ParameterSetupComplete, RunValidation, Save, SetDockedUndockedDropdown, Test, onPartCouple, onPartUndock, testPartVessels
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestKerbalBoarded.cs
- L=261 kind=MonoBehaviour type=TestKerbalBoarded bases=TestModule,IScoreableObjective,INodeWaypoint anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(18): Lifecycle=1 | ksp: FlightGlobals=8, Localizer=7, GameEvents=5, ProtoCrewMember=3, ConfigNode=2 | fan_in=0
- methods(21): Awake, CheckSituation, Cleared, GetExtraText, GetInfo, GetNodeBodyParameterString, GetNodeWaypoint, GetScoreModifier, GetWorldPosition, HasNodeLabel, HasNodeWaypoint, HasWorldPosition, Initialize, Initialized, Load, OnBoarded, OnSeated, OnVesselPersistentIdChanged, RunValidation, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestKerbalEVA.cs
- L=244 kind=MonoBehaviour type=TestKerbalEVA bases=TestModule,IScoreableObjective,INodeWaypoint anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(19): Lifecycle=1 | ksp: FlightGlobals=8, Localizer=7, GameEvents=3, ProtoCrewMember=2, Part=2 | fan_in=0
- methods(20): Awake, Cleared, GetExtraText, GetInfo, GetNodeBodyParameterString, GetNodeWaypoint, GetScoreModifier, GetWorldPosition, HasNodeLabel, HasNodeWaypoint, HasWorldPosition, Initialize, Initialized, Load, OnEVA, OnVesselPersistentIdChanged, RunValidation, Save, ShouldCreateCheckpoint, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestKerbalKilled.cs
- L=103 kind=MonoBehaviour type=TestKerbalKilled bases=TestModule anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(22): Lifecycle=1 | ksp: ProtoCrewMember=5, Localizer=3, HighLogic=2, GameEvents=2, ConfigNode=2 | fan_in=0
- methods(11): Awake, Cleared, GetInfo, GetNodeBodyParameterString, Initialize, Initialized, Load, OnKilled, RunValidation, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestKerbalRecovery.cs
- L=141 kind=MonoBehaviour type=TestKerbalRecovery bases=TestModule,IScoreableObjective anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(20): Lifecycle=1 | ksp: ProtoCrewMember=5, Localizer=3, HighLogic=2, GameEvents=2, ConfigNode=2 | fan_in=0
- methods(13): Awake, Cleared, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Initialize, Initialized, Load, OnRecovered, RunValidation, Save, Test, checkAlreadyRecovered
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestKerbalRescue.cs
- L=169 kind=MonoBehaviour type=TestKerbalRescue bases=TestModule,IScoreableObjective anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(20): Lifecycle=1 | ksp: GameEvents=9, ProtoCrewMember=6, Vessel=3, Part=3, Localizer=3 | fan_in=0
- methods(15): Awake, CheckNewKerbal, Cleared, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Initialize, Initialized, Load, OnCrewBoardVessel, OnCrewTransferred, RunValidation, Save, Test, onKnowledgeChanged
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestMETTime.cs
- L=83 kind=MonoBehaviour type=TestMETTime bases=TestVessel,IScoreableObjective anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(24): Lifecycle=1 | ksp: Localizer=2, ConfigNode=2, FlightGlobals=1 | fan_in=0
- methods(7): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestMissionFunds.cs
- L=110 kind=MonoBehaviour type=TestMissionFunds bases=TestModule,IScoreableObjective anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(25): Lifecycle=1 | ksp: Funding=6, Localizer=3, ConfigNode=2 | fan_in=0
- methods(7): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestMissionScore.cs
- L=106 kind=MonoBehaviour type=TestMissionScore bases=TestModule,IScoreableObjective anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(25): Lifecycle=1 | ksp: Localizer=3, ConfigNode=2 | fan_in=0
- methods(7): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestMissionTime.cs
- L=80 kind=MonoBehaviour type=TestMissionTime bases=TestModule,IScoreableObjective anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(24): Lifecycle=1 | ksp: Planetarium=2, Localizer=2, ConfigNode=2, FlightGlobals=1 | fan_in=0
- methods(7): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestOrbit.cs
- L=241 kind=MonoBehaviour type=TestOrbit bases=TestVessel,IScoreableObjective,INodeOrbit anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(30): Coroutine=4, Lifecycle=1 | ksp: Localizer=20, FlightGlobals=9, Orbit=5, HighLogic=4, ConfigNode=4 | fan_in=1
- methods(15): Awake, Cleared, CreateOrbitRenderer, DrawOrbit, GetInfo, GetNodeBodyParameterString, GetNodeOrbit, GetScoreModifier, HasNodeOrbit, Initialized, Load, OnOrbitAccuracyChanged, Save, Test, initializeDrawOrbit
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestOrbitParams.cs
- L=684 kind=MonoBehaviour type=TestOrbitParams bases=TestVessel,IScoreableObjective anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(34): SetActive=8, GameObject=8, Lifecycle=2 | ksp: Localizer=11, Vessel=4, FlightGlobals=4, CelestialBody=3, Planetarium=2 | fan_in=0
- methods(39): Awake, Cleared, GetInfo, GetMaxAp, GetNodeBodyParameterString, GetScoreModifier, Initialized, Load, OnBodyChanged, OnDestroy, OnInclinationAccuracyChanged, OnLANAccuracyChanged, ParameterSetupComplete, RunValidation, Save, Test, onApMaxValueChanged, onApMinValueChanged, onApoapsisMaxCreated, onApoapsisMinCreated, onBodyCreated, onCheckApoapsisValueChanged, onCheckIncValueChanged, onCheckLANValueChanged ...
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestPartDocking.cs
- L=375 kind=MonoBehaviour type=TestPartDocking bases=TestModule anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(23): Lifecycle=1 | ksp: Localizer=26, ConfigNode=10, GameEvents=9, FlightGlobals=6, Part=3 | fan_in=1
- methods(18): Awake, Cleared, GetInfo, GetNodeBodyParameterString, Initialized, Load, OnPartPersistentIdChanged, OnVesselDocking, OnVesselPersistentIdChanged, RunValidation, Save, SetDockedUndockedDropdown, Test, findPartVesselId, onPartCouple, onPartUndock, setPlayerCreatedRootParts, testPartVessels
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestPlantFlag.cs
- L=170 kind=MonoBehaviour type=TestPlantFlag bases=TestModule,IScoreableObjective,INodeWaypoint anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(20): Lifecycle=1 | ksp: FlightGlobals=4, Localizer=2, GameEvents=2, ConfigNode=2, CelestialBody=2 | fan_in=0
- methods(17): AfterFlagPlanted, Awake, Cleared, GetExtraText, GetInfo, GetNodeBody, GetNodeWaypoint, GetScoreModifier, GetWorldPosition, HasNodeBody, HasNodeLabel, HasNodeWaypoint, HasWorldPosition, Initialized, Load, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestResourcesRecovery.cs
- L=203 kind=MonoBehaviour type=TestResourcesRecovery bases=TestVessel,IScoreableObjective anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(21): Lifecycle=1 | ksp: PartResourceDefinition=4, Localizer=4, FlightGlobals=3, ProtoPartSnapshot=2, GameEvents=2 | fan_in=0
- methods(13): Awake, Cleared, CompareResource, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Initialized, Load, ParameterSetupComplete, Recovered, Save, SetResourceNames, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestScienceExperiment.cs
- L=282 kind=MonoBehaviour type=TestScienceExperiment bases=TestModule,IScoreableObjective,INodeBody anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(18): Lifecycle=1 | ksp: Localizer=19, ResearchAndDevelopment=5, GameEvents=4, ConfigNode=4, FlightGlobals=2 | fan_in=0
- methods(19): Awake, Cleared, ConductScienceTest, GetAppObjectiveInfo, GetInfo, GetNodeBody, GetNodeBodyParameterString, GetScoreModifier, HasNodeBody, Initialized, Load, OnExperimentDeployed, OnScienceReceived, RunValidation, Save, TSEDoR_SetDropDownValues, TSEExpId_SetDropDownValues, TSEExpSit_SetDropDownValues, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestSOILeft.cs
- L=99 kind=MonoBehaviour type=TestSOILeft bases=TestVessel,IScoreableObjective,INodeBody anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(26): Lifecycle=1 | ksp: FlightGlobals=4, GameEvents=3, Localizer=2, ConfigNode=2, CelestialBody=2 | fan_in=0
- methods(11): Awake, Cleared, GetInfo, GetNodeBody, GetScoreModifier, HasNodeBody, Initialized, Load, OnEscapedSOI, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestSOIReached.cs
- L=98 kind=MonoBehaviour type=TestSOIReached bases=TestVessel,IScoreableObjective,INodeBody anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(26): Lifecycle=1 | ksp: FlightGlobals=4, GameEvents=3, Localizer=2, ConfigNode=2, CelestialBody=2 | fan_in=0
- methods(11): Awake, Cleared, GetInfo, GetNodeBody, GetScoreModifier, HasNodeBody, Initialized, Load, OnReachedSOI, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestTakeKerbal.cs
- L=316 kind=MonoBehaviour type=TestTakeKerbal bases=TestModule,IScoreableObjective,INodeWaypoint anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(18): Lifecycle=1 | ksp: FlightGlobals=10, Vessel=8, Localizer=7, GameEvents=6, ProtoCrewMember=5 | fan_in=0
- methods(23): Awake, CheckCrew, Cleared, GetExtraText, GetInfo, GetNodeBodyParameterString, GetNodeWaypoint, GetScoreModifier, GetWorldPosition, HasNodeLabel, HasNodeWaypoint, HasWorldPosition, Initialize, Initialized, Load, RunValidation, Save, Test, onVesselChange, onVesselCrewWasModified, scanVessels, targetVesselStillValid, testLocation
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestTimeSinceNode.cs
- L=128 kind=MonoBehaviour type=TestTimeSinceNode bases=TestModule,IScoreableObjective anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(21): Lifecycle=1 | ksp: Localizer=5, Planetarium=2, ConfigNode=2, FlightGlobals=1 | fan_in=0
- methods(9): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Initialize, Load, RunValidation, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVessel.cs
- L=161 kind=MonoBehaviour type=TestVessel bases=TestModule anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(22): Lifecycle=1 | ksp: Localizer=7, FlightGlobals=6, Vessel=4, ConfigNode=2, ProtoPartSnapshot=1 | fan_in=22
- methods(9): Awake, GetNodeBodyParameterString, Load, OnVesselDocking, OnVesselPersistentIdChanged, OnVesselsUndocking, Save, Test, vesselSelectorCreated
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselActive.cs
- L=27 kind=MonoBehaviour type=TestVesselActive bases=TestVessel anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(36): Lifecycle=1 | ksp: Localizer=1, FlightGlobals=1 | fan_in=0
- methods(3): Awake, GetInfo, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselActiveCount.cs
- L=179 kind=MonoBehaviour type=TestVesselActiveCount bases=TestModule anc=TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(19): Lifecycle=1 | ksp: GameEvents=4, Vessel=3, Localizer=2, FlightGlobals=2, ConfigNode=2 | fan_in=0
- methods(11): Awake, Cleared, GetInfo, GetNodeBodyParameterString, Initialized, Load, OnAddVessel, OnRemoveVessel, Save, Test, UpdateVesselCount
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselAltitude.cs
- L=118 kind=MonoBehaviour type=TestVesselAltitude bases=TestVessel,IScoreableObjective anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(24): Lifecycle=1 | ksp: Localizer=5, FlightGlobals=3, ConfigNode=2 | fan_in=0
- methods(7): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselCrewCount.cs
- L=167 kind=MonoBehaviour type=TestVesselCrewCount bases=TestVessel,IScoreableObjective anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(22): Lifecycle=1 | ksp: Localizer=6, GameDatabase=5, FlightGlobals=3, ProtoCrewMember=2, ConfigNode=2 | fan_in=0
- methods(8): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, SetKerbalRoles, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselMass.cs
- L=97 kind=MonoBehaviour type=TestVesselMass bases=TestVessel,IScoreableObjective anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(26): Lifecycle=1 | ksp: Localizer=3, FlightGlobals=3, ConfigNode=2 | fan_in=0
- methods(7): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselPartCount.cs
- L=147 kind=MonoBehaviour type=TestVesselPartCount bases=TestVessel,IScoreableObjective anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(23): Lifecycle=1 | ksp: FlightGlobals=7, Localizer=3, ConfigNode=2 | fan_in=0
- methods(8): Awake, CheckForUnloadedVessel, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselRecovery.cs
- L=44 kind=MonoBehaviour type=TestVesselRecovery bases=TestVessel anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(30): Lifecycle=1 | ksp: GameEvents=2, ProtoVessel=1, Localizer=1 | fan_in=0
- methods(6): Awake, Cleared, GetInfo, Initialized, Recovered, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselResource.cs
- L=167 kind=MonoBehaviour type=TestVesselResource bases=TestVessel,IScoreableObjective anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(22): Lifecycle=1 | ksp: PartResourceDefinition=4, Localizer=2, ConfigNode=2, ProtoPartSnapshot=1 | fan_in=0
- methods(8): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, SetResourceNames, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselSituation.cs
- L=237 kind=MonoBehaviour type=TestVesselSituation bases=TestVessel,IScoreableObjective anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(26): SetActive=1, Lifecycle=1, GameObject=1 | ksp: Localizer=6, FlightGlobals=3, Vessel=2, Planetarium=2, ConfigNode=2 | fan_in=3
- methods(18): Awake, GetExtraText, GetInfo, GetNodeBody, GetNodeBodyParameterString, GetNodeWaypoint, GetScoreModifier, GetWorldPosition, HasNodeBody, HasNodeLabel, HasNodeWaypoint, HasWorldPosition, Initialized, Load, OnIgnoreSurfaceVelocity, OnSurfaceVelocityControlCreated, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselSituationFlying.cs
- L=68 kind=MonoBehaviour type=TestVesselSituationFlying bases=TestVesselSituation,IScoreableObjective,INodeWaypoint anc=TestVesselSituation/TestVessel/TestModule ns=Expansions.Missions.Tests
- unity(26): Lifecycle=1 | ksp: Localizer=2, FlightGlobals=2, ConfigNode=2, Vessel=1 | fan_in=0
- methods(5): Awake, GetInfo, Load, ParameterSetupComplete, Save
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselSituationLanded.cs
- L=69 kind=MonoBehaviour type=TestVesselSituationLanded bases=TestVesselSituation,INodeWaypoint,ITestNodeLabel anc=TestVesselSituation/TestVessel/TestModule ns=Expansions.Missions.Tests
- unity(25): Lifecycle=1 | ksp: Localizer=2, FlightGlobals=2, ConfigNode=2, Vessel=1 | fan_in=0
- methods(5): Awake, GetInfo, Load, ParameterSetupComplete, Save
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselSituationSplashed.cs
- L=64 kind=MonoBehaviour type=TestVesselSituationSplashed bases=TestVesselSituation,INodeWaypoint,ITestNodeLabel anc=TestVesselSituation/TestVessel/TestModule ns=Expansions.Missions.Tests
- unity(26): Lifecycle=1 | ksp: FlightGlobals=2, ConfigNode=2, Vessel=1, Localizer=1 | fan_in=0
- methods(4): Awake, Load, ParameterSetupComplete, Save
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselStage.cs
- L=121 kind=MonoBehaviour type=TestVesselStage bases=TestVessel,IScoreableObjective anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(24): Lifecycle=1 | ksp: Localizer=3, FlightGlobals=3, ConfigNode=2 | fan_in=0
- methods(7): Awake, GetInfo, GetNodeBodyParameterString, GetScoreModifier, Load, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselStageActivated.cs
- L=88 kind=MonoBehaviour type=TestVesselStageActivated bases=TestVessel,IScoreableObjective anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(27): Lifecycle=1 | ksp: FlightGlobals=4, Localizer=2, GameEvents=2, ConfigNode=2 | fan_in=0
- methods(9): Awake, Cleared, GetInfo, GetScoreModifier, Initialized, Load, Save, StageActivated, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselState.cs
- L=70 kind=MonoBehaviour type=TestVesselState bases=TestVessel,IScoreableObjective anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(34): Lifecycle=1 | ksp: Vessel=3, FlightGlobals=3, ConfigNode=2, Localizer=1 | fan_in=0
- methods(5): Awake, GetScoreModifier, Load, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselStateCrashed.cs
- L=128 kind=MonoBehaviour type=TestVesselStateCrashed bases=TestVessel,IScoreableObjective anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(24): Lifecycle=1 | ksp: FlightGlobals=5, Localizer=2, GameEvents=2, ConfigNode=2, Vessel=1 | fan_in=0
- methods(10): Awake, Cleared, GetInfo, GetScoreModifier, Initialized, Load, OnVesselToBeDestroyed, Save, Test, TestVesselLocation
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Tests/TestVesselVelocity.cs
- L=360 kind=MonoBehaviour type=TestVesselVelocity bases=TestVessel anc=TestVessel/TestModule/MonoBehaviour ns=Expansions.Missions.Tests
- unity(48): SetActive=10, GameObject=10, Lifecycle=1 | ksp: Localizer=10, FlightGlobals=10, Vessel=6, ConfigNode=5, ProtoCrewMember=3 | fan_in=0
- methods(18): Awake, CheckCrew, GetInfo, GetNodeBodyParameterString, Initialize, Load, OnSpeedTypeControlCreated, OnSpeedTypeControlSetup, OnVelocityWRTAsteroidControlCreated, OnVelocityWRTBodyControlCreated, OnVelocityWRTBodyControlSetup, OnVelocityWRTBodyValueChange, OnVelocityWRTCometControlCreated, OnVelocityWRTKerbalControlCreated, OnVelocityWRTVesselControlCreated, ParameterSetupComplete, Save, Test
- auto: B / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselGroundLocation.cs
- L=243 kind=class type=VesselGroundLocation bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(2): none | ksp: FlightGlobals=8, Localizer=3, ConfigNode=3, CelestialBody=1 | fan_in=14
- methods(14): Equals, GetExtraText, GetHashCode, GetNodeBodyParameterString, GetWaypoint, GetWorldPosition, HasNodeLabel, HasWaypoint, Load, OnGizmoIconChangeDelegate, OnVesselIconChange, Save, SetAvailableGizmoIcons, SetCelestialBodies
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselLocation.cs
- L=130 kind=class type=VesselLocation bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(3): none | ksp: ConfigNode=4, FlightGlobals=2, ResearchAndDevelopment=1 | fan_in=5
- methods(4): Load, Save, SelectedlaunchSite_SetDropDownValues, createZeroOrbit
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselPartIDPair.cs
- L=88 kind=class type=VesselPartIDPair bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(4): none | ksp: ConfigNode=2, ShipConstruct=1, Localizer=1 | fan_in=8
- methods(5): Equals, GetHashCode, Load, Save, ValidatePartAgainstCraft
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction.cs
- L=127 kind=class type=VesselRestriction bases=DynamicModule anc=DynamicModule/IConfigNode ns=Expansions.Missions
- unity(16): Time.=1, TMPro=1, GetComponent=1 | ksp: Localizer=2 | fan_in=13
- methods(13): AddAppUIReference, ClearAppUIREference, ClearEvents, GetDisplayName, GetPreflightCheck, GetStateMessage, IsComplete, SameComparator, SameComparatorWrapper, SuscribeToEvents, TestFloat, TestInt, UpdateAppEntry
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_Cost.cs
- L=115 kind=class type=VesselRestriction_Cost bases=VesselRestriction anc=VesselRestriction/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: GameEvents=8, Localizer=3, ConfigNode=3, ShipConstruct=1, Part=1 | fan_in=0
- methods(11): Callback_PlacePart, Callback_ShipLoad, ClearEvents, GetDisplayName, GetShipCurrentCost, GetStateMessage, IsComplete, Load, SameComparator, Save, SuscribeToEvents
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_Crew.cs
- L=168 kind=class type=VesselRestriction_Crew bases=VesselRestriction anc=VesselRestriction/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: Localizer=4, GameEvents=4, KerbalRoster=3, ConfigNode=3, ShipConstruct=1 | fan_in=0
- methods(12): Callback_CrewChange, Callback_ShipLoad, ClearEvents, GetCrewCount, GetCrewTraitCount, GetDisplayName, GetStateMessage, IsComplete, Load, SameComparator, Save, SuscribeToEvents
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_Mass.cs
- L=125 kind=class type=VesselRestriction_Mass bases=VesselRestriction anc=VesselRestriction/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: GameEvents=10, Localizer=5, ConfigNode=3, ShipConstruct=2, Part=1 | fan_in=2
- methods(13): Callback_CrewModified, Callback_PlacePart, Callback_ShipLoad, Callback_ShipModified, ClearEvents, GetCurrentMass, GetDisplayName, GetStateMessage, IsComplete, Load, SameComparator, Save, SuscribeToEvents
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_PartCount.cs
- L=113 kind=class type=VesselRestriction_PartCount bases=VesselRestriction anc=VesselRestriction/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: GameEvents=8, Localizer=5, ConfigNode=3, ShipConstruct=1, Part=1 | fan_in=1
- methods(11): Callback_PlacePart, Callback_ShipLoad, ClearEvents, GetCurrentParts, GetDisplayName, GetStateMessage, IsComplete, Load, SameComparator, Save, SuscribeToEvents
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_RequiredParts.cs
- L=101 kind=class type=VesselRestriction_RequiredParts bases=VesselRestriction anc=VesselRestriction/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: GameEvents=8, ShipConstruct=2, Localizer=2, ConfigNode=2, PartLoader=1 | fan_in=1
- methods(9): Callback_PlacePart, Callback_ShipLoad, ClearEvents, GetDisplayName, GetStateMessage, IsComplete, Load, Save, SuscribeToEvents
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_Resource.cs
- L=161 kind=class type=VesselRestriction_Resource bases=VesselRestriction anc=VesselRestriction/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: GameEvents=10, PartResourceDefinition=4, Localizer=4, ConfigNode=3, ShipConstruct=2 | fan_in=0
- methods(14): Callback_CrewModified, Callback_PlacePart, Callback_ShipLoad, Callback_ShipModified, ClearEvents, GetCurrentResourceValue, GetDisplayName, GetStateMessage, IsComplete, Load, SameComparator, Save, SetResourceNames, SuscribeToEvents
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_Size.cs
- L=153 kind=class type=VesselRestriction_Size bases=VesselRestriction anc=VesselRestriction/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(5): none | ksp: Localizer=11, GameEvents=8, ConfigNode=3, ShipConstruct=1, Part=1 | fan_in=1
- methods(13): CallBack_ShipLoad, Callback_PlacePart, Check, ClearEvents, GetDisplayName, GetShipDimensionsString, GetShipSize, GetStateMessage, IsComplete, Load, SameComparator, Save, SuscribeToEvents
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestriction_Stages.cs
- L=127 kind=class type=VesselRestriction_Stages bases=VesselRestriction anc=VesselRestriction/DynamicModule/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: GameEvents=12, Localizer=4, ConfigNode=3, Part=1 | fan_in=0
- methods(13): Callback_AddStage, Callback_ModifyStage, Callback_PlacePart, Callback_RemoveStage, ClearEvents, GetCurrentStageCount, GetDisplayName, GetStateMessage, IsComplete, Load, SameComparator, Save, SuscribeToEvents
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestrictionList.cs
- L=73 kind=class type=VesselRestrictionList bases=DynamicModuleList anc=DynamicModuleList/IConfigNode ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=1
- methods(3): ClearAppEvents, GetSupportedTypes, StartAppEvents
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselRestrictionOptions.cs
- L=9 kind=attribute type=VesselRestrictionOptions bases=Attribute anc=Attribute ns=Expansions.Missions
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/VesselSituation.cs
- L=309 kind=class type=VesselSituation bases=IConfigNode anc=IConfigNode ns=Expansions.Missions
- unity(4): none | ksp: ProtoCrewMember=5, ConfigNode=5, Localizer=3, KerbalRoster=2, HighLogic=2 | fan_in=33
- methods(13): ClearEditorEvents, GetRequiredExcludedParts, Load, OnAutoGenerateCrewControlCreated, OnPartPickerNeedsToUpdateExcludedPartsFilter, OnRequieredPartsCheckpointValidator, OnRequiredPartsParameterCreated, Save, SetCrewAvailable, SetVesselCrew, StartEditorEvents, UpdateAutoGenerateCrewSettings, UpdateRequiredPartsDisplayedParts
- auto: A / DLC-MakingHistory

## Assembly-CSharp/Expansions/Serenity/AxisLimitLine.cs
- L=79 kind=MonoBehaviour type=AxisLimitLine bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=Expansions.Serenity
- unity(82): EventSystems=8, Transform=2, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=1
- methods(9): Awake, OnBeginDrag, OnDestroy, OnDrag, OnEndDrag, OnPointerClick, SetToolTipValue, Setup, UpdateYPosition
- auto: C / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/Contracts/CollectDeployedScience.cs
- L=108 kind=ContractParameter type=CollectDeployedScience bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Expansions.Serenity.Contracts
- unity(4): none | ksp: Contract=6, GameEvents=3, CelestialBody=3, Localizer=2, ConfigNode=2 | fan_in=1
- methods(7): GetHashString, GetTitle, OnLoad, OnRegister, OnSave, OnScience, OnUnregister
- auto: A / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/Contracts/CollectROCScienceArm.cs
- L=130 kind=ContractParameter type=CollectROCScienceArm bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Expansions.Serenity.Contracts
- unity(3): none | ksp: Contract=6, GameEvents=3, CelestialBody=3, Localizer=2, ConfigNode=2 | fan_in=1
- methods(7): GetHashString, GetTitle, OnLoad, OnRegister, OnSave, OnScience, OnUnregister
- auto: A / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/Contracts/CollectROCScienceRetrieval.cs
- L=138 kind=ContractParameter type=CollectROCScienceRetrieval bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Expansions.Serenity.Contracts
- unity(3): none | ksp: Contract=4, ConfigNode=4, Localizer=3, CelestialBody=3, ProtoPartSnapshot=2 | fan_in=1
- methods(7): GetHashString, GetTitle, OnLoad, OnRegister, OnSave, OnUnregister, OnVesselRecoveryProcessing
- auto: A / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/Contracts/DeployedScienceContract.cs
- L=222 kind=Contract type=DeployedScienceContract bases=Contract anc=Contract/IContractParameterHost ns=Expansions.Serenity.Contracts
- unity(7): none | ksp: CelestialBody=9, ResearchAndDevelopment=4, Localizer=3, Reputation=2, Planetarium=2 | fan_in=0
- methods(11): Generate, GenerateContract_Surface, GetDescription, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave, PotentialsContainBody
- auto: A / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/Contracts/ROCScienceArmContract.cs
- L=316 kind=Contract type=ROCScienceArmContract bases=Contract anc=Contract/IContractParameterHost ns=Expansions.Serenity.Contracts
- unity(5): none | ksp: Localizer=8, Reputation=6, CelestialBody=6, AvailablePart=3, PartLoader=2 | fan_in=0
- methods(9): Generate, GetDescription, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave
- auto: A / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/Contracts/ROCScienceRetrievalContract.cs
- L=232 kind=Contract type=ROCScienceRetrievalContract bases=Contract anc=Contract/IContractParameterHost ns=Expansions.Serenity.Contracts
- unity(5): none | ksp: Localizer=6, CelestialBody=6, Reputation=2, FlightGlobals=2, ConfigNode=2 | fan_in=0
- methods(9): Generate, GetDescription, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave
- auto: A / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ControlledAction.cs
- L=204 kind=class type=ControlledAction bases=ControlledBase anc=ControlledBase/IConfigNode ns=Expansions.Serenity
- unity(11): none | ksp: Part=10, ConfigNode=5, PartModule=1 | fan_in=6
- methods(8): AddSymmetryPart, ClearSymmetryLists, OnAssignReferenceVars, OnChangeSymmetryMaster, OnLoad, OnSave, RescaleTimes, ReverseTimes
- auto: A / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ControlledAxis.cs
- L=501 kind=class type=ControlledAxis bases=ControlledBase anc=ControlledBase/IConfigNode ns=Expansions.Serenity
- unity(31): AnimationCurve=49 | ksp: FloatCurve=20, Part=9, ConfigNode=4, PartModule=1 | fan_in=6
- methods(23): AddSymmetryPart, AlignCurveEnds, ClampAllPointValues, ClampPointValue, ClearPresetRefs, ClearSymmetryLists, GenerateCurveFromReference, InvertCurve, OnAssignReferenceVars, OnChangeSymmetryMaster, OnLoad, OnSave, PresetFlat, PresetRevSaw, PresetSaw, PresetSine, PresetSquare, PresetTriangle, RescaleCurveTime, ReverseCurve, UpdateFieldValue, UpdatePreset, UpdateSoftLimits
- auto: A / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ControlledBase.cs
- L=184 kind=class type=ControlledBase bases=IConfigNode anc=IConfigNode ns=Expansions.Serenity
- unity(18): none | ksp: Part=16, ConfigNode=5, PartModule=2, FlightGlobals=1 | fan_in=4
- methods(12): AddSymmetryPart, AssignReferenceVars, ChangeSymmetryMaster, ClearSymmetryLists, Load, OnAssignReferenceVars, OnChangeSymmetryMaster, OnLoad, OnSave, RebuildSymmetryList, Save, SetPartNickName
- auto: A / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/CurvePanelPoint.cs
- L=232 kind=MonoBehaviour type=CurvePanelPoint bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=Expansions.Serenity
- unity(86): GameObject=11, AnimationCurve=10, EventSystems=8, Transform=7, SetActive=6, UnityEngine.UI=4 | ksp: none | fan_in=6
- methods(18): Awake, Delete, Deselect, HideDelete, OnBeginDrag, OnDeleteClick, OnDestroy, OnDrag, OnEndDrag, OnPointerClick, OnTangentDrag, Select, SetKeyFrame, SetKeyFrameTangent, SetKeyFrameTangents, SetTangentHandles, Setup, ShowDelete
- auto: C / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/CurvePanelPointHandle.cs
- L=45 kind=MonoBehaviour type=CurvePanelPointHandle bases=MonoBehaviour,IDragHandler,IEventSystemHandler anc=MonoBehaviour ns=Expansions.Serenity
- unity(82): UnityEngine.UI=2, EventSystems=2, Transform=1, MonoBehaviour=1, GameObject=1 | ksp: none | fan_in=0
- methods(1): OnDrag
- auto: C / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/DeployedScience/Runtime/DeployedScience.cs
- L=634 kind=ScenarioModule type=DeployedScience bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=Expansions.Serenity.DeployedScience.Runtime
- unity(42): GameObject=14, Instantiate/Destroy=5, Transform=2, Lifecycle=2, GetComponent=2, Coroutine=1 | ksp: GameEvents=24, ConfigNode=23, FlightGlobals=6, Vessel=5, TimeWarp=4 | fan_in=9
- methods(24): DeRegisterCluster, ExpDimReturnsRate, ExpMaxScienceMultiplier, FixedUpdate, GetDiminishingReturnsMultiplier, OnAwake, OnDestroy, OnGameStateLoad, OnLoad, OnPartExplodeGroundCollision, OnSave, OnVesselExplodeGroundCollision, OnVesselRecovered, OnVesselTerminated, OnVesselWillDestroy, RegisterCluster, RemoveMannedScienceObjects, ScanAndProcessSeismicEvent, SetDeployedSciencePartVesselType, SetDiminishingScienceRate, SetupDiminishingReturns, SetupSeismicEnergy, UpdateCluster, UpdateSciencePart
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/DeployedScience/Runtime/DeployedScienceCluster.cs
- L=484 kind=MonoBehaviour type=DeployedScienceCluster bases=MonoBehaviour,IConfigNode anc=MonoBehaviour ns=Expansions.Serenity.DeployedScience.Runtime
- unity(47): GameObject=10, Lifecycle=3, GetComponent=3, Transform=2, Coroutine=2, MonoBehaviour=1 | ksp: GameEvents=8, ConfigNode=8, Planetarium=4, Part=1, FlightGlobals=1 | fan_in=7
- methods(14): Awake, GroundScienceModuleRemoved, GroundSciencePartEnabledStateChanged, Load, OnDestroy, Save, SolarPanelUnitsProduced, Spawn, SpawnandLoad, Start, UpdateCluster, UpdateExperimentDiminishReturns, UpdatePowerState, UpdateScience
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/DeployedScience/Runtime/DeployedScienceExtensions.cs
- L=97 kind=static class type=DeployedScienceExtensions bases=- anc=- ns=Expansions.Serenity.DeployedScience.Runtime
- unity(0): none | ksp: none | fan_in=0
- methods(4): Get, GetExperiment, GetExperimentByPartID, GetExperiments
- auto: A / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/DeployedScience/Runtime/DeployedSciencePart.cs
- L=269 kind=MonoBehaviour type=DeployedSciencePart bases=MonoBehaviour,IConfigNode anc=MonoBehaviour ns=Expansions.Serenity.DeployedScience.Runtime
- unity(67): GameObject=13, Transform=10, Lifecycle=3, GetComponent=3, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: GameEvents=6, ConfigNode=6, Part=2, FlightGlobals=2 | fan_in=9
- methods(10): Awake, Load, OnDestroy, OnGroundSciencePartRemoved, PartIsLoaded, Save, Spawn, SpawnandLoad, Start, UpdateSciencePart
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/IRoboticServo.cs
- L=17 kind=interface type=IRoboticServo bases=- anc=- ns=Expansions.Serenity
- unity(57): Rigidbody=2, GameObject=2 | ksp: AttachNode=2 | fan_in=3
- methods(0): 
- auto: A / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ModuleRoboticRotationServo.cs
- L=756 kind=PartModule type=ModuleRoboticRotationServo bases=BaseServo anc=BaseServo/PartModule/MonoBehaviour ns=Expansions.Serenity
- unity(47): Transform=12, Joint=11, GameObject=4, Time.=1, SetActive=1, Lifecycle=1 | ksp: KSPField=17, HighLogic=10, Localizer=6, GameEvents=4, TimeWarp=3 | fan_in=2
- methods(39): CheckSymmetry, GetFrameDisplacement, GetInfo, GetModuleDisplayName, InitAxisFieldLimits, IsMoving, MaximumAngleAction, MinimumAngleAction, ModifyInverted, ModifyLimits, ModifyLimitsMode, ModifyTargetAngle, ModifyTraverseLimits, OnCopy, OnDestroy, OnEditorPartEvent, OnFixedUpdate, OnJointInit, OnModifyServo, OnPostStartJointInit, OnPreModifyServo, OnResourceNonemptyEmpty, OnSaveShip, OnServoLockApplied ...
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ModuleRoboticServoHinge.cs
- L=742 kind=PartModule type=ModuleRoboticServoHinge bases=BaseServo,IMultipleDragCube anc=BaseServo/PartModule/MonoBehaviour ns=Expansions.Serenity
- unity(47): Transform=11, Joint=11, GameObject=4, Time.=1, SetActive=1, Lifecycle=1 | ksp: KSPField=16, HighLogic=7, Localizer=6, GameEvents=4, TimeWarp=3 | fan_in=2
- methods(42): AssumeDragCubePosition, CheckSymmetry, GetDragCubeNames, GetFrameDisplacement, GetInfo, GetModuleDisplayName, InitAxisFieldLimits, IsMoving, MaximumAngleAction, MinimumAngleAction, ModifyLimits, ModifyTraverseLimits, OnCopy, OnDestroy, OnEditorPartEvent, OnFixedUpdate, OnInventoryModeDisable, OnInventoryModeEnable, OnJointInit, OnModifyServo, OnPostStartJointInit, OnPreModifyServo, OnResourceNonemptyEmpty, OnSaveShip ...
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ModuleRoboticServoPiston.cs
- L=675 kind=PartModule type=ModuleRoboticServoPiston bases=BaseServo,IMultipleDragCube anc=BaseServo/PartModule/MonoBehaviour ns=Expansions.Serenity
- unity(50): Transform=15, Joint=10, Lifecycle=2, GameObject=2, Time.=1, GetComponent=1 | ksp: KSPField=12, Localizer=7, HighLogic=4, TimeWarp=3, KSPAction=3 | fan_in=4
- methods(41): AssumeDragCubePosition, CacheLocalStrings, CalcTargetPosition, ExtendPiston, ExtendPistonAction, GetDragCubeNames, GetFrameDisplacement, GetInfo, GetModuleDisplayName, InitAxisFieldLimits, IsMoving, LateUpdate, ModifyLimits, OnDestroy, OnFixedUpdate, OnJointInit, OnModifyServo, OnPostStartJointInit, OnPreModifyServo, OnResourceNonemptyEmpty, OnSaveShip, OnServoLockApplied, OnServoLockRemoved, OnServoMotorDisengaged ...
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/ModuleRoboticServoRotor.cs
- L=748 kind=PartModule type=ModuleRoboticServoRotor bases=BaseServo anc=BaseServo/PartModule/MonoBehaviour ns=Expansions.Serenity
- unity(53): Joint=15, Collision/Force=8, Transform=7, Time.=5, UnityEngine.UI=1, Lifecycle=1 | ksp: KSPField=14, HighLogic=9, Localizer=6, GameEvents=4, TimeWarp=3 | fan_in=4
- methods(37): BrakeAction, GetFrameDisplacement, GetInfo, GetModuleDisplayName, InitAxisFieldLimits, IsMoving, ModifyDirection, ModifyRPMLimits, MotorDirectionAction, MotorPowerAction, OnDestroy, OnFixedUpdate, OnJointInit, OnModifyServo, OnPostStartJointInit, OnPreModifyServo, OnResourceNonemptyEmpty, OnSaveShip, OnServoLockApplied, OnServoLockRemoved, OnServoMotorDisengaged, OnServoMotorEngaged, OnStart, OnUpdate ...
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RobotArmFX/RobotArmFXDrill.cs
- L=133 kind=ScriptableObject type=RobotArmFXDrill bases=RobotArmScannerFX anc=RobotArmScannerFX/ScriptableObject ns=Expansions.Serenity.RobotArmFX
- unity(52): ParticleSystem=6, Transform=3, GetComponent=2, Raycast/Query=1 | ksp: ConfigNode=2, Part=1 | fan_in=0
- methods(6): Load, OnEffectStart, OnEffectStop, OnStart, OnUpdate, Save
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RobotArmFX/RobotArmFXLaser.cs
- L=113 kind=ScriptableObject type=RobotArmFXLaser bases=RobotArmScannerFX anc=RobotArmScannerFX/ScriptableObject ns=Expansions.Serenity.RobotArmFX
- unity(52): Renderer=2, Raycast/Query=2, Material=2, Transform=1, Shader=1, GetComponent=1 | ksp: ConfigNode=2, Part=1 | fan_in=0
- methods(6): Load, OnEffectStart, OnEffectStop, OnStart, OnUpdate, Save
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RobotArmFX/RobotArmFXSpectrometer.cs
- L=70 kind=ScriptableObject type=RobotArmFXSpectrometer bases=RobotArmScannerFX anc=RobotArmScannerFX/ScriptableObject ns=Expansions.Serenity.RobotArmFX
- unity(41): Light=2, Transform=1, GetComponent=1 | ksp: ConfigNode=2, Part=1 | fan_in=0
- methods(6): Load, OnEffectStart, OnEffectStop, OnStart, OnUpdate, Save
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RobotArmFX/RobotArmScannerFX.cs
- L=60 kind=ScriptableObject type=RobotArmScannerFX bases=ScriptableObject,IConfigNode anc=ScriptableObject ns=Expansions.Serenity.RobotArmFX
- unity(33): ScriptableObject=3 | ksp: Part=4, ConfigNode=2 | fan_in=4
- methods(7): CreateInstanceOfRobotArmScannerFX, Load, OnEffectStart, OnEffectStop, OnStart, OnUpdate, Save
- auto: A / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RoboticCollisions.cs
- L=50 kind=MonoBehaviour type=RoboticCollisions bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Serenity
- unity(86): Collision/Force=6, Collider=2, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1 | ksp: Part=1, HighLogic=1 | fan_in=1
- methods(5): OnCollisionEnter, OnCollisionExit, OnTriggerEnter, OnTriggerExit, Start
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RoboticControllerManager.cs
- L=148 kind=MonoBehaviour type=RoboticControllerManager bases=MonoBehaviour anc=MonoBehaviour ns=Expansions.Serenity
- unity(49): Lifecycle=3, Instantiate/Destroy=3, GameObject=2, MonoBehaviour=1 | ksp: GameEvents=2, FloatCurve=1 | fan_in=6
- methods(10): AnyWindowTextFieldHasFocus, Awake, ClearUpdateQueue, CloseAllWindows, OnDestroy, OnSceneLoad, QueueFieldUpdate, Reset, Update, UpdateFieldValues
- auto: B / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RoboticControllerWindowAction.cs
- L=283 kind=MonoBehaviour type=RoboticControllerWindowAction bases=RoboticControllerWindowBaseRow anc=RoboticControllerWindowBaseRow/MonoBehaviour ns=Expansions.Serenity
- unity(52): Transform=4, UnityEngine.UI=3, SetActive=3, GameObject=3, GetComponent=2, Instantiate/Destroy=1 | ksp: Part=4, Localizer=3, FloatCurve=2 | fan_in=1
- methods(23): HidePoints, InsertPoint, LoadTimesCurve, OnPanelLeftClick, OnPointAdded, OnPointDeleted, OnPointDragging, OnPointMoved, OnPointSelectionChanged, OnRowCollapsed, OnRowDestroy, OnRowExpanded, OnRowStart, RedrawCurve, ReloadCurve, ReverseCurve, SelectAllPoints, SelectPointAtTime, SetLayoutElements, Setup, ShowPoints, Spawn, UpdateUILayout
- auto: C / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RoboticControllerWindowAxis.cs
- L=490 kind=MonoBehaviour type=RoboticControllerWindowAxis bases=RoboticControllerWindowBaseRow anc=RoboticControllerWindowBaseRow/MonoBehaviour ns=Expansions.Serenity
- unity(52): Transform=5, SetActive=5, GameObject=5, UnityEngine.UI=4, GetComponent=4, EventSystems=3 | ksp: Part=5, Localizer=3, HighLogic=2 | fan_in=2
- methods(36): AlignCurveEnds, BeginDragAxisLimit, ClampAllPointValues, ClampPointValues, ClearPresetRefs, EndDragAxisLimit, HidePoints, InsertPoint, InvertCurve, MoveAxisLimits, OnDragAxisLimit, OnLimitsChanged, OnPanelLeftClick, OnPointDragging, OnPointSelectionChanged, OnRowCollapsed, OnRowDestroy, OnRowExpanded, OnRowStart, PresetFlat, PresetRevSaw, PresetSaw, PresetSine, PresetSquare ...
- auto: C / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/RoboticControllerWindowBaseRow.cs
- L=326 kind=MonoBehaviour type=RoboticControllerWindowBaseRow bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=Expansions.Serenity
- unity(63): GameObject=5, EventSystems=5, UnityEngine.UI=4, TMPro=3, SetActive=3, Lifecycle=3 | ksp: Part=6, HighLogic=1 | fan_in=4
- methods(30): AnyTextFieldHasFocus, AssignBaseReferenceVars, Awake, Collapse, EditButtonClicked, Expand, HighlightParts, InputFieldDone, InsertPoint, OnClickRow, OnDestroy, OnPointDragging, OnPointSelectionChanged, OnPointerEnter, OnPointerExit, OnRemoveRowClick, OnRowCollapsed, OnRowDestroy, OnRowExpanded, OnRowStart, RedrawCurve, ReloadCurve, ReverseCurve, SelectAllPoints ...
- auto: C / DLC-BreakingGround

## Assembly-CSharp/Expansions/Serenity/SerenityUtils.cs
- L=17 kind=static class type=SerenityUtils bases=- anc=- ns=Expansions.Serenity
- unity(48): GameObject=2, Texture=1 | ksp: none | fan_in=7
- methods(2): SerenityPrefab, SerenityTexture
- auto: A / DLC-BreakingGround

## Assembly-CSharp/UIPartActionInventory.cs
- L=573 kind=MonoBehaviour type=UIPartActionInventory bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(68): GameObject=18, UnityEngine.UI=13, Transform=8, GetComponent=6, SetActive=5, Instantiate/Destroy=4 | ksp: GameEvents=19, HighLogic=9, Vessel=4, Localizer=3, Part=2 | fan_in=5
- methods(26): Awake, DestroyHeldPart, DestroyItem, GetFieldValue, HideGridObject, InitializeMassVolumeDisplay, InitializeSlots, OnDestroy, OnModuleInventoryChanged, OnModuleInventorySlotChanged, OnVesselChange, OnVesselSituationChange, ResetPlacePartIcons, SetAllSlotsNotSelected, Setup, SetupConstruction, ShowGridObject, SpawnEmptySlot, SpawnLoadedPart, StartPartPlacement, ToggleGridObject, Update, UpdateMassLimits, UpdatePlacePartIcons ...
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionInventorySlot.cs
- L=535 kind=MonoBehaviour type=UIPartActionInventorySlot bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(48): GetComponent=10, EventSystems=8, Transform=3, UnityEngine.UI=2, Resources.=1, MonoBehaviour=1 | ksp: Part=10, HighLogic=9, GameSettings=5, FlightGlobals=5, Vessel=4 | fan_in=4
- methods(23): CanStackInSlot, ClearEmptiedSlot, CreatePartFromThisSlot, CreatePartIconFromThisSlot, EmptyStack, OnCargoModeToggled, OnDestroy, OnEditorPartDeleted, OnPointerClick, OnPointerEnter, OnPointerExit, ProcessClickNoHeldPart, ProcessClickWithHeldPart, RemovePartFromStack, ReturnHeldPartToThisSlot, SetStackedPartsInSlot, Setup, SlotClicked, StackPartsInSlot, StackPartsInThisSlot, StorePartInEmptySlot, UpdateCurrentSelectedSlot, UpdateStackUI
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionItem.cs
- L=72 kind=MonoBehaviour type=UIPartActionItem bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(25): MonoBehaviour=1 | ksp: PartModule=4, Part=4, UIPartActionWindow=3, InputLockManager=2 | fan_in=12
- methods(5): AddInputFieldLock, IsItemValid, RemoveInputfieldLock, SetupItem, UpdateItem
- auto: B / FlightUI

## Assembly-CSharp/UIPartActionLabel.cs
- L=43 kind=MonoBehaviour type=UIPartActionLabel bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(82): UnityEngine.UI=2, TMPro=2, GetComponent=2, GameObject=2, Lifecycle=1 | ksp: none | fan_in=0
- methods(2): Awake, UpdateItem
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionMinMaxRange.cs
- L=177 kind=MonoBehaviour type=UIPartActionMinMaxRange bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(54): TMPro=5, SetActive=2, GameObject=2, Lifecycle=1 | ksp: GameEvents=2, UIPartActionWindow=1, PartModule=1, Part=1, InputLockManager=1 | fan_in=0
- methods(9): GetFieldValue, OnDestroy, OnValueChanged, SetNumericMaxValue, SetNumericMinValue, SetSliderValue, Setup, ToggleNumericSlider, UpdateItem
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionProgressBar.cs
- L=31 kind=MonoBehaviour type=UIPartActionProgressBar bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(69): TMPro=2, UnityEngine.UI=1 | ksp: none | fan_in=0
- methods(1): UpdateItem
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionResource.cs
- L=110 kind=MonoBehaviour type=UIPartActionResource bases=UIPartActionResourceItem anc=UIPartActionResourceItem/UIPartActionItem/MonoBehaviour ns=-
- unity(57): TMPro=3, GameObject=2, UnityEngine.UI=1, SetActive=1, GetComponent=1 | ksp: UIPartActionWindow=2, PartResource=1, Part=1, InputLockManager=1, FlightGlobals=1 | fan_in=2
- methods(5): FlowBtnToggled, IsItemValid, SetButtonState, Setup, UpdateItem
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionResourceDrain.cs
- L=134 kind=MonoBehaviour type=UIPartActionResourceDrain bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(56): GameObject=4, Lifecycle=2, GetComponent=2, UnityEngine.UI=1, Transform=1, Instantiate/Destroy=1 | ksp: Localizer=3, UIPartActionWindow=1, PartResource=1, PartModule=1, Part=1 | fan_in=1
- methods(10): Awake, CacheLocalStrings, GetFieldValue, InitializeResourceFieldsDrainStatus, InitializeResources, OnDestroy, Setup, UpdatePartResourceStatus, UpdateResourcesDrainRate, UpdateResourcesDrainRateAndStatus
- auto: B / FlightUI

## Assembly-CSharp/UIPartActionResourceEditor.cs
- L=176 kind=MonoBehaviour type=UIPartActionResourceEditor bases=UIPartActionResourceItem anc=UIPartActionResourceItem/UIPartActionItem/MonoBehaviour ns=-
- unity(60): TMPro=5, GameObject=4, SetActive=3, UnityEngine.UI=1, Lifecycle=1, GetComponent=1 | ksp: GameEvents=3, UIPartActionWindow=1, PartResource=1, Part=1, InputLockManager=1 | fan_in=2
- methods(10): FlowBtnToggled, IsItemValid, OnDestroy, OnFieldInput, OnSliderChanged, SetButtonState, Setup, ToggleNumericSlider, UpdateItem, onSliderChangeProcess
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionResourceItem.cs
- L=36 kind=MonoBehaviour type=UIPartActionResourceItem bases=UIPartActionItem anc=UIPartActionItem/MonoBehaviour ns=-
- unity(15): none | ksp: PartResource=3, UIPartActionWindow=1, Part=1 | fan_in=3
- methods(3): SetSymCounterpartsAmount, SetSymCounterpartsFlowState, Setup
- auto: B / FlightUI

## Assembly-CSharp/UIPartActionResourcePriority.cs
- L=89 kind=MonoBehaviour type=UIPartActionResourcePriority bases=UIPartActionItem anc=UIPartActionItem/MonoBehaviour ns=-
- unity(68): UnityEngine.UI=3, TMPro=3, Lifecycle=1 | ksp: GameSettings=2, UIPartActionWindow=1, Part=1 | fan_in=2
- methods(6): Awake, OnDecClick, OnIncClick, OnResetClick, Setup, UpdateItem
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionResourceToggle.cs
- L=91 kind=MonoBehaviour type=UIPartActionResourceToggle bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(55): TMPro=2, Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1 | ksp: PartResource=3, Localizer=2 | fan_in=1
- methods(10): Awake, CacheLocalStrings, InitializeItem, OnDestroy, PartStatus, PartStatusToggle, UpdateItemName, UpdateItemNameAndStatus, UpdateStatusText, UpdateStatusTextAndStatus
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionResourceTransfer.cs
- L=244 kind=MonoBehaviour type=UIPartActionResourceTransfer bases=UIPartActionResourceItem anc=UIPartActionResourceItem/UIPartActionItem/MonoBehaviour ns=-
- unity(75): SetActive=19, GameObject=19, UnityEngine.UI=3, Lifecycle=2 | ksp: InputLockManager=3, UIPartActionWindow=2, Planetarium=2, PartResource=1, Part=1 | fan_in=2
- methods(11): Awake, FlowIn, FlowOut, FlowStop, IsItemValid, OnBtnIn, OnBtnOut, OnBtnStop, OnDestroy, Setup, UpdateItem
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionRoboticJointDisplay.cs
- L=155 kind=MonoBehaviour type=UIPartActionRoboticJointDisplay bases=UIPartActionItem anc=UIPartActionItem/MonoBehaviour ns=-
- unity(78): TMPro=8, SetActive=7, GameObject=7, UnityEngine.UI=1, Lifecycle=1 | ksp: Localizer=4, UIPartActionWindow=1, Part=1 | fan_in=2
- methods(5): Awake, DebugText, NumberFormat, Setup, UpdateItem
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionScaleEdit.cs
- L=146 kind=MonoBehaviour type=UIPartActionScaleEdit bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(35): TMPro=2, UnityEngine.UI=1 | ksp: InputLockManager=3, UIPartActionWindow=1, PartModule=1, Part=1 | fan_in=0
- methods(10): FindInterval, GetFieldValue, GetStep, OnTap_dec, OnTap_inc, OnValueChanged, Setup, UpdateDisplay, UpdateInterval, UpdateSlider
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionThermalDisplay.cs
- L=66 kind=MonoBehaviour type=UIPartActionThermalDisplay bases=UIPartActionItem anc=UIPartActionItem/MonoBehaviour ns=-
- unity(85): TMPro=9, Lifecycle=1 | ksp: Localizer=10, UIPartActionWindow=1, Part=1 | fan_in=2
- methods(3): Awake, Setup, UpdateItem
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionToggle.cs
- L=86 kind=MonoBehaviour type=UIPartActionToggle bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(64): TMPro=3, UnityEngine.UI=2, Lifecycle=1, GetComponent=1, GameObject=1 | ksp: UIPartActionWindow=1, PartModule=1, Part=1, InputLockManager=1 | fan_in=0
- methods(6): Awake, GetFieldValue, OnTap, SetButtonState, Setup, UpdateItem
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionVariantButton.cs
- L=81 kind=MonoBehaviour type=UIPartActionVariantButton bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=-
- unity(88): EventSystems=6, UnityEngine.UI=5, SetActive=3, GameObject=3, Shader=2, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(5): OnButtonPressed, Reset, Select, SetColor, Setup
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionVariantSelector.cs
- L=200 kind=MonoBehaviour type=UIPartActionVariantSelector bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(55): UnityEngine.UI=3, Transform=3, Instantiate/Destroy=2, GetComponent=2, GameObject=2, TMPro=1 | ksp: Part=4, Localizer=3, GameEvents=2, UIPartActionWindow=1, PartModule=1 | fan_in=1
- methods(14): AddVariantButton, GetFieldValue, GetSurfaceAttachedPartCount, OnButtonNext, OnButtonPressed, OnButtonPrev, OnDestroy, OnEditorPartEvent, RefreshVariantButtons, ResetButtonOverText, SelectVariant, SetButtonOverText, Setup, UpdateItem
- auto: C / FlightUI

## Assembly-CSharp/UIPartActionVector2.cs
- L=57 kind=MonoBehaviour type=UIPartActionVector2 bases=UIPartActionFieldItem anc=UIPartActionFieldItem/UIPartActionItem/MonoBehaviour ns=-
- unity(49): UnityEngine.UI=1, Lifecycle=1 | ksp: UIPartActionWindow=1, PartModule=1, Part=1, InputLockManager=1 | fan_in=0
- methods(6): Awake, GetFieldValue, OnValueChanged, SetSliderValue, Setup, UpdateItem
- auto: B / FlightUI

## Assembly-CSharp/UISkinDef.cs
- L=52 kind=class type=UISkinDef bases=- anc=- ns=-
- unity(7): none | ksp: none | fan_in=45
- methods(0): 
- auto: C / UI

## Assembly-CSharp/UISkinManager.cs
- L=88 kind=MonoBehaviour type=UISkinManager bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(77): GameObject=10, Lifecycle=3, TMPro=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: none | fan_in=38
- methods(5): Awake, GetPrefab, GetSkin, OnDestroy, Start
- auto: C / UI

## Assembly-CSharp/UIStyle.cs
- L=65 kind=class type=UIStyle bases=- anc=- ns=-
- unity(6): none | ksp: none | fan_in=7
- methods(0): 
- auto: C / UI

## Assembly-CSharp/UIStyleState.cs
- L=10 kind=class type=UIStyleState bases=- anc=- ns=-
- unity(36): Texture=1 | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/UnderwaterFog.cs
- L=270 kind=MonoBehaviour type=UnderwaterFog bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(73): Shader=26, Lifecycle=3, Transform=1, MonoBehaviour=1, Coroutine=1 | ksp: FlightGlobals=10, HighLogic=5, GameEvents=5, Planetarium=4, FlightCamera=4 | fan_in=1
- methods(12): Awake, LateUpdate, OnDestroy, OnPostRender, OnPreRender, OnSceneChange, SetDefaultCB, Start, UpdateCBProperties, UpdateShaderProperties, UpdateShaderPropertiesDefault, VesselSOIChange
- auto: C / Rendering

## Assembly-CSharp/UnitTestingWindow.cs
- L=89 kind=MonoBehaviour type=UnitTestingWindow bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(90): IMGUI=15, Screen/Cursor=2, Lifecycle=2, GameObject=2, UnityEngine.UI=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(6): Awake, DrawState, DrawUnitTestWindow, OnDestroy, OnGUI, OpenWindow
- auto: C / Debug

## Assembly-CSharp/UntrackedObjectClass.cs
- L=23 kind=enum type=UntrackedObjectClass bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=24
- methods(0): 
- auto: A / Celestials

## Assembly-CSharp/UtilMath.cs
- L=548 kind=class type=UtilMath bases=- anc=- ns=-
- unity(0): none | ksp: Vector3d=13 | fan_in=58
- methods(35): ACosh, ACoth, ACsch, ASech, ASinh, ATanh, AngleBetween, Approximately, BSPSolver, Clamp, Clamp01, ClampDegrees180, ClampDegrees360, ClampRadians, ClampRadiansPI, ClampRadiansTwoPI, Coth, Csch, DegreesToRadians, Flatten, InverseLerp, IsDivisible, IsPowerOfTwo, Lerp ...
- auto: A / Core

## Assembly-CSharp/VABCamera.cs
- L=288 kind=MonoBehaviour type=VABCamera bases=MonoBehaviour,IKSPCamera anc=MonoBehaviour ns=-
- unity(76): Transform=27, Time.=7, Input.=4, Lifecycle=3, GameObject=2, Texture=1 | ksp: GameSettings=18, GameEvents=4, InputLockManager=3, EditorLogic=3, ShipConstruct=1 | fan_in=1
- methods(19): GetCameraTransform, GetPivot, OnDestroy, OnEditorRestart, OnEditorShipLoad, OnEnable, OnNavigatorHandoff, OnNavigatorRequestControl, OnNavigatorTakeOver, PlaceCamera, ResetCamera, SetCamCoordsFromPosition, SetInitialCameraPosition, Start, Update, UpdateCamera, getPitch, getReferenceFrame, getYaw
- auto: C / Camera

## Assembly-CSharp/ValueBox.cs
- L=9 kind=class type=ValueBox bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: C / UI

## Assembly-CSharp/ValueModifierQuery.cs
- L=94 kind=class type=ValueModifierQuery bases=- anc=- ns=-
- unity(0): none | ksp: GameEvents=1 | fan_in=3
- methods(6): AddDelta, GetEffectDelta, GetEffectDeltaText, GetEffectPercentageText, GetInput, RunQuery
- auto: A / Career

## Assembly-CSharp/Vect.cs
- L=19 kind=static class type=Vect bases=- anc=- ns=-
- unity(17): none | ksp: none | fan_in=0
- methods(0): 
- auto: A / Core

## Assembly-CSharp/Vector3Curve.cs
- L=56 kind=class type=Vector3Curve bases=- anc=- ns=-
- unity(47): AnimationCurve=6 | ksp: none | fan_in=1
- methods(3): Add, EvaluateColor, EvaluateVector
- auto: A / Core

## Assembly-CSharp/Vector3d.cs
- L=354 kind=struct type=Vector3d bases=- anc=- ns=-
- unity(3): none | ksp: Vector3d=129 | fan_in=184
- methods(29): Angle, AngleBetween, Basis, Cross, Distance, Dot, Equals, Exclude, GetHashCode, Internal_OrthoNormalize2, Internal_OrthoNormalize3, IsZero, Lerp, Magnitude, Max, Min, Normalize, OrthoNormalize, Project, Reflect, RotateTowards, Scale, Slerp, SqrMagnitude ...
- auto: A / Core

## Assembly-CSharp/Vector4Curve.cs
- L=62 kind=class type=Vector4Curve bases=- anc=- ns=-
- unity(50): AnimationCurve=8 | ksp: none | fan_in=1
- methods(3): Add, EvaluateColor, EvaluateVector
- auto: A / Core

## Assembly-CSharp/VectorArrayMaterialProperty.cs
- L=17 kind=class type=VectorArrayMaterialProperty bases=MaterialPropertyExtensions.MaterialProperty anc=MaterialProperty ns=-
- unity(32): Material=1 | ksp: none | fan_in=0
- methods(1): Apply
- auto: C / Rendering

## Assembly-CSharp/VectorExtensions.cs
- L=112 kind=static class type=VectorExtensions bases=- anc=- ns=-
- unity(26): none | ksp: Vector3d=1 | fan_in=1
- methods(8): BearingDegrees, BearingRadians, HeadingDegrees, HeadingRadians, IsInvalid, IsSmallerThan, IsZero, Zero
- auto: A / Core

## Assembly-CSharp/VectorMaterialProperty.cs
- L=17 kind=class type=VectorMaterialProperty bases=MaterialPropertyExtensions.MaterialProperty anc=MaterialProperty ns=-
- unity(46): Shader=1, Material=1 | ksp: none | fan_in=0
- methods(1): Apply
- auto: C / Rendering

## Assembly-CSharp/VehicleAssemblyBuilding.cs
- L=51 kind=MonoBehaviour type=VehicleAssemblyBuilding bases=SpaceCenterBuilding anc=SpaceCenterBuilding/MonoBehaviour ns=-
- unity(19): none | ksp: HighLogic=8, MissionSystem=3, Localizer=3, Game=3, InputLockManager=2 | fan_in=12
- methods(3): IsOpen, OnClicked, OnPopupWarningDismiss
- auto: B / Scene

## Assembly-CSharp/VersionCompareResult.cs
- L=7 kind=enum type=VersionCompareResult bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=17
- methods(0): 
- auto: A / Save

## Assembly-CSharp/Versioning.cs
- L=538 kind=MonoBehaviour type=Versioning bases=VersioningBase anc=VersioningBase/MonoBehaviour ns=-
- unity(58): IMGUI=14, Application.=11, Coroutine=5, WWW/WebRequest=4, UnityEngine.UI=3, Steam=2 | ksp: GameSettings=5, InputLockManager=2 | fan_in=24
- methods(21): BytesToHex, CancelUpdate, CheckForUpdates, FileMD5Bytes, FileMD5String, GetOSCode, GetUpdateChannel, GetVersion, GetVersionFromString, GetVersionStringFull, GetVersionStringWithExperimental, GetVersionStringWithPrerelease, OnAwake, PatcherDownloaded, QuitAndStartPatcher, Start, Update, UpdateGame, UpdatePatcher, checkForUpdate, drawNewVersionWindow
- auto: C / Save

## Assembly-CSharp/VersioningBase.cs
- L=60 kind=MonoBehaviour type=VersioningBase bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(55): Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1, GetComponent=1 | ksp: none | fan_in=8
- methods(5): Awake, GetVersion, GetVersionString, OnAwake, OnDestroy
- auto: B / Save

## Assembly-CSharp/VersionReadout.cs
- L=26 kind=MonoBehaviour type=VersionReadout bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(89): GetComponent=4, TMPro=3, MonoBehaviour=1, Lifecycle=1 | ksp: Localizer=2 | fan_in=0
- methods(1): Start
- auto: C / Misc

## Assembly-CSharp/VesselActuatorMode.cs
- L=6 kind=enum type=VesselActuatorMode bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Vessel

## Assembly-CSharp/VesselAutopilotUI.cs
- L=246 kind=MonoBehaviour type=VesselAutopilotUI bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(65): SetActive=10, GameObject=10, Lifecycle=4, UnityEngine.UI=3, MonoBehaviour=1, GetComponent=1 | ksp: FlightGlobals=12, GameEvents=5, Vessel=1, ProtoCrewMember=1 | fan_in=0
- methods(13): Awake, GetButtonIndex, IsSkillActive, LateUpdate, OnClickButton, OnDestroy, OnKerbalLevelUp, OnVesselChange, SetButtonTrue, SetPilotSkill, Start, ToggleUI, onGameParametersChanged
- auto: C / PartModules-Control

## Assembly-CSharp/VesselCrewManifest.cs
- L=581 kind=class type=VesselCrewManifest bases=- anc=- ns=-
- unity(5): GameObject=1 | ksp: ProtoCrewMember=19, HighLogic=14, KerbalRoster=9, ConfigNode=3, ShipConstruct=2 | fan_in=34
- methods(25): AddCrewMembers, AnyCrewInState, AnyCrewWithTrait, AssignCrewToVessel, CloneOf, Contains, DebugManifest, Filter, FromConfigNode, GetAllCrew, GetCrewInventoryCosts, GetCrewInventoryMass, GetCrewResourceMass, GetCrewableParts, GetPartCrewManifest, GetPartForCrew, HasAnyCrew, MergeInto, RemoveCrewMember, RemovePartManifest, SetCrewMember, SetPartManifest, SetPartManifestNoOverwrite, UpdateCrewForVessel ...
- auto: A / Kerbals

## Assembly-CSharp/VesselEngiUtil.cs
- L=7 kind=static class type=VesselEngiUtil bases=- anc=- ns=-
- unity(0): none | ksp: Vessel=1 | fan_in=0
- methods(1): GetDeltaV
- auto: A / Vessel

## Assembly-CSharp/VesselLabel.cs
- L=21 kind=MonoBehaviour type=VesselLabel bases=BaseLabel anc=BaseLabel/MonoBehaviour ns=-
- unity(59): EventSystems=2 | ksp: Vessel=2 | fan_in=1
- methods(2): OnClick, Setup
- auto: B / MapView

## Assembly-CSharp/VesselLabels.cs
- L=505 kind=MonoBehaviour type=VesselLabels bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(51): Transform=10, Instantiate/Destroy=7, GameObject=6, Lifecycle=3, UnityEngine.UI=2, Texture=1 | ksp: Vessel=14, GameSettings=7, FlightGlobals=7, GameEvents=6, Localizer=5 | fan_in=4
- methods(20): AddNodeLabel, Awake, CreateNodeLabel, CreateVesselLabel, DestroyAllLabels, DestroyNodeLabels, DestroyVesselLabel, DisableAllLabels, EnableAllLabels, GetCurrentTarget, GetLabel, GetLabelType, LateUpdate, OnDestroy, OnDoubleClickLabel, OnSceneLoad, OnVesselCreate, OnVesselDestroy, ProcessLabel, RemoveNodeLabel
- auto: C / MapView

## Assembly-CSharp/VesselModule.cs
- L=134 kind=VesselModule type=VesselModule bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(36): Lifecycle=2, MonoBehaviour=1, GetComponent=1 | ksp: Vessel=4, ConfigNode=4, HighLogic=2, VesselModule=1 | fan_in=9
- methods(15): Awake, GetActivation, GetOrder, Load, OnAwake, OnGoOffRails, OnGoOnRails, OnLoad, OnLoadVessel, OnSave, OnStart, OnUnloadVessel, Save, ShouldBeActive, Start
- auto: A / Vessel

## Assembly-CSharp/VesselModuleManager.cs
- L=198 kind=static class type=VesselModuleManager bases=- anc=- ns=-
- unity(44): GameObject=11, GetComponent=4, Instantiate/Destroy=3 | ksp: VesselModule=10, Vessel=3 | fan_in=2
- methods(7): AddModulesToVessel, CompileModules, GetModules, GetWrapper, RemoveModuleOfType, RemoveModulesFromVessel, SetWrapperActive
- auto: B / Vessel

## Assembly-CSharp/VesselNaming.cs
- L=64 kind=class type=VesselNaming bases=IConfigNode anc=IConfigNode ns=-
- unity(0): none | ksp: Part=4, ConfigNode=3, Vessel=1, ShipConstruct=1 | fan_in=6
- methods(3): FindPriorityNamePart, Load, Save
- auto: A / Vessel

## Assembly-CSharp/VesselPrecalculate.cs
- L=525 kind=MonoBehaviour type=VesselPrecalculate bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(48): Rigidbody=13, Lifecycle=4, Transform=3, Physics.=1, MonoBehaviour=1, GetComponent=1 | ksp: Vector3d=31, PhysicsGlobals=8, OrbitDriver=6, GameEvents=5, Vessel=4 | fan_in=2
- methods(16): ApplyVelocityCorrection, Awake, CacheLocalStrings, CalculateGravity, CalculatePhysicsStats, FixedUpdate, GoOffRails, GoOnRails, MainPhysics, OnDestroy, RunFirst, SetLandedPosRot, StartEasing, StopEasing, Update, onDockingComplete
- auto: B / Vessel

## Assembly-CSharp/VesselRanges.cs
- L=159 kind=class type=VesselRanges bases=IConfigNode anc=IConfigNode ns=-
- unity(5): none | ksp: Vessel=7, ConfigNode=6 | fan_in=4
- methods(3): GetSituationRanges, Load, Save
- auto: A / Vessel

## Assembly-CSharp/VesselRecovery.cs
- L=181 kind=ScenarioModule type=VesselRecovery bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=-
- unity(29): Lifecycle=2 | ksp: Reputation=7, ResearchAndDevelopment=6, GameEvents=6, Funding=5, ProtoVessel=3 | fan_in=6
- methods(6): OnAwake, OnDestroy, OnVesselRecovered, Start, checkLaunchSites, recoverVesselCrew
- auto: A / Career

## Assembly-CSharp/VesselRetrieval.cs
- L=93 kind=MonoBehaviour type=VesselRetrieval bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(61): SceneManager=3, Lifecycle=2, Coroutine=2, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1 | ksp: Vessel=7, HighLogic=4, GameEvents=3, FlightGlobals=2 | fan_in=0
- methods(8): Awake, OnDestroy, OnLevelLoaded, OnSceneLoaded, getVesselsToRecover, onVesselRecoveryRequested, recoverVessel, recoverVessels
- auto: B / Vessel

## Assembly-CSharp/VesselSpawnSearch.cs
- L=223 kind=MonoBehaviour type=VesselSpawnSearch bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(81): UnityEngine.UI=8, GetComponent=7, GameObject=6, Transform=4, Lifecycle=4, SetActive=3 | ksp: InputLockManager=2 | fan_in=1
- methods(15): AdjustSearchInputSize, Awake, HasFiltered, ItemMatchesSearch, OnDestroy, OnDisable, SearchField_OnClick, SearchField_OnEndEdit, SearchField_OnValueChange, SearchRestart, SearchRoutine, SearchStart, SearchStop, SteamCraftListReceived, Update
- auto: C / Vessel

## Assembly-CSharp/VesselSwitching.cs
- L=130 kind=MonoBehaviour type=VesselSwitching bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(40): Lifecycle=3, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: FlightGlobals=9, HighLogic=5, Vessel=4, GameSettings=4, Localizer=3 | fan_in=0
- methods(3): Awake, OnDestroy, Update
- auto: B / Vessel

## Assembly-CSharp/VesselTargetModes.cs
- L=7 kind=enum type=VesselTargetModes bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=9
- methods(0): 
- auto: A / Orbit

## Assembly-CSharp/VesselTripLog.cs
- L=47 kind=class type=VesselTripLog bases=- anc=- ns=-
- unity(0): none | ksp: Vessel=1, ProtoVessel=1, ProtoPartSnapshot=1, ProtoPartModuleSnapshot=1, Part=1 | fan_in=5
- methods(2): FromProtoVessel, FromVessel
- auto: A / Vessel

## Assembly-CSharp/VesselTutorialStep.cs
- L=194 kind=class type=VesselTutorialStep bases=IntermediateTutorialPageStep anc=IntermediateTutorialPageStep/TutorialPageConfig ns=-
- unity(33): GameObject=12 | ksp: GameEvents=2 | fan_in=1
- methods(17): AddTutorialStepConfig, CanHighlightVesselLanded, FilterVesselLandedNodeCriteria, IsDialogLinked, IsPlanetSetted, OnEnterDragAndLinkVesselNode, OnEnterSelectKerbinPlanet, OnEnterSelectKerbinPlanetHelper, OnLeaveDragAndLinkVesselNode, OnLeaveSelectKerbinPlanet, OnLeaveSelectKerbinPlanetHelper, OnNodeFocused, OnSelectedGameObjectChange, OnSelectedGameObjectChangeSetPlanet, OnUpdateLinkVesselNode, OnUpdateSelectKerbinPlanet, OnUpdateSelectKerbinPlanetHelper
- auto: B / Tutorials

## Assembly-CSharp/VesselType.cs
- L=39 kind=enum type=VesselType bases=- anc=- ns=-
- unity(2): none | ksp: none | fan_in=70
- methods(0): 
- auto: A / Vessel

## Assembly-CSharp/VesselValues.cs
- L=144 kind=class type=VesselValues bases=- anc=- ns=-
- unity(5): Time.=2 | ksp: Part=19, Vessel=5 | fan_in=11
- methods(1): ResetValueCache
- auto: A / Vessel

## Assembly-CSharp/VFXSequencer.cs
- L=146 kind=MonoBehaviour type=VFXSequencer bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(73): ParticleSystem=6, Coroutine=6, Time.=3, Audio=2, MonoBehaviour=1, Lifecycle=1 | ksp: GameSettings=1 | fan_in=1
- methods(5): GetFXDuration, Play, Start, getSubFXHierarchyDuration, playSequence
- auto: C / Rendering

## Assembly-CSharp/VideoSettings.cs
- L=642 kind=class type=VideoSettings bases=DialogGUIVerticalLayout,ISettings anc=DialogGUIVerticalLayout/DialogGUILayoutBase/DialogGUIBase ns=-
- unity(14): QualitySettings/SystemInfo=10, Screen/Cursor=2, Application.=2, Texture=1, SetActive=1 | ksp: GameSettings=65, Localizer=56, HighLogic=1 | fan_in=1
- methods(7): ApplySettings, DrawMiniSettings, DrawSettings, GetAvailableResolutions, GetName, GetSettings, OnUpdate
- auto: C / UI

## Assembly-CSharp/VolumeController.cs
- L=10 kind=MonoBehaviour type=VolumeController bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(81): GetComponent=2, Audio=2, MonoBehaviour=1, Lifecycle=1 | ksp: GameSettings=1 | fan_in=0
- methods(1): Start
- auto: C / Audio

## Assembly-CSharp/VolumeNormalizer.cs
- L=49 kind=MonoBehaviour type=VolumeNormalizer bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(81): Audio=5, Time.=2, Lifecycle=2, MonoBehaviour=1 | ksp: GameSettings=6 | fan_in=0
- methods(2): Awake, Update
- auto: C / Audio

## Assembly-CSharp/VolumetricObjectBase.cs
- L=237 kind=MonoBehaviour type=VolumetricObjectBase bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(81): Mesh=12, Material=5, Lifecycle=4, Instantiate/Destroy=4, Camera=4, Renderer=3 | ksp: none | fan_in=4
- methods(14): CleanUp, CopyMesh, CreateCube, HasChanged, LateUpdate, OnDestroy, OnDisable, OnEnable, PopulateShaderName, ScaleMesh, ScaleVector, SetChangedValues, SetupUnitVerts, UpdateVolume
- auto: C / Rendering

## Assembly-CSharp/WarpTransition.cs
- L=23 kind=class type=WarpTransition bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Orbit

## Assembly-CSharp/WarpTransitionCalculator.cs
- L=123 kind=static class type=WarpTransitionCalculator bases=- anc=- ns=-
- unity(0): none | ksp: TimeWarp=13 | fan_in=2
- methods(4): CalcWarpRateTransitions, CheckForTransitionChanges, SafeRateToUTPeriod, UTToRateDown
- auto: A / Orbit

## Assembly-CSharp/WeldFX.cs
- L=252 kind=MonoBehaviour type=WeldFX bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(82): Transform=30, Coroutine=12, ParticleSystem=4, Lifecycle=3, Time.=2, Renderer=2 | ksp: GameEvents=2, KerbalEVA=1, GameSettings=1 | fan_in=2
- methods(16): AdjustWeldingTransform, Awake, DisableMesh, EnableMesh, FadeInLight, FadeOutLight, GetAttachPoint, Play, PlaySequence, Reset, SetSparksGravity, Start, Stop, ToggleMesh, Update, UpdateGunAiming
- auto: C / Rendering

## Assembly-CSharp/WhackAKerbal.cs
- L=108 kind=MonoBehaviour type=WhackAKerbal bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(100): IMGUI=19, GameObject=14, GetComponent=6, Input.=5, UnityEngine.UI=3, Transform=3 | ksp: InputLockManager=3 | fan_in=1
- methods(5): OnDestroy, OnGUI, Start, Update, drawProjectileOptions
- auto: B / Scene

## Assembly-CSharp/WhatsNewDialog.cs
- L=254 kind=MonoBehaviour type=WhatsNewDialog bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(73): UnityEngine.UI=6, GameObject=5, TMPro=3, SetActive=3, Lifecycle=3, KeyCode=3 | ksp: Localizer=7, GameSettings=7, InputLockManager=2 | fan_in=1
- methods(11): Create, Dismiss, GetChangeLogText, OnCloseButton, OnDestroy, OnDontShowAgainToggle, OnMerchButton, OnMoreInfoButton, ParseReadMeFile, Start, Update
- auto: C / UI

## Assembly-CSharp/WhatsNewModes.cs
- L=5 kind=enum type=WhatsNewModes bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/Wheel.cs
- L=159 kind=class type=Wheel bases=IConfigNode anc=Block ns=-
- unity(59): Transform=6, SetActive=6, GameObject=6, WheelCollider=2, Raycast/Query=2, Physics.=1 | ksp: ConfigNode=2, TimeWarp=1 | fan_in=0
- methods(5): Load, Save, Update, damageWheel, repairWheel
- auto: B / PartModules-Wheels

## Assembly-CSharp/WheelType.cs
- L=6 kind=enum type=WheelType bases=- anc=- ns=-
- unity(0): none | ksp: none | fan_in=5
- methods(0): 
- auto: A / PartModules-Wheels

## Assembly-CSharp/WindmillAnimScript.cs
- L=50 kind=MonoBehaviour type=WindmillAnimScript bases=MonoBehaviour anc=MonoBehaviour ns=-
- unity(61): Lifecycle=3, Transform=1, MonoBehaviour=1 | ksp: GameEvents=4 | fan_in=0
- methods(5): OnDestroy, Start, Update, onPause, onUnPause
- auto: B / Misc

## Assembly-CSharp/XButton.cs
- L=118 kind=MonoBehaviour type=XButton bases=Button anc=Button ns=-
- unity(77): EventSystems=12, GameObject=2, UnityEngine.UI=1, Lifecycle=1, Instantiate/Destroy=1, GetComponent=1 | ksp: none | fan_in=0
- methods(10): Awake, Create, OnDeselect, OnMove, OnPointerDown, OnPointerEnter, OnPointerExit, OnPointerUp, OnSelect, Terminate
- auto: C / UI

## Assembly-CSharp/XGraphic.cs
- L=12 kind=UIBehaviour type=XGraphic bases=Graphic anc=Graphic ns=-
- unity(44): UnityEngine.UI=1 | ksp: none | fan_in=1
- methods(1): OnFillVBO
- auto: C / UI

## Assembly-CSharp/XHoverable.cs
- L=40 kind=UIBehaviour type=XHoverable bases=UIBehaviour,IEventSystemHandler,IPointerEnterHandler anc=UIBehaviour ns=-
- unity(85): EventSystems=7, Lifecycle=1 | ksp: none | fan_in=1
- methods(4): Awake, Clear, OnPointerEnter, OnPointerExit
- auto: C / UI

## Assembly-CSharp/XSelectable.cs
- L=104 kind=UIBehaviour type=XSelectable bases=UIBehaviour,IEventSystemHandler,IPointerEnterHandler anc=UIBehaviour ns=-
- unity(84): EventSystems=19, Lifecycle=1 | ksp: none | fan_in=10
- methods(9): Awake, Clear, OnDeselect, OnMove, OnPointerDown, OnPointerEnter, OnPointerExit, OnPointerUp, OnSelect
- auto: C / UI
