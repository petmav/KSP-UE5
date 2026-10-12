#!/usr/bin/env python3
"""WP-54 slice 2 independent reference (no kspcore code): the mission data classes, the vessel restrictions, the deferred Tests.* and the create Actions.

Written from the decompiled C# (Assembly-CSharp/Expansions/Missions/*.cs). It generates the INPUTS (seeded configs and vessel / mission scenarios, labelled
synthetic: every stock mission file of the GameData mirror is a 0-byte file and the Making History cfgs are absent) and the EXPECTED outputs;
tests/test_missions2.cpp rebuilds the same scenarios in kspcore and compares them. Text goes through the real Squad/Localization/dictionary.cfg.

Usage: python -I tools/wp54b/ref54b.py   (writes tests/data/wp54b_reference.json)
"""
import json
import os
import random
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from ref_common import OUT, BODIES  # noqa: E402
import ref_data  # noqa: E402


def main():
    cases = []
    r = random.Random(54001)
    gens = [("ground", ref_data.gen_ground, 40), ("area", ref_data.gen_area, 40), ("volume", ref_data.gen_volume, 40), ("asteroid", ref_data.gen_asteroid, 30),
            ("comet", ref_data.gen_comet, 30), ("surface", ref_data.gen_surface, 80), ("launchsite", ref_data.gen_launchsite, 30), ("orbit", ref_data.gen_orbit, 60),
            ("location", ref_data.gen_location, 40), ("crew", ref_data.gen_crew, 30)]
    for _, fn, n in gens:
        for _ in range(n):
            cases.append(fn(r))
    for mod in ("ref_restrict", "ref_tests2", "ref_tests3", "ref_tests4", "ref_actions2"):
        if os.path.exists(os.path.join(HERE, mod + ".py")):
            m = __import__(mod)
            cases += m.generate(r)
    doc = {"bodies": BODIES, "cases": cases}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(doc, f, indent=None, separators=(",", ":"))
    kinds = {}
    for c in cases:
        kinds[c["kind"]] = kinds.get(c["kind"], 0) + 1
    print("wrote %s: %d cases %s" % (OUT, len(cases), kinds))


if __name__ == "__main__":
    main()
