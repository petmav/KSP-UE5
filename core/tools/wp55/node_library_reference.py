#!/usr/bin/env python3
"""WP-55 independent reference for the Making History node library (Tests / Actions / Adjusters).

Written from the decompiled C# (Assembly-CSharp/Expansions/Missions/{Tests,Actions,Adjusters}/*.cs) with no kspcore code. It generates the
INPUTS (seeded module configs, vessel / mission scenarios, event sequences) and the EXPECTED outputs; tests/test_node_library.cpp rebuilds the
same scenarios in kspcore and compares:

  per module and case   title (Awake), GetInfo, GetDisplayName, Load -> Save of a seeded ConfigNode (canonical dump), GetNodeBodyParameterString,
                        Test() over seeded vessel / mission scenarios, GetScoreModifier, the event-driven tests (stage activated, SOI changes,
                        recovery, vessel count), the action Fire() side effects, the adjusters' Apply* maths (float32, Mono rule), Activate()
                        status texts and the calls made on the adjusted part module
  registry              the names registered per kind, the accounting of every concrete C# class of Tests/, Actions/, Adjusters/ as ported or
                        deferred (computed from the decompiled sources) and the [MEScoreModule] table (parsed from the sources)

Text goes through the real Squad/Localization/dictionary.cfg of the GameData mirror. The mission files of the mirror are 0-byte files, so every
scenario is a labelled synthetic input to an algorithm; no game data is hand-authored.

Usage: python -I tools/wp55/node_library_reference.py   (writes tests/data/wp55_reference.json)
"""
import json
import os
import random
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from ref_base import *  # noqa: E402,F401,F403
from ref_base import Cfg, TESTS, ACTIONS, DYNAMICS, ENUMS, MISSIONS, CORE, OUT, f32, fnum, dnum, fmt, tag  # noqa: E402
import ref_tests  # noqa: E402
import ref_actions  # noqa: E402
import ref_adjusters  # noqa: E402
from ref_tests import Env, round_half_up  # noqa: E402

POOL = [0.0, 10.0, 25.0, 50.0, 100.0, 300.0, 1000.0]
GUID_FMT = "6d650000-0000-0000-0000-%012d"

# ---- the accounting of the C# classes ------------------------------------------------------------------------------------------
DEFERRED = {
    "Tests": [],
    "Actions": [],
    "Adjusters": [],
}
# ported by WP-54c (tools/wp54c): registered through RegisterNodeLibrary(), gated by tests/test_missions3.cpp
SLICE3 = {
    "Tests": ["TestDistance"],
    "Actions": ["ActionPartFailure", "ActionPartRepair", "ActionPartResourceDrain"],
    "Adjusters": [],
}
# ported by WP-54 slice 2 (tools/wp54b): registered in the same registries, gated by tests/test_missions2.cpp
SLICE2 = {
    "Tests": ["TestCrewAssignment", "TestFlyThrough", "TestGoTo", "TestGrapple", "TestKerbalBoarded", "TestKerbalEVA", "TestKerbalKilled",
              "TestKerbalRecovery", "TestKerbalRescue", "TestOrbit", "TestOrbitParams", "TestPartDocking", "TestPlantFlag", "TestScienceExperiment",
              "TestTakeKerbal", "TestVesselSituationFlying", "TestVesselSituationLanded", "TestVesselSituationSplashed", "TestVesselStateCrashed",
              "TestVesselVelocity"],
    "Actions": ["ActionCreateAsteroid", "ActionCreateComet", "ActionCreateFlag", "ActionCreateKerbal", "ActionCreateLaunchSite", "ActionCreateVessel"],
    "Adjusters": [],
}


def concrete_classes(folder):
    """Top-level non-abstract classes of Missions/<folder>/*.cs: (name -> abstract?)."""
    out = {}
    d = os.path.join(MISSIONS, folder)
    for fn in sorted(os.listdir(d)):
        if not fn.endswith(".cs"):
            continue
        stem = fn[:-3]
        src = open(os.path.join(d, fn), encoding="utf-8-sig").read()
        for m in re.finditer(r"(?:public|internal)\s+(abstract\s+)?(?:sealed\s+)?class\s+(\w+)", src):
            if m.group(2) == stem:
                out[stem] = bool(m.group(1))
    return out


