#!/usr/bin/env python3
"""WP-38 slice 2 independent reference: the FinePrint contracts ARMContract, BaseContract, StationContract, ISRUContract, SatelliteContract
(generation with no vessels in the universe), the texts / node values of the FinePrint parameters, StringUtilities, SystemUtilities.ProcessSideRequests.

No kspcore code. Inputs are the game's own data (GameData-mirror Contracts.cfg / Agents.cfg / StoryDefs.cfg / dictionary.cfg / Resources, the golden body
table, Physics.cfg) and the decompiled C# (FinePrint/Contracts/*.cs, FinePrint/Utilities/*.cs); the algorithms are re-implemented here. It re-uses the
independent WP-37 / WP-38 reference modules (cfg parser, Localizer, UnityEngine.Random, KSPRandom, Contract.Set*, mentalities, agents, TextGen, orbit
draw model, random positions, progress scenario model).

The Mono float rule of core/README.md applies: a C# float expression is evaluated in double and rounded to float32 only where the C# stores, passes,
returns or casts it (F()).

Fixtures that are NOT KSP data (STATUS.md "needs data"): everything listed in tools/wp37 and tools/wp38 plus
  * R&D: no part database is loaded, so ResearchedValidContractObjectives answers from the FIXTURES below and HaveModuleTech from FIX_MODULES; HavePartTech is
    always false (no PartLoader);
  * the crew roster is empty (CrewTraitMissionAvailability = 0, every CREW_REQUEST is skipped), no vessel exists (contextual / network branches are off);
  * UnityEngine.Random-free parts use the contract seed; WaypointManager.ChooseRandomPosition(generator = null) draws from `new KSPRandom(4242)`
    (ContractsEnvironment.newRandomSeed seam);
  * the site generator text asset (StringUtilities.GenerateSiteName) is not in the repo: its titles are not compared.

Usage: python -I core/tools/wp38b/fineprint2_reference.py   -> core/tests/data/wp38b_reference.json
"""
import json
import math
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(CORE, "tools", "wp37"))
sys.path.insert(0, os.path.join(CORE, "tools", "wp38"))
import contract_reference as w37  # noqa: E402
import fineprint_reference as w38  # noqa: E402

OUT = os.path.join(CORE, "tests", "data", "wp38b_reference.json")
F = w37.F


class Loc2(w37.Loc):
    r"""Localizer.Format with the managed Lingoona fallback of Grammar.useGrammar (Assembly-CSharp-firstpass/Lingoona/Grammar.cs:53):
    regex <<(?:[^>]+:)?([a-z_]+|\d+)>> restarted from the beginning after every substitution"""

    RX = re.compile(r"<<(?:[^>]+:)?([a-z_]+|\d+)>>")

    def fmt(self, template, *params):
        if not template:
            return ""
        if params:
            t = self.single(template)
            ps = [self.single(p) for p in params]
            while True:
                m = self.RX.search(t)
                if not m:
                    break
                g = m.group(1)
                if g[0].isdigit():
                    n = int(g)
                    t = t[:m.start()] + t[m.end():]
                    if 0 < n <= len(ps):
                        x = ps[n - 1]
                        k = x.find("^")
                        if k != -1:
                            x = x[:k]
                        t = t[:m.start()] + x + t[m.start():]
                else:
                    ln = m.end() - m.start()
                    t = t[:m.start()] + t[m.start() + 2:]
                    t = t[:m.start() + ln - 4] + t[m.start() + ln - 2:]
            r = t
        else:
            r = self.single(template)
        return r.replace("\\n", "\n").replace('\\"', '"').replace("\\t", "\t")


LOC = Loc2()
BD = w38.BD
BODY_LIST = w37.BODY_LIST
HOME = w38.HOME
KSPRandom = w38.KSPRandom
PRESTIGE_NAMES = ["Trivial", "Significant", "Exceptional"]
CONTRACTS = w37.CONTRACTS
RANDOM_SEED_FIXTURE = 4242


# ---------------------------------------------------------------------------------------------------------------- cfg access (ContractDefs.*)
def cfg_node(section, sub=None):
    n = w37.child(CONTRACTS, section)
    if sub:
        n = w37.child(n, sub)
    return n


def cf(section, sub, key):
    """a C# float member"""
    return F(float(w37.val(cfg_node(section, sub or None), key)))


def cd(section, sub, key):
    """a C# double member"""
    return float(w37.val(cfg_node(section, sub or None), key))


def ci(section, sub, key):
    return int(w37.val(cfg_node(section, sub or None), key))


def cb(section, sub, key):
    return w37.val(cfg_node(section, sub or None), key).strip().lower() == "true"


def coin(g):
    return g.next_max(2) == 1


def super_seed(guid, seed):
    return w37.super_seed(guid, seed)


# ---------------------------------------------------------------------------------------------------------------- the part database of the mirror
def mirror_parts():
    out = set()
    root = os.path.join(w37.SQUAD, "Parts")
    for dp, _dn, fn in os.walk(root):
        for f in fn:
            if f.endswith(".cfg"):
                try:
                    n = w37.parse_cfg(os.path.join(dp, f))
                except Exception:
                    continue
                for pn in n["nodes"]:
                    if pn["name"] == "PART":
                        nm = w37.val(pn, "name")
                        if nm:
                            out.add(nm.replace("_", "."))
    return out


MIRROR_PARTS = mirror_parts()

# ---------------------------------------------------------------------------------------------------------------- fixtures
class Fix:
    """R&D / unlock fixture: objectives researched, module techs researched, probe cores unlocked"""

    def __init__(self, name, objectives, modules, probe=True):
        self.name = name
        self.objectives = set(objectives)
        self.modules = set(modules)
        self.probe = probe

    def researched(self, *types):
        return all(t in self.objectives for t in types)

    def have_any_tech(self, parts, modules):
        # HavePartTech: the part exists in the loaded part database (PartTechAvailable is true without R&D); HaveModuleTech: the fixture
        return any(pt.replace("_", ".") in MIRROR_PARTS for pt in parts) or any(m in self.modules for m in modules)


FIXTURES = [
    Fix("all", ["Antenna", "Dock", "Generator", "Grapple", "Wheel"], ["ModuleScienceLab", "ModuleResourceHarvester", "ModuleCommand"]),
    Fix("some", ["Antenna", "Dock", "Generator"], ["ModuleResourceHarvester"]),
    Fix("none", [], []),
]


# ---------------------------------------------------------------------------------------------------------------- progress helpers (ProgressUtilities)
def bp(s, ptype, progress_complete, manned, where, reached=None):
    """GetBodiesProgress: FlightGlobals.Bodies walked from the end"""
    out = []
    for b in reversed(BODY_LIST):
        n = b["name"]
        if not where(n):
            continue
        if reached is not None and s.sc.any_progress(n, manned) != reached:
            continue
        if s.sc.complete(n, ptype, manned) == progress_complete:
            out.append(n)
    return out


def bodies_reached(s, reached, manned, where):
    return [b["name"] for b in reversed(BODY_LIST) if where(b["name"]) and s.sc.any_progress(b["name"], manned) == reached]


def next_unreached_m(s, count, manned, where):
    lst = [BD["Sun"]] + [BD[n] for n in BD["Sun"]["children"]]
    rec = lambda a, b: (a["recovery"] > b["recovery"]) - (a["recovery"] < b["recovery"])
    w38.mono_sort(lst, rec)
    stack = list(reversed(lst))
    out = []
    while stack and len(out) < count:
        b = stack.pop()
        if not s.sc.any_progress(b["name"], manned) and (where is None or where(b["name"])):
            out.append(b["name"])
        if b["name"] != "Sun":
            kids = [BD[n] for n in b["children"]]
            w38.mono_sort(kids, rec)
            for k in reversed(kids):
                stack.append(k)
    return out


def wb(g, names, W):
    """ContractSystem.WeightedBodyChoice with a KSPRandom (an empty list: null, no draw)"""
    if not names:
        return None
    num = 0
    for n in names:
        num += W[n]
    n2 = g.next_range(0, num)
    for i in range(len(names) - 1, -1, -1):
        w = W[names[i]]
        if n2 >= w:
            n2 -= w
            continue
        n2 = i
        break
    return names[n2]


def neighbors(name, where):
    out = []
    p = BD[name]["parent"]
    if p is not None and where(p):
        out.append(p)
    for c in reversed(BD[name]["children"]):
        if where(c):
            out.append(c)
    return out


def shuffle(lst, g):
    """SystemUtilities.ShuffleList"""
    for num in range(len(lst) - 1, 0, -1):
        j = g.next_range(0, num + 1)
        lst[num], lst[j] = lst[j], lst[num]


