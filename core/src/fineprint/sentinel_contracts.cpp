// WP-38 slice 3: SentinelMission/{SentinelParameter,SentinelContract,CometDetectionContract}.cs. See include/ksp/fineprint/sentinel.hpp.
#include <algorithm>
#include <cmath>

#include "ksp/career/game_variables.hpp"
#include "ksp/career/progress.hpp"
#include "ksp/career/upgradeables.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/agents.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/parameters.hpp"
#include "ksp/contracts/text_gen.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/fineprint/contract_defs.hpp"
#include "ksp/fineprint/sentinel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "param_common.hpp"

namespace ksp::SentinelMission {

using namespace ksp::fpcommon;
using ksp::Contracts::ContractSystem;
using ksp::Contracts::TextGen;
using ksp::Contracts::Parameters::PartRequestParameter;
using ksp::Contracts::Parameters::SpecificOrbitParameter;
using ksp::Contracts::Parameters::CometDetectionParameter;
using ksp::Contracts::ParameterState;
using ksp::Contracts::Parameters::EnterOrbit;
using FinePrint::ContractDefs;
namespace L = ksp::Contracts::Lite;
namespace M = ksp::dotnet::Math;

namespace {
CelestialBody* Home() { return Planetarium::fetch->Home; }
CelestialBody* Sun() { return Planetarium::fetch->Sun; }
Orbit& OrbitOfBody(CelestialBody* b) {
    Orbit* o = BodyOrbit(b);
    if (o == nullptr) throw NullReferenceException("body " + b->bodyName + " has no orbit");
    return *o;
}
float Mul(float a, float b) { return f32store(fx(a) * fx(b)); }
int RoundI(float f) { return Mathf::RoundToInt(f); }
}  // namespace

// ================================================================================================================================ SentinelParameter

// SentinelParameter.cs:27
SentinelParameter::SentinelParameter() {
    FocusBody = Home();
    ScanType = SentinelScanType::NONE;
    if (SentinelEnvironment::discoverableObjects.has_value())
        TargetSize = SentinelEnvironment::discoverableObjects->maxAsteroidClass;
    else
        TargetSize = static_cast<UntrackedObjectClass>(9 - 1);  // Enum.GetNames(typeof(UntrackedObjectClass)).Length - 1
    MinimumEccentricity = 0.0;
    MinimumInclination = 0.0;
    TotalDiscoveries = 3;
    RemainingDiscoveries = 3;
}

// SentinelParameter.cs:42
SentinelParameter::SentinelParameter(CelestialBody* focusBody, SentinelScanType scanType, UntrackedObjectClass targetSize, double minimumEccentricity,
                                     double minimumInclination, int discoveryCount) {
    FocusBody = focusBody;
    ScanType = scanType;
    TargetSize = targetSize;
    MinimumEccentricity = minimumEccentricity;
    MinimumInclination = minimumInclination;
    TotalDiscoveries = discoveryCount;
    RemainingDiscoveries = discoveryCount;
}

// SentinelParameter.cs:53
std::string SentinelParameter::GetHashString() { return I(L::SuperSeed(*Root())) + ID(); }

// SentinelParameter.cs:58
std::string SentinelParameter::GetTitle() {
    const std::string text = (FocusBody == Home()) ? Fmt("#autoLOC_6002303") : Fmt("#autoLOC_6002304");
    switch (ScanType) {
        default: return Fmt("#autoLOC_6002308", {I(TotalDiscoveries), text, FocusBody->displayName()});
        case SentinelScanType::CLASS: return Fmt("#autoLOC_6002305", {I(TotalDiscoveries), dotnet::EnumToString(TargetSize), text, FocusBody->displayName()});
        case SentinelScanType::ECCENTRICITY:
            return Fmt("#autoLOC_6002306", {I(TotalDiscoveries), text, FocusBody->displayName(), dotnet::FormatDoubleG(M::Round(MinimumEccentricity, 2, dotnet::MidpointRounding::ToEven))});
        case SentinelScanType::INCLINATION:
            return Fmt("#autoLOC_6002307", {I(TotalDiscoveries), text, FocusBody->displayName(), dotnet::FormatDoubleG(M::Round(MinimumInclination))});
    }
}

// SentinelParameter.cs:75
std::string SentinelParameter::GetNotes() { return std::string(Scene() == GameScenes::SPACECENTER ? "\n" : "") + Fmt("#autoLOC_6002309"); }

// SentinelParameter.cs:80
void SentinelParameter::OnRegister() { SetDisableOnStateChange(true); }

// SentinelParameter.cs:85
void SentinelParameter::OnSave(ConfigNode& node) {
    node.AddValue("FocusBody", static_cast<int32_t>(FocusBody->flightGlobalsIndex));
    node.AddValue("ScanType", dotnet::EnumToString(ScanType));
    node.AddValue("TargetSize", dotnet::EnumToString(TargetSize));
    node.AddValue("MinimumEccentricity", MinimumEccentricity);
    node.AddValue("MinimumInclination", MinimumInclination);
    node.AddValue("TotalDiscoveries", static_cast<int32_t>(TotalDiscoveries));
    node.AddValue("RemainingDiscoveries", static_cast<int32_t>(RemainingDiscoveries));
}

// SentinelParameter.cs:96
void SentinelParameter::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "SentinelParameter", "FocusBody", FocusBody, Home());
    L::LoadNode(node, "SentinelParameter", "ScanType", ScanType, SentinelScanType::NONE);
    L::LoadNode(node, "SentinelParameter", "TargetSize", TargetSize, static_cast<UntrackedObjectClass>(9 - 1));
    L::LoadNode(node, "SentinelParameter", "MinimumEccentricity", MinimumEccentricity, 0.0);
    L::LoadNode(node, "SentinelParameter", "MinimumInclination", MinimumInclination, 0.0);
    L::LoadNode(node, "SentinelParameter", "TotalDiscoveries", TotalDiscoveries, 3);
    L::LoadNode(node, "SentinelParameter", "RemainingDiscoveries", RemainingDiscoveries, 3);
}

