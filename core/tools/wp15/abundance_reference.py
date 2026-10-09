#!/usr/bin/env python3
"""WP-15: independent reference for ResourceMap.GetAbundance (ResourceMap.cs:405) -- test data generator.

Re-implements, in plain Python and from the decompiled C#, the pieces of the planetary resource map that decide an abundance:
KSPRandom (KSPRandom.cs), NoiseGenerator (NoiseGenerator.cs, float32 stores per the Mono float rule via numpy.float32),
ResourceMap.GenerateAbundanceSeed / GetBestResourceData / GetAbundance, ResourceUtilities.clampLat/clampLon/Deg2Rad.
It shares NO code with kspcore. Inputs are the game's own data: the RESOURCE configs of the GameData mirror
(testdata/GameData-mirror/GameData/Squad/Resources/Ore.cfg), the body list / radii of the harness golden body_catalog, the biome names of
data/kerbol_body_assets.json and the resource seed of the game's physprofile save (testdata/saves/physprofile/persistent.sfs).

Usage: python3 tools/wp15/abundance_reference.py > tests/data/wp15_abundance_reference.json
"""
import json
import math
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
ROOT = os.path.abspath(os.path.join(CORE, ".."))
INT_MAX = 2147483647


def st(x):
    """Round to float32 (a C# float store) and return the exact value as a Python float."""
    return float(np.float32(x))


def i32(x):
    x &= 0xFFFFFFFF
    return x - (1 << 32) if x & 0x80000000 else x


def trunc_int(x):
    """C# (int)double conversion for in-range values."""
    return int(math.trunc(x))


# ---- KSPRandom.cs ---------------------------------------------------------------------------------------------------------
class KSPRandom:
    def __init__(self, seed):
        self.a = [0] * 56
        num = i32(161803398 - abs(seed))  # (unchecked int arithmetic: the differences below wrap like C# ints)
        self.a[55] = num
        num2 = 1
        for i in range(1, 55):
            n3 = 21 * i % 55
            self.a[n3] = num2
            num2 = i32(num - num2)
            if num2 < 0:
                num2 = i32(num2 + INT_MAX)
            num = self.a[n3]
        for _ in range(1, 5):
            for k in range(1, 56):
                self.a[k] = i32(self.a[k] - self.a[1 + (k + 30) % 55])
                if self.a[k] < 0:
                    self.a[k] = i32(self.a[k] + INT_MAX)
        self.ia = 0
        self.ib = 31

    def sample(self):
        self.ia += 1
        if self.ia >= 56:
            self.ia = 1
        self.ib += 1
        if self.ib >= 56:
            self.ib = 1
        num = i32(self.a[self.ia] - self.a[self.ib])
        if num < 0:
            num = i32(num + INT_MAX)
        self.a[self.ia] = num
        return num * 4.6566128752458e-10

    def next_double(self):
        return self.sample()

    def next_max(self, maxv):
        return trunc_int(self.sample() * float(maxv))


