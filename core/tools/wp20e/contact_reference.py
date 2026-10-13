#!/usr/bin/env python3
"""WP-20e independent reference: the KSP statements of contact, landing and water, replayed by tests/test_contact_reference.cpp.

No kspcore code. Everything is written from the decompiled C# (Assembly-CSharp), Unity's managed math (Unity/UnityEngine.CoreModule) and
the game's Physics.cfg (harness/inputs/Physics.cfg, the values the World fixture loads):
  Part.cs           OnCollisionEnter (:3355), LandedCollisionChecks (:3417), OnCollisionExit (:3471), CheckOtherGroundContact (:3520),
                    CheckCollision (:3539), HandleCollision (:3613), OnCollisionStay (:3941)
  Vessel.cs         checkLanded (:1015), checkSplashed (:1044), AddRBAnchor (:1105), ResetRBAnchor (:1169), SetLandedAt (:1185),
                    updateSituation (:3133), UpdateLandedSplashed (:3190)
  MiniBiome.cs      ConvertTagtoLandedAt (:40, Vessel.GetLandedAtString)
  PartBuoyancy.cs   UpdateDisplacement (:171), FixedUpdate (:231)
  CollisionEnhancer.cs  FixedUpdate (:47)
  FlightGlobals.cs  getAltitudeAtPos (:1466 / :1471), getUpAxis (:1491), getGeeForceAtPosition (:1347)
  UnityEngine       Vector3 (operators, Dot, magnitude, normalized, Reflect, Lerp, Distance, Angle), Quaternion * Vector3, Matrix4x4.MultiplyPoint3x4,
                    Mathf (Max, Abs, Clamp01, Sign); Vector3d (KSP's, double; the Vector3 + Vector3d mixed operators give Vector3d)

The Mono float rule (core/README.md, ksp/dotnet/mono_float.hpp): a C# float expression is evaluated in double and rounded to float32 where the C#
stores, returns or passes it. Python floats hold the float32-exact values of C# floats; F() is the float32 store.

Fixtures that are NOT KSP data: every input (collision sequences, part poses, velocities, cube boxes, vessel states, body position) is generated
here from a fixed seed; only the code under test and Physics.cfg are the game's. Vector3.Angle (Math.Acos) is the only libm call (the anchor's
force angle): that column is compared to a stated tolerance by the test, every other value bit for bit.

Output: core/tests/data/wp20e_reference.json. Doubles as 16 hex digits of the IEEE-754 bits, float32 as 8.
Usage:  python -I core/tools/wp20e/contact_reference.py
"""
import json
import math
import os
import random
import struct

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
ROOT = os.path.abspath(os.path.join(CORE, ".."))
OUT = os.path.join(CORE, "tests", "data", "wp20e_reference.json")
PHYSICS_CFG = os.path.join(ROOT, "harness", "inputs", "Physics.cfg")

R = random.Random(2005)


def F(x):
    """C# float store of a double expression"""
    return float(np.float32(x))


def hx(x):
    return struct.pack(">d", float(x)).hex()


def hf(x):
    return struct.pack(">f", F(x)).hex()


def HF3(v):
    return [hf(c) for c in v]


def HX3(v):
    return [hx(c) for c in v]


def rf(lo, hi):
    """a random float32 value"""
    return F(R.uniform(lo, hi))


def rf3(lo, hi):
    return (rf(lo, hi), rf(lo, hi), rf(lo, hi))


# ---------------------------------------------------------------------------------------------------- Physics.cfg (the game's values)
def load_physics_cfg():
    vals = {}
    with open(PHYSICS_CFG, encoding="utf-8") as f:
        for line in f:
            line = line.split("//")[0].strip()
            if "=" not in line:
                continue
            k, v = [t.strip() for t in line.split("=", 1)]
            vals[k] = v
    return vals


CFG = load_physics_cfg()


def cfg_d(k):
    return float(CFG[k])


def cfg_f(k):  # a C# float field: float.Parse = the nearest float32
    return F(float(CFG[k]))


def cfg_b(k):
    return CFG[k].lower() == "true"


PG = {
    "BuoyancyScalar": cfg_d("buoyancyScalar"),
    "BuoyancyUseCoBOffset": cfg_b("buoyancyUseCoBOffset"),
    "BuoyancyApplyForceOnDie": cfg_b("buoyancyApplyForceOnDie"),
    "BuoyancyForceOffsetLerp": cfg_f("buoyancyForceOffsetLerp"),
    "BuoyancyWaterDragScalar": cfg_d("buoyancyWaterDragScalar"),
    "BuoyancyWaterDragScalarEnd": cfg_d("buoyancyWaterDragScalarEnd"),
    "BuoyancyWaterDragScalarLerp": cfg_d("buoyancyWaterDragScalarLerp"),
    "BuoyancyWaterDragScalarLerpDotMultBase": cfg_d("buoyancyWaterDragScalarLerpDotMultBase"),
    "BuoyancyWaterDragScalarLerpDotMult": cfg_d("buoyancyWaterDragScalarLerpDotMult"),
    "BuoyancyWaterLiftScalarEnd": cfg_d("buoyancyWaterLiftScalarEnd"),
    "BuoyancyWaterDragMinVel": cfg_d("buoyancyWaterDragMinVel"),
    "BuoyancyWaterDragMinVelMult": cfg_d("buoyancyWaterDragMinVelMult"),
    "BuoyancyWaterDragMinVelMultCOBOff": cfg_d("buoyancyWaterDragMinVelMultCOBOff"),
    "BuoyancyWaterDragPartVelGreaterVesselMult": cfg_d("buoyancyWaterDragPartVelGreaterVesselMult"),
    "BuoyancyWaterDragTimer": cfg_d("buoyancyWaterDragTimer"),
    "BuoyancyWaterDragMultMinForMinDot": cfg_d("buoyancyWaterDragMultMinForMinDot"),
    "BuoyancyAngularDragMinControlSqrMag": cfg_d("buoyancyAngularDragMinControlSqrMag"),
    "BuoyancyWaterAngularDragSlow": cfg_f("buoyancyWaterAngularDragSlow"),
    "BuoyancyWaterDragSlow": cfg_f("buoyancyWaterDragSlow"),
    "BuoyancyWaterDragExtraRBDragAboveDot": cfg_d("buoyancyWaterDragExtraRBDragAboveDot"),
    "BuoyancyScaleAboveDepth": cfg_d("buoyancyScaleAboveDepth"),
    "BuoyancyDefaultVolume": cfg_d("buoyancyDefaultVolume"),
    "BuoyancyMinCrashMult": cfg_f("buoyancyMinCrashMult"),
    "BuoyancyCrashToleranceMult": cfg_f("buoyancyCrashToleranceMult"),
    "BuoyancyRange": cfg_d("buoyancyRange"),
    "DragMultiplier": cfg_f("dragMultiplier"),
    "DragCubeMultiplier": cfg_f("dragCubeMultiplier"),
    "PartRBMassMin": cfg_f("partRBMassMin"),
    "GraviticForceMultiplier": 1.0,  # PhysicsGlobals.GraviticForceMultiplier (static, not in Physics.cfg)
}


# ---------------------------------------------------------------------------------------------------- UnityEngine.Vector3 (managed, Mono rule)
def v_add(a, b):
    return (F(a[0] + b[0]), F(a[1] + b[1]), F(a[2] + b[2]))


def v_sub(a, b):
    return (F(a[0] - b[0]), F(a[1] - b[1]), F(a[2] - b[2]))


def v_neg(a):
    return (-a[0], -a[1], -a[2])


