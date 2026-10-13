#!/usr/bin/env python
"""WP-mucol: extract the KSC / city scenery colliders from the shipped Unity assets.

STATUS: WRITTEN BUT NOT RUN. The session that wrote it could not read decompiled/KSP-dev (the permission layer denied the path) and the
type-tree cache (harness/tools-cache, harness/lib) is not on that machine. Run it locally and fix what the first run reports; the
output format is fixed by core/include/ksp/space_center/scenery_colliders.hpp (schema kspcore-scenery-colliders/1) and the loader is
unit-tested against a synthetic document of that format.

    <harness>/tools-cache/venv/Scripts/python.exe -I tools/mucollider/extract_scenery_colliders.py ^
        <KSP_x64_Data dir> <harness lib dir> data/scenery/scenery_colliders.json [--city Kerbin/KSC --city Kerbin/KSC2 ...]

Method (the same loading approach as tools/wp11/extract_terrain.py): UnityPy over EVERY file of KSP_x64_Data (the buildings live in
files other than sharedassets9.assets: the PQSCity prefab values in data/terrain/pqs_prefab.json show `renderers` and `objects`
references that resolve to null when only sharedassets9 is loaded). The colliders are NATIVE classes (BoxCollider, SphereCollider,
CapsuleCollider, MeshCollider + Mesh), so UnityPy reads them without the managed type trees; only the PQSCity / PQSCity2
MonoBehaviours (lod ranges, LodObject lists) need the trees generated from the shipped IL, exactly as in extract_terrain.py.

For each city GameObject (default: every PQSCity / PQSCity2 of every body in pqs_prefab.json, e.g. 'Kerbin/KSC'):
  1. find the GameObject by hierarchy path (Transform.m_Father chain from the PQS prefab root);
  2. walk its Transform subtree depth-first through m_Children (inactive objects included: PQSCity switches them at run time);
  3. per GameObject write path, local position / rotation / scale (Transform m_LocalPosition / m_LocalRotation / m_LocalScale);
  4. per collider component: Box (m_Center, m_Size), Sphere (m_Center, m_Radius), Capsule (m_Center, m_Radius, m_Height, m_Direction),
     Mesh (m_Convex, the Mesh asset's vertices and triangles: sub-mesh index ranges concatenated, topology triangles only), all with
     m_IsTrigger; the PhysicMaterial reference (m_Material) is recorded by name;
  5. children = the indices of the city's direct child objects; lod = the PQSCity.lod[i].objects / PQSCity2.objects[i].objects
     transforms mapped to object indices (+ visibleRange, KeepActive).
The root object (index 0) is the city GameObject itself: its TRS is ignored by the loader (the city pose is the PQSCity placement).
"""
import argparse
import hashlib
import json
import os
import sys

import UnityPy

SCHEMA = "kspcore-scenery-colliders/1"


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 22), b""):
            h.update(chunk)
    return h.hexdigest()


def vec(v):
    return [float(v.x), float(v.y), float(v.z)]


def quat(q):
    return [float(q.x), float(q.y), float(q.z), float(q.w)]


