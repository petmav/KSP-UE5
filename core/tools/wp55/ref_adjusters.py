"""WP-55 reference, part 4: the Adjusters.* dynamic modules.

The simple facts of each class (constructor flags and guiName of the two constructors, field names and defaults, Activate status texts, body
texts, Save / Load keys) are READ FROM THE DECOMPILED SOURCE (regexes over Adjusters/*.cs), not transcribed; the Apply* maths and the part
module calls are written out and checked against the source text (the return expression must match)."""
import os
import re

from ref_base import *  # noqa: F401,F403
from ref_base import Cfg, Mod, DYNAMICS, f32, fnum, dnum, fmt, tag, parse, vtext, dynamic_class, ENUMS, MISSIONS, f32bits, dbits

ADJ_DIR = os.path.join(MISSIONS, "Adjusters")
KSP_ACTION = {"Activate": 0, "Deactivate": 1}


def read_cs(name):
    return open(os.path.join(ADJ_DIR, name + ".cs"), encoding="utf-8-sig").read()


def block(src, header):
    """The text of the method whose declaration contains `header` (up to its closing brace at the method indent)."""
    i = src.index(header)
    j = src.index("{", i)
    depth = 0
    k = j
    while True:
        c = src[k]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return src[j + 1:k]
        k += 1


def eval_str(expr):
    expr = expr.strip()
    m = re.fullmatch(r'"([^"]*)"', expr)
    if m:
        return m.group(1)
    m = re.fullmatch(r'Localizer\.Format\("([^"]*)"\)', expr)
    if m:
        return fmt(m.group(1))
    raise ValueError(expr)


def round_even(x):
    # Mathf.Round: Math.Round, banker's rounding
    return float(round(x))


class Plan:
    """What the C# source of one adjuster says (parsed once)."""

    def __init__(self, name):
        self.name = name
        src = read_cs(name)
        self.src = src
        # constructors
        self.ctor = []
        for pat in (r"public %s\(\)\s*\{" % name, r"public %s\(MENode node\)\s*:\s*base\(node\)\s*\{" % name):
            m = re.search(pat, src)
            body = block(src, m.group(0))
            flags = set(re.findall(r"(disableKSPFields|disableKSPActions|disableKSPEvents|canBeRepaired) = true;", body))
            gm = re.search(r"guiName = ([^;]+);", body)
            self.ctor.append({"flags": flags, "guiName": eval_str(gm.group(1))})
        # fields with initialisers: [MEGUI...] public float x = 10f;
        self.fields = []
        for m in re.finditer(r"public (float|int|FailureActivationState|FailureDeploymentState|FailureOpenState) (\w+)(?: = ([-\d.]+)f?)?;", src):
            self.fields.append((m.group(1), m.group(2), m.group(3)))
        # Activate: the status message
        act = re.search(r"public override void Activate\(\)\s*\{", src)
        self.status = None
        if act:
            body = block(src, act.group(0))
            sm = re.search(r'UpdateStatusMessage\(Localizer\.Format\("([^"]+)"(?:, ([^;]+?))?\)\);', body)
            if sm:
                self.status = (sm.group(1), sm.group(2))
        # GetNodeBodyParameterString: [(field, tag, argexpr)]
        self.body = []
        bm = re.search(r"public override string GetNodeBodyParameterString\(BaseAPField field\)\s*\{", src)
        if bm:
            body = block(src, bm.group(0))
            for m in re.finditer(r'field\.name == "(\w+)"\)\s*\{\s*return Localizer\.Format\("([^"]+)"(?:, ([^;]+?))?\);', body):
                self.body.append((m.group(1), m.group(2), m.group(3)))
        # Save: [(key, expr)]
        self.save = []
        sm = re.search(r"public override void Save\(ConfigNode node\)\s*\{", src)
        if sm:
            body = block(src, sm.group(0))
            for m in re.finditer(r'node\.AddValue\("([^"]+)", ([^;]+?)\);', body):
                self.save.append((m.group(1), m.group(2)))
        # Load: statements in order
        self.load = []
        lm = re.search(r"public override void Load\(ConfigNode node\)\s*\{", src)
        if lm:
            body = block(src, lm.group(0))
            self.load_src = body
            for m in re.finditer(r'node\.TryGetValue\("([^"]+)", ref (\w+)\);|if \(!node\.HasValue\("([^"]+)"\)\)\s*\{\s*(\w+) \*= 100f;\s*\}|'
                                 r'if \(node\.HasValue\("stuckState"\)\)', body):
                if m.group(1):
                    self.load.append(("get", m.group(1), m.group(2)))
                elif m.group(3):
                    self.load.append(("x100", m.group(3), m.group(4)))
                else:
                    self.load.append(("enumparse", "stuckState", None))


