set(KSP_LOAD_LOG "${PROJECT_SOURCE_DIR}/testdata/logs/ksp-capture-capture2-final2-KSP.log")

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
    test_patched_conics.cpp
    test_timewarp.cpp
    test_terrain.cpp
    test_pqs_quadtree.cpp
    test_terrain_colliders.cpp
    test_partloader.cpp
    test_modules.cpp
    test_part.cpp
    test_resource_flow.cpp
    test_resource_modules.cpp
    test_resource_processing.cpp
    test_deployables.cpp
    test_kerbals.cpp
    test_vessel.cpp
    test_ship_construct.cpp
    test_aero.cpp
    test_engines.cpp
    test_staging.cpp
    test_docking.cpp
    test_thermal.cpp
    test_structural_modules.cpp
    test_misc_modules.cpp
    test_deltav.cpp
    test_timing.cpp
    test_physics_world.cpp
    test_vessel_physics.cpp
    test_mu_colliders.cpp
    test_structure.cpp
    test_structure_events.cpp
    test_physics_bubble.cpp
    test_physics_bubble_ref.cpp
    test_flight_integrator.cpp
    test_flight.cpp
    test_flight2.cpp
    test_flight3.cpp
    test_command.cpp
    test_science.cpp
    test_commnet.cpp
    test_editor.cpp
    test_progress.cpp
    test_contracts.cpp
    test_serenity.cpp
    test_robotics.cpp
    test_robotics_controller.cpp
    test_robotics_joint.cpp
    test_robotics_derived.cpp
    test_robotics_behaviour.cpp
    test_robotics_controller_derived.cpp
    test_fineprint.cpp
    test_fineprint2.cpp
    test_fineprint3.cpp
    test_space_center.cpp
    test_missions.cpp
    test_missions_runtime.cpp
    test_missions2.cpp
    test_missions3.cpp
    test_node_library.cpp
    test_inventory.cpp
    test_inventory_rules.cpp
    test_construction.cpp
    test_inventory_ui.cpp
    test_inventory_deploy.cpp
    test_inventory_misc.cpp
    test_construction_flow.cpp
    test_eva.cpp
    test_eva_motion.cpp
    test_eva_conditions.cpp
    test_eva_misc.cpp
    test_eva_behaviour.cpp
    test_eva_behaviour2.cpp
    test_input.cpp
    test_input_flight.cpp
    test_input_devices.cpp
    test_input_ui.cpp
    test_wheels.cpp
    test_wheel_modules.cpp
    test_wheel_modules_derived.cpp
    ${PROJECT_SOURCE_DIR}/tools/golden_runner/sha256.cpp
    test_save.cpp
    test_hygiene.cpp
)
target_link_libraries(kspcore_tests PRIVATE kspcore)
ksp_set_build_flags(kspcore_tests)
target_include_directories(kspcore_tests PRIVATE ${PROJECT_SOURCE_DIR}/third_party ${PROJECT_SOURCE_DIR}/tools/golden_runner
                           ${PROJECT_SOURCE_DIR}/tools/support)
target_compile_definitions(kspcore_tests PRIVATE KSPCORE_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
                           KSPCORE_HARNESS_DIR="${KSP_HARNESS_DIR}" KSPCORE_PROJECT_ROOT="${KSP_PROJECT_ROOT}"
                           KSPCORE_DATA_DIR="${PROJECT_SOURCE_DIR}/data"
                           KSPCORE_TESTDATA_DIR="${PROJECT_SOURCE_DIR}/testdata"
                           KSPCORE_DEFAULT_GAME_ROOT="${KSP_GAMEDATA_ROOT}")
# The unit suite (~860 doctest cases, one process, ~3 min) is split into fixed-size shards by case index (this doctest
# has no --shard-count; -f/-l take the 1-based range of the filtered, in-file-order list) so `ctest -j` overlaps them.
# The last shard is open-ended, so cases added later are never skipped. Cases must not share state across shards
# (tests/test_hygiene.cpp, unique temp dirs). Whole suite in one process:  kspcore_tests
set(KSP_UNIT_SHARDS 8)
set(KSP_UNIT_SHARD_SIZE 120)
math(EXPR _last_shard "${KSP_UNIT_SHARDS} - 1")
foreach(shard RANGE 0 ${_last_shard})
    math(EXPR _first "${shard} * ${KSP_UNIT_SHARD_SIZE} + 1")
    math(EXPR _last "(${shard} + 1) * ${KSP_UNIT_SHARD_SIZE}")
    if(shard EQUAL _last_shard)
        add_test(NAME unit.shard.${shard} COMMAND kspcore_tests -f=${_first})
    else()
        add_test(NAME unit.shard.${shard} COMMAND kspcore_tests -f=${_first} -l=${_last})
    endif()
    set_tests_properties(unit.shard.${shard} PROPERTIES LABELS unit TIMEOUT 1200)
