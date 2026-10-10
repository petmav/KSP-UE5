#!/usr/bin/env python
"""WP-17b: read the transform hierarchy of the staged engine .mu models and write core/data/engine_thrust_transforms.json.

Run:  python -I core/tools/wp17/mu_transforms.py [--staging DIR] [--out FILE]

Input  : core/data/mu_staging/<partName>/{*.cfg,*.mu} + MANIFEST.json (staged copy of the original game files; the
         manifest records the GameData origin path and sha256 of every file).
Output : core/data/engine_thrust_transforms.json (per part, per thrust transform: the local chain from the part's
         "model" child down to the transform, the part-space rotation and forward vector).

Spec (decompiled/KSP-Decompiled/Assembly-CSharp/):
  PartReader.cs   binary .mu reader (every entry type is consumed exactly as PartReader does; payloads are skipped)
  PartLoader.cs   CompileModel (:1765) places the model under the part; CompilePart (:664) supplies rescaleFactor
  Part.cs         FindModelTransforms (:5147) / FindHeirarchyTransforms (:5161): pre-order search below partTransform.Find("model")
  ModuleEngines.cs OnLoad (:665): thrustTransforms = part.FindModelTransforms(thrustVectorTransformName)

Float rule: every quantity is a float32 (numpy.float32, one operation per rounding); the quaternion product is the native
Transform code's (see qmul).
"""
import argparse
import hashlib
import json
import os
import struct
import sys

import numpy as np

F = np.float32
FILE_TYPE_MODEL_BINARY = 76543  # PartToolsLib/FileType.cs

# PartToolsLib/EntryType.cs (declaration order = value)
(ChildTransformStart, ChildTransformEnd, Animation, MeshCollider, SphereCollider, CapsuleCollider, BoxCollider, MeshFilter,
 MeshRenderer, SkinnedMeshRenderer, Materials, Material, Textures, MeshStart, MeshVerts, MeshUV, MeshUV2, MeshNormals,
 MeshTangents, MeshTriangles, MeshBoneWeights, MeshBindPoses, MeshEnd, Light, TagAndLayer, MeshCollider2, SphereCollider2,
 CapsuleCollider2, BoxCollider2, WheelCollider, Camera, ParticleEmitter, MeshVertexColors) = range(33)

# PartToolsLib/ShaderType.cs
(Custom, Diffuse, Specular, Bumped, BumpedSpecular, Emissive, EmissiveSpecular, EmissiveBumpedSpecular, AlphaCutout,
 AlphaCutoutBumped, Alpha, AlphaSpecular, AlphaUnlit, Unlit, ParticleAlpha, ParticleAdditive, BumpedSpecularMap) = range(17)
# PartReader.ShaderPropertyType (PartReader.cs:10)
SPT_Color, SPT_Vector, SPT_Float, SPT_Range, SPT_TexEnv = range(5)


class Reader:
    """System.IO.BinaryReader over a bytes object (little endian)."""

    def __init__(self, data):
        self.d = data
        self.p = 0

    def eof(self):
        return self.p == len(self.d)

    def take(self, n):
        if self.p + n > len(self.d):
            raise EOFError("read past end of file at %d" % self.p)
        b = self.d[self.p:self.p + n]
        self.p += n
        return b

    def i32(self):
        return struct.unpack("<i", self.take(4))[0]

    def f32bits(self):
        return struct.unpack("<I", self.take(4))[0]

    def f32(self):
        return struct.unpack("<f", self.take(4))[0]

    def boolean(self):
        return self.take(1)[0] != 0

    def byte(self):
        return self.take(1)[0]

    def string(self):  # BinaryReader.ReadString: 7-bit encoded byte length + UTF-8
        n = 0
        shift = 0
        while True:
            b = self.byte()
            n |= (b & 0x7F) << shift
            if not b & 0x80:
                break
            shift += 7
        return self.take(n).decode("utf-8")

    def skip_floats(self, n):
        self.take(4 * n)


class Node:
    def __init__(self, name, pos, rot, scl, parent):
        self.name, self.pos, self.rot, self.scl, self.parent = name, pos, rot, scl, parent
        self.children = []


