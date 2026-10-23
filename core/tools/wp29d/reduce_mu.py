"""WP-29d: reduce a .mu model to what the physics needs, byte for byte.

Keeps, per transform: name + local TRS, TagAndLayer, every collider chunk (mesh / sphere / capsule / box, both layouts), the WheelCollider
chunk and the Animation chunk (PartReader.cs:219; the curves are the deployment poses of landing gear and legs). Drops every mesh filter,
renderer, material, texture, light, camera and particle emitter. The kept chunks are copied from the original bytes (nothing is
re-encoded), so the reduced file parses with PartReader's grammar and carries exactly the original values.

    python -I core/tools/wp29d/reduce_mu.py in.mu out.mu
"""
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "wp17"))
import mu_transforms as w17  # noqa: E402

KEEP_SIMPLE = {w17.TagAndLayer, w17.SphereCollider, w17.CapsuleCollider, w17.BoxCollider, w17.SphereCollider2, w17.CapsuleCollider2,
               w17.BoxCollider2, w17.WheelCollider}


class Reducer(w17.MuFile):
    """PartReader.ReadChild with every chunk consumed as in mu_transforms.MuFile; the raw bytes of the kept chunks are collected."""

    def read_child(self, parent):
        r = self.r
        start = r.p
        name = r.string()
        r.skip_floats(10)
        head = r.d[start:r.p]
        kept = []
        children = []
        while not r.eof():
            cs = r.p
            et = r.i32()
            if et == w17.ChildTransformStart:
                children.append(self.read_child(None))
            elif et == w17.Animation:
                self.read_animation()
                kept.append(r.d[cs:r.p])
            elif et == w17.MeshCollider:
                r.boolean()
                self.read_mesh()
                kept.append(r.d[cs:r.p])
            elif et == w17.MeshCollider2:
                r.boolean(); r.boolean()
                self.read_mesh()
                kept.append(r.d[cs:r.p])
            elif et in KEEP_SIMPLE:
                if et == w17.TagAndLayer:
                    r.string(); r.i32()
                elif et == w17.SphereCollider:
                    r.skip_floats(4)
                elif et == w17.CapsuleCollider:
                    r.f32(); r.i32(); r.skip_floats(3)
                elif et == w17.BoxCollider:
                    r.skip_floats(6)
                elif et == w17.SphereCollider2:
                    r.boolean(); r.skip_floats(4)
                elif et == w17.CapsuleCollider2:
                    r.boolean(); r.skip_floats(2); r.i32(); r.skip_floats(3)
                elif et == w17.BoxCollider2:
                    r.boolean(); r.skip_floats(6)
                elif et == w17.WheelCollider:
                    r.skip_floats(3 + 3 + 3 + 5 + 5)
                kept.append(r.d[cs:r.p])
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
            elif et == w17.Camera:
                self.read_camera()
            elif et == w17.ParticleEmitter:
                self.read_particles()
            elif et == w17.ChildTransformEnd:
                break
        return (head, kept, children)


def emit(node, root):
    head, kept, children = node
    out = bytearray(head)
    for k in kept:
        out += k
    for c in children:
        out += struct.pack("<i", w17.ChildTransformStart)
        out += emit(c, False)
    out += struct.pack("<i", w17.ChildTransformEnd)
    return bytes(out)


def reduce(data):
    mf = Reducer(data)
    r = mf.r
    if r.i32() != w17.FILE_TYPE_MODEL_BINARY:
        raise ValueError("incorrect file type")
    mf.version = r.i32()
    r.string()
    header = data[:r.p]
    node = mf.read_child(None)
    return header + emit(node, True)


def main():
    out = reduce(open(sys.argv[1], "rb").read())
    open(sys.argv[2], "wb").write(out)
    print("%s -> %s: %d bytes" % (sys.argv[1], sys.argv[2], len(out)))


if __name__ == "__main__":
    main()
