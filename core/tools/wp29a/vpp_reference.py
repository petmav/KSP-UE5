#!/usr/bin/env python3
"""WP-29a: independent reference for the VehiclePhysics (VPP) wheel core and KSPWheelController -- test data generator.

Written from the decompiled C# ONLY. It was written BEFORE the C++ port existed and shares no code with it; the C++ port was then written
from the same C# (not from this file), so the two agree only where both transcribe the C# correctly. Sources, all under
decompiled/KSP-Decompiled/Assembly-CSharp/:
  VehiclePhysics/TireFriction.cs   (Flat, Lineal, Smooth, Parametric, Pacejka curves, GetForce, the adherent helpers)
  VehiclePhysics/Wheel.cs, Block.cs, DirectDrive.cs, Solver.cs   (the wheel-spin block chain, Euler and RK4 integration)
  VehiclePhysics/VehicleBase.cs    (FixedUpdate: suspension travel / forces, downforce, local frame, tyre force, Update)
  VehiclePhysics/VPWheelCollider.cs (GetGroundHit fix-ups, GetContactDepth, UpdateWheelCollider, SetSuspensionForceOffset, ApplyForce)
  KSPWheelController.cs, KSPWheelGravity.cs
  EdyCommonTools/MathUtility.cs (FastAbs, UnclampedLerp), InterpolatedFloat.cs
  Unity/UnityEngine.CoreModule/UnityEngine/{Mathf,Vector2,Vector3,Quaternion}.cs (the managed parts)

Float32 follows the Mono rule (core/include/ksp/dotnet/mono_float.hpp, CAPTURE-SPEC C7): a float expression is evaluated in double and rounded
(st()) only where the C# stores it in a float local / field, passes it as a float argument, returns it or casts it. Native Unity code
(Transform.up / forward / right, InverseTransformPoint / Direction, Rigidbody.GetPointVelocity, Quaternion.AngleAxis) is per-operation float32
(numpy float32 scalars below). Those natives have no C# source: they are written here from Unity's documented formulation (Runtime/Math/
Quaternion.h RotateVectorByQuat, PhysX getVelocityAtPos, AxisAngleToQuaternionSafe) and are UNVERIFIED against the game (STATUS.md WP-29a).
The native UnityEngine.WheelCollider itself (suspension ray, spring force, WheelHit) is WP-29c: its outputs are scripted per tick here.

Usage: python -I core/tools/wp29a/vpp_reference.py > core/tests/data/wp29a_reference.json
"""
import json
import math
import sys

import numpy as np

np.seterr(all="ignore")
F = np.float32
INF = float("inf")
NAN = float("nan")


def st(x):
    """Mono float store: round a double expression to float32."""
    return float(F(x))


def fdiv(a, b):
    """IEEE double division (no ZeroDivisionError)."""
    if b == 0.0:
        if a == 0.0 or a != a:
            return NAN
        neg = (math.copysign(1.0, a) < 0) != (math.copysign(1.0, b) < 0)
        return -INF if neg else INF
    return a / b


def fsqrt(x):
    return math.sqrt(x) if x >= 0.0 else NAN


# ----------------------------------------------------------------------------------------------------------------------- Mathf (managed)
EPS_MATHF = 1.401298464324817e-45  # Mathf.Epsilon (float denormal min)
C1E5 = st(1e-5)    # 1E-05f
C1E6 = st(1e-6)    # 1E-06f
C1E15 = st(1e-15)  # 1E-15f


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


def m_lerp(a, b, t):
    return st(a + (b - a) * m_clamp01(t))


def m_inverse_lerp(a, b, v):
    if a != b:
        return m_clamp01(st(fdiv(v - a, b - a)))
    return 0.0


def m_move_towards(cur, tgt, maxd):
    if abs(st(tgt - cur)) <= maxd:
        return tgt
    return st(cur + m_sign(st(tgt - cur)) * maxd)


def m_sqrt(f):
    return st(fsqrt(f))


def m_atan(f):
    return st(math.atan(f))


def m_sin(f):
    return st(math.sin(f))


def m_cos(f):
    return st(math.cos(f))


def m_atan2(y, x):
    return st(math.atan2(y, x))


def m_max(a, b):
    return a if a > b else b


def m_min(a, b):
    return a if a < b else b


def fast_abs(x):
    return x if not (x < 0.0) else st(0.0 - x)


def unclamped_lerp(frm, to, t):
    return st(frm + (to - frm) * t)


# ----------------------------------------------------------------------------------------------------------------------- Vector2 / Vector3 (managed)
def v2_mag(v):
    return st(fsqrt(v[0] * v[0] + v[1] * v[1]))


def v2_mul(v, d):
    return (st(v[0] * d), st(v[1] * d))


def v2_div(v, d):
    return (st(fdiv(v[0], d)), st(fdiv(v[1], d)))


def v2_add(a, b):
    return (st(a[0] + b[0]), st(a[1] + b[1]))


def v3_add(a, b):
    return (st(a[0] + b[0]), st(a[1] + b[1]), st(a[2] + b[2]))


def v3_sub(a, b):
    return (st(a[0] - b[0]), st(a[1] - b[1]), st(a[2] - b[2]))


def v3_mul(a, d):
    return (st(a[0] * d), st(a[1] * d), st(a[2] * d))


def v3_div(a, d):
    return (st(fdiv(a[0], d)), st(fdiv(a[1], d)), st(fdiv(a[2], d)))


def v3_neg(a):  # the UnityEngine.CoreModule IL is an IEEE negation
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
    if n > C1E5:
        return v3_div(v, n)
    return (0.0, 0.0, 0.0)


def v3_project(v, n):
    num = v3_dot(n, n)
    if num < EPS_MATHF:
        return (0.0, 0.0, 0.0)
    num2 = v3_dot(v, n)
    return (st(fdiv(n[0] * num2, num)), st(fdiv(n[1] * num2, num)), st(fdiv(n[2] * num2, num)))


def v3_angle(frm, to):
    num = st(fsqrt(v3_sqrmag(frm) * v3_sqrmag(to)))
    if num < C1E15:
        return 0.0
    num2 = m_clamp(st(fdiv(v3_dot(frm, to), num)), -1.0, 1.0)
    return st(st(math.acos(num2)) * st(57.29578))


def v3_lerp(a, b, t):
    t = m_clamp01(t)
    return (st(a[0] + (b[0] - a[0]) * t), st(a[1] + (b[1] - a[1]) * t), st(a[2] + (b[2] - a[2]) * t))


def v3_approx_eq(l, r):  # Vector3.operator==
    n = st(l[0] - r[0])
    n2 = st(l[1] - r[1])
    n3 = st(l[2] - r[2])
    return st(n * n + n2 * n2 + n3 * n3) < st(9.9999994e-11)


def quat_mul_vec(q, p):
    """Quaternion.operator*(Quaternion, Vector3) (managed, Mono rule)."""
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


# ----------------------------------------------------------------------------------------------------------------------- natives (per-op float32)
def n_rotate(q, v):
    """Native RotateVectorByQuat (Runtime/Math/Quaternion.h): float32 per operation."""
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


def n_conj(q):
    return (-q[0], -q[1], -q[2], q[3])


def n_up(q):
    return n_rotate(q, (0.0, 1.0, 0.0))


def n_forward(q):
    return n_rotate(q, (0.0, 0.0, 1.0))


def n_right(q):
    return n_rotate(q, (1.0, 0.0, 0.0))


def n_inverse_transform_point(pos, q, p):
    d = (float(F(p[0]) - F(pos[0])), float(F(p[1]) - F(pos[1])), float(F(p[2]) - F(pos[2])))
    return n_rotate(n_conj(q), d)


def n_inverse_transform_direction(q, d):
    return n_rotate(n_conj(q), d)


def n_cross(a, b):
    a0, a1, a2, b0, b1, b2 = F(a[0]), F(a[1]), F(a[2]), F(b[0]), F(b[1]), F(b[2])
    return (float(a1 * b2 - a2 * b1), float(a2 * b0 - a0 * b2), float(a0 * b1 - a1 * b0))


def n_angle_axis(angle, axis):
    """Quaternion.AngleAxis (native AxisAngleToQuaternionSafe); the Deg2Rad operation order is the capture-verified one (math_shim_native.json)."""
    rad = float(F(float(F(F(angle) / F(360.0)) * F(2.0)) * math.pi))
    ax, ay, az = F(axis[0]), F(axis[1]), F(axis[2])
    mag = F(math.sqrt(float(ax * ax + ay * ay + az * az)))
    if mag > F(1e-6):
        half = F(F(rad) * F(0.5))
        w = F(math.cos(float(half)))
        s = F(math.sin(float(half))) / mag
        return (float(s * ax), float(s * ay), float(s * az), float(w))
    return (0.0, 0.0, 0.0, 1.0)


class Rb:
    """UnityEngine.Rigidbody: the state the wheel code reads and the force calls it makes."""

    def __init__(self, mass=1.0, vel=(0, 0, 0), ang=(0, 0, 0), com=(0, 0, 0), kinematic=False):
        self.mass = mass
        self.velocity = vel
        self.angvel = ang
        self.com = com
        self.kinematic = kinematic
        self.forces = []
        self.maxDepen = 0.0
        self.maxAng = 0.0

    def point_velocity(self, p):  # native: v + w x (p - com), float32 per operation
        d = (float(F(p[0]) - F(self.com[0])), float(F(p[1]) - F(self.com[1])), float(F(p[2]) - F(self.com[2])))
        c = n_cross(self.angvel, d)
        return (float(F(self.velocity[0]) + F(c[0])), float(F(self.velocity[1]) + F(c[1])), float(F(self.velocity[2]) + F(c[2])))

    def add_force(self, f, p):
        self.forces.append((f, p))


