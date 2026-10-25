"""WP-38d reference: CometManager / CometOrbitType / CometDefinition / DiscoverableObjectsUtil / ScenarioDiscoverableObjects (Assembly-CSharp root).

Independent of kspcore (no kspcore import). Inputs, all game data: the shipped CometDefs.cfg of the GameData mirror (core/testdata/GameData-mirror/GameData/Squad/
Resources/CometDefs.cfg: COMET_MANAGER, COMET_ORBIT_TYPE x4, SPACEOBJECTS_LIMITS), the serialized celestial bodies (harness/golden/serialized/celestial_bodies.serialized.json:
Kerbin and Jool semi-major axes, Dres radius / sphere of influence), the localization dictionary (#autoLOC_6001923) and the decompiled C#. The earlier independent
references of WP-37 / WP-38 / WP-29b / WP-53 supply UnityEngine.Random (C2 capture), KSPRandom, the AnimationCurve evaluator and the cfg parser.

What the tables compare with kspcore (tests/test_space_objects.cpp):
  * manager / types: every field LoadManagerDefinitions / LoadCometTypes reads (float32 where the C# field is float, double otherwise), the curves sampled on a grid.
  * spawn_chance / should_spawn / weighted_type / spawn_distance / class_radius / definitions: CometManager's generators, bit exact (Unity Random draws + KSPRandom).
  * orbits: CometOrbitType.CalculateOrbit at the DRAW level: the element draws (inclination, eccentricity, sma, LAN) are bit exact, the Unity Random state after the call is
    bit exact (it proves the draw order and count), the mean anomaly comes from acos / atan2 / sin / sinh of the host libm here and from the game's runtime in kspcore (x87
    Math.Sin, VS2013 CRT acos): compared with a 1e-9 absolute bound, as WP-38's GenerateOrbit.
  * asteroid_names / comet_names / size_class / spawn_asteroid / spawn_comet: DiscoverableObjectsUtil and the ScenarioDiscoverableObjects spawners; each case lists the values
    of the spawned proto vessel node that are pure functions of the draws and the Unity Random state afterwards (the draws inside Orbit.CreateRandomOrbitFlyBy and the
    ProtoVessel node builder are replayed as plain draws: the orbit vectors themselves are WP-03 / WP-10 territory).
SYNTHETIC inputs (labelled in the tests): the Kerbal name tables of CrewGenerator (the text assets are not in the repo) and the comet name dictionary states, the Sentinel
orbit fields (LAN, argument of periapsis, true anomaly, radius), the ScenarioDiscoverableObjects seeds.

Usage: python -I core/tools/wp38d/space_objects_reference.py > core/tests/data/wp38d_reference.json
"""
import json
import math
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
for sub in ("wp37", "wp38", "wp53", "wp29b"):
    sys.path.insert(0, os.path.join(CORE, "tools", sub))
import contract_reference as w37  # noqa: E402
import fineprint_reference as w38  # noqa: E402
import serenity_reference as w53  # noqa: E402
import wheel_reference as wr  # noqa: E402

F = w37.F
f32 = np.float32
M32 = 0xFFFFFFFF
PI = math.pi
DEFS = os.path.join(CORE, "testdata", "GameData-mirror", "GameData", "Squad", "Resources", "CometDefs.cfg")
BODIES = os.path.join(CORE, "..", "harness", "golden", "serialized", "celestial_bodies.serialized.json")
BD = w38.BD
HOME_SMA = BD["Kerbin"]["sma"]
PERTURBER = "Jool"  # the one body with isCometPerturber = 1 in the serialized data
PERT_SMA = BD[PERTURBER]["sma"]
LOC = w37.LOC


def st(x):
    return float(f32(x))


# ---------------------------------------------------------------------------------------------------------------- UnityEngine.Random (C2) with InitState
class U:
    def __init__(self, seed=0):
        self.r = w37.Unity(seed)

    def init(self, seed):
        self.r = w37.Unity(seed)

    def draw(self, n=1):
        for _ in range(n):
            self.r.next()

    def rng(self, mn, mx):  # Random.Range(float, float)
        return self.r.range_f(st(mn), st(mx))

    def rngi(self, mn, mx):  # Random.Range(int, int)
        return self.r.range_i(mn, mx)

    def value(self):
        return self.r.value()

    def state(self):
        return list(self.r.s)


def kspr(seed):
    return w53.KSPRandom(seed)


def k_next_max(k, n):
    return int(k.sample() * float(n))


def k_next_range(k, lo, hi):
    return k.next_range(lo, hi)


# ---------------------------------------------------------------------------------------------------------------- cfg
def load_defs():
    text = open(DEFS, encoding="utf-8-sig").read()
    text = re.sub(r"\}\s*,", "}", text)  # PreFormatConfig splits "}," into "}" and ","; the stray "," line is dropped by RecurseFormat
    tmp = os.path.join(HERE, "_defs_tmp.cfg")
    open(tmp, "w", encoding="utf-8").write(text)
    try:
        return w37.parse_cfg(tmp)
    finally:
        os.remove(tmp)


