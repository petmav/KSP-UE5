#!/usr/bin/env python3
"""WP-19a independent reference: flight-scene registry, rails and VesselPrecalculate formulas.

No kspcore code. Everything is written from the decompiled C# (Assembly-CSharp):
  VesselPrecalculate.cs   CalculateGravity (cs:293-374: gravity at the CoM, Coriolis / centrifugal, easing, orbit drift compensation)
  FlightGlobals.cs        getGeeForceAtPosition (:1347), getCentrifugalAcc / getCoriolisAcc (:1356-1373)
  OrbitPhysicsManager.cs  LateUpdate (:107-153: pack / unpack by distance and time warp)
  Vessel.cs               Update (:2990: load / unload by range), CheckKill (:2746), updateSituation (:3133)
  VesselRanges.cs         GetSituationRanges (:95)
  UnityEngine Vector3.cs  Distance (:373), sqrMagnitude, operator- (Mono float rule)

The arithmetic of the C# is IEEE double everywhere except the pack / unpack / load / unload / kill distance checks, which are Unity Vector3 (float)
code under the Mono rule (core/README.md): a C# `float` expression is evaluated in double and rounded to float32 where the C# stores, returns or
passes it. Python floats hold float32-exact values for C# floats (numpy float32 only at those boundaries).

Fixtures that are NOT KSP data: every input row (positions, velocities, thresholds, ranges, states) is generated here from a fixed seed; only the
code under test is the game's.

Output: core/tests/data/wp19a_reference.json. Doubles are written as 16 hex digits of the IEEE-754 bits, float32 as 8; bool / int as JSON.
Usage:  python -I core/tools/wp19a/flight_reference.py
"""
import json
import math
import os
import random
import struct

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(CORE, "tests", "data", "wp19a_reference.json")

f32 = np.float32
R = random.Random(190)


def hx(x):
    return struct.pack(">d", x).hex()


def hf(x):
    return struct.pack(">f", float(x)).hex()


def H3(v):
    return [hx(c) for c in v]


def F(x):
    """C# float store of a double expression"""
    return float(f32(x))


# ---------------------------------------------------------------------------------------------------- Vector3d (all double)
def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def mul(a, d):
    return (a[0] * d, a[1] * d, a[2] * d)


def div(a, d):
    return (a[0] / d, a[1] / d, a[2] / d)


def sqr(a):
    return a[0] * a[0] + a[1] * a[1] + a[2] * a[2]


def cross(l, r):
    return (l[1] * r[2] - l[2] * r[1], l[2] * r[0] - l[0] * r[2], l[0] * r[1] - l[1] * r[0])


def swz(a):
    return (a[0], a[2], a[1])


def invalid(v):
    d = v[0] + v[1] + v[2]
    return math.isnan(d) or math.isinf(d)


def rv(scale):
    return (R.uniform(-scale, scale), R.uniform(-scale, scale), R.uniform(-scale, scale))


def unit(v):
    n = math.sqrt(sqr(v))
    return div(v, n)


# ---------------------------------------------------------------------------------------------------- FlightGlobals.cs
def gee_force(pos, body, g):
    """FlightGlobals.cs:1347 getGeeForceAtPosition(pos, mainBody)"""
    p = sub(pos, body)
    s = sqr(p)
    s *= math.sqrt(s)
    return div(mul(p, 0.0 - g), s)


def centrifugal(omega, body, pos, inv):
    """FlightGlobals.cs:1356"""
    if not inv:
        return (0.0, 0.0, 0.0)
    p = sub(body, pos)
    return cross(omega, cross(omega, p))


def coriolis(omega, vel, inv):
    """FlightGlobals.cs:1366 -2.0 * Vector3d.Cross(angularVelocity, vel)"""
    if not inv:
        return (0.0, 0.0, 0.0)
    c = cross(omega, vel)
    return (c[0] * -2.0, c[1] * -2.0, c[2] * -2.0)


