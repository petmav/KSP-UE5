# WP-12a progress: PQS quadtree and quad build (branch `wp12a`, optimizer tier)

Worktree `core/.worktrees/wp12a`, from jolt-spike `b3b70db5` (PR #14), origin/main `79ea152d` merged at start (`3df60db0`).

## Scope (from the orchestrator)

Engine-agnostic PQS quadtree: PQ subdivision / collapse / neighbours, visibility and subdivision decisions from the
target position and the body's PQS settings, vertex / normal / tangent / UV / colour channel build over WP-11's height
maths, quad pool (PQSCache) semantics; output = plain mesh data for the UE5 shell. Colliders are WP-12b (leave the seam).
Gate: independent Python reference (no kspcore code) at 0 ULP / byte-exact vertex data; terrain_height capture as truth
for heights. STATUS row 38, README, check_cites exact mode, no subagents.

## Facts established (before coding)

- Native Unity functions the quad build needs, checked in Python against `harness/captures/out/math_shim_native.json`:
  `Matrix4x4.TRS` = Unity QuaternionToMatrix (x2 = x*2, xx = x*x2, ...; m = 1 - (yy + zz) ...) with columns scaled by s,
  per-op float32: 45/45 rows bit-exact. `Vector3.OrthoNormalize` = normalise u by DIVISION (|u| <= 1e-5 -> (1,0,0)),
  v -= dot(u, v) * u, |v| < 1e-5 -> OrthoNormalVectorFast(u) (|n.z| > 1/sqrt2: (0, -z k, y k) else (-y k, x k, 0),
  k = 1 / sqrt(...)), else v / |v|; dot = ((x*x + y*y) + z*z) per-op float: 49/49 rows bit-exact (multiply-by-reciprocal: 22 rows differ).
  `Quaternion.FromToRotation` is WP-11's verified native.
- `PQS.verts` (static Vector3d) holds the planet-centred DOUBLE vertex positions in both quad modes; BuildNormals reads
  them, so normals are world-independent. Only the float mesh vertices of `surfaceRelativeQuads` go through Unity's float
  world transforms (TransformPoint / InverseTransformPoint).
- `PQSMod_UVPlanetRelativePosition.OnQuadBuilt` overwrites the STATIC `PQS.cacheUVs` (that is why BuildMeshUVChannel
  assigns cacheUVs and the `uvs` scratch is never read); `OnQuadUpdateNormals` reads `quad.verts[i] + quad.positionPlanet`
  (assumes surfaceRelativeQuads).
- `PQS.UpdateQuads` sorts the 6 roots and updates only the first 5 (`Mathf.Min(quads.Length, 5)`).
- `UpdateQuadsInit` always runs 10 passes (quadCount is only ever 6).
- `PQSMod_QuadEnhanceCoast.OnQuadBuilt` multiplies `subdivideThresholdFactor` (Kerbin: coastFactor 2): quad structure
  depends on built heights.
- All OnVertexBuild overrides leave vertHeight alone (writes only in OnVertexBuildHeight).
- PQS LOD fields not in `data/terrain/pqs_prefab.json` (only radius, minLevel, maxLevel, buildTangents ... were
  extracted): minDetailDistance, maxDetailDistance, subdivisionThreshold, collapse*, visRad*, maxQuadLenghtsPerFrame,
  surfaceRelativeQuads, maxFrameTime. KSP-dev may not be read in this package -> FIXTURE values (PQS.Reset defaults)
  plus an extended PQS_FIELDS list in tools/wp11/extract_terrain.py for a later local run.
- PQSMeshPlanet / PQSMeshWrapper are not referenced anywhere in Assembly-CSharp (editor tools).

## Plan

1. unity natives: TRS, OrthoNormalize (+ capture tests)
2. PQ / PQS quadtree / PQSMeshCache / PQSCache pool + presets / build / normals / edge normals
3. mods: QuadEnhanceCoast, CelestialBodyTransform (activation, fade values), UVPlanetRelativePosition, AltitudeUV,
   AltitudeAlpha, QuadMeshColliders (collider seam); generator update
4. Python reference tools/wp12/pqs_reference.py (synthetic stack full emulation; real bodies with WP-11 height oracle)
5. tests, perf bench (per-quad build time), optimisations with bit-identity tests
6. STATUS / README / check_cites, merge origin/main, fresh run, push

## Log

- (start) read spec, C# (PQ.cs, PQS.cs, PQSCache.cs, mods), WP-11 code; merged origin/main; natives probed.
- `c0524cfb` C++ port: PQ / PQS quadtree / build / normals / stitching / cache / pool / presets / quad mods; generator
  emits the quad mods + PQS LOD fields (missing ones: PQS.Reset() FIXTURE, surfaceRelativeQuads FIXTURE true);
  TRS / OrthoNormalize bit-exact vs C3 (45 / 49 rows). Smoke: Kerbin pad 698 quads, 479 built, levels 0-9.
- `4e04da65` Python reference tools/wp12 (pqs_ref_math / tree / sphere + pqs_reference.py): natives vs C3 capture
  (TRS 45, OrthoNormalize 49, FromToRotation 49 rows), x87 model vs libm capture (6,180 Sin/Cos rows); scenarios
  synth_a (custom normals, stitching, tangents, FlattenArea, coast factors, CBT deactivate/activate, RebuildSphere,
  counting clock), synth_b (sphere normals, planet-centred vertices, quad UVs, map coords), kerbin_ksc and mun (WP-11
  heights through tools/wp12/height_oracle.cpp; sampled quads + their stitching sources exact). Replay in
  tests/test_pqs_quadtree.cpp: 0 mismatches over 24,728 quad rows and 13,051 mesh digests; mutation checks (edge
  normal 0.5f -> 0.4999f; updating 6 roots instead of 5) fail as expected. Pushed origin/wp12a.
  Default clock changed to a constant 1 s (KSP: Time.realtimeSinceStartup > maxFrameEnd = 0 during UpdateQuadsInit's
  pass, so that pass never subdivides).
  Lesson: bash heredocs here collapse backslashes; use the Edit tool for strings with escapes.
- next: unit tests (pool, presets, colliders, fades, all-quads heights == WP-11, KSC triangulation vs capture), bench,
  parallel height prefetch (bit-identical, gated by the replay), docs, check_cites, merge, fresh run.
- `d85be49a` unit tests: every built quad == WP-11 heights (Kerbin 479 quads / 107,775 vertices, Mun 421 / 94,725),
  the live quad-building sphere vs the terrain_height capture (140,522 points, 0 differ), pool, presets, colliders seam,
  fades, render handoff (max |origin + float vertex - double vertex| 0.031 m).
- bench (tools/wp12/bench.cpp, wp12_bench 5, 7950X3D, 1 thread): Kerbin BuildQuad 732 us/quad (heights 695, geometry
  ~38), StartSphere 369 ms for 479 built quads; Mun 1,234 us/quad; flat stack 38 us/quad; steady frame 0.018-0.029 ms.
- `7abad55b` PQSHeightWorkers (opt-in parallel height pass; UpdateQuadsInit leaves + Subdivide children), pushed.
- resumed after a rate-limit cut; merged origin/main (wp33, wp38b) cleanly; build + WP-12a tests green.
- next: replay gate with workers on (bit-identical), bench with workers, docs (STATUS row 38, README), check_cites,
  fresh full run, push.
- `45125488` worker gates: the 4 replays with 4 workers 0 mismatches; every stock built quad identical inline vs
  workers (Kerbin 501, Mun 421); StartSphere Kerbin 305-369 ms -> 121-124 ms with 15 workers, Mun 529-542 -> 78-102 ms.
- `68773105` C ABI kspcore_pqs_* (additive, KSPCORE_ABI_TERRAIN; ABI version stays 1: the UE bridge static_asserts it),
  capi.c (Kerbin, 2 workers) + capi.ue_like (Mun) exercise it; STATUS section + row 38; README; check_cites exact 164 / 0
  on the WP-12a .cpp (pqs_mods.cpp keeps 4 pre-existing WP-11 mismatches); header cites 184 / 0 (scratch checker).
  Incremental full run before the ABI change: 34/34. CI runs on push (Linux + Windows).
- fresh run (core/build deleted, -Fresh): 34/34, unit 733 / 1,207,865; bench pinned per CCD (frequency CCD: Kerbin 0.64 ms/quad, StartSphere 308 -> 108 ms with 15 workers; Mun 332 -> 90 ms). STATUS perf + verification updated.
