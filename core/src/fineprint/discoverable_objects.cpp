// WP-38d: DiscoverableObjectsUtil.cs, ScenarioDiscoverableObjects.cs and the proto-vessel node builders they call. See include/ksp/fineprint/space_objects.hpp.
#include <algorithm>
#include <cmath>

#include "ksp/career/game_variables.hpp"
#include "ksp/career/progress.hpp"
#include "ksp/career/upgradeables.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/config/kspparseable.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/career/career_environment.hpp"
#include "ksp/database/game_database.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/fineprint/contracts.hpp"
#include "ksp/fineprint/space_objects.hpp"
#include "ksp/kerbals/crew_generator.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/inventory/inventory.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

namespace M = ksp::dotnet::Math;
using GE = events::GameEvents;
using KSP::Localization::Localizer;

namespace {
UnityRandom& Rnd() { return UnityRandom::Global(); }
double UT() { return Planetarium::GetUniversalTime(); }
}  // namespace

// ================================================================================================================================ SpaceObjectNodes

// ProtoVessel.cs:853
ConfigNode::NodePtr SpaceObjectNodes::CreateVesselNode(const std::string& vesselName, VesselType vesselType, const Orbit& orbit, int rootPartIndex,
                                                       const std::vector<ConfigNode::NodePtr>& partNodes,
                                                       const std::vector<ConfigNode::NodePtr>& additionalNodes) {
    const Vector3d positionAtUT = const_cast<Orbit&>(orbit).getPositionAtUT(UT());
    double lat = 0.0, lon = 0.0, alt = 0.0;
    orbit.referenceBody->GetLatLonAlt(positionAtUT, lat, lon, alt);
    const uint32_t uniquepersistentId = SaveEnvironment::Current().persistentIds.GetUniquepersistentId();
    auto configNode = ConfigNode::Create();
    configNode->AddValue("pid", Contracts::ContractsEnvironment::Current().NewGuid().ToString('N'));
    configNode->AddValue("name", vesselName);
    configNode->AddValue("type", dotnet::EnumToString(vesselType));
    configNode->AddValue("persistentId", uniquepersistentId);
    configNode->AddValue("sit", std::string("ORBITING"));  // Vessel.Situations.ORBITING.ToString()
    configNode->AddValue("landed", false);
    configNode->AddValue("skipGroundPositioning", false);
    configNode->AddValue("vesselSpawning", true);
    configNode->AddValue("landedAt", "");
    configNode->AddValue("splashed", false);
    configNode->AddValue("alt", alt);
    configNode->AddValue("lat", lat);
    configNode->AddValue("lon", lon);
    configNode->AddValue("hgt", -1.0f);
    configNode->AddValue("nrm", KSPUtil::WriteVector(Vector3f::up()));
    configNode->AddValue("rot", KSPUtil::WriteQuaternion(Rnd().rotation()));
    configNode->AddValue("met", UT());
    configNode->AddValue("lct", UT());
    configNode->AddValue("root", static_cast<int32_t>(rootPartIndex));
    configNode->AddValue("PQSMin", 0);
    configNode->AddValue("PQSMax", 0);
    configNode->AddValue("altDispState", std::string("DEFAULT"));
    configNode->AddNode(ProtoVessel::CreateOrbitNode(orbit, FlightRegistry::Bodies()));
    for (const auto& p : partNodes) configNode->AddNode(p);
    for (const auto& p : additionalNodes) configNode->AddNode(p);
    if (!configNode->HasNode("ACTIONGROUPS")) configNode->AddNode("ACTIONGROUPS");
    return configNode;
}

// ProtoVessel.cs:902 (public CreatePartNode(partName, id, params crew): the prefab path). ProtoVesselNodes::CreatePartNode takes the persistentId from the WP-33
// InventoryEnvironment seam (unset: 0); FlightGlobals.GetUniquepersistentId() is drawn here and replaces the value in place (same position in the node).
ConfigNode::NodePtr SpaceObjectNodes::CreatePartNode(const std::string& partName, uint32_t id) {
    ConfigNode::NodePtr configNode = ProtoVesselNodes::CreatePartNode(nullptr, partName, id, {});
    configNode->SetValue("persistentId", SaveEnvironment::Current().persistentIds.GetUniquepersistentId());
    return configNode;
}

