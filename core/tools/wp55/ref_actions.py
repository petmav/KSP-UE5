"""WP-55 reference, part 3: the Actions.* modules, written from Assembly-CSharp/Expansions/Missions/Actions/*.cs and VesselPartIDPair.cs."""
from ref_base import *  # noqa: F401,F403
from ref_base import Cfg, Mod, ACTIONS, f32, fnum, dnum, fmt, tag, parse, vtext, action_class, ENUMS
from ref_tests import Env, vkey, fixed, round_half_up


def esc(s):
    return s.replace("\n", "\\n").replace("\t", "\\t")


def unesc(s):
    return s.replace("\\n", "\n").replace("\\t", "\t")


@action_class
class ActionMessage(Mod):
    NAME = "ActionMessage"
    TITLE = "#autoLOC_8000031"
    INFO = "#autoLOC_8004021"

    def awake(self):
        Mod.awake(self)
        self.message = ""
        self.duration = 5.0
        self.style = "UPPER_LEFT"

    def save_fields(self, cn):
        cn.values += [("message", esc(self.message)), ("duration", fnum(self.duration)), ("style", self.style)]

    def load_fields(self, cfg):
        v = cfg.get("message")
        if v is not None:
            self.message = v
        self.message = unesc(self.message)
        v = parse(cfg.get("duration") or "", "float")
        if v is not None:
            self.duration = v
        s = cfg.get("style")
        self.style = s if (s is not None and s in ENUMS["ScreenStyle"]) else "UPPER_LEFT"

    def fire(self, env):
        return {"screen": [{"text": self.message, "duration": fnum(self.duration), "style": self.style, "persist": True}]}


@action_class
class ActionDialogMessage(Mod):
    NAME = "ActionDialogMessage"
    TITLE = "#autoLOC_8006007"
    INFO = "#autoLOC_8004012"

    def awake(self):
        Mod.awake(self)
        self.messageHeading = ""
        self.message = ""
        self.instructorName = "Instructor_Gene"
        self.vintageSuit = False
        self.textAreaSize = 135
        self.pauseMission = False
        self.screenArea = "Center"
        self.autoClose = False
        self.autoCloseTimeOut = 20
        self.isBriefingMessage = False
        self.autoGrowDialogHeight = False

    def save_fields(self, cn):
        if self.messageHeading:
            cn.values.append(("messageHeading", self.messageHeading))
        if self.message:
            cn.values.append(("message", esc(self.message)))
        if self.instructorName:
            cn.values.append(("instructorName", self.instructorName))
        cn.values += [("vintageSuit", vtext(self.vintageSuit, "bool")), ("textAreaSize", str(self.textAreaSize)),
                      ("pauseMission", vtext(self.pauseMission, "bool")), ("screenArea", self.screenArea),
                      ("autoClose", vtext(self.autoClose, "bool")), ("autoCloseTimeOut", str(self.autoCloseTimeOut)),
                      ("isBriefingMessage", vtext(self.isBriefingMessage, "bool")),
                      ("autoGrowDialogHeight", vtext(self.autoGrowDialogHeight, "bool"))]

    def load_fields(self, cfg):
        v = cfg.get("messageHeading")
        if v is not None:
            self.messageHeading = v
        v = cfg.get("message")
        if v is not None:
            self.message = v
        self.message = unesc(self.message)
        v = parse(cfg.get("textAreaSize") or "", "int")
        if v is not None:
            self.textAreaSize = v
        v = parse(cfg.get("pauseMission") or "", "bool")
        if v is not None:
            self.pauseMission = v
        s = cfg.get("screenArea")
        self.screenArea = s if (s is not None and s in ENUMS["DialogArea"]) else "Center"
        for key in ("autoClose", "isBriefingMessage"):
            v = parse(cfg.get(key) or "", "bool")
            if v is not None:
                setattr(self, key, v)
        v = parse(cfg.get("autoCloseTimeOut") or "", "int")
        if v is not None:
            self.autoCloseTimeOut = v
        # missionInstructor.Load: instructorName = "" then TryGetValue; vintageSuit
        self.instructorName = cfg.get("instructorName") or ""
        v = parse(cfg.get("vintageSuit") or "", "bool")
        if v is not None:
            self.vintageSuit = v
        g = parse(cfg.get("autoGrowDialogHeight") or "", "bool")
        if g is not None:
            self.autoGrowDialogHeight = g
        else:
            m = parse(cfg.get("maxDialogHeight") or "", "bool")
            if m is not None:
                self.autoGrowDialogHeight = m

    def fire(self, env):
        paused = bool(self.pauseMission)
        name = self.instructorName.replace("Instructor_", "GAPKerbal_").replace("Strategy_", "GAPKerbal_")
        name2 = ""
        if "Veteran" in name:
            if self.vintageSuit:
                name += "_Vintage"
            if "Jeb_" in name:
                name2 = fmt("#autoLOC_20803")
                name = name.replace("Jeb_", "")
            elif "Bill_" in name:
                name2 = fmt("#autoLOC_20811")
                name = name.replace("Bill_", "")
            elif "Bob_" in name:
                name2 = fmt("#autoLOC_20819")
                name = name.replace("Bob_", "")
            elif "Val_" in name:
                name2 = fmt("#autoLOC_20827")
                name = name.replace("Val_", "")
        return {"isPaused": paused,
                "dialog": {"heading": self.messageHeading, "message": self.message, "instructor": name, "button": "#autoLOC_8000259",
                           "textAreaSize": self.textAreaSize, "area": self.screenArea, "autoClose": self.autoClose,
                           "timeout": self.autoCloseTimeOut, "grow": self.autoGrowDialogHeight, "instructor2": name2},
                "systemMessage": [self.messageHeading, self.message]}

    def body(self, field, env):
        if field["name"] == "missionInstructor":
            return instructor_body(self.instructorName)
        return self.base_body(field)