def v_mul(a, s):
    return (F(a[0] * s), F(a[1] * s), F(a[2] * s))


def v_div(a, s):
    return (F(a[0] / s), F(a[1] / s), F(a[2] / s))


def v_dot(a, b):
    return F(a[0] * b[0] + a[1] * b[1] + a[2] * b[2])


def v_sqr(a):
    return F(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])


def v_mag(a):
    return F(math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]))


def v_norm(a):
    m = v_mag(a)
    return v_div(a, m) if m > 1e-05 else (0.0, 0.0, 0.0)


def v_reflect(d, n):
    num = F(-2.0 * v_dot(n, d))
    return (F(num * n[0] + d[0]), F(num * n[1] + d[1]), F(num * n[2] + d[2]))


def clamp01(t):
    return 0.0 if t < 0.0 else (1.0 if t > 1.0 else t)


def v_lerp(a, b, t):
    t = clamp01(t)
    return (F(a[0] + (b[0] - a[0]) * t), F(a[1] + (b[1] - a[1]) * t), F(a[2] + (b[2] - a[2]) * t))


def v_dist(a, b):
    return F(math.sqrt((a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 + (a[2] - b[2]) ** 2)) if False else v_mag(v_sub(a, b))


def v_invalid(a):
    return any(math.isnan(c) or math.isinf(c) for c in a)


def v_angle(a, b):
    """Vector3.Angle (libm acos: compared to a tolerance)"""
    num = F(math.sqrt(v_sqr(a) * v_sqr(b)))
    if num < 1e-15:
        return 0.0
    num2 = F(max(-1.0, min(1.0, v_dot(a, b) / num)))
    return F(F(math.acos(num2)) * F(57.29578))


def q_mul(q, p):
    """Quaternion * Vector3 (Quaternion.cs:181: float locals)"""
    x, y, z, w = q
    num = F(x * 2.0)
    num2 = F(y * 2.0)
    num3 = F(z * 2.0)
    num4 = F(x * num)
    num5 = F(y * num2)
    num6 = F(z * num3)
    num7 = F(x * num2)
    num8 = F(x * num3)
    num9 = F(y * num3)
    num10 = F(w * num)
    num11 = F(w * num2)
    num12 = F(w * num3)
    rx = F((1.0 - (num5 + num6)) * p[0] + (num7 - num12) * p[1] + (num8 + num11) * p[2])
    ry = F((num7 + num12) * p[0] + (1.0 - (num4 + num6)) * p[1] + (num9 - num10) * p[2])
    rz = F((num8 - num11) * p[0] + (num9 + num10) * p[1] + (1.0 - (num4 + num5)) * p[2])
    return (rx, ry, rz)


def m_point(m, p):
    """Matrix4x4.MultiplyPoint3x4 (Matrix4x4.cs:424); m = [m00, m01, m02, m03, m10, ..., m23]"""
    return (F(m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3]), F(m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7]),
            F(m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11]))


def to_f3(d):
    """the implicit Vector3d -> Vector3 conversion"""
    return (F(d[0]), F(d[1]), F(d[2]))


# Vector3d (double)
def d_sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def d_add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def d_mul(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def d_div(a, s):
    return (a[0] / s, a[1] / s, a[2] / s)


def d_neg(a):
    return (0.0 - a[0], 0.0 - a[1], 0.0 - a[2])


def d_mag(a):
    return math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])


def d_norm(a):
    m = d_mag(a)
    return d_div(a, m) if m > 0.0 else (0.0, 0.0, 0.0)


def d_dist(a, b):
    v = (a[0] - b[0], a[1] - b[1], a[2] - b[2])
    return math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])


# ---------------------------------------------------------------------------------------------------- the body (FlightGlobals statics)
# A synthetic Kerbin: its position (the world origin is elsewhere), radius and gravitational parameter are inputs of the C++ replay, which
# sets them on the fixture's Kerbin; the ocean density is KSP's (1 t/m^3)
BODY = {
    "position": (1523.25, -600000.0 - 812.5, 4321.125),
    "radius": 600000.0,
    "mu": 3.5316e12,
    "oceanDensity": 1.0,
}


def alt_d(p):
    """FlightGlobals.getAltitudeAtPos(Vector3d, body) (:1471)"""
    return d_dist(p, BODY["position"]) - BODY["radius"]


def alt_f(p):
    """FlightGlobals.getAltitudeAtPos(Vector3, body) (:1466): (float) of the double overload"""
    return F(alt_d(p))


def up_axis(p):
    """FlightGlobals.getUpAxis(body, Vector3d) (:1491)"""
    return d_norm(d_sub(p, BODY["position"]))


def gee_at(p):
    """FlightGlobals.getGeeForceAtPosition(pos, body) (:1347)"""
    pos = d_sub(p, BODY["position"])
    sq = pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]
    sq *= math.sqrt(sq)
    return d_div(d_mul(pos, 0.0 - BODY["mu"]), sq)


# ==================================================================================================== 1. HandleCollision decisions
def handle_collision(c, tol, no_crash):
    """Part.HandleCollision (:3613): the early-out, then the event the part's explode is reported with"""
    mag = v_mag(c["v"])
    if (not c["enabled"]) or (not c["active"]) or c["trigger"] or c["piston"] or mag <= tol or (c["hasRb"] and mag * c["mass"] <= tol):
        return {"explode": False, "ground": False, "event": "none", "param": hf(0.0)}
    ground = c["isPQ"]
    if no_crash:
        return {"explode": False, "ground": ground, "event": "none", "param": hf(0.0)}
    if c["isPQ"]:
        ev = "crash_pq"
    elif c["otherPart"]:
        ev = "collision"
    elif c["crashObject"]:
        ev = "crash_object"
    else:
        ev = "crash_name"
    return {"explode": True, "ground": ground, "event": ev, "param": hf(mag)}


def gen_handle():
    cases = []
    for i in range(1200):
        tol = R.choice([F(6.0), F(9.0), F(12.0), F(14.0), F(20.0), F(50.0), rf(1.0, 80.0)])
        kind = R.random()
        if kind < 0.15:  # |v| exactly at the tolerance, or one float step either side
            m = R.choice([0, 1, -1])
            vx = float(np.nextafter(np.float32(tol), np.float32(1e9 if m > 0 else -1e9))) if m else tol
            v = (F(vx), 0.0, 0.0)
        elif kind < 0.3:
            s = tol * R.uniform(0.85, 1.15)
            d = v_norm(rf3(-1, 1))
            v = (F(d[0] * s), F(d[1] * s), F(d[2] * s))
        else:
            v = rf3(-tol * 2, tol * 2)
        other = R.choice(["static", "pq", "part", "crashobject", "body"])
        has_rb = other in ("part", "body")
        mass = R.choice([F(0.5), F(0.01), F(2.0), rf(0.005, 40.0)])
        if has_rb and R.random() < 0.15:  # the momentum early-out exactly at the tolerance
            mag = v_mag(v)
            if mag > 0:
                mass = F(tol / mag)
        c = {
            "v": v, "enabled": R.random() > 0.05, "active": R.random() > 0.05, "trigger": R.random() < 0.05, "piston": R.random() < 0.05,
            "hasRb": has_rb, "mass": mass, "isPQ": other == "pq", "otherPart": other == "part", "crashObject": other == "crashobject",
        }
        no_crash = R.random() < 0.08
        out = handle_collision(c, tol, no_crash)
        cases.append({
            "v": HF3(c["v"]), "tol": hf(tol), "enabled": c["enabled"], "active": c["active"], "trigger": c["trigger"], "piston": c["piston"],
            "hasRb": has_rb, "mass": hf(mass), "other": other, "noCrash": no_crash, "out": out,
        })
    return cases


