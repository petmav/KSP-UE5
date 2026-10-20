#!/usr/bin/env python
"""WP-12c: extract the KSC / launch-site / city scenery colliders, their tags / layers and the PhysicMaterials from the shipped Unity assets.

    <harness>/tools-cache/venv/Scripts/python.exe -I core/tools/mucollider/extract_scenery_colliders.py ^
        <KSP_x64_Data dir> <harness lib dir> <pqs_prefab.json> <out dir>

e.g. (repository root):
    harness\\tools-cache\\venv\\Scripts\\python.exe -I core\\tools\\mucollider\\extract_scenery_colliders.py ^
        decompiled\\KSP-dev\\KSP_x64_Data harness\\lib core\\data\\terrain\\pqs_prefab.json core\\data\\scenery

Read-only on the game files (CLAUDE.md "Original game assets"). Writes <out>/scenery_colliders.json (schema
kspcore-scenery-colliders/2, documented in include/ksp/space_center/scenery_colliders.hpp) and <out>/scenery_meshes.bin (the
MeshCollider meshes, one record per Mesh asset: float32 xyz vertices then uint32 triangle indices, little endian).

What the game does (Assembly-CSharp), and what this script replays on the serialized data:
  * the city GameObjects are the PQSCity / PQSCity2 children of a body's PQS GameObject in the PQS prefab (data/terrain/pqs_prefab.json
    lists them with the MonoBehaviour pathID);
  * NestedPrefabSpawner (NestedPrefabSpawner.cs:20 Spawn): Instantiate(prefab), parent = the target transform's parent (appended
    LAST), the target's local TRS and name, layers and tags of the new subtree defaulted to the target's (setLayersAndTagsRecursive,
    :60), the target destroyed. "Kerbin/KSC/SpaceCenter" is such a spawner: the KSC buildings are a prefab, not children of the PQS prefab.
  * Upgradeables.UpgradeableObject (UpgradeableObject.cs:9 UpgradeLevel.Spawn): per facility level a prefab (`facilityPrefab`) placed
    at the "Facility" transform's local TRS under its parent, named like it, replacing it. Every level is emitted ("level groups"): the
    host selects the level (ScenarioUpgradeableFacilities).
  * DestructibleBuilding (DestructibleBuilding.cs:10 CollapsibleObject): collapseObject / replacementObject GameObjects; intact =
    collapse active and replacement inactive, destroyed = the opposite (SetDestroyed, :163).
  * A GameObject's tag is the TagManager name of m_Tag (20000 + index); its layer is m_Layer.
Meshes are decoded with UnityPy's MeshHandler (vertex channels / streams / compression) -> float32 values, exact.
"""
import argparse
import hashlib
import json
import math
import os
import struct
import sys

import UnityPy
import UnityPy.helpers.TypeTreeHelper as TH
from UnityPy.helpers.MeshHelper import MeshHandler
from UnityPy.helpers.TypeTreeGenerator import TypeTreeGenerator
from UnityPy.streams import EndianBinaryReader

SCHEMA = "kspcore-scenery-colliders/2"
UNITY_VERSION = "2019.4.18f1"
BCL = ["mscorlib.dll", "System.dll", "System.Core.dll", "System.Xml.dll", "Mono.Security.dll",
       "System.Configuration.dll", "System.Security.dll"]
BUILTIN_TAGS = {0: "Untagged", 1: "Respawn", 2: "Finish", 4: "EditorOnly", 5: "MainCamera", 6: "Player", 7: "GameController"}
COLLIDER_TYPES = ("BoxCollider", "SphereCollider", "CapsuleCollider", "MeshCollider")


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 22), b""):
            h.update(chunk)
    return h.hexdigest()


def f32(x):
    """A Python float that came from a float32: JSON keeps the shortest repr of the double, which re-reads as the same float32."""
    return float(x)


def vec(v):
    if isinstance(v, dict):
        return [f32(v["x"]), f32(v["y"]), f32(v["z"])]
    return [f32(v.x), f32(v.y), f32(v.z)]


