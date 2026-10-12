# WP-12b progress: terrain colliders (branch `wp12b`, from main b9119103)

Scope (orchestrator): the WP-12a seam `PQS::onQuadColliderEnabled / onQuadColliderDisabled` -> static collision geometry in
PhysicsWorld; PQSCity / PQSCity2 object colliders as seams; terrain query API; gate = independent Python reference of the
activation decisions + analytic ray/normal tests + KSC pad box drop (1/2/4/8 threads) + performance numbers.

## Facts established from the C#

- `PQSMod_QuadMeshColliders.cs:17-44` is the whole rule: minLevel = maxLevel - |maxLevelOffset| (all 15 stock bodies: offset 0),
  collider created+enabled in OnQuadBuilt, disabled in OnQuadDestroy; NO distance test. Quads near the target are the PQS
  quadtree's decision (WP-12a). A quad that goes invisible or is subdivided keeps its collider until DestroyQuad.
- Layer: quads take the PQS GameObject's layer (PQS.cs:1015); analysis/physics: 15 "Local Scenery" = terrain+scatter, mask 32768 in
  PQSCity2.OrientateRayCast (:354); KSP has no IgnoreLayerCollision call.
- PQSCity colliders = prefab children toggled by `transformsActive` + LODRange.objects by float distance (PQSCity.cs:22-60, 168-260);
  PQSCity2 LodObject (PQSCity2.cs:10-60, 367-513) by squared distance, KeepActive.
- PQS_Collider / PQS_ColliderAdvanced / PQS_PartCollider are never instantiated by shipped code (Part.cs only destroys
  `terrainCollider`); PQSMod_CreateSphereCollider not in the stock prefabs; PQSMod_SurfaceObjectQuads/SurfaceObject = vessel
  parenting for the floating origin.
- PhysicMaterial "Ground" values are not in core/data (only the name): friction is a FIXTURE (0.6, as the spike).

## Log

- built baseline, added PhysicsWorld::AddStaticMesh/AddStaticShape/RemoveStatic/RayCast/OverlapSphere,
  `ksp/terrain/terrain_colliders.*` (TerrainColliders), `ksp/space_center/city_colliders.*`.
- `tools/wp12/colliders_reference.py` -> `tests/data/wp12b_reference.json` (events + sets for synth_a/b/c, kerbin_ksc, mun).
- `5730c617` replay gate (5 scenarios x plain / budget / workers), flat sphere patch, WP-11 height agreement (Kerbin, Mun), KSC box drop
  at 1/2/4/8 threads, 17 WP-12b cases; 38/38 ctest. Mutants (rule `>`, float-mesh geometry) detected. bench: cook ~83 us/quad.
- findings: Jolt CollideShape wants the WORLD transform; acos(dot) of float normals lies by 4.5e-4 rad (use the chord); rays through
  shared quad-border vertices can miss (retry 2 mm aside); mesh vs WP-11 between vertices = sub-cell relief (1.5 m near the pad, 40 m worst).
- next: ASan/UBSan in WSL (running), STATUS section + row 47 + README (drafted), fresh build + rand seeds 11 / 27, final report.
