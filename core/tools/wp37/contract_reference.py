#!/usr/bin/env python3
"""WP-37 independent reference: contract reward / deadline maths, mentality factors, contract counts, generation weights, TextGen strings and
the generated parameter tree of the PlantFlag and CollectScience templates.

No kspcore code. Inputs are the game's own data: GameData-mirror Contracts.cfg, Agents.cfg, StoryDefs.cfg and the en-us dictionary.cfg
(core/testdata), the body table of harness/golden/serialized/celestial_bodies.serialized.json. C# `float` arithmetic follows the Mono rule
(core/README.md): an expression is evaluated in double and rounded to float32 (numpy) only where the C# stores, passes, returns or casts it.
Python floats below always hold float32-exact values for C# floats unless a value is a C# double.

Spec (decompiled C#): Contracts/Contract.cs (SetFunds ..., SetExpiry, SetupID, Generate), ContractSystem.cs (GetContractCounts, weights,
WeightedBodyChoice), Contracts/Agents/*.cs, Contracts/TextGen.cs, Contracts/Templates/{PlantFlag,CollectScience}.cs,
FinePrint/Utilities/{ProgressMilestone,ProgressUtilities}.cs, KSP/Localization/Localizer.cs (+ the managed Lingoona fallback), KSPUtil.cs
(GenerateSuperSeed), StringExtensions.cs (GetHashCode_Net35), UnityEngine.Random (harness/captures/README.md C2).

Fixtures that are NOT KSP data (the Unity prefab values GameVariables carries are not in the repo, STATUS.md "needs data"): the GameVariables
contract / mentality factors below are the C# field initialisers of GameVariables.cs (the prefab serializes them; nothing in the repo
shows a different value), the Career difficulty multipliers (the second set is arbitrary and only exercises the multiplications), the
progress scenario and the generation GUIDs of the tree cases.

Usage: python -I core/tools/wp37/contract_reference.py        -> core/tests/data/wp37_reference.json
"""
import json
import math
import os
import re

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
ROOT = os.path.abspath(os.path.join(CORE, ".."))
if not os.path.isdir(os.path.join(ROOT, "harness", "golden")):
    ROOT = r"C:\Projects\KSP-UE5"
SQUAD = os.path.join(CORE, "testdata", "GameData-mirror", "GameData", "Squad")
BODIES = os.path.join(ROOT, "harness", "golden", "serialized", "celestial_bodies.serialized.json")
OUT = os.path.join(CORE, "tests", "data", "wp37_reference.json")

f32 = np.float32
M32 = 0xFFFFFFFF


def F(x):
    """round a C# float expression (evaluated in double) to float32 at a store/argument/return boundary"""
    return float(f32(x))


# ---------------------------------------------------------------------------------------------------------------- config files
def parse_cfg(path):
    text = open(path, encoding="utf-8-sig").read()
    root = {"name": "", "values": [], "nodes": []}
    stack = [root]
    pending = None
    for raw in text.splitlines():
        line = raw
        i = line.find("//")
        if i >= 0:
            line = line[:i]
        line = line.strip()
        if not line:
            continue
        if line == "{":
            node = {"name": pending, "values": [], "nodes": []}
            stack[-1]["nodes"].append(node)
            stack.append(node)
            pending = None
            continue
        if line == "}":
            stack.pop()
            continue
        if "=" in line:
            k, v = line.split("=", 1)
            stack[-1]["values"].append((k.strip(), v.strip()))
            continue
        pending = line
    return root


def child(node, name):
    for n in node["nodes"]:
        if n["name"] == name:
            return n
    return None


def val(node, key):
    for k, v in node["values"]:
        if k == key:
            return v
    return None


# ---------------------------------------------------------------------------------------------------------------- localisation
class Loc:
    def __init__(self):
        root = parse_cfg(os.path.join(SQUAD, "Localization", "dictionary.cfg"))
        en = child(child(root, "Localization"), "en-us")
        self.tags = {}
        for k, v in en["values"]:
            v = v.replace("\uff62", "{").replace("\uff63", "}")
            v = re.sub(r"\\[uU]([0-9A-F]{4})", lambda m: chr(int(m.group(1), 16)), v)
            self.tags[k] = v

    def single(self, s):
        return self.tags.get(s, s) if s else ""

    def fmt(self, template, *params):
        if not template:
            return ""
        if params:
            t = self.single(template)
            ps = [self.single(p) for p in params]

            def sub(m):
                n = int(m.group(1))
                if 1 <= n <= len(ps):
                    p = ps[n - 1]
                    k = p.find("^")
                    return p if k < 0 else p[:k]
                return ""

            r = re.sub(r"<<(\d+)>>", sub, t)
        else:
            r = self.single(template)
        return r.replace("\\n", "\n").replace('\\"', '"').replace("\\t", "\t")


LOC = Loc()


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


# ---------------------------------------------------------------------------------------------------------------- Mathf
def m_round(x):
    return F(np.rint(np.float64(x)))


def m_clamp01(t):
    return 0.0 if t < 0.0 else (1.0 if t > 1.0 else t)


def m_lerp(a, b, t):
    return F(a + (b - a) * m_clamp01(t))


def m_inverse_lerp(a, b, v):
    if a != b:
        return m_clamp01(F((v - a) / (b - a)))
    return 0.0


def m_floor(x):
    return F(math.floor(x))


def round_to_int(x):
    return int(np.rint(np.float64(x)))