ROOT = load_defs()


def nodes(root, name):
    return [n for n in root["nodes"] if n["name"] == name]


MGR = nodes(ROOT, "COMET_MANAGER")[0]
TYPES_CFG = nodes(ROOT, "COMET_ORBIT_TYPE")
LIMITS = nodes(ROOT, "SPACEOBJECTS_LIMITS")[0]

# C# field initialisers (CometManager.cs) for the fields the cfg may leave out; (kind, default)
MGR_FIELDS = {
    "maxSpawnDistance": ("f", 2.0), "minSpawnDistance": ("f", 1.5),
    "maxVFXStartDistance": ("d", 2.0), "minVFXStartDistance": ("d", 0.8),
    "maxComaSizeRatio": ("d", 1000000.0), "minComaSizeRatio": ("d", 10000.0),
    "maxTailWidthRatio": ("d", 1.0), "maxIonTailWidthRatio": ("d", 0.3),
    "maxTailLengthRatio": ("d", 100000.0), "minTailLengthRatio": ("d", 1000.0),
    "minGeyserRadiusRatio": ("d", 0.04), "maxGeyserRadiusRatio": ("d", 0.04),
    "geyserVisibleDistance": ("f", 500.0), "geyserSizeRadiusMultiplier": ("f", 0.05), "nearDustVisibleRadiusMultiplier": ("f", 2.0),
    "minNearDustEmitterRadiusRatio": ("d", 0.1), "maxNearDustEmitterRadiusRatio": ("d", 0.1),
    "atmospherePressureForVFXFade": ("f", 0.05), "gravityAdditionForVFXFadeStart": ("f", 300.0), "gravityMultiplierForVFXFadeEnd": ("f", 100.0),
    "gravitySubtractionForVFXFadeEnd": ("f", 20.0), "vesselRangeOrbitLoadOverride": ("f", 150000.0), "vesselRangeOrbitUnloadOverride": ("f", 151000.0),
    "fragmentdynamicPressurekPa": ("f", 10.0), "minFragments": ("i", 3), "maxFragments": ("i", 5), "minFragmentClassStep": ("i", 1),
    "maxFragmentClassStep": ("i", 3), "fragmentInstanceOffset": ("f", 2.0), "fragmentAngleVariation": ("f", 22.5), "fragmentIgnoreCollisions": ("i", 20),
    "fragmentMinPressureModifier": ("f", 6.0), "fragmentMaxPressureModifier": ("f", 1.0), "fragmentMinSafeTime": ("f", 3.0), "fragmentMaxSafeTime": ("f", 20.0),
}
CURVE_DEFAULTS = {
    "maxCometsPerUntrackedObjects": [(10.0, 1.0), (250.0, 5.0)], "spawnChance": [(0.0, 0.1), (1.0, 0.0)],
    "vfxStartDistribution": [(0.0, 0.0), (1.0, 1.0)], "comaSizeRatioDistribution": [(0.0, 0.0), (1.0, 1.0)], "tailWidthDistribution": [(0.0, 0.0), (1.0, 1.0)],
    "tailLengthRatioDistribution": [(0.0, 0.0), (1.0, 1.0)], "vfxSizeFromDistance": [(0.0, 1.0), (1.0, 0.0)], "nearDustVFXSizeFromDistance": [(0.0, 1.0), (1.0, 0.0)],
    "geyserVFXSizeFromDistance": [(0.0, 1.0), (1.0, 0.0)], "geysersRatioDistribution": [(0.0, 0.0), (1.0, 1.0)],
    "atmospherePressureFadeDistributionForVFXFade": [(0.0, 0.0), (1.0, 1.0)],
}


def val(node, key):
    for k, v in node["values"]:
        if k == key:
            return v
    return None


def parse_val(kind, text):
    if kind == "f":
        return st(float(text))
    if kind == "d":
        return float(text)
    return int(text)


def curve_from_node(node):
    entries = []
    for k, v in node["values"]:
        if k != "key":
            continue
        p = [x for x in v.replace(",", " ").replace(";", " ").replace("\t", " ").split(" ") if x]
        if len(p) == 4:
            entries.append((st(float(p[0])), st(float(p[1])), st(float(p[2])), st(float(p[3])), True))
        else:
            entries.append((st(float(p[0])), st(float(p[1])), 0.0, 0.0, False))
    return wr.Curve(entries)


def default_curve(name):
    # FloatCurve(Keyframe[]) of the field initialisers: AddKey with the keyframe as given (tangents 0)
    return wr.Curve([(a, b, 0.0, 0.0, True) for (a, b) in CURVE_DEFAULTS[name]])


