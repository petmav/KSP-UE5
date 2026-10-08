cmake_minimum_required(VERSION 3.28)

project(kspcore
    VERSION 0.1.0
    DESCRIPTION "Engine-agnostic C++ reimplementation of the KSP 1.12.5 simulation core"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------------------------------------------------
# Unreal Engine 5.8 link requirements (ue5/README.md "What the core must provide"), applied to EVERY configuration:
#   * release CRT DLL (/MD) even in Debug -> _ITERATOR_DEBUG_LEVEL=0 (UE never uses the debug CRT; LNK2038 otherwise)
#   * debug info embedded in the objects (/Z7) so kspcore.lib links without a separate PDB (no LNK4099)
#   * no /GL / LTCG (whole-program objects are toolset-version locked); CMake never adds it unless asked
#   * C++20
# ---------------------------------------------------------------------------------------------------------------------
# CMake puts /EHsc into CMAKE_CXX_FLAGS by default; kspcore targets get it from ksp_set_build_flags() instead, so
# the Unreal-like consumer test can build with exceptions disabled without D9025 overrides.
string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

# Location of the sibling directories (harness/, decompiled/) of the KSP-UE5 project. core/ may also be checked out as a
# git worktree (core/.worktrees/<name>/), so walk up the parents until harness/compare/compare.py is found.
set(_ksp_root_guess "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(_ksp_dir "${CMAKE_CURRENT_SOURCE_DIR}")
foreach(_ksp_i RANGE 4)
    cmake_path(GET _ksp_dir PARENT_PATH _ksp_dir)
    if(EXISTS "${_ksp_dir}/harness/compare/compare.py")
        set(_ksp_root_guess "${_ksp_dir}")
        break()
    endif()
endforeach()
set(KSP_PROJECT_ROOT "${_ksp_root_guess}" CACHE PATH "KSP-UE5 project root")
cmake_path(NORMAL_PATH KSP_PROJECT_ROOT)
set(KSP_HARNESS_DIR "${KSP_PROJECT_ROOT}/harness" CACHE PATH "Golden-output harness directory (read-only)")

option(KSPCORE_BUILD_TESTS "Build unit tests and the golden gate" ON)

# ---------------------------------------------------------------------------------------------------------------------
# kspcore's own compiler flags, applied PRIVATELY to kspcore's targets (never propagated to consumers, and consumers'
# flags never reach kspcore's objects). Floating point: IEEE semantics, no contraction (no FMA), SSE2 scalar float32 on
# x64 -- required for bit-exact float32 results and for the explicit Mono float rule (ksp/dotnet/mono_float.hpp).
# Unreal's Game target compiles its own code with /fp:fast; that is fine because UE only includes the C header
# include/kspcore/kspcore_c.h, which has no inline numerics.
# ---------------------------------------------------------------------------------------------------------------------
function(ksp_set_build_flags target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /EHsc /fp:precise /Zc:__cplusplus
                                                 /Zc:preprocessor /bigobj /MP)
        target_compile_definitions(${target} PRIVATE _ITERATOR_DEBUG_LEVEL=0 _CRT_SECURE_NO_WARNINGS NOMINMAX
                                                     WIN32_LEAN_AND_MEAN)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -ffp-contract=off -fno-fast-math)
    endif()
endfunction()

add_subdirectory(src)

if(KSPCORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tools/golden_runner)
    add_subdirectory(tests)
endif()
