"""WP-55 reference, part 1: helpers, the ConfigNode model, the generic Load / Save machinery (see node_library_reference.py)."""
import importlib.util
import json
import os
import random
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(CORE, "tests", "data", "wp55_reference.json")


def _find_cs_root():
    for base in (os.path.join(CORE, ".."), os.path.join(CORE, "..", "..", ".."), os.path.join(CORE, "..", "..", "..", "..")):
        p = os.path.abspath(os.path.join(base, "decompiled", "KSP-Decompiled", "Assembly-CSharp"))
        if os.path.isdir(p):
            return p
    return r"D:\Projects\KSP-UE5\decompiled\KSP-Decompiled\Assembly-CSharp"


CS_ROOT = _find_cs_root()
MISSIONS = os.path.join(CS_ROOT, "Expansions", "Missions")

# helpers shared with the WP-54 reference (number text, Localizer, ConfigNode dump); no kspcore code
_spec = importlib.util.spec_from_file_location("wp54ref", os.path.join(CORE, "tools", "wp54", "mission_reference.py"))
r54 = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(r54)
f32, fnum, dnum, fmt, tag = r54.f32, r54.fnum, r54.dnum, r54.fmt, r54.tag
f32bits = r54.f32bits


def dbits(x):
    return "0x%016x" % struct.unpack("<Q", struct.pack("<d", x))[0]

CN = r54.CN


# ---------------------------------------------------------------------------------------------------------------------------
# value text of AddValue(...)
def vtext(v, typ):
    if typ == "double":
        return dnum(v)
    if typ == "float":
        return fnum(v)
    if typ in ("int", "uint"):
        return str(int(v))
    if typ == "bool":
        return "True" if v else "False"
    return str(v)


def parse(text, typ):
    """TryGetValue parse of a clean text; None when it does not parse."""
    try:
        if typ == "double":
            return float(text)
        if typ == "float":
            return f32(float(text))
        if typ in ("int", "uint"):
            return int(text)
        if typ == "bool":
            t = text.strip().lower()
            if t == "true":
                return True
            if t == "false":
                return False
            return None
    except ValueError:
        return None
    return text


class Cfg:
    """A ConfigNode: ordered values and child nodes."""

    def __init__(self, name="", values=None, nodes=None):
        self.name = name
        self.values = list(values or [])
        self.nodes = list(nodes or [])

    def get(self, key):
        for k, v in self.values:
            if k == key:
                return v
        return None

    def get_all(self, key):
        return [v for k, v in self.values if k == key]

    def to_json(self):
        return {"name": self.name, "values": [[k, v] for k, v in self.values], "nodes": [n.to_json() for n in self.nodes]}

    def dump(self):
        s = self.name + "{" + "".join("%s=%s;" % kv for kv in self.values)
        for n in self.nodes:
            s += n.dump()
        return s + "}"


# ---------------------------------------------------------------------------------------------------------------------------
# enums (declaration order = parse table)
ENUMS = {
    "Cmp5": ["LessThan", "LessThanorEqual", "Equal", "GreaterThanorEqual", "GreaterThan"],
    "LG": ["LessThan", "GreaterThan"],
    "VesselType": ["All", "PlayerControlled", "Asteroids", "Comets", "Unowned"],
    "VesselState": ["INACTIVE", "ACTIVE", "DEAD"],
    "FundsMod": ["Add", "Multiply", "Divide", "Substract", "Set"],
    "ScreenStyle": ["UPPER_CENTER", "UPPER_LEFT", "UPPER_RIGHT", "LOWER_CENTER", "KERBAL_EVA"],
    "DialogArea": ["Left", "Center", "Right"],
    "CamMode": ["NoChange", "Flight", "IVA"],
    "CamLock": ["NoChange", "LockAllowMap", "LockDisableMap", "Unlock"],
    "ActionGroup": ["REPLACEWITHDEFAULT", "None", "Stage", "Gear", "Light", "RCS", "SAS", "Brakes", "Abort", "Custom01", "Custom02", "Custom03",
                    "Custom04", "Custom05", "Custom06", "Custom07", "Custom08", "Custom09", "Custom10"],
    "Act": ["CurrentState", "On", "Off"],
    "Dep": ["CurrentState", "Deployed", "Retracted"],
    "Open": ["CurrentState", "Open", "Closed"],
}