def mround(x):
    return w37.m_round(F(x))


def thousands(x):
    """Convert.ToDecimal(Math.Round(x)).ToString("#,###") ("" for zero)"""
    r = int(np.rint(np.float64(x)))
    if r == 0:
        return ""
    return "{:,}".format(r)


# ---------------------------------------------------------------------------------------------------------------- contract tail
def finish(ctype, p, seed, mult, guid, rng, kws, req, c, params, title, synopsis, topic, subject, flags, extra):
    agent = w37.suitable_agent(rng, kws, req)
    if agent is None:
        return None
    contract = {"fa": c.fa, "fc": c.fc, "ff": c.ff, "rc": c.rc, "rf": c.rf, "sc": c.sc, "expiry": c.expiry, "deadline": c.deadline}
    for nm, f in agent["mentality"]:
        if w37.can_process(nm, kws, req):
            w37.apply_mentality(contract, nm, f, p)
    contract["ff"] = max(contract["ff"], contract["fa"])
    ss = str(super_seed(guid, seed))
    num = w37.wrap64(391 + w37.hash35(ctype))
    num = w37.wrap64(num * 23 + w37.hash35(ss))
    for pr in params:
        num = w37.wrap64(num * 23 + w37.hash35(pr["type"]))
        if pr["hash"]:
            num = w37.wrap64(num * 23 + w37.hash35(pr["hash"]))
    desc = w37.back_story(ctype, agent["title"], topic, subject, seed, *flags)
    res = {"type": ctype, "prestige": PRESTIGE_NAMES[p], "seed": seed, "agent": agent["name"], "agentTitle": agent["title"], "keywords": kws, "title": title,
           "synopsis": synopsis, "description": desc, "contractID": num, "params": params, "fa": contract["fa"], "fc": contract["fc"], "ff": contract["ff"],
           "rc": contract["rc"], "rf": contract["rf"], "sc": contract["sc"], "expiry": contract["expiry"], "deadline": contract["deadline"]}
    res.update(extra)
    return res


def ss_hash(guid, seed):
    return str(super_seed(guid, seed))


def vsp_title(descs, vdesc, manned, require_new):
    """VesselSystemsParameter.GetTitle with check descriptions"""
    mstr = "" if manned == "ANY" else (LOC.fmt("#autoLOC_286382") if manned == "UNMANNED" else LOC.fmt("#autoLOC_286380"))
    if descs:
        joined = this_this_and_that(descs)
        if require_new:
            return LOC.fmt("#autoLOC_7001085", mstr, vdesc, joined) + "\n" + LOC.fmt("#autoLOC_6002645", vdesc)
        return LOC.fmt("#autoLOC_7001086", vdesc, joined)
    if require_new:
        return LOC.fmt("#autoLOC_286401", mstr, vdesc) + "\n" + LOC.fmt("#autoLOC_6002645", vdesc)
    if manned != "ANY":
        return LOC.fmt("#autoLOC_286404", vdesc, mstr)
    return LOC.fmt("#autoLOC_286407", vdesc)


def this_this_and_that(lst, connector="#autoLOC_6002373", conj="#autoLOC_6002374"):
    if not lst:
        return "(NO ELEMENTS)"
    if len(lst) == 1:
        return lst[0]
    conn = LOC.fmt(connector)
    cj = LOC.fmt(conj)
    out = ""
    for i in range(len(lst) - 2):
        out += LOC.fmt(lst[i]) + conn
    out += LOC.fmt(lst[-2]) + cj + LOC.fmt(lst[-1])
    return out


def las_title(body, situation, noun, final=False):
    flag = 0 if final else 1
    disp = BD_DISPLAY(body)
    tag = {"SUB_ORBITAL": "#autoLOC_7000017", "FLYING": "#autoLOC_7000012", "LANDED": "#autoLOC_7000013", "SPLASHED": "#autoLOC_7000016", "PRELAUNCH": "#autoLOC_7000015",
           "DOCKED": "#autoLOC_7000010", "ESCAPING": "#autoLOC_7000011", "ORBITING": "#autoLOC_7000014"}.get(situation, "#autoLOC_7000018")
    return LOC.fmt(tag, str(flag), noun, disp)


def BD_DISPLAY(name):
    return w37.BODY[name]["display"]


def stab_title(hold=10.0):
    return LOC.fmt("#autoLOC_285413", str(int(hold + 0.5)))


def stab_param(ss):
    return {"type": "StabilityParameter", "hash": ss, "title": stab_title()}


# ---------------------------------------------------------------------------------------------------------------- ProcessSideRequests
def process_side_requests(c, node, prestige, seed, fx, kws, params, ss, vessel_name):
    """SystemUtilities.ProcessSideRequests with no vessel and an empty crew roster. Returns (fm, sm, rm) multipliers, mutates c / kws / params."""
    g = KSPRandom(seed)
    if prestige == 2:
        num = g.next_range(2, 4)
        name = "Exceptional"
    elif prestige == 0:
        num = g.next_range(0, 2)
        name = "Trivial"
    else:
        num = g.next_range(1, 3)
        name = "Significant"
    lst = []
    for n in reversed(node["nodes"]):
        if n["name"] not in ("PART_REQUEST", "RESOURCE_REQUEST", "CREW_REQUEST"):
            continue
        parts = [v for k, v in n["values"] if k == "Part"]
        mods = [v for k, v in n["values"] if k == "Module"]
        if not fx.have_any_tech(parts, mods):
            continue
        if n["name"] == "CREW_REQUEST":
            continue  # num5 = prestige + 1 > CrewTraitMissionAvailability (0)
        sub = w37.child(n, name)
        wgt = 1
        if sub is not None and w37.val(sub, "Weight") is not None:
            wgt = int(w37.val(sub, "Weight"))
        lst.append((n, wgt))
    shuffle(lst, g)
    fm = sm = rm = 1.0
    for _ in range(num):
        if not lst:
            break
        total = sum(w for _, w in lst)
        if total <= 0:
            continue
        n9 = g.next_max(total)
        chosen = None
        for i in range(len(lst) - 1, -1, -1):
            n9 -= lst[i][1]
            if n9 <= 0:
                chosen = lst[i][0]
                del lst[i]
                break
        if chosen is None:
            continue
        sub = w37.child(chosen, name)

        def lv(nd, key, default):
            if nd is None or w37.val(nd, key) is None:
                return default
            return F(float(w37.val(nd, key)))

        v4 = lv(sub, "FundsMultiplier", 1.0)
        v5 = lv(sub, "ScienceMultiplier", 1.0)
        v6 = lv(sub, "ReputationMultiplier", 1.0)
        kw = w37.val(chosen, "Keyword") or ""
        v7 = lv(chosen, "MinimumFunds", 0.0)
        v8 = lv(chosen, "MinimumScience", 0.0)
        v9 = lv(chosen, "MinimumReputation", 0.0)
        fm = F(fm * v4)
        sm = F(sm * v5)
        rm = F(rm * v6)
        c.min_fc = max(c.min_fc, v7)
        c.min_sc = max(c.min_sc, v8)
        c.min_rc = max(c.min_rc, v9)
        if kw and kw not in kws:
            kws.append(kw)
        if chosen["name"] == "RESOURCE_REQUEST":
            rname = w37.val(chosen, "Name") or "Matter"
            rtitle = w37.val(chosen, "Title") or "matter"
            amount = lv(sub, "Amount", 1000.0)
            params.append({"type": "ResourcePossessionParameter", "hash": ss, "kind": "possession",
                           "title": LOC.fmt("#autoLOC_284677", thousands(amount), rtitle, vessel_name)})
        elif chosen["name"] == "PART_REQUEST":
            pdesc = w37.val(chosen, "PartDescription") or "part"
            vdesc = w37.val(chosen, "VesselDescription") or "vessel"
            params.append({"type": "PartRequestParameter", "hash": ss, "kind": "partRequest", "title": LOC.fmt("#autoLOC_7001088", pdesc, vdesc)})
    return fm, sm, rm


class CC(w37.C):
    def __init__(self, p, mult):
        super().__init__(p, mult)
        self.min_fc = 0.0
        self.min_sc = 0.0
        self.min_rc = 0.0


