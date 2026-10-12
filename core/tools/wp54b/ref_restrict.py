"""WP-54 slice 2 reference, part 2: VesselRestriction* (Expansions/Missions/VesselRestriction*.cs), MissionPreflightCheck, MissionBlackListPreFlightCheck and
VesselSituation. No kspcore code."""
import math
import os
import re

from ref_common import *  # noqa: F401,F403
from ref_common import Cfg, f32, fnum, fmt, tag, b, g9, MISSIONS, CORE, CS_ROOT, nfmt
from ref_data import D, F, FS, rng_pick, gen_location, VGL, Orb, body, HOME, parse_f, _net_exp  # noqa: F401
from ref_common import KSPRandom

OPS = ["LessOrEqual", "Equal", "GreaterOrEqual"]
TRAITS = ["Pilot", "Engineer", "Scientist", "Tourist"]
CREW_TYPES = ["TotalCrew", "Engineers", "Scientists", "Pilots"]
CREW_DESC = {"TotalCrew": "#autoLOC_8005005", "Engineers": "#autoLOC_8005008", "Scientists": "#autoLOC_8005007", "Pilots": "#autoLOC_8005006"}
CREW_TRAIT = {"Engineers": "Engineer", "Scientists": "Scientist", "Pilots": "Pilot"}


def first_resource_name():
    path = os.path.join(CORE, "testdata", "GameData-mirror", "GameData", "Squad", "Resources", "ResourcesGeneric.cfg")
    text = open(path, encoding="utf-8-sig").read()
    m = re.search(r"RESOURCE_DEFINITION\s*\{(.*?)\n\}", text, re.S)
    return re.search(r"^\s*name\s*=\s*(\S+)", m.group(1), re.M).group(1)


def part_renames():
    """MissionsUtils.UpdatePartName: the `case "a": return "b";` table parsed from the C# source"""
    src = open(os.path.join(MISSIONS, "MissionsUtils.cs"), encoding="utf-8-sig").read()
    i = src.index("internal static string UpdatePartName")
    j = src.index("default:", i)
    return dict(re.findall(r'case "([^"]+)":\s*return "([^"]+)";', src[i:j]))


RENAMES = part_renames()


def rename(n):
    return RENAMES.get(n, n)


def test_float(cur, tgt, op):
    cur, tgt = f32(cur), f32(tgt)
    if op == "LessOrEqual":
        return cur <= tgt
    if op == "Equal":
        return abs(f32(tgt - cur)) < 1e-15
    if op == "GreaterOrEqual":
        return cur >= tgt
    return True


def test_int(cur, tgt, op):
    if op == "LessOrEqual":
        return cur <= tgt
    if op == "Equal":
        return tgt - cur == 0
    if op == "GreaterOrEqual":
        return cur >= tgt
    return True


def try_float(v):
    try:
        return parse_f(v)
    except ValueError:
        return 0.0


def try_int(v):
    t = v.strip()
    try:
        return int(t)
    except ValueError:
        return 0


def n2(x):
    return nfmt(f32(x), 2)


class Restriction:
    NAME = ""
    SAVE_KEYS = []

    def __init__(self):
        self.op = "GreaterOrEqual"
        self.id = ""

    def load_base(self, cfg):
        v = cfg.get("id")
        if v is not None:
            self.id = v

    def enum_op(self, cfg):
        v = cfg.get("comparisonOperator")
        self.op = v if v in OPS else "GreaterOrEqual"

    def save(self, out):
        out.values += [("name", self.NAME), ("id", self.id)]

    def display(self):
        return fmt("#autoLOC_8100211")


