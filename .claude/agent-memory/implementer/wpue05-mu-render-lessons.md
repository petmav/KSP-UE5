---
name: wpue05-mu-render-lessons
description: WP-UE-05 facts - game files reachable only via python -I sweeps, .mu v0-2 extra value per node, stock shader/texture facts, fast gate pattern (rows through the C ABI vs python rows), build dir paths
metadata:
  type: feedback
---

- Gate pattern that worked first time: python writes (key, value) rows from its OWN parse (floats as raw u32 bits, never python floats); C++ dumps the same rows only through the C ABI (tests/mu_dump.hpp); compact mode (arrays as count + FNV-1a 64 of raw bytes) doubles as the per-file digest for the stock sweep, so one script cross-checks C++ vs python on all 551 real .mu with only digests printed.
- Real-file reading: `python -I script --game C:/Projects/KSP-UE5/decompiled/KSP-dev --out x.json` worked from the worktree (derived data only). Debug scripts that print offsets / entry ids only are fine; never print file bytes.
- Windows build tree is `core/build/RelWithDebInfo/{tests,...}` (not `core/build/tests`). `cmake --build` directly lacks the MSVC env: always go through `powershell -File core/run_tests.ps1 -BuildOnly` (incremental ~50 s for a src change). Mutation loop = edit src, run_tests.ps1 -BuildOnly, kspcore_tests -tc="..." (restore the file afterwards).
- Tool output elision: `sed -n a,bp file | sed 's/^ /~/'` shows code bodies; the Read tool elides bodies on .cpp/.hpp.
- .mu versions 0-2 (101 stock files) have one extra 4-byte value after every node header; PartReader skips it as an unknown entry id (harmless). Do not treat "unknown entry" as an error in a .mu reader.
- Stock .mu use 28 distinct shader names (not only the KSP/* list in DESIGN); the DDS loader accepts only DXT1/DXT5 (no BC5); PNG/JPEG/TRUECOLOR format fields live inside Unity's decoder and are inferred.
- MSVC C4701 on `struct x; x.struct_size = ..` test locals: declare `= {}`.