PLANS = {}
SCOREABLE_NONE = None


def plan(name):
    if name not in PLANS:
        PLANS[name] = Plan(name)
    return PLANS[name]


class Adj(Mod):
    """AdjusterPartModuleBase + the concrete class, driven by its Plan."""
    BASE = "dynamic"
    KIND = "dynamic"
    ENUM_FIELD = None   # name of the enum type of stuckState

    def __init__(self, ctor=1, guid_counter=None):
        self.ctor_kind = ctor
        self.guid_counter = guid_counter if guid_counter is not None else [0]
        Mod.__init__(self)

    def next_guid(self):
        self.guid_counter[0] += 1
        return "55555555-0000-0000-0000-%012d" % self.guid_counter[0]

    def awake(self):
        p = plan(self.NAME)
        self.id = "1"
        self.adjusterID = self.next_guid()
        c = p.ctor[self.ctor_kind]
        self.canBeRepaired = "canBeRepaired" in c["flags"]
        self.disable = tuple(f in c["flags"] for f in ("disableKSPFields", "disableKSPActions", "disableKSPEvents"))
        self.guiName = c["guiName"]
        self.AdjusterState = "#autoLOC_8100276"
        self.vals = {}
        for typ, fname, default in p.fields:
            if typ in ("float", "int"):
                self.vals[fname] = f32(float(default)) if typ == "float" else int(default or 0)
            else:
                self.vals[fname] = "CurrentState"

    # --- DynamicModule / AdjusterPartModuleBase Save and Load
    def save(self):
        p = plan(self.NAME)
        cn = Cfg("N")
        cn.values += [("name", self.NAME), ("id", self.id), ("canBeRepaired", vtext(self.canBeRepaired, "bool")), ("adjusterID", self.adjusterID)]
        for key, expr in p.save:
            if expr == "value: true":
                cn.values.append((key, "True"))
            elif expr in self.vals:
                v = self.vals[expr]
                cn.values.append((key, fnum(v) if isinstance(v, float) else str(v)))
            elif expr.endswith(".ToString()") and expr[:-11] in self.vals:
                cn.values.append((key, self.vals[expr[:-11]]))
            elif expr.startswith("KSPUtil.WriteVector("):
                t = getattr(self, "stuckTorque", (0.0, 0.0, 0.0))
                cn.values.append((key, ",".join(fnum(c) for c in t)))
            else:
                raise ValueError((self.NAME, key, expr))
        return cn

    def load(self, cfg):
        p = plan(self.NAME)
        v = cfg.get("id")
        if v is not None:
            self.id = v
        t = cfg.get("canBeRepaired")
        if t is not None:
            self.canBeRepaired = parse(t, "bool")
        else:
            self.canBeRepaired = False
        a = cfg.get("adjusterID")
        self.adjusterID = a if a is not None else self.next_guid()
        for kind, key, field in p.load:
            if kind == "get":
                if field == "stuckTorque":
                    t = cfg.get(key)
                    if t is not None:
                        self.stuckTorque = tuple(f32(float(x)) for x in t.split(","))
                    self.loadedFromConfigNode = True
                    continue
                text = cfg.get(key)
                if text is None:
                    continue
                cur = self.vals[field]
                pv = parse(text, "float" if isinstance(cur, float) else "int")
                if pv is not None:
                    self.vals[field] = pv
            elif kind == "x100":
                if cfg.get(key) is None:
                    self.vals[field] = f32(self.vals[field] * 100.0)
            elif kind == "enumparse":
                t = cfg.get("stuckState")
                if t is not None:
                    self.vals["stuckState"] = t

    def gui_name_text(self, ctor):
        return plan(self.NAME).ctor[ctor]["guiName"]

    def display_name(self):
        return fmt(self.guiName)

    # --- Activate
    def eval_arg(self, arg):
        """A Localizer.Format argument expression of the adjusters: field | Mathf.Round(field / Nf) [/ Mf] (float math, Mono rule)."""
        arg = arg.strip()
        m = re.fullmatch(r"Mathf\.Round\((\w+) / ([\d.]+)f\)(?: / ([\d.]+)f)?", arg)
        if m:
            x = f32(self.vals[m.group(1)] / float(m.group(2)))
            r = f32(round_even(x))
            if m.group(3):
                r = f32(r / float(m.group(3)))
            return fnum(r)
        if arg in self.vals:
            v = self.vals[arg]
            return fnum(v) if isinstance(v, float) else str(v)
        raise ValueError((self.NAME, arg))

    def status_text(self):
        p = plan(self.NAME)
        if p.status is None:
            return None
        tg, arg = p.status
        if arg is None:
            return fmt(tg)
        return fmt(tg, self.eval_arg(arg))

    def body(self, field, env=None):
        p = plan(self.NAME)
        for fname, tg, arg in p.body:
            if field["name"] == fname:
                if arg is None:
                    return fmt(tg)
                return fmt(tg, self.eval_arg(arg))
        if field["name"] == "guiName" and field["has"]:
            return field["value"]
        return self.base_body(field)


