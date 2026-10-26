#!/usr/bin/env python3
"""WP-19c independent reference: the rules of the flight-registry close-out.

No kspcore code. Everything is written from the decompiled C# (decompiled/KSP-Decompiled/Assembly-CSharp); the English texts are read from the
game's own dictionary (core/testdata/GameData-mirror/GameData/Squad/Localization/dictionary.cfg, en-us, KSP 1.12.5):
  FlightState.cs          FlightState() (:33-89): FlightGlobals.Vessels last to first, DEAD skipped, DECLUTTER_KSC auto-clean of landed
                          debris (Vessel.LandedInKSC :604, LandedInStockLaunchSite :617, SetAutoClean :4412 keeps the first reason), the
                          MAX_VESSELS_BUDGET cut (last to first, non-persistent non-commandable, while over budget), BackupVessel in reverse,
                          activeVesselIdx = ready ? IndexOf(ActiveVessel.protoVessel) : 0; Vessel.isCommandable :642 (type > Debris)
  ProtoVessel.cs          Clean (:1163-1213): the home-world recovery deltas Mathf.Round(after - before) on float32 (Mathf.Round = (float)
                          Math.Round(double): ties to even), the CAREER / SCIENCE_SANDBOX flag, the warning text;
                          FinePrint/Utilities/StringUtilities.cs FormattedCurrencies (:467, symbols / verbose false, reason None, style
                          None) and ThisThisAndThat (:226); CurrentFunds / Science / Reputation (:1215-1240: 0 without the scenario)
                          GetSituationString (:994-1059)
  ProtoPartSnapshot.cs    LoadEVAInfoBySuitGender (:964-1063); ExpansionsLoader.IsExpansionAnyKerbalSuitInstalled / IsExpansionKerbalSuitInstalled
  QuickSaveLoad.cs        QuickSaveClearToSave (:226-234); Update (:101-182): the QUICKSAVE / QUICKLOAD / MODIFIER key branches

Fixtures that are NOT KSP data: every scene (flags, names, amounts) is generated here from a fixed seed.

Output: core/tests/data/wp19c_reference.json (float32 as 8 hex digits of the IEEE-754 bits, double as 16; everything else plain JSON),
byte-reproducible.  Usage:  python -I core/tools/wp19c/registry_reference.py
"""
import json
import os
import random
import re
import struct

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(CORE, "tests", "data", "wp19c_reference.json")
DICT = os.path.join(CORE, "testdata", "GameData-mirror", "GameData", "Squad", "Localization", "dictionary.cfg")

f32 = np.float32
R = random.Random(19193)

SITUATIONS = {"LANDED": 1, "SPLASHED": 2, "PRELAUNCH": 4, "FLYING": 8, "SUB_ORBITAL": 16, "ORBITING": 32, "ESCAPING": 64, "DOCKED": 128}


def hf(x):
    return struct.pack(">f", float(f32(x))).hex()


def hx(x):
    return struct.pack(">d", float(x)).hex()


def load_en_us():
    """The en-us block of the game's dictionary: tag -> text (backslash-u escapes decoded, as Localizer.AddTagValuesFromNode does)"""
    tags = {}
    lang = None
    depth = 0
    with open(DICT, encoding="utf-8-sig") as f:
        for raw in f:
            line = raw.strip()
            if line == "{":
                depth += 1
                continue
            if line == "}":
                depth -= 1
                if depth <= 1:
                    lang = None
                continue
            if depth == 1 and line and "=" not in line:
                lang = line
                continue
            if lang == "en-us" and "=" in line:
                k, v = line.split("=", 1)
                v = v.strip()
                v = re.sub(r"\\u([0-9a-fA-F]{4})", lambda m: chr(int(m.group(1), 16)), v)
                tags[k.strip()] = v
    return tags


EN = load_en_us()


