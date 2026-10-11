#!/usr/bin/env python3
"""WP-36 independent reference: GameVariables formulas, the strategy effects on the currency modifier pipeline, the Funding /
Reputation transactions, the CurrencyExchanger totals, the facility level maths and the vessel recovery value.

No kspcore code. Inputs are the game's own data: the strategy / department cfgs of core/testdata/GameData-mirror, the Kerbin radius
of harness/golden/serialized/celestial_bodies.serialized.json (WP-09 catalog source) and the KSC PQSCity position of
core/data/terrain/pqs_prefab.json. C# `float` arithmetic follows the Mono rule (core/README.md): an expression is evaluated in double
and rounded to float32 (numpy) only where the C# stores, passes, returns or casts it. Python floats below always hold float32-exact
values for C# floats.

Fixtures that are NOT KSP data (they only exercise the code paths; the real values need the Unity prefab / scene assets, see
STATUS.md "needs data"): the two constant reputation curves (reputationAddition / reputationSubtraction), the facility level costs.

Usage: python -I core/tools/wp36/currency_reference.py            -> core/tests/data/wp36_reference.json
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
STRAT_DIR = os.path.join(CORE, "testdata", "GameData-mirror", "GameData", "Squad", "Strategies")
BODIES = os.path.join(ROOT, "harness", "golden", "serialized", "celestial_bodies.serialized.json")
PQS = os.path.join(CORE, "data", "terrain", "pqs_prefab.json")
OUT = os.path.join(CORE, "tests", "data", "wp36_reference.json")

f32 = np.float32


def F(x):
    """round a C# float expression (evaluated in double) to float32 at a store/argument/return boundary"""
    return float(f32(x))


def clamp01(v):
    return 0.0 if v < 0.0 else (1.0 if v > 1.0 else v)


def lerp(a, b, t):  # Mathf.Lerp
    return F(a + (b - a) * clamp01(t))


def nextafter32(x, direction):
    return float(np.nextafter(f32(x), f32(direction)))


# ---- TransactionReasons (TransactionReasons.cs) -------------------------------------------------------------------------------
REASONS = {
    "None": 0, "ContractAdvance": 2, "ContractReward": 4, "ContractPenalty": 8, "Contracts": 0x4000E, "VesselRollout": 0x10,
    "VesselRecovery": 0x20, "VesselLoss": 0x40, "Vessels": 0x70, "StrategyInput": 0x80, "StrategyOutput": 0x100, "StrategySetup": 0x200,
    "Strategies": 0x380, "ScienceTransmission": 0x400, "StructureRepair": 0x800, "StructureCollapse": 0x1000,
    "StructureConstruction": 0x2000, "Structures": 0x3800, "RnDTechResearch": 0x4000, "RnDPartPurchase": 0x8000, "RnDs": 0xC000,
    "Cheating": 0x10000, "CrewRecruited": 0x20000, "ContractDecline": 0x40000, "Progression": 0x80000, "Mission": 0x100000, "Any": -1,
}
CURRENCY = {"Funds": 0, "Science": 1, "Reputation": 2}


# ---- a minimal ConfigNode reader (// comments, NAME { key = value }) ------------------------------------------------------------
def parse_cfg(path):
    text = open(path, encoding="utf-8-sig").read()
    root = {"name": "", "values": [], "nodes": []}
    stack = [root]
    pending = None
    for raw in text.splitlines():
        line = raw.split("//")[0].strip() if "//" in raw else raw.strip()
        if not line:
            continue
        if line == "{":
            n = {"name": pending, "values": [], "nodes": []}
            stack[-1]["nodes"].append(n)
            stack.append(n)
            pending = None
        elif line == "}":
            stack.pop()
        elif "=" in line:
            k, v = line.split("=", 1)
            stack[-1]["values"].append((k.strip(), v.strip()))
        else:
            pending = line
    return root


def val(node, key):
    for k, v in node["values"]:
        if k == key:
            return v
    return None