# ---------------------------------------------------------------------------------------------------- VesselPrecalculate.cs
def drift_check(always, d_pos, d_vel, thr, wait, frames):
    """cs:278-290. d_pos = cpos - (CoMD - body.position); d_vel = cvel - curRotFrameVel - velocityD (all swizzled)"""
    if not always and (not (sqr(d_pos) < thr) or sqr(d_vel) >= thr):
        return 0, wait
    if frames > 0:
        return 1, frames - 1
    return 2, frames


def drift_apply(i):
    """cs:293-314"""
    fdt, rec = i["fdt"], i["rec"]
    pre = mul(sub(sub(add(i["rpn"], i["bodypos"]), i["comd"]), mul(i["vel"], fdt)), rec)
    post = sub(sub(sub(i["rvn"], i["vel"]), i["rfn"]), pre)
    k = i["G"] * i["gm"]
    pre = mul(pre, k)
    post = mul(post, k)
    gacc = mul(sub(sub(i["rvn"], i["vel"]), i["crf"]), rec)
    gfp = mul(pre, rec)
    inv = invalid(pre) or invalid(post)
    if inv:
        pre = (0.0, 0.0, 0.0)
        post = (0.0, 0.0, 0.0)
    return pre, post, (0.0, 0.0, 0.0), gacc, gfp, inv


def rfv_fixture(w2, refpos_sw):
    """stands for vessel.orbit.GetRotFrameVelAtPos(mainBody, refPos): any deterministic double function of the swizzled position"""
    return cross(w2, refpos_sw)


def gravity(i):
    """cs:317-343 (the `if (!wasCorrectingDrift)` block)"""
    ia = i["gtrue"]
    k = i["G"] * i["gm"]
    fdt = i["fdt"]
    if i["inv"]:
        ia = add(ia, add(i["cor"], i["cen"]))
        ia = mul(ia, k)
        gfp = ia
        v2 = add(i["vel"], mul(ia, fdt))
        refpos = sub(add(i["comd"], mul(v2, fdt)), i["bodypos"])
        refpos = swz(refpos)
        rfv = i["rfv"]
        rfvp = swz(rfv_fixture(i["w2"], refpos))
        g = sub(add(v2, rfvp), add(i["vel"], rfv))
        g = mul(g, 1.0 / fdt)
        return ia, g, gfp
    ia = mul(ia, k)
    return ia, ia, ia


# ---------------------------------------------------------------------------------------------------- Vector3 (float) boundary
def f_sub(a, b):
    return tuple(F(a[i] - b[i]) for i in range(3))


def f_sqr(a):
    return F(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])


def f_distance(a, b):
    """Vector3.cs:373"""
    n = F(a[0] - b[0])
    n2 = F(a[1] - b[1])
    n3 = F(a[2] - b[2])
    return F(math.sqrt(n * n + n2 * n2 + n3 * n3))


SIT = {"LANDED": 1, "SPLASHED": 2, "PRELAUNCH": 4, "FLYING": 8, "SUB_ORBITAL": 0x10, "ORBITING": 0x20, "ESCAPING": 0x40, "DOCKED": 0x80}
RANGE_KEYS = ["prelaunch", "landed", "splashed", "flying", "subOrbital", "orbit", "escaping"]
DEFAULT_RANGES = {
    "prelaunch": (2250.0, 2500.0, 350.0, 200.0), "landed": (2250.0, 2500.0, 350.0, 200.0), "splashed": (2250.0, 2500.0, 350.0, 200.0),
    "flying": (2250.0, 22500.0, 25000.0, 2000.0), "subOrbital": (2250.0, 15000.0, 10000.0, 200.0),
    "orbit": (2250.0, 2500.0, 350.0, 200.0), "escaping": (2250.0, 2500.0, 350.0, 200.0)}


def get_ranges(rng, situation):
    """VesselRanges.cs:95 -> (load, unload, pack, unpack)"""
    m = {SIT["FLYING"]: "flying", SIT["LANDED"]: "landed", SIT["SPLASHED"]: "splashed", SIT["PRELAUNCH"]: "prelaunch",
         SIT["ESCAPING"]: "escaping", SIT["SUB_ORBITAL"]: "subOrbital"}
    return rng[m.get(situation, "orbit")]