def registry_section():
    reg = {}
    for folder, table in (("Tests", TESTS), ("Actions", ACTIONS), ("Adjusters", DYNAMICS)):
        cls = concrete_classes(folder)
        concrete = sorted(n for n, abstract in cls.items() if not abstract)
        deferred = sorted(DEFERRED[folder])
        slice2 = sorted(SLICE2[folder])
        slice3 = sorted(SLICE3[folder])
        for d in deferred + slice2 + slice3:
            assert d in concrete, (folder, d)
        ported = sorted(n for n in concrete if n not in deferred and n not in slice2 and n not in slice3)
        assert ported == sorted(table.keys()), (folder, set(ported) ^ set(table.keys()))
        reg[folder] = {"ported": ported, "deferred": deferred, "slice2": slice2, "slice3": slice3, "abstract": sorted(n for n, a in cls.items() if a)}
    return reg


def score_attr_table():
    """[MEScoreModule(new Type[] { typeof(A), typeof(B) })] of each ported test class (parsed from the sources)."""
    out = {}
    for name in TESTS:
        src = open(os.path.join(MISSIONS, "Tests", name + ".cs"), encoding="utf-8-sig").read()
        m = re.search(r"\[MEScoreModule\(new Type\[\]\s*\{(.*?)\}\)\]\s*(?:public|internal)", src, re.S)
        if m:
            out[name] = re.findall(r"typeof\((\w+)\)", m.group(1))
    return out


# ---- scenario generators ---------------------------------------------------------------------------------------------------------
def pick(rng, seq):
    return seq[rng.randrange(len(seq))]


def gen_vessel(rng, i, loaded=True):
    pid = 100 + 7 * i
    nparts = 1 + rng.randrange(5)
    v = {
        "pid": pid, "id": GUID_FMT % (500 + i), "missionTime": pick(rng, POOL), "altitude": pick(rng, POOL), "radarAltitude": pick(rng, POOL),
        "state": pick(rng, ENUMS["VesselState"]), "currentStage": rng.randrange(6), "loaded": loaded,
        "partMasses": [f32(pick(rng, [0.5, 1.25, 2.0, 5.0, 12.5, 25.0])) for _ in range(nparts)],
        "partIds": [pid * 10 + k for k in range(nparts)],
        "crew": [pick(rng, ["Pilot", "Engineer", "Scientist"]) for _ in range(rng.randrange(4))],
        "type": pick(rng, ["Ship", "Probe", "EVA", "Flag", "Debris", "DeployedSciencePart", "Station"]),
        "discovery": pick(rng, ["Owned", "Unowned", "Presence"]),
        "modules": pick(rng, [[], [], ["ModuleAsteroid"], ["ModuleComet"]]),
        "resources": {"LiquidFuel": pick(rng, POOL), "Oxidizer": pick(rng, POOL)},
    }
    return v


def gen_env(rng, nv=None, start=True):
    nv = nv if nv is not None else 1 + rng.randrange(3)
    vessels = [gen_vessel(rng, i) for i in range(nv)]
    nodes = []
    if start:
        nodes.append({"guid": GUID_FMT % 1, "title": "Start Node", "activatedUT": pick(rng, [0.0, 10.0, 50.0]), "isStart": True})
    nodes.append({"guid": GUID_FMT % 2, "title": "Alpha", "activatedUT": pick(rng, [0.0, 25.0, 100.0]), "isStart": False})
    nodes.append({"guid": GUID_FMT % 3, "title": "Beta", "activatedUT": pick(rng, [0.0, 50.0, 300.0]), "isStart": False})
    return {
        "ut": pick(rng, POOL), "startedUT": pick(rng, [0.0, 10.0, 100.0]), "score": float(f32(pick(rng, POOL))),
        "funds": pick(rng, [None, 0.0, 1000.0, 25000.0]), "vessels": vessels, "active": rng.randrange(-1, nv),
        "nodes": nodes, "startNode": (GUID_FMT % 1) if start else None,
        "resourceDefs": RESOURCE_DEFS,
    }


def parse_resource_defs():
    """name -> localized displayName of the stock resource definitions (GameData mirror, Squad/Resources)."""
    path = os.path.join(CORE, "testdata", "GameData-mirror", "GameData", "Squad", "Resources", "ResourcesGeneric.cfg")
    text = open(path, encoding="utf-8-sig").read()
    defs = {}
    for m in re.finditer(r"RESOURCE_DEFINITION\s*\{(.*?)\n\}", text, re.S):
        body = m.group(1)
        n = re.search(r"^\s*name\s*=\s*(\S+)", body, re.M)
        d = re.search(r"^\s*displayName\s*=\s*([^\s/]+)", body, re.M)
        if n:
            # PartResourceDefinition.displayName is the raw value (PartResourceDefinition.cs:105): the tag, localized only by the UI
            defs[n.group(1)] = d.group(1) if d else n.group(1)
    return defs


