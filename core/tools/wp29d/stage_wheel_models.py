"""WP-29d: stage the .mu models of every stock wheel / landing gear / landing leg part from the dev copy.

Copies the part .cfg and the .mu model(s) it references from decompiled/KSP-dev/GameData into core/data/mu_staging_wheels/<partName>/
(gitignored: original game assets are never committed) and writes MANIFEST.json with the origin path and sha256 of every file
(CLAUDE.md: record each asset's origin next to where it is used). Then writes the REDUCED models that ARE committed
(core/data/wheel_models/<partName>.mu: transform tree + TagAndLayer + collider / WheelCollider chunks + Animation chunk, every mesh,
renderer, material and texture removed) together with core/data/wheel_models/MANIFEST.json (origin and sha256 of each original, of each
reduced file and the part name -> reduced file map). The reducer is core/tools/wp29d/reduce_mu.py.

Local-only: needs decompiled/KSP-dev. Run from the repo root:
    python -I core/tools/wp29d/stage_wheel_models.py [--game decompiled/KSP-dev]
"""
import argparse
import hashlib
import json
import pathlib
import re
import shutil
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import reduce_mu  # noqa: E402

WHEEL_MODULES = ("ModuleWheelBase", "ModuleWheelDeployment")


def instances_of(cfg, reduced):
    """PartLoader.CompileModel's view of the cfg (PartLoader.cs:1765): one instance per MODEL node (model, parent, name, scale,
    position, rotation), else the directory model. `file` names the reduced .mu in the same order as the files staged."""
    text = chr(10).join(l.split("//")[0] for l in cfg.read_text(encoding="utf-8-sig", errors="replace").splitlines())
    out = []
    blocks = re.findall(r"(?ms)^\s*MODEL\s*\{(.*?)\}", text)
    if not blocks:
        return [{"file": reduced[0]["file"], "fromModelNode": False}]
    for blk, red in zip(blocks, reduced):
        kv = dict(re.findall(r"(?m)^\s*(\w+)\s*=\s*(.+?)\s*$", blk))
        inst = {"file": red["file"], "fromModelNode": True, "parent": kv.get("parent", "")}
        if "name" in kv:
            inst["name"] = kv["name"]
        for k in ("scale", "position", "rotation"):
            if k in kv:
                inst[k] = [float(x) for x in kv[k].replace(" ", "").split(",")]
        out.append(inst)
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--game", default=str(ROOT / "decompiled" / "KSP-dev"))
    args = ap.parse_args()
    game = pathlib.Path(args.game)
    gd = game / "GameData"
    staging = ROOT / "core" / "data" / "mu_staging_wheels"
    reduced_dir = ROOT / "core" / "data" / "wheel_models"
    found = {}
    for cfg in sorted(gd.rglob("*.cfg")):
        if "zDeprecated" in cfg.parts:
            continue
        text = cfg.read_text(encoding="utf-8-sig", errors="replace")
        if not re.search(r"^\s*PART\b", text, re.M) or not any(m in text for m in WHEEL_MODULES):
            continue
        m = re.search(r"(?m)^\s*name\s*=\s*(\S+)", text)
        name = m.group(1).replace("_", ".")  # PartLoader.cs: '_' -> '.'
        if name in found:
            continue
        found[name] = (cfg, re.findall(r"(?m)^\s*model\s*=\s*(\S+)", text), re.findall(r"(?m)^\s*mesh\s*=\s*(\S+)", text))
    staging.mkdir(parents=True, exist_ok=True)
    reduced_dir.mkdir(parents=True, exist_ok=True)
    manifest, red_manifest = {}, {}
    for name, (cfg, models, meshes) in sorted(found.items()):
        dest = staging / name
        dest.mkdir(exist_ok=True)
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
            shutil.copy2(f, dest / f.name)
            rec = {"file": f.name, "origin": f.relative_to(game).as_posix(), "sha256": hashlib.sha256(f.read_bytes()).hexdigest()}
            records.append(rec)
            if f.suffix == ".mu":
                out = reduce_mu.reduce(f.read_bytes())
                rname = "%s__%s" % (name, f.name)
                (reduced_dir / rname).write_bytes(out)
                reduced.append({"file": rname, "from": rec["file"], "origin": rec["origin"], "originalSha256": rec["sha256"],
                                "sha256": hashlib.sha256(out).hexdigest(), "bytes": len(out)})
        manifest[name] = {"files": records}
        red_manifest[name] = {"cfg": records[0]["origin"], "models": reduced, "instances": instances_of(cfg, reduced)}
        print(name, [r["file"] for r in records])
    (staging / "MANIFEST.json").write_text(json.dumps(manifest, indent=1) + "\n", encoding="utf-8")
    (reduced_dir / "MANIFEST.json").write_text(json.dumps(
        {"note": "reduced copies (transform tree, tag/layer, collider + wheel collider chunks, animation chunk; no meshes/renderers/"
                 "materials/textures) of the original game files named in 'origin' (decompiled/KSP-dev), made by core/tools/wp29d/"
                 "reduce_mu.py", "parts": red_manifest}, indent=1) + "\n", encoding="utf-8")
    print("staged", len(found), "parts")
    return 0


if __name__ == "__main__":
    sys.exit(main())
