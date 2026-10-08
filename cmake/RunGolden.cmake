# Golden gate for one dataset (invoked by ctest with cmake -P):
#   1. golden_runner <DATASET> writes ${OUT}/<DATASET>/<DATASET>.json (ksp-golden/1 candidate) and runs any
#      dataset-specific exact checks (e.g. byte-identical ConfigNode.Save output); non-zero exit = failure.
#   2. python -I harness/compare/compare.py golden candidate --tol tolerances.json; any out-of-tolerance column,
#      missing row/column or table fails the test.
foreach(v RUNNER DATASET HARNESS OUT PYTHON)
    if(NOT DEFINED ${v})
        message(FATAL_ERROR "RunGolden.cmake: ${v} not set")
    endif()
endforeach()

execute_process(
    COMMAND "${RUNNER}" "${DATASET}" --harness "${HARNESS}" --out "${OUT}"
    RESULT_VARIABLE runner_rc)
if(NOT runner_rc EQUAL 0)
    message(FATAL_ERROR "golden_runner ${DATASET} failed (exit ${runner_rc})")
endif()

set(golden_file "${HARNESS}/golden/${DATASET}/${DATASET}.json")
set(candidate_file "${OUT}/${DATASET}/${DATASET}.json")
set(report_file "${OUT}/${DATASET}/compare_report.json")
execute_process(
    COMMAND "${PYTHON}" -I "${HARNESS}/compare/compare.py" "${golden_file}" "${candidate_file}"
            --tol "${HARNESS}/compare/tolerances.json" --report "${report_file}"
    RESULT_VARIABLE compare_rc)
if(NOT compare_rc EQUAL 0)
    message(FATAL_ERROR "compare.py: ${DATASET} out of tolerance (exit ${compare_rc}); report: ${report_file}")
endif()
message(STATUS "golden ${DATASET}: PASS")