# ----------------------------------------------------------------------------------------------------------------------- TireFriction.cs
class Settings:
    def __init__(self, d=None):
        d = d or {}
        self.adherent = tuple(d.get("adherent", (st(0.5), st(0.95))))
        self.peak = tuple(d.get("peak", (st(1.5), st(1.1))))
        self.limit = tuple(d.get("limit", (4.0, st(0.8))))
        self.a = d.get("a", 0.0)
        self.b = d.get("b", 0.0)
        self.c = d.get("c", st(0.5))
        self.d = d.get("d", 0.0)
        self.A = d.get("A", st(0.95))
        self.B = d.get("B", st(0.8))
        self.C = d.get("C", st(1.5))
        self.D = d.get("D", st(1.1))
        self.E = d.get("E", -2.0)

    def mutate(self, d):
        """Field-wise assignment (`settings.adherent.x = v`): the curve built over this object sees it, nothing is re-constrained."""
        for k, v in d.items():
            if k in ("adherent", "peak", "limit"):
                setattr(self, k, tuple(v))
            else:
                setattr(self, k, v)

    def apply_apl(self):  # Settings.ApplyAPLConstraints
        self.adherent = (m_max(st(0.01), self.adherent[0]), self.adherent[1])
        self.peak = (m_max(self.adherent[0], self.peak[0]), self.peak[1])
        self.limit = (m_max(self.peak[0], self.limit[0]), self.limit[1])
        self.adherent = (self.adherent[0], m_max(0.0, self.adherent[1]))
        self.peak = (self.peak[0], m_max(0.0, self.peak[1]))
        self.limit = (self.limit[0], m_max(0.0, self.limit[1]))
        self.adherent = (self.adherent[0], m_min(self.peak[1], self.adherent[1]))
        self.limit = (self.limit[0], m_min(self.peak[1], self.limit[1]))


class Patch:  # TireFriction.ContactPatch
    def __init__(self):
        self.slip = (0.0, 0.0)
        self.load = 1.0
        self.groundGrip = 1.0


def fast_lerp(P0, P1, x):
    return st(P0[1] + fdiv((x - P0[0]) * (P1[1] - P0[1]), (P1[0] - P0[0])))


def cubic_lerp(x, P0, P1):
    num = st(fdiv(x - P0[0], P1[0] - P0[0]))
    num2 = st(num * num)
    num3 = st(num * num2)
    return st(P0[1] * (2.0 * num3 - 3.0 * num2 + 1.0) + P1[1] * (-2.0 * num3 + 3.0 * num2))


def tangent_lerp(x, P0, P1, a, b):
    num = st(P1[1] - P0[1])
    num2 = st(3.0 * num * a)
    num3 = st(3.0 * num * b)
    num4 = st(fdiv(x - P0[0], P1[0] - P0[0]))
    num5 = st(num4 * num4)
    num6 = st(num4 * num5)
    return st(P0[1] * (2.0 * num6 - 3.0 * num5 + 1.0) + P1[1] * (-2.0 * num6 + 3.0 * num5) + num2 * (num6 - 2.0 * num5 + num4) + num3 * (num6 - num5))


def pacejka(slip, B, C, D, E):
    num = st(B * slip)
    return st(D * m_sin(st(C * m_atan(st(num - E * (num - m_atan(num)))))))


class Curve:
    def __init__(self, s):
        self.p = s


class Flat(Curve):
    def __init__(self, s):
        Curve.__init__(self, s)
        s.adherent = (m_max(st(0.01), s.adherent[0]), s.adherent[1])
        s.adherent = (s.adherent[0], m_max(0.0, s.adherent[1]))

    def eval(self, s, cp):
        return st(self.p.adherent[1] * cp.load * cp.groundGrip)

    def adh(self, cp):
        return self.p.adherent[0]

    def peak(self, cp):
        return self.p.adherent[0]

    def lim(self, cp):
        return self.p.adherent[0]


class Piecewise(Curve):
    def __init__(self, s):
        Curve.__init__(self, s)
        s.apply_apl()

    def eval(self, s, cp):
        num = st(cp.load * cp.groundGrip)
        p = self.p
        if s <= p.adherent[0]:
            return st(num * p.adherent[1])
        if s < p.peak[0]:
            return st(num * self.seg(s, p.adherent, p.peak, True))
        if s < p.limit[0]:
            return st(num * self.seg(s, p.peak, p.limit, False))
        return st(num * p.limit[1])

    def adh(self, cp):
        return self.p.adherent[0]

    def peak(self, cp):
        return self.p.peak[0]

    def lim(self, cp):
        return self.p.limit[0]


class Lineal(Piecewise):
    def seg(self, s, P0, P1, first):
        return fast_lerp(P0, P1, s)


class Smooth(Piecewise):
    def seg(self, s, P0, P1, first):
        return cubic_lerp(s, P0, P1)


class Parametric(Piecewise):
    def seg(self, s, P0, P1, first):
        if first:
            return tangent_lerp(s, P0, P1, self.p.a, self.p.b)
        return tangent_lerp(s, P0, P1, self.p.c, self.p.d)


class Pacejka(Curve):
    def __init__(self, s):
        Curve.__init__(self, s)
        s.A = m_max(s.A, st(0.01))
        s.D = m_max(s.D, 0.0)
        s.B = m_max(s.B, st(0.1))
        s.C = m_clamp(s.C, 1.0, 2.0)
        s.E = m_min(s.E, 1.0)
        self.a = (0.0, 0.0)
        self.pk = (0.0, 0.0)
        self.l = (0.0, 0.0)
        self.points(s.A, s.B, s.C, s.D, s.E)

    def points(self, A, B, C, D, E):
        flag = flag2 = flag3 = False
        num = 0.0
        num2 = st(0.01)
        num3 = 15.0
        num4 = pacejka(num, B, C, D, E)
        adh = (0.0, 0.0)
        peak = (0.0, 0.0)
        lim = (0.0, 0.0)
        while num < num3:
            num5 = pacejka(st(num + num2), B, C, D, E)
            if not flag and A >= num4 and A < num5:
                ax = m_lerp(num, st(num + num2), m_inverse_lerp(num4, num5, A))
                adh = (ax, pacejka(ax, B, C, D, E))
                flag = True
            if not flag2 and num5 <= num4:
                peak = (num, num4)
                flag2 = True
            if (not flag2) or not (num > peak[0] + fdiv(1.0, B)) or fdiv(fast_abs(st(num5 - num4)), num2) >= st(0.01):
                num = st(num + num2)
                num4 = num5
                continue
            lim = (st(num + num2), lim[1])
            flag3 = True
            break
        if not flag3:
            lim = (num3, lim[1])
        lim = (lim[0], pacejka(lim[0], B, C, D, E))
        if not flag2:
            peak = lim
        if not flag:
            adh = peak if flag2 else lim
        self.a, self.pk, self.l = adh, peak, lim

    def eval(self, s, cp):
        num = st(cp.load * cp.groundGrip)
        if s <= self.a[0]:
            return st(num * self.a[1])
        if s > self.l[0]:
            return st(num * self.l[1])
        return st(num * pacejka(s, self.p.B, self.p.C, self.p.D, self.p.E))

    def adh(self, cp):
        return self.a[0]

    def peak(self, cp):
        return self.pk[0]

    def lim(self, cp):
        return self.l[0]


MODELS = ["flat", "lineal", "smooth", "parametric", "pacejka"]  # TireFriction.Model: Flat=0, Lineal=1, Smooth=2, Parametric=3, Pacejka=4
CURVE_CLASS = {"flat": Flat, "lineal": Lineal, "smooth": Smooth, "parametric": Parametric, "pacejka": Pacejka}


class TireFriction:
    def __init__(self):
        self.model = "smooth"
        self.settings = Settings()
        self.frictionMultiplier = 1.0
        self.curve = None
        self.setup()

    def setup(self):
        self.curve = CURVE_CLASS[self.model](self.settings)

    def get_force(self, cp):
        slip = cp.slip
        magnitude = v2_mag(slip)
        if magnitude < C1E6:
            return (0.0, 0.0)
        slip = (st(fdiv(slip[0], magnitude)), st(fdiv(slip[1], magnitude)))
        d = st(0.0 - self.curve.eval(magnitude, cp))
        return v2_mul(v2_mul(slip, d), self.frictionMultiplier)

    def is_adherent(self, cp):
        return v2_mag(cp.slip) < self.curve.adh(cp)

    def adherent_forward(self, cp):
        x = cp.slip[0]
        a = self.curve.adh(cp)
        if not (fast_abs(x) < a):
            return 0.0
        return m_sqrt(st(a * a - x * x))

    def adherent_sideways(self, cp):
        y = cp.slip[1]
        a = self.curve.adh(cp)
        if not (fast_abs(y) < a):
            return 0.0
        return m_sqrt(st(a * a - y * y))

    def adherent_force(self, cp):
        a = self.curve.adh(cp)
        d = st(0.0 - fdiv(self.curve.eval(a, cp) * self.frictionMultiplier, a))
        return v2_mul(cp.slip, d)


# ----------------------------------------------------------------------------------------------------------------------- Block / Wheel / DirectDrive / Solver
RpmToW = st(F(math.pi) / F(30.0))
MIN_INERTIA = st(0.01)


class Conn:
    def __init__(self):
        self.L = 0.0
        self.I = 0.0
        self.Tr = 0.0
        self.outTd = 0.0
        self.output = None


class SolverStatic:
    time = 0.0
    deltaTime = 0.0


def solve_sym(Tsum, T, Tmax):
    flag = Tsum >= 0.0
    flag2 = Tmax >= 0.0
    num = Tsum if flag else st(0.0 - Tsum)
    num2 = Tmax if flag2 else st(0.0 - Tmax)
    if Tmax == 0.0 or (flag == flag2 and num > num2):
        t = m_inverse_lerp(0.0, st(Tsum * 0.5), T)
        Tsum = unclamped_lerp(Tsum, Tmax, t)
    return Tsum


