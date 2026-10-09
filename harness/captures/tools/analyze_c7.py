"""Analyse C7: libm_dotnet.json (.NET Framework 4.8 x64) vs libm_mono.json (KSP's Mono).

  python -I harness/captures/tools/analyze_c7.py [--md report.md]

- f64/f32 libm tables: per function mismatches split into signed-zero-only, special (NaN/Inf) and finite; max ULP; and,
  with mpmath, which runtime is correctly rounded (error in ULPs vs the exact value).
- f32ops / f32ops_wide: each row classified against two evaluation models emulated exactly in Python:
    A = every float32 operation rounded to float32 (IEEE single, what .NET x64 RyuJIT does)
    B = float32 operands promoted to double, intermediates kept in double, rounding to float32 only at explicit (float)
        casts and at stores to float32 locals/array elements (variants listed per probe)
- conv / iconv / tostring / parse / sysrand: every difference listed.
"""
import json
import math
import os
import struct
import sys
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hcutil as H  # noqa: E402

try:
    import mpmath
    mpmath.mp.prec = 200
except ImportError:  # pragma: no cover
    mpmath = None

HERE = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))


def f32(x):
    """Round a double to float32 (round-half-even), returned as a Python float."""
    try:
        return struct.unpack("<f", struct.pack("<f", x))[0]
    except OverflowError:
        return math.copysign(math.inf, x)


def rows(doc, name):
    t = H.table(doc, name)
    cols = [c["name"] for c in t["columns"]]
    out = []
    for r in t["rows"]:
        d = dict(zip(cols, r))
        for c in t["columns"]:
            if c["type"] in ("f32", "f64") and (c["name"] + "_hex") in cols:
                h = d[c["name"] + "_hex"]
                d[c["name"]] = None if h is None else (H.bits_f32(int(h, 16)) if c["type"] == "f32" else H.bits_f64(int(h, 16)))
        out.append(d)
    return out


def bits_eq(a, b, f32_=False):
    if a is None or b is None:
        return a is None and b is None
    if isinstance(a, float) and isinstance(b, float):
        if math.isnan(a) or math.isnan(b):
            return math.isnan(a) and math.isnan(b)
        return (H.f32_bits(a) == H.f32_bits(b)) if f32_ else (H.f64_bits(a) == H.f64_bits(b))
    return a == b


EXACT = {}
if mpmath:
    mp = mpmath
    EXACT = {
        "Math.Sin": lambda x, y: mp.sin(x), "Math.Cos": lambda x, y: mp.cos(x), "Math.Tan": lambda x, y: mp.tan(x),
        "Math.Asin": lambda x, y: mp.asin(x), "Math.Acos": lambda x, y: mp.acos(x), "Math.Atan": lambda x, y: mp.atan(x),
        "Math.Atan2(x,0.7)": lambda x, y: mp.atan2(x, y), "Math.Atan2(0.7,x)": lambda x, y: mp.atan2(y, x),
        "Math.Sinh": lambda x, y: mp.sinh(x), "Math.Cosh": lambda x, y: mp.cosh(x), "Math.Tanh": lambda x, y: mp.tanh(x),
        "Math.Exp": lambda x, y: mp.exp(x), "Math.Log": lambda x, y: mp.log(x), "Math.Log10": lambda x, y: mp.log10(x),
        "Math.Sqrt": lambda x, y: mp.sqrt(x), "Math.Pow(x,0.4)": lambda x, y: mp.power(x, y), "Math.Pow(|x|,1/3)": lambda x, y: mp.power(x, y),
        "Math.Pow(x,2.0)": lambda x, y: mp.power(x, y), "Math.Pow(1.0000001,x)": lambda x, y: mp.power(x, y),
    }


def err_ulps(v, exact):
    """|v - exact| in ULPs of v's binade (double)."""
    if v == 0 or math.isinf(v) or math.isnan(v):
        return None
    e = math.frexp(v)[1]
    ulp = math.ldexp(1.0, e - 53)
    return float(abs(mpmath.mpf(v) - exact) / ulp)


