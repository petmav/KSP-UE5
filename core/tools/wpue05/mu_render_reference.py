#!/usr/bin/env python
"""WP-UE-05: independent reference for the .mu RENDER chunks and the texture headers.

Run:  python -I core/tools/wpue05/mu_render_reference.py [--data-dir core/tests/data] [--mirror core/testdata/GameData-mirror]

Output: core/tests/data/wpue05/*.mu          the synthetic coverage models (written by this file's own writer)
        core/tests/data/wpue05_reference.json  per model the full row list and the compact digest, truncation sweep, error cases,
                                               texture header records (synthetic headers + the GameData mirror's stubs)

INDEPENDENCE: this file shares NO code with core/src (the C++ reader) and none with the older WP-17b / WP-mucol python (its own parser).
It is written from the C# spec only:
  Assembly-CSharp/PartReader.cs  Read :140, ReadChild :205, ReadTextures :385, ReadMeshRenderer :441, ReadSkinnedMeshRenderer :468,
      ReadMesh :495, ReadMaterial :646 (version < 4), ReadMaterial4 :758, ReadMaterialTexture :790, ReadAnimation :802, ReadLight :950,
      ReadTagAndLayer :964, ReadCamera :970, ReadParticles :985, FindChildByName :1068, ReadColor :1085
  Assembly-CSharp/PartToolsLib/{EntryType,ShaderType,TextureType,AnimationType}.cs
  Assembly-CSharp/DatabaseLoaderTexture_{DDS,MBM,PNG,JPEG,TGA,TRUECOLOR}.cs, MBMReader.cs, TGAHeader.cs, TGAImage.cs, DDSHeaders/*,
      GameDatabase.cs:616 BitmapToUnityNormalMap

THE SYNTHETIC COVERAGE MODELS are labelled test fixtures for format coverage (every chunk layout and version branch of the reader), NOT
substitutes for any game asset: the real .mu files are swept locally (core/tools/wpue05/sweep_stock.py). Floats are handled as raw IEEE
bit patterns everywhere (never converted to a Python float, except to order animation key times), so the comparison is exact (0 ULP,
NaN payloads and signed zeros included).

ROWS: the shared description of a parsed model is a list of (key, value) string rows (see rows()): ints in decimal, floats as 8 hex
digits of their bits, strings with bytes outside 0x20..0x7e and the backslash as \\xHH. `full` mode lists every array element; the
compact mode (used for the stock sweep digest) lists arrays as `<count>:<FNV-1a 64 of the raw little-endian bytes>`. The digest of a
model is FNV-1a 64 over "key=value\\n" of the compact rows. The C++ side produces the same rows through the C ABI (tests/mu_dump.hpp).
"""
import argparse
import hashlib
import json
import os
import random
import struct
import sys

# ---------------------------------------------------------------------------------------------------------------------------------
# PartToolsLib enums (EntryType.cs)
ENTRY = ["ChildTransformStart", "ChildTransformEnd", "Animation", "MeshCollider", "SphereCollider", "CapsuleCollider", "BoxCollider",
         "MeshFilter", "MeshRenderer", "SkinnedMeshRenderer", "Materials", "Material", "Textures", "MeshStart", "MeshVerts", "MeshUV",
         "MeshUV2", "MeshNormals", "MeshTangents", "MeshTriangles", "MeshBoneWeights", "MeshBindPoses", "MeshEnd", "Light",
         "TagAndLayer", "MeshCollider2", "SphereCollider2", "CapsuleCollider2", "BoxCollider2", "WheelCollider", "Camera",
         "ParticleEmitter", "MeshVertexColors"]
E = {n: i for i, n in enumerate(ENTRY)}
FILE_TYPE_MODEL_BINARY = 76543

# ShaderType.cs value -> (Shader.Find name, [(kind, property)]) ; kinds: T texture, C colour, F float (PartReader.cs:646-749, in read order)
LEGACY = {
    1: ("KSP/Diffuse", [("T", "_MainTex")]),
    2: ("KSP/Specular", [("T", "_MainTex"), ("C", "_SpecColor"), ("F", "_Shininess")]),
    3: ("KSP/Bumped", [("T", "_MainTex"), ("T", "_BumpMap")]),
    4: ("KSP/Bumped Specular", [("T", "_MainTex"), ("T", "_BumpMap"), ("C", "_SpecColor"), ("F", "_Shininess")]),
    5: ("KSP/Emissive/Diffuse", [("T", "_MainTex"), ("T", "_Emissive"), ("C", "_EmissiveColor")]),
    6: ("KSP/Emissive/Specular", [("T", "_MainTex"), ("C", "_SpecColor"), ("F", "_Shininess"), ("T", "_Emissive"),
                                  ("C", "_EmissiveColor")]),
    7: ("KSP/Emissive/Bumped Specular", [("T", "_MainTex"), ("T", "_BumpMap"), ("C", "_SpecColor"), ("F", "_Shininess"),
                                         ("T", "_Emissive"), ("C", "_EmissiveColor")]),
    8: ("KSP/Alpha/Cutoff", [("T", "_MainTex"), ("F", "_Cutoff")]),
    9: ("KSP/Alpha/Cutoff Bumped", [("T", "_MainTex"), ("T", "_BumpMap"), ("F", "_Cutoff")]),
    10: ("KSP/Alpha/Translucent", [("T", "_MainTex")]),
    11: ("KSP/Alpha/Translucent Specular", [("T", "_MainTex"), ("F", "_Gloss"), ("C", "_SpecColor"), ("F", "_Shininess")]),
    12: ("KSP/Alpha/Unlit Transparent", [("T", "_MainTex"), ("C", "_Color")]),
    13: ("KSP/Unlit", [("T", "_MainTex"), ("C", "_Color")]),
    14: ("KSP/Particles/Alpha Blended", [("T", "_MainTex"), ("C", "_Color"), ("F", "_InvFade")]),
    15: ("KSP/Particles/Additive", [("T", "_MainTex"), ("C", "_Color"), ("F", "_InvFade")]),
    16: ("KSP/Bumped Specular (Mapped)", [("T", "_MainTex"), ("T", "_BumpMap"), ("T", "_SpecMap"), ("F", "_SpecTint"),
                                          ("F", "_Shininess")]),
}
LEGACY_DEFAULT = ("KSP/Diffuse", [("T", "_MainTex")])  # `default:` also covers ShaderType.Custom (0) and any unknown value


class ParseError(ValueError):
    pass