# ---------------------------------------------------------------------------------------------------------------- GameVariables (C# field initialisers)
PRESTIGE = ["Trivial", "Significant", "Exceptional"]
GV = {
    "prestige": [1.0, 1.25, 1.5],
    "mentality": {"funds": [1.1, 1.2, 1.3], "rep": [1.1, 1.2, 1.3], "sci": [1.1, 1.2, 1.3], "expiry": [1.1, 1.2, 1.3], "deadline": [1.1, 1.2, 1.3]},
}
MULT_SETS = [
    {"fundsGain": 1.0, "fundsLoss": 1.0, "repGain": 1.0, "repLoss": 1.0, "sciGain": 1.0},
    {"fundsGain": 1.5, "fundsLoss": 0.75, "repGain": 0.5, "repLoss": 1.25, "sciGain": 2.0},
]


def contract_factors(p, mult):
    pf = GV["prestige"][p]
    return {
        "fa": F(F(pf) * 1.0 * mult["fundsGain"]),
        "fc": F(F(pf) * 1.0 * mult["fundsGain"]),
        "ff": F(F(pf) * 1.0 * mult["fundsLoss"]),
        "rc": F(F(pf) * 1.0 * mult["repGain"]),
        "rf": F(F(pf) * 1.0 * mult["repLoss"]),
        "sc": F(F(pf) * 1.0 * mult["sciGain"]),
    }


# ---------------------------------------------------------------------------------------------------------------- bodies
def load_bodies():
    r = json.load(open(BODIES, encoding="utf-8"))
    bs = []
    for e in r["bodies"]:
        cb = e["celestialBody"]
        bs.append({
            "name": cb["bodyName"], "index": e["flightGlobalsIndex"], "recovery": F(cb["scienceValues"]["RecoveryValue"]),
            "solid": bool(cb["hasSolidSurface"]), "ocean": bool(cb["ocean"]), "atmosphere": bool(cb["atmosphere"]), "home": bool(cb["isHomeWorld"]),
            "display": LOC.fmt(cb["bodyDisplayName"]),
        })
    bs.sort(key=lambda b: b["index"])
    return bs


BODY_LIST = load_bodies()
BODY = {b["name"]: b for b in BODY_LIST}

# ---------------------------------------------------------------------------------------------------------------- Contracts.cfg
CONTRACTS = child(parse_cfg(os.path.join(SQUAD, "Contracts", "Contracts.cfg")), "Contracts")


def cfg_f(section, sub, key):
    n = child(CONTRACTS, section)
    if sub:
        n = child(n, sub)
    return F(float(val(n, key)))


def cfg_i(section, sub, key):
    n = child(CONTRACTS, section)
    if sub:
        n = child(n, sub)
    return int(val(n, key))


DAY = 21600
YEAR = 9201600


# ---------------------------------------------------------------------------------------------------------------- Contract.Set* (Contract.cs)
class C:
    def __init__(self, prestige, mult, body_weight=None):
        self.p = prestige
        self.mult = mult
        self.fa = self.fc = self.ff = 0.0
        self.sc = self.rc = self.rf = 0.0
        self.expiry = 0.0
        self.deadline = 0.0

    def fac(self):
        return contract_factors(self.p, self.mult)

    def dest(self, body):
        return 1.0 if body is None else F(BODY[body]["recovery"] * 1.0)

    def set_funds(self, adv, comp, fail, body):
        n = self.dest(body)
        f = self.fac()
        self.fa = m_round(F(adv * f["fa"] * n))
        self.fc = m_round(F(comp * f["fc"] * n))
        self.ff = max(self.fa, m_round(F(fail * f["ff"] * n)))

    def set_funds2(self, adv, comp, body):
        n = self.dest(body)
        f = self.fac()
        self.fa = m_round(F(adv * f["fa"] * n))
        self.fc = m_round(F(comp * f["fc"] * n))
        self.ff = self.fa

    def set_rep(self, comp, fail, body=None):
        n = self.dest(body)
        f = self.fac()
        self.rc = m_round(F(comp * f["rc"] * n))
        self.rf = m_round(F(fail * f["rf"] * n))

    def set_science(self, comp, body=None):
        n = self.dest(body)
        self.sc = m_round(F(comp * self.fac()["sc"] * n))

    def set_deadline_days(self, days, body=None):
        n = self.dest(body)
        self.deadline = days * float(DAY) * n

    def set_deadline_years(self, years, body=None):
        n = self.dest(body)
        self.deadline = years * float(YEAR) * n

    def set_expiry_draw(self, draw):
        # (float)Random.Range(min, max) * (float)Day / 60f -> Mathf.Round -> * 60f (assigned to a double: exact product)
        self.expiry = m_round(F(float(draw) * float(DAY) / 60.0)) * 60.0


def day_year_expiry(rng, mn, mx):
    return rng.range_i(mn, mx)


# ---------------------------------------------------------------------------------------------------------------- mentalities (Contracts/Agents)
def mentality_def():
    return {
        "Commercial": dict(score={"Commercial": 1}, proc=[("fundsCompletion", True), ("fundsFailure", True), ("repCompletion", False)]),
        "Competitive": dict(score={"Commercial": 1}, proc=[]),
        "Conglomerate": dict(score={"Commercial": 2}, proc=[("expiry", False), ("deadline", False), ("fundsAdvance", True), ("fundsCompletion", True)]),
        "Cooperative": dict(score={"Commercial": 1}, proc=[]),
        "EasyGoing": dict(score={"Commercial": 1}, proc=[("repFailure", False), ("repCompletion", False)]),
        "Ecomental": dict(score={"Commercial": 1}, proc=[]),
        "Economic": dict(score={"Commercial": 1}, proc=[("fundsAdvance", True), ("fundsFailure", True), ("fundsCompletion", True), ("repCompletion", False)]),
        "Hasty": dict(score={"Commercial": 1}, proc=[("deadline", False), ("expiry", False), ("repCompletion", True)]),
        "Industrial": dict(score={"Commercial": 1}, proc=[]),
        "Kerbalminded": dict(score={"Commercial": 1}, proc=[]),
        "Moral": dict(score={}, proc=[("fundsAdvance", False), ("fundsCompletion", False), ("repCompletion", True)]),
        "NoRandomContractMentality": dict(score="neg", proc=[], can="never"),
        "Patient": dict(score={"Commercial": 1}, proc=[("deadline", True), ("expiry", True), ("repCompletion", False), ("repFailure", False)]),
        "Perfectionist": dict(score={"Commercial": 1}, proc=[]),
        "Pioneer": dict(score={"Commercial": 1}, proc=[]),
        "Record": dict(score="record", proc=[], can="record"),
        "Scientific": dict(score={"Scientific": 1}, proc=[], can="noRecord"),
        "Startup": dict(score={"Commercial": 1}, proc=[("fundsAdvance", False), ("fundsCompletion", False), ("expiry", False), ("deadline", False)]),
        "Stern": dict(score={"Commercial": 1}, proc=[("repFailure", True), ("repCompletion", True)]),
    }


