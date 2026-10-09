# Review chunk 04 (root): 279 files, 73567 lines

## Assembly-CSharp/MouseEventsHandlerBase.cs
- 78 lines; kind=MonoBehaviour; type=MouseEventsHandlerBase; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=4, Transform=3, GameObject=3, Lifecycle=1, GetComponent=1; unity_score=70
- ksp refs: none; fan_in=2 fan_out=1
- methods(7): FindHandleUpwards, OnMouseDown, OnMouseDrag, OnMouseEnter, OnMouseExit, OnMouseUp, Start
- auto suggestion: tier=B subsystem=Input

## Assembly-CSharp/MouseRayEventsHandler.cs
- 82 lines; kind=MonoBehaviour; type=MouseRayEventsHandler; bases=MouseEventsHandlerBase; ancestry=MouseEventsHandlerBase/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Input.=7, Time.=3, Lifecycle=2, Raycast/Query=1; unity_score=75
- ksp refs: none; fan_in=2 fan_out=1
- methods(3): LateUpdate, OnRayHit, Start
- auto suggestion: tier=B subsystem=Input

## Assembly-CSharp/MultiModeEngine.cs
- 415 lines; kind=PartModule; type=MultiModeEngine; bases=PartModule,IEngineStatus; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=1; unity_score=18
- ksp refs: KSPField=10, KSPAction=4, GameEvents=4, KSPEvent=3, PartModule=2, HighLogic=2; fan_in=4 fan_out=13
- methods(17): ActivateAction, DisableAutoSwitch, EnableAutoSwitch, ModeAction, ModeEvent, OnAction, OnAutoSwitch, OnAwake, OnDestroy, OnLoad, OnStart, OnUpdate, SetPrimary, SetSecondary, ShutdownAction, ToggleMode, onVesselModified
- auto suggestion: tier=A subsystem=PartModules-Engines

## Assembly-CSharp/MultiOptionDialog.cs
- 112 lines; kind=class; type=MultiOptionDialog; bases=DialogGUIBase; ancestry=DialogGUIBase; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=3, Transform=1, IMGUI=1, GameObject=1; unity_score=33
- ksp refs: none; fan_in=81 fan_out=3
- methods(6): Create, FixedUpdate, LateUpdate, OnRenderObject, Resize, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/MunOrBustLogic.cs
- 16 lines; kind=MonoBehaviour; type=MunOrBustLogic; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Texture=1, Shader=1, MonoBehaviour=1, Material=1, Lifecycle=1; unity_score=80
- ksp refs: GameDatabase=1; fan_in=0 fan_out=1
- methods(1): Start
- auto suggestion: tier=B subsystem=Tutorials

## Assembly-CSharp/MusicLogic.cs
- 549 lines; kind=MonoBehaviour; type=MusicLogic; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI.Screens, UnityEngine, UnityEngine.SceneManagement
- unity: Audio=26, Coroutine=23, Time.=5, SceneManager=5, Transform=2, Lifecycle=2; unity_score=69
- ksp refs: GameSettings=10, GameEvents=8, TimeWarp=5, HighLogic=5, FlightGlobals=4, ResearchAndDevelopment=2; fan_in=6 fan_out=21
- methods(30): Awake, ConstructionMusic, CreditsMusic, Crossfade, FlightSpaceMusic, GetSettings, HandleEditorSwitch, MenuMusic, MissionBuilderMusic, OnDestroy, OnGameSceneChange, OnLevelLoaded, OnSceneLoaded, PauseWithCrossfade, PlayAmbienceLoop, PlayEditorAmbience, PlayFlight, PlayList, PlaySpaceCenter, PlayTheme, ResetConstValues, ResetFlightValues, ResetSpaceCenterValues, SetVolume, SpaceCenterMusic, TrackingStMusic, UnpauseWithCrossfade, WaitandPlayLoop, onGamePause, onGameUnpause
- auto suggestion: tier=C subsystem=Audio

## Assembly-CSharp/NavBallVector.cs
- 7 lines; kind=enum; type=NavBallVector; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/NavWaypoint.cs
- 338 lines; kind=MonoBehaviour; type=NavWaypoint; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: FinePrint, FinePrint.Utilities, KSP.Localization, KSP.UI.Screens.Flight, UnityEngine
- unity: Transform=17, Material=12, GameObject=11, GetComponent=5, SetActive=4, Renderer=4; unity_score=80
- ksp refs: FlightGlobals=5, HighLogic=4, Vessel=2, Planetarium=2, GameSettings=2, GameEvents=2; fan_in=6 fan_out=19
- methods(16): Activate, Awake, Clear, Deactivate, DeactivateIfWaypoint, IsUsing, IsVisible, LateUpdate, OnDestroy, OnGameSettingsApplied, SetMaterialColor, SetMaterialTexture, Setup, UpdateShowGhosted, UpdateVisualAppearance, UpdateVisualPosition
- auto suggestion: tier=C subsystem=Orbit

## Assembly-CSharp/NestedPrefabSpawner.cs
- 157 lines; kind=MonoBehaviour; type=NestedPrefabSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=14, Transform=13, LayerMask=8, Instantiate/Destroy=3, MonoBehaviour=1, Lifecycle=1; unity_score=83
- ksp refs: none; fan_in=1 fan_out=1
- methods(7): Awake, CleanUp, Despawn, Spawn, SpawnPrefabs, Start, setLayersAndTagsRecursive
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/NoiseEffect.cs
- 142 lines; kind=MonoBehaviour; type=NoiseEffect; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=7, Material=7, Texture=4, Screen/Cursor=4, Instantiate/Destroy=4, Lifecycle=2; unity_score=86
- ksp refs: none; fan_in=0 fan_out=0
- methods(4): OnDisable, OnRenderImage, SanitizeParameters, Start
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/NoiseGenerator.cs
- 190 lines; kind=class; type=NoiseGenerator; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=2
- ksp refs: none; fan_in=1 fan_out=0
- methods(9): GetSeed, SetSeed, b, coherentNoise, fastfloor, getDensity, kay, noise, shuffle
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/OcclusionCone.cs
- 121 lines; kind=class; type=OcclusionCone; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=2
- ksp refs: none; fan_in=2 fan_out=2
- methods(2): GetShockRadius, Setup
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/OcclusionCylinder.cs
- 26 lines; kind=class; type=OcclusionCylinder; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=9
- ksp refs: none; fan_in=2 fan_out=1
- methods(1): Setup
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/OcclusionData.cs
- 319 lines; kind=class; type=OcclusionData; bases=IComparable<OcclusionData>; ancestry=IComparable; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: Vector3d=6, GameSettings=3, Part=1; fan_in=4 fan_out=7
- methods(10): AreaOfIntersection, CompareTo, CreateCornerArray, GetConvectionMultVerts, GetCylinderOcclusion, GetCylinderOcclusionVerts, GetShockStats, RectRectIntersection, Update, sA
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/Orbit.cs **MUST-OPEN (>800 lines)**
- 1854 lines; kind=class; type=Orbit; bases=-; ancestry=-; ns=(global)
- usings: Smooth.Pools, UnityEngine
- unity: none; unity_score=7
- ksp refs: Vector3d=133, Orbit=68, ScaledSpace=28, Planetarium=25, CelestialBody=13, GameSettings=3; fan_in=96 fan_out=10
- methods(110): AscendingNodeTrueAnomaly, Clamp_dt, CreateRandomOrbitAround, CreateRandomOrbitFlyBy, CreateRandomOrbitNearby, DescendingNodeTrueAnomaly, DrawOrbit, DumpData, EccentricAnomalyAtObT, EccentricAnomalyAtUT, FindClosestPointsDelegate, FindClosestPoints_old, GetANVector, GetDTforTrueAnomaly, GetDTforTrueAnomalyAtUT, GetEccVector, GetEccentricAnomaly, GetFrameVel, GetFrameVelAtUT, GetMeanAnomaly, GetMeanMotion, GetNextApoapsisTime, GetNextPeriapsisTime, GetNextTimeOfRadius, GetOrbitNormal, GetOrbitalCurvatureAtTrueAnomaly, GetOrbitalStateVectorsAtObT, GetOrbitalStateVectorsAtTrueAnomaly, GetOrbitalStateVectorsAtUT, GetRadiusAtPhaseAngle, GetRadiusAtUT, GetRelativeInclination, GetRelativeVel, GetRotFrameVel, GetRotFrameVelAtPos, GetTimeToPeriapsis, GetTrueAnomaly, GetTrueAnomalyOfZupVector, GetUTforTrueAnomaly, GetVel, GetWorldSpaceVel, Halley_dt, Horizontal, Init, NextCloseApproachTime ...
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/OrbitDriver.cs
- 363 lines; kind=MonoBehaviour; type=OrbitDriver; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=4, Rigidbody=3, Renderer=3, Transform=2, MonoBehaviour=2, Instantiate/Destroy=2; unity_score=52
- ksp refs: CelestialBody=11, Vector3d=7, Planetarium=7, GameEvents=4, Vessel=3, TimeWarp=3; fan_in=28 fan_out=15
- methods(13): Awake, CelestialBodyDelegate, CheckDominantBody, FixedUpdate, OnDestroy, OnRailsSOITransition, RecalculateOrbit, SetOrbitMode, Start, TrackRigidbody, UpdateOrbit, unlockFrameSwitch, updateFromParameters
- auto suggestion: tier=B subsystem=Orbit

## Assembly-CSharp/OrbitPhysicsManager.cs
- 353 lines; kind=MonoBehaviour; type=OrbitPhysicsManager; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Rigidbody=17, Transform=6, Lifecycle=4, MonoBehaviour=2, Coroutine=2, Instantiate/Destroy=1; unity_score=64
- ksp refs: FlightGlobals=32, Vector3d=7, CelestialBody=7, Vessel=6, TimeWarp=6, GameEvents=4; fan_in=2 fan_out=12
- methods(14): Awake, CheckReferenceFrame, FixedUpdate, HoldVesselUnpack, LateUpdate, OnDestroy, SetDominantBody, SetReverseOrbit, SetRotatingFrame, Start, ToggleRotatingFrame, checkReferenceFrame, setDominantBody, setRotatingFrame
- auto suggestion: tier=B subsystem=Orbit

## Assembly-CSharp/OrbitRenderer.cs
- 123 lines; kind=MonoBehaviour; type=OrbitRenderer; bases=OrbitRendererBase; ancestry=OrbitRendererBase/MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.EventSystems
- unity: Lifecycle=2, Shader=1, Instantiate/Destroy=1, EventSystems=1; unity_score=45
- ksp refs: MapView=3, GameEvents=2; fan_in=7 fan_out=15
- methods(10): CanDrawAnyIcons, DrawOrbit, GetActive, GetNodeColour, GetOrbitColour, IsDebris, OnDestroy, OnSceneChange, OrbitCast, Start
- auto suggestion: tier=B subsystem=MapView

## Assembly-CSharp/OrbitRendererBase.cs **MUST-OPEN (>800 lines)**
- 994 lines; kind=MonoBehaviour; type=OrbitRendererBase; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions.Missions, Expansions.Missions.Actions, KSP.Localization, KSP.UI.Screens.Mapview, UnityEngine, Vectrosity
- unity: Transform=6, Shader=4, Material=3, Lifecycle=3, Camera=3, Time.=2; unity_score=38
- ksp refs: Localizer=35, ScaledSpace=18, Vessel=8, Vector3d=8, MapView=6, FlightGlobals=6; fan_in=11 fan_out=46
- methods(45): ANDNNodes_OnUpdateIcon, ApNode_OnUpdateIcon, AttachNodeUIs, CacheLocalStrings, CanDrawAnyIcons, CreateScaledSpaceNodes, DestroyScaledSpaceNodes, DetachNodeUIs, DrawNodes, DrawOrbit, DrawSpline, FindMapObject, GetCurrentDrawMode, GetMouseOverNode, GetNodeColour, GetOrbitColour, GetScreenSpacePoint, GetSegmentCount, GetUpdatedOrbitPoint, HaveStateVectorKnowledge, IsRenderableOrbit, LateUpdate, MakeLine, OnDestroy, OrbitCast, PeNode_OnUpdateIcon, RefreshMapObject, SetColor, SplineOpacityUpdate, Start, UpdateSpline, apNode_OnUpdateCaption, apNode_OnUpdatePosition, ascNode_OnUpdateCaption, ascNode_OnUpdatePosition, descNode_OnUpdateCaption, descNode_OnUpdatePosition, getCamVsSmaRatio, objectNode_OnClick, objectNode_OnUpdateCaption, objectNode_OnUpdateIcon, objectNode_OnUpdatePosition, objectNode_OnUpdateType, peNode_OnUpdateCaption, peNode_OnUpdatePosition
- auto suggestion: tier=B subsystem=MapView

## Assembly-CSharp/OrbitRendererData.cs
- 19 lines; kind=class; type=OrbitRendererData; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: CelestialBody=2; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=MapView

## Assembly-CSharp/OrbitSnapshot.cs
- 93 lines; kind=class; type=OrbitSnapshot; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Orbit=3, ConfigNode=3, FlightGlobals=2, CelestialBody=1; fan_in=4 fan_out=4
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/OrbitTargeter.cs **MUST-OPEN (>800 lines)**
- 1081 lines; kind=MonoBehaviour; type=OrbitTargeter; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, KSP.UI.Screens.Mapview, KSP.UI.Screens.Mapview.MapContextMenuOptions, PreFlightTests, UnityEngine, Vectrosity
- unity: Renderer=7, Lifecycle=5, GameObject=3, Input.=2, GetComponent=2, Shader=1; unity_score=35
- ksp refs: Vector3d=34, Orbit=15, Localizer=13, MapView=10, Planetarium=8, OrbitDriver=8; fan_in=3 fan_out=43
- methods(38): Awake, CacheLocalStrings, CanDrawAnyLines, CanDrawAnyNode, CleanupMarker, ClearMenus, CursorInputUpdate, DestroyLine, DottedLineUpdate, DrawLine, DropInvalidTargets, FindPatch, GetETAString, GetSeparations, IsTargetCb, LateUpdate, NodeCleanup, NodeUpdate, OnDestroy, OnUpdateCaption, OnUpdatePosition, OnUpdateType, OnUpdateVisible, ReferencePatchSelect, SetMenuMode, SetTarget, Start, TargetCastNodes, TargetCastSplines, Terminate, Update, UpdateISectMarkers, UpdateNodesAndVectors, getTargetReferencePatch, onContextMenuDismiss, onTrackingStationDismiss, onTrackingStationProceed, onViewInTrackingStationButton
- auto suggestion: tier=C subsystem=Orbit

## Assembly-CSharp/OrbitTargetRenderer.cs
- 177 lines; kind=MonoBehaviour; type=OrbitTargetRenderer; bases=OrbitRenderer; ancestry=OrbitRenderer/OrbitRendererBase/MonoBehaviour; ns=(global)
- usings: FinePrint, FinePrint.Utilities, KSP.UI.Screens.Mapview, UnityEngine
- unity: GameObject=4, Transform=2, GetComponent=2, Time.=1, Shader=1, Renderer=1; unity_score=53
- ksp refs: Vector3d=4, ScaledSpace=4, FlightGlobals=4, OrbitDriver=2, MapView=2, Vessel=1; fan_in=2 fan_out=19
- methods(11): ANDNNodes_OnUpdateIcon, AnimateOrbit, CanDrawAnyIcons, CheckVisibility, Cleanup, LockOrbit, Setup, Update, UpdateLocals, ascNode_OnUpdatePosition, descNode_OnUpdatePosition
- auto suggestion: tier=B subsystem=MapView

## Assembly-CSharp/OrbitUtil.cs
- 159 lines; kind=class; type=OrbitUtil; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Orbit=15, Vector3d=13, CelestialBody=13; fan_in=4 fan_out=5
- methods(10): CalculateIntersections, CurrentEjectionAngle, CurrentPhaseAngle, FindCommonAncestor, FindPlanet, GetBodyHierarchy, GetSmaDistance, GetSmaDistanceToSun, GetTransferPhaseAngle, GetTransferTime
- auto suggestion: tier=A subsystem=Orbit

