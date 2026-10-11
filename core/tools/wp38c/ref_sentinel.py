"""WP-38 slice 3 reference, part 2: the Sentinel mission (SentinelMission/*.cs).

Independent of kspcore. It re-uses the independent WP-37 / WP-38 / WP-38b reference modules (cfg parser, Localizer, UnityEngine.Random, KSPRandom,
Contract.Set*, agents / mentalities, TextGen, progress scenario model, body table) and the ContractDefs reference of this package for the loaded
Contracts.cfg values (ContractDefs.Sentinel.* come from the cfg; ContractDefs.CometDetection.* are the C# initialisers because LoadConfig never reads
that section).

Fixtures that are not KSP data: ScenarioDiscoverableObjects is absent (Instance == null: asteroid class limits A..I), no contract exists in the
ContractSystem, the part database is the mirror's (InfraredTelescope exists), UnityEngine.Random is seeded with the contract seed.
"""
import json
import math
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
for sub in ("wp37", "wp38", "wp38b"):
    sys.path.insert(0, os.path.join(CORE, "tools", sub))
import contract_reference as w37  # noqa: E402
import fineprint_reference as w38  # noqa: E402
import fineprint2_reference as w38b  # noqa: E402
import ref_contractdefs as rcd  # noqa: E402

KEY = "sentinel"
F = w37.F
LOC = w38b.LOC
BD = w38.BD
HOME = w38.HOME
KSPRandom = w38.KSPRandom
BODY_LIST = w37.BODY_LIST
PI = math.pi

SCENARIOS3 = w38b.SCENARIOS + [
    {"name": "sun-only", "top": {"FirstLaunch": "completed", "ReachedSpace": "completed"}, "bodies": {"Sun": {"Orbit": "completed"}}},
    {"name": "sun-kerbin", "top": {"FirstLaunch": "completed", "ReachedSpace": "completed"},
     "bodies": {"Kerbin": {"Science": "completed", "Orbit": "completed"}, "Sun": {"Orbit": "completed"}}},
    {"name": "sun-far", "top": {"FirstLaunch": "completed", "ReachedSpace": "completed"},
     "bodies": {"Kerbin": {"Orbit": "completed"}, "Sun": {"Orbit": "completed"}, "Moho": {"Flyby": "completed"}, "Jool": {"Flyby": "completed"},
                "Eeloo": {"Flyby": "completed"}}},
]


# ---------------------------------------------------------------------------------------------------------------- defaults / cfg values
def defs(cs_root, mirror):
    cs = rcd.CS(cs_root + "/FinePrint/ContractDefs.cs")
    d = rcd.parse_defaults(cs)
    contracts = rcd.get_node(rcd.parse_cfg(os.path.join(mirror, "Squad", "Contracts", "Contracts.cfg")), "Contracts")
    loaded, _ = rcd.load_config(cs, d, contracts, False)
    return loaded, d


# ---------------------------------------------------------------------------------------------------------------- bodies
def body_orbits():
    r = json.load(open(w37.BODIES, encoding="utf-8"))
    out = {}
    for e in r["bodies"]:
        o = (e.get("orbitDriver") or {}).get("orbit")
        if o:
            out[e["celestialBody"]["bodyName"]] = {"inc": o["inclination"], "ecc": o["eccentricity"], "sma": o["semiMajorAxis"], "lan": o["LAN"],
                                                   "argpe": o["argumentOfPeriapsis"], "mep": o["meanAnomalyAtEpoch"], "epoch": o["epoch"]}
    return out


ORB = body_orbits()


def display(name):
    return w38b.BD_DISPLAY(name)


def find_inner_outer(sma):
    """SentinelUtilities.FindInnerAndOuterBodies(double): (inner, outer, found) or None for the ArgumentOutOfRangeException"""
    d = {0.0: "Sun"}
    for b in sorted(BD.values(), key=lambda x: x["index"]):
        if b["name"] != "Sun" and b["parent"] == "Sun":
            assert b["sma"] not in d
            d[b["sma"]] = b["name"]
    keys = sorted(d)
    num = 0
    while True:
        if num < len(keys):
            if keys[num] <= sma:
                if num + 1 >= len(keys):
                    return None
                if not (keys[num + 1] <= sma):
                    break
            num += 1
            continue
        return (d[keys[0]], d[keys[-1]], False)
    return (d[keys[num]], d[keys[num + 1]], True)


def is_on_solar_orbit(name):
    b = BD[name]
    if name == "Sun":
        return False
    # referenceBody: the parent, the body itself for the Sun (checked above)
    return b["parent"] == "Sun"


