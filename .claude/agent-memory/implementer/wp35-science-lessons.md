---
name: wp35-science-lessons
description: Non-obvious facts from the WP-35 science/R&D port: test scaffolding traps, C# facts that differ from the task text, cite-number pitfalls
metadata:
  type: project
---

- ModuleScienceLab in 1.12.5 has no GenerateScience maths: SurfaceBonus/ContextBonus/homeworldMultiplier are applied in ExperimentResultDialogPage.UpdatePageLabValue, science generation is ModuleScienceConverter.PostProcess. Read the C# before trusting a work-package description.
- `TestVessel` (tools/support/craft_tree.hpp) installs `ResourceFlowEnvironment::vesselHost` and never resets it: reset it (`= nullptr`) in any fixture that later builds a REAL `Vessel` (World25-style Stack), else SIGSEGV that only shows when tests run together.
- The game database localises cfg values (ScienceDefs RESULTS texts, titles): compare against `Localizer::Format(raw tag)` when a Python reference reads the raw cfg.
- `FlightGlobals::ActiveVessel` is `IFlightVessel*` (not Vessel); the real one is `FlightRegistry::ActiveVessel` (Vessel*).
- ModuleAnimateGeneric.SetScalar needs `DeployablesEnvironment::modelAnimations` (no .mu models): clear `fxModuleIndices` of the goo/experiment or use a fake IScalarModule in tests.
- C# `(double)(floatExpr)` and a float expression returned from a double method are NOT float stores (ModuleScienceConverter.CalculateScienceRate/Amount).
- When listing a C# file with `cat -n a.cs b.cs`, the second file's numbers continue from the first: wrong `File.cs:N` cites. Use `awk 'NF{print NR": "$0}' file` per file, or run `check_cites.py --fix` (it also mis-picks constructor lines: the class line instead of the ctor; check ctors by hand).
- Writing test text with apostrophes: use the Write tool to a scratch file and `cat >>` it; heredocs with `'` break the Bash tool.
- Part instance from `Game15::New` gets modules by Instantiate: public serialisable non-KSPField members (lists, processResources) need `CopySerializedFrom` or they are empty on instances.
- Stock part internal names: `Large_Crewed_Lab` is `Large.Crewed.Lab`, `mk1pod_v2` is `mk1pod.v2`.

**Why:** each cost a debug cycle in WP-35. **How to apply:** before writing module tests / citing C# lines.
