#!/usr/bin/env python3
"""Generate the larger scaling-curve crafts of the Jolt spike with the physics profile's own generator.

Usage: python -I gen_xl_crafts.py <path to analysis/physics/scripts/gen_craft.py> <outdir>

Same booster design as PhysProf-S/M2/L2 (PROFILE.md "Test craft"): core probe / K x X200-32 / Mainsail, rows of B radial
boosters (HDM decoupler, T x FL-T100, LV-T45, nose cone) on every other core tank. Part count = (K + 2) + ceil(K / 2) * B * (T + 3).
Written into the build tree only (CMake configure step); never committed.
"""
import importlib.util
import os
import sys

CRAFTS = [
    # name, (K, B, T), autostrut
    ("PhysProf-XL1-AS", (53, 4, 6), "Grandparent"),   # 1,027 parts
    ("PhysProf-XL2-AS", (107, 4, 6), "Grandparent"),  # 2,053 parts
    ("PhysProf-XL1", (53, 4, 6), "Off"),
]


def main():
    gen_path, outdir = sys.argv[1], sys.argv[2]
    spec = importlib.util.spec_from_file_location("gen_craft", gen_path)
    gen = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gen)
    os.makedirs(outdir, exist_ok=True)
    for name, (k, b, t), auto in CRAFTS:
        path = os.path.join(outdir, name + ".craft")
        gen.P.n = 4294000000
        parts = gen.build(k, b, t)
        if not os.path.exists(path):
            gen.write(path, name, parts, auto)
        print("%s=%d" % (name, len(parts)), end=" ")


if __name__ == "__main__":
    main()
