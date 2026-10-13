#!/usr/bin/env python
"""WP-mucol: independent reference for the .mu collider chunks and the part-space collider shapes.

Run:  python -I core/tools/mucollider/mu_colliders_reference.py [--staging DIR] [--data-dir core/tests/data]

Input  : core/data/mu_staging/<partName>/{*.cfg,*.mu} + MANIFEST.json (staged originals; manifest = GameData origin + sha256).
Output : core/tests/data/mucollider_reference.json (per part: every collider chunk with its raw values as IEEE bit patterns,
         and the part-space shape computed in float32 with one rounding per operation).

Independence: this file shares NO code with core/src (the C++ reader). The grammar of the chunks the reader has to SKIP (meshes,
materials, animation ...) is taken from core/tools/wp17/mu_transforms.py (WP-17b; verified there to end every staged file exactly
at EOF); everything about COLLIDERS (the chunk layouts, defaults, the part-space maths) is written here from the spec:

  PartReader.cs:229 MeshCollider (bool convex, mesh; ALWAYS forced convex :233), :240 SphereCollider (radius, center), :247
    CapsuleCollider (radius, direction, center; no height field: the component keeps Unity's default 2), :255 BoxCollider (size,
    center), :301 MeshCollider2 (isTrigger, convex, mesh), :314 SphereCollider2 (isTrigger, radius, center), :322 CapsuleCollider2
    (isTrigger, radius, height, direction, center), :332 BoxCollider2 (isTrigger, size, center), :340 WheelCollider (not a shape
    here: enabled = false), :298 TagAndLayer.
  PartLoader.cs:1765 CompileModel places the .mu root: no MODEL node -> root renamed "model", scale = rescaleFactor; MODEL nodes ->
    a synthetic "model" (scale = rescaleFactor), each .mu root below it (NestToParent resets pos / rot, scale = cfg scale or one,
    then cfg position / rotation (Euler degrees)).
  Unity collider scaling: world shape = collider shape x lossyScale (no shear: component-wise product of the local scales down the
    chain).
"""
import argparse
import json
import math
import os
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "wp17"))
import mu_transforms as w17  # noqa: E402  (skipper grammar only; see the module docstring)

F = np.float32
f2b, b2f, qrot = w17.f_to_bits, w17.bits_to_f, w17.qrot


def qmul(a, b):
    """Quaternionf operator* of the native Transform code as core/src/unity/quaternion_native.cpp Multiply has it (the WP-17b fit,
    UNVERIFIED against Unity): x = ((ay*bz - az*by) + aw*bx) + bw*ax (y, z cyclic); w = (aw*bw - ax*bx) - (ay*by + az*bz).
    (core/tools/wp17/mu_transforms.py qmul associates w as aw*bw - ((ax*bx + ay*by) + az*bz): different rounding, same value.)"""
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    cx = F(F(ay * bz) - F(az * by))
    cy = F(F(az * bx) - F(ax * bz))
    cz = F(F(ax * by) - F(ay * bx))
    return (F(F(cx + F(aw * bx)) + F(bw * ax)),
            F(F(cy + F(aw * by)) + F(bw * ay)),
            F(F(cz + F(aw * bz)) + F(bw * az)),
            F(F(F(aw * bw) - F(ax * bx)) - F(F(ay * by) + F(az * bz))))


def h(f):
    return "%08x" % f2b(f)


def hv(v):
    return [h(x) for x in v]


class Collider:
    def __init__(self, entry, trigger, convex, params, mesh):
        self.entry, self.trigger, self.convex, self.params, self.mesh = entry, trigger, convex, params, mesh


class Node(w17.Node):
    def __init__(self, *a):
        super().__init__(*a)
        self.colliders = []
        self.tag = None
        self.layer = None