def view_angle(inner_sma, outer_sma, vang=200.0):
    num = inner_sma / 2.0
    num2 = outer_sma / 2.0
    num3 = PI / 180.0 * ((360.0 - vang) / 2.0)
    arg = num / num2 * math.sin(num3)
    if abs(arg) > 1.0:  # Math.Asin(|x| > 1) = NaN; Math.Max(NaN, y) = NaN and both comparisons are false: the result is NaN (JSON null)
        return None
    num4 = math.asin(arg)
    num5 = PI - num4
    val = (PI - num3 - num4) * (180.0 / PI)
    val2 = (PI - num3 - num5) * (180.0 / PI)
    n6 = max(val, val2) * 2.0
    if n6 > vang:
        n6 = vang
    if n6 < vang / 100.0:
        n6 = vang / 100.0
    return n6


def escape_velocity(body, altitude):
    b = BD[body]
    if body != "Sun":
        if altitude <= b["soi"]:
            return math.sqrt(2.0 * b["mu"] / altitude - b["mu"] / (b["radius"] + b["soi"]))
        return 0.0
    return math.sqrt(2.0 * b["mu"] / altitude)


def min_orbital_speed(body):
    b = BD[body]
    num = w38.solar_extents() if body == "Sun" else b["soi"]
    return math.sqrt(b["mu"] / num)


def read_duration(s):
    return F(F(F(float(len(s)) / 5.0) / 200.0) * 60.0)


def weighted_class(g, lo=0, hi=8):
    num3 = hi - lo
    while num3 > 0 and g.next_max(100) > 50:
        num3 -= 1
    return num3 + lo


def utilities():
    out = {}
    out["findInner"] = []
    smas = [-1.0, 0.0, 1.0]
    for b in BD.values():
        if b["name"] != "Sun" and b["parent"] == "Sun":
            smas += [b["sma"] - 1.0, b["sma"], b["sma"] + 1.0]
    smas += [4.0e10, 9.0e10, 9.1e10, 1.0e12]
    for s in smas:
        out["findInner"].append({"sma": s, "result": find_inner_outer(s)})
    out["viewAngle"] = []
    planets = [b for b in BD.values() if b["name"] != "Sun" and b["parent"] == "Sun"]
    for a in planets:
        for b in planets:
            out["viewAngle"].append({"inner": a["name"], "outer": b["name"], "angle": view_angle(a["sma"], b["sma"])})
    out["escape"] = []
    for name in ("Sun", "Kerbin", "Mun", "Jool", "Eeloo"):
        b = BD[name]
        for alt in (b["radius"] + 1.0, b["radius"] * 2.0, 1.0e6, 1.0e9, 1.0e11):
            out["escape"].append({"body": name, "altitude": alt, "v": escape_velocity(name, alt)})
    out["minSpeed"] = [{"body": n, "v": min_orbital_speed(n)} for n in BD if n != "Sun" or True]
    out["solarOrbit"] = [{"body": b["name"], "v": is_on_solar_orbit(b["name"])} for b in BODY_LIST]
    out["readDuration"] = [{"text": t, "v": read_duration(t)} for t in ("", "a", "hello world", "x" * 199, "The Sentinel " * 40)]
    rng = []
    for seed in (0, 1, 7, 1234, -5, 987654321):
        g = KSPRandom(seed)
        r = {"seed": seed}
        r["scan"] = [g.next_range(0, 4) for _ in range(6)]
        r["class"] = [weighted_class(g) for _ in range(6)]
        r["classLimited"] = [weighted_class(g, 2, 5) for _ in range(6)]
        r["wr"] = [math.pow(g.next_double(), 3) for _ in range(4)]
        r["wrRange"] = [0.05 + math.pow(g.next_double(), 3) * (0.4 - 0.05) for _ in range(4)]
        r["wrMax"] = [0.0 + math.pow(g.next_double(), 3) * (12.5 - 0.0) for _ in range(4)]
        r["range"] = [1.05 + g.next_double() * (1.2 - 1.05) for _ in range(4)]
        r["rangeSwapped"] = [(-3.0) + g.next_double() * (7.0 - (-3.0)) for _ in range(2)]
        out.setdefault("rng", []).append(r)
    return out


# ---------------------------------------------------------------------------------------------------------------- parameter
def round_even(x, digits=0):
    p = 10.0 ** digits
    return round(x * p) / p


