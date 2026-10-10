#!/usr/bin/env python3
"""WP-25: independent reference for the docking maths -- test data generator for tests/test_docking.cpp.

Re-implements, in plain Python (numpy float32) and from the decompiled C#, the numeric core of
  ModuleDockingNode.CheckDockContact          (ModuleDockingNode.cs:1281: sqrMagnitude / Dot / the snapRotation Acos-Cos roll fold)
  ModuleDockingNode.FindNodeApproaches        (ModuleDockingNode.cs:1226: the nodeType / gender / snapRotation matching expression)
  ModuleDockingNode st_acquire.OnFixedUpdate  (ModuleDockingNode.cs:895: the acquire force and torques of both ports, Mono float chain)
It shares NO code with kspcore. Float32 stores use numpy.float32 per the Mono float rule (a float expression is evaluated in double and rounded
only where the C# stores / passes / returns it as a float).

Inputs (original game data, never hand-authored):
  harness/captures/out2/part_database.json   module_fields: every KSPField of every ModuleDockingNode of every stock part as the game resolved it
The POSES are hand-constructed (rotations about the axes by listed angles); everything the maths reads about the ports (ranges, dots, nodeType,
gender, snap flags, forces) is the game's value.

Usage: python -I core/tools/wp25/dock_reference.py > core/tests/data/wp25_reference.json
"""
import json
import math
import os
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
F = np.float32


def f32(x):
    return F(x)


def bits(x):
    return int(np.asarray(F(x)).view(np.uint32))


def from_bits(b):
    return F(np.asarray(b, dtype=np.uint32).view(np.float32))


# ---- Unity Vector3 in the Mono float rule: components are float32, products/sums are double, stores round ----
def vsub(a, b):
    return tuple(f32(float(x) - float(y)) for x, y in zip(a, b))


def vneg(a):
    return tuple(f32(-float(x)) for x in a)


def vscale(a, s):
    return tuple(f32(float(x) * float(s)) for x in a)


def sqr(a):
    return f32(float(a[0]) * float(a[0]) + float(a[1]) * float(a[1]) + float(a[2]) * float(a[2]))


def dot(a, b):
    return f32(float(a[0]) * float(b[0]) + float(a[1]) * float(b[1]) + float(a[2]) * float(b[2]))


def cross(l, r):
    return (f32(float(l[1]) * float(r[2]) - float(l[2]) * float(r[1])),
            f32(float(l[2]) * float(r[0]) - float(l[0]) * float(r[2])),
            f32(float(l[0]) * float(r[1]) - float(l[1]) * float(r[0])))


def rot(axis, deg):
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)
    if axis == "x":
        return np.array([[1, 0, 0], [0, c, -s], [0, s, c]], dtype=np.float64)
    if axis == "y":
        return np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]], dtype=np.float64)
    return np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]], dtype=np.float64)


def pose(position, R):
    """Transform.position / forward / up / right of a transform with world rotation R (columns: right, up, forward)."""
    right, up, fwd = R[:, 0], R[:, 1], R[:, 2]
    return {"position": tuple(f32(v) for v in position), "forward": tuple(f32(v) for v in fwd),
            "up": tuple(f32(v) for v in up), "right": tuple(f32(v) for v in right)}


def pose_bits(p):
    return [bits(v) for k in ("position", "forward", "up", "right") for v in p[k]]


def check_dock_contact(p1, p2, snap, snap_offset, min_dist, min_fwd, min_roll):
    """ModuleDockingNode.cs:1281"""
    d = vsub(p1["position"], p2["position"])
    if float(sqr(d)) < float(min_dist) * float(min_dist) and float(dot(p1["forward"], vneg(p2["forward"]))) > float(min_fwd):
        num = dot(p1["up"], p2["up"])
        if snap:
            num2 = math.acos(float(num)) if abs(float(num)) <= 1.0 else float("nan")
            if num2 != num2:
                num2 = math.pi if not (float(num) > 0.0) else 0.0
            elif float(dot(p1["up"], p2["right"])) > 0.0:
                num2 = math.pi * 2.0 - num2
            num3 = math.pi / 180.0 * float(snap_offset)
            if num3 > 0.0:
                while num2 > num3:
                    num2 -= num3
            num = f32(math.cos(num2))
        return bool(float(num) > float(min_roll)), num
    return False, f32(0.0)


