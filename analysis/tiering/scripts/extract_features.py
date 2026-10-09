"""Per-file feature extraction for the KSP 1.12.5 decompilation (tiering analysis).

Run: python -I extract_features.py
Reads (read-only) C:/Projects/KSP-UE5/decompiled/KSP-Decompiled/{Assembly-CSharp,Assembly-CSharp-firstpass}
Writes analysis/tiering/work/features.jsonl
"""
import bisect, json, os, re
from collections import Counter

ROOT = r"C:/Projects/KSP-UE5/decompiled/KSP-Decompiled"
OUT = r"C:/Projects/KSP-UE5/analysis/tiering/work"


def iter_files():
    base = os.path.join(ROOT, "Assembly-CSharp")
    for dp, dn, fn in os.walk(base):
        rel = os.path.relpath(dp, base).replace("\\", "/")
        if rel.split("/")[0] in ("obj", "bin"):
            continue
        for f in fn:
            if f.endswith(".cs"):
                p = os.path.join(dp, f)
                yield "Assembly-CSharp/" + os.path.relpath(p, base).replace("\\", "/"), p
    base = os.path.join(ROOT, "Assembly-CSharp-firstpass")
    for dp, dn, fn in os.walk(base):
        rel = os.path.relpath(dp, base).replace("\\", "/")
        if rel.split("/")[0] in ("Steamworks", "Vectrosity", "obj", "bin"):
            continue
        for f in fn:
            if f.endswith(".cs"):
                p = os.path.join(dp, f)
                yield "Assembly-CSharp-firstpass/" + os.path.relpath(p, base).replace("\\", "/"), p


def strip_code(src):
    """Remove comments and string/char literals (replaced by empty literals). Keeps newlines."""
    out = []
    i, n = 0, len(src)
    while i < n:
        c = src[i]
        nx = src[i + 1] if i + 1 < n else ""
        if c == "/" and nx == "/":
            j = src.find("\n", i)
            if j < 0:
                break
            i = j
            continue
        if c == "/" and nx == "*":
            j = src.find("*/", i + 2)
            end = j + 2 if j >= 0 else n
            out.append("\n" * src.count("\n", i, end))
            i = end
            continue
        verbatim = (c == "@" and nx == '"') or (c in "$@" and nx in "$@" and nx != c and i + 2 < n and src[i + 2] == '"')
        if verbatim:
            k = src.find('"', i) + 1
            while k < n:
                if src[k] == '"':
                    if k + 1 < n and src[k + 1] == '"':
                        k += 2
                        continue
                    break
                k += 1
            out.append('""' + "\n" * src.count("\n", i, k + 1))
            i = k + 1
            continue
        if c == '"' or (c == "$" and nx == '"'):
            k = src.find('"', i) + 1
            while k < n and src[k] != '"' and src[k] != "\n":
                if src[k] == "\\":
                    k += 2
                    continue
                k += 1
            out.append('""')
            i = k + 1
            continue
        if c == "'":
            k = i + 1
            while k < n and src[k] != "'" and src[k] != "\n":
                if src[k] == "\\":
                    k += 2
                    continue
                k += 1
            out.append("''")
            i = k + 1
            continue
        out.append(c)
        i += 1
    return "".join(out)


def W(*names):
    return r"\b(?:" + "|".join(names) + r")\b"


