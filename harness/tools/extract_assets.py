"""Extract KSP's *serialized* (prefab) values for the celestial bodies and PhysicsGlobals from the
shipped Unity asset files, using type trees generated from KSP's own managed assemblies.

These are the values Unity deserializes into the CelestialBody / OrbitDriver / PSystemBody /
PhysicsGlobals MonoBehaviours before any KSP code runs (Awake/Start then derive Mass, gravParameter,
SOI, ...; that derivation is done by the C# harness calling KSP's code).

Usage: python -I extract_assets.py <KSP_x64_Data dir> <harness lib dir> <out dir>
Requires (in harness/tools-cache/venv): UnityPy, TypeTreeGeneratorAPI.

Sources (verified by scanning every level*/sharedassets* file for MonoBehaviours by script class):
  sharedassets9.assets : PSystem (1), PSystemBody (17), CelestialBody (17), OrbitDriver (16)
  level9               : PSystemManager (references the PSystem prefab), Planetarium, FlightGlobals
  level0               : PhysicsGlobals
  globalgamemanagers.assets : MonoScript table
Assembly used for type trees: harness/lib/Assembly-CSharp.de4dot-original.dll (the shipped IL,
de4dot-cleaned, names preserved) + Unity/firstpass DLLs from harness/lib + Mono BCL from Managed/.
"""
import hashlib
import json
import math
import os
import sys

import UnityPy
from UnityPy.helpers.TypeTreeGenerator import TypeTreeGenerator

UNITY_VERSION = "2019.4.18f1"
BCL = ["mscorlib.dll", "System.dll", "System.Core.dll", "System.Xml.dll", "Mono.Security.dll",
       "System.Configuration.dll", "System.Security.dll"]


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def clean(v):
    """Make the parsed tree JSON-safe; keep floats exact (Python floats are doubles; float32 fields
    are widened exactly). Non-finite floats become strings."""
    if isinstance(v, dict):
        return {k: clean(x) for k, x in v.items()}
    if isinstance(v, list):
        return [clean(x) for x in v]
    if isinstance(v, float):
        if math.isnan(v):
            return "NaN"
        if math.isinf(v):
            return "Infinity" if v > 0 else "-Infinity"
        return v
    if isinstance(v, (bytes, bytearray)):
        return v.hex()
    return v


