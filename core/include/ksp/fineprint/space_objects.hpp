// WP-38d: the comet / asteroid package that WP-38c left as seams (SentinelEnvironment, CometManagerSeam):
//   Assembly-CSharp/CometDefinition.cs, CometOrbitType.cs, CometManager.cs, DiscoverableObjectsUtil.cs, ScenarioDiscoverableObjects.cs
// (listed under WP-09 "space objects"; the data side and the spawn logic are engine-agnostic and live here, in the folder of their first consumer,
// the Sentinel scenario). Not ported (scene side, see STATUS WP-38d): the VFX / collider setters of CometManager (they call into CometVessel components),
// CometVessel / ModuleComet themselves, FXMonger.ExplodeWithDebris.
//
// Unity statics: UnityEngine.Random is UnityRandom::Global() (WP-02 C2 capture, bit exact); Debug.Log is ksp::log.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/curves/float_curve.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/fineprint/sentinel.hpp"
#include "ksp/save/game.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/vessel/discovery_info.hpp"

namespace ksp {

class CelestialBody;
class Orbit;
class ProtoVessel;
class Vessel;

// HighLogic.CurrentGame.AddVessel(node) (Game.cs:695): the proto vessel and the vessel it created (null when the load failed)
struct AddedVessel {
    ProtoVessel* protoVessel = nullptr;
    Vessel* vessel = nullptr;
    uint32_t persistentId = 0;  // protoVessel.persistentId
};

// ================================================================================================================================ CometDefinition

// CometDefinition.cs:1
class CometOrbitType;
struct CometDefinition {
    std::string typeName;                 // :3
    double vfxStartDistance = 0.0;        // :5
    double comaWidthRatio = 0.0;          // :7
    double tailWidthRatio = 0.0;          // :9
    double tailMaxLengthRatio = 0.0;      // :11
    double ionTailWidthRatio = 0.0;       // :13
    float radius = 0.0f;                  // :15
    int seed = 0;                         // :17
    CometOrbitType* cometType = nullptr;  // :19
    int numGeysers = 0;                   // :21
    int numNearDustEmitters = 0;          // :23
    // CometDefinition.cs:25
    ConfigNode::NodePtr CreateVesselNode(bool optimizedCollider, float fragmentDynamicPressureModifier, bool hasName) const;
};

// ================================================================================================================================ CometOrbitType

// CometOrbitType.cs:5 (the two members SentinelScenario.SpawnComet reads are the ICometOrbitType interface of sentinel.hpp)
class CometOrbitType : public SentinelMission::ICometOrbitType {
public:
    std::string name;                 // :8
    std::string displayName;          // :11
    std::string description;          // :14
    float inclination1Min = 0.0f;     // :17
    float inclination1Max = 0.0f;     // :20
    float inclination2Min = 0.0f;     // :23
    float inclination2Max = 0.0f;     // :26
    float inclination1Chance = 0.8f;  // :29
    float peMin = 0.0f;               // :32
    float peMax = 0.0f;               // :35
    float apMin = 0.0f;               // :38
    float apMax = 0.0f;               // :41
    bool useEccentricity = false;     // :44
    float eccentricityMin = 1.01f;    // :47
    float eccentricityMax = 1.4f;     // :50
    UntrackedObjectClass minCometClass = UntrackedObjectClass::F;  // :53
    UntrackedObjectClass maxCometClass = UntrackedObjectClass::I;  // :56
    int chanceWeight = 5;             // :59

    CometOrbitType() = default;       // :67 (internal)
    // :72
    CometOrbitType(std::string name, float inclination1Min, float inclination1Max, float inclination2Min, float inclination2Max, float inclination1Chance,
                   float PEMin, float PEMax, float APMin, float APMax, bool UseEcc, float EccMin, float EccMax, UntrackedObjectClass minCometClass,
                   UntrackedObjectClass maxCometClass);

