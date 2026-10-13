"""WP-54c reference, part 2: ActionPartFailure, ActionPartRepair, ActionPartResourceDrain (Expansions/Missions/Actions/*.cs) and the proto-part config
manipulations they use (ProtoPartSnapshot.cs / ProtoPartModuleSnapshot.cs adjuster bookkeeping). Written from the C#; scenarios are labelled synthetic
(a seeded vessel world; the PartModule side of the adjusters is a recording seam on both sides). No kspcore code.

Float rule: C# float arithmetic is evaluated in double and rounded at a store (f32)."""
import math
import os
import re

from ref_c_common import *  # noqa: F401,F403
from ref_c_common import (Cfg, CfgR, f32, fmt, tag, b, guid_of, BODIES, HOME, body, D, F, rng_pick, EMPTY_GUID, MissionModel, MappedVessel, mv_convert, TICK,
                          CORE)
from ref_actions2 import AM, node_of
from ref_tests2 import gen_world, active, vessel_by_pid, vtext, KERBALS
from ref_tests3 import parse_bool
import ref_adjusters  # noqa: F401  (registers the 39 adjuster models in ref_base.DYNAMICS)
from ref_base import DYNAMICS, MISSIONS

ADJ_DIR = os.path.join(MISSIONS, "Adjusters")
MODULE_POOL = ["ModuleEngines", "ModuleLight", "ModuleRCS", "ModuleGimbal", "ModuleCommand", "ModuleGenerator"]
ADJ_POOL = ["AdjusterEnginesThrottle", "FailureEnginesDeadEngine", "FailureLightStuck", "AdjusterLightIntensity", "FailureRCSStopWorking", "AdjusterGimbalLimitControl",
            "AdjusterGeneratorEfficiency", "FailureRCSFireRandomly"]
ADJ_IDS = [guid_of(300 + i) for i in range(6)]
RESOURCES = ["LiquidFuel", "Oxidizer", "ElectricCharge", "MonoPropellant", "Ore"]
FAIL_STATES = [0, 1, 2]


def read_cs(name):
    return open(os.path.join(ADJ_DIR, name + ".cs"), encoding="utf-8-sig").read()


# ---- the adjuster type tables (MissionsUtils.InitialiseAdjusterTypes), parsed from the C# sources --------------------------------------
def adjuster_target(cls):
    """GetTargetPartModule() of the class chain (typeof(Module...).Name), None when no class of the chain returns one"""
    seen = 0
    while cls and seen < 8:
        seen += 1
        src = read_cs(cls)
        m = re.search(r"GetTargetPartModule\(\)\s*\{\s*return typeof\((\w+)\);", src)
        if m:
            return m.group(1)
        mb = re.search(r"class %s\s*:\s*(\w+)" % cls, src)
        cls = mb.group(1) if mb else None
    return None


def table_by_module():
    t = {}
    for name in sorted(DYNAMICS):
        tgt = adjuster_target(name)
        if tgt is not None:
            t.setdefault(tgt, []).append(name)
    return t


BY_MODULE = table_by_module()


# ---- ProtoPartModuleSnapshot adjuster bookkeeping ------------------------------------------------------------------------------------
def guid_norm(g):
    return g.strip().lower()


def proto_add_adjuster(mod, adj_cfg):
    """ProtoPartModuleSnapshot.AddPartModuleAdjusterNode: ADJUSTERS node = existing (by reference) or new; ADJUSTERMODULE appended; SetNode(back)"""
    node = mod.get_node("ADJUSTERS") or CfgR("ADJUSTERS")
    n = node.add_node("ADJUSTERMODULE")
    n.values = list(adj_cfg.values)
    n.nodes = [CfgR.from_cfg(x) for x in adj_cfg.nodes]
    mod.set_node("ADJUSTERS", node, True)


def proto_remove_guid(mod, gid):
    node = mod.get_node("ADJUSTERS") or CfgR("")
    nodes = list(node.nodes)
    node2 = mod.get_node("ADJUSTERSTOREMOVE") or CfgR("ADJUSTERSTOREMOVE")
    for n in nodes:
        a = n.get("adjusterID")
        if a is not None and guid_norm(a) == guid_norm(gid):
            node2.add_node_obj(n)
    mod.set_node("ADJUSTERS", node2, True)       # sic: the C# stores node2 under the name ADJUSTERS


def proto_module_repair(mod):
    node = mod.get_node("ADJUSTERS") or CfgR("")
    nodes = list(node.nodes)
    node2 = mod.get_node("ADJUSTERSTOREMOVE") or CfgR("ADJUSTERSTOREMOVE")
    for n in nodes:
        node2.add_node_obj(n)
    mod.set_node("ADJUSTERSTOREMOVE", node2, True)