# ==================================================================================================== 2. collision state sequences
# The world of a sequence: vessel V = parts R (root, FULL), T (child of R, physicsless), M (child of T, FULL) (the C++ replay: the first three
# parts of PhysProf-S, T demoted to physicsless); other vessels O1 (landed at "Runway"), O2 (splashed), O3 (flying), O4 (an EVA kerbal, landed
# and splashed), each one part; static colliders S1 (layer 15, untagged), S2 ("LaunchPad"), S3 ("KSC_Grounds_Pond"), S4 (a terrain PQ),
# S5 (layer 0, not a part: a physical object)
PARTS = ["R", "T", "M"]
PARENT = {"R": None, "T": "R", "M": "T"}
CHILDREN = {"R": ["T"], "T": ["M"], "M": []}
FULL = {"R": True, "T": False, "M": True}
OTHERS = {
    "O1": {"landed": True, "splashed": False, "landedAt": "Runway", "display": "Runway", "eva": False},
    "O2": {"landed": False, "splashed": True, "landedAt": "", "display": "", "eva": False},
    "O3": {"landed": False, "splashed": False, "landedAt": "", "display": "", "eva": False},
    "O4": {"landed": True, "splashed": True, "landedAt": "KSC_Grounds", "display": "KSC", "eva": True},
}
STATICS = {
    "S1": {"layer": 15, "tag": "Untagged", "pq": False},
    "S2": {"layer": 15, "tag": "LaunchPad", "pq": False},
    "S3": {"layer": 15, "tag": "KSC_Grounds_Pond", "pq": False},
    "S4": {"layer": 15, "tag": "Untagged", "pq": True},
    "S5": {"layer": 0, "tag": "Untagged", "pq": False},
}


def convert_tag(tag):
    """MiniBiome.ConvertTagtoLandedAt (MiniBiome.cs:40)"""
    if not tag:
        return ""
    if "Pad" not in tag and "LaunchPad" not in tag:
        if "Runway" in tag:
            return "Runway"
        text = tag
        if "KSC" in tag and tag != "KSC":
            text = text.replace("KSC", "")
        if "Grounds" in tag and tag != "Grounds":
            text = text.replace("Grounds", "")
        return text.replace("_", " ").strip()
    return "LaunchPad"


class SeqWorld:
    def __init__(self, state):
        self.landed = state["landed"]
        self.splashed = False
        self.landedAt = state["landedAt"]
        self.display = state["landedAt"]
        self.situation = state["situation"]
        self.ignore = 0
        self.ground_contacts = []  # part names, insertion order
        self.ground = {p: False for p in PARTS}
        self.perm = {p: False for p in PARTS}
        self.current = {"R": [], "M": []}  # receivers' currentCollisions: [[collider, count]]
        self.after_reset = {"R": False, "M": False}
        self.other_state = {o: "IDLE" for o in OTHERS}
        self.tol = {p: F(1e30) for p in PARTS}
        self.out = []  # outputs of the last message

    # Vessel.SetLandedAt (:1185) with no facility / biome hooks: displaylandedAt is the landedAt string through GetLandedAtString
    def set_landed_at(self, landed_at, go_tag="", input_display=""):
        if self.landedAt == landed_at or landed_at == "Untagged":
            return
        self.landedAt = landed_at
        self.display = landed_at
        if input_display != "":
            self.display = input_display
            return
        if landed_at == "":
            return
        self.display = convert_tag(landed_at)

    def check_landed(self):  # Vessel.checkLanded (:1015), loaded, unpacked, not easing
        self.landed = False
        for p in PARTS:
            if self.ground[p] or self.perm[p]:
                self.landed = True
                break
        if not self.landed:
            self.set_landed_at("")

    def check_splashed(self):  # :1044 (no part has WaterContact)
        self.splashed = False

    def update_situation(self):  # :3133 (srfSpeed 0, not easing / spawning; staticPressure > 0)
        old = self.situation
        if self.landed:
            self.situation = "PRELAUNCH" if self.situation == "PRELAUNCH" else "LANDED"
        elif self.splashed:
            self.situation = "SPLASHED"
        else:
            self.situation = "FLYING"
        if self.situation != old:
            self.out.append("situation:" + old + ">" + self.situation)

    def update_landed_splashed(self):
        self.check_landed()
        self.check_splashed()
        self.update_situation()

    # ---- the collision record of a message
    def other_info(self, other):
        if other in STATICS:
            s = STATICS[other]
            return {"layer": s["layer"], "tag": s["tag"], "pq": s["pq"], "part": None}
        return {"layer": 0, "tag": "Untagged", "pq": False, "part": other}

    def check_other_ground(self, recv, collider):  # :3520
        for k, n in self.current[recv]:
            if k != collider and self.other_info(k)["layer"] == 15:
                return True
        return False

    def landed_checks(self, recv, c):  # :3417
        info = self.other_info(c["other"])
        if info["layer"] == 15:
            self.ground[recv] = True
            if recv not in self.ground_contacts:
                self.ground_contacts.append(recv)
            self.landed = True
            if not self.perm[recv] and info["tag"] != "Untagged":
                self.set_landed_at(info["tag"], info["tag"])
        if info["layer"] != 0:
            return
        o = info["part"]
        if o is not None and not self.check_other_ground(recv, c["other"]) and self.other_state[o] not in ("PLACEMENT", "CARGO"):
            ov = OTHERS[o]
            if ov["landed"] or ov["splashed"]:
                if not ov["eva"]:
                    self.landed = ov["landed"]
                else:
                    self.landed = ov["landed"] and not ov["splashed"]
                if ov["splashed"]:
                    self.splashed = True
                if not self.perm[recv]:
                    self.set_landed_at(ov["landedAt"], "", ov["display"])

    def handle(self, part, c):  # :3613 -> the explode is recorded (the replay takes over Part.explode)
        info = self.other_info(c["other"])
        has_rb = info["part"] is not None
        mag = v_mag(c["v"])
        mass = c["mass"]
        if mag <= self.tol[part] or (has_rb and mag * mass <= self.tol[part]):
            return
        if info["pq"]:
            self.out.append("ground:" + part)
        self.out.append("explode:" + part)
        if info["pq"]:
            self.out.append("crash_pq:" + part)
        elif info["part"] is not None:
            self.out.append("collision:" + part + ":" + info["part"])
        else:
            self.out.append("crash_name:" + part)

    def check_collision(self, part, c):  # :3539 (the part's only collider is its own)
        if c["this"] == part:
            self.handle(part, c)
            return True
        for ch in CHILDREN[part]:
            if not FULL[ch] and ch is not None and self.check_collision(ch, c):
                return True
        return False

    def enter(self, recv, c):  # :3355
        if self.after_reset[recv]:
            if not (c["this"] == recv):  # FindModelComponents<Collider>(): the receiver's own collider
                return
        found = False
        for e in self.current[recv]:
            if e[0] == c["other"]:
                e[1] += 1
                found = True
                break
        if not found:
            self.current[recv].append([c["other"], 1])
        info = self.other_info(c["other"])
        if info["layer"] == 0 and info["part"] is not None and self.other_state[info["part"]] in ("PLACEMENT", "CARGO"):
            return
        self.landed_checks(recv, c)
        if self.ignore > 0:
            return
        self.check_collision(recv, c)

    def exit_(self, recv, c):  # :3471
        idx = None
        for i, e in enumerate(self.current[recv]):
            if e[0] == c["other"]:
                idx = i
        if idx is not None:
            self.current[recv][idx][1] -= 1
            if self.current[recv][idx][1] > 0:
                return
            del self.current[recv][idx]
        info = self.other_info(c["other"])
        part = None
        if info["layer"] == 0:
            part = info["part"]
            if part is not None and self.other_state[part] not in ("PLACEMENT", "CARGO") and OTHERS[part]["splashed"]:
                self.splashed = False
        if (info["layer"] == 15 or info["layer"] == 0) and not self.check_other_ground(recv, c["other"]):
            self.ground[recv] = False
            if recv in self.ground_contacts:
                self.ground_contacts.remove(recv)
            self.update_landed_splashed()
        # :3517 currentCollisions.Remove(c.collider)
        self.current[recv] = [e for e in self.current[recv] if e[0] != c["other"]]

    def stay(self, recv, c):  # :3941
        if self.after_reset[recv]:
            self.enter(recv, c)

    def snapshot(self):
        return {
            "landed": self.landed, "splashed": self.splashed, "landedAt": self.landedAt, "display": self.display, "situation": self.situation,
            "groundContacts": list(self.ground_contacts), "ground": [self.ground[p] for p in PARTS],
            "current": {k: [[e[0], e[1]] for e in v] for k, v in self.current.items()}, "out": list(self.out),
        }