def libm(dn, mo, table, lines, f32_=False):
    rd = {(r["fn"], r["i"]): r for r in rows(dn, table)}
    by = defaultdict(lambda: {"n": 0, "zero": 0, "special": 0, "finite": 0, "max_ulp": 0, "ex": [], "acc": Counter()})
    for r in rows(mo, table):
        g = rd[(r["fn"], r["i"])]
        st = by[r["fn"]]
        st["n"] += 1
        a, b = g["r"], r["r"]
        if bits_eq(a, b, f32_):
            continue
        if a == 0 and b == 0:
            st["zero"] += 1
            kind = "signed zero"
        elif math.isnan(a) or math.isnan(b) or math.isinf(a) or math.isinf(b):
            st["special"] += 1
            kind = "special"
        else:
            st["finite"] += 1
            u = H.ulp_dist32(a, b) if f32_ else H.ulp_dist64(a, b)
            st["max_ulp"] = max(st["max_ulp"], u)
            kind = "%d ulp" % u
            if not f32_ and mpmath and r["fn"] in EXACT:
                x = mpmath.mpf(r["x"])
                y = mpmath.mpf(r["y"]) if r.get("y") is not None else None
                ex = EXACT[r["fn"]](x, y)
                ea, eb = err_ulps(a, ex), err_ulps(b, ex)
                if ea is not None and eb is not None:
                    st["acc"]["dotnet better" if ea < eb else ("mono better" if eb < ea else "tie")] += 1
                    kind += " (err .NET %.3g ulp, Mono %.3g ulp)" % (ea, eb)
        if len(st["ex"]) < 3:
            st["ex"].append("x=%r: .NET %r, Mono %r [%s]" % (r["x"], a, b, kind))
    lines.append("")
    lines.append("### %s: %d functions, %d values" % (table, len(by), sum(s["n"] for s in by.values())))
    lines.append("")
    lines.append("| fn | n | differ | signed-zero | NaN/Inf | finite | max ULP (finite) | more accurate | first examples |")
    lines.append("|---|---|---|---|---|---|---|---|---|")
    tot = 0
    for fn, st in sorted(by.items()):
        d = st["zero"] + st["special"] + st["finite"]
        tot += d
        acc = ", ".join("%s %d" % kv for kv in st["acc"].items())
        lines.append("| %s | %d | %d | %d | %d | %d | %d | %s | %s |" % (fn, st["n"], d, st["zero"], st["special"], st["finite"], st["max_ulp"], acc, "<br>".join(st["ex"]) if d else ""))
    return tot


C11, C03, C07, C33, C0999 = f32(1.1), f32(0.3), f32(0.7), f32(3.3), f32(0.999)


def models(probe, a, b, c, d, e, x):
    """Return dict model-name -> expected float32/double result."""
    m = {}
    if probe == "poly":
        m["A per-op f32"] = f32(f32(f32(f32(f32(x * C11) + C03) * x) - C07) / C33)
        m["B double, round at store"] = f32(((x * C11 + C03) * x - C07) / C33)
    elif probe in ("dot2", "dot2_call"):
        m["A per-op f32"] = f32(f32(f32(a * b) + f32(c * d)) - e)
        m["B double, round at store"] = f32(a * b + c * d - e)
    elif probe == "cast_each":
        m["A per-op f32"] = f32(f32(f32(a * b) + f32(c * d)) - e)
        m["B double, round at store"] = m["A per-op f32"]
    elif probe == "acc_loop":
        acc_a = 0.0
        acc_b = 0.0
        acc_c = 0.0
        for _ in range(16):
            acc_a = f32(f32(acc_a * C0999) + f32(a * b))
            acc_b = f32(acc_b * C0999 + a * b)
            acc_c = acc_c * C0999 + a * b
        m["A per-op f32"] = acc_a
        m["B double, local rounded each iteration"] = acc_b
        m["B' double, local never rounded"] = f32(acc_c)
    elif probe == "locals":
        m["A per-op f32"] = f32(f32(f32(a * b) + f32(c * d)) - e)
        # Roslyn /optimize+ keeps t1 on the evaluation stack and stores only t2 (and r) in IL locals (see ilspycmd -il)
        m["B double, IL locals rounded on store: f32(a*b + f32(c*d) - e)"] = f32(a * b + f32(c * d) - e)
        m["B' double, nothing rounded: f32(a*b + c*d - e)"] = f32(a * b + c * d - e)
        m["B'' both products rounded: f32(f32(a*b) + f32(c*d) - e)"] = f32(f32(a * b) + f32(c * d) - e)
    elif probe == "sqrt_div":
        m["A per-op f32"] = f32(f32(math.sqrt(f32(f32(a * a) + f32(b * b)))) / c)
        m["B double, round at (float) and store"] = f32(f32(math.sqrt(a * a + b * b)) / c)
    elif probe == "compare":
        m["A per-op f32"] = 1.0 if f32(f32(a * b) + c) == f32(f32(a * b) + c) else 0.0
        p = f32(a * b)
        m["B double, local p rounded"] = 1.0 if (a * b + c) == (p + c) else 0.0
        m["B' double, p not rounded"] = 1.0
    elif probe in ("arg_noinline", "arg_inline"):
        m["A per-op f32"] = f32(f32(a * b) + c)
        m["B double, argument not rounded"] = f32(a * b + c)
    elif probe in ("ret_noinline", "ret_inline", "field", "struct_field", "static_field", "local"):
        m["A per-op f32"] = f32(a * b)
        m["B double, not rounded"] = a * b
    elif probe == "mul_wide":
        m["A per-op f32"] = f32(a * b)
        m["B double (exact product)"] = a * b
    elif probe == "muladd_wide":
        m["A per-op f32"] = f32(f32(a * b) + c)
        m["B double"] = a * b + c
    return m


