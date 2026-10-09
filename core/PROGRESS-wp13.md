# PROGRESS WP-13 (part model and part tree)

Branch wp13 (from wp06 e27bc88b). Notes written after every major step.

- [ ] baseline fresh build/test of wp06 tip (log: /tmp/claude-0/wp13_baseline.log)
- Design: `class Part : public PartPrefab` (PartPrefab = prefab data record of WP-05; AvailablePart::partPrefab becomes unique_ptr<Part>);
  AttachNode gets owner/attachedPart; ModulePartVariants becomes a real PartModule owning variantList; PartPrefab::Modules renamed ModuleData (Part::Modules() is the PartModuleList).

## Step 1 (done): Part class + PartLoader on Part + ModulePartVariants + PartVariant
- new: include/ksp/parts/{part.hpp,part_types.hpp}, unity/bounds.hpp, modules/module_part_variants.hpp;
  src/parts/{part.cpp,part_modules.cpp,part_variant.cpp}, src/modules/module_part_variants.cpp
- PartLoader now builds `Part` (derives PartPrefab; ModuleData = old PartPrefab::Modules; srfAttachNode/baseVariant are shared_ptr);
  Part::AddModule/AddResource replace the loader's private copies; ModulePartVariants is a real PartModule (replaces module_behaviours' stand-in)
- capture.part_database still 0 ULP, unit/corpus green (build linux-rel, ~1 min incremental)
- TODO next: PartGeometryUtil, AxisGroupsManager, small modules (MirrorPlacement, DisplaceTweak, CompoundPartModule), FlagNameManager,
  AvailableVariantTheme, ProtoPartSnapshot<->Part, tests/test_part.cpp, craft trees, STATUS/README

## Step 2 (done): remaining ports + first tests
- PartGeometryUtil (+unity/bounds.hpp), AxisGroupsManager (PartModule Save/Load now call it), ModuleMirrorPlacement, ModuleDisplaceTweak, CompoundPartModule,
  FlagNameManager, AvailableVariantTheme, live ProtoPartSnapshot/ModuleSnapshot/ResourceSnapshot (src/save/proto_part_live.cpp)
- tests/test_part.cpp: enums/records, flag/theme, bounds, C9 cross-check of all 353 prefabs (counts, resource mass, module mass/cost), Instantiate + UpdateMass of all prefabs
- TODO: tests for modules/attach/tree/resources/crew/variants/copy, physprofile snapshot round trip, craft trees; citations check pass; STATUS/README
