# Review chunk 10 (Career): 178 files, 38994 lines

## Assembly-CSharp/CommNet/CommLink.cs
- 107 lines; kind=class; type=CommLink; bases=Link<CommNetwork,CommNode,CommLink,CommPath>; ancestry=Link; ns=CommNet
- usings: CommNet.Network
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=7
- methods(7): GetBestSignal, GetSignalStrength, Set, SetSignalStrength, ToString, Update, ZeroSignalStrength
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/CommNetBody.cs
- 44 lines; kind=MonoBehaviour; type=CommNetBody; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=CommNet
- usings: CommNet.Occluders, UnityEngine
- unity: Lifecycle=2, Transform=1, MonoBehaviour=1, GetComponent=1; unity_score=63
- ksp refs: HighLogic=3, GameEvents=2, CelestialBody=2; fan_in=0 fan_out=7
- methods(5): CreateOccluder, OnDestroy, OnNetworkInitialized, OnNetworkPreUpdate, Start
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/CommNet/CommNetHome.cs
- 106 lines; kind=MonoBehaviour; type=CommNetHome; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=CommNet
- usings: UnityEngine
- unity: Transform=4, Lifecycle=3, MonoBehaviour=1, GetComponent=1; unity_score=55
- ksp refs: HighLogic=3, GameEvents=2, CelestialBody=2; fan_in=1 fan_out=9
- methods(6): CreateNode, OnDestroy, OnNetworkInitialized, OnNetworkPreUpdate, Start, Update
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/CommNet/CommNetNetwork.cs
- 167 lines; kind=MonoBehaviour; type=CommNetNetwork; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=CommNet
- usings: UnityEngine
- unity: Lifecycle=3, Time.=1, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=41
- ksp refs: GameEvents=5, FlightGlobals=2, Vessel=1, HighLogic=1; fan_in=8 fan_out=9
- methods(10): Add, Awake, DebugInfo, OnDestroy, OnMapFocusChange, QueueRebuild, Remove, Reset, ResetNetwork, Update
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/CommNet/CommNetNode.cs
- 64 lines; kind=MonoBehaviour; type=CommNetNode; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=CommNet
- usings: UnityEngine
- unity: Lifecycle=2, Transform=1, MonoBehaviour=1; unity_score=48
- ksp refs: GameEvents=2; fan_in=0 fan_out=3
- methods(5): OnDestroy, OnNetworkInitialized, OnNetworkPostUpdate, OnNetworkPreUpdate, Start
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/CommNet/CommNetParams.cs
- 74 lines; kind=class; type=CommNetParams; bases=GameParameters.CustomParameterNode; ancestry=CustomParameterNode/ParameterNode/IConfigNode; ns=CommNet
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=2; fan_in=5 fan_out=2
- methods(2): Interactible, SetDifficultyPreset
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/CommNetScenario.cs
- 103 lines; kind=ScenarioModule; type=CommNetScenario; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=CommNet
- usings: UnityEngine
- unity: Instantiate/Destroy=4, GameObject=3, Lifecycle=2, GetComponent=2; unity_score=62
- ksp refs: HighLogic=2, ConfigNode=2, ScenarioModule=1; fan_in=18 fan_out=10
- methods(5): OnAwake, OnDestroy, OnLoad, OnSave, Start
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/CommNet/CommNetUI.cs
- 479 lines; kind=MonoBehaviour; type=CommNetUI; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=CommNet
- usings: KSP.Localization, UnityEngine, Vectrosity
- unity: Shader=8, Material=5, Texture=4, Instantiate/Destroy=4, Lifecycle=3, Resources.=2; unity_score=56
- ksp refs: Localizer=6, GameEvents=6, FlightGlobals=4, Vessel=3, MapView=2, ScaledSpace=1; fan_in=5 fan_out=21
- methods(15): Awake, ClampAndSetMode, CreateLine, Hide, NextMode, OnDestroy, OnMapFocusChange, PreviousMode, ResetMode, SetDisplayVessel, Show, Start, SwitchMode, UpdateDisplay, UpdateLine
- auto suggestion: tier=C subsystem=CommNet

## Assembly-CSharp/CommNet/CommNetUIModeButton.cs
- 79 lines; kind=MonoBehaviour; type=CommNetUIModeButton; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler; ancestry=MonoBehaviour; ns=CommNet
- usings: KSP.Localization, KSP.UI, KSP.UI.TooltipTypes, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Lifecycle=5, EventSystems=5, SetActive=2, GameObject=2, UnityEngine.UI=1, MonoBehaviour=1; unity_score=81
- ksp refs: GameEvents=2, Localizer=1; fan_in=1 fan_out=5
- methods(8): Awake, OnDestroy, OnEnable, OnNetworkInitialized, OnPointerClick, Start, Update, UpdateUI
- auto suggestion: tier=C subsystem=CommNet

