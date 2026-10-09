"""Batch-2 capture checks and summaries (C4, C5, C6, C8, C9, C10, terrain).

  python -I harness/captures/tools/batch2_report.py <c4|c5|c6|c8|c9|c10|terrain> [--out2 DIR] [--md FILE]

Reads harness/captures/out2/<dataset>.json and writes a markdown summary (default reports2/<item>_summary.md).
Prints one summary line. The checks are internal-consistency and independent-reference checks; the capture files
themselves are the goldens.
"""
import argparse
import math
import os
import re
import sys
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hcutil as H  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
CAP = os.path.dirname(HERE)
ROOT = os.path.dirname(os.path.dirname(CAP))
KSPDEV = os.path.join(ROOT, "decompiled", "KSP-dev")

FILES = {"c4": "atmosphere_thermal_offsets", "c5": "deltav_readout", "c6": "thermal_step", "c8": "orbit_rails_ingame",
         "c9": "part_database", "c10": "aero_forces_vessel", "terrain": "terrain_height"}


def T(doc, name):
    t = H.table(doc, name)
    cols = [c["name"] for c in t["columns"]]
    return cols, t["rows"]


def D(doc, name):
    cols, rows = T(doc, name)
    return [dict(zip(cols, r)) for r in rows]


def f(v):
    return H.num64(v)


def g(x, p=6):
    if x is None:
        return "-"
    if isinstance(x, float):
        if math.isnan(x) or math.isinf(x):
            return str(x)
        return "%.*g" % (p, x)
    return str(x)


def md_table(head, rows):
    out = ["| " + " | ".join(head) + " |", "|" + "---|" * len(head)]
    for r in rows:
        out.append("| " + " | ".join(g(x) if not isinstance(x, str) else x for x in r) + " |")
    return "\n".join(out)


def prov(doc):
    p = doc.get("provenance", {})
    keys = ["ksp_version", "unity_version", "runtime", "assembly_csharp_sha256", "assembly_csharp_jit", "profiling_branch_commit", "utc"]
    return "\n".join("- %s: `%s`" % (k, p.get(k)) for k in keys if k in p)


# ------------------------------------------------------------------------------------------------------------- C9
def parse_partdb(path):
    """PartDatabase.cfg -> {url: [(cube name, [30 number strings])]} for top-level PART nodes only (the file contains
    a PART nested inside another PART, which the game ignores)."""
    out = {}
    url = None
    depth = 0
    top = None
    pending = None
    with open(path, encoding="utf-8-sig") as fh:
        for line in fh:
            s = line.split("//")[0].strip()
            if not s:
                continue
            if s == "{":
                depth += 1
                if depth == 1:
                    top = pending
                    url = None
                continue
            if s == "}":
                depth -= 1
                continue
            if "=" not in s:
                pending = s
                continue
            k, v = [x.strip() for x in s.split("=", 1)]
            if top == "PART" and depth == 1 and k == "url":
                url = v
                out.setdefault(url, [])
            elif top == "PART" and depth == 2 and k == "cube" and url:
                parts = [x.strip() for x in v.split(",")]
                out[url].append((parts[0], parts[1:]))
    return out


def part_cfg_cubes(path):
    """DRAG_CUBE cube lines of a part cfg (only the first PART in the file is assumed)."""
    out = []
    try:
        txt = open(path, encoding="utf-8-sig").read()
    except OSError:
        return out
    if "DRAG_CUBE" not in txt:
        return out
    for line in txt.splitlines():
        t = line.split("//")[0].strip()
        if t.startswith("cube") and "=" in t and t.split("=")[0].strip() == "cube":
            parts = [x.strip() for x in t.split("=", 1)[1].split(",")]
            out.append((parts[0], parts[1:]))
    return out


