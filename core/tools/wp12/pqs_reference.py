#!/usr/bin/env python3
"""WP-12a: independent reference for the PQS quadtree and quad build -- test data for tests/test_pqs_quadtree.cpp.

Re-implements from the decompiled C# (decompiled/KSP-Decompiled/Assembly-CSharp), with NO kspcore code:
  PQ.cs, PQS.cs (lifecycle, UpdateVisual, UpdateQuadsInit / UpdateQuads, the quad build, normals, tangents, the edge-normal
  stitching), PQSCache.cs (the pool), PQSMod_FlattenArea / QuadEnhanceCoast / CelestialBodyTransform /
  UVPlanetRelativePosition / AltitudeUV / AltitudeAlpha / QuadMeshColliders  (tools/wp12/pqs_ref_tree.py, pqs_ref_sphere.py)
and the runtime they run on (tools/wp12/pqs_ref_math.py): the Mono float rule for managed Unity code; native Matrix4x4.TRS,
Vector3.OrthoNormalize and Quaternion.FromToRotation per-operation float32, each checked here against the in-game C3
capture (harness/captures/out/math_shim_native.json); System.Math Acos / Asin / Atan / Pow from the game runtime's
VS2013 CRT (msvcr120_clr0400.dll through ctypes, Windows only) and Sin / Cos as the x87 model, checked against all
Sin / Cos rows of harness/captures/out/libm_mono.json.

Scenarios: two synthetic spheres whose height stack is defined here and identically in the C++ test (class
SynthHeightMod) plus the real PQSMod_FlattenArea and the quad mods: full emulation, every built quad's mesh compared
frame by frame. Stock Kerbin (KSC pad) and Mun: the tree is emulated here; heights come from the WP-11 height function
through a height oracle (tools/wp12/height_oracle.json, written by the C++ tool tools/wp12/height_oracle.cpp, which
calls only WP-11 entry points; WP-11's GetSurfaceHeight is gated bit for bit by the in-game capture
harness/captures/out2/terrain_height.json); a sample of quads plus every quad whose edge normals they read is built with
the oracle heights and compared bit for bit. radiusMin / radiusMax and the mod requirement flags of the stock spheres
come from the same capture (bodies and mods tables).

Usage (from core/):
  python -I tools/wp12/pqs_reference.py --oracle-exe build/RelWithDebInfo/tests/wp12_height_oracle.exe
      queries the oracle for whatever is missing (tools/wp12/height_oracle.json), then writes tests/data/wp12_reference.json
  python -I tools/wp12/pqs_reference.py
      uses the committed oracle file (fails when a query is missing)
"""
import argparse
import hashlib
import json
import math
import os
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from pqs_ref_math import (CORE, ROOT, check_natives, check_x87, dbits, dnormalized, f32, fbits, from_dhex,  # noqa: E402
                          from_fhex)
from pqs_ref_sphere import (REQ, AltitudeAlpha, AltitudeFade, AltitudeUV, CelestialBodyTransform, FlattenArea,  # noqa: E402
                            PassiveReq, QuadEnhanceCoast, QuadMeshColliders, Sphere, SynthHeight, Target,
                            UVPlanetRelativePosition)
from pqs_ref_tree import MeshCache, Pool  # noqa: E402

ORACLE_FILE = os.path.join(HERE, "height_oracle.json")


def dh(x):
    return "%016x" % dbits(x)


def fh(x):
    return "%08x" % fbits(x)


def dvec(v):
    return [dh(c) for c in v]


def surface_dir(lat_deg, lon_deg):
    """a unit vector from latitude / longitude (y up), plain double maths: an input of the scenarios, not KSP code"""
    la = math.radians(lat_deg)
    lo = math.radians(lon_deg)
    return dnormalized((math.cos(la) * math.cos(lo), math.sin(la), math.cos(la) * math.sin(lo)))