## Assembly-CSharp/CommNet/CommNetVessel.cs
- 599 lines; kind=VesselModule; type=CommNetVessel; bases=VesselModule; ancestry=VesselModule/MonoBehaviour; ns=CommNet
- usings: Experience.Effects, UnityEngine
- unity: GameObject=3, Lifecycle=2, Transform=1, Instantiate/Destroy=1; unity_score=24
- ksp refs: PhysicsGlobals=12, GameEvents=6, Vessel=5, HighLogic=4, Part=3, PartModule=2; fan_in=7 fan_out=32
- methods(18): CalculatePlasmaMult, CreateControlConnection, FindCommandSources, GetBestTransmitter, GetControlLevel, GetSignalStrengthModifier, OnAwake, OnDestroy, OnMapFocusChange, OnNetworkInitialized, OnNetworkPostUpdate, OnNetworkPreUpdate, OnStart, RegisterCommandSource, UnregisterCommandSource, Update, UpdateComm, UpdateControlState
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/CommNet/CommNetwork.cs
- 459 lines; kind=class; type=CommNetwork; bases=Net<CommNetwork,CommNode,CommLink,CommPath>; ancestry=Net; ns=CommNet
- usings: CommNet.Network
- unity: Raycast/Query=1; unity_score=3
- ksp refs: Vector3d=4; fan_in=6 fan_out=7
- methods(16): Add, CreateDebug, CreateShortestPathTree, FindClosestControlSource, FindClosestWhere, FindHome, FindPath, IsControlSource, IsHome, Rebuild, Remove, SetNodeConnection, TestOcclusion, TryConnect, UpdateShortestPath, UpdateShortestWhere
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/CommNet/CommNode.cs
- 147 lines; kind=class; type=CommNode; bases=Node<CommNetwork,CommNode,CommLink,CommPath>; ancestry=Node/Dictionary; ns=CommNet
- usings: CommNet.Network, UnityEngine
- unity: Transform=9; unity_score=35
- ksp refs: Vector3d=3; fan_in=9 fan_out=5
- methods(5): GetSignalStrengthMultiplier, NetworkPostUpdate, NetworkPreUpdate, ToString, Update
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/CommNet/CommPath.cs
- 71 lines; kind=class; type=CommPath; bases=Path<CommNetwork,CommNode,CommLink,CommPath>; ancestry=Path/List; ns=CommNet
- usings: CommNet.Network
- unity: none; unity_score=0
- ksp refs: none; fan_in=8 fan_out=8
- methods(4): Clear, IsLastHopHome, ToString, UpdateFromPath
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/CommRangeModel.cs
- 27 lines; kind=class; type=CommRangeModel; bases=IRangeModel; ancestry=IRangeModel; ns=CommNet
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): GetMaximumRange, GetNormalizedRange, InRange
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/HopType.cs
- 9 lines; kind=enum; type=HopType; bases=-; ancestry=-; ns=CommNet
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/ICommNetControlSource.cs
- 13 lines; kind=interface; type=ICommNetControlSource; bases=-; ancestry=-; ns=CommNet
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/IRangeModel.cs
- 11 lines; kind=interface; type=IRangeModel; bases=-; ancestry=-; ns=CommNet
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/IRelayEnabler.cs
- 9 lines; kind=interface; type=IRelayEnabler; bases=-; ancestry=-; ns=CommNet
- usings: -
- unity: none; unity_score=0
- ksp refs: ProtoPartModuleSnapshot=1; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/ModuleProbeControlPoint.cs
- 93 lines; kind=PartModule; type=ModuleProbeControlPoint; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=CommNet
- usings: Expansions.Missions.Adjusters, KSP.Localization
- unity: none; unity_score=15
- ksp refs: Localizer=4, KSPField=2, ProtoPartModuleSnapshot=1, PartModule=1; fan_in=3 fan_out=6
- methods(8): CacheLocalStrings, CanControl, CanControlUnloaded, GetInfo, GetModuleDisplayName, IsProbeControlPointBroken, OnModuleAdjusterAdded, OnModuleAdjusterRemoved
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/ModuleRelayEnabler.cs
- 99 lines; kind=PartModule; type=ModuleRelayEnabler; bases=PartModule,IResourceConsumer,IRelayEnabler; ancestry=PartModule/MonoBehaviour; ns=CommNet
- usings: KSP.Localization
- unity: Lifecycle=1; unity_score=22
- ksp refs: PartResourceDefinition=3, KSPField=2, ProtoPartModuleSnapshot=1, PartModule=1, Localizer=1, HighLogic=1; fan_in=0 fan_out=10
- methods(8): CanRelay, CanRelayUnloaded, FixedUpdate, GetConsumedResources, GetInfo, OnAwake, OnSave, OnStart
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/Network/Link.cs
- 83 lines; kind=class; type=Link; bases=-; ancestry=-; ns=CommNet.Network
- usings: KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=10
- ksp refs: Localizer=2; fan_in=0 fan_out=2
- methods(7): Contains, GetPoints, OnDestroy, OtherEnd, Set, ToString, Update
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/Network/Net.cs
- 419 lines; kind=class; type=Net; bases=-; ancestry=-; ns=CommNet.Network
- usings: UnityEngine
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=1
- methods(19): Add, Connect, Contains, CreateShortestPathTree, Disconnect, FindClosestWhere, FindPath, GetLinkPoints, IncrementPathingID, PostUpdateNodes, PreUpdateNodes, Rebuild, Remove, SetNodeConnection, ToString, UpdateNetwork, UpdateOccluders, UpdateShortestPath, UpdateShortestWhere
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/Network/Node.cs
- 132 lines; kind=class; type=Node; bases=Dictionary<_Data,_Link>; ancestry=Dictionary; ns=CommNet.Network
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=1
- ksp refs: Vector3d=1, Localizer=1; fan_in=0 fan_out=4
- methods(6): GetLinkPoints, NetworkPostUpdate, NetworkPreUpdate, SetNet, SetOccluder, ToString
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/Network/Path.cs
- 185 lines; kind=class; type=Path; bases=List<_Link>,IConfigNode; ancestry=List; ns=CommNet.Network
- usings: UnityEngine
- unity: none; unity_score=1
- ksp refs: ConfigNode=2; fan_in=0 fan_out=3
- methods(8): CopyTo, Equals, GetHashCode, GetPoints, IsEmpty, Load, Save, UpdateFromPath
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/NodeUtilities.cs
- 26 lines; kind=static class; type=NodeUtilities; bases=-; ancestry=-; ns=CommNet
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(1): ConvertSignalStrength
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/Occluder.cs
- 22 lines; kind=class; type=Occluder; bases=-; ancestry=-; ns=CommNet
- usings: -
- unity: Raycast/Query=1, Lifecycle=1; unity_score=40
- ksp refs: Vector3d=4; fan_in=6 fan_out=1
- methods(3): InRange, Raycast, Update
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/CommNet/Occluders/OccluderHorizonCulling.cs
- 107 lines; kind=class; type=OccluderHorizonCulling; bases=Occluder; ancestry=Occluder; ns=CommNet.Occluders
- usings: UnityEngine
- unity: Transform=9, Raycast/Query=1, Lifecycle=1, GetComponent=1; unity_score=48
- ksp refs: Vector3d=6, QuaternionD=2, CelestialBody=2; fan_in=1 fan_out=4
- methods(3): InRange, Raycast, Update
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/CommNet/SignalStrength.cs
- 11 lines; kind=enum; type=SignalStrength; bases=-; ancestry=-; ns=CommNet
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=12 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/SignalStrengthColor.cs
- 11 lines; kind=enum; type=SignalStrengthColor; bases=-; ancestry=-; ns=CommNet
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/CommNet/Tooltip_SignalStrength.cs
- 13 lines; kind=MonoBehaviour; type=Tooltip_SignalStrength; bases=Tooltip; ancestry=Tooltip/MonoBehaviour; ns=CommNet
- usings: KSP.UI, TMPro, UnityEngine
- unity: UnityEngine.UI=1, TMPro=1; unity_score=59
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/CommNet/Tooltip_SignalStrengthItem.cs
- 22 lines; kind=MonoBehaviour; type=Tooltip_SignalStrengthItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=CommNet
- usings: KSP.UI, TMPro, UnityEngine
- unity: TMPro=1, MonoBehaviour=1; unity_score=55
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): Setup
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/CommNet/TooltipController_SignalStrength.cs
- 172 lines; kind=MonoBehaviour; type=TooltipController_SignalStrength; bases=TooltipController; ancestry=TooltipController/MonoBehaviour; ns=CommNet
- usings: KSP.Localization, KSP.UI, UnityEngine
- unity: Instantiate/Destroy=3, Transform=2, GameObject=2; unity_score=40
- ksp refs: Localizer=6, FlightGlobals=4, Vessel=1, CommNetVessel=1; fan_in=2 fan_out=11
- methods(9): CacheLocalStrings, ClearList, OnNetworkUpdate, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipSpawned, OnTooltipUpdate, UpdateList, UpdateTooltip
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/CommNet/VesselControlState.cs
- 17 lines; kind=enum; type=VesselControlState; bases=-; ancestry=-; ns=CommNet
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=8 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/Contracts/Agents/Agent.cs
- 290 lines; kind=class; type=Agent; bases=-; ancestry=-; ns=Contracts.Agents
- usings: KSP.Localization, UnityEngine
- unity: Texture=6; unity_score=34
- ksp refs: Contract=4, Localizer=3, GameDatabase=3, ConfigNode=1; fan_in=10 fan_out=10
- methods(6): CanProcessContract, GetMindsetString, HasExclusiveKeywords, LoadAgent, ProcessContract, ScoreAgentSuitability
- auto suggestion: tier=B subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/AgentList.cs
- 164 lines; kind=MonoBehaviour; type=AgentList; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Contracts.Agents
- usings: KSP.Localization, UnityEngine
- unity: Lifecycle=3, MonoBehaviour=1; unity_score=38
- ksp refs: Localizer=2, ConfigNode=2, GameDatabase=1, Contract=1, AvailablePart=1; fan_in=8 fan_out=6
- methods(9): Awake, GetAgent, GetAgentRandom, GetAgentbyTitle, GetPartManufacturer, GetSuitableAgentForContract, LoadAgents, OnDestroy, Start
- auto suggestion: tier=B subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/AgentMentality.cs
- 202 lines; kind=class; type=AgentMentality; bases=-; ancestry=-; ns=Contracts.Agents
- usings: UnityEngine
- unity: none; unity_score=9
- ksp refs: Contract=10; fan_in=20 fan_out=5
- methods(17): AddMentality, CanProcessContract, FactorDeadline, FactorExpiry, FactorFundsAdvance, FactorFundsCompletion, FactorFundsFailure, FactorReputationCompletion, FactorReputationFailure, FactorScienceCompletion, GenerateMentalityTypes, GetDescription, GetDisplayName, GetMentalityType, OnAdded, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/AgentStanding.cs
- 44 lines; kind=class; type=AgentStanding; bases=-; ancestry=-; ns=Contracts.Agents
- usings: UnityEngine
- unity: none; unity_score=27
- ksp refs: none; fan_in=1 fan_out=2
- methods(1): Link
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/KeywordScore.cs
- 12 lines; kind=enum; type=KeywordScore; bases=-; ancestry=-; ns=Contracts.Agents
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=20 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Commercial.cs
- 40 lines; kind=class; type=Commercial; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=9
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Competitive.cs
- 37 lines; kind=class; type=Competitive; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=10
- ksp refs: Contract=2, Localizer=1; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Conglomerate.cs
- 41 lines; kind=class; type=Conglomerate; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=9
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Cooperative.cs
- 37 lines; kind=class; type=Cooperative; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=10
- ksp refs: Contract=2, Localizer=1; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/EasyGoing.cs
- 39 lines; kind=class; type=EasyGoing; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=9
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Ecomental.cs
- 37 lines; kind=class; type=Ecomental; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=10
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Economic.cs
- 41 lines; kind=class; type=Economic; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=9
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Hasty.cs
- 40 lines; kind=class; type=Hasty; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=9
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Industrial.cs
- 37 lines; kind=class; type=Industrial; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=10
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Kerbalminded.cs
- 37 lines; kind=class; type=Kerbalminded; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=10
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Moral.cs
- 31 lines; kind=class; type=Moral; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=11
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=3
- methods(4): CanProcessContract, GetDescription, GetDisplayName, ProcessContract
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/NoRandomContractMentality.cs
- 37 lines; kind=class; type=NoRandomContractMentality; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=10
- ksp refs: Localizer=1, Contract=1; fan_in=0 fan_out=5
- methods(4): CanProcessContract, GetDisplayName, OnAdded, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Patient.cs
- 41 lines; kind=class; type=Patient; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=9
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Perfectionist.cs
- 37 lines; kind=class; type=Perfectionist; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=10
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Pioneer.cs
- 37 lines; kind=class; type=Pioneer; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=10
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Record.cs
- 46 lines; kind=class; type=Record; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=8
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Scientific.cs
- 37 lines; kind=class; type=Scientific; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=10
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Startup.cs
- 41 lines; kind=class; type=Startup; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=9
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Stern.cs
- 39 lines; kind=class; type=Stern; bases=AgentMentality; ancestry=AgentMentality; ns=Contracts.Agents.Mentalities
- usings: KSP.Localization
- unity: none; unity_score=9
- ksp refs: Localizer=2, Contract=2; fan_in=0 fan_out=4
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/BodyLocation.cs
- 8 lines; kind=enum; type=BodyLocation; bases=-; ancestry=-; ns=Contracts
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Contract.cs **MUST-OPEN (>800 lines)**
- 2020 lines; kind=class; type=Contract; bases=IContractParameterHost; ancestry=IContractParameterHost; ns=Contracts
- usings: Contracts.Agents, KSP.Localization, KSP.UI.Screens, KSPAchievements, UnityEngine
- unity: Analytics=3, Lifecycle=1, Coroutine=1; unity_score=9
- ksp refs: CelestialBody=56, Reputation=41, Localizer=31, Contract=26, ContractParameter=24, GameEvents=17; fan_in=106 fan_out=33
- methods(97): Accept, AddBodyNodes, AddBodyTrees, AddKeywords, AddKeywordsRequired, AddNextUnreachedBodyTrees, AddParameter, AwardAdvance, AwardCompletion, CanBeCancelled, CanBeDeclined, CanBeFailed, Cancel, Complete, CountBodies_Complete, CountBodies_Reached, Decline, Fail, Generate, GenerateFailed, GetBodies, GetBodies_Complete, GetBodies_InComplete, GetBodies_NextUnreached, GetBodies_NotReached, GetBodies_Reached, GetDescription, GetDestinationWeight, GetHashString, GetNotes, GetParameter, GetSynopsys, GetTitle, GetWeightBodies, IsFinished, Kill, Load, MeetRequirements, MessageAccepted, MessageAdvances, MessageCancellationPenalties, MessageCancelled, MessageCompleted, MessageDeadlineExpired, MessageFailed ...
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/ContractParameter.cs **MUST-OPEN (>800 lines)**
- 841 lines; kind=class; type=ContractParameter; bases=IContractParameterHost; ancestry=IContractParameterHost; ns=Contracts
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Coroutine=2, Lifecycle=1; unity_score=11
- ksp refs: ContractParameter=23, Reputation=11, ConfigNode=7, Localizer=6, Contract=6, CelestialBody=6; fan_in=60 fan_out=19
- methods(39): AddParameter, AllChildParametersComplete, AnyChildParametersFailed, AwardCompletion, CreateID, Disable, Enable, GetDestinationWeight, GetHashString, GetMessageComplete, GetMessageFailed, GetMessageIncomplete, GetNotes, GetParameter, GetTitle, Load, NestToParent, OnLoad, OnParameterStateChange, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, ParameterStateUpdate, PenalizeFailure, Register, RemoveParameter, Reset, Save, SendStateMessage, SetComplete, SetFailed, SetFunds, SetIncomplete, SetReputation, SetScience, Unregister, Update
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/ContractPredicate.cs
- 73 lines; kind=class; type=ContractPredicate; bases=-; ancestry=-; ns=Contracts
- usings: KSP.Localization, UnityEngine
- unity: Lifecycle=1; unity_score=14
- ksp refs: ConfigNode=4, Vessel=1, ProtoVessel=1, Localizer=1, Contract=1; fan_in=6 fan_out=6
- methods(9): GetAllowMultiple, GetDescription, Load, OnLoad, OnSave, OnUpdate, Save, Test, Update
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/ContractSystem.cs **MUST-OPEN (>800 lines)**
- 1551 lines; kind=ScenarioModule; type=ContractSystem; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=Contracts
- usings: Expansions, Expansions.Missions, Expansions.Missions.Runtime, FinePrint, FinePrint.Contracts, FinePrint.Utilities, ModuleWheels, UnityEngine, Upgradeables
- unity: Coroutine=11, GameObject=10, Transform=5, SetActive=2, Instantiate/Destroy=2, GetComponent=2; unity_score=38
- ksp refs: Contract=81, ConfigNode=20, GameEvents=19, FlightGlobals=15, HighLogic=14, Vessel=9; fan_in=41 fan_out=54
- methods(59): AdjustWeight, AnyCompletedContracts, AnyCurrentActiveContracts, AnyCurrentContracts, ClampedWeight, ClearContractsCurrent, ClearContractsFinished, ConstructShip, CountContracts, GenerateContract, GenerateContractTypes, GenerateContracts, GenerateMandatoryContracts, GenerateMandatoryTypes, GenerateParameterTypes, GeneratePredicateTypes, GenerateTypes, GetActiveContractCount, GetCompletedContracts, GetContractByGuid, GetContractCounts, GetContractType, GetCurrentActiveContracts, GetCurrentContracts, GetParameterType, GetPredicateType, HasCompletedContract, LoadContract, LoadContractWeights, OnAwake, OnDestroy, OnFlightGlobalsReady, OnFlightReady, OnLoad, OnLoadRoutine, OnMissionControlSpawned, OnNodeReached, OnReputationChanged, OnSave, OnSceneLoadRequested, OnVesselChange, ReactivateComponents, RebuildContracts, RefreshContracts, RegisterContracts ...
- auto suggestion: tier=B subsystem=Contracts

