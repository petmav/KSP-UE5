"""C3 sanity/convention check: native Unity maths (math_shim_native.json) vs textbook formulas evaluated in double.

  python -I harness/captures/tools/c3_check.py [--md report.md]

There is no harness golden for the native InternalCalls; the capture itself is the golden for the C++ port. This script
confirms the conventions (Euler order, quaternion layout, TRS layout, Transform semantics, Inverse normalisation) and
reports how far the native float32 results are from a double-precision evaluation of each formula (float32 ULPs). It
does not claim bit-exactness of any formula.
"""
import math
import os
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hcutil as H  # noqa: E402

HERE = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
D2R = math.pi / 180.0


def qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by + ay * bw + az * bx - ax * bz,
            aw * bz + az * bw + ax * by - ay * bx,
            aw * bw - ax * bx - ay * by - az * bz)


def axis_angle(deg, axis):
    n = math.sqrt(sum(c * c for c in axis))
    if n == 0:
        return (0.0, 0.0, 0.0, 1.0)
    h = deg * D2R * 0.5
    s = math.sin(h)
    return (axis[0] / n * s, axis[1] / n * s, axis[2] / n * s, math.cos(h))


def euler(x, y, z):  # Unity: rotate Z, then X, then Y  =>  q = qy * qx * qz
    return qmul(qmul(axis_angle(y, (0, 1, 0)), axis_angle(x, (1, 0, 0))), axis_angle(z, (0, 0, 1)))


def rotate(q, v):
    qv = (v[0], v[1], v[2], 0.0)
    qc = (-q[0], -q[1], -q[2], q[3])
    r = qmul(qmul(q, qv), qc)
    n2 = sum(c * c for c in q)
    return tuple(c / n2 for c in r[:3])