def loc(tag, *args):
    """Localizer.Format for the plain templates used here: the tag's text, <<n>> replaced by the n-th argument"""
    text = EN.get(tag, tag)
    for i, a in enumerate(args, 1):
        text = text.replace("<<%d>>" % i, a)
    return text


# ---------------------------------------------------------------- FlightState() (FlightState.cs:33)
STOCK_SITES = ["LaunchPad", "Runway", "Island_Airfield", "Woomerang_Launch_Site", "Desert_Launch_Site"]
LANDED_AT = ["", "KSC", "LaunchPad", "Runway", "KSC_LaunchPad_Platform", "Island_Airfield", "Woomerang_Launch_Site", "Mun", "Duna_Site",
             "Desert_Airfield"]


def landed_in_ksc(text):  # Vessel.cs:604
    return "KSC" in text or "LaunchPad" in text or "Runway" in text


def landed_in_stock_site(text):  # Vessel.cs:617 (text.Contains(site.name))
    return any(s in text for s in STOCK_SITES)


def capture_scene():
    n = R.randint(0, 9)
    vessels = []
    for _ in range(n):
        t = R.choice([0, 0, 0, 0, 1, 3, 7, 11, 15, 16])
        vessels.append({
            "dead": R.random() < 0.15,
            "type": t,
            "persistent": R.random() < 0.3,
            "situation": R.choice(["LANDED", "LANDED", "LANDED", "ORBITING", "PRELAUNCH", "SPLASHED", "FLYING"]),
            "landedAt": R.choice(LANDED_AT),
            "autoClean": R.random() < 0.1,  # already auto-cleaned (its reason must survive)
        })
    budget = R.choice([-1, -1, 250, 0, 1, 2, 3, 4, 5])
    declutter = R.random() < 0.7
    ready = R.random() < 0.7
    active = R.randint(0, n - 1) if n > 0 else -1
    clean = [[v["autoClean"], "old reason" if v["autoClean"] else ""] for v in vessels]
    lst = []
    i = n
    while i > 0:
        i -= 1
        v = vessels[i]
        if v["dead"]:
            continue
        if declutter and not (v["type"] > 0) and not v["persistent"] and v["situation"] == "LANDED":
            if landed_in_ksc(v["landedAt"]) and not clean[i][0]:
                clean[i] = [True, "it was debris cluttering up KSC"]
            if landed_in_stock_site(v["landedAt"]) and not clean[i][0]:
                clean[i] = [True, "it was debris cluttering up a LaunchSite"]
        lst.append(i)
    if budget != -1:
        c2 = len(lst)
        while True:
            c2 -= 1
            if not (c2 >= 0 and len(lst) > budget):
                break
            v = vessels[lst[c2]]
            if not v["persistent"] and not (v["type"] > 0):
                del lst[c2]
    captured = list(reversed(lst))
    if ready:
        active_idx = captured.index(active) if active in captured else -1
    else:
        active_idx = 0
    return {"vessels": vessels, "budget": budget, "declutter": declutter, "ready": ready, "active": active,
            "captured": captured, "autoClean": clean, "activeVesselIdx": active_idx}


# ---------------------------------------------------------------- ProtoVessel.Clean (ProtoVessel.cs:1163)
def round_half_even(x):
    return float(np.round(np.float64(x)))  # Math.Round(double): ties to even


def n0(x):
    """float.ToString("N0") of an integral float32 (InvariantCulture group separator ',')"""
    v = int(float(x))
    s = "{:,}".format(abs(v))
    return ("-" if v < 0 else "") + s


def this_this_and_that(lst):  # StringUtilities.cs:226
    if not lst:
        return "(NO ELEMENTS)"
    if len(lst) == 1:
        return lst[0]
    connector = loc("#autoLOC_6002373")
    conjugation = loc("#autoLOC_6002374")
    out = ""
    for i in range(len(lst) - 2):
        out += lst[i] + connector
    return out + lst[-2] + conjugation + lst[-1]


