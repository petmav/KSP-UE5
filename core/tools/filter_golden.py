"""Write a filtered copy of a ksp-golden/1 file for compare.py (run with `python -I`).

    python -I filter_golden.py SRC DST [--tables t1,t2] [--key t:c1,c2/...]
                               [--keep-rows ROWSPEC] [--drop-rows ROWSPEC] [--keep-rows-file FILE]
                               [--drop-keys-file FILE] [--renumber t:col:groupcol/...]
                               [--drop-hex] [--keep-columns t:c1,c2/...] [--drop-columns t:c1,c2/...]

Every option maps 1:1 to a ksp_golden_gate() option in tests/CMakeLists.txt (README.md "Golden gate options").
Each filter that removes golden data must be documented next to its gate and in STATUS.md, with the reason the data is
outside the package's scope (another package's code, objects that are not the experiment's inputs). Never use a filter
to hide a mismatch.

ROWSPEC  table:cond[+cond...][/table2:...], one or more conditions per table, OR'd (a row matches when any holds):
           col=v1|v2|...   the cell's text equals one of the values (strings as-is, null reads "null", numbers and
                           booleans as JSON text)
           col^p1|p2|...   the cell is a string starting with one of the prefixes
         Values cannot contain '/', '+' or '|'.

Steps, applied per table in this order:
 1. --tables          keep only these tables (default: all); a listed table missing from SRC is an error.
 2. --key             set the key columns (capture tables written without a key; compare.py needs one). Steps 4-5
                      use the key set here.
 3. --keep-rows       keep only the rows matching ROWSPEC (objects that belong to the experiment's inputs).
    --drop-rows       then drop the rows matching ROWSPEC (rows outside the package's scope by a property of the golden
                      row itself, e.g. assets kind=texture).
 4. --keep-rows-file  JSON {table: [[key values...], ...]}: keep only the rows whose key values are listed (a scope
                      list the runner derives from its INPUTS, e.g. the module fields a part cfg sets; never from the
                      comparison).
 5. --drop-keys-file  JSON {table: [{"key": [key values...], "reason": "..."}, ...]} (top-level names starting with
                      "_" are comments): drop individually listed rows, each with its documented reason.
 6. --renumber        rewrite an index key column as 0, 1, 2... within each value of a group column (row order kept),
                      for tables whose index counted rows that were dropped.
 7. --drop-hex        drop the `<col>_hex` companion columns of the in-game capture files (harness/captures/out*). The
                      decimals in those files are rewritten from the hex bits in the harness's exact number format, so
                      comparing the decimals with compare.py's ULP metric is equivalent; the candidate does not need to
                      emit hex columns.
    --keep-columns    keep only the listed columns of the named tables (key columns must be listed); tables not named
                      keep all columns.
    --drop-columns    drop the listed columns (and their `_hex` companions) of the named tables.

A table name or column named by any option but absent from SRC is an error (a typo must not silently widen a gate).
The source file is never modified (harness/ is read-only); the copy goes into the build tree.
"""
import json
import re
import sys

VALUE_OPTS = ("--tables", "--key", "--keep-rows", "--drop-rows", "--keep-rows-file", "--drop-keys-file", "--renumber",
              "--keep-columns", "--drop-columns")
FLAG_OPTS = ("--drop-hex",)


def table_specs(spec):
    """'t:a,b/t2:c' -> [(t, 'a,b'), (t2, 'c')]"""
    return [tuple(s.split(":", 1)) for s in spec.split("/") if s]


def parse_columns(spec):
    out = {}
    for t, cols in table_specs(spec):
        out.setdefault(t, []).extend(c for c in cols.split(",") if c)
    return out


def parse_rowspec(spec):
    out = {}
    for t, conds in table_specs(spec):
        for cond in conds.split("+"):
            m = re.fullmatch(r"([^=^]+)([=^])(.*)", cond)
            if not m:
                raise ValueError("bad row condition %r for table %s" % (cond, t))
            out.setdefault(t, []).append((m.group(1), m.group(2), m.group(3).split("|")))
    return out


def cell_text(v):
    return "null" if v is None else (v if isinstance(v, str) else json.dumps(v))


def matches(v, op, vals):
    if op == "=":
        return cell_text(v) in vals
    return isinstance(v, str) and any(v.startswith(p) for p in vals)


def load_key_file(path, entry_key=None):
    out = {}
    with open(path, encoding="utf-8") as f:
        for tname, entries in json.load(f).items():
            if tname.startswith("_"):
                continue
            out[tname] = {tuple(json.dumps(v) for v in (e[entry_key] if entry_key else e)) for e in entries}
    return out


