#!/usr/bin/env python3
"""WP-29b: independent reference for the KSP wheel part modules -- test data generator.

Written from the decompiled C# ONLY, before the C++ port existed; it shares no code with it. The C++ port was then written from the same C#
(not from this file), so the two agree only where both transcribe the C# correctly. Circularity is avoided three ways: (1) this file is a
separate implementation in another language with its own object model (plain Python objects, a generator-based coroutine scheduler, its own
FSM, curve and animation models); (2) the inputs are generated here (seeded) and the C++ side only replays them; (3) the stock wheel
parameters are parsed here from the part .cfg files of core/testdata/GameData-mirror, the C++ side reads them through its own module host and
the test asserts that both read the same bits.

Sources, all under decompiled/KSP-Decompiled/Assembly-CSharp/:
  ModuleWheelBase.cs, ModuleWheels/{ModuleWheelSubmodule, ModuleWheelMotor, ModuleWheelMotorSteering, ModuleWheelSteering, ModuleWheelBrakes,
  ModuleWheelLock, ModuleWheelSuspension, ModuleWheelDamage, ModuleWheelDeployment, ModuleWheelBogey, WheelSubsystem(s), WheelUtil}.cs,
  SuspensionLoadBalancer.cs, WheelType.cs, FloatCurve.cs, KerbalFSM.cs, CallbackUtil.cs, KSPUtil.cs (Bearing*), KSPWheelController.cs
  (LegacyWheelLoad), Unity/UnityEngine.CoreModule/UnityEngine/{Mathf, Vector3, Quaternion}.cs (the managed parts).

Float32 follows the Mono rule (core/include/ksp/dotnet/mono_float.hpp, CAPTURE-SPEC C7): a float expression is evaluated in double and rounded
(st()) only where the C# stores it in a float local / field, passes it as a float argument, returns it or casts it.

What is a seam, not logic (scripted per tick, recorded in the output so the replay is identical): the native WheelCollider / KSPWheelController
state (WheelState values, the effective spring), part and vessel transforms, the vessel's body gravity, the electric-charge store, the collider
kinds a wheel can touch, the Unity Animation clock. Natives without C# source follow the same documented formulations as the WP-29a / WP-27
references (PartTransform = rotation * p + position, Animation = time += speed * dt, clamp forever) and are UNVERIFIED against the game.

Usage: python -I core/tools/wp29b/wheel_reference.py > core/tests/data/wp29b_reference.json
"""
import json
import math
import os
import random
import sys

import numpy as np

np.seterr(all="ignore")
F = np.float32
INF = float("inf")
NAN = float("nan")
FLT_MAX = float(F(3.4028234663852886e38))
HERE = os.path.dirname(os.path.abspath(__file__))
MIRROR = os.path.normpath(os.path.join(HERE, "..", "..", "testdata", "GameData-mirror", "GameData", "Squad", "Parts"))


def st(x):
    """Mono float store: round a double expression to float32."""
    return float(F(x))


def f32c(x):
    """a C# `f`-suffixed float literal widened to double (0.2f is 0.20000000298...)"""
    return float(F(x))


def fdiv(a, b):
    if b == 0.0:
        if a == 0.0 or a != a:
            return NAN
        neg = (math.copysign(1.0, a) < 0) != (math.copysign(1.0, b) < 0)
        return -INF if neg else INF
    return a / b


def fsqrt(x):
    return math.sqrt(x) if x >= 0.0 else NAN


PI32 = st(math.pi)                 # (float)Math.PI
DEG2RAD32 = st(PI32 / 180.0)       # (float)Math.PI / 180f folded by the C# compiler
EPS_MATHF = 1.401298464324817e-45  # Mathf.Epsilon


# ----------------------------------------------------------------------------------------------------------------------- Mathf
def m_sign(f):
    return 1.0 if f >= 0.0 else -1.0


def m_abs(f):
    return abs(f)


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


def m_lerp(a, b, t):
    return st(a + (b - a) * m_clamp01(t))


def m_inverse_lerp(a, b, v):
    if a != b:
        return m_clamp01(st(fdiv(v - a, b - a)))
    return 0.0


def m_max(a, b):
    return a if a > b else b


def m_min(a, b):
    return a if a < b else b


def m_pow(f, p):
    try:
        return st(math.pow(f, p))
    except (ValueError, OverflowError):
        return NAN


def m_round(f):
    return st(float(round(f)))  # Mathf.Round: (float)Math.Round(double), banker's rounding


def m_acos(f):
    return st(math.acos(f)) if -1.0 <= f <= 1.0 else NAN


def m_tan(f):
    return st(math.tan(f))


def m_move_towards(cur, tgt, maxd):
    if abs(st(tgt - cur)) <= maxd:
        return tgt
    return st(cur + m_sign(st(tgt - cur)) * maxd)


# ----------------------------------------------------------------------------------------------------------------------- Vector3 / Quaternion (managed)
V3Z = (0.0, 0.0, 0.0)


def v3_add(a, b):
    return (st(a[0] + b[0]), st(a[1] + b[1]), st(a[2] + b[2]))


def v3_sub(a, b):
    return (st(a[0] - b[0]), st(a[1] - b[1]), st(a[2] - b[2]))


def v3_mul(a, d):
    return (st(a[0] * d), st(a[1] * d), st(a[2] * d))


def v3_div(a, d):
    return (st(fdiv(a[0], d)), st(fdiv(a[1], d)), st(fdiv(a[2], d)))


def v3_neg(a):
    return (-a[0], -a[1], -a[2])


def v3_dot(a, b):
    return st(a[0] * b[0] + a[1] * b[1] + a[2] * b[2])


def v3_cross(l, r):
    return (st(l[1] * r[2] - l[2] * r[1]), st(l[2] * r[0] - l[0] * r[2]), st(l[0] * r[1] - l[1] * r[0]))


def v3_mag(v):
    return st(fsqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]))


def v3_sqrmag(v):
    return st(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])


def v3_normalized(v):
    n = v3_mag(v)
    if n > st(1e-5):
        return v3_div(v, n)
    return V3Z


def v3_project(v, n):
    num = v3_dot(n, n)
    if num < EPS_MATHF:
        return V3Z
    num2 = v3_dot(v, n)
    return (st(fdiv(n[0] * num2, num)), st(fdiv(n[1] * num2, num)), st(fdiv(n[2] * num2, num)))


def v3_project_on_plane(v, n):
    num = v3_dot(n, n)
    if num < EPS_MATHF:
        return v
    num2 = v3_dot(v, n)
    return (st(v[0] - fdiv(n[0] * num2, num)), st(v[1] - fdiv(n[1] * num2, num)), st(v[2] - fdiv(n[2] * num2, num)))


def v3_approx_eq(l, r):
    n = st(l[0] - r[0])
    n2 = st(l[1] - r[1])
    n3 = st(l[2] - r[2])
    return st(n * n + n2 * n2 + n3 * n3) < st(9.9999994e-11)


def quat_mul_vec(q, p):
    x, y, z, w = q
    n1 = st(x * 2.0)
    n2 = st(y * 2.0)
    n3 = st(z * 2.0)
    n4 = st(x * n1)
    n5 = st(y * n2)
    n6 = st(z * n3)
    n7 = st(x * n2)
    n8 = st(x * n3)
    n9 = st(y * n3)
    n10 = st(w * n1)
    n11 = st(w * n2)
    n12 = st(w * n3)
    return (st((1.0 - (n5 + n6)) * p[0] + (n7 - n12) * p[1] + (n8 + n11) * p[2]),
            st((n7 + n12) * p[0] + (1.0 - (n4 + n6)) * p[1] + (n9 - n10) * p[2]),
            st((n8 - n11) * p[0] + (n9 + n10) * p[1] + (1.0 - (n4 + n5)) * p[2]))


def quat_mul(l, r):
    lx, ly, lz, lw = l
    rx, ry, rz, rw = r
    return (st(lw * rx + lx * rw + ly * rz - lz * ry), st(lw * ry + ly * rw + lz * rx - lx * rz),
            st(lw * rz + lz * rw + lx * ry - ly * rx), st(lw * rw - lx * rx - ly * ry - lz * rz))


def quat_conj(q):  # Quaternion.Inverse (native; a unit quaternion's conjugate)
    return (-q[0], -q[1], -q[2], q[3])


def n_rotate(q, v):
    """Native RotateVectorByQuat (Runtime/Math/Quaternion.h): float32 per operation (as tools/wp29a/vpp_reference.py)."""
    qx, qy, qz, qw = F(q[0]), F(q[1]), F(q[2]), F(q[3])
    two = F(2.0)
    x, y, z = qx * two, qy * two, qz * two
    xx, yy, zz = qx * x, qy * y, qz * z
    xy, xz, yz = qx * y, qx * z, qy * z
    wx, wy, wz = qw * x, qw * y, qw * z
    one = F(1.0)
    vx, vy, vz = F(v[0]), F(v[1]), F(v[2])
    rx = (one - (yy + zz)) * vx + (xy - wz) * vy + (xz + wy) * vz
    ry = (xy + wz) * vx + (one - (xx + zz)) * vy + (yz - wx) * vz
    rz = (xz - wy) * vx + (yz + wx) * vy + (one - (xx + yy)) * vz
    return (float(rx), float(ry), float(rz))


def n_right(q):
    return n_rotate(q, (1.0, 0.0, 0.0))


def n_forward(q):
    return n_rotate(q, (0.0, 0.0, 1.0))


def n_angle_axis(angle, axis):
    """Quaternion.AngleAxis (native AxisAngleToQuaternionSafe) in the formulation used by tests/test_wheels.cpp RefAngleAxis (cosf / sinf of the
    half angle taken from the correctly rounded double); the C++ test rebinds NativeOps::AngleAxis to the same formulation."""
    t = F(F(angle) / F(360.0))
    t = F(t * F(2.0))
    rad = F(float(t) * math.pi)
    ax, ay, az = F(axis[0]), F(axis[1]), F(axis[2])
    s2 = F(F(ax * ax + ay * ay) + az * az)
    mag = F(math.sqrt(float(s2)))
    if mag > F(1e-6):
        half = F(rad * F(0.5))
        w = F(math.cos(float(half)))
        sn = F(F(math.sin(float(half))) / mag)
        return (float(F(sn * ax)), float(F(sn * ay)), float(F(sn * az)), float(w))
    return (0.0, 0.0, 0.0, 1.0)


def rand_quat(rng):
    q = [rng.uniform(-1, 1) for _ in range(4)]
    n = math.sqrt(sum(c * c for c in q))
    return tuple(st(c / n) for c in q)


# PartTransform (core/include/ksp/parts/part.hpp): scale 1, TransformPoint = rotation * Scale(1, p) + position, TransformDirection = rotation * d
class Tr:
    def __init__(self, pos=V3Z, rot=(0.0, 0.0, 0.0, 1.0)):
        self.pos = pos
        self.rot = rot

    def up(self):
        return quat_mul_vec(self.rot, (0.0, 1.0, 0.0))

    def forward(self):
        return quat_mul_vec(self.rot, (0.0, 0.0, 1.0))

    def transform_point(self, p):
        return v3_add(quat_mul_vec(self.rot, (st(1.0 * p[0]), st(1.0 * p[1]), st(1.0 * p[2]))), self.pos)

    def inverse_transform_point(self, p):
        r = quat_mul_vec(quat_conj(self.rot), v3_sub(p, self.pos))
        return (st(r[0] / 1.0), st(r[1] / 1.0), st(r[2] / 1.0))

    def transform_direction(self, d):
        return quat_mul_vec(self.rot, d)


# ----------------------------------------------------------------------------------------------------------------------- Unity Random (C2)
class URandom:
    def __init__(self, seed):
        self.s0 = seed & 0xFFFFFFFF
        self.s1 = (self.s0 * 1812433253 + 1) & 0xFFFFFFFF
        self.s2 = (self.s1 * 1812433253 + 1) & 0xFFFFFFFF
        self.s3 = (self.s2 * 1812433253 + 1) & 0xFFFFFFFF

    def next_uint(self):
        t = self.s0 ^ ((self.s0 << 11) & 0xFFFFFFFF)
        t ^= t >> 8
        self.s0, self.s1, self.s2 = self.s1, self.s2, self.s3
        self.s3 = (self.s3 ^ (self.s3 >> 19) ^ t) & 0xFFFFFFFF
        return self.s3

    def value(self):
        return float(F(self.next_uint() & 0x7FFFFF) / F(8388607.0))

    def range(self, mn, mx):  # Random.Range(float, float), native per-operation float32
        v = F(self.value())
        return float(F(F(v * F(mn)) + F(F(F(1.0) - v) * F(mx))))