## Assembly-CSharp/OverlayGenerator.cs
- 550 lines; kind=MonoBehaviour; type=OverlayGenerator; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Texture=11, Lifecycle=2, MonoBehaviour=1, GetComponent=1, GameObject=1; unity_score=43
- ksp refs: PartResourceDefinition=10, MapView=4, CelestialBody=4, FlightGlobals=3, GameEvents=2, Vessel=1; fan_in=8 fan_out=22
- methods(24): AverageDots, AverageLine, Awake, ClearDisplay, CreateTexture, FetchNextResource, FetchOverlayColor, GenerateOverlay, GenerateScanPoint, GetColor, GetLerp, GetMaxAbundance, GetMinAbundance, GetPoint, GetTexture, Interpolate, InvertColor, LoadResourceList, OnDestroy, OnEnterMapView, OnExitMapView, OnGameLoaded, OnMapFocusChange, ResetRNG
- auto suggestion: tier=C subsystem=Misc

## Assembly-CSharp/OverlaySetup.cs
- 75 lines; kind=MonoBehaviour; type=OverlaySetup; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, GetComponent=1, GameObject=1; unity_score=45
- ksp refs: ConfigNode=2, GameDatabase=1; fan_in=2 fan_out=4
- methods(3): LoadOverlayConfig, OnDestroy, ResetMapConfig
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/ParseExtensions.cs
- 430 lines; kind=static class; type=ParseExtensions; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=12
- ksp refs: QuaternionD=7, Vector3d=6; fan_in=5 fan_out=4
- methods(15): ArrayMinSize, ParseArray, TryParseBoolArray, TryParseColor, TryParseColor32, TryParseEnumIntArray, TryParseQuaternion, TryParseQuaternionD, TryParseRect, TryParseVector2, TryParseVector2d, TryParseVector3, TryParseVector3d, TryParseVector4, TryParseVector4d
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/ParserMethod.cs
- 1 lines; kind=delegate; type=ParserMethod; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/Part.cs **MUST-OPEN (>800 lines)**
- 9023 lines; kind=MonoBehaviour; type=Part; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler; ancestry=MonoBehaviour; ns=(global)
- usings: CommNet, Expansions, Expansions.Missions.Adjusters, Expansions.Serenity, Experience, Highlighting, KSP.IO, KSP.Localization, KSP.UI, KSP.UI.Screens, ModuleWheels, UnityEngine, UnityEngine.EventSystems, Vectrosity
- unity: Transform=130, GameObject=123, Rigidbody=60, GetComponent=59, LayerMask=27, Instantiate/Destroy=26; unity_score=55
- ksp refs: Part=173, GameEvents=82, HighLogic=72, AttachNode=58, PartModule=44, Vessel=36; fan_in=389 fan_out=171
- methods(386): AddAttachNode, AddCrewmember, AddCrewmemberAt, AddExposedThermalFlux, AddForce, AddForceAtPosition, AddImpulse, AddInternalPart, AddModule, AddOnMouseDown, AddOnMouseEnter, AddOnMouseExit, AddPartModuleAdjusterList, AddResource, AddShield, AddSkinThermalFlux, AddThermalFlux, AddTorque, AimCamera, AllowAutoStruts, AlreadyProcessedRequest, ApplyRigidAttachment, AttachedPartNode, AttributeSetup, Awake, CacheIConstructionPartModules, CacheLocalStrings, CanCrossfeed, ChangeResourcePriority, CheckAutoStruts, CheckBodyLiftAttachment, CheckCollision, CheckOtherGroundContact, CheckTargetAutoStruts, CheckTransferDialog, CleanSymmetryReferences, CleanSymmetryVesselReferences, CleanSymmetryVesselReferencesRecursively, ClearModuleReferenceCache, Couple, CreateAttachJoint, CreateEventList, CreateEventListRecursive, CreateInternalModel, CreateRendererLists ...
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/PartBuoyancy.cs
- 676 lines; kind=MonoBehaviour; type=PartBuoyancy; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Rigidbody=15, SendMessage/Invoke=3, MonoBehaviour=2, Lifecycle=2, GetComponent=2, Instantiate/Destroy=1; unity_score=45
- ksp refs: PhysicsGlobals=55, Vector3d=14, FlightGlobals=13, TimeWarp=8, Part=4, Krakensbane=3; fan_in=2 fan_out=17
- methods(6): FindOverrideCube, FixedUpdate, OnDrawGizmosSelected, ReportSplashDown, Start, UpdateDisplacement
- auto suggestion: tier=B subsystem=Aero

## Assembly-CSharp/PartCategories.cs
- 41 lines; kind=enum; type=PartCategories; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=9 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/PartCrewManifest.cs
- 274 lines; kind=class; type=PartCrewManifest; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=1
- ksp refs: ProtoCrewMember=14, HighLogic=9, KerbalRoster=4, GameEvents=2, AvailablePart=2, PartLoader=1; fan_in=15 fan_out=9
- methods(14): AddCrewToSeat, AllSeatsEmpty, AnySeatTaken, AnySeats, CloneOf, Contains, CountCrewNotType, CountCrewType, FromConfigNode, GetCrewSeat, GetPartCrew, NoSeats, RemoveCrew, RemoveCrewFromSeat
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/PartGameObjectInfo.cs
- 22 lines; kind=class; type=PartGameObjectInfo; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=29
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/PartGeometryUtil.cs
- 251 lines; kind=static class; type=PartGeometryUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Renderer=8, Collider=4, GetComponent=3, GameObject=3, Transform=2, ParticleSystem=1; unity_score=51
- ksp refs: Part=5; fan_in=7 fan_out=2
- methods(8): FindBoundsCentroid, GetColliderBounds, GetPartColliderBounds, GetPartRendererBound, GetPartRendererBounds, GetRendererBounds, GetRendererBoundsWithoutParticles, MergeBounds
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/PartHeightQuery.cs
- 14 lines; kind=class; type=PartHeightQuery; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=2; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/ParticleBillboardModifier.cs
- 24 lines; kind=MonoBehaviour; type=ParticleBillboardModifier; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Mesh=1; unity_score=55
- ksp refs: none; fan_in=0 fan_out=0
- methods(3): ExploreSystemProperties, ReplaceHorizontalBillboardsWithMesh, Test
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ParticleColliderLaunchSequence.cs
- 97 lines; kind=MonoBehaviour; type=ParticleColliderLaunchSequence; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Coroutine=4, Transform=2, Collider=2, Time.=1, MonoBehaviour=1, Lifecycle=1; unity_score=68
- ksp refs: none; fan_in=0 fan_out=0
- methods(4): GoForLaunch, LaunchSequence, Reset, Start
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ParticleConfigFX.cs
- 330 lines; kind=MonoBehaviour; type=ParticleConfigFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=28, Transform=5, Material=5, GetComponent=3, Shader=2, GameObject=2; unity_score=78
- ksp refs: ConfigNode=5, GameDatabase=1; fan_in=0 fan_out=7
- methods(8): CreateMaterial, InitializeComponents, InitializeParticleSystem, OnEvent, OnInitialize, OnLoad, OnSave, UpdateComponents
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ParticleSystemExt.cs
- 16 lines; kind=static class; type=ParticleSystemExt; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=5; unity_score=67
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): GetParticleBuffer
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/PartInfo.cs
- 17 lines; kind=attribute; type=PartInfo; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=18 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/PartItemTransfer.cs
- 198 lines; kind=MonoBehaviour; type=PartItemTransfer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Highlighting, KSP.Localization, UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1, KeyCode=1, Instantiate/Destroy=1, Input.=1, GameObject=1; unity_score=39
- ksp refs: Part=17, GameEvents=8, Localizer=6, Vessel=4, InputLockManager=2; fan_in=8 fan_out=12
- methods(18): AfterPartsFound, Dismiss, DismissActive, DismissAlreadyRunning, DismissInterrupted, HookAdditionalEvents, IsSemiValidPart, IsValidPart, LateUpdate, OnDestroy, OnPartSelect, OnSemiValidPartSelect, OnSrcPartSelect, Setup, UnhookAdditionalEvents, onVesselChanged, onVesselModified, onVesselSituationChange
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/PartJoint.cs
- 500 lines; kind=MonoBehaviour; type=PartJoint; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Joint=29, GameObject=14, Rigidbody=13, Transform=9, Instantiate/Destroy=4, GetComponent=2; unity_score=79
- ksp refs: Part=16, PartJoint=11, AttachNode=9, PhysicsGlobals=8, GameEvents=3, FlightGlobals=2; fan_in=10 fan_out=8
- methods(17): Create, DestroyJoint, JointToLocalSpaceDir, JointToLocalSpacePos, JointToLocalSpaceRot, LocaltoJointSpaceDir, LocaltoJointSpacePos, LocaltoJointSpaceRot, OnDestroy, OnJointBreak, OnPartPack, OnPartUnpack, SetBreakingForces, SetJointLimits, SetUnbreakable, SetupJoint, create
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/PartLoader.cs **MUST-OPEN (>800 lines)**
- 2205 lines; kind=MonoBehaviour; type=PartLoader; bases=LoadingSystem; ancestry=LoadingSystem/MonoBehaviour; ns=(global)
- usings: Expansions.Serenity, KSP.Localization, KSP.UI.Screens, UnityEngine, UnityEngine.Rendering
- unity: GameObject=122, Transform=61, Material=47, Renderer=25, GetComponent=23, Shader=22; unity_score=78
- ksp refs: AvailablePart=53, ConfigNode=27, Part=25, GameDatabase=19, PartLoader=10, FlightGlobals=10; fan_in=74 fan_out=47
- methods(64): AddPartReplacement, AddVariants, ApplyPartValue, Awake, CheckPartDatabaseTimestamps, CheckPartDatabaseVersion, ClearAll, CompileAll, CompileInternalProps, CompileInternalSpaces, CompileModel, CompilePartInfo, CompileParts, CompileVariantThemes, ConstructSoundFXGroup, CreatePart, CreatePartDatabase, CreatePartIcon, DebugHeirarchy, DoesPartExist, DoesPartHaveReplacement, GetAvailableAndPurchaseableCargoParts, GetAvailableCargoInventoryPartNames, GetAvailableCargoInventoryParts, GetAvailableCargoPartNames, GetAvailableCargoParts, GetAvailableDeployedScienceExpParts, GetAvailableRobotArmScannerParts, GetDatabaseConfig, GetInternalPart, GetInternalProp, GetPartBounds, GetPartCollider, GetPartReplacementName, GetUnlockedCargoPartNames, GetUnlockedCargoParts, GetVariantInfoByName, InferBulkheadProfiles, InitializePartDatabase, IsReady, LoadInternalProp, LoadInternalSpace, LoadSound, OnDestroy, ParsePart ...
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/PartMaterialInfo.cs
- 122 lines; kind=class; type=PartMaterialInfo; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: UnityEngine
- unity: Shader=6, Texture=4, Material=2; unity_score=55
- ksp refs: ConfigNode=3, GameDatabase=2; fan_in=1 fan_out=3
- methods(4): ApplyCommand, ApplyCommands, Load, Save
- auto suggestion: tier=C subsystem=Part

## Assembly-CSharp/PartModule.cs **MUST-OPEN (>800 lines)**
- 1256 lines; kind=PartModule; type=PartModule; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, Expansions.Missions, Expansions.Missions.Adjusters, KSP.Localization, UnityEngine
- unity: GetComponent=3, Coroutine=2, MonoBehaviour=1, Lifecycle=1; unity_score=22
- ksp refs: ConfigNode=33, PartModule=13, KSPField=10, Part=6, Localizer=4, HighLogic=4; fan_in=167 fan_out=33
- methods(77): AddPartModuleAdjuster, AddPartModuleAdjusterList, AppliedUpgrades, ApplyAdjustersOnStart, ApplyUpgradeNode, ApplyUpgrades, Awake, ClearPersistentId, Copy, DemoteToPhysicslessPart, FindUpgrades, GetCurrentColor, GetInfo, GetModuleDisplayName, GetPersistenActiontId, GetPersistentId, GetReflectedAttributes, GetStagingDisableText, GetStagingEnableText, GetUpgrade, GetUpgradeInfo, HasUpgrades, IsStageable, IsValidContractObjective, Load, LoadUpgrades, LoadUpgradesApplied, ModularSetup, NewPersistentId, OnActive, OnAwake, OnColorChanged, OnConstructionModeFixedUpdate, OnConstructionModeUpdate, OnCopy, OnFixedUpdate, OnIconCreate, OnInactive, OnInitialize, OnInventoryModeDisable, OnInventoryModeEnable, OnLoad, OnModuleAdjusterAdded, OnModuleAdjusterAddedWrapper, OnModuleAdjusterRemoved ...
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/PartModuleFXSetter.cs
- 48 lines; kind=PartModule; type=PartModuleFXSetter; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: -
- unity: none; unity_score=15
- ksp refs: PartModule=1, KSPField=1; fan_in=2 fan_out=3
- methods(3): OnAwake, OnStart, SetFXModules
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/PartModuleList.cs
- 438 lines; kind=class; type=PartModuleList; bases=IEnumerable; ancestry=IEnumerable; ns=(global)
- usings: UnityEngine
- unity: GameObject=2, Instantiate/Destroy=1, GetComponent=1; unity_score=10
- ksp refs: PartModule=23, Part=2; fan_in=1 fan_out=6
- methods(14): Add, Clear, Contains, FindEngineInterface, FindEngineNearby, GetEnumerator, GetModule, GetModules, GetScalarModule, GetScalarModules, IndexOf, Remove, Send, SendDefault
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/PartModuleUtil.cs
- 60 lines; kind=static class; type=PartModuleUtil; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(1): PrintResourceRequirements
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/PartPointer.cs
- 19 lines; kind=MonoBehaviour; type=PartPointer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=44
- ksp refs: Part=4; fan_in=3 fan_out=1
- methods(2): GetPart, SetPart
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/PartRawInfo.cs
- 22 lines; kind=class; type=PartRawInfo; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=29
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/PartReader.cs **MUST-OPEN (>800 lines)**
- 1104 lines; kind=static class; type=PartReader; bases=-; ancestry=-; ns=(global)
- usings: PartToolsLib, UnityEngine, UnityEngine.Rendering
- unity: Shader=50, GameObject=46, Material=41, Transform=20, Collider=20, Animation=19; unity_score=73
- ksp refs: GameDatabase=1; fan_in=1 fan_out=9
- methods(25): AddMaterialDummy, AddTextureDummy, Contains, FindChildByName, Get, NormalMapToUnityNormalMap, Read, ReadAnimation, ReadAnimationEvents, ReadCamera, ReadChild, ReadColor, ReadLight, ReadMaterial, ReadMaterial4, ReadMaterialTexture, ReadMesh, ReadMeshRenderer, ReadParticles, ReadSkinnedMeshRenderer, ReadTagAndLayer, ReadTextures, ReadVector2, ReadVector3, ReadVector4
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/PartResource.cs
- 231 lines; kind=class; type=PartResource; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=2
- ksp refs: PartResource=10, GameEvents=4, Part=3, PartResourceDefinition=2, ConfigNode=2; fan_in=34 fan_out=5
- methods(7): CanProvide, Copy, Flowing, GetInfo, Load, Save, SetInfo
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/PartResourceDefinition.cs
- 217 lines; kind=class; type=PartResourceDefinition; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=22
- ksp refs: ConfigNode=9, PartResourceDefinition=4, Localizer=4; fan_in=63 fan_out=7
- methods(4): GetFlowModeDescription, GetShortName, Load, Save
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/PartResourceDefinitionList.cs
- 93 lines; kind=class; type=PartResourceDefinitionList; bases=IEnumerable; ancestry=IEnumerable; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: PartResourceDefinition=10, ConfigNode=1; fan_in=1 fan_out=3
- methods(4): Add, Clear, Contains, GetEnumerator
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/PartResourceDrainDefinition.cs
- 77 lines; kind=class; type=PartResourceDrainDefinition; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=24
- ksp refs: ConfigNode=2; fan_in=1 fan_out=1
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/PartResourceLibrary.cs
- 90 lines; kind=MonoBehaviour; type=PartResourceLibrary; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, Lifecycle=2, Instantiate/Destroy=2, GameObject=2, MonoBehaviour=1; unity_score=66
- ksp refs: PartResourceDefinition=3, GameDatabase=1, ConfigNode=1; fan_in=55 fan_out=5
- methods(6): Awake, GetDefaultFlowMode, GetDefinition, LoadDefinitions, OnDestroy, Reset
- auto suggestion: tier=B subsystem=Resources

