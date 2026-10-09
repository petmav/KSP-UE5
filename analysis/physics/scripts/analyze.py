#!/usr/bin/env python3
"""Aggregate PhysProf CSVs into markdown tables for PROFILE.md.
Usage: python analyze.py <path to X_summary.csv> [<path> ...] > tables.md
"""
import csv
import os
import re
import sys
from collections import OrderedDict, defaultdict

DIRS = {}
TAGS = []
for a in sys.argv[1:]:
    d, f = os.path.split(os.path.abspath(a))
    t = f[: -len("_summary.csv")]
    DIRS[t] = d
    TAGS.append(t)
NAMES_CS = r"C:\Projects\KSP-UE5\decompiled\KSP-Decompiled-profiling\Assembly-CSharp\PhysProf\PhysProfNames.cs"

# id -> source path, from the generated name table comments
SRC = {}
for line in open(NAMES_CS, encoding="utf-8"):
    m = re.match(r'\s*"([^"]+)", // (\d+) (\S+):(\d+)', line)
    if m:
        SRC[int(m.group(2))] = m.group(3)


def load_summary(tag):
    d = defaultdict(OrderedDict)
    with open(os.path.join(DIRS[tag], tag + "_summary.csv"), encoding="utf-8") as f:
        for r in csv.DictReader(f):
            d[r["window"]][r["key"]] = r["value"]
    return d


def load_csv(tag, win, kind):
    p = os.path.join(DIRS[tag], "%s_%s_%s.csv" % (tag, win, kind))
    with open(p, encoding="utf-8") as f:
        return list(csv.DictReader(f))


def fnum(x):
    try:
        return float(x)
    except Exception:
        return 0.0


PART_MODULE_PREFIX = ("Module", "FXModule", "ModuleWheel", "KerbalEVA", "LaunchClamp", "CModule")


def classify_method(mid, name):
    p = SRC.get(mid, "")
    if p.startswith("KSP/UI/") or p.startswith("Expansions/Missions/Editor") or "UI" in p.split("/")[0]:
        return "UI"
    return "core"


def slot_class(slot):
    if slot == "FixedUpdate.ScriptRunBehaviourFixedUpdate":
        return "FixedUpdate scripts"
    if slot == "FixedUpdate.PhysicsFixedUpdate":
        return "PhysX step (+collision callbacks)"
    if slot.startswith("FixedUpdate.") or "between FixedUpdate" in slot:
        return "FixedUpdate other"
    if slot == "Update.ScriptRunBehaviourUpdate":
        return "Update scripts"
    if slot == "Update.ScriptRunDelayedDynamicFrameRate":
        return "Coroutines (incl. PQS terrain)"
    if slot == "PreLateUpdate.ScriptRunBehaviourLateUpdate":
        return "LateUpdate scripts"
    if slot in ("PostLateUpdate.FinishFrameRendering", "PostLateUpdate.PresentAfterDraw", "PostLateUpdate.UpdateAllRenderers",
                "PostLateUpdate.UpdateAllSkinnedMeshes", "PostLateUpdate.UpdateCustomRenderTextures"):
        return "Rendering (cameras, renderers, present)"
    if slot in ("PostLateUpdate.PlayerUpdateCanvases", "PostLateUpdate.PlayerEmitCanvasGeometry", "PostLateUpdate.UpdateRectTransform",
                "PostLateUpdate.UpdateCanvasRectTransform", "EarlyUpdate.UpdateCanvasRectTransform"):
        return "UI canvases (uGUI)"
    if "ParticleSystem" in slot:
        return "Particles"
    if "Audio" in slot:
        return "Audio"
    if "Animation" in slot or "Director" in slot:
        return "Animation"
    if "between PostLateUpdate" in slot:
        return "Outside player loop (frame wait / present)"
    return "Other engine"