# =====================================================================================================================
# mesh digest (tests/test_pqs_quadtree.cpp MeshDigest computes the same bytes)
# =====================================================================================================================
def mesh_bytes(q):
    out = bytearray()

    def ch(items, n, fmt):
        out.extend(struct.pack("<I", len(items)))
        for it in items:
            out.extend(struct.pack("<" + fmt * n, *it))

    m = q.mesh
    ch(m.vertices, 3, "f")
    ch(m.normals, 3, "f")
    ch(m.tangents, 4, "f")
    ch(m.uv, 2, "f")
    ch(m.uv2, 2, "f")
    ch(m.uv3, 2, "f")
    ch(m.uv4, 2, "f")
    ch(m.colors, 4, "f")
    out.extend(struct.pack("<i", m.triangleSet))
    ch(q.vertsD, 3, "d")
    out.extend(struct.pack("<ddd", *q.meshOrigin))
    ch(q.vertNormals, 3, "f")
    for e in q.edgeNormals:
        ch(e, 3, "f")
    return bytes(out)


def mesh_digest(q):
    return hashlib.sha256(mesh_bytes(q)).hexdigest()[:16]


def walk(sphere):
    out = []

    def rec(q):
        out.append(q)
        if q.isSubdivided:
            for c in q.subNodes:
                if c is not None:
                    rec(c)

    if sphere.quads is not None:
        for r in sphere.quads:
            rec(r)
    return out


def qid(q):
    return -1 if q is None else q.id


def snapshot(sphere, cbt, real):
    rows = []
    for q in walk(sphere):
        flags = (q.isActive * 1 | q.isSubdivided * 2 | q.isVisible * 4 | q.isForcedInvisible * 8 | q.isBuilt * 16
                 | q.isQueuedForNormalUpdate * 32 | q.isQueuedOnlyForCornerNormalUpdate * 64 | q.isCached * 128
                 | q.isPendingCollapse * 256 | q.rendererEnabled * 512 | q.meshColliderCreated * 1024
                 | q.meshColliderEnabled * 2048)
        exact = q.isBuilt and (not real or q.exact)
        rows.append([q.id, q.name, q.subdivision, q.quadIndex, q.plane, flags, q.edgeState, q.mesh.triangleSet,
                     qid(q.north), qid(q.south), qid(q.east), qid(q.west), qid(q.parent),
                     dh(q.gcDist), dh(q.gcd1), dh(q.subdivideThresholdFactor)] + dvec(q.positionPlanet)
                    + [dh(q.meshVertMin) if exact else "", dh(q.meshVertMax) if exact else "",
                       mesh_digest(q) if exact else ""])
    p = sphere.pool
    s = [int(sphere.isActive), int(sphere.isAlive), int(sphere.isStarted), int(sphere.isThinking), dh(sphere.visibleRadius),
         dh(sphere.horizonDistance), dh(sphere.targetHeight), dh(sphere.targetAltitude), sphere.maxLevelAtCurrentTgtSpeed,
         sphere.quadCount, len(sphere.normalUpdateList), p.assigned, p.unassigned, p.total, p.materialised,
         "" if real else dh(sphere.meshVertMin), "" if real else dh(sphere.meshVertMax),
         sum(1 for r in sphere.routines if r == "update"), fh(cbt.planetFade.a) if cbt is not None else ""]
    return {"sphere": s, "quads": rows}


def rows_text(rows):
    """canonical text of a frame's quad rows (tests/test_pqs_quadtree.cpp RowsText builds the same)"""
    return chr(10).join("|".join(str(x) for x in r) for r in rows)


def rows_digest(rows):
    return hashlib.sha256(rows_text(rows).encode("utf-8")).hexdigest()[:16]


def dump_quad(q):
    m = q.mesh
    return {"name": q.name, "vertices": [[fh(c) for c in v] for v in m.vertices],
            "normals": [[fh(c) for c in v] for v in m.normals], "tangents": [[fh(c) for c in v] for v in m.tangents],
            "uv": [[fh(c) for c in v] for v in m.uv], "uv2": [[fh(c) for c in v] for v in m.uv2],
            "uv3": [[fh(c) for c in v] for v in m.uv3], "uv4": [[fh(c) for c in v] for v in m.uv4],
            "colors": [[fh(c) for c in v] for v in m.colors], "triangleSet": m.triangleSet,
            "vertsD": [dvec(v) for v in q.vertsD], "meshOrigin": dvec(q.meshOrigin)}


