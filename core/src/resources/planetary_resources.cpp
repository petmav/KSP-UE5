// The planetary resource layer of WP-15: see ksp/resources/planetary_resources.hpp.
#include "ksp/resources/planetary_resources.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/dotnet/utf16.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
// C# `%` on doubles (IL rem = fmod: an exact operation, ksp/dotnet/math.hpp header comment)
double Rem(double a, double b) { return std::fmod(a, b); }
// Convert.ChangeType(string, T) of ResourceUtilities.LoadNodeProperties
int ToInt(const std::string& s) { return dotnet::ParseInt32(s); }
float ToFloat(const std::string& s) { return dotnet::ParseSingle(s); }
double ToDouble(const std::string& s) { return dotnet::ParseDouble(s); }
}  // namespace

int32_t ResourceEnvironment::TickCount() {
    if (tickCount) return tickCount();
    return static_cast<int32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

// ---- NoiseGenerator.cs --------------------------------------------------------------------------------------------------------

// NoiseGenerator.cs:28 (`new System.Random()` is time-seeded; SetSeed replaces all eight values)
NoiseGenerator::NoiseGenerator() {
    onesixth_ = f32store(1.0 / 6.0);
    onethird_ = f32store(1.0 / 3.0);
    dotnet::SystemRandom random(ResourceEnvironment::TickCount());
    for (int i = 0; i < 8; i++) T_[static_cast<size_t>(i)] = random.Next();
}

// NoiseGenerator.cs:41
void NoiseGenerator::SetSeed(const std::array<int, 8>& seed) {
    for (int i = 0; i < 8; i++) T_[static_cast<size_t>(i)] = seed[static_cast<size_t>(i)];
}

// NoiseGenerator.cs:49
std::string NoiseGenerator::GetSeed() const {
    std::string text;
    for (int i = 0; i < 8; i++) {
        text += std::to_string(T_[static_cast<size_t>(i)]);
        if (i < 7) text += " ";
    }
    return text;
}

// NoiseGenerator.cs:63 (Vector3 / int and *= float: per-component float operations of UnityEngine.Vector3)
float NoiseGenerator::coherentNoise(float x, float y, float z, int octaves, int multiplier, float amplitude, float lacunarity, float persistence) {
    Vector3f vector = Vector3f(x, y, z) / static_cast<float>(multiplier);
    float num = 0.0f;
    for (int i = 0; i < octaves; i++) {
        num = f32store(fx(num) + fx(noise(vector.x, vector.y, vector.z)) * fx(amplitude));
        vector = vector * lacunarity;
        amplitude = f32store(fx(amplitude) * fx(persistence));
    }
    return num;
}

// NoiseGenerator.cs:76
int NoiseGenerator::getDensity(const Vector3f& loc) {
    float t = coherentNoise(loc.x, loc.y, loc.z);
    return static_cast<int>(Mathf::Lerp(0.0f, 255.0f, t));
}

// NoiseGenerator.cs:82
float NoiseGenerator::noise(float x, float y, float z) {
    s_ = f32store((fx(x) + fx(y) + fx(z)) * fx(onethird_));
    i_ = fastfloor(f32store(fx(x) + fx(s_)));
    j_ = fastfloor(f32store(fx(y) + fx(s_)));
    k_ = fastfloor(f32store(fx(z) + fx(s_)));
    s_ = f32store(fx(static_cast<float>(i_ + j_ + k_)) * fx(onesixth_));
    u_ = f32store(fx(x) - fx(static_cast<float>(i_)) + fx(s_));
    v_ = f32store(fx(y) - fx(static_cast<float>(j_)) + fx(s_));
    w_ = f32store(fx(z) - fx(static_cast<float>(k_)) + fx(s_));
    A_[0] = 0;
    A_[1] = 0;
    A_[2] = 0;
    int num = !(u_ >= w_) ? ((v_ >= w_) ? 1 : 2) : (!(u_ >= v_) ? 1 : 0);
    int num2 = !(u_ < w_) ? ((v_ < w_) ? 1 : 2) : (!(u_ < v_) ? 1 : 0);
    // kay() is called left to right; each call mutates the shared A / s fields
    const double a = fx(kay(num));
    const double b2 = fx(kay(3 - num - num2));
    const double c = fx(kay(num2));
    const double d = fx(kay(0));
    return f32store(a + b2 + c + d);
}

// NoiseGenerator.cs:100
float NoiseGenerator::kay(int a) {
    s_ = f32store(fx(static_cast<float>(A_[0] + A_[1] + A_[2])) * fx(onesixth_));
    float num = f32store(fx(u_) - fx(static_cast<float>(A_[0])) + fx(s_));
    float num2 = f32store(fx(v_) - fx(static_cast<float>(A_[1])) + fx(s_));
    float num3 = f32store(fx(w_) - fx(static_cast<float>(A_[2])) + fx(s_));
    float num4 = f32store(fx(0.6f) - fx(num) * fx(num) - fx(num2) * fx(num2) - fx(num3) * fx(num3));
    int num5 = shuffle(i_ + A_[0], j_ + A_[1], k_ + A_[2]);
    A_[static_cast<size_t>(a)]++;
    if (num4 < 0.0f) return 0.0f;
    int num6 = (num5 >> 5) & 1;
    int num7 = (num5 >> 4) & 1;
    int num8 = (num5 >> 3) & 1;
    int num9 = (num5 >> 2) & 1;
    int num10 = num5 & 3;
    float num11;
    switch (num10) {
        default: num11 = num3; break;
        case 2: num11 = num2; break;
        case 1: num11 = num; break;
    }
    float num12 = num11;
    float num13;
    switch (num10) {
        default: num13 = num; break;
        case 2: num13 = num3; break;
        case 1: num13 = num2; break;
    }
    float num14 = num13;
    float num15;
    switch (num10) {
        default: num15 = num2; break;
        case 2: num15 = num; break;
        case 1: num15 = num3; break;
    }
    float num16 = num15;
    num12 = (num6 == num8) ? (0.0f - num12) : num12;
    num14 = (num6 == num7) ? (0.0f - num14) : num14;
    num16 = (num6 != (num7 ^ num8)) ? (0.0f - num16) : num16;
    num4 = f32store(fx(num4) * fx(num4));
    return f32store(8.0 * fx(num4) * fx(num4) * (fx(num12) + ((num10 == 0) ? (fx(num14) + fx(num16)) : ((num9 == 0) ? fx(num14) : fx(num16)))));
}

// NoiseGenerator.cs:167
int NoiseGenerator::shuffle(int i, int j, int k) const {
    return b(i, j, k, 0) + b(j, k, i, 1) + b(k, i, j, 2) + b(i, j, k, 3) + b(j, k, i, 4) + b(k, i, j, 5) + b(i, j, k, 6) + b(j, k, i, 7);
}

// NoiseGenerator.cs:172
int NoiseGenerator::b(int i, int j, int k, int B) const { return T_[static_cast<size_t>((b(i, B) << 2) | (b(j, B) << 1) | b(k, B))]; }

// NoiseGenerator.cs:177
int NoiseGenerator::b(int N, int B) { return (N >> B) & 1; }

// NoiseGenerator.cs:182 (sic: -2.0 gives -3, n <= 0 truncates then subtracts one)
int NoiseGenerator::fastfloor(float n) {
    if (!(n > 0.0f)) return dotnet::ConvToInt32(static_cast<double>(n)) - 1;
    return dotnet::ConvToInt32(static_cast<double>(n));
}

// ---- ResourceUtilities.cs -----------------------------------------------------------------------------------------------------

// ResourceUtilities.cs:15
const CBAttributeMapSO::MapAttribute* ResourceUtilities::GetBiome(double lat, double lon, CelestialBody& body) {
    try {
        if (!ResourceEnvironment::getBiomeAtt) return nullptr;  // (the decoder is not ported: treated as the exception path)
        return ResourceEnvironment::getBiomeAtt(lat, lon, body);
    } catch (const std::exception&) {
        return nullptr;
    }
}

// ResourceUtilities.cs:44
std::vector<std::shared_ptr<ResourceData>> ResourceUtilities::ImportConfigNodeList(const std::vector<ConfigNode*>& nodes) {
    std::vector<std::shared_ptr<ResourceData>> list;
    for (ConfigNode* node : nodes) {
        auto resourceData = std::make_shared<ResourceData>();
        for (const ConfigNode::Value& v : node->values()) {  // LoadNodeProperties<ResourceData>
            if (v.name == "ResourceName") resourceData->ResourceName = v.value;
            else if (v.name == "PlanetName") resourceData->PlanetName = v.value;
            else if (v.name == "BiomeName") resourceData->BiomeName = v.value;
            else if (v.name == "ResourceType") resourceData->ResourceType = ToInt(v.value);
        }
        ConfigNode* dn = node->GetNode("Distribution");
        if (dn != nullptr) {
            auto distribution = std::make_shared<DistributionData>();
            for (const ConfigNode::Value& v : dn->values()) {  // LoadNodeProperties<DistributionData>
                if (v.name == "PresenceChance") distribution->PresenceChance = ToFloat(v.value);
                else if (v.name == "MinAbundance") distribution->MinAbundance = ToFloat(v.value);
                else if (v.name == "MaxAbundance") distribution->MaxAbundance = ToFloat(v.value);
                else if (v.name == "Variance") distribution->Variance = ToFloat(v.value);
                else if (v.name == "Dispersal") distribution->Dispersal = ToFloat(v.value);
                else if (v.name == "MinAltitude") distribution->MinAltitude = ToFloat(v.value);
                else if (v.name == "MaxAltitude") distribution->MaxAltitude = ToFloat(v.value);
                else if (v.name == "MinRange") distribution->MinRange = ToFloat(v.value);
                else if (v.name == "MaxRange") distribution->MaxRange = ToFloat(v.value);
            }
            resourceData->Distribution = distribution;
        }
        list.push_back(resourceData);
    }
    return list;
}

// ResourceUtilities.cs:62
std::vector<std::shared_ptr<DepletionData>> ResourceUtilities::ImportDepletionNodeList(const std::vector<ConfigNode*>& nodes) {
    std::vector<std::shared_ptr<DepletionData>> list;
    for (ConfigNode* node : nodes) {
        auto depletionData = std::make_shared<DepletionData>();
        for (const ConfigNode::Value& v : node->values()) {
            if (v.name == "PlanetId") depletionData->PlanetId = ToInt(v.value);
            else if (v.name == "ResourceName") depletionData->ResourceName = v.value;
        }
        for (ConfigNode* n2 : node->GetNodes("DEPLETION_NODE")) {
            auto item = std::make_shared<DepletionNode>();
            for (const ConfigNode::Value& v : n2->values()) {
                if (v.name == "X") item->X = ToInt(v.value);
                else if (v.name == "Y") item->Y = ToInt(v.value);
                else if (v.name == "Value") item->Value = ToFloat(v.value);
                else if (v.name == "LastUpdate") item->LastUpdate = ToDouble(v.value);
            }
            depletionData->DepletionNodes.push_back(item);
        }
        list.push_back(depletionData);
    }
    return list;
}

// ResourceUtilities.cs:84
std::vector<std::shared_ptr<BiomeLockData>> ResourceUtilities::ImportBiomeLockNodeList(const std::vector<ConfigNode*>& nodes) {
    std::vector<std::shared_ptr<BiomeLockData>> list;
    for (ConfigNode* node : nodes) {
        auto item = std::make_shared<BiomeLockData>();
        for (const ConfigNode::Value& v : node->values()) {
            if (v.name == "PlanetId") item->PlanetId = ToInt(v.value);
            else if (v.name == "BiomeName") item->BiomeName = v.value;
        }
        list.push_back(item);
    }
    return list;
}

// ResourceUtilities.cs:96
std::vector<std::shared_ptr<PlanetScanData>> ResourceUtilities::ImportPlanetScanNodeList(const std::vector<ConfigNode*>& nodes) {
    std::vector<std::shared_ptr<PlanetScanData>> list;
    for (ConfigNode* node : nodes) {
        auto item = std::make_shared<PlanetScanData>();
        for (const ConfigNode::Value& v : node->values())
            if (v.name == "PlanetId") item->PlanetId = ToInt(v.value);
        list.push_back(item);
    }
    return list;
}

// ResourceUtilities.cs:108
double ResourceUtilities::GetValue(const ConfigNode& node, const std::string& name, double curVal) {
    double result;
    if (node.HasValue(name) && dotnet::TryParseDouble(*node.GetValue(name), result)) return result;
    return curVal;
}

// ResourceUtilities.cs:117
double ResourceUtilities::GetMaxDeltaTime() {
    if (ResourceScenario::Instance() == nullptr) return static_cast<double>(ResourceGameSettings::defaultMaxDeltaTime);
    return static_cast<double>(ResourceScenario::Instance()->gameSettings()->MaxDeltaTime);
}

// ResourceUtilities.cs:142
double ResourceUtilities::Deg2Rad(double degrees) { return degrees * 3.141592653589793 / 180.0; }

// ResourceUtilities.cs:147 (C# % on doubles is fmod: result has the sign of the dividend)
double ResourceUtilities::Rad2Lat(double radians) {
    constexpr double PI = 3.141592653589793;
    double num = Rem(radians, PI * 2.0);
    if (num < 0.0) num = PI * 2.0 + num;
    double num2 = Rem(num, PI);
    if (num2 > PI / 2.0) num2 = PI - num2;
    num = !(num > PI) ? num2 : (0.0 - num2);
    return num / PI * 180.0;
}

// ResourceUtilities.cs:163
double ResourceUtilities::Rad2Lon(double radians) {
    constexpr double PI = 3.141592653589793;
    double num = Rem(radians, PI * 2.0);
    if (num < 0.0) num = PI * 2.0 + num;
    double num2 = Rem(num, PI * 2.0);
    if (num2 > PI) num2 = PI * 2.0 - num2;
    num = !(num > PI) ? num2 : (0.0 - num2);
    return num / PI * 180.0;
}

// ResourceUtilities.cs:179
Color ResourceUtilities::HSL2RGB(double h, double sl, double l, float alpha) {
    double num = l;
    double num2 = l;
    double num3 = l;
    double num4 = !(l <= 0.5) ? (l + sl - l * sl) : (l * (1.0 + sl));
    if (num4 > 0.0) {
        double num5 = l + l - num4;
        double num6 = (num4 - num5) / num4;
        h *= 6.0;
        int num7 = static_cast<int>(h);
        double num8 = h - static_cast<double>(num7);
        double num9 = num4 * num6 * num8;
        double num10 = num5 + num9;
        double num11 = num4 - num9;
        switch (num7) {
            case 0: num = num4; num2 = num10; num3 = num5; break;
            case 1: num = num11; num2 = num4; num3 = num5; break;
            case 2: num = num5; num2 = num4; num3 = num10; break;
            case 3: num = num5; num2 = num11; num3 = num4; break;
            case 4: num = num10; num2 = num5; num3 = num4; break;
            case 5: num = num4; num2 = num5; num3 = num11; break;
            default: break;
        }
    }
    return Color(static_cast<float>(num), static_cast<float>(num2), static_cast<float>(num3), alpha);
}

// ResourceUtilities.cs:232
double ResourceUtilities::clampLat(double lat) { return Rem(lat + 180.0 + 90.0, 180.0) - 90.0; }
// ResourceUtilities.cs:237
double ResourceUtilities::clampLon(double lon) { return Rem(lon + 360.0 + 180.0, 360.0) - 180.0; }

// ResourceUtilities.cs:242 (the MissionEditorLogic branch is not ported)
float ResourceUtilities::GetDifficultyLevel() {
    if (ResourceEnvironment::difficultyResourceAbundance) return ResourceEnvironment::difficultyResourceAbundance();
    Game* g = SaveEnvironment::Current().currentGame;
    if (g == nullptr || !g->Parameters) throw Nre();
    return g->Parameters->Difficulty.ResourceAbundance;
}

// ---- ResourceGameSettings.cs -----------------------------------------------------------------------------------------------

// ResourceGameSettings.cs:23
void ResourceGameSettings::Load(ConfigNode& node) {
    if (node.HasNode("RESOURCE_SETTINGS")) {
        SettingsNode = node.GetNode("RESOURCE_SETTINGS");
        Seed = GetValue(*SettingsNode, "GameSeed", Seed);
        ROCMissionSeed = GetValue(*SettingsNode, "ROCMissionSeed", ROCMissionSeed);
        MaxDeltaTime = GetValue(*SettingsNode, "MaxDeltaTime", MaxDeltaTime);
        depletionInfo_ = SetupDepletionInfo();
        depletionSet_ = true;
        biomeLockInfo_ = SetupBiomeLockInfo();
        biomeLockSet_ = true;
        planetScanInfo_ = SetupPlanetScanInfo();
        planetScanSet_ = true;
        if (ResourceMap::Instance() != nullptr) ResourceMap::Instance()->ResetCache();
    } else {
        int resourceSeed = 0, rocSeed = 0;
        if (missionSeeds && missionSeeds(resourceSeed, rocSeed)) {
            Seed = resourceSeed;
            ROCMissionSeed = rocSeed;
        } else {
            GenerateNewSeed();
            GenerateNewROCMissionSeed();
        }
        MaxDeltaTime = defaultMaxDeltaTime;
        depletionInfo_.clear();
        depletionSet_ = true;
        biomeLockInfo_.clear();
        biomeLockSet_ = true;
        planetScanInfo_.clear();
        planetScanSet_ = true;
    }
}

// ResourceGameSettings.cs:58
void ResourceGameSettings::GenerateNewSeed() {
    KSPRandom r(ResourceEnvironment::TickCount());
    Seed = r.Next(1, INT32_MAX);
}

// ResourceGameSettings.cs:64
void ResourceGameSettings::GenerateNewROCMissionSeed() {
    KSPRandom r(ResourceEnvironment::TickCount());
    ROCMissionSeed = r.Next(1, INT32_MAX);
}

// ResourceGameSettings.cs:70
std::vector<std::shared_ptr<DepletionData>> ResourceGameSettings::SetupDepletionInfo() {
    log::Info("Loading Depletion Nodes");
    std::vector<ConfigNode*> nodes = SettingsNode->GetNodes("DEPLETION_DATA");
    log::Info("DepNodeCount:  " + std::to_string(nodes.size()));
    return ResourceUtilities::ImportDepletionNodeList(nodes);
}

// ResourceGameSettings.cs:78
std::vector<std::shared_ptr<BiomeLockData>> ResourceGameSettings::SetupBiomeLockInfo() {
    log::Info("Loading Biome Nodes");
    std::vector<ConfigNode*> nodes = SettingsNode->GetNodes("BIOME_LOCK_DATA");
    log::Info("BiomeNodeCount:  " + std::to_string(nodes.size()));
    return ResourceUtilities::ImportBiomeLockNodeList(nodes);
}

// ResourceGameSettings.cs:86
std::vector<std::shared_ptr<PlanetScanData>> ResourceGameSettings::SetupPlanetScanInfo() {
    log::Info("Loading Planet Nodes");
    std::vector<ConfigNode*> nodes = SettingsNode->GetNodes("PLANET_SCAN_DATA");
    log::Info("PlanetNodeCount:  " + std::to_string(nodes.size()));
    return ResourceUtilities::ImportPlanetScanNodeList(nodes);
}

// ResourceGameSettings.cs:94 (`_DepletionInfo ?? (_DepletionInfo = new List)`)
std::vector<std::shared_ptr<DepletionData>>& ResourceGameSettings::GetDepletionInfo() {
    depletionSet_ = true;
    return depletionInfo_;
}
// ResourceGameSettings.cs:99
std::vector<std::shared_ptr<BiomeLockData>>& ResourceGameSettings::GetBiomeLockInfo() {
    biomeLockSet_ = true;
    return biomeLockInfo_;
}
// ResourceGameSettings.cs:104
std::vector<std::shared_ptr<PlanetScanData>>& ResourceGameSettings::GetPlanetScanInfo() {
    planetScanSet_ = true;
    return planetScanInfo_;
}

// ResourceGameSettings.cs:109
void ResourceGameSettings::Save(ConfigNode& node) {
    if (node.HasNode("RESOURCE_SETTINGS")) SettingsNode = node.GetNode("RESOURCE_SETTINGS");
    else SettingsNode = node.AddNode("RESOURCE_SETTINGS");
    SettingsNode->AddValue("GameSeed", static_cast<int32_t>(Seed));
    SettingsNode->AddValue("ROCMissionSeed", static_cast<int32_t>(ROCMissionSeed));
    SettingsNode->AddValue("MaxDeltaTime", static_cast<int32_t>(MaxDeltaTime));
    for (const auto& depletionData : depletionInfo_) {
        auto configNode = ConfigNode::Create("DEPLETION_DATA");
        configNode->AddValue("PlanetId", static_cast<int32_t>(depletionData->PlanetId));
        configNode->AddValue("ResourceName", depletionData->ResourceName);
        for (const auto& depletionNode : depletionData->DepletionNodes) {
            auto configNode2 = ConfigNode::Create("DEPLETION_NODE");
            configNode2->AddValue("X", static_cast<int32_t>(depletionNode->X));
            configNode2->AddValue("Y", static_cast<int32_t>(depletionNode->Y));
            configNode2->AddValue("Value", depletionNode->Value);
            configNode2->AddValue("LastUpdate", depletionNode->LastUpdate);
            configNode->AddNode(std::move(configNode2));
        }
        SettingsNode->AddNode(std::move(configNode));
    }
    for (const auto& biomeLockData : biomeLockInfo_) {
        auto configNode3 = ConfigNode::Create("BIOME_LOCK_DATA");
        configNode3->AddValue("PlanetId", static_cast<int32_t>(biomeLockData->PlanetId));
        configNode3->AddValue("BiomeName", biomeLockData->BiomeName);
        SettingsNode->AddNode(std::move(configNode3));
    }
    for (const auto& planetScanData : planetScanInfo_) {
        auto configNode4 = ConfigNode::Create("PLANET_SCAN_DATA");
        configNode4->AddValue("PlanetId", static_cast<int32_t>(planetScanData->PlanetId));
        SettingsNode->AddNode(std::move(configNode4));
    }
    if (ResourceMap::Instance() != nullptr) ResourceMap::Instance()->ResetCache();
}

// ResourceGameSettings.cs:165 (int.TryParse of the culture-sensitive Integer style)
int ResourceGameSettings::GetValue(const ConfigNode& config, const std::string& name, int currentValue) {
    int32_t result = 0;
    int64_t r64;
    if (config.HasValue(name) && dotnet::TryParseInt64(*config.GetValue(name), r64) && r64 >= INT32_MIN && r64 <= INT32_MAX) {
        result = static_cast<int32_t>(r64);
        return result;
    }
    return currentValue;
}

// ResourceGameSettings.cs:174
float ResourceGameSettings::GetValue(const ConfigNode& config, const std::string& name, float currentValue) {
    float result;
    if (config.HasValue(name) && dotnet::TryParseSingle(*config.GetValue(name), result)) return result;
    return currentValue;
}

// ResourceGameSettings.cs:183
void ResourceGameSettings::SaveDepletionNodeValue(int planetId, const std::string& resource, int x, int y, float value) {
    std::shared_ptr<DepletionData> depletionData;
    for (const auto& d : depletionInfo_)
        if (d->PlanetId == planetId && d->ResourceName == resource) {
            depletionData = d;
            break;
        }
    if (!depletionData) {
        depletionData = std::make_shared<DepletionData>();
        depletionData->PlanetId = planetId;
        depletionData->ResourceName = resource;
        depletionInfo_.push_back(depletionData);
    }
    std::shared_ptr<DepletionNode> depletionNode = CheckDepletionNode(planetId, resource, x, y);
    if (!depletionNode) {
        depletionNode = std::make_shared<DepletionNode>();
        depletionNode->X = x;
        depletionNode->Y = y;
        depletionData->DepletionNodes.push_back(depletionNode);
    }
    depletionNode->Value = value;
    depletionNode->LastUpdate = ResourceEnvironment::universalTime ? ResourceEnvironment::universalTime() : Planetarium::GetUniversalTime();
}

// ResourceGameSettings.cs:215
void ResourceGameSettings::SaveBiomeUnlockNode(int planetId, const std::string& biomeName) {
    for (const auto& b : biomeLockInfo_)
        if (b->PlanetId == planetId && b->BiomeName == biomeName) return;
    auto item = std::make_shared<BiomeLockData>();
    item->PlanetId = planetId;
    item->BiomeName = biomeName;
    biomeLockInfo_.push_back(item);
}

// ResourceGameSettings.cs:241
void ResourceGameSettings::SavePlanetUnlockNode(int planetId) {
    for (const auto& p : planetScanInfo_)
        if (p->PlanetId == planetId) return;
    auto item = std::make_shared<PlanetScanData>();
    item->PlanetId = planetId;
    planetScanInfo_.push_back(item);
}

// ResourceGameSettings.cs:258
std::shared_ptr<DepletionNode> ResourceGameSettings::CheckDepletionNode(int planetId, const std::string& resource, int x, int y) {
    std::shared_ptr<DepletionData> depletionData;
    for (const auto& d : depletionInfo_)
        if (d->PlanetId == planetId && d->ResourceName == resource) {
            depletionData = d;
            break;
        }
    if (!depletionData) return nullptr;
    for (const auto& n : depletionData->DepletionNodes)
        if (n->X == x && n->Y == y) return n;
    return nullptr;
}

// ---- ResourceCache.cs ------------------------------------------------------------------------------------------------------

// ResourceCache.cs:29
ResourceCache& ResourceCache::Instance() {
    if (!instance_) instance_.reset(new ResourceCache());
    return *instance_;
}
void ResourceCache::DestroyInstance() { instance_.reset(); }

// ResourceCache.cs:31
std::vector<std::shared_ptr<ResourceCache::AbundanceSummary>>& ResourceCache::AbundanceCache() {
    if (!abundanceLoaded_) {
        abundanceCache_ = LoadAbundanceCache();
        abundanceLoaded_ = true;
    }
    return abundanceCache_;
}
// ResourceCache.cs:33
std::vector<std::shared_ptr<ResourceData>>& ResourceCache::BiomeResources() {
    if (!biomeLoaded_) {
        biomeResources_ = LoadResourceInfo("BIOME_RESOURCE");
        biomeLoaded_ = true;
    }
    return biomeResources_;
}
// ResourceCache.cs:35
std::vector<std::shared_ptr<ResourceData>>& ResourceCache::GlobalResources() {
    if (!globalLoaded_) {
        globalResources_ = LoadResourceInfo("GLOBAL_RESOURCE");
        globalLoaded_ = true;
    }
    return globalResources_;
}
// ResourceCache.cs:37
std::vector<std::shared_ptr<ResourceData>>& ResourceCache::PlanetaryResources() {
    if (!planetaryLoaded_) {
        planetaryResources_ = LoadResourceInfo("PLANETARY_RESOURCE");
        planetaryLoaded_ = true;
    }
    return planetaryResources_;
}

// ResourceCache.cs:47
void ResourceCache::ResetCache() {
    abundanceCache_.clear();
    abundanceLoaded_ = false;
    globalResources_.clear();
    globalLoaded_ = false;
    planetaryResources_.clear();
    planetaryLoaded_ = false;
    biomeResources_.clear();
    biomeLoaded_ = false;
}

// ResourceCache.cs:55
std::shared_ptr<ResourceCache::AbundanceSummary> ResourceCache::RequestSummary(CelestialBody& body, const std::string& biome,
                                                                               const std::string& resourceName, HarvestTypes type) {
    auto abundanceSummary = std::make_shared<AbundanceSummary>();
    AbundanceRequest request;
    request.BodyId = body.flightGlobalsIndex;
    request.CheckForLock = false;
    request.ExcludeVariance = true;
    request.ResourceName = resourceName;
    request.BiomeName = biome;
    request.ResourceType = type;
    if (ResourceMap::Instance() == nullptr) throw Nre();
    abundanceSummary->Abundance = ResourceMap::Instance()->GetAbundance(request);
    abundanceSummary->BodyId = body.flightGlobalsIndex;
    abundanceSummary->BiomeName = biome;
    abundanceSummary->ResourceName = resourceName;
    abundanceSummary->HarvestType = type;
    return abundanceSummary;
}

// ResourceCache.cs:75
std::vector<std::shared_ptr<ResourceCache::AbundanceSummary>> ResourceCache::LoadAbundanceCache() {
    std::vector<std::shared_ptr<AbundanceSummary>> list;
    if (!ResourceEnvironment::bodies) throw Nre();
    const std::vector<CelestialBody*>& bodies = ResourceEnvironment::bodies();
    int count = static_cast<int>(bodies.size());
    for (int i = 0; i < count; i++) {
        CelestialBody* celestialBody = bodies[static_cast<size_t>(i)];
        std::vector<std::string> list2 = ResourceMap::Instance()->FetchAllResourceNames(HarvestTypes::Exospheric);
        std::string defaultSituation = ResourceMap::GetDefaultSituation(HarvestTypes::Exospheric);
        for (const std::string& n : list2) list.push_back(RequestSummary(*celestialBody, defaultSituation, n, HarvestTypes::Exospheric));
        std::vector<std::string> list3 = ResourceMap::Instance()->FetchAllResourceNames(HarvestTypes::Atmospheric);
        std::string defaultSituation2 = ResourceMap::GetDefaultSituation(HarvestTypes::Atmospheric);
        for (const std::string& n : list3) list.push_back(RequestSummary(*celestialBody, defaultSituation2, n, HarvestTypes::Atmospheric));
        std::vector<std::string> list4 = ResourceMap::Instance()->FetchAllResourceNames(HarvestTypes::Oceanic);
        std::string defaultSituation3 = ResourceMap::GetDefaultSituation(HarvestTypes::Oceanic);
        for (const std::string& n : list4) list.push_back(RequestSummary(*celestialBody, defaultSituation3, n, HarvestTypes::Oceanic));
        std::vector<std::string> list5 = ResourceMap::Instance()->FetchAllResourceNames(HarvestTypes::Planetary);
        if (!celestialBody->BiomeMap.has_value()) continue;
        for (const auto& attr : celestialBody->BiomeMap->Attributes)
            for (const std::string& n : list5) list.push_back(RequestSummary(*celestialBody, attr.name, n, HarvestTypes::Planetary));
    }
    return list;
}

// ResourceCache.cs:122
std::vector<std::shared_ptr<ResourceData>> ResourceCache::LoadResourceInfo(const std::string& node) {
    if (!getConfigNodes) throw Nre();
    return ResourceUtilities::ImportConfigNodeList(getConfigNodes(node));
}

// ---- ResourceMap.cs --------------------------------------------------------------------------------------------------------

// ResourceMap.cs:77 Awake
ResourceMap::ResourceMap() {
    if (instance_ != nullptr && instance_ != this) instance_ = nullptr;  // Object.Destroy(Instance): the old one stops being the instance
    instance_ = this;
    events::GameEvents::OnResourceMapLoaded.Fire();
}

// ResourceMap.cs:87
ResourceMap::~ResourceMap() {
    if (instance_ == this) instance_ = nullptr;
}

// ResourceMap.cs:29
std::vector<std::shared_ptr<DepletionData>>& ResourceMap::DepletionInfo() {
    depletionSet_ = true;
    if (ResourceScenario::Instance() != nullptr && depletionInfo_.size() <= 0) {
        auto& src = ResourceScenario::Instance()->gameSettings()->GetDepletionInfo();
        depletionInfo_.insert(depletionInfo_.end(), src.begin(), src.end());
    }
    return depletionInfo_;
}
// ResourceMap.cs:45
std::vector<std::shared_ptr<BiomeLockData>>& ResourceMap::BiomeLockInfo() {
    biomeLockSet_ = true;
    if (ResourceScenario::Instance() != nullptr && biomeLockInfo_.size() <= 0) {
        auto& src = ResourceScenario::Instance()->gameSettings()->GetBiomeLockInfo();
        biomeLockInfo_.insert(biomeLockInfo_.end(), src.begin(), src.end());
    }
    return biomeLockInfo_;
}
// ResourceMap.cs:61
std::vector<std::shared_ptr<PlanetScanData>>& ResourceMap::PlanetScanInfo() {
    planetScanSet_ = true;
    if (ResourceScenario::Instance() != nullptr && planetScanInfo_.size() <= 0) {
        auto& src = ResourceScenario::Instance()->gameSettings()->GetPlanetScanInfo();
        planetScanInfo_.insert(planetScanInfo_.end(), src.begin(), src.end());
    }
    return planetScanInfo_;
}

// ResourceMap.cs:95
void ResourceMap::ResetCache() {
    biomeLockInfo_.clear();
    biomeLockSet_ = false;
    depletionInfo_.clear();
    depletionSet_ = false;
    planetScanInfo_.clear();
    planetScanSet_ = false;
    ResourceCache::Instance().ResetCache();
}

// ResourceMap.cs:103
std::shared_ptr<DepletionNode> ResourceMap::GetDepletionNode(int planetId, const std::string& resource, int x, int y) {
    if (DepletionInfo().size() == 0) return nullptr;
    std::shared_ptr<DepletionData> depletionData;
    int count = static_cast<int>(DepletionInfo().size());
    for (int i = 0; i < count; i++) {
        auto& d = DepletionInfo()[static_cast<size_t>(i)];
        if (d->PlanetId == planetId && d->ResourceName == resource) {
            depletionData = d;
            break;
        }
    }
    if (!depletionData) return nullptr;
    std::shared_ptr<DepletionNode> result;
    for (const auto& n : depletionData->DepletionNodes)
        if (n->X == x && n->Y == y) {
            result = n;
            break;
        }
    return result;
}

// ResourceMap.cs:138
float ResourceMap::GetDepletionNodeValue(int planetId, const std::string& resource, int x, int y) {
    auto n = GetDepletionNode(planetId, resource, x, y);
    return n ? n->Value : 1.0f;
}

// ResourceMap.cs:143
bool ResourceMap::IsBiomeUnlocked(int planetId, const std::string& biomeName) {
    for (const auto& b : BiomeLockInfo())
        if (b->PlanetId == planetId && b->BiomeName == biomeName) return true;
    return false;
}

// ResourceMap.cs:164
bool ResourceMap::IsPlanetScanned(int planetId) {
    for (const auto& p : PlanetScanInfo())
        if (p->PlanetId == planetId) return true;
    return false;
}

// ResourceMap.cs:184
void ResourceMap::SetDepletionNodeValue(int planetId, const std::string& resource, int x, int y, float value) {
    std::shared_ptr<DepletionData> depletionData;
    for (const auto& d : DepletionInfo())
        if (d->PlanetId == planetId && d->ResourceName == resource) {
            depletionData = d;
            break;
        }
    if (!depletionData) {
        depletionData = std::make_shared<DepletionData>();
        depletionData->PlanetId = planetId;
        depletionData->ResourceName = resource;
        DepletionInfo().push_back(depletionData);
    }
    std::shared_ptr<DepletionNode> depletionNode = GetDepletionNode(planetId, resource, x, y);
    if (!depletionNode) {
        depletionNode = std::make_shared<DepletionNode>();
        depletionNode->X = x;
        depletionNode->Y = y;
        depletionData->DepletionNodes.push_back(depletionNode);
    }
    depletionNode->Value = value;
    depletionNode->LastUpdate = ResourceEnvironment::universalTime ? ResourceEnvironment::universalTime() : Planetarium::GetUniversalTime();
    if (ResourceScenario::Instance() == nullptr) throw Nre();
    ResourceScenario::Instance()->gameSettings()->SaveDepletionNodeValue(planetId, resource, x, y, value);
}

// ResourceMap.cs:217
void ResourceMap::UnlockBiome(int planetId, const std::string& biomeName) {
    if (!IsBiomeUnlocked(planetId, biomeName)) {
        auto item = std::make_shared<BiomeLockData>();
        item->PlanetId = planetId;
        item->BiomeName = biomeName;
        BiomeLockInfo().push_back(item);
        if (ResourceScenario::Instance() == nullptr) throw Nre();
        ResourceScenario::Instance()->gameSettings()->SaveBiomeUnlockNode(planetId, biomeName);
    }
}

// ResourceMap.cs:231
void ResourceMap::UnlockPlanet(int planetId) {
    if (!IsPlanetScanned(planetId)) {
        auto item = std::make_shared<PlanetScanData>();
        item->PlanetId = planetId;
        PlanetScanInfo().push_back(item);
        if (ResourceScenario::Instance() == nullptr) throw Nre();
        ResourceScenario::Instance()->gameSettings()->SavePlanetUnlockNode(planetId);
    }
}

// ResourceMap.cs:244
std::vector<std::string> ResourceMap::FetchAllResourceNames(HarvestTypes harvest) {
    std::vector<std::string> list;
    auto add = [&](const std::vector<std::string>& v) { list.insert(list.end(), v.begin(), v.end()); };
    add(GetCacheNames(ResourceCache::Instance().PlanetaryResources(), harvest));
    add(GetCacheNames(ResourceCache::Instance().BiomeResources(), harvest));
    add(GetCacheNames(ResourceCache::Instance().GlobalResources(), harvest));
    return DeDuplicate(list);
}

// ResourceMap.cs:253
std::vector<std::string> ResourceMap::DeDuplicate(const std::vector<std::string>& input) {
    std::vector<std::string> list;
    for (const std::string& s : input)
        if (std::find(list.begin(), list.end(), s) == list.end()) list.push_back(s);
    return list;
}

// ResourceMap.cs:267
std::vector<std::string> ResourceMap::GetCacheNames(const std::vector<std::shared_ptr<ResourceData>>& resList, HarvestTypes harvest) {
    std::vector<std::string> list;
    for (const auto& r : resList)
        if (r->ResourceType == static_cast<int>(harvest)) list.push_back(r->ResourceName);
    return list;
}

// ResourceMap.cs:282 (a configs[0] without a Distribution node is a NullReferenceException that the method logs and swallows)
std::shared_ptr<DistributionData> ResourceMap::GetBestResourceData(const std::vector<std::shared_ptr<ResourceData>>& configs) {
    try {
        if (configs.empty() || !configs[0]->Distribution) throw Nre();
        auto d0 = configs[0]->Distribution;
        auto distributionData = std::make_shared<DistributionData>();
        distributionData->Variance = d0->Variance;
        distributionData->Dispersal = d0->Dispersal;
        distributionData->PresenceChance = d0->PresenceChance;
        distributionData->MinAbundance = d0->MinAbundance;
        distributionData->MaxAbundance = d0->MaxAbundance;
        for (const auto& c : configs) {
            if (!c->Distribution) throw Nre();
            const DistributionData& distribution = *c->Distribution;
            if (distribution.PresenceChance > 0.0f && distribution.PresenceChance > distributionData->PresenceChance)
                distributionData->PresenceChance = distribution.PresenceChance;
            if (distribution.MinAbundance > 0.0f && distribution.MinAbundance < distributionData->MinAbundance)
                distributionData->MinAbundance = distribution.MinAbundance;
            if (distribution.MaxAbundance > 0.0f && distribution.MaxAbundance > distributionData->MaxAbundance)
                distributionData->MaxAbundance = distribution.MaxAbundance;
            if (distribution.Variance > 0.0f && distribution.Variance > distributionData->Variance) distributionData->Variance = distribution.Variance;
            if (distribution.Dispersal > 0.0f && distribution.Dispersal > distributionData->Dispersal) distributionData->Dispersal = distribution.Dispersal;
            if (distribution.MinAltitude > 0.0f && distribution.MinAltitude < distributionData->MinAltitude)
                distributionData->MinAltitude = distribution.MinAltitude;
            if (distribution.MaxAltitude > 0.0f && distribution.MaxAltitude > distributionData->MaxAltitude)
                distributionData->MaxAltitude = distribution.MaxAltitude;
            if (distribution.MinRange > 0.0f && distribution.MinRange < distributionData->MinRange) distributionData->MinRange = distribution.MinRange;
            if (distribution.MaxRange > 0.0f && distribution.MaxRange > distributionData->MaxRange) distributionData->MaxRange = distribution.MaxRange;
        }
        return distributionData;
    } catch (const std::exception& ex) {
        log::Error(std::string("[RESOURCES] - Error in - ResourceMap_GetBestResourceData - ") + ex.what());
        return std::make_shared<DistributionData>();
    }
}

namespace {
CelestialBody* FindBody(int bodyId) {
    if (!ResourceEnvironment::bodies) throw Nre();
    for (CelestialBody* b : ResourceEnvironment::bodies())
        if (b->flightGlobalsIndex == bodyId) return b;
    return nullptr;
}
// `s.Length * ((s.Length > 1) ? s[1] : '\0')` over UTF-16 code units (int)
int32_t LenTimesSecond(const std::string& s) {
    const std::u16string u = dotnet::ToUtf16(s);
    const int32_t len = static_cast<int32_t>(u.size());
    return static_cast<int32_t>(static_cast<uint32_t>(len) * static_cast<uint32_t>(u.size() > 1 ? u[1] : 0));
}
}  // namespace

// ResourceMap.cs:344 (int arithmetic wraps; operator precedence: ((((Seed * (BodyId + 1)) * a) * b) + (c * (type + 1))))
int ResourceMap::GenerateAbundanceSeed(const AbundanceRequest& request, const std::string& biomeName) {
    CelestialBody* celestialBody = FindBody(request.BodyId);
    if (celestialBody == nullptr) return 0;
    if (ResourceScenario::Instance() == nullptr) throw Nre();
    uint32_t seed = static_cast<uint32_t>(ResourceScenario::Instance()->gameSettings()->Seed);
    uint32_t r = seed * static_cast<uint32_t>(request.BodyId + 1);
    r *= static_cast<uint32_t>(LenTimesSecond(request.ResourceName));
    r *= static_cast<uint32_t>(LenTimesSecond(celestialBody->bodyName));
    uint32_t tail = static_cast<uint32_t>(LenTimesSecond(biomeName)) * static_cast<uint32_t>(static_cast<int>(request.ResourceType) + 1);
    return static_cast<int>(r + tail);
}

// ResourceMap.cs:363
std::string ResourceMap::DetermineBiomeName(const AbundanceRequest& request) {
    double lat = ResourceUtilities::Deg2Rad(ResourceUtilities::clampLat(request.Latitude));
    double lon = ResourceUtilities::Deg2Rad(ResourceUtilities::clampLon(request.Longitude));
    CelestialBody* body = FindBody(request.BodyId);
    if (!request.BiomeName.empty()) return request.BiomeName;
    if (body == nullptr) throw Nre();
    const CBAttributeMapSO::MapAttribute* biome = ResourceUtilities::GetBiome(lat, lon, *body);
    if (biome != nullptr) return biome->name;
    return GetDefaultSituation(request.ResourceType);
}

// ResourceMap.cs:389
float ResourceMap::GetAbundanceFromCache(const AbundanceRequest& request, const std::string& biomeName) {
    std::shared_ptr<ResourceCache::AbundanceSummary> found;
    for (const auto& a : ResourceCache::Instance().AbundanceCache())
        if (a->BodyId == request.BodyId && a->ResourceName == request.ResourceName && a->HarvestType == request.ResourceType && a->BiomeName == biomeName) {
            found = a;
            break;
        }
    return found ? found->Abundance : 2.0f;
}

// ResourceMap.cs:405 (managed float code: Mono rule)
float ResourceMap::GetAbundance(const AbundanceRequest& request) {
    try {
        double num = ResourceUtilities::Deg2Rad(ResourceUtilities::clampLat(request.Latitude));
        double num2 = ResourceUtilities::Deg2Rad(ResourceUtilities::clampLon(request.Longitude));
        CelestialBody* celestialBody = FindBody(request.BodyId);
        std::string text = GetDefaultSituation(request.ResourceType);
        bool flag = true;
        if (request.ResourceType == HarvestTypes::Planetary) {
            text = DetermineBiomeName(request);
            flag = Instance()->IsBiomeUnlocked(request.BodyId, text);
            if (!request.CheckForLock) flag = true;
            if (request.ExcludeVariance) flag = false;
        }
        int num3 = GenerateAbundanceSeed(request, text);
        if (celestialBody == nullptr) throw Nre();
        biomeConfigs.clear();
        for (const auto& rd : ResourceCache::Instance().BiomeResources())
            if (rd->PlanetName == celestialBody->bodyName && rd->BiomeName == text && rd->ResourceName == request.ResourceName &&
                rd->ResourceType == static_cast<int>(request.ResourceType))
                biomeConfigs.push_back(rd);
        planetConfigs.clear();
        for (const auto& rd : ResourceCache::Instance().PlanetaryResources())
            if (rd->PlanetName == celestialBody->bodyName && rd->ResourceName == request.ResourceName && rd->ResourceType == static_cast<int>(request.ResourceType))
                planetConfigs.push_back(rd);
        globalConfigs.clear();
        for (const auto& rd : ResourceCache::Instance().GlobalResources())
            if (rd->ResourceName == request.ResourceName && rd->ResourceType == static_cast<int>(request.ResourceType)) globalConfigs.push_back(rd);
        std::shared_ptr<DistributionData> bestResourceData;
        if (biomeConfigs.size() > 0) {
            bestResourceData = GetBestResourceData(biomeConfigs);
            num3 = static_cast<int>(static_cast<uint32_t>(num3) * 2u);
        } else if (planetConfigs.size() > 0) {
            bestResourceData = GetBestResourceData(planetConfigs);
            num3 = static_cast<int>(static_cast<uint32_t>(num3) * 3u);
        } else {
            if (globalConfigs.size() <= 0) return 0.0f;
            bestResourceData = GetBestResourceData(globalConfigs);
            num3 = static_cast<int>(static_cast<uint32_t>(num3) * 4u);
        }
        auto it = seedsCache.find(num3);
        if (it == seedsCache.end()) {
            KSPRandom kSPRandom(num3);
            std::vector<double> value;
            value.push_back(kSPRandom.NextDouble());
            value.push_back(kSPRandom.NextDouble());
            for (int n = 0; n < 8; n++) value.push_back(static_cast<double>(kSPRandom.Next(100)));
            value.push_back(kSPRandom.NextDouble());
            value.push_back(kSPRandom.NextDouble());
            it = seedsCache.emplace(num3, std::move(value)).first;
        }
        const std::vector<double>& value = it->second;
        const float difficulty = ResourceUtilities::GetDifficultyLevel();
        if (!(fx(static_cast<float>(dotnet::ConvToInt32(value[0] * 100.0))) <= fx(bestResourceData->PresenceChance) * fx(difficulty))) return 0.0f;
        int num4 = dotnet::ConvToInt32(fx(bestResourceData->MinAbundance) * 1000.0);
        int num5 = dotnet::ConvToInt32(fx(bestResourceData->MaxAbundance) * 1000.0);
        if (num4 > num5) num5 = num4 + 1;
        int num6 = num5 - num4;
        float num7 = f32store(fx(static_cast<float>(dotnet::ConvToInt32(value[1] * static_cast<double>(num6)) + num4)) / 1000.0 * fx(difficulty));
        if (flag && !request.ExcludeVariance && request.ResourceType == HarvestTypes::Planetary) {
            for (int m = 0; m < 8; m++) noiseSeed[static_cast<size_t>(m)] = dotnet::ConvToInt32(value[static_cast<size_t>(2 + m)]);
            spx.SetSeed(noiseSeed);
            float x = f32store(fx(static_cast<float>(num)) * fx(bestResourceData->Dispersal));
            float y = f32store(fx(static_cast<float>(num2)) * fx(bestResourceData->Dispersal));
            float z = f32store(fx(static_cast<float>(value[10] * 100.0)) / 100.0);
            float num8 = spx.noise(x, y, z);
            if (request.ResourceType != HarvestTypes::Exospheric) {
                float num9 = f32store(fx(num8) * (fx(bestResourceData->Variance) / 100.0));
                num7 = f32store(fx(num7) + fx(num9));
                if (num7 < 0.0f) num7 = 0.0f;
            }
        } else if (request.ResourceType == HarvestTypes::Planetary) {
            num7 = f32store(fx(num7) / 1.5);
        }
        if ((request.ResourceType == HarvestTypes::Atmospheric || request.ResourceType == HarvestTypes::Exospheric) && bestResourceData->HasVariableAltitude()) {
            double radius = celestialBody->Radius;
            double num10 = (radius * static_cast<double>(bestResourceData->MinAltitude) + radius * static_cast<double>(bestResourceData->MaxAltitude)) / 2.0;
            num6 = dotnet::ConvToInt32(radius * static_cast<double>(bestResourceData->MaxRange)) - dotnet::ConvToInt32(radius * static_cast<double>(bestResourceData->MinRange));
            int num11 = dotnet::ConvToInt32(value[11] * static_cast<double>(num6)) + dotnet::ConvToInt32(radius * static_cast<double>(bestResourceData->MinRange));
            double num12 = dotnet::Math::Abs(num10 - request.Altitude) / static_cast<double>(num11);
            double num13 = 1.0 - num12;
            num7 = f32store(fx(num7) * fx(static_cast<float>(num13)));
        }
        if (static_cast<double>(num7) <= 1E-09) return 0.0f;
        return f32store(fx(num7) / 100.0);
    } catch (const std::exception& ex) {
        log::Error(std::string("[RESOURCES] - Error in - ResourceMap_GetAbundance - ") + ex.what());
        return 0.0f;
    }
}

// ResourceMap.cs:565
std::string ResourceMap::GetDefaultSituation(HarvestTypes type) {
    switch (type) {
        case HarvestTypes::Planetary: return "LANDED";
        case HarvestTypes::Oceanic: return "SPLASHED";
        case HarvestTypes::Atmospheric: return "FLYING";
        default: return "ORBITING";
    }
}

// ResourceMap.cs:580 (Math.Round(x, 0): banker's rounding)
Vector2f ResourceMap::GetDepletionNode(double latitude, double longitude) {
    double value = ResourceUtilities::Deg2Rad(ResourceUtilities::clampLat(latitude));
    double value2 = ResourceUtilities::Deg2Rad(ResourceUtilities::clampLon(longitude));
    double num = dotnet::Math::Round(value);
    double num2 = dotnet::Math::Round(value2);
    return Vector2f(static_cast<float>(num), static_cast<float>(num2));
}

// ResourceMap.cs:589
PlanetaryResource ResourceMap::GetResourceByName(const std::string& resName, CelestialBody& body, HarvestTypes harvest) {
    PlanetaryResource planetaryResource;
    if (!Instance()->IsPlanetScanned(body.flightGlobalsIndex)) return planetaryResource;
    std::vector<std::shared_ptr<ResourceCache::AbundanceSummary>> list;
    for (const auto& a : ResourceCache::Instance().AbundanceCache())
        if (a->BodyId == body.flightGlobalsIndex && a->HarvestType == harvest) list.push_back(a);
    std::vector<std::shared_ptr<ResourceCache::AbundanceSummary>> list2;
    for (const auto& a : list)
        if (a->ResourceName == resName) list2.push_back(a);
    if (list2.size() > 0) {
        float num = 0.0f;
        int count3 = static_cast<int>(list2.size());
        for (int k = 0; k < count3; k++) num = f32store(fx(num) + fx(list2[static_cast<size_t>(k)]->Abundance));
        float num2 = f32store(fx(num) / fx(static_cast<float>(count3)));
        if (num2 > 0.0f) {
            const PartResourceDefinition* definition = PartResourceLibrary::Instance()->GetDefinition(resName);
            if (definition == nullptr) throw Nre();
            planetaryResource.fraction = num2;
            planetaryResource.resourceID = definition->id();
            planetaryResource.resourceName = resName;
        }
    }
    return planetaryResource;
}

// ResourceMap.cs:635
std::vector<PlanetaryResource> ResourceMap::GetResourceItemList(HarvestTypes harvest, CelestialBody& body) {
    std::vector<PlanetaryResource> list;
    if (!Instance()->IsPlanetScanned(body.flightGlobalsIndex)) return list;
    std::vector<std::shared_ptr<ResourceCache::AbundanceSummary>> list2;
    for (const auto& a : ResourceCache::Instance().AbundanceCache())
        if (a->BodyId == body.flightGlobalsIndex && a->HarvestType == harvest) list2.push_back(a);
    std::vector<std::string> list3 = FetchAllResourceNames(harvest);
    for (const std::string& text : list3) {
        std::vector<std::shared_ptr<ResourceCache::AbundanceSummary>> list4;
        for (const auto& a : list2)
            if (a->ResourceName == text) list4.push_back(a);
        if (list4.size() > 0) {
            float num = 0.0f;
            int count4 = static_cast<int>(list4.size());
            for (int l = 0; l < count4; l++) num = f32store(fx(num) + fx(list4[static_cast<size_t>(l)]->Abundance));
            float num2 = f32store(fx(num) / fx(static_cast<float>(count4)));
            if (num2 > 0.0f) {
                const PartResourceDefinition* definition = PartResourceLibrary::Instance()->GetDefinition(text);
                if (definition == nullptr) throw Nre();
                PlanetaryResource planetaryResource;
                planetaryResource.fraction = num2;
                planetaryResource.resourceID = definition->id();
                planetaryResource.resourceName = text;
                list.push_back(planetaryResource);
            }
        }
    }
    return list;
}

// ---- ResourceScenario.cs ---------------------------------------------------------------------------------------------------

ResourceScenario::ResourceScenario() : ScenarioModule("ResourceScenario") {}
ResourceScenario::~ResourceScenario() {
    map_.reset();
    if (instance_ == this) instance_ = nullptr;
}

// ResourceScenario.cs:19
void ResourceScenario::OnAwake() {
    instance_ = this;
    gameSettings_ = std::make_unique<ResourceGameSettings>();
    map_ = std::make_unique<ResourceMap>();
}

// ResourceScenario.cs:38
void ResourceScenario::Start() { map_->ResetCache(); }

// ResourceScenario.cs:43
void ResourceScenario::OnLoad(ConfigNode& gameNode) {
    ScenarioModule::OnLoad(gameNode);
    gameSettings_->Load(gameNode);
    if (ResourceMap::Instance() != nullptr) ResourceMap::Instance()->ResetCache();
}

// ResourceScenario.cs:53
void ResourceScenario::OnSave(ConfigNode& gameNode) {
    ScenarioModule::OnSave(gameNode);
    gameSettings_->Save(gameNode);
}

void ResourceScenario::Register() {
    ScenarioModuleFactory::Register("ResourceScenario", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new ResourceScenario()); });
}

}  // namespace ksp