def f32ops(dn, mo, lines):
    out = []
    for table in ("f32ops", "f32ops_wide"):
        rd = {(r["probe"], r["i"]): r for r in rows(dn, table)}
        stats = defaultdict(lambda: {"n": 0, "diff": 0, "dn": Counter(), "mo": Counter(), "max_ulp": 0})
        for r in rows(mo, table):
            g = rd[(r["probe"], r["i"])]
            st = stats[r["probe"]]
            st["n"] += 1
            wide = table == "f32ops_wide"
            if not bits_eq(g["r"], r["r"], not wide):
                st["diff"] += 1
                if not (math.isnan(g["r"]) or math.isnan(r["r"])) and r["probe"] != "compare":
                    st["max_ulp"] = max(st["max_ulp"], H.ulp_dist64(g["r"], r["r"]) if wide else H.ulp_dist32(g["r"], r["r"]))
            ms = models(r["probe"], r["a"], r["b"], r["c"], r.get("d"), r.get("e"), r.get("x"))
            for who, v in (("dn", g["r"]), ("mo", r["r"])):
                hit = [k for k, mv in ms.items() if bits_eq(mv, v, not wide)]
                st[who]["+".join(k.split(" ")[0] for k in hit) if hit else "none"] += 1
            out.append(None)
        lines.append("")
        lines.append("### %s (rows matching each model; A = per-op float32, B/B' = double intermediates, see models())" % table)
        lines.append("")
        lines.append("| probe | n | .NET != Mono | max ULP | .NET matches | Mono matches |")
        lines.append("|---|---|---|---|---|---|")
        for p, st in stats.items():
            lines.append("| %s | %d | %d | %d | %s | %s |" % (p, st["n"], st["diff"], st["max_ulp"], dict(st["dn"]), dict(st["mo"])))
    return out


def simple(dn, mo, table, keyf, lines, f32cols=()):
    rd = {keyf(r): r for r in rows(dn, table)}
    diffs = []
    for r in rows(mo, table):
        g = rd[keyf(r)]
        for k in r:
            if k.endswith("_hex"):
                continue
            if not bits_eq(g[k], r[k], k in f32cols):
                diffs.append((keyf(r), k, g[k], r[k], r.get("x", r.get("v", r.get("s")))))
    lines.append("")
    lines.append("### %s: %d differences" % (table, len(diffs)))
    if diffs:
        lines.append("")
        lines.append("| key | column | input | .NET | Mono |")
        lines.append("|---|---|---|---|---|")
        for k, col, a, b, inp in diffs:
            lines.append("| %s | %s | %r | %r | %r |" % (k, col, inp, a, b))
    return len(diffs)


def main(argv):
    dn = H.load(os.path.join(HERE, "out", "libm_dotnet.json"))
    mo = H.load(os.path.join(HERE, "out", "libm_mono.json"))
    lines = ["# C7 analysis: .NET Framework 4.8 x64 vs KSP Mono", "",
             "- .NET: %s" % dn["provenance"].get("runtime"), "- Mono: %s" % mo["provenance"].get("runtime"),
             "- inputs: libm %s values, f32 %s rows; System.Random regenerated in Mono: random7 mismatches %s, random11 mismatches %s" % (
                 mo["provenance"]["libm_inputs"], mo["provenance"]["f32_inputs"], mo["provenance"]["system_random7_regenerated_mismatches"],
                 mo["provenance"]["system_random11_regenerated_mismatches"])]
    libm(dn, mo, "f64", lines)
    libm(dn, mo, "f32", lines, f32_=True)
    f32ops(dn, mo, lines)
    simple(dn, mo, "conv", lambda r: (r["fn"], r["i"]), lines)
    simple(dn, mo, "iconv", lambda r: (r["fn"], r["i"]), lines)
    simple(dn, mo, "tostring", lambda r: (r["fn"], r["i"]), lines)
    simple(dn, mo, "parse", lambda r: (r["i"],), lines, f32cols=("f",))
    simple(dn, mo, "sysrand", lambda r: (r["seed"], r["k"]), lines)
    text = "\n".join(lines) + "\n"
    if "--md" in argv:
        p = argv[argv.index("--md") + 1]
        with open(p, "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
    print(text)


if __name__ == "__main__":
    main(sys.argv[1:])