# ---- VesselPartIDPair -----------------------------------------------------------------------------------------------------------------
class VPair:
    def __init__(self):
        self.vid = 0
        self.pid = 0
        self.pname = "#autoLOC_8001004"

    def load(self, cfg):
        v = cfg.get("partPersistentId")
        if v is not None:
            self.pid = _uint(v, self.pid)
        v = cfg.get("vesselPersistentId")
        if v is not None:
            self.vid = _uint(v, self.vid)
        v = cfg.get("partName")
        if v is not None:
            self.pname = v

    def save(self, out):
        out.values.append(("partPersistentId", str(self.pid)))
        out.values.append(("vesselPersistentId", str(self.vid)))
        out.values.append(("partName", self.pname))

    def validate(self, craft):
        if self.pid == 0:
            return []
        parts = craft.get(str(self.vid))
        if parts is None:
            return []
        if self.pid not in parts:
            return [["fail", fmt("#autoLOC_8002115")]]
        return []


def _uint(text, cur):
    t = text.strip()
    if t.isdigit() and int(t) < 2 ** 32:
        return int(t)
    return cur


def vessel_name_block(m, vp, field_guiname, repair_style):
    """the vessel-name / part text of GetNodeBodyParameterString (Failure / Drain use Format(8004190, ...), Repair string concatenation)"""
    text = ""
    sit = m.vessel_by_id(vp.vid)
    flag = False
    vname = ""
    if sit is not None:
        vname = sit["name"]
    else:
        found, vn, sit2 = m.update_from_mapped(vp.vid)
        if found:
            flag = True
            vname = vn
            sit = sit2
            if sit is None:
                text = (fmt("#autoLOC_8000001") + ": " + fmt(vname) + "\n") if repair_style else (fmt("#autoLOC_8004190", fmt("#autoLOC_8000001"), vname) + "\n")
    if sit is not None:
        text = (fmt("#autoLOC_8000001") + ": " + fmt(vname) + "\n") if repair_style else (fmt("#autoLOC_8004190", fmt("#autoLOC_8000001"), vname) + "\n")
        if flag:
            text = text + fmt("#autoLOC_6002491", sit["name"]) + "\n"
    if repair_style:
        return text + field_guiname + ": " + fmt(vp.pname)
    return text + fmt("#autoLOC_8004190", field_guiname, fmt(vp.pname))


# ---- the adjuster list (DynamicModuleList of adjusters) ----------------------------------------------------------------------------------
class AdjList:
    def __init__(self):
        self.mods = []      # Adj models
        self.supported = []

    def load(self, cfg):
        node = node_of(cfg, "MODULES")
        if node is None:
            return
        self.mods = []
        for mn in node.nodes:
            if mn.name != "MODULE":
                continue
            name = mn.get("name")
            if name is None or name not in DYNAMICS:
                continue
            a = DYNAMICS[name](1)
            a.load(mn)
            self.mods.append(a)

    def save(self, out):
        mn = Cfg("MODULES")
        for a in self.mods:
            mn.nodes.append(a.save_as("MODULE"))
        out.nodes.append(mn)


def adj_save(a):
    c = a.save()
    c.name = "MODULE"
    return c


# monkey-free: give Adj a save_as without touching the wp55 classes
def _save_as(self, name):
    c = self.save()
    c.name = name
    return c


for _cls in DYNAMICS.values():
    _cls.save_as = _save_as


# ---- world enrichment (modules / resources / part states) -------------------------------------------------------------------------------
def adjuster_cfg(r, name=None):
    name = name or rng_pick(r, ADJ_POOL)
    a = DYNAMICS[name](1)
    c = Cfg("MODULE")
    c.values.append(("name", name))
    c.values.append(("id", str(r.randrange(1, 50))))
    c.values.append(("adjusterID", rng_pick(r, ADJ_IDS)))
    c.values.append(("canBeRepaired", rng_pick(r, ["True", "False"])))
    return c


def enrich_world(r, w):
    for v in w["vessels"]:
        for p in v["parts"]:
            mods = []
            for k in range(r.randrange(0, 4)):
                name = rng_pick(r, MODULE_POOL)
                m = {"name": name, "ids": [g for g in ADJ_IDS if r.random() < 0.3]}
                # the proto MODULE node (unloaded vessels): name, a persisted ADJUSTERS and ADJUSTERSTOREMOVE sometimes
                mc = CfgR("MODULE", [("name", name)], [])
                if r.random() < 0.4:
                    ad = mc.add_node("ADJUSTERS")
                    for _ in range(r.randrange(1, 3)):
                        x = adjuster_cfg(r)
                        ad.add_node_obj(CfgR.from_cfg(x)).name = "ADJUSTERMODULE"
                if r.random() < 0.25:
                    tr = mc.add_node("ADJUSTERSTOREMOVE")
                    x = adjuster_cfg(r)
                    tr.add_node_obj(CfgR.from_cfg(x)).name = "ADJUSTERMODULE"
                m["cfg"] = mc.to_json()
                mods.append(m)
            p["modules"] = mods
            p["resources"] = [{"name": rng_pick(r, RESOURCES), "amount": rng_pick(r, [0.0, 5.0, 12.25, 100.0, 375.5]), "max": 400.0} for _ in range(r.randrange(0, 3))]
            p["state"] = rng_pick(r, FAIL_STATES)
            p["pre"] = rng_pick(r, FAIL_STATES)
    return w


