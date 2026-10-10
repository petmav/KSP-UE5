// WP-37: the stock contract templates PlantFlag and CollectScience (Assembly-CSharp/Contracts/Templates/PlantFlag.cs, CollectScience.cs)
// and the registration of the WP-37 contract / parameter / predicate types.
#include "ksp/contracts/templates.hpp"

#include <algorithm>

#include "ksp/career/career.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/agents.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/contracts/predicates.hpp"
#include "ksp/contracts/text_gen.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/unity/random.hpp"

namespace ksp::Contracts::Templates {

using KSP::Localization::Localizer;
namespace PU = Lite::ProgressUtilities;
using Lite::MannedStatus;
using Lite::ProgressType;

namespace {
CelestialBody* Home() { return Planetarium::fetch->Home; }
CelestialBody* Sun() { return Planetarium::fetch->Sun; }
std::string HomeBodyName() { return CareerEnvironment::Current().HomeBody()->name(); }
std::string Fmt1(const char* tag, const std::string& a) { return Localizer::Format(tag, {a}); }
float Def(const char* section, const char* sub, const char* key) { return ContractDefsLite::F(section, sub, key); }
template <class T>
void RemoveFirst(std::vector<T>& v, const T& x) {
    auto it = std::find(v.begin(), v.end(), x);
    if (it != v.end()) v.erase(it);
}
}  // namespace

// ---- PlantFlag -------------------------------------------------------------------------------------------------------------

// Templates/PlantFlag.cs:19
bool PlantFlag::MeetRequirements() {
    if (!ProgressTracking::Instance()->NodeComplete({HomeBodyName(), "Orbit"})) return false;
    if (!GameVariables::Instance->UnlockedEVAFlags(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::AstronautComplex))) return false;
    return true;
}

// Templates/PlantFlag.cs:36
std::string PlantFlag::GetDescription() {
    return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), targetBody->displayName(), targetBody->displayName() + "Srf", MissionSeed(), true, false, true);
}
// Templates/PlantFlag.cs:41
std::string PlantFlag::GetSynopsys() { return Fmt1("#autoLOC_273996", targetBody->displayName()); }
// Templates/PlantFlag.cs:48
std::string PlantFlag::GetTitle() { return Fmt1("#autoLOC_274001", targetBody->displayName()); }
// Templates/PlantFlag.cs:51
std::string PlantFlag::MessageCompleted() { return Fmt1("#autoLOC_274006", targetBody->displayName()); }

// Templates/PlantFlag.cs:58
void PlantFlag::OnLoad(ConfigNode& node) {
    if (node.HasValue("body")) targetBody = FlightRegistry::Bodies().at(static_cast<size_t>(dotnet::ParseInt32(*node.GetValue("body"))));
}
// Templates/PlantFlag.cs:66
void PlantFlag::OnSave(ConfigNode& node) { node.AddValue("body", static_cast<int32_t>(targetBody->flightGlobalsIndex)); }

