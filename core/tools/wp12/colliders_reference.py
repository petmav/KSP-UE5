#!/usr/bin/env python3
"""WP-12b: independent reference for the terrain-collider activation decisions -- test data for tests/test_terrain_colliders.cpp
(gate) and tests/test_pqs_quadtree.cpp.

No kspcore code. It reuses WP-12a's independent Python port of PQ.cs / PQS.cs / PQSCache.cs and the quad mods (tools/wp12/
pqs_reference.py and pqs_ref_*.py, themselves written from the decompiled C# and checked against the in-game captures), and adds
the one thing WP-12b decides: what PQSMod_QuadMeshColliders (Assembly-CSharp/PQSMod_QuadMeshColliders.cs:17-44) enables and
disables, in what order, as the target moves, and the geometry each collider is baked from.

The rule, from the C# (OnSetup :17, OnQuadBuilt :22, OnQuadDestroy :36):
    minLevel = sphere.maxLevel - |maxLevelOffset|
    OnQuadBuilt(q):   if q.subdivision >= minLevel: AddComponent<MeshCollider> once; enabled = true; sharedMesh = q.mesh
    OnQuadDestroy(q): if the quad has a MeshCollider: enabled = false; sharedMesh = null
For every enable / disable this script records an event {seq, kind, quad id, name, subdivision, geometry hash}. The geometry is
what the collider is baked from: triangles = cacheIndices[0] (mesh.triangles in BuildQuad), vertices = the exact double
planet-relative vertices of the quad (PQS.verts, kept per quad as vertsD) relative to the quad's positionPlanet, narrowed to float32;
the hash is FNV-1a 64 over: positionPlanet (3 x f64 LE), vertex count (u32), the relative vertices (3 x f32 each), index count
(u32), the indices (i32). After every frame it also records the set of quad ids that hold a collider, derived only by applying
the events to an empty set. For the stock spheres the heights of most quads are dummies (the WP-12a oracle covers the sampled
quads and their stitching sources only), so a hash is recorded only where the quad's vertices are exact.

Scenarios: the four WP-12a scenarios (synth_a with maxLevelOffset 1, synth_b, stock Kerbin at the KSC, stock Mun) and synth_c, a
Kerbin-sized synthetic sphere walked along the ground, then lifted and put down elsewhere, to make colliders come and go.

Usage (from core/):  python -I tools/wp12/colliders_reference.py [--oracle-exe build/RelWithDebInfo/tests/wp12_height_oracle.exe]
Writes tests/data/wp12b_reference.json.
"""
import argparse
import hashlib
import json
import os
import struct
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import pqs_reference as pr  # noqa: E402
from pqs_ref_math import CORE, check_natives, check_x87  # noqa: E402
from pqs_ref_sphere import REQ, QuadMeshColliders  # noqa: E402

FNV_OFFSET = 14695981039346656037
FNV_PRIME = 1099511628211
MASK = (1 << 64) - 1


def fnv(h, data):
    for b in data:
        h = ((h ^ b) * FNV_PRIME) & MASK
    return h


def geometry_hash(centre, vertices, indices):
    """centre: 3 doubles; vertices: (x, y, z) already-narrowed float32 values; indices: ints"""
    h = FNV_OFFSET
    h = fnv(h, struct.pack("<3d", *centre))
    h = fnv(h, struct.pack("<I", len(vertices)))
    for v in vertices:
        h = fnv(h, struct.pack("<3f", *v))
    h = fnv(h, struct.pack("<I", len(indices)))
    h = fnv(h, struct.pack("<%di" % len(indices), *indices))
    return h


class Log:
    """the events of one scenario run, and the tracked collider set"""

    def __init__(self):
        self.events = []     # [seq, 'E'|'D', id, name, subdivision, hash hex or '']
        self.tracked = {}    # id(q) -> q  (the pooled quad object owns the Unity component)
        self.marks = []      # (len(events), sorted ids) after each frame


LOG = Log()


class LoggingQuadMeshColliders(QuadMeshColliders):
    """PQSMod_QuadMeshColliders with the activation log"""

    def OnQuadBuilt(self, q):  # :22
        super().OnQuadBuilt(q)
        if q.subdivision >= self.minLevel:
            idx = self.sphere.cache.cacheIndices[0]
            vr = []
            for v in q.vertsD:
                vr.append(tuple(struct.unpack("<f", struct.pack("<f", v[i] - q.positionPlanet[i]))[0] for i in range(3)))
            h = geometry_hash(q.positionPlanet, vr, idx) if q.exact else None
            LOG.events.append([len(LOG.events), "E", q.id, q.name, q.subdivision, "%016x" % h if h is not None else ""])
            LOG.tracked[id(q)] = q

    def OnQuadDestroy(self, q):  # :36
        super().OnQuadDestroy(q)
        if q.meshColliderCreated:
            LOG.events.append([len(LOG.events), "D", q.id, q.name, q.subdivision, ""])
            LOG.tracked.pop(id(q), None)


def snapshot_hook(orig):
    def snap(sphere, cbt, real):
        LOG.marks.append((len(LOG.events), sorted(q.id for q in LOG.tracked.values())))
        return orig(sphere, cbt, real)
    return snap


