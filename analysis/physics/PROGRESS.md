# Physics review + profile: progress log (working memory)

Updated: 2026-10-08, about 20:55.

## Done

### Deliverable 1: `PHYSICS-ENGINE-REVIEW.md`
- Sections 1-12 are written and inserted, about 1060 lines.
- §1 holds the measured call order from `raw/run1/run1-M_calltrace.csv`.
- §11 ranking:
  1. No per-tick floating-origin shift
  2. Data-oriented per-part pipeline
  3. Reduced rigid-body model
  4. Module systems
  5. Multithreading
  6. Non-active vessels
  7. Collision filtering
  8. GC
  9. Joint solver
  10. Cheap skips
- §11 numbers come from run3 L2-AS, run2 M2 and run1 S.
- **TODO:** refresh the §11 numbers with the run4 repeats if they differ by more than about 10%.

### Instrumentation
- Branch `profiling`, commits `91e23f3a`, `0414c022`, `10006ec2`.
- The worktree is at `decompiled/KSP-Decompiled-profiling`. It builds with Optimize=true. The output is copied over `decompiled/KSP-Decompiled/GameData/Assembly-CSharp-cleaned/`, which is the symlink target.
- **Main checkout:** branch `main`, clean.
- **Symlinked DLL:** currently the **profiling** build. It must be rebuilt from main at the end.

### Craft
- In `analysis/physics/craft/` and copied to `KSP-dev/Ships/VAB/`.

| Craft | Parts | Engines | Autostruts |
|---|---|---|---|
| S | 21 | 3 | — |
| M2 | 153 | 17 | — |
| L2 | 457 | 49 | — |
| S-AS, M2-AS, L2-AS | as above | as above | Grandparent |

- `craft/run1-layout/` holds the original M (150) and L (446). L self-destroyed at ignition: exhaust impingement and stacked boosters.
- L2 (no autostruts) collapsed within 1 s of unpack.
- L2-AS flies.

### Runs
Raw data is in `analysis/physics/raw/run{1..4}`.

| Run | Contents |
|---|---|
| run1 | S, M (150), L (446: pad valid, ascent = explosion) |
| run2 | M2 OK; L2 collapsed (invalid) |
| run3 | L2-AS OK. This is the first build with `fo_shifts` and the `rt_scene` census. |
| run4 | S-AS, M2-AS, M2, S, L2-AS repeats (batch in progress) |

## Key measured facts (keep)

**Runtime constants** (from the `rt_` dump):

| Constant | Value |
|---|---|
| `Physics.defaultSolverIterations` | 50 (velocity iterations: 1) |
| `autoSyncTransforms` | True |
| `maxDepenetrationVelocity` | 1e32 |
| `StackAttachStiffNess` / `SrfAttachStiffNess` | 10 |
| Kerbin `inverseRotThresholdAltitude` | 100000 |
| `warpRates` | 1, 5, 10, 50, 100, 1e3, 1e4, 1e5 |
| `physicsWarpRates` | 1-4 |
| `maxPhysicsRate_index` | 0 |
| Krakensbane `MaxV` | 5 |
| FloatingOrigin threshold | 500 |
| Joint `projectionMode` | None |
| Rigidbody interpolation | None |
| Collision detection | Discrete |

**L2-AS** (457 parts, 960 joints):

| | Pad | Ascent |
|---|---|---|
| FPS | 88 | 26 |
| Frame time | 11.4 ms | 38.4 ms |
| Ticks per frame | 0.57 | 1.77 |
| Sim rate | 1.0 | 0.92 |
| Physics tick | 8.1 ms | 15.7 ms |
| FlightIntegrator | 3.3 ms | — |
| FloatingOrigin | 0 | 3.3 ms |
| PhysX | 1.96 ms | 3.96 ms |

- Within the ascent window, PhysX is 3.8-4.1 ms on ticks with an origin shift and 1.9-2.25 ms on ticks without.
- Scene census: 1325 static colliders, of which 917 are static MeshColliders.

**Other results:**
- S ascent: 173 FPS. M2 ascent: 115 FPS. PQS UpdateQuads costs 0.55-0.72 ms per frame, constant.
- GC: 0-3 per minute, about 1 MB/s allocation; frames containing a GC take 13-25 ms.
- Instrumentation costs about 50 ns per call pair, which is ≤ 2% overhead.
- **Measured call order:** Timing0 → Planetarium → VesselPrecalculate → TimingPre → OrbitDriver → Timing1 → CollisionEnhancer → FlightGlobals → PQS → FlightInputHandler → TimeWarp → [Part + module FixedUpdates in registration order, PartBuoyancy] → Timing3 → FlightIntegrator → TimingFI → OrbitPhysicsManager → Vessel → Timing4 → Krakensbane → FloatingOrigin → Timing5 → CollisionManager → PhysX.