# ---------------------------------------------------------------------------------------------------------------- ARMContract
def gen_arm(seed, p, s, W, mult, guid, fx):
    # MeetRequirements: Sun Orbit complete and UnlockedSpaceObjectDiscovery(1.0f) (> 0.6)
    if not s.node_complete("Sun", "Orbit"):
        return None
    if not fx.researched("Grapple"):
        return None
    g = KSPRandom(seed)
    rng = w37.Unity(seed)
    c = CC(p, mult)
    n1 = n2 = n3 = 1.0
    kws = []
    notsd = lambda n: n != "Sun" and n != "Dres"
    if p == 0:
        cls = "A"
        tb = HOME
    elif p == 1:
        cls = "B" if coin(g) else "C"
        lst = bp(s, "ORBIT", True, "ANY", notsd)
        if float(g.next_range(0, 100)) < cf("ARM", "", "SignificantSolarEjectionChance") and cb("ARM", "", "AllowSolarEjections"):
            lst.append("Sun")
        if not lst:
            lst.append(HOME)
        tb = wb(g, lst, W)
    else:
        cls = "D" if coin(g) else "E"
        lst = bp(s, "ORBIT", True, "ANY", notsd)
        lst += next_unreached_m(s, 2, "ANY", notsd)
        if float(g.next_range(0, 100)) < cf("ARM", "", "ExceptionalSolarEjectionChance") and cb("ARM", "", "AllowSolarEjections"):
            lst.append("Sun")
        if not lst:
            lst.append(HOME)
        tb = wb(g, lst, W)
    ss = ss_hash(guid, seed)
    params = [{"type": "AsteroidParameter", "hash": ss, "title": LOC.fmt("#autoLOC_282739", cls), "notes": None}]
    landing = False
    noun = LOC.fmt("#autoLOC_7001016")
    if tb == "Sun":
        w37.add_keywords(kws, "Pioneer")
        params.append({"type": "LocationAndSituationParameter", "hash": ss, "title": las_title(tb, "ESCAPING", noun)})
        n1 = F(n1 * cf("ARM", "Funds", "SolarEjectionMultiplier"))
        n2 = F(n2 * cf("ARM", "Science", "SolarEjectionMultiplier"))
        n3 = F(n3 * cf("ARM", "Reputation", "SolarEjectionMultiplier"))
    elif tb == HOME and float(g.next_range(0, 101)) < cf("ARM", "", "HomeLandingChance") and cb("ARM", "", "AllowHomeLandings"):
        landing = True
        params.append({"type": "LocationAndSituationParameter", "hash": ss, "title": las_title(tb, "LANDED", noun)})
    else:
        params.append({"type": "LocationAndSituationParameter", "hash": ss, "title": las_title(tb, "ORBITING", noun)})
    k = {"E": 2.0, "D": 1.75, "C": 1.5, "B": 1.25}.get(cls, 1.0)
    n1, n2, n3 = F(n1 * k), F(n2 * k), F(n3 * k)
    w37.add_keywords(kws, "Scientific")
    c.set_expiry_draw(rng.range_i(ci("ARM", "Expiration", "MinimumExpireDays"), ci("ARM", "Expiration", "MaximumExpireDays")))
    c.set_deadline_days(float(ci("ARM", "Expiration", "DeadlineDays")), tb)
    c.set_funds(mround(cf("ARM", "Funds", "BaseAdvance") * n1), mround(cf("ARM", "Funds", "BaseReward") * n1), mround(cf("ARM", "Funds", "BaseFailure") * n1), tb)
    c.set_science(mround(cf("ARM", "Science", "BaseReward") * n2))
    c.set_rep(mround(cf("ARM", "Reputation", "BaseReward") * n3), mround(cf("ARM", "Reputation", "BaseFailure") * n3))
    params.append(stab_param(ss))
    disp = BD_DISPLAY(tb)
    home_disp = BD_DISPLAY(HOME)
    if tb != "Sun":
        title = LOC.fmt("#autoLOC_278134" if landing else "#autoLOC_278136", cls, disp)
    else:
        title = LOC.fmt("#autoLOC_278139", cls)
    gk = KSPRandom(seed)
    if tb != "Sun":
        if landing:
            synopsis = LOC.fmt("#autoLOC_278155", cls, disp)
        else:
            d = gk.next_range(0, 3)
            synopsis = LOC.fmt("#autoLOC_278166", home_disp, cls, disp) if d == 2 else (LOC.fmt("#autoLOC_278164", cls, disp) if d == 1 else LOC.fmt("#autoLOC_278162", cls, disp))
    else:
        d = gk.next_range(0, 3)
        synopsis = LOC.fmt("#autoLOC_278179", cls) if d == 2 else (LOC.fmt("#autoLOC_278177", cls, home_disp) if d == 1 else LOC.fmt("#autoLOC_278175", cls))
    topic, subject = disp, LOC.fmt("#autoLOC_7001015")
    return finish("ARMContract", p, seed, mult, guid, rng, kws, [], c, params, title, synopsis, topic, subject, (True, False, True),
                  {"targetBody": tb, "asteroidClass": cls, "isLanding": landing})


# ---------------------------------------------------------------------------------------------------------------- BaseContract / StationContract
def gen_base(seed, p, s, W, mult, guid, fx):
    if not s.node_complete(HOME, LOC.fmt("#autoLOC_278465")):
        return None
    if not fx.researched("Antenna", "Dock", "Generator"):
        return None
    g = KSPRandom(seed)
    rng = w37.Unity(seed)
    c = CC(p, mult)
    fm = sm = rm = 1.0
    kws = []
    solid = lambda n: BD[n]["solid"] and n != "Sun" and n != HOME
    lst = bp(s, "LANDING", True, "MANNED", solid)
    if p == 1:
        lst += next_unreached_m(s, 1, "MANNED", solid)
    elif p == 2:
        lst += next_unreached_m(s, 2, "MANNED", solid)
    if not lst:
        return None
    tb = wb(g, lst, W)
    num = 5
    num2 = 5 + g.next_range(1, 8)
    num3 = 7 + g.next_range(1, 14)
    num4 = w37.round_to_int(F(cf("Base", "", "ContextualChance") * (min(0.0, cf("Base", "", "ContextualAssets")) / cf("Base", "", "ContextualAssets"))))
    g.next_range(0, 100)  # < num4 (0): the draw happens
    if s.sc.complete(tb, "LANDING", "MANNED"):
        cap = [num, num2, num3][p]
    else:
        cap = [num, num, num2][p]
        w37.add_keywords(kws, "Pioneer")
    ss = ss_hash(guid, seed)
    descs = [LOC.fmt("#autoLOC_7001075"), LOC.fmt("#autoLOC_7001076"), LOC.fmt("#autoLOC_7001077")]
    vdesc = LOC.fmt("#autoLOC_7001078")
    params = [{"type": "VesselSystemsParameter", "hash": ss, "title": vsp_title(descs, vdesc, "ANY", True)},
              {"type": "CrewCapacityParameter", "hash": ss, "title": LOC.fmt("#autoLOC_282944", str(cap))}]
    num6 = [cf("Base", "", "TrivialMobileChance"), cf("Base", "", "SignificantMobileChance"), cf("Base", "", "ExceptionalMobileChance")][p]
    bnode = cfg_node("Base")
    fm, sm, rm = process_side_requests(c, bnode, p, seed, fx, kws, params, ss, LOC.fmt("#autoLOC_278368"))
    if fx.researched("Wheel") and cb("Base", "", "AllowMobile") and float(g.next_range(0, 100)) < num6:
        w37.add_keywords(kws, "Pioneer")
        params.append({"type": "MobileBaseParameter", "hash": ss, "title": LOC.fmt("#autoLOC_7001018")})
        fm = F(fm * cf("Base", "Funds", "MobileMultiplier"))
        sm = F(sm * cf("Base", "Science", "MobileMultiplier"))
        rm = F(rm * cf("Base", "Reputation", "MobileMultiplier"))
    params.append(stab_param(ss))
    params.append({"type": "LocationAndSituationParameter", "hash": ss, "title": las_title(tb, "LANDED", LOC.fmt("#autoLOC_278346"))})
    n7 = F(max(c.min_fc, cf("Base", "Funds", "BaseReward")))
    n8 = F(max(c.min_sc, cf("Base", "Science", "BaseReward")))
    n9 = F(max(c.min_rc, cf("Base", "Reputation", "BaseReward")))
    c.set_expiry_draw(rng.range_i(ci("Base", "Expiration", "MinimumExpireDays"), ci("Base", "Expiration", "MaximumExpireDays")))
    c.set_deadline_days(float(ci("Base", "Expiration", "DeadlineDays")), tb)
    c.set_funds(mround(cf("Base", "Funds", "BaseAdvance") * fm), mround(n7 * fm), mround(cf("Base", "Funds", "BaseFailure") * fm), tb)
    c.set_science(mround(n8 * sm))
    c.set_rep(mround(n9 * rm), mround(cf("Base", "Reputation", "BaseFailure") * rm))
    disp = BD_DISPLAY(tb)
    title = LOC.fmt("#autoLOC_7001080", disp)
    synopsis = LOC.fmt("#autoLOC_278438", str(cap), disp)
    greek = GREEK_FIX[KSPRandom(seed).next_max(23)]
    topic = disp + " Base " + greek
    return finish("BaseContract", p, seed, mult, guid, rng, kws, [], c, params, title, synopsis, topic, disp + "Srf", (True, False, True),
                  {"targetBody": tb, "capacity": cap})


