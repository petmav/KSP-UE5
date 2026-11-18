# WP-UE-11 progress (terrain)

Branch `wpue11` (on top of `wpue10`). Inline in the orchestrating session, 2026-11-18.
Results and numbers: `STATUS.md` "Terrain, first half"; core side: `core/STATUS.md` "WP-UE-11, core side".

## First half (this PR, progress 0.5)

- [x] Core: re-stitched quads are sent again; `capi.flight_slice` checks the terrain stream over the whole ascent.
- [x] Core: `PQSLandControl.OnVertexBuild` colour statements ported (terrain was black without them); oracle and reference regenerated.
- [x] Facade: `Bodies()`, `QuadsAdded()`, `QuadsRemoved()`.
- [x] `UKspTerrainView` (pooled procedural mesh components below a body-frame actor), `M_KspTerrain` (vertex colour).
- [x] `KspUe5.Terrain.PadQuads` (headless, mirror data): placement, winding, colours, counts.
- [x] Screenshot `slice/shots/wpue11_terrain_pad.png`.

## Second half (not started)

1. The terrain material's textures: regenerate `ue5/GameAssets` (`Tools/extract_game_assets.py --game decompiled/KSP-dev`, needs
   `harness/lib`; the session's shell may not read the game copy, so the user runs it), then a terrain master that blends them as the
   game's PQS shader does (the quads carry uv .. uv4 and the alpha the shader reads).
2. Ocean sphere, terrain scatter (core computes the instances; base meshes are not extracted).
3. Acceptance measurements: descent from 100 km to 10 m with render-thread average and worst frame, screenshots at six altitudes,
   visible quad count against core's every frame (the stream bookkeeping is already checked in `capi.flight_slice` and the pad test).
4. Shadows on and from terrain; planet fade to scaled space (with WP-UE-12).

## Things that cost time

- Black terrain was not a shell bug: core's quads had black vertex colours because the colour branch of `PQSLandControl` was never
  ported. A placement test with numbers (distance to the vessel, colour statistics) found it in one run; guessing from the screenshot
  would not have.
- The stock terrain gate compares mesh digests that include colours, and its Python reference takes heights from a C++ oracle file:
  changing what core computes per vertex means extending the oracle protocol and regenerating `height_oracle.json` and
  `wp12_reference.json` (24 s).
- Tool output of Python sources is shortened by a session hook (function bodies elided): prefix lines (`sed 's/^/| /'`) to read them.