RESOURCE_DEFS = {}


def rand_value(rng, typ, enum):
    if typ == "double":
        return dnum(pick(rng, POOL))
    if typ == "float":
        return fnum(f32(pick(rng, [0.0, 0.5, 1.25, 10.0, 25.0, 150.0, 1000.0])))
    if typ in ("int", "uint"):
        return str(pick(rng, [0, 1, 2, 3, 5, 10]))
    if typ == "bool":
        return pick(rng, ["True", "False", "true", "bogus"])
    if typ == "enum":
        return pick(rng, ENUMS[enum] + ["Bogus"])
    return pick(rng, ["Pilot", "Any Role", "", "LiquidFuel", "Oxidizer", "abc"])


RESOURCE_KEYS = ("resourceName",)


def cfg_for(rng, cls, env=None):
    """A seeded ConfigNode for class `cls` (keys of its Load, some missing)."""
    cn = Cfg("N")
    if cls.BASE in ("test", "action") and rng.randrange(2):
        cn.values.append(("title", pick(rng, ["My title", "#autoLOC_8000052", "T"])))
    if cls.BASE in ("test", "action") and rng.randrange(2):
        for _ in range(1 + rng.randrange(2)):
            cn.values.append(("parametersDisplayedInSAP", pick(rng, ["a", "b", "time"])))
    if cls.BASE == "action":
        if rng.randrange(2):
            cn.values.append(("restartOnSceneLoad", pick(rng, ["True", "False"])))
        if rng.randrange(2):
            cn.values.append(("isRunning", pick(rng, ["True", "False"])))
    if issubclass(cls, ref_tests.TV) or cls.NAME in ("ActionVessel", "ActionVesselExplode", "ActionPartResourceAmount"):
        if rng.randrange(3):
            cn.values.append(("vesselID", str(pick(rng, [0, 100, 107, 114, 999]))))
        if rng.randrange(4) == 0:
            cn.values.append(("oldRootPart", str(pick(rng, [0, 1000, 1070]))))
    for entry in getattr(cls, "LOAD", []):
        key, attr, typ, enum, default = entry
        if rng.randrange(5) == 0:
            continue
        cn.values.append((key, pick(rng, ["LiquidFuel", "Oxidizer", "ElectricCharge"]) if key in RESOURCE_KEYS else rand_value(rng, typ, enum)))
    return cn


