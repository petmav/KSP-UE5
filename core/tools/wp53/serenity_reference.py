#!/usr/bin/env python3
"""WP-53 independent reference: Breaking Ground deployed-science and ROC maths.

No kspcore code. Everything is written from the decompiled C# (Assembly-CSharp):
  Expansions/Serenity/DeployedScience/Runtime/DeployedScienceCluster.cs   (UpdatePowerState, SolarPanelUnitsProduced)
  Expansions/Serenity/DeployedScience/Runtime/DeployedScience.cs          (GetDiminishingReturnsMultiplier, SetupDiminishingReturns defaults)
  Expansions/Serenity/DeployedScience/Runtime/DeployedScienceExperiment.cs (CalculateScience, TimeToSendStoredData, ProcessSeismicEvent maths)
  LandClassROC.cs (AddScatterMeshController, CreateScatterMesh), StringExtensions.cs (GetHashCode_Net35),
  Expansions/Serenity/Contracts/{DeployedScience,ROCScienceArm,ROCScienceRetrieval}Contract.cs (the KSPRandom picks),
  UnityEngine.Random (harness/captures/README.md C2: xorshift128, fitted against the in-game capture), KSPRandom.cs.

C# `float` arithmetic follows the Mono rule (core/README.md): an expression is evaluated in double and rounded to float32 (numpy) only where
the C# stores, passes, returns or casts it. Python floats below hold float32-exact values for C# floats unless a value is a C# double.

Fixtures that are NOT KSP data: the GameData mirror has no SquadExpansion/Serenity (no ROC_DEFINITION, no deployed-science part cfgs, no
DEPLOYEDSCIENCE node), so every input row below (part power tables, experiment values, synthetic quad meshes, ROC type values) is generated
here from a fixed seed; only the code under test is the game's.

Usage: python -I core/tools/wp53/serenity_reference.py        -> core/tests/data/wp53_reference.json
"""
import json
import math
import os

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(CORE, "tests", "data", "wp53_reference.json")

f32 = np.float32
M32 = 0xFFFFFFFF


def F(x):
    """round a C# float expression (evaluated in double) to float32 at a store/argument/return boundary"""
    return float(f32(x))


def i32(x):
    x &= M32
    return x - (1 << 32) if x >= 1 << 31 else x


def to_int(x):
    """(int)double on x64: truncation, NaN / out of range -> int.MinValue"""
    if not (x > -2147483649.0 and x < 2147483648.0):
        return -2147483648
    return int(x)


def hash35(s):
    """StringExtensions.GetHashCode_Net35 (StringExtensions.cs:3)"""
    n = 0
    i = 0
    L = len(s)
    while i < L - 1:
        n2 = i32((n << 5) - n + ord(s[i]))
        n = i32((n2 << 5) - n2 + ord(s[i + 1]))
        i += 2
    if i < L:
        n = i32((n << 5) - n + ord(s[i]))
    return n


# ---------------------------------------------------------------------------------------------------------------- UnityEngine.Random (C2)
class Unity:
    def __init__(self, seed):
        s0 = seed & M32
        s1 = (s0 * 1812433253 + 1) & M32
        s2 = (s1 * 1812433253 + 1) & M32
        s3 = (s2 * 1812433253 + 1) & M32
        self.s = [s0, s1, s2, s3]

    def next(self):
        s0, s1, s2, s3 = self.s
        t = (s0 ^ (s0 << 11)) & M32
        t ^= t >> 8
        n3 = (s3 ^ (s3 >> 19) ^ t) & M32
        self.s = [s1, s2, s3, n3]
        return n3

    def value(self):
        return float(f32(self.next() & 0x7FFFFF) / f32(8388607.0))

    def range_f(self, mn, mx):
        v = f32(self.value())
        return float(f32(v * f32(mn)) + f32(f32(1.0) - v) * f32(mx))

    def range_i(self, mn, mx):
        if mx > mn:
            return mn + (self.next() % ((mx - mn) & M32))
        if mx < mn:
            return mn - (self.next() % ((mn - mx) & M32))
        return mn

    def rotation(self):
        # Random.rotation: 4 x Range(-1, 1) (only the draw count matters here)
        for _ in range(4):
            self.range_f(-1.0, 1.0)


