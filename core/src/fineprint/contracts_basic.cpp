// WP-38 slice 2: FinePrint/Contracts/{ARMContract,BaseContract,StationContract,ISRUContract}.cs.
#include <algorithm>
#include <cmath>

#include "ksp/career/career.hpp"
#include "ksp/career/game_variables.hpp"
#include "ksp/career/progress.hpp"
#include "ksp/career/upgradeables.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/agents.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/text_gen.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/fineprint/contracts.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/game.hpp"
#include "ksp/unity/mathf.hpp"
#include "param_common.hpp"

namespace ksp::FinePrint::Contracts {

using namespace ksp::fpcommon;
using ksp::Contracts::ContractDefsLite;
using ksp::Contracts::ContractSystem;
using ksp::Contracts::TextGen;
using ksp::Contracts::Parameters::AsteroidParameter;
using ksp::Contracts::Parameters::CrewCapacityParameter;
using ksp::Contracts::Parameters::LocationAndSituationParameter;
using ksp::Contracts::Parameters::ResourceExtractionParameter;
using ksp::Contracts::Parameters::ResourcePossessionParameter;
using ksp::Contracts::Parameters::StabilityParameter;
using ksp::Contracts::Parameters::VesselSystemsParameter;
using Lite::MannedStatus;
using Lite::ProgressType;
namespace PU = Lite::ProgressUtilities;
namespace M = ksp::dotnet::Math;

namespace {
float DF(const char* sec, const char* sub, const char* key) { return ContractDefsLite::F(sec, sub, key); }
int DI(const char* sec, const char* sub, const char* key) { return ContractDefsLite::I(sec, sub, key); }
bool DB(const char* sec, const char* sub, const char* key) { return ContractDefsLite::B(sec, sub, key); }
// float expression `a * b` stored (the Mono rule: evaluated in double, rounded at the store / argument)
float Mul(float a, float b) { return f32store(fx(a) * fx(b)); }
float Round(float f) { return Mathf::Round(f); }
CelestialBody* Home() { return Planetarium::fetch->Home; }
CelestialBody* Sun() { return Planetarium::fetch->Sun; }
bool Contains(const std::vector<CelestialBody*>& l, CelestialBody* b) { return std::find(l.begin(), l.end(), b) != l.end(); }
void RemoveAll(std::vector<CelestialBody*>& l, CelestialBody* b) {
    while (Contains(l, b)) l.erase(std::find(l.begin(), l.end(), b));
}
}  // namespace

// ================================================================================================================================ ARMContract

// ARMContract.cs:19
bool ARMContract::Generate() {
    if (!Lite::ResearchedValidContractObjectives({"Grapple"})) return false;
    if (static_cast<int>(ContractSystem::Instance->GetCurrentContracts<ARMContract>().size()) >= DI("ARM", "", "MaximumExistent")) return false;
    KSPRandom kSPRandom(MissionSeed());
    float num = 1.0f;
    float num2 = 1.0f;
    float num3 = 1.0f;
    auto notSunNotDres = [](CelestialBody* cb) { return cb != Sun() && cb->name() != "Dres"; };
    if (prestige == ContractPrestige::Trivial) {
        asteroidClass = "A";
        targetBody = Home();
    } else if (prestige == ContractPrestige::Significant) {
        asteroidClass = Lite::CoinFlip(&kSPRandom) ? "B" : "C";
        std::vector<CelestialBody*> bodiesProgress = PU::GetBodiesProgressNoReach(ProgressType::ORBIT, true, notSunNotDres);
        if (static_cast<float>(kSPRandom.Next(0, 100)) < DF("ARM", "", "SignificantSolarEjectionChance") && DB("ARM", "", "AllowSolarEjections")) bodiesProgress.push_back(Sun());
        if (bodiesProgress.empty()) bodiesProgress.push_back(Home());
        targetBody = WeightedBodyChoice(bodiesProgress, &kSPRandom);
    } else if (prestige == ContractPrestige::Exceptional) {
        asteroidClass = Lite::CoinFlip(&kSPRandom) ? "D" : "E";
        std::vector<CelestialBody*> bodiesProgress2 = PU::GetBodiesProgressNoReach(ProgressType::ORBIT, true, notSunNotDres);
        for (CelestialBody* b : PU::GetNextUnreached(2, notSunNotDres)) bodiesProgress2.push_back(b);
        if (static_cast<float>(kSPRandom.Next(0, 100)) < DF("ARM", "", "ExceptionalSolarEjectionChance") && DB("ARM", "", "AllowSolarEjections")) bodiesProgress2.push_back(Sun());
        if (bodiesProgress2.empty()) bodiesProgress2.push_back(Home());
        targetBody = WeightedBodyChoice(bodiesProgress2, &kSPRandom);
    }
    if (targetBody == nullptr) targetBody = Home();
    AddParameter(std::make_unique<AsteroidParameter>(asteroidClass, false));
    if (targetBody == Sun()) {
        AddKeywords({"Pioneer"});
        AddParameter(std::make_unique<LocationAndSituationParameter>(targetBody, Vessel::Situations::ESCAPING, Fmt("#autoLOC_7001016")));
        num = Mul(num, DF("ARM", "Funds", "SolarEjectionMultiplier"));
        num2 = Mul(num2, DF("ARM", "Science", "SolarEjectionMultiplier"));
        num3 = Mul(num3, DF("ARM", "Reputation", "SolarEjectionMultiplier"));
    } else if (targetBody == Home() && static_cast<float>(kSPRandom.Next(0, 101)) < DF("ARM", "", "HomeLandingChance") && DB("ARM", "", "AllowHomeLandings")) {
        isLanding = true;
        AddParameter(std::make_unique<LocationAndSituationParameter>(targetBody, Vessel::Situations::LANDED, Fmt("#autoLOC_7001016")));
    } else {
        AddParameter(std::make_unique<LocationAndSituationParameter>(targetBody, Vessel::Situations::ORBITING, Fmt("#autoLOC_7001016")));
    }
    float num4 = 1.0f;
    if (asteroidClass == "E")
        num4 = 2.0f;
    else if (asteroidClass == "D")
        num4 = 1.75f;
    else if (asteroidClass == "C")
        num4 = 1.5f;
    else if (asteroidClass == "B")
        num4 = 1.25f;
    num = Mul(num, num4);
    num2 = Mul(num2, num4);
    num3 = Mul(num3, num4);
    AddKeywords({"Scientific"});
    SetExpiry(DI("ARM", "Expiration", "MinimumExpireDays"), DI("ARM", "Expiration", "MaximumExpireDays"));
    SetDeadlineDays(static_cast<float>(DI("ARM", "Expiration", "DeadlineDays")), targetBody);
    SetFunds(Round(Mul(DF("ARM", "Funds", "BaseAdvance"), num)), Round(Mul(DF("ARM", "Funds", "BaseReward"), num)), Round(Mul(DF("ARM", "Funds", "BaseFailure"), num)), targetBody);
    SetScience(Round(Mul(DF("ARM", "Science", "BaseReward"), num2)));
    SetReputation(Round(Mul(DF("ARM", "Reputation", "BaseReward"), num3)), Round(Mul(DF("ARM", "Reputation", "BaseFailure"), num3)));
    if (ContractSystem::Instance->AnyCurrentContracts<ARMContract>([this](ARMContract* contract) { return contract->targetBody == targetBody && contract->asteroidClass == asteroidClass; }))
        return false;
    AddParameter(std::make_unique<StabilityParameter>(10.0f));
    return true;
}
// ARMContract.cs:181
std::string ARMContract::GetHashString() { return I(Lite::SuperSeed(*this)); }
// ARMContract.cs:186
std::string ARMContract::GetTitle() {
    if (targetBody != Sun()) {
        if (isLanding) return Fmt("#autoLOC_278134", {asteroidClass, targetBody->displayName()});
        return Fmt("#autoLOC_278136", {asteroidClass, targetBody->displayName()});
    }
    return Fmt("#autoLOC_278139", {asteroidClass});
}
// ARMContract.cs:197
std::string ARMContract::GetDescription() {
    return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), targetBody->displayName(), Fmt("#autoLOC_7001015"), MissionSeed(), true, false, true);
}
// ARMContract.cs:202
std::string ARMContract::GetSynopsys() {
    KSPRandom kSPRandom(MissionSeed());
    if (targetBody != Sun()) {
        if (isLanding) return Fmt("#autoLOC_278155", {asteroidClass, targetBody->displayName()});
        switch (kSPRandom.Next(0, 3)) {
            default: return Fmt("#autoLOC_278166", {Home()->displayName(), asteroidClass, targetBody->displayName()});
            case 1: return Fmt("#autoLOC_278164", {asteroidClass, targetBody->displayName()});
            case 0: return Fmt("#autoLOC_278162", {asteroidClass, targetBody->displayName()});
        }
    }
    switch (kSPRandom.Next(0, 3)) {
        default: return Fmt("#autoLOC_278179", {asteroidClass});
        case 1: return Fmt("#autoLOC_278177", {asteroidClass, Home()->displayName()});
        case 0: return Fmt("#autoLOC_278175", {asteroidClass});
    }
}
// ARMContract.cs:238
std::string ARMContract::MessageCompleted() {
    KSPRandom kSPRandom(MissionSeed());
    if (targetBody != Sun()) return Fmt("#autoLOC_278189", {targetBody->displayName()});
    switch (kSPRandom.Next(0, 3)) {
        default: return Fmt("#autoLOC_278199");
        case 1: return Fmt("#autoLOC_278197");
        case 0: return Fmt("#autoLOC_278195");
    }
}
// ARMContract.cs:257
void ARMContract::OnSave(ConfigNode& node) {
    node.AddValue("targetBody", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("asteroidClass", asteroidClass);
    node.AddValue("isLanding", isLanding);
}
// ARMContract.cs:264
void ARMContract::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "ARMContract", "targetBody", targetBody, Home());
    Lite::LoadNode(node, "ARMContract", "asteroidClass", asteroidClass, std::string("A"));
    Lite::LoadNode(node, "ARMContract", "isLanding", isLanding, false);
}
// ARMContract.cs:271
bool ARMContract::MeetRequirements() {
    if (!ProgressTracking::Instance()->NodeComplete({Sun()->name(), "Orbit"})) return false;
    if (!GameVariables::Instance->UnlockedSpaceObjectDiscovery(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::TrackingStation))) return false;
    return true;
}

