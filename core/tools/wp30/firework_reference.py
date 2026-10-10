#!/usr/bin/env python3
"""WP-30: independent reference for the numbers of ModulePartFirework (and the ModuleGPS lat/lon text) -- test data generator.

Re-implements from the decompiled C# (no kspcore code, no kspcore output):
  ModulePartFirework.GetModuleMass (ModulePartFirework.cs:769), updateFireworkShotLabel (:624, fireworkShots.ToString("F0")),
  LaunchShell's rigidbody numbers (:505, :516, :518-:521, :522: shell mass, impulse force, vessel-velocity force, recoil),
  the OnStart clamp `fireworkShots > maxShots` (:284),
  ModuleGPS.Update's number formatting (ModuleGPS.cs:116-123: clampLat / clampLon / Deg2Rad, Math.Abs(x).ToString("0.000")).
Inputs are game data: the launcher parts' ModulePartFirework values from core/testdata/GameData-mirror (shellMass, maxShots) and the
field initialisers parsed out of decompiled/KSP-Decompiled/Assembly-CSharp/ModulePartFirework.cs; the sweep (velocities, directions,
fixedDeltaTime) is a fixed grid. Float32 stores use numpy.float32 per the Mono float rule (a float expression is evaluated in double and
rounded only where the C# stores / passes / returns it as a float).

Usage: python -I tools/wp30/firework_reference.py > tests/data/wp30_reference.json
"""
import json
import math
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.normpath(os.path.join(HERE, "..", ".."))
CS = os.path.normpath(os.path.join(CORE, "..", "decompiled", "KSP-Decompiled", "Assembly-CSharp", "ModulePartFirework.cs"))
PARTS = os.path.join(CORE, "testdata", "GameData-mirror", "GameData", "Squad", "Parts", "Utility", "Fireworks")


def st(x):
    """Round to float32 (a C# float store); return the exact value as a Python float."""
    return float(np.float32(x))


def bits(x):
    return int(np.float32(x).view(np.uint32))


def parse_defaults():
    d = {}
    for m in re.finditer(r"public float (\w+) = ([-0-9.]+)f;", open(CS, encoding="utf-8").read()):
        d[m.group(1)] = st(float(m.group(2)))
    return d


def part_values():
    out = []
    for fn in sorted(os.listdir(PARTS)):
        if not fn.endswith(".cfg"):
            continue
        txt = open(os.path.join(PARTS, fn), encoding="utf-8").read()
        m = re.search(r"name = ModulePartFirework\s*\n((?:\s*\w+ = [^\n]*\n)+)", txt)
        if not m:
            continue
        vals = dict(re.findall(r"(\w+) = ([^\s]+)", m.group(1)))
        out.append({"part": fn[:-4], "shellMass": st(float(vals["shellMass"])), "maxShots": st(float(vals["maxShots"]))})
    return out


def vmul(v, d):
    return [st(c * d) for c in v]


def vnorm(v):
    num = st(math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]))
    if num > st(1e-5):
        return [st(c / num) for c in v]
    return [0.0, 0.0, 0.0]


def vnorm_d(v):
    num = math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])
    if num > 0.0:
        return [c / num for c in v]
    return [0.0, 0.0, 0.0]


def f0(x):
    # float.ToString("F0") of a float32 value: round half away from zero
    return str(int(math.floor(abs(x) + 0.5)) * (1 if x >= 0 else -1))


def f3(x):
    # double.ToString("0.000") (Mono NumberFormatter): the value is first reduced to 15 significant digits, then rounded half up at
    # 3 decimals (so 12.3455 -> 12.346 although the double is 12.34549999...)
    from decimal import Decimal, ROUND_HALF_UP
    d15 = Decimal(format(x, ".14e"))
    return str(d15.quantize(Decimal("0.001"), rounding=ROUND_HALF_UP))


def clamp_lat(lat):  # ResourceUtilities.clampLat (ResourceUtilities.cs:232)
    return math.fmod(lat + 180.0 + 90.0, 180.0) - 90.0