# ---- the strategy configs (StrategyConfig.Create) -------------------------------------------------------------------------------
def load_strategies():
    deps = [val(n, "name") for n in parse_cfg(os.path.join(STRAT_DIR, "Departments.cfg"))["nodes"] if n["name"] == "STRATEGY_DEPARTMENT"]
    out = []
    for n in parse_cfg(os.path.join(STRAT_DIR, "Strategies.cfg"))["nodes"]:
        if n["name"] != "STRATEGY":
            continue
        d = val(n, "department")
        if not d or d not in deps:
            continue
        s = {"name": val(n, "name"), "department": d, "groupTags": [t.strip() for t in (val(n, "groupTag") or "").split(",")] if val(n, "groupTag") else [],
             "icMin": [0.0] * 3, "icMax": [0.0] * 3, "reqMin": 0.0, "reqMax": 0.0, "hasSlider": False, "sliderDefault": 0.0, "sliderSteps": 0,
             "effects": []}
        for i, key in enumerate(("initialCostFunds", "initialCostScience", "initialCostReputation")):
            v = val(n, key)
            if v:
                s["icMin"][i] = s["icMax"][i] = F(float(v))
            if val(n, key + "Min") is not None:
                s["icMin"][i] = F(float(val(n, key + "Min")))
            if val(n, key + "Max") is not None:
                s["icMax"][i] = F(float(val(n, key + "Max")))
        if val(n, "requiredReputationMin") is not None:
            s["reqMin"] = F(float(val(n, "requiredReputationMin")))
        if val(n, "requiredReputationMax") is not None:
            s["reqMax"] = F(float(val(n, "requiredReputationMax")))
        hs = val(n, "hasFactorSlider")
        if hs:
            s["hasSlider"] = hs.lower() == "true"
        if val(n, "factorSliderDefault"):
            s["sliderDefault"] = F(float(val(n, "factorSliderDefault")))
        if val(n, "factorSliderSteps"):
            s["sliderSteps"] = int(val(n, "factorSliderSteps"))
        for e in n["nodes"]:
            if e["name"] != "EFFECT" or not val(e, "name"):
                continue
            eff = {"type": val(e, "name")}
            for key in ("minValue", "maxValue", "minRate", "maxRate", "minShare", "maxShare"):
                if val(e, key) is not None:
                    eff[key] = F(float(val(e, key)))
            for key in ("currency", "operation", "input", "output", "valueId"):
                if val(e, key) is not None:
                    eff[key] = val(e, key)
            reasons = 0
            if val(e, "AffectReasons"):
                for part in val(e, "AffectReasons").split(","):
                    reasons |= REASONS[part.strip()]
            eff["reasons"] = reasons
            s["effects"].append(eff)
        out.append(s)
    return out


class Query:
    """CurrencyModifierQuery: float deltas accumulate with a float store per AddDelta"""

    def __init__(self, reason, f0, s0, r0):
        self.reason = reason
        self.inp = [f0, s0, r0]
        self.delta = [0.0, 0.0, 0.0]

    def add(self, c, v):
        self.delta[c] = F(self.delta[c] + v)


def run_effect(eff, factor, q):
    """CurrencyOperation.OnEffectQuery / CurrencyConverter.OnEffectQuery (only these two react to a CurrencyModifierQuery)"""
    if eff["type"] == "CurrencyOperation":
        if (q.reason & eff["reasons"]) != 0:
            pl = lerp(eff.get("minValue", 0.0), eff.get("maxValue", 0.0), factor)
            c = CURRENCY[eff["currency"]]
            op = eff.get("operation", "Add")
            if op == "Add":
                q.add(c, pl)
            elif op == "Multiply":
                q.add(c, F(pl * q.inp[c] - q.inp[c]))
            else:
                q.add(c, F(pl - q.inp[c]))
    elif eff["type"] == "CurrencyConverter":
        if (q.reason & eff["reasons"]) != 0:
            ic, oc = CURRENCY[eff["input"]], CURRENCY[eff["output"]]
            share = lerp(eff.get("minShare", 0.0), eff.get("maxShare", 0.0), factor)
            rate = lerp(eff.get("minRate", 0.0), eff.get("maxRate", 0.0), factor)
            num = F(q.inp[ic] * share)
            q.add(ic, F(0.0 - num))
            q.add(oc, F(rate * num))


def value_effect(eff, factor, value_id, inp):
    """ValueModifier.OnValueModifierQuery: the delta added to a ValueModifierQuery"""
    if eff["type"] == "ValueModifier" and eff.get("valueId") == value_id:
        return F(inp * lerp(eff.get("minValue", 0.0), eff.get("maxValue", 0.0), factor) - inp)
    return None


