// WP-38 slice 3: SentinelMission/{SentinelModule,SentinelScenario}.cs. See include/ksp/fineprint/sentinel.hpp.
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "ksp/career/career_environment.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/core/log.hpp"
#include "ksp/database/game_database.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/fineprint/contract_defs.hpp"
#include "ksp/fineprint/sentinel.hpp"
#include "ksp/fineprint/utilities.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp::SentinelMission {

using KSP::Localization::Localizer;
namespace L = ksp::Contracts::Lite;
namespace M = ksp::dotnet::Math;
using GE = events::GameEvents;

namespace {
CelestialBody* Sun() { return Planetarium::fetch->Sun; }
Orbit& OrbitOfBody(CelestialBody* b) {
    Orbit* o = BodyOrbit(b);
    if (o == nullptr) throw NullReferenceException("body " + b->bodyName + " has no orbit");
    return *o;
}
}  // namespace

// ================================================================================================================================ SentinelModule

Part* SentinelModule::PartOf() const { return dynamic_cast<Part*>(part); }

// SentinelModule.cs:17
std::string SentinelModule::FocusName() {
    CelestialBody* innerBody = nullptr;
    CelestialBody* outerBody = nullptr;
    SentinelUtilities::FindInnerAndOuterBodies(*PartOf()->vessel, innerBody, outerBody);
    return Localizer::Format("#autoLOC_7001301", {outerBody->displayName()});
}

// (C++ addition) PartModule.Awake binds the [KSPEvent] methods by reflection; here by name
void SentinelModule::OnAwake() {
    Events().Bind("StartTracking", BaseEvent::Handler([this] { StartTracking(); }));
    Events().Bind("StopTracking", BaseEvent::Handler([this] { StopTracking(); }));
}

// SentinelModule.cs:26
void SentinelModule::OnStart(StartState /*state*/) {
    if (isTracking()) {
        Events()["StartTracking"]->active = false;
        Events()["StopTracking"]->active = true;
    } else {
        Events()["StartTracking"]->active = true;
        Events()["StopTracking"]->active = false;
    }
}

// SentinelModule.cs:41
void SentinelModule::StartTracking() {
    if (TelescopeCanActivate()) {
        ShowMessage(Localizer::Format("#autoLOC_6002285", {SentinelUtilities::SentinelPartTitle(), FocusName()}));
        setIsTracking(true);
        Events()["StartTracking"]->active = false;
        Events()["StopTracking"]->active = true;
        // MonoUtilities.RefreshPartContextWindow(part): the part action window is UI
    }
}

// SentinelModule.cs:54
void SentinelModule::StopTracking() {
    ShowMessage(Localizer::Format("#autoLOC_6002287", {SentinelUtilities::SentinelPartTitle(), FocusName()}));
    setIsTracking(false);
    Events()["StartTracking"]->active = true;
    Events()["StopTracking"]->active = false;
}

// SentinelModule.cs:63
void SentinelModule::FixedUpdate() {
    Vessel* vessel = PartOf()->vessel;
    if (isTracking()) {
        CelestialBody* innerBody = nullptr;
        CelestialBody* outerBody = nullptr;
        if (SentinelUtilities::FindInnerAndOuterBodies(*vessel, innerBody, outerBody))
            Set<std::string>("status", SentinelUtilities::SentinelCanScan(*vessel, innerBody, outerBody) ? Localizer::Format("#autoLOC_6002291", {outerBody->displayName()})
                                                                                                           : Localizer::Format("#autoLOC_6002292", {outerBody->displayName()}));
        else
            Set<std::string>("status", cacheAutoLOC_6002290);
    } else {
        Set<std::string>("status", cacheAutoLOC_6002296);
    }
}

// SentinelModule.cs:82
bool SentinelModule::TelescopeCanActivate() {
    Vessel* vessel = PartOf()->vessel;
    const bool flag = L::HasValidContractObjectives(*vessel, {"Antenna"});
    if (!L::HasValidContractObjectives(*vessel, {"Generator"})) {
        ShowMessage(Localizer::Format("#autoLOC_6002293", {SentinelUtilities::SentinelPartTitle()}));
        return false;
    }
    if (!flag) {
        ShowMessage(Localizer::Format("#autoLOC_6002294", {SentinelUtilities::SentinelPartTitle()}));
        return false;
    }
    if (vessel->orbit().referenceBody != Planetarium::fetch->Sun) {
        ShowMessage(Localizer::Format("#autoLOC_6002295", {SentinelUtilities::SentinelPartTitle()}));
        return false;
    }
    return true;
}

