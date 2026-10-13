#!/usr/bin/env python3
"""WP-52: independent reference for the Breaking Ground robotic servos and the robotics controller -- test data generator.

Written from the decompiled C# ONLY (Assembly-CSharp/Expansions/Serenity/{BaseServo, ModuleRoboticServoHinge, ModuleRoboticServoPiston,
ModuleRoboticServoRotor, ModuleRoboticRotationServo, ModuleRoboticController, ControlledAxis, ControlledAction, RoboticControllerManager}.cs and
the managed parts of Mathf / Vector3 / Quaternion / Matrix4x4 of Unity 2019.4), before the C++ port was written, and it shares no code with it.
Circularity is avoided as in the other WP references: another language with its own object model (plain Python objects, a dict of KSPFields, a
list of events), seeded inputs generated here and only replayed by the C++ side, and a separate hand-written state machine per class.

Float32 follows the Mono rule (core/include/ksp/dotnet/mono_float.hpp, CAPTURE-SPEC C7): a float expression is evaluated in double and rounded
(st()) only where the C# stores it in a float local / field, passes it as a float argument, returns it or casts it.

What is a seam, not logic (scripted per tick and recorded in the output so the replay is identical):
  * the Transform angle / position the servo reads (`currentTransformAngle()` is Transform.localEulerAngles of the moving object, a native
    Quaternion.eulerAngles: the C++ side feeds the scripted values through BaseServo::angleSource / positionSource);
  * the PhysX joint (the module only WRITES ConfigurableJoint properties: recorded as data), the resource handler (electric charge is always
    available in the gated runs: the power-loss branches are covered by labelled derived cases in the C++ tests);
  * the curve evaluation of ControlledAxis presets and axis updates (UnityEngine.AnimationCurve is native): curve KEYS are gated bit for bit,
    interior curve VALUES are compared against a double-precision Hermite evaluation at a relative tolerance.

Usage: python -I core/tools/wp52/servo_reference.py > core/tests/data/wp52_reference.json
"""
import json
import math
import random
import sys

import numpy as np

np.seterr(all="ignore")
F = np.float32
INF = float("inf")
NAN = float("nan")
FLT_MAX = float(F(3.4028234663852886e38))


def st(x):
    """Mono float store: round a double expression to float32."""
    return float(F(x))


def f32c(x):
    """a C# `f`-suffixed literal widened to double"""
    return float(F(x))


def fdiv(a, b):
    if b == 0.0:
        if a == 0.0 or a != a:
            return NAN
        neg = (math.copysign(1.0, a) < 0) != (math.copysign(1.0, b) < 0)
        return -INF if neg else INF
    return a / b


PI32 = st(math.pi)
DEG2RAD32 = st(PI32 / 180.0)
TWOPI32 = st(PI32 * 2.0)
RPM2RAD32 = st(TWOPI32 / 60.0)


# ----------------------------------------------------------------------------------------------------------------------- Mathf
def m_sign(f):
    return 1.0 if f >= 0.0 else -1.0


def m_clamp(v, lo, hi):
    if v < lo:
        v = lo
    elif v > hi:
        v = hi
    return v


def m_clamp01(v):
    if v < 0.0:
        return 0.0
    if v > 1.0:
        return 1.0
    return v


def m_min(a, b):
    return a if a < b else b


def m_max(a, b):
    return a if a > b else b


def m_repeat(t, length):
    return m_clamp(st(t - m_floor(st(fdiv(t, length))) * length), 0.0, length)


def m_floor(f):
    return st(math.floor(f)) if math.isfinite(f) else f


def m_delta_angle(current, target):
    num = m_repeat(st(target - current), 360.0)
    if num > 180.0:
        num = st(num - 360.0)
    return num


def m_move_towards(current, target, max_delta):
    if abs(st(target - current)) <= max_delta:
        return target
    return st(current + m_sign(st(target - current)) * max_delta)


def m_move_towards_angle(current, target, max_delta):
    num = m_delta_angle(current, target)
    if 0.0 - max_delta < num and num < max_delta:
        return target
    target = st(current + num)
    return m_move_towards(current, target, max_delta)


def float_equals(a, b):
    return a == b or (a != a and b != b)


# ----------------------------------------------------------------------------------------------------------------------- Vector3 / Matrix4x4
def v3_scale(v, s):
    return [st(v[0] * s), st(v[1] * s), st(v[2] * s)]


def v3_add(a, b):
    return [st(a[0] + b[0]), st(a[1] + b[1]), st(a[2] + b[2])]


def v3_sub(a, b):
    return [st(a[0] - b[0]), st(a[1] - b[1]), st(a[2] - b[2])]


def v3_dot(a, b):
    return st(a[0] * b[0] + a[1] * b[1] + a[2] * b[2])


def v3_div(a, d):
    return [st(a[0] / d), st(a[1] / d), st(a[2] / d)]


def quat_mul_vec(q, p):
    """Quaternion * Vector3 (Quaternion.cs operator*)"""
    x, y, z, w = q
    num = st(x * 2.0)
    num2 = st(y * 2.0)
    num3 = st(z * 2.0)
    num4 = st(x * num)
    num5 = st(y * num2)
    num6 = st(z * num3)
    num7 = st(x * num2)
    num8 = st(x * num3)
    num9 = st(y * num3)
    num10 = st(w * num)
    num11 = st(w * num2)
    num12 = st(w * num3)
    return [st((1.0 - (num5 + num6)) * p[0] + (num7 - num12) * p[1] + (num8 + num11) * p[2]),
            st((num7 + num12) * p[0] + (1.0 - (num4 + num6)) * p[1] + (num9 - num10) * p[2]),
            st((num8 - num11) * p[0] + (num9 + num10) * p[1] + (1.0 - (num4 + num5)) * p[2])]


def mat_mul_dir(m, v):
    """Matrix4x4 * Vector4(v, 0) -> xyz; m is a row-major 4x4 list of lists"""
    return [st(m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2] + m[0][3] * 0.0),
            st(m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2] + m[1][3] * 0.0),
            st(m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2] + m[2][3] * 0.0)]


MAIN_AXES = {"Z": [0.0, 0.0, 1.0], "Z-": [0.0, 0.0, -1.0], "Y": [0.0, 1.0, 0.0], "Y-": [0.0, -1.0, 0.0], "X": [1.0, 0.0, 0.0], "X-": [-1.0, 0.0, 0.0]}


def main_axis(name):
    return MAIN_AXES.get(name, MAIN_AXES["Z"]) if name in MAIN_AXES else MAIN_AXES["Z"]


# ----------------------------------------------------------------------------------------------------------------------- BaseServo
class Joint:
    """the ConfigurableJoint properties the servos write (recorded, not simulated)"""

    def __init__(self):
        self.angularXDrive = [0.0, 0.0, FLT_MAX]   # positionSpring, positionDamper, maximumForce
        self.xDrive = [0.0, 0.0, FLT_MAX]
        self.slerpDrive = [0.0, 0.0, FLT_MAX]
        self.angularXLimitSpring = [0.0, 0.0]
        self.linearLimit = [0.0, 0.0, 0.0]         # limit, bounciness, contactDistance
        self.linearLimitSpring = [0.0, 0.0]
        self.high = 0.0
        self.low = 0.0
        self.targetPosition = [0.0, 0.0, 0.0]
        self.targetAngularVelocity = [0.0, 0.0, 0.0]
        self.connectedAnchor = [0.0, 0.0, 0.0]
        self.rotationDriveMode = "XYAndZ"
        self.angularXMotion = "Locked"
        self.set_target_rotation_calls = 0


DEFAULTS = {
    "servoIsLocked": False, "servoIsMotorized": True, "servoMotorIsEngaged": True, "servoMotorSize": 100.0, "servoMotorLimit": 100.0,
    "maxMotorOutput": 100.0, "lockPartOnPowerLoss": True, "mainAxis": "Z", "hardMinMaxLimits": [0.0, 180.0],
    "traverseVelocityLimits": [0.1, 10.0], "efficiency": 1.0, "connectedMassScale": 1.0, "baseResourceConsumptionRate": 0.001,
    "referenceConsumptionVelocity": 90.0, "resourceConsumptionMode": "CurrentVelocity", "launchPosition": 0.0,
    "motorizedMassPerKN": 0.1, "motorizedCostPerDriveUnit": 1.0,
}


