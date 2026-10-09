# PROGRESS WP-16 (staging, action groups, decouplers, decouple/couple tree ops)

Branch wp16 (from wp14 7bc606d6). Notes after every major step.

## Findings (reading)
- C5 capture deltav_readout.json `parts` table: `inverseStage` = part.inverseStage AFTER the editor's load rebuilds the indices
  (StageManager.SetSeparationIndices -> setSeparationIndices: non-staging parts take carriedInverseStage). 14 stock crafts: equals file istg;
  PhysProf-S/M2/L2-AS: differs from file istg (non-icon parts under a decoupler take the decoupler's stage). Also `craft.stage_count`,
  `stages.separationIndex` (DeltaVStageInfo, WP-17), `parts.decoupleStage/activationStage` (DeltaVPartInfo, WP-17).
- StageGroup/StageIcon/ProtoStageIcon are WP-43 (UI): modelled as plain data records here.
- Part.Actions (Part's own KSPActions) are not modelled by WP-13; BaseAction.CreateActionList is mine.
- Baseline build log: /tmp/claude-0/wp16/baseline_build.log
- baseline (wp14 tip) ctest 22/22 in 50 s
- written (not yet compiled): base_event.hpp KSPActionParam/KSPActionType/BaseAction statics (modules/base_action_lists.cpp), ActionGroupList::ToggleGroup/SetGroup,
  tools/wp16/gen_stageable.py -> include/ksp/modules/stageable_classes.inc, PartModule staging queries (modules/module_staging.cpp), Part staging hooks
  (part.hpp/part.cpp), staging/stage_manager.{hpp,cpp} (StageManager+StageGroup+StageIcon data model), modules/module_decouplers.{hpp,cpp}.
- TODO: parts/part_decouple.cpp (decouple/Couple/Undock/disconnect/SetVessel/CheckBodyLiftAttachment/StartStaging), vessel/vessel_staging.cpp
  (ClearStaging/ResumeStaging/Spawned vessels/part ownership move), docked_vessel_info.hpp, tests, dataset, STATUS/README.
- library compiles (stage manager, decouplers, part_decouple, vessel_staging). Next: unit test + gate + fix runs.
- gate capture.staging_sequence PASSES: 1225 inverseStage + 17 stage_count + 1225 isDecoupler (clamps null). Needed: OnStart stagingIcon defaults (module_staging.cpp).
- tests/test_staging.cpp chunks in /tmp/claude-0/wp16/parts (00_head, 01_actions done+passing); concatenated into tests/test_staging.cpp. Next: 02 modules, 03 stock crafts, 04 sortIcons, 05 flight/decouple, 06 docked info
- test chunks 00-03 pass (actions, modules, stock-craft sequence 51 crafts: icon istg 470/470, dstg 2403, sepI 120/122 (2 Osprey hardpoints), capture decoupleStage 1001 rows). Bug fixed: EditorSession root = part without parent. Next: 04 FindStageIndices+SortIcons rules, 05 flight/decouple/couple/undock, 06 DockedVesselInfo, then STATUS/README/fresh run
- chunks 04 (FindStageIndices etc.), 05 (SortIcons, groups, placement) pass. Fixed AvailablePart::preferredStage default 0->3 (AvailablePart.cs:114). Next: 06 flight activation/decouple/couple/undock, 07 DockedVesselInfo; STATUS/README; fresh run
- chunk 06 flight/decouple/couple/undock/DockedVesselInfo passing (14 WP-16 cases). Next: more tests (module OnDecouple omni, ToggleStaging, Vessel staging misc), full ctest, STATUS/README, commit
