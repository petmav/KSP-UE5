// The planetary resource layer of WP-15 (what Kerbal's resource scanner / drills read):
//   NoiseGenerator.cs, ResourceUtilities.cs, ResourceGameSettings.cs, ResourceCache.cs, ResourceMap.cs, ResourceScenario.cs.
//   (data records: resource_types.hpp)
//
// ResourceMap.GetAbundance(request) is the abundance (0..1 fraction, float) of a resource at a latitude/longitude of a body: the
// resource definitions (BIOME_RESOURCE / PLANETARY_RESOURCE / GLOBAL_RESOURCE nodes of GameData/Squad/Resources/*.cfg) are merged
// to the best distribution, a KSPRandom seeded from the game seed, body, resource and biome names draws the presence chance and
// abundance range, and a 3D simplex-style NoiseGenerator adds the lat/lon variance. Everything but the biome-map pixel lookup is
// ported; the pixel lookup (CBAttributeMapSO.GetAtt) is a host hook (ResourceEnvironment::getBiomeAtt) until the MapSO decoder
// exists: without it a request with an empty BiomeName falls back to the default situation, exactly like a GetAtt exception.
#pragma once

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/resources/resource_types.hpp"
#include "ksp/save/game.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

// What the resource map reads from other subsystems (FlightGlobals.Bodies, HighLogic difficulty, the Planetarium clock, the biome map)
struct ResourceEnvironment {
    static inline std::function<const std::vector<CelestialBody*>&()> bodies;  // FlightGlobals.Bodies
    // CBAttributeMapSO.GetAtt(lat, lon) of body.BiomeMap (rad); nullptr = no attribute (or the C# exception path)
    static inline std::function<const CBAttributeMapSO::MapAttribute*(double lat, double lon, CelestialBody&)> getBiomeAtt;
    // HighLogic.CurrentGame.Parameters.Difficulty.ResourceAbundance; unset: SaveEnvironment::Current().currentGame
    static inline std::function<float()> difficultyResourceAbundance;
    static inline std::function<double()> universalTime;  // Planetarium.GetUniversalTime(); unset: Planetarium
    // Environment.TickCount (the seed of `new KSPRandom()` / `new System.Random()`); unset: a monotonic millisecond counter
    static inline std::function<int32_t()> tickCount;
    static int32_t TickCount();
};

// NoiseGenerator.cs:5 (all float state is Mono-rule float32)
class NoiseGenerator {
public:
    NoiseGenerator();                                                    // :27 (T = 8 System.Random values)
    void SetSeed(const std::array<int, 8>& seed);                        // :37
    std::string GetSeed() const;                                         // :45
    float coherentNoise(float x, float y, float z, int octaves = 1, int multiplier = 25, float amplitude = 0.5f, float lacunarity = 2.0f,
                        float persistence = 0.9f);                       // :60
    int getDensity(const Vector3f& loc);                                 // :75
    float noise(float x, float y, float z);                              // :82

private:
    float kay(int a);                                                    // :97
    int shuffle(int i, int j, int k) const;                              // :158
    int b(int i, int j, int k, int B) const;                             // :163
    static int b(int N, int B);                                          // :168
    static int fastfloor(float n);                                       // :173
    std::array<int, 3> A_{};
    std::array<int, 8> T_{};
    int i_ = 0, j_ = 0, k_ = 0;
    float onesixth_, onethird_;
    float s_ = 0, u_ = 0, v_ = 0, w_ = 0;
};

// ResourceUtilities.cs:11
class ResourceUtilities {
public:
    static constexpr double FLOAT_TOLERANCE = 1E-09;      // :14
    static constexpr double SECONDS_PER_TICK = 0.02;      // :16
    // :18 GetBiome: body.BiomeMap.GetAtt(lat, lon) with every exception turned into null
    static const CBAttributeMapSO::MapAttribute* GetBiome(double lat, double lon, CelestialBody& body);
    static std::vector<std::shared_ptr<ResourceData>> ImportConfigNodeList(const std::vector<ConfigNode*>& nodes);        // :46
    static std::vector<std::shared_ptr<DepletionData>> ImportDepletionNodeList(const std::vector<ConfigNode*>& nodes);    // :64
    static std::vector<std::shared_ptr<BiomeLockData>> ImportBiomeLockNodeList(const std::vector<ConfigNode*>& nodes);    // :84
    static std::vector<std::shared_ptr<PlanetScanData>> ImportPlanetScanNodeList(const std::vector<ConfigNode*>& nodes);  // :98
    static double GetValue(const ConfigNode& node, const std::string& name, double curVal);  // :112
    static double GetMaxDeltaTime();                                                         // :120
    static double Deg2Rad(double degrees);                                                   // :156
    static double Rad2Lat(double radians);                                                   // :161
    static double Rad2Lon(double radians);                                                   // :177
    static Color HSL2RGB(double h, double sl, double l, float alpha);                        // :193
    static double clampLat(double lat);                                                      // :253
    static double clampLon(double lon);                                                      // :258
    static float GetDifficultyLevel();                                                       // :263
};

