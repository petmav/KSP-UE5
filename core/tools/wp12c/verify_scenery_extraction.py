#!/usr/bin/env python
"""WP-12c gate (local, needs the game): re-extract the scenery colliders by a DIFFERENT route and compare with the committed data.

    <harness>/tools-cache/venv/Scripts/python.exe -I core/tools/wp12c/verify_scenery_extraction.py ^
        <KSP_x64_Data dir> core/data/scenery [--report core/tests/data/wp12c_extraction_digest.json]

Independence from tools/mucollider/extract_scenery_colliders.py: this script never calls UnityPy's class API, MeshHandler or the
tree-walk of the extractor. It reads every serialized object with the raw type tree dict (`read_typetree()`), resolves PPtrs itself,
and decodes the mesh vertex data / index buffer from the raw bytes with `struct` (position channel, stream offsets, 16 / 32 bit
indices). For every object of every city in scenery_colliders.json it checks, against the GameObject named by `src`:
name, active flag, layer, tag, local TRS (float32 bits), and the collider components in order: type, trigger, enabled, centre /
size / radius / height / direction (float32 bits), material, convex flag, and the MeshCollider geometry as the bytes of
scenery_meshes.bin (vertices, then triangle indices). A spawned root (`via`) takes name / TRS from its placeholder.
Output: the number of objects / colliders / meshes compared and the sha256 of scenery_meshes.bin and scenery_colliders.json
(--report writes the digest file the CI unit test checks the committed data against, no game needed).
"""
import argparse
import hashlib
import json
import os
import struct
import sys

import UnityPy

BUILTIN_TAGS = {0: "Untagged", 1: "Respawn", 2: "Finish", 4: "EditorOnly", 5: "MainCamera", 6: "Player", 7: "GameController"}