# =====================================================================================================================
# the height oracle for the stock spheres
# =====================================================================================================================
class Oracle:
    def __init__(self):
        self.data = {"bodies": {}}
        if os.path.exists(ORACLE_FILE):
            self.data = json.load(open(ORACLE_FILE, encoding="utf-8"))
        self.pending = {}

    def body(self, name):
        return self.data["bodies"].setdefault(name, {"public": {}, "build": {}})

    def want_public(self, body, v, state):
        key = ",".join(dvec(v)) + "|" + state
        b = self.body(body)
        if key in b["public"]:
            return from_dhex(b["public"][key])
        self.pending.setdefault(body, {"public": {}, "build": {}})["public"][key] = [dvec(v), state]
        return None

    def want_build(self, body, name, centre, sub, gvs):
        b = self.body(body)
        if name in b["build"]:
            e = b["build"][name]
            return [(from_dhex(h), tuple(from_fhex(c) for c in col)) for h, col in zip(e["h"], e["c"])]
        self.pending.setdefault(body, {"public": {}, "build": {}})["build"][name] = {
            "centre": dvec(centre), "sub": sub, "gv": [dvec(g) for g in gvs]}
        return None

    def resolve(self, exe):
        if not self.pending:
            return False
        if not exe:
            raise SystemExit("height oracle queries missing from %s (%s); rerun with --oracle-exe" % (
                ORACLE_FILE, {b: (len(v["public"]), len(v["build"])) for b, v in self.pending.items()}))
        q = {"bodies": {b: {"public": list(v["public"].values()),
                            "build": [dict(name=n, **e) for n, e in v["build"].items()]} for b, v in self.pending.items()}}
        qpath = os.path.join(HERE, "_oracle_queries.json")
        rpath = os.path.join(HERE, "_oracle_results.json")
        json.dump(q, open(qpath, "w", encoding="utf-8"))
        subprocess.run([os.path.abspath(exe), qpath, rpath], check=True)
        res = json.load(open(rpath, encoding="utf-8"))
        os.remove(qpath)
        os.remove(rpath)
        for bname, r in res["bodies"].items():
            b = self.body(bname)
            for (vec, state), h in zip(q["bodies"][bname]["public"], r["public"]):
                b["public"][",".join(vec) + "|" + state] = h
            for e, rr in zip(q["bodies"][bname]["build"], r["build"]):
                b["build"][e["name"]] = {"centre": e["centre"], "sub": e["sub"], "h": rr["h"], "c": rr["c"]}
        if "provenance" in res:
            self.data["provenance"] = res["provenance"]
        self.pending = {}
        json.dump(self.data, open(ORACLE_FILE, "w", encoding="utf-8", newline="\n"), indent=0, sort_keys=True)
        return True


class OracleHeights:
    """height provider of a stock sphere: public GetSurfaceHeight and per-quad build heights from the oracle"""

    def __init__(self, oracle, body, radius_min, radius_max, exact_builds):
        self.oracle, self.body = oracle, body
        self.radius_min, self.radius_max = radius_min, radius_max
        self.exact_builds = exact_builds  # names whose builds use oracle heights (None: none)
        self.build_records = {}           # name -> (centre, sub, gvs) of every build (pass 1)
        self.reads = {}                   # name -> set of build names whose edge normals its stitching read
        self.missing_public = 0

    def public(self, sphere, v):
        h = self.oracle.want_public(self.body, v, "built" if sphere.any_built else "initial")
        if h is None:
            self.missing_public += 1
            return sphere.radius
        return h

    def build(self, sphere, q, gvs):
        self.build_records[q.name] = (q.positionPlanetRelative, q.subdivision, gvs)
        if self.exact_builds is None or q.name not in self.exact_builds:
            return None
        r = self.oracle.want_build(self.body, q.name, q.positionPlanetRelative, q.subdivision, gvs)
        if r is None:
            raise SystemExit("oracle build heights missing for %s %s" % (self.body, q.name))
        return r

    def record_reads(self, q, reads):
        self.reads.setdefault(q.name, set()).update(p.buildKey for p in reads if p.buildKey is not None)