# ----------------------------------------------------------------------------------------------------------------------- AnimationCurve / FloatCurve
class Curve:
    """Unity AnimationCurve (cubic Hermite per segment, clamp-forever) as FloatCurve.Load builds it: a 4-value key is AnimationCurve.AddKey(Keyframe)
    (tangents as given), a 2-value key is AddKey(time, value) = AddKeySmoothTangents, which re-derives the slope of the new key and of both
    neighbours (RecalculateSplineSlope with b = 0). Both are NATIVE Unity code with no C# source: the smoothing is transcribed from the capture
    verified WP-01 port (core/src/unity/animation_curve.cpp, RVA 0x750780 / 0x74FDB0) -- the one place this reference is not independent."""

    def __init__(self, entries):
        self.keys = []  # [time, value, inTangent, outTangent] as float32
        self.min_time = FLT_MAX
        self.max_time = -FLT_MAX
        for e in entries:
            t, v, it, ot, explicit = e
            idx = self._insert([F(t), F(v), F(it), F(ot)])
            if not explicit and idx >= 0:
                if idx > 0:
                    self._smooth(idx - 1)
                self._smooth(idx)
                if idx + 1 < len(self.keys):
                    self._smooth(idx + 1)
            self.min_time = min(self.min_time, float(F(t)))
            self.max_time = max(self.max_time, float(F(t)))

    def _insert(self, k):
        first = 0
        n = len(self.keys)
        while n > 0:
            half = n >> 1
            if k[0] > self.keys[first + half][0]:
                first = first + half + 1
                n = n - half - 1
            else:
                n = half
        if first != len(self.keys) and not (self.keys[first][0] > k[0]):
            return -1
        self.keys.insert(first, k)
        return first

    def _smooth(self, i):
        K = self.keys
        n = len(K)
        if n < 2:
            return
        if i == 0:
            m = (K[1][1] - K[0][1]) / (K[1][0] - K[0][0])
            K[0][2] = m
            K[0][3] = m
            return
        if i == n - 1:
            m = (K[i][1] - K[i - 1][1]) / (K[i][0] - K[i - 1][0])
            K[i][2] = m
            K[i][3] = m
            return
        t0, v0 = K[i][0], K[i][1]
        dx1 = t0 - K[i - 1][0]
        dx2 = K[i + 1][0] - t0
        dy2 = K[i + 1][1] - v0
        if abs(dx1) > F(1e-5):
            m1 = (v0 - K[i - 1][1]) / dx1
        else:
            m1 = F(0.0)
        if abs(dx2) > F(1e-5):
            m2 = dy2 / dx2
        else:
            m2 = F(0.0)
        wR = F(F(1.0) - F(0.0))
        wL = F(F(0.0) + F(1.0))
        wR = wR * F(0.5)
        wL = wL * F(0.5)
        wR = wR * m2
        wL = wL * m1
        slope = wR + wL
        K[i][2] = slope
        K[i][3] = slope

    def defined(self):
        return len(self.keys) > 0

    def evaluate(self, x):
        """AnimationCurve.Evaluate (native, clamp-forever): the cubic of the segment in polynomial form (CalculateCacheData / poly of the capture
        verified WP-01 port), float32 per operation."""
        k = self.keys
        if not k:
            return 0.0
        if len(k) == 1:
            return float(k[0][1])
        x = F(x)
        if x >= k[-1][0]:
            return float(k[-1][1])
        if k[0][0] > x:
            return float(k[0][1])
        i = 0
        while not (x < k[i + 1][0]):
            i += 1
        L, R = k[i], k[i + 1]
        dx = F(R[0] - L[0])
        dy = F(R[1] - L[1])
        length = F(1e-4) if F(1e-4) > dx else dx
        m1 = L[3]
        d1 = F(length * R[2])
        d0 = F(length * m1)
        length2 = F(length * length)
        inv = F(F(1.0) / length2)
        c = m1
        a = F(d1 + d0)
        a = F(a - dy)
        a = F(a - dy)
        a = F(a * inv)
        a = F(a / length)
        b = F(dy + dy)
        b = F(b + dy)
        b = F(b - d0)
        b = F(b - d0)
        b = F(b - d1)
        b = F(b * inv)
        t = F(x - L[0])
        r = F(t * a)
        r = F(r + b)
        r = F(r * t)
        r = F(r + c)
        r = F(r * t)
        r = F(r + L[1])
        return float(r)

    def find_min_max(self):
        """FloatCurve.FindMinMaxValue(out min, out max): 100 samples."""
        if not self.keys:
            return 0.0, 0.0
        mn, mx = FLT_MAX, -FLT_MAX
        lo, hi = FLT_MAX, -FLT_MAX
        for k in self.keys:
            if float(k[0]) < lo:
                lo = float(k[0])
            if float(k[0]) > hi:
                hi = float(k[0])
        step = st((hi - lo) / 100.0)
        for j in range(100):
            v = self.evaluate(st(lo + float(j) * step))
            if v < mn:
                mn = v
            if v > mx:
                mx = v
        return mn, mx


# ----------------------------------------------------------------------------------------------------------------------- cfg parsing (PartLoader's ConfigNode subset)
class Node:
    def __init__(self, name):
        self.name = name
        self.values = []  # (key, value)
        self.nodes = []

    def get(self, key, default=None):
        r = default
        for k, v in self.values:
            if k == key:
                r = v
                break
        return r

    def get_all(self, key):
        return [v for k, v in self.values if k == key]

    def node(self, name):
        for n in self.nodes:
            if n.name == name:
                return n
        return None


def parse_cfg(text):
    root = Node("root")
    stack = [root]
    last = None
    for raw in text.splitlines():
        line = raw.split("//")[0].strip()
        if not line:
            continue
        if line == "{":
            n = Node(last or "")
            stack[-1].nodes.append(n)
            stack.append(n)
            last = None
            continue
        if line == "}":
            stack.pop()
            continue
        if "=" in line:
            k, v = line.split("=", 1)
            stack[-1].values.append((k.strip(), v.strip()))
            last = None
        else:
            last = line
    return root


def f32p(s):  # float.Parse
    return st(float(s))


def parse_bool(s):
    return s.strip().lower() == "true"


def parse_v3(s):
    p = [x for x in s.replace(",", " ").split() if x]
    return (f32p(p[0]), f32p(p[1]), f32p(p[2]))


def curve_of(node):
    if node is None:
        return Curve([])
    keys = []
    for v in node.get_all("key"):
        p = [x for x in v.replace(",", " ").replace(";", " ").split() if x]
        if len(p) == 4:
            keys.append((f32p(p[0]), f32p(p[1]), f32p(p[2]), f32p(p[3]), True))
        else:
            keys.append((f32p(p[0]), f32p(p[1]), 0.0, 0.0, False))
    return Curve(keys)


def load_parts():
    out = {}
    for dp, _, fns in os.walk(MIRROR):
        for fn in sorted(fns):
            if not fn.endswith(".cfg"):
                continue
            with open(os.path.join(dp, fn), encoding="utf-8-sig") as fh:
                txt = fh.read()
            if "ModuleWheelBase" not in txt:
                continue
            root = parse_cfg(txt)
            part = root.node("PART")
            name = part.get("name").replace("_", ".")
            mods = [n for n in part.nodes if n.name == "MODULE"]
            out[name] = (part, mods)
    return out


# ----------------------------------------------------------------------------------------------------------------------- enums
WT_FREE, WT_MOTORIZED, WT_LEG = 0, 1, 2
WHEEL_TYPE = {"FREE": 0, "MOTORIZED": 1, "LEG": 2}
SS_NONE, SS_TIRE, SS_SUSP, SS_STEER, SS_MOTOR, SS_BRAKES, SS_WCOL, SS_BOGEY, SS_ALL = 0, 1, 2, 4, 8, 16, 32, 64, -1
ACT_ACTIVATE, ACT_DEACTIVATE, ACT_TOGGLE = 0, 1, 2
MS_INOPERABLE, MS_NOTENOUGH, MS_DISABLED, MS_IDLE, MS_RUNNING = -2, -1, 0, 1, 2
ENV = {  # GameSettings defaults (GameSettings.cs:1028-1067) and the other globals the modules read
    "PART_REPAIR_MASS_PER_KIT": st(0.05), "PART_REPAIR_MAX_KIT_AMOUNT": 4, "WHEEL_WEIGHT_STRESS_MULTIPLIER": 1.0, "WHEEL_SLIP_STRESS_MULTIPLIER": 1.0,
    "WHEEL_SUBSTEPS_ACTIVE": 8, "WHEEL_SUBSTEPS_INACTIVE": 4, "WHEEL_AUTO_SPRINGDAMPER": True, "WHEEL_AUTO_STEERINGADJUST": True,
    "WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED": True, "WHEEL_DAMAGE_WHEELCOLLIDER_ENABLED": True, "GRAV_ACCEL": 9.80665, "MAX_SURFACE_GEE": 1.67,
}


# ----------------------------------------------------------------------------------------------------------------------- scheduler / animation / FSM
class Sched:
    """Unity coroutine model as documented in core/include/ksp/core/coroutine.hpp (null: first dynamic pass of a later frame; WaitForSeconds:
    first dynamic pass with curTime >= wake); generators yield ("null",) or ("secs", s)."""

    def __init__(self):
        self.cur = 0.0
        self.frame = 0
        self.queue = []

    def start(self, gen):
        task = {"gen": gen, "wait": None, "yf": -1, "wake": 0.0, "dead": False}
        self._step(task)
        return task

    def _step(self, task):
        try:
            y = next(task["gen"])
        except StopIteration:
            task["dead"] = True
            return
        task["wait"] = y
        task["yf"] = self.frame
        task["wake"] = self.cur + (float(F(y[1])) if y[0] == "secs" else 0.0)
        self.queue.append(task)

    def advance_frame(self, dt):
        self.cur += float(F(dt))
        self.frame += 1

    def dynamic_pass(self):
        batch = list(self.queue)
        for t in batch:
            if t["dead"] or t not in self.queue:
                continue
            w = t["wait"]
            due = (self.frame > t["yf"]) if w[0] == "null" else (self.cur >= t["wake"])
            if not due:
                continue
            self.queue.remove(t)
            self._step(t)

    def stop_all(self):
        for t in self.queue:
            t["dead"] = True
        self.queue = []


def co_delayed_frames(n, cb):
    for _ in range(n):
        yield ("null",)
    cb()


def co_delayed_secs(s, cb):
    yield ("secs", s)
    cb()


def co_wait_until(cond, cb):
    while not cond():
        yield ("null",)
    cb()


class AnimState:
    def __init__(self, length):
        self.length = F(length)
        self.time = F(0.0)
        self.speed = F(1.0)
        self.enabled = False
        self.weight = F(0.0)
        self.wrap = "clamp"

    @property
    def normalized_time(self):
        return float(F(self.time / self.length)) if self.length > 0 else 0.0

    @normalized_time.setter
    def normalized_time(self, v):
        self.time = F(F(v) * self.length)

    def tick(self, dt):
        if not self.enabled:
            return
        self.time = F(self.time + F(self.speed * F(dt)))
        if self.time > self.length:
            self.time = self.length
        if self.time < F(0.0):
            self.time = F(0.0)


class FState:
    def __init__(self, name):
        self.name = name
        self.on_enter = lambda prev: None
        self.on_update = lambda: None
        self.on_fixed = lambda: None
        self.on_late = lambda: None
        self.on_leave = lambda nxt: None
        self.events = []


class FEvent:
    def __init__(self, name, mode):
        self.name = name
        self.mode = mode
        self.goto = None
        self.on_event = lambda: None
        self.cond = lambda st_: False


class FSM:
    """KerbalFSM.cs, written from the C#."""

    def __init__(self):
        self.states = []
        self.cur = None
        self.started = False
        self.cur_name = ""
        self.on_change = []

    def add_state(self, s):
        self.states.append(s)

    def add_event(self, ev, *to):
        for s in reversed(to):
            assert s in self.states
            s.events.append(ev)

    def add_event_excluding(self, ev, *exc):
        for s in reversed(self.states):
            if s not in exc:
                s.events.append(ev)

    def run_event(self, ev):
        assert self.started
        if ev not in self.cur.events:
            return
        ev.on_event()
        nxt = ev.goto
        self.cur_name = nxt.name
        self.cur.on_leave(nxt)
        last = self.cur
        self.cur = nxt
        for cb in self.on_change:
            cb(last, nxt, ev)
        self.cur.on_enter(last)

    def start(self, name):
        s = [x for x in self.states if x.name == name][0]
        self.cur = s
        s.on_enter(None)
        self.cur_name = s.name
        self.started = True

    def _upd(self, mode):
        i = 0
        while i < len(self.cur.events):
            ev = self.cur.events[i]
            if ev.mode == mode and ev.cond(self.cur):
                self.run_event(ev)
            i += 1

    def update(self):
        if self.started:
            self.cur.on_update()
            self._upd("update")

    def fixed(self):
        if self.started:
            self.cur.on_fixed()
            self._upd("fixed")

    def late(self):
        if self.started:
            self.cur.on_late()
            self._upd("late")


# ----------------------------------------------------------------------------------------------------------------------- the world
class Col:
    """A WheelCollider component of the wheel (VPWheelCollider managed fields)."""

    def __init__(self):
        self.springRate = st(30000.0)
        self.damperRate = st(1500.0)
        self.suspensionDistance = st(0.25)
        self.suspensionAnchor = st(0.5)
        self.suspensionOffset = 0.0
        self.radius = st(0.3)
        self.mass = st(20.0)
        self.groundPenetration = st(0.02)
        self.springSlerpRate = st(0.02)
        self.updateSuspension = False
        self.updateCaliper = False
        self.updateWheel = False
        self.center = V3Z
        self.layerMask = -1


class WState:
    def __init__(self):
        self.grounded = False
        self.angularVelocity = 0.0
        self.lwv = (0.0, 0.0)
        self.compression = 0.0
        self.sidewaysSlip = 0.0
        self.normal = V3Z
        self.downforce = 0.0
        self.contactDepth = 0.0
        self.collider = 0  # scripted collider kind (0 = none)


class Wheel:
    """KSPWheelController as the modules see it."""

    def __init__(self):
        self.maxDriveTorque = 0.0
        self.maxBrakeTorque = 0.0
        self.maxSteerAngle = 0.0
        self.maxRpm = FLT_MAX
        self.wheelDamping = st(0.95)
        self.driveInput = self.brakeInput = self.steerInput = 0.0
        self.steeringResponse = self.driveResponse = self.brakeResponse = 0.0
        self.minimumdownForce = 0.0
        self.integrationSteps = 4
        self.tireForceSharpness = st(5.0)
        self.suspensionForceSharpness = st(5.0)
        self.tireForceSmoothing = True
        self.suspensionForceSmoothing = True
        self.tireSideDeflection = False
        self.tireSideDeflectionRate = st(10.0)
        self.model = "smooth"
        self.adherent = (st(0.5), st(0.95))
        self.peak = (st(1.5), st(1.1))
        self.limit = (4.0, st(0.8))
        self.col = Col()
        self.state = WState()
        self.enabled = False
        self.init = False
        self.gravity = V3Z
        self.effSpring = 0.0
        self.tgtRb = "own"

    @property
    def is_grounded(self):
        return self.init and self.state.grounded

    def set_grounded(self, v):
        if self.init:
            self.state.grounded = v

    def set_enabled(self, v):
        if v == self.enabled:
            return
        if v:
            # VehicleBase.OnEnable: OnInitialize (fresh WheelState), radius / mass checks
            self.state = WState()
            self.init = True
            if self.col.radius < f32c(0.01) or not (self.col.mass >= f32c(0.01)):
                return
            self.enabled = True
        else:
            self.enabled = False

    def initialize(self):
        if not self.init:
            self.state = WState()
            self.init = True

    def legacy_wheel_load(self):
        if not self.state.grounded:
            return 0.0
        return m_max(0.0, st(self.state.contactDepth * self.effSpring * (1.0 / self.gravity_mag())))

    def gravity_mag(self):
        return v3_mag(self.gravity)


class Subsys:
    def __init__(self, reason, typ, owner):
        self.reason = reason
        self.type = typ
        self.owner = owner


class Subsystems:
    def __init__(self):
        self.subs = []
        self.types = 0
        self.on_modified = [lambda s: None]

    def _fire(self):
        for cb in list(self.on_modified):
            cb(self)

    def add(self, d):
        if d not in self.subs:
            self.subs.append(d)
        self.types |= d.type
        self._fire()

    def remove(self, d):
        if d in self.subs:
            self.subs.remove(d)
        self.types = 0
        for s in self.subs:
            self.types |= s.type
        self._fire()

    def has(self, t):
        return (self.types & t) != 0

    def contains(self, d):
        return d in self.subs