def clamp_lon(lon):  # ResourceUtilities.clampLon (:237)
    return math.fmod(lon + 360.0 + 180.0, 360.0) - 180.0


def deg2rad(d):  # ResourceUtilities.Deg2Rad (ResourceUtilities.cs:142): degrees * Math.PI / 180.0
    return d * math.pi / 180.0


def main():
    dflt = parse_defaults()
    parts = part_values()
    dirs = [[0.0, 1.0, 0.0], [0.3, 0.9, -0.2], [-0.5, 0.5, 0.70710677], [0.123456, 0.654321, 0.111111]]
    vels = [st(10.0), st(33.3), dflt["shellVelocity"], st(100.0)]
    dts = [st(0.02), st(1.0 / 60.0), st(0.04)]
    vessel_vels = [[0.0, 0.0, 0.0], [3.5, -2.25, 11.0], [1234.5678, 0.001, -87.6]]
    shots = [32.0, 17.0, 1.0, 0.0]
    cases = []
    for p in parts:
        for v in vels:
            for dt in dts:
                for di, dr in enumerate(dirs):
                    dr_f = [st(c) for c in dr]
                    sm, sc = p["shellMass"], dflt["shellRBMassScaleValue"]
                    # :516 shellForceDir * shellMass * (shellVelocity / Time.fixedDeltaTime) * shellRBMassScaleValue
                    q = st(v / dt)
                    force = vmul(vmul(vmul(dr_f, sm), q), sc)
                    # :522 force.normalized * shellMass * shellVelocity * scale * -1f
                    rec = vmul(vmul(vmul(vmul(vnorm(force), sm), v), sc), -1.0)
                    c = {"part": p["part"], "shellMass": sm, "shellVelocity": v, "fixedDeltaTime": dt, "dir": dr_f, "scale": sc,
                         "rbMass": st(sm * sc), "force": force, "recoil": rec, "vesselForce": []}
                    for vv in vessel_vels:
                        n = vnorm_d(vv)
                        mag = math.sqrt(vv[0] * vv[0] + vv[1] * vv[1] + vv[2] * vv[2])
                        k = mag / float(np.float32(dt))
                        vf = [float(np.float32(((n[i] * float(np.float32(sm))) * k) * float(np.float32(sc)))) for i in range(3)]
                        c["vesselForce"].append({"vel": vv, "force": vf})
                    cases.append(c)
    mass = []
    for p in parts:
        for s in shots:
            for dm in [0.0, 0.5, 2.25]:
                mass.append({"part": p["part"], "defaultMass": st(dm), "fireworkShots": s, "shellMass": p["shellMass"],
                             "mass": st(st(dm) + s * p["shellMass"] * st(0.001)), "label": f0(s)})
    clamp = [{"part": p["part"], "maxShots": p["maxShots"], "fireworkShots": s, "after": min(s, p["maxShots"])} for p in parts
             for s in [40.0, 32.0, 10.0]]
    # ModuleGPS.Update (ModuleGPS.cs:116-123): the argument texts of #autoLOC_6001052 / #autoLOC_6001053
    gps = []
    for lat, lon in [(-0.0972, -74.5575), (28.608, -80.604), (-89.9999, 179.9996), (0.0, 0.0), (45.12345, -122.98765), (12.3455, 5.0005)]:
        la, lo = clamp_lat(lat), clamp_lon(lon)
        gps.append({"lat": lat, "lon": lon, "latAbs": f3(abs(la)), "latSign": 1 if la >= 0.0 else 0, "latRad": f3(deg2rad(la)),
                    "lonAbs": f3(abs(lo)), "lonSign": 1 if lo >= 0.0 else 0, "lonRad": f3(deg2rad(lo))})
    json.dump({"defaults": dflt, "parts": parts, "launch": cases, "mass": mass, "clamp": clamp, "gps": gps}, sys.stdout, indent=0)


if __name__ == "__main__":
    main()