## Assembly-CSharp/PartResourceList.cs
- 352 lines; kind=class; type=PartResourceList; bases=IEnumerable<PartResource>,IEnumerable; ancestry=IEnumerable; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=9
- ksp refs: PartResource=36, Part=7, GameEvents=6, PartResourceDefinition=2, ConfigNode=1; fan_in=3 fan_out=8
- methods(15): Add, Clear, Contains, Get, GetAll, GetAllFlowing, GetEnumerator, GetFlowing, GetFlowingTotals, HasFlowable, HasFlowableUnhidden, Initializer, RefListInitializer, RefreshSimulationListAmounts, Remove
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/PartSelector.cs
- 98 lines; kind=MonoBehaviour; type=PartSelector; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, GetComponent=2, GameObject=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=58
- ksp refs: Part=9; fan_in=2 fan_out=2
- methods(7): Create, Dismiss, LateUpdate, OnDestroy, OnMouseEntered, OnMouseExited, Setup
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/PartSet.cs **MUST-OPEN (>800 lines)**
- 1022 lines; kind=class; type=PartSet; bases=-; ancestry=-; ns=(global)
- usings: RUI.Algorithms, UnityEngine
- unity: Resources.=1; unity_score=5
- ksp refs: PartResource=44, Part=39, Vessel=8, GameEvents=6, ShipConstruct=5, AttachNode=2; fan_in=6 fan_out=9
- methods(21): AddPartResource, AddPartToSet, BuildPartSets, BuildPartSimulationSets, BuildResList, ContainsPart, GetConnectedResourceTotals, GetOrCreateList, GetParts, GetResourceList, HookEvents, OnFlowModeChange, OnFlowStateChange, ProcessRequest, RebuildInPlace, RebuildParts, RebuildVessel, RemovePart, RemovePartResource, RequestResource, SetsEqual
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/PartSpaceMode.cs
- 5 lines; kind=enum; type=PartSpaceMode; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/PartStates.cs
- 10 lines; kind=enum; type=PartStates; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=34 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/PartStatsUpgradeModule.cs
- 146 lines; kind=PartModule; type=PartStatsUpgradeModule; bases=PartModule,IModuleInfo,IPartMassModifier,IPartCostModifier; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens.Editor, UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=9, Localizer=5, PartModule=1, PartLoader=1; fan_in=0 fan_out=11
- methods(10): GetDrawModulePanelCallback, GetInfo, GetModuleCost, GetModuleCostChangeWhen, GetModuleDisplayName, GetModuleMass, GetModuleMassChangeWhen, GetModuleTitle, GetPrimaryField, OnLoad
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/PartTestConstraint.cs
- 322 lines; kind=class; type=PartTestConstraint; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: Contracts, UnityEngine
- unity: none; unity_score=5
- ksp refs: Contract=4, ConfigNode=3, Vessel=2, Planetarium=2, CelestialBody=1; fan_in=4 fan_out=7
- methods(6): BodyAvailable, Load, PrestigeAvailable, Save, SituationAvailable, Test
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/PartThermalData.cs
- 170 lines; kind=class; type=PartThermalData; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=11
- ksp refs: GameSettings=4, Part=2, PartModule=1, KerbalEVA=1; fan_in=5 fan_out=11
- methods(1): GetUnifiedSkinTemp
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/PartThermalDataIntComparer.cs
- 17 lines; kind=class; type=PartThermalDataIntComparer; bases=IComparer<PartThermalData>; ancestry=IComparer; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(1): Compare
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/PartThermalDataSkinComparer.cs
- 17 lines; kind=class; type=PartThermalDataSkinComparer; bases=IComparer<PartThermalData>; ancestry=IComparer; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(1): Compare
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/PartTools.cs
- 113 lines; kind=MonoBehaviour; type=PartTools; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=26, Texture=3, MonoBehaviour=1, Material=1; unity_score=92
- ksp refs: none; fan_in=1 fan_out=1
- methods(2): Reset, SetDefaultShaders
- auto suggestion: tier=C subsystem=Part

## Assembly-CSharp/PartToolsLibrary.cs
- 24 lines; kind=MonoBehaviour; type=PartToolsLibrary; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.SceneManagement
- unity: Texture=2, SceneManager=1, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1, GameObject=1; unity_score=83
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): Awake
- auto suggestion: tier=B subsystem=Part

## Assembly-CSharp/PartUpgradeHandler.cs
- 468 lines; kind=class; type=PartUpgradeHandler; bases=IEnumerable; ancestry=IEnumerable; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=1
- ksp refs: HighLogic=30, Game=14, ConfigNode=10, PartModule=7, Part=5, ResearchAndDevelopment=2; fan_in=10 fan_out=15
- methods(23): AddUpgrade, AddUsedBy, CanHaveUpgrades, FillUpgrades, GetEnumerator, GetUpgrade, GetUpgradeCost, GetUpgradesForTech, GetUsedBy, GetUsedByStrings, IsAvailableToUnlock, IsEnabled, IsUnlocked, IsUsed, LinkUpgrades, OnLoad, OnSave, RemoveUpgrade, SetEnabled, SetFromInfo, SetFromUntrackedUpgrade, SetUnlocked, UgpradesAllowed
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/PartUpgradeManager.cs
- 59 lines; kind=ScenarioModule; type=PartUpgradeManager; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=1, Instantiate/Destroy=1; unity_score=36
- ksp refs: ConfigNode=2, ScenarioModule=1; fan_in=7 fan_out=6
- methods(4): OnAwake, OnDestroy, OnLoad, OnSave
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/PartValues.cs
- 32 lines; kind=class; type=PartValues; bases=EventValueWrapper; ancestry=EventValueWrapper; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=16 fan_out=3
- methods(0): 
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/PartVariant.cs
- 592 lines; kind=class; type=PartVariant; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: UnityEngine
- unity: Material=15, Transform=11, Shader=5, SetActive=1, GetComponent=1, GameObject=1; unity_score=42
- ksp refs: AttachNode=18, ConfigNode=10, PartLoader=1, HighLogic=1, GameDatabase=1; fan_in=17 fan_out=12
- methods(11): FindChild, GetAttachedNodeIndex, GetExtraInfoValue, Load, MaterialExists, ParseNode, Save, TryCopyMaterial, UpdateMaterial, UpdateMaterialFromExtraInfo, UpdateModel
- auto suggestion: tier=C subsystem=Part

## Assembly-CSharp/PAsteroid.cs
- 60 lines; kind=MonoBehaviour; type=PAsteroid; bases=PSpaceObject; ancestry=PSpaceObject/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GetComponent=2, GameObject=2, Transform=1, Renderer=1; unity_score=61
- ksp refs: Part=1; fan_in=4 fan_out=5
- methods(1): SetupPartParameters
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/PatchedConicRenderer.cs
- 410 lines; kind=MonoBehaviour; type=PatchedConicRenderer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens.Mapview, UnityEngine
- unity: Transform=4, Lifecycle=4, Shader=3, Time.=2, MonoBehaviour=1, GetComponent=1; unity_score=40
- ksp refs: MapView=14, GameEvents=6, Vector3d=4, Planetarium=3, Localizer=3, GameSettings=3; fan_in=7 fan_out=25
- methods(15): AddManeuverNode, Awake, CanDrawAnyNode, FindRenderingForPatch, GetPatchColor, LateUpdate, ManeuverUINode_OnClick, ManeuverUINode_OnUpdateCaption, ManeuverUINode_OnUpdatePosition, ManeuverUINode_OnUpdateVisible, OnDestroy, OnGameSettingsApplied, OnSceneSwitch, SetMouseOverGizmo, Start
- auto suggestion: tier=B subsystem=MapView

## Assembly-CSharp/PatchedConics.cs
- 472 lines; kind=class; type=PatchedConics; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Camera=6, Raycast/Query=1, Input.=1; unity_score=25
- ksp refs: Orbit=38, ScaledSpace=12, Vector3d=9, OrbitDriver=5, CelestialBody=4, QuaternionD=2; fan_in=8 fan_out=12
- methods(20): AngleWrap, CalculatePatchDelegate, CheckEncounterDelegate, EncountersBodyDelegate, GetClosestApproachDelegate, GetScreenSpacePoint, GetUpdatedOrbitPoint, GetUpdatedOrigin, ScreenCastDelegate, ScreenCastWorkerDelegate, SolveDynamicTA_BSP, SolveLocalTA_BSP, SolveRelativeTA_BSP, TAIsWithinPatchBounds, _CalculatePatch, _CheckEncounter, _EncountersBody, _GetClosestApproach, _ScreenCast, _ScreenCastWorker
- auto suggestion: tier=B subsystem=Orbit

## Assembly-CSharp/PatchedConicSolver.cs
- 377 lines; kind=MonoBehaviour; type=PatchedConicSolver; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: Lifecycle=3, Time.=1, MonoBehaviour=1, GetComponent=1, GameObject=1, Coroutine=1; unity_score=31
- ksp refs: Orbit=20, OrbitDriver=10, Planetarium=6, GameEvents=6, Vector3d=5, GameSettings=3; fan_in=12 fan_out=16
- methods(17): AddManeuverNode, Awake, CheckNextManeuver, DecreasePatchLimit, FindManeuverIndex, FindPatchContainingUT, IncreasePatchLimit, Load, OnDestroy, OnGameSettingsApplied, OnReferenceBodyChange, RemoveManeuverNode, Save, SortNodesByDate, Start, Update, UpdateFlightPlan
- auto suggestion: tier=B subsystem=Orbit

## Assembly-CSharp/PatchRendering.cs
- 682 lines; kind=class; type=PatchRendering; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens.Mapview, UnityEngine, Vectrosity
- unity: Material=3, Shader=2, LayerMask=1, Instantiate/Destroy=1, GameObject=1; unity_score=14
- ksp refs: Orbit=26, Vector3d=10, ScaledSpace=10, Localizer=7, Planetarium=5, MapView=5; fan_in=4 fan_out=16
- methods(29): AttachUINodes, CanDrawAnyNode, CreateMapObjects, DestroyMapObjects, DestroyUINodes, DestroyVector, DetachUINodes, DrawSplines, GetScaledSpacePointFromTA, MakeVector, SetColor, SetColorSegments, Terminate, UpdatePR, UpdateSpline, UpdateUINodes, mnAp_OnUpdateCaption, mnAp_OnUpdateVisible, mnCB_OnUpdateCaption, mnCB_OnUpdateVisible, mnCBatUT_OnUpdatePosition, mnCBatUT_OnUpdateVisible, mnEnd_OnUpdateCaption, mnEnd_OnUpdateType, mnEnd_OnUpdateVisible, mnNextStart_OnUpdateType, mnNextStart_OnUpdateVisible, mnPe_OnUpdateCaption, mnPe_OnUpdateVisible
- auto suggestion: tier=B subsystem=MapView

## Assembly-CSharp/PauseAudioFadeHandler.cs
- 79 lines; kind=MonoBehaviour; type=PauseAudioFadeHandler; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Coroutine=7, Audio=5, Time.=2, MonoBehaviour=1; unity_score=78
- ksp refs: none; fan_in=1 fan_out=0
- methods(3): PauseWithFade, PauseWithFadeRoutine, UnpauseWithFade
- auto suggestion: tier=C subsystem=Audio

## Assembly-CSharp/PauseMenu.cs
- 756 lines; kind=MonoBehaviour; type=PauseMenu; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, Expansions.Missions.Editor, Expansions.Missions.Runtime, KSP.Localization, KSP.UI, KSP.UI.Screens, KSP.UI.Screens.DebugToolbar, UnityEngine, UnityEngine.UI
- unity: GameObject=33, SetActive=30, Lifecycle=4, Analytics=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=59
- ksp refs: HighLogic=93, Localizer=61, MissionSystem=15, GameEvents=11, Game=8, FlightGlobals=8; fan_in=4 fan_out=48
- methods(21): Awake, BuildButtonList, CancelExitToMainMenuConfirmation, Close, Display, MissionModeValid, OnDestroy, ShowExitToMainConfirmation, Start, SubmitExitToMainMenuConfirmation, Update, draw, drawExitScenarioOptions, drawExitTestMissionOptions, drawExitWithoutSaveOptions, drawMissionRestartOptions, drawMissionsRevertOptions, drawRevertOptions, drawStockRevertOptions, miniSettingsFinished, saveAndExit
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/PComet.cs
- 60 lines; kind=MonoBehaviour; type=PComet; bases=PSpaceObject; ancestry=PSpaceObject/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GetComponent=2, GameObject=2, Transform=1, Renderer=1; unity_score=61
- ksp refs: Part=1; fan_in=3 fan_out=5
- methods(1): SetupPartParameters
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/PDebug.cs
- 62 lines; kind=static class; type=PDebug; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=21
- ksp refs: PartLoader=1, GameSettings=1, GameDatabase=1, ConfigNode=1; fan_in=17 fan_out=4
- methods(4): Error, General, Log, Warning
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/Performance.cs
- 219 lines; kind=MonoBehaviour; type=Performance; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Time.=5, Lifecycle=4, Resources.=1, MonoBehaviour=1; unity_score=45
- ksp refs: none; fan_in=3 fan_out=0
- methods(7): Awake, GetFramePerSecondPoints, GetFrameTimePoints, OnDestroy, Start, Update, UpdateFrameTimes
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/Persistent.cs
- 27 lines; kind=attribute; type=Persistent; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=20 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/PersistentLinkable.cs
- 6 lines; kind=attribute; type=PersistentLinkable; bases=Attribute; ancestry=Attribute; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/PersistentRelation.cs
- 6 lines; kind=enum; type=PersistentRelation; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/physicalObject.cs
- 98 lines; kind=MonoBehaviour; type=physicalObject; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=8, Rigidbody=7, Renderer=5, GetComponent=5, Transform=3, Lifecycle=3; unity_score=95
- ksp refs: FlightGlobals=4, TimeWarp=3, Part=1; fan_in=0 fan_out=3
- methods(6): Awake, CleanupMaterials, ConvertToPhysicalObject, OnDestroy, Start, Update
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/PhysicMaterialColor.cs
- 31 lines; kind=class; type=PhysicMaterialColor; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=33
- ksp refs: ConfigNode=1; fan_in=1 fan_out=1
- methods(1): Load
- auto suggestion: tier=A subsystem=Physics

