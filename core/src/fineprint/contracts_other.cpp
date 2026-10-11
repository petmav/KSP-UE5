// WP-38 slice 2: FinePrint/Contracts/{SatelliteContract,CometSampleContract,TourismContract}.cs.
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
#include "ksp/contracts/parameters.hpp"
#include "ksp/contracts/text_gen.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/fineprint/contracts.hpp"
#include "ksp/fineprint/scenario_contract_events.hpp"
#include "ksp/kerbals/crew_generator.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/science/science.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"
#include "param_common.hpp"

namespace ksp::FinePrint::Contracts {

using namespace ksp::fpcommon;
using ksp::Contracts::ContractDefsLite;
using ksp::Contracts::ContractSystem;
using ksp::Contracts::TextGen;
using ksp::Contracts::Parameters::CometScienceParameter;
using ksp::Contracts::Parameters::CrewTraitParameter;
using ksp::Contracts::Parameters::KerbalDestinationParameter;
using ksp::Contracts::Parameters::KerbalGeeAdventureParameter;
using ksp::Contracts::Parameters::KerbalTourParameter;
using ksp::Contracts::Parameters::ReachDestination;
using ksp::Contracts::Parameters::ReachSituation;
using ksp::Contracts::Parameters::SpecificOrbitParameter;
using ksp::Contracts::Parameters::StabilityParameter;
using ksp::Contracts::Parameters::StationaryPointParameter;
using ksp::Contracts::Parameters::VesselSystemsParameter;
using Lite::MannedStatus;
using Lite::ProgressType;
namespace PU = Lite::ProgressUtilities;
namespace M = ksp::dotnet::Math;
using GE = events::GameEvents;

namespace {
float DF(const char* sec, const char* sub, const char* key) { return ContractDefsLite::F(sec, sub, key); }
double DD(const char* sec, const char* sub, const char* key) { return ContractDefsLite::D(sec, sub, key); }
double DFd(const char* sec, const char* sub, const char* key) { return static_cast<double>(ContractDefsLite::F(sec, sub, key)); }
int DI(const char* sec, const char* sub, const char* key) { return ContractDefsLite::I(sec, sub, key); }
bool DB(const char* sec, const char* sub, const char* key) { return ContractDefsLite::B(sec, sub, key); }
float Mul(float a, float b) { return f32store(fx(a) * fx(b)); }
float Round(float f) { return Mathf::Round(f); }
CelestialBody* Home() { return Planetarium::fetch->Home; }
CelestialBody* Sun() { return Planetarium::fetch->Sun; }
std::string Lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
bool Has(const std::vector<CelestialBody*>& l, CelestialBody* b) { return std::find(l.begin(), l.end(), b) != l.end(); }
}  // namespace

// ================================================================================================================================ SatelliteContract

// SatelliteContract.cs:632
bool SatelliteContract::AreSatellitesUnlocked() {
    if (ProbeCoresUnlocked() && Lite::ResearchedValidContractObjectives({"Antenna", "Generator"})) return true;
    return false;
}
// SatelliteContract.cs:634
bool SatelliteContract::ProbeCoresUnlocked() {
    if (Lite::FinePrintEnvironment::probeCoresUnlocked) return Lite::FinePrintEnvironment::probeCoresUnlocked();
    PartLoader* loader = PartLoader::Instance();
    if (loader == nullptr) return false;
    int count = static_cast<int>(loader->loadedParts.size());
    AvailablePart* availablePart;
    do {
        if (count-- > 0) {
            availablePart = loader->loadedParts[static_cast<size_t>(count)].get();
            continue;
        }
        return false;
    } while (availablePart->partPrefab == nullptr || !ResearchAndDevelopment::PartTechAvailable(availablePart) ||
             static_cast<int>(availablePart->partPrefab->fields.isControlSource) <= static_cast<int>(ControlLevel::NONE) ||
             availablePart->partPrefab->fields.CrewCapacity != 0);
    return true;
}
// SatelliteContract.cs:38
bool SatelliteContract::Generate() {
    if (!AreSatellitesUnlocked()) return false;
    int num = 0;
    int num2 = 0;
    auto currentContracts = ContractSystem::Instance->GetCurrentContracts<SatelliteContract>();
    int num3 = static_cast<int>(currentContracts.size());
    while (num3-- > 0) {
        switch (currentContracts[static_cast<size_t>(num3)]->ContractState()) {
            case State::Active: num2++; break;
            case State::Offered: num++; break;
            default: break;
        }
    }
    if (num < DI("Satellite", "", "MaximumAvailable") && num2 < DI("Satellite", "", "MaximumActive")) {
        const int seed = Lite::SuperSeed(*this);
        generator = std::make_unique<KSPRandom>(seed);
        float fundsMultiplier = 1.0f;
        float scienceMultiplier = 1.0f;
        float reputationMultiplier = 1.0f;
        const int num4 = static_cast<int>(prestige);
        auto notSun = [](CelestialBody* cb) { return cb != Sun(); };
        std::vector<CelestialBody*> bodiesProgress = PU::GetBodiesProgressNoReach(ProgressType::ORBIT, true, notSun);
        for (CelestialBody* b : PU::GetNextUnreached(num4, MannedStatus::UNMANNED, notSun)) bodiesProgress.push_back(b);
        if (DB("Satellite", "", "PreferHome") && generator->Next(0, num4 + 1) == num4) bodiesProgress.push_back(Home());
        if (DB("Satellite", "", "AllowSolar") && generator->Next(0, 3) < num4 && ProgressTracking::Instance()->NodeComplete({Sun()->name(), "Orbit"}))
            bodiesProgress.push_back(Sun());
        if (bodiesProgress.empty()) bodiesProgress.push_back(Home());
        switch (prestige) {
            case ContractPrestige::Trivial:
                deviation = DD("Satellite", "", "TrivialDeviation");
                altitudeFactor = DFd("Satellite", "", "TrivialAltitudeDifficulty");
                inclinationFactor = DFd("Satellite", "", "TrivialInclinationDifficulty");
                break;
            case ContractPrestige::Significant:
                deviation = DD("Satellite", "", "SignificantDeviation");
                altitudeFactor = DFd("Satellite", "", "SignificantAltitudeDifficulty");
                inclinationFactor = DFd("Satellite", "", "SignificantInclinationDifficulty");
                break;
            case ContractPrestige::Exceptional:
                deviation = DD("Satellite", "", "ExceptionalDeviation");
                altitudeFactor = DFd("Satellite", "", "ExceptionalAltitudeDifficulty");
                inclinationFactor = DFd("Satellite", "", "ExceptionalInclinationDifficulty");
                break;
        }
        targetBody = WeightedBodyChoice(bodiesProgress, generator.get());
        if (targetBody == nullptr) targetBody = Home();
        switch (prestige) {
            case ContractPrestige::Trivial: pickEasy(); break;
            case ContractPrestige::Significant: pickMedium(); break;
            case ContractPrestige::Exceptional: pickHard(); break;
        }
        std::vector<Vessel*> list = Lite::SpecificVesselClassAt(Vessel::Situations::ORBITING, targetBody, VesselType::Probe, true, true, false);
        std::unique_ptr<Orbit> orbit;
        double stationaryLongitude = 0.0;
        Vessel* v = nullptr;
        Vessel* adjustVessel = nullptr;
        const float num5 = targetBody == Home() ? DF("Satellite", "", "ContextualHomeAssets") : DF("Satellite", "", "ContextualAssets");
        const int num6 = Mathf::RoundToInt(f32store(fx(DF("Satellite", "", "ContextualChance")) * (static_cast<double>(M::Min(static_cast<float>(list.size()), num5)) / fx(num5))));
        if (generator->Next(0, 100) < num6) {
            Lite::SystemUtilities::ShuffleList(list, generator.get());
            int count = static_cast<int>(list.size());
            while (count-- > 0 && placementMode == SatellitePlacementMode::NEW) {
                Vessel* vessel = list[static_cast<size_t>(count)];
                const OrbitType orbitTypeL = Utilities::OrbitUtilities::IdentifyOrbit(vessel->loaded ? vessel->orbit() : vessel->protoVessel->orbitSnapShot->Load(FlightRegistry::Bodies()));
                const unsigned ot = static_cast<unsigned>(orbitTypeL);
                if (ot > 1u && (ot - 4u) > 1u) {
                    v = vessel;
                    placementMode = SatellitePlacementMode::ADJUST;
                    this->orbitType = OrbitType::RANDOM;
                } else {
                    if (!CheckSatelliteNetwork(NetworkSatelliteCount(targetBody), list, orbitTypeL, orbit, stationaryLongitude, adjustVessel, vessel)) continue;
                    if (adjustVessel == nullptr) {
                        if (PrestigeAppropriateOrbitType(orbitTypeL)) {
                            v = vessel;
                            placementMode = SatellitePlacementMode::NETWORK;
                        }
                    } else if (Prestige() >= ContractPrestige::Significant) {
                        v = adjustVessel;
                        placementMode = SatellitePlacementMode::NETWORKADJUST;
                    }
                    this->orbitType = orbitTypeL;
                }
            }
            if (placementMode == SatellitePlacementMode::ADJUST || placementMode == SatellitePlacementMode::NETWORKADJUST) {
                switch (Prestige()) {
                    case ContractPrestige::Exceptional:
                        fundsMultiplier = Mul(fundsMultiplier, 0.4f);
                        scienceMultiplier = Mul(scienceMultiplier, 0.4f);
                        reputationMultiplier = Mul(reputationMultiplier, 0.4f);
                        break;
                    default:
                        fundsMultiplier = Mul(fundsMultiplier, 0.2f);
                        scienceMultiplier = Mul(scienceMultiplier, 0.2f);
                        reputationMultiplier = Mul(reputationMultiplier, 0.2f);
                        break;
                    case ContractPrestige::Significant:
                        fundsMultiplier = Mul(fundsMultiplier, 0.3f);
                        scienceMultiplier = Mul(scienceMultiplier, 0.3f);
                        reputationMultiplier = Mul(reputationMultiplier, 0.3f);
                        break;
                }
                AddParameter(std::make_unique<SpecificVesselParameter>(v));
            }
        }
        if (objectiveTypes.empty()) objectiveTypes = {"Antenna", "Generator"};
        AddParameter(std::make_unique<VesselSystemsParameter>(objectiveTypes, std::vector<std::string>{Fmt("#autoLOC_7001075"), Fmt("#autoLOC_7001077")}, Fmt("#autoLOC_7001055"),
                                                              MannedStatus::UNMANNED, placementMode == SatellitePlacementMode::NEW || placementMode == SatellitePlacementMode::NETWORK));
        double eccentricityOverride = 0.0;
        if (this->orbitType == OrbitType::SYNCHRONOUS) eccentricityOverride = generator->NextDouble() * (altitudeFactor / 2.0);
        std::unique_ptr<Orbit> orbit2;
        switch (placementMode) {
            case SatellitePlacementMode::NETWORK:
            case SatellitePlacementMode::NETWORKADJUST:
                if (orbit != nullptr) orbit2 = std::make_unique<Orbit>(*orbit);
                break;
            default: orbit2 = std::make_unique<Orbit>(Utilities::OrbitUtilities::GenerateOrbit(seed, targetBody, this->orbitType, altitudeFactor, inclinationFactor, eccentricityOverride)); break;
            case SatellitePlacementMode::ADJUST:
                switch (Prestige()) {
                    case ContractPrestige::Exceptional:
                        orbit2 = std::make_unique<Orbit>(Lite::GenerateAdjustedVesselOrbit(deviation, DD("Satellite", "", "TrivialDeviation"), 3, generator.get(), v));
                        break;
                    default:
                        orbit2 = std::make_unique<Orbit>(Lite::GenerateAdjustedVesselOrbit(deviation, DD("Satellite", "", "ExceptionalDeviation"), 1, generator.get(), v));
                        break;
                    case ContractPrestige::Significant:
                        orbit2 = std::make_unique<Orbit>(Lite::GenerateAdjustedVesselOrbit(deviation, DD("Satellite", "", "SignificantDeviation"), 2, generator.get(), v));
                        break;
                }
                break;
        }
        if (orbit2 == nullptr) return false;
        AddParameter(std::make_unique<SpecificOrbitParameter>(this->orbitType, orbit2->inclination, orbit2->eccentricity, orbit2->semiMajorAxis, orbit2->LAN,
                                                              orbit2->argumentOfPeriapsis, orbit2->meanAnomalyAtEpoch, orbit2->epoch, targetBody, deviation));
        if (this->orbitType == OrbitType::STATIONARY) {
            if (placementMode != SatellitePlacementMode::NETWORK && placementMode != SatellitePlacementMode::NETWORKADJUST) {
                double ignoredLatitude = 0.0;
                WaypointManager::ChooseRandomPosition(ignoredLatitude, stationaryLongitude, targetBody->GetName(), false, true);
            }
            AddParameter(std::make_unique<StationaryPointParameter>(targetBody, stationaryLongitude, deviation));
        }
        ConfigNode* node = ContractDefsLite::Node("Satellite");
        if (node != nullptr && placementMode != SatellitePlacementMode::ADJUST && placementMode != SatellitePlacementMode::NETWORKADJUST)
            Lite::SystemUtilities::ProcessSideRequests(*this, *node, targetBody, Fmt("#autoLOC_500053"), fundsMultiplier, scienceMultiplier, reputationMultiplier);
        switch (this->orbitType) {
            case OrbitType::SYNCHRONOUS:
                fundsMultiplier = Mul(fundsMultiplier, DF("Satellite", "Funds", "SynchronousMultiplier"));
                scienceMultiplier = Mul(scienceMultiplier, DF("Satellite", "Science", "SynchronousMultiplier"));
                reputationMultiplier = Mul(reputationMultiplier, DF("Satellite", "Reputation", "SynchronousMultiplier"));
                break;
            case OrbitType::STATIONARY:
                fundsMultiplier = Mul(fundsMultiplier, DF("Satellite", "Funds", "StationaryMultiplier"));
                scienceMultiplier = Mul(scienceMultiplier, DF("Satellite", "Science", "StationaryMultiplier"));
                reputationMultiplier = Mul(reputationMultiplier, DF("Satellite", "Reputation", "StationaryMultiplier"));
                break;
            case OrbitType::POLAR:
                fundsMultiplier = Mul(fundsMultiplier, DF("Satellite", "Funds", "PolarMultiplier"));
                scienceMultiplier = Mul(scienceMultiplier, DF("Satellite", "Science", "PolarMultiplier"));
                reputationMultiplier = Mul(reputationMultiplier, DF("Satellite", "Reputation", "PolarMultiplier"));
                break;
            case OrbitType::KOLNIYA:
                fundsMultiplier = Mul(fundsMultiplier, DF("Satellite", "Funds", "KolniyaMultiplier"));
                scienceMultiplier = Mul(scienceMultiplier, DF("Satellite", "Science", "KolniyaMultiplier"));
                reputationMultiplier = Mul(reputationMultiplier, DF("Satellite", "Reputation", "KolniyaMultiplier"));
                break;
            case OrbitType::TUNDRA:
                fundsMultiplier = Mul(fundsMultiplier, DF("Satellite", "Funds", "TundraMultiplier"));
                scienceMultiplier = Mul(scienceMultiplier, DF("Satellite", "Science", "TundraMultiplier"));
                reputationMultiplier = Mul(reputationMultiplier, DF("Satellite", "Reputation", "TundraMultiplier"));
                break;
            default: break;
        }
        if (targetBody == Home()) {
            AddKeywords({"Commercial"});
            fundsMultiplier = Mul(fundsMultiplier, DF("Satellite", "Funds", "HomeMultiplier"));
            scienceMultiplier = Mul(scienceMultiplier, DF("Satellite", "Science", "HomeMultiplier"));
            reputationMultiplier = Mul(reputationMultiplier, DF("Satellite", "Reputation", "HomeMultiplier"));
        } else {
            AddKeywords({Lite::CoinFlip(generator.get()) ? "Commercial" : "Scientific"});
        }
        AddParameter(std::make_unique<StabilityParameter>(10.0f));
        const float num7 = static_cast<float>(M::Max(FundsCompletion, static_cast<double>(DF("Satellite", "Funds", "BaseReward"))));
        const float num8 = M::Max(ScienceCompletion, DF("Satellite", "Science", "BaseReward"));
        const float num9 = M::Max(ReputationCompletion, DF("Satellite", "Reputation", "BaseReward"));
        SetExpiry(DI("Satellite", "Expiration", "MinimumExpireDays"), DI("Satellite", "Expiration", "MaximumExpireDays"));
        SetDeadlineDays(static_cast<float>(DI("Satellite", "Expiration", "DeadlineDays")), targetBody);
        SetFunds(Round(Mul(DF("Satellite", "Funds", "BaseAdvance"), fundsMultiplier)), Round(Mul(num7, fundsMultiplier)),
                 Round(Mul(DF("Satellite", "Funds", "BaseFailure"), fundsMultiplier)), targetBody);
        SetScience(Round(Mul(num8, scienceMultiplier)));
        SetReputation(Round(Mul(num9, reputationMultiplier)), Round(Mul(DF("Satellite", "Reputation", "BaseFailure"), reputationMultiplier)));
        if (static_cast<int>(ContractSystem::Instance->GetCurrentContracts<SatelliteContract>([this](SatelliteContract* contract) { return contract->targetBody == targetBody; }).size()) >= 2)
            return false;
        return true;
    }
    return false;
}
// SatelliteContract.cs:307
std::string SatelliteContract::GetHashString() { return I(Lite::SuperSeed(*this)); }

namespace {
// ContractDefs.OtherSynchronousName etc. (cached Localizer.Format of #autoLOC_7001205 .. #autoLOC_7001210, #autoLOC_7001203)
std::string SyncName(CelestialBody* body) {
    if (body == Sun()) return Lower(Fmt("#autoLOC_7001208"));
    if (body == Home()) return Lower(Fmt("#autoLOC_7001209"));
    return Lower(Fmt("#autoLOC_7001210"));
}
std::string StatName(CelestialBody* body) {
    if (body == Sun()) return Lower(Fmt("#autoLOC_7001205"));
    if (body == Home()) return Lower(Fmt("#autoLOC_7001206"));
    return Lower(Fmt("#autoLOC_7001207"));
}
std::string MolniyaName() { return Fmt("#autoLOC_7001203"); }
}  // namespace

// SatelliteContract.cs:312
std::string SatelliteContract::GetTitle() {
    using SPM = SatellitePlacementMode;
    const std::string bn = targetBody->displayName();
    if (placementMode == SPM::ADJUST) return Fmt("#autoLOC_279915", {Lite::StringUtilities::SpecificVesselName(*this), bn});
    switch (orbitType) {
        default: return Fmt("#autoLOC_279970", {bn});
        case OrbitType::SYNCHRONOUS: {
            const std::string text2 = SyncName(targetBody);
            if (placementMode == SPM::NETWORK) return Fmt("#autoLOC_279963", {text2, bn});
            if (placementMode == SPM::NETWORKADJUST) return Fmt("#autoLOC_279966", {Lite::StringUtilities::SpecificVesselName(*this), text2, bn});
            return Fmt("#autoLOC_279968", {text2, bn});
        }
        case OrbitType::STATIONARY: {
            const std::string text = StatName(targetBody);
            if (placementMode == SPM::NETWORK) return Fmt("#autoLOC_279948", {text, bn});
            if (placementMode == SPM::NETWORKADJUST) return Fmt("#autoLOC_279951", {Lite::StringUtilities::SpecificVesselName(*this), text, bn});
            return Fmt("#autoLOC_279953", {text, bn});
        }
        case OrbitType::POLAR: return Fmt("#autoLOC_279922", {bn});
        case OrbitType::EQUATORIAL: return Fmt("#autoLOC_279920", {bn});
        case OrbitType::KOLNIYA:
            if (placementMode == SPM::NETWORK) return Fmt("#autoLOC_279925", {Lite::StringUtilities::TitleCase(MolniyaName()), bn});
            if (placementMode == SPM::NETWORKADJUST)
                return Fmt("#autoLOC_279928", {Lite::StringUtilities::SpecificVesselName(*this), Lite::StringUtilities::TitleCase(MolniyaName()), bn});
            return Fmt("#autoLOC_279930", {Lite::StringUtilities::TitleCase(MolniyaName()), bn});
        case OrbitType::TUNDRA:
            if (placementMode == SPM::NETWORK) return Fmt("#autoLOC_279933", {bn});
            if (placementMode == SPM::NETWORKADJUST) return Fmt("#autoLOC_279936", {Lite::StringUtilities::SpecificVesselName(*this), bn});
            return Fmt("#autoLOC_279938", {bn});
    }
}
// SatelliteContract.cs:391
std::string SatelliteContract::GetDescription() {
    std::string topic = "Satellite";
    const std::string text = (placementMode == SatellitePlacementMode::NETWORK || placementMode == SatellitePlacementMode::NETWORKADJUST) ? "Network" : "Orbit";
    switch (orbitType) {
        case OrbitType::SYNCHRONOUS: topic = "Synchronous " + text; break;
        case OrbitType::STATIONARY: topic = "Stationary " + text; break;
        case OrbitType::POLAR: topic = "Polar " + text; break;
        case OrbitType::EQUATORIAL: topic = "Equatorial " + text; break;
        case OrbitType::KOLNIYA: topic = Lite::StringUtilities::TitleCase(MolniyaName()) + " " + text; break;
        case OrbitType::TUNDRA: topic = "Tundra " + text; break;
        default: break;
    }
    if (placementMode == SatellitePlacementMode::ADJUST || placementMode == SatellitePlacementMode::NETWORKADJUST) topic = Lite::StringUtilities::SpecificVesselName(*this);
    return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), topic, targetBody->displayName(), MissionSeed(), true, true, true);
}
// SatelliteContract.cs:423
std::string SatelliteContract::GetSynopsys() {
    using SPM = SatellitePlacementMode;
    const std::string bn = targetBody->displayName();
    if (placementMode == SPM::ADJUST) return Fmt("#autoLOC_280010", {Lite::StringUtilities::SpecificVesselName(*this), bn});
    switch (orbitType) {
        default: return Fmt("#autoLOC_280065", {bn});
        case OrbitType::SYNCHRONOUS: {
            const std::string text2 = SyncName(targetBody);
            if (placementMode == SPM::NETWORK) return Fmt("#autoLOC_280058", {bn, text2});
            if (placementMode == SPM::NETWORKADJUST) return Fmt("#autoLOC_280061", {Lite::StringUtilities::SpecificVesselName(*this), text2, bn});
            return Fmt("#autoLOC_280063", {text2, bn});
        }
        case OrbitType::STATIONARY: {
            const std::string text = StatName(targetBody);
            if (placementMode == SPM::NETWORK) return Fmt("#autoLOC_280043", {bn, text});
            if (placementMode == SPM::NETWORKADJUST) return Fmt("#autoLOC_280046", {Lite::StringUtilities::SpecificVesselName(*this), text, bn});
            return Fmt("#autoLOC_280048", {text, bn});
        }
        case OrbitType::POLAR: return Fmt("#autoLOC_280017", {bn});
        case OrbitType::EQUATORIAL: return Fmt("#autoLOC_280015", {bn});
        case OrbitType::KOLNIYA:
            if (placementMode == SPM::NETWORK) return Fmt("#autoLOC_280020", {bn, MolniyaName()});
            if (placementMode == SPM::NETWORKADJUST) return Fmt("#autoLOC_280023", {Lite::StringUtilities::SpecificVesselName(*this), MolniyaName(), bn});
            return Fmt("#autoLOC_280025", {Lite::StringUtilities::TitleCase(MolniyaName()), bn});
        case OrbitType::TUNDRA:
            if (placementMode == SPM::NETWORK) return Fmt("#autoLOC_280028", {bn});
            if (placementMode == SPM::NETWORKADJUST) return Fmt("#autoLOC_280031", {Lite::StringUtilities::SpecificVesselName(*this), bn});
            return Fmt("#autoLOC_280033", {bn});
    }
}
// SatelliteContract.cs:502
std::string SatelliteContract::MessageCompleted() {
    using SPM = SatellitePlacementMode;
    const std::string bn = targetBody->displayName();
    if (placementMode == SPM::ADJUST) return Fmt("#autoLOC_280072", {Lite::StringUtilities::SpecificVesselName(*this), bn});
    switch (orbitType) {
        default: return Fmt("#autoLOC_280127", {bn});
        case OrbitType::SYNCHRONOUS: {
            const std::string text2 = SyncName(targetBody);
            if (placementMode == SPM::NETWORK) return Fmt("#autoLOC_280120", {text2, bn});
            if (placementMode == SPM::NETWORKADJUST) return Fmt("#autoLOC_280123", {Lite::StringUtilities::SpecificVesselName(*this), text2, bn});
            return Fmt("#autoLOC_280125", {text2, bn});
        }
        case OrbitType::STATIONARY: {
            const std::string text = StatName(targetBody);
            if (placementMode == SPM::NETWORK) return Fmt("#autoLOC_280105", {text, bn});
            if (placementMode == SPM::NETWORKADJUST) return Fmt("#autoLOC_280108", {Lite::StringUtilities::SpecificVesselName(*this), text, bn});
            return Fmt("#autoLOC_280110", {text, bn});
        }
        case OrbitType::POLAR: return Fmt("#autoLOC_280079", {bn});
        case OrbitType::EQUATORIAL: return Fmt("#autoLOC_280077", {bn});
        case OrbitType::KOLNIYA:
            if (placementMode == SPM::NETWORK) return Fmt("#autoLOC_280082", {Lite::StringUtilities::TitleCase(MolniyaName()), bn});
            if (placementMode == SPM::NETWORKADJUST)
                return Fmt("#autoLOC_280085", {Lite::StringUtilities::SpecificVesselName(*this), Lite::StringUtilities::TitleCase(MolniyaName()), bn});
            return Fmt("#autoLOC_280087", {Lite::StringUtilities::TitleCase(MolniyaName()), bn});
        case OrbitType::TUNDRA:
            if (placementMode == SPM::NETWORK) return Fmt("#autoLOC_280090", {bn});
            if (placementMode == SPM::NETWORKADJUST) return Fmt("#autoLOC_280093", {Lite::StringUtilities::SpecificVesselName(*this), bn});
            return Fmt("#autoLOC_280095", {bn});
    }
}
// SatelliteContract.cs:581
void SatelliteContract::OnSave(ConfigNode& node) {
    node.AddValue("targetBody", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("deviation", deviation);
    node.AddValue("orbitType", static_cast<int32_t>(orbitType));
    node.AddValue("placementMode", static_cast<int32_t>(placementMode));
}
// SatelliteContract.cs:589
void SatelliteContract::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "SatelliteContract", "targetBody", targetBody, Home());
    Lite::LoadNode(node, "SatelliteContract", "deviation", deviation, 10.0);
    Lite::LoadNode(node, "SatelliteContract", "orbitType", orbitType, OrbitType::RANDOM);
    Lite::LoadNode(node, "SatelliteContract", "placementMode", placementMode, SatellitePlacementMode::NEW);
}
// SatelliteContract.cs:597
bool SatelliteContract::MeetRequirements() {
    if (!ProgressTracking::Instance()->NodeComplete({Home()->name(), "Orbit"})) return false;
    if (static_cast<int>(GameVariables::Instance->GetOrbitDisplayMode(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::TrackingStation))) <
        static_cast<int>(GameVariables::OrbitDisplayMode::AllOrbits))
        return false;
    return true;
}
// SatelliteContract.cs:615
void SatelliteContract::OnAccepted() {
    auto* parameter = GetParameterT<VesselSystemsParameter>();
    if (parameter != nullptr && parameter->requireNew) parameter->launchID = SaveEnvironment::Current().currentGame->launchID;
}
// SatelliteContract.cs:624
void SatelliteContract::OnFinished() {
    auto* parameter = GetParameterT<SpecificOrbitParameter>();
    auto* parameter2 = GetParameterT<SpecificVesselParameter>();
    if (parameter != nullptr) parameter->CleanupRenderer();
    if (parameter2 != nullptr) parameter2->CleanupWaypoints();
}
// SatelliteContract.cs:658
void SatelliteContract::pickEasy() {
    if (generator != nullptr) {
        const int num = generator->Next(0, 101);
        if (num <= 33)
            setOrbitType(OrbitType::RANDOM, altitudeFactor);
        else if (num > 33 && num <= 66)
            setOrbitType(OrbitType::POLAR, altitudeFactor);
        else
            setOrbitType(OrbitType::EQUATORIAL, altitudeFactor);
    }
}
// SatelliteContract.cs:678
void SatelliteContract::pickMedium() {
    if (generator != nullptr) {
        const int num = generator->Next(0, 101);
        if (num <= 33)
            pickEasy();
        else if (num > 33 && num <= 66)
            setOrbitType(OrbitType::RANDOM, altitudeFactor);
        else
            setOrbitType(OrbitType::SYNCHRONOUS, altitudeFactor);
    }
}
// SatelliteContract.cs:698
void SatelliteContract::pickHard() {
    if (generator != nullptr) {
        const int num = generator->Next(0, 101);
        if (num <= 20)
            pickMedium();
        else if (num > 20 && num <= 40)
            setOrbitType(OrbitType::RANDOM, altitudeFactor);
        else if (num > 40 && num <= 60)
            setOrbitType(OrbitType::KOLNIYA, altitudeFactor);
        else if (num > 60 && num <= 80)
            setOrbitType(OrbitType::TUNDRA, altitudeFactor);
        else
            setOrbitType(OrbitType::STATIONARY, altitudeFactor);
    }
}
// SatelliteContract.cs:726
void SatelliteContract::setOrbitType(OrbitType targetType, double altitudeFactor_) {
    if (targetBody == nullptr) return;
    switch (targetType) {
        case OrbitType::SYNCHRONOUS:
            if (!DB("Satellite", "", "AllowSynchronous")) {
                orbitType = OrbitType::RANDOM;
                return;
            }
            break;
        case OrbitType::STATIONARY:
            if (!DB("Satellite", "", "AllowStationary")) {
                orbitType = OrbitType::RANDOM;
                return;
            }
            break;
        case OrbitType::POLAR:
            if (!DB("Satellite", "", "AllowPolar")) {
                orbitType = OrbitType::RANDOM;
                return;
            }
            break;
        case OrbitType::EQUATORIAL:
            if (!DB("Satellite", "", "AllowEquatorial")) {
                orbitType = OrbitType::RANDOM;
                return;
            }
            break;
        case OrbitType::KOLNIYA:
            if (!DB("Satellite", "", "AllowKolniya")) {
                orbitType = OrbitType::RANDOM;
                return;
            }
            break;
        case OrbitType::TUNDRA:
            if (!DB("Satellite", "", "AllowTundra")) {
                orbitType = OrbitType::RANDOM;
                return;
            }
            break;
        default: break;
    }
    switch (targetType) {
        case OrbitType::SYNCHRONOUS:
            orbitType = Lite::CelestialUtilities::CanBodyBeSynchronous(targetBody, altitudeFactor_ / 2.0) ? targetType : OrbitType::RANDOM;
            break;
        case OrbitType::STATIONARY:
            orbitType = Lite::CelestialUtilities::CanBodyBeSynchronous(targetBody, altitudeFactor_ / 2.0) ? targetType : OrbitType::RANDOM;
            break;
        case OrbitType::KOLNIYA: orbitType = Lite::CelestialUtilities::CanBodyBeKolniya(targetBody) ? targetType : OrbitType::RANDOM; break;
        case OrbitType::TUNDRA: orbitType = Lite::CelestialUtilities::CanBodyBeTundra(targetBody) ? targetType : OrbitType::RANDOM; break;
        default: orbitType = targetType; break;
    }
}
// SatelliteContract.cs:825
bool SatelliteContract::PrestigeAppropriateOrbitType(OrbitType ot) {
    switch (ot) {
        default: return true;
        case OrbitType::KOLNIYA:
        case OrbitType::TUNDRA: return Prestige() >= ContractPrestige::Exceptional;
        case OrbitType::SYNCHRONOUS:
        case OrbitType::STATIONARY: return Prestige() >= ContractPrestige::Significant;
    }
}
// SatelliteContract.cs:840
int SatelliteContract::NetworkSatelliteCount(CelestialBody* body) {
    const int seed = (391 + ScenarioContractEvents::GameSeed()) * 23 + body->flightGlobalsIndex;
    const int minValue = M::Min(DI("Satellite", "", "NetworkMinimum"), DI("Satellite", "", "NetworkMaximum"));
    const int maxValue = M::Max(DI("Satellite", "", "NetworkMinimum"), DI("Satellite", "", "NetworkMaximum")) + 1;
    return KSPRandom(seed).Next(minValue, maxValue);
}
// SatelliteContract.cs:848
bool SatelliteContract::CheckSatelliteNetwork(int networkCount, std::vector<Vessel*>& possibleSatellites, OrbitType orbitType_, std::unique_ptr<Orbit>& orbit,
                                              double& stationaryLongitude, Vessel*& adjustVessel, Vessel* startVessel) {
    if (Lite::ActiveVesselFallback(startVessel) && networkCount > 1) {
        Orbit orbit2 = startVessel->loaded ? startVessel->orbit() : startVessel->protoVessel->orbitSnapShot->Load(FlightRegistry::Bodies());
        orbit = std::make_unique<Orbit>(orbit2.inclination, orbit2.eccentricity, orbit2.semiMajorAxis, orbit2.LAN, orbit2.argumentOfPeriapsis, orbit2.meanAnomalyAtEpoch,
                                        orbit2.epoch, orbit2.referenceBody);
        stationaryLongitude = orbit->referenceBody->GetLongitude(orbit->getPositionAtUT(Planetarium::GetUniversalTime()));
        const double num = 360.0 / static_cast<double>(networkCount);
        networkCount--;
        int num2 = networkCount;
        std::vector<Vessel*> list;
        std::vector<Vessel*> list2;
        do {
            if (num2-- > 0) {
                list.clear();
                list2.clear();
                if (orbitType_ == OrbitType::STATIONARY)
                    stationaryLongitude = std::fmod(stationaryLongitude + num, 360.0);
                else
                    orbit->LAN = std::fmod(orbit->LAN + num, 360.0);
                int count = static_cast<int>(possibleSatellites.size());
                while (count-- > 0) {
                    Vessel* vessel = possibleSatellites[static_cast<size_t>(count)];
                    if (vessel == startVessel) continue;
                    if (orbitType_ == OrbitType::STATIONARY) {
                        if (Lite::VesselAtOrbit(orbit.get(), DD("Satellite", "", "SignificantDeviation"), vessel)) {
                            Orbit orbit3 = vessel->loaded ? vessel->orbit() : vessel->protoVessel->orbitSnapShot->Load(FlightRegistry::Bodies());
                            const double longitude = orbit->referenceBody->GetLongitude(orbit3.getPositionAtUT(Planetarium::GetUniversalTime()));
                            double num3 = std::fmod(M::Abs(stationaryLongitude - longitude), 360.0);
                            if (num3 > 180.0) num3 = 360.0 - num3;
                            if (num3 <= DD("Satellite", "", "SignificantDeviation") / 100.0 * 360.0)
                                list2.push_back(vessel);
                            else if (num3 <= DD("Satellite", "", "TrivialDeviation") / 100.0 * 360.0)
                                list.push_back(vessel);
                        } else if (Lite::VesselAtOrbit(orbit.get(), DD("Satellite", "", "TrivialDeviation"), vessel)) {
                            list.push_back(vessel);
                        }
                    } else if (Lite::VesselAtOrbit(orbit.get(), DD("Satellite", "", "SignificantDeviation"), vessel)) {
                        list2.push_back(vessel);
                    } else if (Lite::VesselAtOrbit(orbit.get(), DD("Satellite", "", "TrivialDeviation"), vessel)) {
                        list.push_back(vessel);
                    }
                }
                continue;
            }
            return false;
        } while (!list2.empty());
        if (!list.empty()) adjustVessel = list[0];
        return true;
    }
    return false;
}