def gen_station(seed, p, s, W, mult, guid, fx):
    if not s.node_complete(HOME, "Orbit"):
        return None
    if not fx.researched("Antenna", "Dock", "Generator"):
        return None
    g = KSPRandom(seed)
    rng = w37.Unity(seed)
    c = CC(p, mult)
    kws = []
    lst = bp(s, "ORBIT", True, "MANNED", lambda n: True)
    if p == 2:
        lst += next_unreached_m(s, 2, "MANNED", None)
        lst = [n for n in lst if n != HOME]
    elif p == 1:
        lst += next_unreached_m(s, 1, "MANNED", None)
    if not cb("Station", "", "AllowSolar"):
        lst = [n for n in lst if n != "Sun"]
    if not lst:
        return None
    tb = wb(g, lst, W)
    num = 5
    num2 = 5 + g.next_range(1, 8)
    num3 = 7 + g.next_range(1, 14)
    g.next_range(0, 100)  # contextual draw (num4 = 0)
    if s.sc.complete(tb, "ORBIT", "MANNED"):
        cap = [num, num2, num3][p]
    else:
        cap = [num, num, num2][p]
        w37.add_keywords(kws, "Pioneer")
    ss = ss_hash(guid, seed)
    text = LOC.fmt("#autoLOC_7001017")
    descs = [LOC.fmt("#autoLOC_7001075"), LOC.fmt("#autoLOC_7001076"), LOC.fmt("#autoLOC_7001077")]
    params = [{"type": "VesselSystemsParameter", "hash": ss, "title": vsp_title(descs, text, "ANY", True)},
              {"type": "CrewCapacityParameter", "hash": ss, "title": LOC.fmt("#autoLOC_282944", str(cap))}]
    num6 = [cf("Station", "", "TrivialAsteroidChance"), cf("Station", "", "SignificantAsteroidChance"), cf("Station", "", "ExceptionalAsteroidChance")][p]
    size = [("A", "B"), ("B", "C"), ("D", "E")][p][0 if coin(g) else 1]
    fm, sm, rm = process_side_requests(c, cfg_node("Station"), p, seed, fx, kws, params, ss, text)
    asteroids = (cb("Station", "", "AllowAsteroid") and s.node_complete("Sun", "Orbit") and fx.researched("Grapple"))
    # GameVariables.UnlockedSpaceObjectDiscovery(1.0f) is true
    if asteroids:
        if float(g.next_range(0, 100)) < num6:
            w37.add_keywords(kws, "Pioneer")
            params.append({"type": "AsteroidParameter", "hash": ss, "title": LOC.fmt("#autoLOC_282737", size)})
            fm = F(fm * cf("Station", "Funds", "AsteroidMultiplier"))
            sm = F(sm * cf("Station", "Science", "AsteroidMultiplier"))
            rm = F(rm * cf("Station", "Reputation", "AsteroidMultiplier"))
    params.append(stab_param(ss))
    params.append({"type": "LocationAndSituationParameter", "hash": ss, "title": las_title(tb, "ORBITING", text)})
    n7 = F(max(c.min_fc, cf("Station", "Funds", "BaseReward")))
    n8 = F(max(c.min_sc, cf("Station", "Science", "BaseReward")))
    n9 = F(max(c.min_rc, cf("Station", "Reputation", "BaseReward")))
    c.set_expiry_draw(rng.range_i(ci("Station", "Expiration", "MinimumExpireDays"), ci("Station", "Expiration", "MaximumExpireDays")))
    c.set_deadline_days(float(ci("Station", "Expiration", "DeadlineDays")), tb)
    c.set_funds(mround(cf("Station", "Funds", "BaseAdvance") * fm), mround(n7 * fm), mround(cf("Station", "Funds", "BaseFailure") * fm), tb)
    c.set_science(mround(n8 * sm))
    c.set_rep(mround(n9 * rm), mround(cf("Station", "Reputation", "BaseFailure") * rm))
    disp = BD_DISPLAY(tb)
    t1 = LOC.fmt("#autoLOC_7001020")
    t2 = LOC.fmt("#autoLOC_7001021") if tb == "Sun" else LOC.fmt("#autoLOC_7001022", disp)
    title = LOC.fmt("#autoLOC_6002375", t1, t2)
    synopsis = LOC.fmt("#autoLOC_7001071", str(cap)) if tb == "Sun" else LOC.fmt("#autoLOC_7001072", str(cap), disp)
    greek = GREEK_FIX[KSPRandom(seed).next_max(23)]
    topic = disp + " Station " + greek
    return finish("StationContract", p, seed, mult, guid, rng, kws, [], c, params, title, synopsis, topic, disp, (True, True, True),
                  {"targetBody": tb, "capacity": cap})


# ---------------------------------------------------------------------------------------------------------------- ISRUContract
RESOURCES = {}


def load_resources():
    for fn in ("ResourcesGeneric.cfg", "Ore.cfg"):
        root = w37.parse_cfg(os.path.join(w37.SQUAD, "Resources", fn))
        for n in root["nodes"]:
            if n["name"] == "RESOURCE_DEFINITION":
                RESOURCES[w37.val(n, "name")] = {"name": w37.val(n, "name"), "display": LOC.single(w37.val(n, "displayName") or w37.val(n, "name"))}


load_resources()


def gen_isru(seed, p, s, W, mult, guid, fx):
    if not s.node_complete(HOME, "Orbit"):
        return None
    g = KSPRandom(seed)
    rng = w37.Unity(seed)
    c = CC(p, mult)
    node = cfg_node("ISRU")
    cand = [n for n in node["nodes"] if n["name"] == "RESOURCE_REQUEST" and fx.have_any_tech([], [v for k, v in n["values"] if k == "Module"])]
    if not cand:
        return None
    cn = cand[g.next_range(0, len(cand))]
    res = w37.val(cn, "Name") or "Matter"
    rtitle = w37.val(cn, "Title") or (RESOURCES[res]["display"] if res in RESOURCES else res)
    notHome = lambda n: n != HOME
    lst = bodies_reached(s, True, "ANY", notHome)
    if p == 1:
        lst += next_unreached_m(s, 1, "ANY", notHome)
    elif p == 2:
        lst += next_unreached_m(s, 2, "ANY", notHome)
    if not lst:
        return None
    forbidden = [v for k, v in cn["values"] if k == "Forbidden"]
    while True:
        tb = wb(g, lst, W)
        if tb not in forbidden:
            break
        lst.remove(tb)
        if not lst:
            return None
    sub = w37.child(cn, PRESTIGE_NAMES[p])
    d = [(500.0, 50, 1.0), (1000.0, 65, 1.1), (2500.0, 80, 1.3)][p]

    def lv(key, default):
        if sub is None or w37.val(sub, key) is None:
            return F(default)
        return F(float(w37.val(sub, key)))

    goal = lv("Amount", d[0])
    chance = int(w37.val(sub, "DeliveryChance")) if sub is not None and w37.val(sub, "DeliveryChance") is not None else d[1]
    v1 = lv("FundsMultiplier", d[2])
    v2 = lv("ScienceMultiplier", d[2])
    v3 = lv("ReputationMultiplier", d[2])
    delivering = False
    delivery = None
    if g.next_range(0, 100) < chance:
        nb = neighbors(tb, lambda n: n != "Sun")
        delivery = wb(g, nb, W)
        delivering = True
    if delivery is None:
        delivering = False
    if delivering:
        v5 = F(float(w37.val(cn, "DeliveryMultiplier"))) if w37.val(cn, "DeliveryMultiplier") is not None else 1.8
        v1, v2, v3 = F(v1 * v5), F(v2 * v5), F(v3 * v5)
    num = F(F(g.next_double()) * 0.25)
    num = F(1.0 - num) if g.next_range(0, 100) <= 50 else F(1.0 + num)
    v1, v2, v3 = F(v1 * num), F(v2 * num), F(v3 * num)
    goal = F(goal * num)
    goal = F(w37.m_round(F(goal / 50.0)) * 50.0)
    ss = ss_hash(guid, seed)
    params = [{"type": "ResourceExtractionParameter", "hash": ss, "title": LOC.fmt("#autoLOC_284531", thousands(goal), rtitle, BD_DISPLAY(tb))}]
    sit = None
    if delivering:
        sit = applicable_situation(seed, delivery, False)
        params.append({"type": "ResourcePossessionParameter", "hash": ss, "title": LOC.fmt("#autoLOC_284677", thousands(goal), rtitle, LOC.fmt("#autoLOC_7001016"))})
        params.append({"type": "LocationAndSituationParameter", "hash": ss, "title": las_title(delivery, sit, RESOURCES[res]["display"])})
        params.append(stab_param(ss))
    kw = w37.val(cn, "Keyword") or "ISRU"
    kws = []
    w37.add_keywords(kws, kw)
    c.set_expiry_draw(rng.range_i(ci("ISRU", "Expiration", "MinimumExpireDays"), ci("ISRU", "Expiration", "MaximumExpireDays")))
    c.set_deadline_days(float(ci("ISRU", "Expiration", "DeadlineDays")), tb)
    c.set_funds(mround(cf("ISRU", "Funds", "BaseAdvance") * v1), mround(cf("ISRU", "Funds", "BaseReward") * v1), mround(cf("ISRU", "Funds", "BaseFailure") * v1), tb)
    c.set_science(mround(cf("ISRU", "Science", "BaseReward") * v2))
    c.set_rep(mround(cf("ISRU", "Reputation", "BaseReward") * v3), mround(cf("ISRU", "Reputation", "BaseFailure") * v3))
    disp = BD_DISPLAY(tb)
    gs = thousands(goal)
    if delivering:
        title = LOC.fmt("#autoLOC_279475", rtitle, disp, BD_DISPLAY(delivery))
        synopsis = LOC.fmt("#autoLOC_279488", rtitle, disp, gs, BD_DISPLAY(delivery))
    else:
        title = LOC.fmt("#autoLOC_279477", gs, rtitle, disp)
        synopsis = LOC.fmt("#autoLOC_279490", rtitle, disp, gs)
    topic = LOC.fmt(rtitle).lower()
    return finish("ISRUContract", p, seed, mult, guid, rng, kws, [], c, params, title, synopsis, topic, disp + "Srf", (True, False, True),
                  {"targetBody": tb, "deliveryBody": delivery, "isDelivering": delivering, "gatherGoal": goal, "targetResource": res, "resourceTitle": rtitle,
                   "deliverySituation": sit})