# =====================================================================================================================
# scenarios
# =====================================================================================================================
def kerbin_fades():
    return AltitudeFade(20000.0, 120000.0, 0.0, 1.0), [AltitudeFade(80000.0, 150000.0, 0.0, 1.0)]


def synth_a():
    """Kerbin-sized synthetic sphere: custom normals + edge stitching + tangents, surface-relative quads, colour / uv2 /
    uv3 / uv4 channels, a quad-dependent FlattenArea, coast factors that change the tree, colliders, the
    CelestialBodyTransform activation cycle, a RebuildSphere, and a call-counting clock (frame budget)"""
    R = 600000.0
    fp = surface_dir(10.6, 31.6)
    stack = [
        {"type": "CelestialBodyTransform", "order": 0, "deactivateAltitude": 160000.0, "hqDeactivate": -1.0,
         "planetFade": [20000.0, 120000.0, 0.0, 1.0], "secondaryFades": [[80000.0, 150000.0, 0.0, 1.0]]},
        {"type": "SynthHeight", "order": 10,
         "req": REQ["MeshCustomNormals"] | REQ["MeshColorChannel"] | REQ["MeshUV2"] | REQ["MeshUV4"],
         "A": 1500.0, "B": -800.0, "C": 600.0, "D": 400.0, "E": 40.0, "K1": 3000.0, "K2": 2500.0, "F": 0.0, "hmax": 4000.0},
        {"type": "FlattenArea", "order": 20, "outerRadius": 3000.0, "innerRadius": 800.0,
         "position": [fp[0] * (R + 120.0), fp[1] * (R + 120.0), fp[2] * (R + 120.0)], "flattenTo": 120.0,
         "smoothStart": 0.0, "smoothEnd": 0.0},
        {"type": "QuadEnhanceCoast", "order": 30, "coastLessThan": 150.0 - R, "oceanFactor": 0.5, "coastFactor": 2.0},
        {"type": "UVPlanetRelativePosition", "order": 40},
        {"type": "AltitudeUV", "order": 50, "atmosphereHeight": 20000.0, "oceanDepth": 1000.0, "invert": False},
        {"type": "AltitudeAlpha", "order": 60, "atmosphereDepth": 4000.0, "invert": True},
        {"type": "QuadMeshColliders", "order": 70, "maxLevelOffset": 1},
    ]
    frames = []

    def fr(lat, lon, alt, fixed, actions):
        d = surface_dir(lat, lon)
        frames.append({"target": [d[0] * (R + alt), d[1] * (R + alt), d[2] * (R + alt)], "fixed": fixed, "actions": actions})

    fr(10.0, 30.0, 500.0, 0, ["start"])
    fr(10.0, 30.0, 500.0, 1, ["frame"])
    fr(10.0, 30.0, 500.0, 1, ["frame"])
    fr(10.0, 30.02, 500.0, 1, ["frame"])
    fr(10.0, 30.04, 600.0, 1, ["frame"])
    fr(10.0, 30.06, 700.0, 2, ["frame"])
    fr(10.0, 30.4, 800.0, 1, ["frame"])
    fr(10.1, 31.0, 5000.0, 1, ["frame"])
    fr(10.2, 31.2, 30000.0, 1, ["frame"])
    fr(10.3, 31.4, 80000.0, 1, ["frame"])
    fr(10.3, 31.5, 170000.0, 1, ["frame"])
    fr(10.4, 31.5, 100000.0, 1, ["frame"])
    fr(10.5, 31.55, 2000.0, 1, ["rebuild", "frame"])
    fr(10.55, 31.58, 800.0, 1, ["frame"])
    fr(10.6, 31.6, 300.0, 1, ["frame"])
    fr(10.6, 31.6, 300.0, 1, ["frame"])
    return {"name": "synth_a", "kind": "synth", "sphere": {"name": "SynthA", "radius": R, "surfaceRelativeQuads": True,
                                                            "buildTangents": True, "preset": [6.0, 1, 8], "clockStep": 0.0002},
            "stack": stack, "frames": frames}


