"""WP-12a reference, part 1: the runtime numerics (see tools/wp12/pqs_reference.py). No kspcore code.

Mono float rule helpers for managed Unity code, KSP's Vector3d / QuaternionD maths, native Unity Matrix4x4.TRS /
Vector3.OrthoNormalize / Quaternion.FromToRotation (per-operation float32; check_natives() compares them with the in-game
C3 capture), System.Math through the game runtime's VS2013 CRT (ctypes) and the x87 FSIN / FCOS model (check_x87()
compares it with the in-game libm capture).
"""
import argparse
import ctypes
import json
import math
import os
import struct
import subprocess
import sys
import tempfile

import mpmath

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
ROOT = CORE
while not os.path.exists(os.path.join(ROOT, "harness", "compare", "compare.py")):
    parent = os.path.dirname(ROOT)
    if parent == ROOT:
        raise SystemExit("project root not found")
    ROOT = parent

# =====================================================================================================================
# runtime numerics
# =====================================================================================================================
_FS = struct.Struct("<f")
_DS = struct.Struct("<d")
INF = float("inf")
FLT_MAX = 3.4028234663852886e38


def f32(x):
    """round a double to float32 (IEEE round to nearest even), as a Python float"""
    try:
        return _FS.unpack(_FS.pack(x))[0]
    except OverflowError:
        # beyond FLT_MAX + half an ulp: infinity; between FLT_MAX and that boundary: FLT_MAX
        if abs(x) < 3.4028235677973366e38:
            return math.copysign(FLT_MAX, x)
        return math.copysign(INF, x)


def fbits(x):
    return struct.unpack("<I", _FS.pack(x))[0]


def dbits(x):
    return struct.unpack("<Q", _DS.pack(x))[0]


def from_fhex(h):
    return struct.unpack("<f", int(h, 16).to_bytes(4, "little"))[0]


def from_dhex(h):
    return struct.unpack("<d", int(h, 16).to_bytes(8, "little"))[0]


# ---- System.Math through the game runtime's CRT (msvcr120_clr0400.dll) ------------------------------------------------
_crt = None
for _name in ("msvcr120_clr0400.dll", "msvcr120.dll"):
    try:
        _crt = ctypes.CDLL(os.path.join(os.environ.get("SystemRoot", r"C:\Windows"), "System32", _name))
        break
    except OSError:
        pass
if _crt is None:
    raise SystemExit("the VS2013 CRT (msvcr120_clr0400.dll) is needed (Windows with .NET Framework 4.x)")
for _fn in ("acos", "asin", "atan"):
    getattr(_crt, _fn).restype = ctypes.c_double
    getattr(_crt, _fn).argtypes = [ctypes.c_double]
_crt.pow.restype = ctypes.c_double
_crt.pow.argtypes = [ctypes.c_double, ctypes.c_double]


def Acos(x):
    return _crt.acos(x)


def Asin(x):
    return _crt.asin(x)


def Atan(x):
    return _crt.atan(x)


def Pow(x, y):
    # Mono: a zero base with a negative exponent is +Infinity (capture: Mathf.Pow(-0f, -3f))
    if x == 0.0 and y < 0.0:
        return INF
    return _crt.pow(x, y)


# ---- x87 FSIN / FCOS (66-bit pi) ---------------------------------------------------------------------------------------
mpmath.mp.prec = 320
_PI87_HALF = (mpmath.floor(mpmath.pi * mpmath.mpf(2) ** 64) / mpmath.mpf(2) ** 64) / 2


def _x87(fn, x):
    if x != x or x in (INF, -INF):
        return float("nan")
    if x == 0.0:
        return x if fn == "sin" else 1.0
    if abs(x) >= 2.0 ** 63:
        return x  # out of range: FSIN / FCOS leave the operand (C2 set)
    X = mpmath.mpf(x)
    k = int(mpmath.nint(X / _PI87_HALF))
    r = X - k * _PI87_HALF
    k %= 4
    if fn == "sin":
        v = (mpmath.sin(r), mpmath.cos(r), -mpmath.sin(r), -mpmath.cos(r))[k]
    else:
        v = (mpmath.cos(r), -mpmath.sin(r), -mpmath.cos(r), mpmath.sin(r))[k]
    return float(v)


def Sin(x):
    return _x87("sin", x)


def Cos(x):
    return _x87("cos", x)


