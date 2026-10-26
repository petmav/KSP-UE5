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

## Step 0: baseline

(filled below)