class Base:
    def __init__(self, spec):
        self.f = dict(DEFAULTS)
        self.f.update(self.CLASS_DEFAULTS)
        for k, v in spec["fields"].items():
            self.f[k] = list(v) if isinstance(v, list) else v
        # every C# float field holds a float32 (the defaults above are written as decimals)
        for k, v in list(self.f.items()):
            if isinstance(v, float):
                self.f[k] = st(v)
            elif isinstance(v, list):
                self.f[k] = [st(x) if isinstance(x, float) else x for x in v]
        self.fdt = spec["fdt"]
        self.dt = spec["dt"]
        self.paw = spec["paw"]
        # ModuleResourceHandler.HasEnoughResourcesToAutoStart (cs:305): false unless the input resource has a start-up handler (startUpAmount 0.5 and
        # the charge is 1e6); the synthetic RESOURCE has none by default
        self.autostart = spec["autostart"]
        self.cur = spec["angles"][0]          # the transform coordinate the servo reads (angle or position)
        self.joint = Joint()
        self.joint_exists = True
        self.hasEnoughResources = False
        self.servoIsBraking = False
        self.motorManualDisengaged = False
        self.servoInitComplete = False
        self.initComplete = False
        self.partIsPhysicsLess = False
        self.rate = 0.0
        self.prevMotorEng = False
        self.prevMotorized = False
        self.prevLocked = False
        self.partActionMenuOpen = self.paw
        self.wasMoving = False
        self.lockAngle = 0.0
        self.motorOutput = 0.0
        self.driveUnit = 0.0
        self.currentVelocityLimit = 0.0
        self.previousDisplacement = 0.0
        self.currentFrameVelocity = 0.0
        self.transformRateOfMotion = 0.0
        self.readings = [0.0] * 20
        self.ridx = 0
        self.rsum = 0.0
        self.events = []
        self.handlers = {}

    # ---- field plumbing: Fields[name].SetValue(v) then OnValueModified
    def subscribe(self, name, fn):
        self.handlers.setdefault(name, []).append(fn)

    def set_field(self, name, value):
        self.f[name] = list(value) if isinstance(value, list) else value
        for fn in self.handlers.get(name, []):
            fn(value)

    # ---- BaseServo.cs
    def set_drive_unit(self):
        self.driveUnit = st(self.f["servoMotorSize"] * f32c(0.01) * self.f["maxMotorOutput"])

    def calculate_power(self):
        if not self.servoInitComplete:
            return 0.0
        result = 0.0
        if self.f["resourceConsumptionMode"] == "VelocityLimit":
            num = fdiv(self.currentVelocityLimit, self.f["referenceConsumptionVelocity"])
        else:
            num = fdiv(self.transformRateOfMotion, self.f["referenceConsumptionVelocity"])
        if self.f["servoMotorIsEngaged"] and self.is_moving():
            result = (self.f["baseResourceConsumptionRate"] * self.motorOutput) * num
        return result

    def calc_avg_rate(self):
        self.currentFrameVelocity = st(fdiv(self.get_frame_displacement(), self.fdt))
        self.rsum = st(self.rsum - self.readings[self.ridx])
        self.readings[self.ridx] = self.currentFrameVelocity
        self.rsum = st(self.rsum + self.readings[self.ridx])
        self.ridx = (self.ridx + 1) % 20
        self.transformRateOfMotion = st(fdiv(self.rsum, 20.0))

    def calc_resource_drain(self):
        # flight scene; electric charge is always available in the gated runs (the resource handler is a seam)
        self.rate = fdiv(self.calculate_power(), self.f["efficiency"])
        self.hasEnoughResources = True
        if abs(self.rate) < 0.001:
            self.rate = 0.0
        # rateMultiplier(2) feed the resource handler only

    def modify_servo(self, _field=None):
        self.on_pre_modify()
        self.set_drive_unit()
        self.motorOutput = self.driveUnit
        self.motorOutput = st(self.motorOutput * (self.f["servoMotorLimit"] * f32c(0.01)))
        if self.joint_exists:
            self.on_modify()

    def engage_lock(self):
        if self.f["servoIsLocked"]:
            return True
        if not self.f["servoIsLocked"] and not self.is_moving():
            self.set_field("servoIsLocked", True)
            self.modify_locked(None)
            return True
        return False

    def disengage_lock(self):
        if self.f["servoIsLocked"]:
            self.set_field("servoIsLocked", False)
            self.modify_locked(None)

    def engage_motor(self):
        if not self.f["servoMotorIsEngaged"]:
            self.set_field("servoMotorIsEngaged", True)
            self.modify_servo(None)

    def disengage_motor(self):
        if self.f["servoMotorIsEngaged"]:
            self.set_field("servoMotorIsEngaged", False)
            self.modify_servo(None)

    def modify_locked(self, field):
        flag = False
        locked = self.f["servoIsLocked"]
        if locked and not self.prevLocked:
            self.events.append("lockChanging:%d" % int(locked))
            flag = True
            self.on_lock_applied()
        elif not locked and self.prevLocked:
            self.events.append("lockChanging:%d" % int(locked))
            flag = True
            self.on_lock_removed()
        self.events.append("cycleAllAutoStrut")
        self.modify_servo(field)
        if flag:
            self.events.append("lockChanged:%d" % int(locked))

    def manually_modify_engaged(self, field):
        if self.f["servoMotorIsEngaged"] and not self.prevMotorEng:
            self.on_motor_engaged()
            self.calc_resource_drain()
        elif not self.f["servoMotorIsEngaged"] and self.prevMotorEng:
            self.on_motor_disengaged()
        self.modify_servo(field)

    def base_on_start(self):
        """BaseServo.OnStart, flight scene, moving object and joint present"""
        self.prevMotorEng = not self.f["servoMotorIsEngaged"]
        self.prevMotorized = not self.f["servoIsMotorized"]
        self.motorOutput = st(self.f["servoMotorSize"] * f32c(0.01) * self.f["maxMotorOutput"])
        self.motorOutput = st(self.motorOutput * (self.f["servoMotorLimit"] * f32c(0.01)))
        # InitJoint -> OnJointInit (subclass, called by the subclass driver)
        self.on_joint_init()
        if not self.f["servoIsMotorized"]:
            self.f["servoMotorIsEngaged"] = False
            self.prevMotorEng = True
        self.original_engaged = True   # the cfg value of servoMotorIsEngaged (the module schema default) is what `originalValue` holds
        if not self.original_engaged:
            self.motorManualDisengaged = True
        self.update_paw()              # `if (HighLogic.LoadedSceneIsFlight) UpdatePAWUI(UI_Scene.Flight)` runs whatever the PAW state
        self.subscribe("servoMotorSize", self.modify_servo)
        self.subscribe("servoMotorLimit", self.modify_servo)
        self.subscribe("servoMotorIsEngaged", self.manually_modify_engaged)
        self.subscribe("servoIsLocked", self.modify_locked)
        self.calc_resource_drain()

    def start_before_attach(self):
        # OnStartBeforePartAttachJoint (flight): PostStartInitJoint, SetInitialDisplacement, servoInitComplete = true
        self.post_start_init_joint()
        self.set_initial_displacement()
        self.servoInitComplete = True

    def fixed_update(self):
        if not self.servoInitComplete or self.partIsPhysicsLess:
            return
        self.calc_avg_rate()
        if self.f["servoIsMotorized"]:
            if self.f["servoMotorIsEngaged"] and not self.servoIsBraking and not self.f["servoIsLocked"]:
                self.calc_resource_drain()
            elif not self.f["servoMotorIsEngaged"] and self.autostart and not self.motorManualDisengaged:   # HasEnoughResourcesToAutoStart()
                self.hasEnoughResources = True
                if self.f["lockPartOnPowerLoss"] and self.f["servoIsLocked"]:
                    self.disengage_lock()
                if self.servoIsBraking:
                    self.servoIsBraking = False
                self.engage_motor()
                self.motorManualDisengaged = False
        self.servo_fixed_update()

    def update(self):
        if self.partIsPhysicsLess:
            return
        if self.partActionMenuOpen:
            self.update_paw()
        else:
            self.prevLocked = self.f["servoIsLocked"]

    def base_update_paw(self):
        pass   # motorState text only

    def can_be_detached(self):
        return not self.is_moving()

    def get_module_mass(self):
        result = 0.0
        if self.f["servoIsMotorized"]:
            result = st(self.f["motorizedMassPerKN"] * self.driveUnit)
        return result

    def get_module_cost(self):
        if self.f["servoIsMotorized"]:
            self.set_drive_unit()
            return st(self.f["motorizedCostPerDriveUnit"] * self.driveUnit)
        return 0.0


# ----------------------------------------------------------------------------------------------------------------------- Hinge
class Hinge(Base):
    CLASS_DEFAULTS = {"softMinMaxAngles": [0.0, 177.0], "targetAngle": 0.0, "modelInitialAngle": 0.0, "mirrorRotation": False, "currentAngle": 0.0,
                      "traverseVelocity": 90.0, "hingeDamping": 100.0, "angularXLimitSpring": 0.0, "angularXLimitDamper": 0.0,
                      "driveDampingMutliplier": 20.0, "driveSpringMutliplier": 100.0, "previousTargetAngle": 0.0, "useMultipleDragCubes": False}
    displacementLimitIgnoreRange = f32c(0.025)
    displacementLimitIgnoreFrameDistance = f32c(0.025)

    def __init__(self, spec):
        super().__init__(spec)
        self.driveTargetAngle = 0.0
        self.maxAnglePerFrame = 0.0
        self.angleDiff = 0.0
        self.cachedStartingRotationOffset = 0.0
        self.working = 0.0
        self.lastDragCubeAngle = FLT_MAX
        self.drag_weights = None

    def current_angle(self):
        return self.cur

    def joint_target_angle(self):
        soft = self.f["softMinMaxAngles"]
        w = m_clamp(self.f["targetAngle"], soft[0], soft[1])
        w = st(0.0 - w) if self.f["mirrorRotation"] else w
        w = st(w - self.f["modelInitialAngle"])
        self.working = w
        return w

    def on_start(self):
        self.base_on_start()
        self.modify_limits(None)
        self.modify_traverse(None)
        self.subscribe("softMinMaxAngles", self.modify_limits)
        self.subscribe("traverseVelocity", self.modify_traverse)
        self.subscribe("targetAngle", self.modify_servo)
        self.subscribe("hingeDamping", self.modify_servo)
        self.lastDragCubeAngle = FLT_MAX

    def on_joint_init(self):
        soft = self.f["softMinMaxAngles"]
        j = self.joint
        j.angularXMotion = "Limited"
        j.angularXDrive = [0.0, f32c(0.1), FLT_MAX]
        j.high = st(self.cachedStartingRotationOffset + abs(st(soft[0] - self.f["modelInitialAngle"])))
        j.low = st(self.cachedStartingRotationOffset - (soft[1] - self.f["modelInitialAngle"]))

    def post_start_init_joint(self):
        # OnPostStartJointInit
        self.f["previousTargetAngle"] = st(self.f["modelInitialAngle"] - self.current_angle())
        self.driveTargetAngle = self.current_angle()
        self.on_lock_applied()
        self.modify_limits(None)
        self.initComplete = True

    def on_pre_modify(self):
        soft = self.f["softMinMaxAngles"]
        self.f["targetAngle"] = m_clamp(self.f["targetAngle"], soft[0], soft[1])

    def on_modify(self):
        j = self.joint
        if (not self.f["servoMotorIsEngaged"] and not self.f["lockPartOnPowerLoss"]) or not self.f["servoIsMotorized"]:
            j.set_target_rotation_calls += 1
        j.angularXLimitSpring = [self.f["angularXLimitSpring"], self.f["angularXLimitDamper"]]
        d = list(j.angularXDrive)
        if not self.f["servoMotorIsEngaged"] and not self.f["servoIsLocked"]:
            if not self.f["lockPartOnPowerLoss"]:
                d = [0.0, 0.0, 0.0]
        else:
            d[0] = st(self.motorOutput * self.f["driveSpringMutliplier"])
            d[1] = st(self.f["driveDampingMutliplier"] * self.f["maxMotorOutput"] * (self.f["servoMotorSize"] * f32c(0.01)) * (self.f["hingeDamping"] * f32c(0.01)))
            d[2] = self.motorOutput
        j.angularXDrive = d

    def modify_limits(self, field):
        soft = self.f["softMinMaxAngles"]
        self.f["targetAngle"] = m_clamp(self.f["targetAngle"], soft[0], soft[1])
        self.modify_servo(field)

    def modify_traverse(self, field):
        tvl = self.f["traverseVelocityLimits"]
        self.f["traverseVelocity"] = m_clamp(self.f["traverseVelocity"], tvl[0], tvl[1])
        self.currentVelocityLimit = self.f["traverseVelocity"]
        self.modify_servo(field)

    def servo_fixed_update(self):
        if self.f["servoMotorIsEngaged"]:
            if not self.f["servoIsLocked"]:
                self.maxAnglePerFrame = st(self.f["traverseVelocity"] * self.fdt)
                if not float_equals(self.driveTargetAngle, self.joint_target_angle()):
                    self.driveTargetAngle = m_move_towards(self.driveTargetAngle, self.joint_target_angle(), self.maxAnglePerFrame)
                self.joint.set_target_rotation_calls += 1
        else:
            if not self.hasEnoughResources and self.f["lockPartOnPowerLoss"] and not self.f["servoIsLocked"]:
                self.engage_lock()
            if self.f["servoIsLocked"]:
                self.driveTargetAngle = self.lockAngle
        self.previousDisplacement = self.current_angle()
        if self.f["useMultipleDragCubes"]:
            self.set_drag_cubes(self.current_angle())

    def is_moving(self):
        return self.transformRateOfMotion > f32c(0.049)

    def get_frame_displacement(self):
        self.angleDiff = abs(st(self.current_angle() - self.previousDisplacement))
        if self.angleDiff < self.displacementLimitIgnoreFrameDistance:
            soft = self.f["softMinMaxAngles"]
            lo = st(soft[0] + self.displacementLimitIgnoreRange)
            hi = st(soft[1] - self.displacementLimitIgnoreRange)
            if self.f["targetAngle"] < lo and self.f["currentAngle"] < lo:
                self.angleDiff = 0.0
            elif self.f["targetAngle"] > hi and self.f["currentAngle"] > hi:
                self.angleDiff = 0.0
        return self.angleDiff

    def set_initial_displacement(self):
        self.previousDisplacement = self.current_angle()

    def update_paw(self):
        self.f["currentAngle"] = st(self.current_angle() + self.f["modelInitialAngle"])
        if (self.prevMotorEng == self.f["servoMotorIsEngaged"]) and (True) and self.prevLocked == self.f["servoIsLocked"]:
            return
        self.prevMotorEng = self.f["servoMotorIsEngaged"]
        self.prevMotorized = self.f["servoIsMotorized"]
        self.prevLocked = self.f["servoIsLocked"]

    def on_lock_applied(self):
        self.lockAngle = st(self.current_angle() + self.f["modelInitialAngle"])

    def on_lock_removed(self):
        self.driveTargetAngle = self.current_angle()

    def on_motor_engaged(self):
        self.driveTargetAngle = self.current_angle()
        self.motorManualDisengaged = False

    def on_motor_disengaged(self):
        self.motorManualDisengaged = True

    def set_min(self):
        if not self.f["servoIsLocked"]:
            self.set_field("targetAngle", self.f["softMinMaxAngles"][0])
            self.modify_servo(None)

    def set_max(self):
        if not self.f["servoIsLocked"]:
            self.set_field("targetAngle", self.f["softMinMaxAngles"][1])
            self.modify_servo(None)

    def toggle(self):
        soft = self.f["softMinMaxAngles"]
        if (self.f["targetAngle"] - soft[0]) < (soft[1] - soft[0]) / 2.0:
            self.set_max()
        else:
            self.set_min()

    def set_drag_cubes(self, angle):
        if not float_equals(angle, self.lastDragCubeAngle):
            num = angle
            if angle < -90.0:
                num = st(-180.0 - angle)
            elif angle > 90.0:
                num = st(180.0 - angle)
            w = m_max(st((0.0 - num) * f32c(0.01)), 0.0)
            w2 = st(1.0 - m_max(st(abs(st(0.0 - num)) * f32c(0.01)), 0.0))
            w3 = m_max(st(num * f32c(0.01)), 0.0)
            self.drag_weights = [w, w2, w3]
            self.lastDragCubeAngle = angle

    def observe(self):
        j = self.joint
        return {
            "driveTargetAngle": self.driveTargetAngle, "maxAnglePerFrame": self.maxAnglePerFrame, "previousDisplacement": self.previousDisplacement,
            "currentFrameVelocity": self.currentFrameVelocity, "transformRateOfMotion": self.transformRateOfMotion, "rsum": self.rsum,
            "motorOutput": self.motorOutput, "driveUnit": self.driveUnit, "angleDiff": self.angleDiff, "working": self.working, "lockAngle": self.lockAngle,
            "targetAngle": self.f["targetAngle"], "traverseVelocity": self.f["traverseVelocity"], "currentAngle": self.f["currentAngle"],
            "previousTargetAngle": self.f["previousTargetAngle"], "softMin": self.f["softMinMaxAngles"][0], "softMax": self.f["softMinMaxAngles"][1],
            "drive": list(j.angularXDrive), "limitSpring": list(j.angularXLimitSpring), "high": j.high, "low": j.low,
            "locked": 1.0 if self.f["servoIsLocked"] else 0.0, "engaged": 1.0 if self.f["servoMotorIsEngaged"] else 0.0,
            "moving": 1.0 if self.is_moving() else 0.0,
            "mass": self.get_module_mass(), "cost": self.get_module_cost(), "dragWeights": self.drag_weights or [], "rate": self.rate,
        }