# ---------------------------------------------------------------------------------------------------------------- KSPRandom (KSPRandom.cs)
class KSPRandom:
    def __init__(self, seed):
        num = i32(161803398 - abs(seed))
        sa = [0] * 56
        sa[55] = num
        num2 = 1
        for i in range(1, 55):
            num3 = 21 * i % 55
            sa[num3] = num2
            num2 = i32(num - num2)
            if num2 < 0:
                num2 = i32(num2 + 2147483647)
            num = sa[num3]
        for _ in range(1, 5):
            for k in range(1, 56):
                sa[k] = i32(sa[k] - sa[1 + (k + 30) % 55])
                if sa[k] < 0:
                    sa[k] = i32(sa[k] + 2147483647)
        self.sa = sa
        self.a = 0
        self.b = 31

    def sample(self):
        self.a += 1
        if self.a >= 56:
            self.a = 1
        self.b += 1
        if self.b >= 56:
            self.b = 1
        num = i32(self.sa[self.a] - self.sa[self.b])
        if num < 0:
            num = i32(num + 2147483647)
        self.sa[self.a] = num
        return num * 4.6566128752458e-10

    def next_range(self, lo, hi):
        num = (hi - lo) & M32
        if num <= 1:
            return lo
        return i32(int(self.sample() * float(num)) + lo)


# ---------------------------------------------------------------------------------------------------------------- DeployedScienceCluster.UpdatePowerState
def update_power_state(parts, controller_id, is_powered, controller_enabled):
    """parts: dicts {id, enabled, onGround, loaded, solar, produced, actual, required, isSolarListed}"""
    power_required = 0
    power_available = 0
    flag = False
    for p in parts:
        if p["enabled"] and p["onGround"]:
            if p["loaded"]:
                flag = True
            if p["solar"]:
                power_available += p["actual"] if p["loaded"] else p["produced"]
            else:
                power_available += p["produced"]
            power_required += p["required"]
        if p["id"] == controller_id:
            controller_enabled = p["enabled"]
    flag2 = controller_enabled
    solar_units = sum(p["produced"] for p in parts if p["isSolarListed"])
    if flag:
        if power_available >= power_required:
            partial, mult = False, 1.0
        else:
            partial, mult, flag2 = True, 0.0, False
    elif power_available < power_required:
        partial, mult, flag2 = True, 0.0, False
    elif power_available - solar_units < power_required:
        partial, mult = True, 0.5
    else:
        partial, mult = False, 1.0
    changed = flag2 != is_powered
    return {
        "powerRequired": power_required,
        "powerAvailable": power_available,
        "partialPower": partial,
        "partialMult": mult,
        "isPowered": flag2,
        "controllerEnabled": controller_enabled,
        "solarUnits": solar_units,
        "powerChanged": changed,
    }


def power_cases(rng):
    cases = []
    for c in range(600):
        n = int(rng.integers(1, 7))
        parts = []
        controller = 100
        for i in range(n):
            solar = bool(rng.integers(0, 2))
            produced = int(rng.integers(0, 12)) if (solar or rng.integers(0, 3) == 0) else 0
            parts.append({
                "id": 100 + i,
                "enabled": bool(rng.integers(0, 4) != 0),
                "onGround": bool(rng.integers(0, 6) != 0),
                "loaded": bool(rng.integers(0, 2)),
                "solar": solar,
                "produced": produced,
                "actual": int(rng.integers(0, produced + 1)) if solar else 0,
                "required": int(rng.integers(0, 8)),
                "isSolarListed": solar,
            })
        is_powered = bool(rng.integers(0, 2))
        ce = bool(rng.integers(0, 2))
        r = update_power_state(parts, controller, is_powered, ce)
        cases.append({"parts": parts, "controllerId": controller, "isPowered": is_powered, "controllerEnabled": ce, "expect": r})
    return cases


# ---------------------------------------------------------------------------------------------------------------- DeployedScience.GetDiminishingReturnsMultiplier
def dim_mult(keys, values, n):
    result = 1.0
    flag = False
    for i in range(len(keys)):
        if keys[i] <= n:
            if keys[i] == n:
                flag = True
                result = values[i]
            elif not flag:
                result = values[i]
    return result


def dimret_cases():
    tables = [
        ([1, 2, 3, 4, 5], [F(1.0), F(0.2), F(0.35), F(0.45), F(0.5)]),  # SetupDiminishingReturns(null)
        ([3, 1, 2], [F(0.5), F(1.0), F(0.25)]),  # insertion order differs from key order
        ([2, 4], [F(0.3), F(0.6)]),
        ([], []),
    ]
    out = []
    for keys, values in tables:
        out.append({"keys": keys, "values": values, "results": [dim_mult(keys, values, n) for n in range(0, 9)]})
    return out