def fnv1a64(data):
    h = 0xCBF29CE484222325
    for b in data:
        h = ((h ^ b) * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return h


def esc(s):
    """s is a latin-1 decoded str (one char per byte)."""
    out = []
    for ch in s:
        o = ord(ch)
        if 0x20 <= o <= 0x7E and ch != "\\":
            out.append(ch)
        else:
            out.append("\\x%02x" % o)
    return "".join(out)


# ---------------------------------------------------------------------------------------------------------------------------------
# the parser (PartReader.cs)
class Rd:
    def __init__(self, data):
        self.d = data
        self.p = 0

    def eof(self):
        return self.p == len(self.d)

    def take(self, n):
        if n < 0 or self.p + n > len(self.d):
            raise ParseError("EndOfStream at %d" % self.p)
        b = self.d[self.p:self.p + n]
        self.p += n
        return b

    def i32(self):
        return struct.unpack("<i", self.take(4))[0]

    def f(self):  # a float as its raw bits
        return struct.unpack("<I", self.take(4))[0]

    def fs(self, n):
        return [self.f() for _ in range(n)]

    def boolean(self):
        return self.take(1)[0] != 0

    def string(self):  # BinaryReader.ReadString: 7-bit encoded length
        n = 0
        shift = 0
        while True:
            b = self.take(1)[0]
            n |= (b & 0x7F) << shift
            if not b & 0x80:
                break
            shift += 7
            if shift > 35:
                raise ParseError("bad 7-bit length")
        return self.take(n).decode("latin-1")


def file_name_without_extension(path):  # Path.GetFileNameWithoutExtension
    i = max(path.rfind("/"), path.rfind("\\"))
    name = path[i + 1:]
    j = name.rfind(".")
    return name if j < 0 else name[:j]


class Mesh:
    pass


def read_mesh(rd, acc):  # :495 (acc: the model-wide count of entry values the mesh switch has no case for)
    if rd.i32() != E["MeshStart"]:
        raise ParseError("Mesh Error")
    m = Mesh()
    m.n = rd.i32()
    m.header_int = rd.i32()
    if m.n < 0:
        raise ParseError("negative vertex count (new Vector3[num] overflows)")
    m.verts = m.normals = m.tangents = m.uv = m.uv2 = m.colors = None  # raw little-endian bytes of the last chunk of that kind
    m.bones = None  # (index bytes, weight bytes)
    m.bind = None  # raw bytes, count
    m.bind_count = 0
    m.tris = []  # raw bytes per list, count
    n = m.n
    while True:
        et = rd.i32()
        if et == E["MeshEnd"]:
            break
        if et == E["MeshVertexColors"]:
            m.colors = rd.take(4 * n)
        elif et == E["MeshVerts"]:
            m.verts = rd.take(12 * n)
        elif et == E["MeshUV"]:
            m.uv = rd.take(8 * n)
        elif et == E["MeshUV2"]:
            m.uv2 = rd.take(8 * n)
        elif et == E["MeshNormals"]:
            m.normals = rd.take(12 * n)
        elif et == E["MeshTangents"]:
            m.tangents = rd.take(16 * n)
        elif et == E["MeshTriangles"]:
            k = rd.i32()
            if k < 0:
                raise ParseError("negative triangle count")
            m.tris.append((rd.take(4 * k), k))
        elif et == E["MeshBoneWeights"]:
            raw = rd.take(32 * n)
            idx = b"".join(raw[32 * v + 8 * j:32 * v + 8 * j + 4] for v in range(n) for j in range(4))
            wgt = b"".join(raw[32 * v + 8 * j + 4:32 * v + 8 * j + 8] for v in range(n) for j in range(4))
            m.bones = (idx, wgt)
        elif et == E["MeshBindPoses"]:
            k = rd.i32()
            if k < 0:
                raise ParseError("negative bind pose count")
            m.bind = rd.take(64 * k)
            m.bind_count = k
        else:
            acc[et] = acc.get(et, 0) + 1  # the switch has no case
    return m


class Node:
    def __init__(self, name):
        self.name = name
        self.id = 0
        self.children = []
        self.pos = self.rot = self.scale = None
        self.has_tag = False
        self.tag = "Untagged"
        self.layer = 0
        self.colliders = 0
        self.filters = []
        self.mrs = []
        self.srs = []
        self.lights = []
        self.cams = []
        self.parts = []
        self.anims = []


class Model:
    pass


class Parser:
    def __init__(self, rd, version):
        self.rd = rd
        self.version = version
        self.slots = 0  # matDummies.Count
        self.tex_slots = 0  # textureDummies.Count
        self.materials = []
        self.materials_chunks = 0
        self.textures = []
        self.textures_chunks = 0
        self.ignored = {}
        self.ignored_mesh = {}
        self.warnings = []

    def note_slot(self, i):  # while (i >= matDummies.Count) Add; matDummies[i]: negative throws
        if i < 0:
            raise ParseError("negative material id")
        self.slots = max(self.slots, i + 1)

    def tex_prop(self, name):  # ReadMaterialTexture :790
        rd = self.rd
        tid = rd.i32()
        if tid < -1:
            self.warnings.append("texture id %d (PartReader.cs:113 would throw)" % tid)
        elif tid + 1 > self.tex_slots:
            self.tex_slots = tid + 1
        return {"name": name, "kind": 4, "value": [0, 0, 0, 0], "tid": tid, "scale": rd.fs(2), "offset": rd.fs(2)}

    def read_material_legacy(self):  # :646
        rd = self.rd
        name = rd.string()
        st = rd.i32()
        shader, props = LEGACY.get(st, LEGACY_DEFAULT)
        out = []
        for kind, pname in props:
            if kind == "T":
                out.append(self.tex_prop(pname))
            elif kind == "C":
                out.append({"name": pname, "kind": 0, "value": rd.fs(4), "tid": -1, "scale": [0, 0], "offset": [0, 0]})
            else:
                out.append({"name": pname, "kind": 2, "value": rd.fs(1) + [0, 0, 0], "tid": -1, "scale": [0, 0], "offset": [0, 0]})
        return {"name": name, "shader": shader, "legacy": st, "props": out}

    def read_material4(self):  # :758
        rd = self.rd
        name = rd.string()
        shader = rd.string()
        n = rd.i32()
        out = []
        for _ in range(n):
            pname = rd.string()
            kind = rd.i32()
            if kind in (0, 1):  # Color: ReadColor; Vector: ReadVector4
                out.append({"name": pname, "kind": kind, "value": rd.fs(4), "tid": -1, "scale": [0, 0], "offset": [0, 0]})
            elif kind in (2, 3):  # Float, Range
                out.append({"name": pname, "kind": kind, "value": rd.fs(1) + [0, 0, 0], "tid": -1, "scale": [0, 0], "offset": [0, 0]})
            elif kind == 4:
                out.append(self.tex_prop(pname))
            else:  # the switch has no case
                self.warnings.append("material '%s' property '%s' has unknown ShaderPropertyType %d (no value read)" % (name, pname, kind))
        return {"name": name, "shader": shader, "legacy": -1, "props": out}

    def read_animation(self):  # :802
        rd = self.rd
        a = {"clips": []}
        n = rd.i32()
        flag = False
        for _ in range(n):
            clip = {"name": rd.string(), "bounds": rd.fs(6), "wrap": rd.i32(), "curves": [], "dropped": False}
            for _ in range(rd.i32()):
                c = {"path": rd.string(), "prop": rd.string(), "type": rd.i32(), "pre": rd.i32(), "post": rd.i32()}
                nk = rd.i32()
                if nk < 0:
                    raise ParseError("negative key count")
                keys = []
                for _ in range(nk):
                    keys.append((rd.f(), rd.f(), rd.f(), rd.f(), rd.i32()))
                c["keys"] = keys
                if c["type"] not in (0, 1, 2, 3):  # switch leaves `type` null for ParticleEmitter (4) / anything else
                    flag = True
                clip["curves"].append(c)
            clip["dropped"] = flag
            a["clips"].append(clip)
        a["default"] = rd.string()
        a["default_applied"] = a["default"] != "" and not flag
        a["play"] = rd.boolean()
        return a

    def read_child(self):  # :205
        rd = self.rd
        node = Node(rd.string())
        node.pos = rd.fs(3)
        node.rot = rd.fs(4)
        node.scale = rd.fs(3)
        while not rd.eof():
            et = rd.i32()
            if et == E["ChildTransformStart"]:
                node.children.append(self.read_child())
            elif et == E["Animation"]:
                node.anims.append(self.read_animation())
            elif et == E["MeshCollider"]:
                rd.boolean()
                read_mesh(rd, self.ignored_mesh)
                node.colliders += 1
            elif et == E["SphereCollider"]:
                rd.fs(1 + 3)
                node.colliders += 1
            elif et == E["CapsuleCollider"]:
                rd.fs(1)
                rd.i32()
                rd.fs(3)
                node.colliders += 1
            elif et == E["BoxCollider"]:
                rd.fs(6)
                node.colliders += 1
            elif et == E["MeshFilter"]:
                node.filters.append(read_mesh(rd, self.ignored_mesh))
            elif et == E["MeshRenderer"]:
                mr = {"shadow": None, "slots": []}
                if self.version >= 1:
                    mr["shadow"] = (rd.boolean(), rd.boolean())
                for _ in range(rd.i32()):
                    i = rd.i32()
                    self.note_slot(i)
                    mr["slots"].append(i)
                node.mrs.append(mr)
            elif et == E["SkinnedMeshRenderer"]:
                sr = {"slots": []}
                for _ in range(rd.i32()):
                    i = rd.i32()
                    self.note_slot(i)
                    sr["slots"].append(i)
                sr["bounds"] = rd.fs(6)
                sr["quality"] = rd.i32()
                sr["offscreen"] = rd.boolean()
                sr["bones"] = [rd.string() for _ in range(rd.i32())]
                sr["mesh"] = read_mesh(rd, self.ignored_mesh)
                node.srs.append(sr)
            elif et == E["Materials"]:
                n = rd.i32()
                self.materials_chunks += 1
                if n > self.slots:
                    self.warnings.append("Materials chunk has %d entries but only %d material slots are referenced (PartReader.cs:277 would throw)"
                                         % (n, self.slots))
                for i in range(n):
                    mat = self.read_material_legacy() if self.version < 4 else self.read_material4()
                    while len(self.materials) <= i:
                        self.materials.append(None)
                    self.materials[i] = mat
            elif et == E["Textures"]:
                n = rd.i32()
                if n != self.tex_slots:
                    raise ParseError("TextureError: %d %d" % (n, self.tex_slots))
                self.textures_chunks += 1
                self.textures = []
                for _ in range(n):
                    fname = rd.string()
                    self.textures.append({"file": fname, "name": file_name_without_extension(fname), "type": rd.i32()})
            elif et == E["Light"]:
                L = {"type": rd.i32(), "intensity": rd.f(), "range": rd.f(), "color": rd.fs(4), "mask": rd.i32(), "has_spot": False,
                     "spot": 0x41F00000}  # 30.0f
                if self.version > 1:
                    L["has_spot"] = True
                    L["spot"] = rd.f()
                node.lights.append(L)
            elif et == E["TagAndLayer"]:
                node.tag = rd.string()
                node.layer = rd.i32()
                node.has_tag = True
            elif et == E["MeshCollider2"]:
                rd.boolean()
                rd.boolean()
                read_mesh(rd, self.ignored_mesh)
                node.colliders += 1
            elif et == E["SphereCollider2"]:
                rd.boolean()
                rd.fs(4)
                node.colliders += 1
            elif et == E["CapsuleCollider2"]:
                rd.boolean()
                rd.fs(2)
                rd.i32()
                rd.fs(3)
                node.colliders += 1
            elif et == E["BoxCollider2"]:
                rd.boolean()
                rd.fs(6)
                node.colliders += 1
            elif et == E["WheelCollider"]:
                rd.fs(3 + 3 + 3 + 5 + 5)
                node.colliders += 1
            elif et == E["Camera"]:
                node.cams.append({"clear": rd.i32(), "bg": rd.fs(4), "mask": rd.i32(), "ortho": rd.boolean(), "fov": rd.f(),
                                  "near": rd.f(), "far": rd.f(), "depth": rd.f()})
            elif et == E["ParticleEmitter"]:
                rd.boolean()
                rd.i32()
                rd.fs(3 + 2 + 1)
                rd.fs(4)
                rd.boolean()
                rd.fs(4)
                rd.i32()
                rd.i32()
                rd.fs(9)
                rd.fs(3)
                rd.boolean()
                rd.boolean()
                rd.fs(20)
                rd.fs(6)
                rd.fs(1 + 3 + 3 + 1)
                rd.boolean()
                rd.boolean()
                rd.fs(3)
                rd.i32()  # render mode switch
                rd.i32()
                rd.i32()
                rd.i32()  # uv animation x, y, cycles
                slot = rd.i32()
                self.note_slot(slot)
                node.parts.append(slot)
            elif et == E["ChildTransformEnd"]:
                return node
            else:
                self.ignored[et] = self.ignored.get(et, 0) + 1
        return node


def parse_mu(data):
    rd = Rd(data)
    ftype = rd.i32()
    version = rd.i32()
    header = rd.string()
    if ftype != FILE_TYPE_MODEL_BINARY:
        raise ParseError("incorrect file type")
    p = Parser(rd, version)
    root = p.read_child()
    m = Model()
    m.file_type, m.version, m.header, m.root = ftype, version, header, root
    m.materials, m.materials_chunks = p.materials, p.materials_chunks
    m.textures, m.textures_chunks = p.textures, p.textures_chunks
    m.ignored, m.ignored_mesh, m.warnings = p.ignored, p.ignored_mesh, p.warnings
    # preorder ids, then the bones (FindChildByName: first match of a depth-first walk from the root)
    order = []

    def walk(n, parent):
        n.id = len(order)
        n.parent = parent
        order.append(n)
        for c in n.children:
            walk(c, n.id)
    walk(root, -1)
    m.order = order
    for n in order:
        for sr in n.srs:
            sr["bone_nodes"] = [next((c.id for c in order if c.name == b), -1) for b in sr["bones"]]
    return m


# ---------------------------------------------------------------------------------------------------------------------------------
# rows
def H(bits):
    return "%08x" % bits


def rows(m, parent_url, full):
    out = []

    def R(k, v):
        out.append((k, str(v)))

    def arr(key, raw, count_elems, elem):
        """elem: 'f' float bits as hex, 'i' signed int, 'b' unsigned byte"""
        if raw is None:
            return
        if full:
            if elem == "f":
                for j, (u,) in enumerate(struct.iter_unpack("<I", raw)):
                    R("%s.%d" % (key, j), H(u))
            elif elem == "i":
                for j, (u,) in enumerate(struct.iter_unpack("<i", raw)):
                    R("%s.%d" % (key, j), u)
            else:
                for j, u in enumerate(raw):
                    R("%s.%d" % (key, j), u)
        else:
            R(key, "%d:%016x" % (count_elems, fnv1a64(raw)))

    order = m.order
    meshes, renderers, lights, cams, anims, clips, parts = [], [], [], [], [], [], []
    ncoll = 0
    for n in order:
        ncoll += n.colliders
        first = len(meshes)
        for mesh in n.filters:
            meshes.append((n.id, 0, mesh))
        for mr in n.mrs:
            renderers.append((n.id, 0, first if n.filters else -1, mr))
        for sr in n.srs:
            meshes.append((n.id, 1, sr["mesh"]))
            renderers.append((n.id, 1, len(meshes) - 1, sr))
        for L in n.lights:
            lights.append((n.id, L))
        for c in n.cams:
            cams.append((n.id, c))
        parts.extend(n.parts)
        for a in n.anims:
            ai = len(anims)
            anims.append((n.id, len(clips), a))
            for c in a["clips"]:
                clips.append((ai, c))
    ign = sorted(m.ignored.items())
    ign_mesh = sorted(m.ignored_mesh.items())  # accumulated over ALL meshes, collider meshes included
    R("model.file_type", m.file_type)
    R("model.version", m.version)
    R("model.header", esc(m.header))
    R("model.node_count", len(order))
    R("model.mesh_count", len(meshes))
    R("model.renderer_count", len(renderers))
    R("model.material_count", len(m.materials))
    R("model.texture_count", len(m.textures))
    R("model.light_count", len(lights))
    R("model.camera_count", len(cams))
    R("model.particle_emitter_count", len(parts))
    R("model.animation_count", len(anims))
    R("model.clip_count", len(clips))
    R("model.collider_count", ncoll)
    R("model.materials_chunk_count", m.materials_chunks)
    R("model.textures_chunk_count", m.textures_chunks)
    R("model.warning_count", len(m.warnings))
    R("model.ignored_entry_count", len(ign))
    R("model.ignored_mesh_entry_count", len(ign_mesh))
    for i, w in enumerate(m.warnings):
        R("warning.%d" % i, esc(w))
    for i, (v, c) in enumerate(ign):
        R("ignored.%d" % i, "%d:%d" % (v, c))
    for i, (v, c) in enumerate(ign_mesh):
        R("ignoredmesh.%d" % i, "%d:%d" % (v, c))
    for n in order:
        p = "node.%d." % n.id
        R(p + "name", esc(n.name))
        R(p + "parent", n.parent)
        R(p + "child_count", len(n.children))
        for j, b in enumerate(n.pos):
            R(p + "pos.%d" % j, H(b))
        for j, b in enumerate(n.rot):
            R(p + "rot.%d" % j, H(b))
        for j, b in enumerate(n.scale):
            R(p + "scale.%d" % j, H(b))
        R(p + "has_tag", int(n.has_tag))
        R(p + "tag", esc(n.tag))
        R(p + "layer", n.layer)
        R(p + "mesh_filter_count", len(n.filters))
        R(p + "mesh_renderer_count", len(n.mrs))
        R(p + "skinned_renderer_count", len(n.srs))
        R(p + "collider_count", n.colliders)
        R(p + "animation_count", len(n.anims))
        R(p + "light_count", len(n.lights))
        R(p + "camera_count", len(n.cams))
        R(p + "particle_emitter_count", len(n.parts))
        for k, c in enumerate(n.children):
            R(p + "child.%d" % k, c.id)
    for i, (nid, src, mesh) in enumerate(meshes):
        p = "mesh.%d." % i
        R(p + "node", nid)
        R(p + "source", src)
        R(p + "vertex_count", mesh.n)
        R(p + "header_int", mesh.header_int)
        flags = (1 if mesh.verts is not None else 0) | (2 if mesh.normals is not None else 0) | (4 if mesh.tangents is not None else 0) | \
                (8 if mesh.uv is not None else 0) | (16 if mesh.uv2 is not None else 0) | (32 if mesh.colors is not None else 0) | \
                (64 if mesh.bones is not None else 0) | (128 if mesh.bind is not None else 0)
        R(p + "flags", flags)
        arr(p + "vertices", mesh.verts, mesh.n * 3, "f")
        arr(p + "normals", mesh.normals, mesh.n * 3, "f")
        arr(p + "tangents", mesh.tangents, mesh.n * 4, "f")
        arr(p + "uv", mesh.uv, mesh.n * 2, "f")
        arr(p + "uv2", mesh.uv2, mesh.n * 2, "f")
        arr(p + "colors32", mesh.colors, mesh.n * 4, "b")
        R(p + "submesh_count", len(mesh.tris))
        for s, (raw, k) in enumerate(mesh.tris):
            arr(p + "submesh.%d" % s, raw, k, "i")
        if mesh.bones is not None:
            arr(p + "bone_indices", mesh.bones[0], mesh.n * 4, "i")
            arr(p + "bone_weights", mesh.bones[1], mesh.n * 4, "f")
        R(p + "bind_pose_count", mesh.bind_count)
        arr(p + "bind_poses", mesh.bind, mesh.bind_count * 16, "f")
    for i, (nid, kind, mesh_idx, r) in enumerate(renderers):
        p = "renderer.%d." % i
        R(p + "node", nid)
        R(p + "kind", kind)
        R(p + "mesh", mesh_idx)
        if kind == 0:
            sh = r["shadow"]
            R(p + "has_shadow_fields", int(sh is not None))
            R(p + "cast_shadows", int(sh[0]) if sh is not None else 1)
            R(p + "receive_shadows", int(sh[1]) if sh is not None else 1)
        else:
            R(p + "has_shadow_fields", 0)
            R(p + "cast_shadows", 1)
            R(p + "receive_shadows", 1)
        R(p + "material_slots", ",".join(str(s) for s in r["slots"]))
        if kind == 1:
            for j, b in enumerate(r["bounds"]):
                R(p + "bounds.%d" % j, H(b))
            R(p + "quality", r["quality"])
            R(p + "update_when_offscreen", int(r["offscreen"]))
            R(p + "bone_count", len(r["bones"]))
            R(p + "bone_nodes", ",".join(str(s) for s in r["bone_nodes"]))
            for j, b in enumerate(r["bones"]):
                R(p + "bone.%d" % j, esc(b))
    for i, mat in enumerate(m.materials):
        p = "material.%d." % i
        R(p + "name", esc(mat["name"]))
        R(p + "shader", esc(mat["shader"]))
        R(p + "legacy_shader_type", mat["legacy"])
        R(p + "property_count", len(mat["props"]))
        for j, pr in enumerate(mat["props"]):
            q = p + "prop.%d." % j
            R(q + "name", esc(pr["name"]))
            R(q + "kind", pr["kind"])
            for k, b in enumerate(pr["value"]):
                R(q + "value.%d" % k, H(b))
            R(q + "texture_id", pr["tid"])
            for k, b in enumerate(pr["scale"]):
                R(q + "scale.%d" % k, H(b))
            for k, b in enumerate(pr["offset"]):
                R(q + "offset.%d" % k, H(b))
    for i, t in enumerate(m.textures):
        p = "texture.%d." % i
        R(p + "file", esc(t["file"]))
        R(p + "name", esc(t["name"]))
        R(p + "url", esc((parent_url + "/" + t["name"]) if parent_url else t["name"]))
        R(p + "type", t["type"])
    for i, (nid, L) in enumerate(lights):
        p = "light.%d." % i
        R(p + "node", nid)
        R(p + "type", L["type"])
        R(p + "intensity", H(L["intensity"]))
        R(p + "range", H(L["range"]))
        for k, b in enumerate(L["color"]):
            R(p + "color.%d" % k, H(b))
        R(p + "culling_mask", L["mask"])
        R(p + "has_spot_angle", int(L["has_spot"]))
        R(p + "spot_angle", H(L["spot"]))
    for i, (nid, c) in enumerate(cams):
        p = "camera.%d." % i
        R(p + "node", nid)
        R(p + "clear_flags", c["clear"])
        for k, b in enumerate(c["bg"]):
            R(p + "background.%d" % k, H(b))
        R(p + "culling_mask", c["mask"])
        R(p + "orthographic", int(c["ortho"]))
        R(p + "fov", H(c["fov"]))
        R(p + "near", H(c["near"]))
        R(p + "far", H(c["far"]))
        R(p + "depth", H(c["depth"]))
    for i, s in enumerate(parts):
        R("particle.%d.material" % i, s)
    for i, (nid, first, a) in enumerate(anims):
        p = "animation.%d." % i
        R(p + "node", nid)
        R(p + "first_clip", first)
        R(p + "clip_count", len(a["clips"]))
        R(p + "default_clip", esc(a["default"]))
        R(p + "default_clip_applied", int(a["default_applied"]))
        R(p + "play_automatically", int(a["play"]))
    for i, (ai, c) in enumerate(clips):
        p = "clip.%d." % i
        R(p + "animation", ai)
        R(p + "name", esc(c["name"]))
        for k, b in enumerate(c["bounds"]):
            R(p + "bounds.%d" % k, H(b))
        R(p + "wrap_mode", c["wrap"])
        R(p + "dropped_by_ksp", int(c["dropped"]))
        R(p + "curve_count", len(c["curves"]))
        length = 0  # AnimationClip.length: the latest key time (float compare), bits of the winner; 0.0f start
        for cv in c["curves"]:
            for key in cv["keys"]:
                if struct.unpack("<f", struct.pack("<I", key[0]))[0] > struct.unpack("<f", struct.pack("<I", length))[0]:
                    length = key[0]
        R(p + "length", H(length))
        for j, cv in enumerate(c["curves"]):
            q = p + "curve.%d." % j
            R(q + "path", esc(cv["path"]))
            R(q + "property", esc(cv["prop"]))
            R(q + "type", cv["type"])
            R(q + "pre_wrap_mode", cv["pre"])
            R(q + "post_wrap_mode", cv["post"])
            R(q + "key_count", len(cv["keys"]))
            if full:
                for k, (t, v, it, ot, un) in enumerate(cv["keys"]):
                    R(q + "key.%d" % k, "%s,%s,%s,%s,%d" % (H(t), H(v), H(it), H(ot), un))
            else:
                raw = b"".join(struct.pack("<IIIIi", *key) for key in cv["keys"])
                R(q + "keys", "%d:%016x" % (len(cv["keys"]), fnv1a64(raw)))
    return out


def digest(rows_):
    return fnv1a64("".join("%s=%s\n" % kv for kv in rows_).encode("latin-1"))


def stats(m):
    """Chunk counts of a parsed model (the stock sweep records them per part)."""
    c = {"nodes": len(m.order), "colliders": 0, "mesh_filters": 0, "mesh_renderers": 0, "skinned_renderers": 0, "lights": 0, "cameras": 0,
         "particle_emitters": 0, "animations": 0, "clips": 0, "curves": 0, "keys": 0, "tag_and_layer": 0, "vertices": 0, "triangles": 0,
         "submeshes": 0, "bind_poses": 0}
    for n in m.order:
        c["colliders"] += n.colliders
        c["mesh_filters"] += len(n.filters)
        c["mesh_renderers"] += len(n.mrs)
        c["skinned_renderers"] += len(n.srs)
        c["lights"] += len(n.lights)
        c["cameras"] += len(n.cams)
        c["particle_emitters"] += len(n.parts)
        c["animations"] += len(n.anims)
        c["tag_and_layer"] += int(n.has_tag)
        for mesh in [f for f in n.filters] + [s["mesh"] for s in n.srs]:
            c["vertices"] += mesh.n
            c["submeshes"] += len(mesh.tris)
            c["triangles"] += sum(k for _, k in mesh.tris)  # indices for now; divided by 3 below
            c["bind_poses"] += mesh.bind_count
        for a in n.anims:
            c["clips"] += len(a["clips"])
            for clip in a["clips"]:
                c["curves"] += len(clip["curves"])
                c["keys"] += sum(len(cv["keys"]) for cv in clip["curves"])
    c["triangles"] //= 3
    c["materials"] = len(m.materials)
    c["textures"] = len(m.textures)
    return c


# ---------------------------------------------------------------------------------------------------------------------------------
# texture headers (DatabaseLoaderTexture_*.cs)
TF = {"RGB24": 3, "RGBA32": 4, "ARGB32": 5, "DXT1": 10, "DXT5": 12}


def fourcc(s):
    return struct.unpack("<I", s.encode("ascii"))[0]


def full_chain(w, h):
    m, n = max(w, h), 1
    while m > 1:
        m //= 2
        n += 1
    return n


def ext_of(name):
    base = name.replace("\\", "/").split("/")[-1]
    return base.rsplit(".", 1)[1].lower() if "." in base else ""


def stem_of(name):
    base = name.replace("\\", "/").split("/")[-1]
    return base.rsplit(".", 1)[0] if "." in base else base


def blank_header():
    return {"container": 0, "valid": 0, "loadable": 0, "width": 0, "height": 0, "source_format": 0, "unity_format": 0, "compressed": 0,
            "mip_chain": -1, "mip_count": 0, "is_normal_map": 0, "is_readable": 0, "is_compressed_flag": 0, "nrm_suffix": 0, "has_alpha": 0,
            "dds_header_flags": 0, "dds_pixel_format_flags": 0, "dds_fourcc": 0, "dds_caps": 0, "dds_mip_map_count": 0,
            "dds_rgb_bit_count": 0, "dds_dxgi_format": 0, "mbm_texture_type": -1, "mbm_bits_per_pixel": -1, "png_color_type": -1,
            "png_bit_depth": -1, "tga_image_type": -1, "tga_pixel_depth": -1, "data_offset": 0, "data_bytes": 0, "expected_data_bytes": 0,
            "error": ""}


def u32(b, o):
    return struct.unpack_from("<I", b, o)[0]


def sniff_png(b):
    if len(b) < 33 or b[:8] != b"\x89PNG\r\n\x1a\n" or b[12:16] != b"IHDR":
        return None
    w, h = struct.unpack(">II", b[16:24])
    depth, ctype = b[24], b[25]
    alpha = ctype in (4, 6)
    off = 8
    while off + 8 <= len(b):
        ln = struct.unpack(">I", b[off:off + 4])[0]
        typ = b[off + 4:off + 8]
        if typ == b"IDAT":
            break
        if typ == b"tRNS":
            alpha = True
        if ln + 12 > len(b) - off:
            break
        off += ln + 12
    return {"w": w, "h": h, "depth": depth, "ctype": ctype, "alpha": alpha}


def sniff_jpeg(b):
    if len(b) < 4 or b[0] != 0xFF or b[1] != 0xD8:
        return None
    off = 2
    while off + 4 <= len(b):
        if b[off] != 0xFF:
            off += 1
            continue
        mk = b[off + 1]
        if mk == 0xFF:
            off += 1
            continue
        if mk in (0xD8, 0x01) or 0xD0 <= mk <= 0xD7:
            off += 2
            continue
        if 0xC0 <= mk <= 0xCF and mk not in (0xC4, 0xC8, 0xCC):
            if off + 9 > len(b):
                return None
            return {"w": (b[off + 7] << 8) | b[off + 8], "h": (b[off + 5] << 8) | b[off + 6]}
        off += 2 + ((b[off + 2] << 8) | b[off + 3])
    return None


def ref_header(name, b, file_size=0):
    """The expected header record of file `name` whose first bytes are `b` (file_size 0 = len(b))."""
    h = _ref_header(name, b, file_size)
    if h["valid"]:
        h["data_bytes"] = max(0, (file_size or len(b)) - h["data_offset"])
    return h


def _ref_header(name, b, file_size):
    h = blank_header()
    ext = ext_of(name)
    nrm = stem_of(name).endswith("NRM")

    def nrm_result(compressed_flag=True):
        h.update(nrm_suffix=1, source_format=4, unity_format=4, compressed=0, mip_chain=1, is_normal_map=1, is_readable=0,
                 is_compressed_flag=int(compressed_flag), mip_count=full_chain(h["width"], h["height"]))

    if ext == "dds":
        h["container"] = 1
        if len(b) < 4 or u32(b, 0) != fourcc("DDS "):
            h["error"] = "DDS: File is not a DDS format file!"
            return h
        if len(b) < 128:
            h["error"] = "DDS: truncated header"
            return h
        h.update(dds_header_flags=u32(b, 8), height=u32(b, 12), width=u32(b, 16), dds_mip_map_count=u32(b, 28),
                 dds_pixel_format_flags=u32(b, 80), dds_fourcc=u32(b, 84), dds_rgb_bit_count=u32(b, 88), dds_caps=u32(b, 108),
                 data_offset=128)
        for k in ("height", "width"):
            if h[k] >= 2 ** 31:
                h[k] -= 2 ** 32
        if h["dds_fourcc"] == fourcc("DX10"):
            if len(b) < 148:
                h["error"] = "DDS: truncated DX10 header"
                return h
            h.update(dds_dxgi_format=u32(b, 128), data_offset=148)
        h["valid"] = 1
        h["mip_chain"] = 1 if h["dds_caps"] & 0x400000 else 0
        h["is_normal_map"] = int(bool(h["dds_pixel_format_flags"] & 0x80000) or bool(h["dds_pixel_format_flags"] & 0x80000000))
        h["is_readable"] = 0
        h["is_compressed_flag"] = 1
        h["compressed"] = 1
        fc = h["dds_fourcc"]
        if fc == fourcc("DXT1"):
            h["source_format"] = h["unity_format"] = TF["DXT1"]
            h["loadable"] = 1
        elif fc == fourcc("DXT5"):
            h["source_format"] = h["unity_format"] = TF["DXT5"]
            h["loadable"] = 1
        elif fc == fourcc("DXT3"):
            h["error"] = "DDS: DXT3 format is NOT supported. Use DXT5"
        elif fc == fourcc("DXT2"):
            h["error"] = "DDS: DXT2 is not supported!"
        elif fc == fourcc("DXT4"):
            h["error"] = "DDS: DXT4 is not supported!"
        elif fc == fourcc("DX10"):
            h["error"] = "DDS: DX10 formats not supported"
        else:
            h["error"] = "DDS: unsupported pixel format (no FourCC branch matches: the loader returns a null texture)"
        h["mip_count"] = full_chain(h["width"], h["height"]) if h["mip_chain"] else 1
        if h["loadable"]:
            blk = 8 if h["unity_format"] == TF["DXT1"] else 16
            tot, w, hh = 0, h["width"], h["height"]
            for _ in range(h["mip_count"]):
                tot += max(1, (w + 3) // 4) * max(1, (hh + 3) // 4) * blk
                w, hh = max(1, w // 2), max(1, hh // 2)
            h["expected_data_bytes"] = tot
    elif ext == "mbm":
        h["container"] = 2
        off, n, shift = 0, 0, 0
        while True:
            if off >= len(b) or shift > 35:
                h["error"] = "MBM: truncated header"
                return h
            byte = b[off]
            off += 1
            n |= (byte & 0x7F) << shift
            if not byte & 0x80:
                break
            shift += 7
        off += n
        if off + 16 > len(b):
            h["error"] = "MBM: truncated header"
            return h
        w, hh, ttype, bpp = struct.unpack_from("<iiii", b, off)
        h.update(width=w, height=hh, mbm_texture_type=ttype, mbm_bits_per_pixel=bpp, data_offset=off + 16, valid=1, mip_chain=1,
                 is_compressed_flag=1, is_readable=1)
        px = w * hh
        if ttype == 0:
            if bpp == 32:
                h["source_format"] = 4
                h["has_alpha"] = 1
                h["expected_data_bytes"] = px * 4
            elif bpp == 24:
                h["source_format"] = 3
                h["expected_data_bytes"] = px * 3
            else:
                h["error"] = "MBM: bits per pixel other than 24 / 32: texture2D stays null, the loader fails"
                return h
            h["loadable"] = 1
            if w % 4 == 0 and hh % 4 == 0:
                h["compressed"] = 1
                h["unity_format"] = TF["DXT1"] if h["source_format"] == 3 else TF["DXT5"]
            else:
                h["unity_format"] = h["source_format"]
        elif ttype == 1:
            h.update(source_format=5, unity_format=5, is_normal_map=1, has_alpha=1, loadable=1, expected_data_bytes=px * 4)
        else:
            h["error"] = "MBM: unknown texture type: texture2D stays null, the loader fails"
            return h
        h["mip_count"] = full_chain(w, hh)
    elif ext == "png":
        h["container"] = 3
        im = sniff_png(b)
        if im is None:
            h["error"] = "PNG: no IHDR"
            return h
        h.update(valid=1, loadable=1, width=im["w"], height=im["h"], png_color_type=im["ctype"], png_bit_depth=im["depth"],
                 has_alpha=int(im["alpha"]), data_offset=8)
        if nrm:
            nrm_result()
        else:
            decoded = TF["RGBA32"] if im["alpha"] else TF["RGB24"]
            n = name.replace("\\", "/")
            icons = any(("/%s/" % d) in n for d in ("Icons", "Tutorials", "SimpleIcons"))
            fmt = TF["ARGB32"] if icons else decoded
            h.update(is_readable=1, is_compressed_flag=1, source_format=decoded, mip_chain=0 if icons else 1)
            h["unity_format"] = fmt
            if im["w"] % 4 == 0 and im["h"] % 4 == 0:
                h["compressed"] = 1
                h["unity_format"] = TF["DXT1"] if fmt == TF["RGB24"] else TF["DXT5"]
            h["mip_count"] = full_chain(im["w"], im["h"]) if h["mip_chain"] else 1
    elif ext in ("jpg", "jpeg"):
        h["container"] = 4
        im = sniff_jpeg(b)
        if im is None:
            h["error"] = "JPEG: no SOF marker in the buffer"
            return h
        h.update(valid=1, loadable=1, width=im["w"], height=im["h"])
        if nrm:
            nrm_result()
        else:
            h.update(source_format=3, unity_format=3, is_readable=1, is_compressed_flag=1, mip_chain=1)
            if im["w"] % 4 == 0 and im["h"] % 4 == 0:
                h.update(compressed=1, unity_format=TF["DXT1"])
            h["mip_count"] = full_chain(im["w"], im["h"])
    elif ext == "tga":
        h["container"] = 5
        if len(b) < 18:
            h["error"] = "TGA invalid length"
            return h
        itype, w, hh, depth = b[2], struct.unpack_from("<H", b, 12)[0], struct.unpack_from("<H", b, 14)[0], b[16]
        h.update(tga_image_type=itype, width=w, height=hh, tga_pixel_depth=depth, data_offset=18, valid=1)
        if itype not in (2, 10):
            h["error"] = "TGA: image type is not supported"
            return h
        bpp = depth // 8
        if bpp == 4:
            h.update(source_format=4, has_alpha=1)
        elif bpp == 3:
            h["source_format"] = 3
        else:
            h["error"] = "Cannot create texture: Header denotes incorrect format"
            return h
        h.update(loadable=1, mip_chain=1, mip_count=full_chain(w, hh), unity_format=h["source_format"])
        if itype == 2:
            h["expected_data_bytes"] = w * hh * (4 if bpp == 4 else 3)
        if nrm:
            nrm_result()
        else:
            h.update(is_readable=1, is_compressed_flag=1)
    elif ext == "truecolor":
        h["container"] = 6
        im = sniff_png(b)
        is_png = im is not None
        if im is None:
            im = sniff_jpeg(b)
            if im is not None:
                im = dict(im, ctype=-1, depth=-1, alpha=False)
        if im is None:
            h["error"] = "Texture load error (neither a PNG nor a JPEG header)"
            return h
        h.update(valid=1, loadable=1, width=im["w"], height=im["h"], png_color_type=im["ctype"], png_bit_depth=im["depth"],
                 has_alpha=int(im["alpha"]), data_offset=8 if is_png else 2)
        if nrm:
            nrm_result(False)
        else:
            f = 4 if im["alpha"] else 3
            h.update(source_format=f, unity_format=f, mip_chain=0, mip_count=1, is_readable=0, is_compressed_flag=0)
    else:
        h["error"] = "no texture loader for the extension '%s'" % ext
        return h
    return h


# ---------------------------------------------------------------------------------------------------------------------------------
# the writer of the synthetic coverage models (test fixtures; see the module docstring)
class W:
    def __init__(self, seed):
        self.b = bytearray()
        self.rnd = random.Random(seed)

    def i32(self, v):
        self.b += struct.pack("<i", v)

    def e(self, name):
        self.i32(E[name])

    def f(self, bits):
        self.b += struct.pack("<I", bits)

    def fl(self, x):
        self.b += struct.pack("<f", x)

    def rf(self):  # a random float bit pattern: mostly ordinary values, sometimes the awkward ones
        r = self.rnd.random()
        if r < 0.04:
            return self.rnd.choice([0x80000000, 0x7FC00001, 0x7F800000, 0xFF800000, 0x00000001, 0x007FFFFF, 0x7F7FFFFF])
        return struct.unpack("<I", struct.pack("<f", self.rnd.uniform(-100, 100)))[0]

    def rfs(self, n):
        for _ in range(n):
            self.f(self.rf())

    def bool(self, v):
        self.b += b"\x01" if v else b"\x00"

    def s(self, text):
        raw = text.encode("latin-1")
        n = len(raw)
        while True:
            byte = n & 0x7F
            n >>= 7
            self.b.append(byte | (0x80 if n else 0))
            if not n:
                break
        self.b += raw

    def raw(self, data):
        self.b += data


def w_mesh(w, n, verts=True, normals=False, tangents=False, uv=False, uv2=False, colors=False, tris=(), bones=False, bind=0,
           header_int=7, extras=(), order=None):
    """MeshStart ... MeshEnd. `extras` = unknown entry values to put in the middle; `order` = explicit chunk order (default fixed)."""
    w.e("MeshStart")
    w.i32(n)
    w.i32(header_int)
    chunks = order or (["colors"] * colors + ["verts"] * verts + ["uv"] * uv + ["uv2"] * uv2 + ["normals"] * normals +
                       ["tangents"] * tangents + ["tris"] * len(tris) + ["bones"] * bones + ["bind"] * (1 if bind else 0))
    ti = 0
    for c in chunks:
        if c == "colors":
            w.e("MeshVertexColors")
            w.raw(bytes(w.rnd.randrange(256) for _ in range(4 * n)))
        elif c == "verts":
            w.e("MeshVerts")
            w.rfs(3 * n)
        elif c == "uv":
            w.e("MeshUV")
            w.rfs(2 * n)
        elif c == "uv2":
            w.e("MeshUV2")
            w.rfs(2 * n)
        elif c == "normals":
            w.e("MeshNormals")
            w.rfs(3 * n)
        elif c == "tangents":
            w.e("MeshTangents")
            w.rfs(4 * n)
        elif c == "tris":
            w.e("MeshTriangles")
            t = tris[ti % len(tris)]
            ti += 1
            w.i32(len(t))
            for x in t:
                w.i32(x)
        elif c == "bones":
            w.e("MeshBoneWeights")
            for _ in range(n):
                for j in range(4):
                    w.i32(w.rnd.randrange(0, 5))
                    w.f(w.rf())
        elif c == "bind":
            w.e("MeshBindPoses")
            w.i32(bind)
            w.rfs(16 * bind)
        if extras and c == chunks[len(chunks) // 2]:
            for x in extras:
                w.i32(x)
    w.e("MeshEnd")


def w_tex_prop(w, tid):
    w.i32(tid)
    w.rfs(4)


def w_light(w, version):
    w.e("Light")
    w.i32(2)
    w.rfs(2)
    w.rfs(4)
    w.i32(w.rnd.randrange(1, 1 << 20))
    if version > 1:
        w.rfs(1)


def w_camera(w):
    w.e("Camera")
    w.i32(1)
    w.rfs(4)
    w.i32(-1)
    w.bool(True)
    w.rfs(4)


def w_particles(w, slot):
    w.e("ParticleEmitter")
    w.bool(True)
    w.i32(3)
    w.rfs(6)
    w.rfs(4)
    w.bool(False)
    w.rfs(4)
    w.i32(10)
    w.i32(20)
    w.rfs(9)
    w.rfs(3)
    w.bool(True)
    w.bool(False)
    w.rfs(20)
    w.rfs(6)
    w.rfs(8)
    w.bool(True)
    w.bool(True)
    w.rfs(3)
    w.i32(3)
    w.i32(4)
    w.i32(4)
    w.i32(2)
    w.i32(slot)


def w_anim(w, clips, default, play=True):
    """clips: list of (name, [curves]) ; curve = (path, prop, type, pre, post, nkeys)"""
    w.e("Animation")
    w.i32(len(clips))
    for name, curves in clips:
        w.s(name)
        w.rfs(6)
        w.i32(2)
        w.i32(len(curves))
        for path, prop, typ, pre, post, nk in curves:
            w.s(path)
            w.s(prop)
            w.i32(typ)
            w.i32(pre)
            w.i32(post)
            w.i32(nk)
            for _ in range(nk):
                w.rfs(4)
                w.i32(w.rnd.randrange(-5, 5))
    w.s(default)
    w.bool(play)


def w_header(w, version, header="synthetic WP-UE-05 coverage model"):
    w.i32(FILE_TYPE_MODEL_BINARY)
    w.i32(version)
    w.s(header)


def w_node_start(w, name, pos=None, rot=None, scl=None):
    w.s(name)
    w.rfs(3)
    w.rfs(4)
    w.rfs(3)


def model_v5_full():
    """.mu version 5: every render chunk in the generic (ReadMaterial4) layout, nodes 4 deep with duplicate names, bones, ignored entries"""
    w = W(5001)
    w_header(w, 5)
    w_node_start(w, "model")
    # child 0: body, a mesh with every array and three submeshes, renderer with three slots, colliders alongside
    w.e("ChildTransformStart")
    w_node_start(w, "body")
    w.e("TagAndLayer")
    w.s("Untagged")
    w.i32(0)
    w.e("MeshFilter")
    w_mesh(w, 7, normals=True, tangents=True, uv=True, uv2=True, colors=True, tris=[[0, 1, 2, 2, 3, 4], [4, 5, 6], [0, 2, 4, 6, 1, 3]])
    w.e("MeshRenderer")
    w.bool(True)
    w.bool(False)
    w.i32(3)
    for s in (0, 1, 2):
        w.i32(s)
    w.e("BoxCollider2")
    w.bool(False)
    w.rfs(6)
    w.e("MeshCollider2")
    w.bool(True)
    w.bool(True)
    w_mesh(w, 4, tris=[[0, 1, 2, 0, 2, 3]])
    w.e("ChildTransformEnd")
    # child 1: nested chain a / a / b with light, camera, particles, animation
    w.e("ChildTransformStart")
    w_node_start(w, "a")
    w.e("ChildTransformStart")
    w_node_start(w, "a")
    w.e("ChildTransformStart")
    w_node_start(w, "b")
    w_light(w, 5)
    w_camera(w)
    w_particles(w, 3)
    w_anim(w, [("open", [("", "m_LocalPosition.x", 0, 8, 8, 3), ("sub/leaf", "m_LocalRotation.w", 0, 0, 2, 2)]),
               ("lights", [("", "m_Intensity", 2, 8, 8, 2), ("", "_Color.r", 1, 8, 8, 4), ("", "m_Volume", 3, 8, 8, 1)])], "open")
    w.e("ChildTransformEnd")
    w.e("ChildTransformEnd")
    w.e("ChildTransformEnd")
    # child 2: empty mesh (zero vertices, a vertex chunk with no data), a renderer with no slots
    w.e("ChildTransformStart")
    w_node_start(w, "empty")
    w.e("MeshFilter")
    w_mesh(w, 0, tris=[[]])
    w.e("MeshRenderer")
    w.bool(False)
    w.bool(True)
    w.i32(0)
    w.e("ChildTransformEnd")
    # child 3: a skinned mesh renderer; two bones share a name with nodes (first preorder match), one is missing
    w.e("ChildTransformStart")
    w_node_start(w, "skin")
    w.e("SkinnedMeshRenderer")
    w.i32(1)
    w.i32(1)
    w.rfs(6)
    w.i32(2)
    w.bool(True)
    w.i32(4)
    for bn in ("b", "body", "missing", "a"):
        w.s(bn)
    w_mesh(w, 5, normals=True, uv=True, tris=[[0, 1, 2, 2, 3, 4]], bones=True, bind=4, tangents=True)
    w.e("ChildTransformEnd")
    # child 4: entries PartReader has no case for (Material = 11, 99, -3), a mesh with an unknown mesh entry and a repeated chunk
    w.e("ChildTransformStart")
    w_node_start(w, "ignored")
    w.i32(11)
    w.i32(99)
    w.i32(99)
    w.i32(-3)
    w.e("MeshFilter")
    w_mesh(w, 3, normals=True, tris=[[0, 1, 2]], extras=(77, 77, 12), order=["verts", "tris", "verts", "normals", "tris", "uv"])
    w.e("MeshRenderer")
    w.bool(True)
    w.bool(True)
    w.i32(1)
    w.i32(3)
    w.e("ChildTransformEnd")
    # root: the Materials chunk (version 5 layout), then the Textures chunk
    w.e("Materials")
    w.i32(4)
    mats = [("body_mat", "KSP/Bumped Specular", [("_MainTex", 4, 0), ("_BumpMap", 4, 1), ("_SpecColor", 0, None), ("_Shininess", 2, None)]),
            ("glow", "KSP/Emissive/Bumped Specular", [("_MainTex", 4, 0), ("_Emissive", 4, 2), ("_EmissiveColor", 0, None),
                                                      ("_Cutoff", 3, None), ("_Tint", 1, None), ("_NoTex", 4, -1)]),
            ("alpha", "KSP/Alpha/Translucent", [("_MainTex", 4, 3)]),
            ("empty_props", "KSP/Unlit", [])]
    for name, shader, props in mats:
        w.s(name)
        w.s(shader)
        w.i32(len(props))
        for pname, kind, tid in props:
            w.s(pname)
            w.i32(kind)
            if kind in (0, 1):
                w.rfs(4)
            elif kind in (2, 3):
                w.rfs(1)
            else:
                w_tex_prop(w, tid)
    w.e("Textures")
    w.i32(4)
    for fname, ttype in (("Squad/Parts/body.dds", 0), ("bodyNRM.png", 1), ("glow.mbm", 0), ("a.b.alpha.tga", 0)):
        w.s(fname)
        w.i32(ttype)
    return bytes(w.b)


def model_v3_legacy():
    """.mu version 3: the fixed ReadMaterial table, every ShaderType (and Custom 0 and an unknown 99), shadow fields, light with spot angle"""
    w = W(3003)
    w_header(w, 3)
    w_node_start(w, "model")
    w.e("ChildTransformStart")
    w_node_start(w, "mesh")
    w.e("MeshFilter")
    w_mesh(w, 3, normals=True, uv=True, tris=[[0, 1, 2]])
    w.e("MeshRenderer")
    w.bool(False)
    w.bool(False)
    nslots = 19
    w.i32(nslots)
    for s in range(nslots):
        w.i32(s)
    w_light(w, 3)
    w.e("ChildTransformEnd")
    w.e("Materials")
    w.i32(nslots)
    types = [0] + list(range(1, 17)) + [99, 17]
    tid = 0
    for i, st in enumerate(types):
        w.s("mat%d" % i)
        w.i32(st)
        shader, props = LEGACY.get(st, LEGACY_DEFAULT)
        for kind, pname in props:
            if kind == "T":
                w_tex_prop(w, tid if tid % 5 else -1)
                tid += 1
            elif kind == "C":
                w.rfs(4)
            else:
                w.rfs(1)
    # texture ids used: every non -1 one: tid values 1,2,3,4,6,... (tid % 5 != 0); the Textures count = max id + 1
    return bytes(w.b), tid


def model_v3_legacy_full():
    data, tid = model_v3_legacy()
    ids = [t for t in range(tid) if t % 5]
    count = max(ids) + 1
    w = W(3004)
    w.raw(data)
    w.e("Textures")
    w.i32(count)
    for i in range(count):
        w.s("tex%d%s" % (i, [".dds", ".png", ".mbm", ""][i % 4]))
        w.i32(i % 2)
    return bytes(w.b)


def model_v0_old():
    """.mu version 0: MeshRenderer without shadow fields, light without spot angle, legacy materials"""
    w = W(7000)
    w_header(w, 0, "")
    w_node_start(w, "old")
    w.e("MeshFilter")
    w_mesh(w, 4, uv=True, tris=[[0, 1, 2, 1, 2, 3]], header_int=0)
    w.e("MeshRenderer")
    w.i32(2)
    w.i32(0)
    w.i32(1)
    w_light(w, 0)
    w.e("Materials")
    w.i32(2)
    w.s("m0")
    w.i32(1)
    w_tex_prop(w, 0)
    w.s("m1")
    w.i32(2)
    w_tex_prop(w, 1)
    w.rfs(4)
    w.rfs(1)
    w.e("Textures")
    w.i32(2)
    w.s("t0.mbm")
    w.i32(0)
    w.s("t1.dds")
    w.i32(1)
    return bytes(w.b)


def model_v1_shadow():
    """.mu version 1: shadow fields present, light without a spot angle (the spot angle needs version > 1)"""
    w = W(1001)
    w_header(w, 1)
    w_node_start(w, "v1")
    w.e("MeshFilter")
    w_mesh(w, 3, tris=[[0, 1, 2]])
    w.e("MeshRenderer")
    w.bool(False)
    w.bool(True)
    w.i32(1)
    w.i32(0)
    w_light(w, 1)
    w.e("Materials")
    w.i32(1)
    w.s("only")
    w.i32(13)
    w_tex_prop(w, -1)
    w.rfs(4)
    return bytes(w.b)


def model_v2_anim():
    """animation edge cases: a ParticleEmitter (4) curve drops its clip and every later clip and the default clip; a second component
    applies its default clip; empty names; clips without curves; keys with the awkward floats"""
    w = W(2002)
    w_header(w, 2)
    w_node_start(w, "anim")
    w_anim(w, [("ok", [("", "m_LocalScale.x", 0, 8, 8, 2)]), ("drop", [("", "x", 4, 0, 0, 1), ("", "y", 0, 0, 0, 1)]),
               ("after", [("p", "z", 0, 1, 1, 0)]), ("", [])], "after")
    w_anim(w, [("one", [("child", "m_LocalPosition.y", 0, 8, 8, 5)]), ("two", [])], "two", play=False)
    w_anim(w, [], "")
    w.e("ChildTransformStart")
    w_node_start(w, "child")
    w_anim(w, [("c", [("", "m_LocalRotation.x", 0, 8, 8, 3), ("", "m_LocalRotation.y", 3, 8, 8, 0)])], "c")
    w.e("ChildTransformEnd")
    return bytes(w.b)


def model_v4_warn():
    """accepted-but-anomalous: Materials chunk longer than the referenced slots, texture id -2, an unknown ShaderPropertyType (9);
    a camera and a light, a UTF-8 (latin-1 bytes) name longer than 127 bytes (two-byte length prefix)"""
    w = W(4004)
    w_header(w, 4)
    w_node_start(w, "w" * 200)
    w.e("MeshRenderer")
    w.bool(True)
    w.bool(True)
    w.i32(1)
    w.i32(0)
    w_camera(w)
    w.e("Materials")
    w.i32(3)
    w.s("m0")
    w.s("KSP/Diffuse")
    w.i32(3)
    w.s("_MainTex")
    w.i32(4)
    w_tex_prop(w, 0)
    w.s("_Odd")
    w.i32(9)
    w.s("_Neg")
    w.i32(4)
    w_tex_prop(w, -2)
    for nm in ("m1", "m2"):
        w.s(nm)
        w.s("KSP/Unlit")
        w.i32(0)
    w.e("Textures")
    w.i32(1)
    w.s("\\\\dir\\\\x.y.dds")
    w.i32(0)
    return bytes(w.b)


def model_small():
    """small model for the truncation sweep: every prefix of it is parsed by both implementations"""
    w = W(9009)
    w_header(w, 5, "t")
    w_node_start(w, "r")
    w.e("TagAndLayer")
    w.s("t")
    w.i32(3)
    w.e("ChildTransformStart")
    w_node_start(w, "c")
    w.e("MeshFilter")
    w_mesh(w, 2, normals=True, tangents=True, uv=True, uv2=True, colors=True, tris=[[0, 1, 0]], bones=True, bind=1)
    w.e("MeshRenderer")
    w.bool(True)
    w.bool(True)
    w.i32(1)
    w.i32(0)
    w_light(w, 5)
    w.e("ChildTransformEnd")
    w.e("Materials")
    w.i32(1)
    w.s("m")
    w.s("KSP/Specular")
    w.i32(2)
    w.s("_MainTex")
    w.i32(4)
    w_tex_prop(w, 0)
    w.s("_Shininess")
    w.i32(2)
    w.rfs(1)
    w.e("Textures")
    w.i32(1)
    w.s("x.dds")
    w.i32(0)
    return bytes(w.b)


def error_cases():
    """(label, bytes): both implementations must refuse each"""
    out = []
    w = W(1)
    w.i32(1234)
    w.i32(5)
    w.s("")
    out.append(("wrong file type", bytes(w.b)))
    w = W(2)
    w_header(w, 5)
    w_node_start(w, "r")
    w.e("Textures")
    w.i32(2)  # no material references a texture: TextureError 2 0
    out.append(("TextureError", bytes(w.b)))
    w = W(3)
    w_header(w, 5)
    w_node_start(w, "r")
    w.e("MeshFilter")
    w.e("Light")  # not MeshStart
    out.append(("Mesh Error", bytes(w.b)))
    w = W(4)
    w_header(w, 5)
    w_node_start(w, "r")
    w.e("MeshFilter")
    w.e("MeshStart")
    w.i32(-1)
    w.i32(0)
    out.append(("negative vertex count", bytes(w.b)))
    w = W(5)
    w_header(w, 5)
    w_node_start(w, "r")
    w.e("MeshRenderer")
    w.bool(True)
    w.bool(True)
    w.i32(1)
    w.i32(-1)
    out.append(("negative material id", bytes(w.b)))
    w = W(6)
    w_header(w, 5)
    w_node_start(w, "r")
    w.e("MeshFilter")
    w.e("MeshStart")
    w.i32(1000000000)  # a vertex array that cannot be in the file
    w.i32(0)
    w.e("MeshVerts")
    out.append(("vertex array past the end", bytes(w.b)))
    return out


MODELS = [("syn_v5_full.mu", model_v5_full, "/Squad/Parts/Syn"), ("syn_v3_legacy.mu", model_v3_legacy_full, "Squad/Parts/Legacy"),
          ("syn_v0_old.mu", model_v0_old, ""), ("syn_v1_shadow.mu", model_v1_shadow, "Squad/Parts/Syn"),
          ("syn_v2_anim.mu", model_v2_anim, "Squad/Parts/Syn"), ("syn_v4_warn.mu", model_v4_warn, "Squad/Parts/Syn"),
          ("syn_small.mu", model_small, "Squad/Parts/Syn")]


# ---------------------------------------------------------------------------------------------------------------------------------
# synthetic texture headers
def dds_bytes(w, h, fcc, mips, pf_flags=4, caps=0x1000, extra_dx10=False, trunc=0, payload=0):
    b = bytearray(b"DDS ")
    hdr = [124, 0x81007 if mips > 1 else 0x1007, h, w, 0, 0, mips] + [0] * 11
    b += struct.pack("<%dI" % len(hdr), *hdr)
    b += struct.pack("<8I", 32, pf_flags, fcc, 0, 0, 0, 0, 0)
    b += struct.pack("<5I", caps | (0x400008 if mips > 1 else 0), 0, 0, 0, 0)
    if extra_dx10:
        b += struct.pack("<5I", 98, 3, 0, 1, 0)
    return bytes(b[:len(b) - trunc]) + bytes(payload)


def mbm_bytes(w, h, ttype, bpp, magic="KSP-MBM"):
    b = bytearray()
    raw = magic.encode()
    b.append(len(raw))
    b += raw
    b += struct.pack("<iiii", w, h, ttype, bpp)
    return bytes(b)


def png_bytes(w, h, ctype, depth=8, trns=False, idat=True):
    b = bytearray(b"\x89PNG\r\n\x1a\n")
    b += struct.pack(">I", 13) + b"IHDR" + struct.pack(">IIBBBBB", w, h, depth, ctype, 0, 0, 0) + b"\0\0\0\0"
    if trns:
        b += struct.pack(">I", 1) + b"tRNS" + b"\0" + b"\0\0\0\0"
    if idat:
        b += struct.pack(">I", 2) + b"IDAT" + b"xx" + b"\0\0\0\0"
    return bytes(b)


def jpeg_bytes(w, h):
    b = bytearray(b"\xff\xd8")
    b += b"\xff\xe0" + struct.pack(">H", 16) + b"JFIF\0\1\1\0\0\1\0\1\0\0"
    b += b"\xff\xdb" + struct.pack(">H", 4) + b"\0\0"
    b += b"\xff\xc0" + struct.pack(">H", 11) + b"\x08" + struct.pack(">HH", h, w) + b"\x03"
    return bytes(b)


def tga_bytes(w, h, itype, depth):
    return bytes([0, 0, itype, 0, 0, 0, 0, 0, 0, 0, 0, 0]) + struct.pack("<HHBB", w, h, depth, 0)


def texture_cases():
    D1, D3, D5, D2, D4, DX = (fourcc(x) for x in ("DXT1", "DXT3", "DXT5", "DXT2", "DXT4", "DX10"))
    c = []

    def add(name, data, fsize=0):
        c.append((name, data, fsize))
    add("a/b/colors.dds", dds_bytes(256, 128, D1, 9), 128 + 22000)
    add("a/b/colors.DDS", dds_bytes(256, 256, D5, 9, payload=0), 0)
    add("a/b/nomips.dds", dds_bytes(64, 64, D5, 1), 128 + 4096)
    add("a/b/nonsq.dds", dds_bytes(100, 52, D1, 1), 128)
    add("a/b/tiny.dds", dds_bytes(1, 1, D1, 1), 136)
    add("a/b/normal.dds", dds_bytes(512, 512, D5, 10, pf_flags=0x80000004), 1000)
    add("a/b/normal2.dds", dds_bytes(512, 512, D5, 1, pf_flags=0x80004), 1000)
    add("a/b/dxt3.dds", dds_bytes(32, 32, D3, 1))
    add("a/b/dxt2.dds", dds_bytes(32, 32, D2, 1))
    add("a/b/dxt4.dds", dds_bytes(32, 32, D4, 1))
    add("a/b/dx10.dds", dds_bytes(32, 32, DX, 1, extra_dx10=True), 200)
    add("a/b/dx10trunc.dds", dds_bytes(32, 32, DX, 1))
    add("a/b/rgba.dds", dds_bytes(32, 32, 0, 1, pf_flags=0x41))
    add("a/b/short.dds", dds_bytes(32, 32, D1, 1)[:100])
    add("a/b/notdds.dds", b"XXXX" + bytes(200))
    add("a/b/empty.dds", b"")
    add("a/b/a.mbm", mbm_bytes(64, 32, 0, 32), 20 + 64 * 32 * 4)
    add("a/b/b.mbm", mbm_bytes(30, 30, 0, 24), 100)
    add("a/b/c.mbm", mbm_bytes(8, 8, 1, 32), 0)
    add("a/b/d.mbm", mbm_bytes(8, 8, 0, 16))
    add("a/b/e.mbm", mbm_bytes(8, 8, 7, 32))
    add("a/b/long magic.mbm", mbm_bytes(4, 4, 0, 24, magic="m" * 200))
    add("a/b/short.mbm", mbm_bytes(4, 4, 0, 24)[:10])
    add("Squad/Parts/x/tex.png", png_bytes(64, 64, 6), 5000)
    add("Squad/Parts/x/tex2.png", png_bytes(60, 64, 2), 5000)
    add("Squad/Parts/x/tex3.png", png_bytes(64, 64, 3, trns=True), 5000)
    add("Squad/Parts/x/tex4.png", png_bytes(8, 8, 0, depth=16), 100)
    add("Squad/Parts/x/tex5.png", png_bytes(8, 8, 4), 100)
    add("Squad/Parts/x/noidat.png", png_bytes(16, 16, 6, idat=False))
    add("Squad/Alarms/Icons/ap.png", png_bytes(24, 24, 6), 500)
    add("Squad/Alarms/Icons/odd.png", png_bytes(25, 24, 6), 500)
    add("Squad\\Tutorials\\t.png", png_bytes(32, 32, 2), 500)
    add("Squad/SimpleIcons/s.png", png_bytes(32, 32, 2), 500)
    add("Squad/Flags/f.png", png_bytes(256, 192, 2), 5000)
    add("Squad/Parts/x/bodyNRM.png", png_bytes(128, 128, 2), 5000)
    add("Squad/Parts/x/nrm.png", png_bytes(128, 128, 2), 5000)
    add("Squad/Parts/x/bad.png", b"\x89PNG\r\n\x1a\n" + bytes(10))
    add("Squad/Parts/x/photo.jpg", jpeg_bytes(100, 64), 4000)
    add("Squad/Parts/x/photo2.jpeg", jpeg_bytes(101, 64), 4000)
    add("Squad/Parts/x/photoNRM.jpg", jpeg_bytes(64, 64), 4000)
    add("Squad/Parts/x/bad.jpg", b"\xff\xd8\xff\xe0\x00\x04\x00\x00")
    add("Squad/Parts/x/t24.tga", tga_bytes(16, 16, 2, 24), 18 + 16 * 16 * 3)
    add("Squad/Parts/x/t32.tga", tga_bytes(16, 8, 10, 32), 300)
    add("Squad/Parts/x/t8.tga", tga_bytes(16, 16, 2, 8), 300)
    add("Squad/Parts/x/t16.tga", tga_bytes(16, 16, 2, 16), 300)
    add("Squad/Parts/x/tmap.tga", tga_bytes(16, 16, 1, 24), 300)
    add("Squad/Parts/x/tNRM.tga", tga_bytes(16, 16, 2, 32), 300)
    add("Squad/Parts/x/short.tga", tga_bytes(16, 16, 2, 24)[:10])
    add("Squad/Parts/x/tc.truecolor", png_bytes(32, 32, 6), 1000)
    add("Squad/Parts/x/tc2.truecolor", png_bytes(32, 32, 2), 1000)
    add("Squad/Parts/x/tc3.truecolor", jpeg_bytes(48, 40), 1000)
    add("Squad/Parts/x/tcNRM.truecolor", png_bytes(32, 32, 2), 1000)
    add("Squad/Parts/x/bad.truecolor", bytes(40))
    add("Squad/Parts/x/what.bmp", bytes(40))
    add("Squad/Parts/x/noext", bytes(40))
    return c


# ---------------------------------------------------------------------------------------------------------------------------------
def main():
    here = os.path.dirname(os.path.abspath(__file__))
    core = os.path.normpath(os.path.join(here, "..", ".."))
    ap = argparse.ArgumentParser()
    ap.add_argument("--data-dir", default=os.path.join(core, "tests", "data"))
    ap.add_argument("--mirror", default=os.path.join(core, "testdata", "GameData-mirror"))
    args = ap.parse_args()
    out = {"schema": "kspcore-wpue05-reference/1",
           "about": "independent parse (tools/wpue05/mu_render_reference.py) of the SYNTHETIC coverage models in tests/data/wpue05 (labelled "
                    "test fixtures for format coverage, not game assets) and of texture headers; floats are raw bit patterns",
           "models": [], "truncation": {}, "errors": [], "textures": [], "mirror_textures": {}}
    mdir = os.path.join(args.data_dir, "wpue05")
    os.makedirs(mdir, exist_ok=True)
    for fname, builder, parent in MODELS:
        data = builder()
        with open(os.path.join(mdir, fname), "wb") as f:
            f.write(data)
        m = parse_mu(data)
        full = rows(m, parent, True)
        compact = rows(m, parent, False)
        out["models"].append({"file": fname, "parent_url": parent, "size": len(data), "sha256": hashlib.sha256(data).hexdigest(),
                              "stats": stats(m), "digest": "%016x" % digest(compact), "rows": full, "compact": compact})
    # every prefix of the small model: both implementations must agree on ok / refused and on the digest of the accepted ones
    small = model_small()
    pref = []
    for n in range(len(small) + 1):
        try:
            m = parse_mu(small[:n])
            pref.append([n, 1, "%016x" % digest(rows(m, "", False))])
        except ParseError:
            pref.append([n, 0, ""])
        except struct.error:
            pref.append([n, 0, ""])
    out["truncation"] = {"file": "syn_small.mu", "prefixes": pref}
    for label, data in error_cases():
        try:
            parse_mu(data)
            raise SystemExit("error case parsed: " + label)
        except ParseError:
            pass
        out["errors"].append({"label": label, "hex": data.hex()})
    for name, data, fsize in texture_cases():
        out["textures"].append({"name": name, "hex": data.hex(), "file_size": fsize, "expect": ref_header(name, data, fsize)})
    # the GameData mirror's header-only stubs (core/testdata/GameData-mirror: 895 DDS 128-byte headers, 2 MBM headers, zero-byte rest)
    gd = os.path.join(args.mirror, "GameData")
    fields = list(blank_header().keys())
    mirror = []
    for root, _, files in os.walk(gd):
        for fn in sorted(files):
            if fn.lower().endswith((".dds", ".mbm")):
                p = os.path.join(root, fn)
                with open(p, "rb") as f:
                    data = f.read()
                rel = os.path.relpath(p, args.mirror).replace("\\", "/")
                rec = ref_header(rel, data, len(data))
                mirror.append([rel, hashlib.sha256(data).hexdigest(), len(data)] + [rec[k] for k in fields])
    mirror.sort(key=lambda r: r[0])
    out["mirror_textures"] = {"fields": ["path", "sha256", "size"] + fields, "rows": mirror}
    path = os.path.join(args.data_dir, "wpue05_reference.json")
    with open(path, "w", newline="\n") as f:
        json.dump(out, f, separators=(",", ":"))
    print("wrote %s: %d models, %d prefix cases, %d error cases, %d texture cases, %d mirror stubs" %
          (path, len(out["models"]), len(pref), len(out["errors"]), len(out["textures"]), len(mirror)))


if __name__ == "__main__":
    main()