class World:
    """Everything the modules reach through the part and the vessel."""

    def __init__(self, gee, rate, seed):
        self.gee = gee                  # mainBody.GeeASL (double)
        self.gravMult = 1.0
        self.rate = rate                # physics warp rate: TimeWarp.deltaTime = Time.deltaTime * rate
        self.dt = st(0.02)              # Time.deltaTime
        self.fdt = st(0.02)             # Time.fixedDeltaTime
        self.rand = URandom(seed)
        self.sched = Sched()
        self.trace = []
        self.ec = 0.0
        self.totalMass = 0.0
        self.ctrl = (0.0, 0.0, 0.0, 0.0)  # wheelThrottle, wheelThrottleTrim, wheelSteer, wheelSteerTrim
        self.refTr = Tr()
        self.CoM = V3Z
        self.upAxis = (0.0, 1.0, 0.0)
        self.gravityForPos = (0.0, -9.81, 0.0)
        self.landed = False
        self.srfSpeed = 0.0
        self.groundContacts = []
        self.parts = []
        self.brakesGroup = False
        self.timeSinceLevelLoad = 10.0
        self.active_vessel = True
        self.steeringRadiusValue = 1.0   # VesselValues.SteeringRadius.value (cached per frame)
        self.frame = 0

    def tw_delta(self):
        return st(self.dt * self.rate)

    def tw_fixed(self):
        return st(self.fdt * self.rate)


# ----------------------------------------------------------------------------------------------------------------------- modules
class PartM:
    def __init__(self, world, name, part_node, mod_nodes, rng):
        self.w = world
        self.name = name
        self.rescale = f32p(part_node.get("rescaleFactor", "1.25"))
        self.mass = f32p(part_node.get("mass", "0"))
        self.tr = Tr()
        self.rb_vel = V3Z
        self.started = True
        self.packed = False
        self.dead = False
        self.groundContact = False
        self.mods = []
        self.base = None
        for n in mod_nodes:
            nm = n.get("name")
            if nm == "ModuleWheelBase":
                m = Base(self, n)
                self.base = m
            elif nm in ("ModuleWheelSuspension", "ModuleWheelSteering", "ModuleWheelMotor", "ModuleWheelMotorSteering", "ModuleWheelBrakes",
                        "ModuleWheelLock", "ModuleWheelDamage", "ModuleWheelDeployment", "ModuleWheelBogey"):
                m = {"ModuleWheelSuspension": Suspension, "ModuleWheelSteering": Steering, "ModuleWheelMotor": Motor,
                     "ModuleWheelMotorSteering": MotorSteering, "ModuleWheelBrakes": Brakes, "ModuleWheelLock": Lock,
                     "ModuleWheelDamage": Damage, "ModuleWheelDeployment": Deployment, "ModuleWheelBogey": Bogey}[nm](self, n)
            else:
                m = None
            self.mods.append(m)
            if m is not None and m is not self.base:
                m.on_awake()
            elif m is self.base:
                m.on_awake()

    def find(self, cls):
        for m in self.mods:
            if isinstance(m, cls):
                return m
        return None


class Base:
    def __init__(self, part, n):
        self.part = part
        self.w = part.w
        g = lambda k, d: n.get(k, d)
        self.wheelType = WHEEL_TYPE[g("wheelType", "FREE")]
        self.isGrounded = False
        self.FitWheelColliderToMesh = parse_bool(g("FitWheelColliderToMesh", "False"))
        self.radius = f32p(g("radius", "0"))
        self.center = parse_v3(g("center", "0,0,0"))
        self.mass = f32p(g("mass", "0"))
        self.frictionSharpness = f32p(g("frictionSharpness", "0"))
        self.wheelDamping = f32p(g("wheelDamping", "0.05"))
        self.wheelMaxSpeed = f32p(g("wheelMaxSpeed", "1000"))
        self.adherentStart = f32p(g("adherentStart", "0.5"))
        self.frictionAdherent = f32p(g("frictionAdherent", "0.25"))
        self.peakStart = f32p(g("peakStart", "4"))
        self.frictionPeak = f32p(g("frictionPeak", "1.45"))
        self.limitStart = f32p(g("limitStart", "7"))
        self.frictionLimit = f32p(g("frictionLimit", "1.1"))
        self.autoFrictionAvailable = parse_bool(g("autoFrictionAvailable", "True"))
        self.autoFriction = parse_bool(g("autoFriction", "True"))
        self.frictionMultiplier = f32p(g("frictionMultiplier", "1"))
        self.geeBias = f32p(g("geeBias", "1.6"))
        self.groundHeightOffset = f32p(g("groundHeightOffset", "0"))
        self.inactiveSubsteps = int(g("inactiveSubsteps", "-1"))
        self.activeSubsteps = int(g("activeSubsteps", "-1"))
        self.tireForceSharpness = f32p(g("tireForceSharpness", "10"))
        self.suspensionForceSharpness = f32p(g("suspensionForceSharpness", "10"))
        self.ApplyForcesToParent = parse_bool(g("ApplyForcesToParent", "False"))
        self.springSlerpRate = f32p(g("springSlerpRate", "0.02"))
        self.minimumDownforce = f32p(g("minimumDownforce", "0.5"))
        self.useNewFrictionModel = parse_bool(g("useNewFrictionModel", "False"))
        self.hasWheelTransform = g("wheelTransformName", "") != ""
        self.clipObject = g("clipObject", "")
        self.suspensionEnabled = True
        self.subs = []
        self.setup = False
        self.slip = (0.0, 0.0)
        self.inop = Subsystems()
        self.inopOnRails = Subsys("Part is on-rails", SS_WCOL, self)
        self.inopSusp = Subsys("Suspension is disabled", SS_SUSP, self)
        self.wheel = None
        self.rbBrakeConstraints = False
        self.schloompaTime = 2.0
        self.kd = 100.0
        self.ki = st(0.1)
        self.driftCorrection = False
        self.dstate = "Acquire"
        self.fixFwd = V3Z
        self.error = V3Z
        self.errorLast = V3Z
        self.acquireMaxSpeed = st(0.1)
        self.gCollider = 0
        self.gColliderPrev = 0
        self.vSrfContact = None
        self.wheelOrgPosR = V3Z
        self.wheelOrgRotR = (0.0, 0.0, 0.0, 1.0)
        self.wheelTrPos = V3Z
        self.wheelTrRot = (0.0, 0.0, 0.0, 1.0)
        self.damage = None
        self.brakes = None
        self.lock = None
        self.parentRb = False
        self.setup_started = False

    # ---- lifecycle
    def on_awake(self):
        self.subs = []
        self.inop = Subsystems()

    def on_start(self):
        w = self.w
        self.suspensionEnabled = True
        # (the hierarchy lookups of wheelTransformName / wheelColliderTransformName succeed in the scripted world)
        self.inopOnRails = Subsys("Part is on-rails", SS_WCOL, self)
        self.inopSusp = Subsys("Suspension is disabled", SS_SUSP, self)
        self.action_ui_update()
        if self.activeSubsteps < 0:
            self.activeSubsteps = ENV["WHEEL_SUBSTEPS_ACTIVE"]
        if self.inactiveSubsteps < 0:
            self.inactiveSubsteps = ENV["WHEEL_SUBSTEPS_INACTIVE"]
        self.damage = self.part.find(Damage)
        self.brakes = self.part.find(Brakes)
        self.lock = self.part.find(Lock)
        if self.lock is not None and self.wheelType == WT_LEG and w.gee > 1.100000023841858:
            self.ki = 1000.0
        if self.wheelType == WT_LEG:
            self.minimumDownforce = 10.0
        w.sched.start(co_wait_until(lambda: self.part.started and not self.part.packed, self.wheel_setup))

    def register(self, m):
        if m not in self.subs:
            self.subs.append(m)
        if self.setup:
            m.on_wheel_init(self.wheel)

    def wheel_setup(self):
        w = self.w
        p = self.part
        wh = Wheel()
        self.wheel = wh
        wh.gravity = w.gravityForPos
        wh.minimumdownForce = self.minimumDownforce
        wh.col.springSlerpRate = self.springSlerpRate
        # (the model transform exists: wcTransform re-parent and localScale are transform bookkeeping)
        wh.col.layerMask = 8421377  # ~(~LayerUtil.DefaultEquivalent | 0x20000) | 0x8000
        wh.col.mass = self.mass
        self.check_substeps()
        wh.col.suspensionDistance = st(0.02)
        wh.col.suspensionAnchor = st(0.5)
        wh.col.springRate = st(10000.0)
        wh.col.damperRate = st(1.0)
        wh.col.groundPenetration = st(0.02)
        wh.tireForceSmoothing = True
        wh.tireForceSharpness = self.tireForceSharpness
        wh.suspensionForceSmoothing = True
        wh.suspensionForceSharpness = self.suspensionForceSharpness
        if self.frictionSharpness > 0.0:
            wh.tireSideDeflection = True
            wh.tireSideDeflectionRate = self.frictionSharpness
        wh.wheelDamping = st(1.0 - self.wheelDamping)
        wh.maxRpm = speed_to_rpm(self.wheelMaxSpeed, self.radius)
        if not (self.FitWheelColliderToMesh and self.hasWheelTransform):
            wh.col.radius = st(self.radius * p.rescale)
            wh.col.center = v3_mul(self.center, p.rescale)
        # WheelOrgPosR / RotR from the scripted wheel transform (the wheel transform is the collider host when wheelTransformName is empty)
        self.wheelOrgPosR = p.tr.inverse_transform_point(self.wheelTrPos)
        self.wheelOrgRotR = quat_mul(quat_conj(p.tr.rot), self.wheelTrRot)
        self.inop.on_modified = [self.on_subsystems_modified]
        for m in reversed(self.subs):
            m.on_wheel_init(wh)
        if True:
            self.apply_gee_bias(st(w.gee))
        wh.model = "lineal"  # TireFriction.model = Lineal (the curve itself is never rebuilt: ModuleWheelBase.cs:361)
        self.setup = True
        self.inop_update(True)
        if not wh.enabled:
            wh.initialize()
        if self.ApplyForcesToParent and self.parentRb:
            wh.tgtRb = "parent"

    def check_substeps(self):
        if self.wheel is not None:
            self.wheel.integrationSteps = self.activeSubsteps if self.w.active_vessel else self.inactiveSubsteps

    def apply_gee_bias(self, gee):
        if self.autoFriction:
            if self.useNewFrictionModel:
                self.frictionMultiplier = m_clamp(st(gee * self.geeBias + f32c(0.4)), f32c(0.4), 10.0)
            else:
                self.minimumDownforce = 10.0
                if self.geeBias == st(1.6):
                    self.geeBias = 1.0
                self.frictionMultiplier = m_clamp(st(1.0 / gee * self.geeBias + 1.0), 0.5, 10.0)
        if self.wheel is not None:
            wh = self.wheel
            wh.adherent = (self.adherentStart, wh.adherent[1])
            wh.peak = (self.peakStart, wh.peak[1])
            wh.limit = (self.limitStart, wh.limit[1])
            self.update_friction()
        return self.frictionMultiplier

    def update_friction(self):
        wh = self.wheel
        wh.adherent = (wh.adherent[0], st(self.frictionAdherent * self.frictionMultiplier))
        wh.peak = (wh.peak[0], st(self.frictionPeak * self.frictionMultiplier))
        wh.limit = (wh.limit[0], st(self.frictionLimit * self.frictionMultiplier))

    # ---- FixedUpdate
    def fixed_update(self):
        w = self.w
        p = self.part
        wh = self.wheel
        if not self.setup or p.packed or p.dead:
            return
        self.check_suspension_toggle()
        self.check_substeps()
        s = wh.state
        sx = s.lwv[0]
        sy = st(s.angularVelocity * self.radius - s.lwv[1])
        self.slip = (st(sx * w.tw_delta()), st(sy * w.tw_delta()))
        if self.damage is not None and wh.is_grounded and not self.isGrounded:
            self.damage.reset_immunity()
            if ENV["WHEEL_DAMAGE_WHEELCOLLIDER_ENABLED"]:
                vel = p.rb_vel
                kind = s.collider
                other = COLLIDER_KINDS[kind]
                if other["rbVel"] is not None:
                    vel = v3_sub(vel, other["rbVel"])
                self.damage.handle_collision_velocity(kind, vel, s.normal)
        self.isGrounded = wh.is_grounded
        if not self.autoFriction:
            self.update_friction()
        wh.gravity = w.gravityForPos
        self.landing_detection_update()
        wh.maxRpm = speed_to_rpm(self.wheelMaxSpeed, self.radius)
        wh.wheelDamping = st(1.0 - self.wheelDamping)
        if self.driftCorrection:
            self.update_drift_fix()
        if self.wheelType != WT_LEG:
            self.schloompha()

    def landing_detection_update(self):
        w = self.w
        wh = self.wheel
        s = wh.state
        if not self.landed_needs_update(s.collider, self.gColliderPrev, self.vSrfContact, wh.is_grounded):
            return
        self.gColliderPrev = self.gCollider
        self.gCollider = s.collider
        p = self.part
        p.groundContact = False
        if p in w.groundContacts:
            w.groundContacts.remove(p)
        self.vSrfContact = None
        w.trace.append("crash:null")
        if wh.is_grounded:
            if self.gCollider == 0:
                raise RuntimeError("InvalidOperationException")
            k = COLLIDER_KINDS[self.gCollider]
            if k["layer"] == 15:
                if k["tag"] != "":
                    landed_at = k["tag"]
                    if k["tag"] != "Untagged":
                        if k["tag"] == "ROC" and k.get("roc") is not None:
                            landed_at = k["roc"]
                        w.trace.append("SetLandedAt:%s|%s|" % (landed_at, k["tag"]))
                    else:
                        landed_at = ""
                        w.trace.append("SetLandedAt:|-|")
                else:
                    landed_at = ""
                    w.trace.append("SetLandedAt:|-|")
                p.groundContact = True
                if p not in w.groundContacts:
                    w.groundContacts.append(p)
                if k.get("crash"):
                    w.trace.append("crash:set")
            elif k["layer"] == 0:
                other = k.get("otherVessel")
                if other is not None and other["landedOrSplashed"]:
                    p.groundContact = True
                    if p not in w.groundContacts:
                        w.groundContacts.append(p)
                    self.vSrfContact = other
                    w.trace.append("SetLandedAt:%s|-|%s" % (other["landedAt"], other["display"]))
        else:
            w.trace.append("ResetCollisions")
        w.trace.append("checkLanded")
        # Vessel.checkLanded (Vessel.cs:1015): Landed = any part with a ground contact; no contact clears landedAt
        w.landed = any(q.groundContact for q in w.parts)
        if not w.landed:
            w.trace.append("SetLandedAt:|-|")

    def landed_needs_update(self, hit, prev, vcontact, grounded):
        w = self.w
        if grounded and not w.landed:
            return True
        if prev != hit:
            return True
        if hit != 0 and not grounded:
            return True
        if hit == 0 and grounded:
            return True
        if vcontact is not None and not vcontact["landed"]:
            return True
        return False

    def schloompha(self):
        w = self.w
        wh = self.wheel
        if not (w.timeSinceLevelLoad > 3.0) or not w.vessel_loaded or not w.landed or not self.isGrounded or (self.brakes is None and self.lock is None):
            return
        if (self.brakes is not None and self.brakes.brakeInput >= 1.0) or (self.lock is not None and wh.brakeInput >= 1.0):
            lim = (w.srfSpeed < 0.10000000149011612) if self.wheelType == WT_LEG else (w.srfSpeed < 9.999999747378752e-06)
            if lim and not self.rbBrakeConstraints:
                self.toggle_rb_constraints(True)
                w.sched.start(co_delayed_secs(self.schloompaTime, lambda: self.toggle_rb_constraints(False)))
                self.rbBrakeConstraints = True
        else:
            self.rbBrakeConstraints = False

    def toggle_rb_constraints(self, freeze):
        w = self.w
        if freeze:
            w.trace.append("lock:set")
            self.hold_wheel_damage(st(self.schloompaTime * 1.5))
            w.trace.append("ignoreColliders")
            w.trace.append("rb:FreezeAll")
        else:
            w.trace.append("rb:None")
            w.trace.append("lock:remove")

    def hold_wheel_damage(self, seconds=3.0):
        if self.damage is not None:
            self.damage.startupTime = seconds
        self.w.trace.append("hold:%f" % seconds)

    # ---- inop plumbing
    def on_subsystems_modified(self, sub):
        if sub is self.inop:
            self.inop_update(False)

    def inop_update(self, force):
        self.inop_wheel_collider(self.inop.has(SS_WCOL), force)
        self.wheel.col.updateWheel = not self.inop.has(SS_TIRE)

    def inop_wheel_collider(self, inop, force):
        wh = self.wheel
        w = self.w
        if wh is None:
            return
        if inop and (force or wh.enabled) and self.setup:
            w.trace.append("DisableWheelCollider:%d" % (1 if force else 0))
            wh.set_enabled(False)
            wh.set_grounded(False)
        if (not inop) and (force or not wh.enabled) and self.setup:
            w.trace.append("EnableWheelCollider")
            wh.set_enabled(True)

    def check_suspension_toggle(self):
        if not self.suspensionEnabled and not self.inop.contains(self.inopSusp):
            self.inop.add(self.inopSusp)
        elif self.suspensionEnabled and self.inop.contains(self.inopSusp):
            self.inop.remove(self.inopSusp)

    def on_pack(self):
        self.inop.add(self.inopOnRails)

    def on_unpack(self):
        self.inop.remove(self.inopOnRails)

    def on_soi(self, gee):
        # ModuleWheelBase.cs:815: `FromTo.from == base.vessel` compares a CelestialBody with a Vessel (UnityEngine.Object ==): never true
        pass

    def on_docking(self):
        w = self.w
        self.hold_wheel_damage()
        w.trace.append("ignoreColliders")
        self.suspensionEnabled = False
        self.check_suspension_toggle()
        self.toggle_rb_constraints(False)
        self.inop_wheel_collider(True, True)
        self.inop_wheel_collider(False, True)
        self.wheel.col.springRate = 0.0
        self.suspensionEnabled = True
        self.check_suspension_toggle()
        if self.brakes is not None:
            prev = self.brakes.brakeInput
            self.brakes.brakeInput = 1.0
            w.sched.start(co_delayed_secs(1.0, lambda: setattr(self.brakes, "brakeInput", prev)))
        self.rbBrakeConstraints = False
        if self.wheelType != WT_LEG:
            self.schloompha()

    def on_undocking(self):
        w = self.w
        self.hold_wheel_damage()
        w.trace.append("ignoreColliders")
        self.suspensionEnabled = False
        self.check_suspension_toggle()
        self.toggle_rb_constraints(False)
        self.inop_wheel_collider(True, True)
        self.inop_wheel_collider(False, True)
        self.wheel.col.springRate = 0.0
        self.suspensionEnabled = True
        self.check_suspension_toggle()
        self.rbBrakeConstraints = False

    def on_put_to_ground(self, q):
        """OnPutToGround(PartHeightQuery): q = dict(lowestOnParts (this part), lowestPoint); deployment state read from the part."""
        p = self.part
        if self.groundHeightOffset != 0.0:
            dep = p.find(Deployment)
            flag = False
            if dep is not None and dep.stateString == "Retracted":
                q["lowest"] = m_min(q["lowest"], st(p.tr.transform_point(self.wheelOrgPosR)[1] - self.radius))
                flag = True
            if not flag:
                q["lowest"] = m_min(q["lowest"], st(p.tr.pos[1] - self.groundHeightOffset))
        else:
            q["lowest"] = m_min(q["lowest"], st(p.tr.transform_point(self.wheelOrgPosR)[1] - self.radius))
        q["point"] = m_min(q["point"], q["lowest"])

    # ---- drift fix
    def update_drift_fix(self):
        wh = self.wheel
        if self.dstate == "Idle":
            if wh.is_grounded:
                self.dstate = "Acquire"
        elif self.dstate == "Acquire":
            if wh.is_grounded:
                if v2_sqrmag(wh.state.lwv) < m_pow(self.acquireMaxSpeed, 2.0):
                    self.fixFwd = self.get_fix_fwd()
                    self.error = V3Z
                    self.dstate = "Fix"
            else:
                self.dstate = "Idle"
        elif self.dstate == "Fix":
            if wh.is_grounded:
                if v2_sqrmag(wh.state.lwv) < m_pow(self.acquireMaxSpeed, 2.0) and wh.is_grounded:
                    self.get_fix_torque(self.fixFwd, self.get_fix_fwd())
                    v1 = v3_mul(self.error, self.ki)
                    v2 = v3_mul(v3_sub(self.error, self.errorLast), self.kd)
                    self.w.trace.append("torque:%.17g,%.17g,%.17g" % v3_add(v1, v2))
                else:
                    self.dstate = "Acquire"
            else:
                self.dstate = "Idle"

    def get_fix_fwd(self):
        # wheel.cachedTransform = the wheel controller's own transform (the scripted part rigidbody transform)
        if self.wheelType != WT_LEG:
            return n_forward(self.part.tr.rot)
        return n_right(self.part.tr.rot)

    def get_fix_torque(self, fix, ref):
        vec = v3_mul(v3_cross(ref, fix), st(self.w.gee))
        self.errorLast = self.error
        self.error = v3_add(self.error, v3_div(vec, self.w.tw_dt_time()))

    # ---- events / actions
    def evt_auto_friction_toggle(self):
        self.autoFriction = not self.autoFriction
        self.action_ui_update()
        self.at_sym_update()

    def act_auto_friction_toggle(self, typ):
        self.autoFriction = (typ != ACT_ACTIVATE) or (typ == ACT_TOGGLE and not self.autoFriction)
        self.action_ui_update()

    def at_sym_update(self):
        pass  # no symmetry counterparts in the scripted world

    def action_ui_update(self):
        w = self.w
        if self.autoFrictionAvailable:
            self.apply_gee_bias(st(w.gee))
            return
        self.autoFriction = False
        self.frictionMultiplier = 1.0
        self.apply_gee_bias(st(w.gee))


