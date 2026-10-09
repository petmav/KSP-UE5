# Review chunk 11 (small): 286 files, 31580 lines

## Assembly-CSharp-firstpass/Controller.cs
- 38 lines; kind=MonoBehaviour; type=Controller; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=11, Lifecycle=2, MonoBehaviour=1, KeyCode=1, Input.=1, GameObject=1; unity_score=96
- ksp refs: none; fan_in=14 fan_out=1
- methods(2): Awake, Update
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp-firstpass/DAE.cs **MUST-OPEN (>800 lines)**
- 1721 lines; kind=MonoBehaviour; type=DAE; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=31, Texture=26, Transform=17, Shader=9, Material=8, GetComponent=7; unity_score=58
- ksp refs: none; fan_in=4 fan_out=5
- methods(25): Build, CanParseGeometry, CheckScale, DAEGetColor, FormatFaceData, GetDAEMaterials, GetFaceData, GetGeometryData, GetMaterial, GetSceneGeo, GetUpAxis, GetValidTexturePath, Load, NormalMap, ReadDAE, RemoveWS, ScaleTexture, SolveMaterials, Start, UpPosConv, UpRotDirConv, UpScaleConv, cf, ci, pushtooffsetscale
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/FaceIndices.cs
- 8 lines; kind=struct; type=FaceIndices; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/FileGUI.cs
- 547 lines; kind=MonoBehaviour; type=FileGUI; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: IMGUI=79, UnityEngine.UI=21, Texture=21, Resources.=11, Screen/Cursor=2, MonoBehaviour=1; unity_score=90
- ksp refs: none; fan_in=1 fan_out=1
- methods(8): Display, FileSelector, GetIcon, OnGUI, ProjectBrowseSelector, ProjectBrowser, ProjectCreateSelector, Start
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/FileIO.cs
- 741 lines; kind=MonoBehaviour; type=FileIO; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GameObject=28, GetComponent=13, Transform=4, Texture=4, Mesh=4, Collider=4; unity_score=58
- ksp refs: none; fan_in=4 fan_out=5
- methods(20): CheckSave, CloseGUI, GenerateFileName, GenerateFolderName, InitGUI, InitPlatform, Load3d, LoadCallback, LoadHandle, Loaded, ProjectCreate, ProjectRegister, ProjectSilentCreate, ProjectSilentLoad, ProjectsMenu, ProjectsRead, ResolvePath, SaveMeshesToProject, SaveTextureToProject, SaveTexturesToProject
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/FlyAround.cs
- 20 lines; kind=MonoBehaviour; type=FlyAround; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=4, MonoBehaviour=1, Lifecycle=1; unity_score=78
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): Update
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp-firstpass/GeometryBuffer.cs
- 416 lines; kind=class; type=GeometryBuffer; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Material=6, Mesh=4, GetComponent=4, Renderer=3, GameObject=1; unity_score=39
- ksp refs: none; fan_in=3 fan_out=1
- methods(12): Check, PopulateMeshes, PushFace, PushGroup, PushMaterialGroup, PushNormal, PushObject, PushUV, PushVertex, ResolveLimit, ReturnMaterialNames, TangentSolver
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/Lingoona/Grammar.cs
- 133 lines; kind=class; type=Grammar; bases=-; ancestry=-; ns=Lingoona
- usings: UnityEngine
- unity: none; unity_score=8
- ksp refs: none; fan_in=2 fan_out=1
- methods(7): clearVariable, clearVariables, generateName, getLanguage, setLanguage, setVariable, useGrammar
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp-firstpass/Lingoona/NativeMethods.cs
- 39 lines; kind=class; type=NativeMethods; bases=-; ancestry=-; ns=Lingoona
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(9): clearVariable, clearVariables, generateName, getLanguage, initialize, isAvailable, setLanguage, setVariable, useGrammar
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp-firstpass/LingoonaGrammarFormatter.cs
- 28 lines; kind=MonoBehaviour; type=LingoonaGrammarFormatter; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: Lingoona, UnityEngine, UnityEngine.UI
- unity: Lifecycle=2, MonoBehaviour=1, GetComponent=1; unity_score=72
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): Start, Update
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp-firstpass/MeshOp.cs
- 47 lines; kind=MonoBehaviour; type=MeshOp; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: GetComponent=8, Mesh=6, Collider=5, MonoBehaviour=1, GameObject=1; unity_score=97
- ksp refs: none; fan_in=2 fan_out=1
- methods(1): AddReducedMeshColliders
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/MultiDim.cs
- 24 lines; kind=class; type=MultiDim; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: none; fan_in=0 fan_out=0
- methods(4): JaggedFloat, JaggedInt, JaggedString, JaggedVector3
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/OBJ.cs **MUST-OPEN (>800 lines)**
- 972 lines; kind=MonoBehaviour; type=OBJ; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Texture=27, GameObject=19, Shader=14, Material=9, GetComponent=6, WWW/WebRequest=5; unity_score=68
- ksp refs: none; fan_in=4 fan_out=5
- methods(15): Build, CalculateScaleOffset, CheckScale, GetMaterial, Load, NormalMap, ScaleTexture, SetGeometryData, SetMaterialData, SolveMaterials, Start, cf, ci, gc, pushscaleoffset
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/ParticleScaler.cs
- 27 lines; kind=MonoBehaviour; type=ParticleScaler; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=0 fan_out=0
- methods(4): ScaleShurikenSystems, ScaleTrailRenderers, Start, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/ProgMesh.cs
- 508 lines; kind=class; type=ProgMesh; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Mesh=1; unity_score=7
- ksp refs: none; fan_in=3 fan_out=0
- methods(17): CalculatePM, Collapse, ComputeAllEdgeCollapseCosts, ComputeEdgeCollapseCost, ComputeEdgeCostAtVertex, ComputeNormal, HasVertex, IsABorderVertex, Map, MinimumCostEdge, PermuteVertices, RemoveIfNonNeighbour, RemoveTriangle, RemoveVertex, ReplaceVertex, UpdateMesh, deleted
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/ProgMeshControl.cs
- 90 lines; kind=MonoBehaviour; type=ProgMeshControl; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: IMGUI=9, Mesh=6, GetComponent=5, Lifecycle=2, Collider=2, UnityEngine.UI=1; unity_score=92
- ksp refs: none; fan_in=0 fan_out=1
- methods(6): Display, Init, OnGUI, Start, Update, UpdateMesh
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/ProjectLoad.cs
- 67 lines; kind=MonoBehaviour; type=ProjectLoad; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: WWW/WebRequest=2, Instantiate/Destroy=2, Coroutine=2, MonoBehaviour=1, Lifecycle=1, GameObject=1; unity_score=71
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): Load, ReadProjectData, Start
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/ProjectModelLoad.cs
- 160 lines; kind=MonoBehaviour; type=ProjectModelLoad; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=7, Mesh=4, WWW/WebRequest=1; unity_score=62
- ksp refs: none; fan_in=0 fan_out=0
- methods(5): ReadMesh, ReadVector2Array16bit, ReadVector3Array16bit, ReadVector3ArrayBytes, ReadVector4ArrayBytes
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/ProjectModelSave.cs
- 181 lines; kind=MonoBehaviour; type=ProjectModelSave; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: MonoBehaviour=3, Mesh=2, Texture=1; unity_score=51
- ksp refs: none; fan_in=1 fan_out=0
- methods(6): WriteMesh, WriteTexture, WriteVector2Array16bit, WriteVector3Array16bit, WriteVector3ArrayBytes, WriteVector4ArrayBytes
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/ProjectSave.cs
- 12 lines; kind=MonoBehaviour; type=ProjectSave; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Start, Update
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/Properties/AssemblyInfo.cs
- 5 lines; kind=assembly-attrs; type=; bases=-; ancestry=-; ns=(global)
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/QuaternionExtensions.cs
- 34 lines; kind=static class; type=QuaternionExtensions; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: Transform=1; unity_score=39
- ksp refs: none; fan_in=0 fan_out=0
- methods(6): Inverse, Pitch, QuaternionFromMatrix, Roll, RotateInSpecifiedCoordinateSystem, Yaw
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/Resize.cs
- 10 lines; kind=class; type=Resize; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=3
- ksp refs: none; fan_in=20 fan_out=0
- methods(1): Array
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/SmoothFollow2.cs
- 33 lines; kind=MonoBehaviour; type=SmoothFollow2; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=7, Time.=2, MonoBehaviour=1, Lifecycle=1; unity_score=87
- ksp refs: none; fan_in=0 fan_out=0
- methods(1): Update
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp-firstpass/SpaceNavigator.cs
- 303 lines; kind=class; type=SpaceNavigator; bases=IDisposable; ancestry=IDisposable; ns=(global)
- usings: UnityEngine
- unity: PlayerPrefs=28, Application.=8, Shader=6, Transform=1; unity_score=43
- ksp refs: none; fan_in=13 fan_out=2
- methods(8): Dispose, GetRotation, GetTranslation, ReadSettings, RotationInLocalCoordSys, SetRotationSensitivity, SetTranslationSensitivity, WriteSettings
- auto suggestion: tier=B subsystem=Input

## Assembly-CSharp-firstpass/SpaceNavigatorNoDevice.cs
- 30 lines; kind=class; type=SpaceNavigatorNoDevice; bases=SpaceNavigator; ancestry=SpaceNavigator/IDisposable; ns=(global)
- usings: UnityEngine
- unity: Application.=2; unity_score=32
- ksp refs: none; fan_in=8 fan_out=1
- methods(3): Dispose, GetRotation, GetTranslation
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp-firstpass/SpaceNavigatorWindows.cs
- 85 lines; kind=class; type=SpaceNavigatorWindows; bases=SpaceNavigator; ancestry=SpaceNavigator/IDisposable; ns=(global)
- usings: TDx.TDxInput, UnityEngine
- unity: Application.=2; unity_score=24
- ksp refs: none; fan_in=1 fan_out=1
- methods(3): Dispose, GetRotation, GetTranslation
- auto suggestion: tier=A subsystem=Input

## Assembly-CSharp-firstpass/Torus.cs
- 39 lines; kind=MonoBehaviour; type=Torus; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=(global)
- usings: UnityEngine
- unity: Transform=7, GetComponent=5, Collider=5, Rigidbody=4, Collision/Force=2, MonoBehaviour=1; unity_score=100
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Awake, OnCollisionEnter
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp-firstpass/TriPoly.cs
- 266 lines; kind=class; type=TriPoly; bases=-; ancestry=-; ns=(global)
- usings: UnityEngine
- unity: none; unity_score=7
- ksp refs: none; fan_in=3 fan_out=0
- methods(6): Main, Patch, Triangulation, _area, _insideTriangle, _snip
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/A/-.cs
- 262 lines; kind=class; type=_0003; bases=-; ancestry=-; ns=A
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(8): _0001, _000D, _000E, _0010, _0015, _0016, _001B, _001F
- auto suggestion: tier=A subsystem=Misc

## Assembly-CSharp/A/AssemblyInfoAttribute.cs
- 11 lines; kind=attribute; type=AssemblyInfoAttribute; bases=Attribute; ancestry=Attribute; ns=A
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Misc

## Assembly-CSharp/Assets/_UI5/Rendering/Scripts/ImgFX_ColorMultiply.cs
- 34 lines; kind=MonoBehaviour; type=ImgFX_ColorMultiply; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Assets._UI5.Rendering.Scripts
- usings: UnityEngine
- unity: Shader=4, Texture=2, Material=2, Graphics/GL=2, MonoBehaviour=1, Lifecycle=1; unity_score=96
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Awake, OnRenderImage
- auto suggestion: tier=C subsystem=Modding