// ResourceGameSettings.cs:6
class ResourceGameSettings {
public:
    static inline int defaultMaxDeltaTime = 21600;           // :8 internal static
    int Seed = 0;                                             // :16
    int MaxDeltaTime = 0;                                     // :18
    ConfigNode* SettingsNode = nullptr;                       // :20 (a node of the game node tree that Load/Save was given)
    int ROCMissionSeed = 0;                                   // :22
    // :24 Load. `missionResourceSeeds`: HighLogic.CurrentGame.Mode == MISSION with MissionSystem.missions.Count > 0
    // -> (resourceSeed, rocMissionSeed) of missions[0].situation (MissionSystem is not ported; unset = no mission)
    static inline std::function<bool(int& resourceSeed, int& rocMissionSeed)> missionSeeds;
    void Load(ConfigNode& node);
    void GenerateNewSeed();                                   // :63 new KSPRandom().Next(1, int.MaxValue)
    void GenerateNewROCMissionSeed();                         // :69
    std::vector<std::shared_ptr<DepletionData>>& GetDepletionInfo();       // :105
    std::vector<std::shared_ptr<BiomeLockData>>& GetBiomeLockInfo();       // :110
    std::vector<std::shared_ptr<PlanetScanData>>& GetPlanetScanInfo();     // :115
    void Save(ConfigNode& node);                               // :120
    static int GetValue(const ConfigNode& config, const std::string& name, int currentValue);      // :190
    static float GetValue(const ConfigNode& config, const std::string& name, float currentValue);  // :199
    void SaveDepletionNodeValue(int planetId, const std::string& resource, int x, int y, float value);  // :209
    void SaveBiomeUnlockNode(int planetId, const std::string& biomeName);  // :237
    void SavePlanetUnlockNode(int planetId);                   // :264

private:
    std::vector<std::shared_ptr<DepletionData>> SetupDepletionInfo();      // :85
    std::vector<std::shared_ptr<BiomeLockData>> SetupBiomeLockInfo();      // :92
    std::vector<std::shared_ptr<PlanetScanData>> SetupPlanetScanInfo();    // :99
    std::shared_ptr<DepletionNode> CheckDepletionNode(int planetId, const std::string& resource, int x, int y);  // :284
    std::vector<std::shared_ptr<DepletionData>> depletionInfo_;
    bool depletionSet_ = false;
    std::vector<std::shared_ptr<BiomeLockData>> biomeLockInfo_;
    bool biomeLockSet_ = false;
    std::vector<std::shared_ptr<PlanetScanData>> planetScanInfo_;
    bool planetScanSet_ = false;
};

// ResourceCache.cs:5 (the lists are static in C#: shared by every instance)
class ResourceCache {
public:
    struct AbundanceSummary {  // :7
        int BodyId = 0;
        std::string BiomeName;
        std::string ResourceName;
        float Abundance = 0;
        HarvestTypes HarvestType = HarvestTypes::Planetary;
    };
    static ResourceCache& Instance();                          // :27 (the GameObject singleton)
    static void DestroyInstance();                             // OnDestroy
    std::vector<std::shared_ptr<AbundanceSummary>>& AbundanceCache();   // :29
    std::vector<std::shared_ptr<ResourceData>>& BiomeResources();       // :31
    std::vector<std::shared_ptr<ResourceData>>& GlobalResources();      // :33
    std::vector<std::shared_ptr<ResourceData>>& PlanetaryResources();   // :35
    void ResetCache();                                         // :44
    // GameDatabase.Instance.GetConfigNodes(type): the host's database
    static inline std::function<std::vector<ConfigNode*>(const std::string&)> getConfigNodes;

private:
    std::shared_ptr<AbundanceSummary> RequestSummary(CelestialBody& body, const std::string& biome, const std::string& resourceName, HarvestTypes type);  // :52
    std::vector<std::shared_ptr<AbundanceSummary>> LoadAbundanceCache();  // :70
    std::vector<std::shared_ptr<ResourceData>> LoadResourceInfo(const std::string& node);  // :122
    static inline std::unique_ptr<ResourceCache> instance_;
    static inline std::vector<std::shared_ptr<AbundanceSummary>> abundanceCache_;
    static inline bool abundanceLoaded_ = false;
    static inline std::vector<std::shared_ptr<ResourceData>> globalResources_, planetaryResources_, biomeResources_;
    static inline bool globalLoaded_ = false, planetaryLoaded_ = false, biomeLoaded_ = false;
};