def instructor_body(name):
    """MissionInstructor.GetNodeBodyParameterString (MissionInstructor.cs:43)."""
    table = {"Instructor_Wernher": "#autoLOC_900000", "Instructor_Gene": "#autoLOC_900001", "Strategy_MechanicGuy": "#autoLOC_900002",
             "Strategy_Mortimer": "#autoLOC_900004", "Strategy_PRGuy": "#autoLOC_900005", "Strategy_ScienceGuy": "#autoLOC_900006",
             "Instructor_Jeb_MaleVeteran": "#autoLOC_20803", "Instructor_Bill_MaleVeteran": "#autoLOC_20811",
             "Instructor_Bob_MaleVeteran": "#autoLOC_20819", "Instructor_Val_FemaleVeteran": "#autoLOC_20827"}
    if name in table:
        return fmt("#autoLOC_8006002") + ": " + fmt(table[name])
    return ""


@action_class
class ActionModifierFunds(Mod):
    NAME = "ActionModifierFunds"
    TITLE = "#autoLOC_8003125"
    TITLE_FMT = False
    INFO = "#autoLOC_8003124"

    def awake(self):
        Mod.awake(self)
        self.modifierType = "Add"
        self.modifierFunds = 0.0

    def save_fields(self, cn):
        cn.values += [("type", self.modifierType), ("modifier", fnum(self.modifierFunds))]

    def load_fields(self, cfg):
        t = cfg.get("type")
        self.modifierType = t if (t is not None and t in ENUMS["FundsMod"]) else "Multiply"
        v = parse(cfg.get("modifier") or "", "float")
        if v is not None:
            self.modifierFunds = v

    def fire(self, env):
        if env.funds is None:
            return {"funds": None}
        funds = env.funds
        m = self.modifierFunds
        t = self.modifierType
        if t == "Add":
            funds = funds + m
        elif t == "Multiply":
            num = funds * m
            funds = funds + (num - funds)
        elif t == "Divide":
            num = funds / m
            funds = funds + (num - funds)
        elif t == "Substract":
            funds = funds + f32(0.0 - m)
        elif t == "Set":
            funds = funds + (m - funds)
        return {"funds": dnum(funds)}