def applicable_situation(seed, body, splash_allowed):
    """CelestialUtilities.ApplicableSituation (CelestialUtilities.cs:256)"""
    g = KSPRandom(seed)
    b = BD[body]
    lst = ["ORBITING"]
    if b["ocean"] and splash_allowed:
        lst.append("SPLASHED")
    if b["solid"]:
        lst.append("LANDED")
    return lst[g.next_range(0, len(lst))]


# ---------------------------------------------------------------------------------------------------------------- SatelliteContract
def sat_title(otype, mode, tb, p):
    disp = BD_DISPLAY(tb)
    spec = "SPECIFICVESSEL"

    def sync():
        if tb == "Sun":
            return LOC.fmt("#autoLOC_7001208").lower()
        if tb == HOME:
            return LOC.fmt("#autoLOC_7001209").lower()
        return LOC.fmt("#autoLOC_7001210").lower()

    def stat():
        if tb == "Sun":
            return LOC.fmt("#autoLOC_7001205").lower()
        if tb == HOME:
            return LOC.fmt("#autoLOC_7001206").lower()
        return LOC.fmt("#autoLOC_7001207").lower()

    if otype == "SYNCHRONOUS":
        return LOC.fmt("#autoLOC_279968", sync(), disp)
    if otype == "STATIONARY":
        return LOC.fmt("#autoLOC_279953", stat(), disp)
    if otype == "POLAR":
        return LOC.fmt("#autoLOC_279922", disp)
    if otype == "EQUATORIAL":
        return LOC.fmt("#autoLOC_279920", disp)
    if otype == "KOLNIYA":
        return LOC.fmt("#autoLOC_279930", title_case(LOC.fmt("#autoLOC_7001203")), disp)
    if otype == "TUNDRA":
        return LOC.fmt("#autoLOC_279938", disp)
    return LOC.fmt("#autoLOC_279970", disp)


def sat_synopsis(otype, tb):
    disp = BD_DISPLAY(tb)
    if otype == "SYNCHRONOUS":
        nm = (LOC.fmt("#autoLOC_7001208") if tb == "Sun" else (LOC.fmt("#autoLOC_7001209") if tb == HOME else LOC.fmt("#autoLOC_7001210"))).lower()
        return LOC.fmt("#autoLOC_280063", nm, disp)
    if otype == "STATIONARY":
        nm = (LOC.fmt("#autoLOC_7001205") if tb == "Sun" else (LOC.fmt("#autoLOC_7001206") if tb == HOME else LOC.fmt("#autoLOC_7001207"))).lower()
        return LOC.fmt("#autoLOC_280048", nm, disp)
    if otype == "POLAR":
        return LOC.fmt("#autoLOC_280017", disp)
    if otype == "EQUATORIAL":
        return LOC.fmt("#autoLOC_280015", disp)
    if otype == "KOLNIYA":
        return LOC.fmt("#autoLOC_280025", title_case(LOC.fmt("#autoLOC_7001203")), disp)
    if otype == "TUNDRA":
        return LOC.fmt("#autoLOC_280033", disp)
    return LOC.fmt("#autoLOC_280065", disp)


def sat_completed(otype, tb):
    disp = BD_DISPLAY(tb)
    if otype == "SYNCHRONOUS":
        nm = (LOC.fmt("#autoLOC_7001208") if tb == "Sun" else (LOC.fmt("#autoLOC_7001209") if tb == HOME else LOC.fmt("#autoLOC_7001210"))).lower()
        return LOC.fmt("#autoLOC_280125", nm, disp)
    if otype == "STATIONARY":
        nm = (LOC.fmt("#autoLOC_7001205") if tb == "Sun" else (LOC.fmt("#autoLOC_7001206") if tb == HOME else LOC.fmt("#autoLOC_7001207"))).lower()
        return LOC.fmt("#autoLOC_280110", nm, disp)
    if otype == "POLAR":
        return LOC.fmt("#autoLOC_280079", disp)
    if otype == "EQUATORIAL":
        return LOC.fmt("#autoLOC_280077", disp)
    if otype == "KOLNIYA":
        return LOC.fmt("#autoLOC_280087", title_case(LOC.fmt("#autoLOC_7001203")), disp)
    if otype == "TUNDRA":
        return LOC.fmt("#autoLOC_280095", disp)
    return LOC.fmt("#autoLOC_280127", disp)


def title_case(s):
    out = []
    for w in re.split(r"(\W+)", s):
        out.append(w[:1].upper() + w[1:].lower() if w.isalpha() and not (w.isupper() and len(w) > 1) else w)
    return "".join(out)


def orbit_title(otype, tb, p):
    """SpecificOrbitParameter.GetTitle"""
    text = LOC.fmt("#autoLOC_284812")
    if p == 0:
        text = LOC.fmt("#autoLOC_284817")
    elif p == 1:
        text = LOC.fmt("#autoLOC_284820")
    else:
        text = LOC.fmt("#autoLOC_284823")
    disp = BD_DISPLAY(tb)
    sun, home = tb == "Sun", tb == HOME
    if otype == "SYNCHRONOUS":
        return LOC.fmt("#autoLOC_7001089", LOC.fmt("#autoLOC_7001208" if sun else ("#autoLOC_7001209" if home else "#autoLOC_7001210")), disp, text)
    if otype == "STATIONARY":
        return LOC.fmt("#autoLOC_7001090", LOC.fmt("#autoLOC_7001205" if sun else ("#autoLOC_7001206" if home else "#autoLOC_7001207")), disp, text)
    if otype == "POLAR":
        return LOC.fmt("#autoLOC_7001089", LOC.fmt("#autoLOC_7001202"), disp, text)
    if otype == "EQUATORIAL":
        return LOC.fmt("#autoLOC_7001089", LOC.fmt("#autoLOC_7001201"), disp, text)
    if otype == "KOLNIYA":
        return LOC.fmt("#autoLOC_7001089", title_case(LOC.fmt("#autoLOC_7001203")), disp, text)
    if otype == "TUNDRA":
        return LOC.fmt("#autoLOC_7001089", LOC.fmt("#autoLOC_7001204"), disp, text)
    return LOC.fmt("#autoLOC_7001091", disp, text)