class ColliderMu(w17.MuFile):
    """MuFile whose ReadChild keeps the collider chunks (everything else is consumed by the WP-17b skippers)."""

    def read_mesh(self):  # PartReader.cs:495; keeps the vertices and the triangle lists
        r = self.r
        if r.i32() != w17.MeshStart:
            raise ValueError("Mesh Error")
        n = r.i32()
        r.i32()
        verts, tris = None, []
        while True:
            et = r.i32()
            if et == w17.MeshEnd:
                break
            if et == w17.MeshVertexColors:
                r.take(4 * n)
            elif et in (w17.MeshVerts, w17.MeshNormals):
                raw = r.take(12 * n)
                if et == w17.MeshVerts:
                    verts = raw
            elif et in (w17.MeshUV, w17.MeshUV2):
                r.take(8 * n)
            elif et == w17.MeshTangents:
                r.take(16 * n)
            elif et == w17.MeshTriangles:
                tris.append(r.take(4 * r.i32()))
            elif et == w17.MeshBoneWeights:
                r.take(32 * n)
            elif et == w17.MeshBindPoses:
                r.skip_floats(16 * r.i32())
        return {"count": n, "verts": verts or b"", "tris": tris}

    def read_child(self, parent):
        r = self.r
        name = r.string()
        pos = [r.f32bits() for _ in range(3)]
        rot = [r.f32bits() for _ in range(4)]
        scl = [r.f32bits() for _ in range(3)]
        node = Node(name, pos, rot, scl, parent)
        E = w17
        fl = lambda n: [r.f32bits() for _ in range(n)]
        while not r.eof():
            et = r.i32()
            if et == E.ChildTransformStart:
                node.children.append(self.read_child(node))
            elif et == E.Animation:
                self.read_animation()
            elif et == E.MeshCollider:
                node.colliders.append(Collider(et, False, True, {"convexFlag": r.boolean()}, self.read_mesh()))
            elif et == E.SphereCollider:
                node.colliders.append(Collider(et, False, None, {"radius": fl(1), "center": fl(3)}, None))
            elif et == E.CapsuleCollider:
                node.colliders.append(Collider(et, False, None, {"radius": fl(1), "direction": r.i32(), "center": fl(3),
                                                                  "height": [f2b(F(2))]}, None))
            elif et == E.BoxCollider:
                node.colliders.append(Collider(et, False, None, {"size": fl(3), "center": fl(3)}, None))
            elif et == E.MeshFilter:
                self.read_mesh()
            elif et == E.MeshRenderer:
                self.read_mesh_renderer()
            elif et == E.SkinnedMeshRenderer:
                self.read_skinned_mesh_renderer()
            elif et == E.Materials:
                for _ in range(r.i32()):
                    self.read_material() if self.version < 4 else self.read_material4()
            elif et == E.Textures:
                self.read_textures()
            elif et == E.Light:
                self.read_light()
            elif et == E.TagAndLayer:
                node.tag = r.string()
                node.layer = r.i32()
            elif et == E.MeshCollider2:
                trig = r.boolean()
                conv = r.boolean()
                node.colliders.append(Collider(et, trig, True, {"convexFlag": conv}, self.read_mesh()))
            elif et == E.SphereCollider2:
                trig = r.boolean()
                node.colliders.append(Collider(et, trig, None, {"radius": fl(1), "center": fl(3)}, None))
            elif et == E.CapsuleCollider2:
                trig = r.boolean()
                node.colliders.append(Collider(et, trig, None, {"radius": fl(1), "height": fl(1), "direction": r.i32(),
                                                                  "center": fl(3)}, None))
            elif et == E.BoxCollider2:
                trig = r.boolean()
                node.colliders.append(Collider(et, trig, None, {"size": fl(3), "center": fl(3)}, None))
            elif et == E.WheelCollider:
                r.skip_floats(3 + 3 + 3 + 5 + 5)
                node.colliders.append(Collider(et, False, None, {}, None))
            elif et == E.Camera:
                self.read_camera()
            elif et == E.ParticleEmitter:
                self.read_particles()
            elif et == E.ChildTransformEnd:
                return node
        return node


ENTRY_NAMES = {w17.MeshCollider: "MeshCollider", w17.SphereCollider: "SphereCollider", w17.CapsuleCollider: "CapsuleCollider",
               w17.BoxCollider: "BoxCollider", w17.MeshCollider2: "MeshCollider2", w17.SphereCollider2: "SphereCollider2",
               w17.CapsuleCollider2: "CapsuleCollider2", w17.BoxCollider2: "BoxCollider2", w17.WheelCollider: "WheelCollider"}


