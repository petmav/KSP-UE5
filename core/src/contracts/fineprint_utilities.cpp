// WP-38: the rest of FinePrint/Utilities/ProgressUtilities.cs, CelestialUtilities.cs and the pure helpers of SystemUtilities.cs /
// StringUtilities.cs (the part WP-37 did not need). Declared in contracts/fineprint_lite.hpp (namespace ksp::Contracts::Lite keeps its
// WP-37 name so that no WP-37 call site changes).
#include "ksp/contracts/fineprint_lite.hpp"

#include <cmath>

#include "ksp/career/career.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/math/quaterniond.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp::Contracts::Lite {

namespace M = ksp::dotnet::Math;

// ---- ProgressUtilities -----------------------------------------------------------------------------------------------------------

namespace ProgressUtilities {

namespace {
float ProgFloat(const char* sub, const char* key) { return ContractDefsLite::F("Progression", sub, key); }
}  // namespace

// ProgressUtilities.cs:338
bool IntroWorldFirstContract(ProgressMilestone& milestone) {
    if (milestone.type == ProgressType::FIRSTLAUNCH) return true;
    if (milestone.type == ProgressType::REACHSPACE) return true;
    if (milestone.type == ProgressType::ORBIT && milestone.body == Planetarium::fetch->Home) return true;
    return false;
}

// ProgressUtilities.cs:350
bool OutlierWorldFirstContract(ProgressType type, CelestialBody* body) {
    if (type == ProgressType::LANDINGRETURN && body != nullptr && body->atmosphere) return true;
    switch (type) {
        case ProgressType::BASECONSTRUCTION: return true;
        case ProgressType::STATIONCONSTRUCTION: return true;
        default: return false;
    }
}

// ProgressUtilities.cs:367
bool OutlierWorldFirstContract(ProgressMilestone& milestone) { return OutlierWorldFirstContract(milestone.type, milestone.body); }

// ProgressUtilities.cs:437
ContractPrestige ProgressTypePrestige(ProgressType type, CelestialBody* body) {
    if (OutlierWorldFirstContract(type, body)) return ContractPrestige::Exceptional;
    const float num = ScoreProgressType(type, body);
    if (!(body == nullptr) && !(body == Planetarium::fetch->Home)) {
        if (fx(num) <= fx(ScoreProgressType(ProgressType::ESCAPE, body))) return ContractPrestige::Trivial;
        if (fx(num) >= fx(ScoreProgressType(ProgressType::FLIGHT, body))) return ContractPrestige::Exceptional;
    } else {
        if (fx(num) <= fx(ScoreProgressType(ProgressType::FLIGHT, body))) return ContractPrestige::Trivial;
        if (fx(num) >= fx(ScoreProgressType(ProgressType::ORBIT, body))) return ContractPrestige::Exceptional;
    }
    return ContractPrestige::Significant;
}

// ProgressUtilities.cs:467
ContractPrestige ProgressTypePrestige(ProgressMilestone& milestone) { return ProgressTypePrestige(milestone.type, milestone.body); }

// ProgressUtilities.cs:488
float WorldFirstStandardReward(ProgressRewardType reward, Currency currency, ProgressType progress, CelestialBody* body) {
    float num = 0.0f;
    switch (currency) {
        case Currency::Funds: num = ProgFloat("Funds", "BaseReward"); break;
        case Currency::Science: num = ProgFloat("Science", "BaseReward"); break;
        case Currency::Reputation: num = ProgFloat("Reputation", "BaseReward"); break;
    }
    const double passive = fx(ProgFloat("", "PassiveBaseRatio"));
    num = f32store(fx(num) * (reward == ProgressRewardType::CONTRACT ? (1.0 - passive) : passive));
    num = f32store(fx(num) * fx(ScoreProgressType(progress, body)));
    if (OutlierWorldFirstContract(progress, body)) num = f32store(fx(num) * fx(ProgFloat("", "OutlierMilestoneMultiplier")));
    if (reward == ProgressRewardType::CONTRACT) return num;
    GameVariables* gv = GameVariables::Instance;
    switch (currency) {
        case Currency::Funds: num = f32store(fx(num) * fx(gv->GetContractFundsCompletionFactor(ProgressTypePrestige(progress)))); break;
        case Currency::Science: num = f32store(fx(num) * fx(gv->GetContractScienceCompletionFactor(ProgressTypePrestige(progress)))); break;
        case Currency::Reputation: num = f32store(fx(num) * fx(gv->GetContractReputationCompletionFactor(ProgressTypePrestige(progress)))); break;
    }
    if (currency == Currency::Funds && body != nullptr)
        num = f32store(fx(num) * (1.0 + (fx(gv->GetContractDestinationWeight(body)) - 1.0) * fx(ProgFloat("", "PassiveBodyRatio"))));
    return Mathf::Round(num);
}

// ProgressUtilities.cs:530
float WorldFirstIntervalReward(ProgressRewardType reward, Currency currency, ProgressType progress, CelestialBody* body, int currentInterval,
                               int totalIntervals) {
    currentInterval = (currentInterval < 1) ? 1 : currentInterval;
    totalIntervals = (totalIntervals < currentInterval) ? currentInterval : totalIntervals;
    const float num = WorldFirstStandardReward(reward, currency, progress, body);
    const float num2 = f32store(fx(num) / fx(static_cast<float>(totalIntervals)));
    if (fx(num2) > 1.0) return num2;
    if (fx(num2) <= 0.0) return 0.0f;
    const float f = f32store(fx(num2) * fx(static_cast<float>(currentInterval - 1)));
    const float f2 = f32store(fx(num2) * fx(static_cast<float>(currentInterval)));
    if (fx(std::floor(f)) < fx(std::floor(f2))) return 1.0f;
    if (currentInterval == totalIntervals) return Mathf::Round(f32store(std::fmod(fx(num), 1.0)));
    return 0.0f;
}

// ProgressUtilities.cs:574
bool ReachedHomeBodies() {
    if (ProgressTracking::Instance() == nullptr) return false;
    KSPAchievements::CelestialBodySubtree* bodyTree = ProgressTracking::Instance()->GetBodyTree(Planetarium::fetch->Home);
    if (bodyTree != nullptr && bodyTree->IsReached()) {
        int count = static_cast<int>(Planetarium::fetch->Home->orbitingBodies.size());
        while (count-- > 0) {
            KSPAchievements::CelestialBodySubtree* bodyTree2 = ProgressTracking::Instance()->GetBodyTree(Planetarium::fetch->Home->orbitingBodies[static_cast<size_t>(count)]);
            if (!(bodyTree2 != nullptr && bodyTree2->IsReached())) return false;
        }
        return true;
    }
    return false;
}

// ProgressUtilities.cs:603
double FindNextRecord(double currentRecord, double maximumRecord, double roundValue) {
    int interval = 0;
    return FindNextRecord(currentRecord, maximumRecord, roundValue, interval);
}

// ProgressUtilities.cs:624
double FindNextRecord(double currentRecord, double maximumRecord, double roundValue, int& interval) {
    const double num = static_cast<double>(ContractDefsLite::I("Progression", "", "RecordSplit"));
    const double x = M::Pow(100.0, 1.0 / (num - 1.0));
    int num2 = 1;
    double num3;
    while (true) {
        if (static_cast<double>(num2) <= num) {
            num3 = maximumRecord * (0.01 * M::Pow(x, static_cast<double>(num2 - 1)));
            num3 = M::Round(num3 / roundValue) * roundValue;
            if (!(num3 <= currentRecord)) break;
            num2++;
            continue;
        }
        interval = 2147483647;
        return std::numeric_limits<double>::max();
    }
    interval = num2;
    return num3;
}

// ProgressUtilities.cs:645
bool VisitedSurfaceOf(CelestialBody* body, MannedStatus manned) {
    if (body->isHomeWorld) return true;
    bool flag;
    bool flag2;
    ProgressTracking* pt = ProgressTracking::Instance();
    switch (manned) {
        default:
            flag = body->hasSolidSurface && pt->NodeComplete({body->name(), "Landing"});
            flag2 = body->ocean && pt->NodeComplete({body->name(), "Splashdown"});
            break;
        case MannedStatus::UNMANNED:
            flag = body->hasSolidSurface && pt->NodeCompleteUnmanned({body->name(), "Landing"});
            flag2 = body->ocean && pt->NodeCompleteUnmanned({body->name(), "Splashdown"});
            break;
        case MannedStatus::MANNED:
            flag = body->hasSolidSurface && pt->NodeCompleteManned({body->name(), "Landing"});
            flag2 = body->ocean && pt->NodeCompleteManned({body->name(), "Splashdown"});
            break;
    }
    return flag || flag2;
}

}  // namespace ProgressUtilities

