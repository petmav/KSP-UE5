---
name: wp17b-thrust-lessons
description: WP-17b thrust-transform forensics - what the stage delta-v capture revealed about Unity native quaternion/Transform behaviour, and the sepMotor1 staging gap
metadata:
  type: project
---

- The .mu staging (core/data/mu_staging, main checkout only) has 18 engines but NOT `sepMotor1` (Sepratron I; Dynawing, Learstar, Ariane 5 ...). The runner throws on it (gate red) until it is staged and `python -I core/tools/wp17/mu_transforms.py` rerun.
- Native Transform.rotation composition is NOT the managed Quaternion.operator* order: the order "cross first, then lw*rv, then rw*lv" (UnityNative::Multiply) fixed PhysProf crafts; the Mono-rule product (double, one store) is wrong for native code.
- Editor ship parts are re-parented with world pose kept (ShipConstruct.cs:473/644); Unity normalises stored rotations: normalising after parenting is required (ComSat Lx 467 ULP off otherwise).
- Fast forensics loop: `run_tests.ps1 -BuildOnly`, run `build/RelWithDebInfo/tools/golden_runner/golden_runner.exe deltav_stages --harness harness --out build/RelWithDebInfo/candidate`, compare per row in python (stage tables are in `tables` list with `columns`/`rows`). Env-var knobs in code made 100 variants cheap; strip them before commit. Residuals left: Kerbal X (Swivel |forward.y| wants +1..2 ULP), Stearwing A300 (turboFanSize2).
- `namespace support` already exists (editor_staging.hpp, global) and clashes with `using namespace ksp` + any other `support`; new tool helpers use their own namespace (`wp17b`).
- Never `rm` from a `../..` relative path in Bash: the safety check blocks it and aborts the whole command; scratch files go under core/build.