def world_trs(chain):
    """pos / rot / scale of the last node of `chain` relative to the part (the part transform is the identity)."""
    pos, rot, scl = (F(0),) * 3, (F(0), F(0), F(0), F(1)), (F(1),) * 3
    for n in chain:
        lp = tuple(b2f(b) for b in n.pos)
        r = qrot(rot, (F(scl[0] * lp[0]), F(scl[1] * lp[1]), F(scl[2] * lp[2])))
        pos = (F(pos[0] + r[0]), F(pos[1] + r[1]), F(pos[2] + r[2]))
        rot = qmul(rot, tuple(b2f(b) for b in n.rot))
        ls = tuple(b2f(b) for b in n.scl)
        scl = (F(scl[0] * ls[0]), F(scl[1] * ls[1]), F(scl[2] * ls[2]))
    return pos, rot, scl


def hull_mass(v):
    """KSP colliders are convex (PartReader.cs:233): PhysX / Unity use the HULL of the vertices. Qhull (scipy) hull, faces oriented
    outward, then the same solid integrals as mesh_mass (volume, centroid, inertia about the centroid per unit mass)."""
    from scipy.spatial import ConvexHull
    h = ConvexHull(v)
    tri = []
    for simplex, eq in zip(h.simplices, h.equations):
        a, b, c = v[simplex[0]], v[simplex[1]], v[simplex[2]]
        if np.dot(np.cross(b - a, c - a), eq[:3]) < 0:
            simplex = [simplex[0], simplex[2], simplex[1]]
        tri.append(simplex)
    out = mesh_mass(v, np.array(tri))
    out["qhullVolume"] = repr(float(h.volume))
    out["hullFaces"] = len(tri)
    return out


def mesh_mass(v, tri):
    """Volume, centroid and inertia about the centroid per unit mass of the closed triangle mesh as a solid, float64, tetrahedra
    against the ORIGIN (the C++ side integrates about the vertex mean: a different reference point and summation)."""
    a, b, c = v[tri[:, 0]], v[tri[:, 1]], v[tri[:, 2]]
    vol = np.einsum("ij,ij->i", a, np.cross(b, c)) / 6.0
    sg = -1.0 if vol.sum() < 0 else 1.0
    vol = vol * sg
    V = vol.sum()
    sv = a + b + c
    cm = (vol[:, None] * sv / 4.0).sum(axis=0) / V
    C = np.zeros((3, 3))
    for x in (a, b, c, sv):
        C += np.einsum("i,ij,ik->jk", vol / 20.0, x, x)
    io = np.trace(C) * np.eye(3) - C
    ic = io - V * ((cm @ cm) * np.eye(3) - np.outer(cm, cm))
    return {"volume": repr(float(V)), "centroid": [repr(float(x)) for x in cm],
            "inertiaPerUnitMass": [[repr(float(x)) for x in row] for row in (ic / V)]}