# ---- sections -----------------------------------------------------------------------------------------------------------------------
def thresholds():
    t = [0.2, 0.25, 0.3, 0.33, 0.4, 0.5, 0.6, 0.66, 0.75, 0.8, 1.0]
    levels = {F(k / 40.0) for k in range(41)}
    for x in t:
        v = F(x)
        levels.update([v, nextafter32(v, 0.0), nextafter32(v, 2.0)])
    for n in (1, 2, 3):  # facility MaxLevel
        for i in range(n + 1):
            levels.add(F(float(i) / float(n)))  # UpgradeableFacility.GetNormLevel = (float)level / (float)MaxLevel
    return sorted(levels)


def gv_rows(levels):
    big = 3.4028234663852886e+38
    imax = 2147483647
    c = F  # every threshold is a C# float literal (0.2f ...): compare L with the float32 value of the literal
    rows = []
    for L in levels:
        r = {"level": L}
        r["partCount"] = 30 if L < c(0.2) else 110 if L < c(0.4) else 255 if L < c(0.6) else 450 if L < c(0.8) else imax
        r["fuelTransfer"] = L > c(0.2)
        r["agStock"] = [L > c(0.4), True]  # [ActionGroupsAlways false, true]
        r["agCustom"] = [L > c(0.6), True]
        r["craftMass"] = 18.0 if L < c(0.2) else 32.0 if L < c(0.4) else 140.0 if L < c(0.6) else 300.0 if L < c(0.8) else big
        r["craftSize"] = ([15.0, 20.0, 15.0] if L < c(0.2) else [20.0, 28.0, 20.0] if L < c(0.4) else [28.0, 36.0, 28.0] if L < c(0.6)
                          else [36.0, 50.0, 36.0] if L < c(0.8) else [big, big, big])
        r["strategyCount"] = 1 if L < c(0.25) else 2 if L < c(0.5) else 3 if L < c(0.75) else 5
        r["strategyRange"] = c(0.25) if L < c(0.3) else c(0.6) if L < c(0.6) else 1.0
        r["levelLimit"] = L
        r["contracts"] = 2 if L < c(0.25) else 4 if L < c(0.5) else 7 if L < c(0.75) else imax
        r["flightPlanning"] = L > c(0.4)
        r["crewLimit"] = 5 if L < c(0.25) else 8 if L < c(0.5) else 12 if L < c(0.75) else imax
        r["crewLevel"] = 1.0 if L < c(0.33) else 3.0 if L < c(0.66) else big
        r["eva"] = L > c(0.2)
        r["evaFlags"] = L >= c(0.4)
        r["evaClamber"] = L > c(0.6)
        base = 250000000000.0 if L > 0.500000001 else (2000000000.0 if not (L > 1e-09) else 50000000000.0)  # (double)level vs double literals
        r["dsnRange"] = [base * 1.0, base * float(f32(0.5)), base * float(f32(1.5))]  # DSNModifier = 1, 0.5f, 1.5f
        r["orbitDisplay"] = 2 if L < c(0.2) else 3
        r["trackedObjects"] = 0 if L < c(0.25) else 3 if L < c(0.5) else 8 if L < c(0.75) else imax
        r["patchesAhead"] = [0 if L < c(0.25) else 1 if L < c(0.5) else 2 if L < c(0.75) else 3]  # CONIC_PATCH_LIMIT = 3
        r["spaceObjectDiscovery"] = L > c(0.6)
        r["minTracked"] = 4 if L < c(0.2) else 3 if L < c(0.4) else 2 if L < c(0.6) else 1 if L < c(0.8) else 0
        r["maneuverTool"] = L > c(0.75)
        r["scienceCost"] = 100.0 if L < c(0.25) else 300.0 if L < c(0.5) else 500.0 if L < c(0.75) else big
        r["dataRatio"] = c(0.4) if L < c(0.25) else c(0.6) if L < c(0.5) else c(0.8) if L < c(0.75) else 1.0
        r["experimentLevel"] = L
        rows.append(r)
    return rows