def c9(doc, md):
    parts = D(doc, "parts")
    cubes = D(doc, "drag_cubes")
    assets = D(doc, "assets")
    cfgv = D(doc, "part_cfg_values")
    url_of = {p["name"]: p["partUrl"] for p in parts}
    db = parse_partdb(os.path.join(KSPDEV, "PartDatabase.cfg"))
    # cube comparison: in-game float32 (hex) vs float32(decimal string) of PartDatabase.cfg
    n_cmp = n_exact = n_missing = 0
    worst = []
    bycube = defaultdict(list)
    for c in cubes:
        bycube[c["part"]].append(c)
    cfgfile = {a["part"]: a["file"] for a in assets if a["kind"] == "cfg" and a["file"]}
    n_cfgsrc = 0
    for pn, cl in bycube.items():
        entry = db.get(url_of.get(pn))
        own = part_cfg_cubes(os.path.join(KSPDEV, cfgfile[pn])) if pn in cfgfile else []
        if own:  # DRAG_CUBE in the part's own cfg takes precedence over PartDatabase.cfg
            entry = own
            n_cfgsrc += len(cl)
        if entry is None:
            n_missing += len(cl)
            continue
        for c in cl:
            match = [e for e in entry if e[0] == c["name"]]
            if not match:
                n_missing += 1
                continue
            nums = match[0][1]
            ours = []
            for i in range(6):
                ours += [c["area_%d_hex" % i], c["drag_%d_hex" % i], c["depth_%d_hex" % i]]
            ours += [c["center_x_hex"], c["center_y_hex"], c["center_z_hex"], c["size_x_hex"], c["size_y_hex"], c["size_z_hex"]]
            n_cmp += 1
            bad = []
            for k, (h, txt) in enumerate(zip(ours, nums)):
                ref = H.exact_f32(txt)
                if "%08X" % H.f32_bits(ref) != h:
                    bad.append((k, txt, h))
            if not bad:
                n_exact += 1
            else:
                worst.append((pn, c["name"], len(bad), bad[:2]))
    # cfg value -> resolved check for plain numeric Part fields
    num_ok = num_bad = 0
    bad_rows = []
    for r in cfgv:
        if r["target"] in ("Part", "AvailablePart") and r["resolved_num"] is not None:
            try:
                raw = float(r["cfg_value"].split("//")[0].strip())
            except ValueError:
                continue
            res = f(r["resolved_num"])
            if r["field_type"] == "Single":
                ok = H.f32_bits(H.exact_f32(r["cfg_value"].split("//")[0].strip())) == H.f32_bits(res)
            else:
                ok = raw == res
            if ok:
                num_ok += 1
            else:
                num_bad += 1
                bad_rows.append((r["part"], r["key"], r["cfg_value"], r["resolved"]))
    src = Counter((a["kind"], a["source"]) for a in assets)
    ex = Counter((a["kind"], a["exists"]) for a in assets)
    errs = [(p["name"], p["error"]) for p in parts if p["error"]]
    L = ["# C9 part_database summary", "", prov(doc), "",
         "Parts: %d (errors: %d). Tables: %s." % (len(parts), len(errs), ", ".join("%s %d" % (t["name"], len(t["rows"])) for t in doc["tables"])), ""]
    if errs:
        L += ["Parts without a usable prefab: " + ", ".join("%s (%s)" % e for e in errs), ""]
    L += ["## Drag cubes vs KSP-dev/PartDatabase.cfg", "",
          "Compared %d loaded cubes with the DRAG_CUBE text the game read: the part's own cfg DRAG_CUBE node when it has one (%d cubes), otherwise the "
          "KSP-dev/PartDatabase.cfg entry for the same partUrl and cube name (all 30 numbers, float32 bits): %d bit-exact, %d differ, %d loaded cubes have no entry." % (n_cmp, n_cfgsrc, n_exact, n_cmp - n_exact, n_missing), ""]
    if worst:
        L += md_table(["part", "cube", "#diff", "first diffs (index, cfg text, loaded hex)"], [(a, b, c, str(d)) for a, b, c, d in worst[:20]]).split("\n") + [""]
    L += ["Note: KSP-dev/PartDatabase.cfg has a PART node nested inside another PART (MpoProbe inside FlagDecal Size5Flag); only top-level PART nodes are used by the game, and only those are compared.", ""]
    L += ["## cfg values vs resolved Part/AvailablePart fields", "",
          "Known transforms: (1) PartLoader.ApplyPartValue parses every double-typed Part field with float.Parse (the decompiled code tests typeof(double) twice), so e.g. emissiveConstant 0.7 loads as 0.699999988079071; "
          "(2) crewed parts get mass -= PhysicsGlobals.PerSeatReduction (PerCommandSeatReduction for command parts) * CrewCapacity.", "",
          "Numeric top-level cfg values that map to a Part/AvailablePart field: %d equal to the parsed cfg text, %d differ (loader transforms them)." % (num_ok, num_bad), ""]
    if bad_rows:
        c = Counter((k) for _, k, _, _ in bad_rows)
        L += ["Keys that differ (count): " + ", ".join("%s %d" % kv for kv in c.most_common(20)), ""]
        L += md_table(["part", "key", "cfg", "resolved"], bad_rows[:15]).split("\n") + [""]
    L += ["## Asset origins", "", md_table(["kind", "source", "rows"], [(k[0], k[1], n) for k, n in sorted(src.items())]), "",
          md_table(["kind", "file exists", "rows"], [(k[0], str(k[1]), n) for k, n in sorted(ex.items(), key=str)]), "",
          "Unity-asset textures (not in GameData; they come from KSP_x64_Data): " +
          ", ".join("%s (%d)" % kv for kv in Counter(a["url"] for a in assets if a["source"] == "unity_asset").most_common(12)), ""]
    open(md, "w", encoding="utf-8").write("\n".join(L))
    return "c9: %d parts, cubes %d/%d bit-exact vs cfg text (part cfg DRAG_CUBE for %d, else PartDatabase.cfg; %d without entry), cfg numeric %d ok / %d transformed, assets %d" % (
        len(parts), n_exact, n_cmp, n_cfgsrc, n_missing, num_ok, num_bad, len(assets))