# ---- NoiseGenerator.cs ----------------------------------------------------------------------------------------------------
class Noise:
    def __init__(self):
        self.A = [0, 0, 0]
        self.T = [0] * 8
        self.onesixth = st(1.0 / 6.0)
        self.onethird = st(1.0 / 3.0)
        self.s = self.u = self.v = self.w = 0.0
        self.i = self.j = self.k = 0

    @staticmethod
    def fastfloor(n):
        if not (n > 0.0):
            return trunc_int(n) - 1
        return trunc_int(n)

    def b2(self, N, B):
        return (N >> B) & 1

    def b4(self, i, j, k, B):
        return self.T[(self.b2(i, B) << 2) | (self.b2(j, B) << 1) | self.b2(k, B)]

    def shuffle(self, i, j, k):
        b = self.b4
        return b(i, j, k, 0) + b(j, k, i, 1) + b(k, i, j, 2) + b(i, j, k, 3) + b(j, k, i, 4) + b(k, i, j, 5) + b(i, j, k, 6) + b(j, k, i, 7)

    def noise(self, x, y, z):
        self.s = st((x + y + z) * self.onethird)
        self.i = self.fastfloor(st(x + self.s))
        self.j = self.fastfloor(st(y + self.s))
        self.k = self.fastfloor(st(z + self.s))
        self.s = st(float(np.float32(self.i + self.j + self.k)) * self.onesixth)
        self.u = st(x - float(np.float32(self.i)) + self.s)
        self.v = st(y - float(np.float32(self.j)) + self.s)
        self.w = st(z - float(np.float32(self.k)) + self.s)
        self.A = [0, 0, 0]
        u, v, w = self.u, self.v, self.w
        # C#: `(!(u >= w)) ? ((v >= w) ? 1 : 2) : ((!(u >= v)) ? 1 : 0)`
        num = ((1 if v >= w else 2) if not (u >= w) else (1 if not (u >= v) else 0))
        num2 = ((1 if v < w else 2) if not (u < w) else (1 if not (u < v) else 0))
        a = self.kay(num)
        b = self.kay(3 - num - num2)
        c = self.kay(num2)
        d = self.kay(0)
        return st(a + b + c + d)

    def kay(self, a):
        A = self.A
        self.s = st(float(np.float32(A[0] + A[1] + A[2])) * self.onesixth)
        num = st(self.u - float(np.float32(A[0])) + self.s)
        num2 = st(self.v - float(np.float32(A[1])) + self.s)
        num3 = st(self.w - float(np.float32(A[2])) + self.s)
        num4 = st(st(0.6) - num * num - num2 * num2 - num3 * num3)
        num5 = self.shuffle(self.i + A[0], self.j + A[1], self.k + A[2])
        A[a] += 1
        if num4 < 0.0:
            return 0.0
        num6 = (num5 >> 5) & 1
        num7 = (num5 >> 4) & 1
        num8 = (num5 >> 3) & 1
        num9 = (num5 >> 2) & 1
        num10 = num5 & 3
        n12 = {2: num2, 1: num}.get(num10, num3)
        n14 = {2: num3, 1: num2}.get(num10, num)
        n16 = {2: num, 1: num3}.get(num10, num2)
        n12 = (0.0 - n12) if num6 == num8 else n12
        n14 = (0.0 - n14) if num6 == num7 else n14
        n16 = (0.0 - n16) if num6 != (num7 ^ num8) else n16
        num4 = st(num4 * num4)
        inner = n12 + ((n14 + n16) if num10 == 0 else (n14 if num9 == 0 else n16))
        return st(8.0 * num4 * num4 * inner)


# ---- resource configs (ConfigNode text of Ore.cfg) -------------------------------------------------------------------------
def parse_nodes(text):
    """Minimal parser of the cfg grammar: returns a list of (name, values dict, children)."""
    text = re.sub(r"//.*", "", text)
    tokens = re.findall(r"\{|\}|[^\s{}=]+(?:\s*=\s*[^\n{}]*)?", text)
    stack = [("ROOT", {}, [])]
    pending = None
    for t in tokens:
        if t == "{":
            node = (pending, {}, [])
            stack[-1][2].append(node)
            stack.append(node)
            pending = None
        elif t == "}":
            stack.pop()
        elif "=" in t:
            k, v = t.split("=", 1)
            stack[-1][1][k.strip()] = v.strip()
        else:
            pending = t
    return stack[0][2]


def load_resource_configs():
    path = os.path.join(CORE, "testdata", "GameData-mirror", "GameData", "Squad", "Resources", "Ore.cfg")
    out = {"BIOME_RESOURCE": [], "PLANETARY_RESOURCE": [], "GLOBAL_RESOURCE": []}
    for name, vals, children in parse_nodes(open(path, encoding="utf-8").read()):
        if name in out:
            dist = {}
            for cn, cv, _ in children:
                if cn == "Distribution":
                    dist = {k: st(float(v)) for k, v in cv.items()}
            out[name].append({
                "ResourceName": vals.get("ResourceName"),
                "PlanetName": vals.get("PlanetName"),
                "BiomeName": vals.get("BiomeName"),
                "ResourceType": int(vals.get("ResourceType", "0")),
                "Distribution": dist,
            })
    return out


