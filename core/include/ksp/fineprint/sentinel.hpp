// WP-38 slice 3: the Sentinel mission (Assembly-CSharp/SentinelMission/*.cs): SentinelScanType, SentinelUtilities, SentinelParameter,
// SentinelContract, CometDetectionContract, SentinelModule (the InfraredTelescope part module) and SentinelScenario (the asteroid / comet spawner).
//
// C# `private` / `internal` members are public (the tests drive them). The contract definitions are the typed members of
// FinePrint::ContractDefs (ContractDefs::Sentinel / CometDetection). The host-side systems the spawner reaches and that are other packages are seams
// (SentinelEnvironment): ScenarioDiscoverableObjects (class limits, untracked object count), DiscoverableObjectsUtil (SpawnAsteroid / SpawnComet),
// CometManager (ShouldSpawnComet, comet types, definitions) and the screen message sink (FinePrintEnvironment::Post).
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/contracts/contract.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/fineprint/contracts.hpp"
#include "ksp/fineprint/parameters.hpp"
#include "ksp/fineprint/utilities.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/save/game.hpp"
#include "ksp/vessel/discovery_info.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::SentinelMission {

using ksp::Contracts::Contract;
using ksp::Contracts::ContractParameter;

// SentinelScanType.cs:3
enum class SentinelScanType : int { NONE, CLASS, ECCENTRICITY, INCLINATION };

// A comet orbit type of CometManager (CometOrbitType.cs): the two members SentinelScenario.SpawnComet uses.
class ICometOrbitType {
public:
    virtual ~ICometOrbitType() = default;
    virtual UntrackedObjectClass GetRandomObjClass() = 0;                                                               // CometOrbitType.GetRandomObjClass
    virtual Orbit CalculateSentinelOrbit(const Orbit& sentinelOrbit, double minDiscoveryDistance, bool& validOrbit) = 0;  // :CalculateSentinelOrbit
};

struct SentinelEnvironment {
    // ScenarioDiscoverableObjects.Instance (ScenarioDiscoverableObjects is not ported). nullopt = Instance == null.
    struct DiscoverableObjects {
        UntrackedObjectClass minAsteroidClass = UntrackedObjectClass::A;
        UntrackedObjectClass maxAsteroidClass = UntrackedObjectClass::I;
        float minUntrackedLifetime = 0.0f;                   // days
        float maxUntrackedLifetime = 0.0f;
        int untrackedObjectIDsCount = 0;                     // untrackedObjectIDs.Count
    };
    static inline std::optional<DiscoverableObjects> discoverableObjects;
    // FlightGlobals.CountSpaceObjects(). Unset: 0.
    static inline std::function<int()> countSpaceObjects;
    // DiscoverableObjectsUtil.GenerateAsteroidName / SpawnAsteroid(name, orbit, seed, class, lifetime, lifetimeMax). Unset: nothing is spawned.
    static inline std::function<std::string()> generateAsteroidName;
    static inline std::function<void(const std::string& name, Orbit& orbit, uint32_t seed, UntrackedObjectClass cls, double lifeTime, double lifeTimeMax)> spawnAsteroid;
    // CometManager: ShouldSpawnComet(), GenerateWeightedCometType(), GetCometOrbitType(typeName) (nullptr: unknown type),
    // GenerateDefinition(type, class, seed) -> vfxStartDistance, DiscoverableObjectsUtil.SpawnComet(...). Unset: no comets.
    static inline std::function<bool()> shouldSpawnComet;
    static inline std::function<std::shared_ptr<ICometOrbitType>()> generateWeightedCometType;
    static inline std::function<std::shared_ptr<ICometOrbitType>(const std::string& typeName)> getCometOrbitType;
    struct CometDefinitionSeam {
        double vfxStartDistance = 0.0;       // CometDefinition.vfxStartDistance
        std::shared_ptr<void> payload;       // the host's CometDefinition
    };
    static inline std::function<CometDefinitionSeam(ICometOrbitType&, UntrackedObjectClass, int seed)> generateCometDefinition;
    static inline std::function<void(const std::string& name, Orbit& o, CometDefinitionSeam& def, uint32_t seed, UntrackedObjectClass cls, double lifeTime,
                                     double lifeTimeMax, bool optimizedCollider, float unused)> spawnComet;
    static void Reset();
};

// SentinelUtilities.cs:9
struct SentinelUtilities {
    // private statics :11-:27, public static :29-:37 (properties :43-:132 read and write the private fields)
    static inline std::string SentinelPartName = "InfraredTelescope";
    static inline std::string SentinelPartDisplayName = "#autoLOC_6002282";
    static inline std::string SentinelModuleName = "SentinelModule";
    static inline std::string SentinelModuleDisplayName = "#autoLOC_6002283";
    static inline double SentinelViewAngle = 200.0;
    static inline float SpawnChance = 0.25f;
    static inline int WeightedStability = 3;
    static inline double MinAsteroidEccentricity = 0.05;
    static inline double MaxAsteroidEccentricity = 0.4;
    static inline double MinAsteroidInclination = 5.0;
    static inline double MaxAsteroidInclination = 40.0;
    static inline int perSentinelObjectLimit = 10;
    static inline std::string sentinelPartTitle;   // :36 (always "")