# ---------------------------------------------------------------------------------------------------------------- DeployedScienceExperiment.CalculateScience / TimeToSendStoredData
def time_to_send(seismic, has_subject, total, limit, stored, subject_value):
    if seismic:
        return True
    if not has_subject:
        return False
    if total >= limit:
        return True
    if stored * subject_value / limit * 100.0 >= 10.0:
        return True
    return False


def science_cases(rng):
    out = []
    for _ in range(60):
        sv = F(rng.uniform(0.5, 40.0))
        limit = F(rng.uniform(1.0, 60.0))
        subj_value = F(rng.uniform(0.2, 4.0))
        dim = F(rng.choice([1.0, 0.2, 0.35, 0.45, 0.5]))
        ut0 = float(rng.integers(0, 2_000_000))
        ut = ut0
        last = ut0
        total = 0.0
        stored = 0.0
        enabled = True
        steps = []
        for _ in range(45):
            dt = float(rng.choice([0.5, 30.0, 60.0, 600.0, 3600.0, 7200.0, float(rng.uniform(1.0, 5000.0))]))
            ut += dt
            solar = F([1.0, 0.5, 0.0][int(rng.integers(0, 3))])
            generated = None
            if enabled and not (total >= limit):
                num = F(sv / 60.0 / 60.0 * F(ut - last) * dim * solar)
                if total + num > limit:
                    num = F(limit - total)
                total = F(total + num)
                stored = F(stored + num / subj_value)
                generated = num
            else:
                if total >= limit and enabled:
                    enabled = False
            last = ut
            send = time_to_send(False, True, total, limit, stored, subj_value)
            transmit = bool(send and stored > 0.0 and rng.integers(0, 3) == 0)
            steps.append({"ut": ut, "solar": solar, "generated": generated, "total": total, "stored": stored, "enabled": enabled, "send": send, "transmit": transmit})
            if transmit:
                stored = 0.0
        out.append({"scienceValue": sv, "limit": limit, "subjectValue": subj_value, "dim": dim, "startUT": ut0, "steps": steps})
    return out


# ---------------------------------------------------------------------------------------------------------------- ProcessSeismicEvent maths
def seismic_case(rng):
    n = int(rng.integers(1, 6))
    parts = []
    tiny = rng.integers(0, 4) == 0
    for _ in range(n):
        parts.append({"mass": F(rng.uniform(0.0005, 0.01) if tiny else rng.uniform(0.05, 30.0)), "velocity": F(rng.uniform(0.1, 1.5) if tiny else rng.uniform(0.5, 600.0)),
                      "impactDistance": float(rng.uniform(0.0, 3.0e5)), "mult": F(rng.uniform(0.0, 1.0))})
    rate = F(rng.choice([1000000.0, 4.184e10, 782000000.0, 2.02e9, 3.4028234663852886e+38]))
    min_e = F(3000.0)
    dim = F(rng.choice([1.0, 0.2, 0.35]))
    solar = F(rng.choice([1.0, 0.5]))
    data_scale = F(rng.uniform(0.5, 3.0))
    limit = F(rng.uniform(5.0, 60.0))
    rate_mod = F(rng.choice([5.0, 100.0, 66.0, 150.0]))
    subj_value = F(rng.uniform(0.2, 4.0))
    total0 = F(rng.uniform(0.0, 1.0) * limit)
    stored0 = F(rng.uniform(0.0, 3.0))
    total_joules = 0.0
    mass_sum = 0.0
    dist_mult = 1.0
    for p in parts:
        total_joules += (0.5 * (p["mass"] * 1000.0)) * (p["velocity"] ** 2.0)
        mass_sum = F(mass_sum + p["mass"])
        dist_mult = p["mult"]
    out = {"parts": parts, "rate": rate, "minEnergy": min_e, "dim": dim, "solar": solar, "dataScale": data_scale, "limit": limit, "rateMod": rate_mod,
           "subjectValue": subj_value, "total0": total0, "stored0": stored0, "totalJoules": total_joules, "massSum": mass_sum, "distMult": dist_mult}
    if total_joules < min_e:
        out["ignored"] = True
        return out
    out["ignored"] = False
    num3 = F(dim * solar * data_scale * limit * (F(total_joules) / rate) * (rate_mod / 100.0))
    num3 = F(num3 * F(dist_mult))
    if total0 + num3 > limit:
        num3 = F(limit - total0)
    total = F(total0 + num3)
    stored = F(stored0 + num3 / subj_value)
    out.update({"num3": num3, "total": total, "stored": stored})
    return out


