---
name: wp38-fineprint-lessons
description: WP-38 FinePrint port lessons - tool output elision workaround, golden body values are prefab values, node ids, fixture defaults, reuse of the wp37 python reference
metadata:
  type: project
---

- The harness elides function bodies of .py/.cpp/.hpp when a file is printed (shows `...` / `/* caveman: body elided */`); the file on disk is intact. Print through `awk '{print "|" $0}'` (any line prefix) to see real code.
- Never print golden JSON lists/dicts (celestial_bodies orbitingBodies has hundreds of empty refs): print scalars only.
- Chain destructive steps with `&&` (a `rm x.in` after a failing python assembler lost the only copy of a 500 line source). Windows python cannot read git-bash `/tmp` files created by shell redirects only through relative paths; copy to cwd first.
- No `python3` on this box: use `python` (3.10, numpy ok).
- Golden `celestial_bodies.serialized.json` holds PREFAB values: Mass / gravParameter / sphereOfInfluence are 0, rotationPeriod is the prefab one (Mun 100 s). Runtime values come from CelestialBody Awake / Start (solar day -> sidereal, SOI = sma * (M/Mref)^0.4) and CBUpdate (tidallyLocked: rotationPeriod = orbit.period). kspcore does not evaluate `minOrbitalDistance` in SetupConstants (WP-10); the WP-38 test fixture applies `GetMinimumOrbitalDistance(b, 1f)`.
- ProgressNode ids: crew transfer is `CelestialBodyTransfer` (the wp37 python NODE_ID said `CrewTransfer`, patched by monkeypatch in wp38 reference); returns are `ReturnFromOrbit/FlyBy/Surface`; the home subtree has no Flight/Suborbit/Flyby/ReturnFromFlyBy/ReturnFromSurface nodes (null pointers in C++!).
- ScenarioUpgradeableFacilities.GetFacilityLevel returns 1.0 when no scenario is loaded (so eva / flags / rendezvous are unlocked and GetProgressLevel adds 5).
- `tools/wp37/contract_reference.py` is importable (sys.path) for cfg parser, Loc, Unity RNG, Contract.Set*, agents, back_story; reuse it for any contract reference.
- Mono List.Sort for n <= 16 is insertion sort, n == 2 / 3 use SwapIfGreater networks (not stable for ties in the 3 case): the python reference restates this.
- KSPRandom(int.MinValue+1) needs int32 wrap in the python seeding (Abs(seed) near int.MaxValue overflows in `num - num2`).
**Why:** each cost a debug round in WP-38. **How to apply:** before FinePrint / contract / body-constant work and before writing python references.
