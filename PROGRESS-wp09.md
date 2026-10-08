# WP-09 progress (worktree core/.worktrees/wp09, branch wp09)

## Step log
1. Read CLAUDE.md, NEXT-STEPS.md, core README/STATUS, WP-09 rows, harness READMEs/generators (BodiesGen, AtmosphereGen,
   ThermalGen, Rehosted/CelestialBodyLifecycle, KspEnv), serialized JSON layout. DONE.
2. CMakeLists.txt: KSP_PROJECT_ROOT now walks up to find harness/compare/compare.py (worktree-safe). Baseline 10/10 green
   in the worktree (build/RelWithDebInfo). DONE.

## Decisions
- Catalog storage: generated C++ data (tools/gen_celestial_data.py -> src/celestial/kerbol_system_data.cpp) from
  harness/golden/serialized/celestial_bodies.serialized.json; exact literals; unit test re-reads the JSON and compares
  every field bit-exactly. No JSON parser in the library.
- PhysicsGlobals: member initializers = level0 prefab values (Unity overwrites every [SerializeField]); statics get C#
  initializers; LoadDatabase(ConfigNode) ported as a generated ordered statement list (same order as C#).
- Orbit: CelestialBody holds serialized orbit elements (OrbitElements) + optional ICelestialBodyOrbit* hook
  (Period(), UpdateFromUT()). No Kepler maths in the library. Golden runner uses a stand-in period
  (Orbit.cs:334-370 formula) ONLY to gate tidal-lock rotationPeriod and solarDayLength; orbits table: element
  columns gated, propagated columns excluded (pending WP-07).
- Atmosphere FlightIntegrator helpers: ksp::FlightIntegratorAtmosphere struct (fields named as FlightIntegrator's).

3. Orchestrator directive (original game assets): tools/extract_body_assets.py (harness venv UnityPy, read-only on
   decompiled/KSP-dev/KSP_x64_Data) -> data/kerbol_body_assets.json (biome maps + attribute tables, mini-biomes, PQS
   MapSO/texture refs, scaled mesh/materials/textures, by file+PathID). Generated catalog embeds them (GameAssetRef). DONE.
4. tools/gen_wp09_data.py -> physics_globals_{members,properties}.inc, physics_globals_{load,save,fields}.inc,
   src/celestial/kerbol_system_data.cpp. PhysicsGlobals, VesselRanges, CelestialBody, Planetarium, CelestialSystem,
   FlightIntegratorAtmosphere ported. DONE.
5. Runner datasets physics_constants, physics_drag_curve (capture aero_forces_mono drag_curve_value), body_catalog
   (orbits: elements only via new COLUMNS filter), atmosphere_curves. golden_json numbers now Normalize()d (".0") because
   compare.py keys rows by json.dumps. ALL 14 ctest green, every WP-09 column 0 ULP. DONE.

## Next
- tests/test_celestial.cpp (catalog integrity vs serialized JSON, atmosphere edges, PhysicsGlobals save/load), commit
- STATUS.md WP-09 section + README layout rows, final commit