// SentinelParameter.cs:107
void SentinelParameter::DiscoverAsteroid(UntrackedObjectClass size, double eccentricity, double inclination, CelestialBody* body) {
    if (Root()->ContractState() != Contract::State::Active || body != FocusBody || (ScanType == SentinelScanType::CLASS && size != TargetSize) ||
        (ScanType == SentinelScanType::ECCENTRICITY && eccentricity < MinimumEccentricity) ||
        (ScanType == SentinelScanType::INCLINATION && inclination < MinimumInclination))
        return;
    auto* parameter = Root()->GetParameterT<SpecificOrbitParameter>();
    if (parameter != nullptr && parameter->State() != ParameterState::Complete && parameter->Orbit_() != nullptr) {
        Orbit* orbit = parameter->Orbit_();
        if (!SentinelScenario::Instance->ActiveSentinelMatchingOrbit(orbit, parameter->deviationWindow)) return;
    }
    RemainingDiscoveries--;
    if (RemainingDiscoveries > 0) {
        const std::string text = Fmt("#autoLOC_6002310", {I(TotalDiscoveries - RemainingDiscoveries), I(TotalDiscoveries), body->displayName(), Root()->Agent()->Title()});
        L::FinePrintEnvironment::Post(text, SentinelUtilities::CalculateReadDuration(text), L::ScreenMessageStyle::UPPER_CENTER);
    } else {
        const std::string text2 = Fmt("#autoLOC_6002311", {FocusBody->displayName(), Root()->Agent()->Title()});
        L::FinePrintEnvironment::Post(text2, SentinelUtilities::CalculateReadDuration(text2), L::ScreenMessageStyle::UPPER_CENTER);
        SetComplete();
        Root()->Complete();
    }
}