def check_x87():
    d = json.load(open(os.path.join(ROOT, "harness", "captures", "out", "libm_mono.json"), encoding="utf-8"))
    t = [t for t in d["tables"] if t["name"] == "f64"][0]
    cols = [c["name"] for c in t["columns"]]
    n = 0
    for row in t["rows"]:
        r = dict(zip(cols, row))
        if r["fn"] not in ("Math.Sin", "Math.Cos"):
            continue
        x = from_dhex(r["x_hex"])
        if x != x or x in (INF, -INF):
            continue
        got = Sin(x) if r["fn"] == "Math.Sin" else Cos(x)
        if dbits(got) != dbits(from_dhex(r["r_hex"])):
            raise SystemExit("x87 model mismatch: %s(%r) = %r, game %r" % (r["fn"], x, got, from_dhex(r["r_hex"])))
        n += 1
    return n


# ---- managed UnityEngine vectors (Mono rule) ---------------------------------------------------------------------------
# A Vector3 / Vector2 / Vector4 / Color is a tuple of Python floats that are float32 values.
V3ZERO = (0.0, 0.0, 0.0)


def v3add(a, b):  # Vector3.cs:415
    return (f32(a[0] + b[0]), f32(a[1] + b[1]), f32(a[2] + b[2]))


def v3mul(a, d):  # Vector3.cs:430
    return (f32(a[0] * d), f32(a[1] * d), f32(a[2] * d))


def v3div(a, d):  # Vector3.cs:440
    return (f32(a[0] / d), f32(a[1] / d), f32(a[2] / d))


def v3mag(v):  # Vector3.cs:395 (the sum is a double expression)
    return f32(math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]))


def v3normalized(v):  # Vector3.cs:302
    m = v3mag(v)
    if m > f32(1e-05):
        return v3div(v, m)
    return V3ZERO


def v3dot(a, b):  # Vector3.cs:325
    return f32(a[0] * b[0] + a[1] * b[1] + a[2] * b[2])


def v3cross(l, r):  # Vector3.cs:272
    return (f32(l[1] * r[2] - l[2] * r[1]), f32(l[2] * r[0] - l[0] * r[2]), f32(l[0] * r[1] - l[1] * r[0]))


def d2f(v):  # Vector3d -> Vector3 (Vector3d.cs:97)
    return (f32(v[0]), f32(v[1]), f32(v[2]))


def mul_point3x4(m, p):  # Matrix4x4.cs:424 MultiplyPoint3x4 (m[row][col], Mono rule)
    return (f32(m[0][0] * p[0] + m[0][1] * p[1] + m[0][2] * p[2] + m[0][3]),
            f32(m[1][0] * p[0] + m[1][1] * p[1] + m[1][2] * p[2] + m[1][3]),
            f32(m[2][0] * p[0] + m[2][1] * p[1] + m[2][2] * p[2] + m[2][3]))


# ---- KSP Vector3d / QuaternionD (double) -------------------------------------------------------------------------------
def dadd(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def dsub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def dmul(a, d):
    return (a[0] * d, a[1] * d, a[2] * d)


def ddot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def dmag(a):
    return math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])


def dsqrmag(a):
    return a[0] * a[0] + a[1] * a[1] + a[2] * a[2]


def dnormalized(a):  # Vector3d.cs:176
    n = math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])
    if n > 0.0:
        return (a[0] / n, a[1] / n, a[2] / n)
    return (0.0, 0.0, 0.0)


def qd_mul_v(q, p):  # QuaternionD.cs:465
    x, y, z, w = q
    num = x * 2.0
    num2 = y * 2.0
    num3 = z * 2.0
    num4 = x * num
    num5 = y * num2
    num6 = z * num3
    num7 = x * num2
    num8 = x * num3
    num9 = y * num3
    num10 = w * num
    num11 = w * num2
    num12 = w * num3
    return ((1.0 - (num5 + num6)) * p[0] + (num7 - num12) * p[1] + (num8 + num11) * p[2],
            (num7 + num12) * p[0] + (1.0 - (num4 + num6)) * p[1] + (num9 - num10) * p[2],
            (num8 - num11) * p[0] + (num9 + num10) * p[1] + (1.0 - (num4 + num5)) * p[2])


# ---- native Unity (per-operation float32) ------------------------------------------------------------------------------
def nf_dot(a, b):
    return f32(f32(f32(a[0] * b[0]) + f32(a[1] * b[1])) + f32(a[2] * b[2]))


def nf_sqrt(x):
    return f32(math.sqrt(x))