class Cost(Restriction):
    NAME = "VesselRestriction_Cost"

    def __init__(self):
        super().__init__()
        self.target = 0.0

    def load(self, cfg):
        self.load_base(cfg)
        for k, v in cfg.values:
            if k == "cost":
                self.target = try_float(v)
            elif k == "comparisonOperator":
                self.enum_op(cfg)

    def save(self, out):
        super().save(out)
        out.values += [("cost", F(self.target)), ("comparisonOperator", self.op)]

    def display(self):
        return "#autoLOC_8100136"

    def complete(self, env):
        return test_float(env["cost"], self.target, self.op)

    def message(self, env):
        t = {"LessOrEqual": "#autoLOC_8100138", "Equal": "#autoLOC_8100137", "GreaterOrEqual": "#autoLOC_8100139"}.get(self.op)
        return "" if t is None else fmt(t, FS(self.target), FS(env["cost"]))

    def same(self, o):
        return self.op == o.op


class Crew(Restriction):
    NAME = "VesselRestriction_Crew"

    def __init__(self):
        super().__init__()
        self.ctype = "TotalCrew"
        self.target = 0

    def load(self, cfg):
        self.load_base(cfg)
        for k, v in cfg.values:
            if k == "crewType":
                self.ctype = v if v in CREW_TYPES else "TotalCrew"
            elif k == "comparisonOperator":
                self.enum_op(cfg)
            elif k == "crewCount":
                self.target = try_int(v)

    def save(self, out):
        super().save(out)
        out.values += [("crewType", self.ctype), ("comparisonOperator", self.op), ("crewCount", str(self.target))]

    def display(self):
        return fmt("#autoLOC_8100143")

    def count(self, env):
        if self.ctype == "TotalCrew":
            return env["crewCount"] if env["crewCount"] is not None else 0
        if env["crewTraits"] is None:
            return 0
        return sum(1 for t in env["crewTraits"] if t == CREW_TRAIT[self.ctype])

    def complete(self, env):
        return test_int(self.count(env), self.target, self.op)

    def message(self, env):
        t = {"LessOrEqual": "#autoLOC_8100145", "Equal": "#autoLOC_8100144", "GreaterOrEqual": "#autoLOC_8100146"}.get(self.op)
        return "" if t is None else fmt(t, str(self.target), CREW_DESC[self.ctype], str(self.count(env)))

    def same(self, o):
        raise TypeError("InvalidCast")  # `(VesselRestriction_Mass)otherRestriction`


class Mass(Restriction):
    NAME = "VesselRestriction_Mass"

    def __init__(self):
        super().__init__()
        self.target = 0.0

    def load(self, cfg):
        self.load_base(cfg)
        for k, v in cfg.values:
            if k == "mass":
                self.target = try_float(v)
            elif k == "comparisonOperator":
                self.enum_op(cfg)

    def save(self, out):
        super().save(out)
        out.values += [("mass", F(self.target)), ("comparisonOperator", self.op)]

    def display(self):
        return fmt("#autoLOC_8100169")

    def complete(self, env):
        return test_float(env["mass"], self.target, self.op)

    def message(self, env):
        t = {"LessOrEqual": "#autoLOC_8100171", "Equal": "#autoLOC_8100170", "GreaterOrEqual": "#autoLOC_8100172"}.get(self.op)
        return fmt("#autoLOC_8100173") if t is None else fmt(t, FS(self.target), FS(env["mass"]))

    def same(self, o):
        return self.op == o.op


class PartCount(Restriction):
    NAME = "VesselRestriction_PartCount"

    def __init__(self):
        super().__init__()
        self.target = 0

    def load(self, cfg):
        self.load_base(cfg)
        for k, v in cfg.values:
            if k == "numberOfParts":
                self.target = try_int(v)
            elif k == "comparisonOperator":
                self.enum_op(cfg)

    def save(self, out):
        super().save(out)
        out.values += [("numberOfParts", str(self.target)), ("comparisonOperator", self.op)]

    def display(self):
        return fmt("#autoLOC_8100177")

    def complete(self, env):
        return test_int(env["parts"], self.target, self.op)

    def message(self, env):
        t = {"LessOrEqual": "#autoLOC_8100179", "Equal": "#autoLOC_8100178", "GreaterOrEqual": "#autoLOC_8100180"}.get(self.op)
        return fmt("#autoLOC_8100173") if t is None else fmt(t, str(self.target), str(env["parts"]))

    def same(self, o):
        return self.op == o.op


