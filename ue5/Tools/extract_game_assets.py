#!/usr/bin/env python
"""WP-UE-06: extract the Unity-serialized game assets the first playable slice renders, into ue5/GameAssets/ (gitignored).

    <harness>/tools-cache/venv/Scripts/python.exe -I ue5/Tools/extract_game_assets.py --game C:/Projects/KSP-UE5/decompiled/KSP-dev ^
        --out ue5/GameAssets [--summary ue5/Tools/extraction_summary.json]

Read-only on the game copy (CLAUDE.md "Original game assets": nothing is generated, repainted, upscaled or substituted; what cannot be
found or decoded is listed in manifest.json "missing"). Categories, output layout and conventions: ue5/Tools/README.md.
Provenance: every output file has an entry in manifest.json with its origin asset file, pathID, Unity type / name and sha256.
The independent check is ue5/Tools/verify_game_assets.py.
"""
import argparse
import json
import math
import os
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)   # python -I drops the script directory from sys.path

from ksp_unity import Game, sha256_bytes, sha256_file  # noqa: E402
import ksp_textures as T  # noqa: E402
import ksp_mesh as M  # noqa: E402
import ksc_scenery as KSC  # noqa: E402

SCHEMA = "ksp-ue5-gameassets/1"
MAIN_MENU_SHADER_SKIP = ("TextMeshPro",)


def safe(name, limit=60):
    s = re.sub(r"[^A-Za-z0-9._-]+", "_", name).strip("_")
    return (s or "unnamed")[:limit]


def stem(file):
    return (file[:-len(".assets")] if file.endswith(".assets") else file).replace(" ", "_")


def jclean(v, src_file=None):
    """JSON-safe copy of a type-tree value: PPtr dicts become {"$ptr": [file, pathID]} (null -> None), NaN / Inf become strings,
    byte blobs become their length."""
    if isinstance(v, float):
        if math.isnan(v):
            return "NaN"
        if math.isinf(v):
            return "Infinity" if v > 0 else "-Infinity"
        return v
    if isinstance(v, dict):
        if set(v.keys()) == {"m_FileID", "m_PathID"}:
            if v["m_PathID"] == 0:
                return None
            return {"$ptr": [src_file, v["m_PathID"]] if v["m_FileID"] == 0 else ["external:%d" % v["m_FileID"], v["m_PathID"]]}
        return {k: jclean(x, src_file) for k, x in v.items()}
    if isinstance(v, (list, tuple)):
        return [jclean(x, src_file) for x in v]
    if isinstance(v, (bytes, bytearray)):
        return {"bytes": len(v)}
    return v