def sentinel_param_title(focus, scan, count, size, ecc, inc):
    text = LOC.fmt("#autoLOC_6002303") if focus == HOME else LOC.fmt("#autoLOC_6002304")
    disp = display(focus)
    sz = "ABCDEFGHI"[size]
    if scan == 1:
        return LOC.fmt("#autoLOC_6002305", str(count), sz, text, disp)
    if scan == 2:
        return LOC.fmt("#autoLOC_6002306", str(count), text, disp, w38b.g15(round_even(ecc, 2)))
    if scan == 3:
        return LOC.fmt("#autoLOC_6002307", str(count), text, disp, w38b.g15(float(round(inc))))
    return LOC.fmt("#autoLOC_6002308", str(count), text, disp)


def parameter_texts():
    out = []
    for focus in ("Kerbin", "Duna", "Eve", "Sun", "Jool"):
        for scan in range(4):
            for count in (1, 3, 12):
                for size in (0, 4, 8):
                    for ecc, inc in ((0.1234, 5.5), (0.05, 12.5), (0.3999, 39.9999), (0.125, 13.5), (0.045, 0.0)):
                        out.append({"focus": focus, "scan": scan, "count": count, "size": size, "ecc": ecc, "inc": inc,
                                    "title": sentinel_param_title(focus, scan, count, size, ecc, inc)})
    return {"rows": out, "notes": LOC.fmt("#autoLOC_6002309"), "notesSpaceCenter": "\n" + LOC.fmt("#autoLOC_6002309")}


# ---------------------------------------------------------------------------------------------------------------- contracts
class Vals:
    def __init__(self, loaded, defaults):
        self.loaded = loaded
        self.defaults = defaults

    def v(self, path):
        t = self.defaults[path][0]
        x = self.loaded[path]
        if t == "float":
            return F(float(x))
        return x


def part_title():
    txt = open(os.path.join(w37.SQUAD, "Parts", "Misc", "AsteroidDay", "CamSat.cfg"), encoding="utf-8").read()
    m = re.search(r"^title\s*=\s*(#autoLOC_\d+)", txt, re.M)
    return LOC.fmt(m.group(1))


PART_TITLE = part_title()


def reached(s, name):
    return s.reached(name)


def bodies_reached(s, include_kerbin, include_sun):
    spawn = sorted(BD.values(), key=lambda b: b["spawn"])
    lst = []
    for i, b in enumerate(spawn):
        if (i != 0 or include_sun) and (i != 1 or include_kerbin) and reached(s, b["name"]):
            lst.append(b["name"])
    w38.mono_sort(lst, lambda a, b: (BD[a]["recovery"] > BD[b]["recovery"]) - (BD[a]["recovery"] < BD[b]["recovery"]))
    return lst


def rti(x):
    return w37.round_to_int(F(x))


