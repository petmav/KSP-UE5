"""WP-32 reference helpers: the Mono float rule and the managed UnityEngine math the EVA code uses, translated from the decompiled UnityEngine sources
(Vector3.cs, Quaternion.cs, Mathf.cs). Floats are Python floats holding exact float32 values; `f32` is the store rounding.

Mono float rule (harness/captures C7): a float expression is evaluated in double; it is rounded to float32 only when stored (local, field, argument,
return, array element) or explicitly cast. Every function below spells the C# statements, so the roundings sit where the C# stores.
"""
import math

import numpy as np


def f32(x):
    return float(np.float32(x))


def bits(x):
    return int(np.float32(x).view(np.uint32))


def unbits(u):
    return float(np.uint32(u).view(np.float32))


# ---- Mathf (Mathf.cs) ----
def clamp01(v):
    if v < 0.0:
        return 0.0
    if v > 1.0:
        return 1.0
    return v


def clamp(v, lo, hi):
    if v < lo:
        v = lo
    elif v > hi:
        v = hi
    return v


def lerp(a, b, t):
    return f32(a + (b - a) * clamp01(t))


def inverse_lerp(a, b, v):
    if a != b:
        return clamp01(f32((v - a) / (b - a)))
    return 0.0


def sign(f):
    return 1.0 if f >= 0.0 else -1.0


def mabs(f):
    return abs(f)


def mmin(a, b):
    return a if a < b else b


def mmax(a, b):
    return a if a > b else b


def mpow(f, p):
    return f32(math.pow(f, p))


def mround(f):
    # Math.Round(double) is banker's rounding (round half to even); Python's round() on floats is the same rule
    return f32(float(round(f)))


def macos(f):
    return f32(math.acos(f))


# ---- Vector3 (Vector3.cs): tuples of three float32 values ----
def vadd(a, b):
    return (f32(a[0] + b[0]), f32(a[1] + b[1]), f32(a[2] + b[2]))


def vsub(a, b):
    return (f32(a[0] - b[0]), f32(a[1] - b[1]), f32(a[2] - b[2]))


def vmul(a, d):
    return (f32(a[0] * d), f32(a[1] * d), f32(a[2] * d))


def vdiv(a, d):
    return (f32(a[0] / d), f32(a[1] / d), f32(a[2] / d))


def vneg(a):
    return (f32(-a[0]), f32(-a[1]), f32(-a[2]))


def dot(a, b):
    return f32(a[0] * b[0] + a[1] * b[1] + a[2] * b[2])


def cross(l, r):
    return (f32(l[1] * r[2] - l[2] * r[1]), f32(l[2] * r[0] - l[0] * r[2]), f32(l[0] * r[1] - l[1] * r[0]))


def sqr_magnitude(a):
    return f32(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])


def magnitude(a):
    return f32(math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]))


def normalize(v):
    num = magnitude(v)
    if num > f32(1e-05):
        return vdiv(v, num)
    return (0.0, 0.0, 0.0)


def clamp_magnitude(v, max_length):
    num = sqr_magnitude(v)
    if num > max_length * max_length:
        num2 = f32(math.sqrt(num))
        num3 = f32(v[0] / num2)
        num4 = f32(v[1] / num2)
        num5 = f32(v[2] / num2)
        return (f32(num3 * max_length), f32(num4 * max_length), f32(num5 * max_length))
    return v


def vlerp(a, b, t):
    t = clamp01(t)
    return (f32(a[0] + (b[0] - a[0]) * t), f32(a[1] + (b[1] - a[1]) * t), f32(a[2] + (b[2] - a[2]) * t))


def project_on_plane(v, n):
    num = dot(n, n)
    if num < f32(1.401298464324817e-45):
        return v
    num2 = dot(v, n)
    return (f32(v[0] - n[0] * num2 / num), f32(v[1] - n[1] * num2 / num), f32(v[2] - n[2] * num2 / num))


def vangle(a, b):
    num = f32(math.sqrt(sqr_magnitude(a) * sqr_magnitude(b)))
    if num < f32(1e-15):
        return 0.0
    num2 = clamp(f32(dot(a, b) / num), -1.0, 1.0)
    return f32(f32(math.acos(num2)) * f32(57.29578))


def signed_angle(a, b, axis):
    num = vangle(a, b)
    num2 = f32(a[1] * b[2] - a[2] * b[1])
    num3 = f32(a[2] * b[0] - a[0] * b[2])
    num4 = f32(a[0] * b[1] - a[1] * b[0])
    num5 = sign(f32(axis[0] * num2 + axis[1] * num3 + axis[2] * num4))
    return f32(num * num5)


def v_approx_zero(v):
    # Vector3.operator== (approximate): (lhs - rhs).sqrMagnitude < 9.99999944E-11
    num = f32(v[0] - 0.0)
    num2 = f32(v[1] - 0.0)
    num3 = f32(v[2] - 0.0)
    return f32(num * num + num2 * num2 + num3 * num3) < f32(9.99999944e-11)


# ---- Quaternion (Quaternion.cs): (x, y, z, w) ----
def qmul_vec(q, p):
    num = f32(q[0] * 2.0)
    num2 = f32(q[1] * 2.0)
    num3 = f32(q[2] * 2.0)
    num4 = f32(q[0] * num)
    num5 = f32(q[1] * num2)
    num6 = f32(q[2] * num3)
    num7 = f32(q[0] * num2)
    num8 = f32(q[0] * num3)
    num9 = f32(q[1] * num3)
    num10 = f32(q[3] * num)
    num11 = f32(q[3] * num2)
    num12 = f32(q[3] * num3)
    x = f32((1.0 - (num5 + num6)) * p[0] + (num7 - num12) * p[1] + (num8 + num11) * p[2])
    y = f32((num7 + num12) * p[0] + (1.0 - (num4 + num6)) * p[1] + (num9 - num10) * p[2])
    z = f32((num8 - num11) * p[0] + (num9 + num10) * p[1] + (1.0 - (num4 + num5)) * p[2])
    return (x, y, z)


def qconj(q):
    # Quaternion.Inverse of a unit quaternion (native; conjugate for |q| = 1 -- the inputs of the tests are exactly unit)
    return (f32(-q[0]), f32(-q[1]), f32(-q[2]), q[3])


def axes(q):
    """transform.up / forward / right of a rotation (rotation * unit axis)"""
    return (qmul_vec(q, (0.0, 1.0, 0.0)), qmul_vec(q, (0.0, 0.0, 1.0)), qmul_vec(q, (1.0, 0.0, 0.0)))


# the rotations the tests use: their axes are exact (components 0 / +-1)
ROTATIONS = [
    (0.0, 0.0, 0.0, 1.0),
    (0.5, 0.5, 0.5, 0.5),
    (0.5, -0.5, -0.5, 0.5),
    (0.0, 1.0, 0.0, 0.0),
    (0.0, 0.0, 1.0, 0.0),
    (-0.5, 0.5, -0.5, 0.5),
]


def F3(t):
    """a literal vector rounded to float32 (inputs are float32 values)"""
    return tuple(f32(x) for x in t)
