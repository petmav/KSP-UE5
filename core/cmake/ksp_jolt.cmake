# Vendored Jolt Physics (third_party/jolt, v5.6.0; origin and sha256 in third_party/jolt/README.md). No network access:
# the production build never downloads anything. Included once by the root CMakeLists.txt before add_subdirectory(src).
#
# Configuration = the one the physics spike measured (core/spikes/jolt, PHYSICS-SPIKE.md):
#   * DOUBLE_PRECISION (JPH_DOUBLE_PRECISION): RVec3 / body positions are double, velocities / rotations stay float.
#   * CROSS_PLATFORM_DETERMINISTIC OFF by default (the spike's setting: Jolt's own flags /fp:fast on MSVC, FMA allowed on
#     x86; deterministic per binary and call order, independent of the thread count). ON turns on /fp:precise, no FMA and the
#     deterministic code paths (bit-identical across compilers / OSes, a few percent slower): -DKSP_JOLT_DETERMINISTIC=ON.
#   * /MD runtime (kspcore's), no IPO, no asserts, no debug renderer / profiler / object stream, no GPU back ends,
#     no -Werror / /WX on third-party code, FP exceptions off.
#   * ISA: Jolt's defaults (AVX2 + BMI/LZCNT/F16C/FMA on x86-64). Jolt's PUBLIC compile options (/arch:AVX2, -mavx2 ...) are
#     applied only to the physics glue objects (src/physics/physics_world.cpp), never to the rest of kspcore, whose float
#     code must stay SSE2-scalar and uncontracted for bit-exact results (see ksp_set_build_flags).
option(KSP_JOLT_DOUBLE "Build Jolt with DOUBLE_PRECISION (JPH_DOUBLE_PRECISION: RVec3 = double positions)" ON)
option(KSP_JOLT_DETERMINISTIC "Build Jolt with CROSS_PLATFORM_DETERMINISTIC" OFF)

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

# Jolt's Build/CMakeLists.txt defines the `Jolt` static library when it is not the top-level project.
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/jolt/Build ${CMAKE_BINARY_DIR}/third_party/jolt EXCLUDE_FROM_ALL)