## Assembly-CSharp/CameraFXModules/CameraFXCollection.cs
- 108 lines; kind=class; type=CameraFXCollection; bases=-; ancestry=-; ns=CameraFXModules
- usings: UnityEngine
- unity: none; unity_score=8
- ksp refs: none; fan_in=4 fan_out=2
- methods(8): AddFX, Clear, Contains, GetLocalPositionFX, GetLocalRotationFX, IndexOf, InsertFX, RemoveFX
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraFXModules/CameraFXModule.cs
- 41 lines; kind=class; type=CameraFXModule; bases=-; ancestry=-; ns=CameraFXModules
- usings: UnityEngine
- unity: none; unity_score=13
- ksp refs: none; fan_in=3 fan_out=2
- methods(6): GetModuleID, IsActive, OnFXAdded, OnFXRemoved, UpdateLocalPosition, UpdateLocalRotation
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraFXModules/Fade.cs
- 80 lines; kind=class; type=Fade; bases=CameraFXModule; ancestry=CameraFXModule; ns=CameraFXModules
- usings: UnityEngine
- unity: Time.=4, Coroutine=3; unity_score=38
- ksp refs: none; fan_in=0 fan_out=4
- methods(6): OnFXAdded, OnFXRemoved, RemoveSelf, UpdateFade, UpdateLocalPosition, UpdateLocalRotation
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraFXModules/FadeWobbleFXParams.cs
- 14 lines; kind=class; type=FadeWobbleFXParams; bases=-; ancestry=-; ns=CameraFXModules
- usings: -
- unity: none; unity_score=12
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraFXModules/FlightCameraFX.cs
- 273 lines; kind=MonoBehaviour; type=FlightCameraFX; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=CameraFXModules
- usings: UnityEngine
- unity: Time.=6, Lifecycle=2, MonoBehaviour=1, Coroutine=1; unity_score=37
- ksp refs: GameEvents=28, FlightGlobals=14, Vessel=7, MapView=1; fan_in=0 fan_out=15
- methods(8): GetVibration, LateUpdate, OnDestroy, OnExplosion, OnVesselEvent, OnVesselSitChange, OnVesselUpdate, Start
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraFXModules/FXModuleParams.cs
- 12 lines; kind=class; type=FXModuleParams; bases=-; ancestry=-; ns=CameraFXModules
- usings: -
- unity: none; unity_score=12
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraFXModules/Views.cs
- 17 lines; kind=enum; type=Views; bases=-; ancestry=-; ns=CameraFXModules
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=8 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraFXModules/Wobble.cs
- 57 lines; kind=class; type=Wobble; bases=CameraFXModule; ancestry=CameraFXModule; ns=CameraFXModules
- usings: UnityEngine
- unity: Time.=2; unity_score=31
- ksp refs: none; fan_in=2 fan_out=4
- methods(4): OnFXAdded, OnFXRemoved, UpdateLocalPosition, UpdateLocalRotation
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraFXModules/WobbleFXParams.cs
- 16 lines; kind=class; type=WobbleFXParams; bases=-; ancestry=-; ns=CameraFXModules
- usings: -
- unity: none; unity_score=12
- ksp refs: none; fan_in=3 fan_out=2
- methods(0): 
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraFXModules/WobbleModes.cs
- 19 lines; kind=enum; type=WobbleModes; bases=-; ancestry=-; ns=CameraFXModules
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraKeyFrameEvents/CameraKeyFrameEvent.cs
- 15 lines; kind=MonoBehaviour; type=CameraKeyFrameEvent; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=CameraKeyFrameEvents
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=36
- ksp refs: none; fan_in=4 fan_out=0
- methods(1): RunEvent
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraKeyFrameEvents/Fade.cs
- 24 lines; kind=class; type=Fade; bases=CameraKeyFrameEvent; ancestry=CameraFXModule; ns=CameraKeyFrameEvents
- usings: UnityEngine
- unity: none; unity_score=12
- ksp refs: none; fan_in=0 fan_out=2
- methods(1): RunEvent
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraKeyFrameEvents/SceneTransition.cs
- 22 lines; kind=MonoBehaviour; type=SceneTransition; bases=CameraKeyFrameEvent; ancestry=CameraKeyFrameEvent/MonoBehaviour; ns=CameraKeyFrameEvents
- usings: UnityEngine
- unity: none; unity_score=36
- ksp refs: HighLogic=1; fan_in=0 fan_out=3
- methods(1): RunEvent
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CameraKeyFrameEvents/TextHeaderFlip.cs
- 15 lines; kind=MonoBehaviour; type=TextHeaderFlip; bases=CameraKeyFrameEvent; ancestry=CameraKeyFrameEvent/MonoBehaviour; ns=CameraKeyFrameEvents
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=2
- methods(1): RunEvent
- auto suggestion: tier=C subsystem=Camera

## Assembly-CSharp/CompoundParts/CModuleFuelLine.cs
- 74 lines; kind=PartModule; type=CModuleFuelLine; bases=CompoundPartModule,IModuleInfo; ancestry=CompoundPartModule/PartModule/MonoBehaviour; ns=CompoundParts
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=16
- ksp refs: GameEvents=4, Part=3, Localizer=3, HighLogic=1; fan_in=4 fan_out=9
- methods(10): CacheLocalStrings, CloseFuelLine, GetDrawModulePanelCallback, GetInfo, GetModuleDisplayName, GetModuleTitle, GetPrimaryField, OnStartFinished, OnTargetLost, OnTargetSet
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/CompoundParts/CModuleLinkedMesh.cs
- 319 lines; kind=PartModule; type=CModuleLinkedMesh; bases=CompoundPartModule,ICMTweakTarget; ancestry=CompoundPartModule/PartModule/MonoBehaviour; ns=CompoundParts
- usings: UnityEngine
- unity: GameObject=24, Transform=23, SetActive=22, GetComponent=6, Collider=5, Camera=1; unity_score=84
- ksp refs: HighLogic=11, KSPField=6, EditorLogic=3, Part=1, FlightCamera=1; fan_in=1 fan_out=9
- methods(12): GetReferenceTransform, GetSelectedColliders, OnPreviewAttachment, OnPreviewEnd, OnStart, OnTargetLost, OnTargetSet, OnTargetUpdate, SelectTweakTarget, SetSymmetryValues, SetTargetPointer, TrackAnchor
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/CompoundParts/CModuleStrut.cs
- 108 lines; kind=PartModule; type=CModuleStrut; bases=CompoundPartModule,IModuleInfo; ancestry=CompoundPartModule/PartModule/MonoBehaviour; ns=CompoundParts
- usings: KSP.Localization, UnityEngine
- unity: Rigidbody=3, Transform=2, GetComponent=2, GameObject=1; unity_score=58
- ksp refs: Part=5, Localizer=3, AttachNode=3, PartJoint=2, KSPField=2, HighLogic=2; fan_in=1 fan_out=12
- methods(9): CacheLocalStrings, GetDrawModulePanelCallback, GetInfo, GetModuleDisplayName, GetModuleTitle, GetPrimaryField, OnStartFinished, OnTargetLost, OnTargetSet
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/CompoundParts/CompoundPartModule.cs
- 53 lines; kind=PartModule; type=CompoundPartModule; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=CompoundParts
- usings: UnityEngine
- unity: GameObject=1; unity_score=37
- ksp refs: Part=2, PartModule=1; fan_in=4 fan_out=3
- methods(7): OnAwake, OnModuleAwake, OnPreviewAttachment, OnPreviewEnd, OnTargetLost, OnTargetSet, OnTargetUpdate
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/CompoundParts/ICompoundPartAnchorMethod.cs
- 7 lines; kind=interface; type=ICompoundPartAnchorMethod; bases=-; ancestry=-; ns=CompoundParts
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/Cursors/CursorController.cs
- 165 lines; kind=MonoBehaviour; type=CursorController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Cursors
- usings: UnityEngine, UnityEngine.SceneManagement
- unity: Lifecycle=4, SceneManager=3, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=45
- ksp refs: HighLogic=1; fan_in=1 fan_out=7
- methods(13): AddCursor, Awake, ChangeCursor, Clear, Contains, ForceDefaultCursor, OnDestroy, OnLevelLoaded, OnSceneLoaded, RemoveCursor, SetCursor, Start, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/Cursors/CursorItem.cs
- 21 lines; kind=class; type=CursorItem; bases=-; ancestry=-; ns=Cursors
- usings: -
- unity: none; unity_score=12
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/Cursors/CustomCursor.cs
- 9 lines; kind=class; type=CustomCursor; bases=-; ancestry=-; ns=Cursors
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(2): SetCursor, Unset
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/Cursors/TextureCursor.cs
- 22 lines; kind=class; type=TextureCursor; bases=CustomCursor; ancestry=CustomCursor; ns=Cursors
- usings: UnityEngine
- unity: Texture=1, Screen/Cursor=1; unity_score=41
- ksp refs: none; fan_in=2 fan_out=1
- methods(2): SetCursor, Unset
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/EditorGizmos/GizmoHandle.cs
- 133 lines; kind=MonoBehaviour; type=GizmoHandle; bases=MonoBehaviour,IMouseEvents; ancestry=MonoBehaviour; ns=EditorGizmos
- usings: UnityEngine
- unity: Material=7, Renderer=3, MonoBehaviour=2, GetComponent=1; unity_score=71
- ksp refs: none; fan_in=3 fan_out=1
- methods(15): BaseSetup, CanHover, GetInstance, OnMouseDown, OnMouseDrag, OnMouseEnter, OnMouseExit, OnMouseOut, OnMouseUp, On_MouseDown, On_MouseDrag, On_MouseEnter, On_MouseExit, On_MouseUp, SetLock
- auto suggestion: tier=C subsystem=Editor

## Assembly-CSharp/EditorGizmos/GizmoOffset.cs
- 231 lines; kind=MonoBehaviour; type=GizmoOffset; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=EditorGizmos
- usings: UniLinq, UnityEngine
- unity: Transform=19, GameObject=5, SetActive=4, Lifecycle=3, Camera=3, Instantiate/Destroy=2; unity_score=76
- ksp refs: GameSettings=2, GameEvents=2, HighLogic=1; fan_in=3 fan_out=9
- methods(11): Attach, Detach, LateUpdate, OnHandleMove, OnHandleMoveEnd, OnHandleMoveStart, SetCoordSystem, SnapToGrid, Start, Update, onEditorSnapChanged
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/EditorGizmos/GizmoOffsetHandle.cs
- 94 lines; kind=MonoBehaviour; type=GizmoOffsetHandle; bases=GizmoHandle; ancestry=GizmoHandle/MonoBehaviour; ns=EditorGizmos
- usings: UnityEngine
- unity: Transform=7, Material=2, Input.=2, Camera=2, Renderer=1, GetComponent=1; unity_score=80
- ksp refs: none; fan_in=1 fan_out=2
- methods(8): CanHover, IsFacingDirection, On_MouseDown, On_MouseDrag, On_MouseEnter, On_MouseExit, On_MouseUp, Setup
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/EditorGizmos/GizmoRotate.cs
- 209 lines; kind=MonoBehaviour; type=GizmoRotate; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=EditorGizmos
- usings: UniLinq, UnityEngine
- unity: Transform=19, Camera=3, Lifecycle=2, Instantiate/Destroy=2, MonoBehaviour=1, GetComponent=1; unity_score=72
- ksp refs: GameSettings=2, GameEvents=2, HighLogic=1; fan_in=3 fan_out=7
- methods(10): Attach, Detach, OnHandleRotate, OnHandleRotateEnd, OnHandleRotateStart, SetCoordSystem, SnapToGrid, Start, Update, onAngleSnapChanged
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/EditorGizmos/GizmoRotateHandle.cs
- 128 lines; kind=MonoBehaviour; type=GizmoRotateHandle; bases=GizmoHandle; ancestry=GizmoHandle/MonoBehaviour; ns=EditorGizmos
- usings: UnityEngine
- unity: Transform=9, Camera=2, Raycast/Query=1, Input.=1; unity_score=72
- ksp refs: none; fan_in=1 fan_out=4
- methods(12): CanHover, GetAngleBetweenPoints, GetMousePoint, GetPlane, On_MouseDown, On_MouseDrag, On_MouseEnter, On_MouseExit, On_MouseUp, ResetSpinner, RotateSpinner, Setup
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/Experience/Effects/AutopilotSkill.cs
- 77 lines; kind=class; type=AutopilotSkill; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=1
- ksp refs: Localizer=5, Part=2; fan_in=1 fan_out=5
- methods(5): GetDefaultValue, GetDescription, GetValue, OnRegister, OnUnregister
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/ConverterSkill.cs
- 27 lines; kind=class; type=ConverterSkill; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=0 fan_out=3
- methods(2): GetDefaultValue, GetDescription
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/DeployedScienceExpSkill.cs
- 46 lines; kind=class; type=DeployedScienceExpSkill; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: Expansions, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=1 fan_out=4
- methods(3): GetDefaultValue, GetDescription, GetValue
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/DeployedSciencePowerSkill.cs
- 58 lines; kind=class; type=DeployedSciencePowerSkill; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: Expansions, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=2 fan_out=4
- methods(4): GetDefaultValue, GetDescription, GetTraitBonus, GetValue
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/DrillSkill.cs
- 27 lines; kind=class; type=DrillSkill; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=0 fan_out=3
- methods(2): GetDefaultValue, GetDescription
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/EnginePower.cs
- 43 lines; kind=class; type=EnginePower; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=2, Localizer=1; fan_in=0 fan_out=5
- methods(5): GetDefaultValue, GetDescription, GetValue, OnRegister, OnUnregister
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/EVAChuteSkill.cs
- 44 lines; kind=class; type=EVAChuteSkill; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=2; fan_in=0 fan_out=4
- methods(3): GetValue, OnRegister, OnUnregister
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/ExternalExperimentSkill.cs
- 27 lines; kind=class; type=ExternalExperimentSkill; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=1 fan_out=3
- methods(2): GetDefaultValue, GetDescription
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/FailureRepairSkill.cs
- 66 lines; kind=class; type=FailureRepairSkill; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=2, Localizer=2; fan_in=0 fan_out=5
- methods(5): GetDefaultValue, GetDescription, GetValue, OnRegister, OnUnregister
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/FuelUsage.cs
- 43 lines; kind=class; type=FuelUsage; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=2, Localizer=1; fan_in=0 fan_out=5
- methods(5): GetDefaultValue, GetDescription, GetValue, OnRegister, OnUnregister
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/FullVesselControlSkill.cs
- 32 lines; kind=class; type=FullVesselControlSkill; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1, HighLogic=1; fan_in=3 fan_out=4
- methods(2): GetDefaultValue, GetDescription
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/GeeForceTolerance.cs
- 41 lines; kind=class; type=GeeForceTolerance; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1, HighLogic=1; fan_in=1 fan_out=5
- methods(3): GeeTolerance, GetDefaultValue, GetDescription
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/HeatProduction.cs
- 43 lines; kind=class; type=HeatProduction; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=2, Localizer=1; fan_in=0 fan_out=5
- methods(5): GetDefaultValue, GetDescription, GetValue, OnRegister, OnUnregister
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/MaxThrottle.cs
- 43 lines; kind=class; type=MaxThrottle; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=2, Localizer=1; fan_in=0 fan_out=5
- methods(5): GetDefaultValue, GetDescription, GetValue, OnRegister, OnUnregister
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/PartScienceReturn.cs
- 43 lines; kind=class; type=PartScienceReturn; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=2, Localizer=1; fan_in=0 fan_out=5
- methods(5): GetDefaultValue, GetDescription, GetValue, OnRegister, OnUnregister
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/RepairSkill.cs
- 73 lines; kind=class; type=RepairSkill; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=5, Part=2; fan_in=0 fan_out=5
- methods(5): GetDefaultValue, GetDescription, GetValue, OnRegister, OnUnregister
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/ScienceResetSkill.cs
- 27 lines; kind=class; type=ScienceResetSkill; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=1; fan_in=1 fan_out=3
- methods(2): GetDefaultValue, GetDescription
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/ScienceSkill.cs
- 40 lines; kind=class; type=ScienceSkill; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=2; fan_in=2 fan_out=4
- methods(5): GetDefaultValue, GetDescription, GetValue, OnRegister, OnUnregister
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/SpecialExperimentSkill.cs
- 25 lines; kind=class; type=SpecialExperimentSkill; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=2
- methods(2): GetDefaultValue, GetDescription
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/Effects/VesselScienceReturn.cs
- 43 lines; kind=class; type=VesselScienceReturn; bases=ExperienceEffect; ancestry=ExperienceEffect; ns=Experience.Effects
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=2, Localizer=1; fan_in=0 fan_out=5
- methods(5): GetDefaultValue, GetDescription, GetValue, OnRegister, OnUnregister
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/ExperienceEffect.cs
- 104 lines; kind=class; type=ExperienceEffect; bases=-; ancestry=-; ns=Experience
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=4, ConfigNode=2; fan_in=25 fan_out=3
- methods(8): GetDefaultValue, GetDescription, LoadFromConfig, OnLoadFromConfig, OnRegister, OnUnregister, Register, Unregister
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/ExperienceEffectConfig.cs
- 26 lines; kind=class; type=ExperienceEffectConfig; bases=-; ancestry=-; ns=Experience
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=3; fan_in=2 fan_out=1
- methods(1): Create
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/ExperienceSystemConfig.cs
- 137 lines; kind=class; type=ExperienceSystemConfig; bases=-; ancestry=-; ns=Experience
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: KerbalRoster=1, GameDatabase=1, ConfigNode=1; fan_in=1 fan_out=4
- methods(4): GetExperienceTraitConfig, GetTraitsNamesWithEffect, GetTraitsWithEffect, LoadTraitConfigs
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/ExperienceTrait.cs
- 155 lines; kind=class; type=ExperienceTrait; bases=-; ancestry=-; ns=Experience
- usings: KSP.UI.Screens, UnityEngine
- unity: none; unity_score=9
- ksp refs: Part=4, ProtoCrewMember=3, ConfigNode=2, KerbalRoster=1; fan_in=24 fan_out=9
- methods(11): AddEffect, Create, CrewMemberExperienceLevel, OnLoad, OnRegister, OnSave, OnUnregister, Register, SendStateMessage, SetupConfig, Unregister
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/Experience/ExperienceTraitConfig.cs
- 76 lines; kind=class; type=ExperienceTraitConfig; bases=-; ancestry=-; ns=Experience
- usings: UnityEngine
- unity: Texture=2; unity_score=27
- ksp refs: ConfigNode=2, GameDatabase=1; fan_in=5 fan_out=3
- methods(1): Create
- auto suggestion: tier=A subsystem=Kerbals

