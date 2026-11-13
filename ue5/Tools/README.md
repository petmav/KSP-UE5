# ue5/Tools: game asset extraction (WP-UE-06)

Extracts the Unity-serialized assets the first playable slice renders from the local game copy (`decompiled/KSP-dev`) into
`ue5/GameAssets/` (gitignored: nothing extracted is ever committed) in raw formats, with a manifest that records where every file came
from. The loose GameData files (`.mu`, `.dds`, `.png`, `.mbm`, `.cfg`, ...) are read by the shell at run time and are not touched here
(`DESIGN.md` 3.7). CLAUDE.md "Original game assets": nothing is generated, repainted, upscaled or substituted; what could not be found
or decoded is in `manifest.json` `missing` with the asset file and pathID.

## Run

Python 3.12 with UnityPy 1.25 (the harness venv). Run with `-I`; the scripts live here, never in the game or output directory.

```
harness\tools-cache\venv\Scripts\python.exe -I ue5\Tools\extract_game_assets.py --game <repo>\decompiled\KSP-dev --out ue5\GameAssets ^
    [--summary ue5\Tools\extraction_summary.json] [--only scaled_space,skybox,pqs,ksc,fonts,ui]
harness\tools-cache\venv\Scripts\python.exe -I ue5\Tools\verify_game_assets.py --game <repo>\decompiled\KSP-dev --assets ue5\GameAssets ^
    [--summary ue5\Tools\extraction_summary.json]
```

* `--out` must be empty or absent. A worktree has no `harness/lib` (gitignored): the tool finds the main checkout's through
  `<game>/../../harness/lib`, or pass `--lib`. `harness/lib` supplies the MonoBehaviour type trees (the player build ships none).
* KSC objects are cross-checked against `core/data/scenery/scenery_colliders.json` and `core/data/terrain/pqs_prefab.json`
  (defaults: the repo copies; `--scenery`, `--pqs-prefab` override).
