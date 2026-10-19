#!/usr/bin/env python
"""WP-12c: independent reference for the scenery collider loader (no kspcore code, no UnityPy, no game files).

    python -I core/tools/wp12c/scenery_reference.py core/data/scenery core/tests/data/wp12c_reference.json

From core/data/scenery/scenery_colliders.json + scenery_meshes.bin (the extraction, itself checked against the game by
verify_scenery_extraction.py) this recomputes, with numpy float32 arithmetic that rounds after every operation (the native Unity
Transform maths the C++ port reproduces, see tools/mucollider/mu_colliders_reference.py qmul):

  * the city-frame collider of every non-trigger enabled collider of every object of every city (Box half extents / centre / rotation,
    Sphere radius, Capsule radius and half height and the Jolt rotation, Mesh: placed vertices and indices as FNV-1a 64 of their bytes);
  * the GameObject data of the contact records (layer, tag, name, CrashObjectName found upwards, PositionMobileLaunchPad in the parents);
  * the presence / activation sets: SceneryPresence for three level selections, GameObject.SetActive(true) subtrees of the city's
    switched roots, the objects active from the start (SceneryInitiallyActive);
  * vertical rays onto the tagged launch pad / runway colliders (double precision Moller-Trumbore on the placed triangles).
"""
import hashlib
import json
import math
import os
import struct
import sys

import numpy as np

F = np.float32


