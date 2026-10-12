# PROGRESS wp54c (Making History mission runtime, slice 3) - branch wp54c, worktree core/.worktrees/wp54c, base main b9119103

Build: `export CMAKE_BUILD_PARALLEL_LEVEL=6; powershell -NoProfile -ExecutionPolicy Bypass -File run_tests.ps1 -BuildOnly` from the worktree core dir
(helper: C:/Users/pmavr/AppData/Local/Temp/claude/D--Projects-KSP-UE5/9035e596-1691-418f-908a-4d9deca60056/scratchpad/b.sh builds, prints errors).
Unit binary: `build/RelWithDebInfo/tests/kspcore_tests.exe` (baseline 810 cases / 1,366,529 assertions; `-tc="*WP-54c*"` etc.).
Python: `python -I` (not python3). Regenerate the gate: `python -I tools/wp54c/ref54c.py` -> tests/data/wp54c_reference.json.
Cites: write `// File.cs:@Name[#k]` placeholders then `python -I tools/wp55/check_cites.py --resolve --cs-root D:/Projects/KSP-UE5/decompiled/KSP-Decompiled/Assembly-CSharp file.cpp`, then the check without --resolve.
TOOLING TRAPS: heredocs through the Bash tool collapse `\\` to `\` and break `\n` inside C++/python string literals: use the Write/Edit tools for anything with backslashes;
python text-mode writes on Windows produce CRLF (use newline=''); `from ref_common import *` overwrites OUT (define OUT after it).

## State (all committed on branch wp54c)
- [x] test world moved verbatim from test_missions2.cpp into tests/missions2_world.hpp (namespace ksp::mtest54b; RunTestModuleCases helper; wp54b gate unchanged 4 cases / 40,219 assertions)
- [x] A Mission craft-file bookkeeping + vessel/part event handlers + ProcessMappedVesselNodeMap/RemapVesselId: src/missions/mission_craft.cpp; flag MissionsEnvironment::craftFileBookkeeping (default false keeps WP-54 gates bit-identical);
      seams MissionDataEnvironment::{directoryExists,getFiles,objectHashCode}, MissionsEnvironment::refreshMissionRequested. GATED (kinds craft 150, nodemap 60).
- [x] B ActionPartFailure/Repair/ResourceDrain: mission_actions3.{hpp,cpp}; NodeLibraryEnvironment seams; ProtoPartSnapshot::ProtoPartRepair, ProtoPartModuleSnapshot::Add/RemovePartModuleAdjusterNode;
      DynamicModuleList::Clone concrete; TestModule::UpdateNodeBodyUI throws NRE on null guiNode. GATED (kind pactions 360).
- [x] C TestDistance: mission_tests4.{hpp,cpp}, mission_system.hpp (static missions list only). GATED (kind distance 320).
- [x] D MissionScoreInfo / MissionLocalizer / MissionImporting / MissionEditorHistory: mission_extras.{hpp,cpp}. GATED (scoreinfo 60, localizer 40, importing 120, history 80).
- [x] E GetHashCode overrides (mission_hash.{hpp,cpp}): 11 classes. GATED (hash 338).
- [x] F real BaseAPField + BaseAPFieldList (mission_display.hpp block, mission_apfield.{hpp,cpp}); stand-in members kept. GATED (apfield 250).
- [-] G PositionMobileLaunchPad - DEFERRED (Transform/Collider/Raycast hierarchy; needs a scene-graph seam layer, not engine-agnostic)
- [-] H MissionSystem - DEFERRED (only the static `missions` list exists)
- [-] I ActionCreateVessel scene branch + validateCraft - DEFERRED (hooks exist, unchanged)
- [x] registry accounting DONE (tools/wp55/node_library_reference.py: deferred names -> ported; tests/test_node_library.cpp expectations; tools/wp55/ref_*.py deferred sets), RegisterNodeLibrary already registers TestDistance + 3 actions
- [x] DONE check_cites exact over all new/changed .cpp (placeholders `File.cs:@Name` are in the new files: resolve them), STATUS section + row 48 + unit row, README rows
- [x] full run DONE (819 / 1,421,365; fresh build ctest 38/38; ASan see STATUS): unit binary default order + --order-by=rand -rs=11 and -rs=27, ctest (run_tests.ps1), fresh build for final verification, ASan/UBSan in WSL (README 'Sanitizer run')

## Test files
tests/test_missions3.cpp (all WP-54c gates; Ref3() reads tests/data/wp54c_reference.json), tests/missions2_world.hpp (shared Fixture54b/Scene54/World54).
Debug aids: KSP_DBG=1 prints per-case/op lines in the runners.

## Findings (C# behaviour kept)
- ActionPartResourceDrain: when Setup() fails the C# continues (yield return null, setup = true) into the drain loop; resourcedef null -> NullReferenceException at `resourcedef.id` in the loaded-part/vessel branches (ported as a throw); GetNodeBodyParameterString("resourceName") sets resourcedef as a side effect.
- ProtoPartModuleSnapshot adjuster nodes use `TryGetNode` references + `SetNode(name, node, true)`: SetNode clears the target and copies `node` into it, so when node IS the target (ADJUSTERS already present when adding, ADJUSTERSTOREMOVE already present in ProtoPartModuleRepair) the node ends up EMPTY (adding a second adjuster to a module that already has ADJUSTERS wipes it). RemovePartModuleAdjuster(Guid) stores node2 under the name ADJUSTERS (sic). The reference reproduces all of it (CfgR with reference semantics).
- ActionPartRepair.Save calls vesselPartIDs.Load(node) after Save; TestDistance.Load tests configNode2 for the DISTANCETOLOCATION load (NRE if only DISTANCETOKERBAL is present); TestDistance Vessel body text passes distanceFromVesselID to UpdateFromMappedVesselIDs; RunValidation `distanceFromKerbal == null` never true; GetNodeBodyParameterString NRE branches.
- Mission.OnDestroy does not remove onVesselDocking / onVesselsUndocking (C# leak); the C++ destructor removes all seven handlers so a dead Mission is never called.
- GetVesselSituationByVesselID finds the node of the situation then the FIRST ActionCreateVessel of that node (two vessel actions on one node always resolve to the first).
- Mission.PrintObjectives of an empty mission returns the untranslated tag "#autoLOC_8100310".
- MissionScoreInfo.AddScore stores `mission.awards.awardedAwards` itself (aliasing not reproduced: copy); List.Sort is not stable in Mono (ties unverified; scenarios keep (score, time) unique); ScoreInfo.Load does not unescape resultsText (Save escapes).
- BaseFieldList.GetValue/SetValue(name, ...) go through BaseField<K>.GetValue/SetValue(host) (no onValueChange, no validation); the property path of BaseAPField.SetValue swallows onValueChange exceptions, the field path lets them escape.
- tools/wp54/mission_reference.py dictionary loader now also reads `#autoLoc_` keys (the dictionary has `#autoLoc_6002179`); wp54/wp54b/wp55 references regenerate byte-identically.

## Unverified / needs data
- Double/Single.GetHashCode of -0.0 (Mono corlib vs .NET 4.x referencesource), Unity Object / reference hash codes (seam), Directory.GetFiles ordering (sorted), mappedVesselKeys aliasing with mappedVessels (copies; not observable).
- Location target of TestDistance and the node-label world position use CelestialBody.GetWorldSurfacePosition (double libm cos/sin): booleans compared with a 1e-6 margin.
