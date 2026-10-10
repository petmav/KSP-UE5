---
name: wp-workflow-notes
description: Practical tooling notes for implementing KSP-UE5 work packages on this Windows laptop (build times, heredoc/python quirks, gate patterns)
metadata:
  type: reference
---

- Windows build in a worktree: `powershell -NoProfile -ExecutionPolicy Bypass -File core\run_tests.ps1 [-BuildOnly] [-TestFilter x]` (cold build ~90 s, incremental 5-25 s). Full ctest ~75 s. WSL `./run_tests.sh -f` on /mnt/c takes ~25 min (build 230 s, tests 1250 s).
- Bash heredocs containing many quotes/backticks can die with "unexpected EOF"; write files with the Write tool instead. `python3` does not exist, use `python` (Windows python rewrites files with CRLF; git normalizes via .gitattributes).
- Python scripts in /tmp: bash maps /tmp to C:\Users\pmavr\AppData\Local\Temp; inside python use that full path.
- Gate pattern for float32 modules: template the maths over aero::MonoFloat (game, captures) / aero::NetFloat (.NET harness golden); same code passes both at 0 ULP.
- ksp_golden_gate capture string columns: capture uses R() formatting (no trailing ".0"); ExactFormatFloat appends ".0".
- Pre-existing main bug fixed in WP-23: IAirstreamShield was defined in both staging/staging_types.hpp and aero/aero_interfaces.hpp.
