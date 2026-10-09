"""Unity API surface used by KSP-authored code (shim spec input).

Run: python -I unity_surface.py      (after extract_features.py; uses ../tiering.csv for tier splits if present)
Writes work/unity_types.json, work/unity_static_members.json, work/unity_instance_members.json,
work/unity_lifecycle.json
Counting rules:
- Unity type catalog = every .cs file stem under decompiled Unity/UnityEngine*/ (game copies), with its namespace.
- A KSP file can see a Unity type if it imports that namespace (or lives inside it). Identifiers are counted after
  comments/strings are stripped (extract_features.strip_code).
- Unity type names that KSP itself declares (e.g. Wheel, Tooltip) are excluded to avoid false hits.
- Third-party embedded code (TMPro, Smooth, VehiclePhysics, ...) is excluded from the counts.
"""
import csv, json, os, re, sys
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from extract_features import strip_code, iter_files  # noqa: E402

UNITY = r"C:/Projects/KSP-UE5/decompiled/KSP-Decompiled/Unity"
WORK = r"C:/Projects/KSP-UE5/analysis/tiering/work"
TIERING = r"C:/Projects/KSP-UE5/analysis/tiering/tiering.csv"
THIRD = ("TMPro/", "Smooth/", "VehiclePhysics/", "EdyCommonTools/", "LibNoise/", "SoftMasking/", "UniLinq/",
         "DDSHeaders/", "Highlighting/")
NS_RE = re.compile(r"^\s*namespace\s+([\w\.]+)", re.M)
USING = re.compile(r"^\s*using\s+([\w\.]+)\s*;", re.M)
MEMBER_RE = re.compile(r"^\s*public\s+(?:static\s+|extern\s+|override\s+|virtual\s+|new\s+|unsafe\s+|readonly\s+)*"
                       r"(?:[\w<>\[\],\.\?]+\s+)+?(\w+)\s*(?:[\({;=]|$)", re.M)
STATIC_HOSTS = ["Physics", "Mathf", "Time", "Input", "Debug", "Application", "Screen", "Cursor", "GUI", "GUILayout",
                "GUIUtility", "Resources", "Graphics", "GL", "Shader", "QualitySettings", "SystemInfo", "PlayerPrefs",
                "Random", "Gizmos", "Object", "GameObject", "Vector3", "Quaternion", "Vector2", "Color", "Matrix4x4",
                "RenderSettings", "AudioSettings", "SceneManager", "LayerMask", "Camera", "RectTransformUtility",
                "EventSystem", "ExecuteEvents", "LayoutRebuilder", "JsonUtility", "AssetBundle", "Profiler",
                "Physics2D", "AnimationCurve", "Mesh", "Texture2D", "Sprite", "RenderTexture", "Light"]
INSTANCE_FOCUS = ["Rigidbody", "Transform", "GameObject", "ConfigurableJoint", "Joint", "Collider", "WheelCollider",
                  "Renderer", "Material", "Animation", "AnimationState", "AudioSource", "Camera", "MonoBehaviour",
                  "Mesh", "MeshFilter", "ParticleSystem", "Light", "RectTransform"]
LIFECYCLE = ["Awake", "Start", "Update", "FixedUpdate", "LateUpdate", "OnDestroy", "OnEnable", "OnDisable",
             "OnGUI", "OnApplicationQuit", "OnApplicationFocus", "OnApplicationPause", "OnValidate", "Reset",
             "OnCollisionEnter", "OnCollisionStay", "OnCollisionExit", "OnTriggerEnter", "OnTriggerStay",
             "OnTriggerExit", "OnMouseDown", "OnMouseUp", "OnMouseEnter", "OnMouseExit", "OnMouseOver",
             "OnMouseDrag", "OnBecameVisible", "OnBecameInvisible", "OnWillRenderObject", "OnRenderObject",
             "OnPreCull", "OnPreRender", "OnPostRender", "OnRenderImage", "OnDrawGizmos", "OnDrawGizmosSelected",
             "OnJointBreak", "OnLevelWasLoaded", "OnRectTransformDimensionsChange", "OnTransformParentChanged",
             "OnAnimatorMove", "OnParticleCollision", "OnAudioFilterRead"]


