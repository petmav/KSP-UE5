---
name: kspcore-module-porting-patterns
description: How PartModule packages are ported and tested in kspcore (host seam, test fixtures, traps found in WP-28)
metadata:
  type: project
---

- Module package pattern (WP-15/28): header in `include/ksp/modules/`, `.cpp` in `src/modules/`, registered via a `detail::Register...Modules()` called from
  `RegisterBuiltinModules` in `module_behaviours.cpp`; fields are string-keyed (`Get<T>("name")`), Unity messages (FixedUpdate/Update) are public methods the host calls.
- `Part::vessel` is only a fake `TestVessel*` in tests (reinterpret_cast), so modules read vessel state through host records/hooks (`ResourceProcessingEnvironment`
  in WP-28), never through `Vessel*` directly.
- Test fixtures: `tests/resource_fixture.hpp` (`Game15`, `SceneScope`, `LogCapture`), `tools/support/craft_tree.hpp` (`TestVessel`), stock prefab modules via
  `ap->partPrefab->ModuleData`, runtime parts via `g.New(name)` + `part.AddModule(cls, true)`.
- Unit-gate recipe that worked: independent Python reference (numpy float32 `st()`) + JSON in `tests/data/`; the Python found a real KSP quirk (first catch-up
  step runs with IsActivated=false).
- Trap: `PartModule::CopySerializedFrom` originally did not copy `resHandler` lists (fixed in WP-28); static test state (`ResourceProcessingEnvironment`, ResourceMap lists,
  `ResourceCache::AbundanceCache`) leaks between doctest cases: reset it per test.

**Why:** saves re-deriving the seams in the next module package (WP-23/24/25/27...).
**How to apply:** start a new module package from `src/modules/resource_*.cpp` and `tests/test_resource_processing.cpp`.