def build_manager():
    m = {}
    for name, (kind, default) in MGR_FIELDS.items():
        t = val(MGR, name)
        m[name] = parse_val(kind, t) if t is not None else default
    curves = {}
    for name in CURVE_DEFAULTS:
        node = next((n for n in MGR["nodes"] if n["name"] == name), None)
        if node is not None and len([1 for k, _ in node["values"]]) > 0:
            curves[name] = curve_from_node(node)  # `node.values.Count > 0`: any value of the node
        else:
            curves[name] = default_curve(name)
    return m, curves


MANAGER, CURVES = build_manager()


def parse_color(text):
    p = [x for x in text.replace(",", " ").split() if x]
    return [st(float(x)) for x in p]


def color_list(node_name, default):
    node = next((n for n in MGR["nodes"] if n["name"] == node_name), None)
    if node is None:
        return default
    out = []
    for n in node["nodes"]:
        if n["name"] != "weightedColor":
            continue
        c = parse_color(val(n, "emitterColor")) if val(n, "emitterColor") is not None else [0.04313725, 0.2906635, 13.0 / 15.0, 23.0 / 51.0]
        w = int(val(n, "chanceWeight")) if val(n, "chanceWeight") is not None else 5
        out.append((c, w))
    if not out:
        out.append(([st(0.04313725), st(0.2906635), st(13.0 / 15.0), st(23.0 / 51.0)], 5))  # new CometTailColor() defaults
    return out


ION = color_list("ionTailColors", [([st(0.04313725), st(0.2906635), st(13.0 / 15.0), st(23.0 / 51.0)], 5)])
DUST = color_list("dustTailColors", [([st(105.0 / 106.0), st(0.9433554), st(0.7429245), st(0.03137255)], 5)])
ION_P67 = parse_color(val(MGR, "ionTailColorP67"))

CLASS_NAMES = "ABCDEFGHI"


def parse_class(t, default):
    return CLASS_NAMES.index(t) if t in list("ABCDEFGHI") and t is not None else default


def build_types():
    out = []
    for n in TYPES_CFG:
        d = {"name": "", "displayName": "", "description": "", "inclination1Min": 0.0, "inclination1Max": 0.0, "inclination2Min": 0.0, "inclination2Max": 0.0,
             "inclination1Chance": st(0.8), "peMin": 0.0, "peMax": 0.0, "apMin": 0.0, "apMax": 0.0, "useEccentricity": False, "eccentricityMin": st(1.01),
             "eccentricityMax": st(1.4), "minCometClass": 5, "maxCometClass": 8, "chanceWeight": 5}
        for k in ("name", "displayName", "description"):
            t = val(n, k)
            if t is not None:
                d[k] = t
        for k in ("inclination1Min", "inclination1Max", "inclination2Min", "inclination2Max", "inclination1Chance", "peMin", "peMax", "eccentricityMin",
                  "eccentricityMax", "apMin", "apMax"):
            t = val(n, k)  # exact (case sensitive) key: the shipped cfg spells inclination2MAx, which is never read
            if t is not None:
                d[k] = st(float(t))
        t = val(n, "useEccentricity")
        if t is not None:
            d["useEccentricity"] = t.strip().lower() == "true"
        t = val(n, "chanceWeight")
        if t is not None:
            d["chanceWeight"] = int(t)
        t = val(n, "minCometClass")
        d["minCometClass"] = parse_class(t, 5)
        t = val(n, "maxCometClass")
        d["maxCometClass"] = parse_class(t, 8)
        out.append(d)
    return out


TYPES = build_types()
TYPE_BY_NAME = {t["name"]: t for t in TYPES}


# ---------------------------------------------------------------------------------------------------------------- curve grid
def grid_for(curve):
    ks = curve.keys
    lo, hi = ks[0][0], ks[-1][0]
    xs = [st(lo - 0.25), st(lo - 1e-3)]
    xs += [st(lo + (hi - lo) * i / 40.0) for i in range(41)]
    xs += [k[0] for k in ks]
    xs += [st(hi + 1e-3), st(hi + 0.25)]
    return xs


def curve_table(curve):
    xs = grid_for(curve)
    return {"keys": [list(k) for k in curve.keys], "samples": [[x, curve.evaluate(x)] for x in xs]}


# ---------------------------------------------------------------------------------------------------------------- CometManager generators
def spawn_chance(group_max, sentinel_max, comets):
    """CometManager.GetSpawnChance (cs:343)"""
    num = 0
    if group_max is not None:
        num += group_max
    if sentinel_max is not None:
        num += sentinel_max
    cap = CURVES["maxCometsPerUntrackedObjects"].evaluate(st(float(num)))
    ratio = st(float(comets) / cap)
    return cap, CURVES["spawnChance"].evaluate(ratio)


def should_spawn(seed, group_max, sentinel_max, comets):
    u = U(seed)
    cap, chance = spawn_chance(group_max, sentinel_max, comets)
    draw = u.rng(0.0, 1.0)
    return {"cap": cap, "chance": chance, "draw": draw, "result": bool(draw < chance), "state": u.state()}