// ---- StringUtilities / SystemUtilities (pure helpers) ----------------------------------------------------------------------------

namespace StringUtilities {
// StringUtilities.cs:220
std::string PossessiveString(std::string thing) {
    const auto endsWith = [&](const char* suffix) {
        const std::string s(suffix);
        return thing.size() >= s.size() && thing.compare(thing.size() - s.size(), s.size(), s) == 0;
    };
    thing = (endsWith("s") || endsWith("ch") || endsWith("z")) ? (thing + "'") : (thing + "'s");
    return thing;
}
}  // namespace StringUtilities

namespace SystemUtilities {

namespace {
// SystemUtilities.cs:541 GetDeviationAsFraction
double GetDeviationAsFraction(double deviation) {
    deviation = (deviation > 100.0) ? 100.0 : deviation;
    deviation = (deviation < 0.0) ? 0.0 : deviation;
    deviation = (deviation > 1.0) ? (deviation / 100.0) : deviation;
    return deviation;
}
}  // namespace

// SystemUtilities.cs:519
bool WithinDeviation(double v1, double v2, double deviation) {
    const double deviationAsFraction = GetDeviationAsFraction(deviation);
    return MeasureDeviation(v1, v2) <= deviationAsFraction;
}

// SystemUtilities.cs:525
bool WithinDeviationByValue(double v1, double v2, double deviation, double fullValue) {
    const double deviationAsFraction = GetDeviationAsFraction(deviation);
    return MeasureDeviationByValue(v1, v2, fullValue) <= deviationAsFraction;
}

// SystemUtilities.cs:531
double MeasureDeviation(double v1, double v2) { return M::Abs(v1 - v2) / ((v1 + v2) / 2.0); }

// SystemUtilities.cs:536
double MeasureDeviationByValue(double v1, double v2, double fullValue) { return M::Abs(v1 - v2) / fullValue; }

// SystemUtilities.cs:549
double WithinDeviationAccuracy(double v1, double v2, double deviation) {
    deviation = (deviation > 100.0) ? 100.0 : deviation;
    deviation = (deviation < 0.0) ? 0.0 : deviation;
    deviation = (deviation > 1.0) ? (deviation / 100.0) : deviation;
    return UtilMath::InverseLerp(deviation, 0.0, M::Abs(v1 - v2) / ((v1 + v2) / 2.0));
}

// SystemUtilities.cs:557
double WithinDeviationByReferenceAccuracy(double v1, double v2, double reference, double deviation) {
    deviation = (deviation > 100.0) ? 100.0 : deviation;
    deviation = (deviation < 0.0) ? 0.0 : deviation;
    deviation = (deviation > 1.0) ? (deviation / 100.0) : deviation;
    return UtilMath::InverseLerp(deviation, 0.0, M::Abs(v1 - v2) / reference);
}

}  // namespace SystemUtilities

