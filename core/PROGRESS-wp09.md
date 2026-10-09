# WP-09 progress (worktree core/.worktrees/wp09, branch wp09)

State: DONE for the assigned scope. 14/14 ctest green in RelWithDebInfo and Debug; every WP-09 gate 0 ULP.
Details, exclusions and the Orbit interface: STATUS.md section "WP-09".

## Step log
1. Read CLAUDE.md, NEXT-STEPS.md, core README/STATUS, WP-09 rows, harness READMEs/generators (BodiesGen, AtmosphereGen,
   ThermalGen, Rehosted/CelestialBodyLifecycle, KspEnv), serialized JSON layout.
2. CMakeLists.txt: KSP_PROJECT_ROOT walks up to harness/compare/compare.py (worktree-safe). Baseline 10/10 green.
3. Orchestrator directive (original game assets): tools/extract_body_assets.py (harness venv UnityPy, read-only on
   decompiled/KSP-dev/KSP_x64_Data) -> data/kerbol_body_assets.json; the generated catalog embeds the origins.
4. tools/gen_wp09_data.py -> physics_globals_{members,properties}.inc, physics_globals_{load,save,fields}.inc,
   src/celestial/kerbol_system_data.cpp (reproducible: rerun gives identical files).
5. Ported PhysicsGlobals, VesselRanges, CelestialBody, Planetarium, CelestialSystem (spawn), FlightIntegratorAtmosphere.
6. Runner datasets physics_constants, physics_drag_curve, body_catalog, atmosphere_curves; COLUMNS filter in the gate;
   golden_json numbers normalised (".0") because compare.py keys rows by json.dumps.
7. tests/test_celestial.cpp (8 cases, 5,744 assertions). Commit 663d4c2 (code) + docs/tests commit.

## Decisions
- Catalog = generated C++ data (no JSON parser in the library), checked bit-exactly against the serialized JSON by a
  unit test and by the body_catalog gate.
- PhysicsGlobals members initialised from the level0 prefab; statics from C# initialisers.
- Orbit: CelestialBody holds serialized elements + ICelestialBodyOrbit* link (WP-07 glue). Gate-only stand-in period in
  tools/golden_runner/celestial_env.cpp; orbits table gated on elements only.

## Next (outside this package)
- WP-07 merge: adapter ksp::Orbit -> ICelestialBodyOrbit, orbitFactory in CelestialEnv, drop the COLUMNS filter.
- GetAtmoThermalStats after native Quaternion.AngleAxis + C4 capture; biome GetAtt after the MapSO decoder.