def set_orbit_type(tb, target, alt_factor):
    n = BD[tb]
    allow = {"SYNCHRONOUS": "AllowSynchronous", "STATIONARY": "AllowStationary", "POLAR": "AllowPolar", "EQUATORIAL": "AllowEquatorial", "KOLNIYA": "AllowKolniya",
             "TUNDRA": "AllowTundra"}
    if target in allow and not cb("Satellite", "", allow[target]):
        return "RANDOM"
    if target in ("SYNCHRONOUS", "STATIONARY"):
        return target if w38.can_sync(n, alt_factor / 2.0) else "RANDOM"
    if target == "KOLNIYA":
        return target if w38.can_kol(n) else "RANDOM"
    if target == "TUNDRA":
        return target if w38.can_tundra(n) else "RANDOM"
    return target


def gen_satellite(seed, p, s, W, mult, guid, fx):
    # MeetRequirements: Home Orbit complete; GetOrbitDisplayMode(1.0f) = PatchedConics (not < AllOrbits)
    if not s.node_complete(HOME, "Orbit"):
        return None
    if not (fx.probe and fx.researched("Antenna", "Generator")):
        return None
    rng = w37.Unity(seed)
    c = CC(p, mult)
    sseed = super_seed(guid, seed)
    g = KSPRandom(sseed)
    fm = sm = rm = 1.0
    kws = []
    notsun = lambda n: n != "Sun"
    lst = bp(s, "ORBIT", True, "ANY", notsun)
    lst += next_unreached_m(s, p, "UNMANNED", notsun)
    if cb("Satellite", "", "PreferHome") and g.next_range(0, p + 1) == p:
        lst.append(HOME)
    if cb("Satellite", "", "AllowSolar") and g.next_range(0, 3) < p and s.node_complete("Sun", "Orbit"):
        lst.append("Sun")
    if not lst:
        lst.append(HOME)
    pre = ["Trivial", "Significant", "Exceptional"][p]
    deviation = cd("Satellite", "", pre + "Deviation")
    alt_f = cf("Satellite", "", pre + "AltitudeDifficulty")
    inc_f = cf("Satellite", "", pre + "InclinationDifficulty")
    tb = wb(g, lst, W)

    def pick_easy():
        n = g.next_range(0, 101)
        if n <= 33:
            return set_orbit_type(tb, "RANDOM", alt_f)
        if n <= 66:
            return set_orbit_type(tb, "POLAR", alt_f)
        return set_orbit_type(tb, "EQUATORIAL", alt_f)

    def pick_medium():
        n = g.next_range(0, 101)
        if n <= 33:
            return pick_easy()
        if n <= 66:
            return set_orbit_type(tb, "RANDOM", alt_f)
        return set_orbit_type(tb, "SYNCHRONOUS", alt_f)

    def pick_hard():
        n = g.next_range(0, 101)
        if n <= 20:
            return pick_medium()
        if n <= 40:
            return set_orbit_type(tb, "RANDOM", alt_f)
        if n <= 60:
            return set_orbit_type(tb, "KOLNIYA", alt_f)
        if n <= 80:
            return set_orbit_type(tb, "TUNDRA", alt_f)
        return set_orbit_type(tb, "STATIONARY", alt_f)

    otype = [pick_easy, pick_medium, pick_hard][p]()
    num5 = cf("Satellite", "", "ContextualHomeAssets") if tb == HOME else cf("Satellite", "", "ContextualAssets")
    num6 = w37.round_to_int(F(cf("Satellite", "", "ContextualChance") * (min(0.0, num5) / num5)))
    g.next_range(0, 100)  # < num6 (0): the draw happens
    ss = ss_hash(guid, seed)
    descs = [LOC.fmt("#autoLOC_7001075"), LOC.fmt("#autoLOC_7001077")]
    params = [{"type": "VesselSystemsParameter", "hash": ss, "title": vsp_title(descs, LOC.fmt("#autoLOC_7001055"), "UNMANNED", True)}]
    ecc_override = 0.0
    if otype == "SYNCHRONOUS":
        ecc_override = g.next_double() * (alt_f / 2.0)
    el = w38.gen_orbit_raw(sseed, tb, otype, alt_f, inc_f, ecc_override)
    params.append({"type": "SpecificOrbitParameter", "hash": ss, "title": orbit_title(otype, tb, p), "orbit": el, "deviation": deviation, "orbitType": otype})
    stat_lon = None
    if otype == "STATIONARY":
        _, stat_lon = w38.choose_random_position(tb, False, True, RANDOM_SEED_FIXTURE)
        params.append({"type": "StationaryPointParameter", "hash": ss, "longitude": stat_lon, "deviation": deviation})
    fm, sm, rm = process_side_requests(c, cfg_node("Satellite"), p, seed, fx, kws, params, ss, LOC.fmt("#autoLOC_500053"))
    mult_name = {"SYNCHRONOUS": "Synchronous", "STATIONARY": "Stationary", "POLAR": "Polar", "KOLNIYA": "Kolniya", "TUNDRA": "Tundra"}.get(otype)
    if mult_name:
        fm = F(fm * cf("Satellite", "Funds", mult_name + "Multiplier"))
        sm = F(sm * cf("Satellite", "Science", mult_name + "Multiplier"))
        rm = F(rm * cf("Satellite", "Reputation", mult_name + "Multiplier"))
    if tb == HOME:
        w37.add_keywords(kws, "Commercial")
        fm = F(fm * cf("Satellite", "Funds", "HomeMultiplier"))
        sm = F(sm * cf("Satellite", "Science", "HomeMultiplier"))
        rm = F(rm * cf("Satellite", "Reputation", "HomeMultiplier"))
    else:
        w37.add_keywords(kws, "Commercial" if coin(g) else "Scientific")
    params.append(stab_param(ss))
    n7 = F(max(c.min_fc, cf("Satellite", "Funds", "BaseReward")))
    n8 = F(max(c.min_sc, cf("Satellite", "Science", "BaseReward")))
    n9 = F(max(c.min_rc, cf("Satellite", "Reputation", "BaseReward")))
    c.set_expiry_draw(rng.range_i(ci("Satellite", "Expiration", "MinimumExpireDays"), ci("Satellite", "Expiration", "MaximumExpireDays")))
    c.set_deadline_days(float(ci("Satellite", "Expiration", "DeadlineDays")), tb)
    c.set_funds(mround(cf("Satellite", "Funds", "BaseAdvance") * fm), mround(n7 * fm), mround(cf("Satellite", "Funds", "BaseFailure") * fm), tb)
    c.set_science(mround(n8 * sm))
    c.set_rep(mround(n9 * rm), mround(cf("Satellite", "Reputation", "BaseFailure") * rm))
    title = sat_title(otype, "NEW", tb, p)
    synopsis = sat_synopsis(otype, tb)
    topic = {"SYNCHRONOUS": "Synchronous Orbit", "STATIONARY": "Stationary Orbit", "POLAR": "Polar Orbit", "EQUATORIAL": "Equatorial Orbit",
             "KOLNIYA": title_case(LOC.fmt("#autoLOC_7001203")) + " Orbit", "TUNDRA": "Tundra Orbit"}.get(otype, "Satellite")
    return finish("SatelliteContract", p, seed, mult, guid, rng, kws, [], c, params, title, synopsis, topic, BD_DISPLAY(tb), (True, True, True),
                  {"targetBody": tb, "orbitType": otype, "deviation": deviation, "completed": sat_completed(otype, tb), "stationaryLongitude": stat_lon})


# ---------------------------------------------------------------------------------------------------------------- site generator fixture (NOT KSP data)
GREEK_FIX = ["greek%02d" % i for i in range(24)]
SITE_FIX = {
    "GreekMap": GREEK_FIX,
    "Alphanumeric": list("ABCDEFGHJKLMNPQRSTUVWXYZ0123456789"),
    "UniqueSites": ["site-u0", "site-u1"],
    "GenericPrefixes": ["pfx0", "pfx1", "pfx2"],
}


def alpha_numeric(seed):
    g = KSPRandom(seed)
    text = ""
    flag = False
    num = g.next_range(4, 7)
    al = SITE_FIX["Alphanumeric"]
    for i in range(1, num + 1):
        if g.next_range(0, 101) > 70 and i != 1 and i != num and not flag:
            text += "-"
            flag = True
        else:
            text += al[g.next_max(len(al))]
    return text