def weighted_type(seed):
    u = U(seed)
    total = sum(t["chanceWeight"] for t in TYPES)
    n2 = u.rngi(0, total)
    count = len(TYPES)
    idx = None
    for c in range(count - 1, -1, -1):
        w = TYPES[c]["chanceWeight"]
        if n2 >= w:
            n2 -= w
            continue
        idx = c
        break
    if idx is None:
        idx = n2  # the C# keeps the remaining n2 as the index (never reached: the weights sum to `total`)
    return {"seed": seed, "index": idx, "name": TYPES[idx]["name"], "state": u.state()}


def home_spawn_distance(u, instance):
    if instance:
        return HOME_SMA * float(u.rng(MANAGER["minSpawnDistance"], MANAGER["maxSpawnDistance"]))
    return 13600000000.0 * float(u.rng(1.5, 2.0))  # FlightGlobals.ready is false in the fixture of the instance-less case


def sentinel_spawn_distance(u, distance, instance):
    if instance:
        return distance * float(u.rng(1.1, 2.0))  # minSentinelSpawnDistance / maxSentinelSpawnDistance are never loaded from the cfg
    return distance * float(u.rng(1.1, 1.5))


CLASS_RADIUS = {0: 3.24, 1: 5.4, 2: 9.0, 3: 15.0, 4: 25.0, 5: 41.7, 6: 69.5, 7: 115.8, 8: 193.0}


def class_radius(u, cls, seed):
    """DiscoveryInfo.GetClassRadius (cs:362): InitState(seed) then `num * Random.Range(0.75f, 1.25f)` (float * float, stored)"""
    u.init(seed)
    num = st(CLASS_RADIUS.get(cls, 0.0))
    return st(num * u.rng(0.75, 1.25))


def gen_definition(type_name, cls, seed, max_geysers, max_dust):
    """CometManager.GenerateDefinition (cs:558)"""
    t = TYPE_BY_NAME[type_name]
    u = U(0)
    radius = class_radius(u, cls, seed)
    g = kspr(seed)
    m = MANAGER

    def ev(name):
        return CURVES[name].evaluate(st(g.sample()))

    vfx = m["minVFXStartDistance"] + float(ev("vfxStartDistribution")) * (m["maxVFXStartDistance"] - m["minVFXStartDistance"])
    coma = m["minComaSizeRatio"] + float(ev("comaSizeRatioDistribution")) * (m["maxComaSizeRatio"] - m["minComaSizeRatio"])
    tail = float(ev("tailWidthDistribution")) * m["maxTailWidthRatio"]
    taillen = m["minTailLengthRatio"] + float(ev("tailLengthRatioDistribution")) * (m["maxTailLengthRatio"] - m["minTailLengthRatio"])
    ion = float(ev("tailWidthDistribution")) * m["maxIonTailWidthRatio"]
    num = m["minGeyserRadiusRatio"] + float(ev("geysersRatioDistribution")) * (m["maxGeyserRadiusRatio"] - m["minGeyserRadiusRatio"])
    geysers = int(num * float(radius))
    geysers = min(geysers, max_geysers)
    num2 = m["minNearDustEmitterRadiusRatio"] + g.sample() * (m["maxNearDustEmitterRadiusRatio"] - m["minNearDustEmitterRadiusRatio"])
    dust = int(num2 * float(radius))
    geysers = min(geysers, max_dust)  # sic: numGeysers is clamped twice, numNearDustEmitters never
    return {"typeName": t["name"], "vfxStartDistance": vfx, "comaWidthRatio": coma, "tailWidthRatio": tail, "tailMaxLengthRatio": taillen, "ionTailWidthRatio": ion,
            "radius": radius, "seed": seed, "numGeysers": geysers, "numNearDustEmitters": dust}


# ---------------------------------------------------------------------------------------------------------------- orbit maths (host libm)
def nan_or(f, *a):
    try:
        return f(*a)
    except (ValueError, ZeroDivisionError, OverflowError):
        return math.nan


def clamp_two_pi(x):
    x = math.fmod(x, 2 * PI)
    return x + 2 * PI if x < 0 else x


def orbit_functions(e, sma):
    p = sma * (1.0 - e * e)

    def true_anomaly_at_radius_simple(R):
        return nan_or(lambda: math.acos(p / (R * e) - 1.0 / e))

    def ecc_anomaly(ta):
        c, s = math.cos(ta / 2.0), math.sin(ta / 2.0)
        if e < 1.0:
            return 2.0 * math.atan2(math.sqrt(1.0 - e) * s, math.sqrt(1.0 + e) * c)
        n3 = math.sqrt((e - 1.0) / (e + 1.0)) * s / c
        if n3 >= 1.0:
            return math.inf
        if n3 <= -1.0:
            return -math.inf
        return math.log((1.0 + n3) / (1.0 - n3))

    def mean_anomaly(E):
        if e < 1.0:
            return clamp_two_pi(E - e * math.sin(E))
        if math.isinf(E):
            return E
        return nan_or(lambda: e * math.sinh(E) - E)

    def true_anomaly(E):
        if e < 1.0:
            c, s = math.cos(E / 2.0), math.sin(E / 2.0)
            return 2.0 * math.atan2(math.sqrt(1.0 + e) * s, math.sqrt(1.0 - e) * c)
        if math.isinf(E) and E > 0:
            return math.acos(-1.0 / e)
        if math.isinf(E):
            return -math.acos(-1.0 / e)
        return 2.0 * math.atan(math.sqrt((e + 1.0) / (e - 1.0)) * math.tanh(E / 2.0))

    return true_anomaly_at_radius_simple, ecc_anomaly, mean_anomaly, true_anomaly