    Orbit CalculateHomeOrbit();                                                                                         // :91
    Orbit CalculateSentinelOrbit(const Orbit& sentinelOrbit, double minDiscoveryDistance, bool& validOrbit) override;  // :97
    UntrackedObjectClass GetRandomObjClass() override;                                                                  // :185
    void Load(const ConfigNode& node);                                                                                  // :190

private:
    Orbit CalculateOrbit(bool homeOrbit, const Orbit* sentinelOrbit, double minDiscoveryDistance, bool& validOrbit);  // :102
    double trueAnom = 0.0;             // :61
    double eccAnom = 0.0;              // :63
    double meanAnomalyAtEpoch = 0.0;   // :65
};

// ================================================================================================================================ CometManager

// What CometSpawnFragments reads of a CometVessel component (CometVessel.cs; the component itself is not ported)
struct CometVesselSeam {
    Vessel* Vessel_ = nullptr;           // CometVessel.Vessel
    CometOrbitType* CometType = nullptr; // CometVessel.CometType
    float radius = 0.0f;                 // CometVessel.radius
};

// CometManager.cs:6 (MonoBehaviour: Awake / Start / OnDestroy are called by the host)
class CometManager {
public:
    // CometManager.cs:9
    struct CometTailColor {
        Color emitterColor{0.04313725f, 0.2906635f, 13.0f / 15.0f, 23.0f / 51.0f};  // :12
        int chanceWeight = 5;                                                          // :15
        void Load(const ConfigNode& node);                                             // :17
        void Save(ConfigNode& node) const;                                             // :23
    };

    // ---- host seams (every one is scene / other-package code) -------------------------------------------------------------------------------
    // FXMonger.ExplodeWithDebris(cometVessel.transform.position, scale, objectToFollow) (SpawnCometFragments :464). Unset: nothing.
    static inline std::function<void(Vessel& comet, float scale, Vessel* objectToFollow)> explodeWithDebris;
    // originalComet.GetComponent<ModuleComet>().fragmentDynamicPressureModifier (SpawnCometFragment :481); nullopt: no ModuleComet. Unset: no ModuleComet.
    static inline std::function<std::optional<float>(Vessel&)> cometFragmentDynamicPressureModifier;
    // GameDatabase.Instance.GetConfigNodes(type) (COMET_MANAGER, COMET_ORBIT_TYPE; ScenarioDiscoverableObjects: SPACEOBJECTS_LIMITS). Unset: the GameDatabase
    // singleton; with neither, the C# NullReferenceException.
    static std::vector<ConfigNode*> GetConfigNodes(const std::string& type);

