#include "celestial_env.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <stdexcept>

#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/math/util_math.hpp"

using namespace ksp;

CelestialEnv& CelestialEnv::Get(const RunnerOptions& opt) {
    static std::unique_ptr<CelestialEnv> env;
    if (env) return *env;
    env = std::make_unique<CelestialEnv>();
    // KspEnv.InitPhysicsGlobals: Path.Combine(Run.InputsDir, "Physics.cfg") (Windows separators)
    std::string harness = opt.harness;
    std::replace(harness.begin(), harness.end(), '/', '\\');
    while (!harness.empty() && harness.back() == '\\') harness.pop_back();
    env->physicsCfgPath = harness + "\\inputs\\Physics.cfg";
    env->physics = std::make_unique<PhysicsGlobals>();
    env->physics->physicsDatabaseFilename = env->physicsCfgPath;
    env->physics->Awake("");
    if (!env->physics->LoadDatabase()) throw std::runtime_error("PhysicsGlobals.LoadDatabase failed for " + env->physicsCfgPath);
    env->physics->bodyLiftCurve = PhysicsGlobals::GetLiftingSurfaceCurve("BodyLift");
    // KspEnv.InitPlanetarium
    env->planetarium = std::make_unique<Planetarium>();
    env->planetarium->Awake();
    // KspEnv.InitBodies(0)
    env->system = CelestialSystem::Spawn(KerbolSystemPrefab(), *env->planetarium, [](CelestialBody& b) {
        return std::unique_ptr<ICelestialBodyOrbit>(new StandInOrbitPeriod(b));
    });
    env->physics->CalculateValues(env->system->Bodies(), env->planetarium->Sun);
    return *env;
}

StandInOrbitPeriod::StandInOrbitPeriod(const CelestialBody& body) {
    const CelestialOrbitElements& o = body.orbitDriver->orbit;
    // Orbit.cs:334 GetMeanMotion
    const double num = dotnet::Math::Abs(o.semiMajorAxis);
    const double meanMotion = dotnet::Math::Sqrt(body.orbitDriver->referenceBody->gravParameter / (num * num * num));
    // Orbit.cs:368-376
    period_ = o.eccentricity < 1.0 ? UtilMath::PI * 2.0 / meanMotion : std::numeric_limits<double>::infinity();
}

namespace {
[[noreturn]] void Pending(const char* what) {
    throw std::logic_error(std::string("StandInOrbitPeriod::") + what + ": needs ksp::Orbit (WP-07)");
}
}  // namespace
double StandInOrbitPeriod::trueAnomaly() const { Pending("trueAnomaly"); }
double StandInOrbitPeriod::radius() const { Pending("radius"); }
double StandInOrbitPeriod::PeR() const { Pending("PeR"); }
double StandInOrbitPeriod::ApR() const { Pending("ApR"); }
Vector3d StandInOrbitPeriod::vel() const { Pending("vel"); }
Vector3d StandInOrbitPeriod::getRelativePositionAtUT(double) const { Pending("getRelativePositionAtUT"); }
Vector3d StandInOrbitPeriod::getOrbitalVelocityAtUT(double) const { Pending("getOrbitalVelocityAtUT"); }

namespace {
std::string Normalize(std::string s) {
    for (char& c : s)
        if (c == 'E') c = 'e';
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos) s += ".0";
    return s;
}
}  // namespace

std::string ExactFormatDouble(double v) {
    if (std::isnan(v)) return "NaN";
    if (std::isinf(v)) return v > 0 ? "Infinity" : "-Infinity";
    if (v == 0.0) return std::signbit(v) ? "-0.0" : "0.0";
    for (int p = 15; p <= 17; ++p) {
        const std::string s = dotnet::FormatDoubleG(v, p, dotnet::Runtime::NetFramework);
        double back = 0.0;
        const auto r = std::from_chars(s.data(), s.data() + s.size(), back);
        if (r.ec == std::errc() && std::memcmp(&back, &v, sizeof v) == 0) return Normalize(s);
    }
    throw std::runtime_error("no round-trip representation");
}

std::string ExactFormatFloat(float f) {
    if (std::isnan(f)) return "NaN";
    if (std::isinf(f)) return f > 0 ? "Infinity" : "-Infinity";
    if (f == 0.0f) return std::signbit(f) ? "-0.0" : "0.0";
    for (int p = 6; p <= 9; ++p) {
        const std::string s = dotnet::FormatSingleG(f, p, dotnet::Runtime::NetFramework);
        double back = 0.0;  // ExactNumber.ParseDouble (correctly rounded) then (float)
        const auto r = std::from_chars(s.data(), s.data() + s.size(), back);
        if (r.ec == std::errc() && static_cast<float>(back) == f) return Normalize(s);
    }
    throw std::runtime_error("no round-trip representation for float");
}
