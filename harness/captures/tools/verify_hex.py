"""Check every f32/f64 decimal in capture files against its <col>_hex column.

  python -I harness/captures/tools/verify_hex.py <file.json> [...] [--fix]

The hex columns are written from the raw IEEE bits and are authoritative. A decimal that does not parse back to its
bits means the runtime's ToString("G9"/"G17") is wrong; --fix rewrites such decimals from the hex (shortest repr) and
records the count in provenance.decimal_fixups.
"""
import json
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hcutil as H  # noqa: E402


def dec_bits(v, f32):
    if isinstance(v, str):
        x = H.SPECIAL[v]
        if math.isnan(x):
            return "NaN"
        return H.hex32(x) if f32 else H.hex64(x)
    if f32:
        return H.hex32(H.exact_f32(v))
    return H.hex64(H.num64(v))


def short(x, f32):
    if math.isnan(x):
        return "NaN"
    if math.isinf(x):
        return "Infinity" if x > 0 else "-Infinity"
    if f32:
        for p in range(6, 10):
            s = "%.*g" % (p, x)
            if H.f32_bits(H.exact_f32(s)) == H.f32_bits(x):
                return json.loads(s if ("." in s or "e" in s or "inf" in s) else s + ".0")
    return x


def check(path, fix):
    with open(path, encoding="utf-8") as f:
        doc = json.load(f, parse_float=H.Decimal)
    bad = 0
    n = 0
    examples = []
    for t in doc["tables"]:
        cols = [c["name"] for c in t["columns"]]
        types = {c["name"]: c["type"] for c in t["columns"]}
        pairs = [(cols.index(c), cols.index(c + "_hex"), types[c] == "f32") for c in cols if types[c] in ("f32", "f64") and (c + "_hex") in cols]
        for r in t["rows"]:
            for vi, hi, f32 in pairs:
                v, h = r[vi], r[hi]
                if v is None and h is None:
                    continue
                n += 1
                if v is None or h is None:
                    bad += 1
                    continue
                hb = int(h, 16)
                hv = H.bits_f32(hb) if f32 else H.bits_f64(hb)
                if math.isnan(hv):
                    ok = v == "NaN"
                else:
                    ok = dec_bits(v, f32) == h.upper()
                if not ok:
                    bad += 1
                    if len(examples) < 5:
                        examples.append((t["name"], cols[vi], str(v), h))
                    if fix:
                        if math.isnan(hv):
                            r[vi] = "NaN"
                        elif math.isinf(hv):
                            r[vi] = "Infinity" if hv > 0 else "-Infinity"
                        else:
                            r[vi] = short(hv, True) if f32 else hv
    print("%s: %d float values, %d decimal/hex mismatches %s" % (os.path.basename(path), n, bad, examples if examples else ""))
    if fix and bad:
        doc["provenance"]["decimal_fixups"] = bad

        def default(o):
            if isinstance(o, H.Decimal):
                return float(o)
            raise TypeError
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            json.dump(doc, f, indent=None, default=default)
        print("  fixed and rewrote", path)
    return bad


def main(argv):
    fix = "--fix" in argv
    files = [a for a in argv if not a.startswith("--")]
    tot = sum(check(p, fix) for p in files)
    return 0 if tot == 0 or fix else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
