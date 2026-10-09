# Unity API surface used by KSP-authored code (shim spec for the re-architect tier)

Generated 2026-10-08 by `analysis/tiering/scripts/unity_surface.py` and `surface_report.py`. Scope: the 2,867 KSP-authored files in `tiering.csv` (Assembly-CSharp without the embedded third-party namespaces, plus the reviewed part of Assembly-CSharp-firstpass). Tier splits use the final `tiering.csv`.

## How to read this

- **Type table**: a file counts as using a Unity type if the type name appears as an identifier after comments and strings are stripped, and the file imports the type's namespace. Unity names that KSP also declares (e.g. `Input`, `Renderer`, `Resources`, `Plane`, `Ray`, `WrapMode`; full list at the end) are excluded from the type table to avoid false hits; their usage is covered by the API-family table, which uses member-access regexes such as `Input.`.
- **Static members** are exact (`Type.Member` occurrences). **Instance members** are approximate: they count `.member` accesses whose name is a public member of the Unity type in files that use that type, so names shared with other types (e.g. `.position` on Transform vs Rigidbody, `.enabled`) are conflated.
- **Equivalent / shim**: what the C++ core provides (tiers A and B) or what UE5 replaces it with (tier C).

## 1. API families by tier

Files / references per tier (A = transpile, B = re-architect, C = UE5 rewrite). Families with zero A use need no core shim. Families used by B need the core shim. Families used only by C map to UE5 directly.

| API family | Files | Refs | A files/refs | B files/refs | C files/refs | Core shim needed? |
|---|---:|---:|---|---|---|---|
| Lifecycle | 1103 | 2,188 | 156/228 | 167/387 | 780/1573 | yes: tick phases |
| MonoBehaviour | 839 | 935 | 32/53 | 107/144 | 700/738 | yes: host base |
| GameObject | 801 | 5,786 | 61/199 | 165/1798 | 575/3789 | yes (B): entity handle |
| Vector2/3/4 | 762 | 7,268 | 153/1124 | 148/2478 | 461/3666 | yes: float vector maths |
| Transform | 697 | 5,782 | 36/144 | 164/2334 | 497/3304 | yes (B): transform hierarchy |
| GetComponent | 683 | 2,296 | 39/55 | 134/623 | 510/1618 | yes (B): typed component lookup |
| SerializeField | 668 | 2,830 | 201/714 | 90/417 | 377/1699 | no |
| Debug. | 604 | 2,470 | 203/839 | 139/778 | 262/853 | yes: logging |
| UnityEngine.UI | 543 | 2,906 | 13/18 | 18/81 | 512/2807 | no (UMG) |
| Instantiate/Destroy | 504 | 1,377 | 35/45 | 103/367 | 366/965 | yes (B): spawn/destroy |
| Mathf | 434 | 2,386 | 101/578 | 81/663 | 252/1145 | yes: Mathf |
| SetActive | 379 | 2,203 | 15/61 | 65/354 | 299/1788 | yes (B): active flag |
| Color | 349 | 3,832 | 45/344 | 55/240 | 249/3248 | yes: colour data |
| TMPro | 329 | 1,058 | 2/3 | 6/16 | 321/1039 | no (UMG) |
| Quaternion | 277 | 1,321 | 66/214 | 84/641 | 127/466 | yes: float quaternion |
| Coroutine | 261 | 1,293 | 41/198 | 72/446 | 148/649 | yes: scheduler |
| Time. | 236 | 728 | 20/44 | 51/222 | 165/462 | yes: clock |
| Texture | 218 | 882 | 17/72 | 14/98 | 187/712 | minimal: pixel arrays for map sampling |
| EventSystems | 177 | 840 | 0/0 | 8/32 | 169/808 | no (UMG) |
| Material | 175 | 790 | 0/0 | 32/217 | 143/573 | no (shell) |
| Renderer | 141 | 480 | 2/6 | 42/180 | 97/294 | no (shell); bounds query for a few B rows |
| Shader | 130 | 722 | 2/3 | 21/113 | 107/606 | no (shell) |
| Rect/Bounds/Ray/Plane | 127 | 412 | 17/46 | 34/144 | 76/222 | yes: geometry value types |
| Input. | 120 | 517 | 2/5 | 11/97 | 107/415 | no (Enhanced Input -> FlightCtrlState) |
| Camera | 115 | 319 | 1/6 | 19/57 | 95/256 | minimal: camera position for LOD/occlusion |
| Random | 114 | 402 | 35/116 | 19/86 | 60/200 | yes: bit-exact Unity Random |
| Collider | 81 | 355 | 0/0 | 53/265 | 28/90 | yes (B): collision shapes |
| LayerMask | 78 | 192 | 1/2 | 31/107 | 46/83 | yes: layer constants |
| KeyCode | 70 | 458 | 1/3 | 8/55 | 61/400 | no |
| Raycast/Query | 62 | 206 | 6/8 | 32/132 | 24/66 | yes (B): scene queries |
| Rigidbody | 59 | 489 | 3/7 | 38/422 | 18/60 | yes (B): rigid-body facade |
| Screen/Cursor | 57 | 152 | 1/2 | 3/10 | 53/140 | no |
| Audio | 55 | 264 | 0/0 | 12/71 | 43/193 | no |
| Physics. | 51 | 122 | 0/0 | 36/100 | 15/22 | yes (B): physics world API |
| Animation | 50 | 238 | 1/2 | 18/107 | 31/129 | yes (B): animation sampler |
| Resources. | 49 | 114 | 15/23 | 13/44 | 21/47 | no (Asset Manager); data-path for configs |
| Mesh | 47 | 205 | 0/0 | 19/68 | 28/137 | yes (B): mesh buffers for colliders/cubes |
| Application. | 46 | 131 | 5/11 | 8/18 | 33/102 | minimal: paths |
| ParticleSystem | 43 | 355 | 1/1 | 10/58 | 32/296 | no (Niagara) |
| Analytics | 43 | 209 | 12/34 | 12/73 | 19/102 | no (drop) |
| IMGUI | 38 | 1,272 | 3/4 | 5/7 | 30/1261 | no |
| Light | 37 | 148 | 1/1 | 8/20 | 28/127 | no |
| AnimationCurve | 37 | 333 | 14/177 | 2/16 | 21/140 | yes: curve evaluator |
| SendMessage/Invoke | 31 | 67 | 6/14 | 11/28 | 14/25 | yes: timers/typed events |
| SceneManager | 28 | 101 | 0/0 | 6/37 | 22/64 | yes (B): scene state |
| Collision/Force | 27 | 122 | 0/0 | 20/102 | 7/20 | yes (B): contacts, ForceMode |
| Matrix4x4 | 22 | 97 | 5/43 | 5/21 | 12/33 | yes: matrix maths |
| Joint | 20 | 160 | 0/0 | 18/154 | 2/6 | yes (B): joint facade |
| WWW/WebRequest | 19 | 43 | 0/0 | 2/4 | 17/39 | no |
| QualitySettings/SystemInfo | 17 | 94 | 0/0 | 1/1 | 16/93 | no |
| Steam | 17 | 226 | 2/4 | 2/36 | 13/186 | no (drop) |
| WheelCollider | 11 | 27 | 1/1 | 8/20 | 2/6 | yes (B): suspension model |
| ScriptableObject | 10 | 13 | 4/5 | 1/1 | 5/7 | minimal: data assets -> plain structs |
| PhysicMaterial | 10 | 64 | 2/16 | 7/36 | 1/12 | yes (B): friction materials |
| AssetBundle | 5 | 23 | 1/1 | 1/3 | 3/19 | no |
| Graphics/GL | 3 | 7 | 0/0 | 0/0 | 3/7 | no |
| PlayerPrefs | 3 | 36 | 0/0 | 0/0 | 3/36 | no (settings store) |

