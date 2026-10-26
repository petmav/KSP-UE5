# PROGRESS wp12d: PQS package close-out

Branch `wp12d` from main `839f2f5f` (worktree `D:/Projects/wt-wp12d`). Agent: optimizer.

## Step 1: audit (done, 2026-10-26)

Every WP-12 spec file of `analysis/tiering/work_packages.csv` (20 files) against the cites in `core/src` + `core/include` (`grep -o "<File>.cs:N"`), then the C# read in full for the files with no or few cites.

| C# file | cites before | decision | reason |
|---|---|---|---|
| PQ.cs, PQS.cs, PQSMod.cs, PQSMod_QuadMeshColliders.cs, PQS_GameBindings.cs | 32 / 224 / 24 / 6 / 3 | done by WP-12a/b except below | PQS_GameBindings lacks its 4 PQSSurfaceObject events (OnPQSCityLoaded / Unloaded, OnGetPQSCityLoadRange, OnGetPOIRange) and C# multicast semantics: port in item 2b |
| PQS_ColliderAdvanced.cs (452) | 0 | **port** | gravity / velocity box colliders around a body: activation decisions, box poses, the 3-sample terrain normal, the halving velocity probe; double / float paths. Engine side = the Unity BoxCollider objects and Physics.IgnoreCollision: kept as data (pose, size, active, ignore pairs) + optional static boxes in PhysicsWorld. Debug cubes (`collidersVisibleDEBUG`, `showNormalsDEBUG`) are rendering: excluded. Never instantiated by shipped code (WP-12b finding), so stock behaviour does not change |
| PQS_Collider.cs (103), PQS_PartCollider.cs (473), PQSMod_CreateSphereCollider.cs (25) | 0 | **port** (not in the brief, but WP-12 spec files with no port) | same family; PQS_PartCollider is `Part.terrainCollider` (Part.cs:255, destroyed at Part.cs:2057, created by nothing); CreateSphereCollider is on no stock prefab |
| PQS_KSPBinder.cs (105) | 0 | **port** | binds GameSettings / HighLogic / PhysicsGlobals.VesselRangesDefault into PQS.GameBindings on Awake, unbinds on OnDestroy (sic: OnGetPOIRange is never removed) |
| PQSSurfaceObject.cs (8) | 0 | **port** | base of PQSCity / PQSCity2 (SurfaceObjectName, DisplaySurfaceObjectName, PlanetRelativePosition); CelestialBody.pqsSurfaceObjects |
| PQSCity.cs / PQSCity2.cs remainder | 23 / 21 | **port** | not covered by WP-42 / WP-12b: CheckLocals (loadRange / poiRange from the bindings), the OnPQSCityLoaded / Unloaded and OnPOIRangeEntered / Exited events of OnUpdateFinished, PQSCity2.OnPostSetup's poiRange, PQSCity2.Orientate's setVisibility / 7 km rule, OrientateRayCast over the WP-12b terrain colliders (the 4 stock launch sites have raycastSurface = 1), OrientateOnWater (no stock city; natives LookRotation / Euler UNVERIFIED) |
| SurfaceObject.cs (206) | 0 | **port** | lat / lon / alt pinning (GrabCoordsAtStart via CelestialBody.GetLatLonAlt, onUpdate position), the init delay, PopToSceneRoot / ReturnToParent over the `UnityEngine::Transform` shim, the update modes |
| SpaceCenterSurfaceObject.cs (57) | 0 | **port** | the scene-event wiring around a SurfaceObject |
| PQSMod_SurfaceObjectQuads.cs (39) | 0 | **port** | a SurfaceObject per quad at subdivision >= maxLevel - |offset| (on no stock prefab) |
| PQSLandControl.cs (930) | 8 (height half, WP-11) | **port the scatter half**, exclude colours | port: OnSetup's scatter matching and LandClassScatter.Setup, OnQuadPreBuild, the scatter branch of OnVertexBuild (lcScatterList), OnQuadBuilt, OnSphereStarted / Reset, LandClassScatter (cache counters, AddScatterMeshController count + seed, CreateScatterMesh placement: UnityEngine.Random draws, positions, rotations, scales, transformed vertices of the quad base mesh, DestroyQuad), ApplyCastShadowsSetting, CubicHermite / Clamp / DoubleLerp. The scatter quad (PQSMod_LandClassScatterQuad.cs, WP-48 tier C) is ported only as far as the scheduling that decides when CreateScatterMesh runs. Excluded: the vertex-colour branch of OnVertexBuild (createColors: the colour channel only the terrain shader reads; would also change WP-12a's mesh digests), meshes / materials / renderers / shadows, the baseMesh vertex data of the stock tree / rock meshes (not extracted; a host supplies them) |
| PQSCache.cs (495) | 24 | **port the rest** | Reset defaults, Awake singleton, OnDestroy, ResetCache, CoroutineCreatePQCache + onCacheReady; nothing affects positions or heights |
| PQSMod_CelestialBodyTransform.cs (355) | 10 | nothing engine-agnostic left | only AltitudeFade.Setup / Setup(PQS) (Shader.PropertyToID, renderer discovery: rendering) and Reset (editor; the field initialisers already hold its values) are uncited |
| PQSMeshPlanet.cs (587), PQSMeshWrapper.cs (155) | 0 | **exclude** | editor mesh tools (rendering, tier C, Unreal side), referenced nowhere in Assembly-CSharp |