## Assembly-CSharp/PhysicMaterialDefinition.cs
- 124 lines; kind=class; type=PhysicMaterialDefinition; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: PhysicMaterial=12, Material=1; unity_score=61
- ksp refs: ConfigNode=5; fan_in=2 fan_out=2
- methods(2): Load, Save
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/PhysicMaterialLibrary.cs
- 115 lines; kind=MonoBehaviour; type=PhysicMaterialLibrary; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: PhysicMaterial=6, Material=4, Transform=2, Lifecycle=2, Instantiate/Destroy=2, GameObject=2; unity_score=80
- ksp refs: ConfigNode=2, GameDatabase=1; fan_in=2 fan_out=4
- methods(6): Awake, CreateDefaultPhysicMaterial, CreatePhysicMaterials, LoadDefinitions, OnDestroy, Reset
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/PhysicsGlobals.cs **MUST-OPEN (>800 lines)**
- 3875 lines; kind=MonoBehaviour; type=PhysicsGlobals; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: AnimationCurve=54, PhysicMaterial=12, Transform=3, Lifecycle=3, Shader=2, Resources.=1; unity_score=38
- ksp refs: ConfigNode=43, FloatCurve=33, Part=9, GameSettings=6, Planetarium=5, PhysicsGlobals=4; fan_in=55 fan_out=12
- methods(16): Awake, CalculateValues, DragCurveValue, GetBlackBodyRadiation, GetLiftingSurfaceCurve, Load, LoadDatabase, LoadDefaultLiftingSurfaceCurves, LoadLiftingSurfaceCurves, OnDestroy, OnDrawGizmosSelected, OnFlightGlobalsReady, Save, SaveDatabase, Start, UpdateKerbalEVAPhysicsMaterial
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/PhysicsUtil.cs
- 120 lines; kind=class; type=PhysicsUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Raycast/Query=17, Transform=14, Physics.=4, Collider=4; unity_score=78
- ksp refs: none; fan_in=2 fan_out=0
- methods(7): CapsuleCastAllIgnoreSelf, HasAncestorTransform, RaycastAllIgnoreSelf, RaycastHitDistCompare, SphereCastAllIgnoreSelf, SphereSweepTest, SphereSweepTestWhere
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/PIDclamp.cs
- 134 lines; kind=class; type=PIDclamp; bases=IPid; ancestry=IPid; ns=(global)
- usings: -
- unity: none; unity_score=3
- ksp refs: none; fan_in=1 fan_out=2
- methods(8): ClampIntegral, Parse, Reinitialize, Reset, ResetIntegral, ToString, Update, double
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/PieChart.cs
- 327 lines; kind=MonoBehaviour; type=PieChart; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Material=9, Transform=5, Mesh=4, GetComponent=4, GameObject=4, Instantiate/Destroy=3; unity_score=69
- ksp refs: none; fan_in=2 fan_out=1
- methods(15): AddOnExit, AddOnOver, AddOnTap, CreateDirection, CreateEdges, CreateEnds, CreateTopBottom, OnInputDelegate, RemoveOnExit, RemoveOnOver, RemoveOnTap, Reset, SetSlices, Start, UpdateChart
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/PieChartSlice.cs
- 31 lines; kind=MonoBehaviour; type=PieChartSlice; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler,IPointerEnterHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=6, MonoBehaviour=1; unity_score=87
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): OnPointerClick, OnPointerEnter, OnPointerExit
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/Planetarium.cs
- 314 lines; kind=MonoBehaviour; type=Planetarium; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=4, MonoBehaviour=1; unity_score=26
- ksp refs: Vector3d=22, CelestialBody=10, QuaternionD=5, HighLogic=3, Planetarium=2, OrbitDriver=2; fan_in=188 fan_out=6
- methods(18): Awake, FindRootBody, FixedUpdate, FrameIsRotating, GetUniversalTime, LocalToWorld, OnDestroy, OrbitalFrame, PlanetaryFrame, SetFrame, SetUniversalTime, SphericalVector, Update, UpdateCBs, UpdateCBsRecursive, WorldToLocal, ZupAtT, findRotatingBodiesRecursive
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/PlanetariumCamera.cs
- 585 lines; kind=MonoBehaviour; type=PlanetariumCamera; bases=MonoBehaviour,IKSPCamera; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine, UnityEngine.EventSystems, UnityEngine.SceneManagement
- unity: Transform=32, Input.=8, GetComponent=7, GameObject=7, Camera=7, Time.=6; unity_score=73
- ksp refs: GameSettings=15, ScaledSpace=9, GameEvents=8, Planetarium=6, HighLogic=6, FlightGlobals=6; fan_in=23 fan_out=28
- methods(32): Activate, AddTarget, Awake, CreatePivot, Deactivate, FindNearestTarget, GetCameraTransform, GetPivot, GetTarget, GetTargetIndex, LateUpdate, OnDestroy, OnLevelLoaded, OnNavigatorHandoff, OnNavigatorRequestControl, OnNavigatorTakeOver, OnSceneChange, OnSceneLoaded, OnVesselDestroy, RemoveTarget, ResetPivot, SetCamCoordsFromPosition, SetDistance, SetTarget, Start, getNearestBody, getPitch, getReferenceFrame, getYaw, onVesselSwitching, updateAbortCondition, updateDistance
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/PlanetaryResource.cs
- 8 lines; kind=class; type=PlanetaryResource; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/PlanetarySample.cs
- 6 lines; kind=class; type=PlanetarySample; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/PlanetScanData.cs
- 4 lines; kind=class; type=PlanetScanData; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/PopupDialog.cs
- 284 lines; kind=MonoBehaviour; type=PopupDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.UI, TMPro, UnityEngine, UnityEngine.Events, UnityEngine.UI
- unity: GameObject=29, UnityEngine.UI=17, GetComponent=16, Transform=13, Instantiate/Destroy=6, SetActive=5; unity_score=91
- ksp refs: HighLogic=1; fan_in=71 fan_out=12
- methods(15): CheckForOpenDialogs, ClearPopUps, Contains, Dismiss, DismissPopup, FixedUpdate, LateUpdate, OnDestroy, OnRectTransformDimensionsChange, OnRenderObject, SetDraggable, SetPopupData, SpawnPopupDialog, Start, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/PopupDialogController.cs
- 27 lines; kind=MonoBehaviour; type=PopupDialogController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, Lifecycle=2, MonoBehaviour=1; unity_score=72
- ksp refs: none; fan_in=6 fan_out=1
- methods(2): Awake, OnDestroy
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/PositionTarget.cs
- 101 lines; kind=class; type=PositionTarget; bases=ITargetable; ancestry=ITargetable; ns=(global)
- usings: UnityEngine
- unity: Transform=3, GameObject=2, Instantiate/Destroy=1; unity_score=34
- ksp refs: Vector3d=2, CelestialBody=2, Vessel=1, OrbitDriver=1, Orbit=1; fan_in=1 fan_out=7
- methods(12): GetActiveTargetable, GetDisplayName, GetFwdVector, GetName, GetObtVelocity, GetOrbit, GetOrbitDriver, GetSrfVelocity, GetTargetingMode, GetTransform, GetVessel, Update
- auto suggestion: tier=B subsystem=Orbit