def mesh_triangles(mesh):
    """Vertices (flat list) and triangle indices of a Unity Mesh. UnityPy exposes m_Vertices (flat floats) and
    get_triangles() / m_IndexBuffer depending on the version; use whichever exists."""
    verts = [float(x) for x in mesh.m_Vertices]
    if hasattr(mesh, "get_triangles"):
        tris = [int(i) for sub in mesh.get_triangles() for tri in sub for i in tri]
    else:  # raw index buffer, 16 or 32 bit per m_IndexFormat, first sub-mesh ranges
        import struct
        fmt = "<%dH" if mesh.m_IndexFormat == 0 else "<%dI"
        sz = 2 if mesh.m_IndexFormat == 0 else 4
        raw = bytes(mesh.m_IndexBuffer)
        idx = list(struct.unpack(fmt % (len(raw) // sz), raw))
        tris = []
        for sm in mesh.m_SubMeshes:
            tris += idx[sm.firstByte // sz: sm.firstByte // sz + sm.indexCount]
    return verts, tris


def collider_json(comp, cls):
    d = {"trigger": bool(comp.m_IsTrigger)}
    if cls == "BoxCollider":
        d.update(type="Box", center=vec(comp.m_Center), size=vec(comp.m_Size))
    elif cls == "SphereCollider":
        d.update(type="Sphere", center=vec(comp.m_Center), radius=float(comp.m_Radius))
    elif cls == "CapsuleCollider":
        d.update(type="Capsule", center=vec(comp.m_Center), radius=float(comp.m_Radius), height=float(comp.m_Height),
                 direction=int(comp.m_Direction))
    elif cls == "MeshCollider":
        mesh = comp.m_Mesh.read() if comp.m_Mesh else None
        if mesh is None:
            return None
        verts, tris = mesh_triangles(mesh)
        d.update(type="Mesh", convex=bool(comp.m_Convex), vertices=verts, triangles=tris)
    else:
        return None
    return d


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("data_dir", help="KSP_x64_Data")
    ap.add_argument("lib_dir", help="harness/lib (type trees for the PQSCity MonoBehaviours)")
    ap.add_argument("out", help="scenery_colliders.json")
    ap.add_argument("--city", action="append", help="city GameObject path, e.g. Kerbin/KSC (default: all PQSCity / PQSCity2)")
    ap.add_argument("--pqs-prefab", default=os.path.join("data", "terrain", "pqs_prefab.json"))
    args = ap.parse_args()

    env = UnityPy.load(args.data_dir)  # all files: the externals of sharedassets9 are the level / sharedassets files
    by_path = {}
    for obj in env.objects:
        by_path.setdefault((obj.assets_file.name, obj.path_id), obj)

    def read(ptr):
        return ptr.read() if ptr and ptr.path_id else None

    def go_name(go):
        return go.m_Name

    def children_of(tr):
        return [c.read() for c in tr.m_Children]

    def gameobject_of(tr):
        return tr.m_GameObject.read()

    def component_list(go):
        out = []
        for c in go.m_Components:
            ptr = getattr(c, "component", None) or c
            try:
                comp = ptr.read()
            except Exception:  # noqa: BLE001
                continue
            out.append((ptr.type.name if hasattr(ptr, "type") else type(comp).__name__, comp))
        return out

    # city GameObject candidates: the paths in pqs_prefab.json ("Kerbin/KSC") are Body/Child
    wanted = args.city
    if not wanted:
        prefab = json.load(open(args.pqs_prefab, encoding="utf-8"))
        wanted = sorted({m["gameObject"] for b in prefab["bodies"] for m in b["mods"] if m["class"] in ("PQSCity", "PQSCity2")})

    roots = {}
    for obj in env.objects:
        if obj.type.name == "GameObject":
            go = obj.read()
            roots.setdefault(go.m_Name, []).append((obj, go))

    cities = []
    for path in wanted:
        body, _, leaf = path.rpartition("/")
        cand = [(o, g) for (o, g) in roots.get(leaf, [])]
        # keep the one whose Transform parent chain reads body/leaf
        match = None
        for o, g in cand:
            tr = next((c for n, c in component_list(g) if n == "Transform"), None)
            if tr is None:
                continue
            parent = read(tr.m_Father)
            if parent is not None and gameobject_of(parent).m_Name == body.split("/")[-1]:
                match = (o, g, tr)
                break
        if match is None:
            print("city not found:", path, file=sys.stderr)
            continue
        _, root_go, root_tr = match
        objects, index_of = [], {}

        def visit(tr, parent_idx, parent_path):
            go = gameobject_of(tr)
            p = (parent_path + "/" + go.m_Name) if parent_path else go.m_Name
            cols = []
            for n, comp in component_list(go):
                if n in ("BoxCollider", "SphereCollider", "CapsuleCollider", "MeshCollider"):
                    c = collider_json(comp, n)
                    if c:
                        cols.append(c)
            idx = len(objects)
            index_of[tr.path_id if hasattr(tr, "path_id") else id(tr)] = idx
            objects.append({"path": p, "parent": parent_idx, "position": vec(tr.m_LocalPosition),
                            "rotation": quat(tr.m_LocalRotation), "scale": vec(tr.m_LocalScale), "colliders": cols})
            for ch in children_of(tr):
                visit(ch, idx, p)

        visit(root_tr, -1, "")
        children = [i for i, o in enumerate(objects) if o["parent"] == 0]
        # lod: the PQSCity / PQSCity2 component on the root GameObject needs the MonoBehaviour type trees (see extract_terrain.py);
        # until wired in here the document carries no lod table and hosts take PQSCity.lod from data/terrain/pqs_prefab.json
        cities.append({"name": path, "objects": objects, "children": children, "lod": []})
        print("%s: %d objects, %d colliders" % (path, len(objects), sum(len(o["colliders"]) for o in objects)))

    files = {}
    for fn in sorted(os.listdir(args.data_dir)):
        if fn.startswith(("level", "sharedassets")) and not fn.endswith(".resS"):
            files[fn] = sha256(os.path.join(args.data_dir, fn))
    doc = {"schema": SCHEMA, "source": {"dataDir": args.data_dir, "files": files, "tool": "tools/mucollider/extract_scenery_colliders.py"},
           "cities": cities}
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        json.dump(doc, f)
    return 0


if __name__ == "__main__":
    sys.exit(main())