def native_trs(pos, q, s):
    """Matrix4x4.TRS (MatrixScripting::TRS): QuaternionToMatrix, columns scaled; returns m[row][col]"""
    x, y, z, w = q
    x2 = f32(x * 2.0)
    y2 = f32(y * 2.0)
    z2 = f32(z * 2.0)
    xx = f32(x * x2)
    yy = f32(y * y2)
    zz = f32(z * z2)
    xy = f32(x * y2)
    xz = f32(x * z2)
    yz = f32(y * z2)
    wx = f32(w * x2)
    wy = f32(w * y2)
    wz = f32(w * z2)
    r = [[0.0] * 4 for _ in range(4)]
    r[0][0] = f32(f32(1.0 - f32(yy + zz)) * s[0])
    r[1][0] = f32(f32(xy + wz) * s[0])
    r[2][0] = f32(f32(xz - wy) * s[0])
    r[0][1] = f32(f32(xy - wz) * s[1])
    r[1][1] = f32(f32(1.0 - f32(xx + zz)) * s[1])
    r[2][1] = f32(f32(yz + wx) * s[1])
    r[0][2] = f32(f32(xz + wy) * s[2])
    r[1][2] = f32(f32(yz - wx) * s[2])
    r[2][2] = f32(f32(1.0 - f32(xx + yy)) * s[2])
    r[0][3] = pos[0]
    r[1][3] = pos[1]
    r[2][3] = pos[2]
    r[3][3] = 1.0
    return r


def ortho_normal_vector_fast(n):
    if abs(n[2]) > f32(0.70710678118654752440):
        a = f32(f32(n[1] * n[1]) + f32(n[2] * n[2]))
        k = f32(1.0 / nf_sqrt(a))
        return (0.0, f32(-n[2] * k), f32(n[1] * k))
    a = f32(f32(n[0] * n[0]) + f32(n[1] * n[1]))
    k = f32(1.0 / nf_sqrt(a))
    return (f32(-n[1] * k), f32(n[0] * k), 0.0)


def native_orthonormalize(u, v):
    """Vector3.OrthoNormalize(ref u, ref v) (VectorScripting::OrthoNormalize); returns (u, v)"""
    mag = nf_sqrt(nf_dot(u, u))
    if mag > f32(1e-05):
        u = (f32(u[0] / mag), f32(u[1] / mag), f32(u[2] / mag))
    else:
        u = (1.0, 0.0, 0.0)
    d0 = nf_dot(u, v)
    v = (f32(v[0] - f32(d0 * u[0])), f32(v[1] - f32(d0 * u[1])), f32(v[2] - f32(d0 * u[2])))
    mag = nf_sqrt(nf_dot(v, v))
    if mag < f32(1e-05):
        v = ortho_normal_vector_fast(u)
    else:
        v = (f32(v[0] / mag), f32(v[1] / mag), f32(v[2] / mag))
    return u, v