class ScoreActionBase(Mod):
    """ActionMissionScore / ActionModifierScore: the MissionScore holds ScoreModule_Completion / ScoreModule_Modifier modules."""
    MODULE = "ScoreModule_Completion"
    SAVE = []

    def awake(self):
        Mod.awake(self)
        self.module = self.MODULE
        self.bonusScore = 0.0
        self.modifierType = "Multiply"
        self.modifierScore = 0.0

    def load_fields(self, cfg):
        for modules in [n for n in cfg.nodes if n.name == "MODULES"]:
            for m in [n for n in modules.nodes if n.name == "MODULE"]:
                name = m.get("name")
                self.module = name
                if name == "ScoreModule_Completion":
                    v = parse(m.get("bonusScore") or "", "float")
                    if v is not None:
                        self.bonusScore = v
                elif name == "ScoreModule_Modifier":
                    t = m.get("type")
                    self.modifierType = t if (t is not None and t in ENUMS["ScoreMod"]) else "Multiply"
                    v = parse(m.get("modifier") or "", "float")
                    if v is not None:
                        self.modifierScore = v

    def score_description(self):
        if self.module == "ScoreModule_Completion":
            return fmt("#autoLOC_8005404") + " " + fmt("#autoLOC_8100149", fmt("#autoLOC_8100148"), fnum(self.bonusScore))
        return fmt("#autoLOC_8005404") + " " + modifier_description(self.modifierType, self.modifierScore)

    def award(self, cur):
        if self.module == "ScoreModule_Completion":
            return f32(cur + self.bonusScore)
        t, m = self.modifierType, self.modifierScore
        if t == "Multiply":
            return f32(cur * m)
        if t == "Divide":
            return f32(cur / m)
        if t == "Substract":
            return f32(cur - m)
        return m

    def fire(self, env):
        if env.d.get("scoreEnabled", True):
            return {"score": fnum(self.award(env.score))}
        return {"score": fnum(env.score)}

    def info(self):
        return fmt(self.INFO)


ENUMS["ScoreMod"] = ["Multiply", "Divide", "Substract", "Set"]


def modifier_description(t, m):
    """ScoreModule_Modifier.ScoreDescription (ScoreModule_Modifier.cs:68)."""
    desc = {"Multiply": "#autoLOC_8100127", "Divide": "#autoLOC_8100128", "Substract": "#autoLOC_8100129", "Set": "#autoLOC_8100130"}
    name = "#autoLOC_8100125"
    if t in ("Multiply", "Divide"):
        return fmt("#autoLOC_8100154", name, desc[t], fnum(m))
    if t == "Substract":
        text = fmt("#autoLOC_8005414") + " " + fnum(m) + " " + fmt("#autoLOC_8100147")
        return fmt("#autoLOC_8004190", name, text)
    return fmt("#autoLOC_8006050", name, fnum(m))


@action_class
class ActionMissionScore(ScoreActionBase):
    NAME = "ActionMissionScore"
    TITLE = "#autoLOC_8000078"
    INFO = "#autoLOC_8004002"


@action_class
class ActionModifierScore(ScoreActionBase):
    NAME = "ActionModifierScore"
    TITLE = "#autoLOC_8000078"
    INFO = "#autoLOC_8004002"
    MODULE = "ScoreModule_Modifier"


@action_class
class ActionVessel(Mod):
    NAME = "ActionVessel"
    TITLE = None
    USE_ACTIVE = False
    SAVE = []

    def awake(self):
        Mod.awake(self)
        self.vesselID = 0
        self.oldRootPart = 0

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

    def body(self, field, env):
        if field["name"] == "vesselID":
            return fmt("#autoLOC_8000069", fmt("#autoLOC_8004217") if self.USE_ACTIVE else fmt("#autoLOC_8001004"))
        return self.base_body(field)

    def vessel(self, env):
        if self.vesselID == 0:
            return env.active()
        return env.by_pid(self.vesselID)

    def fire(self, env):
        return {}


