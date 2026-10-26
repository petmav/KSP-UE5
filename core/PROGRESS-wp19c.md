# PROGRESS WP-19c: flight registry close-out

Branch `wp19c` from main `839f2f5f`, worktree `D:/Projects/wt-wp19c`. Agent: optimizer (Opus).

## Step 1: audit (2026-10-26)

Method: every member of the ten WP-19 spec files (analysis/tiering/work_packages.csv rows of WP-19) located in the decompiled C#
(Assembly-CSharp/<File>.cs, member line ranges) and matched against every `<File>.cs:<line>` cite in core/src and core/include; each
uncited or partial member read in the C# and classified. Baseline fresh build at main `839f2f5f`: see Step 0 below.

| spec file (lines) | cited members / all | not yet ported | class | plan |
|---|---|---|---|---|
| VesselRetrieval.cs (93) | 0 / 7 | whole class: Awake / OnDestroy (OnVesselRecoveryRequested, SceneManager.sceneLoaded), OnSceneLoaded -> OnLevelLoaded coroutine (SPACECENTER, KSCFrameDelay = 8 frames), onVesselRecoveryRequested (id set, SaveGame persistent OVERWRITE, LoadScene SPACECENTER), recoverVessels, getVesselsToRecover, recoverVessel (onVesselRecovered(pv, false), DestroyImmediate) | engine-agnostic | port all; funds / science / reputation are the onVesselRecovered listeners (WP-36 VesselRecovery / Funding / Reputation / R&D): gate through them |
| QuickSaveLoad.cs (494) | 3 / 24 (naming only) | Awake / OnDestroy (fetch singleton), Update (QUICKSAVE / QUICKLOAD keys, input locks, CanQuickLoad, holdToLoad Invoke / CancelInvoke, modifier+QUICKLOAD dialog), onHoldComplete, QuickSave, quickSave, QuickSaveClearToSave, doSave, quickLoad (ROC cache, LoadSFSFile, mission branch, upgrade pipeline), onQuickloadPipelineFailed / Finished, SpawnSaveAsDialog / ConfirmDialog / OnSaveAsClose / SpawnQuickLoadDialog / OnLoadClose decisions, CacheLocalStrings | engine-agnostic except the dialogs | port all decisions; the PopupDialog / LoadGameDialog / MenuNavigation / TMP input objects, Escape / Return dialog keys, UISkin are UI: seams (dialog spawn = hook, the user's answer = a call) |
| VesselSwitching.cs (130) | 3 / 3 | none in the class; the branch it reaches in FlightGlobals.setActiveVessel for an UNLOADED target (save persistent + FlightDriver.StartAndFocusVessel) is a seam with no default | engine-agnostic | default the two seams to the real GamePersistence.SaveGame(..., FLIGHT) and FlightDriver.StartAndFocusVessel(CurrentGame.Updated(), index) |
| FlightState.cs (192) | 4 / 5 | `Load()` (protoVessels Load, Planetarium.SetUniversalTime, MapViewFiltering.LoadFilterState); the live half of `FlightState()` (DEAD skip, DECLUTTER_KSC auto-clean of KSC / launch-site debris, MAX_VESSELS_BUDGET, BackupVessel in reverse, activeVesselIdx = IndexOf(ActiveVessel.protoVessel)) | engine-agnostic | port both; the live constructor lives on the vessel side (src/vessel/flight_state_live.cpp) |
| VesselRanges.cs (159) | 5 / 9 | the constructors (field initialisers are there; the copy constructors are C++ value copies, not cited) | engine-agnostic | cite / make explicit (behaviour already equal) |
| ProtoVessel.cs (1,262) | 22 / 27 | `GetSituationString` (994), `Clean` (1163: recovery on the home world with the funds / science / reputation deltas, removal from the flight state, the log text), `CurrentFunds / CurrentScience / CurrentReputation`, `CacheLocalStrings`; ProtoVessel(Vessel): patchedConicSolver.Save into flightPlanNode when PatchedConicsAttached (Vessel.BackupVessel too); ProtoVessel.Load: Comet VFX | engine-agnostic except Comet VFX (Unity prefab / map object) | port all but the Comet VFX controller (excluded: AssetBase prefab, ScaledMovement); Vessel.Clean calls ProtoVessel.Clean (the WP-14 `cleanVessel` seam stays as an override) |
| ProtoPartSnapshot.cs (1,312) | 19 / 23 | `partPrefab` getter, `LoadEVAInfoBySuitGender` + the `vesselRef.isEVA` branch of Load, `FindAttachNode` (private, unused by the C#), Add / RemoveProtoPartModuleAdjusters (ported by WP-55 in missions/mission_actions3.cpp with a placeholder cite) | engine-agnostic | port the getter, the EVA branch, FindAttachNode; re-cite the adjuster helpers |
| FlightGlobals.cs (2,003) | 94 / 113 | debug orbit setters SetShipOrbit / SetShipOrbitRendezvous / PrepForOrbitSet / PostOrbitSet / SetVesselPosition x2 / OnGameUnpaused / ActiveVesselGoOffRails; getFoR / GetFoR x3 and the navball block of UpdateInformation (ship_orientation_offset, ship_orientation, ship_heading, lastVel); clearInverseRotation (instance), HookVesselEvents / UnhookVesselEvents (ported inside Awake / OnDestroy without cite); GetPartUpwardsCached / GetPartPointerUpwardsCached / Reset*Cache (GameObject -> Part component cache) | engine-agnostic except the GameObject caches | port the setters (Vessel.KillPermanentGroundContact / ResetGroundContact needed: port them too), getFoR, the navball block (approximate: Unity-native Quaternion.LookRotation / Slerp / eulerAngles of quaternion_native.hpp, UNVERIFIED against the game); exclude the GameObject caches (Unity scene graph, no kspcore equivalent) |
| OrbitPhysicsManager.cs (353) | 14 / 16 | ToggleRotatingFrame ([ContextMenu]: Unity-editor debug menu), the DominantBody getter is ported | excluded (editor-only) | none |
| VesselPrecalculate.cs (525) | 18 / 19 | CacheLocalStrings: the easing screen message text (#autoLOC_6003101) | engine-agnostic | add the message text next to easingMessageSeconds |

Also needed by the quickload chain (Game.cs is WP-04's file, but its live half is only reached from here): `Game.Load()` (HighLogic.CurrentGame =
this, ScenarioRunner.SetProtoModules, CrewRoster.Init, flightState.Load when the scene has a Planetarium, onGameStateLoad) and the
FlightDriver.Start statements that call it (Planetarium.SetUniversalTime / UpdateCBs, FlightStateCache.Load()), which WP-42 left as a comment.
GamePersistence.SaveGame(name, folder, mode[, scene]) overloads (GamePersistence.cs:9 / :20) are missing too (VesselRetrieval and
FlightGlobals.setActiveVessel call them).

### Plan (order of the commits)

1. this audit.
2. FlightState live constructor + Load, Game.Load, GamePersistence.SaveGame overloads, FlightDriver.Start loading the cache.
3. QuickSaveLoad (save / load orchestration, keys, dialogs as seams).
4. VesselRetrieval; ProtoVessel.Clean / GetSituationString / Current*; Vessel.Clean.
5. FlightGlobals debug setters (+ Vessel.KillPermanentGroundContact / ResetGroundContact), getFoR / navball, the unloaded-switch defaults; ProtoPartSnapshot EVA branch; VesselRanges / VesselPrecalculate small items; ProtoVessel(Vessel) flight plan.
6. Gates: corpus.saves (61 .sfs byte-identical, run against the main checkout's decompiled/KSP-dev), quicksave -> quickload -> save byte-identical over the harness saves, FlightState round trip (live capture -> Save -> Load -> live capture), vessel switching / retrieval / Clean on the physprofile save's vessels against the C# rules, an independent Python reference (tools/wp19c) for the numeric parts (debug setters, navball / FoR, GetSituationString table).
7. STATUS / README / progress.csv, fresh suite, seeds 11 / 27, WSL g++, pr_review, PR.

## Step 0: baseline (main 839f2f5f, fresh, this worktree)

`run_tests.ps1 -Fresh` (CMAKE_BUILD_PARALLEL_LEVEL=4): configure 3.1 s, build 417.5 s, ctest 175.1 s, 52 / 52 pass; `kspcore_tests` 1,094 cases /
1,694,282 assertions.

## Step 2-5: ports (commit 0d10d5b9)

FlightState() live capture + Load (src/vessel/flight_state_live.cpp), FlightState.protoVessels shared (ProtoVessel / Game
enable_shared_from_this; Vessel.StartFromBackup shares the snapshot; ShipConstruction's TakeOut follows), Game.Load / Start
(src/save/game_live.cpp), FlightDriver.Start loads its cache and defaults its seams to FlightGlobals, GamePersistence.SaveGame(name, folder,
mode[, scene]), HighLogic::currentGameOwner, QuickSaveLoad (src/save/quick_save_load.cpp), VesselRetrieval, ProtoVessel.Clean /
GetSituationString / Current*, Vessel.Clean -> ProtoVessel.Clean, FlightGlobals debug setters + OnGameUnpaused + the unloaded-switch defaults
+ navball block + disableOverride Invoke list (CollisionEnhancer.bypass now cleared as in the C#), Vessel.KillPermanentGroundContact /
ResetGroundContact / LandedInStockLaunchSite, ProtoPartSnapshot EVA part lookup / partPrefab / FindAttachNode, the flight plan of
ProtoVessel(Vessel) / BackupVessel, VesselRanges constructors, VesselPrecalculate easing text. Whole unit binary after the ports: 1,094 /
1,694,282, all pass (no existing number moved).

## Step 6: gates (commits 87b28824, 554f347a)

- tests/test_wp19c_quicksave.cpp: physprofile through FlightDriver.Start (RESUME_SAVED_FILE) -> F5 -> F9 -> new flight scene -> F5: Q2 == Q1
  byte for byte; original vs Q1: exactly 12 documented lines (scene 5 -> 7, the stand-in FlightCamera's cPch / cHdg / cMod, ModuleReactionWheel
  stateString and CommNetVessel controlState / canComm that only the physics tick / CommNet produce); one Backup file.
- tests/test_wp19c_registry.cpp vs tools/wp19c/registry_reference.py (tests/data/wp19c_reference.json, sha256 see STATUS): FlightState capture
  400 scenes (1,368 vessels, 80 DECLUTTER cleans, 209 budget cuts), Clean 300 (236 recoveries, 162 refund texts), situation strings 300,
  EVA table 32, QuickSaveClearToSave 16, QuickSaveLoad.Update 400 frames.
- tests/test_wp19c_flows.cpp: VesselRetrieval on physprofile, FlightState round trip (+ Serenity map-filter bit), unloaded switch -> new scene,
  save-as dialog, quickLoad branches (no file / KSC save / 0.20 incompatible / 0.23.5 / hold-to-load / load dialog / mission), Game.Start,
  debug setters.
- WP-19c cases: 14 / 10,105 assertions, all pass.
- corpus: save_corpus over decompiled/KSP-dev (main checkout): 60 KSP-1.12.5 .sfs byte-identical once mod-owned elements are dropped, 44 older
  documented, 201 crafts, 0 unexplained, PASS; over testdata/saves (physprofile): R1 byte-identical. = the 61 .sfs of WP-04, unchanged.
- pr_review: 0 failing checks (cite aliases for Awake / OnDestroy constructors and the dialog delegates added to tools/wp28/check_cites.py);
  check_cites exact over the 6 new .cpp: 50 cites, 0 mismatches.

## Step 7: verification (head ed53b588)

Fresh: configure 2.5 s, build 306.9 s, ctest 155.7 s, 52 / 52. Random orders -rs=11 / -rs=27: 1,108 cases / 1,704,390 assertions, all pass
(the quicksave gate was order-dependent through the shared vessel-module registry: the fixture now compiles it, CometVessel off). WSL g++
-fsyntax-only clean on the 20 changed files; pr_review 0 failing. origin/main still 839f2f5f: no merge needed.
