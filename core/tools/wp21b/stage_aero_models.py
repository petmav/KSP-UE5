"""WP-21b: stage the reduced .mu models of the parts of the stock aircraft (control surfaces, wings, intakes, engines, fuselages, gear bays).

For every part name used by the listed SPH crafts (harness/inputs/Ships/SPH/<craft>.craft) this finds the part .cfg under the dev copy's GameData
(decompiled/KSP-dev), takes the .mu model(s) the cfg compiles (MODEL nodes, else `mesh =`, else the first .mu of the directory: the rule of
core/tools/wp29d/stage_wheel_models.py), reduces each with core/tools/wp29d/reduce_mu.py (transform tree + tag/layer + collider chunks + wheel
collider + animation chunk, no meshes / materials / textures) and writes core/data/aero_models/<part>__<model>.mu together with MANIFEST.json
in the format core/src/physics/wheel_models.cpp (WheelModelLibrary) reads: origin path and sha256 of every original, sha256 of every reduced file
and PartLoader.CompileModel's view of the cfg.

Local-only: needs decompiled/KSP-dev. Run from the repo root:
    python -I core/tools/wp21b/stage_aero_models.py [--game decompiled/KSP-dev] [craft names...]
"""
import argparse
import hashlib
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "core" / "tools" / "wp29d"))
import reduce_mu  # noqa: E402
import stage_wheel_models as sw  # noqa: E402

DEFAULT_CRAFTS = ["Aeris 3A", "Aeris 4A", "Ravenspear Mk1", "Stearwing A300"]


def craft_parts(name):
    t = (ROOT / "harness" / "inputs" / "Ships" / "SPH" / (name + ".craft")).read_text(encoding="utf-8-sig", errors="replace")
    return {p.rsplit("_", 1)[0] for p in re.findall(r"(?m)^\s*part = (\S+)", t)}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--game", default=str(ROOT / "decompiled" / "KSP-dev"))
    ap.add_argument("crafts", nargs="*", default=DEFAULT_CRAFTS)
    args = ap.parse_args()
    game = pathlib.Path(args.game)
    gd = game / "GameData"
    wanted = set()
    for c in args.crafts:
        wanted |= craft_parts(c)
    found = {}
    for cfg in sorted(gd.rglob("*.cfg")):
        if "zDeprecated" in cfg.parts:
            continue
        text = cfg.read_text(encoding="utf-8-sig", errors="replace")
        if not re.search(r"^\s*PART\b", text, re.M):
            continue
        m = re.search(r"(?m)^\s*name\s*=\s*(\S+)", text)
        if not m:
            continue
        name = m.group(1).replace("_", ".")  # PartLoader.cs: '_' -> '.'
        if name not in wanted or name in found:
            continue
        found[name] = (cfg, re.findall(r"(?m)^\s*model\s*=\s*(\S+)", text), re.findall(r"(?m)^\s*mesh\s*=\s*(\S+)", text))
    missing = sorted(wanted - set(found))
    out_dir = ROOT / "core" / "data" / "aero_models"
    out_dir.mkdir(parents=True, exist_ok=True)
    red_manifest = {}
    for name, (cfg, models, meshes) in sorted(found.items()):
        if models:
            files = [cfg] + [gd / (mo + ".mu") for mo in models]
        elif meshes:
            files = [cfg] + [cfg.parent / (me if me.endswith(".mu") else me + ".mu") for me in meshes]
        else:
            files = [cfg] + sorted(cfg.parent.glob("*.mu"))[:1]
        records, reduced = [], []
        for f in files:
            if not f.exists():
                print("missing file", name, f)
                continue
            rec = {"file": f.name, "origin": f.relative_to(game).as_posix(), "sha256": hashlib.sha256(f.read_bytes()).hexdigest()}
            records.append(rec)
            if f.suffix == ".mu":
                out = reduce_mu.reduce(f.read_bytes())
                rname = "%s__%s" % (name, f.name)
                (out_dir / rname).write_bytes(out)
                reduced.append({"file": rname, "from": rec["file"], "origin": rec["origin"], "originalSha256": rec["sha256"],
                                "sha256": hashlib.sha256(out).hexdigest(), "bytes": len(out)})
        if not reduced:
            print("no model for", name)
            continue
        red_manifest[name] = {"cfg": records[0]["origin"], "models": reduced, "instances": sw.instances_of(cfg, reduced)}
    (out_dir / "MANIFEST.json").write_text(json.dumps(
        {"note": "reduced copies (transform tree, tag/layer, collider + wheel collider chunks, animation chunk; no meshes/renderers/"
                 "materials/textures) of the original game files named in 'origin' (decompiled/KSP-dev), made by core/tools/wp21b/"
                 "stage_aero_models.py with core/tools/wp29d/reduce_mu.py for the parts of: " + ", ".join(args.crafts),
         "parts": red_manifest}, indent=1) + "\n", encoding="utf-8")
    print("staged", len(red_manifest), "parts; no cfg found for:", missing)
    return 0


if __name__ == "__main__":
    sys.exit(main())
