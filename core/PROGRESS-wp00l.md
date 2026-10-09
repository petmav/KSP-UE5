# PROGRESS wp00l
- [x] read docs; WSL has g++13 clang18 cmake ninja python3.12 (no sudo needed)

- [x] presets linux-rel/dbg/gcc-rel, run_tests.sh, clang preference in CMakeLists
- [x] clang linux build compiles (fixed PhysicsGlobals SurfaceCurvesList NSDMI, MapSO Origin default args)
- NOTE: bash heredoc python eats double backslashes; use Edit tool for backslash content
- [x] PathFromUtf8 helper (text.hpp) used for fs paths; linux-rel: 11/20 pass. todo: capi crt=MD check, libm_mono, unity_random, physics_constants, orbit_rails, part_database, unit
- libm plan: LIFT msvcr120 x64 machine code (capstone, tools/libm/*.py) into generated portable C++ (Cpu struct, emulated stack, rdata blob); verify vs DLL on Windows w/ random inputs + capture. exports: tools/libm/pe_tool.py; glibc mismatch (of 3093): asin21 acos18 atan115 sinh540 cosh407 tanh183 exp19 log0 log10 246 tan130 atan2 10/5 pow ~5-8
- libm lift design: tools/libm/{pe_tool,lift_discover,callgraph,census}.py written. Discovery: 26 funcs/4024 insns from 12 exports of C:\Windows\System32\msvcr120_clr0400.dll (image VA base 0x180000000; tables via lea rdx,[rip-X] = image base). Stubs: CRT infra (a2488 handle_error => xmm0=bits(r8); a2330/a22b4/a2294/a2350/a2364/a23cc/a56d0/a2700/a2710/a271a/a5a28/297d0(errno)/24520/72230). FMA3 flag = cmp dword [rip+..],0 at function start (jne => FMA path); make runtime-switchable.
- next: write src/dotnet/vs2013_lift.hpp (Cpu, ld/st, flag helpers) + tools/libm/gen_lift.py -> src/dotnet/vs2013_libm_gen.cpp + _data.inc; wire LibmBackend; compare to DLL on Windows w/ random inputs
- generated src/dotnet/vs2013_libm_gen.cpp + _data.inc, vs2013_lift.hpp, include/ksp/dotnet/vs2013_libm.hpp; added to src/CMakeLists. next: libm_check tool vs DLL on Windows; wire math.cpp (KSP_LIBM_PORTABLE)
- !! lifted libm bit-exact vs msvcr120_clr0400.dll on 20k random/fn (libm_check tool, tools/libm_check). segments: 3 (rdata b7ca0.., be530.., data e4fb0). next: big N, fma3=0 sensitivity, Linux build+ctest, force-portable on Windows
- Windows ctest 20/20 with KSPCORE_LIBM_PORTABLE=1 (env) too. next: Linux build/test; physics_constants path; mirror
- libm DONE: msvcr120 (12 fns) + ucrt sinf/cosf/powf lifted, bit-exact vs DLLs on Windows (1-2M inputs/fn, tools/libm_check, FMA3 path; fma3=0 differs for exp/tan/pow). generator: tools/libm/gen_main.py {msvcr120|ucrt}. random.cpp/quaternion_native use dotnet::NativeSinf/Cosf/Powf. TODO: ctest libm.portable_vs_dll, linux ctest, physics_constants path, part_database 'resolved', mirror, CI, root files, docs, commit
- committed checkpoint 1 (linux build + libm). next: Linux ctest full; mirror; CI; root files; docs
- mirror built (testdata/GameData-mirror, 535 cfg, 2804 GD files, dds/mbm header stubs needed), tests use ksp::testing::GameRoot() (tools/golden_runner/game_root.hpp, env KSP_GAMEDATA_ROOT). Windows 21/21 with mirror. next: Linux fresh run + gcc; portable-forced windows build; CI; root files; docs; commit
- README/STATUS updated. todo: CLOUD-TASKS.md, Debug+gcc-dbg check, commit, final fresh runs both platforms, report
- Debug win + linux-dbg 21/21. CLOUD-TASKS written. todo: final fresh RelWithDebInfo win + linux-rel, commit, report