class Exporter:
    def __init__(self, game, out_dir):
        self.g = game
        self.out = out_dir
        self.entries = {}       # relpath -> manifest entry
        self.missing = []
        self.used_sources = set()
        self.cache = {}
        self.warnings = []

    # ---- files --------------------------------------------------------------------------------------------------------------
    def origin(self, key):
        o = self.g.objs[key]
        t = o.type.name
        return {"file": key[0], "pathID": key[1], "type": t, "name": self.g.name_of(key) if t != "MonoBehaviour" else self.g.mb_class(key)}

    def emit(self, rel, data, kind, category, key, meta=None):
        if rel in self.entries:
            e = self.entries[rel]
            if e["sha256"] != sha256_bytes(data):
                raise RuntimeError("two different contents for %s" % rel)
            if category not in e["categories"]:
                e["categories"].append(category)
                e["categories"].sort()
            return rel
        path = os.path.join(self.out, rel)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as f:
            f.write(data)
        self.entries[rel] = {"path": rel, "kind": kind, "categories": [category], "bytes": len(data), "sha256": sha256_bytes(data),
                             "origin": self.origin(key), "meta": meta or {}}
        self.used_sources.add(key[0])
        return rel

    def miss(self, what, key, reason):
        self.missing.append({"what": what, "origin": ({"file": key[0], "pathID": key[1]} if key else None), "reason": reason})

    def emit_json(self, rel, doc, category, key, kind="description"):
        data = (json.dumps(doc, indent=1, sort_keys=True) + "\n").encode("utf-8")
        return self.emit(rel, data, kind, category, key)

    # ---- textures -----------------------------------------------------------------------------------------------------------
    def texture(self, key, category):
        """Relative path of the exported Texture2D, or None (recorded in `missing`)."""
        if key in self.cache:
            ck = self.cache[key]
            if ck and category not in self.entries[ck]["categories"]:
                self.entries[ck]["categories"].append(category)
                self.entries[ck]["categories"].sort()
            return ck
        rel = None
        try:
            rel = self._texture(key, category)
        except T.Undecodable as e:
            self.miss("texture %s" % self.g.name_of(key), key, "undecodable: %s" % e)
        except Exception as e:  # noqa: BLE001
            self.miss("texture %s" % self.g.name_of(key), key, "%s: %s" % (type(e).__name__, str(e)[:120]))
        self.cache[key] = rel
        return rel

    def _texture(self, key, category):
        g = self.g
        ot = g.type_of(key)
        if ot != "Texture2D":
            raise T.Undecodable("%s objects are not extracted" % ot)
        t = g.objs[key].read()
        fmt = int(t.m_TextureFormat)
        if fmt not in T.FORMATS:
            raise T.Undecodable("TextureFormat %d" % fmt)
        name, kind, bpp, dxgi = T.FORMATS[fmt]
        w, h, mips = int(t.m_Width), int(t.m_Height), int(t.m_MipCount)
        if int(t.m_TextureDimension) != 2 or int(t.m_ImageCount) != 1:
            raise T.Undecodable("dimension %d images %d" % (t.m_TextureDimension, t.m_ImageCount))
        data = bytes(t.get_image_data())
        sd = t.m_StreamData
        if sd is not None and sd.path:
            self.used_sources.add(os.path.basename(sd.path))
        if kind == "crunch":
            import texture2ddecoder
            data = texture2ddecoder.unpack_unity_crunch(data)
            fmt_out = T.CRUNCH_TARGET[fmt]
            mips_stored = 1         # the codec returns the top level only (checked by the size test below)
        else:
            fmt_out = fmt
            mips_stored = mips
        sizes = T.mip_sizes(w, h, mips_stored, fmt_out)
        if sum(sizes) != len(data):
            raise T.Undecodable("data size %d != expected %d (%dx%d, %d mips, %s)" % (len(data), sum(sizes), w, h, mips, name))
        ts = t.m_TextureSettings
        meta = {"width": w, "height": h, "mipCount": mips, "unityFormat": fmt, "unityFormatName": name, "sRGB": bool(t.m_ColorSpace),
                "filterMode": int(ts.m_FilterMode), "aniso": int(ts.m_Aniso), "wrapU": int(ts.m_WrapU), "wrapV": int(ts.m_WrapV),
                "rowOrder": "unity-bottom-up"}
        base = "textures/%s_%d_%s" % (stem(key[0]), key[1], safe(g.name_of(key)))
        if fmt in T.PNG_FORMATS:
            ctype, px = T.to_png_pixels(fmt, w, h, data)
            meta["container"] = "png"
            meta["mipsWritten"] = 1
            return self.emit(base + ".png", T.png(w, h, ctype, px), "texture", category, key, meta)
        meta["container"] = "dds"
        meta["mipsWritten"] = mips_stored
        if kind == "crunch":
            meta["unpackedFrom"] = name
            meta["storedFormat"] = T.FORMATS[fmt_out][0]
        return self.emit(base + ".dds", T.dds(w, h, mips_stored, fmt, data), "texture", category, key, meta)

    # ---- materials ----------------------------------------------------------------------------------------------------------
    def shader_name(self, skey):
        if skey is None:
            return None
        try:
            return self.g.name_of(skey)
        except Exception:  # noqa: BLE001
            return ""

    def material(self, key, category, follow_textures=True):
        if key in self.cache:
            ck = self.cache[key]
            if ck and category not in self.entries[ck]["categories"]:
                self.entries[ck]["categories"].append(category)
                self.entries[ck]["categories"].sort()
                if follow_textures:
                    for tr in self.entries[ck]["meta"].get("textureFiles", []):
                        self.entries[tr]["categories"] = sorted(set(self.entries[tr]["categories"]) | {category})
            return ck
        g = self.g
        t = g.tt(key)
        skey = g.ptr(key[0], t["m_Shader"])
        sp = t["m_SavedProperties"]
        texs, tex_files = {}, []
        for name, env_ in sp.get("m_TexEnvs", []):
            tk = g.ptr(key[0], env_["m_Texture"])
            entry = {"texture": None, "scale": [env_["m_Scale"]["x"], env_["m_Scale"]["y"]],
                     "offset": [env_["m_Offset"]["x"], env_["m_Offset"]["y"]]}
            if tk is not None:
                entry["origin"] = {"file": tk[0], "pathID": tk[1], "type": g.type_of(tk) if tk in g.objs else "?"}
                if follow_textures and tk in g.objs:
                    entry["texture"] = self.texture(tk, category)
                    if entry["texture"]:
                        tex_files.append(entry["texture"])
            texs[name] = entry
        doc = {"schema": "ksp-ue5-material/1", "name": t["m_Name"], "shader": self.shader_name(skey),
               "shaderOrigin": {"file": skey[0], "pathID": skey[1]} if skey else None,
               "keywords": [k for k in t["m_ShaderKeywords"].split(" ") if k], "customRenderQueue": int(t["m_CustomRenderQueue"]),
               "floats": {n: v for n, v in sp.get("m_Floats", [])},
               "colors": {n: [c["r"], c["g"], c["b"], c["a"]] for n, c in sp.get("m_Colors", [])}, "textures": texs}
        if "m_Ints" in sp:
            doc["ints"] = {n: v for n, v in sp["m_Ints"]}
        doc = jclean(doc)
        rel = "materials/%s_%d_%s.json" % (stem(key[0]), key[1], safe(t["m_Name"]))
        data = (json.dumps(doc, indent=1, sort_keys=True) + "\n").encode("utf-8")
        self.emit(rel, data, "material", category, key, {"textureFiles": sorted(set(tex_files)), "shader": doc["shader"]})
        self.cache[key] = rel
        return rel

    # ---- meshes -------------------------------------------------------------------------------------------------------------
    def mesh(self, key, category):
        if key in self.cache:
            ck = self.cache[key]
            if ck:
                for r in (ck, ck[:-5] + ".kmesh"):
                    if category not in self.entries[r]["categories"]:
                        self.entries[r]["categories"].append(category)
                        self.entries[r]["categories"].sort()
            return ck
        rel = None
        try:
            blob, desc = M.export_mesh(self.g.objs[key])
            base = "meshes/%s_%d_%s" % (stem(key[0]), key[1], safe(desc["name"]))
            desc["binary"] = os.path.basename(base) + ".kmesh"
            desc["binarySha256"] = sha256_bytes(blob)
            meta = {"vertexCount": desc["vertexCount"], "indexCount": desc["indexCount"], "channels": sorted(desc["arrays"])}
            self.emit(base + ".kmesh", blob, "mesh-binary", category, key, meta)
            rel = self.emit_json(base + ".json", desc, category, key, kind="mesh-description")
            sd = self.g.tt(key).get("m_StreamData") or {}
            if sd.get("path"):
                self.used_sources.add(os.path.basename(sd["path"]))
        except Exception as e:  # noqa: BLE001
            self.miss("mesh %s" % self.g.name_of(key), key, "%s: %s" % (type(e).__name__, str(e)[:120]))
        self.cache[key] = rel
        return rel


