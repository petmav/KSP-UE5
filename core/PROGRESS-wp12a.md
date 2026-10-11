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
