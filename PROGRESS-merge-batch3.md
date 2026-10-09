# PROGRESS merge batch 3 (wp05, then wp04 if finished)

## Step 1: merge wp05 into main (d25ebbb)
- `git merge --no-ff --no-commit wp05` started. Conflicts (7): README.md, STATUS.md, cmake/RunGolden.cmake,
  src/CMakeLists.txt, tests/CMakeLists.txt, tools/filter_golden.py, tools/golden_runner/CMakeLists.txt.
  Auto-merged: tools/golden_runner/{datasets.hpp,main.cpp}.
- Resolved (uncommitted): src/CMakeLists.txt, tools/golden_runner/CMakeLists.txt, tests sources = unions.
  filter_golden.py rewritten: one option set, fixed step order (tables, key, keep/drop rows, keep-rows-file,
  drop-keys-file, renumber, columns); ROWS -> KEEP_ROWS (rowspec grammar `col=v|v` / `col^prefix`); validation of
  table/column names. Fixed auto-merge bug: wp05's keep-rows-file loop shadowed main's `keys` dict.
  RunGolden.cmake: table-driven option->flag, <OUT> substitution for both file options.
  tests/CMakeLists.txt: unified ksp_golden_gate options; duplicate KSP_CAPTURES2_DIR (wp05) removed; orbit_rails uses KEEP_ROWS.
  golden_runner main.cpp/datasets.hpp auto-merged (--gamedata), no conflict.
  STATUS: WP-05 section after WP-11; header 20 tests; unit row 125/39,849 (to verify). README: tables unioned,
  "### Golden gate options" table added; WP-08 ROWS mentions annotated.
- NEXT: run_tests -Fresh RelWithDebInfo + Debug; equivalence check old vs new filter_golden outputs.
- wp04 check (step 3): NOT finished. .worktrees/wp04 STATUS.md has no WP-04 section; worktree dirty (M CMakeLists.txt,
  src/CMakeLists.txt; untracked include/ksp/save/, src/save/, tools/wp04/ ...); branch tip f964447 (= base, no WP-04 commits).
  Not merged, not touched.
- RelWithDebInfo fresh: 20/20 ctest pass; doctest 125 cases / 39,849 assertions; every gate max_ulp 0, max_abs 0.
  Filter equivalence: new filter_golden.py output == old main filter (math_shim x2, libm_mono, unity_random,
  physics_drag_curve, orbit_rails, terrain_height) and == old wp05 filter (part_database), byte-for-byte JSON equal.
  Note: part_database compare report has 641 extra CANDIDATE rows (parts 5, modules 66, module_fields 565, assets 5);
  compare.py does not fail on extras; pre-existing wp05 behaviour (same runner + same filtered golden).
- Debug fresh: running.
- Debug fresh: 20/20 ctest; doctest 125/39,849; every gate max_ulp 0. Filter negative checks: unknown table/column/option rejected.
- STATUS: "### Merge into main (after WP-11)" subsection added at end of WP-05 section.
- NEXT: commit merge (PROGRESS file stays untracked until final docs commit).
- Merge commit: bfb732f (Fable 5.1 trailer, as the earlier wp08/wp11 merges). Docs commit records the hash.

## Step 2: README "Golden gate options" table: done inside the merge commit (bfb732f).

## Step 3: wp04 NOT finished (see above). Not merged, worktree not touched.
