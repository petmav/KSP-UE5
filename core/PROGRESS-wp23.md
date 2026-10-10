# PROGRESS WP-23 (engines, RCS, gimbal, intakes; numeric), branch wp23 from main 289d0c98

## Findings (step 0: reading)
- Golden harness/golden/engine_thrust: tables engines (41), isp_thrust_vs_pressure (738), kerbin_flight (4059), rocket_equation (410). Generator EnginesGen.cs (re-hosts OnLoad maxFuelFlow etc).
- Capture deltav_readout.json `engines` (460 rows): ispVac/ASL/Actual, thrustVac/ASL/Actual, requiresAir, propellants = DeltaVEngineInfo (WP-17) values computed from ModuleEngines editor branch (CalculateISP :610, CalculateThrustVector :687, CalcThrustActual :535). One entry per part (VesselDeltaV.cs:1127); MultiModeEngine picks runningPrimary engine.
- Infra: PartModule Get/Set; ModuleRegistrar; Part::Instantiate/LoadModule/InitializeModules in tools/support/craft_tree.hpp; Propellant struct in parts/part_module_data.hpp (extend: currentRequirement, actualTotalAvailable, ...).
- module_behaviours.cpp has stand-ins: ModuleGimbal, ModuleResourceIntake, MultiModeEngine, RegCurves(ModuleEngines...), module_staging.cpp (IsStageable ModuleEngines/RCS/Gimbal, OnStart stagingIcon)
## Plan / status
- [x] baseline build ok (93 s). Plan files: include/ksp/engines/engine_interfaces.hpp (done), include/ksp/modules/module_engines.hpp + src/modules/module_engines.cpp (ModuleEngines, ModuleEnginesFX, MultiModeEngine), module_rcs, module_gimbal, module_resource_intake; Propellant extension in part_module_data.hpp; gates ds_engine_thrust.cpp (golden, NetFloat) + ds_engine_capture.cpp (capture, MonoFloat)
- design: policy-templated <P> statics for float maths (GetEngineThrust, MaxThrustOutputAtm, OnLoad flows); prefab modules from GameData used by the golden runner; thrustTransforms are PartTransform vector fed by a host hook (model reader)
- [x] engines/rcs/gimbal/intake sources written + compile (Windows). IAirstreamShield dup fixed in aero_interfaces.hpp. next: run existing ctest, then ds_engine_thrust + ds_engine_capture + tests
- [x] ds_engine_thrust (golden.engine_thrust 0 ULP all 4 tables, NetFloat) + ds_engine_deltav (capture.engine_deltav, 460 rows, 0 ULP, MonoFloat) pass on Windows. next: tests/test_engines.cpp, docs, citation check, linux run
- [x] tests/test_engines.cpp (10 cases) pass; Windows full run 30/30 (before citation fixes). TODO: citation check script + fix, docs (STATUS WP-23, README rows, header counts), linux run (wsl), cleanup logs, commit
- [x] docs written (STATUS WP-23, README rows, header counts), citations fixed. TODO: Windows -Fresh, WSL run_tests.sh -f, commit
