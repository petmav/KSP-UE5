"""Write a filtered copy of a ksp-golden/1 file for compare.py (run with `python -I`).

    python -I filter_golden.py SRC DST [--tables t1,t2,...] [--keep-columns t:c1,c2/t2:c3,...] [--drop-hex]

--tables        keep only these tables (default: all)
--keep-columns  per table, keep only the listed columns (key columns must be listed); tables not named keep all
                columns. Used when some columns need code another package provides (documented in tests/CMakeLists.txt
                and STATUS.md); never to hide a mismatch.
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
    for i, a in enumerate(argv):
        if a == "--tables":
            tables = [t for t in argv[i + 1].split(",") if t]
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
    out_tables = []
    for t in g["tables"]:
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