def part_space(col, chain):
    pos, rot, scl = world_trs(chain)
    P = {k: [b2f(b) for b in v] for k, v in col.params.items() if k not in ("direction", "convexFlag")}
    out = {"nodePosition": hv(pos), "nodeRotation": hv(rot), "nodeScale": hv(scl)}
    sc = lambda v: (F(scl[0] * v[0]), F(scl[1] * v[1]), F(scl[2] * v[2]))
    if "center" in P:
        r = qrot(rot, sc(P["center"]))
        out["center"] = hv((F(pos[0] + r[0]), F(pos[1] + r[1]), F(pos[2] + r[2])))
    a = [abs(x) for x in scl]
    if col.entry in (w17.BoxCollider, w17.BoxCollider2):
        s = sc(P["size"])
        out["halfExtents"] = hv([F(abs(s[i]) * F(0.5)) for i in range(3)])
    elif col.entry in (w17.SphereCollider, w17.SphereCollider2):
        out["radius"] = h(F(P["radius"][0] * max(a)))
    elif col.entry in (w17.CapsuleCollider, w17.CapsuleCollider2):
        d = col.params["direction"]
        ra = F(P["radius"][0] * max(a[i] for i in range(3) if i != d))
        ht = F(P["height"][0] * a[d])
        out["radius"] = h(ra)
        out["height"] = h(ht)
        out["halfCylinder"] = h(max(F(0), F(F(ht * F(0.5)) - ra)))
        out["axis"] = d
    elif col.mesh is not None:
        v = np.frombuffer(col.mesh["verts"], dtype="<f4").reshape(-1, 3)
        pv = []
        for i in range(len(v)):
            r = qrot(rot, sc(tuple(F(x) for x in v[i])))
            pv.append((F(pos[0] + r[0]), F(pos[1] + r[1]), F(pos[2] + r[2])))
        arr = np.array(pv, dtype="<f4").reshape(-1, 3)
        out["vertexFnv1a64"] = "%016x" % fnv1a64(arr.tobytes())
        out["massProps"] = hull_mass(arr.astype(np.float64))
        tri = np.concatenate([np.frombuffer(t, dtype="<i4") for t in col.mesh["tris"]]).reshape(-1, 3)
        out["meshSolidVolume"] = repr(abs(float(mesh_mass(arr.astype(np.float64), tri)["volume"])))
    return out


def fnv1a64(data):
    hv_ = 0xcbf29ce484222325
    for b in data:
        hv_ = ((hv_ ^ b) * 0x100000001b3) & 0xFFFFFFFFFFFFFFFF
    return hv_


def raw_params(col):
    out = {}
    for k, v in col.params.items():
        out[k] = v if k in ("direction", "convexFlag") else ["%08x" % b for b in v]
    return out


