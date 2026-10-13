# Jolt physics spike (core/PHYSICS-SPIKE.md). Opt-in: configure with -DKSPCORE_BUILD_SPIKES=ON, or use the presets
# `spike-jolt` (Jolt DOUBLE_PRECISION) / `spike-jolt-f32` (single precision), each in its own build tree. It is not part of
# run_tests.ps1 / ctest / CI.
# Jolt itself is the vendored copy built by the root CMakeLists (cmake/ksp_jolt.cmake, third_party/jolt): since WP-20a the spike
# no longer fetches it. KSP_JOLT_DOUBLE / KSP_JOLT_DETERMINISTIC are the root options.

add_executable(jolt_spike
    src/main.cpp
    src/bench.cpp
    src/jolt_world.cpp
    src/ksp_context.cpp
    src/craft_model.cpp
    src/articulated.cpp
    src/reduced.cpp
    src/structure_graph.cpp
    src/structure_graph_direct.cpp
    src/terrain_patch.cpp
    src/part_pipeline.cpp
    src/flight.cpp
)
target_link_libraries(jolt_spike PRIVATE kspcore Jolt)
ksp_set_build_flags(jolt_spike)
target_include_directories(jolt_spike PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(MSVC)
    # Jolt headers: unreferenced parameters, padding, conversions under /W4 (not our code)
    target_compile_options(jolt_spike PRIVATE /wd4100 /wd4324 /wd4127)
endif()
target_compile_definitions(jolt_spike PRIVATE
    KSPCORE_DEFAULT_GAME_ROOT="${KSP_GAMEDATA_ROOT}"
    KSPCORE_HARNESS_DIR="${KSP_HARNESS_DIR}"
    KSPCORE_PROJECT_ROOT="${KSP_PROJECT_ROOT}"
    KSPCORE_DATA_DIR="${PROJECT_SOURCE_DIR}/data"
    SPIKE_CRAFT_DIR="${KSP_PROJECT_ROOT}/analysis/physics/craft"
    SPIKE_GEN_CRAFT_DIR="${CMAKE_CURRENT_BINARY_DIR}/craft")

# Larger crafts for the scaling curve, generated at configure time by the profile's own generator
# (analysis/physics/scripts/gen_craft.py, same booster design) into the build tree; never committed.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    execute_process(COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_xl_crafts.py
                            ${KSP_PROJECT_ROOT}/analysis/physics/scripts/gen_craft.py ${CMAKE_CURRENT_BINARY_DIR}/craft
                    RESULT_VARIABLE _spike_gen_rc OUTPUT_VARIABLE _spike_gen_out ERROR_VARIABLE _spike_gen_err)
    message(STATUS "jolt_spike: gen_xl_crafts rc=${_spike_gen_rc} ${_spike_gen_out}${_spike_gen_err}")
endif()

# WP-20b: the spike's ascent / pad scenarios on the PRODUCTION vessel binding (ksp/physics/vessel_physics.hpp), built here so that its
# timings come from the same Release configuration as the spike's tables (STATUS.md WP-20b): wp20b_bench ascent <craft>... [options]
add_executable(wp20b_bench ${PROJECT_SOURCE_DIR}/tools/wp20b/bench.cpp)
target_link_libraries(wp20b_bench PRIVATE kspcore)
ksp_set_build_flags(wp20b_bench)
target_include_directories(wp20b_bench PRIVATE ${PROJECT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/tools/golden_runner)
target_compile_definitions(wp20b_bench PRIVATE KSPCORE_HARNESS_DIR="${KSP_HARNESS_DIR}" KSPCORE_DATA_DIR="${PROJECT_SOURCE_DIR}/data"
                           KSPCORE_DEFAULT_GAME_ROOT="${KSP_GAMEDATA_ROOT}")