def synth_b():
    """Mun-sized synthetic sphere: sphere normals (no stitching), planet-centred float vertices, quad UVs and vertex map
    coordinates, no CelestialBodyTransform (the host activates the sphere), no clock"""
    R = 200000.0
    stack = [
        {"type": "SynthHeight", "order": 10, "req": REQ["VertexMapCoords"] | REQ["UVQuadCoords"] | REQ["MeshUV4"],
         "A": 900.0, "B": 300.0, "C": -500.0, "D": 250.0, "E": 25.0, "K1": 1700.0, "K2": 1900.0, "F": 30.0, "hmax": 3000.0},
        {"type": "AltitudeUV", "order": 20, "atmosphereHeight": 8000.0, "oceanDepth": 500.0, "invert": False},
        {"type": "QuadMeshColliders", "order": 30, "maxLevelOffset": 0},
    ]
    frames = []

    def fr(lat, lon, alt, fixed, actions):
        d = surface_dir(lat, lon)
        frames.append({"target": [d[0] * (R + alt), d[1] * (R + alt), d[2] * (R + alt)], "fixed": fixed, "actions": actions})

    fr(-35.0, 120.0, 1000.0, 0, ["start", "activate"])
    fr(-35.0, 120.0, 1000.0, 1, ["frame"])
    fr(-35.0, 120.1, 1500.0, 1, ["frame"])
    fr(-35.1, 120.3, 4000.0, 1, ["frame"])
    fr(-35.1, 120.3, 300.0, 1, ["frame"])
    return {"name": "synth_b", "kind": "synth", "sphere": {"name": "SynthB", "radius": R, "surfaceRelativeQuads": False,
                                                            "buildTangents": False, "preset": [6.0, 1, 7], "clockStep": 0.0},
            "stack": stack, "frames": frames}


def real_kerbin():
    R = 600000.0
    frames = []

    def fr(lat, lon, alt, fixed, actions):
        d = surface_dir(lat, lon)
        frames.append({"target": [d[0] * (R + alt), d[1] * (R + alt), d[2] * (R + alt)], "fixed": fixed, "actions": actions})

    # the KSC pad (WP-11: TerrainAltitude 64.78 m at lat -0.0972, lon -74.5577), then a short climb east
    fr(-0.0972, -74.5577, 70.0, 0, ["start"])
    fr(-0.0972, -74.5577, 70.0, 1, ["frame"])
    fr(-0.0972, -74.5577, 70.0, 1, ["frame"])
    fr(-0.0972, -74.5560, 250.0, 1, ["frame"])
    fr(-0.0970, -74.5500, 1000.0, 1, ["frame"])
    fr(-0.0960, -74.5300, 3000.0, 1, ["frame"])
    fr(-0.0950, -74.5000, 3200.0, 1, ["frame"])
    return {"name": "kerbin_ksc", "kind": "real", "body": "Kerbin", "sphere": {"preset": [6.0, 1, 9], "clockStep": 0.0},
            "frames": frames}


def real_mun():
    R = 200000.0
    frames = []

    def fr(lat, lon, alt, fixed, actions):
        d = surface_dir(lat, lon)
        frames.append({"target": [d[0] * (R + alt), d[1] * (R + alt), d[2] * (R + alt)], "fixed": fixed, "actions": actions})

    fr(3.0, 25.0, 6000.0, 0, ["start"])
    fr(3.0, 25.0, 6000.0, 1, ["frame"])
    fr(3.0, 25.05, 5000.0, 1, ["frame"])
    fr(3.05, 25.1, 4500.0, 1, ["frame"])
    return {"name": "mun", "kind": "real", "body": "Mun", "sphere": {"preset": [6.0, 1, 8], "clockStep": 0.0}, "frames": frames}


