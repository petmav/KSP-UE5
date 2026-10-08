"""Index the game's own celestial-body assets (biome maps, mini-biomes, PQS height/colour maps, scaled-space meshes,
materials and textures) in KSP's shipped Unity files, for kspcore's Kerbol-system catalog.

    <harness>/tools-cache/venv/Scripts/python.exe -I extract_body_assets.py <KSP_x64_Data dir> <harness lib dir> <out.json>

e.g. (from core/):
    ..\\harness\\tools-cache\\venv\\Scripts\\python.exe -I tools\\extract_body_assets.py ^
        ..\\decompiled\\KSP-dev\\KSP_x64_Data ..\\harness\\lib data\\kerbol_body_assets.json

Read-only on the game files. Same method as harness/tools/extract_assets.py (UnityPy + type trees generated from KSP's
shipped IL): the player build has no type trees. Every asset is recorded by origin (asset file relative to the game
root + Unity PathID + object name), so later packages decode the real data (biome bitmap = CBAttributeMapSO._data,
height/colour maps = MapSO._data, scaled-space textures = Texture2D in sharedassets9.assets(.resS)) instead of
substitutes. Biome attribute tables (name, localization tag, map colour, value) and MapSO headers are copied verbatim
because the catalog consumes them directly. Nothing here is distributed.
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
SA9 = "sharedassets9.assets"
REF_TYPES = {"Texture2D", "Texture2DArray", "Texture3D", "Cubemap", "Material", "Mesh", "MonoBehaviour"}
MAP_CLASSES = {"MapSO", "CBAttributeMapSO", "CBTextureAtlasSO"}


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 22), b""):
            h.update(chunk)
    return h.hexdigest()


def clean(v):
    if isinstance(v, float):
        if math.isnan(v):
            return "NaN"
        if math.isinf(v):
            return "Infinity" if v > 0 else "-Infinity"
        return v
    if isinstance(v, dict):
        return {k: clean(x) for k, x in v.items()}
    if isinstance(v, (list, tuple)):
        return [clean(x) for x in v]
    if isinstance(v, (bytes, bytearray)):
        return {"bytes": len(v)}
    return v


def main():
    data_dir, lib_dir, out_path = sys.argv[1:4]
    gen = TypeTreeGenerator(UNITY_VERSION)
    for f in sorted(os.listdir(lib_dir)):
        if f.endswith(".dll") and f != "Assembly-CSharp.dll":
            gen.load_dll(open(os.path.join(lib_dir, f), "rb").read())
    for f in BCL:
        gen.load_dll(open(os.path.join(data_dir, "Managed", f), "rb").read())
    first = UnityPy.load(os.path.join(data_dir, SA9))
    externals = [e.path for f in first.files.values() for e in f.externals]
    load = ["globalgamemanagers.assets", SA9] + [e for e in externals if e.endswith(".assets") and e != "globalgamemanagers.assets"]
    env = UnityPy.load(*[os.path.join(data_dir, f) for f in load])
    env.typetree_generator = gen
    files = {os.path.basename(n): f for n, f in env.files.items() if os.path.basename(n) in load}
    sa9 = files[SA9]
    objs = sa9.objects
    scripts = {}
    for obj in files["globalgamemanagers.assets"].objects.values():
        if obj.type.name == "MonoScript":
            s = obj.read()
            scripts[obj.path_id] = (s.m_Namespace + "." if s.m_Namespace else "") + s.m_ClassName

    def mb_class(o):
        return scripts.get(o.parse_monobehaviour_head().m_Script.m_PathID, "?")

    def target_file(pptr, src=SA9):
        fid = pptr["m_FileID"]
        return src if fid == 0 else [e.path for e in files[src].externals][fid - 1]

    def origin(pptr, src=SA9):
        f = target_file(pptr, src)
        return {"file": "KSP_x64_Data/" + f if not f.startswith(("Resources/", "Library/")) else f, "pathID": pptr["m_PathID"]}

    def resolve(pptr, src=SA9):
        f = target_file(pptr, src)
        if f not in files or pptr["m_PathID"] not in files[f].objects:
            return f, None
        return f, files[f].objects[pptr["m_PathID"]]

    def describe(pptr, src=SA9):
        """origin + type + name (+ size/format for textures, header for MapSO) of a PPtr target (any loaded file)."""
        d = origin(pptr, src)
        f, o = resolve(pptr, src)
        if o is None:
            return d
        d["type"] = o.type.name
        if o.type.name == "MonoBehaviour":
            d["type"] = mb_class(o)
            t = o.read_typetree()
            d["name"] = t.get("m_Name", "")
            if d["type"] in MAP_CLASSES:
                d["map"] = {k: t.get(k) for k in ("_name", "_width", "_height", "_bpp", "_rowWidth", "_isCompiled")}
                d["map"]["dataBytes"] = len(t.get("_data") or b"")
        elif o.type.name in ("Texture2D", "Cubemap", "Texture3D", "Texture2DArray"):
            t = o.read_typetree()
            d["name"] = t.get("m_Name", "")
            d["width"], d["height"] = t.get("m_Width"), t.get("m_Height")
            d["format"] = t.get("m_TextureFormat")
            sd = t.get("m_StreamData") or {}
            if sd.get("path"):
                d["stream"] = {"path": "KSP_x64_Data/" + sd["path"].replace("archive:/", ""), "offset": sd.get("offset"), "size": sd.get("size")}
        elif o.type.name == "Shader":
            d["name"] = o.read().m_ParsedForm.m_Name if hasattr(o.read(), "m_ParsedForm") else ""
        else:
            t = o.read_typetree()
            d["name"] = t.get("m_Name", "")
        return d

    def material(pptr, src=SA9):
        d = describe(pptr, src)
        if d.get("type") != "Material":
            return d
        f, o = resolve(pptr, src)
        t = o.read_typetree()
        sh = t.get("m_Shader")
        if sh and sh["m_PathID"]:
            d["shader"] = describe(sh, f)
        tex = []
        for name, env_ in (t.get("m_SavedProperties", {}).get("m_TexEnvs") or []):
            p = env_.get("m_Texture")
            if p and p["m_PathID"]:
                tex.append({"property": name, **describe(p, f)})
        d["textures"] = tex
        return d

    def pptrs(v, path=""):
        """every non-null PPtr inside a type tree, with its field path"""
        if isinstance(v, dict):
            if set(v.keys()) == {"m_FileID", "m_PathID"}:
                if v["m_PathID"]:
                    yield path, v
                return
            for k, x in v.items():
                if k in ("m_GameObject", "m_Script"):
                    continue
                yield from pptrs(x, f"{path}.{k}" if path else k)
        elif isinstance(v, list):
            for i, x in enumerate(v):
                yield from pptrs(x, f"{path}[{i}]")

    def components(go_id):
        t = objs[go_id].read_typetree()
        return t.get("m_Name", ""), [c["component"]["m_PathID"] for c in t["m_Component"]]

    def children(go_id):
        _, comps = components(go_id)
        for cid in comps:
            o = objs[cid]
            if o.type.name == "Transform":
                for ch in o.read_typetree()["m_Children"]:
                    yield objs[ch["m_PathID"]].read_typetree()["m_GameObject"]["m_PathID"]

    def walk_pqs(go_id, path, out):
        name, comps = components(go_id)
        path = f"{path}/{name}" if path else name
        for cid in comps:
            o = objs[cid]
            if o.type.name != "MonoBehaviour":
                continue
            c = mb_class(o)
            t = o.read_typetree()
            refs = []
            for fpath, p in pptrs(t):
                d = describe(p)
                if d.get("type") in MAP_CLASSES or d.get("type") in REF_TYPES - {"MonoBehaviour"}:
                    if d.get("type") == "Material":
                        d = material(p)
                    refs.append({"field": fpath, **d})
            if refs:
                out.append({"gameObject": path, "component": c, "pathID": cid, "refs": refs})
        for ch in children(go_id):
            walk_pqs(ch, path, out)

    psb = {}
    cbs = {}
    for o in objs.values():
        if o.type.name == "MonoBehaviour":
            c = mb_class(o)
            if c == "PSystemBody":
                psb[o.path_id] = o.read_typetree()
            elif c == "CelestialBody":
                cbs[o.path_id] = o.read_typetree()

    bodies = []
    for pid, p in sorted(psb.items(), key=lambda kv: kv[1]["flightGlobalsIndex"]):
        cb = cbs[p["celestialBody"]["m_PathID"]]
        e = {"bodyName": cb["bodyName"], "psystemBody": origin({"m_FileID": 0, "m_PathID": pid}),
             "celestialBody": origin(p["celestialBody"])}
        bm = cb.get("BiomeMap")
        if bm and bm["m_PathID"]:
            d = describe(bm)
            t = objs[bm["m_PathID"]].read_typetree()
            d["exactSearch"] = t["exactSearch"]
            d["nonExactThreshold"] = t["nonExactThreshold"]
            d["neighborColorThresh"] = t["neighborColorThresh"]
            d["attributes"] = [{"name": a["name"], "localizationTag": a["localizationTag"], "value": a["value"],
                                "mapColor": a["mapColor"]} for a in t["Attributes"]]
            e["biomeMap"] = d
        e["miniBiomes"] = []
        for mbp in cb.get("MiniBiomes") or []:
            if not mbp["m_PathID"]:
                continue
            t = objs[mbp["m_PathID"]].read_typetree()
            go = t["m_GameObject"]["m_PathID"]
            e["miniBiomes"].append({**origin(mbp), "name": t.get("m_Name", ""), "gameObject": objs[go].read_typetree()["m_Name"] if go else "",
                                    "TagKey": t["TagKey"], "LocalizedTag": t["LocalizedTag"], "TagKeyID": t["TagKeyID"]})
        sv = p.get("scaledVersion")
        if sv and sv["m_PathID"]:
            name, comps = components(sv["m_PathID"])
            s = {**origin(sv), "name": name, "mesh": None, "materials": []}
            for cid in comps:
                o = objs[cid]
                t = o.read_typetree()
                if o.type.name == "MeshFilter" and t["m_Mesh"]["m_PathID"]:
                    s["mesh"] = describe(t["m_Mesh"])
                elif o.type.name == "MeshRenderer":
                    s["materials"] = [material(m) for m in t["m_Materials"] if m["m_PathID"]]
            e["scaledVersion"] = s
        pq = p.get("pqsVersion")
        if pq and pq["m_PathID"]:
            t = objs[pq["m_PathID"]].read_typetree()
            go = t["m_GameObject"]["m_PathID"]
            q = {**origin(pq), "gameObject": objs[go].read_typetree()["m_Name"], "radius": t.get("radius"),
                 "mapFilename": t.get("mapFilename"), "mapMaxHeight": t.get("mapMaxHeight"), "mapOcean": t.get("mapOcean"),
                 "mapOceanHeight": t.get("mapOceanHeight"), "components": []}
            walk_pqs(go, "", q["components"])
            e["pqs"] = q
        bodies.append(e)

    out = {"provenance": {"unity_version": UNITY_VERSION, "tool": "core/tools/extract_body_assets.py, UnityPy %s" % UnityPy.__version__,
                          "game_root": "decompiled/KSP-dev",
                          "asset_files": {f: sha256(os.path.join(data_dir, f)) for f in load},
                          "files_loaded": load},
           "bodies": bodies}
    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(clean(out), f, indent=1)
    print("bodies:", len(bodies), [b["bodyName"] for b in bodies])


if __name__ == "__main__":
    main()
