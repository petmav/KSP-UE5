// Planetarium.cs ported (paths relative to decompiled/KSP-Decompiled/Assembly-CSharp/).
#include "ksp/celestial/planetarium.hpp"

#include <cmath>
#include <stdexcept>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/exceptions.hpp"  // NullReferenceException
#include "ksp/dotnet/math.hpp"

namespace ksp {

// Planetarium.cs:74 `public static CelestialFrame Zup` (zero in KSP until the first Awake; see planetarium.hpp)
CelestialFrame Planetarium::Zup = Planetarium::AwakeZup(0.0);

// Planetarium.cs:193 OnDestroy
Planetarium::~Planetarium() {
    if (fetch == this) fetch = nullptr;
}

// Planetarium.cs:182
void Planetarium::Awake() {
    if (fetch != nullptr) throw std::runtime_error("Don't try to instantiate the singleton Planetarium class more than once!");
    fetch = this;
    PlanetariumAwakeFrames(inverseRotAngle, Zup, rotation);
}

// Planetarium.cs:201 (the profiling hooks present in the instrumented decompile are omitted)
void Planetarium::FixedUpdate() {
    if (!pause) {
        time += fixedDeltaTime * timeScale;
        if (Sun != nullptr) {
            CurrentMainBody = FindRootBody(Sun);
            UpdateCBsRecursive(CurrentMainBody);
        }
    }
}

// Planetarium.cs:238
void Planetarium::UpdateCBs() {
    if (Sun != nullptr) {
        CurrentMainBody = FindRootBody(Sun);
        UpdateCBsRecursive(CurrentMainBody);
    }
}

// Planetarium.cs:247: descend into the first child whose OrbitDriver is reversed (KSP keeps the home body fixed)
CelestialBody* Planetarium::FindRootBody(CelestialBody* cb) const {
    for (CelestialBody* c : cb->orbitingBodies)
        if (c->orbitDriver && c->orbitDriver->reverse) return FindRootBody(c);
    return cb;
}

// Planetarium.cs:268
void Planetarium::UpdateCBsRecursive(CelestialBody* cb) {
    cb->CBUpdate();
    const size_t count = cb->orbitingBodies.size();
    for (size_t i = 0; i < count; ++i) {
        CelestialBody* c = cb->orbitingBodies[i];
        if (!c->orbitDriver || !c->orbitDriver->reverse) UpdateCBsRecursive(c);
    }
    if (cb->orbitDriver && cb->orbitDriver->reverse) UpdateCBsRecursive(cb->referenceBody());
}

// Planetarium.cs:127 Rotation / :139 InverseRotAngle (fetch.rotation, fetch.inverseRotAngle). Without an instance KSP
// throws NullReferenceException; the getter answers with Awake(0)'s frame instead (planetarium.hpp), the setters throw.
QuaternionD Planetarium::Rotation() {
    if (fetch == nullptr) {
        static const QuaternionD awake0 = AwakeRotation(0.0);
        return awake0;
    }
    return fetch->rotation;
}

void Planetarium::SetRotation(const QuaternionD& q) {
    if (fetch == nullptr) throw NullReferenceException("Planetarium.Rotation set: no Planetarium");
    fetch->rotation = q;
}

void Planetarium::SetInverseRotAngle(double a) {
    if (fetch == nullptr) throw NullReferenceException("Planetarium.InverseRotAngle set: no Planetarium");
    fetch->inverseRotAngle = a;
}

// Planetarium.cs:189-190
CelestialFrame Planetarium::AwakeZup(double inverseRotAngle) {
    CelestialFrame zup;
    QuaternionD rot;
    PlanetariumAwakeFrames(inverseRotAngle, zup, rot);
    return zup;
}

QuaternionD Planetarium::AwakeRotation(double inverseRotAngle) {
    CelestialFrame zup;
    QuaternionD rot;
    PlanetariumAwakeFrames(inverseRotAngle, zup, rot);
    return rot;
}

// Planetarium.cs:286
double Planetarium::GetUniversalTime() {
    if (fetch == nullptr) throw NullReferenceException("Planetarium.GetUniversalTime: no Planetarium and no HighLogic.CurrentGame");
    return fetch->time;
}

// Planetarium.cs:295
void Planetarium::SetUniversalTime(double t) {
    if (fetch != nullptr) fetch->time = t;
}

// Planetarium.cs:157
void Planetarium::ZupAtT(double UT, const CelestialBody& body, CelestialFrame& tempZup) {
    const double num = std::fmod(body.initialRotation + 360.0 * body.rotPeriodRecip * UT, 360.0);
    if (body.inverseRotation) {
        const double rot = std::fmod(num - body.directRotAngle, 360.0);
        CelestialFrame::PlanetaryFrame(0.0, 90.0, rot, tempZup);
    } else {
        tempZup.X = Zup.X;
        tempZup.Y = Zup.Y;
        tempZup.Z = Zup.Z;
    }
}

// Planetarium.cs:173
Vector3d Planetarium::SphericalVector(double lat, double lon) {
    const double num = dotnet::Math::Cos(lat);
    const double z = dotnet::Math::Sin(lat);
    const double num2 = dotnet::Math::Cos(lon);
    const double num3 = dotnet::Math::Sin(lon);
    return Vector3d(num * num2, num * num3, z);
}

// Planetarium.cs:303
bool Planetarium::FrameIsRotating() { return findRotatingBodiesRecursive(fetch->Sun); }

bool Planetarium::findRotatingBodiesRecursive(const CelestialBody* cb) {
    if (cb->rotates && cb->inverseRotation) return true;
    for (const CelestialBody* c : cb->orbitingBodies)
        if (findRotatingBodiesRecursive(c)) return true;
    return false;
}

}  // namespace ksp