def quat(q):
    return [f32(q.x), f32(q.y), f32(q.z), f32(q.w)]


class Node:
    """One GameObject of an instantiated prefab (or scene) subtree."""

    def __init__(self, key, name, active, layer, tag, pos, rot, scl):
        self.key = key              # (asset file, GameObject pathID) the LOD tables of the PQS prefab refer to (a spawned root keeps its placeholder's)
        self.src = key              # (asset file, GameObject pathID) of the GameObject the values come from
        self.via = None             # the placeholder GameObject a spawned root replaced (its name / TRS / layer are the placeholder's)
        self.name = name
        self.active = active
        self.layer = layer
        self.tag = tag
        self.pos, self.rot, self.scl = pos, rot, scl
        self.children = []
        self.parent = None
        self.colliders = []         # dicts, see collider_json
        self.behaviours = []        # (class name, dict-form fields) of the MonoBehaviours we replay
        self.level_group = None     # UpgradeableObject id the node belongs to
        self.level = -1
        self.crash = None           # CrashObjectName (objectName, displayName)
        self.mobile_pad = False     # PositionMobileLaunchPad component
        self.destructible = None    # list of (collapse Node, replacement Node | None, extra) for a DestructibleBuilding component
        self.index = -1

    def add(self, child):
        child.parent = self
        self.children.append(child)

    def remove(self, child):
        self.children.remove(child)
        child.parent = None