# ======================================================================================================================================
# categories
# ======================================================================================================================================
def dump_mb(ex, key, cls):
    try:
        f = ex.g.mb_fields(key, cls)
    except Exception as e:  # noqa: BLE001
        ex.warnings.append("MonoBehaviour %s %s: fields unreadable (%s)" % (cls, key, str(e)[:60]))
        return None
    f = {k: v for k, v in f.items() if k not in ("m_GameObject", "m_Script")}
    return jclean(f, key[0])


def walk_tree(ex, go_key, category, mb_classes=None, skip_go=None, depth_cap=64):
    """Flat node list of a GameObject subtree: transform, renderers (mesh + materials extracted), components, selected MB fields."""
    g = ex.g
    nodes = []

    def visit(k, parent, depth):
        if depth > depth_cap:
            raise RuntimeError("tree too deep")
        if skip_go and skip_go(k):
            return
        t = g.tt(k)
        tk = g.transform_of(k)
        tr = g.tt(tk)
        node = {"name": t["m_Name"], "origin": {"file": k[0], "pathID": k[1]}, "parent": parent, "active": bool(t["m_IsActive"]),
                "layer": int(t["m_Layer"]), "tag": g.tag_name(int(t["m_Tag"])),
                "position": [tr["m_LocalPosition"][a] for a in "xyz"],
                "rotation": [tr["m_LocalRotation"][a] for a in "xyzw"], "scale": [tr["m_LocalScale"][a] for a in "xyz"],
                "components": [], "renderers": [], "behaviours": []}
        idx = len(nodes)
        nodes.append(node)
        mesh_key = None
        comps = g.components(k)
        for n, ck in comps:
            if n == "MeshFilter":
                mesh_key = g.ptr(ck[0], g.tt(ck)["m_Mesh"])
        for n, ck in comps:
            if n in ("Transform", "RectTransform", "GameObject"):
                continue
            if n == "MonoBehaviour":
                cls = g.mb_class(ck)
                node["components"].append(cls)
                if mb_classes is None or cls in mb_classes or cls.split(".")[-1] in mb_classes:
                    node["behaviours"].append({"class": cls, "origin": {"file": ck[0], "pathID": ck[1]}, "fields": dump_mb(ex, ck, cls)})
            else:
                node["components"].append(n)
            if n in ("MeshRenderer", "SkinnedMeshRenderer"):
                rt = g.tt(ck)
                mk = mesh_key if n == "MeshRenderer" else g.ptr(ck[0], rt["m_Mesh"])
                mats = [g.ptr(ck[0], m) for m in rt["m_Materials"]]
                r = {"component": n, "enabled": bool(rt["m_Enabled"]), "castShadows": int(rt["m_CastShadows"]),
                     "receiveShadows": bool(rt["m_ReceiveShadows"]), "mesh": None, "materials": []}
                if mk is not None and mk in g.objs:
                    r["mesh"] = ex.mesh(mk, category)
                    r["meshOrigin"] = {"file": mk[0], "pathID": mk[1]}
                for m in mats:
                    r["materials"].append(ex.material(m, category) if m is not None and m in g.objs else None)
                node["renderers"].append(r)
        for c in g.children(k):
            visit(c, idx, depth + 1)

    visit(go_key, -1, 0)
    return nodes