class Wheel:
    def __init__(self):
        self.tireFriction = TireFriction()
        self.radius = 0.0
        self.mass = 0.0
        self.I = 0.0
        self.invI = 0.0
        self.load = 0.0
        self.grip = 0.0
        self.V = (0.0, 0.0)
        self.Fext = (0.0, 0.0)
        self.brake = 0.0
        self.L = 0.0
        self.P = (0.0, 0.0)
        self.w = 0.0
        self.T = 0.0
        self.F = (0.0, 0.0)
        self.Tr = 0.0
        self.isResting = False
        self.fslip = 0.0
        self.tireForce = (0.0, 0.0)
        self.Ty = 0.0
        self.isAdherent = False
        self.adhSlip = (0.0, 0.0)
        self.adhForce = (0.0, 0.0)
        self.inTd = 0.0
        self.inTb = 0.0
        self.contact = Patch()
        self.inp = None  # inputs[0]

    def recalc_constants(self):
        self.I = st(self.mass * self.radius * self.radius * 0.5)
        if self.I < MIN_INERTIA:
            self.I = MIN_INERTIA
        self.invI = st(fdiv(1.0, self.I))

    def add_brake(self, t):
        self.brake = st(self.brake + t)
        if self.brake < 0.0:
            self.brake = 0.0

    def recalc_vars(self):
        self.w = st(self.L * self.invI)

    def get_state(self):  # Block.State = [L, P(x,y), Lr]
        self.isResting = True
        return [self.L, (0.0, 0.0), 0.0]

    def set_substep_state(self, S):
        self.L = S[0]
        self.P = S[1]
        self.recalc_vars()

    def compute_state_upstream(self):
        self.fslip = st(self.w * self.radius - self.V[1])
        slip = (self.V[0], self.fslip)
        c = self.contact
        c.load = self.load
        c.slip = slip
        c.groundGrip = self.grip
        tf = self.tireFriction
        self.tireForce = tf.get_force(c)
        self.Ty = st(self.tireForce[1] * self.radius)
        self.isAdherent = tf.is_adherent(c)
        if self.isAdherent:
            self.adhSlip = (slip[0], tf.adherent_forward(c))
            c.slip = self.adhSlip
            c.slip = (c.slip[0], st(c.slip[1] * m_sign(slip[1])))
            self.adhForce = tf.adherent_force(c)
            num = st(self.adhForce[1] * self.radius)
            if fast_abs(self.Ty) < fast_abs(num):
                self.Ty = num
        else:
            self.adhSlip = (0.0, 0.0)
            self.adhForce = (0.0, 0.0)
        if self.inp is not None:
            self.inp.L = self.L
            self.inp.I = self.I
            self.inp.Tr = self.Tr

    def eval_torque_downstream(self):
        self.inTd = self.inp.outTd if self.inp is not None else 0.0
        self.inTb = self.brake
        num = st(self.inTb * m_sign(st(0.0 - self.L)))
        tsum = st(self.inTd + self.Ty + num)
        dt = SolverStatic.deltaTime
        num2 = st(fdiv(0.0 - self.L, dt))
        num3 = st(fdiv(st(fdiv(self.V[1], self.radius) - self.w) * self.I, dt))
        num4 = fast_abs(num2)
        num5 = fast_abs(num3)
        if (not (num4 < num5)) and fast_abs(st(num5 - num4)) >= C1E6:
            tsum = solve_sym(tsum, self.Ty, num3)
            tsum = solve_sym(tsum, num, num2)
        else:
            tsum = solve_sym(tsum, num, num2)
            tsum = solve_sym(tsum, self.Ty, num3)
        self.T = tsum
        self.Tr = st(self.T - self.inTd)
        num6 = st(m_clamp(st(self.Fext[1] * self.radius - self.inTd), st(0.0 - self.inTb), self.inTb) + self.inTd)
        if num3 <= 0.0:
            num7 = m_max(self.Ty, num3)
            if num6 < 0.0:
                num8 = st(self.Ty * m_inverse_lerp(st(self.adhSlip[1] * m_sign(self.fslip)), 0.0, self.fslip))
                if num6 < num8:
                    num6 = num8
                num9 = st(num6 - num7)
            else:
                num9 = m_clamp(num6, st(0.0 - num7), st(0.0 - self.Ty))
        else:
            num7 = m_min(self.Ty, num3)
            if num6 > 0.0:
                num10 = st(self.Ty * m_inverse_lerp(st(self.adhSlip[1] * m_sign(self.fslip)), 0.0, self.fslip))
                if num6 > num10:
                    num6 = num10
                num9 = st(num6 - num7)
            else:
                num9 = m_clamp(num6, st(0.0 - self.Ty), st(0.0 - num7))
        Fy = st(fdiv(num9, self.radius))
        tf = self.tireFriction
        c = self.contact
        if self.isAdherent:
            self.adhSlip = (self.adhSlip[0], st(fdiv(self.adhSlip[1] * Fy, self.adhForce[1])))
            c.slip = self.adhSlip
            self.adhSlip = (tf.adherent_sideways(c), self.adhSlip[1])
            c.slip = self.adhSlip
            self.adhForce = tf.adherent_force(c)
            x = self.adhForce[0]
        else:
            x = self.tireForce[0]
        x = fast_abs(x)
        Fx = m_clamp(self.Fext[0], st(0.0 - x), x)
        self.F = (Fx, Fy)
        self.isResting = self.isResting and (x > fast_abs(self.Fext[0]))
        self.isResting = self.isResting and (self.inTb > fast_abs(st(self.Fext[1] * self.radius - self.inTd)))

    def get_derivative(self):
        return [self.T, self.F, self.Tr]

    def set_state(self, S):
        dt = SolverStatic.deltaTime
        self.L = S[0]
        self.F = v2_div(S[1], dt)
        self.Tr = st(fdiv(S[2], dt))
        self.recalc_vars()
        self.brake = 0.0


class DirectDrive:
    def __init__(self):
        self.motorInput = 0.0
        self.maxMotorTorque = 100.0
        self.maxRpm = 500.0
        self.damping = st(0.95)
        self.out = None
        self.inp = None

    def get_state(self):
        return [0.0, (0.0, 0.0), 0.0]

    def set_substep_state(self, S):
        pass

    def compute_state_upstream(self):
        pass

    def eval_torque_downstream(self):
        value = st(self.motorInput * self.maxMotorTorque)
        num = st(self.out.I * fast_abs(self.maxRpm) * RpmToW)
        dt = SolverStatic.deltaTime
        mx = st(fdiv(num - self.out.L, dt) - self.out.Tr * self.damping)
        mn = st(fdiv(0.0 - num - self.out.L, dt) - self.out.Tr * self.damping)
        self.out.outTd = m_clamp(value, mn, mx)

    def get_derivative(self):
        return None

    def set_state(self, S):
        pass


def euler_step(nw, S, D, O):
    dt = SolverStatic.deltaTime
    for i in range(len(S)):
        if i < nw:
            L = st(S[i][0] + D[i][0] * dt)
            P = v2_add(S[i][1], v2_mul(D[i][1], dt))
            Lr = st(S[i][2] + D[i][2] * dt)
            O[i] = [L, P, Lr]
        else:
            L = st(S[i][0] + D[i][0] * dt)
            Lr = st(S[i][2] + D[i][2] * dt)
            O[i] = [L, O[i][1], Lr]


class Solver:
    def __init__(self, wheels):
        lst = list(wheels)
        self.nw = len(wheels)
        n = 0
        while n < len(lst):
            cnt = len(lst)
            for j in range(n, cnt):
                c = lst[j].inp
                if c is not None and c.output not in lst:
                    lst.append(c.output)
            n = cnt
        self.blocks = lst
        self.S = [[0.0, (0.0, 0.0), 0.0] for _ in lst]
        self.D = [[0.0, (0.0, 0.0), 0.0] for _ in lst]

    def derivative(self, S, Out):
        for i, b in enumerate(self.blocks):
            b.set_substep_state(S[i])
            b.compute_state_upstream()
        for i in range(len(self.blocks) - 1, -1, -1):
            b = self.blocks[i]
            b.eval_torque_downstream()
            d = b.get_derivative()
            if d is not None:
                Out[i] = [d[0], d[1], d[2]]

    def integrate(self, t, dt, steps, rk4):
        if rk4:
            self.rk4(t, dt)
        else:
            self.euler(t, dt, steps)

    def euler(self, t, dt, steps):
        SolverStatic.time = t
        SolverStatic.deltaTime = dt
        S = self.S
        for i, b in enumerate(self.blocks):
            S[i] = b.get_state()
        num2 = st(fdiv(dt, float(steps)))
        for _ in range(steps):
            SolverStatic.time = t
            SolverStatic.deltaTime = num2
            self.derivative(S, self.D)
            euler_step(self.nw, S, self.D, S)
            t = st(t + num2)
        SolverStatic.time = st(t + dt)
        SolverStatic.deltaTime = dt
        for i, b in enumerate(self.blocks):
            b.set_state(S[i])

    def eval_rk4(self, S, Stemp, DIn, t, dt, DOut):
        SolverStatic.time = t
        SolverStatic.deltaTime = dt
        euler_step(self.nw, S, DIn, Stemp)
        SolverStatic.time = st(t + dt)
        self.derivative(Stemp, DOut)

    def rk4(self, t, dt):
        SolverStatic.time = t
        SolverStatic.deltaTime = dt
        n = len(self.blocks)
        S = self.S
        Stemp = [[0.0, (0.0, 0.0), 0.0] for _ in range(n)]
        Da = [[0.0, (0.0, 0.0), 0.0] for _ in range(n)]
        Db = [[0.0, (0.0, 0.0), 0.0] for _ in range(n)]
        Dc = [[0.0, (0.0, 0.0), 0.0] for _ in range(n)]
        Dd = [[0.0, (0.0, 0.0), 0.0] for _ in range(n)]
        D = self.D
        for i, b in enumerate(self.blocks):
            S[i] = b.get_state()
        self.derivative(S, Da)
        half = st(dt * 0.5)
        self.eval_rk4(S, Stemp, Da, t, half, Db)
        self.eval_rk4(S, Stemp, Db, t, half, Dc)
        self.eval_rk4(S, Stemp, Dc, t, dt, Dd)
        SolverStatic.time = t
        SolverStatic.deltaTime = dt
        c6 = st(1.0 / 6.0)  # 1f / 6f
        for i in range(n):
            T = st(c6 * (Da[i][0] + 2.0 * (Db[i][0] + Dc[i][0]) + Dd[i][0]))
            Tr = st(c6 * (Da[i][2] + 2.0 * (Db[i][2] + Dc[i][2]) + Dd[i][2]))
            if i < self.nw:
                bc = v2_add(Db[i][1], Dc[i][1])
                s1 = v2_add(Da[i][1], v2_mul(bc, 2.0))
                s2 = v2_add(s1, Dd[i][1])
                D[i] = [T, v2_mul(s2, c6), Tr]
            else:
                D[i] = [T, D[i][1], Tr]
        euler_step(self.nw, S, D, S)
        SolverStatic.time = st(t + dt)
        for i, b in enumerate(self.blocks):
            b.set_state(S[i])


def connect(wheel, dd):
    c = Conn()
    c.output = dd
    wheel.inp = c
    dd.out = c