@action_class
class ActionVesselActionGroup(Mod):
    NAME = "ActionVesselActionGroup"
    TITLE = "#autoLOC_8003179"
    INFO = "#autoLOC_8003180"
    SAVE = [("actionGroup", "actionGroup", "string", None)]
    LOAD = [("actionGroup", "actionGroup", "enum", "ActionGroup", "None")]

    def awake(self):
        Mod.awake(self)
        self.actionGroup = "None"

    def fire(self, env):
        a = env.active()
        if a is None:
            return {"waits": [], "toggled": None}
        return {"waits": [fnum(f32(0.1))], "toggled": self.actionGroup}


@action_class
class ActionVesselExplode(ActionVessel):
    NAME = "ActionVesselExplode"
    TITLE = "#autoLOC_8000040"
    INFO = "#autoLOC_8004001"
    USE_ACTIVE = True

    def awake(self):
        ActionVessel.awake(self)
        self.partsExplodeTime = 0.0
        self.restart = True

    def save_fields(self, cn):
        ActionVessel.save_fields(self, cn)
        cn.values.append(("explodeTime", fnum(self.partsExplodeTime)))

    def load_fields(self, cfg):
        ActionVessel.load_fields(self, cfg)
        v = parse(cfg.get("explodeTime") or "", "float")
        if v is not None:
            self.partsExplodeTime = v

    def body(self, field, env):
        if field["name"] == "partsExplodeTime":
            return fmt("#autoLOC_8006045", fnum(self.partsExplodeTime))
        return ActionVessel.body(self, field, env)

    def fire(self, env):
        v = self.vessel(env)
        if v is None:
            return {"exploded": [], "waits": [], "state": None, "isRunning": False}
        n = len(v["partMasses"])
        wait = f32(self.partsExplodeTime / 1000.0)
        return {"exploded": list(range(n)), "waits": [fnum(wait)] * n, "state": "DEAD", "isRunning": False, "pid": v["pid"]}


@action_class
class ActionVesselStage(Mod):
    NAME = "ActionVesselStage"
    TITLE = "#autoLOC_8000043"
    INFO = "#autoLOC_8004005"

    def fire(self, env):
        if env.active() is None:
            return {"waits": [], "stages": 0, "toggled": None}
        return {"waits": [fnum(f32(0.1))], "stages": 1, "toggled": "Stage"}


@action_class
class ActionSetCamera(Mod):
    NAME = "ActionSetCamera"
    TITLE = "#autoLOC_8004179"
    INFO = "#autoLOC_8004180"

    def awake(self):
        Mod.awake(self)
        self.cameraLockMode = "NoChange"
        self.cameraLockOptions = "NoChange"

    def save_fields(self, cn):
        cn.values += [("cameraTypeOptions", self.cameraLockMode), ("cameraLockOptions", self.cameraLockOptions)]

    def load_fields(self, cfg):
        m = cfg.get("cameraTypeOptions")
        self.cameraLockMode = m if (m is not None and m in ENUMS["CamMode"]) else "NoChange"
        o = cfg.get("cameraLockOptions")
        self.cameraLockOptions = o if (o is not None and o in ENUMS["CamLock"]) else "Unlock"

    def fire(self, env):
        return {"camera": [self.cameraLockMode, self.cameraLockOptions]}


EPS = 1e-15


def adjust_unloaded_part(resources, name, adjust, pulling):
    """ActionPartResourceAmount.adjustUnloadedPart (ActionPartResourceAmount.cs:75); resources = [{"name","amount","max"}] mutated."""
    num = 0.0
    adjust = abs(adjust)
    i = 0
    snap = None
    while True:
        if i < len(resources):
            snap = resources[i]
            if snap["name"] == name:
                if pulling:
                    num3 = snap["amount"]
                    if num3 > 0.0:
                        if not (num3 < adjust):
                            snap["amount"] -= adjust
                            return num + adjust
                        snap["amount"] = 0.0
                        num += num3
                        adjust += num3
                else:
                    num3 = snap["max"] - snap["amount"]
                    if num3 > 0.0:
                        if not (num3 < adjust):
                            break
                        snap["amount"] = snap["max"]
                        num += num3
                        adjust -= num3
            i += 1
            continue
        return num
    snap["amount"] += adjust
    return num + adjust


