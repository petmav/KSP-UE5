# kspcore: engine-agnostic static library (no Unity / Unreal dependencies).
add_library(kspcore STATIC
    core/log.cpp
    dotnet/math.cpp
    dotnet/number.cpp
    dotnet/text.cpp
    unity/vector.cpp
    math/vector3d.cpp
    math/quaterniond.cpp
    math/util_math.cpp
)
add_library(ksp::core ALIAS kspcore)
target_include_directories(kspcore PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(kspcore PUBLIC cxx_std_20)
