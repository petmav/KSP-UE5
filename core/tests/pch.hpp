// Precompiled header for kspcore_tests (MSVC, KSPCORE_PCH). Contents only: no behaviour, no macros that change semantics.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "doctest/doctest.h"
#include "nlohmann/json.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"
