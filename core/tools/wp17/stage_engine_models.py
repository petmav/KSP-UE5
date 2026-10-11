"""Stage the engine .mu models and .cfg files that WP-17's delta-v gate needs.

Copies, for each engine part used by the 17 captured crafts, the part .cfg and the
.mu model(s) it references from the dev copy (decompiled/KSP-dev/GameData) into
core/data/mu_staging/<partName>/ and writes MANIFEST.json with the origin path and
sha256 of every file (CLAUDE.md: record each asset's origin next to where it is used).

Local-only: needs decompiled/KSP-dev. Run from the repo root:
    python -I core/tools/wp17/stage_engine_models.py
"""
import hashlib
import json
import pathlib
import re
import shutil
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
GAMEDATA = ROOT / "decompiled" / "KSP-dev" / "GameData"
OUT = ROOT / "core" / "data" / "mu_staging"

# Engine parts of the 17 crafts in capture.deltav_stages (STATUS.md, WP-17).
WANT = {
    "JetEngine", "MassiveBooster", "SSME", "Size3AdvancedEngine", "ionEngine",
    "liquidEngine2", "liquidEngine2-2.v2", "liquidEngine3.v2", "liquidEngineMainsail.v2",
    "microEngine.v2", "miniJetEngine", "omsEngine", "radialLiquidEngine1-2",
    "solidBooster.sm.v2", "solidBooster.v2", "toroidalAerospike", "turboFanEngine",
    "turboFanSize2",
    "sepMotor1",  # Sepratron I (Dynawing, Learstar A1 separation motors)
}


def main() -> int:
    if not GAMEDATA.is_dir():
        print(f"missing {GAMEDATA}", file=sys.stderr)
        return 1
    found = {}
    for cfg in GAMEDATA.rglob("*.cfg"):
        if "zDeprecated" in cfg.parts:
            continue
        # utf-8-sig: most Squad cfgs start with a BOM, which would defeat the ^PART match.
        text = cfg.read_text(encoding="utf-8-sig", errors="replace")
        if not re.search(r"^\s*PART\b", text, re.M):
            continue
        for m in re.finditer(r"^\s*name\s*=\s*(\S+)", text, re.M):
            # PartLoader replaces '_' with '.' in part names (PartLoader.cs, CompilePart).
            name = m.group(1).replace("_", ".")
            if name in WANT and name not in found:
                found[name] = (
                    cfg,
                    re.findall(r"^\s*model\s*=\s*(\S+)", text, re.M),
                    re.findall(r"^\s*mesh\s*=\s*(\S+)", text, re.M),
                    re.findall(r"thrustVectorTransformName\s*=\s*(\S+)", text),
                )
    missing = WANT - set(found)
    if missing:
        print("parts not found:", sorted(missing))
    OUT.mkdir(parents=True, exist_ok=True)
    manifest = {}
    for name, (cfg, models, meshes, ttn) in sorted(found.items()):
        dest = OUT / name
        dest.mkdir(exist_ok=True)
        if models:
            files = [cfg] + [GAMEDATA / (mo + ".mu") for mo in models]
        elif meshes:
            # `mesh = name` may omit the extension (PartLoader appends .mu).
            files = [cfg] + [cfg.parent / (me if me.endswith(".mu") else me + ".mu") for me in meshes]
        else:
            files = [cfg] + sorted(cfg.parent.glob("*.mu"))
        records = []
        for f in files:
            if not f.exists():
                print("missing file", name, f)
                continue
            shutil.copy2(f, dest / f.name)
            records.append({
                "file": f.name,
                "origin": f.relative_to(ROOT / "decompiled" / "KSP-dev").as_posix(),
                "sha256": hashlib.sha256(f.read_bytes()).hexdigest(),
            })
        manifest[name] = {"thrustVectorTransformName": ttn, "files": records}
        print(name, [r["file"] for r in records], ttn)
    (OUT / "MANIFEST.json").write_text(json.dumps(manifest, indent=1) + "\n", encoding="utf-8")
    print("wrote", OUT / "MANIFEST.json")
    return 0


if __name__ == "__main__":
    sys.exit(main())
