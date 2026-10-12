# PROGRESS wp54b (Making History mission runtime, slice 2) - branch wp54b, worktree core/.worktrees/wp54b, base wp55 283a712c

Build: `export CMAKE_BUILD_PARALLEL_LEVEL=6; powershell -NoProfile -ExecutionPolicy Bypass -File run_tests.ps1 -BuildOnly` from the worktree core dir (python = `python`, not python3).

## State: DONE (see STATUS.md "WP-54b")
- Ported + gated: mission data classes, VesselRestriction*, VesselSituation, 20 Tests (+ base), ActionCreate{Vessel,Kerbal,Asteroid,Comet,Flag,LaunchSite} (Asteroid / Comet / Kerbal / Flag gated).
- Gate: tools/wp54b/*.py -> tests/data/wp54b_reference.json (1,250 cases), tests/test_missions2.cpp (4 cases / 40,149 assertions, all pass).
- check_cites exact: 565 cites, 0 mismatches.
- origin/main 3abcff94 merged (clean); fresh build + ctest 34/34; unit 769 cases / 1,332,750 assertions.

## Deferred (STATUS lists them)
TestDistance, ActionPartFailure / PartRepair / PartResourceDrain, MissionSystem, MissionImporting, MissionEditorHistory + real BaseAPField, PositionMobileLaunchPad, MissionScoreInfo, MissionLocalizer, craft-file bookkeeping,
ActionCreateVessel scene branch + craft validation, proto nodes of Asteroid / Comet / Kerbal (ungated), event logic of Kerbal EVA / Boarded / TakeKerbal (ungated).
