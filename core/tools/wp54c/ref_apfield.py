"""WP-54c reference, part 6: the real BaseAPField (Editor/BaseAPField.cs, BaseField.cs BaseField<K>, BaseFieldList.cs BaseFieldList<R,K>, BaseAPFieldList.cs, MEGUI_Control.cs,
FieldAttribute.cs). Written from the C#. The reflection is replaced on both sides by a scripted host: typed members (fields / properties with throwing setters), callbacks
resolved by name (the method table), a MissionEditorValidator in a chosen mode. No kspcore code."""
import copy

from ref_c_common import *  # noqa: F401,F403
from ref_c_common import Cfg, fmt, rng_pick

TYPES = ["int", "double", "string", "bool"]
TAGS = ["#autoLOC_8004190", "#autoLOC_8000001", "#autoLOC_8100310", "", "plain text"]
HIDE = ["hideWhenSiblingsExist", "hideWhenStartNode", "hideWhenDocked", "hideWhenInputConnected", "hideWhenOutputConnected", "hideWhenNoTestModules", "hideWhenNoActionModules",
        "hideOnSetup"]
VALUES = {"int": [0, 1, -7, 42], "double": [0.0, 1.5, -2.25, 1e6], "string": ["", "a", "bad", "text"], "bool": [True, False]}


def typed(t, v):
    return {"t": t, "v": v}


def same_type(t, value_t):
    return t == value_t


class Mem:
    def __init__(self, spec):
        self.spec = spec
        self.name = spec["name"]
        self.kind = spec["kind"]
        self.type = spec["type"]
        self.value = spec["init"]
        self.original = None
        self.attr = spec["attr"]


class FieldM:
    """BaseAPField as the C# builds it"""

    def __init__(self, mem, methods, host):
        self.mem = mem
        self.host = host
        a = mem.attr
        self.guiName = fmt(fmt(a["guiName"]) if a["guiName"] != "" else mem.name)
        self.is_property = mem.kind == "property"
        self.has_interface = (not self.is_property) and mem.spec["iconfig"]
        # attribute guiName / Tooltip are localized when assigned
        self.tooltip = fmt(a["tooltip"]) if a["tooltip"] != "" else ""
        self.ovc = self.find(methods, a["onValueChange"], "value") if a["onValueChange"] != "" else None
        self.occ = self.find(methods, a["onControlCreated"], "control") if a["onControlCreated"] != "" else None
        self.ocsc = self.find(methods, a["onControlSetupComplete"], "control") if a["onControlSetupComplete"] not in (None, "") else None
        self.cmp = None
        if a["compare"] not in (None, ""):
            m = self.find(methods, a["compare"], "compare")
            if m is not None and m["returnsBool"]:
                self.cmp = m
        self.modified = 0       # OnValueModified fires (a counter handler is subscribed by the test)
        self.set_original()

    def find(self, methods, name, kind):
        m = methods.get(name)
        if m is None or m["kind"] != kind:
            return None
        if kind in ("value", "compare") and m["param"] != self.mem.type:
            return None
        return m

    # BaseField<K>.SetValue(newValue, host)
    def base_set(self, value, log):
        try:
            if self.is_property:
                raise KeyError("NRE")
            if value["t"] != self.mem.type:
                raise TypeError("ArgumentException")
            self.mem.value = value["v"]
            self.modified += 1
            return True
        except (KeyError, TypeError):
            return False

    def base_get(self):
        try:
            if self.is_property:
                raise KeyError("NRE")
            return typed(self.mem.type, self.mem.value)
        except KeyError:
            return None

    def prop_set(self, value):
        s = self.mem.spec
        if s.get("readonly") or value["v"] in s.get("throwOn", []) or value["t"] != self.mem.type:
            raise TypeError("setter")
        self.mem.value = value["v"]

    def call_value_method(self, value, log):
        m = self.ovc
        log.append("vm:%s(%s)" % (self.mem.attr["onValueChange"], fmtv(value)))
        if m["throws"]:
            raise RuntimeError("onValueChange")

    def set_value(self, value, log, validator):
        """BaseAPField.SetValue(newValue) -> True / False / "throws" (the field path lets an onValueChange exception escape)"""
        if not self.is_property:
            if self.base_set(value, log):
                if self.ovc is not None:
                    try:
                        self.call_value_method(value, log)
                    except RuntimeError:
                        return "throws"
                validator.run_on_param_change()
                return True
            return False
        try:
            self.prop_set(value)
            if self.ovc is not None:
                self.call_value_method(value, log)
            validator.run_on_param_change()
            return True
        except (TypeError, RuntimeError):
            return False

    def get_value(self):
        if not self.is_property:
            return self.base_get()
        s = self.mem.spec
        if s.get("getThrows"):
            return None
        return typed(self.mem.type, self.mem.value)

    def set_original(self):
        v = typed(self.mem.type, self.mem.value)
        self.original = v

    def props(self):
        a = self.mem.attr
        return {"name": self.mem.name, "guiName": self.guiName, "gap": a["gapDisplay"], "group": a["group"], "groupDisplayName": a["groupDisplayName"], "order": a["order"],
                "tabStop": a["tabStop"], "hide": [a[h] for h in HIDE], "groupStartCollapsed": a["groupStartCollapsed"], "hasOvc": self.ovc is not None,
                "hasOcc": self.occ is not None, "hasOcsc": self.ocsc is not None, "hasCmp": self.cmp is not None,
                "fieldID": self.mem.spec["declaring"] + "." + self.mem.name, "fieldType": self.mem.type, "hasInterface": self.has_interface, "isProperty": self.is_property,
                "original": self.original, "canBeReset": a["canBeReset"], "canBePinned": a["canBePinned"], "tooltip": self.tooltip, "resetValue": a["resetValue"],
                "checkpoint": a["checkpoint"]}