def cmp5(op, a, b):
    return {"LessThan": a < b, "LessThanorEqual": a <= b, "Equal": a == b, "GreaterThanorEqual": a >= b, "GreaterThan": a > b}.get(op, False)


def opstr(op):
    return {"LessThan": "<", "LessThanorEqual": "<=", "Equal": "=", "GreaterThanorEqual": ">=", "GreaterThan": ">"}.get(op, "")


def lg_prefix(op):
    return "<" if op == "LessThan" else ">"


# ---------------------------------------------------------------------------------------------------------------------------
# generic Load / Save driven by the field tables written from each class's C# (key, attribute, type[, enum, default])
class Mod:
    KIND = "test"
    NAME = ""
    TITLE = None            # Awake title (a tag / literal, already the final text after Localizer.Format where the C# formats it)
    TITLE_FMT = True        # True: Localizer.Format(tag); False: literal (the C# assigns a raw string / tag)
    INFO = None
    SAVE = []               # (key, attr, type, enum)   type in double,float,int,uint,bool,string,enum,guid
    LOAD = []               # (key, attr, type, enum, default)
    BASE = "test"           # test | action | dynamic

    def __init__(self):
        self.title = ""
        self.sap = []
        self.restart = False
        self.running = False
        self.awake()

    def awake(self):
        if self.TITLE is not None:
            self.title = fmt(self.TITLE) if self.TITLE_FMT else self.TITLE

    # --- base Load / Save (TestModule.cs:314 / :320, ActionModule.cs:267 / :276)
    def load(self, cfg):
        if self.BASE == "test":
            t = cfg.get("title")
            if t is not None:
                self.title = t
            self.sap = cfg.get_all("parametersDisplayedInSAP")
        elif self.BASE == "action":
            t = cfg.get("title")
            if t is not None:
                self.title = t
            self.sap = cfg.get_all("parametersDisplayedInSAP")
            v = parse(cfg.get("restartOnSceneLoad") or "", "bool")
            if v is not None:
                self.restart = v
            v = parse(cfg.get("isRunning") or "", "bool")
            if v is not None:
                self.running = v
        self.load_fields(cfg)

    def load_fields(self, cfg):
        for key, attr, typ, enum, default in self.LOAD:
            text = cfg.get(key)
            if typ == "enum":
                setattr(self, attr, text if (text is not None and text in ENUMS[enum]) else default)
            else:
                if text is None:
                    continue
                v = parse(text, typ)
                if v is not None:
                    setattr(self, attr, v)

    def save(self):
        cn = Cfg("N")
        if self.BASE == "test":
            cn.values += [("name", self.NAME), ("title", self.title)]
            cn.values += [("parametersDisplayedInSAP", s) for s in self.sap]
        elif self.BASE == "action":
            cn.values += [("name", self.NAME), ("title", self.title)]
            cn.values += [("parametersDisplayedInSAP", s) for s in self.sap]
            cn.values += [("restartOnSceneLoad", vtext(self.restart, "bool")), ("isRunning", vtext(self.running, "bool"))]
        self.save_fields(cn)
        return cn

    def save_fields(self, cn):
        for key, attr, typ, enum in self.SAVE:
            cn.values.append((key, vtext(getattr(self, attr), "string" if typ == "enum" else typ)))

    def display_name(self):
        return fmt(self.title)

    def info(self):
        return fmt(self.INFO) if self.INFO else self.NAME

    def body(self, field, env=None):
        return self.base_body(field)

    def base_body(self, field):
        """TestModule / ActionModule / DynamicModule base GetNodeBodyParameterString for a plain value field."""
        if self.BASE == "action":
            text = tag("#autoLOC_6003083")
        elif self.BASE == "dynamic":
            text = "None"
        else:
            text = tag("#autoLOC_6003000")
        if field["has"]:
            text = field["value"]
        return fmt("#autoLOC_8004190", field["guiName"], text)


TESTS = {}
ACTIONS = {}
DYNAMICS = {}


def test_class(cls):
    cls.KIND = "test"
    cls.BASE = "test"
    TESTS[cls.NAME] = cls
    return cls


def action_class(cls):
    cls.KIND = "action"
    cls.BASE = "action"
    ACTIONS[cls.NAME] = cls
    return cls


def dynamic_class(cls):
    cls.KIND = "dynamic"
    cls.BASE = "dynamic"
    DYNAMICS[cls.NAME] = cls
    return cls