def collider_json(col, node, chain):
    j = {"entry": ENTRY_NAMES[col.entry], "trigger": col.trigger, "tag": getattr(node, "tag", None),
         "layer": getattr(node, "layer", None), "path": "/".join(n.name for n in chain),
         "isShape": col.entry != w17.WheelCollider}
    if col.entry != w17.WheelCollider:
        j["params"] = raw_params(col)
        if col.mesh is not None:
            j["mesh"] = {"count": col.mesh["count"], "vertsFnv1a64": "%016x" % fnv1a64(col.mesh["verts"]),
                         "triangleLists": [{"count": len(t) // 4, "fnv1a64": "%016x" % fnv1a64(t)} for t in col.mesh["tris"]]}
            j["convex"] = True
        j["partSpace"] = part_space(col, chain)
    return j


def walk(node, chain, out):
    chain = chain + [node]
    for c in getattr(node, "colliders", []):
        out.append(collider_json(c, node, chain))
    for ch in node.children:
        walk(ch, chain, out)


def count_nodes(node):
    return 1 + sum(count_nodes(c) for c in node.children)


def read_mu(data):
    mf = ColliderMu(data)
    root = mf.read()
    if not mf.r.eof():
        raise SystemExit("reader stopped at %d of %d" % (mf.r.p, len(data)))
    return mf, root


# ---------------------------------------------------------------------------------------------------------------------
# A general PartLoader.CompileModel (PartLoader.cs:1765): any number of MODEL nodes with parent / name / scale / position
# (rotation is Quaternion.Euler: native, not needed by the cases written here).

def find_child(node, path):
    cur = node
    for part in path.split("/"):
        nxt = None
        for c in cur.children:
            if c.name == part:
                nxt = c
                break
        if nxt is None:
            return None
        cur = nxt
    return cur


def build_chain_general(read_roots, rescale, models):
    """read_roots: one freshly parsed .mu root per MODEL node (or one for the no-MODEL case)."""
    one, zero = f2b(F(1)), f2b(F(0))
    ident = [zero, zero, zero, one]
    if not models:
        root = read_roots[0]
        root.name, root.parent = "model", None
        root.pos, root.rot, root.scl = [zero] * 3, ident, [f2b(F(rescale))] * 3
        return root
    model = w17.Node("model", [zero] * 3, ident, [f2b(F(rescale))] * 3, None)
    for root, m in zip(read_roots, models):
        root.pos, root.rot = [zero] * 3, ident
        root.scl = [f2b(F(x)) for x in m["scale"]] if "scale" in m else [one] * 3
        if "name" in m:
            root.name = m["name"]
        if "position" in m:
            root.pos = [f2b(F(x)) for x in m["position"]]
        host = find_child(model, m["parent"]) if "parent" in m else None
        host = host or model
        root.parent = host
        host.children.append(root)
    return model


# ---------------------------------------------------------------------------------------------------------------------
# .mu writer (the format PartReader.cs reads), used for (a) the reduced real models: hierarchy + colliders + TagAndLayer only,
# and (b) the synthetic coverage model. Floats are written from bit patterns.

class W:
    def __init__(self):
        self.b = bytearray()

    def i32(self, v):
        self.b += struct.pack("<i", v)

    def u32(self, v):
        self.b += struct.pack("<I", v)

    def f(self, v):
        self.b += struct.pack("<f", float(F(v)))

    def bools(self, v):
        self.b += b"\x01" if v else b"\x00"

    def s(self, v):
        raw = v.encode("utf-8")
        n = len(raw)
        out = bytearray()
        while True:
            byte = n & 0x7F
            n >>= 7
            if n:
                out.append(byte | 0x80)
            else:
                out.append(byte)
                break
        self.b += out + raw

    def raw(self, data):
        self.b += data


def write_mesh(w, col_mesh, with_extras=False):
    w.i32(w17.MeshStart)
    w.i32(col_mesh["count"])
    w.i32(0)
    w.i32(w17.MeshVerts)
    w.raw(col_mesh["verts"])
    if with_extras:  # normals + uv + tangents: skipped by the readers
        w.i32(w17.MeshNormals)
        w.raw(b"\x00" * (12 * col_mesh["count"]))
        w.i32(w17.MeshUV)
        w.raw(b"\x00" * (8 * col_mesh["count"]))
        w.i32(w17.MeshTangents)
        w.raw(b"\x00" * (16 * col_mesh["count"]))
    for t in col_mesh["tris"]:
        w.i32(w17.MeshTriangles)
        w.i32(len(t) // 4)
        w.raw(t)
    w.i32(w17.MeshEnd)


def write_collider(w, c, extras=False):
    E = w17
    p = c.params
    bits = lambda k: p[k]
    if c.entry == E.MeshCollider:
        w.i32(c.entry); w.bools(p["convexFlag"]); write_mesh(w, c.mesh, extras)
    elif c.entry == E.MeshCollider2:
        w.i32(c.entry); w.bools(c.trigger); w.bools(p["convexFlag"]); write_mesh(w, c.mesh, extras)
    elif c.entry in (E.SphereCollider, E.SphereCollider2):
        w.i32(c.entry)
        if c.entry == E.SphereCollider2:
            w.bools(c.trigger)
        for b in bits("radius") + bits("center"):
            w.u32(b)
    elif c.entry == E.CapsuleCollider:
        w.i32(c.entry)
        for b in bits("radius"):
            w.u32(b)
        w.i32(p["direction"])
        for b in bits("center"):
            w.u32(b)
    elif c.entry == E.CapsuleCollider2:
        w.i32(c.entry); w.bools(c.trigger)
        for b in bits("radius") + bits("height"):
            w.u32(b)
        w.i32(p["direction"])
        for b in bits("center"):
            w.u32(b)
    elif c.entry in (E.BoxCollider, E.BoxCollider2):
        w.i32(c.entry)
        if c.entry == E.BoxCollider2:
            w.bools(c.trigger)
        for b in bits("size") + bits("center"):
            w.u32(b)
    elif c.entry == E.WheelCollider:
        w.i32(c.entry)
        for _ in range(19):
            w.f(0.5)
    else:
        raise ValueError(c.entry)


def write_node(w, node, extras_fn=None, extras=False):
    w.s(node.name)
    for b in node.pos + node.rot + node.scl:
        w.u32(b)
    if getattr(node, "tag", None) is not None:
        w.i32(w17.TagAndLayer); w.s(node.tag); w.i32(node.layer)
    if extras_fn:
        extras_fn(w, node)
    for c in getattr(node, "colliders", []):
        write_collider(w, c, extras)
    for ch in node.children:
        w.i32(w17.ChildTransformStart)
        write_node(w, ch, extras_fn, extras)
        w.i32(w17.ChildTransformEnd)


def write_mu(version, root, extras_fn=None, extras=False):
    w = W()
    w.i32(w17.FILE_TYPE_MODEL_BINARY)
    w.i32(version)
    w.s("kspcore mucollider test model")
    write_node(w, root, extras_fn, extras)
    return bytes(w.b)


def mk_node(name, pos=(0, 0, 0), rot=(0, 0, 0, 1), scl=(1, 1, 1), tag=None, layer=None):
    n = Node(name, [f2b(F(x)) for x in pos], [f2b(F(x)) for x in rot], [f2b(F(x)) for x in scl], None)
    n.tag, n.layer = tag, layer
    return n


def add(parent, child):
    child.parent = parent
    parent.children.append(child)
    return child


def mk_collider(entry, trigger=False, **kw):
    P = {}
    for k, v in kw.items():
        if k == "mesh":
            continue
        if k in ("direction", "convexFlag"):
            P[k] = v
        else:
            P[k] = [f2b(F(x)) for x in (v if isinstance(v, (list, tuple)) else [v])]
    return Collider(entry, trigger, True if entry in (w17.MeshCollider, w17.MeshCollider2) else None, P, kw.get("mesh"))


def mk_mesh(verts, tris_lists):
    v = np.array(verts, dtype="<f4").reshape(-1, 3)
    return {"count": len(v), "verts": v.tobytes(), "tris": [np.array(t, dtype="<i4").tobytes() for t in tris_lists]}


CUBE_V = [(-1, -1, -1), (1, -1, -1), (1, 1, -1), (-1, 1, -1), (-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)]
CUBE_T = [0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4, 3, 7, 6, 3, 6, 2, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5]


def synthetic_model():
    """Coverage model: every collider chunk (legacy and v2 layouts, trigger on and off, three capsule axes, a wheel marker), tag /
    layer chunks, non-trivial rotations and non-uniform scales down a 4-deep chain, and every chunk the reader has to skip."""
    s2 = 0.70710678
    root = mk_node("SynthRoot", pos=(0.5, -0.25, 1.0), rot=(0, 0, 0, 1), scl=(1.0, 1.0, 1.0), tag="Untagged", layer=0)
    a = add(root, mk_node("Arm", pos=(1.0, 2.0, 3.0), rot=(0.0, s2, 0.0, s2), scl=(1.0, 2.0, 1.0)))
    b = add(a, mk_node("Hand", pos=(0.25, -0.5, 0.75), rot=(0.5, 0.5, 0.5, 0.5), scl=(1.5, 1.0, 0.5), tag="NoAttach", layer=15))
    c = add(b, mk_node("Fingertip", pos=(-1.0, 0.0, 0.5), rot=(0.0, 0.0, s2, s2), scl=(1.0, 1.0, 1.0)))
    d = add(root, mk_node("Side", pos=(-2.0, 0.5, 0.0), rot=(s2, 0.0, 0.0, s2), scl=(2.0, 2.0, 2.0)))
    root.colliders = [
        mk_collider(w17.BoxCollider, size=(1, 2, 3), center=(0.1, 0.2, 0.3)),
        mk_collider(w17.SphereCollider, radius=0.75, center=(0, 1, 0)),
        mk_collider(w17.CapsuleCollider, radius=0.3, direction=2, center=(0, 0, 0.5)),
    ]
    a.colliders = [
        mk_collider(w17.BoxCollider2, False, size=(0.5, 0.5, 4.0), center=(0, 0, 0)),
        mk_collider(w17.BoxCollider2, True, size=(9, 9, 9), center=(0, 0, 0)),  # trigger
        mk_collider(w17.WheelCollider),
    ]
    b.colliders = [
        mk_collider(w17.SphereCollider2, False, radius=0.5, center=(0.1, 0.2, 0.3)),
        mk_collider(w17.CapsuleCollider2, False, radius=0.25, height=3.0, direction=0, center=(0, 0.5, 0)),
        mk_collider(w17.CapsuleCollider2, False, radius=0.25, height=1.0, direction=1, center=(0, 0, 0)),  # height < 2 radius... 1 > 0.5 ok
        mk_collider(w17.CapsuleCollider2, False, radius=0.75, height=1.0, direction=2, center=(0, 0, 0)),  # height < 2 radius
    ]
    c.colliders = [
        mk_collider(w17.MeshCollider2, False, convexFlag=True, mesh=mk_mesh(CUBE_V, [CUBE_T])),
        mk_collider(w17.MeshCollider2, True, convexFlag=False, mesh=mk_mesh(CUBE_V, [CUBE_T])),  # trigger, flag false
    ]
    d.colliders = [
        mk_collider(w17.MeshCollider, convexFlag=False, mesh=mk_mesh(CUBE_V, [CUBE_T[:18], CUBE_T[18:]])),  # legacy, two submeshes
        mk_collider(w17.BoxCollider, size=(1, 1, 1), center=(0, 0, 0)),
    ]
    return root


def synth_extras(w, node):
    """Chunks PartReader consumes that carry no collider data (written on the nodes called Arm / Hand / Side)."""
    if node.name == "Arm":
        w.i32(w17.MeshFilter)
        write_mesh(w, mk_mesh(CUBE_V, [CUBE_T]), with_extras=True)
        w.i32(w17.MeshRenderer)
        w.bools(True); w.bools(False)       # version >= 1: shadows
        w.i32(1); w.i32(0)                  # one material index
        w.i32(w17.Materials)                # version 5: ReadMaterial4
        w.i32(1)
        w.s("mat0"); w.s("KSP/Bumped Specular")
        w.i32(3)
        w.s("_Color"); w.i32(w17.SPT_Color); [w.f(0.5) for _ in range(4)]
        w.s("_Shininess"); w.i32(w17.SPT_Range); w.f(0.4)
        w.s("_MainTex"); w.i32(w17.SPT_TexEnv); w.i32(0); [w.f(1.0) for _ in range(4)]
        w.i32(w17.Textures)
        w.i32(1); w.s("tex.dds"); w.i32(0)
    if node.name == "Hand":
        w.i32(w17.Light)
        w.i32(2); w.f(1.5); w.f(10.0); [w.f(1.0) for _ in range(4)]; w.i32(-1); w.f(30.0)
        w.i32(w17.Camera)
        w.i32(1); [w.f(0.1) for _ in range(4)]; w.i32(255); w.bools(False); [w.f(1.0) for _ in range(4)]
    if node.name == "Side":
        w.i32(w17.Animation)
        w.i32(1)
        w.s("clip0"); [w.f(0.0) for _ in range(6)]; w.i32(2)
        w.i32(1)
        w.s("Arm"); w.s("m_LocalPosition.x"); w.i32(0); w.i32(2); w.i32(2); w.i32(2)
        for t in (0.0, 1.0):
            w.f(t); w.f(t); w.f(0.0); w.f(0.0); w.i32(0)
        w.s("clip0"); w.bools(False)


SYNTH_CASES = [
    {"name": "no MODEL node, rescaleFactor 1.25", "rescale": 1.25, "models": []},
    {"name": "one MODEL, scale and position", "rescale": 0.9, "models": [{"scale": [2.0, 2.0, 2.0], "position": [0.5, 1.0, -1.5]}]},
    {"name": "two MODELs, parent path and name", "rescale": 1.0,
     "models": [{"name": "first", "scale": [1.0, 1.5, 1.0]},
                {"name": "second", "parent": "first/Arm", "position": [0.0, 0.25, 0.0], "scale": [0.5, 0.5, 0.5]}]},
]


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.abspath(os.path.join(here, "..", "..", ".."))
    ap = argparse.ArgumentParser()
    ap.add_argument("--staging", default=os.path.join(root_dir, "core", "data", "mu_staging"))
    ap.add_argument("--data-dir", default=os.path.join(root_dir, "core", "tests", "data"))
    args = ap.parse_args()
    out_dir = os.path.join(args.data_dir, "mucollider")
    os.makedirs(out_dir, exist_ok=True)

    # (a) the staged originals: parse, derive, and write a REDUCED copy (hierarchy + colliders + tag/layer only)
    manifest = json.load(open(os.path.join(args.staging, "MANIFEST.json"), encoding="utf-8"))
    parts = {}
    for part in sorted(manifest):
        files = {f["file"]: f for f in manifest[part]["files"]}
        cfg = [n for n in files if n.endswith(".cfg")][0]
        mu = [n for n in files if n.endswith(".mu")][0]
        for n, f in files.items():
            if w17.sha256(os.path.join(args.staging, part, n)) != f["sha256"]:
                raise SystemExit("sha256 mismatch: %s/%s" % (part, n))
        data = open(os.path.join(args.staging, part, mu), "rb").read()
        rescale, models = w17.parse_cfg_model_nodes(open(os.path.join(args.staging, part, cfg), encoding="utf-8").read())
        mf, root = read_mu(data)
        nodes = count_nodes(root)
        model = w17.build_part_chain(root, rescale, models)
        cols = []
        walk(model, [], cols)
        # the reduced file is written from a fresh parse (build_part_chain renamed / rescaled the root in place)
        mf2, root2 = read_mu(data)
        reduced = write_mu(mf2.version, root2)
        rname = part + ".mu"
        open(os.path.join(out_dir, rname), "wb").write(reduced)
        parts[part] = {"mu": {"file": mu, "origin": files[mu]["origin"], "sha256": files[mu]["sha256"], "bytes": len(data)},
                       "cfg": {"file": cfg, "origin": files[cfg]["origin"], "sha256": files[cfg]["sha256"]},
                       "reduced": {"file": "mucollider/" + rname, "bytes": len(reduced)},
                       "muVersion": mf.version, "nodes": nodes, "rescaleFactor": h(rescale), "modelNodes": len(models),
                       "model": models[0] if models else None, "colliders": cols}

    # (b) the synthetic coverage model
    root = synthetic_model()
    synth_bytes = write_mu(5, root, synth_extras, extras=True)
    open(os.path.join(out_dir, "synthetic.mu"), "wb").write(synth_bytes)
    cases = []
    for case in SYNTH_CASES:
        n_models = max(1, len(case["models"]))
        roots = [read_mu(synth_bytes)[1] for _ in range(n_models)]
        model = build_chain_general(roots, case["rescale"], case["models"])
        cols = []
        walk(model, [], cols)
        cases.append({"name": case["name"], "rescaleFactor": h(F(case["rescale"])), "models": case["models"], "colliders": cols})
    synth = {"file": "mucollider/synthetic.mu", "version": 5, "bytes": len(synth_bytes), "cases": cases}

    doc = {"about": "WP-mucol: collider chunks of the staged stock engine models and their part-space shapes (floats = binary32 bit "
                    "patterns, mesh data as FNV-1a 64 of the raw little-endian bytes). The reduced .mu files next to this file are the "
                    "originals with everything but the transform tree, TagAndLayer and collider chunks removed. Produced by "
                    "core/tools/mucollider/mu_colliders_reference.py.", "parts": parts, "synthetic": synth}
    out = os.path.join(args.data_dir, "mucollider_reference.json")
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        json.dump(doc, f, indent=1, sort_keys=True)
        f.write("\n")
    tot = 0
    for k, v in parts.items():
        tot += len(v["colliders"])
        print("%-26s v%d nodes %3d colliders %d  %s" % (k, v["muVersion"], v["nodes"], len(v["colliders"]),
              sorted(set(c["entry"] + ("(trigger)" if c["trigger"] else "") for c in v["colliders"]))))
    print("parts %d, colliders %d; synthetic: %d bytes, cases %d" % (len(parts), tot, len(synth_bytes), len(cases)))


if __name__ == "__main__":
    sys.exit(main())
