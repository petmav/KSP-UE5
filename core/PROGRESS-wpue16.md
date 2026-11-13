# PROGRESS WP-UE-16a (EFFECTS evaluation in kspcore)

Worktree .worktrees/wpue16, branch wpue16. Merged origin/main (WP-UE-05) at start.

## Done (written, compiling)
- include/ksp/fx/{fx_curve,effects,effect_classes,fx_group}.hpp, src/fx/*.cpp: FXCurve, EffectList + 15 effect classes, FXGroup state, Part::Effects / InitializeEffects / LoadEffects / SaveEffects / Effect (+ effectCallHook), Part::FindFXGroup / AddFXGroup / BeginFXGroups / UpdateFX.
- Engine wiring: ModuleEngines legacy FXGroup path (SetupFXGroups, Play*FX, FXUpdate ...), ModuleEnginesFX Effect calls (OnAwake, Play*, FXUpdate, Deactivate*, InitializeFX), FixedUpdate now ends with FXUpdate (part_forces.cpp no longer calls it). RCS: virtual SetupFX / UpdatePowerFX / DeactivateFX / DeactivatePowerFX, ModuleRCSFX overrides.
- tools/wpue16/effects_reference.py (independent) -> tests/data/wpue16_reference.json; tests/test_effects.cpp (5 cases).

## Todo
1. run kspcore_tests -tc="WP-UE-16a*", fix mismatches (python dump: --dump <part> <scenario>).
2. Other callers: decouplers fx.Burst, CompoundPart Effect(disconnected), ModuleAnimationGroup, WheelDeployment, Firework, RobotArmScanner, KerbalEVA golf/banana, ModuleEffectTest, ModuleInventoryPart PlayRandomEffect.
3. FXModule* (AnimateThrottle, AnimateRCS, ThrottleEffects, LookAt, ConstrainPosition) as modules with seams.
4. Full suite fresh, random order, g++ -fsyntax-only in WSL, pr_review.py, check_cites.py, STATUS section, README rows, progress.csv, merge origin/main, push, PR.

## Facts
- Stock EFFECTS: 52 parts, 158 effect names; classes AUDIO 151, PREFAB_PARTICLE 37, MODEL_MULTI_PARTICLE 28, AUDIO_LOOP 14, AUDIO_MULTI_POOL 4, MODEL_PARTICLE 1; no unknown class; no collapsed unnamed duplicates.
- Quirks kept: FXCurve ctor ignores valueName; one-line curve = first token; unnamed same-class nodes share one behaviour; PREFAB_PARTICLE speed no-op; PARTICLE_CONFIG OnEvent clamps the parameter; animation OnSave self-copy; AudioFX polyphony<=0 throws; ModuleRCSFX.DeactivateFX does not zero thrustForces; high warp RCS only DeactivatePowerFX.
