# PROGRESS WP-UE-16a (EFFECTS evaluation in kspcore)

Worktree .worktrees/wpue16, branch wpue16.

## Done
- fx core (FXCurve, EffectList, 15 effect classes, FXGroup state, Part wiring), engine / RCS / decoupler / compound part / animation group / wheel deployment callers, FXModule* modules.
- Gate: tools/wpue16/effects_reference.py -> tests/data/wpue16_reference.json, tests/test_effects.cpp (6 cases) green; mutation check done.
- Fresh build + ctest 58/58 green (core/STATUS.md "WP-UE-16a" has the numbers), WSL g++ -fsyntax-only clean, pr_review.py and check_cites.py clean.
- STATUS section, README rows, progress.csv WP-UE-16 = 0.4, memory note.

## Left (not in this package slice)
- C ABI / snapshot exposure (wpue02 owns the ABI), shell half (WP-UE-16b), a host that calls InitializeEffects / BeginFXGroups / UpdateFX / pause events.
- Callers whose modules are not in core: firework, robot arm scanner, ModuleEffectTest, EVA golf / banana, inventory PlayRandomEffect.
- Emitter base values from the .mu / prefab assets (seams in place).