def best(configs):
    d0 = configs[0]["Distribution"]
    r = {k: d0.get(k, 0.0) for k in ("Variance", "Dispersal", "PresenceChance", "MinAbundance", "MaxAbundance")}
    r.update({"MinAltitude": 0.0, "MaxAltitude": 0.0, "MinRange": 0.0, "MaxRange": 0.0})
    for c in configs:
        d = {k: c["Distribution"].get(k, 0.0) for k in ("PresenceChance", "MinAbundance", "MaxAbundance", "Variance", "Dispersal", "MinAltitude", "MaxAltitude", "MinRange", "MaxRange")}
        if d["PresenceChance"] > 0 and d["PresenceChance"] > r["PresenceChance"]:
            r["PresenceChance"] = d["PresenceChance"]
        if d["MinAbundance"] > 0 and d["MinAbundance"] < r["MinAbundance"]:
            r["MinAbundance"] = d["MinAbundance"]
        if d["MaxAbundance"] > 0 and d["MaxAbundance"] > r["MaxAbundance"]:
            r["MaxAbundance"] = d["MaxAbundance"]
        if d["Variance"] > 0 and d["Variance"] > r["Variance"]:
            r["Variance"] = d["Variance"]
        if d["Dispersal"] > 0 and d["Dispersal"] > r["Dispersal"]:
            r["Dispersal"] = d["Dispersal"]
        for lo, hi in (("MinAltitude", "MaxAltitude"), ("MinRange", "MaxRange")):
            if d[lo] > 0 and d[lo] < r[lo]:
                r[lo] = d[lo]
            if d[hi] > 0 and d[hi] > r[hi]:
                r[hi] = d[hi]
    return r


def clamp_lat(lat):
    return math.fmod(lat + 180.0 + 90.0, 180.0) - 90.0


def clamp_lon(lon):
    return math.fmod(lon + 360.0 + 180.0, 360.0) - 180.0


def deg2rad(d):
    return d * math.pi / 180.0


def abundance(cfgs, seed, body, bodyId, resource, rtype, biome, lat_deg, lon_deg, check_lock, exclude_variance, unlocked, difficulty):
    num = deg2rad(clamp_lat(lat_deg))
    num2 = deg2rad(clamp_lon(lon_deg))
    flag = True
    text = biome
    if rtype == 0:
        flag = unlocked
        if not check_lock:
            flag = True
        if exclude_variance:
            flag = False
    # GenerateAbundanceSeed (int arithmetic wraps)
    def len_times_second(s):
        return len(s) * (ord(s[1]) if len(s) > 1 else 0)
    r = i32(seed * (bodyId + 1))
    r = i32(r * len_times_second(resource))
    r = i32(r * len_times_second(body))
    num3 = i32(r + i32(len_times_second(text) * (rtype + 1)))
    biome_cfg = [c for c in cfgs["BIOME_RESOURCE"] if c["PlanetName"] == body and c["BiomeName"] == text and c["ResourceName"] == resource and c["ResourceType"] == rtype]
    planet_cfg = [c for c in cfgs["PLANETARY_RESOURCE"] if c["PlanetName"] == body and c["ResourceName"] == resource and c["ResourceType"] == rtype]
    global_cfg = [c for c in cfgs["GLOBAL_RESOURCE"] if c["ResourceName"] == resource and c["ResourceType"] == rtype]
    if biome_cfg:
        bd = best(biome_cfg)
        num3 = i32(num3 * 2)
    elif planet_cfg:
        bd = best(planet_cfg)
        num3 = i32(num3 * 3)
    else:
        if not global_cfg:
            return 0.0
        bd = best(global_cfg)
        num3 = i32(num3 * 4)
    rnd = KSPRandom(num3)
    value = [rnd.next_double(), rnd.next_double()] + [float(rnd.next_max(100)) for _ in range(8)] + [rnd.next_double(), rnd.next_double()]
    if not (float(np.float32(trunc_int(value[0] * 100.0))) <= bd["PresenceChance"] * difficulty):
        return 0.0
    num4 = trunc_int(bd["MinAbundance"] * 1000.0)
    num5 = trunc_int(bd["MaxAbundance"] * 1000.0)
    if num4 > num5:
        num5 = num4 + 1
    num6 = num5 - num4
    num7 = st(float(np.float32(trunc_int(value[1] * float(num6)) + num4)) / 1000.0 * difficulty)
    if flag and not exclude_variance and rtype == 0:
        gen = Noise()
        gen.T = [trunc_int(value[2 + m]) for m in range(8)]
        x = st(st(num) * bd["Dispersal"])
        y = st(st(num2) * bd["Dispersal"])
        z = st(st(value[10] * 100.0) / 100.0)
        n8 = gen.noise(x, y, z)
        n9 = st(n8 * (bd["Variance"] / 100.0))
        num7 = st(num7 + n9)
        if num7 < 0.0:
            num7 = 0.0
    elif rtype == 0:
        num7 = st(num7 / 1.5)
    if num7 <= 1e-9:
        return 0.0
    return st(num7 / 100.0)