# per class extra cfg keys not covered by LOAD
def extra_cfg(rng, cls, cn):
    n = cls.NAME
    if n == "TestTimeSinceNode":
        if rng.randrange(3):
            cn.values.append(("nodeID", GUID_FMT % pick(rng, [1, 2, 3, 9])))
        if rng.randrange(3):
            cn.values.append(("time", dnum(pick(rng, POOL))))
        if rng.randrange(3):
            cn.values.append(("comparisonOperator", pick(rng, ENUMS["LG"] + ["Bogus"])))
    elif n in ("TestSOILeft", "TestSOIReached"):
        if rng.randrange(3):
            cn.values.append(("bodyName", pick(rng, ["Kerbin", "Mun", "Minmus", "AnyValid", "Bogus", ""])))
    elif n == "ActionMessage":
        if rng.randrange(3):
            cn.values.append(("message", pick(rng, ["hello", "line1\\nline2", "tab\\there", ""])))
        if rng.randrange(3):
            cn.values.append(("duration", fnum(f32(pick(rng, [0.0, 2.5, 5.0, 12.0])))))
        if rng.randrange(3):
            cn.values.append(("style", pick(rng, ENUMS["ScreenStyle"] + ["Bogus"])))
    elif n == "ActionDialogMessage":
        if rng.randrange(2):
            cn.values.append(("messageHeading", pick(rng, ["Head", "Welcome"])))
        if rng.randrange(2):
            cn.values.append(("message", pick(rng, ["hello", "line1\\nline2", "tab\\there"])))
        if rng.randrange(3):
            cn.values.append(("instructorName", pick(rng, ["Instructor_Gene", "Strategy_PRGuy", "Instructor_Jeb_MaleVeteran", "Instructor_Bill_MaleVeteran",
                                                         "Instructor_Bob_MaleVeteran", "Instructor_Val_FemaleVeteran", "Other"])))
        for key, typ in (("vintageSuit", "bool"), ("textAreaSize", "int"), ("pauseMission", "bool"), ("autoClose", "bool"),
                         ("autoCloseTimeOut", "int"), ("isBriefingMessage", "bool"), ("autoGrowDialogHeight", "bool"), ("maxDialogHeight", "bool")):
            if rng.randrange(3):
                cn.values.append((key, rand_value(rng, typ, None)))
        if rng.randrange(3):
            cn.values.append(("screenArea", pick(rng, ENUMS["DialogArea"] + ["Bogus"])))
    elif n in ("ActionMissionScore", "ActionModifierScore"):
        m = Cfg("SCOREMODULES")
        mod = Cfg("SCOREMODULE")
        if n == "ActionMissionScore":
            mod.values += [("name", "ScoreModule_Completion"), ("bonusScore", fnum(f32(pick(rng, [0.0, 100.0, 250.0, 1000.0]))))]
        else:
            mod.values += [("name", "ScoreModule_Modifier"), ("type", pick(rng, ENUMS["ScoreMod"])), ("modifier", fnum(f32(pick(rng, [2.0, 4.0, 10.0, 50.0]))))]
        m.nodes.append(mod)
        cn.nodes.append(m)
    elif n == "ActionModifierFunds":
        pass
    elif n == "ActionPartExplode":
        if rng.randrange(2):
            cn.values.append(("partPersistentId", str(pick(rng, [0, 1000, 1070, 1140]))))
        if rng.randrange(2):
            cn.values.append(("vesselPersistentId", str(pick(rng, [0, 100, 107, 999]))))
        if rng.randrange(2):
            cn.values.append(("partName", pick(rng, ["#autoLOC_8001004", "Mk1 Pod"])))
    return cn


# ---- modules under test -----------------------------------------------------------------------------------------------------------
SCOREABLE = {"TestMETTime", "TestMissionTime", "TestTimeSinceNode", "TestMissionScore", "TestMissionFunds", "TestGameEvent", "TestVesselMass",
             "TestVesselPartCount", "TestVesselCrewCount", "TestVesselAltitude", "TestVesselResource", "TestVesselStage", "TestVesselStageActivated",
             "TestVesselState", "TestFundsRecovery", "TestResourcesRecovery", "TestSOILeft", "TestSOIReached"}

BODY_FIELDS = {
    "TestMETTime": ["time"], "TestMissionTime": ["time"], "TestTimeSinceNode": ["nodeID", "time"], "TestMissionScore": ["score"],
    "TestMissionFunds": ["funds"], "TestAccuracy": ["accuracy"], "TestVesselMass": ["mass"], "TestVesselPartCount": ["parts"],
    "TestVesselCrewCount": ["crewAmount", "traitName"], "TestVesselAltitude": ["altitude", "useRadarAltimiter"],
    "TestVesselActiveCount": ["vesselCount"], "TestVesselResource": ["resourceAmount", "resourceName"], "TestVesselStage": ["stage"],
    "TestFundsRecovery": ["fundsToRecover"], "TestResourcesRecovery": ["resourcesToRecover"],
    "ActionDialogMessage": ["missionInstructor"], "ActionVessel": ["vesselID"], "ActionVesselExplode": ["partsExplodeTime", "vesselID"],
    "ActionPartResourceAmount": ["resourceName", "vesselID"], "ActionPartExplode": ["vesselPartIDs"],
}


def body_field(name, gui):
    if name == "traitName":
        return {"name": name, "guiName": "#autoLOC_8000111", "value": gui if gui is not None else "", "has": True}
    if name == "resourceName":
        return {"name": name, "guiName": "#autoLOC_8000014", "value": gui if gui is not None else "", "has": True}
    return {"name": name, "guiName": "#autoLOC_8000052", "value": "VAL", "has": True}


def py_module(cls, cfg, env_d, ctor=1):
    if cls.KIND == "dynamic":
        m = cls(ctor=ctor)
    else:
        m = cls()
    env = Env(env_d)
    if cls.NAME == "TestTimeSinceNode":
        m.initialize(env)
    if cfg is not None:
        m.load(cfg)
    return m


def basic_ops(m, cls, rng):
    ops = [{"op": "title", "out": m.title}, {"op": "displayName", "out": m.display_name()}, {"op": "info", "out": m.info()},
           {"op": "saved", "out": m.save().dump()}]
    return ops