## Assembly-CSharp/PQ.cs **MUST-OPEN (>800 lines)**
- 1077 lines; kind=MonoBehaviour; type=PQ; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=8, GameObject=6, Mesh=2, LayerMask=2, Instantiate/Destroy=2, Time.=1; unity_score=35
- ksp refs: PQS=32, Vector3d=12, QuaternionD=2; fan_in=0 fan_out=3
- methods(30): Awake, Build, ClearAndDestroy, ClearCache, Collapse, Destroy, FastUpdateSubQuadsPosition, GetEdge, GetEdgeQuads, GetEdgeState, GetRightmostCornerPQ, GetSidePQ, PreciseUpdateSubQuadsPosition, QuadDelegate, QueueForCornerNormalUpdate, QueueForNormalUpdate, SetCache, SetEdgeState, SetInvisible, SetMasterInvisible, SetMasterVisible, SetNeighbour, SetVisible, SetupQuad, Subdivide, UnqueueForNormalUpdate, UpdateSubdivision, UpdateSubdivisionInit, UpdateTargetRelativity, UpdateVisibility
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQS.cs **MUST-OPEN (>800 lines)**
- 3168 lines; kind=MonoBehaviour; type=PQS; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.Rendering
- unity: GameObject=24, Transform=20, Material=16, Texture=14, Coroutine=9, GetComponent=6; unity_score=42
- ksp refs: Vector3d=61, PQS=12, QuaternionD=7, GameEvents=3, GameSettings=2, HighLogic=1; fan_in=79 fan_out=17
- methods(125): ActivateSphere, AddPQToLocalSpaceStorage, ApplyTerrainShaderSettings, AssignMeshTangents, AssignQuad, BackupEdgeNormals, BilinearInterpColorMap, BilinearInterpFloatMap, BuildMeshColorChannel, BuildMeshCustomNormals, BuildMeshSphereNormals, BuildMeshTangents, BuildMeshUV2Channel, BuildMeshUV3Channel, BuildMeshUV4Channel, BuildMeshUVChannel, BuildNormals, BuildNormalsNoClear, BuildQuad, BuildTangents, BuildVertexColor, BuildVertexHeight, BuildVertexMapCoords, BuildVertexQuadUV, BuildVertexSphereNormal, BuildVertexSphereUV, BuildVertexSurfaceRelative, BuildVertexUV2, BuildVertexUV3, BuildVertexUV4, ClearCache, CloseExternalRender, CombineEdgeNormals, CreateCache, CreateCacheColors, CreateCacheNormals, CreateCacheUVExtras, CreateIndexCache, CreateIndexState, CreateMaps, CreateQuads, DeactivateSphere, DestroyQuad, DisableSphere, DisableSubdivision ...
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQS_Collider.cs
- 103 lines; kind=MonoBehaviour; type=PQS_Collider; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=9, GameObject=5, SetActive=3, Lifecycle=2, MonoBehaviour=1, GetComponent=1; unity_score=79
- ksp refs: PQS=2; fan_in=0 fan_out=1
- methods(7): ActivateCollider, DeactivateCollider, FixedUpdate, Reset, SetTargetSphere, Start, UpdateCollider
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQS_ColliderAdvanced.cs
- 452 lines; kind=MonoBehaviour; type=PQS_ColliderAdvanced; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=32, Collider=19, Physics.=18, GameObject=18, GetComponent=17, Instantiate/Destroy=4; unity_score=86
- ksp refs: Vector3d=21, QuaternionD=5, PQS=2, CelestialBody=1; fan_in=0 fan_out=4
- methods(20): ActivateGravityCollider, ActivateVelocityCollider, DeactivateGravityCollider, DeactivateVelocityCollider, FixedUpdate, GetNormalAtPoint, IterateVelocity, OnDestroy, OnReferenceBodyChange, RegisterColliders, Reset, SetTargetSphere, ShowDebugNormals, Start, UnregisterColliders, UpdateColliders, UpdateGravityCollider, UpdateNormalCollider, UpdateVelocityCollider, UpdateVerticalCollider
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQS_ColliderRigidbodyTest.cs
- 59 lines; kind=MonoBehaviour; type=PQS_ColliderRigidbodyTest; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=8, Lifecycle=3, Input.=3, Time.=2, Rigidbody=2, MonoBehaviour=1; unity_score=92
- ksp refs: PhysicsGlobals=1, PQS=1; fan_in=0 fan_out=2
- methods(4): FixedUpdate, Reset, Start, Update
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQS_GameBindings.cs
- 63 lines; kind=class; type=PQS_GameBindings; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=4
- methods(5): DispatchOnPQSCityLoaded, DispatchOnPQSCityUnloaded, GetPOIRange, GetPQSCityLoadRange, TerrainPresetListIsCompatible
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQS_KSPBinder.cs
- 105 lines; kind=MonoBehaviour; type=PQS_KSPBinder; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=3, Lifecycle=2, MonoBehaviour=1; unity_score=48
- ksp refs: CelestialBody=6, PhysicsGlobals=4, GameSettings=4, PQS=2, GameEvents=2, HighLogic=1; fan_in=0 fan_out=12
- methods(13): Awake, GetCBForPQS, GetPQSCityLoadRange, OnDestroy, OnGetPOIRange, OnPQSCityLoaded, OnPQSCityUnloaded, pqsBindings_GetLoadedSceneIsGame, pqsBindings_GetPlanetForceShaderModel20, pqsBindings_GetPlanetScatterFactor, pqsBindings_GetPresetListCompatible, pqsBindings_GetSettingsReady, pqsBindings_GetUsePlanetScatter
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQS_PartCollider.cs
- 473 lines; kind=MonoBehaviour; type=PQS_PartCollider; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Collider=19, Physics.=18, Transform=17, GameObject=15, GetComponent=6, Lifecycle=4; unity_score=80
- ksp refs: Vector3d=20, QuaternionD=5, PQS=2, Part=1, CelestialBody=1; fan_in=1 fan_out=5
- methods(20): ActivateGravityCollider, ActivateVelocityCollider, Awake, DeactivateGravityCollider, DeactivateVelocityCollider, FixedUpdate, GetNormalAtPoint, IterateVelocity, OnDestroy, OnReferenceBodyChange, RegisterColliders, Reset, SetTargetSphere, Start, UnregisterColliders, UpdateColliders, UpdateGravityCollider, UpdateNormalCollider, UpdateVelocityCollider, UpdateVerticalCollider
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSCache.cs
- 495 lines; kind=MonoBehaviour; type=PQSCache; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.Rendering
- unity: GameObject=14, Transform=9, GetComponent=4, SetActive=2, MonoBehaviour=2, Mesh=2; unity_score=58
- ksp refs: PQS=16, ConfigNode=11; fan_in=6 fan_out=3
- methods(17): Awake, CoroutineCreatePQCache, CreateCache, CreateDefaultPresetList, DestroyQuad, GetPreset, GetQuad, IncreasePQCache, Load, LoadPresetList, OnDestroy, OnEvent, Reset, ResetCache, Save, SavePresetList, SetPreset
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSCity.cs
- 413 lines; kind=PQSMod; type=PQSCity; bases=PQSSurfaceObject; ancestry=PQSSurfaceObject/PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=37, Transform=28, SetActive=9, Renderer=4, GetComponent=3, Coroutine=2; unity_score=79
- ksp refs: GameEvents=6, Vector3d=5, CelestialBody=4, Planetarium=3, PQS=2, HighLogic=2; fan_in=7 fan_out=12
- methods(20): CheckLocals, GetPQSCitySeed, GetStableHashCode, OnPostSetup, OnSetup, OnSphereActive, OnSphereInactive, OnSphereReset, OnSphereStart, OnUpdateFinished, Orientate, OrientateToOrigin, OrientateToWorld, PostSetup, Randomize, Reset, ResetCelestialBody, SetActive, Setup, Start
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSCity2.cs
- 513 lines; kind=PQSMod; type=PQSCity2; bases=PQSSurfaceObject; ancestry=PQSSurfaceObject/PQSMod/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Scenery.Scripts, KSP.Localization, UnityEngine
- unity: Transform=27, GameObject=7, SetActive=5, GetComponent=4, Raycast/Query=3, Lifecycle=3; unity_score=64
- ksp refs: Vector3d=9, FloatingOrigin=8, QuaternionD=6, HighLogic=5, GameEvents=5, CelestialBody=4; fan_in=6 fan_out=16
- methods(21): Awake, OnDestroy, OnPostSetup, OnScenerySettingChanged, OnSetup, OnSphereActive, OnSphereInactive, OnSphereReset, OnSphereStart, OnUpdateFinished, Orientate, OrientateOnWater, OrientateRayCast, Reset, SetActive, SetBody, SetInactive, SetRange, Setup, Start, setVisibility
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSGlobeDisplay.cs
- 149 lines; kind=MonoBehaviour; type=PQSGlobeDisplay; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Mesh=6, GetComponent=5, Renderer=3, Material=3, Instantiate/Destroy=2, GameObject=2; unity_score=80
- ksp refs: PQS=3; fan_in=0 fan_out=1
- methods(8): CreateGlobe, CreateLatitudeLine, CreateLatitudeLines, CreateLongitudeCircle, CreateLongitudeLines, OnDestroy, Reset, Start
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSLandControl.cs **MUST-OPEN (>800 lines)**
- 930 lines; kind=PQSMod; type=PQSLandControl; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.Rendering
- unity: GameObject=19, Transform=10, Instantiate/Destroy=4, Mesh=3, GetComponent=3, SetActive=2; unity_score=50
- ksp refs: PQS=15, GameSettings=2, GameEvents=2; fan_in=3 fan_out=7
- methods(26): AddScatterMeshController, ApplyCastShadowsSetting, BuildCache, BuildCacheMesh, BuildCacheQuad, Clamp, ClearAll, CreateScatterMesh, CubicHermite, DestroyQuad, DoubleLerp, Lerp, OnDestroy, OnQuadBuilt, OnQuadPreBuild, OnSetup, OnSphereReset, OnSphereStarted, OnVertexBuild, OnVertexBuildHeight, RandomRange, Reset, Setup, SphereActive, SphereInactive, Start
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMeshPlanet.cs
- 587 lines; kind=MonoBehaviour; type=PQSMeshPlanet; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Time.=4, Mesh=4, GetComponent=2, GameObject=2, Coroutine=2, Renderer=1; unity_score=40
- ksp refs: Vector3d=6, PQS=1; fan_in=0 fan_out=2
- methods(22): AddTriangle, Awake, CreateBox, CreateIcosahedron, CreateMesh, CreateMeshObject, CreatePlanetMesh, GetDoubleVert, GetEnumerator, GetVertex, HasVertex, RemoveVertex, ReplaceEdge, Reset, SetDoubleVert, SetEdges, SetVerts, SetupLists, Split, SplitWrap, Subdivide, Wrap
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMeshWrapper.cs
- 155 lines; kind=MonoBehaviour; type=PQSMeshWrapper; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Mesh=5, MonoBehaviour=1; unity_score=54
- ksp refs: Vector3d=6, PQS=1; fan_in=0 fan_out=3
- methods(3): CreateTangents, CreateWrappedMesh, CreateWrappedMesh2
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod.cs
- 120 lines; kind=MonoBehaviour; type=PQSMod; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=24
- ksp refs: PQS=5; fan_in=70 fan_out=1
- methods(22): GetVertexMaxHeight, GetVertexMinHeight, OnMeshBuild, OnPostSetup, OnPreUpdate, OnQuadBuilt, OnQuadCreate, OnQuadDestroy, OnQuadPreBuild, OnQuadUpdate, OnQuadUpdateNormals, OnSetup, OnSphereActive, OnSphereInactive, OnSphereReset, OnSphereStart, OnSphereStarted, OnSphereTransformUpdate, OnUpdateFinished, OnVertexBuild, OnVertexBuildHeight, RebuildSphere
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_AerialPerspectiveMaterial.cs
- 55 lines; kind=PQSMod; type=PQSMod_AerialPerspectiveMaterial; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=8; unity_score=79
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): OnSetup, OnSphereActive, OnUpdateFinished
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_AltitudeAlpha.cs
- 34 lines; kind=PQSMod; type=PQSMod_AltitudeAlpha; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=2; fan_in=0 fan_out=2
- methods(3): OnSetup, OnVertexBuild, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_AltitudeUV.cs
- 46 lines; kind=PQSMod; type=PQSMod_AltitudeUV; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=2; fan_in=1 fan_out=3
- methods(3): OnSetup, OnVertexBuild, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_BillboardObject.cs
- 16 lines; kind=PQSMod; type=PQSMod_BillboardObject; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, Lifecycle=1; unity_score=61
- ksp refs: Vector3d=1; fan_in=0 fan_out=2
- methods(1): FixedUpdate
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_CelestialBodyTransform.cs
- 355 lines; kind=PQSMod; type=PQSMod_CelestialBodyTransform; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=8, Renderer=5, Transform=4, GameObject=4, Material=2, GetComponent=2; unity_score=59
- ksp refs: FlightGlobals=9, GameSettings=6, PQS=5, HighLogic=3, MapView=2, CelestialBody=2; fan_in=2 fan_out=9
- methods(11): DoFade, FadeOutSphere, OnPreUpdate, OnSetup, OnSphereInactive, OnSphereReset, OnSphereStart, OnSphereTransformUpdate, OnUpdateFinished, Reset, Setup
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_CreateSphereCollider.cs
- 25 lines; kind=PQSMod; type=PQSMod_CreateSphereCollider; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Collider=3, GetComponent=2, GameObject=1; unity_score=82
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): OnSetup, Reset
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_FlattenArea.cs
- 158 lines; kind=PQSMod; type=PQSMod_FlattenArea; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GetComponent=2, GameObject=2, Transform=1; unity_score=38
- ksp refs: Vector3d=4, Planetarium=3, PQS=3, CelestialBody=1; fan_in=1 fan_out=8
- methods(7): CubicHermite, Lerp, OnQuadBuilt, OnQuadPreBuild, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_FlattenAreaTangential.cs
- 120 lines; kind=PQSMod; type=PQSMod_FlattenAreaTangential; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=21
- ksp refs: Vector3d=3, PQS=3; fan_in=0 fan_out=3
- methods(6): CubicHermite, OnQuadBuilt, OnQuadPreBuild, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_FlattenOcean.cs
- 38 lines; kind=PQSMod; type=PQSMod_FlattenOcean; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=2; fan_in=0 fan_out=2
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_GnomonicTest.cs
- 198 lines; kind=PQSMod; type=PQSMod_GnomonicTest; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=1, KeyCode=1, Input.=1; unity_score=32
- ksp refs: PQS=22, Vector3d=2; fan_in=0 fan_out=3
- methods(5): GetGnomonicMapCoords, OnSetup, OnVertexBuild, OnVertexBuild2, Update
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_HeightColorMap.cs
- 127 lines; kind=PQSMod; type=PQSMod_HeightColorMap; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=29
- ksp refs: PQS=2; fan_in=0 fan_out=2
- methods(7): Clamp, CubicHermite, Lerp, OnSetup, OnVertexBuild, Reset, SelectLandClassByHeight
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_HeightColorMap2.cs
- 139 lines; kind=PQSMod; type=PQSMod_HeightColorMap2; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=28
- ksp refs: PQS=2; fan_in=0 fan_out=2
- methods(7): Clamp, CubicHermite, Lerp, OnSetup, OnVertexBuild, Reset, SelectLandClassByHeight
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_HeightColorMapNoise.cs
- 127 lines; kind=PQSMod; type=PQSMod_HeightColorMapNoise; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=29
- ksp refs: PQS=2; fan_in=0 fan_out=2
- methods(7): Clamp, CubicHermite, Lerp, OnSetup, OnVertexBuild, Reset, SelectLandClassByHeight
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_LandClassController.cs
- 201 lines; kind=PQSMod; type=PQSMod_LandClassController; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Texture=1; unity_score=26
- ksp refs: none; fan_in=0 fan_out=2
- methods(3): Lerp, Reset, Setup
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_LandClassScatterQuad.cs
- 148 lines; kind=MonoBehaviour; type=PQSMod_LandClassScatterQuad; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=4, SetActive=4, GameObject=3, Mesh=2, Instantiate/Destroy=2, Renderer=1; unity_score=67
- ksp refs: none; fan_in=1 fan_out=1
- methods(7): Destroy, OnDestroy, OnQuadDestroy, OnQuadInvisible, OnQuadUpdate, OnQuadVisible, Setup
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_MapDecal.cs
- 283 lines; kind=PQSMod; type=PQSMod_MapDecal; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=20
- ksp refs: Vector3d=4, PQS=4; fan_in=0 fan_out=4
- methods(9): GetColorSmoothing, GetHeightSmoothing, Lerp, OnQuadBuilt, OnQuadPreBuild, OnSetup, OnVertexBuild, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_MapDecalTangent.cs
- 281 lines; kind=PQSMod; type=PQSMod_MapDecalTangent; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=20
- ksp refs: Vector3d=5, PQS=4; fan_in=0 fan_out=4
- methods(9): GetColorSmoothing, GetHeightSmoothing, Lerp, OnQuadBuilt, OnQuadPreBuild, OnSetup, OnVertexBuild, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_MaterialFadeAltitude.cs
- 64 lines; kind=PQSMod; type=PQSMod_MaterialFadeAltitude; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Renderer=2, Material=2, GetComponent=2, Shader=1, Lifecycle=1; unity_score=72
- ksp refs: none; fan_in=0 fan_out=1
- methods(3): OnSetup, Reset, Update
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_MaterialFadeAltitudeDouble.cs
- 89 lines; kind=PQSMod; type=PQSMod_MaterialFadeAltitudeDouble; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=2, Renderer=2, Material=2, Lifecycle=2, GetComponent=2; unity_score=69
- ksp refs: none; fan_in=0 fan_out=1
- methods(4): OnDestroy, OnSetup, Reset, Update
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_MaterialQuadRelative.cs
- 32 lines; kind=PQSMod; type=PQSMod_MaterialQuadRelative; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=9, Renderer=3, Material=3, GetComponent=3; unity_score=100
- ksp refs: PQS=1; fan_in=0 fan_out=2
- methods(2): OnQuadBuilt, OnSetup
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_MaterialSetDirection.cs
- 26 lines; kind=PQSMod; type=PQSMod_MaterialSetDirection; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=4, Shader=2; unity_score=80
- ksp refs: none; fan_in=0 fan_out=2
- methods(2): OnUpdateFinished, Reset
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_MeshScatter.cs
- 343 lines; kind=PQSMod; type=PQSMod_MeshScatter; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.Rendering
- unity: Mesh=12, Transform=10, GameObject=10, SetActive=3, GetComponent=3, Renderer=2; unity_score=74
- ksp refs: PQS=10; fan_in=1 fan_out=3
- methods(10): AddScatterMeshController, AssignScatterMesh, BuildCache, BuildCacheMesh, BuildCacheQuad, OnQuadBuilt, OnSetup, RandomRange, Reset, UnassignScatterMesh
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_MeshScatter_QuadControl.cs
- 112 lines; kind=MonoBehaviour; type=PQSMod_MeshScatter_QuadControl; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Renderer=2, GetComponent=2, MonoBehaviour=1, Mesh=1, Instantiate/Destroy=1; unity_score=53
- ksp refs: none; fan_in=1 fan_out=1
- methods(5): OnQuadDestroy, OnQuadInvisible, OnQuadUpdate, OnQuadVisible, Setup
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_OceanFX.cs
- 112 lines; kind=PQSMod; type=PQSMod_OceanFX; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=15, Texture=5, Renderer=2, Material=2, GetComponent=2, Time.=1; unity_score=89
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): OnSetup, OnUpdateFinished
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_QuadEnhanceCoast.cs
- 40 lines; kind=PQSMod; type=PQSMod_QuadEnhanceCoast; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=1
- methods(3): OnQuadBuilt, OnSetup, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_QuadMeshColliders.cs
- 45 lines; kind=PQSMod; type=PQSMod_QuadMeshColliders; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Material=2, PhysicMaterial=1, GetComponent=1, GameObject=1, Collider=1; unity_score=74
- ksp refs: none; fan_in=0 fan_out=1
- methods(4): OnQuadBuilt, OnQuadDestroy, OnSetup, Reset
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_RemoveQuadMap.cs
- 50 lines; kind=PQSMod; type=PQSMod_RemoveQuadMap; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=2; fan_in=0 fan_out=3
- methods(5): OnQuadBuilt, OnQuadPreBuild, OnSetup, OnVertexBuild, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_ROCScatterQuad.cs
- 168 lines; kind=MonoBehaviour; type=PQSMod_ROCScatterQuad; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=8, GameObject=4, SetActive=3, MonoBehaviour=1, Instantiate/Destroy=1, GetComponent=1; unity_score=64
- ksp refs: GameSettings=1; fan_in=2 fan_out=3
- methods(6): Destroy, OnQuadDestroy, OnQuadInvisible, OnQuadUpdate, OnQuadVisible, Setup
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_SmoothLatitudeRange.cs
- 48 lines; kind=PQSMod; type=PQSMod_SmoothLatitudeRange; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=2; fan_in=0 fan_out=3
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_SurfaceObjectQuads.cs
- 39 lines; kind=PQSMod; type=PQSMod_SurfaceObjectQuads; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GetComponent=3, GameObject=2; unity_score=66
- ksp refs: CelestialBody=2; fan_in=0 fan_out=4
- methods(4): OnQuadBuilt, OnQuadDestroy, OnSetup, Reset
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_TangentTextureRanges.cs
- 67 lines; kind=PQSMod; type=PQSMod_TangentTextureRanges; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=5; fan_in=0 fan_out=2
- methods(4): OnMeshBuild, OnSetup, OnVertexBuild, SmoothStep
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_TextureAtlas.cs
- 215 lines; kind=PQSMod; type=PQSMod_TextureAtlas; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: Expansions.Missions.Editor, UnityEngine
- unity: Material=11, Lifecycle=1; unity_score=55
- ksp refs: Vector3d=6, HighLogic=5, PQS=4, CelestialBody=3, Planetarium=1, GameSettings=1; fan_in=0 fan_out=12
- methods(4): CanTextureAtlasModBeUsed, OnQuadBuilt, OnSetup, Start
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSMod_UV_Quad.cs
- 10 lines; kind=PQSMod; type=PQSMod_UV_Quad; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=1; fan_in=0 fan_out=2
- methods(1): OnSetup
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_UV_Sphere.cs
- 10 lines; kind=PQSMod; type=PQSMod_UV_Sphere; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=1; fan_in=0 fan_out=2
- methods(1): OnSetup
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_UVPlanetRelativePosition.cs
- 34 lines; kind=PQSMod; type=PQSMod_UVPlanetRelativePosition; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=12, Vector3d=3; fan_in=0 fan_out=3
- methods(2): OnQuadBuilt, OnQuadUpdateNormals
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexColorMap.cs
- 17 lines; kind=PQSMod; type=PQSMod_VertexColorMap; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=2; fan_in=0 fan_out=3
- methods(2): OnSetup, OnVertexBuild
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexColorMapBlend.cs
- 24 lines; kind=PQSMod; type=PQSMod_VertexColorMapBlend; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: PQS=2; fan_in=0 fan_out=3
- methods(3): OnSetup, OnVertexBuild, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexColorNoise.cs
- 74 lines; kind=PQSMod; type=PQSMod_VertexColorNoise; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: LibNoise, UnityEngine
- unity: none; unity_score=20
- ksp refs: PQS=2; fan_in=0 fan_out=7
- methods(3): OnSetup, OnVertexBuild, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexColorNoiseRGB.cs
- 83 lines; kind=PQSMod; type=PQSMod_VertexColorNoiseRGB; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: LibNoise, UnityEngine
- unity: none; unity_score=20
- ksp refs: PQS=2; fan_in=0 fan_out=7
- methods(3): OnSetup, OnVertexBuild, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexColorSolid.cs
- 24 lines; kind=PQSMod; type=PQSMod_VertexColorSolid; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=21
- ksp refs: PQS=2; fan_in=0 fan_out=2
- methods(3): OnSetup, OnVertexBuild, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexColorSolidBlend.cs
- 17 lines; kind=PQSMod; type=PQSMod_VertexColorSolidBlend; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: PQS=2; fan_in=0 fan_out=2
- methods(2): OnSetup, OnVertexBuild
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexDefineCoastLine.cs
- 41 lines; kind=PQSMod; type=PQSMod_VertexDefineCoastLine; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=2; fan_in=0 fan_out=2
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexHeightMap.cs
- 54 lines; kind=PQSMod; type=PQSMod_VertexHeightMap; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=3; fan_in=0 fan_out=3
- methods(6): GetVertexMaxHeight, GetVertexMinHeight, Lerp, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexHeightMapStep.cs
- 67 lines; kind=PQSMod; type=PQSMod_VertexHeightMapStep; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Texture=1; unity_score=30
- ksp refs: PQS=3; fan_in=0 fan_out=2
- methods(6): GetVertexMaxHeight, GetVertexMinHeight, Lerp, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexHeightNoise.cs
- 70 lines; kind=PQSMod; type=PQSMod_VertexHeightNoise; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: LibNoise, UnityEngine
- unity: none; unity_score=16
- ksp refs: PQS=2; fan_in=0 fan_out=7
- methods(4): GetVertexMaxHeight, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexHeightNoiseHeightMap.cs
- 104 lines; kind=PQSMod; type=PQSMod_VertexHeightNoiseHeightMap; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: LibNoise, UnityEngine
- unity: Texture=1; unity_score=26
- ksp refs: PQS=2; fan_in=0 fan_out=7
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexHeightNoiseVertHeight.cs
- 102 lines; kind=PQSMod; type=PQSMod_VertexHeightNoiseVertHeight; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: LibNoise, UnityEngine
- unity: none; unity_score=16
- ksp refs: PQS=2; fan_in=0 fan_out=7
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexHeightNoiseVertHeightCurve.cs
- 111 lines; kind=PQSMod; type=PQSMod_VertexHeightNoiseVertHeightCurve; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: LibNoise, UnityEngine
- unity: AnimationCurve=2; unity_score=22
- ksp refs: PQS=1; fan_in=0 fan_out=7
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexHeightNoiseVertHeightCurve2.cs
- 127 lines; kind=PQSMod; type=PQSMod_VertexHeightNoiseVertHeightCurve2; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: LibNoise, UnityEngine
- unity: AnimationCurve=4; unity_score=28
- ksp refs: PQS=2; fan_in=0 fan_out=5
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexHeightNoiseVertHeightCurve3.cs
- 180 lines; kind=PQSMod; type=PQSMod_VertexHeightNoiseVertHeightCurve3; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: LibNoise, UnityEngine
- unity: AnimationCurve=4; unity_score=28
- ksp refs: Vector3d=3, PQS=2; fan_in=0 fan_out=7
- methods(9): GetVertexMaxHeight, GetVertexMinHeight, Lerp, Noise, NoiseNormalized, OnSetup, OnVertexBuildHeight, Reset, Setup
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexHeightOblate.cs
- 30 lines; kind=PQSMod; type=PQSMod_VertexHeightOblate; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=2; fan_in=0 fan_out=2
- methods(3): OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexHeightOffset.cs
- 17 lines; kind=PQSMod; type=PQSMod_VertexHeightOffset; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=1; fan_in=0 fan_out=2
- methods(2): OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexNoise.cs
- 75 lines; kind=PQSMod; type=PQSMod_VertexNoise; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: LibNoise, LibNoise.Modifiers, UnityEngine
- unity: none; unity_score=16
- ksp refs: PQS=2; fan_in=0 fan_out=7
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexPlanet.cs
- 361 lines; kind=PQSMod; type=PQSMod_VertexPlanet; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: LibNoise, UnityEngine
- unity: none; unity_score=23
- ksp refs: PQS=4; fan_in=0 fan_out=7
- methods(11): Clamp, CubicHermite, GetVertexMaxHeight, GetVertexMinHeight, Lerp, OnSetup, OnVertexBuild, OnVertexBuildHeight, Reset, SelectLandClassByHeight, Setup
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexRidgedAltitudeCurve.cs
- 115 lines; kind=PQSMod; type=PQSMod_VertexRidgedAltitudeCurve; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: LibNoise, UnityEngine
- unity: AnimationCurve=4; unity_score=29
- ksp refs: PQS=2; fan_in=0 fan_out=6
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexSimplexColorRGB.cs
- 54 lines; kind=PQSMod; type=PQSMod_VertexSimplexColorRGB; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=20
- ksp refs: PQS=2; fan_in=0 fan_out=3
- methods(3): OnSetup, OnVertexBuild, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexSimplexHeight.cs
- 48 lines; kind=PQSMod; type=PQSMod_VertexSimplexHeight; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=2; fan_in=0 fan_out=3
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexSimplexHeightAbsolute.cs
- 48 lines; kind=PQSMod; type=PQSMod_VertexSimplexHeightAbsolute; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=2; fan_in=0 fan_out=3
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexSimplexHeightFlatten.cs
- 60 lines; kind=PQSMod; type=PQSMod_VertexSimplexHeightFlatten; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=2; fan_in=0 fan_out=3
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexSimplexHeightMap.cs
- 64 lines; kind=PQSMod; type=PQSMod_VertexSimplexHeightMap; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=2; fan_in=0 fan_out=4
- methods(5): GetVertexMaxHeight, GetVertexMinHeight, OnSetup, OnVertexBuildHeight, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexSimplexMultiChromatic.cs
- 91 lines; kind=PQSMod; type=PQSMod_VertexSimplexMultiChromatic; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: PQS=2; fan_in=0 fan_out=3
- methods(3): OnSetup, OnVertexBuild, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexSimplexNoiseColor.cs
- 45 lines; kind=PQSMod; type=PQSMod_VertexSimplexNoiseColor; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=27
- ksp refs: PQS=2; fan_in=0 fan_out=3
- methods(3): OnSetup, OnVertexBuild, Reset
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VertexVoronoi.cs
- 134 lines; kind=PQSMod; type=PQSMod_VertexVoronoi; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: LibNoise, UnityEngine
- unity: none; unity_score=15
- ksp refs: PQS=1; fan_in=0 fan_out=4
- methods(5): GetValue, IntValueNoise3D, OnSetup, OnVertexBuildHeight, ValueNoise3D
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VoronoiCraters.cs
- 83 lines; kind=PQSMod; type=PQSMod_VoronoiCraters; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: LibNoise, UnityEngine
- unity: AnimationCurve=2; unity_score=27
- ksp refs: PQS=2; fan_in=0 fan_out=4
- methods(3): OnSetup, OnVertexBuild, OnVertexBuildHeight
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSMod_VoronoiCraters2.cs
- 105 lines; kind=PQSMod; type=PQSMod_VoronoiCraters2; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: LibNoise, UnityEngine
- unity: AnimationCurve=1; unity_score=21
- ksp refs: PQS=2, Vector3d=1; fan_in=0 fan_out=6
- methods(3): OnSetup, OnVertexBuild, OnVertexBuildHeight
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PQSROCControl.cs
- 160 lines; kind=PQSMod; type=PQSROCControl; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: Expansions, UnityEngine
- unity: GameObject=6, Mesh=2, GetComponent=2, Transform=1, Lifecycle=1; unity_score=57
- ksp refs: PQS=3, CelestialBody=2; fan_in=2 fan_out=10
- methods(8): Awake, OnQuadBuilt, OnQuadDestroy, OnQuadPreBuild, OnSetup, OnSphereReset, OnSphereStarted, OnVertexBuild
- auto suggestion: tier=B subsystem=Terrain