// ResourceMap.cs:6
class ResourceMap {
public:
    static ResourceMap* Instance() { return instance_; }                      // :27
    static bool Initialized() { return instance_ != nullptr; }                // :29
    ResourceMap();                                                            // Awake (:73)
    ~ResourceMap();                                                           // OnDestroy (:83)
    ResourceMap(const ResourceMap&) = delete;
    ResourceMap& operator=(const ResourceMap&) = delete;

    std::vector<std::shared_ptr<DepletionData>>& DepletionInfo();             // :31
    std::vector<std::shared_ptr<BiomeLockData>>& BiomeLockInfo();             // :45
    std::vector<std::shared_ptr<PlanetScanData>>& PlanetScanInfo();           // :59
    void ResetCache();                                                        // :90
    std::shared_ptr<DepletionNode> GetDepletionNode(int planetId, const std::string& resource, int x, int y);  // :98
    float GetDepletionNodeValue(int planetId, const std::string& resource, int x, int y);  // :129
    bool IsBiomeUnlocked(int planetId, const std::string& biomeName);         // :134
    bool IsPlanetScanned(int planetId);                                       // :155
    void SetDepletionNodeValue(int planetId, const std::string& resource, int x, int y, float value);  // :175
    void UnlockBiome(int planetId, const std::string& biomeName);             // :202
    void UnlockPlanet(int planetId);                                          // :216
    std::vector<std::string> FetchAllResourceNames(HarvestTypes harvest);     // :229
    int GenerateAbundanceSeed(const AbundanceRequest& request, const std::string& biomeName);  // :331
    std::string DetermineBiomeName(const AbundanceRequest& request);          // :363
    float GetAbundanceFromCache(const AbundanceRequest& request, const std::string& biomeName);  // :386
    float GetAbundance(const AbundanceRequest& request);                      // :402
    static std::string GetDefaultSituation(HarvestTypes type);                // :578
    Vector2f GetDepletionNode(double latitude, double longitude);             // :592
    PlanetaryResource GetResourceByName(const std::string& resName, CelestialBody& body, HarvestTypes harvest);  // :602
    std::vector<PlanetaryResource> GetResourceItemList(HarvestTypes harvest, CelestialBody& body);               // :652

private:
    static std::vector<std::string> DeDuplicate(const std::vector<std::string>& input);  // :243
    std::vector<std::string> GetCacheNames(const std::vector<std::shared_ptr<ResourceData>>& resList, HarvestTypes harvest);  // :257
    std::shared_ptr<DistributionData> GetBestResourceData(const std::vector<std::shared_ptr<ResourceData>>& configs);  // :273
    static inline ResourceMap* instance_ = nullptr;
    std::vector<std::shared_ptr<ResourceData>> biomeConfigs, planetConfigs, globalConfigs;
    std::map<int, std::vector<double>> seedsCache;
    std::array<int, 8> noiseSeed{};
    NoiseGenerator spx;
    std::vector<std::shared_ptr<DepletionData>> depletionInfo_;
    bool depletionSet_ = false;
    std::vector<std::shared_ptr<BiomeLockData>> biomeLockInfo_;
    bool biomeLockSet_ = false;
    std::vector<std::shared_ptr<PlanetScanData>> planetScanInfo_;
    bool planetScanSet_ = false;
};

// ResourceScenario.cs:12 ([KSPScenario(3198, FLIGHT, TRACKSTATION, SPACECENTER, EDITOR, MISSIONBUILDER)])
class ResourceScenario : public ScenarioModule {
public:
    ResourceScenario();
    ~ResourceScenario() override;
    static ResourceScenario* Instance() { return instance_; }                // :23
    ResourceGameSettings* gameSettings() { return gameSettings_.get(); }     // :25
    void OnAwake() override;                                                  // :27
    void Start();                                                             // :44
    void OnLoad(ConfigNode& gameNode) override;                               // :49
    void OnSave(ConfigNode& gameNode) override;                               // :59
    static void Register();                                                   // ScenarioModuleFactory registration

private:
    static inline ResourceScenario* instance_ = nullptr;
    std::unique_ptr<ResourceGameSettings> gameSettings_;
    std::unique_ptr<ResourceMap> map_;
};

}  // namespace ksp
