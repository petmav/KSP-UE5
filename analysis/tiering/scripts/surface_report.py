"""Writes analysis/tiering/unity-api-surface.md from work/unity_*.json + work/api_groups_by_tier.json.

Run: python -I surface_report.py     (after unity_surface.py, merge.py and the api_groups_by_tier step in TIERING notes)
"""
import json, os

BASE = r"C:/Projects/KSP-UE5/analysis/tiering"
WORK = os.path.join(BASE, "work")

# Unity type -> (C++ core shim / UE5 equivalent)
EQUIV = {
    "MonoBehaviour": "core: component/host base with tick phases (WP-00); C rows: UObject/UActorComponent or UMG widget",
    "Vector3": "core: `ksp::Vector3f` with Unity float semantics; shell: FVector3f/FVector at boundary",
    "Debug": "core: `ksp::log` (Log/Warning/Error/Exception); DrawLine/DrawRay -> debug draw hook",
    "Object": "core: object lifetime (Instantiate/Destroy) via entity registry; shell: SpawnActor/DestroyActor",
    "Mathf": "core: `ksp::Mathf` (float, Round = banker's, Sign(0)=1, Approximately, Repeat/PingPong, SmoothDamp)",
    "SerializeField": "drop (attribute); persistence comes from KSPField/ConfigNode",
    "GameObject": "core: entity handle + name/active flag/layer/tag; shell: AActor",
    "Transform": "core: double-precision transform hierarchy (WP-00); shell: USceneComponent (camera-relative)",
    "Color": "core: `ksp::Color` (float RGBA) for data; shell: FLinearColor",
    "Vector2": "core: `ksp::Vector2f`; shell: FVector2D",
    "Quaternion": "core: `ksp::Quaternionf` replicating Unity natives (AngleAxis, LookRotation, FromToRotation, Euler ZXY order, Slerp)",
    "Time": "core: `ksp::Clock` (deltaTime, fixedDeltaTime, time, timeScale, realtimeSinceStartup, frameCount)",
    "RectTransform": "UMG slot layout (anchors/offsets) - C only",
    "Button": "UMG UButton / CommonUI button",
    "PointerEventData": "UMG/Slate pointer events",
    "Random": "core: bit-exact Unity Random (xorshift128 + InitState seeding + Range mapping) - needed for seeded contracts/asteroids/ROC",
    "Image": "UMG UImage",
    "Camera": "shell: UCameraComponent / SceneCapture; core needs only camera position for LOD/occlusion queries",
    "Texture2D": "shell: UTexture2D; core: map sampling via plain pixel arrays (MapSO/CBAttributeMap)",
    "Material": "shell: UMaterialInstanceDynamic",
    "AddComponentMenu": "drop (editor attribute)",
    "Toggle": "UMG UCheckBox",
    "RequireComponent": "drop (editor attribute)",
    "Rect": "core: `ksp::Rect` (data); UI: FSlateRect",
    "ContextMenu": "drop (editor attribute)",
    "KeyCode": "Enhanced Input keys (FKey) via binding table",
    "IEventSystemHandler": "UMG/Slate events",
    "HideInInspector": "drop (editor attribute)",
    "Sprite": "UMG brush / Paper2D sprite",
    "Slider": "UMG USlider",
    "Collider": "core: collision shape handle in physics shim (box/sphere/capsule/mesh/convex)",
    "Physics": "core: physics query API (Raycast, RaycastAll, SphereCast, OverlapSphere, Linecast, IgnoreCollision, gravity)",
    "MeshRenderer": "shell: UStaticMeshComponent / UProceduralMeshComponent",
    "Shader": "shell: material parameter names (FName) / global shader params via Material Parameter Collection",
    "Rigidbody": "core: rigid-body facade over the chosen solver (Chaos/Jolt/custom): pose, velocity, mass, CoM, inertia, forces",
    "Screen": "shell: viewport size (GEngine->GameViewport)",
    "RawImage": "UMG UImage (render target brush)",
    "WaitForSeconds": "core: timer awaitable in the coroutine scheduler (WP-00)",
    "Application": "shell: FPlatformMisc / paths; core: data-path config",
    "RaycastHit": "core: hit record (point, normal, distance, collider, rigidbody, triangle index)",
    "AudioSource": "UAudioComponent / MetaSounds",
    "LayoutElement": "UMG size box / slot",
    "LayerMask": "core: layer bitmask constants table (KSP layer numbers are gameplay-relevant for raycast filtering)",
    "Selectable": "UMG widget focus/navigation",
    "Texture": "shell: UTexture",
    "Vector4": "core: `ksp::Vector4f`",
    "Mesh": "core (B rows only): mesh buffers for colliders/drag cubes; shell: UProceduralMeshComponent / RealtimeMesh",
    "Animation": "core: animation sampler (clip curves sampled at normalized time) for gameplay-driving animations; shell: skeletal/rigid anim",
    "ScrollRect": "UMG UScrollBox",
    "Coroutine": "core: coroutine scheduler (C++20 coroutines or explicit state machines)",
    "IPointerExitHandler": "UMG OnMouseLeave",
    "IPointerEnterHandler": "UMG OnMouseEnter",
    "ParticleSystem": "Niagara",
    "Space": "core: enum (Self/World) for transform ops",
    "TextAnchor": "UMG text justification",
    "Canvas": "UMG root widget / Widget Component",
    "RectOffset": "UMG margin",
    "AnimationCurve": "core: `ksp::AnimationCurve` evaluator (Unity Hermite + wrap modes) - native in Unity, needs golden",
    "Light": "shell: ULightComponent (point/spot/directional)",
    "MeshFilter": "shell: mesh component",
    "AudioClip": "USoundWave",
    "EventSystem": "Slate focus/navigation",
    "Display": "shell: multi-display info",
    "ToggleGroup": "UMG radio group (CommonUI)",
    "Bounds": "core: `ksp::Bounds` (AABB maths)",
    "SceneManager": "core: scene state machine (HighLogic); shell: level streaming / OpenLevel",
    "GUI": "drop or UMG (IMGUI debug windows)",
    "CanvasGroup": "UMG render opacity / visibility",
    "IPointerClickHandler": "UMG OnClicked",
    "Color32": "core: `ksp::Color32`; shell: FColor",
    "RenderTexture": "UTextureRenderTarget2D",
    "LoadSceneMode": "level streaming mode",
    "Matrix4x4": "core: `ksp::Matrix4x4f` (TRS, inverse, MultiplyPoint3x4)",
    "Navigation": "UMG navigation rules",
    "Scene": "level handle",
    "BaseEventData": "Slate event",
    "CameraClearFlags": "camera setting (shell)",
    "MaterialPropertyBlock": "per-instance material params (Custom Primitive Data)",
    "GUILayout": "drop or UMG (IMGUI debug windows)",
    "MeshCollider": "core: triangle-mesh/convex collider in physics shim",
    "Keyframe": "core: `ksp::Keyframe` (time, value, inTangent, outTangent, weights)",
    "Text": "UMG UTextBlock",
    "UnityAction": "std::function / delegate",
    "UnityEvent": "multicast delegate",
    "ExecuteInEditMode": "drop (editor attribute)",
    "Gizmos": "debug draw (drop)",
    "RectTransformUtility": "Slate geometry helpers",
    "WaitForEndOfFrame": "core: end-of-frame awaitable",
    "ContentSizeFitter": "UMG auto-size",
    "AnimationState": "core: animation channel state (time, speed, weight, wrapMode, enabled)",
    "ColorUtility": "core: hex colour parse/format",
    "VerticalLayoutGroup": "UMG UVerticalBox",
    "RenderTextureFormat": "render target format (shell)",
    "UnityWebRequest": "drop (or FHttpModule)",
    "EventTrigger": "UMG events",
    "Component": "core: component base",
    "Collision": "core: contact report (impulse, relative velocity, contact points, other collider)",
    "ConfigurableJoint": "core: 6-DoF joint facade (linear/angular motions, limits, drives, target rotation, break force/torque)",
    "IDragHandler": "UMG drag-drop operation",
    "SystemInfo": "shell: platform info (drop for core)",
    "RuntimePlatform": "shell: platform enum",
    "SkinnedMeshRenderer": "shell: USkeletalMeshComponent (kerbals)",
    "EventTriggerType": "UMG events",
    "ShadowCastingMode": "shell: shadow flags",
    "QueueMode": "animation queue mode (core animation sampler if gameplay)",
    "Scrollbar": "UMG scroll bar",
    "Event": "Slate input event (IMGUI)",
    "SphereCollider": "core: sphere collider",
    "SendMessageOptions": "drop (typed events instead)",
    "QueryTriggerInteraction": "core: raycast trigger filter flag",
    "BoxCollider": "core: box collider",
    "CapsuleCollider": "core: capsule collider",
    "WheelCollider": "core: raycast suspension model replacing PhysX WheelCollider (WP-29)",
    "Joint": "core: joint base facade (connectedBody, anchors, break force/torque)",
    "FixedJoint": "core: fixed joint",
    "ForceMode": "core: force mode enum (Force/Impulse/Acceleration/VelocityChange)",
    "PhysicMaterial": "core: friction/bounciness material with combine modes",
    "JointDrive": "core: drive params (spring, damper, max force)",
    "SoftJointLimit": "core: joint limit params",
    "ConfigurableJointMotion": "core: motion enum (Locked/Limited/Free)",
    "ContactPoint": "core: contact record",
    "CollisionDetectionMode": "core: CCD mode flag",
    "RigidbodyInterpolation": "shell: render interpolation",
    "Renderer": "shell: primitive component",
    "Input": "Enhanced Input (via binding table); core sees only FlightCtrlState/actions",
    "Resources": "Asset Manager / soft object paths",
    "WrapMode": "core: curve/animation wrap enum",
}


