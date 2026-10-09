# WP-11 PROGRESS (Opus implementer, 2026-10-09)

## Task
ksp::terrain: PQS height eval (GetSurfaceHeight + mod stack) for every body from prefab data; MapSO port;
LibNoise reachable subset; capture gate vs harness/captures/out2/terrain_height.json (h_override golden);
tests/test_terrain.cpp; STATUS.md WP-11 section; commit on wp11.

## Key facts
- Capture tables: bodies, mods(69), mod_params(3430), map_assets(8), scene_probe(4), samples(140522)
  samples cols: body,set,lat,lon,h_surface,h_override,terrain_alt (+_hex). Golden = h_override.
- Kerbin height mods: VertexHeightMap(10), VertexRidgedAltitudeCurve(16), VertexSimplexHeightAbsolute(20),
  VertexHeightNoiseVertHeightCurve2(32), MapDecalTangent(9999), MapDecal(99999), MapDecalTangent(99999),
  FlattenArea(99999), PQSLandControl(9999991)
- Mun height mods: VertexHeightMap(10), VoronoiCraters(100), FlattenArea(100)x2, VoronoiCraters(101),
  VertexSimplexHeight(102), VertexHeightNoiseVertHeight(103), PQSLandControl(999999)
- Kerbin pqs radius 600000, radiusMin 598015, radiusMax 608735; Mun 200000/199600/208300

## Status
- [ ] spec reading
- [ ] asset extraction tool + data/terrain
- [ ] mod stack extraction (gen) 
- [ ] C++ port
- [ ] gate
- [ ] tests, STATUS, commit

## Findings (step 1, spec + extraction)
- MapSO._data is serialized in the MapSO asset itself (no texture decode): extract bytes verbatim.
- tools/wp11/extract_terrain.py works (venv python -I; ~6 s). Scratch out: build/scratch/terrain_out. 18 maps 61 MB
  (exclude colorMap fields -> pyramidcolor). Prefab Kerbin 39 mods (runtime 46: 9 runtime PQSCity2 added, AltitudeAlpha
  modEnabled=0 and PQSROCControl removed), Mun 24 (PQSROCControl removed: modExpansionDisabled, no Serenity DLC).
- Prefab height-mod fields == captured mod_params (714 values) except OnSetup-computed/runtime-state ones. OK.
- Sort: List.Sort (introsort, unstable) by order -> use dotnet::ListSort on prefab GetChildMods list. Kerbin equal-order
  height mods (99999: MapDecal KSC2, MapDecalTangent KSC, FlattenArea Pyramids) are spatially disjoint. Mun order 100:
  VoronoiCraters, FlattenArea x2 order matters (capture: Voronoi, FA Monolith01, FA Monolith02).
- MapDecal(Tangent): `quadActive/buildHeight` private, default false -> applies. FlattenArea needs override or quadActive.
- MapDecal rot = native AngleAxis * native FromToRotation (need UnityNative::FromToRotation; 49 capture rows in
  harness/captures/out/math_shim_native.json fn=Quaternion.FromToRotation a0-a5 in, r0-r3 out). Quaternion*Vector3d
  resolves to Unity operator*(Quaternion, Vector3) (Vector3d->Vector3 implicit), managed Mono rule.
- radiusMin/Max = radius + sum(GetVertexMin/MaxHeight) (SetupBuildDelegates PQS.cs:2203). Kerbin 598015/608735 OK.
- requirements: OnSetup assigns; reqVertexMapCoods from union (UVSphereCoords also sets it).
- PQSROCControl.Awake: modExpansionDisabled = !Serenity installed.

## Step 2 (code) status
- DONE: data/terrain extracted (16 maps, 55 MB; pqs_prefab.json; manifest.json). include/src terrain/libnoise.*,
  terrain/map_so.*; UnityNative::FromToRotation added to unity/quaternion_native.* (variant 0 = 49/49 capture rows,
  scratch build/scratch/ftr.cpp; compile helper build/scratch/cl1.bat <file.cpp>).
- NEXT: include/ksp/terrain/pqs.hpp (PQS, VertexBuildData, PQSMod, PQ stub), pqs_mods.hpp (28 height classes +
  PQSModPassive), gen tool tools/wp11/gen_wp11_terrain.py -> src/terrain/{libnoise_random_vectors.inc,
  pqs_prefab_data.cpp, pqs_mod_fields.inc}; runner ds_terrain_height.cpp; tests/test_terrain.cpp.
- Non-stock mods VertexHeightMapStep / VertexHeightNoiseHeightMap sample a Texture2D (native GetPixelBilinear) ->
  pluggable sampler hook, throws if unset (deferred).
- VertexNoise sets Select.ControlModule after the ctor (fix header comment in libnoise.hpp).

## Step 3 (after rate-limit restart)
- DONE (written, not yet compiled): include/ksp/terrain/{pqs,pqs_mods}.hpp, src/terrain/{pqs,pqs_mods,pqs_mods_land}.cpp
  (all 28 height classes + PQSModPassive), tools/wp11/gen_wp11_terrain.py (writes libnoise_random_vectors.inc +
  pqs_prefab_data.cpp; validates every prefab field consumed or IGNORED; capture requirements cross-check).
- NEXT: include/ksp/terrain/pqs_prefab.hpp + src/terrain/pqs_prefab.cpp (MapLibrary, StockMapInfo, StockPQSInfo,
  CreateStockPQS, TerrainAltitude helpers); run gen; CMake; build; runner ds_terrain_height; gate; tests.
- GATE capture.terrain_height PASSED bit-exact (first run): samples 140,522 rows h_surface/h_override/terrain_alt
  max_ulp 0; bodies radii 0; scene_probe 0. 3 Mun h_surface!=h_override rows reproduced via scene-state emulation
  (Kerbin Mod_OnQuadBuilt in FLIGHT, Mun none) -> no exclusions.
- NEXT: tests/test_terrain.cpp (full), README/STATUS, commit.
- tests/test_terrain.cpp 13 cases 20081 assertions pass (prefab 3653 values bit-exact, maps sha256, Mun stack ==
  game, Kerbin 99999 group permuted vs game but commutes (6x41x41 pts both orders bit-equal), KSC pad 64.78470540174749,
  MapSO, LibNoise (tools/wp11/libnoise_reference.py), FromToRotation 49 rows, antipode decal quirk).
- NEXT: full ctest RelWithDebInfo + Debug; README + STATUS WP-11; commit.
- COMMITTED 9ed17fb on wp11 (45 files; maps marked `binary` in .gitattributes). Full suite 16/16 pass RelWithDebInfo
  (18.6 s) and Debug (45.6 s). Docs commit follows. DONE except final report.
