# PROGRESS wp29b (KSP wheel part modules) - branch wp29b, worktree core/.worktrees/wp29b, base wp29a 2d90e6fe (local, unpushed)

- [x] read the C# (ModuleWheelBase, ModuleWheels/*, SuspensionLoadBalancer, WheelType) and the WP-29a / WP-06 / WP-15 / WP-27 infrastructure
- [x] independent Python reference written first, from the C# (tools/wp29b/wheel_reference.py -> tests/data/wp29b_reference.json, 11 MB)
- [x] C++ port: include/ksp/wheels/wheel_modules.hpp, src/wheels/wheel_modules.cpp (subsystems, util, load balancer, environment), wheel_base.cpp, wheel_submodules.cpp; registered in module_behaviours.cpp (replaces the WP-06 RegCurves of Steering / Motor / MotorSteering)
- [x] tests/test_wheel_modules.cpp (reference replay) + tests/test_wheel_modules_derived.cpp (derived cases); ctest wheels.modules
- [x] mutation checks: radius quirk, torque rounding (setup and tick), damper constant, stress timing, gee formula, steering last input, sign: all caught; the steerAngle > 90 fold is unreachable (acos of a non-negative argument)
- [x] check_cites exact (wp28 and wp55 wrappers): 155 cites, 0 mismatches (name aliases added to tools/wp28/check_cites.py for the camelCase private members)
- [x] full ctest 40/40; unit 832 cases / 1,383,876 assertions in default, rs=11, rs=27 order; g++ ASan/UBSan/LSan: 0 reports
- [ ] push (coordinator; nothing is pushed from this session)