## Assembly-CSharp/FlightUIModeControllerUtil/FlightUIModeControllerUtil.cs
- 23 lines; kind=static class; type=FlightUIModeControllerUtil; bases=-; ancestry=-; ns=FlightUIModeControllerUtil
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=1
- methods(2): TransitionIndex, TransitionStateName
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/FlightUIModeControllerUtil/TabAction.cs
- 9 lines; kind=enum; type=TabAction; bases=-; ancestry=-; ns=FlightUIModeControllerUtil
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/FlightUIModeControllerUtil/UIModeCheckCallback.cs
- 4 lines; kind=delegate; type=UIModeCheckCallback; bases=-; ancestry=-; ns=FlightUIModeControllerUtil
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=2
- methods(1): UIModeCheckCallback
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/FlightUIModeControllerUtil/UIModePanelState.cs
- 56 lines; kind=class; type=UIModePanelState; bases=-; ancestry=-; ns=FlightUIModeControllerUtil
- usings: -
- unity: none; unity_score=7
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): ApplyMode
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/FlightUIModeControllerUtil/UIModeSwitchCallback.cs
- 4 lines; kind=delegate; type=UIModeSwitchCallback; bases=-; ancestry=-; ns=FlightUIModeControllerUtil
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=2
- methods(1): UIModeSwitchCallback
- auto suggestion: tier=A subsystem=FlightUI

## Assembly-CSharp/KSPAchievements/BaseConstruction.cs
- 55 lines; kind=class; type=BaseConstruction; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: GameEvents=3, Part=2, ConfigNode=2, CelestialBody=2, Localizer=1; fan_in=1 fan_out=8
- methods(3): OnLoad, OnSave, onPartsCoupled
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/CelestialBodyEscape.cs
- 40 lines; kind=class; type=CelestialBodyEscape; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: Analytics=2; unity_score=38
- ksp refs: GameEvents=4, CelestialBody=3, Vessel=1, Localizer=1; fan_in=1 fan_out=9
- methods(1): OnSOIEscaped
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/CelestialBodyFlight.cs
- 68 lines; kind=class; type=CelestialBodyFlight; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: Analytics=2; unity_score=26
- ksp refs: Vessel=6, GameEvents=3, Localizer=2, ConfigNode=2, CelestialBody=2, FlightGlobals=1; fan_in=1 fan_out=13
- methods(3): OnLoad, OnSave, onVesselSitChange
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/CelestialBodyFlyby.cs
- 40 lines; kind=class; type=CelestialBodyFlyby; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: Analytics=2; unity_score=38
- ksp refs: GameEvents=4, CelestialBody=3, Vessel=1, Localizer=1; fan_in=1 fan_out=9
- methods(1): OnVesselSOIChange
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/CelestialBodyLanding.cs
- 85 lines; kind=class; type=CelestialBodyLanding; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: Analytics=6; unity_score=46
- ksp refs: Vessel=8, GameEvents=4, Localizer=2, HighLogic=2, FlightGlobals=2, ConfigNode=2; fan_in=1 fan_out=14
- methods(4): IsValidVessel, OnLoad, OnSave, OnVesselSituationChange
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/CelestialBodyOrbit.cs
- 69 lines; kind=class; type=CelestialBodyOrbit; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: Analytics=2; unity_score=26
- ksp refs: GameEvents=4, Vessel=3, Localizer=2, ConfigNode=2, CelestialBody=2, FlightGlobals=1; fan_in=1 fan_out=13
- methods(3): OnLoad, OnSave, onVesselSitChange
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/CelestialBodyReturn.cs
- 114 lines; kind=class; type=CelestialBodyReturn; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: Analytics=6; unity_score=39
- ksp refs: GameEvents=6, Vessel=5, Localizer=4, Orbit=2, ConfigNode=2, CelestialBody=2; fan_in=1 fan_out=15
- methods(3): OnLoad, OnSave, OnVesselSituationChange
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/CelestialBodyScience.cs
- 99 lines; kind=class; type=CelestialBodyScience; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: Analytics=2; unity_score=20
- ksp refs: Localizer=4, GameEvents=4, FlightGlobals=2, ConfigNode=2, CelestialBody=2, Vessel=1; fan_in=1 fan_out=16
- methods(4): OnLoad, OnSave, OnScience, OnTriggeredScience
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/CelestialBodySplashdown.cs
- 55 lines; kind=class; type=CelestialBodySplashdown; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: Analytics=4; unity_score=47
- ksp refs: Vessel=8, GameEvents=4, Localizer=2, HighLogic=2, CelestialBody=2, FlightGlobals=1; fan_in=1 fan_out=11
- methods(2): IsValidVessel, OnVesselSituationChange
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/CelestialBodySuborbit.cs
- 67 lines; kind=class; type=CelestialBodySuborbit; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Vessel=7, GameEvents=3, Localizer=2, ConfigNode=2, CelestialBody=2, FlightGlobals=1; fan_in=1 fan_out=12
- methods(3): OnLoad, OnSave, onVesselSitChange
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/CelestialBodySubtree.cs
- 211 lines; kind=class; type=CelestialBodySubtree; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: UnityEngine
- unity: none; unity_score=4
- ksp refs: CelestialBody=3, GameEvents=2, ConfigNode=2, Vessel=1, Orbit=1; fan_in=7 fan_out=24
- methods(6): DebugBodyTree, LinkBodyHome, OnAchievementAchieve, OnLoad, OnSave, generateSubtreeSummary
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/CelestialBodyTransfer.cs
- 45 lines; kind=class; type=CelestialBodyTransfer; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: GameEvents=3, ConfigNode=2, CelestialBody=2, ProtoCrewMember=1, Part=1, Localizer=1; fan_in=1 fan_out=8
- methods(3): OnCrewTransfer, OnLoad, OnSave
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/CrewRecovery.cs
- 51 lines; kind=class; type=CrewRecovery; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: GameEvents=2, ConfigNode=2, Vessel=1, ProtoVessel=1, Localizer=1; fan_in=1 fan_out=8
- methods(3): OnLoad, OnSave, onVesselRecovered
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/CrewRef.cs
- 107 lines; kind=class; type=CrewRef; bases=IConfigNode; ancestry=IConfigNode; ns=KSPAchievements
- usings: -
- unity: none; unity_score=0
- ksp refs: ProtoCrewMember=9, HighLogic=2, ConfigNode=2, Vessel=1, ProtoVessel=1; fan_in=13 fan_out=6
- methods(4): FromProtoVessel, FromVessel, Load, Save
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/Docking.cs
- 39 lines; kind=class; type=Docking; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: Analytics=1; unity_score=24
- ksp refs: GameEvents=3, Part=2, CelestialBody=2, Localizer=1; fan_in=1 fan_out=7
- methods(1): OnPartsCoupled
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/FirstLaunch.cs
- 56 lines; kind=class; type=FirstLaunch; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: Coroutine=2; unity_score=22
- ksp refs: Vessel=7, GameEvents=4, Planetarium=2, Localizer=1; fan_in=1 fan_out=6
- methods(2): OnVesselSituationChange, TestFlight
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/FlagPlant.cs
- 57 lines; kind=class; type=FlagPlant; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=2, GameEvents=2, ConfigNode=2, CelestialBody=2, Vessel=1, FlightGlobals=1; fan_in=1 fan_out=9
- methods(3): OnFlagPlant, OnLoad, OnSave
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/PointOfInterest.cs
- 184 lines; kind=class; type=PointOfInterest; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization, KSP.UI.Screens
- unity: Analytics=1; unity_score=6
- ksp refs: Localizer=21, FlightGlobals=5, GameEvents=3, ConfigNode=2, CelestialBody=2, Vessel=1; fan_in=2 fan_out=22
- methods(4): GetUpliftText, OnAnomalyLoaded, OnLoad, OnSave
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/ReachSpace.cs
- 67 lines; kind=class; type=ReachSpace; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Vessel=4, GameEvents=4, ConfigNode=2, Localizer=1; fan_in=1 fan_out=10
- methods(3): OnLoad, OnSave, OnVesselSituationChange
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/RecordsAltitude.cs
- 114 lines; kind=class; type=RecordsAltitude; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Vessel=3, Planetarium=2, FlightGlobals=2, ConfigNode=2, Localizer=1, CelestialBody=1; fan_in=2 fan_out=14
- methods(3): OnLoad, OnSave, iterateVessels
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/RecordsDepth.cs
- 115 lines; kind=class; type=RecordsDepth; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Vessel=3, Planetarium=2, ConfigNode=2, Localizer=1, FlightGlobals=1; fan_in=2 fan_out=13
- methods(3): OnLoad, OnSave, iterateVessels
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/RecordsDistance.cs
- 134 lines; kind=class; type=RecordsDistance; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: Expansions, FinePrint, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Vessel=3, ConfigNode=2, Planetarium=1, Localizer=1, FlightGlobals=1; fan_in=2 fan_out=17
- methods(3): OnLoad, OnSave, iterateVessels
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/RecordsSpeed.cs
- 127 lines; kind=class; type=RecordsSpeed; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint, FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Vessel=6, Planetarium=2, ConfigNode=2, Localizer=1, FlightGlobals=1; fan_in=2 fan_out=13
- methods(3): OnLoad, OnSave, iterateVessels
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/Rendezvous.cs
- 56 lines; kind=class; type=Rendezvous; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization, UnityEngine
- unity: Coroutine=2; unity_score=30
- ksp refs: Vessel=5, FlightGlobals=4, GameEvents=2, CelestialBody=2, Localizer=1; fan_in=1 fan_out=7
- methods(2): OnVesselLoaded, TrackCandidateVessel
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/ReturnFrom.cs
- 11 lines; kind=enum; type=ReturnFrom; bases=-; ancestry=-; ns=KSPAchievements
- usings: -
- unity: none; unity_score=0
- ksp refs: Orbit=1; fan_in=6 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/Spacewalk.cs
- 57 lines; kind=class; type=Spacewalk; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: GameEvents=3, Vessel=2, Part=2, ConfigNode=2, CelestialBody=2, Localizer=1; fan_in=1 fan_out=9
- methods(3): OnGoEVA, OnLoad, OnSave
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/StationConstruction.cs
- 55 lines; kind=class; type=StationConstruction; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: GameEvents=3, Part=2, ConfigNode=2, CelestialBody=2, Vessel=1, Localizer=1; fan_in=1 fan_out=9
- methods(3): OnLoad, OnSave, onPartsCoupled
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/SurfaceEVA.cs
- 57 lines; kind=class; type=SurfaceEVA; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization
- unity: none; unity_score=0
- ksp refs: GameEvents=3, Part=2, Localizer=2, ConfigNode=2, CelestialBody=2, FlightGlobals=1; fan_in=1 fan_out=9
- methods(3): OnGoEVA, OnLoad, OnSave
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/TargetedLanding.cs
- 92 lines; kind=class; type=TargetedLanding; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: UnityEngine
- unity: Coroutine=3; unity_score=21
- ksp refs: Vessel=7, GameEvents=7, FlightGlobals=4, Orbit=2; fan_in=1 fan_out=10
- methods(2): OnVesselSituationChange, WaitAndCheckLandedTag
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAchievements/TowerBuzz.cs
- 148 lines; kind=class; type=TowerBuzz; bases=ProgressNode; ancestry=ProgressNode/IConfigNode; ns=KSPAchievements
- usings: FinePrint.Utilities, KSP.Localization, UnityEngine, Upgradeables
- unity: SetActive=4; unity_score=18
- ksp refs: GameEvents=6, Vessel=2, FlightGlobals=2, ConfigNode=2, Localizer=1, CelestialBody=1; fan_in=1 fan_out=16
- methods(9): Initialize, OnFacilityUpgrade, OnFlightGlobalsReady, OnFlightReady, OnLoad, OnSave, SetActive, Terminate, iterateVessels
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/KSPAchievements/VesselRef.cs
- 83 lines; kind=class; type=VesselRef; bases=IConfigNode; ancestry=IConfigNode; ns=KSPAchievements
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=2, Vessel=1; fan_in=11 fan_out=3
- methods(3): FromVessel, Load, Save
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/KSPAssets/KSPedia/KSPedia.cs
- 408 lines; kind=class; type=KSPedia; bases=KSPediaController; ancestry=KSPediaController; ns=KSPAssets.KSPedia
- usings: KSP.Localization, KSP.UI, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=12, KeyCode=5, Input.=5, GameObject=3, Lifecycle=2, EventSystems=2; unity_score=47
- ksp refs: Localizer=7, GameEvents=6, HighLogic=2; fan_in=5 fan_out=10
- methods(28): AssignDelegates, CalculateRelativeRectBounds, ContentsShowScreen, CreateContents, GetScreenTitle, InstantiateCategoryItem, InstantiateScreenItem, OnAspectControllerLayoutChange, OnBtnPauseFalse, OnBtnPauseTrue, OnClickBack, OnClickClose, OnClickForward, OnClickNext, OnClickPrevious, OnClickUp, OnCompileScreen, OnDatabaseReady, OnDestroy, OnGameSceneLoadRequested, OnHideScreen, OnInstantiateScreenPrefab, OnShowScreen, OnUnhide, ReadyCallback, SetPauseButtonState, SetupScale, Update
- auto suggestion: tier=C subsystem=Modding

