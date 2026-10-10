#!/usr/bin/env python3
"""WP-35: independent reference for the science maths -- test data generator.

Re-implements, in plain Python (numpy float32 stores per the Mono float rule: a C# float expression is evaluated in double and rounded
only where the source stores, passes, returns or casts it to float) and from the decompiled C#:
  ScienceSubject(exp, sit, body, biome) id / subjectValue / scienceCap       (ScienceSubject.cs:40)
  ResearchAndDevelopment.GetReferenceDataValue / GetSubjectValue / GetScienceValue / GetNextScienceValue / SubmitScienceData
                                                                             (ResearchAndDevelopment.cs:47, :49, :656, :664, :698)
  ExperimentResultDialogPage.UpdatePageLabValue (SurfaceBonus / ContextBonus / homeworldMultiplier, Math.Round)   (ExperimentResultDialogPage.cs:148)
  ModuleScienceConverter.GetScientists / CalculateScienceAmount / CalculateScienceRate / CalculateResearchTime / PostProcess
                                                                             (ModuleScienceConverter.cs:164, :63, :68, :74, :118)
  BaseConverter.FixedUpdate catch-up (GetDeltaTime / GetBestDeltaTime, :258, :292, :500) + ResourceConverter.ProcessRecipe (:22)
  for the science converter's one-input (ElectricCharge) recipe on a simple tank store.
It shares NO code with kspcore. Inputs are the game's own data only:
  GameData-mirror/GameData/Squad/Resources/ScienceDefs.cfg       (every EXPERIMENT_DEFINITION)
  GameData-mirror/GameData/Squad/Parts/Science/LargeCrewedLab/largeCrewedLab.cfg   (ModuleScienceLab / ModuleScienceConverter values)
  harness/golden/serialized/celestial_bodies.serialized.json     (CelestialBodyScienceParams, atmosphere, isHomeWorld of the 17 bodies)
  core/src/celestial/kerbol_system_data.cpp                      (the biome attribute names of each body, from the PSystem prefab)

Usage: python -I tools/wp35/science_reference.py > tests/data/wp35_reference.json   (run in core/)
"""
import json
import math
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
REPO = os.path.abspath(os.path.join(CORE, ".."))


def f32(x):
    return float(np.float32(x))


def bits(x):
    return int(np.array(x, dtype=np.float32).view(np.uint32))


# ------------------------------------------------------------------------------------------------ cfg parsing (own, minimal)
class Node:
    def __init__(self, name):
        self.name = name
        self.values = []  # ordered (key, value)
        self.children = []

    def get(self, key, default=None):
        for k, v in self.values:
            if k == key:
                return v
        return default


def parse_cfg(text):
    text = text.lstrip("﻿")
    root = Node("")
    stack = [root]
    pending = None
    for raw in text.splitlines():
        line = raw.split("//")[0].strip()
        if not line:
            continue
        if line == "{":
            n = Node(pending)
            stack[-1].children.append(n)
            stack.append(n)
            pending = None
        elif line == "}":
            stack.pop()
        elif "=" in line:
            k, v = line.split("=", 1)
            stack[-1].values.append((k.strip(), v.strip()))
        else:
            pending = line
    return root


def find_nodes(node, name):
    out = []
    for c in node.children:
        if c.name == name:
            out.append(c)
        out.extend(find_nodes(c, name))
    return out


def parse_bool(s):
    return s.strip().lower() == "true"


# ------------------------------------------------------------------------------------------------ game data
def load_experiments():
    path = os.path.join(CORE, "testdata", "GameData-mirror", "GameData", "Squad", "Resources", "ScienceDefs.cfg")
    root = parse_cfg(open(path, encoding="utf-8").read())
    exps = []
    for n in find_nodes(root, "EXPERIMENT_DEFINITION"):
        e = {"id": n.get("id"), "title": n.get("title")}
        e["baseValue"] = f32(float(n.get("baseValue", "0")))
        e["scienceCap"] = f32(float(n.get("scienceCap", "0")))
        e["situationMask"] = int(n.get("situationMask", "0"))
        e["biomeMask"] = int(n.get("biomeMask", "0"))
        e["dataScale"] = f32(float(n.get("dataScale", "0")))
        e["requireAtmosphere"] = parse_bool(n.get("requireAtmosphere", "False"))
        e["requireNoAtmosphere"] = parse_bool(n.get("requireNoAtmosphere", "False"))
        e["requiredExperimentLevel"] = f32(float(n.get("requiredExperimentLevel", "0")))
        e["applyScienceScale"] = parse_bool(n.get("applyScienceScale", "True"))
        res = []
        names = set()
        for r in find_nodes(n, "RESULTS"):
            for k, v in r.values:
                while k in names:
                    k += "*"
                names.add(k)
                res.append([k, v])
        e["results"] = res
        exps.append(e)
    return exps


