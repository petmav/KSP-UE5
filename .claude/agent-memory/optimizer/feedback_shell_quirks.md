---
name: shell-quirks-wp20f
description: Bash-tool quirks seen on this laptop: heredocs containing apostrophes fail, a hook elides repetitive output lines, never git stash during a background build
metadata:
  type: feedback
---

Write multi-line edit scripts with the Write tool and run them with `python -I script.py`; do not pass them through a bash heredoc.
**Why:** in WP-20f (2026-10-20) every `python - <<'EOF'` whose body contained an apostrophe (`wheel's`, `module's`) died with
"unexpected EOF while looking for matching `''" even with a quoted delimiter.
**How to apply:** any source edit with prose comments; heredocs are fine only for apostrophe-free text.

A caveman/compression hook elides "similar" lines from tool output ("... N sections elided"); join lines with `tr '\n' '#'` or print 2-4
lines at a time to see them all. Read on whole source files is also elided: use `sed -n a,bp` ranges.

Do not `git stash` while a background build runs in the same worktree (it briefly reverts sources; ninja then rebuilds hundreds of
objects). Timings on this shared laptop vary 2x with other agents' builds: measure per-call costs in a micro loop, not whole ticks.
