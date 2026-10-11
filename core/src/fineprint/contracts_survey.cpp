// WP-38 slice 2: FinePrint/Contracts/SurveyContract.cs.
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
#include "ksp/modules/science_modules.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/science/science.hpp"
#include "ksp/space_center/space_center.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"
#include "param_common.hpp"

namespace ksp::FinePrint::Contracts {

using namespace ksp::fpcommon;
using ksp::Contracts::ContractDefsLite;
using ksp::Contracts::ContractSystem;
using ksp::Contracts::TextGen;
using ksp::Contracts::Parameters::SurveyWaypointParameter;
using Lite::MannedStatus;
using Lite::SurveyDefinition;
using Lite::SurveyDefinitionParameter;
using Lite::WaypointClusterState;
namespace PU = Lite::ProgressUtilities;
namespace M = ksp::dotnet::Math;

namespace {
float DF(const char* sec, const char* sub, const char* key) { return ContractDefsLite::F(sec, sub, key); }
double DD(const char* sec, const char* sub, const char* key) { return ContractDefsLite::D(sec, sub, key); }
int DI(const char* sec, const char* sub, const char* key) { return ContractDefsLite::I(sec, sub, key); }
float Round(float f) { return Mathf::Round(f); }
CelestialBody* Home() { return Planetarium::fetch->Home; }
bool Has(const std::vector<std::string>& l, const std::string& s) { return std::find(l.begin(), l.end(), s) != l.end(); }
}  // namespace

// SurveyContract.cs:873
double SurveyContract::ParamRange(Contract::ContractPrestige prestige, CelestialBody* body, FlightBand band) {
    double num = 10000.0;
    switch (prestige) {
        case ContractPrestige::Trivial: num = DD("Survey", "", "TrivialRange"); break;
        case ContractPrestige::Significant: num = DD("Survey", "", "SignificantRange"); break;
        case ContractPrestige::Exceptional: num = DD("Survey", "", "ExceptionalRange"); break;
    }
    double result = 2000.0;
    switch (band) {
        case FlightBand::GROUND:
            result = num / 2.0;
            result += result * body->GeeASL;
            break;
        case FlightBand::LOW: result = num * 25.0; break;
        case FlightBand::HIGH: result = num * 50.0; break;
        default: break;
    }
    return result;
}

// SurveyContract.cs:905
void SurveyContract::RandomizeNearKSC(Contract::ContractPrestige prestigeLevel, bool allowWater, KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(MissionSeed());
        generator = own.get();
    }
    CelestialBody* celestialBody = SpaceCenter::Instance == nullptr ? CareerEnvironment::Current().HomeBody() : SpaceCenter::Instance->cb;
    Waypoint waypoint;
    waypoint.celestialName = celestialBody->name();
    waypoint.latitude = SpaceCenter::Instance == nullptr ? 0.0 : SpaceCenter::Instance->Latitude();
    waypoint.longitude = SpaceCenter::Instance == nullptr ? 0.0 : SpaceCenter::Instance->Longitude();
    FlightBand band;
    switch (targetBand) {
        default: band = FlightBand::GROUND; break;
        case FlightBand::LOW:
        case FlightBand::LOWMIX: band = FlightBand::LOW; break;
        case FlightBand::HIGH:
        case FlightBand::HIGHMIX:
        case FlightBand::ANYMIX: band = FlightBand::HIGH; break;
    }
    const double num = ParamRange(prestigeLevel, celestialBody, band);
    const double num2 = SurveyWaypointParameter::TriggerRange(band);
    const double num3 = focusedSurvey ? num2 : (num2 + num);
    const double maximumDistance = focusedSurvey ? (num2 + num) : (num3 + 1.0);
    waypoint.RandomizeNear(waypoint.latitude, waypoint.longitude, num3, maximumDistance, allowWater, generator);
    centerLatitude = waypoint.latitude;
    centerLongitude = waypoint.longitude;
}