* Time on the dev laptop: extraction 45 s, verification 115 s. Free disk needed: about 1.2 GB.
* Running the extraction twice gives byte-identical output (the manifest holds every output's sha256; compare `manifest.json`).

Files: `extract_game_assets.py` (CLI, categories, manifest), `ksp_unity.py` (loader: UnityPy + type trees, PPtr / GameObject helpers),
`ksp_textures.py` (DDS / PNG writers), `ksp_mesh.py` (mesh binary), `ksc_scenery.py` (KSC tree replay), `verify_game_assets.py` (the
independent check), `extraction_summary.json` (counts and digests of the last run, no game content).

## Output layout (what the shell reads)

```
ue5/GameAssets/
  manifest.json                         provenance of every file (below)
  scaled_space/<Body>.json              17 bodies: scaledVersion node tree (TRS, renderers -> mesh + material files, selected MonoBehaviour fields)
  skybox/galaxy_cube.json               GalaxyCube: 6 quads, their materials, GalaxyCubeControl fields
  pqs/Kerbin.json                       roles: which PQS field / mod uses which material or texture file
  ksc/Kerbin_KSC.json                   2,795 objects, same index / order as the WP-12c collider tree, with their renderers
  ui/flight_hud.json                    chosen HUD sprites (atlas file + rect + border + pivot), HUD textures, materials, meshes
  textures/<file>_<pathID>_<name>.dds|png
  meshes/<file>_<pathID>_<name>.kmesh   + .json (the description of that binary)
  materials/<file>_<pathID>_<name>.json shader name, floats, colours, texture slots (file + tiling)
  fonts/<file>_<pathID>_<name>.ttf
```
`<file>` is the asset file without `.assets` (`sharedassets9`, `level9`, `resources`). Description JSONs refer to other outputs by
relative path and to game objects by `{file, pathID}`; a `{"$ptr": [file, pathID]}` value is a PPtr inside a dumped MonoBehaviour.

### manifest.json

`{schema, tool, unityVersion, sourceFiles{asset or .resS file: sha256}, files[], missing[], warnings[], categories{name: {files, bytes}}}`.
Per file: `path`, `kind` (`texture`, `mesh-binary`, `mesh-description`, `material`, `font`, `description`), `categories` (a file shared by
two categories lists both), `bytes`, `sha256`, `origin {file, pathID, type, name}` (the Unity object it came from; for a description
the root object of what it describes) and `meta` (textures: size, mip count, Unity format, sRGB, wrap / filter, container).
The shell keys its caches on (path, sha256).

### Conventions

* **Textures keep Unity's memory order: the first row is the BOTTOM row (v = 0)**, in DDS and PNG alike, and mesh UVs stay Unity's, so
  a texture and a mesh agree with no flip anywhere. An image viewer shows these files upside down; do not "fix" one side only.
* DDS: block formats (DXT1, DXT5, BC7, BC4, BC5) are the stored mip chain behind a header, nothing re-encoded. DXT1 / DXT5 use the legacy
  FourCC header, the others a DX10 header (BC7 = DXGI 98). `meta.sRGB` says whether the data is sRGB (the DDS format is UNORM).
* PNG: 8-bit uncompressed textures (RGBA32, RGB24, Alpha8 as grey, ARGB32 / BGRA32 reordered, 4444 expanded) as mip 0 only (`mipsWritten`).
* Crunched textures (`DXT1Crunched`, `DXT5Crunched`, 34 of them) are unpacked to their DXT blocks by texture2ddecoder (Unity's codec); it
  returns the top level only, so these DDS files have `mipsWritten` 1 and the consumer generates mips.
* Meshes: `.kmesh` is little endian float32 arrays (position 3, normal 3, tangent 4, color 4, uv0..uv7 2; only the channels the mesh has)
  followed by uint32 indices; offsets, submeshes (`firstIndex`, `indexCount`, `topology`, `baseVertex`), bounds and the source vertex
  format of each channel are in the `.json` (full rule in `ksp_mesh.py`). Left-handed Unity frame, nothing mirrored or rewound.
  Meshes carrying bone weights are written without them (`skinned`).
* Transforms in description JSONs are the prefab's local TRS (position xyz, rotation xyzw quaternion, scale xyz), Unity frame.

## What is extracted

| category | content | origin |
|---|---|---|
| `scaled_space` | the 17 bodies' `scaledVersion` GameObject trees (planet sphere, Kerbin / Eve / Laythe `Atmosphere` shells, Sun `Corona` quads): meshes, materials with their textures (`_MainTex`, `_BumpMap`, rim ramps, ...), `ScaledSpaceFader` / `MaterialSetDirection` / `AtmosphereFromGround` / `ScaledSun` / `SunCoronas` ... fields | `sharedassets9.assets` PSystemBody objects; meshes, materials, textures in `sharedassets2/9` |
| `skybox` | the six GalaxyCube quads (`GalaxyTex_*` 4096 x 4096 BC7), their materials, `GalaxyCubeControl` fields | `level9` GameObject 15, textures `sharedassets2` |
| `pqs_kerbin` | the materials and textures used by Kerbin's PQS tree: the PQS component's quality materials and fallback, `PQSMod_TextureAtlas` blend materials, the `KerbinOcean` child sphere's `KerbinWater` / fallback materials and `PQSMod_OceanFX` textures (the ocean data DESIGN R9 was missing) | `sharedassets9.assets` PQS GameObject |
| `ksc` | the `Kerbin/KSC` city (pad, VAB, SPH, runway, tracking station, administration, R&D, mission control, buildings, all facility levels): per object index the renderers (MeshFilter / MeshRenderer: mesh, materials, shadow flags) | `sharedassets9` city + nested prefabs (`sharedassets2/9/...`) |
| `fonts` | every Font object with embedded data: Calibri family, HEADINGFONT, dotty, Twin Marker, Perfect DOS VGA 437 | `sharedassets0/4`, `resources` |
| `ui` | flight HUD sprites, 3D HUD objects, see below | `sharedassets7` (flight scene) and shared UI files |

**KSC placement.** The render tree is built by replaying the same `NestedPrefabSpawner` and `UpgradeableObject` spawners as
`core/tools/mucollider/extract_scenery_colliders.py`, and the tool refuses to write unless index, path, parent, source GameObject,
active / layer / tag and the float32 bits of every local TRS equal the committed collider tree. Object `i` here is object `i` there:
use the same `PQSCity::Orientate` pose (`core/include/ksp/space_center/`), the collider doc's `lod`, `destructibles` and `levelGroup`
fields for visibility, and this file's `renderers` for the meshes. Only `Kerbin/KSC` is extracted (the pad's neighbours are all in it);
the other 58 cities, the launch-site set dressings and KSC2 need `cat_ksc(..., city_name=...)` and their own render pass.

**UI sprites: what was chosen and why.** Roots (all in the flight scene file `sharedassets7.assets`): the `Flight` HUD canvas (navball
frame, speed / altitude / time frames, crew frame, temperature gauges, action group buttons, mode buttons, throttle / linear / rotation
gauges), `NavBallController`, `StageManager`, `StageGroup`, `StageMarker` (staging stack, delta-v gauge), `ManeuverGizmo`, `MapNode`
(map markers: Ap, Pe, AN, DN, encounter, escape, vessel type icons), `VesselLabels`. A sprite is chosen when a component in those prefab
trees (UnityEngine.UI.Image `m_Sprite` or any MonoBehaviour field; prefab references are followed inside `sharedassets7` only)
references it; 3D HUD objects are the meshes / materials of MeshRenderers in the same trees (the NavBall sphere, its vector
materials, the tumbler, the maneuver gizmo handles, the CoM / CoL / CoT markers), TextMeshPro font materials left out. The
staging part-icon atlas `StageIconMapDefault` is added by name (referenced from the `StageIcon` prefab outside the flight file). Sprites are
**not cropped**: each entry names its atlas texture file and gives `textureRect` (atlas pixels, origin bottom left) so the shell uses a
UV region and nothing is decoded or re-encoded. `border` is the 9-slice border, `settingsRaw` the packing flags.

## What is not extracted

* Music and sound (not needed for the slice).
* `Texture2DArray` (Kerbin's ultra-quality `diffuseTextureArray` / `normalTextureArray` atlas, 2 objects), `Cubemap` (`OceanReflection`,
  referenced by a KSC material) and `Texture3D`: not decoded; listed in `missing`. The slice's PQS quality levels low / medium / high do
  not use them.
* The other 16 bodies' PQS materials; Kerbin's scatter props (`PQSLandControl`, ROC rocks) and monument / launch-site city subtrees.
* KSC lights, particle systems, animation clips, skinned-mesh bones (12 skinned renderers are written as static meshes), audio.
* Built-in `Arial` (no embedded data) and the TextMeshPro SDF font assets (Slate uses the TTFs).
* Crunched textures' lower mips (see above); 4444 / 565 PNG paths exist but no extracted texture uses them.
* The three KSC renderers whose mesh PPtr is null in the game (`RnD_apollo_Smallgrp01 / 02`, `beat_up_tracking_dish`).

## Verification (`verify_game_assets.py`)

Shares no code with the extractor (the WP-12c pattern): raw type-tree dicts through UnityPy, its own `.resS` reader, its own `struct`
vertex decode, own DDS / PNG parsers and expected-pixel mapping. Checks: manifest vs disk (size, sha256, no unlisted files, the source
file digests), origin type / name, DDS header + payload bytes, PNG decode vs raw pixels, every channel / index / submesh / bounds of
every mesh bitwise, every material property, fonts, node trees (names, TRS, layer, tag, renderers, mesh and material origins) of the
scaled bodies and the GalaxyCube, KSC vs the committed collider tree and the raw renderers of each source GameObject, scaled-body
origins and PQS roles vs `core/data/kerbol_body_assets.json` (written by a different tool), sprites vs raw Sprite objects.
Not independent: Crunched textures (texture2ddecoder is the only codec) and the HUD sprite selection rule / MonoBehaviour field dumps
(they need the type-tree generator). Mutation test: six single edits (a mesh float, a DDS payload byte, a material colour, a KSC
position, a sprite border, a PNG pixel) each reported as a mismatch.
