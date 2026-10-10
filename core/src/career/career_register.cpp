// WP-36: RegisterCareerScenarioModules.
#include "ksp/career/career.hpp"

namespace ksp {

void RegisterCareerScenarioModules() {
    Funding::Register();
    Reputation::Register();
    ProgressTracking::Register();
    ScenarioUpgradeableFacilities::Register();
    Strategies::StrategySystem::Register();
    ScenarioAchievements::Register();
    ScenarioDestructibles::Register();
    VesselRecovery::Register();
}

}  // namespace ksp
