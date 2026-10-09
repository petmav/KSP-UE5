"""Merge auto features + hand reviews into analysis/tiering/tiering.csv and print summary stats.

Run: python -I merge.py
Precedence (highest first):
  work/review/lead_overrides.tsv   lead corrections after consistency review
  work/review/lead_big_*.tsv       lead hand review of every KSP-authored file > 800 lines
  work/review/child_*.tsv          sonnet reviewer rows (root, KSP/, Expansions/, CommNet/, Contracts/, FinePrint/ ...)
  work/review/script_overrides.tsv lead corrections for script-classified small directories
  group templates below            homogeneous small directories (Experience, KSPAchievements, PreFlightTests, ...)
  work/auto.json                   automatic suggestion (fallback; flagged review_source=script-auto)
Third-party embedded code gets tier "third-party" with the library decision from THIRD_PARTY_DECISION.
"""
import csv, glob, json, os, re, sys
from collections import Counter, defaultdict

BASE = r"C:/Projects/KSP-UE5/analysis/tiering"
WORK = os.path.join(BASE, "work")
TAXONOMY = ["Core", "Save", "Modding", "Scene", "Orbit", "Celestials", "Terrain", "Physics", "Vessel", "Part",
            "Resources", "PartModules-Engines", "PartModules-Wheels", "PartModules-Control",
            "PartModules-Structural", "PartModules-Resources", "PartModules-Misc", "Aero", "Thermal", "Editor",
            "FlightUI", "MapView", "UI", "Career", "Science", "Contracts", "Kerbals", "CommNet", "Localization",
            "Audio", "Camera", "Rendering", "Input", "Steam", "Analytics", "Tutorials", "Debug",
            "DLC-MakingHistory", "DLC-BreakingGround", "Misc"]
THIRD_PARTY_DECISION = {
    "TMPro": ("UI", "drop (UMG text / Slate font rendering)", "TextMesh Pro text rendering; consumed by 300+ KSP UI files; UE5 text rendering replaces it."),
    "Smooth": ("Core", "drop (std containers / object pools)", "Smooth.Foundations; KSP uses only Pools/Dispose (Orbit, Targeting, AudioMultiPooledFX); replace with C++ pools."),
    "VehiclePhysics": ("PartModules-Wheels", "port subset (VehicleBase solver, VPWheelCollider, TireFriction) into core wheel model", "Edy's Vehicle Physics; KSP wheels run on VehicleBase+VPWheelCollider+TireFriction; ~26 of 99 files reachable."),
    "EdyCommonTools": ("PartModules-Wheels", "port 4 helpers inline / drop rest", "Edy's common tools; only MathUtility/BiasedRatio/InterpolatedFloat/ColliderUtility reached via VehiclePhysics."),
    "LibNoise": ("Terrain", "port bit-exact (Perlin, RidgedMultifractal, Billow, Voronoi + basis/modifiers used)", "LibNoise C# port; drives 14 PQSMod noise height/colour mods; terrain heights must match KSP."),
    "SoftMasking": ("UI", "drop (UMG Retainer Box / material mask)", "Soft Mask for Unity UI; only used by NavBallBurnVector."),
    "UniLinq": ("Core", "drop (C++ algorithms/ranges)", "AOT-safe LINQ reimplementation used via extension methods in 11 KSP files."),
    "DDSHeaders": ("Rendering", "replace (UE texture import / DirectXTex if loading user assets)", "DDS header structs for DatabaseLoaderTexture_DDS."),
    "Highlighting": ("Rendering", "replace (Custom Depth/Stencil + post-process outline)", "Highlighting System (part outline glow); used by Part, editor, cameras."),
}
GROUP_TEMPLATES = [
    (r"/Experience/Effects/", "A", "Kerbals", "core", "Kerbal trait effect {name}: scales a module stat by crew experience level; pure rules."),
    (r"/Experience/", "A", "Kerbals", "core", "Experience system {name} (trait/effect config and registry); pure data/rules."),
    (r"/KSPAchievements/", "A", "Career", "core", "Progress-tracking node {name} (world firsts/records feeding career rewards); pure rules."),
    (r"/PreFlightTests/", "A", "Editor", "core", "Editor pre-flight design check {name} over craft part/module data; pure rules."),
    (r"/SaveUpgradePipeline/", "A", "Save", "core", "Save/craft upgrade pipeline {name}: rewrites legacy ConfigNodes on load; pure data transform."),
    (r"/Strategies/", "A", "Career", "core", "Administration strategies {name}: currency conversion/modifier rules and persistence; pure rules."),
    (r"/CameraFXModules/", "C", "Camera", "Camera Shake / Post Process Volume", "Camera FX module {name} (shake/wobble/fade); presentation."),
]


def read_tsv(path):
    with open(path, encoding="utf-8-sig") as fh:
        rows = list(csv.DictReader(fh, delimiter="\t"))
    return {r["path"].strip(): {k: (v or "").strip() for k, v in r.items() if k} for r in rows if r.get("path")}


