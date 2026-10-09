"""Work-package assignment + summary tables for TIERING.md.

Run: python -I report.py      (after merge.py)
Writes:
  analysis/tiering/work_packages.csv    path -> work package (every row of tiering.csv)
  work/tables.md                        generated tables pasted into TIERING.md (tiers, subsystems, top-40, WPs)
  work/wp_stats.json
Rules: first matching rule wins. A rule = (wp, path regex or None, subsystems or None, tiers or None).
"""
import csv, json, math, os, re
from collections import Counter, defaultdict, OrderedDict

BASE = r"C:/Projects/KSP-UE5/analysis/tiering"
WORK = os.path.join(BASE, "work")

import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from wp_rules import WPS, assign  # noqa: E402


def main():
    rows = list(csv.DictReader(open(os.path.join(BASE, "tiering.csv"), encoding="utf-8")))
    for r in rows:
        r["lines"] = int(r["lines"])
        r["fan_in"] = int(r["fan_in"])
        r["wp"] = assign(r)
    with open(os.path.join(BASE, "work_packages.csv"), "w", encoding="utf-8", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["path", "lines", "tier", "subsystem", "work_package", "work_package_name"])
        for r in rows:
            w.writerow([r["path"], r["lines"], r["tier"], r["subsystem"], r["wp"], WPS[r["wp"]][0]])
    out = []
    # tier table
    tf, tl = Counter(), Counter()
    for r in rows:
        tf[r["tier"]] += 1
        tl[r["tier"]] += r["lines"]
    tot_f, tot_l = sum(tf.values()), sum(tl.values())
    ksp_f = tot_f - tf["third-party"]
    ksp_l = tot_l - tl["third-party"]
    out.append("### Totals per tier\n")
    out.append("| Tier | Files | % of KSP files | Lines | % of KSP lines |")
    out.append("|---|---:|---:|---:|---:|")
    for t in ["A", "B", "C"]:
        out.append(f"| {t} | {tf[t]:,} | {100*tf[t]/ksp_f:.1f}% | {tl[t]:,} | {100*tl[t]/ksp_l:.1f}% |")
    out.append(f"| **KSP-authored total** | **{ksp_f:,}** | 100% | **{ksp_l:,}** | 100% |")
    out.append(f"| third-party (excluded) | {tf['third-party']:,} | | {tl['third-party']:,} | |")
    out.append(f"| **All rows** | **{tot_f:,}** | | **{tot_l:,}** | |\n")
    # by assembly
    asm = defaultdict(Counter)
    for r in rows:
        a = r["path"].split("/")[0]
        asm[a][r["tier"]] += 1
        asm[a][r["tier"] + "_l"] += r["lines"]
    out.append("| Assembly | A files | B files | C files | third-party files | Lines (A/B/C/3p) |")
    out.append("|---|---:|---:|---:|---:|---|")
    for a, c in sorted(asm.items()):
        out.append(f"| {a} | {c['A']} | {c['B']} | {c['C']} | {c['third-party']} | {c['A_l']:,} / {c['B_l']:,} / {c['C_l']:,} / {c['third-party_l']:,} |")
    out.append("")
    # subsystem table
    sub = defaultdict(Counter)
    for r in rows:
        if r["tier"] == "third-party":
            continue
        s = sub[r["subsystem"]]
        s[r["tier"]] += 1
        s[r["tier"] + "_l"] += r["lines"]
        s["n"] += 1
        s["l"] += r["lines"]
    out.append("### Totals per subsystem (KSP-authored files only)\n")
    out.append("| Subsystem | Files | Lines | A files | A lines | B files | B lines | C files | C lines |")
    out.append("|---|---:|---:|---:|---:|---:|---:|---:|---:|")
    for k, s in sorted(sub.items(), key=lambda kv: -kv[1]["l"]):
        out.append(f"| {k} | {s['n']} | {s['l']:,} | {s['A']} | {s['A_l']:,} | {s['B']} | {s['B_l']:,} | {s['C']} | {s['C_l']:,} |")
    out.append("")
    # review provenance
    src = Counter()
    for r in rows:
        src[r["review_source"]] += 1
    out.append("### Review provenance\n")
    out.append("| Source | Rows |")
    out.append("|---|---:|")
    for k, v in src.most_common():
        out.append(f"| {k} | {v} |")
    out.append("")
    # top-40 leverage
    ksp = [r for r in rows if r["tier"] != "third-party"]
    for r in ksp:
        r["lev"] = r["fan_in"] + r["lines"] / 25.0
    top = sorted(ksp, key=lambda r: -r["lev"])[:40]
    out.append("### Top 40 highest-leverage files\n")
    out.append("Leverage = fan-in (distinct KSP files referencing a type declared in the file) + lines/25. "
               "So 25 lines of code weigh the same as one dependent file.\n")
    out.append("| # | File | Lines | Fan-in | Tier | Subsystem | Work package |")
    out.append("|---:|---|---:|---:|---|---|---|")
    for i, r in enumerate(top, 1):
        out.append(f"| {i} | `{r['path'].replace('Assembly-CSharp/', '')}` | {r['lines']:,} | {r['fan_in']} | {r['tier']} | {r['subsystem']} | {r['wp']} |")
    out.append("")
    # work package table
    wps = defaultdict(lambda: Counter())
    files_of = defaultdict(list)
    for r in rows:
        c = wps[r["wp"]]
        c[r["tier"]] += 1
        c["l"] += r["lines"]
        c["n"] += 1
        files_of[r["wp"]].append(r)
    out.append("### Work-package sizes (generated from work_packages.csv)\n")
    out.append("| WP | Name | Files | Lines | A | B | C | 3p | Verify | Largest files |")
    out.append("|---|---|---:|---:|---:|---:|---:|---:|---|---|")
    stats = {}
    for wp, (name, ver, acc) in WPS.items():
        c = wps[wp]
        big = ", ".join(f"`{os.path.basename(r['path'])}`" for r in sorted(files_of[wp], key=lambda r: -r["lines"])[:4])
        out.append(f"| {wp} | {name} | {c['n']} | {c['l']:,} | {c['A']} | {c['B']} | {c['C']} | {c['third-party']} | {ver} | {big} |")
        stats[wp] = {"name": name, "files": c["n"], "lines": c["l"], "A": c["A"], "B": c["B"], "C": c["C"],
                     "3p": c["third-party"], "verify": ver, "acceptance": acc,
                     "largest": [r["path"] for r in sorted(files_of[wp], key=lambda r: -r["lines"])[:8]]}
    out.append("")
    open(os.path.join(WORK, "tables.md"), "w", encoding="utf-8").write("\n".join(out))
    json.dump(stats, open(os.path.join(WORK, "wp_stats.json"), "w", encoding="utf-8"), indent=1)
    empty = [wp for wp in WPS if wps[wp]["n"] == 0]
    print("rows", len(rows), "empty WPs", empty)
    for wp in WPS:
        c = wps[wp]
        print(wp, WPS[wp][0][:50], c["n"], c["l"], dict((k, c[k]) for k in ("A", "B", "C", "third-party")))


if __name__ == "__main__":
    main()
