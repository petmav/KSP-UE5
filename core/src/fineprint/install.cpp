// WP-38 slice 2: FinePrint::InstallAll(): registers the contract and parameter types of slice 2 on top of FinePrint::Install() and wires the WP-36 /
// WP-37 seams that the FinePrint ports answer (valueAndUnits, researchedValidContractObjectives, checkTouristRecoveryContractKerbals, the GrandTour
// parameter factories, discoverVessel, estimatePartSize is left to the part geometry package).
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/templates.hpp"
#include "ksp/fineprint/contracts.hpp"
#include "ksp/fineprint/parameters.hpp"
#include "ksp/fineprint/compatibility.hpp"
#include "ksp/fineprint/scenario_contract_events.hpp"
#include "ksp/fineprint/sentinel.hpp"
#include "ksp/save/game.hpp"

namespace ksp::FinePrint {

void InstallAll() {
    using ksp::Contracts::ContractSystem;
    namespace P = ksp::Contracts::Parameters;
    namespace L = ksp::Contracts::Lite;
    namespace T = ksp::Contracts::Templates;
    Install();
    // contracts (FinePrint/Contracts/*.cs; ContractSystem.GenerateTypes registers them by reflection in the assembly order)
    ContractSystem::RegisterContractType("ARMContract", [] { return std::make_unique<Contracts::ARMContract>(); });
    ContractSystem::RegisterContractType("BaseContract", [] { return std::make_unique<Contracts::BaseContract>(); });
    ContractSystem::RegisterContractType("CometSampleContract", [] { return std::make_unique<Contracts::CometSampleContract>(); });
    ContractSystem::RegisterContractType("ISRUContract", [] { return std::make_unique<Contracts::ISRUContract>(); });
    ContractSystem::RegisterContractType("SatelliteContract", [] { return std::make_unique<Contracts::SatelliteContract>(); });
    ContractSystem::RegisterContractType("StationContract", [] { return std::make_unique<Contracts::StationContract>(); });
    ContractSystem::RegisterContractType("SurveyContract", [] { return std::make_unique<Contracts::SurveyContract>(); });
    ContractSystem::RegisterContractType("TourismContract", [] { return std::make_unique<Contracts::TourismContract>(); });
    // SentinelMission/*.cs (namespace SentinelMission)
    ContractSystem::RegisterContractType("CometDetectionContract", [] { return std::make_unique<SentinelMission::CometDetectionContract>(); });
    ContractSystem::RegisterContractType("SentinelContract", [] { return std::make_unique<SentinelMission::SentinelContract>(); });
    ContractSystem::RegisterParameterType("SentinelParameter", [] { return std::make_unique<SentinelMission::SentinelParameter>(); });
    // parameters
    ContractSystem::RegisterParameterType("AsteroidParameter", [] { return std::make_unique<P::AsteroidParameter>(); });
    ContractSystem::RegisterParameterType("CometDetectionParameter", [] { return std::make_unique<P::CometDetectionParameter>(); });
    ContractSystem::RegisterParameterType("CometParameter", [] { return std::make_unique<P::CometParameter>(); });
    ContractSystem::RegisterParameterType("CometScienceParameter", [] { return std::make_unique<P::CometScienceParameter>(); });
    ContractSystem::RegisterParameterType("ConstructionParameter", [] { return std::make_unique<P::ConstructionParameter>(); });
    ContractSystem::RegisterParameterType("CrewCapacityParameter", [] { return std::make_unique<P::CrewCapacityParameter>(); });
    ContractSystem::RegisterParameterType("CrewTraitParameter", [] { return std::make_unique<P::CrewTraitParameter>(); });
    ContractSystem::RegisterParameterType("KerbalDestinationParameter", [] { return std::make_unique<P::KerbalDestinationParameter>(); });
    ContractSystem::RegisterParameterType("KerbalGeeAdventureParameter", [] { return std::make_unique<P::KerbalGeeAdventureParameter>(); });
    ContractSystem::RegisterParameterType("KerbalTourParameter", [] { return std::make_unique<P::KerbalTourParameter>(); });
    ContractSystem::RegisterParameterType("LocationAndSituationParameter", [] { return std::make_unique<P::LocationAndSituationParameter>(); });
    ContractSystem::RegisterParameterType("MobileBaseParameter", [] { return std::make_unique<P::MobileBaseParameter>(); });
    ContractSystem::RegisterParameterType("PartRequestParameter", [] { return std::make_unique<P::PartRequestParameter>(); });
    ContractSystem::RegisterParameterType("RepairPartParameter", [] { return std::make_unique<P::RepairPartParameter>(); });
    ContractSystem::RegisterParameterType("ResourceExtractionParameter", [] { return std::make_unique<P::ResourceExtractionParameter>(); });
    ContractSystem::RegisterParameterType("ResourcePossessionParameter", [] { return std::make_unique<P::ResourcePossessionParameter>(); });
    ContractSystem::RegisterParameterType("RoverWayPointParameter", [] { return std::make_unique<P::RoverWayPointParameter>(); });
    ContractSystem::RegisterParameterType("SpecificOrbitParameter", [] { return std::make_unique<P::SpecificOrbitParameter>(); });
    ContractSystem::RegisterParameterType("SpecificVesselParameter", [] { return std::make_unique<P::SpecificVesselParameter>(); });
    ContractSystem::RegisterParameterType("StabilityParameter", [] { return std::make_unique<P::StabilityParameter>(); });
    ContractSystem::RegisterParameterType("StationaryPointParameter", [] { return std::make_unique<P::StationaryPointParameter>(); });
    ContractSystem::RegisterParameterType("SurveyWaypointParameter", [] { return std::make_unique<P::SurveyWaypointParameter>(); });
    ContractSystem::RegisterParameterType("VesselDestinationParameter", [] { return std::make_unique<P::VesselDestinationParameter>(); });
    ContractSystem::RegisterParameterType("VesselSystemsParameter", [] { return std::make_unique<P::VesselSystemsParameter>(); });
    ScenarioContractEvents::Register();
    SentinelMission::SentinelScenario::Register();
    SentinelMission::SentinelModule::Register();
    // seams
    ksp::Contracts::ContractsEnvironment::Current().oldCareerSave = [](const std::string& v) { return L::CompatibilityUtilities::OldCareerSave(Game::Modes::CAREER, v); };
    ksp::Contracts::ContractsEnvironment::Current().updateCareerSave = [](ConfigNode& n) { L::CompatibilityUtilities::UpdateCareerSave(n); };
    ksp::CareerEnvironment::Current().valueAndUnits = [](int recordTrackType, double value) {
        return L::StringUtilities::ValueAndUnits(static_cast<L::RecordTrackType>(recordTrackType), value);
    };
    T::TemplateEnvironment::researchedValidContractObjectives = [](const std::string& type) { return L::ResearchedValidContractObjectives({type}); };
    T::TemplateEnvironment::checkTouristRecoveryContractKerbals = [](const std::string& name) { return L::SystemUtilities::CheckTouristRecoveryContractKerbals(name); };
    T::TemplateEnvironment::discoverVessel = [](Vessel* v) { L::DiscoverVessel(v); };
    T::TemplateEnvironment::makeVesselSystemsParameter = [](const std::string& title) -> std::unique_ptr<ksp::Contracts::ContractParameter> {
        return std::make_unique<P::VesselSystemsParameter>(std::vector<std::string>{}, std::vector<std::string>{}, title);
    };
    T::TemplateEnvironment::makeVesselDestinationParameter = [](CelestialBody* body, int entryType) -> std::unique_ptr<ksp::Contracts::ContractParameter> {
        return std::make_unique<P::VesselDestinationParameter>(body, static_cast<FlightLog_EntryType>(entryType));
    };
    T::TemplateEnvironment::makeLocationAndSituationParameter = [](CelestialBody* body, Vessel::Situations sit, const std::string& title,
                                                                   bool finalObjective) -> std::unique_ptr<ksp::Contracts::ContractParameter> {
        return std::make_unique<P::LocationAndSituationParameter>(body, sit, title, finalObjective);
    };
    T::TemplateEnvironment::grandTourAccepted = [](ksp::Contracts::Contract& c, uint32_t launchID) {
        auto* parameter = c.GetParameterT<P::VesselSystemsParameter>();
        if (parameter != nullptr && parameter->requireNew) parameter->launchID = launchID;
    };
}

}  // namespace ksp::FinePrint