// ProtoVessel.cs:982
ConfigNode::NodePtr SpaceObjectNodes::CreateDiscoveryNode(DiscoveryLevels level, UntrackedObjectClass size, double lifeTime, double maxLifeTime) {
    auto configNode = ConfigNode::Create("DISCOVERY");
    DiscoveryInfo obj(nullptr, lifeTime);
    obj.SetLevel(level);
    obj.SetUntrackedObjectSize(size);
    obj.SetLastObservedTime(UT());
    obj.SetReferenceLifetime(maxLifeTime);
    obj.Save(*configNode);
    return configNode;
}

// ================================================================================================================================ SpaceObjectsEnvironment

void SpaceObjectsEnvironment::Reset() {
    addVessel = nullptr;
    spawned.clear();
    addedNodes.clear();
    bodyReached = nullptr;
    discoveryUnlocked = nullptr;
    persistentVesselIdKnown = nullptr;
    gameDatabaseConfigNodes = nullptr;
}

namespace {
// HighLogic.CurrentGame.AddVessel(protoVesselNode) (Game.cs:695)
AddedVessel AddVessel(const ConfigNode& protoVesselNode) {
    SpaceObjectsEnvironment::addedNodes.push_back(protoVesselNode.CreateCopy());
    if (SpaceObjectsEnvironment::addVessel) return SpaceObjectsEnvironment::addVessel(protoVesselNode);
    Game* game = SaveEnvironment::Current().currentGame;
    SpaceObjectsEnvironment::Spawned s;
    s.protoVessel = std::make_unique<ProtoVessel>(protoVesselNode, game);
    AddedVessel added;
    added.protoVessel = s.protoVessel.get();
    added.persistentId = s.protoVessel->persistentId;
    s.vessel = s.protoVessel->Load(game != nullptr ? game->flightState.get() : nullptr);
    added.vessel = s.vessel.get();
    GE::onNewVesselCreated.Fire(added.vessel);
    SpaceObjectsEnvironment::spawned.push_back(std::move(s));
    return added;
}
}  // namespace

// ================================================================================================================================ DiscoverableObjectsUtil

// DiscoverableObjectsUtil.cs:9
std::string DiscoverableObjectsUtil::GenerateAsteroidName() {
    std::string text;
    text += randomLetter(true);
    text += randomLetter(true);
    text += randomLetter(true);
    text += "-";
    text += randomNumber(3);
    return Localizer::Format("#autoLOC_6001923", {text});
}

// DiscoverableObjectsUtil.cs:20
char DiscoverableObjectsUtil::randomLetter(bool uppercase) {
    const int num = Rnd().Range(0, 26);
    char c = static_cast<char>(97 + num);
    if (uppercase) return static_cast<char>(c - 32);  // char.ToUpper of a-z
    return c;
}

// DiscoverableObjectsUtil.cs:31 (int.ToString("000"): zero padded to `digits`)
std::string DiscoverableObjectsUtil::randomNumber(int digits) {
    const int max = dotnet::ConvToInt32(M::Pow(10.0, static_cast<double>(digits)));
    std::string text = std::to_string(Rnd().Range(0, max));
    while (static_cast<int>(text.size()) < digits) text = "0" + text;
    return text;
}

// DiscoverableObjectsUtil.cs:42
AddedVessel DiscoverableObjectsUtil::SpawnAsteroid(const std::string& asteroidName, const Orbit& o, uint32_t seed, UntrackedObjectClass objClass, double lifeTime,
                                                   double lifeTimeMax) {
    // C# argument order: the part node array, new ConfigNode("ACTIONGROUPS"), then the discovery node
    std::vector<ConfigNode::NodePtr> partNodes{SpaceObjectNodes::CreatePartNode("PotatoRoid", seed)};
    auto actionGroups = ConfigNode::Create("ACTIONGROUPS");
    ConfigNode::NodePtr discovery = SpaceObjectNodes::CreateDiscoveryNode(DiscoveryLevels::Presence, objClass, lifeTime, lifeTimeMax);
    ConfigNode::NodePtr protoVesselNode = SpaceObjectNodes::CreateVesselNode(asteroidName, VesselType::SpaceObject, o, 0, partNodes, {actionGroups, discovery});
    AddedVessel protoVessel = AddVessel(*protoVesselNode);
    GE::onAsteroidSpawned.Fire(protoVessel.vessel);
    return protoVessel;
}