def gv_scalars():
    out = {}
    # GetRecruitHireCost(currentActive, 2000f, ...) = Mathf.Round(base + 80000f * (Mathf.Log(0.25f * n + 1.12f) / 500f) * 500f) * FundsLossMultiplier
    mults = [F(x) for x in (0.5, 0.75, 1.0, 1.25, 1.5)]
    hire = []
    for n in range(0, 41):
        lg = F(math.log(F(0.25 * float(n) + F(1.12))))
        rounded = F(round(F(2000.0 + 80000.0 * (lg / 500.0) * 500.0)))
        hire.append({"active": n, "costs": [F(rounded * m) for m in mults]})
    out["hire"] = {"multipliers": mults, "rows": hire}
    # contract factors (prefab scalars = the C# initialisers 1 / 1.25 / 1.5)
    pf = [F(1.0), F(1.25), F(1.5)]
    cm = []
    for m in mults:
        for prestige in range(3):
            fac = pf[prestige]
            cm.append({"mult": m, "prestige": prestige, "factor": fac, "gain": F(fac * 1.0 * m), "loss": F(fac * 1.0 * m)})
    out["contractFactors"] = cm
    # mentality factors Lerp(1, prefab, f)
    ment = {"trivial": F(1.1), "significant": F(1.2), "exceptional": F(1.3)}
    out["mentality"] = [{"f": F(k / 10.0), "trivial": lerp(1.0, ment["trivial"], F(k / 10.0)), "significant": lerp(1.0, ment["significant"], F(k / 10.0)),
                         "exceptional": lerp(1.0, ment["exceptional"], F(k / 10.0))} for k in range(0, 11)]
    # ScoreFlightEnvelope
    env = []
    for alt in (0.0, 1000.0, 10000.0, 25000.0, 70000.0, 123456.7):
        for ae in (0.0, 500.0, 5000.0, 90000.0):
            for sp in (0.0, 100.0, 1000.0, 2400.5, 8000.0):
                for se in (0.0, 50.0, 800.0, 9000.0):
                    alt, ae, sp, se = F(alt), F(ae), F(sp), F(se)
                    a = b = 0.0
                    if ae != 0.0:
                        n3 = F(math.floor(F(alt / 10000.0 * 1000.0)) / 1000.0)
                        t = 0.0 if alt == 0.0 else (clamp01(F((ae - 0.0) / (alt - 0.0))))  # Mathf.InverseLerp(0, altitude, altEnvelope)
                        a = F(max(F(n3 - t), 0.0))
                    if se != 0.0:
                        n5 = F(math.floor(F(sp / 1000.0 * 100.0)) / 100.0)
                        t = 0.0 if sp == 0.0 else clamp01(F((se - 0.0) / (sp - 0.0)))
                        b = F(max(F(n5 - t), 0.0))
                    env.append({"alt": alt, "altEnv": ae, "speed": sp, "speedEnv": se, "score": F(a + b)})
    out["flightEnvelope"] = env
    # recovered value factors
    vals = [F(v) for v in (0.0, 1.0, 1234.5, 99999.99, 0.1)]
    out["recovered"] = [{"v": v, "part": F(v * F(0.9)), "resource": F(v * F(0.95))} for v in vals]
    return out