class MuFile:
    """PartReader state: fileVersion and the texture-dummy count (TextureDummyList.Count)."""

    def __init__(self, data):
        self.r = Reader(data)
        self.version = 0
        self.texture_dummies = 0
        self.root = None

    # PartReader.cs:140 Read
    def read(self):
        r = self.r
        if r.i32() != FILE_TYPE_MODEL_BINARY:
            raise ValueError("incorrect file type")
        self.version = r.i32()
        r.string()
        self.root = self.read_child(None)
        return self.root

    # PartReader.cs:205 ReadChild
    def read_child(self, parent):
        r = self.r
        name = r.string()
        pos = [r.f32bits() for _ in range(3)]
        rot = [r.f32bits() for _ in range(4)]
        scl = [r.f32bits() for _ in range(3)]
        node = Node(name, pos, rot, scl, parent)
        while not r.eof():
            et = r.i32()
            if et == ChildTransformStart:  # :216
                node.children.append(self.read_child(node))
            elif et == Animation:  # :219 (fileVersion >= 3 and < 3 call the same ReadAnimation)
                self.read_animation()
            elif et == MeshCollider:  # :229
                r.boolean()
                self.read_mesh()
            elif et == SphereCollider:  # :240
                r.skip_floats(4)
            elif et == CapsuleCollider:  # :247
                r.f32(); r.i32(); r.skip_floats(3)
            elif et == BoxCollider:  # :255
                r.skip_floats(6)
            elif et == MeshFilter:  # :262
                self.read_mesh()
            elif et == MeshRenderer:  # :265
                self.read_mesh_renderer()
            elif et == SkinnedMeshRenderer:  # :268
                self.read_skinned_mesh_renderer()
            elif et == Materials:  # :271
                n = r.i32()
                for _ in range(n):
                    if self.version < 4:
                        self.read_material()
                    else:
                        self.read_material4()
            elif et == Textures:  # :292
                self.read_textures()
            elif et == Light:  # :295
                self.read_light()
            elif et == TagAndLayer:  # :298
                r.string(); r.i32()
            elif et == MeshCollider2:  # :301
                r.boolean(); r.boolean()
                self.read_mesh()
            elif et == SphereCollider2:  # :314
                r.boolean(); r.skip_floats(4)
            elif et == CapsuleCollider2:  # :322
                r.boolean(); r.skip_floats(2); r.i32(); r.skip_floats(3)
            elif et == BoxCollider2:  # :332
                r.boolean(); r.skip_floats(6)
            elif et == WheelCollider:  # :340 mass, radius, suspensionDistance, center(3), spring(3), 2 friction curves (5 each)
                r.skip_floats(3 + 3 + 3 + 5 + 5)
            elif et == Camera:  # :372
                self.read_camera()
            elif et == ParticleEmitter:  # :375
                self.read_particles()
            elif et == ChildTransformEnd:  # :378
                return node
            # any other value: PartReader's switch has no case, nothing is consumed
        return node

    # PartReader.cs:441 ReadMeshRenderer
    def read_mesh_renderer(self):
        r = self.r
        if self.version >= 1:
            r.boolean(); r.boolean()
        n = r.i32()
        for _ in range(n):
            r.i32()

    # PartReader.cs:468 ReadSkinnedMeshRenderer
    def read_skinned_mesh_renderer(self):
        r = self.r
        n = r.i32()
        for _ in range(n):
            r.i32()
        r.skip_floats(6)
        r.i32()
        r.boolean()
        nb = r.i32()
        for _ in range(nb):
            r.string()
        self.read_mesh()

    # PartReader.cs:495 ReadMesh
    def read_mesh(self):
        r = self.r
        if r.i32() != MeshStart:
            raise ValueError("Mesh Error")
        n = r.i32()
        r.i32()
        while True:
            et = r.i32()
            if et == MeshEnd:
                break
            if et == MeshVertexColors:  # :519
                r.take(4 * n)
            elif et == MeshVerts or et == MeshNormals:  # :529 / :559
                r.skip_floats(3 * n)
            elif et == MeshUV or et == MeshUV2:  # :539 / :549
                r.skip_floats(2 * n)
            elif et == MeshTangents:  # :569
                r.skip_floats(4 * n)
            elif et == MeshTriangles:  # :579
                r.take(4 * r.i32())
            elif et == MeshBoneWeights:  # :595
                r.take(32 * n)
            elif et == MeshBindPoses:  # :613
                r.skip_floats(16 * r.i32())

    # PartReader.cs:790 ReadMaterialTexture
    def read_material_texture(self):
        r = self.r
        tid = r.i32()
        if tid != -1 and tid >= self.texture_dummies:  # TextureDummyList.AddTextureDummy (:109)
            self.texture_dummies = tid + 1
        r.skip_floats(4)

    # PartReader.cs:646 ReadMaterial (fileVersion < 4)
    def read_material(self):
        r = self.r
        r.string()
        st = r.i32()
        rt, rc = self.read_material_texture, lambda: r.skip_floats(4)
        rf = lambda: r.skip_floats(1)
        seq = {
            Specular: [rt, rc, rf], Bumped: [rt, rt], BumpedSpecular: [rt, rt, rc, rf], Emissive: [rt, rt, rc],
            EmissiveSpecular: [rt, rc, rf, rt, rc], EmissiveBumpedSpecular: [rt, rt, rc, rf, rt, rc],
            AlphaCutout: [rt, rf], AlphaCutoutBumped: [rt, rt, rf], Alpha: [rt], AlphaSpecular: [rt, rf, rc, rf],
            AlphaUnlit: [rt, rc], Unlit: [rt, rc], ParticleAlpha: [rt, rc, rf], ParticleAdditive: [rt, rc, rf],
            BumpedSpecularMap: [rt, rt, rt, rf, rf],
        }.get(st, [rt])  # default (Custom, Diffuse): _MainTex only
        for fn in seq:
            fn()

    # PartReader.cs:758 ReadMaterial4
    def read_material4(self):
        r = self.r
        r.string(); r.string()
        n = r.i32()
        for _ in range(n):
            r.string()
            t = r.i32()
            if t in (SPT_Color, SPT_Vector):
                r.skip_floats(4)
            elif t in (SPT_Float, SPT_Range):
                r.skip_floats(1)
            elif t == SPT_TexEnv:
                self.read_material_texture()

    # PartReader.cs:385 ReadTextures
    def read_textures(self):
        r = self.r
        n = r.i32()
        if n != self.texture_dummies:
            # PartReader logs "TextureError" and returns without consuming the entries: the stream would be misaligned
            raise ValueError("TextureError: %d %d" % (n, self.texture_dummies))
        for _ in range(n):
            r.string(); r.i32()

    # PartReader.cs:802 ReadAnimation (clips; ReadAnimationEvents :872 is unused by ReadChild)
    def read_animation(self):
        r = self.r
        nclips = r.i32()
        for _ in range(nclips):
            r.string()
            r.skip_floats(6)
            r.i32()
            ncurves = r.i32()
            for _ in range(ncurves):
                r.string(); r.string(); r.i32()
                r.i32(); r.i32()
                nk = r.i32()
                for _ in range(nk):
                    r.skip_floats(4)
                    r.i32()
        r.string()
        r.boolean()

    # PartReader.cs:950 ReadLight
    def read_light(self):
        r = self.r
        r.i32(); r.skip_floats(2); r.skip_floats(4); r.i32()
        if self.version > 1:
            r.f32()

    # PartReader.cs:970 ReadCamera
    def read_camera(self):
        r = self.r
        r.i32(); r.skip_floats(4); r.i32(); r.boolean(); r.skip_floats(4)

    # PartReader.cs:985 ReadParticles
    def read_particles(self):
        r = self.r
        r.boolean(); r.i32()
        r.skip_floats(3 + 2 + 1 + 4)  # shape3D, shape2D, shape1D, color
        r.boolean()
        r.skip_floats(4)  # minSize maxSize minEnergy maxEnergy
        r.i32(); r.i32()
        r.skip_floats(3 * 3)  # worldVelocity localVelocity rndVelocity
        r.skip_floats(3)  # emitterVelocityScale angularVelocity rndAngularVelocity
        r.boolean(); r.boolean()
        r.skip_floats(4 * 5)  # colorAnimation[5]
        r.skip_floats(6)  # worldRotationAxis localRotationAxis
        r.skip_floats(1 + 3 + 3 + 1)  # sizeGrow rndForce force damping
        r.boolean(); r.boolean()
        r.skip_floats(3)  # lengthScale velocityScale maxParticleSize
        r.i32()
        r.i32(); r.i32(); r.i32()
        r.i32()