def part_pids(v):
    return [p["pid"] for p in v["parts"]]


def find_loaded_part(w, pid):
    if pid == 0:
        return None
    for v in w["vessels"]:
        if v["loaded"]:
            for p in v["parts"]:
                if p["pid"] == pid:
                    return v, p
    return None


def find_unloaded_part(w, pid):
    if pid == 0:
        return None
    for v in w["vessels"]:
        if not v["loaded"]:
            for p in v["parts"]:
                if p["pid"] == pid:
                    return v, p
    return None


_PROTO = {}


class ProtoState:
    """the ProtoPart config state of the unloaded vessels of a world (modules as CfgR with reference semantics)"""

    def __init__(self, w):
        self.parts = {}     # (vpid, ppid) -> {"state","pre","mods":[CfgR], "res":[[name, amount]]}
        for v in w["vessels"]:
            if v["loaded"]:
                continue
            for p in v["parts"]:
                self.parts[(v["pid"], p["pid"])] = {"state": p["state"], "pre": p["pre"], "mods": [CfgR.from_json(m["cfg"]) for m in p["modules"]],
                                                    "res": [[x["name"], x["amount"]] for x in p["resources"]], "maxres": [x["max"] for x in p["resources"]]}

    def snapshot(self, w):
        out = []
        for v in w["vessels"]:
            if v["loaded"]:
                continue
            for p in v["parts"]:
                s = self.parts[(v["pid"], p["pid"])]
                out.append([v["pid"], p["pid"], s["state"], [m.dump() for m in s["mods"]], s["res"]])
        return out

    def snapshot_part(self, v, p):
        return self.parts[(v["pid"], p["pid"])]


# ---- common action base -------------------------------------------------------------------------------------------------------------------
class PartAction(AM):
    def __init__(self, mission):
        super().__init__()
        self.m = mission
        self.vp = VPair()
        self.yields = 0

    def ids_changed(self, old, new):
        if self.vp.vid == old:
            self.vp.vid = new

    def part_id_changed(self, vid, old, new):
        if self.vp.vid == vid and self.vp.pid == old:
            self.vp.pid = new

    def undocking(self, w, old_pid, new_pid):
        ov, nv = vessel_by_pid(w, old_pid), vessel_by_pid(w, new_pid)
        if self.vp.vid != ov["pid"] and self.vp.vid != nv["pid"]:
            return
        for p in nv["parts"]:
            if self.vp.pid == p["pid"]:
                self.vp.vid = nv["pid"]
                return

    def base_body(self, field):
        text = fmt("#autoLOC_6003083")
        if field["has"]:
            text = field["value"]
        return fmt("#autoLOC_8004190", field["guiName"], text)


# ---- ActionPartFailure ---------------------------------------------------------------------------------------------------------------------
class Failure(PartAction):
    NAME = "ActionPartFailure"
    TITLE = "#autoLOC_8000002"
    INFO = "#autoLOC_8004024"

    def __init__(self, mission):
        super().__init__(mission)
        self.mods = AdjList()

    def load(self, cfg):
        self.load_base(cfg)
        self.vp.load(cfg)
        self.mods.load(cfg)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.vp.save(out)
        self.mods.save(out)
        return out

    def body_text(self, f):
        if f["name"] == "vesselPartIDs":
            return vessel_name_block(self.m, self.vp, f["guiName"], False)
        if f["name"] == "Modules":
            text = ""
            n = 0
            for i, a in enumerate(self.mods.mods):
                text += a.display_name()
                n += 1
                if i != len(self.mods.mods) - 1:
                    text += "\n"
            if n > 0:
                return fmt("#autoLOC_8004190", f["guiName"], text)
            return fmt("#autoLOC_8004190", f["guiName"], fmt("#autoLOC_8100310"))
        return self.base_body(f)

    def fire(self, w):
        calls = []
        y = 0

        def add_list(part, vpid, lst):
            ids = ",".join(a.adjusterID for a in lst)
            calls.append("AddPartModuleAdjusterList(%d,[%s])" % (part["pid"], ids))

        proto = _PROTO[id(w)]
        key = self.m.current_vessel_id(self.vp.vid)
        if self.vp.pid == 0:
            vessel = vessel_by_pid(w, key)
            if vessel is not None:
                for part in vessel["parts"]:
                    lst = []
                    for mod in part["modules"]:
                        tbl = BY_MODULE.get(mod["name"])
                        if tbl is None:
                            continue
                        for a in self.mods.mods:
                            if a.NAME in tbl:
                                lst.append(a)
                    add_list(part, vessel["pid"], lst)
                y += 1
            y += 1
        lp = find_loaded_part(w, self.vp.pid)
        if lp is not None:
            if len(self.mods.mods) > 0:
                add_list(lp[1], lp[0]["pid"], list(self.mods.mods))
        else:
            up = find_unloaded_part(w, self.vp.pid)
            if up is not None:
                if len(self.mods.mods) > 0:
                    st = proto.snapshot_part(*up)
                    for mod_cfg, mod_json in zip(st["mods"], up[1]["modules"]):
                        tbl = BY_MODULE.get(mod_json["name"])
                        if tbl is None:
                            continue
                        for a in self.mods.mods:
                            if a.NAME in tbl:
                                proto_add_adjuster(mod_cfg, a.save_as("ADJUSTERMODULE"))
        y += 1
        return {"yields": y, "waits": [], "calls": calls}

    def supported_types(self, selected_part):
        """OnPartIDChanged: the supported adjuster type list (as a sorted multiset; the table order is the assembly's)"""
        lst = []
        if selected_part is not None:
            for mod in selected_part["modules"]:
                if mod["name"] in BY_MODULE:
                    lst += BY_MODULE[mod["name"]]
        else:
            for k in BY_MODULE:
                lst += BY_MODULE[k]
        return sorted(lst)