    // ---- fields (public in the C#) -----------------------------------------------------------------------------------------------------------
    std::vector<std::shared_ptr<CometOrbitType>> cometDefinitions;  // :39
    float maxSpawnDistance = 2.0f;                                   // :41
    float minSpawnDistance = 1.5f;                                   // :43
    float maxSentinelSpawnDistance = 2.0f;                           // :45
    float minSentinelSpawnDistance = 1.1f;                           // :47
    float explosionScaleMultiplier = 0.2f;                           // :49
    FloatCurve maxCometsPerUntrackedObjects{std::vector<Keyframe>{Keyframe(10.0f, 1.0f), Keyframe(250.0f, 5.0f)}};  // :53
    FloatCurve spawnChance{std::vector<Keyframe>{Keyframe(0.0f, 0.1f), Keyframe(1.0f, 0.0f)}};                     // :53
    double maxVFXStartDistance = 2.0;                                // :69
    double minVFXStartDistance = 0.8;                                // :71
    FloatCurve vfxStartDistribution{std::vector<Keyframe>{Keyframe(0.0f, 0.0f), Keyframe(1.0f, 1.0f)}};  // :54
    double maxComaSizeRatio = 1000000.0;                             // :79
    double minComaSizeRatio = 10000.0;                               // :81
    FloatCurve comaSizeRatioDistribution{std::vector<Keyframe>{Keyframe(0.0f, 0.0f), Keyframe(1.0f, 1.0f)}};  // :60
    double maxTailWidthRatio = 1.0;                                  // :89
    double maxIonTailWidthRatio = 0.3;                               // :91
    FloatCurve tailWidthDistribution{std::vector<Keyframe>{Keyframe(0.0f, 0.0f), Keyframe(1.0f, 1.0f)}};  // :76
    double maxTailLengthRatio = 100000.0;                            // :99
    double minTailLengthRatio = 1000.0;                              // :101
    FloatCurve tailLengthRatioDistribution{std::vector<Keyframe>{Keyframe(0.0f, 0.0f), Keyframe(1.0f, 1.0f)}};  // :86
    FloatCurve vfxSizeFromDistance{std::vector<Keyframe>{Keyframe(0.0f, 1.0f), Keyframe(1.0f, 0.0f)}};  // :95
    FloatCurve nearDustVFXSizeFromDistance{std::vector<Keyframe>{Keyframe(0.0f, 1.0f), Keyframe(1.0f, 0.0f)}};  // :96
    FloatCurve geyserVFXSizeFromDistance{std::vector<Keyframe>{Keyframe(0.0f, 1.0f), Keyframe(1.0f, 0.0f)}};  // :105
    float atmospherePressureForVFXFade = 0.05f;                      // :134
    FloatCurve atmospherePressureFadeDistributionForVFXFade{std::vector<Keyframe>{Keyframe(0.0f, 0.0f), Keyframe(1.0f, 1.0f)}};  // :118
    float gravityAdditionForVFXFadeStart = 300.0f;                   // :144
    float gravityMultiplierForVFXFadeEnd = 100.0f;                   // :147
    float gravitySubtractionForVFXFadeEnd = 20.0f;                   // :150
    double minGeyserRadiusRatio = 0.04;                              // :153
    double maxGeyserRadiusRatio = 0.04;                              // :156
    FloatCurve geysersRatioDistribution{std::vector<Keyframe>{Keyframe(0.0f, 0.0f), Keyframe(1.0f, 1.0f)}};  // :140
    float geyserVisibleDistance = 500.0f;                            // :166
    float geyserSizeRadiusMultiplier = 0.05f;                        // :171
    float nearDustVisibleRadiusMultiplier = 2.0f;                    // :174
    double minNearDustEmitterRadiusRatio = 0.1;                      // :177
    double maxNearDustEmitterRadiusRatio = 0.1;                      // :180
    float vesselRangeOrbitLoadOverride = 150000.0f;                  // :185
    float vesselRangeOrbitUnloadOverride = 151000.0f;                // :188
    float vesselRangeOrbitPackOverride = 1100.0f;                    // :191
    float vesselRangeOrbitUnpackOverride = 1000.0f;                  // :194
    float fragmentdynamicPressurekPa = 10.0f;                        // :196
    int minFragments = 3;                                            // :198
    int maxFragments = 5;                                            // :200
    int minFragmentClassStep = 1;                                    // :202
    int maxFragmentClassStep = 3;                                    // :204
    float fragmentMaxPressureModifier = 1.0f;                        // :206
    float fragmentMinPressureModifier = 6.0f;                        // :208
    float fragmentMinSafeTime = 3.0f;                                // :210
    float fragmentMaxSafeTime = 20.0f;                               // :212
    float fragmentInstanceOffset = 2.0f;                             // :214
    float fragmentAngleVariation = 22.5f;                            // :216
    int fragmentIgnoreCollisions = 20;                               // :218
    std::vector<CometTailColor> ionTailColors;                       // :242
    std::vector<CometTailColor> dustTailColors;                      // :245
    Color ionTailColorP67{0.04313725f, 0.2906635f, 13.0f / 15.0f, 23.0f / 51.0f};  // :248

    static inline CometManager* Instance = nullptr;                  // :250