# Unity API usage groups. Order is the column order in features/csv.
API_GROUPS = {
    # object model / scene graph
    "MonoBehaviour": W("MonoBehaviour"),
    "GameObject": W("GameObject") + r"|\bgameObject\b",
    "Transform": W("Transform") + r"|\btransform\b",
    "GetComponent": r"\b(?:GetComponent|GetComponents|GetComponentInChildren|GetComponentsInChildren|GetComponentInParent|GetComponentsInParent|AddComponent|FindObjectOfType|FindObjectsOfType)\b",
    "Instantiate/Destroy": r"\b(?:Instantiate|Destroy|DestroyImmediate|DontDestroyOnLoad)\s*[<(]",
    "SetActive": r"\bSetActive\s*\(|\bactiveSelf\b|\bactiveInHierarchy\b",
    "SendMessage/Invoke": r"\b(?:SendMessage|BroadcastMessage|SendMessageUpwards|InvokeRepeating|CancelInvoke)\s*\(|\bInvoke\s*\(\s*\"",
    "ScriptableObject": W("ScriptableObject"),
    "Coroutine": r"\b(?:StartCoroutine|StopCoroutine|StopAllCoroutines|Coroutine|WaitForSeconds|WaitForEndOfFrame|WaitForFixedUpdate|WaitForSecondsRealtime|WaitUntil|WaitWhile)\b|\byield\s+return\b",
    "Lifecycle": r"\bvoid\s+(?:Awake|Start|Update|FixedUpdate|LateUpdate|OnDestroy|OnEnable|OnDisable|OnApplicationQuit|OnApplicationFocus|OnApplicationPause|OnValidate)\s*\(\s*\)",
    "Time.": r"\bTime\.\w+",
    "Debug.": r"\bDebug\.\w+",
    # math value types
    "Vector2/3/4": W("Vector2", "Vector3", "Vector4", "Vector2Int", "Vector3Int"),
    "Quaternion": W("Quaternion"),
    "Matrix4x4": W("Matrix4x4"),
    "Mathf": r"\bMathf\.\w+",
    "Color": W("Color", "Color32"),
    "Rect/Bounds/Ray/Plane": W("Rect", "Bounds", "Ray", "Plane", "RectInt", "BoundsInt"),
    "Random": r"\bRandom\.(?:Range|value|insideUnitSphere|insideUnitCircle|onUnitSphere|rotation|seed|InitState|state)\b",
    # physics
    "Rigidbody": W("Rigidbody") + r"|\.rb\b|\brb\.",
    "Physics.": r"\bPhysics\.\w+",
    "Raycast/Query": r"\b(?:Raycast|RaycastAll|RaycastNonAlloc|SphereCast|SphereCastAll|SphereCastNonAlloc|CapsuleCast|OverlapSphere|OverlapSphereNonAlloc|OverlapBox|Linecast|ComputePenetration|ClosestPoint|RaycastHit)\b",
    "Collider": W("Collider", "BoxCollider", "SphereCollider", "CapsuleCollider", "MeshCollider", "TerrainCollider", "CharacterController"),
    "WheelCollider": W("WheelCollider", "WheelHit", "WheelFrictionCurve"),
    "Joint": W("ConfigurableJoint", "Joint", "FixedJoint", "HingeJoint", "SpringJoint", "CharacterJoint", "JointDrive", "SoftJointLimit", "SoftJointLimitSpring", "JointMotion", "ConfigurableJointMotion", "JointProjectionMode", "RotationDriveMode", "JointSpring", "JointLimits", "JointMotor"),
    "PhysicMaterial": W("PhysicMaterial", "PhysicMaterialCombine"),
    "Collision/Force": W("Collision", "ContactPoint", "CollisionDetectionMode", "RigidbodyInterpolation", "RigidbodyConstraints", "ForceMode") + r"|\bvoid\s+On(?:Collision|Trigger)(?:Enter|Stay|Exit)\s*\(",
    "LayerMask": W("LayerMask") + r"|\.layer\b",
    # rendering
    "Renderer": W("Renderer", "MeshRenderer", "SkinnedMeshRenderer", "LineRenderer", "TrailRenderer", "SpriteRenderer"),
    "Mesh": W("Mesh", "MeshFilter", "MeshTopology", "BoneWeight", "CombineInstance"),
    "Material": W("Material", "MaterialPropertyBlock") + r"|\.sharedMaterials?\b|\.materials?\b",
    "Shader": W("Shader") + r"|\bSetFloat\b|\bSetColor\b|\bSetTexture\b|\bSetVector\b|\bSetMatrix\b|\bPropertyToID\b",
    "Texture": W("Texture", "Texture2D", "Texture3D", "RenderTexture", "Cubemap", "Sprite", "TextureFormat", "RenderTextureFormat"),
    "Camera": W("Camera", "CameraClearFlags"),
    "Light": W("Light", "LightType", "LightShadows", "RenderSettings", "ReflectionProbe"),
    "Graphics/GL": r"\b(?:Graphics|GL|CommandBuffer|CameraEvent|RenderTargetIdentifier|GraphicsSettings)\b",
    "LODGroup": W("LODGroup"),
    # FX / animation / audio
    "ParticleSystem": W("ParticleSystem", "ParticleSystemRenderer", "KSPParticleEmitter", "ParticleEmitter", "ParticleAnimator", "ParticleRenderer"),
    "Animation": W("Animation", "AnimationState", "Animator", "AnimationClip", "WrapMode", "AnimationBlendMode", "RuntimeAnimatorController", "AnimatorStateInfo"),
    "AnimationCurve": W("AnimationCurve", "Keyframe"),
    "Audio": W("AudioSource", "AudioClip", "AudioListener", "AudioMixer", "AudioMixerGroup", "AudioReverbZone", "AudioLowPassFilter", "AudioHighPassFilter", "AudioRolloffMode", "AudioSettings"),
    # input
    "Input.": r"\bInput\.\w+",
    "KeyCode": W("KeyCode"),
    # UI
    "UnityEngine.UI": W("RectTransform", "Canvas", "CanvasGroup", "CanvasScaler", "GraphicRaycaster", "Image", "RawImage", "Button", "Toggle", "ToggleGroup", "Slider", "Scrollbar", "ScrollRect", "InputField", "Dropdown", "LayoutElement", "LayoutGroup", "HorizontalLayoutGroup", "VerticalLayoutGroup", "GridLayoutGroup", "ContentSizeFitter", "Selectable", "Mask", "RectMask2D", "Graphic", "MaskableGraphic", "LayoutRebuilder", "RectTransformUtility"),
    "EventSystems": W("EventSystem", "PointerEventData", "BaseEventData", "IPointerClickHandler", "IPointerEnterHandler", "IPointerExitHandler", "IPointerDownHandler", "IPointerUpHandler", "IDragHandler", "IBeginDragHandler", "IEndDragHandler", "IScrollHandler", "ISelectHandler", "IDeselectHandler", "IDropHandler", "UIBehaviour", "ExecuteEvents"),
    "TMPro": r"\b(?:TextMeshProUGUI|TextMeshPro|TMP_Text|TMP_InputField|TMP_Dropdown|TMP_FontAsset|TMP_SpriteAsset|TMP_Settings)\b",
    "IMGUI": r"\b(?:GUI|GUILayout|GUIStyle|GUISkin|GUIContent|GUIUtility|GUILayoutOption)\b|\bvoid\s+OnGUI\s*\(",
    # assets / app / platform
    "Resources.": r"\bResources\.\w+",
    "AssetBundle": W("AssetBundle", "AssetBundleRequest", "AssetBundleCreateRequest"),
    "WWW/WebRequest": W("WWW", "UnityWebRequest", "DownloadHandler", "DownloadHandlerTexture"),
    "Application.": r"\bApplication\.\w+",
    "Screen/Cursor": r"\bScreen\.\w+|\bCursor\.\w+",
    "PlayerPrefs": W("PlayerPrefs"),
    "QualitySettings/SystemInfo": W("QualitySettings", "SystemInfo"),
    "SceneManager": W("SceneManager", "LoadSceneMode"),
    "SerializeField": r"\[\s*SerializeField\b|\[\s*Serializable\b",
    "Steam": r"\b(?:Steamworks|SteamUser|SteamApps|SteamUGC|SteamManager|SteamFriends|SteamUserStats|SteamController|SteamAPI|CSteamID)\b",
    "Analytics": r"\b(?:Analytics|AnalyticsEvent|AnalyticsUtil|AnalyticsResult|KSPAnalytics)\b",
}