## Assembly-CSharp/PQSSurfaceObject.cs
- 8 lines; kind=PQSMod; type=PQSSurfaceObject; bases=PQSMod; ancestry=PQSMod/MonoBehaviour; ns=(global)
- usings: -
- unity: Transform=1; unity_score=36
- ksp refs: Vector3d=1; fan_in=6 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Terrain

## Assembly-CSharp/PrefabAnimationFX.cs
- 106 lines; kind=MonoBehaviour; type=PrefabAnimationFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=7, Animation=3, GameObject=2, Resources.=1, GetComponent=1; unity_score=73
- ksp refs: ConfigNode=4; fan_in=0 fan_out=4
- methods(4): OnEvent, OnInitialize, OnLoad, OnSave
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/PrefabMultiParticleFX.cs
- 241 lines; kind=MonoBehaviour; type=PrefabMultiParticleFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=17, GameObject=10, Transform=7, Instantiate/Destroy=3, GetComponent=2, SetActive=1; unity_score=80
- ksp refs: ConfigNode=4; fan_in=0 fan_out=5
- methods(6): OnDestroy, OnEvent, OnInitialize, OnLoad, OnSave, SetEmitter
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/PrefabParticleFX.cs
- 174 lines; kind=MonoBehaviour; type=PrefabParticleFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=16, GameObject=7, Transform=6, Instantiate/Destroy=2, SetActive=1, Resources.=1; unity_score=83
- ksp refs: ConfigNode=4; fan_in=0 fan_out=5
- methods(5): OnDestroy, OnEvent, OnInitialize, OnLoad, OnSave
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/PrefabSpawner.cs
- 17 lines; kind=MonoBehaviour; type=PrefabSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=6, GameObject=3, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1; unity_score=92
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): Awake
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/PrefabSpawnFX.cs
- 69 lines; kind=MonoBehaviour; type=PrefabSpawnFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=10, Transform=5, Instantiate/Destroy=2, Resources.=1; unity_score=84
- ksp refs: ConfigNode=4; fan_in=0 fan_out=4
- methods(4): OnEvent, OnInitialize, OnLoad, OnSave
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/PreferredInitialStage.cs
- 7 lines; kind=enum; type=PreferredInitialStage; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Vessel

## Assembly-CSharp/PreFlightCheck.cs
- 116 lines; kind=class; type=PreFlightCheck; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization, PreFlightTests, UnityEngine
- unity: none; unity_score=16
- ksp refs: GameSettings=6, HighLogic=4, Localizer=1; fan_in=8 fan_out=10
- methods(5): Abort, AddTest, Complete, RunTests, runNextTest
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreviewPanel.cs
- 17 lines; kind=MonoBehaviour; type=PreviewPanel; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=4, MonoBehaviour=1; unity_score=80
- ksp refs: none; fan_in=1 fan_out=0
- methods(2): OnPointerEnter, OnPointerExit
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/PreviewType.cs
- 12 lines; kind=class; type=PreviewType; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: GameObject=1; unity_score=29
- ksp refs: ProtoCrewMember=2; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/PrivacyDialog.cs
- 64 lines; kind=MonoBehaviour; type=PrivacyDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.Analytics, UnityEngine.UI
- unity: GameObject=3, UnityEngine.UI=2, Transform=2, Lifecycle=2, Instantiate/Destroy=2, SetActive=1; unity_score=87
- ksp refs: InputLockManager=2; fan_in=1 fan_out=4
- methods(8): Create, Dismiss, OnCloseButton, OnPrivacyURLFailure, OnPrivacyURLReceived, OnUnityData, Start, Update
- auto suggestion: tier=C subsystem=Analytics

## Assembly-CSharp/ProceduralAsteroid.cs
- 133 lines; kind=MonoBehaviour; type=ProceduralAsteroid; bases=ProceduralSpaceObject; ancestry=ProceduralSpaceObject/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=7, Mesh=3, GetComponent=3, GameObject=3, Time.=2, Application.=2; unity_score=75
- ksp refs: Part=2; fan_in=3 fan_out=5
- methods(4): CreatePAsteroid, Generate, Rebuild, Start
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/ProceduralAsteroidTest.cs
- 88 lines; kind=MonoBehaviour; type=ProceduralAsteroidTest; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=3, Rigidbody=3, GetComponent=3, Instantiate/Destroy=2, GameObject=2, Application.=2; unity_score=83
- ksp refs: none; fan_in=0 fan_out=3
- methods(6): AddRigidbody, NewSeed, Randomize, RangefinderGeneric, Rebuild, Start
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/ProceduralComet.cs
- 154 lines; kind=MonoBehaviour; type=ProceduralComet; bases=ProceduralSpaceObject; ancestry=ProceduralSpaceObject/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=7, Mesh=3, GetComponent=3, GameObject=3, Time.=2, Application.=2; unity_score=73
- ksp refs: Part=2; fan_in=3 fan_out=6
- methods(4): CreatePComet, Generate, Rebuild, Start
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/ProceduralCometTest.cs
- 99 lines; kind=MonoBehaviour; type=ProceduralCometTest; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=7, GetComponent=4, GameObject=4, Rigidbody=3, Instantiate/Destroy=3, Resources.=2; unity_score=88
- ksp refs: none; fan_in=0 fan_out=3
- methods(6): AddRigidbody, NewSeed, Randomize, RangefinderGeneric, Rebuild, Start
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/ProceduralCubemap.cs
- 120 lines; kind=MonoBehaviour; type=ProceduralCubemap; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Texture=25, Camera=13, Transform=6, GameObject=6, Instantiate/Destroy=4, GetComponent=2; unity_score=99
- ksp refs: none; fan_in=0 fan_out=0
- methods(7): CreateCamera, CreateCameraOrtho, CreateCubeMap, CreateIndividualFaces, RenderCamera, RenderCubemap, RenderIndividualFaces
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ProceduralSpaceObject.cs
- 232 lines; kind=MonoBehaviour; type=ProceduralSpaceObject; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=2, PhysicMaterial=2, Mesh=2, Material=2, GetComponent=2, MonoBehaviour=1; unity_score=62
- ksp refs: PQS=5; fan_in=2 fan_out=3
- methods(10): CreateMeshCollider, CreateMeshVisual, GetSphereVolume, HasWrapperOfTypeName, Mod_Build, Mod_OnSetup, NewSeed, RangefinderGeneric, Reset, UpdateWrappers
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/ProceduralStarfield.cs
- 166 lines; kind=MonoBehaviour; type=ProceduralStarfield; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.Rendering
- unity: ParticleSystem=7, Transform=5, Time.=2, Material=2, GetComponent=2, GameObject=2; unity_score=74
- ksp refs: none; fan_in=0 fan_out=0
- methods(5): CreateStarFields, CreateStarsWithNewSystem, ExclusionZoneTest, Reset, Start
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ProfileTimer.cs
- 58 lines; kind=static class; type=ProfileTimer; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Time.=2; unity_score=17
- ksp refs: none; fan_in=2 fan_out=0
- methods(3): OutputTimer, Pop, Push
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/ProgressNode.cs
- 280 lines; kind=class; type=ProgressNode; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: FinePrint, FinePrint.Utilities, KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: none; unity_score=3
- ksp refs: GameEvents=8, ResearchAndDevelopment=6, ConfigNode=6, Reputation=4, Localizer=4, HighLogic=3; fan_in=35 fan_out=29
- methods(15): Achieve, AddOrAppendWorldFirstMessage, AwardProgress, AwardProgressInterval, AwardProgressRandomTech, AwardProgressStandard, CheatComplete, Complete, CrewSensitiveComplete, GetNodeSummary, Load, OnLoad, OnSave, Reach, Save
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/ProgressTracking.cs
- 533 lines; kind=ScenarioModule; type=ProgressTracking; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSPAchievements, UnityEngine
- unity: Lifecycle=3, GameObject=1; unity_score=22
- ksp refs: Localizer=50, FlightGlobals=5, GameEvents=2, ConfigNode=2, ScenarioModule=1, HighLogic=1; fan_in=25 fan_out=24
- methods(22): CheatEarlyProgression, CheatProgression, DebugAchievements, DebugSummary, FindNode, FindNodeRecurse, GenerateSummary, GetBodyTree, NodeComplete, NodeCompleteManned, NodeCompleteUnmanned, NodeReached, OnAchievementComplete, OnAwake, OnDestroy, OnLoad, OnSave, RecurseCheatProgression, Start, TestProgressDataPost, Update, generateAchievementsTree
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/ProgressTree.cs
- 158 lines; kind=class; type=ProgressTree; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=7
- ksp refs: ConfigNode=2, Vessel=1; fan_in=3 fan_out=4
- methods(10): AddNode, AllComplete, Contains, Deploy, GetEnumerator, GetTreeSummary, IterateVessels, Load, Save, Stow
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/PropCollider.cs
- 7 lines; kind=MonoBehaviour; type=PropCollider; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Collider=1; unity_score=55
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/Propellant.cs
- 144 lines; kind=class; type=Propellant; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=8
- ksp refs: PartResourceDefinition=4, ConfigNode=2, Part=1; fan_in=5 fan_out=5
- methods(6): GetFlowMode, GetFlowModeDescription, Load, Save, UpdateConnectedResources, getPartResourceDefinition
- auto suggestion: tier=A subsystem=PartModules-Engines

## Assembly-CSharp/PropertyIDs.cs
- 59 lines; kind=MonoBehaviour; type=PropertyIDs; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=32, MonoBehaviour=1, Lifecycle=1, LayerMask=1; unity_score=100
- ksp refs: none; fan_in=13 fan_out=0
- methods(1): Awake
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/PropObject.cs
- 146 lines; kind=MonoBehaviour; type=PropObject; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=15, Mesh=14, GetComponent=9, Transform=7, Material=5, Instantiate/Destroy=5; unity_score=97
- ksp refs: none; fan_in=0 fan_out=1
- methods(5): Create, CreateProxies, GetComponentOnParent, RecreateProxies, RefreshProxy
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/PropTools.cs
- 213 lines; kind=MonoBehaviour; type=PropTools; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=34
- ksp refs: none; fan_in=1 fan_out=0
- methods(4): CreatePropInfo, CreatePropInfoList, CreateProxyInfo, ProxyError
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/ProtoCrewMember.cs **MUST-OPEN (>800 lines)**
- 1375 lines; kind=class; type=ProtoCrewMember; bases=-; ancestry=-; ns=(global)
- usings: Expansions, Expansions.Missions, Expansions.Serenity, Experience, Experience.Effects, KSP.Localization, UnityEngine
- unity: GameObject=9, GetComponent=5, Instantiate/Destroy=2, Resources.=1; unity_score=10
- ksp refs: ProtoCrewMember=18, GameEvents=18, PhysicsGlobals=16, HighLogic=15, ConfigNode=13, KerbalRoster=12; fan_in=110 fan_out=35
- methods(45): ActiveFixedUpdate, ActiveUpdate, ArchiveFlightLog, CalculateExperiencePoints, ChangeName, CheckActive, CheckRespawnTimer, Die, Equals, GetEffect, GetHashCode, GetKerbalEVAPartName, GetKerbalIVAName, GetKerbalIconSuitPath, GetKerbalIconSuitSuffix, GetLocalizedStatus, GetLocalizedTrait, GetLocalizedType, HasEffect, InventoryCosts, InventoryMass, OnGameStateCreated, OnPartPack, OnPartUnpack, RegisterExperienceTraits, ResourceMass, Save, SaveEVAChute, SaveInventory, SetDefaultInventory, SetInactive, SetTimeForRespawn, SpawnKerbalDelegate, StartRespawnPeriod, ToString, UnregisterExperienceTraits, Update, UpdateExperience, _GToleranceMult, _MaxSustainedG, _Spawn, doubleDelegatePCM, voidDelegate_PCM, voidDelegate_PCM_Part, voidDelegate_PCM_UT
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/ProtoPartModuleSnapshot.cs
- 224 lines; kind=class; type=ProtoPartModuleSnapshot; bases=-; ancestry=-; ns=(global)
- usings: Expansions, Expansions.Missions, Expansions.Missions.Adjusters, UnityEngine
- unity: GameObject=1; unity_score=5
- ksp refs: ConfigNode=28, GameEvents=12, ProtoPartModuleSnapshot=7, PartModule=4, Part=1; fan_in=30 fan_out=7
- methods(9): AddPartModuleAdjuster, AddPartModuleAdjusterList, EnsureModuleValuesIsInitialized, GetListOfActiveAdjusters, Load, ProtoPartModuleRepair, RemovePartModuleAdjuster, RemovePartModuleAdjusterList, Save
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/ProtoPartResourceSnapshot.cs
- 101 lines; kind=class; type=ProtoPartResourceSnapshot; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=6, ConfigNode=5, AvailablePart=3, PartResourceDefinition=2, PartResource=2, Part=1; fan_in=11 fan_out=7
- methods(4): GetCurrentResourceInfo, Load, Save, UpdateConfigNodeAmounts
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/ProtoPartSnapshot.cs **MUST-OPEN (>800 lines)**
- 1312 lines; kind=class; type=ProtoPartSnapshot; bases=-; ancestry=-; ns=(global)
- usings: Expansions, Expansions.Missions.Adjusters, KSP.UI.Screens, UnityEngine
- unity: Transform=28, GetComponent=19, GameObject=15, Animation=14, Instantiate/Destroy=5, SetActive=2; unity_score=37
- ksp refs: ProtoCrewMember=33, ConfigNode=23, PartLoader=18, Part=15, ProtoPartSnapshot=13, GameEvents=11; fan_in=46 fan_out=31
- methods(17): AddProtoPartModuleAdjusters, ConfigurePart, CreatePart, FindAttachNode, FindAttachNodeSnapShot, FindModule, GetCrew, GetCrewIndex, HasCrew, Init, Load, LoadEVAInfoBySuitGender, ProtoPartRepair, RemoveCrew, RemoveProtoPartModuleAdjusters, Save, storePartRefs
- auto suggestion: tier=B subsystem=Save

