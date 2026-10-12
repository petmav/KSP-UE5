"""WP-11: extract the PQS mod stacks (every PQSMod's serialized fields, in PQS.GetChildMods order) and the original
height-map / decal MapSO pixel arrays of every Kerbol-system body from KSP's shipped Unity assets.

    <harness>/tools-cache/venv/Scripts/python.exe -I tools/wp11/extract_terrain.py ^
        <KSP_x64_Data dir> <harness lib dir> <decompiled Assembly-CSharp dir> <out dir>

e.g. (from core/):
    ..\\harness\\tools-cache\\venv\\Scripts\\python.exe -I tools\\wp11\\extract_terrain.py ^
        ..\\decompiled\\KSP-dev\\KSP_x64_Data ..\\harness\\lib ..\\decompiled\\KSP-Decompiled\\Assembly-CSharp data\\terrain

Read-only on the game files; same loading method as tools/extract_body_assets.py (WP-09: UnityPy + type trees generated
from KSP's shipped IL, sharedassets9.assets with its externals). Writes:

  <out>/pqs_prefab.json   per body: the PQS component (radius, ...), the CelestialBody fields terrain needs, and every
                          PQSMod under the PQS GameObject in the order PQS.GetChildMods (PQS.cs:2161) collects them
                          (depth-first over Transform.m_Children, components in m_Component order, children carrying
                          their own PQS skipped), with the GameObject path, activeSelf and the complete serialized
                          field tree (MapSO references replaced by {"map": <map key>}).
  <out>/maps/<key>.bin    MapSO._data verbatim (the bytes MapSO.GetPixel* index: PixelIndex = x*_bpp + y*_rowWidth)
                          for every MapSO referenced by a mod that overrides OnVertexBuildHeight (height-affecting).
  <out>/manifest.json     one entry per map: origin asset file + PathID, Unity object name, MapSO header (_name,
                          _width, _height, _bpp, _rowWidth), format, byte size, sha256 of the .bin, consumers.

Nothing here is distributed (CLAUDE.md "Original game assets").
"""
import hashlib
import json
import math
import os
import re
import sys

import UnityPy
from UnityPy.helpers.TypeTreeGenerator import TypeTreeGenerator

UNITY_VERSION = "2019.4.18f1"
BCL = ["mscorlib.dll", "System.dll", "System.Core.dll", "System.Xml.dll", "Mono.Security.dll",
       "System.Configuration.dll", "System.Security.dll"]
SA9 = "sharedassets9.assets"
MAP_CLASSES = {"MapSO", "CBAttributeMapSO", "CBTextureAtlasSO"}
PQS_FIELDS = ["radius", "maxLevel", "minLevel", "maxQTAngle", "subdivisionThresholds", "collapseThresholds",
              "mapFilename", "mapMaxHeight", "mapOcean", "mapOceanHeight", "mapOceanColor", "isFakeBuild",
              "buildTangents", "parentSphere",
              # WP-12a: the LOD fields the quadtree reads (PQS.cs:188-260); not in the WP-11 extraction yet, so the
              # generated stacks use PQS.Reset() defaults until this script is re-run locally on KSP-dev
              "seed", "frameTimeDelta", "maxFrameTime", "maxQuadLenghtsPerFrame", "minDetailDistance",
              "maxDetailDistance", "subdivisionThreshold", "collapseSeaLevelValue", "collapseAltitudeValue",
              "collapseAltitudeMax", "visRadSeaLevelValue", "visRadAltitudeValue", "visRadAltitudeMax",
              "detailSeaLevelQuads", "detailAltitudeQuads", "detailAltitudeMax", "surfaceRelativeQuads",
              "meshCastShadows", "meshRecieveShadows"]
CB_FIELDS = ["bodyName", "Radius", "ocean", "pqsController", "Density", "GeeASL"]


def sha256_file(path):
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