def v2_sqrmag(v):
    return st(v[0] * v[0] + v[1] * v[1])


World.tw_dt_time = lambda self: self.dt  # Time.deltaTime (getFixTorque divides by it)
World.vessel_loaded = True


def speed_to_rpm(speed, r):
    return st(speed * 30.0 / (r * PI32))


def rpm_to_speed(rpm, r):
    return st(PI32 * 2.0 * r * (rpm / 60.0))


def rpm_to_degs_sec(rpm):
    return st(rpm * 360.0 * 60.0)


# collider kinds a wheel can touch (scripted; set by the scenario generator; kind 0 = no collider)
COLLIDER_KINDS = {}


class Sub:
    def __init__(self, part, n):
        self.part = part
        self.w = part.w
        self.node = n
        self.baseSetup = False
        self.wheel = None
        self.wheelBase = None

    def on_awake(self):
        self.baseSetup = False
        self.wheelBase = self.part.base
        self.wheelBase.register(self)

    def on_wheel_init(self, wheel):
        self.wheel = wheel
        self.baseSetup = True
        self.on_wheel_setup()
        self.wheelBase.inop.on_modified.append(self.on_subsystems_modified)

    def on_subsystems_modified(self, s):
        pass


class Motor(Sub):
    def __init__(self, part, n):
        super().__init__(part, n)
        g = n.get
        self.driveResponse = f32p(g("driveResponse", "3"))
        self.idleDrain = float(g("idleDrain", "0.3"))
        self.wheelSpeedMax = f32p(g("wheelSpeedMax", "120"))
        self.autoTorque = parse_bool(g("autoTorque", "True"))
        self.motorEnabled = parse_bool(g("motorEnabled", "True"))
        self.motorInverted = parse_bool(g("motorInverted", "False"))
        self.driveLimiter = f32p(g("driveLimiter", "100"))
        self.tractionControlScale = f32p(g("tractionControlScale", "1"))
        self.driveOutput = 0.0
        self.driveInput = 0.0
        self.maxTorque = 0.0
        self.state = MS_DISABLED
        self.resourceFraction = 0.0
        self.torqueCurve = curve_of(n.node("torqueCurve"))
        res = [r for r in n.nodes if r.name == "RESOURCE"]
        assert len(res) <= 1 and all(r.get("name") == "ElectricCharge" for r in res)
        self.ecRate = float(res[0].get("rate")) if res else None
        self.adjusters = []

    def on_wheel_setup(self):
        mn, mx = self.torqueCurve.find_min_max()
        self.maxTorque = mx
        self.wheel.maxDriveTorque = self.maxTorque
        self.wheel.maxDriveTorque = round_hundredth(self.wheel.maxDriveTorque)
        self.wheel.maxRpm = speed_to_rpm(self.wheelSpeedMax, self.wheelBase.radius)
        self.wheel.driveResponse = self.driveResponse

    def on_drive_update(self, motor_input):
        return motor_input

    def fixed_update(self):
        w = self.w
        wh = self.wheel
        p = self.part
        if not self.baseSetup:
            return
        if self.motorEnabled and not p.packed:
            c = w.ctrl
            self.driveInput = m_clamp(st(c[0] + c[1]), -1.0, 1.0)
            self.driveInput = st(self.driveInput * self.orientation_sign())
            self.driveInput = st(self.driveInput * (-1.0 if self.motorInverted else 1.0))
            wh.maxDriveTorque = self.torqueCurve.evaluate(st(abs(st(wh.state.angularVelocity * self.wheelBase.radius))))
            wh.maxDriveTorque = round_hundredth(wh.maxDriveTorque)
            self.driveInput = m_clamp(self.on_drive_update(self.driveInput), -1.0, 1.0)
            if self.autoTorque:
                self.driveInput = st(self.driveInput * (1.0 - abs(st(self.wheelBase.slip[1] * self.tractionControlScale))))
                self.driveLimiter = st(m_lerp(1.0, st(w.gee), m_clamp01(st(self.tractionControlScale / 2.0))) * 100.0)
            self.driveInput = st(self.driveInput * st_(self.driveLimiter * f32c(0.01)))
            if self.maxTorque > 0.0:
                self.driveInput = self.apply_adjust(self.driveInput)
                ok = self.check_motor_state(abs(self.driveInput), st(wh.maxDriveTorque / self.maxTorque))
                if ok:
                    wh.driveInput = st(self.driveInput * st(self.resourceFraction))
                else:
                    wh.driveInput = 0.0
                self.driveOutput = st(abs(st(self.driveInput * wh.maxDriveTorque / self.maxTorque)) * 100.0 * st(self.resourceFraction))
            else:
                self.driveOutput = 0.0
                self.state = MS_DISABLED
        else:
            self.driveInput = 0.0
            self.driveOutput = 0.0
            wh.driveInput = 0.0

    def apply_adjust(self, t):
        for a in self.adjusters:
            t = a(t)
        return t

    def orientation_sign(self):
        p = self.part
        ref = self.w.refTr
        num = v3_dot(p.tr.up(), ref.up())
        num2 = v3_dot(p.tr.forward(), ref.forward())
        num3 = v3_dot(p.tr.forward(), ref.up())
        num4 = -FLT_MAX
        f = num
        num5 = abs(num3)
        if num5 > num4:
            num4 = num5
            f = num3
        num5 = abs(num2)
        if num5 > num4:
            num4 = num5
            f = num2
        return m_sign(f)

    def update_inputs(self, rate_mult, threshold):
        """ModuleResourceHandler.UpdateModuleResourceInputs(ref error, rateMult, threshold, returnOnFirstLack: false, average: false, stringOps: false)
        for the single ElectricCharge input (no shut-off handler, no cheat options)."""
        w = self.w
        if self.ecRate is None:
            return 1.0
        num = 1.0
        dt = float(w.tw_fixed())
        num4 = self.ecRate * rate_mult
        request = num4 * dt
        got = min(request, max(w.ec, 0.0)) if request > 0.0 else 0.0
        w.ec -= got
        threshold = max(threshold, 0.001)
        if got < request * threshold:
            num = got / request
        return num

    def check_motor_state(self, drive_input, torque_scalar):
        if self.get_motor_enabled(self.motorEnabled):
            num = float(drive_input) * float(torque_scalar) + self.idleDrain
            r = self.update_inputs(num, 0.999)
            self.resourceFraction = r
            if r > 0.5:
                if drive_input == 0.0:
                    self.state = MS_IDLE
                    return True
                self.state = MS_RUNNING
                return True
            self.state = MS_NOTENOUGH
            return r > 0.1
        self.resourceFraction = 0.0
        return False

    def get_motor_enabled(self, base_enabled):
        if self.wheelBase.inop.has(SS_MOTOR):
            self.state = MS_INOPERABLE
            return False
        if not base_enabled:
            self.state = MS_DISABLED
        return base_enabled

    def get_max_speed(self):
        return self.torqueCurve.max_time


def st_(x):
    """a float expression that stays in double until it meets the next float store (no rounding here)"""
    return x


def round_hundredth(v):
    """(float)Math.Round(v * 100f) / 100f"""
    return st(float(round(v * 100.0)) / 100.0)


class MotorSteering(Motor):
    def __init__(self, part, n):
        super().__init__(part, n)
        self.steeringTorque = f32p(n.get("steeringTorque", "50"))
        self.steeringEnabled = parse_bool(n.get("steeringEnabled", "True"))
        self.steeringInvert = parse_bool(n.get("steeringInvert", "False"))
        self.wDot = 0.0

    def on_drive_update(self, motor_input):
        w = self.w
        p = self.part
        if self.steeringEnabled:
            wFwd = p.tr.forward()
            rcom = v3_normalized(v3_sub(p.tr.pos, w.CoM))
            wXcom = v3_cross(wFwd, rcom)
            axis = v3_mul(v3_neg(w.refTr.forward()), st(w.ctrl[2] + w.ctrl[3]))
            self.wDot = v3_dot(wXcom, axis)
            if abs(self.wDot) > f32c(0.1):
                self.wDot = m_sign(self.wDot)
                if self.steeringInvert:
                    self.wDot = st(self.wDot * -1.0)
                self.wheel.maxDriveTorque = self.steeringTorque
            else:
                self.wDot = 0.0
            return st(motor_input + self.wDot)
        return motor_input

    def get_motor_enabled(self, base_enabled):
        if self.wheelBase.inop.has(SS_STEER):
            return False
        if not base_enabled and not self.steeringEnabled:
            return False
        return True


