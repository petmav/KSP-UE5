# Terrain height capture summary

- ksp_version: `1.12.5 build 03190`
- unity_version: `2019.4.18f1`
- runtime: `Mono 5.11.0 (Visual Studio built mono) (Environment.Version 4.0.30319.42000)`
- assembly_csharp_sha256: `42d4b00db940c91eecf5d75ad1d95a4632c80f6322f2aad753f246b9335624cd`
- assembly_csharp_jit: `IsJITOptimizerDisabled=False`
- profiling_branch_commit: `36ef51bd9d095f57904e29e7b98886d0fd753d1d`
- utc: `2026-10-09T00:06:30.6674569Z`

| body | Radius | pqs.radius | radiusMin | radiusMax | mods | isActive |
|---|---|---|---|---|---|---|
| Kerbin | 600000 | 600000 | 598015 | 608735 | 46 | True |
| Mun | 200000 | 200000 | 199600 | 208300 | 23 | False |

| body | set | samples | h_surface != h_override | min terrain_alt | max terrain_alt | below sea level |
|---|---|---|---|---|---|---|
| Kerbin | active_vessel | 1 | 0 | 64.7847 | 64.7847 | 0 |
| Kerbin | global_1deg | 65160 | 0 | -1379.12 | 5809.95 | 29328 |
| Kerbin | ksc_0.01deg | 10201 | 0 | -883.767 | 330.455 | 5091 |
| Mun | global_1deg | 65160 | 3 | -40.1433 | 6852.33 | 1 |

## Scene probe (MAINMENU vs FLIGHT)

| scene | body | isStarted | isActive | mods | h(KSC) | h_override(0,0) |
|---|---|---|---|---|---|---|
| MAINMENU | Kerbin | False | False | 46 | 600065 | 599153 |
| MAINMENU | Mun | False | False | 23 | 204268 | 202631 |
| FLIGHT | Kerbin | True | True | 46 | 600065 | 599153 |
| FLIGHT | Mun | False | False | 23 | 204268 | 202631 |

## Mod stacks (evaluation order; * = overrides OnVertexBuildHeight)

- Kerbin: PQSMod_CelestialBodyTransform(0), *PQSMod_VertexHeightMap(10), *PQSMod_VertexRidgedAltitudeCurve(16), *PQSMod_VertexSimplexHeightAbsolute(20), *PQSMod_VertexHeightNoiseVertHeightCurve2(32), PQSCity2(100), PQSCity2(100), PQSCity2(100), PQSCity2(100), PQSCity2(100), PQSCity2(100), PQSCity2(100), PQSCity2(100), PQSCity2(100), PQSCity2(100), PQSCity(100), PQSCity(100), PQSCity(100), PQSCity2(100), PQSCity2(100), PQSCity2(100), PQSCity2(100), PQSCity2(100), PQSCity2(100), PQSCity(100), PQSCity(100), PQSCity2(100), PQSMod_AerialPerspectiveMaterial(100), PQSMod_TextureAtlas(100), PQSMod_MaterialSetDirection(100), PQSMod_QuadMeshColliders(100), PQSCity(100), PQSCity(100), PQSCity(100), PQSCity(100), PQSCity(100), PQSCity2(100), PQSMod_QuadEnhanceCoast(9999), *PQSMod_MapDecalTangent(9999), PQSCity(10000), *PQSMod_MapDecal(99999), *PQSMod_MapDecalTangent(99999), *PQSMod_FlattenArea(99999), PQSMod_UVPlanetRelativePosition(999999), *PQSLandControl(9999991), PQSMod_AltitudeUV(2147483647)
- Mun: PQSMod_CelestialBodyTransform(0), PQSMod_VertexColorMap(10), *PQSMod_VertexHeightMap(10), PQSCity(100), PQSCity(100), PQSCity(100), PQSCity(100), PQSCity(100), PQSCity(100), *PQSMod_VoronoiCraters(100), *PQSMod_FlattenArea(100), *PQSMod_FlattenArea(100), PQSCity(100), PQSMod_QuadMeshColliders(100), PQSCity(100), PQSMod_UVPlanetRelativePosition(100), PQSCity(100), *PQSMod_VoronoiCraters(101), *PQSMod_VertexSimplexHeight(102), *PQSMod_VertexHeightNoiseVertHeight(103), PQSMod_VertexSimplexNoiseColor(200), *PQSLandControl(999999), PQSMod_AltitudeAlpha(999999999)

## Map assets referenced by the stack

| body | mod | class | field | type | Unity asset name | MapName | w | h | bpp |
|---|---|---|---|---|---|---|---|---|---|
| Kerbin | 1 | PQSMod_VertexHeightMap | heightMap | MapSO | kerbin_height_new | kerbin_height_new | 4096 | 2048 | 1 |
| Kerbin | 28 | PQSMod_TextureAtlas | textureAtlasMap | CBTextureAtlasSO | KerbinTextureAtlasMap | 8KIndex | 8192 | 4096 | 3 |
| Kerbin | 38 | PQSMod_MapDecalTangent | heightMap | MapSO | island_runway_decal_heightmap | island_runway_decal | 256 | 256 | 2 |
| Kerbin | 40 | PQSMod_MapDecal | heightMap | MapSO | decal_spaceport_0_height | decal_spaceport_0_height | 1024 | 1024 | 2 |
| Kerbin | 41 | PQSMod_MapDecalTangent | heightMap | MapSO | ksc_decal_heightMap | ksc_decal_heightMap | 1024 | 1024 | 2 |
| Kerbin | 44 | PQSLandControl | heightMap | MapSO | kerbin_height_new | kerbin_height_new | 4096 | 2048 | 1 |
| Mun | 1 | PQSMod_VertexColorMap | vertexColorMap | MapSO | munColorMap2 | Mun [Albedo] | 4096 | 2048 | 3 |
| Mun | 2 | PQSMod_VertexHeightMap | heightMap | MapSO | munHeightMap2 | munHeightMap2 | 4096 | 2048 | 1 |