def best_situation(situation, was_flying):
    """Vessel.cs:691: lastSituation == FLYING ? lastSituation : situation"""
    return SIT["FLYING"] if was_flying else situation


def late_update(sc):
    """OrbitPhysicsManager.cs:107-153. Returns the list of (vessel index or -1 for the active vessel, 'on'|'off') and the final packed flags"""
    acts = []
    act_packed = sc["active"]["packed"]
    packed = [v["packed"] for v in sc["vessels"]]
    if not (sc["ready"] and sc["started"] and not sc["override"]):
        return acts, act_packed, packed
    flag = sc["warpHigh"] and sc["rate"] > sc["maxPhys"]
    if not sc["warpDrive"]:
        if flag:
            if not act_packed:
                acts.append((-1, "on"))
                act_packed = True
        elif act_packed and not sc["hold"]:
            acts.append((-1, "off"))
            act_packed = False
    apos = sc["active"]["pos"]
    for i, v in enumerate(sc["vessels"]):
        if v["isActive"] or not v["hasDriver"]:
            continue
        if not sc["warpDrive"] and flag and not packed[i]:
            acts.append((i, "on"))
            packed[i] = True
        num = f_distance(v["pos"], apos)
        pack = get_ranges(v["ranges"], best_situation(v["situation"], v["wasFlying"]))[2]
        unpack = get_ranges(v["ranges"], v["situation"])[3]
        if num > pack:
            if not packed[i]:
                acts.append((i, "on"))
                packed[i] = True
        elif num < unpack and packed[i] and sc["rate"] <= sc["maxPhys"] and not sc["hold"]:
            acts.append((i, "off"))
            packed[i] = False
    return acts, act_packed, packed


def vessel_update(sc):
    """Vessel.cs:2990-3053, the `this != ActiveVessel` branch. Returns the list of actions in order and the final loaded flag"""
    acts = []
    loaded = sc["loaded"]
    d = f_distance(sc["pos"], sc["apos"])
    ld = get_ranges(sc["ranges"], sc["situation"])
    if loaded and d > ld[1]:
        acts.append("unload")
        loaded = False
    if not loaded and d < ld[0]:
        acts.append("load")
        loaded = True
    return acts, loaded


def check_kill(sc):
    """Vessel.cs:2746-2793. Returns 'die' (pressure kill), 'crash_explode', 'crash_die' or 'none'"""
    if sc["ready"] and sc["packed"] and not sc["isActive"] and not sc["override"] and not sc["landedOrSplashed"] and sc["pressureAbove1"]:
        flag = True
        if sc["sceneFlight"] and sc["hasActive"]:
            pack = get_ranges(sc["ranges"], best_situation(sc["situation"], sc["wasFlying"]))[2]
            pack = F(pack * pack)
            if f_sqr(f_sub(sc["pos"], sc["apos"])) < pack:
                flag = False
        if flag:
            return "die"
    if not sc["landedOrSplashed"] and sc["holdExpired"]:
        ta = sc["terrainAltitude"]
        lim = ta if ta != -1.0 else (-250.0 if sc["loaded"] else -0.1)
        if sc["altitude"] < lim:
            return "crash_explode" if sc["loaded"] else "crash_die"
    return "none"


def situation(sc):
    """Vessel.cs:3133-3180 (EVALadderVessel == this branch); Orbit.cs:304-308 PeR / ApR / PeA"""
    apr = (1.0 + sc["ecc"]) * sc["sma"]
    pea = (1.0 - sc["ecc"]) * sc["sma"] - sc["radius"]
    if sc["landed"]:
        if sc["situation"] == SIT["PRELAUNCH"] and (not (sc["srfSpeed"] > 2.5) or sc["easing"] or sc["spawning"]):
            return SIT["PRELAUNCH"]
        return SIT["LANDED"]
    if sc["splashed"]:
        return SIT["SPLASHED"]
    if sc["pressure"] > 0.0:
        return SIT["FLYING"]
    if sc["ecc"] < 1.0 and apr < sc["soi"]:
        if pea < (sc["depth"] if sc["atmo"] else 0.0):
            return SIT["SUB_ORBITAL"]
        return SIT["ORBITING"]
    return SIT["ESCAPING"]