def acquire(pa, pb, ka, kb):
    """st_acquire.OnFixedUpdate (ModuleDockingNode.cs:895): (force A, torque A, torque-roll A, force B, torque B, torque-roll B)."""
    v = vsub(pb["position"], pa["position"])
    v2 = cross(pa["forward"], vneg(pb["forward"]))
    v3 = cross(pa["up"], vneg(pb["up"]))
    s = sqr(v)
    num = f32(1.0 / float(max(s, f32(0.05))))
    ta = f32(float(ka["acquireForceTweak"]) * float(f32(0.01)))
    tb = f32(float(kb["acquireForceTweak"]) * float(f32(0.01)))
    half = f32(0.5)

    def chain(x, n, k, tw):
        return vscale(vscale(vscale(vscale(x, n), k), half), tw)

    fa = chain(v, num, ka["acquireForce"], ta)
    qa = chain(v2, num, ka["acquireTorque"], ta)
    ra = chain(v3, num, ka["acquireTorqueRoll"], ta)
    # otherNode.part.AddForceAtPosition(-(vector * num) * acquireForce * 0.5f * tweak, ...)
    fb = vscale(vscale(vscale(vneg(vscale(v, num)), kb["acquireForce"]), half), tb)
    qb = vscale(vscale(vscale(vneg(vscale(v2, num)), kb["acquireTorque"]), half), tb)
    rb = vscale(vscale(vscale(vneg(vscale(v3, num)), kb["acquireTorqueRoll"]), half), tb)
    return fa, qa, ra, fb, qb, rb


def node_types(s):
    out = set()
    cur = ""
    for ch in s:
        if ch in ", ":
            if cur:
                out.add(cur)
            cur = ""
        else:
            cur += ch
    if cur:
        out.add(cur)
    return out


def matches(a, b):
    """The matching expression of FindNodeApproaches (ModuleDockingNode.cs:1257-1270): `this` = a, the candidate = b."""
    flag = True
    for t in node_types(a["nodeType"]):
        flag &= t not in node_types(b["nodeType"])
    return (not flag and b["gendered"] == a["gendered"] and (not a["gendered"] or b["genderFemale"] != a["genderFemale"]) and
            b["snapRotation"] == a["snapRotation"] and (not a["snapRotation"] or b["snapOffset"] == a["snapOffset"]))


def load_ports():
    d = json.load(open(os.path.join(ROOT, "harness", "captures", "out2", "part_database.json")))
    T = {t["name"]: t for t in d["tables"]}
    cols = [c["name"] for c in T["module_fields"]["columns"]]
    modules = [r for r in T["modules"]["rows"] if r[3] == "ModuleDockingNode"]
    ports = {}
    for part, idx, *_ in modules:
        ports[part] = {}
    for r in T["module_fields"]["rows"]:
        row = dict(zip(cols, r))
        if row["part"] in ports and any(m[0] == row["part"] and m[1] == row["module_index"] for m in modules):
            ports[row["part"]][row["field"]] = row
    out = {}
    for part, fields in sorted(ports.items()):
        def fl(name):
            return f32(fields[name]["value_num"])

        def bo(name):
            return fields[name]["value"] == "True"

        out[part] = {
            "nodeType": fields["nodeType"]["value"],
            "gendered": bo("gendered"), "genderFemale": bo("genderFemale"), "snapRotation": bo("snapRotation"), "snapOffset": fl("snapOffset"),
            "acquireRange": fl("acquireRange"), "acquireMinFwdDot": fl("acquireMinFwdDot"), "acquireMinRollDot": fl("acquireMinRollDot"),
            "captureRange": fl("captureRange"), "captureMinFwdDot": fl("captureMinFwdDot"), "captureMinRollDot": fl("captureMinRollDot"),
            "acquireForce": fl("acquireForce"), "acquireTorque": fl("acquireTorque"), "acquireTorqueRoll": fl("acquireTorqueRoll"),
            "acquireForceTweak": fl("acquireForceTweak"), "crossfeed": bo("crossfeed"),
        }
    return out


NUMERIC = ["snapOffset", "acquireRange", "acquireMinFwdDot", "acquireMinRollDot", "captureRange", "captureMinFwdDot", "captureMinRollDot",
           "acquireForce", "acquireTorque", "acquireTorqueRoll", "acquireForceTweak"]


