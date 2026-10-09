# PROGRESS WP-06 (module host)

Branch wp06 (from origin/main f50d6121).

DONE
- [x] gap measured: 27,391 initialiser rows + 5,401 cfg rows + ~500 module logic rows
- [x] generators: tools/wp05/gen_module_schema.py extended (host_schema.inc for ScenarioModule/VesselModule trees; module_schema.inc byte-identical),
      tools/wp06/gen_module_members.py -> include/ksp/modules/module_members.inc (KSPField/UI attrs, KSPEvent, KSPAction, UI_* classes)
- [x] host: include/ksp/modules/{cs_expr,module_members,base_field,base_event,part_module}.hpp + src/modules/*.cpp
- [x] PartLoader::AddModule creates module (AddComponent+Awake) and Load() through BaseFieldList; runner emits all KSPFields
- [x] module behaviours (module_behaviours.cpp): OnAwake/OnLoad that change KSPFields + IPartMass/CostModifier implementations
- [x] gate: capture.part_database module_fields 32,911/32,911 rows gated minus 18 excluded (model pivot rotations); module_mass/module_cost un-dropped (357 rows, 0 ULP)
- [x] full suite 21/21 green (non fresh)

TODO
- [ ] VesselModule / VesselModuleManager / ScenarioRunner / ScenarioModule field integration (host_schema), IScalarModule etc
- [ ] unit tests tests/test_modules.cpp (cs_expr, BaseField, BaseEventList, PartModule Load/Save, PartModuleList, VesselModule, ScenarioRunner)
- [ ] README rows, STATUS WP-06, tests/CMakeLists comment, estimate_module_defaults.py note
- [ ] fresh ./run_tests.sh -f; commits