def gen_sentinel(V, seed, p, s, mult, guid):
    if not s.node_complete("Sun", "Orbit"):  # MeetRequirements (GetOrbitDisplayMode(1.0) = PatchedConics, not < AllOrbits)
        return None
    if "InfraredTelescope" not in w38b.MIRROR_PARTS:
        return None
    rng = w37.Unity(seed)
    c = w38b.CC(p, mult)
    sseed = w38b.super_seed(guid, seed)
    g = KSPRandom(sseed)
    P = "Sentinel."
    pre = ["Trivial", "Significant", "Exceptional"][p]
    num4 = 1.0
    scan = 0
    size = 8
    ecc = inc = 0.0
    count = g.next_range(V.v(P + "Trivial.MinAsteroids"), V.v(P + "Trivial.MaxAsteroids"))

    def set_focus(body):
        if not is_on_solar_orbit(body):
            return "Sun", HOME
        r = find_inner_outer(ORB[body]["sma"] - 1.0)
        return r[0], r[1]

    if p == 2:
        lst = [n for n in bodies_reached(s, True, False) if BD[n]["parent"] == "Sun"]
        if not lst:
            return None
        inner, outer = set_focus(lst[g.next_range(0, len(lst))])
        count = g.next_range(V.v(P + "Exceptional.MinAsteroids"), V.v(P + "Exceptional.MaxAsteroids"))
        scan = g.next_range(0, 4)
    elif p == 1:
        inner, outer = set_focus(HOME)
        count = g.next_range(V.v(P + "Significant.MinAsteroids"), V.v(P + "Significant.MaxAsteroids"))
        scan = g.next_range(0, 4)
    else:
        inner, outer = set_focus(HOME)
    if scan == 0:
        num4 = V.v(P + "ScanTypeBaseMultiplier")
    elif scan == 1:
        size = weighted_class(g)
        n7 = 8.0
        num4 = F((n7 + 1.0 + float(size)) / ((n7 + 1.0) * 2.0))
        num4 = F(num4 * V.v(P + "ScanTypeBaseMultiplier"))
    elif scan == 2:
        ecc = 0.05 + math.pow(g.next_double(), 3) * (0.4 - 0.05)
        num4 = F(0.5 + ecc / 2.0)
        num4 = F(num4 * V.v(P + "ScanTypeEccentricityMultiplier"))
    else:
        inc = 5.0 + math.pow(g.next_double(), 3) * (40.0 - 5.0)
        num4 = F(0.5 + inc / 2.0)
        num4 = F(num4 * V.v(P + "ScanTypeInclinationMultiplier"))
    num4 = F(num4 * w37.m_round(F(float(count) / 2.0)))
    # CreateRandomOrbitNearby(outerBody.orbit): six UnityEngine.Random.Range(float, float) draws
    base = ORB[outer]
    e = base["ecc"] + float(rng.range_f(0.0001, 0.01))
    sma0 = base["sma"] * float(rng.range_f(0.999, 1.001))
    i_ = base["inc"] + float(rng.range_f(-0.001, 0.001))
    lan = base["lan"] * float(rng.range_f(0.999, 1.001))
    argpe = base["argpe"] * float(rng.range_f(0.999, 1.001))
    mep = base["mep"] * float(rng.range_f(0.999, 1.001))
    epoch = base["epoch"]
    rr = 1.05 + g.next_double() * (1.2 - 1.05)
    if inner == "Sun":
        sma = BD["Sun"]["minOrb"] * rr
    else:
        sma = ORB[inner]["sma"] * rr
    dev = V.v(P + pre + "Deviation")
    ss = w38b.ss_hash(guid, seed)
    params = [
        {"type": "PartRequestParameter", "hash": ss, "title": LOC.fmt("#autoLOC_7001088", PART_TITLE, LOC.fmt("#autoLOC_6002407"))},
        {"type": "SpecificOrbitParameter", "hash": ss, "title": w38b.orbit_title("RANDOM", "Sun", p),
         "orbit": {"inc": i_, "ecc": e, "sma": sma, "lan": lan, "argpe": argpe, "mep": mep, "epoch": epoch}, "deviation": float(dev)},
        {"type": "SentinelParameter", "hash": ss, "title": sentinel_param_title(outer, scan, count, size, ecc, inc)},
    ]
    kws = []
    w37.add_keywords(kws, "Pioneer", "Scientific")  # adds only the first keyword that is not in the list yet
    c.set_expiry_draw(rng.range_i(V.v(P + "Expire.MinimumExpireDays"), V.v(P + "Expire.MaximumExpireDays")))
    c.set_deadline_days(float(rti(F(F(float(V.v(P + "Expire.DeadlineDays"))) * num4))), inner)
    mm = lambda a, b: F(a * num4 * b)  # Mono rule: one rounding where RoundToInt takes the float
    c.set_science(float(rti(mm(V.v(P + "Science.BaseReward"), V.v(P + pre + ".ScienceMultiplier")))))
    c.set_rep(float(rti(mm(V.v(P + "Reputation.BaseReward"), V.v(P + pre + ".ReputationMultiplier")))),
              float(rti(mm(V.v(P + "Reputation.BaseFailure"), V.v(P + pre + ".ReputationMultiplier")))))
    c.set_funds(float(rti(mm(V.v(P + "Funds.BaseAdvance"), V.v(P + pre + ".FundsMultiplier")))),
                float(rti(mm(V.v(P + "Funds.BaseReward"), V.v(P + pre + ".FundsMultiplier")))),
                float(rti(mm(V.v(P + "Funds.BaseFailure"), V.v(P + pre + ".FundsMultiplier")))), inner)
    if outer == HOME:
        title = LOC.fmt("#autoLOC_6002297", str(count), display(outer), PART_TITLE)
        synopsis = LOC.fmt("#autoLOC_6002300", display(outer), LOC.fmt("#autoLOC_7001311"), PART_TITLE, display(inner))
    else:
        title = LOC.fmt("#autoLOC_6002298", str(count), display(outer), LOC.fmt("#autoLOC_7001310"), PART_TITLE)
        synopsis = LOC.fmt("#autoLOC_6002301", PART_TITLE, display(HOME), LOC.fmt("#autoLOC_7001310"), display(outer))
    completed = LOC.fmt("#autoLOC_6002302", display(outer), LOC.fmt("#autoLOC_7001310"))
    return w38b.finish("SentinelContract", p, seed, mult, guid, rng, kws, [], c, params, title, synopsis, LOC.fmt("#autoLOC_6002299"), display(outer),
                       (True, False, True),
                       {"innerBody": inner, "outerBody": outer, "asteroidCount": count, "scanType": scan, "targetSize": size, "minEcc": ecc, "minInc": inc,
                        "completed": completed})


