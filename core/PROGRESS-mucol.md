# PROGRESS: WP-mucol (real part colliders from .mu files, scenery collider loader)

Branch `mucol`, worktree `core/.worktrees/mucol`, from origin/main `cd5e4df4`.

## Done

- [x] Read WP-12b / WP-20b / WP-20c / WP-17b STATUS sections; PartReader.cs collider chunks (:229-:340), PartLoader.CompileModel (:1765).
- [x] Independent python reference `tools/mucollider/mu_colliders_reference.py` (collider chunks, CompileModel placement, part-space float32
      shapes, mesh mass properties) -> `tests/data/mucollider_reference.json`, reduced real models `tests/data/mucollider/*.mu`, synthetic model.
- [x] C++ reader `ksp/parts/mu_model.hpp` (every PartReader chunk consumed), part-space shapes `ksp/parts/part_colliders.hpp`, physics bridge
      `ksp/physics/mu_part_colliders.hpp` (ColliderProxy extended: Sphere / Capsule / ConvexHull + analytic mass properties; `ShapeDesc::ConvexHull`).
- [x] Scenery loader `ksp/space_center/scenery_colliders.hpp` (JSON schema `kspcore-scenery-colliders/1`) + extractor script (not run).
- [x] Tests `tests/test_mu_colliders.cpp`.
- [x] NEXT-STEPS fixes for WP-38.

## Blocker recorded

The permission layer denied reads of `decompiled/KSP-dev/**` in this session (Read, Glob and Bash all refused). So: no stock-part sweep over all 362
parts, no KSC asset extraction. Only the 19 engine models staged earlier in `core/data/mu_staging/` (WP-17b, sha256 manifest) were available.
`tools/mucollider/sweep_stock_parts.py` and `tools/mucollider/extract_scenery_colliders.py` are the local next steps.

## Verification log

(see STATUS.md "WP-mucol")