class Brakes(Sub):
    def __init__(self, part, n):
        super().__init__(part, n)
        g = n.get
        self.maxBrakeTorque = f32p(g("maxBrakeTorque", "2000"))
        self.brakeResponse = f32p(g("brakeResponse", "10"))
        self.brakeTweakable = f32p(g("brakeTweakable", "50"))
        self.brakeInput = f32p(g("brakeInput", "-1"))
        self.hasLight = g("statusLightModuleIndex", "-1") != "-1"
        self.light_on = False
        self.light_log = []
        self.adjusters = []

    def on_start(self):
        if self.brakeInput < 0.0:
            self.brakeInput = 0.0
            if self.w.brakesGroup:
                self.brakeInput = 1.0

    def on_wheel_setup(self):
        self.wheel.maxBrakeTorque = self.maxBrakeTorque
        self.wheel.brakeResponse = self.brakeResponse
        if self.hasLight:
            self.set_light(self.brakeInput > 0.0 and self.brakeTweakable > 0.0)

    def set_light(self, on):
        self.light_on = on
        self.light_log.append(on)

    def brake_action(self, typ):
        k = typ
        if k == ACT_TOGGLE:
            k = ACT_DEACTIVATE if not (self.brakeInput <= 0.0) else ACT_ACTIVATE
        if k == ACT_DEACTIVATE:
            self.brakeInput = 0.0
        elif k == ACT_ACTIVATE:
            self.brakeInput = 1.0
        if self.hasLight:
            self.set_light(typ == ACT_ACTIVATE and self.brakeTweakable > 0.0)

    def fixed_update(self):
        w = self.w
        wh = self.wheel
        if not self.baseSetup:
            return
        if not self.part.packed and not self.wheelBase.inop.has(SS_BRAKES):
            num = m_min(1.0, m_lerp(1.0, st(w.gee), abs(wh.state.angularVelocity)))
            t = self.brakeInput
            for a in self.adjusters:
                t = a(t)
            wh.brakeInput = st(t * st_(self.brakeTweakable * f32c(0.01)) * num)
            wh.brakeResponse = self.brakeResponse if wh.brakeInput != 0.0 else st(100.0)
        else:
            wh.brakeResponse = self.brakeResponse
            wh.brakeInput = 0.0

    def on_subsystems_modified(self, s):
        if s is not self.wheelBase.inop or not self.hasLight:
            return
        if s.has(SS_BRAKES):
            if self.light_on:
                self.set_light(False)
        elif self.brakeInput > 0.0:
            self.set_light(True)


class Lock(Sub):
    def __init__(self, part, n):
        super().__init__(part, n)
        self.maxTorque = f32p(n.get("maxTorque", "1000"))

    def on_wheel_setup(self):
        self.wheel.maxBrakeTorque = self.maxTorque
        self.wheel.brakeResponse = st(1000.0)
        self.wheel.brakeInput = 1.0


class Suspension(Sub):
    def __init__(self, part, n):
        super().__init__(part, n)
        g = n.get
        self.suspensionOffset = f32p(g("suspensionOffset", "0"))
        self.suspensionDistance = f32p(g("suspensionDistance", "0"))
        self.spring = 0.0
        self.damper = 0.0
        self.targetPosition = f32p(g("targetPosition", "1"))
        self.springRatio = f32p(g("springRatio", "50"))
        self.springTweakable = f32p(g("springTweakable", "1"))
        self.damperRatio = f32p(g("damperRatio", "1"))
        self.damperTweakable = f32p(g("damperTweakable", "1"))
        self.autoSpringDamper = True
        self.autoSpringDamperSet = False
        self.boostRatio = f32p(g("boostRatio", "0"))
        self.maximumLoad = f32p(g("maximumLoad", "0"))
        self.useDistributedMass = parse_bool(g("useDistributedMass", "False"))
        self.damperFudge = 0.0
        self.boost = 0.0
        self.vesselMass = 0.0
        self.suspCompression = 0.0
        self.suspMass = 0.0
        self.sprungMassGravity = 0.0
        self.autoBoost = f32p(g("autoBoost", "0"))
        self.useAutoBoost = parse_bool(g("useAutoBoost", "True"))
        self.adjustForHighGee = parse_bool(g("adjustForHighGee", "True"))
        self.highGeeThreshold = f32p(g("highGeeThreshold", "1.5"))
        self.highGeeSpringTweakable = f32p(g("highGeeSpringTweakable", "3"))
        self.highGeeDamperTweakable = f32p(g("highGeeDamperTweakable", "1.2"))
        self.highGeeOverride = False
        self.springClampMax = st(5000.0)
        self.damperClampMax = st(5000.0)
        self.damperLerpBase = 1.0
        self.oscillationDamper = 3.0
        self.boostClamp = st(0.5)
        self.autoBoostClamp = 1.0

    def on_awake(self):
        super().on_awake()
        self.autoSpringDamper = ENV["WHEEL_AUTO_SPRINGDAMPER"] or self.maximumLoad <= 0.0

    def on_wheel_setup(self):
        wh = self.wheel
        wh.col.updateSuspension = not self.wheelBase.inop.has(SS_SUSP)
        wh.col.suspensionDistance = st(self.suspensionDistance * self.part.rescale)
        wh.col.suspensionAnchor = self.targetPosition
        wh.col.suspensionOffset = st(self.suspensionOffset * self.part.rescale)
        wh.col.springRate = self.spring
        wh.col.damperRate = self.damper

    def fixed_update(self):
        w = self.w
        wh = self.wheel
        if self.baseSetup and not self.part.packed and wh is not None and wh.is_grounded:
            self.vesselMass = st(w.totalMass)
            self.spring_update(self.vesselMass)

    def boost_curve(self, b, power):
        inner = m_pow(m_clamp(b, 0.0, 2.0), m_clamp(power, f32c(0.01), self.autoBoostClamp))
        v = m_clamp(st(1.0 / abs(st(1.0 - 2.0 / inner))), f32c(0.01), 10000000.0)
        return st(m_round(st(v * 10.0)) / 10.0)

    def update_auto_boost(self, stv):
        if not self.useAutoBoost:
            self.autoBoost = 0.0
        elif stv > f32c(0.8):
            self.autoBoost = m_clamp(st(self.autoBoost + f32c(0.2) * self.w.fdt), f32c(0.85), 2.0)
        elif stv < f32c(0.4):
            self.autoBoost = m_max(0.0, st(self.autoBoost - f32c(0.2) * self.w.fdt))

    def spring_update(self, sprung):
        w = self.w
        wh = self.wheel
        col = wh.col
        self.suspCompression = wh.state.compression
        self.suspMass = sprung
        num = st(sprung * self.suspCompression)
        lerp_t = m_inverse_lerp(50.0, 200.0, sprung)
        self.springClampMax = m_lerp(5000.0, 10000.0, lerp_t)
        self.damperClampMax = m_lerp(5000.0, 10000.0, m_inverse_lerp(50.0, 200.0, sprung))
        self.update_auto_boost(self.suspCompression)
        self.boost = st(1.0 + m_clamp(st(self.suspCompression * self.boost_curve(st(st(w.gee) * st(w.gravMult)), 1.0)), -1.0, self.boostClamp))
        if not self.useDistributedMass:
            self.sprungMassGravity = num
        if self.autoSpringDamper and (not self.autoSpringDamperSet or (self.maximumLoad > 0.0 and num > self.maximumLoad)):
            if w.gee < 1.0:
                self.damperTweakable = st(1.1)
            else:
                t = m_inverse_lerp(self.damperLerpBase, st(ENV["MAX_SURFACE_GEE"]), st(w.gee))
                self.damperTweakable = st(round(m_lerp(1.0, 2.0, t) * 100.0) / 100.0)
            if self.spring > 0.0 and (not self.autoSpringDamperSet or (self.maximumLoad > 0.0 and num > self.maximumLoad)):
                value = st(self.suspensionDistance * self.spring / st(ENV["GRAV_ACCEL"]))
                t2 = m_inverse_lerp(0.0, st(self.maximumLoad * 2.0), value)
                self.springTweakable = st(round(m_lerp(1.0, 3.0, t2) * 100.0) / 100.0)
                self.autoSpringDamperSet = True
        self.damperFudge = m_lerp(1.0, 15.0, m_inverse_lerp(10.0, 100.0, num))
        self.spring = m_clamp(st(self.springTweakable * self.springRatio * self.sprungMassGravity * self.boost_curve(self.boost, st(self.boostRatio + self.autoBoost))), f32c(0.01), self.springClampMax)
        self.damper = st(st(math.sqrt(m_clamp(st(self.spring * self.damperRatio * self.damperFudge), f32c(0.01), self.damperClampMax))) * self.damperTweakable)
        if self.adjustForHighGee and w.gee > float(self.highGeeThreshold):
            self.spring = st(self.spring * self.highGeeSpringTweakable)
            self.damper = st(self.damper * self.highGeeDamperTweakable)
            self.highGeeOverride = True
        else:
            self.highGeeOverride = False
        self.damper = m_max(m_round(self.damper), f32c(0.1))
        self.spring = m_max(m_round(self.spring), f32c(0.1))
        if st(self.spring - col.springRate) > self.oscillationDamper:
            col.springRate = self.spring
        elif col.springRate > st(self.spring + self.oscillationDamper):
            col.springRate = self.spring
        if abs(st(self.damper - col.damperRate)) > f32c(1.05):
            col.damperRate = self.damper

    def on_subsystems_modified(self, s):
        if s is self.wheelBase.inop:
            self.wheel.col.updateSuspension = not s.has(SS_SUSP)

    def on_part_count_changed(self):
        if self.baseSetup:
            self.wheel.col.springRate = 0.0

    def late_update(self):
        pass

    def toggle_auto_spring_damper(self):
        self.autoSpringDamper = not self.autoSpringDamper
        self.autoSpringDamperSet = False


class Steering(Sub):
    def __init__(self, part, n):
        super().__init__(part, n)
        g = n.get
        self.steeringCurve = curve_of(n.node("steeringCurve"))
        self.steeringMaxAngleCurve = curve_of(n.node("steeringMaxAngleCurve"))
        self.steeringResponse = f32p(g("steeringResponse", "8"))
        self.autoSteeringAdjust = parse_bool(g("autoSteeringAdjust", "True"))
        self.steeringRange = f32p(g("steeringRange", "0"))
        self.steeringInput = 0.0
        self.steeringInputLast = 0.0
        self.steerAngle = 0.0
        self.steeringEnabled = parse_bool(g("steeringEnabled", "True"))
        self.steeringInvert = parse_bool(g("steeringInvert", "False"))
        self.angleTweakable = f32p(g("angleTweakable", "-1"))
        self.responseTweakable = f32p(g("responseTweakable", "-1"))
        self.caliperUpdating = False
        self.CoMfwdLength = 0.0
        self.upAxis = V3Z
        self.fwdAxis = V3Z
        self.leftAxis = V3Z
        self.CoM = V3Z
        self.wCenter = V3Z
        self.wLeft0 = V3Z
        self.wLeft = V3Z
        self.wRight = V3Z
        self.sAxis = V3Z
        self.prevLwv = (0.0, 0.0)
        self.tPivot = V3Z
        self.tOrt = V3Z
        self.tCenter = V3Z
        self.h = 0.0
        self.hasWheelTransform = part.base.hasWheelTransform
        self.angleCurveDefined = False
        self.curveDefined = False

    def on_start(self):
        self.curveDefined = self.steeringCurve.max_time != -FLT_MAX
        self.angleCurveDefined = self.steeringMaxAngleCurve.max_time != -FLT_MAX
        if self.steeringRange < f32c(0.01):
            mn, mx = self.steeringCurve.find_min_max()
            self.steeringRange = m_max(mx, f32c(0.01))
        if self.angleTweakable < 0.0:
            self.angleTweakable = self.steeringRange
        if self.responseTweakable < 0.0:
            self.responseTweakable = self.steeringResponse

    def on_wheel_setup(self):
        wh = self.wheel
        if self.angleTweakable < 0.0:
            self.angleTweakable = self.steeringResponse
        wh.maxSteerAngle = self.angleTweakable
        if self.responseTweakable < 0.0:
            self.responseTweakable = self.steeringRange
        wh.steeringResponse = self.steeringResponse
        self.set_caliper_update(not self.wheelBase.inop.has(SS_STEER))
        if self.hasWheelTransform:
            self.wLeft0 = quat_mul_vec(quat_conj(self.part.tr.rot), v3_neg(n_right(self.wheelBase.wheelTrRot)))
        else:
            self.wLeft0 = (-1.0, 0.0, 0.0)

    def set_caliper_update(self, update):
        self.caliperUpdating = update
        if self.wheel is not None:
            self.wheel.col.updateCaliper = update
        self.steerAngle = 0.0
        self.steeringInput = 0.0

    def on_subsystems_modified(self, s):
        if s is self.wheelBase.inop:
            self.set_caliper_update(not s.has(SS_STEER))

    def on_wheel_repaired(self):
        self.steerAngle = 0.0
        self.steeringInput = 0.0

    def fixed_update(self):
        w = self.w
        wh = self.wheel
        if not self.baseSetup:
            return
        if not self.part.packed and self.steeringEnabled and self.caliperUpdating:
            self.update_coord_frame()
            self.steeringInput = m_clamp(st(w.ctrl[2] + w.ctrl[3]), -1.0, 1.0)
            if self.steeringInvert:
                self.steeringInput = st(self.steeringInput * -1.0)
            wh.steerInput = self.update_steering(self.steeringInput, self.steeringRange)
            self.steeringInputLast = self.steeringInput
            wh.steeringResponse = st(self.steeringResponse * self.steering_response_scale(st(self.steeringInput - self.steeringInputLast)))
        else:
            wh.steeringResponse = self.steeringResponse
            wh.steerInput = 0.0

    def steering_response_scale(self, delta):
        wh = self.wheel
        s = wh.state
        num = abs(s.lwv[1] if s.grounded else self.prevLwv[1])
        if s.grounded:
            self.prevLwv = s.lwv
        if delta < 0.0:
            num = 0.0
        num = st(num * (1.0 - s.sidewaysSlip))
        if self.autoSteeringAdjust:
            wh.maxSteerAngle = st(self.angleTweakable * (m_clamp01(self.steeringMaxAngleCurve.evaluate(num)) if self.angleCurveDefined else 1.0))
            wh.maxSteerAngle = round_hundredth(wh.maxSteerAngle)
            if self.curveDefined:
                return st(self.steeringCurve.evaluate(num) / self.steeringRange)
            return self.responseTweakable
        wh.maxSteerAngle = self.angleTweakable
        return self.responseTweakable

    def update_coord_frame(self):
        w = self.w
        ref = w.refTr
        ref_up, ref_fwd = ref.up(), ref.forward()
        up_axis_v = tuple(st(c) for c in w.upAxis)
        num = v3_dot(ref_up, up_axis_v)
        self.upAxis = ref_up
        if num < v3_dot(v3_neg(ref_fwd), up_axis_v):
            self.upAxis = v3_neg(ref_fwd)
        ref_forward = v3_neg(ref_fwd)
        if v3_approx_eq(self.upAxis, ref_up):
            ref_forward = ref_fwd
        self.fwdAxis = ref_up
        if num > v3_dot(ref_forward, up_axis_v):
            self.fwdAxis = ref_forward
        self.leftAxis = v3_cross(self.fwdAxis, self.upAxis)
        self.CoM = w.CoM
        self.wCenter = self.part.tr.transform_point(self.wheelBase.wheelOrgPosR)
        self.wLeft = v3_normalized(v3_project_on_plane(self.part.tr.transform_direction(self.wLeft0), self.upAxis))
        self.wRight = v3_neg(self.wLeft)

    def update_steering(self, inp, steering_range):
        w = self.w
        num = st(steering_range * inp)
        vec = self.find_turn_center(num, w.steeringRadiusValue, self.CoM)
        if not v3_approx_eq(vec, V3Z):
            self.sAxis = v3_normalized(v3_project_on_plane(v3_sub(vec, self.wCenter), self.upAxis))
            a = v3_dot(self.wRight, self.sAxis)
            a = m_max(a, v3_dot(self.wLeft, self.sAxis))
            self.steerAngle = st(m_acos(a) * f32c(57.29578) * m_sign(inp))
        else:
            self.steerAngle = 0.0
        if self.CoMfwdLength > 0.0:
            self.steerAngle = st(0.0 - self.steerAngle)
        if self.steerAngle > 90.0:
            self.steerAngle = st(180.0 - self.steerAngle)
        elif self.steerAngle < -90.0:
            self.steerAngle = st(-180.0 - self.steerAngle)
        return m_clamp(st(self.steerAngle / self.wheel.maxSteerAngle), -1.0, 1.0)

    def find_turn_center(self, steer_angle, length, com):
        if steer_angle != 0.0:
            self.tPivot = v3_mul(self.fwdAxis, length)
            q = n_angle_axis(abs(steer_angle), v3_mul(v3_neg(self.upAxis), m_sign(steer_angle)))
            self.tOrt = v3_mul(quat_mul_vec(q, self.leftAxis), m_sign(steer_angle))
            self.h = st(length * (1.0 / m_tan(st(abs(steer_angle) * DEG2RAD32))))
            self.tCenter = v3_add(v3_add(com, self.tPivot), v3_mul(self.tOrt, self.h))
        else:
            self.tCenter = V3Z
        return self.tCenter

    def get_com_fwd_length(self):
        self.CoMfwdLength = st(v3_dot(v3_sub(self.wCenter, self.CoM), self.fwdAxis))
        return abs(self.CoMfwdLength)