// ================================================================================================================================ SentinelContract

// SentinelContract.cs:23
bool SentinelContract::Generate() {
    if (!L::ProgressUtilities::HavePartTech(SentinelUtilities::SentinelPartName, false)) return false;
    int num = 0;
    int num2 = 0;
    std::vector<SentinelContract*> currentContracts = ContractSystem::Instance->GetCurrentContracts<SentinelContract>();
    int num3 = static_cast<int>(currentContracts.size());
    while (num3-- > 0) {
        switch (currentContracts[static_cast<size_t>(num3)]->ContractState()) {
            case State::Active: num2++; break;
            case State::Offered: num++; break;
            default: break;
        }
    }
    if (num < ContractDefs::Sentinel::MaximumAvailable && num2 < ContractDefs::Sentinel::MaximumActive) {
        const int seed = L::SuperSeed(*this);
        generator = std::make_unique<KSPRandom>(seed);
        float num4 = 1.0f;
        SentinelScanType sentinelScanType = SentinelScanType::NONE;
        UntrackedObjectClass untrackedObjectClass = SentinelEnvironment::discoverableObjects.has_value() ? SentinelEnvironment::discoverableObjects->maxAsteroidClass
                                                                                                          : static_cast<UntrackedObjectClass>(9 - 1);
        double num5 = 0.0;
        double num6 = 0.0;
        asteroidCount = generator->Next(ContractDefs::Sentinel::Trivial::MinAsteroids, ContractDefs::Sentinel::Trivial::MaxAsteroids);
        switch (Prestige()) {
            default: SetFocusBody(Home()); break;
            case ContractPrestige::Exceptional: {
                std::vector<CelestialBody*> list;
                for (CelestialBody* cb : GetBodies_Reached(true, false))
                    if (cb->referenceBody() == Sun()) list.push_back(cb);
                if (list.empty()) return false;
                SetFocusBody(list[static_cast<size_t>(generator->Next(0, static_cast<int>(list.size())))]);
                asteroidCount = generator->Next(ContractDefs::Sentinel::Exceptional::MinAsteroids, ContractDefs::Sentinel::Exceptional::MaxAsteroids);
                sentinelScanType = SentinelUtilities::RandomScanType(generator.get());
                break;
            }
            case ContractPrestige::Significant:
                SetFocusBody(Home());
                asteroidCount = generator->Next(ContractDefs::Sentinel::Significant::MinAsteroids, ContractDefs::Sentinel::Significant::MaxAsteroids);
                sentinelScanType = SentinelUtilities::RandomScanType(generator.get());
                break;
        }
        switch (sentinelScanType) {
            case SentinelScanType::NONE: num4 = ContractDefs::Sentinel::ScanTypeBaseMultiplier; break;
            case SentinelScanType::CLASS: {
                untrackedObjectClass = SentinelUtilities::WeightedAsteroidClass(generator.get());
                const double num7 = static_cast<double>(9 - 1);  // Enum.GetNames(typeof(UntrackedObjectClass)).Length - 1
                num4 = static_cast<float>((num7 + 1.0 + static_cast<double>(untrackedObjectClass)) / ((num7 + 1.0) * 2.0));
                num4 = Mul(num4, ContractDefs::Sentinel::ScanTypeBaseMultiplier);
                break;
            }
            case SentinelScanType::ECCENTRICITY:
                num5 = SentinelUtilities::WeightedRandom(generator.get(), SentinelUtilities::MinAsteroidEccentricity, SentinelUtilities::MaxAsteroidEccentricity);
                num4 = static_cast<float>(0.5 + num5 / 2.0);
                num4 = Mul(num4, ContractDefs::Sentinel::ScanTypeEccentricityMultiplier);
                break;
            case SentinelScanType::INCLINATION:
                num6 = SentinelUtilities::WeightedRandom(generator.get(), SentinelUtilities::MinAsteroidInclination, SentinelUtilities::MaxAsteroidInclination);
                num4 = static_cast<float>(0.5 + num6 / 2.0);
                num4 = Mul(num4, ContractDefs::Sentinel::ScanTypeInclinationMultiplier);
                break;
        }
        num4 = Mul(num4, Mathf::Round(f32store(fx(static_cast<float>(asteroidCount)) / 2.0f)));
        ConfigNode configNode("PART_REQUEST");
        configNode.AddValue("Article", "a");
        configNode.AddValue("PartDescription", SentinelUtilities::SentinelPartTitle());
        configNode.AddValue("VesselDescription", Fmt("#autoLOC_6002407"));
        configNode.AddValue("Part", SentinelUtilities::SentinelPartName);
        AddParameter(std::make_unique<PartRequestParameter>(configNode));
        Orbit orbit = Orbit::CreateRandomOrbitNearby(OrbitOfBody(outerBody));
        if (innerBody == Sun())
            orbit.semiMajorAxis = L::CelestialUtilities::GetMinimumOrbitalDistance(innerBody, 1.0f) * SentinelUtilities::RandomRange(generator.get(), 1.05, 1.2);
        else
            orbit.semiMajorAxis = OrbitOfBody(innerBody).semiMajorAxis * SentinelUtilities::RandomRange(generator.get(), 1.05, 1.2);
        switch (Prestige()) {
            default:
                AddParameter(std::make_unique<SpecificOrbitParameter>(FinePrint::OrbitType::RANDOM, orbit.inclination, orbit.eccentricity, orbit.semiMajorAxis, orbit.LAN,
                                                                      orbit.argumentOfPeriapsis, orbit.meanAnomalyAtEpoch, orbit.epoch, Sun(), ContractDefs::Sentinel::TrivialDeviation));
                break;
            case ContractPrestige::Exceptional:
                AddParameter(std::make_unique<SpecificOrbitParameter>(FinePrint::OrbitType::RANDOM, orbit.inclination, orbit.eccentricity, orbit.semiMajorAxis, orbit.LAN,
                                                                      orbit.argumentOfPeriapsis, orbit.meanAnomalyAtEpoch, orbit.epoch, Sun(), ContractDefs::Sentinel::ExceptionalDeviation));
                break;
            case ContractPrestige::Significant:
                AddParameter(std::make_unique<SpecificOrbitParameter>(FinePrint::OrbitType::RANDOM, orbit.inclination, orbit.eccentricity, orbit.semiMajorAxis, orbit.LAN,
                                                                      orbit.argumentOfPeriapsis, orbit.meanAnomalyAtEpoch, orbit.epoch, Sun(), ContractDefs::Sentinel::SignificantDeviation));
                break;
        }
        AddParameter(std::make_unique<SentinelParameter>(outerBody, sentinelScanType, untrackedObjectClass, num5, num6, asteroidCount));
        AddKeywords({"Pioneer", "Scientific"});
        SetExpiry(ContractDefs::Sentinel::Expire::MinimumExpireDays, ContractDefs::Sentinel::Expire::MaximumExpireDays);
        SetDeadlineDays(static_cast<float>(RoundI(Mul(static_cast<float>(ContractDefs::Sentinel::Expire::DeadlineDays), num4))), innerBody);
        switch (Prestige()) {
            default:
                SetScience(static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Science::BaseReward, num4), ContractDefs::Sentinel::Trivial::ScienceMultiplier))));
                SetReputation(static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Reputation::BaseReward, num4), ContractDefs::Sentinel::Trivial::ReputationMultiplier))),
                              static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Reputation::BaseFailure, num4), ContractDefs::Sentinel::Trivial::ReputationMultiplier))));
                SetFunds(static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Funds::BaseAdvance, num4), ContractDefs::Sentinel::Trivial::FundsMultiplier))),
                         static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Funds::BaseReward, num4), ContractDefs::Sentinel::Trivial::FundsMultiplier))),
                         static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Funds::BaseFailure, num4), ContractDefs::Sentinel::Trivial::FundsMultiplier))), innerBody);
                break;
            case ContractPrestige::Exceptional:
                SetScience(static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Science::BaseReward, num4), ContractDefs::Sentinel::Exceptional::ScienceMultiplier))));
                SetReputation(static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Reputation::BaseReward, num4), ContractDefs::Sentinel::Exceptional::ReputationMultiplier))),
                              static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Reputation::BaseFailure, num4), ContractDefs::Sentinel::Exceptional::ReputationMultiplier))));
                SetFunds(static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Funds::BaseAdvance, num4), ContractDefs::Sentinel::Exceptional::FundsMultiplier))),
                         static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Funds::BaseReward, num4), ContractDefs::Sentinel::Exceptional::FundsMultiplier))),
                         static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Funds::BaseFailure, num4), ContractDefs::Sentinel::Exceptional::FundsMultiplier))), innerBody);
                break;
            case ContractPrestige::Significant:
                SetScience(static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Science::BaseReward, num4), ContractDefs::Sentinel::Significant::ScienceMultiplier))));
                SetReputation(static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Reputation::BaseReward, num4), ContractDefs::Sentinel::Significant::ReputationMultiplier))),
                              static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Reputation::BaseFailure, num4), ContractDefs::Sentinel::Significant::ReputationMultiplier))));
                SetFunds(static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Funds::BaseAdvance, num4), ContractDefs::Sentinel::Significant::FundsMultiplier))),
                         static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Funds::BaseReward, num4), ContractDefs::Sentinel::Significant::FundsMultiplier))),
                         static_cast<float>(RoundI(Mul(Mul(ContractDefs::Sentinel::Funds::BaseFailure, num4), ContractDefs::Sentinel::Significant::FundsMultiplier))), innerBody);
                break;
        }
        currentContracts = ContractSystem::Instance->GetCurrentContracts<SentinelContract>();
        int num8 = static_cast<int>(currentContracts.size());
        do {
            if (num8-- <= 0) return true;
        } while (!(currentContracts[static_cast<size_t>(num8)]->outerBody == outerBody));
        return false;
    }
    return false;
}