// ================================================================================================================================ CometSampleContract

namespace {
Vessel* FindVessel(uint32_t persistentId) {
    for (Vessel* v : FlightRegistry::Vessels)
        if (v->persistentId == persistentId) return v;
    return nullptr;
}
CelestialBody* HomeBody() { return Planetarium::fetch->Home; }
double HomeSma() { return BodyOrbit(HomeBody())->semiMajorAxis; }
std::string CometTypeName(Vessel& v) { return Lite::FinePrintEnvironment::cometTypeName ? Lite::FinePrintEnvironment::cometTypeName(v) : std::string(); }
}  // namespace

// CometSampleContract.cs:227
bool CometSampleContract::ValidCometAvailable() {
    std::vector<Vessel*> discoveredComets = CometManagerSeam::discoveredComets ? CometManagerSeam::discoveredComets() : std::vector<Vessel*>();
    if (discoveredComets.empty()) return false;
    const int index = UnityRandom::Global().Range(0, static_cast<int32_t>(discoveredComets.size()));
    auto currentContracts = ContractSystem::Instance->GetCurrentContracts<CometSampleContract>();
    for (size_t num = 0; num < currentContracts.size(); ++num) {
        if (currentContracts[num]->CometPersistentId() == discoveredComets[static_cast<size_t>(index)]->persistentId) {
            log::Info("Comet " + discoveredComets[static_cast<size_t>(index)]->gameObjectName + "already assigned.");
            return false;
        }
    }
    auto completedContracts = ContractSystem::Instance->GetCompletedContracts<CometSampleContract>();
    for (size_t num2 = 0; num2 < completedContracts.size(); ++num2) {
        if (completedContracts[num2]->CometPersistentId() == discoveredComets[static_cast<size_t>(index)]->persistentId) {
            log::Info("Comet " + discoveredComets[static_cast<size_t>(index)]->gameObjectName + "already completed.");
            return false;
        }
    }
    scienceSubjectId = "cometSample_" + CometTypeName(*discoveredComets[static_cast<size_t>(index)]) + "@SunInSpaceHigh_" + discoveredComets[static_cast<size_t>(index)]->vesselName;
    std::shared_ptr<ScienceSubject> subjectByID = ResearchAndDevelopment::GetSubjectByID(scienceSubjectId);
    if (subjectByID != nullptr && subjectByID->science > 0.0f) return false;
    cometPersistentId = discoveredComets[static_cast<size_t>(index)]->persistentId;
    cometName = discoveredComets[static_cast<size_t>(index)]->vesselName;
    return true;
}
// CometSampleContract.cs:27
bool CometSampleContract::Generate() {
    if (!ValidCometAvailable()) return false;
    int num = 0;
    int num2 = 0;
    auto currentContracts = ContractSystem::Instance->GetCurrentContracts<CometSampleContract>();
    int num3 = static_cast<int>(currentContracts.size());
    while (num3-- > 0) {
        switch (currentContracts[static_cast<size_t>(num3)]->ContractState()) {
            case State::Active: num2++; break;
            case State::Offered: num++; break;
            default: break;
        }
    }
    if (num < DI("CometSample", "", "MaximumAvailable") && num2 < DI("CometSample", "", "MaximumActive")) {
        const float num4 = 1.0f;
        AddParameter(std::make_unique<CrewTraitParameter>("Scientist", 1, Fmt("#autoLOC_8004217")));
        AddParameter(std::make_unique<CometScienceParameter>(cometPersistentId, cometName, scienceSubjectId));
        AddKeywords({"Scientific"});
        Vessel* vessel = FindVessel(cometPersistentId);
        if (vessel != nullptr) {
            if (vessel->orbit().ApR() > 0.0 && vessel->orbit().ApR() < HomeSma() * 15.0)
                prestige = ContractPrestige::Significant;
            else
                prestige = ContractPrestige::Exceptional;
        }
        SetExpiry(DI("CometSample", "Expiration", "MinimumExpireDays"), DI("CometSample", "Expiration", "MaximumExpireDays"));
        SetDeadlineDays(Mul(static_cast<float>(DI("CometSample", "Expiration", "DeadlineDays")), num4));
        const char* sub = prestige == ContractPrestige::Exceptional ? "Exceptional" : (prestige == ContractPrestige::Significant ? "Significant" : "Trivial");
        auto m3 = [&](float a, float b, float c) { return f32store(fx(a) * fx(b) * fx(c)); };
        SetScience(static_cast<float>(Mathf::RoundToInt(m3(DF("CometSample", "Science", "BaseReward"), num4, DF("CometSample", sub, "ScienceMultiplier")))));
        SetReputation(static_cast<float>(Mathf::RoundToInt(m3(DF("CometSample", "Reputation", "BaseReward"), num4, DF("CometSample", sub, "ReputationMultiplier")))),
                      static_cast<float>(Mathf::RoundToInt(m3(DF("CometSample", "Reputation", "BaseFailure"), num4, DF("CometSample", sub, "ReputationMultiplier")))));
        SetFunds(static_cast<float>(Mathf::RoundToInt(m3(DF("CometSample", "Funds", "BaseAdvance"), num4, DF("CometSample", sub, "FundsMultiplier")))),
                 static_cast<float>(Mathf::RoundToInt(m3(DF("CometSample", "Funds", "BaseReward"), num4, DF("CometSample", sub, "FundsMultiplier")))),
                 static_cast<float>(Mathf::RoundToInt(m3(DF("CometSample", "Funds", "BaseFailure"), num4, DF("CometSample", sub, "FundsMultiplier")))));
        return true;
    }
    return false;
}
// CometSampleContract.cs:93
void CometSampleContract::OnRegister() {
    GE::onVesselPersistentIdChanged.Add(this, &CometSampleContract::VesselIdChanged);
    GE::onVesselRename.Add(this, &CometSampleContract::OnVesselRename);
    GE::OnDiscoverableObjectExpired.Add(this, &CometSampleContract::OnDiscoverableObjectExpired);
    scienceParameter = dynamic_cast<CometScienceParameter*>(GetParameterOfType("CometScienceParameter"));
}
// CometSampleContract.cs:101
void CometSampleContract::OnUnregister() {
    GE::onVesselPersistentIdChanged.Remove(this, &CometSampleContract::VesselIdChanged);
    GE::onVesselRename.Remove(this, &CometSampleContract::OnVesselRename);
    GE::OnDiscoverableObjectExpired.Remove(this, &CometSampleContract::OnDiscoverableObjectExpired);
}
// CometSampleContract.cs:108
void CometSampleContract::OnVesselRename(GE::HostedFromToAction<Vessel*, std::string> hostedFromTo) {
    if (hostedFromTo.host != nullptr) {
        if (scienceParameter == nullptr) scienceParameter = dynamic_cast<CometScienceParameter*>(GetParameterOfType("CometScienceParameter"));
        if ((scienceParameter == nullptr || !scienceParameter->ScienceDataCollected()) && cometPersistentId == hostedFromTo.host->persistentId) {
            cometName = hostedFromTo.to;
            scienceSubjectId = "cometSample_" + CometTypeName(*hostedFromTo.host) + "@SunInSpaceHigh_" + cometName;
        }
    }
}
// CometSampleContract.cs:124
void CometSampleContract::OnDiscoverableObjectExpired(Vessel* v) {
    if (scienceParameter == nullptr) scienceParameter = dynamic_cast<CometScienceParameter*>(GetParameterOfType("CometScienceParameter"));
    if (scienceParameter == nullptr || !scienceParameter->ScienceDataCollected()) {
        Vessel* vessel = FindVessel(cometPersistentId);
        if (v == vessel) Fail();
    }
}
// CometSampleContract.cs:140
void CometSampleContract::VesselIdChanged(uint32_t oldId, uint32_t newId) {
    if (cometPersistentId == oldId) cometPersistentId = newId;
}
// CometSampleContract.cs:158
std::string CometSampleContract::GetHashString() { return I(Lite::SuperSeed(*this)); }
// CometSampleContract.cs:163
std::string CometSampleContract::GetTitle() { return Fmt("#autoLOC_8012043", {cometName}); }
// CometSampleContract.cs:168
std::string CometSampleContract::GetDescription() {
    return TextGen::GenerateBackStories("CometSample", Agent()->Title(), cometName, Fmt("#autoLOC_6002299"), MissionSeed(), false, false, false);
}
// CometSampleContract.cs:173
std::string CometSampleContract::GetSynopsys() {
    Vessel* vessel = FindVessel(cometPersistentId);
    if (vessel != nullptr) {
        if (vessel->orbit().semiMajorAxis < 0.0) return Fmt("#autoLOC_8012044", {cometName, Fmt("#autoLOC_8012049")});
        if (vessel->orbit().ApR() < HomeSma() * 6.0) return Fmt("#autoLOC_8012044", {cometName, Fmt("#autoLOC_8012047")});
        if (vessel->orbit().ApR() > HomeSma() * 6.0 && vessel->orbit().ApR() < HomeSma() * 15.0) return Fmt("#autoLOC_8012044", {cometName, Fmt("#autoLOC_8012048")});
        if (vessel->orbit().ApR() > HomeSma() * 15.0) return Fmt("#autoLOC_8012044", {cometName, Fmt("#autoLOC_8012049")});
    }
    return Fmt("#autoLOC_8012044", {cometName});
}
// CometSampleContract.cs:199
std::string CometSampleContract::MessageCompleted() { return Fmt("#autoLOC_8012046", {cometName}); }
// CometSampleContract.cs:204
void CometSampleContract::OnLoad(ConfigNode& node) {
    node.TryGetValue("cometName", cometName);
    node.TryGetValue("cometPersistentId", cometPersistentId);
    node.TryGetValue("scienceSubjectId", scienceSubjectId);
}
// CometSampleContract.cs:211
void CometSampleContract::OnSave(ConfigNode& node) {
    node.AddValue("cometName", cometName);
    node.AddValue("cometPersistentId", cometPersistentId);
    node.AddValue("scienceSubjectId", scienceSubjectId);
}
// CometSampleContract.cs:218
bool CometSampleContract::MeetRequirements() {
    if (PU::GetBodiesProgress(ProgressType::LANDING, true, true, MannedStatus::ANY, [](CelestialBody* cb) { return cb != Home(); }).size() == 0) return false;
    return true;
}

