// WP-38 slice 3: Contracts/Templates/{RoverConstructionContract,OrbitalConstructionContract,VesselRepairContract}.cs.
// See include/ksp/fineprint/construction_templates.hpp.
#include "ksp/fineprint/construction_templates.hpp"

#include <algorithm>
#include <stdexcept>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/agents.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/text_gen.hpp"
#include "ksp/contracts/waypoints.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/fineprint/contract_defs.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/deployables.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/science/science.hpp"
#include "ksp/unity/mathf.hpp"
#include "param_common.hpp"

namespace ksp::Contracts::Templates {

using namespace ksp::fpcommon;
using ksp::FinePrint::ContractDefs;
using ksp::Contracts::Parameters::ConstructionParameter;
using ksp::Contracts::Parameters::CrewTraitParameter;
using ksp::Contracts::Parameters::RepairPartParameter;
using ksp::Contracts::Parameters::RoverWayPointParameter;
using ksp::Contracts::Lite::PreBuiltEnvironment;
using ksp::Contracts::Lite::PreBuiltVesselSituation;
namespace L = ksp::Contracts::Lite;
namespace M = ksp::dotnet::Math;
using GE = events::GameEvents;

namespace {
CelestialBody* Home() { return Planetarium::fetch->Home; }
float Mul(float a, float b) { return f32store(fx(a) * fx(b)); }
// a float expression `a * b * c` evaluated in double and rounded where the C# passes it (Mathf.RoundToInt)
float Mul3(float a, float b, float c) { return f32store(fx(a) * fx(b) * fx(c)); }
int RoundI(float f) { return Mathf::RoundToInt(f); }
float FI(int v) { return static_cast<float>(v); }
// `new KSPRandom()`: ContractsEnvironment::newRandomSeed
KSPRandom FreshRandom() { return KSPRandom(ContractsEnvironment::Current().NewRandomSeed()); }
CelestialBody* BodyByName(const std::string& name) {
    for (CelestialBody* b : FlightRegistry::Bodies())
        if (b != nullptr && b->bodyName == name) return b;
    return nullptr;
}
// FlightGlobals.FindVessel(persistentId, out vessel)
Vessel* FindVesselById(uint32_t persistentId) {
    for (Vessel* v : FlightRegistry::Vessels)
        if (v != nullptr && v->persistentId == persistentId) return v;
    return nullptr;
}
const AvailablePart* PartInfoByName(const std::string& name) {
    return SaveEnvironment::Current().getPartInfoByName ? SaveEnvironment::Current().getPartInfoByName(name) : nullptr;
}
void SpawnPreBuiltShip(const std::shared_ptr<L::PreBuiltCraftDefinition>& def, const PreBuiltVesselSituation& situation) {
    if (PreBuiltEnvironment::spawnPreBuiltShip && def) PreBuiltEnvironment::spawnPreBuiltShip(*def, situation);
}
std::string VesselNameFrom(const std::vector<std::string>& tags, int missionSeed, const std::string& bodyName) {
    KSPRandom kSPRandom(missionSeed);
    return KSP::Localization::Localizer::Format(tags[static_cast<size_t>(kSPRandom.Next(0, static_cast<int>(tags.size())))],
                                                {bodyName, L::StringUtilities::AlphaNumericDesignation(missionSeed)});
}
}  // namespace

// The reward block of the three Generate functions (SetScience / SetReputation / SetFunds with Mathf.RoundToInt of base * num4 * multiplier)
#define KSP_REWARD_BLOCK(Sec, Trivial_, Significant_, Exceptional_, bodyArg)                                                                                          \
    switch (Prestige()) {                                                                                                                                              \
        default:                                                                                                                                                       \
            SetScience(FI(RoundI(Mul3(ContractDefs::Sec::Science::BaseReward, num4, ContractDefs::Sec::Trivial_::ScienceMultiplier))), bodyArg);                    \
            SetReputation(FI(RoundI(Mul3(ContractDefs::Sec::Reputation::BaseReward, num4, ContractDefs::Sec::Trivial_::ReputationMultiplier))),                    \
                          FI(RoundI(Mul3(ContractDefs::Sec::Reputation::BaseFailure, num4, ContractDefs::Sec::Trivial_::ReputationMultiplier))), bodyArg);          \
            SetFunds(FI(RoundI(Mul3(ContractDefs::Sec::Funds::BaseAdvance, num4, ContractDefs::Sec::Trivial_::FundsMultiplier))),                                  \
                     FI(RoundI(Mul3(ContractDefs::Sec::Funds::BaseReward, num4, ContractDefs::Sec::Trivial_::FundsMultiplier))),                                    \
                     FI(RoundI(Mul3(ContractDefs::Sec::Funds::BaseFailure, num4, ContractDefs::Sec::Trivial_::FundsMultiplier))), bodyArg);                         \
            break;                                                                                                                                                     \
        case ContractPrestige::Exceptional:                                                                                                                            \
            SetScience(FI(RoundI(Mul3(ContractDefs::Sec::Science::BaseReward, num4, ContractDefs::Sec::Exceptional_::ScienceMultiplier))), bodyArg);                \
            SetReputation(FI(RoundI(Mul3(ContractDefs::Sec::Reputation::BaseReward, num4, ContractDefs::Sec::Exceptional_::ReputationMultiplier))),                \
                          FI(RoundI(Mul3(ContractDefs::Sec::Reputation::BaseFailure, num4, ContractDefs::Sec::Exceptional_::ReputationMultiplier))), bodyArg);      \
            SetFunds(FI(RoundI(Mul3(ContractDefs::Sec::Funds::BaseAdvance, num4, ContractDefs::Sec::Exceptional_::FundsMultiplier))),                              \
                     FI(RoundI(Mul3(ContractDefs::Sec::Funds::BaseReward, num4, ContractDefs::Sec::Exceptional_::FundsMultiplier))),                                \
                     FI(RoundI(Mul3(ContractDefs::Sec::Funds::BaseFailure, num4, ContractDefs::Sec::Exceptional_::FundsMultiplier))), bodyArg);                     \
            break;                                                                                                                                                     \
        case ContractPrestige::Significant:                                                                                                                            \
            SetScience(FI(RoundI(Mul3(ContractDefs::Sec::Science::BaseReward, num4, ContractDefs::Sec::Significant_::ScienceMultiplier))), bodyArg);                \
            SetReputation(FI(RoundI(Mul3(ContractDefs::Sec::Reputation::BaseReward, num4, ContractDefs::Sec::Significant_::ReputationMultiplier))),                \
                          FI(RoundI(Mul3(ContractDefs::Sec::Reputation::BaseFailure, num4, ContractDefs::Sec::Significant_::ReputationMultiplier))), bodyArg);      \
            SetFunds(FI(RoundI(Mul3(ContractDefs::Sec::Funds::BaseAdvance, num4, ContractDefs::Sec::Significant_::FundsMultiplier))),                              \
                     FI(RoundI(Mul3(ContractDefs::Sec::Funds::BaseReward, num4, ContractDefs::Sec::Significant_::FundsMultiplier))),                                \
                     FI(RoundI(Mul3(ContractDefs::Sec::Funds::BaseFailure, num4, ContractDefs::Sec::Significant_::FundsMultiplier))), bodyArg);                     \
            break;                                                                                                                                                     \
    }

// ================================================================================================================================ RoverConstructionContract

// RoverConstructionContract.cs:44
bool RoverConstructionContract::Generate() {
    int num = 0;
    int num2 = 0;
    std::vector<RoverConstructionContract*> currentContracts = ContractSystem::Instance->GetCurrentContracts<RoverConstructionContract>();
    int num3 = static_cast<int>(currentContracts.size());
    while (num3-- > 0) {
        switch (currentContracts[static_cast<size_t>(num3)]->ContractState()) {
            case State::Active: num2++; break;
            case State::Offered: num++; break;
            default: break;
        }
    }
    if (num < ContractDefs::RoverConstruction::MaximumAvailable && num2 < ContractDefs::RoverConstruction::MaximumActive) {
        if (ContractDefs::PreBuiltCraftDefs == nullptr) throw NullReferenceException("Object reference not set to an instance of an object.");
        L::PreBuiltCraft::List& list = ContractDefs::PreBuiltCraftDefs->CraftForContractType("RoverConstruction");
        if (list.size() <= 0) return false;
        std::vector<CelestialBody*> bodiesProgress = L::ProgressUtilities::GetBodiesProgress(L::ProgressType::ORBIT, true, true, L::MannedStatus::ANY, [](CelestialBody* cb) { return cb != Home(); });
        int count = static_cast<int>(bodiesProgress.size());
        while (count-- > 0) {
            CelestialBody* b = bodiesProgress[static_cast<size_t>(count)];
            if (!b->hasSolidSurface)
                bodiesProgress.erase(bodiesProgress.begin() + count);
            else if (b->GeeASL < static_cast<double>(ContractDefs::RoverConstruction::MinimumGeeASL) || b->GeeASL > static_cast<double>(ContractDefs::RoverConstruction::MaximumGeeASL))
                bodiesProgress.erase(bodiesProgress.begin() + count);
        }
        targetBody = WeightedBodyChoice(bodiesProgress);
        KSPRandom kSPRandom = FreshRandom();
        const int index = kSPRandom.Next(0, static_cast<int>(list.size()) - 1);
        roverCraftDefinition = list[static_cast<size_t>(index)];
        bool flag = false;
        if (targetBody == nullptr) throw NullReferenceException("Object reference not set to an instance of an object.");
        if (roverCraftDefinition->usePreBuiltPositions && ContractDefs::PreBuiltCraftPositions.ContainsKey(targetBody->bodyName)) {
            ContractDefs::PositionList& list2 = ContractDefs::PreBuiltCraftPositions[targetBody->bodyName];
            const int index2 = kSPRandom.Next(0, static_cast<int>(list2.size()) - 1);
            const double centerLatitude = list2[static_cast<size_t>(index2)].CenterLatitude;
            const double centerLongitude = list2[static_cast<size_t>(index2)].CenterLongitude;
            const float searchRadius = list2[static_cast<size_t>(index2)].SearchRadius;
            FinePrint::WaypointManager::ChooseRandomPositionNear(craftStartLatitude, craftStartLongitude, centerLatitude, centerLongitude, targetBody->bodyName, searchRadius,
                                                                 roverCraftDefinition->allowWater);
            flag = true;
        }
        if (!flag)
            FinePrint::WaypointManager::ChooseRandomPosition(craftStartLatitude, craftStartLongitude, targetBody->bodyName, roverCraftDefinition->allowWater, false, &kSPRandom);
        float wayPointMinDistance = ContractDefs::RoverConstruction::WayPointMinDistance;
        float wayPointMaxDistance = ContractDefs::RoverConstruction::WayPointMaxDistance;
        switch (Prestige()) {
            default:
                wayPointMinDistance = Mul(wayPointMinDistance, ContractDefs::RoverConstruction::Trivial::WayPointMultiplier);
                wayPointMaxDistance = Mul(wayPointMaxDistance, ContractDefs::RoverConstruction::Trivial::WayPointMultiplier);
                break;
            case ContractPrestige::Exceptional:
                wayPointMinDistance = Mul(wayPointMinDistance, ContractDefs::RoverConstruction::Exceptional::WayPointMultiplier);
                wayPointMaxDistance = Mul(wayPointMaxDistance, ContractDefs::RoverConstruction::Exceptional::WayPointMultiplier);
                break;
            case ContractPrestige::Significant:
                wayPointMinDistance = Mul(wayPointMinDistance, ContractDefs::RoverConstruction::Significant::WayPointMultiplier);
                wayPointMaxDistance = Mul(wayPointMaxDistance, ContractDefs::RoverConstruction::Significant::WayPointMultiplier);
                break;
        }
        CalculateWaypoint(wayPointMinDistance, wayPointMaxDistance);
        const float num4 = 1.0f;
        AddParameter(std::make_unique<RoverWayPointParameter>(craftStartLatitude, craftStartLongitude, craftEndLatitude, craftEndLongitude, targetBody));
        AddKeywords({"Commercial"});
        SetExpiry(ContractDefs::RoverConstruction::Expire::MinimumExpireDays, ContractDefs::RoverConstruction::Expire::MaximumExpireDays);
        SetDeadlineDays(Mul(static_cast<float>(ContractDefs::RoverConstruction::Expire::DeadlineDays), num4), targetBody);
        KSP_REWARD_BLOCK(RoverConstruction, Trivial, Significant, Exceptional, targetBody)
        return true;
    }
    return false;
}

// RoverConstructionContract.cs:147
void RoverConstructionContract::CalculateWaypoint(float minRadius, float maxRadius) {
    FinePrint::Waypoint waypoint;
    waypoint.celestialName = TargetBody()->bodyName;
    waypoint.latitude = craftStartLatitude;
    waypoint.longitude = craftStartLongitude;
    KSPRandom generator = FreshRandom();
    waypoint.RandomizeNear(waypoint.latitude, waypoint.longitude, minRadius, maxRadius, roverCraftDefinition->allowWater, &generator);
    craftEndLatitude = waypoint.latitude;
    craftEndLongitude = waypoint.longitude;
}

// RoverConstructionContract.cs:161
void RoverConstructionContract::OnRegister() {
    GE::Contract::onContractPreBuiltVesselSpawned.Add(this, &RoverConstructionContract::OnContractPreBuiltVesselSpawned);
    GE::onVesselPersistentIdChanged.Add(this, &RoverConstructionContract::VesselIdChanged);
    GE::onVesselDestroy.Add(this, &RoverConstructionContract::OnVesselDestroy);
}

// RoverConstructionContract.cs:168
void RoverConstructionContract::OnUnregister() {
    GE::Contract::onContractPreBuiltVesselSpawned.Remove(this, &RoverConstructionContract::OnContractPreBuiltVesselSpawned);
    GE::onVesselPersistentIdChanged.Remove(this, &RoverConstructionContract::VesselIdChanged);
    GE::onVesselDestroy.Remove(this, &RoverConstructionContract::OnVesselDestroy);
}

// RoverConstructionContract.cs:185
std::string RoverConstructionContract::GetHashString() { return I(L::SuperSeed(*this)); }

// RoverConstructionContract.cs:190
std::string RoverConstructionContract::GetTitle() { return Fmt("#autoLOC_6002550", {targetBody->displayName()}); }

// RoverConstructionContract.cs:195
std::string RoverConstructionContract::GetDescription() {
    return TextGen::GenerateBackStories("RoverConstruction", Agent()->Title(), targetBody->displayName(), "", MissionSeed(), false, false, false);
}

// RoverConstructionContract.cs:200
std::string RoverConstructionContract::GetSynopsys() { return Fmt("#autoLOC_6002551"); }

// RoverConstructionContract.cs:205
std::string RoverConstructionContract::MessageCompleted() { return Fmt("#autoLOC_6002552"); }

// RoverConstructionContract.cs:210
void RoverConstructionContract::OnLoad(ConfigNode& node) {
    std::string value;
    if (node.TryGetValue("bodyName", value)) targetBody = BodyByName(value);
    std::string value2;
    if (node.TryGetValue("roverCraftDef", value2)) {
        if (ContractDefs::PreBuiltCraftDefs == nullptr) throw NullReferenceException("Object reference not set to an instance of an object.");
        roverCraftDefinition = ContractDefs::PreBuiltCraftDefs->CraftByURL(value2);
    }
    node.TryGetValue("craftStartLat", craftStartLatitude);
    node.TryGetValue("craftStartLon", craftStartLongitude);
    node.TryGetValue("craftEndLat", craftEndLatitude);
    node.TryGetValue("craftEndLon", craftEndLongitude);
    node.TryGetValue("roverVslId", roverVesselId);
    Vessel* vessel = FindVesselById(roverVesselId);
    if (vessel != nullptr) return;
    const std::string value3 = CreateVesselName(targetBody->bodyDisplayName);
    if (value3.empty()) return;
    for (size_t i = 0; i < FlightRegistry::Vessels.size(); ++i) {
        if (FlightRegistry::Vessels[i]->vesselName == value3) {
            roverVesselId = FlightRegistry::Vessels[i]->persistentId;
            break;
        }
    }
    vessel = FindVesselById(roverVesselId);
    RoverWayPointParameter* roverWayPointParameter = GetParameterT<RoverWayPointParameter>();
    if (vessel != nullptr && roverWayPointParameter != nullptr) roverWayPointParameter->UpdateRoverInfo(vessel);
}

// RoverConstructionContract.cs:253
void RoverConstructionContract::OnSave(ConfigNode& node) {
    node.AddValue("bodyName", targetBody->bodyName);
    if (roverCraftDefinition != nullptr) node.AddValue("roverCraftDef", roverCraftDefinition->craftURL);
    node.AddValue("craftStartLat", craftStartLatitude);
    node.AddValue("craftStartLon", craftStartLongitude);
    node.AddValue("craftEndLat", craftEndLatitude);
    node.AddValue("craftEndLon", craftEndLongitude);
    if (roverVesselId != 0) node.AddValue("roverVslId", roverVesselId);
}

// RoverConstructionContract.cs:270
bool RoverConstructionContract::MeetRequirements() {
    if (L::ProgressUtilities::GetBodiesProgress(L::ProgressType::ORBIT, true, true, L::MannedStatus::ANY, [](CelestialBody* cb) { return cb != Home(); }).size() <= 0) return false;
    if (ResearchAndDevelopment::GetTechnologyState(ContractDefs::RoverConstruction::TechNodeRequired) != RDTech_State::Available) return false;
    return true;
}

// RoverConstructionContract.cs:283
void RoverConstructionContract::OnAccepted() {
    PreBuiltVesselSituation vesselSituation;
    vesselSituation.targetBody = targetBody;
    vesselSituation.latitude = craftStartLatitude;
    vesselSituation.longitude = craftStartLongitude;
    vesselSituation.hasOrbit = false;  // vesselSituation.location.orbitSnapShot = new MissionOrbit(targetBody)
    vesselSituation.playerCreated = false;
    vesselSituation.craftFile = roverCraftDefinition->craftURL;
    vesselSituation.focusOnSpawn = false;
    vesselSituation.vesselName = CreateVesselName(targetBody->bodyDisplayName);
    vesselSituation.situation = PreBuiltVesselSituation::StartSituation::LANDED;
    waitingForVslSpawn = true;
    SpawnPreBuiltShip(roverCraftDefinition, vesselSituation);
}

// RoverConstructionContract.cs:302
std::string RoverConstructionContract::CreateVesselName(const std::string& bodyName) { return VesselNameFrom(VesselNameStrings, MissionSeed(), bodyName); }

// RoverConstructionContract.cs:308
void RoverConstructionContract::OnContractPreBuiltVesselSpawned(Vessel* vsl) {
    if (waitingForVslSpawn) {
        roverVesselId = vsl->persistentId;
        if (auto* roverWayPointParameter = GetParameterT<RoverWayPointParameter>()) roverWayPointParameter->UpdateRoverInfo(vsl);
        waitingForVslSpawn = false;
    }
}

// RoverConstructionContract.cs:321
void RoverConstructionContract::VesselIdChanged(uint32_t oldId, uint32_t newId) {
    if (roverVesselId == oldId) roverVesselId = newId;
}

// RoverConstructionContract.cs:329
void RoverConstructionContract::OnVesselDestroy(Vessel* vsl) {
    if (vsl->persistentId == roverVesselId) Fail();
}

// ================================================================================================================================ OrbitalConstructionContract

// OrbitalConstructionContract.cs:59
bool OrbitalConstructionContract::Generate() {
    int num = 0;
    int num2 = 0;
    std::vector<OrbitalConstructionContract*> currentContracts = ContractSystem::Instance->GetCurrentContracts<OrbitalConstructionContract>();
    int num3 = static_cast<int>(currentContracts.size());
    while (num3-- > 0) {
        switch (currentContracts[static_cast<size_t>(num3)]->ContractState()) {
            case State::Active: num2++; break;
            case State::Offered: num++; break;
            default: break;
        }
    }
    if (num < ContractDefs::OrbitalConstruction::MaximumAvailable && num2 < ContractDefs::OrbitalConstruction::MaximumActive) {
        if (ContractDefs::PreBuiltCraftDefs == nullptr) throw NullReferenceException("Object reference not set to an instance of an object.");
        L::PreBuiltCraft::List& list = ContractDefs::PreBuiltCraftDefs->CraftForContractType("OrbitalConstruction");
        if (list.size() <= 0) return false;
        std::vector<CelestialBody*> bodiesProgress = L::ProgressUtilities::GetBodiesProgress(L::ProgressType::FLYBY, true, true, L::MannedStatus::MANNED);
        CelestialBody* homeBody = Home();
        switch (Prestige()) {
            case ContractPrestige::Trivial:
                if (L::ProgressUtilities::GetBodyProgress(L::ProgressType::ORBIT, homeBody, L::MannedStatus::MANNED)) {
                    targetBody = homeBody;
                    break;
                }
                return false;
            case ContractPrestige::Significant: {
                int count2 = static_cast<int>(bodiesProgress.size());
                while (count2-- > 0)
                    if (!bodiesProgress[static_cast<size_t>(count2)]->HasParent(homeBody)) bodiesProgress.erase(bodiesProgress.begin() + count2);
                if (bodiesProgress.size() <= 0) return false;
                targetBody = WeightedBodyChoice(bodiesProgress);
                break;
            }
            case ContractPrestige::Exceptional: {
                int count = static_cast<int>(bodiesProgress.size());
                while (count-- > 0) {
                    CelestialBody* b = bodiesProgress[static_cast<size_t>(count)];
                    if (b->isHomeWorld || b->isStar || b->HasParent(homeBody)) bodiesProgress.erase(bodiesProgress.begin() + count);
                }
                if (bodiesProgress.size() <= 0) return false;
                targetBody = WeightedBodyChoice(bodiesProgress);
                break;
            }
        }
        if (targetBody == nullptr) return false;
        KSPRandom kSPRandom = FreshRandom();
        const int index = kSPRandom.Next(0, static_cast<int>(list.size()) - 1);
        craftDefinition = list[static_cast<size_t>(index)];
        if (ContractDefs::constructionParts == nullptr) throw NullReferenceException("Object reference not set to an instance of an object.");
        L::ConstructionParts::List& list2 = ContractDefs::constructionParts->PartsForContractType("OrbitalConstruction");
        int count3 = static_cast<int>(list2.size());
        while (count3-- > 0) {
            const L::ConstructionPart& cp = *list2[static_cast<size_t>(count3)];
            const bool purchased = cp.availablePart != nullptr && (ContractsEnvironment::Current().partModelPurchased ? ContractsEnvironment::Current().partModelPurchased(cp.availablePart->name) : true);
            if (cp.availablePart == nullptr || !purchased) list2.erase(list2.begin() + count3);
        }
        if (list2.size() <= 0) return false;
        const int index2 = kSPRandom.Next(0, static_cast<int>(list2.size()) - 1);
        constructionPartName = list2[static_cast<size_t>(index2)]->partName;
        constructionPart = list2[static_cast<size_t>(index2)]->availablePart;
        orbitEccentricity = 0.0;
        orbitAltitudeFactor = 0.5;
        orbitInclinationFactor = 0.5;
        switch (Prestige()) {
            default:
                orbitEccentricity = M::Max(0.0, M::Min(static_cast<double>(ContractDefs::OrbitalConstruction::Trivial::MaxOrbitEccentricityFactor), kSPRandom.NextDouble() / 2.0));
                orbitAltitudeFactor = ContractDefs::OrbitalConstruction::Trivial::OrbitAltitudeFactor;
                orbitInclinationFactor = ContractDefs::OrbitalConstruction::Trivial::OrbitInclinationFactor;
                break;
            case ContractPrestige::Exceptional:
                orbitEccentricity = M::Max(0.0, M::Min(static_cast<double>(ContractDefs::OrbitalConstruction::Exceptional::MaxOrbitEccentricityFactor), kSPRandom.NextDouble() / 2.0));
                orbitAltitudeFactor = ContractDefs::OrbitalConstruction::Exceptional::OrbitAltitudeFactor;
                orbitInclinationFactor = ContractDefs::OrbitalConstruction::Exceptional::OrbitInclinationFactor;
                break;
            case ContractPrestige::Significant:
                orbitEccentricity = M::Max(0.0, M::Min(static_cast<double>(ContractDefs::OrbitalConstruction::Significant::MaxOrbitEccentricityFactor), kSPRandom.NextDouble() / 2.0));
                orbitAltitudeFactor = ContractDefs::OrbitalConstruction::Significant::OrbitAltitudeFactor;
                orbitInclinationFactor = ContractDefs::OrbitalConstruction::Significant::OrbitInclinationFactor;
                break;
        }
        orbit = std::make_unique<Orbit>(FinePrint::Utilities::OrbitUtilities::GenerateOrbit(MissionSeed(), targetBody, FinePrint::OrbitType::RANDOM, orbitAltitudeFactor, orbitInclinationFactor,
                                                                                            orbitEccentricity));
        vesselName = CreateVesselName(targetBody->bodyDisplayName);
        const float num4 = 1.0f;
        AddParameter(std::make_unique<CrewTraitParameter>("Engineer", 1, Fmt("#autoLOC_8004217")));
        AddParameter(std::make_unique<ConstructionParameter>(constructionPartName, constructionVesselId, vesselName, targetBody));
        AddKeywords({"Commercial"});
        // sic: the C# reads ContractDefs.RoverConstruction here
        SetExpiry(ContractDefs::RoverConstruction::Expire::MinimumExpireDays, ContractDefs::RoverConstruction::Expire::MaximumExpireDays);
        SetDeadlineDays(Mul(static_cast<float>(ContractDefs::RoverConstruction::Expire::DeadlineDays), num4), targetBody);
        KSP_REWARD_BLOCK(RoverConstruction, Trivial, Significant, Exceptional, targetBody)
        return true;
    }
    return false;
}

// OrbitalConstructionContract.cs:209
void OrbitalConstructionContract::OnRegister() {
    GE::Contract::onContractPreBuiltVesselSpawned.Add(this, &OrbitalConstructionContract::OnContractPreBuiltVesselSpawned);
    GE::onVesselPersistentIdChanged.Add(this, &OrbitalConstructionContract::VesselIdChanged);
    GE::onVesselDestroy.Add(this, &OrbitalConstructionContract::OnVesselDestroy);
}

// OrbitalConstructionContract.cs:216
void OrbitalConstructionContract::OnUnregister() {
    GE::Contract::onContractPreBuiltVesselSpawned.Remove(this, &OrbitalConstructionContract::OnContractPreBuiltVesselSpawned);
    GE::onVesselPersistentIdChanged.Remove(this, &OrbitalConstructionContract::VesselIdChanged);
    GE::onVesselDestroy.Remove(this, &OrbitalConstructionContract::OnVesselDestroy);
    CleanupRenderer();
}

// OrbitalConstructionContract.cs:234
std::string OrbitalConstructionContract::GetHashString() { return I(L::SuperSeed(*this)); }

// OrbitalConstructionContract.cs:239
std::string OrbitalConstructionContract::GetTitle() { return Fmt("#autoLOC_6002592", {targetBody->displayName()}); }

// OrbitalConstructionContract.cs:244
std::string OrbitalConstructionContract::GetDescription() {
    return TextGen::GenerateBackStories("OrbitConstruction", Agent()->Title(), targetBody->displayName(), "", MissionSeed(), false, false, false);
}

// OrbitalConstructionContract.cs:249
std::string OrbitalConstructionContract::GetSynopsys() {
    std::string text;
    if (constructionPart == nullptr) constructionPart = PartInfoByName(constructionPartName);
    if (constructionPart != nullptr) text = constructionPart->title;
    return Fmt("#autoLOC_6002599", {text});
}

// OrbitalConstructionContract.cs:263
std::string OrbitalConstructionContract::MessageCompleted() { return Fmt("#autoLOC_6002603"); }

// OrbitalConstructionContract.cs:268
void OrbitalConstructionContract::OnLoad(ConfigNode& node) {
    std::string value;
    if (node.TryGetValue("bodyName", value)) targetBody = BodyByName(value);
    std::string value2;
    if (node.TryGetValue("constructionCraftDef", value2)) {
        if (ContractDefs::PreBuiltCraftDefs == nullptr) throw NullReferenceException("Object reference not set to an instance of an object.");
        craftDefinition = ContractDefs::PreBuiltCraftDefs->CraftByURL(value2);
    }
    node.TryGetValue("constructionVslId", constructionVesselId);
    node.TryGetValue("constructionPartName", constructionPartName);
    if (!constructionPartName.empty()) constructionPart = PartInfoByName(constructionPartName);
    node.TryGetValue("orbitEccentricity", orbitEccentricity);
    node.TryGetValue("orbitAltitudeFactor", orbitAltitudeFactor);
    node.TryGetValue("orbitInclinationFactor", orbitInclinationFactor);
    node.TryGetValue("vesselName", vesselName);
    if (targetBody != nullptr) {
        orbit = std::make_unique<Orbit>(FinePrint::Utilities::OrbitUtilities::GenerateOrbit(MissionSeed(), targetBody, FinePrint::OrbitType::RANDOM, orbitAltitudeFactor,
                                                                                            orbitInclinationFactor, orbitEccentricity));
        if (Scene() == GameScenes::TRACKSTATION && ContractState() == State::Offered && ContractDefs::DisplayOfferedOrbits) SetupRenderer();
    }
    Vessel* vessel = FindVesselById(constructionVesselId);
    if (vessel != nullptr) return;
    for (size_t i = 0; i < FlightRegistry::Vessels.size(); ++i) {
        if (FlightRegistry::Vessels[i]->vesselName == vesselName) {
            constructionVesselId = FlightRegistry::Vessels[i]->persistentId;
            break;
        }
    }
    if (auto* constructionParameter = GetParameterT<ConstructionParameter>()) constructionParameter->UpdatePartInfo("", constructionVesselId, vesselName, nullptr);
}

// OrbitalConstructionContract.cs:317
void OrbitalConstructionContract::OnSave(ConfigNode& node) {
    node.AddValue("bodyName", targetBody->bodyName);
    if (craftDefinition != nullptr) node.AddValue("constructionCraftDef", craftDefinition->craftURL);
    if (constructionVesselId != 0) node.AddValue("constructionVslId", constructionVesselId);
    if (!constructionPartName.empty()) node.AddValue("constructionPartName", constructionPartName);
    node.AddValue("orbitEccentricity", orbitEccentricity);
    node.AddValue("orbitAltitudeFactor", orbitAltitudeFactor);
    node.AddValue("orbitInclinationFactor", orbitInclinationFactor);
    node.AddValue("vesselName", vesselName);
}

// OrbitalConstructionContract.cs:338
bool OrbitalConstructionContract::MeetRequirements() {
    if (ResearchAndDevelopment::GetTechnologyState(ContractDefs::OrbitalConstruction::TechNodeRequired) != RDTech_State::Available) return false;
    return true;
}

// OrbitalConstructionContract.cs:347
void OrbitalConstructionContract::OnAccepted() {
    PreBuiltVesselSituation vesselSituation;
    orbit = std::make_unique<Orbit>(FinePrint::Utilities::OrbitUtilities::GenerateOrbit(MissionSeed(), targetBody, FinePrint::OrbitType::RANDOM, orbitAltitudeFactor, orbitInclinationFactor,
                                                                                        orbitEccentricity));
    vesselSituation.targetBody = targetBody;
    vesselSituation.hasOrbit = true;
    vesselSituation.orbit = *orbit;
    vesselSituation.situation = PreBuiltVesselSituation::StartSituation::ORBITING;
    vesselSituation.playerCreated = false;
    vesselSituation.craftFile = craftDefinition->craftURL;
    vesselSituation.focusOnSpawn = false;
    vesselSituation.vesselName = vesselName;
    waitingForVslSpawn = true;
    SpawnPreBuiltShip(craftDefinition, vesselSituation);
}

// OrbitalConstructionContract.cs:363
std::string OrbitalConstructionContract::CreateVesselName(const std::string& bodyName) { return VesselNameFrom(VesselNameStrings, MissionSeed(), bodyName); }

// OrbitalConstructionContract.cs:369
void OrbitalConstructionContract::OnContractPreBuiltVesselSpawned(Vessel* vsl) {
    if (waitingForVslSpawn) {
        constructionVesselId = vsl->persistentId;
        if (auto* constructionParameter = GetParameterT<ConstructionParameter>()) constructionParameter->UpdatePartInfo(constructionPartName, constructionVesselId, vesselName, targetBody);
        waitingForVslSpawn = false;
    }
}

// OrbitalConstructionContract.cs:382
void OrbitalConstructionContract::VesselIdChanged(uint32_t oldId, uint32_t newId) {
    if (constructionVesselId == oldId) constructionVesselId = newId;
}

// OrbitalConstructionContract.cs:390
void OrbitalConstructionContract::OnVesselDestroy(Vessel* vsl) {
    if (vsl->persistentId == constructionVesselId) Fail();
}

// OrbitalConstructionContract.cs:398
void OrbitalConstructionContract::SetupRenderer() {
    if (!orbitRendererSet && orbit != nullptr && L::FinePrintEnvironment::setupOrbitRenderer) {
        L::FinePrintEnvironment::setupOrbitRenderer(*Root(), *orbit);
        orbitRendererSet = true;
    }
}

// OrbitalConstructionContract.cs:406
void OrbitalConstructionContract::CleanupRenderer() {
    if (orbitRendererSet && L::FinePrintEnvironment::cleanupOrbitRenderer) L::FinePrintEnvironment::cleanupOrbitRenderer();
}

// ================================================================================================================================ VesselRepairContract

// VesselRepairContract.cs:75
bool VesselRepairContract::Generate() {
    int num = 0;
    int num2 = 0;
    std::vector<VesselRepairContract*> currentContracts = ContractSystem::Instance->GetCurrentContracts<VesselRepairContract>();
    int num3 = static_cast<int>(currentContracts.size());
    while (num3-- > 0) {
        switch (currentContracts[static_cast<size_t>(num3)]->ContractState()) {
            case State::Active: num2++; break;
            case State::Offered: num++; break;
            default: break;
        }
    }
    if (num < ContractDefs::VesselRepair::MaximumAvailable && num2 < ContractDefs::VesselRepair::MaximumActive) {
        if (ContractDefs::PreBuiltCraftDefs == nullptr) throw NullReferenceException("Object reference not set to an instance of an object.");
        L::PreBuiltCraft::List& list = ContractDefs::PreBuiltCraftDefs->CraftForContractType("VesselRepair");
        int count = static_cast<int>(list.size());
        while (count-- > 0) {
            if (list[static_cast<size_t>(count)]->brokenPartNames().size() <= 0 || list[static_cast<size_t>(count)]->brokenPartNames().size() > 1) list.erase(list.begin() + count);
        }
        if (list.size() <= 0) return false;
        std::vector<CelestialBody*> bodiesProgress = L::ProgressUtilities::GetBodiesProgress(L::ProgressType::ORBIT, true, true, L::MannedStatus::MANNED);
        if (bodiesProgress.size() <= 0) return false;
        CelestialBody* homeBody = Home();
        switch (Prestige()) {
            case ContractPrestige::Trivial:
                if (L::ProgressUtilities::GetBodyProgress(L::ProgressType::ORBIT, homeBody, L::MannedStatus::MANNED)) {
                    targetBody = homeBody;
                    break;
                }
                return false;
            case ContractPrestige::Significant: {
                int count3 = static_cast<int>(bodiesProgress.size());
                while (count3-- > 0)
                    if (!bodiesProgress[static_cast<size_t>(count3)]->HasParent(homeBody)) bodiesProgress.erase(bodiesProgress.begin() + count3);
                if (bodiesProgress.size() <= 0) return false;
                targetBody = WeightedBodyChoice(bodiesProgress);
                break;
            }
            case ContractPrestige::Exceptional: {
                int count2 = static_cast<int>(bodiesProgress.size());
                while (count2-- > 0) {
                    CelestialBody* b = bodiesProgress[static_cast<size_t>(count2)];
                    if (b->isHomeWorld || b->isStar || b->HasParent(homeBody)) bodiesProgress.erase(bodiesProgress.begin() + count2);
                }
                if (bodiesProgress.size() <= 0) return false;
                targetBody = WeightedBodyChoice(bodiesProgress);
                break;
            }
        }
        if (targetBody == nullptr) return false;
        KSPRandom kSPRandom = FreshRandom();
        const int index = kSPRandom.Next(0, static_cast<int>(list.size()) - 1);
        repairCraftDefinition = list[static_cast<size_t>(index)];
        const int index2 = kSPRandom.Next(0, static_cast<int>(repairCraftDefinition->brokenPartNames().size()) - 1);
        repairPartName = repairCraftDefinition->brokenPartNames()[static_cast<size_t>(index2)];
        repairPart = PartInfoByName(repairPartName);
        if (repairPart == nullptr) return false;
        if (repairCraftDefinition->allowGround && repairCraftDefinition->allowOrbit) {
            if (kSPRandom.Next(0, 1) == 1)
                spawnOnGround = true;
            else
                spawnInOrbit = true;
        } else if (repairCraftDefinition->allowGround) {
            spawnOnGround = true;
        } else if (repairCraftDefinition->allowOrbit) {
            spawnInOrbit = true;
        }
        if (!spawnInOrbit && !spawnOnGround) return false;
        if (spawnOnGround) {
            bool flag = false;
            if (repairCraftDefinition->usePreBuiltPositions && ContractDefs::PreBuiltCraftPositions.ContainsKey(targetBody->bodyName)) {
                ContractDefs::PositionList& list2 = ContractDefs::PreBuiltCraftPositions[targetBody->bodyName];
                const int index3 = kSPRandom.Next(0, static_cast<int>(list2.size()) - 1);
                const double centerLatitude = list2[static_cast<size_t>(index3)].CenterLatitude;
                const double centerLongitude = list2[static_cast<size_t>(index3)].CenterLongitude;
                const float searchRadius = list2[static_cast<size_t>(index3)].SearchRadius;
                FinePrint::WaypointManager::ChooseRandomPositionNear(craftStartLatitude, craftStartLongitude, centerLatitude, centerLongitude, targetBody->bodyName, searchRadius,
                                                                     repairCraftDefinition->allowWater);
                flag = true;
            }
            if (!flag)
                FinePrint::WaypointManager::ChooseRandomPosition(craftStartLatitude, craftStartLongitude, targetBody->bodyName, repairCraftDefinition->allowWater, false, &kSPRandom);
        } else if (spawnInOrbit) {
            orbitEccentricity = 0.0;
            orbitAltitudeFactor = 0.5;
            orbitInclinationFactor = 0.5;
            switch (Prestige()) {
                default:
                    orbitEccentricity = M::Max(0.0, M::Min(static_cast<double>(ContractDefs::VesselRepair::Trivial::MaxOrbitEccentricityFactor), kSPRandom.NextDouble() / 2.0));
                    orbitAltitudeFactor = ContractDefs::VesselRepair::Trivial::OrbitAltitudeFactor;
                    orbitInclinationFactor = ContractDefs::VesselRepair::Trivial::OrbitInclinationFactor;
                    break;
                case ContractPrestige::Exceptional:
                    orbitEccentricity = M::Max(0.0, M::Min(static_cast<double>(ContractDefs::VesselRepair::Exceptional::MaxOrbitEccentricityFactor), kSPRandom.NextDouble() / 2.0));
                    orbitAltitudeFactor = ContractDefs::VesselRepair::Exceptional::OrbitAltitudeFactor;
                    orbitInclinationFactor = ContractDefs::VesselRepair::Exceptional::OrbitInclinationFactor;
                    break;
                case ContractPrestige::Significant:
                    orbitEccentricity = M::Max(0.0, M::Min(static_cast<double>(ContractDefs::VesselRepair::Significant::MaxOrbitEccentricityFactor), kSPRandom.NextDouble() / 2.0));
                    orbitAltitudeFactor = ContractDefs::VesselRepair::Significant::OrbitAltitudeFactor;
                    orbitInclinationFactor = ContractDefs::VesselRepair::Significant::OrbitInclinationFactor;
                    break;
            }
            orbit = std::make_unique<Orbit>(FinePrint::Utilities::OrbitUtilities::GenerateOrbit(MissionSeed(), targetBody, FinePrint::OrbitType::RANDOM, orbitAltitudeFactor,
                                                                                                orbitInclinationFactor, orbitEccentricity));
        }
        vesselName = CreateVesselName(targetBody->bodyDisplayName);
        const float num4 = 1.0f;
        AddParameter(std::make_unique<CrewTraitParameter>("Engineer", 1, Fmt("#autoLOC_8004217")));
        AddParameter(std::make_unique<RepairPartParameter>(0u, repairPartName, 0u, vesselName, targetBody));
        AddKeywords({"Commercial"});
        SetExpiry(ContractDefs::VesselRepair::Expire::MinimumExpireDays, ContractDefs::VesselRepair::Expire::MaximumExpireDays);
        SetDeadlineDays(Mul(static_cast<float>(ContractDefs::VesselRepair::Expire::DeadlineDays), num4), targetBody);
        KSP_REWARD_BLOCK(VesselRepair, Trivial, Significant, Exceptional, targetBody)
        return true;
    }
    return false;
}

// VesselRepairContract.cs:272
void VesselRepairContract::OnRegister() {
    GE::Contract::onContractPreBuiltVesselSpawned.Add(this, &VesselRepairContract::OnContractPreBuiltVesselSpawned);
    GE::onVesselPersistentIdChanged.Add(this, &VesselRepairContract::VesselIdChanged);
    GE::onVesselDestroy.Add(this, &VesselRepairContract::OnVesselDestroy);
}

// VesselRepairContract.cs:279
void VesselRepairContract::OnUnregister() {
    GE::Contract::onContractPreBuiltVesselSpawned.Remove(this, &VesselRepairContract::OnContractPreBuiltVesselSpawned);
    GE::onVesselPersistentIdChanged.Remove(this, &VesselRepairContract::VesselIdChanged);
    GE::onVesselDestroy.Remove(this, &VesselRepairContract::OnVesselDestroy);
    CleanupRenderer();
}

// VesselRepairContract.cs:297
std::string VesselRepairContract::GetHashString() { return I(L::SuperSeed(*this)); }

// VesselRepairContract.cs:302
std::string VesselRepairContract::GetTitle() { return Fmt("#autoLOC_6002568", {targetBody->displayName()}); }

// VesselRepairContract.cs:307
std::string VesselRepairContract::GetDescription() {
    return TextGen::GenerateBackStories("VesselRepair", Agent()->Title(), targetBody->displayName(), "", MissionSeed(), false, false, false);
}

// VesselRepairContract.cs:312
std::string VesselRepairContract::GetSynopsys() {
    std::string text;
    if (repairPart == nullptr) repairPart = PartInfoByName(repairPartName);
    if (repairPart != nullptr) text = repairPart->title;
    return Fmt("#autoLOC_6002576", {text});
}

// VesselRepairContract.cs:326
std::string VesselRepairContract::MessageCompleted() { return Fmt("#autoLOC_6002579"); }

// VesselRepairContract.cs:331
void VesselRepairContract::OnLoad(ConfigNode& node) {
    std::string value;
    if (node.TryGetValue("bodyName", value)) targetBody = BodyByName(value);
    std::string value2;
    if (node.TryGetValue("roverCraftDef", value2)) {
        if (ContractDefs::PreBuiltCraftDefs == nullptr) throw NullReferenceException("Object reference not set to an instance of an object.");
        repairCraftDefinition = ContractDefs::PreBuiltCraftDefs->CraftByURL(value2);
    }
    node.TryGetValue("vesselName", vesselName);
    node.TryGetValue("craftStartLat", craftStartLatitude);
    node.TryGetValue("craftStartLon", craftStartLongitude);
    node.TryGetValue("orbitEccentricity", orbitEccentricity);
    node.TryGetValue("orbitAltitudeFactor", orbitAltitudeFactor);
    node.TryGetValue("orbitInclinationFactor", orbitInclinationFactor);
    node.TryGetValue("repairVslId", repairVesselId);
    node.TryGetValue("repairPartId", repairPartId);
    node.TryGetValue("repairPartName", repairPartName);
    spawnInOrbit = true;
    spawnOnGround = false;
    std::string value3 = "Orbit";
    node.TryGetValue("spawnOrbit", value3);
    if (value3 == "Landed") {
        spawnInOrbit = false;
        spawnOnGround = true;
    }
    if (spawnInOrbit && targetBody != nullptr) {
        orbit = std::make_unique<Orbit>(FinePrint::Utilities::OrbitUtilities::GenerateOrbit(MissionSeed(), targetBody, FinePrint::OrbitType::RANDOM, orbitAltitudeFactor,
                                                                                            orbitInclinationFactor, orbitEccentricity));
        if (Scene() == GameScenes::TRACKSTATION && ContractState() == State::Offered && ContractDefs::DisplayOfferedOrbits) SetupRenderer();
    }
    Vessel* vessel = FindVesselById(repairVesselId);
    if (vessel != nullptr) return;
    for (size_t i = 0; i < FlightRegistry::Vessels.size(); ++i) {
        if (FlightRegistry::Vessels[i]->vesselName == vesselName) {
            repairVesselId = FlightRegistry::Vessels[i]->persistentId;
            break;
        }
    }
    vessel = FindVesselById(repairVesselId);
    bool flag = false;
    // FlightGlobals.FindLoadedPart(persistentId) / FindUnloadedPart(persistentId)
    Part* partout = nullptr;
    for (Vessel* v : FlightRegistry::Vessels) {
        if (v == nullptr || !v->loaded) continue;
        for (Part* p : v->parts)
            if (p->fields.persistentId == repairPartId) {
                partout = p;
                break;
            }
        if (partout != nullptr) break;
    }
    if (partout == nullptr) {
        ProtoPartSnapshot* partout2 = nullptr;
        for (Vessel* v : FlightRegistry::Vessels) {
            if (v == nullptr || v->loaded || v->protoVessel == nullptr) continue;
            for (auto& pp : v->protoVessel->protoPartSnapshots)
                if (pp->persistentId == repairPartId) {
                    partout2 = pp.get();
                    break;
                }
            if (partout2 != nullptr) break;
        }
        if (partout2 != nullptr && partout2->pVesselRef != nullptr && partout2->pVesselRef->persistentId == repairVesselId) flag = true;
    } else if (partout->vessel != nullptr && partout->vessel->persistentId == repairVesselId) {
        flag = true;
    }
    if (!flag && vessel != nullptr) {
        if (vessel->loaded) {
            for (size_t j = 0; j < vessel->parts.size(); ++j) {
                if (vessel->parts[j]->fields.partName.value_or("") == repairPartName) {
                    for (int k = 0; k < vessel->parts[j]->Modules().Count(); ++k) {
                        auto* moduleDeployablePart = dynamic_cast<ModuleDeployablePart*>(vessel->parts[j]->Modules()[static_cast<size_t>(k)]);
                        if (moduleDeployablePart != nullptr && moduleDeployablePart->deployState() == ModuleDeployablePart::DeployState::BROKEN) {
                            repairPartId = vessel->parts[j]->fields.persistentId;
                            flag = true;
                            break;
                        }
                    }
                }
                if (flag) break;
            }
        } else {
            for (size_t l = 0; l < vessel->protoVessel->protoPartSnapshots.size(); ++l) {
                if (vessel->protoVessel->protoPartSnapshots[l]->partName.value_or("") == repairPartName) {
                    for (size_t m = 0; m < vessel->protoVessel->protoPartSnapshots[l]->modules.size(); ++m) {
                        std::string value4;
                        if (vessel->protoVessel->protoPartSnapshots[l]->modules[m]->moduleValues->TryGetValue("deployState", value4) && value4 == "BROKEN") {
                            if (l >= vessel->parts.size()) throw std::out_of_range("Index was out of range. Must be non-negative and less than the size of the collection.");
                            repairPartId = vessel->parts[l]->fields.persistentId;
                            flag = true;
                            break;
                        }
                    }
                }
                if (flag) break;
            }
        }
    }
    RepairPartParameter* repairPartParameter = GetParameterT<RepairPartParameter>();
    if (vessel != nullptr && repairPartParameter != nullptr) repairPartParameter->UpdatePartInfo(repairPartId, "", repairVesselId, "", nullptr);
}

// VesselRepairContract.cs:455
void VesselRepairContract::OnSave(ConfigNode& node) {
    node.AddValue("bodyName", targetBody->bodyName);
    if (repairCraftDefinition != nullptr) node.AddValue("roverCraftDef", repairCraftDefinition->craftURL);
    node.AddValue("vesselName", vesselName);
    node.AddValue("craftStartLat", craftStartLatitude);
    node.AddValue("craftStartLon", craftStartLongitude);
    node.AddValue("orbitEccentricity", orbitEccentricity);
    node.AddValue("orbitAltitudeFactor", orbitAltitudeFactor);
    node.AddValue("orbitInclinationFactor", orbitInclinationFactor);
    if (repairVesselId != 0) node.AddValue("repairVslId", repairVesselId);
    if (repairPartId != 0) node.AddValue("repairPartId", repairPartId);
    if (!repairPartName.empty()) node.AddValue("repairPartName", repairPartName);
    if (spawnOnGround) node.AddValue("spawnOrbit", "Landed");
}

// VesselRepairContract.cs:486
bool VesselRepairContract::MeetRequirements() {
    const AvailablePart* partInfoByName = PartInfoByName("evaRepairKit");
    if (partInfoByName != nullptr && (ContractsEnvironment::Current().partModelPurchased ? ContractsEnvironment::Current().partModelPurchased(partInfoByName->name) : true)) return true;
    return false;
}

// VesselRepairContract.cs:496
void VesselRepairContract::OnAccepted() {
    PreBuiltVesselSituation vesselSituation;
    vesselSituation.targetBody = targetBody;
    vesselSituation.latitude = craftStartLatitude;
    vesselSituation.longitude = craftStartLongitude;
    if (spawnOnGround) {
        vesselSituation.hasOrbit = false;
        vesselSituation.situation = PreBuiltVesselSituation::StartSituation::LANDED;
    }
    if (spawnInOrbit) {
        orbit = std::make_unique<Orbit>(FinePrint::Utilities::OrbitUtilities::GenerateOrbit(MissionSeed(), targetBody, FinePrint::OrbitType::RANDOM, orbitAltitudeFactor,
                                                                                            orbitInclinationFactor, orbitEccentricity));
        vesselSituation.hasOrbit = true;
        vesselSituation.orbit = *orbit;
        vesselSituation.situation = PreBuiltVesselSituation::StartSituation::ORBITING;
    }
    vesselSituation.playerCreated = false;
    vesselSituation.craftFile = repairCraftDefinition->craftURL;
    vesselSituation.focusOnSpawn = false;
    vesselSituation.vesselName = vesselName;
    waitingForVslSpawn = true;
    SpawnPreBuiltShip(repairCraftDefinition, vesselSituation);
}

// VesselRepairContract.cs:524
std::string VesselRepairContract::CreateVesselName(const std::string& bodyName) { return VesselNameFrom(VesselNameStrings, MissionSeed(), bodyName); }

// VesselRepairContract.cs:530
void VesselRepairContract::OnContractPreBuiltVesselSpawned(Vessel* vsl) {
    if (!waitingForVslSpawn) return;
    repairVesselId = vsl->protoVessel->persistentId;
    if (repairPart == nullptr) repairPart = PartInfoByName(repairPartName);
    ProtoPartSnapshot* protoPartSnapshot = nullptr;
    bool flag = false;
    for (size_t i = 0; i < vsl->protoVessel->protoPartSnapshots.size(); ++i) {
        if (vsl->protoVessel->protoPartSnapshots[i]->partName.value_or("") == repairPart->name) {
            for (size_t j = 0; j < vsl->protoVessel->protoPartSnapshots[i]->modules.size(); ++j) {
                std::string value;
                if (vsl->protoVessel->protoPartSnapshots[i]->modules[j]->moduleValues->TryGetValue("deployState", value) && value == "BROKEN") {
                    protoPartSnapshot = vsl->protoVessel->protoPartSnapshots[i].get();
                    flag = true;
                    break;
                }
            }
        }
        if (flag) break;
    }
    if (auto* repairPartParameter = GetParameterT<RepairPartParameter>())
        repairPartParameter->UpdatePartInfo(protoPartSnapshot != nullptr ? protoPartSnapshot->persistentId : 0u, repairPartName, repairVesselId, vesselName, targetBody);
    waitingForVslSpawn = false;
}

// VesselRepairContract.cs:570
void VesselRepairContract::VesselIdChanged(uint32_t oldId, uint32_t newId) {
    if (repairVesselId == oldId) repairVesselId = newId;
}

// VesselRepairContract.cs:578
void VesselRepairContract::OnVesselDestroy(Vessel* vsl) {
    if (vsl->persistentId == repairVesselId) Fail();
}

// VesselRepairContract.cs:586
void VesselRepairContract::SetupRenderer() {
    if (!orbitRendererSet && orbit != nullptr && L::FinePrintEnvironment::setupOrbitRenderer) {
        L::FinePrintEnvironment::setupOrbitRenderer(*Root(), *orbit);
        orbitRendererSet = true;
    }
}

// VesselRepairContract.cs:594
void VesselRepairContract::CleanupRenderer() {
    if (orbitRendererSet && L::FinePrintEnvironment::cleanupOrbitRenderer) L::FinePrintEnvironment::cleanupOrbitRenderer();
}

}  // namespace ksp::Contracts::Templates