// SentinelContract.cs:247
std::string SentinelContract::GetHashString() { return I(L::SuperSeed(*this)); }

// SentinelContract.cs:252
std::string SentinelContract::GetTitle() {
    if (outerBody == Home()) return Fmt("#autoLOC_6002297", {I(asteroidCount), outerBody->displayName(), SentinelUtilities::SentinelPartTitle()});
    const std::string text = Fmt("#autoLOC_7001310");
    return Fmt("#autoLOC_6002298", {I(asteroidCount), outerBody->displayName(), text, SentinelUtilities::SentinelPartTitle()});
}

// SentinelContract.cs:262
std::string SentinelContract::GetDescription() {
    return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), Fmt("#autoLOC_6002299"), outerBody->displayName(), MissionSeed(), true, false, true);
}

// SentinelContract.cs:267
std::string SentinelContract::GetSynopsys() {
    if (outerBody == Home()) {
        const std::string text = Fmt("#autoLOC_7001311");
        return Fmt("#autoLOC_6002300", {outerBody->displayName(), text, SentinelUtilities::SentinelPartTitle(), innerBody->displayName()});
    }
    const std::string text2 = Fmt("#autoLOC_7001310");
    return Fmt("#autoLOC_6002301", {SentinelUtilities::SentinelPartTitle(), Home()->displayName(), text2, outerBody->displayName()});
}