# ---------------------------------------------------------------------------------------------------------------------
# float32 helpers

def bits_to_f(b):
    return F(struct.unpack("<f", struct.pack("<I", b))[0])


def f_to_bits(f):
    return struct.unpack("<I", struct.pack("<f", float(F(f))))[0]


def vec_bits(v):
    return ["%08x" % f_to_bits(x) for x in v]


def qmul(a, b):
    """Quaternionf operator* of the native Transform code (UnityNative::Multiply in core/src/unity/quaternion_native.cpp), float32 per
    operation: x = ((ay*bz - az*by) + aw*bx) + bw*ax (y, z cyclic); w = aw*bw - ((ax*bx + ay*by) + az*bz). (x, y, z, w) tuples."""
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    cx = F(F(ay * bz) - F(az * by))
    cy = F(F(az * bx) - F(ax * bz))
    cz = F(F(ax * by) - F(ay * bx))
    dd = F(F(F(ax * bx) + F(ay * by)) + F(az * bz))
    return (F(F(cx + F(aw * bx)) + F(bw * ax)),
            F(F(cy + F(aw * by)) + F(bw * ay)),
            F(F(cz + F(aw * bz)) + F(bw * az)),
            F(F(aw * bw) - dd))


def qrot_forward(q):
    """Quaternion.operator*(Quaternion, Vector3.forward)  (Quaternion.cs): rotation * (0,0,1), float32 per operation."""
    x, y, z, w = q
    num = F(x * F(2)); num2 = F(y * F(2)); num3 = F(z * F(2))
    num4 = F(x * num); num5 = F(y * num2); num6 = F(z * num3)
    num7 = F(x * num2); num8 = F(x * num3); num9 = F(y * num3)
    num10 = F(w * num); num11 = F(w * num2); num12 = F(w * num3)
    # vector (0, 0, 1): result.x = (1 - (num5 + num6)) * 0 + (num7 - num12) * 0 + (num8 + num11) * 1
    rx = F(F(F(F(F(1) - F(num5 + num6)) * F(0)) + F(F(num7 - num12) * F(0))) + F(F(num8 + num11) * F(1)))
    ry = F(F(F(F(num7 + num12) * F(0)) + F(F(F(1) - F(num4 + num6)) * F(0))) + F(F(num9 - num10) * F(1)))
    rz = F(F(F(F(num8 - num11) * F(0)) + F(F(num9 + num10) * F(0))) + F(F(F(1) - F(num4 + num5)) * F(1)))
    return (rx, ry, rz)


