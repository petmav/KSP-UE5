"""Fit/verify a model of UnityEngine.Random (Unity 2019.4 native xorshift128) against unity_random.json.

  python -I harness/captures/tools/random_model.py [--md report.md]

Model (verified bit-exactly against the capture; see the report for which tables each part covers):
  InitState(seed): s0 = seed; s1 = s0*1812433253+1; s2 = s1*1812433253+1; s3 = s2*1812433253+1   (uint32 wrap)
  next():  t = s0 ^ (s0 << 11); t ^= t >> 8; (s0,s1,s2) = (s1,s2,s3); s3 = s3 ^ (s3 >> 19) ^ t; return s3
  value = (float)(r & 0x7FFFFF) / 8388607f       (identical to * (1f/8388607f) for every 23-bit input)
  Range(float min, float max) = v*min + (1-v)*max  (float32 ops; NOTE reversed vs a textbook lerp: Range(0f,1f) == 1 - value)
  Range(int min, int max) = min + (int)(r % (uint)(max - min)) for max > min; max == min returns min without a draw
"""
import math
import os
import struct
import sys
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hcutil as H  # noqa: E402

HERE = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
M32 = 0xFFFFFFFF


def f32(x):
    try:
        return struct.unpack("<f", struct.pack("<f", x))[0]
    except OverflowError:
        return math.copysign(math.inf, x)


class XS:
    def __init__(self, seed):
        s0 = seed & M32
        s1 = (s0 * 1812433253 + 1) & M32
        s2 = (s1 * 1812433253 + 1) & M32
        s3 = (s2 * 1812433253 + 1) & M32
        self.s = [s0, s1, s2, s3]

    def state(self):
        return [x - (1 << 32) if x & 0x80000000 else x for x in self.s]

    def next(self):
        s0, s1, s2, s3 = self.s
        t = (s0 ^ (s0 << 11)) & M32
        t ^= t >> 8
        s3n = (s3 ^ (s3 >> 19) ^ t) & M32
        self.s = [s1, s2, s3, s3n]
        return s3n


# candidate uint32 -> float mappings for Random.value (all evaluated in float32 like the native code)
VALUE = {
    "(r & 0x7FFFFF) / 8388607f": lambda r: f32(f32(r & 0x7FFFFF) / f32(8388607.0)),
    "(r & 0x7FFFFF) * (1f/8388607f)": lambda r: f32(f32(r & 0x7FFFFF) * f32(1.0 / 8388607.0)),
    "(r & 0x7FFFFF) / 8388608f": lambda r: f32((r & 0x7FFFFF) / 8388608.0),
    "r / 4294967295f": lambda r: f32(f32(r) / f32(4294967295.0)),
    "(r >> 9) / 8388607f": lambda r: f32(f32(r >> 9) / f32(8388607.0)),
    "(r >> 8) / 16777215f": lambda r: f32(f32(r >> 8) / f32(16777215.0)),
    "(r >> 8) / 16777216f": lambda r: f32((r >> 8) / 16777216.0),
    "1 - (r & 0x7FFFFF)/8388607f": lambda r: f32(1.0 - f32(f32(r & 0x7FFFFF) / f32(8388607.0))),
}

RANGEF = {
    "v*min + (1-v)*max": lambda v, a, b: f32(f32(v * a) + f32(f32(1.0 - v) * b)),
    "min + (max-min)*v": lambda v, a, b: f32(a + f32(f32(b - a) * v)),
    "v*max + (1-v)*min": lambda v, a, b: f32(f32(v * b) + f32(f32(1.0 - v) * a)),
    "(1-v)*min + v*max": lambda v, a, b: f32(f32(f32(1.0 - v) * a) + f32(v * b)),
    "min*(1-v) + max*v (double)": lambda v, a, b: f32(a * (1.0 - v) + b * v),
    "lerp a+(b-a)*v (double)": lambda v, a, b: f32(a + (b - a) * v),
}


def rangei_candidates(r, a, b):
    out = {}
    if b > a:
        out["min + r % (max-min)"] = a + (r % ((b - a) & M32))
    elif b < a:
        out["min - r % (min-max)"] = a - (r % ((a - b) & M32))
    else:
        out["min"] = a
    out["min + r % (uint)(max-min) (wrap)"] = (a + (r % (((b - a) & M32) or 1))) if b != a else a
    return {k: ((v + (1 << 31)) & M32) - (1 << 31) for k, v in out.items()}