## Assembly-CSharp/KSPAssets/KSPedia/KSPediaAspectController.cs
- 122 lines; kind=MonoBehaviour; type=KSPediaAspectController; bases=MonoBehaviour,ILayoutController,ILayoutSelfController; ancestry=MonoBehaviour; ns=KSPAssets.KSPedia
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=5, Transform=2, SendMessage/Invoke=1, MonoBehaviour=1, Lifecycle=1; unity_score=62
- ksp refs: none; fan_in=1 fan_out=1
- methods(9): GetHeightFromWidth, GetWidthFromHeight, OnRectTransformDimensionsChange, Reset, SetLayout, SetLayoutBestFit, SetLayoutHorizontal, SetLayoutVertical, Start
- auto suggestion: tier=C subsystem=Modding

## Assembly-CSharp/KSPAssets/KSPedia/KSPediaContentItem.cs
- 36 lines; kind=MonoBehaviour; type=KSPediaContentItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSPAssets.KSPedia
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, TMPro=1, MonoBehaviour=1; unity_score=65
- ksp refs: none; fan_in=0 fan_out=2
- methods(2): OnClick, Setup
- auto suggestion: tier=B subsystem=Modding

## Assembly-CSharp/KSPAssets/KSPedia/KSPediaSpawner.cs
- 128 lines; kind=MonoBehaviour; type=KSPediaSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSPAssets.KSPedia
- usings: KSP.UI, KSP.UI.Screens, KSPAssets.Loaders, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, Transform=4, SetActive=4, GameObject=4, Lifecycle=3, MonoBehaviour=1; unity_score=77
- ksp refs: InputLockManager=3, GameEvents=2, HighLogic=1; fan_in=3 fan_out=11
- methods(8): Awake, Hide, HideKSPedia, KSPediaReadyCallback, OnDestroy, Show, ShowKSPedia, Start
- auto suggestion: tier=C subsystem=Modding

## Assembly-CSharp/ModuleWheels/ModuleWheelBogey.cs
- 196 lines; kind=PartModule; type=ModuleWheelBogey; bases=ModuleWheelSubmodule; ancestry=ModuleWheelSubmodule/PartModule/MonoBehaviour; ns=ModuleWheels
- usings: UnityEngine
- unity: Transform=11, GameObject=6, Lifecycle=1; unity_score=64
- ksp refs: KSPField=11, TimeWarp=1, HighLogic=1; fan_in=0 fan_out=7
- methods(4): LateUpdate, OnGatherInfo, OnStart, OnWheelSetup
- auto suggestion: tier=B subsystem=PartModules-Wheels

## Assembly-CSharp/ModuleWheels/ModuleWheelBrakes.cs
- 160 lines; kind=PartModule; type=ModuleWheelBrakes; bases=ModuleWheelSubmodule; ancestry=ModuleWheelSubmodule/PartModule/MonoBehaviour; ns=ModuleWheels
- usings: Expansions.Missions.Adjusters, KSP.Localization, UnityEngine
- unity: UnityEngine.UI=1, Lifecycle=1, GameObject=1; unity_score=33
- ksp refs: KSPField=5, Localizer=2, HighLogic=2, KSPAction=1; fan_in=4 fan_out=16
- methods(10): ApplyTorqueAdjustments, BrakeAction, FixedUpdate, GetModuleDisplayName, OnGatherInfo, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStart, OnSubsystemsModified, OnWheelSetup
- auto suggestion: tier=A subsystem=PartModules-Wheels

## Assembly-CSharp/ModuleWheels/ModuleWheelDamage.cs
- 560 lines; kind=PartModule; type=ModuleWheelDamage; bases=ModuleWheelSubmodule,IConstruction; ancestry=ModuleWheelSubmodule/PartModule/MonoBehaviour; ns=ModuleWheels
- usings: KSP.Localization, UnityEngine
- unity: Transform=11, GameObject=9, SetActive=4, Time.=2, Lifecycle=2, Collision/Force=2; unity_score=50
- ksp refs: KSPField=15, GameEvents=11, Localizer=10, FlightGlobals=8, Part=6, GameSettings=6; fan_in=4 fan_out=29
- methods(23): CacheLocalStrings, CanBeDetached, CanBeOffset, CanBeRotated, DamageWheel, EventRepairExternal, FixedUpdate, GetDamageTransforms, HandleCollision, HandleCollisionVelocity, Initialize, OnDestroy, OnGatherInfo, OnInventoryModeDisable, OnInventoryModeEnable, OnStart, OnVesselModified, OnWheelSetup, ResetImmunity, SetDamaged, onPartCouple, onPartDecouple, onVesselFocusChange
- auto suggestion: tier=B subsystem=PartModules-Wheels

## Assembly-CSharp/ModuleWheels/ModuleWheelDeployment.cs **MUST-OPEN (>800 lines)**
- 898 lines; kind=PartModule; type=ModuleWheelDeployment; bases=ModuleWheelSubmodule,IMultipleDragCube; ancestry=ModuleWheelSubmodule/PartModule/MonoBehaviour; ns=ModuleWheels
- usings: KSP.Localization, UnityEngine
- unity: Transform=11, GameObject=10, Collider=4, Lifecycle=3, Animation=3, Instantiate/Destroy=2; unity_score=43
- ksp refs: KSPField=16, Localizer=13, HighLogic=11, ProtoPartModuleSnapshot=2, ConfigNode=2, Vessel=1; fan_in=5 fan_out=31
- methods(24): ActionToggle, AssumeDragCubePosition, CacheLocalStrings, CreateStandInCollider, EventToggle, FSMSetup, FindTransformAnimationTime, FixedUpdate, GetDragCubeNames, GetModuleDisplayName, GetToggleEventName, LateUpdate, OnFSMStateChange, OnGatherInfo, OnInventoryModeDisable, OnLoad, OnSave, OnStart, OnStoredInInventory, OnWheelSetup, SetInopSubsystems, ToggleDeployment, Update, UsesProceduralDragCubes
- auto suggestion: tier=B subsystem=PartModules-Wheels

## Assembly-CSharp/ModuleWheels/ModuleWheelLock.cs
- 20 lines; kind=PartModule; type=ModuleWheelLock; bases=ModuleWheelSubmodule; ancestry=ModuleWheelSubmodule/PartModule/MonoBehaviour; ns=ModuleWheels
- usings: -
- unity: none; unity_score=15
- ksp refs: KSPField=1; fan_in=1 fan_out=2
- methods(2): OnGatherInfo, OnWheelSetup
- auto suggestion: tier=A subsystem=PartModules-Wheels

## Assembly-CSharp/ModuleWheels/ModuleWheelMotor.cs
- 359 lines; kind=PartModule; type=ModuleWheelMotor; bases=ModuleWheelSubmodule,IResourceConsumer; ancestry=ModuleWheelSubmodule/PartModule/MonoBehaviour; ns=ModuleWheels
- usings: Expansions.Missions.Adjusters, KSP.Localization, UnityEngine
- unity: UnityEngine.UI=1, Lifecycle=1; unity_score=24
- ksp refs: KSPField=11, Localizer=9, KSPAction=4, PartResourceDefinition=3, FloatCurve=2, KSPEvent=1; fan_in=3 fan_out=23
- methods(23): ATsymPartUpdate, ActAutoTorqueToggle, ActionUIUpdate, ApplyTorqueAdjustments, CacheLocalStrings, CheckMotorState, EvtAutoTorqueToggle, FixedUpdate, GetConsumedResources, GetMaxSpeed, GetModuleDisplayName, GetMotorEnabled, GetMotorOrientationSign, MotorDisable, MotorEnable, MotorToggle, OnAwake, OnDriveUpdate, OnGatherInfo, OnModuleAdjusterAdded, OnModuleAdjusterRemoved, OnStart, OnWheelSetup
- auto suggestion: tier=A subsystem=PartModules-Wheels

## Assembly-CSharp/ModuleWheels/ModuleWheelMotorSteering.cs
- 81 lines; kind=PartModule; type=ModuleWheelMotorSteering; bases=ModuleWheelMotor; ancestry=ModuleWheelMotor/ModuleWheelSubmodule/PartModule/MonoBehaviour; ns=ModuleWheels
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=24
- ksp refs: KSPField=3, Localizer=1; fan_in=0 fan_out=6
- methods(4): GetMotorEnabled, OnDriveUpdate, OnGatherInfo, OnStart
- auto suggestion: tier=A subsystem=PartModules-Wheels

