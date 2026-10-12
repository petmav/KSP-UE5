"""WP-54 slice 2 reference, part 0: shared helpers (no kspcore code). Written from the decompiled C# (Assembly-CSharp/Expansions/Missions/*.cs)."""
import importlib.util
import math
import os
import random
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(CORE, "tests", "data", "wp54b_reference.json")
sys.path.insert(0, os.path.join(CORE, "tools", "wp55"))

from ref_base import Cfg, f32, fnum, dnum, tag, vtext, parse, MISSIONS, ENUMS, CS_ROOT  # noqa: E402,F401
import ref_base  # noqa: E402

g9 = ref_base.r54.g9

import re as _re


def fmt(template, *args):
    """Localizer.Format with the managed Lingoona fallback: <<N>> and the gender-modified <<m:N>> insert argument N (text from the first '^' removed);
    arguments that are tags are translated first; an out of range N is removed; the result is unescaped (backslash-n, -quote, -t)."""
    tpl = ref_base.tag(template)
    if not args:
        out = tpl
    else:
        params = [ref_base.tag(a) for a in args]

        def sub(m):
            n = int(m.group(1))
            if 1 <= n <= len(params):
                return params[n - 1].split("^")[0]
            return ""

        out = _re.sub(r"<<(?:[A-Za-z]+:)?(\d+)(\^[^>]*)?>>", sub, tpl)
    return out.replace("\\n", "\n").replace('\\"', '"').replace("\\t", "\t")


f32bits = ref_base.f32bits


def b(x):
    return "True" if x else "False"


def dbits(x):
    return ref_base.dbits(x)


# ---- the synthetic world ---------------------------------------------------------------------------------------------------------
# (labelled synthetic: radii / SOI / gravity parameters are round stand-ins for the stock bodies, not game data)
BODIES = [
    {"name": "Kerbin", "display": "Kerbin^N", "radius": 600000.0, "soi": 84159286.0, "mu": 3.5316e12, "rot": 12.5, "drot": 7.25, "home": True},
    {"name": "Mun", "display": "Mun^N", "radius": 200000.0, "soi": 2429559.0, "mu": 6.5138398e10, "rot": 0.0, "drot": 0.0, "home": False},
    {"name": "Minmus", "display": "Minmus^N", "radius": 60000.0, "soi": 2247428.0, "mu": 1.7658e9, "rot": 3.5, "drot": 0.5, "home": False},
    {"name": "Sun", "display": "Sun^N", "radius": 261600000.0, "soi": None, "mu": 1.1723328e18, "rot": 0.0, "drot": 0.0, "home": False},
]
INF = float("inf")


def body(i):
    return BODIES[i]


def body_by_name(n):
    for i, bd in enumerate(BODIES):
        if bd["name"] == n:
            return i
    return None


HOME = 0


def soi(bd):
    return INF if bd["soi"] is None else bd["soi"]


def strip_gender(s):
    return s.split("^")[0] if "^" in s else s


# ---- number formats --------------------------------------------------------------------------------------------------------------
def _round_half_away(x, places):
    from decimal import Decimal, ROUND_HALF_UP
    return Decimal(x).quantize(Decimal(1).scaleb(-places), rounding=ROUND_HALF_UP)


def custom_0hh(x):
    """x.ToString("0.##") (non-tie, non-negative-zero values)."""
    d = _round_half_away(x, 2)
    s = format(d, "f")
    if "." in s:
        s = s.rstrip("0").rstrip(".")
    if s in ("-0", ""):
        s = "0"
    return s


def custom_0(x):
    d = _round_half_away(x, 0)
    s = format(d, "f")
    return "0" if s == "-0" else s


def custom_0h0(x):
    """x.ToString("0.0")"""
    d = _round_half_away(x, 1)
    return format(d, "f")


def nfmt(x, places):
    """x.ToString("N<places>") (invariant)."""
    d = _round_half_away(x, places)
    return "{:,.{p}f}".format(d, p=places)


def deg_vec(v):
    return ",".join(g9(c) for c in v)


# ---- float32 vector helpers (Mono rule: operands widened, the expression in double, the store rounds) -----------------------------
def v3(x, y, z):
    return [f32(x), f32(y), f32(z)]


def v3_sub(a, c):
    return [f32(a[0] - c[0]), f32(a[1] - c[1]), f32(a[2] - c[2])]


def v3_mag(a):
    return f32(math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]))


def v3_div(a, d):
    return [f32(a[0] / d), f32(a[1] / d), f32(a[2] / d)]


def v3_norm(a):
    m = v3_mag(a)
    if m > 1e-05:
        return v3_div(a, m)
    return [0.0, 0.0, 0.0]


def v3_cross(l, r):
    return [f32(l[1] * r[2] - l[2] * r[1]), f32(l[2] * r[0] - l[0] * r[2]), f32(l[0] * r[1] - l[1] * r[0])]


def qmul(a, c):
    x = a[3] * c[0] + a[0] * c[3] + a[1] * c[2] - a[2] * c[1]
    y = a[3] * c[1] + a[1] * c[3] + a[2] * c[0] - a[0] * c[2]
    z = a[3] * c[2] + a[2] * c[3] + a[0] * c[1] - a[1] * c[0]
    w = a[3] * c[3] - a[0] * c[0] - a[1] * c[1] - a[2] * c[2]
    return [f32(x), f32(y), f32(z), f32(w)]


def quat_text(q):
    """KSPUtil.WriteQuaternion(Quaternion): four G9 floats."""
    return ",".join(g9(c) for c in q)


# ---- the Unity native quaternion stand-ins used by BOTH sides of the create-action gate (not the natives: those are gated by WP-07 / WP-11) --
def stub_from_to(a, c):
    return [f32(a[0] + c[0]), f32(a[1] + c[1]), f32(a[2] + c[2]), 1.0]


def stub_angle_axis(angle, axis):
    return [f32(axis[0] * angle), f32(axis[1] * angle), f32(axis[2] * angle), f32(angle)]


def stub_look(f, u):
    return [f32(f[0] - u[0]), f32(f[1] - u[1]), f32(f[2] - u[2]), 2.0]


# ---- KSPRandom (KSPRandom.cs) ----------------------------------------------------------------------------------------------------
class KSPRandom:
    MBIG = 2147483647
    MSEED = 161803398

    def __init__(self, seed):
        s = [0] * 56
        num = self.MSEED - abs(seed)
        s[55] = num
        num2 = 1
        for i in range(1, 55):
            num3 = 21 * i % 55
            s[num3] = num2
            num2 = num - num2
            if num2 < 0:
                num2 += self.MBIG
            num = s[num3]
        for j in range(1, 5):
            for k in range(1, 56):
                s[k] -= s[1 + (k + 30) % 55]
                if s[k] < 0:
                    s[k] += self.MBIG
        self.s = s
        self.a = 0
        self.b = 31

    def sample(self):
        self.a += 1
        if self.a >= 56:
            self.a = 1
        self.b += 1
        if self.b >= 56:
            self.b = 1
        num = self.s[self.a] - self.s[self.b]
        if num < 0:
            num += self.MBIG
        self.s[self.a] = num
        return num * 4.6566128752458e-10

    def next_double(self):
        return self.sample()


def guid_of(i):
    return "6d650000-0000-0000-0000-%012d" % (i + 1)