MENT = mentality_def()


def mentality_factor(kind, factor, p):
    table = {"fundsAdvance": "funds", "fundsCompletion": "funds", "fundsFailure": "funds", "repCompletion": "rep", "repFailure": "rep",
             "sciCompletion": "sci", "deadline": "deadline", "expiry": "expiry"}[kind]
    return m_lerp(1.0, F(GV["mentality"][table][p]), factor)


def apply_mentality(contract, name, factor, p):
    d = MENT[name]
    for kind, positive in d["proc"]:
        num = mentality_factor(kind, factor, p)
        if not positive:
            num = F(1.0 / num)
        if kind == "fundsAdvance":
            contract["fa"] = contract["fa"] * num
        elif kind == "fundsCompletion":
            contract["fc"] = contract["fc"] * num
        elif kind == "fundsFailure":
            contract["ff"] = contract["ff"] * num
        elif kind == "repCompletion":
            contract["rc"] = F(contract["rc"] * num)
        elif kind == "repFailure":
            contract["rf"] = F(contract["rf"] * num)
        elif kind == "sciCompletion":
            contract["sc"] = F(contract["sc"] * num)
        elif kind == "deadline":
            contract["deadline"] = contract["deadline"] * num
        elif kind == "expiry":
            contract["expiry"] = contract["expiry"] * num


def can_process(name, keywords, required):
    d = MENT[name]
    c = d.get("can")
    if c == "never":
        return False
    if c == "record":
        return "Record" in required
    if c == "noRecord":
        return "Record" not in keywords
    return True


def score_kw(name, kw):
    d = MENT[name]
    s = d["score"]
    if s == "neg":
        return -2
    if s == "record":
        return 1 if kw == "Record" else -2
    return s.get(kw, 0)


# ---------------------------------------------------------------------------------------------------------------- Agents.cfg
def load_agents():
    root = parse_cfg(os.path.join(SQUAD, "Agencies", "Agents.cfg"))
    agents = []
    for n in root["nodes"]:
        if n["name"] != "AGENT":
            continue
        a = {"name": val(n, "name"), "title": val(n, "title"), "mentality": []}
        for nm, f in (("Cooperative", 0.5), ("Competitive", 0.5)):
            a["mentality"].append((nm, f))
        for k, v in n["values"]:
            if k != "mentality":
                continue
            parts = [x for x in re.split(r"[, ]", v) if x]
            nm = parts[0].strip()
            f = 0.5
            if len(parts) == 2:
                f = F(float(parts[1]))
            if any(m[0] == nm for m in a["mentality"]):
                continue
            a["mentality"].append((nm, min(1.0, max(0.0, f))))
            if nm == "NoRandomContractMentality":
                a["mentality"] = [m for m in a["mentality"] if m[0] == nm]
        agents.append(a)
    return agents


AGENTS = load_agents()


def suitable_agent(rng, keywords, required):
    pool = []
    for a in AGENTS:
        ok = True
        if required:
            ok = any(score_kw(m, k) > 0 for m, _ in a["mentality"] for k in required)
        if ok:
            ok = any(can_process(m, keywords, required) for m, _ in a["mentality"])
        if not ok:
            continue
        num = 1
        for m, _ in a["mentality"]:
            if can_process(m, keywords, required):
                for k in keywords:
                    num += score_kw(m, k)
        num *= 2
        pool.extend([a] * max(num, 0))
    if not pool:
        return None
    return pool[rng.range_i(0, len(pool))]


# ---------------------------------------------------------------------------------------------------------------- hashes
def hash35(s):
    n = 0
    i = 0
    L = len(s)

    def i32(x):
        x &= M32
        return x - (1 << 32) if x >= 1 << 31 else x

    while i < L - 1:
        n2 = i32((n << 5) - n + ord(s[i]))
        n = i32((n2 << 5) - n2 + ord(s[i + 1]))
        i += 2
    if i < L:
        n = i32((n << 5) - n + ord(s[i]))
    return n


def super_seed(guid_bytes, seed):
    a = list(guid_bytes)
    for i in range(3):
        a[i] ^= a[i + 4]
        a[i] ^= a[i + 8]
        a[i] ^= a[i + 12]
    num = int.from_bytes(bytes(a[0:4]), "little", signed=True)

    def i32(x):
        x &= M32
        return x - (1 << 32) if x >= 1 << 31 else x

    prod = i32(i32(num + seed) * i32(num + seed + 1))
    return i32(_csdiv(prod, 2) + seed)


def _csdiv(a, b):
    q = abs(a) // abs(b)
    return -q if (a < 0) != (b < 0) else q