def cat_scaled_space(ex, bodies_filter=None):
    g = ex.g
    res = {}
    for k in sorted(g.all("MonoBehaviour", file="sharedassets9.assets")):
        if g.mb_class(k) != "PSystemBody":
            continue
        f = g.tt(k)
        sv = g.ptr(k[0], f["scaledVersion"])
        cb = g.ptr(k[0], f["celestialBody"])
        if sv is None:
            continue
        cbf = g.tt(cb)
        bname = cbf["bodyName"]
        nodes = walk_tree(ex, sv, "scaled_space",
                          mb_classes={"ScaledSpaceFader", "MaterialSetDirection", "AtmosphereFromGround", "ScaledSun", "SunShaderController",
                                      "SunCoronas", "MaterialBasedOnGraphicsSetting", "GasGiantMaterialControl", "SunLensFlare"})
        doc = {"schema": "ksp-ue5-scaled-body/1", "body": bname, "flightGlobalsIndex": int(f["flightGlobalsIndex"]),
               "psystemBody": {"file": k[0], "pathID": k[1]}, "celestialBody": {"file": cb[0], "pathID": cb[1]},
               "radius": cbf.get("Radius"), "scaledVersion": {"file": sv[0], "pathID": sv[1]}, "nodes": nodes,
               "note": "node transforms are the prefab's local TRS; KSP draws scaledVersion at ScaledSpace.scaleFactor 6000 (ScaledSpace.cs)"}
        res[bname] = ex.emit_json("scaled_space/%s.json" % bname, doc, "scaled_space", sv)
    return res


def cat_galaxy(ex):
    g = ex.g
    root = ("level9", 15)
    assert g.name_of(root) == "GalaxyCube"
    nodes = walk_tree(ex, root, "skybox", mb_classes={"GalaxyCubeControl"})
    doc = {"schema": "ksp-ue5-galaxy-cube/1", "root": {"file": root[0], "pathID": root[1]}, "nodes": nodes,
           "note": "six quads, one material each (shader and textures in materials/); GalaxyCubeControl rotates the cube with the planetarium"}
    return ex.emit_json("skybox/galaxy_cube.json", doc, "skybox", root)


PQS_SKIP_CLASSES = ("PQSLandControl", "PQSROCControl", "PQSCity", "PQSCity2")