## Assembly-CSharp/ModuleWheels/ModuleWheelSteering.cs
- 454 lines; kind=PartModule; type=ModuleWheelSteering; bases=ModuleWheelSubmodule; ancestry=ModuleWheelSubmodule/PartModule/MonoBehaviour; ns=ModuleWheels
- usings: KSP.Localization, UnityEngine
- unity: Lifecycle=2, UnityEngine.UI=1, Transform=1, GameObject=1; unity_score=35
- ksp refs: KSPField=8, GameEvents=6, GameSettings=4, FloatCurve=4, Part=3, Localizer=2; fan_in=0 fan_out=26
- methods(25): ATsymPartUpdate, ActionSteeringAdjustUIUpdate, EvtAutoSteeringAdjustToggle, FixedUpdate, GetCoMFwdLength, GetSteeringResponseScale, OnAwake, OnDestroy, OnGatherInfo, OnLoad, OnPartActionUIDismiss, OnPartActionUIShown, OnSave, OnStart, OnSubsystemsModified, OnWheelRepaired, OnWheelSetup, ResetCaliper, SetCaliperUpdate, SetSteeringTweakablesSliders, SteeringToggle, findCoMfwdLength, findTurnCenter, updateCoordFrame, updateSteering
- auto suggestion: tier=A subsystem=PartModules-Wheels

## Assembly-CSharp/ModuleWheels/ModuleWheelSubmodule.cs
- 72 lines; kind=PartModule; type=ModuleWheelSubmodule; bases=PartModule; ancestry=PartModule/MonoBehaviour; ns=ModuleWheels
- usings: UnityEngine
- unity: Lifecycle=1, GetComponent=1, GameObject=1; unity_score=43
- ksp refs: PartModule=1, KSPField=1; fan_in=9 fan_out=5
- methods(7): OnAwake, OnDestroy, OnGatherInfo, OnSubsystemsModified, OnWheelInit, OnWheelSetup, SetWheelBase
- auto suggestion: tier=A subsystem=PartModules-Wheels

## Assembly-CSharp/ModuleWheels/ModuleWheelSuspension.cs
- 466 lines; kind=PartModule; type=ModuleWheelSuspension; bases=ModuleWheelSubmodule; ancestry=ModuleWheelSubmodule/PartModule/MonoBehaviour; ns=ModuleWheels
- usings: KSP.Localization, UnityEngine
- unity: Transform=5, Lifecycle=3, Time.=2, Collider=2, GetComponent=1, GameObject=1; unity_score=48
- ksp refs: KSPField=22, GameSettings=5, Localizer=4, GameEvents=3, HighLogic=2, Vessel=1; fan_in=1 fan_out=18
- methods(19): ATsymPartUpdate, ActionSpringDamperUIUpdate, BoostCurve, EvtAutoSpringDamperToggle, FixedUpdate, LateUpdate, OnAwake, OnDestroy, OnGatherInfo, OnInventoryModeDisable, OnInventoryModeEnable, OnStart, OnSubsystemsModified, OnWheelSetup, ResetSuspensionCollider, SuspensionSpringUpdate, ToggleTweakables, UpdateAutoBoost, onVesselPartCountChanged
- auto suggestion: tier=B subsystem=PartModules-Wheels

## Assembly-CSharp/ModuleWheels/WheelFrictionNode.cs
- 65 lines; kind=class; type=WheelFrictionNode; bases=-; ancestry=-; ns=ModuleWheels
- usings: UnityEngine
- unity: WheelCollider=4; unity_score=44
- ksp refs: ConfigNode=1; fan_in=0 fan_out=1
- methods(3): From, GetCurve, Load
- auto suggestion: tier=B subsystem=PartModules-Wheels

## Assembly-CSharp/ModuleWheels/WheelSubsystem.cs
- 37 lines; kind=class; type=WheelSubsystem; bases=-; ancestry=-; ns=ModuleWheels
- usings: -
- unity: WheelCollider=1; unity_score=24
- ksp refs: PartModule=2; fan_in=10 fan_out=1
- methods(1): IsType
- auto suggestion: tier=B subsystem=PartModules-Wheels

## Assembly-CSharp/ModuleWheels/WheelSubsystems.cs
- 73 lines; kind=class; type=WheelSubsystems; bases=-; ancestry=-; ns=ModuleWheels
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=1
- methods(6): AddSubsystem, Contains, GetReasons, HasType, RemoveSubsystem, ResetFlags
- auto suggestion: tier=A subsystem=PartModules-Wheels

## Assembly-CSharp/ModuleWheels/WheelUtil.cs
- 22 lines; kind=class; type=WheelUtil; bases=-; ancestry=-; ns=ModuleWheels
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(3): RpmToDegsSec, RpmToSpeed, SpeedToRpm
- auto suggestion: tier=A subsystem=PartModules-Wheels

## Assembly-CSharp/PartToolsLib/AnimationType.cs
- 11 lines; kind=enum; type=AnimationType; bases=-; ancestry=-; ns=PartToolsLib
- usings: -
- unity: Transform=1, ParticleSystem=1, Material=1, Light=1, Audio=1; unity_score=65
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/PartToolsLib/EntryType.cs
- 39 lines; kind=enum; type=EntryType; bases=-; ancestry=-; ns=PartToolsLib
- usings: -
- unity: Collider=4, Renderer=2, WheelCollider=1, ParticleSystem=1, Mesh=1, Material=1; unity_score=80
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/PartToolsLib/FileType.cs
- 7 lines; kind=enum; type=FileType; bases=-; ancestry=-; ns=PartToolsLib
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/PartToolsLib/ShaderType.cs
- 23 lines; kind=enum; type=ShaderType; bases=-; ancestry=-; ns=PartToolsLib
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/PartToolsLib/TextureType.cs
- 8 lines; kind=enum; type=TextureType; bases=-; ancestry=-; ns=PartToolsLib
- usings: -
- unity: Texture=1; unity_score=29
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/PreFlightTests/AirBreathingEnginesForIntakes.cs
- 75 lines; kind=class; type=AirBreathingEnginesForIntakes; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=5, ShipConstruct=2, Localizer=2, EditorLogic=1; fan_in=1 fan_out=11
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/AntennaPresent.cs
- 187 lines; kind=class; type=AntennaPresent; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: CommNet, KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=8, Part=2, Vessel=1, ShipConstruct=1, ProtoCrewMember=1, EditorLogic=1; fan_in=2 fan_out=16
- methods(6): CacheLocalStrings, GetConcernDescription, GetConcernTitle, GetSeverity, PartIsCrewControlled, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/CanAffordLaunchTest.cs
- 77 lines; kind=class; type=CanAffordLaunchTest; bases=IPreFlightTest; ancestry=IPreFlightTest; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=3, Funding=2, ShipConstruct=1; fan_in=3 fan_out=6
- methods(6): CacheLocalStrings, GetAbortOption, GetProceedOption, GetWarningDescription, GetWarningTitle, Test
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/ComOffset.cs
- 46 lines; kind=class; type=ComOffset; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=2, ShipConstruct=1; fan_in=1 fan_out=5
- methods(6): CacheLocalStrings, GetConcernDescription, GetConcernTitle, GetEditorFacilities, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/ContractEquipment.cs
- 56 lines; kind=class; type=ContractEquipment; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=3, ShipConstruct=2, Localizer=2, HighLogic=1, Game=1; fan_in=1 fan_out=7
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/CraftWithinMassLimits.cs
- 90 lines; kind=class; type=CraftWithinMassLimits; bases=IPreFlightTest; ancestry=IPreFlightTest; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=6, ShipConstruct=2; fan_in=4 fan_out=7
- methods(6): CacheLocalStrings, GetAbortOption, GetProceedOption, GetWarningDescription, GetWarningTitle, Test
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/CraftWithinPartCountLimit.cs
- 69 lines; kind=class; type=CraftWithinPartCountLimit; bases=IPreFlightTest; ancestry=IPreFlightTest; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=6, ShipConstruct=1; fan_in=4 fan_out=6
- methods(6): CacheLocalStrings, GetAbortOption, GetProceedOption, GetWarningDescription, GetWarningTitle, Test
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/CraftWithinSizeLimits.cs
- 135 lines; kind=class; type=CraftWithinSizeLimits; bases=IPreFlightTest; ancestry=IPreFlightTest; ns=PreFlightTests
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=8
- ksp refs: Localizer=21, ShipConstruct=2; fan_in=4 fan_out=10
- methods(6): CacheLocalStrings, GetAbortOption, GetProceedOption, GetWarningDescription, GetWarningTitle, Test
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/DecouplerFacing.cs
- 76 lines; kind=class; type=DecouplerFacing; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=7, Localizer=4, ShipConstruct=1, EditorLogic=1, AttachNode=1; fan_in=2 fan_out=9
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/DecouplersBeforeClamps.cs
- 64 lines; kind=class; type=DecouplersBeforeClamps; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization, KSP.UI.Screens
- unity: none; unity_score=0
- ksp refs: Localizer=4, Part=3, ShipConstruct=1, EditorLogic=1; fan_in=2 fan_out=9
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/DesignConcernBase.cs
- 79 lines; kind=class; type=DesignConcernBase; bases=IDesignConcern; ancestry=IDesignConcern; ns=PreFlightTests
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=1; fan_in=31 fan_out=4
- methods(8): GetAffectedParts, GetConcernDescription, GetConcernTitle, GetEditorFacilities, GetPreviousResult, GetSeverity, Test, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/DesignConcernSeverity.cs
- 9 lines; kind=enum; type=DesignConcernSeverity; bases=-; ancestry=-; ns=PreFlightTests
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=34 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/DockingPortAsDecoupler.cs
- 147 lines; kind=class; type=DockingPortAsDecoupler; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=3
- ksp refs: Part=5, Localizer=4, EditorLogic=2, AttachNode=1; fan_in=2 fan_out=8
- methods(8): AddToFailedParts, CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, RecurseHierarchy, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/DockingPortFacing.cs
- 153 lines; kind=class; type=DockingPortFacing; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization, UnityEngine
- unity: Transform=3; unity_score=17
- ksp refs: Part=5, Localizer=4, EditorLogic=2, AttachNode=1; fan_in=2 fan_out=8
- methods(8): AddToFailedParts, CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, RecurseHierarchy, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/DockingPortRCS.cs
- 50 lines; kind=class; type=DockingPortRCS; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization, KSP.UI.Screens
- unity: none; unity_score=0
- ksp refs: Part=3, Localizer=2, ShipConstruct=1, EditorLogic=1; fan_in=1 fan_out=9
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/ElectricBatteryAndNoCharge.cs
- 106 lines; kind=class; type=ElectricBatteryAndNoCharge; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: Resources.=1; unity_score=10
- ksp refs: Localizer=6, Part=5, ShipConstruct=1, EditorLogic=1; fan_in=2 fan_out=11
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/ElectricChargeAndNoBattery.cs
- 84 lines; kind=class; type=ElectricChargeAndNoBattery; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: Resources.=1; unity_score=12
- ksp refs: Part=5, Localizer=4, ShipConstruct=1, EditorLogic=1; fan_in=2 fan_out=11
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/ElectricChargeAndNoConsumer.cs
- 91 lines; kind=class; type=ElectricChargeAndNoConsumer; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=5, Localizer=4, ShipConstruct=1, PartResourceDefinition=1, EditorLogic=1; fan_in=2 fan_out=13
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/ElectricConsumerAndNoCharge.cs
- 112 lines; kind=class; type=ElectricConsumerAndNoCharge; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=6, Part=5, ShipConstruct=1, PartResourceDefinition=1, EditorLogic=1; fan_in=2 fan_out=14
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/EnginesJettisonedBeforeUse.cs
- 85 lines; kind=class; type=EnginesJettisonedBeforeUse; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=7, Localizer=4, ShipConstruct=1, EditorLogic=1; fan_in=2 fan_out=8
- methods(7): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, RecurseHierarchy, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/ExperimentalPartsAvailable.cs
- 111 lines; kind=class; type=ExperimentalPartsAvailable; bases=IPreFlightTest; ancestry=IPreFlightTest; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: AvailablePart=9, ResearchAndDevelopment=7, Localizer=3, PartLoader=2, ShipConstruct=1; fan_in=2 fan_out=12
- methods(5): GetAbortOption, GetProceedOption, GetWarningDescription, GetWarningTitle, Test
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/FacilityOperational.cs
- 58 lines; kind=class; type=FacilityOperational; bases=IPreFlightTest; ancestry=IPreFlightTest; ns=PreFlightTests
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=6
- ksp refs: Localizer=3; fan_in=8 fan_out=4
- methods(5): GetAbortOption, GetProceedOption, GetWarningDescription, GetWarningTitle, Test
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/HatchObstructed.cs
- 67 lines; kind=class; type=HatchObstructed; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=4, Localizer=2, ShipConstruct=1, EditorLogic=1, AttachNode=1; fan_in=2 fan_out=8
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/IDesignConcern.cs
- 23 lines; kind=interface; type=IDesignConcern; bases=-; ancestry=-; ns=PreFlightTests
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=1; fan_in=2 fan_out=3
- methods(0): 
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/InputAuthority.cs
- 45 lines; kind=class; type=InputAuthority; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: ShipConstruct=2, Localizer=2; fan_in=1 fan_out=4
- methods(5): CacheLocalStrings, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/IntakesForAirBreathingEngines.cs
- 75 lines; kind=class; type=IntakesForAirBreathingEngines; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=5, ShipConstruct=2, Localizer=2, EditorLogic=1; fan_in=1 fan_out=11
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/IPreFlightTest.cs
- 15 lines; kind=interface; type=IPreFlightTest; bases=-; ancestry=-; ns=PreFlightTests
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=14 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/KerbalSeatCollision.cs
- 64 lines; kind=class; type=KerbalSeatCollision; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=4, Localizer=2, ShipConstruct=1, EditorLogic=1; fan_in=1 fan_out=10
- methods(5): GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/LadderPresent.cs
- 70 lines; kind=class; type=LadderPresent; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=2, Localizer=2, ShipConstruct=1, EditorLogic=1; fan_in=2 fan_out=8
- methods(5): CacheLocalStrings, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/LandingGearPresent.cs
- 82 lines; kind=class; type=LandingGearPresent; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=4, ShipConstruct=1, EditorLogic=1; fan_in=2 fan_out=9
- methods(6): CacheLocalStrings, GetConcernDescription, GetConcernTitle, GetEditorFacilities, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/LaunchSiteClear.cs
- 123 lines; kind=class; type=LaunchSiteClear; bases=IPreFlightTest; ancestry=IPreFlightTest; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=6, Game=5, HighLogic=4, ProtoVessel=2; fan_in=2 fan_out=8
- methods(9): GetAbortOption, GetGameState, GetObstructingVesselIndex, GetObstructingVesselName, GetObstructingVessels, GetProceedOption, GetWarningDescription, GetWarningTitle, Test
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/MissingCrew.cs
- 85 lines; kind=class; type=MissingCrew; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=4, Localizer=4, ShipConstruct=1, EditorLogic=1; fan_in=2 fan_out=10
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/NoControlSources.cs
- 109 lines; kind=class; type=NoControlSources; bases=DesignConcernBase,IPreFlightTest; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=6, ProtoCrewMember=2, KerbalRoster=2, HighLogic=2, Vessel=1; fan_in=4 fan_out=14
- methods(9): GetAbortOption, GetConcernDescription, GetConcernTitle, GetProceedOption, GetSeverity, GetTestName, GetWarningDescription, GetWarningTitle, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/NoCrewInMannedVessel.cs
- 106 lines; kind=class; type=NoCrewInMannedVessel; bases=DesignConcernBase,IPreFlightTest; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=7, Vessel=1; fan_in=0 fan_out=9
- methods(9): GetAbortOption, GetConcernDescription, GetConcernTitle, GetProceedOption, GetSeverity, GetTestName, GetWarningDescription, GetWarningTitle, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/NonRootCmdMissaligned.cs
- 73 lines; kind=class; type=NonRootCmdMissaligned; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization, UnityEngine
- unity: Transform=4; unity_score=34
- ksp refs: Part=4, Localizer=4, EditorLogic=4, ShipConstruct=1; fan_in=2 fan_out=7
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/PreFlightTests/ParachuteOnEngineStage.cs
- 66 lines; kind=class; type=ParachuteOnEngineStage; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=9, Localizer=4, ShipConstruct=1, EditorLogic=1; fan_in=2 fan_out=10
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/ParachuteOnFirstStage.cs
- 73 lines; kind=class; type=ParachuteOnFirstStage; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization, KSP.UI.Screens
- unity: none; unity_score=0
- ksp refs: Part=4, Localizer=4, ShipConstruct=1, EditorLogic=1; fan_in=2 fan_out=7
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/ParachutePresent.cs
- 107 lines; kind=class; type=ParachutePresent; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=6, Localizer=6, Vessel=1, ShipConstruct=1, EditorLogic=1; fan_in=2 fan_out=15
- methods(7): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetEditorFacilities, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/RequiredIScienceDataTransmitter.cs
- 61 lines; kind=class; type=RequiredIScienceDataTransmitter; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=5, Localizer=3, ShipConstruct=1, EditorLogic=1; fan_in=2 fan_out=9
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/ResourceConsumersReachable.cs
- 104 lines; kind=class; type=ResourceConsumersReachable; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Resources.=2; unity_score=21
- ksp refs: Localizer=8, Part=5, PartResourceDefinition=2, ShipConstruct=1, EditorLogic=1; fan_in=1 fan_out=12
- methods(5): GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/ResourceContainersReachable.cs
- 77 lines; kind=class; type=ResourceContainersReachable; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Resources.=2; unity_score=27
- ksp refs: Part=5, Localizer=3, PartResourceDefinition=2, ShipConstruct=1, EditorLogic=1; fan_in=1 fan_out=11
- methods(5): GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/StationHubAttachments.cs
- 65 lines; kind=class; type=StationHubAttachments; bases=DesignConcernBase; ancestry=DesignConcernBase/IDesignConcern; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Part=4, Localizer=3, ShipConstruct=1, EditorLogic=1, AttachNode=1; fan_in=2 fan_out=8
- methods(6): CacheLocalStrings, GetAffectedParts, GetConcernDescription, GetConcernTitle, GetSeverity, TestCondition
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/PreFlightTests/WrongVesselTypeForLaunchSite.cs
- 48 lines; kind=class; type=WrongVesselTypeForLaunchSite; bases=IPreFlightTest; ancestry=IPreFlightTest; ns=PreFlightTests
- usings: KSP.Localization
- unity: none; unity_score=0
- ksp refs: Localizer=4; fan_in=2 fan_out=4
- methods(5): GetAbortOption, GetProceedOption, GetWarningDescription, GetWarningTitle, Test
- auto suggestion: tier=A subsystem=Editor