def qrot(q, v):
    """Quaternion.operator*(Quaternion, Vector3) (Quaternion.cs), float32 per operation."""
    x, y, z, w = q
    num = F(x * F(2)); num2 = F(y * F(2)); num3 = F(z * F(2))
    num4 = F(x * num); num5 = F(y * num2); num6 = F(z * num3)
    num7 = F(x * num2); num8 = F(x * num3); num9 = F(y * num3)
    num10 = F(w * num); num11 = F(w * num2); num12 = F(w * num3)
    vx, vy, vz = v
    return (F(F(F(F(F(1) - F(num5 + num6)) * vx) + F(F(num7 - num12) * vy)) + F(F(num8 + num11) * vz)),
            F(F(F(F(num7 + num12) * vx) + F(F(F(1) - F(num4 + num6)) * vy)) + F(F(num9 - num10) * vz)),
            F(F(F(F(num8 - num11) * vx) + F(F(num9 + num10) * vy)) + F(F(F(1) - F(num4 + num5)) * vz)))


def part_space_position(chain):
    """Position of the leaf relative to the part (informational: delta-v reads only the forward vectors; the engine CoT
    position uses it). p = parent position + parent rotation * (parent scale * local position), root first."""
    pos = (F(0), F(0), F(0))
    rot = (F(0), F(0), F(0), F(1))
    scl = (F(1), F(1), F(1))
    for n in chain:
        lp = tuple(bits_to_f(b) for b in n.pos)
        scaled = (F(scl[0] * lp[0]), F(scl[1] * lp[1]), F(scl[2] * lp[2]))
        r = qrot(rot, scaled)
        pos = (F(pos[0] + r[0]), F(pos[1] + r[1]), F(pos[2] + r[2]))
        lr = tuple(bits_to_f(b) for b in n.rot)
        rot = qmul(rot, lr)
        ls = tuple(bits_to_f(b) for b in n.scl)
        scl = (F(scl[0] * ls[0]), F(scl[1] * ls[1]), F(scl[2] * ls[2]))
    return pos


