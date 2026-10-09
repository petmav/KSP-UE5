"""Split the review work after the reviewer cap (4 sonnet children + hand review by the lead).

Run: python -I make_lists.py         (after classify.py)
Writes work/lists/{lead_big.md, child_1..4.md} (compact digests) and work/lists/assignment.json
- lead_big: every KSP-authored file > 800 lines (hand-reviewed by the lead).
- child_1..4: remaining files (<= 800 lines) in the directories that require a hand rationale:
  root, KSP/, Expansions/, CommNet/, Contracts/, FinePrint/ (+ SentinelMission/, KerbNet/ which are career/comm code).
- everything else (small directories, firstpass) is script-classified (work/auto.json + templates in merge.py).
"""
import json, os

WORK = r"C:/Projects/KSP-UE5/analysis/tiering/work"


def compact(p, a):
    ms = a["methods"]
    return (f"## {p}\n- L={a['lines']} kind={a['kind']} type={a['type']} bases={','.join(a['bases'][:3]) or '-'} "
            f"anc={'/'.join(a['ancestry'][:3]) or '-'} ns={a['namespace'] or '-'}\n"
            f"- unity({a['unity_score']}): {', '.join(a['unity_top'][:6]) or 'none'} | ksp: {', '.join(a['ksp_top'][:5]) or 'none'}"
            f" | fan_in={a['fan_in']}\n"
            f"- methods({len(ms)}): {', '.join(ms[:24])}{' ...' if len(ms) > 24 else ''}\n"
            f"- auto: {a['tier']} / {a['subsystem']}\n")


def main():
    auto = json.load(open(os.path.join(WORK, "auto.json"), encoding="utf-8"))
    ksp = sorted((p for p, a in auto.items() if not a["third_party"]), key=str.lower)
    big = [p for p in ksp if auto[p]["lines"] > 800]

    def area(p):
        if not p.startswith("Assembly-CSharp/"):
            return None
        rel = p.split("/", 1)[1]
        if "/" not in rel:
            return "root"
        top = rel.split("/")[0]
        if top == "KSP":
            return "KSP"
        if top == "Expansions":
            return "Expansions"
        if top in ("CommNet", "Contracts", "FinePrint", "SentinelMission", "KerbNet"):
            return "Career"
        return None
    rest = [p for p in ksp if p not in set(big) and area(p)]
    root = [p for p in rest if area(p) == "root"]
    kspd = [p for p in rest if area(p) == "KSP"]
    exp = [p for p in rest if area(p) == "Expansions"]
    car = [p for p in rest if area(p) == "Career"]
    n = (len(rest) + 3) // 4
    lists = {
        "child_1": root[:n],
        "child_2": root[n:2 * n],
        "child_3": kspd + car,
        "child_4": exp + root[2 * n:],
    }
    os.makedirs(os.path.join(WORK, "lists"), exist_ok=True)
    os.makedirs(os.path.join(WORK, "review"), exist_ok=True)
    with open(os.path.join(WORK, "lists", "lead_big.md"), "w", encoding="utf-8") as fh:
        fh.write(f"# Lead hand review: {len(big)} files > 800 lines\n\n" + "\n".join(compact(p, auto[p]) for p in big))
    for k, ps in lists.items():
        with open(os.path.join(WORK, "lists", f"{k}.md"), "w", encoding="utf-8") as fh:
            fh.write(f"# {k}: {len(ps)} files, {sum(auto[p]['lines'] for p in ps)} lines\n\n" +
                     "\n".join(compact(p, auto[p]) for p in ps))
    script_only = [p for p in ksp if p not in set(big) and not area(p)]
    json.dump({"lead_big": big, **lists, "script_only": script_only},
              open(os.path.join(WORK, "lists", "assignment.json"), "w", encoding="utf-8"), indent=1)
    print("big", len(big), {k: len(v) for k, v in lists.items()}, "script_only", len(script_only),
          "total", len(big) + sum(len(v) for v in lists.values()) + len(script_only), "of", len(ksp))


if __name__ == "__main__":
    main()