def wrap64(x):
    x &= (1 << 64) - 1
    return x - (1 << 64) if x >= 1 << 63 else x


# ---------------------------------------------------------------------------------------------------------------- contract counts
def contract_counts(rep, avg):
    num = F(float(avg) * m_lerp(0.5, 1.5, m_inverse_lerp(-1.0, 1.0, rep)))
    num2 = m_inverse_lerp(-1.0, 1.0, rep)
    num3 = m_inverse_lerp(F(-0.33), F(0.6), rep)
    num4 = m_inverse_lerp(F(0.1), 1.0, rep)
    num5 = m_floor(m_lerp(num, 2.0, F(num2 - num3 * 0.5)))
    num6 = m_floor(m_lerp(0.0, num, F(num3 - num4 * 0.5)))
    num7 = m_floor(m_lerp(0.0, num, num4))
    num8 = F(1.0 / ((num5 + num6 + num7) / num))
    return [round_to_int(F(num5 * num8)), round_to_int(F(num6 * num8)), round_to_int(F(num7 * num8))]


# ---------------------------------------------------------------------------------------------------------------- weights (ContractSystem.cs)
def weights_cfg():
    return {k: int(val(CONTRACTS, k)) for k in ("WeightDefault", "WeightMinimum", "WeightMaximum", "WeightAcceptDelta", "WeightDeclineDelta",
                                                "WeightWithdrawReadDelta", "WeightWithdrawSeenDelta")}


# ---------------------------------------------------------------------------------------------------------------- TextGen
def story_tables():
    root = parse_cfg(os.path.join(SQUAD, "Resources", "StoryDefs.cfg"))
    sd = child(root, "STORY_DEF")
    t = {}
    for sec in sd["nodes"]:
        d = {}
        for k, v in reversed(sec["values"]):
            lst = d.setdefault(k, [])
            tr = LOC.single(v)  # GameDatabase loading translates the config values (Localizer.TranslateBranch)
            if tr not in lst:
                lst.append(tr)
        t[sec["name"]] = d
    return t


STORY = story_tables()


def process_replacements(src):
    if "[" in src:
        num = src.rfind("[")
        while num != -1:
            num2 = src.find("]", num)
            if num2 != -1:
                text = src[num + 1:num2]
                if "[" not in text and "]" not in text:
                    flag = False
                    if num >= 4:
                        t2 = src[num - 4:num]
                        if t2[0] == "<" and t2[1] == "<" and t2[3] == ":":
                            flag = True
                    src = src[:num] + src[num2 + 1:]
                    value = ""
                    if text != "Agency":
                        if text == "Topic":
                            value = "2" if flag else "<<2>>"
                    else:
                        value = "1" if flag else "<<1>>"
                    src = src[:num] + value + src[num:]
                    num = src.rfind("[")
                    continue
                return src
            return src
    return src


def get_sentence(rng, kind, ctype, subject, allow_generic):
    d = STORY[kind]
    num = 0
    v1 = d.get(subject) if subject else None
    if v1 is not None:
        num += len(v1)
        if not v1:
            v1 = None
    v2 = None
    if num == 0 or allow_generic:
        v2 = d.get(ctype)
        if v2 is not None:
            num += len(v2)
            if not v2:
                v2 = None
    v3 = None
    if num == 0 or allow_generic:
        v3 = d.get("Generic")
        if v3 is not None:
            num += len(v3)
            if not v3:
                v3 = None
    n2 = rng.range_i(0, num)
    if v2 is not None:
        if n2 < len(v2):
            return v2[n2]
        n2 -= len(v2)
    if v1 is not None:
        if n2 < len(v1):
            return v1[n2]
        n2 -= len(v1)
    if v3 is not None and n2 < len(v3):
        return v3[n2]
    return ""


def back_story(contract_type, agency, topic, subject, seed, gi, gp, gc):
    ctype = contract_type[:contract_type.rfind("Contract")] if contract_type.endswith("Contract") else contract_type
    rng = Unity(seed)
    s1 = get_sentence(rng, "Introduction", ctype, subject, gi)
    s2 = get_sentence(rng, "Problem", ctype, subject, gp)
    s3 = get_sentence(rng, "Conclusion", ctype, subject, gc)
    text = "%s %s %s" % (s1, s2, s3)
    n = 0
    while "[" in text and n < 8:
        n += 1
        text = process_replacements(text)
    return LOC.fmt(text, agency, topic)


# ---------------------------------------------------------------------------------------------------------------- progress (ProgressMilestone)
NODE_ID = {"BASECONSTRUCTION": "BaseConstruction", "CREWTRANSFER": "CrewTransfer", "DOCKING": "Docking", "ESCAPE": "Escape", "FLAGPLANT": "FlagPlant",
           "FLIGHT": "Flight", "FLYBY": "Flyby", "FLYBYRETURN": "ReturnFromFlyBy", "LANDING": "Landing", "LANDINGRETURN": "ReturnFromSurface",
           "ORBIT": "Orbit", "ORBITRETURN": "ReturnFromOrbit", "RENDEZVOUS": "Rendezvous", "SCIENCE": "Science", "SPACEWALK": "Spacewalk",
           "SPLASHDOWN": "Splashdown", "STATIONCONSTRUCTION": "StationConstruction", "SUBORBIT": "Suborbit", "SURFACEEVA": "SurfaceEVA"}
BODY_SENS = set(NODE_ID)
CREW_SENS = {"BASECONSTRUCTION", "DOCKING", "ESCAPE", "FLIGHT", "FLYBY", "FLYBYRETURN", "LANDING", "LANDINGRETURN", "ORBIT", "ORBITRETURN", "RENDEZVOUS",
             "SCIENCE", "SPLASHDOWN", "STATIONCONSTRUCTION", "SUBORBIT"}
