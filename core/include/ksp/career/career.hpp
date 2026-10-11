// WP-36: umbrella header of the career package (funds, reputation, facilities, strategies, progress tracking, recovery) and the
// registration of its ScenarioModule types with the ScenarioModuleFactory.
#pragma once

#include "ksp/career/career_environment.hpp"
#include "ksp/career/currency.hpp"
#include "ksp/career/funding.hpp"
#include "ksp/career/game_variables.hpp"
#include "ksp/career/progress.hpp"
#include "ksp/career/scenario_misc.hpp"
#include "ksp/career/strategies.hpp"
#include "ksp/career/upgradeables.hpp"

namespace ksp {

// Registers Funding, Reputation, ProgressTracking, ScenarioUpgradeableFacilities, StrategySystem (Strategies namespace),
// ScenarioAchievements, ScenarioDestructibles and VesselRecovery with ScenarioModuleFactory (a type without a registration is
// instantiated as the generic ScenarioModule, which saves only its host-schema fields).
void RegisterCareerScenarioModules();

}  // namespace ksp
