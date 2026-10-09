"""Strict bit-level comparison of two ksp-golden/1 files (golden vs capture), independent of decimal formatting.

  python -I harness/captures/tools/bitcompare.py GOLDEN CANDIDATE [--tables t1,t2] [--max-examples N] [--json report.json]

Rows are joined on the golden table's key columns. f32 columns: golden decimal -> correctly rounded float32 (exact
rational rounding, no double rounding), candidate from its <col>_hex column when present. f64 likewise. Reports per
column: n, bit mismatches (NaN payload/sign ignored), max ULP, max abs, and the first mismatches with keys.
Other columns must be equal. Exit 0 if no mismatches.
"""
import json
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hcutil as H  # noqa: E402


def val(row, cols, name, typ):
    """Python float (exact) for a float column, using <name>_hex when available."""
    hn = name + "_hex"
    if hn in cols:
        h = row[cols.index(hn)]
        if h is None:
            return None
        b = int(h, 16)
        return H.bits_f32(b) if typ == "f32" else H.bits_f64(b)
    v = row[cols.index(name)]
    if v is None:
        return None
    if typ == "f32":
        return H.exact_f32(v)
    return H.num64(v)


def same(a, b, typ):
    if a is None or b is None:
        return a is None and b is None
    if math.isnan(a) or math.isnan(b):
        return math.isnan(a) and math.isnan(b)
    if typ == "f32":
        return H.f32_bits(a) == H.f32_bits(b)
    return H.f64_bits(a) == H.f64_bits(b)


def compare(gpath, cpath, only=None, max_ex=5):
    g = H.load(gpath, exact=True)
    c = H.load(cpath, exact=True)
    ctabs = {t["name"]: t for t in c["tables"]}
    report = []
    total_bad = 0
    for gt in g["tables"]:
        if only and gt["name"] not in only:
            continue
        ct = ctabs.get(gt["name"])
        if ct is None:
            report.append({"table": gt["name"], "error": "missing in candidate"})
            total_bad += 1
            continue
        gcols = [x["name"] for x in gt["columns"]]
        ccols = [x["name"] for x in ct["columns"]]
        types = {x["name"]: x["type"] for x in gt["columns"]}
        key = gt["key"]
        cidx = {}
        for r in ct["rows"]:
            cidx[tuple(json.dumps(r[ccols.index(k)], default=str) for k in key)] = r
        stats = {}
        missing = 0
        for r in gt["rows"]:
            k = tuple(json.dumps(r[gcols.index(k2)], default=str) for k2 in key)
            cr = cidx.pop(k, None)
            if cr is None:
                missing += 1
                continue
            for name in gcols:
                if name in key or name.endswith("_hex"):
                    continue
                typ = types[name]
                st = stats.setdefault(name, {"type": typ, "n": 0, "mismatch": 0, "max_ulp": 0, "max_abs": 0.0, "examples": []})
                if name not in ccols:
                    st["error"] = "column missing in candidate"
                    continue
                st["n"] += 1
                if typ in ("f32", "f64"):
                    a = val(r, gcols, name, typ)
                    b = val(cr, ccols, name, typ)
                    if same(a, b, typ):
                        continue
                    st["mismatch"] += 1
                    if a is not None and b is not None and not (math.isnan(a) or math.isnan(b) or math.isinf(a) or math.isinf(b)):
                        u = H.ulp_dist32(a, b) if typ == "f32" else H.ulp_dist64(a, b)
                        st["max_ulp"] = max(st["max_ulp"], u)
                        st["max_abs"] = max(st["max_abs"], abs(a - b))
                    else:
                        u = "special"
                    if len(st["examples"]) < max_ex:
                        st["examples"].append({"key": [json.loads(x) for x in k], "golden": repr(a), "candidate": repr(b), "ulp": u})
                else:
                    gv, cv = r[gcols.index(name)], cr[ccols.index(name)]
                    if gv != cv:
                        st["mismatch"] += 1
                        if len(st["examples"]) < max_ex:
                            st["examples"].append({"key": [json.loads(x) for x in k], "golden": gv, "candidate": cv})
        for st in stats.values():
            total_bad += st["mismatch"] + (1 if "error" in st else 0)
        total_bad += missing
        report.append({"table": gt["name"], "rows": len(gt["rows"]), "missing": missing, "extra": len(cidx), "columns": stats})
    return total_bad, report


def main(argv):
    opts = {}
    args = []
    i = 0
    while i < len(argv):
        if argv[i] in ("--tables", "--max-examples", "--json"):
            opts[argv[i]] = argv[i + 1]
            i += 2
            continue
        args.append(argv[i])
        i += 1
    only = set(opts["--tables"].split(",")) if "--tables" in opts else None
    bad, rep = compare(args[0], args[1], only, int(opts.get("--max-examples", 5)))
    for t in rep:
        if "error" in t:
            print("%s: ERROR %s" % (t["table"], t["error"]))
            continue
        print("%s: rows=%d missing=%d extra=%d" % (t["table"], t["rows"], t["missing"], t["extra"]))
        for name, st in t["columns"].items():
            if "error" in st:
                print("   %-28s ERROR %s" % (name, st["error"]))
                continue
            flag = "BITEXACT" if st["mismatch"] == 0 else "DIFF    "
            extra = ""
            if st["type"] in ("f32", "f64") and st["mismatch"]:
                extra = " max_ulp=%s max_abs=%.3e" % (st["max_ulp"], st["max_abs"])
            print("   %s %-28s %s n=%d mismatches=%d%s" % (flag, name, st["type"], st["n"], st["mismatch"], extra))
            for e in st["examples"]:
                print("        %s" % json.dumps(e, default=str)[:300])
    if "--json" in opts:
        with open(opts["--json"], "w", encoding="utf-8") as f:
            json.dump({"golden": args[0], "candidate": args[1], "bit_mismatches": bad, "tables": rep}, f, indent=1, default=str)
    print("BIT-EXACT" if bad == 0 else "MISMATCHES: %d" % bad)
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
