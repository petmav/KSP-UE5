// WP-38 slice 2: helpers shared by the FinePrint parameter and contract translation units (not a public header).
#pragma once

#include <string>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/contract.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/fineprint/utilities.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::fpcommon {

using KSP::Localization::Localizer;

// FlightGlobals.Bodies[index]
inline CelestialBody* BodyAt(int index) { return FlightRegistry::Bodies().at(static_cast<size_t>(index)); }
// CelestialBody.flightGlobalsIndex
inline int BodyIndexOf(CelestialBody* b) { return b->flightGlobalsIndex; }
inline std::string Fmt(const char* tag, std::initializer_list<std::string> args = {}) { return Localizer::Format(tag, args); }
inline std::string I(int v) { return dotnet::FormatInt64(v); }
// Convert.ToDecimal(Math.Round(x)).ToString("#,###")
std::string RoundedThousands(double x);

// HighLogic.LoadedScene / LoadedSceneIsFlight
inline GameScenes Scene() { return SaveEnvironment::Current().loadedScene; }
inline bool SceneIsFlight() { return SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT; }

// `x ?? ""` null-string C# values are modelled as empty strings
}  // namespace ksp::fpcommon