    CometManager();
    ~CometManager();
    CometManager(const CometManager&) = delete;
    CometManager& operator=(const CometManager&) = delete;

    std::vector<Vessel*>& Comets() { return comets_; }               // :252
    std::vector<Vessel*> DiscoveredComets() const;                   // :254
    static float SentinelDiscoveryArc();                             // :270 (Instance.sentinelAngle)
    static double VFXUpdateFrequency();                              // :272
    static double VFXUpdateFrequencyInAtmosphere();                  // :284
    // true when Awake found an existing Instance (the C# destroys the new object and clears Instance)
    bool destroyed() const { return destroyed_; }

    void Awake();                                                    // :296
    void Start();                                                    // :313
    void OnDestroy();                                                // :320
    void OnGameDatabaseLoaded();                                     // :325
    void OnVesselAdded(Vessel* v);                                   // :331
    void OnVesselRemoved(Vessel* v);                                 // :339
    float GetSpawnChance();                                          // :353
    static bool ShouldSpawnComet();                                  // :370
    static std::shared_ptr<CometOrbitType> GenerateWeightedCometType();  // :379
    static double GenerateHomeSpawnDistance();                       // :408
    static double GenerateSentinelSpawnDistance(double distance);    // :422
    static std::shared_ptr<CometOrbitType> GetCometOrbitType(const std::string& name);  // :431
    void LoadManagerDefinitions();                                   // :454
    void LoadCometTypes();                                           // :565
    static CometDefinition GenerateDefinition(CometOrbitType& cometType, UntrackedObjectClass objClass, int seed);  // :577
    void SpawnCometFragments(const CometVesselSeam& cometVessel);    // :604
    static Color GenerateWeightedIonTailColor(std::optional<int> cometSeed);   // :784 (CometVessel.seed; nullopt = comet == null)
    static Color GenerateWeightedDustTailColor(int cometSeed);                 // :817

    // :156 / :158 / :160 / :162 (private fields the C# keeps)
    float sentinelAngle = 70.0f;                                     // :33
    double vFXUpdateFrequency = 30.0;                                // :128
    double vFXUpdateFrequencyInAtmosphere = 0.029999999329447746;    // :131
    float currentComets = 0.0f, cometCap = 0.0f, cometChance = 0.0f;  // :223-:208
    std::unique_ptr<KSPRandom> generator;                            // :220

    static inline double spawnSMADefault = 13600000000.0;            // :231
    static inline float minSpawnDefault = 1.5f;                      // :233
    static inline float maxSpawnDefault = 2.0f;                      // :235
    static inline float minSentinelSpawnDefault = 1.1f;              // :237
    static inline float maxSentinelSpawnDefault = 1.5f;              // :239

private:
    AddedVessel SpawnCometFragment(const std::string& cometName, Vessel& originalComet, CometOrbitType& cometOrbitType, UntrackedObjectClass explodeCometClass,
                                    float cometRadius, bool focusAfterSpawned);  // :614
    void LoadTailColorLists();                                       // :737
    std::vector<CometTailColor> LoadColorsList(const ConfigNode& colorsNode);  // :766 (private)