# ----------------------------------------------------------------------------------------------------------------------- vehicle
class WState:
    def __init__(self):
        self.wheelCol = None
        self.steerAngle = 0.0
        self.grounded = False
        self.hit = None
        self.groundMaterial = None
        self.lastHitMat = -1
        self.contactDepth = 0.0
        self.suspensionCompression = 0.0
        self.downforce = 0.0
        self.normalizedLoad = 0.0
        self.contactAngle = 0.0
        self.contactSpeed = 0.0
        self.damperForce = 0.0
        self.lastContactDepth = 0.0
        self.wheelVelocity = (0.0, 0.0, 0.0)
        self.surfaceForce = (0.0, 0.0, 0.0)
        self.localWheelVelocity = (0.0, 0.0)
        self.localSurfaceForce = (0.0, 0.0)
        self.externalTireForce = (0.0, 0.0)
        self.angularVelocity = 0.0
        self.tireForce = (0.0, 0.0)
        self.reactionTorque = 0.0
        self.tireSlip = (0.0, 0.0)
        self.combinedTireSlip = 0.0
        self.lastTireForce = (0.0, 0.0)
        self.driveTorque = 0.0
        self.brakeTorque = 0.0


class Hit:
    pass


class Collider:
    def __init__(self, rb, mat):
        self.rb = rb
        self.mat = mat


class WCol:
    """The native UnityEngine.WheelCollider (state only)."""

    def __init__(self, nat):
        self.center = tuple(nat["center"])
        self.suspensionDistance = nat["suspensionDistance"]
        self.radius = nat["radius"]
        self.sprungMass = nat["sprungMass"]
        self.spring = nat["spring"]
        self.damper = nat["damper"]
        self.target = nat["target"]
        self.forceAppPointDistance = 0.0
        self.motorTorque = 0.0
        self.steerAngle = 0.0
        self.hit = None


class VPWC:
    scaleFactor = 1.0
    minSuspensionDistance = st(0.01)

    def __init__(self, ctl, d, nat):
        self.ctl = ctl
        self.mass = d["mass"]
        self.radius = d["radius"]
        self.center = tuple(d["center"])
        self.suspensionDistance = d["suspensionDistance"]
        self.suspensionAnchor = d["suspensionAnchor"]
        self.springRate = d["springRate"]
        self.damperRate = d["damperRate"]
        self.groundPenetration = d["groundPenetration"]
        self.disableSuspensionMovement = d.get("disableSuspensionMovement", False)
        self.suspensionOffset = d.get("suspensionOffset", 0.0)
        self.springSlerpRate = d["springSlerpRate"]
        self.disableSteerAngleFix = d.get("disableSteerAngleFix", False)
        self.disableWheelReferenceFrameFix = d.get("disableWheelReferenceFrameFix", False)
        self.col = WCol(nat)
        self.tf_pos = (0.0, 0.0, 0.0)
        self.tf_rot = (0.0, 0.0, 0.0, 1.0)
        self.rbtf_rot = (0.0, 0.0, 0.0, 1.0)
        self.m_steerAngle = 0.0
        self.angularPosition = 0.0
        self.contactDistance = 0.0
        self.suspensionPosition = (0.0, 0.0, 0.0)
        self.angularVelocity = 0.0
        self.canSleep = False
        self.runtimeSpringRate = 0.0
        self.runtimeDamperRate = 0.0
        self.runtimeSuspensionTravel = 0.0
        self.lastRuntimeSpringRate = 0.0
        self.lastRuntimeDamperRate = 0.0
        self.lastRuntimeSuspensionTravel = 0.0
        self.runtimeExtraDownforce = 0.0
        self.active = True  # isActiveAndEnabled

    def up(self):
        return n_up(self.tf_rot)

    def forward(self):
        return n_forward(self.tf_rot)

    def on_enable_vehicle(self):
        self.suspensionPosition = self.tf_pos
        self.runtimeSpringRate = self.springRate
        self.runtimeDamperRate = self.damperRate
        self.runtimeSuspensionTravel = self.suspensionDistance
        self.runtimeExtraDownforce = 0.0

    def set_steer(self, value):
        if self.active:
            self.col.steerAngle = value if self.disableSteerAngleFix else self.fix_steer(value)
            self.m_steerAngle = value

    def fix_steer(self, v):
        if not self.active:
            return v
        vec = quat_mul_vec(n_angle_axis(v, self.up()), self.forward())
        rbup = n_up(self.rbtf_rot)
        vec2 = v3_sub(vec, v3_project(vec, rbup))
        return st(v3_angle(n_forward(self.rbtf_rot), vec2) * m_sign(v3_dot(n_right(self.rbtf_rot), vec2)))

    def fixed_update_vehicle(self):
        self.update_wheel_collider(self.col)
        self.lastRuntimeSpringRate = self.runtimeSpringRate
        self.lastRuntimeDamperRate = self.runtimeDamperRate
        self.lastRuntimeSuspensionTravel = self.runtimeSuspensionTravel
        self.runtimeSpringRate = self.springRate
        self.runtimeDamperRate = self.damperRate
        self.runtimeSuspensionTravel = self.suspensionDistance
        self.runtimeExtraDownforce = 0.0

    def set_force_offset(self, offset):
        self.col.forceAppPointDistance = st(offset - self.lastRuntimeSuspensionTravel * self.col.target)

    def get_ground_hit(self, ctl):
        h = self.col.hit
        if h is None:
            return False, None
        flag = True
        if not self.active:
            flag = False
        out = Hit()
        out.point, out.normal, out.forwardDir, out.sidewaysDir, out.force, out.collider = h.point, h.normal, h.forwardDir, h.sidewaysDir, h.force, h.collider
        if flag:
            out.point = v3_add(out.point, v3_mul(ctl.rb.velocity, ctl.fixedDeltaTime))
            if not self.disableWheelReferenceFrameFix:
                up = self.up()
                vec = quat_mul_vec(n_angle_axis(self.m_steerAngle, up), self.forward())
                num = st(0.0 - v3_dot(vec, out.normal))
                num2 = v3_dot(up, out.normal)
                if num2 > C1E6:
                    vec2 = v3_mul(up, st(fdiv(num, num2)))
                    out.forwardDir = v3_normalized(v3_add(vec, vec2))
                else:
                    out.forwardDir = up if num >= 0.0 else v3_neg(up)
                out.sidewaysDir = v3_cross(out.normal, out.forwardDir)
        return flag, out

    def get_contact_depth(self, p, travel):
        if self.suspensionDistance < self.minSuspensionDistance:
            self.suspensionDistance = self.minSuspensionDistance
        num = st(0.0 - n_inverse_transform_point(self.tf_pos, self.tf_rot, p)[1])
        return st(travel + self.radius - (num + self.suspensionDistance * (1.0 - self.suspensionAnchor)))

    def tangent_velocity(self, ctl, p, n, surf):
        pv = ctl.rb.point_velocity(p)
        if surf is not None:
            pv = v3_sub(pv, surf.point_velocity(p))
        return v3_sub(pv, v3_project(pv, n))

    def apply_force(self, ctl, force, pos, other):
        ctl.rb.add_force(force, pos)
        if other is not None and not other.kinematic:
            num = m_clamp01(st(fdiv(other.mass, ctl.rb.mass)))
            other.add_force(v3_mul(v3_neg(force), num), pos)

    def update_wheel_collider(self, wc):
        ctl = self.ctl
        wc.motorTorque = 0.0 if self.canSleep else st(1e-5)
        if self.suspensionDistance < self.minSuspensionDistance:
            self.suspensionDistance = self.minSuspensionDistance
        b = v3_add(self.center, v3_mul(v3_mul((0.0, 1.0, 0.0), self.suspensionDistance), st(1.0 - self.suspensionAnchor)))
        if not (wc.center[0] == b[0] and wc.center[1] == b[1] and wc.center[2] == b[2]):
            wc.center = b
        if self.runtimeSuspensionTravel < self.minSuspensionDistance:
            self.runtimeSuspensionTravel = self.minSuspensionDistance
        if wc.suspensionDistance != self.runtimeSuspensionTravel:
            wc.suspensionDistance = self.runtimeSuspensionTravel
        if wc.radius != self.radius:
            wc.radius = self.radius
        num = st(wc.sprungMass * ctl.gravity.magnitude())
        num2 = m_max(st(fdiv(num, wc.suspensionDistance)), st(0.1))
        spring = m_clamp(st(self.runtimeSpringRate * VPWC.scaleFactor), num2, INF)
        damper = 0.0 if ctl.advancedSuspensionDamper else m_max(st(self.runtimeDamperRate * VPWC.scaleFactor), st(0.1))
        target = m_clamp(st(fdiv(fdiv(num, spring), wc.suspensionDistance)), st(0.1), 1.0)
        if target != target:
            return
        cur = wc.target
        if abs(st(target - cur)) > self.springSlerpRate:
            if cur > target:
                target = st(cur - self.springSlerpRate)
            else:
                target = st(cur + self.springSlerpRate)
        if wc.spring != spring or wc.damper != damper or wc.target != target:
            wc.spring, wc.damper, wc.target = spring, damper, target


class Gravity:
    def __init__(self):
        self.value = v3_mul((0.0, -1.0, 0.0), st(9.80665))
        self.cached = False
        self.mag = 0.0
        self.upv = (0.0, 0.0, 0.0)

    def set(self, v):
        if not v3_approx_eq(self.value, v):
            self.value = v
            self.refresh()

    def refresh(self):
        self.mag = v3_mag(self.value)
        self.upv = v3_normalized(v3_neg(self.value))
        self.cached = True

    def magnitude(self):
        if not self.cached:
            self.refresh()
        return self.mag

    def up(self):
        if not self.cached:
            self.refresh()
        return self.upv