def load_bodies():
    d = json.load(open(os.path.join(REPO, "harness", "golden", "serialized", "celestial_bodies.serialized.json")))
    src = open(os.path.join(CORE, "src", "celestial", "kerbol_system_data.cpp"), encoding="utf-8").read()
    # biome attribute names per body: between `b.bodyName = "X"` and the next one
    marks = [(m.start(), m.group(1)) for m in re.finditer(r'b\.bodyName = "([^"]+)"', src)]
    biomes = {}
    for i, (pos, name) in enumerate(marks):
        end = marks[i + 1][0] if i + 1 < len(marks) else len(src)
        biomes[name] = re.findall(r'm\.Attributes\.push_back\(\{Color\{[^}]*\}, "([^"]+)", "#autoLOC', src[pos:end])
    bodies = []
    for b in d["bodies"]:
        cb = b["celestialBody"]
        sv = cb["scienceValues"]
        bodies.append({
            "name": cb["bodyName"],
            "atmosphere": bool(cb["atmosphere"]),
            "isHomeWorld": bool(cb["isHomeWorld"]),
            "sv": {k: f32(v) for k, v in sv.items()},
            "biomes": biomes.get(cb["bodyName"], []),
        })
    return bodies


def load_lab():
    path = os.path.join(CORE, "testdata", "GameData-mirror", "GameData", "Squad", "Parts", "Science", "LargeCrewedLab", "largeCrewedLab.cfg")
    root = parse_cfg(open(path, encoding="utf-8").read())
    lab = conv = None
    for m in find_nodes(root, "MODULE"):
        if m.get("name") == "ModuleScienceLab":
            lab = m
        elif m.get("name") == "ModuleScienceConverter":
            conv = m
    return {
        "crewsRequired": f32(float(lab.get("crewsRequired", "0"))),
        "SurfaceBonus": f32(float(lab.get("SurfaceBonus", "0.1"))),
        "ContextBonus": f32(float(lab.get("ContextBonus", "0.25"))),
        "homeworldMultiplier": f32(float(lab.get("homeworldMultiplier", "0.5"))),
        "dataStorage": f32(float(lab.get("dataStorage", "500"))),
        "dataProcessingMultiplier": f32(float(conv.get("dataProcessingMultiplier", "0.5"))),
        "scientistBonus": f32(float(conv.get("scientistBonus", "0.25"))),
        "researchTime": int(conv.get("researchTime", "8")),
        "scienceMultiplier": int(conv.get("scienceMultiplier", "5")),
        "scienceCap": int(conv.get("scienceCap", "500")),
        "powerRequirement": f32(float(conv.get("powerRequirement", "5"))),
    }


SITS = [("SrfLanded", 1, "LandedDataValue"), ("SrfSplashed", 2, "SplashedDataValue"), ("FlyingLow", 4, "FlyingLowDataValue"),
        ("FlyingHigh", 8, "FlyingHighDataValue"), ("InSpaceLow", 16, "InSpaceLowDataValue"), ("InSpaceHigh", 32, "InSpaceHighDataValue")]


# ------------------------------------------------------------------------------------------------ R&D maths (ResearchAndDevelopment.cs)
def clamp01(t):
    return 0.0 if t < 0.0 else (1.0 if t > 1.0 else t)


def lerp(a, b, t):  # Mathf.Lerp: float result of a + (b - a) * Clamp01(t)
    return f32(a + (b - a) * clamp01(t))


def ref_value(data, subj):  # :47
    return f32(data / subj["dataScale"] * subj["subjectValue"])


def subj_value(science, subj):  # :49
    return max(0.0, f32(1.0 - science / subj["scienceCap"]))


def sci_value(data, ratio, subj, x):  # :656
    rdv = ref_value(data, subj)
    num = min(f32(rdv * subj["scientificValue"] * x), f32(subj["scienceCap"] * ratio))
    b = f32(lerp(rdv, f32(subj["scienceCap"] * ratio), x) * x)
    return max(f32(min(f32(subj["science"] + num), b) - subj["science"]), 0.0)


