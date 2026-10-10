---
name: shell-quirks-bash-tool
description: Bash tool on Windows chokes on heredocs with apostrophes/backslash-n; use the Write tool for C++/markdown with such text
metadata:
  type: feedback
---

Heredocs (`<<'EOF'`) that contain an apostrophe in prose (e.g. "bodies' ") can fail with "unexpected EOF while looking for matching `'`" and write nothing. A python `\\n` inside a heredoc ended up as a real newline in C++ string literals.

**Why:** hit repeatedly in WP-22 (broken printf format strings, files silently not written).
**How to apply:** write source/markdown files with the Write tool; for in-place edits from python build the backslash with `bytes([92])` or `chr(92)`, write a script file first, and edit CRLF files in binary mode (working copy is CRLF, `\n` patterns miss). Verify with grep after editing.
