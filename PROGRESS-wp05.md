# PROGRESS-wp05 (WP-05 UrlDir, GameDatabase data, PartLoader data, resource library)

Worktree: core/.worktrees/wp05, branch wp05 from main f964447. Commit only here.

## Inputs read
- README, STATUS (WP-01/02 UrlDir deferred; WP-03 Localizer/GameEvents), work_packages.csv WP-05 rows, captures README C9.
- Capture code: decompiled/KSP-Decompiled-profiling/Assembly-CSharp/PhysProf/HarnessCapture2.cs:423-870 (C9), Fmt 249, Num 343.
- part_database.json tables: parts(362), part_cfg_values(6235), modules(1378), module_fields(34801),
  module_propellants(77), resources(166), attach_nodes(827), drag_cubes(535), variants(228), assets(3978).
- 9 parts without cfg: kerbalEVA, kerbalEVAfemale, flag, kerbalEVASlimSuit(+Female) (code-built), 4 DLC suits (error rows).
- partConfig values in capture exclude name/title/cost/... (ParsePart removes them?) -> check.

## Plan / status
(steps below updated as work proceeds)

## Key spec findings (read so far)
- UrlDir.cs: dirs skip .svn/PluginData/zDeprecated; ext case-sensitive; cfg->Config; others via loader table
  (Assembly dll; Audio wav,ogg; Texture dds,jpg,jpeg,mbm,png,tga,truecolor; Model mu,dae,DAE). GameData dir urlRoot "".
  Parts-type dirs only for legacy Parts/Internals (empty in KSP-dev). Order: NTFS order (sort upper-case ordinal).
- GameDatabase.cs:942 CreateDatabase: LoadObjects (UrlDir, Localizer.Init, translateLoadedNodes=TranslateBranch every config,
  Audio/Texture/Model tables in GetFiles order; duplicates replace (lastLoadTime=0001)), asset bundles (*.ksp, skip),
  then CompileConfig ("config" clone values) for AllConfigs, PartResourceLibrary.LoadDefinitions, PhysicMaterialLibrary,
  PartUpgradeManager.FillUpgrades.
- Oracle: decompiled/logs/ksp-capture-capture2-final2-KSP.log has "Config(TYPE) url", "Load(Texture|Model|Audio): url"
  in load order (1236 tex, 487 models, 51 audio, 358 PART). Capture parts order == log PART order.
- PartLoader.cs:161 CompileAll: 9 code-built parts first (Unity prefabs; idx kerbalEVA0 female1 flag2 Vintage3 femVintage4
  Future5 femFuture6 SlimSuit7 SlimSuitFemale8); PART cfgs named like these are applied as overlay (ApplyPartValue, MODULE
  __OVERLOAD/AddModule). Prebuilt cfgs exist for kerbalEVA, female, flag, SlimSuit x2; DLC 4 have none -> prefab never
  Awake'd -> error rows (NRE).
- ParsePart PartLoader.cs:642; ApplyPartValue :985 (double parsed with float.Parse: sic); CompilePartInfo :1169 (seat mass
  reduction; flag=any module is ModuleCommand); InferBulkheadProfiles :1581 (not called in ParsePart?) ; CompileModel :1765.
- LoadObjectFromConfig(availablePart, node, removeAfterUse) removes AvailablePart [Persistent] keys from partConfig.
- DragCubeList.LoadCubes reads cube values LAST->FIRST; ClearCubes resets procedural not none; DragCube.Load 24 values
  throws IndexOutOfRange (quirk). PartDatabase.cfg at game root; GetNode("PART","url",partUrl) top-level only.
- module_fields: only ~5,187 of 34,801 rows are cfg-present; rest are KSPField defaults (WP-06 class metadata).

## Plan
1. io::UrlDir + GameDatabase data + Localizer hookup  [done: src/io/url_dir.cpp, src/database/game_database.cpp;
   VERIFIED 2836/2836 load-log lines (Load(Audio|Texture|Model), Config(TYPE) url) == KSP.log order; runner has --gamedata]
2. PartResourceLibrary [code written: src/resources/part_resource_library.cpp, MonoStringHash ids]; PhysicMaterialLibrary,
   PartUpgrade data [ ]
   Generators: tools/wp05/gen_part_fields.py -> include/ksp/parts/{part_fields,compound_part_fields,part_enums}.inc;
   tools/wp05/gen_module_schema.py -> include/ksp/parts/module_schema.inc (151 PartModule classes, 1595 KSPField members;
   VERIFIED vs capture modules: 1378/1378 field sets, field_count, Type.Name, isPersistant all equal; checker
   build/scratch/check_schema.py). src/parts/ksp_enums.cpp (EnumTable Parse/ToString).
3. PartLoader data: Part fields table (gen from Part.cs), ApplyPartValue, attach nodes, drag cubes, resources, modules (raw +
   typed cfg fields via generated KSPField schema), variants, internal, assets [ ]
4. golden runner ds_part_database + gate (out2 capture) + filter for documented out-of-scope rows [ ]
5. tests/test_partloader.cpp [ ]
6. stretch: .mu metadata reader (texture slots, partSize, variant materials) [ ]
7. STATUS/README, commit [ ]
