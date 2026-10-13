# Wheel / landing-leg spike (core/WHEEL-SPIKE.md). Opt-in like the Jolt spike: built only with -DKSPCORE_BUILD_SPIKES=ON (presets
# `spike-jolt` / `spike-jolt-f32`), never part of run_tests.ps1 / ctest / CI. Reuses the Jolt spike's scene pieces (kspcore
# context, craft model, Jolt world, PQS terrain patch) by compiling them into this target; Jolt is the vendored copy.

add_executable(wheel_spike
    src/main.cpp
    src/wheel_model.cpp
    src/rover.cpp
    src/scenarios.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../jolt/src/jolt_world.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../jolt/src/ksp_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../jolt/src/craft_model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../jolt/src/terrain_patch.cpp
)
target_link_libraries(wheel_spike PRIVATE kspcore Jolt)
ksp_set_build_flags(wheel_spike)
target_include_directories(wheel_spike PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/../jolt/src)
if(MSVC)
    target_compile_options(wheel_spike PRIVATE /wd4100 /wd4324 /wd4127)
endif()
target_compile_definitions(wheel_spike PRIVATE
    KSPCORE_DEFAULT_GAME_ROOT="${KSP_GAMEDATA_ROOT}"
    KSPCORE_HARNESS_DIR="${KSP_HARNESS_DIR}"
    KSPCORE_PROJECT_ROOT="${KSP_PROJECT_ROOT}"
    KSPCORE_DATA_DIR="${PROJECT_SOURCE_DIR}/data"
    WHEEL_SPIKE_RESULTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/results"
    WHEEL_SPIKE_TMP_DIR="${CMAKE_CURRENT_BINARY_DIR}/craft")
