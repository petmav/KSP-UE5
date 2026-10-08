# ---- unit tests (doctest) -------------------------------------------------------------------------------------------
add_executable(kspcore_tests
    test_main.cpp
    test_dotnet_number.cpp
    test_dotnet_text.cpp
    test_math.cpp
    test_curves.cpp
    test_config_node.cpp
    test_netfx_reference.cpp
)
target_link_libraries(kspcore_tests PRIVATE kspcore)
target_compile_definitions(kspcore_tests PRIVATE KSPCORE_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
target_include_directories(kspcore_tests PRIVATE ${PROJECT_SOURCE_DIR}/third_party)
add_test(NAME unit COMMAND kspcore_tests)

# ---- golden gate: golden_runner writes candidates, harness/compare/compare.py judges them --------------------------
# One ctest per harness dataset. A work package adds its dataset name here (and a ds_<name>.cpp to the runner).
set(KSP_GOLDEN_DATASETS
    math_shim               # WP-01 maths / Unity value types
    confignode_roundtrip    # WP-02 ConfigNode
    curves_eval             # WP-01 FloatCurve / AnimationCurve
)
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
set(KSP_CANDIDATE_DIR "${CMAKE_BINARY_DIR}/candidate")
foreach(ds IN LISTS KSP_GOLDEN_DATASETS)
    add_test(NAME golden.${ds}
        COMMAND ${CMAKE_COMMAND}
            -DRUNNER=$<TARGET_FILE:golden_runner>
            -DDATASET=${ds}
            -DHARNESS=${KSP_HARNESS_DIR}
            -DOUT=${KSP_CANDIDATE_DIR}
            -DPYTHON=${Python3_EXECUTABLE}
            -P ${PROJECT_SOURCE_DIR}/cmake/RunGolden.cmake)
    set_tests_properties(golden.${ds} PROPERTIES LABELS golden TIMEOUT 600)
endforeach()

# GameData corpus: parse + round-trip every cfg/craft in harness/inputs (no golden; self-consistency + no errors)
add_test(NAME corpus.gamedata
    COMMAND golden_runner gamedata --harness ${KSP_HARNESS_DIR} --out ${KSP_CANDIDATE_DIR})
set_tests_properties(corpus.gamedata PROPERTIES LABELS corpus TIMEOUT 600)