// Templates/PlantFlag.cs:70
bool PlantFlag::Generate() {
    if (static_cast<int>(ContractSystem::Instance->GetCurrentContracts<PlantFlag>().size()) >= ContractDefsLite::I("Flag", "", "MaximumExistent")) return false;
    const auto notHomeSolid = [](CelestialBody* cb) { return cb != Home() && cb != Sun() && cb->hasSolidSurface; };
    if (Prestige() == ContractPrestige::Trivial) {
        std::vector<CelestialBody*> bodiesProgress = PU::GetBodiesProgressNoReach(ProgressType::LANDING, true, MannedStatus::MANNED, notHomeSolid);
        std::map<CelestialBody*, int> dictionary = PU::CelestialCrewCounts(
            {Vessel::Situations::LANDED, Vessel::Situations::SPLASHED, Vessel::Situations::FLYING, Vessel::Situations::SUB_ORBITAL});
        int count = static_cast<int>(bodiesProgress.size());
        while (count-- > 0) {
            CelestialBody* celestialBody = bodiesProgress[static_cast<size_t>(count)];
            auto it = dictionary.find(celestialBody);
            if (it != dictionary.end() && it->second > 0 && ProgressTracking::Instance()->NodeComplete({celestialBody->name(), "FlagPlant"}))
                RemoveFirst(bodiesProgress, celestialBody);
        }
        if (bodiesProgress.size() <= 0) return false;
        targetBody = WeightedBodyChoice(bodiesProgress);
        AddParameter(std::make_unique<Parameters::PlantFlag>(targetBody));
        AddKeywords({"Commercial"});
    } else {
        if (Prestige() != ContractPrestige::Significant) return false;
        std::vector<CelestialBody*> bodiesProgress2 = PU::GetBodiesProgress(ProgressType::LANDING, true, false, MannedStatus::MANNED, notHomeSolid);
        if (bodiesProgress2.empty()) return false;
        targetBody = WeightedBodyChoice(bodiesProgress2);
        AddParameter(std::make_unique<Parameters::PlantFlag>(targetBody));
        AddKeywords({"Commercial"});
        AddKeywords({"Pioneer"});
    }
    SetExpiry(ContractDefsLite::I("Flag", "Expiration", "MinimumExpireDays"), ContractDefsLite::I("Flag", "Expiration", "MaximumExpireDays"));
    SetDeadlineDays(static_cast<float>(ContractDefsLite::I("Flag", "Expiration", "DeadlineDays")), targetBody);
    SetFunds(Def("Flag", "Funds", "BaseAdvance"), Def("Flag", "Funds", "BaseReward"), Def("Flag", "Funds", "BaseFailure"), targetBody);
    SetReputation(Def("Flag", "Reputation", "BaseReward"), Def("Flag", "Reputation", "BaseFailure"));
    SetScience(Def("Flag", "Science", "BaseReward"));
    if (ContractSystem::Instance->AnyCurrentContracts<PlantFlag>([this](PlantFlag* contract) { return contract->TargetBody() == TargetBody(); })) return false;
    return true;
}

// Templates/PlantFlag.cs:132
std::string PlantFlag::GetHashString() { return dotnet::FormatInt64(Lite::SuperSeed(*this)); }

// ---- CollectScience ----------------------------------------------------------------------------------------------------------

// Templates/CollectScience.cs:21
bool CollectScience::MeetRequirements() { return ProgressTracking::Instance()->NodeComplete({HomeBodyName(), "Orbit"}); }

