# PROGRESS WP-25 (branch wp25 from main 95ed0a16)
## Facts
- No docking golden/capture in harness (MANIFEST.json has no dock entry; captures/out*: no docking tables). Gate = part_database module_fields 0 ULP + ship_construct/deltav_stages/staging_sequence stay green.
- No DOCKEDVESSEL node in core/testdata/saves (only physprofile). DockedVesselInfo exists already (include/ksp/parts/docked_vessel_info.hpp, WP-16).
- Seams (env hooks in ModuleDockingNode::DockingEnvironment): node pose, part velocity, torque, joints, rotation joint, neutral controls; reuse ModuleDecouplerBase::addForce/addForceAtPosition/forceSetActiveVessel.
## Plan
A module_docking_node.{hpp,cpp} | B module_grapple_node | C module_strut (CModuleStrut) | D replace stand-in in module_decouplers.cpp, runner ds_deltav_stages.cpp | E tools/wp25/dock_reference.py + tests/data/wp25_reference.json + tests/test_docking.cpp | F build/test | G STATUS/README
## Done
- [x] ModuleJettison flag hook (jettisonTransformParentIsPartRoot) real
- [x] A,B,C code written + compiles; stand-in removed; runner stand-in removed. Next: run gates, then tests+reference
- [x] tools/wp25/dock_reference.py + tests/data/wp25_reference.json (contact 1080+, acquire, matching). Next: tests/test_docking.cpp
- [x] tests/test_docking.cpp written (uncompiled), in tests/CMakeLists.txt
- [x] tests pass (15 cases). Next: check_cites, full run, STATUS/README, WSL
