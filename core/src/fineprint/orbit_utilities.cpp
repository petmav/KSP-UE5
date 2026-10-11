// WP-38: FinePrint/Utilities/OrbitUtilities.cs. Orbit generation is a pure function of the KSPRandom(seed) draw sequence and the body
// (minOrbitalDistance, sphereOfInfluence, gravParameter, rotationPeriod), so it is reproducible for a given seed.
#include "ksp/fineprint/fineprint.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp::FinePrint::Utilities::OrbitUtilities {

namespace M = ksp::dotnet::Math;
namespace CU = ksp::Contracts::Lite::CelestialUtilities;
namespace SU = ksp::Contracts::Lite::SystemUtilities;
using ksp::Contracts::ContractDefsLite;

namespace {

// OrbitUtilities.cs:10 OrbitGenerationInfo
struct OrbitGenerationInfo {
    CelestialBody* body = nullptr;
    OrbitType type = OrbitType::RANDOM;
    double altitudeFactor = 0.0;
    double inclinationFactor = 0.0;
    double eccentricityOverride = 0.0;
    Orbit orbit;
    std::unique_ptr<KSPRandom> generator;

    // :40
    OrbitGenerationInfo(int seed, CelestialBody* body_, OrbitType type_, double altitudeDifficulty, double inclinationDifficulty,
                        double eccentricityOverride_ = 0.0)
        : body(body_), type(type_), altitudeFactor(altitudeDifficulty), inclinationFactor(inclinationDifficulty), eccentricityOverride(eccentricityOverride_) {
        orbit.referenceBody = body_;
        generator = std::make_unique<KSPRandom>(seed);
    }
    // :54 (the ref Orbit constructor: the info works on the orbit it is given)
    OrbitGenerationInfo(int seed, const Orbit& orbit_, OrbitType type_, double altitudeDifficulty, double inclinationDifficulty,
                        double eccentricityOverride_ = 0.0)
        : body(orbit_.referenceBody), type(type_), altitudeFactor(altitudeDifficulty), inclinationFactor(inclinationDifficulty),
          eccentricityOverride(eccentricityOverride_), orbit(orbit_) {
        generator = std::make_unique<KSPRandom>(seed);
    }
};

bool CoinFlip(OrbitGenerationInfo& info) { return Contracts::Lite::CoinFlip(info.generator.get()); }

// OrbitUtilities.cs:256
void GenerateApsides(OrbitGenerationInfo& info) {
    if (info.orbit.referenceBody == nullptr) {
        log::Error("Cannot generate orbit apoapsis or periapsis with no reference body.");
        return;
    }
    const double num = info.orbit.referenceBody->minOrbitalDistance * 1.05;
    const double num2 = (info.orbit.referenceBody == Planetarium::fetch->Sun)
                            ? CU::GetSolarExtents()
                            : M::Max(num, info.orbit.referenceBody->sphereOfInfluence * info.altitudeFactor);
    switch (info.type) {
        case OrbitType::SYNCHRONOUS:
        case OrbitType::STATIONARY:
            info.orbit.semiMajorAxis = CU::SynchronousSMA(info.orbit.referenceBody);
            info.orbit.eccentricity = (info.type == OrbitType::SYNCHRONOUS) ? info.eccentricityOverride : 0.0;
            break;
        case OrbitType::KOLNIYA: {
            info.orbit.semiMajorAxis = CU::KolniyaSMA(info.orbit.referenceBody);
            const double num6 = num;
            const double num5 = info.orbit.semiMajorAxis * 2.0 - num6;
            info.orbit.eccentricity = (num5 - num6) / (num5 + num6);
            break;
        }
        case OrbitType::TUNDRA: {
            info.orbit.semiMajorAxis = CU::SynchronousSMA(info.orbit.referenceBody);
            const double num6 = num;
            const double num5 = info.orbit.semiMajorAxis * 2.0 - num6;
            info.orbit.eccentricity = (num5 - num6) / (num5 + num6);
            break;
        }
        case OrbitType::POLAR:
        case OrbitType::EQUATORIAL:
        case OrbitType::RANDOM: {
            const double num3 = num + (num2 - num) * info.generator->NextDouble();
            const double num4 = num + (num2 - num) * info.generator->NextDouble();
            const double val = static_cast<double>(Mathf::Lerp(static_cast<float>(num3), static_cast<float>(num4), static_cast<float>(1.0 - info.altitudeFactor)));
            const double num5 = M::Max(val, num4);
            const double num6 = M::Min(val, num4);
            info.orbit.semiMajorAxis = (num5 + num6) / 2.0;
            info.orbit.eccentricity = (num5 - num6) / (num5 + num6);
            break;
        }
    }
}

// OrbitUtilities.cs:307
void GenerateInclination(OrbitGenerationInfo& info) {
    double num = M::Max(1.0, info.generator->NextDouble() * 90.0 * info.inclinationFactor);
    if (info.type != OrbitType::EQUATORIAL && info.type != OrbitType::STATIONARY) {
        if (info.type == OrbitType::POLAR) num = 90.0;
    } else {
        num = 0.0;
    }
    if (info.orbit.referenceBody != nullptr && (info.orbit.referenceBody == Planetarium::fetch->Home || info.orbit.referenceBody == Planetarium::fetch->Sun)) {
        switch (info.type) {
            case OrbitType::KOLNIYA:
            case OrbitType::TUNDRA:
                num = (static_cast<double>(info.generator->Next(0, 100)) < info.inclinationFactor * 50.0) ? 116.6 : 63.4;
                break;
            case OrbitType::SYNCHRONOUS:
            case OrbitType::POLAR:
            case OrbitType::EQUATORIAL:
            case OrbitType::RANDOM:
                if (static_cast<double>(info.generator->Next(0, 100)) < info.inclinationFactor * 50.0) num = 180.0 - num;
                break;
            default: break;
        }
    } else {
        switch (info.type) {
            case OrbitType::KOLNIYA:
            case OrbitType::TUNDRA: num = CoinFlip(info) ? 116.6 : 63.4; break;
            case OrbitType::SYNCHRONOUS:
            case OrbitType::POLAR:
            case OrbitType::EQUATORIAL:
            case OrbitType::RANDOM:
                if (CoinFlip(info)) num = 180.0 - num;
                break;
            default: break;
        }
    }
    info.orbit.inclination = num;
}

// OrbitUtilities.cs:366
void GenerateLongitudeOfAscendingNode(OrbitGenerationInfo& info) {
    info.orbit.LAN = (info.type == OrbitType::EQUATORIAL || info.type == OrbitType::STATIONARY) ? 0.0 : (info.generator->NextDouble() * 360.0);
}

// OrbitUtilities.cs:371
void GenerateArgumentOfPeriapsis(OrbitGenerationInfo& info) {
    switch (info.type) {
        case OrbitType::KOLNIYA:
        case OrbitType::TUNDRA: info.orbit.argumentOfPeriapsis = CoinFlip(info) ? 270.0 : 90.0; break;
        case OrbitType::SYNCHRONOUS:
        case OrbitType::STATIONARY:
        case OrbitType::POLAR:
        case OrbitType::EQUATORIAL:
        case OrbitType::RANDOM: info.orbit.argumentOfPeriapsis = info.generator->NextDouble() * 360.0; break;
    }
}

// OrbitUtilities.cs:388
void GenerateEpoch(OrbitGenerationInfo& info) { info.orbit.epoch = 0.999 + info.generator->NextDouble() * 0.0019999999999998908; }

// OrbitUtilities.cs:393
void GenerateMeanAnomalyAtEpoch(OrbitGenerationInfo& info) { info.orbit.meanAnomalyAtEpoch = info.generator->NextDouble() * 2.0 * 3.141592653589793; }

// OrbitUtilities.cs:398
void GenerateFinalization(OrbitGenerationInfo& info) {
    if (info.type == OrbitType::EQUATORIAL || info.type == OrbitType::STATIONARY) info.orbit.an = Vector3d::zero();
    info.orbit.Init();
    if (info.orbit.referenceBody != nullptr)
        info.orbit.UpdateFromStateVectors(info.orbit.getRelativePositionAtUT(0.0), info.orbit.getOrbitalVelocityAtUT(0.0), info.orbit.referenceBody, 0.0);
    else
        log::Error("Cannot finalize procedural orbit with no reference body.");
}

bool BadNumber(double v) { return std::isnan(v) || std::isinf(v); }

}  // namespace

