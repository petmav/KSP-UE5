# WP-37 gate progress (gatekeeper)

- HEAD 679c45bb, main dcef6d17 contained; merge-tree main+wp37: clean.
- Static checks done: no std:: transcendental in src/contracts (only exact sqrt/ceil, as Math.Sqrt/Math.Ceiling), tolerances / harness / decompiled / ue5 untouched,
  reference regeneration byte-identical, check_cites 497/0 (cpp), headers 0 mismatches, faithfulness spot-check (see final report).
- Running: Windows -Fresh, WSL clang then gcc (logs /tmp/gate37_clang.log, /tmp/gate37_gcc.log; script .worktrees/gate37.sh).
- Windows -Fresh: 34/34, unit 565 / 935,110 (measured), configure 3.3 s, build 169.5 s, test 261 s. 27 compare reports: all 0 except deltav_stages (same as main); part_database + deltav_stages reports identical to main's.
- WSL clang: ctest 34/34 (unit 3207 s, WSL starved by other agents' ASAN runs). Totals run pending (pid 109449 started by gatekeeper). gcc run pending.
- CI: windows success, linux fails with SIGSEGV test_part.cpp:307 (same as main).
- main moved to bb923bc2: merge-tree conflicts only in core/STATUS.md (line 82 unit row, ~3925 section placement); check_cites.py auto-merges, but main's exact default fails 116/497 wp37 cites (needs --fix).
- WSL clang: 34/34, unit 565 / 935,094 (configure 171 s, build 340 s, test 3230 s). WSL gcc: 34/34, unit 565 / 935,094 (configure 9 s, build 206 s, test 2864 s). Windows 565 / 935,110. Linux -16 as usual.
- All 27 reports on all three builds: 0 except deltav_stages (rel 1.02e-6, same as main). part_database identical to main.
- Docs edited: STATUS unit row + Verification (measured numbers, CI result, cite-checker note). Verdict: MERGE after STATUS.md conflict resolution (main bb923bc2) and check_cites --fix.
