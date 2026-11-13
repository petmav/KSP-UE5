# PROGRESS WP-UE-05 (.mu render reader, texture headers, kspcore_model.h)

Branch wpue05 (worktree .worktrees/wpue05), from main da6db948.

Plan / state (update after each step):
1. [x] Extend ksp::mu reader (mu_model.hpp/.cpp): RenderMesh (all arrays), MeshRenderer, SkinnedMeshRenderer (bones, weights, bind poses),
   Materials (v<4 legacy table -> property list, v>=4 generic), Textures chunk, Light, Camera, ParticleEmitter (count + material slot only),
   anomalies / ignored entries recorded. Collider gate (tests/test_mu_colliders.cpp) must stay bit-identical.
2. [x] Texture header decode: include/ksp/parts/texture_header.hpp + src/parts/texture_header.cpp (DDS, MBM, PNG, JPEG, TGA, TRUECOLOR).
3. [x] C ABI (written; compile pending): include/kspcore/kspcore_model.h + src/capi/kspcore_model.cpp (own last-error: kspcore_c.cpp belongs to wpue02).
4. [x] Independent python reference tools/wpue05/mu_render_reference.py (own parser + synthetic writer + texture header ref) -> tests/data/wpue05_reference.json.
5. [~] Tests (test_mu_render.cpp, mu_dump.hpp, capi/model_c_test.c, capi/mu_sweep.cpp written; not yet built/run): tests/test_mu_render.cpp (+ tests/mu_dump.hpp shared flatten via the C ABI), CMake rows, C compile test of the header.
6. [~] Sweep (tools/wpue05/sweep_stock.py + mu_sweep tool written; not yet run): tools/wpue05/sweep_stock.py (+ C++ tool mu_sweep) -> data JSON; run with --game C:/Projects/KSP-UE5/decompiled/KSP-dev.
7. [ ] STATUS.md section, README rows, progress.csv WP-UE-05, self-verification checklist, PR.

Design notes:
- Tool output of Read is elided for code bodies: use `sed -n a,bp file | sed 's/^ /~/'`.
- Gate flattening: one dump helper walks the model through the C ABI (rows key=value; floats as %08x bits). Python writes the same rows. Compact mode
  (arrays -> count + FNV-1a64 of raw bytes) is the per-file digest for the stock sweep.

## State 2026-11-13 (after first green run)
- Library, tests, tools build. `kspcore_tests -tc="WP-UE-05*"`: 8 cases / 3301 assertions pass; mutation checks (spot angle version rule, texture scale/offset
  swap, bone lookup last-match, default-clip-applied) fail 7 / 26 / 5 / 5 assertions respectively (restored; build/mutate.sh).
- Stock sweep ran: data/wpue05_stock_sweep.json (551 .mu, 0 parse errors, python vs C++ digests equal on all, 1354 texture headers equal).
- FINDING: all 101 files of .mu version 0-2 carry ONE unknown 4-byte value after every node header (683 runs of length 1, root 101 + child 582): PartReader
  has no case for it and skips 4 bytes (entry id). Rotation unit-norm and scale sane for all those nodes. Node-level ignored entries therefore NOT 0.
- TODO: README rows, STATUS section, progress.csv, WSL g++ -fsyntax-only, gcc -x c header, pr_review, fresh build + random orders, commit/merge/push/PR.