# physics components: (label, list of method names, inclusive?)
PHYS = [
    ("Planetarium.FixedUpdate (UT, body orbits)", ["Planetarium.FixedUpdate"]),
    ("VesselPrecalculate.FixedUpdate", ["VesselPrecalculate.FixedUpdate"]),
    ("OrbitDriver.FixedUpdate (all drivers)", ["OrbitDriver.FixedUpdate"]),
    ("CollisionEnhancer.FixedUpdate (per part)", ["CollisionEnhancer.FixedUpdate"]),
    ("FlightInputHandler.FixedUpdate (input, SAS/autopilot)", ["FlightInputHandler.FixedUpdate"]),
    ("Part.FixedUpdate (per part, incl. OnFixedUpdate)", ["Part.FixedUpdate"]),
    ("PartModule Unity FixedUpdates (sum)", ["@modules"]),
    ("PartBuoyancy.FixedUpdate (per part)", ["PartBuoyancy.FixedUpdate"]),
    ("FlightIntegrator.FixedUpdate (total)", ["FlightIntegrator.FixedUpdate"]),
    ("  FI.UpdateMassStats", ["FI.UpdateMassStats"]),
    ("  FI.DragCubeSetupAndPartAeroStats", ["FI.DragCubeSetupAndPartAeroStats"]),
    ("  FI.CheckThermalGraph", ["FI.CheckThermalGraph"]),
    ("  FI.UpdateThermodynamics", ["FI.UpdateThermodynamics"]),
    ("    of which UpdateConduction", ["FI.UpdateConduction"]),
    ("  FI.Integrate (force application, self)", ["@FI.Integrate.self"]),
    ("  FI.UpdateAerodynamics (drag, body lift)", ["FI.UpdateAerodynamics(per part)"]),
    ("Vessel.FixedUpdate", ["Vessel.FixedUpdate"]),
    ("Krakensbane.FixedUpdate", ["Krakensbane.FixedUpdate"]),
    ("FloatingOrigin.FixedUpdate (incl. setOffset)", ["FloatingOrigin.FixedUpdate"]),
    ("CollisionManager.FixedUpdate", ["CollisionManager.FixedUpdate"]),
    ("PQS.FixedUpdate", ["PQS.FixedUpdate"]),
]
UPD = [
    ("VesselPrecalculate.Update (CoM/MOI stats, orbit)", ["VesselPrecalculate.Update"]),
    ("  of which CalculatePhysicsStats (all calls)", ["VesselPrecalculate.CalculatePhysicsStats"]),
    ("FlightIntegrator.Update (sun raycast, occlusion)", ["FlightIntegrator.Update"]),
    ("Part.Update (incl. OnUpdate)", ["Part.Update"]),
    ("PQS.UpdateQuads (coroutine)", ["PQS.UpdateQuads(coroutine)"]),
    ("PQS.UpdateVisual + Mod hooks (coroutine)", ["PQS.UpdateVisual(coroutine)", "PQS.Mod_OnPreUpdate(coroutine)", "PQS.Mod_OnUpdateFinished(coroutine)"]),
]


def is_module_cb(name):
    cls, _, meth = name.rpartition(".")
    return meth in ("FixedUpdate",) and cls.startswith(("Module", "FXModule", "CModule", "LaunchClamp", "KerbalEVA", "BaseServo", "BaseConverter"))


def comp_ms(methods, keys, frames):
    by = {r["name"]: r for r in methods}
    tot = 0.0
    for k in keys:
        if k == "@modules":
            tot += sum(fnum(r["incl_ms_per_frame"]) for r in methods if is_module_cb(r["name"]))
        elif k == "@FI.Integrate.self":
            r = by.get("FI.Integrate(root)")
            tot += fnum(r["self_ms_per_frame"]) if r else 0.0
        else:
            r = by.get(k)
            tot += fnum(r["incl_ms_per_frame"]) if r else 0.0
    return tot