def synth_c():
    """Kerbin-sized synthetic sphere (no FlattenArea): walked east along the ground (colliders follow the target across quad
    borders), climbed above the CelestialBodyTransform deactivation altitude and back (the sphere is deactivated and its quads
    destroyed), put down 1.5 degrees away, walked again, RebuildSphere"""
    R = 600000.0
    stack = [
        {"type": "CelestialBodyTransform", "order": 0, "deactivateAltitude": 160000.0, "hqDeactivate": -1.0,
         "planetFade": [20000.0, 120000.0, 0.0, 1.0], "secondaryFades": [[80000.0, 150000.0, 0.0, 1.0]]},
        {"type": "SynthHeight", "order": 10, "req": REQ["MeshCustomNormals"], "A": 900.0, "B": -400.0, "C": 300.0, "D": 250.0,
         "E": 30.0, "K1": 4000.0, "K2": 3000.0, "F": 0.0, "hmax": 3000.0},
        {"type": "QuadMeshColliders", "order": 70, "maxLevelOffset": 0},
    ]
    frames = []

    def fr(lat, lon, alt, fixed, actions):
        d = pr.surface_dir(lat, lon)
        frames.append({"target": [d[0] * (R + alt), d[1] * (R + alt), d[2] * (R + alt)], "fixed": fixed, "actions": actions})

    fr(5.0, 20.0, 400.0, 0, ["start"])
    fr(5.0, 20.0, 400.0, 1, ["frame"])
    for i in range(1, 15):          # about 200 m per step eastwards
        fr(5.0, 20.0 + 0.02 * i, 400.0, 1, ["frame"])
    for alt in (3000.0, 30000.0, 30000.0, 80000.0, 170000.0, 170000.0, 100000.0, 20000.0, 2000.0):   # climb out and back
        fr(5.0, 20.3, alt, 1, ["frame"])
    fr(6.5, 21.8, 800.0, 1, ["frame"])
    fr(6.5, 21.8, 800.0, 1, ["frame"])
    for i in range(1, 8):
        fr(6.5, 21.8 + 0.03 * i, 300.0, 1, ["frame"])
    fr(6.5, 22.0, 300.0, 1, ["rebuild", "frame"])
    fr(6.5, 22.0, 300.0, 1, ["frame"])
    return {"name": "synth_c", "kind": "synth", "sphere": {"name": "SynthC", "radius": R, "surfaceRelativeQuads": True,
                                                            "buildTangents": False, "preset": [6.0, 1, 9], "clockStep": 0.0},
            "stack": stack, "frames": frames}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--oracle-exe", default=None)
    ap.add_argument("--out", default=os.path.join(CORE, "tests", "data", "wp12b_reference.json"))
    ap.add_argument("--only", default=None)
    args = ap.parse_args()
    t0 = time.time()
    natives = check_natives()
    x87 = check_x87()
    print("natives vs C3 capture:", natives, " x87 Sin/Cos vs libm capture:", x87, file=sys.stderr)
    pr.QuadMeshColliders = LoggingQuadMeshColliders
    orig_snapshot = pr.snapshot
    pr.snapshot = snapshot_hook(orig_snapshot)

    out = {"provenance": {"tool": "core/tools/wp12/colliders_reference.py", "natives_checked": natives, "x87_checked": x87,
                          "oracle": "core/tools/wp12/height_oracle.json",
                          "rule": "PQSMod_QuadMeshColliders.cs:17-44; geometry = vertsD - positionPlanet as float32, cacheIndices[0]"},
           "scenarios": []}
    oracle = pr.Oracle()
    for sc in (pr.synth_a(), pr.synth_b(), pr.real_kerbin(), pr.real_mun(), synth_c()):
        if args.only and sc["name"] != args.only:
            continue
        t1 = time.time()
        LOG.__init__()
        if sc["kind"] == "synth":
            sphere, snaps, _ = pr.run(sc)
        else:
            for fr in sc["frames"]:
                d = pr.dnormalized(tuple(fr["target"]))
                oracle.want_public(sc["body"], d, "initial")
                oracle.want_public(sc["body"], d, "built")
            oracle.resolve(args.oracle_exe)
            LOG.__init__()
            sphere, snaps, h = pr.run(sc, oracle, None)
            samples = pr.choose_samples(sphere)
            need = set(samples)
            for s in samples:
                need |= h.reads.get(s, set())
            for n in sorted(need):
                c, sub, gvs = h.build_records[n]
                oracle.want_build(sc["body"], n, c, sub, gvs)
            oracle.resolve(args.oracle_exe)
            LOG.__init__()
            sphere, snaps, h = pr.run(sc, oracle, need)
        entry = dict(sc)
        frames = []
        prev = 0
        for (n, ids) in LOG.marks:
            rows = LOG.events[prev:n]
            prev = n
            text = "\n".join("|".join(str(x) for x in r) for r in rows)
            frames.append({"events": [list(r) for r in rows], "events_digest": hashlib.sha256(text.encode()).hexdigest()[:16],
                           "ids": ids})
        entry["colliders"] = frames
        entry["event_count"] = len(LOG.events)
        out["scenarios"].append(entry)
        en = sum(1 for e in LOG.events if e[1] == "E")
        hashed = sum(1 for e in LOG.events if e[1] == "E" and e[5])
        print("%s: %d frames, %d enable / %d disable events (%d with a geometry hash), final set %d (%.1f s)" % (
            sc["name"], len(frames), en, len(LOG.events) - en, hashed, len(LOG.marks[-1][1]), time.time() - t1), file=sys.stderr)
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        json.dump(out, f, separators=(",", ":"))
    print("wrote %s (%.1f s)" % (args.out, time.time() - t0), file=sys.stderr)


if __name__ == "__main__":
    main()