## Assembly-CSharp/ProceduralFairings/ColliderChunk.cs
- 105 lines; kind=class; type=ColliderChunk; bases=-; ancestry=-; ns=ProceduralFairings
- usings: UnityEngine
- unity: GameObject=7, Mesh=3, Collider=3, Transform=2, LayerMask=1, GetComponent=1; unity_score=61
- ksp refs: none; fan_in=1 fan_out=3
- methods(3): CreateColliderGO, GenerateColliderMesh, TriangulateColliderMesh
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ProceduralFairings/ColliderSection.cs
- 111 lines; kind=class; type=ColliderSection; bases=-; ancestry=-; ns=ProceduralFairings
- usings: UnityEngine
- unity: Mesh=6, GameObject=5, Collider=3, Transform=2, GetComponent=1; unity_score=64
- ksp refs: none; fan_in=2 fan_out=1
- methods(3): Create, CreateCapSection, CreateSection
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ProceduralFairings/FairingPanel.cs **MUST-OPEN (>800 lines)**
- 897 lines; kind=class; type=FairingPanel; bases=-; ancestry=-; ns=ProceduralFairings
- usings: UnityEngine, UnityEngine.Rendering
- unity: Transform=20, GameObject=9, Material=8, Renderer=6, Mesh=5, GetComponent=5; unity_score=43
- ksp refs: HighLogic=4, Part=2, AttachNode=1; fan_in=3 fan_out=11
- methods(41): AddFlag, AssignMesh, BuildMesh, CleanUp, ContainsSection, Despawn, FetchExistingFlagReferences, GenerateCompoundColliders, GenerateConvexMesh, GeneratePanelColliders, GenerateSectionColliders, GetArea, GetBarycenter, GetBaseHeight, GetBounds, GetCursorProximity, GetMidPoint, GetPanelHeight, GetPanelSlope, GetSlope, LazyInitMeshArray, ReadyFlagsForJettison, RemoveFlag, ReparentFlagstoPanels, SetArcPivot, SetArcPoints, SetCollapsedViewInstantly, SetExplodedView, SetNoseConePoints, SetOpacity, SetPanelMeshPoints, SetTgtExplodedView, SetTgtOpacity, SetupPanelArray, Spawn, TriangulateConePanel, TriangulatePanelMesh, TriangulateQuad, TriangulateTri, TriangulateTriangleMesh, UpdateInterpolations
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ProceduralFairings/FairingPanelFlags.cs
- 19 lines; kind=class; type=FairingPanelFlags; bases=-; ancestry=-; ns=ProceduralFairings
- usings: -
- unity: none; unity_score=12
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/ProceduralFairings/FairingXSection.cs
- 281 lines; kind=class; type=FairingXSection; bases=IConfigNode,IComparable<FairingXSection>; ancestry=IConfigNode; ns=ProceduralFairings
- usings: UnityEngine
- unity: Raycast/Query=5, Physics.=2; unity_score=29
- ksp refs: ConfigNode=4; fan_in=4 fan_out=3
- methods(11): AddAttachedFlag, AddNewFairingPanel, CircleCast, CompareTo, ConeCast, GetPanelIndexFromFlag, GetSlopeAngle, Load, RemoveAttachedFlag, Save, UpdateLerp
- auto suggestion: tier=B subsystem=PartModules-Structural

## Assembly-CSharp/ProceduralFairings/MeshArc.cs
- 26 lines; kind=class; type=MeshArc; bases=-; ancestry=-; ns=ProceduralFairings
- usings: UnityEngine
- unity: none; unity_score=3
- ksp refs: none; fan_in=2 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/ProceduralFairings/MeshPoint.cs
- 97 lines; kind=class; type=MeshPoint; bases=-; ancestry=-; ns=ProceduralFairings
- usings: UnityEngine
- unity: none; unity_score=21
- ksp refs: none; fan_in=3 fan_out=0
- methods(7): Clear, CopyFrom, GetColors, GetNormals, GetUV1, GetUV2, GetVerts
- auto suggestion: tier=A subsystem=PartModules-Structural

## Assembly-CSharp/Properties/AssemblyInfo.cs
- 8 lines; kind=assembly-attrs; type=; bases=-; ancestry=-; ns=(global)
- usings: A
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Misc

## Assembly-CSharp/Radiators/RadiatorData.cs
- 13 lines; kind=struct; type=RadiatorData; bases=-; ancestry=-; ns=Radiators
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=2; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/Radiators/RadiatorUtilities.cs
- 26 lines; kind=class; type=RadiatorUtilities; bases=-; ancestry=-; ns=Radiators
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=4; fan_in=1 fan_out=2
- methods(2): GetThermalData, TransferEnergy
- auto suggestion: tier=A subsystem=Thermal