def catalog():
    types = {}
    members = defaultdict(set)
    for mod in os.listdir(UNITY):
        if not (mod.startswith("UnityEngine") or mod in ("Unity.Timeline",)):
            continue
        for dp, dn, fn in os.walk(os.path.join(UNITY, mod)):
            for f in fn:
                if not f.endswith(".cs"):
                    continue
                stem = f[:-3].split("`")[0]
                if stem in ("AssemblyInfo",) or stem.startswith("_"):
                    continue
                src = open(os.path.join(dp, f), encoding="utf-8-sig", errors="replace").read()
                m = NS_RE.search(src)
                ns = m.group(1) if m else ""
                if not ns.startswith("UnityEngine"):
                    continue
                types.setdefault(stem, (ns, mod))
                for mm in MEMBER_RE.findall(src):
                    if mm != stem:
                        members[stem].add(mm)
    return types, members


def main():
    types, members = catalog()
    tiers = {}
    if os.path.exists(TIERING):
        for row in csv.DictReader(open(TIERING, encoding="utf-8")):
            tiers[row["path"]] = row["tier"]
    # KSP-declared names that shadow Unity names
    feats = [json.loads(l) for l in open(os.path.join(WORK, "features.jsonl"), encoding="utf-8")]
    ksp_names = {d["name"] for r in feats for d in r["decls"]}
    shadow = sorted(n for n in types if n in ksp_names)
    type_files = defaultdict(set)
    type_refs = Counter()
    type_tier_files = defaultdict(Counter)
    static_members = defaultdict(Counter)
    inst_members = defaultdict(Counter)
    lifecycle = Counter()
    lifecycle_files = defaultdict(set)
    ident_re = re.compile(r"\b[A-Z]\w*\b")
    nfiles = 0
    for rel, full in iter_files():
        sub = rel.split("/", 1)[1]
        if rel.startswith("Assembly-CSharp/") and sub.startswith(THIRD):
            continue
        nfiles += 1
        code = strip_code(open(full, encoding="utf-8-sig", errors="replace").read())
        usings = set(USING.findall(code))
        nsm = NS_RE.search(code)
        own_ns = nsm.group(1) if nsm else ""
        visible_ns = usings | ({own_ns} if own_ns.startswith("UnityEngine") else set())
        tier = tiers.get(rel, "?")
        idents = Counter(ident_re.findall(code))
        for name, cnt in idents.items():
            t = types.get(name)
            if not t or name in ksp_names:
                continue
            if t[0] not in visible_ns and t[0] != "UnityEngine":
                continue
            if t[0] == "UnityEngine" and "UnityEngine" not in visible_ns:
                continue
            type_files[name].add(rel)
            type_refs[name] += cnt
            type_tier_files[name][tier] += 1
        if "UnityEngine" in visible_ns or own_ns.startswith("UnityEngine"):
            for host in STATIC_HOSTS:
                for m in re.findall(r"(?<![\w\.])" + host + r"\.([A-Za-z_]\w*)", code):
                    if host in ksp_names:
                        break
                    static_members[host][m] += 1
            for t in INSTANCE_FOCUS:
                if t not in idents or not members.get(t):
                    continue
                for m in re.findall(r"\.([A-Za-z_]\w*)", code):
                    if m in members[t]:
                        inst_members[t][m] += 1
        for msg in LIFECYCLE:
            n = len(re.findall(r"\bvoid\s+" + msg + r"\s*\(", code))
            if n:
                lifecycle[msg] += n
                lifecycle_files[msg].add(rel)
    out_types = []
    for name, fs in type_files.items():
        ns, mod = types[name]
        out_types.append({"type": name, "namespace": ns, "module": mod, "files": len(fs), "refs": type_refs[name],
                          "tiers": dict(type_tier_files[name])})
    out_types.sort(key=lambda d: (-d["files"], -d["refs"]))
    json.dump({"files_scanned": nfiles, "shadowed": shadow, "types": out_types},
              open(os.path.join(WORK, "unity_types.json"), "w", encoding="utf-8"), indent=1)
    json.dump({h: c.most_common() for h, c in static_members.items()},
              open(os.path.join(WORK, "unity_static_members.json"), "w", encoding="utf-8"), indent=1)
    json.dump({h: c.most_common(40) for h, c in inst_members.items()},
              open(os.path.join(WORK, "unity_instance_members.json"), "w", encoding="utf-8"), indent=1)
    json.dump({m: {"defs": lifecycle[m], "files": len(lifecycle_files[m])} for m in LIFECYCLE if lifecycle[m]},
              open(os.path.join(WORK, "unity_lifecycle.json"), "w", encoding="utf-8"), indent=1)
    print("files scanned", nfiles, "unity types used", len(out_types), "shadowed", shadow)
    for d in out_types[:60]:
        print(d["files"], d["refs"], d["type"], d["module"], d["tiers"])


if __name__ == "__main__":
    main()