# ----------------------------------------------------------------------------------------------------------------------- Rotation servo
class RotationServo(Base):
    CLASS_DEFAULTS = {"softMinMaxAngles": [0.0, 177.0], "allowFullRotation": True, "targetAngle": 0.0, "currentAngle": 0.0, "inverted": False,
                      "mirrorRotation": False, "traverseVelocity": 90.0, "hingeDamping": 100.0, "angularXLimitSpring": 0.0,
                      "angularXLimitDamper": 0.0, "driveDampingMutliplier": 20.0, "driveSpringMutliplier": 100.0, "previousTargetAngle": 0.0}

    def __init__(self, spec):
        super().__init__(spec)
        self.driveTargetAngle = 0.0
        self.maxAnglePerFrame = 0.0
        self.angleDiff = 0.0
        self.cachedStartingRotationOffset = 0.0
        self.working = 0.0
        self.config_hard = None
        self.prev_soft = None

    def current_angle(self):
        return self.cur

    def joint_target_angle(self):
        soft = self.f["softMinMaxAngles"]
        w = m_clamp(self.f["targetAngle"], soft[0], soft[1])
        w = st(0.0 - w) if self.f["mirrorRotation"] else w
        if self.f["inverted"]:
            w = st(0.0 - w)
        self.working = w
        return w

    def on_start(self):
        self.base_on_start()
        self.config_hard = list(self.f["hardMinMaxLimits"])
        self.prev_soft = list(self.f["softMinMaxAngles"])
        self.modify_limits(None)
        self.modify_limits_mode(None)
        self.modify_traverse(None)
        self.subscribe("allowFullRotation", self.modify_limits_mode)
        self.subscribe("softMinMaxAngles", self.modify_limits)
        self.subscribe("traverseVelocity", self.modify_traverse)
        self.subscribe("targetAngle", self.modify_target_angle)
        self.subscribe("hingeDamping", self.modify_servo)
        self.subscribe("inverted", self.modify_inverted)

    def on_joint_init(self):
        j = self.joint
        j.angularXMotion = "Limited"
        j.angularXDrive = [0.0, f32c(0.1), FLT_MAX]
        self.set_limits()

    def set_limits(self):
        soft = self.f["softMinMaxAngles"]
        mirror = self.f["mirrorRotation"]
        j = self.joint
        if self.f["inverted"]:
            up = soft[0] if mirror else soft[1]
            lo = st(0.0 - soft[1]) if mirror else soft[0]
        else:
            up = st(self.cachedStartingRotationOffset + (abs(soft[1]) if mirror else abs(soft[0])))
            lo = st(self.cachedStartingRotationOffset - ((0.0 - soft[0]) if mirror else soft[1]))
        j.high = up
        j.low = lo

    def post_start_init_joint(self):
        self.f["previousTargetAngle"] = self.current_angle()
        self.driveTargetAngle = self.current_angle()
        self.on_lock_applied()
        self.modify_servo(None)
        self.initComplete = True

    def on_pre_modify(self):
        soft = self.f["softMinMaxAngles"]
        self.f["targetAngle"] = m_clamp(self.f["targetAngle"], soft[0], soft[1])

    def on_modify(self):
        j = self.joint
        if (not self.f["servoMotorIsEngaged"] and not self.f["lockPartOnPowerLoss"]) or not self.f["servoIsMotorized"]:
            j.set_target_rotation_calls += 1
        if not self.f["allowFullRotation"]:
            j.angularXLimitSpring = [self.f["angularXLimitSpring"], self.f["angularXLimitDamper"]]
        else:
            j.angularXLimitSpring = [f32c(1e-16), f32c(1e-16)]
        d = list(j.angularXDrive)
        if not self.f["servoMotorIsEngaged"] and not self.f["servoIsLocked"]:
            if not self.f["lockPartOnPowerLoss"]:
                d = [0.0, 1000.0, 1.0]
        else:
            d[0] = st(self.motorOutput * self.f["driveSpringMutliplier"])
            d[1] = st(self.f["driveDampingMutliplier"] * self.f["maxMotorOutput"] * (self.f["servoMotorSize"] * f32c(0.01)) * (self.f["hingeDamping"] * f32c(0.01)))
            d[2] = self.motorOutput
        j.angularXDrive = d

    def modify_target_angle(self, field):
        soft = self.f["softMinMaxAngles"]
        if not self.f["allowFullRotation"]:
            self.f["targetAngle"] = m_clamp(self.f["targetAngle"], soft[0], soft[1])
        elif self.f["targetAngle"] < f32c(-179.99):
            self.f["targetAngle"] = st(360.0 + self.f["targetAngle"])
        elif self.f["targetAngle"] > 180.0:
            self.f["targetAngle"] = st(self.f["targetAngle"] - 360.0)
        self.modify_servo(field)

    def set_hard(self, v):
        self.f["hardMinMaxLimits"] = list(v)
        self.modify_limits(None)

    def set_soft(self, v):
        hard = self.f["hardMinMaxLimits"]
        v = [m_max(v[0], hard[0]), m_min(v[1], hard[1])]
        self.f["softMinMaxAngles"] = list(v)
        self.modify_limits(None)

    def modify_limits_mode(self, field):
        if not self.f["allowFullRotation"]:
            self.set_hard(self.config_hard)
            self.set_soft(self.prev_soft)
        else:
            self.prev_soft = list(self.f["softMinMaxAngles"])
            self.set_hard([f32c(-179.99), 180.0])
            self.set_soft([f32c(-179.99), 180.0])
        self.modify_servo(field)

    def modify_limits(self, field):
        soft = self.f["softMinMaxAngles"]
        self.f["targetAngle"] = m_clamp(self.f["targetAngle"], soft[0], soft[1])
        self.set_limits()
        self.modify_servo(field)

    def modify_inverted(self, field):
        self.set_limits()

    def modify_traverse(self, field):
        tvl = self.f["traverseVelocityLimits"]
        self.f["traverseVelocity"] = m_clamp(self.f["traverseVelocity"], tvl[0], tvl[1])
        self.currentVelocityLimit = self.f["traverseVelocity"]
        self.modify_servo(field)

    def servo_fixed_update(self):
        if not self.f["servoIsLocked"] and self.f["servoMotorIsEngaged"]:
            self.maxAnglePerFrame = st(self.f["traverseVelocity"] * self.fdt)
            if not float_equals(self.driveTargetAngle, self.joint_target_angle()):
                if not self.f["allowFullRotation"]:
                    self.driveTargetAngle = m_move_towards(self.driveTargetAngle, self.joint_target_angle(), self.maxAnglePerFrame)
                else:
                    self.driveTargetAngle = m_move_towards_angle(self.driveTargetAngle, self.joint_target_angle(), self.maxAnglePerFrame)
            self.joint.set_target_rotation_calls += 1
        elif not self.hasEnoughResources and self.f["lockPartOnPowerLoss"] and not self.f["servoIsLocked"]:
            self.engage_lock()
        else:
            self.joint.set_target_rotation_calls += 1
        self.previousDisplacement = self.current_angle()

    def is_moving(self):
        return self.transformRateOfMotion > f32c(0.049)

    def get_frame_displacement(self):
        self.angleDiff = st(self.current_angle() - self.previousDisplacement)
        if self.angleDiff >= 180.0:
            self.angleDiff = st(self.angleDiff - 360.0)
        elif self.angleDiff < -180.0:
            self.angleDiff = st(self.angleDiff + 360.0)
        self.angleDiff = abs(self.angleDiff)
        return self.angleDiff

    def set_initial_displacement(self):
        self.previousDisplacement = self.current_angle()

    def update_paw(self):
        self.f["currentAngle"] = self.current_angle()
        if (self.prevMotorEng == self.f["servoMotorIsEngaged"]) and self.prevLocked == self.f["servoIsLocked"]:
            return
        self.prevMotorEng = self.f["servoMotorIsEngaged"]
        self.prevMotorized = self.f["servoIsMotorized"]
        self.prevLocked = self.f["servoIsLocked"]

    def on_lock_applied(self):
        self.lockAngle = self.current_angle()

    def on_lock_removed(self):
        self.driveTargetAngle = self.current_angle()

    def on_motor_engaged(self):
        self.driveTargetAngle = self.current_angle()
        self.motorManualDisengaged = False

    def on_motor_disengaged(self):
        self.motorManualDisengaged = True

    def set_min(self):
        if not self.f["servoIsLocked"]:
            self.set_field("targetAngle", self.f["softMinMaxAngles"][0])
            self.modify_servo(None)

    def set_max(self):
        if not self.f["servoIsLocked"]:
            self.set_field("targetAngle", self.f["softMinMaxAngles"][1])
            self.modify_servo(None)

    def toggle(self):
        soft = self.f["softMinMaxAngles"]
        if (self.f["targetAngle"] - soft[0]) < (soft[1] - soft[0]) / 2.0:
            self.set_max()
        else:
            self.set_min()

    def observe(self):
        j = self.joint
        return {
            "driveTargetAngle": self.driveTargetAngle, "maxAnglePerFrame": self.maxAnglePerFrame, "previousDisplacement": self.previousDisplacement,
            "currentFrameVelocity": self.currentFrameVelocity, "transformRateOfMotion": self.transformRateOfMotion, "rsum": self.rsum,
            "motorOutput": self.motorOutput, "driveUnit": self.driveUnit, "angleDiff": self.angleDiff, "working": self.working, "lockAngle": self.lockAngle,
            "targetAngle": self.f["targetAngle"], "traverseVelocity": self.f["traverseVelocity"], "currentAngle": self.f["currentAngle"],
            "previousTargetAngle": self.f["previousTargetAngle"], "softMin": self.f["softMinMaxAngles"][0], "softMax": self.f["softMinMaxAngles"][1],
            "hardMin": self.f["hardMinMaxLimits"][0], "hardMax": self.f["hardMinMaxLimits"][1],
            "drive": list(j.angularXDrive), "limitSpring": list(j.angularXLimitSpring), "high": j.high, "low": j.low,
            "locked": 1.0 if self.f["servoIsLocked"] else 0.0, "engaged": 1.0 if self.f["servoMotorIsEngaged"] else 0.0,
            "moving": 1.0 if self.is_moving() else 0.0, "rate": self.rate,
            "mass": self.get_module_mass(), "cost": self.get_module_cost(),
        }


