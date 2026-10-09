"""Write the capture inputs (harness/captures/in/) from the harness golden files.

  python -I harness/captures/tools/prep_inputs.py

Outputs (tab-separated, backslash escapes \\ \t \n \r, floats as IEEE hex bits):
  curves_eval_in.tsv     C <curve_id> <origin> <path> <cfg_keys>  /  S <curve_id> <i> <t hex32>   (golden/curves_eval)
  libm_base_inputs.tsv   <hex64> <source>   kepler e/M/E, state_at_ut UT*meanMotion and eccentricAnomaly (golden/orbit_propagation)
  parse_tokens.tsv       <token>            every numeric token in the curves_eval cfg keys + edge cases
  aero_in.tsv            D <dotNormalized> <mach> / R <case_id> <dir_id> <flow xyz> <mach>   (golden/aero_forces)
The .NET reference tool (tools/netfx) then adds Specials + System.Random(7) values -> libm_inputs.tsv, and f32_inputs.tsv.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hcutil as H  # noqa: E402

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
GOLD = os.path.join(ROOT, "harness", "golden")
OUT = os.path.join(ROOT, "harness", "captures", "in")


def main():
    os.makedirs(OUT, exist_ok=True)
    ce = H.load(os.path.join(GOLD, "curves_eval", "curves_eval.json"), exact=True)
    curves = H.rows_as_dicts(H.table(ce, "curves"))
    samples = H.table(ce, "samples")
    scols = [c["name"] for c in samples["columns"]]
    ci, ii, ti = scols.index("curve_id"), scols.index("i"), scols.index("t")
    n_s = 0
    with open(os.path.join(OUT, "curves_eval_in.tsv"), "w", encoding="utf-8", newline="\n") as f:
        f.write("# from harness/golden/curves_eval/curves_eval.json\n")
        for c in curves:
            f.write("\t".join(["C", H.esc(c["curve_id"]), H.esc(c["origin"]), H.esc(c["path"]), H.esc(c["cfg_keys"] or "")]) + "\n")
        for r in samples["rows"]:
            f.write("S\t%s\t%d\t%s\n" % (H.esc(r[ci]), r[ii], H.hex32(H.exact_f32(r[ti]))))
            n_s += 1
    print("curves_eval_in.tsv: %d curves, %d samples" % (len(curves), n_s))

    # parse tokens: exactly how FloatCurve.Load splits a key line (delimiters ' ' ',' ';' '\t', RemoveEmptyEntries)
    toks, seen = [], set()
    for c in curves:
        raw = c["cfg_keys"] or ""
        if not raw:
            continue
        for key in raw.split(" | "):
            for t in re.split(r"[ ,;\t]", key):
                if t and t not in seen:
                    seen.add(t)
                    toks.append(t)
    edge = ["Infinity", "-Infinity", "NaN", "1e-45", "1.4e-45", "7e-46", "7.1e-46", "3.4028235e38", "3.40282357e38", "3.4028236e38", "1e39", "-0", "0.1",
            "0.3", ".5", "5.", "1,000.5", "1e400", "4.9e-324", "2.4703282292062327e-324", "2.2250738585072011e-308", "0.6960422", "1.17549435E-38",
            "16777217", "9007199254740993", "123456789012345678901234567890", "0.30000000000000004", "1.0000000000000002", "  7 ", "+3", "1E+2", "0x10",
            "1.00000017881393432617187499", "1.000000178813934326171875", "1.00000017881393432617187501", "0.000000000000000000000000000000000000000000001401298464324817070923729583289916131280"]
    for t in edge:
        if t not in seen:
            seen.add(t)
            toks.append(t)
    with open(os.path.join(OUT, "parse_tokens.tsv"), "w", encoding="utf-8", newline="\n") as f:
        f.write("# numeric tokens of curves_eval cfg keys (FloatCurve.Load split) + edge cases\n")
        for t in toks:
            f.write(H.esc(t) + "\n")
    print("parse_tokens.tsv: %d tokens" % len(toks))

    ae = H.load(os.path.join(GOLD, "aero_forces", "aero_forces.json"), exact=True)
    nd = nr = 0
    with open(os.path.join(OUT, "aero_in.tsv"), "w", encoding="utf-8", newline="\n") as f:
        f.write("# from harness/golden/aero_forces: D <dotNormalized> <mach> (drag_curve_value) / R <case_id> <dir_id> <flow_x> <flow_y> <flow_z> <mach> (cube_drag), float32 hex\n")
        for r in H.rows_as_dicts(H.table(ae, "drag_curve_value")):
            f.write("D\t%s\t%s\n" % (H.hex32(H.exact_f32(r["dotNormalized"])), H.hex32(H.exact_f32(r["mach"]))))
            nd += 1
        for r in H.rows_as_dicts(H.table(ae, "cube_drag")):
            f.write("R\t%s\t%s\t%s\t%s\t%s\t%s\n" % (H.esc(r["case_id"]), H.esc(r["dir_id"]), H.hex32(H.exact_f32(r["flow_x"])), H.hex32(H.exact_f32(r["flow_y"])),
                                                H.hex32(H.exact_f32(r["flow_z"])), H.hex32(H.exact_f32(r["mach"]))))
            nr += 1
    print("aero_in.tsv: %d drag_curve_value, %d cube_drag rows" % (nd, nr))

    op = H.load(os.path.join(GOLD, "orbit_propagation", "orbit_propagation.json"))
    vals, srcs, seenb = [], [], set()

    def add(x, src):
        b = H.f64_bits(x)
        if b in seenb:
            return
        seenb.add(b)
        vals.append(x)
        srcs.append(src)

    for r in H.rows_as_dicts(H.table(op, "kepler")):
        for k in ("e", "M", "E"):
            add(H.num64(r[k]), "kepler.%s" % k)
    mm = {r["case_id"]: H.num64(r["meanMotion"]) for r in H.rows_as_dicts(H.table(op, "cases"))}
    st = H.rows_as_dicts(H.table(op, "state_at_ut"))
    for r in st:
        add(H.num64(r["UT"]) * mm[r["case_id"]], "UT*meanMotion:%s@%s" % (r["case_id"], r["UT"]))
    for r in st:
        add(H.num64(r["eccentricAnomaly"]), "state_at_ut.eccentricAnomaly:%s@%s" % (r["case_id"], r["UT"]))
    with open(os.path.join(OUT, "libm_base_inputs.tsv"), "w", encoding="utf-8", newline="\n") as f:
        f.write("# doubles from harness/golden/orbit_propagation (kepler e/M/E, state_at_ut UT*meanMotion, eccentricAnomaly), unique by bits\n")
        for x, s in zip(vals, srcs):
            f.write("%s\t%s\n" % (H.hex64(x), H.esc(s)))
    print("libm_base_inputs.tsv: %d values" % len(vals))


if __name__ == "__main__":
    main()