    static void Reset();   // C++ helper: the C# initialisers
    static std::string SentinelPartTitle();                                                       // :122
    static bool SentinelCanScan(Vessel& v, CelestialBody* innerBody = nullptr, CelestialBody* outerBody = nullptr);  // :139
    static bool FindInnerAndOuterBodies(double SMA, CelestialBody*& innerBody, CelestialBody*& outerBody);          // :198
    static bool FindInnerAndOuterBodies(Orbit& o, CelestialBody*& innerBody, CelestialBody*& outerBody);            // :251
    static bool FindInnerAndOuterBodies(Vessel& v, CelestialBody*& innerBody, CelestialBody*& outerBody);           // :274
    static double AdjustedSentinelViewAngle(const Orbit& innerOrbit, const Orbit& outerOrbit);    // :277
    static double GetEscapeVelocity(CelestialBody* body, double altitude);                        // :319
    static double GetMinimumOrbitalSpeed(CelestialBody* body);                                    // :311
    static double GetProgradeBurnAllowance(Orbit& o);                                             // :317
    static double GetRetrogradeBurnAllowance(Orbit& o);                                           // :322
    static UntrackedObjectClass GetVesselClass(Vessel& v);                                        // :327
    static SentinelScanType RandomScanType(KSPRandom* generator = nullptr);                       // :344
    static UntrackedObjectClass WeightedAsteroidClass(KSPRandom* generator = nullptr);            // :349
    static double WeightedRandom(KSPRandom* generator);                                           // :383
    static double WeightedRandom(KSPRandom* generator, double min, double max);                   // :383
    static double WeightedRandom(KSPRandom* generator, double max);                               // :383
    static double RandomRange(KSPRandom* generator = nullptr, double min = -1.7976931348623157e308, double max = 1.7976931348623157e308);  // :386
    static float CalculateReadDuration(const std::string& s);                                     // :396
    static bool IsOnSolarOrbit(CelestialBody* body);                                              // :403
};

// SentinelParameter.cs:11
class SentinelParameter : public ContractParameter {
public:
    CelestialBody* FocusBody = nullptr;                       // :12
    int TotalDiscoveries = 0;                                 // :14
    int RemainingDiscoveries = 0;                             // :16
    SentinelScanType ScanType = SentinelScanType::NONE;       // :18
    UntrackedObjectClass TargetSize = UntrackedObjectClass::A;  // :20
    double MinimumEccentricity = 0.0;                         // :22
    double MinimumInclination = 0.0;                          // :24
    SentinelParameter();                                      // :26
    SentinelParameter(CelestialBody* focusBody, SentinelScanType scanType, UntrackedObjectClass targetSize, double minimumEccentricity,
                      double minimumInclination, int discoveryCount);  // :42
    std::string TypeName() const override { return "SentinelParameter"; }
    std::string GetHashString() override;                     // :55
    std::string GetTitle() override;                          // :60
    std::string GetNotes() override;                          // :76
    void OnRegister() override;                               // :81
    void OnSave(ConfigNode& node) override;                   // :86
    void OnLoad(ConfigNode& node) override;                   // :97
    void DiscoverAsteroid(UntrackedObjectClass size, double eccentricity, double inclination, CelestialBody* body);  // :108
};

// SentinelContract.cs:13
class SentinelContract : public Contract, public FinePrint::IUpdateWaypoints {
public:
    std::unique_ptr<KSPRandom> generator;                     // :16
    CelestialBody* innerBody = nullptr;                       // :18
    CelestialBody* outerBody = nullptr;                       // :20
    int asteroidCount = 3;                                    // :22
    std::string TypeName() const override { return "SentinelContract"; }
    bool Generate() override;                                 // :24
    bool CanBeCancelled() override { return true; }           // :170
    bool CanBeDeclined() override { return true; }            // :175
    std::string GetHashString() override;                     // :180
    std::string GetTitle() override;                          // :185
    std::string GetDescription() override;                    // :195
    std::string GetSynopsys() override;                       // :200
    std::string MessageCompleted() override;                  // :211
    void OnLoad(ConfigNode& node) override;                   // :217
    void OnSave(ConfigNode& node) override;                   // :224
    bool MeetRequirements() override;                         // :233
    void SetFocusBody(CelestialBody* body);                   // :246
};