// SurveyContract.cs:792
std::vector<std::unique_ptr<SurveyDefinition>> SurveyContract::PossibleDefinitions() {
    std::vector<std::unique_ptr<SurveyDefinition>> surveyDefinitions = Lite::SurveyDefinitions();
    int count = static_cast<int>(surveyDefinitions.size());
    while (count-- > 0) {
        SurveyDefinition& surveyDefinition = *surveyDefinitions[static_cast<size_t>(count)];
        int count2 = static_cast<int>(surveyDefinition.Parameters.size());
        while (count2-- > 0) {
            SurveyDefinitionParameter& p = *surveyDefinition.Parameters[static_cast<size_t>(count2)];
            bool flag = true;
            ScienceExperiment* experiment = ResearchAndDevelopment::GetExperiment(p.Experiment);
            if (experiment != nullptr) {
                if (!experiment->IsUnlocked()) flag = false;
            } else {
                flag = false;
            }
            if (p.CrewRequired && !PU::GetAnyBodyProgress(targetBody, MannedStatus::MANNED)) flag = false;
            if (p.EVARequired && !GameVariables::Instance->UnlockedEVA(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::AstronautComplex))) {
                if (targetBody == Home()) {
                    if (!p.AllowGround) flag = false;
                } else {
                    flag = false;
                }
            }
            if (!targetBody->atmosphere && !p.AllowVacuum) flag = false;
            if (focusedSurvey && !p.AllowLow && !p.AllowHigh) flag = false;
            if (contextual && !p.AllowGround && !p.AllowLow) flag = false;
            if (Lite::CelestialUtilities::IsGasGiant(targetBody) && !p.AllowHigh) flag = false;
            int count3 = static_cast<int>(p.Tech.size());
            while (count3-- > 0) {
                const std::string text = p.Tech[static_cast<size_t>(count3)];
                if (!text.empty() && !PU::HavePartTech(text)) {
                    flag = false;
                    break;
                }
            }
            if (!flag) surveyDefinition.Parameters.erase(surveyDefinition.Parameters.begin() + count2);
        }
        if (surveyDefinition.Parameters.empty()) surveyDefinitions.erase(surveyDefinitions.begin() + count);
    }
    return surveyDefinitions;
}