// DiscoverableObjectsUtil.cs:50
AddedVessel DiscoverableObjectsUtil::SpawnComet(const std::string& cometName, const Orbit& o, const CometDefinition& cometDef, uint32_t seed, UntrackedObjectClass objClass,
                                                double lifeTime, double lifeTimeMax, bool optimizedCollider, float fragmentDynamicPressureModifier) {
    ConfigNode::NodePtr configNode = cometDef.CreateVesselNode(optimizedCollider, fragmentDynamicPressureModifier, false);
    ConfigNode::NodePtr configNode2 = SpaceObjectNodes::CreatePartNode("PotatoComet", seed);
    uint32_t value = 0u;
    configNode2->TryGetValue("persistentId", value);
    configNode->AddValue("cometPartId", value);
    auto configNode3 = ConfigNode::Create("VESSELMODULES");
    configNode3->AddNode(configNode);
    auto actionGroups = ConfigNode::Create("ACTIONGROUPS");
    ConfigNode::NodePtr discovery = SpaceObjectNodes::CreateDiscoveryNode(DiscoveryLevels::Presence, objClass, lifeTime, lifeTimeMax);
    ConfigNode::NodePtr protoVesselNode = SpaceObjectNodes::CreateVesselNode(cometName, VesselType::SpaceObject, o, 0, {configNode2}, {actionGroups, discovery, configNode3});
    AddedVessel protoVessel = AddVessel(*protoVesselNode);
    GE::onCometSpawned.Fire(protoVessel.vessel);
    return protoVessel;
}

// DiscoverableObjectsUtil.cs:65 (the C# `generator as KSPRandom`: a non-KSPRandom generator would be null there; the parameter is a KSPRandom already)
std::string DiscoverableObjectsUtil::GenerateCometName(KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    KSPRandom* kSPRandom = generator;
    if (kSPRandom == nullptr) {
        own = std::make_unique<KSPRandom>(Contracts::ContractsEnvironment::Current().NewRandomSeed());
        kSPRandom = own.get();
    }
    Game* game = SaveEnvironment::Current().currentGame;
    if (game != nullptr) {
        double num = 0.0;
        num = 1.0 - static_cast<double>(game->cometNames.Count()) / 10.0;
        (void)M::Max(num, 0.1);  // sic: the result is discarded
        if (kSPRandom->NextDouble() > num) {
            const int index = kSPRandom->Next(0, game->cometNames.Count() - 1);
            return game->GetCometNumberedName(game->cometNames.KeysList()[static_cast<size_t>(index)]);
        }
    }
    const std::optional<std::vector<std::string>>* array = nullptr;
    const std::optional<std::vector<std::string>>* array2 = nullptr;
    if (kSPRandom->Next(0, 9) < 5) {
        array = &CrewGenerator::SpecialNamesMale();
        array2 = &CrewGenerator::KerbalNamesMale();
    } else {
        array = &CrewGenerator::SpecialNamesFemale();
        array2 = &CrewGenerator::KerbalNamesFemale();
    }
    if (!array->has_value() || !array2->has_value()) throw NullReferenceException("CrewGenerator name tables");
    const std::vector<std::string>& special = **array;
    const std::vector<std::string>& normal = **array2;
    auto pick = [&]() -> std::string {
        if (kSPRandom->Next(specialNameChance) != 0) return normal.at(static_cast<size_t>(kSPRandom->Next(static_cast<int32_t>(normal.size()))));
        return special.at(static_cast<size_t>(kSPRandom->Next(static_cast<int32_t>(special.size()))));
    };
    std::string empty = pick();
    if (kSPRandom->Next(0, 100) < 24) {
        const std::string text = pick();
        empty = empty + "-" + text;
        if (kSPRandom->Next(0, 100) < 24) {
            const std::string text2 = pick();
            empty = empty + "-" + text2;
        }
    }
    if (game != nullptr) return game->GetCometNumberedName(empty);
    return empty + " 1";
}

// ================================================================================================================================ ScenarioDiscoverableObjects

ScenarioDiscoverableObjects::ScenarioDiscoverableObjects() : ScenarioModule("ScenarioDiscoverableObjects") {}

ScenarioDiscoverableObjects::~ScenarioDiscoverableObjects() {
    if (Instance == this) Instance = nullptr;
    if (!destroyed_) OnDestroy();
}

