# PROGRESS WP-06 (module host)

Branch wp06. Plan:
1. [ ] measure gap (estimate_module_defaults with scope.json) 
2. [ ] generator extension: KSPField attribute args (guiName etc), UI_Control, KSPEvent, KSPAction -> module_members.inc
3. [ ] host headers/sources: include/ksp/modules/*.hpp, src/modules/*.cpp
4. [ ] initializer evaluator, instantiate through PartLoader, runner emits all fields
5. [ ] exclusions + gate widen, module_mass/cost
6. [ ] VesselModule/ScenarioModule data model, unit tests
7. [ ] STATUS/README, fresh run