def calc_orbit(u, t, home, sentinel, min_dist, instance=True):
    """CometOrbitType.CalculateOrbit (cs:101). sentinel = dict(LAN, argPe, trueAnomaly, radius) or None. Returns the draw-level orbit."""
    pe_min, pe_max, ap_min, ap_max = t["peMin"], t["peMax"], t["apMin"], t["apMax"]
    if not t["useEccentricity"]:
        n = float(u.rng(pe_min, pe_max))
        n2 = float(u.rng(ap_min, ap_max))
        n3 = n * PERT_SMA
        n4 = n2 * PERT_SMA
        sma = (n3 + n4) / 2.0
        ecc = 1.0 - n3 / sma
    else:
        n3 = float(u.rng(pe_min, pe_max)) * PERT_SMA
        ecc = float(u.rng(t["eccentricityMin"], t["eccentricityMax"]))
        n4 = n3 / (1.0 - ecc)
        sma = (n3 + n4) / 2.0
    r0 = u.rngi(0, 1)
    if not (float(r0) < t["inclination1Chance"]):
        inc = float(u.rng(t["inclination2Min"], t["inclination2Max"]))
    else:
        inc = float(u.rng(t["inclination1Min"], t["inclination1Max"]))
    lan = float(u.rng(0.0, 360.0))
    out = {"inc": inc, "ecc": ecc, "sma": sma, "lan": lan, "valid": True}
    tar, eca, mea, tra = orbit_functions(ecc, sma)
    ap_r, pe_r = (1.0 + ecc) * sma, (1.0 - ecc) * sma
    if home:
        val_ = home_spawn_distance(u, instance)
        val_ = min(val_, min_dist)
        ta = tar(val_)
        ea = nan_or(eca, ta) if not math.isnan(ta) else math.nan
        ma = 0.0 - mea(ea) if not math.isnan(ea) else math.nan
        if math.isnan(ma):
            ma = -PI / 2.0
        out["lan_final"] = lan
        out["mna"] = ma
    else:
        val2 = sentinel_spawn_distance(u, sentinel["radius"], instance)
        val2 = max(val2, min_dist)
        if val2 > ap_r and ecc < 1.0:
            out["valid"] = False
        else:
            if val2 < pe_r:
                ea = float(u.rng(st(PI) / 4.0, st(PI) / 2.0))
                ta = tra(ea)
            else:
                ta = tar(val2)
                ea = eca(ta) if not math.isnan(ta) else math.nan
            ma = 0.0 - mea(ea) if not math.isnan(ea) else math.nan
            if inc > 90.0 or inc < -90.0:
                ta *= -1.0
            if math.isnan(ma):
                ma = 0.0 - mea(-PI / 2.0)
            num8 = (sentinel["LAN"] + sentinel["argPe"]) * (PI / 180.0) + sentinel["trueAnomaly"] + ta
            num9 = st(sentinel["radius"] / val2 * math.sin(float(st(180.0 - 70.0)) * PI / 180.0))
            num8 += float(u.rng(st(0.0 - num9), num9))
            num8 *= 180.0 / PI
            out["lan_final"] = num8
            out["mna"] = ma
    out["state"] = u.state()
    return out


def random_obj_class(u, t):
    return u.rngi(t["minCometClass"], t["maxCometClass"] + 1)


# ---------------------------------------------------------------------------------------------------------------- names
def asteroid_name(u):
    text = ""
    for _ in range(3):
        text += chr(ord("A") + u.rngi(0, 26))
    text += "-"
    text += "%03d" % u.rngi(0, 1000)
    return LOC.fmt("#autoLOC_6001923", text)