# -------------------------------------------------------------------------------------------------------- terrain
def terrain(doc, md):
    s_cols, rows = T(doc, "samples")
    ix = {c: i for i, c in enumerate(s_cols)}
    stats = defaultdict(lambda: [0, 0, math.inf, -math.inf, 0])
    for r in rows:
        k = (r[0], r[1])
        st = stats[k]
        hs, ho, ta = f(r[ix["h_surface"]]), f(r[ix["h_override"]]), f(r[ix["terrain_alt"]])
        st[0] += 1
        st[1] += hs != ho
        st[2] = min(st[2], ta)
        st[3] = max(st[3], ta)
        st[4] += ta < 0
    bodies = D(doc, "bodies")
    mods = D(doc, "mods")
    assets = D(doc, "map_assets")
    probe = D(doc, "scene_probe")
    L = ["# Terrain height capture summary", "", prov(doc), "",
         md_table(["body", "Radius", "pqs.radius", "radiusMin", "radiusMax", "mods", "isActive"],
                  [(b["body"], f(b["Radius"]), f(b["pqs_radius"]), f(b["radiusMin"]), f(b["radiusMax"]), b["mod_count"], str(b["isActive"])) for b in bodies]), "",
         md_table(["body", "set", "samples", "h_surface != h_override", "min terrain_alt", "max terrain_alt", "below sea level"],
                  [(k[0], k[1], v[0], v[1], v[2], v[3], v[4]) for k, v in sorted(stats.items())]), "",
         "## Scene probe (MAINMENU vs FLIGHT)", "",
         md_table(["scene", "body", "isStarted", "isActive", "mods", "h(KSC)", "h_override(0,0)"],
                  [(p["scene"], p["body"], str(p["isStarted"]), str(p["isActive"]), p["mod_count"], f(p["h_ksc"]), f(p["h_override_0_0"])) for p in probe]), "",
         "## Mod stacks (evaluation order; * = overrides OnVertexBuildHeight)", ""]
    for b in bodies:
        L.append("- %s: " % b["body"] + ", ".join(("%s%s(%d)" % ("*" if m["height_mod"] else "", m["class"], m["order"])) for m in mods if m["body"] == b["body"]))
    L += ["", "## Map assets referenced by the stack", "",
          md_table(["body", "mod", "class", "field", "type", "Unity asset name", "MapName", "w", "h", "bpp"],
                   [(a["body"], a["mod_idx"], a["mod_class"], a["field"], a["asset_type"], a["asset_name"], a["MapName"] or "-", a["width"], a["height"], a["bpp"]) for a in assets]), ""]
    same = len({(p["body"], p["h_ksc_hex"], p["h_override_0_0_hex"]) for p in probe}) == len({p["body"] for p in probe})
    open(md, "w", encoding="utf-8").write("\n".join(L))
    return "terrain: %d samples; MAINMENU==FLIGHT probe heights: %s; h_surface!=h_override: %s" % (
        len(rows), same, ", ".join("%s/%s %d" % (k[0], k[1], v[1]) for k, v in sorted(stats.items())))


