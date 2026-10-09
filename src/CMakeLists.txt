# kspcore: engine-agnostic static library (no Unity / Unreal dependencies).
add_library(kspcore STATIC
    core/log.cpp
    core/delegate.cpp
    dotnet/math.cpp
    dotnet/number.cpp
    dotnet/random.cpp
    dotnet/text.cpp
    dotnet/version.cpp
    unity/vector.cpp
    unity/animation_curve.cpp
    unity/random.cpp
    unity/quaternion_native.cpp
    math/vector3d.cpp
    math/quaterniond.cpp
    math/util_math.cpp
    math/matrix4x4d.cpp
    math/mathfx.cpp
    curves/float_curve.cpp
    curves/double_curve.cpp
    curves/serialized_float_curve.cpp
    config/config_node.cpp
    config/parse_extensions.cpp
    config/kspparseable.cpp
    physics/physics_globals.cpp
    physics/vessel_ranges.cpp
    celestial/celestial_body.cpp
    celestial/planetarium.cpp
    celestial/celestial_system.cpp
    celestial/kerbol_system_data.cpp
    celestial/flight_atmosphere.cpp
    celestial/celestial_body_orbit.cpp
    orbit/orbit.cpp
    orbit/targeting.cpp
    orbit/orbit_snapshot.cpp
    orbit/orbit_util.cpp
    events/base_game_event.cpp
    events/game_events.cpp
    fsm/kerbal_fsm.cpp
    localization/localizer.cpp
    util/ksp_util.cpp
    util/misc_util.cpp
    util/simplex.cpp
    save/save_environment.cpp
    save/game_parameters.cpp
    save/crew.cpp
    save/proto_vessel.cpp
    save/game.cpp
    save/upgrade_pipeline.cpp
    save/ship_construct.cpp
    save/game_persistence.cpp
    capi/kspcore_c.cpp
)
add_library(ksp::core ALIAS kspcore)
target_include_directories(kspcore PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(kspcore PUBLIC cxx_std_20)
ksp_set_build_flags(kspcore)
# The Unreal bridge links <core>/build/RelWithDebInfo/kspcore.lib: put the archive at the top of the build tree.
set_target_properties(kspcore PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