# ---- ActionPartRepair ---------------------------------------------------------------------------------------------------------------------------
REPAIR = ["entireVessel", "onePart", "partModule", "failureID"]
REPAIR_TAG = {"entireVessel": "#autoLOC_8100115", "onePart": "#autoLOC_8100116", "partModule": "#autoLOC_8100117", "failureID": "#autoLOC_8100118"}


class Repair(PartAction):
    NAME = "ActionPartRepair"
    TITLE = "#autoLOC_8000063"
    INFO = "#autoLOC_8004025"

    def __init__(self, mission):
        super().__init__(mission)
        self.repair = "entireVessel"
        self.partModule = ""
        self.failureID = EMPTY_GUID
        self.sit = None
        self.playerCreated = False

    def load(self, cfg):
        self.load_base(cfg)
        self.vp.load(cfg)
        v = cfg.get("repairType")
        self.repair = v if v in REPAIR else "entireVessel"
        v = cfg.get("partModule")
        if v is not None:
            self.partModule = v
        v = cfg.get("failureID")
        if v is not None:
            self.failureID = v.strip().lower()
        else:
            self.failureID = None   # Guid.NewGuid() (a fresh id, not comparable): scenarios always give failureID unless the case only checks Save

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.vp.save(out)
        out.values.append(("repairType", self.repair))
        if self.partModule != "None Available":
            out.values.append(("partModule", self.partModule))
        if self.failureID != EMPTY_GUID and self.failureID is not None:
            out.values.append(("failureID", self.failureID))
        return out

    def display(self, e):
        return fmt(REPAIR_TAG[e])

    def body_text(self, f):
        name = f["name"]
        if name == "vesselPartIDs":
            return vessel_name_block(self.m, self.vp, f["guiName"], True)
        if name == "repairType":
            e = self.repair
            if e == "entireVessel" or e not in REPAIR:
                return fmt("#autoLOC_8004190", fmt("#autoLOC_8100122"), fmt("#autoLOC_8100115"))
            if e == "onePart":
                return fmt("#autoLOC_8004190", self.display("onePart"), fmt(self.vp.pname))
            if e == "partModule":
                return fmt("#autoLOC_8004190", self.display("partModule"), self.partModule)
            if e == "failureID":
                for fa in self.m.failures:
                    if fa.vp.vid != self.vp.vid:
                        continue
                    for a in fa.mods.mods:
                        if a.adjusterID.lower() == self.failureID:
                            return fmt("#autoLOC_8004190", fmt("#autoLOC_8100122"), a.display_name())
                # falls out of the switch to the base text below
        if name not in ("partModule", "failureID"):
            return self.base_body(f)
        return ""

    def fire(self, w):
        calls = []
        vessel = vessel_by_pid(w, self.vp.vid)
        proto = _PROTO[id(w)]

        def repair_part_loaded(part, repair_module, use_guid):
            for i, mod in enumerate(part["modules"]):
                if not (repair_module == "") and not (repair_module == mod["name"]):
                    continue
                ids = mod["ids"]
                for num in range(len(ids) - 1, -1, -1):
                    if not use_guid or self.failureID == ids[num].lower():
                        calls.append("RemovePartModuleAdjuster(%d,%d,%s)" % (part["pid"], i, ids[num]))

        def repair_part_proto(v, part, repair_module, use_guid):
            st = proto.snapshot_part(v, part)
            for mod_cfg, mj in zip(st["mods"], part["modules"]):
                if repair_module == "" or repair_module == mj["name"]:
                    if use_guid:
                        proto_remove_guid(mod_cfg, self.failureID)
                    else:
                        proto_module_repair(mod_cfg)

        if vessel is not None:
            if vessel["loaded"]:
                if self.repair == "entireVessel":
                    for part in vessel["parts"]:
                        repair_part_loaded(part, "", False)
                elif self.repair == "onePart":
                    lp = find_loaded_part(w, self.vp.pid)
                    if lp is not None:
                        repair_part_loaded(lp[1], "", False)
                elif self.repair == "partModule":
                    for part in vessel["parts"]:
                        repair_part_loaded(part, self.partModule, False)
                elif self.repair == "failureID":
                    for part in vessel["parts"]:
                        repair_part_loaded(part, "", True)
            else:
                if self.repair == "entireVessel":
                    for part in vessel["parts"]:
                        repair_part_proto(vessel, part, "", False)
                elif self.repair == "onePart":
                    up = find_unloaded_part(w, self.vp.pid)
                    if up is not None:
                        # ProtoPartSnapshot.ProtoPartRepair
                        st = proto.snapshot_part(*up)
                        st["state"] = st["pre"]
                        for mod_cfg in st["mods"]:
                            proto_module_repair(mod_cfg)
                elif self.repair == "partModule":
                    for part in vessel["parts"]:
                        repair_part_proto(vessel, part, self.partModule, False)
                elif self.repair == "failureID":
                    for part in vessel["parts"]:
                        repair_part_proto(vessel, part, "", True)
        return {"yields": 1, "waits": [], "calls": calls}

    # ---- editor drop-down item builders -------------------------------------------------------------------------------------------
    def repair_items(self):
        sit = self.m.vessel_by_id(self.vp.vid)
        player = True if sit is None else sit.get("player", True)
        items = []
        for e in REPAIR:
            if (player or self.vp.pid != 0 or not (e == "onePart")) and (not player or (not (e == "onePart") and not (e == "partModule"))):
                items.append([e, e, self.display(e)])
        return items

    def failure_items(self):
        items = []
        for fa in self.m.failures:
            if fa.vp.vid != self.vp.vid:
                continue
            for a in fa.mods.mods:
                items.append([a.adjusterID, a.adjusterID, a.display_name()])
        if not items:
            items.append(["None Available", EMPTY_GUID, fmt("#autoLOC_8100310")])
        return items