def comet_name(seed, names, special, comet_names):
    """DiscoverableObjectsUtil.GenerateCometName with a KSPRandom(seed) and the game's comet name dictionary (insertion ordered dict name -> count)"""
    k = kspr(seed)
    cn = dict(comet_names)
    have_game = comet_names is not None

    def numbered(n):
        # Game.GetCometNumberedName: `cometNames[name] = cometNames[name] + 1` is the DictionaryValueList indexer set (DictionaryValueList.cs:15), which removes the key
        # and appends it again: an incremented name moves to the END of the key list (the next index draw addresses another name)
        if n in cn:
            v = cn.pop(n)
            cn[n] = v + 1
        else:
            cn[n] = 1
        return n + " " + str(cn[n])

    if have_game:
        num = 1.0 - float(len(cn)) / 10.0
        if k.sample() > num:
            index = k.next_range(0, len(cn) - 1)
            return numbered(list(cn.keys())[index]), cn
    if k.next_range(0, 9) < 5:
        arr, arr2 = special["male"], names["male"]
    else:
        arr, arr2 = special["female"], names["female"]

    def pick():
        if k_next_max(k, 20) != 0:
            return arr2[k_next_max(k, len(arr2))]
        return arr[k_next_max(k, len(arr))]

    empty = pick()
    if k.next_range(0, 100) < 24:
        empty = empty + "-" + pick()
        if k.next_range(0, 100) < 24:
            empty = empty + "-" + pick()
    if have_game:
        return numbered(empty), cn
    return empty + " 1", cn


# ---------------------------------------------------------------------------------------------------------------- ScenarioDiscoverableObjects
SIZE_CURVE = wr.Curve([(0.0, 0.0, 0.0, 0.0, False), (0.3, 0.45, 0.0, 0.0, False), (0.7, 0.55, 0.0, 0.0, False), (1.0, 1.0, 0.0, 0.0, False)])


def m_round_to_int(x):
    return int(np.rint(np.float64(x)))


def size_curve_class(u, min_range, max_range, cmin, cmax):
    num = cmax - cmin
    t = st(min(max(float(u.rng(min_range, max_range)), 0.0), 1.0))
    num2 = SIZE_CURVE.evaluate(t)
    return cmin + m_round_to_int(st(num2 * float(num)))


DRES = BD["Dres"]


def spawn_asteroid_case(seed_state_seed, reached_dres, cmin, cmax, minlife, maxlife, spawn_seed=None):
    """ScenarioDiscoverableObjects.SpawnAsteroid() (cs:284): the Unity generator starts at InitState(seed_state_seed) (the Start() of lastSeed)"""
    u = U(seed_state_seed)
    num = u.rngi(0, 2147483647) if spawn_seed is None else spawn_seed
    if spawn_seed is not None:
        pass
    u.init(num)
    dres = reached_dres and abs(int(math.fmod(num, 3))) == 0
    out = {"seed": num, "dres": dres}
    if dres:
        text = asteroid_name(u)
        cls = size_curve_class(u, 0.5, 1.0, cmin, cmax)
        life = float(u.rng(minlife, maxlife)) * 24.0 * 60.0 * 60.0
        life_max = float(maxlife) * 24.0 * 60.0 * 60.0
        n = (DRES["soi"] - DRES["radius"]) / 2.0
        min_alt = DRES["radius"] + n * 1.100000023841858
        max_alt = DRES["radius"] + n * 1.25
        ecc = float(u.rng(0.0001, 0.01))
        sma = float(u.rng(st(min_alt), st(max_alt)))
        inc = float(u.rng(-0.001, 0.001))
        lan = float(u.rng(0.999, 1.001))
        argpe = float(u.rng(0.999, 1.001))
        u.rng(0.999, 1.001)  # meanAnomalyAtEpoch (overwritten below)
        epoch = float(u.rng(0.999, 1.001))
        mna = float(st(u.value() * 2.0)) * PI
        out["orbit"] = {"ecc": ecc, "sma": sma, "inc": inc, "lan": lan, "argpe": argpe, "mna": mna, "epoch": epoch}
    else:
        u.rng(15.0, 60.0)  # GetRandomDuration
        text = asteroid_name(u)
        cls = size_curve_class(u, 0.0, 1.0, cmin, cmax)
        life = float(u.rng(minlife, maxlife)) * 24.0 * 60.0 * 60.0
        life_max = float(maxlife) * 24.0 * 60.0 * 60.0
        u.rng(0.0, 1.1)  # periapsis
        u.rng(100.0, 500.0)  # deltaVatPeriapsis
        u.draw(2)  # onUnitSphere
        u.draw(2)  # onUnitSphere
    u.draw(4)  # ProtoVessel.CreateVesselNode: Random.rotation
    out.update({"name": text, "class": cls, "lifeTime": life, "lifeTimeMax": life_max, "state": u.state()})
    return out


def spawn_comet_case(seed_state_seed, type_name, minlife, maxlife, max_geysers, max_dust):
    """ScenarioDiscoverableObjects.SpawnComet(CometOrbitType) (cs:386) with a CometManager instance"""
    t = TYPE_BY_NAME[type_name]
    u = U(seed_state_seed)
    seed = u.rngi(0, 2147483647)
    u.init(seed)
    life = float(u.rng(minlife, maxlife)) * 24.0 * 60.0 * 60.0
    life_max = float(maxlife) * 24.0 * 60.0 * 60.0
    cls = random_obj_class(u, t)
    definition = gen_definition(type_name, cls, seed, max_geysers, max_dust)
    class_radius(u, cls, seed)  # GetClassRadius inside GenerateDefinition: InitState(seed) + Range(0.75f, 1.25f) re-seed the global generator
    orbit = calc_orbit(u, t, True, None, 1.7976931348623157e308)
    u.draw(4)  # ProtoVessel.CreateVesselNode: Random.rotation
    return {"seed": seed, "lifeTime": life, "lifeTimeMax": life_max, "class": cls, "definition": definition, "orbit": orbit, "state": u.state()}


