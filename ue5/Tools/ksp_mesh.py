"""Mesh -> (binary, JSON description) for the WP-UE-06 extractor.

Layout of `<name>.kmesh` (little endian, no header; every offset is in the JSON):
  channel arrays, one after the other, each `vertexCount * components` float32:
      position (3), normal (3), tangent (4), color (4), uv0 .. uv7 (2; only the channels the mesh has)
  indices: `indexCount` uint32, the submeshes' index ranges back to back (Unity stores them relative to the submesh's baseVertex; the
      values are written unchanged, `baseVertex` is in the JSON).
Vertex formats other than float32 are converted to float32 exactly: float16 widened, UNorm n -> v / (2^n - 1), SNorm n ->
max(v / (2^(n-1) - 1), -1) in float32 arithmetic (the Unity rule). The JSON lists the source format of every channel.
Geometry stays in Unity's frame (left handed, +Y up, UV origin bottom left); nothing is mirrored, flipped or rewound.
Bone weights / indices (skinned meshes) are not written; `skinned` says whether the mesh has them.
"""
import struct

from UnityPy.helpers.MeshHelper import MeshHandler

# Unity VertexFormat (2019): id -> (struct code, bytes, normalisation)
VFMT = {0: ("f", 4, None), 1: ("e", 2, None), 2: ("B", 1, ("u", 255)), 3: ("b", 1, ("s", 127)), 4: ("H", 2, ("u", 65535)),
        5: ("h", 2, ("s", 32767)), 6: ("B", 1, None), 7: ("b", 1, None), 8: ("H", 2, None), 9: ("h", 2, None),
        10: ("I", 4, None), 11: ("i", 4, None)}
VFMT_NAME = {0: "float32", 1: "float16", 2: "unorm8", 3: "snorm8", 4: "unorm16", 5: "snorm16", 6: "uint8", 7: "sint8", 8: "uint16",
             9: "sint16", 10: "uint32", 11: "sint32"}
# channel index (2019 VertexAttribute order) -> (JSON name, components written)
CHANNELS = {0: ("position", 3), 1: ("normal", 3), 2: ("tangent", 4), 3: ("color", 4), 4: ("uv0", 2), 5: ("uv1", 2), 6: ("uv2", 2),
            7: ("uv3", 2), 8: ("uv4", 2), 9: ("uv5", 2), 10: ("uv6", 2), 11: ("uv7", 2)}
ATTR = {0: "m_Vertices", 1: "m_Normals", 2: "m_Tangents", 3: "m_Colors", 4: "m_UV0", 5: "m_UV1", 6: "m_UV2", 7: "m_UV3",
        8: "m_UV4", 9: "m_UV5", 10: "m_UV6", 11: "m_UV7"}


def to_f32(values, fmt):
    """Flat list of python floats exactly representable as float32 for one channel's raw component values."""
    norm = VFMT[fmt][2]
    if norm is None:
        return [float(v) for v in values]
    kind, mx = norm
    # float32 arithmetic: round the divisor and quotient through float32 like the GPU conversion does
    f = struct.Struct("<f")
    d = f.unpack(f.pack(float(mx)))[0]
    out = []
    for v in values:
        q = f.unpack(f.pack(float(v) / d))[0]
        if kind == "s" and q < -1.0:
            q = -1.0
        out.append(q)
    return out


def export_mesh(reader):
    """Returns (kmesh bytes, JSON dict without file names, summary) for a UnityPy Mesh object reader."""
    mesh = reader.read()
    if mesh.m_MeshCompression != 0:
        raise ValueError("compressed mesh (m_MeshCompression=%d)" % mesh.m_MeshCompression)
    h = MeshHandler(mesh)
    h.process()
    vd = mesh.m_VertexData
    count = vd.m_VertexCount
    chans = list(vd.m_Channels)
    parts = []          # (name, components, flat float list, source format)
    for idx, (name, ncomp) in CHANNELS.items():
        ci = chans[idx] if idx < len(chans) else None
        if ci is None or (ci.dimension & 0xF) == 0:
            continue
        dim = ci.dimension & 0xF
        rows = getattr(h, ATTR[idx], None)
        if rows is None:
            raise ValueError("channel %s missing after decode" % name)
        if len(rows) != count:
            raise ValueError("channel %s has %d rows for %d vertices" % (name, len(rows), count))
        flat = []
        for r in rows:
            flat.extend(r)
        flat = to_f32(flat, ci.format)
        if dim != ncomp:
            # a channel with fewer components than the canonical width (a 3 component colour or tangent): pad with the Unity default
            # of the missing component (w = 1) so the arrays have a fixed stride; recorded in the JSON.
            padded = []
            for i in range(count):
                row = flat[i * dim:(i + 1) * dim]
                padded.extend(row + [1.0] * (ncomp - dim) if dim < ncomp else row[:ncomp])
            flat = padded
        parts.append((name, ncomp, dim, flat, VFMT_NAME[ci.format]))
    names = [p[0] for p in parts]
    if "position" not in names:
        raise ValueError("no position channel")
    blob = bytearray()
    arrays = {}
    for name, ncomp, dim, flat, srcfmt in parts:
        arrays[name] = {"offset": len(blob), "components": ncomp, "sourceComponents": dim, "sourceFormat": srcfmt}
        blob += struct.pack("<%df" % len(flat), *flat)
    ib = bytes(mesh.m_IndexBuffer)
    width = 2 if mesh.m_IndexFormat == 0 else 4
    subs = []
    idx_all = []
    for sm in mesh.m_SubMeshes:
        n = sm.indexCount
        start = sm.firstByte
        raw = struct.unpack("<%d%s" % (n, "H" if width == 2 else "I"), ib[start:start + n * width])
        subs.append({"firstIndex": len(idx_all), "indexCount": n, "topology": int(sm.topology), "baseVertex": int(sm.baseVertex),
                     "firstVertex": int(sm.firstVertex), "vertexCount": int(sm.vertexCount)})
        idx_all.extend(raw)
    idx_off = len(blob)
    blob += struct.pack("<%dI" % len(idx_all), *idx_all)
    la = mesh.m_LocalAABB
    desc = {"schema": "ksp-ue5-mesh/1", "name": mesh.m_Name, "vertexCount": count, "indexCount": len(idx_all),
            "sourceIndexWidth": width, "indexOffset": idx_off, "arrays": arrays, "submeshes": subs,
            "boundsCenter": [la.m_Center.x, la.m_Center.y, la.m_Center.z], "boundsExtent": [la.m_Extent.x, la.m_Extent.y, la.m_Extent.z],
            "skinned": bool(mesh.m_BindPose) if hasattr(mesh, "m_BindPose") else False,
            "isReadable": bool(mesh.m_IsReadable)}
    return bytes(blob), desc