def main(argv):
    d = H.load(os.path.join(HERE, "out", "unity_random.json"))
    T = {t["name"]: H.rows_as_dicts(t) for t in d["tables"]}
    lines = ["# C2 analysis: UnityEngine.Random model vs capture", ""]
    # ---- InitState
    seeds = [r["seed"] for r in T["seeds"]]
    st = {(r["seed"], r["block"]): [r["s0"], r["s1"], r["s2"], r["s3"]] for r in T["states"]}
    ok_init = sum(1 for s in seeds if XS(s).state() == st[(s, "init")])
    lines.append("- InitState: %d/%d seeds match `s0=seed; s(i+1)=s(i)*1812433253+1`" % (ok_init, len(seeds)))
    # ---- step + value via trace
    ok_step = 0
    n_step = 0
    vhits = Counter()
    for r in T["trace"]:
        x = XS(0)
        x.s = [v & M32 for v in (r["s0"], r["s1"], r["s2"], r["s3"])]
        rr = x.next()
        for name, f in VALUE.items():
            if H.f32_bits(f(rr)) == H.f32_bits(r["value"]):
                vhits[name] += 1
        n_step += 1
    # state chaining within the trace
    by = defaultdict(list)
    for r in T["trace"]:
        by[r["seed"]].append(r)
    for s, rs in by.items():
        x = XS(s)
        for r in sorted(rs, key=lambda q: q["k"]):
            if x.state() == [r["s0"], r["s1"], r["s2"], r["s3"]]:
                ok_step += 1
            x.next()
    lines.append("- xorshift128 step: %d/%d traced states reproduced by the step function" % (ok_step, n_step))
    lines.append("- Random.value mapping hits on %d traced calls: %s" % (n_step, dict(vhits)))
    vname = max(vhits, key=vhits.get) if vhits else None
    vf = VALUE[vname]
    # ---- value table + states after 64 values
    okv = 0
    n = 0
    oks = 0
    for s in seeds:
        x = XS(s)
        rows = sorted([r for r in T["value"] if r["seed"] == s], key=lambda q: q["k"])
        for r in rows:
            n += 1
            okv += H.f32_bits(vf(x.next())) == H.f32_bits(r["value"])
        oks += x.state() == st[(s, "after_value")]
    lines.append("- value table: %d/%d values match `%s`; state after 64 values: %d/%d seeds" % (okv, n, vname, oks, len(seeds)))
    # ---- Range(float)
    fh = Counter()
    n = 0
    oks = 0
    for s in seeds:
        x = XS(s)
        for r in sorted([r for r in T["range_f"] if r["seed"] == s], key=lambda q: q["k"]):
            v = vf(x.next())
            n += 1
            for name, f in RANGEF.items():
                if H.f32_bits(f(v, -10.0, 10.0)) == H.f32_bits(r["value"]):
                    fh[name] += 1
        oks += x.state() == st[(s, "after_range_f")]
    lines.append("- Range(-10f,10f): %d calls; formula hits %s; one draw per call: state after 64 calls %d/%d" % (n, dict(fh), oks, len(seeds)))
    # ---- Range(int)
    ih = Counter()
    n = 0
    oks = 0
    for s in seeds:
        x = XS(s)
        for call, (a, b) in (("0_7", (0, 7)), ("-1000_1000", (-1000, 1000))):
            for r in sorted([r for r in T["range_i"] if r["seed"] == s and r["call"] == call], key=lambda q: q["k"]):
                rr = x.next()
                n += 1
                for name, v in rangei_candidates(rr, a, b).items():
                    if v == r["value"]:
                        ih[name] += 1
        oks += x.state() == st[(s, "after_range_i")]
    lines.append("- Range(0,7) + Range(-1000,1000): %d calls; formula hits %s; state after 128 calls %d/%d" % (n, dict(ih), oks, len(seeds)))
    ti = Counter()
    for r in T["trace_int"]:
        x = XS(0)
        x.s = [v & M32 for v in (r["s0"], r["s1"], r["s2"], r["s3"])]
        rr = x.next()
        for name, v in rangei_candidates(rr, -2147483648, 2147483647).items():
            if v == r["value"]:
                ti[name] += 1
    lines.append("- Range(int.MinValue,int.MaxValue) traced: %d calls; hits %s" % (len(T["trace_int"]), dict(ti)))
    # ---- edge table: replay with the model where possible
    edge_ok = 0
    edge_n = 0
    edge_bad = []
    for s in seeds:
        x = XS(s)
        rows = sorted([r for r in T["edge"] if r["seed"] == s], key=lambda q: q["k"])
        for r in rows:
            c = r["call"]
            exp = None
            if c.startswith("Range(") and "f" not in c and "int." not in c:
                a, b = [int(t) for t in c[6:-1].split(",")]
                if a == b:
                    exp = ("i", a)  # no draw?
                else:
                    rr = x.next()
                    cand = rangei_candidates(rr, a, b)
                    exp = ("i", list(cand.values())[0])
            elif c == "Range(int.MinValue,int.MaxValue)":
                exp = ("i", rangei_candidates(x.next(), -2147483648, 2147483647)["min + r % (max-min)"])
            elif c == "Range(0,int.MaxValue)":
                exp = ("i", rangei_candidates(x.next(), 0, 2147483647)["min + r % (max-min)"])
            elif c.startswith("Range(") and c.endswith("f)"):
                a, b = [float(t.rstrip("f")) for t in c[6:-1].split(",")]
                a, b = f32(a), f32(b)
                exp = ("f", RANGEF["v*min + (1-v)*max"](vf(x.next()), a, b))
            elif c == "value":
                exp = ("f", vf(x.next()))
            else:
                break  # rotationUniform / ColorHSV: draw counts unknown -> stop replay for this seed
            edge_n += 1
            got = r["ri"] if exp[0] == "i" else r["f0"]
            good = (got == exp[1]) if exp[0] == "i" else H.f32_bits(got) == H.f32_bits(exp[1])
            edge_ok += good
            if not good and len(edge_bad) < 6:
                edge_bad.append((s, r["k"], c, got, exp[1]))
    lines.append("- edge calls replayed up to the first rotationUniform: %d/%d match; mismatches: %s" % (edge_ok, edge_n, edge_bad))
    # ---- draws per vector call (from the after_vec state)
    lines.append("")
    lines.append("Draw counts: after_vec = 16 x insideUnitSphere + 16 x onUnitSphere + 16 x rotation + 16 x insideUnitCircle.")
    for s in seeds[:3]:
        x = XS(s)
        for k in range(2000):
            if x.state() == st[(s, "after_vec")]:
                lines.append("- seed %d: after_vec reached after %d draws" % (s, k))
                break
            x.next()
        else:
            lines.append("- seed %d: after_vec not reached within 2000 draws (rejection sampling or non-xorshift path)" % s)
        x = XS(s)
        for k in range(4000):
            if x.state() == st[(s, "after_edge")]:
                lines.append("- seed %d: after_edge reached after %d draws" % (s, k))
                break
            x.next()
    # ---- KSP consumers: class radius = base * Range(0.75f,1.25f) after InitState(seed)
    base = {"A": 3.24, "B": 5.4, "C": 9.0, "D": 15.0, "E": 25.0, "F": 41.7, "G": 69.5, "H": 115.8, "I": 193.0}
    okc = 0
    nc = 0
    for r in T["ksp_class_radius"]:
        if r["class"] not in base:
            continue
        x = XS(r["seed"])
        v = RANGEF["v*min + (1-v)*max"](vf(x.next()), 0.75, 1.25)
        nc += 1
        okc += H.f32_bits(f32(f32(base[r["class"]]) * v)) == H.f32_bits(r["radius"])
    lines.append("- DiscoveryInfo.GetClassRadius: %d/%d radii reproduced by base * Range(0.75f,1.25f) with the model" % (okc, nc))
    lines.append("")
    lines.append("ksp_asteroid rows: " + "; ".join("seed %s: %s, class %s, %.6g days, ref %s, sma %r, err %s" % (
        r["seed"], r["name"], r["size_class"], r["duration_days"] or float("nan"), r["ref_body"], r["sma"], r["err"]) for r in T["ksp_asteroid"]))
    text = "\n".join(lines) + "\n"
    if "--md" in argv:
        with open(argv[argv.index("--md") + 1], "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
    print(text)


if __name__ == "__main__":
    main(sys.argv[1:])