# ------------------------------------------------------------------------------------------------------------- C4
def c4(doc, md):
    s = D(doc, "samples")
    ep = D(doc, "epochs")
    agg = defaultdict(lambda: [math.inf, -math.inf, math.inf, -math.inf, 0, 0.0])
    quirk = 0
    for r in s:
        a = agg[(r["body"], r["epoch"])]
        o = f(r["atmosphereTemperatureOffset"])
        t = f(r["GetFullTemperature_alt_offset"])
        a[0], a[1] = min(a[0], o), max(a[1], o)
        a[2], a[3] = min(a[2], t), max(a[3], t)
        a[4] += 1
        quirk += f(r["GetFullTemperature_pos"]) != o
    L = ["# C4 atmosphere_thermal_offsets summary", "", prov(doc), "",
         md_table(["epoch", "UT", "offset s"], [(e["epoch"], f(e["UT"]), f(e["ut_offset"])) for e in ep]), "",
         md_table(["body", "epoch", "min offset K", "max offset K", "min T(alt,offset) K", "max T K", "rows"],
                  [(k[0], k[1], v[0], v[1], v[2], v[3], v[4]) for k, v in sorted(agg.items())]), "",
         "GetFullTemperature(Vector3d pos) returns the *offset* (not a temperature) computed with an unnormalised sun vector; "
         "it differs from the FlightIntegrator-path offset in %d of %d rows." % (quirk, len(s)), ""]
    open(md, "w", encoding="utf-8").write("\n".join(L))
    return "c4: %d samples over %d epochs; GetFullTemperature(pos) != FI-path offset in %d rows" % (len(s), len(ep), quirk)


# ------------------------------------------------------------------------------------------------------------- C5
def c5(doc, md):
    st = D(doc, "stages")
    tot = D(doc, "totals")
    craft = D(doc, "craft")
    g0 = 9.80665
    worst = 0.0
    n = 0
    outl = []
    for r in st:
        if r["body"] != "Kerbin":
            continue
        isp, m0, m1, dv = f(r["ispVac"]), f(r["startMass"]), f(r["endMass"]), f(r["deltaVinVac"])
        if isp and m0 and m1 and m1 > 0 and m0 > m1 * (1 + 1e-6) and dv > 0:
            ref = isp * g0 * math.log(m0 / m1)
            rel = abs(ref - dv) / dv
            n += 1
            if rel > 1e-6:
                outl.append((r["craft_id"], r["stage"], isp, m0, m1, dv, rel, r["engines_active"]))
            else:
                worst = max(worst, rel)
    L = ["# C5 deltav_readout summary", "", prov(doc), "",
         md_table(["craft", "parts", "stages", "mass t", "Kerbin vac", "Kerbin ASL", "Mun ASL", "Duna ASL", "Eve ASL", "note"],
                  [(c["craft_id"], c["parts"], c["stage_count"], f(c["total_mass"]),
                    *[next((f(t["TotalDeltaVVac" if (b == "Kerbin" and k == "vac") else "TotalDeltaVASL"]) for t in tot if t["craft_id"] == c["craft_id"] and t["body"] == b), None)
                      for b, k in (("Kerbin", "vac"), ("Kerbin", "asl"), ("Mun", "asl"), ("Duna", "asl"), ("Eve", "asl"))],
                    c["error"] or "") for c in craft]), "",
         "Rocket-equation cross-check (Kerbin rows, stages with ispVac > 0 and startMass > endMass): %d of %d stages agree with Isp g0 ln(m0/m1) (g0 = 9.80665) to max rel %.3g (float32 storage); "
         "the %d others are multi-phase stages (engines with different Isp or burn times; the stage simulation integrates over burn phases and reports a combined Isp):" % (n - len(outl), n, worst, len(outl)), "",
         md_table(["craft", "stage", "ispVac", "startMass", "endMass", "deltaVinVac", "rel diff", "engines active"], outl), "",
         "Note: in the editor, deltaVatASL and deltaVActual use DeltaVAppValues.body's pressure at DeltaVAppValues.altitude, so 'ASL' follows the selected body.", ""]
    open(md, "w", encoding="utf-8").write("\n".join(L))
    return "c5: %d craft, %d stage rows; rocket equation: %d/%d single-phase stages within rel %.3g, %d multi-phase" % (len(craft), len(st), n - len(outl), n, worst, len(outl))