class Resource(Restriction):
    NAME = "VesselRestriction_Resource"

    def __init__(self):
        super().__init__()
        self.name = first_resource_name()
        self.target = 0.0

    def load(self, cfg):
        self.load_base(cfg)
        for k, v in cfg.values:
            if k == "comparisonOperator":
                self.enum_op(cfg)
            elif k == "resourceName":
                self.name = v
            elif k == "amount":
                self.target = try_float(v)

    def save(self, out):
        super().save(out)
        out.values += [("amount", F(self.target)), ("resourceName", self.name), ("comparisonOperator", self.op)]

    def display(self):
        return fmt("#autoLOC_8100186")

    def current(self, env):
        t = 0.0
        for n, a in env["resources"]:
            if n == self.name:
                t += a
        return t

    def complete(self, env):
        return test_float(f32(self.current(env)), self.target, self.op)

    def message(self, env):
        t = {"LessOrEqual": "#autoLOC_8100188", "Equal": "#autoLOC_8100187", "GreaterOrEqual": "#autoLOC_8100189"}.get(self.op)
        return "" if t is None else fmt(t, nfmt(f32(self.target), 1), self.name, nfmt(self.current(env), 1))

    def same(self, o):
        return self.op == o.op


class Size(Restriction):
    NAME = "VesselRestriction_Size"

    def __init__(self):
        super().__init__()
        self.x = self.y = self.z = 0.0

    def load(self, cfg):
        self.load_base(cfg)
        for k, v in cfg.values:
            if k == "comparisonOperator":
                self.enum_op(cfg)
            elif k == "sizeZ":
                self.z = try_float(v)
            elif k == "sizeY":
                self.y = try_float(v)
            elif k == "sizeX":
                self.x = try_float(v)

    def save(self, out):
        super().save(out)
        out.values += [("sizeX", F(self.x)), ("sizeY", F(self.y)), ("sizeZ", F(self.z)), ("comparisonOperator", self.op)]

    def display(self):
        return fmt("#autoLOC_8100194")

    def complete(self, env):
        cur = env["size"]
        return test_float(cur[0], self.x, self.op) and test_float(cur[1], self.y, self.op) and test_float(cur[2], self.z, self.op)

    @staticmethod
    def dims(d):
        u = fmt("#autoLOC_7001411")
        return ("\t<color=#b4d455>" + fmt("#autoLOC_8100191") + ":</color> " + n2(d[1]) + u + "\n" + "\t<color=#b4d455>" + fmt("#autoLOC_8100192") + ":</color> " + n2(d[0]) +
                u + "\n" + "\t<color=#b4d455>" + fmt("#autoLOC_8100193") + ":</color> " + n2(d[2]) + u + "\n")

    def message(self, env):
        t = {"LessOrEqual": "#autoLOC_8100196", "Equal": "#autoLOC_8100195", "GreaterOrEqual": "#autoLOC_8100197"}.get(self.op)
        return fmt("#autoLOC_8100173") if t is None else fmt(t, self.dims([self.x, self.y, self.z]), self.dims(env["size"]))

    def same(self, o):
        return self.op == o.op


class Stages(Restriction):
    NAME = "VesselRestriction_Stages"

    def __init__(self):
        super().__init__()
        self.target = 0

    def load(self, cfg):
        self.load_base(cfg)
        for k, v in cfg.values:
            if k == "stages":
                self.target = try_int(v)
            elif k == "comparisonOperator":
                self.enum_op(cfg)

    def save(self, out):
        super().save(out)
        out.values += [("stages", str(self.target)), ("comparisonOperator", self.op)]

    def display(self):
        return fmt("#autoLOC_8100204")

    def complete(self, env):
        return test_int(env["stages"], self.target, self.op)

    def message(self, env):
        t = {"LessOrEqual": "#autoLOC_8100206", "Equal": "#autoLOC_8100205", "GreaterOrEqual": "#autoLOC_8100207"}.get(self.op)
        return "" if t is None else fmt(t, str(self.target), str(env["stages"]))

    def same(self, o):
        return self.op == o.op


