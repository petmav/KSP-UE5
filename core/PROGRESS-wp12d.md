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
- 2b done: `PQS_GameBindings` as C# multicast (`ksp::Delegate`, default handler first) with its four PQSSurfaceObject events;
  `include/ksp/terrain/pqs_surface_object.hpp` (PQSSurfaceObject base of PQSCity / PQSCity2); `pqs_ksp_binder.hpp/.cpp` (PQS_KSPBinder,
  OnGetPOIRange left bound after OnDestroy as in the C#); PQSCity Start / OnPostSetup / PostSetup / CheckLocals / the load and POI range
  events; PQSCity2 Start / OnDestroy / OnScenerySettingChanged / OnPostSetup / DisplaySurfaceObjectName / the complete Orientate
  (setVisibility, 7 km rule, double snapHeightOffset on the raycast branch, positionMobileLaunchPad), OrientateOnWater (PQS-local frame),
  OnUpdateFinished re-orientation + POI events; GetSurfaceHeight(.., overrideQuadBuildCheck: true) as the C# (was the public variant);
  PQSCityColliders / PQSCity2Colliders link (`city`, `Link`); `SceneryCityHost::LinkCity`; `physics::MakeOrientateRayCast`;
  the city generator emits OrientateOnStart / OrientateOnPostSetup. Tests: binder / multicast, range events, the four stock launch sites'
  raycast correction (Crater: collider 3.98 m below the analytic height, placed 18.49 m above the collider; KSP's correction adds
  (city point - collider) instead of subtracting it, and snapHeightOffset twice). check_cites 124 cites, 0 mismatches.
- 2c done: `include/ksp/terrain/surface_object.hpp`, `src/terrain/surface_object.cpp`: SurfaceObject (3 Create overloads, the Start
  coroutine one frame after Create as Unity's Start, PopToSceneRoot / ReturnToParent over the Transform shim, the three update modes,
  Terminate), SpaceCenterSurfaceObject (scene events, DelayedCallback(5) re-pop), PQSMod_SurfaceObjectQuads (per-quad Transforms owned
  by the mod). 3 test cases; check_cites 19 cites, 0 mismatches.
- 2d done: PQSLandControl scatter half (`src/terrain/pqs_land_scatter.cpp`; header in `pqs_mods.hpp`), the generator emits the `scatters`
  table (pqs_prefab_data.cpp regenerated: additions only; the WP-11 prefab test now also compares the scatter fields), PQ quad delegates
  as multicast `ksp::Delegate` (invoked through a snapshot), the land-class state carried through PQSHeightWorkers. Gate:
  `tools/wp12d/scatter_inputs.cpp` (inputs) -> `tools/wp12d/scatter_reference.py` (Random checked against 7,424 rows of the C2 capture,
  AngleAxis against the 84 C3 rows) -> `tests/data/wp12d_scatter_reference.json`: 17 scatter quads, 2,096 instances, 0 mismatches; inline
  vs 4 height workers: 136 scatter quads identical. Mutations: Range(-1, 2) in the vertex pick, no RestoreBuildState: both detected.
  Finding: the WP-12d cases that build stock spheres must restore Simplex / SimplexNoise permutations and UnityEngine.Random
  (`wp12d::StateGuard`), else WP-03's Simplex cases fail later in the run. Full unit binary: 1,105 cases / 1,694,932 assertions pass.
- 2e done: PQSCache Reset / Awake (singleton) / OnDestroy / ResetCache / CoroutineCreatePQCache + onCacheReady (cachePQCoroutine,
  default false = the C# field default). PQSMod_CelestialBodyTransform: nothing engine-agnostic left (AltitudeFade.Setup is renderer
  discovery). 1 test case.
- 3 done: `CityColliderSet` hierarchy mode (activeSelf / activeInHierarchy / presence / per-object rigid transforms); `SceneryCityHost`
  on /2 documents uses it with the switched GameObjects as roots, plus SetFacilityLevel / SetObjectActiveSelf / SetObjectLocalPose;
  `include/ksp/physics/ksc_facilities.hpp` (UpgradeableSlave port, KSCFacilityHost: UpgradeableFacility / UpgradeableSlave per level group
  driven by ScenarioUpgradeableFacilities, DestructibleBuildings of the shown levels with ScenarioDestructibles, Sync). Findings:
  UpgradeLevel.Spawn destroys the GameObject facilityTransform points at (the replaced level's Spawned turns false through Unity's null
  check; the WP-36 port's flag did not, so a re-upgrade to the Awake level was a no-op until the host clears it); DestructibleBuilding.Start
  always ends in SetDestroyed(!intact) (Load, or Reset when RegisterInstance returns false, which it does on the first registration):
  collapse objects inactive in the prefab (5 records: flare lights of the LaunchPad / SPH / VAB top levels, RnD level 1) become active.
  Gate: `tools/wp12d/facility_reference.py` -> `tests/data/wp12d_facility_reference.json` (sha256 3df56c1a...): 4 scenarios x 2,795 KSC
  objects: 0 mismatches; run time upgrade LaunchPad 0 -> 2 (42 -> 139 pad objects, 206 -> 270 static bodies); a collapse (TrackingStation
  dish, FIXTURE timings: 2 s): the collider sinks 1.875 m after 1 s, is gone at the end, stays gone through Deactivate / ActivateAll,
  comes back on Repair at the original place (< 1e-6 m). WP-12c / 20g / 21b / 29d / 20e scenarios unchanged (CoM 0.174 / 0.190 m).
  Deferred (needs the game copy, not readable from this session): UpgradeableSlave.neighbourIDs (FIXTURE from the group names) and the
  DestructibleBuilding prefab values.
- 4 done: the KSC has no FlattenArea; its ground is PQSMod_MapDecalTangent (absolute, heightMapDeformity 70, radius 7.5 km). At the
  LaunchPad_spawn radial (lat -0.0972078, lon -74.5577): WP-11 terrain 64.7847 m (= the capture's active_vessel h_override bit for bit);
  WP-12b terrain collider 64.4682 m (-0.32 m: sub-cell relief); KSC origin 42.66 m (radius + repositionRadiusOffset 42.7, float);
  pad collider level 0 "collider" at 67.5261 m (+2.74 m over the terrain), highest level "Launch Pad" at 72.4079 m (+7.62 m).
- physxMaterials measured (forced on in a throw-away build, whole unit binary): 1 assertion flips (WP-12c's legacy-rule check), every
  other gate within its tolerance; the moved numbers are in STATUS WP-12d. Left off.
- Merged origin/main 3eac9e97 (WP-29e). Fresh: 53 / 53 ctest; random orders 11 / 27: 1,121 cases / 1,696,411 assertions. STATUS
  section, row 68, README, progress WP-12 1.0 written.