// ================================================================================================================================ TourismContract

// TourismContract.cs:26 TouristCount
int TourismContract::TouristCount() const { return !tourists.empty() ? static_cast<int>(tourists.size()) : 0; }
// TourismContract.cs:40 FirstTourist
std::string TourismContract::FirstTourist() const { return !tourists.empty() ? tourists[0] : std::string(); }
// TourismContract.cs:52 TouristNamesJoined
std::string TourismContract::TouristNamesJoined() const {
    std::vector<std::string> list;
    for (const std::string& t : tourists) list.push_back(Lite::StringUtilities::ShortKerbalName(t));
    return Lite::StringUtilities::ThisThisAndThat(list);
}
// TourismContract.cs:70
std::shared_ptr<ProtoCrewMember> TourismContract::GetKerbal(const std::string& name) {
    Game* game = SaveEnvironment::Current().currentGame;
    if (game->CrewRoster->Exists(name)) return (*game->CrewRoster)[name];
    return nullptr;
}
// TourismContract.cs:606
void TourismContract::GetContractSubject() {
    if (tourists.size() > 1) {
        contractSubject = "^p";
        return;
    }
    std::shared_ptr<ProtoCrewMember> kerbal = GetKerbal(FirstTourist());
    if (kerbal != nullptr)
        contractSubject = kerbal->gender() == ProtoCrewMember_Gender::Male ? "^m" : "^f";
    else
        contractSubject = "^m";
}
// TourismContract.cs:473
std::vector<CelestialBody*> TourismContract::RandomReachedBodyGrouping(const std::vector<CelestialBody*>& reachedBodies, KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(MissionSeed());
        generator = own.get();
    }
    std::vector<CelestialBody*> list;
    int count = static_cast<int>(reachedBodies.size());
    while (count-- > 0) {
        CelestialBody* b = reachedBodies[static_cast<size_t>(count)];
        if (b->referenceBody() != nullptr && b->referenceBody() == Sun()) list.push_back(b);
    }
    if (list.empty()) return list;
    CelestialBody* celestialBody = WeightedBodyChoice(list, generator);
    std::vector<CelestialBody*> list2{celestialBody};
    int count2 = static_cast<int>(celestialBody->orbitingBodies.size());
    while (count2-- > 0) {
        CelestialBody* item = celestialBody->orbitingBodies[static_cast<size_t>(count2)];
        if (Has(reachedBodies, item)) list2.push_back(item);
    }
    return list2;
}
// TourismContract.cs:413
std::vector<CelestialBody*> TourismContract::PossibleBodies(KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(MissionSeed());
        generator = own.get();
    }
    std::vector<CelestialBody*> list{Home()};
    for (CelestialBody* b : PU::GetBodiesReached(true, MannedStatus::MANNED, [](CelestialBody* cb) { return cb != Home(); })) list.push_back(b);
    std::vector<CelestialBody*> list2;
    switch (prestige) {
        default: list2.insert(list2.end(), list.begin(), list.end()); break;
        case ContractPrestige::Significant: {
            auto g = RandomReachedBodyGrouping(list, generator);
            list2.insert(list2.end(), g.begin(), g.end());
            break;
        }
        case ContractPrestige::Trivial: list2.push_back(WeightedBodyChoice(list, generator)); break;
    }
    return list2;
}
// TourismContract.cs:437
std::vector<FlightLog_EntryType> TourismContract::PossibleSituations(CelestialBody* body) {
    std::vector<FlightLog_EntryType> list;
    if (body == Home()) {
        if (PU::GetBodyProgress(ProgressType::REACHSPACE, body, MannedStatus::MANNED)) list.push_back(FlightLog_EntryType::Suborbit);
        if (PU::GetBodyProgress(ProgressType::ORBIT, body, MannedStatus::MANNED)) list.push_back(FlightLog_EntryType::Orbit);
    } else {
        if (PU::GetBodyProgress(ProgressType::FLYBY, body, MannedStatus::MANNED)) list.push_back(FlightLog_EntryType::Flyby);
        if (PU::GetBodyProgress(ProgressType::ORBIT, body, MannedStatus::MANNED)) list.push_back(FlightLog_EntryType::Orbit);
        if (!Lite::CelestialUtilities::IsGasGiant(body) &&
            (PU::GetBodyProgress(ProgressType::LANDING, body, MannedStatus::MANNED) || PU::GetBodyProgress(ProgressType::SPLASHDOWN, body, MannedStatus::MANNED)))
            list.push_back(FlightLog_EntryType::Land);
        if (Lite::CelestialUtilities::IsFlyablePlanet(body) && PU::GetBodyProgress(ProgressType::FLIGHT, body, MannedStatus::MANNED)) list.push_back(FlightLog_EntryType::Flight);
    }
    return list;
}
// TourismContract.cs:379
std::vector<TourismContract::Itinerary> TourismContract::BuildPossibleItineraries(KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(MissionSeed());
        generator = own.get();
    }
    std::vector<Itinerary> list;
    std::vector<CelestialBody*> list2 = PossibleBodies(generator);
    if (!list2.empty()) {
        if (Prestige() <= ContractPrestige::Significant)
            preposition = Fmt("#autoLOC_7000038", {I(!list2[0]->isHomeWorld ? 1 : 0), list2[0]->displayName()});
        else
            preposition = Fmt("#autoLOC_282101");
        int count = static_cast<int>(list2.size());
        for (int i = 0; i < count; i++) {
            CelestialBody* celestialBody = list2[static_cast<size_t>(i)];
            std::vector<FlightLog_EntryType> list3 = PossibleSituations(celestialBody);
            for (FlightLog_EntryType t : list3) list.emplace_back(celestialBody, t);
        }
        return list;
    }
    return list;
}
// TourismContract.cs:506
TourismContract::Itinerary TourismContract::WeightedItineraryChoice(const std::vector<Itinerary>& itineraryChoices, KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(ksp::Contracts::ContractsEnvironment::Current().NewRandomSeed());
        generator = own.get();
    }
    std::vector<Itinerary> list(itineraryChoices);
    std::vector<CelestialBody*> list2;
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        CelestialBody* key = list[static_cast<size_t>(count)].first;
        if (!Has(list2, key)) list2.push_back(key);
    }
    CelestialBody* celestialBody = WeightedBodyChoice(list2, generator);
    int count2 = static_cast<int>(list.size());
    while (count2-- > 0) {
        Itinerary item = list[static_cast<size_t>(count2)];
        if (item.first != celestialBody) list.erase(std::find(list.begin(), list.end(), item));
    }
    return list[static_cast<size_t>(generator->Next(0, static_cast<int>(list.size())))];
}
// TourismContract.cs:79
bool TourismContract::Generate() {
    int num = 0;
    int num2 = 0;
    auto currentContracts = ContractSystem::Instance->GetCurrentContracts<TourismContract>();
    int num3 = static_cast<int>(currentContracts.size());
    while (num3-- > 0) {
        switch (currentContracts[static_cast<size_t>(num3)]->ContractState()) {
            case State::Active: num2++; break;
            case State::Offered: num++; break;
            default: break;
        }
    }
    if (num < DI("Tour", "", "MaximumAvailable") && num2 < DI("Tour", "", "MaximumActive")) {
        KSPRandom kSPRandom(Lite::SuperSeed(*this));
        tourists.clear();
        int num4 = 0;
        switch (prestige) {
            case ContractPrestige::Trivial: num4 = kSPRandom.Next(1, 3); break;
            case ContractPrestige::Significant: num4 = kSPRandom.Next(2, 5); break;
            case ContractPrestige::Exceptional: num4 = kSPRandom.Next(3, 7); break;
        }
        Game* game = SaveEnvironment::Current().currentGame;
        isGeeAdventure = game->Parameters->CustomParams<GameParameters::AdvancedParams>().GKerbalLimits && kSPRandom.NextDouble() < static_cast<double>(DF("Tour", "", "GeeAdventureChance"));
        CelestialBody* celestialBody = Home();
        if (!isGeeAdventure) {
            std::vector<Itinerary> list = BuildPossibleItineraries(&kSPRandom);
            if (list.empty()) return false;
            homeDestinations = true;
            for (int i = 0; i < num4; i++) {
                std::shared_ptr<ProtoCrewMember> protoCrewMember;
                bool flag = false;
                while (!flag) {
                    protoCrewMember = game->CrewRoster->GetNewKerbal(ProtoCrewMember_KerbalType::Unowned);
                    if (!Lite::SystemUtilities::CheckTouristRecoveryContractKerbals(CrewGenerator::RemoveLastName(protoCrewMember->name().value_or("")))) flag = true;
                }
                protoCrewMember->set_rosterStatus(ProtoCrewMember_RosterStatus::Assigned);
                protoCrewMember->trait = KerbalRoster::touristTrait;
                protoCrewMember->seatIdx = -1;
                tourists.push_back(protoCrewMember->name().value_or(""));
                int val = 0;
                switch (prestige) {
                    case ContractPrestige::Trivial: val = kSPRandom.Next(1, 3); break;
                    case ContractPrestige::Significant: val = kSPRandom.Next(2, 4); break;
                    case ContractPrestige::Exceptional: val = kSPRandom.Next(3, 5); break;
                }
                std::vector<Itinerary> list2(list);
                std::vector<Itinerary> list3;
                val = M::Min(val, static_cast<int>(list2.size()));
                if (val != 0) {
                    for (int j = 0; j < val; j++) {
                        Itinerary item = WeightedItineraryChoice(list2, &kSPRandom);
                        list3.push_back(item);
                        list2.erase(std::find(list2.begin(), list2.end(), item));
                    }
                    ContractParameter* contractParameter =
                        AddParameter(std::make_unique<KerbalTourParameter>(protoCrewMember->name().value_or(""), protoCrewMember->gender()));
                    CelestialBody* celestialBody2 = Home();
                    int count = static_cast<int>(list3.size());
                    for (int k = 0; k < count; k++) {
                        Itinerary item = list3[static_cast<size_t>(k)];
                        if (item.first->scienceValues.RecoveryValue >= celestialBody2->scienceValues.RecoveryValue) celestialBody2 = item.first;
                        if (item.first->scienceValues.RecoveryValue >= celestialBody->scienceValues.RecoveryValue) celestialBody = item.first;
                        if (homeDestinations && !item.first->isHomeWorld) homeDestinations = false;
                        contractParameter->AddParameter(std::make_unique<KerbalDestinationParameter>(item.first, item.second, protoCrewMember->name().value_or("")));
                    }
                    const float num5 = Mul(DF("Tour", "Funds", "DefaultFare"), static_cast<float>(contractParameter->ParameterCount()));
                    const float completion = f32store(fx(num5) * 0.25 / static_cast<double>(contractParameter->ParameterCount()));
                    const float completion2 = Mul(num5, 0.75f);
                    std::vector<ContractParameter*> all;
                    contractParameter->AllParameters(all);
                    for (ContractParameter* p : all)
                        if (p != nullptr) p->SetFunds(completion, celestialBody2);
                    contractParameter->SetFunds(completion2, celestialBody2);
                    continue;
                }
                return false;
            }
        } else {
            for (int l = 0; l < num4; l++) {
                std::shared_ptr<ProtoCrewMember> newKerbal = game->CrewRoster->GetNewKerbal(ProtoCrewMember_KerbalType::Unowned);
                newKerbal->set_rosterStatus(ProtoCrewMember_RosterStatus::Assigned);
                newKerbal->trait = KerbalRoster::touristTrait;
                newKerbal->seatIdx = -1;
                tourists.push_back(newKerbal->name().value_or(""));
                ContractParameter* contractParameter2 = AddParameter(std::make_unique<KerbalGeeAdventureParameter>(newKerbal->name().value_or("")));
                contractParameter2->AddParameter(std::make_unique<ReachDestination>(Home(), ""))->SetDisableOnStateChange(false);
                const Vessel::Situations sit = prestige == ContractPrestige::Trivial
                                                   ? Vessel::Situations::FLYING
                                                   : (prestige == ContractPrestige::Significant ? Vessel::Situations::SUB_ORBITAL : Vessel::Situations::ORBITING);
                contractParameter2->AddParameter(std::make_unique<ReachSituation>(sit, ""))->SetDisableOnStateChange(false);
                contractParameter2->SetFunds(DF("Tour", "Funds", "DefaultFare"), Home());
            }
        }
        GetContractSubject();
        SetExpiry(DI("Tour", "Expiration", "MinimumExpireDays"), DI("Tour", "Expiration", "MaximumExpireDays"));
        SetDeadlineDays(static_cast<float>(DI("Tour", "Expiration", "DeadlineDays")), celestialBody);
        SetScience(DF("Tour", "Science", "BaseReward"));
        SetReputation(DF("Tour", "Reputation", "BaseReward"), DF("Tour", "Reputation", "BaseFailure"));
        return true;
    }
    return false;
}
// TourismContract.cs:236
std::string TourismContract::GetHashString() { return I(Lite::SuperSeed(*this)); }
// TourismContract.cs:241
std::string TourismContract::GetTitle() {
    if (isGeeAdventure) return Fmt("#autoLOC_7000002", {I(TouristCount())});
    return Fmt("#autoLOC_7000003", {I(TouristCount()), contractSubject, I(SingleDestination() ? 1 : 0), I(homeDestinations ? 1 : 0)});
}
// TourismContract.cs:250
std::string TourismContract::GetDescription() {
    const std::string fullKerbalName = TouristCount() <= 0 ? "#autoLOC_7000019" : FirstTourist();
    return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), Lite::StringUtilities::ShortKerbalName(fullKerbalName), "#autoLOC_7000020", MissionSeed(), true, true, true);
}
// TourismContract.cs:256
std::string TourismContract::GetSynopsys() {
    const std::string text = TouristCount() > 1 ? TouristNamesJoined() : FirstTourist();
    if (isGeeAdventure) return Fmt("#autoLOC_7000000", {text, I(TouristCount()), contractSubject});
    return Fmt("#autoLOC_7000001", {text, I(TouristCount()), preposition, contractSubject});
}
// TourismContract.cs:266
std::string TourismContract::GetNotes() {
    if (isGeeAdventure) return "";
    std::string text;
    text += Fmt("#autoLOC_7000004", {I(TouristCount()), contractSubject, Home()->displayName()});
    Game* game = SaveEnvironment::Current().currentGame;
    if (game->Parameters->CustomParams<GameParameters::AdvancedParams>().GKerbalLimits && DB("Tour", "", "FailOnInactive")) text += Fmt("#autoLOC_281970");
    if (Scene() == GameScenes::SPACECENTER) text += "\n";
    return text;
}
// TourismContract.cs:285
std::string TourismContract::MessageCompleted() {
    const std::string text = TouristCount() > 1 ? TouristNamesJoined() : FirstTourist();
    if (isGeeAdventure) return Fmt("#autoLOC_7000005", {text, contractSubject});
    return Fmt("#autoLOC_7000006", {text});
}
// TourismContract.cs:295
std::string TourismContract::MessageFailed() { return Fmt("#autoLOC_7000007", {I(static_cast<int>(tourists.size()))}); }
// TourismContract.cs:300
std::string TourismContract::MessageCancelled() {
    const std::string text = TouristCount() > 1 ? TouristNamesJoined() : FirstTourist();
    return Fmt("#autoLOC_7003400", {text, contractSubject});
}
// TourismContract.cs:306
std::string TourismContract::MessageDeadlineExpired() {
    const std::string text = TouristCount() > 1 ? TouristNamesJoined() : FirstTourist();
    return Fmt("#autoLOC_7000008", {text, contractSubject});
}
// TourismContract.cs:312
void TourismContract::OnSave(ConfigNode& node) {
    if (!preposition.empty()) node.AddValue("preposition", preposition);
    node.AddValue("homeDestinations", homeDestinations);
    node.AddValue("isGeeAdventure", isGeeAdventure);
    Lite::SystemUtilities::SaveNodeList(&node, "tourists", tourists);
}
// TourismContract.cs:323
void TourismContract::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "TourismContract", "preposition", preposition, std::string("of the solar system"));
    Lite::LoadNode(node, "TourismContract", "homeDestinations", homeDestinations, false);
    Lite::LoadNode(node, "TourismContract", "isGeeAdventure", isGeeAdventure, false);
    Lite::SystemUtilities::LoadNodeList(&node, "TourismContract", "tourists", tourists);
    GetContractSubject();
}
// TourismContract.cs:332
bool TourismContract::MeetRequirements() {
    if (!ProgressTracking::Instance()->NodeComplete({"ReachedSpace"})) return false;
    return true;
}
// TourismContract.cs:341
std::vector<CelestialBody*> TourismContract::WeightBodies() {
    std::vector<CelestialBody*> list;
    std::vector<ContractParameter*> all;
    AllParameters(all);
    for (ContractParameter* current : all) {
        if (dynamic_cast<KerbalTourParameter*>(current) == nullptr) continue;
        std::vector<ContractParameter*> sub;
        current->AllParameters(sub);
        for (ContractParameter* q : sub) {
            auto* d = dynamic_cast<KerbalDestinationParameter*>(q);
            if (d != nullptr && !Has(list, d->targetBody)) list.push_back(d->targetBody);
        }
    }
    return list;
}
// TourismContract.cs:364
void TourismContract::OnAccepted() {
    int count = static_cast<int>(tourists.size());
    while (count-- > 0) {
        std::shared_ptr<ProtoCrewMember> kerbal = GetKerbal(tourists[static_cast<size_t>(count)]);
        if (kerbal != nullptr) {
            kerbal->set_type(ProtoCrewMember_KerbalType::Tourist);
            KerbalRoster::SetExperienceTrait(*kerbal, KerbalRoster::touristTrait);
            kerbal->set_rosterStatus(ProtoCrewMember_RosterStatus::Available);
        }
    }
}
// TourismContract.cs:533
void TourismContract::ClearKerbalsSoft() {
    int count = static_cast<int>(tourists.size());
    while (count-- > 0) {
        std::shared_ptr<ProtoCrewMember> kerbal = GetKerbal(tourists[static_cast<size_t>(count)]);
        if (kerbal != nullptr) {
            if (kerbal->type() == ProtoCrewMember_KerbalType::Tourist && kerbal->rosterStatus() == ProtoCrewMember_RosterStatus::Assigned)
                kerbal->hasToured = true;
            else
                SaveEnvironment::Current().currentGame->CrewRoster->Remove(kerbal->name().value_or(""));
        }
    }
}
// TourismContract.cs:553 / :623 / :628 / :633
void TourismContract::OnCancelled() { ClearKerbalsSoft(); }
void TourismContract::OnDeadlineExpired() { ClearKerbalsSoft(); }
void TourismContract::OnFailed() { ClearKerbalsSoft(); }
void TourismContract::OnFinished() { ClearKerbalsSoft(); }
// TourismContract.cs:573
void TourismContract::ClearKerbalsHard() {
    int count = static_cast<int>(tourists.size());
    while (count-- > 0) {
        std::shared_ptr<ProtoCrewMember> kerbal = GetKerbal(tourists[static_cast<size_t>(count)]);
        if (kerbal != nullptr) SaveEnvironment::Current().currentGame->CrewRoster->Remove(kerbal->name().value_or(""));
    }
}
// TourismContract.cs:586 / :656 / :661 / :666
void TourismContract::OnGenerateFailed() { ClearKerbalsHard(); }
void TourismContract::OnWithdrawn() { ClearKerbalsHard(); }
void TourismContract::OnDeclined() { ClearKerbalsHard(); }
void TourismContract::OnOfferExpired() { ClearKerbalsHard(); }
// TourismContract.cs:624
std::string TourismContract::Pluralize(const std::string& single, const std::string& plural) {
    if (TouristCount() != 1) return plural;
    return single;
}

}  // namespace ksp::FinePrint::Contracts