def gen_sequences():
    seqs = []
    colliders = list(STATICS.keys()) + list(OTHERS.keys())
    for i in range(300):
        start = R.choice([{"landed": False, "landedAt": "", "situation": "FLYING"}, {"landed": True, "landedAt": "LaunchPad", "situation": "PRELAUNCH"},
                          {"landed": False, "landedAt": "Runway", "situation": "FLYING"}])
        w = SeqWorld(start)
        tol = {p: R.choice([F(1e30), F(1e30), F(9.0), rf(3.0, 30.0)]) for p in PARTS}
        w.tol = tol
        perm_root = R.random() < 0.05
        w.perm["R"] = perm_root
        events = []
        live = []  # (recv, this, other) pairs entered and not yet exited
        n = R.randint(4, 16)
        for k in range(n):
            w.out = []
            ev = {"pre": {}}
            # random state changes before the message
            if R.random() < 0.15:
                w.ignore = R.choice([0, 0, 3, 60])
                ev["pre"]["ignore"] = w.ignore
            if R.random() < 0.08:
                r = R.choice(["R", "M"])
                w.after_reset[r] = not w.after_reset[r]
                ev["pre"]["afterReset"] = [r, w.after_reset[r]]
            if R.random() < 0.06:
                o = R.choice(list(OTHERS.keys()))
                w.other_state[o] = R.choice(["IDLE", "PLACEMENT", "CARGO"])
                ev["pre"]["otherState"] = [o, w.other_state[o]]
            if live and R.random() < 0.4:
                recv, this, other = live.pop(R.randrange(len(live)))
                kind = "exit"
            else:
                recv = R.choice(["R", "M"])
                this = recv if recv == "M" else R.choice(["R", "T"])
                other = R.choice(colliders)
                kind = R.choice(["enter", "enter", "enter", "stay"])
                if kind == "enter":
                    live.append((recv, this, other))
            vmag = R.choice([0.5, 5.0, 8.0, 12.0, 40.0])
            d = v_norm(rf3(-1, 1))
            v = (F(d[0] * vmag), F(d[1] * vmag), F(d[2] * vmag))
            mass = R.choice([F(0.5), F(0.05), F(3.0)])
            c = {"this": this, "other": other, "v": v, "mass": mass}
            if kind == "enter":
                w.enter(recv, c)
            elif kind == "stay":
                w.stay(recv, c)
            else:
                w.exit_(recv, c)
            ev.update({"kind": kind, "recv": recv, "this": this, "other": other, "v": HF3(v), "mass": hf(mass), "after": w.snapshot()})
            events.append(ev)
        seqs.append({"start": start, "tol": {p: hf(tol[p]) for p in PARTS}, "permRoot": perm_root, "events": events})
    return seqs


# ==================================================================================================== 3. PartBuoyancy
def quat_from(axis, ang):
    s = math.sin(ang / 2.0)
    n = d_norm(axis)
    return (F(n[0] * s), F(n[1] * s), F(n[2] * s), F(math.cos(ang / 2.0)))


def q_mul_q(a, b):  # float products (only to make test poses; not under test)
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (F(aw * bx + ax * bw + ay * bz - az * by), F(aw * by + ay * bw + az * bx - ax * bz), F(aw * bz + az * bw + ax * by - ay * bx),
            F(aw * bw - ax * bx - ay * by - az * bz))


def trs_matrix(pos, q):
    """the localToWorld matrix of a pose (the native TRS is not under test: the replay builds the same matrix from the pose with these statements,
    the rotation matrix of the float quaternion in double, each entry stored to float, and hands it to the code)"""
    x, y, z, w = q
    m00 = F(1 - 2 * (y * y + z * z))
    m01 = F(2 * (x * y - z * w))
    m02 = F(2 * (x * z + y * w))
    m10 = F(2 * (x * y + z * w))
    m11 = F(1 - 2 * (x * x + z * z))
    m12 = F(2 * (y * z - x * w))
    m20 = F(2 * (x * z - y * w))
    m21 = F(2 * (y * z + x * w))
    m22 = F(1 - 2 * (x * x + y * y))
    return [m00, m01, m02, pos[0], m10, m11, m12, pos[1], m20, m21, m22, pos[2]]