# ----------------------------------------------------------------------------------------------------------------------- Piston
class Piston(Base):
    CLASS_DEFAULTS = {"softMinMaxExtension": [0.0, 100.0], "targetExtension": 0.0, "currentExtension": 0.0, "traverseVelocity": 1.0, "pistonDamping": 100.0,
                      "linearLimitBounce": f32c(0.2), "linearLimitSpringSpring": 100000.0, "linearLimitSpringDamper": 100000.0,
                      "linearLimitContactDistance": 1.0, "positionDampingMutliplier": f32c(0.1), "positionSpringMutliplier": 100.0}

    def __init__(self, spec):
        super().__init__(spec)
        self.driveTargetPosition = 0.0
        self.targetPosition = 0.0
        self.previousTargetPosition = 0.0
        self.positionDiff = 0.0
        self.initComplete = False
        self.axis = [0.0, 0.0, 1.0]
        self.servoParentTransform = spec["parent"]          # 4x4 row-major
        self.servoParentTransformInverse = spec["parentInv"]
        self.moving_rot = spec["movingRot"]                  # Transform.localRotation of the moving object (x, y, z, w)
        self.moving_pos = spec["movingPos"]
        self.drag = None

    def current_position(self):
        return self.cur

    def calc_target_position(self, start, offset, relative):
        a = self.f["mainAxis"]
        vec = [0.0, 0.0, 1.0] if a == "Z" else ([0.0, 1.0, 0.0] if a == "Y" else [1.0, 0.0, 0.0])
        if relative:
            return v3_add(start, v3_scale(vec, offset))
        return v3_scale(vec, offset)

    def on_start(self):
        # the soft-range control (UI_MinMaxRange) clamps the stored range
        hard = self.f["hardMinMaxLimits"]
        step = st((hard[1] - hard[0]) / 100.0)
        soft = self.f["softMinMaxExtension"]
        self.f["softMinMaxExtension"] = [m_clamp(soft[0], hard[0], st(hard[1] - step)), m_clamp(soft[1], st(hard[0] + step), hard[1])]
        soft = self.f["softMinMaxExtension"]
        self.f["targetExtension"] = m_min(m_max(soft[0], self.f["targetExtension"]), soft[1])
        self.targetPosition = self.f["targetExtension"]
        self.previousTargetPosition = self.targetPosition
        self.driveTargetPosition = self.targetPosition
        self.base_on_start()
        self.update_traverse(None)
        self.update_target(None)
        self.subscribe("softMinMaxExtension", self.modify_limits)
        self.subscribe("targetExtension", self.update_target)
        self.subscribe("pistonDamping", self.modify_servo)
        self.subscribe("traverseVelocity", self.update_traverse)

    def on_joint_init(self):
        self.joint.linearLimit = [0.0, 0.0, 0.0]
        self.joint.xMotion = "Limited"
        mr = self.moving_rot
        self.axis = quat_mul_vec([-mr[0], -mr[1], -mr[2], mr[3]], main_axis(self.f["mainAxis"]))   # Quaternion.Inverse(localRotation) * GetMainAxis()

    def post_start_init_joint(self):
        self.initComplete = True
        self.modify_limits(None)
        if self.f["servoIsLocked"]:
            self.f["currentExtension"] = self.targetPosition
            self.lockAngle = self.f["currentExtension"]
            self.set_joint_target_position()

    def on_pre_modify(self):
        pass

    def modify_limits(self, field):
        soft = self.f["softMinMaxExtension"]
        self.f["targetExtension"] = m_min(m_max(soft[0], self.f["targetExtension"]), soft[1])
        self.update_target(field)
        soft = self.f["softMinMaxExtension"]
        v = self.calc_target_position([0.0, 0.0, 0.0], soft[0], False)
        v2 = self.calc_target_position([0.0, 0.0, 0.0], soft[1], False)
        v3 = v3_div(v3_add(v, v2), 2.0)
        self.joint.connectedAnchor = mat_mul_dir(self.servoParentTransform, v3)
        self.joint.linearLimit[0] = st((soft[1] - soft[0]) * f32c(1.01) / 2.0)
        self.modify_servo(field)

    def update_target(self, field):
        soft = self.f["softMinMaxExtension"]
        self.f["targetExtension"] = m_min(m_max(soft[0], self.f["targetExtension"]), soft[1])
        self.targetPosition = self.f["targetExtension"]
        self.modify_servo(field)

    def update_traverse(self, field):
        self.currentVelocityLimit = self.f["traverseVelocity"]
        self.modify_servo(field)

    def on_modify(self):
        j = self.joint
        soft = self.f["softMinMaxExtension"]
        j.linearLimit[0] = st((soft[1] - soft[0]) * f32c(1.01) / 2.0)
        j.linearLimit[1] = self.f["linearLimitBounce"]
        j.linearLimit[2] = self.f["linearLimitContactDistance"]
        j.linearLimitSpring = [self.f["linearLimitSpringSpring"], self.f["linearLimitSpringDamper"]]
        d = list(j.xDrive)
        if not self.f["servoMotorIsEngaged"] and not self.f["servoIsLocked"]:
            if not self.f["lockPartOnPowerLoss"]:
                d = [0.0, 0.0, 0.0]
        else:
            d[0] = st(self.motorOutput * self.f["positionSpringMutliplier"])
            d[1] = st(self.f["positionDampingMutliplier"] * self.f["maxMotorOutput"] * (self.f["servoMotorSize"] * f32c(0.01)) * (self.f["pistonDamping"] * f32c(0.01)))
            d[2] = self.motorOutput
        j.xDrive = d

    def set_joint_target_position(self):
        soft = self.f["softMinMaxExtension"]
        vec = v3_scale([1.0, 0.0, 0.0], st((soft[1] + soft[0]) / 2.0 - self.driveTargetPosition))
        self.joint.targetPosition = vec

    def update_current_extension(self):
        j = self.joint
        vec = v3_add(quat_mul_vec(self.moving_rot, v3_scale(self.axis, j.targetPosition[0])), self.moving_pos)
        vec2 = mat_mul_dir(self.servoParentTransformInverse, j.connectedAnchor)
        self.f["currentExtension"] = st(v3_dot(v3_sub(vec, vec2), main_axis(self.f["mainAxis"])) + self.driveTargetPosition)

    def servo_fixed_update(self):
        if not self.initComplete:
            return
        if self.f["servoMotorIsEngaged"] and not self.f["servoIsLocked"]:
            num = st(self.f["traverseVelocity"] * self.fdt)
            if self.driveTargetPosition < self.targetPosition:
                self.driveTargetPosition = m_min(st(self.driveTargetPosition + num), self.targetPosition)
            elif self.driveTargetPosition > self.targetPosition:
                self.driveTargetPosition = m_max(st(self.driveTargetPosition - num), self.targetPosition)
        else:
            if not self.hasEnoughResources and self.f["lockPartOnPowerLoss"] and not self.f["servoIsLocked"]:
                self.engage_lock()
            if self.f["servoIsLocked"]:
                self.driveTargetPosition = self.lockAngle
        self.previousDisplacement = self.current_position()
        self.set_joint_target_position()
        num2 = self.f["currentExtension"]
        self.update_current_extension()
        if num2 != self.f["currentExtension"]:
            self.set_drag_cubes(self.f["currentExtension"])

    def set_drag_cubes(self, extension):
        hard = self.f["hardMinMaxLimits"]
        x, y = hard[0], hard[1]
        num = st((x + y) / 2.0)
        num2 = m_clamp01(st((num - extension) / (num - x)))
        num3 = st(m_clamp01(st(extension - num)) / (y - num))
        weight = st((1.0 - num2) * (1.0 - num3))
        self.drag = [num2, weight, num3]

    def is_moving(self):
        return self.transformRateOfMotion > f32c(0.049)

    def get_frame_displacement(self):
        self.positionDiff = abs(st(self.current_position() - self.previousDisplacement))
        return self.positionDiff

    def set_initial_displacement(self):
        self.previousDisplacement = self.current_position()

    def update_paw(self):
        if self.initComplete:
            self.update_current_extension()
            self.prevLocked = self.f["servoIsLocked"]

    def on_lock_applied(self):
        self.update_current_extension()
        self.lockAngle = self.f["currentExtension"]
        self.driveTargetPosition = self.f["currentExtension"]

    def on_lock_removed(self):
        self.update_current_extension()
        self.f["currentExtension"] = self.lockAngle
        self.driveTargetPosition = self.f["currentExtension"]

    def on_motor_engaged(self):
        self.update_current_extension()
        self.driveTargetPosition = self.f["currentExtension"]
        self.motorManualDisengaged = False

    def on_motor_disengaged(self):
        self.motorManualDisengaged = True

    def extend(self):
        if not self.f["servoIsLocked"]:
            self.set_field("targetExtension", self.f["softMinMaxExtension"][1])
            self.update_target(None)

    def retract(self):
        if not self.f["servoIsLocked"]:
            self.set_field("targetExtension", self.f["softMinMaxExtension"][0])
            self.update_target(None)

    def observe(self):
        j = self.joint
        return {
            "driveTargetPosition": self.driveTargetPosition, "targetPosition": self.targetPosition, "previousDisplacement": self.previousDisplacement,
            "currentFrameVelocity": self.currentFrameVelocity, "transformRateOfMotion": self.transformRateOfMotion, "rsum": self.rsum,
            "motorOutput": self.motorOutput, "driveUnit": self.driveUnit, "positionDiff": self.positionDiff, "lockAngle": self.lockAngle,
            "targetExtension": self.f["targetExtension"], "traverseVelocity": self.f["traverseVelocity"], "currentExtension": self.f["currentExtension"],
            "softMin": self.f["softMinMaxExtension"][0], "softMax": self.f["softMinMaxExtension"][1],
            "drive": list(j.xDrive), "linearLimit": list(j.linearLimit), "linearLimitSpring": list(j.linearLimitSpring),
            "targetPositionVec": list(j.targetPosition), "connectedAnchor": list(j.connectedAnchor),
            "locked": 1.0 if self.f["servoIsLocked"] else 0.0, "engaged": 1.0 if self.f["servoMotorIsEngaged"] else 0.0,
            "moving": 1.0 if self.is_moving() else 0.0, "mass": self.get_module_mass(), "cost": self.get_module_cost(), "dragWeights": self.drag or [], "rate": self.rate,
        }


