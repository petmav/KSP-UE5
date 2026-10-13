# PROGRESS wp29a (VPP wheel core + KSPWheelController) - branch wp29a, worktree core/.worktrees/wp29a, base main b9119103

- [x] independent Python reference written first from the C# (tools/wp29a/vpp_reference.py -> tests/data/wp29a_reference.json)
- [x] C++ port include/ksp/wheels, src/wheels (6 files), written from the C#
- [x] tests/test_wheels.cpp (12 cases / 7,513 assertions), ctest wheels.vpp; 0 ULP; mutation checks fail as expected
- [x] check_cites exact: 215 cites, 0 mismatches (wp28 and wp55 wrappers)
- [x] unit 822 cases / 1,374,044 assertions: default, -rs=11, -rs=27 all pass
- [x] g++ ASan/UBSan/LSan in WSL: no report in wheel code (22 unrelated data-dependent cases fail in the partial WSL copy)
- [x] STATUS row 49 + section, README entry
- [ ] push (coordinator)
