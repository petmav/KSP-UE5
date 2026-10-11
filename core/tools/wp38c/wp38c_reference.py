"""WP-38 slice 3 reference driver -> tests/data/wp38c_reference.json (no kspcore imports; reads the decompiled C# and the mirror cfgs).
Usage: python -I tools/wp38c/wp38c_reference.py [Assembly-CSharp dir]"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
CS_ROOT = sys.argv[1] if len(sys.argv) > 1 else r"D:/Projects/KSP-UE5/decompiled/KSP-Decompiled/Assembly-CSharp"
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
MIRROR = os.path.join(CORE, "testdata", "GameData-mirror", "GameData")

import ref_contractdefs  # noqa: E402

out = {}
out["contractDefs"] = ref_contractdefs.build(CS_ROOT, os.path.join(MIRROR, "Squad", "Contracts", "Contracts.cfg"))
for mod in ("ref_sentinel", "ref_compat"):
    if os.path.exists(os.path.join(HERE, mod + ".py")):
        m = __import__(mod)
        out[m.KEY] = m.build(CS_ROOT, MIRROR)
with open(os.path.join(CORE, "tests", "data", "wp38c_reference.json"), "w", encoding="utf-8", newline="\n") as f:
    json.dump(out, f, indent=0, sort_keys=True)
print("wrote", len(json.dumps(out)), "bytes")