## Assembly-CSharp/Contracts/IContractParameterHost.cs
- 41 lines; kind=interface; type=IContractParameterHost; bases=-; ancestry=-; ns=Contracts
- usings: -
- unity: none; unity_score=0
- ksp refs: ContractParameter=11, Contract=1; fan_in=9 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/AcquireCrew.cs
- 197 lines; kind=ContractParameter; type=AcquireCrew; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: FinePrint.Utilities, UnityEngine
- unity: none; unity_score=8
- ksp refs: GameEvents=13, Part=7, ConfigNode=2, Vessel=1, ProtoCrewMember=1, FlightGlobals=1; fan_in=1 fan_out=10
- methods(17): AddKerbal, CheckNewKerbal, GetTitle, KillKerbal, OnCouple, OnCrewBoardVessel, OnCrewKilled, OnCrewTransferred, OnLoad, OnPartDied, OnRegister, OnReset, OnSave, OnUnregister, RemoveKerbal, ScanVessel, SetTitle
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/AcquirePart.cs
- 167 lines; kind=ContractParameter; type=AcquirePart; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: FinePrint.Utilities, UnityEngine
- unity: none; unity_score=9
- ksp refs: GameEvents=5, Part=3, ConfigNode=2, Vessel=1, FlightGlobals=1, ContractParameter=1; fan_in=1 fan_out=8
- methods(14): AddPart, CheckNewPart, DestroyPart, GetTitle, OnCouple, OnLoad, OnPartDied, OnRegister, OnReset, OnSave, OnUnregister, RemovePart, ScanVessel, SetTitle
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/CollectScience.cs
- 109 lines; kind=ContractParameter; type=CollectScience; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: KSP.Localization
- unity: none; unity_score=4
- ksp refs: Vessel=5, GameEvents=4, CelestialBody=3, Localizer=2, ConfigNode=2, ScienceSubject=1; fan_in=0 fan_out=12
- methods(8): GetHashString, GetTitle, OnLoad, OnRegister, OnSave, OnScience, OnTriggeredScience, OnUnregister
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/EnterOrbit.cs
- 63 lines; kind=ContractParameter; type=EnterOrbit; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: KSP.Localization
- unity: none; unity_score=6
- ksp refs: CelestialBody=4, GameEvents=2, ConfigNode=2, Vessel=1, Localizer=1, FlightGlobals=1; fan_in=1 fan_out=7
- methods(7): GetHashString, GetTitle, OnEnterOrbit, OnLoad, OnRegister, OnSave, OnUnregister
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/EnterSOI.cs
- 63 lines; kind=ContractParameter; type=EnterSOI; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: KSP.Localization
- unity: none; unity_score=6
- ksp refs: CelestialBody=4, GameEvents=2, ConfigNode=2, Vessel=1, Localizer=1, FlightGlobals=1; fan_in=0 fan_out=7
- methods(7): GetHashString, GetTitle, OnEnterSOI, OnLoad, OnRegister, OnSave, OnUnregister
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/KerbalDeaths.cs
- 82 lines; kind=ContractParameter; type=KerbalDeaths; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: KSP.Localization
- unity: none; unity_score=5
- ksp refs: Localizer=2, GameEvents=2, ConfigNode=2, ContractParameter=1; fan_in=0 fan_out=7
- methods(7): GetHashString, GetTitle, OnCrewKilled, OnLoad, OnRegister, OnSave, OnUnregister
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/LandOnBody.cs
- 63 lines; kind=ContractParameter; type=LandOnBody; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: KSP.Localization
- unity: none; unity_score=6
- ksp refs: CelestialBody=4, GameEvents=2, ConfigNode=2, Vessel=1, Localizer=1, FlightGlobals=1; fan_in=0 fan_out=7
- methods(7): GetHashString, GetTitle, OnLoad, OnRegister, OnSave, OnUnregister, OnVesselLand
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/OR.cs
- 32 lines; kind=ContractParameter; type=OR; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: -
- unity: none; unity_score=11
- ksp refs: ContractParameter=2; fan_in=0 fan_out=2
- methods(2): GetHashString, OnParameterStateChange
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/PartTest.cs
- 336 lines; kind=ContractParameter; type=PartTest; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: GameEvents=7, FlightGlobals=7, Vessel=4, Contract=4, Part=3, Localizer=2; fan_in=0 fan_out=16
- methods(16): GetHashString, GetNotes, GetTitle, OnDock, OnLoad, OnPartRunTest, OnRegister, OnReset, OnSave, OnTestPartDestroyed, OnUnregister, OnUpdate, OnVesselChanged, PartModified, VesselModified, hasTestedPart
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/PlantFlag.cs
- 63 lines; kind=ContractParameter; type=PlantFlag; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: KSP.Localization
- unity: none; unity_score=6
- ksp refs: CelestialBody=3, GameEvents=2, ConfigNode=2, Vessel=1, Localizer=1, FlightGlobals=1; fan_in=0 fan_out=7
- methods(7): GetHashString, GetTitle, OnLoad, OnPlantFlag, OnRegister, OnSave, OnUnregister
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/ReachAltitudeEnvelope.cs
- 72 lines; kind=ContractParameter; type=ReachAltitudeEnvelope; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=5
- ksp refs: FlightGlobals=3, ConfigNode=2, Localizer=1, HighLogic=1, ContractParameter=1, Contract=1; fan_in=1 fan_out=8
- methods(5): GetTitle, OnLoad, OnReset, OnSave, OnUpdate
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/ReachBiome.cs
- 110 lines; kind=ContractParameter; type=ReachBiome; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: -
- unity: none; unity_score=4
- ksp refs: FlightGlobals=3, ConfigNode=2, Vessel=1, HighLogic=1, ContractParameter=1, Contract=1; fan_in=0 fan_out=8
- methods(8): GetTitle, GetTitleStringShort, OnLoad, OnReset, OnSave, OnUpdate, bodyHasBiome, checkVesselWithinBiome
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/ReachDestination.cs
- 99 lines; kind=ContractParameter; type=ReachDestination; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=4
- ksp refs: FlightGlobals=4, CelestialBody=4, Vessel=3, GameEvents=3, ConfigNode=2, Localizer=1; fan_in=2 fan_out=9
- methods(10): GetTitle, GetTitleStringShort, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnVesselSOIChange, TrackVessel, checkVesselDestination
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/ReachFlightEnvelope.cs
- 287 lines; kind=ContractParameter; type=ReachFlightEnvelope; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: KSP.Localization, UnityEngine
- unity: Coroutine=4; unity_score=12
- ksp refs: Vessel=33, Localizer=19, GameEvents=7, FlightGlobals=3, Contract=3, CelestialBody=3; fan_in=0 fan_out=11
- methods(12): GetSituationStringShort, GetTitle, OnFlightReady, OnLoad, OnRegister, OnSave, OnUnregister, OnVesselChange, OnVesselSituationChange, TrackVessel, checkVesselWithinBiome, checkVesselWithinFlightEnvelope
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/ReachSituation.cs
- 141 lines; kind=ContractParameter; type=ReachSituation; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=3
- ksp refs: Vessel=22, Localizer=8, GameEvents=3, FlightGlobals=2, ConfigNode=2, ContractParameter=1; fan_in=2 fan_out=8
- methods(10): GetTitle, GetTitleStringShort, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnVesselSituationChange, TrackVessel, checkVesselSituation
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/ReachSpeedEnvelope.cs
- 90 lines; kind=ContractParameter; type=ReachSpeedEnvelope; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=4
- ksp refs: Vessel=8, FlightGlobals=2, ConfigNode=2, Localizer=1, HighLogic=1, ContractParameter=1; fan_in=1 fan_out=9
- methods(6): GetTitle, OnLoad, OnReset, OnSave, OnUpdate, checkVesselWithinEnvelope
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/RecoverKerbal.cs
- 262 lines; kind=ContractParameter; type=RecoverKerbal; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: Contracts.Templates, UnityEngine
- unity: none; unity_score=7
- ksp refs: GameEvents=10, ProtoCrewMember=2, FlightGlobals=2, ConfigNode=2, Vessel=1, ProtoVessel=1; fan_in=1 fan_out=11
- methods(13): AddKerbal, GetTitle, OnCrewKilled, OnFlightReady, OnLoad, OnPartDied, OnRegister, OnSave, OnUnregister, OnVesselLoad, OnVesselRecovered, RemoveKerbal, SetTitle
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/RecoverPart.cs
- 189 lines; kind=ContractParameter; type=RecoverPart; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: Contracts.Templates, FinePrint.Utilities
- unity: none; unity_score=2
- ksp refs: GameEvents=8, FlightGlobals=2, ConfigNode=2, Vessel=1, ProtoVessel=1, ProtoPartSnapshot=1; fan_in=1 fan_out=11
- methods(11): AddPartToRecover, GetTitle, OnFlightReady, OnLoad, OnPartDie, OnRegister, OnSave, OnUnregister, OnVesselLoad, OnVesselRecovered, RemovePartToRecover
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Parameters/XOR.cs
- 41 lines; kind=ContractParameter; type=XOR; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Parameters
- usings: -
- unity: none; unity_score=9
- ksp refs: ContractParameter=2; fan_in=0 fan_out=2
- methods(2): GetHashString, OnParameterStateChange
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/ParameterState.cs
- 14 lines; kind=enum; type=ParameterState; bases=-; ancestry=-; ns=Contracts
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=33 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Predicates/AgentFlag.cs
- 88 lines; kind=class; type=AgentFlag; bases=ContractPredicate; ancestry=ContractPredicate; ns=Contracts.Predicates
- usings: Contracts.Agents, KSP.Localization, UnityEngine
- unity: none; unity_score=12
- ksp refs: ConfigNode=2, Vessel=1, ProtoVessel=1, Localizer=1; fan_in=0 fan_out=8
- methods(4): GetDescription, OnLoad, OnSave, Test
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Predicates/AgentParts.cs
- 112 lines; kind=class; type=AgentParts; bases=ContractPredicate; ancestry=ContractPredicate; ns=Contracts.Predicates
- usings: Contracts.Agents, KSP.Localization, UnityEngine
- unity: none; unity_score=12
- ksp refs: Localizer=3, ConfigNode=2, Vessel=1, ProtoVessel=1; fan_in=0 fan_out=8
- methods(4): GetDescription, OnLoad, OnSave, Test
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Predicates/IsActiveVessel.cs
- 23 lines; kind=class; type=IsActiveVessel; bases=ContractPredicate; ancestry=ContractPredicate; ns=Contracts.Predicates
- usings: -
- unity: none; unity_score=12
- ksp refs: Vessel=1, ProtoVessel=1, FlightGlobals=1; fan_in=0 fan_out=5
- methods(1): Test
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Predicates/IsControllable.cs
- 28 lines; kind=class; type=IsControllable; bases=ContractPredicate; ancestry=ContractPredicate; ns=Contracts.Predicates
- usings: -
- unity: none; unity_score=12
- ksp refs: Vessel=1, ProtoVessel=1; fan_in=0 fan_out=4
- methods(2): GetDescription, Test
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Predicates/LaunchID.cs
- 111 lines; kind=class; type=LaunchID; bases=ContractPredicate; ancestry=ContractPredicate; ns=Contracts.Predicates
- usings: KSP.Localization
- unity: none; unity_score=3
- ksp refs: Vessel=2, ProtoVessel=2, HighLogic=2, Contract=2, ConfigNode=2, Localizer=1; fan_in=0 fan_out=8
- methods(6): ContainsLaunchID_GreaterThanOrEqualTo, GetDescription, OnContractStateChange, OnLoad, OnSave, Test
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Templates/CollectScience.cs
- 177 lines; kind=ContractParameter; type=CollectScience; bases=Contract; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Templates
- usings: Contracts.Parameters, FinePrint, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=5
- ksp refs: CelestialBody=12, Localizer=7, Planetarium=6, Reputation=4, FlightGlobals=2, ConfigNode=2; fan_in=0 fan_out=15
- methods(11): Generate, GenerateContract_Orbit, GenerateContract_Surface, GetDescription, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Templates/GrandTour.cs
- 552 lines; kind=Contract; type=GrandTour; bases=Contract; ancestry=Contract/IContractParameterHost; ns=Contracts.Templates
- usings: FinePrint, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=1
- ksp refs: CelestialBody=29, Localizer=27, Vessel=19, Planetarium=14, FlightGlobals=4, Reputation=2; fan_in=0 fan_out=24
- methods(16): AttemptFinalDestination, AttemptFocusedTour, AttemptRandomTour, Generate, GetDescription, GetFocusedChallenges, GetHashString, GetNotes, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnAccepted, OnLoad, OnSave
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Templates/OrbitalConstructionContract.cs
- 414 lines; kind=Contract; type=OrbitalConstructionContract; bases=Contract,IUpdateWaypoints; ancestry=Contract/IContractParameterHost; ns=Contracts.Templates
- usings: Expansions.Missions, FinePrint, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=4
- ksp refs: Reputation=6, GameEvents=6, FlightGlobals=6, Localizer=5, CelestialBody=4, Contract=3; fan_in=0 fan_out=37
- methods(20): CanBeCancelled, CanBeDeclined, CleanupRenderer, CreateVesselName, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, MeetRequirements, MessageCompleted, OnAccepted, OnContractPreBuiltVesselSpawned, OnLoad, OnRegister, OnSave, OnUnregister, OnVesselDestroy, SetupRenderer, VesselIdChanged
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Templates/PartTest.cs **MUST-OPEN (>800 lines)**
- 874 lines; kind=ContractParameter; type=PartTest; bases=Contract; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Templates
- usings: Contracts.Agents, Contracts.Parameters, FinePrint, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: GetComponent=1; unity_score=6
- ksp refs: Vessel=97, Localizer=36, CelestialBody=13, ResearchAndDevelopment=7, Planetarium=7, PartLoader=5; fan_in=0 fan_out=35
- methods(17): CombineSituations, Generate, GetDescription, GetSituationString, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnAccepted, OnFinished, OnLoad, OnPartResearched, OnRegister, OnSave, OnUnregister, RemoveSituations
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Templates/PlantFlag.cs
- 137 lines; kind=ContractParameter; type=PlantFlag; bases=Contract; ancestry=ContractParameter/IContractParameterHost; ns=Contracts.Templates
- usings: Contracts.Parameters, FinePrint, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=6
- ksp refs: CelestialBody=10, Vessel=5, Planetarium=4, Localizer=3, Reputation=2, FlightGlobals=2; fan_in=0 fan_out=20
- methods(10): Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Templates/RecoverAsset.cs **MUST-OPEN (>800 lines)**
- 1024 lines; kind=Contract; type=RecoverAsset; bases=Contract; ancestry=Contract/IContractParameterHost; ns=Contracts.Templates
- usings: Contracts.Parameters, FinePrint, FinePrint.Utilities, KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: none; unity_score=0
- ksp refs: Localizer=56, ConfigNode=21, ProtoCrewMember=19, Planetarium=16, HighLogic=10, ProtoVessel=8; fan_in=3 fan_out=54
- methods(39): AcceptablePart, CheckComplete, ChooseLocation, ChooseOrbit, ChooseVesselType, Cleanup, CreateProcessedPartNode, DiscoverAsset, Generate, GenerateKerbalInPart, GenerateLandRecovery, GenerateLoneKerbal, GenerateLonePart, GenerateOrbitalRecovery, GetDescription, GetHashString, GetMultiplier, GetNotes, GetSynopsys, GetTitle, GetWeightBodies, GroundNode, Initialize, MeetRequirements, MessageCancelled, MessageCompleted, MessageDeadlineExpired, MessageFailed, OnAccepted, OnDeclined, OnFinished, OnGenerateFailed, OnLoad, OnOfferExpired, OnParameterStateChange, OnSave, PartHasSeats, SeatKerbal, ShiftCoordinates
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Templates/RoverConstructionContract.cs
- 337 lines; kind=Contract; type=RoverConstructionContract; bases=Contract,IUpdateWaypoints; ancestry=Contract/IContractParameterHost; ns=Contracts.Templates
- usings: Expansions.Missions, FinePrint, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=5
- ksp refs: Reputation=6, GameEvents=6, FlightGlobals=6, CelestialBody=5, Localizer=4, Contract=3; fan_in=0 fan_out=31
- methods(19): CalculateWaypoint, CanBeCancelled, CanBeDeclined, CreateVesselName, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, MeetRequirements, MessageCompleted, OnAccepted, OnContractPreBuiltVesselSpawned, OnLoad, OnRegister, OnSave, OnUnregister, OnVesselDestroy, VesselIdChanged
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/Templates/VesselRepairContract.cs
- 602 lines; kind=Contract; type=VesselRepairContract; bases=Contract; ancestry=Contract/IContractParameterHost; ns=Contracts.Templates
- usings: Expansions.Missions, FinePrint, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=3
- ksp refs: FlightGlobals=9, Reputation=6, GameEvents=6, Localizer=5, PartLoader=4, CelestialBody=4; fan_in=0 fan_out=39
- methods(20): CanBeCancelled, CanBeDeclined, CleanupRenderer, CreateVesselName, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, MeetRequirements, MessageCompleted, OnAccepted, OnContractPreBuiltVesselSpawned, OnLoad, OnRegister, OnSave, OnUnregister, OnVesselDestroy, SetupRenderer, VesselIdChanged
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/Contracts/TextGen.cs
- 180 lines; kind=static class; type=TextGen; bases=-; ancestry=-; ns=Contracts
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=11
- ksp refs: ConfigNode=2, Localizer=1, GameDatabase=1; fan_in=22 fan_out=5
- methods(5): GenerateBackStories, GetSentence, ProcessTextReplacements, ReplaceCharAt, Setup
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/ContractDefs.cs **MUST-OPEN (>800 lines)**
- 3533 lines; kind=KSPAddon; type=ContractDefs; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=FinePrint
- usings: Expansions, FinePrint.Utilities, KSP.Localization, UniLinq, UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1, GetComponent=1, GameObject=1; unity_score=16
- ksp refs: ConfigNode=481, Reputation=177, Localizer=10, GameEvents=3, GameDatabase=3, PartLoader=1; fan_in=45 fan_out=23
- methods(14): AddCrewRequests, AddPartRequests, AddResourceRequests, AddSurveyDefinitions, Awake, CreateDefaultConfig, GetPreBuiltCraft, GetPreBuiltCraftPositions, LoadConfig, LoadConstructionPartsList, LoadContractCraftDefs, LoadDefinitions, OnDestroy, TechResearched
- auto suggestion: tier=B subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/ARMContract.cs
- 235 lines; kind=Contract; type=ARMContract; bases=Contract; ancestry=Contract/IContractParameterHost; ns=FinePrint.Contracts
- usings: Contracts, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Planetarium=18, Localizer=18, CelestialBody=8, Vessel=3, Reputation=3, ConfigNode=2; fan_in=0 fan_out=23
- methods(12): CanBeCancelled, CanBeDeclined, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/BaseContract.cs
- 264 lines; kind=Contract; type=BaseContract; bases=Contract,IUpdateWaypoints,ICheckSpecificVessels; ancestry=Contract/IContractParameterHost; ns=FinePrint.Contracts
- usings: Contracts, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=3
- ksp refs: Localizer=13, Planetarium=9, CelestialBody=9, Vessel=3, Reputation=3, ConfigNode=3; fan_in=1 fan_out=32
- methods(15): AreFacilitiesUnlocked, CanBeCancelled, CanBeDeclined, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnAccepted, OnFinished, OnLoad, OnSave
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/CometSampleContract.cs
- 278 lines; kind=Contract; type=CometSampleContract; bases=Contract; ancestry=Contract/IContractParameterHost; ns=FinePrint.Contracts
- usings: Contracts, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=9
- ksp refs: Localizer=13, FlightGlobals=8, GameEvents=7, Reputation=6, Vessel=5, ConfigNode=2; fan_in=0 fan_out=21
- methods(17): CanBeCancelled, CanBeDeclined, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, MeetRequirements, MessageCompleted, OnDiscoverableObjectExpired, OnLoad, OnRegister, OnSave, OnUnregister, OnVesselRename, ValidCometAvailable, VesselIdChanged
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/ExplorationContract.cs **MUST-OPEN (>800 lines)**
- 846 lines; kind=Contract; type=ExplorationContract; bases=Contract; ancestry=Contract/IContractParameterHost; ns=FinePrint.Contracts
- usings: Contracts, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization, KSPAchievements, UnityEngine
- unity: none; unity_score=0
- ksp refs: Planetarium=17, Localizer=15, CelestialBody=12, FlightGlobals=2, ConfigNode=2, ResearchAndDevelopment=1; fan_in=1 fan_out=28
- methods(20): AllowableContracts, CanBeCancelled, CanBeDeclined, CanBeFailed, Compare, Generate, GetDescription, GetMissionMilestones, GetSeedMilestones, GetSynopsys, GetThemedMilestones, GetTitle, MeetRequirements, MessageCompleted, OnLoad, OnSave, ProgressTypeValue, SetLocals, SetPrestige, SetRewards
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/ISRUContract.cs
- 298 lines; kind=Contract; type=ISRUContract; bases=Contract; ancestry=Contract/IContractParameterHost; ns=FinePrint.Contracts
- usings: Contracts, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=4
- ksp refs: ConfigNode=14, CelestialBody=11, Localizer=8, Planetarium=7, Vessel=3, Reputation=2; fan_in=0 fan_out=21
- methods(14): CanBeCancelled, CanBeDeclined, CelestialIsForbidden, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave, PossibleResources
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/AsteroidParameter.cs
- 231 lines; kind=ContractParameter; type=AsteroidParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: FlightGlobals=8, Localizer=7, GameEvents=7, Part=3, Contract=3, Vessel=2; fan_in=2 fan_out=14
- methods(13): GetHashString, GetNotes, GetTitle, OnDock, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, PartModified, VesselModified, VesselTowingAsteroid
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/CometDetectionParameter.cs
- 92 lines; kind=ContractParameter; type=CometDetectionParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization, SentinelMission
- unity: none; unity_score=0
- ksp refs: Localizer=4, GameEvents=2, ConfigNode=2, Vessel=1, HighLogic=1, ContractParameter=1; fan_in=1 fan_out=11
- methods(9): CometDetected, GetHashString, GetNotes, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUnregister
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/CometParameter.cs
- 231 lines; kind=ContractParameter; type=CometParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: FlightGlobals=8, Localizer=7, GameEvents=7, Part=3, Contract=3, Vessel=2; fan_in=0 fan_out=14
- methods(13): GetHashString, GetNotes, GetTitle, OnDock, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, PartModified, VesselModified, VesselTowingComet
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/CometScienceParameter.cs
- 125 lines; kind=ContractParameter; type=CometScienceParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, KSP.Localization
- unity: none; unity_score=3
- ksp refs: GameEvents=11, Vessel=2, ConfigNode=2, ScienceSubject=1, ProtoVessel=1, Localizer=1; fan_in=1 fan_out=8
- methods(11): GetHashString, GetTitle, OnExperimentDeployed, OnLoad, OnRegister, OnSave, OnScience, OnTriggeredScience, OnUnregister, OnVesselRename, VesselIdChanged
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/ConstructionParameter.cs
- 128 lines; kind=ContractParameter; type=ConstructionParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, KSP.Localization
- unity: none; unity_score=3
- ksp refs: GameEvents=4, FlightGlobals=4, CelestialBody=4, Vessel=2, PartLoader=2, Localizer=2; fan_in=1 fan_out=10
- methods(10): GetHashString, GetMessageComplete, GetTitle, OnConstructionPartAttached, OnLoad, OnRegister, OnSave, OnUnregister, UpdatePartInfo, VesselIdChanged
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/CrewCapacityParameter.cs
- 167 lines; kind=ContractParameter; type=CrewCapacityParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: FlightGlobals=11, GameEvents=7, Part=3, Localizer=3, Contract=2, ConfigNode=2; fan_in=2 fan_out=12
- methods(12): GetHashString, GetNotes, GetTitle, OnDock, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, PartModified, VesselModified
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/CrewTraitParameter.cs
- 202 lines; kind=ContractParameter; type=CrewTraitParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: FlightGlobals=12, GameEvents=9, Localizer=4, Part=3, Vessel=2, Contract=2; fan_in=4 fan_out=13
- methods(13): CrewModified, GetHashString, GetNotes, GetTitle, OnDock, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, PartModified, VesselModified
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/KerbalDestinationParameter.cs
- 143 lines; kind=ContractParameter; type=KerbalDestinationParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=6, ProtoCrewMember=3, HighLogic=3, Planetarium=2, GameEvents=2, ConfigNode=2; fan_in=2 fan_out=13
- methods(8): CheckFlightLog, CheckFlightLogs, GetHashString, GetTitle, OnLoad, OnRegister, OnSave, OnUnregister
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/KerbalGeeAdventureParameter.cs
- 93 lines; kind=ContractParameter; type=KerbalGeeAdventureParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: GameEvents=4, ProtoCrewMember=2, ConfigNode=2, Localizer=1, ContractParameter=1, Contract=1; fan_in=1 fan_out=10
- methods(8): GetHashString, GetTitle, OnCrewKilled, OnKerbalPassedOut, OnLoad, OnRegister, OnSave, OnUnregister
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/KerbalTourParameter.cs
- 134 lines; kind=ContractParameter; type=KerbalTourParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization, KSP.UI.Screens
- unity: none; unity_score=0
- ksp refs: ProtoCrewMember=8, GameEvents=6, Localizer=2, HighLogic=2, ContractParameter=2, ConfigNode=2; fan_in=2 fan_out=14
- methods(11): GetHashString, GetMessageFailed, GetTitle, OnCrewKilled, OnKerbalInactiveChange, OnLoad, OnParameterStateChange, OnRegister, OnSave, OnUnregister, OnVesselRecovered
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/LocationAndSituationParameter.cs
- 169 lines; kind=ContractParameter; type=LocationAndSituationParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Vessel=15, Localizer=10, FlightGlobals=9, GameEvents=6, CelestialBody=3, Planetarium=2; fan_in=5 fan_out=13
- methods(10): ChangeBody, ChangeSituation, GetHashString, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/MobileBaseParameter.cs
- 135 lines; kind=ContractParameter; type=MobileBaseParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: FlightGlobals=12, GameEvents=10, Vessel=3, Part=3, Contract=2, Localizer=1; fan_in=1 fan_out=11
- methods(10): ChangeSituation, GetHashString, GetTitle, OnDock, OnRegister, OnReset, OnUnregister, OnUpdate, PartModified, VesselModified
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/PartRequestParameter.cs
- 340 lines; kind=ContractParameter; type=PartRequestParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: GameEvents=7, FlightGlobals=7, Part=5, Vessel=4, ConfigNode=3, ProtoPartSnapshot=2; fan_in=3 fan_out=14
- methods(14): CheckLists, GetHashString, GetTitle, OnDock, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, PartModified, ScanVesselForExistingParts, VesselHasPartRequest, VesselModified
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/ProgressTrackingParameter.cs
- 351 lines; kind=ContractParameter; type=ProgressTrackingParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=95, Planetarium=25, GameEvents=2, ConfigNode=2, HighLogic=1, ContractParameter=1; fan_in=1 fan_out=12
- methods(9): GetHashString, GetMessageComplete, GetNotes, GetTitle, OnLoad, OnRegister, OnSave, OnUnregister, ProgressCheck
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/RepairPartParameter.cs
- 148 lines; kind=ContractParameter; type=RepairPartParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, KSP.Localization
- unity: none; unity_score=3
- ksp refs: GameEvents=6, FlightGlobals=4, CelestialBody=4, PartLoader=2, Localizer=2, ConfigNode=2; fan_in=1 fan_out=10
- methods(11): GetHashString, GetMessageComplete, GetTitle, OnLoad, OnPartRepaired, OnRegister, OnSave, OnUnregister, PartIdChanged, UpdatePartInfo, VesselIdChanged
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/ResourceExtractionParameter.cs
- 155 lines; kind=ContractParameter; type=ResourceExtractionParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=3, Planetarium=2, GameEvents=2, Contract=2, ConfigNode=2, CelestialBody=2; fan_in=1 fan_out=12
- methods(9): GetHashString, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, ResourcesHarvested
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/ResourcePossessionParameter.cs
- 130 lines; kind=ContractParameter; type=ResourcePossessionParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=3, ConfigNode=2, ContractParameter=1, Contract=1; fan_in=2 fan_out=8
- methods(8): GetHashString, GetNotes, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUpdate
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/RoverWayPointParameter.cs
- 310 lines; kind=ContractParameter; type=RoverWayPointParameter; bases=WaypointParameter; ancestry=WaypointParameter/ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=6, Contract=4, Vessel=3, HighLogic=2, GameEvents=2, FlightGlobals=2; fan_in=1 fan_out=19
- methods(14): CleanupWaypoints, GetHashString, GetMessageComplete, GetNotes, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, UpdateRoverInfo, UpdateWaypoints, VesselIdChanged
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/SpecificOrbitParameter.cs
- 303 lines; kind=ContractParameter; type=SpecificOrbitParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=21, Contract=8, Planetarium=6, Orbit=6, HighLogic=3, ConfigNode=2; fan_in=3 fan_out=19
- methods(12): CleanupRenderer, GetHashString, GetNotes, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, SetupRenderer, ValidateOrbit
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/SpecificVesselParameter.cs
- 361 lines; kind=ContractParameter; type=SpecificVesselParameter; bases=WaypointParameter; ancestry=WaypointParameter/ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization, KSP.UI.Screens
- unity: Coroutine=3; unity_score=6
- ksp refs: Vessel=16, Localizer=11, FlightGlobals=7, Contract=6, Planetarium=3, HighLogic=3; fan_in=8 fan_out=18
- methods(16): CleanupWaypoints, GetHashString, GetMessageComplete, GetNotes, GetTitle, KillContract, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, SpecificVesselRename, SpecificVesselScan, TargetVesselSearch, UpdateWaypoints
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/StabilityParameter.cs
- 98 lines; kind=ContractParameter; type=StabilityParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: Time.=1; unity_score=4
- ksp refs: Vessel=2, ConfigNode=2, Localizer=1, FlightGlobals=1, ContractParameter=1, Contract=1; fan_in=5 fan_out=8
- methods(7): GetHashString, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUpdate
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/StationaryPointParameter.cs
- 159 lines; kind=ContractParameter; type=StationaryPointParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Contract=3, Planetarium=2, Localizer=2, HighLogic=2, ConfigNode=2, CelestialBody=2; fan_in=1 fan_out=16
- methods(9): GetHashString, GetMessageComplete, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/SurveyWaypointParameter.cs
- 557 lines; kind=ContractParameter; type=SurveyWaypointParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=1
- ksp refs: Localizer=24, Contract=5, HighLogic=3, CelestialBody=3, ResearchAndDevelopment=2, Planetarium=2; fan_in=1 fan_out=32
- methods(17): Bounce, CalculateAltitudes, CheckExperimentResults, GetHashString, GetMessageComplete, GetTitle, NextWaypointInCluster, OnLoad, OnRegister, OnSave, OnUnregister, OnUpdate, ProcessWaypoint, SiteString, TriggerRange, UpdateNavWaypoint, VerifyWaypoint
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/VesselDestinationParameter.cs
- 116 lines; kind=ContractParameter; type=VesselDestinationParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=6, GameEvents=4, Planetarium=2, ConfigNode=2, CelestialBody=2, Vessel=1; fan_in=1 fan_out=14
- methods(8): CheckFlightLog, GetHashString, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUnregister
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/VesselSystemsParameter.cs
- 248 lines; kind=ContractParameter; type=VesselSystemsParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Contracts.Parameters
- usings: Contracts, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: FlightGlobals=11, Localizer=10, GameEvents=7, Part=3, Vessel=2, Contract=2; fan_in=4 fan_out=13
- methods(13): GetHashString, GetNotes, GetTitle, OnDock, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, PartModified, VesselModified, isVesselValid
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/SatelliteContract.cs **MUST-OPEN (>800 lines)**
- 930 lines; kind=Contract; type=SatelliteContract; bases=Contract,IUpdateWaypoints,ICheckSpecificVessels; ancestry=Contract/IContractParameterHost; ns=FinePrint.Contracts
- usings: Contracts, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=1
- ksp refs: Localizer=52, Planetarium=25, Vessel=14, Reputation=8, CelestialBody=7, Orbit=6; fan_in=0 fan_out=40
- methods(23): AreSatellitesUnlocked, CanBeCancelled, CanBeDeclined, CheckSatelliteNetwork, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, NetworkSatelliteCount, OnAccepted, OnFinished, OnLoad, OnSave, PrestigeAppropriateOrbitType, ProbeCoresUnlocked, pickEasy, pickHard, pickMedium, setOrbitType
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/StationContract.cs
- 305 lines; kind=Contract; type=StationContract; bases=Contract,IUpdateWaypoints,ICheckSpecificVessels; ancestry=Contract/IContractParameterHost; ns=FinePrint.Contracts
- usings: Contracts, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=2
- ksp refs: Localizer=15, Planetarium=10, CelestialBody=4, Vessel=3, Reputation=3, ConfigNode=3; fan_in=1 fan_out=34
- methods(16): AreAsteroidsUnlocked, AreFacilitiesUnlocked, CanBeCancelled, CanBeDeclined, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnAccepted, OnFinished, OnLoad, OnSave
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/SurveyContract.cs **MUST-OPEN (>800 lines)**
- 943 lines; kind=Contract; type=SurveyContract; bases=Contract; ancestry=Contract/IContractParameterHost; ns=FinePrint.Contracts
- usings: Contracts, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: Texture=1; unity_score=3
- ksp refs: Vessel=11, CelestialBody=10, Localizer=9, Planetarium=7, FlightGlobals=6, Part=2; fan_in=1 fan_out=40
- methods(18): CachePossibleVessels, CanBeCancelled, CanBeDeclined, Generate, GetDescription, GetHashString, GetNotes, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave, ParamRange, PossibleDefinitions, RandomizeNearKSC, SetupParam
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Contracts/TourismContract.cs
- 633 lines; kind=Contract; type=TourismContract; bases=Contract; ancestry=Contract/IContractParameterHost; ns=FinePrint.Contracts
- usings: Contracts, Contracts.Parameters, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: CelestialBody=42, ProtoCrewMember=16, Localizer=13, HighLogic=9, Planetarium=7, ContractParameter=6; fan_in=2 fan_out=34
- methods(35): BuildPossibleItineraries, CanBeCancelled, CanBeDeclined, ClearKerbalsHard, ClearKerbalsSoft, Generate, GetContractSubject, GetDescription, GetHashString, GetKerbal, GetNotes, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCancelled, MessageCompleted, MessageDeadlineExpired, MessageFailed, OnAccepted, OnCancelled, OnDeadlineExpired, OnDeclined, OnFailed, OnFinished, OnGenerateFailed, OnLoad, OnOfferExpired, OnSave, OnWithdrawn, Pluralize, PossibleBodies, PossibleSituations, RandomReachedBodyGrouping, WeightedItineraryChoice
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/ScenarioContractEvents.cs
- 185 lines; kind=ScenarioModule; type=ScenarioContractEvents; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=FinePrint
- usings: Contracts, FinePrint.Contracts.Parameters, FinePrint.Utilities, UnityEngine
- unity: SendMessage/Invoke=3, Coroutine=3, Lifecycle=1; unity_score=38
- ksp refs: GameEvents=14, Vessel=3, ProtoVessel=3, ProtoCrewMember=3, HighLogic=3, Part=2; fan_in=1 fan_out=19
- methods(14): CheckSpecificVessels, InvokeCheckSpecificVessels, OnAwake, OnDestroy, OnLoad, OnSave, PartCoupleEvent, ProcessGameSeed, TouristDepartures, VesselRecoveredEvent, VesselRenameEvent, VesselTerminatedEvent, VesselWasModifiedEvent, VesselWillDestroyEvent
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/CelestialUtilities.cs
- 467 lines; kind=static class; type=CelestialUtilities; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: UnityEngine
- unity: Transform=1, GetComponent=1; unity_score=4
- ksp refs: CelestialBody=49, Vector3d=8, Vessel=7, Planetarium=6, FlightGlobals=4, PhysicsGlobals=3; fan_in=30 fan_out=14
- methods(26): AddChildren, ApplicableSituation, CanBodyBeKolniya, CanBodyBeSynchronous, CanBodyBeTundra, ChildrenOf, GetAltitudeForDensity, GetHighestPeak, GetHostPlanet, GetMinimumOrbitalDistance, GetNeighbors, GetNextTimeWarp, GetPlanetarySystem, GetSolarExtents, GreatCircleDistance, HighestBody, IsFlyablePlanet, IsGasGiant, KolniyaSMA, LLAtoECEF, LowestBody, MapFocusBody, PlanetScienceRanking, RandomBody, SynchronousSMA, TerrainAltitude
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/CompatibilityUtilities.cs
- 619 lines; kind=class; type=CompatibilityUtilities; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: Contracts, UnityEngine
- unity: none; unity_score=3
- ksp refs: Vessel=21, ConfigNode=21, Contract=4, Planetarium=3, Game=3, CelestialBody=3; fan_in=2 fan_out=19
- methods(6): CleanUpUnsanitaryEVAKerbals, FindSurveyDefinition, OldCareerSave, SavePriorTo, UpdateCareerSave, UpgradeVesselSituation
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/ConstructionExtensions.cs
- 26 lines; kind=static class; type=ConstructionExtensions; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): ContainsPart
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/ConstructionPart.cs
- 20 lines; kind=class; type=ConstructionPart; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: AvailablePart=2; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/ConstructionParts.cs
- 39 lines; kind=class; type=ConstructionParts; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: UnityEngine
- unity: none; unity_score=17
- ksp refs: none; fan_in=1 fan_out=1
- methods(1): PartsForContractType
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/FlightBand.cs
- 13 lines; kind=enum; type=FlightBand; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/GrammaticalGender.cs
- 9 lines; kind=enum; type=GrammaticalGender; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/ICheckSpecificVessels.cs
- 6 lines; kind=interface; type=ICheckSpecificVessels; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/IUpdateWaypoints.cs
- 6 lines; kind=interface; type=IUpdateWaypoints; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=8 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/LoadResult.cs
- 10 lines; kind=enum; type=LoadResult; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/MannedStatus.cs
- 9 lines; kind=enum; type=MannedStatus; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=16 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/OrbitalProperties.cs
- 13 lines; kind=enum; type=OrbitalProperties; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/OrbitType.cs
- 13 lines; kind=enum; type=OrbitType; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=7 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/OrbitUtilities.cs
- 360 lines; kind=static class; type=OrbitUtilities; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: UnityEngine
- unity: none; unity_score=5
- ksp refs: Orbit=15, Vector3d=10, Planetarium=3, CelestialBody=3, FlightGlobals=2; fan_in=6 fan_out=12
- methods(15): AngleOfAscendingNode, AngleOfDescendingNode, GenerateApsides, GenerateArgumentOfPeriapsis, GenerateEpoch, GenerateFinalization, GenerateInclination, GenerateLongitudeOfAscendingNode, GenerateMeanAnomalyAtEpoch, GenerateOrbit, GetRelativeInclination, IdentifyOrbit, PositionOfApoapsis, PositionOfPeriapsis, ValidateOrbit
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/PreBuiltCraft.cs
- 61 lines; kind=class; type=PreBuiltCraft; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: UnityEngine
- unity: none; unity_score=12
- ksp refs: none; fan_in=1 fan_out=1
- methods(2): CraftByURL, CraftForContractType
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/PreBuiltCraftDefinition.cs
- 38 lines; kind=class; type=PreBuiltCraftDefinition; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: KSP.UI.Screens
- unity: none; unity_score=10
- ksp refs: ConfigNode=2; fan_in=7 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/PreBuiltCraftPosition.cs
- 11 lines; kind=class; type=PreBuiltCraftPosition; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/ProgressMilestone.cs
- 282 lines; kind=class; type=ProgressMilestone; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: KSPAchievements
- unity: none; unity_score=0
- ksp refs: CelestialBody=2, Planetarium=1; fan_in=4 fan_out=9
- methods(1): CheckCompletion
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/ProgressRewardType.cs
- 8 lines; kind=enum; type=ProgressRewardType; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/ProgressType.cs
- 35 lines; kind=enum; type=ProgressType; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=44 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/ProgressUtilities.cs
- 676 lines; kind=class; type=ProgressUtilities; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: Contracts, KSPAchievements, UnityEngine
- unity: none; unity_score=1
- ksp refs: CelestialBody=57, Planetarium=10, Contract=8, FlightGlobals=5, ResearchAndDevelopment=4, Reputation=3; fan_in=29 fan_out=32
- methods(21): AverageFacilityLevel, CelestialCrewCounts, CurrentTrackRecord, ExperimentPossibleAt, FindNextRecord, GetAnyBodyProgress, GetBodiesProgress, GetBodyProgress, GetNextUnreached, GetProgressLevel, HaveAnyTech, HaveModuleTech, HavePartTech, IntroWorldFirstContract, OutlierWorldFirstContract, ProgressTypePrestige, ReachedHomeBodies, ScoreProgressType, VisitedSurfaceOf, WorldFirstIntervalReward, WorldFirstStandardReward
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/PronounCasing.cs
- 9 lines; kind=enum; type=PronounCasing; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/RecordTrackType.cs
- 11 lines; kind=enum; type=RecordTrackType; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=6 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/SpriteMap.cs
- 24 lines; kind=class; type=SpriteMap; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: UnityEngine
- unity: Texture=5; unity_score=67
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=B subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/StringUtilities.cs
- 569 lines; kind=class; type=StringUtilities; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: Contracts, FinePrint.Contracts.Parameters, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=25, ProtoCrewMember=6, ConfigNode=5, Reputation=3, Planetarium=2, HighLogic=2; fan_in=33 fan_out=22
- methods(26): AlphaNumericDesignation, AppropriatePronoun, CacheLocalStrings, CalculateReadDuration, CardinalDirectionBetween, FillListWithElements, FormattedCurrencies, GenerateSiteName, GetSiteGeneratorInfoFileName, IntegerToGreek, IsVowel, KerbalGrammaticalGender, LoadSiteGenerationInfo, NamedSitePrefix, NamedSiteSuffix, PackDelimitedString, PossessiveString, ShortKerbalName, ShortName, SpecificVesselName, ThisThisAndThat, TitleCase, TryParseGeneric, UnpackDelimitedPair, UnpackDelimitedString, ValueAndUnits
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/SurveyDefinition.cs
- 79 lines; kind=class; type=SurveyDefinition; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: Reputation=4, ConfigNode=4; fan_in=4 fan_out=5
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/SurveyDefinitionParameter.cs
- 102 lines; kind=class; type=SurveyDefinitionParameter; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: Texture=4; unity_score=32
- ksp refs: ConfigNode=3; fan_in=2 fan_out=3
- methods(0): 
- auto suggestion: tier=B subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/SystemUtilities.cs
- 652 lines; kind=class; type=SystemUtilities; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: Contracts, Contracts.Templates, FinePrint.Contracts, FinePrint.Contracts.Parameters, KSP.Localization, UnityEngine
- unity: Texture=3, GameObject=1; unity_score=11
- ksp refs: ConfigNode=14, FlightGlobals=10, HighLogic=6, Contract=6, CelestialBody=5, Vessel=3; fan_in=66 fan_out=26
- methods(26): CheckTouristRecoveryContractKerbals, CoinFlip, CondenseNodeList, ExpungeKerbal, ExpungeVessel, FlightIsReady, GetDeviationAsFraction, HashNumber, LoadNode, LoadNodeList, LoadNodePair, LoadTexture, MeasureDeviation, MeasureDeviationByValue, ProcessSideRequests, RandomColor, RandomSplitChoice, SaveNodeList, ShuffleList, SuperSeed, TryConvert, ValidateLoadCraftNode, WithinDeviation, WithinDeviationAccuracy, WithinDeviationByReferenceAccuracy, WithinDeviationByValue
- auto suggestion: tier=B subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/VesselUtilities.cs **MUST-OPEN (>800 lines)**
- 1291 lines; kind=class; type=VesselUtilities; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: Contracts, FinePrint.Contracts, FinePrint.Contracts.Parameters, UnityEngine
- unity: Renderer=4, Transform=1; unity_score=5
- ksp refs: Vessel=51, FlightGlobals=21, ProtoCrewMember=14, Orbit=10, Part=8, CelestialBody=6; fan_in=23 fan_out=32
- methods(29): ActiveVesselFallback, ActualCrewCapacity, ClassifyAllVesselsAt, ClassifyVesselType, CrewTraitMissionAvailability, DiscoverVessel, EstimatePartSize, FindFirstPartOrModuleName, FindVesselWithPartIDs, GenerateAdjustedVesselOrbit, GetFlightBand, GetPartIDList, GetPartName, SpecificVesselClassAt, VesselAtOrbit, VesselAtOrbitAccuracy, VesselCrewWithTrait, VesselCrewWithTraitCount, VesselHasAnyModules, VesselHasAnyParts, VesselHasAnyPartsOrModules, VesselHasModuleName, VesselHasPartName, VesselHasWheelsOnGround, VesselID, VesselIsOwned, VesselLaunchedAfterID, VesselPartAndModuleCount, VesselResourceAmount
- auto suggestion: tier=B subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/WaypointClusterState.cs
- 10 lines; kind=enum; type=WaypointClusterState; bases=-; ancestry=-; ns=FinePrint.Utilities
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Utilities/WaypointParameter.cs
- 21 lines; kind=ContractParameter; type=WaypointParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=FinePrint.Utilities
- usings: Contracts
- unity: none; unity_score=0
- ksp refs: ContractParameter=1, CelestialBody=1; fan_in=3 fan_out=2
- methods(3): CleanupWaypoints, SetupWaypoints, UpdateWaypoints
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/Waypoint.cs
- 479 lines; kind=class; type=Waypoint; bases=-; ancestry=-; ns=FinePrint
- usings: Contracts, FinePrint.Utilities, KSP.Localization, KSP.UI.Screens.Mapview, KSP.UI.Screens.Mapview.MapContextMenuOptions, UnityEngine
- unity: Time.=3, Transform=1, Texture=1; unity_score=8
- ksp refs: Vector3d=15, ScaledSpace=6, CelestialBody=5, FlightGlobals=4, MapView=3, Localizer=3; fan_in=33 fan_out=21
- methods(16): CachePositions, CheckAlpha, CheckOcclusion, CheckZoom, CleanupMapNode, OnMenuDismissed, OnUpdateNodeCaption, OnUpdateNodeIcon, OnUpdateNodePosition, ProcessClick, RandomizeAwayFrom, RandomizeNear, SetFadeRange, SetupMapNode, UpdateWaypoint, setName
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/WaypointCastHit.cs
- 19 lines; kind=struct; type=WaypointCastHit; bases=IScreenCaster; ancestry=IScreenCaster; ns=FinePrint
- usings: UnityEngine
- unity: Camera=1; unity_score=30
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): GetScreenSpacePoint
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/FinePrint/WaypointManager.cs
- 346 lines; kind=MonoBehaviour; type=WaypointManager; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=FinePrint
- usings: Contracts, FinePrint.Utilities, UnityEngine
- unity: Material=3, GameObject=3, Shader=2, Lifecycle=2, GetComponent=2, Camera=2; unity_score=47
- ksp refs: CelestialBody=8, Vector3d=6, HighLogic=6, FlightGlobals=6, MapView=5, Vessel=2; fan_in=13 fan_out=20
- methods(14): AddWaypoint, ChooseRandomPosition, ChooseRandomPositionNear, Distance, DistanceToVessel, FindWaypoint, Instance, LateUpdate, LateralDistanceToVessel, OnDestroy, RemoveWaypoint, UpdateContractWaypoints, UpdateWaypointNodes, WaypointsNearVessel
- auto suggestion: tier=C subsystem=Contracts