// SentinelContract.cs:279
std::string SentinelContract::MessageCompleted() {
    const std::string text = Fmt("#autoLOC_7001310");
    return Fmt("#autoLOC_6002302", {outerBody->displayName(), text});
}

// SentinelContract.cs:285
void SentinelContract::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "SentinelContract", "innerBody", innerBody, Sun());
    L::LoadNode(node, "SentinelContract", "outerBody", outerBody, Home());
    L::LoadNode(node, "SentinelContract", "asteroidCount", asteroidCount, 3);
}

// SentinelContract.cs:292
void SentinelContract::OnSave(ConfigNode& node) {
    const int flightGlobalsIndex = innerBody->flightGlobalsIndex;
    node.AddValue("innerBody", static_cast<int32_t>(flightGlobalsIndex));
    const int flightGlobalsIndex2 = outerBody->flightGlobalsIndex;
    node.AddValue("outerBody", static_cast<int32_t>(flightGlobalsIndex2));
    node.AddValue("asteroidCount", static_cast<int32_t>(asteroidCount));
}

// SentinelContract.cs:302
bool SentinelContract::MeetRequirements() {
    if (!ProgressTracking::Instance()->NodeComplete({Sun()->name(), "Orbit"})) return false;
    if (static_cast<int>(GameVariables::Instance->GetOrbitDisplayMode(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::TrackingStation))) <
        static_cast<int>(GameVariables::OrbitDisplayMode::AllOrbits))
        return false;
    return true;
}