def main():
    data_dir, lib_dir, out_dir = sys.argv[1:4]
    os.makedirs(out_dir, exist_ok=True)
    gen = TypeTreeGenerator(UNITY_VERSION)
    loaded = []
    for f in sorted(os.listdir(lib_dir)):
        if not f.endswith(".dll") or f == "Assembly-CSharp.dll":
            continue  # the rebuilt DLL is skipped; the shipped (de4dot) IL defines the layout
        gen.load_dll(open(os.path.join(lib_dir, f), "rb").read())
        loaded.append(f)
    for f in BCL:
        gen.load_dll(open(os.path.join(data_dir, "Managed", f), "rb").read())
        loaded.append("Managed/" + f)

    files = ["globalgamemanagers.assets", "sharedassets9.assets", "level9", "level0"]
    env = UnityPy.load(*[os.path.join(data_dir, f) for f in files])
    env.typetree_generator = gen

    scripts = {}
    by_file = {}
    for name, f in env.files.items():
        by_file[os.path.basename(name)] = f
    for obj in by_file["globalgamemanagers.assets"].objects.values():
        if obj.type.name == "MonoScript":
            s = obj.read()
            scripts[obj.path_id] = (s.m_Namespace + "." if s.m_Namespace else "") + s.m_ClassName

    def classify(f):
        out = {}
        for obj in f.objects.values():
            if obj.type.name != "MonoBehaviour":
                continue
            mb = obj.parse_monobehaviour_head()
            cls = scripts.get(mb.m_Script.m_PathID, "?")
            out.setdefault(cls, []).append(obj)
        return out

    sa9 = classify(by_file["sharedassets9.assets"])
    lvl0 = classify(by_file["level0"])
    lvl9 = classify(by_file["level9"])

    # GameObject names (for cross-referencing components)
    go_names = {}
    for obj in by_file["sharedassets9.assets"].objects.values():
        if obj.type.name == "GameObject":
            go_names[obj.path_id] = obj.read().m_Name

    def tree(obj):
        return obj.read_typetree()

    bodies = {}
    for obj in sa9["CelestialBody"]:
        t = tree(obj)
        go = t["m_GameObject"]["m_PathID"]
        t = {k: v for k, v in t.items() if k not in ("m_Script",)}
        t["_gameObject"] = go_names.get(go)
        t["_pathID"] = obj.path_id
        bodies[obj.path_id] = t
    orbit_drivers = {}
    for obj in sa9["OrbitDriver"]:
        t = tree(obj)
        orbit_drivers[t["m_GameObject"]["m_PathID"]] = {"_pathID": obj.path_id, "orbit": t.get("orbit"),
                                                         "reverse": t.get("reverse"), "updateMode": t.get("updateMode"),
                                                         "celestialBody": t.get("celestialBody")}
    psb = {}
    for obj in sa9["PSystemBody"]:
        t = tree(obj)
        psb[obj.path_id] = t
    psystem = tree(sa9["PSystem"][0])

    # Walk the PSystem tree from rootBody, as PSystemManager.SpawnBody does (depth-first, children order).
    ordered = []

    def walk(psb_id, parent_name, depth):
        p = psb[psb_id]
        cb_id = p["celestialBody"]["m_PathID"]
        cb = bodies[cb_id]
        go = cb["m_GameObject"]["m_PathID"]
        od = orbit_drivers.get(go)
        entry = {
            "spawnOrder": len(ordered),
            "depth": depth,
            "parent": parent_name,
            "flightGlobalsIndex": p.get("flightGlobalsIndex"),
            "planetariumCameraInitial": p.get("planetariumCameraInitial"),
            "celestialBody": cb,
            "orbitDriver": od,
        }
        ordered.append(entry)
        for ch in p.get("children", []):
            walk(ch["m_PathID"], cb["bodyName"], depth + 1)

    walk(psystem["rootBody"]["m_PathID"], None, 0)

    provenance = {
        "unity_version": UNITY_VERSION,
        "asset_files": {f: sha256(os.path.join(data_dir, f)) for f in files},
        "typetree_assemblies": loaded,
        "assembly_csharp_for_layout_sha256": sha256(os.path.join(lib_dir, "Assembly-CSharp.de4dot-original.dll")),
        "tool": "UnityPy %s + TypeTreeGeneratorAPI" % UnityPy.__version__,
    }
    with open(os.path.join(out_dir, "celestial_bodies.serialized.json"), "w", encoding="utf-8") as f:
        json.dump(clean({"provenance": provenance, "systemName": psystem.get("systemName"),
                         "systemScale": psystem.get("systemScale"), "systemTimeScale": psystem.get("systemTimeScale"),
                         "bodies": ordered}), f, indent=1)

    pg = tree(lvl0["PhysicsGlobals"][0])
    pg = {k: v for k, v in pg.items() if k not in ("m_Script",)}
    with open(os.path.join(out_dir, "physicsglobals.serialized.json"), "w", encoding="utf-8") as f:
        json.dump(clean({"provenance": provenance, "PhysicsGlobals": pg}), f, indent=1)

    extra = {}
    for cls in ("Planetarium", "FlightGlobals", "PSystemManager"):
        if cls in lvl9:
            t = tree(lvl9[cls][0])
            extra[cls] = {k: v for k, v in t.items() if k not in ("m_Script",)}
    with open(os.path.join(out_dir, "scene_singletons.serialized.json"), "w", encoding="utf-8") as f:
        json.dump(clean({"provenance": provenance, **extra}), f, indent=1)
    print("bodies:", len(ordered), [b["celestialBody"]["bodyName"] for b in ordered])


if __name__ == "__main__":
    main()
