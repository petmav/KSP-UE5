# PROGRESS WP-28 (branch wp28 from main 289d0c98)

## Scope facts (verified)
- 26 csv rows. Already done by WP-15: ConversionRecipe, ConverterResults, ResourceConverter, ResourceBroker.
- ModuleDeployableSolarPanel is WP-27 in the csv: NOT ported here (documented). ModuleAsteroid/ModuleComet are WP-09.
- Existing `BaseConverter` stand-in in module_behaviours.cpp (anon ns) must be replaced by the real class (game_events.hpp forward-declares ksp::BaseConverter).
## Plan
1. include/ksp/modules/resource_processing.hpp + src/modules/{converters,generators,scanners}.cpp
2. tools/wp28/ python reference for catch-up maths -> tests/data/wp28_*.json
3. tests/test_resource_processing.cpp
4. STATUS/README
## Steps done
- [x] read specs
- [x] header resource_processing.hpp, src/modules/resource_converters.cpp written (uncompiled). Next: generators.cpp (generator/alternator/jettison/drain), scanners.cpp, registration, remove stand-in BaseConverter in module_behaviours.cpp, ISpaceObjectMass in stand-ins, CMake
- [x] compiles (Windows). Next: run existing tests, CopySerializedFrom, tests, python ref
- [x] ported all 26 files (resource_processing.hpp, src/modules/resource_{converters,generators,scanners}.cpp); full ctest 28/28 green on Windows after port
- [x] tools/wp28/processing_reference.py + tests/data/wp28_processing_reference.json; tests: stock cfg sweep, catch-up vs python (48 scen/420 steps), SpaceObjectInfo vs python (40) all pass 0 diff
- KSP quirk found: first catch-up step runs with IsActivated=false (AlwaysActive set after GetDeltaTime) -> PrepareRecipe null -> no GetBestDeltaTime retry
- TODO: tests for harvester branches, drills, generator, alternator, jettison, drain, scanners, surveyor, biome, analysis, BaseConverter misc; STATUS + README; Linux run
- [x] fix found: PartModule::CopySerializedFrom did not copy resHandler lists (Instantiate): added ModuleResource::CloneSerialized (WP-15/06 file touch, documented in STATUS)
- [x] 17 WP-28 test cases pass (8571 assertions) on Windows; next: full ctest, STATUS/README, WSL run, commit
