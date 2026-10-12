# PROGRESS wpfix8 (sanitizer cleanup)

- WSL tree ~/wpfix8 (rsync of the worktree), build dir ~/wpfix8/asan, g++ 13.3, flags `-fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g`.
- Per-case sweep (one process per case, -f=N -l=N): 36 failing cases first, all fixed (see STATUS.md "Sanitizer run 2026-10-12").
- Whole binary in one process: 779 cases / 1,307,562 assertions, ASan + UBSan + LSan clean.
- Remaining: Windows `run_tests.ps1 -Fresh` gate, re-run the WSL sweep after the last ContractParameter edit, commit.
