"""Shared helpers for the capture tools: exact decimal->float32/float64 conversion, hex bits, ksp-golden/1 loading."""
import json
import math
import struct
from decimal import Decimal
from fractions import Fraction

SPECIAL = {"NaN": math.nan, "Infinity": math.inf, "-Infinity": -math.inf}


def f32_bits(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


def f64_bits(x):
    return struct.unpack("<Q", struct.pack("<d", x))[0]


def bits_f32(b):
    return struct.unpack("<f", struct.pack("<I", b & 0xFFFFFFFF))[0]


def bits_f64(b):
    return struct.unpack("<d", struct.pack("<Q", b & 0xFFFFFFFFFFFFFFFF))[0]


def hex32(x):
    return "%08X" % f32_bits(x)


def hex64(x):
    return "%016X" % f64_bits(x)


def _next_f32(b, up):
    """Neighbouring float32 bit pattern (finite, positive or negative)."""
    x = bits_f32(b)
    if x == 0:
        return 1 if up else 0x80000001
    if (x > 0) == up:
        return b + 1
    return b - 1


def exact_f32(v):
    """Correctly rounded float32 (as a Python float) of a JSON value. Pass values loaded with load(..., exact=True)
    (decimal.Decimal) or decimal strings to avoid double rounding (decimal -> double -> float)."""
    if v is None:
        return None
    if isinstance(v, str) and v in SPECIAL:
        return SPECIAL[v]
    if isinstance(v, float):
        if math.isnan(v) or math.isinf(v):
            return v
        q = Fraction(v)
        neg = math.copysign(1.0, v) < 0
    else:
        q = Fraction(v) if not isinstance(v, str) else Fraction(v.strip())
        neg = str(v).strip().startswith("-")
    if q == 0:
        return -0.0 if neg else 0.0
    d = float(q)  # correctly rounded double
    try:
        c = struct.unpack("<f", struct.pack("<f", d))[0]
    except OverflowError:
        return -math.inf if neg else math.inf
    if math.isinf(c):
        return c
    best = None
    cb = f32_bits(c)
    for b in (cb, _next_f32(cb, True), _next_f32(cb, False)):
        f = bits_f32(b)
        if math.isinf(f) or math.isnan(f):
            continue
        err = abs(Fraction(f) - q)
        key = (err, b & 1)  # ties to even mantissa
        if best is None or key < best[0]:
            best = (key, f)
    return best[1]


def num64(v):
    if v is None:
        return None
    if isinstance(v, str):
        return SPECIAL[v] if v in SPECIAL else float(v)
    return float(v)  # float(Decimal) is correctly rounded


def load(path, exact=False):
    """exact=True keeps JSON decimals as decimal.Decimal (exact) instead of Python floats."""
    with open(path, encoding="utf-8") as f:
        return json.load(f, parse_float=Decimal) if exact else json.load(f)


def table(doc, name):
    for t in doc["tables"]:
        if t["name"] == name:
            return t
    raise KeyError(name)


def rows_as_dicts(t):
    cols = [c["name"] for c in t["columns"]]
    return [dict(zip(cols, r)) for r in t["rows"]]


def esc(s):
    return s.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n").replace("\r", "\\r")


def ulp_dist32(a, b):
    ia, ib = f32_bits(a), f32_bits(b)
    ia = ia if ia < 0x80000000 else 0x80000000 - ia
    ib = ib if ib < 0x80000000 else 0x80000000 - ib
    return abs(ia - ib)


def ulp_dist64(a, b):
    ia, ib = f64_bits(a), f64_bits(b)
    ia = ia if ia < 0x8000000000000000 else 0x8000000000000000 - ia
    ib = ib if ib < 0x8000000000000000 else 0x8000000000000000 - ib
    return abs(ia - ib)