def gen_test_case(rng, cls, idx, with_cfg):
    env_d = gen_env(rng)
    cfg = None
    if with_cfg:
        cfg = extra_cfg(rng, cls, cfg_for(rng, cls))
    m = py_module(cls, cfg, env_d)
    ops = basic_ops(m, cls, rng)
    for fname in BODY_FIELDS.get(cls.NAME, []) + ["bogusField"]:
        for gui in (["Pilot", "Any Role", "", "Oxidizer"] if fname in ("traitName", "resourceName") else [None]):
            f = body_field(fname, gui)
            ops.append({"op": "body", "field": f, "env": env_d, "out": m.body(f, Env(env_d)) if hasattr(m, "body") else m.base_body(f)})
    return {"id": "%s#%d" % (cls.NAME, idx), "kind": "test", "name": cls.NAME, "env": env_d, "cfg": cfg.to_json() if cfg else None,
            "ops": ops}, m


def add_test_ops(rng, m, cls, case):
    """Test() / GetScoreModifier over three scenarios (the module keeps its state between them)."""
    ops = case["ops"]
    for k in range(3):
        env_d = gen_env(rng)
        # make the module's vessel exist half of the time
        if hasattr(m, "vesselID") and m.vesselID and rng.randrange(2):
            if env_d["vessels"]:
                env_d["vessels"][0]["pid"] = m.vesselID
                env_d["vessels"][0]["partIds"] = [m.vesselID * 10 + j for j in range(len(env_d["vessels"][0]["partIds"]))]
        env = Env(env_d)
        ops.append({"op": "env", "env": env_d})
        if cls.NAME == "TestAccuracy":
            if rng.randrange(3):
                env_d["dockParent"] = {"accuracyModifier": pick(rng, [None, 0.25, 0.5, 1.0])}
        ops.append({"op": "test", "env": env_d, "out": m.test(env)})
        if cls.NAME in SCOREABLE:
            for mod in ("ScoreModule_Resource", "ScoreModule_Time", "ScoreModule_Accuracy"):
                ops.append({"op": "scoreMod", "module": mod, "out": m.score_modifier(mod, env)})


def gen_event_case(rng, cls, idx):
    """Event-driven tests: init / events / test sequences."""
    env_d = gen_env(rng, nv=3)
    cfg = extra_cfg(rng, cls, cfg_for(rng, cls))
    m = py_module(cls, cfg, env_d)
    env = Env(env_d)
    ops = basic_ops(m, cls, rng)
    ops.append({"op": "env", "env": env_d})

    def ev(e):
        m.on_event(e, env)
        ops.append({"op": "event", "ev": e})

    def t():
        ops.append({"op": "test", "env": env_d, "out": m.test(env)})

    n = cls.NAME
    if n == "TestGameEvent":
        ev({"name": "stageActivate", "arg": 1})
        t()
        t()
        ev({"name": "stageActivate", "arg": 2})
        t()
    elif n == "TestVesselActiveCount":
        m.initialized(env)
        ops.append({"op": "init"})
        t()
        for _ in range(4):
            i = rng.randrange(len(env_d["vessels"]))
            ev({"name": pick(rng, ["addVessel", "removeVessel"]), "vessel": i})
            t()
        # Cleared() registers the handlers again (C# bug): the next event is counted twice
        ops.append({"op": "clear"})
        ev({"name": "addVessel", "vessel": 0})
        t()
    elif n == "TestVesselStageActivated":
        ev({"name": "stageActivate", "arg": m.activation})
        t()
        m.initialized(env)
        ops.append({"op": "init"})
        t()
        ev({"name": "stageActivate", "arg": m.activation + 1})
        t()
        ev({"name": "stageActivate", "arg": m.activation})
        t()
        m.cleared(env)
        ops.append({"op": "clear"})
        ev({"name": "stageActivate", "arg": m.activation})
        t()
    elif n in ("TestVesselRecovery", "TestFundsRecovery"):
        m.initialized(env)
        ops.append({"op": "init"})
        t()
        for pid, funds in ((999, 500.0), (100, 1000.0), (100, 250.0)):
            ev({"name": "recoveryComplete", "pid": pid, "fundsEarned": funds})
            t()
        m.cleared(env)
        ops.append({"op": "clear"})
    elif n == "TestResourcesRecovery":
        m.initialized(env)
        ops.append({"op": "init"})
        t()
        for k in range(3):
            v = env_d["vessels"][rng.randrange(len(env_d["vessels"]))]
            parts = [[{"name": pick(rng, ["LiquidFuel", "Oxidizer", "Bogus"]), "amount": pick(rng, POOL), "max": 1000.0}
                      for _ in range(1 + rng.randrange(3))] for _ in range(1 + rng.randrange(3))]
            ev({"name": "recoveryProcessing", "vesselGuid": v["id"], "pid": v["pid"], "parts": parts})
            t()
        m.cleared(env)
        ops.append({"op": "clear"})
    elif n in ("TestSOILeft", "TestSOIReached"):
        m.initialized(env)
        ops.append({"op": "init"})
        t()
        for k in range(4):
            vi = rng.randrange(len(env_d["vessels"]))
            frm, to = pick(rng, ["Kerbin", "Mun", "Minmus"]), pick(rng, ["Kerbin", "Mun", "Minmus", "Duna"])
            ev({"name": "soi", "vessel": env_d["vessels"][vi]["pid"], "from": frm, "to": to})
            t()
        m.cleared(env)
        ops.append({"op": "clear"})
    if cls.NAME in SCOREABLE:
        for mod in ("ScoreModule_Resource", "ScoreModule_Time"):
            ops.append({"op": "scoreMod", "module": mod, "out": m.score_modifier(mod, env)})
    return {"id": "%s#ev%d" % (cls.NAME, idx), "kind": "test", "name": cls.NAME, "env": env_d, "cfg": cfg.to_json(), "ops": ops}


