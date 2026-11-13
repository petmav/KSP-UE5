"""KSC render tree for the WP-UE-06 extractor: the GameObject tree WP-12c extracted the colliders of, with its renderers.

It replays the same spawners as core/tools/mucollider/extract_scenery_colliders.py (the render side must hang on the SAME objects):
* NestedPrefabSpawner (decompiled/KSP-Decompiled/Assembly-CSharp/NestedPrefabSpawner.cs:20 Spawn, :60 setLayersAndTagsRecursive):
  Instantiate(prefab), parent = the target's parent (appended last), the target's local TRS and name, the new subtree's layers / tags
  defaulted to the target's, the target removed;
* Upgradeables.UpgradeableObject (UpgradeableObject.cs:9, UpgradeLevel.Spawn): one prefab per facility level, placed at the "Facility"
  transform's TRS under its parent, named like it, replacing it; every level is kept ("level groups");
and flattens the result depth first, children in transform order. `check_against_colliders` proves the flat order, names, source
GameObject keys, active / layer / tag and the float32 bits of the local TRS equal core/data/scenery/scenery_colliders.json (the
committed WP-12c output), so index i of the render tree is index i of the collider tree.
"""
import json
import os
import struct

COLLIDER_TYPES = ("BoxCollider", "SphereCollider", "CapsuleCollider", "MeshCollider")