KSP_TYPES = ["Part", "Vessel", "CelestialBody", "Orbit", "ConfigNode", "PartModule", "FlightGlobals",
             "HighLogic", "GameEvents", "Planetarium", "ProtoVessel", "ProtoPartSnapshot",
             "ProtoPartModuleSnapshot", "ProtoCrewMember", "PartResource", "PartResourceDefinition",
             "ScenarioModule", "Localizer", "Contract", "ContractParameter", "ScienceSubject",
             "ResearchAndDevelopment", "Funding", "Reputation", "GameSettings", "InputLockManager",
             "KSPField", "KSPEvent", "KSPAction", "PQS", "TimeWarp", "EditorLogic", "ShipConstruct",
             "FlightIntegrator", "PhysicsGlobals", "PartJoint", "AttachNode", "VesselModule",
             "CommNetVessel", "KerbalEVA", "FloatCurve", "Vector3d", "QuaternionD", "UIPartActionWindow",
             "GameDatabase", "PartLoader", "AvailablePart", "Game", "MissionSystem", "FlightCamera",
             "MapView", "ScaledSpace", "OrbitDriver", "Krakensbane", "FloatingOrigin", "KerbalRoster"]

TYPE_DECL = re.compile(r"^(?P<indent>[ \t]*)(?P<attrs>(?:\[[^\]\n]*\]\s*)*)(?P<mods>(?:(?:public|private|protected|internal|static|abstract|sealed|partial|unsafe|new|readonly|ref)\s+)*)(?P<kind>class|struct|interface|enum|delegate)\s+(?P<rest>[^\n]*)$", re.M)
NAMESPACE = re.compile(r"^\s*namespace\s+([\w\.]+)", re.M)
USING = re.compile(r"^\s*using\s+(?:static\s+)?([\w\.]+)\s*;", re.M)
ATTR_ADDON = re.compile(r"\[\s*KSPAddon\s*\(")
ATTR_SCEN = re.compile(r"\[\s*KSPScenario\s*\(")
METHOD = re.compile(r"^[ \t]+(?:(?:public|private|protected|internal|static|virtual|override|abstract|sealed|new|extern|unsafe|async)\s+)+[\w<>\[\],\.\? ]+?\s+(\w+)\s*(?:<[^>\n]*>)?\s*\(", re.M)
IDENT = re.compile(r"\b[A-Za-z_]\w*\b")