CLASSES = {c.NAME: c for c in (Cost, Crew, Mass, PartCount, Resource, Size, Stages)}


def make_env(r):
    return {"cost": f32(rng_pick(r, [0.0, 1000.5, 25000.0, 300.25])), "mass": f32(rng_pick(r, [0.0, 1.5, 12.25, 100.0])), "parts": rng_pick(r, [0, 1, 5, 12]),
            "size": [f32(rng_pick(r, [0.0, 1.5, 3.25, 10.0])) for _ in range(3)], "stages": rng_pick(r, [0, 1, 3, 6]),
            "resources": [[rng_pick(r, ["LiquidFuel", "Oxidizer", "MonoPropellant", "ElectricCharge"]), rng_pick(r, [0.0, 90.0, 180.5, 1234.5])] for _ in range(r.randrange(0, 5))],
            "crewCount": rng_pick(r, [None, 0, 2, 5]), "crewTraits": rng_pick(r, [None, [], ["Pilot"], ["Pilot", "Engineer", "Scientist"], ["Engineer", "Engineer", "Tourist"]]),
            "time": rng_pick(r, [0.0, 12.5, 100.25])}


def gen_restriction(r):
    cls = r.choice(list(CLASSES.values()))
    inst = cls()
    cfg = Cfg("MODULE")
    cfg.values.append(("name", cls.NAME))
    cfg.values.append(("id", str(r.randrange(1, 99))))
    keys = {"VesselRestriction_Cost": ["cost"], "VesselRestriction_Crew": ["crewType", "crewCount"], "VesselRestriction_Mass": ["mass"],
            "VesselRestriction_PartCount": ["numberOfParts"], "VesselRestriction_Resource": ["resourceName", "amount"],
            "VesselRestriction_Size": ["sizeX", "sizeY", "sizeZ"], "VesselRestriction_Stages": ["stages"]}[cls.NAME] + ["comparisonOperator"]
    r.shuffle(keys)
    for k in keys[: r.randrange(0, len(keys) + 1)]:
        if k == "comparisonOperator":
            cfg.values.append((k, rng_pick(r, OPS + ["Bogus"])))
        elif k == "crewType":
            cfg.values.append((k, rng_pick(r, CREW_TYPES + ["Nope"])))
        elif k in ("crewCount", "numberOfParts", "stages"):
            cfg.values.append((k, rng_pick(r, ["0", "1", "5", "12", "x", "6"])))
        elif k == "resourceName":
            cfg.values.append((k, rng_pick(r, ["LiquidFuel", "Oxidizer", "MonoPropellant"])))
        else:
            cfg.values.append((k, rng_pick(r, ["0", "1.5", "12.25", "100", "25000", "abc", "3.25", "1000.5", "90", "180.5"])))
    inst.load(cfg)
    out = Cfg("N")
    inst.save(out)
    scenarios = []
    for _ in range(4):
        env = make_env(r)
        # bias one scenario to the target so the Equal / boundary branches are hit
        if _ == 0:
            if isinstance(inst, Cost):
                env["cost"] = f32(inst.target)
            elif isinstance(inst, Mass):
                env["mass"] = f32(inst.target)
            elif isinstance(inst, PartCount):
                env["parts"] = inst.target
            elif isinstance(inst, Stages):
                env["stages"] = inst.target
            elif isinstance(inst, Size):
                env["size"] = [f32(inst.x), f32(inst.y), f32(inst.z)]
            elif isinstance(inst, Crew):
                env["crewCount"] = inst.target
            elif isinstance(inst, Resource):
                env["resources"] = [[inst.name, float(inst.target)]]
        comp = inst.complete(env)
        scenarios.append({"env": env, "complete": comp, "message": inst.message(env),
                          "ui": {"text": inst.message(env), "state": 0 if comp else 1, "readyReset": not comp}})
    # another instance of the same type: SameComparatorWrapper
    other = cls()
    ocfg = Cfg("MODULE")
    ocfg.values.append(("comparisonOperator", rng_pick(r, OPS)))
    other.load(ocfg)
    try:
        same = inst.same(other)
    except TypeError:
        same = "throw"
    return {"kind": "restriction", "type": cls.NAME, "cfg": cfg.to_json(), "save": out.dump(), "display": inst.display(), "scenarios": scenarios,
            "otherCfg": ocfg.to_json(), "same": same, "firstResource": first_resource_name()}


