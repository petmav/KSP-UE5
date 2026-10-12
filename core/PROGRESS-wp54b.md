# PROGRESS wp54b (Making History mission runtime, slice 2) - branch wp54b, worktree core/.worktrees/wp54b, base wp55 283a712c

Build: `export CMAKE_BUILD_PARALLEL_LEVEL=6; powershell -NoProfile -ExecutionPolicy Bypass -File run_tests.ps1 -BuildOnly` from the worktree core dir (python = `python`, not python3).

## Done (committed locally, compiles)
- include/ksp/missions/mission_data.hpp + src/missions/mission_data.cpp: MissionQuaternion, MissionBiome, MissionOrbit, VesselGroundLocation, VesselLocation, LaunchSiteSituation (scene hooks),
  Crew (methods; struct lives in kerbals/crew_manifest.hpp), SurfaceArea, SurfaceVolume, Asteroid, Comet, MissionKerbal, ParamChoices_CelestialBodySurface / _VesselSimpleLocation, MissionCraft,
  MissionDataEnvironment seams; NodeLibraryEnvironment::Bodies().
- include/ksp/missions/vessel_restrictions.hpp + src: VesselRestriction (+8 subclasses), VesselRestrictionList, MissionPreflightCheck, MissionBlackListPreFlightCheck, RestrictionEnvironment.
- src/missions/vessel_situation.cpp: full VesselSituation (stand-in in mission.hpp replaced); Mission::UpdateFromMappedVesselIDs.
- mission_tests2.hpp/.cpp/mission_tests3.cpp: 19 deferred Tests registered via RegisterNodeLibrarySlice2Tests (TestDistance NOT ported).
- BaseAPField extended (fieldType, paramChoices, missionKerbal) and TestModule::GetNodeBodyParameterString uses them.
- MissionParamsGeneral/Extras/Facilities were already ported (save/game_parameters.hpp).

## TODO
1. Create* actions (Vessel, Kerbal, Asteroid, Comet, Flag, LaunchSite) in mission_actions2.hpp/.cpp (+ ActionCreateEnvironment seams), register, update DEFERRED accounting.
2. MissionScoreInfo + MissionLocalizer (small), Mission craft-file bookkeeping if time.
3. tools/wp54b/ Python reference (data classes, restrictions, tests, create actions) -> tests/data/wp54b_reference.json, tests/test_missions2.cpp; check_cites (tools/wp55/check_cites.py --resolve then exact, --cs-root D:/Projects/KSP-UE5/decompiled/KSP-Decompiled/Assembly-CSharp).
4. STATUS.md section + summary row 43 + README entry; merge origin/main; fresh verification (delete core/build, -Fresh); final commit; report.

## Deferred (say so in STATUS)
TestDistance, ActionPartFailure / PartRepair / PartResourceDrain, MissionSystem (1,809 lines scene host), MissionImporting (zip/popup UI), MissionEditorHistory + real BaseAPField (editor UI / attribute reflection), PositionMobileLaunchPad (scenery), createLaunchSiteObject scene body.
