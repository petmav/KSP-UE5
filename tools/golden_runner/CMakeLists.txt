# golden_runner: regenerates harness datasets with kspcore (candidate files for harness/compare/compare.py).
add_executable(golden_runner
    main.cpp
    golden_json.cpp
    sha256.cpp
    ds_math_shim.cpp
)
target_link_libraries(golden_runner PRIVATE kspcore)
target_include_directories(golden_runner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/third_party)
