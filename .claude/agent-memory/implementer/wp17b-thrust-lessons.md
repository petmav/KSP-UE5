---
name: wp17b-thrust-lessons
description: WP-17b thrust-transform forensics - what the stage delta-v capture revealed about Unity native quaternion/Transform behaviour, and the sepMotor1 staging gap
metadata:
  type: project
---

- The .mu staging (core/data/mu_staging, main checkout only, gitignored) now has 19 engines incl. `sepMotor1` (nonThrustMotor, not in delta-v). Point `mu_transforms.py --staging C:/Projects/KSP-UE5/core/data/mu_staging` at it.
- Native Transform.rotation composition is NOT the managed Quaternion.operator* order: the order "cross first, then lw*rv, then rw*lv" (UnityNative::Multiply) fixed PhysProf crafts; the Mono-rule product (double, one store) is wrong for native code.
- Editor ship parts are re-parented with world pose kept (ShipConstruct.cs:473/644); Unity normalises stored rotations: normalising after parenting is required (ComSat Lx 467 ULP off otherwise).
- Fast forensics loop: `run_tests.ps1 -BuildOnly`, run `build/RelWithDebInfo/tools/golden_runner/golden_runner.exe deltav_stages --harness harness --out build/RelWithDebInfo/candidate`, compare per row in python (stage tables are in `tables` list with `columns`/`rows`). Env-var knobs in code made 100 variants cheap; strip them before commit. Residuals left: Kerbal X (Swivel |forward.y| wants +1..2 ULP), Stearwing A300 (turboFanSize2).
- `namespace support` already exists (editor_staging.hpp, global) and clashes with `using namespace ksp` + any other `support`; new tool helpers use their own namespace (`wp17b`).
- Never `rm` from a `../..` relative path in Bash: the safety check blocks it and aborts the whole command; scratch files go under core/build.
- Session 2 findings: Unity composes world rotation fully right-nested from the LEAF (part chain AND model chain together): `World(part,&chain)` fixed Kerbal X (28 rows). Native w = (lw*rw - lx*rx) - (ly*rz... i.e. (ww - d0) - (d1 + d2)) fixed Learstar A1. Remaining RED: Dynawing (14 rows, MassiveBooster stage 1, 3-4 ULP) and Stearwing A300 (6 rows); our forward there has |f|^2 up to 1.0000012 yet normalising anywhere makes 10+ crafts wrong. Next step needs an in-game capture of part.transform.rotation + thrustTransform.rotation/forward for those two crafts.
- A bash heredoc containing an apostrophe in a long multi-heredoc command broke parsing; write long docs with the Write tool. Background `( ... ) &` in Bash tool: ok, poll with `until grep -q ^done log`.