// OrbitUtilities.cs:79
Vector3d PositionOfApoapsis(const Orbit& o) { return o.getPositionFromTrueAnomaly(3.141592653589793); }

// OrbitUtilities.cs:84
Vector3d PositionOfPeriapsis(const Orbit& o) { return o.getPositionFromTrueAnomaly(0.0); }

// OrbitUtilities.cs:89
double AngleOfAscendingNode(const Orbit& currentOrbit, const Orbit& targetOrbit) {
    const Vector3d lhs = -currentOrbit.GetOrbitNormal().xzy();
    const Vector3d rhs = -targetOrbit.GetOrbitNormal().xzy();
    const Vector3d vector3d = Vector3d::Cross(lhs, rhs);
    const Vector3d xzy = currentOrbit.getRelativePositionAtT(0.0).xzy();
    const double num = Vector3d::Angle(xzy, vector3d);
    if (!(M::Abs(Vector3d::Angle(vector3d, Vector3d::Cross(lhs, xzy))) < 90.0)) return 360.0 - num;
    return num;
}

// OrbitUtilities.cs:102
double AngleOfDescendingNode(const Orbit& currentOrbit, const Orbit& targetOrbit) {
    return std::fmod(AngleOfAscendingNode(currentOrbit, targetOrbit) + 180.0, 360.0);
}