class Buoy:
    """PartBuoyancy's fields and the part / vessel / rigid-body state it reads and writes"""

    def __init__(self, part, vessel):
        self.p = part
        self.v = vessel
        self.rb_drag = F(0.0)
        self.rb_angular = None
        self.angularDragByFI = True
        self.splashed = False
        self.wasSplashed = False
        self.splashedCounter = 0.0
        self.dragScalar = 0.0
        self.liftScalar = 0.0
        self.submergedPortion = 0.0
        self.lastBuoyantForce = (0.0, 0.0, 0.0)
        self.lastForcePosition = (0.0, 0.0, 0.0)
        self.V = (0.0, 0.0, 0.0)
        self.pLast = (0.0, 0.0, 0.0)
        self.canForceUpdate = True
        self.displacement = 0.0
        self.maxDimension = 0.0
        self.xPortion = self.yPortion = self.zPortion = self.xzPortion = 0.0
        self.waterContact = False
        self.part_submerged = 0.0
        self.dead = False

    def override(self):
        return self.p["override"]

    def update_displacement(self):  # :171
        self.displacement = PG["BuoyancyDefaultVolume"]
        self.maxDimension = F(1.0)
        if not self.p["none"]:
            ov = self.override()
            if self.p["wsize"][0] == 0.0 and self.canForceUpdate and ov is None:
                raise RuntimeError("ForceUpdate path is not generated")
            arr = ov["area"] if ov is not None else self.p["warea"]
            vec = ov["size"] if ov is not None else self.p["wsize"]
            self.displacement = vec[0] * vec[1] * vec[2]  # float expression stored to a double: not rounded to float
            self.maxDimension = vec[0]
            if vec[1] > self.maxDimension:
                self.maxDimension = vec[1]
            if vec[1] > self.maxDimension:
                self.maxDimension = vec[2]
            self.xPortion = arr[0] / (vec[1] * vec[2])
            self.yPortion = arr[2] / (vec[0] * vec[2])
            self.zPortion = arr[4] / (vec[0] * vec[1])
            num = self.xPortion + self.yPortion + self.zPortion
            if not math.isnan(num) and num > 0.0:
                self.xzPortion = (min(self.xPortion, self.zPortion) + 2.0 * (self.xPortion * self.zPortion)) * (1.0 / 3.0)
                self.displacement *= self.xzPortion * self.yPortion
        elif self.p["colliderSize"] is not None:
            s = self.p["colliderSize"]
            self.displacement = (s[0] * s[1] * s[2]) * 0.75
            self.maxDimension = s[0]
            if s[1] > self.maxDimension:
                self.maxDimension = s[1]
            if s[1] > self.maxDimension:
                self.maxDimension = s[2]

    def start(self, pos, rot):  # :98
        self.dragScalar = PG["BuoyancyWaterDragScalar"]
        self.submergedPortion = 0.0
        self.liftScalar = 0.0
        self.update_displacement()
        cob = v_add(pos, q_mul(rot, self.p["cob"]))
        self.pLast = cob
        self.lastForcePosition = cob

    def fixed_update(self, t):  # :231
        p, v = self.p, self.v
        out = {}
        self.dead = False
        pos, rot, m = t["pos"], t["rot"], t["matrix"]
        dt = t["dt"]
        num = v["waterOffset"] + 0.0
        cob = v_add(pos, q_mul(rot, p["cob"]))
        cod = v_add(pos, q_mul(rot, p["cod"]))
        depth = 0.0 - alt_d(cod) + num
        depths = [0.0] * 8
        coords = [None] * 8
        if (not p["shielded"]) and (not p["none"]) and depth > 0.0 - PG["BuoyancyRange"] and v["terrainAltitude"] < 20.0:
            minDepth = 1.7976931348623157e308
            maxDepth = -1.7976931348623157e308
            ov = self.override()
            bc, bs = (ov["center"], ov["size"]) if ov is not None else (p["wcenter"], p["wsize"])
            ext = v_mul(bs, 0.5)
            bmin = v_sub(bc, ext)
            bmax = v_add(bc, ext)
            for i in range(8):
                c = (bmax[0] if i & 4 else bmin[0], bmax[1] if i & 2 else bmin[1], bmax[2] if i & 1 else bmin[2])
                coords[i] = m_point(m, c)
                depths[i] = (0.0 - alt_f(coords[i])) + num
                if depths[i] > maxDepth:
                    maxDepth = depths[i]
                if depths[i] < minDepth:
                    minDepth = depths[i]
        else:
            minDepth = maxDepth = depth
        out["depth"] = hx(depth)
        out["minDepth"] = hx(minDepth)
        out["maxDepth"] = hx(maxDepth)
        rbv = t["rbv"]
        fv = t["frameVel"]
        lvm = v_mag(rbv) + d_mag(fv)
        down = to_f3(d_neg(up_axis(cob)))
        if lvm == 0.0:
            num2 = F(1.0)
        else:
            num2 = v_dot(to_f3(d_div(d_add(rbv, fv), F(lvm))), down)
        sv = v["srfVelDir"]
        if to_f3(sv) == (0.0, 0.0, 0.0):
            num3 = 1.0
        else:
            num3 = max(abs(num2), abs(v_dot(to_f3(sv), down)))
        num4 = PG["BuoyancyWaterDragScalarLerpDotMultBase"] - num3 * PG["BuoyancyWaterDragScalarLerpDotMult"]
        early = self.splashedCounter < PG["BuoyancyWaterDragTimer"]
        slow = lvm < PG["BuoyancyWaterDragMinVel"] * PG["BuoyancyWaterDragMinVelMult"]
        num5 = PG["BuoyancyWaterDragScalar"]
        if early:
            num5 *= 0.5 + num3
        end, lerp = PG["BuoyancyWaterDragScalarEnd"], PG["BuoyancyWaterDragScalarLerp"]
        lend = PG["BuoyancyWaterLiftScalarEnd"]
        if self.splashed and not slow:
            if not early:
                if self.dragScalar > end:
                    self.dragScalar += (end - self.dragScalar) * lerp * num4 * dt
                    if self.dragScalar < end:
                        self.dragScalar = end
                else:
                    self.dragScalar = end
                if self.liftScalar < lend:
                    if early:
                        self.liftScalar = 0.0
                    else:
                        self.liftScalar += (lend - self.liftScalar) * lerp * dt
                        if self.liftScalar > lend:
                            self.liftScalar = lend
                else:
                    self.liftScalar = lend
        else:
            if self.dragScalar < num5:
                self.dragScalar += (num5 - self.dragScalar) * lerp * dt
                if self.dragScalar > num5:
                    self.dragScalar = num5
            else:
                self.dragScalar = num5
            if self.liftScalar > 0.0:
                self.liftScalar -= self.liftScalar * lerp * dt
                if self.liftScalar < 0.0:
                    self.liftScalar = 0.0
        if self.splashed:
            if early:
                self.splashedCounter += dt
        else:
            self.splashedCounter = 0.0
        applied = None
        crash = False
        fx = None
        if (not p["shielded"]) and maxDepth > 0.0 and not p["dead"]:
            self.update_displacement()
            if not self.splashed:
                self.pLast = v_sub(cob, v_mul(to_f3(fv), dt))
                self.V = v_sub(cob, self.pLast)
                dH = v_dot(self.V, down)
                num6 = F(num2 * F(lvm))
                cam = t["camera"]
                if lvm > 2.0 and v_dist(pos, to_f3(cam)) < 5000.0:
                    fpos = v_add(cob, to_f3(d_mul(down, maxDepth - depth - dH + 0.0)))
                    fx = [HF3(fpos), hf(F(num6 * F(0.1) * self.maxDimension))]
                self.splashed = True
                crash_speed = max(F(PG["BuoyancyMinCrashMult"] * F(lvm)), num6)
                if crash_speed > p["crashTolerance"] * PG["BuoyancyCrashToleranceMult"]:
                    crash = True
                    self.dead = True
            self.waterContact = True
            v["splashed"] = True
            sub = 1.0
            vec = cob
            if minDepth < 0.0 and minDepth < maxDepth:
                if depth <= 0.0 and depth != maxDepth:
                    sub = maxDepth / (maxDepth - depth) * 0.5
                else:
                    sub = 0.5 + depth / (depth - minDepth) * 0.5
                if p["useSine"]:
                    sub *= math.pi
                    sub -= math.pi / 2.0
                    # Math.Sin is the runtime's (x87 FSIN semantics in KSP's Mono, ksp/dotnet/math.hpp); Python's libm can differ by an ulp:
                    # the replay compares the two and, where they differ, the values that follow from it to a stated tolerance
                    out["sinArg"] = hx(sub)
                    out["sin"] = hx(math.sin(sub))
                    sub = math.sin(sub) * 0.5 + 0.5
                if PG["BuoyancyUseCoBOffset"]:
                    zero = (0.0, 0.0, 0.0)
                    num8 = F(0.0)
                    for k in range(7, -1, -1):
                        num10 = F(depths[k])
                        if num10 > 0.0:
                            zero = v_add(zero, v_mul(coords[k], num10))
                            num8 = F(num8 + num10)
                    zero = v_div(zero, num8)
                    vec = v_add(cob, v_mul(v_sub(v_add(zero, q_mul(rot, p["cob"])), cob), F(1.0 - sub)))
            self.submergedPortion = sub
            self.part_submerged = sub
            ramp = (maxDepth / PG["BuoyancyScaleAboveDepth"]) if maxDepth < PG["BuoyancyScaleAboveDepth"] else 1.0
            bgf = self.displacement * sub * BODY["oceanDensity"] * ramp * PG["BuoyancyScalar"] * p["buoyancy"]
            g = gee_at(vec)
            eff = to_f3(d_mul(d_mul(d_mul(d_neg(g), bgf), v["gravityMultiplier"]), PG["GraviticForceMultiplier"]))
            out["buoyantGeeForce"] = hx(bgf)
            if early and not self.wasSplashed:
                mm = PG["BuoyancyWaterDragMultMinForMinDot"]
                self.dragScalar = min(self.dragScalar, (mm + num3 * (1.0 - mm)) * (num5 - end) + end)
            if lvm < PG["BuoyancyWaterDragMinVel"] * PG["BuoyancyWaterDragMinVelMultCOBOff"]:
                vec = v_lerp(v_add(self.lastForcePosition, self.V), vec, PG["BuoyancyForceOffsetLerp"])
            if not (lvm < PG["BuoyancyWaterDragMinVel"]) and lvm <= v["srfSpeed"] * PG["BuoyancyWaterDragPartVelGreaterVesselMult"]:
                self.angularDragByFI = True
                self.rb_angular = F(0.0)
                num11 = 1.0 if p["none"] else p["areaDrag"]
                num12 = 0.5 * BODY["oceanDensity"] * lvm * num11 * PG["DragCubeMultiplier"] * self.dragScalar * sub * PG["DragMultiplier"]
                mass = t["mass"]
                self.rb_drag = F(num12 / (mass if mass != 0.0 else F(0.1)))
            else:
                c = v["ctrl"]
                if c[0] * c[0] + c[1] * c[1] + c[2] * c[2] < PG["BuoyancyAngularDragMinControlSqrMag"]:
                    self.angularDragByFI = False
                    self.rb_angular = PG["BuoyancyWaterAngularDragSlow"]
                else:
                    self.angularDragByFI = True
                    self.rb_angular = F(0.0)
                self.rb_drag = PG["BuoyancyWaterDragSlow"]
            physwarp = t["rate"] > 1.0 and t["low"]
            if early:
                if physwarp:
                    self.rb_drag = F(self.rb_drag + F((self.dragScalar - end) * dt * 50.0))
                    if t["rate"] > 2.0:
                        eff = v_lerp(self.lastBuoyantForce, eff, F(1.25 - t["rate"] * 0.25))
                elif num3 > PG["BuoyancyWaterDragExtraRBDragAboveDot"]:
                    self.rb_drag = F(self.rb_drag + F(num3 - PG["BuoyancyWaterDragExtraRBDragAboveDot"]))
            if v_invalid(eff):
                eff = (0.0, 0.0, 0.0)
            self.lastBuoyantForce = eff
            self.lastForcePosition = vec
            out["effectiveForce"] = HF3(eff)
            if self.dead:
                if PG["BuoyancyApplyForceOnDie"]:
                    applied = ["parent", HF3(eff), HF3(vec)]
            else:
                applied = ["self", HF3(eff), HF3(vec)]
        else:
            self.waterContact = False
            if self.splashed:
                v["splashed"] = False  # vessel.checkSplashed(): the only part of the vessel left the water
                self.splashed = False
                self.rb_drag = F(0.0)
            self.part_submerged = 0.0
            self.submergedPortion = 0.0
            self.lastBuoyantForce = (0.0, 0.0, 0.0)
            self.lastForcePosition = cob
        self.wasSplashed = self.splashed
        out.update({
            "lvm": hx(lvm), "submerged": hx(self.part_submerged), "dragScalar": hx(self.dragScalar), "liftScalar": hx(self.liftScalar),
            "splashedCounter": hx(self.splashedCounter), "splashed": self.splashed, "waterContact": self.waterContact, "vesselSplashed": v["splashed"],
            "rbDrag": hf(self.rb_drag), "rbAngular": None if self.rb_angular is None else hf(self.rb_angular), "angularDragByFI": self.angularDragByFI,
            "displacement": hx(self.displacement), "maxDimension": hf(self.maxDimension), "applied": applied, "crash": crash, "dead": self.dead,
            "fx": fx, "lastForcePosition": HF3(self.lastForcePosition),
        })
        return out