def fnv(data, h=0xCBF29CE484222325):
    for b in data:
        h ^= b
        h = (h * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return h


def bits(arr):
    return np.asarray(arr, dtype="<f4").tobytes()


def hexf(vals):
    return "".join("%08x" % struct.unpack("<I", struct.pack("<f", float(v)))[0] for v in vals)


# ---- native Transform maths (float32 after every operation) --------------------------------------------------------------------------
def qmul(a, b):
    ax, ay, az, aw = [F(x) for x in a]
    bx, by, bz, bw = [F(x) for x in b]
    cx = F(F(ay * bz) - F(az * by))
    cy = F(F(az * bx) - F(ax * bz))
    cz = F(F(ax * by) - F(ay * bx))
    return (F(F(cx + F(aw * bx)) + F(bw * ax)), F(F(cy + F(aw * by)) + F(bw * ay)), F(F(cz + F(aw * bz)) + F(bw * az)),
            F(F(F(aw * bw) - F(ax * bx)) - F(F(ay * by) + F(az * bz))))


def rotate(q, v):
    """Quaternion * Vector3 with v a tuple of float32 scalars or arrays (elementwise)."""
    x, y, z, w = [F(c) for c in q]
    two = F(2.0)
    n, n2, n3 = F(x * two), F(y * two), F(z * two)
    n4, n5, n6 = F(x * n), F(y * n2), F(z * n3)
    n7, n8, n9 = F(x * n2), F(x * n3), F(y * n3)
    n10, n11, n12 = F(w * n), F(w * n2), F(w * n3)
    one = F(1.0)
    vx, vy, vz = v
    rx = (F(one - F(n5 + n6)) * vx + F(n7 - n12) * vy) + F(n8 + n11) * vz
    ry = (F(n7 + n12) * vx + F(one - F(n4 + n6)) * vy) + F(n9 - n10) * vz
    rz = (F(n8 - n11) * vx + F(n9 + n10) * vy) + F(one - F(n4 + n5)) * vz
    return rx, ry, rz


def world_chain(objs):
    out = []
    for i, o in enumerate(objs):
        pos, rot, scl = tuple(F(x) for x in o["position"]), tuple(F(x) for x in o["rotation"]), tuple(F(x) for x in o["scale"])
        if i == 0:
            out.append(((F(0), F(0), F(0)), (F(0), F(0), F(0), F(1)), (F(1), F(1), F(1))))
        elif o["parent"] < 0:
            out.append((pos, rot, scl))
        else:
            pp, pr, ps = out[o["parent"]]
            scaled = (F(ps[0] * pos[0]), F(ps[1] * pos[1]), F(ps[2] * pos[2]))
            r = rotate(pr, scaled)
            out.append(((F(pp[0] + r[0]), F(pp[1] + r[1]), F(pp[2] + r[2])), qmul(pr, rot),
                        (F(ps[0] * scl[0]), F(ps[1] * scl[1]), F(ps[2] * scl[2]))))
    return out


def place(w, v):
    pos, rot, scl = w
    r = rotate(rot, (F(scl[0] * v[0]), F(scl[1] * v[1]), F(scl[2] * v[2])))
    return (F(pos[0] + r[0]), F(pos[1] + r[1]), F(pos[2] + r[2]))


def collider_ref(w, c, meshes, blob):
    pos, rot, scl = w
    ax, ay, az = abs(float(scl[0])), abs(float(scl[1])), abs(float(scl[2]))
    a = [F(ax), F(ay), F(az)]
    t = c["type"]
    if t == "Box":
        sz = (F(scl[0] * F(c["size"][0])), F(scl[1] * F(c["size"][1])), F(scl[2] * F(c["size"][2])))
        half = [F(F(abs(float(s))) * F(0.5)) for s in sz]
        ctr = place(w, [F(x) for x in c["center"]])
        return {"k": "B", "v": hexf(half + list(ctr) + list(rot))}
    if t == "Sphere":
        r = F(F(c["radius"]) * max(a))
        ctr = place(w, [F(x) for x in c["center"]])
        return {"k": "S", "v": hexf([r] + list(ctr) + list(rot))}
    if t == "Capsule":
        d = c["direction"] if 0 <= c["direction"] <= 2 else 1
        other = F(0)
        for k in range(3):
            if k != d:
                other = max(other, a[k])
        r = F(F(c["radius"]) * other)
        ht = F(F(c["height"]) * a[d])
        half_cyl = max(F(0), F(F(ht * F(0.5)) - r))
        ctr = place(w, [F(x) for x in c["center"]])
        h = F(0.70710678118654752)
        q = rot
        if d == 0:
            q = qmul(rot, (F(0), F(0), F(-h), h))
        elif d == 2:
            q = qmul(rot, (h, F(0), F(0), h))
        return {"k": "C", "v": hexf([half_cyl, r] + list(ctr) + list(q))}
    m = meshes[c["mesh"]]
    nv, ni, off = m["vertexCount"], m["indexCount"], m["offset"]
    verts = np.frombuffer(blob, dtype="<f4", count=nv * 3, offset=off).reshape(nv, 3)
    idx = np.frombuffer(blob, dtype="<u4", count=ni, offset=off + nv * 12)
    vx, vy, vz = verts[:, 0], verts[:, 1], verts[:, 2]
    scaled = (F(scl[0]) * vx, F(scl[1]) * vy, F(scl[2]) * vz)
    r = rotate(rot, scaled)
    px, py, pz = F(pos[0]) + r[0], F(pos[1]) + r[1], F(pos[2]) + r[2]
    placed = np.stack([px, py, pz], axis=1).astype("<f4")
    return {"k": "M", "n": int(nv), "t": int(ni), "vh": "%016x" % fnv(placed.tobytes()), "ih": "%016x" % fnv(idx.astype("<u4").tobytes())}


# ---- presence and activation --------------------------------------------------------------------------------------------------------
def presence(objs, fallback, levels):
    maxl = {}
    for o in objs:
        g = o.get("levelGroup")
        if g:
            maxl[g] = max(maxl.get(g, 0), o["level"])
    pres = []
    for i, o in enumerate(objs):
        p = True if o["parent"] < 0 else pres[o["parent"]]
        g = o.get("levelGroup")
        if p and g:
            want = levels.get(g, fallback)
            if want < 0 or want > maxl[g]:
                want = maxl[g]
            p = o["level"] == want
        pres.append(p)
    return pres


def subtree(objs, root, pres):
    if not pres[root]:
        return []
    inn = {root}
    out = [root]
    for i in range(root + 1, len(objs)):
        par = objs[i]["parent"]
        if par in inn and objs[i]["active"] and pres[i]:
            inn.add(i)
            out.append(i)
    return out


def initially_active(city, pres):
    objs = city["objects"]
    switched_roots = [] if city.get("class") == "PQSCity2" else list(city["children"])
    for l in city["lod"]:
        switched_roots += l["objects"]
    sw = set()
    for r in switched_roots:
        sw.update(subtree(objs, r, pres))
    act, out = [], []
    for i, o in enumerate(objs):
        pa = True if o["parent"] < 0 else act[o["parent"]]
        a = pres[i] and pa and (i == 0 or o["active"])
        act.append(a)
        if a and i not in sw:
            out.append(i)
    return out


def idx_hash(lst):
    return "%016x" % fnv(np.asarray(lst, dtype="<u4").tobytes())


# ---- rays -----------------------------------------------------------------------------------------------------------------------------
def ray_down(tris, x, z):
    """Highest y of a vertical ray (x, z) through the triangles (double)."""
    best = None
    for a, b, c in tris:
        # barycentric in the xz plane
        d = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2])
        if abs(d) < 1e-12:
            continue
        l1 = ((b[2] - c[2]) * (x - c[0]) + (c[0] - b[0]) * (z - c[2])) / d
        l2 = ((c[2] - a[2]) * (x - c[0]) + (a[0] - c[0]) * (z - c[2])) / d
        l3 = 1.0 - l1 - l2
        if l1 < 0 or l2 < 0 or l3 < 0:
            continue
        y = l1 * a[1] + l2 * b[1] + l3 * c[1]
        if best is None or y > best:
            best = y
    return best