def fmtv(v):
    x = v["v"]
    if isinstance(x, bool):
        return "True" if x else "False"
    if isinstance(x, float):
        return "%.15g" % x
    return str(x)


class Validator:
    def __init__(self, spec):
        self.spec = spec
        self.runs = 0
        self.validated = spec["validated"] if spec else False

    def run_on_param_change(self):
        s = self.spec
        if s is None:
            return
        mode = s["mode"]
        if mode in ("AutoAfterRun", "FullAuto") and (mode != "AutoAfterRun" or self.validated):
            self.runs += 1
            self.validated = True


def gen_member(r, i):
    t = rng_pick(r, TYPES)
    kind = rng_pick(r, ["field", "field", "property"])
    a = {"guiName": rng_pick(r, TAGS), "tooltip": rng_pick(r, ["", "#autoLOC_8100310", "tip"]), "gapDisplay": r.random() < 0.3, "onValueChange": rng_pick(r, ["", "ovc_ok", "ovc_throw", "ovc_missing", "ovc_wrongtype"]),
         "onControlCreated": rng_pick(r, ["", "ctl_a", "ctl_missing"]), "onControlSetupComplete": rng_pick(r, [None, "", "ctl_a", "ctl_b"]), "group": rng_pick(r, ["", "G1", "G2"]),
         "groupDisplayName": rng_pick(r, [None, "Group One"]), "order": rng_pick(r, [-1, 0, 5, 10]), "tabStop": r.random() < 0.3, "groupStartCollapsed": r.random() < 0.3,
         "compare": rng_pick(r, [None, "", "cmp_ok", "cmp_int", "cmp_str"]), "canBeReset": r.random() < 0.8, "canBePinned": r.random() < 0.8, "resetValue": rng_pick(r, [None, "0", "Foo"]),
         "checkpoint": rng_pick(r, ["None", "Equals", "CustomMethod", "Controls"])}
    for h in HIDE:
        a[h] = r.random() < 0.2
    spec = {"name": "m%d" % i, "kind": kind, "type": t, "declaring": "Expansions.Missions.Tests.Host%d" % r.randrange(3), "iconfig": r.random() < 0.3, "init": rng_pick(r, VALUES[t]), "attr": a}
    if kind == "property":
        spec["readonly"] = r.random() < 0.2
        spec["throwOn"] = [rng_pick(r, VALUES[t])] if r.random() < 0.4 else []
        spec["getThrows"] = False       # a throwing getter would make SetOriginalValue (and so the list constructor) throw
    return spec


def methods_table(r, members):
    m = {}
    for name in ("ovc_ok", "ovc_throw"):
        m[name] = {"kind": "value", "param": rng_pick(r, TYPES), "throws": name == "ovc_throw"}
    m["ovc_wrongtype"] = {"kind": "control", "param": "int", "throws": False}      # exists, but is no value method
    m["ctl_a"] = {"kind": "control", "param": "", "throws": False}
    m["cmp_ok"] = {"kind": "compare", "param": rng_pick(r, TYPES), "returnsBool": True, "result": True}
    m["cmp_int"] = {"kind": "compare", "param": "int", "returnsBool": False, "result": False}
    m["cmp_str"] = {"kind": "compare", "param": "string", "returnsBool": True, "result": False}
    # make some of the value methods match the member types so that the callbacks are found often
    ts = [mm["type"] for mm in members]
    if ts:
        m["ovc_ok"]["param"] = rng_pick(r, ts)
        m["ovc_throw"]["param"] = rng_pick(r, ts)
        m["cmp_ok"]["param"] = rng_pick(r, ts)
    return m


