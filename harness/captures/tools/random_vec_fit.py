"""Fit UnityEngine.Random vector functions (native) to the trace_vec capture.

  python -I harness/captures/tools/random_vec_fit.py [--md report.md]

For each call we know the state before it, hence the raw uint32 draws r1..rn and the floats v_i = (r_i & 0x7FFFFF)/8388607f.
Candidate formulas are evaluated in float32 (each op rounded), with sin/cos/sqrt/pow evaluated in double and rounded
to float32 (a stand-in for the CRT's sinf/cosf/sqrtf/powf). Reported: for each candidate, how many components are
bit-exact and the max ULP distance.
"""
import math
import os
import struct
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hcutil as H  # noqa: E402
from random_model import XS, f32, VALUE  # noqa: E402

HERE = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
M32 = 0xFFFFFFFF
V = VALUE["(r & 0x7FFFFF) / 8388607f"]
PI = f32(math.pi)
TWO_PI = f32(2.0 * math.pi)


def rng(v, a, b):  # Random.Range(float) as fitted in C2: v*min + (1-v)*max
    return f32(f32(v * a) + f32(f32(1.0 - v) * b))


def fs(x):
    return f32(math.sin(x))


def fc(x):
    return f32(math.cos(x))


def fsq(x):
    return f32(math.sqrt(x)) if x >= 0 else math.nan


def m(a, b):
    return f32(a * b)


def ad(a, b):
    return f32(a + b)


def sb(a, b):
    return f32(a - b)


def unit_vec_a(v1, v2):  # z = Range(-1,1), a = Range(0, 2pi), r = sqrt(1-z*z), (r cos a, r sin a, z)
    z = rng(v1, -1.0, 1.0)
    a = rng(v2, 0.0, TWO_PI)
    r = fsq(sb(1.0, m(z, z)))
    return [m(r, fc(a)), m(r, fs(a)), z]


def unit_vec_b(v1, v2):  # same with value-based maps: z = 2v-1, a = 2pi v
    z = sb(m(2.0, v1), 1.0)
    a = m(TWO_PI, v2)
    r = fsq(sb(1.0, m(z, z)))
    return [m(r, fc(a)), m(r, fs(a)), z]


def unit_vec_c(v1, v2):  # z = Range(-1,1), a = Range(0,2pi) but a from first draw
    a = rng(v1, 0.0, TWO_PI)
    z = rng(v2, -1.0, 1.0)
    r = fsq(sb(1.0, m(z, z)))
    return [m(r, fc(a)), m(r, fs(a)), z]


def unit_vec_d(v1, v2):  # z = Range(-1,1), a = Range(-pi, pi)
    z = rng(v1, -1.0, 1.0)
    a = rng(v2, -PI, PI)
    r = fsq(sb(1.0, m(z, z)))
    return [m(r, fc(a)), m(r, fs(a)), z]


def inside_sphere(unit, v1, v2, v3, mode):
    u = unit(v1, v2)
    if mode == "cbrt_value":
        s = f32(math.pow(v3, f32(1.0 / 3.0)))
    elif mode == "cbrt_range01":
        s = f32(math.pow(rng(v3, 0.0, 1.0), f32(1.0 / 3.0)))
    else:
        s = rng(v3, 0.0, 1.0)
    return [m(c, s) for c in u]


def quat_norm(q):
    n2 = f32(f32(f32(m(q[0], q[0]) + m(q[1], q[1])) + m(q[2], q[2])) + m(q[3], q[3]))
    n = fsq(n2)
    return [f32(c / n) for c in q]


def quat_norm_inv(q):
    n2 = f32(f32(f32(m(q[0], q[0]) + m(q[1], q[1])) + m(q[2], q[2])) + m(q[3], q[3]))
    inv = f32(1.0 / fsq(n2))
    return [m(c, inv) for c in q]


def wpos(q):
    return [-c for c in q] if q[3] < 0 else q


def rotation_cands(vs):
    out = {}
    s1 = [rng(v, -1.0, 1.0) for v in vs]
    s2 = [sb(m(2.0, v), 1.0) for v in vs]
    out["Normalize(Range(-1,1) x4), /n"] = quat_norm(s1)
    out["Normalize(Range(-1,1) x4), *1/n"] = quat_norm_inv(s1)
    out["Normalize(2v-1 x4), /n"] = quat_norm(s2)
    out["Normalize(2v-1 x4), *1/n"] = quat_norm_inv(s2)
    for k in list(out):
        out[k + ", then w>=0"] = wpos(out[k])
    return out