class Damage(Sub):
    def __init__(self, part, n):
        super().__init__(part, n)
        g = n.get
        self.stressTolerance = f32p(g("stressTolerance", "10"))
        self.impactTolerance = f32p(g("impactTolerance", "40"))
        self.deflectionMagnitude = f32p(g("deflectionMagnitude", "1"))
        self.slipMagnitude = f32p(g("slipMagnitude", "1"))
        self.deflectionSharpness = f32p(g("deflectionSharpness", "1"))
        self.slipSharpness = f32p(g("slipSharpness", "2"))
        self.isRepairable = parse_bool(g("isRepairable", "True"))
        self.explodeMultiplier = f32p(g("explodeMultiplier", "5"))
        self.currentDeflection = self.lastDeflection = 0.0
        self.currentSlip = self.lastSlip = 0.0
        self.currentDownForce = self.lastDownForce = 0.0
        self.repairImmunityTimeTotal = 30.0
        self.repairImmunityTimeLeft = 0.0
        self.startupTime = 8.0
        self.repairKitsNecessary = 1
        self.initialized = False
        self.isDamaged = parse_bool(g("isDamaged", "False"))
        self.totalStress = 0.0
        self.stressTime = 0.0
        self.stressPercent = 0.0
        self.stressVariability = 0.0
        self.impactDamageVelocity = f32p(g("impactDamageVelocity", "0"))
        self.hasImpactCollider = g("impactDamageColliderName", "") != ""
        self.subsystemDamage = Subsys("Wheel Damage", 33, self)
        self.noCrashDamage = False
        self.exploded = False

    def is_impact_damage_enabled(self):
        if self.impactDamageVelocity > 0.0:
            if ENV["WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED"] and self.hasImpactCollider:
                return True
            return ENV["WHEEL_DAMAGE_WHEELCOLLIDER_ENABLED"]
        return False

    def on_start(self):
        self.repairKitsNecessary = min(max(int(self.part.mass / ENV["PART_REPAIR_MASS_PER_KIT"]), 1), ENV["PART_REPAIR_MAX_KIT_AMOUNT"])
        self.isDamaged = not self.isDamaged
        self.set_damaged(not self.isDamaged)

    def on_wheel_setup(self):
        def cb():
            self.isDamaged = not self.isDamaged
            self.set_damaged(not self.isDamaged)
        self.w.sched.start(co_delayed_frames(1, cb))

    def reset_immunity(self):
        self.startupTime = 8.0
        self.repairImmunityTimeTotal = 30.0

    def initialize(self):
        wh = self.wheel
        base = self.wheelBase
        self.currentDeflection = st(self.deflectionMagnitude * wh.legacy_wheel_load())
        self.currentSlip = st(self.slipMagnitude * wh.legacy_wheel_load() * v2_mag(base.slip))
        self.currentDownForce = wh.state.downforce
        self.lastDeflection = self.currentDeflection
        self.lastSlip = self.currentSlip
        self.lastDownForce = self.currentDownForce
        self.initialized = True

    def fixed_update(self):
        w = self.w
        wh = self.wheel
        base = self.wheelBase
        if not self.baseSetup or w.situation_prelaunch:
            return
        if not self.part.packed and not self.isDamaged:
            if not self.initialized:
                self.initialize()
            self.lastDeflection = self.currentDeflection
            self.lastSlip = self.currentSlip
            self.lastDownForce = self.currentDownForce
            if self.repairImmunityTimeLeft > 0.0:
                self.repairImmunityTimeLeft = m_max(0.0, st(self.repairImmunityTimeLeft - w.fdt))
            num = st(1.0 - self.repairImmunityTimeLeft / self.repairImmunityTimeTotal)
            self.currentDeflection = st(self.deflectionMagnitude * wh.legacy_wheel_load())
            self.currentSlip = st(self.slipMagnitude * wh.legacy_wheel_load() * v2_mag(base.slip))
            self.currentDeflection = m_lerp(self.lastDeflection, self.currentDeflection, st(self.deflectionSharpness * w.tw_fixed()))
            self.currentSlip = m_lerp(self.lastSlip, self.currentSlip, st(self.slipSharpness * w.tw_fixed()))
            self.currentDownForce = wh.state.downforce
            if self.startupTime > 0.0:
                self.startupTime = st(self.startupTime - w.dt)
                return
            num2 = st(self.currentDeflection * num * ENV["WHEEL_WEIGHT_STRESS_MULTIPLIER"])
            num3 = st(self.currentSlip * num * ENV["WHEEL_SLIP_STRESS_MULTIPLIER"])
            self.totalStress = m_clamp(st(num2 + num3), 0.0, self.stressTolerance)
            self.stressPercent = st(self.totalStress / self.stressTolerance * 100.0)
            num4 = st(self.stressTolerance + self.stressVariability)
            if self.totalStress >= num4:
                self.stressTime = st(self.stressTime + w.tw_fixed())
                num5 = st(1.0 - m_clamp01(st((self.totalStress - num4) / st(num4 / 2.0))))
                num6 = st(0.25 + 0.25 * num5)
                if self.stressTime > num6:
                    self.set_damaged(True)
            else:
                self.stressTime = 0.0
            num7 = st(st(self.currentDownForce - self.lastDownForce) * wh.gravity_mag() / st(ENV["GRAV_ACCEL"]))
            if not self.noCrashDamage:
                if num7 >= st(self.impactTolerance * self.explodeMultiplier):
                    self.exploded = True
                    w.trace.append("explode")
                elif num7 >= self.impactTolerance:
                    self.set_damaged(True)
        else:
            self.currentDeflection = 0.0
            self.currentSlip = 0.0
            self.currentDownForce = 0.0
            self.stressTime = 0.0
            self.initialized = False
            self.startupTime = 8.0
            self.repairImmunityTimeTotal = 30.0
            self.repairImmunityTimeLeft = 0.0
            self.totalStress = 0.0
            self.stressPercent = 0.0

    def set_damaged(self, damaged):
        w = self.w
        if damaged and not self.isDamaged:
            if self.baseSetup:
                self.wheelBase.inop.add(self.subsystemDamage)
            w.trace.append("onPartRepaired")
        if not damaged and self.isDamaged:
            self.repairImmunityTimeTotal = w.rand.range(30.0, 90.0)
            self.repairImmunityTimeLeft = self.repairImmunityTimeTotal
            if self.baseSetup:
                self.wheelBase.inop.remove(self.subsystemDamage)
            w.trace.append("onWheelRepaired")
            st_ = self.part.find(Steering)
            if st_ is not None:
                st_.on_wheel_repaired()
        self.isDamaged = damaged
        self.stressVariability = st(st(0.0 - w.rand.value()) * self.stressTolerance * f32c(0.02))

    def handle_collision_velocity(self, kind, velocity, normal):
        if not self.is_impact_damage_enabled():
            return
        vec = v3_project(velocity, normal)
        if v3_mag(vec) < self.impactDamageVelocity:
            return
        self.set_damaged(True)

    def repair(self):
        self.set_damaged(False)


def v2_mag(v):
    return st(fsqrt(v[0] * v[0] + v[1] * v[1]))


World.situation_prelaunch = False


