# golden_runner: regenerates harness datasets with kspcore (candidate files for harness/compare/compare.py).
add_executable(golden_runner
    main.cpp
    golden_json.cpp
    sha256.cpp
    ds_math_shim.cpp
    ds_curves_eval.cpp
    confignode_tree.cpp
    ds_confignode.cpp
    ds_gamedata.cpp
    ds_unity_random.cpp
    ds_libm_mono.cpp
    celestial_env.cpp
    ds_physics_constants.cpp
    ds_body_catalog.cpp
    ds_atmosphere_curves.cpp
    ds_orbit_propagation.cpp
    ds_terrain_height.cpp
)
target_link_libraries(golden_runner PRIVATE kspcore)
ksp_set_build_flags(golden_runner)
target_include_directories(golden_runner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/third_party)
# WP-11 terrain: the extracted original height maps (core/data/terrain, tools/wp11/extract_terrain.py)
target_compile_definitions(golden_runner PRIVATE KSPCORE_DATA_DIR="${PROJECT_SOURCE_DIR}/data")