def cat_pqs(ex, body="Kerbin"):
    """Materials (and their textures) of the body's PQS GameObject tree: the PQS component's quality materials, the texture atlas
    mod, the ocean child PQS and OceanFX. Scatter / ROC / PQSCity subtrees are left out (scatter props are not in the slice)."""
    g = ex.g
    pq = None
    for k in sorted(g.all("MonoBehaviour", file="sharedassets9.assets")):
        if g.mb_class(k) == "PSystemBody":
            f = g.tt(k)
            cb = g.ptr(k[0], f["celestialBody"])
            if g.tt(cb)["bodyName"] == body:
                pq = g.ptr(k[0], f["pqsVersion"])
    root = g.ptr(pq[0], g.tt(pq)["m_GameObject"])
    roles = []

    def scan(v, path, src):
        if isinstance(v, dict):
            if set(v.keys()) == {"m_FileID", "m_PathID"}:
                pk = g.ptr(src, v)
                if pk is not None and pk in g.objs and g.type_of(pk) in ("Material", "Texture2D", "Texture2DArray", "Texture3D", "Cubemap"):
                    yield path, pk
                return
            for kk, x in v.items():
                yield from scan(x, path + "." + kk if path else kk, src)
        elif isinstance(v, list):
            for i, x in enumerate(v):
                yield from scan(x, "%s[%d]" % (path, i), src)

    def visit(k, path):
        comps = g.components(k)
        classes = [g.mb_class(ck) for n, ck in comps if n == "MonoBehaviour"]
        if any(c in ("PQSCity", "PQSCity2") for c in classes):
            return
        name = g.tt(k)["m_Name"]
        here = path + "/" + name if path else name
        for n, ck in comps:
            if n != "MonoBehaviour":
                continue
            cls = g.mb_class(ck)
            if cls in PQS_SKIP_CLASSES:
                continue
            try:
                f = g.mb_fields(ck, cls)
            except Exception as e:  # noqa: BLE001
                ex.warnings.append("PQS %s fields unreadable: %s" % (cls, str(e)[:60]))
                continue
            for fp, pk in scan({a: b for a, b in f.items() if a not in ("m_GameObject", "m_Script")}, "", ck[0]):
                roles.append((here, cls, fp, pk))
        for c in g.children(k):
            visit(c, here)

    visit(root, "")
    out_roles, seen = [], set()
    for here, cls, fp, pk in roles:
        t = g.type_of(pk)
        if t == "Material":
            rel = ex.material(pk, "pqs_" + body.lower())
        elif t == "Texture2D":
            rel = ex.texture(pk, "pqs_" + body.lower())
        else:
            rel = None
            if (t, pk) not in seen:
                seen.add((t, pk))
                ex.miss("%s %s (%s.%s)" % (t, g.name_of(pk), cls, fp), pk, "%s objects are not extracted" % t)
        out_roles.append({"gameObject": here, "class": cls, "field": fp, "type": t, "origin": {"file": pk[0], "pathID": pk[1]},
                          "name": g.name_of(pk), "file": rel})
    doc = {"schema": "ksp-ue5-pqs-materials/1", "body": body, "pqsGameObject": {"file": root[0], "pathID": root[1]},
           "roles": sorted(out_roles, key=lambda r: (r["gameObject"], r["class"], r["field"])),
           "notLeftOut": "PQS component quality materials, TextureAtlas mod, ocean child PQS and OceanFX",
           "leftOut": "PQSLandControl / PQSROCControl scatter props and the PQSCity subtrees (KSC and monuments: see ksc/)"}
    return ex.emit_json("pqs/%s.json" % body, doc, "pqs_" + body.lower(), root)


def cat_ksc(ex, pqs_prefab_json, scenery_json, city_name="Kerbin/KSC"):
    sc = KSC.Scenery(ex.g)
    nodes, go_key = sc.city(city_name, pqs_prefab_json)
    mism = KSC.check_against_colliders(nodes, go_key, scenery_json, city_name)
    if mism:
        raise SystemExit("KSC render tree differs from the WP-12c collider tree: %d mismatches, first: %s" % (len(mism), mism[:3]))
    out = []
    for n in nodes:
        rs = []
        for r in n.renderers:
            mk = r["mesh"]
            if mk is None or mk not in ex.g.objs:
                ex.miss("renderer without mesh on %s" % n._path[-60:], None, "mesh PPtr is null")
                continue
            rs.append({"component": r["component"], "enabled": r["enabled"], "castShadows": r["castShadows"],
                       "receiveShadows": r["receiveShadows"], "mesh": ex.mesh(mk, "ksc"), "meshOrigin": {"file": mk[0], "pathID": mk[1]},
                       "rendererOrigin": {"file": r["file"], "pathID": r["pathID"]},
                       "materials": [ex.material(m, "ksc") if m is not None and m in ex.g.objs else None for m in r["materials"]]})
        out.append({"index": n.index, "path": n._path, "parent": n._parent, "src": list(n.src), "active": n.active, "layer": n.layer,
                    "tag": n.tag, "position": n.pos, "rotation": n.rot, "scale": n.scl, "renderers": rs})
    skinned = sum(1 for n in nodes for r in n.renderers if r["component"] == "SkinnedMeshRenderer")
    doc = {"schema": "ksp-ue5-ksc-render/1", "city": city_name, "cityGameObject": {"file": go_key[0], "pathID": go_key[1]},
           "colliderTreeCheck": "index, path, parent, src, active, layer, tag and TRS float32 bits equal core/data/scenery/scenery_colliders.json",
           "objects": out, "warnings": sc.warnings,
           "note": "placement: the city pose of PQSCity::Orientate applies to object 0; render and collider trees share the object index"}
    rel = ex.emit_json("ksc/%s.json" % safe(city_name.replace("/", "_")), doc, "ksc", go_key)
    return rel, {"objects": len(nodes), "renderers": sum(len(n.renderers) for n in nodes), "skinned": skinned,
                 "withRenderer": sum(1 for n in nodes if n.renderers)}