def check_expr(name, fn, expected):
    """The C# return expression of `fn` in class `name` must read `expected` (guards the hand-written maths below)."""
    src = read_cs(name)
    m = re.search(r"public override [\w<>]+ %s\([^)]*\)\s*\{\s*(.*?)\s*\}" % fn, src, re.S)
    body = m.group(1)
    norm = re.sub(r"\s+", " ", body)
    assert re.sub(r"\s+", " ", expected) in norm, (name, fn, body)


MULT = {
    # class: (apply function, field, maths) ; maths(self, x) with the Mono rule
}


def f32div100(a, m):
    return f32(a * m / 100.0)


# multiplier adjusters: (class, field, function name, kind, expected C# text)
MULT_SPECS = [
    ("AdjusterActiveRadiatorMaxEnergyTransfer", "radiatorMaxEnergyTransferMultiplier", "ApplyMaxEnergyTransferAdjustment", "double",
     "return maxEnergyTransfer * (double)radiatorMaxEnergyTransferMultiplier / 100.0;"),
    ("AdjusterAlternatorOutput", "alternatorOutputMultiplier", "ApplyOutputAdjustment", "float", "return output * alternatorOutputMultiplier / 100f;"),
    ("AdjusterControlSurfaceActuatorSpeed", "actuatorSpeedMultiplier", "ApplyActuatorSpeedAdjustment", "float",
     "return actuatorSpeed * actuatorSpeedMultiplier / 100f;"),
    ("AdjusterDeployableSolarPanelEfficiency", "solarPanelEfficiencyMultiplier", "ApplyEfficiencyAdjustment", "float",
     "return efficiency * solarPanelEfficiencyMultiplier / 100f;"),
    ("AdjusterEnginesThrottle", "engineThrottleMultiplier", "ApplyThrottleAdjustment", "float", "return throttle * engineThrottleMultiplier / 100f;"),
    ("AdjusterGeneratorEfficiency", "generatorOutputMultiplier", "ApplyEfficiencyAdjustment", "float", "return efficiency * generatorOutputMultiplier / 100f;"),
    ("AdjusterLiftingSurfaceLiftForce", "liftForceMultiplier", "ApplyLiftForceAdjustment", "vec3", "return liftForce * liftForceMultiplier / 100f;"),
    ("AdjusterLightIntensity", "lightIntensityMultiplier", "ApplyIntensityAdjustment", "float", "return intensity * lightIntensityMultiplier / 100f;"),
    ("AdjusterWheelBrakesTorque", "brakeTorqueMultiplier", "ApplyTorqueAdjustment", "float", "return torque * brakeTorqueMultiplier / 100f;"),
    ("AdjusterWheelMotorTorque", "motorTorqueMultiplier", "ApplyTorqueAdjustment", "float", "return torque * motorTorqueMultiplier / 100f;"),
]


def make_mult(spec):
    cls, field, fn, kind, expected = spec
    check_expr(cls, fn, expected)

    @dynamic_class
    class _A(Adj):
        NAME = cls

        def apply(self, args):
            m = self.vals[field]
            if kind == "double":
                return {"out": dbits(args[0] * m / 100.0)}
            if kind == "float":
                return {"out": f32bits(f32(args[0] * m / 100.0))}
            # Vector3 * float, then Vector3 / float: each operator stores its components
            scaled = [f32(c * m) for c in args]
            return {"out": [f32bits(f32(c / 100.0)) for c in scaled]}

    _A.__name__ = cls
    return _A


for _s in MULT_SPECS:
    make_mult(_s)