# ---------------------------------------------------------------------------------------------------------------- tables
def main():
    out = {}
    out["provenance"] = {
        "cfg": "core/testdata/GameData-mirror/GameData/Squad/Resources/CometDefs.cfg",
        "bodies": "harness/golden/serialized/celestial_bodies.serialized.json",
        "homeSma": HOME_SMA, "perturber": PERTURBER, "perturberSma": PERT_SMA,
    }
    out["manager"] = dict(MANAGER)
    out["curves"] = {name: curve_table(c) for name, c in CURVES.items()}
    out["ionTailColors"] = [{"color": c, "weight": w} for c, w in ION]
    out["dustTailColors"] = [{"color": c, "weight": w} for c, w in DUST]
    out["ionTailColorP67"] = ION_P67
    tc = []
    for seed in [1, 2, 3, 77, 1633779195, 4242, 99999, 2147483646]:
        k = kspr(seed)
        tot = sum(w for _, w in ION)
        n2 = k.next_range(0, tot)
        idx = None
        for c in range(len(ION) - 1, -1, -1):
            if n2 >= ION[c][1]:
                n2 -= ION[c][1]
                continue
            idx = c
            break
        k = kspr(seed)
        tot2 = sum(w for _, w in DUST)
        m2 = k.next_range(0, tot2)
        idx2 = None
        for c in range(len(DUST) - 1, -1, -1):
            if m2 >= DUST[c][1]:
                m2 -= DUST[c][1]
                continue
            idx2 = c
            break
        tc.append({"seed": seed, "ion": ION_P67 if seed == 1633779195 else ION[idx][0], "dust": DUST[idx2][0]})
    out["tailColorPick"] = tc
    out["limits"] = {"min": int(val(LIMITS, "homeBodySpawnGroupMinLimit")), "max": int(val(LIMITS, "homeBodySpawnGroupMaxLimit")),
                     "perSentinel": int(val(LIMITS, "perSentinelSpawnGroupMaxLimit"))}
    out["types"] = TYPES

    sc = []
    for group_max in (None, 3, 10, 40, 100):
        for sentinel_max in (None, 0, 10, 50):
            for comets in (0, 1, 2, 3, 5, 8):
                cap, chance = spawn_chance(group_max, sentinel_max, comets)
                sc.append({"groupMax": group_max, "sentinelMax": sentinel_max, "comets": comets, "cap": cap, "chance": chance})
    out["spawnChance"] = sc
    ss = []
    for seed in range(1, 41):
        for (g, s, c) in ((10, None, 0), (10, 30, 1), (3, 10, 3), (None, None, 0)):
            r = should_spawn(seed * 7919, g, s, c)
            r.update({"seed": seed * 7919, "groupMax": g, "sentinelMax": s, "comets": c})
            ss.append(r)
    out["shouldSpawn"] = ss
    out["weightedType"] = [weighted_type(s * 104729 + 3) for s in range(0, 200)]
    sd = []
    for seed in range(1, 25):
        u = U(seed * 31)
        home = home_spawn_distance(u, True)
        s1 = sentinel_spawn_distance(u, 2.5e10 + seed * 1.0e9, True)
        u2 = U(seed * 37)
        home0 = home_spawn_distance(u2, False)
        s0 = sentinel_spawn_distance(u2, 2.5e10 + seed * 1.0e9, False)
        sd.append({"seed": seed, "home": home, "sentinel": s1, "sentinelInput": 2.5e10 + seed * 1.0e9, "homeNoInstance": home0, "sentinelNoInstance": s0, "state": u2.state(),
                   "seedHome": seed * 31, "seedNoInstance": seed * 37})
    out["spawnDistance"] = sd

    cr = []
    for cls in range(9):
        for seed in (1, 77, 12345, 2147483646):
            u = U(0)
            cr.append({"class": cls, "seed": seed, "radius": class_radius(u, cls, seed), "state": u.state()})
    out["classRadius"] = cr

    defs = []
    for t in TYPES:
        for cls in range(9):
            for seed in (3, 99, 4242, 65537, 987654321):
                for (mg, md) in ((50, 50), (0, 0), (2, 1)):
                    d = gen_definition(t["name"], cls, seed, mg, md)
                    d.update({"class": cls, "maxGeysers": mg, "maxDust": md})
                    defs.append(d)
    out["definitions"] = defs

    orbs = []
    for t in TYPES:
        for seed in range(1, 41):
            u = U(seed * 1013)
            o = calc_orbit(u, t, True, None, 1.7976931348623157e308)
            o.update({"type": t["name"], "seed": seed * 1013, "home": True})
            orbs.append(o)
    # Sentinel branch: SYNTHETIC sentinel orbit fields (LAN, argumentOfPeriapsis, trueAnomaly, radius), min discovery distances around the perturber's sma
    sent = []
    for t in TYPES:
        for seed in range(1, 41):
            sen = {"LAN": (seed * 17) % 360 + 0.5, "argPe": (seed * 29) % 360 + 0.25, "trueAnomaly": (seed * 0.37) % 6.28, "radius": 1.2e10 + seed * 4.0e9}
            md = [5.0e9, 1.0e10, 3.0e10, 9.0e10][seed % 4]
            u = U(seed * 2003)
            o = calc_orbit(u, t, False, sen, md)
            o.update({"type": t["name"], "seed": seed * 2003, "home": False, "sentinel": sen, "minDiscovery": md})
            sent.append(o)
    out["orbits"] = orbs
    out["sentinelOrbits"] = sent

    out["asteroidNames"] = []
    for seed in range(1, 60):
        u = U(seed * 613)
        out["asteroidNames"].append({"seed": seed * 613, "name": asteroid_name(u), "state": u.state()})

    names = {"male": ["Jebediah", "Bob", "Bill", "Valentina", "Gene", "Walt"], "female": ["Valentina", "Wehrner", "Jeb", "Nina", "Tam"]}
    special = {"male": ["Zim", "Ion"], "female": ["Zel", "Zow", "Zan"]}
    cn = []
    for seed in range(1, 61):
        for state in ({}, {"Orion": 1}, {"Orion": 2, "Zim": 1, "Gene": 4}):
            n, after = comet_name(seed * 211, names, special, state)
            cn.append({"seed": seed * 211, "dict": [[k, v] for k, v in state.items()], "name": n, "after": [[k, v] for k, v in after.items()]})
    # the Game == null path (no HighLogic.CurrentGame)
    out["cometNamesNoGame"] = [{"seed": s * 331, "name": comet_name_nogame(s * 331, names, special)} for s in range(1, 40)]
    out["cometNames"] = cn
    out["cometNameTables"] = {"names": names, "special": special}

    sizes = []
    for seed in range(1, 31):
        for (cmin, cmax) in ((0, 4), (0, 8), (2, 6)):
            for (lo, hi) in ((0.0, 1.0), (0.5, 1.0)):
                u = U(seed * 71)
                c = size_curve_class(u, lo, hi, cmin, cmax)
                sizes.append({"seed": seed * 71, "min": cmin, "max": cmax, "lo": lo, "hi": hi, "class": c, "state": u.state()})
    out["sizeClass"] = sizes
    out["sizeCurve"] = curve_table(SIZE_CURVE)

    sa = []
    for k in range(1, 45):
        seed0 = k * 9973
        for reached in (False, True):
            sa.append(dict(spawn_asteroid_case(seed0, reached, 0, 4, 1.0, 20.0), seed0=seed0, reached=reached, cmin=0, cmax=4, minlife=1.0, maxlife=20.0))
    out["spawnAsteroid"] = sa
    sc2 = []
    for t in TYPES:
        for k in range(1, 16):
            seed0 = k * 7919
            sc2.append(dict(spawn_comet_case(seed0, t["name"], 1.0, 20.0, 50, 50), seed0=seed0, type=t["name"]))
    out["spawnComet"] = sc2

    out["dres"] = {"radius": DRES["radius"], "soi": DRES["soi"]}
    json.dump(sanitize(out), sys.stdout, indent=None, separators=(",", ":"), allow_nan=False)


def sanitize(o):
    """non-finite floats as the strings "nan" / "inf" / "-inf" (JSON has no such numbers)"""
    if isinstance(o, (np.floating, np.integer)):
        o = o.item()
    if isinstance(o, float):
        if math.isnan(o):
            return "nan"
        if math.isinf(o):
            return "inf" if o > 0 else "-inf"
        return o
    if isinstance(o, dict):
        return {k: sanitize(v) for k, v in o.items()}
    if isinstance(o, (list, tuple)):
        return [sanitize(v) for v in o]
    return o


def comet_name_nogame(seed, names, special):
    k = kspr(seed)
    if k.next_range(0, 9) < 5:
        arr, arr2 = special["male"], names["male"]
    else:
        arr, arr2 = special["female"], names["female"]

    def pick():
        if k_next_max(k, 20) != 0:
            return arr2[k_next_max(k, len(arr2))]
        return arr[k_next_max(k, len(arr))]

    empty = pick()
    if k.next_range(0, 100) < 24:
        empty = empty + "-" + pick()
        if k.next_range(0, 100) < 24:
            empty = empty + "-" + pick()
    return empty + " 1"


if __name__ == "__main__":
    main()