// SentinelModule.cs:103
void SentinelModule::ShowMessage(const std::string& s) {
    L::FinePrintEnvironment::Post(s, SentinelUtilities::CalculateReadDuration(s), L::ScreenMessageStyle::UPPER_CENTER);
}

// SentinelModule.cs:108
void SentinelModule::CacheLocalStrings() {
    cacheAutoLOC_6002290 = Localizer::Format("#autoLOC_6002290");
    cacheAutoLOC_6002296 = Localizer::Format("#autoLOC_6002296");
}

void SentinelModule::Register() {
    ModuleFactory::Register("SentinelModule", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<SentinelModule>(c)); });
}

// ================================================================================================================================ SentinelScenario

SentinelScenario::SentinelScenario() : ScenarioModule("SentinelScenario") {}
SentinelScenario::~SentinelScenario() {
    if (Instance == this) Instance = nullptr;
}

double SentinelScenario::NextSpawnTime() const {
    // ScenarioModule::Store is non-const
    return const_cast<SentinelScenario*>(this)->Store()->Get<double>("NextSpawnTime");
}
void SentinelScenario::setNextSpawnTime(double v) { Store()->Set<double>("NextSpawnTime", v); }

// SentinelScenario.cs:29
void SentinelScenario::OnAwake() {
    if (Instance != nullptr && Instance != this) {
        log::Error("[SentinelScenario]: Instance already exists!");
        Instance->OnDestroy();  // UnityEngine.Object.Destroy(Instance)
    }
    Instance = this;
    deployedSentinels.clear();
}

// SentinelScenario.cs:40
void SentinelScenario::Start() {
    if (generator == nullptr) generator = std::make_unique<KSPRandom>(Contracts::ContractsEnvironment::Current().NewRandomSeed());
    deployedSentinels = GetDeployedSentinels();
    routines.push_back(CareerEnvironment::Current().Scheduler().StartCoroutine(SentinelRoutine()));
}

// SentinelScenario.cs:50
void SentinelScenario::OnLoad(ConfigNode& /*node*/) {
    std::vector<const ConfigNode*> nodes;
    if (GameDatabase::Instance() != nullptr)
        for (ConfigNode* n : GameDatabase::Instance()->GetConfigNodes("SPACEOBJECTS_LIMITS")) nodes.push_back(n);
    LoadGameDBSettings(nodes);
}

// SentinelScenario.cs:55
void SentinelScenario::LoadGameDBSettings(const std::vector<const ConfigNode*>& configNodes) {
    const ConfigNode* configNode = nullptr;
    if (!configNodes.empty()) {
        configNode = configNodes[0];
        if (configNodes.size() > 1) log::Warning("[ScenarioDiscoverabelObjects]: Found more than one SPACEOBJECTS_LIMITS node using the first one");
    }
    if (configNode == nullptr)
        log::Warning("[ScenarioDiscoverabelObjects]: No SPACEOBJECTS_LIMITS node found. Using default values");
    else
        configNode->TryGetValue("perSentinelSpawnGroupMaxLimit", SentinelUtilities::perSentinelObjectLimit);
}

// SentinelScenario.cs:77
void SentinelScenario::OnDestroy() {
    for (auto& c : routines) CareerEnvironment::Current().Scheduler().StopCoroutine(c);
    routines.clear();
    deployedSentinels.clear();
    destroyed_ = true;
}

// SentinelScenario.cs:82
Enumerator SentinelScenario::SentinelRoutine() {
    while (!destroyed_) {
        if (Planetarium::GetUniversalTime() >= NextSpawnTime()) {
            deployedSentinels = GetDeployedSentinels();
            for (size_t i = 0; i < deployedSentinels.size(); ++i) {
                Vessel* v = deployedSentinels[i];
                routines.push_back(CareerEnvironment::Current().Scheduler().StartCoroutine(ProcessSentinelScan(v)));
            }
            setNextSpawnTime(Planetarium::GetUniversalTime() + static_cast<double>(KSPUtil::dateTimeFormatter().Day()) * SentinelUtilities::RandomRange(generator.get(), 0.5, 1.5));
        }
        while (Planetarium::GetUniversalTime() < NextSpawnTime()) co_yield YieldInstruction::Null();
    }
}

