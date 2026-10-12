#!/usr/bin/env python3
"""WP-54c independent reference (no kspcore code): TestDistance, the part actions, the Mission craft-file bookkeeping, MissionScoreInfo / MissionLocalizer /
MissionImporting / MissionEditorHistory, the GetHashCode overrides, the real BaseAPField, PositionMobileLaunchPad, MissionSystem and the ActionCreateVessel scene branch.

Written from the decompiled C# (Assembly-CSharp/Expansions/Missions/*.cs). It generates the INPUTS (seeded configs and scenarios, labelled synthetic: every
stock mission file of the GameData mirror is a 0-byte file and the Making History cfgs are absent) and the EXPECTED outputs; tests/test_missions3.cpp
rebuilds the same scenarios in kspcore and compares them. Text goes through the real Squad/Localization/dictionary.cfg.

Usage: python -I tools/wp54c/ref54c.py   (writes tests/data/wp54c_reference.json)
"""
import json
import os
import random
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from ref_c_common import OUT, BODIES  # noqa: E402

MODULES = ["ref_distance", "ref_partactions", "ref_craft", "ref_small", "ref_hash", "ref_apfield", "ref_pad", "ref_system"]


def main():
    cases = []
    r = random.Random(54003)
    for mod in MODULES:
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