// Templates/CollectScience.cs:37
std::string CollectScience::GetDescription() {
    if (targetLocation == BodyLocation::Space)
        return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), targetBody->displayName(), targetBody->displayName(), MissionSeed(), true, false, true);
    return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), Localizer::Format("#autoLOC_272424"), targetBody->displayName() + "Srf", MissionSeed(), true, false, true);
}
// Templates/CollectScience.cs:46
std::string CollectScience::GetSynopsys() {
    if (targetLocation == BodyLocation::Space) return Fmt1("#autoLOC_272432", targetBody->displayName());
    return Fmt1("#autoLOC_272436", targetBody->displayName());
}
// Templates/CollectScience.cs:55
std::string CollectScience::GetTitle() {
    if (targetLocation == BodyLocation::Space) return Fmt1("#autoLOC_272443", targetBody->displayName());
    return Fmt1("#autoLOC_272445", targetBody->displayName());
}
// Templates/CollectScience.cs:63
std::string CollectScience::MessageCompleted() {
    if (targetLocation == BodyLocation::Space) return Fmt1("#autoLOC_272451", targetBody->displayName());
    return Fmt1("#autoLOC_272453", targetBody->displayName());
}
// Templates/CollectScience.cs:73
void CollectScience::OnLoad(ConfigNode& node) {
    if (node.HasValue("body")) targetBody = FlightRegistry::Bodies().at(static_cast<size_t>(dotnet::ParseInt32(*node.GetValue("body"))));
    if (node.HasValue("location")) targetLocation = dotnet::EnumParseFull<BodyLocation>(*node.GetValue("location"));
}
// Templates/CollectScience.cs:85
void CollectScience::OnSave(ConfigNode& node) {
    node.AddValue("body", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("location", dotnet::EnumToString(targetLocation));
}

// Templates/CollectScience.cs:90
bool CollectScience::Generate() {
    if (static_cast<int>(ContractSystem::Instance->GetCurrentContracts<CollectScience>().size()) >= ContractDefsLite::I("Science", "", "MaximumExistent")) return false;
    targetLocation = static_cast<BodyLocation>(UnityRandom::Global().Range(0, 2));
    if (targetLocation == BodyLocation::Space) return GenerateContract_Orbit();
    return GenerateContract_Surface();
}

// Templates/CollectScience.cs:104
bool CollectScience::GenerateContract_Orbit() {
    const auto notSun = [](CelestialBody* cb) { return cb != Sun(); };
    if (Prestige() == ContractPrestige::Trivial) {
        std::vector<CelestialBody*> bodiesProgress = PU::GetBodiesProgressNoReach(ProgressType::ORBIT, true, notSun);
        if (bodiesProgress.empty()) return false;
        targetBody = WeightedBodyChoice(bodiesProgress);
        AddParameter(std::make_unique<Parameters::CollectScience>(targetBody, targetLocation));
        AddKeywords({"Scientific", "Commercial"});
    } else {
        if (Prestige() != ContractPrestige::Significant) return false;
        std::vector<CelestialBody*> bodiesProgress2 = PU::GetBodiesProgress(ProgressType::ORBIT, true, false, notSun);
        if (bodiesProgress2.empty()) return false;
        targetBody = WeightedBodyChoice(bodiesProgress2);
        AddParameter(std::make_unique<Parameters::CollectScience>(targetBody, targetLocation));
        AddKeywords({"Pioneer", "Scientific"});
    }
    const auto r = [](const char* sub, const char* key) { return ContractDefsLite::F("Science", sub, key); };
    const auto m085 = [](float v) { return f32store(fx(v) * 0.85f); };
    SetExpiry();
    SetDeadlineYears(5.0f, targetBody);
    SetFunds(m085(r("Funds", "BaseAdvance")), m085(r("Funds", "BaseReward")), m085(r("Funds", "BaseFailure")), targetBody);
    SetReputation(m085(r("Reputation", "BaseReward")), m085(r("Reputation", "BaseFailure")));
    SetScience(m085(r("Science", "BaseReward")));
    return true;
}

// Templates/CollectScience.cs:140
bool CollectScience::GenerateContract_Surface() {
    const auto notHomeSolid = [](CelestialBody* cb) { return cb != Home() && cb != Sun() && cb->hasSolidSurface; };
    if (Prestige() == ContractPrestige::Trivial) {
        std::vector<CelestialBody*> bodiesProgress = PU::GetBodiesProgressNoReach(ProgressType::LANDING, true, notHomeSolid);
        if (bodiesProgress.empty()) return false;
        targetBody = WeightedBodyChoice(bodiesProgress);
        AddParameter(std::make_unique<Parameters::CollectScience>(targetBody, targetLocation));
        AddKeywords({"Scientific", "Commercial"});
    } else {
        if (Prestige() != ContractPrestige::Significant) return false;
        std::vector<CelestialBody*> bodiesProgress2 = PU::GetBodiesProgress(ProgressType::LANDING, true, false, notHomeSolid);
        if (bodiesProgress2.empty()) return false;
        targetBody = WeightedBodyChoice(bodiesProgress2);
        AddParameter(std::make_unique<Parameters::CollectScience>(targetBody, targetLocation));
        AddKeywords({"Scientific", "Commercial"});
    }
    const auto r = [](const char* sub, const char* key) { return ContractDefsLite::F("Science", sub, key); };
    const auto m115 = [](float v) { return f32store(fx(v) * 1.15f); };
    SetExpiry(ContractDefsLite::I("Science", "Expiration", "MinimumExpireDays"), ContractDefsLite::I("Science", "Expiration", "MaximumExpireDays"));
    SetDeadlineDays(static_cast<float>(ContractDefsLite::I("Science", "Expiration", "DeadlineDays")), targetBody);
    SetFunds(m115(r("Funds", "BaseAdvance")), m115(r("Funds", "BaseReward")), m115(r("Funds", "BaseFailure")), targetBody);
    SetReputation(m115(r("Reputation", "BaseReward")), m115(r("Reputation", "BaseFailure")));
    SetScience(m115(r("Science", "BaseReward")));
    return true;
}

// ---- registration -------------------------------------------------------------------------------------------------------------

}  // namespace ksp::Contracts::Templates