# ----------------------------------------------------------------------------------------------------------------------- Rotor
class Rotor(Base):
    CLASS_DEFAULTS = {"rpmLimit": 230.0, "currentRPM": 0.0, "rotateCounterClockwise": False, "inverted": False, "ratcheted": 0, "brakePercentage": 100.0,
                      "rotationMatch": 20.0, "rotorSpoolTime": 3.0, "brakeTorque": 1.0, "maxTorque": 400.0, "angularPositionDamper": 1.0,
                      "angularPositionSpring": f32c(1e-10), "angularDriveMode": "XYAndZ"}
    rpmRefreshSeconds = f32c(0.3)

    def __init__(self, spec):
        super().__init__(spec)
        self.brakingMode = False
        self.tempAngularVelocity = 0.0
        self.workingRPM = 0.0
        self.totalTorque = 0.0
        self.brakingTorque = 0.0
        self.rotationDiff = 0.0
        self.resourceStateChange = False
        self.timeSinceRPMRefresh = 0.0
        self.refreshRPMReadouts = False
        self.initComplete = False

    def current_angle(self):
        return self.cur

    def on_start(self):
        self.base_on_start()
        self.f["servoMotorLimit"] = 0.0
        self.modify_rpm(None)
        self.subscribe("rpmLimit", self.modify_rpm)
        self.subscribe("brakePercentage", self.modify_servo)
        self.subscribe("rotateCounterClockwise", self.modify_direction)
        self.subscribe("ratcheted", self.modify_servo)
        self.subscribe("inverted", self.modify_servo)

    def set_joint_drive(self, torque):
        j = self.joint
        if self.f["angularDriveMode"] == "slerp":
            j.rotationDriveMode = "Slerp"
            j.slerpDrive = [self.f["angularPositionSpring"], self.f["angularPositionDamper"], torque]
        else:
            j.rotationDriveMode = "XYAndZ"
            damper = self.f["angularPositionDamper"] if (self.f["servoMotorIsEngaged"] or self.brakingMode) else f32c(1e-11)
            j.angularXDrive = [self.f["angularPositionSpring"], damper, torque]

    def on_joint_init(self):
        self.joint.angularXMotion = "Limited"
        self.set_joint_drive(0.0)

    def post_start_init_joint(self):
        self.on_lock_applied()
        self.modify_servo(None)
        self.initComplete = True

    def modify_rpm(self, field):
        tvl = self.f["traverseVelocityLimits"]
        self.f["rpmLimit"] = m_clamp(self.f["rpmLimit"], tvl[0], tvl[1])
        self.currentVelocityLimit = self.f["rpmLimit"]
        self.modify_servo(field)

    def modify_direction(self, field):
        self.modify_servo(field)

    def on_pre_modify(self):
        pass

    def on_modify(self):
        j = self.joint
        self.f["maxTorque"] = st(self.f["maxMotorOutput"] * (self.f["servoMotorSize"] * f32c(0.01)))
        if not self.servoIsBraking:
            self.brakingMode = False
        self.brakingTorque = st(self.f["brakeTorque"] * self.f["brakePercentage"] * f32c(0.01)) if self.brakingMode else 0.0
        self.totalTorque = m_max(0.0, st(self.f["maxTorque"] * (self.f["servoMotorLimit"] * f32c(0.01))))
        self.workingRPM = 0.0 if self.brakingMode else m_max(0.0, st(self.f["rpmLimit"] * RPM2RAD32))
        ccw, inv = self.f["rotateCounterClockwise"], self.f["inverted"]
        if self.f["angularDriveMode"] == "slerp":
            if (not ccw and not inv) or (ccw and inv):
                self.workingRPM = st(self.workingRPM * -1.0)
        elif (ccw and not inv) or (not ccw and inv):
            self.workingRPM = st(self.workingRPM * -1.0)
        if self.f["servoMotorIsEngaged"]:
            if self.hasEnoughResources:
                j.angularXMotion = "Limited"
                self.set_joint_drive(self.brakingTorque if self.brakingMode else self.totalTorque)
            else:
                j.targetAngularVelocity = [0.0, 0.0, 0.0]
                self.set_joint_drive(1.0)
        else:
            j.targetAngularVelocity = [0.0, 0.0, 0.0]
            if self.brakingMode:
                self.set_joint_drive(self.brakingTorque)
            else:
                self.set_joint_drive(1.0)

    def servo_fixed_update(self):
        if not self.initComplete:
            return
        j = self.joint
        self.timeSinceRPMRefresh = st(self.timeSinceRPMRefresh + self.dt)
        if self.timeSinceRPMRefresh > self.rpmRefreshSeconds:
            self.timeSinceRPMRefresh = 0.0
            self.refreshRPMReadouts = True
        ccw, inv = self.f["rotateCounterClockwise"], self.f["inverted"]
        if self.f["servoMotorIsEngaged"] and not self.f["servoIsLocked"]:
            if self.f["servoMotorLimit"] > 0.0:
                self.resourceStateChange = False
                if abs(self.tempAngularVelocity) <= abs(self.workingRPM):
                    step = st(self.f["rotorSpoolTime"] * self.fdt)
                    if self.f["angularDriveMode"] == "slerp":
                        if (not ccw and not inv) or (ccw and inv):
                            self.tempAngularVelocity = st(self.tempAngularVelocity - step)
                        else:
                            self.tempAngularVelocity = st(self.tempAngularVelocity + step)
                    elif (ccw and not inv) or (not ccw and inv):
                        self.tempAngularVelocity = st(self.tempAngularVelocity - step)
                    else:
                        self.tempAngularVelocity = st(self.tempAngularVelocity + step)
                    j.targetAngularVelocity = v3_scale([1.0, 0.0, 0.0], self.tempAngularVelocity)
                else:
                    j.targetAngularVelocity = v3_scale([1.0, 0.0, 0.0], self.workingRPM)
            else:
                self.tempAngularVelocity = 0.0
                j.targetAngularVelocity = [0.0, 0.0, 0.0]
        else:
            self.tempAngularVelocity = 0.0
            j.targetAngularVelocity = [0.0, 0.0, 0.0]
            if self.f["lockPartOnPowerLoss"]:
                if not self.f["servoIsLocked"] and self.resourceStateChange:
                    self.brakingMode = True
                    self.servoIsBraking = True
                    self.engage_lock()
                self.set_joint_drive(self.f["brakeTorque"])
            if self.f["servoIsLocked"]:
                j.set_target_rotation_calls += 1
                self.set_joint_drive(1.0)
        self.previousDisplacement = self.current_angle()

    def is_moving(self):
        return self.transformRateOfMotion > 0.5

    def get_frame_displacement(self):
        self.rotationDiff = st(self.current_angle() - self.previousDisplacement)
        if self.rotationDiff >= 180.0:
            self.rotationDiff = st(self.rotationDiff - 360.0)
        elif self.rotationDiff < -180.0:
            self.rotationDiff = st(self.rotationDiff + 360.0)
        self.rotationDiff = st(abs(self.rotationDiff) * 60.0 / 360.0)
        return self.rotationDiff

    def set_initial_displacement(self):
        self.previousDisplacement = self.current_angle()

    def update_paw(self):
        num = (self.prevMotorEng != self.f["servoMotorIsEngaged"]) or self.prevLocked != self.f["servoIsLocked"]
        if self.refreshRPMReadouts:
            self.refreshRPMReadouts = False
            self.f["currentRPM"] = self.transformRateOfMotion
        if not num:
            return
        self.prevMotorEng = self.f["servoMotorIsEngaged"]
        self.prevMotorized = self.f["servoIsMotorized"]
        self.prevLocked = self.f["servoIsLocked"]

    def on_lock_applied(self):
        self.lockAngle = self.current_angle()

    def on_lock_removed(self):
        pass

    def on_motor_engaged(self):
        self.motorManualDisengaged = False

    def on_motor_disengaged(self):
        self.motorManualDisengaged = True

    def brake(self, kind):
        # kind: 0 Activate, 1 Deactivate, 2 Toggle
        if kind == 2:
            kind = 1 if self.brakingMode else 0
        if kind == 1:
            self.brakingMode = False
            self.servoIsBraking = False
            self.modify_servo(None)
        elif kind == 0:
            self.brakingMode = True
            self.servoIsBraking = True
            self.modify_servo(None)
        self.modify_servo(None)

    def observe(self):
        j = self.joint
        return {
            "tempAngularVelocity": self.tempAngularVelocity, "workingRPM": self.workingRPM, "totalTorque": self.totalTorque, "brakingTorque": self.brakingTorque,
            "previousDisplacement": self.previousDisplacement, "currentFrameVelocity": self.currentFrameVelocity,
            "transformRateOfMotion": self.transformRateOfMotion, "rsum": self.rsum, "motorOutput": self.motorOutput, "driveUnit": self.driveUnit,
            "rotationDiff": self.rotationDiff, "lockAngle": self.lockAngle, "maxTorque": self.f["maxTorque"], "rpmLimit": self.f["rpmLimit"],
            "currentRPM": self.f["currentRPM"], "servoMotorLimit": self.f["servoMotorLimit"],
            "drive": list(j.angularXDrive), "slerp": list(j.slerpDrive), "targetAngularVelocity": list(j.targetAngularVelocity),
            "locked": 1.0 if self.f["servoIsLocked"] else 0.0, "engaged": 1.0 if self.f["servoMotorIsEngaged"] else 0.0,
            "braking": 1.0 if self.brakingMode else 0.0, "moving": 1.0 if self.is_moving() else 0.0, "mass": self.get_module_mass(), "cost": self.get_module_cost(),
            "rate": self.rate,
        }