## Next steps (exact)

1. Wait for run4 to finish (`tasks/b00eqk7p4`).
2. Run the GPU-sensitivity test: `cp decompiled/backup/settings.cfg.profiling-userdefault KSP-dev/settings.cfg`, then run `profile_ksp.ps1 -Tag run5-M2-gfxdefault -Craft …PhysProf-M2.craft -Width 1280 -Height 720 -OutDir raw/run5`.
3. Fix `analyze.py`: the sections H and I insertion failed. Insert them before the final `print("\n".join(out))` at line 275.
4. Generate the tables with `python scripts/analyze.py <summary csvs…> > raw/tables.md`. Write `PROFILE.md` results: replace `<!-- RESULTS -->` and `<!-- CRAFT -->`. Its §4 is the origin-shift split, §5 GC, §6 runtime constants.
5. **Restore:**
   - `cp decompiled/backup/settings.cfg.pre-profiling KSP-dev/settings.cfg`
   - `git worktree remove ../KSP-Decompiled-profiling --force` (the branch is kept)
   - in the main checkout: `bash tools/scripts/build.sh 08-post-profiling-main Rebuild`
   - verify the symlinked DLL hash equals the main build output
   - `powershell -File tools/scripts/launch_ksp.ps1 -Tag post-profiling-main` and confirm MAINMENU is reached with 0 exceptions
6. Delete `saves/physprofile` from KSP-dev? It was created by the driver, so it is optional to remove. Remove the `PhysProf-*.craft` files from `KSP-dev/Ships/VAB`? Keep them: the brief says originals may stay.
7. Final report through SubagentHandback. It must state that 5 Explore children were spawned with no model override (inherited) before the orchestrator's cap directive, and none after.

## Added since 20:55
- `analyze.py` sections H (floating-origin split) and I (GC frames) are working.
- `raw/tables-prelim.md` covers the run4 repeats and run3 L2-AS.
- **Repeatability:** M2 run2 vs run4 is within 1% FPS. S ascent varies by about 25%: frames are small and noisy.
- **Autostrut effect:**
  - Joints: M2 has 168, M2-AS has 320.
  - PhysX on the pad: 0.80 → 0.97 ms/tick, about 1.1 µs per extra joint per tick.
  - FPS is essentially unchanged.
- **Orchestrator note:** design the driver so a later "capture mode" is cheap to add, e.g. a cfg-driven action list with CSV outputs. Do not implement it now. Mention it in `PROFILE.md`.
- About 21:00: run4 is complete. `raw/tables-main.md` covers run4 S, S-AS, M2, M2-AS and L2-AS plus run3 L2-AS. `raw/tables-run1-run2.md` covers the older runs.
- run5 (M2 with the user's graphics settings at 1280x720) is in progress. When it finishes, write `PROFILE.md` results, then restore.
- Scaling, ms per tick (ascent / pad):

| Craft | Parts | Ascent | Pad |
|---|---|---|---|
| S | 21 | 2.86 | 1.12 |
| M2 | 153 | 6.45 | 3.15 |
| L2-AS | 457 | 15.78 | 8.25 |

  That is about 30 µs per part per tick in ascent and 16 in pad.
- Floating-origin split, PhysX ms per tick (no shift → shift): M2 0.71 → 2.27; L2-AS 1.89 → 4.14. S pad 0.50 vs ascent 1.73.
- About 21:10: PROFILE.md is assembled. Review §11 is refreshed with the run4 numbers. Next: restore (step 5 in Next steps).
- About 21:15: restore complete.
  - `settings.cfg` was restored; its sha matches `pre-profiling`.
  - The worktree was removed; branch `profiling` is kept (`10006ec2`).
  - Main was rebuilt as build-08: 0 errors, 9,866,240 bytes, no PhysProf code. The symlinked DLL hash matches.
  - Launch check (`post-profiling-main`): MAINMENU reached in 33 s, 0 exceptions. KSP was killed by `launch_ksp.ps1` as usual.
- **User message during the restore:** "dont kill them yet, just ask". It arrived after the verification launch had already been killed by the script. Nothing else is running.
- **Left in place, pending the user's call:**
  - `KSP-dev/saves/physprofile`
  - `KSP-dev/Ships/VAB/PhysProf-*.craft`
  - `decompiled/backup/settings.cfg.profiling-{low,userdefault}`
