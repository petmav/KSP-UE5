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
    // The harness golden (physics_constants) records physicsDatabaseFilename as the author's absolute path. Re-root the
    // recorded string to that canonical form so the gate does not depend on where the repository is checked out
    // (Linux CI, other drives); the file that was actually read is env->physicsCfgPath.
    env->physics->physicsDatabaseFilename = "C:\\Projects\\KSP-UE5\\harness\\inputs\\Physics.cfg";
    env->physics->bodyLiftCurve = PhysicsGlobals::GetLiftingSurfaceCurve("BodyLift");
    // KspEnv.InitPlanetarium
    env->planetarium = std::make_unique<Planetarium>();
    env->planetarium->Awake();
    // KspEnv.InitBodies(0)
    env->system = CelestialSystem::Spawn(KerbolSystemPrefab(), *env->planetarium);
    UseHarnessFrame(*env->planetarium);
    env->physics->CalculateValues(env->system->Bodies(), env->planetarium->Sun);
    return *env;
}

// The harness's CBUpdate (Rehosted/CelestialBodyLifecycle.cs:106-147) leaves out the inverse-rotation branch
// (CelestialBody.cs:560-565: Planetarium.InverseRotAngle, Zup and Rotation). The Mun and Minmus prefabs carry
// inverseRotation = 1, so kspcore's faithful first CBUpdate pass (CelestialSystem::Spawn) leaves InverseRotAngle at
// 230 (their initialRotation at UT 0) and every orbit updated after the Mun in a Zup turned by 230 degrees, while every
// golden orbit state is in the frame Planetarium.Awake builds for the serialized inverseRotAngle 0. Mirror the harness:
// restore that frame and redo OrbitDriver.UpdateOrbit (updateFromParameters -> Orbit.UpdateFromUT) for every body in the
// CBUpdate pass order (Planetarium.UpdateCBsRecursive). Only the frame and the orbit states / positions change; the
// non-inverse bodies' directRotAngle/BodyFrame from that pass are not gated and are left as computed.
void CelestialEnv::UseHarnessFrame(Planetarium& planetarium) {
    planetarium.inverseRotAngle = 0.0;  // scene_singletons.serialized.json: Planetarium.inverseRotAngle
    PlanetariumAwakeFrames(planetarium.inverseRotAngle, Planetarium::Zup, planetarium.rotation);
    struct Pass {
        static void Run(CelestialBody* cb) {  // Planetarium.cs:268 UpdateCBsRecursive, UpdateOrbit part only
            if (cb->orbitDriver && cb->orbitDriver->link != nullptr) cb->orbitDriver->link->UpdateOrbit();
            for (CelestialBody* c : cb->orbitingBodies)
                if (!c->orbitDriver || !c->orbitDriver->reverse) Run(c);
            if (cb->orbitDriver && cb->orbitDriver->reverse) Run(cb->referenceBody());
        }
    };
    Pass::Run(planetarium.FindRootBody(planetarium.Sun));
}

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