def parse_cfg_model_nodes(cfg_text):
    """The MODEL nodes of a part cfg: list of dicts of their key/value pairs (PartLoader.CompileModel reads model/scale/position/rotation)
    plus the part-level rescaleFactor (default 1.25, PartLoader.cs:664)."""
    rescale = F(1.25)
    models = []
    depth = 0
    cur = None
    pending = None
    for raw in cfg_text.splitlines():
        line = raw.split("//")[0].strip()
        if not line:
            continue
        if line == "{":
            depth += 1
            if pending is not None and depth == 2:
                cur = pending
                pending = None
            continue
        if line == "}":
            if cur is not None and depth == 2:
                models.append(cur)
                cur = None
            depth -= 1
            continue
        if depth == 1 and "=" not in line:
            pending = {} if line == "MODEL" else None
            continue
        if "=" in line:
            k, v = [s.strip() for s in line.split("=", 1)]
            if depth == 1 and k == "rescaleFactor":
                rescale = F(v)
            elif depth == 2 and cur is not None:
                cur[k] = v
    return rescale, models


def find_hierarchy(node, name, out):
    """Part.FindHeirarchyTransforms (Part.cs:5161): pre-order, matches gameObject.name."""
    if node is None:
        return
    if node.name == name:
        out.append(node)
    for c in node.children:
        find_hierarchy(c, name, out)


def build_part_chain(root, rescale, models):
    """Place the .mu root under the part as PartLoader.CompileModel (:1765) does. Returns the 'model' node that
    Part.FindModelTransforms searches (partTransform.Find("model")).

    - one or more MODEL nodes (:1768-1851): a synthetic "model" child (localScale = rescaleFactor, pos 0, rot identity), each
      MODEL instantiates the .mu root below it (NestToParent: position 0, rotation identity; scale = cfg scale or 1;
      position / rotation from the cfg when present).  The .mu root keeps its file name.
    - no MODEL node (:1914-1927): the .mu root itself is renamed "model", pos 0, rot identity, scale rescaleFactor."""
    one = f_to_bits(1.0)
    zero = f_to_bits(0.0)
    if models:
        if len(models) != 1:
            raise ValueError("more than one MODEL node: not needed for the staged engines")
        m = models[0]
        for k in ("position", "rotation", "parent", "name"):
            if k in m:
                raise ValueError("MODEL key '%s' not handled (not present in the staged engine cfgs)" % k)
        if "scale" in m:
            scl = [f_to_bits(F(s)) for s in m["scale"].replace(" ", "").split(",")]
        else:
            scl = [one, one, one]
        model = Node("model", [zero] * 3, [zero, zero, zero, one], [f_to_bits(rescale)] * 3, None)
        root.parent = model
        root.pos, root.rot, root.scl = [zero] * 3, [zero, zero, zero, one], scl
        model.children = [root]
        return model
    root.name = "model"
    root.parent = None
    root.pos, root.rot, root.scl = [zero] * 3, [zero, zero, zero, one], [f_to_bits(rescale)] * 3
    return root


def chain_of(node):
    out = []
    while node is not None:
        out.append(node)
        node = node.parent
    return list(reversed(out))


def local_rot(n):
    return tuple(bits_to_f(b) for b in n.rot)