def next_value(data, subj, x):  # :664
    num = f32(subj["science"] + sci_value(data, 1.0, subj, x))
    num2 = subj["scientificValue"]
    if subj["applyScienceScale"]:
        num2 = subj_value(num, subj)
    num3 = min(f32(ref_value(data, subj) * num2 * x), subj["scienceCap"])
    b = f32(lerp(ref_value(data, subj), subj["scienceCap"], x) * x)
    return max(f32(min(f32(num + num3), b) - num), 0.0)


def submit(data, ratio, subj, x, mult):  # :698
    v = sci_value(data, ratio, subj, x)
    subj["science"] = f32(subj["science"] + v)
    if subj["applyScienceScale"]:
        subj["scientificValue"] = subj_value(subj["science"], subj)
    return f32(v * mult)


SCHEDULE = [(1.0, 1.0), (0.5, 1.0), (0.5, 0.5), (1.0, 1.0), (0.25, 1.0), (1.0, 0.30000001192092896), (0.0, 1.0)]


def subject_cases(exps, bodies):
    cases = []
    for e in exps:
        for b in bodies:
            for sname, smask, svk in SITS:
                if not (e["situationMask"] & smask):
                    continue
                if e["requireAtmosphere"] and not b["atmosphere"]:
                    continue
                if e["requireNoAtmosphere"] and b["atmosphere"]:
                    continue
                biome_list = [""]
                if e["biomeMask"] & smask and b["biomes"]:
                    names = b["biomes"]
                    spaced = [n for n in names if " " in n]
                    biome_list = [names[0]] + [n for n in spaced if n != names[0]][:1]
                for biome in biome_list:
                    sv = b["sv"][svk]
                    subj = {
                        "subjectValue": sv,
                        "dataScale": e["dataScale"],
                        "scienceCap": f32(e["scienceCap"] * sv),
                        "applyScienceScale": e["applyScienceScale"],
                        "science": 0.0,
                        "scientificValue": 1.0,
                    }
                    case = {
                        "exp": e["id"], "body": b["name"], "sit": sname, "biome": biome,
                        "id": e["id"] + "@" + b["name"] + sname + biome.replace(" ", ""),
                        "subjectValue": bits(sv), "scienceCap": bits(subj["scienceCap"]), "dataScale": bits(e["dataScale"]),
                        "data": f32(e["baseValue"] * e["dataScale"]),
                        "steps": [],
                    }
                    data = case["data"]
                    case["data"] = bits(data)
                    for x, ratio in SCHEDULE:
                        x = f32(x)
                        ratio = f32(ratio)
                        st = {
                            "x": bits(x), "ratio": bits(ratio),
                            "refValue": bits(ref_value(data, subj)),
                            "scienceValue": bits(sci_value(data, ratio, subj, x)),
                            "nextValue": bits(next_value(data, subj, x)),
                        }
                        got = submit(data, ratio, subj, x, f32(1.0))
                        st["submitted"] = bits(got)
                        st["science"] = bits(subj["science"])
                        st["scientificValue"] = bits(subj["scientificValue"])
                        case["steps"].append(st)
                    cases.append(case)
    return cases


# ------------------------------------------------------------------------------------------------ ExperimentResultDialogPage.UpdatePageLabValue
def lab_value(ref, landed, splashed, context, home, lab):
    v = f32(ref)
    if landed:
        v = f32(v * (1.0 + lab["SurfaceBonus"]))
    if context:
        v = f32(v * (1.0 + lab["ContextBonus"]))
    if (landed or splashed) and home:
        v = f32(v * lab["homeworldMultiplier"])
    return f32(round(v))  # Math.Round(double): banker's rounding (Python round on a float is round-half-even)


def lab_cases(sub_cases, lab):
    out = []
    refs = []
    seen = set()
    for c in sub_cases:
        r = c["steps"][0]["refValue"]
        if r not in seen:
            seen.add(r)
            refs.append(r)
    refs = refs[:60]
    refs += [bits(0.5), bits(1.5), bits(2.5), bits(3.5), bits(0.49999999), bits(100.5)]
    for rb in refs:
        r = float(np.array(rb, dtype=np.uint32).view(np.float32))
        for landed in (False, True):
            for splashed in (False, True):
                if landed and splashed:
                    continue
                for context in (False, True):
                    for home in (False, True):
                        out.append({"ref": rb, "landed": landed, "splashed": splashed, "context": context, "home": home,
                                    "labValue": bits(lab_value(r, landed, splashed, context, home, lab))})
    return out