def uniform_cands(vs):
    out = {}
    for name, (u1, u2, u3) in {
        "u1=v1,u2=2pi v2,u3=2pi v3": (vs[0], m(TWO_PI, vs[1]), m(TWO_PI, vs[2])),
        "u1=Range01,u2=Range(0,2pi),u3=Range(0,2pi)": (rng(vs[0], 0.0, 1.0), rng(vs[1], 0.0, TWO_PI), rng(vs[2], 0.0, TWO_PI)),
    }.items():
        a = fsq(sb(1.0, u1))
        b = fsq(u1)
        out[name + " (a sin u2, a cos u2, b sin u3, b cos u3)"] = [m(a, fs(u2)), m(a, fc(u2)), m(b, fs(u3)), m(b, fc(u3))]
        out[name + " (b sin u3, b cos u3, a sin u2, a cos u2)"] = [m(b, fs(u3)), m(b, fc(u3)), m(a, fs(u2)), m(a, fc(u2))]
        out[name + " (a sin u2, a cos u2, b sin u3, b cos u3) xyzw->wxyz"] = [m(a, fc(u2)), m(b, fs(u3)), m(b, fc(u3)), m(a, fs(u2))]
    for k in list(out):
        out[k + ", then w>=0"] = wpos(out[k])
    return out


def circle_cands(vs):
    out = {}
    for name, (t, r2) in {
        "theta=Range(0,2pi) v1, r=sqrt(Range01 v2)": (rng(vs[0], 0.0, TWO_PI), rng(vs[1], 0.0, 1.0)),
        "theta=2pi v1, r=sqrt(v2)": (m(TWO_PI, vs[0]), vs[1]),
        "theta=Range(0,2pi) v2, r=sqrt(Range01 v1)": (rng(vs[1], 0.0, TWO_PI), rng(vs[0], 0.0, 1.0)),
        "theta=2pi v2, r=sqrt(v1)": (m(TWO_PI, vs[1]), vs[0]),
    }.items():
        r = fsq(r2)
        out[name + " (r cos, r sin)"] = [m(r, fc(t)), m(r, fs(t))]
        out[name + " (r sin, r cos)"] = [m(r, fs(t)), m(r, fc(t))]
    return out


def main(argv):
    d = H.load(os.path.join(HERE, "out", "unity_random.json"))
    tv = H.rows_as_dicts(H.table(d, "trace_vec"))
    ndraw = {"insideUnitSphere": 3, "onUnitSphere": 2, "rotation": 4, "rotationUniform": 3, "insideUnitCircle": 2, "ColorHSV()": 4}
    score = defaultdict(lambda: [0, 0, 0])  # (fn, cand) -> [components, exact, maxulp]
    for r in tv:
        x = XS(0)
        x.s = [v & M32 for v in (r["s0"], r["s1"], r["s2"], r["s3"])]
        vs = [V(x.next()) for _ in range(ndraw[r["fn"]])]
        got = [r[k] for k in ("x", "y", "z", "w") if r[k] is not None]
        fn = r["fn"]
        cands = {}
        if fn == "onUnitSphere":
            for nm, f in (("A z=Range(-1,1) v1, a=Range(0,2pi) v2", unit_vec_a), ("B z=2v1-1, a=2pi v2", unit_vec_b), ("C a from v1, z from v2", unit_vec_c), ("D z=Range(-1,1), a=Range(-pi,pi)", unit_vec_d)):
                cands[nm] = f(vs[0], vs[1])
        elif fn == "insideUnitSphere":
            for nm, f in (("A", unit_vec_a), ("B", unit_vec_b), ("C", unit_vec_c), ("D", unit_vec_d)):
                for mode in ("cbrt_value", "cbrt_range01", "linear_range01"):
                    cands["unit %s * %s(v3)" % (nm, mode)] = inside_sphere(f, vs[0], vs[1], vs[2], mode)
        elif fn == "rotation":
            cands = rotation_cands(vs)
        elif fn == "rotationUniform":
            cands = uniform_cands(vs)
        elif fn == "insideUnitCircle":
            cands = circle_cands(vs)
        else:
            continue
        for nm, c in cands.items():
            st = score[(fn, nm)]
            for a, b in zip(c, got):
                st[0] += 1
                if not (isinstance(a, float) and math.isfinite(a)):
                    continue
                if H.f32_bits(a) == H.f32_bits(b):
                    st[1] += 1
                else:
                    st[2] = max(st[2], H.ulp_dist32(a, b))
    lines = ["# C2 vector functions: fitted formulas (float32 emulation, sin/cos/sqrt/pow rounded from double)", "",
             "| fn | candidate | components | bit-exact | max ULP |", "|---|---|---|---|---|"]
    best = {}
    for (fn, nm), (n, ex, mu) in sorted(score.items(), key=lambda kv: (kv[0][0], -kv[1][1], kv[1][2])):
        lines.append("| %s | %s | %d | %d | %d |" % (fn, nm, n, ex, mu))
        if fn not in best:
            best[fn] = (nm, n, ex, mu)
    lines.append("")
    lines.append("Best per function:")
    for fn, (nm, n, ex, mu) in best.items():
        lines.append("- %s: %s: %d/%d bit-exact, max %d ULP" % (fn, nm, ex, n, mu))
    text = "\n".join(lines) + "\n"
    if "--md" in argv:
        with open(argv[argv.index("--md") + 1], "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
    print(text)


if __name__ == "__main__":
    main(sys.argv[1:])