// SurveyContract.cs:676
std::vector<std::pair<Vessel*, std::vector<std::string>>> SurveyContract::CachePossibleVessels(CelestialBody* body,
                                                                                               std::vector<std::unique_ptr<SurveyDefinition>>& originalDefinitions) {
    std::vector<std::pair<Vessel*, std::vector<std::string>>> list;
    std::vector<std::string> list2;
    std::vector<std::string> list3;
    int count = static_cast<int>(originalDefinitions.size());
    while (count-- > 0) {
        SurveyDefinition& surveyDefinition = *originalDefinitions[static_cast<size_t>(count)];
        int count2 = static_cast<int>(surveyDefinition.Parameters.size());
        while (count2-- > 0) {
            SurveyDefinitionParameter& p = *surveyDefinition.Parameters[static_cast<size_t>(count2)];
            if (!p.Experiment.empty() && !Has(list2, p.Experiment)) {
                list2.push_back(p.Experiment);
                if (p.EVARequired && !Has(list3, p.Experiment)) list3.push_back(p.Experiment);
            }
        }
    }
    if (list2.empty()) return list;
    int count3 = static_cast<int>(FlightRegistry::Vessels.size());
    while (count3-- > 0) {
        Vessel* vessel = FlightRegistry::Vessels[static_cast<size_t>(count3)];
        if (SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT && FlightRegistry::ready && vessel == FlightRegistry::ActiveVessel) continue;
        if (vessel->loaded) {
            if (static_cast<int>(vessel->vesselType) <= static_cast<int>(VesselType::SpaceObject) || vessel->vesselType == VesselType::DeployedScienceController ||
                vessel->vesselType == VesselType::DeployedSciencePart || !vessel->Landed || !vessel->IsControllable() || vessel->orbit().referenceBody != body)
                continue;
        } else if (static_cast<int>(vessel->protoVessel->vesselType) <= static_cast<int>(VesselType::SpaceObject) ||
                   vessel->vesselType == VesselType::DeployedScienceController || vessel->vesselType == VesselType::DeployedSciencePart ||
                   (vessel->protoVessel->situation != Vessel::Situations::LANDED && vessel->protoVessel->situation != Vessel::Situations::PRELAUNCH) ||
                   !vessel->protoVessel->wasControllable ||
                   FlightRegistry::Bodies().at(static_cast<size_t>(vessel->protoVessel->orbitSnapShot->ReferenceBodyIndex)) != body) {
            continue;
        }
        if (!Lite::VesselIsOwned(vessel)) continue;
        std::vector<std::string> list4;
        if (!vessel->GetVesselCrew().empty()) list4.insert(list4.end(), list3.begin(), list3.end());
        if (vessel->loaded) {
            int count4 = static_cast<int>(vessel->Parts().size());
            while (count4-- > 0) {
                Part* part = vessel->Parts()[static_cast<size_t>(count4)];
                int count5 = static_cast<int>(part->Modules().Count());
                while (count5-- > 0) {
                    auto* m = dynamic_cast<ModuleScienceExperiment*>(part->Modules()[static_cast<size_t>(count5)]);
                    if (m != nullptr && m->experiment != nullptr) list4.push_back(m->experiment->id.value_or(""));
                }
            }
        } else {
            int count6 = static_cast<int>(vessel->protoVessel->protoPartSnapshots.size());
            while (count6-- > 0) {
                std::string pn = vessel->protoVessel->protoPartSnapshots[static_cast<size_t>(count6)]->partName.value_or("");
                for (char& c : pn)
                    if (c == '_') c = '.';
                const AvailablePart* info = SaveEnvironment::Current().getPartInfoByName ? SaveEnvironment::Current().getPartInfoByName(pn) : nullptr;
                Part* partPrefab = info->partPrefab.get();
                int count7 = static_cast<int>(partPrefab->Modules().Count());
                while (count7-- > 0) {
                    auto* m = dynamic_cast<ModuleScienceExperiment*>(partPrefab->Modules()[static_cast<size_t>(count7)]);
                    if (m != nullptr) {
                        const std::string experimentID = m->Get<std::string>("experimentID");
                        if (experimentID.find("robotScannerArmROCScan") == std::string::npos) list4.push_back(experimentID);
                    }
                }
            }
        }
        if (!list4.empty()) list.emplace_back(vessel, list4);
    }
    int count8 = static_cast<int>(list.size());
    while (count8-- > 0) {
        const std::vector<std::string>& value = list[static_cast<size_t>(count8)].second;
        bool flag = false;
        int count9 = static_cast<int>(value.size());
        while (count9-- > 0) {
            if (Has(list2, value[static_cast<size_t>(count9)])) {
                flag = true;
                break;
            }
        }
        if (!flag) list.erase(list.begin() + count8);
    }
    return list;
}

// SurveyContract.cs:628
void SurveyContract::SetupParam(int paramIndex, const std::pair<SurveyDefinitionParameter*, FlightBand>& pair, int totalParameters, Vessel* contextualVessel) {
    (void)contextualVessel;
    SurveyDefinitionParameter* key = pair.first;
    FlightBand value = pair.second;
    UnityRandom::Global().InitState(MissionSeed());
    const double num = ParamRange(Prestige(), targetBody, value);
    bool waterAllowed = true;
    if (targetBody->ocean && value == FlightBand::GROUND) waterAllowed = key->AllowWater;
    const float num2 = contextual ? 0.5f : 1.0f;
    const float f = f32store(fx(key->FundsMultiplier) * fx(DF("Survey", "Funds", "WaypointDefaultReward")) * fx(num2));
    const float f2 = f32store(fx(key->ScienceMultiplier) * fx(DF("Survey", "Science", "WaypointDefaultReward")) * fx(num2));
    const float f3 = f32store(fx(key->ReputationMultiplier) * fx(DF("Survey", "Reputation", "WaypointDefaultReward")) * fx(num2));
    Waypoint waypoint;
    waypoint.seed = Lite::SuperSeed(*this);
    waypoint.id = key->Texture;
    waypoint.index = paramIndex;
    waypoint.contractReference = this;
    waypoint.celestialName = targetBody->GetName();
    SurveyWaypointParameter* surveyWaypointParameter;
    if (contextual) {
        waypoint.latitude = centerLatitude;
        waypoint.longitude = centerLongitude;
        const double num3 = M::Max(ParamRange(Prestige(), targetBody, value), DD("Survey", "", "MinimumTriggerRange"));
        waypoint.RandomizeAwayFrom(centerLatitude, centerLongitude, num3, num3 * 2.0, 3, waterAllowed);
        cardinalName = Lite::StringUtilities::CardinalDirectionBetween(centerLatitude, centerLongitude, waypoint.latitude, waypoint.longitude);
        surveyWaypointParameter = new SurveyWaypointParameter(key->Experiment, key->Description, targetBody, waypoint, value, contextual, totalParameters, centerLatitude, centerLongitude);
    } else {
        waypoint.RandomizeNear(centerLatitude, centerLongitude, focusedSurvey ? 1.0 : num, waterAllowed);
        surveyWaypointParameter = new SurveyWaypointParameter(key->Experiment, key->Description, targetBody, waypoint, value);
    }
    AddParameter(std::unique_ptr<ContractParameter>(surveyWaypointParameter));
    surveyWaypointParameter->ProcessWaypoint();
    if (!focusedSurvey) {
        surveyWaypointParameter->SetFunds(Round(f), targetBody);
        surveyWaypointParameter->SetReputation(Round(f3));
        surveyWaypointParameter->SetScience(Round(f2));
    }
}