class Ctl:
    """KSPWheelController : VehicleBase with one wheel (the reachable VehicleBase.FixedUpdate / Update paths)."""

    def __init__(self, spec):
        c = spec["ctl"]
        self.fixedDeltaTime = c["fixedDeltaTime"]
        self.maxDriveTorque = c["maxDriveTorque"]
        self.maxBrakeTorque = c["maxBrakeTorque"]
        self.maxBrakeTorqueRest = c.get("maxBrakeTorqueRest", 1000.0)
        self.maxSteerAngle = c["maxSteerAngle"]
        self.maxRpm = c["maxRpm"]
        self.wheelDamping = c["wheelDamping"]
        self.driveResponse = c["driveResponse"]
        self.brakeResponse = c["brakeResponse"]
        self.steeringResponse = c["steeringResponse"]
        self.driveInput = 0.0
        self.brakeInput = 0.0
        self.steerInput = 0.0
        self.driveState = 0.0
        self.steerState = 0.0
        self.brakeState = 0.0
        self.minimumdownForce = c["minimumdownForce"]
        self.tireForceSharpness = c["tireForceSharpness"]
        self.tireForceSmoothing = c.get("tireForceSmoothing", True)
        self.tireSideDeflection = c["tireSideDeflection"]
        self.tireSideDeflectionRate = c["tireSideDeflectionRate"]
        self.tireImpulseRatio = c.get("tireImpulseRatio", st(0.5))
        self.integrationSteps = c["integrationSteps"]
        self.integrationUseRK4 = c["integrationUseRK4"]
        self.advancedSuspensionDamper = c.get("advancedSuspensionDamper", False)
        self.accurateSuspensionForces = True
        self.scaleFactor = 1.0
        self.contactAngleAffectsTireForce = True
        self.vehicleSleepCriteria = "strict"
        self.wheelSleepVelocity = st(0.2)
        self.inhibitWheelSleep = False
        self.gravity = Gravity()
        self.gravity.set(tuple(c["gravity"]))
        self.rb = Rb()
        self.rbtf_pos = (0.0, 0.0, 0.0)
        self.rbtf_rot = (0.0, 0.0, 0.0, 1.0)
        self.m_time = 0.0
        self.lastVelocity = (0.0, 0.0, 0.0)
        self.speed = 0.0
        self.speedAngle = 0.0
        self.localAcceleration = (0.0, 0.0, 0.0)
        self.localDragVelocity = (0.0, 0.0, 0.0)
        # the controller's own TireFriction: constructed Smooth over its own Settings object; the module mutates the fields
        # (ModuleWheelBase.ApplyGeeBias / UpdateFriction) and sets model = Lineal WITHOUT rebuilding the curve (ModuleWheelBase.cs:361).
        fr = spec["friction"]
        self.tireFriction = TireFriction()
        self.tireFriction.settings.mutate(fr["settings"])
        self.tireFriction.frictionMultiplier = fr["frictionMultiplier"]
        self.tireFriction.model = fr["model"]
        if fr["rebuild"]:
            self.tireFriction.setup()
        self.materials = spec.get("materials")  # None = no GroundMaterialManager in the scene
        self.wc = VPWC(self, spec["wc"], spec["native"])
        self.wheel = None
        self.dd = None
        self.state = None
        self.solver = None
        self.init = False

    def on_initialize(self):  # KSPWheelController.OnInitialize
        self.wheel = Wheel()
        self.state = WState()
        self.state.wheelCol = self.wc
        self.wheel.tireFriction = self.tireFriction
        self.wheel.radius = self.wc.radius
        self.wheel.mass = self.wc.mass
        self.vehicleSleepCriteria = "relaxed"
        self.wheelSleepVelocity = 0.0
        self.dd = DirectDrive()
        connect(self.wheel, self.dd)
        self.init = True

    def initialize(self):
        if not self.init:
            self.on_initialize()

    def enable(self):  # VehicleBase.OnEnable
        self.m_time = 0.0
        VPWC.scaleFactor = self.scaleFactor
        self.on_initialize()
        w = self.wheel
        w.recalc_constants()
        w.recalc_vars()
        self.rb.maxAng = 14.0
        self.solver = Solver([w])
        self.solver.integrate(self.m_time, self.fixedDeltaTime, 1, False)
        self.wc.on_enable_vehicle()

    def compute_vehicle_speed(self):
        velocity = self.rb.velocity
        forward = n_forward(self.rbtf_rot)
        self.speed = v3_dot(velocity, forward)
        velocity = (velocity[0], 0.0, velocity[2])
        forward = (forward[0], 0.0, forward[2])
        self.speedAngle = st(v3_angle(velocity, forward) * m_sign(v3_dot(velocity, n_right(self.rbtf_rot))))

    def compute_local_acceleration(self, dt):
        velocity = self.rb.velocity
        direction = v3_div(v3_sub(velocity, self.lastVelocity), dt)
        self.localAcceleration = n_inverse_transform_direction(self.rbtf_rot, direction)
        self.lastVelocity = velocity

    def calc_travel(self, w):
        wc = w.wheelCol
        w.grounded, w.hit = wc.get_ground_hit(self)
        w.lastContactDepth = w.contactDepth
        if w.grounded:
            w.contactDepth = wc.get_contact_depth(w.hit.point, wc.lastRuntimeSuspensionTravel)
            w.suspensionCompression = st(fdiv(w.contactDepth, wc.lastRuntimeSuspensionTravel))
        else:
            w.contactDepth = 0.0
            w.suspensionCompression = 0.0
            if w.hit is not None:
                w.hit.force = 0.0

    def calc_forces(self, w, dt):
        wc = w.wheelCol
        if w.grounded:
            w.contactDepth = st(w.contactDepth + (wc.runtimeSuspensionTravel - wc.lastRuntimeSuspensionTravel))
            w.suspensionCompression = st(fdiv(w.contactDepth, wc.runtimeSuspensionTravel))
            w.contactSpeed = st(fdiv(w.contactDepth - w.lastContactDepth, dt))
            if self.advancedSuspensionDamper:
                raise RuntimeError("NullReferenceException: static VehicleBase.vehicle is null")
            w.damperForce = st(w.hit.force - w.contactDepth * wc.runtimeSpringRate * self.scaleFactor)
        else:
            w.contactSpeed = 0.0
            w.damperForce = 0.0
        if self.accurateSuspensionForces:
            wc.set_force_offset(w.contactDepth)

    def calc_downforce(self, w, ref):
        wc = w.wheelCol
        if w.grounded:
            w.downforce = st(w.hit.force + wc.runtimeExtraDownforce)
            if self.contactAngleAffectsTireForce:
                up = wc.up()
                y = v3_dot(up, w.hit.sidewaysDir)
                x = v3_dot(up, w.hit.normal)
                w.contactAngle = m_atan2(y, x)
                w.downforce = st(w.downforce * m_cos(w.contactAngle))
            if w.downforce < self.minimumdownForce:
                w.downforce = self.minimumdownForce
            w.normalizedLoad = st(fdiv(w.hit.force, ref))
        else:
            w.contactAngle = 0.0
            w.downforce = 0.0
            w.normalizedLoad = 0.0

    def calc_local_frame(self, w, dt):
        wc = w.wheelCol
        if not w.grounded:
            w.wheelVelocity = (0.0, 0.0, 0.0)
            w.localWheelVelocity = (0.0, 0.0)
            w.surfaceForce = (0.0, 0.0, 0.0)
            w.localSurfaceForce = (0.0, 0.0)
            w.externalTireForce = (0.0, 0.0)
            return
        h = w.hit
        w.wheelVelocity = wc.tangent_velocity(self, h.point, h.normal, h.collider.rb)
        w.localWheelVelocity = (v3_dot(h.sidewaysDir, w.wheelVelocity), v3_dot(h.forwardDir, w.wheelVelocity))
        num = m_inverse_lerp(1.0, 0.25, v3_sqrmag(w.wheelVelocity))
        if num > 0.0:
            num2 = v3_dot(self.gravity.up(), h.normal)
            if num2 > C1E6:
                vec = v3_div(v3_mul(self.gravity.up(), h.force), num2)
                w.surfaceForce = v3_sub(vec, v3_project(vec, h.normal))
            else:
                w.surfaceForce = v3_mul(self.gravity.up(), 100000.0)
            w.localSurfaceForce = (v3_dot(h.sidewaysDir, w.surfaceForce), v3_dot(h.forwardDir, w.surfaceForce))
            w.localSurfaceForce = v2_mul(w.localSurfaceForce, num)
        else:
            w.surfaceForce = (0.0, 0.0, 0.0)
            w.localSurfaceForce = (0.0, 0.0)
        num3 = st((h.force + wc.runtimeExtraDownforce) * fdiv(1.0, self.gravity.magnitude()))
        if num3 < 0.0:
            num3 = 0.0
        t = st((0.0 - self.tireImpulseRatio) * num3)
        vec2 = v2_div(v2_mul(w.localWheelVelocity, t), dt)
        w.externalTireForce = v2_add(vec2, w.localSurfaceForce)
        if self.materials is not None:
            if h.collider.mat != w.lastHitMat:
                w.lastHitMat = h.collider.mat
                w.groundMaterial = self.materials.get(str(h.collider.mat))

    def apply_tire_force(self, w, dt):
        if not w.grounded:
            w.lastTireForce = (0.0, 0.0)
            return
        h = w.hit
        num = w.groundMaterial["drag"] if w.groundMaterial is not None else 0.0
        vec = v2_mul(w.localWheelVelocity, st(0.0 - h.force * v2_mag(w.localWheelVelocity) * num * st(0.001)))
        if self.tireSideDeflection:
            w.tireForce = (m_lerp(w.lastTireForce[0], w.tireForce[0], st(self.tireSideDeflectionRate * dt)), w.tireForce[1])
        if self.tireForceSmoothing:
            r = st(self.tireForceSharpness * dt)
            w.tireForce = (m_lerp(w.lastTireForce[0], w.tireForce[0], r), m_lerp(w.lastTireForce[1], w.tireForce[1], r))
        w.lastTireForce = w.tireForce
        vector2 = v3_add(v3_mul(h.forwardDir, st(w.tireForce[1] + vec[1])), v3_mul(h.sidewaysDir, st(w.tireForce[0] + vec[0])))
        vector3 = v3_mul(w.wheelCol.up(), w.damperForce) if self.advancedSuspensionDamper else (0.0, 0.0, 0.0)
        w.wheelCol.apply_force(self, v3_add(vector2, vector3), h.point, h.collider.rb)

    @staticmethod
    def combined_slip(lv, ts):
        magnitude = v2_mag(lv)
        if magnitude > st(0.01):
            num = st(fdiv(ts[0] * lv[0], magnitude))
            y = ts[1]
            return m_sqrt(st(num * num + y * y))
        return v2_mag(ts)

    def do_update_blocks(self, dt):  # KSPWheelController.DoUpdateBlocks (the keyboard flags are false)
        self.driveState = m_move_towards(self.driveState, self.driveInput, st(self.driveResponse * dt))
        self.brakeState = m_move_towards(self.brakeState, self.brakeInput, st(self.brakeResponse * dt))
        self.dd.motorInput = self.driveState
        self.dd.maxMotorTorque = self.maxDriveTorque
        self.dd.maxRpm = self.maxRpm
        self.dd.damping = self.wheelDamping
        if self.brakeState >= 1.0:
            self.wheel.add_brake(st(self.brakeState * self.maxBrakeTorqueRest))
        else:
            self.wheel.add_brake(st(self.brakeState * self.maxBrakeTorque))

    def fixed_update(self, dt):
        rb = self.rb
        magnitude = v3_mag(rb.velocity)
        if magnitude > 1.0:
            rb.maxDepen = 1.0
        else:
            rb.maxDepen = m_lerp(8.0, 1.0, magnitude)
        VPWC.scaleFactor = self.scaleFactor
        num = st(fdiv(1.0, self.scaleFactor))
        self.compute_vehicle_speed()
        self.compute_local_acceleration(dt)
        w = self.state
        self.calc_travel(w)
        ref = st(rb.mass * self.gravity.magnitude() / 1.0)
        self.calc_forces(w, dt)
        self.calc_downforce(w, ref)
        self.calc_local_frame(w, dt)
        wh = self.wheel
        wh.load = st(w.downforce * num)
        wh.grip = w.groundMaterial["grip"] if w.groundMaterial is not None else 1.0
        wh.V = w.localWheelVelocity
        wh.Fext = v2_mul(w.externalTireForce, num)
        n3 = m_max(w.brakeTorque, st(w.downforce * num * wh.radius))
        wh.Tr = m_clamp(w.reactionTorque, st(0.0 - n3), n3)
        self.do_update_blocks(dt)
        self.solver.integrate(self.m_time, dt, self.integrationSteps, self.integrationUseRK4)
        resting = 1 if wh.isResting else 0
        count = 1
        if self.vehicleSleepCriteria == "strict":
            flag = resting >= count // 2
        else:
            flag = resting > count // 2
        w.angularVelocity = wh.w
        w.tireForce = v2_mul(wh.F, self.scaleFactor)
        w.reactionTorque = wh.Tr
        w.tireSlip = (w.localWheelVelocity[0], st(w.localWheelVelocity[1] - w.angularVelocity * wh.radius))
        w.combinedTireSlip = self.combined_slip(w.localWheelVelocity, w.tireSlip)
        w.driveTorque = wh.inTd
        w.brakeTorque = wh.inTb
        self.apply_tire_force(w, dt)
        flag2 = v2_mag(w.localWheelVelocity) < self.wheelSleepVelocity
        self.wc.canSleep = (not self.inhibitWheelSleep) and (flag2 if flag else (flag2 and wh.isResting))
        self.inhibitWheelSleep = False
        self.wc.fixed_update_vehicle()
        self.m_time = st(self.m_time + dt)

    def update(self, frame_dt, raycast):
        """VehicleBase.Update: steering write-back, visual wheel state, drag state; then KSPWheelController.OnUpdate."""
        w = self.state
        wc = self.wc
        wc.set_steer(w.steerAngle)
        wc.angularVelocity = w.angularVelocity
        # UpdateVisualWheel(deltaTime): the physics-visible state (the Transform writes are the renderer's)
        vec = v3_neg(wc.up())
        if raycast is not None:
            wc.contactDistance = m_clamp(st(raycast - wc.radius * 2.0), 0.0, wc.lastRuntimeSuspensionTravel)
        else:
            wc.contactDistance = wc.lastRuntimeSuspensionTravel
        wc.angularPosition = st(math.fmod(wc.angularPosition + wc.angularVelocity * frame_dt, st(F(math.pi) * F(2.0))))
        if not wc.disableSuspensionMovement:
            wc.suspensionPosition = v3_add(wc.tf_pos, v3_mul(vec, st(wc.contactDistance + wc.groundPenetration - wc.suspensionDistance * (1.0 - wc.suspensionAnchor) + wc.suspensionOffset)))
        else:
            wc.suspensionPosition = wc.tf_pos
        # UpdateDragState(zero, zero, hardness): the drag velocity decays towards zero
        self.localDragVelocity = v3_lerp(self.localDragVelocity, (0.0, 0.0, 0.0), st(10.0 * frame_dt))
        # KSPWheelController.OnUpdate
        self.steerState = m_move_towards(self.steerState, self.steerInput, st(self.steeringResponse * frame_dt))
        w.steerAngle = st(self.steerState * self.maxSteerAngle)