// ================================================================================================================================ BaseContract

// BaseContract.cs:271
bool BaseContract::AreFacilitiesUnlocked() {
    if (Lite::ResearchedValidContractObjectives({"Antenna", "Dock", "Generator"})) return true;
    return false;
}
// BaseContract.cs:21
bool BaseContract::Generate() {
    if (!AreFacilitiesUnlocked()) return false;
    if (static_cast<int>(ContractSystem::Instance->GetCurrentContracts<BaseContract>().size()) >= DI("Base", "", "MaximumExistent")) return false;
    KSPRandom kSPRandom(MissionSeed());
    float fundsMultiplier = 1.0f;
    float scienceMultiplier = 1.0f;
    float reputationMultiplier = 1.0f;
    auto solid = [](CelestialBody* cb) { return cb->hasSolidSurface && cb != Sun() && cb != Home(); };
    std::vector<CelestialBody*> bodiesProgress = PU::GetBodiesProgressNoReach(ProgressType::LANDING, true, MannedStatus::MANNED, solid);
    if (Prestige() == ContractPrestige::Significant) {
        for (CelestialBody* b : PU::GetNextUnreached(1, MannedStatus::MANNED, solid)) bodiesProgress.push_back(b);
    } else if (Prestige() == ContractPrestige::Exceptional) {
        for (CelestialBody* b : PU::GetNextUnreached(2, MannedStatus::MANNED, solid)) bodiesProgress.push_back(b);
    }
    if (bodiesProgress.empty()) return false;
    targetBody = WeightedBodyChoice(bodiesProgress, &kSPRandom);
    Vessel* vessel = nullptr;
    int num = 5;
    int num2 = 5 + kSPRandom.Next(1, 8);
    int num3 = 7 + kSPRandom.Next(1, 14);
    std::vector<Vessel*> list = Lite::SpecificVesselClassAt(targetBody, VesselType::Base, true, true, false);
    const int num4 = Mathf::RoundToInt(f32store(fx(DF("Base", "", "ContextualChance")) *
                                                (static_cast<double>(M::Min(static_cast<float>(list.size()), DF("Base", "", "ContextualAssets"))) / fx(DF("Base", "", "ContextualAssets")))));
    if (kSPRandom.Next(0, 100) < num4) {
        contextual = true;
        vessel = list[static_cast<size_t>(kSPRandom.Next(static_cast<int>(list.size())))];
        AddParameter(std::make_unique<SpecificVesselParameter>(vessel));
        const int num5 = Lite::ActualCrewCapacity(vessel);
        num += num5;
        num2 += num5;
        num3 += num5;
    }
    if (PU::GetBodyProgress(ProgressType::LANDING, targetBody, MannedStatus::MANNED)) {
        switch (Prestige()) {
            case ContractPrestige::Trivial: capacity = num; break;
            case ContractPrestige::Significant: capacity = num2; break;
            case ContractPrestige::Exceptional: capacity = num3; break;
        }
    } else {
        switch (Prestige()) {
            case ContractPrestige::Trivial: capacity = num; break;
            case ContractPrestige::Significant: capacity = num; break;
            case ContractPrestige::Exceptional: capacity = num2; break;
        }
        AddKeywords({"Pioneer"});
    }
    if (Lite::CelestialUtilities::IsGasGiant(targetBody)) {
        std::vector<CelestialBody*> neighbors =
            Lite::CelestialUtilities::GetNeighbors(targetBody, [](CelestialBody* cb) { return cb != Sun() && !Lite::CelestialUtilities::IsGasGiant(cb); });
        if (neighbors.empty()) return false;
        targetBody = WeightedBodyChoice(neighbors, &kSPRandom);
    }
    if (objectiveTypes.empty()) objectiveTypes = {"Antenna", "Generator", "Dock"};
    AddParameter(std::make_unique<VesselSystemsParameter>(objectiveTypes, std::vector<std::string>{Fmt("#autoLOC_7001075"), Fmt("#autoLOC_7001076"), Fmt("#autoLOC_7001077")},
                                                          Fmt("#autoLOC_7001078"), MannedStatus::ANY, !contextual));
    AddParameter(std::make_unique<CrewCapacityParameter>(capacity));
    float num6 = 0.0f;
    switch (Prestige()) {
        case ContractPrestige::Trivial: num6 = DF("Base", "", "TrivialMobileChance"); break;
        case ContractPrestige::Significant: num6 = DF("Base", "", "SignificantMobileChance"); break;
        case ContractPrestige::Exceptional: num6 = DF("Base", "", "ExceptionalMobileChance"); break;
    }
    ConfigNode* node = ContractDefsLite::Node("Base");
    if (node != nullptr) Lite::SystemUtilities::ProcessSideRequests(*this, *node, targetBody, Fmt("#autoLOC_278368"), fundsMultiplier, scienceMultiplier, reputationMultiplier, vessel);
    if (!contextual && Lite::ResearchedValidContractObjectives({"Wheel"}) && DB("Base", "", "AllowMobile") && static_cast<float>(kSPRandom.Next(0, 100)) < num6) {
        AddKeywords({"Pioneer"});
        AddParameter(std::make_unique<ksp::Contracts::Parameters::MobileBaseParameter>());
        fundsMultiplier = Mul(fundsMultiplier, DF("Base", "Funds", "MobileMultiplier"));
        scienceMultiplier = Mul(scienceMultiplier, DF("Base", "Science", "MobileMultiplier"));
        reputationMultiplier = Mul(reputationMultiplier, DF("Base", "Reputation", "MobileMultiplier"));
    }
    AddParameter(std::make_unique<StabilityParameter>(10.0f));
    AddParameter(std::make_unique<LocationAndSituationParameter>(targetBody, Vessel::Situations::LANDED, Fmt("#autoLOC_278346")));
    const float num7 = static_cast<float>(M::Max(FundsCompletion, static_cast<double>(DF("Base", "Funds", "BaseReward"))));
    const float num8 = M::Max(ScienceCompletion, DF("Base", "Science", "BaseReward"));
    const float num9 = M::Max(ReputationCompletion, DF("Base", "Reputation", "BaseReward"));
    SetExpiry(DI("Base", "Expiration", "MinimumExpireDays"), DI("Base", "Expiration", "MaximumExpireDays"));
    SetDeadlineDays(static_cast<float>(DI("Base", "Expiration", "DeadlineDays")), targetBody);
    SetFunds(Round(Mul(DF("Base", "Funds", "BaseAdvance"), fundsMultiplier)), Round(Mul(num7, fundsMultiplier)), Round(Mul(DF("Base", "Funds", "BaseFailure"), fundsMultiplier)), targetBody);
    SetScience(Round(Mul(num8, scienceMultiplier)));
    SetReputation(Round(Mul(num9, reputationMultiplier)), Round(Mul(DF("Base", "Reputation", "BaseFailure"), reputationMultiplier)));
    if (ContractSystem::Instance->AnyCurrentContracts<BaseContract>([this](BaseContract* contract) { return contract->targetBody == targetBody; })) return false;
    if (ContractSystem::Instance->AnyCurrentContracts<StationContract>([this](StationContract* contract) { return contract->targetBody == targetBody; })) return false;
    return true;
}
// BaseContract.cs:188
std::string BaseContract::GetHashString() { return I(Lite::SuperSeed(*this)); }
// BaseContract.cs:193
std::string BaseContract::GetTitle() {
    if (contextual) return Fmt("#autoLOC_7001079", {Lite::StringUtilities::SpecificVesselName(*this), targetBody->displayName()});
    return Fmt("#autoLOC_7001080", {targetBody->displayName()});
}
// BaseContract.cs:202
std::string BaseContract::GetDescription() {
    const std::string text = Lite::StringUtilities::IntegerToGreek(KSPRandom(MissionSeed()).Next(23));
    const std::string topic = contextual ? Lite::StringUtilities::SpecificVesselName(*this) : (targetBody->displayName() + " Base " + text);
    return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), topic, targetBody->displayName() + "Srf", MissionSeed(), true, false, true);
}
// BaseContract.cs:209
std::string BaseContract::GetSynopsys() {
    if (contextual) return Fmt("#autoLOC_278436", {targetBody->displayName(), Lite::StringUtilities::SpecificVesselName(*this)});
    return Fmt("#autoLOC_278438", {I(capacity), targetBody->displayName()});
}
// BaseContract.cs:219
std::string BaseContract::MessageCompleted() {
    if (contextual) return Fmt("#autoLOC_278444", {Lite::StringUtilities::SpecificVesselName(*this)});
    return Fmt("#autoLOC_278446", {targetBody->displayName()});
}
// BaseContract.cs:229
void BaseContract::OnSave(ConfigNode& node) {
    node.AddValue("targetBody", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("capacity", static_cast<int32_t>(capacity));
    node.AddValue("contextual", contextual);
}
// BaseContract.cs:236
void BaseContract::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "BaseContract", "targetBody", targetBody, Home());
    Lite::LoadNode(node, "BaseContract", "capacity", capacity, 8);
    Lite::LoadNode(node, "BaseContract", "contextual", contextual, false);
}
// BaseContract.cs:243
bool BaseContract::MeetRequirements() {
    if (!ProgressTracking::Instance()->NodeComplete({Home()->name(), Fmt("#autoLOC_278465")})) return false;
    return true;
}
// BaseContract.cs:256
void BaseContract::OnAccepted() {
    auto* parameter = GetParameterT<VesselSystemsParameter>();
    if (parameter != nullptr && parameter->requireNew) parameter->launchID = SaveEnvironment::Current().currentGame->launchID;
}
// BaseContract.cs:266
void BaseContract::OnFinished() {
    if (auto* p = GetParameterT<SpecificVesselParameter>()) p->CleanupWaypoints();
}

