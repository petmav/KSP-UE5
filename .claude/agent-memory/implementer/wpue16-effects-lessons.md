---
name: wpue16-effects-lessons
description: WP-UE-16a EFFECTS port - digest-per-step gate over the stock cfgs with hash fixtures, tooling traps (python heredocs, 20 min rebuilds), C# quirks the port keeps
metadata:
  type: reference
---

- Gate shape that worked first run: independent python writes per scenario `steps` (strings) + a FNV-1a 64 digest and a per-step digest string (8 hex per step); C++ replays the same step strings through the real classes and compares; a mismatch prints the first differing step and the `--dump <part> <scenario>` command. Mutation check (swap one operand) fails at step 3-4, so the gate bites. Inputs the mirror lacks (transform counts, clip lengths, particle bases) are hash functions of names, written twice (python FIX_*, C++ InstallFixtures).
- zDeprecated stock parts are in the mirror cfgs but NOT loaded by PartLoader (`getPartInfoByName` null): read EFFECTS from the cfg file for list scenarios, skip with a CHECK that the path contains zDeprecated for module / group scenarios. `Game15::New("kerbalEVA")` crashes for group tests: use a bare `Part` + OnLoad + copied fx/sound definitions.
- Editing `part.hpp` rebuilds everything (20+ min at -j4). Batch header edits; never edit headers while a build runs. A python heredoc with apostrophes or `\n` inside breaks the Bash tool: write the patch script with the Write tool, run it with `python file.py`; sed for one-line changes.
- Static pools: destroy the EffectList BEFORE resetting AudioMultiPooledFX's pool (destructor releases into it) or make the dtor range-check.
- C# quirks kept and gated: FXCurve ctor ignores valueName; one-line two-token curve returns the time; unnamed same-class nodes under one effect share one behaviour; PREFAB_PARTICLE speed curve is a no-op (struct copies); PARTICLE_CONFIG OnEvent clamps the parameter not the field; animation OnSave self-copy doubles values; ModuleRCSFX.DeactivateFX does not zero thrustForces; Pool.Size is the high-water mark of the free stack.
- check_cites.py maps C++ names to C# names: overloads (OnEventPower -> OnEvent) need NAME_ALIAS entries; comments citing a body line (not a declaration) should be prose, not `// File.cs:N`.
