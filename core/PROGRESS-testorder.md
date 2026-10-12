# PROGRESS testorder (branch testorder)

Goal: the doctest `unit` binary passes under --order-by=rand (any seed), --order-by=name and default.

- Found by `-rs=N --order-by=rand` + a bisect over `--first/--last`: order-dependent cases and causes are listed in STATUS.md (Test hygiene: random order).
- Shared net: tests/test_hygiene.{hpp,cpp} (doctest listener resetting shared state before/after every test case; KSP_HYGIENE_TRACE=1 reports what leaked).
- Done: prefab positions, FlightGlobals, SaveEnvironment, Localizer, CompatibilityUtilities versions, TemplateEnvironment, R&D statics, isStageable cache, skills-readable caches, event listeners (truncate to the run-start baseline), ModuleFactory KerbNet registration (test_part LifecycleRegistration).
- Remaining: ctest entry, STATUS/README, final fresh verification.