def quat_matrix(q):
    x, y, z, w = q
    return [[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
            [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
            [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]]


def trs(p, q, s):
    r = quat_matrix(q)
    m = [[r[i][j] * s[j] for j in range(3)] + [p[i]] for i in range(3)] + [[0.0, 0.0, 0.0, 1.0]]
    return [m[row][col] for col in range(4) for row in range(4)]  # Unity Matrix4x4 indexer: column-major m[i] = m[i%4, i/4]


def inv4(m):  # m: list of 16 column-major
    a = [[m[c * 4 + r] for c in range(4)] for r in range(4)]
    n = 4
    aug = [row[:] + [1.0 if i == j else 0.0 for j in range(n)] for i, row in enumerate(a)]
    for col in range(n):
        piv = max(range(col, n), key=lambda r: abs(aug[r][col]))
        aug[col], aug[piv] = aug[piv], aug[col]
        pv = aug[col][col]
        if pv == 0:
            return None
        aug[col] = [v / pv for v in aug[col]]
        for r in range(n):
            if r != col:
                f = aug[r][col]
                aug[r] = [vr - f * vc for vr, vc in zip(aug[r], aug[col])]
    inv = [row[n:] for row in aug]
    return [inv[r][c] for c in range(4) for r in range(4)]


def ulps(ref, got):
    """float32 ULPs between double reference and float32 result (sign-insensitive near 0 via abs diff / ulp(ref))."""
    if ref is None or got is None or math.isnan(got):
        return None
    f = H.exact_f32(repr(ref)) if math.isfinite(ref) else ref
    if not math.isfinite(f):
        return None
    if abs(f) < 1e-30:
        return abs(got - ref) / 1.4e-45 if got != 0 else 0.0
    e = math.frexp(f)[1]
    return abs(got - ref) / math.ldexp(1.0, e - 24)


def main(argv):
    d = H.load(os.path.join(HERE, "out", "math_shim_native.json"))
    rows = H.rows_as_dicts(H.table(d, "f32"))
    by = defaultdict(list)
    for r in rows:
        a = [r["a%d" % i] for i in range(14) if r["a%d" % i] is not None]
        o = [r["r%d" % i] for i in range(16) if r["r%d" % i] is not None]
        by[r["fn"]].append((a, o))
    res = {}

    def check(fn, f, abs_floor=0.0):
        """error per row = max_i |ref_i - native_i| / (2^-23 * max_j |ref_j|): float32 epsilons relative to the vector norm."""
        worst = 0.0
        n = 0
        skipped = 0
        ex = None
        for a, o in by.get(fn, []):
            ref = f(a)
            if ref is None:
                skipped += 1
                continue
            scale = max(abs(x) for x in ref)
            if scale == 0:
                skipped += 1
                continue
            n += 1
            e = max(abs(rv - gv) for rv, gv in zip(ref, o)) / (2.0 ** -23 * scale)
            if e > worst:
                worst = e
                ex = (a, ref, o)
        res[fn] = (n, worst, skipped, ex)

    def q_canon(q, got):  # q and -q are the same rotation; align sign with the native result
        dot = sum(x * y for x, y in zip(q, got))
        return tuple(-c for c in q) if dot < 0 else q

    check("Quaternion.Euler", lambda a: euler(a[0], a[1], a[2]), 1e-7)
    check("Quaternion.AngleAxis", lambda a: axis_angle(a[0], a[1:4]), 1e-7)
    check("Quaternion.Inverse", lambda a: (-a[0], -a[1], -a[2], a[3]))  # native Inverse = conjugate (no 1/|q|^2)
    # eulerAngles: Euler(eulerAngles(q)) must reproduce q (up to sign)
    rt = []
    for a, o in by.get("Quaternion.Euler.eulerAngles", []):
        q2 = euler(o[0], o[1], o[2])
        q2 = q_canon(q2, a)
        rt.append(max(abs(x - y) for x, y in zip(q2, a)))
    check("Matrix4x4.TRS", lambda a: trs(a[0:3], a[3:7], a[7:10]), 1e-6)
    check("Matrix4x4.TRS.inverse", lambda a: inv4(trs(a[0:3], a[3:7], a[7:10])), 1e-6)

    def tp(a):
        p, q, s, v = a[0:3], a[3:7], a[7:10], a[10:13]
        r = rotate(q, (v[0] * s[0], v[1] * s[1], v[2] * s[2]))
        return [r[i] + p[i] for i in range(3)]

    def td(a):
        q, v = a[3:7], a[10:13]
        return list(rotate(q, v))

    def tv(a):
        q, s, v = a[3:7], a[7:10], a[10:13]
        return list(rotate(q, (v[0] * s[0], v[1] * s[1], v[2] * s[2])))

    def itd(a):
        q, v = a[3:7], a[10:13]
        return list(rotate((-q[0], -q[1], -q[2], q[3]), v))

    def itp(a):
        p, q, s, v = a[0:3], a[3:7], a[7:10], a[10:13]
        d0 = [v[i] - p[i] for i in range(3)]
        r = rotate((-q[0], -q[1], -q[2], q[3]), d0)
        return [r[i] / s[i] for i in range(3)]

    check("Transform.TransformPoint", tp, 1e-6)
    check("Transform.TransformDirection", td, 1e-7)
    check("Transform.TransformVector", tv, 1e-6)
    check("Transform.InverseTransformDirection", itd, 1e-7)
    check("Transform.InverseTransformPoint", itp, 1e-6)

    def lerp(a, unclamped):
        q1, q2, t = a[0:4], a[4:8], a[8]
        if not unclamped:
            t = min(max(t, 0.0), 1.0)
        dot = sum(x * y for x, y in zip(q1, q2))
        sg = -1.0 if dot < 0 else 1.0
        r = [q1[i] * (1 - t) + sg * q2[i] * t for i in range(4)]
        n = math.sqrt(sum(c * c for c in r))
        return [c / n for c in r] if n > 0 else None

    check("Quaternion.Lerp", lambda a: lerp(a, False), 1e-7)
    check("Quaternion.LerpUnclamped", lambda a: lerp(a, True), 1e-7)
    lines = ["# C3 native maths: convention check vs double-precision textbook formulas", "",
             "Error per row = max |native - reference| / (2^-23 * max |reference component|), i.e. float32 epsilons relative to the vector/quaternion/matrix magnitude. Reference = textbook formula in double on the float32 inputs. Inverse reference = conjugate (that is what Unity returns; it does not divide by |q|^2).",
             "", "| fn | rows | max error (eps rel. to magnitude) | skipped rows | worst example (inputs -> ref / native) |", "|---|---|---|---|---|"]
    for fn, (n, w, sk, ex) in res.items():
        if fn == "Quaternion.Euler.eulerAngles":
            continue
        e = ""
        if ex:
            e = "%s -> %s / %s" % ([round(x, 6) for x in ex[0]], [round(x, 7) for x in ex[1]], [round(x, 7) for x in ex[2]])
        lines.append("| %s | %d | %.2f | %d | %s |" % (fn, n, w, sk, e[:220]))
    if rt:
        lines.append("")
        lines.append("- Quaternion.eulerAngles round trip: max |Euler(q.eulerAngles) - q| = %.3g over %d quaternions (sign-aligned)." % (max(rt), len(rt)))
    text = "\n".join(lines) + "\n"
    if "--md" in argv:
        with open(argv[argv.index("--md") + 1], "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
    print(text)


if __name__ == "__main__":
    main(sys.argv[1:])