def make_synth_stack(spec, host):
    mods = []
    cbt = None
    for m in spec:
        t = m["type"]
        if t == "SynthHeight":
            mods.append(SynthHeight(m["order"], m["req"], m["A"], m["B"], m["C"], m["D"], m["E"], m["K1"], m["K2"], m["F"], m["hmax"]))
        elif t == "FlattenArea":
            mods.append(FlattenArea(m["order"], m["outerRadius"], m["innerRadius"], m["position"], m["flattenTo"],
                                    m["smoothStart"], m["smoothEnd"]))
        elif t == "QuadEnhanceCoast":
            mods.append(QuadEnhanceCoast(m["order"], m["coastLessThan"], m["oceanFactor"], m["coastFactor"]))
        elif t == "UVPlanetRelativePosition":
            mods.append(UVPlanetRelativePosition(m["order"]))
        elif t == "AltitudeUV":
            mods.append(AltitudeUV(m["order"], m["atmosphereHeight"], m["oceanDepth"], m["invert"]))
        elif t == "AltitudeAlpha":
            mods.append(AltitudeAlpha(m["order"], m["atmosphereDepth"], m["invert"]))
        elif t == "QuadMeshColliders":
            mods.append(QuadMeshColliders(m["order"], m["maxLevelOffset"]))
        elif t == "CelestialBodyTransform":
            cbt = CelestialBodyTransform(m["order"], m["deactivateAltitude"], m["hqDeactivate"], AltitudeFade(*m["planetFade"]),
                                         [AltitudeFade(*f) for f in m["secondaryFades"]], host)
            mods.append(cbt)
        else:
            raise ValueError(t)
    return mods, cbt


def load_capture_body(body):
    cap = json.load(open(os.path.join(ROOT, "harness", "captures", "out2", "terrain_height.json"), encoding="utf-8"))
    req = 0
    rmin = rmax = None
    for t in cap["tables"]:
        cols = [c["name"] for c in t["columns"]]
        if t["name"] == "mods":
            for row in t["rows"]:
                r = dict(zip(cols, row))
                if r["body"] != body:
                    continue
                if r["requirements"] != "Default":
                    for s in r["requirements"].split(","):
                        req |= REQ[s.strip()]
        if t["name"] == "bodies":
            for row in t["rows"]:
                r = dict(zip(cols, row))
                if r["body"] == body:
                    rmin = from_dhex(r["radiusMin_hex"]) if "radiusMin_hex" in r else r["radiusMin"]
                    rmax = from_dhex(r["radiusMax_hex"]) if "radiusMax_hex" in r else r["radiusMax"]
    return req, rmin, rmax


def make_real_stack(body, host):
    """the stock sphere as the quad build sees it: the six quad mods with their prefab values, the requirement union of
    the game's evaluated stack (capture), the prefab's PQS fields (LOD fixtures as in the C++ factories)"""
    prefab = json.load(open(os.path.join(CORE, "data", "terrain", "pqs_prefab.json"), encoding="utf-8"))
    b = [x for x in prefab["bodies"] if x["bodyName"] == body][0]
    req, rmin, rmax = load_capture_body(body)
    mods = [PassiveReq(-1000, req)]
    cbt = None
    for m in b["mods"]:
        f = m["fields"]
        if not f["modEnabled"] or not m["activeSelf"]:
            continue
        c = m["class"]
        if c == "PQSMod_QuadEnhanceCoast":
            mods.append(QuadEnhanceCoast(f["order"], f["coastLessThan"], f["oceanFactor"], f["coastFactor"]))
        elif c == "PQSMod_UVPlanetRelativePosition":
            mods.append(UVPlanetRelativePosition(f["order"]))
        elif c == "PQSMod_AltitudeUV":
            mods.append(AltitudeUV(f["order"], f["atmosphereHeight"], f["oceanDepth"], bool(f["invert"])))
        elif c == "PQSMod_AltitudeAlpha":
            mods.append(AltitudeAlpha(f["order"], f["atmosphereDepth"], bool(f["invert"])))
        elif c == "PQSMod_QuadMeshColliders":
            mods.append(QuadMeshColliders(f["order"], f["maxLevelOffset"]))
        elif c == "PQSMod_CelestialBodyTransform":
            pf = f["planetFade"]
            cbt = CelestialBodyTransform(f["order"], f["deactivateAltitude"], f["highQualityShaderDeactivateAltitude"],
                                         AltitudeFade(pf["fadeStart"], pf["fadeEnd"], pf["valueStart"], pf["valueEnd"],
                                                      pf["highQualityShaderFadeStart"], pf["highQualityShaderFadeEnd"]),
                                         [AltitudeFade(s["fadeStart"], s["fadeEnd"], s["valueStart"], s["valueEnd"],
                                                       s["highQualityShaderFadeStart"], s["highQualityShaderFadeEnd"])
                                          for s in f["secondaryFades"]], host)
            mods.append(cbt)
    # order ties do not matter here: these mods' effects are independent of each other
    for i, m in enumerate(mods):
        m.order = (m.order, i)
    return mods, cbt, b["pqs"], rmin, rmax