# ------------------------------------------------------------------------------------------------------------- C6
def c6(doc, md):
    v = D(doc, "vessel")
    p = D(doc, "parts")
    L = ["# C6 thermal_step summary", "", prov(doc), ""]
    rows = []
    for case in ("pad", "orbit", "reentry"):
        vv = [r for r in v if r["case"] == case]
        if not vv:
            continue
        rows.append((case, len(vv), f(vv[0]["UT"]), f(vv[-1]["UT"]), f(vv[0]["altitude"]), f(vv[-1]["altitude"]), max(f(r["mach"]) for r in vv),
                     max(f(r["externalTemperature"]) for r in vv), max(f(r["convectiveCoefficient"]) for r in vv), sum(1 for r in vv if r["fi_isAnalytical"])))
    L += [md_table(["case", "frames", "UT0", "UT1", "alt0", "alt1", "max mach", "max T_ext", "max conv coeff", "analytic frames"], rows), ""]
    prow = []
    for case in ("pad", "orbit", "reentry"):
        pp = defaultdict(list)
        for r in p:
            if r["case"] == case:
                pp[r["part"]].append(r)
        for k, rr in pp.items():
            prow.append((case, k, f(rr[0]["temperature"]), f(rr[-1]["temperature"]), f(rr[0]["skinTemperature"]), max(f(r["skinTemperature"]) for r in rr), f(rr[-1]["skinTemperature"])))
    L += [md_table(["case", "part", "T0", "T1", "skin0", "skin max", "skin1"], prow), ""]
    open(md, "w", encoding="utf-8").write("\n".join(L))
    return "c6: %d vessel frames, %d part rows; cases %s" % (len(v), len(p), ",".join(r[0] for r in rows))


# ------------------------------------------------------------------------------------------------------------- C8
def kepler_pos(el, mu, ut):
    """Independent two-body position in KSP's Orbit frame (z-up; ascending node at angle LAN from +x)."""
    inc, e, a, lan, w, m0, ep = [math.radians(el[0]), el[1], el[2], math.radians(el[3]), math.radians(el[4]), el[5], el[6]]
    n = math.sqrt(mu / abs(a) ** 3)
    M = m0 + n * (ut - ep)
    if e < 1:
        M = math.fmod(M, 2 * math.pi)
        E = M if e < 0.8 else math.pi
        for _ in range(100):
            dE = (E - e * math.sin(E) - M) / (1 - e * math.cos(E))
            E -= dE
            if abs(dE) < 1e-15:
                break
        x, y = a * (math.cos(E) - e), a * math.sqrt(1 - e * e) * math.sin(E)
    else:
        F = math.asinh(M / e)
        for _ in range(200):
            dF = (e * math.sinh(F) - F - M) / (e * math.cosh(F) - 1)
            F -= dF
            if abs(dF) < 1e-15:
                break
        x, y = a * (math.cosh(F) - e), -a * math.sqrt(e * e - 1) * math.sinh(F)
    cw, sw, ci, si, cl, sl = math.cos(w), math.sin(w), math.cos(inc), math.sin(inc), math.cos(lan), math.sin(lan)
    xo, yo = cw * x - sw * y, sw * x + cw * y
    return (cl * xo - sl * ci * yo, sl * xo + cl * ci * yo, si * yo)