# ----------------------------------------------------------------------------------------------------------------------- scenario generation
class Rng:
    def __init__(self, seed):
        self.r = np.random.RandomState(seed)

    def u(self, lo, hi):
        return st(self.r.uniform(lo, hi))

    def choice(self, xs):
        return xs[int(self.r.randint(0, len(xs)))]

    def i(self, lo, hi):
        return int(self.r.randint(lo, hi))

    def p(self, pr):
        return bool(self.r.uniform(0, 1) < pr)

    def vec(self, lo, hi):
        return [self.u(lo, hi), self.u(lo, hi), self.u(lo, hi)]

    def unit(self):
        while True:
            v = self.r.normal(0, 1, 3)
            n = float(np.linalg.norm(v))
            if n > 0.1:
                return [st(x / n) for x in v]

    def quat(self, yaw_only=False):
        if yaw_only:
            a = self.r.uniform(-math.pi, math.pi)
            return [0.0, st(math.sin(a / 2)), 0.0, st(math.cos(a / 2))]
        v = self.r.normal(0, 1, 4)
        v = v / np.linalg.norm(v)
        return [st(x) for x in v]


def jf(x):
    """Shortest float32 round-trip number (NaN / inf as strings, -0 as "-0")."""
    if x != x:
        return "nan"
    if x == INF:
        return "inf"
    if x == -INF:
        return "-inf"
    f = F(x)
    assert float(f) == x or float(f) == 0.0, "value %r is not float32-exact: an input or output skipped st()" % x
    if float(f) == 0.0:
        return "-0" if math.copysign(1.0, float(f)) < 0 else 0.0
    a = abs(float(f))
    s = np.format_float_scientific(f, unique=True, trim="-") if (a < 1e-3 or a >= 1e7) else np.format_float_positional(f, unique=True, trim="-")
    v = float(s)
    assert F(v) == f
    return v


def enc(x):
    if isinstance(x, dict):
        return {k: enc(v) for k, v in x.items()}
    if isinstance(x, (list, tuple)):
        return [enc(v) for v in x]
    if isinstance(x, bool):
        return 1 if x else 0
    if isinstance(x, float):
        return jf(x)
    return x


# ---- tyre friction rows
def friction_rows(rng, n_per):
    sets = [
        {},
        {"adherent": [st(0.5), st(0.95)], "peak": [st(1.5), st(1.1)], "limit": [4.0, st(0.8)]},
        {"adherent": [st(0.25), st(1.2)], "peak": [st(0.9), st(2.0)], "limit": [st(3.0), st(1.4)], "a": st(0.3), "b": st(0.2), "c": st(0.6), "d": st(0.1)},
        # constraint-violating (ApplyAPLConstraints / the Pacejka clamps)
        {"adherent": [0.0, -1.0], "peak": [st(0.1), st(0.4)], "limit": [st(0.05), st(2.0)], "A": 5.0, "B": 0.0, "C": 3.0, "D": -1.0, "E": 3.0},
        {"adherent": [st(0.7), st(1.5)], "peak": [st(0.7), st(1.5)], "limit": [st(0.7), st(1.5)], "A": st(0.5), "B": st(2.0), "C": st(1.2), "D": st(1.8), "E": st(-5.0)},
    ]
    rows = []
    for si, sd in enumerate(sets):
        for model in MODELS:
            tf = TireFriction()
            tf.settings = Settings(sd)
            tf.model = model
            fm = rng.choice([1.0, st(0.8), 0.0, 2.0])
            tf.frictionMultiplier = fm
            tf.setup()
            cs = tf.settings
            marks = [cs.adherent[0], cs.peak[0], cs.limit[0]]
            for k in range(n_per):
                kind = k % 6
                if kind == 0:  # at / around the thresholds
                    m = rng.choice(marks)
                    m = st(m * rng.choice([1.0, st(1.0000001), st(0.9999999), 0.5, 2.0])) if m > 0 else 0.0
                    ang = rng.u(0, 2 * math.pi)
                    sx, sy = st(m * math.cos(ang)), st(m * math.sin(ang))
                elif kind == 1:  # tiny
                    sx, sy = rng.u(-2e-6, 2e-6), rng.u(-2e-6, 2e-6)
                elif kind == 2:  # axis aligned
                    sx, sy = (0.0, rng.u(-8, 8)) if rng.p(0.5) else (rng.u(-8, 8), 0.0)
                else:
                    sx, sy = st(rng.u(-6, 6) * rng.choice([1.0, 0.2, 0.05])), st(rng.u(-6, 6) * rng.choice([1.0, 0.2, 0.05]))
                load = rng.choice([0.0, st(0.01), 1.0, st(5.3), 25.0, st(133.7)]) if k % 3 else rng.u(0, 40)
                grip = rng.choice([1.0, st(0.5), st(1.3), 0.0]) if k % 4 else rng.u(0.1, 2.0)
                cp = Patch()
                cp.slip = (sx, sy)
                cp.load = load
                cp.groundGrip = grip
                f = tf.get_force(cp)
                af = tf.adherent_force(cp)
                rows.append([si, MODELS.index(model), fm, sx, sy, load, grip, f[0], f[1], 1 if tf.is_adherent(cp) else 0, tf.adherent_forward(cp), tf.adherent_sideways(cp),
                             af[0], af[1], tf.curve.adh(cp), tf.curve.peak(cp), tf.curve.lim(cp), tf.curve.eval(v2_mag((sx, sy)), cp)])
    return sets, rows


