// WP-38 slice 3: SentinelMission/SentinelUtilities.cs and SentinelScanType.cs. See include/ksp/fineprint/sentinel.hpp.
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/fineprint/contract_defs.hpp"
#include "ksp/fineprint/sentinel.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::SentinelMission {

namespace M = ksp::dotnet::Math;
using KSP::Localization::Localizer;
namespace L = ksp::Contracts::Lite;

void SentinelEnvironment::Reset() {
    discoverableObjects.reset();
    discoverableObjectsProvider = nullptr;
    countSpaceObjects = nullptr;
    generateAsteroidName = nullptr;
    spawnAsteroid = nullptr;
    shouldSpawnComet = nullptr;
    generateWeightedCometType = nullptr;
    getCometOrbitType = nullptr;
    generateCometDefinition = nullptr;
    spawnComet = nullptr;
}

// C++ helper: the C# static initialisers of SentinelUtilities.cs:11-39
void SentinelUtilities::Reset() {
    SentinelPartName = "InfraredTelescope";
    SentinelPartDisplayName = "#autoLOC_6002282";
    SentinelModuleName = "SentinelModule";
    SentinelModuleDisplayName = "#autoLOC_6002283";
    SentinelViewAngle = 200.0;
    SpawnChance = 0.25f;
    WeightedStability = 3;
    MinAsteroidEccentricity = 0.05;
    MaxAsteroidEccentricity = 0.4;
    MinAsteroidInclination = 5.0;
    MaxAsteroidInclination = 40.0;
    perSentinelObjectLimit = 10;
    sentinelPartTitle = "";
}

namespace {
CelestialBody* Sun() { return Planetarium::fetch->Sun; }
Orbit& OrbitOfBody(CelestialBody* b) {
    Orbit* o = BodyOrbit(b);
    if (o == nullptr) throw NullReferenceException("SentinelUtilities: body " + b->bodyName + " has no orbit");
    return *o;
}
// `new KSPRandom()` (tick-count seeded: ContractsEnvironment::newRandomSeed)
KSPRandom FreshRandom() { return KSPRandom(Contracts::ContractsEnvironment::Current().NewRandomSeed()); }
}  // namespace

// SentinelUtilities.cs:122
std::string SentinelUtilities::SentinelPartTitle() {
    if (sentinelPartTitle != "") return sentinelPartTitle;
    const AvailablePart* partInfoByName = (SaveEnvironment::Current().getPartInfoByName ? SaveEnvironment::Current().getPartInfoByName(SentinelPartName) : nullptr);
    if (partInfoByName != nullptr) return partInfoByName->title;
    return Localizer::Format("#autoLOC_6002284");
}

// SentinelUtilities.cs:139
bool SentinelUtilities::SentinelCanScan(Vessel& v, CelestialBody* innerBody, CelestialBody* outerBody) {
    double inclination;
    double eccentricity;
    if (v.loaded) {
        if (v.situation != Vessel::Situations::ORBITING || v.orbit().referenceBody != Sun()) return false;
        inclination = v.orbit().inclination;
        eccentricity = v.orbit().eccentricity;
    } else {
        if (v.protoVessel->situation != VesselSituations::ORBITING ||
            FlightRegistry::Bodies().at(static_cast<size_t>(v.protoVessel->orbitSnapShot->ReferenceBodyIndex)) != Sun())
            return false;
        inclination = v.protoVessel->orbitSnapShot->inclination;
        eccentricity = v.protoVessel->orbitSnapShot->eccentricity;
    }
    if ((innerBody == nullptr || outerBody == nullptr) && !FindInnerAndOuterBodies(v, innerBody, outerBody)) return false;
    if (M::Abs(OrbitOfBody(outerBody).eccentricity - eccentricity) > 0.20000000298023224) return false;
    if (M::Abs(M::Abs(OrbitOfBody(outerBody).inclination) - M::Abs(inclination)) >
        static_cast<double>(FinePrint::ContractDefs::Sentinel::SignificantDeviation) / 100.0 * 90.0)
        return false;
    if (std::fmod(M::Abs(OrbitOfBody(outerBody).inclination), 180.0) >= 1.0) {
        double num = v.orbit().LAN;
        double num2 = OrbitOfBody(outerBody).LAN;
        if (v.orbit().inclination < 0.0) num = std::fmod(num + 180.0, 360.0);
        if (OrbitOfBody(outerBody).inclination < 0.0) num2 = std::fmod(num2 + 180.0, 360.0);
        float num3 = static_cast<float>(std::fmod(static_cast<double>(static_cast<float>(M::Abs(num - num2))), 360.0));
        if (num3 > 180.0f) num3 = f32store(360.0f - num3);
        if (static_cast<double>(num3) > FinePrint::ContractDefs::Satellite::SignificantDeviation / 100.0 * 360.0) return false;
    }
    return true;
}