EVENT_TESTS = {"TestGameEvent", "TestVesselActiveCount", "TestVesselStageActivated", "TestVesselRecovery", "TestFundsRecovery",
               "TestResourcesRecovery", "TestSOILeft", "TestSOIReached"}


def gen_tests(rng):
    cases = []
    for name in sorted(TESTS):
        cls = TESTS[name]
        for k in range(5):
            case, m = gen_test_case(rng, cls, k, with_cfg=(k > 0))
            if name not in EVENT_TESTS:
                add_test_ops(rng, m, cls, case)
            cases.append(case)
        if name in EVENT_TESTS:
            for k in range(3):
                cases.append(gen_event_case(rng, cls, k))
    return cases


def gen_actions(rng):
    cases = []
    for name in sorted(ACTIONS):
        cls = ACTIONS[name]
        for k in range(5):
            with_cfg = k > 0
            env_d = gen_env(rng)
            cfg = extra_cfg(rng, cls, cfg_for(rng, cls)) if with_cfg else None
            m = py_module(cls, cfg, env_d)
            ops = basic_ops(m, cls, rng) if name not in ("ActionMissionScore", "ActionModifierScore") else [
                {"op": "title", "out": m.title}, {"op": "displayName", "out": m.display_name()}, {"op": "info", "out": m.info()}]
            if name in ("ActionMissionScore", "ActionModifierScore"):
                ops.append({"op": "scoreDescription", "out": m.score_description()})
                ops.append({"op": "appObjectiveInfo", "out": m.score_description() + "\n"})
            for fname in BODY_FIELDS.get(name, []):
                f = body_field(fname, None)
                ops.append({"op": "body", "field": f, "env": env_d, "out": m.body(f, Env(env_d))})
            for j in range(3):
                e = gen_env(rng)
                if hasattr(m, "vesselID") and m.vesselID and e["vessels"]:
                    e["vessels"][0]["pid"] = m.vesselID
                    e["vessels"][0]["partIds"] = [m.vesselID * 10 + q for q in range(len(e["vessels"][0]["partIds"]))]
                if name == "ActionPartResourceAmount":
                    for v in e["vessels"]:
                        if rng.randrange(2):
                            v["loaded"] = False
                        v["proto"] = [[{"name": pick(rng, ["LiquidFuel", "Oxidizer"]), "amount": pick(rng, POOL), "max": 1000.0}
                                       for _ in range(1 + rng.randrange(2))] for _ in range(len(v["partMasses"]))]
                    e["requestResult"] = pick(rng, [0.0, 5.0])
                if name == "ActionPartExplode":
                    e["randomIndex"] = 1
                    if m.partID and e["vessels"] and rng.randrange(2):
                        v0 = e["vessels"][0]
                        v0["partIds"][0] = m.partID
                    if m.partID == 0 and m.VesselID and e["vessels"]:
                        e["vessels"][0]["pid"] = m.VesselID
                        e["vessels"][0]["partIds"] = [m.VesselID * 10 + q for q in range(len(e["vessels"][0]["partIds"]))]
                if name in ("ActionMissionScore", "ActionModifierScore"):
                    e["scoreEnabled"] = pick(rng, [True, True, False])
                    e["score"] = float(f32(pick(rng, POOL)))
                if name == "ActionModifierFunds" and e["funds"] is not None and rng.randrange(3):
                    e["funds"] = pick(rng, [1000.0, 25000.0, 777.0])
                ops.append({"op": "env", "env": e})
                ops.append({"op": "fire", "env": e, "out": m.fire(Env(e))})
            cases.append({"id": "%s#%d" % (name, k), "kind": "action", "name": name, "env": env_d, "cfg": cfg.to_json() if cfg else None, "ops": ops})
    return cases