## Assembly-CSharp/KerbNet/KerbNetMode.cs
- 192 lines; kind=class; type=KerbNetMode; bases=-; ancestry=-; ns=KerbNet
- usings: KSP.UI.Dialogs, UnityEngine, UnityEngine.Events, UnityEngine.UI
- unity: Texture=4; unity_score=24
- ksp refs: Vessel=7, GameSettings=1; fan_in=4 fan_out=6
- methods(19): AutoGenerateMode, CustomPass, GetBackgroundColor, GetCoordinateColor, GetErrorState, GetModeCaption, GetModeColorTint, GetModeColorTintBlock, GetTerrainContourColors, Init, InterpolateContourTexture, InterpolateMainTexture, LocalCoordinateInfo, OnActivated, OnDeactivated, OnInit, OnPrecache, Precache, isModeActive
- auto suggestion: tier=B subsystem=CommNet

## Assembly-CSharp/KerbNet/KerbNetModeBiome.cs
- 81 lines; kind=class; type=KerbNetModeBiome; bases=KerbNetMode; ancestry=KerbNetMode; ns=KerbNet
- usings: KSP.Localization, UnityEngine
- unity: Texture=1, Resources.=1, Renderer=1; unity_score=35
- ksp refs: Localizer=5, Vessel=3, CelestialBody=2, Planetarium=1, OrbitDriver=1; fan_in=1 fan_out=9
- methods(5): CacheLocalStrings, GetCoordinateColor, LocalCoordinateInfo, OnInit, OnPrecache
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/KerbNet/KerbNetModeResource.cs
- 239 lines; kind=class; type=KerbNetModeResource; bases=KerbNetMode; ancestry=KerbNetMode; ns=KerbNet
- usings: KSP.Localization, KSP.UI.Dialogs, UnityEngine
- unity: Texture=1, Resources.=1; unity_score=15
- ksp refs: Vessel=5, Localizer=5, GameEvents=5, CelestialBody=4, PartResourceDefinition=1, FlightGlobals=1; fan_in=2 fan_out=16
- methods(16): AutoGenerateMode, CacheLocalStrings, CheckScannedBody, GetAbundance, GetCoordinateColor, GetErrorState, GetModeCaption, GetModeColorTint, LocalCoordinateInfo, OnActivated, OnColorClick, OnDeactivated, OnInit, OnPrecache, OrbitalSurveyCompleted, VesselSOIChange
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/KerbNet/KerbNetModeTerrain.cs
- 63 lines; kind=class; type=KerbNetModeTerrain; bases=KerbNetMode; ancestry=KerbNetMode; ns=KerbNet
- usings: FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: Renderer=2, Texture=1, Resources.=1; unity_score=45
- ksp refs: Localizer=4, Vessel=2; fan_in=1 fan_out=5
- methods(4): CacheLocalStrings, GetTerrainContourColors, LocalCoordinateInfo, OnInit
- auto suggestion: tier=A subsystem=CommNet