// SurveyContract.cs:38
bool SurveyContract::Generate() {
    const int progressLevel = PU::GetProgressLevel();
    int num = 0;
    int num2 = 0;
    const int maximumAvailable = DI("Survey", "", "MaximumAvailable");
    const int maximumActive = DI("Survey", "", "MaximumActive");
    auto currentContracts = ContractSystem::Instance->GetCurrentContracts<SurveyContract>();
    int num3 = static_cast<int>(currentContracts.size());
    while (num3-- > 0) {
        switch (currentContracts[static_cast<size_t>(num3)]->ContractState()) {
            case State::Active: num2++; break;
            case State::Offered: num++; break;
            default: break;
        }
    }
    if (!(num < maximumAvailable && num2 < maximumActive)) return false;
    KSPRandom kSPRandom(Lite::SuperSeed(*this));
    auto notSun = [](CelestialBody* cb) { return cb != Planetarium::fetch->Sun; };
    std::vector<CelestialBody*> bodiesProgress = PU::GetBodiesReached(true, notSun);
    std::vector<CelestialBody*> nextUnreached = PU::GetNextUnreached(2, notSun);
    if (Prestige() == ContractPrestige::Exceptional) bodiesProgress.insert(bodiesProgress.end(), nextUnreached.begin(), nextUnreached.end());
    if (bodiesProgress.empty()) return false;
    targetBody = WeightedBodyChoice(bodiesProgress, &kSPRandom);
    const int num4 = M::Max(static_cast<int>(Round(f32store(2.0 - fx(Lite::CelestialUtilities::PlanetScienceRanking(targetBody)) * 2.0))), 0);
    int num5 = 0;
    float num6 = 1.0f;
    float num7 = 1.0f;
    float num8 = 1.0f;
    switch (Prestige()) {
        case ContractPrestige::Trivial:
            num5 = DI("Survey", "", "TrivialWaypoints");
            num5 += num4;
            break;
        case ContractPrestige::Significant:
            num5 = DI("Survey", "", "SignificantWaypoints");
            num5 += num4;
            break;
        case ContractPrestige::Exceptional:
            num5 = DI("Survey", "", "ExceptionalWaypoints");
            num5 += num4;
            break;
    }
    focusedSurvey = false;
    if (progressLevel < 4) {
        const int num9 = static_cast<int>(Prestige()) + 1;
        if (progressLevel == num9) focusedSurvey = true;
    }
    std::vector<std::unique_ptr<SurveyDefinition>> list = PossibleDefinitions();
    if (list.empty()) return false;
    contextual = false;
    Vessel* vessel = nullptr;
    FlightBand flightBand = FlightBand::GROUND;
    if (!focusedSurvey) {
        auto list2 = CachePossibleVessels(targetBody, list);
        const int num10 = Mathf::RoundToInt(f32store(fx(DF("Survey", "", "ContextualChance")) *
                                                     (static_cast<double>(M::Min(static_cast<float>(list2.size()), DF("Survey", "", "ContextualAssets"))) / fx(DF("Survey", "", "ContextualAssets")))));
        if (kSPRandom.Next(0, 100) < num10) {
            contextual = true;
            auto keyValuePair = list2[static_cast<size_t>(kSPRandom.Next(0, static_cast<int>(list2.size())))];
            vessel = keyValuePair.first;
            vesselName = vessel->loaded ? vessel->GetDisplayName() : vessel->protoVessel->GetDisplayName();
            switch (Prestige()) {
                default: flightBand = FlightBand::GROUND; break;
                case ContractPrestige::Exceptional: flightBand = FlightBand::LOW; break;
                case ContractPrestige::Significant: flightBand = Lite::CoinFlip(&kSPRandom) ? FlightBand::GROUND : FlightBand::LOW; break;
            }
            if (static_cast<double>(num5) * ParamRange(Prestige(), targetBody, FlightBand::LOW) > 3.141592653589793 * targetBody->Radius) flightBand = FlightBand::GROUND;
            int count = static_cast<int>(list.size());
            while (count-- > 0) {
                SurveyDefinition& surveyDefinition = *list[static_cast<size_t>(count)];
                int count2 = static_cast<int>(surveyDefinition.Parameters.size());
                while (count2-- > 0) {
                    SurveyDefinitionParameter& p = *surveyDefinition.Parameters[static_cast<size_t>(count2)];
                    if (!Has(keyValuePair.second, p.Experiment)) surveyDefinition.Parameters.erase(surveyDefinition.Parameters.begin() + count2);
                }
                if (surveyDefinition.Parameters.empty()) list.erase(list.begin() + count);
            }
            if (list.empty()) return false;
        }
    }
    SurveyDefinition* surveyDefinition2 = list[static_cast<size_t>(kSPRandom.Next(0, static_cast<int>(list.size())))].get();
    auto& list3 = surveyDefinition2->Parameters;
    bool flag = false;
    bool flag2 = false;
    bool flag3 = false;
    int count3 = static_cast<int>(list3.size());
    while (count3-- > 0) {
        SurveyDefinitionParameter& p = *list3[static_cast<size_t>(count3)];
        flag = flag || p.AllowGround;
        flag3 = flag3 || p.AllowLow;
        flag2 = flag2 || p.AllowHigh;
    }
    if (focusedSurvey) flag = false;
    if (contextual) {
        flag = flightBand == FlightBand::GROUND;
        flag3 = flightBand == FlightBand::LOW;
        flag2 = false;
    }
    if (Lite::CelestialUtilities::IsGasGiant(targetBody)) {
        flag = false;
        flag3 = false;
    }
    std::vector<FlightBand> list4;
    if (flag) list4.push_back(FlightBand::GROUND);
    if (flag3) list4.push_back(FlightBand::LOW);
    if (flag2) list4.push_back(FlightBand::HIGH);
    if (Prestige() >= ContractPrestige::Significant) {
        if (flag3 && flag2) list4.push_back(FlightBand::HIGHMIX);
        if (Prestige() == ContractPrestige::Exceptional) {
            if (flag && flag3) list4.push_back(FlightBand::LOWMIX);
            if (flag && flag3 && flag2) list4.push_back(FlightBand::ANYMIX);
        }
    }
    if (list4.empty()) return false;
    targetBand = list4[static_cast<size_t>(kSPRandom.Next(0, static_cast<int>(list4.size())))];
    int count4 = static_cast<int>(list3.size());
    while (count4-- > 0) {
        SurveyDefinitionParameter& p = *list3[static_cast<size_t>(count4)];
        bool remove = false;
        switch (targetBand) {
            default: remove = !p.AllowGround && !p.AllowLow && !p.AllowHigh; break;
            case FlightBand::GROUND: remove = !p.AllowGround; break;
            case FlightBand::LOW: remove = !p.AllowLow; break;
            case FlightBand::HIGH: remove = !p.AllowHigh; break;
            case FlightBand::LOWMIX: remove = !p.AllowGround && !p.AllowLow; break;
            case FlightBand::HIGHMIX: remove = !p.AllowLow && !p.AllowHigh; break;
        }
        if (remove) list3.erase(list3.begin() + count4);
    }
    if (list3.empty()) return false;
    int totalParameters = INT32_MAX;
    if (contextual) {
        totalParameters = num5;
        num5 = 1;
        switch (Prestige()) {
            case ContractPrestige::Exceptional:
                num6 = f32store(fx(num6) * fx(0.4f));
                num7 = f32store(fx(num7) * fx(0.4f));
                num8 = f32store(fx(num8) * fx(0.4f));
                break;
            default:
                num6 = f32store(fx(num6) * fx(0.2f));
                num7 = f32store(fx(num7) * fx(0.2f));
                num8 = f32store(fx(num8) * fx(0.2f));
                break;
            case ContractPrestige::Significant:
                num6 = f32store(fx(num6) * fx(0.3f));
                num7 = f32store(fx(num7) * fx(0.3f));
                num8 = f32store(fx(num8) * fx(0.3f));
                break;
        }
    }
    if (focusedSurvey) {
        float num11 = static_cast<float>(DI("Survey", "", "TrivialWaypoints") + DI("Survey", "", "SignificantWaypoints") + DI("Survey", "", "ExceptionalWaypoints") + num4 * 3);
        num11 = f32store(fx(num11) / fx(3.0f));
        const float num12 = f32store(1.0 / fx(num11));
        num5 = 1;
        num6 = f32store(fx(num6) * fx(num12));
        num7 = f32store(fx(num7) * fx(num12));
        num8 = f32store(fx(num8) * fx(num12));
    }
    bool flag4 = false;
    if (targetBody->ocean && (targetBand == FlightBand::GROUND || targetBand == FlightBand::LOWMIX || targetBand == FlightBand::ANYMIX)) {
        int count5 = static_cast<int>(list3.size());
        while (count5-- > 0 && !(flag4 = !list3[static_cast<size_t>(count5)]->AllowWater)) {
        }
    }
    if (contextual && vessel != nullptr) {
        centerLatitude = vessel->loaded ? vessel->latitude : vessel->protoVessel->latitude;
        centerLongitude = vessel->loaded ? vessel->longitude : vessel->protoVessel->longitude;
    } else {
        float num13 = 0.0f;
        if (targetBody == Home()) {
            float num14 = static_cast<float>(progressLevel + static_cast<int>(prestige));
            const float homeNearbyProgressCap = DF("Survey", "", "HomeNearbyProgressCap");
            if (num14 < 0.0f)
                num14 = 0.0f;
            else if (num14 > homeNearbyProgressCap)
                num14 = homeNearbyProgressCap;
            num13 = f32store(1.0 - fx(num14) / fx(homeNearbyProgressCap));
        }
        if (targetBody == Home() && (focusedSurvey || kSPRandom.NextDouble() < static_cast<double>(num13)))
            RandomizeNearKSC(prestige, !flag4, &kSPRandom);
        else
            WaypointManager::ChooseRandomPosition(centerLatitude, centerLongitude, targetBody->GetName(), !flag4, false, &kSPRandom);
    }
    std::vector<std::pair<SurveyDefinitionParameter*, FlightBand>> list5;
    int num15 = 0;
    int num16 = 0;
    for (int i = 0; i < num5; i++) {
        SurveyDefinitionParameter* surveyDefinitionParameter4 = list3[static_cast<size_t>(kSPRandom.Next(0, static_cast<int>(list3.size())))].get();
        std::vector<FlightBand> list6;
        bool flag5 = surveyDefinitionParameter4->AllowGround;
        bool flag6 = surveyDefinitionParameter4->AllowLow;
        bool flag7 = surveyDefinitionParameter4->AllowHigh;
        const bool crewRequired = surveyDefinitionParameter4->CrewRequired;
        const bool eVARequired = surveyDefinitionParameter4->EVARequired;
        if (focusedSurvey) flag5 = false;
        if (contextual) {
            flag5 = flightBand == FlightBand::GROUND;
            flag6 = flightBand == FlightBand::LOW;
            flag7 = false;
        }
        if (eVARequired && targetBody == Home() &&
            !GameVariables::Instance->UnlockedEVA(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::AstronautComplex))) {
            flag6 = false;
            flag7 = false;
        }
        switch (targetBand) {
            case FlightBand::GROUND:
                flag6 = false;
                flag7 = false;
                break;
            case FlightBand::LOW:
                flag5 = false;
                flag7 = false;
                break;
            case FlightBand::HIGH:
                flag5 = false;
                flag6 = false;
                break;
            case FlightBand::LOWMIX: flag7 = false; break;
            case FlightBand::HIGHMIX: flag5 = false; break;
            default: break;
        }
        if (Lite::CelestialUtilities::IsGasGiant(targetBody)) {
            if (flag7) list6.push_back(FlightBand::HIGH);
        } else {
            if (flag7) list6.push_back(FlightBand::HIGH);
            if (flag5) {
                if (Prestige() != ContractPrestige::Exceptional) {
                    if (PU::VisitedSurfaceOf(targetBody, !crewRequired ? MannedStatus::ANY : MannedStatus::MANNED)) list6.push_back(FlightBand::GROUND);
                } else {
                    list6.push_back(FlightBand::GROUND);
                }
            }
            if (flag6) list6.push_back(FlightBand::LOW);
        }
        if (!list6.empty()) {
            const FlightBand flightBand2 = list6[static_cast<size_t>(kSPRandom.Next(static_cast<int>(list6.size())))];
            num15++;
            if (flightBand2 == FlightBand::GROUND) num16++;
            list5.emplace_back(surveyDefinitionParameter4, flightBand2);
        }
    }
    if (list5.empty()) return false;
    int num17 = 0;
    const int count6 = static_cast<int>(list5.size());
    for (int j = 0; j < count6; j++) {
        auto pair = list5[static_cast<size_t>(j)];
        if (pair.second == FlightBand::GROUND) SetupParam(num17++, pair, totalParameters, contextual ? vessel : nullptr);
    }
    for (int k = 0; k < count6; k++) {
        auto pair = list5[static_cast<size_t>(k)];
        if (pair.second != FlightBand::GROUND) SetupParam(num17++, pair, totalParameters, contextual ? vessel : nullptr);
    }
    dataName = surveyDefinition2->DataName;
    anomalyName = surveyDefinition2->AnomalyName;
    resultName = surveyDefinition2->ResultName;
    WaypointClusterState waypointClusterState = WaypointClusterState::MIXED;
    if (!contextual) {
        if (num15 == num16) waypointClusterState = WaypointClusterState::FULL;
        if (num16 == 1) waypointClusterState = WaypointClusterState::SINGLE;
    }
    if (waypointClusterState == WaypointClusterState::FULL && !contextual)
        locationName = Fmt("#autoLOC_281212", {targetBody->displayName(), Lite::StringUtilities::GenerateSiteName(Lite::SuperSeed(*this), targetBody, true)});
    else if (contextual)
        locationName = Fmt("#autoLOC_281214", {targetBody->displayName(), vesselName});
    else
        locationName = targetBody->displayName();
    std::vector<ContractParameter*> all;
    AllParameters(all);
    for (ContractParameter* p : all)
        if (auto* s = dynamic_cast<SurveyWaypointParameter*>(p)) s->clusterState = waypointClusterState;
    AddKeywords({Lite::CoinFlip(&kSPRandom) ? "Scientific" : "Commercial"});
    if (std::find(nextUnreached.begin(), nextUnreached.end(), targetBody) != nextUnreached.end()) AddKeywords({"Pioneer"});
    SetExpiry(DI("Survey", "Expiration", "MinimumExpireDays"), DI("Survey", "Expiration", "MaximumExpireDays"));
    SetDeadlineDays(static_cast<float>(DI("Survey", "Expiration", "DeadlineDays")), targetBody);
    SetFunds(Round(f32store(fx(surveyDefinition2->FundsAdvance) * fx(num6))), Round(f32store(fx(surveyDefinition2->FundsReward) * fx(num6))),
             Round(f32store(fx(surveyDefinition2->FundsFailure) * fx(num6))), targetBody);
    SetScience(Round(f32store(fx(surveyDefinition2->ScienceReward) * fx(num7))));
    SetReputation(Round(f32store(fx(surveyDefinition2->ReputationReward) * fx(num8))), Round(f32store(fx(surveyDefinition2->ReputationFailure) * fx(num8))));
    return true;
}
// SurveyContract.cs:518
std::string SurveyContract::GetHashString() { return I(Lite::SuperSeed(*this)); }
// SurveyContract.cs:523
std::string SurveyContract::GetTitle() {
    if (focusedSurvey) return Fmt("#autoLOC_281262", {dataName, locationName});
    return Fmt("#autoLOC_281264", {dataName, locationName});
}
// SurveyContract.cs:532
std::string SurveyContract::GetDescription() {
    return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), targetBody->displayName(), targetBody->displayName(), MissionSeed(), true, false, true);
}
// SurveyContract.cs:537
std::string SurveyContract::GetSynopsys() {
    if (focusedSurvey) return Fmt("#autoLOC_281275", {targetBody->displayName(), dataName, resultName});
    if (contextual) return Fmt("#autoLOC_281278", {vesselName, targetBody->displayName(), dataName, anomalyName, cardinalName, resultName});
    return Fmt("#autoLOC_281280", {dataName, anomalyName, locationName, resultName});
}
// SurveyContract.cs:550
std::string SurveyContract::MessageCompleted() { return Fmt("#autoLOC_281285", {dataName, resultName}); }
// SurveyContract.cs:555
std::string SurveyContract::GetNotes() {
    std::string text;
    if (Lite::CelestialUtilities::IsGasGiant(targetBody)) {
        text += Fmt("#autoLOC_281293");
        if (SaveEnvironment::Current().loadedScene == GameScenes::SPACECENTER) text += "\n";
        return text;
    }
    return "";
}
// SurveyContract.cs:570
void SurveyContract::OnSave(ConfigNode& node) {
    node.AddValue("targetBody", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("centerLatitude", centerLatitude);
    node.AddValue("centerLongitude", centerLongitude);
    node.AddValue("dataName", dataName);
    node.AddValue("anomalyName", anomalyName);
    node.AddValue("reportName", resultName);
    node.AddValue("locationName", locationName);
    node.AddValue("focusedSurvey", focusedSurvey);
    node.AddValue("targetBand", dotnet::EnumToString(targetBand));
    node.AddValue("contextual", contextual);
    if (contextual) {
        node.AddValue("vesselName", vesselName);
        node.AddValue("cardinalName", cardinalName);
    }
}
// SurveyContract.cs:589
void SurveyContract::OnLoad(ConfigNode& node) {
    Lite::LoadNode(node, "SurveyContract", "targetBody", targetBody, Home());
    Lite::LoadNode(node, "SurveyContract", "centerLatitude", centerLatitude, 0.0);
    Lite::LoadNode(node, "SurveyContract", "centerLongitude", centerLongitude, 0.0);
    Lite::LoadNode(node, "SurveyContract", "dataName", dataName, std::string("generic"));
    Lite::LoadNode(node, "SurveyContract", "anomalyName", anomalyName, std::string("anomalies"));
    Lite::LoadNode(node, "SurveyContract", "reportName", resultName, std::string("results"));
    Lite::LoadNode(node, "SurveyContract", "locationName", locationName, std::string("location"));
    Lite::LoadNode(node, "SurveyContract", "focusedSurvey", focusedSurvey, false);
    Lite::LoadNode(node, "SurveyContract", "targetBand", targetBand, FlightBand::NONE);
    Lite::LoadNode(node, "SurveyContract", "contextual", contextual, false);
    if (contextual) {
        Lite::LoadNode(node, "SurveyContract", "vesselName", vesselName, std::string("vessel"));
        Lite::LoadNode(node, "SurveyContract", "cardinalName", cardinalName, std::string("cardinal"));
    }
}
// SurveyContract.cs:608
bool SurveyContract::MeetRequirements() {
    if (!ProgressTracking::Instance()->NodeComplete({"FirstLaunch"})) return false;
    const int progressLevel = PU::GetProgressLevel();
    const int num = static_cast<int>(Prestige()) + 1;
    if (progressLevel < num) return false;
    return true;
}

}  // namespace ksp::FinePrint::Contracts
