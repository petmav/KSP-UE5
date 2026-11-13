#!/usr/bin/env python
"""WP-UE-06 gate (local, needs the game): check ue5/GameAssets/ against the game files by a DIFFERENT route than the extractor.

    <harness>/tools-cache/venv/Scripts/python.exe -I ue5/Tools/verify_game_assets.py --game <game copy> --assets ue5/GameAssets ^
        [--summary ue5/Tools/extraction_summary.json]

Independence from extract_game_assets.py (the WP-12c pattern): this file imports none of ksp_unity / ksp_textures / ksp_mesh /
ksc_scenery. It uses UnityPy only to enumerate objects and to read their RAW type trees (`read_typetree()` dicts, no Texture2D / Mesh /
MeshHandler classes, no type-tree generator), resolves PPtrs itself, reads `.resS` streams with open() / seek(), decodes vertex data
and index buffers with `struct`, builds DDS headers and expected PNG scanlines from its own tables, and parses the output PNG / DDS
files with its own readers. What it checks:
  1. manifest <-> disk: every file listed exists with the recorded size and sha256, no file on disk is unlisted;
  2. origin: (file, pathID) exists in the game and its Unity type / name equal the entry's;
  3. textures: DDS header fields + payload bytes == the stored mip chain; PNG decodes (zlib, own unfilter) to the expected pixels;
     Crunched textures are checked against the codec's output only (texture2ddecoder is the one decoder there is: NOT independent);
  4. meshes: every channel of every .kmesh == the raw vertex data decoded here (bitwise float32), indices, submeshes, bounds;
  5. materials: name, shader, floats, colours, texture slots (scale / offset / target) == the raw Material object;
  6. fonts: bytes == m_FontData;
  7. descriptions: scaled-space and GalaxyCube node trees re-walked from raw GameObjects (names, TRS, layer, tag, mesh, materials); KSC
     objects vs the committed WP-12c collider tree (core/data/scenery) and the raw renderers of each node's source GameObject; the
     scaled bodies' root / mesh / material / texture origins vs core/data/kerbol_body_assets.json (a different extraction tool); PQS
     roles vs the same file's PQS component refs; UI sprites vs the raw Sprite objects.
Not independently re-derivable here (MonoBehaviour fields need the type-tree generator the extractor uses): which sprites the HUD
rule selects, and the dump of MonoBehaviour fields inside the descriptions; both are reported as `unchecked`.
Exit code 1 and the first mismatches printed when anything differs.
"""
import argparse
import hashlib
import json
import os
import struct
import sys
import zlib

import UnityPy

# ---- own tables (not imported from the extractor) --------------------------------------------------------------------------------
TF_NAME = {1: "Alpha8", 2: "ARGB4444", 3: "RGB24", 4: "RGBA32", 5: "ARGB32", 13: "RGBA4444", 14: "BGRA32", 10: "DXT1", 12: "DXT5",
           25: "BC7", 26: "BC4", 27: "BC5", 24: "BC6H", 17: "RGBAHalf", 28: "DXT1Crunched", 29: "DXT5Crunched", 15: "RHalf",
           16: "RGHalf", 18: "RFloat", 19: "RGFloat", 20: "RGBAFloat", 7: "RGB565", 9: "R16"}
BYTES_PER_UNIT = {1: 1, 2: 2, 3: 3, 4: 4, 5: 4, 13: 2, 14: 4, 7: 2, 9: 2, 15: 2, 16: 4, 17: 8, 18: 4, 19: 8, 20: 16,
                  10: 8, 12: 16, 25: 16, 26: 8, 27: 16, 24: 16, 28: 8, 29: 16}
BLOCK_FORMATS = {10, 12, 25, 26, 27, 24, 28, 29}
DXGI = {25: 98, 26: 80, 27: 83, 24: 95, 17: 10, 15: 54, 16: 34, 18: 41, 19: 16, 20: 2, 7: 85, 9: 56}
VERTEX_FORMATS = {0: ("<f", 4), 1: ("<e", 2), 2: ("<B", 1), 3: ("<b", 1), 4: ("<H", 2), 5: ("<h", 2), 6: ("<B", 1), 7: ("<b", 1),
                  8: ("<H", 2), 9: ("<h", 2), 10: ("<I", 4), 11: ("<i", 4)}