def make_plain(cls, apply_fn=None, calls=None, is_fns=()):
    @dynamic_class
    class _A(Adj):
        NAME = cls
    _A.__name__ = cls
    return _A


# ---- three-axis adjusters (GimbalLimitControl, ReactionWheelTorque)
check_expr("AdjusterGimbalLimitControl", "ApplyControlAdjustment",
           "control.x *= pitchControlMultiplier / 100f; control.y *= rollControlMultiplier / 100f; control.z *= yawControlMultiplier / 100f; return control;")
check_expr("AdjusterReactionWheelTorque", "ApplyTorqueAdjustment",
           "torque.x *= pitchTorqueMultiplier / 100f; torque.y *= rollTorqueMultiplier / 100f; torque.z *= yawTorqueMultiplier / 100f; return torque;")


@dynamic_class
class AdjusterGimbalLimitControl(Adj):
    NAME = "AdjusterGimbalLimitControl"

    def apply(self, args):
        v = self.vals
        return {"out": [f32bits(f32(args[0] * (v["pitchControlMultiplier"] / 100.0))), f32bits(f32(args[1] * (v["rollControlMultiplier"] / 100.0))),
                        f32bits(f32(args[2] * (v["yawControlMultiplier"] / 100.0)))]}


@dynamic_class
class AdjusterReactionWheelTorque(Adj):
    NAME = "AdjusterReactionWheelTorque"

    def apply(self, args):
        v = self.vals
        return {"out": [f32bits(f32(args[0] * (v["pitchTorqueMultiplier"] / 100.0))), f32bits(f32(args[1] * (v["rollTorqueMultiplier"] / 100.0))),
                        f32bits(f32(args[2] * (v["yawTorqueMultiplier"] / 100.0)))]}


check_expr("AdjusterDataTransmitterPower", "ApplyPowerAdjustment", "return power * ((double)transmitterPowerMultiplier / 100.0);")


@dynamic_class
class AdjusterDataTransmitterPower(Adj):
    NAME = "AdjusterDataTransmitterPower"

    def apply(self, args):
        return {"out": dbits(args[0] * (self.vals["transmitterPowerMultiplier"] / 100.0))}

    def status_text(self):
        # UpdatePowerValueDisplay: Format(#autoLOC_8100221, Mathf.Round(transmitterPowerMultiplier / 100f))
        return fmt("#autoLOC_8100221", fnum(f32(round_even(f32(self.vals["transmitterPowerMultiplier"] / 100.0)))))

    def activate(self, scen):
        return {"status": self.status_text(), "calls": ["ModuleDataTransmitter.UpdatePowerText(0)"] if scen.get("module") == "ModuleDataTransmitter" else []}


@dynamic_class
class AdjusterSASServiceLevel(Adj):
    NAME = "AdjusterSASServiceLevel"

    def apply(self, args):
        return {"out": str(self.vals["sasServiceLevel"])}


@dynamic_class
class AdjusterKerbNetAccessDisabled(Adj):
    NAME = "AdjusterKerbNetAccessDisabled"

    def activate(self, scen):
        calls = []
        if scen.get("module") == "ModuleKerbNetAccess" and scen.get("queries", {}).get("HasThisDialogOpen"):
            calls.append("KerbNetDialog.Close(0)")
        return {"status": self.status_text(), "calls": calls}


# ---- failures: the part module calls of Activate / Deactivate (written from the C# sources, see the class comments there)
def call(cls, name, arg=0):
    return "%s.%s(%d)" % (cls, name, arg)


class Failure(Adj):
    MODULE = None

    def has_module(self, scen):
        return self.MODULE is not None and scen.get("module") == self.MODULE

    def activate(self, scen):
        return {"status": self.status_text(), "calls": self.activate_calls(scen)}

    def activate_calls(self, scen):
        return []

    def deactivate(self, scen):
        return {"calls": self.deactivate_calls(scen)}

    def deactivate_calls(self, scen):
        return []


def stuck_toggle(cls, fn, state_enum_on, state_enum_off):
    return None


@dynamic_class
class FailureActiveRadiatorStuck(Failure):
    NAME = "FailureActiveRadiatorStuck"
    MODULE = "ModuleActiveRadiator"

    def activate_calls(self, scen):
        if not self.has_module(scen):
            return []
        if self.vals["stuckState"] == "CurrentState":
            self.vals["stuckState"] = "On" if scen.get("queries", {}).get("IsCooling") else "Off"
        if self.vals["stuckState"] == "On":
            return [call(self.MODULE, "ApplyRadiatorActivation", 0)]
        if self.vals["stuckState"] == "Off":
            return [call(self.MODULE, "ApplyRadiatorActivation", 1)]
        return []