def cat_fonts(ex):
    g = ex.g
    out = []
    for k in sorted(g.all("Font")):
        t = g.tt(k)
        data = bytes(t["m_FontData"] or b"")
        if not data:
            ex.miss("font %s" % t["m_Name"], k, "no embedded font data (built-in font)")
            continue
        magic = data[:4]
        ext = ".otf" if magic == b"OTTO" else (".ttf" if magic in (b"\x00\x01\x00\x00", b"true") else None)
        if ext is None:
            ex.miss("font %s" % t["m_Name"], k, "unknown font container %r" % magic)
            continue
        rel = ex.emit("fonts/%s_%d_%s%s" % (stem(k[0]), k[1], safe(t["m_Name"]), ext), data, "font", "fonts", k,
                      {"fontNames": list(t["m_FontNames"]), "fontSize": t["m_FontSize"]})
        out.append(rel)
    return out


HUD_ROOTS = [("sharedassets7.assets", 982, "Flight"),          # the flight HUD canvas (NavballFrame, TopFrame, TimeFrame, CrewFrame ...)
             ("sharedassets7.assets", 699, "NavBallController"),
             ("sharedassets7.assets", 888, "StageManager"),
             ("sharedassets7.assets", 937, "StageGroup"),
             ("sharedassets7.assets", 1186, "StageMarker"),
             ("sharedassets7.assets", 679, "ManeuverGizmo"),
             ("sharedassets7.assets", 701, "MapNode"),
             ("sharedassets7.assets", 1013, "VesselLabels")]
UI_FILES = ("sharedassets0.assets", "sharedassets5.assets", "sharedassets6.assets", "sharedassets7.assets")
# textures the flight HUD uses that no sprite / material in the flight scene file references: the staging stack's part icon atlas
# (StageIcon prefab, KSP.UI.Screens.StageIcon, sharedassets6.assets MonoBehaviour 2515)
EXTRA_TEXTURES = [("sharedassets6.assets", 110, "StageIconMapDefault", "StageIcon")]
FLIGHT_UI_FILE = "sharedassets7.assets"      # the flight scene's asset file: prefab references are followed inside it only


def _pptrs(v):
    if isinstance(v, dict):
        if set(v.keys()) == {"m_FileID", "m_PathID"}:
            if v["m_PathID"]:
                yield v
            return
        for x in v.values():
            yield from _pptrs(x)
    elif isinstance(v, list):
        for x in v:
            yield from _pptrs(x)


def hud_closure(ex):
    """Sprites / textures / meshes / materials reachable from the HUD roots (rule documented in README)."""
    g = ex.g
    seen, stack = set(), []
    for f, pid, name in HUD_ROOTS:
        assert g.name_of((f, pid)) == name, (f, pid, name)
        stack.append(((f, pid), name))
    sprites, textures, meshes, mats, used_by = set(), set(), set(), set(), {}
    while stack:
        k, root = stack.pop()
        if k in seen or k not in g.objs:
            continue
        seen.add(k)
        t = g.type_of(k)
        if t == "GameObject":
            for n, ck in g.components(k):
                stack.append((ck, root))
            for c in g.children(k):
                stack.append((c, root))
        elif t == "MonoBehaviour":
            cls = g.mb_class(k)
            try:
                f = g.mb_fields(k, cls)
            except Exception:  # noqa: BLE001
                continue
            for p in _pptrs({a: b for a, b in f.items() if a not in ("m_GameObject", "m_Script")}):
                pk = g.ptr(k[0], p)
                if pk is None or pk not in g.objs:
                    continue
                pt = g.type_of(pk)
                if pt == "Sprite":
                    sprites.add(pk)
                    used_by.setdefault(pk, set()).add(root)
                elif pt == "Texture2D" and pk[0] in UI_FILES:
                    textures.add(pk)
                    used_by.setdefault(pk, set()).add(root)
                elif pt in ("Material",):
                    mats.add(pk)
                elif pt in ("GameObject", "MonoBehaviour") and pk[0] == FLIGHT_UI_FILE:
                    stack.append((pk, root))
                elif pt in ("RectTransform", "Transform") and pk[0] == FLIGHT_UI_FILE:
                    stack.append((g.ptr(pk[0], g.tt(pk)["m_GameObject"]), root))
        elif t == "MeshFilter":
            pk = g.ptr(k[0], g.tt(k)["m_Mesh"])
            if pk is not None and pk in g.objs and pk[0] != "unity default resources":
                meshes.add(pk)
                used_by.setdefault(pk, set()).add(root)
        elif t in ("MeshRenderer", "SkinnedMeshRenderer"):
            for m in g.tt(k)["m_Materials"]:
                pk = g.ptr(k[0], m)
                if pk is not None and pk in g.objs:
                    mats.add(pk)
                    used_by.setdefault(pk, set()).add(root)
    return sprites, textures, meshes, mats, used_by


