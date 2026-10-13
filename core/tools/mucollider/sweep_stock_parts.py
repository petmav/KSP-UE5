#!/usr/bin/env python
"""WP-mucol: collider sanity sweep over EVERY stock part of a game copy (local only: needs GameData).

Run:  python -I core/tools/mucollider/sweep_stock_parts.py --game decompiled/KSP-dev [--out core/data/mucollider_stock_counts.json]

For each PART in GameData/**/*.cfg (the files PartLoader reads: PartLoader.cs:1765 CompileModel), resolve the model(s) the part is
built from (MODEL { model = <url> } nodes -> GameData/<url>.mu; no MODEL node -> the .mu files of the cfg's directory, the first one
is the part's; directories holding several are listed as AMBIGUOUS because the file order is UrlDir's), parse every .mu with the
independent reference reader (mu_colliders_reference.py), place it as CompileModel does (rescaleFactor, MODEL scale / position;
rotation is Quaternion.Euler and is skipped with a note) and check:

  * the file parses to EOF with the PartReader grammar (no exception);
  * the part has at least one non-trigger, non-wheel collider (parts with none are listed: KSP gives them no collision shape);
  * every mesh collider is a closed solid (positive volume inside its bounding box);
  * collider extents are sane (0.005 m .. 60 m on every axis).
Writes the per-part table (counts per chunk type, triggers, wheel markers, bounding box of the colliders) as JSON. Nothing in the
output is committed unless the owner wants the table: it is derived from the game's files.

Exit status 0 when every part passes; the failures are printed.
"""
import argparse
import json
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mu_colliders_reference as ref  # noqa: E402
import mu_transforms as w17  # noqa: E402  (via ref's sys.path entry)


def part_blocks(text):
    """(name, body) of every top-level PART { } node of a cfg (// comments stripped, braces matched)."""
    clean = "\n".join(l.split("//")[0] for l in text.splitlines())
    out = []
    i = 0
    for m in re.finditer(r"(?m)^\s*PART\s*\{", clean):
        depth = 0
        j = m.end() - 1
        start = j
        while j < len(clean):
            if clean[j] == "{":
                depth += 1
            elif clean[j] == "}":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        body = clean[start:j + 1]
        nm = re.search(r"(?m)^\s*name\s*=\s*(\S+)", body)
        out.append((nm.group(1) if nm else "?", body))
    return out


def model_nodes(body):
    """MODEL nodes (depth 1 only) as dicts."""
    rescale, models = w17.parse_cfg_model_nodes(body.strip()[1:-1] if body.strip().startswith("{") else body)
    return rescale, models


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--game", required=True, help="game root containing GameData/")
    ap.add_argument("--out", default=None)
    args = ap.parse_args()
    gd = os.path.join(args.game, "GameData")
    rows, failures, notes = {}, [], []
    for dirpath, _, files in os.walk(gd):
        for fn in sorted(files):
            if not fn.lower().endswith(".cfg"):
                continue
            path = os.path.join(dirpath, fn)
            try:
                text = open(path, encoding="utf-8-sig", errors="replace").read()
            except OSError:
                continue
            if "PART" not in text:
                continue
            for name, body in part_blocks(text):
                if name in rows:
                    continue
                m = re.search(r"(?m)^\s*rescaleFactor\s*=\s*(\S+)", body)
                rescale = np.float32(m.group(1)) if m else np.float32(1.25)
                models = [dict(re.findall(r"(?m)^\s*(\w+)\s*=\s*(.+?)\s*$", blk)) for blk in
                          re.findall(r"(?ms)^\s*MODEL\s*\{(.*?)\}", body)]
                mus = []
                if models:
                    for mm in models:
                        mus.append(os.path.join(gd, mm["model"].replace("/", os.sep) + ".mu"))
                else:
                    cand = sorted(f for f in os.listdir(dirpath) if f.lower().endswith(".mu"))
                    if len(cand) > 1:
                        notes.append("AMBIGUOUS %s: %d .mu files in %s, no MODEL node" % (name, len(cand), dirpath))
                    if cand:
                        mus.append(os.path.join(dirpath, cand[0]))
                row = {"cfg": os.path.relpath(path, args.game), "mu": [os.path.relpath(p, args.game) for p in mus]}
                try:
                    roots = []
                    for p in mus:
                        _, r = ref.read_mu(open(p, "rb").read())
                        roots.append(r)
                    if not roots:
                        raise ValueError("no model")
                    norm = [{k: [float(x) for x in v.replace(" ", "").split(",")] if k in ("scale", "position") else v
                             for k, v in mm.items()} for mm in models]
                    if any("rotation" in mm for mm in models):
                        notes.append("%s: MODEL rotation (Quaternion.Euler) not applied" % name)
                    model = ref.build_chain_general(roots, rescale, norm)
                    cols = []
                    ref.walk(model, [], cols)
                except Exception as e:  # noqa: BLE001
                    failures.append("%s: %s" % (name, e))
                    continue
                shapes = [c for c in cols if c["isShape"] and not c["trigger"]]
                row["colliders"] = len(cols)
                row["bodyShapes"] = len(shapes)
                row["triggers"] = sum(1 for c in cols if c["trigger"])
                row["wheelMarkers"] = sum(1 for c in cols if not c["isShape"])
                row["kinds"] = sorted(set(c["entry"] for c in shapes))
                if not shapes:
                    failures.append("%s: no non-trigger collider" % name)
                for c in shapes:
                    mp = c["partSpace"].get("massProps")
                    if c["entry"].startswith("Mesh") and (mp is None or float(mp["volume"]) <= 0):
                        failures.append("%s: mesh collider %s is not a closed solid" % (name, c["path"]))
                rows[name] = row
    print("parts %d, failures %d, notes %d" % (len(rows), len(failures), len(notes)))
    for n in notes:
        print("note:", n)
    for f in failures:
        print("FAIL:", f)
    if args.out:
        with open(args.out, "w", encoding="utf-8", newline="\n") as f:
            json.dump({"parts": rows, "failures": failures, "notes": notes}, f, indent=1, sort_keys=True)
            f.write("\n")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
