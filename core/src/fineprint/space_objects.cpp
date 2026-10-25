// WP-38d: CometDefinition.cs, CometOrbitType.cs, CometManager.cs. See include/ksp/fineprint/space_objects.hpp.
#include <algorithm>
#include <cmath>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/core/coroutine_host.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/career/career_environment.hpp"
#include "ksp/database/game_database.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/fineprint/space_objects.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/vessel/vessel.hpp"
#include "../save/save_util.hpp"

namespace ksp {

namespace M = ksp::dotnet::Math;
using GE = events::GameEvents;
using SentinelMission::SentinelScenario;

namespace {
UnityRandom& Rnd() { return UnityRandom::Global(); }
Orbit& OrbitOfBody(CelestialBody* b) {
    Orbit* o = BodyOrbit(b);
    if (o == nullptr) throw NullReferenceException("body " + b->bodyName + " has no orbit");
    return *o;
}
constexpr float kPiF = static_cast<float>(3.1415926535897931);
// DiscoveryInfo.GetObjectClass(info.size.Value): KnowledgeItem.Value (KnowledgeItem.cs:20) is the enum's ToString (the class letter) when the vessel's knowledge covers
// Presence and the localized fallback text otherwise, which GetObjectClass maps to class A
UntrackedObjectClass SizeClassOf(const DiscoveryInfo& info) {
    if (info.HaveKnowledgeAbout(DiscoveryLevels::Presence)) return DiscoveryInfo::GetObjectClass(dotnet::EnumToString(info.objectSize()));
    return UntrackedObjectClass::A;
}
}  // namespace

// ================================================================================================================================ CometDefinition

// CometDefinition.cs:25
ConfigNode::NodePtr CometDefinition::CreateVesselNode(bool optimizedCollider, float fragmentDynamicPressureModifier, bool hasName) const {
    auto configNode = ConfigNode::Create("CometVessel");
    configNode->AddValue("typeName", typeName);
    configNode->AddValue("vfxStartDistance", vfxStartDistance);
    configNode->AddValue("comaWidthRatio", comaWidthRatio);
    configNode->AddValue("tailWidthRatio", tailWidthRatio);
    configNode->AddValue("ionTailWidthRatio", ionTailWidthRatio);
    configNode->AddValue("tailMaxLengthRatio", tailMaxLengthRatio);
    configNode->AddValue("seed", static_cast<int32_t>(seed));
    configNode->AddValue("radius", radius);
    configNode->AddValue("numGeysers", static_cast<int32_t>(numGeysers));
    configNode->AddValue("numNearDustEmitters", static_cast<int32_t>(numNearDustEmitters));
    configNode->AddValue("optimizedCollider", optimizedCollider);
    configNode->AddValue("fragmentDynamicPressureModifier", fragmentDynamicPressureModifier);
    if (hasName) configNode->AddValue("hasName", true);
    return configNode;
}

// ================================================================================================================================ CometOrbitType

// CometOrbitType.cs:71
CometOrbitType::CometOrbitType(std::string name_, float inclination1Min_, float inclination1Max_, float inclination2Min_, float inclination2Max_,
                               float inclination1Chance_, float PEMin, float PEMax, float APMin, float APMax, bool UseEcc, float EccMin, float EccMax,
                               UntrackedObjectClass minCometClass_, UntrackedObjectClass maxCometClass_) {
    name = std::move(name_);
    inclination1Min = inclination1Min_;
    inclination1Max = inclination1Max_;
    inclination2Min = inclination2Min_;
    inclination2Max = inclination2Max_;
    inclination1Chance = inclination1Chance_;
    peMin = PEMin;
    peMax = PEMax;
    apMin = APMin;
    apMax = APMax;
    useEccentricity = UseEcc;
    eccentricityMin = EccMin;
    eccentricityMax = EccMax;
    minCometClass = minCometClass_;
    maxCometClass = maxCometClass_;
}

// CometOrbitType.cs:91
Orbit CometOrbitType::CalculateHomeOrbit() {
    bool validOrbit = false;
    return CalculateOrbit(true, nullptr, std::numeric_limits<double>::max(), validOrbit);
}

// CometOrbitType.cs:97
Orbit CometOrbitType::CalculateSentinelOrbit(const Orbit& sentinelOrbit, double minDiscoveryDistance, bool& validOrbit) {
    return CalculateOrbit(false, &sentinelOrbit, minDiscoveryDistance, validOrbit);
}

// CometOrbitType.cs:102
Orbit CometOrbitType::CalculateOrbit(bool homeOrbit, const Orbit* sentinelOrbit, double minDiscoveryDistance, bool& validOrbit) {
    validOrbit = true;
    Orbit orbit;
    CelestialBody* sun = Planetarium::fetch->Sun;
    double num5 = 0.0;
    double num6 = 0.0;
    if (!useEccentricity) {
        const double num = static_cast<double>(Rnd().Range(peMin, peMax));
        const double num2 = static_cast<double>(Rnd().Range(apMin, apMax));
        const double num3 = num * OrbitOfBody(FlightGlobals::GetCometPerturber()).semiMajorAxis;
        const double num4 = num2 * OrbitOfBody(FlightGlobals::GetCometPerturber()).semiMajorAxis;
        num5 = (num3 + num4) / 2.0;
        num6 = 1.0 - num3 / num5;
    } else {
        const double num3 = static_cast<double>(Rnd().Range(peMin, peMax)) * OrbitOfBody(FlightGlobals::GetCometPerturber()).semiMajorAxis;
        num6 = static_cast<double>(Rnd().Range(eccentricityMin, eccentricityMax));
        const double num4 = num3 / (1.0 - num6);
        num5 = (num3 + num4) / 2.0;
    }
    // `(float)Random.Range(0, 1) < inclination1Chance` (the int overload: [0, 1) is always 0)
    const double num7 = (!(static_cast<float>(Rnd().Range(0, 1)) < inclination1Chance)) ? static_cast<double>(Rnd().Range(inclination2Min, inclination2Max))
                                                                                       : static_cast<double>(Rnd().Range(inclination1Min, inclination1Max));
    const double lan = static_cast<double>(Rnd().Range(0.0f, 360.0f));
    const double argPe = 0.0;
    const double universalTime = Planetarium::GetUniversalTime();
    orbit = Orbit(num7, num6, num5, lan, argPe, 0.0, universalTime, sun);
    if (homeOrbit) {
        double val = CometManager::GenerateHomeSpawnDistance();
        val = M::Min(val, minDiscoveryDistance);
        trueAnom = orbit.TrueAnomalyAtRadiusSimple(val);
        eccAnom = orbit.GetEccentricAnomaly(trueAnom);
        meanAnomalyAtEpoch = 0.0 - orbit.GetMeanAnomaly(eccAnom);
        if (std::isnan(meanAnomalyAtEpoch)) {
            log::Info("[CometOrbitType] Unable to solve HomeSpawnDistance using default value");
            meanAnomalyAtEpoch = -3.1415926535897931 / 2.0;
        }
        orbit.SetOrbit(num7, num6, num5, lan, argPe, meanAnomalyAtEpoch, universalTime, sun);
    } else {
        double val2 = CometManager::GenerateSentinelSpawnDistance(sentinelOrbit->radius);
        val2 = M::Max(val2, minDiscoveryDistance);
        if (val2 > orbit.ApR() && orbit.eccentricity < 1.0) {
            validOrbit = false;
        } else {
            if (val2 < orbit.PeR()) {
                log::Info("[CometOrbitType] Spawn distance inside pe of Comet Orbit - picking spot between 45 and 90 degs on inbound");
                eccAnom = static_cast<double>(Rnd().Range(kPiF / 4.0f, kPiF / 2.0f));
                trueAnom = orbit.GetTrueAnomaly(eccAnom);
            } else {
                trueAnom = orbit.TrueAnomalyAtRadiusSimple(val2);
                eccAnom = orbit.GetEccentricAnomaly(trueAnom);
            }
            meanAnomalyAtEpoch = 0.0 - orbit.GetMeanAnomaly(eccAnom);
            if (num7 > 90.0 || num7 < -90.0) trueAnom *= -1.0;
            if (std::isnan(meanAnomalyAtEpoch)) {
                log::Info("[CometOrbitType] Unable to solve SentinelSpawnDistance using default value - setting to half way point");
                meanAnomalyAtEpoch = 0.0 - orbit.GetMeanAnomaly(-3.1415926535897931 / 2.0);
            }
            double num8 = (sentinelOrbit->LAN + sentinelOrbit->argumentOfPeriapsis) * (3.1415926535897931 / 180.0) + sentinelOrbit->trueAnomaly + trueAnom;
            const float num9 = f32store(static_cast<float>(sentinelOrbit->radius / val2 * M::Sin(static_cast<double>(f32store(180.0f - CometManager::SentinelDiscoveryArc())) *
                                                                                               3.1415926535897931 / 180.0)));
            num8 += static_cast<double>(Rnd().Range(f32store(0.0f - num9), num9));
            num8 *= 180.0 / 3.1415926535897931;
            orbit.SetOrbit(num7, num6, num5, num8, argPe, meanAnomalyAtEpoch, universalTime, sun);
        }
    }
    return orbit;
}

// CometOrbitType.cs:185
UntrackedObjectClass CometOrbitType::GetRandomObjClass() {
    return static_cast<UntrackedObjectClass>(Rnd().Range(static_cast<int>(minCometClass), static_cast<int>(maxCometClass) + 1));
}

// CometOrbitType.cs:190
void CometOrbitType::Load(const ConfigNode& node) {
    node.TryGetValue("name", name);
    node.TryGetValue("displayName", displayName);
    node.TryGetValue("description", description);
    node.TryGetValue("inclination1Min", inclination1Min);
    node.TryGetValue("inclination1Max", inclination1Max);
    node.TryGetValue("inclination2Min", inclination2Min);
    node.TryGetValue("inclination2Max", inclination2Max);
    node.TryGetValue("inclination1Chance", inclination1Chance);
    node.TryGetValue("peMin", peMin);
    node.TryGetValue("peMax", peMax);
    node.TryGetValue("useEccentricity", useEccentricity);
    node.TryGetValue("eccentricityMin", eccentricityMin);
    node.TryGetValue("eccentricityMax", eccentricityMax);
    node.TryGetValue("apMin", apMin);
    node.TryGetValue("apMax", apMax);
    node.TryGetValue("chanceWeight", chanceWeight);
    save_detail::TryGetEnum(node, "minCometClass", minCometClass, UntrackedObjectClass::F);
    save_detail::TryGetEnum(node, "maxCometClass", maxCometClass, UntrackedObjectClass::I);
}

// ================================================================================================================================ CometManager

// CometManager.cs:17
void CometManager::CometTailColor::Load(const ConfigNode& node) {
    node.TryGetValue("emitterColor", emitterColor);
    node.TryGetValue("chanceWeight", chanceWeight);
}

// CometManager.cs:23
void CometManager::CometTailColor::Save(ConfigNode& node) const {
    node.AddValue("emitterColor", emitterColor);
    node.AddValue("chanceWeight", static_cast<int32_t>(chanceWeight));
}

CometManager::CometManager() = default;

CometManager::~CometManager() {
    if (subscribed_) OnDestroy();
    if (Instance == this) Instance = nullptr;
}

// CometManager.cs:254
std::vector<Vessel*> CometManager::DiscoveredComets() const {
    std::vector<Vessel*> list;
    for (size_t i = 0; i < comets_.size(); ++i)
        if (comets_[i]->GetDiscoveryInfo().HaveKnowledgeAbout(DiscoveryLevels::StateVectors)) list.push_back(comets_[i]);
    return list;
}

// CometManager.cs:270
float CometManager::SentinelDiscoveryArc() {
    if (Instance == nullptr) throw NullReferenceException("CometManager.Instance");
    return Instance->sentinelAngle;
}

// CometManager.cs:272
double CometManager::VFXUpdateFrequency() {
    if (Instance != nullptr) return Instance->vFXUpdateFrequency;
    return 10.0;
}

// CometManager.cs:284
double CometManager::VFXUpdateFrequencyInAtmosphere() {
    if (Instance != nullptr) return Instance->vFXUpdateFrequencyInAtmosphere;
    return 0.029999999329447746;
}

// CometManager.cs:296
void CometManager::Awake() {
    if (Instance != nullptr) {
        // UnityEngine.Object.DestroyImmediate(base.gameObject); Instance = null
        destroyed_ = true;
        Instance = nullptr;
        return;
    }
    Instance = this;
    generator = std::make_unique<KSPRandom>(Contracts::ContractsEnvironment::Current().NewRandomSeed());
    cometDefinitions.clear();
    comets_.clear();
    GE::OnGameDatabaseLoaded.Add(this, &CometManager::OnGameDatabaseLoaded);
    GE::onFlightGlobalsAddVessel.Add(this, &CometManager::OnVesselAdded);
    GE::onFlightGlobalsRemoveVessel.Add(this, &CometManager::OnVesselRemoved);
    subscribed_ = true;
}

// CometManager.cs:313
void CometManager::Start() {
    LoadManagerDefinitions();
    LoadCometTypes();
    GE::OnGameDatabaseLoaded.Add(this, &CometManager::OnGameDatabaseLoaded);
}

// CometManager.cs:320 (the C# only removes OnGameDatabaseLoaded; the two vessel handlers are never unsubscribed, which would leave a dangling delegate here)
void CometManager::OnDestroy() {
    if (Instance == this) Instance = nullptr;  // a destroyed Unity object compares equal to null
    GE::OnGameDatabaseLoaded.Remove(this, &CometManager::OnGameDatabaseLoaded);
    GE::OnGameDatabaseLoaded.Remove(this, &CometManager::OnGameDatabaseLoaded);
    GE::onFlightGlobalsAddVessel.Remove(this, &CometManager::OnVesselAdded);
    GE::onFlightGlobalsRemoveVessel.Remove(this, &CometManager::OnVesselRemoved);
    subscribed_ = false;
}

// CometManager.cs:325
void CometManager::OnGameDatabaseLoaded() {
    LoadManagerDefinitions();
    LoadCometTypes();
}

// CometManager.cs:331
void CometManager::OnVesselAdded(Vessel* v) {
    if (v->vesselType == VesselType::SpaceObject && Contracts::Lite::VesselHasPartName("PotatoComet", v))
        if (std::find(comets_.begin(), comets_.end(), v) == comets_.end()) comets_.push_back(v);  // AddUnique
}

// CometManager.cs:339
void CometManager::OnVesselRemoved(Vessel* v) {
    int count = static_cast<int>(comets_.size());
    do {
        if (count-- <= 0) return;
    } while (comets_[static_cast<size_t>(count)]->persistentId != v->persistentId);
    comets_.erase(comets_.begin() + count);
}

// CometManager.cs:353
float CometManager::GetSpawnChance() {
    int num = 0;
    if (ScenarioDiscoverableObjects::Instance != nullptr) num += ScenarioDiscoverableObjects::Instance->spawnGroupMaxLimit;
    if (SentinelScenario::Instance != nullptr) num += SentinelScenario::Instance->MaxSentinelUntrackedSpaceObjects();
    cometCap = maxCometsPerUntrackedObjects.Evaluate(static_cast<float>(num));
    currentComets = static_cast<float>(comets_.size());
    cometChance = spawnChance.Evaluate(f32store(currentComets / cometCap));
    return cometChance;
}

// CometManager.cs:370
bool CometManager::ShouldSpawnComet() {
    if (Instance == nullptr) return false;
    return Rnd().Range(0.0f, 1.0f) < Instance->GetSpawnChance();
}

// CometManager.cs:379
std::shared_ptr<CometOrbitType> CometManager::GenerateWeightedCometType() {
    if (Instance == nullptr) return nullptr;
    int num = 0;
    int count = static_cast<int>(Instance->cometDefinitions.size());
    while (count-- > 0) num += Instance->cometDefinitions[static_cast<size_t>(count)]->chanceWeight;
    int num2 = Rnd().Range(0, num);
    int count2 = static_cast<int>(Instance->cometDefinitions.size());
    while (count2-- > 0) {
        const int chanceWeight = Instance->cometDefinitions[static_cast<size_t>(count2)]->chanceWeight;
        if (num2 >= chanceWeight) {
            num2 -= chanceWeight;
            continue;
        }
        num2 = count2;
        break;
    }
    log::Info("[Comet Manager]:Generated Comet Type:" + Instance->cometDefinitions.at(static_cast<size_t>(num2))->name);
    return Instance->cometDefinitions[static_cast<size_t>(num2)];
}

// CometManager.cs:408
double CometManager::GenerateHomeSpawnDistance() {
    CelestialBody* homeBody = FlightGlobals::GetHomeBody();
    if (Instance == nullptr) {
        if (FlightGlobals::ready && homeBody != nullptr) return OrbitOfBody(homeBody).semiMajorAxis * static_cast<double>(Rnd().Range(minSpawnDefault, maxSpawnDefault));
        return spawnSMADefault * static_cast<double>(Rnd().Range(minSpawnDefault, maxSpawnDefault));
    }
    return OrbitOfBody(homeBody).semiMajorAxis * static_cast<double>(Rnd().Range(Instance->minSpawnDistance, Instance->maxSpawnDistance));
}

// CometManager.cs:422
double CometManager::GenerateSentinelSpawnDistance(double distance) {
    if (Instance == nullptr) return distance * static_cast<double>(Rnd().Range(minSentinelSpawnDefault, maxSentinelSpawnDefault));
    return distance * static_cast<double>(Rnd().Range(Instance->minSentinelSpawnDistance, Instance->maxSentinelSpawnDistance));
}

// CometManager.cs:431
std::shared_ptr<CometOrbitType> CometManager::GetCometOrbitType(const std::string& name) {
    if (Instance == nullptr) return nullptr;
    for (size_t num = 0; num < Instance->cometDefinitions.size(); ++num)
        if (Instance->cometDefinitions[num]->name == name) return Instance->cometDefinitions[num];
    return nullptr;
}

// GameDatabase.Instance.GetConfigNodes(type)
std::vector<ConfigNode*> CometManager::GetConfigNodes(const std::string& type) {
    if (SpaceObjectsEnvironment::gameDatabaseConfigNodes) return SpaceObjectsEnvironment::gameDatabaseConfigNodes(type);
    if (GameDatabase::Instance() == nullptr) throw NullReferenceException("GameDatabase.Instance");
    return GameDatabase::Instance()->GetConfigNodes(type);
}

// CometManager.cs:454
void CometManager::LoadManagerDefinitions() {
    std::vector<ConfigNode*> configNodes = GetConfigNodes("COMET_MANAGER");
    managerDef_ = nullptr;
    if (!configNodes.empty()) {
        managerDef_ = configNodes[0];
        if (configNodes.size() > 1) log::Warning("[CometManager]: Found more than one COMET_MANAGER node using the first one");
    }
    if (managerDef_ == nullptr) {
        log::Warning("[CometManager]: No COMET_MANAGER node found. Using default values");
        return;
    }
    ConfigNode& m = *managerDef_;
    // `managerDef.TryGetNode(name, ref node) && node.values.Count > 0` then `curve = new FloatCurve(); curve.Load(node)`
    auto loadCurve = [&m](const char* name, FloatCurve& curve) {
        ConfigNode* node = nullptr;
        if (m.TryGetNode(name, node) && node->values().Count() > 0) {
            curve = FloatCurve();
            curve.Load(*node);
        }
    };
    m.TryGetValue("maxSpawnDistance", maxSpawnDistance);
    m.TryGetValue("minSpawnDistance", minSpawnDistance);
    loadCurve("maxCometsPerUntrackedObjects", maxCometsPerUntrackedObjects);
    loadCurve("spawnChance", spawnChance);
    m.TryGetValue("maxVFXStartDistance", maxVFXStartDistance);
    m.TryGetValue("minVFXStartDistance", minVFXStartDistance);
    loadCurve("vfxStartDistribution", vfxStartDistribution);
    m.TryGetValue("maxComaSizeRatio", maxComaSizeRatio);
    m.TryGetValue("minComaSizeRatio", minComaSizeRatio);
    loadCurve("comaSizeRatioDistribution", comaSizeRatioDistribution);
    m.TryGetValue("maxTailWidthRatio", maxTailWidthRatio);
    m.TryGetValue("maxIonTailWidthRatio", maxIonTailWidthRatio);
    loadCurve("tailWidthDistribution", tailWidthDistribution);
    m.TryGetValue("maxTailLengthRatio", maxTailLengthRatio);
    m.TryGetValue("minTailLengthRatio", minTailLengthRatio);
    loadCurve("tailLengthRatioDistribution", tailLengthRatioDistribution);
    loadCurve("vfxSizeFromDistance", vfxSizeFromDistance);
    loadCurve("nearDustVFXSizeFromDistance", nearDustVFXSizeFromDistance);
    loadCurve("geyserVFXSizeFromDistance", geyserVFXSizeFromDistance);
    m.TryGetValue("minGeyserRadiusRatio", minGeyserRadiusRatio);
    m.TryGetValue("maxGeyserRadiusRatio", maxGeyserRadiusRatio);
    loadCurve("geysersRatioDistribution", geysersRatioDistribution);
    m.TryGetValue("geyserVisibleDistance", geyserVisibleDistance);
    m.TryGetValue("geyserSizeRadiusMultiplier", geyserSizeRadiusMultiplier);
    m.TryGetValue("nearDustVisibleRadiusMultiplier", nearDustVisibleRadiusMultiplier);
    m.TryGetValue("minNearDustEmitterRadiusRatio", minNearDustEmitterRadiusRatio);
    m.TryGetValue("maxNearDustEmitterRadiusRatio", maxNearDustEmitterRadiusRatio);
    m.TryGetValue("atmospherePressureForVFXFade", atmospherePressureForVFXFade);
    loadCurve("atmospherePressureFadeDistributionForVFXFade", atmospherePressureFadeDistributionForVFXFade);
    m.TryGetValue("gravityAdditionForVFXFadeStart", gravityAdditionForVFXFadeStart);
    m.TryGetValue("gravityMultiplierForVFXFadeEnd", gravityMultiplierForVFXFadeEnd);
    m.TryGetValue("gravitySubtractionForVFXFadeEnd", gravitySubtractionForVFXFadeEnd);
    m.TryGetValue("vesselRangeOrbitLoadOverride", vesselRangeOrbitLoadOverride);
    m.TryGetValue("vesselRangeOrbitUnloadOverride", vesselRangeOrbitUnloadOverride);
    m.TryGetValue("fragmentdynamicPressurekPa", fragmentdynamicPressurekPa);
    m.TryGetValue("minFragments", minFragments);
    m.TryGetValue("maxFragments", maxFragments);
    m.TryGetValue("minFragmentClassStep", minFragmentClassStep);
    m.TryGetValue("maxFragmentClassStep", maxFragmentClassStep);
    m.TryGetValue("fragmentInstanceOffset", fragmentInstanceOffset);
    m.TryGetValue("fragmentAngleVariation", fragmentAngleVariation);
    m.TryGetValue("fragmentIgnoreCollisions", fragmentIgnoreCollisions);
    m.TryGetValue("fragmentMinPressureModifier", fragmentMinPressureModifier);
    m.TryGetValue("fragmentMaxPressureModifier", fragmentMaxPressureModifier);
    m.TryGetValue("fragmentMinSafeTime", fragmentMinSafeTime);
    m.TryGetValue("fragmentMaxSafeTime", fragmentMaxSafeTime);
    LoadTailColorLists();
}

// CometManager.cs:565
void CometManager::LoadCometTypes() {
    cometDefinitions.clear();
    std::vector<ConfigNode*> cometDefs = GetConfigNodes("COMET_ORBIT_TYPE");
    for (size_t i = 0; i < cometDefs.size(); ++i) {
        auto cometOrbitType = std::make_shared<CometOrbitType>();
        cometOrbitType->Load(*cometDefs[i]);
        cometDefinitions.push_back(cometOrbitType);
    }
}

// CometManager.cs:577 (the C# builds `Instance.generator = new KSPRandom(seed)` BEFORE it tests Instance for null: a missing instance is a NullReferenceException)
CometDefinition CometManager::GenerateDefinition(CometOrbitType& cometType, UntrackedObjectClass objClass, int seed) {
    CometDefinition cometDefinition;
    cometDefinition.typeName = cometType.name;
    cometDefinition.cometType = &cometType;
    cometDefinition.seed = seed;
    cometDefinition.radius = DiscoveryInfo::GetClassRadius(objClass, seed);
    if (Instance == nullptr) throw NullReferenceException("CometManager.Instance");
    Instance->generator = std::make_unique<KSPRandom>(seed);
    CometManager& I = *Instance;
    KSPRandom& g = *I.generator;
    cometDefinition.vfxStartDistance = I.minVFXStartDistance + static_cast<double>(I.vfxStartDistribution.Evaluate(static_cast<float>(g.NextDouble()))) * (I.maxVFXStartDistance - I.minVFXStartDistance);
    cometDefinition.comaWidthRatio = I.minComaSizeRatio + static_cast<double>(I.comaSizeRatioDistribution.Evaluate(static_cast<float>(g.NextDouble()))) * (I.maxComaSizeRatio - I.minComaSizeRatio);
    cometDefinition.tailWidthRatio = static_cast<double>(I.tailWidthDistribution.Evaluate(static_cast<float>(g.NextDouble()))) * I.maxTailWidthRatio;
    cometDefinition.tailMaxLengthRatio = I.minTailLengthRatio + static_cast<double>(I.tailLengthRatioDistribution.Evaluate(static_cast<float>(g.NextDouble()))) * (I.maxTailLengthRatio - I.minTailLengthRatio);
    cometDefinition.ionTailWidthRatio = static_cast<double>(I.tailWidthDistribution.Evaluate(static_cast<float>(g.NextDouble()))) * I.maxIonTailWidthRatio;
    const double num = I.minGeyserRadiusRatio + static_cast<double>(I.geysersRatioDistribution.Evaluate(static_cast<float>(g.NextDouble()))) * (I.maxGeyserRadiusRatio - I.minGeyserRadiusRatio);
    cometDefinition.numGeysers = dotnet::ConvToInt32(num * static_cast<double>(cometDefinition.radius));
    cometDefinition.numGeysers = Mathf::Min(cometDefinition.numGeysers, GameSettings::COMET_MAXIMUM_GEYSERS);
    const double num2 = I.minNearDustEmitterRadiusRatio + g.NextDouble() * (I.maxNearDustEmitterRadiusRatio - I.minNearDustEmitterRadiusRatio);
    cometDefinition.numNearDustEmitters = dotnet::ConvToInt32(num2 * static_cast<double>(cometDefinition.radius));
    // sic: the C# clamps numGeysers a second time (with the dust emitter limit) and never clamps numNearDustEmitters
    cometDefinition.numGeysers = Mathf::Min(cometDefinition.numGeysers, GameSettings::COMET_MAXIMUM_NEAR_DUST_EMITTERS);
    return cometDefinition;
}

// CometManager.cs:604
void CometManager::SpawnCometFragments(const CometVesselSeam& cometVessel) {
    if (cometVessel.Vessel_ == nullptr) return;
    const std::string displayName = cometVessel.Vessel_->GetDisplayName();
    const int num = Rnd().Range(minFragments, maxFragments + 1);
    AddedVessel first;
    for (int i = 0; i < num; ++i) {
        const std::string text = "-" + std::string(1, static_cast<char>(65 + i));
        AddedVessel added = SpawnCometFragment(displayName + text, *cometVessel.Vessel_, *cometVessel.CometType, SizeClassOf(cometVessel.Vessel_->GetDiscoveryInfo()),
                                               cometVessel.radius, i == 0);
        if (i == 0) first = added;
    }
    Vessel* objectToFollow = first.vessel;  // protoVessel.vesselRef
    if (explodeWithDebris)
        explodeWithDebris(*cometVessel.Vessel_,
                          f32store(static_cast<float>(static_cast<int>(SizeClassOf(cometVessel.Vessel_->GetDiscoveryInfo()))) * explosionScaleMultiplier),
                          objectToFollow);
}

// CometManager.cs:630
AddedVessel CometManager::SpawnCometFragment(const std::string& cometName, Vessel& originalComet, CometOrbitType& cometOrbitType,
                                              UntrackedObjectClass explodeCometClass, float cometRadius, bool focusAfterSpawned) {
    UntrackedObjectClass untrackedObjectClass = UntrackedObjectClass::A;
    int num = static_cast<int>(explodeCometClass);
    num -= Rnd().Range(minFragmentClassStep, maxFragmentClassStep + 1);
    // Enum.IsDefined(typeof(UntrackedObjectClass), num)
    if (num >= 0 && num <= static_cast<int>(UntrackedObjectClass::I)) {
        untrackedObjectClass = static_cast<UntrackedObjectClass>(num);
        std::optional<float> component = cometFragmentDynamicPressureModifier ? cometFragmentDynamicPressureModifier(originalComet) : std::nullopt;
        float fragmentDynamicPressureModifier = 0.0f;
        if (component.has_value()) fragmentDynamicPressureModifier = f32store(*component + Rnd().Range(fragmentMaxPressureModifier, fragmentMinPressureModifier));
        const Orbit& oo = originalComet.orbit();
        Orbit orbit(oo.inclination, oo.eccentricity, oo.semiMajorAxis, oo.LAN, oo.argumentOfPeriapsis, oo.meanAnomalyAtEpoch, oo.epoch, oo.referenceBody);
        Vector3d pos = oo.pos;
        const float num2 = f32store(cometRadius * fragmentInstanceOffset);
        pos.x += static_cast<double>(Rnd().Range(f32store(0.0f - num2), num2));
        pos.y += static_cast<double>(Rnd().Range(f32store(0.0f - num2), num2));
        pos.z += static_cast<double>(Rnd().Range(f32store(0.0f - num2), num2));
        // Quaternion.AngleAxis(Range(-v, v), (Vector3)new Vector3d(srf_vel_direction.x, 0, 0)): the Vector3d converts to the float Vector3
        const Quaternionf quaternion = UnityNative::AngleAxis(Rnd().Range(f32store(0.0f - fragmentAngleVariation), fragmentAngleVariation),
                                                              Vector3d(originalComet.srf_vel_direction.x, 0.0, 0.0).ToVector3f());
        const Quaternionf quaternion2 = UnityNative::AngleAxis(Rnd().Range(f32store(0.0f - fragmentAngleVariation), fragmentAngleVariation),
                                                               Vector3d(0.0, originalComet.srf_vel_direction.y, 0.0).ToVector3f());
        const Vector3f rotatedVel = (quaternion2 * quaternion) * oo.vel.ToVector3f();
        orbit.UpdateFromStateVectors(pos, Vector3d(rotatedVel.x, rotatedVel.y, rotatedVel.z), oo.referenceBody, Planetarium::GetUniversalTime());
        orbit.Init();
        orbit.UpdateFromUT(Planetarium::GetUniversalTime());
        const int seed = Rnd().Range(0, 2147483647);
        AddedVessel added = DiscoverableObjectsUtil::SpawnComet(cometName, orbit, GenerateDefinition(cometOrbitType, untrackedObjectClass, seed),
                                                                static_cast<uint32_t>(seed), untrackedObjectClass, originalComet.GetDiscoveryInfo().lastObservedTime(),
                                                                originalComet.GetDiscoveryInfo().referenceLifetime(), true, fragmentDynamicPressureModifier);
        if (added.vessel == nullptr) throw NullReferenceException("protoVessel.vesselRef");
        added.vessel->GetDiscoveryInfo().SetLevel(originalComet.GetDiscoveryInfo().Level());
        added.vessel->SetIgnoreCollisionsFrames(fragmentIgnoreCollisions);
        added.vessel->externalTemperature = originalComet.externalTemperature;
        added.vessel->atmosphericTemperature = originalComet.atmosphericTemperature;
        added.vessel->vesselName = cometName;
        if (focusAfterSpawned) {
            Vessel* cometFragmentTofocus = added.vessel;
            if (FlightGlobals::ActiveVessel != nullptr && FlightGlobals::ActiveVessel->persistentId == originalComet.persistentId &&
                cometFragmentTofocus->GetDiscoveryInfo().Level() == DiscoveryLevels::Owned) {
                CareerEnvironment::Current().Scheduler().StartCoroutine(CallbackUtil::DelayedCallback(1, Delegate<void()>::Lambda(this, [cometFragmentTofocus] {
                    if (FlightGlobals::ActiveVessel != nullptr && cometFragmentTofocus != nullptr) FlightGlobals::SetActiveVessel(cometFragmentTofocus, false);
                })));
            }
        }
        return added;
    }
    return AddedVessel();
}

// CometManager.cs:737
void CometManager::LoadTailColorLists() {
    managerDef_->TryGetValue("ionTailColorP67", ionTailColorP67);
    if (managerDef_->HasNode("ionTailColors")) {
        ionTailColors = LoadColorsList(*managerDef_->GetNode("ionTailColors"));
    } else {
        ionTailColors.clear();
        CometTailColor c;
        c.emitterColor = Color(0.04313725f, 0.2906635f, 13.0f / 15.0f, 23.0f / 51.0f);
        c.chanceWeight = 5;
        ionTailColors.push_back(c);
    }
    if (managerDef_->HasNode("dustTailColors")) {
        dustTailColors = LoadColorsList(*managerDef_->GetNode("dustTailColors"));
        return;
    }
    dustTailColors.clear();
    CometTailColor c;
    c.emitterColor = Color(105.0f / 106.0f, 0.9433554f, 0.7429245f, 0.03137255f);
    c.chanceWeight = 5;
    dustTailColors.push_back(c);
}

// CometManager.cs:766
std::vector<CometManager::CometTailColor> CometManager::LoadColorsList(const ConfigNode& colorsNode) {
    std::vector<CometTailColor> list;
    std::vector<ConfigNode*> nodes = colorsNode.GetNodes("weightedColor");
    for (size_t i = 0; i < nodes.size(); ++i) {
        CometTailColor cometTailColor;
        cometTailColor.Load(*nodes[i]);
        list.push_back(cometTailColor);
    }
    if (list.size() < 1) list.push_back(CometTailColor());
    return list;
}

namespace {
// the weighted pick both GenerateWeighted*TailColor share: `new KSPRandom(seed).Next(0, total)` then the reverse walk over the weights
size_t WeightedColorIndex(const std::vector<CometManager::CometTailColor>& colors, int seed) {
    int num = 0;
    int count = static_cast<int>(colors.size());
    while (count-- > 0) num += colors[static_cast<size_t>(count)].chanceWeight;
    int num2 = KSPRandom(seed).Next(0, num);
    int count2 = static_cast<int>(colors.size());
    while (count2-- > 0) {
        const int chanceWeight = colors[static_cast<size_t>(count2)].chanceWeight;
        if (num2 >= chanceWeight) {
            num2 -= chanceWeight;
            continue;
        }
        num2 = count2;
        break;
    }
    return static_cast<size_t>(num2);
}
}  // namespace

// CometManager.cs:784 (a null comet with a populated list is the NullReferenceException of `comet.seed`)
Color CometManager::GenerateWeightedIonTailColor(std::optional<int> cometSeed) {
    if (!(Instance == nullptr) && !Instance->ionTailColors.empty()) {
        if (cometSeed.has_value() && *cometSeed == 1633779195) return Instance->ionTailColorP67;
        if (!cometSeed.has_value()) throw NullReferenceException("comet");
        const size_t num2 = WeightedColorIndex(Instance->ionTailColors, *cometSeed);
        log::Info("[CometManager]: Generated Ion Tail Color: #" + std::to_string(num2));
        return Instance->ionTailColors.at(num2).emitterColor;
    }
    return Color(0.04313725f, 0.2906635f, 13.0f / 15.0f, 23.0f / 51.0f);
}

// CometManager.cs:817
Color CometManager::GenerateWeightedDustTailColor(int cometSeed) {
    if (Instance == nullptr) return Color(105.0f / 106.0f, 0.9433554f, 0.7429245f, 0.03137255f);
    const size_t num2 = WeightedColorIndex(Instance->dustTailColors, cometSeed);
    log::Info("[CometManager]: Generated Dust Tail Color: #" + std::to_string(num2));
    return Instance->dustTailColors.at(num2).emitterColor;
}

}  // namespace ksp