def cat_ui(ex):
    g = ex.g
    sprites, textures, meshes, mats, used_by = hud_closure(ex)
    sprite_docs = []
    for k in sorted(sprites):
        t = g.tt(k)
        rd = t["m_RD"]
        tk = g.ptr(k[0], rd["texture"])
        rel = ex.texture(tk, "ui") if tk is not None and tk in g.objs else None
        if rel is None:
            ex.miss("sprite %s: atlas texture" % t["m_Name"], k, "texture not extractable")
        sprite_docs.append({"name": t["m_Name"], "origin": {"file": k[0], "pathID": k[1]}, "texture": rel,
                            "textureOrigin": {"file": tk[0], "pathID": tk[1]} if tk else None,
                            "rect": [t["m_Rect"][a] for a in ("x", "y", "width", "height")],
                            "offset": [t["m_Offset"]["x"], t["m_Offset"]["y"]],
                            "border": [t["m_Border"][a] for a in ("x", "y", "z", "w")], "pivot": [t["m_Pivot"]["x"], t["m_Pivot"]["y"]],
                            "pixelsPerUnit": t["m_PixelsToUnits"],
                            "textureRect": [rd["textureRect"][a] for a in ("x", "y", "width", "height")],
                            "textureRectOffset": [rd["textureRectOffset"]["x"], rd["textureRectOffset"]["y"]],
                            "settingsRaw": rd["settingsRaw"], "isPolygon": bool(t["m_IsPolygon"]), "usedBy": sorted(used_by[k])})
    tex_docs = []
    for k in sorted(textures):
        rel = ex.texture(k, "ui")
        tex_docs.append({"name": g.name_of(k), "origin": {"file": k[0], "pathID": k[1]}, "file": rel, "usedBy": sorted(used_by[k])})
    for f, pid, name, who in EXTRA_TEXTURES:
        assert g.name_of((f, pid)) == name, (f, pid, name)
        tex_docs.append({"name": name, "origin": {"file": f, "pathID": pid}, "file": ex.texture((f, pid), "ui"), "usedBy": [who]})
    mat_docs = []
    for k in sorted(mats):
        sk = g.ptr(k[0], g.tt(k)["m_Shader"])
        sn = ex.shader_name(sk) or ""
        if sn.startswith(MAIN_MENU_SHADER_SKIP) or k[0] not in UI_FILES:
            continue                       # TextMeshPro font materials (SDF atlases, CJK) and editor-only helpers: not the slice HUD
        mat_docs.append({"name": g.name_of(k), "shader": sn, "origin": {"file": k[0], "pathID": k[1]},
                         "file": ex.material(k, "ui"), "usedBy": sorted(used_by.get(k, []))})
    mesh_docs = [{"name": g.name_of(k), "origin": {"file": k[0], "pathID": k[1]}, "file": ex.mesh(k, "ui"), "usedBy": sorted(used_by[k])}
                 for k in sorted(meshes)]
    navball = walk_tree(ex, ("sharedassets7.assets", 699), "ui")
    doc = {"schema": "ksp-ue5-ui-sprites/1",
           "rule": "sprites / textures reachable from the HUD roots' prefab trees (GameObject children, components, MonoBehaviour fields incl. "
                   "prefab references inside the UI asset files); materials only through MeshRenderers (3D HUD: NavBall, vectors, tumbler, "
                   "maneuver gizmo); TextMeshPro font materials excluded",
           "roots": [{"name": n, "origin": {"file": f, "pathID": p}} for f, p, n in HUD_ROOTS],
           "sprites": sprite_docs, "textures": tex_docs, "materials": mat_docs, "meshes": mesh_docs, "navBallControllerTree": navball,
           "spriteConvention": "textureRect is in pixels of the atlas texture in Unity order (origin bottom left); the atlas files keep "
                               "Unity row order. Sprites are not cropped: the shell uses the rect as a UV region."}
    return ex.emit_json("ui/flight_hud.json", doc, "ui", HUD_ROOTS_KEY), {"sprites": len(sprite_docs), "textures": len(tex_docs),
                                                                         "materials": len(mat_docs), "meshes": len(mesh_docs)}