def part_space_rotation(chain):
    """World rotation relative to the part (the part transform itself is the identity in the prefab). Unity's
    Transform.rotation walks from the transform to the root: rot = localRot(leaf); then rot = localRot(parent) * rot, up to
    the root (right-nested product)."""
    rot = local_rot(chain[-1])
    for n in reversed(chain[:-1]):
        rot = qmul(local_rot(n), rot)
    return rot


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        h.update(f.read())
    return h.hexdigest()


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.abspath(os.path.join(here, "..", "..", ".."))
    ap = argparse.ArgumentParser()
    ap.add_argument("--staging", default=os.path.join(root_dir, "core", "data", "mu_staging"))
    ap.add_argument("--out", default=os.path.join(root_dir, "core", "data", "engine_thrust_transforms.json"))
    args = ap.parse_args()

    manifest = json.load(open(os.path.join(args.staging, "MANIFEST.json"), encoding="utf-8"))
    parts = {}
    for part_name in sorted(manifest):
        entry = manifest[part_name]
        files = {f["file"]: f for f in entry["files"]}
        cfg_name = [n for n in files if n.endswith(".cfg")][0]
        mu_name = [n for n in files if n.endswith(".mu")][0]
        cfg_path = os.path.join(args.staging, part_name, cfg_name)
        mu_path = os.path.join(args.staging, part_name, mu_name)
        for n, f in files.items():  # integrity: the staged bytes are the manifest's bytes
            if sha256(os.path.join(args.staging, part_name, n)) != f["sha256"]:
                raise SystemExit("sha256 mismatch: %s/%s" % (part_name, n))
        data = open(mu_path, "rb").read()
        mf = MuFile(data)
        root = mf.read()
        if not mf.r.eof():
            raise SystemExit("%s: reader stopped at %d of %d" % (mu_name, mf.r.p, len(data)))
        rescale, models = parse_cfg_model_nodes(open(cfg_path, encoding="utf-8").read())
        model = build_part_chain(root, rescale, models)
        transforms = []
        for tname in entry["thrustVectorTransformName"]:
            found = []
            find_hierarchy(model, tname, found)  # Part.FindModelTransforms (Part.cs:5147)
            for node in found:
                ch = chain_of(node)
                q = part_space_rotation(ch)
                fwd = qrot_forward(q)
                transforms.append({
                    "name": node.name,
                    "chain": [{"name": n.name, "position": vec_bits(bits_to_f(b) for b in n.pos),
                               "rotation": vec_bits(bits_to_f(b) for b in n.rot),
                               "scale": vec_bits(bits_to_f(b) for b in n.scl)                               } for n in ch],
                    "partSpacePosition": vec_bits(part_space_position(ch)),
                    "partSpaceRotation": vec_bits(q),
                    "partSpaceForward": vec_bits(fwd),
                    "partSpaceForwardF": [float(x) for x in fwd],
                })
        parts[part_name] = {
            "thrustVectorTransformName": entry["thrustVectorTransformName"],
            "rescaleFactor": "%08x" % f_to_bits(rescale),
            "modelNodes": len(models),
            "muVersion": mf.version,
            "cfg": {"file": cfg_name, "origin": files[cfg_name]["origin"], "sha256": files[cfg_name]["sha256"]},
            "mu": {"file": mu_name, "origin": files[mu_name]["origin"], "sha256": files[mu_name]["sha256"]},
            "thrustTransforms": transforms,
        }
    doc = {
        "about": "WP-17b: thrust transforms of the stock engine models. Floats are IEEE binary32 bit patterns (8 hex digits, "
                 "'*F' arrays are the same values as decimal doubles for reading). Chain = transforms from the part's 'model' child "
                 "down to the thrust transform as PartLoader.CompileModel builds them. Produced by core/tools/wp17/mu_transforms.py.",
        "parts": parts,
    }
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        json.dump(doc, f, indent=1, sort_keys=True)
        f.write("\n")
    for k, v in parts.items():
        print(k, len(v["thrustTransforms"]), [t["partSpaceForwardF"] for t in v["thrustTransforms"]])


if __name__ == "__main__":
    sys.exit(main())