def main():
    tags = TAGS
    sums = {t: load_summary(t) for t in tags}
    wins = ["pad", "ascent"]
    out = []
    # ---- Table A: headline
    out.append("### A. Headline numbers\n")
    hdr = ["craft", "window", "parts", "rigidbodies", "attach joints (CJ)", "all joints", "colliders", "FPS", "frame ms mean", "p50", "p95", "p99",
           "fixed/frame", "sim rate (UT/wall)", "GC/min", "alloc MB/s (lower bound)", "alt end m", "srf speed end"]
    out.append("| " + " | ".join(hdr) + " |")
    out.append("|" + "---|" * len(hdr))
    for t in tags:
        for w in wins:
            s = sums[t].get(w)
            if not s:
                continue
            row = [t, w, s.get("parts"), s.get("rigidbodies"), s.get("attach_configurablejoints"), s.get("joint_components_all"),
                   s.get("colliders_enabled_nontrigger"), "%.1f" % fnum(s["fps"]), "%.2f" % fnum(s["frame_ms_mean"]), "%.2f" % fnum(s["frame_ms_p50"]),
                   "%.2f" % fnum(s["frame_ms_p95"]), "%.2f" % fnum(s["frame_ms_p99"]), "%.3f" % fnum(s["fixed_per_frame"]),
                   "%.3f" % fnum(s["sim_rate_ut_per_wall"]), "%.1f" % fnum(s["gc_per_min"]), "%.2f" % fnum(s["alloc_mb_per_s_lower_bound"]),
                   "%.0f" % fnum(s.get("altitude_m")), "%.0f" % fnum(s.get("srf_speed_ms"))]
            out.append("| " + " | ".join(str(x) for x in row) + " |")
    out.append("")
    # ---- Table B: frame breakdown by player-loop class
    out.append("### B. Frame breakdown by Unity player-loop phase (ms per rendered frame, % of frame)\n")
    classes = OrderedDict()
    data = {}
    for t in tags:
        for w in wins:
            try:
                slots = load_csv(t, w, "slots")
            except FileNotFoundError:
                continue
            agg = defaultdict(float)
            for r in slots:
                agg[slot_class(r["slot"])] += fnum(r["ms_per_frame"])
            data[(t, w)] = agg
            for k in agg:
                classes[k] = 1
    cols = [k for k in data]
    out.append("| phase | " + " | ".join("%s %s" % (t.split("-", 1)[-1], w) for t, w in cols) + " |")
    out.append("|---|" + "---|" * len(cols))
    order = sorted(classes, key=lambda k: -max(data[c].get(k, 0) for c in cols))
    for k in order:
        cells = []
        for c in cols:
            tot = sum(data[c].values())
            v = data[c].get(k, 0.0)
            cells.append("%.3f (%.0f%%)" % (v, 100 * v / tot if tot else 0))
        out.append("| %s | %s |" % (k, " | ".join(cells)))
    out.append("| **total** | " + " | ".join("%.3f" % sum(data[c].values()) for c in cols) + " |")
    out.append("")
    # ---- Table C: physics components per physics tick
    out.append("### C. Physics-tick components (ms per physics tick; inclusive)\n")
    out.append("| component | " + " | ".join("%s %s" % (t.split("-", 1)[-1], w) for t, w in cols) + " |")
    out.append("|---|" + "---|" * len(cols))
    meths = {c: load_csv(c[0], c[1], "methods") for c in cols}
    slotsd = {c: {r["slot"]: fnum(r["ms_per_frame"]) for r in load_csv(c[0], c[1], "slots")} for c in cols}
    fpf = {c: fnum(sums[c[0]][c[1]]["fixed_per_frame"]) for c in cols}
    for label, keys in PHYS:
        cells = []
        for c in cols:
            v = comp_ms(meths[c], keys, None)
            cells.append("%.3f" % (v / fpf[c] if fpf[c] else 0))
        out.append("| %s | %s |" % (label, " | ".join(cells)))
    # scripts total and physx
    cells_s, cells_px, cells_cb, cells_tot = [], [], [], []
    for c in cols:
        s = slotsd[c].get("FixedUpdate.ScriptRunBehaviourFixedUpdate", 0) / fpf[c]
        px = slotsd[c].get("FixedUpdate.PhysicsFixedUpdate", 0) / fpf[c]
        cb = sum(fnum(r["incl_ms_per_frame"]) for r in meths[c] if re.search(r"\.(OnCollision\w+|OnJointBreak)$", r["name"])) / fpf[c]
        cells_s.append("%.3f" % s)
        cells_cb.append("%.3f" % cb)
        cells_px.append("%.3f" % (px - cb))
        cells_tot.append("%.3f" % (s + px))
    out.append("| **All FixedUpdate scripts (measured slot)** | " + " | ".join(cells_s) + " |")
    out.append("| **PhysX simulate (PhysicsFixedUpdate slot minus callbacks)** | " + " | ".join(cells_px) + " |")
    out.append("| OnCollision*/OnJointBreak callbacks (inside PhysX slot) | " + " | ".join(cells_cb) + " |")
    out.append("| **Physics tick total (scripts + PhysX)** | " + " | ".join(cells_tot) + " |")
    out.append("")
    out.append("### D. Per-frame (Update/LateUpdate/coroutine) physics-adjacent work (ms per rendered frame)\n")
    out.append("| component | " + " | ".join("%s %s" % (t.split("-", 1)[-1], w) for t, w in cols) + " |")
    out.append("|---|" + "---|" * len(cols))
    for label, keys in UPD:
        out.append("| %s | %s |" % (label, " | ".join("%.3f" % comp_ms(meths[c], keys, None) for c in cols)))
    ui = []
    for c in cols:
        ui.append("%.3f" % sum(fnum(r["self_ms_per_frame"]) for r in meths[c] if SRC.get(int(r["id"]), "").startswith("KSP/UI/")))
    out.append("| KSP/UI/* scripts (self time, all callbacks) | " + " | ".join(ui) + " |")
    out.append("")
    # ---- Table E: top self-time methods per run
    for c in cols:
        out.append("### E. Top 20 instrumented methods by self time: %s %s\n" % c)
        out.append("| method | calls/frame | self ms/frame | self % of frame | incl µs/call |")
        out.append("|---|---|---|---|---|")
        rows = sorted(meths[c], key=lambda r: -fnum(r["self_ms_per_frame"]))[:20]
        for r in rows:
            out.append("| %s | %.1f | %.3f | %.1f | %.1f |" % (r["name"], fnum(r["calls_per_frame"]), fnum(r["self_ms_per_frame"]), fnum(r["self_pct_of_frame"]), fnum(r["incl_us_per_call"])))
        out.append("")
    # ---- Table F: per-module-type OnFixedUpdate and module FixedUpdate, ascent only
    out.append("### F. PartModule work per physics tick (ascent; inclusive ms/tick, calls/tick)\n")
    asc = [c for c in cols if c[1] == "ascent"]
    names = set()
    for c in asc:
        for r in meths[c]:
            if r["name"].endswith(".OnFixedUpdate") or is_module_cb(r["name"]):
                names.add(r["name"])
    out.append("| module method | " + " | ".join("%s ms/tick (calls/tick)" % c[0].split("-", 1)[-1] for c in asc) + " |")
    out.append("|---|" + "---|" * len(asc))
    def keyf(n):
        return -max(fnum(next((r["incl_ms_per_frame"] for r in meths[c] if r["name"] == n), 0)) / max(fpf[c], 1e-9) for c in asc)
    for n in sorted(names, key=keyf):
        cells = []
        for c in asc:
            r = next((r for r in meths[c] if r["name"] == n), None)
            if r:
                cells.append("%.4f (%.1f)" % (fnum(r["incl_ms_per_frame"]) / fpf[c], fnum(r["calls_per_frame"]) / fpf[c]))
            else:
                cells.append("-")
        out.append("| %s | %s |" % (n, " | ".join(cells)))
    out.append("")
    # ---- instrumentation overhead
    out.append("### G. Instrumentation overhead estimate\n")
    out.append("| run | instrumented calls/frame | pair cost ns | est. overhead ms/frame | % of frame |")
    out.append("|---|---|---|---|---|")
    for c in cols:
        s = sums[c[0]][c[1]]
        cpf = fnum(s["instrumented_calls_per_frame"])
        ns = fnum(sums[c[0]]["pad"].get("calib_pair_ns", 50))
        ov = cpf * ns / 1e6
        out.append("| %s %s | %.0f | %.1f | %.3f | %.1f%% |" % (c[0], c[1], cpf, ns, ov, 100 * ov / fnum(s["frame_ms_mean"])))
    out.append("")
    # ---- H: floating-origin shift split (needs fo_shifts column)
    import statistics
    out.append("### H. PhysX and FixedUpdate-script cost per tick, split by whether the tick had a floating-origin shift (ascent windows)\n")
    out.append("| run | ticks without shift | PhysX ms/tick | scripts ms/tick | ticks with shift | PhysX ms/tick | scripts ms/tick |")
    out.append("|---|---|---|---|---|---|---|")
    for c in cols:
        if c[1] != "ascent":
            continue
        fr = load_csv(c[0], c[1], "frames")
        if not fr or "fo_shifts" not in fr[0]:
            out.append("| %s | (no fo_shifts column in this build) | | | | | |" % c[0])
            continue
        a = [0, 0.0, 0.0]
        b = [0, 0.0, 0.0]
        for r in fr:
            n = int(r["n_fixed"])
            if n == 0:
                continue
            fo = int(r["fo_shifts"])
            tgt = b if fo >= n else (a if fo == 0 else None)
            if tgt is None:
                continue
            tgt[0] += n
            tgt[1] += fnum(r["physx_ms"])
            tgt[2] += fnum(r["fixed_scripts_ms"])
        f = lambda x: ("%d | %.2f | %.2f" % (x[0], x[1] / x[0], x[2] / x[0])) if x[0] else "0 | - | -"
        out.append("| %s | %s | %s |" % (c[0], f(a), f(b)))
    out.append("")
    out.append("### I. Frames containing a GC collection\n")
    out.append("| run | window | median frame ms | GC frames (frame ms) | managed heap MB (min-max) |")
    out.append("|---|---|---|---|---|")
    for c in cols:
        fr = load_csv(c[0], c[1], "frames")
        if not fr:
            continue
        med = statistics.median(fnum(r["frame_ms"]) for r in fr)
        ev = ["%.0f" % fnum(fr[i]["frame_ms"]) for i in range(1, len(fr)) if int(fr[i]["gc0_count"]) > int(fr[i - 1]["gc0_count"])]
        hp = [int(r["heap_bytes"]) for r in fr]
        out.append("| %s | %s | %.2f | %s | %.0f-%.0f |" % (c[0], c[1], med, ", ".join(ev) if ev else "none", min(hp) / 1e6, max(hp) / 1e6))
    out.append("")
    print("\n".join(out))


if __name__ == "__main__":
    main()