# ---------------------------------------------------------------------------------------------------------------- ROC spawn (LandClassROC)
def mround(x):
    return F(np.rint(np.float64(x)))


def v_add(a, b):
    return [F(a[0] + b[0]), F(a[1] + b[1]), F(a[2] + b[2])]


def v_mul(a, d):
    return [F(a[0] * d), F(a[1] * d), F(a[2] * d)]


def v_neg(a):
    return [-a[0], -a[1], -a[2]]


def v_mag(a):
    return F(math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]))


def v_norm(a):
    m = v_mag(a)
    if m > 9.99999974737875e-06:
        return [F(a[0] / m), F(a[1] / m), F(a[2] / m)]
    return [0.0, 0.0, 0.0]


def roc_seed(game_seed, quad_name, roc_name, freq, area):
    quad_seed = i32(game_seed + hash35(quad_name) + hash35(roc_name))
    u = Unity(quad_seed)
    num = F(freq * area)
    if num == 0.0:
        num2 = math.inf
    else:
        num2 = mround(F(1.0 / num))
    roc_n = to_int(num)
    num2_i = to_int(num2) if num2 != math.inf else -2147483648
    num3 = u.range_i(1, num2_i)
    if num < 1.0 and num > 0.0 and num3 == 1:
        roc_n = 1
    return {"quadSeed": quad_seed, "num": num, "num2": num2 if num2 != math.inf else None, "rocN": roc_n, "num3": num3, "maxROCs": i32(roc_n * 5)}


def make_verts(rng, side=15):
    verts = []
    normals = []
    r = 600000.0
    for z in range(side):
        for x in range(side):
            lat = 0.002 * (z - side / 2) + float(rng.uniform(-1e-4, 1e-4))
            lon = 0.002 * (x - side / 2) + float(rng.uniform(-1e-4, 1e-4))
            h = r + float(rng.uniform(-30.0, 30.0))
            v = [F(h * math.sin(lon) * math.cos(lat)), F(h * math.cos(lon) * math.cos(lat)), F(h * math.sin(lat))]
            verts.append(v)
            normals.append(v_norm(v))
    return verts, normals


def roc_place(game_seed, quad_name, roc_name, obj_name, freq, area, verts, normals, depth, size_y, extents_y, random_depth, surface_relative, position_planet,
              used_in, side=15):
    """CreateScatterMesh for the count the seeding produced; returns the per-ROC records"""
    seed = roc_seed(game_seed, quad_name, roc_name, freq, area)
    quad_seed = seed["quadSeed"]
    count = seed["rocN"]
    u = Unity(quad_seed)
    used = [tuple(p) for p in used_in]
    recs = []
    for roc_loop in range(count):
        num2 = -1
        num3 = -1
        num4 = F(depth * size_y)
        num5 = F(u.value() * 360.0)
        u.rotation()
        num6 = u.range_f(0.1, num4)
        while num3 == num2:
            num7 = u.range_i(1, side - 1 + 1)  # PQS.cacheRes + 1 = cacheSideVertCount
            num8 = u.range_i(1, side - 1 + 1)
            x = num7 + u.range_i(-1, 1)
            z = num8 + u.range_i(-1, 1)
            num3 = num8 * side + num7
            num2 = z * side + x
        idx = num3 if u.range_i(0, 1) == 0 else num2
        pos = list(verts[idx])
        rec = {"loop": roc_loop, "index": idx, "vertex": pos}
        key = (mround(pos[0]), mround(pos[1]), mround(pos[2]))
        if key in used:
            rec["duplicate"] = True
            recs.append(rec)
            continue
        rec["duplicate"] = False
        used.append(key)
        rec["position"] = list(key)
        up = v_norm(v_add(pos, position_planet)) if surface_relative else v_norm(pos)
        pos = v_add(pos, v_mul(up, extents_y))
        if not random_depth:
            pos = v_add(pos, v_mul(v_neg(up), num4))
        else:
            pos = v_add(pos, v_mul(v_neg(up), num6))
        rec["localPosition"] = pos
        rec["rocID"] = i32(game_seed + hash35(obj_name) + roc_loop + to_int(v_mag(pos)))
        recs.append(rec)
    return {"seed": seed, "recs": recs, "used": [list(p) for p in used]}


