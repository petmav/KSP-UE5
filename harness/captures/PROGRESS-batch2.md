# Captures batch 2 (C4, C5, C6, C8, C9, C10, terrain) progress log

Agent: Opus implementer, started 2026-10-09.

## Pre-state
- Repo main @ 4e54f682 clean; profiling @ 5fedcfab. Symlinked DLL sha256 42023ed1... (main build-09). No KSP running.

## Log
- Checked out `profiling` in the main checkout (as batch 1 did). MUST checkout main + Rebuild + launch check at end.
- Orchestrator directive (assets): C9 must record cfg/model(.mu)/texture origin paths per part (GameDatabase-resolved); terrain must record heightmap/PQSMod source asset names per body.

## Design (new file Assembly-CSharp/PhysProf/HarnessCapture2.cs; HarnessCapture.Run dispatches)
- Items: c9 (MAINMENU), terrain (FLIGHT; also MAINMENU probe of PQS state), c5 (EDITOR, 3 PhysProf + 14 harness stock craft, situations Kerbin/Mun/Duna/Eve alt 0),
  flight session S: c6 pad (500 fr) -> c4 (UT0,+0.25d,+0.5d,+0.25y via Planetarium.SetUniversalTime) -> c8 (spawn probes via ProtoVessel.CreateVesselNode+Game.AddVessel; warp 1x/100x/1000x/10000x/100000x on pad; Mun encounter built backwards from Mun-relative hyperbola; hyp12 escapes to Sun)
  -> c10 ascent S -> c6 orbit (SetShipOrbit 100 km) -> c6 reentry (PeA 25 km/ApA 500 km at 80 km descending)
  then c10 for M2 and L2-AS (new launches via SPACECENTER).
- Outputs: harness/captures/out2/*.json, reports2/; logs decompiled/logs/ksp-capture-capture2-<tag>-*.log
- Deviations: e081 sma 3000 km has rp inside Kerbin -> use sma 4000 km (rp alt 160 km); epochs = in-game UT at spawn.
- 10:30 build capture2-02/03 OK (HarnessCapture2.cs + HarnessCapture2Flight.cs, csproj updated). Test t1 (c9,terrain): 57 s, OK.
  C9 362 parts (4 DLC-variant EVA parts have null prefab -> error rows), 48.6k rows, 6 MB. Terrain 140,522 samples in 2.4 s, 28.8 MB; MAINMENU == FLIGHT heights; KSC 64.78 m; Mun 3 pts h_surface != h_override.
- Next: test t2 c5,c4,c6; t3 c8,c10.
- 10:45 t2b (c5,c4,c6) OK in 130 s, 0 exceptions (first t2 failed: EventVoid rejects static handlers -> instance Handlers + Safe() coroutine wrapper).
- tools/batch2_report.py written (c9 cube vs PartDatabase.cfg, cfg->field check, terrain stats, c4, c5 rocket-eq check, c6, c8 Kepler check, c10 aoa0==live). run_capture.sh extended (batch-2 items -> out2/, reports2/).
- t3 (c8,c10) running in background (task bsmifsat6); C8 done (4 SOI events).
- 10:57 t3 (c8,c10): 451 s, OK. C8: 4 SOI events (mun_encounter Kerbin->Mun->Kerbin->Mun, hyp12 Kerbin->Sun); in-game pos==getRelativePositionAtUT to 1e-15. Kepler check needs Planetarium.Zup (rotating frame) -> added C8 'frames' table.
  C10: S never lifted off after C8 warp (EC drained -> no probe control) -> EC refill before ascent. AoA sweep rewritten to rotate live dragVectorDirLocal (aoa0 matched only 10/4140 before). Added node contact areas.
  97 NREs in t3 were stock TemperatureGauge.OnDestroy at app teardown from FLIGHT (after 'quitting') -> now return to SPACECENTER before quit.
- Next: commit, build_opt, final `run_capture.sh batch2` run.
- 11:05 run capture2-final (commit c23196a1): all items OK except C6 orbit/reentry: S ran out of fuel in the 90 s C10 window and crashed -> C10 now ends at apex. C8 Kepler check with Zup: 4.7e-13 rel. C10 aoa0==live 4203/4203.
  C9 findings: cubes 531/531 bit-exact vs cfg text (67 from part-cfg DRAG_CUBE, rest PartDatabase.cfg top-level only; PartDatabase has a nested PART for MpoProbe); double Part fields parsed via float.Parse (ApplyPartValue typeof(double) twice); crewed parts mass -= PerSeatReduction*CrewCapacity.
- Next: commit apex fix, build_opt capture2-06, rerun batch2 (tag capture2-final2).
- 11:25 FINAL run capture2-final2 at profiling 36ef51bd (DLL sha 42d4b00d..., Optimize=true), 484 s, all 7 files, 0 decimal/hex mismatches, only 4 'Exception' log lines = handled C9 messages for DLC-suit EVA parts; no teardown NREs.
  Reports in reports2/*.md. C9 cubes 531/531, C8 kepler 4.7e-13 rel, C10 aoa0 4224/4224, C6 3 cases, C5 17 craft, terrain 140,522 samples.
- Next: README section, then restore main (checkout main, build.sh Rebuild, launch_ksp to MAINMENU, 0 exceptions, symlink hash).
- 11:40 README.md: batch-2 section appended (results table, notes, surprises, rerun, files, limitations); title and stale 'not attempted' bullet updated.
- Next: restore main (checkout, build.sh Rebuild, hash check, launch_ksp to MAINMENU).
- DONE. Restore: main checked out (4e54f682, clean); build-10-post-capture2-main Rebuild: 0 errors, 1454 warnings, 9,866,240 bytes, sha256 8966e0c2..., no PhysProf/HarnessCapture code; symlink target == build output.
  launch_ksp.ps1 -Tag post-capture2-main: MAINMENU reached, 0 exceptions (KSP.log + Player.log), killed; no KSP_x64 process running.
  settings.cfg vs backup/settings.cfg.pre-profiling: only SHOW_WHATSNEW flags differ (as after batch 1).
  profiling head 36ef51bd (c23196a1, fdde6a30, 36ef51bd added). KSP-dev/saves/physprofile was re-created by the capture's new game (now holds the batch-2 state incl. HC8 probes).