class Extractor:
    def __init__(self, data_dir, lib_dir):
        self.data_dir = data_dir
        self.env = UnityPy.load(data_dir)
        gen = TypeTreeGenerator(UNITY_VERSION)
        for f in sorted(os.listdir(lib_dir)):
            if f.endswith(".dll") and f != "Assembly-CSharp.dll":
                gen.load_dll(open(os.path.join(lib_dir, f), "rb").read())
        for f in BCL:
            gen.load_dll(open(os.path.join(data_dir, "Managed", f), "rb").read())
        self.gen = gen
        self.env.typetree_generator = gen
        self.objs = {}
        for o in self.env.objects:
            self.objs[(o.assets_file.name, o.path_id)] = o
        self.files = {os.path.basename(n): f for n, f in self.env.files.items()}
        self.node_cache = {}
        self.class_cache = {}
        self.tags = self._tag_names()
        self.warnings = []
        self.meshes = []         # records
        self.mesh_ids = {}       # (file, pathID) -> id
        self.mesh_blob = bytearray()
        self.materials = []      # PhysicMaterial records
        self.material_ids = {}
        self.mono_nodes = {}     # class -> patched typetree node

    # ---- helpers -------------------------------------------------------------------------------------------------------------
    def warn(self, msg):
        self.warnings.append(msg)

    def _tag_names(self):
        for o in self.env.objects:
            if o.type.name == "TagManager":
                return list(o.read_typetree()["tags"])
        raise RuntimeError("no TagManager")

    def tag_name(self, t):
        if t in BUILTIN_TAGS:
            return BUILTIN_TAGS[t]
        if t >= 20000 and t - 20000 < len(self.tags):
            return self.tags[t - 20000]
        self.warn("unknown tag id %d" % t)
        return "#%d" % t

    def key_of(self, reader):
        return (reader.assets_file.name, reader.path_id)

    def ptr_key(self, ptr):
        """Key of a PPtr in class form (UnityPy resolves the file id), None when null / unresolvable."""
        if ptr is None or ptr.path_id == 0:
            return None
        try:
            r = ptr.deref()
        except Exception:  # noqa: BLE001
            return None
        return self.key_of(r)

    def dptr_key(self, src_file, d):
        """Key of a PPtr in dict form {m_FileID, m_PathID} found in a MonoBehaviour of `src_file`."""
        if d is None or d["m_PathID"] == 0:
            return None
        if d["m_FileID"] == 0:
            return (src_file, d["m_PathID"])
        ext = self.files[src_file].externals[d["m_FileID"] - 1].path
        return (os.path.basename(ext), d["m_PathID"])

    def mb_class(self, reader):
        try:
            h = reader.parse_monobehaviour_head()
            s = h.m_Script.read()
            return (s.m_Namespace + "." if s.m_Namespace else "") + s.m_ClassName
        except Exception as e:  # noqa: BLE001
            return "?(%s)" % str(e)[:30]

    def mb_fields(self, reader, cls):
        """The serialized fields of a MonoBehaviour (dict form, PPtr as {m_FileID, m_PathID}). The python reader, not the boost one:
        the generator types a string[] as 'string' (it would read one string), fixed up here to a vector of strings."""
        if cls not in self.mono_nodes:
            nodes = self.gen.get_nodes_up("Assembly-CSharp", cls)
            self._fix_string_arrays(nodes)
            self.mono_nodes[cls] = nodes
        rd = EndianBinaryReader(reader.get_raw_data(), "<")
        return TH.read_typetree(self.mono_nodes[cls], rd, True, None, False, reader.assets_file)

    def _fix_string_arrays(self, n):
        if n.m_Type == "string" and n.m_Children and n.m_Children[0].m_Name == "Array":
            arr = n.m_Children[0]
            if len(arr.m_Children) == 2 and arr.m_Children[1].m_Type == "string":
                n.m_Type = "vector"
        for c in n.m_Children:
            self._fix_string_arrays(c)

    def components(self, go_key):
        go = self.objs[go_key].read()
        return [(p.type.name, self.key_of(p.deref())) for p in go.m_Components]

    def find_prefab_by_name(self, name):
        hits = []
        for k, o in self.objs.items():
            if o.type.name == "GameObject" and o.read().m_Name == name:
                hits.append(k)
        if len(hits) != 1:
            self.warn("prefab name %s resolves to %d GameObjects" % (name, len(hits)))
        return hits[0] if hits else None

    def psystem_setup(self):
        for k, o in self.objs.items():
            if o.type.name == "MonoBehaviour" and self.mb_class(o) == "PSystemSetup":
                f = self.mb_fields(o, "PSystemSetup")
                self.setup_origin = k
                return f
        raise RuntimeError("no PSystemSetup")

    # ---- PhysicMaterial -------------------------------------------------------------------------------------------------------
    def material_id(self, ptr):
        key = self.ptr_key(ptr)
        if key is None:
            return -1
        if key in self.material_ids:
            return self.material_ids[key]
        r = self.objs[key]
        if r.type.name != "PhysicMaterial":
            self.warn("collider material %s is a %s" % (key, r.type.name))
            return -1
        m = r.read()
        rec = {"name": m.m_Name, "origin": {"file": key[0], "pathID": key[1]},
               "dynamicFriction": f32(m.dynamicFriction), "staticFriction": f32(m.staticFriction),
               "bounciness": f32(m.bounciness), "frictionCombine": int(m.frictionCombine), "bounceCombine": int(m.bounceCombine)}
        self.material_ids[key] = len(self.materials)
        self.materials.append(rec)
        return self.material_ids[key]

    # ---- meshes ---------------------------------------------------------------------------------------------------------------
    def mesh_id(self, ptr):
        key = self.ptr_key(ptr)
        if key is None:
            return -1
        if key in self.mesh_ids:
            return self.mesh_ids[key]
        reader = self.objs[key]
        mesh = reader.read()
        h = MeshHandler(mesh)
        h.process()
        verts = h.m_Vertices
        if verts is None:
            raise RuntimeError("mesh %s has no vertices" % (key,))
        flat = []
        for v in verts:
            flat.extend((v[0], v[1], v[2]))
        vb = struct.pack("<%df" % len(flat), *flat)
        tris = []
        for sm_idx, sm in enumerate(mesh.m_SubMeshes):
            if sm.topology != 0:
                self.warn("mesh %s submesh %d has topology %d (only triangles are collision geometry)" % (mesh.m_Name, sm_idx, sm.topology))
                continue
            sub = h.get_triangles()[sm_idx] if hasattr(h, "get_triangles") else None
            base = int(sm.baseVertex)
            if base:
                self.warn("mesh %s submesh %d has baseVertex %d (added to the indices)" % (mesh.m_Name, sm_idx, base))
            for tri in sub:
                tris.extend(i + base for i in tri)
        if tris and (min(tris) < 0 or max(tris) >= len(verts)):
            raise RuntimeError("mesh %s: index out of range" % mesh.m_Name)
        ib = struct.pack("<%dI" % len(tris), *tris)
        rec = {"name": mesh.m_Name, "origin": {"file": key[0], "pathID": key[1]}, "vertexCount": len(verts), "indexCount": len(tris),
               "offset": len(self.mesh_blob), "sha256": hashlib.sha256(vb + ib).hexdigest()}
        self.mesh_blob += vb + ib
        self.mesh_ids[key] = len(self.meshes)
        self.meshes.append(rec)
        return self.mesh_ids[key]

    # ---- GameObject trees -----------------------------------------------------------------------------------------------------
    def collider_json(self, comp, cls):
        d = {"trigger": bool(comp.m_IsTrigger), "enabled": bool(comp.m_Enabled), "material": self.material_id(comp.m_Material)}
        if cls == "BoxCollider":
            d.update(type="Box", center=vec(comp.m_Center), size=vec(comp.m_Size))
        elif cls == "SphereCollider":
            d.update(type="Sphere", center=vec(comp.m_Center), radius=f32(comp.m_Radius))
        elif cls == "CapsuleCollider":
            d.update(type="Capsule", center=vec(comp.m_Center), radius=f32(comp.m_Radius), height=f32(comp.m_Height),
                     direction=int(comp.m_Direction))
        else:
            mid = self.mesh_id(comp.m_Mesh)
            if mid < 0:
                return None
            d.update(type="Mesh", convex=bool(comp.m_Convex), mesh=mid)
        return d

    def build(self, go_key, keymap, mbs):
        """The subtree of the GameObject `go_key` as Nodes (an Instantiate of a prefab). `keymap` collects GameObject key -> Node and
        Transform key -> Node, `mbs` the replayed MonoBehaviours (node, class, fields, reader)."""
        reader = self.objs[go_key]
        go = reader.read()
        comps = []
        for p in go.m_Components:
            try:
                comps.append((p.type.name, p.deref()))
            except Exception:  # noqa: BLE001
                self.warn("unresolvable component of %s" % go.m_Name)
        tr = None
        for n, r in comps:
            if n == "Transform":
                tr = r
        trd = tr.read()
        node = Node(go_key, go.m_Name, bool(go.m_IsActive), int(go.m_Layer), self.tag_name(int(go.m_Tag)),
                    vec(trd.m_LocalPosition), quat(trd.m_LocalRotation), vec(trd.m_LocalScale))
        keymap[go_key] = node
        keymap[self.key_of(tr)] = node
        for n, r in comps:
            if n in COLLIDER_TYPES:
                c = self.collider_json(r.read(), n)
                if c:
                    node.colliders.append(c)
                else:
                    self.warn("%s: %s without a mesh" % (go.m_Name, n))
            elif n == "MonoBehaviour":
                cls = self.mb_class(r)
                if cls == "CrashObjectName":
                    f = self.mb_fields(r, cls)
                    node.crash = (f["objectName"], f["displayName"])
                elif cls.endswith("PositionMobileLaunchPad"):
                    node.mobile_pad = True
                if cls in ("NestedPrefabSpawner", "Upgradeables.UpgradeableFacility", "Upgradeables.UpgradeableSlave", "DestructibleBuilding"):
                    mbs.append((node, cls, self.mb_fields(r, cls), r))
        for ch in trd.m_Children:
            child_tr = ch.deref()
            child_go = child_tr.read().m_GameObject
            node.add(self.build(self.ptr_key(child_go), keymap, mbs))
        return node

    def instantiate(self, go_key, depth=0):
        """Instantiate(prefab) with its Awake-time spawners replayed. Returns (root Node, keymap)."""
        if depth > 8:
            raise RuntimeError("nested prefab recursion")
        keymap, mbs = {}, []
        root = self.build(go_key, keymap, mbs)
        self.replay(root, keymap, mbs, depth)
        return root

    def set_layers_tags(self, node, default_layer, allowed_tags, default_tag, set_tags):
        # NestedPrefabSpawner.cs:60 setLayersAndTagsRecursive: (layers.value & layer) == 0 with allowedLayers = -1 <=> layer == 0
        if node.layer == 0:
            node.layer = default_layer
        if set_tags and node.tag not in allowed_tags:
            node.tag = default_tag
        for c in node.children:
            self.set_layers_tags(c, default_layer, allowed_tags, default_tag, set_tags)

    def replay(self, root, keymap, mbs, depth):
        src = lambda reader: reader.assets_file.name  # noqa: E731
        for node, cls, f, reader in mbs:
            sf = src(reader)
            if cls == "NestedPrefabSpawner":
                if f["SpawnOn"] == 0:
                    continue   # SpawnOnEvent.None: nobody calls SpawnPrefabs
                allowed_tags = list(f["allowedTags"])
                allowed_layers = f["allowedLayers"]["m_Bits"] if isinstance(f["allowedLayers"], dict) else f["allowedLayers"]
                if allowed_layers != 0xFFFFFFFF and allowed_layers != -1:
                    self.warn("NestedPrefabSpawner with allowedLayers %x (only -1 replayed)" % allowed_layers)
                for p in f["Prefabs"]:
                    tkey = self.dptr_key(sf, p["tgtTransform"])
                    pkey = self.dptr_key(sf, p["prefab"])
                    tgt = keymap.get(tkey)
                    if tgt is None or pkey is None:
                        self.warn("NestedPrefabSpawner target / prefab unresolved on %s" % node.name)
                        continue
                    new = self.instantiate(pkey, depth + 1)
                    parent = tgt.parent
                    new.name, new.pos, new.rot, new.scl = tgt.name, tgt.pos, tgt.rot, tgt.scl
                    self.set_layers_tags(new, tgt.layer, allowed_tags, tgt.tag, bool(p["setTags"]))
                    if parent is None:
                        self.warn("NestedPrefabSpawner target is a root")
                        continue
                    parent.remove(tgt)
                    parent.add(new)
                    keymap[tkey] = new
                    new.via = tgt.src
                    new.key = tgt.key
                    # keep the destroyed placeholder's key resolvable to the replacement (LOD tables pointing at it)
            elif cls in ("Upgradeables.UpgradeableFacility", "Upgradeables.UpgradeableSlave"):
                ftkey = self.dptr_key(sf, f["facilityTransform"])
                fac = keymap.get(ftkey)
                if fac is None:
                    self.warn("%s %s has no facilityTransform" % (cls, node.name))
                    continue
                parent = fac.parent
                if parent is None:
                    self.warn("%s %s: facilityTransform is a root" % (cls, node.name))
                    continue
                group = f["id"] or ("%s#%d" % (node.name, reader.path_id))
                fac_info = {"group": group, "node": node}
                parent.remove(fac)
                for lvl, ul in enumerate(f["upgradeLevels"]):
                    pkey = self.dptr_key(sf, ul["facilityPrefab"])
                    if pkey is None:
                        self.warn("%s level %d has no prefab" % (group, lvl))
                        continue
                    new = self.instantiate(pkey, depth + 1)
                    new.name, new.pos, new.rot, new.scl = fac.name, fac.pos, fac.rot, fac.scl
                    new.level_group, new.level = group, lvl
                    new.via = fac.src
                    parent.add(new)
                node.behaviours.append(("UpgradeableObject", {"id": group, "levels": len(f["upgradeLevels"])}))
            elif cls == "DestructibleBuilding":
                coll = []
                for co in f["CollapsibleObjects"]:
                    ck = self.dptr_key(sf, co["collapseObject"])
                    rk = self.dptr_key(sf, co["replacementObject"])
                    coll.append((keymap.get(ck), keymap.get(rk), co))
                node.destructible = {"id": f["id"], "collapsibles": coll}

    # ---- flatten --------------------------------------------------------------------------------------------------------------
    def flatten(self, root, root_name):
        out = []

        def visit(node, parent_idx, path):
            node.index = len(out)
            out.append(node)
            node._parent_idx = parent_idx
            node._path = path
            for c in node.children:
                visit(c, node.index, path + "/" + c.name)

        visit(root, -1, root_name)
        return out