# ---- VesselSituation / restriction list / preflight checks -----------------------------------------------------------------------------
class PartFilter:
    def __init__(self):
        self.ftype = "unavailableParts"
        self.avail = []
        self.unavail = []

    def load(self, cfg):
        v = cfg.get("filterType")
        self.ftype = v if v in ("availableParts", "unavailableParts") else "unavailableParts"
        if self.ftype == "availableParts":
            self.avail = [rename(x) for x in cfg.get_all("availableParts")]
        else:
            self.unavail = [rename(x) for x in cfg.get_all("unavailableParts")]

    def save(self, out):
        lst, key = (self.avail, "availableParts") if self.ftype == "availableParts" else (self.unavail, "unavailableParts")
        for i, x in enumerate(lst):
            if i == 0:
                out.values.append(("filterType", self.ftype))
            out.values.append((key, x))


PID_DEFAULT = 424242  # FlightGlobals.GetUniquepersistentId() pinned by the test (VesselSituation(mission, node) takes one)
TICK = 12345  # ResourceEnvironment::tickCount pinned by the test: new KSPRandom() seeds from it


def default_location_dump(tick):
    """VesselLocation(MENode) after construction (no LOCATION node to Load): the dump of its Save"""
    rnd = KSPRandom(tick)
    lat = -0.008999999612569809 + rnd.next_double() * (0.15530000627040863 - -0.008999999612569809)
    lon = -74.82420349121094 + rnd.next_double() * (-74.51119995117188 - -74.82420349121094)
    g = VGL()
    g.icon = "Rocket"
    g.body = HOME
    g.splashed = True
    g.lat, g.lon = lat, lon
    gn = Cfg("GROUNDPOINT")
    g.save(gn)
    o = Orb(HOME)
    on = Cfg("ORBIT")
    o.save(on)
    return "LOCATION{sit=PRELAUNCH;launchSite=LaunchPad;facility=VAB;brakesOn=True;" + gn.dump() + on.dump() + "}"