# ---- ActionPartResourceDrain ----------------------------------------------------------------------------------------------------------------------
RES_DISPLAY = {}


def resource_defs():
    if not RES_DISPLAY:
        path = os.path.join(CORE, "testdata", "GameData-mirror", "GameData", "Squad", "Resources", "ResourcesGeneric.cfg")
        cur = None
        for line in open(path, encoding="utf-8-sig"):
            mm = re.match(r"\s*name\s*=\s*(\S+)", line)
            if mm:
                cur = mm.group(1)
                RES_DISPLAY[cur] = cur
            md = re.match(r"\s*displayName\s*=\s*(\S+)", line)
            if md and cur:
                RES_DISPLAY[cur] = md.group(1)
    return RES_DISPLAY


def resource_display(name):
    d = resource_defs()
    return fmt(d[name]) if name in d else None


class Drain(PartAction):
    NAME = "ActionPartResourceDrain"
    TITLE = "#autoLOC_8000007"
    INFO = "#autoLOC_8004004"
    EVERY = 0.5

    def __init__(self, mission):
        super().__init__(mission)
        self.restart = True
        self.timePeriod = 0.0
        self.amount = 0.0
        self.resource = ""
        self.left = 0.0
        # run-time
        self.setup = False
        self.vesselDrain = False
        self.drainPerSecond = 0.0
        self.resdef = False       # `resourcedef != null` (set by Setup and by GetNodeBodyParameterString)

    def load(self, cfg):
        self.load_base(cfg)
        for key, attr in (("timePeriod", "timePeriod"), ("amountToDrain", "amount"), ("leftToDrain", "left")):
            v = cfg.get(key)
            if v is not None:
                try:
                    setattr(self, attr, f32(float(v)))
                except ValueError:
                    pass
        v = cfg.get("resourceName")
        if v is not None:
            self.resource = v
        self.vp.load(cfg)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        out.values.append(("timePeriod", F(self.timePeriod)))
        out.values.append(("amountToDrain", F(self.amount)))
        out.values.append(("resourceName", self.resource))
        out.values.append(("leftToDrain", F(self.left)))
        self.vp.save(out)
        return out

    def body_text(self, f):
        if f["name"] == "vesselPartIDs":
            return vessel_name_block(self.m, self.vp, f["guiName"], False)
        if f["name"] == "resourceName":
            text2 = fmt("#autoLOC_6003083")
            if self.resource != "":
                dn = resource_display(self.resource)
                self.resdef = dn is not None     # resourcedef = PartResourceLibrary.Instance.GetDefinition(resourceName)
                if dn is not None:
                    text2 = dn
            return fmt("#autoLOC_8004190", f["guiName"], text2)
        return self.base_body(f)

    def fire(self, w, req_v, req_p):
        """the coroutine run to its end: returns waits, requests, events"""
        waits, requests, events = [], [], []
        yields = 0
        proto = _PROTO[id(w)]
        st = {"vessel": None, "part": None, "proto": None, "partLoaded": False}
        self.running = True

        def find_vessel():
            key = self.m.current_vessel_id(self.vp.vid)
            v = vessel_by_pid(w, key)
            if v is not None:
                st["vessel"] = v
            return v is not None

        def find_part():
            lp = find_loaded_part(w, self.vp.pid)
            st["partLoaded"] = lp is not None
            if lp is not None:
                st["part"] = lp
                return True
            up = find_unloaded_part(w, self.vp.pid)
            if up is not None:
                st["proto"] = up
                return True
            return False

        def setup():
            if self.vp.vid != 0 and self.amount != 0.0 and self.resource != "":
                if self.vp.vid != 0 and self.vp.pid == 0:
                    self.vesselDrain = True
                    if not find_vessel():
                        return False
                else:
                    self.vesselDrain = False
                    if not find_part():
                        return False
                self.resdef = resource_display(self.resource) is not None
                if not self.resdef:
                    return False
                if self.timePeriod <= 0.0:
                    self.drainPerSecond = self.amount
                else:
                    self.drainPerSecond = f32(self.amount / self.timePeriod)
                self.left = self.amount
                return True
            return False

        def drain_unloaded(v, part):
            num = f32(self.drainPerSecond * self.EVERY)
            s = proto.snapshot_part(v, part)
            for k, (rn, amt) in enumerate(s["res"]):
                if rn == self.resource:
                    if amt >= num:
                        s["res"][k][1] = amt - num
                        return num
                    return 0.0
            return 0.0

        def next_req(seq):
            return seq.pop(0) if seq else 0.0

        if not self.setup:
            if not setup():
                self.running = False
                yields += 1                # `yield return null` (continues)
            self.setup = True
        guard = 0
        try:
            while guard < 400:
                guard += 1
                if self.vesselDrain:
                    if st["vessel"] is None:
                        if not find_vessel():
                            return {"yields": yields, "waits": waits, "requests": requests, "events": events}
                    self.drain_vessel(st, w, proto, requests, events, req_v, drain_unloaded)
                else:
                    if (st["partLoaded"] and st["part"] is None) or (not st["partLoaded"] and st["proto"] is None):
                        if not find_part():
                            break
                    self.drain_part(st, w, proto, requests, events, req_p, drain_unloaded)
                if self.left > 1e-15:
                    waits.append("0.5")
                if not (self.left > 1e-15):
                    self.running = False
                    return {"yields": yields, "waits": waits, "requests": requests, "events": events}
        except KeyError:
            return {"yields": yields, "waits": waits, "requests": requests, "events": events, "throws": True}
        return {"yields": yields, "waits": waits, "requests": requests, "events": events}

    def drain_vessel(self, st, w, proto, requests, events, req_v, drain_unloaded):
        num = f32(self.drainPerSecond * self.EVERY)
        v = st["vessel"]
        if v["loaded"]:
            if not self.resdef:
                raise KeyError("NRE")      # resourcedef.id
            n2 = req_v.pop(0) if req_v else 0.0
            requests.append(["vessel", num])
            if n2 <= 0.0:
                self.left = 0.0
            else:
                events.append([v["pid"], v["parts"][0]["pid"] if v["parts"] else None, None])
            self.left = f32(self.left - f32(n2))
            return
        num3 = 0.0
        for part in v["parts"]:
            num4 = drain_unloaded(v, part)
            if not (num4 <= 0.0):
                num3 = f32(num3 + num4)
                break
        if num3 <= 0.0:
            self.left = 0.0
        self.left = f32(self.left - num3)

    def drain_part(self, st, w, proto, requests, events, req_p, drain_unloaded):
        num = f32(self.drainPerSecond * self.EVERY)
        if st["partLoaded"]:
            v, part = st["part"]
            if not self.resdef:
                raise KeyError("NRE")      # resourcedef.id
            n2 = req_p.pop(0) if req_p else 0.0
            requests.append(["part", num])
            if n2 <= 0.0:
                self.left = 0.0
            else:
                events.append([st["vessel"]["pid"] if st["vessel"] is not None else None, part["pid"], None])
            self.left = f32(self.left - f32(n2))
        else:
            v, part = st["proto"]
            num3 = drain_unloaded(v, part)
            if num3 <= 0.0:
                self.left = 0.0
            else:
                events.append([st["vessel"]["pid"] if st["vessel"] is not None else None, None, part["pid"]])
            self.left = f32(self.left - num3)