# ---------------------------------------------------------------------------------------------------------------- scenarios
SCENARIOS = w38.SCENARIOS + [
    {"name": "sun-orbit", "top": {"FirstLaunch": "completed", "ReachedSpace": "completed"},
     "bodies": {"Kerbin": {"Science": "completed", "Orbit": "completed", "ReturnFromOrbit": "completed", "Spacewalk": "completed", "Escape": "completed"},
                "Mun": {"Flyby": "completed", "Orbit": "completedManned", "Landing": "completedManned"},
                "Minmus": {"Flyby": "completed", "Orbit": "completedUnmanned"},
                "Sun": {"Orbit": "completed", "Flyby": "completed"}, "Eve": {"Orbit": "completedUnmanned", "Flyby": "completed"},
                "Duna": {"Orbit": "completedManned", "Flyby": "completed", "Landing": "completedManned"}, "Moho": {"Flyby": "completed"},
                "Dres": {"Orbit": "completed"}, "Jool": {"Orbit": "completedUnmanned"}, "Laythe": {"Flyby": "completed"}, "Eeloo": {"Flyby": "reached"}}},
]
GUID = bytes([0x21, 0x43, 0x65, 0x87, 0xA9, 0xCB, 0xED, 0x0F, 0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE])


def weights():
    d = int(w37.val(w37.CONTRACTS, "WeightDefault") or 30)
    W = {b["name"]: d for b in BODY_LIST}
    W["Mun"] = 55
    W["Minmus"] = 12
    W["Duna"] = 45
    return W


# ---------------------------------------------------------------------------------------------------------------- parameter texts
SITUATIONS = ["LANDED", "SPLASHED", "PRELAUNCH", "FLYING", "SUB_ORBITAL", "ORBITING", "ESCAPING", "DOCKED"]
ENTRY_TYPES = ["Land", "Flight", "Flyby", "Orbit", "Suborbit", "Escape"]
ORBIT_TYPES_ALL = ["SYNCHRONOUS", "STATIONARY", "POLAR", "EQUATORIAL", "KOLNIYA", "TUNDRA", "RANDOM"]


def g15(x):
    """double.ToString() of a value rounded to one decimal (Mono: shortest representation up to 15 digits)"""
    return "%.15g" % x


def round1(x):
    return float(np.round(np.float64(x), 1))


def param_texts():
    out = []
    sun_disp = BD_DISPLAY("Sun")

    def add(**kw):
        out.append(kw)

    for h in (10.0, 0.4, 2.6, 30.0, 1.5):
        add(cls="Stability", hold=h, title=stab_title(h))
    for n in (1, 5, 8, 12):
        add(cls="CrewCapacity", n=n, title=LOC.fmt("#autoLOC_282944", str(n)))
    # ExperienceTraitConfig.Title is the raw cfg value (GameDatabase translates it in the game; the mirror is untranslated), ToLower() leaves a tag unresolved
    traits = {}
    for nd in w37.parse_cfg(os.path.join(w37.SQUAD, "Experience", "Traits.cfg"))["nodes"]:
        if nd["name"] == "EXPERIENCE_TRAIT":
            traits[w37.val(nd, "name")] = w37.val(nd, "title")
    for trait in ("Pilot", "Engineer", "Scientist", "Tourist", "Mystery"):
        disp = traits.get(trait, trait).lower()
        for cnt in (1, 3):
            add(cls="CrewTrait", trait=trait, count=cnt, desc="vessel",
                title=(LOC.fmt("#autoLOC_6001095", disp, "vessel") if cnt <= 1 else LOC.fmt("#autoLOC_283108", str(cnt), disp, "vessel")))
    add(cls="MobileBase", title=LOC.fmt("#autoLOC_7001018"))
    for body in ("Kerbin", "Mun", "Jool"):
        for sit in SITUATIONS + ["XXX"]:
            for final in (False, True):
                add(cls="LocationAndSituation", body=body, sit=sit, final=final, noun="probe", title=las_title(body, sit, "probe", final))
    add(cls="LocationAndSituation", body=None, sit="ORBITING", final=False, noun="probe", title=LOC.fmt("#autoLOC_283645", sun_disp))
    tag_vd = {"Land": "#autoLOC_286262", "Flight": "#autoLOC_286266", "Flyby": "#autoLOC_286268", "Orbit": "#autoLOC_286270", "Suborbit": "#autoLOC_286264"}
    tag_kd = {"Land": "#autoLOC_283270", "Flight": "#autoLOC_283274", "Flyby": "#autoLOC_283276", "Orbit": "#autoLOC_283278", "Suborbit": "#autoLOC_283272"}
    for et in ENTRY_TYPES:
        for body in ("Mun", "Kerbin"):
            add(cls="VesselDestination", body=body, entry=et,
                title=LOC.fmt(tag_vd[et], BD_DISPLAY(body)) if et in tag_vd else LOC.fmt("#autoLOC_286272"))
            add(cls="KerbalDestination", body=body, entry=et, kerbal="Jebediah Kerman",
                title=LOC.fmt(tag_kd[et], BD_DISPLAY(body)) if et in tag_kd else LOC.fmt("#autoLOC_283280"))
    add(cls="KerbalGeeAdventure", kerbal="Jebediah Kerman", title=LOC.fmt("#autoLOC_283371", "Jebediah"))
    for gender in ("Male", "Female"):
        add(cls="KerbalTour", kerbal="Valentina Kerman", gender=gender, title=LOC.fmt("#autoLOC_283467", "Valentina" + ("^M" if gender == "Male" else "^F")))
    descs3 = [LOC.fmt("#autoLOC_7001075"), LOC.fmt("#autoLOC_7001076"), LOC.fmt("#autoLOC_7001077")]
    for dd in ([], descs3, descs3[:1]):
        for manned in ("ANY", "MANNED", "UNMANNED"):
            for rn in (False, True):
                mstr = "" if manned == "ANY" else (LOC.fmt("#autoLOC_286382") if manned == "UNMANNED" else LOC.fmt("#autoLOC_286380"))
                add(cls="VesselSystems", descs=dd, vdesc="probe", manned=manned, requireNew=rn, title=vsp_title(dd, "probe", manned, rn),
                    notes=(LOC.fmt("#autoLOC_7000009", mstr, "probe", "{AGENT}") if rn else None))
    add(cls="PartRequest", article="a", partDescription="#autoLOC_500688", vesselDescription="#autoLOC_500028", parts=["Large_Crewed_Lab"], modules=["ModuleScienceLab"],
        title=LOC.fmt("#autoLOC_7001088", "#autoLOC_500688", "#autoLOC_500028"))
    for goal in (999.5, 1000.0, 123456.4, 0.2):
        add(cls="ResourcePossession", goal=goal, name="Ore", rtitle="ore", vessel="base", title=LOC.fmt("#autoLOC_284677", thousands(goal), "ore", "base"))
        add(cls="ResourceExtraction", goal=goal, name="Ore", rtitle="ore", body="Mun", title=LOC.fmt("#autoLOC_284531", thousands(goal), "ore", BD_DISPLAY("Mun")))
    for cls_ in ("a", "b", "E"):
        for fs in (False, True):
            C = cls_.upper()
            add(cls="Asteroid", size=cls_, forStation=fs, title=LOC.fmt("#autoLOC_282737" if fs else "#autoLOC_282739", C), notes=LOC.fmt("#autoLOC_282744", "{AGENT}", C))
            add(cls="Comet", size=cls_, forStation=fs, title=LOC.fmt("#autoLOC_282737" if fs else "#autoLOC_282739", C), notes=LOC.fmt("#autoLOC_282744", "{AGENT}", C))
    for n in (1, 3):
        add(cls="CometDetection", count=n, title=LOC.fmt("#autoLOC_6011147", str(n)), notes="\n" + LOC.fmt("#autoLOC_8003406"))
    add(cls="CometScience", id=77, name="Borrelly", title=LOC.fmt("#autoLOC_8012045", "Borrelly"), hash="Borrelly77")
    cupola_title = None
    for dp, _dn, fn in os.walk(os.path.join(w37.SQUAD, "Parts")):
        for f in fn:
            if f.endswith(".cfg") and f.startswith("cupola"):
                n = w37.parse_cfg(os.path.join(dp, f))
                for pn in n["nodes"]:
                    if pn["name"] == "PART" and w37.val(pn, "name") == "cupola":
                        cupola_title = LOC.single(w37.val(pn, "title"))
    add(cls="Construction", part="cupola", vessel=1234, vesselName="Duna Base", body="Duna", title=LOC.fmt("#autoLOC_6002602", cupola_title, "Duna Base"),
        hash="cupola1234", complete=LOC.fmt("#autoLOC_6002604"))
    add(cls="Construction", part="NoSuchPart", vessel=9, vesselName="V", body="Duna", title=LOC.fmt("#autoLOC_6002602", "NoSuchPart", "V"), hash="NoSuchPart9",
        complete=LOC.fmt("#autoLOC_6002604"))
    add(cls="RepairPart", part="cupola", partId=55, vessel=1234, vesselName="Duna Base", body="Duna",
        title=LOC.fmt("#autoLOC_6002578", "Duna Base", cupola_title, BD_DISPLAY("Duna")), hash="cupola55", complete=LOC.fmt("#autoLOC_6002580"))
    for otype in ORBIT_TYPES_ALL:
        for p in range(3):
            for body in ("Kerbin", "Sun", "Mun"):
                add(cls="SpecificOrbit", orbitType=otype, prestige=p, body=body, title=orbit_title(otype, body, p))
    # SpecificOrbit notes (HighLogic.LoadedScene == SPACECENTER: a leading line break)
    for (sma, ecc, inc, lan, arg, body) in ((700000.0, 0.01, 28.61, 123.456, 77.77, "Kerbin"), (1200000.0, 0.3, 90.0, 200.04, 45.55, "Mun"),
                                           (1.5e7, 0.0, 0.5, 10.0, 10.0, "Sun"), (9e5, 0.1, 180.0, 0.0, 359.99, "Kerbin")):
        r = BD[body]["radius"]
        a = sma * (1.0 - ecc) - r
        a2 = sma * (1.0 + ecc) - r
        text = "\n"
        text += (LOC.fmt("#autoLOC_7003228") + ":\n" + LOC.fmt("#autoLOC_7003229", thousands(a2)) + "\n" + LOC.fmt("#autoLOC_7003230", thousands(a)) + "\n" +
                 LOC.fmt("#autoLOC_7003231", g15(round1(inc))) + "\n")
        if abs(inc) % 180.0 >= 1.0:
            text += "\n" + LOC.fmt("#autoLOC_7003232", g15(round1(lan)))
        if ecc > 0.05:
            text += "\n" + LOC.fmt("#autoLOC_7003233", g15(round1(arg)))
        add(cls="SpecificOrbitNotes", sma=sma, ecc=ecc, inc=inc, lan=lan, arg=arg, body=body, notes=text)
    add(cls="RoverWayPoint", title=LOC.fmt("#autoLOC_6002560"), complete=LOC.fmt("#autoLOC_6002561"))
    return out