def run(sc, oracle=None, exact_builds=None, cache_pool=None):
    host = {"vessel": None}
    tgt = Target((0.0, 0.0, 0.0))
    host["vessel"] = tgt
    cache, pool = cache_pool if cache_pool else (MeshCache(), None)
    if pool is None:
        pool = Pool(cache, initial=64)
    heights = None
    if sc["kind"] == "synth":
        mods, cbt = make_synth_stack(sc["stack"], host)
        sp = sc["sphere"]
        sphere = Sphere(sp["name"], sp["radius"], mods, pool)
        sphere.surfaceRelativeQuads = sp["surfaceRelativeQuads"]
        sphere.buildTangents = sp["buildTangents"]
    else:
        mods, cbt, pqsj, rmin, rmax = make_real_stack(sc["body"], host)
        heights = OracleHeights(oracle, sc["body"], rmin, rmax, exact_builds)
        sphere = Sphere(pqsj["gameObject"], pqsj["radius"], mods, pool, heights)
        sphere.surfaceRelativeQuads = True  # FIXTURE (as the generated C++ factory: not in pqs_prefab.json)
        sphere.buildTangents = bool(pqsj.get("buildTangents", 0))
        sphere.minLevel, sphere.maxLevel = pqsj["minLevel"], pqsj["maxLevel"]
        sp = sc["sphere"]
    sphere.preset = tuple(sp["preset"])
    step = f32(sp["clockStep"])
    if step != 0.0:
        k = [0]

        def clock():
            k[0] += 1
            return f32(f32(float(k[0])) * step)
        sphere.clock = clock
    sphere.SetTarget(tgt)
    snaps = []
    for fr in sc["frames"]:
        tgt.position = tuple(fr["target"])
        for _ in range(fr["fixed"]):
            sphere.FixedUpdate()
        for a in fr["actions"]:
            if a == "start":
                sphere.Start()
            elif a == "activate":
                sphere.ActivateSphere()
            elif a == "rebuild":
                sphere.RebuildSphere()
            elif a == "frame":
                sphere.FrameUpdate()
            else:
                raise ValueError(a)
        snaps.append(snapshot(sphere, cbt, sc["kind"] == "real"))
    return sphere, snaps, heights


def choose_samples(sphere):
    """the stock spheres' sampled quads at the last frame: the nearest visible leaf at each of the four finest
    subdivision levels present, and the coarsest visible leaf"""
    leaves = [q for q in walk(sphere) if not q.isSubdivided and q.isVisible and q.isBuilt]
    by_level = {}
    for q in leaves:
        by_level.setdefault(q.subdivision, []).append(q)
    levels = sorted(by_level)
    pick = []
    for lv in levels[::-1][:4] + levels[:1]:
        q = min(by_level[lv], key=lambda x: (x.gcd1, x.name))
        if q.name not in pick:
            pick.append(q.name)
    return pick