## Assembly-CSharp/ProtoRDNode.cs
- 93 lines; kind=class; type=ProtoRDNode; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=5, ResearchAndDevelopment=2; fan_in=1 fan_out=3
- methods(3): FindNodeByID, LoadLinks, Save
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ProtoScenarioModule.cs
- 101 lines; kind=class; type=ProtoScenarioModule; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=4
- ksp refs: ConfigNode=7, ScenarioModule=4; fan_in=9 fan_out=4
- methods(4): GetData, Load, Save, SetTargetScenes
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/ProtoTargetInfo.cs
- 200 lines; kind=class; type=ProtoTargetInfo; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: UnityEngine
- unity: GameObject=5, MonoBehaviour=2, GetComponent=1; unity_score=24
- ksp refs: PartModule=8, Part=8, FlightGlobals=6, CelestialBody=6, Vessel=5, ConfigNode=3; fan_in=2 fan_out=8
- methods(3): FindTarget, Load, Save
- auto suggestion: tier=B subsystem=Orbit

## Assembly-CSharp/ProtoTechNode.cs
- 99 lines; kind=class; type=ProtoTechNode; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=11
- ksp refs: ConfigNode=5, AvailablePart=3, PartLoader=1; fan_in=10 fan_out=5
- methods(2): Save, UpdateFromTechNode
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ProtoVessel.cs **MUST-OPEN (>800 lines)**
- 1262 lines; kind=class; type=ProtoVessel; bases=-; ancestry=-; ns=(global)
- usings: FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: GameObject=12, Transform=7, GetComponent=6, Resources.=2, Instantiate/Destroy=2; unity_score=17
- ksp refs: ConfigNode=58, Vessel=35, Localizer=28, ProtoPartSnapshot=18, GameEvents=11, FlightGlobals=10; fan_in=72 fan_out=48
- methods(23): AddCrew, CacheLocalStrings, Clean, CreateDiscoveryNode, CreateOrbitNode, CreatePartNode, CreateVesselNode, CurrentFunds, CurrentReputation, CurrentScience, GetAllProtoPartsFromCrew, GetAllProtoPartsIncludingCargo, GetDisplayName, GetSituationString, GetVesselCrew, Load, LoadObjects, RebuildCrewCounts, RemoveCrew, ResetProtoPartSnapShots, Save, SaveVesselModules, verifyCrew
- auto suggestion: tier=B subsystem=Save

## Assembly-CSharp/ProtoWaypointInfo.cs
- 86 lines; kind=class; type=ProtoWaypointInfo; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: FinePrint
- unity: none; unity_score=0
- ksp refs: ConfigNode=3; fan_in=2 fan_out=4
- methods(3): FindWaypoint, Load, Save
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/PSpaceObject.cs
- 171 lines; kind=MonoBehaviour; type=PSpaceObject; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Mesh=11, Transform=8, LayerMask=6, GameObject=6, Renderer=5, PhysicMaterial=4; unity_score=93
- ksp refs: Part=1; fan_in=3 fan_out=3
- methods(10): Awake, CreateCollider, CreateConvexCollider, CreateVisual, GetVisualObjectRenderer, OnDestroy, SetMaterialColor, Setup, SetupPartParameters, onSOCCSetupComplete
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/PSystem.cs
- 248 lines; kind=MonoBehaviour; type=PSystem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=9, Transform=4, GetComponent=4, MonoBehaviour=1; unity_score=55
- ksp refs: ConfigNode=10, OrbitDriver=1, CelestialBody=1; fan_in=1 fan_out=5
- methods(10): AddBody, FindBody, GetBodies, LoadBody, LoadDatabase, OnDrawGizmos, OnDrawGizmosBody, Reset, SaveBody, SaveDatabase
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/PSystemBody.cs
- 21 lines; kind=MonoBehaviour; type=PSystemBody; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, GameObject=1; unity_score=50
- ksp refs: PQS=1, OrbitDriver=1, CelestialBody=1; fan_in=4 fan_out=4
- methods(0): 
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/PSystemManager.cs
- 249 lines; kind=MonoBehaviour; type=PSystemManager; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=27, Transform=20, GetComponent=11, Instantiate/Destroy=6, Lifecycle=2, MonoBehaviour=1; unity_score=85
- ksp refs: CelestialBody=13, Planetarium=3, ScaledSpace=2, PQS=2, OrbitDriver=2, FlightCamera=2; fan_in=9 fan_out=23
- methods(12): Awake, OnDestroy, PostSetup, Reset, SetBodyReferences, SetupLocal, SetupLocalSpace, SetupPSystem, SetupScaled, SetupScaledSpace, SpawnBody, Start
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/PSystemSetup.cs **MUST-OPEN (>800 lines)**
- 1176 lines; kind=MonoBehaviour; type=PSystemSetup; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, KSP.Localization, UnityEngine, UnityEngine.SceneManagement, Upgradeables
- unity: GameObject=47, Transform=35, GetComponent=19, Coroutine=18, Instantiate/Destroy=8, SetActive=7; unity_score=66
- ksp refs: PQS=27, Planetarium=19, FlightCamera=18, FlightGlobals=9, CelestialBody=9, ScaledSpace=7; fan_in=40 fan_out=34
- methods(47): AddLaunchSite, Awake, FindPQS, ForceInitPQS, GetFacilityDamage, GetFacilityLevel, GetLaunchSite, GetLaunchSiteBody, GetLaunchSiteBodyName, GetLaunchSiteDisplayName, GetSpaceCenterFacility, GetSpawnPoint, GetSpawnPointLatLonAlt, GetSpawnPointTransform, IsFacility, IsFacilityOrLaunchSite, IsLaunchFacility, IsLaunchSite, IsStockLaunchSite, LoadTestScene, OnDestroy, OnLevelLoaded, OnPSystemReady, OnSceneChange, OnSceneLoaded, RemoveLaunchSite, RemoveNonStockLaunchSites, ResetTransforms, SetDisabled, SetEditor, SetFlight, SetMainMenu, SetMissionEditor, SetPQSActive, SetPQSDisabled, SetPQSInactive, SetSpaceCentre, SetSpawnPointLatLonAlt, SetSpawnPointsLatLonAlt, SetTrackingStation, Setup, SetupFacilities, SetupLaunchSites, SetupSystem, SpawnAdditionalPrefabs ...
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/QuickSaveLoad.cs
- 494 lines; kind=MonoBehaviour; type=QuickSaveLoad; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI, SaveUpgradePipeline, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=7, SetActive=5, KeyCode=5, Input.=5, Lifecycle=3, SendMessage/Invoke=2; unity_score=54
- ksp refs: HighLogic=22, Localizer=21, GameSettings=9, InputLockManager=7, ConfigNode=6, FlightGlobals=5; fan_in=2 fan_out=36
- methods(21): Awake, CacheLocalStrings, CheckFilename, ConfirmDialog, EscapeString, FinishedSaveLoadDialogCallback, OnDestroy, OnLoadClose, OnSaveAsClose, QuickSave, QuickSaveClearToSave, SpawnQuickLoadDialog, SpawnSaveAsDialog, Update, doSave, onHoldComplete, onQuickloadPipelineFailed, onQuickloadPipelineFinished, quickLoad, quickSave, start
- auto suggestion: tier=C subsystem=Save

## Assembly-CSharp/RDArchivesAvatarController.cs
- 110 lines; kind=MonoBehaviour; type=RDArchivesAvatarController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1; unity_score=35
- ksp refs: Localizer=5; fan_in=1 fan_out=3
- methods(3): SetAvatarState, Start, getThoughtVector
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/RDPartCollisionHandler.cs
- 39 lines; kind=MonoBehaviour; type=RDPartCollisionHandler; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Collision/Force=6, MonoBehaviour=1, Lifecycle=1; unity_score=84
- ksp refs: KerbalEVA=1; fan_in=1 fan_out=1
- methods(4): OnCollisionEnter, OnCollisionExit, OnCollisionStay, Start
- auto suggestion: tier=B subsystem=Misc

## Assembly-CSharp/RDTech.cs
- 231 lines; kind=MonoBehaviour; type=RDTech; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: GameObject=3, Lifecycle=2, MonoBehaviour=1, Analytics=1; unity_score=40
- ksp refs: GameEvents=11, AvailablePart=11, ResearchAndDevelopment=4, PartLoader=3, Localizer=2, ConfigNode=2; fan_in=17 fan_out=20
- methods(11): AutoPurchaseAllParts, HandlePurchase, Load, OnDestroy, PartIsPurchased, PurchasePart, ResearchTech, Save, Start, UnlockTech, Warmup
- auto suggestion: tier=B subsystem=Science

## Assembly-CSharp/RecoverySituations.cs
- 15 lines; kind=enum; type=RecoverySituations; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/ReentryFXParticleTrail.cs
- 44 lines; kind=MonoBehaviour; type=ReentryFXParticleTrail; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: ParticleSystem=3, Transform=2, Lifecycle=2, GetComponent=2, Time.=1, Rigidbody=1; unity_score=91
- ksp refs: Vessel=2; fan_in=0 fan_out=2
- methods(2): FixedUpdate, Start
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ReflectionUtil.cs
- 61 lines; kind=static class; type=ReflectionUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=12
- ksp refs: none; fan_in=1 fan_out=0
- methods(1): GetAttributedTypesInAssemblies
- auto suggestion: tier=A subsystem=Modding

## Assembly-CSharp/Reputation.cs
- 192 lines; kind=ScenarioModule; type=Reputation; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: KSP.UI.Screens, UnityEngine
- unity: Lifecycle=2, GameObject=1; unity_score=35
- ksp refs: GameEvents=12, HighLogic=4, Reputation=3, ConfigNode=2, ScenarioModule=1, ProtoVessel=1; fan_in=43 fan_out=16
- methods(13): AddReputation, ModifyReputationDelta, OnAwake, OnCrewKilled, OnCurrenciesModified, OnDestroy, OnLoad, OnSave, SetReputation, Start, addReputation_discrete, addReputation_granular, onvesselRecoveryProcessing
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/ReputationWidget.cs
- 38 lines; kind=MonoBehaviour; type=ReputationWidget; bases=CurrencyWidget; ancestry=CurrencyWidget/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2; unity_score=46
- ksp refs: Reputation=5, GameEvents=2, HighLogic=1; fan_in=0 fan_out=6
- methods(5): Awake, DelayedStart, OnAboutToStart, OnDestroy, onReputationChanged
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/ResearchAndDevelopment.cs **MUST-OPEN (>800 lines)**
- 1210 lines; kind=ScenarioModule; type=ResearchAndDevelopment; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: FinePrint, FinePrint.Utilities, KSP.Localization, KSP.UI.Screens, KSP.UI.Screens.SpaceCenter.MissionSummaryDialog, UnityEngine
- unity: Lifecycle=2, GameObject=2, GetComponent=1; unity_score=21
- ksp refs: ScienceSubject=38, Localizer=26, AvailablePart=26, PartLoader=15, HighLogic=14, ConfigNode=14; fan_in=83 fan_out=48
- methods(56): AddExperimentalPart, AddScience, CanAfford, CheatAddScience, CheatTechnology, CheckForMissingParts, CountUniversalScience, GetBiomeTags, GetBiomeTagsLocalized, GetExperiment, GetExperimentIDs, GetExperimentSubject, GetMiniBiomeTags, GetMiniBiomeTagsLocalized, GetMiniBiomedisplayNameByScienceID, GetMiniBiomedisplayNameByUnityTag, GetNextScienceValue, GetReferenceDataValue, GetResults, GetScienceValue, GetSituationTags, GetSituationTagsDescriptions, GetSubjectByID, GetSubjectValue, GetSubjects, GetTechState, GetTechnologyState, GetTechnologyTitle, IsExperimentalPart, OnAwake, OnCurrenciesModified, OnDestroy, OnLeavingScene, OnLoad, OnSave, PartAssignmentSummary, PartModelPurchased, PartTechAvailable, RefreshTechTreeUI, RemoveExperimentalPart, ResearchedValidContractObjectives, ScienceTransmissionRewardString, SetScience, SetTechState, Start ...
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/ResetTransformOnSceneSwitch.cs
- 34 lines; kind=MonoBehaviour; type=ResetTransformOnSceneSwitch; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.SceneManagement
- unity: Transform=4, SceneManager=3, Lifecycle=2, MonoBehaviour=1; unity_score=86
- ksp refs: GameEvents=2; fan_in=1 fan_out=2
- methods(5): OnDestroy, OnLevelLoaded, OnSceneLoaded, OnSceneSwitch, Start
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/ResourceBroker.cs
- 71 lines; kind=class; type=ResourceBroker; bases=IResourceBroker; ancestry=IResourceBroker; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=8; fan_in=2 fan_out=5
- methods(4): AmountAvailable, RequestResource, StorageAvailable, StoreResource
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/ResourceCache.cs
- 126 lines; kind=MonoBehaviour; type=ResourceCache; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, GetComponent=1, GameObject=1; unity_score=35
- ksp refs: FlightGlobals=2, CelestialBody=2, GameDatabase=1; fan_in=3 fan_out=9
- methods(5): LoadAbundanceCache, LoadResourceInfo, OnDestroy, RequestSummary, ResetCache
- auto suggestion: tier=B subsystem=Resources

## Assembly-CSharp/ResourceConverter.cs
- 168 lines; kind=class; type=ResourceConverter; bases=-; ancestry=-; ns=(global)
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=6, PartResourceDefinition=3, PartModule=1, Part=1, GameEvents=1; fan_in=2 fan_out=13
- methods(2): CacheLocalStrings, ProcessRecipe
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ResourceData.cs
- 12 lines; kind=class; type=ResourceData; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/ResourceFlowMode.cs
- 12 lines; kind=enum; type=ResourceFlowMode; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=22 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/ResourceGameSettings.cs
- 288 lines; kind=class; type=ResourceGameSettings; bases=-; ancestry=-; ns=(global)
- usings: Expansions.Missions.Runtime, UnityEngine
- unity: none; unity_score=8
- ksp refs: ConfigNode=16, MissionSystem=3, HighLogic=2, Planetarium=1, Game=1; fan_in=2 fan_out=12
- methods(15): CheckDepletionNode, GenerateNewROCMissionSeed, GenerateNewSeed, GetBiomeLockInfo, GetDepletionInfo, GetPlanetScanInfo, GetValue, Load, Save, SaveBiomeUnlockNode, SaveDepletionNodeValue, SavePlanetUnlockNode, SetupBiomeLockInfo, SetupDepletionInfo, SetupPlanetScanInfo
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/ResourceMap.cs
- 688 lines; kind=MonoBehaviour; type=ResourceMap; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=21
- ksp refs: FlightGlobals=9, CelestialBody=5, PartResourceDefinition=2, Planetarium=1, GameEvents=1; fan_in=15 fan_out=21
- methods(21): Awake, DeDuplicate, DetermineBiomeName, FetchAllResourceNames, GenerateAbundanceSeed, GetAbundance, GetAbundanceFromCache, GetBestResourceData, GetCacheNames, GetDefaultSituation, GetDepletionNode, GetDepletionNodeValue, GetResourceByName, GetResourceItemList, IsBiomeUnlocked, IsPlanetScanned, OnDestroy, ResetCache, SetDepletionNodeValue, UnlockBiome, UnlockPlanet
- auto suggestion: tier=B subsystem=Resources

## Assembly-CSharp/ResourceRatio.cs
- 63 lines; kind=struct; type=ResourceRatio; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=11
- ksp refs: ConfigNode=2; fan_in=8 fan_out=2
- methods(2): Load, Save
- auto suggestion: tier=A subsystem=PartModules-Resources