def main(argv):
    opts, args, i = {}, [], 0
    while i < len(argv):
        a = argv[i]
        if a in VALUE_OPTS:
            if i + 1 >= len(argv):
                print("filter_golden: missing value for", a)
                return 2
            opts[a] = argv[i + 1]
            i += 2
        elif a in FLAG_OPTS:
            opts[a] = True
            i += 1
        elif a.startswith("--"):
            print("filter_golden: unknown option", a)
            print(__doc__)
            return 2
        else:
            args.append(a)
            i += 1
    if len(args) != 2:
        print(__doc__)
        return 2
    src, dst = args

    tables = [t for t in opts["--tables"].split(",") if t] if "--tables" in opts else None
    keys = parse_columns(opts.get("--key", ""))
    keep_rows = parse_rowspec(opts.get("--keep-rows", ""))
    drop_rows = parse_rowspec(opts.get("--drop-rows", ""))
    keep_keys = load_key_file(opts["--keep-rows-file"]) if "--keep-rows-file" in opts else {}
    drop_keys = load_key_file(opts["--drop-keys-file"], "key") if "--drop-keys-file" in opts else {}
    renumber = {}
    for spec in [s for s in opts.get("--renumber", "").split("/") if s]:
        t, col, grp = spec.split(":")
        renumber.setdefault(t, []).append((col, grp))
    keep_cols = parse_columns(opts.get("--keep-columns", ""))
    drop_cols = parse_columns(opts.get("--drop-columns", ""))
    drop_hex = opts.get("--drop-hex", False)

    with open(src, encoding="utf-8") as f:
        g = json.load(f)

    src_tables = {t["name"]: [c["name"] for c in t["columns"]] for t in g["tables"]}
    named = {"--tables": tables or [], "--key": keys, "--keep-rows": keep_rows, "--drop-rows": drop_rows,
             "--keep-rows-file": keep_keys, "--drop-keys-file": drop_keys, "--renumber": renumber,
             "--keep-columns": keep_cols, "--drop-columns": drop_cols}
    for opt, tnames in named.items():
        missing = sorted(set(tnames) - set(src_tables))
        if missing:
            print("filter_golden: %s names tables not found in %s: %s" % (opt, src, missing))
            return 1
    for opt, per_table in (("--key", keys), ("--keep-columns", keep_cols), ("--drop-columns", drop_cols),
                           ("--keep-rows", {t: [c[0] for c in cs] for t, cs in keep_rows.items()}),
                           ("--drop-rows", {t: [c[0] for c in cs] for t, cs in drop_rows.items()}),
                           ("--renumber", {t: [c for p in ps for c in p] for t, ps in renumber.items()})):
        for t, cols in per_table.items():
            unknown = sorted(set(cols) - set(src_tables[t]))
            if unknown:
                print("filter_golden: %s names columns not found in %s: %s" % (opt, t, unknown))
                return 1

    out_tables = []
    for t in g["tables"]:
        name = t["name"]
        if tables is not None and name not in tables:
            continue
        t = dict(t)
        cols0 = src_tables[name]
        rows = t["rows"]
        # 2. key
        if name in keys:
            t["key"] = keys[name]
        # 3. row conditions
        if name in keep_rows:
            conds = [(cols0.index(c), op, vals) for c, op, vals in keep_rows[name]]
            rows = [r for r in rows if any(matches(r[ci], op, vals) for ci, op, vals in conds)]
        if name in drop_rows:
            conds = [(cols0.index(c), op, vals) for c, op, vals in drop_rows[name]]
            rows = [r for r in rows if not any(matches(r[ci], op, vals) for ci, op, vals in conds)]
        # 4./5. key lists
        if name in keep_keys or name in drop_keys:
            kidx = [cols0.index(k) for k in t["key"]]
            if name in keep_keys:
                rows = [r for r in rows if tuple(json.dumps(r[i]) for i in kidx) in keep_keys[name]]
            if name in drop_keys:
                rows = [r for r in rows if tuple(json.dumps(r[i]) for i in kidx) not in drop_keys[name]]
        # 6. renumber
        for col, grp in renumber.get(name, []):
            ci, gi = cols0.index(col), cols0.index(grp)
            counters = {}
            rows = [list(r) for r in rows]
            for r in rows:
                n = counters.get(json.dumps(r[gi]), 0)
                counters[json.dumps(r[gi])] = n + 1
                r[ci] = n
        # 7. columns
        keep = [i for i, c in enumerate(cols0) if not (drop_hex and c.endswith("_hex"))]
        if name in keep_cols:
            keep = [i for i in keep if cols0[i] in keep_cols[name]]
        if name in drop_cols:
            gone = set(drop_cols[name]) | {c + "_hex" for c in drop_cols[name]}
            keep = [i for i in keep if cols0[i] not in gone]
        t["columns"] = [t["columns"][i] for i in keep]
        t["rows"] = [[r[i] for i in keep] for r in rows]
        t["row_count"] = len(t["rows"])
        out_tables.append(t)
    g["tables"] = out_tables
    with open(dst, "w", encoding="utf-8") as f:
        json.dump(g, f, indent=0)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