Findings of the audit:
- The KSC has **no PQSMod_FlattenArea**: the ground under the KSC is flattened by `PQSMod_MapDecalTangent` on `Kerbin/KSC` (order 99999). Kerbin's only FlattenArea is at the Pyramids (the WP-12c note "the KSC pad's PQSMod_FlattenArea ground" is corrected in item 4: the comparison is MapDecalTangent terrain vs the pad collider).
- `UpgradeableSlave` (the 8 `SpaceCenter/Grounds/*` level groups follow their neighbour facilities) has no port and its `neighbourIDs` are not in data: needed for item 3; extracted from the game (sharedassets / level data) with UnityPy into a new data file.
- The DestructibleBuilding prefab values (collapse / repair durations, replace delay, tilt, FacilityDamageFraction, RepairCost, CollapseReputationHit, impactMomentumThreshold, damageDecay) and the UpgradeLevel costs are "needs data" in WP-42 / WP-36: extracted in the same pass.

## Plan (order; a commit after each item)

1. This audit (commit).
2. Ports:
   - 2a `include/ksp/terrain/pqs_surface_colliders.hpp`: PQS_Collider, PQS_ColliderAdvanced, PQS_PartCollider (box colliders as data + `SurfaceColliderHost` to place them in PhysicsWorld), PQSMod_CreateSphereCollider.
   - 2b PQS_GameBindings events (ksp::Delegate multicast), PQS_KSPBinder, PQSSurfaceObject base of PQSCity / PQSCity2, CheckLocals and the load / POI range events, PQSCity2.Orientate remainder (setVisibility, the 7 km rule, OrientateOnWater, the north-pole rotation), the OrientateRayCast hook over PhysicsWorld.
   - 2c SurfaceObject, SpaceCenterSurfaceObject, PQSMod_SurfaceObjectQuads.
   - 2d PQSLandControl scatter half (+ generator: the `scatters` table into pqs_prefab_data.cpp), PQ quad delegates as multicast `ksp::Delegate`.
   - 2e PQSCache remainder.
3. Facility levels and destruction at run time: `SceneryCityHost` with Unity's activeSelf / activeInHierarchy semantics (so a destroyed building stays down when its LOD range toggles), `SetFacilityLevel`, per-object collider transforms (the collapse animation moves the colliders); a KSC facility binding: UpgradeableFacility / UpgradeableSlave per level group driven by ScenarioUpgradeableFacilities, DestructibleBuilding per destructible of the shown level with the extracted prefab values; test on core/data/scenery.
4. KSC pad: WP-11 terrain (MapDecalTangent) vs the pad collider at the spawn point, gap and C# reason in STATUS.
5. Gates: independent Python references under core/tools/wp12d/ (colliders family with WP-11 heights from the WP-12a oracle; scatter placement; facility levels / hierarchy sets); the terrain capture where a height path is new (the pad point); unit tests.
6. STATUS / README / progress.csv, fresh suite, random order, WSL syntax check, pr_review.
7. Merge origin/main if moved, push, PR, CI.

## Log

- 2026-10-26: worktree created, fresh build (configure 2.9 s, build 410.8 s, `CMAKE_BUILD_PARALLEL_LEVEL=4`), audit written.
- 2a done: `include/ksp/terrain/pqs_surface_colliders.hpp`, `src/terrain/pqs_surface_colliders.cpp` (PQS_Collider, PQS_ColliderAdvanced,
  PQS_PartCollider, PQSMod_CreateSphereCollider; 48 cites, check_cites 0 mismatches). Reference `tools/wp12d/surface_colliders_reference.py`
  (oracle cache `tools/wp12d/height_oracle.json`, pruned to the answers used, 633,534 bytes sha256 cc2bd737...) ->
  `tests/data/wp12d_colliders_reference.json` (984,352 bytes, sha256 431a344b..., identical on a second run). Gate: 4 scenarios, 420 steps,
  3,060 box records, 86 IgnoreCollision calls: 0 mismatches. Mutation (cliff depth without the (float) cast + `<=` -> `<` in the part probe):
  107 mismatches.