CLASSES = {"hinge": Hinge, "rotation": RotationServo, "piston": Piston, "rotor": Rotor}


# ----------------------------------------------------------------------------------------------------------------------- scenario generation
def r32(rng, lo, hi):
    return st(rng.uniform(lo, hi))


def make_ops(kind, rng, ticks, f):
    ops = []
    n = rng.randint(4, 9)
    for _ in range(n):
        t = rng.randint(1, ticks - 2)
        if kind in ("hinge", "rotation"):
            choice = rng.choice(["target", "target", "target", "soft", "traverse", "lock", "unlock", "motor_off", "motor_on", "limit", "size", "damping", "min", "max", "toggle"])
            if choice == "target":
                lo, hi = f["softMinMaxAngles"]
                ops.append([t, "set", "targetAngle", r32(rng, lo - 5.0, hi + 5.0)])
            elif choice == "soft":
                a = r32(rng, 0.0, 80.0)
                ops.append([t, "set_pair", "softMinMaxAngles", a, st(a + r32(rng, 5.0, 100.0))])
            elif choice == "traverse":
                ops.append([t, "set", "traverseVelocity", r32(rng, 0.0, 12.0)])
            elif choice == "limit":
                ops.append([t, "set", "servoMotorLimit", r32(rng, 0.0, 100.0)])
            elif choice == "size":
                ops.append([t, "set", "servoMotorSize", r32(rng, 0.0, 100.0)])
            elif choice == "damping":
                ops.append([t, "set", "hingeDamping", r32(rng, 0.0, 200.0)])
            else:
                ops.append([t, choice])
        elif kind == "piston":
            choice = rng.choice(["target", "target", "soft", "traverse", "lock", "unlock", "motor_off", "motor_on", "limit", "size", "damping", "extend", "retract"])
            if choice == "target":
                lo, hi = f["softMinMaxExtension"]
                ops.append([t, "set", "targetExtension", r32(rng, lo - 1.0, hi + 1.0)])
            elif choice == "soft":
                a = r32(rng, 0.0, 40.0)
                ops.append([t, "set_pair", "softMinMaxExtension", a, st(a + r32(rng, 2.0, 60.0))])
            elif choice == "traverse":
                ops.append([t, "set", "traverseVelocity", r32(rng, 0.05, 5.0)])
            elif choice == "limit":
                ops.append([t, "set", "servoMotorLimit", r32(rng, 0.0, 100.0)])
            elif choice == "size":
                ops.append([t, "set", "servoMotorSize", r32(rng, 0.0, 100.0)])
            elif choice == "damping":
                ops.append([t, "set", "pistonDamping", r32(rng, 0.0, 200.0)])
            else:
                ops.append([t, choice])
        else:  # rotor
            choice = rng.choice(["rpm", "rpm", "dir", "limit", "limit", "brake_on", "brake_off", "brake_toggle", "lock", "unlock", "motor_off", "motor_on", "size", "invert", "brakepct"])
            if choice == "rpm":
                ops.append([t, "set", "rpmLimit", r32(rng, 0.0, 12.0)])
            elif choice == "dir":
                ops.append([t, "set", "rotateCounterClockwise", bool(rng.getrandbits(1))])
            elif choice == "limit":
                ops.append([t, "set", "servoMotorLimit", r32(rng, 0.0, 100.0)])
            elif choice == "size":
                ops.append([t, "set", "servoMotorSize", r32(rng, 20.0, 100.0)])
            elif choice == "invert":
                ops.append([t, "set", "inverted", bool(rng.getrandbits(1))])
            elif choice == "brakepct":
                ops.append([t, "set", "brakePercentage", r32(rng, 0.0, 200.0)])
            else:
                ops.append([t, choice])
    ops.sort(key=lambda o: o[0])
    return ops


def gen_angles(kind, rng, ticks, f):
    """a scripted coordinate with moving and standing-still stretches (the servo only locks while it is not moving) and occasional wraps"""
    a = r32(rng, -20.0, 20.0) if kind != "piston" else r32(rng, 0.0, 5.0)
    out = []
    speed = r32(rng, 0.0, 2.0)
    still = rng.random() < 0.4
    left = rng.randint(10, 40)
    for t in range(ticks + 1):
        out.append(a)
        left -= 1
        if left <= 0:
            still = not still
            left = rng.randint(25, 45) if still else rng.randint(8, 35)
        if still:
            continue
        if kind == "rotor" and rng.random() < 0.02:
            a = st(a + r32(rng, 100.0, 200.0))
            if a > 180.0:
                a = st(a - 360.0)
        else:
            a = st(a + speed * r32(rng, -1.0, 1.0) + (r32(rng, 0.0, 0.3) if rng.random() < 0.5 else 0.0))
    return out


def apply_op(s, kind, op):
    name = op[1]
    if name == "set":
        s.set_field(op[2], op[3])
    elif name == "set_pair":
        s.set_field(op[2], [op[3], op[4]])
    elif name == "lock":
        s.engage_lock()
    elif name == "unlock":
        s.disengage_lock()
    elif name == "motor_off":
        s.disengage_motor()
    elif name == "motor_on":
        s.engage_motor()
    elif name == "min":
        s.set_min()
    elif name == "max":
        s.set_max()
    elif name == "toggle":
        s.toggle()
    elif name == "extend":
        s.extend()
    elif name == "retract":
        s.retract()
    elif name == "brake_on":
        s.brake(0)
    elif name == "brake_off":
        s.brake(1)
    elif name == "brake_toggle":
        s.brake(2)
    elif name == "invert":
        pass
    else:
        raise RuntimeError(name)


def run_scenario(spec):
    kind = spec["kind"]
    s = CLASSES[kind](spec)
    s.on_start()
    s.start_before_attach()
    if s.paw:
        s.update_paw()      # OnPartMenuOpen: partActionMenuOpen = true; UpdatePAWUI(Flight)
    out = []
    for t in range(spec["ticks"]):
        for op in spec["ops"]:
            if op[0] == t:
                apply_op(s, kind, op)
        s.cur = spec["angles"][t + 1]
        s.fixed_update()
        s.update()
        out.append(s.observe())
    return out, s.events


def gen_servo_scenarios():
    rng = random.Random(5201)
    scenarios = []
    ticks = 120
    plan = [("hinge", 26), ("rotation", 20), ("piston", 22), ("rotor", 24)]
    ident = [[1.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 1.0]]
    for kind, count in plan:
        for i in range(count):
            f = {}
            fdt = rng.choice([f32c(0.02), f32c(0.01), f32c(0.04)])
            if kind == "hinge":
                lo = r32(rng, 0.0, 30.0)
                f = {"softMinMaxAngles": [lo, st(lo + r32(rng, 40.0, 140.0))], "targetAngle": r32(rng, 0.0, 60.0), "modelInitialAngle": rng.choice([0.0, 0.0, r32(rng, -30.0, 30.0)]),
                     "mirrorRotation": bool(rng.getrandbits(1)), "traverseVelocity": r32(rng, 1.0, 180.0), "hingeDamping": r32(rng, 0.0, 200.0),
                     "servoMotorSize": r32(rng, 10.0, 100.0), "servoMotorLimit": r32(rng, 10.0, 100.0), "hardMinMaxLimits": [0.0, 180.0],
                     "traverseVelocityLimits": [rng.choice([0.1, 1.0]), rng.choice([10.0, 180.0, 240.0])], "angularXLimitSpring": rng.choice([0.0, 100.0]),
                     "angularXLimitDamper": rng.choice([0.0, 5.0]), "servoIsMotorized": rng.random() < 0.85, "lockPartOnPowerLoss": bool(rng.getrandbits(1)),
                     "useMultipleDragCubes": bool(rng.getrandbits(1)), "baseResourceConsumptionRate": rng.choice([0.001, 0.05, 1.0]),
                     "resourceConsumptionMode": rng.choice(["CurrentVelocity", "VelocityLimit"]), "efficiency": rng.choice([1.0, 0.5, 2.0]),
                     "mainAxis": rng.choice(["Z", "Y", "X"])}
            elif kind == "rotation":
                lo = r32(rng, 0.0, 30.0)
                f = {"softMinMaxAngles": [lo, st(lo + r32(rng, 40.0, 140.0))], "allowFullRotation": bool(rng.getrandbits(1)), "targetAngle": r32(rng, 0.0, 60.0),
                     "inverted": bool(rng.getrandbits(1)), "mirrorRotation": bool(rng.getrandbits(1)), "traverseVelocity": r32(rng, 1.0, 180.0),
                     "hingeDamping": r32(rng, 0.0, 200.0), "servoMotorSize": r32(rng, 10.0, 100.0), "servoMotorLimit": r32(rng, 10.0, 100.0),
                     "hardMinMaxLimits": [0.0, 180.0], "traverseVelocityLimits": [0.1, 240.0], "angularXLimitSpring": rng.choice([0.0, 100.0]),
                     "servoIsMotorized": rng.random() < 0.9, "lockPartOnPowerLoss": bool(rng.getrandbits(1))}
            elif kind == "piston":
                lo = r32(rng, 0.0, 10.0)
                f = {"softMinMaxExtension": [lo, st(lo + r32(rng, 5.0, 90.0))], "targetExtension": r32(rng, 0.0, 50.0), "traverseVelocity": r32(rng, 0.05, 5.0),
                     "pistonDamping": r32(rng, 0.0, 200.0), "servoMotorSize": r32(rng, 10.0, 100.0), "servoMotorLimit": r32(rng, 10.0, 100.0),
                     "hardMinMaxLimits": [0.0, rng.choice([100.0, 2.0, 50.0])], "lockPartOnPowerLoss": bool(rng.getrandbits(1)),
                     "linearLimitBounce": rng.choice([f32c(0.2), 0.0]), "mainAxis": rng.choice(["Z", "Y", "X"]),
                     "traverseVelocityLimits": [0.05, 5.0]}
            else:
                f = {"rpmLimit": r32(rng, 1.0, 10.0), "rotateCounterClockwise": bool(rng.getrandbits(1)), "inverted": bool(rng.getrandbits(1)),
                     "servoMotorSize": r32(rng, 20.0, 100.0), "servoMotorLimit": 100.0, "brakePercentage": r32(rng, 0.0, 200.0),
                     "angularDriveMode": rng.choice(["XYAndZ", "slerp"]), "rotorSpoolTime": r32(rng, 0.5, 6.0), "brakeTorque": r32(rng, 0.0, 3.0),
                     "traverseVelocityLimits": [0.1, rng.choice([10.0, 460.0])], "lockPartOnPowerLoss": bool(rng.getrandbits(1)),
                     "servoIsMotorized": rng.random() < 0.9, "angularPositionDamper": rng.choice([1.0, 0.5]), "servoMotorLimit": 100.0}
            angles = gen_angles(kind, rng, ticks, f)
            spec = {"kind": kind, "fields": f, "autostart": bool(rng.getrandbits(1)), "fdt": fdt, "dt": rng.choice([f32c(0.0166667), f32c(0.02), f32c(0.033)]), "paw": bool(rng.getrandbits(1)),
                    "ticks": ticks, "angles": angles}
            if kind == "piston":
                # 90 degree rotations and exact scales so the matrix maths is exactly representable
                par = rng.choice([ident, [[0.0, 1.0, 0.0, 0.0], [-1.0, 0.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 1.0]],
                                  [[2.0, 0.0, 0.0, 0.0], [0.0, 2.0, 0.0, 0.0], [0.0, 0.0, 2.0, 0.0], [0.0, 0.0, 0.0, 1.0]]])
                inv = [[par[0][0], 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 1.0]]
                # exact inverses: identity -> identity; 90 deg about Z: transpose; uniform scale 2: scale 0.5
                if par is ident:
                    inv = ident
                elif par[0][0] == 0.0:
                    inv = [[0.0, -1.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 1.0]]
                else:
                    inv = [[0.5, 0.0, 0.0, 0.0], [0.0, 0.5, 0.0, 0.0], [0.0, 0.0, 0.5, 0.0], [0.0, 0.0, 0.0, 1.0]]
                spec["parent"] = par
                spec["parentInv"] = inv
                spec["movingRot"] = rng.choice([[0.0, 0.0, 0.0, 1.0], [0.0, 0.0, f32c(0.7071068), f32c(0.7071068)], [f32c(0.5), f32c(0.5), f32c(0.5), f32c(0.5)]])
                spec["movingPos"] = [r32(rng, -1.0, 1.0), r32(rng, -1.0, 1.0), r32(rng, -1.0, 1.0)]
            spec["ops"] = make_ops(kind, rng, ticks, f)
            spec["expected"], spec["events"] = run_scenario(spec)
            scenarios.append(spec)
    return scenarios