## Assembly-CSharp/ResourceScenario.cs
- 58 lines; kind=ScenarioModule; type=ResourceScenario; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, Instantiate/Destroy=1, GetComponent=1, GameObject=1; unity_score=56
- ksp refs: ConfigNode=2, ScenarioModule=1; fan_in=8 fan_out=7
- methods(5): OnAwake, OnDestroy, OnLoad, OnSave, Start
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/ResourceSetup.cs
- 40 lines; kind=MonoBehaviour; type=ResourceSetup; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, GetComponent=1, GameObject=1; unity_score=59
- ksp refs: GameDatabase=2; fan_in=5 fan_out=2
- methods(2): LoadResourceConfig, OnDestroy
- auto suggestion: tier=B subsystem=Resources

## Assembly-CSharp/ResourceTransferMode.cs
- 6 lines; kind=enum; type=ResourceTransferMode; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/ResourceUtilities.cs
- 250 lines; kind=static class; type=ResourceUtilities; bases=-; ancestry=-; ns=(global)
- usings: Expansions.Missions.Editor, UnityEngine
- unity: none; unity_score=1
- ksp refs: ConfigNode=9, Vessel=1, HighLogic=1, CelestialBody=1; fan_in=17 fan_out=14
- methods(17): DeepClone, Deg2Rad, GetAltitude, GetBiome, GetDifficultyLevel, GetMaxDeltaTime, GetValue, HSL2RGB, ImportBiomeLockNodeList, ImportConfigNodeList, ImportDepletionNodeList, ImportPlanetScanNodeList, LoadNodeProperties, Rad2Lat, Rad2Lon, clampLat, clampLon
- auto suggestion: tier=A subsystem=Resources

## Assembly-CSharp/RetractableLadder.cs
- 365 lines; kind=PartModule; type=RetractableLadder; bases=PartModule,IMultipleDragCube; ancestry=PartModule/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=14, Animation=6, GetComponent=5, GameObject=5, Lifecycle=2, Collider=2; unity_score=64
- ksp refs: KSPField=5, HighLogic=4, KSPAction=3, ProtoPartModuleSnapshot=2, KSPEvent=2, PartModule=1; fan_in=1 fan_out=15
- methods(17): AssumeDragCubePosition, DemoteToPhysicslessPart, Extend, ExtendAction, FixedUpdate, GetDragCubeNames, OnInventoryModeDisable, OnPartCreatedFomInventory, OnStart, OnStoredInInventory, Retract, RetractAction, SetAnimationToRetracted, SetDragState, ToggleAction, Update, UsesProceduralDragCubes
- auto suggestion: tier=B subsystem=Kerbals

## Assembly-CSharp/RichTextUtil.cs
- 61 lines; kind=class; type=RichTextUtil; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=16
- ksp refs: none; fan_in=3 fan_out=1
- methods(8): Text, TextAdvance, TextAgent, TextAward, TextParam, TextParamsAward, TextPenalty, Title
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/RnDBuilding.cs
- 55 lines; kind=MonoBehaviour; type=RnDBuilding; bases=SpaceCenterBuilding; ancestry=SpaceCenterBuilding/MonoBehaviour; ns=(global)
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=22
- ksp refs: Localizer=6, HighLogic=5, GameEvents=4, InputLockManager=3, ResearchAndDevelopment=2; fan_in=0 fan_out=11
- methods(5): IsOpen, OnClicked, OnOnDestroy, OnPopupWarningDismiss, onDialogClose
- auto suggestion: tier=B subsystem=Science

## Assembly-CSharp/RnDCommsStream.cs
- 92 lines; kind=class; type=RnDCommsStream; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Coroutine=3, Time.=2; unity_score=29
- ksp refs: ScienceSubject=3, ResearchAndDevelopment=3, ProtoVessel=3; fan_in=1 fan_out=3
- methods(3): StreamData, submitStreamData, timeoutCoroutine
- auto suggestion: tier=A subsystem=Science

## Assembly-CSharp/RnDDebugUtil.cs
- 715 lines; kind=class; type=RnDDebugUtil; bases=-; ancestry=-; ns=(global)
- usings: KSP.UI.Screens, RUI.Icons.Simple, UnityEngine
- unity: IMGUI=86, UnityEngine.UI=22, PlayerPrefs=6, Texture=2, GameObject=2; unity_score=66
- ksp refs: AvailablePart=12, ConfigNode=10, HighLogic=5, ResearchAndDevelopment=4, PartResourceDefinition=1, PartLoader=1; fan_in=2 fan_out=16
- methods(25): CompareRDNodes, DiscardPart, DrawFilesPage, DrawNodePage, DrawPartsPage, DrawSciencePage, DrawSettingsPage, DrawTechPage, DrawTreePage, DrawWindow, GetSortedRDNodes, HasTextEditor, Init, LoadSettings, OnNodeSelected, OnNodeUnselected, OnTreeDespawn, OnTreeSpawn, RebuildTree, RefreshUI, ReloadPartAssignments, ReloadParts, SavePart, SaveSettings, Terminate
- auto suggestion: tier=C subsystem=Science

## Assembly-CSharp/ROC.cs
- 430 lines; kind=MonoBehaviour; type=ROC; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, UnityEngine
- unity: GameObject=19, Transform=11, Instantiate/Destroy=5, Raycast/Query=3, Lifecycle=3, GetComponent=2; unity_score=67
- ksp refs: GameEvents=4, HighLogic=3, FloatCurve=2, Vessel=1, ResearchAndDevelopment=1, Part=1; fan_in=8 fan_out=14
- methods(16): Awake, CheckCollision, CompareTo, DestroyRocFinderArrow, DestroyScanPointArrows, GetClosestScanPosition, GetClosestScanPositionWithRaycasts, GetVFXForceScale, OnDestroy, PerformExperiment, PickUpROC, SetRocEmitters, SetStats, Start, UpdateModelTransform, UpdateRocArrow
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/RocCBDefinition.cs
- 28 lines; kind=class; type=RocCBDefinition; bases=-; ancestry=-; ns=(global)
- usings: UniLinq
- unity: none; unity_score=12
- ksp refs: ConfigNode=1; fan_in=5 fan_out=1
- methods(1): Load
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/ROCDefinition.cs
- 188 lines; kind=class; type=ROCDefinition; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=35
- ksp refs: FloatCurve=3, ConfigNode=3; fan_in=7 fan_out=3
- methods(2): ContainsCBBiome, Load
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/ROCEmitter.cs
- 376 lines; kind=MonoBehaviour; type=ROCEmitter; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GetComponent=6, Transform=5, Lifecycle=5, GameObject=5, Time.=4, ParticleSystem=4; unity_score=61
- ksp refs: Part=6, GameEvents=4, KSPField=2; fan_in=1 fan_out=6
- methods(23): AddForceTarget, ApplyForces, Awake, BurstTimer, OnDisable, OnEnable, OnGamePause, OnGameUnpause, OnTriggerEnter, OnTriggerExit, RemoveForceTarget, ResetSecondsTillBurst, SetBurstEmitterMaxWait, SetBurstEmitterMinWait, SetForceScale, SetSFXClipPath, SetSFXPower, Start, StartBurstEmision, StartIdleEmision, StopBurstEmision, StopIdleEmision, Update
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/ROCManager.cs
- 498 lines; kind=MonoBehaviour; type=ROCManager; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Expansions, Expansions.Missions, UnityEngine
- unity: GameObject=18, Transform=6, Instantiate/Destroy=4, GetComponent=4, Lifecycle=2, SetActive=1; unity_score=56
- ksp refs: ConfigNode=6, CelestialBody=4, GameEvents=3, PQS=2, HighLogic=1, GameSettings=1; fan_in=13 fan_out=18
- methods(24): AddROCStats_Quad, AddROCStats_ROCs, Awake, ClearROCsCache, ClearStats, DisableAllROCs, EnableAllROCs, GetROCControlFromCB, GetTerrainTag, LoadRemovedROCs, OnGameNewStart, OnGameStateLoad, PopulateROCControlFromConfig, ROCRemoved, RemoveROC, SaveRemoveROCs, SetROCsStateForGame, Start, SubtractROCStats_Quad, SubtractROCStats_ROCs, ValidCBBiome, ValidCelestialBody, ValidateCBBiomeCombos, onMissionStarted
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/ROCScenario.cs
- 22 lines; kind=ScenarioModule; type=ROCScenario; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: Expansions, UnityEngine
- unity: Lifecycle=1, Instantiate/Destroy=1; unity_score=50
- ksp refs: ScenarioModule=1; fan_in=0 fan_out=6
- methods(1): Start
- auto suggestion: tier=A subsystem=DLC-BreakingGround

## Assembly-CSharp/ROCsSFX.cs
- 197 lines; kind=MonoBehaviour; type=ROCsSFX; bases=EffectBehaviour; ancestry=EffectBehaviour/MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Audio=5, Lifecycle=4, Time.=2, GetComponent=1, GameObject=1; unity_score=54
- ksp refs: GameEvents=4, KSPField=2, GameDatabase=1; fan_in=1 fan_out=6
- methods(16): CreateSource, FadeIn, FadeInPlayback, FadeOut, FadeOutPlayback, OnDisable, OnEnable, OnGamePause, OnGameUnpause, Play, PlayBurstSFX, PlayIdleSFX, SetClipsPath, SetSFXVolume, Start, Update
- auto suggestion: tier=B subsystem=DLC-BreakingGround

## Assembly-CSharp/Rotato.cs
- 15 lines; kind=MonoBehaviour; type=Rotato; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, Transform=1, MonoBehaviour=1; unity_score=67
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): FixedUpdate, Start
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/RUIHoverController.cs
- 102 lines; kind=class; type=RUIHoverController; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Coroutine=5, MonoBehaviour=2; unity_score=35
- ksp refs: none; fan_in=1 fan_out=0
- methods(3): Deschedule, Schedule, UpdateDaemon
- auto suggestion: tier=A subsystem=Misc

## Assembly-CSharp/RUIutils.cs
- 316 lines; kind=static class; type=RUIutils; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Camera=2; unity_score=8
- ksp refs: none; fan_in=34 fan_out=1
- methods(17): All, And, Any, CamToCamReposition, ColorToHex, Compare, Count, CutString, DecToHex, GetYesNoUIString, HexToChar, Repeat, SortAscDescPrimarySecondary, StringListToString, Sum, Where, WhereMutating
- auto suggestion: tier=A subsystem=Misc

## Assembly-CSharp/RunwayCollisionHandler.cs
- 176 lines; kind=MonoBehaviour; type=RunwayCollisionHandler; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: SetActive=3, Lifecycle=3, GameObject=3, Collision/Force=2, Collider=2, Transform=1; unity_score=60
- ksp refs: GameEvents=6; fan_in=0 fan_out=3
- methods(12): Awake, Disable, OnAllSectionsLoaded, OnCollisionEnter, OnDestroy, OnSceneLoadRequested, OnSectionLoaded, ReEnable, Reset, Start, onStructureCollapsing, onStructureRepaired
- auto suggestion: tier=B subsystem=Physics

## Assembly-CSharp/SafeAbortBackgroundWorker.cs
- 38 lines; kind=class; type=SafeAbortBackgroundWorker; bases=BackgroundWorker; ancestry=BackgroundWorker; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(2): Abort, OnDoWork
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/SandCastleLogic.cs
- 14 lines; kind=MonoBehaviour; type=SandCastleLogic; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1, GameObject=1; unity_score=68
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): Start
- auto suggestion: tier=B subsystem=Scene

## Assembly-CSharp/SaveMode.cs
- 7 lines; kind=enum; type=SaveMode; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=39 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SavingBtnSoftLock.cs
- 15 lines; kind=MonoBehaviour; type=SavingBtnSoftLock; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=4, MonoBehaviour=1; unity_score=80
- ksp refs: InputLockManager=2; fan_in=0 fan_out=2
- methods(2): OnPointerEnter, OnPointerExit
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/ScalarModuleSetHandler.cs
- 55 lines; kind=class; type=ScalarModuleSetHandler; bases=IConfigNode; ancestry=IConfigNode; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2, Part=1; fan_in=0 fan_out=4
- methods(4): Initialize, Load, Save, SetFXModules
- auto suggestion: tier=A subsystem=Part

## Assembly-CSharp/ScaledCamera.cs
- 68 lines; kind=MonoBehaviour; type=ScaledCamera; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Camera=6, Transform=5, Lifecycle=3, MonoBehaviour=1, GetComponent=1; unity_score=84
- ksp refs: ScaledSpace=1, FlightCamera=1; fan_in=13 fan_out=2
- methods(8): Awake, LateUpdate, OnDestroy, ResetFoV, ResetTarget, SetCameraClearFlag, SetFoV, SetTarget
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/ScaledFloatingOrigin.cs
- 11 lines; kind=MonoBehaviour; type=ScaledFloatingOrigin; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=3, MonoBehaviour=1, Lifecycle=1; unity_score=72
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): FixedUpdate
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/ScaledMovement.cs
- 88 lines; kind=MonoBehaviour; type=ScaledMovement; bases=MapObject; ancestry=MapObject/MonoBehaviour; ns=(global)
- usings: Expansions, Expansions.Missions, UnityEngine
- unity: Transform=7, GetComponent=3, GameObject=3; unity_score=69
- ksp refs: Vessel=4, ScaledSpace=4, CelestialBody=3, FlightGlobals=1; fan_in=8 fan_out=7
- methods(2): Create, OnLateUpdate
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/ScaledSpace.cs
- 178 lines; kind=MonoBehaviour; type=ScaledSpace; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=8, Transform=5, Lifecycle=4, SetActive=2, Instantiate/Destroy=2, UnityEngine.UI=1; unity_score=67
- ksp refs: Vector3d=8, ScaledSpace=2, CelestialBody=1; fan_in=35 fan_out=3
- methods(10): AddScaledSpaceObject, Awake, LateUpdate, LocalToScaledSpace, OnDestroy, RemoveScaledSpaceObject, ScaledToLocalSpace, Start, Toggle, ToggleAll
- auto suggestion: tier=B subsystem=Celestials

## Assembly-CSharp/ScaledSpaceFader.cs
- 121 lines; kind=MonoBehaviour; type=ScaledSpaceFader; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Shader=8, Renderer=6, Material=5, Transform=3, Lifecycle=3, GetComponent=2; unity_score=89
- ksp refs: FlightGlobals=3, GameSettings=2, Vector3d=1, MapView=1, CelestialBody=1; fan_in=3 fan_out=7
- methods(5): GetAngularSize, OnDestroy, Reset, Start, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ScaledSun.cs
- 26 lines; kind=MonoBehaviour; type=ScaledSun; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=67
- ksp refs: none; fan_in=1 fan_out=0
- methods(2): Awake, OnDestroy
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/ScenarioAchievements.cs
- 330 lines; kind=ScenarioModule; type=ScenarioAchievements; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: Contracts, FinePrint.Utilities, KSP.UI.Screens, KSPAchievements, UnityEngine
- unity: Lifecycle=2, Instantiate/Destroy=2, Coroutine=2; unity_score=30
- ksp refs: GameEvents=24, Vessel=11, HighLogic=7, CelestialBody=5, Part=4, Game=4; fan_in=0 fan_out=26
- methods(21): CheckAchievement, EndInTutorials, GetAchievementComplete, OnAwake, OnContractCompleted, OnCrewEVA, OnDestroy, OnFlagPlant, OnFlyby, OnLanding, OnLaunch, OnLoad, OnOrbit, OnPartsCoupled, OnReturnFromOrbit, OnSave, OnTargetedLanding, OnVesselRecovered, SetAchievementComplete, SetProvider, Start
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/ScenarioCreationOptions.cs
- 21 lines; kind=enum; type=ScenarioCreationOptions; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=26 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/ScenarioCustomWaypoints.cs
- 132 lines; kind=ScenarioModule; type=ScenarioCustomWaypoints; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=(global)
- usings: FinePrint, FinePrint.Utilities, UnityEngine
- unity: Lifecycle=1, Instantiate/Destroy=1; unity_score=28
- ksp refs: ConfigNode=8, GameEvents=4, ScenarioModule=1, FlightGlobals=1; fan_in=5 fan_out=10
- methods(7): AddWaypoint, OnAwake, OnDestroy, OnLoad, OnSave, ProcessWaypoint, RemoveWaypoint
- auto suggestion: tier=A subsystem=Misc
