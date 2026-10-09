# C9 part_database summary

- ksp_version: `1.12.5 build 03190`
- unity_version: `2019.4.18f1`
- runtime: `Mono 5.11.0 (Visual Studio built mono) (Environment.Version 4.0.30319.42000)`
- assembly_csharp_sha256: `42d4b00db940c91eecf5d75ad1d95a4632c80f6322f2aad753f246b9335624cd`
- assembly_csharp_jit: `IsJITOptimizerDisabled=False`
- profiling_branch_commit: `36ef51bd9d095f57904e29e7b98886d0fd753d1d`
- utc: `2026-10-09T00:05:30.9618627Z`

Parts: 362 (errors: 4). Tables: parts 362, part_cfg_values 6235, modules 1378, module_fields 34801, module_propellants 77, resources 166, attach_nodes 827, drag_cubes 535, variants 228, assets 3978.

Parts without a usable prefab: kerbalEVAVintage (NullReferenceException: Object reference not set to an instance of an object), kerbalEVAfemaleVintage (NullReferenceException: Object reference not set to an instance of an object), kerbalEVAFuture (NullReferenceException: Object reference not set to an instance of an object), kerbalEVAfemaleFuture (NullReferenceException: Object reference not set to an instance of an object)

## Drag cubes vs KSP-dev/PartDatabase.cfg

Compared 531 loaded cubes with the DRAG_CUBE text the game read: the part's own cfg DRAG_CUBE node when it has one (67 cubes), otherwise the KSP-dev/PartDatabase.cfg entry for the same partUrl and cube name (all 30 numbers, float32 bits): 531 bit-exact, 0 differ, 4 loaded cubes have no entry.

Note: KSP-dev/PartDatabase.cfg has a PART node nested inside another PART (MpoProbe inside FlagDecal Size5Flag); only top-level PART nodes are used by the game, and only those are compared.

## cfg values vs resolved Part/AvailablePart fields

Known transforms: (1) PartLoader.ApplyPartValue parses every double-typed Part field with float.Parse (the decompiled code tests typeof(double) twice), so e.g. emissiveConstant 0.7 loads as 0.699999988079071; (2) crewed parts get mass -= PhysicsGlobals.PerSeatReduction (PerCommandSeatReduction for command parts) * CrewCapacity.

Numeric top-level cfg values that map to a Part/AvailablePart field: 3069 equal to the parsed cfg text, 235 differ (loader transforms them).

Keys that differ (count): emissiveConstant 158, heatConductivity 60, mass 16, radiatorMax 1

| part | key | cfg | resolved |
|---|---|---|---|
| noseCone | emissiveConstant | 0.7 | 0.699999988079071 |
| airbrake1 | emissiveConstant | 0.4 | 0.40000000596046448 |
| airlinerCtrlSrf | heatConductivity | 0.12 | 0.11999999731779099 |
| airlinerCtrlSrf | emissiveConstant | 0.4 | 0.40000000596046448 |
| airlinerMainWing | heatConductivity | 0.12 | 0.11999999731779099 |
| airlinerMainWing | emissiveConstant | 0.4 | 0.40000000596046448 |
| airlinerTailFin | heatConductivity | 0.12 | 0.11999999731779099 |
| airlinerTailFin | emissiveConstant | 0.4 | 0.40000000596046448 |
| AdvancedCanard | emissiveConstant | 0.95 | 0.949999988079071 |
| CanardController | emissiveConstant | 0.95 | 0.949999988079071 |
| sweptWing | emissiveConstant | 0.95 | 0.949999988079071 |
| tailfin | emissiveConstant | 0.95 | 0.949999988079071 |
| basicFin | emissiveConstant | 0.95 | 0.949999988079071 |
| CircularIntake | emissiveConstant | 0.55 | 0.550000011920929 |
| shockConeIntake | emissiveConstant | 0.95 | 0.949999988079071 |

## Asset origins

| kind | source | rows |
|---|---|---|
| cfg | GameData | 353 |
| cfg | code | 5 |
| internal | internal | 16 |
| model | GameData | 359 |
| texture | GameData | 2497 |
| texture | unity_asset | 733 |
| texture_replacement | GameData | 15 |

| kind | file exists | rows |
|---|---|---|
| cfg | False | 5 |
| cfg | True | 353 |
| internal | False | 16 |
| model | True | 359 |
| texture | False | 733 |
| texture | True | 2497 |
| texture_replacement | True | 15 |

Unity-asset textures (not in GameData; they come from KSP_x64_Data): EVAjetpack (124), EVAjetpackNRM (124), EVAjetpackEmmisive (124), fairydust (70), smoke (45), expl_005 (38), backpack_Diff (20), backpack_NM (20), Default-Particle (16), canopy_Diff (12), canopy_NR (12), canopy_cutoff (12)