HUD_ROOTS_KEY = ("sharedassets7.assets", 982)


# ======================================================================================================================================
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--game", required=True, help="game copy root (decompiled/KSP-dev)")
    ap.add_argument("--out", required=True, help="output directory (ue5/GameAssets); must be empty or absent")
    ap.add_argument("--lib", help="harness/lib (default: found beside the game copy)")
    ap.add_argument("--pqs-prefab", help="core/data/terrain/pqs_prefab.json", default=os.path.join(HERE, "..", "..", "core", "data", "terrain", "pqs_prefab.json"))
    ap.add_argument("--scenery", help="core/data/scenery/scenery_colliders.json", default=os.path.join(HERE, "..", "..", "core", "data", "scenery", "scenery_colliders.json"))
    ap.add_argument("--summary", help="write the small summary JSON here")
    ap.add_argument("--only", help="comma list of categories (scaled_space,skybox,pqs,ksc,fonts,ui); default all")
    args = ap.parse_args()
    out = os.path.abspath(args.out)
    if os.path.isdir(out) and os.listdir(out):
        raise SystemExit("output directory %s is not empty" % out)
    only = set(args.only.split(",")) if args.only else {"scaled_space", "skybox", "pqs", "ksc", "fonts", "ui"}
    t0 = time.time()
    g = Game(args.game, args.lib)
    print("loaded %d objects in %.1f s" % (len(g.objs), time.time() - t0))
    ex = Exporter(g, out)
    summary = {}
    if "scaled_space" in only:
        r = cat_scaled_space(ex)
        summary["scaled_space"] = {"bodies": len(r)}
        print("scaled_space: %d bodies" % len(r))
    if "skybox" in only:
        cat_galaxy(ex)
        print("skybox done")
    if "pqs" in only:
        cat_pqs(ex, "Kerbin")
        print("pqs done")
    if "ksc" in only:
        rel, st = cat_ksc(ex, os.path.abspath(args.pqs_prefab), os.path.abspath(args.scenery))
        summary["ksc"] = st
        print("ksc:", st)
    if "fonts" in only:
        r = cat_fonts(ex)
        summary["fonts"] = {"fonts": len(r)}
        print("fonts:", len(r))
    if "ui" in only:
        rel, st = cat_ui(ex)
        summary["ui"] = st
        print("ui:", st)

    # source files consulted: sha256 of every one (asset files and their .resS streams)
    sources = {}
    for f in sorted(ex.used_sources):
        p = os.path.join(g.data_dir, f)
        if os.path.isfile(p):
            sources[f] = sha256_file(p)
    entries = [ex.entries[k] for k in sorted(ex.entries)]
    cats = {}
    for e in entries:
        for c in e["categories"]:
            d = cats.setdefault(c, {"files": 0, "bytes": 0})
            d["files"] += 1
            d["bytes"] += e["bytes"]
    manifest = {"schema": SCHEMA, "tool": "ue5/Tools/extract_game_assets.py", "unityVersion": "2019.4.18f1",
                "gameRoot": "decompiled/KSP-dev", "sourceFiles": sources, "files": entries,
                "missing": sorted(ex.missing, key=lambda m: (m["what"], json.dumps(m["origin"]))), "warnings": sorted(set(ex.warnings)),
                "categories": cats}
    mdata = (json.dumps(manifest, indent=1, sort_keys=True) + "\n").encode("utf-8")
    with open(os.path.join(out, "manifest.json"), "wb") as f:
        f.write(mdata)
    msha = sha256_bytes(mdata)
    tot = sum(e["bytes"] for e in entries)
    print("files %d, %.1f MB, manifest sha256 %s, missing %d, warnings %d" % (len(entries), tot / 1e6, msha, len(ex.missing), len(ex.warnings)))
    for c, d in sorted(cats.items()):
        print("  %-14s files %5d  %10.1f MB" % (c, d["files"], d["bytes"] / 1e6))
    for m in manifest["missing"]:
        print("MISSING", m["what"], m["origin"], m["reason"])
    if args.summary:
        s = {"schema": "ksp-ue5-gameassets-summary/1", "files": len(entries), "bytes": tot, "manifestSha256": msha, "categories": cats,
             "perCategoryDetail": summary, "missing": len(ex.missing), "warnings": len(ex.warnings),
             "sourceFileCount": len(sources)}
        with open(args.summary, "w", encoding="utf-8", newline="\n") as f:
            json.dump(s, f, indent=1, sort_keys=True)
            f.write("\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
