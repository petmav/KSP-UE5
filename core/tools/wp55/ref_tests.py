"""WP-55 reference, part 2: Env (scenario model) and the Tests.* modules, written from Assembly-CSharp/Expansions/Missions/Tests/*.cs."""
import math

from ref_base import *  # noqa: F401,F403
from ref_base import Cfg, Mod, TESTS, f32, fnum, dnum, fmt, tag, parse, vtext, test_class, ENUMS, cmp5, opstr, lg_prefix

# ---------------------------------------------------------------------------------------------------------------------------
# KSPUtil.PrintDateDeltaCompact(time, includeTime, includeSeconds) with the Kerbin calendar (GameSettings.KERBIN_TIME = true):
# KSPUtil.cs:424, get_date_from_UT :295 (year 9,201,600 s, day 21,600 s); non-negative times only
KERBIN_YEAR = 426 * 21600
KERBIN_DAY = 21600


def date_delta_compact(time, include_time, include_secs, places=5):
    num = int(time / KERBIN_YEAR)
    t = time - num * KERBIN_YEAR
    n2 = int(t)
    d = [n2 % 60, n2 // 60 % 60, n2 // 3600 % (KERBIN_DAY // 3600), n2 // KERBIN_DAY, num]
    u = [fmt("#autoLOC_6002317"), fmt("#autoLOC_6002318"), fmt("#autoLOC_6002319"), fmt("#autoLOC_6002320"), fmt("#autoLOC_6002321")]
    s = ""
    count = 0
    if d[4] > 0 and count < places:
        s += str(d[4]) + u[4]
        count += 1
    if d[3] > 0 and count < places:
        if s:
            s += ", "
        s += str(d[3]) + u[3]
        count += 1
    if include_time:
        if d[2] > 0 and count < places:
            if s:
                s += ", "
            s += str(d[2]) + u[2]
            count += 1
        if d[1] > 0 and count < places:
            if s:
                s += ", "
            s += str(d[1]) + u[1]
            count += 1
        if include_secs and d[0] > 0 and count < places:
            if s:
                s += ", "
            s += str(d[0]) + u[0]
    if not s:
        s = ("0" + u[3]) if not include_time else (("0" + u[0]) if include_secs else ("0" + u[1]))
    return s


def time_body(op, time):
    return ("< " if op == "LessThan" else "> ") + date_delta_compact(time, True, True)


def round_half_up(x):
    return int(math.floor(abs(x) + 0.5)) * (1 if x >= 0 else -1)


def fixed(x, decimals):
    """x.ToString("F<decimals>") for a non-tie value: round half away from zero on the decimal expansion."""
    from decimal import Decimal, ROUND_HALF_UP
    q = Decimal(1).scaleb(-decimals)
    return str(Decimal(repr(x)).quantize(q, rounding=ROUND_HALF_UP)) if decimals else str(Decimal(repr(x)).quantize(Decimal(1), rounding=ROUND_HALF_UP))


# ---------------------------------------------------------------------------------------------------------------------------
# scenario environment (synthetic input of the algorithms, shared by the C++ test)
class Env:
    def __init__(self, d):
        self.d = d
        self.ut = d.get("ut", 0.0)
        self.startedUT = d.get("startedUT", 0.0)
        self.score = f32(d.get("score", 0.0))
        self.funds = d.get("funds")
        self.vessels = d.get("vessels", [])
        self.active_idx = d.get("active", -1)
        self.nodes = {n["guid"]: n for n in d.get("nodes", [])}
        self.startNode = d.get("startNode")

    def active(self):
        return self.vessels[self.active_idx] if 0 <= self.active_idx < len(self.vessels) else None

    def by_pid(self, pid):
        for v in self.vessels:
            if v["pid"] == pid:
                return v
        return None


def vkey(v):
    return None if v is None else v["pid"]


def v_mass(v):
    # Vessel.GetTotalMass (Vessel.cs:3422): the double sum of the part masses, returned as float
    t = 0.0
    for m in v["partMasses"]:
        t += m
    return f32(t)


def ltgt(op, a, b):
    if op == "GreaterThan":
        return a > b
    if op == "LessThan":
        return a < b
    return False


class TV(Mod):
    """TestVessel.cs: the base of the vessel tests."""
    SCORE_VESSEL = True
    SAVE = []
    LOAD = []

    def awake(self):
        Mod.awake(self)
        self.vesselID = 0
        self.vessel = None
        self.oldRootPart = 0
        self.missing = set()

    def tv_test(self, env):
        # TestVessel.Test (TestVessel.cs:113)
        if self.vesselID == 0:
            self.vessel = env.active()
        else:
            v = env.by_pid(self.vesselID)
            if v is not None:
                self.vessel = v
            elif self.vessel is not None:
                self.missing.add(self.vesselID)
                self.vessel = None
        return True

    def test(self, env):
        return self.tv_test(env)

    def save_fields(self, cn):
        cn.values.append(("vesselID", str(self.vesselID)))
        if self.oldRootPart != 0:
            cn.values.append(("oldRootPart", str(self.oldRootPart)))
        Mod.save_fields(self, cn)

    def load_fields(self, cfg):
        for key in ("vesselID", "oldRootPart"):
            v = parse(cfg.get(key) or "", "uint")
            if v is not None:
                setattr(self, key, v)
        Mod.load_fields(self, cfg)

    def score_modifier(self, module, env):
        if module == "ScoreModule_Resource" and self.SCORE_VESSEL:
            if self.vesselID == 0:
                return {"vessel": vkey(env.active()), "number": None}
            return {"vessel": vkey(env.by_pid(self.vesselID)), "number": None}
        return {"vessel": None, "number": None}

    def body_vessel_field(self, field, env):
        """TestVessel.GetNodeBodyParameterString for vesselID (no vessel situations in the scenarios)."""
        if field["name"] == "vesselID":
            return fmt("#autoLOC_8000069", fmt("#autoLOC_8004217") if self.USE_ACTIVE else fmt("#autoLOC_8001004"))
        return None

    USE_ACTIVE = True


def sat_resource_modifier(self, module, env):
    return {"vessel": vkey(env.active()) if module == "ScoreModule_Resource" else None, "number": None}


def lg_cfg(attr="comparisonOperator"):
    return [("comparisonOperator", attr, "enum", "LG", "GreaterThan")]


# ------------------------------------------------------------- the tests
@test_class
class TestEmptyTrue(Mod):
    NAME = "TestEmptyTrue"
    TITLE = "#autoLOC_8003077"
    INFO = "#autoLOC_8003078"

    def test(self, env):
        return True


@test_class
class TestEmptyFalse(Mod):
    NAME = "TestEmptyFalse"
    TITLE = "#autoLOC_8003075"
    INFO = "#autoLOC_8003076"

    def test(self, env):
        return False


@test_class
class TestVessel(TV):
    NAME = "TestVessel"
    SCORE_VESSEL = False
    USE_ACTIVE = False


@test_class
class TestVesselActive(TV):
    NAME = "TestVesselActive"
    TITLE = "#autoLOC_8004217"
    TITLE_FMT = False
    INFO = "#autoLOC_8006116"
    SCORE_VESSEL = False

    def test(self, env):
        self.tv_test(env)
        return vkey(self.vessel) == vkey(env.active())


@test_class
class TestMETTime(TV):
    NAME = "TestMETTime"
    TITLE = "#autoLOC_8004046"
    INFO = "#autoLOC_8004036"
    SAVE = [("time", "time", "double", None), ("comparisonOperator", "comparisonOperator", "string", None)]
    LOAD = [("time", "time", "double", None, None), ("comparisonOperator", "comparisonOperator", "enum", "LG", "LessThan")]

    def awake(self):
        TV.awake(self)
        self.time = 300.0
        self.comparisonOperator = "GreaterThan"

    def test(self, env):
        self.tv_test(env)
        if self.vessel is not None:
            return ltgt(self.comparisonOperator, self.vessel["missionTime"], self.time)
        return False

    def body(self, field, env):
        if field["name"] == "time":
            return time_body(self.comparisonOperator, self.time)
        return self.base_body(field)

    score_modifier = sat_resource_modifier


@test_class
class TestMissionTime(Mod):
    NAME = "TestMissionTime"
    TITLE = "#autoLOC_8000080"
    INFO = "#autoLOC_8004037"
    SAVE = [("time", "time", "double", None), ("comparisonOperator", "comparisonOperator", "string", None)]
    LOAD = [("time", "time", "double", None, None), ("comparisonOperator", "comparisonOperator", "enum", "LG", "LessThan")]

    def awake(self):
        Mod.awake(self)
        self.time = 300.0
        self.comparisonOperator = "GreaterThan"

    def test(self, env):
        return ltgt(self.comparisonOperator, env.ut, env.startedUT + self.time)

    def body(self, field, env):
        if field["name"] == "time":
            return time_body(self.comparisonOperator, self.time)
        return self.base_body(field)

    score_modifier = sat_resource_modifier


ZERO_GUID = "00000000-0000-0000-0000-000000000000"


@test_class
class TestTimeSinceNode(Mod):
    NAME = "TestTimeSinceNode"
    TITLE = "#autoLOC_8000099"
    INFO = "#autoLOC_8004038"

    def awake(self):
        Mod.awake(self)
        self.nodeID = ZERO_GUID
        self.time = 300.0
        self.comparisonOperator = "GreaterThan"
        self.start = None

    def initialize(self, env):
        # TestTimeSinceNode.Initialize (:32): nodeID = the mission's start node
        self.start = env.startNode
        if env.startNode is not None:
            self.nodeID = env.startNode

    def test(self, env):
        if self.nodeID not in env.nodes:
            return False
        n = env.nodes[self.nodeID]
        compare = n["activatedUT"] + self.time
        # MENode.HasBeenActivated (MENode.cs:158): activatedUT > 0 or the start node (no docked nodes in the scenarios)
        if n["activatedUT"] > 0.0 or n["isStart"]:
            return ltgt(self.comparisonOperator, env.ut, compare)
        return False

    def body(self, field, env):
        if field["name"] == "nodeID":
            if self.nodeID in env.nodes:
                objective = env.nodes[self.nodeID]["title"]
                text = fmt(objective)
                return text if text else objective
            return fmt("#autoLOC_8007310")
        if field["name"] == "time":
            return time_body(self.comparisonOperator, self.time)
        return self.base_body(field)

    def save_fields(self, cn):
        cn.values += [("nodeID", self.nodeID), ("time", dnum(self.time)), ("comparisonOperator", self.comparisonOperator)]

    def load_fields(self, cfg):
        if self.nodeID == ZERO_GUID and self.start is not None:
            self.nodeID = self.start
        v = cfg.get("nodeID")
        if v is not None:
            self.nodeID = v
        v = parse(cfg.get("time") or "", "double")
        if v is not None:
            self.time = v
        o = cfg.get("comparisonOperator")
        self.comparisonOperator = o if (o is not None and o in ENUMS["LG"]) else "LessThan"

    score_modifier = sat_resource_modifier


@test_class
class TestMissionScore(Mod):
    NAME = "TestMissionScore"
    TITLE = "#autoLOC_8000049"
    INFO = "#autoLOC_8004022"
    SAVE = [("score", "score", "float", None), ("comparisonOperator", "comparisonOperator", "string", None)]
    LOAD = [("score", "score", "float", None, None), ("comparisonOperator", "comparisonOperator", "enum", "Cmp5", "Equal")]

    def awake(self):
        Mod.awake(self)
        self.score = 0.0
        self.comparisonOperator = "LessThan"

    def test(self, env):
        cur = env.score
        op = self.comparisonOperator
        if op == "Equal":
            return abs(f32(self.score - cur)) < 1e-15
        return cmp5(op, cur, self.score)

    def body(self, field, env):
        if field["name"] == "score":
            return fmt("#autoLOC_8006046", opstr(self.comparisonOperator), "%d" % round_half_up(self.score))
        return self.base_body(field)

    def score_modifier(self, module, env):
        return {"vessel": None, "number": None}


@test_class
class TestMissionFunds(Mod):
    NAME = "TestMissionFunds"
    TITLE = "#autoLOC_8003129"
    INFO = "#autoLOC_8003130"
    SAVE = [("funds", "funds", "float", None), ("comparisonOperator", "comparisonOperator", "string", None)]
    LOAD = [("funds", "funds", "float", None, None), ("comparisonOperator", "comparisonOperator", "enum", "Cmp5", "Equal")]

    def awake(self):
        Mod.awake(self)
        self.funds = 0.0
        self.comparisonOperator = "LessThan"

    def test(self, env):
        if env.funds is None:
            return True
        cur = env.funds
        op = self.comparisonOperator
        if op == "Equal":
            return abs(self.funds - cur) < 1e-15
        return cmp5(op, cur, self.funds)

    def body(self, field, env):
        if field["name"] == "funds":
            return fmt("#autoLOC_8003131", opstr(self.comparisonOperator), "%d" % round_half_up(self.funds))
        return self.base_body(field)

    def score_modifier(self, module, env):
        return {"vessel": None, "number": None}


@test_class
class TestAccuracy(Mod):
    NAME = "TestAccuracy"
    TITLE = "#autoLOC_8001008"
    INFO = "#autoLOC_8001009"
    SAVE = [("accuracy", "accuracy", "double", None), ("comparisonOperator", "comparisonOperator", "string", None)]
    LOAD = [("accuracy", "accuracy", "double", None, None), ("comparisonOperator", "comparisonOperator", "enum", "LG", "GreaterThan")]

    def awake(self):
        Mod.awake(self)
        self.accuracy = 0.0
        self.comparisonOperator = "GreaterThan"
        self.found = False
        self.last = None

    def test(self, env):
        # env.d["dockParent"] = {"accuracyModifier": boxed double of GetScoreModifier(ScoreModule_Accuracy) or None} when the node is docked
        # to a node whose first test module is scoreable. TestAccuracy.Test (TestAccuracy.cs:24) caches the objective once it found one.
        if not self.found and "dockParent" in env.d:
            self.found = True
        if "dockParent" in env.d:
            self.last = env.d["dockParent"]["accuracyModifier"]
        if self.found:
            num = self.last * 100.0 if self.last is not None else 100.0
            return ltgt(self.comparisonOperator, num, self.accuracy)
        return False

    def body(self, field, env):
        if field["name"] == "accuracy":
            return fmt("#autoLOC_8001012", "<" if self.comparisonOperator == "LessThan" else ">", dnum(self.accuracy))
        return self.base_body(field)


@test_class
class TestGameEvent(Mod):
    NAME = "TestGameEvent"
    TITLE = "Game Event Test"
    TITLE_FMT = False
    SAVE = [("eventTriggered", "eventTriggered", "bool", None)]
    LOAD = [("eventTriggered", "eventTriggered", "bool", None, None)]

    def awake(self):
        Mod.awake(self)
        self.eventTriggered = False
        self.subscribed = False

    def test(self, env):
        if not self.subscribed:
            self.subscribed = True
        return self.eventTriggered

    def on_event(self, ev, env):
        if ev["name"] == "stageActivate" and self.subscribed:
            self.eventTriggered = True

    def score_modifier(self, module, env):
        return {"vessel": None, "number": None}


@test_class
class TestVesselMass(TV):
    NAME = "TestVesselMass"
    TITLE = "#autoLOC_8000113"
    INFO = "#autoLOC_8004042"
    SAVE = [("mass", "mass", "double", None), ("comparisonOperator", "comparisonOperator", "string", None)]
    LOAD = [("mass", "mass", "double", None, None), ("comparisonOperator", "comparisonOperator", "enum", "LG", "GreaterThan")]

    def awake(self):
        TV.awake(self)
        self.mass = 25.0
        self.comparisonOperator = "GreaterThan"

    def test(self, env):
        self.tv_test(env)
        if self.vessel is not None:
            return ltgt(self.comparisonOperator, v_mass(self.vessel), self.mass)
        return False

    def body(self, field, env):
        if field["name"] == "mass":
            return fmt("#autoLOC_8100175", "<" if self.comparisonOperator == "LessThan" else ">", fixed(self.mass, 2))
        return self.base_body(field)


@test_class
class TestVesselPartCount(TV):
    NAME = "TestVesselPartCount"
    TITLE = "#autoLOC_8100293"
    INFO = "#autoLOC_8000305"
    SAVE = [("numberOfParts", "parts", "int", None), ("comparisonOperator", "comparisonOperator", "string", None)]
    LOAD = [("numberOfParts", "parts", "int", None, None), ("comparisonOperator", "comparisonOperator", "enum", "Cmp5", "Equal")]

    def awake(self):
        TV.awake(self)
        self.parts = 1
        self.comparisonOperator = "Equal"

    def test(self, env):
        self.tv_test(env)
        if self.vessel is not None:
            return cmp5(self.comparisonOperator, len(self.vessel["partMasses"]), self.parts)
        return False

    def body(self, field, env):
        if field["name"] == "parts":
            return fmt("#autoLOC_8100154", field["guiName"], opstr(self.comparisonOperator), str(self.parts))
        return self.base_body(field)


@test_class
class TestVesselCrewCount(TV):
    NAME = "TestVesselCrewCount"
    TITLE = "#autoLOC_8000106"
    INFO = "#autoLOC_8004041"
    SAVE = [("crewAmount", "crewAmount", "int", None), ("traitName", "traitName", "string", None),
            ("comparisonOperator", "comparisonOperator", "string", None)]
    LOAD = [("crewAmount", "crewAmount", "int", None, None), ("traitName", "traitName", "string", None, None),
            ("comparisonOperator", "comparisonOperator", "enum", "Cmp5", "Equal")]

    def awake(self):
        TV.awake(self)
        self.crewAmount = 0
        self.traitName = ""
        self.comparisonOperator = "LessThan"

    def test(self, env):
        self.tv_test(env)
        if self.vessel is not None:
            crew = self.vessel["crew"]
            if self.traitName == "":
                count = len(crew)
            else:
                count = sum(1 for t in crew if t == self.traitName or self.traitName == "Any Role")
            return cmp5(self.comparisonOperator, count, self.crewAmount)
        return False

    def body(self, field, env):
        if field["name"] == "traitName":
            if field["value"] != "" and field["value"] != "Any Role":
                return fmt("#autoLOC_8004190", field["guiName"], field["value"])
            return fmt("#autoLOC_8004190", field["guiName"], "#autoLOC_8002020")
        if field["name"] == "crewAmount":
            return fmt("#autoLOC_8100154", field["guiName"], opstr(self.comparisonOperator), str(self.crewAmount))
        return self.base_body(field)


@test_class
class TestVesselAltitude(TV):
    NAME = "TestVesselAltitude"
    TITLE = "#autoLOC_8000092"
    INFO = "#autoLOC_8004040"
    SAVE = [("altitude", "altitude", "double", None), ("useRadarAltimiter", "useRadarAltimiter", "bool", None),
            ("comparisonOperator", "comparisonOperator", "string", None)]
    LOAD = [("altitude", "altitude", "double", None, None), ("useRadarAltimiter", "useRadarAltimiter", "bool", None, None),
            ("comparisonOperator", "comparisonOperator", "enum", "LG", "GreaterThan")]

    def awake(self):
        TV.awake(self)
        self.altitude = 0.0
        self.useRadarAltimiter = False
        self.comparisonOperator = "GreaterThan"

    def test(self, env):
        self.tv_test(env)
        if self.vessel is not None:
            cmp_value = self.vessel["radarAltitude"] if self.useRadarAltimiter else self.vessel["altitude"]
            return ltgt(self.comparisonOperator, cmp_value, self.altitude)
        return False

    def body(self, field, env):
        if field["name"] == "altitude":
            return fmt("#autoLOC_8100035", "<" if self.comparisonOperator == "LessThan" else ">", dnum(self.altitude))
        if field["name"] == "useRadarAltimiter":
            return fmt("#autoLOC_8100167") if not self.useRadarAltimiter else fmt("#autoLOC_8100036")
        return self.base_body(field)


@test_class
class TestVesselActiveCount(Mod):
    NAME = "TestVesselActiveCount"
    TITLE = "#autoLOC_8001038"
    TITLE_FMT = False
    INFO = "#autoLOC_8001039"
    SAVE = [("vesselType", "vesselType", "string", None), ("comparisonOperator", "comparisonOperator", "string", None),
            ("vesselCount", "vesselCount", "int", None)]
    LOAD = [("vesselType", "vesselType", "enum", "VesselType", "All"), ("comparisonOperator", "comparisonOperator", "enum", "Cmp5", "GreaterThan"),
            ("vesselCount", "vesselCount", "int", None, None)]

    def awake(self):
        Mod.awake(self)
        self.vesselType = "All"
        self.comparisonOperator = "LessThan"
        self.vesselCount = 0
        self.countCompare = 0

    def update(self, v, value):
        if v["type"] in ("EVA", "Flag", "DeployedSciencePart", "DeployedScienceController"):
            return
        t = self.vesselType
        if t == "All":
            self.countCompare += value
        elif t == "PlayerControlled":
            if v["discovery"] == "Owned":
                self.countCompare += value
        elif t == "Asteroids":
            if "ModuleAsteroid" in v["modules"]:
                self.countCompare += value
        elif t == "Comets":
            if "ModuleComet" in v["modules"]:
                self.countCompare += value
        elif t == "Unowned":
            if v["discovery"] == "Unowned":
                self.countCompare += value

    def initialized(self, env):
        self.countCompare = 0
        for v in env.vessels:
            self.update(v, 1)

    def on_event(self, ev, env):
        if ev["name"] == "addVessel":
            self.update(env.vessels[ev["vessel"]], 1)
        elif ev["name"] == "removeVessel":
            self.update(env.vessels[ev["vessel"]], -1)

    def test(self, env):
        return cmp5(self.comparisonOperator, self.countCompare, self.vesselCount)

    def body(self, field, env):
        if field["name"] == "vesselCount":
            return fmt("#autoLOC_8100154", field["guiName"], opstr(self.comparisonOperator), str(self.vesselCount))
        return self.base_body(field)

    def score_modifier(self, module, env):
        return {"vessel": None, "number": None}


@test_class
class TestVesselResource(TV):
    NAME = "TestVesselResource"
    TITLE = "#autoLOC_8000085"
    INFO = "#autoLOC_8004028"
    SAVE = [("resourceName", "resourceName", "string", None), ("resourceAmount", "resourceAmount", "double", None),
            ("comparisonOperator", "comparisonOperator", "string", None)]
    LOAD = [("resourceName", "resourceName", "string", None, None), ("resourceAmount", "resourceAmount", "double", None, None),
            ("comparisonOperator", "comparisonOperator", "enum", "Cmp5", "Equal")]
    SCORE_VESSEL = False

    def awake(self):
        TV.awake(self)
        self.resourceName = "LiquidFuel"
        self.resourceAmount = 0.0
        self.comparisonOperator = "LessThan"

    def test(self, env):
        self.tv_test(env)
        if self.vessel is not None:
            res = self.vessel["resources"]
            if self.resourceName not in env.d.get("resourceDefs", []):
                return False
            current = res.get(self.resourceName, 0.0)
            op = self.comparisonOperator
            if op == "Equal":
                return abs(current - self.resourceAmount) < 1e-15
            return cmp5(op, current, self.resourceAmount)
        return False

    def body(self, field, env):
        if field["name"] == "resourceAmount":
            disp = {"LessThan": "< ", "LessThanorEqual": "<= ", "Equal": "= ", "GreaterThanorEqual": ">= "}.get(self.comparisonOperator, "> ")
            if self.resourceAmount < 1000.0:
                return disp + fixed(self.resourceAmount, 1)
            return disp + fixed(self.resourceAmount, 0)
        if field["name"] == "resourceName":
            defs = env.d.get("resourceDefs", {})
            if self.resourceName in defs:
                return defs[self.resourceName]
            return field["value"]
        return self.base_body(field)


@test_class
class TestVesselStage(TV):
    NAME = "TestVesselStage"
    TITLE = "#autoLOC_8000124"
    INFO = "#autoLOC_8004043"
    SAVE = [("stage", "stage", "int", None), ("comparisonOperator", "comparisonOperator", "string", None)]
    LOAD = [("stage", "stage", "int", None, None), ("comparisonOperator", "comparisonOperator", "enum", "Cmp5", "GreaterThan")]

    def awake(self):
        TV.awake(self)
        self.stage = 0
        self.comparisonOperator = "GreaterThan"

    def test(self, env):
        self.tv_test(env)
        if self.vessel is not None:
            return cmp5(self.comparisonOperator, self.vessel["currentStage"], self.stage)
        return False

    def body(self, field, env):
        if field["name"] == "stage":
            return fmt("#autoLOC_8100154", field["guiName"], opstr(self.comparisonOperator), str(self.stage))
        return self.base_body(field)


@test_class
class TestVesselStageActivated(TV):
    NAME = "TestVesselStageActivated"
    TITLE = "#autoLOC_8000129"
    INFO = "#autoLOC_8004032"
    SAVE = [("activation", "activation", "int", None)]
    LOAD = [("activation", "activation", "int", None, None)]

    def awake(self):
        TV.awake(self)
        self.activation = 0
        self.eventFound = False

    def initialized(self, env):
        self.eventFound = False
        self.subscribed = True

    def cleared(self, env):
        self.subscribed = False

    def on_event(self, ev, env):
        if ev["name"] == "stageActivate" and getattr(self, "subscribed", False):
            self.tv_test(env)
            if self.vessel is not None and vkey(self.vessel) == vkey(env.active()) and ev["arg"] == self.activation:
                self.eventFound = True

    def test(self, env):
        return self.eventFound


@test_class
class TestVesselState(TV):
    NAME = "TestVesselState"
    TITLE = "#autoLOC_8000121"
    SAVE = [("state", "vesselState", "string", None)]
    LOAD = [("state", "vesselState", "enum", "VesselState", "INACTIVE")]

    def awake(self):
        TV.awake(self)
        self.vesselState = "INACTIVE"

    def test(self, env):
        self.tv_test(env)
        if self.vessel is not None:
            return self.vessel["state"] == self.vesselState
        return False


@test_class
class TestVesselRecovery(TV):
    NAME = "TestVesselRecovery"
    TITLE = "#autoLOC_8001046"
    TITLE_FMT = False
    INFO = "#autoLOC_8001047"
    SCORE_VESSEL = False
    USE_ACTIVE = False

    def awake(self):
        TV.awake(self)
        self.eventFound = False
        self.subscribed = False

    def initialized(self, env):
        self.subscribed = True

    def cleared(self, env):
        self.subscribed = False

    def on_event(self, ev, env):
        if ev["name"] == "recoveryComplete" and self.subscribed:
            if self.vesselID == ev["pid"] or self.vesselID == 0:
                self.eventFound = True

    def test(self, env):
        return self.eventFound


@test_class
class TestFundsRecovery(Mod):
    NAME = "TestFundsRecovery"
    TITLE = "#autoLOC_8000233"
    INFO = "#autoLOC_8004016"
    SAVE = [("comparisonOperator", "comparisonOperator", "string", None), ("fundsRecovered", "fundsToRecover", "double", None)]
    LOAD = [("comparisonOperator", "comparisonOperator", "enum", "Cmp5", "GreaterThan"), ("fundsRecovered", "fundsToRecover", "double", None, None)]

    def awake(self):
        Mod.awake(self)
        self.fundsToRecover = 0.0
        self.comparisonOperator = "LessThan"
        self.eventFound = False
        self.fundsCollected = 0.0
        self.subscribed = False

    def initialized(self, env):
        self.subscribed = True

    def cleared(self, env):
        self.subscribed = False

    def on_event(self, ev, env):
        if ev["name"] == "recoveryComplete" and self.subscribed:
            self.fundsCollected = ev["fundsEarned"]
            op = self.comparisonOperator
            if op == "LessThan" or op == "LessThanorEqual":
                # C# bug kept: LessThanorEqual also compares with <
                self.eventFound = self.fundsCollected < self.fundsToRecover
            elif op == "Equal":
                if abs(self.fundsCollected - self.fundsToRecover) < 1e-15:
                    self.eventFound = True
            elif op == "GreaterThanorEqual":
                self.eventFound = self.fundsCollected >= self.fundsToRecover
            elif op == "GreaterThan":
                self.eventFound = self.fundsCollected > self.fundsToRecover

    def test(self, env):
        return self.eventFound

    def body(self, field, env):
        if field["name"] == "fundsToRecover":
            return fmt("#autoLOC_8100154", field["guiName"], opstr(self.comparisonOperator), "%d" % round_half_up(self.fundsToRecover))
        return self.base_body(field)

    def score_modifier(self, module, env):
        return {"vessel": None, "number": None}


@test_class
class TestResourcesRecovery(TV):
    NAME = "TestResourcesRecovery"
    TITLE = "#autoLOC_8000167"
    INFO = "#autoLOC_8004029"
    SAVE = [("comparisonOperator", "comparisonOperator", "string", None), ("resourcesToRecover", "resourcesToRecover", "double", None),
            ("resourceName", "resourceName", "string", None)]
    LOAD = [("comparisonOperator", "comparisonOperator", "enum", "Cmp5", "GreaterThan"),
            ("resourcesToRecover", "resourcesToRecover", "double", None, None), ("resourceName", "resourceName", "string", None, None)]

    def awake(self):
        TV.awake(self)
        self.resourceName = "LiquidFuel"
        self.resourcesToRecover = 0.0
        self.comparisonOperator = "LessThan"
        self.eventFound = False
        self.subscribed = False

    def initialized(self, env):
        self.subscribed = True

    def cleared(self, env):
        self.subscribed = False

    def on_event(self, ev, env):
        if ev["name"] == "recoveryProcessing" and self.subscribed:
            self.tv_test(env)
            if (self.vessel is None or self.vessel.get("id") != ev["vesselGuid"]) and self.vesselID != 0:
                return
            defs = env.d.get("resourceDefs", {})
            if self.resourceName not in defs:
                return
            recovered = 0.0
            for part in ev["parts"]:
                for r in part:
                    if r["name"] in defs and r["name"] == self.resourceName:
                        recovered += r["amount"]
            op = self.comparisonOperator
            if op == "Equal":
                self.eventFound = abs(recovered - self.resourcesToRecover) < 1e-15
            else:
                self.eventFound = cmp5(op, recovered, self.resourcesToRecover)

    def test(self, env):
        return self.eventFound

    def body(self, field, env):
        if field["name"] == "resourcesToRecover":
            return fmt("#autoLOC_8100154", field["guiName"], opstr(self.comparisonOperator), "%d" % round_half_up(self.resourcesToRecover))
        return self.base_body(field)

    def score_modifier(self, module, env):
        return TV.score_modifier(self, module, env)


class SOI(TV):
    SAVE = []

    def awake(self):
        TV.awake(self)
        self.bodyName = env_home_body()
        self.eventFound = False
        self.subscribed = False

    def initialized(self, env):
        self.eventFound = False
        self.subscribed = True

    def cleared(self, env):
        self.subscribed = False

    def valid(self, name):
        return self.bodyName == "AnyValid" or self.bodyName == name

    def save_fields(self, cn):
        TV.save_fields(self, cn)
        cn.values.append(("bodyName", self.bodyName))

    def load_fields(self, cfg):
        TV.load_fields(self, cfg)
        v = cfg.get("bodyName")
        if v is not None and v != "":
            # MissionCelestialBody.Load (:104): "AnyValid" or an unknown body name -> body = null (saved as AnyValid)
            self.bodyName = v if v in KNOWN_BODIES else "AnyValid"

    def test(self, env):
        return self.eventFound


KNOWN_BODIES = ["Kerbin", "Mun", "Minmus", "Duna"]


def env_home_body():
    return "Kerbin"


@test_class
class TestSOILeft(SOI):
    NAME = "TestSOILeft"
    TITLE = "#autoLOC_8000155"
    INFO = "#autoLOC_8004013"

    def on_event(self, ev, env):
        if ev["name"] == "soi" and self.subscribed:
            self.tv_test(env)
            if vkey(self.vessel) == ev["vessel"] and self.vessel is not None and self.valid(ev["from"]):
                self.eventFound = True


@test_class
class TestSOIReached(SOI):
    NAME = "TestSOIReached"
    TITLE = "#autoLOC_8000164"
    INFO = "#autoLOC_8004027"

    USE_ACTIVE = False

    def on_event(self, ev, env):
        if ev["name"] == "soi" and self.subscribed:
            self.tv_test(env)
            if vkey(self.vessel) == ev["vessel"] and self.vessel is not None and self.valid(ev["to"]):
                self.eventFound = True
