"""Compare a candidate output against KSP golden data (schema ksp-golden/1).

Usage (run with `python -I`):
  python -I compare.py GOLDEN CANDIDATE [--tol tolerances.json] [--report report.json] [--quiet]

GOLDEN / CANDIDATE are either two ksp-golden/1 JSON files or two directories (every *.json with the schema under
GOLDEN is matched by relative path under CANDIDATE). Rows are joined on each table's key columns.

Per numeric column the tool reports n, max |a-b|, max relative error |a-b|/max(|a|,1e-300), max ULP distance
(float32 ULPs for f32 columns, float64 ULPs for f64 columns) and the number of values outside tolerance, with the key
of the worst row. A value passes if ANY of its rule's limits is met (abs <= abs, rel <= rel, ulp <= ulp). NaN equals NaN,
+-Infinity must match exactly. Non-numeric columns (str/bool/i64) must match exactly.
A candidate row whose key is not in the golden ("extra row") is a failure, except in a table listed in ALLOWED_EXTRA_ROWS below,
where exactly the listed number of extra rows is accepted (more or fewer fails: the entry must be kept true). Extra tables and
columns in the candidate are ignored.
Exit code: 0 all within tolerance, 1 tolerance failures, missing rows/tables/columns or unexplained extra rows, 2 usage error.
"""
import fnmatch
import json
import math
import os
import struct
import sys

# Extra candidate rows that are accepted, per "<dataset>/<table>" of the golden file: the exact number and the reason. Every entry
# is rows the candidate writes on purpose and the gate removed from its (filtered) golden copy, see core/tests/CMakeLists.txt.
ALLOWED_EXTRA_ROWS = {
    # capture.part_database: the gate drops the 5 prefab parts whose modules are not ported (kerbalEVA, kerbalEVAfemale,
    # kerbalEVASlimSuit, kerbalEVASlimSuitFemale, flag) from the golden (KEEP_ROWS / DROP_ROWS filter), while the runner still
    # writes them; STATUS.md WP-05 / WP-06 and CLOUD-TASKS.md (f).
    "part_database/parts": {"count": 5, "reason": "the 5 dropped prefab parts"},
    "part_database/modules": {"count": 66, "reason": "modules of the 5 dropped prefab parts (16 + 16 + 16 + 16 + 2)"},
    # 1907 = 465 x 4 EVA kerbals + 29 flag fields of the 5 dropped parts (1889) + the 18 rows listed one by one, with reasons, in
    # core/tests/data/part_database_exclusions.json (DROP_KEYS_FILE)
    "part_database/module_fields": {"count": 1907, "reason": "fields of the 5 dropped prefab parts + the 18 individually excluded rows"},
    "part_database/assets": {"count": 5, "reason": "the asset rows of the 5 dropped prefab parts"},
    # capture.thermal_step: the 6 (case, frame) rows of core/tests/data/thermal_step_exclusions.json (DROP_KEYS_FILE), each with its
    # reason there (altitude not recoverable at the top of the atmosphere; first frame without a previous density)
    "thermal_step/vessel": {"count": 6, "reason": "the 6 rows excluded one by one in thermal_step_exclusions.json"},
    # capture.vessel_rails: the 2 body rows (id "body:Mun" / "body:Minmus", 450 frames each) dropped by DROP_ROWS "samples:id^body:",
    # gated by capture.orbit_rails (a body has no Vessel state)
    "orbit_rails_ingame/samples": {"count": 900, "reason": "the 2 body rows x 450 frames, gated by capture.orbit_rails"},
}

SPECIAL = {"NaN": math.nan, "Infinity": math.inf, "-Infinity": -math.inf}


def num(v):
    if v is None:
        return None
    if isinstance(v, str):
        return SPECIAL.get(v, None) if v in SPECIAL else float(v)
    return float(v)