def main():
    cfgs = load_resource_configs()
    cat = json.load(open(os.path.join(ROOT, "harness", "golden", "body_catalog", "body_catalog.json")))
    bt = [t for t in cat["tables"] if t["name"] == "bodies"][0]
    cols = [c["name"] for c in bt["columns"]]
    bodies = {r[cols.index("name")]: r[cols.index("flightGlobalsIndex")] for r in bt["rows"]}
    assets = json.load(open(os.path.join(CORE, "data", "kerbol_body_assets.json")))
    biomes = {}
    for b in assets["bodies"]:
        bm = b.get("biomeMap")
        biomes[b["bodyName"]] = [a["name"] for a in bm["attributes"]] if bm else []
    save = open(os.path.join(CORE, "testdata", "saves", "physprofile", "persistent.sfs"), encoding="utf-8").read()
    seed = int(re.search(r"GameSeed = (-?\d+)", save).group(1))
    latlons = [(0.0, 0.0), (28.5, -80.0), (-0.1, -74.5), (45.0, 120.0), (-89.0, 179.9), (90.0, -180.0), (13.37, 270.0), (-33.3, 540.5), (200.0, -10.0)]
    cases = []
    for body in ("Kerbin", "Mun", "Minmus", "Duna", "Eve", "Jool", "Sun", "Eeloo"):
        for biome in biomes.get(body, []) or ["LANDED"]:
            for k, (lat, lon) in enumerate(latlons):
                for (check_lock, excl, unlocked, diff) in ((False, False, False, 1.0), (True, False, False, 1.0), (True, False, True, 1.25), (False, True, False, 0.5)):
                    if k % 3 != 0 and diff != 1.0:
                        continue
                    a = abundance(cfgs, seed, body, bodies[body], "Ore", 0, biome, lat, lon, check_lock, excl, unlocked, st(diff))
                    bits = int(np.float32(a).view(np.uint32))
                    cases.append({"body": body, "bodyId": bodies[body], "biome": biome, "lat": lat, "lon": lon, "checkForLock": check_lock,
                                  "excludeVariance": excl, "unlocked": unlocked, "difficulty": diff, "bits": bits, "value": a})
    # ResourceCache.LoadAbundanceCache + ResourceMap.GetResourceByName: per body, one summary per biome (ExcludeVariance, no lock check, lat/lon 0,
    # difficulty 1), then the float32 mean over the body's summaries of the resource (ResourceMap.cs:589)
    items = {}
    for body in ("Kerbin", "Mun", "Minmus", "Duna", "Eve"):  # (bodies without a biome map get no planetary summaries)
        vals = [abundance(cfgs, seed, body, bodies[body], "Ore", 0, biome, 0.0, 0.0, False, True, False, st(1.0)) for biome in biomes[body]]
        total = 0.0
        for v in vals:
            total = st(total + v)
        mean = st(total / float(np.float32(len(vals))))
        items[body] = {"bodyId": bodies[body], "biomes": biomes[body],
                       "bits": [int(np.float32(v).view(np.uint32)) for v in vals], "fractionBits": int(np.float32(mean).view(np.uint32))}
    json.dump({"description": "ResourceMap.GetAbundance reference (tools/wp15/abundance_reference.py): Ore, HarvestTypes.Planetary", "seed": seed,
               "cases": cases, "bodyItems": items}, sys.stdout, indent=0)


if __name__ == "__main__":
    main()
