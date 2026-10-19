"""WP-29d: independent reader of the wheel-relevant .mu chunks (python, numpy float32), the reference of the C++ model reader.

Parses a .mu with PartReader's grammar (PartReader.cs:140 Read, :205 ReadChild; every chunk consumed by the WP-17b skippers of
mu_transforms.MuFile) and keeps, per transform: name, local position / rotation / scale (raw float32 bits), TagAndLayer, collider chunks
(kind, trigger, raw numbers), the WheelCollider chunk (:340, 19 floats) and the Animation chunk (:802 ReadAnimation: clips, curves,
keyframes). Used by dump_wheel_models.py, wheel_model_reference.py and the tests' JSON.
"""
import os
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "wp17"))
import mu_transforms as w17  # noqa: E402

F = np.float32


def bits(b):
    return struct.unpack("<f", struct.pack("<I", b))[0]


class Tnode:
    def __init__(self, name, pos, rot, scl, parent):
        self.name = name
        self.pos = [bits(x) for x in pos]
        self.rot = [bits(x) for x in rot]
        self.scl = [bits(x) for x in scl]
        self.parent = parent
        self.children = []
        self.tag = None
        self.layer = None
        self.colliders = []   # dicts
        self.wheel = None     # dict of the WheelCollider chunk
        self.anims = []       # Animation components on this node: list of clips


class WheelMu(w17.MuFile):
    def read_child(self, parent):
        r = self.r
        name = r.string()
        pos = [r.f32bits() for _ in range(3)]
        rot = [r.f32bits() for _ in range(4)]
        scl = [r.f32bits() for _ in range(3)]
        node = Tnode(name, pos, rot, scl, parent)
        while not r.eof():
            et = r.i32()
            if et == w17.ChildTransformStart:
                node.children.append(self.read_child(node))
            elif et == w17.Animation:
                node.anims.append(self.read_anim())
            elif et == w17.MeshCollider:
                r.boolean()
                node.colliders.append({"kind": "mesh", "trigger": False, "mesh": self.read_mesh_summary()})
            elif et == w17.SphereCollider:
                v = [r.f32() for _ in range(4)]
                node.colliders.append({"kind": "sphere", "trigger": False, "radius": v[0], "center": v[1:]})
            elif et == w17.CapsuleCollider:
                rad = r.f32(); d = r.i32(); c = [r.f32() for _ in range(3)]
                node.colliders.append({"kind": "capsule", "trigger": False, "radius": rad, "height": 2.0, "direction": d, "center": c})
            elif et == w17.BoxCollider:
                v = [r.f32() for _ in range(6)]
                node.colliders.append({"kind": "box", "trigger": False, "size": v[:3], "center": v[3:]})
            elif et == w17.MeshFilter:
                self.read_mesh()
            elif et == w17.MeshRenderer:
                self.read_mesh_renderer()
            elif et == w17.SkinnedMeshRenderer:
                self.read_skinned_mesh_renderer()
            elif et == w17.Materials:
                n = r.i32()
                for _ in range(n):
                    if self.version < 4:
                        self.read_material()
                    else:
                        self.read_material4()
            elif et == w17.Textures:
                self.read_textures()
            elif et == w17.Light:
                self.read_light()
            elif et == w17.TagAndLayer:
                node.tag = r.string()
                node.layer = r.i32()
            elif et == w17.MeshCollider2:
                trig = r.boolean(); r.boolean()
                node.colliders.append({"kind": "mesh", "trigger": trig, "mesh": self.read_mesh_summary()})
            elif et == w17.SphereCollider2:
                trig = r.boolean(); v = [r.f32() for _ in range(4)]
                node.colliders.append({"kind": "sphere", "trigger": trig, "radius": v[0], "center": v[1:]})
            elif et == w17.CapsuleCollider2:
                trig = r.boolean(); rad = r.f32(); h = r.f32(); d = r.i32(); c = [r.f32() for _ in range(3)]
                node.colliders.append({"kind": "capsule", "trigger": trig, "radius": rad, "height": h, "direction": d, "center": c})
            elif et == w17.BoxCollider2:
                trig = r.boolean(); v = [r.f32() for _ in range(6)]
                node.colliders.append({"kind": "box", "trigger": trig, "size": v[:3], "center": v[3:]})
            elif et == w17.WheelCollider:
                v = [r.f32() for _ in range(19)]  # PartReader.cs:340: mass, radius, suspensionDistance, center(3), spring(3), 2 x friction(5)
                node.wheel = {"mass": v[0], "radius": v[1], "suspensionDistance": v[2], "center": v[3:6],
                              "spring": v[6], "damper": v[7], "targetPosition": v[8],
                              "forwardFriction": v[9:14], "sidewaysFriction": v[14:19]}
            elif et == w17.Camera:
                self.read_camera()
            elif et == w17.ParticleEmitter:
                self.read_particles()
            elif et == w17.ChildTransformEnd:
                return node
        return node

    def read_mesh_summary(self):
        # the collider meshes are not needed for the wheel data: count only (the bytes are consumed by read_mesh)
        p = self.r.p
        self.read_mesh()
        return {"bytes": self.r.p - p}

    # PartReader.cs:802 ReadAnimation
    def read_anim(self):
        r = self.r
        clips = []
        for _ in range(r.i32()):
            name = r.string()
            bounds = [r.f32() for _ in range(6)]
            wrap = r.i32()
            curves = []
            for _ in range(r.i32()):
                path = r.string()
                prop = r.string()
                typ = r.i32()
                pre = r.i32()
                post = r.i32()
                keys = []
                for _ in range(r.i32()):
                    t, v, i, o = r.f32(), r.f32(), r.f32(), r.f32()
                    r.i32()
                    keys.append((t, v, i, o))
                curves.append({"path": path, "prop": prop, "type": typ, "pre": pre, "post": post, "keys": keys})
            clips.append({"name": name, "wrap": wrap, "curves": curves})
        default = r.string()
        auto = r.boolean()
        return {"clips": clips, "default": default, "playAutomatically": auto}


def read_wheel_mu(data):
    mf = WheelMu(data)
    r = mf.r
    if r.i32() != w17.FILE_TYPE_MODEL_BINARY:
        raise ValueError("incorrect file type")
    mf.version = r.i32()
    r.string()
    root = mf.read_child(None)
    return mf.version, root


def walk(node, path=()):
    yield node, path + (node.name,)
    for c in node.children:
        yield from walk(c, path + (node.name,))