def dec_hash(x):
    """Convert.ToDecimal(x).ToString("#,###.#") (15 significant digits, one decimal, half away from zero, '#' prints no zero)"""
    from decimal import Decimal, ROUND_HALF_UP
    d = Decimal("%.14e" % abs(x))
    d = d.quantize(Decimal("0.1"), rounding=ROUND_HALF_UP)
    ip = int(d)
    frac = int((d - ip) * 10)
    s = "{:,}".format(ip) if ip else ""
    if frac:
        s += "." + str(frac)
    return ("-" if x < 0 and s else "") + s


def cardinal(la1, lo1, la2, lo2):
    k = math.pi / 180.0
    la1, lo1, la2, lo2 = la1 * k, lo1 * k, la2 * k, lo2 * k
    d = lo2 - lo1
    y = math.sin(d) * math.cos(la2)
    x = math.cos(la1) * math.sin(la2) - math.sin(la1) * math.cos(la2) * math.cos(d)
    i = int(np.rint((math.atan2(y, x) * (180.0 / math.pi) + 360.0) % 360.0 / 45.0))
    tags = {1: "#autoLOC_290048", 2: "#autoLOC_290050", 3: "#autoLOC_290052", 4: "#autoLOC_290054", 5: "#autoLOC_290056", 6: "#autoLOC_290058", 7: "#autoLOC_290060"}
    return LOC.fmt(tags.get(i, "#autoLOC_290062"))


def string_utils():
    out = {}
    out["titleCase"] = [{"in": t, "out": title_case(t)} for t in ("molniya", "tundra orbit", "HELLO world", "a-b c", "")]
    out["thisThisAndThat"] = [{"in": l, "out": this_this_and_that(l)} for l in ([], ["a"], ["a", "b"], ["a", "b", "c"], ["a", "b", "c", "d"])]
    ps = []
    tbl = {("NEUTRAL", "SUBJECTIVE"): "they", ("NEUTRAL", "OBJECTIVE"): "them", ("NEUTRAL", "POSSESSIVE"): "their", ("FEMININE", "SUBJECTIVE"): "she",
           ("FEMININE", "OBJECTIVE"): "her", ("FEMININE", "POSSESSIVE"): "her", ("MASCULINE", "SUBJECTIVE"): "he", ("MASCULINE", "OBJECTIVE"): "him",
           ("MASCULINE", "POSSESSIVE"): "his"}
    for casing in ("SUBJECTIVE", "OBJECTIVE", "POSSESSIVE"):
        for gender in ("NEUTRAL", "MASCULINE", "FEMININE"):
            for post in (False, True):
                t = tbl[(gender, casing)]
                if casing == "POSSESSIVE" and post and t[-1] != "s":
                    t += "s"
                ps.append({"casing": casing, "gender": gender, "post": post, "out": t})
    out["pronouns"] = ps
    vu = []
    for tt in ("ALTITUDE", "DEPTH", "DISTANCE", "SPEED", "NULL_"):
        for v in (0.0, 0.5, 123.45, 9999.94, 10000.0, 1234567.89, 15.0):
            num = v
            if tt == "SPEED":
                unit = LOC.fmt("#autoLOC_289933")
            elif tt in ("ALTITUDE", "DEPTH", "DISTANCE"):
                if num >= 10000.0:
                    num /= 1000.0
                    unit = LOC.fmt("#autoLOC_289926")
                else:
                    unit = LOC.fmt("#autoLOC_289929")
            else:
                unit = LOC.fmt("#autoLOC_289916")
            vu.append({"type": tt, "value": v, "out": dec_hash(num) + unit})
    out["valueAndUnits"] = vu
    card = []
    for (la1, lo1, la2, lo2) in ((0, 0, 10, 0), (0, 0, 0, 10), (0, 0, -10, 0), (0, 0, 0, -10), (0, 0, 10, 10), (0, 0, -10, -10), (0, 0, 10, -10), (0, 0, -10, 10), (5, 5, 5, 5)):
        card.append({"args": [la1, lo1, la2, lo2], "out": cardinal(la1, lo1, la2, lo2)})
    out["cardinal"] = card
    out["isVowel"] = [{"c": c, "out": c.lower() in "aeiouy"} for c in "aEiOuYbZx"]
    out["readDuration"] = [{"in": t, "out": F(F(F(len(t) / 5.0) / 200.0) * 60.0)} for t in ("", "hello", "x" * 117)]
    return out


GENS = {"ARMContract": gen_arm, "BaseContract": gen_base, "StationContract": gen_station, "ISRUContract": gen_isru, "SatelliteContract": gen_satellite}


def main():
    ref = {"provenance": "tools/wp38b/fineprint2_reference.py (independent of kspcore): mirror cfgs of core/testdata/GameData-mirror, golden body table, decompiled FinePrint C#"}
    ref["mult_sets"] = w37.MULT_SETS
    ref["weights"] = weights()
    ref["scenarios"] = SCENARIOS
    ref["fixtures"] = [{"name": f.name, "objectives": sorted(f.objectives), "modules": sorted(f.modules), "probe": f.probe} for f in FIXTURES]
    ref["randomSeedFixture"] = RANDOM_SEED_FIXTURE
    ref["greek"] = GREEK_FIX
    ref["siteFixture"] = SITE_FIX
    W = weights()
    out = []
    for ctype, fn in GENS.items():
        for si, scn in enumerate(SCENARIOS):
            s = w38.Scn(scn["bodies"], scn["top"])
            for fi, fx in enumerate(FIXTURES):
                if fi == 2 and si not in (0, 8, 9):
                    continue
                for p in range(3):
                    for seed in range(1, 11):
                        for mi, mult in enumerate(w37.MULT_SETS):
                            if mi == 1 and seed > 4:
                                continue
                            sd = seed * 7001 + 13 + p
                            g = bytes(((b + seed * 3 + p) & 0xFF) for b in GUID)
                            r = fn(sd, p, s, W, mult, g, fx)
                            out.append({"type": ctype, "scenario": si, "fixture": fi, "prestige": p, "seed": sd, "mult": mi, "guid": list(g), "result": r})
    ref["contracts"] = out
    ref["paramTexts"] = param_texts()
    ref["stringUtils"] = string_utils()
    cnt = {}
    for e in out:
        k = e["type"]
        cnt.setdefault(k, [0, 0])
        cnt[k][0 if e["result"] else 1] += 1
    print("contracts", len(out), cnt)
    ref["alphaNumeric"] = [{"seed": sd, "value": alpha_numeric(sd)} for sd in (1, 7, 123, 99999, -4)]
    json.dump(ref, open(OUT, "w", encoding="utf-8"), indent=1)
    print("wrote", OUT)


if __name__ == "__main__":
    main()
