// Planetarium (Planetarium.cs, 314 lines): universal time, the inverse-rotation frame (Zup / rotation), Sun / Home /
// CurrentMainBody and the per-physics-frame update of every CelestialBody (UpdateCBsRecursive).
//
// KSP's singleton (Planetarium.fetch) is kept: Awake() registers the instance, the destructor clears it; static members
// mirror C#'s statics (Zup is static in C#). Process-wide, not thread-safe (like KSP).
//
// This is the one Planetarium of kspcore: CelestialBody (CBUpdate, surface vectors) and Orbit (Zup, up/right, ZupAtT)
// both use it. Deviation, only observable without a Planetarium instance: KSP's static Zup is the zero frame until the
// first Planetarium.Awake and Rotation/InverseRotAngle throw NullReferenceException without `fetch`; kspcore starts Zup
// at the frame Awake produces for the serialized inverseRotAngle 0 (PlanetaryFrame(0, 90, 0), the identity up to ~6e-17
// cross terms) and answers Rotation()/InverseRotAngle() with that frame's values when `fetch` is null, so Orbit works
// standalone (unit tests). The setters still need an instance (NullReferenceException, as in KSP).
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
    static CelestialFrame Zup;  // Planetarium.cs:74; starts as AwakeZup(0.0) (see the deviation note above)

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
    static QuaternionD Rotation();                // fetch.rotation (no fetch: Awake(0)'s rotation)
    static void SetRotation(const QuaternionD& q);  // fetch.rotation = q (no fetch: NullReferenceException)
    static double InverseRotAngle() { return fetch == nullptr ? 0.0 : fetch->inverseRotAngle; }
    static void SetInverseRotAngle(double a);       // fetch.inverseRotAngle = a (no fetch: NullReferenceException)
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

    // Planetarium.Awake's frame statements (Planetarium.cs:189-190) for a given inverseRotAngle: Zup = PlanetaryFrame(0,
    // 90, angle) and rotation = QuaternionD.Inverse(Zup.Rotation).swizzle.
    static CelestialFrame AwakeZup(double inverseRotAngle);
    static QuaternionD AwakeRotation(double inverseRotAngle);

private:
    static bool findRotatingBodiesRecursive(const CelestialBody* cb);
};

}  // namespace ksp