// SentinelUtilities.cs:198
bool SentinelUtilities::FindInnerAndOuterBodies(double SMA, CelestialBody*& innerBody, CelestialBody*& outerBody) {
    std::vector<std::pair<double, CelestialBody*>> dictionary;
    auto add = [&](double key, CelestialBody* b) {
        for (const auto& kv : dictionary)
            if (kv.first == key) throw ArgumentException("An item with the same key has already been added.");
        dictionary.emplace_back(key, b);
    };
    const std::vector<CelestialBody*>& bodies = FlightRegistry::Bodies();
    for (size_t i = 0; i < bodies.size(); ++i) {
        CelestialBody* celestialBody = bodies[i];
        if (celestialBody == Sun())
            add(0.0, celestialBody);
        else if (celestialBody->referenceBody() == Sun())
            add(OrbitOfBody(celestialBody).semiMajorAxis, celestialBody);
    }
    std::vector<double> list;
    for (const auto& kv : dictionary) list.push_back(kv.first);
    std::sort(list.begin(), list.end());
    auto lookup = [&](double key) -> CelestialBody* {
        for (const auto& kv : dictionary)
            if (kv.first == key) return kv.second;
        throw KeyNotFoundException("The given key was not present in the dictionary.");
    };
    size_t num = 0;
    while (true) {
        if (num < list.size()) {
            if (list[num] <= SMA) {
                if (num + 1 >= list.size()) throw std::out_of_range("Index was out of range. Must be non-negative and less than the size of the collection.");
                if (!(list[num + 1] <= SMA)) break;
            }
            num++;
            continue;
        }
        innerBody = lookup(list[0]);
        outerBody = lookup(list[list.size() - 1]);
        return false;
    }
    innerBody = lookup(list[num]);
    outerBody = lookup(list[num + 1]);
    return true;
}

// SentinelUtilities.cs:251
bool SentinelUtilities::FindInnerAndOuterBodies(Orbit& o, CelestialBody*& innerBody, CelestialBody*& outerBody) {
    double semiMajorAxis = o.semiMajorAxis;
    if (o.referenceBody != nullptr && o.referenceBody != Sun()) {
        CelestialBody* referenceBody = o.referenceBody;
        while (referenceBody != Sun() && referenceBody->referenceBody() != nullptr && referenceBody->referenceBody() != Sun())
            referenceBody = OrbitOfBody(referenceBody).referenceBody;
        semiMajorAxis = OrbitOfBody(referenceBody).semiMajorAxis;
    }
    return FindInnerAndOuterBodies(semiMajorAxis, innerBody, outerBody);
}

// SentinelUtilities.cs:251
bool SentinelUtilities::FindInnerAndOuterBodies(Vessel& v, CelestialBody*& innerBody, CelestialBody*& outerBody) {
    double semiMajorAxis;
    CelestialBody* celestialBody;
    if (v.loaded) {
        semiMajorAxis = v.orbit().semiMajorAxis;
        celestialBody = v.orbit().referenceBody;
    } else {
        semiMajorAxis = v.protoVessel->orbitSnapShot->semiMajorAxis;
        celestialBody = FlightRegistry::Bodies().at(static_cast<size_t>(v.protoVessel->orbitSnapShot->ReferenceBodyIndex));
    }
    if (celestialBody != Sun()) {
        CelestialBody* celestialBody2 = celestialBody;
        while (celestialBody2 != Sun() && celestialBody2->referenceBody() != nullptr && celestialBody2->referenceBody() != Sun())
            celestialBody2 = OrbitOfBody(celestialBody2).referenceBody;
        semiMajorAxis = OrbitOfBody(celestialBody2).semiMajorAxis;
    }
    return FindInnerAndOuterBodies(semiMajorAxis, innerBody, outerBody);
}

// SentinelUtilities.cs:277
double SentinelUtilities::AdjustedSentinelViewAngle(const Orbit& innerOrbit, const Orbit& outerOrbit) {
    const double num = innerOrbit.semiMajorAxis / 2.0;
    const double num2 = outerOrbit.semiMajorAxis / 2.0;
    const double num3 = 3.1415926535897931 / 180.0 * ((360.0 - SentinelViewAngle) / 2.0);
    const double num4 = M::Asin(num / num2 * M::Sin(num3));
    const double num5 = 3.1415926535897931 - num4;
    const double val = (3.1415926535897931 - num3 - num4) * (180.0 / 3.1415926535897931);
    const double val2 = (3.1415926535897931 - num3 - num5) * (180.0 / 3.1415926535897931);
    double num6 = M::Max(val, val2) * 2.0;
    if (num6 > SentinelViewAngle) num6 = SentinelViewAngle;
    if (num6 < SentinelViewAngle / 100.0) num6 = SentinelViewAngle / 100.0;
    return num6;
}

// SentinelUtilities.cs:298
double SentinelUtilities::GetEscapeVelocity(CelestialBody* body, double altitude) {
    if (body != Sun()) {
        if (altitude <= body->sphereOfInfluence)
            return M::Sqrt(2.0 * body->gravParameter / altitude - body->gravParameter / (body->Radius + body->sphereOfInfluence));
        return 0.0;
    }
    return M::Sqrt(2.0 * body->gravParameter / altitude);
}

