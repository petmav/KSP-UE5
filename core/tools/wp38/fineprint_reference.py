#!/usr/bin/env python3
"""WP-38 independent reference: FinePrint utilities (ProgressUtilities, CelestialUtilities, SystemUtilities), KSPRandom draw sequences, orbit
generation (OrbitUtilities.GenerateOrbit element draws), waypoint / random surface position maths and the ExplorationContract
(ProgressTrackingParameter) generation.

No kspcore code. Inputs are the game's own data: GameData-mirror Contracts.cfg / Agents.cfg / StoryDefs.cfg / dictionary.cfg
(core/testdata), harness/inputs/Physics.cfg, the body table of harness/golden/serialized/celestial_bodies.serialized.json, and the decompiled C#
(FinePrint/Contracts/Parameters/ProgressTrackingParameter.cs is PARSED for its localisation-tag switch tables; every algorithm is re-implemented).
The Mono float rule of core/README.md applies: a C# float expression is evaluated in double and rounded to float32 only where the C# stores,
passes, returns or casts it (F()).  Reuses the independent WP-37 reference helpers (cfg parser, Localizer, UnityEngine.Random, Contract.Set*).

Fixtures that are NOT KSP data (STATUS.md "needs data"):
  * GameVariables contract factors / destination weight: the C# field initialisers (prefab not in the repo), via tools/wp37.
  * Career multiplier sets (WP-37's two sets), the progress scenarios, the generation GUIDs.
  * Facility levels: no ScenarioUpgradeableFacilities scenario exists, GetFacilityLevel returns 1.0 for every facility (the C# default).
  * PQS: no terrain sphere is attached, so TerrainAltitude is 0 and CelestialUtilities.GetHighestPeak is 0.
  * ResearchAndDevelopment.ResearchedValidContractObjectives("Dock") is false (R&D not ported), so dockUnlocked is false.

Usage: python -I core/tools/wp38/fineprint_reference.py   -> core/tests/data/wp38_reference.json
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
if not os.path.isdir(os.path.join(ROOT, "harness", "golden")):
    ROOT = r"C:\Projects\KSP-UE5"
sys.path.insert(0, os.path.join(CORE, "tools", "wp37"))
import contract_reference as w37  # noqa: E402  (independent of kspcore; cfg parser, Localizer, UnityEngine.Random, Contract.Set*)

OUT = os.path.join(CORE, "tests", "data", "wp38_reference.json")
CS = os.path.join(ROOT, "decompiled", "KSP-Decompiled", "Assembly-CSharp", "FinePrint", "Contracts", "Parameters", "ProgressTrackingParameter.cs")
BODIES_JSON = os.path.join(ROOT, "harness", "golden", "serialized", "celestial_bodies.serialized.json")
PHYSICS = os.path.join(ROOT, "harness", "inputs", "Physics.cfg")

# the WP-37 reference maps CREWTRANSFER to the node id "CrewTransfer"; the C# node id is base("CelestialBodyTransfer") (CelestialBodyTransfer.cs:11)
w37.NODE_ID["CREWTRANSFER"] = "CelestialBodyTransfer"

F = w37.F
f32 = np.float32
M32 = 0xFFFFFFFF
LOC = w37.LOC
BODY_LIST = w37.BODY_LIST
BODY = w37.BODY
PI = math.pi


def m_ceil(x):
    return F(math.ceil(x))


def i32(x):
    x &= M32
    return x - (1 << 32) if x >= 1 << 31 else x


# ---------------------------------------------------------------------------------------------------------------- KSPRandom (KSPRandom.cs)
class KSPRandom:
    def __init__(self, seed):
        num = i32(161803398 - abs(seed))  # int arithmetic wraps (unchecked)
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

    def next(self):
        return int(self.sample() * 2147483647.0)

    def next_max(self, m):
        return int(self.sample() * float(m))

    def next_range(self, lo, hi):
        num = (hi - lo) & M32
        if num <= 1:
            return lo
        return i32(int(self.sample() * float(num)) + lo)

    def next_double(self):
        return self.sample()


# ---------------------------------------------------------------------------------------------------------------- Mono List.Sort (n <= 16)
def mono_sort(keys, cmp):
    n = len(keys)

    def swap_if_greater(a, b):
        if a != b and cmp(keys[a], keys[b]) > 0:
            keys[a], keys[b] = keys[b], keys[a]

    if n <= 1:
        return keys
    if n == 2:
        swap_if_greater(0, 1)
    elif n == 3:
        swap_if_greater(0, 1)
        swap_if_greater(0, 2)
        swap_if_greater(1, 2)
    else:
        assert n <= 16
        for i in range(0, n - 1):
            j = i
            t = keys[i + 1]
            while j >= 0 and cmp(t, keys[j]) < 0:
                keys[j + 1] = keys[j]
                j -= 1
            keys[j + 1] = t
    return keys


# ---------------------------------------------------------------------------------------------------------------- bodies (golden + C# Awake / Start)
def load_body_data():
    r = json.load(open(BODIES_JSON, encoding="utf-8"))
    G = 6.67408e-11
    g0 = 9.80665
    out = {}
    for e in r["bodies"]:
        cb = e["celestialBody"]
        orb = e["orbitDriver"]["orbit"] if e.get("orbitDriver") else None
        name = cb["bodyName"]
        radius = cb["Radius"]
        mass = radius * radius * (cb["GeeASL"] * g0) / G
        out[name] = {
            "name": name, "index": e["flightGlobalsIndex"], "spawn": e["spawnOrder"], "parent": e["parent"], "radius": radius,
            "gee": cb["GeeASL"], "mass": mass, "mu": mass * G, "rot": cb["rotationPeriod"], "atmDepth": cb["atmosphereDepth"],
            "solarRot": bool(cb["solarRotationPeriod"]), "locked": bool(cb["tidallyLocked"]), "rotates": bool(cb["rotates"]),
            "limits": list(cb["timeWarpAltitudeLimits"]), "sma": orb["semiMajorAxis"] if orb else 0.0, "ecc": orb["eccentricity"] if orb else 0.0,
            "recovery": F(cb["scienceValues"]["RecoveryValue"]), "solid": bool(cb["hasSolidSurface"]), "ocean": bool(cb["ocean"]),
            "atmosphere": bool(cb["atmosphere"]), "home": bool(cb["isHomeWorld"]),
        }
    for b in out.values():
        if b["parent"] is None:
            b["soi"] = math.inf
        else:
            p = out[b["parent"]]
            b["soi"] = b["sma"] * math.pow(b["mass"] / p["mass"], 0.4)
            # CelestialBody.Start: solar day -> sidereal day; CBUpdate: tidally locked bodies rotate with their orbital period
            if b["solarRot"]:
                num = b["rot"]
                num2 = PI * 2.0 * math.sqrt(math.pow(abs(b["sma"]), 3.0) / p["mu"])
                b["rot"] = num * num2 / (num2 + num)
            period = PI * 2.0 / math.sqrt(p["mu"] / (abs(b["sma"]) * abs(b["sma"]) * abs(b["sma"])))
            if b["rotates"] and b["rot"] != 0.0 and (not b["locked"] or period != 0.0):
                if b["locked"]:
                    b["rot"] = period
    kids = {n: [] for n in out}
    for b in sorted(out.values(), key=lambda b: b["spawn"]):
        if b["parent"] is not None:
            kids[b["parent"]].append(b["name"])
    for n, b in out.items():
        b["children"] = kids[n]
    return out


BD = load_body_data()


def physics():
    txt = open(PHYSICS, encoding="utf-8").read()
    g = lambda k: float(re.search(r"^\s*%s\s*=\s*([-0-9.eE+]+)" % k, txt, re.M).group(1))
    return {"space": g("spaceTemperature"), "lum": g("solarLuminosityAtHome"), "sb": 5.670373e-08}


PH = physics()
CONTRACTS = w37.CONTRACTS


def cfg_f(section, sub, key):
    return w37.cfg_f(section, sub, key)


# ---------------------------------------------------------------------------------------------------------------- CelestialUtilities
def sync_sma(b):
    return math.pow(b["mu"] * math.pow(b["rot"] / (PI * 2.0), 2.0), 1.0 / 3.0)


def kolniya_sma(b):
    num = b["rot"] / 2.0
    return math.pow(b["mu"] * math.pow(num / (PI * 2.0), 2.0), 1.0 / 3.0)


def next_time_warp(b, altitude):
    if not b["limits"]:
        return 0.0
    num = 1.7976931348623157e308
    num2 = -1.7976931348623157e308
    for v in reversed(b["limits"]):
        if v > num2:
            num2 = v
        if v > altitude and v < num:
            num = v
    if num > num2:
        num = num2
    return num


def min_orbital_distance(b, margin):
    """CelestialUtilities.GetMinimumOrbitalDistance (highest peak fixture 0: no PQS)"""
    if b["name"] == "Sun":
        home = next(x for x in BD.values() if x["home"])
        while home["parent"] != "Sun" and home["parent"] is not None:
            home = BD[home["parent"]]
        num = math.pow(home["sma"], 2.0) * 4.0 * PI * PH["lum"]
        num2 = F(800.0)  # ContractDefs.SolarOrbitHeatTolerance (float field)
        num2 *= num2
        num2 *= num2
        st = PH["space"]
        st *= st
        st *= st
        return margin * math.sqrt(0.5 * num / PH["sb"] / (num2 - st) / (PI * 4.0))
    num3 = b["radius"]
    radius = b["radius"]
    if b["atmosphere"]:
        num3 += margin * b["atmDepth"]
    radius += margin * next_time_warp(b, 0.0)
    return max(num3, radius)


for _b in BD.values():
    _b["minOrb"] = min_orbital_distance(_b, 1.0)


def can_sync(b, ecc):
    num = b["minOrb"] * 1.05
    num2 = sync_sma(b)
    return not ((1.0 + ecc) * num2 > b["soi"]) and num2 >= num


def can_kol(b):
    num = b["minOrb"] * 1.05
    num2 = kolniya_sma(b)
    return not (num2 * 2.0 - num > b["soi"]) and num2 >= num


def can_tundra(b):
    num = b["minOrb"] * 1.05
    num2 = sync_sma(b)
    return not (num2 * 2.0 - num > b["soi"]) and num2 >= num


def planet_science_ranking(b):
    num = F(0.0)
    for x in BODY_LIST:
        if x["recovery"] > num:
            num = x["recovery"]
    return F(b["recovery"] / num)


def rel_surface_n(lat, lon):
    """Body.GetRelSurfaceNVector(lat, lon): QuaternionD.AngleAxis(lon, down) * AngleAxis(lat, forward) * right (double quaternions)"""
    def aa(angle, axis):
        rad = angle * (PI / 180.0) * 0.5
        s = math.sin(rad)
        return (axis[0] * s, axis[1] * s, axis[2] * s, math.cos(rad))

    def qmul(a, b):
        return (a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1],
                a[3] * b[1] + a[1] * b[3] + a[2] * b[0] - a[0] * b[2],
                a[3] * b[2] + a[2] * b[3] + a[0] * b[1] - a[1] * b[0],
                a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2])

    def qv(q, v):
        x2, y2, z2 = q[0] * 2.0, q[1] * 2.0, q[2] * 2.0
        xx, yy, zz = q[0] * x2, q[1] * y2, q[2] * z2
        xy, xz, yz = q[0] * y2, q[0] * z2, q[1] * z2
        wx, wy, wz = q[3] * x2, q[3] * y2, q[3] * z2
        return ((1.0 - (yy + zz)) * v[0] + (xy - wz) * v[1] + (xz + wy) * v[2],
                (xy + wz) * v[0] + (1.0 - (xx + zz)) * v[1] + (yz - wx) * v[2],
                (xz - wy) * v[0] + (yz + wx) * v[1] + (1.0 - (xx + yy)) * v[2])

    q = qmul(aa(lon, (0.0, -1.0, 0.0)), aa(lat, (0.0, 0.0, 1.0)))
    return qv(q, (1.0, 0.0, 0.0))


def great_circle(b, la1, lo1, la2, lo2):
    v1 = rel_surface_n(la1, lo1)
    v2 = rel_surface_n(la2, lo2)
    return b["radius"] * math.acos(v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2])


def lla_to_ecef(lat, lon, alt, radius):
    lat = (lat - 90.0) * (PI / 180.0)
    lon *= PI / 180.0
    num = (radius + alt) * -1.0 * math.sin(lat) * math.cos(lon)
    num2 = (radius + alt) * math.cos(lat)
    num3 = (radius + alt) * -1.0 * math.sin(lat) * math.sin(lon)
    return [F(num), F(num2), F(num3)]


def within_deviation(v1, v2, dev):
    dev = 100.0 if dev > 100.0 else dev
    dev = 0.0 if dev < 0.0 else dev
    dev = dev / 100.0 if dev > 1.0 else dev
    return abs(v1 - v2) / ((v1 + v2) / 2.0) <= dev


# ---------------------------------------------------------------------------------------------------------------- ProgressUtilities
TYPES = ["NULL", "ALTITUDERECORD", "BASECONSTRUCTION", "CREWTRANSFER", "CREWRECOVERY", "DEPTHRECORD", "DISTANCERECORD", "DOCKING", "ESCAPE", "FIRSTLAUNCH",
         "FLAGPLANT", "FLIGHT", "FLYBY", "FLYBYRETURN", "LANDING", "LANDINGRETURN", "ORBIT", "ORBITRETURN", "POINTOFINTEREST", "REACHSPACE", "RENDEZVOUS",
         "SCIENCE", "SPACEWALK", "SPEEDRECORD", "SPLASHDOWN", "STATIONCONSTRUCTION", "STUNT", "SUBORBIT", "SURFACEEVA"]
HOME = "Kerbin"


def score_type(t, body):
    flag = body is None or body == HOME
    T = {"ESCAPE": (0.6, 1.1), "FLIGHT": (1.2, 0.5), "LANDING": (1.5, 0.1), "LANDINGRETURN": (1.5, 0.1), "ORBIT": (0.8, 1.0), "ORBITRETURN": (0.8, 1.0),
         "RENDEZVOUS": (0.8, 1.0), "SPLASHDOWN": (1.5, 0.3), "BASECONSTRUCTION": (0.8, 0.3), "CREWTRANSFER": (0.8, 0.3), "DOCKING": (0.8, 0.3),
         "POINTOFINTEREST": (0.8, 0.3), "STUNT": (0.8, 0.3), "FLYBY": (1.0, 0.8), "FLYBYRETURN": (1.0, 0.8), "SUBORBIT": (1.0, 0.8),
         "FLAGPLANT": (0.6, 0.2), "SCIENCE": (0.6, 0.2), "SURFACEEVA": (0.6, 0.2)}
    if t in T:
        return F(T[t][1] if flag else T[t][0])
    if t == "SPACEWALK":
        return F(0.6)
    if t in ("ALTITUDERECORD", "DEPTHRECORD", "DISTANCERECORD", "SPEEDRECORD"):
        return F(2.0)
    if t in ("REACHSPACE", "STATIONCONSTRUCTION"):
        return F(0.8)
    return F(0.1)


def outlier(t, body):
    if t == "LANDINGRETURN" and body is not None and BD[body]["atmosphere"]:
        return True
    return t in ("BASECONSTRUCTION", "STATIONCONSTRUCTION")


PRESTIGE_NAMES = ["Trivial", "Significant", "Exceptional"]


def type_prestige(t, body):
    if outlier(t, body):
        return 2
    num = score_type(t, body)
    if body is not None and body != HOME:
        if num <= score_type("ESCAPE", body):
            return 0
        if num >= score_type("FLIGHT", body):
            return 2
    else:
        if num <= score_type("FLIGHT", body):
            return 0
        if num >= score_type("ORBIT", body):
            return 2
    return 1


def prog_cfg(sub, key):
    n = w37.child(w37.CONTRACTS, "Progression")
    if sub:
        n = w37.child(n, sub)
    return F(float(w37.val(n, key)))


def world_first_standard(reward, currency, t, body, mult):
    num = {"Funds": prog_cfg("Funds", "BaseReward"), "Science": prog_cfg("Science", "BaseReward"), "Reputation": prog_cfg("Reputation", "BaseReward")}[currency]
    passive = prog_cfg("", "PassiveBaseRatio")
    num = F(num * ((1.0 - passive) if reward == "CONTRACT" else passive))
    num = F(num * score_type(t, body))
    if outlier(t, body):
        num = F(num * prog_cfg("", "OutlierMilestoneMultiplier"))
    if reward == "CONTRACT":
        return num
    fac = w37.contract_factors(type_prestige(t, None), mult)
    num = F(num * {"Funds": fac["fc"], "Science": fac["sc"], "Reputation": fac["rc"]}[currency])
    if currency == "Funds" and body is not None:
        w = F(BD[body]["recovery"] * 1.0)
        num = F(num * (1.0 + (w - 1.0) * prog_cfg("", "PassiveBodyRatio")))
    return w37.m_round(num)


def world_first_interval(reward, currency, t, body, mult, cur, total):
    cur = 1 if cur < 1 else cur
    total = cur if total < cur else total
    num = world_first_standard(reward, currency, t, body, mult)
    num2 = F(num / float(total))
    if num2 > 1.0:
        return num2
    if num2 <= 0.0:
        return 0.0
    f = F(num2 * float(cur - 1))
    f2 = F(num2 * float(cur))
    if math.floor(f) < math.floor(f2):
        return 1.0
    if cur == total:
        return w37.m_round(F(math.fmod(num, 1.0)))
    return 0.0


def find_next_record(cur, mx, rnd):
    split = int(float(w37.val(w37.child(w37.CONTRACTS, "Progression"), "RecordSplit")))
    num = float(split)
    x = math.pow(100.0, 1.0 / (num - 1.0))
    n2 = 1
    while True:
        if float(n2) <= num:
            num3 = mx * (0.01 * math.pow(x, float(n2 - 1)))
            num3 = round_half_even(num3 / rnd) * rnd
            if not (num3 <= cur):
                return num3, n2
            n2 += 1
            continue
        return 1.7976931348623157e308, 2147483647


def round_half_even(x):
    return float(np.rint(np.float64(x)))


# ---------------------------------------------------------------------------------------------------------------- orbit generation (draws only)
ORBIT_TYPES = ["SYNCHRONOUS", "STATIONARY", "POLAR", "EQUATORIAL", "KOLNIYA", "TUNDRA", "RANDOM"]


def sat_cfg(key):
    return float(w37.val(w37.child(w37.CONTRACTS, "Satellite"), key))


def solar_extents():
    num = 0.0
    for n in BD["Sun"]["children"]:
        b = BD[n]
        apr = (1.0 + b["ecc"]) * b["sma"]
        if apr > num:
            num = apr
    return num


def gen_orbit_raw(seed, bname, otype, alt_d, inc_d, ecc_override):
    """OrbitUtilities.GenerateOrbit up to (not including) GenerateFinalization: the seven elements the draws produce"""
    b = BD[bname]
    g = KSPRandom(seed)
    el = {"sma": 0.0, "ecc": 0.0, "inc": 0.0, "lan": 0.0, "argpe": 0.0, "epoch": 0.0, "mean": 0.0}
    num = b["minOrb"] * 1.05
    num2 = solar_extents() if bname == "Sun" else max(num, b["soi"] * alt_d)
    if otype in ("SYNCHRONOUS", "STATIONARY"):
        el["sma"] = sync_sma(b)
        el["ecc"] = ecc_override if otype == "SYNCHRONOUS" else 0.0
    elif otype == "KOLNIYA":
        el["sma"] = kolniya_sma(b)
        n6 = num
        n5 = el["sma"] * 2.0 - n6
        el["ecc"] = (n5 - n6) / (n5 + n6)
    elif otype == "TUNDRA":
        el["sma"] = sync_sma(b)
        n6 = num
        n5 = el["sma"] * 2.0 - n6
        el["ecc"] = (n5 - n6) / (n5 + n6)
    else:
        n3 = num + (num2 - num) * g.next_double()
        n4 = num + (num2 - num) * g.next_double()
        val = w37.m_lerp(F(n3), F(n4), F(1.0 - alt_d))
        n5 = max(val, n4)
        n6 = min(val, n4)
        el["sma"] = (n5 + n6) / 2.0
        el["ecc"] = (n5 - n6) / (n5 + n6)
    # GenerateInclination
    inc = max(1.0, g.next_double() * 90.0 * inc_d)
    if otype not in ("EQUATORIAL", "STATIONARY"):
        if otype == "POLAR":
            inc = 90.0
    else:
        inc = 0.0
    if bname in (HOME, "Sun"):
        if otype in ("KOLNIYA", "TUNDRA"):
            inc = 116.6 if float(g.next_range(0, 100)) < inc_d * 50.0 else 63.4
        elif otype in ("SYNCHRONOUS", "POLAR", "EQUATORIAL", "RANDOM"):
            if float(g.next_range(0, 100)) < inc_d * 50.0:
                inc = 180.0 - inc
    else:
        coin = lambda: g.next_max(2) == 1
        if otype in ("KOLNIYA", "TUNDRA"):
            inc = 116.6 if coin() else 63.4
        elif otype in ("SYNCHRONOUS", "POLAR", "EQUATORIAL", "RANDOM"):
            if coin():
                inc = 180.0 - inc
    el["inc"] = inc
    el["lan"] = 0.0 if otype in ("EQUATORIAL", "STATIONARY") else g.next_double() * 360.0
    if otype in ("KOLNIYA", "TUNDRA"):
        el["argpe"] = 270.0 if g.next_max(2) == 1 else 90.0
    else:
        el["argpe"] = g.next_double() * 360.0
    el["epoch"] = 0.999 + g.next_double() * 0.0019999999999998908
    el["mean"] = g.next_double() * 2.0 * PI
    return el


# ---------------------------------------------------------------------------------------------------------------- random positions (WaypointManager / Waypoint)
def choose_random_position(bname, water_allowed, equatorial, seed):
    g = KSPRandom(seed)
    b = BD[bname]
    lat = lon = 0.0
    if not b["solid"]:
        water_allowed = True
    if b["ocean"] and not water_allowed:
        num = 10000
        while num >= 0:
            if not equatorial:
                n2 = g.next_double()
                n2 = 1.0 - n2 * 2.0
                lat = math.asin(n2) * 57.295780181884766
            else:
                lat = 0.0
            lon = g.next_double()
            if not (0.0 > 0.0):  # TerrainAltitude(underwater: true) is 0 without a PQS
                num -= 1
                continue
            break
    else:
        if not equatorial:
            n3 = g.next_double()
            n3 = 1.0 - n3 * 2.0
            lat = math.asin(n3) * 57.295780181884766
        else:
            lat = 0.0
        lon = g.next_double()
    return lat, lon


def choose_random_position_near(bname, clat, clon, radius, water_allowed, seed):
    g = KSPRandom(seed)
    b = BD[bname]
    lat = lon = 0.0
    if not b["solid"]:
        water_allowed = True

    def cosv(num):
        c = F(math.cos(F(F(F(3.14159274101257) / F(180.0)) * F(clat))))
        return abs(F(c * F(num)))

    if b["ocean"] and not water_allowed:
        num = 10000
        while num >= 0:
            n2 = b["radius"] * 2.0 * 3.1415927410125732 / 360.0
            n3 = clon - radius / cosv(n2)
            n4 = clon + radius / cosv(n2)
            n5 = clat - radius / n2
            n6 = clat + radius / n2
            lat = n5 + g.next_double() * (n6 - n5)
            lon = n3 + g.next_double() * (n4 - n3)
            if not (0.0 > 0.0):
                radius *= 1.05
                num -= 1
                continue
            break
    else:
        n7 = b["radius"] * 2.0 * 3.1415927410125732 / 360.0
        n8 = clon - radius / cosv(n7)
        n9 = clon + radius / cosv(n7)
        n10 = clat - radius / n7
        n11 = clat + radius / n7
        lat = n10 + g.next_double() * (n11 - n10)
        lon = n8 + g.next_double() * (n9 - n8)
    return lat, lon


def randomize_near(bname, clat, clon, mn, mx, water_allowed, g):
    b = BD[bname]
    if not b["solid"]:
        water_allowed = True
    land_locked = b["ocean"] and not water_allowed
    lat = lon = 0.0
    num = 10000
    while num >= 0:
        lat = PI / 2.0 - clat * (PI / 180.0)
        lon = clon * (PI / 180.0)
        n2 = mn + g.next_double() * (mx - mn)
        n3 = PI / 2.0 - n2 / b["radius"]
        n4 = PI * 2.0 * g.next_double()
        v = (math.cos(n3) * math.sin(n4), math.cos(n3) * math.cos(n4), math.sin(n3))
        v2 = (v[0], v[1] * math.cos(lat) + v[2] * math.sin(lat), (0.0 - v[1]) * math.sin(lat) + v[2] * math.cos(lat))
        v3 = (v2[0] * math.cos(lon) + v2[1] * math.sin(lon), (0.0 - v2[0]) * math.sin(lon) + v2[1] * math.cos(lon), v2[2])
        lat = math.asin(v3[2]) * (180.0 / PI)
        lon = math.atan2(v3[0], v3[1]) * (180.0 / PI)
        if not (not b["ocean"] or water_allowed) and not (0.0 >= 0.0):  # TerrainAltitude(underwater: true) = 0: the first sample is accepted
            mx *= 1.05
            num -= 1
            continue
        break
    return lat, lon, land_locked


def randomize_away_from(bname, olat, olon, mn, mx, samples, water_allowed, seed, clat, clon):
    g = KSPRandom(seed)
    pts = []
    b = BD[bname]
    for _ in range(samples):
        la, lo, _ll = randomize_near(bname, clat, clon, mn, mx, water_allowed, g)
        pts.append((la, lo))
    best = (olat, olon)
    d = 0.0
    for p in reversed(pts):
        if great_circle(b, p[0], p[1], olat, olon) > d:
            best = p
            d = great_circle(b, best[0], best[1], olat, olon)
    return best


# ---------------------------------------------------------------------------------------------------------------- ProgressTrackingParameter tables (parsed from the C#)
def parse_ptp():
    src = open(CS, encoding="utf-8-sig").read().split("\n")

    def block(start_pat, end_pat):
        a = next(i for i, l in enumerate(src) if start_pat in l)
        b = next(i for i in range(a + 1, len(src)) if end_pat in src[i])
        return src[a:b]

    # a case label list ends at the first unguarded return; implement by a second pass that resets the label list
    def table2(lines):
        tab = {}
        labels = []
        cond = None
        for l in lines:
            l = l.strip()
            m = re.match(r"case ProgressType\.(\w+):", l)
            if m:
                if tab and labels and all(tab.get(t) for t in labels) and labels_closed[0]:
                    labels = []
                    labels_closed[0] = False
                labels.append(m.group(1))
                continue
            if l == "default:":
                if labels_closed[0]:
                    labels = []
                    labels_closed[0] = False
                labels.append("default")
                continue
            if l.startswith("if (milestone.body == Planetarium.fetch.Home)"):
                cond = "home"
                continue
            m = re.match(r"return (.*);$", l)
            if m and labels:
                for t in labels:
                    tab.setdefault(t, []).append((cond, m.group(1)))
                if cond is None:
                    labels_closed[0] = True
                cond = None
        return tab

    labels_closed = [False]
    res = {}
    for name, (s, e) in {"instr": ("public string InstructionalNote", "public string FlavorNote"),
                         "flavor": ("public string FlavorNote", "public ProgressTrackingParameter()"),
                         "title": ("protected override string GetTitle()", "protected override string GetNotes()"),
                         "msg": ("protected override string GetMessageComplete()", "protected override void OnRegister()")}.items():
        labels_closed[0] = False
        res[name] = table2(block(s, e))
    return res


PTP = parse_ptp()


def eval_expr(expr, body, home_display):
    """Localizer.Format("tag", args...) with the argument forms of the file"""
    m = re.match(r'Localizer\.Format\((.*)\)$', expr)
    inner = m.group(1)
    args = []
    d = 0
    cur = ""
    for ch in inner:
        if ch == "(":
            d += 1
        if ch == ")":
            d -= 1
        if ch == "," and d == 0:
            args.append(cur.strip())
            cur = ""
            continue
        cur += ch
    args.append(cur.strip())
    tag = args[0]
    th = re.match(r'\(milestone\.body == Planetarium\.fetch\.Home\) \? "([^"]+)" : "([^"]+)"', tag)
    if th:
        tag = th.group(1) if body["home"] else th.group(2)
    elif tag.startswith("milestone.body.ocean"):
        t = re.match(r'milestone\.body\.ocean \? "([^"]+)" : "([^"]+)"', tag)
        tag = t.group(1) if body["ocean"] else t.group(2)
    else:
        tag = tag.strip('"')
    vals = []
    for a in args[1:]:
        if a == "milestone.body.displayName":
            vals.append(body["display"])
        elif a == "Planetarium.fetch.Home.displayName":
            vals.append(home_display)
        elif a.startswith("Convert.ToDecimal"):
            v = int(round_half_even(BD[HOME]["atmDepth"]))
            vals.append("{:,}".format(v) if v else "")
        elif a == "base.Title":
            vals.append(None)
        else:
            raise ValueError(a)
    return tag, vals


def ptp_text(kind, mtype, bname, title_for_msg=None):
    body = BODY[bname]
    home_display = BODY[HOME]["display"]
    tab = PTP[kind]
    ents = tab.get(mtype) or tab["default"]
    for cond, expr in ents:
        if cond == "home" and bname != HOME:
            continue
        tag, vals = eval_expr(expr, body, home_display)
        vals = [title_for_msg if v is None else v for v in vals]
        return LOC.fmt("#" + tag.lstrip("#") if not tag.startswith("#") else tag, *vals)
    raise KeyError((kind, mtype))


def ptp_tutorial(mtype, bname):
    return bname == HOME and mtype in ("FIRSTLAUNCH", "ORBIT", "REACHSPACE", "SCIENCE")


def ptp_title(mtype, bname):
    if ptp_tutorial(mtype, bname):
        return LOC.fmt("#autoLOC_6001924")
    return ptp_text("title", mtype, bname)


# ---------------------------------------------------------------------------------------------------------------- scenario / progress model
IMPL = w37.NODE_ID


class Scn:
    """body states (w37.Scenario) + the top level nodes"""

    def __init__(self, bodies, top):
        self.sc = w37.Scenario(bodies)
        self.top = top
        self.bodies = bodies

    def top_complete(self, name):
        st = self.top.get(name)
        return st is not None and st != "reached"

    def node_complete(self, body, nid):
        st = self.bodies.get(body, {}).get(nid)
        return st is not None and st != "reached"

    def reached(self, body):
        return bool(self.bodies.get(body))

    def milestone_complete(self, t, bname):
        """ProgressMilestone.complete (manned ANY)"""
        if t == "FIRSTLAUNCH":
            return self.top_complete("FirstLaunch")
        if t == "REACHSPACE":
            return self.top_complete("ReachedSpace")
        return self.sc.complete(bname, t, "ANY")


def progress_level(s):
    num = 0
    for b in BODY_LIST:
        if b["name"] not in ("Sun", HOME) and s.reached(b["name"]):
            num += 1
    if s.top_complete("FirstLaunch"):
        num += 1
    if s.node_complete(HOME, "Orbit"):
        num += 1
    avg = 1.0
    return num + max(0, int(round_half_even(avg / F(float(w37.val(w37.CONTRACTS, "FacilityProgressionFactor"))))))


def gen_exploration_milestones(seed, s):
    g = KSPRandom(seed)
    rendezvous = progress_level(s) >= 4
    dock = False
    eva = True
    flags = True
    seeds = seed_milestones(g, s, rendezvous, dock, eva, flags)
    if not seeds:
        return None, None
    idx = g.next_max(len(seeds))
    m = seeds.pop(idx)
    themed = themed_milestones(m, s, rendezvous, dock, eva, flags)
    lst = seeds + themed
    # SystemUtilities.ShuffleList
    for num in range(len(lst) - 1, 0, -1):
        j = g.next_range(0, num + 1)
        lst[num], lst[j] = lst[j], lst[num]
    lst.insert(0, m)
    lst = lst[:min(g.next_max(3) + 1, len(lst))]
    order = {"FLYBY": 0, "FLYBYRETURN": 6, "LANDING": 2, "LANDINGRETURN": 4, "ORBIT": 1, "ORBITRETURN": 5}
    val = lambda x: order.get(x[1], 3)
    mono_sort(lst, lambda a, b: (val(a) > val(b)) - (val(a) < val(b)))
    return lst, (rendezvous, dock, eva, flags)


def any_progress(s, bname, manned):
    return s.sc.any_progress(bname, manned)


def seed_milestones(g, s, rendezvous, dock, eva, flags):
    flag = s.top_complete("FirstLaunch")
    flag2 = s.top_complete("ReachedSpace")
    flag3 = s.node_complete(HOME, "Orbit")
    flag4 = s.node_complete(HOME, "Science")
    if (not flag or not flag4 or not flag2 or not flag3):  # DisableTutorialContracts = False
        if not flag:
            return [(HOME, "FIRSTLAUNCH")]
        if not flag4:
            return [(HOME, "SCIENCE")]
        if not flag2:
            return [(HOME, "REACHSPACE")]
        if not flag3:
            return [(HOME, "ORBIT")]
    # DisableProgressionContracts = False
    bodies = [b["name"] for b in reversed(BODY_LIST) if any_progress(s, b["name"], "ANY") is True]
    list1 = []
    list2 = []
    flag9 = len(bodies) >= len(BODY_LIST) // 2
    if not flag9:
        bodies = [b for b in bodies if b != "Sun"]
    C = lambda b, nid: s.node_complete(b, nid)
    for bn in reversed(bodies):
        b = BD[bn]
        list3 = []
        any_m = any_progress(s, bn, "MANNED")
        if b["home"]:
            if not list3 and not C(bn, "Orbit"):
                list3.append((bn, "ORBIT"))
            if not list3 and eva and any_m and not C(bn, "Spacewalk"):
                list3.append((bn, "SPACEWALK"))
            if not list3 and not C(bn, "ReturnFromOrbit"):
                list3.append((bn, "ORBITRETURN"))
        else:
            if not list3 and not C(bn, "Flyby"):
                list3.append((bn, "FLYBY"))
            if not list3 and not C(bn, "Orbit"):
                list3.append((bn, "ORBIT"))
            if not list3:
                if not C(bn, "Science"):
                    list3.append((bn, "SCIENCE"))
                if eva and any_m and not C(bn, "Spacewalk"):
                    list3.append((bn, "SPACEWALK"))
                if bn != "Sun" and b["atmosphere"] and not C(bn, "Flight"):
                    list3.append((bn, "FLIGHT"))
            if not list3:
                if not C(bn, "ReturnFromFlyBy"):
                    list3.append((bn, "FLYBYRETURN"))
                if not C(bn, "ReturnFromOrbit"):
                    list3.append((bn, "ORBITRETURN"))
                if b["solid"]:
                    if not C(bn, "Landing"):
                        list3.append((bn, "LANDING"))
                    if b["ocean"] and not C(bn, "Splashdown"):
                        list3.append((bn, "SPLASHDOWN"))
        if list3:
            list1 += list3
            continue
        if bn != HOME and b["solid"]:
            if not C(bn, "ReturnFromSurface"):
                if b["atmosphere"]:
                    list2.append((bn, "LANDINGRETURN"))
                else:
                    list3.append((bn, "LANDINGRETURN"))
            if eva and any_m and not C(bn, "SurfaceEVA"):
                list3.append((bn, "SURFACEEVA"))
            if eva and flags and any_m and not C(bn, "FlagPlant"):
                list3.append((bn, "FLAGPLANT"))
            if dock and any_m and not C(bn, "BaseConstruction"):
                list2.append((bn, "BASECONSTRUCTION"))
        if b["home"] and not C(bn, "Escape") and reached_home_bodies(s):
            list3.append((bn, "ESCAPE"))
        if rendezvous:
            if not C(bn, "Rendezvous"):
                list3.append((bn, "RENDEZVOUS"))
            elif any_m and not C(bn, "CelestialBodyTransfer"):
                list3.append((bn, "CREWTRANSFER"))
        if dock and not C(bn, "Docking"):
            list3.append((bn, "DOCKING"))
        if dock and any_m and not C(bn, "StationConstruction"):
            list2.append((bn, "STATIONCONSTRUCTION"))
        list1 += list3
    flag10 = True
    for m in reversed(list1):
        if m[1] in ("FLYBYRETURN", "ORBITRETURN"):
            if m[0] == HOME or BD[m[0]]["parent"] == HOME:
                flag10 = False
        elif m[1] in ("FLIGHT", "FLYBY", "LANDING", "ORBIT", "SCIENCE", "SPACEWALK", "SPLASHDOWN"):
            flag10 = False
    if flag10:
        nxt = next_unreached(s, 1, None if flag9 else (lambda n: n != "Sun"))
        for n in reversed(nxt):
            list1.append((n, "FLYBY"))
    if len(list1) < 1:
        list1 = list2
    if list1:
        bl = []
        for m in reversed(list1):
            if m[0] not in bl:
                bl.append(m[0])
        bodies_list = [BD[n] for n in bl]
        # Contract.WeightedBodyChoice with IgnoresWeight: bodies[KSPRandom.Next(0, count)]
        chosen = bodies_list[g.next_range(0, len(bodies_list))]["name"]
        for i in range(len(list1) - 1, -1, -1):
            if list1[i][0] != chosen:
                del list1[i]
    return list1


def reached_home_bodies(s):
    if not s.reached(HOME):
        return False
    for c in BD[HOME]["children"]:
        if not s.reached(c):
            return False
    return True


def next_unreached(s, count, where):
    lst = [BD["Sun"]] + [BD[n] for n in BD["Sun"]["children"]]
    rec = lambda a, b: (a["recovery"] > b["recovery"]) - (a["recovery"] < b["recovery"])
    mono_sort(lst, rec)
    stack = list(reversed(lst))  # Push(list[count-1]) ... Push(list[0]): pop gives list[0]
    out = []
    while stack and len(out) < count:
        b = stack.pop()
        if not any_progress(s, b["name"], "ANY") and (where is None or where(b["name"])):
            out.append(b["name"])
        if b["name"] != "Sun":
            kids = [BD[n] for n in b["children"]]
            mono_sort(kids, rec)
            for k in reversed(kids):  # while (count2-- > 0) Push(list[count2])
                stack.append(k)
    return out


def themed_milestones(m, s, rendezvous, dock, eva, flags):
    bn, t = m
    b = BD[bn]
    out = []
    C = lambda nid: s.node_complete(bn, nid)
    any_m = any_progress(s, bn, "MANNED")
    solid = b["solid"]
    flag = solid and b["ocean"]
    if not b["home"]:
        if t == "FLIGHT":
            if solid and not C("Landing"):
                out.append((bn, "LANDING"))
            if solid and flag and not C("Splashdown"):
                out.append((bn, "SPLASHDOWN"))
        elif t == "FLYBY":
            if not C("ReturnFromFlyBy"):
                out.append((bn, "FLYBYRETURN"))
            if not C("Science"):
                out.append((bn, "SCIENCE"))
        elif t == "LANDING":
            if not C("ReturnFromSurface"):
                out.append((bn, "LANDINGRETURN"))
            if solid and eva and any_m and not C("SurfaceEVA"):
                out.append((bn, "SURFACEEVA"))
            if solid and eva and flags and any_m and not C("FlagPlant"):
                out.append((bn, "FLAGPLANT"))
        elif t == "ORBIT":
            if not C("ReturnFromOrbit"):
                out.append((bn, "ORBITRETURN"))
            if not C("Science"):
                out.append((bn, "SCIENCE"))
            if eva and any_m and not C("Spacewalk"):
                out.append((bn, "SPACEWALK"))
        elif t in ("FLYBYRETURN", "ORBITRETURN"):
            if rendezvous and not C("Rendezvous"):
                out.append((bn, "RENDEZVOUS"))
            if dock and not C("Docking"):
                out.append((bn, "DOCKING"))
            if rendezvous and any_m and not C("CelestialBodyTransfer"):
                out.append((bn, "CREWTRANSFER"))
        elif t == "SCIENCE":
            if not C("ReturnFromFlyBy"):
                out.append((bn, "FLYBYRETURN"))
            if not C("ReturnFromOrbit"):
                out.append((bn, "ORBITRETURN"))
            if solid and eva and any_m and not C("SurfaceEVA"):
                out.append((bn, "SURFACEEVA"))
        elif t == "SPACEWALK":
            if not C("ReturnFromFlyBy"):
                out.append((bn, "FLYBYRETURN"))
            if not C("ReturnFromOrbit"):
                out.append((bn, "ORBITRETURN"))
            if rendezvous and not C("Rendezvous"):
                out.append((bn, "RENDEZVOUS"))
            if rendezvous and any_m and not C("CelestialBodyTransfer"):
                out.append((bn, "CREWTRANSFER"))
        elif t == "SPLASHDOWN":
            if not C("ReturnFromSurface"):
                out.append((bn, "LANDINGRETURN"))
            if solid and eva and any_m and not C("SurfaceEVA"):
                out.append((bn, "SURFACEEVA"))
    else:
        if t == "SPACEWALK":
            if not C("ReturnFromOrbit"):
                out.append((bn, "ORBITRETURN"))
            if rendezvous and not C("Rendezvous"):
                out.append((bn, "RENDEZVOUS"))
            if rendezvous and any_m and not C("CelestialBodyTransfer"):
                out.append((bn, "CREWTRANSFER"))
        elif t == "ORBITRETURN":
            if rendezvous and not C("Rendezvous"):
                out.append((bn, "RENDEZVOUS"))
            if dock and not C("Docking"):
                out.append((bn, "DOCKING"))
            if rendezvous and any_m and not C("CelestialBodyTransfer"):
                out.append((bn, "CREWTRANSFER"))
    return out


def gen_exploration(seed, s, mult, guid):
    lst, flags = gen_exploration_milestones(seed, s)
    if not lst:
        return None
    rng = w37.Unity(seed)
    p = None
    tracking = lst
    # SetLocals
    target = tracking[0][0]
    # body subtree IsComplete ratio
    nodes = ["Orbit", "Escape", "Landing", "Splashdown", "Science", "Rendezvous", "Docking", "Spacewalk", "SurfaceEVA", "FlagPlant", "BaseConstruction",
             "StationConstruction", "CelestialBodyTransfer", "ReturnFromOrbit"]
    if not BD[target]["home"]:
        nodes += ["Flight", "Suborbit", "Flyby", "ReturnFromFlyBy", "ReturnFromSurface"]
    ratio = 0.0
    for nid in nodes:
        if s.node_complete(target, nid):
            ratio = F(ratio + 1.0)
    ratio = F(ratio / float(len(nodes)))
    # SetPrestige
    tut = len(tracking) == 1 and ptp_tutorial(tracking[0][1], tracking[0][0])
    if tut:
        t = tracking[0][1]
        prestige = {"ORBIT": 2, "FIRSTLAUNCH": 0, "SCIENCE": 0, "REACHSPACE": 1}[t]
    else:
        prestige = len(tracking) - 1
        for m in reversed(tracking):
            if outlier(m[1], m[0]):
                prestige = 2
                break
    # SetRewards
    fac = w37.contract_factors(prestige, mult)
    dest = lambda b: 1.0 if b is None else F(BD[b]["recovery"] * 1.0)
    num = num2 = num3 = 0.0
    params = []
    for m in reversed(tracking):
        n4 = m_ceil(F(world_first_standard("CONTRACT", "Funds", m[1], m[0], mult) / 2.0))
        n5 = m_ceil(F(world_first_standard("CONTRACT", "Science", m[1], m[0], mult) / 2.0))
        n6 = m_ceil(F(world_first_standard("CONTRACT", "Reputation", m[1], m[0], mult) / 2.0))
        pf = w37.m_round(F(n4 * fac["fc"] * dest(target)))
        ps = w37.m_round(F(n5 * fac["sc"] * 1.0))
        pr = w37.m_round(F(n6 * fac["rc"] * 1.0))
        params.append((m, pf, ps, pr))
        num = F(num + n4)
        num2 = F(num2 + n5)
        num3 = F(num3 + n6)
    params.reverse()
    n7 = F(num * F(0.35))
    num = F(num - n7)
    fa = w37.m_round(F(n7 * fac["fa"] * dest(target)))
    fc = w37.m_round(F(num * fac["fc"] * dest(target)))
    sc = w37.m_round(F(num2 * fac["sc"] * 1.0))
    rc = w37.m_round(F(num3 * fac["rc"] * 1.0))
    # SetExpiry() -> Random.Range(1, 7) on the UnityEngine.Random state of Contract.SetupSeed (InitState(seed))
    expiry = w37.m_round(F(float(rng.range_i(1, 7)) * float(w37.DAY) / 60.0)) * 60.0
    kws = []
    req = ["Record"]
    agent = w37.suitable_agent(rng, kws, req)
    if agent is None:
        return None
    contract = {"fa": fa, "fc": fc, "ff": fa, "rc": rc, "rf": 0.0, "sc": sc, "expiry": expiry, "deadline": 0.0}
    for nm, f in agent["mentality"]:
        if w37.can_process(nm, kws, req):
            w37.apply_mentality(contract, nm, f, prestige)
    contract["ff"] = max(contract["ff"], contract["fa"])
    # titles / texts
    disp = BODY[target]["display"]
    if tut:
        t = tracking[0][1]
        tb = tracking[0][0]
        title = {"ORBIT": lambda: LOC.fmt("#autoLOC_278641", BODY[tb]["display"]), "FIRSTLAUNCH": lambda: LOC.fmt("#autoLOC_278637"),
                 "SCIENCE": lambda: LOC.fmt("#autoLOC_278643", BODY[tb]["display"]), "REACHSPACE": lambda: LOC.fmt("#autoLOC_278639")}[t]()
        synopsis = ptp_text("flavor", t, tb)
        topic = {"ORBIT": "Orbit", "FIRSTLAUNCH": "Launch", "SCIENCE": "Science", "REACHSPACE": "Space"}[t]
        desc = w37.back_story("ExplorationContract", agent["title"], topic, topic, seed, False, False, False)
    else:
        title = LOC.fmt("#autoLOC_278647", disp)
        if ratio < F(0.33):
            synopsis = LOC.fmt("#autoLOC_278678", disp)
        elif ratio > F(0.66):
            synopsis = LOC.fmt("#autoLOC_278680", disp)
        else:
            synopsis = LOC.fmt("#autoLOC_278682", disp)
        desc = w37.back_story("ExplorationContract", agent["title"], disp, disp, seed, True, True, True)
    # contract ID
    num = w37.wrap64(391 + w37.hash35("ExplorationContract"))
    ss = str(w37.super_seed(guid, seed))
    out_params = []
    for i, (m, pf, ps, pr) in enumerate(params):
        h = ss + ""  # ID is empty (AddParameter without id)
        num = w37.wrap64(num * 23 + w37.hash35("ProgressTrackingParameter"))
        num = w37.wrap64(num * 23 + w37.hash35(h))
        out_params.append({"body": m[0], "type": m[1], "title": ptp_title(m[1], m[0]), "instr": ptp_text("instr", m[1], m[0]),
                           "flavor": ptp_text("flavor", m[1], m[0]), "hash": h,
                           "msg": ptp_text("msg", m[1], m[0]) if not ptp_tutorial(m[1], m[0]) else None,
                           "fc": pf, "sc": ps, "rc": pr})
    return {"agent": agent["name"], "agentTitle": agent["title"], "keywordsRequired": req, "title": title, "synopsis": synopsis, "description": desc,
            "contractID": num, "prestige": PRESTIGE_NAMES[prestige], "targetBody": target, "bodyRatio": ratio, "tutorial": tut, "params": out_params,
            "fa": contract["fa"], "fc": contract["fc"], "ff": contract["ff"], "rc": contract["rc"], "rf": contract["rf"], "sc": contract["sc"],
            "expiry": contract["expiry"], "deadline": contract["deadline"],
            "rendezvous": flags[0], "dock": flags[1], "eva": flags[2], "flags": flags[3]}


# ---------------------------------------------------------------------------------------------------------------- main
SCENARIOS = [
    {"name": "empty", "top": {}, "bodies": {}},
    {"name": "launched", "top": {"FirstLaunch": "completed"}, "bodies": {}},
    {"name": "science", "top": {"FirstLaunch": "completed"}, "bodies": {"Kerbin": {"Science": "completed"}}},
    {"name": "space", "top": {"FirstLaunch": "completed", "ReachedSpace": "completed"}, "bodies": {"Kerbin": {"Science": "completed"}}},
    {"name": "orbit-done", "top": {"FirstLaunch": "completed", "ReachedSpace": "completed"}, "bodies": {"Kerbin": {"Science": "completed", "Orbit": "completed"}}},
    {"name": "mun-visited", "top": {"FirstLaunch": "completed", "ReachedSpace": "completed"},
     "bodies": {"Kerbin": {"Science": "completed", "Orbit": "completed", "ReturnFromOrbit": "completed", "Spacewalk": "completed"},
                "Mun": {"Flyby": "completed", "Orbit": "completedManned"}}},
    {"name": "mun-minmus", "top": {"FirstLaunch": "completed", "ReachedSpace": "completed"},
     "bodies": {"Kerbin": {"Science": "completed", "Orbit": "completed", "ReturnFromOrbit": "completed", "Spacewalk": "completed"},
                "Mun": {"Flyby": "completed", "Orbit": "completed", "Landing": "completedManned", "Science": "completed", "ReturnFromFlyBy": "completed"},
                "Minmus": {"Flyby": "completedUnmanned", "Orbit": "completedUnmanned", "Landing": "completedUnmanned"}}},
    {"name": "interplanetary", "top": {"FirstLaunch": "completed", "ReachedSpace": "completed"},
     "bodies": {"Kerbin": {"Science": "completed", "Orbit": "completed", "ReturnFromOrbit": "completed", "Spacewalk": "completed", "Escape": "completed"},
                "Mun": {"Flyby": "completed", "Orbit": "completed", "Landing": "completedManned", "Science": "completed", "ReturnFromFlyBy": "completed",
                        "ReturnFromOrbit": "completed", "ReturnFromSurface": "completed", "SurfaceEVA": "completed", "FlagPlant": "completed",
                        "Splashdown": "reached"},
                "Minmus": {"Flyby": "completed", "Orbit": "completed", "Landing": "completed", "Science": "completed"},
                "Duna": {"Flyby": "completedUnmanned", "Orbit": "reached"}, "Eve": {"Flyby": "completed", "Orbit": "completed", "Flight": "completed"},
                "Jool": {"Flyby": "completed"}, "Laythe": {"Orbit": "reached"}, "Ike": {"Flyby": "completed"}}},
    {"name": "everything", "top": {"FirstLaunch": "completed", "ReachedSpace": "completed"},
     "bodies": {b["name"]: {nid: "completed" for nid in list(set(IMPL.values()))} for b in BODY_LIST}},
]


def main():
    ref = {"provenance": "tools/wp38/fineprint_reference.py (independent of kspcore); mirror cfgs of core/testdata/GameData-mirror, "
                         "harness/golden/serialized/celestial_bodies.serialized.json, harness/inputs/Physics.cfg, decompiled FinePrint C#"}
    ref["bodies"] = [{"name": b["name"], "display": b["display"], "recovery": b["recovery"], "minOrb": BD[b["name"]]["minOrb"], "mass": BD[b["name"]]["mass"],
                      "mu": BD[b["name"]]["mu"], "soi": (BD[b["name"]]["soi"] if math.isfinite(BD[b["name"]]["soi"]) else None)} for b in BODY_LIST]
    ref["mult_sets"] = w37.MULT_SETS

    # --- KSPRandom sequences
    rr = []
    for seed in (0, 1, 7, 1234, -5, 2147483647, -2147483647, 987654321):
        k = KSPRandom(seed)
        ints = [k.next() for _ in range(5)]
        mx = [k.next_max(100) for _ in range(5)]
        rg = [k.next_range(-5, 50) for _ in range(5)]
        rg1 = [k.next_range(3, 4) for _ in range(2)]
        dbl = [k.next_double() for _ in range(6)]
        rr.append({"seed": seed, "next": ints, "nextMax100": mx, "nextRange": rg, "nextRangeTiny": rg1, "double": dbl})
    ref["krandom"] = rr

    # --- ProgressUtilities
    sc = []
    for t in TYPES:
        for b in (None, "Kerbin", "Mun", "Eve", "Duna", "Jool", "Sun"):
            sc.append({"type": t, "body": b, "score": score_type(t, b), "outlier": outlier(t, b), "prestige": type_prestige(t, b)})
    ref["scores"] = sc
    rw = []
    for mi, mult in enumerate(w37.MULT_SETS):
        for t in TYPES[1:]:
            for b in (None, "Kerbin", "Mun", "Eve", "Duna", "Eeloo", "Jool"):
                for reward in ("CONTRACT", "PROGRESS"):
                    for cur in ("Funds", "Science", "Reputation"):
                        rw.append({"mult": mi, "type": t, "body": b, "reward": reward, "currency": cur, "value": world_first_standard(reward, cur, t, b, mult)})
    ref["worldFirst"] = rw
    iv = []
    for mi, mult in enumerate(w37.MULT_SETS):
        for t in ("LANDING", "ORBIT", "FLYBY", "ESCAPE", "SCIENCE"):
            for b in ("Mun", "Eve"):
                for cur in ("Funds", "Science", "Reputation"):
                    for (c, n) in ((1, 10), (5, 10), (10, 10), (0, 3), (4, 2), (1, 1), (3, 7)):
                        iv.append({"mult": mi, "type": t, "body": b, "currency": cur, "cur": c, "total": n,
                                   "value": world_first_interval("PROGRESS", cur, t, b, mult, c, n)})
    ref["worldFirstInterval"] = iv
    rec = []
    for (cur, mx, rnd) in ((0.0, 750.0, 1.0), (10.0, 750.0, 1.0), (300.0, 750.0, 1.0), (99999.0, 750.0, 1.0), (0.0, 100000.0, 100.0), (1234.0, 100000.0, 100.0),
                           (60000.0, 100000.0, 100.0), (0.0, 2500.0, 5.0), (800.0, 2500.0, 5.0), (2499.0, 2500.0, 5.0), (3000.0, 2500.0, 5.0)):
        v, it = find_next_record(cur, mx, rnd)
        rec.append({"current": cur, "max": mx, "round": rnd, "value": v, "interval": it})
    ref["records"] = rec

    # --- CelestialUtilities
    cel = []
    for b in BODY_LIST:
        d = BD[b["name"]]
        e = {"name": b["name"], "syncSMA": sync_sma(d), "kolniyaSMA": kolniya_sma(d), "canSync0": can_sync(d, 0.0), "canSync5": can_sync(d, 0.5),
             "canKol": can_kol(d), "canTundra": can_tundra(d), "ranking": planet_science_ranking(d), "minOrb": d["minOrb"],
             "nextWarp": [next_time_warp(d, a) for a in (-1.0, 0.0, 10000.0, 100000.0, 1e12)], "lla": lla_to_ecef(12.5, -77.25, 1500.0, d["radius"]),
             "gc": great_circle(d, 10.0, 20.0, -30.0, 100.0)}
        cel.append(e)
    ref["celestial"] = cel
    ref["solarExtents"] = solar_extents()
    wd = []
    for v1, v2, dev in ((100.0, 104.0, 5.0), (100.0, 110.0, 5.0), (5.0, 5.0, 0.0), (1.0, 2.0, 150.0), (100.0, 103.0, 0.04)):
        wd.append({"v1": v1, "v2": v2, "dev": dev, "within": within_deviation(v1, v2, dev),
                   "measure": abs(v1 - v2) / ((v1 + v2) / 2.0) if (v1 + v2) != 0 else None})
    ref["deviation"] = wd

    # --- orbit generation (draw level)
    orb = []
    for bn in ("Kerbin", "Mun", "Duna", "Jool", "Sun", "Eve", "Minmus"):
        for ot in ORBIT_TYPES:
            for seed in (1, 42, 777, -13):
                for (ad, idf) in ((0.2, 0.2), (0.4, 0.4), (0.8, 0.8)):
                    eo = 0.0 if ot != "SYNCHRONOUS" else 0.15
                    orb.append({"body": bn, "type": ot, "seed": seed, "alt": ad, "inc": idf, "ecc": eo, "el": gen_orbit_raw(seed, bn, ot, ad, idf, eo)})
    ref["orbits"] = orb

    # --- random positions
    pos = []
    for bn in ("Mun", "Kerbin", "Jool", "Eve", "Minmus"):
        for seed in (3, 99, -8):
            for wa in (True, False):
                for eq in (False, True):
                    la, lo = choose_random_position(bn, wa, eq, seed)
                    pos.append({"kind": "random", "body": bn, "seed": seed, "water": wa, "equatorial": eq, "lat": la, "lon": lo})
                la, lo = choose_random_position_near(bn, 5.5, -70.0, 12000.0, wa, seed)
                pos.append({"kind": "near", "body": bn, "seed": seed, "water": wa, "clat": 5.5, "clon": -70.0, "radius": 12000.0, "lat": la, "lon": lo})
    ref["positions"] = pos
    wp = []
    for bn in ("Mun", "Kerbin", "Jool", "Duna"):
        for seed in (1, 5, 31337):
            for wa in (True, False):
                g = KSPRandom(seed)
                la, lo, ll = randomize_near(bn, -3.0, 40.0, 100.0, 5000.0, wa, g)
                wp.append({"kind": "near", "body": bn, "seed": seed, "water": wa, "lat": la, "lon": lo, "landLocked": ll})
                la, lo = randomize_away_from(bn, 10.0, 20.0, 50.0, 8000.0, 3, wa, seed, -3.0, 40.0)
                wp.append({"kind": "away", "body": bn, "seed": seed, "water": wa, "lat": la, "lon": lo})
    ref["waypoints"] = wp

    # --- ProgressTrackingParameter text tables
    tt = []
    for t in TYPES[1:]:
        for bn in ("Kerbin", "Mun", "Eve", "Duna"):
            m = {"type": t, "body": bn, "tutorial": ptp_tutorial(t, bn), "title": ptp_title(t, bn), "instr": ptp_text("instr", t, bn),
                 "flavor": ptp_text("flavor", t, bn), "msg": ptp_text("msg", t, bn, "TITLE") if not ptp_tutorial(t, bn) else None}
            tt.append(m)
    ref["ptpTexts"] = tt

    # --- ExplorationContract
    ref["scenarios"] = SCENARIOS
    guid = bytes([0x21, 0x43, 0x65, 0x87, 0xA9, 0xCB, 0xED, 0x0F, 0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE])
    ex = []
    for si, scn in enumerate(SCENARIOS):
        s = Scn(scn["bodies"], scn["top"])
        for seed in range(1, 31):
            for mi, mult in enumerate(w37.MULT_SETS):
                g = bytes(((b + seed) & 0xFF) for b in guid)
                sd = seed * 6007 + 11
                r = gen_exploration(sd, s, mult, g)
                ex.append({"scenario": si, "seed": sd, "mult": mi, "guid": list(g), "result": r})
    ref["exploration"] = ex
    json.dump(ref, open(OUT, "w", encoding="utf-8"), indent=1)
    gen = sum(1 for e in ex if e["result"])
    print("wrote", OUT, "krandom", len(rr), "worldFirst", len(rw), "orbits", len(orb), "positions", len(pos), "waypoints", len(wp), "ptp", len(tt),
          "exploration", len(ex), "generated", gen)


if __name__ == "__main__":
    main()
