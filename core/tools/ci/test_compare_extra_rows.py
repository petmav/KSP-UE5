"""Check harness/compare/compare.py's extra-row rule (run with `python -I`; ctest `compare.extra_rows`).

compare.py must fail when the candidate has rows whose key is not in the golden, unless the table is in its ALLOWED_EXTRA_ROWS
allow-list with exactly that many rows. Synthetic ksp-golden/1 files (no game data); the allow-list entries are the real ones.
"""
import json
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
COMPARE = None
d = HERE
for _ in range(8):
    cand = os.path.join(d, "harness", "compare", "compare.py")
    if os.path.isfile(cand):
        COMPARE = cand
        break
    d = os.path.dirname(d)
if COMPARE is None:
    sys.exit("harness/compare/compare.py not found above " + HERE)


def doc(dataset, table, keys):
    return {"schema": "ksp-golden/1", "dataset": dataset, "tables": [{
        "name": table, "key": ["k"],
        "columns": [{"name": "k", "type": "i64"}, {"name": "v", "type": "f64"}],
        "rows": [[k, 1.5] for k in keys]}]}


def run(dataset, table, golden_keys, cand_keys):
    with tempfile.TemporaryDirectory() as tmp:
        g, c = os.path.join(tmp, "g.json"), os.path.join(tmp, "c.json")
        json.dump(doc(dataset, table, golden_keys), open(g, "w"))
        json.dump(doc(dataset, table, cand_keys), open(c, "w"))
        p = subprocess.run([sys.executable, "-I", COMPARE, g, c], capture_output=True, text=True)
        return p.returncode, p.stdout


failures = []


def expect(name, got, want):
    if got != want:
        failures.append(f"{name}: exit {got}, expected {want}")
    print(("ok   " if got == want else "FAIL ") + name)


# no extra rows: passes
expect("identical rows", run("ds", "t", range(10), range(10))[0], 0)
# one extra row in a table that is not allow-listed: fails, and the key is printed
rc, out = run("ds", "t", range(10), list(range(10)) + [99])
expect("bogus extra row, unlisted table", rc, 1)
if "[99]" not in out:
    failures.append("the extra key is not printed")
# allow-listed table (part_database/parts: exactly 5)
expect("listed table, exactly 5 extra", run("part_database", "parts", range(10), range(15))[0], 0)
expect("listed table, 6 extra", run("part_database", "parts", range(10), range(16))[0], 1)
expect("listed table, 4 extra", run("part_database", "parts", range(10), range(14))[0], 1)
# a listed dataset with another table name is not covered
expect("listed dataset, other table", run("part_database", "other", range(10), range(15))[0], 1)
# a missing row still fails
expect("missing row", run("ds", "t", range(10), range(9))[0], 1)
if failures:
    print("\n".join(failures))
    sys.exit(1)
print("all extra-row checks passed")
