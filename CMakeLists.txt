cmake_minimum_required(VERSION 3.28)

project(kspcore
    VERSION 0.1.0
    DESCRIPTION "Engine-agnostic C++ reimplementation of the KSP 1.12.5 simulation core"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Location of the sibling directories (harness/, decompiled/) of the KSP-UE5 project.
set(KSP_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "KSP-UE5 project root")
cmake_path(NORMAL_PATH KSP_PROJECT_ROOT)
set(KSP_HARNESS_DIR "${KSP_PROJECT_ROOT}/harness" CACHE PATH "Golden-output harness directory (read-only)")

option(KSPCORE_BUILD_TESTS "Build unit tests and the golden gate" ON)

# ---------------------------------------------------------------------------------------------------------------------
# Compiler settings. Floating point: IEEE semantics, no contraction (no FMA), SSE2 scalar float32 maths on x64; this is
# what the golden data was produced with (RyuJIT x64) and is required for bit-exact float32 results.
# ---------------------------------------------------------------------------------------------------------------------
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /EHsc /fp:precise /Zc:__cplusplus /Zc:preprocessor /bigobj /MP)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    add_compile_options(-Wall -Wextra -ffp-contract=off -fno-fast-math)
endif()

add_subdirectory(src)

if(KSPCORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tools/golden_runner)
    add_subdirectory(tests)
endif()
