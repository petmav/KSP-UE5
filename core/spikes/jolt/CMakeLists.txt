# Jolt physics spike (core/PHYSICS-SPIKE.md). Opt-in: configure with -DKSPCORE_BUILD_SPIKES=ON, or use the presets
# `spike-jolt` (Jolt DOUBLE_PRECISION) / `spike-jolt-f32` (single precision), each in its own build tree. It is not part of
# run_tests.ps1 / ctest / CI and changes nothing in kspcore.
include(FetchContent)

option(KSP_JOLT_DOUBLE "Build Jolt with DOUBLE_PRECISION (JPH_DOUBLE_PRECISION: RVec3 = double positions)" ON)
option(KSP_JOLT_DETERMINISTIC "Build Jolt with CROSS_PLATFORM_DETERMINISTIC" OFF)

# Jolt's own options (Build/CMakeLists.txt of the release). kspcore is /MD, never IPO, never /WX on third-party code.
set(DOUBLE_PRECISION ${KSP_JOLT_DOUBLE} CACHE BOOL "" FORCE)
set(CROSS_PLATFORM_DETERMINISTIC ${KSP_JOLT_DETERMINISTIC} CACHE BOOL "" FORCE)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
set(GENERATE_DEBUG_SYMBOLS OFF CACHE BOOL "" FORCE)
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF CACHE BOOL "" FORCE)
set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_OBJECT_STREAM OFF CACHE BOOL "" FORCE)
set(DEBUG_RENDERER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
set(PROFILER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
set(JPH_USE_DX12 OFF CACHE BOOL "" FORCE)
set(JPH_USE_VK OFF CACHE BOOL "" FORCE)
set(JPH_USE_MTL OFF CACHE BOOL "" FORCE)
set(JPH_USE_CPU_COMPUTE OFF CACHE BOOL "" FORCE)

# Jolt Physics v5.6.0 (2026-07-11), commit e77f175595e64cb44218cc9d9d56fc365ad0e36a: the GitHub release archive, pinned by hash.
FetchContent_Declare(JoltPhysics
    URL https://github.com/jrouwe/JoltPhysics/archive/refs/tags/v5.6.0.tar.gz
    URL_HASH SHA256=6e069ee0172478cc78182047aac87e5310ba14a67a53348ae14cc37801fd3f8e
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR Build)
FetchContent_MakeAvailable(JoltPhysics)

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
