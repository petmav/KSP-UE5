# kspcore: engine-agnostic static library (no Unity / Unreal dependencies).
add_library(kspcore STATIC
    core/log.cpp
    dotnet/math.cpp
    dotnet/number.cpp
    dotnet/random.cpp
    dotnet/text.cpp
    unity/vector.cpp
    unity/animation_curve.cpp
    unity/random.cpp
    math/vector3d.cpp
    math/quaterniond.cpp
    math/util_math.cpp
    math/matrix4x4d.cpp
    math/mathfx.cpp
    curves/float_curve.cpp
    curves/double_curve.cpp
    config/config_node.cpp
    config/parse_extensions.cpp
    config/kspparseable.cpp
    capi/kspcore_c.cpp
)
add_library(ksp::core ALIAS kspcore)
target_include_directories(kspcore PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(kspcore PUBLIC cxx_std_20)
ksp_set_build_flags(kspcore)
# The Unreal bridge links <core>/build/RelWithDebInfo/kspcore.lib: put the archive at the top of the build tree.
set_target_properties(kspcore PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
