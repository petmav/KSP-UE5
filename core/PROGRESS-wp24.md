# PROGRESS wp24 (command, SAS / autopilot, reaction wheels, control state)

- [x] read specs (ControlPoint, SteeringObject, ModuleCommand, ModuleSAS, ModuleReactionWheel, ModuleRemoteController, VesselAutopilot)
- existing: FlightCtrlState fully ported (WP-14), PIDclamp (mathfx.hpp), APSkillExtensions::AvailableAtLevel (WP-31), ModuleSchema fields for the 3 modules
- design: include/ksp/modules/control_types.hpp (ControlPoint, SteeringObject, ISteerable, ITorqueProvider),
  module_command.hpp (ModuleCommand, ModuleSAS, ModuleRemoteController, CommandEnvironment), module_reaction_wheel.hpp,
  include/ksp/vessel/vessel_autopilot.hpp (VesselAutopilot, VesselSAS); Vessel gets Autopilot() + OnAutopilotUpdate
- Unity native seam: Quaternion.eulerAngles (not ported) -> VesselSAS env hook + default implementation
- gate: no SAS golden/capture in harness (check MANIFEST). Required: capture.part_database 0 ULP, deltav_stages, ship_construct, unit tests + tools/wp24/sas_reference.py
- [x] code: control_types, module_command (ModuleCommand/SAS/RemoteController), module_reaction_wheel, vessel_autopilot; Vessel::Autopilot()/OnAutopilotUpdate; PartModule::AsTorqueProvider
- [x] builds (Windows), cites 0 mismatches; stand-ins replaced (ds_deltav_stages, test_kerbals)
- [ ] full run_tests (part_database gate etc.), [ ] sas_reference.py + tests/test_command.cpp, [ ] STATUS/README, [ ] WSL run
- [x] tools/wp24/sas_reference.py + tests/data/wp24_reference.json; tests/test_command.cpp (415 cases / 719,632 assertions green on Windows, SAS replay 37,400 values 0 ULP, wheel replay 9,595 values 0 ULP)
- [ ] STATUS/README, fresh run_tests.ps1, WSL run_tests.sh -f, merge main if moved, commit
- [x] STATUS.md WP-24 section + summary row 20, README rows (unit, headers, tools/wp24)
- [ ] fresh full run_tests.ps1, merge main if moved, WSL run, commit