// (C++ accessors of the persistent FloatCurve field sizeCurve; monostate = null before OnAwake)
bool ScenarioDiscoverableObjects::hasSizeCurve() { return std::holds_alternative<FloatCurve>(*Store()->Value("sizeCurve")); }
FloatCurve& ScenarioDiscoverableObjects::sizeCurve() { return std::get<FloatCurve>(*Store()->Value("sizeCurve")); }
// ScenarioDiscoverableObjects.cs:31
int ScenarioDiscoverableObjects::lastSeed() { return Store()->Get<int32_t>("lastSeed"); }
void ScenarioDiscoverableObjects::setLastSeed(int v) { Store()->Set<int32_t>("lastSeed", static_cast<int32_t>(v)); }

// ScenarioDiscoverableObjects.cs:57
void ScenarioDiscoverableObjects::OnLoad(ConfigNode& node) {
    std::vector<const ConfigNode*> nodes;
    for (ConfigNode* n : CometManager::GetConfigNodes("SPACEOBJECTS_LIMITS")) nodes.push_back(n);
    LoadGameDBSettings(nodes);
    untrackedObjectIDs = loadIDList(node, "untrackedIDs");
    discoveredObjectIDs = loadIDList(node, "discoveredIDs");
}

// ScenarioDiscoverableObjects.cs:64
void ScenarioDiscoverableObjects::LoadGameDBSettings(const std::vector<const ConfigNode*>& configNodes) {
    const ConfigNode* configNode = nullptr;
    if (!configNodes.empty()) {
        configNode = configNodes[0];
        if (configNodes.size() > 1) log::Warning("[ScenarioDiscoverabelObjects]: Found more than one SPACEOBJECTS_LIMITS node using the first one");
    }
    if (configNode == nullptr) {
        log::Warning("[ScenarioDiscoverabelObjects]: No SPACEOBJECTS_LIMITS node found. Using default values");
        return;
    }
    int32_t v = spawnGroupMinLimit;
    configNode->TryGetValue("homeBodySpawnGroupMinLimit", v);
    spawnGroupMinLimit = v;
    v = spawnGroupMaxLimit;
    configNode->TryGetValue("homeBodySpawnGroupMaxLimit", v);
    spawnGroupMaxLimit = v;
}

// ScenarioDiscoverableObjects.cs:85 (uint.TryParse of each comma separated item; the C# keeps the last parsed text in fields)
std::vector<uint32_t> ScenarioDiscoverableObjects::loadIDList(const ConfigNode& node, const std::string& listName) {
    std::vector<uint32_t> list;
    std::string tmpIDs;
    node.TryGetValue(listName, tmpIDs);
    const std::vector<std::string> tmpIDList = dotnet::Split(tmpIDs, ",", false);
    for (size_t i = 0; i < tmpIDList.size(); ++i) {
        uint32_t tmpId = 0u;
        if (dotnet::TryParseUInt32(tmpIDList[i], tmpId)) list.push_back(tmpId);
    }
    return list;
}

// ScenarioDiscoverableObjects.cs:102
void ScenarioDiscoverableObjects::OnSave(ConfigNode& node) {
    SaveIDList(node, "untrackedIDs", untrackedObjectIDs);
    SaveIDList(node, "discoveredIDs", discoveredObjectIDs);
}

// ScenarioDiscoverableObjects.cs:108
void ScenarioDiscoverableObjects::SaveIDList(ConfigNode& node, const std::string& name, const std::vector<uint32_t>& loadList) {
    std::string tmpIDs;
    for (size_t i = 0; i < loadList.size(); ++i) {
        if (i > 0) tmpIDs += ",";
        tmpIDs += std::to_string(loadList[i]);
    }
    node.AddValue(name, tmpIDs);
}

// ScenarioDiscoverableObjects.cs:114
void ScenarioDiscoverableObjects::OnAwake() {
    if (Instance != nullptr && Instance != this) {
        log::Error("[ScenarioDiscoverableObjects]: Instance already exists!");
        destroyed_ = true;  // UnityEngine.Object.Destroy(this)
        return;
    }
    Instance = this;
    if (!hasSizeCurve()) {
        FloatCurve curve;
        curve.Add(0.0f, 0.0f);
        curve.Add(0.3f, 0.45f);
        curve.Add(0.7f, 0.55f);
        curve.Add(1.0f, 1.0f);
        Store()->Set<FloatCurve>("sizeCurve", curve);
    }
    discoveredObjectIDs.clear();
    untrackedObjectIDs.clear();
    GE::onKnowledgeChanged.Add(this, &ScenarioDiscoverableObjects::OnKnowledgeChanged);
    GE::onVesselDestroy.Add(this, &ScenarioDiscoverableObjects::OnVesselDestroy);
}