@dynamic_class
class FailureAnchoredDecouplerCantDecouple(Failure):
    NAME = "FailureAnchoredDecouplerCantDecouple"


@dynamic_class
class FailureCargoBayStuck(Failure):
    NAME = "FailureCargoBayStuck"
    MODULE = "ModuleCargoBay"

    def status_text(self):
        return None

    def activate(self, scen):
        if not self.has_module(scen):
            return {"status": None, "calls": []}
        idx = ENUMS["Open"].index(self.vals["stuckState"])
        return {"status": fmt("#autoLOC_8100324", scen.get("text", {}).get("partTypeName", "")),
                "calls": [call(self.MODULE, "ForceCargoBayStuck", idx)]}

    def deactivate_calls(self, scen):
        return [call(self.MODULE, "RemoveAdjusterForcingCargoBayStuck")] if self.has_module(scen) else []


@dynamic_class
class FailureControlSurfaceDisableControl(Failure):
    NAME = "FailureControlSurfaceDisableControl"

    def apply(self, args):
        return {"out": f32bits(0.0)}


@dynamic_class
class FailureCrossfeedStuck(Failure):
    NAME = "FailureCrossfeedStuck"
    MODULE = "ModuleToggleCrossfeed"

    def activate_calls(self, scen):
        if not self.has_module(scen):
            return []
        s = self.vals["stuckState"]
        return [call(self.MODULE, "ToggleAction", 0)] if s == "On" else ([call(self.MODULE, "ToggleAction", 1)] if s == "Off" else [])


@dynamic_class
class FailureDataTransmitterBroken(Failure):
    NAME = "FailureDataTransmitterBroken"
    MODULE = "ModuleDataTransmitter"

    def activate_calls(self, scen):
        if self.has_module(scen) and scen.get("queries", {}).get("IsBusy"):
            return [call(self.MODULE, "StopTransmission")]
        return []


@dynamic_class
class FailureDecoupleCantDecouple(Failure):
    NAME = "FailureDecoupleCantDecouple"


@dynamic_class
class FailureDeployablePartStuck(Failure):
    NAME = "FailureDeployablePartStuck"
    MODULE = "ModuleDeployablePart"

    def activate_calls(self, scen):
        if not self.has_module(scen):
            return []
        s = self.vals["stuckState"]
        return [call(self.MODULE, "ControlPanelsWithoutUsingSymmetry", 0)] if s == "Deployed" else (
            [call(self.MODULE, "ControlPanelsWithoutUsingSymmetry", 1)] if s == "Retracted" else [])


@dynamic_class
class FailureDockingNodeCantUndock(Failure):
    NAME = "FailureDockingNodeCantUndock"


@dynamic_class
class FailureEnginesDeadEngine(Failure):
    NAME = "FailureEnginesDeadEngine"
    MODULE = "ModuleEngines"

    def activate_calls(self, scen):
        if not self.has_module(scen):
            return []
        allow = 1 if scen.get("queries", {}).get("allowShutdown") else 0
        return [call(self.MODULE, "set_allowShutdown", 1), call(self.MODULE, "Shutdown"), call(self.MODULE, "set_allowShutdown", allow)]


@dynamic_class
class FailureFuelLineBlocked(Failure):
    NAME = "FailureFuelLineBlocked"
    MODULE = "CModuleFuelLine"

    def activate_calls(self, scen):
        return [call(self.MODULE, "CloseFuelLine")] if self.has_module(scen) else []

    def deactivate_calls(self, scen):
        return [call(self.MODULE, "OnTargetSet")] if self.has_module(scen) else []


@dynamic_class
class FailureGPSNoDisplay(Failure):
    NAME = "FailureGPSNoDisplay"


@dynamic_class
class FailureGrappleNodeCantGrapple(Failure):
    NAME = "FailureGrappleNodeCantGrapple"


@dynamic_class
class FailureGrappleNodeCantRelease(Failure):
    NAME = "FailureGrappleNodeCantRelease"
    MODULE = "ModuleGrappleNode"

    def activate_calls(self, scen):
        return [call(self.MODULE, "SetUpAdjusterBlockingGrappleNodeAbilityToRelease")] if self.has_module(scen) else []

    def deactivate_calls(self, scen):
        return [call(self.MODULE, "RemoveAdjusterBlockingGrappleNodeAbilityToRelease")] if self.has_module(scen) else []