def fsub(a, b):
    """a float32 difference under the Mono rule (evaluated in double, one float32 store)"""
    return float(f32(np.float64(a) - np.float64(b)))


def clean_scene():
    mode = R.choice(["CAREER", "CAREER", "SCIENCE_SANDBOX", "SANDBOX"])
    home = R.random() < 0.8
    has = {"funding": R.random() < 0.85, "rnd": R.random() < 0.85, "rep": R.random() < 0.85}
    funds0 = float(R.randint(0, 2000000)) + R.choice([0.0, 0.25, 0.5, 0.75])
    sci0 = float(f32(R.randint(0, 5000) + R.choice([0.0, 0.5, 0.3])))
    rep0 = float(f32(R.uniform(-100, 900)))
    # what the onVesselRecovered listener adds (the recovery's Funding / R&D / Reputation work in the game)
    d_funds = R.choice([0.0, 0.5, 1.5, 2.5, 1234.4, 1234.5, 1235.5, 98765.0, -500.0, 0.4])
    d_sci = float(f32(R.choice([0.0, 0.5, 2.5, 3.5, 12.7, 100.0])))
    d_rep = float(f32(R.choice([0.0, 0.5, 2.5, 7.25, 30.0])))
    name = R.choice(["", "Kerbal X", "Probe 7"])
    reason = R.choice(["", "it was debris cluttering up KSC", "test"])

    funds1 = funds0 + d_funds  # Funding.Funds (double) after the listener
    sci1 = float(f32(np.float64(f32(sci0)) + np.float64(f32(d_sci))))
    rep1 = float(f32(np.float64(f32(rep0)) + np.float64(f32(d_rep))))

    def cur(present, value):
        return float(f32(value)) if present else 0.0  # (float)Funding.Instance.Funds / R&D.Science / Reputation.reputation; 0 without

    flag = False
    df = ds = dr = 0.0
    if home:
        a = cur(has["funding"], funds0)
        b = cur(has["rnd"], sci0)
        c = cur(has["rep"], rep0)
        flag = True
        df = float(f32(round_half_even(fsub(cur(has["funding"], funds1), a))))
        ds = float(f32(round_half_even(fsub(cur(has["rnd"], sci1), b))))
        dr = float(f32(round_half_even(fsub(cur(has["rep"], rep1), c))))
    flag = flag and mode in ("CAREER", "SCIENCE_SANDBOX")
    text = "Vessel " + ("was " if name == "" else name + " was ")
    if flag:
        lst = []
        if has["funding"] and n0(df) != "0":
            lst.append(n0(df) + " " + loc("#autoLOC_7001031") + " ")
        if has["rnd"] and n0(ds) != "0":
            lst.append(n0(ds) + " " + loc("#autoLOC_7001032") + " ")
        if has["rep"] and n0(dr) != "0":
            lst.append(n0(dr) + " " + loc("#autoLOC_7001033") + " ")
        text += ("recovered for a refund of " + this_this_and_that(lst)) if lst else "removed from the game"
    else:
        text += "removed from the game"
    text += "." if reason == "" else ": " + reason + "."
    return {"mode": mode, "home": home, "has": has, "funds0": hx(funds0), "sci0": hf(sci0), "rep0": hf(rep0),
            "dFunds": hx(d_funds), "dSci": hf(d_sci), "dRep": hf(d_rep), "name": name, "reason": reason,
            "fired": home, "deltas": [hf(df), hf(ds), hf(dr)], "text": text}


