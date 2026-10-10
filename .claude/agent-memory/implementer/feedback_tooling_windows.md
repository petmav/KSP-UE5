---
name: tooling-windows-quirks
description: Shell and build quirks when implementing KSP-UE5 work packages from the Windows Bash tool (heredocs, python, incremental builds)
metadata:
  type: feedback
---

On the Windows Git Bash tool: `python3` does not exist, use `python` (3.10/3.12). Long heredocs that contain apostrophes or backticks can fail with
"unexpected EOF"; write big files (test chunks, STATUS sections, python scripts) with the Write tool to a `*.tmp` file and then `cat >>`/run a tiny script.

**Why:** two heredoc appends silently did nothing during WP-28 and cost a build cycle.

**How to apply:** after any `cat >> file <<EOF`, check `wc -l` / `tail`. Iterate with `run_tests.ps1 -BuildOnly` (no `-Fresh`, ~1-3 min incremental) and run
`build/RelWithDebInfo/tests/kspcore_tests.exe -tc="WP-NN*"` directly; reserve the full `run_tests.ps1` for the end.