def gen_buoyancy():
    cases = []
    bp = BODY["position"]
    for i in range(110):
        wsize = (rf(0.3, 3.0), rf(0.3, 4.0), rf(0.3, 3.0))
        part = {
            "cob": R.choice([(0.0, 0.0, 0.0), rf3(-0.4, 0.4)]),
            "cod": R.choice([(0.0, 0.0, 0.0), rf3(-0.3, 0.3)]),
            "buoyancy": R.choice([F(1.0), F(0.1), F(0.5), F(2.0)]),
            "useSine": R.random() > 0.2,
            "crashTolerance": R.choice([F(6.0), F(9.0), F(14.0), F(50.0)]),
            "wcenter": rf3(-0.3, 0.3),
            "wsize": wsize,
            "warea": [rf(0.05, wsize[1] * wsize[2]), rf(0.05, 3), rf(0.05, wsize[0] * wsize[2]), rf(0.05, 3), rf(0.05, wsize[0] * wsize[1]), rf(0.05, 3)],
            "areaDrag": rf(0.1, 4.0),
            "none": R.random() < 0.05,
            "colliderSize": rf3(0.3, 2.0),
            "shielded": R.random() < 0.04,
            "dead": False,
            "override": None,
        }
        if R.random() < 0.2:
            osz = (rf(0.3, 2.0), rf(0.3, 2.0), rf(0.3, 2.0))
            part["override"] = {"center": rf3(-0.2, 0.2), "size": osz, "area": [rf(0.05, 3) for _ in range(6)]}
        vessel = {
            "waterOffset": R.choice([0.0, 0.0, 0.25]),
            "terrainAltitude": R.choice([-1.0, -50.0, 10.0, 50.0]),
            "srfVelDir": R.choice([(0.0, 0.0, 0.0), d_norm((R.uniform(-1, 1), R.uniform(-1, 1), R.uniform(-1, 1)))]),
            "splashed": False,
            "srfSpeed": R.choice([0.0, 1.0, 5.0, 20.0]),
            "ctrl": R.choice([(F(0.0), F(0.0), F(0.0)), (F(0.3), F(0.0), F(0.0)), (F(0.1), F(-0.1), F(0.05))]),
            "gravityMultiplier": R.choice([1.0, 1.0, 0.5]),
        }
        # a path: start h0 above the sea, move down at `vz` with a bob and a sideways drift; attitude turning slowly
        n = R.choice([15, 30, 60, 170])
        lat, lon = R.uniform(-1.2, 1.2), R.uniform(-3.1, 3.1)
        up = (math.cos(lat) * math.cos(lon), math.sin(lat), math.cos(lat) * math.sin(lon))
        side = d_norm((-up[2], 0.0, up[0])) if abs(up[1]) < 0.99 else (1.0, 0.0, 0.0)
        h0 = R.uniform(-2.0, 4.0)
        vz = R.choice([0.0, 0.5, 3.0, 8.0, 20.0])
        bob = R.uniform(0.0, 1.5)
        q0 = quat_from((R.uniform(-1, 1), R.uniform(-1, 1), R.uniform(-1, 1)), R.uniform(0, math.pi))
        dq = quat_from((R.uniform(-1, 1), R.uniform(-1, 1), R.uniform(-1, 1)), R.uniform(0, 0.03))
        rate = R.choice([1.0, 1.0, 1.0, 2.0, 3.0, 4.0])
        low = rate > 1.0 or R.random() < 0.1
        dt = F(0.02 * rate) if low else F(0.02)
        fvel = R.choice([(0.0, 0.0, 0.0), (0.0, 0.0, 0.0), (12.5, -3.25, 0.5)])
        start = d_add(bp, d_mul(up, BODY["radius"] + h0))
        camera = R.choice([d_add(start, (R.uniform(-50, 50), R.uniform(-50, 50), R.uniform(-50, 50))), (1.0e7, 0.0, 0.0)])
        ticks = []
        q = q0
        for k in range(n):
            tt = k * 0.02
            h = h0 - vz * tt + bob * math.sin(tt * 2.1)
            off = R.uniform(-0.01, 0.01)
            world = d_add(d_add(bp, d_mul(up, BODY["radius"] + h)), d_mul(side, tt * 1.5 + off))
            pos = to_f3(world)
            q = q_mul_q(dq, q)
            vel = to_f3(d_add(d_mul(up, -vz + bob * 2.1 * math.cos(tt * 2.1)), d_mul(side, 1.5)))
            if R.random() < 0.05:
                vel = (0.0, 0.0, 0.0)
            ticks.append({"pos": pos, "rot": q, "matrix": trs_matrix(pos, q), "rbv": vel, "mass": R.choice([F(0.5), F(0.0), rf(0.01, 20.0)]) if k == 0 else None,
                          "frameVel": fvel, "camera": camera, "dt": dt, "rate": rate, "low": low})
        mass = ticks[0]["mass"]
        for t in ticks:
            t["mass"] = mass
        b = Buoy(part, vessel)
        b.start(ticks[0]["pos"], ticks[0]["rot"])
        outs = []
        for t in ticks:
            o = b.fixed_update(t)
            outs.append(o)
            if b.dead:
                break
        cases.append({
            "part": {
                "cob": HF3(part["cob"]), "cod": HF3(part["cod"]), "buoyancy": hf(part["buoyancy"]), "useSine": part["useSine"],
                "crashTolerance": hf(part["crashTolerance"]), "wcenter": HF3(part["wcenter"]), "wsize": HF3(part["wsize"]),
                "warea": [hf(a) for a in part["warea"]], "areaDrag": hf(part["areaDrag"]), "none": part["none"], "colliderSize": HF3(part["colliderSize"]),
                "shielded": part["shielded"],
                "override": None if part["override"] is None else {"center": HF3(part["override"]["center"]), "size": HF3(part["override"]["size"]),
                                                                     "area": [hf(a) for a in part["override"]["area"]]},
            },
            "vessel": {
                "waterOffset": hx(vessel["waterOffset"]), "terrainAltitude": hx(vessel["terrainAltitude"]), "srfVelDir": HX3(vessel["srfVelDir"]),
                "srfSpeed": hx(vessel["srfSpeed"]), "ctrl": HF3(vessel["ctrl"]), "gravityMultiplier": hx(vessel["gravityMultiplier"]),
            },
            "rate": hf(rate), "low": low, "dt": hf(dt), "mass": hf(mass), "frameVel": HX3(fvel), "camera": HX3(camera),
            "ticks": [{"pos": HF3(t["pos"]), "rot": HF3(t["rot"]) + [hf(t["rot"][3])], "rbv": HF3(t["rbv"])} for t in ticks[:len(outs)]],
            "out": outs,
        })
    return cases