## Assembly-CSharp/SentinelMission/CometDetectionContract.cs
- 140 lines; kind=Contract; type=CometDetectionContract; bases=Contract,IUpdateWaypoints; ancestry=Contract/IContractParameterHost; ns=SentinelMission
- usings: Contracts, Contracts.Parameters, FinePrint, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=8
- ksp refs: Localizer=5, Reputation=4, ConfigNode=4, Planetarium=3, Contract=1; fan_in=0 fan_out=20
- methods(11): CanBeCancelled, CanBeDeclined, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, MeetRequirements, MessageCompleted, OnLoad, OnSave
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/SentinelMission/SentinelContract.cs
- 259 lines; kind=Contract; type=SentinelContract; bases=Contract,IUpdateWaypoints; ancestry=Contract/IContractParameterHost; ns=SentinelMission
- usings: Contracts, FinePrint, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: none; unity_score=7
- ksp refs: Planetarium=15, Localizer=11, Reputation=6, ConfigNode=4, CelestialBody=4, Orbit=2; fan_in=1 fan_out=28
- methods(12): CanBeCancelled, CanBeDeclined, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, MeetRequirements, MessageCompleted, OnLoad, OnSave, SetFocusBody
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/SentinelMission/SentinelModule.cs
- 114 lines; kind=PartModule; type=SentinelModule; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=SentinelMission
- usings: KSP.Localization
- unity: Lifecycle=1; unity_score=22
- ksp refs: Localizer=10, KSPField=2, KSPEvent=2, Planetarium=1, PartModule=1; fan_in=2 fan_out=10
- methods(7): CacheLocalStrings, FixedUpdate, OnStart, ShowMessage, StartTracking, StopTracking, TelescopeCanActivate
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/SentinelMission/SentinelParameter.cs
- 138 lines; kind=ContractParameter; type=SentinelParameter; bases=ContractParameter; ancestry=ContractParameter/IContractParameterHost; ns=SentinelMission
- usings: Contracts, FinePrint.Contracts.Parameters, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=9, Planetarium=3, Orbit=3, CelestialBody=3, ConfigNode=2, HighLogic=1; fan_in=2 fan_out=19
- methods(7): DiscoverAsteroid, GetHashString, GetNotes, GetTitle, OnLoad, OnRegister, OnSave
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/SentinelMission/SentinelScanType.cs
- 10 lines; kind=enum; type=SentinelScanType; bases=-; ancestry=-; ns=SentinelMission
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/SentinelMission/SentinelScenario.cs
- 340 lines; kind=ScenarioModule; type=SentinelScenario; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=SentinelMission
- usings: Contracts, FinePrint.Utilities, UnityEngine
- unity: Coroutine=5, Lifecycle=2, Instantiate/Destroy=1, GameObject=1; unity_score=36
- ksp refs: Vessel=16, Planetarium=10, Orbit=7, FlightGlobals=4, ConfigNode=4, ProtoPartSnapshot=2; fan_in=3 fan_out=31
- methods(12): ActiveSentinelMatchingOrbit, GetDeployedSentinels, LoadGameDBSettings, OnAwake, OnDestroy, OnLoad, ProcessSentinelScan, ResetSentinels, SentinelAsteroidOrbit, SentinelRoutine, SpawnComet, Start
- auto suggestion: tier=A subsystem=Contracts

## Assembly-CSharp/SentinelMission/SentinelUtilities.cs
- 412 lines; kind=static class; type=SentinelUtilities; bases=-; ancestry=-; ns=SentinelMission
- usings: FinePrint, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: CelestialBody=17, Planetarium=14, Vessel=5, Orbit=5, FlightGlobals=4, PartLoader=1; fan_in=6 fan_out=16
- methods(14): AdjustedSentinelViewAngle, CalculateReadDuration, FindInnerAndOuterBodies, GetEscapeVelocity, GetMinimumOrbitalSpeed, GetProgradeBurnAllowance, GetRetrogradeBurnAllowance, GetVesselClass, IsOnSolarOrbit, RandomRange, RandomScanType, SentinelCanScan, WeightedAsteroidClass, WeightedRandom
- auto suggestion: tier=A subsystem=Contracts