# ---- scenario generation --------------------------------------------------------------------------------------------------------------------------
FIELDS = {
    "ActionPartFailure": [{"name": "vesselPartIDs", "guiName": "Part"}, {"name": "Modules", "guiName": "Failures"}, {"name": "other", "guiName": "Other"}],
    "ActionPartRepair": [{"name": "vesselPartIDs", "guiName": "Part"}, {"name": "repairType", "guiName": "Repair"}, {"name": "partModule", "guiName": "Module"},
                         {"name": "failureID", "guiName": "Failure"}, {"name": "other", "guiName": "Other"}],
    "ActionPartResourceDrain": [{"name": "vesselPartIDs", "guiName": "Part"}, {"name": "resourceName", "guiName": "Resource"}, {"name": "other", "guiName": "Other"}],
}


def mission_for(r, w, failures=None):
    m = MissionModel()
    for i, v in enumerate(w["vessels"]):
        if r.random() < 0.5:
            m.vessels.append({"pid": v["pid"], "name": rng_pick(r, ["Alpha", "Bravo", "#autoLOC_8100086"]), "craft": "", "node": i, "player": r.random() < 0.5})
    for i in range(r.randrange(0, 3)):
        m.mapped.append(MappedVessel(5000 + i, 100 + 7 * i, rng_pick(r, [100, 107, 114, 999]), rng_pick(r, ["Booster", "Lander"]), "c%d.craft" % i, "", 700 + i))
    m.failures = failures or []
    return m