def strategies_section(strats):
    factors = [F(k / 20.0) for k in range(21)]
    qfactors = {F(0.0), F(0.05), F(0.25), F(0.5), F(0.75), 1.0}
    reasons = ["ContractReward", "ContractAdvance", "ContractPenalty", "Progression", "VesselRecovery", "VesselRollout", "RnDPartPurchase",
               "ScienceTransmission", "StructureRepair", "StructureConstruction", "CrewRecruited", "Contracts", "Any", "None"]
    inputs = [(0.0, 0.0, 0.0), (1000.0, 0.0, 0.0), (0.0, 10.0, 0.0), (0.0, 0.0, 12.5), (15000.0, 35.5, 7.25), (-2500.0, 0.0, 0.0), (333.33, 4.5, 1.5)]
    inputs = [tuple(F(x) for x in i) for i in inputs]
    rows = []
    for s in strats:
        srow = {"name": s["name"], "department": s["department"], "groupTags": s["groupTags"], "hasSlider": s["hasSlider"], "sliderDefault": s["sliderDefault"],
                "sliderSteps": s["sliderSteps"], "effectTypes": [e["type"] for e in s["effects"]], "factors": []}
        for f in factors:
            fr = {"factor": f, "cost": [lerp(s["icMin"][i], s["icMax"][i], f) for i in range(3)], "required": lerp(s["reqMin"], s["reqMax"], f), "queries": [],
                  "values": []}
            if f in qfactors and not any(e["type"] == "CurrencyExchanger" for e in s["effects"]):
                for rn in reasons:
                    for inp in inputs:
                        q = Query(REASONS[rn], *inp)
                        for e in reversed(s["effects"]):  # EventData.Fire calls the listeners from the last registered to the first
                            run_effect(e, f, q)
                        fr["queries"].append({"reason": rn, "input": list(inp), "delta": q.delta})
                for vid in ("RecoveryMinimumDelta", "RecoveryMaximumDelta", "Nothing"):
                    d = 0.0
                    for e in s["effects"]:
                        x = value_effect(e, f, vid, 1.0)
                        if x is not None:
                            d = F(d + x)
                    fr["values"].append({"valueId": vid, "input": 1.0, "delta": d})
            srow["factors"].append(fr)
        rows.append(srow)
    # all (non-exchanger) strategies active at once, in config order, at one common factor
    combined = []
    active = [s for s in strats if not any(e["type"] == "CurrencyExchanger" for e in s["effects"])]
    for f in (0.0, F(0.05), F(0.5), 1.0):
        crow = {"factor": f, "queries": []}
        for rn in reasons:
            for inp in inputs:
                q = Query(REASONS[rn], *inp)
                for s in reversed(active):  # reverse registration order (EventData.Fire)
                    for e in reversed(s["effects"]):
                        run_effect(e, f, q)
                crow["queries"].append({"reason": rn, "input": list(inp), "delta": q.delta})
        combined.append(crow)
    return {"reasons": reasons, "inputs": [list(i) for i in inputs], "factors": factors, "strategies": rows, "combined": combined,
            "activeNames": [s["name"] for s in active]}


CONST_ADD, CONST_SUB = F(0.9), F(1.3)  # fixture reputation curves (constant), not KSP data


def rep_granular(rep, value):
    """Reputation.addReputation_granular with constant curves: returns (new rep, applied sum)"""
    num = int(abs(value))
    d = F(1.0 * (1.0 if value >= 0.0 else -1.0))  # 1f * Mathf.Sign(value)

    def mod(delta):  # ModifyReputationDelta
        return F(delta * (CONST_SUB if delta < 0.0 else CONST_ADD))

    n2 = 0.0
    for i in range(num + 1):
        n3 = mod(F(value - n2)) if i == num else mod(d)
        rep = F(rep + n3)
        n2 = F(n2 + n3)
    return rep, n2


def exchangers(strats):
    """CurrencyExchanger.OnRegister: GetTotalInput / GetTotalOutput and the resulting Funding / Reputation / Science transactions"""
    out = []
    for s in strats:
        for e in s["effects"]:
            if e["type"] != "CurrencyExchanger":
                continue
            for f in (F(0.05), F(0.5), 1.0):
                for funds, sci, rep in ((25000.0, 120.5, 300.25), (1.0e6, 0.0, -200.0), (0.0, 55.0, 999.0)):
                    sci, rep = F(sci), F(rep)
                    share = lerp(e.get("minShare", 0.0), e.get("maxShare", 0.0), f)
                    rate = lerp(e.get("minRate", 0.0), e.get("maxRate", 0.0), f)
                    ic = CURRENCY[e["input"]]
                    tin = funds * share if ic == 0 else (sci * share if ic == 1 else (rep + 1000.0) * share)
                    tout = tin * rate
                    sci_adds = []
                    fu, re = funds, rep
                    # input transaction (reason StrategyInput), then output (StrategyOutput); AllowNegativeCurrency = false
                    if ic == 0:
                        fu = max(0.0, fu + (0.0 - tin))
                    elif ic == 1:
                        sci_adds.append(F(0.0 - tin))
                    else:
                        re, _ = rep_granular(re, F(0.0 - tin))
                    oc = CURRENCY[e["output"]]
                    if oc == 0:
                        fu = max(0.0, fu + tout)
                    elif oc == 1:
                        sci_adds.append(F(tout))
                    else:
                        re, _ = rep_granular(re, F(tout))
                    out.append({"strategy": s["name"], "factor": f, "input": e["input"], "output": e["output"], "funds": funds, "science": sci,
                                "reputation": rep, "totalInput": tin, "totalOutput": tout, "fundsAfter": fu, "repAfter": re, "scienceAdds": sci_adds})
    return out