def lod_from_prefab(ex, mb_fields, keymap_by_key):
    """PQSCity.lod[] or PQSCity2.objects[] from data/terrain/pqs_prefab.json: visibleRange, objects (GameObject refs -> Nodes)."""
    lods = []
    entries = mb_fields.get("lod") if "lod" in mb_fields else mb_fields.get("objects")
    for e in entries or []:
        objs = []
        for ref in e.get("objects") or []:
            if ref is None:
                continue
            k = (os.path.basename(ref["file"]), ref["pathID"])
            n = keymap_by_key.get(k)
            if n is None:
                ex.warn("lod object %s not found in the city tree" % ref.get("name"))
                continue
            objs.append(n)
        lods.append({"visibleRange": float(e["visibleRange"]), "keepActive": bool(e.get("KeepActive", False)), "objects": objs})
    return lods


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("data_dir", help="KSP_x64_Data")
    ap.add_argument("lib_dir", help="harness/lib (Assembly-CSharp.de4dot-original.dll for the MonoBehaviour type trees)")
    ap.add_argument("pqs_prefab", help="core/data/terrain/pqs_prefab.json (WP-11)")
    ap.add_argument("out_dir")
    ap.add_argument("--survey", action="store_true", help="print counts, write nothing")
    args = ap.parse_args()

    ex = Extractor(args.data_dir, args.lib_dir)
    prefab = json.load(open(args.pqs_prefab, encoding="utf-8"))

    cities = []
    for body in prefab["bodies"]:
        for mod in body.get("mods", []):
            if mod["class"] not in ("PQSCity", "PQSCity2"):
                continue
            mb_key = ("sharedassets9.assets", mod["pathID"])
            head = ex.objs[mb_key].parse_monobehaviour_head()
            go_key = ex.ptr_key(head.m_GameObject)
            keymap, mbs = {}, []
            root = ex.build(go_key, keymap, mbs)
            ex.replay(root, keymap, mbs, 0)
            nodes = ex.flatten(root, mod["gameObject"].split("/", 1)[1] if "/" in mod["gameObject"] else mod["gameObject"])
            # the keymap maps source keys to the Nodes that replaced them; rebuild a key -> Node map over the final tree
            by_key = {}
            for n in nodes:
                by_key.setdefault(n.key, n)
            lods = lod_from_prefab(ex, mod["fields"], by_key)
            cities.append((body["bodyName"], mod, go_key, nodes, lods))
            ncol = sum(len(n.colliders) for n in nodes)
            print("%-34s %-8s pid %5d: %5d objects, %4d colliders" % (mod["gameObject"], mod["class"], go_key[1], len(nodes), ncol))

    # PSystemSetup (level9): the stock launch sites and facilities; the "stock" bundle's additional prefabs are the set dressings
    setup = ex.psystem_setup()
    for ls in setup["stocklaunchsites"]:
        if ls["BundleName"] != "stock":
            continue
        for path in ls["additionalprefabPaths"]:
            leaf = path.split("/")[-1]
            go_key = ex.find_prefab_by_name(leaf)
            if go_key is None:
                ex.warn("additional prefab %s not found" % leaf)
                continue
            keymap, mbs = {}, []
            root = ex.build(go_key, keymap, mbs)
            ex.replay(root, keymap, mbs, 0)
            nodes = ex.flatten(root, leaf)
            mbk = [k for n_, k in ex.components(go_key) if n_ == "MonoBehaviour" and ex.mb_class(ex.objs[k]).endswith("PQSCity2")][0]
            fields = ex.mb_fields(ex.objs[mbk], "PQSCity2")
            ents = []
            for e in fields["objects"]:
                ents.append({"visibleRange": e["visibleRange"], "KeepActive": e["KeepActive"],
                             "objects": [{"file": k_[0], "pathID": k_[1], "name": ""} for k_ in
                                         (ex.dptr_key(go_key[0], r) for r in e["objects"]) if k_ is not None]})
            fields = dict(fields)
            fields["objects"] = ents
            mod = {"class": "PQSCity2", "gameObject": ls["pqsName"] + "/" + leaf, "fields": fields, "additionalPrefabOf": ls["name"]}
            by_key = {}
            for n in nodes:
                by_key.setdefault(n.key, n)
            lods = lod_from_prefab(ex, fields, by_key)
            cities.append((ls["pqsName"], mod, go_key, nodes, lods))
            print("%-34s %-8s pid %5d: %5d objects, %4d colliders (additional prefab of %s)" % (mod["gameObject"], "PQSCity2", go_key[1], len(nodes), sum(len(n.colliders) for n in nodes), ls["name"]))

    print("meshes %d (%d bytes), materials %d, warnings %d" % (len(ex.meshes), len(ex.mesh_blob), len(ex.materials), len(ex.warnings)))
    for w in ex.warnings[:40]:
        print("WARN", w)
    if args.survey:
        return 0

    objs_json_cities = []
    for body, mod, go_key, nodes, lods in cities:
        objs = []
        for n in nodes:
            o = {"path": n._path, "parent": n._parent_idx, "src": list(n.src), "active": n.active, "layer": n.layer, "tag": n.tag,
                 "position": n.pos, "rotation": n.rot, "scale": n.scl, "colliders": n.colliders}
            if n.crash is not None:
                o["crashObjectName"], o["crashDisplayName"] = n.crash
            if n.mobile_pad:
                o["mobileLaunchPad"] = True
            if n.via is not None:
                o["via"] = list(n.via)
            if n.level_group is not None:
                o["levelGroup"] = n.level_group
                o["level"] = n.level
            objs.append(o)
        root = nodes[0]
        destr = []
        for n in nodes:
            if n.destructible is not None:
                destr.append({"object": n.index, "id": n.destructible["id"],
                              "collapsibles": [{"collapse": c.index if c else -1, "replacement": r.index if r else -1,
                                                "collapseOffset": vec(co["collapseOffset"]),
                                                "collapseBehaviour": co["collapseBehaviour"]}
                                               for c, r, co in n.destructible["collapsibles"]]})
        cities_entry = {"name": mod["gameObject"], "class": mod["class"], "gameObjectPathID": go_key[1],
                        **({"additionalPrefabOf": mod["additionalPrefabOf"], "placement": {k: v for k, v in mod["fields"].items() if k not in ("objects", "sphere", "m_GameObject", "m_Script", "positionMobileLaunchPad", "crashObjectName", "PositioningPoint")}} if "additionalPrefabOf" in mod else {}),
                        "objects": objs, "children": [c.index for c in root.children],
                        "lod": [{"visibleRange": l["visibleRange"], "keepActive": l["keepActive"], "objects": [n.index for n in l["objects"]]} for l in lods],
                        "destructibles": destr}
        objs_json_cities.append(cities_entry)

    # the Ground material of the terrain (PQSMod_QuadMeshColliders.physicsMaterial) per body
    terrain = {}
    for body in prefab["bodies"]:
        for mod in body.get("mods", []):
            if mod["class"] == "PQSMod_QuadMeshColliders":
                ref = mod["fields"].get("physicsMaterial")
                if ref:
                    k = (os.path.basename(ref["file"]), ref["pathID"])
                    r = ex.objs[k]
                    m = r.read()
                    rec_id = ex.material_ids.get(k)
                    if rec_id is None:
                        rec_id = len(ex.materials)
                        ex.material_ids[k] = rec_id
                        ex.materials.append({"name": m.m_Name, "origin": {"file": k[0], "pathID": k[1]},
                                             "dynamicFriction": f32(m.dynamicFriction), "staticFriction": f32(m.staticFriction),
                                             "bounciness": f32(m.bounciness), "frictionCombine": int(m.frictionCombine),
                                             "bounceCombine": int(m.bounceCombine)})
                    terrain[body["bodyName"]] = rec_id
                else:
                    terrain[body["bodyName"]] = -1

    # PhysicsManager defaults (Physics.defaultMaterial, bounceThreshold, defaultContactOffset ...)
    physics = {}
    for o in ex.env.objects:
        if o.type.name == "PhysicsManager":
            t = o.read_typetree()
            for k in ("m_BounceThreshold", "m_DefaultContactOffset", "m_SleepThreshold", "m_ContactPairsMode", "m_FrictionType",
                      "m_EnableAdaptiveForce", "m_ClothInterCollisionDistance", "m_DefaultSolverIterations",
                      "m_DefaultSolverVelocityIterations", "m_Gravity"):
                if k in t:
                    v = t[k]
                    physics[k] = v if not isinstance(v, dict) else {a: b for a, b in v.items()}
            dm = t.get("m_DefaultMaterial")
            if dm:
                dk = ex.dptr_key(o.assets_file.name, dm)
                if dk is not None and dk in ex.objs:
                    m = ex.objs[dk].read()
                    physics["defaultMaterial"] = {"name": m.m_Name, "dynamicFriction": f32(m.dynamicFriction), "staticFriction": f32(m.staticFriction),
                                                  "bounciness": f32(m.bounciness), "frictionCombine": int(m.frictionCombine), "bounceCombine": int(m.bounceCombine)}
                else:
                    physics["defaultMaterial"] = None
            physics["origin"] = {"file": o.assets_file.name, "pathID": o.path_id}

    files = {}
    for fn in sorted(os.listdir(args.data_dir)):
        if fn.startswith(("level", "sharedassets", "globalgamemanagers", "resources")) and not fn.endswith((".resS", ".resource")):
            files[fn] = sha256_file(os.path.join(args.data_dir, fn))
    doc = {"schema": SCHEMA,
           "source": {"tool": "core/tools/mucollider/extract_scenery_colliders.py", "unityPy": UnityPy.__version__, "unityVersion": UNITY_VERSION,
                      "gameRoot": "decompiled/KSP-dev", "files": files},
           "meshFile": "scenery_meshes.bin", "meshes": ex.meshes, "materials": ex.materials, "terrainMaterial": terrain,
           "physicsManager": physics, "tags": ex.tags,
           "launchSites": [{k: ls[k] for k in ("name", "pqsName", "launchSiteTransformURL", "prefabPath", "additionalprefabPaths", "BundleName",
                                             "launchSiteName", "editorFacility", "nodeType", "requiresPOIVisit", "poiName")} |
                           {"spawnPoints": [{"name": sp["name"], "spawnTransformURL": sp["spawnTransformURL"]} for sp in ls["spawnPoints"]]}
                           for ls in setup["stocklaunchsites"]],
           "facilities": [{k: fa[k] for k in ("name", "pqsName", "facilityTransformName", "facilityName", "editorFacility")} |
                          {"spawnPoints": [{"name": sp["name"], "spawnTransformURL": sp["spawnTransformURL"]} for sp in fa["spawnPoints"]]}
                          for fa in setup["facilities"]],
           "psystemSetupOrigin": {"file": ex.setup_origin[0], "pathID": ex.setup_origin[1]}, "cities": objs_json_cities, "warnings": ex.warnings}
    os.makedirs(args.out_dir, exist_ok=True)
    with open(os.path.join(args.out_dir, "scenery_colliders.json"), "w", encoding="utf-8", newline="\n") as f:
        json.dump(doc, f, separators=(",", ":"))
    with open(os.path.join(args.out_dir, "scenery_meshes.bin"), "wb") as f:
        f.write(ex.mesh_blob)
    print("wrote", args.out_dir)
    return 0


if __name__ == "__main__":
    sys.exit(main())