endforeach()
if(KSPCORE_PCH)
    # doctest + nlohmann + the std/ksp headers most test files include (tests/pch.hpp). test_main.cpp defines
    # DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN before including doctest, so it must not get the PCH.
    target_precompile_headers(kspcore_tests PRIVATE pch.hpp)
    set_source_files_properties(test_main.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
endif()
if(KSPCORE_BUILD_DEV_TOOLS)
# WP-12a: the height oracle tools/wp12/pqs_reference.py calls to regenerate tests/data/wp12_reference.json (not a test)
add_executable(wp12_height_oracle ${PROJECT_SOURCE_DIR}/tools/wp12/height_oracle.cpp)
target_link_libraries(wp12_height_oracle PRIVATE kspcore)
ksp_set_build_flags(wp12_height_oracle)
target_include_directories(wp12_height_oracle PRIVATE ${PROJECT_SOURCE_DIR}/third_party)
target_compile_definitions(wp12_height_oracle PRIVATE KSPCORE_DATA_DIR="${PROJECT_SOURCE_DIR}/data")
# WP-12a: quad build / tree update benchmark (not a test): wp12_bench [repeats]
add_executable(wp12_bench ${PROJECT_SOURCE_DIR}/tools/wp12/bench.cpp)
target_link_libraries(wp12_bench PRIVATE kspcore)
ksp_set_build_flags(wp12_bench)
target_compile_definitions(wp12_bench PRIVATE KSPCORE_DATA_DIR="${PROJECT_SOURCE_DIR}/data")
endif()
# Test hygiene: the cases must not depend on the order they run in (tests/test_hygiene.cpp resets the shared state between cases).
# A full run takes ~2 min, so each of three fixed seeds shuffles the whole suite and runs its first 190 cases (a different quarter of
# the suite per seed, ~100 s in all). The complete sweep, any seed:  kspcore_tests --order-by=rand -rs=<seed>
foreach(seed 11 27 42)
    add_test(NAME unit.random_order.${seed} COMMAND kspcore_tests --order-by=rand -rs=${seed} -f=1 -l=190)
    set_tests_properties(unit.random_order.${seed} PROPERTIES LABELS unit TIMEOUT 600)
endforeach()
# WP-31 data gate: ROSTER of every save in testdata / harness/inputs, experience recomputed from the raw flight logs (also part of `unit`)
add_test(NAME data.roster_experience COMMAND kspcore_tests "-tc=WP-31 data gate*")
# WP-29a: the VPP wheel core and KSPWheelController against the independent Python reference (tools/wp29a/vpp_reference.py): 0 ULP
add_test(NAME wheels.vpp COMMAND kspcore_tests "-tc=WP-29a*")
set_tests_properties(wheels.vpp PROPERTIES LABELS unit TIMEOUT 600)
# WP-29b: the wheel part modules against the independent Python reference (tools/wp29b/wheel_reference.py): 0 ULP
add_test(NAME wheels.modules COMMAND kspcore_tests "-tc=WP-29b*")
set_tests_properties(wheels.modules PROPERTIES LABELS unit TIMEOUT 900)
# WP-52: Breaking Ground robotics (servos, controller) against the independent Python reference (tools/wp52/servo_reference.py): 0 ULP
add_test(NAME robotics COMMAND kspcore_tests "-tc=WP-52*")
set_tests_properties(robotics PROPERTIES LABELS unit TIMEOUT 900)
set_tests_properties(data.roster_experience PROPERTIES LABELS data TIMEOUT 600)

# ---- portable libm vs the real VS2013 / UCRT DLLs (Windows only; skips elsewhere): 300k inputs per function
add_test(NAME libm.portable_vs_dll COMMAND libm_check 300000)
set_tests_properties(libm.portable_vs_dll PROPERTIES LABELS libm TIMEOUT 300)

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
add_test(NAME capi.c COMMAND capi_c ${KSP_HARNESS_DIR}/inputs/Physics.cfg ${PROJECT_SOURCE_DIR}/data/terrain)
add_test(NAME capi.ue_like COMMAND capi_ue_like ${KSP_HARNESS_DIR}/inputs/Physics.cfg ${PROJECT_SOURCE_DIR}/data/terrain)

# ---- golden gates: golden_runner writes candidates, harness/compare/compare.py judges them -----------------------------
# ksp_golden_gate(<test name> <runner dataset> [GOLDEN <file>] [TOL <file>] [TABLES <t1,t2>] [KEYS <t:c1,c2/...>]
#                 [KEEP_ROWS <rowspec>] [DROP_ROWS <rowspec>] [KEEP_ROWS_FILE <json>] [DROP_KEYS_FILE <json>]
#                 [RENUMBER <t:col:groupcol/...>] [DROP_HEX] [COLUMNS <t:c1,c2/...>] [DROP_COLUMNS <t:c1,c2/...>])
#   GOLDEN defaults to harness/golden/<dataset>/<dataset>.json and TOL to harness/compare/tolerances.json.
#   In-game capture files (harness/captures/out, out2) carry `<col>_hex` columns: use DROP_HEX.
#   The filter options (one coherent set, applied by tools/filter_golden.py in a fixed order; grammar, order and the
#   gates using each option: README.md "Golden gate options") restrict the golden copy to the package's scope. Every
#   use must say why next to the gate and in STATUS.md (another package's code, objects that are not the experiment's
#   inputs); never to hide a mismatch. rowspec = t:col=v1|v2+col2^prefix/t2:... (conditions OR'd per table).
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
set(KSP_CANDIDATE_DIR "${CMAKE_BINARY_DIR}/candidate")
set(KSP_CAPTURES_DIR "${KSP_HARNESS_DIR}/captures/out")
set(KSP_CAPTURES2_DIR "${KSP_HARNESS_DIR}/captures/out2")  # batch 2 (C4, C5, C6, C8, C9, C10, terrain)
set(KSP_CAPTURE_TOL "${CMAKE_CURRENT_SOURCE_DIR}/tolerances_capture.json")

function(ksp_golden_gate name dataset)
    cmake_parse_arguments(G "DROP_HEX"
        "GOLDEN;TOL;TABLES;KEYS;KEEP_ROWS;DROP_ROWS;KEEP_ROWS_FILE;DROP_KEYS_FILE;RENUMBER;COLUMNS;DROP_COLUMNS" "" ${ARGN})
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
            "-DKEYS=${G_KEYS}"
            "-DKEEP_ROWS=${G_KEEP_ROWS}"
            "-DDROP_ROWS=${G_DROP_ROWS}"
            "-DKEEP_ROWS_FILE=${G_KEEP_ROWS_FILE}"
            "-DDROP_KEYS_FILE=${G_DROP_KEYS_FILE}"
            "-DRENUMBER=${G_RENUMBER}"
            "-DDROP_COLUMNS=${G_DROP_COLUMNS}"
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
# WP-08 patched conics: PatchedConics.CalculatePatch chains (encounter, escape, SOI transitions) on the harness's static
# bodies (the runner freezes every body position at the origin, as the harness never moved them); harness tolerances
# unchanged.
ksp_golden_gate(golden.patched_conics patched_conics)
# WP-08 on-rails propagation vs the in-game capture C8 (moving bodies, rotating Planetarium frame, warp 1x-100,000x,
# on-rails SOI transitions). Exclusions (STATUS.md WP-08): samples world_rel_* (floating-origin world positions and the
# vessel transform / localCoM: WP-14/WP-20) and packed/loaded (Vessel state: WP-14/WP-19); initial.note (free text) and
# the inputs the runner reads (initial.req_*; segments.requested_rate is gated since WP-10: it is TimeWarp.warpRates[rate_index]); soi_events rows of the two asteroids (spawned
# by the game's discovery scenario, not capture inputs): only the "HC8 " probes, keyed by (vessel, UT).
ksp_golden_gate(capture.orbit_rails orbit_rails GOLDEN ${KSP_CAPTURES2_DIR}/orbit_rails_ingame.json DROP_HEX
                TOL ${KSP_CAPTURE_TOL} KEEP_ROWS "soi_events:vessel^HC8" KEYS "soi_events:vessel,UT"
                COLUMNS "initial:id,body,inclination,eccentricity,semiMajorAxis,LAN,argumentOfPeriapsis,meanAnomalyAtEpoch,epoch,meanMotion,period,ObTAtEpoch,mu,spawned/samples:id,frame,segment,UT,warp_rate,ref_body,updateUT,pos_x,pos_y,pos_z,vel_x,vel_y,vel_z,relpos_at_UT_x,relpos_at_UT_y,relpos_at_UT_z,semiMajorAxis,eccentricity,inclination,LAN,argumentOfPeriapsis,meanAnomalyAtEpoch,epoch,ObT,meanAnomaly,trueAnomaly,update_mode/segments:segment,rate_index,requested_rate,achieved_rate,frames,UT_start,UT_end,Time_fixedDeltaTime,TimeWarp_fixedDeltaTime,timeScale")

# WP-19a on-rails propagation through REAL vessels vs the same in-game capture C8: probes are unloaded, packed Vessels in the FlightGlobals
# registry; every frame runs Planetarium.FixedUpdate, VesselPrecalculate.MainPhysics (OrbitDriver.UpdateOrbit, CalculatePhysicsStats, UpdatePosVel,
# UpdateAcceleration, Vessel.CheckKill) and OrbitDriver.FixedUpdate in the order measured in game; SOI changes come from GameEvents.onVesselSOIChanged.
# Adds to capture.orbit_rails: samples.packed and samples.loaded (Vessel state) for the vessel rows (the two body rows "body:Mun" / "body:Minmus"
# are gated by capture.orbit_rails and dropped here because a body has no Vessel state). Exclusions as in capture.orbit_rails: initial.note, initial.req_*.
ksp_golden_gate(capture.vessel_rails vessel_rails GOLDEN ${KSP_CAPTURES2_DIR}/orbit_rails_ingame.json DROP_HEX
                TOL ${KSP_CAPTURE_TOL} KEEP_ROWS "soi_events:vessel^HC8" DROP_ROWS "samples:id^body:" KEYS "soi_events:vessel,UT"
                COLUMNS "initial:id,body,inclination,eccentricity,semiMajorAxis,LAN,argumentOfPeriapsis,meanAnomalyAtEpoch,epoch,meanMotion,period,ObTAtEpoch,mu,spawned/samples:id,frame,segment,UT,warp_rate,ref_body,updateUT,pos_x,pos_y,pos_z,vel_x,vel_y,vel_z,relpos_at_UT_x,relpos_at_UT_y,relpos_at_UT_z,semiMajorAxis,eccentricity,inclination,LAN,argumentOfPeriapsis,meanAnomalyAtEpoch,epoch,ObT,meanAnomaly,trueAnomaly,packed,loaded,update_mode/frames:frame,UT,Zup_X_x,Zup_X_y,Zup_X_z,Zup_Y_x,Zup_Y_y,Zup_Y_z,Zup_Z_x,Zup_Z_y,Zup_Z_z,inverseRotAngle,frame_is_rotating/segments:segment,rate_index,requested_rate,achieved_rate,frames,UT_start,UT_end,Time_fixedDeltaTime,TimeWarp_fixedDeltaTime,timeScale")

# WP-11 terrain: PQS.GetSurfaceHeight for Kerbin and Mun (140,522 samples: 1-degree grids, the 0.01-degree KSC patch,
# the pad) vs the in-game batch-2 capture, plus the per-scene probe heights and the PQS radii. Kept columns: the
# capture's bodies.mod_count/isStarted/isActive and scene_probe.has_pqs/isStarted/isActive/mod_count/ut/go_active are
# scene state of the running game (Kerbin's runtime stack also holds 9 PQSCity2 set-dressing objects spawned by the
# space-centre scene code, not by the PQS prefab), and the mods/mod_params/map_assets tables are checked by
# tests/test_terrain.cpp (stack order, prefab fields) instead.
ksp_golden_gate(capture.terrain_height terrain_height GOLDEN ${KSP_CAPTURES2_DIR}/terrain_height.json DROP_HEX
                TOL ${KSP_CAPTURE_TOL} TABLES bodies,scene_probe,samples
                COLUMNS "bodies:body,Radius,pqs_radius,radiusMin,radiusMax,ocean,pqs_name/scene_probe:scene,body,h_ksc,h_override_ksc,h_override_0_0")

# GameData corpus: parse + round-trip every cfg/craft in harness/inputs (no golden; self-consistency + no errors)
add_test(NAME corpus.gamedata
    COMMAND golden_runner gamedata --harness ${KSP_HARNESS_DIR} --out ${KSP_CANDIDATE_DIR})
set_tests_properties(corpus.gamedata PROPERTIES LABELS corpus TIMEOUT 900)

# WP-05 part database + WP-06 module host: GameDatabase + PartLoader on the GameData mirror (testdata/GameData-mirror, or the
# real GameData via KSP_GAMEDATA_ROOT / golden_runner --gamedata) vs the in-game capture C9 (captures/out2/part_database.json).
# Bit-exact. Every module of every compiled part is instantiated through the WP-06 module host (AddComponent -> Awake -> Load),
# so module_fields covers EVERY KSPField (cfg-set values, C# initialisers, ported OnAwake/OnLoad results) and the parts table
# gates module_mass / module_cost (IPartMassModifier / IPartCostModifier). Documented scope limits (STATUS.md WP-05, WP-06):
#   DROP_ROWS  the 5 parts PartLoader builds in code from Unity prefabs (EVA kerbals, flag): their base values and modules are
#              Unity-serialized prefab data, not GameData (the 4 DLC-suit error rows ARE gated); assets rows of kind
#              `texture` (renderer/material texture slots: they need the .mu model contents)
#   DROP_KEYS_FILE  individually excluded rows with reasons (tests/data/part_database_exclusions.json): the 18 module_fields
#              rows holding a solar panel pivot's localRotation (a transform inside the .mu model)
#   RENUMBER   assets.idx renumbered per part after dropping the texture rows
#   DROP_COLUMNS  parts.partSize (renderer bounds of the model), variants.materials (model materials)
set(KSP_PREFAB_PARTS "kerbalEVA|kerbalEVAfemale|flag|kerbalEVASlimSuit|kerbalEVASlimSuitFemale")
ksp_golden_gate(capture.part_database part_database GOLDEN ${KSP_CAPTURES2_DIR}/part_database.json DROP_HEX
    TOL ${KSP_CAPTURE_TOL}
    DROP_ROWS "parts:name=${KSP_PREFAB_PARTS}/part_cfg_values:part=${KSP_PREFAB_PARTS}/modules:part=${KSP_PREFAB_PARTS}/module_fields:part=${KSP_PREFAB_PARTS}/module_propellants:part=${KSP_PREFAB_PARTS}/resources:part=${KSP_PREFAB_PARTS}/attach_nodes:part=${KSP_PREFAB_PARTS}/drag_cubes:part=${KSP_PREFAB_PARTS}/variants:part=${KSP_PREFAB_PARTS}/assets:part=${KSP_PREFAB_PARTS}+kind=texture"
    DROP_KEYS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/data/part_database_exclusions.json
    RENUMBER "assets:idx:part"
    DROP_COLUMNS "parts:partSize/variants:materials")
set_tests_properties(capture.part_database PROPERTIES FIXTURES_SETUP partdb)
# WP-15 resource flow: Part.GetResourceMass() (PartResourceList amounts x PartResourceDefinition.density) of every part of the
# 17 crafts of the C5 delta-v capture (table `parts`). The craft files are harness/inputs/Ships (+ analysis/physics/craft for
# PhysProf-M2 / -L2-AS), the capture's own inputs; only the resource_mass column is WP-15's (mass / module_mass belong to WP-13).
# The flow solver itself has no capture yet (STATUS.md WP-15: harness dataset `resource_flow`).
ksp_golden_gate(capture.resource_mass resource_mass GOLDEN ${KSP_CAPTURES2_DIR}/deltav_readout.json DROP_HEX
    TOL ${KSP_CAPTURE_TOL} TABLES parts COLUMNS parts:craft_id,part_index,part,resource_mass)
# WP-18 ship construction: ShipConstruct.GetShipMass (dryMass / fuelMass out parameters) and the per-part masses (Part.mass after
# UpdateMass, module mass) of the 17 crafts of the C5 delta-v capture, built as ShipConstruction.CreateConstructFromTemplate builds
# them (craft file -> KSPUpgradePipeline -> ShipConstruct.LoadShip). Same craft set and inputs as capture.resource_mass. NOT gated:
# craft.total_mass (it adds the editor crew mass, VesselCrewManifest: tests/test_ship_construct.cpp checks it is the no-crew total plus an
# integral multiple of KerbalCrewMass), craft.stage_count / loaded / error (VesselDeltaV, WP-17), parts.inverseStage / decoupleStage /
# activationStage / isEngine / isDecoupler (staging, WP-16 / WP-23).
ksp_golden_gate(capture.ship_construct ship_construct GOLDEN ${KSP_CAPTURES2_DIR}/deltav_readout.json DROP_HEX
    TOL ${KSP_CAPTURE_TOL} TABLES craft,parts
    COLUMNS "craft:craft_id,parts,dry_mass,fuel_mass/parts:craft_id,part_index,part,mass,module_mass")

# WP-16 staging: StageManager (SortIcons / UpdateStageGroups / SetSeparationIndices) and the decoupler modules over the 17 crafts of
# the C5 delta-v capture. Gated columns: craft.stage_count, parts.inverseStage (Part.inverseStage after the editor rebuilt the
# staging sequence) and parts.isDecoupler. decoupleStage / activationStage and the stages / engines tables are VesselDeltaV's (WP-17).
# The craft files are the capture's own inputs (see capture.resource_mass).
ksp_golden_gate(capture.staging_sequence staging_sequence GOLDEN ${KSP_CAPTURES2_DIR}/deltav_readout.json DROP_HEX
    TOL ${KSP_CAPTURE_TOL} TABLES craft,parts
    COLUMNS "craft:craft_id,stage_count/parts:craft_id,part_index,part,inverseStage,isDecoupler")
# GameDatabase load order vs the game's own log of the capture run (every "Load(Audio|Texture|Model): url" and
# "Config(TYPE) url" line, in order). Skipped (exit 0 with a note) when the log is not present.
add_test(NAME capture.gamedatabase_load_order
    COMMAND ${Python3_EXECUTABLE} -I ${PROJECT_SOURCE_DIR}/tools/wp05/check_load_log.py
            ${KSP_CANDIDATE_DIR}/part_database/gamedatabase_load.log
            ${KSP_LOAD_LOG})
set_tests_properties(capture.gamedatabase_load_order PROPERTIES LABELS golden FIXTURES_REQUIRED partdb TIMEOUT 300)
# WP-04 save model: Load -> Save round trip of every original save (decompiled/KSP-dev/saves/**/*.sfs, *.craft) and
# stock craft (decompiled/KSP-dev/Ships/**/*.craft); R2 idempotent, R1 byte-identical or every difference documented
# (tools/save_corpus/save_corpus.cpp). No golden dataset: original game data is the reference.
set(KSP_DEV_DIR "${KSP_PROJECT_ROOT}/decompiled/KSP-dev")
if(EXISTS "${KSP_DEV_DIR}/saves")
    add_test(NAME corpus.saves
        COMMAND save_corpus --saves "${KSP_DEV_DIR}/saves" --ships "${KSP_DEV_DIR}/Ships")
    set_tests_properties(corpus.saves PROPERTIES LABELS corpus TIMEOUT 1800)
endif()

# WP-14 vessel model: the assembled-vessel capture C10 (captures/out2/aero_forces_vessel.json). structure: the part tree of the three
# PhysProf crafts assembled into a Vessel; states: the vessel-level atmosphere values + srfSpeed. The parts and aoa_sweep tables are
# the drag-cube pipeline (WP-21). Dropped columns (each documented in STATUS.md "WP-14"):
#   structure.cube_names        cube weights are set by module OnStart logic / DragCubeList.ForceUpdate (WP-21 and the module packages);
#                               the cube NAMES are checked by tests/test_vessel.cpp
#   structure.node_contact_areas  AttachNode.contactArea is written by DragCubeList.SetPartOcclusion (WP-21): gated by
#                               capture.aero_forces_vessel (below), not here
#   states.spd                  input: Vessel.speed is the FlightIntegrator spd copied through ApplyFlightIntegratorAtmosphere, and the
#                               runner feeds it the captured spd, so the column would only compare the capture with itself
#   states.frame, UT, altitude, aoa_deg, srf_velocity_*, ref_up_*  inputs of the experiment or rigid-body state. srf_velocity_* feed
#                               UpdateSurfaceSpeeds; the captured `altitude` (the root part's, not the CoM altitude FlightIntegrator uses)
#                               is only the FALLBACK altitude when the inversion below fails
# What the remaining states columns are (see ds_vessel_state.cpp):
#   staticPressurekPa, staticPressureAtm, atmosphericTemperature  consistency checks, NOT independent gates: they are reproduced from an
#                               altitude recovered by inverting the captured static pressure and a temperature offset recovered by
#                               inverting the captured temperature. The altitude-to-pressure and altitude-to-temperature models are
#                               therefore not gated here (golden.atmosphere_curves gates them).
#   srfSpeed                    output of UpdateSurfaceSpeeds from the captured srf_velocity
#   density, mach, dynamicPressurekPa, pseudoReynolds, pseudoReDragMult  functions of the captured P, T and spd
#   cacheDragCubeMultiplier, parts  PhysicsGlobals.DragCubeMultiplier and vessel.parts.Count of the assembled vessel
ksp_golden_gate(capture.vessel_state vessel_state GOLDEN ${KSP_CAPTURES2_DIR}/aero_forces_vessel.json DROP_HEX
                TOL ${KSP_CAPTURE_TOL} TABLES structure,states
                DROP_COLUMNS "structure:cube_names,node_contact_areas/states:frame,UT,altitude,spd,aoa_deg,srf_velocity_x,srf_velocity_y,srf_velocity_z,ref_up_x,ref_up_y,ref_up_z")

# WP-21 aerodynamics (numeric part). One templated port, two float policies (src/aero, ksp/aero/float_eval.hpp):
#   golden.aero_forces       the .NET harness golden, every table (drag_curve_value, cube_inputs, cube_weighted, cube_drag,
#                            drag_chain_kerbin, lifting_surface), computed with aero::NetFloat (per-operation float32 rounding as
#                            RyuJIT does): it proves the operation order of the port against KSP's IL on the rows that have no
#                            in-game capture (drag_chain_kerbin, lifting_surface). harness/compare/tolerances.json unchanged.
#   capture.aero_forces_mono the in-game Mono capture of the drag-cube tables with aero::MonoFloat (the game's policy, production
#                            code). drag_curve_value of the same file stays WP-09's capture.physics_drag_curve.
ksp_golden_gate(golden.aero_forces aero_forces)
ksp_golden_gate(capture.aero_forces_mono aero_forces_mono GOLDEN ${KSP_CAPTURES_DIR}/aero_forces_mono.json
                TABLES cube_inputs,cube_weighted,cube_drag DROP_HEX TOL ${KSP_CAPTURE_TOL})
# WP-21 drag-cube pipeline on the C10 assembled vessels (captures/out2/aero_forces_vessel.json; see ds_aero_forces_vessel.cpp).
#   structure  only node_contact_areas (AttachNode.contactArea from SetPartOcclusion; the capture writes it with 4 significant digits)
#              and the part name; the tree columns are capture.vessel_state's; cube_names is an INPUT here (ModuleJettison WP-26 + the
#              model's jettison transform, ModulePartVariants WP-13 set the weights at flight start)
#   parts      inputs dropped: dragVectorDirLocal_*, dragVectorMag, dynamicPressurekPa (rigidbody velocity); DragVector_* (negated
#              input: -dragVectorDirLocal while rotateDragVector is false); staticPressureAtm (needs
#              the part's world altitude, WP-19/20); exposedArea (the previous physics frame's ExposedArea: UpdateAerodynamics reads it
#              before this frame's SetDrag). The runner does not write any dropped column. 46 gated columns. No row is filtered.
#   aoa_sweep  input dropped: flow_local_* (R(aoa) * dragVectorDirLocal, rotated in game). No row is filtered.
ksp_golden_gate(capture.aero_forces_vessel aero_forces_vessel GOLDEN ${KSP_CAPTURES2_DIR}/aero_forces_vessel.json DROP_HEX
                TOL ${KSP_CAPTURE_TOL} TABLES structure,parts,aoa_sweep
                COLUMNS "structure:craft,part_index,part,node_contact_areas"
                DROP_COLUMNS "parts:dragVectorDirLocal_x,dragVectorDirLocal_y,dragVectorDirLocal_z,dragVectorMag,dynamicPressurekPa,staticPressureAtm,exposedArea,DragVector_x,DragVector_y,DragVector_z/aoa_sweep:flow_local_x,flow_local_y,flow_local_z")

# WP-22 CelestialBody.GetAtmoThermalStats / GetFullTemperature(pos) vs the in-game capture C4 (captures/out2/atmosphere_thermal_offsets.json,
# see tools/golden_runner/ds_atmosphere_thermal_offsets.cpp). Only `samples` is compared: `epochs` and `body_epoch` are the engine inputs
# (scaled-space transforms, run-time orbit values), echoed by the runner from the capture itself.
ksp_golden_gate(capture.atmosphere_thermal_offsets atmosphere_thermal_offsets GOLDEN ${KSP_CAPTURES2_DIR}/atmosphere_thermal_offsets.json
                DROP_HEX TOL ${KSP_CAPTURE_TOL} TABLES samples)

# WP-22 FlightIntegrator thermal model vs the in-game capture C6 (captures/out2/thermal_step.json; see tools/golden_runner/ds_thermal_step.cpp,
# its header lists every input taken from the capture). Scope filters, each one for data this package cannot reproduce from the file:
#   vessel  only the atmosphere / thermal scalars recomputed from the frame inputs; the inputs (UT, altitude, srfSpeed, spd, sunDot,
#           atmosphereTemperatureOffset, bodyEmissive/AlbedoFlux, ...) and `solarFlux` (sun-ray distance) are not written
#   part_constants  resource_mass (propellant left after the C10 ascent, not an input of the craft file) is dropped
#   parts   frame 0 of each case (no previous row for the pre-state); columns that are inputs of the step (radiativeArea, exposedArea,
#           thermalMass, ptd_intConductionFlux, ptd_skinConductionFlux: conduction to the 16 unlogged parts) or that need
#           the engine (skinExposedAreaFrac stays: computed); skinThermalMass etc. are computed
ksp_golden_gate(capture.thermal_step thermal_step GOLDEN ${KSP_CAPTURES2_DIR}/thermal_step.json DROP_HEX
                TOL ${KSP_CAPTURE_TOL} TABLES vessel,parts,part_constants
                DROP_ROWS "parts:frame=0"
                DROP_KEYS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/data/thermal_step_exclusions.json
                COLUMNS "vessel:case,frame,mach,density,staticPressurekPa,dynamicPressurekPa,atmosphericTemperature,externalTemperature,convectiveCoefficient,convectiveMachLerp,densityThermalLerp,solarFluxMultiplier,backgroundRadiationTemp,backgroundRadiationTempExposed,fi_isAnalytical"
                DROP_COLUMNS "parts:radiativeArea,exposedArea,thermalMass,ptd_intConductionFlux,ptd_skinConductionFlux,ptd_convectionArea,ptd_postShockExtTemp,ptd_brtExposed,ptd_expFlux,ptd_unexpFlux,ptd_sunFlux,ptd_bodyFlux/part_constants:resource_mass")
# WP-23 engines (ModuleEngines / ModuleEnginesFX / MultiModeEngine; see ds_engine_thrust.cpp, ds_engine_deltav.cpp).
#   golden.engine_thrust    the .NET harness golden (harness/golden/engine_thrust): all four tables, every column, computed with
#                           aero::NetFloat (the generator ran KSP's IL on .NET Framework 4.8). harness/compare/tolerances.json unchanged.
#   capture.engine_deltav   the in-game engine rows of the C5 delta-v capture (deltav_readout.json `engines`, Mono float semantics,
#                           aero::MonoFloat = production code). startBurnStage is not gated: it is engine.part.inverseStage
#                           (DeltaVEngineInfo.cs:88), gated by capture.staging_sequence as parts.inverseStage. The other 12 columns
#                           (ispVac, ispASL, ispActual, thrustVac, thrustASL, thrustActual, requiresAir, propellants, part, part_index,
#                           key columns) are ModuleEngines outputs. No row is filtered.
ksp_golden_gate(golden.engine_thrust engine_thrust)
ksp_golden_gate(capture.engine_deltav engine_deltav GOLDEN ${KSP_CAPTURES2_DIR}/deltav_readout.json DROP_HEX
                TOL ${KSP_CAPTURE_TOL} TABLES engines DROP_COLUMNS "engines:startBurnStage")
# WP-17 delta-v calculation (VesselDeltaV / DeltaVStageInfo / DeltaVEngineInfo / DeltaVPartInfo; see ds_deltav_stages.cpp).
#   capture.deltav_stages   the in-game stage readout of the C5 delta-v capture (deltav_readout.json): `stages` (all columns), `totals`
#                           (all columns but calc_events: the number of onDeltaVCalcsCompleted events the capture's own handler saw
#                           after its SetCalcsDirty, not a VesselDeltaV value) and from `parts` the DeltaVPartInfo columns
#                           decoupleStage / activationStage / isEngine (inverseStage, isDecoupler: WP-16, mass columns: WP-18 /
#                           WP-15). Engines are capture.engine_deltav (WP-23). Mono float semantics; no row is filtered.
ksp_golden_gate(capture.deltav_stages deltav_stages GOLDEN ${KSP_CAPTURES2_DIR}/deltav_readout.json DROP_HEX
                TOL ${KSP_CAPTURE_TOL} TABLES stages,totals,parts
                DROP_COLUMNS "totals:calc_events"
                COLUMNS "parts:craft_id,part_index,part,decoupleStage,activationStage,isEngine")