def gen_comet_detection(V, seed, p, s, mult, guid):
    if not s.node_complete("Sun", "Orbit"):
        return None
    if "InfraredTelescope" not in w38b.MIRROR_PARTS:
        return None
    if p == 0:
        return None
    rng = w37.Unity(seed)
    c = w38b.CC(p, mult)
    D = V.defaults
    n = lambda path: (F(float(D[path][1])) if D[path][0] == "float" else D[path][1])
    P = "CometDetection."
    pre = "Significant" if p == 1 else "Exceptional"
    count = 3 if p == 2 else 2
    ss = w38b.ss_hash(guid, seed)
    params = [
        {"type": "PartRequestParameter", "hash": ss, "title": LOC.fmt("#autoLOC_7001088", PART_TITLE, LOC.fmt("#autoLOC_6002407"))},
        {"type": "EnterOrbit", "hash": "Sun", "body": "Sun"},
        {"type": "CometDetectionParameter", "hash": ss, "title": LOC.fmt("#autoLOC_6011147", str(count))},
    ]
    c.set_expiry_draw(rng.range_i(n(P + "Expire.MinimumExpireDays"), n(P + "Expire.MaximumExpireDays")))
    c.set_deadline_days(float(rti(float(n(P + "Expire.DeadlineDays")))))
    mm = lambda a, b: F(a * b)
    c.set_science(float(rti(mm(n(P + "Science.BaseReward"), n(P + pre + ".ScienceMultiplier")))))
    c.set_rep(float(rti(mm(n(P + "Reputation.BaseReward"), n(P + pre + ".ReputationMultiplier")))),
              float(rti(mm(n(P + "Reputation.BaseFailure"), n(P + pre + ".ReputationMultiplier")))))
    c.set_funds(float(rti(mm(n(P + "Funds.BaseAdvance"), n(P + pre + ".FundsMultiplier")))),
                float(rti(mm(n(P + "Funds.BaseReward"), n(P + pre + ".FundsMultiplier")))),
                float(rti(mm(n(P + "Funds.BaseFailure"), n(P + pre + ".FundsMultiplier")))), None)
    title = LOC.fmt("#autoLOC_6011148", str(count), PART_TITLE)
    synopsis = LOC.fmt("#autoLOC_6011146", str(count), PART_TITLE)
    completed = LOC.fmt("#autoLOC_6011145", PART_TITLE)
    return w38b.finish("CometDetectionContract", p, seed, mult, guid, rng, ["Scientific"], [], c, params, title, synopsis, "CometDetection",
                       display("Sun"), (False, False, False), {"cometCount": count, "completed": completed}, )


def build(cs_root, mirror):
    loaded, defaults = defs(cs_root, mirror)
    V = Vals(loaded, defaults)
    out = {"scenarios": SCENARIOS3, "util": utilities(), "paramTexts": parameter_texts(), "partTitle": PART_TITLE}
    W = w38b.weights()
    out["weights"] = W
    cases = []
    for ctype, fn in (("SentinelContract", gen_sentinel), ("CometDetectionContract", gen_comet_detection)):
        for si, scn in enumerate(SCENARIOS3):
            s = w38.Scn(scn["bodies"], scn["top"])
            for p in range(3):
                for seed in range(1, 25 if ctype == "SentinelContract" else 7):
                    for mi, mult in enumerate(w37.MULT_SETS):
                        if mi == 1 and seed > 6:
                            continue
                        sd = seed * 7001 + 13 + p
                        g = bytes(((b + seed * 3 + p) & 0xFF) for b in w38b.GUID)
                        r = fn(V, sd, p, s, mult, g)
                        cases.append({"type": ctype, "scenario": si, "prestige": p, "seed": sd, "mult": mi, "guid": list(g), "result": r})
    out["contracts"] = cases
    gen = sum(1 for c in cases if c["result"])
    print("sentinel contracts: %d cases, %d generated" % (len(cases), gen))
    return out