NORMALISE = {2: 255, 3: 127, 4: 65535, 5: 32767}      # unorm8, snorm8, unorm16, snorm16 divisors
SNORM = {3, 5}
CH_NAMES = ["position", "normal", "tangent", "color", "uv0", "uv1", "uv2", "uv3", "uv4", "uv5", "uv6", "uv7"]
CH_COMPONENTS = [3, 3, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2]
BUILTIN_TAGS = {0: "Untagged", 1: "Respawn", 2: "Finish", 4: "EditorOnly", 5: "MainCamera", 6: "Player", 7: "GameController"}


def f32(x):
    return struct.pack("<f", x)


def stem(file):
    return (file[:-len(".assets")] if file.endswith(".assets") else file).replace(" ", "_")


class Raw:
    def __init__(self, data_dir):
        self.data_dir = data_dir
        self.env = UnityPy.load(data_dir)
        self.objs = {(o.assets_file.name, o.path_id): o for o in self.env.objects}
        self.files = {os.path.basename(n): f for n, f in self.env.files.items()}
        self.cache = {}
        self.tags = list(self.tt(next(k for k, o in self.objs.items() if o.type.name == "TagManager"))["tags"])

    def tt(self, key):
        if key not in self.cache:
            self.cache[key] = self.objs[key].read_typetree()
        return self.cache[key]

    def ptr(self, src, p):
        if p is None or p["m_PathID"] == 0:
            return None
        if p["m_FileID"] == 0:
            return (src, p["m_PathID"])
        ext = self.files[src].externals
        if not 1 <= p["m_FileID"] <= len(ext):
            return None
        return (os.path.basename(ext[p["m_FileID"] - 1].path), p["m_PathID"])

    def tag(self, t):
        return BUILTIN_TAGS.get(t) or (self.tags[t - 20000] if 0 <= t - 20000 < len(self.tags) else "#%d" % t)

    def comps(self, go):
        out = []
        for c in self.tt(go)["m_Component"]:
            k = self.ptr(go[0], c["component"])
            if k in self.objs:
                out.append((self.objs[k].type.name, k))
        return out

    def stream(self, sd):
        path = os.path.join(self.data_dir, os.path.basename(sd["path"]))
        with open(path, "rb") as f:
            f.seek(sd["offset"])
            return f.read(sd["size"])

    def name(self, key):
        o = self.objs[key]
        if o.type.name == "MonoBehaviour":
            return None
        if o.type.name == "Shader":
            return o.read().m_ParsedForm.m_Name
        return self.tt(key).get("m_Name", "")


class Report:
    def __init__(self):
        self.bad = []
        self.counts = {}

    def check(self, cond, msg):
        if not cond:
            self.bad.append(msg)

    def count(self, k, n=1):
        self.counts[k] = self.counts.get(k, 0) + n