## Assembly-CSharp/RUI/Algorithms/FlowGraph.cs
- 42 lines; kind=class; type=FlowGraph; bases=-; ancestry=-; ns=RUI.Algorithms
- usings: UnityEngine
- unity: none; unity_score=9
- ksp refs: Part=3; fan_in=3 fan_out=2
- methods(2): Add, GetPartVertex
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/RUI/Algorithms/SCCFlowGraph.cs
- 124 lines; kind=class; type=SCCFlowGraph; bases=-; ancestry=-; ns=RUI.Algorithms
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=37; fan_in=4 fan_out=5
- methods(6): Connect, GetAllDeliveries, GetAllRequests, GetConnectedComponentsInDeliveries, GetConnectedComponentsInRequests, Process
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/RUI/Algorithms/SCCFlowGraphUCFinder.cs
- 223 lines; kind=class; type=SCCFlowGraphUCFinder; bases=-; ancestry=-; ns=RUI.Algorithms
- usings: -
- unity: Resources.=1; unity_score=5
- ksp refs: Part=36, PartResourceDefinition=2, PartResource=1; fan_in=3 fan_out=8
- methods(7): AddEntrypoint, BuildEntrySets, GetUnreachableFuelDeliveries, GetUnreachableFuelRequests, IsEntryPoint, IsEntryPointDelivery, IsEntryPointRequest
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/RUI/Algorithms/StackFlowGraph.cs
- 146 lines; kind=class; type=StackFlowGraph; bases=-; ancestry=-; ns=RUI.Algorithms
- usings: CompoundParts, UnityEngine
- unity: Transform=10, GetComponent=1; unity_score=38
- ksp refs: Part=20, AttachNode=2; fan_in=3 fan_out=6
- methods(3): BuildTransformGuides, TransformGuideOnVertex, UpdatePartVertex
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/RUI/Algorithms/StronglyConnectedComponentFinder.cs
- 118 lines; kind=static class; type=StronglyConnectedComponentFinder; bases=-; ancestry=-; ns=RUI.Algorithms
- usings: -
- unity: none; unity_score=0
- ksp refs: Part=23; fan_in=1 fan_out=3
- methods(4): DetectCycle, DetectCycleSets, StrongConnect, StrongConnectSet
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/RUI/Algorithms/Vertex.cs
- 44 lines; kind=class; type=Vertex; bases=-; ancestry=-; ns=RUI.Algorithms
- usings: UnityEngine
- unity: Transform=2; unity_score=27
- ksp refs: none; fan_in=6 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/RUI/Icons/Selectable/Icon.cs
- 69 lines; kind=class; type=Icon; bases=-; ancestry=-; ns=RUI.Icons.Selectable
- usings: UnityEngine
- unity: Texture=4, UnityEngine.UI=1; unity_score=48
- ksp refs: none; fan_in=0 fan_out=0
- methods(3): Equals, GetHashCode, GetName
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/RUI/Icons/Selectable/IconLoader.cs
- 128 lines; kind=MonoBehaviour; type=IconLoader; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=RUI.Icons.Selectable
- usings: UnityEngine
- unity: UnityEngine.UI=1, Texture=1, MonoBehaviour=1, Lifecycle=1; unity_score=53
- ksp refs: GameDatabase=11; fan_in=0 fan_out=1
- methods(4): Awake, GetIcon, LoadIcon, SetupIcons
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/RUI/Icons/Simple/Icon.cs
- 55 lines; kind=class; type=Icon; bases=-; ancestry=-; ns=RUI.Icons.Simple
- usings: UnityEngine
- unity: Texture=2; unity_score=34
- ksp refs: none; fan_in=0 fan_out=0
- methods(3): Equals, GetHashCode, GetName
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/RUI/Icons/Simple/IconLoader.cs
- 110 lines; kind=MonoBehaviour; type=IconLoader; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=RUI.Icons.Simple
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1; unity_score=45
- ksp refs: GameDatabase=7; fan_in=0 fan_out=1
- methods(4): Awake, GetIcon, LoadIcon, SetupIcons
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/SaveUpgradePipeline/IterationResult.cs
- 9 lines; kind=enum; type=IterationResult; bases=-; ancestry=-; ns=SaveUpgradePipeline
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/LoadContext.cs
- 13 lines; kind=enum; type=LoadContext; bases=-; ancestry=-; ns=SaveUpgradePipeline
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=54 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/LogEntry.cs
- 30 lines; kind=class; type=LogEntry; bases=-; ancestry=-; ns=SaveUpgradePipeline
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=2
- methods(1): ToString
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/ModuleReplace.cs
- 41 lines; kind=class; type=ModuleReplace; bases=UpgradeScript; ancestry=UpgradeScript; ns=SaveUpgradePipeline
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=4; fan_in=1 fan_out=6
- methods(4): OnInit, OnTest, OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/NodeUtil.cs
- 110 lines; kind=static class; type=NodeUtil; bases=-; ancestry=-; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=2
- ksp refs: ConfigNode=12; fan_in=11 fan_out=4
- methods(8): GetCfgVersion, GetModuleNodesNamed, GetPartNodeNameValue, GetPartPosition, GetPartRotation, SetPartNodeNameValue, SetPartPosition, SetPartRotation
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/PartOffset.cs
- 70 lines; kind=class; type=PartOffset; bases=UpgradeScript; ancestry=UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=12
- ksp refs: ConfigNode=3; fan_in=4 fan_out=7
- methods(4): OnInit, OnTest, OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/PartReplace.cs
- 279 lines; kind=class; type=PartReplace; bases=PartOffset; ancestry=PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=9
- ksp refs: ConfigNode=17, PartLoader=1; fan_in=33 fan_out=8
- methods(8): AdjustCraftChildPartsPos, AdjustPartByOffset, AdjustSFSChildPartsPos, AdjustSFSRootChildPartsPos, OnInit, OnTest, OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/probeCoreSphere.cs
- 42 lines; kind=class; type=probeCoreSphere; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/SaveUpgradePipeline.cs
- 269 lines; kind=class; type=SaveUpgradePipeline; bases=-; ancestry=-; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=8
- ksp refs: ConfigNode=14; fan_in=57 fan_out=10
- methods(13): Init, InitScript, OnSetCfgNodeVersion_Default, PrintLog, ReflectUpgradeScripts, Run, RunIteration, RunLogCoD, RunTest, RunUpgrade, SanityCheck, SetCfgNodeVersion, TestExceptionCases
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/TestResult.cs
- 10 lines; kind=enum; type=TestResult; bases=-; ancestry=-; ns=SaveUpgradePipeline
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=9 fan_out=1
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/UpgradeModule.cs
- 19 lines; kind=attribute; type=UpgradeModule; bases=Attribute; ancestry=Attribute; ns=SaveUpgradePipeline
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=41 fan_out=2
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/UpgradeScript.cs
- 186 lines; kind=class; type=UpgradeScript; bases=-; ancestry=-; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=6
- ksp refs: ConfigNode=14; fan_in=6 fan_out=5
- methods(15): AppliesInContext, CheckMaxVersion, CheckMinVersion, GetCfgNodeVersion, GetNodeURL, Init, LogTestResults, OnInit, OnTest, OnUpgrade, RecurseNodes, Test, ToString, Upgrade, VersionTest
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v110_LandingGearOffset.cs
- 24 lines; kind=class; type=v110_LandingGearOffset; bases=PartOffset; ancestry=PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=12
- ksp refs: none; fan_in=0 fan_out=4
- methods(1): Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v110_LandingLegLT1Offset.cs
- 24 lines; kind=class; type=v110_LandingLegLT1Offset; bases=PartOffset; ancestry=PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=19
- ksp refs: none; fan_in=0 fan_out=4
- methods(1): Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v110_LandingLegLT2Offset.cs
- 24 lines; kind=class; type=v110_LandingLegLT2Offset; bases=PartOffset; ancestry=PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=14
- ksp refs: none; fan_in=0 fan_out=4
- methods(1): Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v110_WheelModuleOverhaul.cs
- 253 lines; kind=class; type=v110_WheelModuleOverhaul; bases=UpgradeScript; ancestry=UpgradeScript; ns=SaveUpgradePipeline
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=30; fan_in=0 fan_out=7
- methods(9): DeployStateUpgrade, OnTest, OnUpgrade, SalvageBrakes, SalvageDamage, SalvageGearDeployment, SalvageLegDeployment, SalvageMotor, SalvageSteering
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_engineLargeSkipper.cs
- 47 lines; kind=class; type=v1120_engineLargeSkipper; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_liquidEngine1_2.cs
- 47 lines; kind=class; type=v1120_liquidEngine1_2; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_liquidEngine2_2.cs
- 47 lines; kind=class; type=v1120_liquidEngine2_2; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_liquidEngine3.cs
- 47 lines; kind=class; type=v1120_liquidEngine3; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_liquidEngineMini.cs
- 47 lines; kind=class; type=v1120_liquidEngineMini; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_microEngine.cs
- 47 lines; kind=class; type=v1120_microEngine; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_mk1pod.cs
- 47 lines; kind=class; type=v1120_mk1pod; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_probeCoreOcto.cs
- 47 lines; kind=class; type=v1120_probeCoreOcto; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_probeCoreOcto2.cs
- 47 lines; kind=class; type=v1120_probeCoreOcto2; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_radialEngineMini.cs
- 35 lines; kind=class; type=v1120_radialEngineMini; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=19
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_RCSBlock.cs
- 35 lines; kind=class; type=v1120_RCSBlock; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=19
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_rocketNoseCone.cs
- 42 lines; kind=class; type=v1120_rocketNoseCone; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_ServiceBay_125.cs
- 57 lines; kind=class; type=v1120_ServiceBay_125; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=17
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_ServiceBay_250.cs
- 57 lines; kind=class; type=v1120_ServiceBay_250; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=17
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_Size3to2Adapter.cs
- 47 lines; kind=class; type=v1120_Size3to2Adapter; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_solidBooster.cs
- 47 lines; kind=class; type=v1120_solidBooster; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_solidBooster_sm.cs
- 47 lines; kind=class; type=v1120_solidBooster_sm; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_stackBiCoupler.cs
- 52 lines; kind=class; type=v1120_stackBiCoupler; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=17
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1120_stackTriCoupler.cs
- 57 lines; kind=class; type=v1120_stackTriCoupler; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=17
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v1123_dockingNode.cs
- 22 lines; kind=class; type=v1123_dockingNode; bases=ModuleReplace; ancestry=ModuleReplace/UpgradeScript; ns=SaveUpgradePipeline
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=4
- methods(1): Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v170_fuelTank1_2.cs
- 35 lines; kind=class; type=v170_fuelTank1_2; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=19
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v170_fuelTank2_2.cs
- 35 lines; kind=class; type=v170_fuelTank2_2; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=19
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v170_fuelTank3_2.cs
- 35 lines; kind=class; type=v170_fuelTank3_2; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=19
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v170_fuelTank4_2.cs
- 35 lines; kind=class; type=v170_fuelTank4_2; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=19
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v170_mk1_2.cs
- 35 lines; kind=class; type=v170_mk1_2; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=19
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v170_stackDecoupler.cs
- 47 lines; kind=class; type=v170_stackDecoupler; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v170_stackDecoupler1_2.cs
- 47 lines; kind=class; type=v170_stackDecoupler1_2; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v170_stackDecouplerMini.cs
- 47 lines; kind=class; type=v170_stackDecouplerMini; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v170_stackDecouplerSize3.cs
- 47 lines; kind=class; type=v170_stackDecouplerSize3; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v170_stackSeparator.cs
- 47 lines; kind=class; type=v170_stackSeparator; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v170_stackSeparatorBig.cs
- 35 lines; kind=class; type=v170_stackSeparatorBig; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=19
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v170_stackSeparatorMini.cs
- 47 lines; kind=class; type=v170_stackSeparatorMini; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v170_toroidalFuelTank.cs
- 47 lines; kind=class; type=v170_toroidalFuelTank; bases=PartReplace; ancestry=PartReplace/PartOffset/UpgradeScript; ns=SaveUpgradePipeline
- usings: UnityEngine
- unity: none; unity_score=18
- ksp refs: ConfigNode=2; fan_in=0 fan_out=5
- methods(2): OnUpgrade, Setup
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v180_ModuleAeroSurface.cs
- 105 lines; kind=class; type=v180_ModuleAeroSurface; bases=UpgradeScript; ancestry=UpgradeScript; ns=SaveUpgradePipeline
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=10, PartModule=1, PartLoader=1, Part=1; fan_in=0 fan_out=11
- methods(5): ConvertAeroAuthority, ConvertAeroAuthorityAxisGroup, GetBaseModule, OnTest, OnUpgrade
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/SaveUpgradePipeline/v180_ModuleControlSurface.cs
- 105 lines; kind=class; type=v180_ModuleControlSurface; bases=UpgradeScript; ancestry=UpgradeScript; ns=SaveUpgradePipeline
- usings: -
- unity: none; unity_score=0
- ksp refs: ConfigNode=10, PartModule=1, PartLoader=1, Part=1; fan_in=0 fan_out=11
- methods(5): ConvertControlAuthority, ConvertControlAuthorityAxisGroup, GetBaseModule, OnTest, OnUpgrade
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/Strategies/CurrencyExchanger.cs
- 149 lines; kind=class; type=CurrencyExchanger; bases=StrategyEffect; ancestry=StrategyEffect; ns=Strategies
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Coroutine=1; unity_score=10
- ksp refs: Reputation=6, ResearchAndDevelopment=3, Funding=3, Localizer=2, ConfigNode=1; fan_in=0 fan_out=12
- methods(7): CanActivate, GetDescription, GetTotalInput, GetTotalOutput, OnLoadFromConfig, OnRegister, OnUnregister
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/Strategies/DepartmentConfig.cs
- 94 lines; kind=class; type=DepartmentConfig; bases=-; ancestry=-; ns=Strategies
- usings: UnityEngine
- unity: Texture=2, GameObject=2; unity_score=33
- ksp refs: GameDatabase=2, ConfigNode=2; fan_in=5 fan_out=4
- methods(1): Create
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/Strategies/Effects/CurrencyConverter.cs
- 154 lines; kind=class; type=CurrencyConverter; bases=StrategyEffect; ancestry=StrategyEffect; ns=Strategies.Effects
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=7
- ksp refs: Localizer=5, GameEvents=2, ConfigNode=1; fan_in=0 fan_out=9
- methods(6): CanActivate, GetDescription, OnEffectQuery, OnLoadFromConfig, OnRegister, OnUnregister
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/Strategies/Effects/CurrencyOperation.cs
- 123 lines; kind=class; type=CurrencyOperation; bases=StrategyEffect; ancestry=StrategyEffect; ns=Strategies.Effects
- usings: KSP.Localization
- unity: none; unity_score=3
- ksp refs: Localizer=4, GameEvents=2, ConfigNode=1; fan_in=0 fan_out=8
- methods(5): GetDescription, OnEffectQuery, OnLoadFromConfig, OnRegister, OnUnregister
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/Strategies/Effects/ValueModifier.cs
- 76 lines; kind=class; type=ValueModifier; bases=StrategyEffect; ancestry=StrategyEffect; ns=Strategies.Effects
- usings: KSP.Localization
- unity: none; unity_score=5
- ksp refs: GameEvents=2, Localizer=1, ConfigNode=1; fan_in=0 fan_out=6
- methods(5): GetDescription, OnLoadFromConfig, OnRegister, OnUnregister, OnValueModifierQuery
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/Strategies/Strategy.cs
- 744 lines; kind=class; type=Strategy; bases=-; ancestry=-; ns=Strategies
- usings: KSP.Localization, KSP.UI.Screens, UnityEngine
- unity: Lifecycle=1; unity_score=4
- ksp refs: Localizer=22, ConfigNode=7, Reputation=5, Planetarium=5, ResearchAndDevelopment=3, Funding=3; fan_in=8 fan_out=20
- methods(25): Activate, AddEffect, CacheLocalStrings, CanActivate, CanBeActivated, CanBeDeactivated, CanDeactivate, Create, Deactivate, FactorLerp, GetEffectText, GetText, Load, MessageDeactivatedMaxTime, OnLoad, OnRegister, OnSave, OnUnregister, OnUpdate, Register, Save, SendStateMessage, SetupConfig, Unregister, Update
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/Strategies/StrategyConfig.cs
- 285 lines; kind=class; type=StrategyConfig; bases=-; ancestry=-; ns=Strategies
- usings: UnityEngine
- unity: Texture=2; unity_score=10
- ksp refs: ConfigNode=2, GameDatabase=1; fan_in=4 fan_out=4
- methods(2): Create, GetDepartmentConfig
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/Strategies/StrategyEffect.cs
- 94 lines; kind=class; type=StrategyEffect; bases=-; ancestry=-; ns=Strategies
- usings: UnityEngine
- unity: Lifecycle=1; unity_score=9
- ksp refs: ConfigNode=6; fan_in=6 fan_out=2
- methods(16): CanActivate, GetDescription, Load, LoadFromConfig, OnLoad, OnLoadFromConfig, OnRegister, OnSave, OnUnregister, OnUpdate, ParentLerp, Register, Save, ToPercentage, Unregister, Update
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/Strategies/StrategyEffectConfig.cs
- 29 lines; kind=class; type=StrategyEffectConfig; bases=-; ancestry=-; ns=Strategies
- usings: -
- unity: none; unity_score=12
- ksp refs: ConfigNode=3; fan_in=2 fan_out=1
- methods(1): Create
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/Strategies/StrategySystem.cs
- 357 lines; kind=ScenarioModule; type=StrategySystem; bases=ScenarioModule; ancestry=ScenarioModule/MonoBehaviour; ns=Strategies
- usings: UnityEngine
- unity: Coroutine=4, Lifecycle=2; unity_score=29
- ksp refs: ConfigNode=10, ScenarioModule=1; fan_in=5 fan_out=11
- methods(16): GenerateStrategyEffectTypes, GenerateStrategyTypes, GenerateTypes, GetStrategies, GetStrategyEffectType, GetStrategyType, HasActiveStrategy, HasConflictingActiveStrategies, LoadStrategies, OnAwake, OnDestroy, OnLoad, OnLoadRoutine, OnSave, Unregister, Update
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/Strategies/StrategySystemConfig.cs
- 108 lines; kind=class; type=StrategySystemConfig; bases=-; ancestry=-; ns=Strategies
- usings: UnityEngine
- unity: none; unity_score=10
- ksp refs: GameDatabase=2, ConfigNode=2; fan_in=1 fan_out=4
- methods(4): GetDepartmentConfig, GetStrategyConfig, LoadDepartmentConfigs, LoadStrategyConfigs
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/TestScripts/FacilitySceneryLoader.cs
- 46 lines; kind=MonoBehaviour; type=FacilitySceneryLoader; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=TestScripts
- usings: UnityEngine, UnityEngine.SceneManagement
- unity: SceneManager=4, Instantiate/Destroy=2, GameObject=2, MonoBehaviour=1, Lifecycle=1; unity_score=78
- ksp refs: none; fan_in=0 fan_out=1
- methods(3): LoadScenery, Start, UnloadScenery
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/TestScripts/HighLogicLoadedSceneSelector.cs
- 27 lines; kind=MonoBehaviour; type=HighLogicLoadedSceneSelector; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=TestScripts
- usings: UnityEngine
- unity: Time.=1, MonoBehaviour=1, Lifecycle=1; unity_score=55
- ksp refs: HighLogic=6; fan_in=0 fan_out=2
- methods(1): Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/TestScripts/KerbalEVATestInitializer.cs
- 79 lines; kind=MonoBehaviour; type=KerbalEVATestInitializer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=TestScripts
- usings: UnityEngine
- unity: GetComponent=7, GameObject=6, Coroutine=4, Transform=1, SetActive=1, MonoBehaviour=1; unity_score=83
- ksp refs: FlightGlobals=5, FlightCamera=4, Part=2, OrbitDriver=2, KerbalEVA=2, Vessel=1; fan_in=0 fan_out=13
- methods(3): InitializeEVA, OnSpawn, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/TestScripts/SceneryTestSceneLoader.cs
- 106 lines; kind=MonoBehaviour; type=SceneryTestSceneLoader; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=TestScripts
- usings: UnityEngine
- unity: Renderer=7, SetActive=2, Lifecycle=2, GameObject=2, Coroutine=2, QualitySettings/SystemInfo=1; unity_score=77
- ksp refs: MapView=9, Planetarium=3, FlightCamera=3, OrbitDriver=2, HighLogic=1, GameSettings=1; fan_in=0 fan_out=14
- methods(3): Awake, Start, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/UnityEngine/QuaternionD.cs
- 504 lines; kind=struct; type=QuaternionD; bases=-; ancestry=-; ns=UnityEngine
- usings: -
- unity: none; unity_score=1
- ksp refs: QuaternionD=73, Vector3d=60; fan_in=33 fan_out=1
- methods(41): Angle, AngleAxis, AxisAngle, Dot, Equals, Euler, EulerAngles, EulerRotation, FrameVectors, FromToRotation, GetHashCode, INTERNAL_CALL_AxisAngle, INTERNAL_CALL_FromToRotation, INTERNAL_CALL_Internal_FromEulerRad, INTERNAL_CALL_Internal_ToAxisAngleRad, INTERNAL_CALL_Internal_ToEulerRad, INTERNAL_CALL_Inverse, INTERNAL_CALL_Lerp, INTERNAL_CALL_Slerp, INTERNAL_CALL_UnclampedSlerp, Internal_FromEulerRad, Internal_ToAxisAngleRad, Internal_ToEulerRad, Inverse, Lerp, LookRotation, Quaternion, QuaternionD, RotateTowards, SetAxisAngle, SetEulerAngles, SetEulerRotation, SetFromToRotation, SetLookRotation, Slerp, ToAngleAxis, ToAxisAngle, ToEuler, ToEulerAngles, ToString, UnclampedSlerp
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/UnityEngine/UI/BoxSlider.cs
- 410 lines; kind=UIBehaviour; type=BoxSlider; bases=Selectable,IDragHandler,IEventSystemHandler,IInitializePotentialDragHandler,ICanvasElement; ancestry=Selectable; ns=UnityEngine.UI
- usings: UnityEngine.EventSystems, UnityEngine.Events
- unity: UnityEngine.UI=10, EventSystems=7, Transform=2, Lifecycle=2, GetComponent=1, Camera=1; unity_score=58
- ksp refs: none; fan_in=1 fan_out=0
- methods(17): GraphicUpdateComplete, LayoutComplete, MayDrag, OnDisable, OnDrag, OnEnable, OnInitializePotentialDrag, OnPointerDown, OnRectTransformDimensionsChange, Rebuild, Set, SetClass, SetStruct, SetY, UpdateCachedReferences, UpdateDrag, UpdateVisuals
- auto suggestion: tier=C subsystem=Core