def parse_decl(rest, kind):
    if kind == "delegate":
        m = re.match(r"[\w<>\[\],\.\s\?]+?\s+(\w+)\s*(<[^>]*>)?\s*\(", rest)
        return (m.group(1) if m else "?"), "", []
    m = re.match(r"(\w+)\s*(<[^{:]*?>)?\s*(?::\s*([^{]*?))?\s*(?:where\b.*)?\s*\{?\s*$", rest)
    if not m:
        m2 = re.match(r"(\w+)", rest)
        return (m2.group(1) if m2 else "?"), "", []
    bases = []
    if m.group(3):
        depth, cur = 0, ""
        for ch in m.group(3):
            if ch == "<":
                depth += 1
            elif ch == ">":
                depth -= 1
            if ch == "," and depth == 0:
                bases.append(cur.strip())
                cur = ""
            else:
                cur += ch
        if cur.strip():
            bases.append(cur.strip())
        bases = [re.sub(r"\s+", "", b) for b in bases]
    return m.group(1), (m.group(2) or ""), bases


def analyze(relpath, fullpath):
    with open(fullpath, "r", encoding="utf-8-sig", errors="replace") as fh:
        src = fh.read()
    lines = src.count("\n")
    code = strip_code(src)
    usings = sorted(set(USING.findall(code)))
    namespaces = NAMESPACE.findall(code)
    ns = namespaces[0] if namespaces else ""
    ns_depth = 1 if namespaces else 0
    line_depth, depth = [], 0
    for ln in code.split("\n"):
        line_depth.append(depth)
        depth += ln.count("{") - ln.count("}")
    starts = [0] + [i + 1 for i, ch in enumerate(code) if ch == "\n"]
    decls = []
    for m in TYPE_DECL.finditer(code):
        lineno = bisect.bisect_right(starts, m.start("kind")) - 1
        kind = m.group("kind")
        name, gen, bases = parse_decl(m.group("rest"), kind)
        decls.append({"kind": kind, "name": name, "generic": gen, "bases": bases,
                      "mods": m.group("mods").split(), "depth": line_depth[lineno],
                      "attrs": m.group("attrs").strip()})
    top = [d for d in decls if d["depth"] == ns_depth]
    stem = os.path.splitext(os.path.basename(relpath))[0].split("`")[0]
    primary = next((d for d in top if d["name"] == stem), None) or (top[0] if top else (decls[0] if decls else None))
    api = {g: len(re.findall(pat, code)) for g, pat in API_GROUPS.items()}
    idents = Counter(IDENT.findall(code))
    ksp = {t: idents.get(t, 0) for t in KSP_TYPES}
    methods = METHOD.findall(code)
    return {"path": relpath, "lines": lines, "namespace": ns, "usings": usings, "decls": decls,
            "primary": primary, "addon": bool(ATTR_ADDON.search(code)),
            "scenario_attr": bool(ATTR_SCEN.search(code)), "api": api, "ksp": ksp,
            "methods": methods, "idents": {k: v for k, v in idents.items() if k[:1].isupper()}}


def main():
    os.makedirs(OUT, exist_ok=True)
    rows = [analyze(rel, full) for rel, full in iter_files()]
    rows.sort(key=lambda r: r["path"].lower())
    # inheritance closure by simple name (first base in list)
    base_of = {}
    for r in rows:
        for d in r["decls"]:
            if d["kind"] in ("class", "struct") and d["bases"]:
                base_of.setdefault(d["name"], re.sub(r"<.*", "", d["bases"][0]).split(".")[-1])
    for r in rows:
        name, seen, chain = (r["primary"] or {}).get("name"), set(), []
        while name in base_of and name not in seen:
            seen.add(name)
            name = base_of[name]
            chain.append(name)
        r["ancestry"] = chain
    with open(os.path.join(OUT, "features.jsonl"), "w", encoding="utf-8") as fh:
        for r in rows:
            fh.write(json.dumps(r) + "\n")
    print(len(rows), "files,", sum(r["lines"] for r in rows), "lines")


if __name__ == "__main__":
    main()