def ulps(a, b, f32):
    if f32:
        ia = struct.unpack("<i", struct.pack("<f", a))[0]
        ib = struct.unpack("<i", struct.pack("<f", b))[0]
        lim = 0x80000000
    else:
        ia = struct.unpack("<q", struct.pack("<d", a))[0]
        ib = struct.unpack("<q", struct.pack("<d", b))[0]
        lim = 0x8000000000000000
    if ia < 0:
        ia = lim - ia
    if ib < 0:
        ib = lim - ib
    return abs(ia - ib)


def load_rules(path):
    if not path:
        return [{"match": "*", "abs": 0.0, "rel": 0.0, "ulp": 0}]
    with open(path, encoding="utf-8") as f:
        return json.load(f)["rules"]


def rule_for(rules, ident, ctype):
    for r in rules:
        if fnmatch.fnmatchcase(ident, r["match"]) and (("type" not in r) or r["type"] == ctype):
            return r
    return {"abs": 0.0, "rel": 0.0, "ulp": 0}


def compare_files(gpath, cpath, rules, out, label):
    with open(gpath, encoding="utf-8") as f:
        g = json.load(f)
    with open(cpath, encoding="utf-8") as f:
        c = json.load(f)
    ds = g.get("dataset", label)
    ok = True
    ctabs = {t["name"]: t for t in c.get("tables", [])}
    for gt in g["tables"]:
        tname = gt["name"]
        if tname not in ctabs:
            out.append({"dataset": ds, "table": tname, "error": "table missing in candidate"})
            ok = False
            continue
        ct = ctabs[tname]
        gcols = [col["name"] for col in gt["columns"]]
        ctypes = {col["name"]: col["type"] for col in gt["columns"]}
        ccols = [col["name"] for col in ct["columns"]]
        key = gt["key"]
        kidx_g = [gcols.index(k) for k in key]
        if any(k not in ccols for k in key):
            out.append({"dataset": ds, "table": tname, "error": "key column missing in candidate"})
            ok = False
            continue
        kidx_c = [ccols.index(k) for k in key]
        crows = {}
        for r in ct["rows"]:
            crows[tuple(json.dumps(r[i]) for i in kidx_c)] = r
        missing = 0
        stats = {}
        for r in gt["rows"]:
            k = tuple(json.dumps(r[i]) for i in kidx_g)
            cr = crows.pop(k, None)
            if cr is None:
                missing += 1
                continue
            for ci, cname in enumerate(gcols):
                if cname in key:
                    continue
                if cname not in ccols:
                    stats.setdefault(cname, {"error": "column missing in candidate"})
                    continue
                gv, cv = r[ci], cr[ccols.index(cname)]
                ctype = ctypes[cname]
                st = stats.setdefault(cname, {"type": ctype, "n": 0, "max_abs": 0.0, "max_rel": 0.0, "max_ulp": 0, "fail": 0, "worst": None, "worst_score": -1.0})
                if "error" in st:
                    continue
                st["n"] += 1
                if ctype in ("f64", "f32"):
                    a, b = num(gv), num(cv) if not isinstance(cv, bool) else None
                    if a is None and b is None:
                        continue
                    if a is None or b is None:
                        st["fail"] += 1
                        st["worst"] = [list(k), gv, cv]
                        continue
                    if math.isnan(a) or math.isnan(b) or math.isinf(a) or math.isinf(b):
                        same = (math.isnan(a) and math.isnan(b)) or a == b
                        if not same:
                            st["fail"] += 1
                            st["worst"] = [list(k), gv, cv]
                        continue
                    d = abs(a - b)
                    rel = d / max(abs(a), 1e-300)
                    u = ulps(a, b, ctype == "f32")
                    st["max_abs"] = max(st["max_abs"], d)
                    st["max_rel"] = max(st["max_rel"], rel)
                    st["max_ulp"] = max(st["max_ulp"], u)
                    rule = rule_for(rules, f"{ds}/{tname}/{cname}", ctype)
                    passed = d <= rule.get("abs", 0.0) or rel <= rule.get("rel", 0.0) or u <= rule.get("ulp", 0)
                    if not passed:
                        st["fail"] += 1
                    score = rel if d > 0 else 0.0
                    if score > st["worst_score"] and d > 0:
                        st["worst_score"] = score
                        st["worst"] = [list(k), gv, cv]
                else:
                    if gv != cv:
                        st["fail"] += 1
                        if st["worst"] is None:
                            st["worst"] = [list(k), gv, cv]
        extra = len(crows)
        res = {"dataset": ds, "table": tname, "rows": len(gt["rows"]), "missing_rows": missing, "extra_rows": extra, "columns": {}}
        allowed = ALLOWED_EXTRA_ROWS.get(f"{ds}/{tname}")
        if extra and (allowed is None or allowed["count"] != extra):
            res["extra_rows_error"] = ("not in the allow-list" if allowed is None else f"allow-list expects exactly {allowed['count']}")
            res["extra_keys_sample"] = [[json.loads(s) for s in k] for k in list(crows)[:5]]
            ok = False
        elif extra:
            res["extra_rows_allowed"] = allowed["reason"]
        for cname, st in stats.items():
            st.pop("worst_score", None)
            res["columns"][cname] = st
            if st.get("error") or st.get("fail"):
                ok = False
        if missing:
            ok = False
        out.append(res)
    return ok