@action_class
class ActionPartResourceAmount(ActionVessel):
    NAME = "ActionPartResourceAmount"
    TITLE = "#autoLOC_8000016"
    INFO = "#autoLOC_8004000"
    USE_ACTIVE = True

    def awake(self):
        ActionVessel.awake(self)
        self.resourceName = ""
        self.adjustAmount = 0.0

    def save_fields(self, cn):
        ActionVessel.save_fields(self, cn)
        cn.values += [("resourceName", self.resourceName), ("adjustAmount", fnum(self.adjustAmount))]

    def load_fields(self, cfg):
        ActionVessel.load_fields(self, cfg)
        v = cfg.get("resourceName")
        if v is not None:
            self.resourceName = v
        v = parse(cfg.get("adjustAmount") or "", "float")
        if v is not None:
            self.adjustAmount = v

    def body(self, field, env):
        if field["name"] == "resourceName":
            text = fmt("#autoLOC_6003083")
            defs = env.d.get("resourceDefs", {})
            if self.resourceName and self.resourceName in defs:
                text = defs[self.resourceName]
            return fmt("#autoLOC_8004190", field["guiName"], text)
        return ActionVessel.body(self, field, env)

    def fire(self, env):
        v = self.vessel(env)
        if v is None:
            return {}
        if v["loaded"]:
            demand = f32(0.0 - self.adjustAmount)
            got = env.d.get("requestResult", 0.0)
            return {"requestDemand": dnum(demand), "event": 1 if got > 0.0 else 0}
        num = float(self.adjustAmount)
        pulling = self.adjustAmount < 0.0
        events = 0
        proto = [[dict(r) for r in part] for part in v["proto"]]
        for part in proto:
            got = adjust_unloaded_part(part, self.resourceName, num, pulling)
            num = (num - got) if not pulling else (num + got)
            if got > 0.0:
                events += 1
            if abs(num) <= EPS:
                break
        return {"events": events, "amounts": [[dnum(r["amount"]) for r in part] for part in proto]}


@action_class
class ActionPartExplode(Mod):
    NAME = "ActionPartExplode"
    TITLE = "#autoLOC_8000060"
    INFO = "#autoLOC_8004003"

    def awake(self):
        Mod.awake(self)
        self.VesselID = 0
        self.partID = 0
        self.partName = "#autoLOC_8001004"

    def save_fields(self, cn):
        cn.values += [("partPersistentId", str(self.partID)), ("vesselPersistentId", str(self.VesselID)), ("partName", self.partName)]

    def load_fields(self, cfg):
        for key, attr in (("partPersistentId", "partID"), ("vesselPersistentId", "VesselID")):
            v = parse(cfg.get(key) or "", "uint")
            if v is not None:
                setattr(self, attr, v)
        v = cfg.get("partName")
        if v is not None:
            self.partName = v

    def body(self, field, env):
        if field["name"] == "vesselPartIDs":
            return fmt("#autoLOC_8004190", field["guiName"], fmt(self.partName))
        return self.base_body(field)

    def fire(self, env):
        if self.partID != 0:
            for vi, v in enumerate(env.vessels):
                if not v["loaded"]:
                    continue
                for pi, pid in enumerate(v["partIds"]):
                    if pid == self.partID:
                        out = {"exploded": [[vi, pi]]}
                        a = env.active()
                        if a is not None and a["partIds"][0] == self.partID:
                            out["switched"] = True
                        return out
            return {"exploded": []}
        v = env.by_pid(self.VesselID)
        if v is None:
            return {"throws": "KeyNotFoundException"}
        if v["loaded"]:
            n = len(v["partIds"])
            lo = 1 if 1 < n else 0
            idx = env.d.get("randomIndex", lo)
            return {"exploded": [[env.vessels.index(v), idx]], "randomRange": [lo, n]}
        return {"exploded": []}