def gen_situation(r, with_location=True):
    cfg = Cfg("VESSELSITUATION")
    st = {"ship": "#autoLOC_8100086", "desc": "", "player": True, "pid": PID_DEFAULT, "craft": "", "focus": False, "launched": False, "ready": False, "auto": False}
    ship_default = fmt("#autoLOC_8100086")
    st["ship"] = ship_default  # VesselSituation(mission, node) formats the tag
    for k in r.sample(["persistentId", "focusonSpawn", "ship", "autoGenerateCrew", "playerCreated", "vesselDescription", "craftFile", "launched", "readyToLaunch"], r.randrange(0, 10)):
        if k == "persistentId":
            v = rng_pick(r, ["0", "12345", "999999"])
            st["pid"] = int(v)
        elif k == "ship":
            v = rng_pick(r, ["My Ship", "Probe 1", "Lander"])
            st["ship"] = v
        elif k == "vesselDescription":
            v = rng_pick(r, ["Hello", "Two words", "x"])
            st["desc"] = v
        elif k == "craftFile":
            v = rng_pick(r, ["a.craft", "Ship One.craft"])
            st["craft"] = v
        else:
            v = rng_pick(r, ["True", "False"])
            key = {"focusonSpawn": "focus", "autoGenerateCrew": "auto", "playerCreated": "player", "launched": "launched", "readyToLaunch": "ready"}[k]
            st[key] = v == "True"
        cfg.values.append((k, v))
    loc = None
    if with_location:
        loc = gen_location(r)
        lnode = Cfg("LOCATION", [tuple(x) for x in loc["cfg"]["values"]], [Cfg(n["name"], [tuple(x) for x in n["values"]], []) for n in loc["cfg"]["nodes"]])
        cfg.nodes.append(lnode)
        loc_dump = "LOCATION{" + loc["save"][2:-1] + "}"
    else:
        loc_dump = default_location_dump(TICK)
    crews = []
    for _ in range(r.randrange(0, 3)):
        c = Cfg("PARTCREW")
        names = [rng_pick(r, ["Jebediah Kerman", "Bob Kerman", "Gus Kerman"]) for _ in range(r.randrange(0, 3))]
        c.values += [("crew", n) for n in names]
        vid, pid = str(r.randrange(1, 100)), str(r.randrange(1, 100))
        c.values += [("vesselPersistentId", vid), ("partPersistentId", pid)]
        cfg.nodes.append(c)
        crews.append((names, vid, pid))
    pf = PartFilter()
    if r.random() < 0.6:
        cfg.values.append(("filterType", rng_pick(r, ["availableParts", "unavailableParts", "Bogus"])))
    for key in ("availableParts", "unavailableParts"):
        for _ in range(r.randrange(0, 3)):
            cfg.values.append((key, rng_pick(r, ["mk1pod", "liquidEngine", "fuelTank1-2", "stackDecoupler", "RCSBlock"])))
    req = []
    for _ in range(r.randrange(0, 3)):
        v = rng_pick(r, ["mk1pod", "probeCoreHex", "solidBooster", "decoupler1-2", "Custom"])
        cfg.values.append(("requiredParts", v))
        req.append(rename(v))
    pf.load(cfg)
    mods = []
    if r.random() < 0.7:
        rn = Cfg("MODULES")
        for _ in range(r.randrange(0, 4)):
            cls = r.choice(list(CLASSES.values()))
            m = Cfg("MODULE")
            m.values.append(("name", cls.NAME))
            m.values.append(("id", str(r.randrange(1, 99))))
            if r.random() < 0.7:
                m.values.append(("comparisonOperator", rng_pick(r, OPS)))
            rn.nodes.append(m)
            inst = cls()
            inst.load(m)
            mods.append(inst)
        if r.random() < 0.15:
            bad = Cfg("MODULE")
            bad.values.append(("name", "VesselRestriction_DoesNotExist"))
            rn.nodes.append(bad)
        cfg.nodes.append(rn)
    v = Cfg("VESSELSITUATION")
    v.values += [("ship", st["ship"]), ("vesselDescription", st["desc"]), ("playerCreated", b(st["player"])), ("persistentId", str(st["pid"])), ("craftFile", st["craft"]),
                 ("focusonSpawn", b(st["focus"])), ("launched", b(st["launched"])), ("readyToLaunch", b(st["ready"])), ("autoGenerateCrew", b(st["auto"]))]
    pf.save(v)
    for x in req:
        v.values.append(("requiredParts", x))
    nodes_text = loc_dump
    for names, vid, pid in crews:
        c = Cfg("PARTCREW")
        c.values += [("partPersistentId", pid), ("vesselPersistentId", vid)] + [("crew", n) for n in names]
        nodes_text += c.dump()
    if st["player"]:
        ml = Cfg("MODULES")
        for m in mods:
            mn = Cfg("MODULE")
            m.save(mn)
            ml.nodes.append(mn)
        if req:  # Game.Mode == MISSION and requiredParts.Count > 0: a VesselRestriction_RequiredParts is appended after the Load
            ml.nodes.append(Cfg("MODULE"))  # VesselRestriction_RequiredParts.Save is empty (C# :47)
        nodes_text += ml.dump()
    values_text = "".join("%s=%s;" % kv for kv in v.values)
    return {"kind": "situation", "cfg": cfg.to_json(), "save": "N{VESSELSITUATION{" + values_text + nodes_text + "}}", "tick": TICK, "defaultPid": PID_DEFAULT, "playerCreated": st["player"],
            "requiredParts": req}


def generate(r):
    cases = []
    for _ in range(140):
        cases.append(gen_restriction(r))
    for i in range(60):
        cases.append(gen_situation(r, with_location=(i % 6 != 5)))
    return cases
