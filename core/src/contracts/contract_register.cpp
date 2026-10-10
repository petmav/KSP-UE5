// WP-37: RegisterContractScenarioModules.
#include "ksp/contracts/contracts.hpp"

namespace ksp {

void RegisterContractScenarioModules() { Contracts::ContractSystem::Register(); }

}  // namespace ksp