// CometDetectionContract.cs:12
class CometDetectionContract : public Contract, public FinePrint::IUpdateWaypoints {
public:
    int cometCount = 3;                                       // :14
    std::string TypeName() const override { return "CometDetectionContract"; }
    bool Generate() override;                                 // :16
    bool CanBeCancelled() override { return true; }           // :82
    bool CanBeDeclined() override { return true; }            // :87
    std::string GetHashString() override;                     // :92
    std::string GetTitle() override;                          // :97
    std::string GetDescription() override;                    // :102
    std::string GetSynopsys() override;                       // :107
    std::string MessageCompleted() override;                  // :112
    void OnLoad(ConfigNode& node) override;                   // :117
    void OnSave(ConfigNode& node) override;                   // :122
    bool MeetRequirements() override;                         // :127
};

// SentinelModule.cs:6 (PartModule; fields isTracking, status)
class SentinelModule : public PartModule {
public:
    explicit SentinelModule(const ModuleClassInfo& cls) : PartModule(cls) {}
    static inline std::string cacheAutoLOC_6002290;           // :13
    static inline std::string cacheAutoLOC_6002296;           // :15
    Part* PartOf() const;
    bool isTracking() const { return Get<bool>("isTracking"); }
    void setIsTracking(bool v) { Set<bool>("isTracking", v); }
    std::string status() const { return Get<std::string>("status"); }
    std::string FocusName();                                  // :17
    void OnAwake() override;                                  // (C++ addition) binds the two [KSPEvent] methods
    void OnStart(StartState state) override;                  // :26
    void StartTracking();                                     // :41
    void StopTracking();                                      // :54
    void FixedUpdate();                                       // :63 (called by the host's FixedUpdate)
    void OnFixedUpdate() override { FixedUpdate(); }
    bool TelescopeCanActivate();                              // :82
    void ShowMessage(const std::string& s);                   // :103
    static void CacheLocalStrings();                          // :108
    static void Register();
};

// SentinelScenario.cs:15 ([KSPScenario(AddToAllGames, FLIGHT, TRACKSTATION, SPACECENTER)])
class SentinelScenario : public ScenarioModule {
public:
    SentinelScenario();
    ~SentinelScenario() override;
    static inline SentinelScenario* Instance = nullptr;       // :25
    double NextSpawnTime() const;                             // :19 (persistent field; double.MinValue initial)
    void setNextSpawnTime(double v);
    std::unique_ptr<KSPRandom> generator;                     // :21
    std::vector<Vessel*> deployedSentinels;                   // :23
    int MaxSentinelUntrackedSpaceObjects() const { return static_cast<int>(deployedSentinels.size()) * SentinelUtilities::perSentinelObjectLimit; }  // :27
    void OnAwake() override;                                  // :29
    void Start();                                             // :40
    void OnLoad(ConfigNode& node) override;                   // :50
    void LoadGameDBSettings(const std::vector<const ConfigNode*>& spaceObjectsLimits);  // :55
    void OnDestroy();                                         // :77
    Enumerator SentinelRoutine();                             // :82
    Enumerator ProcessSentinelScan(Vessel* v);                // :103
    void SpawnComet();                                        // :148
    void SpawnComet(Vessel* sentinel);                        // :154
    void SpawnComet(Vessel* sentinel, const std::string& typeName);  // :167
    void SpawnComet(Vessel* sentinel, ICometOrbitType& cometType);   // :167
    std::vector<Vessel*> GetDeployedSentinels();              // :185
    bool ActiveSentinelMatchingOrbit(Orbit* orbit, double deviationWindow);  // :247
    void ResetSentinels();                                    // :269
    Orbit SentinelAsteroidOrbit(const Orbit& orbit);          // :312
    static void Register();
    std::vector<std::shared_ptr<Coroutine>> routines;         // the coroutines StartCoroutine started (stopped in OnDestroy)
    bool destroyed_ = false;                                  // `(bool)this` of the destroyed component
};

}  // namespace ksp::SentinelMission

// Enum.ToString / Enum.Parse metadata (the names are the C# member names)
template <>
struct ksp::dotnet::EnumTraits<ksp::SentinelMission::SentinelScanType> {
    using E = ksp::SentinelMission::SentinelScanType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::NONE, "NONE"}, {E::CLASS, "CLASS"}, {E::ECCENTRICITY, "ECCENTRICITY"}, {E::INCLINATION, "INCLINATION"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::UntrackedObjectClass> {
    using E = ksp::UntrackedObjectClass;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::A, "A", "#autoLOC_184791"}, {E::B, "B", "#autoLOC_184793"}, {E::C, "C", "#autoLOC_184795"},
                                                {E::D, "D", "#autoLOC_184797"}, {E::E, "E", "#autoLOC_184799"}, {E::F, "F", "#autoLOC_6011124"},
                                                {E::G, "G", "#autoLOC_6011125"}, {E::H, "H", "#autoLOC_6011126"}, {E::I, "I", "#autoLOC_6011129"}};  // UntrackedObjectClass.cs [Description]
};