@dynamic_class
class FailureLightFlickering(Failure):
    NAME = "FailureLightFlickering"
    MODULE = "ModuleLight"

    def awake(self):
        Failure.awake(self)
        self.currentIncrement = f32(-0.1)
        self.gradient = f32(-0.1)

    def update(self, rnd, dt):
        # FailureLightFlickering.UpdateFlickeringData (:...), Mono rule
        if self.currentIncrement < 0.0 or self.currentIncrement > 1.0:
            self.currentIncrement = f32(min(max(self.currentIncrement, 0.0), 1.0))
            sign = 1.0 if self.gradient >= 0.0 else -1.0
            self.gradient = f32(-1.0 * sign * (1.0 / rnd))
        self.currentIncrement = f32(self.currentIncrement + self.gradient * dt)

    def activate_calls(self, scen):
        if not self.has_module(scen):
            return []
        self.update(scen["rnd"], scen["dt"])
        return [call(self.MODULE, "ToggleLightAction", 0)]

    def apply(self, args):
        # args = [intensity, rnd, dt]
        self.update(args[1], args[2])
        return {"out": f32bits(f32(args[0] * self.currentIncrement)), "increment": f32bits(self.currentIncrement), "gradient": f32bits(self.gradient)}


@dynamic_class
class FailureLightStuck(Failure):
    NAME = "FailureLightStuck"
    MODULE = "ModuleLight"

    def activate_calls(self, scen):
        if not self.has_module(scen):
            return []
        s = self.vals["stuckState"]
        return [call(self.MODULE, "ToggleLightAction", 0)] if s == "On" else ([call(self.MODULE, "ToggleLightAction", 1)] if s == "Off" else [])


@dynamic_class
class FailureProbeControlPointBroken(Failure):
    NAME = "FailureProbeControlPointBroken"


@dynamic_class
class FailureRCSFireRandomly(Failure):
    NAME = "FailureRCSFireRandomly"

    def apply(self, args):
        # ApplyInputLinearAdjustment: new Vector3(Random.value x3).normalized (Vector3.normalized: magnitude > 1E-05 ? v / magnitude : zero)
        import math
        x, y, z = args
        mag = f32(math.sqrt(x * x + y * y + z * z))
        if mag > 1e-05:
            return {"out": [f32bits(f32(x / mag)), f32bits(f32(y / mag)), f32bits(f32(z / mag))]}
        return {"out": [f32bits(0.0)] * 3}


@dynamic_class
class FailureRCSStopWorking(Failure):
    NAME = "FailureRCSStopWorking"


@dynamic_class
class FailureReactionWheelBroken(Failure):
    NAME = "FailureReactionWheelBroken"


@dynamic_class
class FailureReactionWheelStuck(Failure):
    NAME = "FailureReactionWheelStuck"
    MODULE = "ModuleReactionWheel"

    def awake(self):
        Failure.awake(self)
        self.stuckTorque = (0.0, 0.0, 0.0)
        self.loadedFromConfigNode = False

    def activate_calls(self, scen):
        if not self.loadedFromConfigNode and self.has_module(scen):
            self.stuckTorque = tuple(f32(c) for c in scen.get("torque", [0.0, 0.0, 0.0]))
        return []

    def apply(self, args):
        return {"out": [f32bits(c) for c in self.stuckTorque]}


@dynamic_class
class FailureResourceIntakeStuck(Failure):
    NAME = "FailureResourceIntakeStuck"
    MODULE = "ModuleResourceIntake"

    def activate_calls(self, scen):
        if not self.has_module(scen):
            return []
        s = self.vals["stuckState"]
        return [call(self.MODULE, "ToggleAction", 0)] if s == "On" else ([call(self.MODULE, "ToggleAction", 1)] if s == "Off" else [])


@dynamic_class
class FailureScienceExperimentDisable(Failure):
    NAME = "FailureScienceExperimentDisable"


@dynamic_class
class FailureWheelDeploymentStuck(Failure):
    NAME = "FailureWheelDeploymentStuck"
    MODULE = "ModuleWheelDeployment"

    def activate_calls(self, scen):
        if not self.has_module(scen):
            return []
        s = self.vals["stuckState"]
        return [call(self.MODULE, "ActionToggle", 0)] if s == "Deployed" else ([call(self.MODULE, "ActionToggle", 1)] if s == "Retracted" else [])