// ScenarioDiscoverableObjects.cs:137
void ScenarioDiscoverableObjects::OnKnowledgeChanged(GE::HostedFromToAction<IDiscoverable*, DiscoveryLevels> kChg) {
    Vessel* vessel = dynamic_cast<Vessel*>(kChg.host);
    if (vessel == nullptr) return;
    if (std::find(discoveredObjectIDs.begin(), discoveredObjectIDs.end(), vessel->persistentId) == discoveredObjectIDs.end()) return;
    if (vessel->GetDiscoveryInfo().HaveKnowledgeAbout(DiscoveryLevels::StateVectors)) {
        auto it = std::find(untrackedObjectIDs.begin(), untrackedObjectIDs.end(), vessel->persistentId);
        if (it != untrackedObjectIDs.end()) untrackedObjectIDs.erase(it);  // List.Remove: the first match
    } else if (std::find(untrackedObjectIDs.begin(), untrackedObjectIDs.end(), vessel->persistentId) == untrackedObjectIDs.end()) {
        untrackedObjectIDs.push_back(vessel->persistentId);  // AddUnique
    }
}

// ScenarioDiscoverableObjects.cs:153
void ScenarioDiscoverableObjects::OnVesselDestroy(Vessel* v) {
    auto it = std::find(discoveredObjectIDs.begin(), discoveredObjectIDs.end(), v->persistentId);
    if (it != discoveredObjectIDs.end()) discoveredObjectIDs.erase(it);
    it = std::find(untrackedObjectIDs.begin(), untrackedObjectIDs.end(), v->persistentId);
    if (it != untrackedObjectIDs.end()) untrackedObjectIDs.erase(it);
}

// ScenarioDiscoverableObjects.cs:159
void ScenarioDiscoverableObjects::Start() {
    Rnd().InitState(lastSeed());
    discoveryUnlocked_ = SpaceObjectsEnvironment::discoveryUnlocked
                             ? SpaceObjectsEnvironment::discoveryUnlocked()
                             : GameVariables::Instance->UnlockedSpaceObjectDiscovery(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::TrackingStation));
    int count = static_cast<int>(untrackedObjectIDs.size());
    while (count-- > 0) {
        const uint32_t id = untrackedObjectIDs[static_cast<size_t>(count)];
        const bool known = SpaceObjectsEnvironment::persistentVesselIdKnown ? SpaceObjectsEnvironment::persistentVesselIdKnown(id)
                                                                           : SaveEnvironment::Current().persistentIds.persistentVesselIds.ContainsKey(id);
        if (!known) untrackedObjectIDs.erase(untrackedObjectIDs.begin() + count);
    }
    routines.push_back(CareerEnvironment::Current().Scheduler().StartCoroutine(SpawnDaemon()));
}

// ScenarioDiscoverableObjects.cs:174
void ScenarioDiscoverableObjects::OnDestroy() {
    if (Instance == this) Instance = nullptr;  // a destroyed Unity object compares equal to null (`Instance != null` in the next OnAwake)
    GE::onKnowledgeChanged.Remove(this, &ScenarioDiscoverableObjects::OnKnowledgeChanged);
    GE::onVesselDestroy.Remove(this, &ScenarioDiscoverableObjects::OnVesselDestroy);
    for (auto& c : routines) CareerEnvironment::Current().Scheduler().StopCoroutine(c);
    routines.clear();
    discoveredObjectIDs.clear();
    untrackedObjectIDs.clear();
    destroyed_ = true;
}

// ScenarioDiscoverableObjects.cs:182
Enumerator ScenarioDiscoverableObjects::SpawnDaemon() {
    while (!destroyed_) {
        if (!ExpireDiscoveredObjects(Planetarium::GetUniversalTime())) UpdateSpaceObjects();
        co_yield YieldInstruction::WaitForSeconds(Mathf::Max(0.1f, f32store(spawnInterval / TimeWarp::CurrentRate())));
    }
}