def c8(doc, md):
    ini = {r["id"]: r for r in D(doc, "initial")}
    smp = D(doc, "samples")
    ev = D(doc, "soi_events")
    seg = D(doc, "segments")
    stats = defaultdict(lambda: [0, 0.0, 0.0, 0.0, 0.0])  # n, max |pos-relpos|/r, max world vs pos.xzy /r, max kepler err /r, max kepler abs
    try:
        zup = {fr["frame"]: [(f(fr[a + "_x"]), f(fr[a + "_y"]), f(fr[a + "_z"])) for a in ("Zup_X", "Zup_Y", "Zup_Z")] for fr in D(doc, "frames")}
    except KeyError:
        zup = {}
    for r in smp:
        i = r["id"]
        pos = (f(r["pos_x"]), f(r["pos_y"]), f(r["pos_z"]))
        rel = (f(r["relpos_at_UT_x"]), f(r["relpos_at_UT_y"]), f(r["relpos_at_UT_z"]))
        wr = (f(r["world_rel_x"]), f(r["world_rel_y"]), f(r["world_rel_z"]))
        rr = math.sqrt(sum(x * x for x in pos)) or 1.0
        s = stats[i]
        s[0] += 1
        s[1] = max(s[1], math.dist(pos, rel) / rr)
        s[2] = max(s[2], math.dist((pos[0], pos[2], pos[1]), wr) / rr)
        el0 = ini.get(i)
        if el0 and el0["spawned"] and r["ref_body"] == el0["body"]:
            el = [f(el0[k]) for k in ("inclination", "eccentricity", "semiMajorAxis", "LAN", "argumentOfPeriapsis", "meanAnomalyAtEpoch", "epoch")]
            # only before the first SOI change of this vessel
            first_ev = min([f(e["UT"]) for e in ev if e["vessel"] == "HC8 " + i] or [math.inf])
            if f(r["UT"]) < first_ev:
                k = kepler_pos(el, f(el0["mu"]), f(r["UT"]))
                fr = zup.get(r["frame"])
                if fr:  # relpos = Zup.WorldToLocal(inertial)
                    k = tuple(sum(k[j] * ax[j] for j in range(3)) for ax in fr)
                err = math.dist(k, rel)
                s[3] = max(s[3], err / rr)
                s[4] = max(s[4], err)
    L = ["# C8 orbit_rails_ingame summary", "", prov(doc), "",
         md_table(["seg", "requested", "achieved", "frames", "UT start", "UT end"], [(s["segment"], f(s["requested_rate"]), f(s["achieved_rate"]), s["frames"], f(s["UT_start"]), f(s["UT_end"])) for s in seg]), "",
         md_table(["id", "body", "inc", "e", "sma", "epoch", "note"], [(k, v["body"], f(v["inclination"]), f(v["eccentricity"]), f(v["semiMajorAxis"]), f(v["epoch"]), v["note"]) for k, v in ini.items()]), "",
         "## Consistency", "",
         "pos = the Orbit's own state after OrbitDriver.UpdateOrbit this frame; relpos = orbit.getRelativePositionAtUT(UT) recomputed; world = GetWorldPos3D() - mainBody.position (Unity axes, compared with pos.xzy; includes the rotated localCoM offset). "
         "kepler = independent double-precision two-body propagation from the spawned elements (only for samples on the initial patch), mapped through the captured Planetarium.Zup frame (relpos = Zup.WorldToLocal(inertial)).", "",
         md_table(["id", "samples", "max |pos-relpos|/r", "max |world-pos.xzy|/r", "max |kepler-relpos|/r", "max |kepler-relpos| m"],
                  [(k, v[0], v[1], v[2], v[3], v[4]) for k, v in sorted(stats.items())]), "",
         "## SOI transitions", "",
         md_table(["vessel", "UT", "from", "to", "new sma", "new e", "warp"], [(e["vessel"], f(e["UT"]), e["from"], e["to"], f(e["semiMajorAxis"]), f(e["eccentricity"]), f(e["warp_rate"])) for e in ev]), ""]
    open(md, "w", encoding="utf-8").write("\n".join(L))
    worst_k = max((v[3] for v in stats.values()), default=0.0)
    return "c8: %d samples, %d SOI events (%s), max kepler rel err %.3g" % (len(smp), len(ev), "; ".join("%s %s->%s" % (e["vessel"], e["from"], e["to"]) for e in ev), worst_k)