// SentinelScenario.cs:103
Enumerator SentinelScenario::ProcessSentinelScan(Vessel* v) {
    if (generator->NextDouble() > static_cast<double>(SentinelUtilities::SpawnChance)) co_return;
    const int dayTime = KSPUtil::dateTimeFormatter().Day();
    const double processTime = Planetarium::GetUniversalTime() + SentinelUtilities::RandomRange(generator.get(), 0.0, static_cast<double>(dayTime) / 3.0);
    while (Planetarium::GetUniversalTime() < processTime) co_yield YieldInstruction::Null();
    const int num = static_cast<int>(deployedSentinels.size()) * SentinelUtilities::perSentinelObjectLimit;
    const int spaceObjects = SentinelEnvironment::countSpaceObjects ? SentinelEnvironment::countSpaceObjects() : 0;
    const int untracked = SentinelEnvironment::GetDiscoverableObjects().has_value() ? SentinelEnvironment::GetDiscoverableObjects()->untrackedObjectIDsCount : 0;
    if (spaceObjects - untracked >= num) co_yield YieldInstruction::Null();
    if (SentinelEnvironment::shouldSpawnComet && SentinelEnvironment::shouldSpawnComet()) {
        SpawnComet(v);
        co_return;
    }
    CelestialBody* innerBody = nullptr;
    CelestialBody* outerBody = nullptr;
    SentinelUtilities::FindInnerAndOuterBodies(v->orbit().semiMajorAxis, innerBody, outerBody);
    Orbit orbit = SentinelAsteroidOrbit(v->orbit());
    UntrackedObjectClass untrackedObjectClass = SentinelUtilities::WeightedAsteroidClass(generator.get());
    if (SentinelEnvironment::spawnAsteroid) {
        const std::string name = SentinelEnvironment::generateAsteroidName ? SentinelEnvironment::generateAsteroidName() : std::string();
        const uint32_t seed = dotnet::ConvToUInt32(SentinelUtilities::RandomRange(generator.get()));
        SentinelEnvironment::spawnAsteroid(name, orbit, seed, untrackedObjectClass,
                                           SentinelUtilities::RandomRange(generator.get(), static_cast<double>(dayTime * 4), static_cast<double>(dayTime * 80)),
                                           static_cast<double>(dayTime * 80));
    } else {
        // the argument draws happen whether or not a host spawns the asteroid
        (void)SentinelUtilities::RandomRange(generator.get());
        (void)SentinelUtilities::RandomRange(generator.get(), static_cast<double>(dayTime * 4), static_cast<double>(dayTime * 80));
    }
    if (Contracts::ContractSystem::Instance != nullptr) {
        std::vector<SentinelContract*> currentContracts = Contracts::ContractSystem::Instance->GetCurrentContracts<SentinelContract>();
        for (size_t i = 0; i < currentContracts.size(); ++i) {
            auto* p = currentContracts[i]->GetParameterT<SentinelParameter>();
            if (p != nullptr) p->DiscoverAsteroid(untrackedObjectClass, orbit.eccentricity, orbit.inclination, outerBody);
        }
    }
}

// SentinelScenario.cs:148
void SentinelScenario::SpawnComet() {
    if (!deployedSentinels.empty()) SpawnComet(deployedSentinels[0]);
}

// SentinelScenario.cs:154
void SentinelScenario::SpawnComet(Vessel* sentinel) {
    if (!SentinelEnvironment::generateWeightedCometType) return;
    std::shared_ptr<ICometOrbitType> cometType = SentinelEnvironment::generateWeightedCometType();
    if (cometType) SpawnComet(sentinel, *cometType);
}

// SentinelScenario.cs:167
void SentinelScenario::SpawnComet(Vessel* sentinel, const std::string& typeName) {
    std::shared_ptr<ICometOrbitType> cometOrbitType = SentinelEnvironment::getCometOrbitType ? SentinelEnvironment::getCometOrbitType(typeName) : nullptr;
    if (cometOrbitType == nullptr)
        log::Warning("[SentinelScenario] Unable to find Spawn Comet Type: " + typeName);
    else
        SpawnComet(sentinel, *cometOrbitType);
}

