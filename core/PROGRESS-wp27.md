# PROGRESS wp27 (Parachutes, deployables, animation-driven modules)

- [x] read specs (ModuleDeployablePart/SolarPanel/Antenna/Radiator, ModuleParachute/EvaChute, AnimateGeneric, AnimationGroup, AnimationSetter, LandingGear)
- design: unity/animation.hpp (AnimationState/Animation model, host ticks), modules/animated_module.hpp (IAnimatedModule),
  modules/deployables.{hpp,cpp}, parachutes.{hpp,cpp}, animation_modules.{hpp,cpp}; IScalarModule gains OnMoving/OnStop (cargo bay hook)
- [ ] code, [ ] gate (capture.part_database), [ ] python ref, [ ] unit tests, [ ] STATUS/README
- [x] code compiles, capture.part_database green with real classes (Windows)
- [x] tests/test_deployables.cpp + tools/wp27/deploy_reference.py written, WP-27 unit tests pass (Windows)