// ================================================================================================================================ StationContract

// StationContract.cs:287
bool StationContract::AreFacilitiesUnlocked() {
    if (Lite::ResearchedValidContractObjectives({"Antenna", "Dock", "Generator"})) return true;
    return false;
}
// StationContract.cs:296
bool StationContract::AreAsteroidsUnlocked() {
    if (!DB("Station", "", "AllowAsteroid")) return false;
    if (!GameVariables::Instance->UnlockedSpaceObjectDiscovery(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::TrackingStation))) return false;
    if (!ProgressTracking::Instance()->NodeComplete({Sun()->name(), "Orbit"})) return false;
    if (!Lite::ResearchedValidContractObjectives({"Grapple"})) return false;
    return true;
}
// StationContract.cs:21
bool StationContract::Generate() {
    if (!AreFacilitiesUnlocked()) return false;
    if (static_cast<int>(ContractSystem::Instance->GetCurrentContracts<StationContract>().size()) >= DI("Station", "", "MaximumExistent")) return false;
    KSPRandom kSPRandom(MissionSeed());
    float fundsMultiplier = 1.0f;
    float scienceMultiplier = 1.0f;
    float reputationMultiplier = 1.0f;
    std::vector<CelestialBody*> bodiesProgress = PU::GetBodiesProgressNoReach(ProgressType::ORBIT, true, MannedStatus::MANNED);
    if (Prestige() == ContractPrestige::Exceptional) {
        for (CelestialBody* b : PU::GetNextUnreached(2, MannedStatus::MANNED)) bodiesProgress.push_back(b);
        RemoveAll(bodiesProgress, Home());
    } else if (Prestige() == ContractPrestige::Significant) {
        for (CelestialBody* b : PU::GetNextUnreached(1, MannedStatus::MANNED)) bodiesProgress.push_back(b);
    }
    if (!DB("Station", "", "AllowSolar")) RemoveAll(bodiesProgress, Sun());
    if (bodiesProgress.empty()) return false;
    targetBody = WeightedBodyChoice(bodiesProgress, &kSPRandom);
    Vessel* vessel = nullptr;
    int num = 5;
    int num2 = 5 + kSPRandom.Next(1, 8);
    int num3 = 7 + kSPRandom.Next(1, 14);
    std::vector<Vessel*> list = Lite::SpecificVesselClassAt(targetBody, VesselType::Station, true, true, false);
    const int num4 = Mathf::RoundToInt(f32store(fx(DF("Station", "", "ContextualChance")) *
                                                (static_cast<double>(M::Min(static_cast<float>(list.size()), DF("Station", "", "ContextualAssets"))) / fx(DF("Station", "", "ContextualAssets")))));
    if (kSPRandom.Next(0, 100) < num4) {
        contextual = true;
        vessel = list[static_cast<size_t>(kSPRandom.Next(static_cast<int>(list.size())))];
        AddParameter(std::make_unique<SpecificVesselParameter>(vessel));
        const int num5 = Lite::ActualCrewCapacity(vessel);
        num += num5;
        num2 += num5;
        num3 += num5;
    }
    if (PU::GetBodyProgress(ProgressType::ORBIT, targetBody, MannedStatus::MANNED)) {
        switch (Prestige()) {
            case ContractPrestige::Trivial: capacity = num; break;
            case ContractPrestige::Significant: capacity = num2; break;
            case ContractPrestige::Exceptional: capacity = num3; break;
        }
    } else {
        switch (Prestige()) {
            case ContractPrestige::Trivial: capacity = num; break;
            case ContractPrestige::Significant: capacity = num; break;
            case ContractPrestige::Exceptional: capacity = num2; break;
        }
        AddKeywords({"Pioneer"});
    }
    if (objectiveTypes.empty()) objectiveTypes = {"Antenna", "Generator", "Dock"};
    const std::string text = Fmt("#autoLOC_7001017");
    AddParameter(std::make_unique<VesselSystemsParameter>(objectiveTypes, std::vector<std::string>{Fmt("#autoLOC_7001075"), Fmt("#autoLOC_7001076"), Fmt("#autoLOC_7001077")},
                                                          text, MannedStatus::ANY, !contextual));
    AddParameter(std::make_unique<CrewCapacityParameter>(capacity));
    float num6 = 0.0f;
    std::string size;
    switch (Prestige()) {
        case ContractPrestige::Trivial:
            num6 = DF("Station", "", "TrivialAsteroidChance");
            size = Lite::CoinFlip(&kSPRandom) ? "A" : "B";
            break;
        case ContractPrestige::Significant:
            num6 = DF("Station", "", "SignificantAsteroidChance");
            size = Lite::CoinFlip(&kSPRandom) ? "B" : "C";
            break;
        case ContractPrestige::Exceptional:
            num6 = DF("Station", "", "ExceptionalAsteroidChance");
            size = Lite::CoinFlip(&kSPRandom) ? "D" : "E";
            break;
    }
    ConfigNode* node = ContractDefsLite::Node("Station");
    if (node != nullptr) Lite::SystemUtilities::ProcessSideRequests(*this, *node, targetBody, text, fundsMultiplier, scienceMultiplier, reputationMultiplier, vessel);
    if (AreAsteroidsUnlocked()) {
        if (contextual && vessel != nullptr && (targetBody->name() != "Dres" || Lite::VesselHasPartName("PotatoRoid", vessel))) num6 = 0.0f;
        if (static_cast<float>(kSPRandom.Next(0, 100)) < num6) {
            AddKeywords({"Pioneer"});
            AddParameter(std::make_unique<AsteroidParameter>(size, true));
            fundsMultiplier = Mul(fundsMultiplier, DF("Station", "Funds", "AsteroidMultiplier"));
            scienceMultiplier = Mul(scienceMultiplier, DF("Station", "Science", "AsteroidMultiplier"));
            reputationMultiplier = Mul(reputationMultiplier, DF("Station", "Reputation", "AsteroidMultiplier"));
        }
    }
    AddParameter(std::make_unique<StabilityParameter>(10.0f));
    AddParameter(std::make_unique<LocationAndSituationParameter>(targetBody, Vessel::Situations::ORBITING, text));
    const float num7 = static_cast<float>(M::Max(FundsCompletion, static_cast<double>(DF("Station", "Funds", "BaseReward"))));
    const float num8 = M::Max(ScienceCompletion, DF("Station", "Science", "BaseReward"));
    const float num9 = M::Max(ReputationCompletion, DF("Station", "Reputation", "BaseReward"));
    SetExpiry(DI("Station", "Expiration", "MinimumExpireDays"), DI("Station", "Expiration", "MaximumExpireDays"));
    SetDeadlineDays(static_cast<float>(DI("Station", "Expiration", "DeadlineDays")), targetBody);
    SetFunds(Round(Mul(DF("Station", "Funds", "BaseAdvance"), fundsMultiplier)), Round(Mul(num7, fundsMultiplier)), Round(Mul(DF("Station", "Funds", "BaseFailure"), fundsMultiplier)), targetBody);
    SetScience(Round(Mul(num8, scienceMultiplier)));
    SetReputation(Round(Mul(num9, reputationMultiplier)), Round(Mul(DF("Station", "Reputation", "BaseFailure"), reputationMultiplier)));
    if (ContractSystem::Instance->AnyCurrentContracts<StationContract>([this](StationContract* contract) { return contract->targetBody == targetBody; })) return false;
    if (ContractSystem::Instance->AnyCurrentContracts<BaseContract>([this](BaseContract* contract) { return contract->targetBody == targetBody; })) return false;
    return true;
}
// StationContract.cs:203
std::string StationContract::GetHashString() { return I(Lite::SuperSeed(*this)); }
// StationContract.cs:208
std::string StationContract::GetTitle() {
    const std::string text = !contextual ? Fmt("#autoLOC_7001020") : Fmt("#autoLOC_7001019", {Lite::StringUtilities::SpecificVesselName(*this)});
    const std::string text2 = !(targetBody == Sun()) ? Fmt("#autoLOC_7001022", {targetBody->displayName()}) : Fmt("#autoLOC_7001021");
    return Fmt("#autoLOC_6002375", {text, text2});
}
// StationContract.cs:215
std::string StationContract::GetDescription() {
    const std::string text = Lite::StringUtilities::IntegerToGreek(KSPRandom(MissionSeed()).Next(23));
    const std::string topic = contextual ? Lite::StringUtilities::SpecificVesselName(*this) : (targetBody->displayName() + " Station " + text);
    return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), topic, targetBody->displayName(), MissionSeed(), true, true, true);
}
// StationContract.cs:221
std::string StationContract::GetSynopsys() {
    if (contextual) return Fmt("#autoLOC_280652", {targetBody->displayName(), Lite::StringUtilities::SpecificVesselName(*this)});
    if (targetBody == Sun()) return Fmt("#autoLOC_7001071", {I(capacity)});
    return Fmt("#autoLOC_7001072", {I(capacity), targetBody->displayName()});
}
// StationContract.cs:233
std::string StationContract::MessageCompleted() {
    if (contextual) return Fmt("#autoLOC_280660", {Lite::StringUtilities::SpecificVesselName(*this)});
    if (targetBody == Sun()) return Fmt("#autoLOC_7001073");
    return Fmt("#autoLOC_7001074", {targetBody->displayName()});
}
// StationContract.cs:246
void StationContract::OnSave(ConfigNode& node) {
    node.AddValue("targetBody", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("capacity", static_cast<int32_t>(capacity));
    node.AddValue("contextual", contextual);
}
// StationContract.cs:253
void StationContract::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "StationContract", "targetBody", targetBody, Home());
    Lite::LoadNode(node, "StationContract", "capacity", capacity, 8);
    Lite::LoadNode(node, "StationContract", "contextual", contextual, false);
}
// StationContract.cs:260
bool StationContract::MeetRequirements() {
    if (!ProgressTracking::Instance()->NodeComplete({Home()->name(), "Orbit"})) return false;
    return true;
}
// StationContract.cs:274
void StationContract::OnAccepted() {
    auto* parameter = GetParameterT<VesselSystemsParameter>();
    if (parameter != nullptr && parameter->requireNew) parameter->launchID = SaveEnvironment::Current().currentGame->launchID;
}
// StationContract.cs:282
void StationContract::OnFinished() {
    if (auto* p = GetParameterT<SpecificVesselParameter>()) p->CleanupWaypoints();
}