# ---------------------------------------------------------------------------------------------------- case generation
def gen_ranges():
    out = {}
    for k in RANGE_KEYS:
        if R.random() < 0.5:
            out[k] = DEFAULT_RANGES[k]
        else:
            lo = F(R.choice([100.0, 500.0, 2250.0]) * R.uniform(0.5, 2))
            out[k] = (lo, F(lo * R.uniform(1.0, 2.0)), F(R.uniform(100, 30000)), F(R.uniform(50, 3000)))
    return out


def ranges_json(r):
    return {k: [hf(c) for c in r[k]] for k in RANGE_KEYS}


def rpos(scale):
    return tuple(F(R.uniform(-scale, scale)) for _ in range(3))


def near_pos(base, dist):
    """a float32 position at about `dist` from base along a random direction"""
    d = unit(rv(1.0))
    return tuple(F(base[i] + d[i] * dist) for i in range(3))


def build():
    out = {"schema": "wp19a/1"}

    # ---- A. gee force
    gee = []
    for _ in range(300):
        pos = rv(R.choice([1e3, 7e5, 2e7, 1e9]))
        body = rv(R.choice([0.0, 1e3, 1e7, 1e11]))
        g = R.choice([3.5316e12 / (6e5 * 6e5), 9.80665, R.uniform(0.1, 30.0)])
        o = gee_force(pos, body, g)
        gee.append({"pos": H3(pos), "body": H3(body), "g": hx(g), "out": H3(o)})
    out["gee"] = gee

    # ---- B. Coriolis / centrifugal
    fict = []
    for k in range(300):
        omega = (R.uniform(-1e-3, 1e-3), R.uniform(-3e-4, -1e-5), R.uniform(-1e-3, 1e-3))
        body = rv(R.choice([1.0, 1e7]))
        pos = add(body, rv(R.choice([6e5, 7e5, 3e6])))
        vel = rv(R.choice([10.0, 2500.0, 8000.0]))
        inv = (k % 4) != 0
        fict.append({"omega": H3(omega), "body": H3(body), "pos": H3(pos), "vel": H3(vel), "inv": inv,
                     "cen": H3(centrifugal(omega, body, pos, inv)), "cor": H3(coriolis(omega, vel, inv))})
    out["fict"] = fict

    # ---- C. drift check (the decision of cs:278-290)
    dc = []
    for k in range(600):
        thr = R.choice([100.0, 25.0, 400.0, R.uniform(10.0, 1000.0)])
        always = (k % 7) == 0
        s = math.sqrt(thr)
        facs = [0.0, 0.5, 0.99, 1.0, 1.0, 1.01, 2.0, R.uniform(0.0, 3.0)]
        d1 = mul(unit(rv(1.0)), s * R.choice(facs))
        d2 = mul(unit(rv(1.0)), s * R.choice(facs))
        if k % 11 == 0 and thr == 100.0:
            d1 = (10.0, 0.0, 0.0)  # exactly on the threshold (10^2 == 100): not less than
        if k % 13 == 0 and thr == 100.0:
            d2 = (0.0, 0.0, 10.0)  # exactly >= the threshold
        wait = R.choice([1, 5, 10, 60])
        fr = R.choice([0, 0, 1, 2, wait])
        a, nf = drift_check(always, d1, d2, thr, wait, fr)
        dc.append({"always": always, "d1": H3(d1), "d2": H3(d2), "thr": hx(thr), "wait": wait, "frames": fr, "action": a, "newFrames": nf})
    out["drift_check"] = dc

    # ---- D. drift apply
    da = []
    for k in range(500):
        pos = rv(7e5)
        vel = rv(2500.0)
        i = {"rpn": add(pos, rv(50.0)), "rvn": add(vel, rv(1.0)), "rfn": rv(300.0), "crf": rv(300.0), "bodypos": rv(R.choice([1.0, 1e7])),
             "comd": add(pos, rv(30.0)), "vel": vel, "fdt": R.choice([0.02, 0.04, 0.0199999995529651]), "G": R.choice([1.0, 1.0, 0.5, 2.0]),
             "gm": R.choice([1.0, 1.0, 0.0625, 0.5])}
        i["rec"] = 1.0 / i["fdt"]
        if k % 50 == 7:
            i["rpn"] = (float("inf"), i["rpn"][1], i["rpn"][2])
            i["comd"] = (float("inf"), i["comd"][1], i["comd"][2])  # inf - inf = NaN in the offset
        if k % 50 == 13:
            i["vel"] = (1e308, i["vel"][1], i["vel"][2])
            i["rvn"] = (-1e308, i["rvn"][1], i["rvn"][2])  # overflow to -inf in the correction
        pre, post, ia, g, gfp, inv = drift_apply(i)
        da.append({"rpn": H3(i["rpn"]), "rvn": H3(i["rvn"]), "rfn": H3(i["rfn"]), "crf": H3(i["crf"]), "bodypos": H3(i["bodypos"]),
                   "comd": H3(i["comd"]), "vel": H3(i["vel"]), "fdt": hx(i["fdt"]), "rec": hx(i["rec"]), "G": hx(i["G"]), "gm": hx(i["gm"]),
                   "pre": H3(pre), "post": H3(post), "ia": H3(ia), "g": H3(g), "gfp": H3(gfp), "invalid": inv})
    out["drift_apply"] = da

    # ---- E. gravity integration
    gv = []
    for k in range(500):
        bodypos = rv(R.choice([1.0, 1e7]))
        comd = add(bodypos, rv(R.choice([6.5e5, 7e5, 2e6])))
        vel = rv(R.choice([5.0, 2300.0, 8000.0]))
        omega = (R.uniform(-1e-3, 1e-3), R.uniform(-3e-4, -1e-5), R.uniform(-1e-3, 1e-3))
        inv = (k % 3) != 0
        gtrue = gee_force(comd, bodypos, R.choice([9.81, R.uniform(1, 12)]))
        cor = coriolis(omega, vel, True)
        cen = centrifugal(omega, bodypos, comd, True)
        i = {"gtrue": gtrue, "inv": inv, "cor": cor, "cen": cen, "G": R.choice([1.0, 1.0, 0.5]), "gm": R.choice([1.0, 1.0, 0.25, 0.0625, 0.0]),
             "comd": comd, "vel": vel, "bodypos": bodypos, "fdt": R.choice([0.02, 0.0199999995529651, 0.04]), "rfv": rv(400.0), "w2": omega}
        ia, g, gfp = gravity(i)
        gv.append({"gtrue": H3(gtrue), "inv": inv, "cor": H3(cor), "cen": H3(cen), "G": hx(i["G"]), "gm": hx(i["gm"]), "comd": H3(comd),
                   "vel": H3(vel), "bodypos": H3(bodypos), "fdt": hx(i["fdt"]), "rfv": H3(i["rfv"]), "w2": H3(omega),
                   "ia": H3(ia), "g": H3(g), "gfp": H3(gfp)})
    out["gravity"] = gv

    # ---- F. easing multiplier sequence (cs:293-300 + StartEasing / StopEasing cs:269-292), PHYSICS_EASE on, lastMode IDLE -> TRACK_Phys
    seq = []
    gm = 1.0
    easing = False
    inc = 0.0625
    for t in range(24):
        if not easing:  # StartEasing at the first unpacked CalculateGravity after a rails -> physics switch
            gm = 0.0
            easing = True
        if easing:
            gm += inc
            if gm >= 1.0:
                easing = False
                gm = 1.0  # StopEasing
        seq.append({"gm": hx(gm), "easing": easing})
        if not easing:
            break
    out["easing"] = {"increment": hx(inc), "sequence": seq}

    # ---- G. OrbitPhysicsManager.LateUpdate
    lu = []
    for k in range(500):
        apos = rpos(R.choice([1.0, 1000.0, 100000.0]))
        n = R.randint(1, 5)
        vs = []
        for j in range(n):
            sit = R.choice(list(SIT.values()))
            rng = gen_ranges()
            wasFlying = R.random() < 0.25
            pk = get_ranges(rng, best_situation(sit, wasFlying))[2]
            up = get_ranges(rng, sit)[3]
            dist = R.choice([pk * R.choice([0.5, 0.999, 1.0, 1.001, 2.0]), up * R.choice([0.5, 0.9999, 1.0, 1.0001, 1.5]), R.uniform(0.0, 40000.0), 0.0])
            vs.append({"packed": R.random() < 0.5, "pos": near_pos(apos, dist) if dist > 0 else apos, "situation": sit, "wasFlying": wasFlying,
                       "ranges": rng, "hasDriver": R.random() < 0.9, "isActive": False})
        if R.random() < 0.1:
            vs.append({"packed": False, "pos": apos, "situation": SIT["LANDED"], "wasFlying": False, "ranges": gen_ranges(), "hasDriver": True,
                       "isActive": True})
        high = R.random() < 0.6
        maxPhys = R.choice([1.0, 2.0, 3.0, 4.0])
        rate = R.choice([1.0, 2.0, 3.0, 4.0, 5.0, 10.0, 50.0, 100.0, 1000.0, 100000.0, maxPhys, maxPhys])
        sc = {"ready": R.random() < 0.95, "started": R.random() < 0.95, "override": R.random() < 0.05, "warpHigh": high, "rate": F(rate),
              "maxPhys": F(maxPhys), "warpDrive": R.random() < 0.1, "hold": R.random() < 0.2,
              "active": {"packed": R.random() < 0.4, "pos": apos}, "vessels": vs}
        acts, ap, pk_ = late_update(sc)
        out_sc = {"ready": sc["ready"], "started": sc["started"], "override": sc["override"], "warpHigh": high, "rate": hf(sc["rate"]),
                  "maxPhys": hf(sc["maxPhys"]), "warpDrive": sc["warpDrive"], "hold": sc["hold"],
                  "active": {"packed": sc["active"]["packed"], "pos": [hf(c) for c in apos]},
                  "vessels": [{"packed": v["packed"], "pos": [hf(c) for c in v["pos"]], "situation": v["situation"], "wasFlying": v["wasFlying"],
                               "ranges": ranges_json(v["ranges"]), "hasDriver": v["hasDriver"], "isActive": v["isActive"]} for v in vs],
                  "actions": [[a, b] for a, b in acts], "activePacked": ap, "packed": pk_}
        lu.append(out_sc)
    out["late_update"] = lu

    # ---- H. Vessel.Update load / unload
    vu = []
    for k in range(500):
        apos = rpos(R.choice([1.0, 1000.0]))
        sit = R.choice(list(SIT.values()))
        rng = gen_ranges()
        ld = get_ranges(rng, sit)
        dist = R.choice([ld[0] * R.choice([0.5, 0.9999, 1.0, 1.0001, 1.5]), ld[1] * R.choice([0.5, 0.9999, 1.0, 1.0001, 1.5]), R.uniform(0, 40000.0)])
        sc = {"pos": near_pos(apos, dist), "apos": apos, "situation": sit, "ranges": rng, "loaded": R.random() < 0.5}
        acts, loaded = vessel_update(sc)
        vu.append({"pos": [hf(c) for c in sc["pos"]], "apos": [hf(c) for c in apos], "situation": sit, "ranges": ranges_json(rng), "loaded": sc["loaded"],
                   "actions": acts, "finalLoaded": loaded})
    out["vessel_update"] = vu

    # ---- I. Vessel.CheckKill
    ck = []
    for k in range(600):
        apos = rpos(R.choice([1.0, 1000.0]))
        sit = R.choice(list(SIT.values()))
        rng = gen_ranges()
        wasFlying = R.random() < 0.3
        pk = get_ranges(rng, best_situation(sit, wasFlying))[2]
        dist = R.choice([pk * R.choice([0.5, 0.99999, 1.0, 1.00001, 2.0]), R.uniform(0.0, 40000.0)])
        sc = {"ready": R.random() < 0.95, "packed": R.random() < 0.8, "isActive": R.random() < 0.1, "override": R.random() < 0.05,
              "landedOrSplashed": R.random() < 0.2, "pressureAbove1": R.random() < 0.5, "sceneFlight": R.random() < 0.9, "hasActive": R.random() < 0.9,
              "ranges": rng, "situation": sit, "wasFlying": wasFlying, "pos": near_pos(apos, dist), "apos": apos,
              "holdExpired": R.random() < 0.8, "terrainAltitude": R.choice([-1.0, 0.0, 10.0, 2500.0]), "loaded": R.random() < 0.5}
        low = [-1000.0, -251.0, -250.0, -249.0, -0.2, -0.1, 0.0, 5.0, 2500.0, 2499.0, 10.0, 9.0]
        # the kspcore test builds the vessel at Kerbin (101 kPa below 25 km: pressure above 1 kPa) when pressureAbove1, else at the airless Mun
        sc["altitude"] = R.choice(low) if sc["pressureAbove1"] else R.choice(low + [1e5, 80000.0])
        if not sc["hasActive"]:
            sc["isActive"] = False  # `this == FlightGlobals.ActiveVessel` needs an active vessel
        res = check_kill(sc)
        ck.append({"ready": sc["ready"], "packed": sc["packed"], "isActive": sc["isActive"], "override": sc["override"],
                   "landedOrSplashed": sc["landedOrSplashed"], "pressureAbove1": sc["pressureAbove1"], "sceneFlight": sc["sceneFlight"],
                   "hasActive": sc["hasActive"], "ranges": ranges_json(rng), "situation": sit, "wasFlying": wasFlying,
                   "pos": [hf(c) for c in sc["pos"]], "apos": [hf(c) for c in apos], "holdExpired": sc["holdExpired"],
                   "terrainAltitude": hx(sc["terrainAltitude"]), "loaded": sc["loaded"], "altitude": hx(sc["altitude"]), "result": res})
    out["check_kill"] = ck

    # ---- J. situation state machine (updateSituation, no ladder)
    st = []
    for k in range(800):
        ecc = R.choice([0.0, 0.1, 0.5, 0.9, 0.9999999, 1.0, 1.5])
        soi = R.choice([84159286.0, 9e6, 2.4e6])
        radius = R.choice([600000.0, 200000.0, 261600.0])
        depth = R.choice([70000.0, 0.0, 5000.0])
        sma = soi * R.choice([0.3, 0.5, 0.99999, 1.0, 1.00001, 2.0]) / (1.0 + ecc)
        if R.random() < 0.3:  # periapsis at / around the atmosphere top
            sma = (radius + R.choice([-1000.0, 0.0, depth - 1.0, depth, depth + 1.0, 100000.0])) / (1.0 - ecc) if ecc < 1.0 else sma
        if ecc >= 1.0:
            sma = -sma
        sc = {"landed": R.random() < 0.3, "splashed": R.random() < 0.2, "situation": R.choice(list(SIT.values())),
              "srfSpeed": R.choice([0.0, 2.5, 2.5000001, 3.0, 100.0]), "easing": R.random() < 0.2, "spawning": R.random() < 0.2,
              "pressure": R.choice([0.0, 0.0, 1e-9, 50.0]), "ecc": ecc, "sma": sma, "radius": radius, "soi": soi, "depth": depth,
              "atmo": R.random() < 0.6}
        res = situation(sc)
        st.append({"landed": sc["landed"], "splashed": sc["splashed"], "situation": sc["situation"], "srfSpeed": hx(sc["srfSpeed"]),
                   "easing": sc["easing"], "spawning": sc["spawning"], "pressure": hx(sc["pressure"]), "ecc": hx(ecc), "sma": hx(sma),
                   "radius": hx(radius), "soi": hx(soi), "depth": hx(depth), "atmo": sc["atmo"], "result": res})
    out["situation"] = st
    return out


if __name__ == "__main__":
    data = build()
    with open(OUT, "w", newline="\n") as f:
        json.dump(data, f, indent=1)
        f.write("\n")
    print("wrote", OUT, {k: len(v) for k, v in data.items() if isinstance(v, list)})