# ---------------------------------------------------------------- ProtoVessel.GetSituationString (ProtoVessel.cs:994)
def situation_scene():
    sit = R.choice(list(SITUATIONS.keys()) + ["NONE"])
    ref = R.randint(0, 3)
    nbodies = R.choice([-1, -1, 0, 1, 2, 3, 4, 4, 4])
    body_names = ["Kerbol", "Kerbin", "Mun", "Minmus"]
    landed_at = R.choice(["", "", "KSC"])
    disp = R.choice(["", "#autoLOC_145785", "Launch Pad"]) if landed_at else R.choice(["", "Launch Pad"])
    dloc = loc(disp) if disp.startswith("#") else disp  # Localizer.Format(pv.displaylandedAt)
    if nbodies != -1 and nbodies > ref:
        text = body_names[ref]
        table = {"SUB_ORBITAL": loc("#autoLOC_145790"), "FLYING": loc("#autoLOC_145786"), "SPLASHED": loc("#autoLOC_145789"),
                 "PRELAUNCH": loc("#autoLOC_145785"), "DOCKED": loc("#autoLOC_145792"), "ESCAPING": loc("#autoLOC_145791", text),
                 "ORBITING": loc("#autoLOC_145788", text)}
        if sit == "LANDED":
            if landed_at == "":
                out = loc("#autoLOC_6100012", text)
            elif disp == "":
                out = None  # the minibiome display name of R&D: not modelled here
            else:
                out = loc("#autoLOC_6100012", dloc)
        else:
            out = table.get(sit, loc("#autoLOC_145793"))
    else:
        table = {"SUB_ORBITAL": loc("#autoLOC_145805"), "FLYING": loc("#autoLOC_145801"), "SPLASHED": loc("#autoLOC_145804"),
                 "PRELAUNCH": loc("#autoLOC_145800"), "DOCKED": loc("#autoLOC_145807"), "ESCAPING": loc("#autoLOC_145806"),
                 "ORBITING": loc("#autoLOC_145803")}
        if sit == "LANDED":
            if landed_at == "":
                out = loc("#autoLoc_6002161")
            elif disp == "":
                out = None
            else:
                out = loc("#autoLOC_6100016", dloc)
        else:
            out = table.get(sit, loc("#autoLOC_145808"))
    return {"situation": SITUATIONS.get(sit, 0), "nbodies": nbodies, "ref": ref, "bodyNames": body_names, "landedAt": landed_at,
            "displaylandedAt": disp, "text": out}


# ---------------------------------------------------------------- ProtoPartSnapshot.LoadEVAInfoBySuitGender (:964)
def eva_part(suit, gender, any_suit, vintage, future):
    def by_gender(female, male):
        return {"Female": female, "Male": male}[gender]
    if any_suit:
        if suit == "Default":
            return by_gender("kerbalEVAfemale", "kerbalEVA")
        if suit == "Vintage":
            return by_gender("kerbalEVAfemaleVintage", "kerbalEVAVintage") if vintage else by_gender("kerbalEVAfemale", "kerbalEVA")
        if suit == "Future":
            return by_gender("kerbalEVAfemaleFuture", "kerbalEVAFuture") if future else by_gender("kerbalEVAfemale", "kerbalEVA")
        if suit == "Slim":
            return by_gender("kerbalEVASlimSuitFemale", "kerbalEVASlimSuit")
        return None
    if suit == "Slim":
        return by_gender("kerbalEVASlimSuit", "kerbalEVASlimSuit")
    if suit == "Default":
        return by_gender("kerbalEVAfemale", "kerbalEVA")
    return None


def suit_getter(stored, mh, bg):
    """ProtoCrewMember.suit (ProtoCrewMember.cs:257): a stored suit whose DLC is not installed reads (and becomes) Default;
    ExpansionsLoader.IsExpansionKerbalSuitInstalled (:371): Default and Slim always, Vintage with Making History, Future with Serenity"""
    installed = {"Default": True, "Slim": True, "Vintage": mh, "Future": bg}[stored]
    return stored if installed else "Default"