# ----------------------------------------------------------------------------------------------------------------------- controller
class Ctl:
    """ModuleRoboticController sequencer (flight scene), manager queue and the stock action checks; written from ModuleRoboticController.cs"""

    def __init__(self, spec):
        self.length = spec["length"]
        self.position = spec["position"]
        self.speed = spec["speed"]
        self.playing = spec["playing"]          # sequencePlayingIndex == 0
        self.direction = spec["direction"]       # 0 forward 1 reverse
        self.loop = spec["loop"]                 # 0 Once 1 Repeat 2 PingPong 3 OnceRestart
        self.enabled = spec["enabled"]
        self.actions = spec["actions"]           # list of lists of float times
        self.hasEnough = True
        self.a1s = self.a1e = self.a2s = self.a2e = 0.0   # float fields: C# default 0
        self.fired = []
        self.events = []
        self.axes = spec["axes"]                 # per axis: {"maxTime": f, "keys": [...], "mult": m or None, "value": 0}
        self.dt = spec["dt"]
        self.axisvals = [None] * len(self.axes)

    def stop(self):
        was = self.playing
        self.playing = False
        if not was:
            self.events.append("stopped")

    def play(self):
        num = not self.playing
        if num and self.loop == 3 and self.position >= self.length:
            self.set_position(0.0)
        self.playing = True
        if not num:
            self.events.append("played")

    def toggle_direction(self):
        self.set_direction(0 if self.direction == 1 else 1)

    def set_direction(self, d):
        changed = self.direction != d
        self.direction = d
        if changed:
            self.events.append("dir:%d" % d)

    def set_loop(self, m):
        changed = self.loop != m
        self.loop = m
        if changed:
            self.events.append("loop:%d" % m)

    def cycle_loop(self):
        nxt = {0: 1, 1: 2, 2: 3}.get(self.loop, 0)
        self.set_loop(nxt)
        self.events.append("loop:%d" % self.loop)

    def set_length(self, new_len):
        if new_len < f32c(0.01):
            new_len = f32c(0.01)
        num = self.position
        if num > 0.0:
            num = st(num / self.length * new_len)
        self.length = new_len
        self.position = num

    def set_position(self, new_pos):
        self.a1s = self.a1e = self.a2s = self.a2e = -1.0
        L = self.length
        if not (new_pos > L) and new_pos >= 0.0:
            if self.direction == 0:
                self.a1s = self.position
                self.a1e = new_pos
            else:
                self.a1s = new_pos
                self.a1e = self.position
        else:
            if self.loop == 1:
                if new_pos < 0.0:
                    new_pos = st(L + new_pos)
                    self.a1s = 0.0
                    self.a1e = self.position
                    self.a2s = new_pos
                    self.a2e = st(L + 1.0)
                else:
                    new_pos = st(new_pos - L)
                    self.a1s = 0.0
                    self.a1e = new_pos
                    self.a2s = self.position
                    self.a2e = st(L + 1.0)
            elif self.loop == 2:
                if new_pos < 0.0:
                    new_pos = st(0.0 - new_pos)
                    self.a1s = 0.0
                    self.a1e = self.position
                    self.a2s = 0.0
                    self.a2e = new_pos
                else:
                    new_pos = st(L - (new_pos - L))
                    self.a1s = self.position
                    self.a1e = st(L + 1.0)
                    self.a2s = new_pos
                    self.a2e = st(L + 1.0)
                self.toggle_direction()
            else:
                if new_pos < 0.0:
                    new_pos = 0.0
                    self.a1s = 0.0
                    self.a1e = self.position
                else:
                    new_pos = L
                    self.a1s = self.position
                    self.a1e = new_pos
                self.stop()
        new_pos = m_clamp(new_pos, 0.0, L)
        self.position = new_pos
        if self.enabled:
            for i, ax in enumerate(self.axes):
                mult = ax["mult"]
                tv = st(new_pos / L * ax["maxTime"])
                self.axisvals[i] = [tv, mult if mult is not None else 1.0]

    def check_and_fire(self):
        to_fire = []
        L = self.length
        for i, times in enumerate(self.actions):
            for t in times:
                flag = False
                if t >= fdiv(self.a1s, L) and t < fdiv(self.a1e, L):
                    flag = True
                if t >= fdiv(self.a2s, L) and t < fdiv(self.a2e, L):
                    flag = True
                if self.loop in (0, 3) and t == fdiv(self.a1e, L):
                    flag = True
                if not flag:
                    continue
                to_fire.append(i)
        # fired in reverse list order
        for i in reversed(to_fire):
            self.fired.append(i)

    def update_playing(self):
        if self.hasEnough:
            ut = self.dt
            if self.direction == 1:
                ut = st(ut * -1.0)
            ut = st(ut * (self.speed * f32c(0.01)))
            self.set_position(st(self.position + ut))
            self.check_and_fire()


def hermite_eval(keys, t):
    """double-precision cubic Hermite through (time, value, inTangent, outTangent) keys, clamped outside the range"""
    if t <= keys[0][0]:
        return keys[0][1]
    if t >= keys[-1][0]:
        return keys[-1][1]
    for i in range(len(keys) - 1):
        t0, v0, _, o0 = keys[i]
        t1, v1, i1, _ = keys[i + 1]
        if t0 <= t <= t1:
            dt = t1 - t0
            u = (t - t0) / dt
            h00 = 2 * u ** 3 - 3 * u ** 2 + 1
            h10 = u ** 3 - 2 * u ** 2 + u
            h01 = -2 * u ** 3 + 3 * u ** 2
            h11 = u ** 3 - u ** 2
            return h00 * v0 + h10 * dt * o0 + h01 * v1 + h11 * dt * i1
    return keys[-1][1]


def gen_controller_scenarios():
    rng = random.Random(5202)
    scenarios = []
    for i in range(60):
        length = rng.choice([5.0, 10.0, 2.5, r32(rng, 1.0, 30.0)])
        n_act = rng.randint(0, 3)
        actions = [sorted(r32(rng, 0.0, 1.0) for _ in range(rng.randint(1, 4))) for _ in range(n_act)]
        # sprinkle exact boundary times (0, 1, and times that equal end-of-window values)
        if actions and rng.random() < 0.5:
            actions[0].append(rng.choice([0.0, 1.0, 0.5]))
        axes = []
        for _ in range(rng.randint(0, 3)):
            axes.append({"maxTime": rng.choice([1.0, 1.0, 2.0, r32(rng, 0.5, 4.0)]), "mult": rng.choice([None, None, 100.0, length])})
        spec = {"length": length, "position": rng.choice([0.0, r32(rng, 0.0, length)]), "speed": rng.choice([100.0, 100.0, r32(rng, 0.0, 100.0), 250.0]),
                "playing": rng.random() < 0.8, "direction": rng.getrandbits(1), "loop": rng.randint(0, 3), "enabled": rng.random() < 0.9,
                "actions": actions, "axes": axes, "dt": rng.choice([f32c(0.02), f32c(0.0166667), f32c(0.2), f32c(0.5), f32c(1.0)]), "ticks": 60}
        ops = []
        for _ in range(rng.randint(2, 6)):
            k = rng.choice(["toggle_dir", "cycle_loop", "play", "stop", "set_pos", "set_len", "set_speed", "start", "end", "forward", "reverse"])
            arg = 0.0
            if k == "set_pos":
                arg = r32(rng, -0.5 * length, 1.5 * length)
            elif k == "set_len":
                arg = r32(rng, 0.001, 2.0 * length)
            elif k == "set_speed":
                arg = r32(rng, -20.0, 140.0)
            ops.append([rng.randint(1, 55), k, arg])
        ops.sort(key=lambda o: o[0])
        spec["ops"] = ops
        c = Ctl(spec)
        out = []
        for t in range(spec["ticks"]):
            for op in spec["ops"]:
                if op[0] == t:
                    k = op[1]
                    if k == "toggle_dir":
                        c.toggle_direction()
                    elif k == "cycle_loop":
                        c.cycle_loop()
                    elif k == "play":
                        c.play()
                    elif k == "stop":
                        c.stop()
                    elif k == "set_pos":
                        c.set_position(op[2])
                    elif k == "set_len":
                        c.set_length(op[2])
                    elif k == "set_speed":
                        c.speed = m_clamp(op[2], 0.0, 100.0)
                    elif k == "start":
                        c.set_position(0.0)
                    elif k == "end":
                        c.set_position(c.length)
                    elif k == "forward":
                        c.set_direction(0)
                    elif k == "reverse":
                        c.set_direction(1)
            if c.playing:
                c.update_playing()
            out.append({"position": c.position, "length": c.length, "direction": float(c.direction), "loop": float(c.loop), "playing": 1.0 if c.playing else 0.0,
                        "a1s": c.a1s, "a1e": c.a1e, "a2s": c.a2s, "a2e": c.a2e, "fired": list(c.fired), "events": list(c.events),
                        "axisvals": [list(v) if v else None for v in c.axisvals], "speed": c.speed})
            c.fired = []
            c.events = []
        spec["expected"] = out
        scenarios.append(spec)
    return scenarios