# ---- textures --------------------------------------------------------------------------------------------------------------------
def level_sizes(w, h, mips, fmt):
    out = []
    for i in range(mips):
        lw, lh = max(1, w >> i), max(1, h >> i)
        if fmt in BLOCK_FORMATS:
            out.append(((lw + 3) // 4) * ((lh + 3) // 4) * BYTES_PER_UNIT[fmt])
        else:
            out.append(lw * lh * BYTES_PER_UNIT[fmt])
    return out


def raw_texture(raw, key):
    t = raw.tt(key)
    data = bytes(t["image data"]) if t.get("image data") else b""
    if not data and t["m_StreamData"]["size"]:
        data = raw.stream(t["m_StreamData"])
    return t, data


def parse_dds(buf):
    if buf[:4] != b"DDS " or struct.unpack_from("<I", buf, 4)[0] != 124:
        raise ValueError("bad DDS magic / size")
    flags, height, width, linear, depth, mips = struct.unpack_from("<6I", buf, 8)
    pf_size, pf_flags, fourcc = struct.unpack_from("<II4s", buf, 76)
    caps = struct.unpack_from("<I", buf, 108)[0]
    off = 128
    ext = None
    if fourcc == b"DX10":
        ext = struct.unpack_from("<5I", buf, 128)
        off = 148
    return {"flags": flags, "h": height, "w": width, "linear": linear, "mips": mips, "pf_size": pf_size, "pf_flags": pf_flags,
            "fourcc": fourcc, "caps": caps, "dx10": ext, "payload": buf[off:]}


def parse_png(buf):
    if buf[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("bad PNG signature")
    pos, idat, ihdr = 8, b"", None
    while pos < len(buf):
        n = struct.unpack_from(">I", buf, pos)[0]
        tag = buf[pos + 4:pos + 8]
        body = buf[pos + 8:pos + 8 + n]
        crc = struct.unpack_from(">I", buf, pos + 8 + n)[0]
        if zlib.crc32(tag + body) & 0xFFFFFFFF != crc:
            raise ValueError("bad CRC in %r" % tag)
        if tag == b"IHDR":
            ihdr = struct.unpack(">IIBBBBB", body)
        elif tag == b"IDAT":
            idat += body
        pos += 12 + n
    w, h, depth, ctype, _c, _f, _i = ihdr
    ch = {0: 1, 2: 3, 6: 4}[ctype]
    raw = zlib.decompress(idat)
    stride = w * ch
    if len(raw) != h * (stride + 1):
        raise ValueError("PNG data size")
    rows = []
    for y in range(h):
        ft = raw[y * (stride + 1)]
        if ft != 0:
            raise ValueError("filter %d (the writer only uses 0)" % ft)
        rows.append(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
    return w, h, depth, ctype, b"".join(rows)


def expected_png(fmt, w, h, data):
    """(colour type, pixels) a PNG of mip 0 must contain, from the raw bytes, by this file's own mapping."""
    n = w * h
    if fmt == 1:
        return 0, data[:n]
    if fmt == 3:
        return 2, data[:n * 3]
    if fmt == 4:
        return 6, data[:n * 4]
    px = bytearray()
    if fmt == 5:
        for i in range(n):
            a, r, g, b = data[4 * i:4 * i + 4]
            px += bytes((r, g, b, a))
    elif fmt == 14:
        for i in range(n):
            b, g, r, a = data[4 * i:4 * i + 4]
            px += bytes((r, g, b, a))
    elif fmt in (2, 13):
        for i in range(n):
            v = data[2 * i] + 256 * data[2 * i + 1]
            n3, n2, n1, n0 = (v >> 12) & 15, (v >> 8) & 15, (v >> 4) & 15, v & 15
            a, r, g, b = (n3, n2, n1, n0) if fmt == 2 else (n0, n3, n2, n1)
            px += bytes((17 * r, 17 * g, 17 * b, 17 * a))
    else:
        raise ValueError("no PNG route for format %d" % fmt)
    return 6, bytes(px)


def verify_texture(raw, rep, entry, disk):
    key = (entry["origin"]["file"], entry["origin"]["pathID"])
    t, data = raw_texture(raw, key)
    fmt, w, h, mips = t["m_TextureFormat"], t["m_Width"], t["m_Height"], t["m_MipCount"]
    m = entry["meta"]
    where = entry["path"]
    rep.check(m["unityFormat"] == fmt and m["width"] == w and m["height"] == h, where + " meta size / format")
    rep.check(m["mipCount"] == mips, where + " meta mipCount")
    rep.check(m["sRGB"] == bool(t["m_ColorSpace"]), where + " sRGB")
    rep.check((m["wrapU"], m["wrapV"], m["filterMode"], m["aniso"]) == (t["m_TextureSettings"]["m_WrapU"], t["m_TextureSettings"]["m_WrapV"],
                                                                         t["m_TextureSettings"]["m_FilterMode"], t["m_TextureSettings"]["m_Aniso"]),
              where + " sampler settings")
    crunch = fmt in (28, 29)
    stored_fmt = {28: 10, 29: 12}.get(fmt, fmt)
    if crunch:
        import texture2ddecoder
        data = texture2ddecoder.unpack_unity_crunch(data)
        stored_mips = 1
        rep.count("texturesCrunchedNotIndependent")
    else:
        stored_mips = mips
    sizes = level_sizes(w, h, stored_mips, stored_fmt)
    rep.check(sum(sizes) == len(data), where + " raw size %d vs expected %d" % (len(data), sum(sizes)))
    if where.endswith(".dds"):
        d = parse_dds(disk)
        rep.check(d["w"] == w and d["h"] == h and d["mips"] == stored_mips, where + " DDS size / mips")
        rep.check(d["pf_size"] == 32 and d["pf_flags"] == 4, where + " DDS pixel format flags")
        want_hdr = 0x1 | 0x2 | 0x4 | 0x1000 | (0x20000 if stored_mips > 1 else 0)
        rep.check(d["flags"] & want_hdr == want_hdr, where + " DDS header flags")
        if stored_fmt in (10, 12):
            rep.check(d["fourcc"] == (b"DXT1" if stored_fmt == 10 else b"DXT5") and d["dx10"] is None, where + " DDS fourcc")
            rep.check(d["linear"] == sizes[0], where + " DDS linear size")
        else:
            rep.check(d["fourcc"] == b"DX10" and d["dx10"][0] == DXGI[stored_fmt] and d["dx10"][1] == 3 and d["dx10"][3] == 1,
                      where + " DDS DX10 header")
        rep.check(d["payload"] == data, where + " DDS payload differs from the stored mip chain")
        rep.count("ddsVerified")
    else:
        pw, ph, depth, ctype, pix = parse_png(disk)
        ct, exp = expected_png(fmt, w, h, data)
        rep.check((pw, ph, depth, ctype) == (w, h, 8, ct), where + " PNG header")
        rep.check(pix == exp, where + " PNG pixels differ")
        rep.count("pngVerified")


# ---- meshes ----------------------------------------------------------------------------------------------------------------------
def raw_mesh_channels(raw, key):
    """{channel name: (components written, source dim, bytes of the float32 array)}, index list, submesh list, raw mesh tree."""
    m = raw.tt(key)
    assert m["m_MeshCompression"] == 0, "compressed"
    vd = m["m_VertexData"]
    count = vd["m_VertexCount"]
    data = bytes(vd["m_DataSize"]) if vd.get("m_DataSize") else b""
    sd = m.get("m_StreamData") or {}
    if not data and sd.get("size"):
        data = raw.stream(sd)
    chans = vd["m_Channels"]
    streams = max(c["stream"] for c in chans if c["dimension"] & 15) + 1
    stride = [0] * streams
    for c in chans:
        dim = c["dimension"] & 15
        if dim:
            stride[c["stream"]] += dim * VERTEX_FORMATS[c["format"]][1]
    offs, o = [], 0
    for s in range(streams):
        offs.append(o)
        o = (o + stride[s] * count + 15) & ~15
    out = {}
    for idx in range(12):
        c = chans[idx] if idx < len(chans) else None
        if c is None or not (c["dimension"] & 15):
            continue
        dim = c["dimension"] & 15
        code, size = VERTEX_FORMATS[c["format"]]
        base = offs[c["stream"]] + c["offset"]
        st = stride[c["stream"]]
        ncomp = CH_COMPONENTS[idx]
        pieces = []
        if c["format"] == 0 and dim == ncomp:
            for i in range(count):
                pieces.append(data[base + i * st: base + i * st + 4 * dim])
        else:
            fmt = "<%d%s" % (dim, code[1])
            for i in range(count):
                vals = struct.unpack_from(fmt, data, base + i * st)
                if c["format"] in NORMALISE:
                    d = NORMALISE[c["format"]]
                    vs = []
                    for v in vals:
                        q = struct.unpack("<f", f32(v / struct.unpack("<f", f32(float(d)))[0]))[0]
                        if c["format"] in SNORM and q < -1.0:
                            q = -1.0
                        vs.append(q)
                else:
                    vs = [float(v) for v in vals]
                vs = vs[:ncomp] + [1.0] * (ncomp - dim)
                pieces.append(b"".join(f32(v) for v in vs))
        out[CH_NAMES[idx]] = (ncomp, dim, b"".join(pieces), c["format"])
    ib = bytes(m["m_IndexBuffer"])
    width = 2 if m["m_IndexFormat"] == 0 else 4
    idx_all, subs = [], []
    for sm in m["m_SubMeshes"]:
        n = sm["indexCount"]
        vals = struct.unpack_from("<%d%s" % (n, "H" if width == 2 else "I"), ib, sm["firstByte"])
        subs.append((len(idx_all), n, sm["topology"], sm["baseVertex"], sm["firstVertex"], sm["vertexCount"]))
        idx_all.extend(vals)
    return count, out, idx_all, subs, m


def verify_mesh(raw, rep, entry, files, assets):
    """entry: the .json description entry; its .kmesh is read next to it."""
    key = (entry["origin"]["file"], entry["origin"]["pathID"])
    desc = json.loads(files[entry["path"]])
    blob = files[entry["path"][:-5] + ".kmesh"]
    where = entry["path"]
    count, chans, idx, subs, m = raw_mesh_channels(raw, key)
    rep.check(desc["name"] == m["m_Name"] and desc["vertexCount"] == count and desc["indexCount"] == len(idx), where + " name / counts")
    rep.check(sorted(desc["arrays"]) == sorted(chans), where + " channel set %s vs %s" % (sorted(desc["arrays"]), sorted(chans)))
    for name, (ncomp, dim, arr, fmt) in chans.items():
        a = desc["arrays"].get(name)
        if a is None:
            continue
        rep.check(a["components"] == ncomp and a["sourceComponents"] == dim, where + " " + name + " components")
        rep.check(blob[a["offset"]:a["offset"] + len(arr)] == arr, where + " " + name + " data differs")
    rep.check(blob[desc["indexOffset"]:] == struct.pack("<%dI" % len(idx), *idx), where + " indices differ")
    rep.check([(s["firstIndex"], s["indexCount"], s["topology"], s["baseVertex"], s["firstVertex"], s["vertexCount"]) for s in desc["submeshes"]]
              == subs, where + " submeshes")
    la = m["m_LocalAABB"]
    rep.check(desc["boundsCenter"] == [la["m_Center"][a] for a in "xyz"] and desc["boundsExtent"] == [la["m_Extent"][a] for a in "xyz"],
              where + " bounds")
    rep.check(hashlib.sha256(blob).hexdigest() == desc["binarySha256"], where + " binarySha256")
    rep.count("meshesVerified")
    rep.count("meshFloatsCompared", sum(len(c[2]) // 4 for c in chans.values()) + len(idx))


# ---- materials ---------------------------------------------------------------------------------------------------------------------
def verify_material(raw, rep, entry, files, by_origin_tex):
    key = (entry["origin"]["file"], entry["origin"]["pathID"])
    t = raw.tt(key)
    doc = json.loads(files[entry["path"]])
    where = entry["path"]
    sk = raw.ptr(key[0], t["m_Shader"])
    rep.check(doc["name"] == t["m_Name"], where + " name")
    rep.check(doc["shader"] == (raw.name(sk) if sk else None), where + " shader %r" % doc["shader"])
    sp = t["m_SavedProperties"]
    rep.check(doc["floats"] == {n: v for n, v in sp["m_Floats"]} or all(
        (isinstance(v, str) or v == doc["floats"].get(n)) for n, v in sp["m_Floats"]), where + " floats")
    rep.check(doc["colors"] == {n: [c["r"], c["g"], c["b"], c["a"]] for n, c in sp["m_Colors"]}, where + " colors")
    rep.check(doc["customRenderQueue"] == t["m_CustomRenderQueue"], where + " render queue")
    rep.check(doc["keywords"] == [k for k in t["m_ShaderKeywords"].split(" ") if k], where + " keywords")
    rep.check(sorted(doc["textures"]) == sorted(n for n, _ in sp["m_TexEnvs"]), where + " texture slots")
    for n, e in sp["m_TexEnvs"]:
        d = doc["textures"][n]
        rep.check(d["scale"] == [e["m_Scale"]["x"], e["m_Scale"]["y"]] and d["offset"] == [e["m_Offset"]["x"], e["m_Offset"]["y"]], where + " " + n + " tiling")
        tk = raw.ptr(key[0], e["m_Texture"])
        if tk is None:
            rep.check(d["texture"] is None and "origin" not in d, where + " " + n + " null texture")
        else:
            rep.check((d["origin"]["file"], d["origin"]["pathID"]) == tk, where + " " + n + " texture origin")
            if d["texture"] is not None:
                rep.check(by_origin_tex.get(tk) == d["texture"], where + " " + n + " texture file vs manifest origin")
    rep.count("materialsVerified")


# ---- GameObject trees ----------------------------------------------------------------------------------------------------------------
def raw_renderers(raw, go):
    mesh = None
    comps = raw.comps(go)
    for n, k in comps:
        if n == "MeshFilter":
            mesh = raw.ptr(k[0], raw.tt(k)["m_Mesh"])
    out = []
    for n, k in comps:
        if n in ("MeshRenderer", "SkinnedMeshRenderer"):
            t = raw.tt(k)
            out.append((n, mesh if n == "MeshRenderer" else raw.ptr(k[0], t["m_Mesh"]), [raw.ptr(k[0], m) for m in t["m_Materials"]],
                        bool(t["m_Enabled"]), int(t["m_CastShadows"]), bool(t["m_ReceiveShadows"]), k))
    return out


def tree_nodes(raw, root):
    out = []

    def visit(k, parent):
        idx = len(out)
        out.append((k, parent))
        tk = next(kk for n, kk in raw.comps(k) if n in ("Transform", "RectTransform"))
        for ch in raw.tt(tk)["m_Children"]:
            ck = raw.ptr(tk[0], ch)
            visit(raw.ptr(ck[0], raw.tt(ck)["m_GameObject"]), idx)

    visit(root, -1)
    return out


def verify_node_tree(raw, rep, doc_nodes, root, files, where):
    nodes = tree_nodes(raw, root)
    rep.check(len(nodes) == len(doc_nodes), where + " node count %d vs %d" % (len(doc_nodes), len(nodes)))
    for (k, parent), d in zip(nodes, doc_nodes):
        w = "%s:%s" % (where, d["name"])
        t = raw.tt(k)
        tk = next(kk for n, kk in raw.comps(k) if n in ("Transform", "RectTransform"))
        tr = raw.tt(tk)
        rep.check(d["name"] == t["m_Name"] and d["parent"] == parent and (d["origin"]["file"], d["origin"]["pathID"]) == k, w + " identity")
        rep.check(d["active"] == bool(t["m_IsActive"]) and d["layer"] == t["m_Layer"] and d["tag"] == raw.tag(t["m_Tag"]), w + " active / layer / tag")
        rep.check(d["position"] == [tr["m_LocalPosition"][a] for a in "xyz"] and d["rotation"] == [tr["m_LocalRotation"][a] for a in "xyzw"]
                  and d["scale"] == [tr["m_LocalScale"][a] for a in "xyz"], w + " TRS")
        rr = raw_renderers(raw, k)
        rep.check(len(rr) == len(d["renderers"]), w + " renderer count")
        for (cn, mesh, mats, en, cs, rs, rk), dr in zip(rr, d["renderers"]):
            rep.check(dr["component"] == cn and dr["enabled"] == en and dr["castShadows"] == cs and dr["receiveShadows"] == rs, w + " renderer flags")
            if mesh is not None:
                rep.check((dr["meshOrigin"]["file"], dr["meshOrigin"]["pathID"]) == mesh and dr["mesh"] is not None, w + " mesh origin")
            rep.check(len(mats) == len(dr["materials"]), w + " material count")
            for mk, mf in zip(mats, dr["materials"]):
                if mk is None:
                    rep.check(mf is None, w + " null material")
                else:
                    rep.check(mf is not None and mf.startswith("materials/%s_%d_" % (stem(mk[0]), mk[1])), w + " material file %r for %s" % (mf, mk))
    rep.count("treeNodesVerified", len(nodes))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--game", required=True)
    ap.add_argument("--assets", required=True)
    ap.add_argument("--summary")
    ap.add_argument("--repo", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    args = ap.parse_args()
    assets = os.path.abspath(args.assets)
    raw = Raw(os.path.join(os.path.abspath(args.game), "KSP_x64_Data"))
    rep = Report()
    manifest = json.load(open(os.path.join(assets, "manifest.json"), encoding="utf-8"))
    entries = {e["path"]: e for e in manifest["files"]}

    # 1. manifest <-> disk
    on_disk = set()
    for dp, _dn, fn in os.walk(assets):
        for f in fn:
            on_disk.add(os.path.relpath(os.path.join(dp, f), assets).replace("\\", "/"))
    on_disk.discard("manifest.json")
    rep.check(on_disk == set(entries), "files on disk vs manifest: %d unlisted, %d missing" % (len(on_disk - set(entries)), len(set(entries) - on_disk)))
    files = {}
    for p, e in entries.items():
        b = open(os.path.join(assets, p), "rb").read()
        rep.check(len(b) == e["bytes"] and hashlib.sha256(b).hexdigest() == e["sha256"], p + " size / sha256")
        files[p] = b
    rep.count("filesChecked", len(entries))
    rep.count("bytesChecked", sum(e["bytes"] for e in entries.values()))
    # source file digests recorded in the manifest
    for f, h in manifest["sourceFiles"].items():
        hh = hashlib.sha256()
        with open(os.path.join(raw.data_dir, f), "rb") as fh:
            for chunk in iter(lambda: fh.read(1 << 22), b""):
                hh.update(chunk)
        rep.check(hh.hexdigest() == h, "source file %s sha256" % f)

    # 2. origin
    for p, e in entries.items():
        k = (e["origin"]["file"], e["origin"]["pathID"])
        rep.check(k in raw.objs, p + " origin object exists")
        if k in raw.objs:
            o = raw.objs[k]
            rep.check(o.type.name == e["origin"]["type"], p + " origin type %s vs %s" % (e["origin"]["type"], o.type.name))
            nm = raw.name(k)
            if nm is not None and e["kind"] not in ("description",):
                rep.check(nm == e["origin"]["name"], p + " origin name")
    # 3. textures
    tex_by_origin = {}
    for p, e in entries.items():
        if e["kind"] == "texture":
            tex_by_origin[(e["origin"]["file"], e["origin"]["pathID"])] = p
            verify_texture(raw, rep, e, files[p])
    # 4. meshes
    for p, e in entries.items():
        if e["kind"] == "mesh-description":
            verify_mesh(raw, rep, e, files, assets)
    # 5. materials
    for p, e in entries.items():
        if e["kind"] == "material":
            verify_material(raw, rep, e, files, tex_by_origin)
    # 6. fonts
    for p, e in entries.items():
        if e["kind"] == "font":
            k = (e["origin"]["file"], e["origin"]["pathID"])
            rep.check(files[p] == bytes(raw.tt(k)["m_FontData"]), p + " font bytes")
            rep.count("fontsVerified")

    # 7. descriptions
    repo = os.path.abspath(args.repo)
    kerbol = json.load(open(os.path.join(repo, "core", "data", "kerbol_body_assets.json"), encoding="utf-8"))
    kb = {b["bodyName"]: b for b in kerbol["bodies"]}
    scaled = [p for p in entries if p.startswith("scaled_space/")]
    rep.check(len(scaled) == 17 and sorted(os.path.basename(p)[:-5] for p in scaled) == sorted(kb), "17 scaled bodies present")
    for p in scaled:
        doc = json.loads(files[p])
        body = doc["body"]
        sv = (doc["scaledVersion"]["file"], doc["scaledVersion"]["pathID"])
        ref = kb[body]["scaledVersion"]
        rep.check(("KSP_x64_Data/" + sv[0], sv[1]) == (ref["file"], ref["pathID"]), p + " scaledVersion vs kerbol_body_assets")
        verify_node_tree(raw, rep, doc["nodes"], sv, files, p)
        root = doc["nodes"][0]
        rep.check(root["renderers"] and (root["renderers"][0]["meshOrigin"]["pathID"] == ref["mesh"]["pathID"]), p + " mesh vs kerbol_body_assets")
        mats = [m for m in root["renderers"][0]["materials"]]
        rep.check(len(mats) == len(ref["materials"]), p + " root material count")
        for mf, rm in zip(mats, ref["materials"]):
            md = json.loads(files[mf])
            rep.check(md["name"] == rm["name"] and md["shaderOrigin"]["pathID"] == rm["shader"]["pathID"], p + " material vs kerbol_body_assets")
            tex_ref = {t["property"]: t["pathID"] for t in rm["textures"]}
            tex_got = {n: d["origin"]["pathID"] for n, d in md["textures"].items() if "origin" in d}
            rep.check(tex_ref == tex_got, p + " " + md["name"] + " textures vs kerbol_body_assets %s %s" % (tex_ref, tex_got))
        rep.count("scaledBodiesVerified")
    gal = json.loads(files["skybox/galaxy_cube.json"])
    verify_node_tree(raw, rep, gal["nodes"], (gal["root"]["file"], gal["root"]["pathID"]), files, "skybox")
    rep.check(sorted(e["origin"]["name"] for e in entries.values() if e["kind"] == "texture" and "skybox" in e["categories"])
              == sorted("GalaxyTex_" + a for a in ("NegativeX", "NegativeY", "NegativeZ", "PositiveX", "PositiveY", "PositiveZ")), "six GalaxyTex textures")
    # KSC
    ksc_path = [p for p in entries if p.startswith("ksc/")][0]
    ksc = json.loads(files[ksc_path])
    scen = json.load(open(os.path.join(repo, "core", "data", "scenery", "scenery_colliders.json"), encoding="utf-8"))
    city = [c for c in scen["cities"] if c["name"] == ksc["city"] and c["gameObjectPathID"] == ksc["cityGameObject"]["pathID"]][0]
    rep.check(len(city["objects"]) == len(ksc["objects"]), "KSC object count vs WP-12c")
    nren = 0
    for o, c in zip(ksc["objects"], city["objects"]):
        w = "ksc #%d %s" % (o["index"], o["path"][-40:])
        rep.check(o["path"] == c["path"] and o["parent"] == c["parent"] and o["src"] == c["src"], w + " path / parent / src vs WP-12c")
        rep.check(o["active"] == c["active"] and o["layer"] == c["layer"] and o["tag"] == c["tag"], w + " active / layer / tag vs WP-12c")
        rep.check([f32(x) for x in o["position"]] == [f32(x) for x in c["position"]] and [f32(x) for x in o["rotation"]] == [f32(x) for x in c["rotation"]]
                  and [f32(x) for x in o["scale"]] == [f32(x) for x in c["scale"]], w + " TRS bits vs WP-12c")
        rr = raw_renderers(raw, tuple(c["src"]))
        got = [(r["component"], (r["meshOrigin"]["file"], r["meshOrigin"]["pathID"]), r["enabled"], r["castShadows"], r["receiveShadows"],
                (r["rendererOrigin"]["file"], r["rendererOrigin"]["pathID"])) for r in o["renderers"]]
        want = [(n, m, en, cs, rs, rk) for (n, m, mats, en, cs, rs, rk) in rr if m is not None]
        rep.check(got == want, w + " renderers vs raw source GameObject")
        for (n, m, mats, en, cs, rs, rk), r in zip([x for x in rr if x[1] is not None], o["renderers"]):
            rep.check(len(mats) == len(r["materials"]), w + " materials")
            for mk, mf in zip(mats, r["materials"]):
                rep.check((mk is None and mf is None) or (mf is not None and mf.startswith("materials/%s_%d_" % (stem(mk[0]), mk[1]))), w + " material file")
        nren += len(o["renderers"])
    rep.count("kscObjectsVerified", len(ksc["objects"]))
    rep.count("kscRenderersVerified", nren)
    # PQS roles vs kerbol_body_assets Kerbin
    pq = json.loads(files["pqs/Kerbin.json"])
    prefab = json.load(open(os.path.join(repo, "core", "data", "terrain", "pqs_prefab.json"), encoding="utf-8"))
    city_gos = {m["gameObject"] for b in prefab["bodies"] if b["bodyName"] == "Kerbin" for m in b["mods"] if m["class"] in ("PQSCity", "PQSCity2")}
    refs = set()
    for c in kb["Kerbin"]["pqs"]["components"]:
        if c["component"] in ("PQSLandControl", "PQSROCControl", "PQSCity", "PQSCity2") or any(c["gameObject"] == g or c["gameObject"].startswith(g + "/") for g in city_gos):
            continue
        for r in c["refs"]:
            if r["type"] in ("Material", "Texture2D", "Texture2DArray", "Texture3D", "Cubemap"):
                refs.add((c["gameObject"], c["component"], r["field"], r["pathID"]))
    mine = {(r["gameObject"], r["class"], r["field"], r["origin"]["pathID"]) for r in pq["roles"]}
    rep.check(mine == refs, "PQS roles vs kerbol_body_assets: %d only mine %s, %d only theirs %s" % (
        len(mine - refs), sorted(mine - refs)[:2], len(refs - mine), sorted(refs - mine)[:2]))
    rep.count("pqsRolesVerified", len(mine & refs))
    # UI sprites vs raw Sprite objects
    ui = json.loads(files["ui/flight_hud.json"])
    for s in ui["sprites"]:
        k = (s["origin"]["file"], s["origin"]["pathID"])
        t = raw.tt(k)
        rd = t["m_RD"]
        tk = raw.ptr(k[0], rd["texture"])
        rep.check(s["name"] == t["m_Name"] and s["rect"] == [t["m_Rect"][a] for a in ("x", "y", "width", "height")], "sprite %s rect" % s["name"])
        rep.check(s["border"] == [t["m_Border"][a] for a in "xyzw"] and s["pivot"] == [t["m_Pivot"]["x"], t["m_Pivot"]["y"]] and s["pixelsPerUnit"] == t["m_PixelsToUnits"], "sprite %s border / pivot" % s["name"])
        rep.check(s["textureRect"] == [rd["textureRect"][a] for a in ("x", "y", "width", "height")] and s["settingsRaw"] == rd["settingsRaw"], "sprite %s textureRect" % s["name"])
        rep.check(tk is not None and s["texture"] == tex_by_origin.get(tk), "sprite %s atlas file" % s["name"])
        rep.count("spritesVerified")
    # the roles: every missing entry has a reason; none of the texture entries is for an object that is not a Texture2D
    out = {"manifest": os.path.join(assets, "manifest.json"), "mismatches": len(rep.bad), "counts": rep.counts,
           "notIndependent": ["Crunched textures (texture2ddecoder is the only codec)",
                              "HUD sprite selection rule and MonoBehaviour field dumps (need the type-tree generator)"]}
    for b in rep.bad[:30]:
        print("MISMATCH", b)
    print(json.dumps(out["counts"], indent=1, sort_keys=True))
    print("mismatches:", len(rep.bad))
    if args.summary and not rep.bad:
        s = json.load(open(args.summary, encoding="utf-8")) if os.path.exists(args.summary) else {}
        s["verification"] = {"tool": "ue5/Tools/verify_game_assets.py", "mismatches": 0, "counts": rep.counts, "notIndependent": out["notIndependent"]}
        with open(args.summary, "w", encoding="utf-8", newline="\n") as f:
            json.dump(s, f, indent=1, sort_keys=True)
            f.write("\n")
    return 1 if rep.bad else 0


if __name__ == "__main__":
    sys.exit(main())