// SentinelUtilities.cs:311
double SentinelUtilities::GetMinimumOrbitalSpeed(CelestialBody* body) {
    const double num = (body == Sun()) ? L::CelestialUtilities::GetSolarExtents() : body->sphereOfInfluence;
    return M::Sqrt(body->gravParameter / num);
}

// SentinelUtilities.cs:317
double SentinelUtilities::GetProgradeBurnAllowance(Orbit& o) {
    return GetEscapeVelocity(o.referenceBody, o.altitude + o.referenceBody->Radius) - o.GetRelativeVel().magnitude();
}

// SentinelUtilities.cs:322
double SentinelUtilities::GetRetrogradeBurnAllowance(Orbit& o) { return o.GetRelativeVel().magnitude() - GetMinimumOrbitalSpeed(o.referenceBody); }

// SentinelUtilities.cs:327
UntrackedObjectClass SentinelUtilities::GetVesselClass(Vessel& v) {
    UntrackedObjectClass result = UntrackedObjectClass::C;
    if (!v.loaded) {
        if (v.protoVessel->discoveryInfo->HasValue("size")) {
            int size = 0;
            if (!dotnet::TryParseInt32(*v.protoVessel->discoveryInfo->GetValue("size"), size)) throw dotnet::FormatException("Input string was not in a correct format.");
            result = static_cast<UntrackedObjectClass>(size);
        }
    } else {
        result = v.GetDiscoveryInfo().objectSize();
    }
    return result;
}

// SentinelUtilities.cs:344 (`generator as KSPRandom`: a null generator is `new KSPRandom()`)
SentinelScanType SentinelUtilities::RandomScanType(KSPRandom* generator) {
    constexpr int names = 4;  // Enum.GetNames(typeof(SentinelScanType)).Length
    if (generator != nullptr) return static_cast<SentinelScanType>(generator->Next(0, names));
    KSPRandom fresh = FreshRandom();
    return static_cast<SentinelScanType>(fresh.Next(0, names));
}

// SentinelUtilities.cs:349
UntrackedObjectClass SentinelUtilities::WeightedAsteroidClass(KSPRandom* generator) {
    std::optional<KSPRandom> fresh;
    KSPRandom* kSPRandom = generator;
    if (kSPRandom == nullptr) {
        fresh.emplace(FreshRandom());
        kSPRandom = &*fresh;
    }
    int num = 0;
    int num2 = 9 - 1;  // Enum.GetNames(typeof(UntrackedObjectClass)).Length - 1
    if (SentinelEnvironment::GetDiscoverableObjects().has_value()) {
        num = static_cast<int>(SentinelEnvironment::GetDiscoverableObjects()->minAsteroidClass);
        num2 = static_cast<int>(SentinelEnvironment::GetDiscoverableObjects()->maxAsteroidClass);
    }
    int num3 = num2 - num;
    while (num3 > 0 && kSPRandom->Next(100) > 50) num3--;
    return static_cast<UntrackedObjectClass>(num3 + num);
}

// SentinelUtilities.cs:377
double SentinelUtilities::WeightedRandom(KSPRandom* generator) {
    if (generator != nullptr) return M::Pow(generator->NextDouble(), WeightedStability);
    KSPRandom fresh = FreshRandom();
    return M::Pow(fresh.NextDouble(), WeightedStability);
}

// SentinelUtilities.cs:377
double SentinelUtilities::WeightedRandom(KSPRandom* generator, double min, double max) {
    if (generator != nullptr) return min + M::Pow(generator->NextDouble(), WeightedStability) * (max - min);
    KSPRandom fresh = FreshRandom();
    return min + M::Pow(fresh.NextDouble(), WeightedStability) * (max - min);
}

// SentinelUtilities.cs:377 (a null generator is `new Random()`: a fresh KSPRandom stands in for System.Random)
double SentinelUtilities::WeightedRandom(KSPRandom* generator, double max) {
    std::optional<KSPRandom> fresh;
    if (generator == nullptr) {
        fresh.emplace(FreshRandom());
        generator = &*fresh;
    }
    return WeightedRandom(generator, 0.0, max);
}

// SentinelUtilities.cs:386
double SentinelUtilities::RandomRange(KSPRandom* generator, double min, double max) {
    std::optional<KSPRandom> fresh;
    KSPRandom* kSPRandom = generator;
    if (kSPRandom == nullptr) {
        fresh.emplace(FreshRandom());
        kSPRandom = &*fresh;
    }
    const double val = min;
    const double val2 = max;
    min = M::Min(val, val2);
    max = M::Max(val, val2);
    return min + kSPRandom->NextDouble() * (max - min);
}

// SentinelUtilities.cs:396
float SentinelUtilities::CalculateReadDuration(const std::string& s) {
    const float num = 200.0f;
    const float num2 = 5.0f;
    return f32store(f32store(f32store(static_cast<float>(s.size()) / num2) / num) * 60.0f);
}

// SentinelUtilities.cs:403
bool SentinelUtilities::IsOnSolarOrbit(CelestialBody* body) {
    if (!(body == Sun()) && !(body->referenceBody() == nullptr) && !(body->referenceBody() != Sun())) return true;
    return false;
}

}  // namespace ksp::SentinelMission