# ------------------------------------------------------------------------------------------------ ModuleScienceConverter
def scientists_of(levels, lab):  # :164 (every crew member of the list has ScienceSkill)
    num = 0.0
    for lvl in levels:
        num = f32(num + (1.0 + lab["scientistBonus"] * float(lvl)))
    return num


def science_amount(data, lab):  # :63 (double product of two floats)
    return data * float(lab["scienceMultiplier"])


def science_rate(data, levels, lab, day):  # :68
    sc = scientists_of(levels, lab)
    num = float(day) * sc * data * lab["dataProcessingMultiplier"] * float(lab["scienceMultiplier"])
    return num / (10.0 ** lab["researchTime"] if lab["researchTime"] >= 0 else math.pow(10.0, lab["researchTime"]))


def research_time(data, stored, levels, lab, day):  # :74
    num = science_rate(f32(stored + data), levels, lab, day)
    return float(day) * (data / num) if num != 0 else (math.inf if data > 0 else math.nan)


def converter_formula_cases(lab):
    out = []
    crews = [[], [0], [1], [3], [5], [0, 0], [1, 2], [3, 3, 5], [5, 5, 5, 5]]
    for day in (21600, 86400):
        for levels in crews:
            for data in (0.5, 1.0, 12.25, 100.0, 333.3):
                for stored in (0.0, 7.5, 250.0):
                    d = f32(data)
                    s = f32(stored)
                    sc = scientists_of(levels, lab)
                    rate = science_rate(d, levels, lab, day)
                    rt = research_time(d, s, levels, lab, day) if sc > 0 else None
                    out.append({"day": day, "levels": levels, "data": bits(d), "stored": bits(s), "scientists": bits(sc),
                                "amount": science_amount(d, lab), "rate": rate,
                                "researchTime": rt if rt is None or math.isfinite(rt) else None})
    return out


def run_converter(sc, lab):
    """BaseConverter.FixedUpdate (:500) for ModuleScienceConverter on one electric charge tank."""
    ec = sc["ec"]
    stored = f32(sc["dataStored"])
    sci = f32(sc["storedScience"])
    storage = lab["dataStorage"]
    cap = float(lab["scienceCap"])
    levels = sc["levels"]
    last_update = 0.0
    retries = 0
    last_tf = 0.0
    activated = True
    max_delta = float(sc["maxDeltaTime"])
    fixed = f32(0.02)
    steps = []
    ratio = lab["powerRequirement"]
    for ut in sc["uts"]:
        status = -1
        # GetDeltaTime (:258): Time.timeSinceLevelLoad >= 1 and FlightGlobals.ready
        if abs(last_update) < 1e-9:
            last_update = ut
            dt = -1.0
        else:
            num = min(ut - last_update, max_delta)
            num2 = min(fixed, 0.02)
            if num > num2 and retries < 10:
                # GetBestDeltaTime (:292): PrepareRecipe gives the one electric input
                best = num
                n3 = num * (ec / (ratio * num))
                if n3 < best:
                    best = n3
                if best < num:
                    retries += 1
                    num = max(num2, best)
            last_update += num
            dt = num
        did_run = not (dt < 1e-9)
        if did_run:
            if activated:
                eff = 1.0  # EfficiencyBonus * heat * total * crew
                e = float(np.float32(eff))
                # ProcessRecipe (ResourceConverter.cs:22), no requirements, one input, no outputs
                num_ = 1.0
                num_ *= e
                tf = None
                if num_ <= 1e-9:
                    tf = 0.0
                else:
                    num4 = dt
                    num5 = ec * 1.0
                    num6 = ratio * num4 * num_
                    if num5 < num6:
                        num4 *= num5 / num6
                        if not (num4 > 1e-9):
                            tf = 0.0
                    if tf is None:
                        take = ratio * num4 * num_
                        t = min(take, ec) if take > 0 else 0.0
                        ec -= t
                        tf = num4 * num_
                # PostProcess (:118)
                if float(stored) < 1e-9:
                    stored = 0.0
                if stored > storage:
                    stored = storage
                if sci > cap:
                    sci = cap
                scientists = scientists_of(levels, lab)
                if float(scientists) < 1e-9:
                    status = 0
                    activated = False  # StopResourceConverter
                else:
                    n = tf / dt
                    if n < 1e-9:
                        status = 1
                    else:
                        n2 = dt * n * lab["dataProcessingMultiplier"] * scientists * float(stored) / (10.0 ** lab["researchTime"])
                        n3 = float(lab["scienceMultiplier"]) * n2
                        if n3 + float(sci) > cap:
                            status = 2
                        else:
                            status = 3
                            stored = f32(stored - f32(n2))
                            sci = f32(sci + f32(n3))
                n2_ = tf / dt
                if abs(n2_ - last_tf) > 1e-9:
                    last_tf = n2_
        steps.append({"ut": ut, "lastUpdateTime": last_update, "catchupRetries": retries, "lastTimeFactor": last_tf, "ec": ec,
                      "dataStored": bits(stored), "storedScience": bits(sci), "status": status, "activated": activated})
    return steps