## 2. MonoBehaviour messages (lifecycle) -> core tick phases

Counts are method definitions with the Unity message name (`Reset` is inflated by KSP methods named Reset).

| Message | Definitions | Core equivalent |
|---|---:|---|
| OnDestroy | 527 | teardown / unregister |
| Awake | 522 | construct + `OnAwake` (registration) |
| Start | 469 | first-tick init (`OnStart` ordering preserved) |
| Update | 367 | variable-rate frame tick (shell-driven; core only for UI-facing state) |
| Reset | 154 | n/a |
| LateUpdate | 128 | post-frame tick (camera/visual sync; mostly shell) |
| FixedUpdate | 115 | fixed physics tick (core main loop, KSP order Timing0..5 / TimingPre / TimingFI) |
| OnEnable | 47 | activate |
| OnDisable | 36 | deactivate |
| OnMouseExit | 15 | shell picking |
| OnCollisionEnter | 13 | physics contact callback (enter) |
| OnMouseDown | 13 | shell picking -> core action |
| OnMouseUp | 13 | shell picking |
| OnMouseEnter | 13 | shell picking |
| OnGUI | 10 | drop (IMGUI) or UMG |
| OnMouseOver | 10 | shell picking |
| OnMouseDrag | 10 | shell picking |
| OnCollisionExit | 8 | physics contact callback (exit) |
| OnApplicationFocus | 7 | shell |
| OnCollisionStay | 7 | physics contact callback (stay) |
| OnDrawGizmos | 7 | drop |
| OnDrawGizmosSelected | 7 | drop |
| OnTriggerExit | 6 | trigger volume callback |
| OnRectTransformDimensionsChange | 6 | UMG |
| OnTriggerEnter | 5 | trigger volume callback |
| OnPreRender | 3 | shell |
| OnJointBreak | 3 | joint break callback (core physics) |
| OnTriggerStay | 2 | trigger volume callback |
| OnRenderObject | 2 | shell |
| OnRenderImage | 2 | shell post-process |
| OnValidate | 1 | drop |
| OnBecameVisible | 1 | shell |
| OnBecameInvisible | 1 | shell |
| OnPreCull | 1 | shell |
| OnPostRender | 1 | shell |

## 3. Unity types ranked by number of KSP files using them (top 120)