def native_from_to_rotation(frm, to):
    """Quaternion.FromToRotation: FromToQuaternionSafe -> Moller-Hughes FromToRotation matrix -> MatrixToQuaternion
    (Shoemake) -> Normalize, all per-operation float32"""
    lm = nf_sqrt(nf_dot(frm, frm))
    rm = nf_sqrt(nf_dot(to, to))
    if lm < f32(1e-05) or rm < f32(1e-05):
        return (0.0, 0.0, 0.0, 1.0)
    f = (f32(frm[0] / lm), f32(frm[1] / lm), f32(frm[2] / lm))
    t = (f32(to[0] / rm), f32(to[1] / rm), f32(to[2] / rm))
    v = (f32(f32(f[1] * t[2]) - f32(f[2] * t[1])), f32(f32(f[2] * t[0]) - f32(f[0] * t[2])),
         f32(f32(f[0] * t[1]) - f32(f[1] * t[0])))
    e = nf_dot(f, t)
    m = [[0.0] * 3 for _ in range(3)]
    if e > 1.0 - 0.000001:
        m = [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]]
    elif e < -1.0 + 0.000001:
        left = [0.0, f[2], f32(-f[1])]
        if nf_dot(left, left) < 0.000001:
            left = [f32(-f[2]), 0.0, f[0]]
        invlen = f32(1.0 / nf_sqrt(nf_dot(left, left)))
        left = [f32(left[0] * invlen), f32(left[1] * invlen), f32(left[2] * invlen)]
        up = [f32(f32(left[1] * f[2]) - f32(left[2] * f[1])), f32(f32(left[2] * f[0]) - f32(left[0] * f[2])),
              f32(f32(left[0] * f[1]) - f32(left[1] * f[0]))]
        fxx = f32(-f[0] * f[0])
        fyy = f32(-f[1] * f[1])
        fzz = f32(-f[2] * f[2])
        fxy = f32(-f[0] * f[1])
        fxz = f32(-f[0] * f[2])
        fyz = f32(-f[1] * f[2])
        uxx = f32(up[0] * up[0])
        uyy = f32(up[1] * up[1])
        uzz = f32(up[2] * up[2])
        uxy = f32(up[0] * up[1])
        uxz = f32(up[0] * up[2])
        uyz = f32(up[1] * up[2])
        lxx = f32(-left[0] * left[0])
        lyy = f32(-left[1] * left[1])
        lzz = f32(-left[2] * left[2])
        lxy = f32(-left[0] * left[1])
        lxz = f32(-left[0] * left[2])
        lyz = f32(-left[1] * left[2])
        m[0][0] = f32(f32(fxx + uxx) + lxx)
        m[0][1] = f32(f32(fxy + uxy) + lxy)
        m[0][2] = f32(f32(fxz + uxz) + lxz)
        m[1][0] = m[0][1]
        m[1][1] = f32(f32(fyy + uyy) + lyy)
        m[1][2] = f32(f32(fyz + uyz) + lyz)
        m[2][0] = m[0][2]
        m[2][1] = m[1][2]
        m[2][2] = f32(f32(fzz + uzz) + lzz)
    else:
        h = f32(f32(1.0 - e) / nf_dot(v, v))
        hvx = f32(h * v[0])
        hvz = f32(h * v[2])
        hvxy = f32(hvx * v[1])
        hvxz = f32(hvx * v[2])
        hvyz = f32(hvz * v[1])
        m[0][0] = f32(e + f32(hvx * v[0]))
        m[0][1] = f32(hvxy - v[2])
        m[0][2] = f32(hvxz + v[1])
        m[1][0] = f32(hvxy + v[2])
        m[1][1] = f32(e + f32(f32(h * v[1]) * v[1]))
        m[1][2] = f32(hvyz - v[0])
        m[2][0] = f32(hvxz - v[1])
        m[2][1] = f32(hvyz + v[0])
        m[2][2] = f32(e + f32(hvz * v[2]))
    # MatrixToQuaternion (Get(row, col) = m[row][col])
    trace = f32(f32(m[0][0] + m[1][1]) + m[2][2])
    q = [0.0, 0.0, 0.0, 0.0]
    if trace > 0.0:
        root = nf_sqrt(f32(trace + 1.0))
        q[3] = f32(0.5 * root)
        root = f32(0.5 / root)
        q[0] = f32(f32(m[2][1] - m[1][2]) * root)
        q[1] = f32(f32(m[0][2] - m[2][0]) * root)
        q[2] = f32(f32(m[1][0] - m[0][1]) * root)
    else:
        nxt = (1, 2, 0)
        i = 0
        if m[1][1] > m[0][0]:
            i = 1
        if m[2][2] > m[i][i]:
            i = 2
        j = nxt[i]
        k = nxt[j]
        root = nf_sqrt(f32(f32(f32(m[i][i] - m[j][j]) - m[k][k]) + 1.0))
        q[i] = f32(0.5 * root)
        root = f32(0.5 / root)
        q[3] = f32(f32(m[k][j] - m[j][k]) * root)
        q[j] = f32(f32(m[j][i] + m[i][j]) * root)
        q[k] = f32(f32(m[k][i] + m[i][k]) * root)
    # Quaternion Normalize: q / Sqrt(Dot(q, q))
    d = f32(f32(f32(f32(q[0] * q[0]) + f32(q[1] * q[1])) + f32(q[2] * q[2])) + f32(q[3] * q[3]))
    mag = nf_sqrt(d)
    return (f32(q[0] / mag), f32(q[1] / mag), f32(q[2] / mag), f32(q[3] / mag))


def check_natives():
    d = json.load(open(os.path.join(ROOT, "harness", "captures", "out", "math_shim_native.json"), encoding="utf-8"))
    tab = d["tables"][0]
    cols = [c["name"] for c in tab["columns"]]
    counts = {"Matrix4x4.TRS": 0, "Vector3.OrthoNormalize": 0, "Quaternion.FromToRotation": 0}
    for row in tab["rows"]:
        r = dict(zip(cols, row))
        fn = r["fn"]
        if fn not in counts:
            continue
        a = [from_fhex(r["a%d_hex" % i]) if r.get("a%d_hex" % i) else None for i in range(14)]
        res = [from_fhex(r["r%d_hex" % i]) if r.get("r%d_hex" % i) else None for i in range(16)]
        if fn == "Matrix4x4.TRS":
            m = native_trs(a[0:3], a[3:7], a[7:10])
            mine = [m[i % 4][i // 4] for i in range(16)]
        elif fn == "Vector3.OrthoNormalize":
            u, v = native_orthonormalize(tuple(a[0:3]), tuple(a[3:6]))
            mine = list(u) + list(v)
        else:
            mine = list(native_from_to_rotation(tuple(a[0:3]), tuple(a[3:6])))
        for i, x in enumerate(mine):
            if fbits(x) != fbits(res[i]):
                raise SystemExit("native %s row %d: r%d %r != game %r" % (fn, r["i"], i, x, res[i]))
        counts[fn] += 1
    return counts