def converter_scenarios(lab):
    scs = []
    base = [0.0, 100.0, 100.02, 100.04]
    scs.append({"name": "one scientist, fast steps then a 6 h rail jump", "levels": [3], "ec": 100000.0, "dataStored": 200.0, "storedScience": 0.0,
                "maxDeltaTime": 2, "uts": base + [100.06 + 0.02 * i for i in range(30)] + [100.7, 21700.7]})
    scs.append({"name": "electric charge runs out in a long step (catch-up retries)", "levels": [1, 5], "ec": 40.0, "dataStored": 300.0, "storedScience": 10.0,
                "maxDeltaTime": 3600, "uts": [0.0, 50.0, 50.02, 50.04, 50.06, 3650.0, 3651.0, 3652.0, 7300.0]})
    scs.append({"name": "no scientist stops the converter", "levels": [], "ec": 1000.0, "dataStored": 50.0, "storedScience": 0.0, "maxDeltaTime": 2,
                "uts": [0.0, 10.0, 10.02, 10.04, 10.06]})
    scs.append({"name": "science cap reached", "levels": [5, 5, 5], "ec": 1e9, "dataStored": 740.0, "storedScience": 499.9, "maxDeltaTime": 100000,
                "uts": [0.0, 20.0, 20.02, 5000.0, 90000.0, 180000.0]})
    scs.append({"name": "data above storage and science above the cap are clamped", "levels": [2], "ec": 500.0, "dataStored": 900.0, "storedScience": 800.0,
                "maxDeltaTime": 5, "uts": [0.0, 1.0, 1.02, 1.04]})
    scs.append({"name": "tiny data stored", "levels": [4], "ec": 500.0, "dataStored": 5e-10, "storedScience": 1.0, "maxDeltaTime": 5,
                "uts": [0.0, 1.0, 1.02, 1.04]})
    scs.append({"name": "four scientists, 24 h warp", "levels": [1, 2, 3, 4], "ec": 1e7, "dataStored": 600.0, "storedScience": 0.0, "maxDeltaTime": 1000000,
                "uts": [0.0, 30.0, 30.02, 86430.0, 172830.0]})
    out = []
    for sc in scs:
        out.append({"name": sc["name"], "levels": sc["levels"], "ec": sc["ec"], "dataStored": sc["dataStored"], "storedScience": sc["storedScience"],
                    "maxDeltaTime": sc["maxDeltaTime"], "expected": run_converter(sc, lab)})
    return out


def main():
    exps = load_experiments()
    bodies = load_bodies()
    lab = load_lab()
    cases = subject_cases(exps, bodies)
    out = {
        "provenance": "tools/wp35/science_reference.py (numpy float32 per the Mono float rule; own cfg parser; shares no code with kspcore)",
        "experiments": exps,
        "bodies": [{"name": b["name"], "atmosphere": b["atmosphere"], "isHomeWorld": b["isHomeWorld"], "biomes": b["biomes"],
                    "sv": {k: bits(v) for k, v in b["sv"].items()}} for b in bodies],
        "lab": {k: (bits(v) if isinstance(v, float) else v) for k, v in lab.items()},
        "subjectCases": cases,
        "labCases": lab_cases(cases, lab),
        "converterFormulas": converter_formula_cases(lab),
        "converterScenarios": converter_scenarios(lab),
    }
    json.dump(out, sys.stdout, indent=None, separators=(",", ":"))
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