class Deployment(Sub):
    def __init__(self, part, n):
        super().__init__(part, n)
        g = n.get
        self.shieldedCanDeploy = parse_bool(g("shieldedCanDeploy", "False"))
        self.stateString = g("stateString", "")
        self.deployedPosition = f32p(g("deployedPosition", "0"))
        self.retractedPosition = 0.0
        self.extendDurationFactor = f32p(g("extendDurationFactor", "1"))
        self.retractDurationFactor = f32p(g("retractDurationFactor", "1"))
        self.TsubSys = f32p(g("TsubSys", "0.3"))
        self.hasDeployTgt = g("deployTargetTransformName", "") != ""
        self.hasRetractTransform = g("retractTransformName", "") != ""
        self.useStandIn = parse_bool(g("useStandInCollider", "False"))
        self.fx = {k: g(k, "") for k in ("fxDeploy", "fxDeployed", "fxRetract", "fxRetracted")}
        self.animLen = 1.0
        self.position = 0.0
        self.animLength = 1.0
        self.subsystemsDisabled = False
        self.retractStarted = False
        self.deploymentSubsystems = Subsys("Wheel Retracted", SS_ALL, self)
        self.anim = AnimState(1.0)
        self.fsm = FSM()
        self.standIn = False
        self.shielded = False
        self.cubeWeights = {}
        self.slave_log = []
        self.sample_axis = V3Z
        self.retract_pos = V3Z

    def effect(self, field, val=None):
        """Part.Effect(name[, value]) when the cfg names the effect"""
        if self.fx[field] != "":
            self.w.trace.append("fx:%s%s" % (self.fx[field], "" if val is None else ":%f" % val))

    def Position(self):
        if self.deployedPosition != 0.0:
            return self.position
        return st(1.0 - self.position)

    def on_load_position(self, pos):
        self.position = pos

    def on_start(self):
        a = self.anim
        a.length = F(self.animLen)
        a.time = F(0.0)           # Animation.Stop(): rewound, disabled
        a.enabled = True
        a.weight = F(1.0)
        a.speed = F(0.0)
        self.animLength = self.animLen
        self.position = m_clamp01(self.position)
        self.retractedPosition = st(1.0 - self.deployedPosition)
        self.standIn = False
        if self.stateString != "":
            a.normalized_time = self.position
            self.fsm_setup(self.stateString)
        else:
            a.normalized_time = self.deployedPosition
            self.fsm_setup("Deployed")

    def on_wheel_setup(self):
        self.set_inop_subsystems(self.Position() < self.TsubSys)
        if self.hasDeployTgt:
            self.w.trace.append("setWheelTransform")

    def set_inop_subsystems(self, disable):
        w = self.w
        if disable and not self.subsystemsDisabled:
            w.trace.append("slaves:0")
            self.wheelBase.inop.add(self.deploymentSubsystems)
            if self.useStandIn and not self.standIn:
                self.standIn = True
                w.trace.append("standIn:create")
        if not disable and self.subsystemsDisabled:
            w.trace.append("slaves:ui")
            self.wheelBase.inop.remove(self.deploymentSubsystems)
            if self.standIn:
                self.standIn = False
                w.trace.append("standIn:destroy")
        self.subsystemsDisabled = disable

    def _set_tr(self):
        if self.hasDeployTgt and self.wheel is not None:
            self.w.trace.append("setWheelTransform")

    def fsm_setup(self, start):
        w = self.w
        a = self.anim
        f = self.fsm
        dep = FState("Deployed")
        ret = FState("Retracted")
        deploying = FState("Deploying...")
        retracting = FState("Retracting...")
        inop = FState("Inoperable")
        self.st_deployed, self.st_retracted, self.st_deploying, self.st_retracting, self.st_inoperable = dep, ret, deploying, retracting, inop

        def dep_enter(prev):
            self.position = self.deployedPosition
            a.speed = F(0.0)
            a.normalized_time = self.deployedPosition
            self._set_tr()
            w.trace.append("cubes:Retracted=0.000000,Deployed=1.000000")
        dep.on_enter = dep_enter

        def retracting_enter(prev):
            self.retractStarted = True
        retracting.on_enter = retracting_enter

        def retracting_fixed():
            if self.retractStarted:
                self.retractStarted = False
                if self.deployedPosition == 0.0:
                    a.speed = F(self.retractDurationFactor)
                else:
                    a.speed = F(-self.retractDurationFactor)
                if self.hasRetractTransform:
                    a.normalized_time = self.find_transform_animation_time()
            self.position = m_clamp01(a.normalized_time)
            self._set_tr()
            w.trace.append("cubes:Retracted=%f,Deployed=%f" % (st(1.0 - self.Position()), self.Position()))
            if self.baseSetup:
                self.set_inop_subsystems(self.Position() < self.TsubSys)
        retracting.on_fixed = retracting_fixed

        def ret_enter(prev):
            self.position = self.retractedPosition
            a.speed = F(0.0)
            a.normalized_time = self.retractedPosition
            self._set_tr()
            w.trace.append("cubes:Retracted=1.000000,Deployed=0.000000")
        ret.on_enter = ret_enter

        def deploying_enter(prev):
            if self.deployedPosition == 0.0:
                a.speed = F(-self.extendDurationFactor)
            else:
                a.speed = F(self.extendDurationFactor)
        deploying.on_enter = deploying_enter

        def deploying_update():
            self.position = m_clamp01(a.normalized_time)
            self._set_tr()
            w.trace.append("cubes:Retracted=%f,Deployed=%f" % (st(1.0 - self.Position()), self.Position()))
            if self.baseSetup:
                self.set_inop_subsystems(self.Position() < self.TsubSys)
        deploying.on_update = deploying_update

        def inop_enter(prev):
            a.speed = F(0.0)
        inop.on_enter = inop_enter
        for s in (dep, retracting, ret, deploying, inop):
            f.add_state(s)
        on_deploy = FEvent("on_deploy", "manual")
        on_deploy.goto = deploying
        on_deploy.on_event = lambda: (self.effect("fxDeploy"), self.effect("fxRetract", 0.0))
        f.add_event(on_deploy, ret, retracting)
        on_deployed = FEvent("on_deployed", "late")
        on_deployed.goto = dep
        on_deployed.cond = lambda s_: abs(st(m_clamp01(a.normalized_time) - self.deployedPosition)) < f32c(0.01)

        def deployed_ev():
            if self.baseSetup:
                self.set_inop_subsystems(False)
            self.effect("fxDeployed")
            self.effect("fxRetract", 0.0)
            self.effect("fxDeploy", 0.0)
        on_deployed.on_event = deployed_ev
        f.add_event(on_deployed, deploying)
        on_retract = FEvent("on_retract", "manual")
        on_retract.goto = retracting
        on_retract.on_event = lambda: (self.effect("fxRetract"), self.effect("fxDeploy", 0.0))
        f.add_event(on_retract, dep, deploying)
        on_retracted = FEvent("on_retracted", "late")
        on_retracted.goto = ret
        on_retracted.cond = lambda s_: abs(st(m_clamp01(a.normalized_time) - self.retractedPosition)) < f32c(0.01)

        def retracted_ev():
            if self.baseSetup:
                self.set_inop_subsystems(True)
            self.effect("fxRetracted")
            self.effect("fxRetract", 0.0)
            self.effect("fxDeploy", 0.0)
        on_retracted.on_event = retracted_ev
        f.add_event(on_retracted, retracting)
        on_inop = FEvent("on_inoperable", "manual")
        on_inop.goto = inop
        on_inop.cond = lambda s_: False
        on_inop.on_event = lambda: (self.effect("fxRetract", 0.0), self.effect("fxDeploy", 0.0))
        f.add_event_excluding(on_inop, inop)
        self.on_deploy, self.on_retract, self.on_inoperable = on_deploy, on_retract, on_inop

        def changed(frm, to, ev):
            self.stateString = f.cur_name
        f.on_change.append(changed)
        f.start(start)
        changed(ret, dep, on_deploy)

    def sample(self):
        """Animation.Sample(): the scripted retract transform follows the animation along a line (position = axis * normalizedTime)"""
        self.retract_pos = v3_mul(self.sample_axis, self.anim.normalized_time)

    def find_transform_animation_time(self):
        a = self.anim
        saved = a.normalized_time
        best = FLT_MAX
        result = FLT_MAX
        b = self.retract_pos

        def sample(_t):
            self.sample()
            return v3_mag(v3_sub(self.retract_pos, b))
        if self.deployedPosition == 0.0:
            n = 0.0
            while n < 1.0:
                a.normalized_time = n
                d = sample(a.normalized_time)
                if d < best:
                    best = d
                    result = n
                n = st(n + f32c(0.025))
        else:
            n = 1.0
            while n > 0.0:
                a.normalized_time = n
                d = sample(a.normalized_time)
                if d < best:
                    best = d
                    result = n
                n = st(n - f32c(0.025))
        a.normalized_time = saved
        self.sample()
        return result

    def toggle_deployment(self):
        f = self.fsm
        if f.cur is not self.st_deployed and f.cur is not self.st_deploying:
            if f.cur is self.st_retracted or f.cur is self.st_retracting:
                if self.shielded and not self.shieldedCanDeploy:
                    self.w.trace.append("shieldMsg")
                    return
                f.run_event(self.on_deploy)
        else:
            f.run_event(self.on_retract)

    def action_toggle(self, typ):
        f = self.fsm
        k = typ
        if k == ACT_TOGGLE:
            k = ACT_ACTIVATE if (f.cur is self.st_retracted or f.cur is self.st_retracting) else ACT_DEACTIVATE
        if k == ACT_DEACTIVATE:
            if f.cur is self.st_deployed or f.cur is self.st_deploying:
                f.run_event(self.on_retract)
        elif k == ACT_ACTIVATE:
            if f.cur is self.st_retracted or f.cur is self.st_retracting:
                if self.shielded and not self.shieldedCanDeploy:
                    self.w.trace.append("shieldMsg")
                else:
                    f.run_event(self.on_deploy)

    def fixed_update(self):
        if self.fsm.started:
            self.fsm.fixed()

    def update(self):
        if self.fsm.started:
            self.fsm.update()

    def late_update(self):
        if self.fsm is not None and self.fsm.started:
            self.fsm.late()


class Bogey(Sub):
    def __init__(self, part, n):
        super().__init__(part, n)
        g = n.get
        self.maxPitch = f32p(g("maxPitch", "25"))
        self.minPitch = f32p(g("minPitch", "-25"))
        self.restPitch = f32p(g("restPitch", "-25"))
        self.pitchResponse = f32p(g("pitchResponse", "8"))
        self.bogeyAxis = parse_v3(g("bogeyAxis", "1,0,0"))
        self.bogeyUpAxis = parse_v3(g("bogeyUpAxis", "0,1,0"))
        self.deployModuleIndex = int(g("deployModuleIndex", "-1"))
        self.bogeyUp = V3Z
        self.bogeyUp0 = V3Z
        self.bogeyRot = (0.0, 0.0, 0.0, 1.0)
        self.bogeyRot0 = (0.0, 0.0, 0.0, 1.0)
        self.bogeyAngle = 0.0
        self.parentRot = (0.0, 0.0, 0.0, 1.0)   # bogey.parent.rotation (scripted)
        self.refLocalRot = (0.0, 0.0, 0.0, 1.0)  # bogeyRef.localRotation (scripted)
        self.deploy = None

    def on_start(self):
        if self.deployModuleIndex != -1:
            self.deploy = self.part.mods[self.deployModuleIndex]

    def on_wheel_setup(self):
        self.bogeyRot0 = self.refLocalRot
        self.bogeyUp0 = quat_mul_vec(self.refLocalRot, self.bogeyUpAxis)

    def late_update(self):
        if not self.baseSetup or self.part.packed:
            return
        if (self.deploy is None or self.deploy.Position() > f32c(0.8)) and not self.wheelBase.inop.has(SS_BOGEY):
            wh = self.wheel
            if wh.is_grounded:
                self.bogeyUp = v3_project_on_plane(wh.state.normal, quat_mul_vec(self.parentRot, self.bogeyAxis))
                loc = quat_mul_vec(quat_conj(self.parentRot), self.bogeyUp)
                self.bogeyAngle = m_clamp(bearing_degrees(self.bogeyUp0, loc, self.bogeyAxis), self.minPitch, self.maxPitch)
            else:
                self.bogeyAngle = self.restPitch
            self.bogeyRot = quat_mul(n_angle_axis(self.bogeyAngle, self.bogeyAxis), self.bogeyRot0)


def bearing_degrees(v1, v2, up):
    if v3_approx_eq(v1, v2):
        return 0.0
    return st(m_acos(v3_dot(v1, v2)) * m_sign(v3_dot(v3_cross(v1, v2), up)) * f32c(57.29578))


# ----------------------------------------------------------------------------------------------------------------------- SuspensionLoadBalancer
def load_balancer(mods, vessel_mass, landed, gee, grav_mult, multiplier, min_mass):
    """FixedUpdate over a list of (suspension, grounded) pairs. mods = list of dict(grounded=bool, has_wheel=bool); returns sprungMassGravity per module."""
    num = float(vessel_mass)
    if landed:
        num = float(round(num * grav_mult * gee * 10.0)) / 10.0
    n2 = sum(1 for m in mods if m["has_wheel"] and m["grounded"])
    out = []
    for m in mods:
        if m["has_wheel"]:
            num3 = st(m_round(st(100.0 / (float(n2) if float(n2) > 0.0 else 1.0))) / 100.0)
            out.append(m_max(st(num * num3 * multiplier), min_mass))
        else:
            out.append(None)
    return out


# ----------------------------------------------------------------------------------------------------------------------- scenario generation
def arr(v):
    return [x for x in v]


def snap(part):
    """The per-tick observation (the C++ test builds the same dict)."""
    b = part.base
    wh = b.wheel
    o = {}
    o["wheel"] = [wh.maxDriveTorque, wh.maxBrakeTorque, wh.maxSteerAngle, wh.maxRpm, wh.wheelDamping, wh.driveInput, wh.brakeInput, wh.steerInput,
                  wh.steeringResponse, wh.driveResponse, wh.brakeResponse, wh.minimumdownForce, float(wh.integrationSteps),
                  wh.adherent[0], wh.adherent[1], wh.peak[0], wh.peak[1], wh.limit[0], wh.limit[1], 1.0 if wh.enabled else 0.0,
                  1.0 if wh.is_grounded else 0.0]
    c = wh.col
    o["col"] = [c.springRate, c.damperRate, c.suspensionDistance, c.suspensionAnchor, c.suspensionOffset, c.radius, c.mass,
                c.springSlerpRate, 1.0 if c.updateSuspension else 0.0, 1.0 if c.updateCaliper else 0.0, 1.0 if c.updateWheel else 0.0,
                c.center[0], c.center[1], c.center[2], float(c.layerMask)]
    o["base"] = [1.0 if b.isGrounded else 0.0, b.frictionMultiplier, b.geeBias, b.minimumDownforce, b.slip[0], b.slip[1],
                 float(b.inop.types), 1.0 if part.groundContact else 0.0, 1.0 if part.w.landed else 0.0, float(len(part.w.groundContacts)),
                 1.0 if b.rbBrakeConstraints else 0.0]
    for m in part.mods:
        if isinstance(m, Motor):
            o["motor"] = [m.driveOutput, m.driveLimiter, float(m.state), m.resourceFraction, m.driveInput, m.maxTorque,
                          m.wDot if isinstance(m, MotorSteering) else 0.0, 1.0 if m.motorEnabled else 0.0, part.w.ec]
        elif isinstance(m, Steering):
            o["steer"] = [m.steeringInput, m.steerAngle, m.steeringRange, m.angleTweakable, m.responseTweakable, m.CoMfwdLength,
                          1.0 if m.caliperUpdating else 0.0, m.h, m.tPivot[0], m.tPivot[1], m.tPivot[2], m.tCenter[0], m.tCenter[1], m.tCenter[2]]
        elif isinstance(m, Brakes):
            o["brakes"] = [m.brakeInput, m.brakeTweakable, 1.0 if m.light_on else 0.0, float(len(m.light_log))]
        elif isinstance(m, Suspension):
            o["susp"] = [m.spring, m.damper, m.springTweakable, m.damperTweakable, m.autoBoost, m.boost, m.suspCompression, m.suspMass,
                         m.sprungMassGravity, 1.0 if m.highGeeOverride else 0.0, 1.0 if m.autoSpringDamperSet else 0.0, m.springClampMax,
                         m.damperClampMax, m.damperFudge, 1.0 if m.autoSpringDamper else 0.0]
        elif isinstance(m, Damage):
            o["damage"] = [m.currentDeflection, m.currentSlip, m.currentDownForce, m.totalStress, m.stressPercent, 1.0 if m.isDamaged else 0.0,
                           m.startupTime, m.stressTime, m.stressVariability, m.repairImmunityTimeLeft, m.repairImmunityTimeTotal,
                           float(m.repairKitsNecessary)]
        elif isinstance(m, Deployment):
            o["deploy"] = [m.position, m.Position(), 1.0 if m.subsystemsDisabled else 0.0, m.anim.normalized_time, float(m.anim.speed)]
            o["deployState"] = m.stateString
        elif isinstance(m, Lock):
            o["lock"] = [m.maxTorque]
        elif isinstance(m, Bogey):
            o["bogey"] = [m.bogeyAngle, m.bogeyUp[0], m.bogeyUp[1], m.bogeyUp[2], m.bogeyRot[0], m.bogeyRot[1], m.bogeyRot[2], m.bogeyRot[3]]
    o["trace"] = list(part.w.trace)
    part.w.trace.clear()
    return o


def jf(v):
    """JSON float: finite as is, specials as strings (the C++ loader decodes them)."""
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
    return v


def jfy(o):
    if isinstance(o, dict):
        return {k: jfy(v) for k, v in o.items()}
    if isinstance(o, (list, tuple)):
        return [jfy(x) for x in o]
    return jf(o)


def make_kinds():
    """Collider kinds (index -> descriptor): 1 terrain with a tag (layer 15), 2 terrain 'Untagged', 3 terrain with no tag, 4 a part of another landed
    vessel (layer 0), 5 a part of an airborne vessel, 6 a dynamic rigidbody terrain with a crash-name component, 7 a ROC boulder."""
    return {
        0: {"layer": 0, "tag": "", "rbVel": None},
        1: {"layer": 15, "tag": "Runway", "rbVel": None, "crash": False},
        2: {"layer": 15, "tag": "Untagged", "rbVel": None, "crash": True},
        3: {"layer": 15, "tag": "", "rbVel": None},
        4: {"layer": 0, "tag": "", "rbVel": (0.0, st(0.5), 0.0), "otherVessel": {"landedOrSplashed": True, "landedAt": "KSC", "display": "KSC Pad", "landed": True}},
        5: {"layer": 0, "tag": "", "rbVel": (st(1.5), 0.0, 0.0), "otherVessel": {"landedOrSplashed": False, "landedAt": "", "display": "", "landed": False}},
        6: {"layer": 15, "tag": "Grass", "rbVel": (0.0, 0.0, st(-2.0)), "crash": True},
        7: {"layer": 15, "tag": "ROC", "rbVel": None, "roc": "ROC_Rock", "crash": False},
    }


def rnd32(rng, lo, hi):
    return st(rng.uniform(lo, hi))


