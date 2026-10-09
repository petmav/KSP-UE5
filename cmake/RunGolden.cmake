# Golden gate for one dataset (invoked by ctest with cmake -P):
#   1. golden_runner <DATASET> writes ${OUT}/<DATASET>/<DATASET>.json (ksp-golden/1 candidate) and runs any
#      dataset-specific exact checks (e.g. byte-identical ConfigNode.Save output); non-zero exit = failure.
#   2. Optional: tools/filter_golden.py writes a copy of the golden file restricted to TABLES and/or without the
#      `_hex` columns of in-game capture files (DROP_HEX) into the build tree (the harness itself is never modified).
#   3. python -I harness/compare/compare.py golden candidate --tol TOL; any out-of-tolerance column, missing
#      row/column or table fails the test.
#
# Parameters: RUNNER DATASET HARNESS OUT PYTHON (required); NAME (report name, default DATASET);
#             GOLDEN (default ${HARNESS}/golden/<DATASET>/<DATASET>.json); TOL (default harness tolerances.json);
#             TABLES (comma-separated); COLUMNS (t:c1,c2/t2:c3: per-table kept columns); DROP_HEX (ON/OFF);
#             FILTER (path of tools/filter_golden.py); row/column filters passed through to filter_golden.py:
#             DROP_COLUMNS (t:c1,c2/...), DROP_ROWS (t:col=v1|v2+col2=v3/...), KEEP_ROWS_FILE (JSON scope list the
#             runner writes; <OUT> is replaced by OUT), DROP_KEYS_FILE (JSON of individually excluded rows with reasons),
#             RENUMBER (t:col:groupcol/...).
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
set(row_filter OFF)
foreach(v DROP_COLUMNS DROP_ROWS KEEP_ROWS_FILE DROP_KEYS_FILE RENUMBER)
    if(DEFINED ${v} AND NOT ${v} STREQUAL "")
        set(row_filter ON)
    endif()
endforeach()
if((DEFINED TABLES AND NOT TABLES STREQUAL "") OR (DEFINED COLUMNS AND NOT COLUMNS STREQUAL "") OR DROP_HEX OR row_filter)
    set(golden_file "${OUT}/${DATASET}/golden_${NAME}.json")
    set(filter_args "${GOLDEN}" "${golden_file}")
    if(DEFINED TABLES AND NOT TABLES STREQUAL "")
        list(APPEND filter_args --tables "${TABLES}")
    endif()
    if(DEFINED COLUMNS AND NOT COLUMNS STREQUAL "")
        list(APPEND filter_args --keep-columns "${COLUMNS}")
    endif()
    if(DROP_HEX)
        list(APPEND filter_args --drop-hex)
    endif()
    if(DEFINED DROP_COLUMNS AND NOT DROP_COLUMNS STREQUAL "")
        list(APPEND filter_args --drop-columns "${DROP_COLUMNS}")
    endif()
    if(DEFINED DROP_ROWS AND NOT DROP_ROWS STREQUAL "")
        list(APPEND filter_args --drop-rows "${DROP_ROWS}")
    endif()
    if(DEFINED KEEP_ROWS_FILE AND NOT KEEP_ROWS_FILE STREQUAL "")
        string(REPLACE "<OUT>" "${OUT}" keep_rows_file "${KEEP_ROWS_FILE}")
        list(APPEND filter_args --keep-rows-file "${keep_rows_file}")
    endif()
    if(DEFINED DROP_KEYS_FILE AND NOT DROP_KEYS_FILE STREQUAL "")
        list(APPEND filter_args --drop-keys-file "${DROP_KEYS_FILE}")
    endif()
    if(DEFINED RENUMBER AND NOT RENUMBER STREQUAL "")
        list(APPEND filter_args --renumber "${RENUMBER}")
    endif()
    execute_process(COMMAND "${PYTHON}" -I "${FILTER}" ${filter_args} RESULT_VARIABLE filter_rc)
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