def eva_table():
    rows = []
    for suit in ["Default", "Vintage", "Future", "Slim"]:
        for gender in ["Male", "Female"]:
            for mh in (False, True):
                for bg in (False, True):
                    # the stock DLCs: Making History brings the Vintage suit, Breaking Ground (Serenity) the Future suit; `suit` is the value
                    # stored on the kerbal (loaded while its DLC was there), the switch reads the getter
                    rows.append({"suit": suit, "gender": gender, "makingHistory": mh, "serenity": bg,
                                 "part": eva_part(suit_getter(suit, mh, bg), gender, mh or bg, mh, bg)})
    return rows


# ---------------------------------------------------------------- QuickSaveLoad
CLEAR_STATUSES = ["CLEAR", "NOT_IN_ATMOSPHERE", "NOT_UNDER_ACCELERATION", "NOT_WHILE_MOVING_OVER_SURFACE", "NOT_WHILE_ABOUT_TO_CRASH",
                  "NOT_WHILE_ON_A_LADDER", "NOT_WHILE_THROTTLED_UP", "ORBIT_EVENT_IMMINENT"]


def quicksave_clear_table():
    rows = []
    for i, st in enumerate(CLEAR_STATUSES):
        for always in (False, True):
            refused = st != "CLEAR" and not always and st in ("NOT_WHILE_ON_A_LADDER", "NOT_WHILE_MOVING_OVER_SURFACE", "NOT_WHILE_ABOUT_TO_CRASH")
            rows.append({"status": i, "canAlways": always, "clear": not refused})
    return rows


def update_scene():
    """QuickSaveLoad.Update (:101-182) for one frame: which actions run, in order"""
    s = {
        "cameraMode": R.random() < 0.1,
        "saveLocked": R.random() < 0.15,
        "loadLocked": R.random() < 0.15,
        "saveDown": R.random() < 0.4,
        "loadDown": R.random() < 0.4,
        "loadUp": R.random() < 0.2,
        "modifier": R.random() < 0.3,
        "canQuickLoad": R.random() < 0.8,
        "holdToLoad": R.random() < 0.4,
        "paused": R.random() < 0.2,
        "invoking": R.random() < 0.3,
    }
    act = []
    invoking = s["invoking"]
    load_locked = s["loadLocked"]
    if not s["cameraMode"]:
        done = False
        if not s["saveLocked"] and s["saveDown"]:
            act.append("quickSave(%s)" % ("saveAs" if s["modifier"] else "save"))
            if s["modifier"]:
                # SpawnSaveAsDialog (:345): InputLockManager.SetControlLock("SaveAsDialog") locks every control type for the rest of the frame
                load_locked = True
        if not load_locked and s["loadDown"]:
            if not s["canQuickLoad"]:
                act.append("msg:#autoLOC_174575")
                done = True  # return
            elif s["holdToLoad"] and not s["paused"]:
                if not s["modifier"]:
                    act.append("msg:#autoLOC_174589")
                if not invoking:
                    act.append("invoke")
                    invoking = True
            else:
                act.append("quickLoad")
        if not done:
            if s["loadUp"] and invoking:
                act.append("cancelInvoke")
                invoking = False
            if not load_locked and s["modifier"] and s["loadDown"]:
                act.append("loadDialog" if s["canQuickLoad"] else "msg:#autoLOC_174610")
    s["actions"] = act
    s["invokingAfter"] = invoking
    return s


def main():
    data = {
        "_origin": "core/tools/wp19c/registry_reference.py (from the decompiled C#; texts from the game's en-us dictionary)",
        "stockSites": STOCK_SITES,
        "capture": [capture_scene() for _ in range(400)],
        "clean": [clean_scene() for _ in range(300)],
        "situation": [situation_scene() for _ in range(300)],
        "eva": eva_table(),
        "quicksaveClear": quicksave_clear_table(),
        "update": [update_scene() for _ in range(400)],
    }
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, indent=None, separators=(",", ":"), sort_keys=True)
        f.write("\n")
    print("wrote", OUT)


if __name__ == "__main__":
    main()