// SentinelContract.cs:316
void SentinelContract::SetFocusBody(CelestialBody* body) {
    if (!SentinelUtilities::IsOnSolarOrbit(body)) {
        innerBody = Sun();
        outerBody = Home();
    } else {
        SentinelUtilities::FindInnerAndOuterBodies(OrbitOfBody(body).semiMajorAxis - 1.0, innerBody, outerBody);
    }
}

// ================================================================================================================================ CometDetectionContract

// CometDetectionContract.cs:16
bool CometDetectionContract::Generate() {
    if (!L::ProgressUtilities::HavePartTech(SentinelUtilities::SentinelPartName, false)) return false;
    if (Prestige() == ContractPrestige::Trivial) return false;
    int num = 0;
    int num2 = 0;
    std::vector<CometDetectionContract*> currentContracts = ContractSystem::Instance->GetCurrentContracts<CometDetectionContract>();
    int num3 = static_cast<int>(currentContracts.size());
    while (num3-- > 0) {
        switch (currentContracts[static_cast<size_t>(num3)]->ContractState()) {
            case State::Active: num2++; break;
            case State::Offered: num++; break;
            default: break;
        }
    }
    if (num < ContractDefs::CometDetection::MaximumAvailable && num2 < ContractDefs::CometDetection::MaximumActive) {
        switch (Prestige()) {
            case ContractPrestige::Exceptional: cometCount = 3; break;
            case ContractPrestige::Significant: cometCount = 2; break;
            default: break;
        }
        ConfigNode configNode("PART_REQUEST");
        configNode.AddValue("Article", "a");
        configNode.AddValue("PartDescription", SentinelUtilities::SentinelPartTitle());
        configNode.AddValue("VesselDescription", Fmt("#autoLOC_6002407"));
        configNode.AddValue("Part", SentinelUtilities::SentinelPartName);
        AddParameter(std::make_unique<PartRequestParameter>(configNode));
        AddParameter(std::make_unique<EnterOrbit>(Sun()));
        AddParameter(std::make_unique<CometDetectionParameter>(cometCount, SentinelUtilities::SentinelPartTitle()));
        AddKeywords({"Scientific"});
        SetExpiry(ContractDefs::CometDetection::Expire::MinimumExpireDays, ContractDefs::CometDetection::Expire::MaximumExpireDays);
        SetDeadlineDays(static_cast<float>(RoundI(static_cast<float>(ContractDefs::CometDetection::Expire::DeadlineDays))));
        switch (Prestige()) {
            case ContractPrestige::Exceptional:
                SetScience(static_cast<float>(RoundI(Mul(ContractDefs::CometDetection::Science::BaseReward, ContractDefs::CometDetection::Exceptional::ScienceMultiplier))));
                SetReputation(static_cast<float>(RoundI(Mul(ContractDefs::CometDetection::Reputation::BaseReward, ContractDefs::CometDetection::Exceptional::ReputationMultiplier))),
                              static_cast<float>(RoundI(Mul(ContractDefs::CometDetection::Reputation::BaseFailure, ContractDefs::CometDetection::Exceptional::ReputationMultiplier))));
                SetFunds(static_cast<float>(RoundI(Mul(ContractDefs::CometDetection::Funds::BaseAdvance, ContractDefs::CometDetection::Exceptional::FundsMultiplier))),
                         static_cast<float>(RoundI(Mul(ContractDefs::CometDetection::Funds::BaseReward, ContractDefs::CometDetection::Exceptional::FundsMultiplier))),
                         static_cast<float>(RoundI(Mul(ContractDefs::CometDetection::Funds::BaseFailure, ContractDefs::CometDetection::Exceptional::FundsMultiplier))));
                break;
            case ContractPrestige::Significant:
                SetScience(static_cast<float>(RoundI(Mul(ContractDefs::CometDetection::Science::BaseReward, ContractDefs::CometDetection::Significant::ScienceMultiplier))));
                SetReputation(static_cast<float>(RoundI(Mul(ContractDefs::CometDetection::Reputation::BaseReward, ContractDefs::CometDetection::Significant::ReputationMultiplier))),
                              static_cast<float>(RoundI(Mul(ContractDefs::CometDetection::Reputation::BaseFailure, ContractDefs::CometDetection::Significant::ReputationMultiplier))));
                SetFunds(static_cast<float>(RoundI(Mul(ContractDefs::CometDetection::Funds::BaseAdvance, ContractDefs::CometDetection::Significant::FundsMultiplier))),
                         static_cast<float>(RoundI(Mul(ContractDefs::CometDetection::Funds::BaseReward, ContractDefs::CometDetection::Significant::FundsMultiplier))),
                         static_cast<float>(RoundI(Mul(ContractDefs::CometDetection::Funds::BaseFailure, ContractDefs::CometDetection::Significant::FundsMultiplier))));
                break;
            default: break;
        }
        return true;
    }
    return false;
}

