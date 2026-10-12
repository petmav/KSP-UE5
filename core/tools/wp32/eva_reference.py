#!/usr/bin/env python3
"""WP-32: independent reference for the EVA kerbal controller -- test data generator.

Written from the decompiled C# only (decompiled/KSP-Decompiled/Assembly-CSharp/KerbalEVA.cs, RetractableLadder.cs, FlightEVA.cs, Kerbal.cs, CrewTransfer.cs,
CrewHatchController.cs, KerbalFSM.cs, KFSM*.cs and the UnityEngine Vector3 / Quaternion / Mathf managed sources); no kspcore code, no kspcore output.

How circularity with the C++ port is avoided:
  * the state / event tables (states, callback names in order, per-state event lists, targets, update modes, timer expressions) are PARSED from the C#
    text by tools/wp32/eva_tables.py -- nobody transcribes them, neither in this script nor, for the comparison, by hand;
  * the formulas are translated here from the C# lines (cited at each function) with numpy.float32 stores under the Mono float rule (a float
    expression is evaluated in double and rounded only where the C# stores / passes / returns it as a float); the C++ was ported separately,
    and the comparison is bit for bit (0 ULP);
  * the scenario traces (scripted worlds driven through the FSM) come from a second implementation of the FSM runtime (KerbalFSM.updateFSM / RunEvent
    semantics) over the PARSED tables with Python versions of the event conditions.
Inputs are game data (the kerbalEVA MODULE of core/testdata/GameData-mirror) plus fixed LCG draws; fixtures that stand for Unity engine state are
labelled synthetic in the C++ tests.

Usage: python -I tools/wp32/eva_reference.py > tests/data/wp32_reference.json
"""
import json
import math
import os
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import eva_tables  # noqa: E402
import eva_ref_motion as motion  # noqa: E402
import eva_ref_conditions as cond  # noqa: E402
import eva_ref_misc as misc  # noqa: E402
from eva_math import ROTATIONS  # noqa: E402


def f32(x):
    return float(np.float32(x))


def bits(x):
    return int(np.float32(x).view(np.uint32))


def v3b(v):
    return [bits(v[0]), bits(v[1]), bits(v[2])]


def main():
    out = {}
    out["kerbal_eva_fsm"] = eva_tables.to_json(eva_tables.kerbal_eva_table())
    out["ladder_fsm"] = eva_tables.to_json(eva_tables.retractable_ladder_table())
    out["rotations"] = [[bits(c) for c in q] for q in ROTATIONS]
    out["fps"] = motion.fps_family()
    out["movement"] = motion.movement_family()
    out["heading"] = motion.heading_family()
    out["pid"] = motion.pid_family()
    out["pack"] = motion.pack_family()
    out["conditions"] = cond.conditions_family()
    out["decisions"] = cond.decisions_family()
    out["helmet"] = misc.helmet_family()
    out["visor"] = misc.visor_family()
    out["impulse"] = misc.impulse_family()
    out["lowg"] = misc.lowg_family()
    out["clamber"] = misc.clamber_family()
    out["seatcam"] = misc.seatcam_family()
    out["rules"] = misc.rules_family()
    out["ladder"] = misc.ladder_family()
    json.dump(out, sys.stdout, indent=None, separators=(",", ":"))
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