def main():
    ports = load_ports()
    names = sorted(ports)
    out = {"schema": "wp25-reference/1", "source": "harness/captures/out2/part_database.json module_fields (ModuleDockingNode); poses hand-constructed",
           "ports": {}, "matching": [], "contact": [], "acquire": []}
    for n in names:
        p = ports[n]
        out["ports"][n] = {"nodeType": p["nodeType"], "gendered": p["gendered"], "genderFemale": p["genderFemale"], "snapRotation": p["snapRotation"],
                           "crossfeed": p["crossfeed"], **{k: bits(p[k]) for k in NUMERIC}}
    for a in names:
        for b in names:
            out["matching"].append({"a": a, "b": b, "match": bool(matches(ports[a], ports[b]))})

    # --- CheckDockContact: pose grid. Port 1 is rotated (yaw / pitch / roll), port 2 faces it, tilted and rolled about its own axis.
    R1 = rot("z", 10.0) @ rot("y", 30.0) @ rot("x", -20.0)
    base1 = np.array([1.5, -2.0, 0.75])
    # distinct parameter sets: one per distinct (ranges, dots) tuple, plus snapRotation variants of the first one (the stock ports never snap)
    seen = {}
    for n in names:
        key = tuple(bits(ports[n][k]) for k in NUMERIC[1:])
        seen.setdefault(key, n)
    sets = [(n, dict(ports[n], snapRotation=False)) for n in seen.values()]
    first = sets[0][1]
    for off in (90.0, 45.0, 60.0, 120.0):
        sets.append(("snap%g" % off, dict(first, snapRotation=True, snapOffset=f32(off))))
    ci = 0
    for sname, k in sets:
        for rng_name, rng, fwd_min, roll_min in (("capture", k["captureRange"], k["captureMinFwdDot"], k["captureMinRollDot"]),
                                                  ("acquire", k["acquireRange"], k["acquireMinFwdDot"], k["acquireMinRollDot"])):
            for dist_f in (0.4, 0.97, 1.08):
                for tilt in (0.0, 1.5, 4.0, 12.0, 40.0):
                    for roll in (0.0, 20.0, 45.0, 89.0, 90.0, 135.0, 180.0, 215.0, 300.0, 359.0):
                        if tilt in (12.0, 40.0) and roll not in (0.0, 90.0, 215.0):
                            continue
                        R2 = R1 @ rot("y", 180.0) @ rot("x", tilt) @ rot("z", roll)
                        pos2 = base1 + R1[:, 2] * (float(rng) * dist_f) + R1[:, 0] * 0.01
                        pa, pb = pose(base1, R1), pose(pos2, R2)
                        res, roll_dot = check_dock_contact(pa, pb, k["snapRotation"], k["snapOffset"], rng, fwd_min, roll_min)
                        out["contact"].append({"set": sname, "kind": rng_name, "snapRotation": bool(k["snapRotation"]), "snapOffset": bits(k["snapOffset"]),
                                               "minDist": bits(rng), "minFwdDot": bits(fwd_min), "minRollDot": bits(roll_min),
                                               "pose1": pose_bits(pa), "pose2": pose_bits(pb), "result": res, "rollDot": bits(roll_dot)})
                        ci += 1
    # --- acquire forces: every distinct parameter set on a few pose pairs; the tweak (a persisted PAW percentage, UI range 0..200) of the two ports differs
    for sname, k0 in sets[:len(seen)]:
        for twa, twb in ((100.0, 100.0), (50.0, 200.0), (0.0, 135.0)):
            ka, kb = dict(k0, acquireForceTweak=f32(twa)), dict(k0, acquireForceTweak=f32(twb))
            for dist, tilt, roll in ((0.3, 0.0, 0.0), (0.45, 3.0, 20.0), (0.05, 8.0, 150.0), (0.0001, 0.0, 0.0), (0.9, 25.0, 270.0)):
                R2 = R1 @ rot("y", 180.0) @ rot("x", tilt) @ rot("z", roll)
                pos2 = base1 + R1[:, 2] * dist + R1[:, 0] * 0.02
                pa, pb = pose(base1, R1), pose(pos2, R2)
                fa, qa, ra, fb, qb, rb = acquire(pa, pb, ka, kb)
                out["acquire"].append({"set": sname, "paramsA": {x: bits(ka[x]) for x in ("acquireForce", "acquireTorque", "acquireTorqueRoll", "acquireForceTweak")},
                                       "paramsB": {x: bits(kb[x]) for x in ("acquireForce", "acquireTorque", "acquireTorqueRoll", "acquireForceTweak")},
                                       "pose1": pose_bits(pa), "pose2": pose_bits(pb),
                                       "forceA": [bits(v) for v in fa], "torqueA": [bits(v) for v in qa], "torqueRollA": [bits(v) for v in ra],
                                       "forceB": [bits(v) for v in fb], "torqueB": [bits(v) for v in qb], "torqueRollB": [bits(v) for v in rb]})
    json.dump(out, sys.stdout, separators=(",", ":"))
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