// ScenarioDiscoverableObjects.cs:194
void ScenarioDiscoverableObjects::UpdateSpaceObjects() {
    if (!discoveryUnlocked_) return;
    rndGroupSize_ = Rnd().Range(spawnGroupMinLimit, spawnGroupMaxLimit);
    newGroupSize_ = std::max(currentSize_, rndGroupSize_);
    if (newGroupSize_ > currentSize_) {
        if (CometManager::ShouldSpawnComet()) {
            SpawnComet();
        } else if (Rnd().Range(0, spawnOddsAgainst) == 0) {
            SpawnAsteroid();
        } else {
            log::Info("[AsteroidSpawner]: No new objects this time. (Odds are 1:" + std::to_string(spawnOddsAgainst) + ")");
        }
    }
}

// ScenarioDiscoverableObjects.cs:219
bool ScenarioDiscoverableObjects::ExpireDiscoveredObjects(double UT_) {
    bool flag = false;
    int num = 0;
    currentSize_ = 0;
    for (int num2 = static_cast<int>(FlightRegistry::Vessels.size()) - 1; num2 >= 0; num2--) {
        Vessel* vessel = FlightRegistry::Vessels[static_cast<size_t>(num2)];
        if (vessel != nullptr && !vessel->GetDiscoveryInfo().HaveKnowledgeAbout(DiscoveryLevels::StateVectors)) {
            if (vessel->GetDiscoveryInfo().GetSignalLife(UT_) == 0.0) {
                if (!flag) {
                    log::Info("[AsteroidSpawner]: " + vessel->GetDisplayName() + " has been untracked for too long and is now lost.");
                    GE::OnDiscoverableObjectExpired.Fire(vessel);
                    vessel->Die();
                    flag = true;
                }
                num++;
            } else if (std::find(discoveredObjectIDs.begin(), discoveredObjectIDs.end(), vessel->persistentId) != discoveredObjectIDs.end()) {
                currentSize_++;
            }
        }
    }
    (void)num;
    return flag;
}

// ScenarioDiscoverableObjects.cs:250
void ScenarioDiscoverableObjects::SpawnAsteroid() {
    const int num = Rnd().Range(0, 2147483647);
    Rnd().InitState(num);
    setLastSeed(num);
    if (ReachedBody("Dres") && std::abs(num % 3) == 0)
        SpawnDresAsteroid(num);
    else
        SpawnHomeAsteroid(num);
}

// ScenarioDiscoverableObjects.cs:266
void ScenarioDiscoverableObjects::SpawnLastAsteroid() {
    Rnd().InitState(lastSeed());
    if (ReachedBody("Dres") && std::abs(lastSeed() % 3) == 0)
        SpawnDresAsteroid(lastSeed());
    else
        SpawnHomeAsteroid(lastSeed());
}

// ScenarioDiscoverableObjects.cs:279
void ScenarioDiscoverableObjects::SpawnHomeAsteroid(int asteroidSeed) {
    CelestialBody* homeBody = FlightGlobals::GetHomeBody();
    if (homeBody == nullptr) return;
    const double randomDuration = GetRandomDuration();
    const std::string text = DiscoverableObjectsUtil::GenerateAsteroidName();
    const UntrackedObjectClass sizeCurveBasedClass = GetSizeCurveBasedClass(0.0f, 1.0f);
    const double lifeTime = static_cast<double>(Rnd().Range(minUntrackedLifetime, maxUntrackedLifetime)) * 24.0 * 60.0 * 60.0;
    const double lifeTimeMax = static_cast<double>(maxUntrackedLifetime) * 24.0 * 60.0 * 60.0;
    const Orbit orbit = Orbit::CreateRandomOrbitFlyBy(*homeBody, randomDuration, UT());
    AddedVessel protoVessel = DiscoverableObjectsUtil::SpawnAsteroid(text, orbit, static_cast<uint32_t>(asteroidSeed), sizeCurveBasedClass, lifeTime, lifeTimeMax);
    discoveredObjectIDs.push_back(protoVessel.persistentId);
    untrackedObjectIDs.push_back(protoVessel.persistentId);
    log::Info("[AsteroidSpawner]: New object found near " + homeBody->name() + ": " + text + "!");
}

// ScenarioDiscoverableObjects.cs:296
UntrackedObjectClass ScenarioDiscoverableObjects::GetSizeCurveBasedClass(float minRange, float maxRange) {
    const int num = static_cast<int>(maxAsteroidClass) - static_cast<int>(minAsteroidClass);
    const float time = Mathf::Clamp01(Rnd().Range(minRange, maxRange));
    const float num2 = sizeCurve().Evaluate(time);
    return static_cast<UntrackedObjectClass>(static_cast<int>(minAsteroidClass) + Mathf::RoundToInt(f32store(num2 * static_cast<float>(num))));
}

