// WP-37: contract system (Contracts/*, Contracts.Agents, Contracts.Parameters, Contracts.Predicates, Contracts.Templates, PartTestConstraint,
// ModuleTestSubject, ScenarioCustomWaypoints). Include this to get all of it.
#pragma once

#include "ksp/contracts/agents.hpp"
#include "ksp/contracts/contract.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/contracts/module_test_subject.hpp"
#include "ksp/contracts/parameters.hpp"
#include "ksp/contracts/predicates.hpp"
#include "ksp/contracts/templates.hpp"
#include "ksp/contracts/text_gen.hpp"

namespace ksp {
// ScenarioModuleFactory registration of the contract scenario modules (ContractSystem, ScenarioCustomWaypoints)
void RegisterContractScenarioModules();
}  // namespace ksp
