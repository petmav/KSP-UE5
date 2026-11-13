# PROGRESS WP-UE-02: game host and C ABI v2 (branch wpue02, from main da6db948)

## Header freeze
First commit (the two ABI headers alone): **643355d8** `core/include/kspcore/kspcore_game.h`, `core/include/kspcore/kspcore_frame.h`.
Compiles as C11 (gcc -pedantic) and C++20. Later header changes are additive only and are listed here.

### Header changes after the freeze
- `kspcore_vessel_state`: appended `altitude`, `surface_speed`, `mach`, `vertical_speed`, `reserved1` (the quantities the C10 gate reads; no HUD view-model exists before WP-UE-04). struct_size leading, earlier fields unmoved.
- `kspcore_c.h`: KSPCORE_ABI_VERSION 1 -> 2, includes kspcore_game.h / kspcore_frame.h at the end (and kspcore_camera.h / kspcore_view.h / kspcore_model.h when present).

## Plan
1. headers (done)
2. host: `core/include/ksp/host/game_host.hpp`, `core/src/host/*.cpp` (product composition of the flight scene), C ABI in `src/capi/kspcore_game.cpp`, `src/capi/kspcore_frame.cpp`
3. ctest `capi.flight_slice` (C only) + C++ hash twin + hygiene re-create test
4. STATUS / README / progress.csv, verification, PR

## Log