def mission_spec(m):
    return {"vessels": [{k: v for k, v in x.items()} for x in m.vessels], "mapped": [mm.to_json() for mm in m.mapped], "flags": [], "asteroids": [], "comets": []}


def body_ops_for(a, cls):
    ops = []
    for f in FIELDS[cls]:
        f = dict(f)
        f["value"] = "x"
        f["has"] = True
        ops.append({"op": "body", "field": f, "out": a.body_text(f)})
    return ops


def head(a):
    return [{"op": "title", "out": a.title}, {"op": "info", "out": a.info()}, {"op": "saved", "out": a.save().dump()}]


def cfg_dict(cfg):
    return {k: v for k, v in cfg.values}


def gen_cfg_vp(r, cfg, w):
    pids = [v["pid"] for v in w["vessels"]]
    ppids = [p["pid"] for v in w["vessels"] for p in v["parts"]]
    if r.random() < 0.85:
        cfg.values.append(("vesselPersistentId", str(rng_pick(r, pids + [0, 999]))))
    if r.random() < 0.7:
        cfg.values.append(("partPersistentId", str(rng_pick(r, ppids + [0, 0, 123456]))))
    if r.random() < 0.5:
        cfg.values.append(("partName", rng_pick(r, ["#autoLOC_8001004", "Mk1 Pod", "Engine"])))


def new_world(r, cfg=None):
    w = gen_world(r)
    enrich_world(r, w)
    if cfg is not None:
        bias_world(r, w, cfg)
    return w


def bias_world(r, w, cfg):
    """give the world the vessel / part ids the configuration refers to (otherwise most fires would not find their target)"""
    vid = int(cfg.get("vesselPersistentId") or 0)
    pid = int(cfg.get("partPersistentId") or 0)
    if r.random() < 0.15 or not w["vessels"]:
        return
    if vid != 0:
        for v in w["vessels"]:
            if v["pid"] == vid:
                v["pid"] += 5000
        rng_pick(r, w["vessels"])["pid"] = vid
    if pid != 0:
        for v in w["vessels"]:
            for p in v["parts"]:
                if p["pid"] == pid:
                    p["pid"] += 5000
        parts = [p for v in w["vessels"] for p in v["parts"]]
        if parts:
            rng_pick(r, parts)["pid"] = pid


def world_ops(w, ops, craft=None, req_v=None, req_p=None):
    w["ut"] = 100.0
    ops.append({"op": "env", "env": w})
    ops.append({"op": "pw", "craft": craft or {}, "reqVessel": req_v or [], "reqPart": req_p or []})
    _PROTO[id(w)] = ProtoState(w)


def gen_failure(r):
    w = new_world(r)
    mission = mission_for(r, w)
    a = Failure(mission)
    cfg = Cfg("N")
    gen_cfg_vp(r, cfg, w)
    mods = Cfg("MODULES")
    for _ in range(r.randrange(0, 3)):
        mods.nodes.append(adjuster_cfg(r))
    if mods.nodes or r.random() < 0.5:
        cfg.nodes.append(mods)
    if r.random() < 0.3:
        cfg.values.append(("title", "Fail it"))
    a.load(cfg)
    w = new_world(r, cfg_dict(cfg))
    ops = [{"op": "mission", "spec": mission_spec(mission)}] + head(a)
    craft = {}
    for v in w["vessels"]:
        if r.random() < 0.7:
            craft[str(v["pid"])] = part_pids(v) if r.random() < 0.6 else []
    world_ops(w, ops, craft)
    ops += body_ops_for(a, "ActionPartFailure")
    ops.append({"op": "validate", "out": a.vp.validate(craft)})
    for k in range(3):
        if k > 0:
            w = new_world(r, cfg_dict(cfg))
            world_ops(w, ops, craft)
        ops.append({"op": "fire", "out": a.fire(w), "proto": _PROTO[id(w)].snapshot(w)})
    sel = None
    lp = [p for v in w["vessels"] for p in v["parts"]]
    for sp in (None, lp[0] if lp else None):
        ops.append({"op": "supported", "partPid": None if sp is None else sp["pid"], "out": a.supported_types(sp)})
    a.ids_changed(a.vp.vid, a.vp.vid + 1000)
    ops.append({"op": "call", "fn": "OnVesselPersistentIdChanged", "a": [a.vp.vid - 1000, a.vp.vid]})
    a.part_id_changed(a.vp.vid, a.vp.pid, a.vp.pid + 7)
    ops.append({"op": "call", "fn": "OnPartPersistentIdChanged", "a": [a.vp.vid, a.vp.pid - 7, a.vp.pid]})
    ops += body_ops_for(a, "ActionPartFailure")
    ops.append({"op": "saved", "out": a.save().dump()})
    return {"kind": "pactions", "cls": "ActionPartFailure", "cfg": cfg.to_json(), "ops": ops}


