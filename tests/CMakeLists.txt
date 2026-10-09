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
    test_celestial.cpp
    test_orbit.cpp
    test_celestial_orbit.cpp
    test_events.cpp
    test_util.cpp
    test_wp03_reference.cpp
    test_partloader.cpp
)
target_link_libraries(kspcore_tests PRIVATE kspcore)
ksp_set_build_flags(kspcore_tests)
target_include_directories(kspcore_tests PRIVATE ${PROJECT_SOURCE_DIR}/third_party)
target_compile_definitions(kspcore_tests PRIVATE KSPCORE_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
                           KSPCORE_HARNESS_DIR="${KSP_HARNESS_DIR}" KSPCORE_PROJECT_ROOT="${KSP_PROJECT_ROOT}"
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
# ksp_golden_gate(<test name> <runner dataset> [GOLDEN <file>] [TOL <file>] [TABLES <t1,t2>]
#                 [COLUMNS <t:c1,c2/t2:c3>] [DROP_HEX])
#   COLUMNS keeps only the listed columns of the named tables (columns that need another package's code; documented).
#   GOLDEN defaults to harness/golden/<dataset>/<dataset>.json and TOL to harness/compare/tolerances.json.
#   In-game capture files (harness/captures/out) carry `<col>_hex` columns: use DROP_HEX.
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
set(KSP_CANDIDATE_DIR "${CMAKE_BINARY_DIR}/candidate")
set(KSP_CAPTURES_DIR "${KSP_HARNESS_DIR}/captures/out")
set(KSP_CAPTURE_TOL "${CMAKE_CURRENT_SOURCE_DIR}/tolerances_capture.json")

function(ksp_golden_gate name dataset)
    cmake_parse_arguments(G "DROP_HEX" "GOLDEN;TOL;TABLES;COLUMNS;DROP_COLUMNS;DROP_ROWS;KEEP_ROWS_FILE;DROP_KEYS_FILE;RENUMBER" "" ${ARGN})
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
            "-DCOLUMNS=${G_COLUMNS}"
            -DDROP_HEX=${drop}
            "-DDROP_COLUMNS=${G_DROP_COLUMNS}"
            "-DDROP_ROWS=${G_DROP_ROWS}"
            "-DKEEP_ROWS_FILE=${G_KEEP_ROWS_FILE}"
            "-DDROP_KEYS_FILE=${G_DROP_KEYS_FILE}"
            "-DRENUMBER=${G_RENUMBER}"
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
# WP-07 Orbit: every table of the orbit_propagation golden (cases, state_at_ut, from_state_vectors, state_vector_cases,
# kepler); harness tolerances unchanged. patched_conics (same generator, separate file) is the WP-08 gate.
ksp_golden_gate(golden.orbit_propagation orbit_propagation)

# WP-09 PhysicsGlobals: every field after level0 prefab + Physics.cfg (harness golden, exact text and values)
ksp_golden_gate(golden.physics_constants physics_constants)
# WP-09 PhysicsGlobals.DragCurveValue (managed float code) vs the in-game Mono capture of the aero_forces table
ksp_golden_gate(capture.physics_drag_curve physics_drag_curve GOLDEN ${KSP_CAPTURES_DIR}/aero_forces_mono.json
                TABLES drag_curve_value DROP_HEX TOL ${KSP_CAPTURE_TOL})
# WP-09 celestial bodies with WP-07's run-time Orbit: every table and column (bodies, orbits incl. the propagated
# meanMotion ... vel_z, atmosphere_profile_1km, body_curves).
ksp_golden_gate(golden.body_catalog body_catalog)
# WP-09 atmosphere: CelestialBody pressure/temperature/density/speed of sound + FlightIntegrator atmosphere helpers
ksp_golden_gate(golden.atmosphere_curves atmosphere_curves)

# GameData corpus: parse + round-trip every cfg/craft in harness/inputs (no golden; self-consistency + no errors)
add_test(NAME corpus.gamedata
    COMMAND golden_runner gamedata --harness ${KSP_HARNESS_DIR} --out ${KSP_CANDIDATE_DIR})
set_tests_properties(corpus.gamedata PROPERTIES LABELS corpus TIMEOUT 900)

# WP-05 part database: GameDatabase + PartLoader (data side) on the real GameData (decompiled/KSP-dev/GameData, read
# by golden_runner's default --gamedata) vs the in-game capture C9 (captures/out2/part_database.json). Bit-exact, with
# these documented scope limits (STATUS.md WP-05):
#   DROP_ROWS  the 5 parts PartLoader builds in code from Unity prefabs (EVA kerbals, flag): their base values are
#              Unity-serialized prefab data, not GameData (the 4 DLC-suit error rows ARE gated); assets rows of kind
#              `texture` (renderer/material texture slots: they need the .mu model contents)
#   KEEP_ROWS_FILE  module_fields: only the fields the part's MODULE node sets (runner-written scope list); the other
#              rows are KSPField defaults / module OnLoad results (WP-06 module instantiation)
#   DROP_KEYS_FILE  individually excluded rows with reasons (tests/data/part_database_exclusions.json)
#   RENUMBER   assets.idx renumbered per part after dropping the texture rows
#   DROP_COLUMNS  parts.partSize (renderer bounds of the model), parts.module_mass/module_cost (IPartMassModifier /
#              IPartCostModifier modules, WP-06), variants.materials (model materials)
set(KSP_CAPTURES2_DIR "${KSP_HARNESS_DIR}/captures/out2")
set(KSP_PREFAB_PARTS "kerbalEVA|kerbalEVAfemale|flag|kerbalEVASlimSuit|kerbalEVASlimSuitFemale")
ksp_golden_gate(capture.part_database part_database GOLDEN ${KSP_CAPTURES2_DIR}/part_database.json DROP_HEX
    TOL ${KSP_CAPTURE_TOL}
    DROP_ROWS "parts:name=${KSP_PREFAB_PARTS}/part_cfg_values:part=${KSP_PREFAB_PARTS}/modules:part=${KSP_PREFAB_PARTS}/module_fields:part=${KSP_PREFAB_PARTS}/module_propellants:part=${KSP_PREFAB_PARTS}/resources:part=${KSP_PREFAB_PARTS}/attach_nodes:part=${KSP_PREFAB_PARTS}/drag_cubes:part=${KSP_PREFAB_PARTS}/variants:part=${KSP_PREFAB_PARTS}/assets:part=${KSP_PREFAB_PARTS}+kind=texture"
    KEEP_ROWS_FILE "<OUT>/part_database/scope.json"
    DROP_KEYS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/data/part_database_exclusions.json
    RENUMBER "assets:idx:part"
    DROP_COLUMNS "parts:partSize,module_mass,module_cost/variants:materials")
set_tests_properties(capture.part_database PROPERTIES FIXTURES_SETUP partdb)
# GameDatabase load order vs the game's own log of the capture run (every "Load(Audio|Texture|Model): url" and
# "Config(TYPE) url" line, in order). Skipped (exit 0 with a note) when the log is not present.
add_test(NAME capture.gamedatabase_load_order
    COMMAND ${Python3_EXECUTABLE} -I ${PROJECT_SOURCE_DIR}/tools/wp05/check_load_log.py
            ${KSP_CANDIDATE_DIR}/part_database/gamedatabase_load.log
            ${KSP_PROJECT_ROOT}/decompiled/logs/ksp-capture-capture2-final2-KSP.log)
set_tests_properties(capture.gamedatabase_load_order PROPERTIES LABELS golden FIXTURES_REQUIRED partdb TIMEOUT 300)
