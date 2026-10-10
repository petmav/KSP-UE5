---
name: wsl-launch-from-gitbash
description: Starting the long WSL run_tests.sh from Git Bash tool - path mangling and detached-process pitfalls
metadata:
  type: feedback
---

Git Bash rewrites `/mnt/c/...` args to `C:/Program Files/Git/mnt/c/...`. Put the WSL command in a script under `.worktrees/` (outside the repo) and launch
`MSYS_NO_PATHCONV=1 wsl -e bash /mnt/c/.../script.sh` with `run_in_background: true`. `wsl -e bash -c "... &"` or nohup/setsid children die when the wsl call returns.
**Why:** WP-40 lost several attempts to a log that never appeared. **How to apply:** log lives in the WSL /tmp, read it with `wsl -e bash -c "tail ..."`.