// SentinelScenario.cs:167
void SentinelScenario::SpawnComet(Vessel* sentinel, ICometOrbitType& cometType) {
    UnityRandom& rnd = UnityRandom::Global();
    const int seed = rnd.Range(0, 2147483647);
    rnd.InitState(seed);
    const SentinelEnvironment::DiscoverableObjects dobj = SentinelEnvironment::GetDiscoverableObjects().value_or(SentinelEnvironment::DiscoverableObjects{});
    const double lifeTime = static_cast<double>(rnd.Range(dobj.minUntrackedLifetime, dobj.maxUntrackedLifetime)) * 24.0 * 60.0 * 60.0;
    const double lifeTimeMax = static_cast<double>(dobj.maxUntrackedLifetime) * 24.0 * 60.0 * 60.0;
    const UntrackedObjectClass randomObjClass = cometType.GetRandomObjClass();
    SentinelEnvironment::CometDefinitionSeam cometDefinition;
    if (SentinelEnvironment::generateCometDefinition) cometDefinition = SentinelEnvironment::generateCometDefinition(cometType, randomObjClass, seed);
    const double minDiscoveryDistance = OrbitOfBody(Planetarium::fetch->Home).semiMajorAxis * cometDefinition.vfxStartDistance;
    bool validOrbit = false;
    Orbit o = cometType.CalculateSentinelOrbit(sentinel->orbit(), minDiscoveryDistance, validOrbit);
    if (validOrbit) {
        const uint32_t cometSeed = dotnet::ConvToUInt32(SentinelUtilities::RandomRange(generator.get()));
        if (SentinelEnvironment::spawnComet)
            SentinelEnvironment::spawnComet("UnknownComet", o, cometDefinition, cometSeed, randomObjClass, lifeTime, lifeTimeMax, false, 0.0f);
        GE::onSentinelCometDetected.Fire(sentinel);
    }
}

// SentinelScenario.cs:185
std::vector<Vessel*> SentinelScenario::GetDeployedSentinels() {
    std::vector<Vessel*> list;
    for (size_t i = 0; i < FlightRegistry::Vessels.size(); ++i) {
        Vessel* vessel = FlightRegistry::Vessels[i];
        if (vessel->loaded) {
            for (size_t j = 0; j < vessel->parts.size(); ++j) {
                Part* part = vessel->parts[j];
                if (!(L::GetPartName(part) != SentinelUtilities::SentinelPartName)) {
                    PartModule* m = part->FindModuleImplementing([](PartModule& pm) { return dynamic_cast<SentinelModule*>(&pm) != nullptr; });
                    auto* sentinelModule = dynamic_cast<SentinelModule*>(m);
                    if (sentinelModule != nullptr && sentinelModule->isTracking()) {
                        list.push_back(vessel);
                        break;
                    }
                }
            }
            continue;
        }
        bool flag = false;
        for (size_t k = 0; k < vessel->protoVessel->protoPartSnapshots.size(); ++k) {
            ProtoPartSnapshot* protoPartSnapshot = vessel->protoVessel->protoPartSnapshots[k].get();
            if (protoPartSnapshot->partName.value_or("") != SentinelUtilities::SentinelPartName) continue;
            if (flag) break;
            for (size_t l = 0; l < protoPartSnapshot->modules.size(); ++l) {
                ProtoPartModuleSnapshot* protoPartModuleSnapshot = protoPartSnapshot->modules[l].get();
                if (!(protoPartSnapshot->partName.value_or("") != SentinelUtilities::SentinelPartName) &&
                    !(protoPartModuleSnapshot->moduleName.value_or("") != SentinelUtilities::SentinelModuleName)) {
                    bool value = false;
                    L::LoadNode(protoPartModuleSnapshot->moduleValues.get(), SentinelUtilities::SentinelModuleName.c_str(), "isTracking", value, false);
                    if (value) {
                        list.push_back(vessel);
                        flag = true;
                        break;
                    }
                }
            }
        }
    }
    for (int num = static_cast<int>(list.size()) - 1; num >= 0; num--)
        if (!SentinelUtilities::SentinelCanScan(*list[static_cast<size_t>(num)])) list.erase(list.begin() + num);
    return list;
}

// SentinelScenario.cs:247
bool SentinelScenario::ActiveSentinelMatchingOrbit(Orbit* orbit, double deviationWindow) {
    std::vector<Vessel*> list = GetDeployedSentinels();
    for (size_t num = 0; num < list.size(); ++num) {
        Vessel* vessel = list[num];
        // (vessel.loaded ? vessel.orbit : vessel.protoVessel.orbitSnapShot.Load()) != null: both exist
        if (L::VesselAtOrbit(orbit, deviationWindow, vessel)) return true;
    }
    return false;
}