// ================================================================================================================================ ISRUContract

// ISRUContract.cs:296
std::vector<ConfigNode*> ISRUContract::PossibleResources() {
    std::vector<ConfigNode*> list;
    ConfigNode* node = ContractDefsLite::Node("ISRU");
    if (node == nullptr) return list;
    std::vector<ConfigNode*> nodes = node->GetNodes("RESOURCE_REQUEST");
    for (ConfigNode* configNode : nodes) {
        const std::vector<std::string> modules = configNode->GetValuesList("Module");
        if (Lite::ProgressUtilities::HaveAnyTech(nullptr, &modules, false)) list.push_back(configNode);
    }
    return list;
}
// ISRUContract.cs:316
bool ISRUContract::CelestialIsForbidden(CelestialBody* body, ConfigNode* resourceRequest) {
    if (body != nullptr && resourceRequest != nullptr) {
        const std::vector<std::string> values = resourceRequest->GetValuesList("Forbidden");
        int num = static_cast<int>(values.size());
        do {
            if (num-- <= 0) return false;
        } while (!(body->GetName() == values[static_cast<size_t>(num)]));
        return true;
    }
    return true;
}
// ISRUContract.cs:27
bool ISRUContract::Generate() {
    if (static_cast<int>(ContractSystem::Instance->GetCurrentContracts<ISRUContract>().size()) >= DI("ISRU", "", "MaximumExistent")) return false;
    std::vector<ConfigNode*> list = PossibleResources();
    if (list.empty()) return false;
    KSPRandom kSPRandom(MissionSeed());
    ConfigNode* configNode = list[static_cast<size_t>(kSPRandom.Next(0, static_cast<int>(list.size())))];
    targetResource = "Matter";
    resourceTitle = "matter";
    Lite::LoadNode(configNode, "ISRUContract", "Name", targetResource, std::string("Matter"), false);
    std::string defaultValue = targetResource;
    if (PartResourceLibrary::Instance() != nullptr && PartResourceLibrary::Instance()->resourceDefinitions.Contains(targetResource))
        defaultValue = PartResourceLibrary::Instance()->GetDefinition(targetResource)->displayName();
    Lite::LoadNode(configNode, "ISRUContract", "Title", resourceTitle, defaultValue, false);
    auto notHome = [](CelestialBody* cb) { return cb != Home(); };
    std::vector<CelestialBody*> bodiesProgress = PU::GetBodiesReached(true, notHome);
    if (Prestige() == ContractPrestige::Significant) {
        for (CelestialBody* b : PU::GetNextUnreached(1, notHome)) bodiesProgress.push_back(b);
    } else if (Prestige() == ContractPrestige::Exceptional) {
        for (CelestialBody* b : PU::GetNextUnreached(2, notHome)) bodiesProgress.push_back(b);
    }
    if (bodiesProgress.empty()) return false;
    for (;;) {
        targetBody = WeightedBodyChoice(bodiesProgress, &kSPRandom);
        if (!CelestialIsForbidden(targetBody, configNode)) break;
        bodiesProgress.erase(std::find(bodiesProgress.begin(), bodiesProgress.end(), targetBody));
        if (bodiesProgress.empty()) return false;
    }
    float value = 1.0f, value2 = 1.0f, value3 = 1.0f;
    int value4 = 0;
    switch (prestige) {
        case ContractPrestige::Trivial: {
            gatherGoal = 0.0f;
            ConfigNode* node3 = configNode->GetNode("Trivial");
            Lite::LoadNode(node3, "ISRUContract", "Amount", gatherGoal, 500.0f, false);
            Lite::LoadNode(node3, "ISRUContract", "DeliveryChance", value4, 50, false);
            Lite::LoadNode(node3, "ISRUContract", "FundsMultiplier", value, 1.0f, false);
            Lite::LoadNode(node3, "ISRUContract", "ScienceMultiplier", value2, 1.0f, false);
            Lite::LoadNode(node3, "ISRUContract", "ReputationMultiplier", value3, 1.0f, false);
            break;
        }
        case ContractPrestige::Significant: {
            gatherGoal = 0.0f;
            ConfigNode* node2 = configNode->GetNode("Significant");
            Lite::LoadNode(node2, "ISRUContract", "Amount", gatherGoal, 1000.0f, false);
            Lite::LoadNode(node2, "ISRUContract", "DeliveryChance", value4, 65, false);
            Lite::LoadNode(node2, "ISRUContract", "FundsMultiplier", value, 1.1f, false);
            Lite::LoadNode(node2, "ISRUContract", "ScienceMultiplier", value2, 1.1f, false);
            Lite::LoadNode(node2, "ISRUContract", "ReputationMultiplier", value3, 1.1f, false);
            break;
        }
        case ContractPrestige::Exceptional: {
            gatherGoal = 0.0f;
            ConfigNode* node = configNode->GetNode("Exceptional");
            Lite::LoadNode(node, "ISRUContract", "Amount", gatherGoal, 2500.0f, false);
            Lite::LoadNode(node, "ISRUContract", "DeliveryChance", value4, 80, false);
            Lite::LoadNode(node, "ISRUContract", "FundsMultiplier", value, 1.3f, false);
            Lite::LoadNode(node, "ISRUContract", "ScienceMultiplier", value2, 1.3f, false);
            Lite::LoadNode(node, "ISRUContract", "ReputationMultiplier", value3, 1.3f, false);
            break;
        }
    }
    if (kSPRandom.Next(0, 100) < value4) {
        deliveryBody = WeightedBodyChoice(Lite::CelestialUtilities::GetNeighbors(targetBody, [](CelestialBody* cb) { return cb != Sun(); }), &kSPRandom);
        isDelivering = true;
    }
    if (deliveryBody == nullptr) isDelivering = false;
    if (isDelivering) {
        float value5 = 1.8f;
        Lite::LoadNode(configNode, "ISRUContract", "DeliveryMultiplier", value5, 1.8f, false);
        value = Mul(value, value5);
        value2 = Mul(value2, value5);
        value3 = Mul(value3, value5);
    }
    float num = f32store(static_cast<double>(static_cast<float>(kSPRandom.NextDouble())) * 0.25);
    num = (kSPRandom.Next(0, 100) <= 50) ? f32store(1.0 - fx(num)) : f32store(1.0 + fx(num));
    value = Mul(value, num);
    value2 = Mul(value2, num);
    value3 = Mul(value3, num);
    gatherGoal = Mul(gatherGoal, num);
    gatherGoal = f32store(fx(Mathf::Round(f32store(fx(gatherGoal) / 50.0))) * 50.0);
    AddParameter(std::make_unique<ResourceExtractionParameter>(targetResource, resourceTitle, static_cast<double>(gatherGoal), targetBody, configNode->GetValuesList("Module")));
    if (isDelivering) {
        deliverySituation = Lite::CelestialUtilities::ApplicableSituation(MissionSeed(), deliveryBody, false);
        AddParameter(std::make_unique<ResourcePossessionParameter>(PartResourceLibrary::Instance()->GetDefinition(targetResource)->name(), resourceTitle, Fmt("#autoLOC_7001016"),
                                                                   static_cast<double>(gatherGoal)));
        AddParameter(std::make_unique<LocationAndSituationParameter>(deliveryBody, deliverySituation, PartResourceLibrary::Instance()->GetDefinition(targetResource)->displayName()));
        AddParameter(std::make_unique<StabilityParameter>(10.0f));
    }
    std::string value6 = "ISRU";
    Lite::LoadNode(configNode, "ISRUContract", "Keyword", value6, std::string("ISRU"), false);
    AddKeywords({value6});
    SetExpiry(DI("ISRU", "Expiration", "MinimumExpireDays"), DI("ISRU", "Expiration", "MaximumExpireDays"));
    SetDeadlineDays(static_cast<float>(DI("ISRU", "Expiration", "DeadlineDays")), targetBody);
    SetFunds(Round(Mul(DF("ISRU", "Funds", "BaseAdvance"), value)), Round(Mul(DF("ISRU", "Funds", "BaseReward"), value)), Round(Mul(DF("ISRU", "Funds", "BaseFailure"), value)), targetBody);
    SetScience(Round(Mul(DF("ISRU", "Science", "BaseReward"), value2)));
    SetReputation(Round(Mul(DF("ISRU", "Reputation", "BaseReward"), value3)), Round(Mul(DF("ISRU", "Reputation", "BaseFailure"), value3)));
    if (ContractSystem::Instance->AnyCurrentContracts<ISRUContract>([this](ISRUContract* contract) { return contract->targetBody == targetBody; })) return false;
    return true;
}
// ISRUContract.cs:205
std::string ISRUContract::GetHashString() { return I(Lite::SuperSeed(*this)); }
// ISRUContract.cs:210
std::string ISRUContract::GetTitle() {
    if (isDelivering) return Fmt("#autoLOC_279475", {resourceTitle, targetBody->displayName(), deliveryBody->displayName()});
    return Fmt("#autoLOC_279477", {RoundedThousands(static_cast<double>(Mathf::Round(gatherGoal))), resourceTitle, targetBody->displayName()});
}
namespace {
std::string Lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
}  // namespace
// ISRUContract.cs:219
std::string ISRUContract::GetDescription() {
    return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), Lower(Fmt(resourceTitle.c_str())), targetBody->displayName() + "Srf", MissionSeed(), true, false, true);
}
// ISRUContract.cs:224
std::string ISRUContract::GetSynopsys() {
    const std::string goal = RoundedThousands(static_cast<double>(Mathf::Round(gatherGoal)));
    if (isDelivering) return Fmt("#autoLOC_279488", {resourceTitle, targetBody->displayName(), goal, deliveryBody->displayName()});
    return Fmt("#autoLOC_279490", {resourceTitle, targetBody->displayName(), goal});
}
// ISRUContract.cs:234
std::string ISRUContract::MessageCompleted() {
    const std::string goal = RoundedThousands(static_cast<double>(Mathf::Round(gatherGoal)));
    if (isDelivering) return Fmt("#autoLOC_279496", {goal, resourceTitle, deliveryBody->displayName(), Agent()->Title()});
    return Fmt("#autoLOC_279498", {goal, resourceTitle, Agent()->Title()});
}
// ISRUContract.cs:244
void ISRUContract::OnSave(ConfigNode& node) {
    node.AddValue("targetBody", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("gatherGoal", gatherGoal);
    node.AddValue("targetResource", targetResource);
    node.AddValue("resourceTitle", resourceTitle);
    node.AddValue("isDelivering", isDelivering);
    if (isDelivering) {
        node.AddValue("deliveryBody", static_cast<int32_t>(deliveryBody->flightGlobalsIndex));
        node.AddValue("deliverySituation", dotnet::EnumToString(deliverySituation));
    }
}
// ISRUContract.cs:258
void ISRUContract::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "ISRUContract", "targetBody", targetBody, Home());
    Lite::LoadNode(node, "ISRUContract", "gatherGoal", gatherGoal, 1.0f);
    Lite::LoadNode(node, "ISRUContract", "targetResource", targetResource, std::string("Matter"));
    Lite::LoadNode(node, "ISRUContract", "resourceTitle", resourceTitle, std::string("matter"));
    Lite::LoadNode(node, "ISRUContract", "isDelivering", isDelivering, false);
    if (isDelivering) {
        Lite::LoadNode(node, "ISRUContract", "deliveryBody", deliveryBody, Home());
        Lite::LoadNode(node, "ISRUContract", "deliverySituation", deliverySituation, Vessel::Situations::ORBITING);
    }
    auto* resourcePossessionParameter = static_cast<ResourcePossessionParameter*>(GetParameterOfType("ResourcePossessionParameter"));
    if (resourcePossessionParameter != nullptr && !PartResourceLibrary::Instance()->resourceDefinitions.Contains(resourcePossessionParameter->resourceName))
        resourcePossessionParameter->resourceName = targetResource;
}
// ISRUContract.cs:277
bool ISRUContract::MeetRequirements() {
    if (!ProgressTracking::Instance()->NodeComplete({Home()->name(), "Orbit"})) return false;
    return true;
}
// ISRUContract.cs:286
std::vector<CelestialBody*> ISRUContract::WeightBodies() {
    std::vector<CelestialBody*> list{targetBody};
    if (isDelivering) list.push_back(deliveryBody);
    return list;
}

}  // namespace ksp::FinePrint::Contracts