IMPLIES_MANNED = {"CREWTRANSFER", "FLAGPLANT", "SPACEWALK", "SURFACEEVA"}
TYPES_DESC = sorted(NODE_ID)  # order is irrelevant for the boolean any()


class Scenario:
    """body -> {node id: state}, state in reached / completed / completedManned / completedUnmanned"""

    def __init__(self, d):
        self.d = d

    def node(self, body, nid):
        return self.d.get(body, {}).get(nid)

    def body_reached(self, body):
        return bool(self.d.get(body))

    def complete(self, body, ptype, manned):
        b = BODY[body]
        if manned == "UNMANNED" and ptype in IMPLIES_MANNED:
            return False
        if ptype == "SPLASHDOWN" and not b["ocean"]:
            return False
        if ptype == "FLIGHT" and not b["atmosphere"]:
            return False
        if not b["solid"] and ptype in ("BASECONSTRUCTION", "FLAGPLANT", "LANDING", "LANDINGRETURN", "SURFACEEVA"):
            return False
        if not self.body_reached(body):
            return False
        st = self.node(body, NODE_ID[ptype])
        if st is None or st == "reached":
            return False
        if manned == "MANNED":
            return st in ("completed", "completedManned")
        if manned == "UNMANNED":
            return st in ("completed", "completedUnmanned")
        return True

    def any_progress(self, body, manned):
        if BODY[body]["home"]:
            return True
        for t in TYPES_DESC:
            if t not in BODY_SENS:
                continue
            if manned != "ANY" and t not in CREW_SENS and t not in IMPLIES_MANNED:
                continue
            if self.complete(body, t, manned):
                return True
        return False


def bodies_progress(sc, ptype, body_reached, progress_complete, manned, where):
    """ProgressUtilities.GetBodiesProgress: FlightGlobals.Bodies walked from the end"""
    out = []
    for b in reversed(BODY_LIST):
        if not where(b):
            continue
        if body_reached is not None and sc.any_progress(b["name"], manned) != body_reached:
            continue
        if sc.complete(b["name"], ptype, manned) == progress_complete:
            out.append(b)
    return out


def weighted_body(rng, bodies, weights):
    num = 0
    for b in reversed(bodies):
        num += weights.get(b["name"], 0)
    n2 = rng.range_i(0, num)
    for i in range(len(bodies) - 1, -1, -1):
        w = weights.get(bodies[i]["name"], 0)
        if n2 >= w:
            n2 -= w
            continue
        n2 = i
        break
    return bodies[n2]


# ---------------------------------------------------------------------------------------------------------------- template generation
def add_keywords(kws, *new):
    """Contract.AddKeywords(params string[]): adds ONLY the first keyword that is not in the list yet (sic), then returns"""
    for k in new:
        if k not in kws:
            kws.append(k)
            return True
    return False