def cache_section(cache):
    return {"side": cache.side, "cacheVerts": [[fh(c) for c in v] for v in cache.cacheVerts],
            "cacheUVs": [[fh(c) for c in v] for v in cache.cacheUVs],
            "cacheUVQuad": [[dh(c) for c in v] for v in cache.cacheUVQuad],
            "cacheIndices": cache.cacheIndices,
            "tan1": [[fh(c) for c in v] for v in cache.tan1], "tan2": [[fh(c) for c in v] for v in cache.tan2],
            "cacheTangents": [[fh(c) for c in v] for v in cache.cacheTangents]}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--oracle-exe", default=None)
    ap.add_argument("--out", default=os.path.join(CORE, "tests", "data", "wp12_reference.json"))
    ap.add_argument("--only", default=None)
    ap.add_argument("--full", action="store_true", help="keep the quad rows of every frame (diagnostics)")
    args = ap.parse_args()
    t0 = time.time()
    natives = check_natives()
    x87 = check_x87()
    print("natives vs C3 capture:", natives, " x87 Sin/Cos vs libm capture:", x87, file=sys.stderr)

    cache = MeshCache()
    cache.create()
    out = {"provenance": {"tool": "core/tools/wp12/pqs_reference.py", "natives_checked": natives, "x87_checked": x87,
                          "oracle": "core/tools/wp12/height_oracle.json"},
           "cache": cache_section(cache), "scenarios": []}
    oracle = Oracle()
    for sc in (synth_a(), synth_b(), real_kerbin(), real_mun()):
        if args.only and sc["name"] != args.only:
            continue
        t1 = time.time()
        if sc["kind"] == "synth":
            sphere, snaps, _ = run(sc)
            samples = choose_samples(sphere)
        else:
            # pass 0: the target heights (both quad states) drive the tree
            for fr in sc["frames"]:
                d = dnormalized(tuple(fr["target"]))
                oracle.want_public(sc["body"], d, "initial")
                oracle.want_public(sc["body"], d, "built")
            oracle.resolve(args.oracle_exe)
            # pass 1: the tree with dummy vertex heights; records every public query, build and stitching read
            sphere, snaps, h = run(sc, oracle, None)
            samples = choose_samples(sphere)
            need = set(samples)
            for s in samples:
                need |= h.reads.get(s, set())
            for n in sorted(need):
                c, sub, gvs = h.build_records[n]
                oracle.want_build(sc["body"], n, c, sub, gvs)
            oracle.resolve(args.oracle_exe)
            # pass 2: exact heights for the sampled quads and their stitching sources
            sphere, snaps, h = run(sc, oracle, need)
            if h.missing_public:
                raise SystemExit("public height queries still missing: %d" % h.missing_public)
            not_exact = [s for s in samples if not [q for q in walk(sphere) if q.name == s][0].exact]
            if not_exact:
                raise SystemExit("sampled quads not exact: %s" % not_exact)
        last = {q.name: q for q in walk(sphere)}
        entry = dict(sc)
        # every frame carries the digest of its quad rows (and, for the stock spheres, the names of the quads whose
        # mesh is exact and digested); the rows themselves are kept for the first and the last frame (diagnostics)
        for i, sn in enumerate(snaps):
            sn["rows_digest"] = rows_digest(sn["quads"])
            if sc["kind"] == "real":
                sn["exact"] = [r[1] for r in sn["quads"] if r[-1]]
            if not args.full and i != 0 and i != len(snaps) - 1:
                sn["quads"] = None
        entry["snapshots"] = snaps
        entry["samples"] = samples
        entry["dumps"] = [dump_quad(last[s]) for s in samples[:1]]
        out["scenarios"].append(entry)
        nq = [len(s["quads"]) for s in snaps if s["quads"] is not None]
        nd = sum(1 for s in snaps if s["quads"] is not None for r in s["quads"] if r[-1])
        print("%s: %d frames, quads per frame %s, %d mesh digests, samples %s (%.1f s)" % (
            sc["name"], len(snaps), nq, nd, samples, time.time() - t1), file=sys.stderr)
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        json.dump(out, f, separators=(",", ":"))
    print("wrote %s (%.1f s)" % (args.out, time.time() - t0), file=sys.stderr)


if __name__ == "__main__":
    main()