// OrbitUtilities.cs:107 (Vector3.Dot of the two normalised Vector3d values converted to Vector3: float components, float dot)
double GetRelativeInclination(const Orbit& a, const Orbit& b) {
    const Vector3f na = a.GetOrbitNormal().normalized().ToVector3f();
    const Vector3f nb = b.GetOrbitNormal().normalized().ToVector3f();
    const float dot = f32store(fx(na.x) * fx(nb.x) + fx(na.y) * fx(nb.y) + fx(na.z) * fx(nb.z));
    return M::Acos(static_cast<double>(dot)) * 57.295780181884766;
}

// OrbitUtilities.cs:112
OrbitType IdentifyOrbit(const Orbit& o) {
    const double significantDeviation = static_cast<double>(ContractDefsLite::F("Satellite", "", "SignificantDeviation"));
    const double v = o.referenceBody->minOrbitalDistance * 1.05;
    const double v2 = CU::SynchronousSMA(o.referenceBody);
    const bool flag = M::Abs(M::Abs(o.inclination) - 116.6) <= significantDeviation / 100.0 * 90.0 ||
                      M::Abs(M::Abs(o.inclination) - 63.4) <= significantDeviation / 100.0 * 90.0;
    if (SU::WithinDeviation(o.semiMajorAxis, v2, significantDeviation)) {
        if (flag && SU::WithinDeviation(o.PeR(), v, significantDeviation)) return OrbitType::TUNDRA;
        if (M::Abs(o.inclination) <= significantDeviation && SU::WithinDeviation(o.PeR(), o.ApR(), significantDeviation)) return OrbitType::STATIONARY;
        return OrbitType::SYNCHRONOUS;
    }
    if (SU::WithinDeviation(o.semiMajorAxis, CU::KolniyaSMA(o.referenceBody), significantDeviation) && flag &&
        SU::WithinDeviation(o.PeR(), v, significantDeviation))
        return OrbitType::KOLNIYA;
    if (std::fmod(M::Abs(o.inclination), 180.0) <= significantDeviation) return OrbitType::EQUATORIAL;
    if (std::fmod(M::Abs(o.inclination - 90.0), 180.0) <= significantDeviation) return OrbitType::POLAR;
    return OrbitType::RANDOM;
}

// OrbitUtilities.cs:154
Orbit GenerateOrbit(int seed, CelestialBody* targetBody, OrbitType orbitType, double altitudeDifficulty, double inclinationDifficulty,
                    double eccentricityOverride) {
    if (targetBody == nullptr) {
        log::Warning("Orbit generation cannot generate with null celestial bodies, defaulting to home body.");
        targetBody = CareerEnvironment::Current().HomeBody();
    }
    OrbitGenerationInfo info(seed, targetBody, orbitType, altitudeDifficulty, inclinationDifficulty, eccentricityOverride);
    GenerateApsides(info);
    GenerateInclination(info);
    GenerateLongitudeOfAscendingNode(info);
    GenerateArgumentOfPeriapsis(info);
    GenerateEpoch(info);
    GenerateMeanAnomalyAtEpoch(info);
    GenerateFinalization(info);
    return info.orbit;
}

// OrbitUtilities.cs:172
bool ValidateOrbit(int seed, Orbit& orbit, OrbitType orbitType, double altitudeDifficulty, double inclinationDifficulty, const std::string& source) {
    OrbitGenerationInfo info(seed, orbit, orbitType, altitudeDifficulty, inclinationDifficulty);
    std::vector<std::string> list;
    bool flag = false;
    if (BadNumber(info.orbit.eccentricity)) {
        GenerateApsides(info);
        list.push_back("eccentricity");
        flag = true;
    }
    info.eccentricityOverride = info.orbit.eccentricity;
    if (BadNumber(info.orbit.semiMajorAxis)) {
        GenerateApsides(info);
        list.push_back("semi-major axis");
        flag = true;
    }
    if (BadNumber(info.orbit.inclination)) {
        GenerateInclination(info);
        list.push_back("inclination");
        flag = true;
    }
    if (BadNumber(info.orbit.LAN)) {
        GenerateLongitudeOfAscendingNode(info);
        list.push_back("longitude of ascending node");
        flag = true;
    }
    if (BadNumber(info.orbit.argumentOfPeriapsis)) {
        GenerateArgumentOfPeriapsis(info);
        list.push_back("argument of periapsis");
        flag = true;
    }
    if (BadNumber(info.orbit.epoch)) {
        GenerateEpoch(info);
        list.push_back("epoch");
        flag = true;
    }
    if (BadNumber(info.orbit.meanAnomalyAtEpoch)) {
        GenerateMeanAnomalyAtEpoch(info);
        list.push_back("mean anomaly at epoch");
        flag = true;
    }
    if (flag) {
        GenerateFinalization(info);
        log::Warning("Generated orbit around " + info.orbit.referenceBody->bodyName + (!source.empty() ? (" for " + source) : std::string()) +
                     " had the following invalid orbital parameters: " + Contracts::Lite::StringUtilities::ThisThisAndThat(list) +
                     ". They were adjusted to make it valid again.");
        orbit = info.orbit;
    }
    return !flag;
}

}  // namespace ksp::FinePrint::Utilities::OrbitUtilities