def gen_template(ctype, prestige, seed, sc, weights, mult, guid):
    p = PRESTIGE.index(prestige)
    rng = Unity(seed)
    c = C(p, mult)
    kws = []
    params = []
    home = next(b for b in BODY_LIST if b["home"])
    sun = BODY_LIST[0]
    manned = "MANNED"
    if ctype == "PlantFlag":
        if prestige == "Trivial":
            lst = bodies_progress(sc, "LANDING", None, True, manned, lambda b: b["name"] != home["name"] and b["name"] != sun["name"] and b["solid"])
            # CelestialCrewCounts is empty in the scenario (no vessels)
            if not lst:
                return None
            tb = weighted_body(rng, lst, weights)
            add_keywords(kws, "Commercial")
        elif prestige == "Significant":
            lst = bodies_progress(sc, "LANDING", True, False, manned, lambda b: b["name"] != home["name"] and b["name"] != sun["name"] and b["solid"])
            if not lst:
                return None
            tb = weighted_body(rng, lst, weights)
            add_keywords(kws, "Commercial")
            add_keywords(kws, "Pioneer")
        else:
            return None
        params.append({"type": "PlantFlag", "body": tb["name"], "hash": tb["name"], "title": LOC.fmt("#autoLOC_270135", tb["display"])})
        c.set_expiry_draw(rng.range_i(cfg_i("Flag", "Expiration", "MinimumExpireDays"), cfg_i("Flag", "Expiration", "MaximumExpireDays")))
        c.set_deadline_days(float(cfg_i("Flag", "Expiration", "DeadlineDays")), tb["name"])
        c.set_funds(cfg_f("Flag", "Funds", "BaseAdvance"), cfg_f("Flag", "Funds", "BaseReward"), cfg_f("Flag", "Funds", "BaseFailure"), tb["name"])
        c.set_rep(cfg_f("Flag", "Reputation", "BaseReward"), cfg_f("Flag", "Reputation", "BaseFailure"))
        c.set_science(cfg_f("Flag", "Science", "BaseReward"))
        title = LOC.fmt("#autoLOC_274001", tb["display"])
        synopsis = LOC.fmt("#autoLOC_273996", tb["display"])
        topic, subject = tb["display"], tb["display"] + "Srf"
        hash_string = str(super_seed(guid, seed))
        extra = {}
    else:  # CollectScience
        loc_draw = rng.range_i(0, 2)
        space = loc_draw == 0
        factor = F(0.85) if space else F(1.15)
        if space:
            if prestige == "Trivial":
                lst = bodies_progress(sc, "ORBIT", None, True, "ANY", lambda b: b["name"] != sun["name"])
                add_keywords(kws, "Scientific", "Commercial")
            elif prestige == "Significant":
                lst = bodies_progress(sc, "ORBIT", True, False, "ANY", lambda b: b["name"] != sun["name"])
                add_keywords(kws, "Pioneer", "Scientific")
            else:
                return None
        else:
            lst = None
            if prestige == "Trivial":
                lst = bodies_progress(sc, "LANDING", None, True, "ANY", lambda b: b["name"] != home["name"] and b["name"] != sun["name"] and b["solid"])
            elif prestige == "Significant":
                lst = bodies_progress(sc, "LANDING", True, False, "ANY", lambda b: b["name"] != home["name"] and b["name"] != sun["name"] and b["solid"])
            else:
                return None
            add_keywords(kws, "Scientific", "Commercial")
        if not lst:
            return None
        tb = weighted_body(rng, lst, weights)
        params.append({"type": "CollectScience", "body": tb["name"], "hash": ("Space " if space else "Surface ") + tb["name"],
                       "title": LOC.fmt("#autoLOC_269461" if space else "#autoLOC_269463", tb["display"])})
        r = lambda sub, key: cfg_f("Science", sub, key)
        mm = lambda v: F(v * factor)
        if space:
            c.set_expiry_draw(rng.range_i(1, 7))
            c.set_deadline_years(5.0, tb["name"])
        else:
            c.set_expiry_draw(rng.range_i(cfg_i("Science", "Expiration", "MinimumExpireDays"), cfg_i("Science", "Expiration", "MaximumExpireDays")))
            c.set_deadline_days(float(cfg_i("Science", "Expiration", "DeadlineDays")), tb["name"])
        c.set_funds(mm(r("Funds", "BaseAdvance")), mm(r("Funds", "BaseReward")), mm(r("Funds", "BaseFailure")), tb["name"])
        c.set_rep(mm(r("Reputation", "BaseReward")), mm(r("Reputation", "BaseFailure")))
        c.set_science(mm(r("Science", "BaseReward")))
        title = LOC.fmt("#autoLOC_272443" if space else "#autoLOC_272445", tb["display"])
        synopsis = LOC.fmt("#autoLOC_272432" if space else "#autoLOC_272436", tb["display"])
        topic = tb["display"] if space else LOC.fmt("#autoLOC_272424")
        subject = tb["display"] if space else tb["display"] + "Srf"
        hash_string = ""
        extra = {"location": "Space" if space else "Surface"}
    agent = suitable_agent(rng, kws, [])
    if agent is None:
        return None
    contract = {"fa": c.fa, "fc": c.fc, "ff": c.ff, "rc": c.rc, "rf": c.rf, "sc": c.sc, "expiry": c.expiry, "deadline": c.deadline}
    for nm, f in agent["mentality"]:
        if can_process(nm, kws, []):
            apply_mentality(contract, nm, f, p)
    contract["ff"] = max(contract["ff"], contract["fa"])
    num = wrap64(391 + hash35(ctype))
    if hash_string:
        num = wrap64(num * 23 + hash35(hash_string))
    for pr in params:
        num = wrap64(num * 23 + hash35(pr["type"]))
        if pr["hash"]:
            num = wrap64(num * 23 + hash35(pr["hash"]))
    desc = back_story(ctype, agent["title"], topic, subject, seed, True, False, True)
    res = {"type": ctype, "prestige": prestige, "seed": seed, "mult": mult, "agent": agent["name"], "agentTitle": agent["title"], "keywords": kws,
           "title": title, "synopsis": synopsis, "description": desc, "contractID": num, "params": params,
           "fa": contract["fa"], "fc": contract["fc"], "ff": contract["ff"], "rc": contract["rc"], "rf": contract["rf"], "sc": contract["sc"],
           "expiry": contract["expiry"], "deadline": contract["deadline"]}
    res.update(extra)
    return res



# ---------------------------------------------------------------------------------------------------------------- PartTestConstraint (PartTestConstraint.cs Load)
CTYPES = ["SPEED", "SPEEDENV", "ALTITUDE", "ALTITUDEENV", "DENSITY", "DYNAMICPRESSURE", "OXYGEN", "ATMOSPHERE", "SITUATION", "REPEATABILITY"]
CTESTS = ["GT", "LT", "EQ", "NEQ"]
CREPEAT = ["ALWAYS", "BODYANDSITUATION", "ONCEPERPART"]


def parse_constraint(node):
    c = {"valid": True, "type": None, "test": "EQ", "body": "_Any", "prestige": "_Any", "value": "", "valueD": None, "valueB": False, "valueI": 0,
         "valueR": "ALWAYS", "situationMask": 4294967295, "prestigeLevel": "Trivial"}
    tv = val(node, "type")
    if tv is not None:
        if tv in CTYPES:
            c["type"] = tv
        else:
            c["valid"] = False
            c["type"] = "SPEED"
    else:
        c["valid"] = False
        c["type"] = "SPEED"
    te = val(node, "test")
    if te is not None:
        if te in CTESTS:
            c["test"] = te
        else:
            c["valid"] = False
    v = val(node, "value")
    if v is not None:
        c["value"] = v
        vd = None
        try:
            if re.fullmatch(r"\s*[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?\s*", v):
                vd = float(v)
        except ValueError:
            vd = None
        if vd is not None:
            c["valueD"] = vd
        flag = v.strip().lower() in ("true", "false")
        c["valueB"] = v.strip().lower() == "true"
        if re.fullmatch(r"\s*\+?\d+\s*", v) and int(v.strip().lstrip("+")) <= 4294967295:
            c["valueI"] = int(v.strip().lstrip("+"))
        ty = c["type"]
        if ty not in ("ALTITUDE", "DENSITY", "DYNAMICPRESSURE", "SPEED"):
            if ty not in ("ATMOSPHERE", "OXYGEN"):
                if ty == "REPEATABILITY":
                    if v in CREPEAT:
                        c["valueR"] = v
                    else:
                        c["valid"] = False
            elif not flag:
                c["valid"] = False
        elif c["valueD"] is None:
            c["valid"] = False
    b = val(node, "body")
    if b is not None:
        c["body"] = b
    sm = val(node, "situationMask")
    if sm is not None and re.fullmatch(r"\s*\+?\d+\s*", sm) and int(sm.strip().lstrip("+")) <= 4294967295:
        c["situationMask"] = int(sm.strip().lstrip("+"))
    pr = val(node, "prestige")
    if pr is not None:
        c["prestige"] = pr
        if pr != "_Any":
            if pr in PRESTIGE:
                c["prestigeLevel"] = pr
            else:
                c["valid"] = False
    return c