# ---- the wheel / solver block chain, driven directly
def wheel_scenarios(rng):
    out = []
    configs = []
    for model in ["smooth", "lineal"]:
        for (radius, mass) in [(st(0.4), st(0.04)), (st(0.25), st(0.01)), (st(0.5), 20.0), (st(0.3), st(0.1)), (st(1.0), 100.0), (st(0.1), st(0.01))]:
            for steps, rk4 in [(8, False), (4, False), (1, False), (8, True)]:
                for dt in [st(0.02), st(0.04), st(0.005), st(0.1)]:
                    configs.append((model, radius, mass, steps, rk4, dt))
    for ci, (model, radius, mass, steps, rk4, dt) in enumerate(configs):
        if ci % 3 != 0 and ci % 5 != 0:
            continue
        sd = rng.choice([{}, {"adherent": [st(0.25), st(1.2)], "peak": [st(0.9), st(2.0)], "limit": [st(3.0), st(1.4)]}])
        w = Wheel()
        w.tireFriction.settings = Settings(sd)
        w.tireFriction.model = model
        w.tireFriction.frictionMultiplier = rng.choice([1.0, st(0.8)])
        w.tireFriction.setup()
        w.radius = radius
        w.mass = mass
        w.recalc_constants()
        w.recalc_vars()
        dd = DirectDrive()
        connect(w, dd)
        solver = Solver([w])
        solver.integrate(0.0, dt, 1, False)
        ticks = []
        t = 0.0
        w.L = st(rng.u(-5, 20) * w.I) if rng.p(0.7) else 0.0
        w.recalc_vars()
        l0 = w.L
        mode = 0
        speed = 0.0
        for k in range(120):
            if k % 40 == 0:
                mode = rng.i(0, 5)
            if k % 10 == 0:
                speed = rng.u(-15, 25)
            V = (rng.u(-3, 3) if mode in (1, 4) else 0.0, st(speed + rng.u(-0.5, 0.5)))
            load = rng.choice([0.0, 0.5, 2.0, 9.0, 40.0]) if mode != 2 else rng.u(0, 30)
            grip = rng.choice([1.0, st(1.3)])
            Fext = (rng.u(-2, 2), rng.u(-2, 2)) if mode in (3, 4) else (0.0, 0.0)
            brakeT = rng.choice([0.0, 0.0, 5.0, 40.0, 1000.0]) if mode in (1, 2) else (1000.0 if mode == 3 else 0.0)
            motorIn = rng.choice([-1.0, 0.0, 1.0, st(0.37)])
            maxMotor = rng.choice([0.0, 4.0, 20.0, 100.0])
            maxRpm = rng.choice([st(500.0), 120.0, st(3.4028234663852886e38), 5.0])
            damping = rng.choice([st(0.95), 0.0, 1.0])
            Trin = st(rng.u(-3, 3))
            w.load, w.grip, w.V, w.Fext = load, grip, V, Fext
            n3 = m_max(w.inTb, st(load * radius))
            w.Tr = m_clamp(Trin, st(0.0 - n3), n3)
            dd.motorInput, dd.maxMotorTorque, dd.maxRpm, dd.damping = motorIn, maxMotor, maxRpm, damping
            w.add_brake(brakeT)
            brake2 = st(-brakeT * 0.5) if (brakeT > 0 and rng.p(0.3)) else 0.0
            w.add_brake(brake2)
            solver.integrate(t, dt, steps, rk4)
            t = st(t + dt)
            ticks.append([V[0], V[1], load, grip, Fext[0], Fext[1], brakeT, brake2, motorIn, maxMotor, maxRpm, damping, Trin,
                          w.L, w.w, w.T, w.Tr, w.F[0], w.F[1], w.Ty, w.tireForce[0], w.tireForce[1], 1 if w.isAdherent else 0,
                          w.adhSlip[0], w.adhSlip[1], w.adhForce[0], w.adhForce[1], 1 if w.isResting else 0, w.inTd, w.inTb, w.fslip,
                          dd.out.outTd, SolverStatic.time, SolverStatic.deltaTime])
        out.append({"model": model, "settings": sd, "fm": w.tireFriction.frictionMultiplier, "radius": radius, "mass": mass, "steps": steps, "rk4": rk4,
                    "dt": dt, "I": w.I, "L0": l0, "ticks": ticks})
    return out


WHEEL_COLS = ["V0", "V1", "load", "grip", "Fx", "Fy", "brake", "brake2", "motorIn", "maxMotor", "maxRpm", "damping", "Tr",
              "L", "w", "T", "TrOut", "Fox", "Foy", "Ty", "tfx", "tfy", "isAdherent", "ahx", "ahy", "afx", "afy", "isResting", "inTd", "inTb", "fslip",
              "outTd", "time", "dtS"]

FAMILIES = ["flat_drive", "airborne_transitions", "slopes", "moving_surface", "leg_lock", "steering", "ground_material", "reference_frame_fix_off",
            "rk4_and_substeps", "inertia_floor", "lineal_rebuilt", "lineal_not_rebuilt"]


def make_spec(rng, fam):
    leg = fam == "leg_lock"
    radius = rng.choice([st(0.3), st(0.4), st(0.5), st(0.625)])
    mass = rng.choice([st(0.04), st(0.1), st(0.25)])
    if fam == "inertia_floor":
        mass = st(0.01)
        radius = st(0.35)   # m r^2 / 2 = 0.0006 t m^2 < 0.01: the floor
    sd = rng.choice([{}, {"adherent": [st(0.3), st(1.1)], "peak": [st(1.0), st(1.8)], "limit": [st(3.5), st(1.2)]}])
    spec = {
        "family": fam,
        "ctl": {
            "fixedDeltaTime": rng.choice([st(0.02), st(0.04)]),
            "maxDriveTorque": rng.choice([0.0, 4.0, st(8.6), 20.0]),
            "maxBrakeTorque": rng.choice([st(3.0), st(10.0), st(24.0)]),
            "maxBrakeTorqueRest": 1000.0,
            "maxSteerAngle": st(28.0) if fam == "steering" else rng.choice([0.0, 15.0, st(30.0)]),
            "maxRpm": rng.choice([st(250.0), st(900.0), st(3.4028234663852886e38)]),
            "wheelDamping": rng.choice([st(0.05), 0.0, st(0.2)]),
            "driveResponse": rng.choice([2.0, 10.0, 100.0]),
            "brakeResponse": rng.choice([1.0, 5.0, 100.0]),
            "steeringResponse": rng.choice([1.0, 8.0, 50.0]),
            "minimumdownForce": 10.0 if leg else st(0.5),
            "tireForceSharpness": rng.choice([st(5.0), st(25.0), st(2.0)]),
            "tireForceSmoothing": True,
            "tireSideDeflection": rng.p(0.4),
            "tireSideDeflectionRate": rng.choice([st(10.0), st(30.0)]),
            "tireImpulseRatio": rng.choice([st(0.5), st(0.8)]),
            "integrationSteps": 8,
            "integrationUseRK4": False,
            "gravity": [0.0, -st(9.80665), 0.0],
        },
        "friction": {"model": "smooth", "rebuild": False, "settings": sd, "frictionMultiplier": rng.choice([1.0, st(0.9), st(1.4)])},
        "wc": {"mass": mass, "radius": radius, "center": rng.vec(-0.2, 0.2), "suspensionDistance": st(0.02), "suspensionAnchor": st(0.5),
               "springRate": 10000.0, "damperRate": 1.0, "groundPenetration": st(0.02), "springSlerpRate": rng.choice([st(0.02), st(0.2)]),
               "suspensionOffset": 0.0},
        "native": {"center": [0.0, 0.0, 0.0], "suspensionDistance": st(0.3), "radius": st(0.5), "sprungMass": rng.choice([st(0.04), st(0.1), st(1.3)]),
                   "spring": 35000.0, "damper": 4500.0, "target": st(0.5)},
        "rbMass": rng.choice([st(0.04), st(0.1), st(1.5)]),
        "materials": None,
    }
    if fam == "lineal_rebuilt":
        spec["friction"]["model"] = "lineal"
        spec["friction"]["rebuild"] = True
    elif fam == "lineal_not_rebuilt":   # ModuleWheelBase.cs:361: the model field is set, the curve is not rebuilt (stays Smooth)
        spec["friction"]["model"] = "lineal"
    elif fam in ("slopes", "moving_surface", "leg_lock", "ground_material"):
        spec["friction"]["model"] = rng.choice(["lineal", "smooth"])
        spec["friction"]["rebuild"] = spec["friction"]["model"] == "lineal"
    if fam == "rk4_and_substeps":
        spec["ctl"]["integrationSteps"] = rng.choice([1, 4, 8])
        spec["ctl"]["integrationUseRK4"] = rng.p(0.5)
    if fam == "ground_material":
        spec["materials"] = {"1": {"grip": st(1.2), "drag": 0.0}, "2": {"grip": st(0.4), "drag": st(30.0)}, "3": {"grip": st(2.0), "drag": st(5.0)}}
    if fam == "reference_frame_fix_off":
        spec["wc"]["disableWheelReferenceFrameFix"] = True
        spec["wc"]["disableSteerAngleFix"] = True
    if fam == "airborne_transitions":
        spec["wc"]["disableSuspensionMovement"] = rng.p(0.5)
    return spec