# ----------------------------------------------------------------------------------------------------------------------- ControlledAxis presets
class Axis:
    def __init__(self, axis_min, axis_max):
        self.min = axis_min
        self.max = axis_max
        self.lastName = ""
        self.lastMin = 0.0
        self.lastMax = 0.0
        self.lastRef = None
        self.lastKeys = None
        self.lastCurveLen = 0
        self.lastCurve = None

    REFS = {
        "stockSine": [(0.0, 1.0, 0.0, 0.0), (0.5, 0.0, 0.0, 0.0), (1.0, 1.0, 0.0, 0.0)],
        "stockSquare": [(0.0, 0.0, 0.0, 0.0), (f32c(0.499), 0.0, 0.0, 1000.0), (0.5, 1.0, 1000.0, 0.0), (f32c(0.999), 1.0, 0.0, -1000.0), (1.0, 0.0, -1000.0, 0.0)],
        "stockTriangle": [(0.0, 0.0, -2.0, 2.0), (0.5, 1.0, 2.0, -2.0), (1.0, 0.0, -2.0, 2.0)],
        "stockSaw": [(0.0, 0.0, 0.0, 1.0), (f32c(0.999), 1.0, 1.0, -1000.0), (1.0, 0.0, -1000.0, 1.0)],
        "stockRevSaw": [(0.0, 1.0, 0.0, -1.0), (f32c(0.999), 0.0, -1.0, 1000.0), (1.0, 1.0, 1000.0, -1.0)],
    }

    def generate(self, ref_id, cycles, phase, soft=None, controller=False):
        num = self.max
        num2 = self.min
        if controller:
            num, num2 = 1.0, 0.0
        if soft is not None and not controller:      # only a module that implements IAxisFieldLimits (the servos; not the controller) overrides
            num2, num = soft
        ref = self.REFS.get(ref_id)
        if ref is None:
            ref = self.lastRef
        ceil = int(math.ceil(st(st(cycles + 4.0) / 10.0)))
        num3 = max(ceil, 2) * 10
        if self.lastName != ref_id or self.lastKeys is None or self.lastMax != num or self.lastMin != num2 or self.lastCurveLen < num3:
            self.lastMin, self.lastMax, self.lastName = num2, num, ref_id
            self.lastRef = ref
            keys = []
            k0 = ref[0]
            keys.append((k0[0], st(num2 + k0[1] * (num - num2)), st(k0[2] * (num - num2)), st(k0[3] * (num - num2))))
            for i in range(num3):
                for j in range(1, len(ref)):
                    kf = ref[j]
                    keys.append((st(kf[0] + float(i)), st(num2 + kf[1] * (num - num2)), st(kf[2] * (num - num2)), st(kf[3] * (num - num2))))
            self.lastKeys = keys
            self.lastCurveLen = len(keys)
        num4 = st(phase / f32c(100.0))
        lst = []
        for k, key in enumerate(self.lastKeys):
            t, v, tin, tout = key
            t = st(t - num4)
            if t <= cycles:
                if len(lst) < 1:
                    if t < 0.0:
                        continue
                    if t != num4:
                        e0 = hermite_eval(self.lastKeys, num4)
                        e1 = hermite_eval(self.lastKeys, st(num4 + f32c(0.001)))
                        lst.append(("synth", 0.0, e0, 0.0, (e1 - e0) / float(F(0.001)) * cycles))
                t = st(t / cycles)
                tin = st(tin * cycles)
                tout = st(tout * cycles)
                lst.append(("key", t, v, tin, tout))
                continue
            if st(self.lastKeys[k - 1][0] - num4) < cycles:
                e1 = hermite_eval(self.lastKeys, st(cycles + num4))
                e0 = hermite_eval(self.lastKeys, st(cycles + num4 - f32c(0.001)))
                lst.append(("synth", 1.0, e1, (e1 - e0) / float(F(0.001)) * cycles, 0.0))
            break
        # new FloatCurve(list.ToArray()) adds the keys one by one: AnimationCurve.AddKey rejects a key whose time already exists (the synthetic
        # start key and the first real key share time 0 when the phase is a whole number of reference keys)
        out = []
        for e in lst:
            if any(o[1] == e[1] for o in out):
                continue
            out.append(e)
        return out


def gen_preset_cases():
    rng = random.Random(5203)
    cases = []
    for i in range(80):
        ax_min = rng.choice([0.0, -90.0, 10.0, r32(rng, -50.0, 20.0)])
        ax_max = st(ax_min + rng.choice([100.0, 180.0, r32(rng, 5.0, 300.0)]))
        ref = rng.choice(list(Axis.REFS.keys()))
        cycles = rng.choice([1.0, 2.0, 3.0, 4.0, 5.0, 8.0, 12.0, r32(rng, 0.5, 20.0)])
        phase = rng.choice([0.0, 0.0, 25.0, 50.0, r32(rng, 0.0, 99.0)])
        soft = None
        if rng.random() < 0.3:
            a = st(ax_min + r32(rng, 0.0, 10.0))
            soft = [a, st(a + r32(rng, 5.0, 50.0))]
        controller = rng.random() < 0.15
        a = Axis(ax_min, ax_max)
        res = a.generate(ref, cycles, phase, soft, controller)
        # a second call with the cached reference (UpdatePreset) at different cycles / phase
        cycles2 = rng.choice([cycles, st(cycles + 1.0), 2.0])
        phase2 = rng.choice([phase, 10.0, 75.0])
        res2 = a.generate(ref, cycles2, phase2, soft, controller)
        cases.append({"min": ax_min, "max": ax_max, "ref": ref, "cycles": cycles, "phase": phase, "soft": soft, "controller": controller,
                      "cycles2": cycles2, "phase2": phase2, "out": res, "out2": res2})
    return cases


# ----------------------------------------------------------------------------------------------------------------------- joint seam maths
def qmul64(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by, aw * by + ay * bw + az * bx - ax * bz, aw * bz + az * bw + ax * by - ay * bx, aw * bw - ax * bx - ay * by - az * bz)


def qinv64(q):
    return (-q[0], -q[1], -q[2], q[3])


def qaxis64(angle_deg, axis):
    h = math.radians(angle_deg) / 2.0
    n = math.sqrt(sum(c * c for c in axis))
    s = math.sin(h) / n
    return (axis[0] * s, axis[1] * s, axis[2] * s, math.cos(h))


def cross64(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def norm64(v):
    n = math.sqrt(sum(c * c for c in v))
    return tuple(c / n for c in v)


def look_rotation64(fwd, up):
    z = norm64(fwd)
    x = norm64(cross64(up, z))
    y = cross64(z, x)
    m00, m01, m02 = x[0], y[0], z[0]
    m10, m11, m12 = x[1], y[1], z[1]
    m20, m21, m22 = x[2], y[2], z[2]
    tr = m00 + m11 + m22
    if tr > 0.0:
        s = math.sqrt(tr + 1.0)
        w = s * 0.5
        s = 0.5 / s
        return ((m21 - m12) * s, (m02 - m20) * s, (m10 - m01) * s, w)
    if m00 >= m11 and m00 >= m22:
        s = math.sqrt(m00 - m11 - m22 + 1.0)
        xq = 0.5 * s
        s = 0.5 / s
        return (xq, (m01 + m10) * s, (m02 + m20) * s, (m12 - m21) * s)
    if m11 > m22:
        s = math.sqrt(m11 - m00 - m22 + 1.0)
        yq = 0.5 * s
        s = 0.5 / s
        return ((m10 + m01) * s, yq, (m21 + m12) * s, (m20 - m02) * s)
    s = math.sqrt(m22 - m00 - m11 + 1.0)
    zq = 0.5 * s
    s = 0.5 / s
    return ((m20 + m02) * s, (m21 + m12) * s, zq, (m01 - m10) * s)


def set_target_rotation_local64(axis, sec, target, start):
    """ConfigurableJointExtensions.SetTargetRotationLocal in double precision"""
    n = norm64(cross64(axis, sec))
    n2 = norm64(cross64(n, axis))
    q = look_rotation64(n, n2)
    t2 = qinv64(q)
    t2 = qmul64(t2, qmul64(qinv64(target), start))
    t2 = qmul64(t2, q)
    return t2


def gen_joint_cases():
    rng = random.Random(5204)
    cases = []
    axes = [((0.0, 0.0, 1.0), (0.0, 1.0, 0.0)), ((1.0, 0.0, 0.0), (0.0, 1.0, 0.0)), ((0.0, 1.0, 0.0), (1.0, 0.0, 0.0)), ((0.0, 0.0, -1.0), (0.0, 1.0, 0.0)),
            ((0.0, -1.0, 0.0), (1.0, 0.0, 0.0)), ((-1.0, 0.0, 0.0), (0.0, 1.0, 0.0))]
    for axis, sec in axes:
        for _ in range(12):
            a = rng.uniform(-175.0, 175.0)
            start_a = rng.choice([0.0, rng.uniform(-30.0, 30.0)])
            target = qaxis64(a, axis)
            start = qaxis64(start_a, axis)
            q = set_target_rotation_local64(axis, sec, target, start)
            cases.append({"axis": list(axis), "sec": list(sec), "angle": a, "startAngle": start_a, "target": list(map(float, target)), "start": list(map(float, start)),
                          "expected": list(map(float, q))})
    return cases


def gen_drive_cases():
    """StepDrive1D (the documented D6 drive model) in double precision"""
    rng = random.Random(5205)
    cases = []
    for _ in range(40):
        k = rng.choice([0.0, 10.0, 1000.0, 1e5])
        c = rng.choice([0.0, 1.0, 100.0, 1e4])
        fmax = rng.choice([1e30, 5.0, 100.0, 1e4])
        m = rng.choice([0.1, 1.0, 5.0])
        dt = rng.choice([0.02, 0.01, 0.04])
        limited = rng.random() < 0.5
        lo, hi = -rng.uniform(0.1, 3.0), rng.uniform(0.1, 3.0)
        target = rng.uniform(-4.0, 4.0)
        tvel = rng.choice([0.0, 0.0, rng.uniform(-3.0, 3.0)])
        x, v = rng.uniform(-1.0, 1.0), rng.uniform(-1.0, 1.0)
        x0, v0 = x, v
        traj = []
        for _ in range(100):
            dm = dt / m
            v1 = (v + dm * (k * (target - x) + c * tvel)) / (1.0 + dm * (c + dt * k))
            force = m * (v1 - v) / dt
            if abs(force) > fmax:
                force = -fmax if force < 0.0 else fmax
                v1 = v + dt * force / m
            x1 = x + dt * v1
            if limited:
                if x1 < lo:
                    x1 = lo
                    if v1 < 0.0:
                        v1 = 0.0
                elif x1 > hi:
                    x1 = hi
                    if v1 > 0.0:
                        v1 = 0.0
            x, v = x1, v1
            traj.append([x, v, force])
        cases.append({"k": k, "c": c, "fmax": fmax, "m": m, "dt": dt, "limited": limited, "lo": lo, "hi": hi, "target": target, "tvel": tvel,
                      "x0": x0, "v0": v0, "traj": traj})
    return cases


# ----------------------------------------------------------------------------------------------------------------------- output
def jf(v):
    if isinstance(v, bool):
        return v
    if isinstance(v, float):
        if v != v:
            return "nan"
        if v == INF:
            return "inf"
        if v == -INF:
            return "-inf"
        if v == 0.0 and math.copysign(1.0, v) < 0:
            return "-0"
        return v
    if isinstance(v, (list, tuple)):
        return [jf(x) for x in v]
    if isinstance(v, dict):
        return {k: jf(x) for k, x in v.items()}
    return v


def main():
    out = {
        "servo": gen_servo_scenarios(),
        "controller": gen_controller_scenarios(),
        "presets": gen_preset_cases(),
        "joint": gen_joint_cases(),
        "drive": gen_drive_cases(),
    }
    json.dump(jf(out), sys.stdout, separators=(",", ":"))


if __name__ == "__main__":
    main()