def main():
    sdir, out = sys.argv[1], sys.argv[2]
    doc = json.load(open(os.path.join(sdir, "scenery_colliders.json"), encoding="utf-8"))
    blob = open(os.path.join(sdir, doc["meshFile"]), "rb").read()
    for i, m in enumerate(doc["meshes"]):
        got = hashlib.sha256(blob[m["offset"]: m["offset"] + m["vertexCount"] * 12 + m["indexCount"] * 4]).hexdigest()
        assert got == m["sha256"], "mesh %d digest" % i
    ref = {"schema": "wp12c-scenery-reference/1", "cities": {}}
    for city in doc["cities"]:
        objs = city["objects"]
        chain = world_chain(objs)
        entry = {"objects": [], "presence": {}, "subtrees": {}, "initial": {}}
        for i, o in enumerate(objs):
            crash, mobile = "", False
            k = i
            while k >= 0:
                if not crash and objs[k].get("crashObjectName"):
                    crash = objs[k]["crashObjectName"]
                if objs[k].get("mobileLaunchPad"):
                    mobile = True
                k = objs[k]["parent"]
            cols = [collider_ref(chain[i], c, doc["meshes"], blob) for c in o["colliders"] if not c["trigger"] and c["enabled"]]
            entry["objects"].append({"layer": o["layer"], "tag": o["tag"], "name": o["path"].rsplit("/", 1)[-1], "crash": crash,
                                     "mobile": mobile, "colliders": cols})
        selections = {"first": (0, {}), "highest": (-1, {}), "mixed": (0, {})}
        groups = sorted({o["levelGroup"] for o in objs if o.get("levelGroup")})
        selections["mixed"] = (1, {g: (i % 3) for i, g in enumerate(groups)})
        for name, (fb, lv) in selections.items():
            pres = presence(objs, fb, lv)
            entry["presence"][name] = idx_hash([i for i, p in enumerate(pres) if p])
            roots = list(city["children"]) + [r for l in city["lod"] for r in l["objects"]]
            entry["subtrees"][name] = idx_hash(sorted({j for r in roots for j in subtree(objs, r, pres)}))
            entry["initial"][name] = idx_hash(initially_active(city, pres))
        ref["cities"][city["name"] + "#" + str(city["gameObjectPathID"])] = entry
    ref["levelSelections"] = {"mixed": "fallback 1; group i (sorted facility ids) -> level i % 3"}

    # vertical rays onto the pads: every Mesh collider of an object tagged LaunchPad / Runway of Kerbin/KSC, level 0, in the CITY frame
    ksc = [c for c in doc["cities"] if c["name"] == "Kerbin/KSC"][0]
    objs = ksc["objects"]
    chain = world_chain(objs)
    pres = presence(objs, 0, {})
    rays = {}
    for tag in ("LaunchPad", "Runway"):
        tris = []
        for i, o in enumerate(objs):
            if not pres[i] or o["tag"] != tag:
                continue
            # active in hierarchy below the city's children
            ok, k = True, i
            while k > 0:
                # a direct child of the city is forced active by PQSCity.transformsActive (SetActive(true)), whatever its activeSelf
                ok = ok and pres[k] and (objs[k]["parent"] == 0 or objs[k]["active"])
                k = objs[k]["parent"]
            if not ok:
                continue
            for c in o["colliders"]:
                if c["type"] != "Mesh" or c["trigger"] or not c["enabled"]:
                    continue
                m = doc["meshes"][c["mesh"]]
                nv, ni, off = m["vertexCount"], m["indexCount"], m["offset"]
                verts = np.frombuffer(blob, dtype="<f4", count=nv * 3, offset=off).reshape(nv, 3)
                idx = np.frombuffer(blob, dtype="<u4", count=ni, offset=off + nv * 12).reshape(-1, 3)
                pos, rot, scl = chain[i]
                r = rotate(rot, (F(scl[0]) * verts[:, 0], F(scl[1]) * verts[:, 1], F(scl[2]) * verts[:, 2]))
                p = np.stack([F(pos[0]) + r[0], F(pos[1]) + r[1], F(pos[2]) + r[2]], axis=1).astype("<f4").astype(np.float64)
                for t in idx:
                    tris.append((p[t[0]], p[t[1]], p[t[2]]))
        rays[tag] = {"triangles": len(tris), "samples": []}
        leaf = {"LaunchPad": "LaunchPad_spawn", "Runway": "SpawnPoint"}[tag]
        for i, o in enumerate(objs):
            if pres[i] and o["path"].endswith("/" + leaf) and (tag != "Runway" or "End09" in o["path"]):
                sp = chain[i][0]   # the spawn transform in the city frame (level-0 selection)
                rays[tag]["spawn"] = {"path": o["path"], "x": float(sp[0]), "z": float(sp[2]), "y": float(sp[1])}
                break
        if tris and "spawn" in rays[tag]:
            sx, sz = rays[tag]["spawn"]["x"], rays[tag]["spawn"]["z"]
            rays[tag]["spawn"]["surface"] = ray_down(tris, sx, sz)
        if tris:
            allv = np.array([v for t in tris for v in t])
            xs = np.linspace(allv[:, 0].min(), allv[:, 0].max(), 9)[1:-1]
            zs = np.linspace(allv[:, 2].min(), allv[:, 2].max(), 9)[1:-1]
            for x in xs:
                for z in zs:
                    y = ray_down(tris, float(x), float(z))
                    if y is not None:
                        rays[tag]["samples"].append([float(x), float(z), y])
    ref["rays"] = rays

    # the other stock launch sites: the surface under their SpawnPoint from the mesh colliders tagged with the site's name (level 0, active
    # objects: the subtrees of the switched roots plus the objects no callback switches)
    sites = {"Kerbin/IslandAirfield": "Island_Airfield", "Kerbin/Crater_Launch_Site": "Crater_Launch_Site", "Kerbin/MahiMahi_Launch_Site": "MahiMahi_Launch_Site",
             "Kerbin/Cove_Launch_Site": "Cove_Launch_Site", "Kerbin/GlacierLake_Launch_Site": "GlacierLake_Launch_Site"}
    ref["siteRays"] = {}
    for city in doc["cities"]:
        if city["name"] not in sites or city.get("additionalPrefabOf"):
            continue
        objs = city["objects"]
        chain = world_chain(objs)
        pres = presence(objs, 0, {})
        roots = ([] if city.get("class") == "PQSCity2" else list(city["children"])) + [r for l in city["lod"] for r in l["objects"]]
        act = set(initially_active(city, pres))
        for r in roots:
            act.update(subtree(objs, r, pres))
        tris = []
        for i in sorted(act):
            o = objs[i]
            if o["tag"] != sites[city["name"]]:
                continue
            for c in o["colliders"]:
                if c["trigger"] or not c["enabled"] or c["type"] not in ("Mesh", "Box"):
                    continue
                pos, rot, scl = chain[i]
                if c["type"] == "Box":
                    # the 12 triangles of the oriented box: centre placed, half extents |scale * size| / 2 rotated by the world rotation
                    ctr = place(chain[i], [F(x) for x in c["center"]])
                    hx = [F(F(abs(float(F(scl[k]) * F(c["size"][k])))) * F(0.5)) for k in range(3)]
                    corners = []
                    for sx in (-1, 1):
                        for sy in (-1, 1):
                            for sz in (-1, 1):
                                r = rotate(rot, (F(sx) * hx[0], F(sy) * hx[1], F(sz) * hx[2]))
                                corners.append(np.array([float(F(ctr[0] + r[0])), float(F(ctr[1] + r[1])), float(F(ctr[2] + r[2]))]))
                    faces = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
                    for a, b, cc, d in faces:
                        tris += [(corners[a], corners[b], corners[cc]), (corners[a], corners[cc], corners[d])]
                    continue
                m = doc["meshes"][c["mesh"]]
                nv, ni, off = m["vertexCount"], m["indexCount"], m["offset"]
                verts = np.frombuffer(blob, dtype="<f4", count=nv * 3, offset=off).reshape(nv, 3)
                idx = np.frombuffer(blob, dtype="<u4", count=ni, offset=off + nv * 12).reshape(-1, 3)
                r = rotate(rot, (F(scl[0]) * verts[:, 0], F(scl[1]) * verts[:, 1], F(scl[2]) * verts[:, 2]))
                p = np.stack([F(pos[0]) + r[0], F(pos[1]) + r[1], F(pos[2]) + r[2]], axis=1).astype("<f4").astype(np.float64)
                tris += [(p[t[0]], p[t[1]], p[t[2]]) for t in idx]
        entry = {"tag": sites[city["name"]], "triangles": len(tris)}
        for i, o in enumerate(objs):
            if pres[i] and o["path"].rsplit("/", 1)[-1] == "SpawnPoint":
                sp = chain[i][0]
                entry["spawn"] = {"path": o["path"], "x": float(sp[0]), "z": float(sp[2]), "y": float(sp[1]),
                                  "surface": ray_down(tris, float(sp[0]), float(sp[2])) if tris else None}
                break
        ref["siteRays"][city["name"]] = entry
    json.dump(ref, open(out, "w", encoding="utf-8", newline="\n"), separators=(",", ":"))
    n = sum(len(e["objects"]) for e in ref["cities"].values())
    print("cities", len(ref["cities"]), "objects", n, "ray samples", {k: len(v["samples"]) for k, v in rays.items()})


if __name__ == "__main__":
    sys.exit(main())