// ScenarioDiscoverableObjects.cs:304
void ScenarioDiscoverableObjects::SpawnDresAsteroid(int asteroidSeed) {
    CelestialBody* bodyByName = FlightGlobals::GetBodyByName("Dres");
    if (bodyByName == nullptr) return;
    const std::string text = DiscoverableObjectsUtil::GenerateAsteroidName();
    const UntrackedObjectClass sizeCurveBasedClass = GetSizeCurveBasedClass(0.5f, 1.0f);
    const double lifeTime = static_cast<double>(Rnd().Range(minUntrackedLifetime, maxUntrackedLifetime)) * 24.0 * 60.0 * 60.0;
    const double lifeTimeMax = static_cast<double>(maxUntrackedLifetime) * 24.0 * 60.0 * 60.0;
    const double num = (bodyByName->sphereOfInfluence - bodyByName->Radius) / 2.0;
    Orbit orbit = Orbit::CreateRandomOrbitAround(bodyByName, bodyByName->Radius + num * 1.100000023841858, bodyByName->Radius + num * 1.25);
    orbit.meanAnomalyAtEpoch = static_cast<double>(f32store(Rnd().value() * 2.0f)) * 3.1415926535897931;
    AddedVessel protoVessel = DiscoverableObjectsUtil::SpawnAsteroid(text, orbit, static_cast<uint32_t>(asteroidSeed), sizeCurveBasedClass, lifeTime, lifeTimeMax);
    discoveredObjectIDs.push_back(protoVessel.persistentId);
    untrackedObjectIDs.push_back(protoVessel.persistentId);
    log::Info("[AsteroidSpawner]: New object found near Dres: " + text + "!");
}

// ScenarioDiscoverableObjects.cs:343
void ScenarioDiscoverableObjects::SpawnComet() {
    std::shared_ptr<CometOrbitType> cometType = CometManager::GenerateWeightedCometType();
    if (cometType == nullptr) throw NullReferenceException("cometType");
    SpawnComet(*cometType);
}

// ScenarioDiscoverableObjects.cs:343
void ScenarioDiscoverableObjects::SpawnComet(const std::string& typeName) {
    std::shared_ptr<CometOrbitType> cometOrbitType = CometManager::GetCometOrbitType(typeName);
    if (cometOrbitType == nullptr)
        log::Warning("[ScenarioDiscoverableObject] Unable to find Spawn Comet Type: " + typeName);
    else
        SpawnComet(*cometOrbitType);
}

// ScenarioDiscoverableObjects.cs:343
void ScenarioDiscoverableObjects::SpawnComet(CometOrbitType& cometType) {
    const int seed = Rnd().Range(0, 2147483647);
    Rnd().InitState(seed);
    setLastSeed(seed);
    const double lifeTime = static_cast<double>(Rnd().Range(minUntrackedLifetime, maxUntrackedLifetime)) * 24.0 * 60.0 * 60.0;
    const double lifeTimeMax = static_cast<double>(maxUntrackedLifetime) * 24.0 * 60.0 * 60.0;
    const UntrackedObjectClass randomObjClass = cometType.GetRandomObjClass();
    CometDefinition cometDef = CometManager::GenerateDefinition(cometType, randomObjClass, seed);
    Orbit o = cometType.CalculateHomeOrbit();
    DiscoverableObjectsUtil::SpawnComet("UnknownComet", o, cometDef, static_cast<uint32_t>(seed), randomObjClass, lifeTime, lifeTimeMax, false, 0.0f);
    log::Info("[CometSpawner]: New object found near " + FlightGlobals::GetHomeBodyName());
}

// ScenarioDiscoverableObjects.cs:373
double ScenarioDiscoverableObjects::GetRandomDuration() { return static_cast<double>(Rnd().Range(15.0f, 60.0f)); }

// ScenarioDiscoverableObjects.cs:378
bool ScenarioDiscoverableObjects::ReachedBody(const std::string& bodyName) {
    if (SpaceObjectsEnvironment::bodyReached) return SpaceObjectsEnvironment::bodyReached(bodyName);
    if (ProgressTracking::Instance() != nullptr) {
        KSPAchievements::CelestialBodySubtree* bodyTree = ProgressTracking::Instance()->GetBodyTree(bodyName);
        if (bodyTree != nullptr && bodyTree->IsReached()) return true;
    }
    return false;
}