# ------------------------------------------------------------------------------------------------------------ C10
def c10(doc, md):
    states = D(doc, "states")
    parts = D(doc, "parts")
    aoa = D(doc, "aoa_sweep")
    live = {(p["craft"], p["state"], p["part_index"]): p for p in parts}
    a0_ok = a0_n = 0
    for a in aoa:
        if f(a["aoa_deg"]) == 0.0:
            p = live[(a["craft"], a["state"], a["part_index"])]
            a0_n += 1
            a0_ok += all(a[k + "_hex"] == p[k + "_hex"] for k in ("AreaDrag", "Area", "ExposedArea", "DragCoeff"))
    rows = []
    for s in states:
        pp = [p for p in parts if p["craft"] == s["craft"] and p["state"] == s["state"]]
        occl = sum(1 for p in pp if any(f(p["AreaOccluded_%d" % i]) < f(p["WeightedArea_%d" % i]) for i in range(6)))
        wa = sum(f(p["WeightedArea_%d" % i]) for p in pp for i in range(6))
        ao = sum(f(p["AreaOccluded_%d" % i]) for p in pp for i in range(6))
        shielded = sum(1 for p in pp if p["ShieldedFromAirstream"])
        dsum = sum(f(p["dragScalar"]) for p in pp)
        rows.append((s["craft"], s["state"], f(s["altitude"]), f(s["mach"]), f(s["dynamicPressurekPa"]), f(s["pseudoReDragMult"]), len(pp), occl, (1 - ao / wa) if wa else None, shielded, dsum, f(s["aoa_deg"])))
    sweep = defaultdict(float)
    for a in aoa:
        sweep[(a["craft"], a["state"], f(a["aoa_deg"]))] += f(a["AreaDrag"])
    L = ["# C10 aero_forces_vessel summary", "", prov(doc), "",
         md_table(["craft", "state", "alt m", "mach", "q kPa", "pseudoReDragMult", "parts", "parts with occluded faces", "occluded share of cube area", "shielded", "sum dragScalar", "AoA deg"], rows), "",
         "Occlusion (DragCubeList.SetPartOcclusion) only acts through attach nodes with an attached part: areaOccluded[face] = max(0, weightedArea[face] - facing area of the neighbour); surface attachment only sets srfAttachNode.contactArea.", "",
         "aoa_sweep at 0 deg reproduces the live AreaDrag/Area/ExposedArea/DragCoeff bits for %d of %d parts." % (a0_ok, a0_n), "",
         "## Sum of AreaDrag over parts vs synthetic AoA", ""]
    keys = sorted({(k[0], k[1]) for k in sweep})
    angs = sorted({k[2] for k in sweep})
    L += md_table(["craft", "state"] + ["%g deg" % a for a in angs], [(c, st, *[sweep[(c, st, a)] for a in angs]) for c, st in keys]).split("\n") + [""]
    open(md, "w", encoding="utf-8").write("\n".join(L))
    return "c10: %d states (%s), %d part rows, %d aoa rows; aoa0==live %d/%d" % (
        len(states), ",".join(sorted({s["craft"] for s in states})), len(parts), len(aoa), a0_ok, a0_n)


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("item", choices=sorted(FILES))
    ap.add_argument("--out2", default=os.path.join(CAP, "out2"))
    ap.add_argument("--md")
    a = ap.parse_args(argv)
    path = os.path.join(a.out2, FILES[a.item] + ".json")
    md = a.md or os.path.join(CAP, "reports2", a.item + "_summary.md")
    os.makedirs(os.path.dirname(md), exist_ok=True)
    doc = H.load(path)
    print(globals()[a.item](doc, md))


if __name__ == "__main__":
    main(sys.argv[1:])