def find_pairs(g, c):
    if os.path.isfile(g):
        return [(g, c, os.path.basename(g))]
    pairs = []
    for root, _, files in os.walk(g):
        for fn in sorted(files):
            if not fn.endswith(".json"):
                continue
            gp = os.path.join(root, fn)
            try:
                with open(gp, encoding="utf-8") as f:
                    head = f.read(64)
            except OSError:
                continue
            if '"ksp-golden/1"' not in head:
                continue
            rel = os.path.relpath(gp, g)
            pairs.append((gp, os.path.join(c, rel), rel))
    return pairs


def main(argv):
    args = [a for a in argv if not a.startswith("--")]
    opts = {}
    i = 0
    while i < len(argv):
        if argv[i] in ("--tol", "--report"):
            opts[argv[i]] = argv[i + 1]
            i += 2
            continue
        if argv[i] == "--quiet":
            opts["--quiet"] = True
        i += 1
    args = [a for a in args if a not in opts.values()]
    if len(args) != 2:
        print(__doc__)
        return 2
    rules = load_rules(opts.get("--tol"))
    out = []
    ok = True
    for gp, cp, label in find_pairs(args[0], args[1]):
        if not os.path.exists(cp):
            out.append({"file": label, "error": "candidate file missing"})
            ok = False
            continue
        ok = compare_files(gp, cp, rules, out, label) and ok
    if not opts.get("--quiet"):
        for r in out:
            if "error" in r:
                print(f"ERROR {r.get('dataset', r.get('file'))}/{r.get('table', '')}: {r['error']}")
                continue
            print(f"{r['dataset']}/{r['table']}: rows={r['rows']} missing={r['missing_rows']} extra={r['extra_rows']}")
            if "extra_rows_error" in r:
                print(f"   FAIL extra rows ({r['extra_rows_error']}); first keys (json) {r['extra_keys_sample']}")
            elif "extra_rows_allowed" in r:
                print(f"   ok   extra rows allowed: {r['extra_rows_allowed']}")
            for cname, st in r["columns"].items():
                if "error" in st:
                    print(f"   {cname:34s} ERROR {st['error']}")
                elif st["type"] in ("f64", "f32"):
                    flag = "FAIL" if st["fail"] else "ok  "
                    print(f"   {flag} {cname:34s} {st['type']} n={st['n']:6d} max_abs={st['max_abs']:.3e} max_rel={st['max_rel']:.3e} max_ulp={st['max_ulp']} fails={st['fail']}"
                          + (f" worst={st['worst']}" if st["fail"] else ""))
                elif st["fail"]:
                    print(f"   FAIL {cname:34s} {st['type']} mismatches={st['fail']} first={st['worst']}")
    if "--report" in opts:
        with open(opts["--report"], "w", encoding="utf-8") as f:
            json.dump({"ok": ok, "results": out}, f, indent=1)
    print("RESULT:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
