---
name: bash-apostrophe-heredoc
description: Bash tool in this environment fails to parse commands (even quoted heredocs and python -c) that contain apostrophes; use Write for files with apostrophes
metadata:
  type: feedback
---

Bash tool commands containing an apostrophe anywhere (even inside `<<'EOF'` heredocs or `python -c "..."` strings) failed with
"unexpected EOF while looking for matching `''"` and NOTHING executed (so files/dirs from that command do not exist).

**Why:** observed repeatedly in WP-00 (comments like "Unity's", 'SpaceCenter').
**How to apply:** write source/doc text with the Write tool (create parent dirs with a separate Bash `mkdir -p` first); keep Bash commands
apostrophe-free; use python scripts with double-quoted text and no apostrophes for edits. Also: a command that fails to parse leaves
earlier `mkdir` in the same command undone.