    std::vector<Vessel*> comets_;                                    // (C++ addition)
    ConfigNode* managerDef_ = nullptr;                               // (C++ addition)
    bool destroyed_ = false;
    bool subscribed_ = false;
};

// ================================================================================================================================ DiscoverableObjectsUtil

// ProtoVessel.CreateVesselNode / CreatePartNode / CreateDiscoveryNode as DiscoverableObjectsUtil calls them (ProtoVessel.cs:853, :903, :982): the WP-33 builders of
// ProtoVesselNodes have seam defaults (identity rotation, persistentId 0) that would change the draw sequence of the space-object spawners; these consume the
// Unity generator and the game's id registry like the C#.
struct SpaceObjectNodes {
    // ProtoVessel.cs:853 (additionalNodes after the part nodes; ACTIONGROUPS is added when absent)
    static ConfigNode::NodePtr CreateVesselNode(const std::string& vesselName, VesselType vesselType, const Orbit& orbit, int rootPartIndex,
                                                const std::vector<ConfigNode::NodePtr>& partNodes, const std::vector<ConfigNode::NodePtr>& additionalNodes);
    // ProtoVessel.cs:903 public CreatePartNode(partName, id, crew): the prefab path of ProtoVesselNodes::CreatePartNode with FlightGlobals.GetUniquepersistentId()
    static ConfigNode::NodePtr CreatePartNode(const std::string& partName, uint32_t id);
    // ProtoVessel.cs:982
    static ConfigNode::NodePtr CreateDiscoveryNode(DiscoveryLevels level, UntrackedObjectClass size, double lifeTime, double maxLifeTime);
};

struct SpaceObjectsEnvironment {
    // GameDatabase.Instance.GetConfigNodes(type), shared by CometManager and ScenarioDiscoverableObjects (see CometManager::GetConfigNodes)
    static inline std::function<std::vector<ConfigNode*>(const std::string&)> gameDatabaseConfigNodes;
    // Unset: new ProtoVessel(node, game), Load(game.flightState) (the registry of FlightGlobals gets the vessel); both are kept alive in `spawned`;
    // GameEvents.onNewVesselCreated is fired with the vessel.
    static inline std::function<AddedVessel(const ConfigNode&)> addVessel;
    struct Spawned {
        std::unique_ptr<ProtoVessel> protoVessel;
        std::unique_ptr<Vessel> vessel;
    };
    static inline std::vector<Spawned> spawned;
    // Every vessel AddVessel was asked to add, in order, whatever addVessel is (the tests read the node formats from here)
    static inline std::vector<ConfigNode::NodePtr> addedNodes;
    // ProgressTracking.Instance.GetBodyTree(name).IsReached (ScenarioDiscoverableObjects.ReachedBody). Unset: the ProgressTracking singleton.
    static inline std::function<bool(const std::string&)> bodyReached;
    // GameVariables.Instance.UnlockedSpaceObjectDiscovery(ScenarioUpgradeableFacilities.GetFacilityLevel(TrackingStation)). Unset: GameVariables + upgradeables.
    static inline std::function<bool()> discoveryUnlocked;
    // FlightGlobals.PersistentVesselIds.Contains(id). Unset: the PersistentIdRegistry of the SaveEnvironment.
    static inline std::function<bool(uint32_t)> persistentVesselIdKnown;
    static void Reset();
};

// DiscoverableObjectsUtil.cs:5
struct DiscoverableObjectsUtil {
    static inline int specialNameChance = 20;                    // :7
    static std::string GenerateAsteroidName();                   // :9
    static AddedVessel SpawnAsteroid(const std::string& asteroidName, const Orbit& o, uint32_t seed, UntrackedObjectClass objClass, double lifeTime,
                                     double lifeTimeMax);        // :39
    static AddedVessel SpawnComet(const std::string& cometName, const Orbit& o, const CometDefinition& cometDef, uint32_t seed, UntrackedObjectClass objClass,
                                  double lifeTime, double lifeTimeMax, bool optimizedCollider, float fragmentDynamicPressureModifier);  // :48
    static std::string GenerateCometName(KSPRandom* generator = nullptr);  // :65
    // :20 / :31 (private)
    static char randomLetter(bool uppercase);
    static std::string randomNumber(int digits);
};

// ================================================================================================================================ ScenarioDiscoverableObjects

// ScenarioDiscoverableObjects.cs:8 ([KSPScenario(AddToAllGames, FLIGHT, TRACKSTATION, SPACECENTER)]); sizeCurve and lastSeed are the [KSPField]s of the store
class ScenarioDiscoverableObjects : public ScenarioModule {
public:
    ScenarioDiscoverableObjects();
    ~ScenarioDiscoverableObjects() override;