// ---- CelestialUtilities ----------------------------------------------------------------------------------------------------------

namespace CelestialUtilities {

std::function<double(CelestialBody*)> highestPeakProvider;

// CelestialUtilities.cs:10
CelestialBody* RandomBody(const std::vector<CelestialBody*>& bodies) {
    KSPRandom kSPRandom(ContractsEnvironment::Current().NewRandomSeed());
    return bodies.at(static_cast<size_t>(kSPRandom.Next(static_cast<int>(bodies.size()))));
}

// CelestialUtilities.cs:16
CelestialBody* HighestBody(const std::vector<CelestialBody*>& bodies) {
    float num = std::numeric_limits<float>::lowest();
    int count = static_cast<int>(bodies.size());
    while (count-- > 0) {
        const float num2 = PlanetScienceRanking(bodies[static_cast<size_t>(count)]);
        if (fx(num2) > fx(num)) num = num2;
    }
    std::vector<CelestialBody*> list;
    int count2 = static_cast<int>(bodies.size());
    while (count2-- > 0) {
        if (fx(PlanetScienceRanking(bodies[static_cast<size_t>(count2)])) == fx(num)) list.push_back(bodies[static_cast<size_t>(count2)]);
    }
    if (!list.empty()) return RandomBody(list);
    return nullptr;
}

// CelestialUtilities.cs:44
CelestialBody* LowestBody(const std::vector<CelestialBody*>& bodies) {
    float num = std::numeric_limits<float>::max();
    int count = static_cast<int>(bodies.size());
    while (count-- > 0) {
        const float num2 = PlanetScienceRanking(bodies[static_cast<size_t>(count)]);
        if (fx(num2) < fx(num)) num = num2;
    }
    std::vector<CelestialBody*> list;
    int count2 = static_cast<int>(bodies.size());
    while (count2-- > 0) {
        if (fx(PlanetScienceRanking(bodies[static_cast<size_t>(count2)])) == fx(num)) list.push_back(bodies[static_cast<size_t>(count2)]);
    }
    if (!list.empty()) return RandomBody(list);
    return nullptr;
}

// CelestialUtilities.cs:72
std::vector<CelestialBody*> GetNeighbors(CelestialBody* body, const std::function<bool(CelestialBody*)>& where) {
    std::vector<CelestialBody*> list;
    if (body == nullptr) return list;
    if (body->referenceBody() != nullptr && (!where || where(body->referenceBody()))) list.push_back(body->referenceBody());
    int count = static_cast<int>(body->orbitingBodies.size());
    while (count-- > 0) {
        CelestialBody* cb = body->orbitingBodies[static_cast<size_t>(count)];
        if (!(cb == nullptr) && (!where || where(cb))) list.push_back(cb);
    }
    return list;
}

// CelestialUtilities.cs:111
double GetHighestPeak(CelestialBody* body) {
    if (!(body == nullptr) && !(body->pqsController == nullptr)) return highestPeakProvider ? highestPeakProvider(body) : 0.0;
    return 0.0;
}

// CelestialUtilities.cs:148
double GetNextTimeWarp(CelestialBody* body, double altitude) {
    if (body == nullptr) return 0.0;
    if (body->timeWarpAltitudeLimits.empty()) return 0.0;
    double num = std::numeric_limits<double>::max();
    double num2 = std::numeric_limits<double>::lowest();
    int num3 = static_cast<int>(body->timeWarpAltitudeLimits.size());
    while (num3-- > 0) {
        const double num4 = static_cast<double>(body->timeWarpAltitudeLimits[static_cast<size_t>(num3)]);
        if (num4 > num2) num2 = num4;
        if (num4 > altitude && num4 < num) num = num4;
    }
    if (num > num2) num = num2;
    return num;
}

// CelestialUtilities.cs:184
double GetMinimumOrbitalDistance(CelestialBody* body, float margin) {
    if (body == Planetarium::fetch->Sun) {
        CelestialBody* celestialBody = CareerEnvironment::Current().HomeBody();
        double num = 0.0;
        if (celestialBody != nullptr) {
            while (celestialBody->referenceBody() != FlightRegistry::Bodies()[0] && celestialBody->referenceBody() != nullptr) celestialBody = celestialBody->referenceBody();
            num = M::Pow(celestialBody->orbit()->GetOrbit()->semiMajorAxis, 2.0) * 4.0 * 3.141592653589793 * PhysicsGlobals::SolarLuminosityAtHome();
        }
        double num2 = static_cast<double>(ContractDefsLite::F("", "", "SolarOrbitHeatTolerance"));
        num2 *= num2;
        num2 *= num2;
        double spaceTemperature = PhysicsGlobals::SpaceTemperature();
        spaceTemperature *= spaceTemperature;
        spaceTemperature *= spaceTemperature;
        return static_cast<double>(margin) *
               M::Sqrt(0.5 * num / PhysicsGlobals::StefanBoltzmanConstant() / (num2 - spaceTemperature) / (3.141592653589793 * 4.0));
    }
    double num3 = body->Radius;
    double radius = body->Radius;
    if (body->atmosphere) num3 += static_cast<double>(margin) * body->atmosphereDepth;
    const double highestPeak = GetHighestPeak(body);
    radius += static_cast<double>(margin) * GetNextTimeWarp(body, highestPeak);
    return M::Max(num3, radius);
}

// CelestialUtilities.cs:243
Vessel::Situations ApplicableSituation(int seed, CelestialBody* body, bool splashAllowed) {
    KSPRandom kSPRandom(seed);
    std::vector<Vessel::Situations> list;
    list.push_back(Vessel::Situations::ORBITING);
    if (body->ocean && splashAllowed) list.push_back(Vessel::Situations::SPLASHED);
    if (!IsGasGiant(body)) list.push_back(Vessel::Situations::LANDED);
    return list[static_cast<size_t>(kSPRandom.Next(0, static_cast<int>(list.size())))];
}

// CelestialUtilities.cs:292
float PlanetScienceRanking(CelestialBody* body) {
    float num = 0.0f;
    const auto& bodies = FlightRegistry::Bodies();
    int count = static_cast<int>(bodies.size());
    while (count-- > 0) {
        CelestialBody* cb = bodies[static_cast<size_t>(count)];
        if (fx(cb->scienceValues.RecoveryValue) > fx(num)) num = cb->scienceValues.RecoveryValue;
    }
    return f32store(fx(body->scienceValues.RecoveryValue) / fx(num));
}

// CelestialUtilities.cs:306
Vector3f LLAtoECEF(double lat, double lon, double alt, double radius) {
    lat = (lat - 90.0) * (3.141592653589793 / 180.0);
    lon *= 3.141592653589793 / 180.0;
    const double num = (radius + alt) * -1.0 * M::Sin(lat) * M::Cos(lon);
    const double num2 = (radius + alt) * M::Cos(lat);
    const double num3 = (radius + alt) * -1.0 * M::Sin(lat) * M::Sin(lon);
    return Vector3f(static_cast<float>(num), static_cast<float>(num2), static_cast<float>(num3));
}

// CelestialUtilities.cs:314
double SynchronousSMA(CelestialBody* body) {
    if (body == nullptr) return 0.0;
    return M::Pow(body->gravParameter * M::Pow(body->rotationPeriod / (3.141592653589793 * 2.0), 2.0), 1.0 / 3.0);
}

// CelestialUtilities.cs:324
bool CanBodyBeSynchronous(CelestialBody* body, double eccentricity) {
    if (body == nullptr) return false;
    const double num = body->minOrbitalDistance * 1.05;
    const double num2 = SynchronousSMA(body);
    if (!((1.0 + eccentricity) * num2 > body->sphereOfInfluence) && num2 >= num) return true;
    return false;
}

// CelestialUtilities.cs:339
double KolniyaSMA(CelestialBody* body) {
    if (body == nullptr) return 0.0;
    const double num = body->rotationPeriod / 2.0;
    return M::Pow(body->gravParameter * M::Pow(num / (3.141592653589793 * 2.0), 2.0), 1.0 / 3.0);
}

// CelestialUtilities.cs:350
bool CanBodyBeKolniya(CelestialBody* body) {
    if (body == nullptr) return false;
    const double num = body->minOrbitalDistance * 1.05;
    const double num2 = KolniyaSMA(body);
    const double num3 = num;
    if (!(num2 * 2.0 - num3 > body->sphereOfInfluence) && num2 >= num) return true;
    return false;
}

// CelestialUtilities.cs:367
bool CanBodyBeTundra(CelestialBody* body) {
    if (body == nullptr) return false;
    const double num = body->minOrbitalDistance * 1.05;
    const double num2 = SynchronousSMA(body);
    const double num3 = num;
    if (!(num2 * 2.0 - num3 > body->sphereOfInfluence) && num2 >= num) return true;
    return false;
}

// CelestialUtilities.cs:383
double TerrainAltitude(CelestialBody* body, double latitude, double longitude, bool underwater) {
    double num = 0.0;
    if (body->pqsController != nullptr) {
        const Vector3d radialVector = QuaternionD::AngleAxis(longitude, Vector3d::down()) * QuaternionD::AngleAxis(latitude, Vector3d::forward()) * Vector3d::right();
        num = body->pqsController->GetSurfaceHeight(radialVector) - body->pqsController->radius;
        if (body->ocean && !underwater && num < 0.0) num = 0.0;
    }
    return num;
}

// CelestialUtilities.cs:432
double GetSolarExtents() {
    double num = 0.0;
    CelestialBody* sun = Planetarium::fetch->Sun;
    for (int num2 = static_cast<int>(sun->orbitingBodies.size()) - 1; num2 >= 0; num2--) {
        CelestialBody* cb = sun->orbitingBodies[static_cast<size_t>(num2)];
        if (cb->orbit() != nullptr && cb->orbit()->GetOrbit()->ApR() > num) num = cb->orbit()->GetOrbit()->ApR();
    }
    return num;
}

// CelestialUtilities.cs:446
double GreatCircleDistance(CelestialBody* body, double latitude1, double longitude1, double latitude2, double longitude2) {
    return body->Radius * M::Acos(Vector3d::Dot(body->GetRelSurfaceNVector(latitude1, longitude1), body->GetRelSurfaceNVector(latitude2, longitude2)));
}

// CelestialUtilities.cs:451
double GreatCircleDistance(CelestialBody* body, Vector3d position1, Vector3d position2) {
    const Vector3d bp = body->position;
    return body->Radius * M::Acos(Vector3d::Dot((position1 - bp).normalized(), (position2 - bp).normalized()));
}

// CelestialUtilities.cs:456
CelestialBody* GetHostPlanet(CelestialBody* body) {
    if (body == nullptr) return Planetarium::fetch->Sun;
    while (body->referenceBody() != nullptr && body->referenceBody() != Planetarium::fetch->Sun) body = body->referenceBody();
    return body;
}

// CelestialUtilities.cs:466
std::vector<CelestialBody*> GetPlanetarySystem(CelestialBody* body) {
    body = GetHostPlanet(body);
    return ChildrenOf(body);
}

}  // namespace CelestialUtilities

}  // namespace ksp::Contracts::Lite

namespace ksp::Contracts::Lite {

// SystemUtilities.cs:113 (the CelestialBody branch)
void LoadNode(ConfigNode* node, const char* className, const char* valueName, CelestialBody*& value, CelestialBody* defaultValue, bool logging) {
    enum class R { Null, Success, Invalid, NoValue } result = R::Null;
    std::string error;
    std::string name;
    if (node == nullptr) {
        result = R::Null;
    } else {
        name = ShortName(valueName);
        if (node->HasValue(name)) {
            int32_t value2 = 0;
            if (!detail::TryConvert<int32_t>(*node->GetValue(name), value2)) {
                result = R::Invalid;
            } else {
                CelestialBody* celestialBody = FlightRegistry::Bodies().at(static_cast<size_t>(value2));
                if (celestialBody == nullptr) {
                    result = R::Invalid;
                } else {
                    value = celestialBody;
                    result = R::Success;
                }
            }
        } else {
            result = R::NoValue;
        }
    }
    if (result == R::Success) return;
    if (logging) log::Warning(std::string("Contract Log: ") + className + " cannot load " + valueName + " as a body. Initializing with the default!");
    value = defaultValue;
}

}  // namespace ksp::Contracts::Lite