def gen_run(parts, name, gee, rate, seed, nticks):
    """One scripted run of a stock wheel part: all its modules driven for nticks ticks. Returns the run record (inputs + expected outputs)."""
    rng = random.Random(seed)
    part_node, mod_nodes = parts[name]
    world = World(gee, rate, seed + 7)
    world.dt = st(0.6) if seed % 2 == 1 else st(0.02)    # long ticks let the 30-90 s repair immunity run out inside a run
    world.fdt = world.dt
    world.refTr = Tr(V3Z, rand_quat(rng))
    world.CoM = tuple(rnd32(rng, -2, 2) for _ in range(3))
    world.upAxis = tuple(st(c) for c in (rnd32(rng, -1, 1), rnd32(rng, 0.2, 1), rnd32(rng, -1, 1)))
    world.gravityForPos = (0.0, st(-9.81 * gee), 0.0)
    world.totalMass = st(rng.uniform(1.0, 25.0))
    world.landed = False
    ec0 = rng.uniform(0.5, 12.0)
    world.ec = ec0
    part = PartM(world, name, part_node, mod_nodes, rng)
    world.parts = [part]
    part.tr = Tr(tuple(rnd32(rng, -3, 3) for _ in range(3)), rand_quat(rng))
    part.rb_vel = tuple(rnd32(rng, -2, 2) for _ in range(3))
    base = part.base
    base.wheelTrPos = tuple(rnd32(rng, -3, 3) for _ in range(3))
    base.wheelTrRot = rand_quat(rng)
    base.driftCorrection = (seed % 3 == 0)
    bogey = part.find(Bogey)
    if bogey is not None:
        bogey.parentRot = rand_quat(rng)
        bogey.refLocalRot = rand_quat(rng)
    dep = part.find(Deployment)
    sample_line = None
    if dep is not None:
        dep.shielded = False
        dep.animLen = rnd32(rng, 0.5, 3.0)
        ax = tuple(rnd32(rng, -1, 1) for _ in range(3))
        sample_line = {"axis": ax}
        dep.sample_axis = ax
    dmg = part.find(Damage)
    stress_tol = impact_tol = None
    if dmg is not None:
        if seed % 2 == 1:
            stress_tol = st(dmg.stressTolerance * 0.002)
            dmg.stressTolerance = stress_tol
        if seed % 3 == 1:
            impact_tol = st(dmg.impactTolerance * 0.25)
            dmg.impactTolerance = impact_tol
    rec = {"stressTol": stress_tol, "impactTol": impact_tol, "part": name, "gee": gee, "rate": rate, "seed": seed, "ec0": ec0, "dt": world.dt, "fdt": world.fdt,
           "refRot": arr(world.refTr.rot), "CoM": arr(world.CoM), "upAxis": arr(world.upAxis), "gravityForPos": arr(world.gravityForPos),
           "totalMass": world.totalMass, "partPos": arr(part.tr.pos), "partRot": arr(part.tr.rot), "rbVel": arr(part.rb_vel),
           "wheelTrPos": arr(base.wheelTrPos), "wheelTrRot": arr(base.wheelTrRot), "sampleLine": jfy(sample_line) if sample_line else None,
           "bogeyParentRot": arr(bogey.parentRot) if bogey else None, "bogeyRefLocalRot": arr(bogey.refLocalRot) if bogey else None,
           "brakesGroup": False, "drift": base.driftCorrection, "animLen": dep.animLen if dep else None, "ticks": []}
    # OnStart of every module in cfg order while Part.Start has not finished (part.started false: the base's WaitUntil coroutine waits), then
    # Part.Start ends and the next frame's coroutine pass runs wheelSetup
    part.started = False
    for m in part.mods:
        if m is not None and hasattr(m, "on_start"):
            m.on_start()
    part.started = True
    world.sched.advance_frame(world.dt)
    world.sched.dynamic_pass()
    rec["afterStart"] = snap_setup(part)
    # scripted state profile
    mode = 0
    seg = 0
    cur = {}
    for t in range(nticks):
        if seg == 0:
            seg = rng.randint(8, 40)
            mode = rng.randint(0, 5)
            cur = {
                "grounded": rng.random() < 0.7,
                "angVel": rnd32(rng, -60, 60) if mode != 0 else 0.0,
                "lwv": (rnd32(rng, -3, 3), rnd32(rng, -12, 12) if mode != 0 else 0.0),
                "comp": rnd32(rng, 0.0, 1.2),
                "side": rnd32(rng, -1.0, 1.0),
                "normal": tuple(st(c) for c in (rnd32(rng, -0.3, 0.3), rnd32(rng, 0.7, 1.0), rnd32(rng, -0.3, 0.3))),
                "down": rnd32(rng, 0.0, 25.0) if rng.random() < 0.92 else rnd32(rng, 0.0, 300.0),
                "depth": rnd32(rng, 0.0, 0.25),
                "spring": st(rnd32(rng, 100.0, 5000.0) * (rng.choice([5.0, 40.0]) if rng.random() < 0.12 else 1.0)),
                "coll": rng.choice([1, 1, 2, 3, 4, 5, 6, 7]),
                "ctrl": (rng.choice([0.0, 1.0, -1.0, rnd32(rng, -1, 1)]), rng.choice([0.0, rnd32(rng, -0.2, 0.2)]),
                         rng.choice([0.0, 1.0, -1.0, rnd32(rng, -1, 1)]), rng.choice([0.0, rnd32(rng, -0.2, 0.2)])),
                "speed": rnd32(rng, 0.0, 30.0) if mode % 2 else rnd32(rng, 0.0, 0.2),
                "landed_srf": rnd32(rng, 0.0, 0.02),
            }
        seg -= 1
        s = cur
        tick = {"grounded": s["grounded"], "angVel": s["angVel"], "lwv": arr(s["lwv"]), "comp": s["comp"], "side": s["side"], "normal": arr(s["normal"]),
                "down": s["down"], "depth": s["depth"], "spring": s["spring"], "coll": s["coll"] if s["grounded"] else rng.choice([0, 0, 1]),
                "ctrl": arr(s["ctrl"]), "srfSpeed": s["speed"] if rng.random() < 0.9 else s["landed_srf"], "events": [], "ecSet": None,
                "rate": rate}
        if rng.random() < 0.04:
            tick["ecSet"] = rnd32(rng, 0.0, 6.0)
        # events
        if rng.random() < 0.05:
            evs = ["brake", "motor", "gear", "friction", "steer", "susp", "damage", "pack", "soi", "dock", "undock", "suspEnable", "brakeAction", "tractionOff",
                   "startup0", "startup0", "startup0", "startup0"]
            for _ in range(rng.randint(1, 2)):
                e = rng.choice(evs)
                tick["events"].append([e, rng.choice([ACT_ACTIVATE, ACT_DEACTIVATE, ACT_TOGGLE]), rnd32(rng, 0.1, 2.0)])
        rec["ticks"].append(tick)
        out = step_tick(world, part, tick, sample_line)
        rec.setdefault("out", []).append(out)
    return rec


def snap_setup(part):
    return snap(part)


def apply_event(world, part, ev):
    name, typ, val = ev
    b = part.base
    br = part.find(Brakes)
    mo = part.find(Motor)
    de = part.find(Deployment)
    st_ = part.find(Steering)
    su = part.find(Suspension)
    da = part.find(Damage)
    if name == "brake" and br is not None:
        br.brake_action(typ)
    elif name == "brakeAction" and br is not None:
        br.brakeInput = 0.5 if val > 1.0 else br.brakeInput
    elif name == "motor" and mo is not None:
        if typ == ACT_TOGGLE:
            mo.motorEnabled = not mo.motorEnabled
        elif typ == ACT_ACTIVATE:
            mo.motorEnabled = True
        else:
            mo.motorEnabled = False
    elif name == "tractionOff" and mo is not None:
        mo.autoTorque = not mo.autoTorque
    elif name == "gear" and de is not None:
        if typ == ACT_TOGGLE and val > 1.0:
            de.toggle_deployment()
        else:
            de.action_toggle(typ)
    elif name == "friction":
        if val > 1.0:
            b.evt_auto_friction_toggle()
        else:
            b.act_auto_friction_toggle(typ)
    elif name == "steer" and st_ is not None:
        st_.steeringEnabled = (typ == ACT_ACTIVATE) or (typ == ACT_TOGGLE and not st_.steeringEnabled)
    elif name == "susp" and su is not None:
        su.toggle_auto_spring_damper()
    elif name == "suspEnable":
        b.suspensionEnabled = (typ == ACT_ACTIVATE)
    elif name == "startup0" and da is not None:
        da.startupTime = 0.0
    elif name == "damage" and da is not None:
        if typ == ACT_DEACTIVATE:
            da.repair()
        else:
            da.set_damaged(True)
    elif name == "pack":
        if typ == ACT_DEACTIVATE:
            part.packed = False
            b.on_unpack()
        else:
            part.packed = True
            b.on_pack()
    elif name == "soi":
        world.gee = float(st(val))
        b.on_soi(val)
    elif name == "dock":
        b.on_docking()
    elif name == "undock":
        b.on_undocking()


def step_tick(world, part, tick, sample_line):
    w = world
    w.rate = tick["rate"]
    w.sched.advance_frame(w.dt)
    w.ctrl = tuple(tick["ctrl"])
    w.srfSpeed = tick["srfSpeed"]
    if tick["ecSet"] is not None:
        w.ec = tick["ecSet"]
    for ev in tick["events"]:
        apply_event(w, part, ev)
    # Update phase
    w.sched.dynamic_pass()
    for m in part.mods:
        if isinstance(m, Deployment):
            m.update()
    b = part.base
    wh = b.wheel
    if wh is not None:
        s = wh.state
        s.grounded = tick["grounded"]
        s.angularVelocity = tick["angVel"]
        s.lwv = tuple(tick["lwv"])
        s.compression = tick["comp"]
        s.sidewaysSlip = tick["side"]
        s.normal = tuple(tick["normal"])
        s.downforce = tick["down"]
        s.contactDepth = tick["depth"]
        s.collider = tick["coll"]
        wh.effSpring = tick["spring"]
    # FixedUpdate phase (cfg order)
    for m in part.mods:
        if m is None:
            continue
        m.fixed_update() if hasattr(m, "fixed_update") else None
    # animation
    for m in part.mods:
        if isinstance(m, Deployment):
            m.anim.tick(w.fdt)
            m.sample()
    # LateUpdate
    for m in part.mods:
        if m is not None and hasattr(m, "late_update"):
            m.late_update()
    # PartValues.Update: SteeringRadius (GetCoMFwdLength of every steering module), cached vessel value for the next tick
    for m in part.mods:
        if isinstance(m, Steering):
            w.steeringRadiusValue = m_max(1.0, m.get_com_fwd_length())
    if b.wheel is None:
        return {"none": True}
    return snap(part)


# ----------------------------------------------------------------------------------------------------------------------- the other families
def gee_bias_rows(parts):
    rows = []
    for name in sorted(parts):
        pn, mn = parts[name]
        for gee in (0.05, 0.166, 0.99, 1.6, 9.0):
            for auto in (True, False):
                for new_model in (None, True, False):
                    for gb in (None, 1.6, 2.0):
                        for has_wheel in (False, True):
                            world = World(1.0, 1, 1)
                            part = PartM(world, name, pn, mn, None)
                            b = part.base
                            b.autoFriction = auto
                            if new_model is not None:
                                b.useNewFrictionModel = new_model
                            if gb is not None:
                                b.geeBias = st(gb)
                            if has_wheel:
                                b.wheel = Wheel()
                            ret = b.apply_gee_bias(st(gee))
                            r = {"part": name, "gee": gee, "auto": auto, "newModel": b.useNewFrictionModel, "geeBias0": gb, "hasWheel": has_wheel,
                                 "ret": ret, "geeBias": b.geeBias, "minDown": b.minimumDownforce, "fm": b.frictionMultiplier}
                            if has_wheel:
                                wh = b.wheel
                                r["tf"] = [wh.adherent[0], wh.adherent[1], wh.peak[0], wh.peak[1], wh.limit[0], wh.limit[1]]
                            rows.append(r)
    return rows


def util_rows():
    rng = random.Random(5)
    rows = []
    for _ in range(300):
        speed = rnd32(rng, -50, 1200)
        rpm = rnd32(rng, -500, 5000)
        r = rnd32(rng, 0.05, 2.0)
        rows.append({"speed": speed, "rpm": rpm, "r": r, "toRpm": speed_to_rpm(speed, r), "toSpeed": rpm_to_speed(rpm, r), "toDegs": rpm_to_degs_sec(rpm)})
    return rows


def balancer_rows():
    rng = random.Random(9)
    rows = []
    for _ in range(400):
        n = rng.randint(1, 9)
        mods = [{"has_wheel": rng.random() < 0.9, "grounded": rng.random() < 0.7} for _ in range(n)]
        mass = st(rng.uniform(0.5, 60.0))
        landed = rng.random() < 0.6
        gee = rng.choice([0.166, 1.0, 1.6, 0.4])
        gm = rng.choice([1.0, 1.0, 0.5, 2.0])
        mult = st(rng.choice([1.0, 1.0, 0.8, 1.3]))
        mm = st(rng.choice([0.5, 0.5, 2.0]))
        out = load_balancer(mods, mass, landed, gee, gm, mult, mm)
        rows.append({"mods": mods, "mass": mass, "landed": landed, "gee": gee, "gm": gm, "mult": mult, "minMass": mm, "out": out})
    return rows


def put_to_ground_rows(parts):
    rows = []
    rng = random.Random(21)
    for name in sorted(parts):
        pn, mn = parts[name]
        for trial in range(6):
            world = World(1.0, 1, 1)
            part = PartM(world, name, pn, mn, None)
            part.tr = Tr(tuple(rnd32(rng, -5, 5) for _ in range(3)), rand_quat(rng))
            b = part.base
            b.wheelOrgPosR = tuple(rnd32(rng, -1, 1) for _ in range(3))
            if trial % 2:
                b.groundHeightOffset = rnd32(rng, 0.1, 1.5)
            dep = part.find(Deployment)
            retracted = False
            if dep is not None and trial % 3 == 0:
                dep.stateString = "Retracted"
                retracted = True
            q = {"lowest": rnd32(rng, -2, 10), "point": rnd32(rng, -2, 10)}
            q0 = dict(q)
            b.on_put_to_ground(q)
            rows.append({"part": name, "partPos": arr(part.tr.pos), "partRot": arr(part.tr.rot), "orgPos": arr(b.wheelOrgPosR), "offset": b.groundHeightOffset,
                         "retracted": retracted, "radius": b.radius, "lowest0": q0["lowest"], "point0": q0["point"], "lowest": q["lowest"], "point": q["point"]})
    return rows


def main():
    parts = load_parts()
    COLLIDER_KINDS.update(make_kinds())
    out = {"env": ENV, "kinds": jfy(COLLIDER_KINDS), "parts": sorted(parts)}
    out["util"] = jfy(util_rows())
    out["geeBias"] = jfy(gee_bias_rows(parts))
    out["balancer"] = jfy(balancer_rows())
    out["putToGround"] = jfy(put_to_ground_rows(parts))
    runs = []
    seed = 1000
    for name in sorted(parts):
        for gee, rate in ((1.0, 1), (0.166, 2), (1.67, 4)):
            seed += 17
            runs.append(jfy(gen_run(parts, name, gee, rate, seed, 200)))
    out["runs"] = runs
    json.dump(out, sys.stdout, separators=(",", ":"))


if __name__ == "__main__":
    main()