## Assembly-CSharp/UnityEngine/Vector2d.cs
- 240 lines; kind=struct; type=Vector2d; bases=-; ancestry=-; ns=UnityEngine
- usings: -
- unity: none; unity_score=0
- ksp refs: Vector3d=2; fan_in=10 fan_out=1
- methods(16): Angle, ClampMagnitude, Distance, Dot, Equals, GetHashCode, Lerp, Max, Min, MoveTowards, Normalize, Scale, SqrMagnitude, ToString, Vector2d, Vector3d
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/UnityEngine/Vector4d.cs
- 278 lines; kind=struct; type=Vector4d; bases=-; ancestry=-; ns=UnityEngine
- usings: -
- unity: none; unity_score=1
- ksp refs: Vector3d=3; fan_in=2 fan_out=1
- methods(17): Distance, Dot, Equals, GetHashCode, Lerp, Magnitude, Max, Min, MoveTowards, Normalize, Project, Scale, SqrMagnitude, ToString, Vector3d, Vector4, Vector4d
- auto suggestion: tier=A subsystem=Core

## Assembly-CSharp/Upgradeables/KSCFacilityLevelText.cs
- 313 lines; kind=class; type=KSCFacilityLevelText; bases=-; ancestry=-; ns=Upgradeables
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=6
- ksp refs: Localizer=41, ResearchAndDevelopment=1; fan_in=1 fan_out=11
- methods(5): CreateFromAsset, GetText, GetValue, GetValueOrFallback, ProcessTextTags
- auto suggestion: tier=A subsystem=Career

## Assembly-CSharp/Upgradeables/UpgradeableFacility.cs
- 196 lines; kind=MonoBehaviour; type=UpgradeableFacility; bases=UpgradeableObject; ancestry=UpgradeableObject/MonoBehaviour; ns=Upgradeables
- usings: UnityEngine
- unity: Coroutine=7, GameObject=6, SetActive=3, Transform=1; unity_score=60
- ksp refs: GameEvents=6, HighLogic=5, ConfigNode=2; fan_in=8 fan_out=7
- methods(19): GetDowngradeCost, GetLevelText, GetNextLevelText, GetNormLevel, GetUpgradeCost, Load, LoadOnSceneStart, OnAwake, OnLevelLoaded, OnMissionsBuilderPQSLoaded, OnOnDestroy, OnStart, RegisterInstance, Reset, Save, SetLevel, UnregisterInstance, setNormLevel, waitForLevelSpawn
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/Upgradeables/UpgradeableInterior.cs
- 26 lines; kind=MonoBehaviour; type=UpgradeableInterior; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Upgradeables
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1; unity_score=55
- ksp refs: none; fan_in=5 fan_out=2
- methods(4): GetLevelCount, OnEnable, SetLevel, UpdateLevel
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/Upgradeables/UpgradeableInteriorMaterial.cs
- 35 lines; kind=MonoBehaviour; type=UpgradeableInteriorMaterial; bases=UpgradeableInterior; ancestry=UpgradeableInterior/MonoBehaviour; ns=Upgradeables
- usings: UnityEngine
- unity: Texture=1, Shader=1, Material=1; unity_score=72
- ksp refs: GameDatabase=1; fan_in=0 fan_out=2
- methods(3): GetLevelCount, SetLevel, UpdateLevel
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/Upgradeables/UpgradeableInteriorScene.cs
- 49 lines; kind=MonoBehaviour; type=UpgradeableInteriorScene; bases=UpgradeableInterior; ancestry=UpgradeableInterior/MonoBehaviour; ns=Upgradeables
- usings: UnityEngine, UnityEngine.SceneManagement
- unity: Transform=2, SceneManager=2, GameObject=2, Coroutine=2, Instantiate/Destroy=1; unity_score=79
- ksp refs: none; fan_in=1 fan_out=1
- methods(4): GetLevelCount, SetLevel, UpdateLevel, loadInteriorScene
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/Upgradeables/UpgradeableInteriorSprite.cs
- 32 lines; kind=MonoBehaviour; type=UpgradeableInteriorSprite; bases=UpgradeableInterior; ancestry=UpgradeableInterior/MonoBehaviour; ns=Upgradeables
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, Texture=1; unity_score=67
- ksp refs: none; fan_in=0 fan_out=1
- methods(3): GetLevelCount, SetLevel, UpdateLevel
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/Upgradeables/UpgradeableInteriorTexture.cs
- 36 lines; kind=MonoBehaviour; type=UpgradeableInteriorTexture; bases=UpgradeableInterior; ancestry=UpgradeableInterior/MonoBehaviour; ns=Upgradeables
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, Texture=1; unity_score=64
- ksp refs: GameDatabase=1; fan_in=0 fan_out=2
- methods(3): GetLevelCount, SetLevel, UpdateLevel
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/Upgradeables/UpgradeableObject.cs
- 247 lines; kind=MonoBehaviour; type=UpgradeableObject; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=Upgradeables
- usings: UnityEngine
- unity: Transform=13, GameObject=12, Instantiate/Destroy=5, Lifecycle=3, Application.=2, MonoBehaviour=1; unity_score=70
- ksp refs: GameEvents=5, HighLogic=2; fan_in=6 fan_out=6
- methods(16): Awake, ClearID, CompileAssets, CompileID, Despawn, OnAwake, OnDestroy, OnGameSceneSwitchRequested, OnOnDestroy, OnStart, SetLevel, Setup, SetupLevels, Spawn, Start, setLevel
- auto suggestion: tier=B subsystem=Career

## Assembly-CSharp/Upgradeables/UpgradeableSlave.cs
- 88 lines; kind=MonoBehaviour; type=UpgradeableSlave; bases=UpgradeableObject; ancestry=UpgradeableObject/MonoBehaviour; ns=Upgradeables
- usings: UnityEngine
- unity: Coroutine=1; unity_score=32
- ksp refs: GameEvents=2; fan_in=1 fan_out=5
- methods(6): CompileIDs, OnAwake, OnObjectLevelChange, OnOnDestroy, OnStart, UpdateLevel
- auto suggestion: tier=B subsystem=Career