def transactions(strats):
    """Funding.AddFunds / Reputation.AddReputation with the stock strategies active (constant reputation curves). The two
    OnCurrencyModified listeners (Funding.OnAwake, Reputation.Start) touch independent state, their order does not matter."""
    active = [s for s in strats if not any(e["type"] == "CurrencyExchanger" for e in s["effects"])]
    steps = [("f", 12000.0, "ContractReward"), ("f", -60000.0, "VesselRollout"), ("r", 7.5, "Progression"), ("f", 300.5, "Progression"),
             ("r", -20.25, "VesselLoss"), ("f", 5000.0, "VesselRecovery"), ("r", 33.0, "ContractReward"), ("f", -4000.0, "RnDPartPurchase"),
             ("f", 100000.0, "ContractAdvance"), ("r", -1200.0, "VesselLoss")]
    out = []
    for f in (0.0, F(0.05), F(0.5), 1.0):
        for allow_negative in (False, True):
            funds, rep = 25000.0, 0.0
            seq = []
            for kind, amount, rn in steps:
                reason = REASONS[rn]
                if kind == "f":
                    funds += amount
                    q = Query(reason, F(amount), 0.0, 0.0)
                else:
                    rep, _ = rep_granular(rep, F(amount))
                    q = Query(reason, 0.0, 0.0, F(amount))
                for s in reversed(active):  # reverse registration order (EventData.Fire)
                    for e in reversed(s["effects"]):
                        run_effect(e, f, q)
                funds += q.delta[0]  # Funding.OnCurrenciesModified
                rep, _ = rep_granular(rep, q.delta[2])  # Reputation.OnCurrenciesModified
                if kind == "f" and not allow_negative:
                    funds = max(0.0, funds)  # Funding.AddFunds
                seq.append({"funds": funds, "rep": rep})
            out.append({"factor": f, "allowNegative": allow_negative, "steps": [{"kind": k, "amount": F(a), "reason": r} for k, a, r in steps], "after": seq,
                        "addCurve": CONST_ADD, "subCurve": CONST_SUB})
    return out


def facility():
    costs = [F(1000.5), F(2500.25), F(7000.125), F(12000.0625)]  # fixture, not KSP values
    out = []
    for max_level in (1, 2, 3):
        for lvl in [F(k / 16.0) for k in range(-1, 18)] + [F(0.5), F(0.3333), F(0.6667), F(0.999999)]:
            l = clamp01(lvl)
            fl = int(math.floor(F(l * F(float(max_level + 1)))))
            fl = max(0, min(max_level, fl))
            out.append({"maxLevel": max_level, "norm": lvl, "level": fl, "normBack": F(float(fl) / float(max_level))})
    mults = [F(1.0), F(0.5), F(1.5), F(2.0)]
    price = []
    for max_level in (1, 2, 3):
        for level in range(max_level + 1):
            for m in mults:
                up_game = F(costs[level + 1] * m) if level != max_level else 0.0
                down_game = F(costs[level - 1] * F(0.667) * m) if level != 0 else 0.0
                up_free = costs[level + 1] if level != max_level else 0.0
                down_free = F(costs[level - 1] * F(0.667)) if level != 0 else 0.0
                price.append({"maxLevel": max_level, "level": level, "mult": m, "upGame": up_game, "downGame": down_game, "upFree": up_free,
                              "downFree": down_free})
    return {"levelCosts": costs, "rows": out, "prices": price, "lossMultipliers": mults}


