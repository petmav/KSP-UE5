"""Rewrite the decimals of a capture file from its _hex columns using the harness's own number format.

  python -I harness/captures/tools/normalize.py <file.json> [...]

The in-game writer emits G9/G17 decimals; the harness emits the shortest round-trip form (ExactNumber.FormatFloat /
FormatDouble: first of G6..G9 / G15..G17 that parses back, 'E' -> 'e', '.0' appended to integers). compare.py joins
rows on the JSON text of key columns, so float keys (e.g. aero_forces mach) only match if both sides use the same
format. Values are taken from the hex bits (authoritative), so this never changes a number. Idempotent.
"""
import json
import math
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hcutil as H  # noqa: E402


def _norm(s):
    s = s.replace("E", "e")
    if "." not in s and "e" not in s:
        s += ".0"
    return s


LIBMREF = os.path.join(os.path.dirname(os.path.abspath(__file__)), "netfx", "bin", "LibmRef.exe")
NET = {}  # ("f"|"d", bits) -> list of .NET ToString("G<p>") candidates


def net_candidates(keys):
    """Ask the .NET Framework reference tool for ToString("G6".."G9") / ("G15".."G17") of every value (the harness
    formats with .NET Framework, whose float G<p> output is not always the correctly rounded decimal)."""
    keys = [k for k in keys if k not in NET]
    if not keys:
        return
    with tempfile.TemporaryDirectory() as td:
        fi, fo = os.path.join(td, "in.txt"), os.path.join(td, "out.txt")
        with open(fi, "w", encoding="ascii", newline="\n") as f:
            for t, b in keys:
                f.write("%s %s\n" % (t, ("%08X" % b) if t == "f" else ("%016X" % b)))
        subprocess.run([LIBMREF, "fmt", fi, fo], check=True)
        with open(fo, encoding="utf-8") as f:
            lines = f.read().split("\n")
    for k, line in zip(keys, lines):
        NET[k] = line.split("\t")


def fmt_f32(f):
    if math.isnan(f):
        return '"NaN"'
    if math.isinf(f):
        return '"Infinity"' if f > 0 else '"-Infinity"'
    if f == 0:
        return "-0.0" if math.copysign(1.0, f) < 0 else "0.0"
    b = H.f32_bits(f)
    for s in NET[("f", b)]:
        # harness ExactNumber.FormatFloat: (float)ParseDouble(s) == f, ParseDouble correctly rounded
        try:
            ok = H.f32_bits(struct_f32(float(s))) == b
        except OverflowError:
            ok = False
        if ok:
            return _norm(s)
    raise ValueError(f)


def struct_f32(d):
    import struct
    return struct.unpack("<f", struct.pack("<f", d))[0]


def fmt_f64(v):
    if math.isnan(v):
        return '"NaN"'
    if math.isinf(v):
        return '"Infinity"' if v > 0 else '"-Infinity"'
    if v == 0:
        return "-0.0" if math.copysign(1.0, v) < 0 else "0.0"
    for s in NET[("d", H.f64_bits(v))]:
        if H.f64_bits(float(s)) == H.f64_bits(v):
            return _norm(s)
    raise ValueError(v)


def q(s):
    return json.dumps(s, ensure_ascii=True)


def raw(v):
    if v is None:
        return "null"
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, (int,)):
        return str(v)
    if isinstance(v, H.Decimal):
        return str(v)
    if isinstance(v, float):
        return repr(v)
    if isinstance(v, str):
        return q(v)
    return json.dumps(v)


def normalize(path):
    with open(path, encoding="utf-8") as f:
        doc = json.load(f, parse_float=H.Decimal)
    need = set()
    for t in doc["tables"]:
        cols = [c["name"] for c in t["columns"]]
        for c in t["columns"]:
            if c["type"] in ("f32", "f64") and (c["name"] + "_hex") in cols:
                hi = cols.index(c["name"] + "_hex")
                for r in t["rows"]:
                    if r[hi] is not None:
                        b = int(r[hi], 16)
                        x = H.bits_f32(b) if c["type"] == "f32" else H.bits_f64(b)
                        if not (math.isnan(x) or math.isinf(x) or x == 0):
                            need.add(("f" if c["type"] == "f32" else "d", b))
    net_candidates(sorted(need))
    out = ['{', ' "schema": %s,' % q(doc["schema"]), ' "dataset": %s,' % q(doc["dataset"]), ' "trust": %s,' % q(doc["trust"]),
           ' "description": %s,' % q(doc["description"]), ' "oracles": %s,' % json.dumps(doc.get("oracles", []))]
    prov = doc.get("provenance", {})
    prov["decimal_format"] = "normalized from _hex by harness/captures/tools/normalize.py (harness ExactNumber.FormatFloat/FormatDouble on .NET Framework ToString G6..G9 / G15..G17)"
    out.append(' "provenance": {')
    items = list(prov.items())
    for i, (k, v) in enumerate(items):
        out.append('  %s: %s%s' % (q(k), json.dumps(v, default=str) if not isinstance(v, H.Decimal) else str(v), "," if i < len(items) - 1 else ""))
    out.append(' },')
    out.append(' "tables": [')
    n = 0
    for ti, t in enumerate(doc["tables"]):
        cols = [c["name"] for c in t["columns"]]
        types = [c["type"] for c in t["columns"]]
        hexi = [cols.index(c + "_hex") if (types[i] in ("f32", "f64") and (c + "_hex") in cols) else -1 for i, c in enumerate(cols)]
        out.append('  {')
        out.append('   "name": %s,' % q(t["name"]))
        out.append('   "description": %s,' % q(t["description"]))
        out.append('   "key": %s,' % json.dumps(t["key"]))
        out.append('   "columns": [')
        for ci, c in enumerate(t["columns"]):
            out.append('    %s%s' % (json.dumps(c), "," if ci < len(cols) - 1 else ""))
        out.append('   ],')
        out.append('   "row_count": %d,' % len(t["rows"]))
        out.append('   "rows": [')
        for ri, r in enumerate(t["rows"]):
            cells = []
            for i, v in enumerate(r):
                if hexi[i] >= 0 and r[hexi[i]] is not None:
                    b = int(r[hexi[i]], 16)
                    cells.append(fmt_f32(H.bits_f32(b)) if types[i] == "f32" else fmt_f64(H.bits_f64(b)))
                    n += 1
                else:
                    cells.append(raw(v))
            out.append('    [%s]%s' % (", ".join(cells), "," if ri < len(t["rows"]) - 1 else ""))
        out.append('   ]')
        out.append('  }%s' % ("," if ti < len(doc["tables"]) - 1 else ""))
    out.append(' ]')
    out.append('}')
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out) + "\n")
    print("%s: normalized %d float values" % (os.path.basename(path), n))


if __name__ == "__main__":
    for p in sys.argv[1:]:
        normalize(p)
