// Planetarium (Planetarium.cs, 314 lines): universal time, the inverse-rotation frame (Zup / rotation), Sun / Home /
// CurrentMainBody and the per-physics-frame update of every CelestialBody (UpdateCBsRecursive).
//
// KSP's singleton (Planetarium.fetch) is kept: Awake() registers the instance, the destructor clears it; static members
// mirror C#'s statics (Zup is static in C#). Process-wide, not thread-safe (like KSP).
#pragma once

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/math/vector3d.hpp"

namespace ksp {

class CelestialBody;

class Planetarium {
public:
    // serialized (level9 scene; harness/golden/serialized/scene_singletons.serialized.json: all defaults)
    double time = 0.0;
    double timeScale = 1.0;
    bool pause = false;
    QuaternionD rotation;
    double inverseRotAngle = 0.0;
    CelestialBody* Sun = nullptr;
    CelestialBody* Home = nullptr;
    CelestialBody* CurrentMainBody = nullptr;
    double fixedDeltaTime = 0.02;

    static inline Planetarium* fetch = nullptr;
    static inline CelestialFrame Zup{};

    Planetarium() = default;
    Planetarium(const Planetarium&) = delete;
    Planetarium& operator=(const Planetarium&) = delete;
    ~Planetarium();  // OnDestroy (Planetarium.cs:193)

    void Awake();        // Planetarium.cs:182 (throws if another instance is registered, like C#)
    void FixedUpdate();  // Planetarium.cs:201: time += fixedDeltaTime * timeScale; UpdateCBs
    void UpdateCBs();    // Planetarium.cs:238
    CelestialBody* FindRootBody(CelestialBody* cb) const;  // Planetarium.cs:247
    void UpdateCBsRecursive(CelestialBody* cb);            // Planetarium.cs:268

    // statics (Planetarium.cs:85-180, 286-314)
    static double TimeScale() { return fetch == nullptr ? 1.0 : fetch->timeScale; }
    static bool Pause() { return fetch != nullptr && fetch->pause; }
    static QuaternionD Rotation() { return fetch->rotation; }
    static void SetRotation(const QuaternionD& q) { fetch->rotation = q; }
    static double InverseRotAngle() { return fetch->inverseRotAngle; }
    static void SetInverseRotAngle(double a) { fetch->inverseRotAngle = a; }
    static Vector3d up() { return Rotation() * Vector3d(0.0, 1.0, 0.0); }
    static Vector3d forward() { return Rotation() * Vector3d(0.0, 0.0, 1.0); }
    static Vector3d right() { return Rotation() * Vector3d(1.0, 0.0, 0.0); }
    // GetUniversalTime: fetch.time, else HighLogic.CurrentGame.UniversalTime (no game object in kspcore yet: throws
    // NullReferenceException like KSP without a current game).
    static double GetUniversalTime();
    static void SetUniversalTime(double t);
    static void ZupAtT(double UT, const CelestialBody& body, CelestialFrame& tempZup);
    static Vector3d SphericalVector(double lat, double lon);
    static bool FrameIsRotating();

private:
    static bool findRotatingBodiesRecursive(const CelestialBody* cb);
};

}  // namespace ksp
