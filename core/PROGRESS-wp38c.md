# PROGRESS WP-38 slice 3 (FinePrint remainder), branch wp38c from wp38b (main a5d23742 merged)

Scope: (1) Sentinel, (2) full ContractDefs, (3) CompatibilityUtilities, (4) VesselRepair / OrbitalConstruction / RoverConstruction templates + PreBuiltCraft model, (5) CometSample generation test.
Build: CMAKE_BUILD_PARALLEL_LEVEL=6 powershell -File run_tests.ps1 -BuildOnly ; tests: build/RelWithDebInfo/tests/kspcore_tests.exe -tc="WP-38c*"
- [x] (2) ContractDefs: tools/wp38c/gen_contract_defs.py (C# -> contract_defs_members.inc + src/fineprint/contract_defs_gen.cpp: writer, reader, Add*Requests, registry), contract_defs.{hpp,cpp}; ContractDefsLite delegates to the typed members; reference ref_contractdefs.py -> tests/data/wp38c_reference.json; 5 cases / 4,872 assertions; full suite 676 cases / 1,150,400 assertions green (every earlier gate bit-identical)
- [x] (1) Sentinel: sentinel.hpp + sentinel_{utilities,contracts,scenario}.cpp, ref_sentinel.py; 1,638 contract cases (540 generated) 0 ULP + behaviour tests
- [x] (3) CompatibilityUtilities: compatibility.{hpp,cpp}, ref_compat.py (3,744 upgrade cases byte-exact, 600+ situation rows); hooks installed by InstallAll
- [x] (4) prebuilt.{hpp,cpp}, construction_templates.{hpp,cpp}, ContractDefs loaders; ref_templates.py (2,304 generation cases, synthetic craft fixtures), behaviour tests
