# Review chunk 08 (ExpA): 243 files, 30571 lines

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterActiveRadiatorBase.cs
- 31 lines; kind=class; type=AdjusterActiveRadiatorBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=3
- methods(3): ApplyMaxEnergyTransferAdjustment, GetTargetPartModule, IsBlockingCooling
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterActiveRadiatorMaxEnergyTransfer.cs
- 60 lines; kind=class; type=AdjusterActiveRadiatorMaxEnergyTransfer; bases=AdjusterActiveRadiatorBase; ancestry=AdjusterActiveRadiatorBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Localizer=4, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Activate, ApplyMaxEnergyTransferAdjustment, GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterAlternatorBase.cs
- 26 lines; kind=class; type=AdjusterAlternatorBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): ApplyOutputAdjustment, GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterAlternatorOutput.cs
- 60 lines; kind=class; type=AdjusterAlternatorOutput; bases=AdjusterAlternatorBase; ancestry=AdjusterAlternatorBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Localizer=3, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Activate, ApplyOutputAdjustment, GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterAnchoredDecouplerBase.cs
- 26 lines; kind=class; type=AdjusterAnchoredDecouplerBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=3
- methods(2): GetTargetPartModule, IsBlockingDecouple
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterCargoBayBase.cs
- 26 lines; kind=class; type=AdjusterCargoBayBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): GetTargetPartModule, IsCargoBayStuck
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterControlSurfaceActuatorSpeed.cs
- 60 lines; kind=class; type=AdjusterControlSurfaceActuatorSpeed; bases=AdjusterControlSurfaceBase; ancestry=AdjusterControlSurfaceBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Localizer=2, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Activate, ApplyActuatorSpeedAdjustment, GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterControlSurfaceBase.cs
- 26 lines; kind=class; type=AdjusterControlSurfaceBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=3
- methods(2): ApplyActuatorSpeedAdjustment, GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterDataTransmitterBase.cs
- 31 lines; kind=class; type=AdjusterDataTransmitterBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=3
- methods(3): ApplyPowerAdjustment, GetTargetPartModule, IsDataTransmitterBroken
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterDataTransmitterPower.cs
- 75 lines; kind=class; type=AdjusterDataTransmitterPower; bases=AdjusterDataTransmitterBase; ancestry=AdjusterDataTransmitterBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=1
- ksp refs: Localizer=3, ConfigNode=2; fan_in=0 fan_out=8
- methods(7): Activate, ApplyPowerAdjustment, GetNodeBodyParameterString, Load, PowerValueChanged, Save, UpdatePowerValueDisplay
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterDecoupleBase.cs
- 26 lines; kind=class; type=AdjusterDecoupleBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): GetTargetPartModule, IsBlockingDecouple
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterDeployablePartBase.cs
- 26 lines; kind=class; type=AdjusterDeployablePartBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): GetTargetPartModule, IsDeployablePartStuck
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterDeployableSolarPanelBase.cs
- 26 lines; kind=class; type=AdjusterDeployableSolarPanelBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): ApplyEfficiencyAdjustment, GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterDeployableSolarPanelEfficiency.cs
- 60 lines; kind=class; type=AdjusterDeployableSolarPanelEfficiency; bases=AdjusterDeployableSolarPanelBase; ancestry=AdjusterDeployableSolarPanelBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Localizer=2, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Activate, ApplyEfficiencyAdjustment, GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterDockingNodeBase.cs
- 26 lines; kind=class; type=AdjusterDockingNodeBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): GetTargetPartModule, IsBlockingUndock
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterEnginesBase.cs
- 31 lines; kind=class; type=AdjusterEnginesBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=3
- methods(3): ApplyThrottleAdjustment, GetTargetPartModule, IsEngineDead
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterEnginesThrottle.cs
- 60 lines; kind=class; type=AdjusterEnginesThrottle; bases=AdjusterEnginesBase; ancestry=AdjusterEnginesBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Localizer=2, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Activate, ApplyThrottleAdjustment, GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterFuelLineBase.cs
- 22 lines; kind=class; type=AdjusterFuelLineBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: CompoundParts
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterGeneratorBase.cs
- 26 lines; kind=class; type=AdjusterGeneratorBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): ApplyEfficiencyAdjustment, GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterGeneratorEfficiency.cs
- 60 lines; kind=class; type=AdjusterGeneratorEfficiency; bases=AdjusterGeneratorBase; ancestry=AdjusterGeneratorBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Localizer=2, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Activate, ApplyEfficiencyAdjustment, GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterGimbalBase.cs
- 27 lines; kind=class; type=AdjusterGimbalBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): ApplyControlAdjustment, GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterGimbalLimitControl.cs
- 91 lines; kind=class; type=AdjusterGimbalLimitControl; bases=AdjusterGimbalBase; ancestry=AdjusterGimbalBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Localizer=4, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Activate, ApplyControlAdjustment, GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterGPSBase.cs
- 21 lines; kind=class; type=AdjusterGPSBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterGrappleNodeBase.cs
- 31 lines; kind=class; type=AdjusterGrappleNodeBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=3
- methods(3): GetTargetPartModule, IsBlockingGrappleGrab, IsBlockingGrappleRelease
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterKerbNetAccessBase.cs
- 21 lines; kind=class; type=AdjusterKerbNetAccessBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterKerbNetAccessDisabled.cs
- 38 lines; kind=class; type=AdjusterKerbNetAccessDisabled; bases=AdjusterKerbNetAccessBase; ancestry=AdjusterKerbNetAccessBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization, KSP.UI.Dialogs
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=0 fan_out=5
- methods(1): Activate
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterLiftingSurfaceBase.cs
- 27 lines; kind=class; type=AdjusterLiftingSurfaceBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): ApplyLiftForceAdjustment, GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterLiftingSurfaceLiftForce.cs
- 60 lines; kind=class; type=AdjusterLiftingSurfaceLiftForce; bases=AdjusterLiftingSurfaceBase; ancestry=AdjusterLiftingSurfaceBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=5
- ksp refs: Localizer=2, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Activate, ApplyLiftForceAdjustment, GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterLightBase.cs
- 26 lines; kind=class; type=AdjusterLightBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=3
- methods(2): ApplyIntensityAdjustment, GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterLightIntensity.cs
- 60 lines; kind=class; type=AdjusterLightIntensity; bases=AdjusterLightBase; ancestry=AdjusterLightBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Localizer=2, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Activate, ApplyIntensityAdjustment, GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterPartModuleBase.cs
- 279 lines; kind=class; type=AdjusterPartModuleBase; bases=DynamicModule,IConfigNode,IPartModuleAdjuster; ancestry=DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=8
- ksp refs: ConfigNode=4, PartModule=2, Localizer=2, KSPField=1, KSPEvent=1, FlightGlobals=1; fan_in=59 fan_out=18
- methods(17): Activate, CleanUp, CreateInstanceOfModuleAdjuster, CreateModuleAdjusterList, Deactivate, GetDisplayName, GetNodeBodyParameterString, GetPartModuleTargetList, GetPartModulesThatCanBeAffected, GetTargetPartModule, Load, ModularSetup, OnPartModuleAdjusterListModified, RemoveAdjuster, Save, Setup, UpdateStatusMessage
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterProbeControlPointBase.cs
- 27 lines; kind=class; type=AdjusterProbeControlPointBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: CommNet
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): GetTargetPartModule, IsProbeControlPointBroken
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterRCSBase.cs
- 37 lines; kind=class; type=AdjusterRCSBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: UnityEngine
- unity: none; unity_score=10
- ksp refs: none; fan_in=3 fan_out=3
- methods(4): ApplyInputLinearAdjustment, ApplyInputRotationAdjustment, GetTargetPartModule, IsRCSBroken
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterReactionWheelBase.cs
- 32 lines; kind=class; type=AdjusterReactionWheelBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: none; fan_in=4 fan_out=3
- methods(3): ApplyTorqueAdjustment, GetTargetPartModule, IsReactionWheelBroken
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterReactionWheelTorque.cs
- 91 lines; kind=class; type=AdjusterReactionWheelTorque; bases=AdjusterReactionWheelBase; ancestry=AdjusterReactionWheelBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Localizer=5, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Activate, ApplyTorqueAdjustment, GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterResourceIntakeBase.cs
- 21 lines; kind=class; type=AdjusterResourceIntakeBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterSASBase.cs
- 26 lines; kind=class; type=AdjusterSASBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): ApplySASServiceLevelAdjustment, GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterSASServiceLevel.cs
- 54 lines; kind=class; type=AdjusterSASServiceLevel; bases=AdjusterSASBase; ancestry=AdjusterSASBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=2, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Activate, ApplySASServiceLevelAdjustment, GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterScienceExperimentBase.cs
- 21 lines; kind=class; type=AdjusterScienceExperimentBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterToggleCrossfeedBase.cs
- 21 lines; kind=class; type=AdjusterToggleCrossfeedBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterWheelBrakesBase.cs
- 27 lines; kind=class; type=AdjusterWheelBrakesBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: ModuleWheels
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): ApplyTorqueAdjustment, GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterWheelBrakesTorque.cs
- 60 lines; kind=class; type=AdjusterWheelBrakesTorque; bases=AdjusterWheelBrakesBase; ancestry=AdjusterWheelBrakesBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Localizer=2, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Activate, ApplyTorqueAdjustment, GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterWheelDeploymentBase.cs
- 22 lines; kind=class; type=AdjusterWheelDeploymentBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: ModuleWheels
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterWheelMotorBase.cs
- 27 lines; kind=class; type=AdjusterWheelMotorBase; bases=AdjusterPartModuleBase; ancestry=AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: ModuleWheels
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): ApplyTorqueAdjustment, GetTargetPartModule
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/AdjusterWheelMotorTorque.cs
- 60 lines; kind=class; type=AdjusterWheelMotorTorque; bases=AdjusterWheelMotorBase; ancestry=AdjusterWheelMotorBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Localizer=2, ConfigNode=2; fan_in=0 fan_out=6
- methods(5): Activate, ApplyTorqueAdjustment, GetNodeBodyParameterString, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureActiveRadiatorStuck.cs
- 79 lines; kind=class; type=FailureActiveRadiatorStuck; bases=AdjusterActiveRadiatorBase; ancestry=AdjusterActiveRadiatorBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: ConfigNode=2, Localizer=1; fan_in=0 fan_out=7
- methods(4): Activate, IsBlockingCooling, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureAnchoredDecouplerCantDecouple.cs
- 37 lines; kind=class; type=FailureAnchoredDecouplerCantDecouple; bases=AdjusterAnchoredDecouplerBase; ancestry=AdjusterAnchoredDecouplerBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=3; fan_in=0 fan_out=3
- methods(2): Activate, IsBlockingDecouple
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureCargoBayStuck.cs
- 72 lines; kind=class; type=FailureCargoBayStuck; bases=AdjusterCargoBayBase; ancestry=AdjusterCargoBayBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: ConfigNode=2, Localizer=1; fan_in=0 fan_out=6
- methods(5): Activate, Deactivate, IsCargoBayStuck, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureControlSurfaceDisableControl.cs
- 37 lines; kind=class; type=FailureControlSurfaceDisableControl; bases=AdjusterControlSurfaceBase; ancestry=AdjusterControlSurfaceBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=3; fan_in=0 fan_out=3
- methods(2): Activate, ApplyActuatorSpeedAdjustment
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureCrossfeedStuck.cs
- 64 lines; kind=class; type=FailureCrossfeedStuck; bases=AdjusterToggleCrossfeedBase; ancestry=AdjusterToggleCrossfeedBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: ConfigNode=2, Localizer=1; fan_in=0 fan_out=7
- methods(3): Activate, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureDataTransmitterBroken.cs
- 42 lines; kind=class; type=FailureDataTransmitterBroken; bases=AdjusterDataTransmitterBase; ancestry=AdjusterDataTransmitterBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=0 fan_out=4
- methods(2): Activate, IsDataTransmitterBroken
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureDecoupleCantDecouple.cs
- 37 lines; kind=class; type=FailureDecoupleCantDecouple; bases=AdjusterDecoupleBase; ancestry=AdjusterDecoupleBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=0 fan_out=3
- methods(2): Activate, IsBlockingDecouple
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureDeployablePartStuck.cs
- 69 lines; kind=class; type=FailureDeployablePartStuck; bases=AdjusterDeployablePartBase; ancestry=AdjusterDeployablePartBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: ConfigNode=2, Localizer=1; fan_in=0 fan_out=7
- methods(4): Activate, IsDeployablePartStuck, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureDockingNodeCantUndock.cs
- 31 lines; kind=class; type=FailureDockingNodeCantUndock; bases=AdjusterDockingNodeBase; ancestry=AdjusterDockingNodeBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=0 fan_out=3
- methods(2): Activate, IsBlockingUndock
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureEnginesDeadEngine.cs
- 45 lines; kind=class; type=FailureEnginesDeadEngine; bases=AdjusterEnginesBase; ancestry=AdjusterEnginesBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=0 fan_out=4
- methods(2): Activate, IsEngineDead
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureFuelLineBlocked.cs
- 48 lines; kind=class; type=FailureFuelLineBlocked; bases=AdjusterFuelLineBase; ancestry=AdjusterFuelLineBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: CompoundParts, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=0 fan_out=4
- methods(2): Activate, Deactivate
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureGPSNoDisplay.cs
- 32 lines; kind=class; type=FailureGPSNoDisplay; bases=AdjusterGPSBase; ancestry=AdjusterGPSBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=3; fan_in=0 fan_out=3
- methods(1): Activate
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureGrappleNodeCantGrapple.cs
- 31 lines; kind=class; type=FailureGrappleNodeCantGrapple; bases=AdjusterGrappleNodeBase; ancestry=AdjusterGrappleNodeBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=0 fan_out=3
- methods(2): Activate, IsBlockingGrappleGrab
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureGrappleNodeCantRelease.cs
- 46 lines; kind=class; type=FailureGrappleNodeCantRelease; bases=AdjusterGrappleNodeBase; ancestry=AdjusterGrappleNodeBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=3; fan_in=0 fan_out=4
- methods(3): Activate, Deactivate, IsBlockingGrappleRelease
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureLightFlickering.cs
- 89 lines; kind=class; type=FailureLightFlickering; bases=AdjusterLightBase; ancestry=AdjusterLightBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, UnityEngine
- unity: Time.=1; unity_score=8
- ksp refs: Localizer=4, ConfigNode=2; fan_in=0 fan_out=8
- methods(6): Activate, ApplyIntensityAdjustment, GetNodeBodyParameterString, Load, Save, UpdateFlickeringData
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureLightStuck.cs
- 64 lines; kind=class; type=FailureLightStuck; bases=AdjusterLightBase; ancestry=AdjusterLightBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: ConfigNode=2, Localizer=1; fan_in=0 fan_out=7
- methods(3): Activate, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureProbeControlPointBroken.cs
- 29 lines; kind=class; type=FailureProbeControlPointBroken; bases=AdjusterProbeControlPointBase; ancestry=AdjusterProbeControlPointBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=2
- methods(1): IsProbeControlPointBroken
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureRCSFireRandomly.cs
- 43 lines; kind=class; type=FailureRCSFireRandomly; bases=AdjusterRCSBase; ancestry=AdjusterRCSBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=17
- ksp refs: Localizer=1; fan_in=0 fan_out=3
- methods(3): Activate, ApplyInputLinearAdjustment, ApplyInputRotationAdjustment
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureRCSStopWorking.cs
- 37 lines; kind=class; type=FailureRCSStopWorking; bases=AdjusterRCSBase; ancestry=AdjusterRCSBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=0 fan_out=3
- methods(2): Activate, IsRCSBroken
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureReactionWheelBroken.cs
- 37 lines; kind=class; type=FailureReactionWheelBroken; bases=AdjusterReactionWheelBase; ancestry=AdjusterReactionWheelBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=0 fan_out=3
- methods(2): Activate, IsReactionWheelBroken
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureReactionWheelStuck.cs
- 63 lines; kind=class; type=FailureReactionWheelStuck; bases=AdjusterReactionWheelBase; ancestry=AdjusterReactionWheelBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=6
- ksp refs: ConfigNode=2, Localizer=1; fan_in=0 fan_out=6
- methods(4): Activate, ApplyTorqueAdjustment, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureResourceIntakeStuck.cs
- 64 lines; kind=class; type=FailureResourceIntakeStuck; bases=AdjusterResourceIntakeBase; ancestry=AdjusterResourceIntakeBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization
- unity: none; unity_score=0
- ksp refs: ConfigNode=2, Localizer=1; fan_in=0 fan_out=7
- methods(3): Activate, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureScienceExperimentDisable.cs
- 32 lines; kind=class; type=FailureScienceExperimentDisable; bases=AdjusterScienceExperimentBase; ancestry=AdjusterScienceExperimentBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=0 fan_out=3
- methods(1): Activate
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/FailureWheelDeploymentStuck.cs
- 65 lines; kind=class; type=FailureWheelDeploymentStuck; bases=AdjusterWheelDeploymentBase; ancestry=AdjusterWheelDeploymentBase/AdjusterPartModuleBase/DynamicModule/IConfigNode; ns=Expansions.Missions.Adjusters
- usings: Expansions.Missions.Editor, KSP.Localization, ModuleWheels
- unity: none; unity_score=0
- ksp refs: ConfigNode=2, Localizer=1; fan_in=0 fan_out=7
- methods(3): Activate, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Adjusters/IPartModuleAdjuster.cs
- 6 lines; kind=interface; type=IPartModuleAdjuster; bases=-; ancestry=-; ns=Expansions.Missions.Adjusters
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/ActionPaneDisplay.cs
- 349 lines; kind=UIBehaviour; type=ActionPaneDisplay; bases=Selectable,IPointerClickHandler,IEventSystemHandler,IDragHandler,IBeginDragHandler,IEndDragHandler; ancestry=Selectable; ns=Expansions.Missions.Editor
- usings: KSP.UI, KSP.UI.TooltipTypes, UnityEngine, UnityEngine.EventSystems, UnityEngine.Events, UnityEngine.UI
- unity: UnityEngine.UI=22, EventSystems=16, Raycast/Query=11, Texture=6, GetComponent=6, MonoBehaviour=5; unity_score=89
- ksp refs: none; fan_in=6 fan_out=5
- methods(28): AddObject, AddToolbarButton, AddToolbarToggle, Awake, Clean, ClearToolbar, ClearToolbarEvents, Destroy, GetMousePointOnCamera, GetToolbarButton, OnBeginDrag, OnDeselect, OnDisplayClick, OnDisplayClickUp, OnDisplayDrag, OnDisplayDragEnd, OnDrag, OnEndDrag, OnMouseOver, OnPointerClick, OnPointerEnter, OnPointerExit, OnPointerUp, OnSelect, Raycast, Setup, Update, UpdateDisplayArea
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/ActionPaneDisplay_BiSelector.cs
- 22 lines; kind=UIBehaviour; type=ActionPaneDisplay_BiSelector; bases=ActionPaneDisplay; ancestry=ActionPaneDisplay/Selectable; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: Transform=3, UnityEngine.UI=2, GetComponent=2, GameObject=2, Lifecycle=1, Instantiate/Destroy=1; unity_score=92
- ksp refs: none; fan_in=0 fan_out=3
- methods(1): Awake
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/ActionPaneDisplay_CelestialBody.cs
- 24 lines; kind=UIBehaviour; type=ActionPaneDisplay_CelestialBody; bases=ActionPaneDisplay; ancestry=ActionPaneDisplay/Selectable; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: Transform=3, UnityEngine.UI=2, GetComponent=2, Lifecycle=1, Instantiate/Destroy=1, GameObject=1; unity_score=90
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): Awake
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/AreaState.cs
- 9 lines; kind=enum; type=AreaState; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/BaseAPField.cs
- 366 lines; kind=class; type=BaseAPField; bases=BaseField<MEGUI_Control>; ancestry=BaseField/BaseField; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: none; fan_in=82 fan_out=4
- methods(3): GetValue, SetOriginalValue, SetValue
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/BaseAPFieldList.cs
- 14 lines; kind=class; type=BaseAPFieldList; bases=BaseFieldList<BaseAPField,MEGUI_Control>; ancestry=BaseFieldList/IEnumerable; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=13 fan_out=3
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/Category.cs
- 6 lines; kind=class; type=Category; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/CheckpointBrowserDialog.cs
- 449 lines; kind=MonoBehaviour; type=CheckpointBrowserDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: Expansions.Missions.Flow, Expansions.Missions.Runtime, KSP.Localization, KSP.UI, KSP.UI.Util, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=13, GameObject=4, SetActive=3, Transform=2, Instantiate/Destroy=2, GetComponent=2; unity_score=59
- ksp refs: Localizer=19, ConfigNode=8, InputLockManager=3, GameEvents=3, MissionSystem=1; fan_in=1 fan_out=23
- methods(32): AddMissionEntryWidget, BuildCheckpointList, CancelledCallback, ClearMissionList, ClearSelection, DeleteEntry, Dismiss, Hide, InjectMissionDataIntoCheckpoint, Lock, MissionCompare, OnButtonCancel, OnButtonRemoveDirty, OnButtonReset, OnButtonTest, OnDeleteConfirm, OnDeleteDismiss, OnDeleteEntry, OnEntrySelected, OnInputLocksModified, OnRemoveDirtyConfirm, OnRemoveDirtyDismiss, OnResetConfirm, OnSelectionChanged, SelectFileCallback, Show, Spawn, Start, TestSelectedCheckpoint, Unlock, UpdateUI, ValidateCheckpoints
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/CheckpointEntry.cs
- 230 lines; kind=MonoBehaviour; type=CheckpointEntry; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, TMPro=3, SetActive=2, Instantiate/Destroy=2, GameObject=2, MonoBehaviour=1; unity_score=58
- ksp refs: Localizer=12, ConfigNode=8, Game=6, FlightGlobals=1; fan_in=1 fan_out=11
- methods(10): Create, GetMissionNode, GetSaveGame, GetSaveGameMeta, Init, LoadMission, OnButtonRemove, OnValueChanged, Terminate, Validate
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/CrewCreationDialog.cs
- 227 lines; kind=MonoBehaviour; type=CrewCreationDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: Experience, KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=9, SetActive=5, GameObject=5, TMPro=4, Lifecycle=2, GetComponent=2; unity_score=71
- ksp refs: ProtoCrewMember=21, GameDatabase=6, Localizer=4, KerbalRoster=4, HighLogic=2, Game=1; fan_in=1 fan_out=14
- methods(14): CancelledCallback, CreateCallback, Dismiss, Hide, OnButtonCancel, OnButtonCreate, OnButtonRandom, OnDestroy, OnRoleChanged, ResetValues, SetValues, Setup, Show, Start
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/DisplayParameter.cs
- 18 lines; kind=struct; type=DisplayParameter; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=12
- ksp refs: ConfigNode=1; fan_in=4 fan_out=1
- methods(1): Load
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBody.cs
- 634 lines; kind=UIBehaviour; type=GAPCelestialBody; bases=ActionPaneDisplay_CelestialBody; ancestry=ActionPaneDisplay_CelestialBody/ActionPaneDisplay/Selectable; ns=Expansions.Missions.Editor
- usings: KSP.Localization, KSP.UI.TooltipTypes, UnityEngine, UnityEngine.EventSystems, UnityEngine.Events
- unity: GameObject=14, Transform=9, SetActive=9, LayerMask=9, Instantiate/Destroy=8, Raycast/Query=7; unity_score=69
- ksp refs: CelestialBody=8, Vector3d=7, Localizer=5, ScaledSpace=4, Orbit=2, QuaternionD=1; fan_in=20 fan_out=27
- methods(33): CallbackPQSLoaded, Clean, Destroy, DisplayResource, DoLocalSpaceRay, DoLocalSpaceRayAll, GetLightTimeCycle, GetUTFormattedString, Initialize, LateUpdate, Load, LoadPQS, LoadPQSCities, LoadPlanet, OnDisplayClick, OnDisplayClickUp, OnDisplayDrag, OnDisplayDragEnd, OnMouseOver, OnResourcesButtonClicked, SetFooterText, SetState, SliderAction_LightCycle, SliderAction_Zoom, SuscribeToLeftButton, SuscribeToRightButton, TogglePQS, UnloadPlanet, Update, UpdateFooterText_Light, UpdateFooterText_Zoom, UpdateScroll, UpdateSurfaceValues
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBody_InteractiveSurfaceGizmo.cs
- 182 lines; kind=MonoBehaviour; type=GAPCelestialBody_InteractiveSurfaceGizmo; bases=GAPCelestialBody_SurfaceGizmo; ancestry=GAPCelestialBody_SurfaceGizmo/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: Transform=5, GameObject=4, Texture=2, Collider=2, SetActive=1; unity_score=56
- ksp refs: CelestialBody=4, Vector3d=1; fan_in=7 fan_out=5
- methods(20): GetFocusTarget, GetFooterText, Initialize, IsActive, IsDragReady, OnButtonPressed_FocusTarget, OnGAPClick, OnGAPDrag, OnGAPDragEnd, OnGAPOver, OnGizmoHandleDrag, OnGizmoHandleDragEnd, OnGizmoHandleOver, OnGizmoHandleSelected, OnLoadPlanet, OnPointTranslate, OnUpdate, SetGizmoPosition, TryEndHover, TrySelectHandle
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBody_SurfaceGizmo.cs
- 100 lines; kind=MonoBehaviour; type=GAPCelestialBody_SurfaceGizmo; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: Transform=5, MonoBehaviour=1, Lifecycle=1; unity_score=52
- ksp refs: Vector3d=6, CelestialBody=3, QuaternionD=2; fan_in=3 fan_out=4
- methods(8): Initialize, IsActive, OnDestroy, OnLoadPlanet, OnUpdate, SetGizmoPosition, SetInSurfaceFromPoint, SetInSurfaceFromPointAtSeaLevel
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBody_SurfaceGizmo_Area.cs
- 212 lines; kind=MonoBehaviour; type=GAPCelestialBody_SurfaceGizmo_Area; bases=GAPCelestialBody_InteractiveSurfaceGizmo; ancestry=GAPCelestialBody_InteractiveSurfaceGizmo/GAPCelestialBody_SurfaceGizmo/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: Transform=2, Shader=2, Material=2, GetComponent=1, Collider=1; unity_score=45
- ksp refs: CelestialBody=3, Vector3d=1, Localizer=1; fan_in=1 fan_out=8
- methods(14): CheckAreaSelected, GetFooterText, Initialize, IsActive, OnGAPClick, OnGAPDrag, OnGAPDragEnd, OnGAPOver, OnGizmoHandleSelected, OnLoadPlanet, OnRadiusChange, OnUpdate, SetAreaColor, SetAreaRadius
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBody_SurfaceGizmo_Icon.cs
- 24 lines; kind=MonoBehaviour; type=GAPCelestialBody_SurfaceGizmo_Icon; bases=GAPCelestialBody_SurfaceGizmo; ancestry=GAPCelestialBody_SurfaceGizmo/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: none; unity_score=27
- ksp refs: CelestialBody=1; fan_in=1 fan_out=3
- methods(1): Initialize
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBody_SurfaceGizmo_PlaceVessel.cs
- 191 lines; kind=MonoBehaviour; type=GAPCelestialBody_SurfaceGizmo_PlaceVessel; bases=GAPCelestialBody_InteractiveSurfaceGizmo; ancestry=GAPCelestialBody_InteractiveSurfaceGizmo/GAPCelestialBody_SurfaceGizmo/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, KSP.UI, UnityEngine
- unity: Transform=8, SetActive=6, GameObject=3, Renderer=1, Input.=1; unity_score=63
- ksp refs: Localizer=1; fan_in=1 fan_out=6
- methods(15): AddTransformButtons, ClearMode, GetFooterText, GetGizmoCenterAngle, Initialize, OnGizmoHandleDrag, OnGizmoHandleDragEnd, OnGizmoHandleSelected, OnPointRotate, OnUpdate, SelectRotate, SelectTranslate, SetGizmoIcon, SetGizmoRotation, SetMode
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBody_SurfaceGizmo_Volume.cs
- 421 lines; kind=MonoBehaviour; type=GAPCelestialBody_SurfaceGizmo_Volume; bases=GAPCelestialBody_InteractiveSurfaceGizmo; ancestry=GAPCelestialBody_InteractiveSurfaceGizmo/GAPCelestialBody_SurfaceGizmo/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: SetActive=8, Transform=7, GameObject=7, Material=4, Shader=2, AnimationCurve=2; unity_score=57
- ksp refs: CelestialBody=12, Localizer=3, Vector3d=2; fan_in=1 fan_out=12
- methods(24): GetFooterText, HideShape, Initialize, IsActive, OnButtonPressed_Shape, OnDoubleSliderValue, OnGAPClick, OnGAPDrag, OnGAPDragEnd, OnGAPOver, OnGAPShapeBoundsChange, OnGAPShapeChange, OnGAPValueChange, OnGizmoHandleSelected, OnLoadPlanet, OnSimpleSliderValue, OnUpdate, SetAreaColor, SetConeBounds, SetShape, UpdateAreaState, UpdateFooterText_Cone, UpdateFooterText_Sphere, UpdateLineRenderer
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBodyGizmoHandle.cs
- 52 lines; kind=MonoBehaviour; type=GAPCelestialBodyGizmoHandle; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: Shader=5, Material=2, Renderer=1, MonoBehaviour=1, Lifecycle=1; unity_score=87
- ksp refs: none; fan_in=4 fan_out=0
- methods(5): OnHoverEnd, OnHoverStart, SetAlpha, SetColor, Start
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBodyState.cs
- 10 lines; kind=enum; type=GAPCelestialBodyState; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=7 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBodyState_Base.cs
- 489 lines; kind=class; type=GAPCelestialBodyState_Base; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: Expansions.Missions.Actions, FinePrint, KSP.Localization, KSP.UI.Screens.Mapview, UnityEngine, UnityEngine.EventSystems
- unity: GameObject=5, Instantiate/Destroy=4, Transform=3, SetActive=3, Raycast/Query=3, Lifecycle=2; unity_score=30
- ksp refs: Vessel=17, Orbit=9, Localizer=3, CelestialBody=3, QuaternionD=1, ProtoCrewMember=1; fan_in=4 fan_out=37
- methods(26): ClearAdditionalInfo, CreateSimpleOrbit, CreateSurfaceAreaGizmo, CreateSurfaceIcon, End, Init, LateUpdate, LoadAdditionalInfo, LoadPlanet, OnClick, OnClickUp, OnDrag, OnDragEnd, OnFilterButton, OnFilterButton_Asteroids, OnFilterButton_Comets, OnFilterButton_Flags, OnFilterButton_Kerbals, OnFilterButton_LaunchSites, OnFilterButton_Objectives, OnFilterButton_Vessels, OnMouseOver, SetFilter, UnloadPlanet, Update, UpdateAdditionalInfo
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBodyState_Biomes.cs
- 183 lines; kind=class; type=GAPCelestialBodyState_Biomes; bases=GAPCelestialBodyState_Base; ancestry=GAPCelestialBodyState_Base; ns=Expansions.Missions.Editor
- usings: KSP.Localization, UnityEngine
- unity: Shader=9, Material=3, Transform=2, Renderer=2, GetComponent=2, Time.=1; unity_score=57
- ksp refs: CelestialBody=17, Localizer=1; fan_in=2 fan_out=6
- methods(13): End, GetBiomeIndex, HasSelectedBiome, Init, LoadPlanet, OnBiomeSelected, OnClickUp, OnMouseOver, SelectBiome, SelectBiomeByName, UnloadPlanet, Update, UpdateGAPText
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBodyState_Orbit.cs
- 266 lines; kind=class; type=GAPCelestialBodyState_Orbit; bases=GAPCelestialBodyState_Base; ancestry=GAPCelestialBodyState_Base; ns=Expansions.Missions.Editor
- usings: KSP.Localization, KSP.UI.Screens.Mapview, UnityEngine, UnityEngine.EventSystems
- unity: Instantiate/Destroy=4, Transform=3, Raycast/Query=3, Input.=3, GameObject=3, SetActive=1; unity_score=43
- ksp refs: Vector3d=8, CelestialBody=3, ScaledSpace=2, Localizer=1; fan_in=4 fan_out=17
- methods(21): CalculateBodyEditorAngle, CreateMapNode, End, GizmoDragCastHit, Init, LateUpdate, LoadPlanet, OnClick, OnDrag, OnMouseOver, OnUpdateCaptionUser, OnUpdatePositionHover, OnUpdatePositionToUI, OnUpdatePositionUser, OnUpdateType, OnUpdateVisibleHover, OnUpdateVisibleUser, ToggleMeanAnomalyIcon, UnloadPlanet, UpdateGizmoPosition, UpdateOrbit
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPCelestialBodyState_SurfaceGizmo.cs
- 160 lines; kind=class; type=GAPCelestialBodyState_SurfaceGizmo; bases=GAPCelestialBodyState_Base; ancestry=GAPCelestialBodyState_Base; ns=Expansions.Missions.Editor
- usings: KSP.UI, UnityEngine, UnityEngine.EventSystems
- unity: Raycast/Query=4, Input.=3, Instantiate/Destroy=2, GameObject=2, Lifecycle=1, GetComponent=1; unity_score=48
- ksp refs: CelestialBody=5, Vector3d=4, QuaternionD=1; fan_in=1 fan_out=11
- methods(13): ButtonAction_Recenter, End, GetSurfaceLocation, Init, InstantiateGizmo, LoadPlanet, OnClickUp, OnDrag, OnDragEnd, OnMouseOver, UnloadPlanet, Update, UpdateFooterText
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPOrbitRenderer.cs
- 248 lines; kind=MonoBehaviour; type=GAPOrbitRenderer; bases=OrbitRendererBase; ancestry=OrbitRendererBase/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: FinePrint.Utilities, KSP.UI.Screens.Mapview, UnityEngine, Vectrosity
- unity: Instantiate/Destroy=8, GameObject=7, Transform=4, Shader=2, Lifecycle=2, GetComponent=2; unity_score=65
- ksp refs: MapView=4, Vector3d=2, OrbitDriver=2, ScaledSpace=1, Orbit=1; fan_in=2 fan_out=13
- methods(13): AttachNodeUIs, AttachObjectNode, Create, Destroy, Display, DrawOrbit, DrawSpline, LateUpdate, OnUpdateCaption, OnUpdatePosition, OnUpdatePositionToUI, OnUpdateVisibleHover, Start
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPPartPicker.cs
- 46 lines; kind=UIBehaviour; type=GAPPartPicker; bases=ActionPaneDisplay; ancestry=ActionPaneDisplay/Selectable; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: Instantiate/Destroy=3, GameObject=3, UnityEngine.UI=2, SetActive=2, Transform=1, GetComponent=1; unity_score=85
- ksp refs: none; fan_in=1 fan_out=3
- methods(3): Clean, Destroy, Setup
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPPrefabDisplay.cs
- 103 lines; kind=UIBehaviour; type=GAPPrefabDisplay; bases=ActionPaneDisplay; ancestry=ActionPaneDisplay/Selectable; ns=Expansions.Missions.Editor
- usings: UnityEngine, UnityEngine.EventSystems
- unity: GameObject=10, Instantiate/Destroy=9, SetActive=3, GetComponent=3, Transform=2, LayerMask=2; unity_score=90
- ksp refs: none; fan_in=5 fan_out=4
- methods(6): Clean, Destroy, LoadTarget, OnDisplayDrag, Setup, Update
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPSurfaceIcon.cs
- 93 lines; kind=class; type=GAPSurfaceIcon; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: KSP.UI.Screens.Mapview, UnityEngine
- unity: Transform=3, UnityEngine.UI=1, Instantiate/Destroy=1; unity_score=33
- ksp refs: Vector3d=3, CelestialBody=2; fan_in=1 fan_out=4
- methods(8): CreateMapNode, Destroy, Display, OnUpdate, OnUpdateCaption, OnUpdatePosition, OnUpdatePositionToUI, OnUpdateVisibleHover
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPUtil_CelestialBody.cs
- 222 lines; kind=MonoBehaviour; type=GAPUtil_CelestialBody; bases=GAPUtil_BiSelector; ancestry=GAPUtil_BiSelector/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.UI, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=5, SetActive=5, GameObject=5, Lifecycle=3, Light=2, EventSystems=2; unity_score=68
- ksp refs: none; fan_in=3 fan_out=4
- methods(14): Awake, ClearEvents, ClearHoveredSlider, InitializeSliderEvents, OnDestroy, OnPointerDown, OnPointerUp, OnSliderHovered, OnSliderMouseDown, OnSliderMouseEnter, OnSliderMouseExit, OnSliderMouseUp, ScrollWithButtons, Update
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPUtil_VesselFrame.cs
- 100 lines; kind=MonoBehaviour; type=GAPUtil_VesselFrame; bases=GAPUtil_BiSelector; ancestry=GAPUtil_BiSelector/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Lifecycle=2, EventSystems=2, UnityEngine.UI=1; unity_score=54
- ksp refs: Localizer=2, ShipConstruct=1; fan_in=2 fan_out=7
- methods(9): Awake, OnCategoryValueChange, OnDestroy, OnPointerDown, OnPointerUp, ScrollWithButtons, SetPartTypes, SetTitleText, categoryValueChange
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/GAPVesselDisplay.cs
- 445 lines; kind=UIBehaviour; type=GAPVesselDisplay; bases=ActionPaneDisplay; ancestry=ActionPaneDisplay/Selectable; ns=Expansions.Missions.Editor
- usings: KSP.Localization, ProceduralFairings, UnityEngine
- unity: GetComponent=13, Instantiate/Destroy=10, GameObject=9, SetActive=7, Renderer=4, Collider=4; unity_score=69
- ksp refs: Part=9, Localizer=7, ShipConstruct=5, FlightGlobals=1; fan_in=2 fan_out=19
- methods(18): ChangeHoverPart, ChangeSelectedPart, Clean, Destroy, DestroyCurrentVessel, HideFairings, HideVessel, LoadVessel, OnDisplayClick, OnMouseOver, OnNextVessel, OnPartCategoryChanged, OnPrevVessel, Setup, SetupVessel, ShowFairings, ShowVessel, UpdateFrameFooter
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/HistoryType.cs
- 8 lines; kind=enum; type=HistoryType; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=26 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/IMEHistoryTarget.cs
- 7 lines; kind=interface; type=IMEHistoryTarget; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=1; fan_in=7 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEActionPane.cs
- 661 lines; kind=MonoBehaviour; type=MEActionPane; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, RUI.Icons.Selectable, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Transform=13, GameObject=13, UnityEngine.UI=12, Instantiate/Destroy=9, GetComponent=8, SetActive=7; unity_score=67
- ksp refs: GameSettings=3, Localizer=1, GameEvents=1; fan_in=3 fan_out=22
- methods(33): AddParameterBlock, Awake, CheckDeletedNodeFromGap, Clean, ClearCache, CurrentLockdeNode, DisplayModuleFooter, DisplayModuleHeader, DisplayNodeModuleSection, DisplayNodeSettingsSection, DisplayParameter, GAPInitialize, GetInputFieldsList, GetNextInputTabStop, GetParameterFromFieldID, InitializeGapMinHeight, InputfieldStringCatcher, OnDestroy, OnGAPLockValueChange, OnParameterClick, OnSapScrollbarValueChange, RemoveParameterGroup, SAPDisplayConnectorParameters, SAPDisplayMultipleSelectedItemsMessage, SAPDisplayNodeParameters, SAPRefreshNodeParameters, SavePreferedGapSize, ScrollBar, ScrollPanel, SetLockInteractivity, SetMaxPanelWidth, Start, UpdateSAPScroll
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEBasicNode.cs
- 207 lines; kind=class; type=MEBasicNode; bases=IConfigNode; ancestry=IConfigNode; ns=Expansions.Missions.Editor
- usings: KSP.Localization, UnityEngine
- unity: GameObject=2, Instantiate/Destroy=1; unity_score=13
- ksp refs: Localizer=8, ConfigNode=8; fan_in=7 fan_out=14
- methods(4): BuildExtendedInfoListFromObject, Initialize, Load, Save
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MECrewAssignmentDialog.cs
- 774 lines; kind=MonoBehaviour; type=MECrewAssignmentDialog; bases=BaseCrewAssignmentDialog; ancestry=BaseCrewAssignmentDialog/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: Expansions.Missions.Actions, KSP.Localization, KSP.UI, KSP.UI.Util, TMPro, UnityEngine, UnityEngine.UI
- unity: GetComponent=19, UnityEngine.UI=17, GameObject=9, SetActive=4, Instantiate/Destroy=4, Transform=2; unity_score=57
- ksp refs: ProtoCrewMember=28, Vessel=6, Localizer=4, KerbalRoster=4; fan_in=1 fan_out=34
- methods(40): AddItem, AddSpacer, AddVesselTitle, ChangeKerbalEditorMode, CleanSelection, CreateAvailList, CreateCrewList, CreateTempKerbalList, Dismiss, DropOnAvailList, DropOnCrewList, DropOnTempList, GetCurrentCrewRoster, Hide, ListItemButtonClick, MoveCrewToAvail, MoveCrewToEmptySeat, OkCallback, OnButtonCancel, OnButtonOk, OnCrewEntrySelected, OnCrewMemberCreate, OnCrewMemberCreateDialogCancelled, OnCrewMemberDeleteConfirm, OnCrewSelectionChanged, OnGlobalTitleClick, OnTabApplicantsPress, OnTabCreateEditPress, PromptDeleteCrewMemberConfirm, Refresh, RefreshCrewItem, RefreshCrewLists, RemoveCrewFromRoster, SaveGlobalChanges, SetCurrentCrewRoster, SetTab, SetTabVisible, Show, Spawn, Start
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Asteroid.cs
- 10 lines; kind=attribute; type=MEGUI_Asteroid; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_AsteroidSelect.cs
- 6 lines; kind=attribute; type=MEGUI_AsteroidSelect; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Button.cs
- 12 lines; kind=attribute; type=MEGUI_Button; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Category.cs
- 6 lines; kind=attribute; type=MEGUI_Category; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_CelestialBody.cs
- 14 lines; kind=attribute; type=MEGUI_CelestialBody; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=6 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_CelestialBody_Biomes.cs
- 6 lines; kind=attribute; type=MEGUI_CelestialBody_Biomes; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_CelestialBody_Orbit.cs
- 7 lines; kind=attribute; type=MEGUI_CelestialBody_Orbit; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Checkbox.cs
- 7 lines; kind=attribute; type=MEGUI_Checkbox; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=20 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_ColorPicker.cs
- 6 lines; kind=attribute; type=MEGUI_ColorPicker; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Comet.cs
- 10 lines; kind=attribute; type=MEGUI_Comet; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_CometSelect.cs
- 6 lines; kind=attribute; type=MEGUI_CometSelect; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Control.cs
- 109 lines; kind=attribute; type=MEGUI_Control; bases=FieldAttribute; ancestry=FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=4
- ksp refs: Localizer=1; fan_in=78 fan_out=3
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_CrewList.cs
- 6 lines; kind=attribute; type=MEGUI_CrewList; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Dropdown.cs
- 19 lines; kind=attribute; type=MEGUI_Dropdown; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=63 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_DynamicModule.cs
- 6 lines; kind=attribute; type=MEGUI_DynamicModule; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_DynamicModuleList.cs
- 16 lines; kind=attribute; type=MEGUI_DynamicModuleList; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_FlagSelect.cs
- 6 lines; kind=attribute; type=MEGUI_FlagSelect; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_GameParameters.cs
- 6 lines; kind=attribute; type=MEGUI_GameParameters; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_InputField.cs
- 15 lines; kind=attribute; type=MEGUI_InputField; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=41 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Label.cs
- 13 lines; kind=attribute; type=MEGUI_Label; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_LaunchSiteSelect.cs
- 6 lines; kind=attribute; type=MEGUI_LaunchSiteSelect; bases=MEGUI_Dropdown; ancestry=MEGUI_Dropdown/MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_LaunchSiteSituation.cs
- 10 lines; kind=attribute; type=MEGUI_LaunchSiteSituation; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_ListOrder.cs
- 12 lines; kind=attribute; type=MEGUI_ListOrder; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_MissionInstructor.cs
- 6 lines; kind=attribute; type=MEGUI_MissionInstructor; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_MissionKerbal.cs
- 11 lines; kind=attribute; type=MEGUI_MissionKerbal; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: ProtoCrewMember=1; fan_in=11 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_NodeLabelNodeSelect.cs
- 6 lines; kind=attribute; type=MEGUI_NodeLabelNodeSelect; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_NodeSelect.cs
- 6 lines; kind=attribute; type=MEGUI_NodeSelect; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_NumberRange.cs
- 19 lines; kind=attribute; type=MEGUI_NumberRange; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=31 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_ParameterSwitchCompound.cs
- 12 lines; kind=attribute; type=MEGUI_ParameterSwitchCompound; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=14 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_ParameterSwitchCompound_KeyField.cs
- 10 lines; kind=attribute; type=MEGUI_ParameterSwitchCompound_KeyField; bases=MEGUI_Dropdown; ancestry=MEGUI_Dropdown/MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_PartPicker.cs
- 62 lines; kind=attribute; type=MEGUI_PartPicker; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Quaternion.cs
- 10 lines; kind=attribute; type=MEGUI_Quaternion; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_ScoreRangeList.cs
- 31 lines; kind=attribute; type=MEGUI_ScoreRangeList; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_ScrollViewSelector.cs
- 6 lines; kind=attribute; type=MEGUI_ScrollViewSelector; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_SurfaceArea.cs
- 6 lines; kind=attribute; type=MEGUI_SurfaceArea; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_SurfaceVolume.cs
- 6 lines; kind=attribute; type=MEGUI_SurfaceVolume; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_TextArea.cs
- 15 lines; kind=attribute; type=MEGUI_TextArea; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=6 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_Time.cs
- 6 lines; kind=attribute; type=MEGUI_Time; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=9 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_VesselGroundLocation.cs
- 20 lines; kind=attribute; type=MEGUI_VesselGroundLocation; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=6 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_VesselLocation.cs
- 6 lines; kind=attribute; type=MEGUI_VesselLocation; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_VesselPartSelect.cs
- 6 lines; kind=attribute; type=MEGUI_VesselPartSelect; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=7 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_VesselSelect.cs
- 9 lines; kind=attribute; type=MEGUI_VesselSelect; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=11 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUI_VesselSituation.cs
- 10 lines; kind=attribute; type=MEGUI_VesselSituation; bases=MEGUI_Control; ancestry=MEGUI_Control/FieldAttribute/Attribute; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUICompoundParameter.cs
- 212 lines; kind=MonoBehaviour; type=MEGUICompoundParameter; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: Transform=10, GameObject=6, SetActive=4, Lifecycle=2, Instantiate/Destroy=1, Input.=1; unity_score=64
- ksp refs: none; fan_in=20 fan_out=7
- methods(8): AddSubParameter, Display, GetSubParameter, OnDestroy, Setup, SortParameters, SortParametersByOrder, Update
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIConnector.cs
- 409 lines; kind=MonoBehaviour; type=MEGUIConnector; bases=MonoBehaviour,IMEHistoryTarget; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine, Vectrosity
- unity: Instantiate/Destroy=5, GameObject=3, Coroutine=3, UnityEngine.UI=2, SetActive=2, MonoBehaviour=1; unity_score=46
- ksp refs: GameEvents=4, ConfigNode=3; fan_in=4 fan_out=11
- methods(23): Activate, AddNewEnd, CleanUp, ConnectLogic, DelayedUpdateLine, Deselect, Destroy, DisconnectLogic, FirstUpdate, GetConnectedNode, GetState, HideLine, IsConnectedToNode, IsNewConnectionValid, MakeCurveType1, MakeCurveType2, OnHistoryCreate, OnHistoryDestroy, ResetLineWidth, Select, SetUpConnector, UpdateLine, WasMouseClickWithinTolerance
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIConnectorDrag.cs
- 88 lines; kind=MonoBehaviour; type=MEGUIConnectorDrag; bases=MonoBehaviour,IPointerDownHandler,IEventSystemHandler,IPointerUpHandler; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=7, MonoBehaviour=1, GetComponent=1, GameObject=1; unity_score=72
- ksp refs: none; fan_in=0 fan_out=2
- methods(2): GetMeGUIParent, IsValidDrop
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIDropDownItem.cs
- 18 lines; kind=class; type=MEGUIDropDownItem; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=26 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIDropDownItemListExtensions.cs
- 37 lines; kind=static class; type=MEGUIDropDownItemListExtensions; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=1
- methods(3): GetDisplayStrings, GetKeys, GetValues
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIFooterAdditionalButton.cs
- 100 lines; kind=MonoBehaviour; type=MEGUIFooterAdditionalButton; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro, UnityEngine
- unity: Transform=3, TMPro=2, Instantiate/Destroy=1; unity_score=51
- ksp refs: none; fan_in=1 fan_out=6
- methods(4): Create, OnAdditionalValueChanged, OnRemovevalueChanged, Setup
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIListOrderItem.cs
- 15 lines; kind=MonoBehaviour; type=MEGUIListOrderItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, TMPro=1, MonoBehaviour=1; unity_score=74
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUINode.cs **MUST-OPEN (>800 lines)**
- 1390 lines; kind=MonoBehaviour; type=MEGUINode; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler,IMEHistoryTarget; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, KSP.UI, KSP.UI.TooltipTypes, RUI.Icons.Selectable, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: SetActive=32, GameObject=31, UnityEngine.UI=27, Transform=14, GetComponent=9, Instantiate/Destroy=8; unity_score=65
- ksp refs: ConfigNode=8, Localizer=1; fan_in=16 fan_out=23
- methods(74): AddNewLine, Awake, CanDock, CancelDrag, CheckDockStatus, CheckNodeDockingPosition, CleanNodeBodyParameters, CleanUp, ClearDockableIndicator, ClearHighlighter, ClearValidityIndicators, DelayedUpdateConnectors, Deselect, Destroy, DisplayModuleSection, DisplayNodeBodyParameters, DisplayParameter, DockNode, DoesConnectionExist, GetDepthOfDockedNode, GetHighlighterColor, GetInputButtonPosition, GetOutputButtonPosition, GetState, HasInputConnections, HasOutputConnections, InputButtonDragged, InputButtonDropped, NodeGroupDrag, OnDragEndClearDockingIndicators, OnDragSetDockingIndicators, OnHistoryCreate, OnHistoryDestroy, OnHistoryDock, OnHistoryMove, OnHistoryUnDock, OnNodeBeginDrag, OnNodeDrag, OnNodeEndDrag, OnPointerClick, OutputButtonDragged, OutputButtonDropped, PlaceNodeAtCanvas, PushUndoActionOnPasteNode, RemoveAllConnections ...
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUINodeBodyParameter.cs
- 26 lines; kind=MonoBehaviour; type=MEGUINodeBodyParameter; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro, UnityEngine
- unity: Transform=4, UnityEngine.UI=2, TMPro=1, MonoBehaviour=1; unity_score=87
- ksp refs: none; fan_in=1 fan_out=2
- methods(1): Setup
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUINodeIcon.cs
- 387 lines; kind=MonoBehaviour; type=MEGUINodeIcon; bases=MonoBehaviour,IBeginDragHandler,IEventSystemHandler,IDragHandler,IEndDragHandler,IPointerEnterHandler,IPointerExitHandler,IComparable<MEGUINodeIcon>,IPointerClickHandler; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: RUI.Icons.Selectable, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: EventSystems=13, UnityEngine.UI=10, Lifecycle=2, Transform=1, TMPro=1, SetActive=1; unity_score=69
- ksp refs: none; fan_in=12 fan_out=7
- methods(15): CompareTo, OnBeginDrag, OnClickCanvas, OnDrag, OnEndDrag, OnPointerClick, OnPointerEnter, OnPointerExit, ResetCanvasSetting, SetHighlighterColor, SetUp, Start, ToggleHighlighter, Update, onToggle
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIPanel.cs
- 341 lines; kind=MonoBehaviour; type=MEGUIPanel; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=13, Screen/Cursor=12, SetActive=6, GameObject=6, Coroutine=6, GetComponent=5; unity_score=73
- ksp refs: none; fan_in=3 fan_out=0
- methods(13): Awake, FixedUpdate, Hide, Maximize, OnMaximizeValueChange, OnVisibilityValueChange, RebuildPanelLayout, Reset, Restore, Show, Start, VisibilityHideAnimation, VisibilityShowAnimation
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIPanelSeparator.cs
- 86 lines; kind=MonoBehaviour; type=MEGUIPanelSeparator; bases=MonoBehaviour,IDragHandler,IEventSystemHandler,IPointerEnterHandler,IPointerExitHandler,IBeginDragHandler,IEndDragHandler; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: Cursors, KSP.UI, UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=10, Screen/Cursor=6, MonoBehaviour=1, Lifecycle=1; unity_score=81
- ksp refs: none; fan_in=0 fan_out=5
- methods(6): OnBeginDrag, OnDrag, OnEndDrag, OnPointerEnter, OnPointerExit, Start
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameter.cs
- 569 lines; kind=MonoBehaviour; type=MEGUIParameter; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler,IMEHistoryTarget; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.UI, KSP.UI.TooltipTypes, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=18, EventSystems=12, GameObject=11, GetComponent=10, Transform=9, SetActive=6; unity_score=72
- ksp refs: ConfigNode=4; fan_in=37 fan_out=15
- methods(34): Awake, BlockMaskedControls, ChangeColor, Create, Display, DisplayGAP, DisplayParameterObject, GetGroupName, GetState, HideColor, HideGAPButton, LockLocalizedText, MaskScrollControls, OnEnable, OnHistoryPinned, OnParameterReset, OnPinValueChanged, OnPointerEnter, OnPointerExit, RefreshUI, RemoveParameterByGroup, ResetDefaultValue, Select, SetPinToggleImage, SetTooltipActive, SetTooltipText, Setup, ShowColor, Start, ToggleGAPButton, UnSelect, UnblockMaskedControls, Update, UpdateNodeBodyUI
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterAsteroid.cs
- 94 lines; kind=MonoBehaviour; type=MEGUIParameterAsteroid; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro, UnityEngine.UI
- unity: TMPro=2, UnityEngine.UI=1, Lifecycle=1, GetComponent=1; unity_score=51
- ksp refs: none; fan_in=0 fan_out=10
- methods(6): Button_RegenerateSeed, DisplayGAP, InputText_NewSeed, RefreshGapPrefab, Setup, Start
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterAsteroidDropdownList.cs
- 141 lines; kind=MonoBehaviour; type=MEGUIParameterAsteroidDropdownList; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: Expansions.Missions.Actions, TMPro
- unity: TMPro=1, Lifecycle=1; unity_score=27
- ksp refs: GameEvents=4, ConfigNode=1; fan_in=3 fan_out=12
- methods(9): Display, OnDestroy, OnHistoryValueChange, OnParameterValueChanged, RefreshUI, SetDropdownValues, Setup, onBuilderNodeAdded, onBuilderNodeDeleted
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterAwardModule.cs
- 101 lines; kind=MonoBehaviour; type=MEGUIParameterAwardModule; bases=MEGUIParameterDynamicModule; ancestry=MEGUIParameterDynamicModule/MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: Transform=6, SetActive=3, UnityEngine.UI=2, TMPro=2, GameObject=2; unity_score=72
- ksp refs: none; fan_in=1 fan_out=5
- methods(5): OnAwardedPointsEndEdit, OnAwardedPointsValueChange, OnModuleEnabledToggleValueChange, Setup, SetupAwardModule
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterButton.cs
- 60 lines; kind=MonoBehaviour; type=MEGUIParameterButton; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro, UnityEngine.UI
- unity: UnityEngine.UI=1, TMPro=1; unity_score=44
- ksp refs: none; fan_in=0 fan_out=3
- methods(2): OnButtonClick, Setup
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCelestialBody.cs
- 73 lines; kind=MonoBehaviour; type=MEGUIParameterCelestialBody; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=4 fan_out=7
- methods(5): DisplayGAP, OnLeftGapClick, OnParameterValueChanged, OnRightGapClick, Setup
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCelestialBody_Area.cs
- 159 lines; kind=MonoBehaviour; type=MEGUIParameterCelestialBody_Area; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=15
- ksp refs: CelestialBody=1; fan_in=0 fan_out=11
- methods(12): DisplayGAP, OnDragRadius, OnDropDownBody, OnGAPPointTranslate, OnInputLatitude, OnInputLongitude, OnInputRadius, OnLeftGapClick, OnRightGapClick, Setup, UpdateGizmo, UpdateMaxRadiusValues
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCelestialBody_Biomes.cs
- 99 lines; kind=MonoBehaviour; type=MEGUIParameterCelestialBody_Biomes; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=15
- ksp refs: FlightGlobals=1, CelestialBody=1; fan_in=2 fan_out=11
- methods(8): DisplayGAP, OnDropDownBiome, OnDropDownBody, OnGAPBiomeSelected, OnGapLeftArrow, OnGapRightArrow, Setup, UpdateValue
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCelestialBody_VesselGroundLocation.cs
- 222 lines; kind=MonoBehaviour; type=MEGUIParameterCelestialBody_VesselGroundLocation; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: SetActive=6, GameObject=6, Transform=3; unity_score=51
- ksp refs: none; fan_in=3 fan_out=13
- methods(12): DisplayGAP, OnDropDownBody, OnGAPPointTranslate, OnGapLeftArrow, OnGapPointRotate, OnGapRightArrow, OnGizmoIconChange, OnInputLatitude, OnInputLongitude, OnInputRot, Setup, UpdateGizmo
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCelestialBody_Volume.cs
- 339 lines; kind=MonoBehaviour; type=MEGUIParameterCelestialBody_Volume; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: SetActive=12, GameObject=12; unity_score=51
- ksp refs: CelestialBody=1; fan_in=0 fan_out=11
- methods(23): AttachCallbacks, DisplayGAP, MinValuesUpdate, OnDragRadius, OnDropDownBody, OnDropDownShape, OnGAPPointTranslate, OnGAPShapeChange, OnGAPSliderConeBounds, OnGAPSliderSphere, OnInputHeightConeMax, OnInputHeightConeMin, OnInputHeightSphere, OnInputLatitude, OnInputLongitude, OnInputRadius, OnLeftGapClick, OnRightGapClick, RefreshMinHeightValues, Setup, UpdateGizmo, UpdateGizmoShape, UpdateMaxValues
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCelestialBodyOrbit.cs
- 244 lines; kind=MonoBehaviour; type=MEGUIParameterCelestialBodyOrbit; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: SetActive=2, GameObject=2; unity_score=27
- ksp refs: Orbit=12, Vector3d=6, ConfigNode=6, CelestialBody=2, QuaternionD=1; fan_in=1 fan_out=20
- methods(16): DisplayGAP, GetState, OnDropDownBody, OnGapLeftArrow, OnGapRightArrow, OnGlobalGizmoUpdate, OnHistoryValueChange, OnOrbitReset, OnPointGizmoUpdate, OnSubParameterResetValue, OnSubParameterValueChange, OnUpdateSliderValues, OnUpdateValues, RefreshUI, Setup, UpdateSemiMajorAxisLimits
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCheckbox.cs
- 80 lines; kind=MonoBehaviour; type=MEGUIParameterCheckbox; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine.UI
- unity: UnityEngine.UI=1; unity_score=28
- ksp refs: ConfigNode=1; fan_in=9 fan_out=5
- methods(5): OnHistoryValueChange, OnParameterValueChanged, RefreshUI, ResetDefaultValue, Setup
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterColorPicker.cs
- 109 lines; kind=MonoBehaviour; type=MEGUIParameterColorPicker; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=9, GetComponent=3, Transform=2, GameObject=2, Instantiate/Destroy=1; unity_score=78
- ksp refs: ConfigNode=4; fan_in=0 fan_out=6
- methods(7): GetState, OnColorChange, OnHistoryValueChange, OnPickColorButton, RefreshUI, ResetDefaultValue, Setup
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterComet.cs
- 94 lines; kind=MonoBehaviour; type=MEGUIParameterComet; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, TMPro, UnityEngine.UI
- unity: TMPro=2, UnityEngine.UI=1, Lifecycle=1, GetComponent=1; unity_score=51
- ksp refs: Localizer=1; fan_in=0 fan_out=11
- methods(6): Button_RegenerateSeed, DisplayGAP, InputText_NewSeed, RefreshGapPrefab, Setup, Start
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCometDropdownList.cs
- 141 lines; kind=MonoBehaviour; type=MEGUIParameterCometDropdownList; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: Expansions.Missions.Actions, TMPro
- unity: TMPro=1, Lifecycle=1; unity_score=27
- ksp refs: GameEvents=4, ConfigNode=1; fan_in=3 fan_out=12
- methods(9): Display, OnDestroy, OnHistoryValueChange, OnParameterValueChanged, RefreshUI, SetDropdownValues, Setup, onBuilderNodeAdded, onBuilderNodeDeleted
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterCrewList.cs
- 73 lines; kind=MonoBehaviour; type=MEGUIParameterCrewList; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro
- unity: TMPro=1; unity_score=29
- ksp refs: ProtoCrewMember=4, KerbalRoster=1, ConfigNode=1; fan_in=0 fan_out=8
- methods(5): OnHistoryValueChange, OnParameterValueChanged, RefreshUI, ResetDefaultValue, Setup
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterDropdownList.cs
- 300 lines; kind=MonoBehaviour; type=MEGUIParameterDropdownList; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro, UnityEngine
- unity: GameObject=3, UnityEngine.UI=1, Transform=1, TMPro=1, Lifecycle=1; unity_score=33
- ksp refs: ConfigNode=4; fan_in=28 fan_out=10
- methods(14): BuildDropDownItemsList, DisplayGAP, GetItemIndex, GetState, OnEnable, OnHistoryValueChange, OnParameterValueChanged, RebuildDropDown, RefreshGapPrefab, RefreshUI, ResetDefaultValue, SetExcludedOptions, Setup, ValueComparer
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterDynamicModule.cs
- 78 lines; kind=MonoBehaviour; type=MEGUIParameterDynamicModule; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine, UnityEngine.UI
- unity: Transform=6, SetActive=3, UnityEngine.UI=2, GameObject=2; unity_score=75
- ksp refs: none; fan_in=2 fan_out=4
- methods(4): Create, OnRemoveButton, Setup, ToggleExpand
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterDynamicModuleList.cs
- 313 lines; kind=MonoBehaviour; type=MEGUIParameterDynamicModuleList; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=11, SetActive=9, Transform=4, UnityEngine.UI=3, Coroutine=3, Texture=2; unity_score=65
- ksp refs: ConfigNode=11; fan_in=6 fan_out=12
- methods(15): AddModule, AddModules, CollapseGroup, FieldValueContainsModule, GetState, OnAddValueChange, OnButtonCollapsePressed, OnEnable, OnHistoryAddModule, OnHistoryRemoveModule, RefreshUI, RemoveModule, RemoveModules, Setup, WaitForDropdownList
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterFlagDropdownList.cs
- 141 lines; kind=MonoBehaviour; type=MEGUIParameterFlagDropdownList; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: Expansions.Missions.Actions, TMPro
- unity: TMPro=1, Lifecycle=1; unity_score=27
- ksp refs: GameEvents=4, ConfigNode=1; fan_in=1 fan_out=12
- methods(9): Display, OnDestroy, OnHistoryValueChange, OnParameterValueChanged, RefreshUI, SetDropdownValues, Setup, onBuilderNodeAdded, onBuilderNodeDeleted
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterGameParameters.cs
- 274 lines; kind=MonoBehaviour; type=MEGUIParameterGameParameters; bases=MEGUIParameter,IMEHistoryTarget; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, Instantiate/Destroy=1; unity_score=22
- ksp refs: ConfigNode=8, GameEvents=2, Game=1; fan_in=0 fan_out=17
- methods(10): CopyDeleteStockCraft, GetState, OnDifficultySettingsOpen, OnGameParametersChanged, OnHistoryValueChanged, OpenDifficultySettings, ResetDefaultValue, Setup, craftFileInUse, getStockFiles
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterGroup.cs
- 156 lines; kind=MonoBehaviour; type=MEGUIParameterGroup; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=7, SetActive=7, GameObject=7, Transform=3, Texture=2, GetComponent=2; unity_score=78
- ksp refs: none; fan_in=5 fan_out=6
- methods(8): CollapseGroup, Create, Display, OnButtonCollapsePressed, OnParameterGroupReset, Setup, SortChilden, SortParametersByOrder
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterGroupHeader.cs
- 45 lines; kind=MonoBehaviour; type=MEGUIParameterGroupHeader; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: EventSystems=4, UnityEngine.UI=3, Lifecycle=2, MonoBehaviour=1, GetComponent=1; unity_score=87
- ksp refs: none; fan_in=1 fan_out=0
- methods(4): Awake, OnPointerEnter, OnPointerExit, Start
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterInputField.cs
- 268 lines; kind=MonoBehaviour; type=MEGUIParameterInputField; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, TMPro, UnityEngine
- unity: TMPro=5, UnityEngine.UI=1, Transform=1, GameObject=1; unity_score=40
- ksp refs: Localizer=9, ConfigNode=1; fan_in=4 fan_out=13
- methods(11): Convert, LockLocalizedText, OnHistoryValueChange, OnParameterEndEdit, OnParameterValueChanged, OnSelectInput, OnValidatePercentageInput, Parse, RefreshUI, ResetDefaultValue, Setup
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterLabel.cs
- 61 lines; kind=MonoBehaviour; type=MEGUIParameterLabel; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro
- unity: TMPro=1, SetActive=1, GameObject=1; unity_score=46
- ksp refs: none; fan_in=2 fan_out=3
- methods(2): RefreshUI, Setup
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterLaunchSiteDropdownList.cs
- 79 lines; kind=MonoBehaviour; type=MEGUIParameterLaunchSiteDropdownList; bases=MEGUIParameterDropdownList; ancestry=MEGUIParameterDropdownList/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: -
- unity: Lifecycle=1; unity_score=24
- ksp refs: GameEvents=4; fan_in=1 fan_out=10
- methods(5): BuildDropDownItemsList, OnDestroy, Setup, onBuilderNodeAdded, onBuilderNodeDeleted
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterLaunchSiteSituation.cs
- 114 lines; kind=MonoBehaviour; type=MEGUIParameterLaunchSiteSituation; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=6, Transform=5, SetActive=3, GameObject=3, GetComponent=2, TMPro=1; unity_score=79
- ksp refs: ConfigNode=4; fan_in=0 fan_out=9
- methods(6): Display, GetState, InputTextAction_SetLaunchSiteName, Setup, onHistorysetLaunchSiteName, toggleExpand
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterListOrder.cs
- 166 lines; kind=MonoBehaviour; type=MEGUIParameterListOrder; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine, UnityEngine.UI
- unity: Transform=7, SetActive=6, GameObject=4, UnityEngine.UI=2, Lifecycle=2, GetComponent=2; unity_score=70
- ksp refs: none; fan_in=0 fan_out=3
- methods(9): CleanList, DownButton, FillList, GetTitle, OnDisable, OnEnable, Setup, ToggleExpand, UpButton
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterMissionInstructor.cs
- 132 lines; kind=MonoBehaviour; type=MEGUIParameterMissionInstructor; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, UnityEngine
- unity: GameObject=11, SetActive=2, GetComponent=1; unity_score=61
- ksp refs: Localizer=4; fan_in=1 fan_out=10
- methods(6): DisplayGAP, OnParameterValueChanged, RefreshGapPrefab, SetGAPKerbal, Setup, onVintageSuitChanged
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterMissionKerbal.cs
- 105 lines; kind=MonoBehaviour; type=MEGUIParameterMissionKerbal; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: Expansions.Serenity, UnityEngine
- unity: GameObject=3; unity_score=34
- ksp refs: ProtoCrewMember=6; fan_in=2 fan_out=10
- methods(6): Display, DisplayGAP, OnParameterValueChanged, RefreshGapPrefab, SetGAPKerbal, Setup
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterNodeDropdownList.cs
- 148 lines; kind=MonoBehaviour; type=MEGUIParameterNodeDropdownList; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro
- unity: TMPro=4; unity_score=39
- ksp refs: ConfigNode=1; fan_in=2 fan_out=9
- methods(8): Display, GetNodeList, OnHistoryValueChange, OnParameterValueChanged, RefreshNodeList, RefreshUI, ResetDefaultValue, Setup
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterNodeLabelNodeDropdownList.cs
- 28 lines; kind=MonoBehaviour; type=MEGUIParameterNodeLabelNodeDropdownList; bases=MEGUIParameterNodeDropdownList; ancestry=MEGUIParameterNodeDropdownList/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=1 fan_out=5
- methods(2): GetNodeList, Setup
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterNumberRange.cs
- 432 lines; kind=MonoBehaviour; type=MEGUIParameterNumberRange; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=10, SetActive=8, UnityEngine.UI=7, TMPro=4, Input.=3, EventSystems=3; unity_score=66
- ksp refs: Localizer=1, ConfigNode=1; fan_in=10 fan_out=9
- methods(17): EnableSlider, OnButtonClicked, OnHistoryValueChange, OnInputFieldLoseFocus, OnInputHoverEnd, OnInputHoverStart, OnSAPScroll, OnSliderMouserUp, OnSliderValueChange, RefreshUI, ResetDefaultValue, SetUnitsText, Setup, Toggle, Update, UpdateSlider, UpdateValueText
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterPartPicker.cs
- 146 lines; kind=MonoBehaviour; type=MEGUIParameterPartPicker; bases=MEGUIParameter,IMEHistoryTarget; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, TMPro=1; unity_score=30
- ksp refs: ConfigNode=4, Localizer=3, AvailablePart=1; fan_in=5 fan_out=12
- methods(9): DisplayGAP, GetState, OnHistoryValueChange, OnPartPickerOpen, OnPartSelectionChange, RefreshUI, ResetDefaultValue, Setup, UpdateDisplayedParts
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterQuaternion.cs
- 82 lines; kind=MonoBehaviour; type=MEGUIParameterQuaternion; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: UnityEngine.UI=3; unity_score=47
- ksp refs: none; fan_in=1 fan_out=4
- methods(5): OnValueChange, OnValueChanged, SetQuaternion, Setup, ToggleEulerAxis
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParametersController.cs
- 90 lines; kind=MonoBehaviour; type=MEGUIParametersController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: Lifecycle=3, GameObject=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=59
- ksp refs: none; fan_in=5 fan_out=2
- methods(5): Awake, GetControl, OnDestroy, SetupParameterControls, Start
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterScoreRangeList.cs
- 89 lines; kind=MonoBehaviour; type=MEGUIParameterScoreRangeList; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro, UnityEngine.UI
- unity: Transform=4, TMPro=3, UnityEngine.UI=1; unity_score=62
- ksp refs: none; fan_in=0 fan_out=6
- methods(4): AddScoreRange, OnAddButton, OnRemoveScoreRange, Setup
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterScrollViewSelector.cs
- 137 lines; kind=MonoBehaviour; type=MEGUIParameterScrollViewSelector; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=11, UnityEngine.UI=4, Transform=4, SetActive=4, GetComponent=2, TMPro=1; unity_score=79
- ksp refs: none; fan_in=0 fan_out=3
- methods(9): ButtonCallback, ExpandEvent, FillList, IsExpanded, OnExpandView, OnSelectedElement, SelectedElementEvent, Setup, ToggleExpand
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterSwitchCompound.cs
- 120 lines; kind=MonoBehaviour; type=MEGUIParameterSwitchCompound; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: SetActive=3, GameObject=3, Transform=2; unity_score=51
- ksp refs: none; fan_in=1 fan_out=9
- methods(3): Display, OnControlToDisplaySet, Setup
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterTextArea.cs
- 137 lines; kind=MonoBehaviour; type=MEGUIParameterTextArea; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, TMPro, UnityEngine
- unity: TMPro=2, UnityEngine.UI=1, Transform=1, GameObject=1; unity_score=43
- ksp refs: Localizer=9, GameEvents=1, ConfigNode=1; fan_in=4 fan_out=10
- methods(9): LockLocalizedText, OnHistoryValueChange, OnMouseOver, OnParameterEndEdit, OnParameterValueChanged, RefreshUI, ResetDefaultValue, Setup, onLocalizationLockOverriden
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterTime.cs
- 54 lines; kind=MonoBehaviour; type=MEGUIParameterTime; bases=MEGUIParameter; ancestry=MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: UnityEngine.UI=1, Transform=1, GameObject=1; unity_score=49
- ksp refs: ConfigNode=1; fan_in=1 fan_out=7
- methods(3): OnHistoryValueChange, OnValueChange, Setup
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterVessel.cs
- 72 lines; kind=MonoBehaviour; type=MEGUIParameterVessel; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: GameObject=1; unity_score=25
- ksp refs: Part=1; fan_in=3 fan_out=9
- methods(5): OnChangePartSelection, OnNextVessel, OnPrevVessel, SetMission, Setup
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterVesselDropdownList.cs
- 248 lines; kind=MonoBehaviour; type=MEGUIParameterVesselDropdownList; bases=MEGUIParameterVessel; ancestry=MEGUIParameterVessel/MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, TMPro
- unity: TMPro=1, Lifecycle=1; unity_score=22
- ksp refs: GameEvents=8, Localizer=5, ConfigNode=1; fan_in=7 fan_out=14
- methods(15): Display, DisplayGAP, OnDestroy, OnHistoryValueChange, OnNextVessel, OnParameterValueChanged, OnPrevVessel, OverrideDefaultValue, RefreshUI, ResetDefaultValue, SetCraftOnly, SetDropdownValues, Setup, onBuilderNodeAdded, onBuilderNodeDeleted
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterVesselLocation.cs
- 305 lines; kind=MonoBehaviour; type=MEGUIParameterVesselLocation; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine, UnityEngine.UI
- unity: SetActive=25, GameObject=25, UnityEngine.UI=4, Transform=1, Lifecycle=1; unity_score=75
- ksp refs: ConfigNode=4, FlightGlobals=1, CelestialBody=1; fan_in=1 fan_out=17
- methods(10): Create, Display, DropDownAction_Facility, DropDownAction_LaunchSite, DropDownAction_Situation, GetState, OnEnable, OnHistoryUpdateDropdowns, Setup, setSituationType
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterVesselPartSelector.cs
- 238 lines; kind=MonoBehaviour; type=MEGUIParameterVesselPartSelector; bases=MEGUIParameterVessel; ancestry=MEGUIParameterVessel/MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization
- unity: Lifecycle=2; unity_score=21
- ksp refs: Localizer=6, GameEvents=4, ConfigNode=4, Part=3, ShipConstruct=1, HighLogic=1; fan_in=3 fan_out=17
- methods(17): Awake, DisplayGAP, GetSelectedPart, GetState, LoadSelectedPart, OnChangePartSelection, OnDestroy, OnHistoryValueChange, OnNextVessel, OnPrevVessel, OnVesselValueChange, RefreshUI, ResetDefaultValue, SetFieldValues, Setup, UpdateSelectedPartInDisplay, onVesselSituationChanged
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIParameterVesselSituation.cs
- 550 lines; kind=MonoBehaviour; type=MEGUIParameterVesselSituation; bases=MEGUICompoundParameter; ancestry=MEGUICompoundParameter/MEGUIParameter/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: Expansions.Missions.Actions, Expansions.Missions.Runtime, KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=23, SetActive=22, UnityEngine.UI=14, Transform=8, TMPro=6, GetComponent=2; unity_score=71
- ksp refs: Localizer=13, ConfigNode=9, GameEvents=6, FlightGlobals=4, MissionSystem=3, ProtoCrewMember=2; fan_in=1 fan_out=35
- methods(21): ButtonAction_CreateVessel, ButtonAction_ResetVessel, ButtonAction_SetCrew, CreateVessel_SaveCompleted, DelegateCallback, Display, DropDownAction_Vessels, GetState, InputTextAction_SetVesselName, OnDestroy, OnHistorySetCrew, Setup, ToggleAction_AutoPopulateCrew, ToggleAction_PlayerBuilt, onCreateVesselMissionSetupSuccess, onHistoryDropDownVessels, onHistoryResetVessel, onHistoryTogglePlayerBuilt, onHistorysetVesselName, setupVessels, toggleExpand
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIScoreRangeItem.cs
- 149 lines; kind=MonoBehaviour; type=MEGUIScoreRangeItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=15, Transform=2, Instantiate/Destroy=2, UnityEngine.UI=1, MonoBehaviour=1, GameObject=1; unity_score=77
- ksp refs: none; fan_in=2 fan_out=3
- methods(11): Create, OnInputValueChange, OnMaxRangeInputEndEdit, OnMinRangeInputEndEdit, OnRemoveButton, OnScoreInputEndEdit, OnSelectMaxRangeInput, OnSelectMinRangeInput, OnValidatePercentageInput, Setup, Terminate
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUIScoreRangeTimeItem.cs
- 34 lines; kind=MonoBehaviour; type=MEGUIScoreRangeTimeItem; bases=MEGUIScoreRangeItem; ancestry=MEGUIScoreRangeItem/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=1 fan_out=3
- methods(3): OnMaxRangeTimeValueChange, OnMinRangeTimeValueChange, Setup
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEGUITimeControl.cs
- 203 lines; kind=MonoBehaviour; type=MEGUITimeControl; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: TMPro, UnityEngine, UnityEngine.Events, UnityEngine.UI
- unity: TMPro=7, GameObject=6, SetActive=5, UnityEngine.UI=2, Lifecycle=2, MonoBehaviour=1; unity_score=70
- ksp refs: none; fan_in=3 fan_out=3
- methods(7): Awake, EditTime, Start, UpdateButtonText, UpdateTextValues, UpdateTimeFromInput, UpdateValues
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MENodeCanvas.cs
- 543 lines; kind=MonoBehaviour; type=MENodeCanvas; bases=MonoBehaviour,IScrollHandler,IEventSystemHandler; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.UI, UnityEngine, UnityEngine.EventSystems
- unity: Transform=33, Screen/Cursor=18, EventSystems=13, UnityEngine.UI=4, Camera=4, Lifecycle=3; unity_score=72
- ksp refs: ConfigNode=4, GameSettings=2, GameEvents=2; fan_in=5 fan_out=7
- methods(35): AdjustOrthographicsSizeAndCameraPosition, Awake, CalculateBorders, CheckSnap, CheckZoomBoundries, FitCameraToArea, FitCameraToSelectedNodes, FocusNode, FocusStartNode, GetCanvasDragPosition, GetClampedPosition, GetMousePointOnGrid, GetNodeBounds, Initialize, Load, OnDestroy, OnDrag, OnMissionLoaded, OnPointerClick, OnPointerDown, OnPointerUp, OnScroll, PointerInsideCanvasView, Save, ScrollDir, SetZoom, StopCanvasMovement, ToggleScrollLock, TryCanvasMovement, Update, UpdateTextPercentage, Zoom, ZoomIn, ZoomOut, zoomChanged
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MENodeCanvasBackground.cs
- 31 lines; kind=MonoBehaviour; type=MENodeCanvasBackground; bases=MonoBehaviour,IDragHandler,IEventSystemHandler,IPointerDownHandler,IPointerUpHandler,IPointerClickHandler; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=8, MonoBehaviour=1; unity_score=93
- ksp refs: none; fan_in=0 fan_out=1
- methods(4): OnDrag, OnPointerClick, OnPointerDown, OnPointerUp
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MENodeCategorizer.cs
- 672 lines; kind=MonoBehaviour; type=MENodeCategorizer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens, RUI.Icons.Selectable, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Transform=34, GameObject=21, SetActive=15, Instantiate/Destroy=5, GetComponent=4, Coroutine=3; unity_score=69
- ksp refs: Localizer=4, ConfigNode=4, InputLockManager=3, GameDatabase=2; fan_in=6 fan_out=17
- methods(31): AddCanvasNodeButton, Awake, ClearNodesInCategory, ClearSearchField, ContainsCategoryConfig, CreateSearchToolboxGroups, DeselectLastCategoryButton, DisplayNodesInCategory, DisplayStoredIcon, EnableDrag, FocusSearchField, GetCategoryImage, GetDisplayedNodeNames, GetNodes, HighLightDisplayedNodeIcon, HighLightSelectedSearchResults, HighlightNodes, HightLightCategoryButton, HightLightNodeIcon, NodeMatchesSearch, RefreshCurrentDisplay, RemoveCanvasNodeButton, RemoveSearchToolboxGroups, ResetStoredIcons, SearchField_OnClick, SearchField_OnEndEdit, SearchField_OnValueChange, SearchRoutine, SearchStart, SearchStop, Start
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MENodeCategoryButton.cs
- 101 lines; kind=MonoBehaviour; type=MENodeCategoryButton; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.UI.TooltipTypes, RUI.Icons.Selectable, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, Texture=2, SetActive=1, MonoBehaviour=1, GameObject=1; unity_score=66
- ksp refs: none; fan_in=1 fan_out=2
- methods(6): DeselectCategory, OnMouseClick, SelectCategory, SetIcon, Setup, ToggleHighlighter
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MENodeCategoryColor.cs
- 13 lines; kind=class; type=MENodeCategoryColor; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: none; unity_score=17
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MENodeColors.cs
- 16 lines; kind=class; type=MENodeColors; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: none; unity_score=17
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MENodeConnectionType.cs
- 8 lines; kind=enum; type=MENodeConnectionType; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEPartCategorizer.cs
- 228 lines; kind=MonoBehaviour; type=MEPartCategorizer; bases=BasePartCategorizer; ancestry=BasePartCategorizer/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.UI, KSP.UI.Screens, RUI.Icons.Selectable, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=7, SetActive=6, GetComponent=6, EventSystems=5, KeyCode=4, Input.=4; unity_score=79
- ksp refs: AvailablePart=21, PartLoader=1; fan_in=1 fan_out=10
- methods(14): Awake, InstantiatePartCategorizerButton, OnClick, OnDestroy, OnFalse, OnPointerDown, OnPointerUp, OnTrue, PartSelected, Reset, ScrollWithButtons, SearchFilterResult, SelectedPartsOfCategory, Setup
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEPartCategorizerButton.cs
- 10 lines; kind=MonoBehaviour; type=MEPartCategorizerButton; bases=PartCategorizerButton; ancestry=PartCategorizerButton/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.UI.Screens, UnityEngine.UI
- unity: UnityEngine.UI=1; unity_score=44
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEPartSelectorBrowser.cs
- 322 lines; kind=MonoBehaviour; type=MEPartSelectorBrowser; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, KSP.UI, KSP.UI.Screens.Editor, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=8, GameObject=7, Transform=6, SetActive=5, Instantiate/Destroy=3, Lifecycle=2; unity_score=67
- ksp refs: AvailablePart=25, PartLoader=1, Localizer=1; fan_in=4 fan_out=13
- methods(23): AddPartIcon, Awake, BuildPartList, CancelledCallback, ClearPartList, DisablePart, Dismiss, Hide, OnDestroy, OnFilterSelectionChange, OnPartListUpdate, OnPartSelectionChange, OnSearchChange, PartSelectionCallback, SelectionCallback, SetPartSelectionStatus, Setup, Show, SortingCallback, Spawn, Start, onButtonCancel, onButtonSelect
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEPartSelectorEntry.cs
- 46 lines; kind=MonoBehaviour; type=MEPartSelectorEntry; bases=EditorPartIcon; ancestry=EditorPartIcon/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.UI.Screens, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, GetComponent=2, GameObject=1; unity_score=72
- ksp refs: AvailablePart=2; fan_in=1 fan_out=4
- methods(3): Create, OnPartSelectorClick, UpdateSelection
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MEValidityHighlighColor.cs
- 13 lines; kind=class; type=MEValidityHighlighColor; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: none; unity_score=17
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionBiome.cs
- 94 lines; kind=class; type=MissionBiome; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: FlightGlobals=5, Localizer=3, ConfigNode=2, CelestialBody=2; fan_in=11 fan_out=7
- methods(7): Equals, GetHashCode, GetNodeBodyParameterString, Load, Save, SetBiomes, SetCelestialBodies
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionBriefingDialog.cs **MUST-OPEN (>800 lines)**
- 1257 lines; kind=MonoBehaviour; type=MissionBriefingDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: Expansions.Missions.Flow, KSP.Localization, KSP.UI, KSP.UI.TooltipTypes, KSP.UI.Util, Steamworks, TMPro, UnityEngine, UnityEngine.UI
- unity: Analytics=40, Steam=31, UnityEngine.UI=30, GameObject=22, TMPro=19, SetActive=12; unity_score=79
- ksp refs: Localizer=63, HighLogic=5, GameEvents=5, GameDatabase=3, GameSettings=1; fan_in=4 fan_out=50
- methods(63): DestroyDialog, DisableExportButon, Display, ExportMissionAfterChecksPassed, ExportMissionNameCancelled, ExportMissionNameConfirmed, GetTempBanner, Hide, LoadMissionPacks, LocalizationLock, LocalizationLockAuthor, LocalizationLockBriefing, LocalizationLockTitle, OnAddTag, OnAuthorTextChanged, OnBannerCancel, OnBannerDeleted, OnBannerSelected, OnBriefTextChanged, OnCancel, OnDestroy, OnDifficultySliderChanged, OnEndEdit, OnExitAfterSave, OnExport, OnFlagCancel, OnFlagSelect, OnMissionTagRemoved, OnModsList, OnOK, OnOpenBannerFail, OnOpenBannerMenu, OnOpenBannerSuccess, OnOpenflag, OnTabAwardsPress, OnTabInfoPress, OnTabScorePress, OnTabValidationPress, OnTitleTextChanged, OpenBannerSelector, RefreshBanners, SetTab, SetTabCaption, SetTabVisible, SetTempBanner ...
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionEditorHistory.cs
- 269 lines; kind=MonoBehaviour; type=MissionEditorHistory; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: MonoBehaviour=5, Instantiate/Destroy=3, Time.=2, GameObject=2, Lifecycle=1; unity_score=44
- ksp refs: ConfigNode=2; fan_in=27 fan_out=7
- methods(11): ActionCallback, Awake, Clear, ClearRedoActions, Peek, Pop, Push, PushUndoAction, Redo, SelectAffectedObject, Undo
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionEditorLogic.cs **MUST-OPEN (>800 lines)**
- 2639 lines; kind=MonoBehaviour; type=MissionEditorLogic; bases=MonoBehaviour,IMEHistoryTarget; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: Expansions.Missions.Actions, Expansions.Missions.Flow, Expansions.Missions.Runtime, KSP.Localization, KSP.UI, KSP.UI.Screens, KSP.UI.TooltipTypes, KSPAssets.KSPedia, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=39, KeyCode=23, Input.=23, EventSystems=14, GetComponent=13, GameObject=12; unity_score=56
- ksp refs: Localizer=63, InputLockManager=28, GameEvents=23, ConfigNode=22, GameSettings=13, HighLogic=11; fan_in=87 fan_out=80
- methods(149): AddConnectorToConnectorList, AddConnectorToSelectedList, AddNode, AddNodeToSelectedList, ArrangeGraphNodes, Awake, CanvasClicked, CheckDialogsBeforeExiting, CheckpointTestMissionSetupSuccess, CheckpointToLoadSelected, ClearAllSelections, ClearConnector, ClearConnectorGroupSelection, ClearEditor, ClearSelectedNodesList, ConnectNodes, ConnectorButtonClicked, ConnectorButtonDragged, ConnectorButtonDropped, ConnectorSelectionChange, ContinueToTrackingStation, CopyMissionBanners, CopySelectedNodes, CreateEmptyMission, CreateNodeAtMousePosition, CreateNodeAtPosition, DestroyCurrentConnector, DisplayMultipleSelectedItems, FilterSideBarNode, FitNodesInView, GetCategoryColor, GetDisplayedNodeNames, GetDisplayedNodes, GetGridMousePosition, GetNodeFromID, GetNodeListCout, GetOnClickOnSearchListener, GetParentParameterGroup, GetStartNodeColor, GetState, GetToolboxWidth, HideTutorialIndicator, HighLightDisplayedNode, InputUpdate, IsNodeDockable ...
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionEditorMapView.cs
- 27 lines; kind=MonoBehaviour; type=MissionEditorMapView; bases=MapView; ancestry=MapView/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: Lifecycle=1, Camera=1; unity_score=55
- ksp refs: MapView=1; fan_in=3 fan_out=2
- methods(2): Awake, CreateMapFXCamera
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionEditorValidator.cs
- 265 lines; kind=MonoBehaviour; type=MissionEditorValidator; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=31
- ksp refs: Localizer=1, GameSettings=1; fan_in=32 fan_out=10
- methods(13): AddNodeFail, AddNodePass, AddNodeResult, AddNodeWarn, AddResults, Awake, GetStatusColor, GetValidationColor, NextMode, ResetValidator, RunValidation, RunValidationOnParamChange, Start
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionsBrowserDialog.cs
- 541 lines; kind=MonoBehaviour; type=MissionsBrowserDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: Expansions.Missions.Runtime, KSP.Localization, KSP.UI.Screens, KSP.UI.Util, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=10, GameObject=10, Transform=8, Texture=5, Coroutine=5, SetActive=4; unity_score=67
- ksp refs: Localizer=20, GameSettings=3, GameEvents=2, MissionSystem=1; fan_in=1 fan_out=35
- methods(25): BuildMissionList, BuildMissionListRoutine, BuildPacksList, CancelledCallback, ClearListItems, ClearSelection, ConfirmLoadGame, CreateWidget, Dismiss, DisplayAddedMissions, Hide, OnDeleteConfirm, OnSelectionChanged, OnTabStock, OnTabUser, PromptDeleteFileConfirm, SelectFileCallback, Show, Spawn, Start, Update, onButtonCancel, onButtonDelete, onButtonLoad, onMissionImported
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionValidationDialog.cs
- 172 lines; kind=MonoBehaviour; type=MissionValidationDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.UI, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, Transform=3, Lifecycle=3, Instantiate/Destroy=3, GameObject=3, TMPro=2; unity_score=74
- ksp refs: GameSettings=4; fan_in=1 fan_out=11
- methods(11): BuildDropDown, ClearValidationResults, DestroyDialog, Display, OnButtonValidate, OnCancel, OnDestroy, OnOK, PrintValidationResults, Start, Update
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionValidationEntry.cs
- 63 lines; kind=MonoBehaviour; type=MissionValidationEntry; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: Transform=3, TMPro=3, UnityEngine.UI=2, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=81
- ksp refs: Localizer=1; fan_in=1 fan_out=7
- methods(4): Awake, Create, OnToggleValueChange, Start
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/MissionValidationTestResult.cs
- 17 lines; kind=class; type=MissionValidationTestResult; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/NodeListTooltip.cs
- 188 lines; kind=MonoBehaviour; type=NodeListTooltip; bases=Tooltip; ancestry=Tooltip/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.UI, KSP.UI.Screens.Editor, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=11, SetActive=10, UnityEngine.UI=8, Transform=5, TMPro=4, Instantiate/Destroy=4; unity_score=84
- ksp refs: none; fan_in=2 fan_out=3
- methods(8): CreateExtInfoElement, CreateExtendedInfo, DisplayExtendedInfo, GetNewTooltipSpacer, GetNewTooltipWidget, HidePreviousTooltipWidgets, OnDestroy, Setup
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/NodeListTooltipController.cs
- 83 lines; kind=MonoBehaviour; type=NodeListTooltipController; bases=PinnableTooltipController; ancestry=PinnableTooltipController/TooltipController/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: KSP.Localization, KSP.UI, UnityEngine, UnityEngine.EventSystems
- unity: UnityEngine.UI=4, EventSystems=2, Transform=1, GetComponent=1; unity_score=67
- ksp refs: Localizer=3; fan_in=2 fan_out=8
- methods(5): CreateTooltip, GetTooltipHintText, OnPointerClick, OnTooltipDespawned, OnTooltipSpawned
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/NodeListTooltipMasterController.cs
- 36 lines; kind=MonoBehaviour; type=NodeListTooltipMasterController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1; unity_score=68
- ksp refs: none; fan_in=1 fan_out=1
- methods(2): Awake, OnDestroy
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/OrbitGizmo.cs
- 216 lines; kind=MonoBehaviour; type=OrbitGizmo; bases=ManeuverGizmoBase; ancestry=ManeuverGizmoBase/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: SetActive=4, Transform=3, GameObject=2, Lifecycle=1, Instantiate/Destroy=1, Input.=1; unity_score=50
- ksp refs: Vector3d=17; fan_in=3 fan_out=7
- methods(22): ChangeToGlobalMode, ChangeToPointMode, Create, HandlesUpdatedCallback, OnAntiSMAUpdate, OnAntinormalUpdate, OnEccInUpdate, OnEccOutUpdate, OnGlobalModeButtonPress, OnIncClockwiseUpdate, OnIncCounterUpdate, OnMouseDrag, OnMouseUp, OnNormalUpdate, OnPointModeButtonPress, OnProgradeUpdate, OnRadialInUpdate, OnRadialOutUpdate, OnResetButtonPress, OnRetrogradeUpdate, OnSMAUpdate, Start
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/OrbitGizmoMode.cs
- 9 lines; kind=enum; type=OrbitGizmoMode; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/SpriteButton.cs
- 81 lines; kind=MonoBehaviour; type=SpriteButton; bases=MouseRayEventsHandler; ancestry=MouseRayEventsHandler/MouseEventsHandlerBase/MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine, UnityEngine.Events, UnityEngine.UI
- unity: Texture=1, Renderer=1, Lifecycle=1; unity_score=43
- ksp refs: none; fan_in=1 fan_out=1
- methods(5): Awake, OnMouseDown, OnMouseEnter, OnMouseExit, OnMouseUp
- auto suggestion: tier=B subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/SurfaceLocation.cs
- 11 lines; kind=struct; type=SurfaceLocation; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/UICollapsibleSection.cs
- 31 lines; kind=MonoBehaviour; type=UICollapsibleSection; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Expansions.Missions.Editor
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=6, SetActive=3, GameObject=3, Texture=2, MonoBehaviour=1, Lifecycle=1; unity_score=99
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): OnButtonCollapsePressed, Start
- auto suggestion: tier=C subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/ValidationColors.cs
- 17 lines; kind=class; type=ValidationColors; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: UnityEngine
- unity: none; unity_score=21
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/ValidationStatus.cs
- 16 lines; kind=enum; type=ValidationStatus; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory

## Assembly-CSharp/Expansions/Missions/Editor/ValidatorMode.cs
- 16 lines; kind=enum; type=ValidatorMode; bases=-; ancestry=-; ns=Expansions.Missions.Editor
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=DLC-MakingHistory