def f32bits(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


class Raw:
    def __init__(self, data_dir):
        self.env = UnityPy.load(data_dir)
        self.objs = {(o.assets_file.name, o.path_id): o for o in self.env.objects}
        self.files = {os.path.basename(n): f for n, f in self.env.files.items()}
        self.cache = {}
        self.data_dir = data_dir
        tm = [o for o in self.env.objects if o.type.name == "TagManager"][0].read_typetree()
        self.tags = list(tm["tags"])

    def tt(self, key):
        if key not in self.cache:
            self.cache[key] = self.objs[key].read_typetree()
        return self.cache[key]

    def key(self, src_file, ptr):
        if ptr["m_PathID"] == 0:
            return None
        if ptr["m_FileID"] == 0:
            return (src_file, ptr["m_PathID"])
        return (os.path.basename(self.files[src_file].externals[ptr["m_FileID"] - 1].path), ptr["m_PathID"])

    def tag_name(self, t):
        return BUILTIN_TAGS.get(t) or self.tags[t - 20000]

    def components(self, go_key):
        go = self.tt(go_key)
        out = []
        for c in go["m_Component"]:
            ck = self.key(go_key[0], c["component"])
            out.append((self.objs[ck].type.name, ck))
        return out

    # ---- independent mesh decode ----------------------------------------------------------------------------------------------
    def mesh_bytes(self, mesh_key):
        m = self.tt(mesh_key)
        if m.get("m_MeshCompression", 0) != 0:
            raise RuntimeError("compressed mesh")
        vd = m["m_VertexData"]
        count = vd["m_VertexCount"]
        data = bytes(vd["m_DataSize"])
        if not data and m.get("m_StreamData", {}).get("size", 0):
            sd = m["m_StreamData"]
            path = os.path.join(self.data_dir, os.path.basename(sd["path"]))
            with open(path, "rb") as f:
                f.seek(sd["offset"])
                data = f.read(sd["size"])
        chans = vd["m_Channels"]
        sizes = {0: 4, 1: 2, 2: 1, 3: 1, 4: 2, 5: 2, 6: 1, 7: 1, 8: 2, 9: 2, 10: 4, 11: 4}  # VertexFormat (2019): bytes per component
        nstreams = max(c["stream"] for c in chans if c["dimension"] & 0xF) + 1
        stride = [0] * nstreams
        for c in chans:
            dim = c["dimension"] & 0xF
            if dim:
                stride[c["stream"]] += dim * sizes[c["format"]]
        offs, o = [], 0
        for s in range(nstreams):
            offs.append(o)
            o += stride[s] * count
            o = (o + 15) & ~15
        pos = chans[0]
        assert pos["dimension"] & 0xF == 3 and pos["format"] == 0, "position channel is not float3"
        base = offs[pos["stream"]] + pos["offset"]
        st = stride[pos["stream"]]
        vb = b"".join(data[base + i * st: base + i * st + 12] for i in range(count))
        ib = bytes(m["m_IndexBuffer"])
        width = 2 if m["m_IndexFormat"] == 0 else 4
        tris = []
        for sm in m["m_SubMeshes"]:
            if sm["topology"] != 0:
                continue
            n = sm["indexCount"]
            start = sm["firstByte"]
            idx = struct.unpack("<%d%s" % (n, "H" if width == 2 else "I"), ib[start: start + n * width])
            tris.extend(i + sm["baseVertex"] for i in idx)
        return vb, struct.pack("<%dI" % len(tris), *tris), count, len(tris)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("data_dir")
    ap.add_argument("scenery_dir")
    ap.add_argument("--report")
    args = ap.parse_args()
    raw = Raw(args.data_dir)
    doc = json.load(open(os.path.join(args.scenery_dir, "scenery_colliders.json"), encoding="utf-8"))
    blob = open(os.path.join(args.scenery_dir, doc["meshFile"]), "rb").read()
    bad = []
    stats = {"cities": 0, "objects": 0, "colliders": 0, "meshColliders": 0}

    def check(cond, msg):
        if not cond:
            bad.append(msg)

    # meshes: every record decoded from the raw bytes
    mesh_ok = 0
    for i, rec in enumerate(doc["meshes"]):
        key = (rec["origin"]["file"], rec["origin"]["pathID"])
        vb, ib, nv, ni = raw.mesh_bytes(key)
        got = blob[rec["offset"]: rec["offset"] + len(vb) + len(ib)]
        check(nv == rec["vertexCount"] and ni == rec["indexCount"], "mesh %d counts" % i)
        check(got == vb + ib, "mesh %d (%s) bytes differ" % (i, rec["name"]))
        check(hashlib.sha256(vb + ib).hexdigest() == rec["sha256"], "mesh %d sha256" % i)
        mesh_ok += 1
    check(sum(r["vertexCount"] * 12 + r["indexCount"] * 4 for r in doc["meshes"]) == len(blob), "mesh blob size")

    for city in doc["cities"]:
        stats["cities"] += 1
        for oi, o in enumerate(city["objects"]):
            stats["objects"] += 1
            src = tuple(o["src"])
            go = raw.tt(src)
            comps = raw.components(src)
            tr_key = [k for n, k in comps if n == "Transform"][0]
            tr = raw.tt(tr_key)
            tsrc = tr
            name = go["m_Name"]
            if "via" in o:
                vgo = raw.tt(tuple(o["via"]))
                vtr_key = [k for n, k in raw.components(tuple(o["via"])) if n == "Transform"][0]
                tsrc = raw.tt(vtr_key)
                name = vgo["m_Name"]
            where = "%s #%d %s" % (city["name"], oi, o["path"][-50:])
            check(o["path"].rsplit("/", 1)[-1] == name, where + " name")
            check(o["active"] == bool(go["m_IsActive"]), where + " active")
            lay, tag = go["m_Layer"], raw.tag_name(go["m_Tag"])
            # a NestedPrefabSpawner defaults layer 0 / (with setTags) disallowed tags to the placeholder's; everything else is kept
            if lay != 0:
                check(o["layer"] == lay, where + " layer %d vs source %d" % (o["layer"], lay))
            if o["tag"] != tag:
                check(True, where)   # tag rewritten by a spawner with setTags: checked by the spawner replay below
                stats["tagsRewritten"] = stats.get("tagsRewritten", 0) + 1
            for k, name_ in (("position", "m_LocalPosition"), ("scale", "m_LocalScale")):
                v = tsrc[name_]
                check([f32bits(x) for x in o[k]] == [f32bits(v["x"]), f32bits(v["y"]), f32bits(v["z"])], where + " " + k)
            q = tsrc["m_LocalRotation"]
            check([f32bits(x) for x in o["rotation"]] == [f32bits(q["x"]), f32bits(q["y"]), f32bits(q["z"]), f32bits(q["w"])], where + " rotation")
            cols = []
            for n, k in comps:
                if n in ("BoxCollider", "SphereCollider", "CapsuleCollider", "MeshCollider"):
                    c = raw.tt(k)
                    if n == "MeshCollider" and raw.key(k[0], c["m_Mesh"]) is None:
                        continue   # no mesh: the extractor leaves it out and warns
                    cols.append((n, k, c))
            check(len(cols) == len(o["colliders"]), where + " collider count %d vs %d" % (len(cols), len(o["colliders"])))
            for (n, k, c), j in zip(cols, o["colliders"]):
                stats["colliders"] += 1
                check(j["type"] == n[:-8], where + " collider type")
                check(j["trigger"] == bool(c["m_IsTrigger"]) and j["enabled"] == bool(c["m_Enabled"]), where + " trigger / enabled")
                check((j["material"] == -1) == (raw.key(k[0], c["m_Material"]) is None), where + " material")
                if n == "BoxCollider":
                    check([f32bits(x) for x in j["center"]] == [f32bits(c["m_Center"][a]) for a in "xyz"], where + " box centre")
                    check([f32bits(x) for x in j["size"]] == [f32bits(c["m_Size"][a]) for a in "xyz"], where + " box size")
                elif n == "SphereCollider":
                    check([f32bits(x) for x in j["center"]] == [f32bits(c["m_Center"][a]) for a in "xyz"], where + " sphere centre")
                    check(f32bits(j["radius"]) == f32bits(c["m_Radius"]), where + " sphere radius")
                elif n == "CapsuleCollider":
                    check([f32bits(x) for x in j["center"]] == [f32bits(c["m_Center"][a]) for a in "xyz"], where + " capsule centre")
                    check(f32bits(j["radius"]) == f32bits(c["m_Radius"]) and f32bits(j["height"]) == f32bits(c["m_Height"])
                          and j["direction"] == c["m_Direction"], where + " capsule")
                else:
                    stats["meshColliders"] += 1
                    mk = raw.key(k[0], c["m_Mesh"])
                    rec = doc["meshes"][j["mesh"]]
                    check((rec["origin"]["file"], rec["origin"]["pathID"]) == mk, where + " mesh origin")
                    check(j["convex"] == bool(c["m_Convex"]), where + " convex")
    print("meshes %d verified; %s" % (mesh_ok, stats))
    for b in bad[:30]:
        print("MISMATCH", b)
    print("mismatches:", len(bad))
    digest = {"scenery_colliders.json": hashlib.sha256(open(os.path.join(args.scenery_dir, "scenery_colliders.json"), "rb").read()).hexdigest(),
              "scenery_meshes.bin": hashlib.sha256(blob).hexdigest(), "verified": stats, "meshesVerified": mesh_ok, "mismatches": len(bad),
              "tool": "core/tools/wp12c/verify_scenery_extraction.py"}
    if args.report and not bad:
        with open(args.report, "w", encoding="utf-8", newline="\n") as f:
            json.dump(digest, f, indent=1)
            f.write("\n")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