def main():
    feats = {json.loads(l)["path"]: json.loads(l) for l in open(os.path.join(WORK, "features.jsonl"), encoding="utf-8")}
    auto = json.load(open(os.path.join(WORK, "auto.json"), encoding="utf-8"))
    sources = []   # (label, dict) in increasing precedence
    so = os.path.join(WORK, "review", "script_overrides.tsv")
    if os.path.exists(so):
        sources.append(("script+lead-check", read_tsv(so)))
    for f in sorted(glob.glob(os.path.join(WORK, "review", "child_*.tsv"))):
        sources.append(("hand:sonnet-" + os.path.basename(f)[:-4], read_tsv(f)))
    for f in sorted(glob.glob(os.path.join(WORK, "review", "lead_big_*.tsv"))):
        sources.append(("hand:lead", read_tsv(f)))
    for f in sorted(glob.glob(os.path.join(WORK, "review", "lead_rationale*.tsv"))):
        sources.append(("hand:lead-rationale", read_tsv(f)))
    for f in sorted(glob.glob(os.path.join(WORK, "review", "lead_override*.tsv"))):
        sources.append(("hand:lead-override", read_tsv(f)))
    out, problems = [], []
    for path in sorted(auto, key=str.lower):
        a, f = auto[path], feats[path]
        rec = {"tier": a["tier"], "subsystem": a["subsystem"], "ue5_counterpart": "", "rationale": "", "review_source": "script-auto"}
        if a["third_party"]:
            sub, decision, why = THIRD_PARTY_DECISION[a["third_party"]]
            rec.update(tier="third-party", subsystem=sub, ue5_counterpart=decision,
                       rationale=f"[{a['third_party']}] {why}", review_source="third-party rule")
        else:
            for pat, t, s, c, r in GROUP_TEMPLATES:
                if re.search(pat, path):
                    rec.update(tier=t, subsystem=s, ue5_counterpart=c, rationale=r.format(name=a["type"]),
                               review_source="script+group-template")
                    break
            for label, src in sources:
                if path in src:
                    r = src[path]
                    old_source = rec["review_source"]
                    rec.update(tier=r.get("tier") or rec["tier"], subsystem=r.get("subsystem") or rec["subsystem"],
                               ue5_counterpart=r.get("ue5_counterpart") or rec["ue5_counterpart"],
                               rationale=r.get("rationale") or rec["rationale"], review_source=label)
                    if label in ("hand:lead-override", "hand:lead-rationale"):
                        rec["review_source"] = old_source.split(" >")[0] + " > " + label.split(":")[1]
            if not rec["rationale"]:
                rec["rationale"] = f"Auto: {a['kind']} {a['type']}; Unity deps {', '.join(a['unity_top'][:3]) or 'none'}."
        if rec["tier"] not in ("A", "B", "C", "third-party"):
            problems.append((path, "tier", rec["tier"]))
        if rec["subsystem"] not in TAXONOMY:
            problems.append((path, "subsystem", rec["subsystem"]))
        if rec["tier"] == "C" and not rec["ue5_counterpart"]:
            problems.append((path, "no counterpart", ""))
        if rec["tier"] == "A" and not rec["ue5_counterpart"]:
            rec["ue5_counterpart"] = "core"
        keydeps = "; ".join(x for x in [" ".join(a["unity_top"][:5]), " ".join(a["ksp_top"][:5])] if x)
        out.append({"path": path, "lines": a["lines"], "kind": a["kind"], "tier": rec["tier"],
                    "subsystem": rec["subsystem"], "unity_dependency_score": a["unity_score"],
                    "key_dependencies": keydeps, "ue5_counterpart": rec["ue5_counterpart"],
                    "rationale": rec["rationale"].replace("\n", " "), "review_source": rec["review_source"],
                    "fan_in": a["fan_in"], "namespace": a["namespace"]})
    # post-pass: rows whose reviewer rationale is a group template (same text, modulo class name, used for
    # several files) get script-derived evidence appended so every row is file-specific, and are labelled.
    def norm(row):
        return row["rationale"].replace(os.path.basename(row["path"])[:-3], "<N>")
    tcount = Counter((row["review_source"], norm(row)) for row in out)
    for row in out:
        src = row["review_source"]
        if src.startswith("hand:sonnet") and " > " not in src and tcount[(src, norm(row))] > 1:
            a = auto[row["path"]]
            base = ",".join(a["bases"][:2]) or "-"
            unity = " ".join(a["unity_top"][:3]) or "no Unity refs"
            ms = ",".join(m for m in a["methods"][:5]) or "-"
            row["rationale"] += f" [evidence: {a['type']} : {base}; {unity}; methods {ms}]"
            row["review_source"] = src + " (group rationale + script evidence)"
    cols = ["path", "lines", "kind", "tier", "subsystem", "unity_dependency_score", "key_dependencies",
            "ue5_counterpart", "rationale", "review_source", "fan_in", "namespace"]
    with open(os.path.join(BASE, "tiering.csv"), "w", encoding="utf-8", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=cols)
        w.writeheader()
        w.writerows(out)
    # ------------------------------------------------------------ summaries
    t_files, t_lines = Counter(), Counter()
    s_tab = defaultdict(lambda: Counter())
    src = Counter()
    for r in out:
        t_files[r["tier"]] += 1
        t_lines[r["tier"]] += r["lines"]
        s_tab[r["subsystem"]][r["tier"]] += 1
        s_tab[r["subsystem"]][r["tier"] + "_lines"] += r["lines"]
        src[r["review_source"].split(" >")[0]] += 1
    summary = {"rows": len(out), "tier_files": dict(t_files), "tier_lines": dict(t_lines),
               "subsystems": {k: dict(v) for k, v in s_tab.items()}, "review_source": dict(src),
               "problems": problems}
    json.dump(summary, open(os.path.join(WORK, "summary.json"), "w", encoding="utf-8"), indent=1)
    print("rows", len(out), "problems", len(problems), problems[:20])
    print("tier files", dict(t_files))
    print("tier lines", dict(t_lines))
    print("sources", dict(src))


if __name__ == "__main__":
    main()