def pqsmod_classes(cs_dir):
    """every class deriving (transitively) from PQSMod, and the subset that overrides OnVertexBuildHeight"""
    base, height = {}, set()
    for root, _, fs in os.walk(cs_dir):
        for f in fs:
            if not f.endswith(".cs"):
                continue
            s = open(os.path.join(root, f), encoding="utf-8", errors="replace").read()
            for m in re.finditer(r"\bclass\s+(\w+)\s*:\s*([\w\.]+)", s):
                base[m.group(1)] = m.group(2).split(".")[-1]
            if "override void OnVertexBuildHeight" in s:
                for m in re.finditer(r"\bclass\s+(\w+)\s*:", s):
                    height.add(m.group(1))

    def is_mod(c):
        for _ in range(32):
            if c == "PQSMod":
                return True
            if c not in base:
                return False
            c = base[c]
        return False
    mods = {c for c in base if is_mod(c)} | {"PQSMod"}
    return mods, height & mods


def main():
    data_dir, lib_dir, cs_dir, out_dir = sys.argv[1:5]
    mod_classes, height_classes = pqsmod_classes(cs_dir)
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
    objs = files[SA9].objects
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

    def resolve(pptr, src=SA9):
        f = target_file(pptr, src)
        if f not in files or pptr["m_PathID"] not in files[f].objects:
            return f, None
        return f, files[f].objects[pptr["m_PathID"]]

    maps = {}          # key -> manifest entry (without sha) ; data kept in map_data
    map_data = {}

    def map_key(f, pid, name):
        return re.sub(r"[^A-Za-z0-9_]+", "_", name) + "." + str(pid)

    def ref(pptr, consumer, want_data):
        """PPtr -> JSON value: maps by key (data extracted for height mods), other objects by origin/type/name"""
        f, o = resolve(pptr)
        d = {"file": "KSP_x64_Data/" + f, "pathID": pptr["m_PathID"]}
        if o is None:
            return d
        if o.type.name == "MonoBehaviour":
            c = mb_class(o)
            if c in MAP_CLASSES:
                t = o.read_typetree()
                key = map_key(f, pptr["m_PathID"], t.get("m_Name", ""))
                if key not in maps:
                    maps[key] = {"key": key, "class": c, "file": "KSP_x64_Data/" + f, "pathID": pptr["m_PathID"],
                                 "name": t.get("m_Name", ""), "MapName": t.get("_name"), "width": t.get("_width"),
                                 "height": t.get("_height"), "bpp": t.get("_bpp"), "rowWidth": t.get("_rowWidth"),
                                 "isCompiled": t.get("_isCompiled"), "consumers": []}
                maps[key]["consumers"].append(consumer)
                if want_data and key not in map_data:
                    map_data[key] = bytes(t.get("_data") or b"")
                return {"map": key}
            d["type"] = c
        else:
            d["type"] = o.type.name
        try:
            d["name"] = o.read_typetree().get("m_Name", "")
        except Exception:
            pass
        return d

    def convert(v, consumer, want_data, field=""):
        # want_data: the mod affects height; colour maps (fields named *color*) are referenced, not extracted
        if isinstance(v, dict):
            if set(v.keys()) == {"m_FileID", "m_PathID"}:
                if not v["m_PathID"]:
                    return None
                return ref(v, consumer + "." + field, want_data and "color" not in field.lower())
            return {k: convert(x, consumer, want_data, k) for k, x in v.items() if k not in ("m_GameObject", "m_Script")}
        if isinstance(v, list):
            return [convert(x, consumer, want_data, field) for x in v]
        return v

    def go_info(go_id):
        t = objs[go_id].read_typetree()
        return t.get("m_Name", ""), bool(t.get("m_IsActive", 1)), [c["component"]["m_PathID"] for c in t["m_Component"]]

    def transform_of(comps):
        for cid in comps:
            if objs[cid].type.name in ("Transform", "RectTransform"):
                return objs[cid].read_typetree()
        return None

    def has_pqs(comps):
        return any(objs[c].type.name == "MonoBehaviour" and mb_class(objs[c]) == "PQS" for c in comps)

    def get_child_mods(go_id, path, body, out):
        # PQS.cs:2161 GetChildMods: foreach child transform (m_Children order) without a PQS component: AddRange its
        # PQSMod components (m_Component order), then recurse into it.
        _, _, comps = go_info(go_id)
        tr = transform_of(comps)
        for ch in tr["m_Children"]:
            cgo = objs[ch["m_PathID"]].read_typetree()["m_GameObject"]["m_PathID"]
            name, active, ccomps = go_info(cgo)
            cpath = f"{path}/{name}"
            if has_pqs(ccomps):
                continue
            for cid in ccomps:
                o = objs[cid]
                if o.type.name != "MonoBehaviour":
                    continue
                c = mb_class(o)
                if c not in mod_classes:
                    continue
                t = o.read_typetree()
                consumer = f"{body}:{cpath}:{c}"
                fields = convert(t, consumer, c in height_classes)
                for k in ("m_Enabled", "m_Name", "m_EditorHideFlags", "m_EditorClassIdentifier"):
                    fields.pop(k, None)
                out.append({"index": len(out), "gameObject": cpath, "activeSelf": active,
                            "behaviourEnabled": bool(t.get("m_Enabled", 1)), "class": c, "pathID": cid,
                            "height_mod": c in height_classes, "fields": fields})
            get_child_mods(cgo, cpath, body, out)

    psb, cbs = {}, {}
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
        e = {"bodyName": cb["bodyName"], "flightGlobalsIndex": p["flightGlobalsIndex"],
             "celestialBody": {"file": "KSP_x64_Data/" + SA9, "pathID": p["celestialBody"]["m_PathID"],
                               **{k: clean(cb.get(k)) for k in CB_FIELDS if k in cb}}}
        pq = p.get("pqsVersion")
        if pq and pq["m_PathID"]:
            t = objs[pq["m_PathID"]].read_typetree()
            go = t["m_GameObject"]["m_PathID"]
            name, active, _ = go_info(go)
            q = {"file": "KSP_x64_Data/" + SA9, "pathID": pq["m_PathID"], "gameObject": name, "activeSelf": active,
                 **{k: clean(t.get(k)) for k in PQS_FIELDS if k in t}}
            mods = []
            get_child_mods(go, name, cb["bodyName"], mods)
            e["pqs"] = q
            e["mods"] = mods
        bodies.append(e)

    os.makedirs(os.path.join(out_dir, "maps"), exist_ok=True)
    manifest = []
    for key in sorted(map_data):
        m = maps[key]
        data = map_data[key]
        path = os.path.join(out_dir, "maps", key + ".bin")
        with open(path, "wb") as f:
            f.write(data)
        expect = m["width"] * m["height"] * m["bpp"]
        manifest.append({**m, "bin": "maps/" + key + ".bin", "bytes": len(data), "expectedBytes": expect,
                         "format": "MapSO._data, %d byte(s) per pixel, row-major from y=0, PixelIndex = x*bpp + y*rowWidth" % m["bpp"],
                         "sha256": hashlib.sha256(data).hexdigest()})
    prov = {"unity_version": UNITY_VERSION, "tool": "core/tools/wp11/extract_terrain.py, UnityPy %s" % UnityPy.__version__,
            "game_root": "decompiled/KSP-dev", "files_loaded": load,
            "asset_files": {f: sha256_file(os.path.join(data_dir, f)) for f in load}}
    with open(os.path.join(out_dir, "pqs_prefab.json"), "w", encoding="utf-8", newline="\n") as f:
        json.dump(clean({"provenance": prov, "height_mod_classes": sorted(height_classes), "bodies": bodies}), f, indent=1)
    with open(os.path.join(out_dir, "manifest.json"), "w", encoding="utf-8", newline="\n") as f:
        json.dump(clean({"provenance": prov, "maps": manifest,
                         "referenced_not_extracted": sorted(set(maps) - set(map_data))}), f, indent=1)
    for b in bodies:
        ms = b.get("mods", [])
        print(b["bodyName"], len(ms), "mods,", sum(1 for m in ms if m["height_mod"]), "height")
    print("maps:", [(m["key"], m["bytes"]) for m in manifest], "total", sum(m["bytes"] for m in manifest))


if __name__ == "__main__":
    main()