def roc_cases(rng):
    seeds = []
    names = ["_quad_0_1", "Kerbin_a3f", "Mun0_4", "x", "", "Duna_quad_deep_7"]
    rocs = ["ROC_DunaMound", "ROC_MunRock", "ROC_BoppyPlant", "A"]
    for gs in [0, 1, 424242, -5, 2147483000]:
        for qn in names:
            for rn in rocs:
                for freq, area in [(0.05, 1.0), (0.4, 2.2), (3.0, 1.7), (0.0, 1.0), (1.0, 0.0), (12.5, 0.9), (0.25, 3.0)]:
                    s = roc_seed(gs, qn, rn, F(freq), F(area))
                    seeds.append({"gameSeed": gs, "quad": qn, "roc": rn, "freq": F(freq), "area": F(area), "expect": s})
    places = []
    for c in range(40):
        verts, normals = make_verts(rng)
        gs = int(rng.integers(-1000, 100000))
        qn = "Kerbin_%x" % int(rng.integers(0, 1 << 24))
        rn = ["ROC_DunaMound", "ROC_MunRock"][int(rng.integers(0, 2))]
        obj = rn + "(Clone)"
        freq = F(rng.choice([0.3, 1.0, 2.5, 6.0]))
        area = F(rng.uniform(0.5, 3.0))
        depth = F(rng.uniform(0.1, 1.0))
        size_y = F(rng.uniform(0.2, 4.0))
        extents_y = F(size_y / 2.0)
        random_depth = bool(rng.integers(0, 2))
        surface_relative = bool(rng.integers(0, 2))
        pp = [F(rng.uniform(-500, 500)), F(rng.uniform(-500, 500)), F(rng.uniform(-500, 500))]
        used = []
        if rng.integers(0, 2):
            # a pre-existing dedupe entry that equals the rounded position of some vertex
            used = [[mround(verts[int(i)][0]), mround(verts[int(i)][1]), mround(verts[int(i)][2])] for i in rng.integers(0, len(verts), 3)]
        res = roc_place(gs, qn, rn, obj, freq, area, verts, normals, depth, size_y, extents_y, random_depth, surface_relative, pp, used)
        places.append({"gameSeed": gs, "quad": qn, "roc": rn, "obj": obj, "freq": freq, "area": area, "depth": depth, "sizeY": size_y, "extentsY": extents_y,
                       "randomDepth": random_depth, "surfaceRelative": surface_relative, "positionPlanet": pp, "verts": verts,
                       "usedIn": used, "expect": res})
    return seeds, places


# ---------------------------------------------------------------------------------------------------------------- contract picks (KSPRandom)
def pick_cases(rng):
    out = []
    for _ in range(120):
        seed = int(rng.integers(-2_000_000_000, 2_000_000_000))
        n = int(rng.integers(1, 40))
        biome_count = int(rng.integers(1, 9))
        g = KSPRandom(seed)
        index = g.next_range(0, n - 1)
        want = 1
        if biome_count >= 5:
            want = 3
        elif biome_count >= 3:
            want = 2
        picked = []
        for _ in range(10):
            if len(picked) >= want:
                break
            i2 = g.next_range(0, biome_count - 1)
            if i2 not in picked:
                picked.append(i2)
        out.append({"seed": seed, "candidates": n, "biomeCount": biome_count, "index": index, "biomes": picked})
    return out


def main():
    rng = np.random.default_rng(53)
    seeds, places = roc_cases(rng)
    ref = {
        "note": "WP-53 reference (tools/wp53/serenity_reference.py): independent of kspcore; inputs are generated fixtures, not Serenity game data (absent from the mirror)",
        "power": power_cases(rng),
        "dimret": dimret_cases(),
        "science": science_cases(rng),
        "seismic": [seismic_case(rng) for _ in range(160)],
        "rocSeeds": seeds,
        "rocPlace": places,
        "picks": pick_cases(rng),
    }
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        json.dump(ref, f, separators=(",", ":"), allow_nan=False)
        f.write("\n")
    print("wrote", OUT, os.path.getsize(OUT), "bytes")


if __name__ == "__main__":
    main()