def f32bits(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


class Node:
    def __init__(self, key, name, active, layer, tag, pos, rot, scl):
        self.key = key
        self.src = key
        self.via = None
        self.name = name
        self.active = active
        self.layer = layer
        self.tag = tag
        self.pos, self.rot, self.scl = pos, rot, scl
        self.children = []
        self.parent = None
        self.renderers = []         # dicts (see build)
        self.level_group = None
        self.level = -1
        self.index = -1

    def add(self, c):
        c.parent = self
        self.children.append(c)

    def remove(self, c):
        self.children.remove(c)
        c.parent = None


def vec3(d):
    return [d["x"], d["y"], d["z"]]


class Scenery:
    def __init__(self, game):
        self.g = game
        self.warnings = []

    def warn(self, m):
        self.warnings.append(m)

    # ---- tree ---------------------------------------------------------------------------------------------------------------
    def build(self, go_key, keymap, mbs):
        g = self.g
        go = g.tt(go_key)
        comps = g.components(go_key)
        tr_key = None
        for n, k in comps:
            if n == "Transform":
                tr_key = k
        tr = g.tt(tr_key)
        node = Node(go_key, go["m_Name"], bool(go["m_IsActive"]), int(go["m_Layer"]), g.tag_name(int(go["m_Tag"])),
                    vec3(tr["m_LocalPosition"]), [tr["m_LocalRotation"][a] for a in "xyzw"], vec3(tr["m_LocalScale"]))
        keymap[go_key] = node
        keymap[tr_key] = node
        mesh_key = None
        for n, k in comps:
            if n == "MeshFilter":
                mesh_key = g.ptr(k[0], g.tt(k)["m_Mesh"])
        for n, k in comps:
            if n in ("MeshRenderer", "SkinnedMeshRenderer"):
                t = g.tt(k)
                mats = [g.ptr(k[0], m) for m in t["m_Materials"]]
                rk = {"component": n, "pathID": k[1], "file": k[0], "enabled": bool(t["m_Enabled"]), "castShadows": int(t["m_CastShadows"]),
                      "receiveShadows": bool(t["m_ReceiveShadows"]), "materials": mats}
                if n == "MeshRenderer":
                    rk["mesh"] = mesh_key
                    sb = t.get("m_StaticBatchInfo") or {}
                    if sb.get("subMeshCount"):
                        self.warn("%s: static batched renderer" % go["m_Name"])
                else:
                    rk["mesh"] = g.ptr(k[0], t["m_Mesh"])
                node.renderers.append(rk)
            elif n == "MonoBehaviour":
                cls = g.mb_class(k)
                if cls in ("NestedPrefabSpawner", "Upgradeables.UpgradeableFacility", "Upgradeables.UpgradeableSlave"):
                    mbs.append((node, cls, g.mb_fields(k, cls), k))
        for ch in tr["m_Children"]:
            ctr = g.ptr(go_key[0], ch)
            cgo = g.ptr(ctr[0], g.tt(ctr)["m_GameObject"])
            node.add(self.build(cgo, keymap, mbs))
        return node

    def instantiate(self, go_key, depth=0):
        if depth > 8:
            raise RuntimeError("nested prefab recursion")
        keymap, mbs = {}, []
        root = self.build(go_key, keymap, mbs)
        self.replay(root, keymap, mbs, depth)
        return root

    def set_layers_tags(self, node, default_layer, allowed_tags, default_tag, set_tags):
        if node.layer == 0:
            node.layer = default_layer
        if set_tags and node.tag not in allowed_tags:
            node.tag = default_tag
        for c in node.children:
            self.set_layers_tags(c, default_layer, allowed_tags, default_tag, set_tags)

    def replay(self, root, keymap, mbs, depth):
        g = self.g
        for node, cls, f, mk in mbs:
            sf = mk[0]
            if cls == "NestedPrefabSpawner":
                if f["SpawnOn"] == 0:
                    continue
                allowed_tags = list(f["allowedTags"])
                for p in f["Prefabs"]:
                    tkey = g.ptr(sf, p["tgtTransform"])
                    pkey = g.ptr(sf, p["prefab"])
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
            else:
                fac = keymap.get(g.ptr(sf, f["facilityTransform"]))
                if fac is None or fac.parent is None:
                    self.warn("%s %s: no facilityTransform parent" % (cls, node.name))
                    continue
                parent = fac.parent
                group = f["id"] or ("%s#%d" % (node.name, mk[1]))
                parent.remove(fac)
                for lvl, ul in enumerate(f["upgradeLevels"]):
                    pkey = g.ptr(sf, ul["facilityPrefab"])
                    if pkey is None:
                        self.warn("%s level %d has no prefab" % (group, lvl))
                        continue
                    new = self.instantiate(pkey, depth + 1)
                    new.name, new.pos, new.rot, new.scl = fac.name, fac.pos, fac.rot, fac.scl
                    new.level_group, new.level = group, lvl
                    new.via = fac.src
                    parent.add(new)

    def city(self, city_name, pqs_prefab_json):
        """(flat nodes) of the PQSCity / PQSCity2 object named like core/data/terrain/pqs_prefab.json's `gameObject` field."""
        g = self.g
        doc = json.load(open(pqs_prefab_json, encoding="utf-8"))
        for body in doc["bodies"]:
            for mod in body.get("mods", []):
                if mod["class"] in ("PQSCity", "PQSCity2") and mod["gameObject"] == city_name:
                    mb_key = ("sharedassets9.assets", mod["pathID"])
                    go_key = g.ptr(mb_key[0], g.tt(mb_key)["m_GameObject"])
                    keymap, mbs = {}, []
                    root = self.build(go_key, keymap, mbs)
                    self.replay(root, keymap, mbs, 0)
                    out = []

                    def visit(n, parent_idx, path):
                        n.index = len(out)
                        n._parent = parent_idx
                        n._path = path
                        out.append(n)
                        for c in n.children:
                            visit(c, n.index, path + "/" + c.name)
                    visit(root, -1, city_name.split("/", 1)[1] if "/" in city_name else city_name)
                    return out, go_key
        raise KeyError(city_name)


def check_against_colliders(nodes, go_key, scenery_json, city_name):
    """Mismatch strings between the render tree and the committed WP-12c collider tree of the same city (empty = identical)."""
    doc = json.load(open(scenery_json, encoding="utf-8"))
    city = [c for c in doc["cities"] if c["name"] == city_name and c["gameObjectPathID"] == go_key[1]]
    if len(city) != 1:
        return ["city %s pathID %d not found in %s" % (city_name, go_key[1], os.path.basename(scenery_json))]
    objs = city[0]["objects"]
    bad = []
    if len(objs) != len(nodes):
        bad.append("object count %d vs %d" % (len(nodes), len(objs)))
    for n, o in zip(nodes, objs):
        w = "#%d %s" % (n.index, n._path[-40:])
        if n._path != o["path"] or n._parent != o["parent"]:
            bad.append(w + " path / parent")
        if list(n.src) != o["src"] or (n.via is not None) != ("via" in o):
            bad.append(w + " src / via")
        if n.active != o["active"] or n.layer != o["layer"] or n.tag != o["tag"]:
            bad.append(w + " active / layer / tag")
        if [f32bits(x) for x in n.pos] != [f32bits(x) for x in o["position"]] or [f32bits(x) for x in n.scl] != [f32bits(x) for x in o["scale"]] \
                or [f32bits(x) for x in n.rot] != [f32bits(x) for x in o["rotation"]]:
            bad.append(w + " TRS bits")
        if (n.level_group, n.level) != (o.get("levelGroup"), o.get("level", -1)):
            bad.append(w + " level group")
    return bad
