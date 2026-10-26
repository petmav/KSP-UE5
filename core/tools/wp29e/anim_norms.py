#!/usr/bin/env python3
"""WP-29e: the unit length of the rotation keys of the 13 stock wheel / gear / leg model clips -> core/tests/data/wp29e_anim_norms.json.

PartModelRig::SampleClip renormalises the rotation it samples (Unity's quaternion blend normalises; UNVERIFIED in WP-29d). This measures what the
renormalisation can change, from the reduced models (core/data/wheel_models, origin and sha256 in its MANIFEST.json), per clip and per node that has rotation
curves (m_LocalRotation.x / y / z / w):

  * the STORED quaternions: at every key time of the four component curves the four curve values (the stored keys when the times coincide), their norm;
  * the INTERPOLATED quaternion between keys: the four components evaluated with Unity's cached Hermite cubic (wheel_model_reference.evaluate) at 64
    equidistant times per segment, the norm before any renormalisation.

All in float64 on the float32 key values / the float32 curve evaluation. Output: per clip the maxima of |1 - norm| and where, plus the totals.

Usage:  python -I core/tools/wp29e/anim_norms.py > core/tests/data/wp29e_anim_norms.json
"""
import hashlib
import json
import math
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "wp29d"))
import mu_wheel_reader as R  # noqa: E402
import wheel_model_reference as W  # noqa: E402

F = np.float32
REDUCED = os.path.join(HERE, "..", "..", "data", "wheel_models")


def walk(n, path=()):
    yield n, path + (n.name,)
    for c in n.children:
        yield from walk(c, path + (n.name,))


def main():
    manifest = json.load(open(os.path.join(REDUCED, "MANIFEST.json")))["parts"]
    out = {"format": "kspcore-wp29e-anim-norms/1", "note": "tools/wp29e/anim_norms.py over core/data/wheel_models", "models": {}, "totals": {}}
    stored_max, interp_max, groups, key_samples, interp_samples = 0.0, 0.0, 0, 0, 0
    for name in sorted(manifest):
        for inst in manifest[name]["instances"]:
            data = open(os.path.join(REDUCED, inst["file"]), "rb").read()
            ver, root = R.read_wheel_mu(data)
            entry = {"file": inst["file"], "sha256": hashlib.sha256(data).hexdigest(), "clips": []}
            for n, path in walk(root):
                for a in n.anims:
                    for cl in a["clips"]:
                        by_target = {}
                        for cu in cl["curves"]:
                            if cu["type"] != 0 or not cu["prop"].startswith("m_LocalRotation."):
                                continue
                            by_target.setdefault(cu["path"], {})[cu["prop"][-1]] = cu["keys"]
                        cj = {"owner": "/".join(path), "clip": cl["name"], "rotationTargets": len(by_target), "storedMax": 0.0, "interpMax": 0.0, "keySamples": 0,
                              "interpSamples": 0}
                        for tgt, comps in by_target.items():
                            if set(comps) != set("xyzw"):
                                raise SystemExit("%s %s: a rotation target without four components" % (name, cl["name"]))
                            groups += 1
                            times = sorted({float(F(k[0])) for c in comps.values() for k in c})
                            for t in times:
                                q = [float(W.evaluate(comps[c], F(t))) for c in "xyzw"]
                                dev = abs(1.0 - math.sqrt(sum(v * v for v in q)))
                                cj["storedMax"] = max(cj["storedMax"], dev)
                                cj["keySamples"] += 1
                                key_samples += 1
                            for a0, b0 in zip(times, times[1:]):
                                for i in range(1, 64):
                                    t = a0 + (b0 - a0) * i / 64.0
                                    q = [float(W.evaluate(comps[c], F(t))) for c in "xyzw"]
                                    dev = abs(1.0 - math.sqrt(sum(v * v for v in q)))
                                    cj["interpMax"] = max(cj["interpMax"], dev)
                                    cj["interpSamples"] += 1
                                    interp_samples += 1
                        stored_max = max(stored_max, cj["storedMax"])
                        interp_max = max(interp_max, cj["interpMax"])
                        entry["clips"].append(cj)
            out["models"][name] = entry
    out["totals"] = {"rotationTargets": groups, "storedKeySamples": key_samples, "storedMax": stored_max, "interpSamples": interp_samples, "interpMax": interp_max}
    json.dump(out, sys.stdout, indent=1, sort_keys=True)
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
