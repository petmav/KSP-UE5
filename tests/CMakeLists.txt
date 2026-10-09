# ---- unit tests (doctest) -------------------------------------------------------------------------------------------
add_executable(kspcore_tests
    test_main.cpp
    test_dotnet_number.cpp
    test_dotnet_text.cpp
    test_math.cpp
    test_curves.cpp
    test_config_node.cpp
    test_wp01_misc.cpp
    test_netfx_reference.cpp
    test_events.cpp
    test_util.cpp
    test_wp03_reference.cpp
)
target_link_libraries(kspcore_tests PRIVATE kspcore)
ksp_set_build_flags(kspcore_tests)
target_include_directories(kspcore_tests PRIVATE ${PROJECT_SOURCE_DIR}/third_party)
target_compile_definitions(kspcore_tests PRIVATE KSPCORE_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    KSP_DICTIONARY_CFG="${KSP_PROJECT_ROOT}/decompiled/KSP-dev/GameData/Squad/Localization/dictionary.cfg")
add_test(NAME unit COMMAND kspcore_tests)

# ---- C ABI consumers: a pure C program, and a C++ program built with Unreal's flags (/GR-, _HAS_EXCEPTIONS=0,
# /fp:fast) against include/kspcore/kspcore_c.h only. They prove the header is C, the lib links under UE's settings and
# the consumer's flags do not change kspcore's results.
add_executable(capi_c capi/capi_c_test.c)
target_include_directories(capi_c PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(capi_c PRIVATE kspcore)
add_executable(capi_ue_like capi/capi_ue_like.cpp)
target_include_directories(capi_ue_like PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(capi_ue_like PRIVATE kspcore)
if(MSVC)
    target_compile_options(capi_ue_like PRIVATE /GR- /EHs-c- /fp:fast /W4)
    target_compile_definitions(capi_ue_like PRIVATE _HAS_EXCEPTIONS=0)
endif()
add_test(NAME capi.c COMMAND capi_c ${KSP_HARNESS_DIR}/inputs/Physics.cfg)
add_test(NAME capi.ue_like COMMAND capi_ue_like ${KSP_HARNESS_DIR}/inputs/Physics.cfg)

# ---- golden gates: golden_runner writes candidates, harness/compare/compare.py judges them -----------------------------
# ksp_golden_gate(<test name> <runner dataset> [GOLDEN <file>] [TOL <file>] [TABLES <t1,t2>] [DROP_HEX])
#   GOLDEN defaults to harness/golden/<dataset>/<dataset>.json and TOL to harness/compare/tolerances.json.
#   In-game capture files (harness/captures/out) carry `<col>_hex` columns: use DROP_HEX.
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
set(KSP_CANDIDATE_DIR "${CMAKE_BINARY_DIR}/candidate")
set(KSP_CAPTURES_DIR "${KSP_HARNESS_DIR}/captures/out")
set(KSP_CAPTURE_TOL "${CMAKE_CURRENT_SOURCE_DIR}/tolerances_capture.json")

function(ksp_golden_gate name dataset)
    cmake_parse_arguments(G "DROP_HEX" "GOLDEN;TOL;TABLES" "" ${ARGN})
    if(G_DROP_HEX)
        set(drop ON)
    else()
        set(drop OFF)
    endif()
    add_test(NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DRUNNER=$<TARGET_FILE:golden_runner>
            -DDATASET=${dataset}
            -DNAME=${name}
            -DHARNESS=${KSP_HARNESS_DIR}
            -DOUT=${KSP_CANDIDATE_DIR}
            -DPYTHON=${Python3_EXECUTABLE}
            -DGOLDEN=${G_GOLDEN}
            -DTOL=${G_TOL}
            -DTABLES=${G_TABLES}
            -DDROP_HEX=${drop}
            -DFILTER=${PROJECT_SOURCE_DIR}/tools/filter_golden.py
            -P ${PROJECT_SOURCE_DIR}/cmake/RunGolden.cmake)
    set_tests_properties(${name} PROPERTIES LABELS golden TIMEOUT 900 RUN_SERIAL TRUE)
endfunction()

# WP-01 maths: double maths vs the harness golden (runtime-independent, CAPTURE-SPEC C7); Unity float32 maths vs the
# in-game capture (Mono float semantics; the harness's .NET float32 rows are not in-game truth).
ksp_golden_gate(golden.math_shim.f64 math_shim TABLES f64)
ksp_golden_gate(capture.math_shim.f32 math_shim GOLDEN ${KSP_CAPTURES_DIR}/math_shim_mono.json TABLES f32 DROP_HEX
                TOL ${KSP_CAPTURE_TOL})
# WP-01 runtime layer: System.Math, float32 evaluation, conversions, formatting, parsing, System.Random under Mono
ksp_golden_gate(capture.libm_mono libm_mono GOLDEN ${KSP_CAPTURES_DIR}/libm_mono.json DROP_HEX TOL ${KSP_CAPTURE_TOL})
# WP-01 UnityEngine.Random + KSPRandom vs the in-game capture (C2)
ksp_golden_gate(capture.unity_random unity_random GOLDEN ${KSP_CAPTURES_DIR}/unity_random.json DROP_HEX TOL ${KSP_CAPTURE_TOL}
                TABLES seeds,states,value,range_f,range_i,vec,edge,trace,trace_int,trace_vec,ksp_random)
# WP-02 ConfigNode
ksp_golden_gate(golden.confignode_roundtrip confignode_roundtrip)
# WP-01 FloatCurve / AnimationCurve (harness golden == native Unity, CAPTURE-SPEC C1: 0 ULP)
ksp_golden_gate(golden.curves_eval curves_eval)

# GameData corpus: parse + round-trip every cfg/craft in harness/inputs (no golden; self-consistency + no errors)
add_test(NAME corpus.gamedata
    COMMAND golden_runner gamedata --harness ${KSP_HARNESS_DIR} --out ${KSP_CANDIDATE_DIR})
set_tests_properties(corpus.gamedata PROPERTIES LABELS corpus TIMEOUT 900)
