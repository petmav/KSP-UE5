# Golden gate for one dataset (invoked by ctest with cmake -P; tests/CMakeLists.txt ksp_golden_gate() builds the call):
#   1. golden_runner <DATASET> writes ${OUT}/<DATASET>/<DATASET>.json (ksp-golden/1 candidate) and runs any
#      dataset-specific exact checks (e.g. byte-identical ConfigNode.Save output); non-zero exit = failure.
#   2. When any filter option is set, tools/filter_golden.py writes a filtered copy of the golden file into the build
#      tree (${OUT}/<DATASET>/golden_<NAME>.json; the harness itself is never modified).
#   3. python -I harness/compare/compare.py golden candidate --tol TOL; any out-of-tolerance column, missing
#      row/column or table fails the test.
#
# Parameters: RUNNER DATASET HARNESS OUT PYTHON (required); NAME (report name, default DATASET);
#             GOLDEN (default ${HARNESS}/golden/<DATASET>/<DATASET>.json); TOL (default harness tolerances.json);
#             FILTER (path of tools/filter_golden.py).
# Filter options (empty = unset), each passed to the filter_golden.py flag in brackets; README.md "Golden gate options"
# documents the grammar and which gates use which option:
#             TABLES [--tables], KEYS [--key], KEEP_ROWS [--keep-rows], DROP_ROWS [--drop-rows],
#             KEEP_ROWS_FILE [--keep-rows-file], DROP_KEYS_FILE [--drop-keys-file], RENUMBER [--renumber],
#             DROP_HEX (ON/OFF) [--drop-hex], COLUMNS [--keep-columns], DROP_COLUMNS [--drop-columns].
#             In KEEP_ROWS_FILE and DROP_KEYS_FILE the text <OUT> is replaced by OUT (files the runner writes).
foreach(v RUNNER DATASET HARNESS OUT PYTHON)
    if(NOT DEFINED ${v})
        message(FATAL_ERROR "RunGolden.cmake: ${v} not set")
    endif()
endforeach()
if(NOT DEFINED NAME OR NAME STREQUAL "")
    set(NAME "${DATASET}")
endif()
if(NOT DEFINED GOLDEN OR GOLDEN STREQUAL "")
    set(GOLDEN "${HARNESS}/golden/${DATASET}/${DATASET}.json")
endif()
if(NOT DEFINED TOL OR TOL STREQUAL "")
    set(TOL "${HARNESS}/compare/tolerances.json")
endif()

execute_process(
    COMMAND "${RUNNER}" "${DATASET}" --harness "${HARNESS}" --out "${OUT}"
    RESULT_VARIABLE runner_rc)
if(NOT runner_rc EQUAL 0)
    message(FATAL_ERROR "golden_runner ${DATASET} failed (exit ${runner_rc})")
endif()

set(golden_file "${GOLDEN}")
# option name -> filter_golden.py flag (step order is fixed inside filter_golden.py, not by this list)
set(filter_options TABLES --tables KEYS --key KEEP_ROWS --keep-rows DROP_ROWS --drop-rows
    KEEP_ROWS_FILE --keep-rows-file DROP_KEYS_FILE --drop-keys-file RENUMBER --renumber
    COLUMNS --keep-columns DROP_COLUMNS --drop-columns)
set(filter_args "")
list(LENGTH filter_options n_filter_options)
math(EXPR last "${n_filter_options} - 1")
foreach(i RANGE 0 ${last} 2)
    math(EXPR j "${i} + 1")
    list(GET filter_options ${i} opt)
    list(GET filter_options ${j} flag)
    if(DEFINED ${opt} AND NOT "${${opt}}" STREQUAL "")
        string(REPLACE "<OUT>" "${OUT}" value "${${opt}}")
        list(APPEND filter_args ${flag} "${value}")
    endif()
endforeach()
if(DROP_HEX)
    list(APPEND filter_args --drop-hex)
endif()
if(filter_args)
    set(golden_file "${OUT}/${DATASET}/golden_${NAME}.json")
    execute_process(COMMAND "${PYTHON}" -I "${FILTER}" "${GOLDEN}" "${golden_file}" ${filter_args}
                    RESULT_VARIABLE filter_rc)
    if(NOT filter_rc EQUAL 0)
        message(FATAL_ERROR "filter_golden.py failed for ${NAME} (exit ${filter_rc})")
    endif()
endif()

set(candidate_file "${OUT}/${DATASET}/${DATASET}.json")
set(report_file "${OUT}/${DATASET}/compare_report_${NAME}.json")
execute_process(
    COMMAND "${PYTHON}" -I "${HARNESS}/compare/compare.py" "${golden_file}" "${candidate_file}"
            --tol "${TOL}" --report "${report_file}"
    RESULT_VARIABLE compare_rc)
if(NOT compare_rc EQUAL 0)
    message(FATAL_ERROR "compare.py: ${NAME} out of tolerance (exit ${compare_rc}); report: ${report_file}")
endif()
message(STATUS "golden ${NAME}: PASS")
