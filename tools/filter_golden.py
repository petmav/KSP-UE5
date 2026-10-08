"""Write a filtered copy of a ksp-golden/1 file for compare.py (run with `python -I`).

    python -I filter_golden.py SRC DST [--tables t1,t2,...] [--drop-hex]

--tables    keep only these tables (default: all)
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
    drop_hex = "--drop-hex" in argv
    for i, a in enumerate(argv):
        if a == "--tables":
            tables = [t for t in argv[i + 1].split(",") if t]
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