def gen_repair(r):
    w = new_world(r)
    # failures of the mission: one or two ActionPartFailure with adjusters from the shared id pool
    fails = []
    fail_specs = []
    mission = mission_for(r, w)
    for _ in range(r.randrange(0, 3)):
        f = Failure(mission)
        c = Cfg("N")
        gen_cfg_vp(r, c, w)
        mods = Cfg("MODULES")
        for _k in range(r.randrange(1, 3)):
            mods.nodes.append(adjuster_cfg(r))
        c.nodes.append(mods)
        f.load(c)
        fails.append(f)
        fail_specs.append(c.to_json())
    mission.failures = fails
    a = Repair(mission)
    cfg = Cfg("N")
    gen_cfg_vp(r, cfg, w)
    cfg.values.append(("repairType", rng_pick(r, REPAIR + ["bogus"])))
    if r.random() < 0.8:
        cfg.values.append(("partModule", rng_pick(r, MODULE_POOL + ["None Available"])))
    cfg.values.append(("failureID", rng_pick(r, ADJ_IDS + [EMPTY_GUID])))
    a.load(cfg)
    w = new_world(r, cfg_dict(cfg))
    spec = mission_spec(mission)
    spec["failures"] = fail_specs
    ops = [{"op": "mission", "spec": spec}] + head(a)
    craft = {}
    world_ops(w, ops, craft)
    ops += body_ops_for(a, "ActionPartRepair")
    ops.append({"op": "validate", "out": a.vp.validate(craft)})
    ops.append({"op": "dropRepair", "out": a.repair_items()})
    ops.append({"op": "dropFailure", "out": a.failure_items()})
    for k in range(3):
        if k > 0:
            w = new_world(r, cfg_dict(cfg))
            world_ops(w, ops, craft)
            a.repair = rng_pick(r, REPAIR)
            ops.append({"op": "setRepair", "value": a.repair})
        ops.append({"op": "fire", "out": a.fire(w), "proto": _PROTO[id(w)].snapshot(w)})
    ops.append({"op": "saved", "out": a.save().dump()})
    return {"kind": "pactions", "cls": "ActionPartRepair", "cfg": cfg.to_json(), "ops": ops}


def gen_drain(r):
    w = new_world(r)
    mission = mission_for(r, w)
    a = Drain(mission)
    cfg = Cfg("N")
    gen_cfg_vp(r, cfg, w)
    if r.random() < 0.9:
        cfg.values.append(("amountToDrain", rng_pick(r, ["10", "37.5", "150", "0", "400"])))
    if r.random() < 0.7:
        cfg.values.append(("timePeriod", rng_pick(r, ["0", "5", "30.5", "-1"])))
    cfg.values.append(("resourceName", rng_pick(r, RESOURCES * 3 + ["Nonesuch", ""])))
    if r.random() < 0.3:
        cfg.values.append(("leftToDrain", "4.25"))
    a.load(cfg)
    ops = [{"op": "mission", "spec": mission_spec(mission)}] + head(a)
    world_ops(w, ops, {})
    ops += body_ops_for(a, "ActionPartResourceDrain")
    ops.append({"op": "validate", "out": a.vp.validate({})})
    for k in range(2):
        if k > 0:
            a = Drain(mission)
            a.load(cfg)
            ops.append({"op": "reload"})
        req_v = [rng_pick(r, [0.0, 2.5, 7.25, 50.0, 200.0]) for _ in range(40)]
        req_p = [rng_pick(r, [0.0, 2.5, 7.25, 50.0, 200.0]) for _ in range(40)]
        w = new_world(r, cfg_dict(cfg))
        # the ids the configuration refers to exist in the world
        world_ops(w, ops, {}, list(req_v), list(req_p))
        out = a.fire(w, list(req_v), list(req_p))
        ops.append({"op": "fire", "out": {"yields": out["yields"], "waits": out["waits"], "requests": out["requests"], "events": out["events"], "left": a.left, "running": a.running, "throws": out.get("throws", False)},
                    "proto": _PROTO[id(w)].snapshot(w)})
    ops.append({"op": "saved", "out": a.save().dump()})
    return {"kind": "pactions", "cls": "ActionPartResourceDrain", "cfg": cfg.to_json(), "ops": ops}


def generate(r):
    cases = []
    for _ in range(120):
        cases.append(gen_failure(r))
        cases.append(gen_repair(r))
        cases.append(gen_drain(r))
    return cases
