"""Write a filtered copy of a ksp-golden/1 file for compare.py (run with `python -I`).

    python -I filter_golden.py SRC DST [--tables t1,t2,...] [--keep-columns t:c1,c2/t2:c3,...] [--drop-hex]
                               [--drop-rows t:col=v1|v2+col2=v3/t2:...] [--keep-rows-file FILE]
                               [--renumber t:col:groupcol/...] [--drop-keys-file FILE] [--drop-columns t:c1,c2/...]

--tables        keep only these tables (default: all)
--keep-columns  per table, keep only the listed columns (key columns must be listed); tables not named keep all
                columns. Used when some columns need code another package provides (documented in tests/CMakeLists.txt
                and STATUS.md); never to hide a mismatch.
--drop-rows  drop the rows of a table whose column text equals one of the listed values (OR over the listed
             columns; a null cell reads "null"). For rows whose subject is outside a package's scope by a property
             of the golden row itself (e.g. assets source=unity_asset), documented in tests/CMakeLists.txt + STATUS.md.
--keep-rows-file  JSON {table: [[key values...], ...]}: keep only the rows whose key values are listed (a scope list
             the runner derives from its INPUTS, e.g. the module fields a part cfg sets; never from the comparison).
--drop-columns  per table, drop the listed columns (the complement of --keep-columns; same rule: only columns that
             need another package's code, documented).
--drop-keys-file  JSON {table: [{"key": [key values...], "reason": "..."}, ...]}: drop individually listed rows, each
             with its documented reason (module behaviour another package ports, ...).
--renumber  after the row filters, rewrite an index key column as 0, 1, 2... within each value of a group column (row
             order kept), for tables whose index counted rows that were dropped.
--drop-hex  drop the `<col>_hex` companion columns of the in-game capture files (harness/captures/out/*.json). The
            decimals in those files are rewritten from the hex bits in the harness's exact number format, so comparing
            the decimals with compare.py's ULP metric is equivalent; the candidate does not need to emit hex columns.

The source file is never modified (harness/ is read-only); the copy goes into the build tree.
"""
import json
import sys


def main(argv):
    args = [a for a in argv if not a.startswith("--")]
    tables = None
    keep_cols = {}
    drop_hex = "--drop-hex" in argv
    drop_rows = {}
    keep_rows_file = None
    renumber = []
    drop_keys_file = None
    drop_cols = {}
    for i, a in enumerate(argv):
        if a == "--tables":
            tables = [t for t in argv[i + 1].split(",") if t]
            args.remove(argv[i + 1])
        if a == "--drop-rows":
            for spec in argv[i + 1].split("/"):
                t, conds = spec.split(":", 1)
                for cond in conds.split("+"):
                    c, vals = cond.split("=", 1)
                    drop_rows.setdefault(t, []).append((c, set(vals.split("|"))))
            args.remove(argv[i + 1])
        if a == "--keep-rows-file":
            keep_rows_file = argv[i + 1]
            args.remove(argv[i + 1])
        if a == "--drop-columns":
            for spec in argv[i + 1].split("/"):
                t, cols = spec.split(":", 1)
                drop_cols[t] = [c for c in cols.split(",") if c]
            args.remove(argv[i + 1])
        if a == "--drop-keys-file":
            drop_keys_file = argv[i + 1]
            args.remove(argv[i + 1])
        if a == "--renumber":
            for spec in argv[i + 1].split("/"):
                t, col, grp = spec.split(":")
                renumber.append((t, col, grp))
            args.remove(argv[i + 1])
        if a == "--keep-columns":
            for spec in argv[i + 1].split("/"):
                t, cols = spec.split(":", 1)
                keep_cols[t] = [c for c in cols.split(",") if c]
            args.remove(argv[i + 1])
    if len(args) != 2:
        print(__doc__)
        return 2
    src, dst = args
    with open(src, encoding="utf-8") as f:
        g = json.load(f)
    keep_keys = {}
    if keep_rows_file:
        with open(keep_rows_file, encoding="utf-8") as f:
            for tname, keys in json.load(f).items():
                keep_keys[tname] = {tuple(json.dumps(v) for v in k) for k in keys}

    drop_keys = {}
    if drop_keys_file:
        with open(drop_keys_file, encoding="utf-8") as f:
            for tname, entries in json.load(f).items():
                if tname.startswith("_"):
                    continue
                drop_keys[tname] = {tuple(json.dumps(v) for v in e["key"]) for e in entries}

    def cell_text(v):
        return "null" if v is None else (v if isinstance(v, str) else json.dumps(v))

    out_tables = []
    for t in g["tables"]:
        cols0 = [c["name"] for c in t["columns"]]
        rows = t["rows"]
        if t["name"] in drop_rows:
            conds = [(cols0.index(c), vals) for c, vals in drop_rows[t["name"]]]
            rows = [r for r in rows if not any(cell_text(r[ci]) in vals for ci, vals in conds)]
        if t["name"] in keep_keys:
            kidx = [cols0.index(k) for k in t["key"]]
            rows = [r for r in rows if tuple(json.dumps(r[i]) for i in kidx) in keep_keys[t["name"]]]
        if t["name"] in drop_keys:
            kidx = [cols0.index(k) for k in t["key"]]
            rows = [r for r in rows if tuple(json.dumps(r[i]) for i in kidx) not in drop_keys[t["name"]]]
        for tn, col, grp in renumber:
            if tn != t["name"]:
                continue
            ci, gi = cols0.index(col), cols0.index(grp)
            counters = {}
            rows = [list(r) for r in rows]
            for r in rows:
                n = counters.get(json.dumps(r[gi]), 0)
                counters[json.dumps(r[gi])] = n + 1
                r[ci] = n
        t = dict(t)
        t["rows"] = rows
        if tables is not None and t["name"] not in tables:
            continue
        keep = [i for i, c in enumerate(t["columns"]) if not (drop_hex and c["name"].endswith("_hex"))]
        if t["name"] in keep_cols:
            wanted = keep_cols[t["name"]]
            missing_cols = set(wanted) - {c["name"] for c in t["columns"]}
            if missing_cols:
                print("filter_golden: columns not found in", t["name"], sorted(missing_cols))
                return 1
            keep = [i for i in keep if t["columns"][i]["name"] in wanted]
        if t["name"] in drop_cols:
            unknown = set(drop_cols[t["name"]]) - {c["name"] for c in t["columns"]}
            if unknown:
                print("filter_golden: columns not found in", t["name"], sorted(unknown))
                return 1
            gone = set(drop_cols[t["name"]]) | {c + "_hex" for c in drop_cols[t["name"]]}
            keep = [i for i in keep if t["columns"][i]["name"] not in gone]
        t = dict(t)
        t["columns"] = [t["columns"][i] for i in keep]
        t["rows"] = [[r[i] for i in keep] for r in t["rows"]]
        t["row_count"] = len(t["rows"])
        out_tables.append(t)
    if tables is not None:
        missing = set(tables) - {t["name"] for t in out_tables}
        if missing:
            print("filter_golden: tables not found:", sorted(missing))
            return 1
    g["tables"] = out_tables
    with open(dst, "w", encoding="utf-8") as f:
        json.dump(g, f, indent=0)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