namespace ksp::Contracts {

// C++ helper (see ContractSystem.cs:80)/:107/:149 (the TypeOperation scans, WP-37 types)
void ContractSystem::RegisterStockTypes() {
    using namespace Templates;
    RegisterContractType("PlantFlag", [] { return std::make_unique<PlantFlag>(); });
    RegisterContractType("CollectScience", [] { return std::make_unique<CollectScience>(); });
    RegisterParameterType("EnterOrbit", [] { return std::make_unique<Parameters::EnterOrbit>(); });
    RegisterParameterType("EnterSOI", [] { return std::make_unique<Parameters::EnterSOI>(); });
    RegisterParameterType("LandOnBody", [] { return std::make_unique<Parameters::LandOnBody>(); });
    RegisterParameterType("PlantFlag", [] { return std::make_unique<Parameters::PlantFlag>(); });
    RegisterParameterType("OR", [] { return std::make_unique<Parameters::OR>(); });
    RegisterParameterType("XOR", [] { return std::make_unique<Parameters::XOR>(); });
    RegisterParameterType("KerbalDeaths", [] { return std::make_unique<Parameters::KerbalDeaths>(); });
    RegisterParameterType("CollectScience", [] { return std::make_unique<Parameters::CollectScience>(); });
    RegisterParameterType("ReachSituation", [] { return std::make_unique<Parameters::ReachSituation>(); });
    RegisterParameterType("ReachDestination", [] { return std::make_unique<Parameters::ReachDestination>(); });
    RegisterParameterType("ReachBiome", [] { return std::make_unique<Parameters::ReachBiome>(); });
    RegisterParameterType("ReachAltitudeEnvelope", [] { return std::make_unique<Parameters::ReachAltitudeEnvelope>(); });
    RegisterParameterType("ReachSpeedEnvelope", [] { return std::make_unique<Parameters::ReachSpeedEnvelope>(); });
    RegisterParameterType("ReachFlightEnvelope", [] { return std::make_unique<Parameters::ReachFlightEnvelope>(); });
    RegisterParameterType("RecoverKerbal", [] { return std::make_unique<Parameters::RecoverKerbal>(); });
    RegisterParameterType("RecoverPart", [] { return std::make_unique<Parameters::RecoverPart>(); });
    RegisterParameterType("AcquireCrew", [] { return std::make_unique<Parameters::AcquireCrew>(); });
    RegisterParameterType("AcquirePart", [] { return std::make_unique<Parameters::AcquirePart>(); });
    RegisterParameterType("PartTest", [] { return std::make_unique<Parameters::PartTest>(); });
    RegisterPredicateType("AgentFlag", [](IContractParameterHost* h) { return std::make_unique<Predicates::AgentFlag>(h); });
    RegisterPredicateType("AgentParts", [](IContractParameterHost* h) { return std::make_unique<Predicates::AgentParts>(h); });
    RegisterPredicateType("IsActiveVessel", [](IContractParameterHost* h) { return std::make_unique<Predicates::IsActiveVessel>(h); });
    RegisterPredicateType("IsControllable", [](IContractParameterHost* h) { return std::make_unique<Predicates::IsControllable>(h); });
    RegisterPredicateType("LaunchID", [](IContractParameterHost* h) { return std::make_unique<Predicates::LaunchID>(h); });
}

}  // namespace ksp::Contracts