def gen_adjusters(rng):
    cases = []
    for name in sorted(DYNAMICS):
        cls = DYNAMICS[name]
        p = ref_adjusters.plan(name)
        for k in range(6):
            ctor = 1
            with_cfg = k > 0
            cfg = Cfg("N")
            if with_cfg:
                if rng.randrange(2):
                    cfg.values.append(("id", pick(rng, ["7", "12"])))
                if rng.randrange(2):
                    cfg.values.append(("canBeRepaired", pick(rng, ["True", "False"])))
                if rng.randrange(2):
                    cfg.values.append(("adjusterID", GUID_FMT % pick(rng, [11, 12])))
                for kind, key, field in p.load:
                    if rng.randrange(3) == 0:
                        continue
                    if kind == "get":
                        if field == "stuckTorque":
                            cfg.values.append((key, ",".join(fnum(f32(pick(rng, [0.0, 0.5, 1.25, -2.0]))) for _ in range(3))))
                        else:
                            cur = None
                            for typ, fname, default in p.fields:
                                if fname == field:
                                    cur = typ
                            cfg.values.append((key, fnum(f32(pick(rng, [0.0, 10.0, 50.0, 100.0, 150.0, 200.0]))) if cur == "float" else str(pick(rng, [0, 1, 2, 3]))))
                    elif kind == "x100":
                        if rng.randrange(2):
                            cfg.values.append((key, "True"))
                    elif kind == "enumparse":
                        enum_name = None
                        for typ, fname, default in p.fields:
                            if fname == "stuckState":
                                enum_name = {"FailureActivationState": "Act", "FailureDeploymentState": "Dep", "FailureOpenState": "Open"}[typ]
                        cfg.values.append(("stuckState", pick(rng, ENUMS[enum_name])))
            m = cls(ctor=ctor)
            if with_cfg:
                m.load(cfg)
            ops = [{"op": "guiName", "ctor": 0, "out": cls(ctor=0).guiName}, {"op": "guiName", "ctor": 1, "out": cls(ctor=1).guiName},
                   {"op": "displayName", "out": m.display_name()}, {"op": "saved", "out": m.save().dump()},
                   {"op": "flags", "out": [bool(m.canBeRepaired)] + list(m.disable)}]
            for fname, tg, arg in p.body:
                f = {"name": fname, "guiName": "#autoLOC_8000052", "value": "VAL", "has": True}
                ops.append({"op": "body", "field": f, "out": m.body(f)})
            f = {"name": "guiName", "guiName": "#autoLOC_8000052", "value": "My gui", "has": True}
            ops.append({"op": "body", "field": f, "out": m.body(f)})
            f = {"name": "other", "guiName": "#autoLOC_8000052", "value": "VAL", "has": True}
            ops.append({"op": "body", "field": f, "out": m.body(f)})
            ops.append({"op": "body", "field": {"name": "other", "guiName": "#autoLOC_8000052", "value": "", "has": False}, "out": m.body({"name": "other", "guiName": "#autoLOC_8000052", "value": "", "has": False})})
            # Apply* maths
            if hasattr(m, "apply") and name != "FailureLightFlickering":
                for _ in range(5):
                    if name in ("AdjusterLiftingSurfaceLiftForce", "AdjusterGimbalLimitControl", "AdjusterReactionWheelTorque", "FailureRCSFireRandomly"):
                        args = [f32(pick(rng, [0.1, 0.3, 0.7, 1.5, -2.25, 10.0])) for _ in range(3)]
                    else:
                        args = [f32(pick(rng, [0.1, 0.3, 0.7, 1.5, 12.5, 100.0, 333.3, 0.0001]))]
                        if name in ("AdjusterActiveRadiatorMaxEnergyTransfer", "AdjusterDataTransmitterPower"):
                            args = [float(pick(rng, [0.1, 0.3, 0.7, 1.5, 12.5, 100.0, 333.3, 1e-05]))]
                        if name == "AdjusterSASServiceLevel":
                            args = [rng.randrange(4)]
                    ops.append({"op": "apply", "args": args, "out": m.apply(args)})
            if name == "FailureLightFlickering":
                scen = {"module": "ModuleLight", "rnd": f32(pick(rng, [0.1, 0.5, 1.7])), "dt": f32(pick(rng, [0.02, 0.1]))}
                ops.append({"op": "activate", "scen": scen, "out": m.activate(scen)})
                for _ in range(4):
                    args = [f32(pick(rng, [0.3, 1.0, 25.0])), f32(pick(rng, [0.1, 0.5, 1.7, 3.0])), f32(pick(rng, [0.02, 0.1, 0.5, 2.0]))]
                    ops.append({"op": "apply", "args": args, "out": m.apply(args)})
            # Activate / Deactivate scenarios
            if hasattr(m, "activate") and name != "FailureLightFlickering":
                mods = [getattr(m, "MODULE", None), None, "ModuleOther"] if getattr(m, "MODULE", None) else [None]
                for mod in mods:
                    scen = {"module": mod, "queries": {"IsCooling": rng.randrange(2) == 1, "IsBusy": rng.randrange(2) == 1,
                                                        "allowShutdown": rng.randrange(2) == 1, "HasThisDialogOpen": rng.randrange(2) == 1},
                            "text": {"partTypeName": "Cargo bay"}, "torque": [f32(pick(rng, [0.0, 0.5, 1.25])) for _ in range(3)]}
                    mm = cls(ctor=ctor)
                    if with_cfg:
                        mm.load(cfg)
                    out = mm.activate(scen)
                    out["stuckState"] = mm.vals.get("stuckState")
                    ops.append({"op": "activate", "scen": scen, "out": out, "fresh": True})
            elif hasattr(m, "status_text") and plan_has_status(p):
                mm = cls(ctor=ctor)
                if with_cfg:
                    mm.load(cfg)
                ops.append({"op": "activate", "scen": {"module": None}, "out": {"status": mm.status_text(), "calls": [], "stuckState": mm.vals.get("stuckState")},
                            "fresh": True})
            if hasattr(m, "deactivate"):
                for mod in ([m.MODULE, None] if getattr(m, "MODULE", None) else [None]):
                    ops.append({"op": "deactivate", "scen": {"module": mod}, "out": m.deactivate({"module": mod})})
            # Is* flags of the base classes
            ops += is_flag_ops(cls)
            cases.append({"id": "%s#%d" % (name, k), "kind": "dynamic", "name": name, "ctor": ctor, "cfg": cfg.to_json() if with_cfg else None, "ops": ops})
    return cases