# ==================================================================================================== 4. CollisionEnhancer
def gen_enhancer():
    cases = []
    behaviours = ["EXPLODE", "TRANSLATE_BACK", "TRANSLATE_BACK_SPLAT"]
    for i in range(400):
        tol = R.choice([F(6.0), F(9.0), F(20.0)])
        beh = R.choice(behaviours + ["EXPLODE"])
        inst = R.choice([F(0.0), F(0.0), F(0.25)])
        state = {"lastPos": rf3(-50, 50), "wasPacked": R.random() < 0.3, "framesToSkip": R.choice([0, 0, 0, 2])}
        ticks = []
        for k in range(R.randint(1, 6)):
            t = {
                "packed": R.random() < 0.1, "bypass": R.random() < 0.1, "dead": R.random() < 0.03,
                "pos": rf3(-50, 50) if R.random() < 0.3 else v_add(state["lastPos"], rf3(-3, 3)),
                "shift": R.choice([(0.0, 0.0, 0.0), (0.0, 0.0, 0.0), (R.uniform(-600, 600), R.uniform(-600, 600), R.uniform(-600, 600))]),
                "rbv": rf3(-30, 30), "frameVel": R.choice([(0.0, 0.0, 0.0), (R.uniform(-900, 900), R.uniform(-900, 900), R.uniform(-900, 900))]),
                "noCrash": R.random() < 0.05,
            }
            if R.random() < 0.4:
                t["rbv"] = v_mul(v_norm(rf3(-1, 1)), R.choice([F(tol * 0.9), F(tol * 1.1), F(2.0)]))
            hit = R.random() < 0.6
            t["hit"] = None
            if hit:
                hn = v_norm(rf3(-1, 1))
                t["hit"] = {"point": rf3(-50, 50), "normal": hn, "hasRb": R.random() < 0.3, "rbv": rf3(-10, 10), "mass": R.choice([F(0.5), F(3.0)])}
                if R.random() < 0.05:
                    t["hit"]["point"] = (float("nan"), 0.0, 0.0)
            t["up"] = v_norm(rf3(-1, 1))
            ticks.append(t)
        # replay
        lastPos, wasPacked, fts = state["lastPos"], state["wasPacked"], state["framesToSkip"]
        outs = []
        for t in ticks:
            o = {"linecast": None, "explode": False, "setPos": None, "setVel": None, "hitEvent": False, "collision": None}
            pos = t["pos"]
            if t["packed"]:
                lastPos = pos
                wasPacked = True
            elif fts > 0:
                lastPos = pos
                fts -= 1
            else:
                if not wasPacked:  # lastPos -= FloatingOrigin.Offset: Vector3 - Vector3d (double), stored back to a Vector3
                    lastPos = (F(lastPos[0] - t["shift"][0]), F(lastPos[1] - t["shift"][1]), F(lastPos[2] - t["shift"][2]))
                wasPacked = False
                thr = inst if inst > 0.0 else F(0.010000001)
                if (not t["bypass"]) and (not t["dead"]) and v_sqr(v_sub(lastPos, pos)) > thr:
                    o["linecast"] = [HF3(lastPos), HF3(pos)]
                    h = t["hit"]
                    flag = False
                    if h is not None:
                        rbv = t["rbv"]
                        vec = t["up"]
                        fv3 = to_f3(t["frameVel"])

                        def tb():
                            vf = v_add(rbv, fv3)
                            n = v_mul(h["normal"], F(1.0 if v_dot(h["normal"], vec) >= 0.0 else -1.0))
                            return v_sub(v_mul(v_mul(v_norm(v_reflect(v_norm(vf), n)), v_mag(v_add(rbv, fv3))), F(0.2)), fv3)

                        def setvel(x):
                            o["setVel"] = HF3((0.0, 0.0, 0.0) if v_invalid(x) else x)

                        if beh == "EXPLODE":
                            if h["hasRb"]:
                                if v_mag(v_sub(rbv, h["rbv"])) * h["mass"] > tol and not t["noCrash"]:
                                    o["collision"] = "rb"
                                    o["explode"] = True
                                else:
                                    if not v_invalid(h["point"]):
                                        o["setPos"] = HF3(v_add(h["point"], v_mul(vec, F(0.25))))
                                    setvel(tb())
                            elif v_sqr(rbv) > tol * tol and not t["noCrash"]:
                                o["collision"] = "terrain"
                                o["explode"] = True
                            else:
                                if not v_invalid(h["point"]):
                                    o["setPos"] = HF3(v_add(h["point"], v_mul(vec, F(0.25))))
                                setvel(tb())
                        elif beh == "TRANSLATE_BACK":
                            if not v_invalid(h["point"]):
                                o["setPos"] = HF3(v_add(h["point"], v_mul(vec, F(0.25))))
                            setvel(tb())
                        else:
                            if not v_invalid(h["point"]):
                                o["setPos"] = HF3(v_add(h["point"], v_mul(vec, F(0.25))))
                            o["setVel"] = HF3((0.0, 0.0, 0.0))
                        flag = True
                    if flag:
                        o["hitEvent"] = True
                # lastPos = transform.position: the position after any teleport
                lastPos = pos
                if o["setPos"] is not None:
                    lastPos = tuple(struct.unpack(">f", bytes.fromhex(c))[0] for c in o["setPos"])
            o["lastPos"] = HF3(lastPos)
            o["framesToSkip"] = fts
            o["wasPacked"] = wasPacked
            outs.append(o)
        cases.append({
            "tol": hf(tol), "behaviour": beh, "instMinDistSqr": hf(inst), "state": {"lastPos": HF3(state["lastPos"]), "wasPacked": state["wasPacked"],
                                                                                    "framesToSkip": state["framesToSkip"]},
            "ticks": [{"packed": t["packed"], "bypass": t["bypass"], "dead": t["dead"], "pos": HF3(t["pos"]), "shift": HX3(t["shift"]), "rbv": HF3(t["rbv"]),
                       "frameVel": HX3(t["frameVel"]), "noCrash": t["noCrash"], "up": HF3(t["up"]),
                       "hit": None if t["hit"] is None else {"point": HF3(t["hit"]["point"]), "normal": HF3(t["hit"]["normal"]), "hasRb": t["hit"]["hasRb"],
                                                             "rbv": HF3(t["hit"]["rbv"]), "mass": hf(t["hit"]["mass"])}} for t in ticks],
            "out": outs,
        })
    return cases