def recovery(strats):
    with open(BODIES) as fh:
        bodies = json.load(fh)["bodies"]
    kerbin = next(b["celestialBody"] for b in bodies if b["celestialBody"].get("bodyName") == "Kerbin")
    radius = float(kerbin["Radius"])
    pq = json.load(open(PQS))

    def find(o):
        if isinstance(o, dict):
            if o.get("gameObject") == "Kerbin/KSC" and o.get("class") == "PQSCity":
                return o["fields"]
            for v in o.values():
                r = find(v)
                if r:
                    return r
        elif isinstance(o, list):
            for v in o:
                r = find(v)
                if r:
                    return r
        return None

    # WP-42: the KSC position as SpaceCenter.Start derives it: PQSCity.Orientate (Vector3.normalized of repositionRadial in float32, times
    # radius + repositionRadiusOffset, stored as the float32 transform.localPosition), then CelestialBody.GetLatitudeAndLongitude (identity
    # frame) and GetRelSurfaceNVector(lat, lon) (the srfNVector GreatCircleDistance uses)
    fields = find(pq["bodies"])
    rr = [F(fields["repositionRadial"][k]) for k in "xyz"]
    num = F(math.sqrt(rr[0] * rr[0] + rr[1] * rr[1] + rr[2] * rr[2]))
    unit = [F(c / num) for c in rr]
    local = [F(c * (radius + fields["repositionRadiusOffset"])) for c in unit]
    mag = math.sqrt(local[0] * local[0] + local[1] * local[1] + local[2] * local[2])
    nrm = [c / mag for c in local]
    sc_lat = math.asin(nrm[1]) * 57.295780181884766  # (x, z, y) swizzle: rPos.z is the local y
    sc_lon = math.atan2(nrm[2], nrm[0]) * 57.295780181884766

    def nvec(lat, lon):  # CelestialBody.GetRelSurfaceNVector
        la, lo = lat * (math.pi / 180.0), lon * (math.pi / 180.0)
        x, y, z = math.cos(la) * math.cos(lo), math.cos(la) * math.sin(lo), math.sin(la)
        return [x, z, y]

    ksc = nvec(sc_lat, sc_lon)

    transponder = next(s for s in strats if s["name"] == "RecoveryTransponders")
    deltas = [(0.0, 0.0)]
    for f in (F(0.05), F(0.5), 1.0):
        mn = value_effect(transponder["effects"][0], f, "RecoveryMinimumDelta", 1.0)
        mx = value_effect(transponder["effects"][1], f, "RecoveryMaximumDelta", 1.0)
        deltas.append((mn, mx))
    lands = ["", "KSC", "Runway", "LaunchPad", "KSC_Runway", "Mun_Crater", "Island_Airfield"]
    rows = []
    pts = [(-0.0972, -74.5576)]
    for lat in (-60.0, -30.0, -5.0, 0.0, 12.5, 45.0, 80.0):
        for lon in (-170.0, -120.0, -74.0, -10.0, 33.3, 90.0, 140.0):
            pts.append((lat, lon))
    for lat, lon in pts:
        v = nvec(lat, lon)
        dot = ksc[0] * v[0] + ksc[1] * v[1] + ksc[2] * v[2]
        if not (-1.0 <= dot <= 1.0):
            continue
        dist = radius * math.acos(dot)
        num2 = radius * math.pi
        frac = F(dist / num2)
        for mn, mx in deltas:
            for landed in lands:
                b = F(max(0.0, min(1.0, F(F(0.1) + mn))))
                a = F(max(0.0, min(1.0, F(F(0.98) + mx))))
                num3 = lerp(a, b, frac)
                num4 = lerp(F(0.98), F(0.1), frac)
                if "KSC" in landed:
                    num3 = num4 = F(0.98)
                if "Runway" in landed or "LaunchPad" in landed:
                    num3 = num4 = 1.0
                rows.append({"lat": lat, "lon": lon, "distance": dist, "minDelta": mn, "maxDelta": mx, "landedAt": landed, "num3": num3, "num4": num4})
    return {"radius": radius, "ksc": ksc, "rows": rows}


def main():
    strats = load_strategies()
    levels = thresholds()
    doc = {"provenance": {"strategies": "core/testdata/GameData-mirror/GameData/Squad/Strategies/{Strategies,Departments}.cfg",
                          "kerbinRadius": "harness/golden/serialized/celestial_bodies.serialized.json", "ksc": "core/data/terrain/pqs_prefab.json Kerbin/KSC PQSCity",
                          "fixtures": "constant reputation curves 0.9 / 1.3 and facility level costs are test fixtures, not KSP values"},
           "gv": {"levels": levels, "rows": gv_rows(levels), "scalars": gv_scalars()},
           "strategies": strategies_section(strats), "exchangers": exchangers(strats), "transactions": transactions(strats), "facility": facility(),
           "recovery": recovery(strats)}
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w") as fh:
        json.dump(doc, fh, indent=None, separators=(",", ":"))
    print("wrote", OUT, os.path.getsize(OUT), "bytes;", len(strats), "strategies")


if __name__ == "__main__":
    sys.exit(main())