// SentinelScenario.cs:269
void SentinelScenario::ResetSentinels() {
    std::vector<Vessel*> list = GetDeployedSentinels();
    for (size_t i = 0; i < list.size(); ++i) {
        Vessel* vessel = list[i];
        if (vessel->loaded) {
            for (size_t j = 0; j < vessel->parts.size(); ++j) {
                Part* part = vessel->parts[j];
                if (!(L::GetPartName(part) != SentinelUtilities::SentinelPartName)) {
                    PartModule* m = part->FindModuleImplementing([](PartModule& pm) { return dynamic_cast<SentinelModule*>(&pm) != nullptr; });
                    auto* sentinelModule = dynamic_cast<SentinelModule*>(m);
                    if (sentinelModule != nullptr) sentinelModule->setIsTracking(false);
                }
            }
            continue;
        }
        for (size_t k = 0; k < vessel->protoVessel->protoPartSnapshots.size(); ++k) {
            ProtoPartSnapshot* protoPartSnapshot = vessel->protoVessel->protoPartSnapshots[k].get();
            if (protoPartSnapshot->partName.value_or("") != SentinelUtilities::SentinelPartName) continue;
            for (size_t l = 0; l < protoPartSnapshot->modules.size(); ++l) {
                ProtoPartModuleSnapshot* protoPartModuleSnapshot = protoPartSnapshot->modules[l].get();
                if (!(protoPartSnapshot->partName.value_or("") != SentinelUtilities::SentinelPartName)) {
                    ConfigNode::NodePtr configNode = protoPartModuleSnapshot->moduleValues->CreateCopy();
                    configNode->SetValue("isTracking", false);
                    configNode->CopyTo(*protoPartModuleSnapshot->moduleValues, true);
                }
            }
        }
    }
}

// SentinelScenario.cs:312
Orbit SentinelScenario::SentinelAsteroidOrbit(const Orbit& orbit) {
    Orbit orbit2(orbit.inclination, orbit.eccentricity, orbit.semiMajorAxis, orbit.LAN, orbit.argumentOfPeriapsis, orbit.meanAnomalyAtEpoch, orbit.epoch, orbit.referenceBody);
    orbit2.UpdateFromStateVectors(orbit.pos, orbit.vel, orbit.referenceBody, Planetarium::GetUniversalTime());
    CelestialBody* innerBody = nullptr;
    CelestialBody* outerBody = nullptr;
    if (SentinelUtilities::FindInnerAndOuterBodies(orbit.semiMajorAxis, innerBody, outerBody)) {
        const float h = f32store(static_cast<float>(OrbitOfBody(outerBody).semiMajorAxis) * 0.1f);
        orbit2.semiMajorAxis = M::Max(orbit2.semiMajorAxis, OrbitOfBody(outerBody).semiMajorAxis + SentinelUtilities::RandomRange(generator.get(), static_cast<double>(f32store(0.0f - h)), static_cast<double>(h)));
    }
    orbit2.Init();
    orbit2.UpdateFromUT(Planetarium::GetUniversalTime());
    Vector3d vel;
    if (L::CoinFlip(generator.get())) {
        const double num = SentinelUtilities::WeightedRandom(generator.get()) * SentinelUtilities::GetProgradeBurnAllowance(orbit2);
        vel = orbit2.vel + orbit2.getOrbitalVelocityAtUT(Planetarium::GetUniversalTime()).normalized() * num;
    } else {
        const double num2 = SentinelUtilities::WeightedRandom(generator.get()) * SentinelUtilities::GetRetrogradeBurnAllowance(orbit2);
        vel = orbit2.vel + orbit2.getOrbitalVelocityAtUT(Planetarium::GetUniversalTime()).normalized() * (0.0 - num2);
    }
    orbit2.UpdateFromStateVectors(orbit2.pos, vel, orbit2.referenceBody, Planetarium::GetUniversalTime());
    const double num3 = SentinelUtilities::AdjustedSentinelViewAngle(orbit, OrbitOfBody(outerBody)) / 2.0;
    orbit2.meanAnomalyAtEpoch += (L::CoinFlip(generator.get()) ? SentinelUtilities::WeightedRandom(generator.get(), num3 * (3.1415926535897931 / 180.0))
                                                               : (0.0 - SentinelUtilities::WeightedRandom(generator.get(), num3 * (3.1415926535897931 / 180.0))));
    orbit2.inclination += (L::CoinFlip(generator.get()) ? SentinelUtilities::WeightedRandom(generator.get(), num3) : (0.0 - SentinelUtilities::WeightedRandom(generator.get(), num3)));
    return orbit2;
}

void SentinelScenario::Register() {
    ScenarioModuleFactory::Register("SentinelScenario", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new SentinelScenario()); });
}

}  // namespace ksp::SentinelMission