void ScenarioDiscoverableObjects::Register() {
    ScenarioModuleFactory::Register("ScenarioDiscoverableObjects", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new ScenarioDiscoverableObjects()); });
}

// ================================================================================================================================ Sentinel seams

namespace SpaceObjects {
namespace SE = SentinelMission;
using SEnv = SentinelMission::SentinelEnvironment;

namespace {
// the seam's CometDefinition payload: the generated definition (the node builder of SpawnComet reads it back)
struct DefinitionPayload {
    CometDefinition def;
};
}  // namespace

void InstallSentinelSeams() {
    SEnv::discoverableObjectsProvider = []() -> std::optional<SEnv::DiscoverableObjects> {
        ScenarioDiscoverableObjects* s = ScenarioDiscoverableObjects::Instance;
        if (s == nullptr) return std::nullopt;
        SEnv::DiscoverableObjects d;
        d.minAsteroidClass = s->minAsteroidClass;
        d.maxAsteroidClass = s->maxAsteroidClass;
        d.minUntrackedLifetime = s->minUntrackedLifetime;
        d.maxUntrackedLifetime = s->maxUntrackedLifetime;
        d.untrackedObjectIDsCount = static_cast<int>(s->untrackedObjectIDs.size());
        return d;
    };
    SEnv::countSpaceObjects = [] { return FlightGlobals::CountSpaceObjects(); };
    SEnv::generateAsteroidName = [] { return DiscoverableObjectsUtil::GenerateAsteroidName(); };
    SEnv::spawnAsteroid = [](const std::string& name, Orbit& orbit, uint32_t seed, UntrackedObjectClass cls, double lifeTime, double lifeTimeMax) {
        DiscoverableObjectsUtil::SpawnAsteroid(name, orbit, seed, cls, lifeTime, lifeTimeMax);
    };
    SEnv::shouldSpawnComet = [] { return CometManager::ShouldSpawnComet(); };
    SEnv::generateWeightedCometType = []() -> std::shared_ptr<SE::ICometOrbitType> { return CometManager::GenerateWeightedCometType(); };
    SEnv::getCometOrbitType = [](const std::string& n) -> std::shared_ptr<SE::ICometOrbitType> { return CometManager::GetCometOrbitType(n); };
    SEnv::generateCometDefinition = [](SE::ICometOrbitType& t, UntrackedObjectClass cls, int seed) {
        SEnv::CometDefinitionSeam seam;
        auto payload = std::make_shared<DefinitionPayload>();
        payload->def = CometManager::GenerateDefinition(static_cast<CometOrbitType&>(t), cls, seed);
        seam.vfxStartDistance = payload->def.vfxStartDistance;
        seam.payload = payload;
        return seam;
    };
    SEnv::spawnComet = [](const std::string& name, Orbit& o, SEnv::CometDefinitionSeam& def, uint32_t seed, UntrackedObjectClass cls, double lifeTime, double lifeTimeMax,
                          bool optimizedCollider, float fragmentDynamicPressureModifier) {
        auto payload = std::static_pointer_cast<DefinitionPayload>(def.payload);
        if (payload == nullptr) throw NullReferenceException("cometDef");
        DiscoverableObjectsUtil::SpawnComet(name, o, payload->def, seed, cls, lifeTime, lifeTimeMax, optimizedCollider, fragmentDynamicPressureModifier);
    };
    FinePrint::Contracts::CometManagerSeam::discoveredComets = [] {
        return CometManager::Instance != nullptr ? CometManager::Instance->DiscoveredComets() : std::vector<Vessel*>();
    };
}

void UninstallSentinelSeams() {
    SEnv::discoverableObjectsProvider = nullptr;
    SEnv::countSpaceObjects = nullptr;
    SEnv::generateAsteroidName = nullptr;
    SEnv::spawnAsteroid = nullptr;
    SEnv::shouldSpawnComet = nullptr;
    SEnv::generateWeightedCometType = nullptr;
    SEnv::getCometOrbitType = nullptr;
    SEnv::generateCometDefinition = nullptr;
    SEnv::spawnComet = nullptr;
    FinePrint::Contracts::CometManagerSeam::discoveredComets = nullptr;
}
}  // namespace SpaceObjects

}  // namespace ksp
