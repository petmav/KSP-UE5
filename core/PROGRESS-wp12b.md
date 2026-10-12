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
- next: replay gate, sphere-patch / Kerbin / Mun height bound, KSC box drop, bench, STATUS/README, cites, fresh + rand + ASan.