def make_ticks(rng, spec, n):
    fam = spec["family"]
    ticks = []
    dt0 = spec["ctl"]["fixedDeltaTime"]
    rbmass = spec["rbMass"]
    pos = [0.0, 1.0, 0.0]
    vel = rng.vec(-2, 2)
    wcq = rng.quat(yaw_only=(fam != "slopes"))
    rbq = wcq
    surf = None
    mode = 0
    for k in range(n):
        if k % 60 == 0:
            mode = rng.i(0, 6)
        dt = dt0
        if fam == "rk4_and_substeps" and k % 50 == 0:
            dt = rng.choice([st(0.02), st(0.04), st(0.08), st(0.005)])
        if k % 25 == 0:
            vel = [rng.u(-4, 20) if rng.p(0.8) else 0.0, rng.u(-1, 1), rng.u(-6, 6)]
        else:
            vel = [st(vel[0] + rng.u(-0.2, 0.2)), st(vel[1] + rng.u(-0.05, 0.05)), st(vel[2] + rng.u(-0.2, 0.2))]
        ang = rng.vec(-0.5, 0.5) if fam in ("moving_surface", "slopes", "steering") else [0.0, 0.0, 0.0]
        pos = [st(pos[0] + vel[0] * dt), st(pos[1] + vel[1] * dt), st(pos[2] + vel[2] * dt)]
        if fam == "airborne_transitions":
            grounded = (k // 7) % 3 != 2 if mode % 2 == 0 else rng.p(0.6)
        elif fam == "leg_lock":
            grounded = (k // 40) % 5 != 4
        else:
            grounded = rng.p(0.97)
        if fam in ("slopes", "moving_surface", "steering", "ground_material", "reference_frame_fix_off"):
            nrm = rng.unit() if rng.p(0.4) else [0.0, 1.0, 0.0]
            if nrm[1] < 0.2:
                nrm = [nrm[0] * 0.1, 1.0, nrm[2] * 0.1]
                nn = math.sqrt(sum(c * c for c in nrm))
                nrm = [st(c / nn) for c in nrm]
        else:
            nrm = [0.0, 1.0, 0.0] if rng.p(0.9) else [st(0.1), st(0.99), 0.0]
        force = rng.choice([0.0, st(0.04), 3.0, st(11.7), 40.0, st(300.0)]) if rng.p(0.8) else rng.u(0, 80)
        hp = [st(pos[0] + rng.u(-0.05, 0.05)), st(pos[1] - rng.u(0.1, 0.5)), st(pos[2] + rng.u(-0.05, 0.05))]
        mat = rng.choice([0, 1, 2, 3]) if spec["materials"] is not None else 0
        surf_rb = None
        if fam == "moving_surface" or (fam == "slopes" and rng.p(0.2)):
            if k % 30 == 0 or surf is None:
                surf = {"mass": rng.choice([st(0.05), st(10.0), 500.0]), "kinematic": rng.p(0.4), "vel": rng.vec(-5, 5), "ang": rng.vec(-1, 1), "com": rng.vec(-2, 2)}
            surf_rb = surf
        t = {
            "dt": dt,
            "frames": [[rng.u(0.004, 0.05), (rng.u(0.0, 0.6) if rng.p(0.5) else None)] for _ in range(rng.i(0, 3))],
            "drive": rng.choice([-1.0, 0.0, 1.0, st(0.4)]) if (k // 30) % 3 else 0.0,
            "brake": 1.0 if fam == "leg_lock" else (1.0 if (k // 20) % 4 == 3 else rng.choice([0.0, 0.0, st(0.3), 1.0])),
            "steer": rng.choice([-1.0, 0.0, 1.0, st(0.2)]) if (k // 30) % 2 else 0.0,
            "rb": {"mass": rbmass, "vel": vel, "ang": ang, "com": [st(pos[0] + 0.1), st(pos[1] + 0.2), st(pos[2] - 0.1)], "pos": pos},
            "rbq": rbq,
            "wcpos": [st(pos[0] + 0.3), st(pos[1] - 0.1), st(pos[2])],
            "wcq": wcq,
            "sprungMass": spec["native"]["sprungMass"] if k % 17 else rng.choice([st(0.04), st(0.1), 2.0]),
            "runtime": None,
            "hit": None,
        }
        if k % 4 == 0:
            t["runtime"] = [rng.choice([5000.0, 20000.0, 158000.0]), rng.choice([1.0, 400.0, 3000.0]), rng.choice([st(0.02), st(0.5), st(1.19), st(0.001)]), rng.choice([0.0, 0.0, st(2.0)])]
        if grounded:
            t["hit"] = {"point": hp, "normal": nrm, "fwd": rng.unit(), "side": rng.unit(), "force": force, "mat": mat, "surf": surf_rb}
        if fam == "steering" and k % 6 == 0:
            wcq = rng.quat()
            rbq = rng.quat()
        elif fam == "slopes" and k % 20 == 0:
            wcq = rng.quat()
            rbq = wcq
        ticks.append(t)
    return ticks


def run_vehicle(spec, ticks):
    c = Ctl(spec)
    c.rb.mass = spec["rbMass"]
    c.initialize()   # KSPWheelController.Initialize (ModuleWheelBase.wheelSetup): OnInitialize once ...
    c.enable()       # ... then `wheel.enabled = true` runs VehicleBase.OnEnable (OnInitialize again, the solver priming step)
    out = []
    for t in ticks:
        dt = t["dt"]
        rb = c.rb
        r = t["rb"]
        rb.mass, rb.velocity, rb.angvel, rb.com = r["mass"], tuple(r["vel"]), tuple(r["ang"]), tuple(r["com"])
        rb.forces = []
        c.rbtf_pos = tuple(r["pos"])
        c.rbtf_rot = tuple(t["rbq"])
        wc = c.wc
        wc.tf_pos = tuple(t["wcpos"])
        wc.tf_rot = tuple(t["wcq"])
        wc.rbtf_rot = tuple(t["rbq"])
        wc.col.sprungMass = t["sprungMass"]
        if t["runtime"] is not None:
            wc.runtimeSpringRate, wc.runtimeDamperRate, wc.runtimeSuspensionTravel, wc.runtimeExtraDownforce = t["runtime"]
        c.driveInput, c.brakeInput, c.steerInput = t["drive"], t["brake"], t["steer"]
        c.fixedDeltaTime = dt
        surf_rb = None
        if t["hit"] is not None:
            h = t["hit"]
            hh = Hit()
            hh.point, hh.normal, hh.forwardDir, hh.sidewaysDir, hh.force = tuple(h["point"]), tuple(h["normal"]), tuple(h["fwd"]), tuple(h["side"]), h["force"]
            if h["surf"] is not None:
                s = h["surf"]
                surf_rb = Rb(s["mass"], tuple(s["vel"]), tuple(s["ang"]), tuple(s["com"]), s["kinematic"])
            hh.collider = Collider(surf_rb, h["mat"])
            wc.col.hit = hh
        else:
            wc.col.hit = None
        for (fdt, ray) in t["frames"]:
            c.update(fdt, ray)
        c.fixed_update(dt)
        w = c.state
        wh = c.wheel
        col = wc.col
        row = [1 if w.grounded else 0, w.contactDepth, w.suspensionCompression, w.contactSpeed, w.damperForce, w.downforce, w.normalizedLoad, w.contactAngle]
        row += list(w.wheelVelocity) + list(w.localWheelVelocity) + list(w.surfaceForce) + list(w.localSurfaceForce) + list(w.externalTireForce)
        row += [w.angularVelocity] + list(w.tireForce) + [w.reactionTorque] + list(w.tireSlip) + [w.combinedTireSlip] + list(w.lastTireForce) + [w.driveTorque, w.brakeTorque]
        row += [wh.L, wh.Tr, wh.F[0], wh.F[1], 1 if wh.isResting else 0, wh.w, wh.load, wh.grip]
        row += [c.speed, c.speedAngle] + list(c.localAcceleration) + [rb.maxDepen]
        row += [col.center[0], col.center[1], col.center[2], col.suspensionDistance, col.radius, col.motorTorque, col.spring, col.damper, col.target, col.forceAppPointDistance,
                col.steerAngle, 1 if wc.canSleep else 0]
        row += [c.driveState, c.brakeState, c.steerState, w.steerAngle, SolverStatic.time]
        if w.hit is not None and w.grounded:
            row += list(w.hit.point) + list(w.hit.forwardDir) + list(w.hit.sidewaysDir)
        else:
            row += [0.0] * 9
        row += [wc.angularPosition, wc.contactDistance] + list(wc.suspensionPosition) + list(c.localDragVelocity)
        row += [wc.lastRuntimeSpringRate, wc.lastRuntimeDamperRate, wc.lastRuntimeSuspensionTravel, wc.m_steerAngle]
        row += [len(rb.forces)]
        row += (list(rb.forces[0][0]) + list(rb.forces[0][1])) if rb.forces else [0.0] * 6
        if surf_rb is not None:
            row += [len(surf_rb.forces)]
            row += (list(surf_rb.forces[0][0]) + list(surf_rb.forces[0][1])) if surf_rb.forces else [0.0] * 6
        else:
            row += [-1] + [0.0] * 6
        out.append(row)
    return out


ROW_COLS = (["grounded", "contactDepth", "compression", "contactSpeed", "damperForce", "downforce", "normalizedLoad", "contactAngle"]
            + ["wvx", "wvy", "wvz", "lvx", "lvy", "sfx", "sfy", "sfz", "lsfx", "lsfy", "etfx", "etfy"]
            + ["angVel", "tfx", "tfy", "reactionTorque", "tsx", "tsy", "combinedSlip", "ltfx", "ltfy", "driveTorque", "brakeTorque"]
            + ["L", "Tr", "Fx", "Fy", "isResting", "w", "load", "grip"]
            + ["speed", "speedAngle", "lax", "lay", "laz", "maxDepen"]
            + ["wcx", "wcy", "wcz", "wcSuspDist", "wcRadius", "motorTorque", "spring", "damper", "target", "forceAppPoint", "wcSteer", "canSleep"]
            + ["driveState", "brakeState", "steerState", "steerAngleState", "solverTime"]
            + ["hpx", "hpy", "hpz", "hfx", "hfy", "hfz", "hsx", "hsy", "hsz"]
            + ["angPos", "contactDist", "spx", "spy", "spz", "ldx", "ldy", "ldz"]
            + ["lastSpring", "lastDamper", "lastTravel", "mSteer"]
            + ["nf", "f0x", "f0y", "f0z", "p0x", "p0y", "p0z", "nfs", "g0x", "g0y", "g0z", "q0x", "q0y", "q0z"])


def main():
    rng = Rng(2902)
    out = {"format": 1, "note": "WP-29a independent reference; generated by tools/wp29a/vpp_reference.py"}
    sets, rows = friction_rows(rng, 60)
    out["friction"] = {"sets": enc(sets), "models": MODELS,
                       "cols": ["set", "model", "fm", "sx", "sy", "load", "grip", "fx", "fy", "adherent", "adhFwd", "adhSide", "afx", "afy", "adhBound", "peakBound", "limBound", "eval"],
                       "rows": enc(rows)}
    ws = wheel_scenarios(rng)
    out["wheel"] = {"cols": WHEEL_COLS, "nInputs": 13, "scenarios": [
        {"model": s["model"], "settings": enc(s["settings"]), "fm": jf(s["fm"]), "radius": jf(s["radius"]), "mass": jf(s["mass"]), "steps": s["steps"],
         "rk4": 1 if s["rk4"] else 0, "dt": jf(s["dt"]), "I": jf(s["I"]), "L0": jf(s["L0"]), "ticks": enc(s["ticks"])} for s in ws]}
    veh = []
    for fam in FAMILIES:
        for rep in range(2):
            spec = make_spec(rng, fam)
            ticks = make_ticks(rng, spec, 150)
            rows = run_vehicle(spec, ticks)
            veh.append({"spec": enc(spec), "ticks": enc(ticks), "rows": enc(rows)})
    out["vehicle"] = {"cols": ROW_COLS, "scenarios": veh}
    json.dump(out, sys.stdout, separators=(",", ":"))


if __name__ == "__main__":
    main()