def main():
    types = json.load(open(os.path.join(WORK, "unity_types.json"), encoding="utf-8"))
    statics = json.load(open(os.path.join(WORK, "unity_static_members.json"), encoding="utf-8"))
    inst = json.load(open(os.path.join(WORK, "unity_instance_members.json"), encoding="utf-8"))
    life = json.load(open(os.path.join(WORK, "unity_lifecycle.json"), encoding="utf-8"))
    groups = json.load(open(os.path.join(WORK, "api_groups_by_tier.json"), encoding="utf-8"))
    L = []
    a = L.append
    a("# Unity API surface used by KSP-authored code (shim spec for the re-architect tier)")
    a("")
    a("Generated 2026-10-08 by `analysis/tiering/scripts/unity_surface.py` and `surface_report.py`. Scope: the "
      f"{types['files_scanned']:,} KSP-authored files in `tiering.csv` (Assembly-CSharp without the embedded third-party "
      "namespaces, plus the reviewed part of Assembly-CSharp-firstpass). Tier splits use the final `tiering.csv`.")
    a("")
    a("## How to read this")
    a("")
    a("- **Type table**: a file counts as using a Unity type if the type name appears as an identifier after comments "
      "and strings are stripped, and the file imports the type's namespace. Unity names that KSP also declares (e.g. "
      "`Input`, `Renderer`, `Resources`, `Plane`, `Ray`, `WrapMode`; full list at the end) are excluded from the type "
      "table to avoid false hits; their usage is covered by the API-family table, which uses member-access regexes "
      "such as `Input.`.")
    a("- **Static members** are exact (`Type.Member` occurrences). **Instance members** are approximate: they count "
      "`.member` accesses whose name is a public member of the Unity type in files that use that type, so names shared "
      "with other types (e.g. `.position` on Transform vs Rigidbody, `.enabled`) are conflated.")
    a("- **Equivalent / shim**: what the C++ core provides (tiers A and B) or what UE5 replaces it with (tier C).")
    a("")
    # ---------------------------------------------------------------- families
    a("## 1. API families by tier")
    a("")
    a("Files / references per tier (A = transpile, B = re-architect, C = UE5 rewrite). Families with zero A use need no "
      "core shim. Families used by B need the core shim. Families used only by C map to UE5 directly.")
    a("")
    a("| API family | Files | Refs | A files/refs | B files/refs | C files/refs | Core shim needed? |")
    a("|---|---:|---:|---|---|---|---|")
    shim_note = {
        "Lifecycle": "yes: tick phases", "MonoBehaviour": "yes: host base", "GameObject": "yes (B): entity handle",
        "Vector2/3/4": "yes: float vector maths", "Transform": "yes (B): transform hierarchy",
        "GetComponent": "yes (B): typed component lookup", "SerializeField": "no", "Debug.": "yes: logging",
        "UnityEngine.UI": "no (UMG)", "Instantiate/Destroy": "yes (B): spawn/destroy", "Mathf": "yes: Mathf",
        "SetActive": "yes (B): active flag", "Color": "yes: colour data", "TMPro": "no (UMG)",
        "Quaternion": "yes: float quaternion", "Coroutine": "yes: scheduler", "Time.": "yes: clock",
        "Texture": "minimal: pixel arrays for map sampling", "EventSystems": "no (UMG)", "Material": "no (shell)",
        "Renderer": "no (shell); bounds query for a few B rows", "Shader": "no (shell)",
        "Rect/Bounds/Ray/Plane": "yes: geometry value types", "Input.": "no (Enhanced Input -> FlightCtrlState)",
        "Camera": "minimal: camera position for LOD/occlusion", "Random": "yes: bit-exact Unity Random",
        "Collider": "yes (B): collision shapes", "LayerMask": "yes: layer constants", "KeyCode": "no",
        "Raycast/Query": "yes (B): scene queries", "Rigidbody": "yes (B): rigid-body facade", "Screen/Cursor": "no",
        "Audio": "no", "Physics.": "yes (B): physics world API", "Animation": "yes (B): animation sampler",
        "Resources.": "no (Asset Manager); data-path for configs", "Mesh": "yes (B): mesh buffers for colliders/cubes",
        "Application.": "minimal: paths", "ParticleSystem": "no (Niagara)", "Analytics": "no (drop)", "IMGUI": "no",
        "Light": "no", "AnimationCurve": "yes: curve evaluator", "SendMessage/Invoke": "yes: timers/typed events",
        "SceneManager": "yes (B): scene state", "Collision/Force": "yes (B): contacts, ForceMode",
        "Matrix4x4": "yes: matrix maths", "Joint": "yes (B): joint facade", "WWW/WebRequest": "no",
        "QualitySettings/SystemInfo": "no", "Steam": "no (drop)", "WheelCollider": "yes (B): suspension model",
        "ScriptableObject": "minimal: data assets -> plain structs", "PhysicMaterial": "yes (B): friction materials",
        "AssetBundle": "no", "Graphics/GL": "no", "PlayerPrefs": "no (settings store)", "LODGroup": "no",
    }
    for g, nf, nr, files, refs in groups:
        if nf == 0:
            continue
        a(f"| {g} | {nf} | {nr:,} | {files.get('A', 0)}/{refs.get('A', 0)} | {files.get('B', 0)}/{refs.get('B', 0)} | "
          f"{files.get('C', 0)}/{refs.get('C', 0)} | {shim_note.get(g, '')} |")
    a("")
    # ---------------------------------------------------------------- lifecycle
    a("## 2. MonoBehaviour messages (lifecycle) -> core tick phases")
    a("")
    a("Counts are method definitions with the Unity message name (`Reset` is inflated by KSP methods named Reset).")
    a("")
    a("| Message | Definitions | Core equivalent |")
    a("|---|---:|---|")
    life_map = {
        "Awake": "construct + `OnAwake` (registration)", "Start": "first-tick init (`OnStart` ordering preserved)",
        "Update": "variable-rate frame tick (shell-driven; core only for UI-facing state)",
        "FixedUpdate": "fixed physics tick (core main loop, KSP order Timing0..5 / TimingPre / TimingFI)",
        "LateUpdate": "post-frame tick (camera/visual sync; mostly shell)", "OnDestroy": "teardown / unregister",
        "OnEnable": "activate", "OnDisable": "deactivate", "OnGUI": "drop (IMGUI) or UMG",
        "OnCollisionEnter": "physics contact callback (enter)", "OnCollisionStay": "physics contact callback (stay)",
        "OnCollisionExit": "physics contact callback (exit)", "OnTriggerEnter": "trigger volume callback",
        "OnTriggerStay": "trigger volume callback", "OnTriggerExit": "trigger volume callback",
        "OnMouseDown": "shell picking -> core action", "OnMouseUp": "shell picking", "OnMouseEnter": "shell picking",
        "OnMouseExit": "shell picking", "OnMouseOver": "shell picking", "OnMouseDrag": "shell picking",
        "OnJointBreak": "joint break callback (core physics)", "OnApplicationFocus": "shell", "Reset": "n/a",
        "OnDrawGizmos": "drop", "OnDrawGizmosSelected": "drop", "OnPreRender": "shell", "OnPostRender": "shell",
        "OnRenderImage": "shell post-process", "OnRenderObject": "shell", "OnPreCull": "shell",
        "OnBecameVisible": "shell", "OnBecameInvisible": "shell", "OnRectTransformDimensionsChange": "UMG",
        "OnValidate": "drop", "OnApplicationQuit": "shutdown", "OnApplicationPause": "shell",
    }
    for m, v in sorted(life.items(), key=lambda kv: -kv[1]["defs"]):
        a(f"| {m} | {v['defs']} | {life_map.get(m, '')} |")
    a("")
    # ---------------------------------------------------------------- types
    a("## 3. Unity types ranked by number of KSP files using them (top 120)")
    a("")
    a("| # | Type | Module | Files | Refs | A | B | C | C++ core shim / UE5 equivalent |")
    a("|---:|---|---|---:|---:|---:|---:|---:|---|")
    for i, t in enumerate(types["types"][:120], 1):
        tr = t["tiers"]
        a(f"| {i} | `{t['type']}` | {t['module'].replace('UnityEngine.', '')} | {t['files']} | {t['refs']:,} | "
          f"{tr.get('A', 0)} | {tr.get('B', 0)} | {tr.get('C', 0)} | {EQUIV.get(t['type'], '')} |")
    a("")
    a(f"Types used in total: {len(types['types'])}. Remaining types (each used by few files): " +
      ", ".join(f"`{t['type']}` ({t['files']})" for t in types["types"][120:]) + ".")
    a("")
    # ---------------------------------------------------------------- statics
    a("## 4. Static API members (exact counts)")
    a("")
    a("| Host | Total | Members (count) |")
    a("|---|---:|---|")
    order = ["Debug", "Mathf", "Vector3", "GUILayout", "Quaternion", "Time", "Object", "Shader", "Screen",
             "Application", "Random", "Physics", "GUI", "SceneManager", "QualitySettings", "Gizmos", "LayerMask",
             "Camera", "SystemInfo", "GameObject", "PlayerPrefs", "RectTransformUtility", "RenderSettings", "Cursor",
             "Graphics", "GL", "AssetBundle"]
    for h in order:
        if h in statics:
            ms = statics[h]
            a(f"| `{h}` | {sum(c for _, c in ms):,} | " + ", ".join(f"{m} ({c})" for m, c in ms[:16]) + " |")
    a("| `Input` (from regex scan) | 517 | GetKey (167), GetKeyDown (101), mousePosition (96), GetAxis (60), GetKeyUp (51), GetMouseButton (46), GetMouseButtonUp (34), GetMouseButtonDown (32), compositionString (11) |")
    a("")
    # ---------------------------------------------------------------- instance
    a("## 5. Instance member hotspots (approximate)")
    a("")
    a("| Type | Most used members (count) |")
    a("|---|---|")
    for h in ["Transform", "GameObject", "Rigidbody", "ConfigurableJoint", "Joint", "Collider", "WheelCollider",
              "Animation", "AnimationState", "Renderer", "Material", "Mesh", "MeshFilter", "Camera", "AudioSource",
              "Light", "ParticleSystem", "MonoBehaviour", "RectTransform"]:
        if h in inst:
            a(f"| `{h}` | " + ", ".join(f"{m} ({c})" for m, c in inst[h][:18]) + " |")
    a("")
    # ---------------------------------------------------------------- shim spec
    a("## 6. Shim specification for the core (tiers A and B)")
    a("")
    a("The core is plain C++20 with no UE dependency. These are the Unity-shaped facilities the transpiled (A) and "
      "re-architected (B) code needs. Items marked **native** are implemented in Unity's C++ runtime, so the "
      "decompilation does not specify them; they need golden data captured from KSP (harness datasets named in "
      "TIERING.md).")
    a("")
    spec = [
        ("Value types: `Vector2/3/4`, `Quaternion`, `Matrix4x4`, `Color`, `Color32`, `Rect`, `Bounds`, `Ray`, `Plane`",
         "A, B", "Float (32-bit) semantics like Unity's. `Vector3.Normalize` returns zero below magnitude 1e-5, `==` is approximate (squared distance < 9.9999994e-11). Quaternion `AngleAxis`, `LookRotation`, `FromToRotation`, `Euler` (ZXY order), `Inverse`, `Slerp`, `Lerp` are **native**. Keep separate from KSP's own double types (`Vector3d`, `QuaternionD`, `Matrix4x4D`), which are managed and transpile directly. Golden: `math_shim`."),
        ("`Mathf`", "A, B", "Managed in Unity, so copy the semantics exactly: `Round`/`RoundToInt` use banker's rounding (`Math.Round`, to-even), `Sign(0) = 1` (`f >= 0`), `Approximately` is `|b-a| < max(1e-6*max(|a|,|b|), Epsilon*8)`, plus `Repeat`, `PingPong`, `SmoothDamp`, `MoveTowards`, `DeltaAngle`. `PerlinNoise` is **native** (check use before porting)."),
        ("`AnimationCurve` / `Keyframe` / `WrapMode`", "A", "Behind every `FloatCurve` (engine ISP curves, atmosphere curves, PQS mods, thermal curves). `Evaluate` is **native**: cubic Hermite between keys, tangents scaled by key interval, clamp/loop/ping-pong wrap. Bit-exact replication is critical. Golden: `curves_eval` (all GameData curves)."),
        ("`Random`", "A, B", "**Native** xorshift128 with `InitState(seed)` seeding. Used with explicit seeds by `ContractSystem`, `Contract` (mission seed), `TextGen`, `SurveyContract`, `ModuleAsteroid`/`ModuleComet`/`ProceduralAsteroid`/`ProceduralComet`, `LandClassROC`, `PQSLandControl`, `PQSMod_MeshScatter`, `DiscoveryInfo`, `Kerbal` (by name hash). Seeded contract offers and asteroid shapes only match KSP if this is bit-exact. Golden: `unity_random` (sequences for 1k seeds, Range int/float, value, insideUnitSphere). `KSPRandom` (managed Knuth subtractive, used by GrandTour, CometManager and lat/lon picking) transpiles directly. KSP's `GetHashCode_Net35` string hash must also be replicated."),
        ("`Time`", "A, B", "Core clock: `fixedDeltaTime` (KSP physics step 0.02 s; `TimeWarp` sets it to 0.02 * timeScale during physics warp), `deltaTime`, `time`, `timeScale` (TimeWarp physics warp), `realtimeSinceStartup`, `unscaledDeltaTime`, `frameCount`, `maximumDeltaTime`. Orbital time is `Planetarium.GetUniversalTime()` (double), which is separate."),
        ("`Debug`", "A, B", "Logging sink. `Debug.DrawLine`/`DrawRay` become a debug-draw callback (shell)."),
        ("Coroutines (`StartCoroutine`, `yield return`, `WaitForSeconds`, `WaitForFixedUpdate`, `WaitForEndOfFrame`), `Invoke`/`InvokeRepeating`", "A, B", "Scheduler bound to the tick phases. Many A rows use coroutines only as timers. Preserve ordering relative to FixedUpdate: `WaitForFixedUpdate` sequencing is used in `Part` (multi-step physics init/coupling), `TimeWarp`, `physicalObject`, `PSystemSetup`, `KerbalEVA`, `ModuleDataTransmitter`."),
        ("Object model: `MonoBehaviour`, `GameObject`, `Component`, `GetComponent*`, `AddComponent`, `Instantiate`, `Destroy`, `SetActive`, `SendMessage`, `DontDestroyOnLoad`", "B (A hosts)", "Entity + component registry with typed lookup, active flags, deferred destroy (end of frame, like Unity), prefab cloning for parts (`PartLoader` prefab -> instance). `SendMessage` (string-dispatched) becomes typed interfaces or events. Execution order: KSP's `Timing0..5`, `TimingPre` and `TimingFI` proxies define the per-tick phase order."),
        ("`Transform` hierarchy", "B", "Double-precision local/world TRS with parent/child, `TransformPoint`/`InverseTransformPoint`/`Direction`/`Vector`, `up`/`forward`/`right`, `Find(path)`, sibling order, `SetParent(worldPositionStays)`. All of it is **native** in Unity. Part models depend on named child transforms (thrustTransform, attach nodes, CoM offsets), so names and hierarchy must come from the model import. Floating origin and Krakensbane shift the root positions."),
        ("`Rigidbody`", "B", "Facade over the chosen solver: `position`/`rotation`, `velocity`/`angularVelocity`, `mass`, `centerOfMass`/`worldCenterOfMass`, `inertiaTensor`(+rotation), `AddForce`/`AddForceAtPosition`/`AddTorque` with `ForceMode`, `isKinematic`, `useGravity` (KSP applies gravity itself), `drag`/`angularDrag`, `maxAngularVelocity`, `constraints`, `Sleep`/`WakeUp`, `GetPointVelocity`, `MovePosition`/`MoveRotation`, `detectCollisions`, interpolation (shell). PhysX integration order and solver iterations are **native**. Match behaviour with scenario traces, not bit-exactly."),
        ("Joints: `ConfigurableJoint`, `FixedJoint`, `Joint`, `JointDrive`, `SoftJointLimit(Spring)`, `ConfigurableJointMotion`, `JointProjectionMode`, `RotationDriveMode`", "B", "Used by `PartJoint` (part-to-part attachment with stiffness from attach-node size and PhysicsGlobals), autostruts, struts, docking/grapple, launch clamps, robotics servos, EVA anchors. Members used: x/y/z and angular motions, low/high angular limits, limit springs, `angularXDrive`/`angularYZDrive`/`xDrive`/`slerpDrive`, `targetRotation`/`targetAngularVelocity`/`targetPosition`, `secondaryAxis`/`axis`/anchors, `configuredInWorldSpace`, `breakForce`/`breakTorque`, `connectedBody`, `connectedMassScale`, projection. The reduced-rigid-body architecture (FEASIBILITY.md section 2.2) can satisfy this through a structural model plus a few articulated joints."),
        ("Colliders and queries: `Collider` (Box/Sphere/Capsule/Mesh/convex), `PhysicMaterial`, `Physics.Raycast`/`RaycastAll`/`SphereCast`/`OverlapSphere`/`Linecast`/`IgnoreCollision`, `RaycastHit`, `LayerMask`, `QueryTriggerInteraction`, trigger volumes", "B", "Layer filtering is gameplay-relevant: code resolves layers by name (`Local Scenery`, `Scaled Scenery`, `PhysicalObjects`, `TerrainColliders`, `WheelColliders`, `WheelCollidersIgnore`, `Water`, `Ignore Raycast`, ...) plus numeric masks; the name->index table lives in Unity project settings, not in code, so capture it from the game data and port it. `Physics.IgnoreCollision` (39 calls) manages same-vessel and docking collisions. Collision callbacks (`OnCollisionEnter/Stay/Exit`) feed crash tolerance and ground contact."),
        ("`WheelCollider` (+ VehiclePhysics)", "B", "**Native** PhysX suspension raycast, sprung mass and spring/damper. Tyre friction is replaced by VehiclePhysics `TireFriction` (stiffness zeroed). The core needs its own raycast suspension. See third-party.md."),
        ("`Animation` / `AnimationState` / `AnimationClip`", "B", "Legacy Animation is used to drive gameplay state: deployables, landing gear, parachutes, cargo bays, solar tracking, `ModuleAnimateGeneric` progress feeding drag cubes. The core needs a clip sampler (normalized time, speed, wrap, blend weight) over imported rigid-transform curves. Clip curves come from the model import (.mu). Kerbal skeletal animation is shell-only."),
        ("`Mesh` / `MeshFilter` (B rows)", "B", "Only where geometry has gameplay meaning: PQS quad meshes and colliders, procedural fairing panels and colliders, asteroid/comet colliders, drag cube rendering (`DragCubeSystem` renders parts from 6 directions; `PartLoader` caches the results in `PartDatabase.cfg`, so the port can import KSP's generated cache first and defer cube generation)."),
        ("`Texture2D` pixel reads", "A, B", "Biome maps (`CBAttributeMapSO`), PQS height/colour maps (`MapSO`), resource maps: the core needs raw pixel arrays plus KSP's bilinear sampling, not GPU textures."),
        ("`ScriptableObject` data assets", "A", "MapSO/CBAttributeMapSO/PQS assets become plain data loaded from the import pipeline."),
        ("Input (`Input.*`, `KeyCode`)", "C", "Enhanced Input in the shell. The core sees `FlightCtrlState`, action-group triggers and editor commands. `GameSettings` binding names map to Input Actions."),
        ("UI (`UnityEngine.UI`, `EventSystems`, TMPro, IMGUI)", "C", "UMG/CommonUI. No core shim. Rule-owning UI classes are tier B and get their rules extracted (see TIERING.md)."),
    ]
    a("| Facility | Used by tiers | Spec / notes |")
    a("|---|---|---|")
    for f, t, n in spec:
        a(f"| {f} | {t} | {n} |")
    a("")
    a("## 7. Unity names also declared by KSP (excluded from the type table)")
    a("")
    a(", ".join(f"`{n}`" for n in types["shadowed"]) + ".")
    a("")
    open(os.path.join(BASE, "unity-api-surface.md"), "w", encoding="utf-8").write("\n".join(L))
    print("written", len(L), "lines")


if __name__ == "__main__":
    main()
