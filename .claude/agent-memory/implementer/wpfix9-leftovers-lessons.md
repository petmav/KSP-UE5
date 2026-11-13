---
name: wpfix9-leftovers-lessons
description: wpfix9 (small core leftovers) lessons - verify the C# premise first, test fixtures for Vessel / PartItemTransfer / Kerbal, CallbackUtil int vs float, cite checker quirks, python heredoc traps
metadata:
  type: project
---

- Check the item premise in the C# before porting: `ContractParameter.CreateID`'s `!= null` guard is always true (`HashString` never returns null) and `PartModule.ApplyUpgradeNode` needs the whole `PartUpgradeHandler`; both "small" items were not what the STATUS line said.
- A bare `Vessel v;` needs `v.loaded = true` before `RebuildCrewList` / `CrewWasModified` (null `protoVessel` otherwise: SIGSEGV). `PartItemTransfer::Setup` formats a message with arguments: `Localizer::Format(tag, args)` throws NullReferenceException with no Localizer instance (`inv33::LoadLocalizer()`, and `Localizer::Shutdown()` after: other files assume none). `Vessel()` also adds every registered VesselModule, so tests must look for their module by pointer, not by `vesselModules.size()` (order dependent).
- `CallbackUtil.DelayedCallback(int)` counts FRAMES, the float overload seconds (CrewTransfer waits one frame, Kerbal's overlay 3 s). Pump with `AdvanceFrame` + `RunDelayedDynamicFrameRate` (`eva_fixture.hpp` `Pump`).
- The kerbalEVA prefab's `CrewCapacity` is not in the mirror cfg: set it (SYNTHETIC) when a test moves crew into the spawned EVA part.
- `check_cites.py` takes the function name from the next code line, also for `Class::member(` calls: a `// File.cs:NNN:` comment directly above a call line is checked as a cite. Use `(File.cs line NNN)` wording inside a function; C++ destructors need a `NAME_ALIAS` entry.
- Adding a field to `vessel.hpp` / a header used everywhere costs a ~5 min incremental rebuild; fresh build here was 548 s at 4 parallel with other agents building.
- python on Windows: text-mode `open(...,'w')` writes CRLF (broke a bash script run in WSL); use binary mode. `\\n` in a python heredoc string becomes a real newline in C++ strings (use `chr(92)`). `str.replace` silently does nothing when the pattern does not match: assert the count.
- Test files were LF on disk, `progress.csv` CRLF (edit in binary, assert the old line exists).
- A bogus extra row in compare.py is covered by `core/tools/ci/test_compare_extra_rows.py` (ctest `compare.extra_rows`); the allow-list lives in `harness/compare/compare.py` (`ALLOWED_EXTRA_ROWS`), count exact.
**Why:** each cost a build / debug cycle in wpfix9. **How to apply:** before small cross-cutting leftover work and before writing Vessel / crew-transfer fixtures.