# ==================================================================================================== 5. Vessel.AddRBAnchor
def gen_anchor():
    cases = []
    vel_thr, time_thr = F(0.05), F(1.5)
    ang_thr, ang_time = F(20.0), F(0.1)
    bf_factor, bt = F(1.05), F(100.0)
    for i in range(90):
        mass = R.choice([35.2, 0.001, R.uniform(1.0, 700.0)])
        g = R.uniform(9.0, 9.9)
        pos = rf3(-30, 30)
        up = to_f3(up_axis(pos))
        gacc = d_mul(d_norm(d_sub(BODY["position"], pos)), g)
        gmag = d_mag(gacc)
        joint = False
        timer = F(0.0)
        dtimer = F(0.0)
        prev = F(-1.0)
        angle = F(0.0)
        delta = F(0.0)
        frames = []
        active = R.random() < 0.5
        n = R.randint(60, 160)
        regime = R.choice(["still", "still", "jitter", "turning", "lift"])
        for k in range(n):
            dtf = R.choice([F(0.02), F(0.02), F(0.016666668), F(0.033333335)])
            f = {
                "dt": dtf, "loaded": R.random() > 0.01, "packed": R.random() < 0.01, "easing": k < 10 or R.random() < 0.01,
                "landed": R.random() > 0.03, "rbv": v_mul(v_norm(rf3(-1, 1)), R.choice([F(0.0), F(0.1), F(0.3)])),
                "throttle": F(0.0) if k < n * 0.8 else R.choice([F(0.0), F(0.5)]), "wheel": F(0.0),
            }
            w = F(mass * g)
            if regime == "still":
                force = v_mul(up, w)
            elif regime == "jitter":
                force = v_add(v_mul(up, w), rf3(-w * 0.5, w * 0.5))
            elif regime == "turning":
                force = v_mul(v_norm(v_add(up, (F(math.sin(k * 0.3)), F(math.cos(k * 0.3)), 0.0))), w)
            else:
                force = v_mul(up, F(w * (1.0 - k / 60.0)))
            f["force"] = force
            out = {"create": None, "destroy": False, "breakForce": None}
            # Vessel.cs:1105 (not EVA, not a flag, not DEAD; no ground contact with an anchor-exception module)
            timer = F(timer + dtf)
            if joint:
                if active and (f["wheel"] != 0.0 or f["throttle"] > 0.0):
                    joint = False
                    out["destroy"] = True
                    timer, dtimer, prev = F(0.0), F(0.0), F(-1.0)
                else:
                    angle = v_angle(force, (0.0, 1.0, 0.0))
                    reset = False
                    if dtimer >= ang_time:
                        delta = F(angle - prev) if prev > 0.0 else F(0.0)
                        if abs(delta) >= ang_thr:
                            joint = False
                            out["destroy"] = True
                            timer, dtimer, prev = F(0.0), F(0.0), F(-1.0)
                            reset = True
                        else:
                            dtimer = F(0.0)
                            prev = angle
                    if not reset:
                        dtimer = F(dtimer + dtf)
                        if v_dot(force, up) < 0.0:
                            joint = False
                            out["destroy"] = True
                            timer, dtimer, prev = F(0.0), F(0.0), F(-1.0)
                        else:
                            out["breakForce"] = hf(F(max(F(mass), PG["PartRBMassMin"]) * F(gmag) * bf_factor))
            elif f["loaded"] and not f["packed"] and not f["easing"] and f["landed"] and v_sqr(f["rbv"]) < vel_thr and timer >= time_thr:
                timer, dtimer, prev = F(0.0), F(0.0), F(-1.0)
                bfv = F(max(F(mass), PG["PartRBMassMin"]) * F(gmag) * bf_factor)
                out["create"] = [hf(bfv), hf(bt)]
                joint = True
            out.update({"joint": joint, "timer": hf(timer), "dtimer": hf(dtimer), "prev": hf(prev), "angle": hf(angle), "delta": hf(delta)})
            f["out"] = out
            frames.append(f)
        cases.append({
            "mass": hx(mass), "gacc": HX3(gacc), "pos": HF3(pos), "active": active,
            "frames": [{"dt": hf(f["dt"]), "loaded": f["loaded"], "packed": f["packed"], "easing": f["easing"], "landed": f["landed"], "rbv": HF3(f["rbv"]),
                        "throttle": hf(f["throttle"]), "force": HF3(f["force"]), "out": f["out"]} for f in frames],
        })
    return cases


def main():
    data = {
        "provenance": "tools/wp20e/contact_reference.py (independent: decompiled C# + Unity managed math + harness/inputs/Physics.cfg), seed 2005",
        "body": {"position": HX3(BODY["position"]), "radius": hx(BODY["radius"]), "mu": hx(BODY["mu"]), "oceanDensity": hx(BODY["oceanDensity"])},
        "handle": gen_handle(),
        "sequences": gen_sequences(),
        "buoyancy": gen_buoyancy(),
        "enhancer": gen_enhancer(),
        "anchor": gen_anchor(),
    }
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, separators=(",", ":"), sort_keys=True)
        f.write("\n")
    n_ticks = sum(len(c["out"]) for c in data["buoyancy"])
    print(f"wrote {OUT}: {len(data['handle'])} HandleCollision cases, {len(data['sequences'])} collision sequences "
          f"({sum(len(s['events']) for s in data['sequences'])} messages), {len(data['buoyancy'])} buoyancy cases ({n_ticks} ticks), "
          f"{len(data['enhancer'])} CollisionEnhancer cases, {len(data['anchor'])} anchor timelines")


if __name__ == "__main__":
    main()
