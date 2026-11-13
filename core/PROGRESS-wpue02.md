# PROGRESS WP-UE-02: game host and C ABI v2 (branch wpue02, from main da6db948)

## Header freeze
First commit (the two ABI headers alone): **643355d8** `core/include/kspcore/kspcore_game.h`, `core/include/kspcore/kspcore_frame.h`.
Compiles as C11 (gcc -pedantic) and C++20. Later header changes are additive only and are listed here.

### Header changes after the freeze
- `kspcore_vessel_state`: appended `altitude`, `surface_speed`, `mach`, `vertical_speed`, `reserved1` (the quantities the C10 gate reads; no HUD view-model exists before WP-UE-04). struct_size leading, earlier fields unmoved.
- `kspcore_snapshot_hash` doc: vessel / part ids are NOT hashed (process-wide counters differ between sims in one process); the hashed fields are listed in the comment. Behaviour is documented as implemented (no consumer existed at the freeze).
- `kspcore_c.h`: KSPCORE_ABI_VERSION 1 -> 2, includes kspcore_game.h / kspcore_frame.h at the end (and kspcore_camera.h / kspcore_view.h / kspcore_model.h when present).

## Plan
1. headers (done)
2. host: `core/include/ksp/host/game_host.hpp`, `core/src/host/*.cpp` (product composition of the flight scene), C ABI in `src/capi/kspcore_game.cpp`, `src/capi/kspcore_frame.cpp`
3. ctest `capi.flight_slice` (C only) + C++ hash twin + hygiene re-create test
4. STATUS / README / progress.csv, verification, PR

## Log
- header freeze 643355d8; host + ABI implementation 96bbeb2b: C test first reached ignition after fixing TimeWarp Awake/Start (curr_rate 0 -> no propellant) and CommNet (EnableCommNet off).
- hash: ids not hashed, GUID seed per sim; FlightGlobals speed modes reset at scene end; FlightDriver statics kept across the teardown (quickload).
- quicksave (on the pad: KSP refuses it under thrust), quickload, revert to launch, Kerbal X (upgrade pipeline + crew fill) all through the ABI.
- fresh Windows suite (core/build deleted): 55 / 55 pass; capi.flight_slice 44.8 s, capi.flight_slice_twin 54.8 s.
- remaining: random-order runs of the whole unit binary (-rs=11, -rs=27), STATUS section (draft core/_status_wpue02.md), merge origin/main, push, PR.