    float spawnInterval = 15.0f;                                 // :15
    float maxUntrackedLifetime = 20.0f;                          // :17
    float minUntrackedLifetime = 1.0f;                           // :19
    int spawnOddsAgainst = 2;                                    // :21
    int spawnGroupMinLimit = 3;                                  // :23
    int spawnGroupMaxLimit = 10;                                 // :25
    UntrackedObjectClass minAsteroidClass = UntrackedObjectClass::A;  // :33
    UntrackedObjectClass maxAsteroidClass = UntrackedObjectClass::E;  // :35
    std::vector<uint32_t> untrackedObjectIDs;                    // :45
    std::vector<uint32_t> discoveredObjectIDs;                   // :47

    static inline ScenarioDiscoverableObjects* Instance = nullptr;  // :55 (protected set)

    FloatCurve& sizeCurve();                                     // :28 (the persistent field; null before OnAwake)
    bool hasSizeCurve();
    int lastSeed();                                              // :31
    void setLastSeed(int v);

    void OnLoad(ConfigNode& node) override;                      // :57
    void LoadGameDBSettings(const std::vector<const ConfigNode*>& spaceObjectsLimits);  // :64
    std::vector<uint32_t> loadIDList(const ConfigNode& node, const std::string& listName);  // :85
    void OnSave(ConfigNode& node) override;                      // :102
    void SaveIDList(ConfigNode& node, const std::string& name, const std::vector<uint32_t>& loadList);  // :108
    void OnAwake() override;                                     // :114
    void OnKnowledgeChanged(events::GameEvents::HostedFromToAction<IDiscoverable*, DiscoveryLevels> kChg);  // :137
    void OnVesselDestroy(Vessel* v);                             // :153
    void Start();                                                // :159
    void OnDestroy();                                            // :174
    Enumerator SpawnDaemon();                                    // :182
    void UpdateSpaceObjects();                                   // :194
    bool ExpireDiscoveredObjects(double UT);                     // :219
    void SpawnAsteroid();                                        // :250
    void SpawnLastAsteroid();                                    // :266
    void SpawnHomeAsteroid(int asteroidSeed);                    // :279
    UntrackedObjectClass GetSizeCurveBasedClass(float minRange, float maxRange);  // :296
    void SpawnDresAsteroid(int asteroidSeed);                    // :304
    void SpawnComet();                                           // :343
    void SpawnComet(const std::string& typeName);                // :343 (internal)
    void SpawnComet(CometOrbitType& cometType);                  // :343 (internal)
    double GetRandomDuration();                                  // :373
    bool ReachedBody(const std::string& bodyName);               // :378
    static void Register();

    std::vector<std::shared_ptr<Coroutine>> routines;            // the coroutine Start started (stopped in OnDestroy)
    bool destroyed_ = false;                                     // `(bool)this` of the destroyed component

private:
    int currentSize_ = 0;                                        // (C++ addition)
    int rndGroupSize_ = 0;                                       // (C++ addition)
    int newGroupSize_ = 0;                                       // (C++ addition)
    bool discoveryUnlocked_ = false;                             // (C++ addition)
};

// The Sentinel seams (SentinelEnvironment) answered by the classes above: ScenarioDiscoverableObjects.Instance (class limits, untracked count), the space object
// count of the vessel registry, DiscoverableObjectsUtil.GenerateAsteroidName / SpawnAsteroid / SpawnComet, CometManager (ShouldSpawnComet, GenerateWeightedCometType,
// GetCometOrbitType, GenerateDefinition), and CometManagerSeam::discoveredComets (CometManager.Instance.DiscoveredComets). Called by the host after the singletons
// exist (never by FinePrint::InstallAll: the Sentinel tests fill those seams with fixtures).
namespace SpaceObjects {
void InstallSentinelSeams();
void UninstallSentinelSeams();
}  // namespace SpaceObjects

}  // namespace ksp