def part_constraints():
    out = []
    base = os.path.join(CORE, "testdata", "GameData-mirror", "GameData")
    for dp, _dn, fn in os.walk(base):
        for f in sorted(fn):
            if not f.endswith(".cfg"):
                continue
            path = os.path.join(dp, f)
            if "ModuleTestSubject" not in open(path, encoding="utf-8-sig", errors="replace").read():
                continue
            root = parse_cfg(path)
            for part in [n for n in root["nodes"] if n["name"] == "PART"]:
                mods = [m for m in part["nodes"] if m["name"] == "MODULE" and val(m, "name") == "ModuleTestSubject"]
                for m in mods:
                    cons = [parse_constraint(n) for n in m["nodes"] if n["name"] == "CONSTRAINT"]
                    out.append({"part": val(part, "name"), "file": os.path.relpath(path, base).replace("\\", "/"), "total": len(cons),
                                "valid": [c for c in cons if c["valid"]],
                                "situationMask": val(m, "situationMask"), "useStaging": val(m, "useStaging"), "useEvent": val(m, "useEvent")})
    out.sort(key=lambda x: (x["file"], x["part"]))
    return out


# ---------------------------------------------------------------------------------------------------------------- the reference
def main():
    ref = {"provenance": "tools/wp37/contract_reference.py (independent of kspcore); Contracts.cfg / Agents.cfg / StoryDefs.cfg / dictionary.cfg of "
                         "core/testdata/GameData-mirror, bodies of harness/golden/serialized/celestial_bodies.serialized.json"}
    ref["bodies"] = [{"name": b["name"], "display": b["display"], "recovery": b["recovery"]} for b in BODY_LIST]
    ref["mult_sets"] = MULT_SETS
    ref["weights_cfg"] = weights_cfg()

    # --- UnityEngine.Random cross-check
    rr = []
    for seed in (0, 1, 7, 1234, -5, 2147483647, -2147483648):
        u = Unity(seed)
        ints = [u.range_i(0, 1000) for _ in range(6)] + [u.range_i(1, 7) for _ in range(3)]
        u2 = Unity(seed)
        fl = [u2.range_f(0.0, 2.0) for _ in range(4)]
        rr.append({"seed": seed, "ints": ints, "floats": fl})
    ref["rng"] = rr

    # --- Set* reward maths (Contract.cs SetFunds / SetReputation / SetScience / SetDeadline*)
    rewards = []
    bases = [(23000.0, 58000.0, 0.0, 14.0, 7.0, 0.0), (46000.0, 120000.0, 30000.0, 3.5, 1.5, 12.0), (1234.5, 987.25, 55.5, 0.0, 9.75, 3.3)]
    for mi, mult in enumerate(MULT_SETS):
        for p in range(3):
            for body in ("Kerbin", "Mun", "Minmus", "Duna", "Eeloo", "Jool", None):
                for bi, (adv, comp, fail, rrw, rf, sci) in enumerate(bases):
                    c = C(p, mult)
                    c.set_funds(F(adv), F(comp), F(fail), body)
                    c.set_rep(F(rrw), F(rf), body)
                    c.set_science(F(sci), body)
                    c.set_deadline_days(F(852.0), body)
                    d1 = c.deadline
                    c.set_deadline_years(F(2.5), body)
                    d2 = c.deadline
                    c3 = C(p, mult)
                    c3.set_funds2(F(adv), F(comp), body)  # the (advance, completion) overload: failure = advance
                    rewards.append({"mult": mi, "prestige": p, "body": body, "base": bi, "in": [F(adv), F(comp), F(fail), F(rrw), F(rf), F(sci)],
                                    "fa": c.fa, "fc": c.fc, "ff": c.ff, "rc": c.rc, "rf": c.rf, "sc": c.sc, "daysDeadline": d1, "yearsDeadline": d2,
                                    "fa2": c3.fa, "fc2": c3.fc, "ff2": c3.ff})
    ref["rewards"] = rewards

    # --- expiry draws
    exp = []
    for seed in (3, 99, 12345, -77):
        u = Unity(seed)
        d = u.range_i(1, 7)
        c = C(0, MULT_SETS[0])
        c.set_expiry_draw(d)
        u2 = Unity(seed)
        df = u2.range_f(1.0, 7.0)
        e2 = m_round(F(df * float(DAY) / 60.0)) * 60.0
        exp.append({"seed": seed, "draw": d, "expiry": c.expiry, "drawF": df, "expiryF": e2})
    ref["expiry"] = exp

    # --- mentality processing (all 19 mentalities x factors x prestige)
    ment = []
    for name in MENT:
        for p in range(3):
            for factor in (0.0, 0.25, 0.5, 1.0):
                contract = {"fa": 10000.0, "fc": 25000.0, "ff": 12000.0, "rc": F(20.0), "rf": F(10.0), "sc": F(8.0), "deadline": 1e6, "expiry": 50000.0}
                if can_process(name, ["Commercial"], []):
                    apply_mentality(contract, name, factor, p)
                ment.append({"name": name, "prestige": p, "factor": factor, "out": contract,
                             "can": can_process(name, ["Commercial", "Record"], []), "canRecord": can_process(name, ["Record"], ["Record"]),
                             "scoreCommercial": score_kw(name, "Commercial"), "scoreRecord": score_kw(name, "Record"),
                             "scoreScientific": score_kw(name, "Scientific")})
    ref["mentality"] = ment

    # --- GetContractCounts
    counts = []
    for avg in (10, 6, 18):
        for i in range(-20, 21):
            rep = F(i / 20.0)
            counts.append({"rep": rep, "avg": avg, "tiers": contract_counts(rep, avg)})
        for rep in (F(0.123), F(-0.77), F(0.333), F(0.9999)):
            counts.append({"rep": rep, "avg": avg, "tiers": contract_counts(rep, avg)})
    ref["counts"] = counts

    # --- weights after accept / decline / withdraw / complete
    w = weights_cfg()
    lo, hi, dflt = w["WeightMinimum"], w["WeightMaximum"], w["WeightDefault"]
    wd = lambda x: min(max(x, lo), hi)
    seq = [("PlantFlag", "accept"), ("PlantFlag", "accept"), ("PlantFlag", "complete"), ("CollectScience", "decline"), ("CollectScience", "decline"),
           ("CollectScience", "decline"), ("CollectScience", "decline"), ("PlantFlag", "withdrawRead"), ("PlantFlag", "withdrawSeen"),
           ("PlantFlag", "accept"), ("PlantFlag", "accept"), ("PlantFlag", "accept"), ("PlantFlag", "accept"), ("PlantFlag", "accept"),
           ("PlantFlag", "accept"), ("PlantFlag", "accept")]
    cur = {"PlantFlag": dflt, "CollectScience": dflt, "Mun": dflt}
    steps = []
    for name, op in seq:
        delta = {"accept": w["WeightAcceptDelta"], "decline": w["WeightDeclineDelta"], "withdrawRead": w["WeightWithdrawReadDelta"],
                 "withdrawSeen": w["WeightWithdrawSeenDelta"], "complete": 0}[op]
        cur[name] = wd(cur[name] + delta)
        cur["Mun"] = wd(cur["Mun"] + delta)  # the contract also adjusts the weight of its WeightBodies
        steps.append({"type": name, "op": op, "PlantFlag": cur["PlantFlag"], "CollectScience": cur["CollectScience"], "Mun": cur["Mun"]})
    ref["weights"] = {"default": dflt, "min": lo, "max": hi, "steps": steps}

    # --- TextGen
    texts = []
    topics = [("PlantFlag", "Mun", "MunSrf"), ("PlantFlag", "Duna", "DunaSrf"), ("CollectScience", "Minmus", "Minmus"), ("CollectScience", "Mun", "MunSrf"),
              ("PartTest", "Mk1 Command Pod", "PartsMk1 Command Pod"), ("RecoverAsset", "Jebediah Kerman", "RecoverKerbal"),
              ("GrandTour", "Kerbin", "Kerbin"), ("Exploration", "Eve", "Eve"), ("NoSuchContract", "Mun", "Mun")]
    for cname, topic, subject in topics:
        for seed in (0, 1, 2, 17, 4242, -9, 1000003):
            for gi, gp, gc in ((True, False, True), (False, False, False), (True, True, True)):
                agent = AGENTS[seed % len(AGENTS)]
                texts.append({"type": cname, "topic": topic, "subject": subject, "seed": seed, "agentTitle": agent["title"], "allow": [gi, gp, gc],
                              "text": back_story(cname, agent["title"], topic, subject, seed, gi, gp, gc)})
    ref["text"] = texts

    # --- generated trees
    scen = {
        "Kerbin": {"Orbit": "completed", "Landing": "completed"},
        "Mun": {"Orbit": "completed", "Landing": "completedManned", "FlagPlant": "reached"},
        "Minmus": {"Orbit": "completedUnmanned", "Landing": "completed"},
        "Duna": {"Orbit": "completedManned", "Landing": "reached"},
        "Eve": {"Orbit": "completed"},
        "Jool": {"Orbit": "completedManned"},
        "Ike": {"Orbit": "reached"},
    }
    sc = Scenario(scen)
    guid = bytes([0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF])
    weights = {b["name"]: weights_cfg()["WeightDefault"] for b in BODY_LIST}
    weights["Mun"] = 55
    weights["Minmus"] = 12
    trees = []
    for ctype in ("PlantFlag", "CollectScience"):
        for prestige in ("Trivial", "Significant", "Exceptional"):
            for seed in range(1, 41):
                for mi, mult in enumerate(MULT_SETS):
                    g = bytes(((b + seed) & 0xFF) for b in guid)
                    r = gen_template(ctype, prestige, seed * 7919 + 13, sc, weights, mult, g)
                    entry = {"type": ctype, "prestige": prestige, "seed": seed * 7919 + 13, "mult": mi, "guid": list(g), "result": r}
                    trees.append(entry)
    ref["scenario"] = {"progress": scen, "weights": weights, "agents": [a["name"] for a in AGENTS]}
    ref["trees"] = trees
    ref["part_constraints"] = part_constraints()
    ref["agents"] = [{"name": a["name"], "title": a["title"], "mentality": [[m, f] for m, f in a["mentality"]]} for a in AGENTS]
    json.dump(ref, open(OUT, "w", encoding="utf-8"), indent=1, sort_keys=False)
    print("wrote", OUT, "trees:", len(trees), "generated:", sum(1 for t in trees if t["result"]), "text:", len(texts), "rewards:", len(rewards))


if __name__ == "__main__":
    main()