def plan_has_status(p):
    return p.status is not None


def is_flag_ops(cls):
    """The Is* virtuals of the class's base: true when the class overrides with `return true`."""
    p = ref_adjusters.plan(cls.NAME)
    base = re.search(r"class %s\s*:\s*(\w+)" % cls.NAME, p.src).group(1)
    bsrc = ref_adjusters.read_cs(base)
    out = []
    for fn in re.findall(r"public virtual bool (Is\w+)\(\)", bsrc):
        m = re.search(r"public override bool %s\(\)\s*\{\s*return (true|false);" % fn, p.src)
        out.append({"op": "flag", "fn": fn, "out": bool(m and m.group(1) == "true")})
    return out


def main():
    global RESOURCE_DEFS
    RESOURCE_DEFS.update(parse_resource_defs())
    ref_tests.RESOURCE_DEFS = RESOURCE_DEFS
    rng = random.Random(55)
    data = {"version": 1, "registry": registry_section(), "scoreAttr": score_attr_table(), "resourceDefs": RESOURCE_DEFS}
    data["tests"] = gen_tests(rng)
    data["actions"] = gen_actions(rng)
    data["adjusters"] = gen_adjusters(rng)
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, separators=(",", ":"))
    n = sum(len(c["ops"]) for k in ("tests", "actions", "adjusters") for c in data[k])
    print("wrote %s: %d cases, %d ops" % (OUT, sum(len(data[k]) for k in ("tests", "actions", "adjusters")), n))


if __name__ == "__main__":
    main()