def gen_case(r):
    n = r.randrange(0, 6)
    members = [gen_member(r, i) for i in range(n)]
    methods = methods_table(r, members)
    vspec = rng_pick(r, [None, {"mode": "FullAuto", "validated": True}, {"mode": "AutoAfterRun", "validated": True}, {"mode": "AutoAfterRun", "validated": False}, {"mode": "Manual", "validated": True}])
    host_null = r.random() < 0.05
    mems = [Mem(s) for s in members]
    validator = Validator(vspec)
    log = []
    fields = [FieldM(m, methods, None) for m in mems]
    ops = []
    if host_null:
        ops.append({"op": "countThrows"})
        return {"kind": "apfield", "members": members, "methods": methods, "validator": vspec, "hostNull": True, "ops": ops}
    ops.append({"op": "props", "out": [f.props() for f in fields]})
    ops.append({"op": "count", "out": len(fields)})
    for k in range(r.randrange(6, 16)):
        t = rng_pick(r, ["set", "set", "listSet", "get", "listGet", "orig", "index", "ctl", "cmp", "display"])
        if not fields:
            t = "index"
        f = rng_pick(r, fields) if fields else None
        if t == "set":
            val = typed(f.mem.type if r.random() < 0.8 else rng_pick(r, TYPES), None)
            val["v"] = rng_pick(r, VALUES[val["t"]])
            log.clear()
            before = validator.runs
            res = f.set_value(val, log, validator)
            ops.append({"op": "set", "name": f.mem.name, "value": val, "out": res, "log": list(log), "validations": validator.runs - before, "after": f.get_value(), "modified": f.modified})
        elif t == "listSet":
            val = typed(f.mem.type, rng_pick(r, VALUES[f.mem.type]))
            res = f.base_set(val, log)
            ops.append({"op": "listSet", "name": f.mem.name, "value": val, "out": res, "after": f.get_value(), "modified": f.modified})
        elif t == "get":
            ops.append({"op": "get", "name": f.mem.name, "out": f.get_value()})
        elif t == "listGet":
            ops.append({"op": "listGet", "name": f.mem.name, "out": f.base_get()})
        elif t == "orig":
            for ff in fields:
                ff.set_original()
            ops.append({"op": "orig", "out": [ff.original for ff in fields]})
        elif t == "index":
            ops.append({"op": "index", "name": "nothing", "out": None})
        elif t == "ctl":
            log.clear()
            if f.occ is not None:
                log.append("ctl:%s" % f.mem.attr["onControlCreated"])
            if f.ocsc is not None:
                log.append("ctl:%s" % f.mem.attr["onControlSetupComplete"])
            ops.append({"op": "ctl", "name": f.mem.name, "out": list(log)})
        elif t == "cmp":
            if f.cmp is not None:
                v = typed(f.mem.type, rng_pick(r, VALUES[f.mem.type]))
                ops.append({"op": "cmp", "name": f.mem.name, "value": v, "out": f.cmp["result"]})
            else:
                ops.append({"op": "cmp", "name": f.mem.name, "value": typed("int", 0), "out": None})
        elif t == "display":
            v = f.get_value()
            out = None
            if v is not None:
                out = ("True" if v["v"] else "False") if f.mem.type == "bool" and False else None
            ops.append({"op": "display", "name": f.mem.name, "out": display_text(f)})
    return {"kind": "apfield", "members": members, "methods": methods, "validator": vspec, "hostNull": False, "ops": ops}


def display_text(f):
    """MissionsUtils.GetFieldValueForDisplay: bool -> localized True / False; otherwise ToString() (null -> exception)"""
    v = f.get_value()
    if f.mem.type == "bool":
        if v is None:
            return "throws"
        return fmt("#autoLOC_439839") if v["v"] else fmt("#autoLOC_439840")
    if v is None:
        return "throws"
    x = v["v"]
    if isinstance(x, float):
        return ("%.15g" % x)
    return str(x)


def generate(r):
    return [gen_case(r) for _ in range(250)]
