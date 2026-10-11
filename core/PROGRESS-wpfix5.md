# PROGRESS wpfix5 (small fixes, wave 5)

- Baseline (main 7199d110 tree) Windows: 505 cases, 868,698 assertions, 34/34 ctest.
- Item 2 done (commit SqrtF): per-case counts identical before/after, 0 failures.
- Item 3 done: check_cites.py exact mode + --window; audit 677 mismatches (472 off by k) over 182 files; fixed editor_gizmos / editor_logic only.
- Item 5, 7, 6 done (tests). Item 4 done: driver stamped the synchronous first packet with pass-0 index; fixed, gaps now exact.
- Item 1: waiting for the WSL per-case xml (unit takes ~1 h on /mnt/c).
- Item 1 DONE: WP-03 reference test NaN-sign fallback (CellEquals/Dbl), 16 = 4 rows x 4 assertions. Documented in STATUS.
- Final: Windows -Fresh 34/34, 506 cases / 868,747; WSL ./run_tests.sh -f 34/34, 868,731 (= -16).