| # | Type | Module | Files | Refs | A | B | C | C++ core shim / UE5 equivalent |
|---:|---|---|---:|---:|---:|---:|---:|---|
| 1 | `MonoBehaviour` | CoreModule | 839 | 935 | 32 | 107 | 700 | core: component/host base with tick phases (WP-00); C rows: UObject/UActorComponent or UMG widget |
| 2 | `Vector3` | CoreModule | 618 | 5,581 | 133 | 140 | 345 | core: `ksp::Vector3f` with Unity float semantics; shell: FVector3f/FVector at boundary |
| 3 | `Debug` | CoreModule | 617 | 2,488 | 203 | 139 | 275 | core: `ksp::log` (Log/Warning/Error/Exception); DrawLine/DrawRay -> debug draw hook |
| 4 | `Object` | CoreModule | 513 | 1,378 | 37 | 103 | 373 | core: object lifetime (Instantiate/Destroy) via entity registry; shell: SpawnActor/DestroyActor |
| 5 | `Mathf` | CoreModule | 433 | 2,372 | 101 | 81 | 251 | core: `ksp::Mathf` (float, Round = banker's, Sign(0)=1, Approximately, Repeat/PingPong, SmoothDamp) |
| 6 | `SerializeField` | CoreModule | 407 | 2,415 | 73 | 73 | 261 | drop (attribute); persistence comes from KSPField/ConfigNode |
| 7 | `GameObject` | CoreModule | 405 | 1,421 | 18 | 84 | 303 | core: entity handle + name/active flag/layer/tag; shell: AActor |
| 8 | `Transform` | CoreModule | 374 | 1,089 | 17 | 117 | 240 | core: double-precision transform hierarchy (WP-00); shell: USceneComponent (camera-relative) |
| 9 | `Color` | CoreModule | 342 | 3,695 | 44 | 55 | 243 | core: `ksp::Color` (float RGBA) for data; shell: FLinearColor |
| 10 | `Vector2` | CoreModule | 292 | 1,538 | 32 | 42 | 218 | core: `ksp::Vector2f`; shell: FVector2D |
| 11 | `Quaternion` | CoreModule | 277 | 1,321 | 66 | 84 | 127 | core: `ksp::Quaternionf` replicating Unity natives (AngleAxis, LookRotation, FromToRotation, Euler ZXY order, Slerp) |
| 12 | `Time` | CoreModule | 236 | 728 | 20 | 51 | 165 | core: `ksp::Clock` (deltaTime, fixedDeltaTime, time, timeScale, realtimeSinceStartup, frameCount) |
| 13 | `RectTransform` | CoreModule | 224 | 644 | 1 | 6 | 217 | UMG slot layout (anchors/offsets) - C only |
| 14 | `Button` | UI | 213 | 650 | 0 | 4 | 209 | UMG UButton / CommonUI button |
| 15 | `PointerEventData` | UI | 150 | 526 | 0 | 5 | 145 | UMG/Slate pointer events |
| 16 | `Random` | CoreModule | 121 | 420 | 40 | 21 | 60 | core: bit-exact Unity Random (xorshift128 + InitState seeding + Range mapping) - needed for seeded contracts/asteroids/ROC |
| 17 | `Image` | UI | 114 | 285 | 0 | 1 | 113 | UMG UImage |
| 18 | `Camera` | CoreModule | 113 | 288 | 1 | 19 | 93 | shell: UCameraComponent / SceneCapture; core needs only camera position for LOD/occlusion queries |
| 19 | `Texture2D` | CoreModule | 112 | 445 | 13 | 12 | 87 | shell: UTexture2D; core: map sampling via plain pixel arrays (MapSO/CBAttributeMap) |
| 20 | `Material` | CoreModule | 100 | 354 | 0 | 18 | 82 | shell: UMaterialInstanceDynamic |
| 21 | `AddComponentMenu` | CoreModule | 93 | 93 | 48 | 12 | 33 | drop (editor attribute) |
| 22 | `Toggle` | UI | 81 | 239 | 0 | 0 | 81 | UMG UCheckBox |
| 23 | `RequireComponent` | CoreModule | 79 | 86 | 1 | 0 | 78 | drop (editor attribute) |
| 24 | `Rect` | CoreModule | 76 | 204 | 10 | 10 | 56 | core: `ksp::Rect` (data); UI: FSlateRect |
| 25 | `ContextMenu` | CoreModule | 76 | 133 | 8 | 31 | 37 | drop (editor attribute) |
| 26 | `KeyCode` | CoreModule | 70 | 458 | 1 | 8 | 61 | Enhanced Input keys (FKey) via binding table |
| 27 | `IEventSystemHandler` | UI | 64 | 64 | 0 | 1 | 63 | UMG/Slate events |
| 28 | `HideInInspector` | CoreModule | 61 | 207 | 10 | 20 | 31 | drop (editor attribute) |
| 29 | `Sprite` | CoreModule | 61 | 172 | 2 | 1 | 58 | UMG brush / Paper2D sprite |
| 30 | `Slider` | UI | 58 | 117 | 0 | 0 | 58 | UMG USlider |
| 31 | `Physics` | PhysicsModule | 57 | 129 | 0 | 36 | 21 | core: physics query API (Raycast, RaycastAll, SphereCast, OverlapSphere, Linecast, IgnoreCollision, gravity) |
| 32 | `Collider` | PhysicsModule | 56 | 242 | 0 | 41 | 15 | core: collision shape handle in physics shim (box/sphere/capsule/mesh/convex) |
| 33 | `MeshRenderer` | CoreModule | 56 | 137 | 1 | 23 | 32 | shell: UStaticMeshComponent / UProceduralMeshComponent |
| 34 | `Shader` | CoreModule | 54 | 243 | 1 | 13 | 40 | shell: material parameter names (FName) / global shader params via Material Parameter Collection |
| 35 | `Rigidbody` | PhysicsModule | 54 | 203 | 2 | 34 | 18 | core: rigid-body facade over the chosen solver (Chaos/Jolt/custom): pose, velocity, mass, CoM, inertia, forces |
| 36 | `Screen` | CoreModule | 53 | 155 | 1 | 2 | 50 | shell: viewport size (GEngine->GameViewport) |
| 37 | `RawImage` | UI | 49 | 69 | 0 | 0 | 49 | UMG UImage (render target brush) |
| 38 | `WaitForSeconds` | CoreModule | 48 | 75 | 10 | 14 | 24 | core: timer awaitable in the coroutine scheduler (WP-00) |
| 39 | `Application` | CoreModule | 46 | 131 | 5 | 8 | 33 | shell: FPlatformMisc / paths; core: data-path config |
| 40 | `RaycastHit` | PhysicsModule | 44 | 108 | 1 | 25 | 18 | core: hit record (point, normal, distance, collider, rigidbody, triangle index) |
| 41 | `AudioSource` | AudioModule | 42 | 139 | 0 | 10 | 32 | UAudioComponent / MetaSounds |
| 42 | `LayoutElement` | UI | 41 | 83 | 0 | 1 | 40 | UMG size box / slot |
| 43 | `LayerMask` | CoreModule | 41 | 77 | 0 | 14 | 27 | core: layer bitmask constants table (KSP layer numbers are gameplay-relevant for raycast filtering) |
| 44 | `Selectable` | UI | 40 | 138 | 0 | 0 | 40 | UMG widget focus/navigation |
| 45 | `Texture` | CoreModule | 40 | 98 | 2 | 4 | 34 | shell: UTexture |
| 46 | `Vector4` | CoreModule | 39 | 142 | 11 | 4 | 24 | core: `ksp::Vector4f` |
| 47 | `Mesh` | CoreModule | 38 | 125 | 0 | 17 | 21 | core (B rows only): mesh buffers for colliders/drag cubes; shell: UProceduralMeshComponent / RealtimeMesh |
| 48 | `Animation` | AnimationModule | 38 | 124 | 0 | 17 | 21 | core: animation sampler (clip curves sampled at normalized time) for gameplay-driving animations; shell: skeletal/rigid anim |
| 49 | `ScrollRect` | UI | 38 | 53 | 0 | 1 | 37 | UMG UScrollBox |
| 50 | `Coroutine` | CoreModule | 37 | 40 | 3 | 5 | 29 | core: coroutine scheduler (C++20 coroutines or explicit state machines) |
| 51 | `IPointerExitHandler` | UI | 37 | 40 | 0 | 0 | 37 | UMG OnMouseLeave |
| 52 | `IPointerEnterHandler` | UI | 36 | 39 | 0 | 0 | 36 | UMG OnMouseEnter |
| 53 | `ParticleSystem` | ParticleSystemModule | 34 | 300 | 0 | 8 | 26 | Niagara |
| 54 | `Space` | CoreModule | 34 | 211 | 3 | 12 | 19 | core: enum (Self/World) for transform ops |
| 55 | `TextAnchor` | TextRenderingModule | 34 | 139 | 1 | 0 | 33 | UMG text justification |
| 56 | `Canvas` | UIModule | 32 | 88 | 0 | 2 | 30 | UMG root widget / Widget Component |
| 57 | `RectOffset` | CoreModule | 30 | 119 | 1 | 0 | 29 | UMG margin |
| 58 | `AnimationCurve` | CoreModule | 30 | 108 | 11 | 1 | 18 | core: `ksp::AnimationCurve` evaluator (Unity Hermite + wrap modes) - native in Unity, needs golden |
| 59 | `Light` | CoreModule | 30 | 102 | 0 | 7 | 23 | shell: ULightComponent (point/spot/directional) |
| 60 | `MeshFilter` | CoreModule | 30 | 72 | 0 | 9 | 21 | shell: mesh component |
| 61 | `AudioClip` | AudioModule | 29 | 105 | 0 | 6 | 23 | USoundWave |
| 62 | `EventSystem` | UI | 29 | 92 | 0 | 6 | 23 | Slate focus/navigation |
| 63 | `Display` | CoreModule | 29 | 75 | 2 | 1 | 26 | shell: multi-display info |
| 64 | `ToggleGroup` | UI | 29 | 36 | 0 | 1 | 28 | UMG radio group (CommonUI) |
| 65 | `Bounds` | CoreModule | 28 | 115 | 2 | 15 | 11 | core: `ksp::Bounds` (AABB maths) |
| 66 | `SceneManager` | CoreModule | 28 | 66 | 0 | 6 | 22 | core: scene state machine (HighLogic); shell: level streaming / OpenLevel |
| 67 | `GUI` | IMGUIModule | 27 | 113 | 2 | 3 | 22 | drop or UMG (IMGUI debug windows) |
| 68 | `CanvasGroup` | UIModule | 27 | 61 | 0 | 0 | 27 | UMG render opacity / visibility |
| 69 | `IPointerClickHandler` | UI | 25 | 28 | 0 | 1 | 24 | UMG OnClicked |
| 70 | `Color32` | CoreModule | 24 | 137 | 4 | 2 | 18 | core: `ksp::Color32`; shell: FColor |
| 71 | `RenderTexture` | CoreModule | 24 | 65 | 0 | 4 | 20 | UTextureRenderTarget2D |
| 72 | `LoadSceneMode` | CoreModule | 24 | 35 | 0 | 5 | 19 | level streaming mode |
| 73 | `Matrix4x4` | CoreModule | 22 | 97 | 5 | 5 | 12 | core: `ksp::Matrix4x4f` (TRS, inverse, MultiplyPoint3x4) |
| 74 | `Navigation` | UI | 21 | 124 | 0 | 2 | 19 | UMG navigation rules |
| 75 | `Scene` | CoreModule | 21 | 21 | 0 | 4 | 17 | level handle |
| 76 | `BaseEventData` | UI | 20 | 47 | 0 | 0 | 20 | Slate event |
| 77 | `CameraClearFlags` | CoreModule | 20 | 29 | 0 | 4 | 16 | camera setting (shell) |
| 78 | `MaterialPropertyBlock` | CoreModule | 19 | 30 | 0 | 3 | 16 | per-instance material params (Custom Primitive Data) |
| 79 | `GUILayout` | IMGUIModule | 18 | 1,072 | 0 | 0 | 18 | drop or UMG (IMGUI debug windows) |
| 80 | `MeshCollider` | PhysicsModule | 18 | 47 | 0 | 11 | 7 | core: triangle-mesh/convex collider in physics shim |
| 81 | `Keyframe` | CoreModule | 17 | 225 | 8 | 2 | 7 | core: `ksp::Keyframe` (time, value, inTangent, outTangent, weights) |
| 82 | `Text` | UI | 17 | 50 | 0 | 0 | 17 | UMG UTextBlock |
| 83 | `UnityAction` | CoreModule | 17 | 36 | 0 | 0 | 17 | std::function / delegate |
| 84 | `UnityEvent` | CoreModule | 17 | 27 | 0 | 0 | 17 | multicast delegate |
| 85 | `ExecuteInEditMode` | CoreModule | 17 | 17 | 0 | 0 | 17 | drop (editor attribute) |
| 86 | `Gizmos` | CoreModule | 16 | 65 | 2 | 10 | 4 | debug draw (drop) |
| 87 | `RectTransformUtility` | UIModule | 16 | 31 | 0 | 1 | 15 | Slate geometry helpers |
| 88 | `WaitForEndOfFrame` | CoreModule | 16 | 22 | 2 | 6 | 8 | core: end-of-frame awaitable |
| 89 | `ContentSizeFitter` | UI | 15 | 50 | 1 | 0 | 14 | UMG auto-size |
| 90 | `AnimationState` | AnimationModule | 15 | 30 | 0 | 6 | 9 | core: animation channel state (time, speed, weight, wrapMode, enabled) |
| 91 | `ColorUtility` | CoreModule | 15 | 26 | 3 | 0 | 12 | core: hex colour parse/format |
| 92 | `VerticalLayoutGroup` | UI | 15 | 22 | 0 | 1 | 14 | UMG UVerticalBox |
| 93 | `RenderTextureFormat` | CoreModule | 15 | 21 | 0 | 3 | 12 | render target format (shell) |
| 94 | `UnityWebRequest` | UnityWebRequestModule | 15 | 20 | 0 | 2 | 13 | drop (or FHttpModule) |
| 95 | `EventTrigger` | UI | 14 | 83 | 0 | 0 | 14 | UMG events |
| 96 | `Component` | CoreModule | 14 | 70 | 1 | 6 | 7 | core: component base |
| 97 | `Collision` | PhysicsModule | 14 | 34 | 0 | 10 | 4 | core: contact report (impulse, relative velocity, contact points, other collider) |
| 98 | `ConfigurableJoint` | PhysicsModule | 14 | 32 | 0 | 13 | 1 | core: 6-DoF joint facade (linear/angular motions, limits, drives, target rotation, break force/torque) |
| 99 | `IDragHandler` | UI | 14 | 14 | 0 | 0 | 14 | UMG drag-drop operation |
| 100 | `SystemInfo` | CoreModule | 13 | 39 | 0 | 1 | 12 | shell: platform info (drop for core) |
| 101 | `RuntimePlatform` | CoreModule | 13 | 36 | 2 | 1 | 10 | shell: platform enum |
| 102 | `SkinnedMeshRenderer` | CoreModule | 13 | 31 | 1 | 8 | 4 | shell: USkeletalMeshComponent (kerbals) |
| 103 | `EventTriggerType` | UI | 12 | 36 | 0 | 0 | 12 | UMG events |
| 104 | `ShadowCastingMode` | CoreModule | 12 | 20 | 0 | 6 | 6 | shell: shadow flags |
| 105 | `QueueMode` | AnimationModule | 11 | 28 | 0 | 1 | 10 | animation queue mode (core animation sampler if gameplay) |
| 106 | `Scrollbar` | UI | 11 | 24 | 0 | 0 | 11 | UMG scroll bar |
| 107 | `Event` | IMGUIModule | 10 | 35 | 0 | 1 | 9 | Slate input event (IMGUI) |
| 108 | `SphereCollider` | PhysicsModule | 10 | 26 | 0 | 5 | 5 | core: sphere collider |
| 109 | `SendMessageOptions` | CoreModule | 10 | 23 | 0 | 3 | 7 | drop (typed events instead) |
| 110 | `QueryTriggerInteraction` | PhysicsModule | 10 | 19 | 0 | 8 | 2 | core: raycast trigger filter flag |
| 111 | `GUISkin` | IMGUIModule | 10 | 15 | 0 | 3 | 7 |  |
| 112 | `ScriptableObject` | CoreModule | 10 | 13 | 4 | 1 | 5 |  |
| 113 | `ConfigurableJointMotion` | PhysicsModule | 9 | 45 | 0 | 9 | 0 | core: motion enum (Locked/Limited/Free) |
| 114 | `Animator` | AnimationModule | 9 | 33 | 0 | 2 | 7 |  |
| 115 | `ForceMode` | PhysicsModule | 9 | 29 | 0 | 7 | 2 | core: force mode enum (Force/Impulse/Acceleration/VelocityChange) |
| 116 | `PhysicMaterial` | PhysicsModule | 9 | 26 | 2 | 7 | 0 | core: friction/bounciness material with combine modes |
| 117 | `ParticleSystemRenderer` | ParticleSystemModule | 9 | 18 | 1 | 3 | 5 |  |
| 118 | `SpriteState` | UI | 9 | 16 | 0 | 0 | 9 |  |
| 119 | `WaitForFixedUpdate` | CoreModule | 9 | 14 | 2 | 5 | 2 |  |
| 120 | `DownloadHandlerTexture` | UnityWebRequestTextureModule | 9 | 11 | 0 | 1 | 8 |  |

Types used in total: 265. Remaining types (each used by few files): `UnityWebRequestTexture` (9), `IBeginDragHandler` (9), `BoxCollider` (8), `WheelCollider` (8), `Gradient` (8), `JointDrive` (8), `IEndDragHandler` (8), `Joint` (7), `ParticleSystemCurveMode` (7), `HideFlags` (7), `SoftJointLimit` (7), `LayoutRebuilder` (7), `Cursor` (7), `IPointerDownHandler` (7), `AudioRolloffMode` (7), `QualitySettings` (6), `PhysicMaterialCombine` (6), `GUIStyle` (6), `GridLayoutGroup` (6), `ParticleSystemSimulationSpace` (6), `PrimitiveType` (6), `Graphic` (6), `WaitForSecondsRealtime` (6), `LayoutGroup` (6), `SoftJointLimitSpring` (6), `FilterMode` (6), `AssetBundle` (5), `AudioListener` (5), `ParticleSystemRenderMode` (5), `Cubemap` (5), `CursorLockMode` (5), `LightType` (5), `HorizontalLayoutGroup` (5), `IPointerUpHandler` (5), `GradientColorKey` (4), `LineRenderer` (4), `GradientAlphaKey` (4), `Font` (4), `RigidbodyConstraints` (4), `WWW` (4), `AnimationClip` (4), `RenderTextureReadWrite` (4), `Security` (4), `PlayerPrefs` (3), `RenderSettings` (3), `ExecuteEvents` (3), `FontStyle` (3), `CapsuleCollider` (3), `FixedJoint` (3), `UIBehaviour` (3), `GUILayoutUtility` (3), `DepthTextureMode` (3), `Graphics` (3), `RenderMode` (3), `ThreadPriority` (3), `ContactPoint` (3), `WaitUntil` (3), `ReflectionProbeUsage` (3), `Projector` (3), `IScrollHandler` (3), `ISubmitHandler` (3), `PlayMode` (2), `GUIContent` (2), `Profiler` (2), `HorizontalOrVerticalLayoutGroup` (2), `Resolution` (2), `GUILayoutOption` (2), `WheelFrictionCurve` (2), `ColorBlock` (2), `TextAsset` (2), `CanvasRenderer` (2), `LayoutUtility` (2), `RectMask2D` (2), `AssetBundleRequest` (2), `InputField` (2), `AxisEventData` (2), `CanvasUpdate` (2), `DefaultReflectionMode` (2), `RotationDriveMode` (2), `Analytics` (2), `MeshTopology` (2), `AnimationBlendMode` (2), `UnityWebRequestAssetBundle` (2), `DownloadHandlerAssetBundle` (2), `TextureWrapMode` (2), `LensFlare` (2), `ISelectHandler` (2), `DownloadHandlerBuffer` (2), `SpriteRenderer` (2), `ILayoutController` (2), `ILayoutSelfController` (2), `IInitializePotentialDragHandler` (2), `IDropHandler` (2), `GraphicRaycaster` (2), `LogType` (1), `GUIUtility` (1), `ShadowProjection` (1), `Flare` (1), `CharacterJoint` (1), `TextMesh` (1), `GL` (1), `ReflectionProbeTimeSlicingMode` (1), `Texture2DArray` (1), `FocusType` (1), `Terrain` (1), `BoneWeight` (1), `CubemapFace` (1), `LightShadows` (1), `ComputeBuffer` (1), `ReflectionProbe` (1), `AspectRatioFitter` (1), `AnimationEvent` (1), `CanvasScaler` (1), `CombineInstance` (1), `WWWForm` (1), `Texture3D` (1), `UnityWebRequestMultimedia` (1), `AudioType` (1), `DownloadHandlerAudioClip` (1), `JointProjectionMode` (1), `ReflectionProbeMode` (1), `ReflectionProbeRefreshMode` (1), `ReflectionProbeClearFlags` (1), `AsyncOperation` (1), `JointSpring` (1), `SkinQuality` (1), `AnimatorCullingMode` (1), `ScreenCapture` (1), `StandaloneInputModule` (1), `DisallowMultipleComponent` (1), `TouchScreenKeyboardType` (1), `TextClipping` (1), `PropertyAttribute` (1), `FogMode` (1), `WheelHit` (1), `UIVertex` (1), `IDeselectHandler` (1), `IMoveHandler` (1), `CursorMode` (1), `ICanvasRaycastFilter` (1), `RaycastResult` (1), `GraphicsDeviceType` (1), `UnityException` (1), `ICanvasElement` (1), `ExecuteAlways` (1).

## 4. Static API members (exact counts)

| Host | Total | Members (count) |
|---|---:|---|
| `Debug` | 2,389 | Log (812), LogError (776), LogWarning (361), LogFormat (214), LogErrorFormat (66), LogWarningFormat (48), DrawRay (43), DrawLine (33), LogException (19), Break (15), DebugBreak (2) |
| `Mathf` | 2,386 | Clamp (347), Abs (314), Lerp (259), Max (257), Round (168), Clamp01 (162), Min (161), RoundToInt (129), Pow (83), InverseLerp (71), FloorToInt (57), Sign (44), Sin (38), Approximately (33), Log (31), Sqrt (28) |
| `Vector3` | 1,943 | zero (683), Dot (222), up (194), one (186), right (113), forward (99), Distance (74), Cross (69), Lerp (61), ProjectOnPlane (53), back (45), Scale (31), down (24), left (22), SqrMagnitude (11), Angle (10) |
| `GUILayout` | 1,072 | Label (214), BeginHorizontal (144), EndHorizontal (144), Width (122), Button (115), Space (69), Toggle (41), ExpandWidth (33), BeginScrollView (28), EndScrollView (28), FlexibleSpace (27), HorizontalSlider (19), TextField (17), ExpandHeight (12), Window (8), Toolbar (8) |
| `Quaternion` | 845 | identity (223), AngleAxis (213), LookRotation (98), Euler (92), Inverse (79), FromToRotation (69), Lerp (30), Angle (20), RotateTowards (14), Slerp (7) |
| `Time` | 728 | deltaTime (295), realtimeSinceStartup (188), time (83), frameCount (43), fixedDeltaTime (37), timeSinceLevelLoad (22), unscaledDeltaTime (18), unscaledTime (17), timeScale (16), maximumDeltaTime (6), fixedTime (2), fixedUnscaledDeltaTime (1) |
| `Object` | 567 | Destroy (275), Instantiate (201), DestroyImmediate (58), DontDestroyOnLoad (17), FindObjectOfType (10), FindObjectsOfType (6) |
| `Shader` | 194 | Find (90), PropertyToID (58), SetGlobalFloat (29), SetGlobalVector (9), SetGlobalColor (3), SetGlobalMatrix (3), SetGlobalTexture (2) |
| `Screen` | 142 | height (73), width (61), SetResolution (4), resolutions (3), fullScreen (1) |
| `Application` | 131 | platform (40), isPlaying (33), dataPath (28), isEditor (12), LogCallback (5), Quit (4), runInBackground (2), backgroundLoadingPriority (2), targetFrameRate (2), isFocused (2), logMessageReceivedThreaded (1) |
| `Random` | 126 | Range (92), value (22), InitState (6), insideUnitSphere (5), onUnitSphere (1) |
| `Physics` | 120 | Raycast (59), IgnoreCollision (39), RaycastAll (5), OverlapSphere (5), gravity (4), Linecast (1), defaultContactOffset (1), CapsuleCastAll (1), SphereCastAll (1), SphereCast (1), SyncTransforms (1), RaycastNonAlloc (1), SphereCastNonAlloc (1) |
| `GUI` | 100 | enabled (23), Label (16), Button (12), DragWindow (11), skin (10), Box (10), TextField (7), Window (3), depth (2), BringWindowToFront (2), changed (1), DrawTexture (1), FocusWindow (1), HorizontalSlider (1) |
| `SceneManager` | 66 | sceneLoaded (41), LoadScene (10), LoadSceneAsync (5), UnloadSceneAsync (4), GetActiveScene (4), MoveGameObjectToScene (2) |
| `QualitySettings` | 55 | shadowCascades (15), shadowCascade4Split (9), shadowDistance (5), vSyncCount (5), GetQualityLevel (4), shadowCascade2Split (3), SetQualityLevel (3), antiAliasing (3), masterTextureLimit (3), pixelLightCount (3), shadowProjection (1), names (1) |
| `Gizmos` | 65 | color (24), DrawLine (17), DrawWireSphere (9), DrawWireCube (5), DrawRay (4), DrawSphere (4), DrawCube (2) |
| `LayerMask` | 56 | NameToLayer (52), GetMask (3), LayerToName (1) |
| `Camera` | 47 | main (43), current (2), ScreenPointToRay (1), WorldToScreenPoint (1) |
| `SystemInfo` | 39 | SupportsRenderTextureFormat (6), graphicsShaderLevel (4), supportsImageEffects (4), maxCubemapSize (4), graphicsDeviceVersion (3), operatingSystem (2), processorCount (2), processorType (2), systemMemorySize (2), graphicsDeviceName (2), graphicsDeviceType (2), graphicsMemorySize (2), processorFrequency (1), graphicsDeviceVendor (1), maxTextureSize (1), supportsRenderTextures (1) |
| `GameObject` | 42 | Find (31), CreatePrimitive (9), FindGameObjectWithTag (2) |
| `PlayerPrefs` | 36 | GetInt (10), SetInt (8), GetFloat (6), SetFloat (6), SetString (3), GetString (3) |
| `RectTransformUtility` | 34 | RectangleContainsScreenPoint (12), ScreenPointToLocalPointInRectangle (11), WorldToScreenPoint (5), ScreenPointToWorldPointInRectangle (5), CalculateRelativeRectTransformBounds (1) |
| `RenderSettings` | 33 | ambientLight (7), customReflection (7), ambientEquatorColor (5), ambientGroundColor (5), ambientSkyColor (5), defaultReflectionMode (4) |
| `Cursor` | 10 | lockState (7), visible (2), SetCursor (1) |
| `Graphics` | 4 | Blit (3), DrawTexture (1) |
| `GL` | 3 | PushMatrix (1), MultMatrix (1), PopMatrix (1) |
| `AssetBundle` | 2 | LoadFromMemory (2) |
| `Input` (from regex scan) | 517 | GetKey (167), GetKeyDown (101), mousePosition (96), GetAxis (60), GetKeyUp (51), GetMouseButton (46), GetMouseButtonUp (34), GetMouseButtonDown (32), compositionString (11) |

## 5. Instance member hotspots (approximate)

| Type | Most used members (count) |
|---|---|
| `Transform` | position (1269), rotation (448), parent (424), localPosition (420), localRotation (306), up (301), forward (282), SetParent (276), localScale (210), Find (163), right (122), GetChild (115), childCount (102), TransformPoint (95), eulerAngles (88), Current (69), InverseTransformPoint (63), InverseTransformDirection (57) |
| `GameObject` | transform (2727), gameObject (1932), SetActive (1050), GetComponent (721), AddComponent (333), Find (161), active (118), GetComponentsInChildren (114), activeSelf (95), GetComponentInChildren (89), layer (89), activeInHierarchy (52), GetComponentInParent (36), CompareTag (33), tag (25), GetComponents (24), CreatePrimitive (9), SendMessage (6) |
| `Rigidbody` | position (518), rotation (190), velocity (133), mass (87), angularVelocity (56), AddForce (29), isKinematic (27), drag (22), useGravity (19), angularDrag (17), AddForceAtPosition (15), worldCenterOfMass (15), maxAngularVelocity (10), centerOfMass (8), inertiaTensor (7), MovePosition (6), constraints (6), GetPointVelocity (5) |
| `ConfigurableJoint` | angularXDrive (20), secondaryAxis (19), targetAngularVelocity (14), xDrive (13), angularXMotion (11), lowAngularXLimit (8), highAngularXLimit (8), targetRotation (7), angularYMotion (7), angularZMotion (7), xMotion (7), yMotion (7), zMotion (7), angularXLimitSpring (7), angularYZDrive (5), configuredInWorldSpace (5), linearLimit (5), yDrive (3) |
| `Joint` | connectedBody (9), axis (8), breakForce (8), breakTorque (6), anchor (2), currentForce (2), autoConfigureConnectedAnchor (1), connectedAnchor (1), enablePreprocessing (1), connectedMassScale (1) |
| `Collider` | enabled (133), material (56), bounds (39), Raycast (34), attachedRigidbody (23), sharedMaterial (20), isTrigger (17), ClosestPointOnBounds (4), ClosestPoint (3) |
| `WheelCollider` | mass (27), center (20), radius (17), suspensionDistance (6), suspensionSpring (1), forwardFriction (1), sidewaysFriction (1), rpm (1) |
| `Animation` | Play (64), Blend (56), CrossFade (43), Stop (29), wrapMode (20), clip (16), isPlaying (14), Sample (11), GetClip (9), IsPlaying (8), CrossFadeQueued (7), AddClip (5), localBounds (3), Current (3), playAutomatically (2), GetEnumerator (2), MoveNext (2), GetClipCount (1) |
| `AnimationState` | normalizedTime (82), name (71), enabled (50), speed (49), length (36), weight (30), time (26), layer (15), clip (14), wrapMode (12), normalizedSpeed (9), AddMixingTransform (3), RemoveMixingTransform (2), blendMode (2) |
| `Renderer` | material (186), enabled (153), bounds (33), sharedMaterial (29), materials (24), SetPropertyBlock (12), sharedMaterials (9), shadowCastingMode (6), receiveShadows (4), castShadows (2), localToWorldMatrix (1), GetPropertyBlock (1), isVisible (1), reflectionProbeUsage (1), sortingLayerName (1), sortingOrder (1) |
| `Material` | color (124), Lerp (80), SetColor (67), SetTexture (53), SetFloat (48), shader (47), GetTexture (36), mainTexture (31), Create (28), HasProperty (14), mainTextureScale (13), SetTextureScale (8), mainTextureOffset (6), SetVector (6), CopyPropertiesFromMaterial (4), SetTextureOffset (4), EnableKeyword (3), GetTextureOffset (2) |
| `Mesh` | vertices (68), triangles (53), normals (48), uv (32), RecalculateBounds (22), Clear (21), tangents (15), uv2 (10), RecalculateNormals (8), colors (8), bounds (6), subMeshCount (6), vertexCount (5), SetTriangles (3), MarkDynamic (2), SetIndices (2), colors32 (2), uv3 (2) |
| `MeshFilter` | mesh (50), sharedMesh (32) |
| `Camera` | velocity (52), main (47), targetTexture (44), fieldOfView (43), WorldToScreenPoint (42), rect (36), current (35), farClipPlane (31), clearFlags (30), ScreenPointToRay (28), cullingMask (28), orthographicSize (24), nearClipPlane (21), backgroundColor (20), mode (15), orthographic (14), Render (12), allowHDR (11) |
| `AudioSource` | volume (83), time (53), Play (51), Stop (48), isPlaying (43), clip (36), PlayOneShot (33), pitch (29), loop (27), spatialBlend (13), Pause (10), dopplerLevel (9), playOnAwake (8), rolloffMode (7), maxDistance (5), priority (3), PlayDelayed (3), mute (2) |
| `Light` | color (64), intensity (47), type (17), cullingMask (10), range (5), shadowNormalBias (5), shadows (2), renderingLayerMask (2), Reset (2), shadowBias (2), spotAngle (1), shape (1) |
| `ParticleSystem` | position (157), x (87), y (87), z (82), enabled (77), MinMaxCurve (57), main (49), constantMax (42), size (40), MainModule (38), Play (37), constantMin (36), startLifetime (34), constant (32), Stop (32), emission (31), Evaluate (31), Particle (28) |
| `MonoBehaviour` | Invoke (86), print (57), StartCoroutine (27), StopCoroutine (4), IsInvoking (2), CancelInvoke (2) |
| `RectTransform` | sizeDelta (235), anchoredPosition (139), rect (112), offsetMax (53), offsetMin (45), anchorMin (22), anchorMax (21), pivot (16), Axis (11), GetWorldCorners (9), anchoredPosition3D (3), SetSizeWithCurrentAnchors (2), GetLocalCorners (2) |

## 6. Shim specification for the core (tiers A and B)

The core is plain C++20 with no UE dependency. These are the Unity-shaped facilities the transpiled (A) and re-architected (B) code needs. Items marked **native** are implemented in Unity's C++ runtime, so the decompilation does not specify them; they need golden data captured from KSP (harness datasets named in TIERING.md).

| Facility | Used by tiers | Spec / notes |
|---|---|---|
| Value types: `Vector2/3/4`, `Quaternion`, `Matrix4x4`, `Color`, `Color32`, `Rect`, `Bounds`, `Ray`, `Plane` | A, B | Float (32-bit) semantics like Unity's. `Vector3.Normalize` returns zero below magnitude 1e-5, `==` is approximate (squared distance < 9.9999994e-11). Quaternion `AngleAxis`, `LookRotation`, `FromToRotation`, `Euler` (ZXY order), `Inverse`, `Slerp`, `Lerp` are **native**. Keep separate from KSP's own double types (`Vector3d`, `QuaternionD`, `Matrix4x4D`), which are managed and transpile directly. Golden: `math_shim`. |
| `Mathf` | A, B | Managed in Unity, so copy the semantics exactly: `Round`/`RoundToInt` use banker's rounding (`Math.Round`, to-even), `Sign(0) = 1` (`f >= 0`), `Approximately` is `|b-a| < max(1e-6*max(|a|,|b|), Epsilon*8)`, plus `Repeat`, `PingPong`, `SmoothDamp`, `MoveTowards`, `DeltaAngle`. `PerlinNoise` is **native** (check use before porting). |
| `AnimationCurve` / `Keyframe` / `WrapMode` | A | Behind every `FloatCurve` (engine ISP curves, atmosphere curves, PQS mods, thermal curves). `Evaluate` is **native**: cubic Hermite between keys, tangents scaled by key interval, clamp/loop/ping-pong wrap. Bit-exact replication is critical. Golden: `curves_eval` (all GameData curves). |
| `Random` | A, B | **Native** xorshift128 with `InitState(seed)` seeding. Used with explicit seeds by `ContractSystem`, `Contract` (mission seed), `TextGen`, `SurveyContract`, `ModuleAsteroid`/`ModuleComet`/`ProceduralAsteroid`/`ProceduralComet`, `LandClassROC`, `PQSLandControl`, `PQSMod_MeshScatter`, `DiscoveryInfo`, `Kerbal` (by name hash). Seeded contract offers and asteroid shapes only match KSP if this is bit-exact. Golden: `unity_random` (sequences for 1k seeds, Range int/float, value, insideUnitSphere). `KSPRandom` (managed Knuth subtractive, used by GrandTour, CometManager and lat/lon picking) transpiles directly. KSP's `GetHashCode_Net35` string hash must also be replicated. |
| `Time` | A, B | Core clock: `fixedDeltaTime` (KSP physics step 0.02 s; `TimeWarp` sets it to 0.02 * timeScale during physics warp), `deltaTime`, `time`, `timeScale` (TimeWarp physics warp), `realtimeSinceStartup`, `unscaledDeltaTime`, `frameCount`, `maximumDeltaTime`. Orbital time is `Planetarium.GetUniversalTime()` (double), which is separate. |
| `Debug` | A, B | Logging sink. `Debug.DrawLine`/`DrawRay` become a debug-draw callback (shell). |
| Coroutines (`StartCoroutine`, `yield return`, `WaitForSeconds`, `WaitForFixedUpdate`, `WaitForEndOfFrame`), `Invoke`/`InvokeRepeating` | A, B | Scheduler bound to the tick phases. Many A rows use coroutines only as timers. Preserve ordering relative to FixedUpdate: `WaitForFixedUpdate` sequencing is used in `Part` (multi-step physics init/coupling), `TimeWarp`, `physicalObject`, `PSystemSetup`, `KerbalEVA`, `ModuleDataTransmitter`. |
| Object model: `MonoBehaviour`, `GameObject`, `Component`, `GetComponent*`, `AddComponent`, `Instantiate`, `Destroy`, `SetActive`, `SendMessage`, `DontDestroyOnLoad` | B (A hosts) | Entity + component registry with typed lookup, active flags, deferred destroy (end of frame, like Unity), prefab cloning for parts (`PartLoader` prefab -> instance). `SendMessage` (string-dispatched) becomes typed interfaces or events. Execution order: KSP's `Timing0..5`, `TimingPre` and `TimingFI` proxies define the per-tick phase order. |
| `Transform` hierarchy | B | Double-precision local/world TRS with parent/child, `TransformPoint`/`InverseTransformPoint`/`Direction`/`Vector`, `up`/`forward`/`right`, `Find(path)`, sibling order, `SetParent(worldPositionStays)`. All of it is **native** in Unity. Part models depend on named child transforms (thrustTransform, attach nodes, CoM offsets), so names and hierarchy must come from the model import. Floating origin and Krakensbane shift the root positions. |
| `Rigidbody` | B | Facade over the chosen solver: `position`/`rotation`, `velocity`/`angularVelocity`, `mass`, `centerOfMass`/`worldCenterOfMass`, `inertiaTensor`(+rotation), `AddForce`/`AddForceAtPosition`/`AddTorque` with `ForceMode`, `isKinematic`, `useGravity` (KSP applies gravity itself), `drag`/`angularDrag`, `maxAngularVelocity`, `constraints`, `Sleep`/`WakeUp`, `GetPointVelocity`, `MovePosition`/`MoveRotation`, `detectCollisions`, interpolation (shell). PhysX integration order and solver iterations are **native**. Match behaviour with scenario traces, not bit-exactly. |
| Joints: `ConfigurableJoint`, `FixedJoint`, `Joint`, `JointDrive`, `SoftJointLimit(Spring)`, `ConfigurableJointMotion`, `JointProjectionMode`, `RotationDriveMode` | B | Used by `PartJoint` (part-to-part attachment with stiffness from attach-node size and PhysicsGlobals), autostruts, struts, docking/grapple, launch clamps, robotics servos, EVA anchors. Members used: x/y/z and angular motions, low/high angular limits, limit springs, `angularXDrive`/`angularYZDrive`/`xDrive`/`slerpDrive`, `targetRotation`/`targetAngularVelocity`/`targetPosition`, `secondaryAxis`/`axis`/anchors, `configuredInWorldSpace`, `breakForce`/`breakTorque`, `connectedBody`, `connectedMassScale`, projection. The reduced-rigid-body architecture (FEASIBILITY.md section 2.2) can satisfy this through a structural model plus a few articulated joints. |
| Colliders and queries: `Collider` (Box/Sphere/Capsule/Mesh/convex), `PhysicMaterial`, `Physics.Raycast`/`RaycastAll`/`SphereCast`/`OverlapSphere`/`Linecast`/`IgnoreCollision`, `RaycastHit`, `LayerMask`, `QueryTriggerInteraction`, trigger volumes | B | Layer filtering is gameplay-relevant: code resolves layers by name (`Local Scenery`, `Scaled Scenery`, `PhysicalObjects`, `TerrainColliders`, `WheelColliders`, `WheelCollidersIgnore`, `Water`, `Ignore Raycast`, ...) plus numeric masks; the name->index table lives in Unity project settings, not in code, so capture it from the game data and port it. `Physics.IgnoreCollision` (39 calls) manages same-vessel and docking collisions. Collision callbacks (`OnCollisionEnter/Stay/Exit`) feed crash tolerance and ground contact. |
| `WheelCollider` (+ VehiclePhysics) | B | **Native** PhysX suspension raycast, sprung mass and spring/damper. Tyre friction is replaced by VehiclePhysics `TireFriction` (stiffness zeroed). The core needs its own raycast suspension. See third-party.md. |
| `Animation` / `AnimationState` / `AnimationClip` | B | Legacy Animation is used to drive gameplay state: deployables, landing gear, parachutes, cargo bays, solar tracking, `ModuleAnimateGeneric` progress feeding drag cubes. The core needs a clip sampler (normalized time, speed, wrap, blend weight) over imported rigid-transform curves. Clip curves come from the model import (.mu). Kerbal skeletal animation is shell-only. |
| `Mesh` / `MeshFilter` (B rows) | B | Only where geometry has gameplay meaning: PQS quad meshes and colliders, procedural fairing panels and colliders, asteroid/comet colliders, drag cube rendering (`DragCubeSystem` renders parts from 6 directions; `PartLoader` caches the results in `PartDatabase.cfg`, so the port can import KSP's generated cache first and defer cube generation). |
| `Texture2D` pixel reads | A, B | Biome maps (`CBAttributeMapSO`), PQS height/colour maps (`MapSO`), resource maps: the core needs raw pixel arrays plus KSP's bilinear sampling, not GPU textures. |
| `ScriptableObject` data assets | A | MapSO/CBAttributeMapSO/PQS assets become plain data loaded from the import pipeline. |
| Input (`Input.*`, `KeyCode`) | C | Enhanced Input in the shell. The core sees `FlightCtrlState`, action-group triggers and editor commands. `GameSettings` binding names map to Input Actions. |
| UI (`UnityEngine.UI`, `EventSystems`, TMPro, IMGUI) | C | UMG/CommonUI. No core shim. Rule-owning UI classes are tier B and get their rules extracted (see TIERING.md). |

## 7. Unity names also declared by KSP (excluded from the type table)

`Axis`, `BaseField`, `Behaviour`, `ColorTween`, `CommandType`, `Easing`, `EventType`, `Extents`, `FaceInfo`, `FloatTween`, `FontData`, `FontStyles`, `FontWeights`, `Gender`, `GlyphValueRecord`, `HorizontalAlignment`, `ITextElement`, `ITweenValue`, `Input`, `InputDevices`, `KerningPair`, `KerningPairKey`, `KerningTable`, `ListPool`, `Marker`, `MaterialReference`, `MaterialReferenceManager`, `MouseButton`, `MouseOverEvent`, `Page`, `Panel`, `Plane`, `Pool`, `Ray`, `Renderer`, `Resources`, `SetPropertyUtility`, `ShaderPropertyType`, `ShaderUtilities`, `State`, `TextureFormat`, `TweenRunner`, `Vertex`, `VertexSortingOrder`, `VerticalAlignment`, `WordWrapState`, `WrapMode`.