// CometDetectionContract.cs:104
std::string CometDetectionContract::GetHashString() { return I(L::SuperSeed(*this)); }

// CometDetectionContract.cs:109
std::string CometDetectionContract::GetTitle() { return Fmt("#autoLOC_6011148", {I(cometCount), SentinelUtilities::SentinelPartTitle()}); }

// CometDetectionContract.cs:114
std::string CometDetectionContract::GetDescription() {
    return TextGen::GenerateBackStories("CometDetection", Agent()->Title(), Fmt("#autoLOC_6006050"), Sun()->displayName(), MissionSeed(), false, false, false);
}

// CometDetectionContract.cs:119
std::string CometDetectionContract::GetSynopsys() { return Fmt("#autoLOC_6011146", {I(cometCount), SentinelUtilities::SentinelPartTitle()}); }

// CometDetectionContract.cs:124
std::string CometDetectionContract::MessageCompleted() { return Fmt("#autoLOC_6011145", {SentinelUtilities::SentinelPartTitle()}); }

// CometDetectionContract.cs:129
void CometDetectionContract::OnLoad(ConfigNode& node) { L::LoadNode(node, "CometContract", "cometCount", cometCount, 3); }

// CometDetectionContract.cs:134
void CometDetectionContract::OnSave(ConfigNode& node) { node.AddValue("cometCount", static_cast<int32_t>(cometCount)); }

// CometDetectionContract.cs:139
bool CometDetectionContract::MeetRequirements() {
    if (!ProgressTracking::Instance()->NodeComplete({Sun()->name(), "Orbit"})) return false;
    if (static_cast<int>(GameVariables::Instance->GetOrbitDisplayMode(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::TrackingStation))) <
        static_cast<int>(GameVariables::OrbitDisplayMode::AllOrbits))
        return false;
    return true;
}

}  // namespace ksp::SentinelMission
